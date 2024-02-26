/*
 * Copyright (c) 2024 Telink Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "zephyr_key_matrix.h"

/* Key matrix scanning period */
#define KEY_MATRIX_SCAN_PERIOD_MS      100

/* Poll key matrix and rise event on key change */
static void key_matrix_poll(struct key_matrix_data *key_matrix)
{
	for (size_t i = 0; i < key_matrix->col_len; i++) {
		gpio_pin_set_dt(&key_matrix->col[i], 1);
		for (size_t j = 0; j < key_matrix->row_len; j++) {
			size_t button_num = i * key_matrix->col_len + j;
			bool pin = gpio_pin_get_dt(&key_matrix->row[j]);

			if (pin !=
				(bool)(key_matrix->buttons[button_num / 8] & BIT(button_num % 8))) {
				WRITE_BIT(key_matrix->buttons[button_num / 8], button_num % 8, pin);
				if (key_matrix->on_button_change) {
					key_matrix->on_button_change(
						button_num, pin, key_matrix->context);
				}
			}
		}
		gpio_pin_set_dt(&key_matrix->col[i], 0);
	}
}

/* Key matrix scan worker */
static void key_matrix_scan_work(struct k_work *item)
{
	struct key_matrix_data *key_matrix =
		CONTAINER_OF(item, struct key_matrix_data, work);

	(void) k_work_schedule(&key_matrix->work, K_MSEC(KEY_MATRIX_SCAN_PERIOD_MS));
	key_matrix_poll(key_matrix);
}

/* Public APIs */

bool key_matrix_init(struct key_matrix_data *key_matrix)
{
	bool result = true;

	do {
		if (!key_matrix->col_len || !key_matrix->row_len) {
			result = false;
			break;
		}
		/* check if all GPIOs are ready */
		for (size_t i = 0; i < key_matrix->col_len; i++) {
			if (!gpio_is_ready_dt(&key_matrix->col[i])) {
				result = false;
				break;
			}
		}
		if (!result) {
			break;
		}
		for (size_t i = 0; i < key_matrix->row_len; i++) {
			if (!gpio_is_ready_dt(&key_matrix->row[i])) {
				result = false;
				break;
			}
		}
		if (!result) {
			break;
		}
		/* init all GPIOs are ready */
		for (size_t i = 0; i < key_matrix->col_len; i++) {
			if (gpio_pin_configure_dt(&key_matrix->col[i], GPIO_OUTPUT_INACTIVE)) {
				result = false;
				break;
			}
		}
		if (!result) {
			break;
		}
		for (size_t i = 0; i < key_matrix->row_len; i++) {
			if (gpio_pin_configure_dt(&key_matrix->row[i], GPIO_INPUT)) {
				result = false;
				break;
			}
		}
		if (!result) {
			break;
		}
		/* set all keys as released */
		for (size_t i = 0;
			i < DIV_ROUND_UP(key_matrix->col_len * key_matrix->row_len, 8); i++) {
			key_matrix->buttons[i] = 0;
		}
		key_matrix->on_button_change = NULL;
		key_matrix->context = NULL;
		/* work init */
		k_work_init_delayable(&key_matrix->work, key_matrix_scan_work);
		while (k_work_schedule(&key_matrix->work, K_NO_WAIT) == -EBUSY) {
			k_usleep(10); /* Let other stuffs run */
		}
		/* all done */
	} while (0);

	return result;
}

void key_matrix_set_callback(struct key_matrix_data *key_matrix,
	key_matrix_on_button_change_t on_button_change, void *context)
{
	key_matrix->on_button_change = on_button_change;
	key_matrix->context = context;
}
