#include "GPIO_int.h"
#include "GPIO_cfg.h"
#include "driver/gpio.h"
#include "soc/gpio_struct.h"

void GPIO_vInitPin(GPIO_Pin_t PinNum, GPIO_Mode_t Mode) {
    //Initialize ESP-IDF configuration structure 
    gpio_config_t xPinConfig = {0};

    //Set bit mask for the selected GPIO pin
    xPinConfig.pin_bit_mask = (1ULL << PinNum);
    xPinConfig.intr_type = GPIO_INTR_DISABLE;

    if (Mode == GPIO_MODE_OUTPUT) {
        xPinConfig.mode = GPIO_MODE_OUTPUT;
        xPinConfig.pull_up_en = GPIO_PULLUP_DISABLE;
        xPinConfig.pull_down_en = GPIO_PULLDOWN_DISABLE;

    }
     else if (Mode == GPIO_MODE_INPUT) {
        xPinConfig.mode = GPIO_MODE_INPUT;
        xPinConfig.pull_up_en = GPIO_PULLUP_DISABLE;
        xPinConfig.pull_down_en = GPIO_PULLDOWN_DISABLE;
    }
     else if (Mode == GPIO_MODE_INPUT_PULLUP) {
        xPinConfig.mode = GPIO_MODE_INPUT;
        xPinConfig.pull_up_en = GPIO_PULLUP_ENABLE;
        xPinConfig.pull_down_en = GPIO_PULLDOWN_DISABLE;
    }
     else if (Mode == GPIO_MODE_INPUT_PULLDOWN) {
        xPinConfig.mode = GPIO_MODE_INPUT;
        xPinConfig.pull_up_en = GPIO_PULLUP_DISABLE;
        xPinConfig.pull_down_en = GPIO_PULLDOWN_ENABLE;
    }
     else {
        // if there is an invalid mode, return without configuring the pin
        return;
    }

    gpio_config(&xPinConfig);
}

void GPIO_vSetPinValue(GPIO_Pin_t PinNum, uint8_t u8PinValue) {
    if (u8PinValue == GPIO_HIGH) {
        if (PinNum < 32) {
            //Pins 0-31 use lower register bank (out_w1ts)
            GPIO.out_w1ts = (1ULL << PinNum);
        } else {
            GPIO.out1_w1ts.val = (1ULL << (PinNum - 32));
        }
    }
        // if the value is low, clear the pin
        else {
            if (PinNum < 32) {
             GPIO.out_w1tc = (1ULL << PinNum);
        } else {
            GPIO.out1_w1tc.val = (1ULL << (PinNum - 32));
        }
    }
}

void GPIO_vTogglePin(GPIO_Pin_t PinNum) {

    uint8_t u8CurrentVal = GPIO_u8GetPinValue(PinNum); // read current value
    GPIO_vSetPinValue(PinNum, !u8CurrentVal);
}

uint8_t GPIO_u8GetPinValue(GPIO_Pin_t PinNum) {
    uint32_t u32RegVal = 0;

    if (PinNum < 32) {
        u32RegVal = GPIO.in;
        return (uint8_t)((u32RegVal >> PinNum) & 0x01);
    } else {
        u32RegVal = GPIO.in1.val;
        return (uint8_t)((u32RegVal >> (PinNum - 32)) & 0x01);
    }
}