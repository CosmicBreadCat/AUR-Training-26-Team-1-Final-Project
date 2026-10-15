#include "MPU_int.h"
#include "MPU_cfg.h"

I2C_Status_t MPU6050_vInit(I2C_Port_t Copy_xPort) {
    /* 
     * Write 0x00 to Power Management Register 1 (0x6B) 
     * This clears the SLEEP bit, wakes up the sensor, and selects internal 8MHz oscillator.
     */
    return I2C_xWriteRegister(Copy_xPort, MPU6050_I2C_ADDR,
                              MPU6050_REG_PWR_MGMT_1, 0x00);
}

I2C_Status_t MPU6050_vReadData(I2C_Port_t Copy_xPort, MPU6050_Data_t *PsData) {
    if (PsData == NULL) {
        return I2C_ERR_FAIL;
    }

    uint8_t u8Buffer[14];

    /* 
     * Burst read 14 consecutive bytes starting from ACCEL_XOUT_H (0x3B):
     * - Bytes 0-5:   Accel X, Y, Z
     * - Bytes 6-7:   Temperature
     * - Bytes 8-13:  Gyro X, Y, Z
     */
    I2C_Status_t xStatus = I2C_xReadBytes(
        Copy_xPort, MPU6050_I2C_ADDR, MPU6050_REG_ACCEL_XOUT_H, u8Buffer, 14);
    if (xStatus != I2C_OK) {
        return xStatus;
    }
        
        /* Combine high and low bytes for Accelerometer */
        int16_t s16RawAccelX = (int16_t)((u8Buffer[0] << 8) | u8Buffer[1]);
        int16_t s16RawAccelY = (int16_t)((u8Buffer[2] << 8) | u8Buffer[3]);
        int16_t s16RawAccelZ = (int16_t)((u8Buffer[4] << 8) | u8Buffer[5]);

        /* Combine high and low bytes for Gyroscope (skipping temperature bytes 6 & 7) */
        int16_t s16RawGyroX  = (int16_t)((u8Buffer[8] << 8) | u8Buffer[9]);
        int16_t s16RawGyroY  = (int16_t)((u8Buffer[10] << 8) | u8Buffer[11]);
        int16_t s16RawGyroZ  = (int16_t)((u8Buffer[12] << 8) | u8Buffer[13]);

        /* Convert raw ADC values to physical units ($g$ and degrees/second) */
        PsData->fAccelX = (float)s16RawAccelX / MPU6050_ACCEL_SENSITIVITY;
        PsData->fAccelY = (float)s16RawAccelY / MPU6050_ACCEL_SENSITIVITY;
        PsData->fAccelZ = (float)s16RawAccelZ / MPU6050_ACCEL_SENSITIVITY;

        PsData->fGyroX  = (float)s16RawGyroX  / MPU6050_GYRO_SENSITIVITY;
        PsData->fGyroY  = (float)s16RawGyroY  / MPU6050_GYRO_SENSITIVITY;
        PsData->fGyroZ  = (float)s16RawGyroZ  / MPU6050_GYRO_SENSITIVITY;

    return I2C_OK;
}