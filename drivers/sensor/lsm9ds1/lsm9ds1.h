#ifndef ZEPHYR_DRIVERS_SENSOR_LSM9DS1_LSM9DS1_H_
#define ZEPHYR_DRIVERS_SENSOR_LSM9DS1_LSM9DS1_H_

////////////////////////////////////////////////////////////////////////////////
// Includes
#include <device.h>
//#include <drivers/gpio.h>
//#include <sys/util.h>
#include <zephyr/types.h>
//#include <drivers/sensor.h>

////////////////////////////////////////////////////////////////////////////////
// Define Registers

// Accelerometer and Gyroscope registers
#define LSM9DS1XG_ACT_THS           0x04
#define LSM9DS1XG_ACT_DUR           0x05
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

/**
 * The address of the accl, gyro and magn are set in the devicetree.
 * 
 * reg = <0x6B 0x1E>;
 */
//#define LSM9DS1XG_ADDRESS           0x6B   //  Device address
//#define LSM9DS1M_ADDRESS            0x1E   //  Address of magnetometer

////////////////////////////////////////////////////////////////////////////////
// Define API functions
static int lsm9ds1_attr_set(
    struct device *dev,
    enum sensor_channel chan,
    enum sensor_attribute attr,
    const struct sensor_value *val
);

static void enableBlockDataUpdateAndAutoInc(struct device* dev);

/**
 * TODO: Guter Stil wenn man alle funktionen im Header aufführt?
 */

////////////////////////////////////////////////////////////////////////////////
// Define structs

/*
 * This struct defines the datastructure to save the configuration
 * of the sensor.
 */
struct lsm9ds1_config {
    char *i2c_master_dev_name;
    u16_t i2c_slave_addr_acclgyro;
    u16_t i2c_slave_addr_magn;
};

/*
 * This struct defines the datastructure to save the sensor data.
 */
struct lsm9ds1_data {
    struct device *i2c_master;

    // Accelerometer values
    float accl_x;
    float accl_y;
    float accl_z;

    // Gyroscope values
    float gyro_x;
    float gyro_y;
    float gyro_z;

    // Magnetometer values
    float magn_x;
    float magn_y;
    float magn_z;

    // Temperature value
    float temp_c;

    // Accelerometer settings
    u8_t accl_scale;
    u8_t accl_outputDataRate;
    u8_t accl_bandwidth;

    // Gyroscope settings
    u8_t gyro_scale;
    u8_t gyro_outputDataRate;
    u8_t gyro_bandwidth;

    // Magnetometer settings
    u8_t magn_scale;
    u8_t magn_outputDataRate;
    u8_t magn_mode;

    // Temperature sensor settings
    u8_t temp_oversampleRate;


    // Calculated resolutions
    float acclRes;
    float gyroRes;
    float magnRes;

    // Calculated biases
    float acclBias[3];
    float gyroBias[3];
    float magnBias[3];
};

////////////////////////////////////////////////////////////////////////////////
// Enums
enum sensor_attribute_extended {
    SENSOR_ATTR_SCALE   = SENSOR_ATTR_PRIV_START,
    SENSOR_ATTR_ODR     = SENSOR_ATTR_PRIV_START + 1,
    SENSOR_ATTR_BW      = SENSOR_ATTR_PRIV_START + 2,
    SENSOR_ATTR_MODE    = SENSOR_ATTR_PRIV_START + 3,
    SENSOR_ATTR_OSR     = SENSOR_ATTR_PRIV_START + 4,
};

enum sensor_channel_extended {
    /**
     * Custom sensor channels to configure sensors.
     */
    SENSOR_CHAN_CALIBRATE_ACCL = SENSOR_CHAN_PRIV_START,
    SENSOR_CHAN_CALIBRATE_GYRO = SENSOR_CHAN_PRIV_START + 1,
    SENSOR_CHAN_CALIBRATE_MAGN = SENSOR_CHAN_PRIV_START + 2,

    SENSOR_CHAN_APPLY_SETTINGS = SENSOR_CHAN_PRIV_START + 3,

    /**
     * Custom sensor channel to accomodate the naming scheme with 4 characters
     * per sensor. The included sensor channel for the accelerometer
     * is called 'SENSOR_CHAN_ACCEL_XYZ' but I use ACCL throughout the
     * whole driver code. Therefore I define this custom sensor channel.
     * Both (the default and my custom) sensor channels trigger the same code,
     * it doesn't matter which one is used.
     */
    SENSOR_CHAN_ACCL_XYZ       = SENSOR_CHAN_PRIV_START + 4,
};

///////////////////
// Accelerometer //
///////////////////
enum AcclScale {
    AFS_2G = 0,
    AFS_16G,
    AFS_4G,
    AFS_8G
};

enum AcclOutputDataRate {
    AODR_PowerDown = 0,
    AODR_10Hz,
    AODR_50Hz,
    AODR_119Hz,
    AODR_238Hz,
    AODR_476Hz,
    AODR_952Hz
};

enum AcclBandwidth {
    ABW_408Hz = 0,
    ABW_211Hz,
    ABW_105Hz,
    ABW_50Hz
};

///////////////////
// Gyroscope     //
///////////////////
enum GyroScale {
    GFS_245DPS = 0,
    GFS_500DPS,
    GFS_NoOp,
    GFS_2000DPS
};

enum GyroOutputDataRate {
    GODR_PowerDown = 0,
    GODR_14_9Hz,
    GODR_59_5Hz,
    GODR_119Hz,
    GODR_238Hz,
    GODR_476Hz,
    GODR_952Hz
};

enum GyroBandwidth {
    GBW_low = 0,  // 14 Hz at Godr = 238 Hz,  33 Hz at Godr = 952 Hz
    GBW_med,      // 29 Hz at Godr = 238 Hz,  40 Hz at Godr = 952 Hz
    GBW_high,     // 63 Hz at Godr = 238 Hz,  58 Hz at Godr = 952 Hz
    GBW_highest   // 78 Hz at Godr = 238 Hz, 100 Hz at Godr = 952 Hz
};

///////////////////
// Magnetometer  //
///////////////////
enum MagnScale {
    MFS_4G = 0,
    MFS_8G,
    MFS_12G,
    MFS_16G
};

enum MagnOutputDataRate {
    MODR_0_625Hz = 0,
    MODR_1_25Hz,
    MODR_2_5Hz,
    MODR_5Hz,
    MODR_10Hz,
    MODR_20Hz,
    MODR_80Hz
};

enum MagnMode {
    MMode_LowPower = 0,
    MMode_MedPerformance,
    MMode_HighPerformance,
    MMode_UltraHighPerformance
};

/////////////////////////
// Temperature Sensor  //
/////////////////////////
enum TempConversionRate {
    ADC_256  = 0x00,
    ADC_512  = 0x02,
    ADC_1024 = 0x04,
    ADC_2048 = 0x06,
    ADC_4096 = 0x08,
    ADC_D1   = 0x40,
    ADC_D2   = 0x50,
};

////////////////////////////////////////////////////////////////////////////////
#endif /* ZEPHYR_DRIVERS_SENSOR_LSM9DS1_LSM9DS1_H_ */
////////////////////////////////////////////////////////////////////////////////