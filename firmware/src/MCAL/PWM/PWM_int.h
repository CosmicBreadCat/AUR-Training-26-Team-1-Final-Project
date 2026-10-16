#ifndef PWM_INT_H_
#define PWM_INT_H_

#include <stdint.h>
#include "driver/ledc.h"

/* Speed Modes */
typedef enum {
    PWM_MODE_HIGH_SPEED = LEDC_HIGH_SPEED_MODE,
    PWM_MODE_LOW_SPEED  = LEDC_LOW_SPEED_MODE
} PWM_SpeedMode_t;

/* Timers */
typedef enum {
    PWM_TIMER0 = LEDC_TIMER_0,
    PWM_TIMER1 = LEDC_TIMER_1,
    PWM_TIMER2 = LEDC_TIMER_2,
    PWM_TIMER3 = LEDC_TIMER_3
} PWM_TimerNum_t;

/* Channels */
typedef enum {
    PWM_CH0 = LEDC_CHANNEL_0,
    PWM_CH1 = LEDC_CHANNEL_1,
    PWM_CH2 = LEDC_CHANNEL_2,
    PWM_CH3 = LEDC_CHANNEL_3,
    PWM_CH4 = LEDC_CHANNEL_4,
    PWM_CH5 = LEDC_CHANNEL_5,
    PWM_CH6 = LEDC_CHANNEL_6,
    PWM_CH7 = LEDC_CHANNEL_7
} PWM_ChannelNum_t;

/* Function Prototypes */
#ifdef __cplusplus
extern "C" {
#endif
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


#ifdef __cplusplus
}
#endif                         
#endif /* PWM_INT_H_ */