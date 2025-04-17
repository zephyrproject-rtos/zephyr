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
#include <zephyr/pmci/mctp/mctp_i2c_gpio_controller.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(mctp_i2c_gpio_bus_owner);

#define LOCAL_EID 20

MCTP_I2C_GPIO_CONTROLLER_DT_DEFINE(mctp_i2c_ctrl, DT_NODELABEL(mctp_i2c));

static void rx_message(uint8_t eid, bool tag_owner, uint8_t msg_tag, void *data, void *msg,
		       size_t len)
{
	LOG_INF("received message %s from endpoint %d to %d, msg_tag %d, len %zu", (char *)msg, eid,
		LOCAL_EID, msg_tag, len);
}

int main(void)
{
	int rc;
	struct mctp *mctp_ctx;

	LOG_INF("MCTP Host EID:%d on %s\n", LOCAL_EID, CONFIG_BOARD_TARGET);

	mctp_ctx = mctp_init();
	__ASSERT_NO_MSG(mctp_ctx != NULL);
	mctp_register_bus(mctp_ctx, &mctp_i2c_ctrl.binding, LOCAL_EID);
	mctp_set_rx_all(mctp_ctx, rx_message, NULL);

	while (true) {
		for (int i = 0; i < mctp_i2c_ctrl.num_endpoints; i++) {

			LOG_INF("sending message \"ping\" to endpoint %d",
				mctp_i2c_ctrl.endpoint_ids[i]);
			rc = mctp_message_tx(mctp_ctx, mctp_i2c_ctrl.endpoint_ids[i], false,
					0, "ping", sizeof("ping"));
			if (rc != 0) {
				LOG_WRN("Failed to send message \"ping\" to endpoint %d,"
					" errno %d\n", mctp_i2c_ctrl.endpoint_ids[i], rc);
			}

			k_msleep(500);
		}
	}

	return 0;
}
