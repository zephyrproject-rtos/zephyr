/*
 * Copyright (c) 2024 Vogl Electronic GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "atecc_priv.h"
LOG_MODULE_DECLARE(ateccx08);

#define LOCK_ZONE_CONFIG 0x00U
#define LOCK_ZONE_DATA   0x01U
#define LOCK_ZONE_NO_CRC 0x80U
#define ATECC_UNLOCKED   0x55U

static int atecc_lock(const struct device *dev, uint8_t mode, uint16_t summary_crc)
{
	__ASSERT(dev != NULL, "device is NULL");

	struct ateccx08_packet packet = {0};
	int ret;

	packet.param1 = mode;
	packet.param2 = summary_crc;

	atecc_command(ATECC_LOCK, &packet);

	ret = atecc_execute_command_pm(dev, &packet);
	if (ret < 0) {
		LOG_ERR("%s: failed: %d", __func__, ret);
		return ret;
	}

	atecc_update_lock(dev);

	return 0;
}

int atecc_lock_zone(const struct device *dev, enum atecc_zone zone)
{
	switch (zone) {
	case ATECC_ZONE_CONFIG:
		return atecc_lock(dev, LOCK_ZONE_NO_CRC | LOCK_ZONE_CONFIG, 0);

	case ATECC_ZONE_OTP:
	case ATECC_ZONE_DATA:
		return atecc_lock(dev, LOCK_ZONE_NO_CRC | LOCK_ZONE_DATA, 0);

	default:
		return -EINVAL;
	}
}

int atecc_lock_zone_crc(const struct device *dev, enum atecc_zone zone, uint16_t summary_crc)
{
	switch (zone) {
	case ATECC_ZONE_CONFIG:
		return atecc_lock(dev, LOCK_ZONE_CONFIG, summary_crc);

	case ATECC_ZONE_OTP:
	case ATECC_ZONE_DATA:
		return atecc_lock(dev, LOCK_ZONE_DATA, summary_crc);

	default:
		return -EINVAL;
	}
}

void atecc_update_lock(const struct device *dev)
{
	__ASSERT(dev != NULL, "device is NULL");

	struct ateccx08_data *dev_data = dev->data;
	uint8_t data_lock[2];
	int ret;

	ret = atecc_read_bytes(dev, ATECC_ZONE_CONFIG, 0, 86, data_lock, sizeof(data_lock));
	if (ret < 0) {
		LOG_ERR("%s: failed: %d", __func__, ret);
		return;
	}

	if (data_lock[0] == ATECC_UNLOCKED) {
		dev_data->is_locked_data = false;
	} else {
		dev_data->is_locked_data = true;
	}

	if (data_lock[1] == ATECC_UNLOCKED) {
		dev_data->is_locked_config = false;
	} else {
		dev_data->is_locked_config = true;
	}
}

bool atecc_is_locked_config(const struct device *dev)
{
	__ASSERT(dev != NULL, "device is NULL");

	struct ateccx08_data *data = dev->data;

	return data->is_locked_config;
}

bool atecc_is_locked_data(const struct device *dev)
{
	__ASSERT(dev != NULL, "device is NULL");

	struct ateccx08_data *data = dev->data;

	return data->is_locked_data;
}
