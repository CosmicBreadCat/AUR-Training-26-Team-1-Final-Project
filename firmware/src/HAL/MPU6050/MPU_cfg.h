#ifndef MPU6050_CFG_H_
#define MPU6050_CFG_H_

/* MPU6050 I2C 7-bit Address (AD0 pulled low to GND) */
#define MPU6050_I2C_ADDR             0x68

/* Register Map Definitions */
#define MPU6050_REG_PWR_MGMT_1       0x6B
#define MPU6050_REG_ACCEL_XOUT_H     0x3B
#define MPU6050_REG_WHO_AM_I         0x75

/* Sensitivity Scale Factors (Default settings: Accel = +-2g, Gyro = +-250 deg/s) */
#define MPU6050_ACCEL_SENSITIVITY    16384.0f
#define MPU6050_GYRO_SENSITIVITY     131.0f

#endif /* MPU6050_CFG_H_ */