/*
 * Copyright (c) 2024 Freedom Veiculos Eletricos
 * Copyright (c) 2024 OS Systems
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>

#include <zephyr/drivers/misc/morse_code/morse_code.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(app, CONFIG_MORSE_CODE_LOG_LEVEL);

/**
 * @file Send data using morse code.
 */

const uint8_t data[] = "paris";

void morse_cb_handler(void *ctx, int status)
{
	LOG_INF("Status: %d", status);
}

int main(void)
{
	const struct device *const morse = DEVICE_DT_GET(DT_NODELABEL(morse));

	if (!device_is_ready(morse)) {
		return 0;
	}

	morse_code_manage_callback(morse, morse_cb_handler, NULL);
	morse_code_send(morse, data, strlen(data));

	return 0;
}
