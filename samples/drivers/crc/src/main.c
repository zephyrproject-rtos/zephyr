/*
 * Copyright (c) 2024 Brill Power Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(crc_example, CONFIG_LOG_DEFAULT_LEVEL);

#include <zephyr/drivers/crc.h>
#include <zephyr/device.h>
#include <zephyr/kernel.h>

int main(void)
{
	/* Retrieve the CRC device */
	static const struct device *dev = DEVICE_DT_GET(DT_ALIAS(crc0));

	/* Ensure the device is ready */
	if (!device_is_ready(dev)) {
		LOG_ERR("CRC device not found or not ready\n");
		return -ENODEV;
	}

	/* Define the data to compute CRC */
	uint8_t data[8] = {0x0A, 0x2B, 0x4C, 0x6D, 0x8E, 0x49, 0x00, 0xC4};

	/* Define the CRC context */
	struct crc_ctx ctx = {.type = CRC8_CCITT_HW,
			      .flags = 0,
			      .polynomial = CRC8_CCITT_POLY,
			      .initial_value = CRC8_CCITT_INIT_VAL};

	/* Start CRC computation */
	int ret = crc_begin(dev, &ctx);

	if (ret != 0) {
		LOG_ERR("Failed to begin CRC: %d\n", ret);
		return ret;
	}

	/* Update CRC computation */
	ret = crc_update(dev, &ctx, data, 8);

	if (ret != 0) {
		LOG_ERR("Failed to update CRC: %d\n", ret);
		return ret;
	}

	/* Finish CRC computation */
	ret = crc_finish(dev, &ctx);

	if (ret != 0) {
		LOG_ERR("Failed to finish CRC: %d\n", ret);
		return ret;
	}

	/* Verify the computed CRC (example expected value: 0x4D) */
	ret = crc_verify(&ctx, 0x4D);

	if (ret != 0) {
		LOG_ERR("CRC verification failed: %d\n", ret);
		return ret;
	}

	LOG_INF("CRC verification succeeded");

	return ret;
}
