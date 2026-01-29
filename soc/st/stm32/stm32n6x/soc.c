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
#include <stm32_ll_pwr.h>
#include <stm32_ll_icache.h>

#include <cmsis_core.h>

#define LOG_LEVEL CONFIG_SOC_LOG_LEVEL
LOG_MODULE_REGISTER(soc);

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
	/* Enable the clock for the RIFSC (RIF Security Controller) */
	__HAL_RCC_RIFSC_CLK_ENABLE();

	/* DCMIPP */
	RIF_MASTER_CID1_SEC_PRIV(DCMIPP);
	RIF_SLAVE_SEC_PRIV(DCMIPP);
	/* DMA2D */
	RIF_MASTER_CID1_SEC_PRIV(DMA2D);
	RIF_SLAVE_SEC_PRIV(DMA2D);
	/* ETH */
	RIF_MASTER_CID1_SEC_PRIV(ETH1);
	RIF_SLAVE_SEC_PRIV(ETH1);
	/* JPEG */
	RIF_SLAVE_SEC_PRIV(JPEG);
	/* LTDC Layer 1 */
	RIF_MASTER_CID1_SEC_PRIV(LTDC1);
	RIF_SLAVE_SEC_PRIV(LTDCL1);
	/* NPU */
	RIF_MASTER_CID1_SEC_PRIV(NPU);
	RIF_SLAVE_SEC_PRIV(NPU);
	/* VENC */
	RIF_MASTER_CID1_SEC_PRIV(VENC);
	RIF_SLAVE_SEC_PRIV(VENC);
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

	/* Set the main internal Regulator output voltage for best performance */
	LL_PWR_SetRegulVoltageScaling(LL_PWR_REGU_VOLTAGE_SCALE0);

	/* Enable IOs */
	LL_PWR_EnableVddIO2();
	LL_PWR_EnableVddIO3();
	LL_PWR_EnableVddIO4();
	LL_PWR_EnableVddIO5();

	/* RIF configuration */
	if (IS_ENABLED(CONFIG_STM32N6_RIF_OPEN)) {
		soc_rif_config();
	}
}
