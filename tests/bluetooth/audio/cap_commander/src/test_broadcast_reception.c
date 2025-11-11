/* test_broadcast_reception.c - unit test for broadcast reception start and stop */

/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <errno.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <zephyr/autoconf.h>
#include <zephyr/bluetooth/addr.h>
#include <zephyr/bluetooth/audio/audio.h>
#include <zephyr/bluetooth/audio/bap.h>
#include <zephyr/bluetooth/audio/cap.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gap.h>
#include <zephyr/bluetooth/hci_types.h>
#include <zephyr/fff.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/util_macro.h>
#include <zephyr/ztest_assert.h>
#include <zephyr/ztest_test.h>

#include "cap_commander.h"
#include "conn.h"
#include "expects_util.h"
#include "cap_mocks.h"
#include "test_common.h"

LOG_MODULE_REGISTER(bt_broadcast_reception_test, CONFIG_BT_CAP_COMMANDER_LOG_LEVEL);

#define FFF_GLOBALS

#define SID          0x0E
#define ADV_INTERVAL 10
#define BROADCAST_ID 0x55AA55

struct cap_commander_test_broadcast_reception_fixture {
	struct bt_conn conns[CONFIG_BT_MAX_CONN];

	struct bt_bap_bass_subgroup subgroups[CONFIG_BT_BAP_BASS_MAX_SUBGROUPS];
	struct bt_cap_commander_broadcast_reception_start_member_param
		start_member_params[CONFIG_BT_MAX_CONN];
	struct bt_cap_commander_broadcast_reception_start_param start_param;
	struct bt_cap_commander_broadcast_reception_stop_member_param
		stop_member_params[CONFIG_BT_MAX_CONN];
	struct bt_cap_commander_broadcast_reception_stop_param stop_param;
	struct bt_bap_broadcast_assistant_cb broadcast_assistant_cb;
};

/* src_id can not be part of the fixture since it is accessed in the callback function */
static uint8_t src_id[CONFIG_BT_MAX_CONN];

static void test_start_param_init(void *f);
static void test_stop_param_init(void *f);

static void cap_commander_broadcast_assistant_recv_state_cb(
	struct bt_conn *conn, int err, const struct bt_bap_scan_delegator_recv_state *state)
{
	uint8_t index;

	index = bt_conn_index(conn);
	src_id[index] = state->src_id;
}

static void cap_commander_test_broadcast_reception_fixture_init(
	struct cap_commander_test_broadcast_reception_fixture *fixture)
{
	int err;

	for (size_t i = 0; i < ARRAY_SIZE(fixture->conns); i++) {
		test_conn_init(&fixture->conns[i]);
		fixture->conns[i].index = i;
	}
	test_start_param_init(fixture);
	test_stop_param_init(fixture);

	fixture->broadcast_assistant_cb.recv_state =
		cap_commander_broadcast_assistant_recv_state_cb;
	err = bt_bap_broadcast_assistant_register_cb(&fixture->broadcast_assistant_cb);
	zassert_equal(0, err, "Failed registering broadcast assistant callback functions %d", err);
}

static void *cap_commander_test_broadcast_reception_setup(void)
{
	struct cap_commander_test_broadcast_reception_fixture *fixture;

	fixture = malloc(sizeof(*fixture));
	zassert_not_null(fixture);

	return fixture;
}

static void cap_commander_test_broadcast_reception_before(void *f)
{
	int err;
	struct cap_commander_test_broadcast_reception_fixture *fixture = f;

	memset(f, 0, sizeof(struct cap_commander_test_broadcast_reception_fixture));
	cap_commander_test_broadcast_reception_fixture_init(fixture);

	for (size_t i = 0; i < ARRAY_SIZE(fixture->conns); i++) {
		err = bt_cap_commander_discover(&fixture->conns[i]);
		zassert_equal(0, err, "Unexpected return value %d", err);
	}
}

static void cap_commander_test_broadcast_reception_after(void *f)
{
	struct cap_commander_test_broadcast_reception_fixture *fixture = f;

	bt_cap_commander_unregister_cb(&mock_cap_commander_cb);
	bt_bap_broadcast_assistant_unregister_cb(&fixture->broadcast_assistant_cb);

	/* We need to cleanup since the CAP commander remembers state */
	(void)bt_cap_commander_cancel();

	for (size_t i = 0; i < ARRAY_SIZE(fixture->conns); i++) {
		mock_bt_conn_disconnected(&fixture->conns[i], BT_HCI_ERR_REMOTE_USER_TERM_CONN);
	}
}

static void cap_commander_test_broadcast_reception_teardown(void *f)
{
	free(f);
}

static void test_start_param_init(void *f)
{
	struct cap_commander_test_broadcast_reception_fixture *fixture = f;
	int err;

	fixture->start_param.type = BT_CAP_SET_TYPE_AD_HOC;
	fixture->start_param.param = fixture->start_member_params;

	fixture->start_param.count = ARRAY_SIZE(fixture->start_member_params);

	for (size_t i = 0; i < ARRAY_SIZE(fixture->subgroups); i++) {
		fixture->subgroups[i].bis_sync = BIT(i);
		fixture->subgroups[i].metadata_len = 0;
	}

	for (size_t i = 0U; i < ARRAY_SIZE(fixture->start_member_params); i++) {
		fixture->start_member_params[i].member.member = &fixture->conns[i];
		bt_addr_le_copy(&fixture->start_member_params[i].addr, BT_ADDR_LE_ANY);
		fixture->start_member_params[i].adv_sid = SID;
		fixture->start_member_params[i].pa_interval = ADV_INTERVAL;
		fixture->start_member_params[i].broadcast_id = BROADCAST_ID;
		memcpy(fixture->start_member_params[i].subgroups, &fixture->subgroups[0],
		       sizeof(struct bt_bap_bass_subgroup) * CONFIG_BT_BAP_BASS_MAX_SUBGROUPS);
		fixture->start_member_params[i].num_subgroups = CONFIG_BT_BAP_BASS_MAX_SUBGROUPS;
	}

	for (size_t i = 0; i < ARRAY_SIZE(fixture->conns); i++) {
		err = bt_cap_commander_discover(&fixture->conns[i]);
		zassert_equal(0, err, "Unexpected return value %d", err);
	}
}

static void test_stop_param_init(void *f)
{
	struct cap_commander_test_broadcast_reception_fixture *fixture = f;

	fixture->stop_param.type = BT_CAP_SET_TYPE_AD_HOC;
	fixture->stop_param.param = fixture->stop_member_params;
	fixture->stop_param.count = ARRAY_SIZE(fixture->stop_member_params);

	for (size_t i = 0U; i < ARRAY_SIZE(fixture->stop_member_params); i++) {
		fixture->stop_member_params[i].member.member = &fixture->conns[i];
		fixture->stop_member_params[i].src_id = 0;
		fixture->stop_member_params[i].num_subgroups = CONFIG_BT_BAP_BASS_MAX_SUBGROUPS;
	}
}

static void
test_broadcast_reception_start(struct bt_cap_commander_broadcast_reception_start_param *start_param)
{
	int err;

	err = bt_cap_commander_broadcast_reception_start(start_param);
	zassert_equal(0, err, "Unexpected return value %d", err);

	zexpect_call_count("bt_cap_commander_cb.broadcast_reception_start", 1,
			   mock_cap_commander_broadcast_reception_start_cb_fake.call_count);
	zassert_equal_ptr(NULL,
			  mock_cap_commander_broadcast_reception_start_cb_fake.arg0_history[0]);
	zassert_equal(0, mock_cap_commander_broadcast_reception_start_cb_fake.arg1_history[0]);
}

static void
test_broadcast_reception_stop(struct bt_cap_commander_broadcast_reception_stop_param *stop_param)
{
	int err;

	err = bt_cap_commander_broadcast_reception_stop(stop_param);
	zassert_equal(0, err, "Unexpected return value %d", err);

	zexpect_call_count("bt_cap_commander_cb.broadcast_reception_stop", 1,
			   mock_cap_commander_broadcast_reception_stop_cb_fake.call_count);
	zassert_equal_ptr(NULL,
			  mock_cap_commander_broadcast_reception_stop_cb_fake.arg0_history[0]);
	zassert_equal(0, mock_cap_commander_broadcast_reception_stop_cb_fake.arg1_history[0]);
}

ZTEST_SUITE(cap_commander_test_broadcast_reception, NULL,
	    cap_commander_test_broadcast_reception_setup,
	    cap_commander_test_broadcast_reception_before,
	    cap_commander_test_broadcast_reception_after,
	    cap_commander_test_broadcast_reception_teardown);

ZTEST_F(cap_commander_test_broadcast_reception, test_commander_reception_start)
{
	int err;

	err = bt_cap_commander_register_cb(&mock_cap_commander_cb);
	zassert_equal(0, err, "Unexpected return value %d", err);

	test_broadcast_reception_start(&fixture->start_param);
}

ZTEST_F(cap_commander_test_broadcast_reception, test_commander_reception_start_one_subgroup)
{
	int err;

	err = bt_cap_commander_register_cb(&mock_cap_commander_cb);
	zassert_equal(0, err, "Unexpected return value %d", err);

	/* We test with one subgroup, instead of CONFIG_BT_BAP_BASS_MAX_SUBGROUPS subgroups */
	for (size_t i = 0U; i < CONFIG_BT_MAX_CONN; i++) {
		fixture->start_param.param[i].num_subgroups = 1;
	}

	test_broadcast_reception_start(&fixture->start_param);
}

ZTEST_F(cap_commander_test_broadcast_reception, test_commander_reception_start_double)
{
	int err;

	err = bt_cap_commander_register_cb(&mock_cap_commander_cb);
	zassert_equal(0, err, "Unexpected return value %d", err);

	test_broadcast_reception_start(&fixture->start_param);

	/*
	 * We can not use test_broadcast_reception_start because of the check on how often the
	 * callback function is called
	 */
	err = bt_cap_commander_broadcast_reception_start(&fixture->start_param);
	zassert_equal(0, err, "Unexpected return value %d", err);

	zexpect_call_count("bt_cap_commander_cb.broadcast_reception_start", 2,
			   mock_cap_commander_broadcast_reception_start_cb_fake.call_count);
}

ZTEST_F(cap_commander_test_broadcast_reception, test_commander_reception_start_inval_param_null)
{
	int err;

	err = bt_cap_commander_broadcast_reception_start(NULL);
	zassert_equal(-EINVAL, err, "Unexpected return value %d", err);

	zexpect_call_count("bt_cap_commander_cb.broadcast_reception_start", 0,
			   mock_cap_commander_broadcast_reception_start_cb_fake.call_count);
}

ZTEST_F(cap_commander_test_broadcast_reception,
	test_commander_reception_start_inval_param_zero_count)
{
	int err;

	fixture->start_param.count = 0;

	err = bt_cap_commander_broadcast_reception_start(&fixture->start_param);
	zassert_equal(-EINVAL, err, "Unexpected return value %d", err);

	zexpect_call_count("bt_cap_commander_cb.broadcast_reception_start", 0,
			   mock_cap_commander_broadcast_reception_start_cb_fake.call_count);
}

ZTEST_F(cap_commander_test_broadcast_reception,
	test_commander_reception_start_inval_param_high_count)
{
	int err;

	fixture->start_param.count = CONFIG_BT_MAX_CONN + 1;

	err = bt_cap_commander_broadcast_reception_start(&fixture->start_param);
	zassert_equal(-EINVAL, err, "Unexpected return value %d", err);

	zexpect_call_count("bt_cap_commander_cb.broadcast_reception_start", 0,
			   mock_cap_commander_broadcast_reception_start_cb_fake.call_count);
}

ZTEST_F(cap_commander_test_broadcast_reception,
	test_commander_reception_start_inval_param_null_param)
{
	int err;

	fixture->start_param.type = BT_CAP_SET_TYPE_AD_HOC;
	fixture->start_param.param = NULL;
	fixture->start_param.count = ARRAY_SIZE(fixture->conns);

	err = bt_cap_commander_broadcast_reception_start(&fixture->start_param);
	zassert_equal(-EINVAL, err, "Unexpected return value %d", err);

	zexpect_call_count("bt_cap_commander_cb.broadcast_reception_start", 0,
			   mock_cap_commander_broadcast_reception_start_cb_fake.call_count);
}

ZTEST_F(cap_commander_test_broadcast_reception, test_commander_reception_start_inval_null_member)
{
	int err;

	fixture->start_param.param[0].member.member = NULL;

	err = bt_cap_commander_broadcast_reception_start(&fixture->start_param);
	zassert_equal(-EINVAL, err, "Unexpected return value %d", err);

	zexpect_call_count("bt_cap_commander_cb.broadcast_reception_start", 0,
			   mock_cap_commander_broadcast_reception_start_cb_fake.call_count);
}

ZTEST_F(cap_commander_test_broadcast_reception, test_commander_reception_start_inval_missing_cas)
{
	int err;

	fixture->start_param.type = BT_CAP_SET_TYPE_CSIP;

	err = bt_cap_commander_broadcast_reception_start(&fixture->start_param);
	zassert_equal(-EINVAL, err, "Unexpected return value %d", err);

	zexpect_call_count("bt_cap_commander_cb.broadcast_reception_start", 0,
			   mock_cap_commander_broadcast_reception_start_cb_fake.call_count);
}

ZTEST_F(cap_commander_test_broadcast_reception, test_commander_reception_start_inval_addr_type)
{
	int err;

	fixture->start_param.param[0].addr.type = BT_ADDR_LE_RANDOM + 1;

	err = bt_cap_commander_broadcast_reception_start(&fixture->start_param);
	zassert_equal(-EINVAL, err, "Unexpected return value %d", err);

	zexpect_call_count("bt_cap_commander_cb.broadcast_reception_start", 0,
			   mock_cap_commander_broadcast_reception_start_cb_fake.call_count);
}

ZTEST_F(cap_commander_test_broadcast_reception, test_commander_reception_start_inval_sid)
{
	int err;

	fixture->start_param.param[0].adv_sid = BT_GAP_SID_MAX + 1;

	err = bt_cap_commander_broadcast_reception_start(&fixture->start_param);
	zassert_equal(-EINVAL, err, "Unexpected return value %d", err);

	zexpect_call_count("bt_cap_commander_cb.broadcast_reception_start", 0,
			   mock_cap_commander_broadcast_reception_start_cb_fake.call_count);
}

ZTEST_F(cap_commander_test_broadcast_reception,
	test_commander_reception_start_inval_pa_interval_low)
{
	int err;

	fixture->start_param.param[0].pa_interval = BT_GAP_PER_ADV_MIN_INTERVAL - 1;

	err = bt_cap_commander_broadcast_reception_start(&fixture->start_param);
	zassert_equal(-EINVAL, err, "Unexpected return value %d", err);

	zexpect_call_count("bt_cap_commander_cb.broadcast_reception_start", 0,
			   mock_cap_commander_broadcast_reception_start_cb_fake.call_count);
}

/*
 * Test for pa_interval_high omitted
 * pa_interval is a uint16_t, BT_GAP_PER_ADV_MAX_INTERVAL is defined as 0xFFFF
 * and therefor we can not test in the current implementation
 */

ZTEST_F(cap_commander_test_broadcast_reception, test_commander_reception_start_inval_broadcast_id)
{
	int err;

	fixture->start_param.param[0].broadcast_id = BT_AUDIO_BROADCAST_ID_MAX + 1;

	err = bt_cap_commander_broadcast_reception_start(&fixture->start_param);
	zassert_equal(-EINVAL, err, "Unexpected return value %d", err);

	zexpect_call_count("bt_cap_commander_cb.broadcast_reception_start", 0,
			   mock_cap_commander_broadcast_reception_start_cb_fake.call_count);
}

ZTEST_F(cap_commander_test_broadcast_reception, test_commander_reception_start_inval_no_subgroups)
{
	int err;

	fixture->start_param.param[0].num_subgroups = 0;

	err = bt_cap_commander_broadcast_reception_start(&fixture->start_param);
	zassert_equal(-EINVAL, err, "Unexpected return value %d", err);

	zexpect_call_count("bt_cap_commander_cb.broadcast_reception_start", 0,
			   mock_cap_commander_broadcast_reception_start_cb_fake.call_count);
}

ZTEST_F(cap_commander_test_broadcast_reception, test_commander_reception_start_inval_num_subgroups)
{
	int err;

	fixture->start_param.param[0].num_subgroups = CONFIG_BT_BAP_BASS_MAX_SUBGROUPS + 1;

	err = bt_cap_commander_broadcast_reception_start(&fixture->start_param);
	zassert_equal(-EINVAL, err, "Unexpected return value %d", err);

	zexpect_call_count("bt_cap_commander_cb.broadcast_reception_start", 0,
			   mock_cap_commander_broadcast_reception_start_cb_fake.call_count);
}

ZTEST_F(cap_commander_test_broadcast_reception,
	test_commander_reception_start_inval_duplicate_bis_sync)
{
	int err;

	if (CONFIG_BT_BAP_BASS_MAX_SUBGROUPS == 1) {
		ztest_test_skip();
	}

	fixture->start_param.param[0].subgroups[0].bis_sync =
		fixture->start_param.param[0].subgroups[1].bis_sync;

	err = bt_cap_commander_broadcast_reception_start(&fixture->start_param);
	zassert_equal(-EINVAL, err, "Unexpected return value %d", err);

	zexpect_call_count("bt_cap_commander_cb.broadcast_reception_start", 0,
			   mock_cap_commander_broadcast_reception_start_cb_fake.call_count);
}

ZTEST_F(cap_commander_test_broadcast_reception, test_commander_reception_start_inval_metadata_len)
{
	int err;

	if (CONFIG_BT_AUDIO_CODEC_CFG_MAX_METADATA_SIZE >= UINT8_MAX) {
		ztest_test_skip();
	}

	fixture->start_param.param[0].subgroups[0].metadata_len =
		(uint8_t)(CONFIG_BT_AUDIO_CODEC_CFG_MAX_METADATA_SIZE + 1);

	err = bt_cap_commander_broadcast_reception_start(&fixture->start_param);
	zassert_equal(-EINVAL, err, "Unexpected return value %d", err);

	zexpect_call_count("bt_cap_commander_cb.broadcast_reception_start", 0,
			   mock_cap_commander_broadcast_reception_start_cb_fake.call_count);
}

ZTEST_F(cap_commander_test_broadcast_reception, test_commander_reception_stop_default_subgroups)
{
	int err;

	err = bt_cap_commander_register_cb(&mock_cap_commander_cb);
	zassert_equal(0, err, "Unexpected return value %d", err);

	test_broadcast_reception_start(&fixture->start_param);

	for (size_t i = 0U; i < CONFIG_BT_MAX_CONN; i++) {
		fixture->stop_param.param[i].src_id = src_id[i];
	}

	test_broadcast_reception_stop(&fixture->stop_param);
}

ZTEST_F(cap_commander_test_broadcast_reception, test_commander_reception_stop_one_subgroup)
{
	int err;

	err = bt_cap_commander_register_cb(&mock_cap_commander_cb);
	zassert_equal(0, err, "Unexpected return value %d", err);

	test_broadcast_reception_start(&fixture->start_param);

	/* We test with one subgroup, instead of CONFIG_BT_BAP_BASS_MAX_SUBGROUPS subgroups */
	for (size_t i = 0U; i < CONFIG_BT_MAX_CONN; i++) {
		fixture->stop_param.param[i].num_subgroups = 1;
		fixture->stop_param.param[i].src_id = src_id[i];
	}

	test_broadcast_reception_stop(&fixture->stop_param);
}

ZTEST_F(cap_commander_test_broadcast_reception, test_commander_reception_stop_double)
{
	int err;

	err = bt_cap_commander_register_cb(&mock_cap_commander_cb);
	zassert_equal(0, err, "Unexpected return value %d", err);

	test_broadcast_reception_start(&fixture->start_param);

	for (size_t i = 0U; i < CONFIG_BT_MAX_CONN; i++) {
		printk("Source ID %d: %d %d\n", i, fixture->stop_param.param[i].src_id, src_id[i]);

		fixture->stop_param.param[i].src_id = src_id[i];
	}

	test_broadcast_reception_stop(&fixture->stop_param);

	/*
	 * We can not use test_broadcast_reception_stop because of the check on how often the
	 * callback function is called
	 */
	err = bt_cap_commander_broadcast_reception_stop(&fixture->stop_param);
	zassert_equal(0, err, "Unexpected return value %d", err);
	zexpect_call_count("bt_cap_commander_cb.broadcast_reception_stop", 2,
			   mock_cap_commander_broadcast_reception_stop_cb_fake.call_count);
}

ZTEST_F(cap_commander_test_broadcast_reception, test_commander_reception_stop_inval_param_null)
{
	int err;

	err = bt_cap_commander_broadcast_reception_stop(NULL);
	zassert_equal(-EINVAL, err, "Unexpected return value %d", err);

	zexpect_call_count("bt_cap_commander_cb.broadcast_reception_stop", 0,
			   mock_cap_commander_broadcast_reception_stop_cb_fake.call_count);
}

ZTEST_F(cap_commander_test_broadcast_reception,
	test_commander_reception_stop_inval_param_zero_count)
{
	int err;

	fixture->stop_param.count = 0;

	err = bt_cap_commander_broadcast_reception_stop(&fixture->stop_param);
	zassert_equal(-EINVAL, err, "Unexpected return value %d", err);

	zexpect_call_count("bt_cap_commander_cb.broadcast_reception_stop", 0,
			   mock_cap_commander_broadcast_reception_stop_cb_fake.call_count);
}

ZTEST_F(cap_commander_test_broadcast_reception,
	test_commander_reception_stop_inval_param_high_count)
{
	int err;

	fixture->stop_param.count = CONFIG_BT_MAX_CONN + 1;

	err = bt_cap_commander_broadcast_reception_stop(&fixture->stop_param);
	zassert_equal(-EINVAL, err, "Unexpected return value %d", err);

	zexpect_call_count("bt_cap_commander_cb.broadcast_reception_stop", 0,
			   mock_cap_commander_broadcast_reception_stop_cb_fake.call_count);
}

ZTEST_F(cap_commander_test_broadcast_reception,
	test_commander_reception_stop_inval_param_null_param)
{
	int err;

	fixture->stop_param.type = BT_CAP_SET_TYPE_AD_HOC;
	fixture->stop_param.param = NULL;
	fixture->stop_param.count = ARRAY_SIZE(fixture->conns);

	err = bt_cap_commander_broadcast_reception_stop(&fixture->stop_param);
	zassert_equal(-EINVAL, err, "Unexpected return value %d", err);

	zexpect_call_count("bt_cap_commander_cb.broadcast_reception_stop", 0,
			   mock_cap_commander_broadcast_reception_stop_cb_fake.call_count);
}

ZTEST_F(cap_commander_test_broadcast_reception, test_commander_reception_stop_inval_null_member)
{
	int err;

	fixture->stop_param.param[0].member.member = NULL;

	err = bt_cap_commander_broadcast_reception_stop(&fixture->stop_param);
	zassert_equal(-EINVAL, err, "Unexpected return value %d", err);

	zexpect_call_count("bt_cap_commander_cb.broadcast_reception_stop", 0,
			   mock_cap_commander_broadcast_reception_stop_cb_fake.call_count);
}

ZTEST_F(cap_commander_test_broadcast_reception, test_commander_reception_stop_inval_missing_cas)
{
	int err;

	fixture->stop_param.type = BT_CAP_SET_TYPE_CSIP;

	err = bt_cap_commander_broadcast_reception_stop(&fixture->stop_param);
	zassert_equal(-EINVAL, err, "Unexpected return value %d", err);

	zexpect_call_count("bt_cap_commander_cb.broadcast_reception_stop", 0,
			   mock_cap_commander_broadcast_reception_stop_cb_fake.call_count);
}

ZTEST_F(cap_commander_test_broadcast_reception, test_commander_reception_stop_inval_no_subgroups)
{
	int err;

	fixture->stop_param.param[0].num_subgroups = 0;

	err = bt_cap_commander_broadcast_reception_stop(&fixture->stop_param);
	zassert_equal(-EINVAL, err, "Unexpected return value %d", err);

	zexpect_call_count("bt_cap_commander_cb.broadcast_reception_stop", 0,
			   mock_cap_commander_broadcast_reception_stop_cb_fake.call_count);
}

ZTEST_F(cap_commander_test_broadcast_reception, test_commander_reception_stop_inval_num_subgroups)
{
	int err;

	fixture->stop_param.param[0].num_subgroups = CONFIG_BT_BAP_BASS_MAX_SUBGROUPS + 1;

	err = bt_cap_commander_broadcast_reception_stop(&fixture->stop_param);
	zassert_equal(-EINVAL, err, "Unexpected return value %d", err);

	zexpect_call_count("bt_cap_commander_cb.broadcast_reception_stop", 0,
			   mock_cap_commander_broadcast_reception_stop_cb_fake.call_count);
}
