/*
 * Copyright (c) 2025 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(crc_example, CONFIG_LOG_DEFAULT_LEVEL);

#include <zephyr/device.h>
#include <zephyr/drivers/crc.h>

/* The devicetree node identifier for the "crc" */
#define CRC_NODE DT_CHOSEN(zephyr_crc)

/*
 * A build error on this line means your board is unsupported.
 * See the sample documentation for information on how to fix this.
 */

int main(void)
{
	static const struct device *const dev = DEVICE_DT_GET(CRC_NODE);
	/* Define the data to compute CRC */
	uint8_t data[8] = {0x0A, 0x2B, 0x4C, 0x6D, 0x8E, 0x49, 0x00, 0xC4};
	int ret;

	if (!device_is_ready(dev)) {
		LOG_ERR("Device is not ready");
		return -ENODEV;
	}

	struct crc_ctx ctx = {
		.type = CRC8,
		.polynomial = CRC8_POLY,
		.seed = CRC8_INIT_VAL,
		.reversed = CRC_FLAG_REVERSE_OUTPUT | CRC_FLAG_REVERSE_INPUT,
	};

	/* Start CRC computation */
	ret = crc_begin(dev, &ctx);

	if (ret != 0) {
		LOG_ERR("Failed to begin CRC: %d", ret);
		return ret;
	}

	/* Update CRC computation */
	ret = crc_update(dev, &ctx, data, 8);

	if (ret != 0) {
		LOG_ERR("Failed to update CRC: %d", ret);
		return ret;
	}

	/* Finish CRC computation */
	ret = crc_finish(dev, &ctx);

	if (ret != 0) {
		LOG_ERR("Failed to finish CRC: %d", ret);
		return ret;
	}
	/* Verify CRC computation
	 * (example expected value: 0xB2 with LSB, 0x4D with MSB bit order)
	 */
	if (ctx.reversed != (CRC_FLAG_REVERSE_OUTPUT | CRC_FLAG_REVERSE_INPUT)) {
		ret = crc_verify(&ctx, 0x4D);

		if (ret != 0) {
			LOG_ERR("CRC verification failed: %d", ret);
			return ret;
		}
	} else { /* Reversed is no reversed output */
		ret = crc_verify(&ctx, 0xB2);

		if (ret != 0) {
			LOG_ERR("CRC verification failed: %d", ret);
			return ret;
		}
	}

	LOG_INF("CRC verification succeeded");

	return 0;
}
