/*
 * Copyright (c) 2025 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/types.h>
#include <zephyr/kernel.h>
#include <libmctp.h>
#include <zephyr/pmci/mctp/mctp_i2c_gpio_controller.h>
#include <zephyr/pmci/mctp/mctp_i2c_gpio_target.h>

#include <zephyr/ztest.h>

#define LOCAL_EID 20

MCTP_I2C_GPIO_CONTROLLER_DT_DEFINE(mctp_i2c_ctrl, DT_NODELABEL(mctp_i2c));
MCTP_I2C_GPIO_TARGET_DT_DEFINE(mctp_i2c_target, DT_NODELABEL(mctp_i2c_target));

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

	TC_PRINT("MCTP Endpoint EID:%d on %s\n", mctp_i2c_target.endpoint_id, CONFIG_BOARD_TARGET);
	mctp_ctx = mctp_init();

	zassert_not_null(mctp_ctx, "Failed to initialize MCTP target context");
	mctp_register_bus(mctp_ctx, &mctp_i2c_target.binding, mctp_i2c_target.endpoint_id);
	mctp_set_rx_all(mctp_ctx, rx_message_target, NULL);

	reply_handler.mctp_ctx = mctp_ctx;
	k_work_init(&reply_handler.work, target_reply);

	return mctp_ctx;
}

static void rx_message(uint8_t eid, bool tag_owner, uint8_t msg_tag, void *data, void *msg,
		       size_t len)
{
	TC_PRINT("Received message \"%s\" from endpoint %d to %d, msg_tag %d, len %zu\n",
		(char *)msg, eid, LOCAL_EID, msg_tag, len);

	ping_pong_done = true;
	k_sem_give(&mctp_rx);
}

ZTEST(mctp_i2c_gpio_test_suite, test_mctp_i2c_gpio_ping_pong)
{
	int rc;
	struct mctp *mctp_ctx, *mctp_ctx_target;

	mctp_ctx_target = init_target();

	TC_PRINT("MCTP Host EID:%d on %s\n", LOCAL_EID, CONFIG_BOARD_TARGET);
	mctp_ctx = mctp_init();

	zassert_not_null(mctp_ctx, "Failed to initialize MCTP context");
	mctp_register_bus(mctp_ctx, &mctp_i2c_ctrl.binding, LOCAL_EID);
	mctp_set_rx_all(mctp_ctx, rx_message, NULL);

	TC_PRINT("Sending message \"ping\" to endpoint %u\n",
		mctp_i2c_ctrl.endpoint_ids[0]);

	rc = mctp_message_tx(mctp_ctx, mctp_i2c_ctrl.endpoint_ids[0], false, 0, "ping",
			    sizeof("ping"));
	zassert_ok(rc, "Failed to send message");

	/* Wait ping-pong to complete */
	k_sem_take(&mctp_rx, K_SECONDS(5));

	zassert_true(ping_pong_done, "Ping-pong message exchange failed");

	mctp_destroy(mctp_ctx);
	mctp_destroy(mctp_ctx_target);
}

ZTEST_SUITE(mctp_i2c_gpio_test_suite, NULL, NULL, NULL, NULL, NULL);
