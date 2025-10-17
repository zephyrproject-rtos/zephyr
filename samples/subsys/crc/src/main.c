/*
 * Copyright (c) 2025 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(subsys_crc_example, CONFIG_LOG_DEFAULT_LEVEL);

#include <zephyr/sys/crc.h>

int main(void)
{
	uint32_t result;
	uint8_t data[8] = {0x0A, 0x2B, 0x4C, 0x6D, 0x8E, 0x49, 0x00, 0xC4};

	/* CRC computation */
	result = crc32_ieee(data, sizeof(data));
	LOG_INF("Result of CRC32 IEEE: 0x%08X", result);

	/* CRC computation */
	result = (uint8_t)crc8_ccitt(0xFF, data, sizeof(data));
	LOG_INF("Result of CRC8 CCITT: 0x%02X", result & 0xFF);

	LOG_INF("CRC computation completed successfully");

	return 0;
}
