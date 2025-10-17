/*
 * Copyright (c) 2025 MASSDRIVER EI (massdriver.space)
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/cache.h>
#include <zephyr/logging/log.h>

#include <bflb_soc.h>
#include <hbn_reg.h>
#include <l1c_reg.h>
#include <zephyr/drivers/clock_control/clock_control_bflb_common.h>

#define DT_DRV_COMPAT bflb_l1c

#define INVLD_TIMEOUT 4194304

LOG_MODULE_REGISTER(cache_bflb_l1c, CONFIG_CACHE_LOG_LEVEL);

struct cache_config {
	uintptr_t base;
	uint32_t dcache_ways_mask;
};

static const struct cache_config cache_cfg = {
	.dcache_ways_mask = (1 << DT_INST_PROP(0, dcache_ways_disabled)) - 1,
	.base = DT_INST_REG_ADDR(0),
};

/* L1C supports basically a single type of invalidate & flush */
int bflb_cache_invalidate(uint32_t timeout)
{
	uint32_t tmp;

	tmp = *((volatile uint32_t *)(cache_cfg.base + L1C_CONFIG_OFFSET));
	tmp |= 1 << L1C_BYPASS_POS;
	tmp &= ~(1 << L1C_CACHEABLE_POS);
	tmp &= ~L1C_WAY_DIS_MSK;
	tmp &= ~(1 << L1C_CNT_EN_POS);
	*((volatile uint32_t *)(cache_cfg.base + L1C_CONFIG_OFFSET)) = tmp;

	tmp &= ~(1 << L1C_INVALID_EN_POS);
	*((volatile uint32_t *)(cache_cfg.base + L1C_CONFIG_OFFSET)) = tmp;
	__asm__ volatile (".rept 4 ; nop ; .endr");

	tmp |= (1 << L1C_INVALID_EN_POS);
	*((volatile uint32_t *)(cache_cfg.base + L1C_CONFIG_OFFSET)) = tmp;
	__asm__ volatile (".rept 4 ; nop ; .endr");

	while ((*((volatile uint32_t *)(cache_cfg.base + L1C_CONFIG_OFFSET))
		& L1C_INVALID_DONE_MSK) == 0 && timeout > 0) {
		clock_bflb_settle();
		clock_bflb_settle();
		timeout--;
	}

	if (!timeout) {
		return -EIO;
	}

#ifdef SOC_SERIES_BL70X
	tmp &= ~(1 << L1C_FLUSH_EN_POS);
	*((volatile uint32_t *)(cache_cfg.base + L1C_CONFIG_OFFSET)) = tmp;
	__asm__ volatile (".rept 4 ; nop ; .endr");
	tmp |= (1 << L1C_FLUSH_EN_POS);
	*((volatile uint32_t *)(cache_cfg.base + L1C_CONFIG_OFFSET)) = tmp;
	__asm__ volatile (".rept 4 ; nop ; .endr");

	while ((*((volatile uint32_t *)(cache_cfg.base + L1C_CONFIG_OFFSET))
		& L1C_FLUSH_DONE_MSK) == 0 && timeout > 0) {
		clock_bflb_settle();
		clock_bflb_settle();
		timeout--;
	}

	if (!timeout) {
		return -EIO;
	}

	tmp &= ~(1 << L1C_FLUSH_EN_POS);
	*((volatile uint32_t *)(cache_cfg.base + L1C_CONFIG_OFFSET)) = tmp;
#endif

	tmp = *((volatile uint32_t *)(cache_cfg.base + L1C_CONFIG_OFFSET));
	tmp |= 1 << L1C_BYPASS_POS;
	*((volatile uint32_t *)(cache_cfg.base + L1C_CONFIG_OFFSET)) = tmp;

	tmp &= ~(1 << L1C_BYPASS_POS);
	tmp |= 1 << L1C_CNT_EN_POS;
	*((volatile uint32_t *)(cache_cfg.base + L1C_CONFIG_OFFSET)) = tmp;

	tmp &= ~L1C_WAY_DIS_MSK;
	tmp |= cache_cfg.dcache_ways_mask << L1C_WAY_DIS_POS;
	*((volatile uint32_t *)(cache_cfg.base + L1C_CONFIG_OFFSET)) = tmp;

	tmp |= 1 << L1C_CACHEABLE_POS;
	*((volatile uint32_t *)(cache_cfg.base + L1C_CONFIG_OFFSET)) = tmp;

	clock_bflb_settle();
	clock_bflb_settle();
	return 0;
}

void cache_instr_enable(void)
{
	cache_data_enable();
}

void cache_data_enable(void)
{
	uint32_t tmp;

	tmp = *((volatile uint32_t *)(cache_cfg.base + L1C_CONFIG_OFFSET));
	tmp &= ~(1 << L1C_BYPASS_POS);
	tmp |= 1 << L1C_CNT_EN_POS;
	*((volatile uint32_t *)(cache_cfg.base + L1C_CONFIG_OFFSET)) = tmp;
	tmp &= ~L1C_WAY_DIS_MSK;
	tmp |= cache_cfg.dcache_ways_mask << L1C_WAY_DIS_POS;
	*((volatile uint32_t *)(cache_cfg.base + L1C_CONFIG_OFFSET)) = tmp;
	tmp |= 1 << L1C_CACHEABLE_POS;
	*((volatile uint32_t *)(cache_cfg.base + L1C_CONFIG_OFFSET)) = tmp;
}

void cache_instr_disable(void)
{
	cache_data_disable();
}

void cache_data_disable(void)
{
	uint32_t tmp;

	tmp = *((volatile uint32_t *)(cache_cfg.base + L1C_CONFIG_OFFSET));
	tmp |= 1 << L1C_BYPASS_POS;
	tmp &= ~(1 << L1C_CACHEABLE_POS);
	tmp &= ~L1C_WAY_DIS_MSK;
	tmp &= ~(1 << L1C_CNT_EN_POS);
	*((volatile uint32_t *)(cache_cfg.base + L1C_CONFIG_OFFSET)) = tmp;
}

int cache_data_invd_all(void)
{
	return bflb_cache_invalidate(INVLD_TIMEOUT);
}

int cache_data_invd_range(void *addr, size_t size)
{
	ARG_UNUSED(addr);
	ARG_UNUSED(size);

	return -ENOTSUP;
}

int cache_instr_invd_all(void)
{
	return bflb_cache_invalidate(INVLD_TIMEOUT);
}

int cache_instr_invd_range(void *addr, size_t size)
{
	ARG_UNUSED(addr);
	ARG_UNUSED(size);

	return -ENOTSUP;
}

int cache_data_flush_all(void)
{
	return bflb_cache_invalidate(INVLD_TIMEOUT);
}

int cache_data_flush_range(void *addr, size_t size)
{
	ARG_UNUSED(addr);
	ARG_UNUSED(size);

	return -ENOTSUP;
}

int cache_data_flush_and_invd_all(void)
{
	return bflb_cache_invalidate(INVLD_TIMEOUT);
}

int cache_data_flush_and_invd_range(void *addr, size_t size)
{
	ARG_UNUSED(addr);
	ARG_UNUSED(size);

	return -ENOTSUP;
}

int cache_instr_flush_all(void)
{
	return -ENOTSUP;
}

int cache_instr_flush_and_invd_all(void)
{
	return -ENOTSUP;
}

int cache_instr_flush_range(void *addr, size_t size)
{
	ARG_UNUSED(addr);
	ARG_UNUSED(size);

	return -ENOTSUP;
}

int cache_instr_flush_and_invd_range(void *addr, size_t size)
{
	ARG_UNUSED(addr);
	ARG_UNUSED(size);

	return -ENOTSUP;
}
