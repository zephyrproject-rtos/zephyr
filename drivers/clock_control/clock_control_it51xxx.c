/*
 * Copyright (c) 2025 ITE Corporation. All Rights Reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ite_it51xxx_ecpm

#include <soc.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/dt-bindings/clock/ite-it51xxx-clock.h>
#include <zephyr/dt-bindings/interrupt-controller/ite-it51xxx-intc.h>
#include <zephyr/irq.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(clock_control_it51xxx, LOG_LEVEL_ERR);

BUILD_ASSERT(DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT) == 1,
	     "only one ite,it51xxx-ecpm compatible node can be supported");

/* it51xxx ECPM registers definition */
/* 0x02: Clock Gating Control 2 register */
#define ECPM_CGCTRL2R     0x02
#define ECPM_CIRCG        BIT(5)
#define ECPM_SWUCCG       BIT(4)
/* 0x03: PLL Control */
#define ECPM_PLLCTRL      0x03
/* 0x04: Auto Clock Gating */
#define ECPM_AUTOCG       0x04
#define ECPM_AUART1CG     BIT(6)
#define ECPM_AUART2CG     BIT(5)
#define ECPM_ASSPICG      BIT(4)
#define ECPM_ACIRCG       BIT(2)
/* 0x02: Clock Gating Control 5 register */
#define ECPM_CGCTRL3R     0x05
#define ECPM_PECICG       BIT(3)
#define ECPM_SSPICG       BIT(1)
/* 0x06: PLL Frequency */
#define ECPM_PLLFREQR     0x06
#define ECPM_PLLFREQ_MASK GENMASK(3, 0)

static const uint8_t pll_cfg[] = {
	[PLL_18400_KHZ] = 0x01,
	[PLL_32300_KHZ] = 0x03,
	[PLL_64500_KHZ] = 0x07,
	[PLL_48000_KHZ] = 0x09,
};

struct clock_control_it51xxx_data {
	const uint8_t *pll_configuration;
};

/* Driver config */
struct clock_control_it51xxx_config {
	mm_reg_t ecpm_base;
	int pll_freq;
};

/* Clock controller local functions */
static inline int clock_control_it51xxx_on(const struct device *dev,
					   clock_control_subsys_t sub_system)
{
	const struct clock_control_it51xxx_config *const config = dev->config;
	struct ite_clk_cfg *clk_cfg = (struct ite_clk_cfg *)(sub_system);

	/* Enable the clock of this module */
	sys_write8(sys_read8(config->ecpm_base + clk_cfg->ctrl) & ~(clk_cfg->bits),
		   config->ecpm_base + clk_cfg->ctrl);

	return 0;
}

static inline int clock_control_it51xxx_off(const struct device *dev,
					    clock_control_subsys_t sub_system)
{
	const struct clock_control_it51xxx_config *const config = dev->config;
	struct ite_clk_cfg *clk_cfg = (struct ite_clk_cfg *)(sub_system);
	uint8_t tmp_mask = 0;

	/* CGCTRL3R, bit 6, must always write a 1. */
	tmp_mask = (clk_cfg->ctrl == IT51XXX_ECPM_CGCTRL3R_OFF) ? 0x40 : 0x00;
	sys_write8(sys_read8(config->ecpm_base + clk_cfg->ctrl) | clk_cfg->bits | tmp_mask,
		   config->ecpm_base + clk_cfg->ctrl);

	return 0;
}

static int clock_control_it51xxx_get_rate(const struct device *dev,
					  clock_control_subsys_t sub_system, uint32_t *rate)
{
	const struct clock_control_it51xxx_config *const config = dev->config;
	int reg_val = sys_read8(config->ecpm_base + ECPM_PLLFREQR) & ECPM_PLLFREQ_MASK;

	switch (reg_val) {
	case 0x01:
		*rate = KHZ(18400);
		break;
	case 0x03:
		*rate = KHZ(32300);
		break;
	case 0x07:
		*rate = KHZ(64500);
		break;
	case 0x09:
		*rate = KHZ(48000);
		break;
	default:
		return -ERANGE;
	}

	return 0;
}

static void pll_change_isr(const void *unused)
{
	ARG_UNUSED(unused);

	/*
	 * We are here because we have completed changing PLL sequence,
	 * so disabled PLL frequency change event interrupt.
	 */
	irq_disable(IT51XXX_IRQ_PLL_CHANGE);
}

static void chip_configure_pll(const struct device *dev, uint8_t pll)
{
	const struct clock_control_it51xxx_config *config = dev->config;

	/* Set pll frequency change event */
	IRQ_CONNECT(IT51XXX_IRQ_PLL_CHANGE, 0, pll_change_isr, NULL, IRQ_TYPE_EDGE_RISING);
	/* Clear interrupt status of pll frequency change event */
	ite_intc_isr_clear(IT51XXX_IRQ_PLL_CHANGE);

	irq_enable(IT51XXX_IRQ_PLL_CHANGE);
	/*
	 * Configure PLL clock dividers.
	 * Writing data to these registers doesn't change the PLL frequency immediately until the
	 * status is changed into wakeup from the sleep mode.
	 * The following code is intended to make the system enter sleep mode, and wait PLL
	 * frequency change event to wakeup chip to complete PLL update.
	 */
	sys_write8(pll, config->ecpm_base + ECPM_PLLFREQR);

	/* Chip sleep after wait for interrupt (wfi) instruction */
	chip_pll_ctrl(CHIP_PLL_SLEEP);
	/* Chip sleep and wait timer wake it up */
	__asm__ volatile("wfi");
	/* Chip sleep and wait timer wake it up */
	chip_pll_ctrl(CHIP_PLL_DOZE);
}

static int clock_control_it51xxx_init(const struct device *dev)
{
	const struct clock_control_it51xxx_config *config = dev->config;
	struct clock_control_it51xxx_data *data = dev->data;
	int reg_val = sys_read8(config->ecpm_base + ECPM_PLLFREQR) & ECPM_PLLFREQ_MASK;
	uint8_t autocg;

	/* Disable auto gating and enable it by the respective module. */
	autocg = sys_read8(config->ecpm_base + ECPM_AUTOCG);
	sys_write8(autocg & ~(ECPM_AUART1CG | ECPM_AUART2CG | ECPM_ASSPICG | ECPM_ACIRCG),
		   config->ecpm_base + ECPM_AUTOCG);

	/* The following modules are gated in the initial state */
	sys_write8(ECPM_CIRCG | ECPM_SWUCCG, config->ecpm_base + ECPM_CGCTRL2R);
	sys_write8(sys_read8(config->ecpm_base + ECPM_CGCTRL3R) | ECPM_PECICG | ECPM_SSPICG,
		   config->ecpm_base + ECPM_CGCTRL3R);

	if (IS_ENABLED(CONFIG_ITE_IT51XXX_INTC)) {
		ite_intc_save_and_disable_interrupts();
	}

	if (reg_val != data->pll_configuration[config->pll_freq]) {
		/* configure PLL clock */
		chip_configure_pll(dev, data->pll_configuration[config->pll_freq]);
	}

	if (IS_ENABLED(CONFIG_ITE_IT51XXX_INTC)) {
		ite_intc_restore_interrupts();
	}

	return 0;
}

/* Clock controller driver registration */
static DEVICE_API(clock_control, clock_control_it51xxx_api) = {
	.on = clock_control_it51xxx_on,
	.off = clock_control_it51xxx_off,
	.get_rate = clock_control_it51xxx_get_rate,
};

static struct clock_control_it51xxx_data clock_control_it51xxx_data = {
	.pll_configuration = pll_cfg,
};

static const struct clock_control_it51xxx_config clock_control_it51xxx_cfg = {
	.ecpm_base = DT_INST_REG_ADDR(0),
	.pll_freq = DT_INST_PROP(0, pll_frequency),
};

DEVICE_DT_INST_DEFINE(0, clock_control_it51xxx_init, NULL, &clock_control_it51xxx_data,
		      &clock_control_it51xxx_cfg, PRE_KERNEL_1,
		      CONFIG_IT51XXX_PLL_SEQUENCE_PRIORITY, &clock_control_it51xxx_api);
