#ifndef MPU6050_INT_H_
#define MPU6050_INT_H_

#include <stdint.h>
#include "../../MCAL/I2C/I2C_int.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Structure to hold scaled sensor data */
typedef struct {
    float fAccelX;
    float fAccelY;
    float fAccelZ;
    float fGyroX;
    float fGyroY;
    float fGyroZ;
} MPU6050_Data_t;

/* Function Prototypes */

/**
 * @brief Initializes and wakes up the MPU6050 sensor over I2C.
 * @param Copy_xPort Target I2C port used.
 */
I2C_Status_t MPU6050_vInit(I2C_Port_t Copy_xPort);

/**
 * @brief Reads raw accelerometer and gyroscope data and converts to physical units.
 * @param Copy_xPort Target I2C port used.
 * @param PsData Pointer to data structure where sensor values will be stored.
 */
I2C_Status_t MPU6050_vReadData(I2C_Port_t Copy_xPort, MPU6050_Data_t *PsData);

#ifdef __cplusplus
}
#endif

#endif /* MPU6050_INT_H_ */