/*
 * Copyright (c) 2025 Gridpoint Technologies
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief Minimal test suite for native_sim PTY symlink functionality
 *
 * This includes both a minimal environment test and an interactive UART
 * communication test that can be enabled via test suite filtering.
 * The real PTY symlink testing is performed by test_e2e.sh.
 *
 * Note: The actual validation happens in test_e2e.sh, not here.
 */

#include <zephyr/kernel.h>
#include <zephyr/ztest.h>
#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>

/* Minimal test suite */
ZTEST_SUITE(pty_test_env, NULL, NULL, NULL, NULL, NULL);

/**
 * @brief Test that the test environment is set up correctly
 *
 * This is a minimal test to ensure the test framework is working.
 * Real PTY symlink testing is done by test_e2e.sh.
 */
ZTEST(pty_test_env, test_environment_ready)
{
	printk("PTY symlink test environment ready\n");
	printk("Real testing is performed by test_e2e.sh\n");
	zassert_true(true, "Test environment is ready");
}

/* Interactive UART communication test suite */
ZTEST_SUITE(pty_uart_comm, NULL, NULL, NULL, NULL, NULL);

/**
 * @brief Interactive UART communication test
 *
 * This test waits for "hello\r" on UART and responds with "world\n".
 * It's designed to be used by test_e2e.sh for bidirectional communication testing.
 * This test will keep the application alive until killed externally.
 *
 * Note: This test is disabled by default to prevent blocking during normal test runs.
 * Enable CONFIG_PTY_INTERACTIVE_TEST=y to include this test.
 */
ZTEST(pty_uart_comm, test_uart_echo)
{
#ifndef CONFIG_PTY_INTERACTIVE_TEST
	ztest_test_skip();
	return;
#else
	const struct device *uart_dev;
	uint8_t recv_buf[32];
	int recv_len = 0;
	uint8_t c;

	printk("Starting interactive UART communication test\n");
	printk("Waiting for 'hello\\r' on UART...\n");

	/* Get the console UART device */
	uart_dev = DEVICE_DT_GET_OR_NULL(DT_CHOSEN(zephyr_console));
	if (uart_dev == NULL) {
		/* Try alternative UART devices */
		uart_dev = DEVICE_DT_GET_OR_NULL(DT_NODELABEL(uart0));
	}

	zassert_not_null(uart_dev, "UART device should be available");
	zassert_true(device_is_ready(uart_dev), "UART device should be ready");

	/* Main communication loop */
	while (1) {
		/* Poll for incoming characters */
		while (uart_poll_in(uart_dev, &c) == 0) {
			if (recv_len < sizeof(recv_buf) - 1) {
				recv_buf[recv_len++] = c;
				recv_buf[recv_len] = '\0';

				/* Check if we received "hello\r" */
				if (recv_len >= 6 &&
				    memcmp(recv_buf + recv_len - 6, "hello\r", 6) == 0) {
					printk("Received 'hello\\r', responding with 'world\\n'\n");

					/* Send response */
					const char *response = "world\n";
					for (int i = 0; response[i] != '\0'; i++) {
						uart_poll_out(uart_dev, response[i]);
					}

					/* Reset buffer for next message */
					recv_len = 0;
					memset(recv_buf, 0, sizeof(recv_buf));
				}
			} else {
				/* Buffer full, reset */
				recv_len = 0;
				memset(recv_buf, 0, sizeof(recv_buf));
			}
		}

		/* Small delay to avoid busy polling */
		k_msleep(10);
	}
#endif /* CONFIG_PTY_INTERACTIVE_TEST */
}
