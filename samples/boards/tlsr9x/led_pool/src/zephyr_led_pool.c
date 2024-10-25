/*
 * Copyright (c) 2024 Telink Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "zephyr_led_pool.h"
#include <stdlib.h>
#include <stdarg.h>

/* Auxiliary data to support blink */
struct led_pool_aux_data {
	enum led_state  state;
	k_ticks_t       t_event;
	k_timeout_t     t_on;
	k_timeout_t     t_off;
	bool            current_state;
};

/* Get timeout to next event in auxiliary data */
static k_timeout_t led_pool_aux_timeout(const struct led_pool_data *led_pool)
{
	k_timeout_t timeout = {
		.ticks = K_TICKS_FOREVER
	};
	const struct led_pool_aux_data *led_pool_aux =
		(const struct led_pool_aux_data *)led_pool->aux;
	size_t led_pool_aux_len = led_pool->out_len;

	for (size_t i = 0; i < led_pool_aux_len; i++) {
		if (led_pool_aux[i].state == LED_BLINK) {
			k_ticks_t t_next = led_pool_aux[i].t_event +
				(led_pool_aux[i].current_state ?
				led_pool_aux[i].t_on.ticks : led_pool_aux[i].t_off.ticks);
			k_ticks_t t_now = sys_clock_tick_get();
			k_timeout_t this = {
				.ticks = t_next > t_now ? t_next - t_now : 0
			};
			if (timeout.ticks == K_TICKS_FOREVER || this.ticks < timeout.ticks) {
				timeout.ticks = this.ticks;
			}
		}
	}
	return timeout;
}

/* Process events in auxiliary data */
static void led_pool_aux_update(const struct led_pool_data *led_pool)
{
	struct led_pool_aux_data *led_pool_aux =
		(struct led_pool_aux_data *)led_pool->aux;
	size_t led_pool_aux_len = led_pool->out_len;

	for (size_t i = 0; i < led_pool_aux_len; i++) {
		if (led_pool_aux[i].state == LED_BLINK) {
			k_ticks_t t_next = led_pool_aux[i].t_event +
				(led_pool_aux[i].current_state ?
				led_pool_aux[i].t_on.ticks : led_pool_aux[i].t_off.ticks);
			k_ticks_t t_now = sys_clock_tick_get();

			if (t_next <= t_now) {
				led_pool_aux[i].current_state = !led_pool_aux[i].current_state;
				led_pool_aux[i].t_event = t_now;
				(void) gpio_pin_set_dt(
					&led_pool->out[i], led_pool_aux[i].current_state);
			}
		}
	}
}

/* Led pool worker */
static void led_pool_event_work(struct k_work *item)
{
	struct led_pool_data *led_pool =
		CONTAINER_OF(k_work_delayable_from_work(item), struct led_pool_data, work);

	led_pool_aux_update(led_pool);
	(void) k_work_reschedule(&led_pool->work, led_pool_aux_timeout(led_pool));
}

/* Public APIs */

bool led_pool_init(struct led_pool_data *led_pool)
{
	bool result = true;

	do {
		if (!led_pool->out_len) {
			result = false;
			break;
		}
		/* check if all GPIOs are ready */
		for (size_t i = 0; i < led_pool->out_len; i++) {
			if (!gpio_is_ready_dt(&led_pool->out[i])) {
				result = false;
				break;
			}
		}
		if (!result) {
			break;
		}
		/* init all GPIOs are ready */
		for (size_t i = 0; i < led_pool->out_len; i++) {
			if (gpio_pin_configure_dt(&led_pool->out[i], GPIO_OUTPUT)) {
				result = false;
				break;
			}
			if (gpio_pin_set_dt(&led_pool->out[i], 0)) {
				result = false;
				break;
			}
		}
		if (!result) {
			break;
		}
		/* set auxiliary blink structure */
		struct led_pool_aux_data *led_pool_aux = (struct led_pool_aux_data *)
			malloc(sizeof(struct led_pool_aux_data) * led_pool->out_len);

		if (!led_pool_aux) {
			result = false;
			break;
		}

		for (size_t i = 0; i < led_pool->out_len; i++) {
			led_pool_aux[i].state = LED_OFF;
		}
		led_pool->aux = led_pool_aux;
		/* init work */
		k_work_init_delayable(&led_pool->work, led_pool_event_work);
		if (k_work_reschedule(&led_pool->work, led_pool_aux_timeout(led_pool)) < 0) {
			result = false;
			break;
		}
		/* all done */
	} while (0);

	return result;
}

bool led_pool_set(struct led_pool_data *led_pool, size_t led, enum led_state state, ...)
{
	bool result = false;

	if (led < led_pool->out_len) {
		if (state == LED_ON || state == LED_OFF) {
			if (!gpio_pin_set_dt(&led_pool->out[led], state)) {
				struct led_pool_aux_data *led_pool_aux = led_pool->aux;

				led_pool_aux[led].state = state;
				if (k_work_reschedule(&led_pool->work,
					led_pool_aux_timeout(led_pool)) >= 0) {
					result = true;
				}
			}
		} else if (state == LED_BLINK) {
			if (!gpio_pin_set_dt(&led_pool->out[led], 1)) {
				struct led_pool_aux_data *led_pool_aux = led_pool->aux;
				va_list argptr;

				va_start(argptr, state);
				led_pool_aux[led].state = state;
				led_pool_aux[led].t_event = sys_clock_tick_get();
				led_pool_aux[led].t_on    = va_arg(argptr, k_timeout_t);
				led_pool_aux[led].t_off   = va_arg(argptr, k_timeout_t);
				led_pool_aux[led].current_state = true;
				va_end(argptr);
				if (k_work_reschedule(&led_pool->work,
					led_pool_aux_timeout(led_pool)) >= 0) {
					result = true;
				}
			}
		}
	}
	return result;
}
