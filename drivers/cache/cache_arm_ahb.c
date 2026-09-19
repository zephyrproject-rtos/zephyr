/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Synaptics Incorporated
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/kernel.h>
#include <zephyr/drivers/cache.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/math_extras.h>

#define DT_DRV_COMPAT arm_ahb_cache

#define ARM_CACHE_CTRL        0x10
#define ARM_CACHE_CTRL_ENABLE BIT(0)

#define ARM_CACHE_MAINT_CTRL_ALL   0x20
#define ARM_CACHE_MAINT_CTRL_LINES 0x24
#define ARM_CACHE_TRIG_CLEAN       BIT(0)
#define ARM_CACHE_TRIG_INVALIDATE  BIT(1)

#define ARM_CACHE_MAINT_STATUS   0x28
#define ARM_CACHE_ONGOING_EN_DIS BIT(1)
#define ARM_CACHE_ONGOING_MAINT  BIT(2)

#define ARM_CACHE_LINE_SIZE 32

struct arm_cache_config {
	uintptr_t base;
};

static const struct arm_cache_config cache_cfg = {
	.base = DT_INST_REG_ADDR(0),
};

static void arm_cache_wait_maint(void)
{
	uint32_t val;

	do {
		val = sys_read32(cache_cfg.base + ARM_CACHE_MAINT_STATUS);
	} while (val & ARM_CACHE_ONGOING_MAINT);
}

static void arm_cache_wait_en_dis(void)
{
	uint32_t val;

	do {
		val = sys_read32(cache_cfg.base + ARM_CACHE_MAINT_STATUS);
	} while (val & ARM_CACHE_ONGOING_EN_DIS);
}

int cache_instr_flush_all(void)
{
	sys_write32(ARM_CACHE_TRIG_CLEAN, cache_cfg.base + ARM_CACHE_MAINT_CTRL_ALL);
	arm_cache_wait_maint();

	return 0;
}

int cache_instr_invd_all(void)
{
	sys_write32(ARM_CACHE_TRIG_INVALIDATE, cache_cfg.base + ARM_CACHE_MAINT_CTRL_ALL);
	arm_cache_wait_maint();

	return 0;
}

int cache_instr_flush_and_invd_all(void)
{
	sys_write32(ARM_CACHE_TRIG_CLEAN | ARM_CACHE_TRIG_INVALIDATE,
		    cache_cfg.base + ARM_CACHE_MAINT_CTRL_ALL);
	arm_cache_wait_maint();

	return 0;
}

static void arm_cache_flush_and_invd_range(uint32_t addr, uint32_t trig)
{
	sys_write32(addr | trig, cache_cfg.base + ARM_CACHE_MAINT_CTRL_LINES);

	arm_cache_wait_maint();
}

int cache_instr_flush_range(void *addr, size_t size)
{
	uint32_t cur_addr = (uint32_t)addr & ~0x1f;
	uint32_t end_addr = cur_addr + size;

	while (cur_addr <= end_addr) {
		arm_cache_flush_and_invd_range(cur_addr, ARM_CACHE_TRIG_CLEAN);
		cur_addr += ARM_CACHE_LINE_SIZE;
	}

	return 0;
}

int cache_instr_invd_range(void *addr, size_t size)
{
	uint32_t cur_addr = (uint32_t)addr & ~0x1f;
	uint32_t end_addr = cur_addr + size;

	while (cur_addr <= end_addr) {
		arm_cache_flush_and_invd_range(cur_addr, ARM_CACHE_TRIG_INVALIDATE);
		cur_addr += ARM_CACHE_LINE_SIZE;
	}

	return 0;
}

int cache_instr_flush_and_invd_range(void *addr, size_t size)
{
	uint32_t cur_addr = (uint32_t)addr & ~0x1f;
	uint32_t end_addr = cur_addr + size;

	while (cur_addr <= end_addr) {
		arm_cache_flush_and_invd_range(cur_addr,
					       ARM_CACHE_TRIG_CLEAN | ARM_CACHE_TRIG_INVALIDATE);
		cur_addr += ARM_CACHE_LINE_SIZE;
	}

	return 0;
}

void cache_instr_enable(void)
{
	uint32_t val;

	cache_instr_invd_all();

	val = sys_read32(cache_cfg.base + ARM_CACHE_CTRL);
	val |= ARM_CACHE_CTRL_ENABLE;
	sys_write32(val, cache_cfg.base + ARM_CACHE_CTRL);

	arm_cache_wait_en_dis();
}

void cache_instr_disable(void)
{
	uint32_t val;

	cache_instr_flush_all();

	val = sys_read32(cache_cfg.base + ARM_CACHE_CTRL);
	val &= ~ARM_CACHE_CTRL_ENABLE;
	sys_write32(val, cache_cfg.base + ARM_CACHE_CTRL);

	arm_cache_wait_en_dis();
}

#ifdef CONFIG_ICACHE_LINE_SIZE_DETECT
size_t cache_instr_line_size_get(void)
{
	if (sys_read32(cache_cfg.base + ARM_CACHE_CTRL) & ARM_CACHE_CTRL_ENABLE) {
		return ARM_CACHE_LINE_SIZE;
	}

	return 0;
}
#endif /* CONFIG_ICACHE_LINE_SIZE_DETECT */
