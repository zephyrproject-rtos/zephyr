/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>

#include <nrfx_gpiote.h>
#include <helpers/nrfx_gppi.h>

#include <zephyr/logging/log.h>
#include <zephyr/irq.h>
LOG_MODULE_REGISTER(nrfx_sample, LOG_LEVEL_INF);

#define INPUT_PIN	NRF_DT_GPIOS_TO_PSEL(DT_ALIAS(sw0), gpios)
#define OUTPUT_PIN	NRF_DT_GPIOS_TO_PSEL(DT_ALIAS(led0), gpios)

#define GPIOTE_INST	NRF_DT_GPIOTE_INST(DT_ALIAS(sw0), gpios)
#define GPIOTE_NODE	DT_NODELABEL(_CONCAT(gpiote, GPIOTE_INST))

BUILD_ASSERT(NRF_DT_GPIOTE_INST(DT_ALIAS(led0), gpios) == GPIOTE_INST,
	"Both sw0 and led0 GPIOs must use the same GPIOTE instance");
BUILD_ASSERT(IS_ENABLED(_CONCAT(CONFIG_, _CONCAT(NRFX_GPIOTE, GPIOTE_INST))),
	"NRFX_GPIOTE" STRINGIFY(GPIOTE_INST) " must be enabled in Kconfig");


static void button_handler(nrfx_gpiote_pin_t pin,
			   nrfx_gpiote_trigger_t trigger,
			   void *context)
{
	LOG_INF("GPIO input event callback");
}

int main(void)
{
	LOG_INF("nrfx_gpiote sample on %s", CONFIG_BOARD);

	nrfx_err_t err;
	uint8_t in_channel, out_channel;
	uint8_t ppi_channel;
	const nrfx_gpiote_t gpiote = NRFX_GPIOTE_INSTANCE(GPIOTE_INST);

	/* Connect GPIOTE instance IRQ to irq handler */
	IRQ_CONNECT(DT_IRQN(GPIOTE_NODE), DT_IRQ(GPIOTE_NODE, priority), nrfx_isr,
		    NRFX_CONCAT(nrfx_gpiote_, GPIOTE_INST, _irq_handler), 0);

	/* Initialize GPIOTE (the interrupt priority passed as the parameter
	 * here is ignored, see nrfx_glue.h).
	 */
	err = nrfx_gpiote_init(&gpiote, 0);
	if (err != NRFX_SUCCESS) {
		LOG_ERR("nrfx_gpiote_init error: 0x%08X", err);
		return 0;
	}

	err = nrfx_gpiote_channel_alloc(&gpiote, &in_channel);
	if (err != NRFX_SUCCESS) {
		LOG_ERR("Failed to allocate in_channel, error: 0x%08X", err);
		return 0;
	}

	err = nrfx_gpiote_channel_alloc(&gpiote, &out_channel);
	if (err != NRFX_SUCCESS) {
		LOG_ERR("Failed to allocate out_channel, error: 0x%08X", err);
		return 0;
	}

	/* Initialize input pin to generate event on high to low transition
	 * (falling edge) and call button_handler()
	 */
	static const nrf_gpio_pin_pull_t pull_config = NRF_GPIO_PIN_PULLUP;
	nrfx_gpiote_trigger_config_t trigger_config = {
		.trigger = NRFX_GPIOTE_TRIGGER_HITOLO,
		.p_in_channel = &in_channel,
	};
	static const nrfx_gpiote_handler_config_t handler_config = {
		.handler = button_handler,
	};
	nrfx_gpiote_input_pin_config_t input_config = {
		.p_pull_config = &pull_config,
		.p_trigger_config = &trigger_config,
		.p_handler_config = &handler_config
	};

	err = nrfx_gpiote_input_configure(&gpiote, INPUT_PIN, &input_config);

	if (err != NRFX_SUCCESS) {
		LOG_ERR("nrfx_gpiote_input_configure error: 0x%08X", err);
		return 0;
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
	err = nrfx_gpiote_output_configure(&gpiote, OUTPUT_PIN,
					   &output_config,
					   &task_config);
	if (err != NRFX_SUCCESS) {
		LOG_ERR("nrfx_gpiote_output_configure error: 0x%08X", err);
		return 0;
	}

	nrfx_gpiote_trigger_enable(&gpiote, INPUT_PIN, true);
	nrfx_gpiote_out_task_enable(&gpiote, OUTPUT_PIN);

	LOG_INF("nrfx_gpiote initialized");

	/* Allocate a (D)PPI channel. */
	err = nrfx_gppi_channel_alloc(&ppi_channel);
	if (err != NRFX_SUCCESS) {
		LOG_ERR("nrfx_gppi_channel_alloc error: 0x%08X", err);
		return 0;
	}

	/* Configure endpoints of the channel so that the input pin event is
	 * connected with the output pin OUT task. This means that each time
	 * the button is pressed, the LED pin will be toggled.
	 */
	nrfx_gppi_channel_endpoints_setup(ppi_channel,
		nrfx_gpiote_in_event_address_get(&gpiote, INPUT_PIN),
		nrfx_gpiote_out_task_address_get(&gpiote, OUTPUT_PIN));

	/* Enable the channel. */
	nrfx_gppi_channels_enable(BIT(ppi_channel));

	LOG_INF("(D)PPI configured, leaving main()");
	return 0;
}
