/*
 * Copyright (c) 2024 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief System/hardware module for STM32N6 processor
 */

#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/cache.h>
#include <zephyr/logging/log.h>

#include <stm32_ll_bus.h>
#include <stm32_ll_gpio.h>
#include <stm32_ll_pwr.h>
#include <stm32_ll_icache.h>

#include <cmsis_core.h>
#include <zephyr/dt-bindings/pinctrl/stm32-pinctrl.h>

#define LOG_LEVEL CONFIG_SOC_LOG_LEVEL
LOG_MODULE_REGISTER(soc);

#define PWR_NODE                  DT_INST(0, st_stm32n6_pwr)
#define PWR_EXT_SMPS_CONTROL_GPIO DT_PROP(PWR_NODE, external_control_pin)
#define PWR_EXT_REGULATOR         DT_ENUM_HAS_VALUE(PWR_NODE, power_supply, external_source)
#define PWR_EXT_SMPS_CONTROL                                                                       \
	(PWR_EXT_REGULATOR &&                                                                      \
	 (((PWR_EXT_SMPS_CONTROL_GPIO >> STM32_MODE_SHIFT) & STM32_MODE_MASK) == STM32_GPIO))
#define USE_VOLTAGE_SCALE0 CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC > MHZ(600)

extern char _vector_start[];
void *g_pfnVectors = (void *)_vector_start;

#if defined(CONFIG_SOC_RESET_HOOK)
void soc_reset_hook(void)
{
	/* This is provided by STM32Cube HAL */
	SystemInit();
}
#endif

#define RIF_MASTER_CID1_SEC_PRIV(device)	\
	do {										\
		RIMC_MasterConfig_t rimc = {						\
			.MasterCID = RIF_CID_1,						\
			.SecPriv = RIF_ATTRIBUTE_SEC | RIF_ATTRIBUTE_PRIV,		\
		};									\
		HAL_RIF_RIMC_ConfigMasterAttributes(RIF_MASTER_INDEX_##device, &rimc);	\
	} while (0)

#define RIF_SLAVE_SEC_PRIV(device)	\
	HAL_RIF_RISC_SetSlaveSecureAttributes(RIF_RISC_PERIPH_INDEX_##device,		\
					      RIF_ATTRIBUTE_SEC | RIF_ATTRIBUTE_PRIV)

static void soc_rif_config(void)
{
#if defined(CONFIG_TRUSTED_EXECUTION_SECURE)
	/* Enable the clock for the RIFSC (RIF Security Controller) */
	__HAL_RCC_RIFSC_CLK_ENABLE();

	/* ADC */
	RIF_SLAVE_SEC_PRIV(ADC12);
	/* DCMIPP */
	RIF_MASTER_CID1_SEC_PRIV(DCMIPP);
	RIF_SLAVE_SEC_PRIV(DCMIPP);
	/* DMA2D */
	RIF_MASTER_CID1_SEC_PRIV(DMA2D);
	RIF_SLAVE_SEC_PRIV(DMA2D);
	/* GPU2D */
	RIF_MASTER_CID1_SEC_PRIV(GPU2D);
	RIF_SLAVE_SEC_PRIV(GPU2D);
	/* ETH */
	RIF_MASTER_CID1_SEC_PRIV(ETH1);
	RIF_SLAVE_SEC_PRIV(ETH1);
	/* JPEG */
	RIF_SLAVE_SEC_PRIV(JPEG);
	/* LTDC Layer 1 */
	RIF_MASTER_CID1_SEC_PRIV(LTDC1);
	RIF_SLAVE_SEC_PRIV(LTDCL1);
#ifdef NPU_PRESENT
	/* NPU */
	RIF_MASTER_CID1_SEC_PRIV(NPU);
	RIF_SLAVE_SEC_PRIV(NPU);
#endif
	/* VENC */
	RIF_MASTER_CID1_SEC_PRIV(VENC);
	RIF_SLAVE_SEC_PRIV(VENC);
#endif /* CONFIG_TRUSTED_EXECUTION_SECURE */
}

/* Voltage scale 0 (VOS0) needs to be configured
 * for CPU frequency above 600 MHz
 */
#if USE_VOLTAGE_SCALE0
static void soc_configure_voltage_scale(void)
{
#if PWR_EXT_REGULATOR
	/* Disable internal SMPS */
	LL_PWR_ConfigSupply(LL_PWR_EXTERNAL_SOURCE_SUPPLY);

#if PWR_EXT_SMPS_CONTROL
	/* We are in early stage of initialization so we can't rely on
	 * Zephyr GPIO driver
	 */
	uint32_t port = (PWR_EXT_SMPS_CONTROL_GPIO >> STM32_PORT_SHIFT) & STM32_PORT_MASK;
	uint32_t pin = (1 << ((PWR_EXT_SMPS_CONTROL_GPIO >> STM32_LINE_SHIFT) & STM32_LINE_MASK));
	GPIO_TypeDef *GPIOx = (GPIO_TypeDef *)(GPIOA_BASE + (GPIOB_BASE - GPIOA_BASE) * port);

	LL_AHB4_GRP1_EnableClock(LL_AHB4_GRP1_PERIPH_GPIOA << port);

	LL_GPIO_SetPinMode(GPIOx, pin, LL_GPIO_MODE_OUTPUT);
	LL_GPIO_SetPinOutputType(GPIOx, pin, LL_GPIO_OUTPUT_PUSHPULL);
	LL_GPIO_SetOutputPin(GPIOx, pin);
#endif /* PWR_EXT_SMPS_CONTROL */
#endif /* PWR_EXT_REGULATOR */
	/* Set the main internal Regulator output voltage for best performance.
	 * Even when ext. SMPS is used, the bit can be used to keep track.
	 */
	LL_PWR_SetRegulVoltageScaling(LL_PWR_REGU_VOLTAGE_SCALE0);
}
#endif /* USE_VOLTAGE_SCALE0 */

/**
 * @brief Perform basic hardware initialization at boot.
 *
 * This needs to be run from the very beginning.
 *
 * @return 0
 */
void soc_early_init_hook(void)
{
	/* Enable caches */
	sys_cache_instr_enable();
	sys_cache_data_enable();

	/* Update CMSIS SystemCoreClock variable (HCLK) */
	/* At reset, system core clock is set to 64 MHz from HSI */
	SystemCoreClock = 64000000;

	/* Enable PWR */
	LL_AHB4_GRP1_EnableClock(LL_AHB4_GRP1_PERIPH_PWR);

#if USE_VOLTAGE_SCALE0
	soc_configure_voltage_scale();
#endif

	/* Enable IOs */
	LL_PWR_EnableVddIO2();
	LL_PWR_EnableVddIO3();
	LL_PWR_EnableVddIO4();
	LL_PWR_EnableVddIO5();

	/* RIF configuration */
	if (IS_ENABLED(CONFIG_STM32N6_RIF_OPEN)) {
		soc_rif_config();
	}

	if (IS_ENABLED(CONFIG_STM32N6_BRANCH_CACHE)) {
		/*
		 * Enable the Cortex-M55's branch cache.
		 * CCR.BP is banked between Security states
		 * so this must be done in every environment.
		 */
		SCB->CCR |= SCB_CCR_LOB_Msk;
		__DSB();
		__ISB();
	}
}
