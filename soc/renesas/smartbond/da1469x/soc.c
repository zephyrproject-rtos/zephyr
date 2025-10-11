/*
 * Copyright (c) 2022 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/init.h>
#include <zephyr/linker/linker-defs.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/reboot.h>
#include <string.h>
#include <DA1469xAB.h>
#include <da1469x_clock.h>
#include <da1469x_otp.h>
#include <da1469x_pd.h>
#include <da1469x_pdc.h>
#include <da1469x_trimv.h>
#include <da1469x_sleep.h>

LOG_MODULE_REGISTER(soc, CONFIG_SOC_LOG_LEVEL);

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
	if (type == SYS_REBOOT_WARM) {
		NVIC_SystemReset();
	} else if (type == SYS_REBOOT_COLD) {
		if ((SYS_WDOG->WATCHDOG_REG & SYS_WDOG_WATCHDOG_REG_WDOG_VAL_NEG_Msk) == 0) {
			/* Cannot write WATCHDOG_REG while WRITE_BUSY */
			while ((SYS_WDOG->WATCHDOG_REG &
				SYS_WDOG_WATCHDOG_CTRL_REG_WRITE_BUSY_Msk) != 0) {
			}
			/* Write WATCHDOG_REG */
			SYS_WDOG->WATCHDOG_REG = BIT(SYS_WDOG_WATCHDOG_REG_WDOG_VAL_Pos);

			GPREG->RESET_FREEZE_REG = GPREG_SET_FREEZE_REG_FRZ_SYS_WDOG_Msk;
			SYS_WDOG->WATCHDOG_CTRL_REG &=
				~SYS_WDOG_WATCHDOG_CTRL_REG_WDOG_FREEZE_EN_Msk;
		}
		/* Wait */
		for (;;) {
			__NOP();
		}
	}
}

#if defined(CONFIG_BOOTLOADER_MCUBOOT)
static void z_renesas_configure_cache(void)
{
	/* Cache region should reflect both MCU boot and slots partition areas */
	uint32_t region_size = (uint32_t)&__text_region_end - CONFIG_FLASH_BASE_ADDRESS;
	uint32_t reg_region_size;
	uint32_t reg_cache_len;

	if (z_renesas_cache_configured == MAGIC) {
		return;
	}

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

	/* Flash base and offset fields should have already been configured by ROM booter. */
	CACHE->CACHE_FLASH_REG =
		(CACHE->CACHE_FLASH_REG & ~CACHE_CACHE_FLASH_REG_FLASH_REGION_SIZE_Msk) |
		reg_region_size;

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

void soc_reset_hook(void)
{
#if defined(CONFIG_PM)
	uint32_t *ivt;
#endif

#if defined(CONFIG_BOOTLOADER_MCUBOOT)
	z_renesas_configure_cache();
#endif

#if defined(CONFIG_PM)
	/* IVT is always placed in reserved space at the start of RAM which
	 * is then remapped to 0x0 and retained. Generic reset handler is
	 * changed to custom routine since next time ARM core is reset we
	 * need to determine whether it was a regular reset or a wakeup from
	 * extended sleep and ARM core state needs to be restored.
	 */
	ivt = (uint32_t *)_image_ram_start;
	ivt[1] = (uint32_t)da1469x_wakeup_handler;
#endif
}

/* defined in power.c */
extern int renesas_da1469x_pm_init(void);

void soc_early_init_hook(void)
{
	/* Freeze watchdog until configured */
	GPREG->SET_FREEZE_REG = GPREG_SET_FREEZE_REG_FRZ_SYS_WDOG_Msk;
	SYS_WDOG->WATCHDOG_REG = SYS_WDOG_WATCHDOG_REG_WDOG_VAL_Msk;

	/* Reset clock dividers to 0 */
	CRG_TOP->CLK_AMBA_REG &= ~(CRG_TOP_CLK_AMBA_REG_HCLK_DIV_Msk |
				CRG_TOP_CLK_AMBA_REG_PCLK_DIV_Msk);

	CRG_TOP->PMU_CTRL_REG |= (CRG_TOP_PMU_CTRL_REG_TIM_SLEEP_Msk   |
				CRG_TOP_PMU_CTRL_REG_PERIPH_SLEEP_Msk  |
				CRG_TOP_PMU_CTRL_REG_COM_SLEEP_Msk     |
				CRG_TOP_PMU_CTRL_REG_RADIO_SLEEP_Msk);

#if defined(CONFIG_PM)
	/* Enable cache retainability */
	CRG_TOP->PMU_CTRL_REG |= CRG_TOP_PMU_CTRL_REG_RETAIN_CACHE_Msk;
#endif

	/*
	 *	Due to crosstalk issues any power rail can potentially
	 *	issue a fake event. This is typically observed upon
	 *	switching power sources, that is DCDC <--> LDOs <--> Retention LDOs.
	 */
	CRG_TOP->BOD_CTRL_REG &= ~(CRG_TOP_BOD_CTRL_REG_BOD_V14_EN_Msk |
				CRG_TOP_BOD_CTRL_REG_BOD_V18F_EN_Msk   |
				CRG_TOP_BOD_CTRL_REG_BOD_VDD_EN_Msk    |
				CRG_TOP_BOD_CTRL_REG_BOD_V18P_EN_Msk   |
				CRG_TOP_BOD_CTRL_REG_BOD_V18_EN_Msk    |
				CRG_TOP_BOD_CTRL_REG_BOD_V30_EN_Msk    |
				CRG_TOP_BOD_CTRL_REG_BOD_VBAT_EN_Msk);

	da1469x_otp_init();
	da1469x_trimv_init_from_otp();

	da1469x_pd_init();

	/*
	 * Take PD_SYS control.
	 */
	da1469x_pd_acquire(MCU_PD_DOMAIN_SYS);
	da1469x_pd_acquire(MCU_PD_DOMAIN_TIM);

	da1469x_pdc_reset();
#if CONFIG_PM
	renesas_da1469x_pm_init();
#endif
}
