/* Copyright (c) 2022 Intel Corporation
 * SPDX-License-Identifier: Apache-2.0
 */
#include <ztest.h>
#include <zephyr/drivers/ipm.h>
#include "tests.h"

#define IPM_DEV device_get_binding("ipm_cavs_host")

/* Two values impossible to transmit in a cAVS ID */
#define ID_INBOUND 0xfffffff0
#define ID_INVALID 0xffffffff

static K_SEM_DEFINE(ipm_sem, 0, 1);

static const uint32_t msg[] = { 29, 15, 58, 71, 99 };

static uint32_t received_id = ID_INVALID;
static volatile uint32_t *received_data;

static void ipm_msg(const struct device *ipmdev, void *user_data,
		    uint32_t id, volatile void *data)
{
	zassert_equal(ipmdev, IPM_DEV, "wrong device");
	zassert_equal(user_data, NULL, "wrong user_data pointer");
	zassert_equal(received_id, ID_INBOUND, "unexpected message");

	received_id = id;
	received_data = data;

	k_sem_give(&ipm_sem);
}

static void msg_transact(bool do_wait)
{
	/* Send an IPCCMD_RETURN_MSG, this will send us a return
	 * message with msg[0] as the ID (on cAVS 1.8+, otherwise
	 * zero).
	 */
	received_id = ID_INBOUND;
	ipm_send(IPM_DEV, do_wait, IPCCMD_RETURN_MSG, msg, sizeof(msg));

	/* Wait for the return message */
	k_sem_take(&ipm_sem, K_FOREVER);

	zassert_equal(received_id,
		      IS_ENABLED(IPM_CAVS_HOST_REGWORD) ? msg[0] : 0,
		      "wrong return message ID");

	received_id = ID_INVALID;

	/* Now whitebox the message protocol: copy the message buffer
	 * (on the host side!) from the outbox to the inbox.  That
	 * will write into our "already received" inbox buffer memory.
	 * We do this using the underlying cavs_ipc API, which works
	 * only because we know it works.  Note that on cAVS 1.8+, the
	 * actual in-use amount of the message will be one word
	 * shorter (because the first word is sent as IPC ext_data),
	 * but it won't be inspected below.
	 */
	for (int i = 0; i < ARRAY_SIZE(msg); i++) {
		cavs_ipc_send_message_sync(CAVS_HOST_DEV, IPCCMD_WINCOPY,
					   (i << 16) | i, K_FOREVER);
	}

	/* Validate data */
	for (int i = 0; i < ARRAY_SIZE(msg); i++) {
		zassert_equal(msg[i], received_data[i], "wrong message data");
	}
}

/* This is a little whiteboxey.  It relies on the knowledge that an
 * IPM message is nothing but a IPC message with the "id" parameter
 * passed as data (and, on cAVS 1.8+ only, the first word of the
 * message buffer passed as ext_data).
 */
void test_ipm_cavs_host(void)
{
	/* Restore IPM driver state (we've been mucking with cavs_ipc tests) */
	cavs_ipc_set_message_handler(CAVS_HOST_DEV, ipm_handler, (void *)IPM_DEV);
	cavs_ipc_set_done_handler(CAVS_HOST_DEV, NULL, NULL);

	ipm_register_callback(IPM_DEV, ipm_msg, NULL);

	/* Do it twice just for coverage on the wait parameter */
	msg_transact(true);
	msg_transact(false);
}
