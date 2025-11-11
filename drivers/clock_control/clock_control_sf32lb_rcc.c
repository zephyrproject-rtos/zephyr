/*
 * Copyright (c) 2025 Core Devices LLC
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT sifli_sf32lb_rcc_clk

#include <stdint.h>

#include <zephyr/arch/cpu.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/clock_control/sf32lb.h>
#include <zephyr/dt-bindings/clock/sf32lb-clocks-common.h>
#include <zephyr/toolchain.h>

#include <register.h>

#define HPSYS_CFG_CAU2_CR offsetof(HPSYS_CFG_TypeDef, CAU2_CR)

#define PMUC_HXT_CR1 offsetof(PMUC_TypeDef, HXT_CR1)

#define HPSYS_RCC_CSR    offsetof(HPSYS_RCC_TypeDef, CSR)
#define HPSYS_RCC_CFGR   offsetof(HPSYS_RCC_TypeDef, CFGR)
#define HPSYS_RCC_DLL1CR offsetof(HPSYS_RCC_TypeDef, DLL1CR)
#define HPSYS_RCC_DLL2CR offsetof(HPSYS_RCC_TypeDef, DLL2CR)

#define HPSYS_RCC_CSR_SEL_SYS_CLK_HXT48  FIELD_PREP(HPSYS_RCC_CSR_SEL_SYS_Msk, 1U)
#define HPSYS_RCC_CSR_SEL_SYS_CLK_DLL1   FIELD_PREP(HPSYS_RCC_CSR_SEL_SYS_Msk, 3U)
#define HPSYS_RCC_CSR_SEL_MPI1_CLK_DLL2  FIELD_PREP(HPSYS_RCC_CSR_SEL_MPI1_Msk, 2U)
#define HPSYS_RCC_CSR_SEL_MPI2_CLK_DLL2  FIELD_PREP(HPSYS_RCC_CSR_SEL_MPI2_Msk, 2U)
#define HPSYS_RCC_CSR_SEL_PERI_CLK_HXT48 FIELD_PREP(HPSYS_RCC_CSR_SEL_PERI_Msk, 1U)

#define HPSYS_RCC_DLLXCR_EN          HPSYS_RCC_DLL1CR_EN
#define HPSYS_RCC_DLLXCR_STG_Msk     HPSYS_RCC_DLL1CR_STG_Msk
#define HPSYS_RCC_DLLXCR_STG_STEP    24000000UL
#define HPSYS_RCC_DLLXCR_IN_DIV2_EN  HPSYS_RCC_DLL1CR_IN_DIV2_EN
#define HPSYS_RCC_DLLXCR_OUT_DIV2_EN HPSYS_RCC_DLL1CR_OUT_DIV2_EN
#define HPSYS_RCC_DLLXCR_READY       HPSYS_RCC_DLL1CR_READY

struct clock_control_sf32lb_rcc_config {
	uintptr_t base;
	uintptr_t cfg;
	uintptr_t pmuc;
	uint8_t hdiv;
	uint8_t pdiv1;
	uint8_t pdiv2;
	uint32_t dll1_freq;
	uint32_t dll2_freq;
	const struct device *hxt48;
};

static inline void configure_dll(const struct device *dev, uint32_t freq, uint32_t dllxcr)
{
	const struct clock_control_sf32lb_rcc_config *config = dev->config;
	uint32_t val;

	/* disable DLLX, clear modified fields */
	val = sys_read32(config->base + dllxcr);
	val &= ~HPSYS_RCC_DLLXCR_EN;
	sys_write32(val, config->base + dllxcr);

	/* configure DLLX */
	val &= ~(HPSYS_RCC_DLLXCR_STG_Msk | HPSYS_RCC_DLLXCR_OUT_DIV2_EN);
	val |= FIELD_PREP(HPSYS_RCC_DLLXCR_STG_Msk, (freq / HPSYS_RCC_DLLXCR_STG_STEP) - 1U) |
	       HPSYS_RCC_DLLXCR_IN_DIV2_EN | HPSYS_RCC_DLLXCR_EN;
	sys_write32(val, config->base + dllxcr);

	do {
		val = sys_read32(config->base + dllxcr);
	} while ((val & HPSYS_RCC_DLLXCR_READY) == 0U);
}

static int clock_control_sf32lb_rcc_on(const struct device *dev, clock_control_subsys_t sys)
{
	const struct clock_control_sf32lb_rcc_config *config = dev->config;
	uint16_t id = *(uint16_t *)sys;

	sys_set_bit(config->base + FIELD_GET(SF32LB_CLOCK_OFFSET_MSK, id),
		    FIELD_GET(SF32LB_CLOCK_BIT_MSK, id));

	return 0;
}

static int clock_control_sf32lb_rcc_off(const struct device *dev, clock_control_subsys_t sys)
{
	const struct clock_control_sf32lb_rcc_config *config = dev->config;
	uint16_t id = *(uint16_t *)sys;

	sys_clear_bit(config->base + FIELD_GET(SF32LB_CLOCK_OFFSET_MSK, id),
		      FIELD_GET(SF32LB_CLOCK_BIT_MSK, id));

	return 0;
}

static enum clock_control_status clock_control_sf32lb_rcc_get_status(const struct device *dev,
								     clock_control_subsys_t sys)
{
	const struct clock_control_sf32lb_rcc_config *config = dev->config;
	uint16_t id = *(uint16_t *)sys;

	if (sys_test_bit(config->base + FIELD_GET(SF32LB_CLOCK_OFFSET_MSK, id),
			 FIELD_GET(SF32LB_CLOCK_BIT_MSK, id)) != 0) {
		return CLOCK_CONTROL_STATUS_ON;
	}

	return CLOCK_CONTROL_STATUS_OFF;
}

static DEVICE_API(clock_control, clock_control_sf32lb_rcc_api) = {
	.on = clock_control_sf32lb_rcc_on,
	.off = clock_control_sf32lb_rcc_off,
	.get_status = clock_control_sf32lb_rcc_get_status,
};

static int clock_control_sf32lb_rcc_init(const struct device *dev)
{
	const struct clock_control_sf32lb_rcc_config *config = dev->config;
	uint32_t val;

	if (config->hxt48 != NULL) {
		(void)clock_control_on(config->hxt48, NULL);

		/* TODO: make this configurable */
		val = sys_read32(config->base + HPSYS_RCC_CFGR);
		val &= HPSYS_RCC_CSR_SEL_SYS_Msk;
		val |= HPSYS_RCC_CSR_SEL_SYS_CLK_HXT48;
		sys_write32(val, config->base + HPSYS_RCC_CSR);

		val = sys_read32(config->base + HPSYS_RCC_CSR);
		val &= ~HPSYS_RCC_CSR_SEL_PERI_Msk;
		val |= HPSYS_RCC_CSR_SEL_PERI_CLK_HXT48;
		sys_write32(val, config->base + HPSYS_RCC_CSR);
	}

	if (config->dll1_freq != 0U || config->dll2_freq != 0U) {
		val = sys_read32(config->pmuc + PMUC_HXT_CR1);
		val |= PMUC_HXT_CR1_BUF_DLL_EN;
		sys_write32(val, config->pmuc + PMUC_HXT_CR1);

		val = sys_read32(config->cfg + HPSYS_CFG_CAU2_CR);
		val |= HPSYS_CFG_CAU2_CR_HPBG_EN | HPSYS_CFG_CAU2_CR_HPBG_VDDPSW_EN;
		sys_write32(val, config->cfg + HPSYS_CFG_CAU2_CR);

		val = sys_read32(config->base + HPSYS_RCC_CSR);

		if (config->dll1_freq != 0U) {
			configure_dll(dev, config->dll1_freq, HPSYS_RCC_DLL1CR);

			/* TODO: make this configurable */
			val &= ~HPSYS_RCC_CSR_SEL_SYS_Msk;
			val |= HPSYS_RCC_CSR_SEL_SYS_CLK_DLL1;
		}

		if (config->dll2_freq != 0U) {
			configure_dll(dev, config->dll2_freq, HPSYS_RCC_DLL2CR);

			/* TODO: make this configurable */
			val &= ~(HPSYS_RCC_CSR_SEL_MPI1_Msk | HPSYS_RCC_CSR_SEL_MPI2_Msk);
			val |= HPSYS_RCC_CSR_SEL_MPI1_CLK_DLL2 | HPSYS_RCC_CSR_SEL_MPI2_CLK_DLL2;
		}

		sys_write32(val, config->base + HPSYS_RCC_CSR);
	}

	/* configure HDIV/PDIV1/PDIV2 dividers */
	val = sys_read32(config->base + HPSYS_RCC_CFGR);
	val &= ~(HPSYS_RCC_CFGR_HDIV_Msk | HPSYS_RCC_CFGR_PDIV1_Msk | HPSYS_RCC_CFGR_PDIV2_Msk);
	val |= FIELD_PREP(HPSYS_RCC_CFGR_HDIV_Msk, config->hdiv) |
	       FIELD_PREP(HPSYS_RCC_CFGR_PDIV1_Msk, config->pdiv1) |
	       FIELD_PREP(HPSYS_RCC_CFGR_PDIV2_Msk, config->pdiv2);
	sys_write32(val, config->base + HPSYS_RCC_CFGR);

	return 0;
}

/* DLL1/2 requires HXT48 */
IF_ENABLED(UTIL_OR(
		DT_NODE_HAS_STATUS(DT_INST_CHILD(0, dll1), okay),
		DT_NODE_HAS_STATUS(DT_INST_CHILD(0, dll2), okay)),
	   (BUILD_ASSERT(
		(DT_NODE_HAS_STATUS(DT_INST_CLOCKS_CTLR_BY_NAME(0, hxt48), okay)),
		"DLL1/2 require HXT48 to be enabled");))

/* DLL1/2 frequency must be a multiple of step size */
IF_ENABLED(DT_NODE_HAS_STATUS(DT_INST_CHILD(0, dll1), okay), (
	   BUILD_ASSERT(
		((DT_PROP(DT_INST_CHILD(0, dll1), clock_frequency) != 0) &&
		 ((DT_PROP(DT_INST_CHILD(0, dll1), clock_frequency) %
		   HPSYS_RCC_DLLXCR_STG_STEP) == 0)),
		"DLL1 frequency must be a non-zero multiple of "
		STRINGIFY(HPSYS_RCC_DLLXCR_STG_STEP)
	   );))

IF_ENABLED(DT_NODE_HAS_STATUS(DT_INST_CHILD(0, dll2), okay), (
	   BUILD_ASSERT(
		((DT_PROP(DT_INST_CHILD(0, dll2), clock_frequency) != 0) &&
		 ((DT_PROP(DT_INST_CHILD(0, dll2), clock_frequency) %
		   HPSYS_RCC_DLLXCR_STG_STEP) == 0)),
		"DLL2 frequency must be a non-zero multiple of "
		STRINGIFY(HPSYS_RCC_DLLXCR_STG_STEP)
	   );))

static const struct clock_control_sf32lb_rcc_config config = {
	.base = DT_REG_ADDR(DT_INST_PARENT(0)),
	.cfg = DT_REG_ADDR(DT_INST_PHANDLE(0, sifli_cfg)),
	.pmuc = DT_REG_ADDR(DT_INST_PHANDLE(0, sifli_pmuc)),
	.hxt48 = DEVICE_DT_GET_OR_NULL(DT_INST_CLOCKS_CTLR_BY_NAME(0, hxt48)),
	.hdiv = DT_INST_PROP(0, sifli_hdiv),
	.pdiv1 = DT_INST_PROP(0, sifli_pdiv1),
	.pdiv2 = DT_INST_PROP(0, sifli_pdiv2),
	.dll1_freq = COND_CODE_1(DT_NODE_HAS_STATUS(DT_INST_CHILD(0, dll1), okay),
				 (DT_PROP(DT_INST_CHILD(0, dll1), clock_frequency)), (0U)),
	.dll2_freq = COND_CODE_1(DT_NODE_HAS_STATUS(DT_INST_CHILD(0, dll2), okay),
				 (DT_PROP(DT_INST_CHILD(0, dll2), clock_frequency)), (0U)),
};

DEVICE_DT_INST_DEFINE(0, clock_control_sf32lb_rcc_init, NULL, NULL, &config, PRE_KERNEL_1,
		      CONFIG_CLOCK_CONTROL_INIT_PRIORITY, &clock_control_sf32lb_rcc_api);
