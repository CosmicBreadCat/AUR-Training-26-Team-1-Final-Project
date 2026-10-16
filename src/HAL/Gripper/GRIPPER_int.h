#ifndef GRIPPER_INT_H_
#define GRIPPER_INT_H_

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Sets up the servo PWM timer and channels and moves to the start pose. */
void GRIPPER_vInit(void);

/**
 * @brief Sets where the gripper should go. It does not jump: GRIPPER_vStep moves there smoothly.
 * @param Copy_fGrip 0.0 = fingers open, 1.0 = fingers closed (values outside are clamped).
 * @param Copy_fLift 0.0 = gripper down,  1.0 = gripper up     (values outside are clamped).
 */
void GRIPPER_vSetTarget(float Copy_fGrip, float Copy_fLift);

/** @brief Moves both servos a small step towards their targets. Call every control tick.
 *  @param Copy_fDtSec Time since the previous call, in seconds. */
void GRIPPER_vStep(float Copy_fDtSec);

#ifdef __cplusplus
}
#endif

#endif /* GRIPPER_INT_H_ */
