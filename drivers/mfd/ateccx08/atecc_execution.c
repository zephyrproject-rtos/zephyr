/*
 * Copyright (c) 2024 Vogl Electronic GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "atecc_priv.h"
LOG_MODULE_DECLARE(ateccx08);

static int atecc_is_error(uint8_t *data)
{
	if (data[0] == 0x04U) {
		if (data[1] == 0x00U) {
			return 0;
		}
		LOG_ERR("ATECC error: %02x", data[1]);
		return -EIO;
	}

	return 0;
}

static int atecc_execute_receive(const struct device *dev, uint8_t *rxdata, size_t rxlength)
{
	const struct ateccx08_config *cfg = dev->config;
	int ret = 0;
	uint16_t read_length = 1;

	ret = i2c_read_dt(&cfg->i2c, rxdata, read_length);
	if (ret < 0) {
		return ret;
	}

	read_length = rxdata[0];

	if (read_length > rxlength) {
		LOG_DBG("Buffer too small to read");
		return -ENOBUFS;
	}

	if (read_length < 4u) {
		LOG_DBG("Invalid response length");
		return -EIO;
	}

	read_length -= 1u;

	ret = i2c_read_dt(&cfg->i2c, &rxdata[1], read_length);
	if (ret < 0) {
		LOG_ERR("Failed to read from device: %d", ret);
		return ret;
	}

	return ret;
}

int atecc_execute_command(const struct device *dev, struct ateccx08_packet *packet)
{
	const struct ateccx08_config *cfg = dev->config;
	struct ateccx08_data *dev_data = dev->data;
	uint32_t max_delay_count = ATECC_POLLING_MAX_TIME_MSEC / ATECC_POLLING_FREQUENCY_TIME_MSEC;
	uint16_t retries = cfg->retries;
	int ret;

	packet->word_addr = ATECC_WA_CMD;

	ret = k_mutex_lock(&dev_data->lock, K_FOREVER);
	if (ret < 0) {
		return ret;
	}

	do {
		if (dev_data->device_state != ATECC_DEVICE_STATE_ACTIVE) {
			(void)atecc_wakeup(dev);
		}

		ret = i2c_write_dt(&cfg->i2c, (uint8_t *)packet, (uint32_t)packet->txsize + 1u);
		if (ret < 0) {
			dev_data->device_state = ATECC_DEVICE_STATE_UNKNOWN;
		} else {
			dev_data->device_state = ATECC_DEVICE_STATE_ACTIVE;
			break;
		}
	} while (retries-- > 0);

	if (ret < 0) {
		LOG_ERR("Failed to write to device: %d", ret);
		goto execute_command_exit;
	}

	k_busy_wait(ATECC_POLLING_INIT_TIME_MSEC * USEC_PER_MSEC);

	do {
		(void)memset(packet->data, 0, sizeof(packet->data));
		/* receive the response */
		ret = atecc_execute_receive(dev, packet->data, sizeof(packet->data));
		if (ret == 0) {
			break;
		}
		LOG_DBG("try receive response again: %d", ret);
		k_busy_wait(ATECC_POLLING_FREQUENCY_TIME_MSEC * USEC_PER_MSEC);
	} while (max_delay_count-- > 0);

	(void)atecc_idle(dev);

execute_command_exit:
	k_mutex_unlock(&dev_data->lock);

	if (ret < 0) {
		return ret;
	}

	ret = atecc_check_crc(packet->data);
	if (ret < 0) {
		return ret;
	}

	return atecc_is_error(packet->data);
}
