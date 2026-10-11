#ifndef PWM_INT_H_
#define PWM_INT_H_

#include <stdint.h>
#include "driver/ledc.h"

/* Speed Modes */
typedef enum {
    PWM_MODE_HIGH_SPEED = HIGHSPEED,
    PWM_MODE_LOW_SPEED  = LOWSPEED
} PWM_SpeedMode_t;

/* Timers */
typedef enum {
    PWM_TIMER_INDEX_0 = TIMER_0,
    PWM_TIMER_INDEX_1 = TIMER_1,
    PWM_TIMER_INDEX_2 = TIMER_2,
    PWM_TIMER_INDEX_3 = TIMER_3
} PWM_TimerNum_t;

/* Channels */
typedef enum {
    PWM_CHANNEL_INDEX_0 = CH_0,
    PWM_CHANNEL_INDEX_1 = CH_1,
    PWM_CHANNEL_INDEX_2 = CH_2,
    PWM_CHANNEL_INDEX_3 = CH_3,
    PWM_CHANNEL_INDEX_4 = CH_4,
    PWM_CHANNEL_INDEX_5 = CH_5,
    PWM_CHANNEL_INDEX_6 = CH_6,
    PWM_CHANNEL_INDEX_7 = CH_7
} PWM_ChannelNum_t;

/* Function Prototypes */

/**
 * @brief Configures a hardware timer for PWM generation.
 * @param Copy_xMode High Speed or Low Speed mode.
 * @param Copy_xTimer Hardware timer index (0-3).
 * @param Copy_u32FreqHz Desired PWM frequency in Hz.
 * @param Copy_xResolution Bit resolution for duty cycle precision.
 */
void PWM_vInitTimer(PWM_SpeedMode_t Copy_xMode, 
                     PWM_TimerNum_t Copy_xTimer, 
                     uint32_t Copy_u32FreqHz, 
                     ledc_timer_bit_t Copy_xResolution);

/**
 * @brief Maps a PWM channel to a hardware GPIO pin.
 * @param Copy_xMode High Speed or Low Speed mode.
 * @param Copy_xChannel Hardware PWM channel (0-7).
 * @param Copy_xTimer Hardware timer assigned to this channel.
 * @param Copy_u8GpioNum GPIO pin number outputting the PWM.
 */
void PWM_vInitChannel(PWM_SpeedMode_t Copy_xMode, 
                      PWM_ChannelNum_t Copy_xChannel, 
                      PWM_TimerNum_t Copy_xTimer, 
                      uint8_t Copy_u8GpioNum);

/**
 * @brief Sets raw duty cycle for motor speed control or general PWM.
 * @param Copy_xMode High Speed or Low Speed mode.
 * @param Copy_xChannel Target PWM channel.
 * @param Copy_u32Duty Duty cycle integer value.
 */
void PWM_vSetDuty(PWM_SpeedMode_t Copy_xMode, 
                   PWM_ChannelNum_t Copy_xChannel, 
                   uint32_t Copy_u32Duty);

/**
 * @brief Helper function to directly set a servo angle (0 to 180 degrees).
 * @param Copy_xMode High Speed or Low Speed mode.
 * @param Copy_xChannel Target PWM channel connected to the servo.
 * @param Copy_u8Angle Desired angle in degrees (0 - 180).
 */
void PWM_vSetServoAngle(PWM_SpeedMode_t Copy_xMode, 
                         PWM_ChannelNum_t Copy_xChannel, 
                         uint8_t Copy_u8Angle);

#endif /* PWM_INT_H_ */