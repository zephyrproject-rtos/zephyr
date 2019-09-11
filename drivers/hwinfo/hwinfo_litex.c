/*
 * Copyright (c) 2019 Antmicro <www.antmicro.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <drivers/hwinfo.h>
#include <soc.h>
#include <string.h>
#include <device.h>
#include <sys/util.h>

ssize_t z_impl_hwinfo_get_device_id(u8_t *buffer, size_t length)
{
	u32_t volatile *ptr = (u32_t volatile *)(DT_INST_0_LITEX_DNA0_BASE_ADDRESS);
	ssize_t end = MIN(length, (DT_INST_0_LITEX_DNA0_SIZE / sizeof(u32_t)));

	for (int i = 0; i < end; i++) {
		/* In LiteX even though registers are 32-bit wide, each one
		   contains meaningful data only in the lowest 8 bits */
		buffer[i] = (u8_t)(ptr[i] & 0xff);
	}

	return end;
}
