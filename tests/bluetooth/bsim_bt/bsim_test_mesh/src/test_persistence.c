/*
 * Copyright (c) 2021 Nordic Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "mesh_test.h"
#include "settings_test_backend.h"
#include <bluetooth/mesh.h>
#include <sys/reboot.h>
#include "mesh/net.h"
#include "mesh/app_keys.h"
#include "mesh/crypto.h"
#include <bs_cmd_line.h>

#define LOG_MODULE_NAME test_persistence

#include <logging/log.h>
LOG_MODULE_REGISTER(LOG_MODULE_NAME);

#define WAIT_TIME 60 /*seconds*/

extern const struct bt_mesh_comp comp;

struct test_va_t {
	uint16_t addr;
	uint8_t uuid[16];
};

struct test_appkey_t {
	uint8_t idx;
	uint8_t key[16];
};

#define TEST_ADDR 0x0123
static uint8_t test_dev_uuid[16] = { 0x6c, 0x69, 0x6e, 0x67, 0x61, 0x6f };
static int test_ividx = 0x123456;
static uint8_t test_flags;
static uint8_t test_netkey_idx = 0x77;
static uint8_t test_netkey[16] = { 0xaa };
static uint8_t test_devkey[16] = { 0xdd };

#define TEST_GROUP_0 0xc001
#define TEST_GROUP_1 0xfab3

#define TEST_VA_0_ADDR 0xb6f0
#define TEST_VA_0_UUID (uint8_t[16]) { 0xca, 0xcd, 0x13, 0xbd, 0x54, 0xfe, 0x43, 0xed, \
				       0x12, 0x3d, 0xa3, 0xe3, 0xb9, 0x03, 0x70, 0xaa }
#define TEST_VA_1_ADDR 0x8700
#define TEST_VA_1_UUID (uint8_t[16]) { 0xdf, 0xca, 0xa3, 0x54, 0x23, 0xfa, 0x33, 0xed, \
				       0x1a, 0xbe, 0xa0, 0xaa, 0xbd, 0xfa, 0x0f, 0xaf }

#define TEST_APPKEY_0_IDX 0x12
#define TEST_APPKEY_0_KEY { 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, \
			    0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f }
#define TEST_APPKEY_1_IDX 0x43
#define TEST_APPKEY_1_KEY { 0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, \
			    0x28, 0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f }

#define TEST_MOD_PUB_PARAMS {                                                                      \
		.addr = TEST_ADDR,                                                                 \
		.uuid = NULL,                                                                      \
		.app_idx = TEST_APPKEY_0_IDX,                                                      \
		.cred_flag = true,                                                                 \
		.ttl = 5,                                                                          \
		.period = BT_MESH_PUB_PERIOD_10SEC(2),                                             \
		.transmit = BT_MESH_TRANSMIT(2, 20),                                               \
	}

#define TEST_VND_MOD_PUB_PARAMS {                                                                  \
		.addr = TEST_VA_0_ADDR,                                                            \
		.uuid = TEST_VA_0_UUID,                                                            \
		.app_idx = TEST_APPKEY_1_IDX,                                                      \
		.cred_flag = true,                                                                 \
		.ttl = 5,                                                                          \
		.period = BT_MESH_PUB_PERIOD_10SEC(1),                                             \
		.transmit = BT_MESH_TRANSMIT(2, 20),                                               \
	}

#define DISABLED_MOD_PUB_PARAMS {                                                                  \
		.addr = 0,                                                                         \
		.uuid = NULL,                                                                      \
		.app_idx = 0,                                                                      \
		.cred_flag = false,                                                                \
		.ttl = 0,                                                                          \
		.period = 0,                                                                       \
		.transmit = 0,                                                                     \
	}

#define TEST_MOD_DATA_NAME "tmdata"
static uint8_t test_mod_data[] = { 0xfa, 0xff, 0xf4, 0x43 };
#define TEST_VND_MOD_DATA_NAME "vtmdata"
static uint8_t vnd_test_mod_data[] = { 0xad, 0xdf, 0x14, 0x53, 0x54, 0x1f };

static struct bt_mesh_prov prov = {
	.uuid = test_dev_uuid
};

static ssize_t test_preset = -1;
static struct {
	struct bt_mesh_cfg_mod_pub pub_params;

	size_t appkeys_count;
	uint16_t appkeys[CONFIG_BT_MESH_MODEL_KEY_COUNT];

	size_t subs_count;
	uint16_t subs[CONFIG_BT_MESH_MODEL_GROUP_COUNT];

	size_t mod_data_len;
} test_access_presets[][2] = {
	/* Default */
	{
		/* SIG model. */
		{
			.pub_params = TEST_MOD_PUB_PARAMS,
			.appkeys_count = 2, .appkeys = { TEST_APPKEY_0_IDX, TEST_APPKEY_1_IDX },
			.subs_count = 2, .subs = { TEST_GROUP_0, TEST_VA_0_ADDR },
			.mod_data_len = sizeof(test_mod_data),
		},

		/* Vendor model. */
		{
			.pub_params = TEST_VND_MOD_PUB_PARAMS,
			.appkeys_count = 2, .appkeys = { TEST_APPKEY_0_IDX, TEST_APPKEY_1_IDX },
			.subs_count = 2, .subs = { TEST_GROUP_0, TEST_VA_0_ADDR },
			.mod_data_len = sizeof(vnd_test_mod_data),
		},
	},

	/* After subscription overwrite */
	{
		/* SIG model. */
		{
			.pub_params = TEST_MOD_PUB_PARAMS,
			.appkeys_count = 2, .appkeys = { TEST_APPKEY_0_IDX, TEST_APPKEY_1_IDX },
			.subs_count = 1, .subs = { TEST_GROUP_0 },
			.mod_data_len = sizeof(test_mod_data),
		},

		/* Vendor model. */
		{
			.pub_params = TEST_VND_MOD_PUB_PARAMS,
			.appkeys_count = 2, .appkeys = { TEST_APPKEY_0_IDX, TEST_APPKEY_1_IDX },
			.subs_count = 1, .subs = { TEST_VA_0_ADDR },
			.mod_data_len = sizeof(vnd_test_mod_data),
		},
	},

	/* After remove */
	{
		/* SIG model. */
		{
			.pub_params = DISABLED_MOD_PUB_PARAMS,
			.appkeys_count = 0, .appkeys = {},
			.subs_count = 0, .subs = {},
			.mod_data_len = 0,
		},

		/* Vendor model. */
		{
			.pub_params = DISABLED_MOD_PUB_PARAMS,
			.appkeys_count = 0, .appkeys = {},
			.subs_count = 0, .subs = {},
			.mod_data_len = 0,
		},
	},
};

static const struct {
	uint8_t beacon;
	uint8_t ttl;
	uint8_t gatt_proxy;
	uint8_t friend;
	uint8_t net_transmit;
	struct {
		enum bt_mesh_feat_state state;
		uint8_t transmit;
	} relay;
} test_cfg_presets[] = {
	{
		.beacon = 1,
		.ttl = 12,
		.gatt_proxy = 1,
		.friend = 1,
		.net_transmit = BT_MESH_TRANSMIT(3, 20),
		.relay = { .state = BT_MESH_FEATURE_ENABLED, .transmit = BT_MESH_TRANSMIT(2, 20) },
	},
	{
		.beacon = 0,
		.ttl = 0,
		.gatt_proxy = 0,
		.friend = 0,
		.net_transmit = BT_MESH_TRANSMIT(1, 30),
		.relay = { .state = BT_MESH_FEATURE_ENABLED, .transmit = BT_MESH_TRANSMIT(1, 10) },
	},
};

static void test_args_parse(int argc, char *argv[])
{
	bs_args_struct_t args_struct[] = {
		{
			.dest = &test_preset,
			.type = 'u',
			.name = "{0, 1, 2}",
			.option = "test-preset",
			.descript = ""
		},
	};

	bs_args_parse_all_cmd_line(argc, argv, &args_struct);
}

static void check_mod_pub_params(struct bt_mesh_cfg_mod_pub *expected,
				 struct bt_mesh_cfg_mod_pub *got)
{
	ASSERT_EQUAL(expected->addr, got->addr);
	ASSERT_EQUAL(expected->app_idx, got->app_idx);
	ASSERT_EQUAL(expected->cred_flag, got->cred_flag);
	ASSERT_EQUAL(expected->ttl, got->ttl);
	ASSERT_EQUAL(expected->period, got->period);
	ASSERT_EQUAL(expected->transmit, got->transmit);
}

int test_model_settings_set(struct bt_mesh_model *model,
			    const char *name, size_t len_rd,
			    settings_read_cb read_cb, void *cb_arg)
{
	uint8_t data[sizeof(test_mod_data)];
	ssize_t result;

	ASSERT_TRUE(name != NULL);
	if (strncmp(name, TEST_MOD_DATA_NAME, strlen(TEST_MOD_DATA_NAME))) {
		FAIL("Invalid entry name: [%s]", name);
	}

	settings_name_next(name, &name);
	ASSERT_TRUE(name == NULL);

	ASSERT_TRUE(test_preset >= 0 && test_preset <= 2);
	result = read_cb(cb_arg, &data, sizeof(data));
	ASSERT_EQUAL(test_access_presets[test_preset][0].mod_data_len, result);

	if (memcmp(data, test_mod_data, test_access_presets[test_preset][0].mod_data_len)) {
		FAIL("Incorrect data restored");
	}

	return 0;
}

int test_vnd_model_settings_set(struct bt_mesh_model *model,
				const char *name, size_t len_rd,
				settings_read_cb read_cb, void *cb_arg)
{
	uint8_t data[sizeof(vnd_test_mod_data)];
	ssize_t result;

	ASSERT_TRUE(name != NULL);
	if (strncmp(name, TEST_VND_MOD_DATA_NAME, strlen(TEST_VND_MOD_DATA_NAME))) {
		FAIL("Invalid entry name: %s", name);
	}

	settings_name_next(name, &name);
	ASSERT_TRUE(name == NULL);

	ASSERT_TRUE(test_preset >= 0 && test_preset <= 2);
	result = read_cb(cb_arg, &data, sizeof(data));
	ASSERT_EQUAL(test_access_presets[test_preset][1].mod_data_len, result);

	if (memcmp(data, vnd_test_mod_data, test_access_presets[test_preset][1].mod_data_len)) {
		FAIL("Incorrect data restored");
	}

	return 0;
}

static int test_persistence_prov_setup(void)
{
	int err = bt_enable(NULL);

	if (err) {
		FAIL("Bluetooth init failed (err %d)", err);
		return err;
	}

	LOG_INF("Bluetooth initialized");

	err = bt_mesh_init(&prov, &comp);
	if (err) {
		FAIL("Initializing mesh failed (err %d)", err);
		return err;
	}

	settings_load();

	err = bt_mesh_provision(test_netkey, test_netkey_idx, test_flags,
				test_ividx, TEST_ADDR, test_devkey);

	return err;
}

static void test_provisioning_data_save(void)
{
	settings_test_backend_clear();
	bt_mesh_test_cfg_set(NULL, WAIT_TIME);

	if (test_persistence_prov_setup()) {
		FAIL("Mesh setup failed. Settings should not be loaded.");
	}

	k_sleep(K_SECONDS(CONFIG_BT_MESH_STORE_TIMEOUT));

	PASS();
}

static void test_provisioning_data_load(void)
{
	/* In this test stack should boot as provisioned */
	bt_mesh_test_cfg_set(NULL, WAIT_TIME);

	if (test_persistence_prov_setup() != -EALREADY) {
		FAIL("Device should boot up as already provisioned");
	}

	/* verify: */
	/* explicitly verify that the keys resolves for a given addr and net_idx */
	struct bt_mesh_msg_ctx ctx;
	struct bt_mesh_net_tx tx = { .ctx = &ctx };
	const uint8_t *dkey;
	uint8_t aid;

	tx.ctx->addr = TEST_ADDR;
	tx.ctx->net_idx = test_netkey_idx;
	tx.ctx->app_idx = BT_MESH_KEY_DEV_REMOTE;	/* to resolve devkey */

	int err = bt_mesh_keys_resolve(tx.ctx, &tx.sub, &dkey, &aid);

	if (err) {
		FAIL("Failed to resolve keys");
	}

	if (memcmp(dkey, test_devkey, sizeof(test_devkey))) {
		FAIL("Resolved dev_key does not match");
	}

	if (memcmp(tx.sub->keys[0].net, test_netkey, sizeof(test_netkey))) {
		FAIL("Resolved net_key does not match");
	}

	if (tx.sub->kr_phase != ((test_flags & 1) << 1)) {
		FAIL("Incorrect KR phase loaded");
	}

	/* send TTL Get to verify Tx/Rx path works with loaded config */
	uint8_t ttl;

	err = bt_mesh_cfg_ttl_get(test_netkey_idx, TEST_ADDR, &ttl);
	if (err) {
		FAIL("Failed to read ttl value");
	}

	/* verify IV index state */
	if (bt_mesh.iv_index != test_ividx ||
	    bt_mesh.ivu_duration != 0 ||
	    atomic_test_bit(bt_mesh.flags, BT_MESH_IVU_IN_PROGRESS)) {
		FAIL("IV loading verification failed");
	}

	k_sleep(K_SECONDS(CONFIG_BT_MESH_STORE_TIMEOUT));

	PASS();
}

static void test_access_data_save(void)
{
	int err;
	uint8_t status;
	uint16_t va;
	struct bt_mesh_cfg_mod_pub pub_params;

	settings_test_backend_clear();
	bt_mesh_test_cfg_set(NULL, WAIT_TIME);

	if (test_persistence_prov_setup()) {
		FAIL("Mesh setup failed. Settings should not be loaded.");
	}

	struct test_appkey_t test_appkeys[] = {
		{ .idx = TEST_APPKEY_0_IDX, .key = TEST_APPKEY_0_KEY },
		{ .idx = TEST_APPKEY_1_IDX, .key = TEST_APPKEY_1_KEY },
	};

	for (size_t i = 0; i < ARRAY_SIZE(test_appkeys); i++) {
		err = bt_mesh_cfg_app_key_add(test_netkey_idx, TEST_ADDR, test_netkey_idx,
					      test_appkeys[i].idx, test_appkeys[i].key, &status);
		if (err || status) {
			FAIL("AppKey add failed (err %d, status %u, i %d)", err, status, i);
		}
	}

	/* SIG model. */
	for (size_t i = 0; i < ARRAY_SIZE(test_appkeys); i++) {
		err = bt_mesh_cfg_mod_app_bind(test_netkey_idx, TEST_ADDR, TEST_ADDR,
					       test_appkeys[i].idx, TEST_MOD_ID, &status);
		if (err || status) {
			FAIL("Mod app bind failed (err %d, status %u, i %d)", err, status, i);
		}
	}

	err = bt_mesh_cfg_mod_sub_add(test_netkey_idx, TEST_ADDR, TEST_ADDR, TEST_GROUP_0,
				      TEST_MOD_ID, &status);
	if (err || status) {
		FAIL("Mod sub add failed (err %d, status %u)", err, status);
	}

	err = bt_mesh_cfg_mod_sub_va_add(test_netkey_idx, TEST_ADDR, TEST_ADDR, TEST_VA_0_UUID,
					 TEST_MOD_ID, &va, &status);
	if (err || status) {
		FAIL("Mod sub add failed (err %d, status %u)", err, status);
	}
	ASSERT_EQUAL(TEST_VA_0_ADDR, va);

	memcpy(&pub_params, &(struct bt_mesh_cfg_mod_pub)TEST_MOD_PUB_PARAMS,
	       sizeof(struct bt_mesh_cfg_mod_pub));
	err = bt_mesh_cfg_mod_pub_set(test_netkey_idx, TEST_ADDR, TEST_ADDR, TEST_MOD_ID,
				      &pub_params, &status);
	if (err || status) {
		FAIL("Mod pub set failed (err %d, status %u)", err, status);
	}

	err = bt_mesh_model_data_store(test_model, false, TEST_MOD_DATA_NAME, test_mod_data,
				       sizeof(test_mod_data));
	if (err) {
		FAIL("Mod data store failed (err %d)", err);
	}

	/* Vendor model. */
	for (size_t i = 0; i < ARRAY_SIZE(test_appkeys); i++) {
		err = bt_mesh_cfg_mod_app_bind_vnd(test_netkey_idx, TEST_ADDR, TEST_ADDR,
						   test_appkeys[i].idx, TEST_VND_MOD_ID,
						   TEST_VND_COMPANY_ID, &status);
		if (err || status) {
			FAIL("Mod app bind failed (err %d, status %u, i %d)", err, status, i);
		}
	}

	err = bt_mesh_cfg_mod_sub_add_vnd(test_netkey_idx, TEST_ADDR, TEST_ADDR,
					  TEST_GROUP_0, TEST_VND_MOD_ID,
					  TEST_VND_COMPANY_ID, &status);
	if (err || status) {
		FAIL("Mod sub add failed (err %d, status %u)", err, status);
	}

	err = bt_mesh_cfg_mod_sub_va_add_vnd(test_netkey_idx, TEST_ADDR, TEST_ADDR, TEST_VA_0_UUID,
					     TEST_VND_MOD_ID,
					     TEST_VND_COMPANY_ID, &va, &status);
	if (err || status) {
		FAIL("Mod sub add failed (err %d, status %u)", err, status);
	}

	ASSERT_EQUAL(TEST_VA_0_ADDR, va);

	memcpy(&pub_params, &(struct bt_mesh_cfg_mod_pub)TEST_VND_MOD_PUB_PARAMS,
	       sizeof(struct bt_mesh_cfg_mod_pub));
	err = bt_mesh_cfg_mod_pub_set_vnd(test_netkey_idx, TEST_ADDR, TEST_ADDR, TEST_VND_MOD_ID,
					  TEST_VND_COMPANY_ID, &pub_params, &status);
	if (err || status) {
		FAIL("Mod pub set failed (err %d, status %u)", err, status);
	}

	err = bt_mesh_model_data_store(test_vnd_model, true, TEST_VND_MOD_DATA_NAME,
				       vnd_test_mod_data, sizeof(vnd_test_mod_data));
	if (err) {
		FAIL("Vnd mod data store failed (err %d)", err);
		return;
	}

	k_sleep(K_SECONDS(CONFIG_BT_MESH_STORE_TIMEOUT));

	PASS();
}

static void test_access_data_load(void)
{
	uint16_t appkeys[CONFIG_BT_MESH_MODEL_KEY_COUNT + 1];
	size_t appkeys_count = ARRAY_SIZE(appkeys);
	uint16_t subs[CONFIG_BT_MESH_MODEL_GROUP_COUNT + 1];
	size_t subs_count = ARRAY_SIZE(subs);
	uint8_t status;
	int err;

	ASSERT_TRUE(test_preset >= 0 && test_preset <= 2);

	/* In this test stack should boot as provisioned */
	bt_mesh_test_cfg_set(NULL, WAIT_TIME);

	if (test_persistence_prov_setup() != -EALREADY) {
		FAIL("Device should boot up as already provisioned");
	}

	for (size_t m = 0; m < 2; m++) {
		bool vnd = m == 1;

		if (!vnd) {
			err = bt_mesh_cfg_mod_app_get(test_netkey_idx, TEST_ADDR, TEST_ADDR,
						      TEST_MOD_ID, &status, appkeys,
						      &appkeys_count);
		} else {
			err = bt_mesh_cfg_mod_app_get_vnd(test_netkey_idx, TEST_ADDR, TEST_ADDR,
							  TEST_VND_MOD_ID, TEST_VND_COMPANY_ID,
							  &status, appkeys, &appkeys_count);
		}
		if (err || status) {
			FAIL("Mod app get failed (err %d, status %u)", err, status);
		}

		ASSERT_EQUAL(test_access_presets[test_preset][m].appkeys_count, appkeys_count);
		for (size_t i = 0; i < appkeys_count; i++) {
			ASSERT_EQUAL(test_access_presets[test_preset][m].appkeys[i], appkeys[i]);
		}

		if (!vnd) {
			err = bt_mesh_cfg_mod_sub_get(test_netkey_idx, TEST_ADDR, TEST_ADDR,
						      TEST_MOD_ID, &status, subs, &subs_count);
		} else {
			err = bt_mesh_cfg_mod_sub_get_vnd(test_netkey_idx, TEST_ADDR, TEST_ADDR,
							  TEST_VND_MOD_ID, TEST_VND_COMPANY_ID,
							  &status, subs, &subs_count);
		}
		if (err || status) {
			FAIL("Mod sub get failed (err %d, status %u)", err, status);
		}

		ASSERT_EQUAL(test_access_presets[test_preset][m].subs_count, subs_count);
		for (size_t i = 0; i < subs_count; i++) {
			ASSERT_EQUAL(test_access_presets[test_preset][m].subs[i], subs[i]);
		}

		struct bt_mesh_cfg_mod_pub pub_params = {};

		if (!vnd) {
			err = bt_mesh_cfg_mod_pub_get(test_netkey_idx, TEST_ADDR, TEST_ADDR,
						      TEST_MOD_ID, &pub_params, &status);
		} else {
			err = bt_mesh_cfg_mod_pub_get_vnd(test_netkey_idx, TEST_ADDR, TEST_ADDR,
							  TEST_VND_MOD_ID, TEST_VND_COMPANY_ID,
							  &pub_params, &status);
		}
		if (err || status) {
			FAIL("Mod pub get failed (err %d, status %u)", err, status);
		}

		check_mod_pub_params(&test_access_presets[test_preset][m].pub_params, &pub_params);
	}

	PASS();
}

static void test_access_sub_overwrite(void)
{
	uint16_t va;
	uint8_t status;
	int err;

	/* In this test stack should boot as provisioned */
	bt_mesh_test_cfg_set(NULL, WAIT_TIME);

	if (test_persistence_prov_setup() != -EALREADY) {
		FAIL("Device should boot up as already provisioned");
	}

	err = bt_mesh_cfg_mod_sub_overwrite(test_netkey_idx, TEST_ADDR, TEST_ADDR,
					    TEST_GROUP_0, TEST_MOD_ID, &status);
	if (err || status) {
		FAIL("Mod sub overwrite failed (err %d, status %u)", err, status);
	}

	err = bt_mesh_cfg_mod_sub_va_overwrite_vnd(test_netkey_idx, TEST_ADDR, TEST_ADDR,
						   TEST_VA_0_UUID, TEST_VND_MOD_ID,
						   TEST_VND_COMPANY_ID, &va, &status);
	if (err || status) {
		FAIL("Mod sub va overwrite failed (err %d, status %u)", err, status);
	}
	ASSERT_EQUAL(TEST_VA_0_ADDR, va);

	k_sleep(K_SECONDS(CONFIG_BT_MESH_STORE_TIMEOUT));

	PASS();
}

static void test_access_data_remove(void)
{
	int err;
	uint8_t status;
	struct bt_mesh_cfg_mod_pub pub_params;

	/* In this test stack should boot as provisioned */
	bt_mesh_test_cfg_set(NULL, WAIT_TIME);

	if (test_persistence_prov_setup() != -EALREADY) {
		FAIL("Device should boot up as already provisioned");
	}

	struct test_appkey_t test_appkeys[] = {
		{ .idx = TEST_APPKEY_0_IDX, .key = TEST_APPKEY_0_KEY },
		{ .idx = TEST_APPKEY_1_IDX, .key = TEST_APPKEY_1_KEY },
	};

	/* SIG Model. */
	for (size_t i = 0; i < ARRAY_SIZE(test_appkeys); i++) {
		err = bt_mesh_cfg_mod_app_unbind(test_netkey_idx, TEST_ADDR, TEST_ADDR,
						 test_appkeys[i].idx, TEST_MOD_ID, &status);
		if (err || status) {
			FAIL("Mod app bind failed (err %d, status %u, i %d)", err, status, i);
		}
	}

	err = bt_mesh_cfg_mod_sub_del_all(test_netkey_idx, TEST_ADDR, TEST_ADDR, TEST_MOD_ID,
					  &status);
	if (err || status) {
		FAIL("Mod sub del all failed (err %d, status %u)", err, status);
	}

	memcpy(&pub_params, &(struct bt_mesh_cfg_mod_pub)TEST_MOD_PUB_PARAMS,
	       sizeof(struct bt_mesh_cfg_mod_pub));
	pub_params.addr = BT_MESH_ADDR_UNASSIGNED;
	err = bt_mesh_cfg_mod_pub_set(test_netkey_idx, TEST_ADDR, TEST_ADDR, TEST_MOD_ID,
				      &pub_params, &status);
	if (err || status) {
		FAIL("Mod pub set failed (err %d, status %u)", err, status);
	}

	err = bt_mesh_model_data_store(test_model, false, TEST_MOD_DATA_NAME, NULL, 0);
	if (err) {
		FAIL("Mod data erase failed (err %d)", err);
	}

	/* Vendor model. */
	for (size_t i = 0; i < ARRAY_SIZE(test_appkeys); i++) {
		err = bt_mesh_cfg_mod_app_unbind_vnd(test_netkey_idx, TEST_ADDR, TEST_ADDR,
						     test_appkeys[i].idx, TEST_VND_MOD_ID,
						     TEST_VND_COMPANY_ID, &status);
		if (err || status) {
			FAIL("Mod app bind failed (err %d, status %u, i %d)", err, status, i);
		}
	}

	err = bt_mesh_cfg_mod_sub_del_all_vnd(test_netkey_idx, TEST_ADDR, TEST_ADDR,
					      TEST_VND_MOD_ID, TEST_VND_COMPANY_ID,
					      &status);
	if (err || status) {
		FAIL("Mod sub del all failed (err %d, status %u)", err, status);
	}

	memcpy(&pub_params, &(struct bt_mesh_cfg_mod_pub)TEST_VND_MOD_PUB_PARAMS,
	       sizeof(struct bt_mesh_cfg_mod_pub));
	pub_params.addr = BT_MESH_ADDR_UNASSIGNED;
	pub_params.uuid = NULL;
	err = bt_mesh_cfg_mod_pub_set_vnd(test_netkey_idx, TEST_ADDR, TEST_ADDR, TEST_VND_MOD_ID,
					  TEST_VND_COMPANY_ID, &pub_params, &status);
	if (err || status) {
		FAIL("Mod pub set failed (err %d, status %u)", err, status);
	}

	err = bt_mesh_model_data_store(test_vnd_model, true, TEST_VND_MOD_DATA_NAME, NULL, 0);
	if (err) {
		FAIL("Vnd mod data erase failed (err %d)", err);
		return;
	}

	k_sleep(K_SECONDS(CONFIG_BT_MESH_STORE_TIMEOUT));

	PASS();
}

static void test_cfg_save(void)
{
	uint8_t transmit;
	uint8_t status;
	int err;

	ASSERT_TRUE(test_preset >= 0 && test_preset <= 1);

	settings_test_backend_clear();
	bt_mesh_test_cfg_set(NULL, WAIT_TIME);

	if (test_persistence_prov_setup()) {
		FAIL("Mesh setup failed. Settings should not be loaded.");
	}

	err = bt_mesh_cfg_beacon_set(test_netkey_idx, TEST_ADDR,
				     test_cfg_presets[test_preset].beacon, &status);
	if (err || status != test_cfg_presets[test_preset].beacon) {
		FAIL("Beacon set failed (err %d, status %u)", err, status);
	}

	err = bt_mesh_cfg_ttl_set(test_netkey_idx, TEST_ADDR,
				  test_cfg_presets[test_preset].ttl, &status);
	if (err || status != test_cfg_presets[test_preset].ttl) {
		FAIL("TTL set failed (err %d, status %u)", err, status);
	}

	err = bt_mesh_cfg_gatt_proxy_set(test_netkey_idx, TEST_ADDR,
					 test_cfg_presets[test_preset].gatt_proxy, &status);
	if (err || status != test_cfg_presets[test_preset].gatt_proxy) {
		FAIL("GATT Proxy set failed (err %d, status %u)", err, status);
	}

	err = bt_mesh_cfg_friend_set(test_netkey_idx, TEST_ADDR,
				     test_cfg_presets[test_preset].friend, &status);
	if (err || status != test_cfg_presets[test_preset].friend) {
		FAIL("Friend set failed (err %d, status %u)", err, status);
	}

	err = bt_mesh_cfg_net_transmit_set(test_netkey_idx, TEST_ADDR,
					   test_cfg_presets[test_preset].net_transmit,
					   &transmit);
	if (err || transmit != test_cfg_presets[test_preset].net_transmit) {
		FAIL("Net transmit set failed (err %d, trasmit %x)", err, transmit);
	}

	err = bt_mesh_cfg_relay_set(test_netkey_idx, TEST_ADDR,
				    test_cfg_presets[test_preset].relay.state,
				    test_cfg_presets[test_preset].relay.transmit,
				    &status, &transmit);
	if (err || status  != test_cfg_presets[test_preset].relay.state ||
	    transmit != test_cfg_presets[test_preset].relay.transmit) {
		FAIL("Relay set failed (err %d, status %u, transmit %x)", err, status,
		     test_cfg_presets[test_preset].relay.transmit);
	}

	k_sleep(K_SECONDS(CONFIG_BT_MESH_STORE_TIMEOUT));

	PASS();
}

static void test_cfg_load(void)
{
	uint8_t transmit;
	uint8_t status;
	int err;

	ASSERT_TRUE(test_preset >= 0 && test_preset <= 1);

	/* In this test stack should boot as provisioned */
	bt_mesh_test_cfg_set(NULL, WAIT_TIME);

	if (test_persistence_prov_setup() != -EALREADY) {
		FAIL("Device should boot up as already provisioned");
	}

	err = bt_mesh_cfg_beacon_get(test_netkey_idx, TEST_ADDR, &status);
	if (err || status != test_cfg_presets[test_preset].beacon) {
		FAIL("Beacon get failed (err %d, status %u)", err, status);
	}

	err = bt_mesh_cfg_ttl_get(test_netkey_idx, TEST_ADDR, &status);
	if (err || status != test_cfg_presets[test_preset].ttl) {
		FAIL("TTL get failed (err %d, status %u)", err, status);
	}

	err = bt_mesh_cfg_gatt_proxy_get(test_netkey_idx, TEST_ADDR, &status);
	if (err || status != test_cfg_presets[test_preset].gatt_proxy) {
		FAIL("GATT Proxy get failed (err %d, status %u)", err, status);
	}

	err = bt_mesh_cfg_friend_get(test_netkey_idx, TEST_ADDR, &status);
	if (err || status != test_cfg_presets[test_preset].friend) {
		FAIL("Friend get failed (err %d, status %u)", err, status);
	}

	err = bt_mesh_cfg_net_transmit_get(test_netkey_idx, TEST_ADDR, &status);
	if (err || status != test_cfg_presets[test_preset].net_transmit) {
		FAIL("Net transmit get failed (err %d, status %u)", err, status);
	}

	err = bt_mesh_cfg_relay_get(test_netkey_idx, TEST_ADDR, &status, &transmit);
	if (err || status != test_cfg_presets[test_preset].relay.state ||
	    transmit != test_cfg_presets[test_preset].relay.transmit) {
		FAIL("Relay get failed (err %d, state %u, transmit %x)", err, status, transmit);
	}

	PASS();
}

#define TEST_CASE(role, name, description)		   \
	{						   \
		.test_id = "persistence_" #role "_" #name, \
		.test_args_f = test_args_parse,		   \
		.test_descr = description,		   \
		.test_tick_f = bt_mesh_test_timeout,       \
		.test_main_f = test_##role##_##name,	   \
	}

static const struct bst_test_instance test_persistence[] = {
	TEST_CASE(provisioning, data_save, "Save provisioning data"),
	TEST_CASE(provisioning, data_load, "Load previously saved data and verify"),
	TEST_CASE(access, data_save, "Save access data"),
	TEST_CASE(access, data_load, "Load previously saved access data and verify"),
	TEST_CASE(access, sub_overwrite, "Overwrite Subscription List and store"),
	TEST_CASE(access, data_remove, "Remove stored access data"),
	TEST_CASE(cfg, save, "Save mesh configuration"),
	TEST_CASE(cfg, load, "Load previously stored mesh configuration and verify"),
	BSTEST_END_MARKER
};

struct bst_test_list *test_persistence_install(struct bst_test_list *tests)
{
	return bst_add_tests(tests, test_persistence);
}
