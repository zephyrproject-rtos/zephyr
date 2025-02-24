/*
 * Copyright (c) 2024 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <unistd.h>
#include <zephyr/types.h>
#include <zephyr/kernel.h>
#include <libmctp.h>
#include <zephyr/pmci/mctp/mctp_uart.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(mctp_host);

#define LOCAL_HELLO_EID 20

#define REMOTE_HELLO_EID 10

K_SEM_DEFINE(mctp_rx, 0, 1);

static void rx_message(uint8_t eid, bool tag_owner, uint8_t msg_tag, void *data, void *msg,
		       size_t len)
{
	LOG_INF("received message %s for endpoint %d, msg_tag %d, len %zu", (char *)msg, eid,
		msg_tag, len);
	k_sem_give(&mctp_rx);
}

MCTP_UART_DT_DEFINE(mctp_host, DEVICE_DT_GET(DT_NODELABEL(arduino_serial)));

int main(void)
{
	int rc;
	struct mctp *mctp_ctx;

	LOG_INF("MCTP Host EID:%d on %s\n", LOCAL_HELLO_EID, CONFIG_BOARD_TARGET);

	mctp_set_alloc_ops(malloc, free, realloc);
	mctp_ctx = mctp_init();
	__ASSERT_NO_MSG(mctp_ctx != NULL);
	mctp_register_bus(mctp_ctx, &mctp_host.binding, LOCAL_HELLO_EID);
	mctp_set_rx_all(mctp_ctx, rx_message, NULL);
	mctp_uart_start_rx(&mctp_host);

	/* MCTP poll loop, send "hello" and get "world" back */
	while (true) {
		rc = mctp_message_tx(mctp_ctx, REMOTE_HELLO_EID, false, 0, "hello",
				     sizeof("hello"));
		if (rc != 0) {
			LOG_WRN("Failed to send message, errno %d\n", rc);
			k_msleep(1000);
		} else {
			k_sem_take(&mctp_rx, K_MSEC(10));
		}
		rc = 0;
	}

	return 0;
}
