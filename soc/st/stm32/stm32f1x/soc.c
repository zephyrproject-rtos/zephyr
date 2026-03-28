/*
 * Copyright (c) 2016 Open-RnD Sp. z o.o.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief System/hardware module for STM32F1 processor
 */

#include <zephyr/device.h>
#include <zephyr/init.h>

#include <stm32_ll_system.h>
#include <stm32_ll_bus.h>
#include <stm32_ll_gpio.h>

#include <cmsis_core.h>

/**
 * Does pinctrl property `swj-cfg` have value @p v ?
 */
#define SWJ_CFG_HAS_VALUE(v) DT_ENUM_HAS_VALUE(DT_NODELABEL(pinctrl), swj_cfg, v)

/**
 * @brief Perform basic hardware initialization at boot.
 *
 * This needs to be run from the very beginning.
 */
void soc_early_init_hook(void)
{
#if defined(FLASH_ACR_PRFTBE) && defined(CONFIG_STM32_FLASH_PREFETCH)
	/* Enable ART Accelerator prefetch */
	LL_FLASH_EnablePrefetch();
#endif

	/* Update CMSIS SystemCoreClock variable (HCLK) */
	/* At reset, system core clock is set to 8 MHz from HSI */
	SystemCoreClock = 8000000;

#if defined(CONFIG_PM) || defined(CONFIG_POWEROFF)
	LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_PWR);
#endif

/*
 * Configure SWD-JTAG port if a non-default value is selected.
 * The default value 'full' corresponds to hardware reset value:
 * 000: Full SJW (JTAG-DP + SW-DP)
 */
#if !SWJ_CFG_HAS_VALUE(full)
	LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_AFIO);

#if SWJ_CFG_HAS_VALUE(no_njtrst)
	/* 001: Full SWJ (JTAG-DP + SW-DP) but without NJTRST */
	/* releases: PB4 */
	LL_GPIO_AF_Remap_SWJ_NONJTRST();
#elif SWJ_CFG_HAS_VALUE(jtag_disable)
	/* 010: JTAG-DP Disabled and SW-DP Enabled */
	/* releases: PB4 PB3 PA15 */
	LL_GPIO_AF_Remap_SWJ_NOJTAG();
#elif SWJ_CFG_HAS_VALUE(disable)
	/* 100: JTAG-DP Disabled and SW-DP Disabled */
	/* releases: PB4 PB3 PA13 PA14 PA15 */
	LL_GPIO_AF_DisableRemap_SWJ();
#else
#error Unreachable?!
#endif /* SWJ_CFG_HAS_VALUE(...) */
#endif /* !SWJ_CFG_HAS_VALUE(full) */
}
