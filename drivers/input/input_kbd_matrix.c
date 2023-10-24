/*
 * Copyright 2023 Google LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/kernel.h>

#include "input_kbd_matrix.h"

int input_kbd_matrix_common_init(const struct device *dev)
{
	const struct input_kbd_matrix_common_config *cfg = dev->config;
	const struct input_kbd_matrix_api *api = &cfg->api;
	struct input_kbd_matrix_common_data *const data = dev->data;

	k_thread_create(&data->thread, data->thread_stack,
			CONFIG_INPUT_KBD_MATRIX_THREAD_STACK_SIZE,
			api->polling_thread, (void *)dev, NULL, NULL,
			K_PRIO_COOP(4), 0, K_NO_WAIT);

	k_thread_name_set(&data->thread, dev->name);

	return 0;
}
