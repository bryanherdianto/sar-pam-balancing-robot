#include "I2Cdev.h"
#include <PID_v1.h>
#include "MPU6050_6Axis_MotionApps20.h"
#include <Wire.h>

MPU6050 mpu;

// MPU control/status vars
bool dmpReady = false;
uint8_t mpuIntStatus;
uint8_t devStatus;
uint16_t packetSize;
uint16_t fifoCount;
uint8_t fifoBuffer[64];

// orientation/motion vars
Quaternion q;
VectorFloat gravity;
float ypr[3];

/*********Tune these 4 values for your BOT*********/
double setpoint = 190;
double Kp = 25.0;
double Kd = 1.2;
double Ki = 80.0; // Add integral term

// Motor deadband compensation
int minMotorSpeed = 50; // Minimum speed to overcome friction
/******End of values setting*********/

double input, output;
PID pid(&input, &output, &setpoint, Kp, Ki, Kd, DIRECT);

// ===== L298N Motor Driver Pins =====
// Motor A (Left)
#define ENA 5  // PWM speed control
#define IN1 16 // Direction
#define IN2 17 // Direction

// Motor B (Right)
#define ENB 18 // PWM speed control
#define IN3 32 // Direction
#define IN4 33 // Direction

// MPU6050 Interrupt Pin
#define MPU_INT 19

// I2C Pins
#define SDA_PIN 21
#define SCL_PIN 22

// ESP32 LEDC PWM Settings
#define PWM_FREQ 5000
#define PWM_RESOLUTION 8
#define PWM_CHANNEL_A 0 // For ENA
#define PWM_CHANNEL_B 1 // For ENB

volatile bool mpuInterrupt = false;

void IRAM_ATTR dmpDataReady()
{
    mpuInterrupt = true;
}

void setup()
{
    Serial.begin(115200);

    // Initialize I2C
    Wire.begin(SDA_PIN, SCL_PIN);
    Wire.setClock(400000);

    Serial.println(F("Initializing I2C devices..."));
    mpu.initialize();

    Serial.println(F("Testing device connections..."));
    Serial.println(mpu.testConnection() ? F("MPU6050 connection successful") : F("MPU6050 connection failed"));

    devStatus = mpu.dmpInitialize();

    // Supply your own gyro offsets here
    mpu.setXGyroOffset(-2);
    mpu.setYGyroOffset(74);
    mpu.setZGyroOffset(7);
    mpu.setZAccelOffset(968);

    if (devStatus == 0)
    {
        Serial.println(F("Enabling DMP..."));
        mpu.setDMPEnabled(true);

        pinMode(MPU_INT, INPUT);
        attachInterrupt(digitalPinToInterrupt(MPU_INT), dmpDataReady, RISING);

        mpuIntStatus = mpu.getIntStatus();
        Serial.println(F("DMP ready!"));
        dmpReady = true;
        packetSize = mpu.dmpGetFIFOPacketSize();

        pid.SetMode(AUTOMATIC);
        pid.SetSampleTime(10);
        pid.SetOutputLimits(-255, 255);
    }
    else
    {
        Serial.print(F("DMP Initialization failed (code "));
        Serial.print(devStatus);
        Serial.println(F(")"));
    }

    // Setup L298N direction pins as OUTPUT
    pinMode(IN1, OUTPUT);
    pinMode(IN2, OUTPUT);
    pinMode(IN3, OUTPUT);
    pinMode(IN4, OUTPUT);

    // Setup PWM for ENA and ENB (speed control)
    ledcSetup(PWM_CHANNEL_A, PWM_FREQ, PWM_RESOLUTION);
    ledcSetup(PWM_CHANNEL_B, PWM_FREQ, PWM_RESOLUTION);
    ledcAttachPin(ENA, PWM_CHANNEL_A);
    ledcAttachPin(ENB, PWM_CHANNEL_B);

    // Initialize motors stopped
    Stop();
}

void loop()
{
    if (!dmpReady)
        return;

    mpuInterrupt = false;
    mpuIntStatus = mpu.getIntStatus();
    fifoCount = mpu.getFIFOCount();

    if ((mpuIntStatus & 0x10) || fifoCount == 1024)
    {
        mpu.resetFIFO();
        Serial.println(F("FIFO overflow!"));
        return; // Skip cycle after FIFO reset
    }
    else if (mpuIntStatus & 0x02)
    {
        while (fifoCount < packetSize)
            fifoCount = mpu.getFIFOCount();

        // Check for I2C errors during read
        if (mpu.getFIFOBytes(fifoBuffer, packetSize) != 0)
        {
            Serial.println(F("I2C error!"));
            mpu.resetFIFO();
            Stop();
            delay(5);
            return;
        }
        fifoCount -= packetSize;

        mpu.dmpGetQuaternion(&q, fifoBuffer);
        mpu.dmpGetGravity(&gravity, &q);
        mpu.dmpGetYawPitchRoll(ypr, &q, &gravity);
        input = ypr[1] * 180 / M_PI + 180;

        // Validate sensor data
        if (isnan(input) || input < 0 || input > 360)
        {
            Serial.println(F("Bad data!"));
            Stop();
            mpu.resetFIFO();
            delay(5);
            return;
        }

        // COMPUTE PID with fresh data only
        pid.Compute();

        // Validate PID output
        if (isnan(output))
        {
            Serial.println(F("PID NaN!"));
            output = 0;
            Stop();
            return;
        }

        // Constrain output
        output = constrain(output, -255, 255);

        Serial.print(input);
        Serial.print(" =>");
        Serial.println(output);

        if (input > 150 && input < 220)
        {
            if (output > 0)
                Forward();
            else if (output < 0)
                Reverse();
        }
        else
            Stop();
    }
}

void Forward()
{
    // Set direction: both motors forward
    digitalWrite(IN1, HIGH);
    digitalWrite(IN2, LOW);
    digitalWrite(IN3, HIGH);
    digitalWrite(IN4, LOW);

    // Apply minimum speed threshold
    int speed = (int)output;
    if (speed > 0 && speed < minMotorSpeed)
    {
        speed = minMotorSpeed;
    }
    speed = constrain(speed, 0, 255);

    ledcWrite(PWM_CHANNEL_A, speed);
    ledcWrite(PWM_CHANNEL_B, speed);
    delayMicroseconds(50);
    Serial.print("F");
}

void Reverse()
{
    // Set direction: both motors backward
    digitalWrite(IN1, LOW);
    digitalWrite(IN2, HIGH);
    digitalWrite(IN3, LOW);
    digitalWrite(IN4, HIGH);

    // Apply minimum speed threshold
    int speed = (int)(output * -1);
    if (speed > 0 && speed < minMotorSpeed)
    {
        speed = minMotorSpeed;
    }
    speed = constrain(speed, 0, 255);

    ledcWrite(PWM_CHANNEL_A, speed);
    ledcWrite(PWM_CHANNEL_B, speed);
    delayMicroseconds(50);
    Serial.print("R");
}

void Stop()
{
    // Stop both motors
    digitalWrite(IN1, LOW);
    digitalWrite(IN2, LOW);
    digitalWrite(IN3, LOW);
    digitalWrite(IN4, LOW);

    ledcWrite(PWM_CHANNEL_A, 0);
    ledcWrite(PWM_CHANNEL_B, 0);
    Serial.print("S");
}