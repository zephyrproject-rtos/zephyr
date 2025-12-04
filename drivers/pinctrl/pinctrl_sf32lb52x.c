/*
 * Copyright (c) 2025 Core Devices LLC
 * Copyright (c) 2025 SiFli Technologies(Nanjing) Co., Ltd
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

static int pinctrl_configure_pin(pinctrl_soc_pin_t pin)
{
	const struct device *dev = DEVICE_DT_INST_GET(0);
	const struct sf32lb52x_pinctrl_config *config = dev->config;
	uintptr_t pad;
	uint8_t pinr_offset;
	uint32_t val;
	uint8_t port = FIELD_GET(SF32LB_PORT_MSK, pin);
	uint8_t pad_num = FIELD_GET(SF32LB_PAD_MSK, pin);
	uint8_t ds_idx = FIELD_GET(SF32LB_DS_IDX_MSK, pin);
	uint8_t ds_reg;

	/*
	 * PA39-PA42 only have DS1 bit (no DS0), supports only 4mA (DS1=0) or 20mA (DS1=1).
	 * - For 2mA/4mA (idx 0,2): use 4mA (DS1=0, reg=0)
	 * - For 8mA/12mA (idx 1,3): invalid, return error
	 * - For 20mA (idx 4): use 20mA (DS1=1, reg=1)
	 * Other pins: 20mA (idx 4) is invalid, ds_idx maps directly to register value.
	 */
	if ((port == SF32LB_PORT_PA) && (pad_num >= 39U) && (pad_num <= 42U)) {
		if (ds_idx == 4U) {
			/* 20mA is valid for PA39-42 */
			ds_reg = 1U;
		} else if ((ds_idx == 0U) || (ds_idx == 2U)) {
			/* 2mA/4mA -> 4mA (DS1=0) */
			ds_reg = 0U;
		} else {
			/* 8mA/12mA not supported on PA39-42 */
			return -EINVAL;
		}
	} else {
		/* Normal pins: 20mA is not valid, ds_idx is the register value */
		if (ds_idx == 4U) {
			return -EINVAL;
		}
		ds_reg = ds_idx;
	}

	/* configure HPSYS_CFG *_PINR if applicable */
	pinr_offset = FIELD_GET(SF32LB_PINR_OFFSET_MSK, pin);
	if (pinr_offset != 0U) {
		uint32_t pinr_msk;

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

	val = sys_read32(pad);
	val &= ~SF32LB_PINMUX_CFG_MSK;
	val |= (pin & (SF32LB_PINMUX_CFG_MSK & ~SF32LB_DS_MSK));
	val |= FIELD_PREP(SF32LB_DS_MSK, ds_reg);

	sys_write32(val, pad);

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

	if (!sf32lb_clock_is_ready_dt(&config->clock)) {
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
