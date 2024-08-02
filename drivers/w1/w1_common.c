/*
 * Copyright (c) 2022 Thomas Stranger
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/sys/crc.h>
#include <zephyr/types.h>
#include <zephyr/drivers/w1.h>


int z_impl_w1_read_block(const struct device *dev, uint8_t *buffer, size_t len)
{
	const struct w1_driver_api *api = dev->api;
	int ret;

	if (api->read_block != NULL) {
		return api->read_block(dev, buffer, len);
	}
	for (int i = 0; i < len; ++i) {
		ret = w1_read_byte(dev);
		if (ret < 0) {
			return ret;
		}
		buffer[i] = ret;
	}

	return 0;
}

int z_impl_w1_write_block(const struct device *dev, const uint8_t *buffer,
			  size_t len)
{
	const struct w1_driver_api *api = dev->api;
	int ret;

	if (api->write_block != NULL) {
		return api->write_block(dev, buffer, len);
	}
	for (int i = 0; i < len; ++i) {
		ret = w1_write_byte(dev, buffer[i]);
		if (ret < 0) {
			return ret;
		}
	}

	return 0;
}
