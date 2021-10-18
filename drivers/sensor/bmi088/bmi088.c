/* Bosch BMI088 inertial measurement unit driver
 *
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Datasheet:
 * https://www.bosch-sensortec.com/media/boschsensortec/downloads/datasheets/bst-bmi088-ds001.pdf
 */

#define DT_DRV_COMPAT bosch_bmi088

#include <init.h>
#include <drivers/sensor.h>
#include <sys/byteorder.h>
#include <kernel.h>
#include <sys/__assert.h>
#include <logging/log.h>
#include <devicetree.h>

#include "bmi088.h"

#define M_PI   3.14159265358979323846264338327950288

LOG_MODULE_REGISTER(BMI088, LOG_LEVEL_DBG);

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

int bmi088_word_read(const struct device *dev, uint8_t reg_addr, uint16_t *word) {
    int rc = bmi088_read(dev, reg_addr, word, 2);
    if (rc != 0) {
        return rc;
    }

    *word = sys_le16_to_cpu(*word);

    return 0;
}

int bmi088_write(const struct device *dev, uint8_t reg_addr, void *buf, uint8_t len) {

    return bmi088_transceive(dev, reg_addr & BMI088_REG_MASK, true, buf, len);
}

int bmi088_byte_write(const struct device *dev, uint8_t reg_addr,
                      uint8_t byte) {
    return bmi088_write(dev, reg_addr & BMI088_REG_MASK, &byte, 1);
}

int bmi088_word_write(const struct device *dev, uint8_t reg_addr, uint16_t word) {
    uint8_t tx_word[2] = {
            (uint8_t) (word & 0xff),
            (uint8_t) (word >> 8)
    };

    return bmi088_write(dev, reg_addr & BMI088_REG_MASK, tx_word, 2);
}

int bmi088_reg_field_update(const struct device *dev, uint8_t reg_addr, uint8_t pos, uint8_t mask, uint8_t val) {
    uint8_t old_val;

    if (bmi088_byte_read(dev, reg_addr, &old_val) < 0) {
        return -EIO;
    }

    return bmi088_byte_write(dev, reg_addr, (old_val & ~mask) | ((val << pos) & mask));
}

static void bmi088_to_fixed_point(int16_t raw_val, uint16_t scale, struct sensor_value *val) {
    int32_t converted_val = raw_val * scale;

    LOG_INF("Conversion: input %d scale %d converted %ld", raw_val, scale, converted_val);

    val->val1 = converted_val / 1000000;
    val->val2 = converted_val % 1000000;
}

static void
bmi088_channel_convert(enum sensor_channel chan, uint16_t scale, uint16_t *raw_xyz, struct sensor_value *val) {
    switch (chan) {
        case SENSOR_CHAN_GYRO_X:
            bmi088_to_fixed_point(raw_xyz[0], scale, val);
            break;
        case SENSOR_CHAN_GYRO_Y:
            bmi088_to_fixed_point(raw_xyz[1], scale, val);
            break;
        case SENSOR_CHAN_GYRO_Z:
            bmi088_to_fixed_point(raw_xyz[2], scale, val);
            break;
        default:
            break;
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
 * @param chan Channel to fetch. Only SENSOR_CHAN_ALL is supported.
 * @return 0 on success
 */

static int bmi088_sample_fetch(const struct device *dev, enum sensor_channel chan) {
    struct bmi088_data *data = to_data(dev);
    uint8_t status;
    size_t i;

    __ASSERT_NO_MSG(chan == SENSOR_CHAN_ALL);

    /*
    status = 0;
    while ((status & BMI088_DATA_READY_BIT_MASK) == 0) {

        if (bmi088_byte_read(dev, BMI088_REG_STATUS, &status) < 0) {
            return -EIO;
        }
    }
     */

    if (bmi088_read(dev, RATE_X_LSB, data->sample.gyr,
                    BMI088_SAMPLE_SIZE) < 0) {
        return -EIO;
    }

    // convert samples to cpu endianness
    for (i = 0; i < BMI088_SAMPLE_SIZE; i += 2) {
        uint16_t *sample =
                (uint16_t *) &data->sample.gyr[i];

        *sample = sys_le16_to_cpu(*sample);
    }

    LOG_INF("Fetched %d %d %d", data->sample.gyr[0], data->sample.gyr[1], data->sample.gyr[2]);

    return 0;
}

/**
 * API function to get a cached sensor value that was previously fetched from the sensor
 *
 * @param dev Sensor device pointer
 * @param chan Channel to read. Implemented: Gyro X, Y, Z or all three
 * @param [out] val Sensor value
 * @return 0 on success, -ENOTSUP on unsupported channel
 */
static int bmi088_channel_get(const struct device *dev, enum sensor_channel chan, struct sensor_value *val) {
    const uint16_t scale = (61 * 1000 * 2 * M_PI / 360);
    switch (chan) {
        case SENSOR_CHAN_GYRO_X:
        case SENSOR_CHAN_GYRO_Y:
        case SENSOR_CHAN_GYRO_Z: {
            struct bmi088_data *data = to_data(dev);
            bmi088_channel_convert(chan, scale, data->sample.gyr, val);
            return 0;
        }

        default:
            LOG_DBG("Channel not supported.");
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

    const struct bmi088_cfg *cfg = to_config(dev);
    struct bmi088_data *data = to_data(dev);

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

    // Set default gyro range, For now: always use largest range
    if (bmi088_byte_write(dev, GYRO_RANGE, BMI088_DEFAULT_RANGE) < 0) {
        LOG_DBG("Cannot set default range for gyroscope.");
        return -EIO;
    }


    // Set Bandwidth to 0x04: ODR 200Hz, Filter bandwidth 23Hz
    if (bmi088_reg_field_update(dev, GYRO_BANDWIDTH,
                                0,
                                FULL_MASK,
                                BMI088_DEFAULT_BW) < 0) {
        LOG_DBG("Failed to set gyro's default ODR.");
        return -EIO;
    }


    return 0;
}

uint16_t bmi088_gyr_scale(int32_t range_dps) {
    return (2 * range_dps * SENSOR_PI) / 180LL / 65536LL;
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
