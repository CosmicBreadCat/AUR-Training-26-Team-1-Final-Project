#include "PWM_int.h"
#include "PWM_cfg.h"

void PWM_vInitTimer(PWM_SpeedMode_t Copy_xMode, 
                     PWM_TimerNum_t Copy_xTimer, 
                     uint32_t Copy_u32FreqHz, 
                     ledc_timer_bit_t Copy_xResolution) {
    ledc_timer_config_t xTimerConfig = {
        .speed_mode      = (ledc_mode_t)Copy_xMode,
        .timer_num       = (ledc_timer_t)Copy_xTimer,
        .duty_resolution = Copy_xResolution,
        .freq_hz         = Copy_u32FreqHz,
        .clk_cfg         = LEDC_AUTO_CLK
    };
    ledc_timer_config(&xTimerConfig);
}

void PWM_vInitChannel(PWM_SpeedMode_t Copy_xMode, 
                      PWM_ChannelNum_t Copy_xChannel, 
                      PWM_TimerNum_t Copy_xTimer, 
                      uint8_t Copy_u8GpioNum) {
    ledc_channel_config_t xChannelConfig = {
        .speed_mode = (ledc_mode_t)Copy_xMode,
        .channel    = (ledc_channel_t)Copy_xChannel,
        .timer_sel  = (ledc_timer_t)Copy_xTimer,
        .intr_type  = LEDC_INTR_DISABLE,
        .gpio_num   = Copy_u8GpioNum,
        .duty       = 0,
        .hpoint     = 0
    };
    ledc_channel_config(&xChannelConfig);
}

void PWM_vSetDuty(PWM_SpeedMode_t Copy_xMode, 
                   PWM_ChannelNum_t Copy_xChannel, 
                   uint32_t Copy_u32Duty) {
    ledc_set_duty((ledc_mode_t)Copy_xMode, (ledc_channel_t)Copy_xChannel, Copy_u32Duty);
    ledc_update_duty((ledc_mode_t)Copy_xMode, (ledc_channel_t)Copy_xChannel);
}

void PWM_vSetServoAngle(PWM_SpeedMode_t Copy_xMode, 
                         PWM_ChannelNum_t Copy_xChannel, 
                         uint8_t Copy_u8Angle) {
    if (Copy_u8Angle > 180) {
        Copy_u8Angle = 180;
    }

    /* Map angle (0 - 180 deg) to pulse width in microseconds (500 - 2500 us) */
    uint32_t u32PulseUs = LEDC_SERVO_MIN_PULSE_US + 
                          ((uint32_t)Copy_u8Angle * (LEDC_SERVO_MAX_PULSE_US - LEDC_SERVO_MIN_PULSE_US)) / 180;

    /* 
     * Convert pulse width (us) to duty ticks for a 50Hz period (20,000 us)
     * Max resolution ticks = (1 << LEDC_SERVO_PWM_RES) - 1
     * Duty = (u32PulseUs / 20000us) * Max Resolution Ticks
     */
    uint32_t u32MaxDuty = (1 << LEDC_SERVO_PWM_RES) - 1;
    uint32_t u32DutyTicks = (u32PulseUs * u32MaxDuty) / 20000;

    LEDC_vSetDuty(Copy_xMode, Copy_xChannel, u32DutyTicks);
}