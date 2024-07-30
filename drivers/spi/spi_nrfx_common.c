/*
 * Copyright (c) 2023, Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "spi_nrfx_common.h"
#include <zephyr/kernel.h>

int spi_nrfx_wake_init(const nrfx_gpiote_t *gpiote, uint32_t wake_pin)
{
	nrf_gpio_pin_pull_t pull_config = NRF_GPIO_PIN_PULLDOWN;
	uint8_t ch;
	nrfx_gpiote_trigger_config_t trigger_config = {
		.trigger = NRFX_GPIOTE_TRIGGER_HITOLO,
		.p_in_channel = &ch,
	};
	nrfx_gpiote_input_pin_config_t input_config = {
		.p_pull_config = &pull_config,
		.p_trigger_config = &trigger_config,
		.p_handler_config = NULL,
	};
	nrfx_err_t res;

	res = nrfx_gpiote_channel_alloc(gpiote, &ch);
	if (res != NRFX_SUCCESS) {
		return -ENODEV;
	}

	res = nrfx_gpiote_input_configure(gpiote, wake_pin, &input_config);
	if (res != NRFX_SUCCESS) {
		nrfx_gpiote_channel_free(gpiote, ch);
		return -EIO;
	}

	return 0;
}

int spi_nrfx_wake_request(const nrfx_gpiote_t *gpiote, uint32_t wake_pin)
{
	nrf_gpiote_event_t trigger_event = nrfx_gpiote_in_event_get(gpiote, wake_pin);
	uint32_t start_cycles;
	uint32_t max_wait_cycles =
		DIV_ROUND_UP(CONFIG_SPI_NRFX_WAKE_TIMEOUT_US *
				 CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC,
			     1000000);
	int err = 0;

	/* Enable the trigger (a high-to-low transition) without its interrupt.
	 * The expected time to wait is quite short so it is not worth paying
	 * the overhead of context switching to handle the interrupt.
	 */
	nrfx_gpiote_trigger_enable(gpiote, wake_pin, false);
	/* Enable pull-up on the WAKE line. After the slave device sees the
	 * WAKE line going high, it will force the line to go low. This will
	 * be caught by the enabled trigger and the loop below waits for that.
	 */
	nrf_gpio_cfg_input(wake_pin, NRF_GPIO_PIN_PULLUP);

	start_cycles = k_cycle_get_32();
	while (!nrf_gpiote_event_check(gpiote->p_reg, trigger_event)) {
		uint32_t elapsed_cycles = k_cycle_get_32() - start_cycles;

		if (elapsed_cycles >= max_wait_cycles) {
			err = -ETIMEDOUT;
			break;
		}
	}

	nrfx_gpiote_trigger_disable(gpiote, wake_pin);
	nrf_gpio_cfg_input(wake_pin, NRF_GPIO_PIN_PULLDOWN);

	return err;
}
