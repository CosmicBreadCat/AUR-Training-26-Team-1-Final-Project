#include <Arduino.h>
#include <math.h>
#include "APP/COMM_TASK/COMM_TASK_int.h"
#include "HAL/MPU6050/MPU_int.h"
#include "HAL/MPU6050/MPU_cfg.h"
#include "HAL/PCNT/PCNT_int.h"
#include "HAL/PCNT/PCNT_cfg.h"
#include "MCAL/I2C/I2C_cfg.h"

static const I2C_Port_t SENSOR_I2C_PORT = I2C_PORT_0;
static const uint32_t SENSOR_REPORT_PERIOD_MS = 500;
static const uint8_t ULTRASONIC_TRIG_PIN = 26;
static const uint8_t ULTRASONIC_ECHO_PIN = 27;
static const uint32_t ULTRASONIC_ECHO_TIMEOUT_US = 30000;

static bool g_bI2cReady = false;
static bool g_bMpuReady = false;
static uint32_t g_u32LastSensorReportMs = 0;
static int16_t g_s16PreviousLeftCount = 0;
static int16_t g_s16PreviousRightCount = 0;

static void vInitSensors(void) {
  pinMode(ULTRASONIC_TRIG_PIN, OUTPUT);
  pinMode(ULTRASONIC_ECHO_PIN, INPUT);
  digitalWrite(ULTRASONIC_TRIG_PIN, LOW);
  Serial.println("Ultrasonic sensor initialized (TRIG=GPIO26, ECHO=GPIO27)");

  g_bI2cReady =
      I2C_xInitMaster(SENSOR_I2C_PORT, I2C_MASTER_SDA_PIN,
                      I2C_MASTER_SCL_PIN, I2C_MASTER_FREQ_HZ) == I2C_OK;
  if (!g_bI2cReady) {
    Serial.println("ERROR: I2C initialization failed; MPU6050 unavailable");
  } else {
    uint8_t u8WhoAmI = 0;
    I2C_Status_t xReadStatus = I2C_xReadRegister(
        SENSOR_I2C_PORT,
        MPU6050_I2C_ADDR,
        MPU6050_REG_WHO_AM_I, &u8WhoAmI);
    if (xReadStatus != I2C_OK) {
      Serial.println("ERROR: MPU6050 did not respond on I2C address 0x68");
    } else if (u8WhoAmI != MPU6050_I2C_ADDR) {
      Serial.printf("ERROR: unexpected MPU6050 WHO_AM_I value 0x%02X\n",
                    u8WhoAmI);
    } else if (MPU6050_vInit(SENSOR_I2C_PORT) != I2C_OK) {
      Serial.println("ERROR: MPU6050 wake-up command failed");
    } else {
      g_bMpuReady = true;
      Serial.println("MPU6050 detected and initialized");
    }
  }

  PCNT_vInitEncoder(PCNT_UNIT_INDEX_0, ENCODER_LEFT_A_PIN,
                    ENCODER_LEFT_B_PIN);
  PCNT_vInitEncoder(PCNT_UNIT_INDEX_1, ENCODER_RIGHT_A_PIN,
                    ENCODER_RIGHT_B_PIN);
  Serial.println("Wheel encoders initialized; rotate wheels to test pulses");
}

static void vReportUltrasonicDistance(void) {
  digitalWrite(ULTRASONIC_TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(ULTRASONIC_TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(ULTRASONIC_TRIG_PIN, LOW);

  uint32_t u32EchoDurationUs =
      pulseIn(ULTRASONIC_ECHO_PIN, HIGH, ULTRASONIC_ECHO_TIMEOUT_US);
  if (u32EchoDurationUs == 0) {
    Serial.println("Ultrasonic: no echo (no target or check wiring)");
    return;
  }

  float fDistanceCm = (float)u32EchoDurationUs * 0.0343f / 2.0f;
  Serial.printf("Ultrasonic: %.1f cm\n", fDistanceCm);
}

static void vReportSensorStatus(void) {
  vReportUltrasonicDistance();

  if (!g_bI2cReady) {
    Serial.println("MPU6050: unavailable (I2C initialization failed)");
  } else if (!g_bMpuReady) {
    Serial.println("MPU6050: unavailable (check power, wiring, and address)");
  } else {
    MPU6050_Data_t xData = {};
    if (MPU6050_vReadData(SENSOR_I2C_PORT, &xData) != I2C_OK) {
      Serial.println("MPU6050: read failed (check I2C connection)");
    } else {
      float fAccelMagnitude = sqrtf(
          xData.fAccelX * xData.fAccelX +
          xData.fAccelY * xData.fAccelY +
          xData.fAccelZ * xData.fAccelZ);
      Serial.printf(
          "MPU6050: accel[g]=(%.2f, %.2f, %.2f) |accel|=%.2f g "
          "gyro[dps]=(%.2f, %.2f, %.2f)\n",
          xData.fAccelX, xData.fAccelY, xData.fAccelZ, fAccelMagnitude,
          xData.fGyroX, xData.fGyroY, xData.fGyroZ);
    }
  }

  int16_t s16LeftCount = PCNT_s16GetCount(PCNT_UNIT_INDEX_0);
  int16_t s16RightCount = PCNT_s16GetCount(PCNT_UNIT_INDEX_1);
  int32_t s32LeftDelta =
      (int32_t)s16LeftCount - (int32_t)g_s16PreviousLeftCount;
  int32_t s32RightDelta =
      (int32_t)s16RightCount - (int32_t)g_s16PreviousRightCount;
  Serial.printf("Encoders: left=%d (delta=%ld), right=%d (delta=%ld)\n",
                s16LeftCount, (long)s32LeftDelta, s16RightCount,
                (long)s32RightDelta);

  g_s16PreviousLeftCount = s16LeftCount;
  g_s16PreviousRightCount = s16RightCount;
}

void setup() {
  Serial.begin(115200);
  delay(200);

  vInitSensors();

  if (!COMMTASK_bStart()) {
    Serial.println("ERROR: comm task failed to start");
  }
}

void loop() {
  uint32_t u32Now = millis();
  if (u32Now - g_u32LastSensorReportMs >= SENSOR_REPORT_PERIOD_MS) {
    vReportSensorStatus();
    g_u32LastSensorReportMs = u32Now;
  }
  delay(1);
}
