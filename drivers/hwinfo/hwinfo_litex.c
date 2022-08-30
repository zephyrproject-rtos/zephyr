/*
 * Copyright (c) 2019 Antmicro <www.antmicro.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT litex_dna0

#include <zephyr/drivers/hwinfo.h>
#include <soc.h>
#include <string.h>
#include <zephyr/device.h>
#include <zephyr/sys/util.h>

ssize_t z_impl_hwinfo_get_device_id(uint8_t *buffer, size_t length)
{
	uint32_t addr = DT_INST_REG_ADDR(0);
	ssize_t end = MIN(length, DT_INST_REG_ADDR(0) / 4 *
			CONFIG_LITEX_CSR_DATA_WIDTH / 8);
	for (int i = 0; i < end; i++) {
#if CONFIG_LITEX_CSR_DATA_WIDTH == 8
		buffer[i] = litex_read8(addr);
		addr += 4;
#elif CONFIG_LITEX_CSR_DATA_WIDTH == 32
		buffer[i] = (uint8_t)(litex_read32(addr) >> (addr % 4 * 8));
		addr += 1;
#else
#error Unsupported CSR data width
#endif
	}
	return end;
}
