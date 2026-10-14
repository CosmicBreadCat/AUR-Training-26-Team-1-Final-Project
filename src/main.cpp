#include "APP/COMM_TASK/COMM_TASK_int.h"
#include "APP/CONTROL_TASK/CONTROL_TASK_int.h"
#include <Arduino.h>

void setup() {
  Serial.begin(115200);
  delay(200);

  if (!COMMTASK_bStart()) {
    Serial.println("comm task failed to start");
  }
  if (!CONTROLTASK_bStart()) {
    Serial.println("control task failed to start");
  }
}

void loop() {
  vTaskDelete(NULL); // all work happens in tasks
}
