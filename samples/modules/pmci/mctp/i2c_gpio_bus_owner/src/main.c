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
#include <zephyr/pmci/mctp/mctp_i2c.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(mctp_i2c_gpio_bus_owner);

#define LOCAL_EID 20

MCTP_I2C_CONTROLLER_GPIO_DT_DEFINE(mctp_i2c_ctrl, DT_NODELABEL(mctp_i2c));

K_SEM_DEFINE(mctp_rx, 0, 1);

static void rx_message(uint8_t eid, bool tag_owner, uint8_t msg_tag, void *data, void *msg,
		       size_t len)
{
	LOG_INF("received message %s for endpoint %d, msg_tag %d, len %zu", (char *)msg, eid,
		msg_tag, len);
	k_sem_give(&mctp_rx);
}

int main(void)
{
	int rc;
	struct mctp *mctp_ctx;

	LOG_INF("MCTP Host EID:%d on %s\n", LOCAL_EID, CONFIG_BOARD_TARGET);

	mctp_set_alloc_ops(malloc, free, realloc);
	mctp_ctx = mctp_init();
	__ASSERT_NO_MSG(mctp_ctx != NULL);
	mctp_register_bus(mctp_ctx, &mctp_i2c_ctrl.binding, LOCAL_EID);
	mctp_set_rx_all(mctp_ctx, rx_message, NULL);

	/*
	 * 1. MCTP poll loop, send "ping" to each endpoint and get "pong" back
	 * 2. Then send a broadcast "hello" to each endpoint and get a "from endoint %d" back
	 */
	while (true) {
		for (int i = 0; i < mctp_i2c_ctrl.num_endpoints; i++) {

			rc = mctp_message_tx(mctp_ctx, mctp_i2c_ctrl.endpoint_ids[i], false,
					0, "ping", sizeof("ping"));
			if (rc != 0) {
				LOG_WRN("Failed to send message \"ping\", errno %d\n", rc);
				k_msleep(1000);
			} else {
				k_sem_take(&mctp_rx, K_MSEC(10));
			}
			rc = 0;
		}
	}

	return 0;
}
