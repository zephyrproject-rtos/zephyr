/* Bosch BMI088 inertial measurement unit header
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_BMI088_BMI088_H_
#define ZEPHYR_DRIVERS_SENSOR_BMI088_BMI088_H_

#include <drivers/sensor.h>
#include <drivers/spi.h>
#include <sys/util.h>

// gyro register

// read-only
#define BMI088_REG_CHIPID    0x00
#define RATE_X_LSB      0x02
#define RATE_X_MSB      0x03
#define RATE_Y_LSB      0x04
#define RATE_Y_MSB      0x05
#define RATE_Z_LSB      0x06
#define RATE_Z_MSB      0x07
#define INT_STAT_1      0x0A
#define FIFO_STATUS     0x0E
#define FIFO_DATA       0x3F

// write-only
#define BMI088_SOFTRESET  0x14

// read/write
#define GYRO_RANGE      0x0F
#define GYRO_BANDWIDTH  0x10
#define GYRO_LPM1       0x11
#define GYRO_INT_CTRL   0x15
#define IO_CONF         0x16
#define IO_MAP          0x18
#define FIFO_WM_EN      0x1E
#define FIFO_EXT_INT_S  0x34
#define G_FIFO_CONF_0   0x3D
#define G_FIFO_CONF_1   0x3E
#define GYRO_SELFTEST   0x3C

// bit-fields

// GYRO_INT_STAT_1
#define GYRO_FIFO_INT   BIT(4)
#define GYRO_DRDY       BIT(7)
// FIFO_STATUS
#define FIFO_FRAMECOUNT
#define FIFO_OVERRUN    BIT(7)
// GYRO_INT_CTRL
#define GYRO_FIFO_EN    BIT(6)
#define GYRO_DATA_EN    BIT(7)

//other defines

// Indicates a read operation; bit 7 is clear on write s
#define BMI088_REG_READ BIT(7)
#define BMI088_REG_MASK 0x7f // Mask lower 7 bits for register addresses

#define BMI088_CHIP_ID 0x0F  // Reset value of BMI088_REG_CHIPID

#define BMI088_SR_VAL   0xB6    // Value for triggering a Soft-Reset

#define BMI088_DEFAULT_RANGE    0x00    // Largest possible range for gyro (2000dps)
#define BMI088_DEFAULT_BW       0x04    // ODR: 200Hz, Filter bw: 23Hz

#define FULL_MASK   0xFF    // Mask with only Ones

#define BMI088_AXES 3   // Number of Axes

#define BMI088_SAMPLE_SIZE  (BMI088_AXES * sizeof(uint16_t))    // Size of Samples with x,y,z

// end of default settings

struct bmi088_cfg {
    struct spi_dt_spec bus;
};


// Each sample has X, Y and Z
struct bmi088_gyro_sample {
    int16_t gyr[BMI088_AXES];
};

struct bmi088_data {
    const struct device *bus;
    struct bmi088_gyro_sample sample;
};

static inline struct bmi088_data *to_data(const struct device *dev) {
    return dev->data;
}

static inline const struct bmi088_cfg *to_config(const struct device *dev) {
    return dev->config;
}

/**
 * Read multiple bytes from the BMI088
 *
 * @param dev Sensor device pointer
 * @param reg_addr Register address
 * @param [out] data Destination address
 * @param len Number of bytes to read
 * @return 0 on success
 */
int bmi088_read(const struct device *dev, uint8_t reg_addr, void *data, uint8_t len);

int bmi088_byte_read(const struct device *dev, uint8_t reg_addr, uint8_t *byte);

int bmi088_word_read(const struct device *dev, uint8_t reg_addr, uint16_t *word);

/**
 * Write multiple bytes to the BMI088
 *
 * @param dev Sensor device pointer
 * @param reg_addr Register address
 * @param [out] buf Data to write to the BMI088
 * @param len Number of bytes to write
 * @return 0 on success
 */
int bmi088_write(const struct device *dev, uint8_t reg_addr, void *buf, uint8_t len);

int bmi088_byte_write(const struct device *dev, uint8_t reg_addr, uint8_t byte);

int bmi088_word_write(const struct device *dev, uint8_t reg_addr, uint16_t word);

/**
 * Update some bits in a BMI088 register without changing the other bits.
 * Pos and Mask are rather redundant, but make calculation easier.
 * It does not make sense to have a mask that is not a contiguous sequence of ones, offset by pos.
 *
 * @param dev Sensor device pointer
 * @param reg_addr Register address
 * @param pos Offset from the least-significant bit for the value
 * @param mask Mask that specifies which bits are allowed to change
 * @param val Value to set the bits to
 * @return
 */
int bmi088_reg_field_update(const struct device *dev, uint8_t reg_addr, uint8_t pos, uint8_t mask, uint8_t val);

uint16_t bmi088_gyr_scale(int32_t range_dps);

#endif /* ZEPHYR_DRIVERS_SENSOR_BMI088_BMI088_H_ */
