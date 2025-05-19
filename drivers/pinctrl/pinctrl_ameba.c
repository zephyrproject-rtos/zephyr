/*
 * Copyright (c) 2024 Realtek Semiconductor Corp.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* Include <soc.h> before <ameba_soc.h> to avoid redefining unlikely() macro */
#include <soc.h>
#include <ameba_soc.h>

#include <zephyr/drivers/pinctrl.h>

#define AMEBA_GET_PORT_NUM(pin_mux)  ((pin_mux >> 13) & 0x03)
#define AMEBA_GET_PIN_NUM(pin_mux)   ((pin_mux >> 8) & 0x1f)
#define AMEBA_GET_PIMNUX_ID(pin_mux) (pin_mux & 0xFF)

#define AMEBA_GPIO_PINNAME(PORT, PIN) (((PORT) << 5) | ((PIN) & 0x1F))

static int ameba_configure_pin(const pinctrl_soc_pin_t *pin)
{
	uint32_t port_idx, pin_idx;
	uint8_t gpio_pin;
	uint32_t function_id;

	port_idx = AMEBA_GET_PORT_NUM(pin->pinmux);
	pin_idx = AMEBA_GET_PIN_NUM(pin->pinmux);
	function_id = AMEBA_GET_PIMNUX_ID(pin->pinmux);
	gpio_pin = AMEBA_GPIO_PINNAME(port_idx, pin_idx);

	Pinmux_Config(gpio_pin, function_id);

	if (pin->pull_up) {
		PAD_PullCtrl(gpio_pin, GPIO_PuPd_UP);
		PAD_SleepPullCtrl(gpio_pin, GPIO_PuPd_UP);
	} else if (pin->pull_down) {
		PAD_PullCtrl(gpio_pin, GPIO_PuPd_DOWN);
		PAD_SleepPullCtrl(gpio_pin, GPIO_PuPd_DOWN);
	} else {
		PAD_PullCtrl(gpio_pin, GPIO_PuPd_NOPULL);
		PAD_SleepPullCtrl(gpio_pin, GPIO_PuPd_NOPULL);
	}

	/* default slew rate fast */
	if (pin->slew_rate_slow) {
		PAD_SlewRateCtrl(gpio_pin, PAD_SlewRate_Slow);
	}

	/* Set the PAD driving strength to PAD_DRV_ABILITITY_LOW, default PAD_DRV_ABILITITY_HIGH */
	if (pin->drive_strength_low) {
		PAD_DrvStrength(gpio_pin, PAD_DRV_ABILITITY_LOW);
	}

	/* default enable digital path input. */
	if (pin->digital_input_disable) {
		PAD_InputCtrl(gpio_pin, DISABLE);
	}

	/* default enable schmitt */
	if (pin->schmitt_disable) {
		PAD_SchmitCtrl(gpio_pin, DISABLE);
	}

	if (pin->swd_off) {
		Pinmux_Swdoff();
	}

	return 0;
}

int pinctrl_configure_pins(const pinctrl_soc_pin_t *pins, uint8_t pin_cnt, uintptr_t reg)
{
	int ret = 0;

	ARG_UNUSED(reg);

	for (int i = 0; i < pin_cnt; i++) {
		ret = ameba_configure_pin(&pins[i]);

		if (ret < 0) {
			return ret;
		}
	}

	return 0;
}
