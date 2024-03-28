/*
 * Copyright (c) 2024 Vogl Electronic GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "atecc_priv.h"
LOG_MODULE_DECLARE(ateccx08);

static int atecc_write(const struct device *dev, enum atecc_zone zone, uint8_t slot, uint8_t block,
		       uint8_t offset, const uint8_t *data, uint8_t len)
{
	__ASSERT(len == 4u || len == 32u, "Invalid length");

	struct ateccx08_packet packet = {0};
	int ret;

	packet.param1 = (len == ATECC_BLOCK_SIZE) ? (zone | ATECC_ZONE_READWRITE_32) : zone;
	packet.param2 = atecc_get_addr(zone, slot, block, offset);

	memcpy(packet.data, data, len);

	atecc_command(ATECC_WRITE, &packet);

	ret = atecc_execute_command(dev, &packet);
	if (ret < 0) {
		LOG_ERR("%s: failed: %d", __func__, ret);
	}

	return ret;
}

int atecc_write_bytes(const struct device *dev, enum atecc_zone zone, uint8_t slot,
		      uint16_t offset_bytes, const uint8_t *data, uint16_t len)
{
	__ASSERT(dev != NULL, "device is NULL");
	__ASSERT(data != NULL, "data is NULL");

	if (offset_bytes % ATECC_WORD_SIZE != 0 || len % ATECC_WORD_SIZE != 0) {
		LOG_ERR("Invalid length/offset");
		return -EINVAL;
	}

	struct ateccx08_data *dev_data = dev->data;
	uint8_t current_block = offset_bytes / ATECC_BLOCK_SIZE;
	uint8_t current_word = (offset_bytes % ATECC_BLOCK_SIZE) / ATECC_WORD_SIZE;
	size_t data_idx = 0;
	int ret = 0;

	switch (zone) {
	case ATECC_ZONE_CONFIG:
		if (dev_data->is_locked_config) {
			LOG_ERR("Config zone locked");
			return -EPERM;
		}
		break;

	case ATECC_ZONE_DATA:
	case ATECC_ZONE_OTP:
		if (!dev_data->is_locked_config) {
			LOG_ERR("Config zone unlocked");
			return -EPERM;
		} else if (dev_data->is_locked_data) {
			LOG_ERR("Data zones locked");
			return -EPERM;
		}
		break;

	default:
		LOG_ERR("Invalid zone");
		return -EINVAL;
	}

	if (offset_bytes + len > atecc_get_zone_size(zone, slot)) {
		LOG_WRN("attempt to write past zone boundary");
		return -EINVAL;
	}

	pm_device_busy_set(dev);
	while (data_idx < len) {
		if (current_word == 0u && len - data_idx >= ATECC_BLOCK_SIZE &&
		    !(zone == ATECC_ZONE_CONFIG && current_block == 2u)) {
			ret = atecc_write(dev, zone, slot, current_block, 0, &data[data_idx],
					  ATECC_BLOCK_SIZE);
			if (ret < 0) {
				goto atecc_write_bytes_end;
			}
			data_idx += ATECC_BLOCK_SIZE;
			current_block += 1u;
		} else {
			/* Skip changing UserExtra, Selector, LockValue, and LockConfig */
			if (!(zone == ATECC_ZONE_CONFIG && current_block == 2u &&
			      current_word == 5u)) {
				ret = atecc_write(dev, zone, slot, current_block, current_word,
						  &data[data_idx], ATECC_WORD_SIZE);
				if (ret < 0) {
					goto atecc_write_bytes_end;
				}
			}
			data_idx += ATECC_WORD_SIZE;
			current_word += 1u;
			if (current_word == ATECC_BLOCK_SIZE / ATECC_WORD_SIZE) {
				current_block += 1u;
				current_word = 0u;
			}
		}
	}

atecc_write_bytes_end:
	pm_device_busy_clear(dev);
	return ret;
}

int atecc_write_config(const struct device *dev, const uint8_t *config_data)
{
	if (config_data == NULL) {
		LOG_ERR("NULL pointer received");
		return -EINVAL;
	}

	int ret;
	uint16_t config_size = atecc_get_zone_size(ATECC_ZONE_CONFIG, 0);

	ret = atecc_write_bytes(dev, ATECC_ZONE_CONFIG, 0, 16, &config_data[16], config_size - 16u);
	if (ret < 0) {
		LOG_ERR("Write config failed: %d", ret);
	}

	return ret;
}
