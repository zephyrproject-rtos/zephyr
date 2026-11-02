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

#if DT_NODE_HAS_STATUS_OKAY(DT_CHOSEN(zephyr_itcm)) &&                                             \
	!DT_SAME_NODE(DT_CHOSEN(zephyr_itcm), DT_CHOSEN(zephyr_flash))
#define ITCM_BASE DT_REG_ADDR(DT_CHOSEN(zephyr_itcm))
#define ITCM_END  (DT_REG_ADDR(DT_CHOSEN(zephyr_itcm)) + DT_REG_SIZE(DT_CHOSEN(zephyr_itcm)))

BUILD_ASSERT((ITCM_BASE & __alignof__(uint32_t)) == 0, "ITCM start address must be 32-bit aligned");
BUILD_ASSERT((ITCM_END & __alignof__(uint32_t)) == 0, "ITCM size must be 32-bit aligned");
#endif
#if DT_NODE_HAS_STATUS_OKAY(DT_CHOSEN(zephyr_dtcm)) &&                                             \
	!DT_SAME_NODE(DT_CHOSEN(zephyr_dtcm), DT_CHOSEN(zephyr_sram))
#define DTCM_BASE DT_REG_ADDR(DT_CHOSEN(zephyr_dtcm))
#define DTCM_END  (DT_REG_ADDR(DT_CHOSEN(zephyr_dtcm)) + DT_REG_SIZE(DT_CHOSEN(zephyr_dtcm)))

BUILD_ASSERT((DTCM_BASE & __alignof__(uint32_t)) == 0, "DTCM start address must be 32-bit aligned");
BUILD_ASSERT((DTCM_END & __alignof__(uint32_t)) == 0, "DTCM size must be 32-bit aligned");
#endif

#if defined(CONFIG_SOC_RESET_HOOK)
void soc_reset_hook(void)
{
	/* ITCM/DTCM are equipped with ECC. On reset, the TCM content
	 * is undefined and reads, unaligned or sub-sized accesses
	 * will trigger an ECC error. Apply the TCM initialization
	 * procedure described in the ST AN5342 "How to use error
	 * correction code (ECC) management for internal memories
	 * protection on STM32 MCUs" Rev. 9, section 4.1.1: write
	 * an arbitrary value (0 in this case) to the entire TCM
	 * using ECC data word-sized writes.
	 */
#ifdef ITCM_BASE
	volatile uint32_t *itcm_start = (void *)ITCM_BASE;
	volatile uint32_t *itcm_end = (void *)ITCM_END;

	for (volatile uint32_t *p = itcm_start; p < itcm_end; p++) {
		*p = 0;
	}
#endif
#ifdef DTCM_BASE
	volatile uint32_t *dtcm_start = (void *)DTCM_BASE;
	volatile uint32_t *dtcm_end = (void *)DTCM_END;

	for (volatile uint32_t *p = dtcm_start; p < dtcm_end; p++) {
		*p = 0;
	}
#endif

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
	/* SDMMC1 */
	RIF_MASTER_CID1_SEC_PRIV(SDMMC1);
	RIF_SLAVE_SEC_PRIV(SDMMC1);
	/* SDMMC2 */
	RIF_MASTER_CID1_SEC_PRIV(SDMMC2);
	RIF_SLAVE_SEC_PRIV(SDMMC2);
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
