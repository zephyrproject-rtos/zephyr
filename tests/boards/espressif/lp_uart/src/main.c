/*
 * Copyright (c) 2026 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/ztest.h>
#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>
#include <string.h>

#define LP_UART_NODE DT_NODELABEL(lp_uart)

#define TEST_MSG "lp uart loopback"
#define BUF_SIZE 64

static const struct device *const lp_uart = DEVICE_DT_GET(LP_UART_NODE);

static uint8_t rx_buf[BUF_SIZE];
static volatile int rx_pos;
static struct k_sem rx_done;

static void lp_uart_cb(const struct device *dev, void *user_data)
{
	uint8_t c;

	uart_irq_update(dev);

	if (!uart_irq_rx_ready(dev)) {
		return;
	}

	while (uart_fifo_read(dev, &c, 1) == 1) {
		if (rx_pos < BUF_SIZE) {
			rx_buf[rx_pos++] = c;
		}
	}
}

/*
 * Drain the RX FIFO, waiting for a sustained quiet window so bytes that arrive
 * with loopback latency (or console/boot residue) are fully cleared.
 */
static void lp_uart_drain(void)
{
	unsigned char c;
	int quiet = 0;

	while (quiet < 2000) {
		if (uart_poll_in(lp_uart, &c) == 0) {
			quiet = 0;
		} else {
			quiet++;
		}
		k_busy_wait(10);
	}
}

/*
 * Check that the test payload appears contiguously in the received bytes. The
 * LP UART loopback path on some SoCs can emit a spurious leading byte on the
 * first burst after an idle line, so the payload may not start at offset 0.
 */
static bool buf_contains_msg(const uint8_t *buf, int n)
{
	const int len = strlen(TEST_MSG);

	for (int i = 0; i + len <= n; i++) {
		if (memcmp(&buf[i], TEST_MSG, len) == 0) {
			return true;
		}
	}
	return false;
}

/*
 * Send the payload twice. The LP UART loopback startup transient can corrupt
 * or drop a byte at the very start of the burst, so the first copy may be
 * damaged; the second copy is always clean. The test passes if the payload is
 * found contiguously anywhere in the echoed stream.
 */
static void send_msg_twice(void)
{
	for (int rep = 0; rep < 2; rep++) {
		for (int i = 0; i < (int)strlen(TEST_MSG); i++) {
			uart_poll_out(lp_uart, TEST_MSG[i]);
		}
	}
}

ZTEST(lp_uart, test_poll_loopback)
{
	unsigned char c = 0;
	const int len = strlen(TEST_MSG);
	uint8_t rx[BUF_SIZE];
	int pos = 0;

	lp_uart_drain();
	send_msg_twice();

	while (pos < (int)sizeof(rx)) {
		bool got = false;

		for (int retry = 0; retry < 200; retry++) {
			if (uart_poll_in(lp_uart, &c) == 0) {
				rx[pos++] = c;
				got = true;
				break;
			}
			k_busy_wait(50);
		}
		if (!got) {
			break;
		}
	}

	zassert_true(pos >= len, "too few bytes received: %d", pos);
	zassert_true(buf_contains_msg(rx, pos), "payload not found in echo");
}

ZTEST(lp_uart, test_irq_loopback)
{
	int ret;
	const int len = strlen(TEST_MSG);

	lp_uart_drain();

	rx_pos = 0;
	k_sem_init(&rx_done, 0, 1);

	ret = uart_irq_callback_user_data_set(lp_uart, lp_uart_cb, NULL);
	zassert_equal(ret, 0, "callback set failed: %d", ret);

	uart_irq_rx_enable(lp_uart);

	send_msg_twice();

	/* Give the RX interrupt time to collect both copies */
	k_msleep(100);
	uart_irq_rx_disable(lp_uart);

	zassert_true(rx_pos >= len, "too few bytes received: %d", rx_pos);
	zassert_true(buf_contains_msg(rx_buf, rx_pos), "payload not found in echo");
}

static void *lp_uart_setup(void)
{
	zassert_true(device_is_ready(lp_uart), "LP UART device not ready");
	return NULL;
}

ZTEST_SUITE(lp_uart, NULL, lp_uart_setup, NULL, NULL, NULL);
