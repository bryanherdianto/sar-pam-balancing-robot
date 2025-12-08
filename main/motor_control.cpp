#include "motor_control.h"

// Mutex for thread-safe motor access
static SemaphoreHandle_t xMotorMutex = NULL;

void motorInit()
{
    // Create mutex for motor access
    xMotorMutex = xSemaphoreCreateMutex();

    // Setup direction pins
    pinMode(PIN_IN1, OUTPUT);
    pinMode(PIN_IN2, OUTPUT);
    pinMode(PIN_IN3, OUTPUT);
    pinMode(PIN_IN4, OUTPUT);

    // Setup PWM channels
    ledcSetup(PWM_CHANNEL_A, PWM_FREQ, PWM_RESOLUTION);
    ledcSetup(PWM_CHANNEL_B, PWM_FREQ, PWM_RESOLUTION);
    ledcAttachPin(PIN_ENA, PWM_CHANNEL_A);
    ledcAttachPin(PIN_ENB, PWM_CHANNEL_B);

    // Start with motors stopped
    motorStop();

    DEBUG_PRINTLN(F("Motor control initialized"));
}

void motorForward(int speed)
{
    if (xMotorMutex != NULL)
    {
        xSemaphoreTake(xMotorMutex, portMAX_DELAY);
    }

    // Apply minimum speed threshold
    if (speed > 0 && speed < MIN_MOTOR_SPEED)
    {
        speed = MIN_MOTOR_SPEED;
    }
    speed = constrain(speed, 0, 255);

    // Set direction: both motors forward
    digitalWrite(PIN_IN1, HIGH);
    digitalWrite(PIN_IN2, LOW);
    digitalWrite(PIN_IN3, HIGH);
    digitalWrite(PIN_IN4, LOW);

    // Set speed
    ledcWrite(PWM_CHANNEL_A, speed);
    ledcWrite(PWM_CHANNEL_B, speed);

    delayMicroseconds(50); // Brief delay for I2C noise reduction

    if (xMotorMutex != NULL)
    {
        xSemaphoreGive(xMotorMutex);
    }
}

void motorReverse(int speed)
{
    if (xMotorMutex != NULL)
    {
        xSemaphoreTake(xMotorMutex, portMAX_DELAY);
    }

    // Apply minimum speed threshold
    if (speed > 0 && speed < MIN_MOTOR_SPEED)
    {
        speed = MIN_MOTOR_SPEED;
    }
    speed = constrain(speed, 0, 255);

    // Set direction: both motors backward
    digitalWrite(PIN_IN1, LOW);
    digitalWrite(PIN_IN2, HIGH);
    digitalWrite(PIN_IN3, LOW);
    digitalWrite(PIN_IN4, HIGH);

    // Set speed
    ledcWrite(PWM_CHANNEL_A, speed);
    ledcWrite(PWM_CHANNEL_B, speed);

    delayMicroseconds(50);

    if (xMotorMutex != NULL)
    {
        xSemaphoreGive(xMotorMutex);
    }
}

void motorTurnLeft(int speed)
{
    if (xMotorMutex != NULL)
    {
        xSemaphoreTake(xMotorMutex, portMAX_DELAY);
    }

    speed = constrain(speed, 0, 255);

    // Right motor forward, left motor stopped
    digitalWrite(PIN_IN1, LOW); // Left motor stop
    digitalWrite(PIN_IN2, LOW);
    digitalWrite(PIN_IN3, HIGH); // Right motor forward
    digitalWrite(PIN_IN4, LOW);

    ledcWrite(PWM_CHANNEL_A, 0);     // Left motor off
    ledcWrite(PWM_CHANNEL_B, speed); // Right motor on

    delayMicroseconds(50);

    if (xMotorMutex != NULL)
    {
        xSemaphoreGive(xMotorMutex);
    }
}

void motorTurnRight(int speed)
{
    if (xMotorMutex != NULL)
    {
        xSemaphoreTake(xMotorMutex, portMAX_DELAY);
    }

    speed = constrain(speed, 0, 255);

    // Left motor forward, right motor stopped
    digitalWrite(PIN_IN1, HIGH); // Left motor forward
    digitalWrite(PIN_IN2, LOW);
    digitalWrite(PIN_IN3, LOW); // Right motor stop
    digitalWrite(PIN_IN4, LOW);

    ledcWrite(PWM_CHANNEL_A, speed); // Left motor on
    ledcWrite(PWM_CHANNEL_B, 0);     // Right motor off

    delayMicroseconds(50);

    if (xMotorMutex != NULL)
    {
        xSemaphoreGive(xMotorMutex);
    }
}

void motorStop()
{
    if (xMotorMutex != NULL)
    {
        xSemaphoreTake(xMotorMutex, portMAX_DELAY);
    }

    // Stop both motors
    digitalWrite(PIN_IN1, LOW);
    digitalWrite(PIN_IN2, LOW);
    digitalWrite(PIN_IN3, LOW);
    digitalWrite(PIN_IN4, LOW);

    ledcWrite(PWM_CHANNEL_A, 0);
    ledcWrite(PWM_CHANNEL_B, 0);

    if (xMotorMutex != NULL)
    {
        xSemaphoreGive(xMotorMutex);
    }
}

void motorSetSpeeds(int leftSpeed, int rightSpeed)
{
    if (xMotorMutex != NULL)
    {
        xSemaphoreTake(xMotorMutex, portMAX_DELAY);
    }

    // Left motor direction
    if (leftSpeed >= 0)
    {
        digitalWrite(PIN_IN1, HIGH);
        digitalWrite(PIN_IN2, LOW);
    }
    else
    {
        digitalWrite(PIN_IN1, LOW);
        digitalWrite(PIN_IN2, HIGH);
        leftSpeed = -leftSpeed;
    }

    // Right motor direction
    if (rightSpeed >= 0)
    {
        digitalWrite(PIN_IN3, HIGH);
        digitalWrite(PIN_IN4, LOW);
    }
    else
    {
        digitalWrite(PIN_IN3, LOW);
        digitalWrite(PIN_IN4, HIGH);
        rightSpeed = -rightSpeed;
    }

    // Apply minimum speed threshold
    if (leftSpeed > 0 && leftSpeed < MIN_MOTOR_SPEED)
        leftSpeed = MIN_MOTOR_SPEED;
    if (rightSpeed > 0 && rightSpeed < MIN_MOTOR_SPEED)
        rightSpeed = MIN_MOTOR_SPEED;

    leftSpeed = constrain(leftSpeed, 0, 255);
    rightSpeed = constrain(rightSpeed, 0, 255);

    ledcWrite(PWM_CHANNEL_A, leftSpeed);
    ledcWrite(PWM_CHANNEL_B, rightSpeed);

    delayMicroseconds(50);

    if (xMotorMutex != NULL)
    {
        xSemaphoreGive(xMotorMutex);
    }
}

void motorApplyWithSteering(int baseOutput, RobotCommand command)
{
    int leftSpeed = baseOutput;
    int rightSpeed = baseOutput;

    // Apply steering differential
    switch (command)
    {
    case CMD_LEFT:
        // Reduce left motor speed for left turn
        leftSpeed = baseOutput / 2;
        break;
    case CMD_RIGHT:
        // Reduce right motor speed for right turn
        rightSpeed = baseOutput / 2;
        break;
    case CMD_FORWARD:
        // Slight forward bias - increase setpoint effect handled in PID
        break;
    case CMD_BACKWARD:
        // Slight backward bias
        break;
    default:
        // No steering adjustment
        break;
    }

    motorSetSpeeds(leftSpeed, rightSpeed);
}
