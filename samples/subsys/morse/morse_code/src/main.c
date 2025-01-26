/*
 * Copyright (c) 2024-2025 Freedom Veiculos Eletricos
 * Copyright (c) 2024-2025 O.S. Systems Software LTDA.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>

#include <zephyr/morse/morse.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(app, CONFIG_MORSE_LOG_LEVEL);

/**
 * @file Send data using morse code.
 */

const uint8_t tx_data[] = "paris";
uint8_t rx_data[sizeof(tx_data)] = {};
int rx_counter;

void morse_tx_cb_handler(void *ctx, int status)
{
	LOG_INF("TX status: %d", status);
}

void morse_rx_cb_handler(void *ctx, enum morse_rx_state status, uint32_t data)
{
	LOG_DBG("RX status: %d, 0x%02x, '%c'", status, data, data);

	rx_data[rx_counter++] = data;

	if (status == MORSE_RX_STATE_END_TRANSMISSION) {
		rx_data[rx_counter++] = 0x00;
		LOG_INF("RX Data: %s", rx_data);
	}
}

int main(void)
{
	const struct device *const morse = DEVICE_DT_GET(DT_NODELABEL(morse));

	if (!device_is_ready(morse)) {
		return 0;
	}

	rx_counter = 0;
	morse_manage_callbacks(morse, morse_tx_cb_handler, morse_rx_cb_handler, NULL);
	morse_send(morse, tx_data, strlen(tx_data));

	return 0;
}
