/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>
#include <zephyr/pm/event_domain.h>

#define SAMPLE_PM_EVENT_DOMAIN_NODE DT_ALIAS(sample_event_domain)
#define SAMPLE_PM_EVENT_DOMAIN PM_EVENT_DOMAIN_DT_GET(SAMPLE_PM_EVENT_DOMAIN_NODE)

#if DT_NODE_EXISTS(DT_ALIAS(sample_request_button))
#define SAMPLE_REQUEST_BUTTON_NODE DT_ALIAS(sample_request_button)
#else
#define SAMPLE_REQUEST_BUTTON_NODE DT_NODELABEL(button0)
#endif

#if DT_NODE_EXISTS(DT_ALIAS(sample_response_led))
#define SAMPLE_RESPONSE_LED_NODE DT_ALIAS(sample_response_led)
#else
#define SAMPLE_RESPONSE_LED_NODE DT_NODELABEL(led0)
#endif

#define SAMPLE_REQUEST_GPIOS  GPIO_DT_SPEC_GET(SAMPLE_REQUEST_BUTTON_NODE, gpios)
#define SAMPLE_RESPONSE_GPIOS GPIO_DT_SPEC_GET(SAMPLE_RESPONSE_LED_NODE, gpios)

static const struct pm_event_domain *sample_event_domain = SAMPLE_PM_EVENT_DOMAIN;
PM_EVENT_DOMAIN_EVENT_DT_DEFINE(sample_event, SAMPLE_PM_EVENT_DOMAIN_NODE);
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
	const uint32_t *latencies_us;
	uint8_t latencies_us_size;
	uint8_t latencies_us_it;
	uint32_t latency_us;
	int64_t effective_uptime_ticks;

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

	latencies_us = pm_event_domain_get_event_latencies_us(sample_event_domain);
	latencies_us_size = pm_event_domain_get_event_latencies_us_size(sample_event_domain);
	latencies_us_it = 0;

	while (1) {
		latency_us = latencies_us[latencies_us_it];
		effective_uptime_ticks = pm_event_domain_request_event(&sample_event, latency_us);
		k_sleep(K_TIMEOUT_ABS_TICKS(effective_uptime_ticks));
		printk("latency in effect: %uus\n", latency_us);
		(void)k_sem_take(&sample_sem, K_FOREVER);
		k_msleep(100);
		gpio_pin_set_dt(&sample_resp_pin, 0);
		pm_event_domain_release_event(&sample_event);
		latencies_us_it = latencies_us_it == latencies_us_size - 1
				? 0
				: latencies_us_it + 1;
	}

	return 0;
}
