/*
 * Copyright (c) 2024 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief System/hardware module for STM32WB0 processor
 */

#include <zephyr/device.h>
#include <zephyr/init.h>
#include <stm32_ll_bus.h>
#include <stm32_ll_pwr.h>
#include <stm32_ll_system.h>
#include <stm32_ll_radio.h>
#include <zephyr/logging/log.h>
#include <zephyr/toolchain.h>
#include <cmsis_core.h>
#include <stdint.h>

#include <system_stm32wb0x.h>

#define LOG_LEVEL CONFIG_SOC_LOG_LEVEL
LOG_MODULE_REGISTER(soc);

/**
 * CMSIS System Core Clock: global variable holding the system core clock,
 * which is the frequency supplied to the SysTick timer and processor core.
 *
 * On STM32WB0 series, after RESET, the system clock frequency is 16MHz.
 */
uint32_t SystemCoreClock = 16000000U;

/**
 * RAM Virtual Register: special structure located at the start
 * of SRAM0; used by the UART bootloader and the Low Power Manager.
 * Data type definition comes from @ref system_stm32wb0xx.h
 */
Z_GENERIC_SECTION("stm32wb0_RAM_VR")
__used RAM_VR_TypeDef RAM_VR;

/** Power Controller node (shorthand for upcoming macros) */
#define PWRC DT_INST(0, st_stm32wb0_pwr)

/* Convert DTS properties to LL macros */
#define SMPS_PRESCALER	_CONCAT(LL_RCC_SMPS_DIV_, DT_PROP(PWRC, smps_clock_prescaler))

#if SMPS_MODE != STM32WB0_SMPS_MODE_OFF
	BUILD_ASSERT(DT_NODE_HAS_PROP(PWRC, smps_bom),
		"smps-bom must be specified");

	#define SMPS_BOM						\
		_CONCAT(LL_PWR_SMPS_BOM, DT_PROP(PWRC, smps_bom))

	#define SMPS_LP_MODE						\
			COND_CODE_1(					\
				DT_PROP(PWRC, smps_lp_floating),	\
					(LL_PWR_SMPS_LPOPEN),		\
					(LL_PWR_NO_SMPS_LPOPEN))

#if defined(PWR_CR5_SMPS_PRECH_CUR_SEL)
	#define SMPS_CURRENT_LIMIT					\
		_CONCAT(LL_PWR_SMPS_PRECH_LIMIT_CUR_,			\
			DT_STRING_UNQUOTED(PWRC, smps_current_limit))
#endif /* PWR_CR5_SMPS_PRECH_CUR_SEL */

	#define SMPS_OUTPUT_VOLTAGE					\
			_CONCAT(LL_PWR_SMPS_OUTPUT_VOLTAGE_,		\
			DT_STRING_UNQUOTED(PWRC, smps_output_voltage))
#endif /* SMPS_MODE != STM32WB0_SMPS_MODE_OFF */

static void configure_smps(void)
{
	/* Configure SMPS clock prescaler */
	LL_RCC_SetSMPSPrescaler(SMPS_PRESCALER);

#if SMPS_MODE == STM32WB0_SMPS_MODE_OFF
	/* Disable SMPS */
	LL_PWR_SetSMPSMode(LL_PWR_NO_SMPS);

	while (LL_PWR_IsSMPSReady()) {
		/* Wait for SMPS to turn off */
	}
#else
	/* Select correct BOM */
	LL_PWR_SetSMPSBOM(SMPS_BOM);

	/* Configure low-power mode */
	LL_PWR_SetSMPSOpenMode(SMPS_LP_MODE);

	/* Enable SMPS */
	LL_PWR_SetSMPSMode(LL_PWR_SMPS);

	while (!LL_PWR_IsSMPSReady()) {
		/* Wait for SMPS to turn on */
	}

	/* Place SMPS in PRECHARGE (BYPASS) mode.
	 * This is required to change SMPS output voltage,
	 * so we can do it unconditionally.
	 */
	LL_PWR_SetSMPSPrechargeMode(LL_PWR_SMPS_PRECHARGE);
	while (LL_PWR_IsSMPSinRUNMode()) {
		/* Wait for SMPS to enter PRECHARGE mode */
	}

	if (SMPS_MODE == STM32WB0_SMPS_MODE_PRECHARGE) {
#if defined(PWR_CR5_SMPS_PRECH_CUR_SEL)
		/**
		 * SMPS should remain in PRECHARGE mode.
		 * We still have to configure the output current
		 * limit specified in Device Tree, though this
		 * can only be done if this SoC supports it.
		 */
		LL_PWR_SetSMPSPrechargeLimitCurrent(SMPS_CURRENT_LIMIT);
#endif /* PWR_CR5_SMPS_PRECH_CUR_SEL */
	} else {
		/**
		 * SMPS mode requested is RUN mode. Configure the output
		 * voltage to the desired value then exit PRECHARGE mode.
		 */
		LL_PWR_SMPS_SetOutputVoltageLevel(SMPS_OUTPUT_VOLTAGE);

		/* Exit PRECHARGE mode (returns in RUN mode) */
		LL_PWR_SetSMPSPrechargeMode(LL_PWR_NO_SMPS_PRECHARGE);
		while (!LL_PWR_IsSMPSinRUNMode()) {
			/* Wait for SMPS to enter RUN mode */
		}
	}
#endif /* SMPS_MODE == STM32WB0_SMPS_MODE_OFF */
}

/**
 * @brief Perform basic hardware initialization at boot.
 *
 * This needs to be run from the very beginning,
 * so the init priority has to be 0 (zero).
 *
 * @return 0
 */
static int stm32wb0_init(void)
{
	/* Update CMSIS SystemCoreClock variable (CLK_SYS) */
	/* On reset, the 64MHz HSI is selected as input to
	 * the SYSCLKPRE prescaler, set to 4, resulting in
	 * CLK_SYS being equal to 16MHz.
	 */
	SystemCoreClock = 16000000U;

	/* Remap address 0 to user flash memory */
	LL_SYSCFG_SetRemapMemory(LL_SYSCFG_REMAP_FLASH);

	/**
	 * Save application exception vector address in RAM_VR.
	 * By now, SCB->VTOR should point to _vector_table,
	 * so use that value instead of _vector_table directly.
	 */
	RAM_VR.AppBase = SCB->VTOR;

	/* Enable retention of all RAM banks in Deepstop */
	LL_PWR_EnableRAMBankRet(LL_PWR_RAMRET_1);
#if defined(LL_PWR_RAMRET_2)
	LL_PWR_EnableRAMBankRet(LL_PWR_RAMRET_2);
#endif
#if defined(LL_PWR_RAMRET_3)
	LL_PWR_EnableRAMBankRet(LL_PWR_RAMRET_3);
#endif

	/* Configure SMPS step-down converter */
	configure_smps();

	return 0;
}

SYS_INIT(stm32wb0_init, PRE_KERNEL_1, 0);
