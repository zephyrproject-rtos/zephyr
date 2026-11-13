/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <sys/types.h>

#include <zephyr/bluetooth/audio/aics.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/fff.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>
#include <zephyr/ztest.h>

#include "aics_internal.h"
#include "gatt.h"

#define HELPER_STACK_SIZE 2048
#define MAX_EVENTS        8U

DEFINE_FFF_GLOBALS;

enum cb_type {
	CB_STATE,
	CB_DESCRIPTION,
};

struct cb_event {
	k_tid_t thread;
	enum cb_type type;
	int8_t gain;
	char description[BT_AICS_DESC_BUF_SIZE];
};

static struct bt_aics *aics;
static struct cb_event events[MAX_EVENTS];
static size_t event_cnt;
static bool block_first_cb;
static bool nested_call;
static int nested_err;

static K_SEM_DEFINE(cb_entered, 0, 1);
static K_SEM_DEFINE(cb_continue, 0, 1);

K_THREAD_STACK_DEFINE(helper_stack, HELPER_STACK_SIZE);
static struct k_thread helper_thread;

static void cb_common(void)
{
	if (nested_call) {
		nested_call = false;
		nested_err = bt_aics_gain_set(aics, 0);
	}

	if (block_first_cb && event_cnt == 1U) {
		k_sem_give(&cb_entered);
		k_sem_take(&cb_continue, K_FOREVER);
	}
}

static void state_cb(struct bt_aics *inst, int err, int8_t gain, uint8_t mute, uint8_t mode)
{
	zassert_true(event_cnt < MAX_EVENTS);

	events[event_cnt].thread = k_current_get();
	events[event_cnt].type = CB_STATE;
	events[event_cnt].gain = gain;
	event_cnt++;

	cb_common();
}

static void description_cb(struct bt_aics *inst, int err, char *description)
{
	zassert_true(event_cnt < MAX_EVENTS);

	events[event_cnt].thread = k_current_get();
	events[event_cnt].type = CB_DESCRIPTION;
	(void)strcpy(events[event_cnt].description, description);
	event_cnt++;

	cb_common();
}

static struct bt_aics_cb aics_cb = {
	.state = state_cb,
	.description = description_cb,
};

static const struct bt_gatt_attr *aics_attr_get(const struct bt_uuid *uuid)
{
	const struct bt_gatt_attr *attr;

	attr = bt_gatt_find_by_uuid(bt_aics_svc_decl_get(aics), aics->srv.service_p->attr_count,
				    uuid);
	zassert_not_null(attr);

	return attr;
}

/* Calls the GATT write callbacks the same way as the ATT layer does for writes from a client */
static ssize_t remote_gain_write(int8_t gain)
{
	const struct bt_gatt_attr *state_attr = aics_attr_get(BT_UUID_AICS_STATE);
	const struct bt_gatt_attr *cp_attr = aics_attr_get(BT_UUID_AICS_CONTROL);
	struct bt_aics_gain_control cp = {
		.cp.opcode = BT_AICS_OPCODE_SET_GAIN,
		.gain_setting = gain,
	};
	struct bt_aics_state state;
	ssize_t ret;

	ret = state_attr->read(NULL, state_attr, &state, sizeof(state), 0U);
	zassert_equal(ret, sizeof(state));

	cp.cp.counter = state.change_counter;

	return cp_attr->write(NULL, cp_attr, &cp, sizeof(cp), 0U, 0U);
}

static ssize_t remote_description_write(const char *description)
{
	const struct bt_gatt_attr *attr = aics_attr_get(BT_UUID_AICS_DESCRIPTION);

	return attr->write(NULL, attr, description, strlen(description), 0U, 0U);
}

static void local_gain_set_thread(void *p1, void *p2, void *p3)
{
	zassert_equal(bt_aics_gain_set(aics, (int8_t)POINTER_TO_INT(p1)), 0);
}

static void local_description_set_thread(void *p1, void *p2, void *p3)
{
	zassert_equal(bt_aics_description_set(aics, p1), 0);
}

static void remote_gain_write_thread(void *p1, void *p2, void *p3)
{
	zassert_equal(remote_gain_write((int8_t)POINTER_TO_INT(p1)),
		      sizeof(struct bt_aics_gain_control));
}

static k_tid_t helper_start(k_thread_entry_t entry, void *p1)
{
	return k_thread_create(&helper_thread, helper_stack, K_THREAD_STACK_SIZEOF(helper_stack),
			       entry, p1, NULL, NULL, K_PRIO_PREEMPT(1), 0, K_NO_WAIT);
}

static void helper_wait_for_cb(void)
{
	zassert_equal(k_sem_take(&cb_entered, K_SECONDS(1)), 0);
}

static void helper_finish(k_tid_t tid)
{
	k_sem_give(&cb_continue);
	zassert_equal(k_thread_join(tid, K_SECONDS(1)), 0);
}

static void *aics_test_suite_setup(void)
{
	struct bt_aics_register_param param = {
		.gain = 0,
		.mute = BT_AICS_STATE_UNMUTED,
		.gain_mode = BT_AICS_MODE_MANUAL,
		.units = 1U,
		.min_gain = -100,
		.max_gain = 100,
		.type = BT_AICS_INPUT_TYPE_ANALOG,
		.status = true,
		.desc_writable = true,
		.description = "Input",
		.cb = &aics_cb,
	};

	mock_bt_gatt_init();

	aics = bt_aics_free_instance_get();
	zassert_not_null(aics);
	zassert_equal(bt_aics_register(aics, &param), 0);

	return NULL;
}

static void aics_test_before(void *f)
{
	ARG_UNUSED(f);

	mock_bt_gatt_init();

	block_first_cb = false;
	nested_call = false;
	nested_err = 0;
	k_sem_reset(&cb_entered);
	k_sem_reset(&cb_continue);

	zassert_equal(bt_aics_gain_set(aics, 0), 0);
	(void)memset(events, 0, sizeof(events));
	event_cnt = 0U;
}

static void aics_test_after(void *f)
{
	ARG_UNUSED(f);

	mock_bt_gatt_cleanup();
}

ZTEST_SUITE(aics_test_suite, NULL, aics_test_suite_setup, aics_test_before, aics_test_after, NULL);

static ZTEST(aics_test_suite, test_local_call_from_callback)
{
	nested_call = true;

	zassert_equal(bt_aics_gain_set(aics, 10), 0);
	zassert_equal(nested_err, -EBUSY);
	zassert_equal(event_cnt, 1U);
	zassert_equal(events[0].gain, 10);

	/* Not busy once the callback has returned */
	zassert_equal(bt_aics_gain_set(aics, 11), 0);
	zassert_equal(event_cnt, 2U);
	zassert_equal(events[1].gain, 11);
}

static ZTEST(aics_test_suite, test_remote_write_while_local_cb_pending)
{
	k_tid_t tid;

	block_first_cb = true;
	tid = helper_start(local_gain_set_thread, INT_TO_POINTER(20));
	helper_wait_for_cb();

	/* The remote write is applied but its callback is deferred to the local thread */
	zassert_equal(remote_gain_write(21), sizeof(struct bt_aics_gain_control));
	zassert_equal(event_cnt, 1U);

	zassert_equal(bt_aics_gain_set(aics, 22), -EBUSY);
	zassert_equal(bt_aics_state_get(aics), -EBUSY);

	helper_finish(tid);

	zassert_equal(event_cnt, 2U);
	zassert_equal(events[0].gain, 20);
	zassert_equal(events[0].thread, tid);
	zassert_equal(events[1].gain, 21);
	zassert_equal(events[1].thread, tid);

	zassert_equal(bt_aics_gain_set(aics, 23), 0);
	zassert_equal(event_cnt, 3U);
	zassert_equal(events[2].gain, 23);
}

static ZTEST(aics_test_suite, test_remote_writes_merged_while_cb_pending)
{
	k_tid_t tid;

	block_first_cb = true;
	tid = helper_start(local_gain_set_thread, INT_TO_POINTER(30));
	helper_wait_for_cb();

	zassert_equal(remote_gain_write(31), sizeof(struct bt_aics_gain_control));
	zassert_equal(remote_gain_write(32), sizeof(struct bt_aics_gain_control));

	helper_finish(tid);

	zassert_equal(event_cnt, 2U);
	zassert_equal(events[0].gain, 30);
	zassert_equal(events[1].gain, 32);
}

static ZTEST(aics_test_suite, test_local_call_while_remote_cb_pending)
{
	k_tid_t tid;

	block_first_cb = true;
	tid = helper_start(remote_gain_write_thread, INT_TO_POINTER(40));
	helper_wait_for_cb();

	zassert_equal(bt_aics_gain_set(aics, 41), -EBUSY);
	zassert_equal(bt_aics_mute(aics), -EBUSY);
	zassert_equal(bt_aics_description_set(aics, "Busy"), -EBUSY);
	zassert_equal(bt_aics_description_get(aics), -EBUSY);
	zassert_equal(bt_aics_deactivate(aics), -EBUSY);

	helper_finish(tid);

	zassert_equal(event_cnt, 1U);
	zassert_equal(events[0].gain, 40);

	zassert_equal(bt_aics_gain_set(aics, 42), 0);
	zassert_equal(event_cnt, 2U);
	zassert_equal(events[1].gain, 42);
}

static ZTEST(aics_test_suite, test_remote_description_write_while_local_cb_pending)
{
	k_tid_t tid;

	block_first_cb = true;
	tid = helper_start(local_description_set_thread, "Local");
	helper_wait_for_cb();

	zassert_equal(remote_description_write("Remote"), strlen("Remote"));
	zassert_equal(event_cnt, 1U);

	helper_finish(tid);

	zassert_equal(event_cnt, 2U);
	zassert_equal(events[0].type, CB_DESCRIPTION);
	zassert_str_equal(events[0].description, "Local");
	zassert_equal(events[1].type, CB_DESCRIPTION);
	zassert_str_equal(events[1].description, "Remote");
	zassert_equal(events[1].thread, tid);
}
