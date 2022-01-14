/* Copyright (c) 2022 Intel Corporation
 * SPDX-License-Identifier: Apache-2.0
 */
#include <kernel.h>
#include <ztest.h>
#include <cavs_ipc.h>
#include "tests.h"

/* The cavstool.py script that launched us listens for a very simple
 * set of IPC commands to help test.  Pass one of the following values
 * as the "data" argument to cavs_ipc_send_message():
 *
 * 0: The host takes no action, but signals DONE to complete the message
 * 1: The host returns done after a short timeout
 * 2: The host issues a new message with the ext_data value as its "data"
 */
enum cavstool_cmd { SIGNAL_DONE, ASYNC_DONE_DELAY, RETURN_MSG };

static volatile bool done_flag, msg_flag;

#define RETURN_MSG_SYNC_VAL  0x12345
#define RETURN_MSG_ASYNC_VAL 0x54321

static bool ipc_message(const struct device *dev, void *arg,
			uint32_t data, uint32_t ext_data)
{
	zassert_is_null(arg, "wrong message arg");
	zassert_equal(data, ext_data, "unequal message data/ext_data");
	zassert_true(data == RETURN_MSG_SYNC_VAL ||
		     data == RETURN_MSG_ASYNC_VAL, "unexpected msg data");
	msg_flag = true;
	return data == RETURN_MSG_SYNC_VAL;
}

static void ipc_done(const struct device *dev, void *arg)
{
	zassert_is_null(arg, "wrong done arg");
	zassert_false(done_flag, "done called unexpectedly");
	done_flag = true;
}

void test_host_ipc(void)
{
	bool ret;

	cavs_ipc_set_message_handler(CAVS_HOST_DEV, ipc_message, NULL);
	cavs_ipc_set_done_handler(CAVS_HOST_DEV, ipc_done, NULL);

	/* Just send a message and wait for it to complete */
	printk("Simple message send...\n");
	done_flag = false;
	ret = cavs_ipc_send_message(CAVS_HOST_DEV, SIGNAL_DONE, 0);
	zassert_true(ret, "send failed");
	WAIT_FOR(cavs_ipc_is_complete(CAVS_HOST_DEV));
	WAIT_FOR(done_flag);

	/* Request the host to return a message which we will complete
	 * immediately.
	 */
	printk("Return message request...\n");
	done_flag = false;
	msg_flag = false;
	ret = cavs_ipc_send_message(CAVS_HOST_DEV, RETURN_MSG,
				    RETURN_MSG_SYNC_VAL);
	zassert_true(ret, "send failed");
	WAIT_FOR(done_flag);
	WAIT_FOR(cavs_ipc_is_complete(CAVS_HOST_DEV));
	WAIT_FOR(msg_flag);

	/* Do exactly the same thing again to check for state bugs
	 * (e.g. failing to signal done on one side or the other)
	 */
	printk("Return message request 2...\n");
	done_flag = false;
	msg_flag = false;
	ret = cavs_ipc_send_message(CAVS_HOST_DEV, RETURN_MSG,
				    RETURN_MSG_SYNC_VAL);
	zassert_true(ret, "send failed");
	WAIT_FOR(done_flag);
	WAIT_FOR(cavs_ipc_is_complete(CAVS_HOST_DEV));
	WAIT_FOR(msg_flag);

	/* Same, but we'll complete it asynchronously (1.8+ only) */
	if (!IS_ENABLED(CONFIG_SOC_SERIES_INTEL_CAVS_V15)) {
		printk("Return message request, async...\n");
		done_flag = false;
		msg_flag = false;
		ret = cavs_ipc_send_message(CAVS_HOST_DEV, RETURN_MSG,
					    RETURN_MSG_ASYNC_VAL);
		zassert_true(ret, "send failed");
		WAIT_FOR(done_flag);
		WAIT_FOR(cavs_ipc_is_complete(CAVS_HOST_DEV));
		WAIT_FOR(msg_flag);
		cavs_ipc_complete(CAVS_HOST_DEV);
	}

	/* Now make a synchronous call with (on the host) a delayed
	 * completion and make sure the interrupt fires and wakes us
	 * up.  (On 1.5 a delay isn't possible and this will complete
	 * immediately).
	 */
	printk("Synchronous message send...\n");
	done_flag = false;
	ret = cavs_ipc_send_message_sync(CAVS_HOST_DEV, ASYNC_DONE_DELAY, 0, K_FOREVER);
	zassert_true(ret, "send failed");
	zassert_true(done_flag, "done interrupt failed to fire");
	zassert_true(cavs_ipc_is_complete(CAVS_HOST_DEV), "sync message incomplete");
}
