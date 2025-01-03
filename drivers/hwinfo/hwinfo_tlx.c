/*
 * Copyright (c) 2024 Telink Semiconductor
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

#if CONFIG_SOC_RISCV_TELINK_TL721X
	flash_mid = flash_read_mid_with_device_num(SLAVE0);
	flash_read_mid_uid_with_check_with_device_num(SLAVE0, &flash_mid, uid);
#elif CONFIG_SOC_RISCV_TELINK_TL321X
	flash_mid = flash_read_mid();
	flash_read_mid_uid_with_check(&flash_mid, uid);
#endif
	if (length > sizeof(uid)) {
		length = sizeof(uid);
	}
	memcpy(buffer, uid, length);

	return length;
}
