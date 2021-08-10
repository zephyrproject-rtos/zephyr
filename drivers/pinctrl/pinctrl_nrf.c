/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "common.h"

#include <device.h>

#include <hal/nrf_gpio.h>

int pinctrl_state_set(const struct device *dev, enum pinctrl_state_id id)
{
	int ret;
	const struct pinctrl_state *state;

	ret = pinctrl_lookup_state(dev, id, &state);
	if (ret < 0) {
		return ret;
	}

	/* configure peripheral pin selection */
	switch (dev->pinctrl->reg) {
	default:
		return -ENOTSUP;
	}

	/* configure pins GPIO settings */
	for (size_t i = 0U; i < state->pincfgs_cnt; i++) {
		nrf_gpio_cfg(PINCTRL_NRF_GET_PIN(state->pincfgs[i]),
			     PINCTRL_NRF_GET_DIR(state->pincfgs[i]),
			     PINCTRL_NRF_GET_INP(state->pincfgs[i]),
			     PINCTRL_NRF_GET_PULL(state->pincfgs[i]),
			     NRF_GPIO_PIN_S0S1,
			     NRF_GPIO_PIN_NOSENSE);
	}

	return 0;
}
