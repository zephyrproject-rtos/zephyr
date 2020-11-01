/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* Original Arduino library: https://github.com/kriswiner/LSM9DS1
*
* LSM9DS1_MS5611_t3 Basic Example Code
* by: Kris Winer
* date: November 1, 2014
* license: Beerware - Use this code however you'd like. If you
* find it useful you can buy me a beer some time.
*/


#include <stdio.h>
#include <drivers/i2c.h>
#include <init.h>
#include <sys/byteorder.h>

#include <logging/log.h>

#include "lsm9ds1.old.h"

// See also LSM9DS1 Register Map and Descriptions,
// http://www.st.com/st-web-ui/static/active/en/resource/technical/document/datasheet/DM00103319.pdf
//
// Accelerometer and Gyroscope registers
#define LSM9DS1XG_ACT_THS        0x04
#define LSM9DS1XG_ACT_DUR        0x05
#define LSM9DS1XG_INT_GEN_CFG_XL    0x06
#define LSM9DS1XG_INT_GEN_THS_X_XL  0x07
#define LSM9DS1XG_INT_GEN_THS_Y_XL  0x08
#define LSM9DS1XG_INT_GEN_THS_Z_XL  0x09
#define LSM9DS1XG_INT_GEN_DUR_XL    0x0A
#define LSM9DS1XG_REFERENCE_G       0x0B
#define LSM9DS1XG_INT1_CTRL         0x0C
#define LSM9DS1XG_INT2_CTRL         0x0D
#define LSM9DS1XG_WHO_AM_I          0x0F  // should return 0x68
#define LSM9DS1XG_CTRL_REG1_G       0x10
#define LSM9DS1XG_CTRL_REG2_G       0x11
#define LSM9DS1XG_CTRL_REG3_G       0x12
#define LSM9DS1XG_ORIENT_CFG_G      0x13
#define LSM9DS1XG_INT_GEN_SRC_G     0x14
#define LSM9DS1XG_OUT_TEMP_L        0x15
#define LSM9DS1XG_OUT_TEMP_H        0x16
#define LSM9DS1XG_STATUS_REG        0x17
#define LSM9DS1XG_OUT_X_L_G         0x18
#define LSM9DS1XG_OUT_X_H_G         0x19
#define LSM9DS1XG_OUT_Y_L_G         0x1A
#define LSM9DS1XG_OUT_Y_H_G         0x1B
#define LSM9DS1XG_OUT_Z_L_G         0x1C
#define LSM9DS1XG_OUT_Z_H_G         0x1D
#define LSM9DS1XG_CTRL_REG4         0x1E
#define LSM9DS1XG_CTRL_REG5_XL      0x1F
#define LSM9DS1XG_CTRL_REG6_XL      0x20
#define LSM9DS1XG_CTRL_REG7_XL      0x21
#define LSM9DS1XG_CTRL_REG8         0x22
#define LSM9DS1XG_CTRL_REG9         0x23
#define LSM9DS1XG_CTRL_REG10        0x24
#define LSM9DS1XG_INT_GEN_SRC_XL    0x26
//#define LSM9DS1XG_STATUS_REG        0x27 // duplicate of 0x17!
#define LSM9DS1XG_OUT_X_L_XL        0x28
#define LSM9DS1XG_OUT_X_H_XL        0x29
#define LSM9DS1XG_OUT_Y_L_XL        0x2A
#define LSM9DS1XG_OUT_Y_H_XL        0x2B
#define LSM9DS1XG_OUT_Z_L_XL        0x2C
#define LSM9DS1XG_OUT_Z_H_XL        0x2D
#define LSM9DS1XG_FIFO_CTRL         0x2E
#define LSM9DS1XG_FIFO_SRC          0x2F
#define LSM9DS1XG_INT_GEN_CFG_G     0x30
#define LSM9DS1XG_INT_GEN_THS_XH_G  0x31
#define LSM9DS1XG_INT_GEN_THS_XL_G  0x32
#define LSM9DS1XG_INT_GEN_THS_YH_G  0x33
#define LSM9DS1XG_INT_GEN_THS_YL_G  0x34
#define LSM9DS1XG_INT_GEN_THS_ZH_G  0x35
#define LSM9DS1XG_INT_GEN_THS_ZL_G  0x36
#define LSM9DS1XG_INT_GEN_DUR_G     0x37
//
// Magnetometer registers
#define LSM9DS1M_OFFSET_X_REG_L_M   0x05
#define LSM9DS1M_OFFSET_X_REG_H_M   0x06
#define LSM9DS1M_OFFSET_Y_REG_L_M   0x07
#define LSM9DS1M_OFFSET_Y_REG_H_M   0x08
#define LSM9DS1M_OFFSET_Z_REG_L_M   0x09
#define LSM9DS1M_OFFSET_Z_REG_H_M   0x0A
#define LSM9DS1M_WHO_AM_I           0x0F  // should be 0x3D
#define LSM9DS1M_CTRL_REG1_M        0x20
#define LSM9DS1M_CTRL_REG2_M        0x21
#define LSM9DS1M_CTRL_REG3_M        0x22  // Set power-mode to: power-down mode
#define LSM9DS1M_CTRL_REG4_M        0x23
#define LSM9DS1M_CTRL_REG5_M        0x24
#define LSM9DS1M_STATUS_REG_M       0x27
#define LSM9DS1M_OUT_X_L_M          0x28
#define LSM9DS1M_OUT_X_H_M          0x29
#define LSM9DS1M_OUT_Y_L_M          0x2A
#define LSM9DS1M_OUT_Y_H_M          0x2B
#define LSM9DS1M_OUT_Z_L_M          0x2C
#define LSM9DS1M_OUT_Z_H_M          0x2D
#define LSM9DS1M_INT_CFG_M          0x30
#define LSM9DS1M_INT_SRC_M          0x31
#define LSM9DS1M_INT_THS_L_M        0x32
#define LSM9DS1M_INT_THS_H_M        0x33


#define LSM9DS1XG_ADDRESS 0x6B   //  Device address
#define LSM9DS1M_ADDRESS  0x1E   //  Address of magnetometer

struct lsm9ds1_data lsm9ds1_driver;

// Set initial input parameters
enum Ascale {  // set of allowable accel full scale settings
    AFS_2G = 0,
    AFS_16G,
    AFS_4G,
    AFS_8G
};

enum Aodr {  // set of allowable gyro sample rates
    AODR_PowerDown = 0,
    AODR_10Hz,
    AODR_50Hz,
    AODR_119Hz,
    AODR_238Hz,
    AODR_476Hz,
    AODR_952Hz
};

enum Abw {  // set of allowable accewl bandwidths
    ABW_408Hz = 0,
    ABW_211Hz,
    ABW_105Hz,
    ABW_50Hz
};

enum Gscale {  // set of allowable gyro full scale settings
    GFS_245DPS = 0,
    GFS_500DPS,
    GFS_NoOp,
    GFS_2000DPS
};

enum Godr {  // set of allowable gyro sample rates
    GODR_PowerDown = 0,
    GODR_14_9Hz,
    GODR_59_5Hz,
    GODR_119Hz,
    GODR_238Hz,
    GODR_476Hz,
    GODR_952Hz
};

enum Gbw {   // set of allowable gyro data bandwidths
    GBW_low = 0,  // 14 Hz at Godr = 238 Hz,  33 Hz at Godr = 952 Hz
    GBW_med,      // 29 Hz at Godr = 238 Hz,  40 Hz at Godr = 952 Hz
    GBW_high,     // 63 Hz at Godr = 238 Hz,  58 Hz at Godr = 952 Hz
    GBW_highest   // 78 Hz at Godr = 238 Hz, 100 Hz at Godr = 952 Hz
};

enum Mscale {  // set of allowable mag full scale settings
    MFS_4G = 0,
    MFS_8G,
    MFS_12G,
    MFS_16G
};

enum Mmode {
    MMode_LowPower = 0,
    MMode_MedPerformance,
    MMode_HighPerformance,
    MMode_UltraHighPerformance
};

enum Modr {  // set of allowable mag sample rates
    MODR_0_625Hz = 0,
    MODR_1_25Hz,
    MODR_2_5Hz,
    MODR_5Hz,
    MODR_10Hz,
    MODR_20Hz,
    MODR_80Hz
};

#define ADC_256  0x00 // define pressure and temperature conversion rates
#define ADC_512  0x02
#define ADC_1024 0x04
#define ADC_2048 0x06
#define ADC_4096 0x08
#define ADC_D1   0x40
#define ADC_D2   0x50

// Specify sensor full scale
uint8_t OSR = ADC_256;      // set pressure amd temperature oversample rate
uint8_t Gscale = GFS_2000DPS; // gyro full scale ( default GFS_245DPS )
uint8_t Godr = GODR_59_5Hz;   // gyro data sample rate
uint8_t Gbw = GBW_low;       // gyro data bandwidth
uint8_t Ascale = AFS_16G;     // accel full scale ( default AFS_2G )
uint8_t Aodr = AODR_50Hz;   // accel data sample rate
uint8_t Abw = ABW_50Hz;      // accel data bandwidth
uint8_t Mscale = MFS_12G;     // mag full scale
uint8_t Modr = MODR_10Hz;    // mag data sample rate
uint8_t Mmode = MMode_LowPower;  // magnetometer operation mode

//uint8_t OSR = ADC_1024;      // set pressure amd temperature oversample rate
//uint8_t Gscale = GFS_2000DPS; // gyro full scale ( default GFS_245DPS )
//uint8_t Godr = GODR_238Hz;   // gyro data sample rate
//uint8_t Gbw = GBW_med;       // gyro data bandwidth
//uint8_t Ascale = AFS_16G;     // accel full scale ( default AFS_2G )
//uint8_t Aodr = AODR_238Hz;   // accel data sample rate
//uint8_t Abw = ABW_211Hz;      // accel data bandwidth
//uint8_t Mscale = MFS_12G;     // mag full scale
//uint8_t Modr = MODR_10Hz;    // mag data sample rate
//uint8_t Mmode = MMode_MedPerformance;  // magnetometer operation mode

float aRes, gRes, mRes;      // scale resolutions per LSB for the sensors

int16_t accelCount[3], gyroCount[3], magCount[3];  // Stores the 16-bit signed accelerometer, gyro, and mag sensor output

float gyroBias[3] = {0, 0, 0}, accelBias[3] = {0, 0, 0},  magBias[3] = {0, 0, 0}; // Bias corrections for gyro, accelerometer, and magnetometer

// I2C read/write functions for the LSM9DS1and AK8963 sensors
static void writeByte(uint8_t address, uint8_t subAddress, uint8_t data)
{
    i2c_reg_write_byte(lsm9ds1_driver.i2c, address, subAddress, data);
}

static uint8_t readByte(u8_t address, u8_t subAddress)
{
    u8_t data;

    if(i2c_reg_read_byte(lsm9ds1_driver.i2c, address, subAddress, &data) < 0){
        return -EIO;
    }

    return data;
}

static int readBytes(uint8_t address, uint8_t subAddress, uint8_t count, uint8_t * dest)
{

    if(i2c_burst_read(lsm9ds1_driver.i2c, address, subAddress, dest, count) < 0){
        return -EIO;
    }
    
    return 0;
}

//===================================================================================================================
//====== Set of useful function to access acceleration. gyroscope, magnetometer, and temperature data
//===================================================================================================================

static void getMres() {
    switch (Mscale)
    {
            // Possible magnetometer scales (and their register bit settings) are:
            // 4 Gauss (00), 8 Gauss (01), 12 Gauss (10) and 16 Gauss (11)
        case MFS_4G:
            mRes = 4.0/32768.0;
            break;
        case MFS_8G:
            mRes = 8.0/32768.0;
            break;
        case MFS_12G:
            mRes = 12.0/32768.0;
            break;
        case MFS_16G:
            mRes = 16.0/32768.0;
            break;
    }
}

static void getGres() {
    switch (Gscale)
    {
            // Possible gyro scales (and their register bit settings) are:
            // 245 DPS (00), 500 DPS (01), and 2000 DPS  (11).
        case GFS_245DPS:
            gRes = 245.0/32768.0;
            break;
        case GFS_500DPS:
            gRes = 500.0/32768.0;
            break;
        case GFS_2000DPS:
            gRes = 2000.0/32768.0;
            break;
    }
}

static void getAres() {
    switch (Ascale)
    {
            // Possible accelerometer scales (and their register bit settings) are:
            // 2 Gs (00), 16 Gs (01), 4 Gs (10), and 8 Gs  (11).
        case AFS_2G:
            aRes = 2.0/32768.0;
            break;
        case AFS_16G:
            aRes = 16.0/32768.0;
            break;
        case AFS_4G:
            aRes = 4.0/32768.0;
            break;
        case AFS_8G:
            aRes = 8.0/32768.0;
            break;
    }
}

static void readAccelData(int16_t * destination)
{
    //OUT_X_L_XL
    uint8_t rawData[6];  // x/y/z accel register data stored here
    //if (readBytes( LSM9DS1XG_ADDRESS, LSM9DS1XG_OUT_X_L_XL, 6, rawData) == 6){
    if (readBytes( LSM9DS1XG_ADDRESS, LSM9DS1XG_OUT_X_L_XL, 6, rawData) == 0){
        // Read the six raw data registers into data array
        destination[0] = ((int16_t)rawData[1] << 8) | rawData[0] ;  // Turn the MSB and LSB into a signed 16-bit value
        destination[1] = ((int16_t)rawData[3] << 8) | rawData[2] ;
        destination[2] = ((int16_t)rawData[5] << 8) | rawData[4] ;
        //printk("readAccelData\n");
    }
}

static void readGyroData(int16_t * destination)
{
    uint8_t rawData[6];  // x/y/z gyro register data stored here
    if ( readBytes(LSM9DS1XG_ADDRESS, LSM9DS1XG_OUT_X_L_G, 6, rawData) == 0 ){ // Read the six raw data registers sequentially into data array
        destination[0] = ((int16_t)rawData[1] << 8) | rawData[0] ;  // Turn the MSB and LSB into a signed 16-bit value
        destination[1] = ((int16_t)rawData[3] << 8) | rawData[2] ;
        destination[2] = ((int16_t)rawData[5] << 8) | rawData[4] ;
    }
}

static void readMagData(int16_t * destination)
{
    uint8_t rawData[6];  // x/y/z gyro register data stored here
    if ( readBytes(LSM9DS1M_ADDRESS, LSM9DS1M_OUT_X_L_M, 6, rawData) == 0){  // Read the six raw data registers sequentially into data array
        destination[0] = ((int16_t)rawData[1] << 8) | rawData[0] ;  // Turn the MSB and LSB into a signed 16-bit value
        destination[1] = ((int16_t)rawData[3] << 8) | rawData[2] ;  // Data stored as little Endian
        destination[2] = ((int16_t)rawData[5] << 8) | rawData[4] ;
    }
}

static void readTempData(int16_t* destination)
{

    uint8_t rawData[2];
    int16_t rawTemp = 0;

    if( readBytes(LSM9DS1XG_ADDRESS, LSM9DS1XG_OUT_TEMP_L, 2, rawData) == 0){
        rawTemp = ((int16_t)rawData[1] << 8) | rawData[0];
        *destination = rawTemp;
    }

}

// Function which accumulates gyro and accelerometer data after device initialization. It calculates the average
// of the at-rest readings and then loads the resulting offsets into accelerometer and gyro bias registers.
static void accelgyrocalLSM9DS1(float * dest1, float * dest2)
{
    uint8_t data[6] = {0, 0, 0, 0, 0, 0};
    int32_t gyro_bias[3] = {0, 0, 0}, accel_bias[3] = {0, 0, 0};
    uint16_t samples, ii;
    
    // enable the 3-axes of the gyroscope
    writeByte(LSM9DS1XG_ADDRESS, LSM9DS1XG_CTRL_REG4, 0x38);

    // configure the gyroscope
    writeByte(LSM9DS1XG_ADDRESS, LSM9DS1XG_CTRL_REG1_G, Godr << 5 | Gscale << 3 | Gbw);
    k_sleep(K_MSEC(200));

    // enable the three axes of the accelerometer
    writeByte(LSM9DS1XG_ADDRESS, LSM9DS1XG_CTRL_REG5_XL, 0x38);

    // configure the accelerometer-specify bandwidth selection with Abw
    writeByte(LSM9DS1XG_ADDRESS, LSM9DS1XG_CTRL_REG6_XL, Aodr << 5 | Ascale << 3 | 0x04 |Abw);
    k_sleep(K_MSEC(200));

    // enable block data update, allow auto-increment during multiple byte read
    writeByte(LSM9DS1XG_ADDRESS, LSM9DS1XG_CTRL_REG8, 0x44);
    
    // First get gyro bias
    int c = readByte(LSM9DS1XG_ADDRESS, LSM9DS1XG_CTRL_REG9);
    writeByte(LSM9DS1XG_ADDRESS, LSM9DS1XG_CTRL_REG9, c | 0x02);     // Enable gyro FIFO
    k_sleep(K_MSEC(50));                                                       // Wait for change to take effect
    writeByte(LSM9DS1XG_ADDRESS, LSM9DS1XG_FIFO_CTRL, 0x20 | 0x1F);  // Enable gyro FIFO stream mode and set watermark at 32 samples
    k_sleep(K_MSEC(1000));  // delay 1000 milliseconds to collect FIFO samples
    
    samples = (readByte(LSM9DS1XG_ADDRESS, LSM9DS1XG_FIFO_SRC) & 0x2F); // Read number of stored samples
    
    for(ii = 0; ii < samples ; ii++) {            // Read the gyro data stored in the FIFO
        int16_t gyro_temp[3] = {0, 0, 0};
        readBytes(LSM9DS1XG_ADDRESS, LSM9DS1XG_OUT_X_L_G, 6, data);
        gyro_temp[0] = (int16_t) (((int16_t)data[1] << 8) | data[0]); // Form signed 16-bit integer for each sample in FIFO
        gyro_temp[1] = (int16_t) (((int16_t)data[3] << 8) | data[2]);
        gyro_temp[2] = (int16_t) (((int16_t)data[5] << 8) | data[4]);
        
        gyro_bias[0] += (int32_t) gyro_temp[0]; // Sum individual signed 16-bit biases to get accumulated signed 32-bit biases
        gyro_bias[1] += (int32_t) gyro_temp[1];
        gyro_bias[2] += (int32_t) gyro_temp[2];
    }
    
    gyro_bias[0] /= samples; // average the data
    gyro_bias[1] /= samples;
    gyro_bias[2] /= samples;
    
    dest1[0] = (float)gyro_bias[0]*gRes;  // Properly scale the data to get deg/s
    dest1[1] = (float)gyro_bias[1]*gRes;
    dest1[2] = (float)gyro_bias[2]*gRes;
    
    c = readByte(LSM9DS1XG_ADDRESS, LSM9DS1XG_CTRL_REG9);
    writeByte(LSM9DS1XG_ADDRESS, LSM9DS1XG_CTRL_REG9, c & ~0x02);   //Disable gyro FIFO
    k_sleep(K_MSEC(50));
    writeByte(LSM9DS1XG_ADDRESS, LSM9DS1XG_FIFO_CTRL, 0x00);  // Enable gyro bypass mode
    
    // now get the accelerometer bias
    c = readByte(LSM9DS1XG_ADDRESS, LSM9DS1XG_CTRL_REG9);
    writeByte(LSM9DS1XG_ADDRESS, LSM9DS1XG_CTRL_REG9, c | 0x02);     // Enable accel FIFO
    k_sleep(K_MSEC(50));                                                       // Wait for change to take effect
    writeByte(LSM9DS1XG_ADDRESS, LSM9DS1XG_FIFO_CTRL, 0x20 | 0x1F);  // Enable accel FIFO stream mode and set watermark at 32 samples
    k_sleep(K_MSEC(1000));  // delay 1000 milliseconds to collect FIFO samples
    
    samples = (readByte(LSM9DS1XG_ADDRESS, LSM9DS1XG_FIFO_SRC) & 0x2F); // Read number of stored samples
    
    for(ii = 0; ii < samples ; ii++) {            // Read the accel data stored in the FIFO
        int16_t accel_temp[3] = {0, 0, 0};
        readBytes(LSM9DS1XG_ADDRESS, LSM9DS1XG_OUT_X_L_XL, 6, data);
        accel_temp[0] = (int16_t) (((int16_t)data[1] << 8) | data[0]); // Form signed 16-bit integer for each sample in FIFO
        accel_temp[1] = (int16_t) (((int16_t)data[3] << 8) | data[2]);
        accel_temp[2] = (int16_t) (((int16_t)data[5] << 8) | data[4]);
        
        accel_bias[0] += (int32_t) accel_temp[0]; // Sum individual signed 16-bit biases to get accumulated signed 32-bit biases
        accel_bias[1] += (int32_t) accel_temp[1];
        accel_bias[2] += (int32_t) accel_temp[2];
    }
    
    accel_bias[0] /= samples; // average the data
    accel_bias[1] /= samples;
    accel_bias[2] /= samples;
    
    if(accel_bias[2] > 0L) {accel_bias[2] -= (int32_t) (1.0f/aRes);}  // Remove gravity from the z-axis accelerometer bias calculation
    else {accel_bias[2] += (int32_t) (1.0f/aRes);}
    
    dest2[0] = (float)accel_bias[0]*aRes;  // Properly scale the data to get g
    dest2[1] = (float)accel_bias[1]*aRes;
    dest2[2] = (float)accel_bias[2]*aRes;
    
    c = readByte(LSM9DS1XG_ADDRESS, LSM9DS1XG_CTRL_REG9);
    writeByte(LSM9DS1XG_ADDRESS, LSM9DS1XG_CTRL_REG9, c & ~0x02);   //Disable accel FIFO
    k_msleep(50);
    writeByte(LSM9DS1XG_ADDRESS, LSM9DS1XG_FIFO_CTRL, 0x00);  // Enable accel bypass mode
}

static void magcalLSM9DS1(float * dest1)
{
    uint8_t data[6]; // data array to hold mag x, y, z, data
    uint16_t ii = 0, sample_count = 0;
    int32_t mag_bias[3] = {0, 0, 0};
    int16_t mag_max[3] = {0, 0, 0}, mag_min[3] = {0, 0, 0};
    
    // configure the magnetometer-enable temperature compensation of mag data
    writeByte(LSM9DS1M_ADDRESS, LSM9DS1M_CTRL_REG1_M, 0x80 | Mmode << 5 | Modr << 2); // select x,y-axis mode
    writeByte(LSM9DS1M_ADDRESS, LSM9DS1M_CTRL_REG2_M, Mscale << 5 ); // select mag full scale
    writeByte(LSM9DS1M_ADDRESS, LSM9DS1M_CTRL_REG3_M, 0x00 ); // continuous conversion mode
    writeByte(LSM9DS1M_ADDRESS, LSM9DS1M_CTRL_REG4_M, Mmode << 2 ); // select z-axis mode
    writeByte(LSM9DS1M_ADDRESS, LSM9DS1M_CTRL_REG5_M, 0x40 ); // select block update mode
    
//    printk("Mag Calibration: Wave device in a figure eight until done!\n");
    k_sleep(K_MSEC(4000));
    
    sample_count = 128;
    for(ii = 0; ii < sample_count; ii++) {
        int16_t mag_temp[3] = {0, 0, 0};
        readBytes(LSM9DS1M_ADDRESS, LSM9DS1M_OUT_X_L_M, 6, data);  // Read the six raw data registers into data array
        mag_temp[0] = (int16_t) (((int16_t)data[1] << 8) | data[0]) ;   // Form signed 16-bit integer for each sample in FIFO
        mag_temp[1] = (int16_t) (((int16_t)data[3] << 8) | data[2]) ;
        mag_temp[2] = (int16_t) (((int16_t)data[5] << 8) | data[4]) ;
        for (int jj = 0; jj < 3; jj++) {
            if(mag_temp[jj] > mag_max[jj]) mag_max[jj] = mag_temp[jj];
            if(mag_temp[jj] < mag_min[jj]) mag_min[jj] = mag_temp[jj];
        }
        k_sleep(K_MSEC(105));  // at 10 Hz ODR, new mag data is available every 100 ms
    }

    mag_bias[0]  = (mag_max[0] + mag_min[0])/2;  // get average x mag bias in counts
    mag_bias[1]  = (mag_max[1] + mag_min[1])/2;  // get average y mag bias in counts
    mag_bias[2]  = (mag_max[2] + mag_min[2])/2;  // get average z mag bias in counts
    
    dest1[0] = (float) mag_bias[0]*mRes;  // save mag biases in G for main program
    dest1[1] = (float) mag_bias[1]*mRes;
    dest1[2] = (float) mag_bias[2]*mRes;
    
    //write biases to accelerometermagnetometer offset registers as counts);
    writeByte(LSM9DS1M_ADDRESS, LSM9DS1M_OFFSET_X_REG_L_M, (int16_t) mag_bias[0]  & 0xFF);
    writeByte(LSM9DS1M_ADDRESS, LSM9DS1M_OFFSET_X_REG_H_M, ((int16_t)mag_bias[0] >> 8) & 0xFF);
    writeByte(LSM9DS1M_ADDRESS, LSM9DS1M_OFFSET_Y_REG_L_M, (int16_t) mag_bias[1] & 0xFF);
    writeByte(LSM9DS1M_ADDRESS, LSM9DS1M_OFFSET_Y_REG_H_M, ((int16_t)mag_bias[1] >> 8) & 0xFF);
    writeByte(LSM9DS1M_ADDRESS, LSM9DS1M_OFFSET_Z_REG_L_M, (int16_t) mag_bias[2] & 0xFF);
    writeByte(LSM9DS1M_ADDRESS, LSM9DS1M_OFFSET_Z_REG_H_M, ((int16_t)mag_bias[2] >> 8) & 0xFF);
    
//    printk("Mag Calibration done!\n");
}

static void initLSM9DS1(void)
{
    // enable the 3-axes of the gyroscope
    writeByte(LSM9DS1XG_ADDRESS, LSM9DS1XG_CTRL_REG4, 0x38);
    // configure the gyroscope
    writeByte(LSM9DS1XG_ADDRESS, LSM9DS1XG_CTRL_REG1_G, Godr << 5 | Gscale << 3 | Gbw);
    
    #ifdef CONFIG_LSM9DS1_GYRO_LOW_POWER
        // low power gyro add
        writeByte(LSM9DS1XG_ADDRESS, LSM9DS1XG_CTRL_REG3_G, (1<<7));
    #endif
    
    k_sleep(K_MSEC(100));

    // enable the three axes of the accelerometer
    writeByte(LSM9DS1XG_ADDRESS, LSM9DS1XG_CTRL_REG5_XL, 0x38);

    // configure the accelerometer-specify bandwidth selection with Abw
    writeByte(LSM9DS1XG_ADDRESS, LSM9DS1XG_CTRL_REG6_XL, Aodr << 5 | Ascale << 3 | 0x04 |Abw);
    k_sleep(K_MSEC(100));

    // enable block data update, allow auto-increment during multiple byte read
    writeByte(LSM9DS1XG_ADDRESS, LSM9DS1XG_CTRL_REG8, 0x44);

    // configure the magnetometer-enable temperature compensation of mag data
    writeByte(LSM9DS1M_ADDRESS, LSM9DS1M_CTRL_REG1_M, 0x80 | Mmode << 5 | Modr << 2); // select x,y-axis mode
    writeByte(LSM9DS1M_ADDRESS, LSM9DS1M_CTRL_REG2_M, Mscale << 5 ); // select mag full scale

#ifndef CONFIG_LSM9DS1_GYRO_LOW_POWER
    writeByte(LSM9DS1M_ADDRESS, LSM9DS1M_CTRL_REG3_M, 0x00 ); // continuous conversion mode
#else
    writeByte(LSM9DS1M_ADDRESS, LSM9DS1M_CTRL_REG3_M,  (1<<5) | (Mmode | 0x03) ); // continuous conversion mode GYRO LOW POWER
#endif
    
    writeByte(LSM9DS1M_ADDRESS, LSM9DS1M_CTRL_REG4_M, Mmode << 2 ); // select z-axis mode
    writeByte(LSM9DS1M_ADDRESS, LSM9DS1M_CTRL_REG5_M, 0x40 ); // select block update mode
    
//    printk("LSM9DS1 initialized for active data mode....\n");

    
}

//static void sleepGyro(bool enable)
//{
//
//    uint8_t temp = readByte(LSM9DS1XG_ADDRESS, LSM9DS1XG_CTRL_REG9);
//
//    if (enable){
//       temp |= (1<<6);
//    } else {
//       temp &= ~(1<<6);
//    }
//
//    writeByte(LSM9DS1XG_ADDRESS, LSM9DS1XG_CTRL_REG9, temp);
//    //k_sleep(K_MSEC(10));
//
//}

static void lsm9ds1_channel_get(struct device *dev,
                               enum sensor_channel chan,
                               float* fp_val)
{
    struct lsm9ds1_data *drv_data = dev->driver_data;

    switch (chan) {
        case SENSOR_CHAN_ACCEL_XYZ:
            memcpy(&fp_val[0], &drv_data->accel_x, sizeof(float));
            memcpy(&fp_val[1], &drv_data->accel_y, sizeof(float));
            memcpy(&fp_val[2], &drv_data->accel_z, sizeof(float));
            break;

        case SENSOR_CHAN_ACCEL_X:
            memcpy(fp_val, &drv_data->accel_x, sizeof(float));
            break;

        case SENSOR_CHAN_ACCEL_Y:
            memcpy(fp_val, &drv_data->accel_y, sizeof(float));
            break;

        case SENSOR_CHAN_ACCEL_Z:
            memcpy(fp_val, &drv_data->accel_z, sizeof(float));
            break;

        case SENSOR_CHAN_GYRO_XYZ:
            memcpy(&fp_val[0], &drv_data->gyro_x, sizeof(float));
            memcpy(&fp_val[1], &drv_data->gyro_y, sizeof(float));
            memcpy(&fp_val[2], &drv_data->gyro_z, sizeof(float));
            break;

        case SENSOR_CHAN_GYRO_X:
            memcpy(fp_val, &drv_data->gyro_x, sizeof(float));
            break;

        case SENSOR_CHAN_GYRO_Y:
            memcpy(fp_val, &drv_data->gyro_y, sizeof(float));
            break;

        case SENSOR_CHAN_GYRO_Z:
            memcpy(fp_val, &drv_data->gyro_z, sizeof(float));
            break;

        case SENSOR_CHAN_MAGN_XYZ:
            memcpy(&fp_val[0], &drv_data->magn_x, sizeof(float));
            memcpy(&fp_val[1], &drv_data->magn_y, sizeof(float));
            memcpy(&fp_val[2], &drv_data->magn_z, sizeof(float));
            break;

        case SENSOR_CHAN_MAGN_X:
            memcpy(fp_val, &drv_data->magn_x, sizeof(float));
            break;

        case SENSOR_CHAN_MAGN_Y:
            memcpy(fp_val, &drv_data->magn_y, sizeof(float));
            break;

        case SENSOR_CHAN_MAGN_Z:
            memcpy(fp_val, &drv_data->magn_z, sizeof(float));
            break;
            
        case SENSOR_CHAN_AMBIENT_TEMP:
            memcpy(fp_val, &drv_data->temperature_c, sizeof(float));
            break;
        
        default:
            break;
    }
    
    //return 0;
}

static void lsm9ds1_sample_fetch(struct device *dev)
{
    struct lsm9ds1_data *drv_data = dev->driver_data;

//    sleepGyro(false);
    
    //if (readByte(LSM9DS1XG_ADDRESS, LSM9DS1XG_STATUS_REG) & 0x01) {  // check if new accel data is ready
    readAccelData(accelCount);  // Read the x/y/z adc values
    
    // Now we'll calculate the accleration value into actual g's
    drv_data->accel_x = (float)accelCount[0]*aRes - accelBias[0];  // get actual g value, this depends on scale being set
    drv_data->accel_y = (float)accelCount[1]*aRes - accelBias[1];
    drv_data->accel_z = (float)accelCount[2]*aRes - accelBias[2];
    
    //if (readByte(LSM9DS1XG_ADDRESS, LSM9DS1XG_STATUS_REG) & 0x02) {  // check if new gyro data is ready
    readGyroData(gyroCount);  // Read the x/y/z adc values
    //
    //        // Calculate the gyro value into actual degrees per second
    drv_data->gyro_x = (float)gyroCount[0]*gRes - gyroBias[0];  // get actual gyro value, this depends on scale being set
    drv_data->gyro_y = (float)gyroCount[1]*gRes - gyroBias[1];
    drv_data->gyro_z = (float)gyroCount[2]*gRes - gyroBias[2];

//    sleepGyro(true);
    
    //if (readByte(LSM9DS1M_ADDRESS, LSM9DS1M_STATUS_REG_M) & 0x08) {  // check if new mag data is ready
    readMagData(magCount);  // Read the x/y/z adc values
    //
    //        // Calculate the magnetometer values in milliGauss
    //        // Include factory calibration per data sheet and user environmental corrections
    drv_data->magn_x = (float)magCount[0]*mRes; // - magBias[0];  // get actual magnetometer value, this depends on scale being set
    drv_data->magn_y = (float)magCount[1]*mRes; // - magBias[1];
    drv_data->magn_z = (float)magCount[2]*mRes; // - magBias[2];
    
    //}

    // Temperature is a 12-bit signed integer
    int16_t temperature_raw = 0;
    readTempData(&temperature_raw);
    drv_data->temperature_c = (float)(temperature_raw)/16.0 + 25.0;
    
    //return 0;
}

static void lsm9ds1_sensor_performance(struct device *dev, lsm9ds1_perform perform)
{

    switch(perform){
      case LOW:
      {
        // Specify sensor full scale
        OSR = ADC_256; //ADC_4096;      // set pressure amd temperature oversample rate
        Gscale = GFS_245DPS; //GFS_2000DPS; // gyro full scale
        Godr = GODR_14_9Hz;  //GODR_238Hz;   // gyro data sample rate
        Gbw = GBW_low; //GBW_med;       // gyro data bandwidth
        Ascale = AFS_2G;     //AFS_16G; // accel full scale
        Aodr = AODR_10Hz; //AODR_238Hz;   // accel data sample rate
        Abw = ABW_50Hz;      // accel data bandwidth
        Mscale = MFS_4G;     // mag full scale
        Modr = MODR_0_625Hz;    //MODR_10Hz;    // mag data sample rate
        Mmode = MMode_LowPower; //MMode_HighPerformance;  // magnetometer operation mode
      }
        break;

      case MID:
      {
       // Specify sensor full scale
        OSR = ADC_256; //ADC_4096;      // set pressure amd temperature oversample rate
        Gscale = GFS_245DPS; //GFS_2000DPS; // gyro full scale
        Godr = GODR_14_9Hz;  //GODR_238Hz;   // gyro data sample rate
        Gbw = GBW_low; //GBW_med;       // gyro data bandwidth
         Ascale = AFS_8G;     // accel full scale
         Aodr = AODR_238Hz;    // accel data sample rate
         Abw = ABW_50Hz;       // accel data bandwidth
         Mscale = MFS_4G;      // mag full scale
         Modr = MODR_10Hz;     // mag data sample rate
         Mmode = MMode_MedPerformance;  // magnetometer operation mode            
      }
        break;

      case HIGH:
      {
       // Specify sensor full scale
         OSR = ADC_4096;       // set pressure amd temperature oversample rate
         Gscale = GFS_2000DPS; // gyro full scale
         Godr = GODR_238Hz;    // gyro data sample rate
         Gbw = GBW_med;        // gyro data bandwidth
         Ascale = AFS_16G;     // accel full scale
         Aodr = AODR_238Hz;    // accel data sample rate
         Abw = ABW_50Hz;       // accel data bandwidth
         Mscale = MFS_4G;      // mag full scale
         Modr = MODR_5Hz;     // mag data sample rate
         Mmode = MMode_HighPerformance;  // magnetometer operation mode      
      }
        break;
      default:
        break;
    }

    getAres();
    getGres();
    getMres();
        
}

static bool init_done(struct device *dev)
{
    struct lsm9ds1_data* drv_data = dev->driver_data;

    if (drv_data->i2c == NULL) {
        u32_t i2c_cfg = I2C_SPEED_SET(I2C_SPEED_STANDARD) | I2C_MODE_MASTER;
        drv_data->i2c = device_get_binding(CONFIG_LSM9DS1_I2C_MASTER_DEV_NAME);
        i2c_configure(drv_data->i2c, i2c_cfg);
    }

    // Read the WHO_AM_I registers, this is a good test of communication
    printk("LSM9DS1 9-axis motion sensor...\n");
    int c = readByte(LSM9DS1XG_ADDRESS, LSM9DS1XG_WHO_AM_I);  // Read WHO_AM_I register for LSM9DS1 accel/gyro
    int d = readByte(LSM9DS1M_ADDRESS, LSM9DS1M_WHO_AM_I);  // Read WHO_AM_I register for LSM9DS1 magnetometer
    
    printk("WHO AM I: 0x%x, 0x%x\n", c, d);
    
    // WHO_AM_I should always be 0x0E for the accel/gyro and 0x3C for the mag
    if (c == 0x68 && d == 0x3D) {
        printk("LSM9DS1 is online...\n");
        
        // get sensor resolutions, only need to do this once
        getAres();
        getGres();
        getMres();
        
        printk(" Calibrate gyro and accel\n");
        accelgyrocalLSM9DS1(gyroBias, accelBias); // Calibrate gyro and accelerometers, load biases in bias registers
        
        magcalLSM9DS1(magBias);
        k_sleep(K_MSEC(500)); // add delay to see results before serial spew of data
        
        // Initialize device for active mode read of acclerometer, gyroscope, and temperature
        initLSM9DS1();
        k_sleep(K_MSEC(10));
        
        //Sleeping gyro
        //sleepGyro(true);
        
    } else {
        printk("Could not connect to LSM9DS1: 0x%x\n", c);
        return false;
    }

//    drv_data->initDone = true;
    
    return true;
}


static const struct lsm9ds1_api lsm9ds1_driver_api = {
    .sample_fetch = lsm9ds1_sample_fetch,
    .channel_get = lsm9ds1_channel_get,
    .sensor_performance = lsm9ds1_sensor_performance,
    .init_done           = init_done,
};

int lsm9ds1_init(struct device *dev)
{
    u32_t i2c_cfg = I2C_SPEED_SET(I2C_SPEED_STANDARD) | I2C_MODE_MASTER;
    struct lsm9ds1_data* drv_data = dev->driver_data;

    drv_data->i2c = device_get_binding(CONFIG_LSM9DS1_I2C_MASTER_DEV_NAME);


    if (drv_data->i2c == NULL) {
        printk("Failed to get pointer to %s device\n", CONFIG_LSM9DS1_I2C_MASTER_DEV_NAME);
        return -EINVAL;
    }else{

        i2c_configure(drv_data->i2c, i2c_cfg);

    }

    return 0;
}

DEVICE_AND_API_INIT(lsm9ds1, CONFIG_LSM9DS1_NAME, lsm9ds1_init, &lsm9ds1_driver,
                    NULL, POST_KERNEL, CONFIG_SENSOR_INIT_PRIORITY,
                    &lsm9ds1_driver_api);

/** @} */
