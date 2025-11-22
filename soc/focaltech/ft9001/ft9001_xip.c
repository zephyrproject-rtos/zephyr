/*
 * Copyright (c) 2025, FocalTech Systems CO.,Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/linker/sections.h>
#include <zephyr/linker/linker-defs.h>
#include <zephyr/sys/sys_io.h>
#include <cmsis_core.h>
#include <stdint.h>

/* clocks (soc-specific) */
#define FT9001_CLK_CFG_ADDR    0x40004008u
#define FT9001_CLK_UPD_ADDR    0x40004018u
#define FT9001_CLK_UPD_GO_MASK 0x02u

/**
 * @brief Switch XIP clock divider.
 *
 * @param clk_div  Lower four bits are the divider value.
 */
__ramfunc void xip_clock_switch(uint32_t clk_div)
{
	uint32_t val = sys_read32(FT9001_CLK_CFG_ADDR);

	val = (val & 0xFFFFFFF0u) | (clk_div & 0x0Fu);
	sys_write32(val, FT9001_CLK_CFG_ADDR);
	sys_write32(sys_read32(FT9001_CLK_UPD_ADDR) | FT9001_CLK_UPD_GO_MASK, FT9001_CLK_UPD_ADDR);

	__DSB();
	__ISB();
}
