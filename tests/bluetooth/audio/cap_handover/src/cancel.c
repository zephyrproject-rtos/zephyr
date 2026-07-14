/* cancel.c - unit test for bt_cap_handover_cancel */

/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include <zephyr/autoconf.h>
#include <zephyr/bluetooth/addr.h>
#include <zephyr/bluetooth/audio/bap.h>
#include <zephyr/bluetooth/audio/cap.h>
#include <zephyr/bluetooth/hci_types.h>
#include <zephyr/fff.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/util_macro.h>
#include <zephyr/ztest_assert.h>
#include <zephyr/ztest_test.h>

#include "audio/cap_internal.h"
#include "cap_handover.h"
#include "expects_util.h"
#include "test_common.h"

#define FFF_GLOBALS

struct cap_handover_test_cancel_fixture {
	struct bt_cap_common_proc *active_proc;
};

static void *cap_handover_test_cancel_setup(void)
{
	struct cap_handover_test_cancel_fixture *fixture;

	fixture = malloc(sizeof(*fixture));
	zassert_not_null(fixture);

	return fixture;
}

static void cap_handover_test_cancel_before(void *f)
{
	struct cap_handover_test_cancel_fixture *fixture = f;
	int err;

	(void)memset(f, 0, sizeof(struct cap_handover_test_cancel_fixture));

	fixture->active_proc = bt_cap_common_get_active_proc();

	atomic_set_bit(fixture->active_proc->proc_state_flags, BT_CAP_COMMON_PROC_STATE_ACTIVE);
	atomic_set_bit(fixture->active_proc->proc_state_flags, BT_CAP_COMMON_PROC_STATE_HANDOVER);

	bt_cap_common_unlock_proc();

	err = bt_cap_handover_register_cb(&mock_cap_handover_cb);
	zassert_equal(0, err, "Unexpected return value %d", err);
}

static void cap_handover_test_cancel_after(void *f)
{
	struct cap_handover_test_cancel_fixture *fixture = f;
	int err;

	(void)memset(fixture->active_proc, 0, sizeof(*fixture->active_proc));

	err = bt_cap_handover_unregister_cb(&mock_cap_handover_cb);
	zassert_true(err == 0, "Unexpected error: %d", err);
}

static void cap_handover_test_cancel_teardown(void *f)
{
	free(f);
}

static void test_cancel(struct cap_handover_test_cancel_fixture *fixture)
{
	int err;

	err = bt_cap_handover_cancel();
	zassert_equal(0, err, "Unexpected return value %d", err);

	if (fixture->active_proc->proc_param.handover.is_unicast_to_broadcast) {
		zexpect_call_count("bt_cap_handover_cb.unicast_to_broadcast_complete", 1,
				   mock_unicast_to_broadcast_complete_cb_fake.call_count);
		zassert_equal(-ECANCELED,
			      mock_unicast_to_broadcast_complete_cb_fake.arg0_history[0]);
	} else {
		zexpect_call_count("bt_cap_handover_cb.broadcast_to_unicast_complete", 1,
				   mock_broadcast_to_unicast_complete_cb_fake.call_count);
		zassert_equal(-ECANCELED,
			      mock_broadcast_to_unicast_complete_cb_fake.arg0_history[0]);
	}
}

ZTEST_SUITE(cap_handover_test_cancel, NULL, cap_handover_test_cancel_setup,
	    cap_handover_test_cancel_before, cap_handover_test_cancel_after,
	    cap_handover_test_cancel_teardown);

static ZTEST_F(cap_handover_test_cancel, test_handover_cancel)
{
	test_cancel(fixture);
}

static ZTEST_F(cap_handover_test_cancel, test_handover_cancel_double)
{
	int err;

	test_cancel(fixture);

	err = bt_cap_handover_cancel();
	zassert_equal(-EALREADY, err, "Unexpected return value %d", err);
}

static ZTEST_F(cap_handover_test_cancel, test_handover_cancel_not_active)
{
	int err;

	atomic_clear_bit(fixture->active_proc->proc_state_flags, BT_CAP_COMMON_PROC_STATE_ACTIVE);

	err = bt_cap_handover_cancel();
	zassert_equal(-EALREADY, err, "Unexpected return value %d", err);
}

static ZTEST_F(cap_handover_test_cancel, test_handover_cancel_not_handover)
{
	int err;

	atomic_clear_bit(fixture->active_proc->proc_state_flags, BT_CAP_COMMON_PROC_STATE_HANDOVER);

	err = bt_cap_handover_cancel();
	zassert_equal(-EOPNOTSUPP, err, "Unexpected return value %d", err);
}
