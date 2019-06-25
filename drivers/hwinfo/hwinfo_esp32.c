/*
 * Copyright (c) 2019 Leandro A. F. Pereira
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <soc/efuse_reg.h>

#include <drivers/hwinfo.h>
#include <string.h>

ssize_t _impl_hwinfo_get_device_id(u8_t *buffer, size_t length)
{
	uint32_t fuse_rdata[] = {
		sys_read32(EFUSE_BLK0_RDATA1_REG),
		sys_read32(EFUSE_BLK0_RDATA2_REG),
	};

	if (length > sizeof(fuse_rdata)) {
		length = sizeof(fuse_rdata);
	}

	memcpy(buffer, fuse_rdata, length);

	return length;
}
