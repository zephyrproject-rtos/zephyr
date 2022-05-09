/*
 * Copyright (c) 2021 Lingao Meng
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <stdlib.h>

#include "mesh_test.h"
#include "mesh/net.h"
#include "argparse.h"
#include <bs_pc_backchannel.h>
#include <time_machine.h>

#include <tinycrypt/constants.h>
#include <tinycrypt/ecc.h>
#include <tinycrypt/ecc_dh.h>

#include <zephyr/sys/byteorder.h>

#define LOG_MODULE_NAME mesh_prov

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(LOG_MODULE_NAME);

/*
 * Provision layer tests:
 *   Tests both the provisioner and device role in various scenarios.
 */

#define PROV_MULTI_COUNT 3
#define PROV_REPROV_COUNT 3
#define WAIT_TIME 80 /*seconds*/

enum test_flags {
	IS_PROVISIONER,

	TEST_FLAGS,
};

static uint8_t static_key1[] = {0x6E, 0x6F, 0x72, 0x64, 0x69, 0x63, 0x5F,
		0x65, 0x78, 0x61, 0x6D, 0x70, 0x6C, 0x65, 0x5F, 0x31};
static uint8_t static_key2[] = {0x6E, 0x6F, 0x72, 0x64, 0x69, 0x63, 0x5F};

static uint8_t private_key_be[64];
static uint8_t public_key_be[64];

static struct oob_auth_test_vector_s {
	const uint8_t *static_val;
	uint8_t        static_val_len;
	uint8_t        output_size;
	uint16_t       output_actions;
	uint8_t        input_size;
	uint16_t       input_actions;
} oob_auth_test_vector[] = {
	{NULL, 0, 0, 0, 0, 0},
	{static_key1, sizeof(static_key1), 0, 0, 0, 0},
	{static_key2, sizeof(static_key2), 0, 0, 0, 0},
	{NULL, 0, 3, BT_MESH_BLINK, 0, 0},
	{NULL, 0, 5, BT_MESH_BEEP, 0, 0},
	{NULL, 0, 6, BT_MESH_VIBRATE, 0, 0},
	{NULL, 0, 7, BT_MESH_DISPLAY_NUMBER, 0, 0},
	{NULL, 0, 8, BT_MESH_DISPLAY_STRING, 0, 0},
	{NULL, 0, 0, 0, 4, BT_MESH_PUSH},
	{NULL, 0, 0, 0, 5, BT_MESH_TWIST},
	{NULL, 0, 0, 0, 8, BT_MESH_ENTER_NUMBER},
	{NULL, 0, 0, 0, 7, BT_MESH_ENTER_STRING},
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

/* Delayed work to avoid requesting OOB info before generation of this. */
static struct k_work_delayable oob_timer;
static void delayed_input(struct k_work *work);

static uint *oob_channel_id;
static bool is_oob_auth;

static void test_device_init(void)
{
	/* Ensure those test devices will not drift more than
	 * 100ms for each other in emulated time
	 */
	tm_set_phy_max_resync_offset(100000);

	/* Ensure that the UUID is unique: */
	dev_uuid[6] = '0' + get_device_nbr();

	bt_mesh_test_cfg_set(NULL, WAIT_TIME);
	k_work_init_delayable(&oob_timer, delayed_input);
}

static void test_provisioner_init(void)
{
	/* Ensure those test devices will not drift more than
	 * 100ms for each other in emulated time
	 */
	tm_set_phy_max_resync_offset(100000);

	atomic_set_bit(flags, IS_PROVISIONER);
	bt_mesh_test_cfg_set(NULL, WAIT_TIME);
	k_work_init_delayable(&oob_timer, delayed_input);
}

static void test_terminate(void)
{
	if (oob_channel_id) {
		bs_clean_back_channels();
	}
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

static bt_mesh_input_action_t gact;
static uint8_t gsize;
static int input(bt_mesh_input_action_t act, uint8_t size)
{
	/* The test system requests the input OOB data earlier than
	 * the output OOB is generated. Need to release context here
	 * to allow output OOB creation. OOB will be inserted later
	 * after the delay.
	 */
	gact = act;
	gsize = size;

	k_work_reschedule(&oob_timer, K_SECONDS(1));

	return 0;
}

static void delayed_input(struct k_work *work)
{
	char oob_str[16];
	uint32_t oob_number;
	int size = bs_bc_is_msg_received(*oob_channel_id);

	if (size <= 0) {
		FAIL("OOB data is not gotten");
	}

	switch (gact) {
	case BT_MESH_PUSH:
	case BT_MESH_TWIST:
	case BT_MESH_ENTER_NUMBER:
		ASSERT_TRUE(size == sizeof(uint32_t));
		bs_bc_receive_msg(*oob_channel_id, (uint8_t *)&oob_number, size);
		ASSERT_OK(bt_mesh_input_number(oob_number));
		break;
	case BT_MESH_ENTER_STRING:
		bs_bc_receive_msg(*oob_channel_id, (uint8_t *)oob_str, size);
		ASSERT_OK(bt_mesh_input_string(oob_str));
		break;
	default:
		FAIL("Unknown input action %u (size %u) requested!", gact, gsize);
	}
}

static void prov_input_complete(void)
{
	LOG_INF("Input OOB data completed");
}

static int output_number(bt_mesh_output_action_t action, uint32_t number);
static int output_string(const char *str);
static void capabilities(const struct bt_mesh_dev_capabilities *cap);
static struct bt_mesh_prov prov = {
	.uuid = dev_uuid,
	.unprovisioned_beacon = unprovisioned_beacon,
	.complete = prov_complete,
	.node_added = prov_node_added,
	.output_number = output_number,
	.output_string = output_string,
	.input = input,
	.input_complete = prov_input_complete,
	.capabilities = capabilities,
	.reset = prov_reset,
};

static int output_number(bt_mesh_output_action_t action, uint32_t number)
{
	LOG_INF("OOB Number: %u", number);

	bs_bc_send_msg(*oob_channel_id, (uint8_t *)&number, sizeof(uint32_t));
	return 0;
}

static int output_string(const char *str)
{
	LOG_INF("OOB String: %s", str);

	bs_bc_send_msg(*oob_channel_id, (uint8_t *)str, strlen(str) + 1);
	return 0;
}

static void capabilities(const struct bt_mesh_dev_capabilities *cap)
{
	if (cap->static_oob) {
		LOG_INF("Static OOB authentication");
		ASSERT_OK(bt_mesh_auth_method_set_static(prov.static_val, prov.static_val_len));
	} else if (cap->output_actions) {
		LOG_INF("Output OOB authentication");
		ASSERT_OK(bt_mesh_auth_method_set_output(prov.output_actions, prov.output_size));
	} else if (cap->input_actions) {
		LOG_INF("Input OOB authentication");
		ASSERT_OK(bt_mesh_auth_method_set_input(prov.input_actions, prov.input_size));
	} else if (!is_oob_auth) {
		bt_mesh_auth_method_set_none();
	} else {
		FAIL("No OOB in capability frame");
	}
}

static void oob_auth_set(int test_step)
{
	struct oob_auth_test_vector_s dummy = {0};

	ASSERT_TRUE(test_step < ARRAY_SIZE(oob_auth_test_vector));

	if (memcmp(&oob_auth_test_vector[test_step], &dummy,
		sizeof(struct oob_auth_test_vector_s))) {
		is_oob_auth = true;
	} else {
		is_oob_auth = false;
	}

	prov.static_val = oob_auth_test_vector[test_step].static_val;
	prov.static_val_len = oob_auth_test_vector[test_step].static_val_len;
	prov.output_size = oob_auth_test_vector[test_step].output_size;
	prov.output_actions = oob_auth_test_vector[test_step].output_actions;
	prov.input_size = oob_auth_test_vector[test_step].input_size;
	prov.input_actions = oob_auth_test_vector[test_step].input_actions;
}
static void oob_device(bool use_oob_pk)
{
	int err = 0;

	k_sem_init(&prov_sem, 0, 1);

	oob_channel_id = bs_open_back_channel(get_device_nbr(),
			(uint[]){(get_device_nbr() + 1) % 2}, (uint[]){0}, 1);
	if (!oob_channel_id) {
		FAIL("Can't open OOB interface (err %d)\n", err);
	}

	bt_mesh_device_setup(&prov, &comp);

	if (use_oob_pk) {
		ASSERT_TRUE(uECC_make_key(public_key_be, private_key_be, uECC_secp256r1()));
		prov.public_key_be = public_key_be;
		prov.private_key_be = private_key_be;
		bs_bc_send_msg(*oob_channel_id, public_key_be, 64);
		LOG_HEXDUMP_INF(public_key_be, 64, "OOB Public Key:");
	}

	for (int i = 0; i < ARRAY_SIZE(oob_auth_test_vector); i++) {
		oob_auth_set(i);

		ASSERT_OK(bt_mesh_prov_enable(BT_MESH_PROV_ADV));

		/* Keep a long timeout so the prov multi case has time to finish: */
		ASSERT_OK(k_sem_take(&prov_sem, K_SECONDS(40)));

		/* Delay to complete procedure with Provisioning Complete PDU frame.
		 * Device shall start later provisioner was able to set OOB public key.
		 */
		k_sleep(K_SECONDS(2));

		bt_mesh_reset();
	}
}

static void oob_provisioner(bool read_oob_pk, bool use_oob_pk)
{
	int err = 0;

	k_sem_init(&prov_sem, 0, 1);

	oob_channel_id = bs_open_back_channel(get_device_nbr(),
			(uint[]){(get_device_nbr() + 1) % 2}, (uint[]){0}, 1);
	if (!oob_channel_id) {
		FAIL("Can't open OOB interface (err %d)\n", err);
	}

	bt_mesh_device_setup(&prov, &comp);

	if (read_oob_pk) {
		/* Delay to complete procedure public key generation on provisioning device. */
		k_sleep(K_SECONDS(1));

		int size = bs_bc_is_msg_received(*oob_channel_id);

		if (size <= 0) {
			FAIL("OOB public key is not gotten");
		}

		bs_bc_receive_msg(*oob_channel_id, public_key_be, 64);
		LOG_HEXDUMP_INF(public_key_be, 64, "OOB Public Key:");
	}

	ASSERT_OK(bt_mesh_cdb_create(test_net_key));

	ASSERT_OK(bt_mesh_provision(test_net_key, 0, 0, 0, 0x0001, dev_key));

	for (int i = 0; i < ARRAY_SIZE(oob_auth_test_vector); i++) {
		oob_auth_set(i);

		if (use_oob_pk) {
			ASSERT_OK(bt_mesh_prov_remote_pub_key_set(public_key_be));
		}

		ASSERT_OK(k_sem_take(&prov_sem, K_SECONDS(40)));

		bt_mesh_cdb_node_del(bt_mesh_cdb_node_get(prov_addr - 1), true);

		/* Delay to complete procedure with cleaning of the public key.
		 * This is important that the provisioner started the new cycle loop
		 * earlier than device to get OOB public key before capabilities frame.
		 */
		k_sleep(K_SECONDS(1));
	}

	bt_mesh_reset();
}

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

static void test_device_pb_adv_oob_auth(void)
{
	oob_device(false);

	PASS();
}

static void test_provisioner_pb_adv_oob_auth(void)
{
	oob_provisioner(false, false);

	PASS();
}

static void test_device_pb_adv_oob_public_key(void)
{
	oob_device(true);

	PASS();
}

static void test_provisioner_pb_adv_oob_public_key(void)
{
	oob_provisioner(true, true);

	PASS();
}

static void test_provisioner_pb_adv_oob_auth_no_oob_public_key(void)
{
	oob_provisioner(true, false);

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
		.test_delete_f = test_terminate                                \
	}

static const struct bst_test_instance test_connect[] = {
	TEST_CASE(device, pb_adv_no_oob,
		  "Device: pb-adv provisioning use no-oob method"),
	TEST_CASE(device, pb_adv_oob_auth,
		  "Device: pb-adv provisioning use oob authentication"),
	TEST_CASE(device, pb_adv_oob_public_key,
		  "Device: pb-adv provisioning use oob public key"),
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
	TEST_CASE(provisioner, pb_adv_oob_auth,
		  "Provisioner: pb-adv provisioning use oob authentication"),
	TEST_CASE(provisioner, pb_adv_oob_public_key,
		  "Provisioner: pb-adv provisioning use oob public key"),
	TEST_CASE(provisioner, pb_adv_oob_auth_no_oob_public_key,
		"Provisioner: pb-adv provisioning use oob authentication, ignore oob public key"),
	TEST_CASE(provisioner, pb_adv_reprovision,
		  "Provisioner: pb-adv provisioning, resetting and reprovisioning multiple times."),

	BSTEST_END_MARKER
};

struct bst_test_list *test_provision_install(struct bst_test_list *tests)
{
	tests = bst_add_tests(tests, test_connect);
	return tests;
}
