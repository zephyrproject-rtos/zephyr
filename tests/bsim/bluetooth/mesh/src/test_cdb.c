/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "mesh_test.h"
#include "mesh/cdb.h"
#include "mesh/net.h"
#include "mesh/app_keys.h"
#include "mesh/settings.h"
#include "mesh/foundation.h"

#define LOG_MODULE_NAME test_cdb
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(LOG_MODULE_NAME, LOG_LEVEL_INF);

#define WAIT_TIME 60
#define TEST_ADDR 0x0001

extern const struct bt_mesh_comp comp;

static uint8_t dev_uuid[16] = { 0x6c, 0x69, 0x6e, 0x67, 0x61, 0x6f };
static const uint8_t dev_key[16] = { 0x01, 0x02, 0x03, 0x04, 0x05 };

static uint8_t test_net_key_new[16] = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88,
					0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88};
static uint8_t test_app_key_new[16] = {0xa1, 0xb2, 0xc3, 0xd4, 0xe5, 0xf6, 0x07, 0x18,
					0xa1, 0xb2, 0xc3, 0xd4, 0xe5, 0xf6, 0x07, 0x18};

static void test_cdb_init(void)
{
	bt_mesh_test_cfg_set(NULL, WAIT_TIME);
}

static struct bt_mesh_prov prov = {
	.uuid = dev_uuid,
};

static void test_cdb_subnet_kr(void)
{
	struct bt_mesh_cdb_subnet *sub_cdb;
	struct bt_mesh_subnet *sub;
	uint8_t exported_key[16];
	uint8_t phase;
	uint8_t status;
	int err;

	bt_mesh_device_setup(&prov, &comp);
	ASSERT_OK(bt_mesh_cdb_create(test_net_key));
	ASSERT_OK(bt_mesh_provision(test_net_key, 0, 0, 0, TEST_ADDR, dev_key));

	/* Verify subnet was created in CDB */
	sub_cdb = bt_mesh_cdb_subnet_get(0);
	ASSERT_TRUE_MSG(sub_cdb != NULL, "CDB subnet not created");

	/* Verify key matches */
	err = bt_mesh_cdb_subnet_key_export(sub_cdb, 0, exported_key);
	ASSERT_OK(err);
	ASSERT_TRUE_MSG(memcmp(exported_key, test_net_key, 16) == 0,
			"Initial NetKey mismatch in CDB");

	/* Get the mesh subnet */
	sub = bt_mesh_subnet_get(0);
	ASSERT_TRUE_MSG(sub != NULL, "Mesh subnet not found");

	/* Update the NetKey in mesh core */
	err = bt_mesh_cfg_cli_net_key_update(0, TEST_ADDR, 0, test_net_key_new, &status);
	ASSERT_EQUAL(status, STATUS_SUCCESS);
	ASSERT_OK(err);

	/* Verify CDB was synchronized - key should be updated */
	sub_cdb = bt_mesh_cdb_subnet_get(0);
	ASSERT_TRUE_MSG(sub_cdb != NULL, "CDB subnet lost after update");
	ASSERT_EQUAL(sub_cdb->kr_phase, BT_MESH_KR_PHASE_1);

	/* Verify new key is in slot 1 */
	err = bt_mesh_cdb_subnet_key_export(sub_cdb, 1, exported_key);
	ASSERT_OK(err);
	ASSERT_TRUE_MSG(memcmp(exported_key, test_net_key_new, 16) == 0,
			"Updated NetKey mismatch in CDB");

	/* Swap keys (move to phase 2) */
	phase = BT_MESH_KR_PHASE_2;
	err = bt_mesh_cfg_cli_krp_set(0, TEST_ADDR, 0, 0x02, &status, &phase);
	ASSERT_EQUAL(status, STATUS_SUCCESS);
	ASSERT_OK(err);

	/* Verify CDB was synchronized - phase updated */
	sub_cdb = bt_mesh_cdb_subnet_get(0);
	ASSERT_EQUAL(sub_cdb->kr_phase, BT_MESH_KR_PHASE_2);

	/* Complete key refresh (move to phase 3 / normal) */
	phase = BT_MESH_KR_PHASE_3;
	err = bt_mesh_cfg_cli_krp_set(0, TEST_ADDR, 0, 0x03, NULL, &phase);
	ASSERT_OK(err);

	/* Verify CDB was synchronized - old key revoked, new key in slot 0 */
	sub_cdb = bt_mesh_cdb_subnet_get(0);
	ASSERT_EQUAL(sub_cdb->kr_phase, BT_MESH_KR_NORMAL);

	err = bt_mesh_cdb_subnet_key_export(sub_cdb, 0, exported_key);
	ASSERT_OK(err);
	ASSERT_TRUE_MSG(memcmp(exported_key, test_net_key_new, 16) == 0,
			"Final NetKey mismatch in CDB after KR");

	PASS();
}

static void test_cdb_appkey_kr(void)
{
	struct bt_mesh_cdb_app_key *app_cdb;
	uint8_t exported_key[16];
	uint8_t phase;
	uint8_t status;
	int err;

	bt_mesh_device_setup(&prov, &comp);
	ASSERT_OK(bt_mesh_cdb_create(test_net_key));
	ASSERT_OK(bt_mesh_provision(test_net_key, 0, 0, 0, TEST_ADDR, dev_key));

	/* Add AppKey to mesh core */
	err = bt_mesh_cfg_cli_app_key_add(0, TEST_ADDR, 0, 0, test_app_key, &status);
	ASSERT_EQUAL(status, STATUS_SUCCESS);
	ASSERT_OK(err);

	/* Verify AppKey was synchronized to CDB */
	app_cdb = bt_mesh_cdb_app_key_get(0);
	ASSERT_TRUE_MSG(app_cdb != NULL, "CDB AppKey not created");
	ASSERT_EQUAL(app_cdb->net_idx, 0);
	ASSERT_EQUAL(app_cdb->app_idx, 0);

	/* Verify key matches */
	err = bt_mesh_cdb_app_key_export(app_cdb, 0, exported_key);
	ASSERT_OK(err);
	ASSERT_TRUE_MSG(memcmp(exported_key, test_app_key, 16) == 0,
			"Initial AppKey mismatch in CDB");

	/* Update NetKey to start KR */
	err = bt_mesh_cfg_cli_net_key_update(0, TEST_ADDR, 0, test_net_key_new, &status);
	ASSERT_EQUAL(status, STATUS_SUCCESS);
	ASSERT_OK(err);

	/* Update AppKey */
	err = bt_mesh_cfg_cli_app_key_update(0, TEST_ADDR, 0, 0, test_app_key_new, &status);
	ASSERT_EQUAL(status, STATUS_SUCCESS);
	ASSERT_OK(err);

	/* Verify CDB was synchronized - new AppKey in slot 1 */
	app_cdb = bt_mesh_cdb_app_key_get(0);
	ASSERT_TRUE_MSG(app_cdb != NULL, "CDB AppKey lost after update");

	err = bt_mesh_cdb_app_key_export(app_cdb, 1, exported_key);
	ASSERT_OK(err);
	ASSERT_TRUE_MSG(memcmp(exported_key, test_app_key_new, 16) == 0,
			"Updated AppKey mismatch in CDB");

	/* Move subnet to phase 2 (swap keys) */
	phase = BT_MESH_KR_PHASE_2;
	err = bt_mesh_cfg_cli_krp_set(0, TEST_ADDR, 0, 0x02, &status, &phase);
	ASSERT_EQUAL(status, STATUS_SUCCESS);
	ASSERT_OK(err);

	/* Complete key refresh */
	phase = BT_MESH_KR_PHASE_3;
	err = bt_mesh_cfg_cli_krp_set(0, TEST_ADDR, 0, 0x03, &status, &phase);
	ASSERT_EQUAL(status, STATUS_SUCCESS);
	ASSERT_OK(err);

	/* Verify CDB AppKey revoked old key, new key in slot 0 */
	app_cdb = bt_mesh_cdb_app_key_get(0);
	ASSERT_TRUE_MSG(app_cdb != NULL, "CDB AppKey lost after KR");

	err = bt_mesh_cdb_app_key_export(app_cdb, 0, exported_key);
	ASSERT_OK(err);
	ASSERT_TRUE_MSG(memcmp(exported_key, test_app_key_new, 16) == 0,
			"Final AppKey mismatch in CDB after KR");

	PASS();
}

static void test_cdb_subnet_delete(void)
{
	struct bt_mesh_cdb_subnet *sub_cdb;
	uint8_t exported_key[16];
	uint8_t status;
	int err;

	bt_mesh_device_setup(&prov, &comp);
	ASSERT_OK(bt_mesh_cdb_create(test_net_key));
	ASSERT_OK(bt_mesh_provision(test_net_key, 0, 0, 0, TEST_ADDR, dev_key));

	/* Add one more subnet to mesh core */
	err = bt_mesh_cfg_cli_net_key_add(0, TEST_ADDR, 1, test_net_key_new, &status);
	ASSERT_EQUAL(status, STATUS_SUCCESS);
	ASSERT_OK(err);

	/* Verify subnet exists in CDB */
	sub_cdb = bt_mesh_cdb_subnet_get(1);
	ASSERT_TRUE_MSG(sub_cdb != NULL, "CDB subnet not created");

	/* Verify key matches */
	err = bt_mesh_cdb_subnet_key_export(sub_cdb, 0, exported_key);
	ASSERT_OK(err);
	ASSERT_TRUE_MSG(memcmp(exported_key, test_net_key_new, 16) == 0,
			"Initial NetKey mismatch in CDB");

	/* Delete subnet from mesh core */
	err = bt_mesh_cfg_cli_net_key_del(0, TEST_ADDR, 1, &status);
	ASSERT_EQUAL(status, STATUS_SUCCESS);
	ASSERT_OK(err);

	/* Verify subnet was removed from CDB */
	sub_cdb = bt_mesh_cdb_subnet_get(1);
	ASSERT_TRUE_MSG(sub_cdb == NULL, "CDB subnet not deleted");

	/* Add subnet with the same index as before to mesh core */
	err = bt_mesh_cfg_cli_net_key_add(0, TEST_ADDR, 1, test_net_key_new, &status);
	ASSERT_EQUAL(status, STATUS_SUCCESS);
	ASSERT_OK(err);

	/* Verify subnet exists in CDB */
	sub_cdb = bt_mesh_cdb_subnet_get(1);
	ASSERT_TRUE_MSG(sub_cdb != NULL, "CDB subnet not created");

	PASS();
}

static void test_cdb_appkey_delete(void)
{
	struct bt_mesh_cdb_app_key *app_cdb;
	uint8_t status;
	int err;

	bt_mesh_device_setup(&prov, &comp);
	ASSERT_OK(bt_mesh_cdb_create(test_net_key));
	ASSERT_OK(bt_mesh_provision(test_net_key, 0, 0, 0, TEST_ADDR, dev_key));

	/* Add AppKey */
	err = bt_mesh_cfg_cli_app_key_add(0, TEST_ADDR, 0, 0, test_app_key, &status);
	ASSERT_EQUAL(status, STATUS_SUCCESS);
	ASSERT_OK(err);

	/* Verify AppKey exists in CDB */
	app_cdb = bt_mesh_cdb_app_key_get(0);
	ASSERT_TRUE_MSG(app_cdb != NULL, "CDB AppKey not created");

	/* Delete AppKey from mesh core */
	err = bt_mesh_cfg_cli_app_key_del(0, TEST_ADDR, 0, 0, &status);
	ASSERT_EQUAL(status, STATUS_SUCCESS);
	ASSERT_OK(err);

	/* Verify AppKey was removed from CDB */
	app_cdb = bt_mesh_cdb_app_key_get(0);
	ASSERT_TRUE_MSG(app_cdb == NULL, "CDB AppKey not deleted");

	/* Add AppKey */
	err = bt_mesh_cfg_cli_app_key_add(0, TEST_ADDR, 0, 0, test_app_key, &status);
	ASSERT_EQUAL(status, STATUS_SUCCESS);
	ASSERT_OK(err);

	/* Verify AppKey exists in CDB */
	app_cdb = bt_mesh_cdb_app_key_get(0);
	ASSERT_TRUE_MSG(app_cdb != NULL, "CDB AppKey not created");

	PASS();
}

static void test_cdb_multiple_appkeys_kr(void)
{
	struct bt_mesh_cdb_app_key *app_cdb;
	uint8_t exported_key[16];
	uint8_t test_app_key2[16] = {0xbb, 0xcc, 0xdd, 0xee, 0xff, 0x00, 0x11, 0x22,
					 0xbb, 0xcc, 0xdd, 0xee, 0xff, 0x00, 0x11, 0x22};
	uint8_t test_app_key2_new[16] = {0xb1, 0xc2, 0xd3, 0xe4, 0xf5, 0x06, 0x17, 0x28,
					 0xb1, 0xc2, 0xd3, 0xe4, 0xf5, 0x06, 0x17, 0x28};
	uint8_t phase;
	uint8_t status;
	int err;

	bt_mesh_device_setup(&prov, &comp);
	ASSERT_OK(bt_mesh_cdb_create(test_net_key));
	ASSERT_OK(bt_mesh_provision(test_net_key, 0, 0, 0, TEST_ADDR, dev_key));

	/* Add multiple AppKeys */
	err = bt_mesh_cfg_cli_app_key_add(0, TEST_ADDR, 0, 0, test_app_key, &status);
	ASSERT_EQUAL(status, STATUS_SUCCESS);
	ASSERT_OK(err);
	err = bt_mesh_cfg_cli_app_key_add(0, TEST_ADDR, 0, 1, test_app_key2, &status);
	ASSERT_EQUAL(status, STATUS_SUCCESS);
	ASSERT_OK(err);

	/* Verify both AppKeys in CDB */
	app_cdb = bt_mesh_cdb_app_key_get(0);
	ASSERT_TRUE_MSG(app_cdb != NULL, "CDB AppKey 0 not created");
	app_cdb = bt_mesh_cdb_app_key_get(1);
	ASSERT_TRUE_MSG(app_cdb != NULL, "CDB AppKey 1 not created");

	/* Start NetKey refresh */
	err = bt_mesh_cfg_cli_net_key_update(0, TEST_ADDR, 0, test_net_key_new, &status);
	ASSERT_EQUAL(status, STATUS_SUCCESS);
	ASSERT_OK(err);

	/* Update both AppKeys */
	err = bt_mesh_cfg_cli_app_key_update(0, TEST_ADDR, 0, 0, test_app_key_new, &status);
	ASSERT_EQUAL(status, STATUS_SUCCESS);
	ASSERT_OK(err);
	err = bt_mesh_cfg_cli_app_key_update(0, TEST_ADDR, 0, 1, test_app_key2_new, &status);
	ASSERT_EQUAL(status, STATUS_SUCCESS);
	ASSERT_OK(err);

	/* Verify both AppKeys updated in CDB */
	app_cdb = bt_mesh_cdb_app_key_get(0);
	err = bt_mesh_cdb_app_key_export(app_cdb, 1, exported_key);
	ASSERT_OK(err);
	ASSERT_TRUE_MSG(memcmp(exported_key, test_app_key_new, 16) == 0,
			"AppKey 0 update mismatch");

	app_cdb = bt_mesh_cdb_app_key_get(1);
	err = bt_mesh_cdb_app_key_export(app_cdb, 1, exported_key);
	ASSERT_OK(err);
	ASSERT_TRUE_MSG(memcmp(exported_key, test_app_key2_new, 16) == 0,
			"AppKey 1 update mismatch");

	/* Complete key refresh */
	phase = BT_MESH_KR_PHASE_2;
	err = bt_mesh_cfg_cli_krp_set(0, TEST_ADDR, 0, 0x02, &status, &phase);
	ASSERT_EQUAL(status, STATUS_SUCCESS);
	ASSERT_OK(err);
	phase = BT_MESH_KR_PHASE_3;
	err = bt_mesh_cfg_cli_krp_set(0, TEST_ADDR, 0, 0x03, &status, &phase);
	ASSERT_EQUAL(status, STATUS_SUCCESS);
	ASSERT_OK(err);

	/* Verify both AppKeys have new keys in slot 0 */
	app_cdb = bt_mesh_cdb_app_key_get(0);
	err = bt_mesh_cdb_app_key_export(app_cdb, 0, exported_key);
	ASSERT_OK(err);
	ASSERT_TRUE_MSG(memcmp(exported_key, test_app_key_new, 16) == 0,
			"AppKey 0 final mismatch");

	app_cdb = bt_mesh_cdb_app_key_get(1);
	err = bt_mesh_cdb_app_key_export(app_cdb, 0, exported_key);
	ASSERT_OK(err);
	ASSERT_TRUE_MSG(memcmp(exported_key, test_app_key2_new, 16) == 0,
			"AppKey 1 final mismatch");

	PASS();
}

#define TEST_CASE(role, name, description)                     \
	{                                                       \
		.test_id = "cdb_" #role "_" #name,              \
		.test_descr = description,                      \
		.test_post_init_f = test_cdb_init,              \
		.test_tick_f = bt_mesh_test_timeout,            \
		.test_main_f = test_cdb_##name,                 \
	}

static const struct bst_test_instance test_cdb[] = {
	TEST_CASE(sync, subnet_kr, "CDB: Subnet KR synchronization"),
	TEST_CASE(sync, appkey_kr, "CDB: AppKey KR synchronization"),
	TEST_CASE(sync, subnet_delete, "CDB: Subnet deletion synchronization"),
	TEST_CASE(sync, appkey_delete, "CDB: AppKey deletion synchronization"),
	TEST_CASE(sync, multiple_appkeys_kr, "CDB: Multiple AppKeys KR synchronization"),
	BSTEST_END_MARKER
};

struct bst_test_list *test_cdb_install(struct bst_test_list *tests)
{
	tests = bst_add_tests(tests, test_cdb);
	return tests;
}
