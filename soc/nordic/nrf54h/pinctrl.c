/*
 * Copyright (c) 2025 Nordic Semiconductor ASA.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/drivers/pinctrl/pinctrl_nrf_common.h>
#include <nrf/gpd.h>

static bool gpd_requested;

int nrf_pinctrl_hook_start(const pinctrl_soc_pin_t *pins, uint8_t pin_cnt,
			   uintptr_t reg)
{
	gpd_requested = false;

	return 0;
}

bool nrf_pinctrl_hook_custom_periph(const struct nrf_pinctrl_pin *pin,
				    struct nrf_pinctrl_config *cfg)
{
	return false;
}

int nrf_pinctrl_hook_pre_write(const struct nrf_pinctrl_pin *pin, struct nrf_pinctrl_config *cfg)
{
	if (pin->gpd_fast_active1) {
		if (!gpd_requested) {
			int ret;

			ret = nrf_gpd_request(NRF_GPD_SLOW_ACTIVE);
			if (ret < 0) {
				return ret;
			}
			gpd_requested = true;
		}

		nrf_gpio_pin_retain_disable(pin->pin_num);
	}

	return 0;
}

int nrf_pinctrl_hook_post_write(const struct nrf_pinctrl_pin *pin, struct nrf_pinctrl_config *cfg)
{
	if (pin->gpd_fast_active1) {
		nrf_gpio_pin_retain_enable(pin->pin_num);
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
