/*
 * Copyright (c) 2025 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(subsys_crc_example, CONFIG_LOG_DEFAULT_LEVEL);

#include <zephyr/crc_new/crc_new.h>

int main(void)
{
	uint32_t result;
	uint8_t data[8] = {0x0A, 0x2B, 0x4C, 0x6D, 0x8E, 0x49, 0x00, 0xC4};
	int ret;

	/* CRC computation */
	ret = crc32_ieee_new(data, sizeof(data), &result);

	if (ret != 0) {
		LOG_ERR("Failed compute CRC32_IEEE: %d", ret);
		return ret;
	}

	LOG_INF("Result of CRC32 IEEE: 0x%08X", result);

	/* CRC computation */
	ret = crc8_ccitt_new(0xFF, data, sizeof(data), (uint8_t *)&result);

	if (ret != 0) {
		LOG_ERR("Failed compute CRC8_CCITT: %d", ret);
		return ret;
	}

	LOG_INF("Result of CRC8 CCITT: 0x%02X", result & 0xFF);

	LOG_INF("CRC computation completed successfully");

	return 0;
}
