#include "periodicalTasksQueue.h"

vector<PeriodicalTasksQueueItem> periodicalTasksQueue;

void checkPeriodicalTasksQueue() {
  if (periodicalTasksQueue.empty()) {
    return;
  }

  if (millis() >= periodicalTasksQueue[0].executionTimeMillis) {
    periodicalTasksQueue[0].taskFunction();
    periodicalTasksQueue.erase(periodicalTasksQueue.begin());
  }
}

void addPeriodicalTaskInMillis(void (*taskFunction)(), unsigned long executionDelayMillis) {
  addPeriodicalTask(taskFunction, millis() + executionDelayMillis);
}

void addPeriodicalTask(void (*taskFunction)(), unsigned long executionTimeMillis) {
  PeriodicalTasksQueueItem newItem;
  newItem.taskFunction = taskFunction;
  newItem.executionTimeMillis = executionTimeMillis;

  if (periodicalTasksQueue.empty()) {
    periodicalTasksQueue.push_back(newItem);
    return;
  }

  for (int i = 0; i < periodicalTasksQueue.size(); i++) {
    if (newItem.executionTimeMillis < periodicalTasksQueue[i].executionTimeMillis) {
      periodicalTasksQueue.insert(periodicalTasksQueue.begin() + i, newItem);
      return;
    }
  }

  // If the new item is the last one, add it to the end of the queue
  periodicalTasksQueue.push_back(newItem);
}