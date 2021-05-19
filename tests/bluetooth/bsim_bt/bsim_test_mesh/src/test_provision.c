/*
 * Copyright (c) 2021 Lingao Meng
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include "mesh_test.h"

#include <sys/byteorder.h>

#define LOG_MODULE_NAME mesh_prov

#include <logging/log.h>
LOG_MODULE_REGISTER(LOG_MODULE_NAME);

/*
 * Provision layer tests:
 *   Tests both the provisioner and device role in various scenarios.
 */

#define PROV_MULTI_COUNT 3

enum test_flags {
	IS_PROVISIONER,

	TEST_FLAGS,
};

static ATOMIC_DEFINE(flags, TEST_FLAGS);
extern const struct bt_mesh_comp comp;
extern const uint8_t test_net_key[16];
extern uint global_device_nbr;

/* Timeout semaphore */
static struct k_sem prov_sem;
static uint16_t prov_addr = 0x0002;
static const uint8_t dev_key[16] = { 0x01, 0x02, 0x03, 0x04, 0x05 };
static uint8_t dev_uuid[16] = { 0x6c, 0x69, 0x6e, 0x67, 0x61, 0x6f };

#define WAIT_TIME 60 /*seconds*/

static void test_device_init(void)
{
	/* Ensure that the UUID is unique: */
	dev_uuid[6] = '0' + global_device_nbr;

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
	prov_addr++;
	k_sem_give(&prov_sem);
}

static struct bt_mesh_prov prov = {
	.uuid = dev_uuid,
	.unprovisioned_beacon = unprovisioned_beacon,
	.complete = prov_complete,
	.node_added = prov_node_added,
};

static void bt_mesh_device_setup(void)
{
	int err;

	err = bt_enable(NULL);
	if (err) {
		FAIL("Bluetooth init failed (err %d)", err);
		return;
	}

	LOG_INF("Bluetooth initialized");

	err = bt_mesh_init(&prov, &comp);
	if (err) {
		FAIL("Initializing mesh failed (err %d)", err);
		return;
	}
}

/** @brief Verify that this device pb-adv provision.
 */
static void test_device_pb_adv_no_oob(void)
{
	int err;

	k_sem_init(&prov_sem, 0, 1);

	bt_mesh_device_setup();

	err = bt_mesh_prov_enable(BT_MESH_PROV_ADV);
	ASSERT_OK(err, "Device PB-ADV Enable failed (err %d)", err);

	LOG_INF("Mesh initialized\n");

	/* Keep a long timeout so the prov multi case has time to finish: */
	ASSERT_OK(k_sem_take(&prov_sem, K_SECONDS(40)),
		  "Device provision fail");

	PASS();
}

/** @brief Verify that this provisioner pb-adv provision.
 */
static void test_provisioner_pb_adv_no_oob(void)
{
	int err;

	k_sem_init(&prov_sem, 0, 1);

	bt_mesh_device_setup();

	err = bt_mesh_cdb_create(test_net_key);
	ASSERT_OK(err, "Failed to create CDB (err %d)\n", err);

	err = bt_mesh_provision(test_net_key, 0, 0, 0, 0x0001, dev_key);
	ASSERT_OK(err, "Provisioning failed (err %d)", err);

	ASSERT_OK(k_sem_take(&prov_sem, K_SECONDS(5)),
		  "Provisioner provision fail");

	PASS();
}

/** @brief Verify that the provisioner can provision multiple devices in a row
 */
static void test_provisioner_pb_adv_multi(void)
{
	int err;

	k_sem_init(&prov_sem, 0, 1);

	bt_mesh_device_setup();

	err = bt_mesh_cdb_create(test_net_key);
	ASSERT_OK(err, "Failed to create CDB (err %d)\n", err);

	err = bt_mesh_provision(test_net_key, 0, 0, 0, 0x0001, dev_key);
	ASSERT_OK(err, "Provisioning failed (err %d)", err);

	for (int i = 0; i < PROV_MULTI_COUNT; i++) {
		ASSERT_OK(k_sem_take(&prov_sem, K_SECONDS(20)),
			  "Provisioner provision #%d fail", i);
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

	TEST_CASE(provisioner, pb_adv_no_oob,
		  "Provisioner: pb-adv provisioning use no-oob method"),
	TEST_CASE(provisioner, pb_adv_multi,
		  "Provisioner: pb-adv provisioning multiple devices"),

	BSTEST_END_MARKER
};

struct bst_test_list *test_provision_install(struct bst_test_list *tests)
{
	tests = bst_add_tests(tests, test_connect);
	return tests;
}
