/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>

#include <nrfx_gpiote.h>
#include <nrfx_dppi.h>

#include <logging/log.h>
LOG_MODULE_REGISTER(nrfx_sample, LOG_LEVEL_INF);

#define INPUT_PIN	DT_GPIO_PIN(DT_ALIAS(sw0), gpios)
#define OUTPUT_PIN	DT_GPIO_PIN(DT_ALIAS(led0), gpios)

static void button_handler(nrfx_gpiote_pin_t pin, nrf_gpiote_polarity_t action)
{
	LOG_INF("GPIO input event callback");
}

void main(void)
{
	LOG_INF("nrfx_gpiote sample on %s", CONFIG_BOARD);

	nrfx_err_t err;

	/* Connect GPIOTE_0 IRQ to nrfx_gpiote_irq_handler */
	IRQ_CONNECT(DT_IRQN(DT_NODELABEL(gpiote)),
		    DT_IRQ(DT_NODELABEL(gpiote), priority),
		    nrfx_isr, nrfx_gpiote_irq_handler, 0);

	/* Initialize GPIOTE (the interrupt priority passed as the parameter
	 * here is ignored, see nrfx_glue.h).
	 */
	err = nrfx_gpiote_init(0);
	if (err != NRFX_SUCCESS) {
		LOG_ERR("nrfx_gpiote_init error: %08x", err);
		return;
	}

	nrfx_gpiote_in_config_t const in_config = {
		.sense = NRF_GPIOTE_POLARITY_HITOLO,
		.pull = NRF_GPIO_PIN_PULLUP,
		.is_watcher = false,
		.hi_accuracy = true,
		.skip_gpio_setup = false,
	};

	/* Initialize input pin to generate event on high to low transition
	 * (falling edge) and call button_handler()
	 */
	err = nrfx_gpiote_in_init(INPUT_PIN, &in_config, button_handler);
	if (err != NRFX_SUCCESS) {
		LOG_ERR("nrfx_gpiote_in_init error: %08x", err);
		return;
	}

	nrfx_gpiote_out_config_t const out_config = {
		.action = NRF_GPIOTE_POLARITY_TOGGLE,
		.init_state = 1,
		.task_pin = true,
	};

	/* Initialize output pin. SET task will turn the LED on,
	 * CLR will turn it off and OUT will toggle it.
	 */
	err = nrfx_gpiote_out_init(OUTPUT_PIN, &out_config);
	if (err != NRFX_SUCCESS) {
		LOG_ERR("nrfx_gpiote_out_init error: %08x", err);
		return;
	}

	nrfx_gpiote_in_event_enable(INPUT_PIN, true);
	nrfx_gpiote_out_task_enable(OUTPUT_PIN);

	LOG_INF("nrfx_gpiote initialized");

	/* Initialize DPPI channel */
	uint8_t channel;

	err = nrfx_dppi_channel_alloc(&channel);
	if (err != NRFX_SUCCESS) {
		LOG_ERR("nrfx_dppi_channel_alloc error: %08x", err);
		return;
	}

	/* Configure input pin to publish to the DPPI channel and output pin to
	 * receive events from the DPPI channel. Note that output pin is
	 * subscribed using the OUT task. This means that each time the button
	 * is pressed, the LED pin will be toggled.
	 */
	nrf_gpiote_publish_set(
		NRF_GPIOTE,
		nrfx_gpiote_in_event_get(INPUT_PIN),
		channel);
	nrf_gpiote_subscribe_set(
		NRF_GPIOTE,
		nrfx_gpiote_out_task_get(OUTPUT_PIN),
		channel);

	/* Enable DPPI channel */
	err = nrfx_dppi_channel_enable(channel);
	if (err != NRFX_SUCCESS) {
		LOG_ERR("nrfx_dppi_channel_enable error: %08x", err);
		return;
	}

	LOG_INF("DPPI configured, leaving main()");
}
