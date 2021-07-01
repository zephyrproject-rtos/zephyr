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


#define LOG_MODULE_NAME test_persistence

#include <logging/log.h>
LOG_MODULE_REGISTER(LOG_MODULE_NAME);

#define WAIT_TIME 60 /*seconds*/

extern const struct bt_mesh_comp comp;

static uint16_t test_addr = 0x0123;
static uint8_t test_dev_uuid[16] = { 0x6c, 0x69, 0x6e, 0x67, 0x61, 0x6f };
static int test_ividx = 0x123456;
static uint8_t test_flags;
static uint8_t test_netkey_idx = 0x77;
static uint8_t test_netkey[16] = { 0xaa };
static uint8_t test_devkey[16] = { 0xdd };

static struct bt_mesh_prov prov = {
	.uuid = test_dev_uuid
};

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
				test_ividx, test_addr, test_devkey);

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

	tx.ctx->addr = test_addr;
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

	err = bt_mesh_cfg_ttl_get(test_netkey_idx, test_addr, &ttl);
	if (err) {
		FAIL("Failed to read ttl value");
	}

	/* verify IV index state */
	if (bt_mesh.iv_index != test_ividx ||
	    bt_mesh.ivu_duration != 96 ||
	    atomic_test_bit(bt_mesh.flags, BT_MESH_IVU_IN_PROGRESS)) {
		FAIL("IV loading verification failed");
	}

	k_sleep(K_SECONDS(CONFIG_BT_MESH_STORE_TIMEOUT));

	PASS();
}

#define TEST_CASE(role, name, description)		   \
	{						   \
		.test_id = "persistence_" #role "_" #name, \
		.test_descr = description,		   \
		.test_tick_f = bt_mesh_test_timeout,       \
		.test_main_f = test_##role##_##name,	   \
	}

static const struct bst_test_instance test_persistence[] = {
	TEST_CASE(provisioning, data_save, "Save provisioning data"),
	TEST_CASE(provisioning, data_load, "Load previously saved data and verify"),
	BSTEST_END_MARKER
};

struct bst_test_list *test_persistence_install(struct bst_test_list *tests)
{
	return bst_add_tests(tests, test_persistence);
}
