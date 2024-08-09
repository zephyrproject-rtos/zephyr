/*
 * Copyright (c) 2024 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <assert.h>
#include <unistd.h>
#include <zephyr/kernel.h>
#include <zephyr/types.h>
#include <libmctp.h>
#include <libmctp-serial.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(mctp_endpoint);

#include <zephyr/drivers/uart.h>

static struct mctp_binding_serial *serial;
static struct mctp *mctp_ctx;

#define LOCAL_HELLO_EID 10

#define REMOTE_HELLO_EID 20

static int mctp_uart_poll_out(void *data, void *buf, size_t len)
{
	const struct device *uart = data;

	for (size_t i = 0; i < len; i++) {
		uart_poll_out(uart, ((uint8_t *)buf)[i]);
	}

	return (int)len;
}

static void rx_message(uint8_t eid, bool tag_owner,
		       uint8_t msg_tag, void *data, void *msg,
		       size_t len)
{
	switch (eid) {
	case REMOTE_HELLO_EID:
		LOG_INF("got mctp message %s for eid %d, replying to 5 with \"world\"",
			(char *)msg, eid);
		mctp_message_tx(mctp_ctx, LOCAL_HELLO_EID, false, 0, "world", sizeof("world"));
		break;
	default:
		LOG_INF("Unknown endpoint %d", eid);
		break;
	}
}

const struct device *uart = DEVICE_DT_GET(DT_NODELABEL(arduino_serial));

#define RX_BUF_SZ 128

int main(void)
{
	LOG_INF("MCTP Endpoint %d on %s\n", LOCAL_HELLO_EID, CONFIG_BOARD_TARGET);
	int rc = 0;
	uint8_t rx_buf[RX_BUF_SZ];

	mctp_ctx = mctp_init();
	assert(mctp_ctx != NULL);

	serial = mctp_serial_init();
	assert(serial);

	mctp_serial_set_tx_fn(serial, mctp_uart_poll_out, (void *)uart);

	mctp_register_bus(mctp_ctx, mctp_binding_serial_core(serial), LOCAL_HELLO_EID);

	mctp_set_rx_all(mctp_ctx, rx_message, NULL);

	/* MCTP poll loop */
	while (true) {
		int i;

		for (i = 0; i < RX_BUF_SZ; i++) {
			rc = uart_poll_in(uart, &rx_buf[i]);
			if (rc != 0) {
				break;
			}
		}

		mctp_serial_rx(serial, rx_buf, i);
		k_yield();
	}

	LOG_INF("exiting");
	return 0;
}
