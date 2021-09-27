/*
 * Copyright (c) 2021 Lingao Meng
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include "mesh_test.h"
#include "mesh/net.h"
#include "argparse.h"

#include <sys/byteorder.h>

#define LOG_MODULE_NAME mesh_prov

#include <logging/log.h>
LOG_MODULE_REGISTER(LOG_MODULE_NAME);

/*
 * Provision layer tests:
 *   Tests both the provisioner and device role in various scenarios.
 */

#define PROV_MULTI_COUNT 3
#define PROV_REPROV_COUNT 10

enum test_flags {
	IS_PROVISIONER,

	TEST_FLAGS,
};

static ATOMIC_DEFINE(flags, TEST_FLAGS);
extern const struct bt_mesh_comp comp;
extern const uint8_t test_net_key[16];
extern const uint8_t test_app_key[16];

/* Timeout semaphore */
static struct k_sem prov_sem;
static uint16_t prov_addr = 0x0002;
static uint16_t current_dev_addr;
static const uint8_t dev_key[16] = { 0x01, 0x02, 0x03, 0x04, 0x05 };
static uint8_t dev_uuid[16] = { 0x6c, 0x69, 0x6e, 0x67, 0x61, 0x6f };

#define WAIT_TIME 60 /*seconds*/

static void test_device_init(void)
{
	/* Ensure that the UUID is unique: */
	dev_uuid[6] = '0' + get_device_nbr();

	bt_mesh_test_cfg_set(NULL, WAIT_TIME);
}

static void test_provisioner_init(void)
{
	atomic_set_bit(flags, IS_PROVISIONER);
	bt_mesh_test_cfg_set(NULL, WAIT_TIME);
}

static void unprovisioned_beacon(uint8_t uuid[16],
				 bt_mesh_prov_oob_info_t oob_info,
				 uint32_t *uri_hash)
{
	if (!atomic_test_bit(flags, IS_PROVISIONER)) {
		return;
	}

	bt_mesh_provision_adv(uuid, 0, prov_addr, 0);
}

static void prov_complete(uint16_t net_idx, uint16_t addr)
{
	if (!atomic_test_bit(flags, IS_PROVISIONER)) {
		k_sem_give(&prov_sem);
	}
}

static void prov_node_added(uint16_t net_idx, uint8_t uuid[16], uint16_t addr,
			    uint8_t num_elem)
{
	LOG_INF("Device 0x%04x provisioned", prov_addr);
	current_dev_addr = prov_addr++;
	k_sem_give(&prov_sem);
}

static void prov_reset(void)
{
	ASSERT_OK(bt_mesh_prov_enable(BT_MESH_PROV_ADV));
}

static struct bt_mesh_prov prov = {
	.uuid = dev_uuid,
	.unprovisioned_beacon = unprovisioned_beacon,
	.complete = prov_complete,
	.node_added = prov_node_added,
	.reset = prov_reset,
};

/** @brief Verify that this device pb-adv provision.
 */
static void test_device_pb_adv_no_oob(void)
{
	k_sem_init(&prov_sem, 0, 1);

	bt_mesh_device_setup(&prov, &comp);

	ASSERT_OK(bt_mesh_prov_enable(BT_MESH_PROV_ADV));

	LOG_INF("Mesh initialized\n");

	/* Keep a long timeout so the prov multi case has time to finish: */
	ASSERT_OK(k_sem_take(&prov_sem, K_SECONDS(40)));

	PASS();
}

/** @brief Verify that this device can be reprovisioned after resets
 */
static void test_device_pb_adv_reprovision(void)
{
	k_sem_init(&prov_sem, 0, 1);

	bt_mesh_device_setup(&prov, &comp);

	ASSERT_OK(bt_mesh_prov_enable(BT_MESH_PROV_ADV));

	LOG_INF("Mesh initialized\n");

	for (int i = 0; i < PROV_REPROV_COUNT; i++) {
		/* Keep a long timeout so the prov multi case has time to finish: */
		LOG_INF("Dev prov loop #%d, waiting for prov ...\n", i);
		ASSERT_OK(k_sem_take(&prov_sem, K_SECONDS(5)));
	}

	PASS();
}

/** @brief Verify that this provisioner pb-adv provision.
 */
static void test_provisioner_pb_adv_no_oob(void)
{
	k_sem_init(&prov_sem, 0, 1);

	bt_mesh_device_setup(&prov, &comp);

	ASSERT_OK(bt_mesh_cdb_create(test_net_key));

	ASSERT_OK(bt_mesh_provision(test_net_key, 0, 0, 0, 0x0001, dev_key));

	ASSERT_OK(k_sem_take(&prov_sem, K_SECONDS(5)));

	PASS();
}

/** @brief Verify that the provisioner can provision multiple devices in a row
 */
static void test_provisioner_pb_adv_multi(void)
{
	k_sem_init(&prov_sem, 0, 1);

	bt_mesh_device_setup(&prov, &comp);

	ASSERT_OK(bt_mesh_cdb_create(test_net_key));

	ASSERT_OK(bt_mesh_provision(test_net_key, 0, 0, 0, 0x0001, dev_key));

	for (int i = 0; i < PROV_MULTI_COUNT; i++) {
		ASSERT_OK(k_sem_take(&prov_sem, K_SECONDS(20)));
	}

	PASS();
}

/** @brief Verify that when the IV Update flag is set to zero at the
 * time of provisioning, internal IV update counter is also zero.
 */
static void test_provisioner_iv_update_flag_zero(void)
{
	uint8_t flags = 0x00;

	bt_mesh_device_setup(&prov, &comp);

	ASSERT_OK(bt_mesh_provision(test_net_key, 0, flags, 0, 0x0001, dev_key));

	if (bt_mesh.ivu_duration != 0) {
		FAIL("IV Update duration counter is not 0 when IV Update flag is zero");
	}

	PASS();
}

/** @brief Verify that when the IV Update flag is set to one at the
 * time of provisioning, internal IV update counter is set to 96 hours.
 */
static void test_provisioner_iv_update_flag_one(void)
{
	uint8_t flags = 0x02; /* IV Update flag bit set to 1 */

	bt_mesh_device_setup(&prov, &comp);

	ASSERT_OK(bt_mesh_provision(test_net_key, 0, flags, 0, 0x0001, dev_key));

	if (bt_mesh.ivu_duration != 96) {
		FAIL("IV Update duration counter is not 96 when IV Update flag is one");
	}

	bt_mesh_reset();

	if (bt_mesh.ivu_duration != 0) {
		FAIL("IV Update duration counter is not reset to 0");
	}

	PASS();
}

/** @brief Verify that the provisioner can provision a device multiple times after resets
 */
static void test_provisioner_pb_adv_reprovision(void)
{
	k_sem_init(&prov_sem, 0, 1);

	bt_mesh_device_setup(&prov, &comp);

	ASSERT_OK(bt_mesh_cdb_create(test_net_key));

	ASSERT_OK(bt_mesh_provision(test_net_key, 0, 0, 0, 0x0001, dev_key));

	for (int i = 0; i < PROV_REPROV_COUNT; i++) {
		LOG_INF("Provisioner prov loop #%d, waiting for prov ...\n", i);
		ASSERT_OK(k_sem_take(&prov_sem, K_SECONDS(20)));

		uint8_t status;
		size_t subs_count = 1;
		uint16_t sub;
		struct bt_mesh_cfg_mod_pub healthpub = { 0 };
		struct bt_mesh_cdb_node *node;

		/* Check that publication and subscription are reset after last iteration */
		ASSERT_OK(bt_mesh_cfg_mod_sub_get(0, current_dev_addr, current_dev_addr,
						  BT_MESH_MODEL_ID_HEALTH_SRV, &status, &sub,
						  &subs_count));
		ASSERT_EQUAL(0, status);
		ASSERT_TRUE(subs_count == 0);

		ASSERT_OK(bt_mesh_cfg_mod_pub_get(0, current_dev_addr, current_dev_addr,
						  BT_MESH_MODEL_ID_HEALTH_SRV, &healthpub,
						  &status));
		ASSERT_EQUAL(0, status);
		ASSERT_TRUE(healthpub.addr == BT_MESH_ADDR_UNASSIGNED, "Pub not cleared");


		/* Set pub and sub to check that they are reset */
		healthpub.addr = 0xc001;
		healthpub.app_idx = 0;
		healthpub.cred_flag = false;
		healthpub.ttl = 10;
		healthpub.period = BT_MESH_PUB_PERIOD_10SEC(1);
		healthpub.transmit = BT_MESH_TRANSMIT(3, 100);

		ASSERT_OK(bt_mesh_cfg_app_key_add(0, current_dev_addr, 0, 0, test_app_key,
						  &status));
		ASSERT_EQUAL(0, status);

		ASSERT_OK(bt_mesh_cfg_mod_app_bind(0, current_dev_addr, current_dev_addr, 0x0,
						   BT_MESH_MODEL_ID_HEALTH_SRV, &status));
		ASSERT_EQUAL(0, status);

		ASSERT_OK(bt_mesh_cfg_mod_sub_add(0, current_dev_addr, current_dev_addr, 0xc000,
						  BT_MESH_MODEL_ID_HEALTH_SRV, &status));
		ASSERT_EQUAL(0, status);

		ASSERT_OK(bt_mesh_cfg_mod_pub_set(0, current_dev_addr, current_dev_addr,
						  BT_MESH_MODEL_ID_HEALTH_SRV, &healthpub,
						  &status));
		ASSERT_EQUAL(0, status);

		ASSERT_OK(bt_mesh_cfg_node_reset(0, current_dev_addr, (bool *)&status));

		node = bt_mesh_cdb_node_get(current_dev_addr);
		bt_mesh_cdb_node_del(node, true);
	}

	PASS();
}

#define TEST_CASE(role, name, description)                                     \
	{                                                                      \
		.test_id = "prov_" #role "_" #name, .test_descr = description, \
		.test_post_init_f = test_##role##_init,                        \
		.test_tick_f = bt_mesh_test_timeout,                           \
		.test_main_f = test_##role##_##name,                           \
	}

static const struct bst_test_instance test_connect[] = {
	TEST_CASE(device, pb_adv_no_oob,
		  "Device: pb-adv provisioning use no-oob method"),
	TEST_CASE(device, pb_adv_reprovision,
		  "Device: pb-adv provisioning, reprovision"),
	TEST_CASE(provisioner, pb_adv_no_oob,
		  "Provisioner: pb-adv provisioning use no-oob method"),
	TEST_CASE(provisioner, pb_adv_multi,
		  "Provisioner: pb-adv provisioning multiple devices"),
	TEST_CASE(provisioner, iv_update_flag_zero,
		  "Provisioner: effect on ivu_duration when IV Update flag is set to zero"),
	TEST_CASE(provisioner, iv_update_flag_one,
		  "Provisioner: effect on ivu_duration when IV Update flag is set to one"),
	TEST_CASE(provisioner, pb_adv_reprovision,
		  "Provisioner: pb-adv provisioning, resetting and reprovisioning multiple times."),

	BSTEST_END_MARKER
};

struct bst_test_list *test_provision_install(struct bst_test_list *tests)
{
	tests = bst_add_tests(tests, test_connect);
	return tests;
}
