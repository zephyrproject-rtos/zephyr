/*
 * Copyright (c) 2012-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include<zephyr/device.h>
#include<zephyr/devicetree.h>
#include <zephyr/drivers/i2c.h>
#include"acc_gyro.h"

#define ACC_GYRO dev, 0x68
#define MAGETO dev, 0x0C
#define CLOCK_FREQUENCY 40000000
float _gres, _ares, _mres; // Gyroscope, Accelerometer, and Magnetometer resolution
float _magXcoef, _magYcoef, _magZcoef; // Magnetometer calibration coefficients for X, Y, Z axes

// Constants for step detection algorithm
const double lowPassFilterAlpha = 0.9; // alpha value for the low-pass filter applied to the accelerometer data
const double highLineAlpha = 0.0005; // rate of adjustment for the high line threshold
const double lowLineAlpha = 0.0005; // rate of adjustment for the low line threshold
const double highLineMin = 1.5; // minimum value for the high line threshold
const double highLineMax = 2.0; // maximum value for the high line threshold
const double lowLineMin = -2.0; // minimum value for the low line threshold
const double lowLineMax = -1.5; // maximum value for the low line threshold
const long stepTimeInterval = 100; // Milliseconds

// Step detection variables
double lastAccelZValue = -0.1;  // stores the previous Z-axis acceleration value
long lastCheckTime = 0; // stores the timestamp of the last check or update in milliseconds
bool highLineState = true;  // the current state of the high line threshold detection
bool lowLineState = true; // the current state of the low line threshold detection.
bool passageState = false;  // passage from high to low or vice versa has occurred, indicating a step
double highLine = 1; // threshold value for detecting upward acceleration (stepping phase)
double highBoundaryLine = 0; // the boundary for the high line threshold
double highBoundaryLineAlpha = 1.0; // scaling factor for adjusting the high boundary line
double lowLine = -1; // threshold value for detecting downward acceleration (lifting phase)
double lowBoundaryLine = 0; // the boundary for the low line threshold
double lowBoundaryLineAlpha = -1.0; // scaling factor for adjusting the low boundary line

/**
 * @fn mpu9250_read(const struct device *dev, uint8_t slave_addr, uint8_t reg_addr, uint8_t num, uint8_t* buffer)
 * @brief The function reads data from MPU9250 sensor
 * @details The function `mpu9250_read` reads data from an MPU9250 sensor using I2C communication.
 * @param dev Specify the I2C interface number used by MPU9250 sensor
 * @param slave_addr address of the slave device on the I2C bus 
 * @param reg_addr register address from which data needs to be read from the MPU9250 sensor 
 * @param num number of bytes to read from the MPU9250 sensor. 
 * @param buffer pointer to an array where the data read from the MPU9250 sensor will be stored
 */
uint64_t get_mcycle_start() {
  uint64_t mcycle = 0;
  __asm__ volatile("csrr %0,mcycle":"=r"(mcycle));
  return mcycle;
}

uint64_t get_mcycle_stop(){
  uint64_t mcycle;
  __asm__ volatile("csrr %0,mcycle":"=r"(mcycle));
  return mcycle;
}
int millis(int total_cycles)
{
	return total_cycles/(CLOCK_FREQUENCY/1000);
}
void mpu9250_read(const struct device *dev, uint8_t slave_addr, uint8_t reg_addr, uint8_t num, uint8_t* buffer){
    uint8_t i = 0;
	uint8_t msg_arr[1];
	struct i2c_msg msg_obj;
	msg_obj.buf = msg_arr;
    for(i = 0; i < num; i++){
	  msg_obj.len = 1;
	  msg_arr[0] = reg_addr;
	  msg_obj.flags = I2C_MSG_WRITE;
	  i2c_transfer(dev,&msg_obj,1,slave_addr);
      msg_obj.flags = I2C_MSG_READ;
	  i2c_transfer(dev,&msg_obj,1,slave_addr);
	  buffer[i++] = msg_arr[0];
    }
}

/**
 * @fn mpu9250_write(const struct device *dev, uint8_t slave_addr, uint8_t reg_addr, uint8_t data)
 * @brief The function `mpu9250_write` is used to write data
 * @details The function `mpu9250_write` writes data to an MPU9250 sensor using I2C communication 
 * @param dev Specify the I2C interface number used by MPU9250 sensor
 * @param slave_addr address of the slave device on the I2C bus 
 * @param reg_addr register address from which data needs to be read from the MPU9250 sensor 
 * @param data data to write to a specific register in the MPU9250 sensor
 */
void mpu9250_write(const struct device *dev, uint8_t slave_addr, uint8_t reg_addr, uint8_t data){
	  uint8_t msg_arr[2];
	  struct i2c_msg msg_obj;
	  msg_obj.buf = msg_arr;
	  msg_obj.len = 2;
	  msg_arr[0] = reg_addr;
      msg_arr[1] = data;
	  msg_obj.flags = I2C_MSG_WRITE;
	  i2c_transfer(dev,&msg_obj,1,slave_addr);
}

/**
 * @fn accel_xyz(const struct device *dev, uint8_t slave_addr, float* ax, float* ay, float* az)
 * @brief The function `accel_xyz` reads raw accelerometer data
 * @details The function `accel_xyz` reads raw accelerometer data from an MPU9250 sensor 
 * and converts it to float values for the x, y, and z axes
 * @param dev Specify the I2C interface number used by MPU9250 sensor
 * @param slave_addr address of the slave device on the I2C bus 
 * @param ax pointer to a float variable where the acceleration value along the X-axis 
 * will be stored after converting the raw accelerometer data.
 * @param ay pointer to a float variable where the acceleration value along the Y-axis 
 * will be stored after converting the raw accelerometer data.
 * @param az pointer to a float variable where the acceleration value along the Z-axis 
 * will be stored after converting the raw accelerometer data.
 */
void accel_xyz(const struct device *dev, uint8_t slave_addr, float* ax, float* ay, float* az){
    uint8_t data[6]; // Array to store raw accelerometer data (2 bytes per axis)
    int16_t axc, ayc, azc; // Variables to store combined accelerometer data for each axis

    // Read raw accelerometer data from MPU9250 starting at ACCEL_XOUT_H register
    mpu9250_read(dev, slave_addr, ACCEL_XOUT_H, 6, data); 

    // Combine high and low bytes for each accelerometer axis
    axc = ((int16_t)data[0] << 8) | data[1]; 
    ayc = ((int16_t)data[2] << 8) | data[3]; 
    azc = ((int16_t)data[4] << 8) | data[5];

    // Convert raw accelerometer data to float values using the accelerometer resolution (_ares)
    *ax = (float)axc * _ares;
    *ay = (float)ayc * _ares;
    *az = (float)azc * _ares;
}

/**
 * @fn gyro_xyz(const struct device *dev, uint8_t slave_addr, float* gx, float* gy, float* gz)
 * @brief The function `gyro_xyz` reads raw gyroscope data
 * @details The function `gyro_xyz` reads raw gyroscope data from an MPU9250 sensor 
 * and converts it to float values for the x, y, and z axes.
 * @param dev Specify the I2C interface number used by MPU9250 sensor
 * @param slave_addr address of the slave device on the I2C bus 
 * @param gx pointer to a float variable where the gyroscope value along for the X-axis 
 * will be stored after converting the raw gyroscope data.
 * @param gy pointer to a float variable where the gyroscope value along for the Y-axis 
 * will be stored after converting the raw gyroscope data.
 * @param gz pointer to a float variable where the gyroscope value along for the Z-axis 
 * will be stored after converting the raw gyroscope data.
 */
void gyro_xyz(const struct device *dev, uint8_t slave_addr, float* gx, float* gy, float* gz){
    uint8_t data[6]; // Array to store raw gyroscope data (2 bytes per axis)
    int16_t gxc, gyc, gzc; // Variables to store combined gyroscope data for each axis

    // Read raw gyroscope data from MPU9250 starting at GYRO_XOUT_L register
    mpu9250_read(dev, slave_addr, GYRO_XOUT_L, 6, data);

    // Combine high and low bytes for each gyroscope axis
    gxc = ((int16_t)data[0] << 8) | data[1];
    gyc = ((int16_t)data[2] << 8) | data[3];
    gzc = ((int16_t)data[4] << 8) | data[5];

    // Convert raw gyroscope data to float values using the gyroscope resolution (_gres)
    *gx = (float)gxc * _gres;
    *gy = (float)gyc * _gres;
    *gz = (float)gzc * _gres;
}

/**
 * @fn magneto_xyz(const struct device *dev, uint8_t slave_addr, float* mx, float* my, float* mz)
 * @brief The function `magneto_xyz` reads raw magnetometer data
 * @details The function `magneto_xyz` reads raw magnetometer data from an AK8963 magnetometer, 
 * combines the data for each axis, and converts it to float values using resolution and coefficients.
 * @param dev Specify the I2C interface number used by MPU9250 sensor
 * @param slave_addr address of the slave device on the I2C bus 
 * @param mx pointer to a float variable where the magnetometer value along for the X-axis 
 * will be stored after converting the raw magnetometer data
 * @param my pointer to a float variable where the magnetometer value along for the Y-axis 
 * will be stored after converting the raw magnetometer data
 * @param mz pointer to a float variable where the magnetometer value along for the Z-axis 
 * will be stored after converting the raw magnetometer data
 */
void magneto_xyz(const struct device *dev, uint8_t slave_addr, float* mx, float* my, float* mz){
    uint8_t data[7]; // Array to store raw magnetometer data (2 bytes per axis + 1 for status)
    uint8_t drdy; // Variable to store data ready status from magnetometer
    int16_t mxc, myc, mzc; // Variables to store combined magnetometer data for each axis

    // Read data ready status from AK8963 magnetometer
    mpu9250_read(dev, slave_addr, MAGNETO_ST1, 1, &drdy);

    // Check if data is ready
    if(drdy & 0x01){
        // Read raw magnetometer data from AK8963 starting at HXL register
        mpu9250_read(dev, slave_addr, MAGNETO_HXL, 7, data);

        // Check for magnetic sensor overflow
        if(!(data[6] & 0x08)){
            // Combine high and low bytes for each magnetometer axis
            mxc = ((int16_t)data[0] << 8) | data[1];
            myc = ((int16_t)data[3] << 8) | data[2];
            mzc = ((int16_t)data[5] << 8) | data[4];

            // Convert raw magnetometer data to float values using magnetometer resolution and coefficients
            *mx = (float)mxc * _mres * _magXcoef;
            *my = (float)myc * _mres * _magYcoef;
            *mz = (float)mzc * _mres * _magZcoef;
        }
    }
}

/**
 * @fn mpu9250_config(const struct device *dev, uint8_t slave_addr, uint8_t gfs, uint8_t afs)
 * @brief The function `mpu9250_config` configures the MPU9250 sensor
 * @details The function `mpu9250_config` configures the MPU9250 sensor with specified gyro and accelerometer
 * full scales, and sets various register values for operation.
 * @param dev Specify the I2C interface number used by MPU9250 sensor
 * @param slave_addr address of the slave device on the I2C bus 
 * @param gfs gfs(Gyroscope Full Scale) specify the full scale range for the gyroscope sensor
 * @param afs afs(accelerometer Full Scale) specify the full scale range for the accelerometer sensor
 */
void mpu9250_config(const struct device *dev, uint8_t slave_addr){
    // Configure gyro full scale
	uint8_t gfs = GYRO_FS_250;
	uint8_t afs = ACCEL_FS_2G;
    switch (gfs)
    {
    case GYRO_FS_250:
        _gres = 250.0/32768.0; // Set gyro resolution for ±250 dps
        break;
    case GYRO_FS_500:
        _gres = 500.0/32768.0; // Set gyro resolution for ±500 dps
        break;
    case GYRO_FS_1000:
        _gres = 1000.0/32768.0; // Set gyro resolution for ±1000 dps
        break;
    case GYRO_FS_2000:
        _gres = 2000.0/32768.0; // Set gyro resolution for ±2000 dps
        break;
    }

    // Configure accelerometer full scale
    switch (afs)
    {
    case ACCEL_FS_2G:
        _ares = 2.0/32768.0; // Set accel resolution for ±2g
        break;
    case ACCEL_FS_4G:
        _ares = 4.0/32768.0; // Set accel resolution for ±4g
        break;
    case ACCEL_FS_8G:
        _ares = 8.0/32768.0; // Set accel resolution for ±8g
        break;
    case ACCEL_FS_16G:
        _ares = 16.0/32768.0; // Set accel resolution for ±16g
        break;
    }

    // Turn off sleep mode
    mpu9250_write(dev, slave_addr, PWR_MGMT_1, 0x00);

    // Auto-select clock source
    mpu9250_write(dev, slave_addr, PWR_MGMT_1, 0x01);

    // Set Digital Low Pass Filter Configuration
    mpu9250_write(dev, slave_addr, CONFIG, 0x03); 

    // Set sample rate divider
    mpu9250_write(dev, slave_addr, INT_SAMP_RATE, 0x04);

    // Set gyro full scale select 
    mpu9250_write(dev, slave_addr, GYRO_CONFIG, gfs << 3); 

    // Set accel full scale select
    mpu9250_write(dev, slave_addr, ACCEL_CONFIG, afs << 3); 

    // Set accel Digital Low Pass Filter Configuration
    mpu9250_write(dev, slave_addr, ACCEL_CONFIG_2, 0x03); 

    // Enable I2C bypass for external magnetometer
    mpu9250_write(dev, slave_addr, INT_PIN_CFG, 0x02); 
}

/**
 * @fn ak8963_config(const struct device *dev, uint8_t slave_addr, uint8_t mode, uint8_t mfs)
 * @brief The function `ak8963_config` configures the AK8963 sensor
 * @details The function `ak8963_config` configures the AK8963 sensor with specified Magneto Scale Select 
 * and sets various register values for operation.
 * @param dev Specify the I2C interface number used by AK8963 sensor
 * @param slave_addr address of the slave device on the I2C bus 
 * @param mode specify the Magneto Mode Select for the AK8963 magnetometer
 * @param mfs mfs(Magneto Scale Select) represents the magnetometer scale factor and
 * determine the resolution of the magnetometer output
 */
void ak8963_config(const struct device *dev, uint8_t slave_addr){
    uint8_t data[3]; // Array to store calibration coefficient data from AK8963 magnetometer
	uint8_t mode = MAGNETO_MODE_C8HZ;
	uint8_t mfs= MAGNETO_BIT_16;
    // Set magnetometer resolution based on selected scale factor
    switch(mfs){
        case MAGNETO_BIT_14:
            _mres = 4912.0 / 8190.0; // Set resolution for 14-bit output
            break;
        case MAGNETO_BIT_16:
            _mres = 4912.0 / 32760.0; // Set resolution for 16-bit output
            break;
    }

    // Set power down mode for AK8963
    mpu9250_write(dev, slave_addr, MAGNETO_CNTL1, 0x00);

    // Set read FuseROM mode to obtain magnetometer calibration coefficients
    mpu9250_write(dev, slave_addr, MAGNETO_CNTL1, 0x0F);

    // Read magnetometer calibration coefficients
    mpu9250_read(dev, slave_addr, MAGNETO_ASAX, 3, data);
    _magXcoef = (float)(data[0] - 128) / 256.0 + 1.0;
    _magYcoef = (float)(data[1] - 128) / 256.0 + 1.0;
    _magZcoef = (float)(data[2] - 128) / 256.0 + 1.0;

    // Set power down mode again before changing mode
    mpu9250_write(dev, slave_addr, MAGNETO_CNTL1, 0x00);

    // Set magnetometer scale and mode
    mpu9250_write(dev, slave_addr, MAGNETO_CNTL1, (mfs << 4 | mode));
}

/**
 * @fn readStepDetection(float ax, float ay, float az)
 * @brief The function `readStepDetection` implements a step detection algorithm
 * @details The function `readStepDetection` implements a step detection algorithm using accelerometer data 
 * and state transitions based on threshold values.
 * @param ax Handling step detection based on accelerometer data `ax` represent the acceleration values in the x,
 * The function processes the accelerometer data to detect steps based on certain thresholds
 * @param ay Handling step detection based on accelerometer data `ay` represent the acceleration values in the y,
 * The function processes the accelerometer data to detect steps based on certain thresholds
 * @param az Handling step detection based on accelerometer data `az` represent the acceleration values in the z,
 * The function processes the accelerometer data to detect steps based on certain thresholds
 */
void readStepDetection(float ax, float ay, float az){
    long currentTime = millis(get_mcycle_stop()); // Get the current time in milliseconds
    long gapTime = currentTime - lastCheckTime; // Calculate time gap since last check

    // Initialize lastAccelZValue
    if (lastAccelZValue == -9999) {
        lastAccelZValue = az;
        printf("lastAccelZValue\n");
    }

    // Apply low pass filter to the accelerometer Z-value
    double zValue = (lowPassFilterAlpha * lastAccelZValue) + (1 - lowPassFilterAlpha) * az;
    // printf("    %d", zValue);

    // Update highLine and highBoundaryLine based on their states
    if (highLineState && highLine > highLineMin) {
        highLine -= highLineAlpha; // Decrease highLine value by highLineAlpha
        highBoundaryLine = highLine * highBoundaryLineAlpha;  // Update highBoundaryLine based on the updated highLine value
        printf("highBoundaryLine\n");
    }

    // Update lowLine and lowBoundaryLine based on their states
    if (lowLineState && lowLine < lowLineMax) {
        lowLine += lowLineAlpha; // Increase lowLine value by lowLineAlpha
        lowBoundaryLine = lowLine * lowBoundaryLineAlpha; // Update lowBoundaryLine based on the updated lowLine value
        printf("lowBoundaryLine\n");
    }

    // Check conditions for highLineState change and potential step detection
    if (highLineState && gapTime > stepTimeInterval && zValue > highBoundaryLine) {
        highLineState = false;
        printf("highLineState\n");
    }

    // Check conditions for lowLineState change
    if (lowLineState && zValue < lowBoundaryLine && passageState) {
        lowLineState = false;
        printf("lowLineState\n");
    }

    // Handle transition to highLineState
    if (!highLineState) {
        if (zValue > highLine) {
            highLine = zValue; // Update highLine with the new zValue
            highBoundaryLine = highLine * highBoundaryLineAlpha; // Update highBoundaryLine based on the updated highLine value
            printf("highBoundaryLine0\n");

            // Limit highLine value    
            if (highLine > highLineMax) {
                highLine = highLineMax;
                highBoundaryLine = highLine * highBoundaryLineAlpha; // Update highBoundaryLine based on the updated highLine value
                printf("highBoundaryLine\n");
            }
        } 
        else {
            if (highBoundaryLine > zValue) {
                highLineState = true; // Transition back to highLineState 
                passageState = true; // set passageState 
                printf("passageState\n");
            }
        }
    }

    // Handle transition to lowLineState
    if (!lowLineState && passageState) {
        if (zValue < lowLine) {
            lowLine = zValue; // Update lowLine with the new zValue
            lowBoundaryLine = lowLine * lowBoundaryLineAlpha; // Update lowBoundaryLine based on the updated lowLine value
            printf("if condition");

            // Limit lowLine value to lowLineMin
            if (lowLine < lowLineMin) {
                lowLine = lowLineMin;
                lowBoundaryLine = lowLine * lowBoundaryLineAlpha; // Update lowBoundaryLine based on the updated lowLine value
            }
        } 
        else {
            if (lowBoundaryLine < zValue) {
                lowLineState = true; // Transition back to lowLineState
                passageState = false; // set passageState

                // Step is detected here
                static int stepCount = 0;
                // Increment step count
                stepCount++;
                printf("Step count: %d\n", stepCount);

                // Update last check time
                lastCheckTime = currentTime;
            }
        }
    }

    lastAccelZValue = zValue; // Update lastAccelZValue for next iteration
}

void main(){
    uint8_t x = get_mcycle_start(); // Get the start time in milliseconds
    float ax, ay, az; // Variables to store accelerometer data (x, y, z)
    float gx, gy, gz; // Variables to store gyroscope data (x, y, z)
    float mx, my, mz; // Variables to store magnetometer data (x, y, z)
	const struct device * dev = DEVICE_DT_GET(DT_NODELABEL(i2c0));
    // configure the mpu9250 for accelerometer and gyroscope
    mpu9250_config(ACC_GYRO);

    // // configure the ak8963 for magnetometer
    // ak8963_config(MAGETO);

    while(1){
        // Read accelerometer data
        accel_xyz(ACC_GYRO, &ax, &ay, &az);

        // // Read gyroscope data
        // gyro_xyz(ACC_GYRO, &gx, &gy, &gz);

        // // Read magnetometer data
        // magneto_xyz(MAGETO, &mx, &my, &mz);

        // waitfor(5);
        // printf("******************************************\n");
        // printf("AX = %f\t GX = %f\t MX = %f\n", ax, gx, mx);
        // printf("AY = %f\t GY = %f\t MY = %f\n", ay, gy, my);
        // printf("AZ = %f\t GZ = %f\t MZ = %f\n", az, gz, mz);

        // Print accelerometer data
        printf("AX = %f\t AY = %f\t AZ = %f\n", ax, ay, az);

        // detect the step count
        readStepDetection(ax, ay, az);
    }
}