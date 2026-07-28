/*
 * Copyright (c) 2026 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * SLIP loopback test for the async UART path. SLIP framing is enabled on a
 * single direction at a time so the escape handling of that direction is
 * exercised in isolation. A symmetric setup would hide errors because the
 * TX-escape and RX-unescape cancel out on the same controller. The DUT uses
 * same-pin internal loopback.
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/ztest.h>
#include <string.h>

#define DUT_NODE DT_NODELABEL(dut)

static const struct device *const uart_dev = DEVICE_DT_GET(DUT_NODE);

static uint8_t tx_buf[32];
static uint8_t rx_buf[64];
static volatile size_t rx_count;

static K_SEM_DEFINE(tx_done, 0, 1);
static K_SEM_DEFINE(rx_disabled, 0, 1);

static void uart_cb(const struct device *dev, struct uart_event *evt, void *user_data)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(user_data);

	switch (evt->type) {
	case UART_TX_DONE:
		k_sem_give(&tx_done);
		break;
	case UART_RX_RDY:
		if (evt->data.rx.len <= sizeof(rx_buf) - rx_count) {
			memcpy(&rx_buf[rx_count], &evt->data.rx.buf[evt->data.rx.offset],
			       evt->data.rx.len);
			rx_count += evt->data.rx.len;
		}
		break;
	case UART_RX_DISABLED:
		k_sem_give(&rx_disabled);
		break;
	default:
		break;
	}
}

static void run_loopback(const uint8_t *tx, size_t tx_len)
{
	int ret;

	zassert_true(tx_len <= sizeof(tx_buf), "tx pattern too large");
	memcpy(tx_buf, tx, tx_len);
	rx_count = 0;

	ret = uart_callback_set(uart_dev, uart_cb, NULL);
	zassert_equal(ret, 0, "uart_callback_set failed: %d", ret);

	ret = uart_rx_enable(uart_dev, rx_buf, sizeof(rx_buf), 50 * USEC_PER_MSEC);
	zassert_equal(ret, 0, "uart_rx_enable failed: %d", ret);

	ret = uart_tx(uart_dev, tx_buf, tx_len, 100 * USEC_PER_MSEC);
	zassert_equal(ret, 0, "uart_tx failed: %d", ret);

	zassert_equal(k_sem_take(&tx_done, K_MSEC(500)), 0, "TX_DONE timeout");
	k_sem_take(&rx_disabled, K_MSEC(500));
}

#if DT_PROP(DUT_NODE, uhci_slip_tx)
ZTEST(uart_async_slip, test_tx_slip_encoding)
{
	static const uint8_t src[] = {
		0x00, 0xC0, 0xDB, 0x42, 0xDC, 0xDD, 0xFF, 0xC0, 0x11, 0xDB, 0x13, 0x55, 0xAA,
	};
	static const uint8_t expected[] = {
		0x00, 0xDB, 0xDC, 0xDB, 0xDD, 0x42, 0xDC, 0xDD, 0xFF,
		0xDB, 0xDC, 0x11, 0xDB, 0xDD, 0x13, 0x55, 0xAA,
	};

	run_loopback(src, sizeof(src));

	zassert_equal(rx_count, sizeof(expected), "RX length %u != expected %u",
		      (unsigned int)rx_count, (unsigned int)sizeof(expected));
	zassert_mem_equal(rx_buf, expected, sizeof(expected), "RX not SLIP-encoded");
}
#endif

#if DT_PROP(DUT_NODE, uhci_slip_rx)
ZTEST(uart_async_slip, test_rx_slip_decoding)
{
	static const uint8_t src[] = {
		0x00, 0xDB, 0xDC, 0xDB, 0xDD, 0x42, 0xDC, 0xDD, 0xFF,
		0xDB, 0xDC, 0x11, 0xDB, 0xDD, 0x13, 0x55, 0xAA,
	};
	static const uint8_t expected[] = {
		0x00, 0xC0, 0xDB, 0x42, 0xDC, 0xDD, 0xFF, 0xC0, 0x11, 0xDB, 0x13, 0x55, 0xAA,
	};

	run_loopback(src, sizeof(src));

	zassert_equal(rx_count, sizeof(expected), "RX length %u != expected %u",
		      (unsigned int)rx_count, (unsigned int)sizeof(expected));
	zassert_mem_equal(rx_buf, expected, sizeof(expected), "RX not SLIP-decoded");
}
#endif

ZTEST_SUITE(uart_async_slip, NULL, NULL, NULL, NULL, NULL);
