#ifndef MOTOR_CONTROL_H
#define MOTOR_CONTROL_H

#include "config.h"

/**
 * Initialize motor pins and PWM channels
 */
void motorInit();

/**
 * Drive both motors forward at specified speed
 */
void motorForward(int speed);

/**
 * Drive both motors backward at specified speed
 */
void motorReverse(int speed);

/**
 * Turn left (right motor forward, left motor stopped/slow)
 */
void motorTurnLeft(int speed);

/**
 * Turn right (left motor forward, right motor stopped/slow)
 */
void motorTurnRight(int speed);

/**
 * Stop both motors
 */
void motorStop();

/**
 * Set individual motor speeds with direction
 */
void motorSetSpeeds(int leftSpeed, int rightSpeed);

/**
 * Apply steering differential to base PID output
 */
void motorApplyWithSteering(int baseOutput, RobotCommand command);

#endif // MOTOR_CONTROL_H
