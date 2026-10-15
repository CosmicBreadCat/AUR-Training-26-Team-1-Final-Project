#include <Arduino.h>
#include "HAL/MPU6050/MPU_int.h"
#include "HAL/MPU6050/MPU_cfg.h"
#include "HAL/PCNT/PCNT_int.h"
#include "HAL/PCNT/PCNT_cfg.h"
#include "MCAL/I2C/I2C_cfg.h"
#include "APP/COMM_TASK/COMM_TASK_int.h"
#include "APP/CONTROL_TASK/CONTROL_TASK_int.h"

static const I2C_Port_t SENSOR_I2C_PORT = I2C_PORT_0;
static const uint8_t ULTRASONIC_TRIG_PIN = 26;
static const uint8_t ULTRASONIC_ECHO_PIN = 27;

static void vInitSensors(void) {
  pinMode(ULTRASONIC_TRIG_PIN, OUTPUT);
  pinMode(ULTRASONIC_ECHO_PIN, INPUT);
  digitalWrite(ULTRASONIC_TRIG_PIN, LOW);
  Serial.println("Ultrasonic sensor initialized (TRIG=GPIO26, ECHO=GPIO27)");

  bool bI2cReady =
      I2C_xInitMaster(SENSOR_I2C_PORT, I2C_MASTER_SDA_PIN,
                      I2C_MASTER_SCL_PIN, I2C_MASTER_FREQ_HZ) == I2C_OK;
  if (!bI2cReady) {
    Serial.println("ERROR: I2C initialization failed; MPU6050 unavailable");
  } else {
    uint8_t u8WhoAmI = 0;
    I2C_Status_t xReadStatus = I2C_xReadRegister(
        SENSOR_I2C_PORT, MPU6050_I2C_ADDR, MPU6050_REG_WHO_AM_I, &u8WhoAmI);
    if (xReadStatus != I2C_OK) {
      Serial.println("ERROR: MPU6050 did not respond on I2C address 0x68");
    } else if (u8WhoAmI != MPU6050_I2C_ADDR) {
      Serial.printf("ERROR: unexpected MPU6050 WHO_AM_I value 0x%02X\n",
                    u8WhoAmI);
    } else if (MPU6050_vInit(SENSOR_I2C_PORT) != I2C_OK) {
      Serial.println("ERROR: MPU6050 wake-up command failed");
    } else {
      Serial.println("MPU6050 detected and initialized");
    }
  }

  PCNT_vInitEncoder(PCNT_UNIT_INDEX_0, ENCODER_LEFT_A_PIN,
                    ENCODER_LEFT_B_PIN);
  PCNT_vInitEncoder(PCNT_UNIT_INDEX_1, ENCODER_RIGHT_A_PIN,
                    ENCODER_RIGHT_B_PIN);
  Serial.println("Wheel encoders initialized; rotate wheels to test pulses");
}

void setup() {
  Serial.begin(115200);
  delay(200);

  vInitSensors();

  if (!COMMTASK_bStart()) {
    Serial.println("ERROR: comm task failed to start");
  }
  if (!CONTROLTASK_bStart()) {
    Serial.println("control task failed to start");
  }
}

void loop() {
  vTaskDelete(NULL);  // all work happens in tasks
}
