/* main.c - Application main entry point */

/*
 * Copyright (c) 2023 Codecoup
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdlib.h>
#include <zephyr/fff.h>
#include <zephyr/kernel.h>
#include <zephyr/bluetooth/audio/hap.h>
#include <zephyr/bluetooth/audio/has.h>

#include "conn.h"
#include "csip.h"
#include "has_client.h"
#include "has_internal.h"

DEFINE_FFF_GLOBALS;

#define CONN_INIT(_index, _role)                                                                   \
{                                                                                                  \
	.index = (_index),                                                                         \
	.info.type = BT_CONN_TYPE_LE,                                                              \
	.info.role = (_role),                                                                      \
	.info.id = BT_ID_DEFAULT,                                                                  \
	.info.state = BT_CONN_STATE_CONNECTED,                                                     \
	.info.security.level = BT_SECURITY_L2,                                                     \
	.info.security.enc_key_size = BT_ENC_KEY_SIZE_MAX,                                         \
	.info.security.flags = BT_SECURITY_FLAG_OOB | BT_SECURITY_FLAG_SC,                         \
	.info.le.dst = ((bt_addr_le_t []) {{ 0, {{ 0xc0, 0xde, 0xc0, 0xde, 0xc0, (_index) }}}}),   \
}

#define CONN(_index, _role)                                                                        \
	((struct bt_conn[]) { CONN_INIT(_index, _role) })

FAKE_VALUE_FUNC(bool, bt_addr_le_is_bonded, uint8_t, const bt_addr_le_t *);

#define MISC_FAKES_LIST(FAKE)                                                                      \
	FAKE(bt_addr_le_is_bonded)                                                                 \

FAKE_VOID_FUNC(bt_hap_harc_connected_cb, struct bt_hap_harc *, int);
FAKE_VOID_FUNC(bt_hap_harc_disconnected_cb, struct bt_hap_harc *);
FAKE_VOID_FUNC(bt_hap_harc_status_cb, struct bt_hap_harc *, int);
FAKE_VOID_FUNC(bt_hap_harc_preset_read_func, struct bt_hap_harc *,
	       const struct bt_hap_harc_preset_read_params *, uint8_t, enum bt_has_properties,
	       const char *);
FAKE_VOID_FUNC(bt_hap_harc_complete_func, int, void *);

#define HAP_HARC_CB_FAKES_LIST(FAKE)                                                               \
	FAKE(bt_hap_harc_connected_cb)                                                             \
	FAKE(bt_hap_harc_disconnected_cb)                                                          \
	FAKE(bt_hap_harc_status_cb)                                                                \

FAKE_VOID_FUNC(bt_hap_harc_preset_active_cb, struct bt_hap_harc *, uint8_t);
FAKE_VOID_FUNC(bt_hap_harc_preset_store_cb, struct bt_hap_harc *,
	       const struct bt_has_preset_record *);
FAKE_VOID_FUNC(bt_hap_harc_preset_remove_cb, struct bt_hap_harc *, uint8_t, uint8_t);
FAKE_VOID_FUNC(bt_hap_harc_preset_available_cb, struct bt_hap_harc *, uint8_t, bool);
FAKE_VOID_FUNC(bt_hap_harc_preset_commit_cb, struct bt_hap_harc *);
FAKE_VALUE_FUNC(int, bt_hap_harc_preset_get_cb, struct bt_hap_harc *, uint8_t,
		struct bt_has_preset_record *);

#define HAP_HARC_PRESET_CB_FAKES_LIST(FAKE)                                                        \
	FAKE(bt_hap_harc_preset_active_cb)                                                         \
	FAKE(bt_hap_harc_preset_store_cb)                                                          \
	FAKE(bt_hap_harc_preset_remove_cb)                                                         \
	FAKE(bt_hap_harc_preset_available_cb)                                                      \
	FAKE(bt_hap_harc_preset_commit_cb)                                                         \
	FAKE(bt_hap_harc_preset_get_cb)                                                            \

static void mock_init_rule_before(const struct ztest_unit_test *test, void *fixture)
{
	HAP_HARC_CB_FAKES_LIST(RESET_FAKE);
	HAP_HARC_PRESET_CB_FAKES_LIST(RESET_FAKE);
	MISC_FAKES_LIST(RESET_FAKE);
	CSIP_FFF_FAKES_LIST(RESET_FAKE);
	HAS_CLIENT_FFF_FAKES_LIST(RESET_FAKE);
}

static void mock_destroy_rule_after(const struct ztest_unit_test *test, void *fixture)
{

}

ZTEST_RULE(mock_rule, mock_init_rule_before, mock_destroy_rule_after);

static struct bt_hap_harc_cb cb = {
	.connected = bt_hap_harc_connected_cb,
	.disconnected = bt_hap_harc_disconnected_cb,
};

static struct bt_hap_harc_preset_cb preset_cb = {
	.active = bt_hap_harc_preset_active_cb,
	.store = bt_hap_harc_preset_store_cb,
	.remove = bt_hap_harc_preset_remove_cb,
	.available = bt_hap_harc_preset_available_cb,
	.commit = bt_hap_harc_preset_commit_cb,
	.get = bt_hap_harc_preset_get_cb,
};

struct test_suite_fixture {
	const struct bt_has_client_cb *client_cb;
	const struct bt_csip_set_coordinator_cb *set_coordinator_cb;
	sys_slist_t devices;
};

static void *test_suite_setup(void)
{
	struct test_suite_fixture *fixture;
	int err;

	fixture = malloc(sizeof(*fixture));

	err = bt_hap_harc_cb_register(&cb);
	zassert_equal(0, err, "unexpected error %d", err);

	zassert_equal(1, bt_has_client_init_fake.call_count);
	fixture->client_cb = bt_has_client_init_fake.arg0_val;

	zassert_equal(1, bt_csip_set_coordinator_register_cb_fake.call_count);
	fixture->set_coordinator_cb = bt_csip_set_coordinator_register_cb_fake.arg0_val;

	sys_slist_init(&fixture->devices);

	return fixture;
}

struct test_device {
	struct bt_conn conn;
	struct bt_has_client client;
	enum bt_has_hearing_aid_type type;
	enum bt_has_features features;
	enum bt_has_capabilities caps;
	uint8_t active_index;

	struct test_suite_fixture *fixture;
	sys_snode_t node;
};

static int bt_has_client_bind_custom_fake(struct bt_conn *conn, struct bt_has_client **client)
{
	struct test_device *dev = CONTAINER_OF(conn, struct test_device, conn);

	*client = &dev->client;

	return 0;
}

static int bt_has_client_info_get_custom_fake(const struct bt_has_client *client,
					      struct bt_has_client_info *info)
{
	struct test_device *dev = CONTAINER_OF(client, struct test_device, client);

	info->type = dev->type;
	info->features = dev->features | dev->type;
	info->caps = dev->caps;
	info->active_index = dev->active_index;

	return 0;
}

static int bt_has_client_conn_get_custom_fake(const struct bt_has_client *client,
					      struct bt_conn **conn)
{
	struct test_device *dev = CONTAINER_OF(client, struct test_device, client);

	*conn = &dev->conn;

	return 0;
}

static void test_suite_before(void *f)
{
	bt_has_client_bind_fake.custom_fake = bt_has_client_bind_custom_fake;
	bt_has_client_info_get_fake.custom_fake = bt_has_client_info_get_custom_fake;
	bt_has_client_conn_get_fake.custom_fake = bt_has_client_conn_get_custom_fake;
}

static void test_suite_after(void *f)
{
	struct test_suite_fixture *fixture = f;
	struct test_device *dev, *tmp;

	SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&fixture->devices, dev, tmp, node) {
		mock_bt_conn_disconnected(&dev->conn, 0);

		sys_slist_remove(&fixture->devices, NULL, &dev->node);

		free(dev);
	}
}

static void test_suite_teardown(void *f)
{
	int err;

	err = bt_hap_harc_cb_unregister(&cb);
	zassert_equal(0, err, "unexpected error %d", err);

	free(f);
}

ZTEST_SUITE(test_suite, NULL, test_suite_setup, test_suite_before, test_suite_after,
	    test_suite_teardown);

static struct test_device *test_device_new(struct test_suite_fixture *fixture)
{
	struct test_device *dev;

	dev = calloc(1, sizeof(*dev));
	dev->fixture = fixture;

	sys_slist_append(&fixture->devices, &dev->node);

	return dev;
}

static struct test_device *test_device_monaural_new(struct test_suite_fixture *fixture)
{
	struct test_device *dev = test_device_new(fixture);

	memcpy(&dev->conn, CONN(0, BT_CONN_ROLE_CENTRAL), sizeof(dev->conn));
	dev->type = BT_HAS_HEARING_AID_TYPE_MONAURAL;

	return dev;
}

ZTEST_F(test_suite, test_hap_harc_bind_monaural)
{
	struct test_device *dev = test_device_monaural_new(fixture);
	struct bt_hap_harc *harc;
	int err;

	err = bt_hap_harc_bind(&dev->conn, &harc);
	zassert_equal(0, err, "unexpected result %d", err);
	zassert_equal(1, bt_has_client_bind_fake.call_count);
	zassert_equal_ptr(&dev->conn, bt_has_client_bind_fake.arg0_val);
	fixture->client_cb->connected(&dev->client, 0);

	zassert_equal(1, bt_hap_harc_connected_cb_fake.call_count);
	zassert_equal_ptr(harc, bt_hap_harc_connected_cb_fake.arg0_val);
	zassert_equal(0, bt_hap_harc_connected_cb_fake.arg1_val);

	err = bt_hap_harc_unbind(harc);
	zassert_equal(0, err, "unexpected result %d", err);
	zassert_equal(1, bt_has_client_unbind_fake.call_count);
	zassert_equal_ptr(&dev->client, bt_has_client_unbind_fake.arg0_val);

	fixture->client_cb->disconnected(&dev->client);
	zassert_equal(1, bt_hap_harc_disconnected_cb_fake.call_count);
	zassert_equal_ptr(harc, bt_hap_harc_disconnected_cb_fake.arg0_val);

	fixture->client_cb->unbound(&dev->client, 0);
}

ZTEST_F(test_suite, test_hap_harc_bind_monaural_err)
{
	struct test_device *dev = test_device_monaural_new(fixture);
	struct bt_hap_harc *harc;
	int err;

	err = bt_hap_harc_bind(&dev->conn, &harc);
	zassert_equal(0, err, "unexpected result %d", err);
	zassert_equal(1, bt_has_client_bind_fake.call_count);
	zassert_equal_ptr(&dev->conn, bt_has_client_bind_fake.arg0_val);
	fixture->client_cb->connected(&dev->client, -ECONNABORTED);

	zassert_equal(1, bt_hap_harc_connected_cb_fake.call_count);
	zassert_equal_ptr(harc, bt_hap_harc_connected_cb_fake.arg0_val);
	zassert_not_equal(0, bt_hap_harc_connected_cb_fake.arg1_val);

	err = bt_hap_harc_unbind(harc);
	zassert_not_equal(0, err, "unexpected result %d", err);
}

#define SIRK_INIT(_set_id)                                                                         \
	{ 0xcd, 0xcc, 0x72, 0xdd, 0x86, 0x8c, 0xcd, 0xce,                                          \
	  0x22, 0xfd, 0xa1, 0x21, 0x09, 0x7d, 0x7d, (_set_id) }

#define SET_INFO_INIT(_set_id, _set_size, _rank, _lockable)                                        \
{                                                                                                  \
	.set_sirk = SIRK_INIT(_set_id),                                                            \
	.set_size = (_set_size),                                                                   \
	.rank = (_rank),                                                                           \
	.lockable = (_lockable),                                                                   \
}

#define SET_MEMBER_INIT(_set_id, _set_size, _rank, _lockable)                                      \
{                                                                                                  \
	.insts = {{ .info = SET_INFO_INIT(_set_id, _set_size, _rank, _lockable) }}                 \
}

static struct test_device *test_device_binaural_new(struct test_suite_fixture *fixture,
						    uint8_t index)
{
	struct test_device *dev = test_device_new(fixture);

	memcpy(&dev->conn, CONN(index, BT_CONN_ROLE_CENTRAL), sizeof(dev->conn));
	dev->type = BT_HAS_HEARING_AID_TYPE_BINAURAL;

	return dev;
}

ZTEST_F(test_suite, test_hap_harc_bind_binaural)
{
	const struct bt_csip_set_coordinator_set_member member = SET_MEMBER_INIT(1, 2, 1, false);
	struct test_device *dev = test_device_binaural_new(fixture, 0);
	struct bt_hap_harc *harc;
	int err;

	err = bt_hap_harc_bind(&dev->conn, &harc);
	zassert_equal(0, err, "unexpected result %d", err);
	zassert_equal(1, bt_has_client_bind_fake.call_count);
	zassert_equal_ptr(&dev->conn, bt_has_client_bind_fake.arg0_val);
	fixture->client_cb->connected(&dev->client, 0);

	zassert_equal(1, bt_csip_set_coordinator_discover_fake.call_count);
	zassert_equal_ptr(&dev->conn, bt_csip_set_coordinator_discover_fake.arg0_val);
	fixture->set_coordinator_cb->discover(&dev->conn, &member, 0, 1);

	zassert_equal(1, bt_hap_harc_connected_cb_fake.call_count);
	zassert_equal_ptr(harc, bt_hap_harc_connected_cb_fake.arg0_val);
	zassert_equal(0, bt_hap_harc_connected_cb_fake.arg1_val);

	err = bt_hap_harc_unbind(harc);
	zassert_equal(0, err, "unexpected result %d", err);
	zassert_equal(1, bt_has_client_unbind_fake.call_count);
	zassert_equal_ptr(&dev->client, bt_has_client_unbind_fake.arg0_val);

	fixture->client_cb->disconnected(&dev->client);
	zassert_equal(1, bt_hap_harc_disconnected_cb_fake.call_count);
	zassert_equal_ptr(harc, bt_hap_harc_disconnected_cb_fake.arg0_val);

	fixture->client_cb->unbound(&dev->client, 0);
}

ZTEST_F(test_suite, test_hap_harc_bind_binaural_err)
{
	struct test_device *dev = test_device_binaural_new(fixture, 0);
	struct bt_hap_harc *harc;
	int err;

	err = bt_hap_harc_bind(&dev->conn, &harc);
	zassert_equal(0, err, "unexpected result %d", err);
	zassert_equal(1, bt_has_client_bind_fake.call_count);
	zassert_equal_ptr(&dev->conn, bt_has_client_bind_fake.arg0_val);
	fixture->client_cb->connected(&dev->client, 0);

	zassert_equal(1, bt_csip_set_coordinator_discover_fake.call_count);
	zassert_equal_ptr(&dev->conn, bt_csip_set_coordinator_discover_fake.arg0_val);
	fixture->set_coordinator_cb->discover(&dev->conn, NULL, -ENOMEM, 0);

	zassert_equal(1, bt_hap_harc_connected_cb_fake.call_count);
	zassert_equal_ptr(harc, bt_hap_harc_connected_cb_fake.arg0_val);
	zassert_equal(-EAGAIN, bt_hap_harc_connected_cb_fake.arg1_val);

	err = bt_hap_harc_unbind(harc);
	zassert_not_equal(0, err, "unexpected result %d", err);
}

ZTEST_F(test_suite, test_hap_harc_bind_binaural_set)
{
	const struct bt_csip_set_coordinator_set_member member[2] = {
		SET_MEMBER_INIT(1, 2, 1, false),
		SET_MEMBER_INIT(1, 2, 2, false),
	};
	struct test_device *dev[2] = {
		test_device_binaural_new(fixture, 0),
		test_device_binaural_new(fixture, 1),
	};
	struct bt_hap_harc *harc[2] = { NULL };
	struct bt_hap_harc_info info = { 0 };
	int err;

	for (size_t i = 0; i < ARRAY_SIZE(member); i++) {
		err = bt_hap_harc_bind(&dev[i]->conn, &harc[i]);
		zassert_equal(0, err, "unexpected result %d", err);
		zassert_equal_ptr(&dev[i]->conn, bt_has_client_bind_fake.arg0_history[i]);
		fixture->client_cb->connected(&dev[i]->client, 0);

		zassert_equal_ptr(&dev[i]->conn,
				  bt_csip_set_coordinator_discover_fake.arg0_history[i]);
		fixture->set_coordinator_cb->discover(&dev[i]->conn, &member[i], 0, 1);

		zassert_equal_ptr(harc[i], bt_hap_harc_connected_cb_fake.arg0_history[i]);
		zassert_equal(0, bt_hap_harc_connected_cb_fake.arg1_history[i]);
	}

	err = bt_hap_harc_info_get(harc[0], &info);
	zassert_equal(0, err, "unexpected error %d", err);
	zassert_equal_ptr(info.binaural.pair, harc[1]);

	err = bt_hap_harc_info_get(harc[1], &info);
	zassert_equal(0, err, "unexpected error %d", err);
	zassert_equal_ptr(info.binaural.pair, harc[0]);

	for (size_t i = 0; i < ARRAY_SIZE(member); i++) {
		err = bt_hap_harc_unbind(harc[i]);
		zassert_equal(0, err, "unexpected result %d", err);

		fixture->client_cb->disconnected(&dev[i]->client);
		fixture->client_cb->unbound(&dev[i]->client, 0);
	}
}

ZTEST_F(test_suite, test_hap_harc_connect_retry)
{
	const struct bt_csip_set_coordinator_set_member member = SET_MEMBER_INIT(1, 2, 1, false);
	struct test_device *dev = test_device_binaural_new(fixture, 0);
	struct bt_hap_harc *harc;
	int err;

	err = bt_hap_harc_bind(&dev->conn, &harc);
	zassert_equal(0, err, "unexpected result %d", err);
	fixture->client_cb->connected(&dev->client, 0);
	fixture->set_coordinator_cb->discover(&dev->conn, NULL, -ENOMEM, 0);
	zassert_equal(-EAGAIN, bt_hap_harc_connected_cb_fake.arg1_val);

	/* Retry */
	err = bt_hap_harc_bind(&dev->conn, &harc);
	zassert_equal(0, err, "unexpected result %d", err);
	fixture->client_cb->connected(&dev->client, 0);
	fixture->set_coordinator_cb->discover(&dev->conn, &member, 0, 1);
	zassert_equal(0, bt_hap_harc_connected_cb_fake.arg1_val);

	err = bt_hap_harc_unbind(harc);
	zassert_equal(0, err, "unexpected result %d", err);
	fixture->client_cb->disconnected(&dev->client);
	fixture->client_cb->unbound(&dev->client, 0);
}

ZTEST_F(test_suite, test_hap_harc_bind_cancel)
{
	const struct bt_csip_set_coordinator_set_member member = SET_MEMBER_INIT(1, 2, 1, false);
	struct test_device *dev = test_device_binaural_new(fixture, 0);
	struct bt_hap_harc *harc;
	int err;

	/* #1 Cancel pending HAS binding */
	err = bt_hap_harc_bind(&dev->conn, &harc);
	zassert_equal(0, err, "unexpected result %d", err);

	err = bt_hap_harc_unbind(harc);
	zassert_equal(0, err, "unexpected result %d", err);

	fixture->client_cb->connected(&dev->client, -ECONNABORTED);

	zassert_equal(1, bt_hap_harc_connected_cb_fake.call_count);
	zassert_not_equal(0, bt_hap_harc_connected_cb_fake.arg1_val);
	HAP_HARC_CB_FAKES_LIST(RESET_FAKE);

	/* #2 Cancel pending CSIP discovery */
	err = bt_hap_harc_bind(&dev->conn, &harc);
	zassert_equal(0, err, "unexpected result %d", err);

	fixture->client_cb->connected(&dev->client, 0);
	/* CSIP discovery pending */

	err = bt_hap_harc_unbind(harc);
	zassert_equal(0, err, "unexpected result %d", err);

	fixture->client_cb->disconnected(&dev->client);

	zassert_equal(1, bt_hap_harc_connected_cb_fake.call_count);
	zassert_equal(-ECANCELED, bt_hap_harc_connected_cb_fake.arg1_val);

	fixture->client_cb->unbound(&dev->client, 0);

	HAP_HARC_CB_FAKES_LIST(RESET_FAKE);

	/* #3 Success */
	err = bt_hap_harc_bind(&dev->conn, &harc);
	zassert_equal(0, err, "unexpected result %d", err);

	fixture->client_cb->connected(&dev->client, 0);
	fixture->set_coordinator_cb->discover(&dev->conn, &member, 0, 1);

	err = bt_hap_harc_unbind(harc);
	zassert_equal(0, err, "unexpected result %d", err);

	fixture->client_cb->disconnected(&dev->client);

	zassert_equal(1, bt_hap_harc_connected_cb_fake.call_count);
	zassert_equal(0, bt_hap_harc_connected_cb_fake.arg1_val);
	zassert_equal(1, bt_hap_harc_disconnected_cb_fake.call_count);

	fixture->client_cb->unbound(&dev->client, 0);
}

ZTEST_F(test_suite, test_hap_harc_read_preset)
{
	struct test_device *dev = test_device_monaural_new(fixture);
	struct bt_hap_harc_preset_read_params params = {
		.complete = bt_hap_harc_complete_func,
		.start_index = 0x01,
		.max_count = 255,
	};
	struct bt_hap_harc *harc;
	int err;

	err = bt_hap_harc_preset_cb_register(&preset_cb);
	zassert_equal(0, err, "unexpected result %d", err);

	err = bt_hap_harc_bind(&dev->conn, &harc);
	zassert_equal(0, err, "unexpected result %d", err);

	fixture->client_cb->connected(&dev->client, 0);

	err = bt_hap_harc_preset_read(harc, &params);
	zassert_equal(0, err, "unexpected result %d", err);

	/* Fail - in progress */
	err = bt_hap_harc_preset_read(harc, &params);
	zassert_equal(-EBUSY, err, "unexpected result %d", err);

	fixture->client_cb->cmd_status(&dev->client, 0);

	struct bt_has_preset_record record_1 = {
		.index = 0x01,
		.properties = BT_HAS_PROP_WRITABLE | BT_HAS_PROP_AVAILABLE,
		.name = "record_1",
	};
	fixture->client_cb->preset_read_rsp(&dev->client, &record_1, false);

	zassert_equal(1, bt_hap_harc_preset_store_cb_fake.call_count);
	zassert_equal(harc, bt_hap_harc_preset_store_cb_fake.arg0_history[0]);
	zassert_equal(&record_1, bt_hap_harc_preset_store_cb_fake.arg1_history[0]);

	struct bt_has_preset_record record_5 = {
		.index = 0x05,
		.properties = BT_HAS_PROP_AVAILABLE,
		.name = "record_5",
	};
	fixture->client_cb->preset_read_rsp(&dev->client, &record_5, false);

	zassert_equal(2, bt_hap_harc_preset_store_cb_fake.call_count);
	zassert_equal(harc, bt_hap_harc_preset_store_cb_fake.arg0_history[1]);
	zassert_equal(&record_5, bt_hap_harc_preset_store_cb_fake.arg1_history[1]);

	struct bt_has_preset_record record_8 = {
		.index = 0x08,
		.properties = BT_HAS_PROP_AVAILABLE,
		.name = "record_8",
	};
	fixture->client_cb->preset_read_rsp(&dev->client, &record_8, true);

	zassert_equal(3, bt_hap_harc_preset_store_cb_fake.call_count);
	zassert_equal(harc, bt_hap_harc_preset_store_cb_fake.arg0_history[2]);
	zassert_equal(&record_8, bt_hap_harc_preset_store_cb_fake.arg1_history[2]);

	zassert_equal(1, bt_hap_harc_preset_commit_cb_fake.call_count);
	zassert_equal(1, bt_has_client_cmd_presets_read_fake.call_count);

	zassert_equal(1, bt_hap_harc_complete_func_fake.call_count);
	zassert_equal(0, bt_hap_harc_complete_func_fake.arg0_val);
	zassert_equal(&params, bt_hap_harc_complete_func_fake.arg1_val);

	err = bt_hap_harc_unbind(harc);
	zassert_equal(0, err, "unexpected result %d", err);
	fixture->client_cb->disconnected(&dev->client);
	fixture->client_cb->unbound(&dev->client, 0);
}
