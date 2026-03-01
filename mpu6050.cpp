/*
 * mpu6050.cpp — MPU6050 driver implementation
 * ───────────────────────────────────────────
 * I2C register access, calibration, and filtering.
 */
#include "mpu6050.h"
#include <Wire.h>

// MPU6050 register map
#define MPU6050_REG_WHO_AM_I        0x75
#define MPU6050_REG_PWR_MGMT_1      0x6B
#define MPU6050_REG_CONFIG          0x1A
#define MPU6050_REG_GYRO_CONFIG     0x1B
#define MPU6050_REG_ACCEL_CONFIG    0x1C
#define MPU6050_REG_ACCEL_XOUT_H    0x3B  // Start of 14-byte accel+temp+gyro block
#define MPU6050_REG_TEMP_OUT_H      0x41
#define MPU6050_REG_GYRO_XOUT_H     0x43

// Static state
static IMUData           lastIMUData = {0, 0, 0, 0, 0, 0, 0, 0};
static IMUCalibration    imuCal      = {{0, 0, 0, 0, 0, 0}, false};

// ══════════════════════════════════════════════════════════
//  I2C REGISTER ACCESS
// ══════════════════════════════════════════════════════════

// Write a single byte to a register
static bool imuWriteReg(uint8_t reg, uint8_t val) {
  Wire.beginTransmission(MPU6050_ADDR);
  Wire.write(reg);
  Wire.write(val);
  uint8_t err = Wire.endTransmission();
  return (err == 0);
}

// Read a single byte from a register
static uint8_t imuReadReg(uint8_t reg) {
  Wire.beginTransmission(MPU6050_ADDR);
  Wire.write(reg);
  uint8_t err = Wire.endTransmission(false);  // don't release bus
  if (err != 0) return 0;

  Wire.requestFrom(MPU6050_ADDR, (uint8_t)1);
  if (Wire.available()) {
    return Wire.read();
  }
  return 0;
}

// Read a 16-bit big-endian value starting at regH (regH and regH+1)
static int16_t imuReadInt16(uint8_t regH) {
  Wire.beginTransmission(MPU6050_ADDR);
  Wire.write(regH);
  uint8_t err = Wire.endTransmission(false);
  if (err != 0) return 0;

  Wire.requestFrom(MPU6050_ADDR, (uint8_t)2);
  if (Wire.available() >= 2) {
    int16_t val = ((int16_t)Wire.read() << 8) | Wire.read();
    return val;
  }
  return 0;
}

// ══════════════════════════════════════════════════════════
//  PUBLIC API IMPLEMENTATION
// ══════════════════════════════════════════════════════════

bool imuInit() {
  // Initialize I2C on specified pins
  Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN, I2C_FREQ);
  delay(100);

  // Check if MPU6050 is present by reading WHO_AM_I register
  uint8_t whoami = imuReadReg(MPU6050_REG_WHO_AM_I);
  if (whoami != 0x68) {
    Serial.printf("[MPU6050] NOT FOUND (WHO_AM_I = 0x%02X, expected 0x68)\n", whoami);
    return false;
  }
  Serial.println("[MPU6050] Device found at 0x68");

  // Wake up the sensor (it starts in sleep mode)
  // Write 0x00 to PWR_MGMT_1 to disable sleep and select internal oscillator
  if (!imuWriteReg(MPU6050_REG_PWR_MGMT_1, 0x00)) {
    Serial.println("[MPU6050] Failed to wake up");
    return false;
  }
  delay(100);

  // Configure accelerometer full scale: ±8g (0x10)
  // FS_SEL = 2: ±8g, sensitivity = 4096 LSB/g
  if (!imuWriteReg(MPU6050_REG_ACCEL_CONFIG, 0x10)) {
    Serial.println("[MPU6050] Failed to configure accelerometer");
    return false;
  }

  // Configure gyroscope full scale: ±500°/s (0x08)
  // FS_SEL = 1: ±500°/s, sensitivity = 65.5 LSB/°/s
  if (!imuWriteReg(MPU6050_REG_GYRO_CONFIG, 0x08)) {
    Serial.println("[MPU6050] Failed to configure gyroscope");
    return false;
  }

  // Configure digital low-pass filter
  // CONFIG = 0x00: Accel 260Hz, Gyro 256Hz (minimal filtering, responsive)
  if (!imuWriteReg(MPU6050_REG_CONFIG, 0x00)) {
    Serial.println("[MPU6050] Failed to configure DLPF");
    return false;
  }

  Serial.println("[MPU6050] Initialized: ±8g accel, ±500°/s gyro, 400kHz I2C");
  return true;
}

bool imuCalibrate(uint16_t sampleCount) {
  if (!imuReadReg(MPU6050_REG_WHO_AM_I) == 0x68) {
    Serial.println("[MPU6050] Calibration failed: device not responding");
    return false;
  }

  Serial.printf("[MPU6050] Calibrating (%d samples, ~%dms)...\n", sampleCount, sampleCount * 5);

  int32_t sumAX = 0, sumAY = 0, sumAZ = 0;
  int32_t sumGX = 0, sumGY = 0, sumGZ = 0;

  // Collect samples while device is stationary
  for (uint16_t i = 0; i < sampleCount; i++) {
    sumAX += imuReadInt16(MPU6050_REG_ACCEL_XOUT_H);
    sumAY += imuReadInt16(MPU6050_REG_ACCEL_XOUT_H + 2);
    sumAZ += imuReadInt16(MPU6050_REG_ACCEL_XOUT_H + 4);

    sumGX += imuReadInt16(MPU6050_REG_GYRO_XOUT_H);
    sumGY += imuReadInt16(MPU6050_REG_GYRO_XOUT_H + 2);
    sumGZ += imuReadInt16(MPU6050_REG_GYRO_XOUT_H + 4);

    delay(5);
  }

  // Calculate average offsets
  // For accel: subtract averages (device should read ~0, 0, 1g when stationary)
  imuCal.accelOffsetX = sumAX / sampleCount;
  imuCal.accelOffsetY = sumAY / sampleCount;
  // Z-axis has 1g gravity when horizontal, subtract it
  imuCal.accelOffsetZ = (sumAZ / sampleCount) - 4096;  // 4096 LSB = 1g at ±8g range

  // For gyro: subtract averages (should read ~0 when stationary)
  imuCal.gyroOffsetX = sumGX / sampleCount;
  imuCal.gyroOffsetY = sumGY / sampleCount;
  imuCal.gyroOffsetZ = sumGZ / sampleCount;

  imuCal.calibrated = true;
  Serial.println("[MPU6050] Calibration complete");
  return true;
}

bool imuRead(IMUData& outData) {
  if (!imuCal.calibrated) {
    // Return zero data if not calibrated
    outData = {0, 0, 0, 0, 0, 0, 0, millis()};
    return false;
  }

  // Read all 14 bytes: accel (6) + temp (2) + gyro (6)
  // This is more efficient than 6 separate reads
  int16_t rawAX = imuReadInt16(MPU6050_REG_ACCEL_XOUT_H) - imuCal.accelOffsetX;
  int16_t rawAY = imuReadInt16(MPU6050_REG_ACCEL_XOUT_H + 2) - imuCal.accelOffsetY;
  int16_t rawAZ = imuReadInt16(MPU6050_REG_ACCEL_XOUT_H + 4) - imuCal.accelOffsetZ;

  int16_t rawT = imuReadInt16(MPU6050_REG_TEMP_OUT_H);

  int16_t rawGX = imuReadInt16(MPU6050_REG_GYRO_XOUT_H) - imuCal.gyroOffsetX;
  int16_t rawGY = imuReadInt16(MPU6050_REG_GYRO_XOUT_H + 2) - imuCal.gyroOffsetY;
  int16_t rawGZ = imuReadInt16(MPU6050_REG_GYRO_XOUT_H + 4) - imuCal.gyroOffsetZ;

  // Convert raw values to physical units
  // Accelerometer: ±8g range, 4096 LSB/g => divide by 4096
  const float ACCEL_SCALE = 1.0f / 4096.0f;
  // Gyroscope: ±500°/s range, 65.5 LSB/°/s
  const float GYRO_SCALE = 1.0f / 65.5f;

  outData.accelX = rawAX * ACCEL_SCALE * 9.8f;  // Convert to m/s²
  outData.accelY = rawAY * ACCEL_SCALE * 9.8f;
  outData.accelZ = rawAZ * ACCEL_SCALE * 9.8f;

  outData.gyroX = rawGX * GYRO_SCALE;
  outData.gyroY = rawGY * GYRO_SCALE;
  outData.gyroZ = rawGZ * GYRO_SCALE;

  // Temperature formula from datasheet: Temp(°C) = (TEMP_OUT / 340.0) + 36.53
  outData.temperature = (rawT / 340.0f) + 36.53f;
  outData.timestamp = millis();

  lastIMUData = outData;
  return true;
}

void imuApplyLowPassFilter(IMUData& data, float filterAlpha) {
  // Exponential Moving Average (EMA) filter
  // filterAlpha: 0.0 = all old data, 1.0 = all new data
  // Typical value: 0.7 = 70% new, 30% old

  data.accelX = filterAlpha * data.accelX + (1.0f - filterAlpha) * lastIMUData.accelX;
  data.accelY = filterAlpha * data.accelY + (1.0f - filterAlpha) * lastIMUData.accelY;
  data.accelZ = filterAlpha * data.accelZ + (1.0f - filterAlpha) * lastIMUData.accelZ;

  data.gyroX = filterAlpha * data.gyroX + (1.0f - filterAlpha) * lastIMUData.gyroX;
  data.gyroY = filterAlpha * data.gyroY + (1.0f - filterAlpha) * lastIMUData.gyroY;
  data.gyroZ = filterAlpha * data.gyroZ + (1.0f - filterAlpha) * lastIMUData.gyroZ;
}

bool imuIsCalibrated() {
  return imuCal.calibrated;
}

void imuResetCalibration() {
  imuCal.calibrated = false;
  memset(&imuCal, 0, sizeof(IMUCalibration));
}
