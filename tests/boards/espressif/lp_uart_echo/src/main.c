/*
 * Copyright (c) 2026 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/ztest.h>
#include <hal/gpio_ll.h>
#include <hal/rtc_io_hal.h>
#include <hal/rtc_io_periph.h>

#define LP_TX_PAD DT_PROP(DT_NODELABEL(lp_uart), tx_pin)
#define LP_RX_PAD DT_PROP(DT_NODELABEL(lp_uart), rx_pin)

#define RX_TIMEOUT_MS 200
#define RX_QUIET_MS   100
#define BUF_SIZE      32

static const struct device *const uart = DEVICE_DT_GET(DT_NODELABEL(uart1));

static void drain_rx(void)
{
	int64_t deadline = k_uptime_get() + RX_QUIET_MS;
	unsigned char c;

	while (k_uptime_get() < deadline) {
		if (uart_poll_in(uart, &c) == 0) {
			deadline = k_uptime_get() + RX_QUIET_MS;
		} else {
			k_msleep(1);
		}
	}
}

static size_t read_echo(unsigned char *buf, size_t len)
{
	int64_t deadline = k_uptime_get() + RX_TIMEOUT_MS;
	size_t n = 0;

	while ((n < len) && (k_uptime_get() < deadline)) {
		if (uart_poll_in(uart, &buf[n]) == 0) {
			n++;
		} else {
			k_msleep(1);
		}
	}

	return n;
}

ZTEST(lp_uart_echo, test_echo)
{
	static const char *const msgs[] = {
		"ping",
		"Zephyr LP core echo",
		"0123456789abcdef",
	};
	unsigned char rx[BUF_SIZE];

	drain_rx();

	ARRAY_FOR_EACH(msgs, i) {
		size_t len = strlen(msgs[i]);
		size_t n;

		for (size_t j = 0; j < len; j++) {
			uart_poll_out(uart, msgs[i][j]);
		}

		n = read_echo(rx, len);
		zassert_equal(n, len, "'%s': got %zu of %zu bytes", msgs[i], n, len);
		zassert_mem_equal(rx, msgs[i], len, "'%s': echo mismatch", msgs[i]);
	}
}

static void *lp_uart_echo_setup(void)
{
	zassert_true(device_is_ready(uart), "UART1 not ready");

	/* UART1 TX drives the LP UART RX pad. UART1 RX samples the LP UART TX
	 * pad, which stays under LP UART control.
	 */
	rtcio_hal_function_select(rtc_io_num_map[LP_RX_PAD], RTCIO_LL_FUNC_DIGITAL);
	gpio_ll_input_enable(&GPIO, LP_RX_PAD);

	rtcio_hal_function_select(rtc_io_num_map[LP_TX_PAD], RTCIO_LL_FUNC_RTC);
	rtcio_hal_set_direction(rtc_io_num_map[LP_TX_PAD], RTC_GPIO_MODE_INPUT_OUTPUT);

	return NULL;
}

ZTEST_SUITE(lp_uart_echo, NULL, lp_uart_echo_setup, NULL, NULL, NULL);
