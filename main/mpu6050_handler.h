/**
 * @file mpu6050_handler.h
 * @brief MPU6050 DMP interface for self-balancing robot
 */

#ifndef MPU6050_HANDLER_H
#define MPU6050_HANDLER_H

#include "config.h"

/**
 * Initialize MPU6050 with DMP
 */
bool mpuInit();

/**
 * Check if DMP is ready
 */
bool mpuIsReady();

/**
 * Read current pitch angle from DMP
 */
bool mpuReadAngle(double *angle);

/**
 * Reset FIFO buffer
 */
void mpuResetFIFO();

/**
 * Set gyro offsets for calibration
 */
void mpuSetOffsets(int16_t xGyro, int16_t yGyro, int16_t zGyro, int16_t zAccel);

/**
 * Get raw sensor data for telemetry
 */
void mpuGetRawData(int16_t *ax, int16_t *ay, int16_t *az,
                   int16_t *gx, int16_t *gy, int16_t *gz);

/**
 * ISR flag check - call in main loop
 */
bool mpuDataReady();

/**
 * Clear the data ready flag
 */
void mpuClearInterrupt();

#endif // MPU6050_HANDLER_H
