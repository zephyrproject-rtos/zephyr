/* lsm9ds1.c - Driver for LSM9DS1 inertial module which includes:
 * 3D accelerometer, 3D gyroscope, 3D magnetometer.
 */

/*
 * Copyright (c) 2020 Luca Quaer
 *
 * SPDX-License-Identifier: Apache-2.0
 */

////////////////////////////////////////////////////////////////////////////////
// Includes
#include <stdio.h>
#include <drivers/i2c.h>
#include <drivers/sensor.h>
#include <init.h>
#include <sys/byteorder.h>
#include <logging/log.h>

#include <kernel.h>
#include <device.h>

#include "lsm9ds1.h"

LOG_MODULE_REGISTER(LSM9DS1, CONFIG_SENSOR_LOG_LEVEL);

////////////////////////////////////////////////////////////////////////////////
// Defines
#define DT_DRV_COMPAT st_lsm9ds1

////////////////////////////////////////////////////////////////////////////////
// Variables

// This variable holds the sensor data on read and write operations.
static struct lsm9ds1_data lsm9ds1_data;

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
// Helper functions
/**
 * This function converts the given float value to a sensor_value.
 *
 * @param f the float value to be convertet to a sensor_value struct.
 *
 * @return the sensor_value struct representing the given float value.
 */
static struct sensor_value floatToSensorValue(float f) {
    struct sensor_value sv;

    sv.val1 = (int32_t) f;
    sv.val2 = (int32_t) ((f - sv.val1) * 1000000);

    return sv;
}

/**
 * Function to wrap the function "lsm9ds1_attr_set". The original function
 * has a sensor_value as 4th attribute. This function has a uint8_t as
 * 4th attribute which is convertet to sensor_value before internally
 * calling "lsm9ds1_attr_set".
 *
 * @param dev device whose attribute should be set
 * @param chan channel of the device which is affected by the attribute change
 * @param attr attribute of the channel which should be set
 * @param val value that should be set to the attribute of the channel
 */
static void setChanAttr(
    const struct device* dev,
    enum sensor_channel chan,
    enum sensor_attribute attr,
    uint8_t val) {
////
    struct sensor_value sv;
    sv.val1 = val;
    sv.val2 = 0;

    lsm9ds1_attr_set(dev, chan, attr, &sv);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
// I²C wrapper functions

/**
 * Read a single byte.
 * This function depends on the static variable 'lsm9ds1_data'.
 *
 * @param addr address to read data from
 * @param subAddr sub-address to read data from
 *
 * @return the data read from the given address and sub-address
 */
static uint8_t readByte(uint8_t addr, uint8_t subAddr) {
    uint8_t data;

    if (i2c_reg_read_byte(lsm9ds1_data.i2c_master, addr, subAddr, &data) < 0) {
        return -EIO;
    }

    return data;
}

/**
 * Read multiple bytes.
 * This function depends on the static variable 'lsm9ds1_data'.
 *
 * @param addr address to read data from
 * @param subAddr sub-address to read data from
 * @param count length of the data to read
 * @param dest pointer to the destination of the requested data
 *
 * @return 0 on success; -EIO on failure
 */
static int readBytes(
    uint8_t addr, uint8_t subAddr,
    uint8_t count, uint8_t *dest) {
////
    if (
        i2c_burst_read(lsm9ds1_data.i2c_master, addr, subAddr, dest, count) < 0
    ) {
        return -EIO;
    }
    return 0;
}

/**
 * Write a single byte.
 * This function depends on the static variable 'lsm9ds1_data'.
 *
 * @param addr address to write data to
 * @param subAddr sub-address to write data to
 * @param data data to write
 */
static void writeByte(uint8_t addr, uint8_t subAddr, uint8_t data)
{
    i2c_reg_write_byte(lsm9ds1_data.i2c_master, addr, subAddr, data);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
// Hardware settings - helper functions

/**
 * The driver stores a resolution for each component the LSM9DS1 offers
 * (accelerometer, gyroscope and magnetometer).
 * This resolution is used to convert the raw sensor readings to useful units.
 *
 * This function updates the before mentioned resolution for all three
 * components of the LSM9DS1 (accelerometer, gyroscope, magnetometer).
 *
 * This function is (ment to be) called in
 *  -   enableAndConfigureAccl
 *  -   enableAndConfigureGyro
 *  -   enableAndConfigureMagn
 * because if settings change these functions must be called
 * and setting changes also affect the resolutions.
 * Therefore this function gets called in these functions.
 *
 * @param dev device
 */
static void updateSensorResolutions(const struct device* dev) {
    struct lsm9ds1_data *data = dev->data;

    /**
     * Get accl scale resolution.
     * Possible accelerometer scales (and their register bit settings) are:
     * 2 Gs (00), 16 Gs (01), 4 Gs (10), and 8 Gs  (11)
     */
    switch (data->accl_scale) {
        case AFS_2G : data->acclRes = 2.0/32768.0;    break;
        case AFS_16G: data->acclRes = 16.0/32768.0;   break;
        case AFS_4G : data->acclRes = 4.0/32768.0;    break;
        case AFS_8G : data->acclRes = 8.0/32768.0;    break;
    }

    /**
     * Get gyro scale resolution.
     * Possible gyro scales (and their register bit settings) are:
     * 245 DPS (00), 500 DPS (01), and 2000 DPS  (11)
     */
    switch (data->gyro_scale) {
        case GFS_245DPS : data->gyroRes = 245.0/32768.0;  break;
        case GFS_500DPS : data->gyroRes = 500.0/32768.0;  break;
        case GFS_2000DPS: data->gyroRes = 2000.0/32768.0; break;
    }

    /**
     * Get magn scale resolution.
     * Possible magnetometer scales (and their register bit settings) are:
     * 4 Gauss (00), 8 Gauss (01), 12 Gauss (10) and 16 Gauss (11)
     */
    switch (data->magn_scale) {
        case MFS_4G : data->magnRes = 4.0/32768.0;    break;
        case MFS_8G : data->magnRes = 8.0/32768.0;    break;
        case MFS_12G: data->magnRes = 12.0/32768.0;   break;
        case MFS_16G: data->magnRes = 16.0/32768.0;   break;
    }
}

/**
 * This function enables block data update and auto increment on multiple
 * byte reads.
 *
 * This function is also (ment to be) called in
 *  -   enableAndConfigureAccl
 *  -   enableAndConfigureGyro
 *  -   enableAndConfigureMagn
 *
 * @param dev device
 */
static void enableBlockDataUpdateAndAutoInc(const struct device* dev) {
    const struct lsm9ds1_config *config = dev->config;

    /**
     * Enable
     *      block data update
     * and
     *      allow auto-increment during multiple byte read.
     */
    writeByte(config->i2c_slave_addr_acclgyro, LSM9DS1XG_CTRL_REG8, 0x44);

    //printk("LSM9DS1 initialized for active data mode.\n");
}

/**
 * This function enables and configures the accelerometer.
 * When the settings
 *  -   accl_scale
 *  -   accl_outputDataRate
 *  -   accl_bandwidth
 * change, this function must be called to apply these changes.
 *
 * @param dev device
 */
static void enableAndConfigureAccl(const struct device* dev) {
    struct lsm9ds1_data *data = dev->data;
    const struct lsm9ds1_config *config = dev->config;

    /**
     * Enable the X,Y,Z axes of the accelerometer.
     *
     * CTRL_REG5_XL:
     *
     * DEC_[0:1]    : Decimation of acceleration data on OUT REG and FIFO
     *                (default: 00)
     * Zen_XL       : Enable Z axis
     * Yen_XL       : Enable Y axis
     * Xen_XL       : Enable X axis
     *
     * DEC_1    DEC_0   Zen_XL  Yen_XL  Xen_XL  0   0   0
     *   |        |        |       |       |    |   |   |
     *   0        0        1       1       1    0   0   0
     * 
     * => 0011 1000 => 0x38
     */
    writeByte(config->i2c_slave_addr_acclgyro, LSM9DS1XG_CTRL_REG5_XL, 0x38);

    /**
     * Configure the accelerometer.
     *
     * CTRL_REG6_XL:
     *
     * ODR_XL [2:0] : Output data rate and power mode selection
     * FS_XL  [1:0] : Accelerometer full-scale selection
     * BW_SCAL_ODR  : Bandwith selection
     * BW_XL  [1:0] : Anti-aliasing filter bandwith selection
     *
     * ODR_XL2  ODR_XL1 ODR_XL0 FS1_XL  FS0_XL  BW_SCAL_ODR BW_XL1  BW_XL0
     *                                                |
     *                                                1 => 0x04
     *
     * => Aodr      is a value between 0-6 (000-110)
     * => Ascale    is a value between 0-3 ( 00- 11)
     * => Abw       is a value between 0-3 ( 00- 11)
     *
     * Shifting and the binari 'or' operation places the values into the 8-bit
     * register as the documentation specifies.
     *
     */
    uint8_t Aodr   = data->accl_outputDataRate;
    uint8_t Ascale = data->accl_scale;
    uint8_t Abw    = data->accl_bandwidth;

    writeByte(
        config->i2c_slave_addr_acclgyro, LSM9DS1XG_CTRL_REG6_XL,
        Aodr << 5 | Ascale << 3 | 0x04 | Abw
    );

    /**
     * Final configurations.
     */

    // Enable auto block updates and auto increment
    enableBlockDataUpdateAndAutoInc(dev);

    // Update resoluctions
    updateSensorResolutions(dev);

    k_msleep(200);
}

/**
 * This function enables and configures the gyroscope.
 * When the settings
 *  -   gyro_scale
 *  -   gyro_outputDataRate
 *  -   gyro_bandwidth
 * change, this function must be called to apply these changes.
 *
 * @param dev device
 */
static void enableAndConfigureGyro(const struct device* dev) {
    struct lsm9ds1_data *data = dev->data;
    const struct lsm9ds1_config *config = dev->config;

    /**
     * Enable the X,Y,Z axes of the gyroscope.
     *
     * CTRL_REG4:
     *
     * [X,Y,Z]en_G  : enable the respective axis output mode.
     * LIR_XL1      : Latched Interrupt (0: Interrupt request not latched).
     * 4D_XL1       : 4D option (0: Interrupt generator uses 6D for position).
     *
     *  0   0   Zen_G   Yen_G   Xen_G   0   LIR_XL1 4D_XL1
     *  |   |     |       |       |     |      |      |
     *  0   0     1       1       1     0      0      0
     *
     *  => 0011 1000 => 0x38
     */
    writeByte(
        config->i2c_slave_addr_acclgyro, LSM9DS1XG_CTRL_REG4,
        0x38
    );


    /**
     * Configure the gyroscope.
     *
     * CTRL_REG1_G:
     *
     * ODR_G [2:0]  : Gyroscope output data rate selection.
     * FS_G  [1:0]  : Gyroscope full-scale selection.
     * BW_G  [1:0]  : Gyroscope bandwith selection.
     *
     * ODR_G2   ODR_G1  ODR_G0  FS_G1   FS_G0   0   BW_G1   BW_G0
     *
     *
     * => Godr      is a value between 0-6 (000-110)
     * => Gscale    is a value between 0-3 ( 00- 11)
     * => Gbw       is a value between 0-3 ( 00- 11)
     *
     * Shifting and the binari 'or' operation places the values into the 8-bit
     * register as the documentation specifies.
     *
     */
    uint8_t Godr   = data->gyro_outputDataRate;
    uint8_t Gscale = data->gyro_scale;
    uint8_t Gbw    = data->gyro_bandwidth;

    writeByte(
        config->i2c_slave_addr_acclgyro, LSM9DS1XG_CTRL_REG1_G,
        Godr << 5 | Gscale << 3 | Gbw
    );

    /**
     * Low power mode.
     * TODO: Sollte der low power mode während der Kalibrierung nicht
     * erlaubt sein?
     */
    #ifdef CONFIG_LSM9DS1_GYRO_LOW_POWER
        // low power gyro add
        writeByte(
            config->i2c_slave_addr_acclgyro,
            LSM9DS1XG_CTRL_REG3_G,
            (1<<7)
        );
    #endif

    /**
     * Final configurations.
     */

    // Enable auto block updates and auto increment
    enableBlockDataUpdateAndAutoInc(dev);

    // Update resoluctions
    updateSensorResolutions(dev);

    k_msleep(200);
}

/**
 * This function enables and configures the magnetometer.
 * When the settings
 *  -   magn_scale
 *  -   magn_outputDataRate
 *  -   magn_mode
 * changes, this function must be called to apply these changes.
 *
 * @param dev device
 */
static void enableAndConfigureMagn(const struct device* dev) {
    struct lsm9ds1_data *data = dev->data;
    const struct lsm9ds1_config *config = dev->config;

    /**
     * Enable the X,Y axes of the magnetometer.
     * Enable the temperature compensation.
     *
     * CTRL_REG1_M
     *
     * TEMP_COMP    : Default: 0 (1: temp. compensation enabled)
     * OM   [1:0]   : X and Y axes operative mode selection.
     * DO   [2:0]   : Output data rate selection.
     * FAST_ODR     : enables data rates higher than 80Hz.
     * ST           : Self-test enable (default 0; 1: self-test enabled).
     *
     * TEMP_COMP    OM1     OM0     DO2     DO1     DO0     FAST_ODR    ST
     *     |         |       |       |       |       |          |        |
     *     1        var     var     var     var     var         0        0
     *     |
     *     +-> 0x80
     *
     * => Mmode     is a value between 0-3 ( 00- 11)
     * => Modr      is a value between 0-6 (000-110)
     *
     * Shifting and the binari 'or' operation places the values into the 8-bit
     * register as the documentation specifies.
     */
    uint8_t Mmode  = data->magn_mode;
    uint8_t Modr   = data->magn_outputDataRate;

    writeByte(
        config->i2c_slave_addr_magn, LSM9DS1M_CTRL_REG1_M,
        0x80 | Mmode << 5 | Modr << 2
    );

    /**
     * Configure magnetometer: select full scale
     *
     * CTRL_REG2_M
     *
     * FS   [1:0]   : Full-scale configuration (default: 00)
     * REBOOT       : Reboot memory content (default: 0)
     * SOFT_RST     : Configuration registers and user registers reset function.
     *
     * 0    FS1     FS0     0   REBOOT      SOFT_RST    0       0
     * |     |       |      |     |             |       |       |
     * 0    var     var     0     0             0       0       0
     *
     * => Mscale    is a value between 0-3 ( 00- 11)
     *
     * Shifting places the values into the 8-bit
     * register as the documentation specifies.
     */
    uint8_t Mscale = data->magn_scale;

    writeByte(config->i2c_slave_addr_magn, LSM9DS1M_CTRL_REG2_M, Mscale << 5 );

    /**
     * Configure magnetometer: continuous conversion mode
     *
     * CTRL_REG3_M
     *
     * I2C_DISABLE  : 0: I²C enabled, 1: I²C disabled
     * LP           : 0: data rate is configured by CTRL_REG1M, 1: Low-pwr mode
     * SIM          : 0: SPI only write operations, 1: SPI read and write
     * MD   [1:0]   : Operating mode selection (default: 11).
     *
     * I2C_DISABLE  0   LP  0   0   SIM     MD1     MD0
     *     |        |    |  |   |    |       |       |
     *     0        0    0  0   0    0       0       0 
     *
     */
    writeByte(config->i2c_slave_addr_magn, LSM9DS1M_CTRL_REG3_M, 0x00 );

    /**
     * Configure magnetometer: z-axis mode
     *
     * CTRL_REG4_M
     *
     * OMZ [1:0]    : z-axis operative mode selection
     * BLE          : Big/Little endian data selection
     *                (default: 0 -> data LSb at lower address)
     *
     * 0    0   0   0    OMZ1   OMZ0    BLE     0
     * |    |   |   |     |      |       |      |
     * 0    0   0   0    var    var      0      0
     *
     * => Mmode     is a value between 0-3 ( 00- 11)
     *
     * Sfifting it by 2 places the values into the 8-bit register as the
     * documentation specifies.
     *
     */
    writeByte(config->i2c_slave_addr_magn, LSM9DS1M_CTRL_REG4_M, Mmode << 2 );

    /**
     * Configure magnetometer: select block update mode
     *
     * CTRL_REG5_M
     *
     * FAST_READ    : 0: disabled, 1: enabled
     * BDU          : Block data update for magn. data
     *                0: continuous update; 1: output registers not updated
     *                until MSB and LSB have been read.
     *
     * FAST_READ    BDU     0   0   0   0   0   0
     *     |         |      |   |   |   |   |   |
     *     0         1      0   0   0   0   0   0
     *
     * => 0100 0000 => 0x40
     */
    writeByte(config->i2c_slave_addr_magn, LSM9DS1M_CTRL_REG5_M, 0x40 );

    /**
     * Final configurations.
     */

    // Enable auto block updates and auto increment
    enableBlockDataUpdateAndAutoInc(dev);

    // Update resoluctions
    updateSensorResolutions(dev);

    k_msleep(200);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
// Read raw sensor data from registers - helper functions

/**
 * Read acceleration data.
 * Use the OUT_X_L_XL register.
 *
 * @param dev device
 * @param destination destination of the data that got read out
 */
static void readAcclData(const struct device* dev, int16_t* destination) {
    // Get config struct from device
    const struct lsm9ds1_config *config = dev->config;

    // X,Y,Z register data stored here (2 8-bit values for each axis).
    uint8_t rawData[6];

    if (readBytes(
            config->i2c_slave_addr_acclgyro,
            LSM9DS1XG_OUT_X_L_XL,
            6,
            rawData
        ) == 0) {
    ////
        // Turn the MSB and LSB into a signed 16-bit value.
        destination[0] = ((int16_t)rawData[1] << 8) | rawData[0];
        destination[1] = ((int16_t)rawData[3] << 8) | rawData[2];
        destination[2] = ((int16_t)rawData[5] << 8) | rawData[4];
    }
}

/**
 * Read gyro data.
 * Use the OUT_X_L_G register.
 */
static void readGyroData(const struct device* dev, int16_t * destination) {
    // Get config struct from device
    const struct lsm9ds1_config *config = dev->config;

    // X,Y,Z register data stored here (2 8-bit values for each axis).
    uint8_t rawData[6];

    if (readBytes(
            config->i2c_slave_addr_acclgyro,
            LSM9DS1XG_OUT_X_L_G,
            6,
            rawData
        ) == 0 ) {
    ////
        // Turn the MSB and LSB into a signed 16-bit value
        destination[0] = ((int16_t)rawData[1] << 8) | rawData[0];
        destination[1] = ((int16_t)rawData[3] << 8) | rawData[2];
        destination[2] = ((int16_t)rawData[5] << 8) | rawData[4];
    }
}

/**
 * Read magn data.
 * Use the OUT_X_L_M register.
 *
 * @param dev device
 * @param destination destination of the data that got read out
 */
static void readMagnData(const struct device* dev, int16_t * destination) {
    // Get config struct from device
    const struct lsm9ds1_config *config = dev->config;

    // X,Y,Z register data stored here (2 8-bit values for each axis).
    uint8_t rawData[6];

    if ( readBytes(
            config->i2c_slave_addr_magn,
            LSM9DS1M_OUT_X_L_M,
            6,
            rawData
        ) == 0) {
    ////
        // Turn the MSB and LSB into a signed 16-bit value
        destination[0] = ((int16_t)rawData[1] << 8) | rawData[0];
        destination[1] = ((int16_t)rawData[3] << 8) | rawData[2];
        destination[2] = ((int16_t)rawData[5] << 8) | rawData[4];
    }
}

/**
 * Read temp data.
 * Use OUT_TEMP_L register.
 *
 * @param dev device
 * @param destination destination of the data that got read out
 */
static void readTempData(const struct device* dev, int16_t* destination) {
    // Get config struct from device
    const struct lsm9ds1_config *config = dev->config;

    // Temperature register data stored here (2 8-bit values).
    uint8_t rawData[2];

    if( readBytes(
            config->i2c_slave_addr_acclgyro,
            LSM9DS1XG_OUT_TEMP_L,
            2,
            rawData
        ) == 0) {
    ////
        *destination = ((int16_t)rawData[1] << 8) | rawData[0];
    }
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
// Calibrate sensors

/**
 * Calibrate accelerometer.
 *
 * @param dev device
 */
static void calibrateAccl(const struct device* dev) {
    struct lsm9ds1_data *data = dev->data;
    const struct lsm9ds1_config *config = dev->config;

    /**
     * These variables are used to store the bias value before
     * writing them to their destination (accelBias_dst).
     */
    int32_t acclBias[3] = {0, 0, 0};

    /**
     * Calibrate accelerometer => calculate bias
     */

    /**
     * Enable FIFO memory.
     */
    writeByte(
        config->i2c_slave_addr_acclgyro, LSM9DS1XG_CTRL_REG9,
        readByte(config->i2c_slave_addr_acclgyro, LSM9DS1XG_CTRL_REG9) | 0x02
    );

    k_sleep(K_MSEC(50));

    /**
     * Set FIFO mode: 001 (FIFO mode, stops collecting data when FIFO is full).
     * Set FIFO threshold to: 1 1111 = 32
     */
    writeByte(
        config->i2c_slave_addr_acclgyro,
        LSM9DS1XG_FIFO_CTRL, 0x20 | 0x1F
    );

    // Wait for the FIFO to collect samples.
    k_sleep(K_MSEC(1000));

    uint16_t numSamples = (
        readByte(config->i2c_slave_addr_acclgyro, LSM9DS1XG_FIFO_SRC) & 0x2F
    );

    // Collect samples in FIFO and add them to acclBias.
    for(uint16_t i = 0; i < numSamples; i++) {
        // Make 16-bit integers from 8-bit values in FIFO.
        int16_t acclData[3] = {0, 0, 0};
        readAcclData(dev, acclData);

        acclBias[0] += (int32_t) acclData[0];
        acclBias[1] += (int32_t) acclData[1];
        acclBias[2] += (int32_t) acclData[2];
    }

    // Calculate average over samples.
    acclBias[0] /= numSamples;
    acclBias[1] /= numSamples;
    acclBias[2] /= numSamples;

    // Remove gravity from the z-axis accelerometer bias calculation
    if(acclBias[2] > 0L) {
        acclBias[2] -= (int32_t) (1.0f / data->acclRes);
    } else {
        acclBias[2] += (int32_t) (1.0f / data->acclRes);
    }

    // Scale data to get g's.
    data->acclBias[0] = (float)acclBias[0] * data->acclRes;
    data->acclBias[1] = (float)acclBias[1] * data->acclRes;
    data->acclBias[2] = (float)acclBias[2] * data->acclRes;

    /**
     * Disable FIFO memory.
     */
    writeByte(
        config->i2c_slave_addr_acclgyro, LSM9DS1XG_CTRL_REG9,
        readByte(config->i2c_slave_addr_acclgyro, LSM9DS1XG_CTRL_REG9) & ~0x02
    );
    k_msleep(50);

    /**
     * Set FIFO mode: 0000 0000 (FIFO bypass mode):
     */
    writeByte(config->i2c_slave_addr_acclgyro, LSM9DS1XG_FIFO_CTRL, 0x00);
}

/**
 * Calibrate gyroscope.
 *
 * @param dev device
 */
static void calibrateGyro(const struct device* dev) {
    struct lsm9ds1_data *data = dev->data;
    const struct lsm9ds1_config *config = dev->config;

    /**
     * These variables are used to store the bias value before
     * writing them to their destination (accelBias_dst).
     */
    int32_t gyroBias[3] = {0, 0, 0};

    /**
     * Calibrate gyroscope => calculate bias
     */

    /**
     * Enable FIFO memory.
     */
    writeByte(
        config->i2c_slave_addr_acclgyro, LSM9DS1XG_CTRL_REG9,
        readByte(config->i2c_slave_addr_acclgyro, LSM9DS1XG_CTRL_REG9) | 0x02
    );

    k_msleep(50);

    /**
     * Set FIFO mode: 001 (FIFO mode, stops collecting data when FIFO is full).
     * Set FIFO threshold to: 1 1111 = 32
     */
    writeByte(config->i2c_slave_addr_acclgyro, LSM9DS1XG_FIFO_CTRL, 0x20 | 0x1F);


    // Wait for FIFO to collect samples.
    k_msleep(1000);

    /**
     * Get number of samples stored in the FIFO.
     *
     * TODO: BUG? => Should it be "& 0x3F" to get the first 6 bits of the
     *       FIFO_SRC register?
     */
    uint16_t numSamples = (
        readByte(config->i2c_slave_addr_acclgyro, LSM9DS1XG_FIFO_SRC) & 0x2F
    );

    // Collect samples in FIFO and add them to gyroBias.
    for(uint16_t i = 0; i < numSamples ; i++) {
        // Make 16-bit integers from 8-bit values in FIFO.
        int16_t gyroData[3] = {0, 0, 0};
        readGyroData(dev, gyroData);

        gyroBias[0] += (int32_t) gyroData[0];
        gyroBias[1] += (int32_t) gyroData[1];
        gyroBias[2] += (int32_t) gyroData[2];
    }

    // Calculate average over samples.
    gyroBias[0] /= numSamples;
    gyroBias[1] /= numSamples;
    gyroBias[2] /= numSamples;

    // Scale data to get deg/s
    data->gyroBias[0] = (float)gyroBias[0] * data->gyroRes;
    data->gyroBias[1] = (float)gyroBias[1] * data->gyroRes;
    data->gyroBias[2] = (float)gyroBias[2] * data->gyroRes;

    /**
     * Disable FIFO memory.
     */
    writeByte(
        config->i2c_slave_addr_acclgyro, LSM9DS1XG_CTRL_REG9,
        readByte(config->i2c_slave_addr_acclgyro, LSM9DS1XG_CTRL_REG9) & ~0x02
    );

    k_sleep(K_MSEC(50));

    /**
     * Set FIFO mode: 0000 0000 (FIFO bypass mode).
     */
    writeByte(config->i2c_slave_addr_acclgyro, LSM9DS1XG_FIFO_CTRL, 0x00);
}

/**
 * Calibrate magnetometer.
 *
 * @param dev device
 */
static void calibrateMagn(const struct device* dev) {
    struct lsm9ds1_data *data = dev->data;
    const struct lsm9ds1_config *config = dev->config;

    int32_t magnBias[3] = {0, 0, 0};
    int16_t magn_max[3] = {0, 0, 0}, magn_min[3] = {0, 0, 0};

    printk("Mag Calibration: Wave device in a figure eight until done!\n");
    k_msleep(1000);

    uint16_t numSamples = 128;
    for(uint16_t i = 0; i < numSamples; i++) {
        int16_t magnData[3] = {0, 0, 0};
        readMagnData(dev, magnData);
        for (int k = 0; k < 3; k++) {
            if(magnData[k] > magn_max[k]) magn_max[k] = magnData[k];
            if(magnData[k] < magn_min[k]) magn_min[k] = magnData[k];
        }
        k_msleep(105); // at 10 Hz ODR, new mag data is available every 100 ms
    }

    // Calculate average
    magnBias[0]  = (magn_max[0] + magn_min[0])/2;
    magnBias[1]  = (magn_max[1] + magn_min[1])/2;
    magnBias[2]  = (magn_max[2] + magn_min[2])/2;

    // Scale data to Gauss
    data->magnBias[0] = (float) magnBias[0] * data->magnRes;
    data->magnBias[1] = (float) magnBias[1] * data->magnRes;
    data->magnBias[2] = (float) magnBias[2] * data->magnRes;

    /**
     * Write magnetometer offset register.
     */
    writeByte(
        config->i2c_slave_addr_magn, LSM9DS1M_OFFSET_X_REG_L_M,
        (int16_t) magnBias[0] & 0xFF
    );

    writeByte(
        config->i2c_slave_addr_magn, LSM9DS1M_OFFSET_X_REG_H_M,
        ((int16_t)magnBias[0] >> 8) & 0xFF
    );

    writeByte(
        config->i2c_slave_addr_magn, LSM9DS1M_OFFSET_Y_REG_L_M,
        (int16_t) magnBias[1] & 0xFF
    );

    writeByte(
        config->i2c_slave_addr_magn, LSM9DS1M_OFFSET_Y_REG_H_M,
        ((int16_t)magnBias[1] >> 8) & 0xFF
    );

    writeByte(
        config->i2c_slave_addr_magn, LSM9DS1M_OFFSET_Z_REG_L_M,
        (int16_t) magnBias[2] & 0xFF
    );

    writeByte(
        config->i2c_slave_addr_magn, LSM9DS1M_OFFSET_Z_REG_H_M,
        ((int16_t)magnBias[2] >> 8) & 0xFF
    );

    printk("Mag Calibration done!\n");
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
// API functions

static int lsm9ds1_init(const struct device *dev) {
    // Get config and data from the device
    const struct lsm9ds1_config * const config  = dev->config;
    struct lsm9ds1_data *data             = dev->data;

    // Set i2c_master by getting the i2c master device binding by name
    data->i2c_master = device_get_binding(config->i2c_master_dev_name);

    // If i2c_master was not found return.
    if (!data->i2c_master) {
        LOG_DBG("I2C master not found: %s", config->i2c_master_dev_name);
        return -EIO;
    }

    /*
     * Read the WHO_AM_I registers as a communication check
     */
    // Read the who-am-i register of the accel/gyro sensor
    int whoAmI_ac   = readByte(
        config->i2c_slave_addr_acclgyro,
        LSM9DS1XG_WHO_AM_I
    );

    // Reat the who-am-i register of the magn sensor
    int whoAmI_m    = readByte(
        config->i2c_slave_addr_magn,
        LSM9DS1M_WHO_AM_I
    );

    // Check the values for correctness
    if (whoAmI_ac == 0x68 && whoAmI_m == 0x3D) {
        printk("LSM9DS1 is online!\n");

        /**
         * Set default settings for sensors
         */

        // Accelerometer
        setChanAttr(dev, SENSOR_CHAN_ACCEL_XYZ, SENSOR_ATTR_SCALE, AFS_2G);
        setChanAttr(dev, SENSOR_CHAN_ACCEL_XYZ, SENSOR_ATTR_ODR, AODR_50Hz);
        setChanAttr(dev, SENSOR_CHAN_ACCEL_XYZ, SENSOR_ATTR_BW, ABW_211Hz);

        // Gyroscope
        setChanAttr(dev, SENSOR_CHAN_GYRO_XYZ, SENSOR_ATTR_SCALE, GFS_245DPS);
        setChanAttr(dev, SENSOR_CHAN_GYRO_XYZ, SENSOR_ATTR_ODR, GODR_59_5Hz);
        setChanAttr(dev, SENSOR_CHAN_GYRO_XYZ, SENSOR_ATTR_BW, GBW_high);

        // Magnetometer
        setChanAttr(dev, SENSOR_CHAN_MAGN_XYZ, SENSOR_ATTR_SCALE, MFS_4G);
        setChanAttr(dev, SENSOR_CHAN_MAGN_XYZ, SENSOR_ATTR_ODR, MODR_20Hz);
        setChanAttr(dev, SENSOR_CHAN_MAGN_XYZ, SENSOR_ATTR_MODE,
            MMode_HighPerformance);

        // Temperature
        setChanAttr(dev, SENSOR_CHAN_AMBIENT_TEMP, SENSOR_ATTR_OSR, ADC_256);

    } else {
        printk("Could not connect to LSM9DS1.\n");
        return -EIO;
    }

    return 0;
}

/**
 * This function is part of the "Sensor API".
 * This function is used for fetching data from the sensor
 * and saving it driver-internally (in a kind of buffer).
 *
 * @param dev device
 * @param chan channel to fetch the data from
 *
 * @return 0 on success
 */
static int lsm9ds1_sample_fetch(
    const struct device* dev,
    enum sensor_channel chan) {
////
    // Get data struct from device
    struct lsm9ds1_data *data = dev->data;

    int16_t raw[3] = {};

    switch (chan) {
        // case SENSOR_CHAN_APPLY_SETTINGS:
        //     // Enable and configure all sensors.
        //     enableAndConfigureAccl(dev);
        //     enableAndConfigureGyro(dev);
        //     enableAndConfigureMagn(dev);

        //     break;

        // case SENSOR_CHAN_CALIBRATE_ACCL:
        //     // Re-calculate resolutions based on resolution settings.
        //     updateSensorResolutions(dev);

        //     // Enable and configure sensors
        //     enableAndConfigureAccl(dev);

        //     // Calibrate sensors
        //     calibrateAccl(dev);

        //     // printf("Accl biases:\n");
        //     // printf("%f | %f | %f\n",
        //     //     data->acclBias[0], data->acclBias[1], data->acclBias[2]);

        //     break;

        // case SENSOR_CHAN_CALIBRATE_GYRO:
        //     // Re-calculate resolutions based on resolution settings.
        //     updateSensorResolutions(dev);

        //     // Enable and configure sensors
        //     enableAndConfigureGyro(dev);

        //     // Calibrate sensors
        //     calibrateGyro(dev);

        //     // printf("Gyro biases:\n");
        //     // printf("%f | %f | %f\n",
        //     //     data->gyroBias[0], data->gyroBias[1], data->gyroBias[2]);

        //     break;

        // case SENSOR_CHAN_CALIBRATE_MAGN:
        //     // Re-calculate resolutions based on resolution settings.
        //     updateSensorResolutions(dev);

        //     // Enable and configure sensors
        //     enableAndConfigureMagn(dev);

        //     // Calibrate sensors
        //     calibrateMagn(dev);

        //     // printf("Magn biases:\n");
        //     // printf("%f | %f | %f\n",
        //     //     data->magnBias[0], data->magnBias[1], data->magnBias[2]);

        //     break;

        //case SENSOR_CHAN_ACCL_XYZ:
        case SENSOR_CHAN_ACCEL_XYZ:
            // Get raw data
            readAcclData(dev, raw);
            // Convert data to G's
            data->accl_x = (float) raw[0]*data->acclRes;
            data->accl_y = (float) raw[1]*data->acclRes;
            data->accl_z = (float) raw[2]*data->acclRes;
            break;

        case SENSOR_CHAN_GYRO_XYZ:
            // Get raw data
            readGyroData(dev, raw);
            // Convert data to degrees per second
            data->gyro_x = (float) raw[0]*data->gyroRes;
            data->gyro_y = (float) raw[1]*data->gyroRes;
            data->gyro_z = (float) raw[2]*data->gyroRes;
            break;

        case SENSOR_CHAN_MAGN_XYZ:
            // Get raw data
            readMagnData(dev, raw);
            // Convert data to milli Gauss
            data->magn_x = (float) raw[0]*data->magnRes;
            data->magn_y = (float) raw[1]*data->magnRes;
            data->magn_z = (float) raw[2]*data->magnRes;
            break;

        case SENSOR_CHAN_AMBIENT_TEMP:
            // Get raw data
            readTempData(dev, raw);
            // Convert data to degrees celsius
            data->temp_c = (float) raw[0]/16.0 + 25.0;
            break;

        case SENSOR_CHAN_ALL:
        default:
            // Get raw data
            readAcclData(dev, raw);
            // Convert data to G's
            data->accl_x = (float) raw[0]*data->acclRes - data->acclBias[0];
            data->accl_y = (float) raw[1]*data->acclRes - data->acclBias[1];
            data->accl_z = (float) raw[2]*data->acclRes - data->acclBias[2];

            // Get raw data
            readGyroData(dev, raw);
            // Convert data to degrees per second
            data->gyro_x = (float) raw[0]*data->gyroRes - data->gyroBias[0];
            data->gyro_y = (float) raw[1]*data->gyroRes - data->gyroBias[1];
            data->gyro_z = (float) raw[2]*data->gyroRes - data->gyroBias[2];

            // Get raw data
            readMagnData(dev, raw);
            // Convert data to milli Gauss
            data->magn_x = (float) raw[0]*data->magnRes - data->magnBias[0];
            data->magn_y = (float) raw[1]*data->magnRes - data->magnBias[1];
            data->magn_z = (float) raw[2]*data->magnRes - data->magnBias[2];

            // Get raw data
            readTempData(dev, raw);
            // Convert data to degrees celsius
            data->temp_c = (float) raw[0]/16.0 + 25.0;
            break;
    }

    return 0;
}

/**
 * This function is part of the "Sensor API".
 * This function is used for getting (previously fetched) data
 * from the sensor.
 *
 * @param dev device
 * @param chan channel to get the data from
 * @param val destination to save the value to (pointer to sensor_value)
 *
 * @return 0 on success
 */
static int lsm9ds1_channel_get(
    const struct device *dev,
    enum sensor_channel chan,
    struct sensor_value *val) {
////
    struct lsm9ds1_data *data = dev->data;

    switch (chan) {
        //case SENSOR_CHAN_ACCL_XYZ:
        case SENSOR_CHAN_ACCEL_XYZ:
            val[0] = floatToSensorValue(data->accl_x);
            val[1] = floatToSensorValue(data->accl_y);
            val[2] = floatToSensorValue(data->accl_z);
            break;

        case SENSOR_CHAN_GYRO_XYZ:
            val[0] = floatToSensorValue(data->gyro_x);
            val[1] = floatToSensorValue(data->gyro_y);
            val[2] = floatToSensorValue(data->gyro_z);
            break;

        case SENSOR_CHAN_MAGN_XYZ:
            val[0] = floatToSensorValue(data->magn_x);
            val[1] = floatToSensorValue(data->magn_y);
            val[2] = floatToSensorValue(data->magn_z);
            break;

        case SENSOR_CHAN_AMBIENT_TEMP:
            *val = floatToSensorValue(data->temp_c);
            break;

        // case SENSOR_CHAN_ALL:
        //     *val = floatToSensorValue(123);
        //     break;

        default:
            break;
    }

    return 0;
}

/**
 * This function configures the settings on the sensors.
 *
 * As sensor_value *val you should pass a sensor_value which only contains
 * an integer (therefore the val2 property is not relevant and not used
 * in this function).
 *
 * E.g: struct sensor_value sv; sv.val1 = AFS_2G; sv.val2 = 0;
 *
 * As values for the sensor_value val1 property you should use
 * the values defined as enums in the lsm9ds1.h file.
 *
 * E.g: AFS_2G, AODR_119Hz, ABW_50Hz, ...
 */
static int lsm9ds1_attr_set(
    const struct device *dev,
    enum sensor_channel chan,
    enum sensor_attribute attr,
    const struct sensor_value *val
) {
    struct lsm9ds1_data *data = dev->data;

    /**
     * TODO: bei den ganzen Zuweisungen wird einem uint8_t ein s32_t zugewiesen.
     * Kann man das machen, signed zu unsigned?
     */

    switch (chan) {
        /**
         * Change accelerometer settings.
         */
        //case SENSOR_CHAN_ACCL_XYZ:
        case SENSOR_CHAN_ACCEL_XYZ:
            if (attr == SENSOR_ATTR_CALIB_TARGET) {
                // Apply changes to accelerometer and re-calculate resolution.
                enableAndConfigureAccl(dev);

                // Calibrate with new settings.
                calibrateAccl(dev);

            } else {
                // Change settings.
                switch (attr) {
                    case SENSOR_ATTR_SCALE:
                        data->accl_scale = val->val1;
                        break;

                    case SENSOR_ATTR_ODR:
                        data->accl_outputDataRate = val->val1;
                        break;

                    case SENSOR_ATTR_BW:
                        data->accl_bandwidth = val->val1;
                        break;

                    default:
                        break;
                }

                // Apply changes to accelerometer and re-calculate resolution.
                enableAndConfigureAccl(dev);
            }

            break; // Break the SENSOR_CHAN_ACCEL_XYZ case

        /**
         * Change gyroscope settings.
         */
        case SENSOR_CHAN_GYRO_XYZ:
            if (attr == SENSOR_ATTR_CALIB_TARGET) {
                // Apply changes to accelerometer and re-calculate resolution.
                enableAndConfigureGyro(dev);

                // Calibrate with new settings.
                calibrateGyro(dev);
            } else {
                // Change settings.
                switch (attr) {
                    case SENSOR_ATTR_SCALE:
                        data->gyro_scale = val->val1;
                        break;

                    case SENSOR_ATTR_ODR:
                        data->gyro_outputDataRate = val->val1;
                        break;

                    case SENSOR_ATTR_BW:
                        data->gyro_bandwidth = val->val1;
                        break;

                    default:
                        break;
                }

                // Apply changes to accelerometer and re-calculate resolution.
                enableAndConfigureGyro(dev);
            }

            break; // Break the SENSOR_CHAN_GYRO_XYZ case

        /**
         * Change magnetoscope settings.
         */
        case SENSOR_CHAN_MAGN_XYZ:
            if (attr == SENSOR_ATTR_CALIB_TARGET) {
                // Apply changes to accelerometer and re-calculate resolution.
                enableAndConfigureMagn(dev);

                // Calibrate with new settings.
                calibrateMagn(dev);

            } else {
                // Change settings.
                switch (attr) {
                    case SENSOR_ATTR_SCALE:
                        data->magn_scale = val->val1;
                        break;

                    case SENSOR_ATTR_ODR:
                        data->magn_outputDataRate = val->val1;
                        break;

                    case SENSOR_ATTR_MODE:
                        data->magn_mode = val->val1;
                        break;

                    default:
                        break;
                }

                // Apply changes to accelerometer and re-calculate resolution.
                enableAndConfigureMagn(dev);
            }

            break; // Break the SENSOR_CHAN_MAGN_XYZ case

        /**
         * Change temperature sensor settings.
         *
         * TODO: This setting is never used or written to a register.
         */
        case SENSOR_CHAN_AMBIENT_TEMP:
            switch (attr) {
                case SENSOR_ATTR_OSR:
                    data->temp_oversampleRate = val->val1;
                    break;

                default:
                    break;
            }
            break;

        default:
            break;
    }

    //updateSensorResolutions(dev);

    return 0;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

// This variable holds the api functions.
static struct sensor_driver_api lsm9ds1_api_funcs = {
    .sample_fetch           = lsm9ds1_sample_fetch,
    .channel_get            = lsm9ds1_channel_get,
    .attr_set               = lsm9ds1_attr_set,
};

// This variable holds the sensor configuration (I²C master; sensor bus addr.)
static struct lsm9ds1_config lsm9ds1_config = {
    .i2c_master_dev_name    = DT_INST_BUS_LABEL(0),
    .i2c_slave_addr_acclgyro= DT_INST_REG_ADDR(0),
    .i2c_slave_addr_magn    = DT_INST_REG_ADDR_BY_IDX(0, 1),
};

////////////////////////////////////////////////////////////////////////////////
// Register the driver
DEVICE_AND_API_INIT(
    lsm9ds1,
    DT_INST_LABEL(0),
    lsm9ds1_init,
    &lsm9ds1_data,
    &lsm9ds1_config,
    POST_KERNEL,
    CONFIG_SENSOR_INIT_PRIORITY,
    &lsm9ds1_api_funcs
);
////////////////////////////////////////////////////////////////////////////////