/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdlib.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/ztest.h>
#include <zephyr/ipc/ipc_service.h>

#include <test_commands.h>
#include "data_queue.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(ipc_sessions, LOG_LEVEL_INF);

enum test_ipc_events {
	TEST_IPC_EVENT_BOUNDED,
	TEST_IPC_EVENT_UNBOUNDED,
	TEST_IPC_EVENT_ERROR
};

struct test_ipc_event_state {
	enum test_ipc_events ev;
	struct ipc_ep *ep;
};

static const struct device *ipc0_instance = DEVICE_DT_GET(DT_NODELABEL(ipc0));
static volatile bool ipc0_bounded;
K_MSGQ_DEFINE(ipc_events, sizeof(struct test_ipc_event_state), 16, 4);

static uint32_t data_queue_memory[ROUND_UP(CONFIG_IPC_TEST_MSG_HEAP_SIZE, sizeof(uint32_t))];
static struct data_queue ipc_data_queue;

struct test_cmd_xdata {
	struct ipc_test_cmd base;
	uint8_t data[CONFIG_IPC_TEST_BLOCK_SIZE];
};

static void (*ep_received_override_cb)(const void *data, size_t len, void *priv);

static void ep_bound(void *priv)
{
	int ret;
	struct test_ipc_event_state ev = {
		.ev = TEST_IPC_EVENT_BOUNDED,
		.ep = priv
	};

	ipc0_bounded = true;
	ret = k_msgq_put(&ipc_events, &ev, K_NO_WAIT);
	if (ret) {
		LOG_ERR("Cannot put event in queue: %d", ret);
	}
}

static void ep_unbound(void *priv)
{
	int ret;
	struct test_ipc_event_state ev = {
		.ev = TEST_IPC_EVENT_UNBOUNDED,
		.ep = priv
	};

	ipc0_bounded = false;
	ret = k_msgq_put(&ipc_events, &ev, K_NO_WAIT);
	if (ret) {
		LOG_ERR("Cannot put event in queue: %d", ret);
	}
}

static void ep_recv(const void *data, size_t len, void *priv)
{
	int ret;

	if (ep_received_override_cb) {
		ep_received_override_cb(data, len, priv);
	} else {
		ret = data_queue_put(&ipc_data_queue, data, len, K_NO_WAIT);
		__ASSERT(ret >= 0, "Cannot put data into queue: %d", ret);
		(void)ret;
	}
}

static void ep_error(const char *message, void *priv)
{
	int ret;
	struct test_ipc_event_state ev = {
		.ev = TEST_IPC_EVENT_ERROR,
		.ep = priv
	};

	ret = k_msgq_put(&ipc_events, &ev, K_NO_WAIT);
	if (ret) {
		LOG_ERR("Cannot put event in queue: %d", ret);
	}
}

static struct ipc_ept_cfg ep_cfg = {
	.cb = {
		.bound = ep_bound,
		.unbound = ep_unbound,
		.received = ep_recv,
		.error = ep_error
	},
};

static struct ipc_ept ep;



/**
 * @brief Estabilish connection before any test run
 */
void *test_suite_setup(void)
{
	int ret;
	struct test_ipc_event_state ev;

	data_queue_init(&ipc_data_queue, data_queue_memory, sizeof(data_queue_memory));

	ret = ipc_service_open_instance(ipc0_instance);
	zassert_true((ret >= 0) || ret == -EALREADY, "ipc_service_open_instance() failure: %d",
		     ret);

	/* Store the pointer to the endpoint */
	ep_cfg.priv = &ep;
	ret = ipc_service_register_endpoint(ipc0_instance, &ep, &ep_cfg);
	zassert_true((ret >= 0), "ipc_service_register_endpoint() failure: %d", ret);

	do {
		ret = k_msgq_get(&ipc_events, &ev, K_MSEC(1000));
		zassert_ok(ret, "Cannot bound to the remote interface");
	} while (!ipc0_bounded);

	return NULL;
}

/**
 * @brief Prepare the test structures
 */
void test_suite_before(void *fixture)
{
	ep_received_override_cb = NULL;
	k_msgq_purge(&ipc_events);
}

static void execute_test_ping_pong(void)
{
	int ret;
	static const struct ipc_test_cmd cmd_ping = { IPC_TEST_CMD_PING };
	struct ipc_test_cmd *cmd_rsp;
	size_t cmd_rsp_size;

	zassert_not_ok(data_queue_is_empty(&ipc_data_queue),
		       "IPC data queue contains unexpected data");
	/* Sending data */
	ret = ipc_service_send(&ep, &cmd_ping, sizeof(cmd_ping));
	zassert_equal(ret, sizeof(cmd_ping), "ipc_service_send failed: %d, expected: %u", ret,
		      sizeof(cmd_ping));
	/* Waiting for response */
	cmd_rsp = data_queue_get(&ipc_data_queue, &cmd_rsp_size, K_MSEC(1000));
	zassert_not_null(cmd_rsp, "No command response on time");
	zassert_equal(cmd_rsp_size, sizeof(struct ipc_test_cmd),
		      "Unexpected response size: %u, expected: %u", cmd_rsp_size,
		      sizeof(struct ipc_test_cmd));
	zassert_equal(cmd_rsp->cmd, IPC_TEST_CMD_PONG,
		      "Unexpected response cmd value: %u, expected: %u", cmd_rsp->cmd,
		      IPC_TEST_CMD_PONG);
	data_queue_release(&ipc_data_queue, cmd_rsp);
}

ZTEST(ipc_sessions, test_ping_pong)
{
	execute_test_ping_pong();
}

ZTEST(ipc_sessions, test_echo)
{
	int ret;
	static const struct ipc_test_cmd cmd_echo = {
		IPC_TEST_CMD_ECHO, {'H', 'e', 'l', 'l', 'o', '!'}
	};
	struct ipc_test_cmd *cmd_rsp;
	size_t cmd_rsp_size;

	zassert_not_ok(data_queue_is_empty(&ipc_data_queue),
		       "IPC data queue contains unexpected data");
	/* Sending data */
	ret = ipc_service_send(&ep, &cmd_echo, sizeof(cmd_echo));
	zassert_equal(ret, sizeof(cmd_echo), "ipc_service_send failed: %d, expected: %u", ret,
		      sizeof(cmd_echo));
	/* Waiting for response */
	cmd_rsp = data_queue_get(&ipc_data_queue, &cmd_rsp_size, K_MSEC(1000));
	zassert_not_null(cmd_rsp, "No command response on time");
	/* Checking response */
	zassert_equal(cmd_rsp_size, sizeof(cmd_echo), "Unexpected response size: %u, expected: %u",
		      cmd_rsp_size, sizeof(cmd_echo));
	zassert_equal(cmd_rsp->cmd, IPC_TEST_CMD_ECHO_RSP,
		      "Unexpected response cmd value: %u, expected: %u", cmd_rsp->cmd,
		      IPC_TEST_CMD_ECHO_RSP);
	zassert_mem_equal(cmd_rsp->data, cmd_echo.data,
			  sizeof(cmd_echo) - sizeof(struct ipc_test_cmd),
			  "Unexpected response content");
	data_queue_release(&ipc_data_queue, cmd_rsp);
}

ZTEST(ipc_sessions, test_reboot)
{
	Z_TEST_SKIP_IFDEF(CONFIG_IPC_TEST_SKIP_UNBOUND);
	Z_TEST_SKIP_IFDEF(CONFIG_IPC_TEST_SKIP_CORE_RESET);

	int ret;
	struct test_ipc_event_state ev;
	static const struct ipc_test_cmd_reboot cmd_rebond = { { IPC_TEST_CMD_REBOOT }, 10 };

	zassert_not_ok(data_queue_is_empty(&ipc_data_queue),
		       "IPC data queue contains unexpected data");
	/* Sending data */
	ret = ipc_service_send(&ep, &cmd_rebond, sizeof(cmd_rebond));
	zassert_equal(ret, sizeof(cmd_rebond), "ipc_service_send failed: %d, expected: %u", ret,
		      sizeof(cmd_rebond));
	/* Waiting for IPC to unbound */
	ret = k_msgq_get(&ipc_events, &ev, K_MSEC(1000));
	zassert_ok(ret, "No IPC unbound event on time");
	zassert_equal(ev.ev, TEST_IPC_EVENT_UNBOUNDED, "Unexpected IPC event: %u, expected: %u",
		      ev.ev, TEST_IPC_EVENT_UNBOUNDED);
	zassert_equal_ptr(ev.ep, &ep, "Unexpected endpoint (unbound)");
	/* Reconnecting */
	ret = ipc_service_register_endpoint(ipc0_instance, &ep, &ep_cfg);
	zassert_true((ret >= 0), "ipc_service_register_endpoint() failure: %d", ret);
	/* Waiting for bound */
	ret = k_msgq_get(&ipc_events, &ev, K_MSEC(1000));
	zassert_ok(ret, "No IPC bound event on time");
	zassert_equal(ev.ev, TEST_IPC_EVENT_BOUNDED, "Unexpected IPC event: %u, expected: %u",
		      ev.ev, TEST_IPC_EVENT_UNBOUNDED);
	zassert_equal_ptr(ev.ep, &ep, "Unexpected endpoint (bound)");

	/* After reconnection - test communication */
	execute_test_ping_pong();
}

ZTEST(ipc_sessions, test_rebond)
{
	Z_TEST_SKIP_IFDEF(CONFIG_IPC_TEST_SKIP_UNBOUND);

	int ret;
	struct test_ipc_event_state ev;
	static const struct ipc_test_cmd_reboot cmd_rebond = { { IPC_TEST_CMD_REBOND }, 10 };

	zassert_not_ok(data_queue_is_empty(&ipc_data_queue),
		       "IPC data queue contains unexpected data");
	/* Sending data */
	ret = ipc_service_send(&ep, &cmd_rebond, sizeof(cmd_rebond));
	zassert_equal(ret, sizeof(cmd_rebond), "ipc_service_send failed: %d, expected: %u", ret,
		      sizeof(cmd_rebond));
	/* Waiting for IPC to unbound */
	ret = k_msgq_get(&ipc_events, &ev, K_MSEC(1000));
	zassert_ok(ret, "No IPC unbound event on time");
	zassert_equal(ev.ev, TEST_IPC_EVENT_UNBOUNDED, "Unexpected IPC event: %u, expected: %u",
		      ev.ev, TEST_IPC_EVENT_UNBOUNDED);
	zassert_equal_ptr(ev.ep, &ep, "Unexpected endpoint (unbound)");
	/* Reconnecting */
	ret = ipc_service_register_endpoint(ipc0_instance, &ep, &ep_cfg);
	zassert_true((ret >= 0), "ipc_service_register_endpoint() failure: %d", ret);
	/* Waiting for bound */
	ret = k_msgq_get(&ipc_events, &ev, K_MSEC(1000));
	zassert_ok(ret, "No IPC bound event on time");
	zassert_equal(ev.ev, TEST_IPC_EVENT_BOUNDED, "Unexpected IPC event: %u, expected: %u",
		      ev.ev, TEST_IPC_EVENT_UNBOUNDED);
	zassert_equal_ptr(ev.ep, &ep, "Unexpected endpoint (bound)");

	/* After reconnection - test communication */
	execute_test_ping_pong();
}

ZTEST(ipc_sessions, test_local_rebond)
{
	Z_TEST_SKIP_IFDEF(CONFIG_IPC_TEST_SKIP_UNBOUND);

	int ret;
	struct test_ipc_event_state ev;

	zassert_not_ok(data_queue_is_empty(&ipc_data_queue),
		       "IPC data queue contains unexpected data");
	/* Rebond locally */
	ret = ipc_service_deregister_endpoint(ep_cfg.priv);
	zassert_ok(ret, "ipc_service_deregister_endpoint() failure: %d", ret);
	ipc0_bounded = false;

	ret = ipc_service_register_endpoint(ipc0_instance, &ep, &ep_cfg);
	zassert_true((ret >= 0), "ipc_service_register_endpoint() failure: %d", ret);
	do {
		ret = k_msgq_get(&ipc_events, &ev, K_MSEC(1000));
		zassert_ok(ret, "Cannot bound to the remote interface");
	} while (!ipc0_bounded);

	/* After reconnection - test communication */
	execute_test_ping_pong();
}

ZTEST(ipc_sessions, test_tx_long)
{
	#define SEED_TXSTART_VALUE 1
	int ret;
	static const struct ipc_test_cmd_xstart cmd_rxstart = {
		.base = { .cmd = IPC_TEST_CMD_RXSTART },
		.blk_size = CONFIG_IPC_TEST_BLOCK_SIZE,
		.blk_cnt = CONFIG_IPC_TEST_BLOCK_CNT,
		.seed = SEED_TXSTART_VALUE };
	static const struct ipc_test_cmd cmd_rxget = { IPC_TEST_CMD_RXGET };
	struct test_cmd_xdata cmd_txdata = { .base = { .cmd = IPC_TEST_CMD_XDATA } };
	unsigned int seed = SEED_TXSTART_VALUE;

	struct ipc_test_cmd_xstat *cmd_rxstat;
	size_t cmd_rsp_size;

	zassert_not_ok(data_queue_is_empty(&ipc_data_queue),
		       "IPC data queue contains unexpected data");

	/* Sending command for the remote to start receiving the data */
	ret = ipc_service_send(&ep, &cmd_rxstart, sizeof(cmd_rxstart));
	zassert_equal(ret, sizeof(cmd_rxstart), "ipc_service_send failed: %d, expected: %u", ret,
		      sizeof(cmd_rxstart));
	/* Check current status */
	ret = ipc_service_send(&ep, &cmd_rxget, sizeof(cmd_rxget));
	zassert_equal(ret, sizeof(cmd_rxget), "ipc_service_send failed: %d, expected: %u", ret,
		      sizeof(cmd_rxget));
	cmd_rxstat = data_queue_get(&ipc_data_queue, &cmd_rsp_size, K_MSEC(1000));
	zassert_not_null(cmd_rxstat, "No command response on time");
	zassert_equal(cmd_rsp_size, sizeof(*cmd_rxstat),
		      "Unexpected response size: %u, expected: %u", cmd_rsp_size,
		      sizeof(cmd_rxstat));
	zassert_equal(cmd_rxstat->base.cmd, IPC_TEST_CMD_XSTAT,
		      "Unexpected command in response: %u", cmd_rxstat->base.cmd);
	zassert_ok(cmd_rxstat->result, "RX result not ok: %d", cmd_rxstat->result);
	zassert_equal(cmd_rxstat->blk_cnt, cmd_rxstart.blk_cnt,
		      "RX blk_cnt in status does not match start command: %u vs %u",
		      cmd_rxstat->blk_cnt, cmd_rxstart.blk_cnt);
	data_queue_release(&ipc_data_queue, cmd_rxstat);

	/* Sending data */
	for (size_t blk = 0; blk < cmd_rxstart.blk_cnt; ++blk) {
		for (size_t n = 0; n < cmd_rxstart.blk_size; ++n) {
			cmd_txdata.data[n] = (uint8_t)rand_r(&seed);
		}
		do {
			ret = ipc_service_send(&ep, &cmd_txdata, sizeof(cmd_txdata));
		} while (ret == -ENOMEM);
		if ((blk % 1000) == 0) {
			LOG_INF("Transfer number: %u of %u", blk, cmd_rxstart.blk_cnt);
		}
		zassert_equal(ret, sizeof(cmd_txdata), "ipc_service_send failed: %d, expected: %u",
			      ret, sizeof(cmd_txdata));
	}

	/* Check current status */
	ret = ipc_service_send(&ep, &cmd_rxget, sizeof(cmd_rxget));
	zassert_equal(ret, sizeof(cmd_rxget), "ipc_service_send failed: %d, expected: %u", ret,
		      sizeof(cmd_rxget));
	cmd_rxstat = data_queue_get(&ipc_data_queue, &cmd_rsp_size, K_MSEC(1000));
	zassert_not_null(cmd_rxstat, "No command response on time");
	zassert_equal(cmd_rsp_size, sizeof(*cmd_rxstat),
		      "Unexpected response size: %u, expected: %u", cmd_rsp_size,
		      sizeof(cmd_rxstat));
	zassert_equal(cmd_rxstat->base.cmd, IPC_TEST_CMD_XSTAT,
		      "Unexpected command in response: %u", cmd_rxstat->base.cmd);
	zassert_ok(cmd_rxstat->result, "RX result not ok: %d", cmd_rxstat->result);
	zassert_equal(cmd_rxstat->blk_cnt, 0,
		      "RX blk_cnt in status does not match start command: %u vs %u",
		      cmd_rxstat->blk_cnt, 0);
	data_queue_release(&ipc_data_queue, cmd_rxstat);
}

static struct {
	unsigned int seed;
	size_t blk_left;
} test_rx_long_data;
K_SEM_DEFINE(test_rx_long_sem, 0, 1);

static void test_rx_long_rec_cb(const void *data, size_t len, void *priv)
{
	const struct test_cmd_xdata *cmd_rxdata = data;

	zassert_true(test_rx_long_data.blk_left > 0, "No data left to interpret");
	zassert_equal(len, sizeof(*cmd_rxdata),
		      "Unexpected response size: %u, expected: %u", len, sizeof(*cmd_rxdata));
	zassert_equal(cmd_rxdata->base.cmd, IPC_TEST_CMD_XDATA,
		      "Unexpected command in response: %u", cmd_rxdata->base.cmd);
	for (size_t n = 0; n < CONFIG_IPC_TEST_BLOCK_SIZE; ++n) {
		uint8_t expected = (uint8_t)rand_r(&test_rx_long_data.seed);

		zassert_equal(cmd_rxdata->data[n], expected,
			      "Data mismatch at %u while %u blocks left", n,
			      test_rx_long_data.blk_left);
	}

	if (test_rx_long_data.blk_left % 1000 == 0) {
		LOG_INF("Receivng left: %u", test_rx_long_data.blk_left);
	}
	test_rx_long_data.blk_left -= 1;
	if (test_rx_long_data.blk_left <= 0) {
		LOG_INF("Interpretation marked finished");
		ep_received_override_cb = NULL;
		k_sem_give(&test_rx_long_sem);
	}
}

ZTEST(ipc_sessions, test_rx_long)
{
	#define SEED_RXSTART_VALUE 1
	int ret;
	static const struct ipc_test_cmd_xstart cmd_txstart = {
		.base = { .cmd = IPC_TEST_CMD_TXSTART },
		.blk_size = CONFIG_IPC_TEST_BLOCK_SIZE,
		.blk_cnt = CONFIG_IPC_TEST_BLOCK_CNT,
		.seed = SEED_RXSTART_VALUE };
	static const struct ipc_test_cmd cmd_txget = { IPC_TEST_CMD_TXGET };
	struct ipc_test_cmd_xstat *cmd_txstat;
	size_t cmd_rsp_size;

	zassert_not_ok(data_queue_is_empty(&ipc_data_queue),
		       "IPC data queue contains unexpected data");

	/* Configuring the callback to interpret the incoming data */
	test_rx_long_data.seed = SEED_RXSTART_VALUE;
	test_rx_long_data.blk_left = cmd_txstart.blk_cnt;
	ep_received_override_cb = test_rx_long_rec_cb;

	/* Sending command for the remote to start sending the data */
	ret = ipc_service_send(&ep, &cmd_txstart, sizeof(cmd_txstart));
	zassert_equal(ret, sizeof(cmd_txstart), "ipc_service_send failed: %d, expected: %u", ret,
		      sizeof(cmd_txstart));

	/* Waiting for all the data */
	ret = k_sem_take(&test_rx_long_sem, K_SECONDS(30));
	LOG_INF("Interpretation finished");
	zassert_ok(ret, "Incoming packet interpretation timeout");
	zassert_is_null(ep_received_override_cb, "Seems like interpretation callback failed");

	/* Check current status */
	ret = ipc_service_send(&ep, &cmd_txget, sizeof(cmd_txget));
	zassert_equal(ret, sizeof(cmd_txget), "ipc_service_send failed: %d, expected: %u", ret,
		      sizeof(cmd_txget));
	cmd_txstat = data_queue_get(&ipc_data_queue, &cmd_rsp_size, K_MSEC(1000));
	zassert_not_null(cmd_txstat, "No command response on time");
	zassert_equal(cmd_rsp_size, sizeof(*cmd_txstat),
		      "Unexpected response size: %u, expected: %u", cmd_rsp_size,
		      sizeof(cmd_txstat));
	zassert_equal(cmd_txstat->base.cmd, IPC_TEST_CMD_XSTAT,
		      "Unexpected command in response: %u", cmd_txstat->base.cmd);
	zassert_ok(cmd_txstat->result, "RX result not ok: %d", cmd_txstat->result);
	zassert_equal(cmd_txstat->blk_cnt, 0,
		      "RX blk_cnt in status does not match start command: %u vs %u",
		      cmd_txstat->blk_cnt, 0);
	data_queue_release(&ipc_data_queue, cmd_txstat);
}


ZTEST_SUITE(
	/* suite_name */              ipc_sessions,
	/* ztest_suite_predicate_t */ NULL,
	/* ztest_suite_setup_t */     test_suite_setup,
	/* ztest_suite_before_t */    test_suite_before,
	/* ztest_suite_after_t */     NULL,
	/* ztest_suite_teardown_t */  NULL
);
