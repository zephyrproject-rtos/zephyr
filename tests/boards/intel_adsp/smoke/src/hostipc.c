/*
 * Copyright (c) 2022, 2025 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/ztest.h>
#include <intel_adsp_ipc.h>
#include "tests.h"

static volatile bool done_flag, msg_flag;

#define RETURN_MSG_SYNC_VAL  0x12345
#define RETURN_MSG_ASYNC_VAL 0x54321

#ifdef CONFIG_INTEL_ADSP_IPC_OLD_INTERFACE
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
#else /* CONFIG_INTEL_ADSP_IPC_OLD_INTERFACE */

#include <zephyr/ipc/ipc_msg_service.h>
#include <zephyr/ipc/backends/ipc_msg_intel_adsp_ipc.h>

struct ipc_msg_ept ipc_ept;

int ipc_receive_cb(uint16_t msg_type, const void *msg_data, void *priv)
{
	zassert_true(msg_type == INTEL_ADSP_IPC_MSG ||
		     msg_type == INTEL_ADSP_IPC_MSG_SYNC, "unexpected msg type");

	const struct intel_adsp_ipc_msg *msg = (const struct intel_adsp_ipc_msg *)msg_data;

	zassert_equal(msg->data, msg->extdata, "unequal message data/ext_data");
	zassert_true(msg->data == RETURN_MSG_SYNC_VAL ||
		     msg->data == RETURN_MSG_ASYNC_VAL, "unexpected msg data");

	msg_flag = true;

	return 0;
}

int ipc_event_cb(uint16_t evt_type, const void *evt_data, void *priv)
{
	zassert_false(done_flag, "done called unexpectedly");

	done_flag = true;

	return 0;
}

struct ipc_msg_ept_cfg host_ipc_ept_cfg = {
	.name = "host_ipc_ept",
	.cb = {
		.received = ipc_receive_cb,
		.event = ipc_event_cb,
	},
};

static void intel_adsp_ipc_complete(const struct device *dev)
{
	int ret;

	ret = ipc_msg_service_send(&ipc_ept, INTEL_ADSP_IPC_MSG_DONE, NULL);

	ARG_UNUSED(ret);
}

static bool intel_adsp_ipc_is_complete(const struct device *dev)
{
	int ret;

	ret = ipc_msg_service_query(&ipc_ept, INTEL_ADSP_IPC_QUERY_IS_COMPLETE, NULL, NULL);

	return ret == 0;
}

int intel_adsp_ipc_send_message(const struct device *dev, uint32_t data, uint32_t ext_data)
{
	struct intel_adsp_ipc_msg msg = {.data = data, .extdata = ext_data};
	int ret;

	ret = ipc_msg_service_send(&ipc_ept, INTEL_ADSP_IPC_MSG, &msg);

	return ret;
}

static int intel_adsp_ipc_send_message_sync(const struct device *dev, uint32_t data,
					    uint32_t ext_data, k_timeout_t timeout)
{
	struct intel_adsp_ipc_msg_sync msg = {
		.data = data, .extdata = ext_data, .timeout = timeout};
	int ret;

	ret = ipc_msg_service_send(&ipc_ept, INTEL_ADSP_IPC_MSG_SYNC, &msg);

	return ret;
}
#endif /* CONFIG_INTEL_ADSP_IPC_OLD_INTERFACE */

ZTEST(intel_adsp, test_host_ipc)
{
	int ret;

#ifdef CONFIG_INTEL_ADSP_IPC_OLD_INTERFACE
	intel_adsp_ipc_set_message_handler(INTEL_ADSP_IPC_HOST_DEV, ipc_message, NULL);
	intel_adsp_ipc_set_done_handler(INTEL_ADSP_IPC_HOST_DEV, ipc_done, NULL);
#else /* CONFIG_INTEL_ADSP_IPC_OLD_INTERFACE */
	ret = ipc_msg_service_register_endpoint(INTEL_ADSP_IPC_HOST_DEV, &ipc_ept,
						&host_ipc_ept_cfg);
	zassert_equal(ret, 0, "cannot register IPC endpoint");
#endif /* CONFIG_INTEL_ADSP_IPC_OLD_INTERFACE */

	/* Just send a message and wait for it to complete */
	printk("Simple message send...\n");
	done_flag = false;
	ret = intel_adsp_ipc_send_message(INTEL_ADSP_IPC_HOST_DEV, IPCCMD_SIGNAL_DONE, 0);
	zassert_true(!ret, "send failed");
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
	zassert_true(!ret, "send failed");
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
	zassert_true(!ret, "send failed");
	AWAIT(done_flag);
	AWAIT(intel_adsp_ipc_is_complete(INTEL_ADSP_IPC_HOST_DEV));
	AWAIT(msg_flag);

	/* Same, but we'll complete it asynchronously (1.8+ only) */
	printk("Return message request, async...\n");
	done_flag = false;
	msg_flag = false;
	ret = intel_adsp_ipc_send_message(INTEL_ADSP_IPC_HOST_DEV, IPCCMD_RETURN_MSG,
				RETURN_MSG_ASYNC_VAL);
	zassert_true(!ret, "send failed");
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
	zassert_true(!ret, "send failed");
	zassert_true(done_flag, "done interrupt failed to fire");
	zassert_true(intel_adsp_ipc_is_complete(INTEL_ADSP_IPC_HOST_DEV),
		"sync message incomplete");

#ifdef CONFIG_INTEL_ADSP_IPC_OLD_INTERFACE
	/* Clean up. Further tests might want to use IPC */
	intel_adsp_ipc_set_message_handler(INTEL_ADSP_IPC_HOST_DEV, NULL, NULL);
	intel_adsp_ipc_set_done_handler(INTEL_ADSP_IPC_HOST_DEV, NULL, NULL);
#else /* CONFIG_INTEL_ADSP_IPC_OLD_INTERFACE */
	ret = ipc_msg_service_deregister_endpoint(&ipc_ept);
	zassert_equal(ret, 0, "cannot de-register IPC endpoint");
#endif /* CONFIG_INTEL_ADSP_IPC_OLD_INTERFACE */
}
