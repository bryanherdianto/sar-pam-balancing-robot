/**
 * @file pid_controller.cpp
 * @brief PID controller implementation
 */

#include "pid_controller.h"
#include <PID_v1.h>

// PID variables
static double pidInput = 0;
static double pidOutput = 0;
static double pidSetpointVal = PID_SETPOINT;
static double baseSetpoint = PID_SETPOINT;

// PID tuning parameters
static double Kp = PID_KP;
static double Ki = PID_KI;
static double Kd = PID_KD;

// PID instance
static PID balancePID(&pidInput, &pidOutput, &pidSetpointVal, Kp, Ki, Kd, DIRECT);

// Mutex for thread-safe access
static SemaphoreHandle_t xPIDMutex = NULL;

void pidInit()
{
    xPIDMutex = xSemaphoreCreateMutex();

    balancePID.SetMode(AUTOMATIC);
    balancePID.SetSampleTime(PID_SAMPLE_TIME);
    balancePID.SetOutputLimits(PID_OUTPUT_MIN, PID_OUTPUT_MAX);

    DEBUG_PRINTLN(F("PID controller initialized"));
    DEBUG_PRINTF("Setpoint: %.1f, Kp: %.1f, Ki: %.1f, Kd: %.2f\n",
                 pidSetpointVal, Kp, Ki, Kd);
}

bool pidCompute(double currentAngle, double *output)
{
    if (xPIDMutex != NULL)
    {
        if (xSemaphoreTake(xPIDMutex, pdMS_TO_TICKS(5)) != pdTRUE)
        {
            return false;
        }
    }

    pidInput = currentAngle;
    bool computed = balancePID.Compute();

    // Validate output
    if (isnan(pidOutput))
    {
        DEBUG_PRINTLN(F("PID output NaN!"));
        pidOutput = 0;
        if (xPIDMutex != NULL)
            xSemaphoreGive(xPIDMutex);
        return false;
    }

    // Constrain output
    pidOutput = constrain(pidOutput, PID_OUTPUT_MIN, PID_OUTPUT_MAX);
    *output = pidOutput;

    if (xPIDMutex != NULL)
        xSemaphoreGive(xPIDMutex);

    return computed;
}

void pidSetTunings(double kp, double ki, double kd)
{
    if (xPIDMutex != NULL)
    {
        xSemaphoreTake(xPIDMutex, portMAX_DELAY);
    }

    Kp = kp;
    Ki = ki;
    Kd = kd;
    balancePID.SetTunings(Kp, Ki, Kd);

    DEBUG_PRINTF("PID tunings updated: Kp=%.1f, Ki=%.1f, Kd=%.2f\n", Kp, Ki, Kd);

    if (xPIDMutex != NULL)
        xSemaphoreGive(xPIDMutex);
}

void pidSetSetpoint(double setpoint)
{
    if (xPIDMutex != NULL)
    {
        xSemaphoreTake(xPIDMutex, portMAX_DELAY);
    }

    pidSetpointVal = setpoint;
    baseSetpoint = setpoint;

    if (xPIDMutex != NULL)
        xSemaphoreGive(xPIDMutex);
}

double pidGetSetpoint()
{
    return pidSetpointVal;
}

void pidAdjustSetpoint(double adjustment)
{
    if (xPIDMutex != NULL)
    {
        xSemaphoreTake(xPIDMutex, portMAX_DELAY);
    }

    pidSetpointVal = baseSetpoint + adjustment;

    if (xPIDMutex != NULL)
        xSemaphoreGive(xPIDMutex);
}

void pidResetSetpoint()
{
    if (xPIDMutex != NULL)
    {
        xSemaphoreTake(xPIDMutex, portMAX_DELAY);
    }

    pidSetpointVal = baseSetpoint;

    if (xPIDMutex != NULL)
        xSemaphoreGive(xPIDMutex);
}

void pidGetValues(double *kp, double *ki, double *kd, double *setpoint)
{
    if (xPIDMutex != NULL)
    {
        xSemaphoreTake(xPIDMutex, portMAX_DELAY);
    }

    *kp = Kp;
    *ki = Ki;
    *kd = Kd;
    *setpoint = pidSetpointVal;

    if (xPIDMutex != NULL)
        xSemaphoreGive(xPIDMutex);
}
