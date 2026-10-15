#ifndef GPIO_INT_H_
#define GPIO_INT_H_

#include <stdint.h>
#include "driver/gpio.h"

/* Pin Logic Levels */
#define GPIO_LOW     0
#define GPIO_HIGH    1

/* Pin Modes */
#ifndef GPIO_MODE_INPUT_PULLUP
#define GPIO_MODE_INPUT_PULLUP   0x10u
#endif

#ifndef GPIO_MODE_INPUT_PULLDOWN
#define GPIO_MODE_INPUT_PULLDOWN 0x20u
#endif
typedef gpio_mode_t GPIO_Mode_t;

/* Pin Numbers */
typedef gpio_num_t GPIO_Pin_t;

/* Function Prototypes */

/**
 * @brief Initializes a specific GPIO pin mode and pull settings.
 * @param Copy_xPin GPIO pin number.
 * @param Copy_xMode Input/Output mode and internal resistor configuration.
 */
void GPIO_vInitPin(GPIO_Pin_t PinNum, GPIO_Mode_t Mode);

/**
 * @brief Sets the output level of a GPIO pin (High/Low).
 * @param Copy_xPin GPIO pin number.
 * @param Copy_u8Value GPIO_HIGH or GPIO_LOW.
 */
void GPIO_vSetPinValue(GPIO_Pin_t PinNum, uint8_t u8PinValue);

/**
 * @brief Toggles the current output state of a GPIO pin.
 * @param Copy_xPin GPIO pin number.
 */
void GPIO_vTogglePin(GPIO_Pin_t PinNum);

/**
 * @brief Reads the digital logic state of a GPIO pin.
 * @param Copy_xPin GPIO pin number.
 * @return uint8_t Returns 1 for GPIO_HIGH, 0 for GPIO_LOW.
 */
uint8_t GPIO_u8GetPinValue(GPIO_Pin_t PinNum);

#endif /* GPIO_INT_H_ */