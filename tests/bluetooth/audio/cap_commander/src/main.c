/* main.c - Application main entry point */

/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdlib.h>

#include <zephyr/bluetooth/audio/cap.h>
#include <zephyr/bluetooth/audio/vcp.h>
#include <zephyr/fff.h>

#include "bluetooth.h"
#include "cap_commander.h"
#include "conn.h"
#include "expects_util.h"
#include "cap_mocks.h"

DEFINE_FFF_GLOBALS;

static void mock_init_rule_before(const struct ztest_unit_test *test, void *fixture)
{
	mock_cap_commander_init();
	mock_bt_csip_init();
	mock_bt_vcp_init();
	mock_bt_vocs_init();
}

static void mock_destroy_rule_after(const struct ztest_unit_test *test, void *fixture)
{
	mock_cap_commander_cleanup();
	mock_bt_csip_cleanup();
	mock_bt_vcp_cleanup();
	mock_bt_vocs_cleanup();
}

ZTEST_RULE(mock_rule, mock_init_rule_before, mock_destroy_rule_after);

struct cap_commander_test_suite_fixture {
	struct bt_conn conns[CONFIG_BT_MAX_CONN];
};

static void test_conn_init(struct bt_conn *conn)
{
	conn->index = 0;
	conn->info.type = BT_CONN_TYPE_LE;
	conn->info.role = BT_CONN_ROLE_PERIPHERAL;
	conn->info.state = BT_CONN_STATE_CONNECTED;
	conn->info.security.level = BT_SECURITY_L2;
	conn->info.security.enc_key_size = BT_ENC_KEY_SIZE_MAX;
	conn->info.security.flags = BT_SECURITY_FLAG_OOB | BT_SECURITY_FLAG_SC;
}

static void cap_commander_test_suite_fixture_init(struct cap_commander_test_suite_fixture *fixture)
{
	for (size_t i = 0; i < ARRAY_SIZE(fixture->conns); i++) {
		test_conn_init(&fixture->conns[i]);
	}
}

static void *cap_commander_test_suite_setup(void)
{
	struct cap_commander_test_suite_fixture *fixture;

	fixture = malloc(sizeof(*fixture));
	zassert_not_null(fixture);

	return fixture;
}

static void cap_commander_test_suite_before(void *f)
{
	memset(f, 0, sizeof(struct cap_commander_test_suite_fixture));
	cap_commander_test_suite_fixture_init(f);
}

static void cap_commander_test_suite_after(void *f)
{
	struct cap_commander_test_suite_fixture *fixture = f;

	bt_cap_commander_unregister_cb(&mock_cap_commander_cb);

	for (size_t i = 0; i < ARRAY_SIZE(fixture->conns); i++) {
		mock_bt_conn_disconnected(&fixture->conns[i], BT_HCI_ERR_REMOTE_USER_TERM_CONN);
	}
}

static void cap_commander_test_suite_teardown(void *f)
{
	free(f);
}

ZTEST_SUITE(cap_commander_test_suite, NULL, cap_commander_test_suite_setup,
	    cap_commander_test_suite_before, cap_commander_test_suite_after,
	    cap_commander_test_suite_teardown);

ZTEST_F(cap_commander_test_suite, test_commander_register_cb)
{
	int err;

	err = bt_cap_commander_register_cb(&mock_cap_commander_cb);
	zassert_equal(0, err, "Unexpected return value %d", err);
}

ZTEST_F(cap_commander_test_suite, test_commander_register_cb_inval_param_null)
{
	int err;

	err = bt_cap_commander_register_cb(NULL);
	zassert_equal(-EINVAL, err, "Unexpected return value %d", err);
}

ZTEST_F(cap_commander_test_suite, test_commander_register_cb_inval_double_register)
{
	int err;

	err = bt_cap_commander_register_cb(&mock_cap_commander_cb);
	zassert_equal(0, err, "Unexpected return value %d", err);

	err = bt_cap_commander_register_cb(&mock_cap_commander_cb);
	zassert_equal(-EALREADY, err, "Unexpected return value %d", err);
}

ZTEST_F(cap_commander_test_suite, test_commander_unregister_cb)
{
	int err;

	err = bt_cap_commander_register_cb(&mock_cap_commander_cb);
	zassert_equal(0, err, "Unexpected return value %d", err);

	err = bt_cap_commander_unregister_cb(&mock_cap_commander_cb);
	zassert_equal(0, err, "Unexpected return value %d", err);
}

ZTEST_F(cap_commander_test_suite, test_commander_unregister_cb_inval_param_null)
{
	int err;

	err = bt_cap_commander_unregister_cb(NULL);
	zassert_equal(-EINVAL, err, "Unexpected return value %d", err);
}

ZTEST_F(cap_commander_test_suite, test_commander_unregister_cb_inval_double_unregister)
{
	int err;

	err = bt_cap_commander_register_cb(&mock_cap_commander_cb);
	zassert_equal(0, err, "Unexpected return value %d", err);

	err = bt_cap_commander_unregister_cb(&mock_cap_commander_cb);
	zassert_equal(0, err, "Unexpected return value %d", err);

	err = bt_cap_commander_unregister_cb(&mock_cap_commander_cb);
	zassert_equal(-EINVAL, err, "Unexpected return value %d", err);
}

ZTEST_F(cap_commander_test_suite, test_commander_discover)
{
	int err;

	err = bt_cap_commander_register_cb(&mock_cap_commander_cb);
	zassert_equal(0, err, "Unexpected return value %d", err);

	for (size_t i = 0; i < ARRAY_SIZE(fixture->conns); i++) {
		err = bt_cap_commander_discover(&fixture->conns[i]);
		zassert_equal(0, err, "Unexpected return value %d", err);
	}

	zexpect_call_count("bt_cap_commander_cb.discovery_complete", ARRAY_SIZE(fixture->conns),
			   mock_cap_commander_discovery_complete_cb_fake.call_count);
}

ZTEST_F(cap_commander_test_suite, test_commander_discover_inval_param_null)
{
	int err;

	err = bt_cap_commander_register_cb(&mock_cap_commander_cb);
	zassert_equal(0, err, "Unexpected return value %d", err);

	err = bt_cap_commander_discover(NULL);
	zassert_equal(-EINVAL, err, "Unexpected return value %d", err);
}

ZTEST_F(cap_commander_test_suite, test_commander_change_volume)
{
	union bt_cap_set_member members[ARRAY_SIZE(fixture->conns)];
	const struct bt_cap_commander_change_volume_param param = {
		.type = BT_CAP_SET_TYPE_AD_HOC,
		.members = members,
		.count = ARRAY_SIZE(fixture->conns),
		.volume = 177,
	};
	int err;

	for (size_t i = 0U; i < ARRAY_SIZE(members); i++) {
		members[i].member = &fixture->conns[i];
	}

	err = bt_cap_commander_register_cb(&mock_cap_commander_cb);
	zassert_equal(0, err, "Unexpected return value %d", err);

	for (size_t i = 0; i < ARRAY_SIZE(fixture->conns); i++) {
		struct bt_vcp_vol_ctlr *vol_ctlr; /* We don't care about this */

		err = bt_cap_commander_discover(&fixture->conns[i]);
		zassert_equal(0, err, "Unexpected return value %d", err);

		err = bt_vcp_vol_ctlr_discover(&fixture->conns[i], &vol_ctlr);
		zassert_equal(0, err, "Unexpected return value %d", err);
	}

	err = bt_cap_commander_change_volume(&param);
	zassert_equal(0, err, "Unexpected return value %d", err);

	zexpect_call_count("bt_cap_commander_cb.volume_changed", 1,
			   mock_cap_commander_volume_changed_cb_fake.call_count);
}

ZTEST_F(cap_commander_test_suite, test_commander_change_volume_double)
{
	union bt_cap_set_member members[ARRAY_SIZE(fixture->conns)];
	const struct bt_cap_commander_change_volume_param param = {
		.type = BT_CAP_SET_TYPE_AD_HOC,
		.members = members,
		.count = ARRAY_SIZE(fixture->conns),
		.volume = 177,
	};
	int err;

	for (size_t i = 0U; i < ARRAY_SIZE(members); i++) {
		members[i].member = &fixture->conns[i];
	}

	err = bt_cap_commander_register_cb(&mock_cap_commander_cb);
	zassert_equal(0, err, "Unexpected return value %d", err);

	for (size_t i = 0; i < ARRAY_SIZE(fixture->conns); i++) {
		struct bt_vcp_vol_ctlr *vol_ctlr; /* We don't care about this */

		err = bt_cap_commander_discover(&fixture->conns[i]);
		zassert_equal(0, err, "Unexpected return value %d", err);

		err = bt_vcp_vol_ctlr_discover(&fixture->conns[i], &vol_ctlr);
		zassert_equal(0, err, "Unexpected return value %d", err);
	}

	err = bt_cap_commander_change_volume(&param);
	zassert_equal(0, err, "Unexpected return value %d", err);

	zexpect_call_count("bt_cap_commander_cb.volume_changed", 1,
			   mock_cap_commander_volume_changed_cb_fake.call_count);

	/* Verify that it still works as expected if we set the same value twice */
	err = bt_cap_commander_change_volume(&param);
	zassert_equal(0, err, "Unexpected return value %d", err);

	zexpect_call_count("bt_cap_commander_cb.volume_changed", 2,
			   mock_cap_commander_volume_changed_cb_fake.call_count);
}

ZTEST_F(cap_commander_test_suite, test_commander_change_volume_inval_param_null)
{
	int err;

	err = bt_cap_commander_change_volume(NULL);
	zassert_equal(-EINVAL, err, "Unexpected return value %d", err);
}

ZTEST_F(cap_commander_test_suite, test_commander_change_volume_inval_param_null_members)
{
	const struct bt_cap_commander_change_volume_param param = {
		.type = BT_CAP_SET_TYPE_AD_HOC,
		.members = NULL,
		.count = ARRAY_SIZE(fixture->conns),
		.volume = 177,
	};
	int err;

	err = bt_cap_commander_change_volume(&param);
	zassert_equal(-EINVAL, err, "Unexpected return value %d", err);
}

ZTEST_F(cap_commander_test_suite, test_commander_change_volume_inval_param_null_member)
{
	union bt_cap_set_member members[ARRAY_SIZE(fixture->conns)];
	const struct bt_cap_commander_change_volume_param param = {
		.type = BT_CAP_SET_TYPE_AD_HOC,
		.members = members,
		.count = ARRAY_SIZE(fixture->conns),
		.volume = 177,
	};
	int err;

	for (size_t i = 0U; i < ARRAY_SIZE(members) - 1; i++) {
		members[i].member = &fixture->conns[i];
	}
	members[ARRAY_SIZE(members) - 1].member = NULL;

	err = bt_cap_commander_change_volume(&param);
	zassert_equal(-EINVAL, err, "Unexpected return value %d", err);
}

ZTEST_F(cap_commander_test_suite, test_commander_change_volume_inval_missing_cas)
{
	union bt_cap_set_member members[ARRAY_SIZE(fixture->conns)];
	const struct bt_cap_commander_change_volume_param param = {
		.type = BT_CAP_SET_TYPE_AD_HOC,
		.members = members,
		.count = ARRAY_SIZE(fixture->conns),
		.volume = 177,
	};
	int err;

	for (size_t i = 0U; i < ARRAY_SIZE(members); i++) {
		members[i].member = &fixture->conns[i];
	}

	err = bt_cap_commander_register_cb(&mock_cap_commander_cb);
	zassert_equal(0, err, "Unexpected return value %d", err);

	for (size_t i = 0; i < ARRAY_SIZE(fixture->conns); i++) {
		struct bt_vcp_vol_ctlr *vol_ctlr; /* We don't care about this */

		err = bt_vcp_vol_ctlr_discover(&fixture->conns[i], &vol_ctlr);
		zassert_equal(0, err, "Unexpected return value %d", err);
	}

	err = bt_cap_commander_change_volume(&param);
	zassert_equal(-EINVAL, err, "Unexpected return value %d", err);
}

ZTEST_F(cap_commander_test_suite, test_commander_change_volume_inval_missing_vcs)
{
	union bt_cap_set_member members[ARRAY_SIZE(fixture->conns)];
	const struct bt_cap_commander_change_volume_param param = {
		.type = BT_CAP_SET_TYPE_AD_HOC,
		.members = members,
		.count = ARRAY_SIZE(fixture->conns),
		.volume = 177,
	};
	int err;

	for (size_t i = 0U; i < ARRAY_SIZE(members); i++) {
		members[i].member = &fixture->conns[i];
	}

	err = bt_cap_commander_register_cb(&mock_cap_commander_cb);
	zassert_equal(0, err, "Unexpected return value %d", err);

	for (size_t i = 0; i < ARRAY_SIZE(fixture->conns); i++) {
		err = bt_cap_commander_discover(&fixture->conns[i]);
		zassert_equal(0, err, "Unexpected return value %d", err);
	}

	err = bt_cap_commander_change_volume(&param);
	zassert_equal(-EINVAL, err, "Unexpected return value %d", err);
}

ZTEST_F(cap_commander_test_suite, test_commander_change_volume_inval_param_zero_count)
{
	union bt_cap_set_member members[ARRAY_SIZE(fixture->conns)];
	const struct bt_cap_commander_change_volume_param param = {
		.type = BT_CAP_SET_TYPE_AD_HOC,
		.members = members,
		.count = 0U,
		.volume = 177,
	};
	int err;

	for (size_t i = 0U; i < ARRAY_SIZE(members); i++) {
		members[i].member = &fixture->conns[i];
	}

	err = bt_cap_commander_change_volume(&param);
	zassert_equal(-EINVAL, err, "Unexpected return value %d", err);
}

ZTEST_F(cap_commander_test_suite, test_commander_change_volume_inval_param_inval_count)
{
	union bt_cap_set_member members[ARRAY_SIZE(fixture->conns)];
	const struct bt_cap_commander_change_volume_param param = {
		.type = BT_CAP_SET_TYPE_AD_HOC,
		.members = members,
		.count = CONFIG_BT_MAX_CONN + 1,
		.volume = 177,
	};
	int err;

	for (size_t i = 0U; i < ARRAY_SIZE(members); i++) {
		members[i].member = &fixture->conns[i];
	}

	err = bt_cap_commander_change_volume(&param);
	zassert_equal(-EINVAL, err, "Unexpected return value %d", err);
}

ZTEST_F(cap_commander_test_suite, test_commander_change_volume_offset)
{
	struct bt_cap_commander_change_volume_offset_member_param
		member_params[ARRAY_SIZE(fixture->conns)];
	const struct bt_cap_commander_change_volume_offset_param param = {
		.type = BT_CAP_SET_TYPE_AD_HOC,
		.param = member_params,
		.count = ARRAY_SIZE(member_params),
	};
	int err;

	for (size_t i = 0U; i < ARRAY_SIZE(member_params); i++) {
		member_params[i].member.member = &fixture->conns[i];
		member_params[i].offset = 100 + i;
	}

	err = bt_cap_commander_register_cb(&mock_cap_commander_cb);
	zassert_equal(0, err, "Unexpected return value %d", err);

	for (size_t i = 0; i < ARRAY_SIZE(fixture->conns); i++) {
		struct bt_vcp_vol_ctlr *vol_ctlr; /* We don't care about this */

		err = bt_cap_commander_discover(&fixture->conns[i]);
		zassert_equal(0, err, "Unexpected return value %d", err);

		err = bt_vcp_vol_ctlr_discover(&fixture->conns[i], &vol_ctlr);
		zassert_equal(0, err, "Unexpected return value %d", err);
	}

	err = bt_cap_commander_change_volume_offset(&param);
	zassert_equal(0, err, "Unexpected return value %d", err);

	zexpect_call_count("bt_cap_commander_cb.volume_offset_changed", 1,
			   mock_cap_commander_volume_offset_changed_cb_fake.call_count);
}

ZTEST_F(cap_commander_test_suite, test_commander_change_volume_offset_double)
{
	struct bt_cap_commander_change_volume_offset_member_param
		member_params[ARRAY_SIZE(fixture->conns)];
	const struct bt_cap_commander_change_volume_offset_param param = {
		.type = BT_CAP_SET_TYPE_AD_HOC,
		.param = member_params,
		.count = ARRAY_SIZE(member_params),
	};
	int err;

	for (size_t i = 0U; i < ARRAY_SIZE(member_params); i++) {
		member_params[i].member.member = &fixture->conns[i];
		member_params[i].offset = 100 + i;
	}

	err = bt_cap_commander_register_cb(&mock_cap_commander_cb);
	zassert_equal(0, err, "Unexpected return value %d", err);

	for (size_t i = 0; i < ARRAY_SIZE(fixture->conns); i++) {
		struct bt_vcp_vol_ctlr *vol_ctlr; /* We don't care about this */

		err = bt_cap_commander_discover(&fixture->conns[i]);
		zassert_equal(0, err, "Unexpected return value %d", err);

		err = bt_vcp_vol_ctlr_discover(&fixture->conns[i], &vol_ctlr);
		zassert_equal(0, err, "Unexpected return value %d", err);
	}

	err = bt_cap_commander_change_volume_offset(&param);
	zassert_equal(0, err, "Unexpected return value %d", err);

	zexpect_call_count("bt_cap_commander_cb.volume_offset_changed", 1,
			   mock_cap_commander_volume_offset_changed_cb_fake.call_count);

	/* Verify that it still works as expected if we set the same value twice */
	err = bt_cap_commander_change_volume_offset(&param);
	zassert_equal(0, err, "Unexpected return value %d", err);

	zexpect_call_count("bt_cap_commander_cb.volume_offset_changed", 2,
			   mock_cap_commander_volume_offset_changed_cb_fake.call_count);
}

ZTEST_F(cap_commander_test_suite, test_commander_change_volume_offset_inval_param_null)
{
	int err;

	err = bt_cap_commander_change_volume_offset(NULL);
	zassert_equal(-EINVAL, err, "Unexpected return value %d", err);
}

ZTEST_F(cap_commander_test_suite, test_commander_change_volume_offset_inval_param_null_param)
{
	const struct bt_cap_commander_change_volume_offset_param param = {
		.type = BT_CAP_SET_TYPE_AD_HOC,
		.param = NULL,
		.count = ARRAY_SIZE(fixture->conns),
	};
	int err;

	err = bt_cap_commander_change_volume_offset(&param);
	zassert_equal(-EINVAL, err, "Unexpected return value %d", err);
}

ZTEST_F(cap_commander_test_suite, test_commander_change_volume_offset_inval_param_null_member)
{
	struct bt_cap_commander_change_volume_offset_member_param
		member_params[ARRAY_SIZE(fixture->conns)];
	const struct bt_cap_commander_change_volume_offset_param param = {
		.type = BT_CAP_SET_TYPE_AD_HOC,
		.param = member_params,
		.count = ARRAY_SIZE(member_params),
	};
	int err;

	for (size_t i = 0U; i < ARRAY_SIZE(member_params); i++) {
		member_params[i].member.member = &fixture->conns[i];
		member_params[i].offset = 100 + i;
	}
	member_params[ARRAY_SIZE(member_params) - 1].member.member = NULL;

	err = bt_cap_commander_change_volume_offset(&param);
	zassert_equal(-EINVAL, err, "Unexpected return value %d", err);
}

ZTEST_F(cap_commander_test_suite, test_commander_change_volume_offset_inval_missing_cas)
{
	struct bt_cap_commander_change_volume_offset_member_param
		member_params[ARRAY_SIZE(fixture->conns)];
	const struct bt_cap_commander_change_volume_offset_param param = {
		.type = BT_CAP_SET_TYPE_AD_HOC,
		.param = member_params,
		.count = ARRAY_SIZE(member_params),
	};
	int err;

	for (size_t i = 0U; i < ARRAY_SIZE(member_params); i++) {
		member_params[i].member.member = &fixture->conns[i];
		member_params[i].offset = 100 + i;
	}

	err = bt_cap_commander_register_cb(&mock_cap_commander_cb);
	zassert_equal(0, err, "Unexpected return value %d", err);

	for (size_t i = 0; i < ARRAY_SIZE(fixture->conns); i++) {
		struct bt_vcp_vol_ctlr *vol_ctlr; /* We don't care about this */

		err = bt_vcp_vol_ctlr_discover(&fixture->conns[i], &vol_ctlr);
		zassert_equal(0, err, "Unexpected return value %d", err);
	}

	err = bt_cap_commander_change_volume_offset(&param);
	zassert_equal(-EINVAL, err, "Unexpected return value %d", err);
}

ZTEST_F(cap_commander_test_suite, test_commander_change_volume_offset_inval_missing_vocs)
{
	struct bt_cap_commander_change_volume_offset_member_param
		member_params[ARRAY_SIZE(fixture->conns)];
	const struct bt_cap_commander_change_volume_offset_param param = {
		.type = BT_CAP_SET_TYPE_AD_HOC,
		.param = member_params,
		.count = ARRAY_SIZE(member_params),
	};
	int err;

	for (size_t i = 0U; i < ARRAY_SIZE(member_params); i++) {
		member_params[i].member.member = &fixture->conns[i];
		member_params[i].offset = 100 + i;
	}

	err = bt_cap_commander_register_cb(&mock_cap_commander_cb);
	zassert_equal(0, err, "Unexpected return value %d", err);

	for (size_t i = 0; i < ARRAY_SIZE(fixture->conns); i++) {
		err = bt_cap_commander_discover(&fixture->conns[i]);
		zassert_equal(0, err, "Unexpected return value %d", err);
	}

	err = bt_cap_commander_change_volume_offset(&param);
	zassert_equal(-EINVAL, err, "Unexpected return value %d", err);
}

ZTEST_F(cap_commander_test_suite, test_commander_change_volume_offset_inval_param_zero_count)
{
	struct bt_cap_commander_change_volume_offset_member_param
		member_params[ARRAY_SIZE(fixture->conns)];
	const struct bt_cap_commander_change_volume_offset_param param = {
		.type = BT_CAP_SET_TYPE_AD_HOC,
		.param = member_params,
		.count = 0U,
	};
	int err;

	for (size_t i = 0U; i < ARRAY_SIZE(member_params); i++) {
		member_params[i].member.member = &fixture->conns[i];
		member_params[i].offset = 100 + i;
	}

	err = bt_cap_commander_change_volume_offset(&param);
	zassert_equal(-EINVAL, err, "Unexpected return value %d", err);
}

ZTEST_F(cap_commander_test_suite, test_commander_change_volume_offset_inval_param_inval_count)
{
	struct bt_cap_commander_change_volume_offset_member_param
		member_params[ARRAY_SIZE(fixture->conns)];
	const struct bt_cap_commander_change_volume_offset_param param = {
		.type = BT_CAP_SET_TYPE_AD_HOC,
		.param = member_params,
		.count = CONFIG_BT_MAX_CONN + 1,
	};
	int err;

	for (size_t i = 0U; i < ARRAY_SIZE(member_params); i++) {
		member_params[i].member.member = &fixture->conns[i];
		member_params[i].offset = 100 + i;
	}

	err = bt_cap_commander_change_volume_offset(&param);
	zassert_equal(-EINVAL, err, "Unexpected return value %d", err);
}

ZTEST_F(cap_commander_test_suite, test_commander_change_volume_offset_inval_param_inval_offset_max)
{
	struct bt_cap_commander_change_volume_offset_member_param
		member_params[ARRAY_SIZE(fixture->conns)];
	const struct bt_cap_commander_change_volume_offset_param param = {
		.type = BT_CAP_SET_TYPE_AD_HOC,
		.param = member_params,
		.count = ARRAY_SIZE(member_params),
	};
	int err;

	for (size_t i = 0U; i < ARRAY_SIZE(member_params); i++) {
		member_params[i].member.member = &fixture->conns[i];
		member_params[i].offset = BT_VOCS_MAX_OFFSET + 1;
	}

	err = bt_cap_commander_change_volume_offset(&param);
	zassert_equal(-EINVAL, err, "Unexpected return value %d", err);
}

ZTEST_F(cap_commander_test_suite, test_commander_change_volume_offset_inval_param_inval_offset_min)
{
	struct bt_cap_commander_change_volume_offset_member_param
		member_params[ARRAY_SIZE(fixture->conns)];
	const struct bt_cap_commander_change_volume_offset_param param = {
		.type = BT_CAP_SET_TYPE_AD_HOC,
		.param = member_params,
		.count = ARRAY_SIZE(member_params),
	};
	int err;

	for (size_t i = 0U; i < ARRAY_SIZE(member_params); i++) {
		member_params[i].member.member = &fixture->conns[i];
		member_params[i].offset = BT_VOCS_MIN_OFFSET - 1;
	}

	err = bt_cap_commander_change_volume_offset(&param);
	zassert_equal(-EINVAL, err, "Unexpected return value %d", err);
}

ZTEST_F(cap_commander_test_suite, test_commander_change_volume_mute_state)
{
	union bt_cap_set_member members[ARRAY_SIZE(fixture->conns)];
	const struct bt_cap_commander_change_volume_mute_state_param param = {
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
		struct bt_vcp_vol_ctlr *vol_ctlr; /* We don't care about this */

		err = bt_cap_commander_discover(&fixture->conns[i]);
		zassert_equal(0, err, "Unexpected return value %d", err);

		err = bt_vcp_vol_ctlr_discover(&fixture->conns[i], &vol_ctlr);
		zassert_equal(0, err, "Unexpected return value %d", err);
	}

	err = bt_cap_commander_change_volume_mute_state(&param);
	zassert_equal(0, err, "Unexpected return value %d", err);

	zexpect_call_count("bt_cap_commander_cb.volume_mute_changed", 1,
			   mock_cap_commander_volume_mute_changed_cb_fake.call_count);
}

ZTEST_F(cap_commander_test_suite, test_commander_change_volume_mute_state_double)
{
	union bt_cap_set_member members[ARRAY_SIZE(fixture->conns)];
	const struct bt_cap_commander_change_volume_mute_state_param param = {
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
		struct bt_vcp_vol_ctlr *vol_ctlr; /* We don't care about this */

		err = bt_cap_commander_discover(&fixture->conns[i]);
		zassert_equal(0, err, "Unexpected return value %d", err);

		err = bt_vcp_vol_ctlr_discover(&fixture->conns[i], &vol_ctlr);
		zassert_equal(0, err, "Unexpected return value %d", err);
	}

	err = bt_cap_commander_change_volume_mute_state(&param);
	zassert_equal(0, err, "Unexpected return value %d", err);

	zexpect_call_count("bt_cap_commander_cb.volume_mute_changed", 1,
			   mock_cap_commander_volume_mute_changed_cb_fake.call_count);

	/* Verify that it still works as expected if we set the same value twice */
	err = bt_cap_commander_change_volume_mute_state(&param);
	zassert_equal(0, err, "Unexpected return value %d", err);

	zexpect_call_count("bt_cap_commander_cb.volume_mute_changed", 2,
			   mock_cap_commander_volume_mute_changed_cb_fake.call_count);
}

ZTEST_F(cap_commander_test_suite, test_commander_change_volume_mute_state_inval_param_null)
{
	int err;

	err = bt_cap_commander_change_volume_mute_state(NULL);
	zassert_equal(-EINVAL, err, "Unexpected return value %d", err);
}

ZTEST_F(cap_commander_test_suite, test_commander_change_volume_mute_state_inval_param_null_members)
{
	const struct bt_cap_commander_change_volume_mute_state_param param = {
		.type = BT_CAP_SET_TYPE_AD_HOC,
		.members = NULL,
		.count = ARRAY_SIZE(fixture->conns),
		.mute = true,
	};
	int err;

	err = bt_cap_commander_change_volume_mute_state(&param);
	zassert_equal(-EINVAL, err, "Unexpected return value %d", err);
}

ZTEST_F(cap_commander_test_suite, test_commander_change_volume_mute_state_inval_param_null_member)
{
	union bt_cap_set_member members[ARRAY_SIZE(fixture->conns)];
	const struct bt_cap_commander_change_volume_mute_state_param param = {
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

	err = bt_cap_commander_change_volume_mute_state(&param);
	zassert_equal(-EINVAL, err, "Unexpected return value %d", err);
}

ZTEST_F(cap_commander_test_suite, test_commander_change_volume_mute_state_inval_missing_cas)
{
	union bt_cap_set_member members[ARRAY_SIZE(fixture->conns)];
	const struct bt_cap_commander_change_volume_mute_state_param param = {
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
		struct bt_vcp_vol_ctlr *vol_ctlr; /* We don't care about this */

		err = bt_vcp_vol_ctlr_discover(&fixture->conns[i], &vol_ctlr);
		zassert_equal(0, err, "Unexpected return value %d", err);
	}

	err = bt_cap_commander_change_volume_mute_state(&param);
	zassert_equal(-EINVAL, err, "Unexpected return value %d", err);
}

ZTEST_F(cap_commander_test_suite, test_commander_change_volume_mute_state_inval_missing_vcs)
{
	union bt_cap_set_member members[ARRAY_SIZE(fixture->conns)];
	const struct bt_cap_commander_change_volume_mute_state_param param = {
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
		err = bt_cap_commander_discover(&fixture->conns[i]);
		zassert_equal(0, err, "Unexpected return value %d", err);
	}

	err = bt_cap_commander_change_volume_mute_state(&param);
	zassert_equal(-EINVAL, err, "Unexpected return value %d", err);
}

ZTEST_F(cap_commander_test_suite, test_commander_change_volume_mute_state_inval_param_zero_count)
{
	union bt_cap_set_member members[ARRAY_SIZE(fixture->conns)];
	const struct bt_cap_commander_change_volume_mute_state_param param = {
		.type = BT_CAP_SET_TYPE_AD_HOC,
		.members = members,
		.count = 0U,
		.mute = true,
	};
	int err;

	for (size_t i = 0U; i < ARRAY_SIZE(members); i++) {
		members[i].member = &fixture->conns[i];
	}

	err = bt_cap_commander_change_volume_mute_state(&param);
	zassert_equal(-EINVAL, err, "Unexpected return value %d", err);
}

ZTEST_F(cap_commander_test_suite, test_commander_change_volume_mute_state_inval_param_inval_count)
{
	union bt_cap_set_member members[ARRAY_SIZE(fixture->conns)];
	const struct bt_cap_commander_change_volume_mute_state_param param = {
		.type = BT_CAP_SET_TYPE_AD_HOC,
		.members = members,
		.count = CONFIG_BT_MAX_CONN + 1,
		.mute = true,
	};
	int err;

	for (size_t i = 0U; i < ARRAY_SIZE(members); i++) {
		members[i].member = &fixture->conns[i];
	}

	err = bt_cap_commander_change_volume_mute_state(&param);
	zassert_equal(-EINVAL, err, "Unexpected return value %d", err);
}
