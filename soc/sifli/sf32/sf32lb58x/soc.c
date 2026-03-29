/*
 * Copyright (c) 2026 Qingsong Gou <gouqs@hotmail.com>
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/arch/cpu.h>
#include <zephyr/cache.h>
#include <zephyr/sys/util.h>

#define BOOTROM_BKP_REG             DT_REG_ADDR(DT_INST(0, sifli_sf32lb_rtc_backup))
#define BOOTROM_FLASH_OFF_DELAY_MSK GENMASK(11U, 4U)
#define BOOTROM_FLASH_ON_DELAY_MSK  GENMASK(23U, 12U)

void soc_early_init_hook(void)
{
	sys_cache_instr_enable();
	sys_cache_data_enable();

#if CONFIG_SOC_SF32LB58X_BOOTROM_FLASH_ON_DELAY_MS > 0 ||                                          \
	CONFIG_SOC_SF32LB58X_BOOTROM_FLASH_OFF_DELAY_MS > 0
	uint32_t val;

	val = sys_read32(BOOTROM_BKP_REG);
	val &= ~(BOOTROM_FLASH_OFF_DELAY_MSK | BOOTROM_FLASH_ON_DELAY_MSK);
	val |= FIELD_PREP(BOOTROM_FLASH_OFF_DELAY_MSK,
			  CONFIG_SOC_SF32LB58X_BOOTROM_FLASH_OFF_DELAY_MS) |
	       FIELD_PREP(BOOTROM_FLASH_ON_DELAY_MSK,
			  CONFIG_SOC_SF32LB58X_BOOTROM_FLASH_ON_DELAY_MS);
	sys_write32(val, BOOTROM_BKP_REG);
#endif
}
