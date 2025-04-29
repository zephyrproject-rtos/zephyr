/*
 * Copyright (c) 2024 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <unistd.h>
#include <zephyr/kernel.h>
#include <zephyr/types.h>
#include <zephyr/pmci/mctp/mctp_uart.h>
#include <libmctp.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(mctp_endpoint);

#include <zephyr/drivers/uart.h>

static struct mctp *mctp_ctx;

#define LOCAL_HELLO_EID 10

#define REMOTE_HELLO_EID 20

K_SEM_DEFINE(mctp_rx, 0, 1);

static void rx_message(uint8_t eid, bool tag_owner, uint8_t msg_tag, void *data, void *msg,
		       size_t len)
{
	switch (eid) {
	case REMOTE_HELLO_EID:
		LOG_INF("got mctp message %s for eid %d, replying to 5 with \"world\"", (char *)msg,
			eid);
		mctp_message_tx(mctp_ctx, LOCAL_HELLO_EID, false, 0, "world", sizeof("world"));
		break;
	default:
		LOG_INF("Unknown endpoint %d", eid);
		break;
	}

	k_sem_give(&mctp_rx);
}

MCTP_UART_DT_DEFINE(mctp_endpoint, DEVICE_DT_GET(DT_NODELABEL(arduino_serial)));

#define RX_BUF_SZ 128

int main(void)
{
	LOG_INF("MCTP Endpoint EID:%d on %s\n", LOCAL_HELLO_EID, CONFIG_BOARD_TARGET);

	mctp_set_alloc_ops(malloc, free, realloc);
	mctp_ctx = mctp_init();
	__ASSERT_NO_MSG(mctp_ctx != NULL);
	mctp_register_bus(mctp_ctx, &mctp_endpoint.binding, LOCAL_HELLO_EID);
	mctp_set_rx_all(mctp_ctx, rx_message, NULL);

	/* MCTP poll loop */
	while (true) {
		mctp_uart_start_rx(&mctp_endpoint);
		k_sem_take(&mctp_rx, K_FOREVER);
	}

	LOG_INF("exiting");
	return 0;
}
