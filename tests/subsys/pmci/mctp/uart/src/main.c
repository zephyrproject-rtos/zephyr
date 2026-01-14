/*
 * Copyright (c) 2025 Intel Corporation
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

#include <zephyr/ztest.h>

#define HOST_EID 20
#define ENDPOINT_EID 11

MCTP_UART_DT_DEFINE(mctp_uart_host, DEVICE_DT_GET(DT_NODELABEL(host_serial)));
MCTP_UART_DT_DEFINE(mctp_uart_endpoint, DEVICE_DT_GET(DT_NODELABEL(endpoint_serial)));

K_SEM_DEFINE(mctp_rx, 0, 1);

struct reply_data {
	struct k_work work;
	struct mctp *mctp_ctx;
	uint8_t eid;
};

bool ping_pong_done;

static struct reply_data reply_handler;

static void target_reply(struct k_work *item)
{
	struct reply_data *reply = CONTAINER_OF(item, struct reply_data, work);
	int rc;

	/* As this test may share a single UART device between host and endpoint,
	 * we need to stop RX on endpoint before restarting RX on host.
	 */
	rc = mctp_uart_stop_rx(&mctp_uart_endpoint);
	zassert_ok(rc, "Failed to stop endpoint RX");
	mctp_uart_start_rx(&mctp_uart_host);

	TC_PRINT("Target replying \"pong\" to endpoint %d\n", reply->eid);
	rc = mctp_message_tx(reply->mctp_ctx, reply->eid, false, 0, "pong", sizeof("pong"));
	zassert_ok(rc, "Failed to send reply message");
}

static void rx_message_target(uint8_t eid, bool tag_owner, uint8_t msg_tag, void *data,
			      void *msg, size_t len)
{
	ARG_UNUSED(tag_owner);
	ARG_UNUSED(msg_tag);
	ARG_UNUSED(data);
	ARG_UNUSED(len);

	TC_PRINT("Target received message \"%s\" from endpoint %d, queuing reply\n",
		(char *)msg, eid);

	reply_handler.eid = eid;
	k_work_submit(&reply_handler.work);
}

static struct mctp *init_target(void)
{
	struct mctp *mctp_ctx;

	TC_PRINT("MCTP Endpoint EID:%d on %s\n", ENDPOINT_EID, CONFIG_BOARD_TARGET);
	mctp_ctx = mctp_init();

	zassert_not_null(mctp_ctx, "Failed to initialize MCTP target context");
	mctp_register_bus(mctp_ctx, &mctp_uart_endpoint.binding, ENDPOINT_EID);
	mctp_set_rx_all(mctp_ctx, rx_message_target, NULL);
	mctp_uart_start_rx(&mctp_uart_endpoint);

	reply_handler.mctp_ctx = mctp_ctx;
	k_work_init(&reply_handler.work, target_reply);

	return mctp_ctx;
}

static void rx_message(uint8_t eid, bool tag_owner, uint8_t msg_tag, void *data, void *msg,
		       size_t len)
{
	TC_PRINT("Received message \"%s\" from endpoint %d to %d, msg_tag %d, len %zu\n",
		(char *)msg, eid, HOST_EID, msg_tag, len);

	ping_pong_done = true;
	k_sem_give(&mctp_rx);
}

ZTEST(mctp_uart_test_suite, test_mctp_uart_ping_pong)
{
	int rc;
	struct mctp *mctp_ctx, *mctp_ctx_target;

	mctp_ctx_target = init_target();

	TC_PRINT("MCTP Host EID:%d on %s\n", HOST_EID, CONFIG_BOARD_TARGET);
	mctp_ctx = mctp_init();

	zassert_not_null(mctp_ctx, "Failed to initialize MCTP context");
	mctp_register_bus(mctp_ctx, &mctp_uart_host.binding, HOST_EID);
	mctp_set_rx_all(mctp_ctx, rx_message, NULL);

	TC_PRINT("Sending message \"ping\" to endpoint %u\n", ENDPOINT_EID);

	rc = mctp_message_tx(mctp_ctx, ENDPOINT_EID, false, 0, "ping", sizeof("ping"));
	zassert_ok(rc, "Failed to send message");

	/* Wait ping-pong to complete */
	k_sem_take(&mctp_rx, K_SECONDS(5));

	zassert_true(ping_pong_done, "Ping-pong message exchange failed");

	mctp_destroy(mctp_ctx);
	mctp_destroy(mctp_ctx_target);
}

ZTEST_SUITE(mctp_uart_test_suite, NULL, NULL, NULL, NULL, NULL);
