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
#include <zephyr/pmci/mctp/mctp_i2c_gpio_target.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(mctp_i2c_gpio_bus_endpoint);

MCTP_I2C_GPIO_TARGET_DT_DEFINE(mctp_i2c_ctrl, DT_NODELABEL(mctp_i2c));

static void rx_message(uint8_t eid, bool tag_owner, uint8_t msg_tag, void *data, void *msg,
		       size_t len)
{
	LOG_INF("received message \"%s\" from endpoint %d, msg_tag %d, len %zu", (char *)msg, eid,
		msg_tag, len);
}

#define BUS_OWNER_ID 20

int main(void)
{
	int rc;
	struct mctp *mctp_ctx;

	LOG_INF("MCTP Host EID:%d on %s\n", mctp_i2c_ctrl.endpoint_id, CONFIG_BOARD_TARGET);

	mctp_ctx = mctp_init();
	__ASSERT_NO_MSG(mctp_ctx != NULL);
	mctp_register_bus(mctp_ctx, &mctp_i2c_ctrl.binding, mctp_i2c_ctrl.endpoint_id);
	mctp_set_rx_all(mctp_ctx, rx_message, NULL);

	/*
	 * 1. MCTP poll loop, send "ping" to each endpoint and get "pong" back
	 * 2. Then send a broadcast "hello" to each endpoint and get a "from endoint %d" back
	 */
	while (true) {
		rc = mctp_message_tx(mctp_ctx, BUS_OWNER_ID, false,
				0, "ping", sizeof("ping"));
		if (rc != 0) {
			LOG_WRN("Failed to send message \"ping\", errno %d\n", rc);
		}
		k_msleep(500);
	}

	return 0;
}
