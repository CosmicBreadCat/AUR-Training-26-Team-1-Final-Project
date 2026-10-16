#include "I2C_int.h"
#include "I2C_cfg.h"

I2C_Status_t I2C_xInitMaster(I2C_Port_t Copy_xPort, int Copy_s32SdaPin, int Copy_s32SclPin, uint32_t Copy_u32ClkSpeedHz) {
    i2c_config_t xI2cConfig = {
        .mode             = I2C_MODE_MASTER,
        .sda_io_num       = Copy_s32SdaPin,
        .scl_io_num       = Copy_s32SclPin,
        .sda_pullup_en    = GPIO_PULLUP_ENABLE,
        .scl_pullup_en    = GPIO_PULLUP_ENABLE,
        .master.clk_speed = Copy_u32ClkSpeedHz,
        .clk_flags        = 0
    };

    if (i2c_param_config((i2c_port_t)Copy_xPort, &xI2cConfig) != ESP_OK) {
        return I2C_ERR_FAIL;
    }

    if (i2c_driver_install((i2c_port_t)Copy_xPort, I2C_MODE_MASTER, 0, 0, 0) != ESP_OK) {
        return I2C_ERR_FAIL;
    }

    return I2C_OK;
}

I2C_Status_t I2C_xWriteRegister(I2C_Port_t Copy_xPort, uint8_t Copy_u8DevAddr, uint8_t Copy_u8RegAddr, uint8_t Copy_u8Data) {
    uint8_t u8TxBuf[2] = {Copy_u8RegAddr, Copy_u8Data};

    esp_err_t xErr = i2c_master_write_to_device(
        (i2c_port_t)Copy_xPort,
        Copy_u8DevAddr,
        u8TxBuf,
        sizeof(u8TxBuf),
        pdMS_TO_TICKS(I2C_MASTER_TIMEOUT_MS)
    );

    return (xErr == ESP_OK) ? I2C_OK : I2C_ERR_FAIL;
}

I2C_Status_t I2C_xReadRegister(I2C_Port_t Copy_xPort, uint8_t Copy_u8DevAddr, uint8_t Copy_u8RegAddr, uint8_t *Pu8Data) {
    return I2C_xReadBytes(Copy_xPort, Copy_u8DevAddr, Copy_u8RegAddr, Pu8Data, 1);
}

I2C_Status_t I2C_xReadBytes(I2C_Port_t Copy_xPort, uint8_t Copy_u8DevAddr, uint8_t Copy_u8StartRegAddr, uint8_t *Pu8Buffer, uint32_t Copy_u32Len) {
    if (Pu8Buffer == (void *)0) {
        return I2C_ERR_FAIL;
    }

    esp_err_t xErr = i2c_master_write_read_device(
        (i2c_port_t)Copy_xPort,
        Copy_u8DevAddr,
        &Copy_u8StartRegAddr,
        1,
        Pu8Buffer,
        Copy_u32Len,
        pdMS_TO_TICKS(I2C_MASTER_TIMEOUT_MS)
    );

    return (xErr == ESP_OK) ? I2C_OK : I2C_ERR_FAIL;
}