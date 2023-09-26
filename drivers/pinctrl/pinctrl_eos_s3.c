/*
 * Copyright (c) 2023 Antmicro <www.antmicro.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT quicklogic_eos_s3_pinctrl

#include <zephyr/arch/cpu.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/logging/log.h>
#include <zephyr/dt-bindings/pinctrl/quicklogic-eos-s3-pinctrl.h>
#include <soc.h>

LOG_MODULE_REGISTER(pinctrl_eos_s3, CONFIG_PINCTRL_LOG_LEVEL);

#define FUNCTION_REGISTER(func)	(func >> 13)
#define PAD_FUNC_SEL_MASK	GENMASK(2, 0)
#define PAD_CTRL_SEL_BIT0	3
#define PAD_CTRL_SEL_BIT1	4
#define PAD_OUTPUT_EN_BIT	5
#define PAD_PULL_UP_BIT		6
#define PAD_PULL_DOWN_BIT	7
#define PAD_DRIVE_STRENGTH_BIT0	8
#define PAD_DRIVE_STRENGTH_BIT1	9
#define PAD_SLEW_RATE_BIT	10
#define PAD_INPUT_EN_BIT	11
#define PAD_SCHMITT_EN_BIT	12

/*
 * Program IOMUX_func_SEL register.
 */
static int pinctrl_eos_s3_input_selection(uint32_t pin, uint32_t sel_reg)
{
	volatile uint32_t *reg = (uint32_t *)IO_MUX_BASE;

	if (sel_reg <= IO_MUX_MAX_PAD_NR || sel_reg > IO_MUX_REG_MAX_OFFSET) {
		return -EINVAL;
	}
	reg += sel_reg;
	*reg = pin;

	return 0;
}

/*
 * Program IOMUX_PAD_x_CTRL register.
 */
static int pinctrl_eos_s3_set(uint32_t pin, uint32_t func)
{
	volatile uint32_t *reg = (uint32_t *)IO_MUX_BASE;

	if (pin > IO_MUX_REG_MAX_OFFSET) {
		return -EINVAL;
	}
	reg += pin;
	*reg = func;

	return 0;
}

static int pinctrl_eos_s3_configure_pin(const pinctrl_soc_pin_t *pin)
{
	uint32_t reg_value = 0;

	/* Set function. */
	reg_value |= (pin->iof & PAD_FUNC_SEL_MASK);

	/* Output enable is active low. */
	WRITE_BIT(reg_value, PAD_OUTPUT_EN_BIT, pin->output_enable ? 0 : 1);

	/* These are active high. */
	WRITE_BIT(reg_value, PAD_INPUT_EN_BIT, pin->input_enable);
	WRITE_BIT(reg_value, PAD_SLEW_RATE_BIT, pin->slew_rate);
	WRITE_BIT(reg_value, PAD_SCHMITT_EN_BIT, pin->schmitt_enable);
	WRITE_BIT(reg_value, PAD_CTRL_SEL_BIT0, pin->control_selection & BIT(0));
	WRITE_BIT(reg_value, PAD_CTRL_SEL_BIT1, pin->control_selection & BIT(1));

	switch (pin->drive_strength) {
	case 2:
		WRITE_BIT(reg_value, PAD_DRIVE_STRENGTH_BIT0, 0);
		WRITE_BIT(reg_value, PAD_DRIVE_STRENGTH_BIT1, 0);
		break;
	case 4:
		WRITE_BIT(reg_value, PAD_DRIVE_STRENGTH_BIT0, 1);
		WRITE_BIT(reg_value, PAD_DRIVE_STRENGTH_BIT1, 0);
		break;
	case 8:
		WRITE_BIT(reg_value, PAD_DRIVE_STRENGTH_BIT0, 0);
		WRITE_BIT(reg_value, PAD_DRIVE_STRENGTH_BIT1, 1);
		break;
	case 12:
		WRITE_BIT(reg_value, PAD_DRIVE_STRENGTH_BIT0, 1);
		WRITE_BIT(reg_value, PAD_DRIVE_STRENGTH_BIT1, 1);
		break;
	default:
		LOG_ERR("Selected drive-strength is not supported: %d\n", pin->drive_strength);
	}

	/* Enable pull-up by default; overwrite if any setting was chosen. */
	WRITE_BIT(reg_value, PAD_PULL_UP_BIT, 1);
	WRITE_BIT(reg_value, PAD_PULL_DOWN_BIT, 0);
	if (pin->high_impedance) {
		WRITE_BIT(reg_value, PAD_PULL_UP_BIT, 0);
	} else if (pin->pull_up | pin->pull_down) {
		WRITE_BIT(reg_value, PAD_PULL_UP_BIT, pin->pull_up);
		WRITE_BIT(reg_value, PAD_PULL_DOWN_BIT, pin->pull_down);
	}

	/* Program registers. */
	pinctrl_eos_s3_set(pin->pin, reg_value);
	if (pin->input_enable && FUNCTION_REGISTER(pin->iof)) {
		pinctrl_eos_s3_input_selection(pin->pin, FUNCTION_REGISTER(pin->iof));
	}
	return 0;
}

int pinctrl_configure_pins(const pinctrl_soc_pin_t *pins, uint8_t pin_cnt, uintptr_t reg)
{
	ARG_UNUSED(reg);

	for (int i = 0; i < pin_cnt; i++) {
		pinctrl_eos_s3_configure_pin(&pins[i]);
	}

	return 0;
}
