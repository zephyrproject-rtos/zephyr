/*
 * Copyright (c) 2025 Qingdao IotPi Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT sifli_sf32lb58_hpsys_rcc

#include <stdint.h>

#include <zephyr/sys/util.h>
#include <zephyr/arch/cpu.h>
#include "zephyr/device.h"
#include "zephyr/devicetree.h"

#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/sf32lb58_clock_control.h>
#include <zephyr/dt-bindings/clock/sf32lb58_clock.h>

#define CLOCK_TIMEOUT 1024

struct hpsys_rcc_config {
	uintptr_t base;
	uint32_t csr;
};

static int check_clock_driver_status(const struct device *clk_dev)
{
	enum clock_control_status status;

	__ASSERT(NULL != clk_dev, "clk_dev is not defined");
	__ASSERT(true == device_is_ready(clk_dev), "clk_dev is not ready");

	status = clock_control_get_status(clk_dev, (clock_control_subsys_t)0);
	switch (status) {
	case CLOCK_CONTROL_STATUS_ON:
		return 0;
	case CLOCK_CONTROL_STATUS_OFF:
		return clock_control_on(clk_dev, (clock_control_subsys_t)0);
	case CLOCK_CONTROL_STATUS_STARTING:
		for (int i = 0; i < CLOCK_TIMEOUT; i++) {
			status = clock_control_get_status(clk_dev, (clock_control_subsys_t)0);
			if (CLOCK_CONTROL_STATUS_ON == status) {
				return 0;
			}
		}
		return -EBUSY;
	case CLOCK_CONTROL_STATUS_UNKNOWN:
	default:
		return -EBUSY;
	}

	return 0;
}

static int check_sel_sys(const struct device *dev)
{
	const struct hpsys_rcc_config *config = dev->config;
	const struct device *clk_dev = NULL;
	uint8_t clk_sel;

	clk_sel = FIELD_GET(HPSYS_RCC_CSR_SEL_SYS_Msk, config->csr);
	switch (clk_sel) {
#if DT_NODE_HAS_STATUS_OKAY(DT_INST_CLOCKS_CTLR_BY_NAME(0, hrc48))
	case HPSYS_RCC_SEL_SYS_HRC48:
		clk_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR_BY_NAME(0, hrc48));
		break;
#endif
#if DT_NODE_HAS_STATUS_OKAY(DT_INST_CLOCKS_CTLR_BY_NAME(0, hxt48))
	case HPSYS_RCC_SEL_SYS_HXT48:
		clk_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR_BY_NAME(0, hxt48));
		break;
#endif
	default:
		break;
	}

	return check_clock_driver_status(clk_dev);
}

static int check_sel_hpsys_peri(const struct device *dev)
{
	const struct hpsys_rcc_config *config = dev->config;
	const struct device *clk_dev = NULL;
	uint8_t clk_sel;

	clk_sel = FIELD_GET(HPSYS_RCC_CSR_SEL_PERI, config->csr);
	switch (clk_sel) {
#if DT_NODE_HAS_STATUS_OKAY(DT_INST_CLOCKS_CTLR_BY_NAME(0, hrc48))
	case HPSYS_RCC_SEL_PERI_HRC48:
		clk_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR_BY_NAME(0, hrc48));
		break;
#endif
#if DT_NODE_HAS_STATUS_OKAY(DT_INST_CLOCKS_CTLR_BY_NAME(0, hxt48))
	case HPSYS_RCC_SEL_PERI_HXT48:
		clk_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR_BY_NAME(0, hxt48));
		break;
#endif
	default:
		break;
	}

	return check_clock_driver_status(clk_dev);
}

static int hpsys_rcc_init(const struct device *dev)
{
	int ret;

	ret = check_sel_sys(dev);
	if (ret) {
		return ret;
	}

	ret = check_sel_hpsys_peri(dev);
	if (ret) {
		return ret;
	}

	/* SEL_SYS_LP select SEL_SYS or clk_lp, which are not handled here */

	const struct hpsys_rcc_config *config = dev->config;
	sys_write32(config->csr, config->base + HPSYS_RCC_CSR);
	return 0;
}

static int hpsys_rcc_on(const struct device *dev, clock_control_subsys_t sys)
{
	const struct hpsys_rcc_config *config = dev->config;
	switch ((uintptr_t)sys) {
	case HPSYS_RCC_SUBSYS_PINMUX1:
		sys_set_bit(config->base + HPSYS_RCC_ENR1, HPSYS_RCC_ENR1_PINMUX1_Pos);
		break;
	}
	return 0;
}

static int hpsys_rcc_off(const struct device *dev, clock_control_subsys_t sys)
{
	const struct hpsys_rcc_config *config = dev->config;
	switch ((uintptr_t)sys) {
	case HPSYS_RCC_SUBSYS_PINMUX1:
		sys_clear_bit(config->base + HPSYS_RCC_ENR1, HPSYS_RCC_ENR1_PINMUX1_Pos);
		break;
	}
	return 0;
}

static enum clock_control_status hpsys_rcc_get_status(const struct device *dev,
						      clock_control_subsys_t sys)
{
	const struct hpsys_rcc_config *config = dev->config;
	switch ((uintptr_t)sys) {
	case HPSYS_RCC_SUBSYS_PINMUX1:
		if (sys_test_bit(config->base + HPSYS_RCC_ENR1, HPSYS_RCC_ENR1_PINMUX1_Pos)) {
			return CLOCK_CONTROL_STATUS_ON;
		} else {
			return CLOCK_CONTROL_STATUS_OFF;
		}
		break;
	}
	return 0;
}

static const struct hpsys_rcc_config config = {
	.base = DT_REG_ADDR(DT_DRV_INST(0)),
	.csr = FIELD_PREP(HPSYS_RCC_CSR_SEL_SYS_Msk, DT_INST_PROP(0, sel_sys)) |
	       FIELD_PREP(HPSYS_RCC_CSR_SEL_SYS_LP_Msk, DT_INST_PROP(0, sel_lp_sys)) |
	       FIELD_PREP(HPSYS_RCC_CSR_SEL_MPI1_Msk, DT_INST_PROP(0, sel_mpi1)) |
	       FIELD_PREP(HPSYS_RCC_CSR_SEL_MPI2_Msk, DT_INST_PROP(0, sel_mpi2)) |
	       FIELD_PREP(HPSYS_RCC_CSR_SEL_MPI3_Msk, DT_INST_PROP(0, sel_mpi3)) |
	       FIELD_PREP(HPSYS_RCC_CSR_SEL_MPI4_Msk, DT_INST_PROP(0, sel_mpi4)) |
	       FIELD_PREP(HPSYS_RCC_CSR_SEL_PERI_Msk, DT_INST_PROP(0, sel_peri)) |
	       FIELD_PREP(HPSYS_RCC_CSR_SEL_PDM1_Msk, DT_INST_PROP(0, sel_pdm1)) |
	       FIELD_PREP(HPSYS_RCC_CSR_SEL_PDM2_Msk, DT_INST_PROP(0, sel_pdm2)) |
	       FIELD_PREP(HPSYS_RCC_CSR_SEL_USBC_Msk, DT_INST_PROP(0, sel_usbc)) |
	       FIELD_PREP(HPSYS_RCC_CSR_SEL_SDMMC_Msk, DT_INST_PROP(0, sel_sdmmc)),
};

static DEVICE_API(clock_control, hpsys_rcc_api) = {
	.on = hpsys_rcc_on,
	.off = hpsys_rcc_off,
	.get_status = hpsys_rcc_get_status,
};

DEVICE_DT_INST_DEFINE(0, hpsys_rcc_init, NULL, NULL, &config, PRE_KERNEL_1,
		      CONFIG_CLOCK_CONTROL_INIT_PRIORITY, &hpsys_rcc_api);
