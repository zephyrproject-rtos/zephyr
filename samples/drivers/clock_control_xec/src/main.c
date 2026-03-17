/*
 * Copyright (c) 2022 Microchip Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <errno.h>
#include <soc.h>
#include <zephyr/arch/cpu.h>
#include <zephyr/device.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/mchp_xec_clock_control.h>
#include <zephyr/dt-bindings/clock/mchp_xec_pcr.h>
#include <zephyr/dt-bindings/pinctrl/mchp-xec-pinctrl.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(app, CONFIG_LOG_DEFAULT_LEVEL);

/* Microchip MEC data sheets use octal numbering for pins */
#define MCHP_XEC_32KHZ_IN_PIN 0165

static void pr_vbat_pfrs(uint32_t pfrs)
{
	if (pfrs & BIT(7)) {
		LOG_INF("VBAT reset occurfed! All VBAT based registers and memory contents lost!");
	}

	if (pfrs & BIT(6)) {
		LOG_INF("System Reset trigger by write to Cortex-M4 SYSRESETREQ bit");
	}

	if (pfrs & BIT(5)) {
		LOG_INF("System Reset caused by Watch Dog Timer");
	}

	if (pfrs & BIT(4)) {
		LOG_INF("System Reset caused by RESETI pin");
	}

	if (pfrs & BIT(2)) {
		LOG_INF("System Reset caused by write to PCR Soft System Reset bit");
	}
}

#ifdef CONFIG_SOC_SERIES_MEC15XX
static void pcr_clock_regs(void)
{
	mem_addr_t pcr_base = ((mem_addr_t)DT_REG_ADDR_BY_IDX(DT_NODELABEL(pcr), 0));
	uint32_t r = sys_read32(pcr_base + MCHP_PCR_PRS_OFS);

	LOG_INF("MEC152x PCR registers");

	LOG_INF("PCR Power Reset Status register(bit[10] is 32K_ACTIVE) = 0x%x", r);

	r = sys_read32(pcr_base + MCHP_PCR_SYS_CLK_CTRL_OFS);
	LOG_INF("PCR Processor Clock Control register = 0x%x", r);

	r = sys_read32(pcr_base + MCHP_PCR_OSC_ID_OFS);
	LOG_INF("PCR Oscillator ID register(bit[8]=PLL Lock) = 0x%x", r);

	r = sys_read32(pcr_base + MCHP_PCR_SLOW_CLK_CTRL_OFS);
	LOG_INF("PCR Slow Clock Control register = 0x%x", r);
}

static void vbat_clock_regs(void)
{
	mem_addr_t vbr_base = ((mem_addr_t)DT_REG_ADDR_BY_IDX(DT_NODELABEL(pcr), 1));
	uint32_t cken = sys_read32(vbr_base + XEC_VBR_CS_OFS);
	uint32_t r = 0;

	LOG_INF("MEC152x VBAT Clock registers");
	LOG_INF("ClockEnable = 0x%08x", cken);
	if (cken & BIT(XEC_VBR_CS_SEL_XTAL_POS)) {
		LOG_INF("32KHz clock source is XTAL");
		if (cken & BIT(XEC_VBR_CS_XTAL_SE_POS)) {
			LOG_INF("XTAL configured for single-ended using XTAL2 pin"
				" (external 32KHz waveform)");
		} else {
			LOG_INF("XTAL configured for parallel resonant crystal circuit on"
				" XTAL1 and XTAL2 pins");
		}
	} else {
		LOG_INF("32KHz clock source is the Internal Silicon 32KHz OSC");
	}

	if (cken & BIT(XEC_VBR_CS_EXT_32K_IN_PIN_POS)) {
		LOG_INF("32KHz clock domain uses the 32KHZ_IN pin(GPIO_0165 F1)");
	} else {
		LOG_INF("32KHz clock domain uses the 32KHz clock source");
	}

	r = sys_read32(vbr_base + XEC_VBR_OT_OFS);
	LOG_INF("32KHz trim = 0x%08x", r);

	r = sys_read32(vbr_base + XEC_VBR_TCR_OFS);
	LOG_INF("32KHz trim control = 0x%08x", r);
}

static void vbat_power_fail(void)
{
	mem_addr_t vbr_base = ((mem_addr_t)DT_REG_ADDR_BY_IDX(DT_NODELABEL(pcr), 1));
	uint32_t pfrs = sys_read32(vbr_base + XEC_VBR_PFR_SR_OFS);

	LOG_INF("MEC152x VBAT Power-Fail-Reset-Status = 0x%x", pfrs);

	pr_vbat_pfrs(pfrs);

	sys_write32(UINT32_MAX, vbr_base + XEC_VBR_PFR_SR_OFS);
}
#else
static void print_pll_clock_src(uint32_t pcr_clk_src)
{
	uint32_t temp = XEC_CC_VTR_CS_PLL_SEL_GET(pcr_clk_src);

	if (temp == XEC_CC_VTR_CS_PLL_SEL_SI) {
		LOG_INF("PLL 32K clock source is Internal Silicon OSC(VTR)");
	} else if (temp == XEC_CC_VTR_CS_PLL_SEL_XTAL) {
		LOG_INF("PLL 32K clock source is XTAL input(VTR)");
	} else if (temp == XEC_CC_VTR_CS_PLL_SEL_32K_PIN) {
		LOG_INF("PLL 32K clock source is 32KHZ_IN pin(VTR)");
	} else {
		LOG_INF("PLL 32K clock source is OFF. PLL disabled. Running on Ring OSC");
	}
}

static void print_periph_clock_src(uint32_t vb_clk_src)
{
	uint32_t temp = XEC_VBR_CS_PCS_GET(vb_clk_src);

	if (temp == XEC_VBR_CS_PCS_SI) {
		LOG_INF("Periph 32K clock source is InternalOSC(VTR) and InternalOSC(VBAT)");
	} else if (temp == XEC_VBR_CS_PCS_XTAL) {
		LOG_INF("Periph 32K clock source is XTAL(VTR) and XTAL(VBAT)");
	} else if (temp == XEC_VBR_CS_PCS_PIN_SI) {
		LOG_INF("Periph 32K clock source is 32KHZ_PIN(VTR) and InternalOSC(VBAT)");
	} else {
		LOG_INF("Periph 32K clock source is 32KHZ_PIN fallback to XTAL when VTR is off");
	}
}

static void pcr_clock_regs(void)
{
	mem_addr_t pcr_base = ((mem_addr_t)DT_REG_ADDR_BY_IDX(DT_NODELABEL(pcr), 0));
	uint32_t pcr_clk_src = sys_read32(pcr_base + XEC_CC_VTR_CS_OFS);
	uint32_t r = sys_read32(pcr_base + XEC_CC_PWR_SR_OFS);

	LOG_INF("MEC17xx PCR registers");

	print_pll_clock_src(pcr_clk_src);

	LOG_INF("PCR Power Reset Status register(b[10] is 32K_ACTIVE) = 0x%x", r);

	r = sys_read32(pcr_base + XEC_CC_PCLK_CR_OFS);
	LOG_INF("PCR Processor Clock Control register = 0x%x", r);

	r = sys_read32(pcr_base + XEC_CC_SCLK_CR_OFS);
	LOG_INF("PCR Slow Clock Control register = 0x%x", r);

	r = sys_read32(pcr_base + XEC_CC_OSC_ID_OFS);
	LOG_INF("PCR Oscillator ID register(bit[8]=PLL Lock) = 0x%x", r);

	r = sys_read32(pcr_base + XEC_CC_32K_PER_CNT_OFS);
	LOG_INF("PCR 32KHz Clock Monitor Period Count register = 0x%x", r);

	r = sys_read32(pcr_base + XEC_CC_32K_HP_CNT_OFS);
	LOG_INF("PCR 32KHz Clock Monitor Pulse High Count register = 0x%x", r);

	r = sys_read32(pcr_base + XEC_CC_32K_MIN_PER_CNT_OFS);
	LOG_INF("PCR 32KHz Clock Monitor Period Minimum Count register = 0x%x", r);

	r = sys_read32(pcr_base + XEC_CC_32K_MAX_PER_CNT_OFS);
	LOG_INF("PCR 32KHz Clock Monitor Period Maximum Count register = 0x%x", r);

	r = sys_read32(pcr_base + XEC_CC_32K_DC_VAR_CNT_OFS);
	LOG_INF("PCR 32KHz Clock Monitor Duty Cycle Variation count register = 0x%x", r);

	r = sys_read32(pcr_base + XEC_CC_32K_MAX_DC_VAR_OFS);
	LOG_INF("PCR 32KHz Clock Monitor Max Duty Cycle Variation egister = 0x%x", r);

	r = sys_read32(pcr_base + XEC_CC_32K_VAL_CNT_OFS);
	LOG_INF("PCR 32KHz Clock Monitor Valid register = 0x%x", r);

	r = sys_read32(pcr_base + XEC_CC_32K_MIN_VAL_CNT_OFS);
	LOG_INF("PCR 32KHz Clock Monitor Valid Min register = 0x%x", r);

	r = sys_read32(pcr_base + XEC_CC_32K_CCR_OFS);
	LOG_INF("PCR 32KHz Clock Monitor Control register = 0x%x", r);

	r = sys_read32(pcr_base + XEC_CC_32K_SR_OFS);
	LOG_INF("PCR 32KHz Clock Monitor Control Status register = 0x%x", r);

	r = sys_read32(pcr_base + XEC_CC_32K_IER_OFS);
	LOG_INF("PCR 32KHz Clock Monitor Control IEN register = 0x%x", r);
}

static void vbat_clock_regs(void)
{
	mem_addr_t vbr_base = ((mem_addr_t)DT_REG_ADDR_BY_IDX(DT_NODELABEL(pcr), 1));
	uint32_t vb_clk_src = sys_read32(vbr_base + XEC_VBR_CS_OFS);

	LOG_INF("Raw VBAT Clock Source reg = 0x%08x", vb_clk_src);
	print_periph_clock_src(vb_clk_src);
}

static void vbat_power_fail(void)
{
	mem_addr_t vbr_base = ((mem_addr_t)DT_REG_ADDR_BY_IDX(DT_NODELABEL(pcr), 1));
	uint32_t pfrs = sys_read32(vbr_base + XEC_VBR_PFR_SR_OFS);

	LOG_INF("MEC172x/4x/5x VBAT Power-Fail-Reset-Status = 0x%x", pfrs);

	pr_vbat_pfrs(pfrs);

	/* clear VBAT powered status */
	sys_write32(UINT32_MAX, vbr_base + XEC_VBR_PFR_SR_OFS);
}
#endif

static const mem_addr_t gpio_base = DT_REG_ADDR(DT_NODELABEL(gpio_000_036));
static const struct device *clkdev = DEVICE_DT_GET(DT_NODELABEL(pcr));

struct sys_clk {
	uint32_t id;
	char *name;
};

static const struct sys_clk sys_clocks[] = {
	{.id = MCHP_XEC_PCR_CLK_CORE, .name = "Core"},
	{.id = MCHP_XEC_PCR_CLK_CPU, .name = "CPU"},
	{.id = MCHP_XEC_PCR_CLK_BUS, .name = "Bus"},
	{.id = MCHP_XEC_PCR_CLK_PERIPH, .name = "Periph"},
	{.id = MCHP_XEC_PCR_CLK_PERIPH_FAST, .name = "Periph-fast"},
	{.id = MCHP_XEC_PCR_CLK_PERIPH_SLOW, .name = "Periph-slow"},
	{.id = MCHP_XEC_PCR_CLK_XTAL, .name = "XTAL"},
};

int main(void)
{
	int rc = 0;
	uint32_t rate = 0U;
	uint32_t r = 0U;
	union clock_mchp_xec_subsys xec_clk_subsys;
	clock_control_subsys_t sys = NULL;
	bool clock_control_ready = false;

	LOG_INF("XEC Clock control driver sample");

	if (device_is_ready(clkdev)) {
		clock_control_ready = true;
		LOG_INF("XEC clock control driver is ready");
	} else {
		LOG_ERR("XEC clock control driver is not ready!");
	}

	vbat_power_fail();

	LOG_INF("32KHZ_IN is function 1 of GPIO 0165");
	r = sys_read32(gpio_base + MCHP_XEC_GPIO_CR1_OFS(MCHP_XEC_32KHZ_IN_PIN));
	LOG_INF("XEC GPIO 0165 Control = 0x%x", r);
	r = MEC_GPIO_CR1_MUX_GET(r);
	LOG_INF("Pin function = %u", r);

	vbat_clock_regs();
	pcr_clock_regs();

	if (clock_control_ready == true) {
		for (size_t i = 0; i < ARRAY_SIZE(sys_clocks); i++) {
			LOG_INF("API get rate for %s", sys_clocks[i].name);

			xec_clk_subsys.val = 0;
			xec_clk_subsys.bits.bus = sys_clocks[i].id;

			sys = (clock_control_subsys_t)&xec_clk_subsys;
			rate = 0U;
			rc = clock_control_get_rate(clkdev, sys, &rate);
			if (rc) {
				LOG_ERR("API error: %d", rc);
			} else {
				LOG_INF("rate = %u", rate);
			}
		}
	}

	return 0;
}
