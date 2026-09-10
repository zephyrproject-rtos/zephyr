/*
 * Copyright (c) 2025, FocalTech Systems CO.,Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/sys/barrier.h>
#include <zephyr/arch/common/sys_io.h>

/* clocks (soc-specific) */
#define FT9001_CLK_CFG_ADDR    0x40004008u
#define FT9001_CLK_UPD_ADDR    0x40004018u
#define FT9001_CLK_UPD_GO_MASK 0x02u

__ramfunc void xip_clock_switch(uint32_t clk_div)
{
	uint32_t val = sys_read32(FT9001_CLK_CFG_ADDR);

	val = (val & 0xFFFFFFF0u) | (clk_div & 0x0Fu);
	sys_write32(val, FT9001_CLK_CFG_ADDR);
	sys_write32(sys_read32(FT9001_CLK_UPD_ADDR) | FT9001_CLK_UPD_GO_MASK, FT9001_CLK_UPD_ADDR);

	barrier_dsync_fence_full();
	barrier_isync_fence_full();
}
