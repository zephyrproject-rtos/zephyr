/*
 * Copyright (c) 2021 Telink Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/hwinfo.h>
#include <string.h>

ssize_t z_impl_hwinfo_get_device_id(uint8_t *buffer, size_t length)
{
	uint32_t flash_mid = 0;
	uint8_t uid[16];

	flash_mid = flash_read_mid();

	flash_read_mid_uid_with_check((unsigned int *)&flash_mid, uid);

	if (length > sizeof(uid)) {
		length = sizeof(uid);
	}
	memcpy(buffer, uid, length);

	return length;
}
