#include "GRIPPER_int.h"
#include "GRIPPER_cfg.h"
#include "../../MCAL/PWM/PWM_int.h"
#include "../../MCAL/PWM/PWM_cfg.h"
#include <math.h>

enum { SERVO_GRIP = 0, SERVO_LIFT = 1, SERVO_COUNT = 2 };

static const PWM_ChannelNum_t axChannel[SERVO_COUNT] = {GRIPPER_GRIP_PWM_CH, GRIPPER_LIFT_PWM_CH};
static const float afMaxSpeedDps[SERVO_COUNT] = {GRIPPER_GRIP_SPEED_DPS, GRIPPER_LIFT_SPEED_DPS};

static float afCurrentDeg[SERVO_COUNT];
static float afTargetDeg[SERVO_COUNT];
static int16_t as16Written[SERVO_COUNT] = {-1, -1};

/* normalized 0..1 -> angle between two limits */
static float fMapToAngle(float Copy_fNorm, float Copy_fAtZero, float Copy_fAtOne) {
    if (Copy_fNorm != Copy_fNorm) Copy_fNorm = 0.0f;   /* NaN */
    if (Copy_fNorm < 0.0f) Copy_fNorm = 0.0f;
    if (Copy_fNorm > 1.0f) Copy_fNorm = 1.0f;
    return Copy_fAtZero + Copy_fNorm * (Copy_fAtOne - Copy_fAtZero);
}

/* Writes to the servo only when the whole-degree angle changes */
static void vWriteServo(int Copy_iServo) {
    int iAngle = (int)(afCurrentDeg[Copy_iServo] + 0.5f);
    if (iAngle < 0)   iAngle = 0;
    if (iAngle > 180) iAngle = 180;
    if (iAngle != as16Written[Copy_iServo]) {
        PWM_vSetServoAngle(GRIPPER_PWM_SPEED_MODE, axChannel[Copy_iServo], (uint8_t)iAngle);
        as16Written[Copy_iServo] = (int16_t)iAngle;
    }
}

void GRIPPER_vInit(void) {
    PWM_vInitTimer(GRIPPER_PWM_SPEED_MODE, GRIPPER_PWM_TIMER, LEDC_SERVO_PWM_FREQ_HZ,
                   LEDC_SERVO_PWM_RES);
    PWM_vInitChannel(GRIPPER_PWM_SPEED_MODE, axChannel[SERVO_GRIP], GRIPPER_PWM_TIMER,
                     GRIPPER_GRIP_PIN);
    PWM_vInitChannel(GRIPPER_PWM_SPEED_MODE, axChannel[SERVO_LIFT], GRIPPER_PWM_TIMER,
                     GRIPPER_LIFT_PIN);

    GRIPPER_vSetTarget(GRIPPER_START_GRIP, GRIPPER_START_LIFT);
    for (int i = 0; i < SERVO_COUNT; i++) {
        afCurrentDeg[i] = afTargetDeg[i];   /* start exactly at the start pose */
        vWriteServo(i);
    }
}

void GRIPPER_vSetTarget(float Copy_fGrip, float Copy_fLift) {
    afTargetDeg[SERVO_GRIP] = fMapToAngle(Copy_fGrip, GRIPPER_GRIP_OPEN_DEG, GRIPPER_GRIP_CLOSED_DEG);
    afTargetDeg[SERVO_LIFT] = fMapToAngle(Copy_fLift, GRIPPER_LIFT_DOWN_DEG, GRIPPER_LIFT_UP_DEG);
}

void GRIPPER_vStep(float Copy_fDtSec) {
    for (int i = 0; i < SERVO_COUNT; i++) {
        float fStep = afMaxSpeedDps[i] * Copy_fDtSec;
        float fDiff = afTargetDeg[i] - afCurrentDeg[i];
        if (fabsf(fDiff) <= fStep) {
            afCurrentDeg[i] = afTargetDeg[i];
        } else {
            afCurrentDeg[i] += (fDiff > 0.0f) ? fStep : -fStep;
        }
        vWriteServo(i);
    }
}
