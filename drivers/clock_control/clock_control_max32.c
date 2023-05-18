/*
 * Copyright (c) 2023 Analog Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/clock_control.h>

#define DT_DRV_COMPAT adi_max32_gcr

#define DT_GCR_CLOCK_SOURCE DT_CLOCKS_CTLR(DT_NODELABEL(gcr))

#if DT_SAME_NODE(DT_GCR_CLOCK_SOURCE, DT_NODELABEL(clk_lpo))
#define MAX32_SYSCLK_SRC 0x00
#elif DT_SAME_NODE(DT_GCR_CLOCK_SOURCE, DT_NODELABEL(clk_hso))
#define MAX32_SYSCLK_SRC 0x04
#elif DT_SAME_NODE(DT_GCR_CLOCK_SOURCE, DT_NODELABEL(clk_obrc))
#define MAX32_SYSCLK_SRC 0x05
#endif

#define GCR_CLKCN_HSO_ENABLE   BIT(19)
#define GCR_CLKCN_OBRC_ENABLE  BIT(20)
#define GRC_CLKCN_PSC_MASK     GENMASK(8, 6)
#define GCR_CLKCN_CLKSEL_MASK  GENMASK(11, 9)
#define GCR_CLKCN_SYSCLK_READY BIT(13)

#define MAX32_CLOCK_BUS_PERCLKCN0 0
#define MAX32_CLOCK_BUS_PERCLKCN1 1

struct max32_gcr_regs {
	uint32_t scon;
	uint32_t rstr0;
	uint32_t clkcn;
	uint32_t pm;
	uint32_t rsv_0x10_0x17[2];
	uint32_t pckdiv;
	uint32_t rsv_0x1c_0x23[2];
	uint32_t perckcn0;
	uint32_t memckcn;
	uint32_t memzcn;
	uint32_t rsv_0x30_0x3f[4];
	uint32_t sysst;
	uint32_t rstr1;
	uint32_t perckcn1;
	uint32_t event_en;
	uint32_t revision;
	uint32_t syssie;
	uint32_t rsv_0x58_0x63[3];
	uint32_t ecc_er;
	uint32_t ecc_ced;
	uint32_t ecc_irqen;
	uint32_t ecc_errad;
	uint32_t btle_ldocr;
	uint32_t btle_ldodcr;
	uint32_t rsv_0x7c;
	uint32_t gp0;
	uint32_t apb_async;
};

struct max32_clock_cfg {
	struct max32_gcr_regs *gcr;
};

static inline int max32_clock_control_on(const struct device *dev,
					 clock_control_subsys_t sub_system)
{
	const struct max32_clock_cfg *cfg = dev->config;
	struct max32_gcr_regs *gcr = cfg->gcr;
	uint32_t clkcfg = (uint32_t)sub_system;
	uint32_t bus = clkcfg >> 16;
	uint32_t bit = clkcfg & 0xFFFF;

	switch (bus) {
	case MAX32_CLOCK_BUS_PERCLKCN0:
		gcr->perckcn0 &= ~BIT(bit);
		break;
	case MAX32_CLOCK_BUS_PERCLKCN1:
		gcr->perckcn1 &= ~BIT(bit);
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static inline int max32_clock_control_off(const struct device *dev,
					  clock_control_subsys_t sub_system)
{
	const struct max32_clock_cfg *cfg = dev->config;
	struct max32_gcr_regs *gcr = cfg->gcr;

	uint32_t clkcfg = (uint32_t)sub_system;

	uint32_t bus = clkcfg >> 16;
	uint32_t bit = clkcfg & 0xFFFF;

	switch (bus) {
	case MAX32_CLOCK_BUS_PERCLKCN0:
		gcr->perckcn0 |= BIT(bit);
		break;
	case MAX32_CLOCK_BUS_PERCLKCN1:
		gcr->perckcn1 |= BIT(bit);
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static struct clock_control_driver_api max32_clock_control_api = {
	.on = max32_clock_control_on,
	.off = max32_clock_control_off,
};

static int max32_clock_control_init(const struct device *dev)
{
	const struct max32_clock_cfg *cfg = dev->config;
	volatile struct max32_gcr_regs *gcr = cfg->gcr;
	unsigned int temp;

	/* Enable desired clocks */
#if DT_NODE_HAS_STATUS(DT_NODELABEL(clk_hso), okay)
	gcr->clkcn |= GCR_CLKCN_HSO_ENABLE;
#endif

#if DT_NODE_HAS_STATUS(DT_NODELABEL(clk_obrc), okay)
	gcr->clkcn |= GCR_CLKCN_OBRC_ENABLE;
#endif

	gcr->clkcn &= ~GRC_CLKCN_PSC_MASK;

	/* Set system clock */
	temp = gcr->clkcn;
	temp &= ~GCR_CLKCN_CLKSEL_MASK;
	temp |= FIELD_PREP(GCR_CLKCN_CLKSEL_MASK, MAX32_SYSCLK_SRC);
	gcr->clkcn = temp;

	while (!(gcr->clkcn & GCR_CLKCN_SYSCLK_READY))
		;

	return 0;
}

static const struct max32_clock_cfg max32_clock_cfg = {
	.gcr = (struct max32_gcr_regs *)DT_INST_REG_ADDR(0)};

DEVICE_DT_INST_DEFINE(0, max32_clock_control_init, NULL, NULL, &max32_clock_cfg, PRE_KERNEL_1,
		      CONFIG_CLOCK_CONTROL_INIT_PRIORITY, &max32_clock_control_api);
