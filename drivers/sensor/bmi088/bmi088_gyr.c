/* Bosch BMI088 inertial measurement unit driver
 * Note: This is for the Gyro part only
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Datasheet:
 * https://www.bosch-sensortec.com/media/boschsensortec/downloads/datasheets/bst-bmi088-ds001.pdf
 */

#define DT_DRV_COMPAT bosch_bmi088_gyr

#include "bmi088_gyr.h"

#include <zephyr/init.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/devicetree.h>
#include <zephyr/logging/log.h>

#include "../lib/libc/minimal/include/math.h"

LOG_MODULE_REGISTER(BMI088_GYR, CONFIG_SENSOR_LOG_LEVEL);

#if DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT) == 0
#error "BMI088 gyroscope driver enabled without any devices"
#endif

/**
 * Send/receive data to/from the BMI088
 *
 * @param dev Sensor device pointer
 * @param reg (Start) register address, for writing the MSb should be 0, for reading 1
 * @param write True for write-only, False for read-only
 * @param buf Out-parameter if reading, In-parameter if writing
 * @param length Length of buf
 * @return 0 on success
 */
static int bmi088_gyr_transceive(const struct device *dev, uint8_t reg, bool write, void *buf, size_t length) {
    const struct bmi088_gyr_cfg *cfg = to_config(dev);
    const struct spi_buf tx_buf[2] = {
            {
                    // Always send register address first
                    .buf = &reg,
                    .len = 1
            },
            {
                    // Then send whatever data there is to send
                    .buf = buf,
                    .len = length
            }
    };
    const struct spi_buf_set tx = {
            .buffers = tx_buf,
            // If the buffer is a null pointer, only use the first TX (register address)
            .count = buf ? 2 : 1
    };


    if (write) {
        // Write-only: No RX buffer necessary
        return spi_write_dt(&cfg->bus, &tx);
    } else {
        // If we want to read, use the same buffers for that purpose.
        // No useful data will be written to reg, as we dont receive anything while we still send the address
        // Whatever data is in the buffer will be sent out. The sensor will ignore that data if the address was a
        // read-request
        const struct spi_buf_set rx = {
                .buffers = tx_buf,
                .count = 2
        };

        return spi_transceive_dt(&cfg->bus, &tx, &rx);
    }
}

bool bmi088_gyr_bus_ready_spi(const struct device *dev) {
    return spi_is_ready(&to_config(dev)->bus);
}

int bmi088_gyr_read(const struct device *dev, uint8_t reg_addr, void *buf, uint8_t len) {
    return bmi088_gyr_transceive(dev, reg_addr | BMI088_GYR_REG_READ, false, buf, len);
}

int bmi088_gyr_byte_read(const struct device *dev, uint8_t reg_addr, uint8_t *byte) {
    return bmi088_gyr_read(dev, reg_addr, byte, 1);
}

int bmi088_gyr_write(const struct device *dev, uint8_t reg_addr, void *buf, uint8_t len) {

    return bmi088_gyr_transceive(dev, reg_addr & BMI088_GYR_REG_MASK, true, buf, len);
}

int bmi088_gyr_byte_write(const struct device *dev, uint8_t reg_addr,
                      uint8_t byte) {
    return bmi088_gyr_write(dev, reg_addr & BMI088_GYR_REG_MASK, &byte, 1);
}

/**
 * Convert the raw value with a factor 'scale' and save the new values integer and fractional part in 'sensor_value'
 *
 * @param raw_val Raw sensor value
 * @param scale Value to scale the raw_val
 */
struct sensor_value bmi088_gyr_to_fixed_point(int16_t raw_val, uint16_t scale) {
    int32_t converted_val = raw_val * scale;

    LOG_INF("Conversion: input %d scale %d converted %ld", raw_val, scale, converted_val);
    struct sensor_value val = {
            .val1=converted_val / 1000000,
            .val2=converted_val % 1000000};
    return val;
}

/**
 * Convert the corresponding value of the channel (X, Y or Z)
 *
 * @param chan Channel to read. Implemented: Gyro X, Y, Z
 * @param scale Value to scale the raw_val
 * @param raw_xyz array for storing X, Y, Z channel raw value of the sensor
 * @return
 */
struct sensor_value bmi088_gyr_channel_convert(enum sensor_channel chan, uint16_t scale, int16_t raw_xyz[3]) {
    switch (chan) {
        case SENSOR_CHAN_GYRO_X:
            return bmi088_gyr_to_fixed_point(raw_xyz[0], scale);
        case SENSOR_CHAN_GYRO_Y:
            return bmi088_gyr_to_fixed_point(raw_xyz[1], scale);
        case SENSOR_CHAN_GYRO_Z:
            return bmi088_gyr_to_fixed_point(raw_xyz[2], scale);
        default:
            LOG_ERR("Channels not supported !");
            struct sensor_value empty = {0};
            return empty;
    }
}

static int bmi088_gyr_attr_set(const struct device *dev, enum sensor_channel chan, enum sensor_attribute attr,
                           const struct sensor_value *val) {
    return -ENOTSUP;
}

/**
 * API function to retrieve a measurement from the sensor.
 * It is assumed that the data is ready
 *
 * @param dev Sensor device pointer
 * @param chan Channel to fetch. Only SENSOR_CHAN_ALL and GYRO_XYZ is supported.
 * @return 0 on success
 */
static int bmi088_gyr_sample_fetch(const struct device *dev, enum sensor_channel chan) {
    struct bmi088_gyr_data *data = to_data(dev);

    __ASSERT(chan == SENSOR_CHAN_ALL || chan == SENSOR_CHAN_GYRO_XYZ, "channel is not valid");

    if (bmi088_gyr_read(dev, GYR_RATE_X_LSB, data->sample.gyr, BMI088_SAMPLE_SIZE) < 0) {
        return -EIO;
    }

    // convert samples to cpu endianness
    for (size_t i = 0; i < BMI088_AXES; i++) {
        uint16_t *sample = &data->sample.gyr[i];

        *sample = sys_le16_to_cpu(*sample);
    }

    LOG_INF("Fetched %d %d %d", data->sample.gyr[0], data->sample.gyr[1], data->sample.gyr[2]);

    return 0;
}

/**
 * API function to get a cached sensor value that was previously fetched from the sensor
 *
 * @param dev Sensor device pointer
 * @param chan Channel to read. Implemented: Gyro X, Y, Z
 * @param [out] val Sensor value
 * @return 0 on success, -ENOTSUP on unsupported channel
 */
static int bmi088_gyr_channel_get(const struct device *dev, enum sensor_channel chan, struct sensor_value *val) {
    const uint16_t scale = (uint16_t) (61 * 1000 * 2 * M_PI / 360);    // scale for converting sensor output to m°/s/lsb
    switch (chan) {
        case SENSOR_CHAN_GYRO_X:
        case SENSOR_CHAN_GYRO_Y:
        case SENSOR_CHAN_GYRO_Z: {
            struct bmi088_gyr_data *data = to_data(dev);
            *val = bmi088_gyr_channel_convert(chan, scale, data->sample.gyr);
            return 0;
        }
        case SENSOR_CHAN_GYRO_XYZ: {
            struct bmi088_gyr_data *data = to_data(dev);
            val[0] = bmi088_gyr_channel_convert(SENSOR_CHAN_GYRO_X, scale, data->sample.gyr);
            val[1] = bmi088_gyr_channel_convert(SENSOR_CHAN_GYRO_Y, scale, data->sample.gyr);
            val[2] = bmi088_gyr_channel_convert(SENSOR_CHAN_GYRO_Z, scale, data->sample.gyr);
            return 0;
        }
        default:
            LOG_ERR("Channel not supported.");
            return -ENOTSUP;
    }
}

/**
 * Sensor device initialization
 *
 * @param dev Sensor device pointer
 * @return 0 on success
 */
static int bmi088_gyr_init(const struct device *dev) {
    LOG_DBG("Initializing BMI088 GYRO device at %p", dev);

    if (!bmi088_gyr_bus_ready_spi(dev)) {
        LOG_ERR("Bus not ready");
        return -EINVAL;
    }

    // reboot the chip: Softreset 0x14
    if (bmi088_gyr_byte_write(dev, BMI088_GYR_SOFTRESET, BMI088_GYR_SR_VAL) < 0) {
        LOG_DBG("Cannot reboot chip.");
        return -EIO;
    }

    k_busy_wait(30000);

    uint8_t val = 0U;
    if (bmi088_gyr_byte_read(dev, BMI088_GYR_REG_CHIPID, &val) < 0) {
        LOG_DBG("Failed to read chip id.");
        return -EIO;
    }

    if (val != BMI088_GYR_CHIP_ID) {
        LOG_DBG("Unsupported chip detected (0x%x)!", val);
        return -ENODEV;
    }
    LOG_DBG("Chip successfully detected");

    // Set default gyro range, For now: always use the largest range
    if (bmi088_gyr_byte_write(dev, GYRO_RANGE, BMI088_GYR_DEFAULT_RANGE) < 0) {
        LOG_DBG("Cannot set default range for gyroscope.");
        return -EIO;
    }


    // Set Bandwidth (default is 0x04, resulting in ODR 200Hz, Filter bandwidth 23Hz)
    int bw = to_config(dev)->bandwidth;
    if (bw < 0x00 || bw > 0x07) {
        bw = BMI088_GYR_DEFAULT_BW;
        LOG_WRN("BMI088 gyro: specified bandwidth is out of range, using default value instead");
    }
    if (bmi088_gyr_byte_write(dev, GYRO_BANDWIDTH, bw) < 0) {
        LOG_DBG("Failed to set gyro's ODR to %d", bw);
        return -EIO;
    }

    return 0;
}

static const struct sensor_driver_api bmi088_gyr_api = {
        .attr_set = bmi088_gyr_attr_set,
        .sample_fetch = bmi088_gyr_sample_fetch,
        .channel_get = bmi088_gyr_channel_get,
};


#define BMI088_GYR_DEVICE_INIT(inst) \
    static struct bmi088_gyr_data bmi088_gyr_data_##inst;               \
    static const struct bmi088_gyr_cfg bmi088_gyr_cfg_##inst = {           \
        .bus = SPI_DT_SPEC_INST_GET(inst, SPI_WORD_SET(8), 0),          \
        .bandwidth = DT_INST_PROP(inst, bandwidth)                \
    };                                   \
    DEVICE_DT_INST_DEFINE(inst, bmi088_gyr_init, NULL,            \
                  &bmi088_gyr_data_##inst, &bmi088_gyr_cfg_##inst,    \
                  POST_KERNEL, CONFIG_SENSOR_INIT_PRIORITY,    \
                  &bmi088_gyr_api);


DT_INST_FOREACH_STATUS_OKAY(BMI088_GYR_DEVICE_INIT)
