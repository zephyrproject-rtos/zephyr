/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>
#include <zephyr/pm/event_domain.h>

#define SAMPLE_PM_EVENT_DOMAIN PM_EVENT_DOMAIN_DT_GET(SAMPLE_EVENT_DOMAIN);

#if DT_NODE_EXISTS(DT_ALIAS(request_button))
#define SAMPLE_REQUEST_BUTTON DT_ALIAS(request_button)
#else
#define SAMPLE_REQUEST_BUTTON DT_NODELABEL(button0)
#endif

#if DT_NODE_EXISTS(DT_ALIAS(response_led))
#define SAMPLE_RESPONSE_LED DT_ALIAS(response_led)
#else
#define SAMPLE_RESPONSE_LED DT_NODELABEL(led0)
#endif

#define SAMPLE_REQUEST_GPIOS   GPIO_DT_SPEC_GET(SAMPLE_REQUEST_BUTTON, gpios)
#define SAMPLE_RESPONSE_GPIOS  GPIO_DT_SPEC_GET(SAMPLE_RESPONSE_LED, gpios)

static const struct gpio_dt_spec sample_req_pin = SAMPLE_REQUEST_GPIOS;
static const struct gpio_dt_spec sample_resp_pin = SAMPLE_RESPONSE_GPIOS;
static K_SEM_DEFINE(sample_sem, 0, 1);
static struct gpio_callback sample_req_cb;

static void sample_req_cb_handler(const struct device *port,
				   struct gpio_callback *cb,
				   uint32_t pin)
{
	ARG_UNUSED(port);
	ARG_UNUSED(cb);
	ARG_UNUSED(pin);

	gpio_pin_set_dt(&sample_resp_pin, 1);
	k_sem_give(&sample_sem);
}

int main(void)
{
	int ret;

	ret = gpio_pin_configure_dt(&sample_resp_pin, GPIO_OUTPUT_INACTIVE);
	if (ret) {
		return ret;
	}

	gpio_init_callback(&sample_req_cb, sample_req_cb_handler, BIT(sample_req_pin.pin));
	ret = gpio_add_callback_dt(&sample_req_pin, &sample_req_cb);
	if (ret) {
		return ret;
	}

	ret = gpio_pin_configure_dt(&sample_req_pin, GPIO_INPUT);
	if (ret) {
		return ret;
	}

	ret = gpio_pin_interrupt_configure_dt(&sample_req_pin, GPIO_INT_EDGE_TO_ACTIVE);
	if (ret) {
		return ret;
	}

	while (1) {
		(void)k_sem_take(&sample_sem, K_FOREVER);
		k_msleep(100);
		gpio_pin_set_dt(&sample_resp_pin, 0);
	}

	return 0;
}
