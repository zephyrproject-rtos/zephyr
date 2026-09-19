/*
 * Copyright (c) 2021 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief System/hardware module for STM32U5 processor
 */

#include <zephyr/device.h>
#include <zephyr/cache.h>
#include <zephyr/init.h>
#include <stm32_ll_bus.h>
#include <stm32_ll_pwr.h>
#include <zephyr/logging/log.h>

#include <cmsis_core.h>

#define LOG_LEVEL CONFIG_SOC_LOG_LEVEL
LOG_MODULE_REGISTER(soc);

#define PWR_NODE DT_INST(0, st_stm32_pwr)

/* Helper to simplify following #if chain */
#define SELECTED_PSU(_x) DT_ENUM_HAS_VALUE(PWR_NODE, power_supply, _x)

#if SELECTED_PSU(ldo)
#define SELECTED_POWER_SUPPLY LL_PWR_LDO_SUPPLY
#elif SELECTED_PSU(smps)
#define SELECTED_POWER_SUPPLY LL_PWR_SMPS_SUPPLY
#endif

/*
 * Time out for the VCORE regulator switch, expressed the way ST's
 * HAL_PWREx_ConfigSupply() expresses it: a microsecond budget scaled by the
 * core clock into a bounded loop count. This code runs before the kernel and
 * before any clock configuration, so no timer API is available yet.
 */
#define REGULATOR_SWITCH_TIMEOUT_US 50U

/*
 * RM0456 requires PWR_SVMSR.REGS to be polled after writing PWR_CR3.REGSEL:
 * the switch is not instantaneous, and until REGS agrees with REGSEL the
 * supply actually feeding VCORE is the previous one. Without this wait the
 * clock driver goes straight on to raise the voltage scaling range, enable the
 * EPOD booster and lock the PLL against a regulator that may still be in
 * transition.
 */
static void stm32_wait_regulator_supply(uint32_t supply)
{
	uint32_t timeout =
		((REGULATOR_SWITCH_TIMEOUT_US * (SystemCoreClock / 1000U)) / 1000U) + 1U;
	const uint32_t expected = (supply == LL_PWR_SMPS_SUPPLY) ? 1U : 0U;

	while ((LL_PWR_IsActiveFlag_REGULATOR() != expected) && (timeout != 0U)) {
		timeout--;
	}

	if (LL_PWR_IsActiveFlag_REGULATOR() != expected) {
		/*
		 * The requested regulator never reported ready - on a board
		 * wired for the LDO, or one whose SMPS network is not fitted,
		 * SMPS can never become ready. Fall back to the LDO, which is
		 * the reset default and always available, rather than proceed
		 * against a supply whose state is unknown.
		 */
		LL_PWR_SetRegulatorSupply(LL_PWR_LDO_SUPPLY);
	}
}

extern void stm32_power_init(void);
/**
 * @brief Perform basic hardware initialization at boot.
 *
 * This needs to be run from the very beginning.
 */
void soc_early_init_hook(void)
{
	sys_cache_instr_enable();

	/* Update CMSIS SystemCoreClock variable (HCLK) */
	/* At reset, system core clock is set to 4 MHz from MSIS */
	SystemCoreClock = 4000000;

	/* Enable PWR */
	LL_AHB3_GRP1_EnableClock(LL_AHB3_GRP1_PERIPH_PWR);

	/* For devices with USB C PD, we can disable the dead battery
	 * pull-down behaviour.
	 */
#if defined(UCPD1)
	if (IS_ENABLED(CONFIG_DT_HAS_ST_STM32_UCPD_ENABLED) ||
		!IS_ENABLED(CONFIG_USB_DEVICE_DRIVER)) {
		/* Disable USB Type-C dead battery pull-down behavior */
		LL_PWR_DisableUCPDDeadBattery();
	}
#endif
#ifdef CONFIG_STM32_BACKUP_SRAM
	/*
	 * Enabling the Backup SRAM regulator is possible only when the LDO
	 * voltage regulator is selected, as is the case after reset. If the
	 * backup SRAM driver is enabled, make sure to enable the Backup SRAM
	 * regulator on its behalf before (potentially) switching to SMPS.
	 */
	LL_PWR_EnableBkUpRegulator();
	while (!LL_PWR_IsEnabledBkUpRegulator()) {
	}
#endif /* CONFIG_STM32_BACKUP_SRAM */

	/* Power Configuration */
	LL_PWR_SetRegulatorSupply(SELECTED_POWER_SUPPLY);
	stm32_wait_regulator_supply(SELECTED_POWER_SUPPLY);

#if CONFIG_PM
	stm32_power_init();
#endif
}
