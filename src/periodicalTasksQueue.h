#ifndef PERIODICAL_TASKS_QUEUE_H
#define PERIODICAL_TASKS_QUEUE_H

#include <Arduino.h>
#include <vector>

using namespace std;

struct PeriodicalTasksQueueItem {
  void (*taskFunction)();
  unsigned long executionTimeMillis;
};

void checkPeriodicalTasksQueue();
void addPeriodicalTask(void (*taskFunction)(), unsigned long executionTimeMillis);
void addPeriodicalTaskInMillis(void (*taskFunction)(), unsigned long executionDelayMillis);

#endif