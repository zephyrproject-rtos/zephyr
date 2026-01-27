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
#include <zephyr/pmci/mctp/mctp_i3c_target.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(mctp_i3c_bus_endpoint);

MCTP_I3C_TARGET_DT_DEFINE(mctp_i3c_ctrl, DT_NODELABEL(mctp_i3c));

K_SEM_DEFINE(mctp_rx, 0, 1);

struct mctp *mctp_ctx;

#define BUS_OWNER_ID 20

static void rx_message_handler(struct k_work *work)
{
	ARG_UNUSED(work);

	mctp_message_tx(mctp_ctx, BUS_OWNER_ID, false, 0, "pong", sizeof("pong"));

	k_sem_give(&mctp_rx);
}
K_WORK_DEFINE(rx_message_work, rx_message_handler);

static void rx_message(uint8_t eid, bool tag_owner, uint8_t msg_tag, void *data, void *msg,
		       size_t len)
{
	LOG_INF("received message \"%s\" from endpoint %d, replying with \"pong\"", (char *)msg,
		eid);

	k_work_submit(&rx_message_work);
}

int main(void)
{
	LOG_INF("MCTP Host EID:%d on %s\n", mctp_i3c_ctrl.endpoint_id, CONFIG_BOARD_TARGET);

	mctp_ctx = mctp_init();
	__ASSERT_NO_MSG(mctp_ctx != NULL);
	mctp_register_bus(mctp_ctx, &mctp_i3c_ctrl.binding, mctp_i3c_ctrl.endpoint_id);
	mctp_set_rx_all(mctp_ctx, rx_message, NULL);

	/*
	 * MCTP poll loop, listening for messages.
	 */
	while (true) {
		k_sem_take(&mctp_rx, K_FOREVER);
	}

	return 0;
}
