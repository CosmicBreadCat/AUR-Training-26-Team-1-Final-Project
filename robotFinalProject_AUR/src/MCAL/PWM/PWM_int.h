#ifndef LEDC_INT_H_
#define LEDC_INT_H_

#include <stdint.h>
#include "driver/ledc.h"

/* Speed Modes */
typedef enum {
    LEDC_MODE_HIGH_SPEED = LEDC_HIGH_SPEED_MODE,
    LEDC_MODE_LOW_SPEED  = LEDC_LOW_SPEED_MODE
} LEDC_SpeedMode_t;

/* Timers */
typedef enum {
    LEDC_TIMER_INDEX_0 = LEDC_TIMER_0,
    LEDC_TIMER_INDEX_1 = LEDC_TIMER_1,
    LEDC_TIMER_INDEX_2 = LEDC_TIMER_2,
    LEDC_TIMER_INDEX_3 = LEDC_TIMER_3
} LEDC_TimerNum_t;

/* Channels */
typedef enum {
    LEDC_CHANNEL_INDEX_0 = LEDC_CHANNEL_0,
    LEDC_CHANNEL_INDEX_1 = LEDC_CHANNEL_1,
    LEDC_CHANNEL_INDEX_2 = LEDC_CHANNEL_2,
    LEDC_CHANNEL_INDEX_3 = LEDC_CHANNEL_3,
    LEDC_CHANNEL_INDEX_4 = LEDC_CHANNEL_4,
    LEDC_CHANNEL_INDEX_5 = LEDC_CHANNEL_5,
    LEDC_CHANNEL_INDEX_6 = LEDC_CHANNEL_6,
    LEDC_CHANNEL_INDEX_7 = LEDC_CHANNEL_7
} LEDC_ChannelNum_t;

/* Function Prototypes */

/**
 * @brief Configures a hardware timer for PWM generation.
 * @param Copy_xMode High Speed or Low Speed mode.
 * @param Copy_xTimer Hardware timer index (0-3).
 * @param Copy_u32FreqHz Desired PWM frequency in Hz.
 * @param Copy_xResolution Bit resolution for duty cycle precision.
 */
void LEDC_vInitTimer(LEDC_SpeedMode_t Copy_xMode, 
                     LEDC_TimerNum_t Copy_xTimer, 
                     uint32_t Copy_u32FreqHz, 
                     ledc_timer_bit_t Copy_xResolution);

/**
 * @brief Maps a PWM channel to a hardware GPIO pin.
 * @param Copy_xMode High Speed or Low Speed mode.
 * @param Copy_xChannel Hardware PWM channel (0-7).
 * @param Copy_xTimer Hardware timer assigned to this channel.
 * @param Copy_u8GpioNum GPIO pin number outputting the PWM.
 */
void LEDC_vInitChannel(LEDC_SpeedMode_t Copy_xMode, 
                       LEDC_ChannelNum_t Copy_xChannel, 
                       LEDC_TimerNum_t Copy_xTimer, 
                       uint8_t Copy_u8GpioNum);

/**
 * @brief Sets raw duty cycle for motor speed control or general PWM.
 * @param Copy_xMode High Speed or Low Speed mode.
 * @param Copy_xChannel Target PWM channel.
 * @param Copy_u32Duty Duty cycle integer value.
 */
void LEDC_vSetDuty(LEDC_SpeedMode_t Copy_xMode, 
                   LEDC_ChannelNum_t Copy_xChannel, 
                   uint32_t Copy_u32Duty);

/**
 * @brief Helper function to directly set a servo angle (0 to 180 degrees).
 * @param Copy_xMode High Speed or Low Speed mode.
 * @param Copy_xChannel Target PWM channel connected to the servo.
 * @param Copy_u8Angle Desired angle in degrees (0 - 180).
 */
void LEDC_vSetServoAngle(LEDC_SpeedMode_t Copy_xMode, 
                         LEDC_ChannelNum_t Copy_xChannel, 
                         uint8_t Copy_u8Angle);

#endif /* LEDC_INT_H_ */