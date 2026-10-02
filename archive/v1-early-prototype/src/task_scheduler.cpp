#include "task_scheduler.h"

// ── Run the cooperative scheduler ─────────────────────────────
uint32_t runTaskScheduler(Task* tasks, uint8_t taskCount) {
  uint32_t now = millis();
  uint32_t nextWakeIn = UINT32_MAX;

  // Evaluate execution demands across all defined structured modules
  for (uint8_t i = 0; i < taskCount; i++) {
    Task& t = tasks[i];
    uint32_t elapsed = now - t.lastRunMs;
    
    if (elapsed >= t.intervalMs) {
      t.handler();
      t.lastRunMs = now;
    } else {
      nextWakeIn = min(nextWakeIn, t.intervalMs - elapsed);
    }
  }

  return nextWakeIn;
}
