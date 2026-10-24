/*
 * Copyright (c) 2019 Richard Osterloh <richard.osterloh@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief System/hardware module for STM32G4 processor
 */

#include <zephyr/device.h>
#include <zephyr/init.h>
#include <stm32_ll_bus.h>
#include <stm32_ll_rcc.h>
#include <stm32_ll_system.h>
#include <soc.h>

#include <cmsis_core.h>
#include <stm32_ll_pwr.h>

/* RCC reset values (RM0440) */
#define STM32G4_HSITRIM_RESET  0x40U
#define STM32G4_PLLCFGR_RESET  (16U << RCC_PLLCFGR_PLLN_Pos)
#define STM32G4_AHB1ENR_RESET  RCC_AHB1ENR_FLASHEN
#define STM32G4_APB1ENR1_RESET RCC_APB1ENR1_RTCAPBEN

#define STM32G4_FLASH_SR_ERRORS                                                                    \
	(FLASH_SR_OPERR | FLASH_SR_PROGERR | FLASH_SR_WRPERR | FLASH_SR_PGAERR | FLASH_SR_SIZERR | \
	 FLASH_SR_PGSERR | FLASH_SR_MISERR | FLASH_SR_FASTERR | FLASH_SR_RDERR | FLASH_SR_OPTVERR)

extern void stm32_power_init(void);

/*
 * The system bootloader starts the application with a jump, not a reset (AN2606). It resets the
 * peripherals it used but leaves RCC changed: HSITRIM, which offsets every clock derived from HSI16
 * and which the clock control driver does not set, HSI48, the PLL configuration and some clock
 * enables. It also leaves a pending interrupt, Range 1 boost mode, flash error flags, which fail
 * the next flash operation, and FLASH_ACR DBG_SWEN cleared. Return those to their reset values.
 */
static void stm32g4_system_bootloader_exit_reset(void)
{
	for (size_t i = 0U; i < ARRAY_SIZE(NVIC->ICER); i++) {
		NVIC->ICER[i] = 0xFFFFFFFFU;
		NVIC->ICPR[i] = 0xFFFFFFFFU;
	}

	SysTick->CTRL = 0U;
	SCB->ICSR = SCB_ICSR_PENDSTCLR_Msk;

	LL_RCC_HSI_Enable();
	while (LL_RCC_HSI_IsReady() == 0U) {
	}

	LL_RCC_WriteReg(CFGR, LL_RCC_SYS_CLKSOURCE_HSI);
	while (LL_RCC_GetSysClkSource() != LL_RCC_SYS_CLKSOURCE_STATUS_HSI) {
	}

	LL_RCC_HSI_SetCalibTrimming(STM32G4_HSITRIM_RESET);

	LL_RCC_WriteReg(CR, RCC_CR_HSION);
	while (LL_RCC_PLL_IsReady() != 0U) {
	}

	LL_RCC_WriteReg(PLLCFGR, STM32G4_PLLCFGR_RESET);
	LL_RCC_WriteReg(CIER, 0U);
	LL_RCC_HSI48_Disable();
	LL_RCC_WriteReg(CCIPR, 0U);
	LL_RCC_WriteReg(CCIPR2, 0U);

	LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_PWR);
	LL_PWR_DisableRange1BoostMode();
	WRITE_REG(FLASH->SR, STM32G4_FLASH_SR_ERRORS);
	SET_BIT(FLASH->ACR, FLASH_ACR_DBG_SWEN);

	LL_RCC_WriteReg(AHB1ENR, STM32G4_AHB1ENR_RESET);
	LL_RCC_WriteReg(AHB2ENR, 0U);
	LL_RCC_WriteReg(AHB3ENR, 0U);
	LL_RCC_WriteReg(APB1ENR1, STM32G4_APB1ENR1_RESET);
	LL_RCC_WriteReg(APB1ENR2, 0U);
	LL_RCC_WriteReg(APB2ENR, 0U);
}

/**
 * @brief Perform basic hardware initialization at boot.
 *
 * This needs to be run from the very beginning.
 */
void soc_early_init_hook(void)
{
	bool syscfg_clocked = LL_APB2_GRP1_IsEnabledClock(LL_APB2_GRP1_PERIPH_SYSCFG) != 0U;

	/* MEM_MODE selects system flash only after the system bootloader started this image */
	LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_SYSCFG);
	if (LL_SYSCFG_GetRemapMemory() == LL_SYSCFG_REMAP_SYSTEMFLASH) {
		stm32g4_system_bootloader_exit_reset();
	} else if (!syscfg_clocked) {
		LL_APB2_GRP1_DisableClock(LL_APB2_GRP1_PERIPH_SYSCFG);
	}

	/* Enable ART Accelerator I/D-cache and prefetch */
	LL_FLASH_EnableInstCache();
	LL_FLASH_EnableDataCache();
#if defined(CONFIG_STM32_FLASH_PREFETCH)
	LL_FLASH_EnablePrefetch();
#endif

	/* Update CMSIS SystemCoreClock variable (HCLK) */
	/* At reset, system core clock is set to 16 MHz from HSI */
	SystemCoreClock = 16000000;

	/* allow reflashing board */
	LL_DBGMCU_EnableDBGSleepMode();

#if defined(PWR_CR3_UCPD_DBDIS)
	if (IS_ENABLED(CONFIG_DT_HAS_ST_STM32_UCPD_ENABLED) ||
		!IS_ENABLED(CONFIG_USB_DEVICE_DRIVER)) {
		/* Disable USB Type-C dead battery pull-down behavior */
		LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_PWR);
		LL_PWR_DisableUCPDDeadBattery();
	}

#endif /* PWR_CR3_UCPD_DBDIS */
#if CONFIG_PM
	stm32_power_init();
#endif
}
