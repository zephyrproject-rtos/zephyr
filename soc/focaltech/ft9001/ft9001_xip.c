/*
 * Copyright (c) 2025, FocalTech Systems CO.,Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/linker/sections.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/barrier.h>
#include <zephyr/sys/sys_io.h>
#include <string.h>
#include <stdint.h>

LOG_MODULE_REGISTER(ft9001_xip, CONFIG_LOG_DEFAULT_LEVEL);

/* addresses & masks (SoC-specific) */
#define FT9001_CLK_CFG_ADDR           0x40004008u
#define FT9001_CLK_UPD_ADDR           0x40004018u
#define FT9001_CLK_UPD_GO_MASK        0x02u

#define FT9001_GPIO_SDA_IN_ADDR       0x40017009u
#define FT9001_GPIO_SDA_LOW_MASK      0x02u

#define FT9001_DCACHE_BASE            0x40055000u
#define FT9001_DCACHE_EN_MASK         0x01u
#define FT9001_DCACHE_CPEA_OFF        0x180u
#define FT9001_DCACHE_CPES_OFF        0x184u
#define FT9001_DCACHE_CPES_GO_MASK    0x01u

#define FT9001_FLASH_FIRST_SECTOR     0x10000000u
#define FT9001_FLASH_CMD_SECTOR_ERASE 0x20u
#define FT9001_ROM_FLASH_ERASE_ADDR   (0x0400747Au | 1u)

/* Symbols provided by the linker script */
extern uint8_t __ramfunc_start[];
extern uint8_t __ramfunc_end[];
extern uint8_t __ramfunc_load_start[];

/**
 * @brief Short delay using NOPs.
 *
 * This is a simple delay function that uses NOP instructions to create a
 * short delay. The number of NOPs can be adjusted based on the required delay.
 */
static inline void short_delay_nops(int n)
{
	for (int i = 0; i < n; i++) {
		__NOP();
	}
}

/**
 * @brief Copy the .ramfunc section from flash to SRAM.
 *
 * Zephyr normally copies RAM-resident functions during boot, but this helper
 * can be invoked early in SoC init if a manual move is required.
 */
void copy_ramfunc(void)
{
	size_t size = (size_t)((uintptr_t)__ramfunc_end - (uintptr_t)__ramfunc_start);
	if (size == 0) {
		return;
	}

	memcpy(__ramfunc_start, __ramfunc_load_start, size);

	/* Small delay to ensure data is visible before execution */
	short_delay_nops(8);
}

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

	short_delay_nops(8);
}

/**
 * @brief  Check I2C_SDA and, if low, erase the first flash sector then jump
 *         back to ROM boot code.
 *
 * This routine must run from RAM (.ramfunc) because it erases the very first
 * sector at 0x1000 0000 while XIP is active.
 */
__ramfunc void xip_return_to_boot(void)
{
	/* Quick exit if SDA is high, nothing to do */
	if (sys_read8(FT9001_GPIO_SDA_IN_ADDR) & FT9001_GPIO_SDA_LOW_MASK) {
		return;
	}

	/* SDA low: start fallback sequence */
	uint32_t primask_backup = __get_PRIMASK();
	__disable_irq();

	/* 1. Invalidate D-Cache */
	if (sys_read32(FT9001_DCACHE_BASE) & FT9001_DCACHE_EN_MASK) {
		sys_write32(0x10000000u, FT9001_DCACHE_BASE + FT9001_DCACHE_CPEA_OFF);
		sys_write32(0x10001001u, FT9001_DCACHE_BASE + FT9001_DCACHE_CPES_OFF);
		while (sys_read32(FT9001_DCACHE_BASE + FT9001_DCACHE_CPES_OFF) &
		       FT9001_DCACHE_CPES_GO_MASK) {
			/* busy-wait */
		}
	}

	/* 2. Build erase descriptor */
	struct flash_cfg {
		uint8_t ssi_id;
		uint8_t sys_div;
		uint8_t is_qpi_mode;
		uint16_t stand_baudr;
		uint16_t quad_baudr;
		uint32_t rx_sample_delay;
		uint8_t cmd;
		uint32_t value;
		uint8_t is_mask_interrupt;
		uint8_t program_mode;
		uint32_t addr;
		uint16_t len;
		uint32_t buf;
		uint32_t delay;
		uint32_t timeout;
	};

	struct flash_cfg cfg = {
		.ssi_id = 1,
		.sys_div = 2,
		.is_qpi_mode = 0,
		.stand_baudr = 0x0002,
		.quad_baudr = 0x0002,
		.rx_sample_delay = 1,
		.cmd = FT9001_FLASH_CMD_SECTOR_ERASE,
		.value = 0,
		.is_mask_interrupt = 1,
		.program_mode = 0,
		.addr = FT9001_FLASH_FIRST_SECTOR,
		.len = 0,
		.buf = 0,
		.delay = 10,
		.timeout = 0xFFFFFFFFu,
	};

	/* 3. Call ROM helper (Thumb) */
	uintptr_t entry = FT9001_ROM_FLASH_ERASE_ADDR;
	uint8_t (*flash_erase)(struct flash_cfg *) = (void *)entry;

	(void)flash_erase(&cfg);

	/* Halt and wait for reset */
	__set_PRIMASK(primask_backup);
	while (true) {
		barrier_dsync_fence_full();
	}
}
