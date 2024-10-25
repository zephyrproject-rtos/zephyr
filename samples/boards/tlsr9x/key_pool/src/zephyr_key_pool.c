/*
 * Copyright (c) 2024 Telink Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "zephyr_key_pool.h"
#include <stdlib.h>

/* Key pool denouncing settle time */
#define KEY_POOL_DEBOUNCING_GUARD_MS     10

/* Auxiliary data to link key pool with pin isr */
struct key_pool_aux_data {
	struct key_pool_data         *key_pool;
	const struct device          *port;
	struct gpio_callback          callback;
};

/* Auxiliary function to get ports number in pool */
static size_t key_pool_port_number(const struct key_pool_data *key_pool)
{
	size_t port_num = 0;

	for (size_t i = 0; i < key_pool->inp_len; i++) {
		bool port_already = false;

		for (size_t j = 0; j < i; j++) {
			if (key_pool->inp[i].port == key_pool->inp[j].port) {
				port_already = true;
				break;
			}
		}
		if (!port_already) {
			port_num++;
		}
	}
	return port_num;
}

/* Poll key pool and rise event on key change */
static void key_pool_poll(struct key_pool_data *key_pool, bool init)
{
	for (size_t i = 0; i < key_pool->inp_len; i++) {
		bool pin = gpio_pin_get_dt(&key_pool->inp[i]);

		if (pin !=
			(bool)(key_pool->buttons[i / 8] & BIT(i % 8))) {
			WRITE_BIT(key_pool->buttons[i / 8], i % 8, pin);
			if (!init && key_pool->on_button_change) {
				key_pool->on_button_change(
					i, pin, key_pool->context);
			}
		}
	}
}

/* Key pool scan worker */
static void key_pool_event_work(struct k_work *item)
{
	struct key_pool_data *key_pool =
		CONTAINER_OF(k_work_delayable_from_work(item), struct key_pool_data, work);

	key_pool_poll(key_pool, false);
}

/* Key pool pin isr */
static void key_pool_on_pin_isr(const struct device *dev,
	struct gpio_callback *cb, uint32_t pins)
{
	struct key_pool_aux_data *key_pool_aux =
		CONTAINER_OF(cb, struct key_pool_aux_data, callback);

	(void) k_work_reschedule(&key_pool_aux->key_pool->work,
		K_MSEC(KEY_POOL_DEBOUNCING_GUARD_MS));
}

/* Public APIs */

bool key_pool_init(struct key_pool_data *key_pool)
{
	bool result = true;

	do {
		if (!key_pool->inp_len) {
			result = false;
			break;
		}
		/* check if all GPIOs are ready */
		for (size_t i = 0; i < key_pool->inp_len; i++) {
			if (!gpio_is_ready_dt(&key_pool->inp[i])) {
				result = false;
				break;
			}
		}
		if (!result) {
			break;
		}
		/* init all GPIOs are ready */
		for (size_t i = 0; i < key_pool->inp_len; i++) {
			if (gpio_pin_configure_dt(&key_pool->inp[i], GPIO_INPUT)) {
				result = false;
				break;
			}
			if (gpio_pin_interrupt_configure_dt(&key_pool->inp[i],
				GPIO_INT_EDGE_BOTH)) {
				result = false;
				break;
			}
		}
		if (!result) {
			break;
		}
		/* add callbacks to all ports */
		struct key_pool_aux_data *key_pool_aux = (struct key_pool_aux_data *)
			malloc(sizeof(struct key_pool_aux_data) * key_pool_port_number(key_pool));

		if (!key_pool_aux) {
			result = false;
			break;
		}

		key_pool->aux = key_pool_aux;
		size_t key_pool_aux_inited_cnt = 0;

		for (size_t i = 0; i < key_pool->inp_len; i++) {
			bool port_found = false;

			for (size_t j = 0; j < key_pool_aux_inited_cnt; j++) {
				if (key_pool->inp[i].port == key_pool_aux[j].port) {
					port_found = true;
					key_pool_aux[j].callback.pin_mask |=
						BIT(key_pool->inp[i].pin);
				}
			}
			if (!port_found) {
				key_pool_aux[key_pool_aux_inited_cnt].key_pool = key_pool;
				key_pool_aux[key_pool_aux_inited_cnt].port = key_pool->inp[i].port;
				gpio_init_callback(&key_pool_aux[key_pool_aux_inited_cnt].callback,
					key_pool_on_pin_isr, BIT(key_pool->inp[i].pin));
				key_pool_aux_inited_cnt++;
			}
		}
		for (size_t i = 0; i < key_pool_aux_inited_cnt; i++) {
			if (gpio_add_callback(key_pool_aux[i].port, &key_pool_aux[i].callback)) {
				result = false;
				break;
			}
		}
		if (!result) {
			break;
		}
		/* set all keys to current state */
		key_pool_poll(key_pool, true);
		/* init work */
		k_work_init_delayable(&key_pool->work, key_pool_event_work);
		/* all done */
	} while (0);

	return result;
}

void key_pool_set_callback(struct key_pool_data *key_pool,
	key_pool_on_button_change_t on_button_change, void *context)
{
	key_pool->on_button_change = on_button_change;
	key_pool->context = context;
}
