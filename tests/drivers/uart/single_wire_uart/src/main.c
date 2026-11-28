/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * This test validates a physical single-wire connection between two UART
 * instances. Each UART uses its TX pin for both transmission and reception;
 * the board overlay assigns the two pins to the test aliases and the test
 * fixture connects them externally.
 *
 * The test methodology is:
 *
 * 1. Resolve both UART devices from the single-line-uart1 and
 *    single-line-uart2 devicetree aliases and verify that they are ready.
 * 2. Transmit the byte 'a' through the first UART, wait for the character
 *    to complete on the wire, and receive it through the second UART.
 * 3. Transmit the byte 'b' through the second UART, wait for the character
 *    to complete, and receive it through the first UART.
 * 4. Check both received values and record each successful transfer.
 *
 * The result structure is placed in .noinit so the external pytest harness
 * can attach with a debugger after the test runs and inspect the result from
 * target RAM without reflashing the device. The state and counters provide
 * evidence of the completed exchange; last_error identifies the first
 * failed operation when the test does not pass.
 */

#include <stdint.h>

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/kernel.h>
#include <zephyr/ztest.h>

#define SINGLE_WIRE_UART1_NODE DT_ALIAS(single_line_uart1)
#define SINGLE_WIRE_UART2_NODE DT_ALIAS(single_line_uart2)

#define TEST_RUNNING 1
#define TEST_PASS 2
#define TEST_FAIL 3

struct single_wire_result {
	volatile uint32_t state;
	volatile uint32_t uart1_tx_count;
	volatile uint32_t uart2_tx_count;
	volatile uint32_t uart1_rx_count;
	volatile uint32_t uart2_rx_count;
	volatile uint32_t last_error;
};

__attribute__((section(".noinit")))
volatile struct single_wire_result single_wire_test_result;

static const struct device *const uart1 = DEVICE_DT_GET(SINGLE_WIRE_UART1_NODE);
static const struct device *const uart2 = DEVICE_DT_GET(SINGLE_WIRE_UART2_NODE);

static void test_fail(uint32_t error)
{
	single_wire_test_result.last_error = error;
	single_wire_test_result.state = TEST_FAIL;
}

ZTEST(single_wire, test_uart_exchange)
{
	unsigned char received;

	single_wire_test_result.state = TEST_RUNNING;
	single_wire_test_result.last_error = 0;

	if (!device_is_ready(uart1) || !device_is_ready(uart2)) {
		test_fail(1);
		zassert_true(false, "UART device is not ready");
	}

	uart_poll_out(uart1, 'a');
	single_wire_test_result.uart1_tx_count++;
	k_sleep(K_MSEC(50));
	if (uart_poll_in(uart2, &received) != 0) {
		test_fail(2);
		zassert_true(false, "UART2 did not receive a byte");
	}
	single_wire_test_result.uart2_rx_count++;
	if (received != 'a') {
		test_fail(3);
		zassert_true(false, "UART2 received an unexpected byte");
	}

	uart_poll_out(uart2, 'b');
	single_wire_test_result.uart2_tx_count++;
	k_sleep(K_MSEC(50));
	if (uart_poll_in(uart1, &received) != 0) {
		test_fail(4);
		zassert_true(false, "UART1 did not receive a byte");
	}
	single_wire_test_result.uart1_rx_count++;
	if (received != 'b') {
		test_fail(5);
		zassert_true(false, "UART1 received an unexpected byte");
	}

	single_wire_test_result.state = TEST_PASS;
}

ZTEST_SUITE(single_wire, NULL, NULL, NULL, NULL, NULL);
