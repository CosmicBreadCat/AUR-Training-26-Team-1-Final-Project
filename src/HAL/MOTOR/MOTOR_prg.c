#include "MOTOR_int.h"
#include "MOTOR_cfg.h"
#include "../../MCAL/GPIO/GPIO_int.h"
#include "../../MCAL/PWM/PWM_int.h"
#include "../../MCAL/PWM/PWM_cfg.h"
#include <math.h>
#include <stdbool.h>

typedef struct {
    uint8_t u8In1Pin;
    uint8_t u8In2Pin;
    uint8_t u8PwmPin;
    PWM_ChannelNum_t xChannel;
    bool bInverted;
} MotorHw_t;

static const MotorHw_t asMotors[MOTOR_COUNT] = {
    [MOTOR_LEFT]  = {MOTOR_LEFT_IN1_PIN,  MOTOR_LEFT_IN2_PIN,  MOTOR_LEFT_PWM_PIN,
                     MOTOR_LEFT_PWM_CH,  MOTOR_LEFT_INVERTED},
    [MOTOR_RIGHT] = {MOTOR_RIGHT_IN1_PIN, MOTOR_RIGHT_IN2_PIN, MOTOR_RIGHT_PWM_PIN,
                     MOTOR_RIGHT_PWM_CH, MOTOR_RIGHT_INVERTED},
};

/* Highest duty value the PWM timer accepts (10 bit -> 1023) */
#define MOTOR_MAX_TICKS ((1u << LEDC_MOTOR_PWM_RES) - 1u)

static void vDrive(const MotorHw_t *Copy_psMotor, uint8_t Copy_u8In1, uint8_t Copy_u8In2,
                   uint32_t Copy_u32Ticks) {
    GPIO_vSetPinValue((GPIO_Pin_t)Copy_psMotor->u8In1Pin, Copy_u8In1);
    GPIO_vSetPinValue((GPIO_Pin_t)Copy_psMotor->u8In2Pin, Copy_u8In2);
    PWM_vSetDuty(MOTOR_PWM_SPEED_MODE, Copy_psMotor->xChannel, Copy_u32Ticks);
}

void MOTOR_vInit(void) {
    PWM_vInitTimer(MOTOR_PWM_SPEED_MODE, MOTOR_PWM_TIMER, LEDC_MOTOR_PWM_FREQ_HZ,
                   LEDC_MOTOR_PWM_RES);
    for (int i = 0; i < MOTOR_COUNT; i++) {
        GPIO_vInitPin((GPIO_Pin_t)asMotors[i].u8In1Pin, GPIO_MODE_OUTPUT);
        GPIO_vInitPin((GPIO_Pin_t)asMotors[i].u8In2Pin, GPIO_MODE_OUTPUT);
        PWM_vInitChannel(MOTOR_PWM_SPEED_MODE, asMotors[i].xChannel, MOTOR_PWM_TIMER,
                         asMotors[i].u8PwmPin);
    }
    MOTOR_vStopAll();
}

void MOTOR_vSetDuty(Motor_Side_t Copy_xSide, float Copy_fDuty) {
    if ((int)Copy_xSide < 0 || Copy_xSide >= MOTOR_COUNT) {
        return;
    }
    const MotorHw_t *psMotor = &asMotors[Copy_xSide];

    if (Copy_fDuty != Copy_fDuty) { /* NaN */
        Copy_fDuty = 0.0f;
    }
    if (Copy_fDuty > 1.0f)  Copy_fDuty = 1.0f;
    if (Copy_fDuty < -1.0f) Copy_fDuty = -1.0f;
    if (psMotor->bInverted) Copy_fDuty = -Copy_fDuty;

    uint32_t u32Ticks = (uint32_t)(fabsf(Copy_fDuty) * (float)MOTOR_MAX_TICKS + 0.5f);
    if (u32Ticks == 0) {
        vDrive(psMotor, 0, 0, 0);          /* coast */
    } else if (Copy_fDuty > 0.0f) {
        vDrive(psMotor, 1, 0, u32Ticks);   /* forward */
    } else {
        vDrive(psMotor, 0, 1, u32Ticks);   /* reverse */
    }
}

void MOTOR_vStop(Motor_Side_t Copy_xSide) {
    MOTOR_vSetDuty(Copy_xSide, 0.0f);
}

void MOTOR_vBrake(Motor_Side_t Copy_xSide) {
    if ((int)Copy_xSide < 0 || Copy_xSide >= MOTOR_COUNT) {
        return;
    }
    vDrive(&asMotors[Copy_xSide], 1, 1, MOTOR_MAX_TICKS);
}

void MOTOR_vStopAll(void) {
    for (int i = 0; i < MOTOR_COUNT; i++) {
        MOTOR_vStop((Motor_Side_t)i);
    }
}
