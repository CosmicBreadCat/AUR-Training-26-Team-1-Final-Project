#include <Arduino.h>
#include "APP/COMM_TASK/COMM_TASK_int.h"

void setup() {
  Serial.begin(115200);
  delay(200);

  if (!COMMTASK_bStart()) {
    Serial.println("comm task failed to start");
  }
}

void loop() {
  vTaskDelete(NULL);  // all work happens in tasks
}
