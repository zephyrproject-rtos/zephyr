/*
 * Copyright (c) 2022 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/init.h>
#include <zephyr/arch/arm/aarch32/cortex_m/cmsis.h>
#include <zephyr/arch/arm/aarch32/nmi.h>
#include <zephyr/linker/linker-defs.h>
#include <string.h>

#define REMAP_ADR0_QSPI           0x2

#define FLASH_REGION_SIZE_32M     0
#define FLASH_REGION_SIZE_16M     1
#define FLASH_REGION_SIZE_8M      2
#define FLASH_REGION_SIZE_4M      3
#define FLASH_REGION_SIZE_2M      4
#define FLASH_REGION_SIZE_1M      5
#define FLASH_REGION_SIZE_05M     6
#define FLASH_REGION_SIZE_025M    7

#if defined(CONFIG_BOOTLOADER_MCUBOOT)
#define MAGIC 0xaabbccdd
static uint32_t z_renesas_cache_configured;
#endif

void sys_arch_reboot(int type)
{
	ARG_UNUSED(type);

	NVIC_SystemReset();
}

#if defined(CONFIG_BOOTLOADER_MCUBOOT)
static void z_renesas_configure_cache(void)
{
	uint32_t cache_start;
	uint32_t region_size;
	uint32_t reg_region_size;
	uint32_t reg_cache_len;

	if (z_renesas_cache_configured == MAGIC) {
		return;
	}

	cache_start = (uint32_t)&_vector_start;
	region_size = (uint32_t)&__rom_region_end - cache_start;

	/* Disable cache before configuring it */
	CACHE->CACHE_CTRL2_REG = 0;
	CRG_TOP->SYS_CTRL_REG &= ~CRG_TOP_SYS_CTRL_REG_CACHERAM_MUX_Msk;

	/* Disable MRM unit */
	CACHE->CACHE_MRM_CTRL_REG = 0;
	CACHE->CACHE_MRM_TINT_REG = 0;
	CACHE->CACHE_MRM_MISSES_THRES_REG = 0;

	if (region_size > MB(16)) {
		reg_region_size = FLASH_REGION_SIZE_32M;
	} else if (region_size > MB(8)) {
		reg_region_size = FLASH_REGION_SIZE_16M;
	} else if (region_size > MB(4)) {
		reg_region_size = FLASH_REGION_SIZE_8M;
	} else if (region_size > MB(2)) {
		reg_region_size = FLASH_REGION_SIZE_4M;
	} else if (region_size > MB(1)) {
		reg_region_size = FLASH_REGION_SIZE_2M;
	} else if (region_size > KB(512)) {
		reg_region_size = FLASH_REGION_SIZE_1M;
	} else if (region_size > KB(256)) {
		reg_region_size = FLASH_REGION_SIZE_05M;
	} else {
		reg_region_size = FLASH_REGION_SIZE_025M;
	}
	CACHE->CACHE_FLASH_REG =
		(cache_start >> 16) << CACHE_CACHE_FLASH_REG_FLASH_REGION_BASE_Pos |
		((cache_start & 0xffff) >> 2) << CACHE_CACHE_FLASH_REG_FLASH_REGION_OFFSET_Pos |
		reg_region_size << CACHE_CACHE_FLASH_REG_FLASH_REGION_SIZE_Pos;

	reg_cache_len = CLAMP(reg_region_size / KB(64), 0, 0x1ff);
	CACHE->CACHE_CTRL2_REG = (CACHE->CACHE_FLASH_REG & ~CACHE_CACHE_CTRL2_REG_CACHE_LEN_Msk) |
				 reg_cache_len;

	/* Copy IVT from flash to start of RAM.
	 * It will be remapped at 0x0 so it can be used after SW Reset
	 */
	memcpy(&_image_ram_start, &_vector_start, 0x200);

	/* Configure remapping */
	CRG_TOP->SYS_CTRL_REG = (CRG_TOP->SYS_CTRL_REG & ~CRG_TOP_SYS_CTRL_REG_REMAP_ADR0_Msk) |
				CRG_TOP_SYS_CTRL_REG_CACHERAM_MUX_Msk |
				CRG_TOP_SYS_CTRL_REG_REMAP_INTVECT_Msk |
				REMAP_ADR0_QSPI;

	z_renesas_cache_configured = MAGIC;

	/* Trigger SW Reset to apply cache configuration */
	CRG_TOP->SYS_CTRL_REG |= CRG_TOP_SYS_CTRL_REG_SW_RESET_Msk;
}
#endif /* CONFIG_HAS_FLASH_LOAD_OFFSET */

void z_arm_platform_init(void)
{
#if defined(CONFIG_BOOTLOADER_MCUBOOT)
	z_renesas_configure_cache();
#endif
}

static int renesas_da14699_init(void)
{

	NMI_INIT();

	/* Freeze watchdog until configured */
	GPREG->SET_FREEZE_REG = GPREG_SET_FREEZE_REG_FRZ_SYS_WDOG_Msk;
	/* Reset clock dividers to 0 */
	CRG_TOP->CLK_AMBA_REG &= ~(CRG_TOP_CLK_AMBA_REG_HCLK_DIV_Msk |
				   CRG_TOP_CLK_AMBA_REG_PCLK_DIV_Msk);
	/* Enable all power domains except for radio */
	CRG_TOP->PMU_CTRL_REG = 0x02;

	return 0;
}

SYS_INIT(renesas_da14699_init, PRE_KERNEL_1, 0);
