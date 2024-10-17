/*
 * Copyright (c) 2024 Vogl Electronic GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "atecc_priv.h"
LOG_MODULE_DECLARE(ateccx08);

static int atecc_read(const struct device *dev, enum atecc_zone zone, uint8_t slot, uint8_t block,
		      uint8_t offset, uint8_t *data, uint8_t len)
{
	__ASSERT(len == 4U || len == 32U, "Invalid length");

	struct ateccx08_packet packet = {0};
	int ret;

	packet.param1 = (len == ATECC_BLOCK_SIZE) ? (zone | ATECC_ZONE_READWRITE_32) : zone;
	packet.param2 = atecc_get_addr(zone, slot, block, offset);

	atecc_command(ATECC_READ, &packet);

	ret = atecc_execute_command(dev, &packet);
	if (ret < 0) {
		LOG_ERR("%s: failed: %d", __func__, ret);
		return ret;
	}

	memcpy(data, &packet.data[1], len);

	return ret;
}

int atecc_read_bytes(const struct device *dev, enum atecc_zone zone, uint8_t slot, uint16_t offset,
		     uint8_t *data, uint16_t len)
{
	__ASSERT(dev != NULL, "device is NULL");
	__ASSERT(data != NULL, "data is NULL");

	struct ateccx08_data *dev_data = dev->data;
	uint8_t buffer_idx = 0;
	uint8_t buffer[ATECC_BLOCK_SIZE] = {0};
	uint8_t copy_length;
	uint8_t current_block;
	uint8_t current_offset = 0;
	uint16_t data_idx = 0;
	uint16_t read_offset = 0;
	uint8_t read_size = ATECC_BLOCK_SIZE;
	uint16_t zone_size;
	int ret = 0;

	switch (zone) {
	case ATECC_ZONE_CONFIG:
		break;
	case ATECC_ZONE_DATA:
	case ATECC_ZONE_OTP:
		if (!dev_data->is_locked_data) {
			LOG_ERR("Data zones unlocked");
			return -EPERM;
		}
		break;

	default:
		LOG_ERR("Invalid zone");
		return -EINVAL;
	}

	zone_size = atecc_get_zone_size(zone, slot);

	if ((offset + len) > zone_size) {
		LOG_WRN("attempt to read past zone boundary");
		return -EINVAL;
	}

	current_block = offset / ATECC_BLOCK_SIZE;

	if (len <= ATECC_WORD_SIZE &&
	    ((offset / ATECC_WORD_SIZE) == ((offset + len - 1) / ATECC_WORD_SIZE))) {
		read_size = ATECC_WORD_SIZE;
		current_offset = (offset / ATECC_WORD_SIZE) % (ATECC_BLOCK_SIZE / ATECC_WORD_SIZE);
	}

	pm_device_busy_set(dev);

	while (data_idx < len) {
		if (read_size == ATECC_BLOCK_SIZE &&
		    zone_size - (uint16_t)current_block * ATECC_BLOCK_SIZE < ATECC_BLOCK_SIZE) {
			/* Switch to word reads */
			read_size = ATECC_WORD_SIZE;
			current_offset = ((data_idx + offset) / ATECC_WORD_SIZE) %
					 (ATECC_BLOCK_SIZE / ATECC_WORD_SIZE);
		}

		ret = atecc_read(dev, zone, slot, current_block, current_offset, buffer, read_size);
		if (ret < 0) {
			LOG_ERR("Reading zone failed: %d", ret);
			goto atecc_read_bytes_end;
		}

		read_offset = (uint16_t)current_block * ATECC_BLOCK_SIZE +
			      (uint16_t)current_offset * ATECC_WORD_SIZE;

		buffer_idx = (read_offset < offset) ? offset - read_offset : 0;

		copy_length = MIN(read_size - buffer_idx, len - data_idx);

		memcpy(&data[data_idx], &buffer[buffer_idx], copy_length);
		data_idx += copy_length;
		if (read_size == ATECC_BLOCK_SIZE) {
			current_block++;
		} else {
			current_offset++;
		}
	}

atecc_read_bytes_end:
	pm_device_busy_clear(dev);
	return ret;
}

int atecc_read_serial_number(const struct device *dev, uint8_t *serial_number)
{
	__ASSERT(dev != NULL, "device is NULL");
	__ASSERT(serial_number != NULL, "serial_number is NULL");

	int ret;
	uint8_t buffer[ATECC_BLOCK_SIZE];

	pm_device_busy_set(dev);
	ret = atecc_read(dev, ATECC_ZONE_CONFIG, 0, 0, 0, buffer, ATECC_BLOCK_SIZE);
	pm_device_busy_clear(dev);
	if (ret < 0) {
		LOG_ERR("Reading serial number failed: %d", ret);
		return ret;
	}

	memcpy(&serial_number[0], &buffer[0], 4);
	memcpy(&serial_number[4], &buffer[8], 5);

	return ret;
}
