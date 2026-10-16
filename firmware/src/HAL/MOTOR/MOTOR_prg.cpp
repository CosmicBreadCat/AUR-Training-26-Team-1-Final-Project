
#include "MOTOR_int.h"
#include "MOTOR_cfg.h"

#include "../../MCAL/GPIO/GPIO_int.h"
#include "../../MCAL/PWM/PWM_int.h"
#include "../../MCAL/PWM/PWM_cfg.h"

static const PWM_SpeedMode_t MOTOR_PWM_MODE = PWM_MODE_HIGH_SPEED;

static void vSetMotor(int16_t Copy_s16Duty,
                      GPIO_Pin_t Copy_xIn1,
                      GPIO_Pin_t Copy_xIn2,
                      PWM_ChannelNum_t Copy_xChannel)
{
    int32_t s32Duty = Copy_s16Duty;

    if (s32Duty > MOTOR_PWM_MAX_DUTY)
        s32Duty = MOTOR_PWM_MAX_DUTY;

    if (s32Duty < -MOTOR_PWM_MAX_DUTY)
        s32Duty = -MOTOR_PWM_MAX_DUTY;

    if (s32Duty > 0) {
        GPIO_vSetPinValue(Copy_xIn1, GPIO_HIGH);
        GPIO_vSetPinValue(Copy_xIn2, GPIO_LOW);
    } else if (s32Duty < 0) {
        GPIO_vSetPinValue(Copy_xIn1, GPIO_LOW);
        GPIO_vSetPinValue(Copy_xIn2, GPIO_HIGH);
    } else {
        GPIO_vSetPinValue(Copy_xIn1, GPIO_LOW);
        GPIO_vSetPinValue(Copy_xIn2, GPIO_LOW);
    }

    uint32_t u32Magnitude =
        (s32Duty < 0) ? (uint32_t)(-s32Duty)
                      : (uint32_t)s32Duty;

    PWM_vSetDuty(MOTOR_PWM_MODE, Copy_xChannel, u32Magnitude);
}

void MOTOR_vInit(void)
{
    GPIO_vInitPin((GPIO_Pin_t)MOTOR_LEFT_IN1_PIN, GPIO_MODE_OUTPUT);
    GPIO_vInitPin((GPIO_Pin_t)MOTOR_LEFT_IN2_PIN, GPIO_MODE_OUTPUT);
    GPIO_vInitPin((GPIO_Pin_t)MOTOR_RIGHT_IN1_PIN, GPIO_MODE_OUTPUT);
    GPIO_vInitPin((GPIO_Pin_t)MOTOR_RIGHT_IN2_PIN, GPIO_MODE_OUTPUT);

    PWM_vInitTimer(
        MOTOR_PWM_MODE,
        PWM_TIMER0,
        MOTOR_PWM_FREQ_HZ,
        LEDC_MOTOR_PWM_RES
    );

    PWM_vInitChannel(
        MOTOR_PWM_MODE, PWM_CH0, PWM_TIMER0,
        MOTOR_LEFT_PWM_PIN
    );

    PWM_vInitChannel(
        MOTOR_PWM_MODE, PWM_CH1, PWM_TIMER0,
        MOTOR_RIGHT_PWM_PIN
    );

    MOTOR_vStop();
}

void MOTOR_vSetLeft(int16_t Copy_s16Duty)
{
    vSetMotor(
        Copy_s16Duty,
        (GPIO_Pin_t)MOTOR_LEFT_IN1_PIN,
        (GPIO_Pin_t)MOTOR_LEFT_IN2_PIN,
        PWM_CH0
    );
}

void MOTOR_vSetRight(int16_t Copy_s16Duty)
{
    vSetMotor(
        Copy_s16Duty,
        (GPIO_Pin_t)MOTOR_RIGHT_IN1_PIN,
        (GPIO_Pin_t)MOTOR_RIGHT_IN2_PIN,
        PWM_CH1
    );
}

void MOTOR_vStop(void)
{
    MOTOR_vSetLeft(0);
    MOTOR_vSetRight(0);
}
