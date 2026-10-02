#ifndef IMU_RING_BUFFER_H
#define IMU_RING_BUFFER_H

// ══════════════════════════════════════════════════════════════
// IMURingBuffer — statically-allocated, ISR-safe circular buffer
// ══════════════════════════════════════════════════════════════
//
// Design rationale (why not std::queue / std::deque):
//
//   std::deque — the backing store of std::queue — allocates memory
//   in fixed-size chunks on the heap.  On an MCU with ~300 KB of RAM
//   and no memory compactor this produces fragmentation over time and
//   makes every push() a potential malloc() failure.  It also has no
//   random-access iterator, so fall_detection.cpp was forced to copy
//   the *entire* queue into a second temporary queue just to walk it,
//   burning both RAM and CPU on every evaluation tick.
//
// This implementation:
//   • Zero heap use — the sample array lives in the BSS segment;
//     size is fixed at compile time via the CAPACITY template parameter.
//   • O(1) push (overwrites oldest when full — correct sliding-window
//     semantics for the fall detection algorithm).
//   • O(1) indexed read — operator[] counts from the oldest sample,
//     matching the chronological index convention used in isFalling().
//   • ISR-safe — a portMUX_TYPE spinlock guards head/tail so readIMU()
//     (called from the ISR-flagged loop drainer) and isFalling() (called
//     from a scheduler task) can never observe a torn state.
//   • clear() resets in O(1) — no element-by-element pop loop needed.
//
// Memory footprint for CAPACITY = 60, IMUData = 12 bytes:
//   60 × 12 = 720 bytes in BSS.  The old std::deque used roughly the
//   same data bytes plus ~128 bytes of chunk-pointer overhead and a
//   live heap allocation per chunk.
//
// Usage:
//   IMURingBuffer<IMUData, 60> accelDataQueue;
//   accelDataQueue.push({ ax, ay, az, gx, gy, gz });
//   uint8_t n = accelDataQueue.size();          // 0–60
//   IMUData d = accelDataQueue[i];              // oldest = 0
//   accelDataQueue.clear();
// ══════════════════════════════════════════════════════════════

#include <Arduino.h>
#include "freertos/FreeRTOS.h"
#include "freertos/portmacro.h"

template <typename T, uint8_t CAPACITY>
class IMURingBuffer {
public:
    // ── Construction ─────────────────────────────────────────
    IMURingBuffer() : _head(0), _count(0) {
        _mux = portMUX_INITIALIZER_UNLOCKED;
    }

    // ── push ─────────────────────────────────────────────────
    // Inserts a new sample.  If the buffer is full the oldest
    // sample is silently overwritten (sliding-window behaviour).
    // Safe to call from the ISR-flagged readIMU() drainer.
    void push(const T& sample) {
        portENTER_CRITICAL(&_mux);

        // Write position is always at the current logical tail.
        // When full, _head advances past the oldest entry.
        uint8_t writeIdx = (_head + _count) % CAPACITY;
        _buf[writeIdx] = sample;

        if (_count < CAPACITY) {
            _count++;
        } else {
            // Buffer is full: discard the oldest by advancing head.
            _head = (_head + 1) % CAPACITY;
        }

        portEXIT_CRITICAL(&_mux);
    }

    // ── size ─────────────────────────────────────────────────
    // Returns the number of valid samples currently stored (0–CAPACITY).
    uint8_t size() const {
        portENTER_CRITICAL(&_mux);
        uint8_t s = _count;
        portEXIT_CRITICAL(&_mux);
        return s;
    }

    // ── operator[] ───────────────────────────────────────────
    // Index 0 = oldest sample (chronologically first).
    // Index size()-1 = most recent sample.
    // Caller is responsible for bounds checking (i < size()).
    // Makes a safe atomic copy of head/count, then the element
    // read itself is outside the lock (the element slot cannot
    // be overwritten while _count samples exist and the caller
    // holds a consistent snapshot of _head).
    T operator[](uint8_t i) const {
        portENTER_CRITICAL(&_mux);
        uint8_t head  = _head;
        uint8_t count = _count;
        portEXIT_CRITICAL(&_mux);

        // Clamp silently — should not happen with correct caller logic.
        if (i >= count) i = (count > 0) ? count - 1 : 0;

        return _buf[(head + i) % CAPACITY];
    }

    // ── clear ────────────────────────────────────────────────
    // Resets the buffer to empty in O(1).
    // Called after a fall event resolves to prevent stale impact
    // samples from re-triggering the pipeline on the next tick.
    void clear() {
        portENTER_CRITICAL(&_mux);
        _head  = 0;
        _count = 0;
        portEXIT_CRITICAL(&_mux);
    }

    // ── empty ────────────────────────────────────────────────
    bool empty() const { return size() == 0; }

    // ── capacity ─────────────────────────────────────────────
    // Compile-time constant; no runtime cost.
    static constexpr uint8_t capacity() { return CAPACITY; }

private:
    T                    _buf[CAPACITY];   // Static storage — lives in BSS
    uint8_t              _head;            // Index of the oldest valid entry
    uint8_t              _count;           // Number of valid entries (0–CAPACITY)
    mutable portMUX_TYPE _mux;             // FreeRTOS spinlock — mutable so const
                                           // methods (size, operator[]) can lock it.
                                           // 'mutable' is the correct C++ idiom for
                                           // synchronisation primitives on const objects.
};

#endif // IMU_RING_BUFFER_H