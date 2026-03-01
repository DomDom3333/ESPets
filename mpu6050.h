/*
 * mpu6050.h — QMI8658 6-axis IMU driver
 * ────────────────────────────────────
 * I2C communication, calibration, and data reading.
 * Abstracts register access and provides calibrated accelerometer/gyro data.
 *
 * NOTE: File is named mpu6050.h for compatibility with existing includes.
 * The actual hardware is a QMI8658 IMU (Waveshare ESP32C6-LCD-1.69).
 */
#pragma once

#include <Arduino.h>

// I2C configuration
// SDA=GPIO8, SCL=GPIO7 (confirmed from Waveshare ESP32-C6-LCD-1.69 hardware)
#define I2C_SDA_PIN     8        // GPIO 8 = I2C SDA
#define I2C_SCL_PIN     7        // GPIO 7 = I2C SCL
#define I2C_FREQ        400000   // 400kHz fast mode
#define QMI8658_ADDR    0x6B     // SA0=HIGH (default on Waveshare board)

// Sensor structure for 6-axis data (calibrated)
struct IMUData {
  float accelX, accelY, accelZ;   // m/s²
  float gyroX, gyroY, gyroZ;      // degrees/second
  float temperature;              // Celsius (unused but kept for API compat)
  uint32_t timestamp;             // milliseconds
};

// Calibration structure (offsets computed at boot)
struct IMUCalibration {
  int16_t accelOffsetX, accelOffsetY, accelOffsetZ;
  int16_t gyroOffsetX, gyroOffsetY, gyroOffsetZ;
  bool    calibrated;
};

// ══════════════════════════════════════════════════════════
//  PUBLIC API
// ══════════════════════════════════════════════════════════

// Initialize I2C bus and configure QMI8658
// Returns: true if device responded (WHO_AM_I check passed)
bool imuInit();

// Perform static calibration (device must be stationary for ~1 second)
// sampleCount: number of readings to average (200 = ~1 second)
// Returns: true if calibration succeeded
bool imuCalibrate(uint16_t sampleCount = 200);

// Read latest sensor data with calibration offsets applied
// outData: structure to fill with calibrated IMU readings
// Returns: true if read succeeded
bool imuRead(IMUData& outData);

// Apply exponential moving average (EMA) low-pass filter
// filterAlpha: 0.0 (all old) to 1.0 (all new); 0.7 = 70% new, 30% old
void imuApplyLowPassFilter(IMUData& data, float filterAlpha = 0.7f);

// Get calibration status
bool imuIsCalibrated();

// Reset calibration offsets to zero
void imuResetCalibration();
