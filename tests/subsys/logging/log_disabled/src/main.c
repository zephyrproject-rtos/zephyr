/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(app, LOG_LEVEL_DBG);

int main(void)
{
	uint8_t data[] = {0, 1, 2, 3};
	uint32_t dummy_1 = 1;
	uint32_t dummy_2 = 2;
	uint32_t dummy_3 = 3;
	uint32_t dummy_4 = 4;
	uint32_t dummy_5 = 5;
	uint32_t dummy_6 = 6;
	uint32_t dummy_7 = 7;

	LOG_DBG("Debug log %u", dummy_1);
	LOG_INF("Info log %u", dummy_2);
	LOG_WRN("Warning log %u", dummy_3);
	LOG_ERR("Error log %u", dummy_4);

	for (int i = 0; i < 10; i++) {
		LOG_WRN_ONCE("Warning on the first execution only %u", dummy_5);
	}

	LOG_PRINTK("Printk log %u\n", dummy_6);

	LOG_RAW("Raw log %u\n", dummy_7);

	LOG_HEXDUMP_DBG(data, sizeof(data), "Debug data");
	LOG_HEXDUMP_INF(data, sizeof(data), "Info data");
	LOG_HEXDUMP_WRN(data, sizeof(data), "Warning data");
	LOG_HEXDUMP_ERR(data, sizeof(data), "Error data");

	printk("All done.\n");
	return 0;
}
