/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>

#include <zephyr/device.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/uart.h>

#define UART_NODE DT_ALIAS(uart)

/* obtain reference to pinctrl config (owned by device) */
PINCTRL_DT_DEV_CONFIG_DECLARE(UART_NODE);
struct pinctrl_dev_config *uart_config = PINCTRL_DT_DEV_CONFIG_GET(UART_NODE);

/* define alternate states and build alternate state */
PINCTRL_DT_STATE_PINS_DEFINE(DT_PATH(zephyr_user), uart_alt_default);
#if DT_PINCTRL_HAS_NAME(UART_NODE, reset)
PINCTRL_DT_STATE_PINS_DEFINE(DT_PATH(zephyr_user), uart_alt_reset);
#endif

static const struct pinctrl_state uart_alt[] = {
	PINCTRL_DT_STATE_INIT(uart_alt_default, PINCTRL_STATE_DEFAULT),
#if DT_PINCTRL_HAS_NAME(UART_NODE, reset)
	PINCTRL_DT_STATE_INIT(uart_alt_reset, PINCTRL_STATE_RESET),
#endif
};

static const struct device *const uart = DEVICE_DT_GET(UART_NODE);

static const char *const msg_a = "Hello World (A)!\r\n";
static const char *const msg_b = "Hello World (B)!\r\n";

int main(void)
{
	int ret;

	ret = device_get(uart);
	if (ret < 0) {
		printf("Failed to get UART device\n");
		return 0;
	}

	for (size_t i = 0U; i < strlen(msg_a); i++) {
		uart_poll_out(uart, msg_a[i]);
	}

	ret = device_put(uart);
	if (ret != 0) {
		printf("Failed to put UART device\n");
		return 0;
	}

	ret = pinctrl_update_states(uart_config, uart_alt, ARRAY_SIZE(uart_alt));
	if (ret < 0) {
		printf("Failed to update pinctrl states\n");
		return 0;
	}

	ret = device_get(uart);
	if (ret < 0) {
		printf("Failed to get UART device\n");
		return 0;
	}

	for (size_t i = 0U; i < strlen(msg_b); i++) {
		uart_poll_out(uart, msg_b[i]);
	}

	return 0;
}
