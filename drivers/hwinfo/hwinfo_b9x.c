/*
 * Copyright (c) 2021-2023 Telink Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/hwinfo.h>
#include <string.h>
#include <flash.h>

ssize_t z_impl_hwinfo_get_device_id(uint8_t *buffer, size_t length)
{
	uint32_t flash_mid = 0;
	uint8_t uid[16];

#if CONFIG_SOC_RISCV_TELINK_B91 || CONFIG_SOC_RISCV_TELINK_B92
	flash_mid = flash_read_mid();
	flash_read_mid_uid_with_check((unsigned int *)&flash_mid, uid);
#elif CONFIG_SOC_RISCV_TELINK_B95
	flash_mid = flash_read_mid(SLAVE0);
	flash_read_mid_uid_with_check(SLAVE0, (unsigned int *)&flash_mid, uid);
#endif

	if (length > sizeof(uid)) {
		length = sizeof(uid);
	}
	memcpy(buffer, uid, length);

	return length;
}
