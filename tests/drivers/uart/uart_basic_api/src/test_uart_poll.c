/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "test_uart.h"

static const char *poll_data = "This is a POLL test.\r\n";

static int test_poll_in(void)
{
	unsigned char recv_char;
	const struct device *const uart_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_console));

	if (!device_is_ready(uart_dev)) {
		TC_PRINT("UART device not ready\n");
		return TC_FAIL;
	}

	TC_PRINT("Please send characters to serial console\n");

	/* Verify uart_poll_in() */
	while (1) {
		while (uart_poll_in(uart_dev, &recv_char) < 0) {
			/* Allow other thread/workqueue to work. */
			k_yield();
		}

		TC_PRINT("%c", recv_char);

		if ((recv_char == '\n') || (recv_char == '\r')) {
			break;
		}
	}

	return TC_PASS;
}

static int test_poll_out(void)
{
	int i;
	const struct device *const uart_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_console));

	if (!device_is_ready(uart_dev)) {
		TC_PRINT("UART device not ready\n");
		return TC_FAIL;
	}

	/* Verify uart_poll_out() */
	for (i = 0; i < strlen(poll_data); i++) {
		uart_poll_out(uart_dev, poll_data[i]);
	}

	return TC_PASS;
}

#if CONFIG_SHELL
void test_uart_poll_out(void)
#else
ZTEST(uart_basic_api, test_uart_poll_out)
#endif
{
	zassert_true(test_poll_out() == TC_PASS);
}

#if CONFIG_SHELL
void test_uart_poll_in(void)
#else
ZTEST(uart_basic_api, test_uart_poll_in)
#endif
{
	zassert_true(test_poll_in() == TC_PASS);
}
