/*
 * Private Porting
 * by David Hor - Xtooltech 2025
 * david.hor@xtooltech.com
 *
 * Copyright (c) 2024 VCI Development
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/hwinfo.h>
#include <zephyr/drivers/hwinfo_lpc.h>
#include <zephyr/logging/log.h>
#include <string.h>
#include <soc.h>

LOG_MODULE_REGISTER(hwinfo_lpc, CONFIG_HWINFO_LOG_LEVEL);

/* LPC54S018 ROM API locations */
#define ROM_API_TABLE_BASE     0x03000000
#define ROM_IAP_ENTRY_LOCATION 0x03000200

/* IAP Commands */
#define IAP_CMD_READ_UID       58

/* IAP return codes */
#define IAP_RET_CMD_SUCCESS    0

typedef void (*iap_entry_t)(uint32_t[], uint32_t[]);

static iap_entry_t iap_entry = (iap_entry_t)ROM_IAP_ENTRY_LOCATION;

/**
 * @brief Read unique ID using ROM IAP
 *
 * The unique ID is 128 bits (16 bytes) arranged as 4 words
 */
static int read_uid_iap(uint32_t uid[4])
{
	uint32_t command[5];
	uint32_t result[5];

	/* Prepare IAP command */
	command[0] = IAP_CMD_READ_UID;

	/* Call IAP */
	iap_entry(command, result);

	/* Check result */
	if (result[0] != IAP_RET_CMD_SUCCESS) {
		LOG_ERR("IAP read UID failed: %d", result[0]);
		return -EIO;
	}

	/* Copy UID from result */
	uid[0] = result[1];
	uid[1] = result[2];
	uid[2] = result[3];
	uid[3] = result[4];

	return 0;
}

int lpc_get_unique_id(uint8_t *buffer, size_t length)
{
	uint32_t uid[4];
	int ret;

	if (!buffer || length < 16) {
		return -EINVAL;
	}

	/* Read UID from ROM */
	ret = read_uid_iap(uid);
	if (ret) {
		return ret;
	}

	/* Convert to byte array */
	memcpy(buffer, uid, 16);

	LOG_DBG("UID: %08x-%08x-%08x-%08x", uid[0], uid[1], uid[2], uid[3]);

	return 0;
}

int lpc_get_serial_number(char *serial, size_t length)
{
	uint8_t uid[16];
	int ret;
	int i;

	if (!serial || length < 33) {
		return -EINVAL;
	}

	ret = lpc_get_unique_id(uid, sizeof(uid));
	if (ret) {
		return ret;
	}

	/* Convert to hex string */
	for (i = 0; i < 16; i++) {
		snprintf(&serial[i * 2], 3, "%02x", uid[i]);
	}
	serial[32] = '\0';

	return 0;
}

/* Implement standard Zephyr hwinfo API */
ssize_t z_impl_hwinfo_get_device_id(uint8_t *buffer, size_t length)
{
	uint8_t uid[16];
	int ret;

	ret = lpc_get_unique_id(uid, sizeof(uid));
	if (ret) {
		return ret;
	}

	if (buffer) {
		memcpy(buffer, uid, MIN(length, 16));
	}

	return 16;
}

int z_impl_hwinfo_get_device_eui64(uint8_t *buffer)
{
	uint8_t uid[16];
	int ret;

	if (!buffer) {
		return -EINVAL;
	}

	ret = lpc_get_unique_id(uid, sizeof(uid));
	if (ret) {
		return ret;
	}

	/* Use first 8 bytes of UID as EUI64 */
	memcpy(buffer, uid, 8);

	/* Set universal/local bit */
	buffer[0] |= 0x02;

	return 0;
}

static int hwinfo_lpc_init(void)
{
	uint8_t uid[16];
	char serial[33];
	int ret;

	/* Test reading UID at startup */
	ret = lpc_get_unique_id(uid, sizeof(uid));
	if (ret) {
		LOG_ERR("Failed to read unique ID: %d", ret);
		return ret;
	}

	ret = lpc_get_serial_number(serial, sizeof(serial));
	if (ret) {
		LOG_ERR("Failed to get serial number: %d", ret);
		return ret;
	}

	LOG_INF("LPC54S018 Unique ID: %s", serial);

	return 0;
}

SYS_INIT(hwinfo_lpc_init, APPLICATION, CONFIG_KERNEL_INIT_PRIORITY_DEVICE);