/*
 * Copyright (c) 2018, Sensirion AG
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * * Redistributions of source code must retain the above copyright notice, this
 *   list of conditions and the following disclaimer.
 *
 * * Redistributions in binary form must reproduce the above copyright notice,
 *   this list of conditions and the following disclaimer in the documentation
 *   and/or other materials provided with the distribution.
 *
 * * Neither the name of Sensirion AG nor the names of its
 *   contributors may be used to endorse or promote products derived from
 *   this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#define DT_DRV_COMPAT sensirion_sgp30

#include <device.h>
#include <drivers/i2c.h>
#include <kernel.h>
#include <drivers/sensor.h>
#include <drivers/sensor/sgp30.h>
#include <sys/__assert.h>
#include <sys/byteorder.h>
#include <logging/log.h>

LOG_MODULE_REGISTER(sensor, CONFIG_SENSOR_LOG_LEVEL);


#include "sgp30.h"

static inline const struct device *sgp30_i2c_device(const struct device *dev)
{
    const struct sgp30_data *ddp = dev->data;

    return ddp->bus;
}

static inline uint8_t sgp30_i2c_address(const struct device *dev)
{
    const struct sgp30_config *dcp = dev->config;

    return dcp->base_address;
}

uint16_t sgp30_bytes_to_uint16_t(uint8_t *data)
{
    return sys_get_be16(data);
}

static uint8_t sgp30_compute_crc(uint8_t *data, uint8_t data_len)
{
	uint16_t current_byte;
    uint8_t crc = SGP30_CRC8_INIT;
    uint8_t crc_bit;

    /* calculates 8-Bit checksum with given polynomial */
    for (current_byte = 0; current_byte < data_len; ++current_byte) {
        crc ^= (data[current_byte]);
        for (crc_bit = 8; crc_bit > 0; --crc_bit) {
            if (crc & 0x80)
                crc = (crc << 1) ^ SGP30_CRC8_POLYNOMIAL;
            else
                crc = (crc << 1);
        }
    }
    return crc;
}

int sgp30_check_crc(uint8_t *data, uint8_t data_len, uint8_t checksum)
{
	uint8_t actual_crc = sgp30_compute_crc(data, data_len);
	
	if (checksum != actual_crc)
	{
		LOG_DBG("CRC check failed");
		return -EIO;
	}

	return 0;
}

uint16_t sgp30_fill_cmd_send_buff(uint8_t *buf, 
                                  uint16_t cmd, 
                                  uint16_t *args, 
                                  uint16_t arg_size)
{
    uint8_t crc;
    uint8_t i;
    uint16_t idx = 0;

    sys_put_be16(cmd, buf);
    idx += 2;

    for (i = 0; i < arg_size; ++i) {
        sys_put_be16(args[i], buf + idx);
        idx += 2;

        crc = sgp30_compute_crc((uint8_t*)&buf[idx - 2],
                                            SGP30_WORD_SIZE);
        buf[idx++] = crc;
    }
    return idx;
}

int sgp30_write_cmd(const struct device *dev,
                    uint16_t cmd)
{
    uint8_t tx_buf[SGP30_WORD_SIZE + 1] = {cmd >> 8, cmd & 0xFF};

    sgp30_fill_cmd_send_buff(tx_buf, cmd, NULL, 0);

    return i2c_write(
        sgp30_i2c_device(dev),
        tx_buf, sizeof(tx_buf),
        sgp30_i2c_address(dev));
}

int sgp30_write_cmd_args(const struct device *dev,
                         uint16_t cmd,
                         uint16_t *args,
                         uint16_t arg_size)
{
    uint8_t tx_buf[30];

    uint16_t data_bytes = sgp30_fill_cmd_send_buff(tx_buf, cmd, args, arg_size);

    return i2c_write(
        sgp30_i2c_device(dev),
        tx_buf, data_bytes,
        sgp30_i2c_address(dev));
}

int sgp30_read_words(const struct device *dev,
                     uint16_t *data_words,
                     uint16_t num_words)
{
    /* For each word read a CRC byte too. */
    uint16_t bytes_to_read = (num_words * (SGP30_WORD_SIZE + 1));
    uint8_t rx_buf[bytes_to_read];

    int rc = i2c_read(
        sgp30_i2c_device(dev),
        rx_buf, bytes_to_read,
        sgp30_i2c_address(dev));

    if (rc != 0)
    {
        return -EIO;
    }

    for (int i = 0; i < num_words; i++)
    {
        int rc = sgp30_check_crc(&rx_buf[3 * i], SGP30_WORD_SIZE, rx_buf[(3 * i) + 2]);
        if (rc != 0)
        {
            return -1;
        }
        data_words[i] = sgp30_bytes_to_uint16_t(rx_buf + 3 * i);
    }

    return 0;
}

int16_t sgp30_delayed_read_cmd(const struct device *dev, uint16_t cmd,
                               uint32_t delay_us, uint16_t *data_words,
                               uint16_t num_words)
{
    int rc;
    rc = sgp30_write_cmd(dev, cmd);

    if (rc != 0)
    {
        return rc;
    }

    k_usleep(delay_us);

    return sgp30_read_words(dev, data_words, num_words);
}

/**
 * sgp30_check_featureset() - Check if the connected sensor has a certain FS
 *
 * @needed_fs: The featureset that is required
 *
 * Return: 0 if the sensor has the required FS,
 *         SGP30_ERR_INVALID_PRODUCT_TYPE if the sensor is not an SGP30,
 *         SGP30_ERR_UNSUPPORTED_FEATURE_SET if the sensor does not
 *                                           have the required FS,
 *         an error code otherwise
 */
static int16_t sgp30_check_featureset(const struct device *dev,
                                      uint16_t needed_fs)
{
    int16_t ret;
    uint16_t fs_version;
    uint8_t product_type;

    ret = sgp30_get_feature_set_version(dev, &fs_version, &product_type);
    if (ret != 0)
        return ret;

    if (product_type != SGP30_PRODUCT_TYPE)
        return SGP30_ERR_INVALID_PRODUCT_TYPE;

    if (fs_version < needed_fs)
        return SGP30_ERR_UNSUPPORTED_FEATURE_SET;

    return 0;
}

int16_t sgp30_measure_test(const struct device *dev,
                           uint16_t *test_result)
{
    uint16_t measure_test_word_buf[SGP30_CMD_MEASURE_TEST_WORDS];
    int16_t ret;

    *test_result = 0;

    ret = sgp30_delayed_read_cmd(
        dev, SGP30_CMD_MEASURE_TEST,
        SGP30_CMD_MEASURE_TEST_DURATION_US, measure_test_word_buf,
        SGP30_NUM_WORDS(measure_test_word_buf));
    if (ret != 0)
        return ret;

    *test_result = *measure_test_word_buf;
    if (*test_result == SGP30_CMD_MEASURE_TEST_OK)
        return 0;

    return -ENOTSUP;
}

int16_t sgp30_measure_iaq(const struct device *dev)
{
    return sgp30_write_cmd(dev, SGP30_CMD_IAQ_MEASURE);
}

int16_t sgp30_read_iaq(const struct device *dev,
                       uint16_t *tvoc_ppb,
                       uint16_t *co2_eq_ppm)
{
    int16_t ret;
    uint16_t words[SGP30_CMD_IAQ_MEASURE_WORDS];

    ret = sgp30_read_words(dev, words,
                           SGP30_CMD_IAQ_MEASURE_WORDS);

    *tvoc_ppb = words[1];
    *co2_eq_ppm = words[0];

    return ret;
}

int16_t sgp30_measure_iaq_blocking_read(const struct device *dev,
                                        uint16_t *tvoc_ppb,
                                        uint16_t *co2_eq_ppm)
{
    int16_t ret;

    ret = sgp30_measure_iaq(dev);
    if (ret != 0)
        return ret;

    k_usleep(SGP30_CMD_IAQ_MEASURE_DURATION_US);

    return sgp30_read_iaq(dev, tvoc_ppb, co2_eq_ppm);
}

int16_t sgp30_measure_tvoc(const struct device *dev)
{
    return sgp30_measure_iaq(dev);
}

int16_t sgp30_read_tvoc(const struct device *dev,
                        uint16_t *tvoc_ppb)
{
    uint16_t co2_eq_ppm;
    return sgp30_read_iaq(dev, tvoc_ppb, &co2_eq_ppm);
}

int16_t sgp30_measure_tvoc_blocking_read(const struct device *dev,
                                         uint16_t *tvoc_ppb)
{
    uint16_t co2_eq_ppm;
    return sgp30_measure_iaq_blocking_read(dev, tvoc_ppb, &co2_eq_ppm);
}

int16_t sgp30_measure_co2_eq(const struct device *dev)
{
    return sgp30_measure_iaq(dev);
}

int16_t sgp30_read_co2_eq(const struct device *dev,
                          uint16_t *co2_eq_ppm)
{
    uint16_t tvoc_ppb;
    return sgp30_read_iaq(dev, &tvoc_ppb, co2_eq_ppm);
}

int16_t sgp30_measure_co2_eq_blocking_read(const struct device *dev,
                                           uint16_t *co2_eq_ppm)
{
    uint16_t tvoc_ppb;
    return sgp30_measure_iaq_blocking_read(dev, &tvoc_ppb, co2_eq_ppm);
}

int16_t sgp30_measure_raw_blocking_read(const struct device *dev,
                                        uint16_t *ethanol_raw_signal,
                                        uint16_t *h2_raw_signal)
{
    int16_t ret;

    ret = sgp30_measure_raw(dev);
    if (ret != 0)
        return ret;

    k_usleep(SGP30_CMD_RAW_MEASURE_DURATION_US);

    return sgp30_read_raw(dev, ethanol_raw_signal, h2_raw_signal);
}

int16_t sgp30_measure_raw(const struct device *dev)
{
    return sgp30_write_cmd(dev, SGP30_CMD_RAW_MEASURE);
}

int16_t sgp30_read_raw(const struct device *dev,
                       uint16_t *ethanol_raw_signal,
                       uint16_t *h2_raw_signal)
{
    int16_t ret;
    uint16_t words[SGP30_CMD_RAW_MEASURE_WORDS];

    ret = sgp30_read_words(dev, words, SGP30_CMD_RAW_MEASURE_WORDS);

    *ethanol_raw_signal = words[1];
    *h2_raw_signal = words[0];

    return ret;
}

int16_t sgp30_get_iaq_baseline(const struct device *dev,
                               uint32_t *baseline)
{
    int16_t ret;
    uint16_t words[SGP30_CMD_GET_IAQ_BASELINE_WORDS];

    ret = sgp30_write_cmd(dev, SGP30_CMD_GET_IAQ_BASELINE);

    if (ret != 0)
        return ret;

    k_usleep(SGP30_CMD_GET_IAQ_BASELINE_DURATION_US);

    ret = sgp30_read_words(dev, words,
                           SGP30_CMD_GET_IAQ_BASELINE_WORDS);

    if (ret != 0)
        return ret;

    *baseline = ((uint32_t)words[1] << 16) | ((uint32_t)words[0]);

    if (*baseline)
        return 0;
    return -ENOTSUP;
}

int16_t sgp30_set_iaq_baseline(const struct device *dev,
                               uint32_t baseline)
{
    int16_t ret;
    uint16_t words[2] = {(uint16_t)((baseline & 0xffff0000) >> 16),
                         (uint16_t)(baseline & 0x0000ffff)};

    if (!baseline)
        return -ENOTSUP;

    ret = sgp30_write_cmd_args(dev, SGP30_CMD_SET_IAQ_BASELINE, words, SGP30_NUM_WORDS(words));

    k_usleep(SGP30_CMD_SET_IAQ_BASELINE_DURATION_US);

    return ret;
}

int16_t sgp30_get_tvoc_inceptive_baseline(const struct device *dev,
                                          uint16_t *tvoc_inceptive_baseline)
{
    int16_t ret;

    ret = sgp30_check_featureset(dev, 0x21);

    if (ret != 0)
        return ret;

    ret = sgp30_write_cmd(dev, SGP30_CMD_GET_TVOC_INCEPTIVE_BASELINE);

    if (ret != 0)
        return ret;

    k_usleep(SGP30_CMD_GET_TVOC_INCEPTIVE_BASELINE_DURATION_US);

    return sgp30_read_words(dev, tvoc_inceptive_baseline, SGP30_CMD_GET_TVOC_INCEPTIVE_BASELINE_WORDS);
}

int16_t sgp30_set_tvoc_baseline(const struct device *dev,
                                uint16_t tvoc_baseline)
{
    int16_t ret;

    ret = sgp30_check_featureset(dev, 0x21);

    if (ret != 0)
        return ret;

    if (!tvoc_baseline)
        return -ENOTSUP;

    ret = sgp30_write_cmd_args(
        dev, SGP30_CMD_SET_TVOC_BASELINE, &tvoc_baseline,
        SGP30_NUM_WORDS(tvoc_baseline));

    k_usleep(SGP30_CMD_SET_TVOC_BASELINE_DURATION_US);

    return ret;
}

int16_t sgp30_set_absolute_humidity(const struct device *dev,
                                    uint32_t absolute_humidity)
{
    int16_t ret;
    uint16_t ah_scaled;

    if (absolute_humidity > 256000)
        return -ENOTSUP;

    /* ah_scaled = (absolute_humidity / 1000) * 256 */
    ah_scaled = (uint16_t)((absolute_humidity * 16777) >> 16);

    ret = sgp30_write_cmd_args(dev, SGP30_CMD_SET_ABSOLUTE_HUMIDITY, &ah_scaled,
                               SGP30_NUM_WORDS(ah_scaled));

    k_usleep(SGP30_CMD_SET_ABSOLUTE_HUMIDITY_DURATION_US);

    return ret;
}

int16_t sgp30_get_feature_set_version(const struct device *dev,
                                      uint16_t *feature_set_version,
                                      uint8_t *product_type)
{
    int16_t ret;
    uint16_t words[SGP30_CMD_GET_FEATURESET_WORDS];

    ret = sgp30_delayed_read_cmd(dev,
                                 SGP30_CMD_GET_FEATURESET,
                                 SGP30_CMD_GET_FEATURESET_DURATION_US,
                                 words, SGP30_CMD_GET_FEATURESET_WORDS);

    if (ret != 0)
        return ret;

    *feature_set_version = words[0] & 0x00FF;
    *product_type = (uint8_t)((words[0] & 0xF000) >> 12);

    return 0;
}

int16_t sgp30_get_serial_id(const struct device *dev,
                            uint64_t *serial_id)
{
    int16_t ret;
    uint16_t words[SGP30_CMD_GET_SERIAL_ID_WORDS];

    ret = sgp30_delayed_read_cmd(dev,
                                 SGP30_CMD_GET_SERIAL_ID,
                                 SGP30_CMD_GET_SERIAL_ID_DURATION_US,
                                 words, SGP30_CMD_GET_SERIAL_ID_WORDS);

    if (ret != 0)
        return ret;

    *serial_id = (((uint64_t)words[0]) << 32) | (((uint64_t)words[1]) << 16) |
                 (((uint64_t)words[2]) << 0);

    return 0;
}

int16_t sgp30_iaq_init(const struct device *dev)
{
    int16_t ret = sgp30_write_cmd(dev, SGP30_CMD_IAQ_INIT);
    k_usleep(SGP30_CMD_IAQ_INIT_DURATION_US);
    return ret;
}

int16_t sgp30_probe(const struct device *dev)
{
    int16_t ret = sgp30_check_featureset(dev, 0x20);

    if (ret != 0)
        return ret;

    return sgp30_iaq_init(dev);
}

/** @brief Update the sensorvalue returned by sensor_channel_get().
 * 
 * This routine updates the data returned from sensor_channel_get().
 * Data is copied from an internal buffer that is updated every second
 * to the internal sensor_values return by sensor_channel_get();
 * 
 **/
int sgp30_sample_fetch(const struct device *dev,
                       enum sensor_channel chan)
{
    struct sgp30_data *data = dev->data;

    while (k_mutex_lock(&data->data_mutex, K_MSEC(100)) != 0)
        ;

    switch (chan)
    {
    case SENSOR_CHAN_VOC:
        data->tvoc.val1 = data->tvoc_internal;
        break;
    case SENSOR_CHAN_CO2:
        data->co2eq.val1 = data->tvoc_internal;
    case SENSOR_CHAN_ALL:
        data->co2eq.val1 = data->co2eq_internal;
        data->tvoc.val1 = data->tvoc_internal;
        break;

    default:
        k_mutex_unlock(&data->data_mutex);
        return -ENOTSUP;
    }

    k_mutex_unlock(&data->data_mutex);
    return 0;
}

int sgp30_channel_get(const struct device *dev,
                      enum sensor_channel chan, 
                      struct sensor_value *val)
{
    struct sgp30_data *data = dev->data;

    if (k_mutex_lock(&data->data_mutex, K_MSEC(100)) == 0)
    {
        switch (chan)
        {
        case SENSOR_CHAN_CO2:
            *val = data->co2eq;
            break;
        case SENSOR_CHAN_VOC:
            *val = data->tvoc;
            break;
        case SENSOR_CHAN_ALL:
            val[0] = data->co2eq;
            val[1] = data->tvoc;
            break;
        default:
            k_mutex_unlock(&data->data_mutex);
            return -ENOTSUP;
        }

        k_mutex_unlock(&data->data_mutex);
    }
    else
    {
        LOG_ERR("Failed to acquire data lock");
        return -EBUSY;
    }
    return 0;
}

void sgp30_fetch_work(struct k_work *work)
{
    struct sgp30_data *data = CONTAINER_OF(work, struct sgp30_data, fetch_work);
    const struct device *dev = data->dev;
    uint16_t tvocd, co2eqd;

    int rc = sgp30_measure_iaq_blocking_read(dev, &tvocd, &co2eqd);
    if (rc != 0)
    {
        LOG_ERR("Reading air quality failed");
    }

    while (k_mutex_lock(&data->data_mutex, K_MSEC(10)) != 0)
        ;
    data->co2eq_internal = co2eqd;
    data->tvoc_internal = tvocd;

    k_mutex_unlock(&data->data_mutex);
}

void sgp30_submit_work(struct k_timer *timer)
{
    struct sgp30_data *data = CONTAINER_OF(timer, struct sgp30_data, fetch_timer);
    k_work_submit(&data->fetch_work);
}

static const struct sensor_driver_api sgp30_driver_api = {
    .sample_fetch = sgp30_sample_fetch,
    .channel_get = sgp30_channel_get,
};

static struct sgp30_config sgp30_0_cfg = {
    .bus_name = DT_INST_BUS_LABEL(0),
    .base_address = DT_INST_REG_ADDR(0),
};

static struct sgp30_data sgp30_0_data = {0};

static int sgp30_init(const struct device *dev)
{
    struct sgp30_data *data = dev->data;
    const struct sgp30_config *cfg = dev->config;
    const struct device *bus = device_get_binding(cfg->bus_name);

    LOG_DBG("Inititializing SGP30");
    if (bus == NULL)
    {
        return -EINVAL;
    }
    data->bus = bus;

    if (!cfg->base_address)
    {
        return -EINVAL;
    }
    data->dev = dev;

    int rc = sgp30_probe(dev);
    if (rc != 0)
    {
        LOG_ERR("SGP30 probe failed with code: %d", rc);
        return rc;
    }

    rc = sgp30_get_serial_id(dev, &data->serial);
    if (rc == 0)
    {
        LOG_INF("SGP30 serial number %lld", data->serial);
    }
    else
    {
        return rc;
    }

    k_work_init(&data->fetch_work, sgp30_fetch_work);
    k_timer_init(&data->fetch_timer, sgp30_submit_work, NULL);
    k_mutex_init(&data->data_mutex);

    k_timer_start(&data->fetch_timer, K_SECONDS(1), K_SECONDS(1));
    return 0;
}

DEVICE_DT_INST_DEFINE(0, sgp30_init, device_pm_control_nop,
                      &sgp30_0_data, &sgp30_0_cfg,
                      POST_KERNEL, CONFIG_SENSOR_INIT_PRIORITY,
                      &sgp30_driver_api);
