/*
 * Copyright (c) 2022 Microchip Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <errno.h>
#include <zephyr/device.h>
#include <soc.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/mchp_xec_clock_control.h>
#include <zephyr/dt-bindings/clock/mchp_xec_pcr.h>
#include <zephyr/dt-bindings/pinctrl/mchp-xec-pinctrl.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/printk.h>
LOG_MODULE_REGISTER(clock32k, CONFIG_CLOCK_CONTROL_LOG_LEVEL);

#include <soc.h>

#ifdef CONFIG_SOC_SERIES_MEC15XX
static void pcr_clock_regs(void)
{
	struct pcr_regs *pcr = ((struct pcr_regs *)DT_REG_ADDR_BY_IDX(DT_NODELABEL(pcr), 0));
	uint32_t r = pcr->PWR_RST_STS;

	LOG_INF("MEC152x PCR registers");

	LOG_INF("PCR Power Reset Status register(bit[10] is 32K_ACTIVE) = 0x%x", r);

	r = pcr->OSC_ID;
	LOG_INF("PCR Oscillator ID register(bit[8]=PLL Lock) = 0x%x", r);

	r = pcr->PROC_CLK_CTRL;
	LOG_INF("PCR Processor Clock Control register = 0x%x", r);

	r = pcr->SLOW_CLK_CTRL;
	LOG_INF("PCR Slow Clock Control register = 0x%x", r);
}

static void vbat_clock_regs(void)
{
	struct vbatr_regs *vbr = ((struct vbatr_regs *)DT_REG_ADDR_BY_IDX(DT_NODELABEL(pcr), 1));
	uint32_t cken = vbr->CLK32_EN;

	LOG_INF("MEC152x VBAT Clock registers");
	LOG_INF("ClockEnable = 0x%08x", cken);
	if (cken & BIT(2)) {
		LOG_INF("32KHz clock source is XTAL");
		if (cken & BIT(3)) {
			LOG_INF("XTAL configured for single-ended using XTAL2 pin"
				" (external 32KHz waveform)");
		} else {
			LOG_INF("XTAL configured for parallel resonant crystal circuit on"
				" XTAL1 and XTAL2 pins");
		}
	} else {
		LOG_INF("32KHz clock source is the Internal Silicon 32KHz OSC");
	}
	if (cken & BIT(1)) {
		LOG_INF("32KHz clock domain uses the 32KHZ_IN pin(GPIO_0165 F1)");
	} else {
		LOG_INF("32KHz clock domain uses the 32KHz clock source");
	}

	LOG_INF("32KHz trim = 0x%08x", vbr->CKK32_TRIM);
}

static void vbat_power_fail(void)
{
	struct vbatr_regs *vbr = ((struct vbatr_regs *)DT_REG_ADDR_BY_IDX(DT_NODELABEL(pcr), 1));
	uint32_t pfrs = vbr->PFRS;

	LOG_INF("MEC152x VBAT Power-Fail-Reset-Status = 0x%x", pfrs);

	if (pfrs & MCHP_VBATR_PFRS_VBAT_RST_POS) {
		LOG_INF("WARNING: VBAT POR. Clock control register settings lost during"
			" power cycle");
	}

	vbr->PFRS = 0xffffffffU;
}
#else
static void print_pll_clock_src(uint32_t pcr_clk_src)
{
	uint32_t temp = pcr_clk_src & MCHP_PCR_VTR_32K_SRC_MASK;

	if (temp == MCHP_PCR_VTR_32K_SRC_SILOSC) {
		LOG_INF("PLL 32K clock source is Internal Silicon OSC(VTR)");
	} else if (temp == MCHP_PCR_VTR_32K_SRC_XTAL) {
		LOG_INF("PLL 32K clock source is XTAL input(VTR)");
	} else if (temp == MCHP_PCR_VTR_32K_SRC_PIN) {
		LOG_INF("PLL 32K clock source is 32KHZ_IN pin(VTR)");
	} else {
		LOG_INF("PLL 32K clock source is OFF. PLL disabled. Running on Ring OSC");
	}
}

static void print_periph_clock_src(uint32_t vb_clk_src)
{
	uint32_t temp = (vb_clk_src & MCHP_VBATR_CS_PCS_MSK) >> MCHP_VBATR_CS_PCS_POS;

	if (temp == MCHP_VBATR_CS_PCR_VTR_VBAT_SO_VAL) {
		LOG_INF("Periph 32K clock source is InternalOSC(VTR) and InternalOSC(VBAT)");
	} else if (temp == MCHP_VBATR_CS_PCS_VTR_VBAT_XTAL_VAL) {
		LOG_INF("Periph 32K clock source is XTAL(VTR) and XTAL(VBAT)");
	} else if (temp == MCHP_VBATR_CS_PCS_VTR_PIN_SO_VAL) {
		LOG_INF("Periph 32K clock source is 32KHZ_PIN(VTR) and InternalOSC(VBAT)");
	} else {
		LOG_INF("Periph 32K clock source is 32KHZ_PIN fallback to XTAL when VTR is off");
	}
}

static void pcr_clock_regs(void)
{
	struct pcr_regs *pcr = ((struct pcr_regs *)DT_REG_ADDR_BY_IDX(DT_NODELABEL(pcr), 0));
	uint32_t pcr_clk_src = pcr->CLK32K_SRC_VTR;
	uint32_t r = pcr->PWR_RST_STS;

	LOG_INF("MEC172x PCR registers");

	print_pll_clock_src(pcr_clk_src);

	LOG_INF("PCR Power Reset Status register(b[10] is 32K_ACTIVE) = 0x%x", r);

	r = pcr->OSC_ID;
	LOG_INF("PCR Oscillator ID register(bit[8]=PLL Lock) = 0x%x", r);

	r = pcr->PROC_CLK_CTRL;
	LOG_INF("PCR Processor Clock Control register = 0x%x", r);

	r = pcr->SLOW_CLK_CTRL;
	LOG_INF("PCR Slow Clock Control register = 0x%x", r);

	r = pcr->CNT32K_PER;
	LOG_INF("PCR 32KHz Clock Monitor Pulse High Count register = 0x%x", r);

	r = pcr->CNT32K_PER_MIN;
	LOG_INF("PCR 32KHz Clock Monitor Period Maximum Count register = 0x%x", r);

	r = pcr->CNT32K_DV;
	LOG_INF("PCR 32KHz Clock Monitor Duty Cycle Variation register = 0x%x", r);

	r = pcr->CNT32K_DV_MAX;
	LOG_INF("PCR 32KHz Clock Monitor Duty Cycle Variation Max register = 0x%x", r);

	r = pcr->CNT32K_VALID;
	LOG_INF("PCR 32KHz Clock Monitor Valid register = 0x%x", r);

	r = pcr->CNT32K_VALID_MIN;
	LOG_INF("PCR 32KHz Clock Monitor Valid Min register = 0x%x", r);

	r = pcr->CNT32K_CTRL;
	LOG_INF("PCR 32KHz Clock Monitor Control register = 0x%x", r);

	r = pcr->CLK32K_MON_ISTS;
	LOG_INF("PCR 32KHz Clock Monitor Control Status register = 0x%x", r);
}

static void vbat_clock_regs(void)
{
	struct vbatr_regs *vbr = ((struct vbatr_regs *)DT_REG_ADDR_BY_IDX(DT_NODELABEL(pcr), 1));
	uint32_t vb_clk_src = vbr->CLK32_SRC;

	print_periph_clock_src(vb_clk_src);
}

static void vbat_power_fail(void)
{
	struct vbatr_regs *vbr = ((struct vbatr_regs *)DT_REG_ADDR_BY_IDX(DT_NODELABEL(pcr), 1));
	uint32_t pfrs = vbr->PFRS;

	LOG_INF("MEC172x VBAT Power-Fail-Reset-Status = 0x%x", pfrs);

	if (pfrs & MCHP_VBATR_PFRS_VBAT_RST_POS) {
		LOG_INF("WARNING: VBAT POR. Clock control register settings"
			" lost during power cycle");
	}

	/* clear VBAT powered status */
	vbr->PFRS = 0xffffffffU;
}
#endif

static const struct gpio_regs * const gpio =
	(struct gpio_regs *)(DT_REG_ADDR(DT_NODELABEL(gpio_000_036)));
static const struct device *clkdev = DEVICE_DT_GET(DT_NODELABEL(pcr));

struct sys_clk {
	uint32_t id;
	char *name;
};

static const struct sys_clk sys_clocks[] = {
	{ .id = MCHP_XEC_PCR_CLK_CORE, .name = "Core" },
	{ .id = MCHP_XEC_PCR_CLK_CPU, .name = "CPU" },
	{ .id = MCHP_XEC_PCR_CLK_BUS, .name = "Bus" },
	{ .id = MCHP_XEC_PCR_CLK_PERIPH, .name = "Periph" },
	{ .id = MCHP_XEC_PCR_CLK_PERIPH_FAST, .name = "Periph-fast" },
	{ .id = MCHP_XEC_PCR_CLK_PERIPH_SLOW, .name = "Periph-slow" },
};

int main(void)
{
	int rc = 0;
	uint32_t rate = 0U;
	uint32_t r = 0U;
	clock_control_subsys_t sys = NULL;

	LOG_INF("XEC Clock control driver sample");

	if (!device_is_ready(clkdev)) {
		LOG_ERR("XEC clock control driver is not ready!");
		return 0;
	}

	vbat_power_fail();

	LOG_INF("32KHZ_IN is function 1 of GPIO 0165");
	r = gpio->CTRL[MCHP_XEC_PINCTRL_REG_IDX(0165)];
	LOG_INF("XEC GPIO 0165 Control = 0x%x", r);
	r = (r & MCHP_GPIO_CTRL_MUX_MASK) >> MCHP_GPIO_CTRL_MUX_POS;
	LOG_INF("Pin function = %u", r);
	vbat_clock_regs();
	pcr_clock_regs();

	for (size_t i = 0; i < ARRAY_SIZE(sys_clocks); i++) {
		LOG_INF("API get rate for %s", sys_clocks[i].name);
		sys = (clock_control_subsys_t)sys_clocks[i].id;
		rate = 0U;
		rc = clock_control_get_rate(clkdev, sys, &rate);
		if (rc) {
			LOG_ERR("API error: %d", rc);
		} else {
			LOG_INF("rate = %u", rate);
		}
	}
	return 0;
}
