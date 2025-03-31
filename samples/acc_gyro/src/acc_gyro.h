
// Gyroscope Self-Test Registers
#define GYRO_SELF_TEST_X 0x00
#define GYRO_SELF_TEST_Y 0x01
#define GYRO_SELF_TEST_Z 0x02

// Accelerometer Self-Test Registers
#define ACCEL_SELF_TEST_X 0x0D
#define ACCEL_SELF_TEST_Y 0x0E
#define ACCEL_SELF_TEST_Z 0x0F

// Gyro Offset Registers
#define XG_OFFSET_H 0x13
#define XG_OFFSET_L 0x14
#define YG_OFFSET_H 0x15
#define YG_OFFSET_L 0x16
#define ZG_OFFSET_H 0x17
#define ZG_OFFSET_L 0x18

// Accelerometer Offset Registers
#define XA_OFFSET_H 0x77
#define XA_OFFSET_L 0x78
#define YA_OFFSET_H 0x7A
#define YA_OFFSET_L 0x7B
#define ZA_OFFSET_H 0x7D
#define ZA_OFFSET_L 0x7E

// Sample Rate Divider
#define INT_SAMP_RATE 0x19 

// Configuration
#define CONFIG 0x1A
#define GYRO_CONFIG 0x1B
#define ACCEL_CONFIG 0x1C
#define ACCEL_CONFIG_2 0x1D

#define LP_ACCEL_ODR 0x1E // Low Power Accelerometer ODR Control
#define WOM_THRESHOLD 0x1F // Wake-on Motion Threshold

#define I2C_MST_CTRL 0x24 // I2C Master ControL
#define I2C_MST_DELAY_CTRL 0x67 

// I2C Slave 0 Control
#define I2C_SLV0_ADDR 0x25
#define I2C_SLV0_REG 0x26
#define I2C_SLV0_CTRL 0x27
#define I2C_SLV0_DO 0x63

// I2C Slave 1 Control
#define I2C_SLV1_ADDR 0x28
#define I2C_SLV1_REG 0x29
#define I2C_SLV1_CTRL 0x2A
#define I2C_SLV1_DO 0x64

// I2C Slave 2 Control
#define I2C_SLV2_ADDR 0x2B
#define I2C_SLV2_REG 0x2C
#define I2C_SLV2_CTRL 0x2D
#define I2C_SLV2_DO 0x65

// I2C Slave 3 Control
#define I2C_SLV3_ADDR 0x2E
#define I2C_SLV3_REG 0x2F
#define I2C_SLV3_CTRL 0x30
#define I2C_SLV3_DO 0x66

// I2C Slave 4 Control
#define I2C_SLV4_ADDR 0x31
#define I2C_SLV4_REG 0x32
#define I2C_SLV4_D0 0x33
#define I2C_SLV4_CTRL 0x34
#define I2C_SLV4_DI 0x35

#define I2C_MST_STATUS 0x36 // I2C Master Status
#define INT_PIN_CFG 0x37
#define INT_ENABLE 0x38
#define INT_STATUS 0x3A

// Accelerometer Measurements
#define ACCEL_XOUT_H 0x3B
#define ACCEL_XOUT_L 0x3C
#define ACCEL_YOUT_H 0x3D
#define ACCEL_YOUT_L 0x3E
#define ACCEL_ZOUT_H 0x3F
#define ACCEL_ZOUT_L 0x40

// Temperature Measurements
#define TEMP_OUT_H 0x41
#define TEMP_OUT_L 0x42

// Gyroscope Measurements
#define GYRO_XOUT_H 0x43
#define GYRO_XOUT_L 0x44
#define GYRO_YOUT_H 0x45
#define GYRO_YOUT_L 0x46
#define GYRO_ZOUT_H 0x47
#define GYRO_ZOUT_H 0x48

// External Sensor Data
#define EXT_SENS_DATA_00 0x49
#define EXT_SENS_DATA_01 0x4A
#define EXT_SENS_DATA_02 0x4B
#define EXT_SENS_DATA_03 0x4C
#define EXT_SENS_DATA_04 0x4D
#define EXT_SENS_DATA_05 0x4E
#define EXT_SENS_DATA_06 0x4F
#define EXT_SENS_DATA_07 0x50
#define EXT_SENS_DATA_08 0x51
#define EXT_SENS_DATA_09 0x52
#define EXT_SENS_DATA_10 0x53
#define EXT_SENS_DATA_11 0x54
#define EXT_SENS_DATA_12 0x55
#define EXT_SENS_DATA_13 0x56
#define EXT_SENS_DATA_14 0x57
#define EXT_SENS_DATA_15 0x58
#define EXT_SENS_DATA_16 0x59
#define EXT_SENS_DATA_17 0x5A
#define EXT_SENS_DATA_18 0x5B
#define EXT_SENS_DATA_19 0x5C
#define EXT_SENS_DATA_20 0x5D
#define EXT_SENS_DATA_21 0x5E
#define EXT_SENS_DATA_22 0x5F
#define EXT_SENS_DATA_23 0x60

#define SIGNAL_PATH_RESET 0x68
#define MOT_DETECT_CTRL 0x69
#define USER_CTRL 0x6A

// Power Management 
#define PWR_MGMT_1 0x6B
#define PWR_MGMT_2 0x6C

#define FIFO_EN 0x23
#define FIFO_COUNT_H 0x72
#define FIFO_COUNT_L 0x73
#define FIFO_R_W 0x74

#define WHO_AM_I 0x75

// Magnetometer Registers
#define MAGNETO_WIA 0x00
#define MAGNETO_INFO 0x01
#define MAGNETO_ST1 0x02

// Measurement data of magnetic sensor X-axis/Y-axis/Z-axis
#define MAGNETO_HXL 0x03
#define MAGNETO_HXH 0x04
#define MAGNETO_HYL 0x05
#define MAGNETO_HYH 0x06
#define MAGNETO_HZL 0x07
#define MAGNETO_HZH 0x08

#define MAGNETO_ST2 0x09
#define MAGNETO_CNTL1 0x0A
#define MAGNETO_CNTL2 0x0B 
#define MAGNETO_ASTC 0x0C // Self-Test Control
#define MAGNETO_TS1 0x0D
#define MAGNETO_TS2 0x0E
#define MAGNETO_I2CDIS 0x0F // I2C Disable

// Sensitivity Adjustment values
#define MAGNETO_ASAX 0x10
#define MAGNETO_ASAY 0x11
#define MAGNETO_ASAZ 0x12
#define MAGNETO_RSV 0x13

// CNTL1 Mode select
#define MAGNETO_MODE_DOWN 0x00 // Power down mode
#define MAGNETO_MODE_ONE 0x01 // One shot data output
#define MAGNETO_MODE_C8HZ 0x02 // Continous data output 8Hz
#define MAGNETO_MODE_C100HZ 0x06 // Continous data output 100Hz
#define MAGNETO_MODE_TRIG 0x04 // External trigger data output
#define MAGNETO_MODE_TEST 0x08 // Self test
#define MAGNETO_MODE_FUSE 0x0F // Fuse ROM access

// Magneto Scale Select
#define MAGNETO_BIT_14 0x00 // 14bit output
#define MAGNETO_BIT_16 0x01 // 16bit output

// Gyro Full Scale Select
#define GYRO_FS_250  0x0 // +250dps
#define GYRO_FS_500  0x1 // +500dps
#define GYRO_FS_1000 0x2 // +1000dps
#define GYRO_FS_2000 0x3 // +2000dps

// Accel Full Scale Select
#define ACCEL_FS_2G  0x0 // 2g
#define ACCEL_FS_4G  0x1 // 4g
#define ACCEL_FS_8G  0x2 // 8g
#define ACCEL_FS_16G 0x3 // 16g

/**
 * @fn mpu9250_read(const struct device *dev, uint8_t slave_addr, uint8_t reg_addr, uint8_t num, uint8_t* buffer)
 * @brief The function reads data from MPU9250 sensor
 * @details The function `mpu9250_read` reads data from an MPU9250 sensor using I2C communication.
 * @param i2c_num Specify the I2C interface number used by MPU9250 sensor
 * @param slave_addr address of the slave device on the I2C bus 
 * @param reg_addr register address from which data needs to be read from the MPU9250 sensor 
 * @param num number of bytes to read from the MPU9250 sensor. 
 * @param buffer pointer to an array where the data read from the MPU9250 sensor will be stored
 */
void mpu9250_read(const struct device *dev, uint8_t slave_addr, uint8_t reg_addr, uint8_t num, uint8_t* buffer);

/**
 * @fn mpu9250_write(const struct device *dev, uint8_t slave_addr, uint8_t reg_addr, uint8_t data)
 * @brief The function `mpu9250_write` is used to write data
 * @details The function `mpu9250_write` writes data to an MPU9250 sensor using I2C communication 
 * @param i2c_num Specify the I2C interface number used by MPU9250 sensor
 * @param slave_addr address of the slave device on the I2C bus 
 * @param reg_addr register address from which data needs to be read from the MPU9250 sensor 
 * @param data data to write to a specific register in the MPU9250 sensor
 */
void mpu9250_write(const struct device *dev, uint8_t slave_addr, uint8_t reg_addr, uint8_t data);

/**
 * @fn accel_xyz(const struct device *dev, uint8_t slave_addr, float* ax, float* ay, float* az)
 * @brief The function `accel_xyz` reads raw accelerometer data
 * @details The function `accel_xyz` reads raw accelerometer data from an MPU9250 sensor 
 * and converts it to float values for the x, y, and z axes
 * @param i2c_num Specify the I2C interface number used by MPU9250 sensor
 * @param slave_addr address of the slave device on the I2C bus 
 * @param ax pointer to a float variable where the acceleration value along the X-axis 
 * will be stored after converting the raw accelerometer data.
 * @param ay pointer to a float variable where the acceleration value along the Y-axis 
 * will be stored after converting the raw accelerometer data.
 * @param az pointer to a float variable where the acceleration value along the Z-axis 
 * will be stored after converting the raw accelerometer data.
 */
void accel_xyz(const struct device *dev, uint8_t slave_addr, float* ax, float* ay, float* az);

/**
 * @fn gyro_xyz(const struct device *dev, uint8_t slave_addr, float* gx, float* gy, float* gz)
 * @brief The function `gyro_xyz` reads raw gyroscope data
 * @details The function `gyro_xyz` reads raw gyroscope data from an MPU9250 sensor 
 * and converts it to float values for the x, y, and z axes.
 * @param i2c_num Specify the I2C interface number used by MPU9250 sensor
 * @param slave_addr address of the slave device on the I2C bus 
 * @param gx pointer to a float variable where the gyroscope value along for the X-axis 
 * will be stored after converting the raw gyroscope data.
 * @param gy pointer to a float variable where the gyroscope value along for the Y-axis 
 * will be stored after converting the raw gyroscope data.
 * @param gz pointer to a float variable where the gyroscope value along for the Z-axis 
 * will be stored after converting the raw gyroscope data.
 */
void gyro_xyz(const struct device *dev, uint8_t slave_addr, float* gx, float* gy, float* gz);

/**
 * @fn magneto_xyz(const struct device *dev, uint8_t slave_addr, float* mx, float* my, float* mz)
 * @brief The function `magneto_xyz` reads raw magnetometer data
 * @details The function `magneto_xyz` reads raw magnetometer data from an AK8963 magnetometer, 
 * combines the data for each axis, and converts it to float values using resolution and coefficients.
 * @param i2c_num Specify the I2C interface number used by MPU9250 sensor
 * @param slave_addr address of the slave device on the I2C bus 
 * @param mx pointer to a float variable where the magnetometer value along for the X-axis 
 * will be stored after converting the raw magnetometer data
 * @param my pointer to a float variable where the magnetometer value along for the Y-axis 
 * will be stored after converting the raw magnetometer data
 * @param mz pointer to a float variable where the magnetometer value along for the Z-axis 
 * will be stored after converting the raw magnetometer data
 */
void magneto_xyz(const struct device *dev, uint8_t slave_addr, float* mx, float* my, float* mz);

/**
 * @fn mpu9250_config(const struct device *dev, uint8_t slave_addr, uint8_t gfs, uint8_t afs)
 * @brief The function `mpu9250_config` configures the MPU9250 sensor
 * @details The function `mpu9250_config` configures the MPU9250 sensor with specified gyro and accelerometer
 * full scales, and sets various register values for operation.
 * @param i2c_num Specify the I2C interface number used by MPU9250 sensor
 * @param slave_addr address of the slave device on the I2C bus 
 * @param gfs gfs(Gyroscope Full Scale) specify the full scale range for the gyroscope sensor
 * @param afs afs(accelerometer Full Scale) specify the full scale range for the accelerometer sensor
 */
void mpu9250_config(const struct device *dev, uint8_t slave_addr);

/**
 * @fn ak8963_config(const struct device *dev, uint8_t slave_addr, uint8_t mode, uint8_t mfs)
 * @brief The function `ak8963_config` configures the AK8963 sensor
 * @details The function `ak8963_config` configures the AK8963 sensor with specified Magneto Scale Select 
 * and sets various register values for operation.
 * @param i2c_num Specify the I2C interface number used by AK8963 sensor
 * @param slave_addr address of the slave device on the I2C bus 
 * @param mode specify the Magneto Mode Select for the AK8963 magnetometer
 * @param mfs mfs(Magneto Scale Select) represents the magnetometer scale factor and
 * determine the resolution of the magnetometer output
 */
void ak8963_config(const struct device *dev, uint8_t slave_addr);

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
void readStepDetection(float ax, float ay, float az);