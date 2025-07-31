/*
 * Copyright (c) 2024 VCI Development
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/hwinfo.h>
#include <zephyr/drivers/hwinfo_lpc.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(hwinfo_sample, LOG_LEVEL_INF);

int main(void)
{
	uint8_t uid[16];
	uint8_t eui64[8];
	char serial[33];
	ssize_t len;
	int ret;
	int i;

	LOG_INF("LPC54S018 Hardware Info Sample");

	/* Get device ID using standard hwinfo API */
	len = hwinfo_get_device_id(uid, sizeof(uid));
	if (len < 0) {
		LOG_ERR("Failed to get device ID: %d", (int)len);
		return -1;
	}

	LOG_INF("Device ID (%d bytes):", (int)len);
	for (i = 0; i < len; i += 8) {
		LOG_INF("  %02x %02x %02x %02x %02x %02x %02x %02x",
			uid[i], uid[i+1], uid[i+2], uid[i+3],
			uid[i+4], uid[i+5], uid[i+6], uid[i+7]);
	}

	/* Get EUI64 */
	ret = hwinfo_get_device_eui64(eui64);
	if (ret == 0) {
		LOG_INF("EUI64: %02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x",
			eui64[0], eui64[1], eui64[2], eui64[3],
			eui64[4], eui64[5], eui64[6], eui64[7]);
	} else {
		LOG_WRN("Failed to get EUI64: %d", ret);
	}

	/* Get serial number as string using LPC-specific API */
	ret = lpc_get_serial_number(serial, sizeof(serial));
	if (ret == 0) {
		LOG_INF("Serial Number: %s", serial);
	} else {
		LOG_ERR("Failed to get serial number: %d", ret);
	}

	/* Demonstrate using unique ID directly */
	ret = lpc_get_unique_id(uid, sizeof(uid));
	if (ret == 0) {
		uint32_t *uid_words = (uint32_t *)uid;
		LOG_INF("Unique ID (as words):");
		LOG_INF("  Word 0: 0x%08x", uid_words[0]);
		LOG_INF("  Word 1: 0x%08x", uid_words[1]);
		LOG_INF("  Word 2: 0x%08x", uid_words[2]);
		LOG_INF("  Word 3: 0x%08x", uid_words[3]);
	}

	/* Show how unique ID can be used for device identification */
	LOG_INF("This device can be uniquely identified by its 128-bit ID");
	LOG_INF("Use cases:");
	LOG_INF("  - License management");
	LOG_INF("  - Device authentication");
	LOG_INF("  - Inventory tracking");
	LOG_INF("  - Secure key derivation");

	return 0;
}