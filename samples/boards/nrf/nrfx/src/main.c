/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>

#include <nrfx_gpiote.h>
#include <helpers/nrfx_gppi.h>
#if defined(DPPI_PRESENT)
#include <nrfx_dppi.h>
#else
#include <nrfx_ppi.h>
#endif

#include <zephyr/logging/log.h>
#include <zephyr/irq.h>
LOG_MODULE_REGISTER(nrfx_sample, LOG_LEVEL_INF);

#define INPUT_PIN	DT_GPIO_PIN(DT_ALIAS(sw0), gpios)
#define OUTPUT_PIN	DT_GPIO_PIN(DT_ALIAS(led0), gpios)

static void button_handler(nrfx_gpiote_pin_t pin,
			   nrfx_gpiote_trigger_t trigger,
			   void *context)
{
	LOG_INF("GPIO input event callback");
}

void main(void)
{
	LOG_INF("nrfx_gpiote sample on %s", CONFIG_BOARD);

	nrfx_err_t err;
	uint8_t in_channel, out_channel;
	uint8_t ppi_channel;

	/* Connect GPIOTE_0 IRQ to nrfx_gpiote_irq_handler */
	IRQ_CONNECT(DT_IRQN(DT_NODELABEL(gpiote)),
		    DT_IRQ(DT_NODELABEL(gpiote), priority),
		    nrfx_isr, nrfx_gpiote_irq_handler, 0);

	/* Initialize GPIOTE (the interrupt priority passed as the parameter
	 * here is ignored, see nrfx_glue.h).
	 */
	err = nrfx_gpiote_init(0);
	if (err != NRFX_SUCCESS) {
		LOG_ERR("nrfx_gpiote_init error: 0x%08X", err);
		return;
	}

	err = nrfx_gpiote_channel_alloc(&in_channel);
	if (err != NRFX_SUCCESS) {
		LOG_ERR("Failed to allocate in_channel, error: 0x%08X", err);
		return;
	}

	err = nrfx_gpiote_channel_alloc(&out_channel);
	if (err != NRFX_SUCCESS) {
		LOG_ERR("Failed to allocate out_channel, error: 0x%08X", err);
		return;
	}

	/* Initialize input pin to generate event on high to low transition
	 * (falling edge) and call button_handler()
	 */
	static const nrfx_gpiote_input_config_t input_config = {
		.pull = NRF_GPIO_PIN_PULLUP,
	};
	const nrfx_gpiote_trigger_config_t trigger_config = {
		.trigger = NRFX_GPIOTE_TRIGGER_HITOLO,
		.p_in_channel = &in_channel,
	};
	static const nrfx_gpiote_handler_config_t handler_config = {
		.handler = button_handler,
	};
	err = nrfx_gpiote_input_configure(INPUT_PIN,
					  &input_config,
					  &trigger_config,
					  &handler_config);
	if (err != NRFX_SUCCESS) {
		LOG_ERR("nrfx_gpiote_input_configure error: 0x%08X", err);
		return;
	}

	/* Initialize output pin. SET task will turn the LED on,
	 * CLR will turn it off and OUT will toggle it.
	 */
	static const nrfx_gpiote_output_config_t output_config = {
		.drive = NRF_GPIO_PIN_S0S1,
		.input_connect = NRF_GPIO_PIN_INPUT_DISCONNECT,
		.pull = NRF_GPIO_PIN_NOPULL,
	};
	const nrfx_gpiote_task_config_t task_config = {
		.task_ch = out_channel,
		.polarity = NRF_GPIOTE_POLARITY_TOGGLE,
		.init_val = 1,
	};
	err = nrfx_gpiote_output_configure(OUTPUT_PIN,
					   &output_config,
					   &task_config);
	if (err != NRFX_SUCCESS) {
		LOG_ERR("nrfx_gpiote_output_configure error: 0x%08X", err);
		return;
	}

	nrfx_gpiote_trigger_enable(INPUT_PIN, true);
	nrfx_gpiote_out_task_enable(OUTPUT_PIN);

	LOG_INF("nrfx_gpiote initialized");

	/* Allocate a (D)PPI channel. */
	err = nrfx_gppi_channel_alloc(&ppi_channel);
	if (err != NRFX_SUCCESS) {
		LOG_ERR("nrfx_gppi_channel_alloc error: 0x%08X", err);
		return;
	}

	/* Configure endpoints of the channel so that the input pin event is
	 * connected with the output pin OUT task. This means that each time
	 * the button is pressed, the LED pin will be toggled.
	 */
	nrfx_gppi_channel_endpoints_setup(ppi_channel,
		nrfx_gpiote_in_event_addr_get(INPUT_PIN),
		nrfx_gpiote_out_task_addr_get(OUTPUT_PIN));

	/* Enable the channel. */
	nrfx_gppi_channels_enable(BIT(ppi_channel));

	LOG_INF("(D)PPI configured, leaving main()");
}
