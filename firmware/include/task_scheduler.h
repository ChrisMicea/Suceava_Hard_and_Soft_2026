#ifndef TASK_SCHEDULER_H
#define TASK_SCHEDULER_H

#include <Arduino.h>

// ── Task Scheduler Structure ──────────────────────────────────
struct Task {
    uint32_t intervalMs;
    uint32_t lastRunMs;
    void (*handler)();
};

// ── Run the cooperative scheduler ─────────────────────────────
uint32_t runTaskScheduler(Task* tasks, uint8_t taskCount);

#endif
