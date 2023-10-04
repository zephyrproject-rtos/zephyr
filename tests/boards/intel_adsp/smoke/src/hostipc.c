/* Copyright (c) 2022 Intel Corporation
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/kernel.h>
#include <zephyr/ztest.h>
#include <intel_adsp_ipc.h>
#include "tests.h"

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

static bool ipc_done(const struct device *dev, void *arg)
{
	zassert_is_null(arg, "wrong done arg");
	zassert_false(done_flag, "done called unexpectedly");
	done_flag = true;
	return false;
}

ZTEST(intel_adsp, test_host_ipc)
{
	bool ret;

	intel_adsp_ipc_set_message_handler(INTEL_ADSP_IPC_HOST_DEV, ipc_message, NULL);
	intel_adsp_ipc_set_done_handler(INTEL_ADSP_IPC_HOST_DEV, ipc_done, NULL);

	/* Just send a message and wait for it to complete */
	printk("Simple message send...\n");
	done_flag = false;
	ret = intel_adsp_ipc_send_message(INTEL_ADSP_IPC_HOST_DEV, IPCCMD_SIGNAL_DONE, 0);
	zassert_true(ret, "send failed");
	AWAIT(intel_adsp_ipc_is_complete(INTEL_ADSP_IPC_HOST_DEV));
	AWAIT(done_flag);

	/* Request the host to return a message which we will complete
	 * immediately.
	 */
	printk("Return message request...\n");
	done_flag = false;
	msg_flag = false;
	ret = intel_adsp_ipc_send_message(INTEL_ADSP_IPC_HOST_DEV, IPCCMD_RETURN_MSG,
				RETURN_MSG_SYNC_VAL);
	zassert_true(ret, "send failed");
	AWAIT(done_flag);
	AWAIT(intel_adsp_ipc_is_complete(INTEL_ADSP_IPC_HOST_DEV));
	AWAIT(msg_flag);

	/* Do exactly the same thing again to check for state bugs
	 * (e.g. failing to signal done on one side or the other)
	 */
	printk("Return message request 2...\n");
	done_flag = false;
	msg_flag = false;
	ret = intel_adsp_ipc_send_message(INTEL_ADSP_IPC_HOST_DEV, IPCCMD_RETURN_MSG,
				RETURN_MSG_SYNC_VAL);
	zassert_true(ret, "send failed");
	AWAIT(done_flag);
	AWAIT(intel_adsp_ipc_is_complete(INTEL_ADSP_IPC_HOST_DEV));
	AWAIT(msg_flag);

	/* Same, but we'll complete it asynchronously (1.8+ only) */
	printk("Return message request, async...\n");
	done_flag = false;
	msg_flag = false;
	ret = intel_adsp_ipc_send_message(INTEL_ADSP_IPC_HOST_DEV, IPCCMD_RETURN_MSG,
				RETURN_MSG_ASYNC_VAL);
	zassert_true(ret, "send failed");
	AWAIT(done_flag);
	AWAIT(intel_adsp_ipc_is_complete(INTEL_ADSP_IPC_HOST_DEV));
	AWAIT(msg_flag);
	intel_adsp_ipc_complete(INTEL_ADSP_IPC_HOST_DEV);

	/* Now make a synchronous call with (on the host) a delayed
	 * completion and make sure the interrupt fires and wakes us
	 * up. (On 1.5 a delay isn't possible and this will complete
	 * immediately).
	 */
	printk("Synchronous message send...\n");
	done_flag = false;
	ret = intel_adsp_ipc_send_message_sync(INTEL_ADSP_IPC_HOST_DEV, IPCCMD_ASYNC_DONE_DELAY,
					 0, K_FOREVER);
	zassert_true(ret, "send failed");
	zassert_true(done_flag, "done interrupt failed to fire");
	zassert_true(intel_adsp_ipc_is_complete(INTEL_ADSP_IPC_HOST_DEV),
		"sync message incomplete");

	/* Clean up. Further tests might want to use IPC */
	intel_adsp_ipc_set_message_handler(INTEL_ADSP_IPC_HOST_DEV, NULL, NULL);
	intel_adsp_ipc_set_done_handler(INTEL_ADSP_IPC_HOST_DEV, NULL, NULL);
}
