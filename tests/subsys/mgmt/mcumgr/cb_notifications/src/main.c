/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/net_buf.h>
#include <zephyr/mgmt/mcumgr/mgmt/mgmt.h>
#include <zephyr/mgmt/mcumgr/transport/smp_dummy.h>
#include <zephyr/mgmt/mcumgr/mgmt/callbacks.h>
#include <zephyr/mgmt/mcumgr/grp/os_mgmt/os_mgmt.h>
#include <zcbor_common.h>
#include <zcbor_decode.h>
#include <zcbor_encode.h>
#include <mgmt/mcumgr/util/zcbor_bulk.h>
#include <zephyr/version.h>
#include <string.h>
#include <smp_internal.h>
#include "smp_test_util.h"

#define SMP_RESPONSE_WAIT_TIME 3
#define ZCBOR_BUFFER_SIZE 64
#define OUTPUT_BUFFER_SIZE 64
#define ZCBOR_HISTORY_ARRAY_SIZE 4

/* Test sets */
enum {
	CB_NOTIFICATION_TEST_CALLBACK_DISABLED,
	CB_NOTIFICATION_TEST_CALLBACK_ENABLED,
	CB_NOTIFICATION_TEST_CALLBACK_DISABLED_VERIFY,

	CB_NOTIFICATION_TEST_SET_COUNT
};

static struct net_buf *nb;

struct state {
	uint8_t test_set;
};

static struct state test_state = {
	.test_set = 0,
};

static bool cmd_recv_got;
static bool cmd_status_got;
static bool cmd_done_got;
static bool cmd_other_got;

/* Responses to commands */

static enum mgmt_cb_return mgmt_event_cmd_callback(uint32_t event, enum mgmt_cb_return prev_status,
						   int32_t *rc, uint16_t *group, bool *abort_more,
						   void *data, size_t data_size)
{
	if (event == MGMT_EVT_OP_CMD_RECV) {
		cmd_recv_got = true;
	} else if (event == MGMT_EVT_OP_CMD_STATUS) {
		cmd_status_got = true;
	} else if (event == MGMT_EVT_OP_CMD_DONE) {
		cmd_done_got = true;
	} else {
		cmd_other_got = true;
	}

	return MGMT_CB_OK;
}

static struct mgmt_callback mgmt_event_callback = {
	.callback = mgmt_event_cmd_callback,
	.event_id = (MGMT_EVT_OP_CMD_RECV | MGMT_EVT_OP_CMD_STATUS | MGMT_EVT_OP_CMD_DONE),
};

static void *setup_callbacks(void)
{
	mgmt_callback_register(&mgmt_event_callback);
	return NULL;
}

static void destroy_callbacks(void *p)
{
	mgmt_callback_unregister(&mgmt_event_callback);
}

static inline void wait_for_sync(void)
{
#ifdef CONFIG_SMP
	/* For SMP systems, it is possible that a dummy response is fully received and processed
	 * prior to the callback code being executed, therefore implement a dummy wait to wait
	 * for callback synchronisation to take place.
	 */
	k_sleep(K_MSEC(1));
#endif
}

ZTEST(callback_disabled, test_notifications_disabled)
{
	uint8_t buffer[ZCBOR_BUFFER_SIZE];
	uint8_t buffer_out[OUTPUT_BUFFER_SIZE];
	bool ok;
	uint16_t buffer_size;
	zcbor_state_t zse[ZCBOR_HISTORY_ARRAY_SIZE] = { 0 };
	zcbor_state_t zsd[ZCBOR_HISTORY_ARRAY_SIZE] = { 0 };
	bool received;

	memset(buffer, 0, sizeof(buffer));
	memset(buffer_out, 0, sizeof(buffer_out));
	buffer_size = 0;
	memset(zse, 0, sizeof(zse));
	memset(zsd, 0, sizeof(zsd));

	zcbor_new_encode_state(zse, 2, buffer, ARRAY_SIZE(buffer), 0);

	ok = create_mcumgr_format_packet(zse, buffer, buffer_out, &buffer_size);
	zassert_true(ok, "Expected packet creation to be successful\n");

	/* Enable dummy SMP backend and ready for usage */
	smp_dummy_enable();
	smp_dummy_clear_state();

	/* Send query command to dummy SMP backend */
	(void)smp_dummy_tx_pkt(buffer_out, buffer_size);
	smp_dummy_add_data();

	/* For a short duration to see if response has been received */
	received = smp_dummy_wait_for_data(SMP_RESPONSE_WAIT_TIME);

	zassert_true(received, "Expected to receive data but timed out\n");

	/* Retrieve response buffer */
	nb = smp_dummy_get_outgoing();
	smp_dummy_disable();

	/* Check events */
	wait_for_sync();
	zassert_false(cmd_recv_got, "Did not expect received command callback\n");
	zassert_false(cmd_status_got, "Did not expect IMG status callback\n");
	zassert_false(cmd_done_got, "Did not expect done command callback\n");
	zassert_false(cmd_other_got, "Did not expect other callback(s)\n");
}

ZTEST(callback_enabled, test_notifications_enabled)
{
	uint8_t buffer[ZCBOR_BUFFER_SIZE];
	uint8_t buffer_out[OUTPUT_BUFFER_SIZE];
	bool ok;
	uint16_t buffer_size;
	zcbor_state_t zse[ZCBOR_HISTORY_ARRAY_SIZE] = { 0 };
	zcbor_state_t zsd[ZCBOR_HISTORY_ARRAY_SIZE] = { 0 };
	bool received;

	memset(buffer, 0, sizeof(buffer));
	memset(buffer_out, 0, sizeof(buffer_out));
	buffer_size = 0;
	memset(zse, 0, sizeof(zse));
	memset(zsd, 0, sizeof(zsd));

	zcbor_new_encode_state(zse, 2, buffer, ARRAY_SIZE(buffer), 0);

	ok = create_mcumgr_format_packet(zse, buffer, buffer_out, &buffer_size);
	zassert_true(ok, "Expected packet creation to be successful\n");

	/* Enable dummy SMP backend and ready for usage */
	smp_dummy_enable();
	smp_dummy_clear_state();

	/* Send query command to dummy SMP backend */
	(void)smp_dummy_tx_pkt(buffer_out, buffer_size);
	smp_dummy_add_data();

	/* For a short duration to see if response has been received */
	received = smp_dummy_wait_for_data(SMP_RESPONSE_WAIT_TIME);

	zassert_true(received, "Expected to receive data but timed out\n");

	/* Retrieve response buffer */
	nb = smp_dummy_get_outgoing();
	smp_dummy_disable();

	/* Check events */
	wait_for_sync();
	zassert_true(cmd_recv_got, "Expected received command callback\n");
	zassert_false(cmd_status_got, "Did not expect IMG status callback\n");
	zassert_true(cmd_done_got, "Expected done command callback\n");
	zassert_false(cmd_other_got, "Did not expect other callback(s)\n");
}

ZTEST(callback_disabled_verify, test_notifications_disabled_verify)
{
	uint8_t buffer[ZCBOR_BUFFER_SIZE];
	uint8_t buffer_out[OUTPUT_BUFFER_SIZE];
	bool ok;
	uint16_t buffer_size;
	zcbor_state_t zse[ZCBOR_HISTORY_ARRAY_SIZE] = { 0 };
	zcbor_state_t zsd[ZCBOR_HISTORY_ARRAY_SIZE] = { 0 };
	bool received;

	memset(buffer, 0, sizeof(buffer));
	memset(buffer_out, 0, sizeof(buffer_out));
	buffer_size = 0;
	memset(zse, 0, sizeof(zse));
	memset(zsd, 0, sizeof(zsd));

	zcbor_new_encode_state(zse, 2, buffer, ARRAY_SIZE(buffer), 0);

	ok = create_mcumgr_format_packet(zse, buffer, buffer_out, &buffer_size);
	zassert_true(ok, "Expected packet creation to be successful\n");

	/* Enable dummy SMP backend and ready for usage */
	smp_dummy_enable();
	smp_dummy_clear_state();

	/* Send query command to dummy SMP backend */
	(void)smp_dummy_tx_pkt(buffer_out, buffer_size);
	smp_dummy_add_data();

	/* For a short duration to see if response has been received */
	received = smp_dummy_wait_for_data(SMP_RESPONSE_WAIT_TIME);

	zassert_true(received, "Expected to receive data but timed out\n");

	/* Retrieve response buffer */
	nb = smp_dummy_get_outgoing();
	smp_dummy_disable();

	/* Check events */
	wait_for_sync();
	zassert_false(cmd_recv_got, "Did not expect received command callback\n");
	zassert_false(cmd_status_got, "Did not expect IMG status callback\n");
	zassert_false(cmd_done_got, "Did not expect done command callback\n");
	zassert_false(cmd_other_got, "Did not expect other callback(s)\n");
}

static void cleanup_test(void *p)
{
	if (nb != NULL) {
		net_buf_unref(nb);
		nb = NULL;
	}

	cmd_recv_got = false;
	cmd_status_got = false;
	cmd_done_got = false;
	cmd_other_got = false;
}

void test_main(void)
{
	while (test_state.test_set < CB_NOTIFICATION_TEST_SET_COUNT) {
		ztest_run_all(&test_state, false, 1, 1);
		++test_state.test_set;
	}

	ztest_verify_all_test_suites_ran();
}

static bool callback_disabled_predicate(const void *state)
{
	return ((struct state *)state)->test_set == CB_NOTIFICATION_TEST_CALLBACK_DISABLED;
}

static bool callback_enabled_predicate(const void *state)
{
	return ((struct state *)state)->test_set == CB_NOTIFICATION_TEST_CALLBACK_ENABLED;
}

static bool callback_disabled_verify_predicate(const void *state)
{
	return ((struct state *)state)->test_set == CB_NOTIFICATION_TEST_CALLBACK_DISABLED_VERIFY;
}

ZTEST_SUITE(callback_disabled, callback_disabled_predicate, NULL, NULL, cleanup_test, NULL);
ZTEST_SUITE(callback_enabled, callback_enabled_predicate, setup_callbacks, NULL, cleanup_test,
	    destroy_callbacks);
ZTEST_SUITE(callback_disabled_verify, callback_disabled_verify_predicate, NULL, NULL,
	    cleanup_test, NULL);
