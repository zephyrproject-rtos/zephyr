/*
 * Copyright (c) 2021 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_lpc_uid

#include <zephyr/drivers/hwinfo.h>
#include <string.h>
#include <zephyr/sys/byteorder.h>

ssize_t z_impl_hwinfo_get_device_id(uint8_t *buffer, size_t length)
{
	uint8_t *uid_addr = (uint8_t *) DT_INST_REG_ADDR(0);

	if (buffer == NULL) {
		return 0;
	}

	if (length > DT_INST_REG_SIZE(0)) {
		length = DT_INST_REG_SIZE(0);
	}

	memcpy(buffer, uid_addr, length);

	return length;
}
