/*
 * Copyright (c) 2025 Core Devices LLC
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT sifli_sf32lb52x_pinmux

#include <zephyr/arch/cpu.h>
#include <zephyr/devicetree.h>
#include <zephyr/dt-bindings/pinctrl/sf32lb52x-pinctrl.h>
#include <zephyr/drivers/clock_control/sf32lb.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/sys/util.h>

struct sf32lb52x_pinctrl_config {
	uintptr_t pad_sa;
	uintptr_t pad_pa;
	uintptr_t cfg;
	struct sf32lb_clock_dt_spec clock;
};

#define SF32LB_PINMUX_MSK                                                                          \
	SF32LB_FSEL_MSK | SF32LB_PE_MSK | SF32LB_PS_MSK | SF32LB_IE_MSK | SF32LB_IS_MSK |          \
		SF32LB_SR_MSK | SF32LB_DS0_MSK

static int pinctrl_configure_pin(pinctrl_soc_pin_t pin)
{
	const struct device *dev = DEVICE_DT_INST_GET(0);
	const struct sf32lb52x_pinctrl_config *config = dev->config;
	uintptr_t pad;
	uint8_t pinr_offset;

	/* configure HPSYS_CFG *_PINR if applicable */
	pinr_offset = FIELD_GET(SF32LB_PINR_OFFSET_MSK, pin);
	if (pinr_offset != 0U) {
		uint32_t pinr_msk;
		uint32_t val;

		pinr_msk = 0xFFU << (8U * FIELD_GET(SF32LB_PINR_FIELD_MSK, pin));
		val = sys_read32(config->cfg + pinr_offset);
		val &= ~pinr_msk;
		val |= FIELD_PREP(pinr_msk, FIELD_GET(SF32LB_PAD_MSK, pin));
		sys_write32(val, config->cfg + pinr_offset);
	}

	/* configure HPSYS_PINMUX */
	switch (FIELD_GET(SF32LB_PORT_MSK, pin)) {
	case SF32LB_PORT_SA:
		pad = config->pad_sa;
		break;
	case SF32LB_PORT_PA:
		pad = config->pad_pa;
		break;
	default:
		return -EINVAL;
	}

	pad += FIELD_GET(SF32LB_PAD_MSK, pin) * 4U;

	sys_write32(FIELD_GET(SF32LB_PINMUX_MSK, pin), pad);

	return 0;
}

int pinctrl_configure_pins(const pinctrl_soc_pin_t *pins, uint8_t pin_cnt, uintptr_t reg)
{
	ARG_UNUSED(reg);

	for (uint8_t i = 0U; i < pin_cnt; i++) {
		int ret;

		ret = pinctrl_configure_pin(pins[i]);
		if (ret < 0) {
			return ret;
		}
	}

	return 0;
}

static int sf32lb52x_pinctrl_init(const struct device *dev)
{
	const struct sf32lb52x_pinctrl_config *const config = dev->config;

	if (!sf3232lb_clock_is_ready_dt(&config->clock)) {
		return -ENODEV;
	}

	return sf32lb_clock_control_on_dt(&config->clock);
}

static const struct sf32lb52x_pinctrl_config config = {
	.pad_sa = DT_INST_REG_ADDR_BY_NAME(0, pad_sa),
	.pad_pa = DT_INST_REG_ADDR_BY_NAME(0, pad_pa),
	.cfg = DT_REG_ADDR(DT_INST_PHANDLE(0, sifli_cfg)),
	.clock = SF32LB_CLOCK_DT_INST_SPEC_GET(0),
};

DEVICE_DT_INST_DEFINE(0, sf32lb52x_pinctrl_init, NULL, NULL, &config, PRE_KERNEL_1,
		      CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, NULL);
