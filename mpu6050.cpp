/*
 * mpu6050.cpp — QMI8658 driver implementation
 * ─────────────────────────────────────────────
 * I2C register access, calibration, and filtering for QMI8658.
 * Data output is little-endian (LSB first), unlike MPU6050.
 *
 * QMI8658 configuration used:
 *   Accelerometer: ±4g range, 250Hz ODR  → 8192 LSB/g
 *   Gyroscope:     ±512dps range, 250Hz  → 64 LSB/dps
 */
#include "mpu6050.h"
#include <Wire.h>

// QMI8658 register map
#define QMI8658_REG_WHO_AM_I   0x00   // Should return 0x05
#define QMI8658_REG_CTRL1      0x02   // Serial interface config
#define QMI8658_REG_CTRL2      0x03   // Accelerometer config
#define QMI8658_REG_CTRL3      0x04   // Gyroscope config
#define QMI8658_REG_CTRL5      0x06   // Sensor data processing
#define QMI8658_REG_CTRL7      0x08   // Enable sensors
#define QMI8658_REG_AX_L       0x35   // Accel X low byte (burst: AX_L..GZ_H = 12 bytes)

// WHO_AM_I expected value
#define QMI8658_WHO_AM_I       0x05

// CTRL2: Accel ±4g (bits[6:4]=001), ODR 250Hz (bits[3:0]=0110)
#define QMI8658_CTRL2_VAL      0x16

// CTRL3: Gyro ±512dps (bits[6:4]=101), ODR 250Hz (bits[3:0]=0110)
#define QMI8658_CTRL3_VAL      0x56

// CTRL5: Low-pass filter enabled for accel (bit0) and gyro (bit4)
#define QMI8658_CTRL5_VAL      0x11

// CTRL7: Enable accel (bit0) + gyro (bit1)
#define QMI8658_CTRL7_VAL      0x03

// Sensitivity constants
#define ACCEL_SENSITIVITY      8192.0f   // LSB/g at ±4g
#define GYRO_SENSITIVITY       64.0f     // LSB/dps at ±512dps

// Static state
static IMUData        lastIMUData;
static IMUCalibration imuCal;
static bool           deviceFound = false;

// ══════════════════════════════════════════════════════════
//  I2C REGISTER ACCESS
// ══════════════════════════════════════════════════════════

static bool imuWriteReg(uint8_t reg, uint8_t val) {
  Wire.beginTransmission(QMI8658_ADDR);
  Wire.write(reg);
  Wire.write(val);
  return (Wire.endTransmission() == 0);
}

static uint8_t imuReadReg(uint8_t reg) {
  Wire.beginTransmission(QMI8658_ADDR);
  Wire.write(reg);
  if (Wire.endTransmission(false) != 0) return 0;
  Wire.requestFrom(QMI8658_ADDR, (uint8_t)1);
  return Wire.available() ? Wire.read() : 0;
}

// Read a 16-bit little-endian value: reg=LSB, reg+1=MSB
static int16_t imuReadInt16LE(uint8_t regL) {
  Wire.beginTransmission(QMI8658_ADDR);
  Wire.write(regL);
  if (Wire.endTransmission(false) != 0) return 0;
  Wire.requestFrom(QMI8658_ADDR, (uint8_t)2);
  if (Wire.available() >= 2) {
    uint8_t lo = Wire.read();
    uint8_t hi = Wire.read();
    return (int16_t)((hi << 8) | lo);
  }
  return 0;
}

// Burst-read 12 bytes starting at AX_L: AX, AY, AZ, GX, GY, GZ (all little-endian)
static bool imuReadBurst(int16_t* ax, int16_t* ay, int16_t* az,
                          int16_t* gx, int16_t* gy, int16_t* gz) {
  Wire.beginTransmission(QMI8658_ADDR);
  Wire.write(QMI8658_REG_AX_L);
  if (Wire.endTransmission(false) != 0) return false;

  Wire.requestFrom(QMI8658_ADDR, (uint8_t)12);
  if (Wire.available() < 12) return false;

  uint8_t buf[12];
  for (int i = 0; i < 12; i++) buf[i] = Wire.read();

  // Each value is LSB first (little-endian)
  *ax = (int16_t)((buf[1]  << 8) | buf[0]);
  *ay = (int16_t)((buf[3]  << 8) | buf[2]);
  *az = (int16_t)((buf[5]  << 8) | buf[4]);
  *gx = (int16_t)((buf[7]  << 8) | buf[6]);
  *gy = (int16_t)((buf[9]  << 8) | buf[8]);
  *gz = (int16_t)((buf[11] << 8) | buf[10]);
  return true;
}

// ══════════════════════════════════════════════════════════
//  I2C BUS SCAN (diagnostic helper)
// ══════════════════════════════════════════════════════════

static void scanI2CBus() {
  Serial.println("[IMU] Scanning I2C bus for devices...");
  int found = 0;
  for (uint8_t addr = 1; addr < 127; addr++) {
    Wire.beginTransmission(addr);
    if (Wire.endTransmission() == 0) {
      Serial.printf("[IMU]   Device found at 0x%02X\n", addr);
      found++;
    }
  }
  if (found == 0) {
    Serial.println("[IMU]   No I2C devices found! Check wiring (SDA=GPIO7, SCL=GPIO8)");
  }
}

// ══════════════════════════════════════════════════════════
//  PUBLIC API IMPLEMENTATION
// ══════════════════════════════════════════════════════════

bool imuInit() {
  Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN, I2C_FREQ);
  delay(50);

  // Scan bus first to diagnose wiring issues
  scanI2CBus();

  // Verify device identity
  uint8_t whoami = imuReadReg(QMI8658_REG_WHO_AM_I);
  if (whoami != QMI8658_WHO_AM_I) {
    // Try alternate address (SA0=LOW → 0x6A)
    Serial.printf("[IMU] QMI8658 not at 0x6B (got 0x%02X), trying 0x6A...\n", whoami);
    // Note: Wire.requestFrom uses the address set per-call; try a direct check
    Wire.beginTransmission(0x6A);
    Wire.write(QMI8658_REG_WHO_AM_I);
    if (Wire.endTransmission(false) == 0) {
      Wire.requestFrom((uint8_t)0x6A, (uint8_t)1);
      if (Wire.available()) {
        uint8_t alt = Wire.read();
        Serial.printf("[IMU] QMI8658 at 0x6A returned WHO_AM_I=0x%02X\n", alt);
      }
    }
    Serial.println("[IMU] QMI8658 NOT FOUND — tilt game will be disabled");
    deviceFound = false;
    return false;
  }

  Serial.printf("[IMU] QMI8658 found at 0x%02X (WHO_AM_I=0x%02X)\n", QMI8658_ADDR, whoami);
  deviceFound = true;

  // Reset device first
  imuWriteReg(QMI8658_REG_CTRL1, 0x60);  // soft reset
  delay(15);

  // CTRL1: I2C mode, auto-increment register address
  if (!imuWriteReg(QMI8658_REG_CTRL1, 0x40)) {
    Serial.println("[IMU] Failed to configure CTRL1");
    return false;
  }

  // CTRL2: Accelerometer ±4g, 250Hz ODR
  if (!imuWriteReg(QMI8658_REG_CTRL2, QMI8658_CTRL2_VAL)) {
    Serial.println("[IMU] Failed to configure accelerometer");
    return false;
  }

  // CTRL3: Gyroscope ±512dps, 250Hz ODR
  if (!imuWriteReg(QMI8658_REG_CTRL3, QMI8658_CTRL3_VAL)) {
    Serial.println("[IMU] Failed to configure gyroscope");
    return false;
  }

  // CTRL5: Enable LPF for both accel and gyro
  if (!imuWriteReg(QMI8658_REG_CTRL5, QMI8658_CTRL5_VAL)) {
    Serial.println("[IMU] Failed to configure CTRL5");
    return false;
  }

  // CTRL7: Enable accelerometer and gyroscope
  if (!imuWriteReg(QMI8658_REG_CTRL7, QMI8658_CTRL7_VAL)) {
    Serial.println("[IMU] Failed to enable sensors");
    return false;
  }

  delay(50);  // Let sensors settle
  Serial.println("[IMU] QMI8658 initialized: ±4g accel, ±512dps gyro, 250Hz");
  return true;
}

bool imuCalibrate(uint16_t sampleCount) {
  if (!deviceFound) {
    Serial.println("[IMU] Calibration skipped: device not found");
    return false;
  }

  Serial.printf("[IMU] Calibrating (%d samples, hold device still)...\n", sampleCount);

  int32_t sumAX = 0, sumAY = 0, sumAZ = 0;
  int32_t sumGX = 0, sumGY = 0, sumGZ = 0;
  uint16_t valid = 0;

  for (uint16_t i = 0; i < sampleCount; i++) {
    int16_t ax, ay, az, gx, gy, gz;
    if (imuReadBurst(&ax, &ay, &az, &gx, &gy, &gz)) {
      sumAX += ax; sumAY += ay; sumAZ += az;
      sumGX += gx; sumGY += gy; sumGZ += gz;
      valid++;
    }
    delay(5);
  }

  if (valid < sampleCount / 2) {
    Serial.printf("[IMU] Calibration failed: only %d/%d samples valid\n", valid, sampleCount);
    return false;
  }

  imuCal.accelOffsetX = sumAX / valid;
  imuCal.accelOffsetY = sumAY / valid;
  // Z should read +1g (8192 LSB) when flat; subtract expected gravity
  imuCal.accelOffsetZ = (sumAZ / valid) - (int16_t)ACCEL_SENSITIVITY;

  imuCal.gyroOffsetX = sumGX / valid;
  imuCal.gyroOffsetY = sumGY / valid;
  imuCal.gyroOffsetZ = sumGZ / valid;

  imuCal.calibrated = true;

  Serial.printf("[IMU] Cal offsets — AX:%d AY:%d AZ:%d  GX:%d GY:%d GZ:%d\n",
                imuCal.accelOffsetX, imuCal.accelOffsetY, imuCal.accelOffsetZ,
                imuCal.gyroOffsetX,  imuCal.gyroOffsetY,  imuCal.gyroOffsetZ);
  Serial.println("[IMU] Calibration complete");
  return true;
}

bool imuRead(IMUData& outData) {
  if (!deviceFound || !imuCal.calibrated) {
    return false;
  }

  int16_t ax, ay, az, gx, gy, gz;
  if (!imuReadBurst(&ax, &ay, &az, &gx, &gy, &gz)) {
    return false;
  }

  // Apply calibration offsets
  ax -= imuCal.accelOffsetX;
  ay -= imuCal.accelOffsetY;
  az -= imuCal.accelOffsetZ;
  gx -= imuCal.gyroOffsetX;
  gy -= imuCal.gyroOffsetY;
  gz -= imuCal.gyroOffsetZ;

  // Convert to physical units
  outData.accelX = (float)ax / ACCEL_SENSITIVITY * 9.81f;  // m/s²
  outData.accelY = (float)ay / ACCEL_SENSITIVITY * 9.81f;
  outData.accelZ = (float)az / ACCEL_SENSITIVITY * 9.81f;

  outData.gyroX = (float)gx / GYRO_SENSITIVITY;  // degrees/sec
  outData.gyroY = (float)gy / GYRO_SENSITIVITY;
  outData.gyroZ = (float)gz / GYRO_SENSITIVITY;

  outData.temperature = 0;  // QMI8658 temp requires separate register read
  outData.timestamp   = millis();

  lastIMUData = outData;
  return true;
}

void imuApplyLowPassFilter(IMUData& data, float filterAlpha) {
  // EMA: alpha=1.0 → raw data, alpha=0.0 → fully smoothed
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
  memset(&imuCal, 0, sizeof(IMUCalibration));
}
