/* Bosch BMI088 inertial measurement unit driver
 * Note: This is for the Gyro part only
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Datasheet:
 * https://www.bosch-sensortec.com/media/boschsensortec/downloads/datasheets/bst-bmi088-ds001.pdf
 */

#define DT_DRV_COMPAT bosch_bmi088

#include "bmi088.h"

#include <init.h>
#include <drivers/sensor.h>
#include <sys/byteorder.h>
#include <kernel.h>
#include <sys/__assert.h>
#include <devicetree.h>
#include <logging/log.h>

#include "../lib/libc/minimal/include/math.h"

LOG_MODULE_REGISTER(BMI088, CONFIG_SENSOR_LOG_LEVEL);

#if DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT) == 0
#error "BMI088 driver enabled without any devices"
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
static int bmi088_transceive(const struct device *dev, uint8_t reg, bool write, void *buf, size_t length) {
    const struct bmi088_cfg *cfg = to_config(dev);
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

bool bmi088_bus_ready_spi(const struct device *dev) {
    return spi_is_ready(&to_config(dev)->bus);
}

int bmi088_read(const struct device *dev, uint8_t reg_addr, void *buf, uint8_t len) {
    return bmi088_transceive(dev, reg_addr | BMI088_REG_READ, false, buf, len);
}

int bmi088_byte_read(const struct device *dev, uint8_t reg_addr, uint8_t *byte) {
    return bmi088_read(dev, reg_addr, byte, 1);
}

int bmi088_write(const struct device *dev, uint8_t reg_addr, void *buf, uint8_t len) {

    return bmi088_transceive(dev, reg_addr & BMI088_REG_MASK, true, buf, len);
}

int bmi088_byte_write(const struct device *dev, uint8_t reg_addr,
                      uint8_t byte) {
    return bmi088_write(dev, reg_addr & BMI088_REG_MASK, &byte, 1);
}

/**
 * Convert the raw value with a factor 'scale' and save the new values integer and fractional part in 'sensor_value'
 *
 * @param raw_val Raw sensor value
 * @param scale Value to scale the raw_val
 */
struct sensor_value bmi088_to_fixed_point(int16_t raw_val, uint16_t scale) {
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
struct sensor_value bmi088_channel_convert(enum sensor_channel chan, uint16_t scale, int16_t raw_xyz[3]) {
    switch (chan) {
        case SENSOR_CHAN_GYRO_X:
            return bmi088_to_fixed_point(raw_xyz[0], scale);
        case SENSOR_CHAN_GYRO_Y:
            return bmi088_to_fixed_point(raw_xyz[1], scale);
        case SENSOR_CHAN_GYRO_Z:
            return bmi088_to_fixed_point(raw_xyz[2], scale);
        default:
            LOG_ERR("Channels not supported !");
            struct sensor_value empty = {0};
            return empty;
    }
}

static int bmi088_attr_set(const struct device *dev, enum sensor_channel chan, enum sensor_attribute attr,
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
static int bmi088_sample_fetch(const struct device *dev, enum sensor_channel chan) {
    struct bmi088_data *data = to_data(dev);

    __ASSERT(chan == SENSOR_CHAN_ALL || chan == SENSOR_CHAN_GYRO_XYZ, "channel is not valid");

    if (bmi088_read(dev, RATE_X_LSB, data->sample.gyr, BMI088_SAMPLE_SIZE) < 0) {
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
static int bmi088_channel_get(const struct device *dev, enum sensor_channel chan, struct sensor_value *val) {
    const uint16_t scale = (uint16_t) (61 * 1000 * 2 * M_PI / 360);    // scale for converting sensor output to m°/s/lsb
    switch (chan) {
        case SENSOR_CHAN_GYRO_X:
        case SENSOR_CHAN_GYRO_Y:
        case SENSOR_CHAN_GYRO_Z: {
            struct bmi088_data *data = to_data(dev);
            *val = bmi088_channel_convert(chan, scale, data->sample.gyr);
            return 0;
        }
        case SENSOR_CHAN_GYRO_XYZ: {
            struct bmi088_data *data = to_data(dev);
            val[0] = bmi088_channel_convert(SENSOR_CHAN_GYRO_X, scale, data->sample.gyr);
            val[1] = bmi088_channel_convert(SENSOR_CHAN_GYRO_Y, scale, data->sample.gyr);
            val[2] = bmi088_channel_convert(SENSOR_CHAN_GYRO_Z, scale, data->sample.gyr);
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
static int bmi088_init(const struct device *dev) {
    LOG_DBG("Initializing BMI088 device at %p", dev);

    if (!bmi088_bus_ready_spi(dev)) {
        LOG_ERR("Bus not ready");
        return -EINVAL;
    }

    // reboot the chip: Softreset 0x14
    if (bmi088_byte_write(dev, BMI088_SOFTRESET, BMI088_SR_VAL) < 0) {
        LOG_DBG("Cannot reboot chip.");
        return -EIO;
    }

    k_busy_wait(30000);

    uint8_t val = 0U;
    if (bmi088_byte_read(dev, BMI088_REG_CHIPID, &val) < 0) {
        LOG_DBG("Failed to read chip id.");
        return -EIO;
    }

    if (val != BMI088_CHIP_ID) {
        LOG_DBG("Unsupported chip detected (0x%x)!", val);
        return -ENODEV;
    }
    LOG_DBG("Chip successfully detected");

    // Set default gyro range, For now: always use the largest range
    if (bmi088_byte_write(dev, GYRO_RANGE, BMI088_DEFAULT_RANGE) < 0) {
        LOG_DBG("Cannot set default range for gyroscope.");
        return -EIO;
    }


    // Set Bandwidth to 0x04 (ODR 200Hz, Filter bandwidth 23Hz)
    if (bmi088_byte_write(dev, GYRO_BANDWIDTH, BMI088_DEFAULT_BW) < 0) {
        LOG_DBG("Failed to set gyro's default ODR");
        return -EIO;
    }

    return 0;
}

static const struct sensor_driver_api bmi088_api = {
        .attr_set = bmi088_attr_set,
        .sample_fetch = bmi088_sample_fetch,
        .channel_get = bmi088_channel_get,
};


#define BMI088_DEVICE_INIT(inst) \
    static struct bmi088_data bmi088_data_##inst;               \
    static const struct bmi088_cfg bmi088_cfg_##inst = {           \
        .bus = SPI_DT_SPEC_INST_GET(inst, SPI_WORD_SET(8), 0), \
    };                                   \
    DEVICE_DT_INST_DEFINE(inst, bmi088_init, NULL,            \
                  &bmi088_data_##inst, &bmi088_cfg_##inst,    \
                  POST_KERNEL, CONFIG_SENSOR_INIT_PRIORITY,    \
                  &bmi088_api);


DT_INST_FOREACH_STATUS_OKAY(BMI088_DEVICE_INIT)
