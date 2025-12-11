/*
 * Copyright (c) 2016 Open-RnD Sp. z o.o.
 * Copyright (c) 2021 Linaro Limited
 * Copyright (c) 2021 Nordic Semiconductor ASA
 * Copyright (c) 2024 Microchip Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT microchip_mec5_pinctrl

#include <zephyr/drivers/pinctrl.h>
#include <zephyr/dt-bindings/pinctrl/mchp-xec-pinctrl.h>
#include <mec_gpio_api.h>

static const struct mec_gpio_props cfg1[] = {
	{MEC_GPIO_OSEL_PROP_ID, MEC_GPIO_PROP_OSEL_CTRL},
	{MEC_GPIO_INPAD_DIS_PROP_ID, MEC_GPIO_PROP_INPAD_EN},
};

/* DT enable booleans take precedence over disable booleans.
 * We initially clear alternate output disable allowing us to set output state
 * in the control register. Hardware sets output state bit in both control and
 * parallel output register bits. Alternate output disable only controls which
 * register bit is writable by the EC. We also clear the input pad disable
 * bit because we need the input pin state and we don't know if the requested
 * alternate function is input or bi-directional.
 * Note 1: hardware allows input and output to be simultaneously enabled.
 * Note 2: hardware interrupt detection is only on the input path.
 */
static int mec5_config_pin(uint32_t pinmux, uint32_t altf)
{
	uint32_t conf = pinmux;
	uint32_t pin = 0, temp = 0;
	int ret = 0;
	size_t idx = 0;
	struct mec_gpio_props cfg2[12];

	ret = mec_hal_gpio_pin_num(MCHP_XEC_PINMUX_PORT(pinmux), MCHP_XEC_PINMUX_PIN(pinmux), &pin);
	if (ret) {
		return -EINVAL;
	}

	ret = mec_hal_gpio_set_props(pin, cfg1, ARRAY_SIZE(cfg1));
	if (ret) {
		return -EIO;
	}

	/* slew rate */
	temp = (conf >> MCHP_XEC_SLEW_RATE_POS) & MCHP_XEC_SLEW_RATE_MSK0;
	if (temp != MCHP_XEC_SLEW_RATE_MSK0) {
		cfg2[idx].prop = MEC_GPIO_SLEW_RATE_ID;
		cfg2[idx].val = (uint8_t)MEC_GPIO_SLEW_RATE_SLOW;
		if (temp == MCHP_XEC_SLEW_RATE_FAST0) {
			cfg2[idx].val = (uint8_t)MEC_GPIO_SLEW_RATE_FAST;
		}
		idx++;
	}

	/* drive strength */
	temp = (conf >> MCHP_XEC_DRV_STR_POS) & MCHP_XEC_DRV_STR_MSK0;
	if (temp != MCHP_XEC_DRV_STR_MSK0) {
		cfg2[idx].prop = MEC_GPIO_DRV_STR_ID;
		cfg2[idx].val = (uint8_t)(temp - 1u);
		idx++;
	}

	/* Touch internal pull-up/pull-down? */
	cfg2[idx].prop = MEC_GPIO_PUD_PROP_ID;
	if (conf & BIT(MCHP_XEC_NO_PUD_POS)) {
		cfg2[idx++].val = MEC_GPIO_PROP_NO_PUD;
	} else if (conf & BIT(MCHP_XEC_PU_POS)) {
		cfg2[idx++].val = MEC_GPIO_PROP_PULL_UP;
	} else if (conf & BIT(MCHP_XEC_PD_POS)) {
		cfg2[idx++].val = MEC_GPIO_PROP_PULL_DN;
	}

	/* Touch output enable. We always enable input */
	if (conf & (BIT(MCHP_XEC_OUT_DIS_POS) | BIT(MCHP_XEC_OUT_EN_POS))) {
		cfg2[idx].prop = MEC_GPIO_DIR_PROP_ID;
		cfg2[idx].val = MEC_GPIO_PROP_DIR_IN;
		if (conf & BIT(MCHP_XEC_OUT_EN_POS)) {
			cfg2[idx].val = MEC_GPIO_PROP_DIR_OUT;
		}
		idx++;
	}

	/* Touch output state? Bit can be set even if the direction is input only */
	if (conf & (BIT(MCHP_XEC_OUT_LO_POS) | BIT(MCHP_XEC_OUT_HI_POS))) {
		cfg2[idx].prop = MEC_GPIO_CTRL_OUT_VAL_ID;
		cfg2[idx].val = 0u;
		if (conf & BIT(MCHP_XEC_OUT_HI_POS)) {
			cfg2[idx].val = 1u;
		}
		idx++;
	}

	/* Touch output buffer type? */
	if (conf & (BIT(MCHP_XEC_PUSH_PULL_POS) | BIT(MCHP_XEC_OPEN_DRAIN_POS))) {
		cfg2[idx].prop = MEC_GPIO_OBUFT_PROP_ID;
		cfg2[idx].val = MEC_GPIO_PROP_PUSH_PULL;
		if (conf & BIT(MCHP_XEC_OPEN_DRAIN_POS)) {
			cfg2[idx].val = MEC_GPIO_PROP_OPEN_DRAIN;
		}
		idx++;
	}

	/* Always touch power gate */
	cfg2[idx].prop = MEC_GPIO_PWRGT_PROP_ID;
	cfg2[idx].val = MEC_GPIO_PROP_PWRGT_VTR;
	if (conf & BIT(MCHP_XEC_PIN_LOW_POWER_POS)) {
		cfg2[idx].val = MEC_GPIO_PROP_PWRGT_OFF;
	}
	idx++;

	/* Always touch MUX (alternate function) */
	cfg2[idx].prop = MEC_GPIO_MUX_PROP_ID;
	cfg2[idx].val = (uint8_t)altf;
	idx++;

	/* Always touch invert of alternate function. Need another bit to avoid touching */
	cfg2[idx].prop = MEC_GPIO_FUNC_POL_PROP_ID;
	cfg2[idx].val = MEC_GPIO_PROP_FUNC_OUT_NON_INV;
	if (conf & BIT(MCHP_XEC_FUNC_INV_POS)) {
		cfg2[idx].val = MEC_GPIO_PROP_FUNC_OUT_INV;
	}
	idx++;

	/* HW sets output state set in control & parallel regs */
	ret = mec_hal_gpio_set_props(pin, cfg2, idx);
	if (ret) {
		return -EIO;
	}

	/* make output state in control read-only in control and read-write in parallel reg */
	ret = mec_hal_gpio_set_property(pin, MEC_GPIO_OSEL_PROP_ID, MEC_GPIO_PROP_OSEL_PAROUT);
	if (ret) {
		return -EIO;
	}

	return 0;
}

int pinctrl_configure_pins(const pinctrl_soc_pin_t *pins, uint8_t pin_cnt, uintptr_t reg)
{
	uint32_t pinmux, func;
	int ret;

	ARG_UNUSED(reg);

	for (uint8_t i = 0U; i < pin_cnt; i++) {
		pinmux = pins[i];

		func = MCHP_XEC_PINMUX_FUNC(pinmux);
		if (func >= MCHP_AFMAX) {
			return -EINVAL;
		}

		ret = mec5_config_pin(pinmux, func);
		if (ret < 0) {
			return ret;
		}
	}

	return 0;
}
