#include "mpu6050_handler.h"
#include "I2Cdev.h"
#include "MPU6050_6Axis_MotionApps20.h"
#include <Wire.h>

// MPU6050 instance
static MPU6050 mpu;

// DMP variables
static bool dmpReady = false;
static uint8_t mpuIntStatus;
static uint8_t devStatus;
static uint16_t packetSize;
static uint16_t fifoCount;
static uint8_t fifoBuffer[64];

// Orientation data
static Quaternion q;
static VectorFloat gravity;
static float ypr[3];

// Interrupt flag
static volatile bool mpuInterrupt = false;

// Mutex for I2C access
static SemaphoreHandle_t xI2CMutex = NULL;

// ISR handler
void IRAM_ATTR dmpDataReady()
{
    mpuInterrupt = true;
}

bool mpuInit()
{
    // Create I2C mutex
    xI2CMutex = xSemaphoreCreateMutex();

    // Initialize I2C
    Wire.begin(PIN_SDA, PIN_SCL);
    Wire.setClock(I2C_CLOCK_SPEED);

    DEBUG_PRINTLN(F("Initializing MPU6050..."));
    mpu.initialize();

    // Test connection
    DEBUG_PRINT(F("Testing MPU6050 connection... "));
    if (!mpu.testConnection())
    {
        DEBUG_PRINTLN(F("FAILED"));
        return false;
    }
    DEBUG_PRINTLN(F("OK"));

    // Initialize DMP
    DEBUG_PRINTLN(F("Initializing DMP..."));
    devStatus = mpu.dmpInitialize();

    // Set calibration offsets (from your calibration)
    mpu.setXGyroOffset(-2);
    mpu.setYGyroOffset(74);
    mpu.setZGyroOffset(7);
    mpu.setZAccelOffset(968);

    if (devStatus == 0)
    {
        DEBUG_PRINTLN(F("Enabling DMP..."));
        mpu.setDMPEnabled(true);

        // Setup interrupt
        pinMode(PIN_MPU_INT, INPUT);
        attachInterrupt(digitalPinToInterrupt(PIN_MPU_INT), dmpDataReady, RISING);

        mpuIntStatus = mpu.getIntStatus();
        dmpReady = true;
        packetSize = mpu.dmpGetFIFOPacketSize();

        DEBUG_PRINTLN(F("DMP ready!"));
        return true;
    }
    else
    {
        DEBUG_PRINT(F("DMP init failed (code "));
        DEBUG_PRINT(devStatus);
        DEBUG_PRINTLN(F(")"));
        return false;
    }
}

bool mpuIsReady()
{
    return dmpReady;
}

bool mpuReadAngle(double *angle)
{
    if (!dmpReady)
    {
        return false;
    }

    if (xI2CMutex != NULL)
    {
        if (xSemaphoreTake(xI2CMutex, pdMS_TO_TICKS(10)) != pdTRUE)
        {
            return false; // Couldn't get mutex in time
        }
    }

    mpuIntStatus = mpu.getIntStatus();
    fifoCount = mpu.getFIFOCount();

    // Check for FIFO overflow
    if ((mpuIntStatus & 0x10) || fifoCount == 1024)
    {
        mpu.resetFIFO();
        DEBUG_PRINTLN(F("FIFO overflow!"));
        if (xI2CMutex != NULL)
            xSemaphoreGive(xI2CMutex);
        return false;
    }

    // Check for DMP data ready
    if (!(mpuIntStatus & 0x02))
    {
        if (xI2CMutex != NULL)
            xSemaphoreGive(xI2CMutex);
        return false;
    }

    // Wait for full packet
    while (fifoCount < packetSize)
    {
        fifoCount = mpu.getFIFOCount();
    }

    // Read packet from FIFO
    if (mpu.getFIFOBytes(fifoBuffer, packetSize) != 0)
    {
        DEBUG_PRINTLN(F("I2C error!"));
        mpu.resetFIFO();
        if (xI2CMutex != NULL)
            xSemaphoreGive(xI2CMutex);
        return false;
    }
    fifoCount -= packetSize;

    // Calculate orientation
    mpu.dmpGetQuaternion(&q, fifoBuffer);
    mpu.dmpGetGravity(&gravity, &q);
    mpu.dmpGetYawPitchRoll(ypr, &q, &gravity);

    // Convert to degrees (0-360)
    *angle = ypr[1] * 180.0 / M_PI + 180.0;

    if (xI2CMutex != NULL)
        xSemaphoreGive(xI2CMutex);

    // Validate data
    if (isnan(*angle) || *angle < 0 || *angle > 360)
    {
        DEBUG_PRINTLN(F("Invalid angle data!"));
        return false;
    }

    return true;
}

void mpuResetFIFO()
{
    if (xI2CMutex != NULL)
    {
        xSemaphoreTake(xI2CMutex, portMAX_DELAY);
    }
    mpu.resetFIFO();
    if (xI2CMutex != NULL)
    {
        xSemaphoreGive(xI2CMutex);
    }
}

void mpuSetOffsets(int16_t xGyro, int16_t yGyro, int16_t zGyro, int16_t zAccel)
{
    if (xI2CMutex != NULL)
    {
        xSemaphoreTake(xI2CMutex, portMAX_DELAY);
    }
    mpu.setXGyroOffset(xGyro);
    mpu.setYGyroOffset(yGyro);
    mpu.setZGyroOffset(zGyro);
    mpu.setZAccelOffset(zAccel);
    if (xI2CMutex != NULL)
    {
        xSemaphoreGive(xI2CMutex);
    }
}

void mpuGetRawData(int16_t *ax, int16_t *ay, int16_t *az,
                   int16_t *gx, int16_t *gy, int16_t *gz)
{
    if (xI2CMutex != NULL)
    {
        xSemaphoreTake(xI2CMutex, portMAX_DELAY);
    }
    mpu.getMotion6(ax, ay, az, gx, gy, gz);
    if (xI2CMutex != NULL)
    {
        xSemaphoreGive(xI2CMutex);
    }
}

bool mpuDataReady()
{
    return mpuInterrupt;
}

void mpuClearInterrupt()
{
    mpuInterrupt = false;
}
