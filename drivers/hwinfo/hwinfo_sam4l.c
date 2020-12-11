/*
 * Copyright (c) 2020 Gerson Fernando Budke <nandojve@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT atmel_sam4l_uid

#include <device.h>
#include <drivers/hwinfo.h>
#include <init.h>
#include <soc.h>
#include <string.h>

ssize_t z_impl_hwinfo_get_device_id(uint8_t *buffer, size_t length)
{
	uint8_t *uid_addr = (uint8_t *) DT_INST_REG_ADDR(0);

	if (buffer == NULL) {
		return 0;
	}

	if (length > DT_INST_REG_SIZE(0) || length < 0) {
		length = DT_INST_REG_SIZE(0);
	}

	memcpy(buffer, uid_addr, length);

	return length;
}
