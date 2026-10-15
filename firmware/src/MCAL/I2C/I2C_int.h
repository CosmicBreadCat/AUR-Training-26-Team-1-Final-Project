#ifndef I2C_INT_H_
#define I2C_INT_H_

#include <stdint.h>
#include "driver/i2c.h"

#ifdef __cplusplus
extern "C" {
#endif

/* I2C Peripheral Index */
typedef enum {
    I2C_PORT_0 = I2C_NUM_0,
    I2C_PORT_1 = I2C_NUM_1
} I2C_Port_t;

/* Driver Status Codes */
typedef enum {
    I2C_OK = 0,
    I2C_ERR_FAIL,
    I2C_ERR_TIMEOUT
} I2C_Status_t;

/* Function Prototypes */

/**
 * @brief Initializes the ESP32 I2C peripheral in Master mode.
 * @param Copy_xPort Target I2C port (I2C_PORT_0 or I2C_PORT_1).
 * @param Copy_s32SdaPin GPIO pin for SDA line.
 * @param Copy_s32SclPin GPIO pin for SCL line.
 * @param Copy_u32ClkSpeedHz Clock frequency in Hz (e.g., 400000).
 * @return I2C_Status_t Status code (I2C_OK on success).
 */
I2C_Status_t I2C_xInitMaster(I2C_Port_t Copy_xPort, int Copy_s32SdaPin, int Copy_s32SclPin, uint32_t Copy_u32ClkSpeedHz);

/**
 * @brief Writes a single byte to a specific slave register.
 * @param Copy_xPort Target I2C port.
 * @param Copy_u8DevAddr 7-bit slave I2C device address.
 * @param Copy_u8RegAddr Register address to write to.
 * @param Copy_u8Data Byte value to write.
 * @return I2C_Status_t Status code.
 */
I2C_Status_t I2C_xWriteRegister(I2C_Port_t Copy_xPort, uint8_t Copy_u8DevAddr, uint8_t Copy_u8RegAddr, uint8_t Copy_u8Data);

/**
 * @brief Reads a single byte from a specific slave register.
 * @param Copy_xPort Target I2C port.
 * @param Copy_u8DevAddr 7-bit slave I2C device address.
 * @param Copy_u8RegAddr Register address to read from.
 * @param Pu8Data Pointer to store the received byte.
 * @return I2C_Status_t Status code.
 */
I2C_Status_t I2C_xReadRegister(I2C_Port_t Copy_xPort, uint8_t Copy_u8DevAddr, uint8_t Copy_u8RegAddr, uint8_t *Pu8Data);

/**
 * @brief Reads multiple consecutive bytes from a slave device (Burst Read).
 * @param Copy_xPort Target I2C port.
 * @param Copy_u8DevAddr 7-bit slave I2C device address.
 * @param Copy_u8StartRegAddr Starting register address.
 * @param Pu8Buffer Pointer to destination buffer.
 * @param Copy_u32Len Number of bytes to read.
 * @return I2C_Status_t Status code.
 */
I2C_Status_t I2C_xReadBytes(I2C_Port_t Copy_xPort, uint8_t Copy_u8DevAddr, uint8_t Copy_u8StartRegAddr, uint8_t *Pu8Buffer, uint32_t Copy_u32Len);

#ifdef __cplusplus
}
#endif

#endif /* I2C_INT_H_ */