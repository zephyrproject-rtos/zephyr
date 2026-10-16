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
#include <zephyr/dt-bindings/gpio/gpio.h>

#define LOG_LEVEL CONFIG_SOC_LOG_LEVEL
LOG_MODULE_REGISTER(soc);

#define PWR_NODE                 DT_INST(0, st_stm32n6_pwr)
#define PWR_EXT_REG_CONTROL_GPIO DT_PROP_HAS_IDX(PWR_NODE, st_external_control_gpios, 0)
#define PWR_EXT_REGULATOR        DT_ENUM_HAS_VALUE(PWR_NODE, power_supply, external_source)
#define PWR_EXT_REG_CONTROL      (PWR_EXT_REGULATOR && PWR_EXT_REG_CONTROL_GPIO)
#define USE_VOLTAGE_SCALE0       (CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC > MHZ(600))

#if PWR_EXT_REG_CONTROL
#define PWR_EXT_PIN_GPIO_HANDLE  DT_GPIO_CTLR_BY_IDX(PWR_NODE, st_external_control_gpios, 0)
#define PWR_EXT_PIN_NUMBER       DT_GPIO_PIN_BY_IDX(PWR_NODE, st_external_control_gpios, 0)
#define PWR_EXT_PIN_FLAGS        DT_GPIO_FLAGS_BY_IDX(PWR_NODE, st_external_control_gpios, 0)
#define PWR_EXT_PIN_GPIO_ADDRESS DT_REG_ADDR_BY_IDX_RAW(PWR_EXT_PIN_GPIO_HANDLE, 0)
#define PWR_EXT_PIN_CLOCK_ENABLE DT_PHA_BY_IDX(PWR_EXT_PIN_GPIO_HANDLE, clocks, 0, bits)

/* Check that st,external-control-gpios points to internal GPIO */
BUILD_ASSERT(IN_RANGE(PWR_EXT_PIN_GPIO_ADDRESS, DT_REG_ADDR(DT_NODELABEL(pinctrl)),
		      DT_REG_ADDR(DT_NODELABEL(pinctrl)) + DT_REG_SIZE(DT_NODELABEL(pinctrl))),
	     "Only internal GPIOx is supported by st,external-control-gpios property");
#endif /* PWR_EXT_REG_CONTROL */

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
__maybe_unused static void soc_configure_voltage_scale_0(void)
{
#if PWR_EXT_REG_CONTROL
	/* We are in early stage of initialization so we can't rely on
	 * Zephyr GPIO driver
	 */
	uint32_t pin = BIT(PWR_EXT_PIN_NUMBER);
	GPIO_TypeDef *gpiox = (GPIO_TypeDef *)PWR_EXT_PIN_GPIO_ADDRESS;

	LL_AHB4_GRP1_EnableClock(PWR_EXT_PIN_CLOCK_ENABLE);

	LL_GPIO_SetPinMode(gpiox, pin, LL_GPIO_MODE_OUTPUT);
	LL_GPIO_SetPinOutputType(gpiox, pin, LL_GPIO_OUTPUT_PUSHPULL);

#if (PWR_EXT_PIN_FLAGS & GPIO_ACTIVE_LOW)
	LL_GPIO_ResetOutputPin(gpiox, pin);
#else
	LL_GPIO_SetOutputPin(gpiox, pin);
#endif /* (PWR_EXT_PIN_FLAGS & GPIO_ACTIVE_LOW) == GPIO_ACTIVE_LOW */
#endif /* PWR_EXT_REG_CONTROL */
	/* Select VOS0 scale ("VOS high") for best performance.
	 * When an external source is used, setting this bit has no effect
	 * but do it anyways to keep track of the fact we want to be in VOS0.
	 */
	LL_PWR_SetRegulVoltageScaling(LL_PWR_REGU_VOLTAGE_SCALE0);
#if !PWR_EXT_REG_CONTROL
	/* Wait for internal SMPS to be ready */
	while (!LL_PWR_IsActiveFlag_ACTVOSRDY()) {
	}
#endif
}

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

#if PWR_EXT_REGULATOR
	/* Disable internal SMPS */
	LL_PWR_ConfigSupply(LL_PWR_EXTERNAL_SOURCE_SUPPLY);
#endif /* PWR_EXT_REGULATOR */
#if USE_VOLTAGE_SCALE0
	soc_configure_voltage_scale_0();
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
