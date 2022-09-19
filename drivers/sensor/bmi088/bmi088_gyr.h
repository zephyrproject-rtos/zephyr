/* Bosch BMI088 inertial measurement unit header
 * Note: This is for the Gyro part only
 *
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_BMI088_GYR_H_
#define ZEPHYR_DRIVERS_SENSOR_BMI088_GYR_H_

#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/sys/util.h>

// gyro register

// read-only
#define BMI088_GYR_REG_CHIPID    0x00
#define GYR_RATE_X_LSB      0x02

// write-only
#define BMI088_GYR_SOFTRESET  0x14

// read/write
#define GYRO_RANGE      0x0F
#define GYRO_BANDWIDTH  0x10

//other defines
#define BMI088_GYR_REG_READ BIT(7)  // Indicates a read operation; bit 7 is clear on write s
#define BMI088_GYR_REG_MASK 0x7f // Mask lower 7 bits for register addresses

#define BMI088_GYR_CHIP_ID 0x0F  // Reset value of BMI088_REG_CHIPID

#define BMI088_GYR_SR_VAL   0xB6    // Value for triggering a Soft-Reset

#define BMI088_GYR_DEFAULT_RANGE    0x00    // Largest possible range for gyro (2000dps)
#define BMI088_GYR_DEFAULT_BW       0x04    // ODR: 200Hz, Filter bw: 23Hz

#define BMI088_AXES 3   // Number of Axes

#define BMI088_SAMPLE_SIZE  (BMI088_AXES * sizeof(uint16_t))    // Size of Samples with 2 bytes per axis = 6 bytes

// end of default settings
struct bmi088_gyr_cfg {
    struct spi_dt_spec bus;
    int bandwidth;
};


// Each sample has X, Y and Z, each with lsb and msb Bytes
struct bmi088_gyr_sample {
    uint16_t gyr[BMI088_AXES];
};

struct bmi088_gyr_data {
    const struct device *bus;
    struct bmi088_gyr_sample sample;
};

static inline struct bmi088_gyr_data *to_data(const struct device *dev) {
    return dev->data;
}

static inline const struct bmi088_gyr_cfg *to_config(const struct device *dev) {
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
int bmi088_gyr_read(const struct device *dev, uint8_t reg_addr, void *data, uint8_t len);

int bmi088_gyr_byte_read(const struct device *dev, uint8_t reg_addr, uint8_t *byte);

/**
 * Write multiple bytes to the BMI088
 *
 * @param dev Sensor device pointer
 * @param reg_addr Register address
 * @param [out] buf Data to write to the BMI088
 * @param len Number of bytes to write
 * @return 0 on success
 */
int bmi088_gyr_write(const struct device *dev, uint8_t reg_addr, void *buf, uint8_t len);

int bmi088_gyr_byte_write(const struct device *dev, uint8_t reg_addr, uint8_t byte);

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


struct sensor_value bmi088_gyr_to_fixed_point(int16_t raw_val, uint16_t scale);


struct sensor_value bmi088_gyr_channel_convert(enum sensor_channel chan, uint16_t scale, int16_t raw_xyz[3]);

#endif /* ZEPHYR_DRIVERS_SENSOR_BMI088_GYR_H_ */
