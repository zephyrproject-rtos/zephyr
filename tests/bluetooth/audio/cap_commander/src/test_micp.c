/* test_micp.c - unit test for microphone settings */

/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdlib.h>

#include <zephyr/bluetooth/audio/cap.h>
#include <zephyr/bluetooth/audio/micp.h>
#include <zephyr/fff.h>

#include "bluetooth.h"
#include "cap_commander.h"
#include "conn.h"
#include "expects_util.h"
#include "cap_mocks.h"
#include "test_common.h"

#define FFF_GLOBALS

struct cap_commander_test_micp_fixture {
	struct bt_conn conns[CONFIG_BT_MAX_CONN];
};

static void cap_commander_test_micp_fixture_init(struct cap_commander_test_micp_fixture *fixture)
{
	for (size_t i = 0; i < ARRAY_SIZE(fixture->conns); i++) {
		test_conn_init(&fixture->conns[i]);
	}
}

static void *cap_commander_test_micp_setup(void)
{
	struct cap_commander_test_micp_fixture *fixture;

	fixture = malloc(sizeof(*fixture));
	zassert_not_null(fixture);

	return fixture;
}

static void cap_commander_test_micp_before(void *f)
{
	memset(f, 0, sizeof(struct cap_commander_test_micp_fixture));
	cap_commander_test_micp_fixture_init(f);
}

static void cap_commander_test_micp_after(void *f)
{
	struct cap_commander_test_micp_fixture *fixture = f;

	bt_cap_commander_unregister_cb(&mock_cap_commander_cb);

	for (size_t i = 0; i < ARRAY_SIZE(fixture->conns); i++) {
		mock_bt_conn_disconnected(&fixture->conns[i], BT_HCI_ERR_REMOTE_USER_TERM_CONN);
	}
}

static void cap_commander_test_micp_teardown(void *f)
{
	free(f);
}

ZTEST_SUITE(cap_commander_test_micp, NULL, cap_commander_test_micp_setup,
	    cap_commander_test_micp_before, cap_commander_test_micp_after,
	    cap_commander_test_micp_teardown);


ZTEST_F(cap_commander_test_micp, test_commander_change_microphone_gain_setting)
{
	struct bt_cap_commander_change_microphone_gain_setting_member_param
		member_params[ARRAY_SIZE(fixture->conns)];
	const struct bt_cap_commander_change_microphone_gain_setting_param param = {
		.type = BT_CAP_SET_TYPE_AD_HOC,
		.param = member_params,
		.count = ARRAY_SIZE(member_params),
	};
	int err;

	for (size_t i = 0U; i < ARRAY_SIZE(member_params); i++) {
		member_params[i].member.member = &fixture->conns[i];
		member_params[i].gain = 10 + i;
	}

	err = bt_cap_commander_register_cb(&mock_cap_commander_cb);
	zassert_equal(0, err, "Unexpected return value %d", err);

	for (size_t i = 0; i < ARRAY_SIZE(fixture->conns); i++) {
		struct bt_micp_mic_ctlr *mic_ctlr; /* We don't care about this */

		err = bt_micp_mic_ctlr_discover(&fixture->conns[i], &mic_ctlr);
		zassert_equal(0, err, "Unexpected return value %d", err);
	}

	err = bt_cap_commander_change_microphone_gain_setting(&param);
	zassert_equal(0, err, "Unexpected return value %d", err);

	zexpect_call_count("bt_cap_commander_cb.microphone_gain_setting_changed", 1,
			   mock_cap_commander_microphone_gain_changed_cb_fake.call_count);
}

ZTEST_F(cap_commander_test_micp, test_commander_change_microphone_gain_setting_double)
{
	struct bt_cap_commander_change_microphone_gain_setting_member_param
		member_params[ARRAY_SIZE(fixture->conns)];
	const struct bt_cap_commander_change_microphone_gain_setting_param param = {
		.type = BT_CAP_SET_TYPE_AD_HOC,
		.param = member_params,
		.count = ARRAY_SIZE(member_params),
	};
	int err;

	for (size_t i = 0U; i < ARRAY_SIZE(member_params); i++) {
		member_params[i].member.member = &fixture->conns[i];
		member_params[i].gain = 10 + i;
	}

	err = bt_cap_commander_register_cb(&mock_cap_commander_cb);
	zassert_equal(0, err, "Unexpected return value %d", err);

	for (size_t i = 0; i < ARRAY_SIZE(fixture->conns); i++) {
		struct bt_micp_mic_ctlr *mic_ctlr; /* We don't care about this */

		err = bt_micp_mic_ctlr_discover(&fixture->conns[i], &mic_ctlr);
		zassert_equal(0, err, "Unexpected return value %d", err);
	}

	err = bt_cap_commander_change_microphone_gain_setting(&param);
	zassert_equal(0, err, "Unexpected return value %d", err);

	zexpect_call_count("bt_cap_commander_cb.microphone_gain_setting_changed", 1,
			   mock_cap_commander_microphone_gain_changed_cb_fake.call_count);

	/* Test that it still works as expected if we set the same value twice */
	err = bt_cap_commander_change_microphone_gain_setting(&param);
	zassert_equal(0, err, "Unexpected return value %d", err);

	zexpect_call_count("bt_cap_commander_cb.microphone_gain_setting_changed", 2,
			   mock_cap_commander_microphone_gain_changed_cb_fake.call_count);
}

ZTEST_F(cap_commander_test_micp, test_commander_change_microphone_gain_setting_inval_param_null)
{
	int err;

	err = bt_cap_commander_change_microphone_gain_setting(NULL);
	zassert_equal(-EINVAL, err, "Unexpected return value %d", err);
}

ZTEST_F(cap_commander_test_micp,
	test_commander_change_microphone_gain_setting_inval_param_null_param)
{
	const struct bt_cap_commander_change_microphone_gain_setting_param param = {
		.type = BT_CAP_SET_TYPE_AD_HOC,
		.param = NULL,
		.count = ARRAY_SIZE(fixture->conns),
	};
	int err;

	err = bt_cap_commander_change_microphone_gain_setting(&param);
	zassert_equal(-EINVAL, err, "Unexpected return value %d", err);
}

ZTEST_F(cap_commander_test_micp,
	test_commander_change_microphone_gain_setting_inval_param_null_member)
{
	struct bt_cap_commander_change_microphone_gain_setting_member_param
		member_params[ARRAY_SIZE(fixture->conns)];
	const struct bt_cap_commander_change_microphone_gain_setting_param param = {
		.type = BT_CAP_SET_TYPE_AD_HOC,
		.param = member_params,
		.count = ARRAY_SIZE(member_params),
	};
	int err;

	for (size_t i = 0U; i < ARRAY_SIZE(member_params); i++) {
		member_params[i].member.member = &fixture->conns[i];
		member_params[i].gain = 10 + i;
	}
	member_params[ARRAY_SIZE(member_params) - 1].member.member = NULL;

	err = bt_cap_commander_change_microphone_gain_setting(&param);
	zassert_equal(-EINVAL, err, "Unexpected return value %d", err);
}

ZTEST_F(cap_commander_test_micp, test_commander_change_microphone_gain_setting_inval_missing_cas)
{
	struct bt_cap_commander_change_microphone_gain_setting_member_param
		member_params[ARRAY_SIZE(fixture->conns)];
	const struct bt_cap_commander_change_microphone_gain_setting_param param = {
		.type = BT_CAP_SET_TYPE_CSIP,
		.param = member_params,
		.count = ARRAY_SIZE(member_params),
	};
	int err;

	for (size_t i = 0U; i < ARRAY_SIZE(member_params); i++) {
		member_params[i].member.member = &fixture->conns[i];
		member_params[i].gain = 10 + i;
	}

	err = bt_cap_commander_register_cb(&mock_cap_commander_cb);
	zassert_equal(0, err, "Unexpected return value %d", err);

	for (size_t i = 0; i < ARRAY_SIZE(fixture->conns); i++) {
		struct bt_micp_mic_ctlr *mic_ctlr; /* We don't care about this */

		err = bt_micp_mic_ctlr_discover(&fixture->conns[i], &mic_ctlr);
		zassert_equal(0, err, "Unexpected return value %d", err);
	}

	err = bt_cap_commander_change_microphone_gain_setting(&param);
	zassert_equal(-EINVAL, err, "Unexpected return value %d", err);
}

ZTEST_F(cap_commander_test_micp, test_commander_change_microphone_gain_setting_inval_missing_aics)
{
	struct bt_cap_commander_change_microphone_gain_setting_member_param
		member_params[ARRAY_SIZE(fixture->conns)];
	const struct bt_cap_commander_change_microphone_gain_setting_param param = {
		.type = BT_CAP_SET_TYPE_AD_HOC,
		.param = member_params,
		.count = ARRAY_SIZE(member_params),
	};
	int err;

	for (size_t i = 0U; i < ARRAY_SIZE(member_params); i++) {
		member_params[i].member.member = &fixture->conns[i];
		member_params[i].gain = 10 + i;
	}

	err = bt_cap_commander_register_cb(&mock_cap_commander_cb);
	zassert_equal(0, err, "Unexpected return value %d", err);

	err = bt_cap_commander_change_microphone_gain_setting(&param);
	zassert_equal(-EINVAL, err, "Unexpected return value %d", err);
}

ZTEST_F(cap_commander_test_micp,
	test_commander_change_microphone_gain_setting_inval_param_zero_count)
{
	struct bt_cap_commander_change_microphone_gain_setting_member_param
		member_params[ARRAY_SIZE(fixture->conns)];
	const struct bt_cap_commander_change_microphone_gain_setting_param param = {
		.type = BT_CAP_SET_TYPE_AD_HOC,
		.param = member_params,
		.count = 0U,
	};
	int err;

	for (size_t i = 0U; i < ARRAY_SIZE(member_params); i++) {
		member_params[i].member.member = &fixture->conns[i];
		member_params[i].gain = 10 + i;
	}

	err = bt_cap_commander_change_microphone_gain_setting(&param);
	zassert_equal(-EINVAL, err, "Unexpected return value %d", err);
}

ZTEST_F(cap_commander_test_micp,
	test_commander_change_microphone_gain_setting_inval_param_inval_count)
{
	struct bt_cap_commander_change_microphone_gain_setting_member_param
		member_params[ARRAY_SIZE(fixture->conns)];
	const struct bt_cap_commander_change_microphone_gain_setting_param param = {
		.type = BT_CAP_SET_TYPE_AD_HOC,
		.param = member_params,
		.count = CONFIG_BT_MAX_CONN + 1,
	};
	int err;

	for (size_t i = 0U; i < ARRAY_SIZE(member_params); i++) {
		member_params[i].member.member = &fixture->conns[i];
		member_params[i].gain = 10 + i;
	}

	err = bt_cap_commander_change_microphone_gain_setting(&param);
	zassert_equal(-EINVAL, err, "Unexpected return value %d", err);
}

ZTEST_F(cap_commander_test_micp, test_commander_change_microphone_mute_state)
{
	union bt_cap_set_member members[ARRAY_SIZE(fixture->conns)];
	const struct bt_cap_commander_change_microphone_mute_state_param param = {
		.type = BT_CAP_SET_TYPE_AD_HOC,
		.members = members,
		.count = ARRAY_SIZE(fixture->conns),
		.mute = true,
	};
	int err;

	for (size_t i = 0U; i < ARRAY_SIZE(members); i++) {
		members[i].member = &fixture->conns[i];
	}

	err = bt_cap_commander_register_cb(&mock_cap_commander_cb);
	zassert_equal(0, err, "Unexpected return value %d", err);

	for (size_t i = 0; i < ARRAY_SIZE(fixture->conns); i++) {
		struct bt_micp_mic_ctlr *mic_ctlr; /* We don't care about this */

		err = bt_micp_mic_ctlr_discover(&fixture->conns[i], &mic_ctlr);
		zassert_equal(0, err, "Unexpected return value %d", err);
	}

	err = bt_cap_commander_change_microphone_mute_state(&param);
	zassert_equal(0, err, "Unexpected return value %d", err);

	zexpect_call_count("bt_cap_commander_cb.microphone_mute_changed", 1,
			   mock_cap_commander_microphone_mute_changed_cb_fake.call_count);
}

ZTEST_F(cap_commander_test_micp, test_commander_change_microphone_mute_state_double)
{
	union bt_cap_set_member members[ARRAY_SIZE(fixture->conns)];
	const struct bt_cap_commander_change_microphone_mute_state_param param = {
		.type = BT_CAP_SET_TYPE_AD_HOC,
		.members = members,
		.count = ARRAY_SIZE(fixture->conns),
		.mute = true,
	};
	int err;

	for (size_t i = 0U; i < ARRAY_SIZE(members); i++) {
		members[i].member = &fixture->conns[i];
	}

	err = bt_cap_commander_register_cb(&mock_cap_commander_cb);
	zassert_equal(0, err, "Unexpected return value %d", err);

	for (size_t i = 0; i < ARRAY_SIZE(fixture->conns); i++) {
		struct bt_micp_mic_ctlr *mic_ctlr; /* We don't care about this */

		err = bt_micp_mic_ctlr_discover(&fixture->conns[i], &mic_ctlr);
		zassert_equal(0, err, "Unexpected return value %d", err);
	}

	err = bt_cap_commander_change_microphone_mute_state(&param);
	zassert_equal(0, err, "Unexpected return value %d", err);

	zexpect_call_count("bt_cap_commander_cb.microphone_mute_changed", 1,
			   mock_cap_commander_microphone_mute_changed_cb_fake.call_count);

	/* Test that it still works as expected if we set the same value twice */
	err = bt_cap_commander_change_microphone_mute_state(&param);
	zassert_equal(0, err, "Unexpected return value %d", err);

	zexpect_call_count("bt_cap_commander_cb.microphone_mute_changed", 2,
			   mock_cap_commander_microphone_mute_changed_cb_fake.call_count);
}

ZTEST_F(cap_commander_test_micp, test_commander_change_microphone_mute_state_inval_param_null)
{
	int err;

	err = bt_cap_commander_change_microphone_mute_state(NULL);
	zassert_equal(-EINVAL, err, "Unexpected return value %d", err);
}

ZTEST_F(cap_commander_test_micp,
	test_commander_change_microphone_mute_state_inval_param_null_members)
{
	const struct bt_cap_commander_change_microphone_mute_state_param param = {
		.type = BT_CAP_SET_TYPE_AD_HOC,
		.members = NULL,
		.count = ARRAY_SIZE(fixture->conns),
		.mute = true,
	};
	int err;

	err = bt_cap_commander_change_microphone_mute_state(&param);
	zassert_equal(-EINVAL, err, "Unexpected return value %d", err);
}

ZTEST_F(cap_commander_test_micp,
	test_commander_change_microphone_mute_state_inval_param_null_member)
{
	union bt_cap_set_member members[ARRAY_SIZE(fixture->conns)];
	const struct bt_cap_commander_change_microphone_mute_state_param param = {
		.type = BT_CAP_SET_TYPE_AD_HOC,
		.members = members,
		.count = ARRAY_SIZE(fixture->conns),
		.mute = true,
	};
	int err;

	for (size_t i = 0U; i < ARRAY_SIZE(members) - 1; i++) {
		members[i].member = &fixture->conns[i];
	}
	members[ARRAY_SIZE(members) - 1].member = NULL;

	err = bt_cap_commander_change_microphone_mute_state(&param);
	zassert_equal(-EINVAL, err, "Unexpected return value %d", err);
}

ZTEST_F(cap_commander_test_micp, test_commander_change_microphone_mute_state_inval_missing_cas)
{
	union bt_cap_set_member members[ARRAY_SIZE(fixture->conns)];
	const struct bt_cap_commander_change_microphone_mute_state_param param = {
		.type = BT_CAP_SET_TYPE_CSIP,
		.members = members,
		.count = ARRAY_SIZE(fixture->conns),
		.mute = true,
	};
	int err;

	for (size_t i = 0U; i < ARRAY_SIZE(members); i++) {
		members[i].member = &fixture->conns[i];
	}

	err = bt_cap_commander_register_cb(&mock_cap_commander_cb);
	zassert_equal(0, err, "Unexpected return value %d", err);

	for (size_t i = 0; i < ARRAY_SIZE(fixture->conns); i++) {
		struct bt_micp_mic_ctlr *mic_ctlr; /* We don't care about this */

		err = bt_micp_mic_ctlr_discover(&fixture->conns[i], &mic_ctlr);
		zassert_equal(0, err, "Unexpected return value %d", err);
	}

	err = bt_cap_commander_change_microphone_mute_state(&param);
	zassert_equal(-EINVAL, err, "Unexpected return value %d", err);
}

ZTEST_F(cap_commander_test_micp, test_commander_change_microphone_mute_state_inval_missing_vcs)
{
	union bt_cap_set_member members[ARRAY_SIZE(fixture->conns)];
	const struct bt_cap_commander_change_microphone_mute_state_param param = {
		.type = BT_CAP_SET_TYPE_AD_HOC,
		.members = members,
		.count = ARRAY_SIZE(fixture->conns),
		.mute = true,
	};
	int err;

	for (size_t i = 0U; i < ARRAY_SIZE(members); i++) {
		members[i].member = &fixture->conns[i];
	}

	err = bt_cap_commander_register_cb(&mock_cap_commander_cb);
	zassert_equal(0, err, "Unexpected return value %d", err);

	err = bt_cap_commander_change_microphone_mute_state(&param);
	zassert_equal(-EINVAL, err, "Unexpected return value %d", err);
}

ZTEST_F(cap_commander_test_micp, test_commander_change_microphone_mute_state_inval_param_zero_count)
{
	union bt_cap_set_member members[ARRAY_SIZE(fixture->conns)];
	const struct bt_cap_commander_change_microphone_mute_state_param param = {
		.type = BT_CAP_SET_TYPE_AD_HOC,
		.members = members,
		.count = 0U,
		.mute = true,
	};
	int err;

	for (size_t i = 0U; i < ARRAY_SIZE(members); i++) {
		members[i].member = &fixture->conns[i];
	}

	err = bt_cap_commander_change_microphone_mute_state(&param);
	zassert_equal(-EINVAL, err, "Unexpected return value %d", err);
}

ZTEST_F(cap_commander_test_micp,
	test_commander_change_microphone_mute_state_inval_param_inval_count)
{
	union bt_cap_set_member members[ARRAY_SIZE(fixture->conns)];
	const struct bt_cap_commander_change_microphone_mute_state_param param = {
		.type = BT_CAP_SET_TYPE_AD_HOC,
		.members = members,
		.count = CONFIG_BT_MAX_CONN + 1,
		.mute = true,
	};
	int err;

	for (size_t i = 0U; i < ARRAY_SIZE(members); i++) {
		members[i].member = &fixture->conns[i];
	}

	err = bt_cap_commander_change_microphone_mute_state(&param);
	zassert_equal(-EINVAL, err, "Unexpected return value %d", err);
}
