#ifndef PID_CONTROLLER_H
#define PID_CONTROLLER_H

#include "config.h"

/**
 * Initialize PID controller with default values from config
 */
void pidInit();

/**
 * Compute PID output based on current input
 */
bool pidCompute(double currentAngle, double *output);

/**
 * Set new PID tuning parameters
 */
void pidSetTunings(double kp, double ki, double kd);

/**
 * Set new setpoint
 */
void pidSetSetpoint(double setpoint);

/**
 * Get current setpoint
 */
double pidGetSetpoint();

/**
 * Adjust setpoint for steering (forward/backward tilt)
 */
void pidAdjustSetpoint(double adjustment);

/**
 * Reset setpoint to default
 */
void pidResetSetpoint();

/**
 * Get current PID values for telemetry
 */
void pidGetValues(double *kp, double *ki, double *kd, double *setpoint);

#endif // PID_CONTROLLER_H
