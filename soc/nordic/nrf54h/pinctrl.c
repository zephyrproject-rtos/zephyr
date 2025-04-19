/*
 * Copyright (c) 2025 Nordic Semiconductor ASA.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <nrf/gpd.h>

static bool gpd_requested = false;

int nrf_pinctrl_hook_start(const pinctrl_soc_pin_t *pins, uint8_t pin_cnt,
			    uintptr_t reg)
{
	gpd_requested = false;

	return 0;
}

bool nrf_pinctrl_hook_custom_periph(struct nrf_pinctrl_pin *pin)
{
	(void)pin;
	return false;
}

int nrf_pinctrl_hook_pre_write(const pinctrl_soc_pin_t pin)
{
	if (NRF_GET_GPD_FAST_ACTIVE1(pin) == 1U) {
		if (!gpd_requested) {
			int ret;

			ret = nrf_gpd_request(NRF_GPD_SLOW_ACTIVE);
			if (ret < 0) {
				return ret;
			}
			gpd_requested = true;
		}

		nrf_gpio_pin_retain_disable(NRF_GET_PIN(pin));
	}

	return 0;
}

int nrf_pinctrl_hook_post_write(const pinctrl_soc_pin_t pin)
{
	if (NRF_GET_GPD_FAST_ACTIVE1(pin) == 1U) {
		nrf_gpio_pin_retain_enable(NRF_GET_PIN(pin));
	}

	return 0;
}

int nrf_pinctrl_hook_end(const pinctrl_soc_pin_t *pins, uint8_t pin_cnt,
			    uintptr_t reg)
{
	if (gpd_requested) {
		return nrf_gpd_release(NRF_GPD_SLOW_ACTIVE);
	}

	return 0;
}
