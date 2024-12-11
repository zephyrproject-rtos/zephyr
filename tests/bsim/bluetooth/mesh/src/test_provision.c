/*
 * Copyright (c) 2021 Lingao Meng
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <stdlib.h>

#include "mesh_test.h"
#include "mesh/access.h"
#include "mesh/net.h"
#include "mesh/crypto.h"
#include "argparse.h"
#include <bs_pc_backchannel.h>
#include <time_machine.h>

#if defined CONFIG_BT_MESH_USES_MBEDTLS_PSA
#include <psa/crypto.h>
#else
#error "Unknown crypto library has been chosen"
#endif

#include <zephyr/sys/byteorder.h>

#define LOG_MODULE_NAME mesh_prov

#include <zephyr/logging/log.h>
#include "mesh/rpr.h"

LOG_MODULE_REGISTER(LOG_MODULE_NAME);

/*
 * Provision layer tests:
 *   Tests both the provisioner and device role in various scenarios.
 */

#define PROV_MULTI_COUNT 3
#define PROV_REPROV_COUNT 3
#define WAIT_TIME 120 /*seconds*/
#define IS_RPR_PRESENT  (CONFIG_BT_MESH_RPR_SRV && CONFIG_BT_MESH_RPR_CLI)
#define IMPOSTER_MODEL_ID 0xe000

enum test_flags {
	IS_PROVISIONER,

	TEST_FLAGS,
};

static uint8_t static_key1[] = {0x6E, 0x6F, 0x72, 0x64, 0x69, 0x63, 0x5F,
		0x65, 0x78, 0x61, 0x6D, 0x70, 0x6C, 0x65, 0x5F, 0x31};
static uint8_t static_key2[] = {0x6E, 0x6F, 0x72, 0x64, 0x69, 0x63, 0x5F};
static uint8_t static_key3[] = {0x45, 0x6E, 0x68, 0x61, 0x6E, 0x63, 0x65, 0x64, 0x20, 0x70, 0x72,
				0x6F, 0x76, 0x69, 0x73, 0x69, 0x6F, 0x6E, 0x69, 0x6E, 0x67, 0x20,
				0x73, 0x74, 0x61, 0x74, 0x69, 0x63, 0x20, 0x4F, 0x4F, 0x42};

static uint8_t private_key_be[32];
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
	{static_key3, sizeof(static_key3), 0, 0, 0, 0},
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

static ATOMIC_DEFINE(test_flags, TEST_FLAGS);

extern const struct bt_mesh_comp comp;
extern const uint8_t test_net_key[16];
extern const uint8_t test_app_key[16];

/* Timeout semaphore */
static struct k_sem prov_sem;
static K_SEM_DEFINE(link_open_sem, 0, 1);
static uint16_t prov_addr = 0x0002;
static uint16_t current_dev_addr;
static const uint8_t dev_key[16] = { 0x01, 0x02, 0x03, 0x04, 0x05 };
static uint8_t dev_uuid[16] = { 0x6c, 0x69, 0x6e, 0x67, 0x61, 0x6f };
static uint8_t *uuid_to_provision;
static struct k_sem reprov_sem;
static uint32_t link_close_timestamp;

/* Set prov_bearer to non-zero invalid value. */
static bt_mesh_prov_bearer_t prov_bearer = 0xF8;

static void test_args_parse(int argc, char *argv[])
{
	bs_args_struct_t args_struct[] = {
		{
			.dest = &prov_bearer,
			.type = 'i',
			.name = "{invalid, PB-ADV, PB-GATT}",
			.option = "prov-brearer",
			.descript = "Provisioning bearer that is to be used."
		},
	};

	bs_args_parse_all_cmd_line(argc, argv, args_struct);
}

#if IS_RPR_PRESENT
static struct k_sem pdu_send_sem;
static struct k_sem scan_sem;
/* Remote Provisioning models related variables. */
static uint8_t *uuid_to_provision_remote;
static void rpr_scan_report(struct bt_mesh_rpr_cli *cli, const struct bt_mesh_rpr_node *srv,
			    struct bt_mesh_rpr_unprov *unprov, struct net_buf_simple *adv_data);

static struct bt_mesh_rpr_cli rpr_cli = {
	.scan_report = rpr_scan_report,
};

static const struct bt_mesh_comp rpr_cli_comp = {
	.elem =
		(const struct bt_mesh_elem[]){
			BT_MESH_ELEM(1,
				     MODEL_LIST(BT_MESH_MODEL_CFG_SRV,
						BT_MESH_MODEL_CFG_CLI(&(struct bt_mesh_cfg_cli){}),
						BT_MESH_MODEL_RPR_CLI(&rpr_cli)),
				     BT_MESH_MODEL_NONE),
		},
	.elem_count = 1,
};

static const struct bt_mesh_comp rpr_srv_comp = {
	.elem =
		(const struct bt_mesh_elem[]){
			BT_MESH_ELEM(1,
				     MODEL_LIST(BT_MESH_MODEL_CFG_SRV,
						BT_MESH_MODEL_RPR_SRV),
				     BT_MESH_MODEL_NONE),
		},
	.elem_count = 1,
};

static const struct bt_mesh_comp rpr_cli_srv_comp = {
	.elem =
		(const struct bt_mesh_elem[]){
			BT_MESH_ELEM(1,
				     MODEL_LIST(BT_MESH_MODEL_CFG_SRV,
						BT_MESH_MODEL_CFG_CLI(&(struct bt_mesh_cfg_cli){}),
						BT_MESH_MODEL_RPR_CLI(&rpr_cli),
						BT_MESH_MODEL_RPR_SRV),
				     BT_MESH_MODEL_NONE),
		},
	.elem_count = 1,
};

static int mock_pdu_send(const struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
			       struct net_buf_simple *buf)
{
	/* Device becomes unresponsive and doesn't communicate with other nodes anymore */
	k_sleep(K_MSEC(10));
	bt_mesh_suspend();

	k_sem_give(&pdu_send_sem);

	return 0;
}

static const struct bt_mesh_model_op model_rpr_op1[] = {
	{ RPR_OP_PDU_SEND, 0, mock_pdu_send },
	BT_MESH_MODEL_OP_END
};

static int mock_model_init(const struct bt_mesh_model *mod)
{
	mod->keys[0] = BT_MESH_KEY_DEV_LOCAL;
	mod->rt->flags |= BT_MESH_MOD_DEVKEY_ONLY;

	return 0;
}

const struct bt_mesh_model_cb mock_model_cb = {
	.init = mock_model_init
};

static const struct bt_mesh_comp rpr_srv_comp_unresponsive = {
	.elem =
		(const struct bt_mesh_elem[]){
			BT_MESH_ELEM(1,
				     MODEL_LIST(BT_MESH_MODEL_CFG_SRV,
						BT_MESH_MODEL_CB(IMPOSTER_MODEL_ID,
								 model_rpr_op1, NULL, NULL,
								 &mock_model_cb),
						BT_MESH_MODEL_RPR_SRV,),
				     BT_MESH_MODEL_NONE),
		},
	.elem_count = 1,
};

static const uint8_t elem_offset1[2] = {1, 2};
static const uint8_t elem_offset2[3] = {4, 5, 6};
static const uint8_t additional_data[2] = {100, 200};

static const struct bt_mesh_comp2_record comp_rec[2] = {
	{.id = 1,
	 .version.x = 2,
	 .version.y = 3,
	 .version.z = 4,
	 .elem_offset_cnt = sizeof(elem_offset1),
	 .elem_offset = elem_offset1,
	 .data_len = 0},
	{.id = 10,
	 .version.x = 20,
	 .version.y = 30,
	 .version.z = 40,
	 .elem_offset_cnt = sizeof(elem_offset2),
	 .elem_offset = elem_offset2,
	 .data_len = sizeof(additional_data),
	 .data = additional_data},
};

static const struct bt_mesh_comp2 comp_p2_1 = {.record_cnt = 1, .record = comp_rec};
static const struct bt_mesh_comp2 comp_p2_2 = {.record_cnt = 2, .record = comp_rec};

static const struct bt_mesh_comp rpr_srv_comp_2_elem = {
	.elem =
		(const struct bt_mesh_elem[]){
			BT_MESH_ELEM(1,
				     MODEL_LIST(BT_MESH_MODEL_CFG_SRV,
						BT_MESH_MODEL_RPR_SRV),
				     BT_MESH_MODEL_NONE),
			BT_MESH_ELEM(2,
				     MODEL_LIST(BT_MESH_MODEL_CB(TEST_MOD_ID, BT_MESH_MODEL_NO_OPS,
								 NULL, NULL, NULL)),
				     BT_MESH_MODEL_NONE),
		},
	.elem_count = 2,
};
#endif /* IS_RPR_PRESENT */

/* Delayed work to avoid requesting OOB info before generation of this. */
static struct k_work_delayable oob_timer;
static void delayed_input(struct k_work *work);

static uint *oob_channel_id;
static bool is_oob_auth;

static void test_device_init(void)
{
	/* Ensure that the UUID is unique: */
	dev_uuid[6] = '0' + get_device_nbr();

	bt_mesh_test_cfg_set(NULL, WAIT_TIME);
	k_work_init_delayable(&oob_timer, delayed_input);
}

static void test_provisioner_init(void)
{
	atomic_set_bit(test_flags, IS_PROVISIONER);
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
	if (!atomic_test_bit(test_flags, IS_PROVISIONER)) {
		return;
	}

	if (uuid_to_provision && memcmp(uuid, uuid_to_provision, 16)) {
		return;
	}
	bt_mesh_provision_adv(uuid, 0, prov_addr, 0);
}

static void unprovisioned_beacon_gatt(uint8_t uuid[16], bt_mesh_prov_oob_info_t oob_info)
{
	if (!atomic_test_bit(test_flags, IS_PROVISIONER)) {
		return;
	}

	if (uuid_to_provision && memcmp(uuid, uuid_to_provision, 16)) {
		return;
	}

	bt_mesh_provision_gatt(uuid, 0, prov_addr, 0);
}

static void prov_complete(uint16_t net_idx, uint16_t addr)
{
	if (!atomic_test_bit(test_flags, IS_PROVISIONER)) {
		k_sem_give(&prov_sem);
	}
}

static void prov_link_open(bt_mesh_prov_bearer_t bearer)
{
	k_sem_give(&link_open_sem);
}

static void prov_link_close(bt_mesh_prov_bearer_t bearer)
{
	link_close_timestamp = k_uptime_get_32();
}

static void prov_node_added(uint16_t net_idx, uint8_t uuid[16], uint16_t addr,
			    uint8_t num_elem)
{
	LOG_INF("Device 0x%04x provisioned", prov_addr);
	current_dev_addr = prov_addr++;
	k_sem_give(&prov_sem);
}

static void prov_reprovisioned(uint16_t addr)
{
	LOG_INF("Device reprovisioned. New address: 0x%04x", addr);
	k_sem_give(&reprov_sem);
}

static void prov_reset(void)
{
	ASSERT_OK(bt_mesh_prov_enable(prov_bearer));
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
	.unprovisioned_beacon_gatt = unprovisioned_beacon_gatt,
	.complete = prov_complete,
	.link_open = prov_link_open,
	.link_close = prov_link_close,
	.reprovisioned = prov_reprovisioned,
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
	if (cap->oob_type & BT_MESH_STATIC_OOB_AVAILABLE) {
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

static void generate_oob_key_pair(void)
{
	psa_key_attributes_t key_attributes = PSA_KEY_ATTRIBUTES_INIT;
	psa_key_id_t priv_key_id = PSA_KEY_ID_NULL;
	psa_status_t status;
	size_t key_len;
	uint8_t public_key_repr[PSA_KEY_EXPORT_ECC_PUBLIC_KEY_MAX_SIZE(256)];

	/* Crypto settings for ECDH using the SHA256 hashing algorithm,
	 * the secp256r1 curve
	 */
	psa_set_key_usage_flags(&key_attributes, PSA_KEY_USAGE_DERIVE | PSA_KEY_USAGE_EXPORT);
	psa_set_key_lifetime(&key_attributes, PSA_KEY_LIFETIME_VOLATILE);
	psa_set_key_algorithm(&key_attributes, PSA_ALG_ECDH);
	psa_set_key_type(&key_attributes, PSA_KEY_TYPE_ECC_KEY_PAIR(PSA_ECC_FAMILY_SECP_R1));
	psa_set_key_bits(&key_attributes, 256);

	/* Generate a key pair */
	status = psa_generate_key(&key_attributes, &priv_key_id);
	ASSERT_TRUE(status == PSA_SUCCESS);

	status = psa_export_public_key(priv_key_id, public_key_repr, sizeof(public_key_repr),
				       &key_len);
	ASSERT_TRUE(status == PSA_SUCCESS);

	ASSERT_TRUE(key_len == PSA_KEY_EXPORT_ECC_PUBLIC_KEY_MAX_SIZE(256));

	status = psa_export_key(priv_key_id, private_key_be, sizeof(private_key_be), &key_len);
	ASSERT_TRUE(status == PSA_SUCCESS);

	ASSERT_TRUE(key_len == sizeof(private_key_be));

	memcpy(public_key_be, public_key_repr + 1, 64);
}

static void oob_device(bool use_oob_pk)
{
	k_sem_init(&prov_sem, 0, 1);

	bt_mesh_device_setup(&prov, &comp);

	if (use_oob_pk) {
		generate_oob_key_pair();
		prov.public_key_be = public_key_be;
		prov.private_key_be = private_key_be;
		bs_bc_send_msg(*oob_channel_id, public_key_be, 64);
		LOG_HEXDUMP_INF(public_key_be, 64, "OOB Public Key:");
	}

	for (int i = 0; i < ARRAY_SIZE(oob_auth_test_vector); i++) {
		oob_auth_set(i);

		ASSERT_OK(bt_mesh_prov_enable(prov_bearer));

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
	k_sem_init(&prov_sem, 0, 1);

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

/** Configures the health server on a node at current_dev_addr address and sends node reset.
 */
static void node_configure_and_reset(void)
{
	uint8_t status;
	size_t subs_count = 1;
	uint16_t sub;
	struct bt_mesh_cfg_cli_mod_pub healthpub = { 0 };
	struct bt_mesh_cdb_node *node;

	/* Check that publication and subscription are reset after last iteration */
	ASSERT_OK(bt_mesh_cfg_cli_mod_sub_get(0, current_dev_addr, current_dev_addr,
					  BT_MESH_MODEL_ID_HEALTH_SRV, &status, &sub,
					  &subs_count));
	ASSERT_EQUAL(0, status);
	ASSERT_TRUE(subs_count == 0);

	ASSERT_OK(bt_mesh_cfg_cli_mod_pub_get(0, current_dev_addr, current_dev_addr,
					  BT_MESH_MODEL_ID_HEALTH_SRV, &healthpub,
					  &status));
	ASSERT_EQUAL(0, status);
	ASSERT_TRUE_MSG(healthpub.addr == BT_MESH_ADDR_UNASSIGNED, "Pub not cleared\n");

	/* Set pub and sub to check that they are reset */
	healthpub.addr = 0xc001;
	healthpub.app_idx = 0;
	healthpub.cred_flag = false;
	healthpub.ttl = 10;
	healthpub.period = BT_MESH_PUB_PERIOD_10SEC(1);
	healthpub.transmit = BT_MESH_TRANSMIT(3, 100);

	ASSERT_OK(bt_mesh_cfg_cli_app_key_add(0, current_dev_addr, 0, 0, test_app_key,
					  &status));
	ASSERT_EQUAL(0, status);

	k_sleep(K_SECONDS(2));

	ASSERT_OK(bt_mesh_cfg_cli_mod_app_bind(0, current_dev_addr, current_dev_addr, 0x0,
					   BT_MESH_MODEL_ID_HEALTH_SRV, &status));
	ASSERT_EQUAL(0, status);

	k_sleep(K_SECONDS(2));

	ASSERT_OK(bt_mesh_cfg_cli_mod_sub_add(0, current_dev_addr, current_dev_addr, 0xc000,
					  BT_MESH_MODEL_ID_HEALTH_SRV, &status));
	ASSERT_EQUAL(0, status);

	k_sleep(K_SECONDS(2));

	ASSERT_OK(bt_mesh_cfg_cli_mod_pub_set(0, current_dev_addr, current_dev_addr,
					  BT_MESH_MODEL_ID_HEALTH_SRV, &healthpub,
					  &status));
	ASSERT_EQUAL(0, status);

	k_sleep(K_SECONDS(2));

	ASSERT_OK(bt_mesh_cfg_cli_node_reset(0, current_dev_addr, (bool *)&status));

	node = bt_mesh_cdb_node_get(current_dev_addr);
	bt_mesh_cdb_node_del(node, true);
}

/** @brief Verify that this device pb-adv provision.
 */
static void test_device_no_oob(void)
{
	k_sem_init(&prov_sem, 0, 1);

	bt_mesh_device_setup(&prov, &comp);
	ASSERT_OK(bt_mesh_prov_enable(prov_bearer));

	LOG_INF("Mesh initialized\n");

	/* Keep a long timeout so the prov multi case has time to finish: */
	ASSERT_OK(k_sem_take(&prov_sem, K_SECONDS(40)));

	PASS();
}

/** @brief Verify that this device can be reprovisioned after resets
 */
static void test_device_reprovision(void)
{
	k_sem_init(&prov_sem, 0, 1);

	bt_mesh_device_setup(&prov, &comp);

	ASSERT_OK(bt_mesh_prov_enable(prov_bearer));

	LOG_INF("Mesh initialized\n");

	for (int i = 0; i < PROV_REPROV_COUNT; i++) {
		/* Keep a long timeout so the prov multi case has time to finish: */
		LOG_INF("Dev prov loop #%d, waiting for prov ...\n", i);
		ASSERT_OK(k_sem_take(&prov_sem, K_SECONDS(20)));
	}

	PASS();
}

/** @brief Verify that this provisioner pb-adv provision.
 */
static void test_provisioner_no_oob(void)
{
	k_sem_init(&prov_sem, 0, 1);

	bt_mesh_device_setup(&prov, &comp);

	ASSERT_OK(bt_mesh_cdb_create(test_net_key));

	ASSERT_OK(bt_mesh_provision(test_net_key, 0, 0, 0, 0x0001, dev_key));

	ASSERT_OK(k_sem_take(&prov_sem, K_SECONDS(5)));

	PASS();
}

static void test_device_oob_auth(void)
{
	oob_device(false);

	PASS();
}

static void test_provisioner_oob_auth(void)
{
	oob_provisioner(false, false);

	PASS();
}

static void test_back_channel_pre_init(void)
{
	oob_channel_id = bs_open_back_channel(get_device_nbr(),
			(uint[]){(get_device_nbr() + 1) % 2}, (uint[]){0}, 1);
	if (!oob_channel_id) {
		FAIL("Can't open OOB interface\n");
	}
}

static void test_device_oob_public_key(void)
{
	oob_device(true);

	PASS();
}

static void test_provisioner_oob_public_key(void)
{
	oob_provisioner(true, true);

	PASS();
}

static void test_provisioner_oob_auth_no_oob_public_key(void)
{
	oob_provisioner(true, false);

	PASS();
}

/** @brief Verify that the provisioner can provision multiple devices in a row
 */
static void test_provisioner_multi(void)
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
static void test_provisioner_reprovision(void)
{
	k_sem_init(&prov_sem, 0, 1);

	bt_mesh_device_setup(&prov, &comp);

	ASSERT_OK(bt_mesh_cdb_create(test_net_key));

	ASSERT_OK(bt_mesh_provision(test_net_key, 0, 0, 0, 0x0001, dev_key));

	for (int i = 0; i < PROV_REPROV_COUNT; i++) {
		LOG_INF("Provisioner prov loop #%d, waiting for prov ...\n", i);
		ASSERT_OK(k_sem_take(&prov_sem, K_SECONDS(20)));

		node_configure_and_reset();
	}

	PASS();
}

/** @brief Device starts unprovisioned. Stops being responsive to Mesh message after initial setup.
 * Later becomes responsive but becomes unresponsive again after provisioning link opens.
 * Then becomes responsive again allowing successful provisioning. Never stops advertising
 * Unprovisioned Device beacons.
 */
static void test_device_unresponsive(void)
{
	bt_mesh_device_setup(&prov, &comp);

	k_sem_init(&prov_sem, 0, 1);

	ASSERT_OK(bt_mesh_prov_enable(prov_bearer));

	/* stop responding for 30s to timeout PB-ADV link establishment. */
	bt_mesh_scan_disable();
	k_sleep(K_SECONDS(30));
	bt_mesh_scan_enable();

	k_sem_take(&link_open_sem, K_SECONDS(20));
	/* stop responding for 60s to timeout protocol */
	bt_mesh_scan_disable();
	k_sleep(K_SECONDS(60));
	bt_mesh_scan_enable();

	k_sem_take(&prov_sem, K_SECONDS(20));
	PASS();
}

#if IS_RPR_PRESENT
static int provision_adv(uint8_t dev_idx, uint16_t *addr)
{
	static uint8_t uuid[16];
	int err;

	memcpy(uuid, dev_uuid, 16);
	uuid[6] = '0' + dev_idx;
	uuid_to_provision = uuid;

	LOG_INF("Waiting for a device with RPR Server to be provisioned over PB-Adv...");
	err = k_sem_take(&prov_sem, K_SECONDS(10));
	*addr = current_dev_addr;

	return err;
}

static int provision_remote(struct bt_mesh_rpr_node *srv, uint8_t dev_idx, uint16_t *addr)
{
	static uint8_t uuid[16];
	struct bt_mesh_rpr_scan_status scan_status;
	int err;

	memcpy(uuid, dev_uuid, 16);
	uuid[6] = '0' + dev_idx;
	uuid_to_provision_remote = uuid;

	LOG_INF("Starting scanning for an unprov device...");
	ASSERT_OK(bt_mesh_rpr_scan_start(&rpr_cli, srv, NULL, 5, 1, &scan_status));
	ASSERT_EQUAL(BT_MESH_RPR_SUCCESS, scan_status.status);
	ASSERT_EQUAL(BT_MESH_RPR_SCAN_MULTI, scan_status.scan);
	ASSERT_EQUAL(1, scan_status.max_devs);
	ASSERT_EQUAL(5, scan_status.timeout);

	err = k_sem_take(&prov_sem, K_SECONDS(20));
	*addr = current_dev_addr;

	return err;
}

static void rpr_scan_report(struct bt_mesh_rpr_cli *cli, const struct bt_mesh_rpr_node *srv,
			    struct bt_mesh_rpr_unprov *unprov, struct net_buf_simple *adv_data)
{
	if (!uuid_to_provision_remote || memcmp(uuid_to_provision_remote, unprov->uuid, 16)) {
		return;
	}

	LOG_INF("Remote device discovered. Provisioning...");
	ASSERT_OK(bt_mesh_provision_remote(cli, srv, unprov->uuid, 0, prov_addr));
}

static void prov_node_added_rpr(uint16_t net_idx, uint8_t uuid[16], uint16_t addr,
				uint8_t num_elem)
{
	LOG_INF("Device 0x%04x reprovisioned", addr);
	k_sem_give(&reprov_sem);
}

static void provisioner_pb_remote_client_setup(void)
{
	k_sem_init(&prov_sem, 0, 1);
	k_sem_init(&reprov_sem, 0, 1);

	bt_mesh_device_setup(&prov, &rpr_cli_comp);

	ASSERT_OK(bt_mesh_cdb_create(test_net_key));
	ASSERT_OK(bt_mesh_provision(test_net_key, 0, 0, 0, 0x0001, dev_key));
}

static void device_pb_remote_server_setup(const struct bt_mesh_comp *comp, bool pb_adv_prov)
{
	k_sem_init(&prov_sem, 0, 1);
	k_sem_init(&reprov_sem, 0, 1);

	bt_mesh_device_setup(&prov, comp);

	if (pb_adv_prov) {
		ASSERT_OK(bt_mesh_prov_enable(BT_MESH_PROV_ADV));

		LOG_INF("Waiting for being provisioned...");
		ASSERT_OK(k_sem_take(&prov_sem, K_SECONDS(20)));
	} else {
		ASSERT_TRUE(bt_mesh_is_provisioned());
	}

	LOG_INF("Enabling PB-Remote server");
	ASSERT_OK(bt_mesh_prov_enable(BT_MESH_PROV_REMOTE));
}

static void device_pb_remote_server_setup_unproved(const struct bt_mesh_comp *comp,
						   const struct bt_mesh_comp2 *comp_p2)
{
	device_pb_remote_server_setup(comp, true);
	bt_mesh_comp2_register(comp_p2);
}

static void device_pb_remote_server_setup_proved(const struct bt_mesh_comp *comp,
						 const struct bt_mesh_comp2 *comp_p2)
{
	device_pb_remote_server_setup(comp, false);
	bt_mesh_comp2_register(comp_p2);
}

/** @brief Verify that the provisioner can provision a device multiple times after resets using
 * PB-Remote and RPR models.
 */
static void test_provisioner_pb_remote_client_reprovision(void)
{
	uint16_t pb_remote_server_addr;

	provisioner_pb_remote_client_setup();

	/* Provision the 2nd device over PB-Adv. */
	ASSERT_OK(provision_adv(1, &pb_remote_server_addr));

	for (int i = 0; i < PROV_REPROV_COUNT; i++) {
		struct bt_mesh_rpr_node srv = {
			.addr = pb_remote_server_addr,
			.net_idx = 0,
			.ttl = 3,
		};

		LOG_INF("Provisioner prov loop #%d, waiting for prov ...\n", i);
		ASSERT_OK(provision_remote(&srv, 2, &srv.addr));

		node_configure_and_reset();
	}

	PASS();
}

static void rpr_scan_report_parallel(struct bt_mesh_rpr_cli *cli,
				const struct bt_mesh_rpr_node *srv,
				struct bt_mesh_rpr_unprov *unprov,
				struct net_buf_simple *adv_data)
{
	if (!uuid_to_provision_remote || memcmp(uuid_to_provision_remote, unprov->uuid, 16)) {
		return;
	}

	LOG_INF("Scanning dev idx 2 succeeded.\n");
	k_sem_give(&scan_sem);
}

static void test_provisioner_pb_remote_client_parallel(void)
{
	static uint8_t uuid[16];
	uint16_t pb_remote_server_addr;
	struct bt_mesh_rpr_scan_status scan_status;

	memcpy(uuid, dev_uuid, 16);

	k_sem_init(&prov_sem, 0, 1);
	k_sem_init(&scan_sem, 0, 1);

	bt_mesh_device_setup(&prov, &rpr_cli_comp);

	ASSERT_OK(bt_mesh_cdb_create(test_net_key));
	ASSERT_OK(bt_mesh_provision(test_net_key, 0, 0, 0, 0x0001, dev_key));

	/* Provision the 2nd device over PB-Adv. */
	ASSERT_OK(provision_adv(1, &pb_remote_server_addr));

	struct bt_mesh_rpr_node srv = {
		.addr = pb_remote_server_addr,
		.net_idx = 0,
		.ttl = 3,
	};

	rpr_cli.scan_report = rpr_scan_report_parallel;

	LOG_INF("Scanning dev idx 2 and provisioning dev idx 3 in parallel ...\n");
	/* provisioning device with dev index 2 */
	uuid[6] = '0' + 2;
	ASSERT_OK(bt_mesh_provision_remote(&rpr_cli, &srv, uuid, 0, prov_addr));
	/* scanning device with dev index 3 */
	uuid[6] = '0' + 3;
	uuid_to_provision_remote = uuid;
	ASSERT_OK(bt_mesh_rpr_scan_start(&rpr_cli, &srv, uuid, 15, 1, &scan_status));
	ASSERT_EQUAL(BT_MESH_RPR_SUCCESS, scan_status.status);
	ASSERT_EQUAL(BT_MESH_RPR_SCAN_SINGLE, scan_status.scan);
	ASSERT_EQUAL(1, scan_status.max_devs);
	ASSERT_EQUAL(15, scan_status.timeout);

	ASSERT_OK(k_sem_take(&scan_sem, K_SECONDS(20)));
	ASSERT_OK(k_sem_take(&prov_sem, K_SECONDS(20)));

	/* Provisioning device index 3. Need it to succeed provisionee test scenario. */
	ASSERT_OK(bt_mesh_provision_remote(&rpr_cli, &srv, uuid, 0, prov_addr));
	ASSERT_OK(k_sem_take(&prov_sem, K_SECONDS(20)));

	PASS();
}

/** @brief Test Provisioning procedure on Remote Provisioning client:
 * - verify procedure timeouts on unresponsive unprovisioned device.
 */
static void test_provisioner_pb_remote_client_provision_timeout(void)
{
	uint16_t pb_remote_server_addr;
	uint8_t uuid[16];
	uint32_t link_close_wait_start;
	struct bt_mesh_rpr_scan_status scan_status;

	k_sem_init(&scan_sem, 0, 1);

	provisioner_pb_remote_client_setup();
	bt_mesh_test_cfg_set(NULL, 300);

	/* Provision the 2nd device over PB-Adv. */
	ASSERT_OK(provision_adv(1, &pb_remote_server_addr));

	/* Provision the 3rd device over PB-Remote. */
	struct bt_mesh_rpr_node srv = {
		.addr = pb_remote_server_addr,
		.net_idx = 0,
		.ttl = 3,
	};

	rpr_cli.scan_report = rpr_scan_report_parallel;

	/* Offset timeline of test to give some time to 3rd device to setup and disable scanning */
	k_sleep(K_SECONDS(10));

	memcpy(uuid, dev_uuid, 16);
	uuid[6] = '0' + 2;
	uuid_to_provision_remote = uuid;

	LOG_INF("Starting scanning for an unprov device...");
	ASSERT_OK(bt_mesh_rpr_scan_start(&rpr_cli, &srv, uuid, 5, 1, &scan_status));
	ASSERT_EQUAL(BT_MESH_RPR_SUCCESS, scan_status.status);
	ASSERT_EQUAL(BT_MESH_RPR_SCAN_SINGLE, scan_status.scan);
	ASSERT_EQUAL(1, scan_status.max_devs);
	ASSERT_EQUAL(5, scan_status.timeout);

	ASSERT_OK(k_sem_take(&scan_sem, K_SECONDS(20)));

	/* Invalidate earlier timestamp */
	link_close_timestamp = -1;
	ASSERT_OK(bt_mesh_provision_remote(&rpr_cli, &srv, uuid, 0, prov_addr));
	link_close_wait_start = k_uptime_get_32();
	ASSERT_EQUAL(k_sem_take(&prov_sem, K_SECONDS(20)), -EAGAIN);
	ASSERT_EQUAL((link_close_timestamp - link_close_wait_start) / MSEC_PER_SEC, 10);

	/* 3rd device should now respond but stop again after link is opened */
	link_close_timestamp = -1;
	ASSERT_OK(bt_mesh_provision_remote(&rpr_cli, &srv, uuid, 0, prov_addr));
	ASSERT_OK(k_sem_take(&link_open_sem, K_SECONDS(20)));
	link_close_wait_start = k_uptime_get_32();
	ASSERT_EQUAL(k_sem_take(&prov_sem, K_SECONDS(61)), -EAGAIN);
	ASSERT_EQUAL((link_close_timestamp - link_close_wait_start) / MSEC_PER_SEC, 60);

	PASS();
}

static void reprovision_remote_devkey_client(struct bt_mesh_rpr_node *srv,
					     struct bt_mesh_cdb_node *node)
{
	uint8_t status;
	uint8_t prev_node_dev_key[16];

	ASSERT_OK_MSG(bt_mesh_cdb_node_key_export(node, prev_node_dev_key),
		      "Can't export device key from cdb");

	bt_mesh_reprovision_remote(&rpr_cli, srv, current_dev_addr, false);

	ASSERT_OK(k_sem_take(&reprov_sem, K_SECONDS(20)));

	/* Check that CDB has updated Device Key for the node. */
	ASSERT_TRUE(bt_mesh_key_compare(prev_node_dev_key, &node->dev_key));
	ASSERT_OK_MSG(bt_mesh_cdb_node_key_export(node, prev_node_dev_key),
			"Can't export device key from cdb");

	/* Check device key by adding appkey. */
	ASSERT_OK(bt_mesh_cfg_cli_app_key_add(0, current_dev_addr, 0, 0, test_app_key,
						&status));
	ASSERT_OK(status);

	/* Let RPR Server verify Device Key. */
	k_sleep(K_SECONDS(2));
}

static void reprovision_remote_comp_data_client(struct bt_mesh_rpr_node *srv,
						struct bt_mesh_cdb_node *node,
						struct net_buf_simple *dev_comp)
{
	NET_BUF_SIMPLE_DEFINE(new_dev_comp, BT_MESH_RX_SDU_MAX);
	uint8_t prev_node_dev_key[16];
	uint8_t page;

	ASSERT_OK_MSG(bt_mesh_cdb_node_key_export(node, prev_node_dev_key),
		      "Can't export device key from cdb");

	bt_mesh_reprovision_remote(&rpr_cli, srv, current_dev_addr, true);

	ASSERT_OK(k_sem_take(&reprov_sem, K_SECONDS(20)));

	/* Check that CDB has updated Device Key for the node. */
	ASSERT_TRUE(bt_mesh_key_compare(prev_node_dev_key, &node->dev_key));
	ASSERT_OK_MSG(bt_mesh_cdb_node_key_export(node, prev_node_dev_key),
			"Can't export device key from cdb");

	/* Check that Composition Data Page 128 is now Page 0. */
	net_buf_simple_reset(&new_dev_comp);
	ASSERT_OK(bt_mesh_cfg_cli_comp_data_get(0, current_dev_addr, 0, &page,
						&new_dev_comp));

	ASSERT_EQUAL(0, page);
	ASSERT_EQUAL(dev_comp->len, new_dev_comp.len);
	if (memcmp(dev_comp->data, new_dev_comp.data, dev_comp->len)) {
		FAIL("Wrong composition data page 0");
	}

	/* Let RPR Server verify Device Key. */
	k_sleep(K_SECONDS(2));
}

static void reprovision_remote_address_client(struct bt_mesh_rpr_node *srv,
					      struct bt_mesh_cdb_node *node)
{
	uint8_t status;
	uint8_t prev_node_dev_key[16];

	ASSERT_OK_MSG(bt_mesh_cdb_node_key_export(node, prev_node_dev_key),
		      "Can't export device key from cdb");

	bt_mesh_reprovision_remote(&rpr_cli, srv, current_dev_addr + 1, false);

	ASSERT_OK(k_sem_take(&reprov_sem, K_SECONDS(20)));

	current_dev_addr++;
	srv->addr++;

	/* Check that device doesn't respond to old address with old and new device key. */
	struct bt_mesh_cdb_node *prev_node;
	uint8_t tmp[16];

	prev_node = bt_mesh_cdb_node_alloc((uint8_t[16]) {}, current_dev_addr - 1, 1, 0);
	ASSERT_TRUE(node);
	ASSERT_OK_MSG(bt_mesh_cdb_node_key_import(prev_node, prev_node_dev_key),
			"Can't import device key into cdb");
	ASSERT_EQUAL(-ETIMEDOUT, bt_mesh_cfg_cli_app_key_add(0, current_dev_addr - 1, 0, 0,
								test_app_key, &status));
	ASSERT_OK_MSG(bt_mesh_cdb_node_key_export(node, tmp),
			"Can't export device key from cdb");
	ASSERT_OK_MSG(bt_mesh_cdb_node_key_import(prev_node, tmp),
			"Can't import device key into cdb");
	ASSERT_EQUAL(-ETIMEDOUT, bt_mesh_cfg_cli_app_key_add(0, current_dev_addr - 1, 0, 0,
								test_app_key, &status));
	bt_mesh_cdb_node_del(prev_node, false);

	/* Check that CDB has updated Device Key for the node. */
	ASSERT_TRUE(bt_mesh_key_compare(prev_node_dev_key, &node->dev_key));
	ASSERT_OK_MSG(bt_mesh_cdb_node_key_export(node, prev_node_dev_key),
			"Can't export device key from cdb");

	/* Check new device address by adding appkey. */
	ASSERT_OK(bt_mesh_cfg_cli_app_key_add(0, current_dev_addr, 0, 0, test_app_key,
						&status));
	ASSERT_OK(status);

	/* Let RPR Server verify Device Key. */
	k_sleep(K_SECONDS(2));

}

/** @brief Verify robustness of NPPI procedures on a RPR Client by running Device Key Refresh,
 * Node Composition Refresh and Node Address Refresh procedures.
 */
static void test_provisioner_pb_remote_client_nppi_robustness(void)
{
	NET_BUF_SIMPLE_DEFINE(dev_comp, BT_MESH_RX_SDU_MAX);
	uint8_t page;
	uint16_t pb_remote_server_addr;
	uint8_t status;
	struct bt_mesh_cdb_node *node;

	provisioner_pb_remote_client_setup();

	/* Provision the 2nd device over PB-Adv. */
	ASSERT_OK(provision_adv(1, &pb_remote_server_addr));

	/* Provision a remote device with RPR Server. */
	struct bt_mesh_rpr_node srv = {
		.addr = pb_remote_server_addr,
		.net_idx = 0,
		.ttl = 3,
	};

	ASSERT_OK(provision_remote(&srv, 2, &srv.addr));

	/* Check device key by adding appkey. */
	ASSERT_OK(bt_mesh_cfg_cli_app_key_add(0, current_dev_addr, 0, 0, test_app_key, &status));
	ASSERT_OK(status);

	/* Swap callback to catch when device reprovisioned. */
	prov.node_added = prov_node_added_rpr;

	/* Store initial Composition Data Page 0. */
	ASSERT_OK(bt_mesh_cfg_cli_comp_data_get(0, current_dev_addr, 0, &page, &dev_comp));

	node = bt_mesh_cdb_node_get(current_dev_addr);
	ASSERT_TRUE(node);

	LOG_INF("Testing DevKey refresh...");
	for (int i = 0; i < PROV_REPROV_COUNT; i++) {
		LOG_INF("Refreshing device key #%d...\n", i);
		reprovision_remote_devkey_client(&srv, node);
	}

	LOG_INF("Testing Composition Data refresh...");
	for (int i = 0; i < PROV_REPROV_COUNT; i++) {
		LOG_INF("Changing Composition Data #%d...\n", i);
		reprovision_remote_comp_data_client(&srv, node, &dev_comp);
	}

	LOG_INF("Testing address refresh...");
	for (int i = 0; i < PROV_REPROV_COUNT; i++) {
		LOG_INF("Changing address #%d...\n", i);
		reprovision_remote_address_client(&srv, node);
	}

	PASS();
}

/** @brief A device running a Remote Provisioning server that is used to provision unprovisioned
 * devices over PB-Remote. Always starts unprovisioned.
 */
static void test_device_pb_remote_server_unproved(void)
{
	device_pb_remote_server_setup_unproved(&rpr_srv_comp, &comp_p2_1);

	PASS();
}

/** @brief A device running a Remote Provisioning server that is used to provision unprovisioned
 * devices over PB-Remote. Always starts unprovisioned. Stops being responsive after receives
 * Remote Provisioning PDU Send message from RPR Client
 */
static void test_device_pb_remote_server_unproved_unresponsive(void)
{
	device_pb_remote_server_setup_unproved(&rpr_srv_comp_unresponsive, NULL);

	k_sem_init(&pdu_send_sem, 0, 1);
	ASSERT_OK(k_sem_take(&pdu_send_sem, K_SECONDS(200)));

	PASS();
}

/** @brief A device running a Remote Provisioning server that is used to provision unprovisioned
 * devices over PB-Remote. Starts provisioned.
 */
static void test_device_pb_remote_server_proved(void)
{
	device_pb_remote_server_setup_proved(&rpr_srv_comp, &comp_p2_1);

	PASS();
}

static void reprovision_remote_devkey_server(const uint16_t initial_addr)
{
	uint8_t prev_dev_key[16];
	uint8_t dev_key[16];

	ASSERT_OK(bt_mesh_key_export(prev_dev_key, &bt_mesh.dev_key));

	ASSERT_OK(k_sem_take(&reprov_sem, K_SECONDS(30)));
	ASSERT_EQUAL(initial_addr, bt_mesh_primary_addr());

	/* Let Configuration Client activate the new Device Key and verify that it has
	 * been changed.
	 */
	k_sleep(K_SECONDS(2));
	ASSERT_OK(bt_mesh_key_export(dev_key, &bt_mesh.dev_key));
	ASSERT_TRUE(memcmp(&prev_dev_key, dev_key, sizeof(dev_key)));
}

static void reprovision_remote_comp_data_server(const uint16_t initial_addr)
{
	u_int8_t prev_dev_key[16];
	u_int8_t dev_key[16];

	/* The RPR Server won't let to run Node Composition Refresh procedure without first
	 * setting the BT_MESH_COMP_DIRTY flag. The flag is set on a boot if there is a
	 * "bt/mesh/cmp" entry in settings. The entry is added by the
	 * `bt_mesh_comp_change_prepare() call. The test suite is not compiled
	 * with CONFIG_BT_SETTINGS, so the flag will never be set. Since the purpose of the
	 * test is to check RPR Server behavior, but not the actual swap of the Composition
	 * Data, the flag is toggled directly from the test.
	 */
	atomic_set_bit(bt_mesh.flags, BT_MESH_COMP_DIRTY);
	ASSERT_OK(bt_mesh_key_export(prev_dev_key, &bt_mesh.dev_key));

	ASSERT_OK(k_sem_take(&reprov_sem, K_SECONDS(30)));

	/* Drop the flag manually as CONFIG_BT_SETTINGS is not enabled. */
	atomic_clear_bit(bt_mesh.flags, BT_MESH_COMP_DIRTY);

	ASSERT_EQUAL(initial_addr, bt_mesh_primary_addr());

	/* Let Configuration Client activate the new Device Key and verify that it has
	 * been changed.
	 */
	k_sleep(K_SECONDS(2));
	ASSERT_OK(bt_mesh_key_export(dev_key, &bt_mesh.dev_key));
	ASSERT_TRUE(memcmp(prev_dev_key, dev_key, sizeof(dev_key)));
}

static void reprovision_remote_address_server(const uint16_t initial_addr)
{
	uint8_t prev_dev_key[16];
	uint8_t dev_key[16];

	ASSERT_OK(bt_mesh_key_export(prev_dev_key, &bt_mesh.dev_key));

	ASSERT_OK(k_sem_take(&reprov_sem, K_SECONDS(30)));
	ASSERT_EQUAL(initial_addr + 1, bt_mesh_primary_addr());

	/* Let Configuration Client activate the new Device Key and verify that it has
	 * been changed.
	 */
	k_sleep(K_SECONDS(2));
	ASSERT_OK(bt_mesh_key_export(dev_key, &bt_mesh.dev_key));
	ASSERT_TRUE(memcmp(prev_dev_key, dev_key, sizeof(dev_key)));
}

/** @brief Verify robustness of NPPI procedures on a RPR Server by running Device Key Refresh,
 * Node Composition Refresh and Node Address Refresh procedures multiple times each.
 */
static void test_device_pb_remote_server_nppi_robustness(void)
{
	k_sem_init(&prov_sem, 0, 1);
	k_sem_init(&reprov_sem, 0, 1);

	bt_mesh_device_setup(&prov, &rpr_srv_comp);

	ASSERT_OK(bt_mesh_prov_enable(BT_MESH_PROV_ADV));

	LOG_INF("Mesh initialized\n");

	ASSERT_OK(k_sem_take(&prov_sem, K_SECONDS(20)));
	const uint16_t initial_addr = bt_mesh_primary_addr();

	LOG_INF("Enabling PB-Remote server");
	ASSERT_OK(bt_mesh_prov_enable(BT_MESH_PROV_REMOTE));

	/* Test Device Key Refresh procedure robustness. */
	for (int i = 0; i < PROV_REPROV_COUNT; i++) {
		LOG_INF("Devkey refresh loop #%d, waiting for being reprov ...\n", i);
		reprovision_remote_devkey_server(initial_addr);
	}

	/* Test Node Composition Refresh procedure robustness. */
	for (int i = 0; i < PROV_REPROV_COUNT; i++) {
		LOG_INF("Composition data refresh loop #%d, waiting for being reprov ...\n", i);
		reprovision_remote_comp_data_server(initial_addr);
	}

	/* Node Address Refresh robustness. */
	for (int i = 0; i < PROV_REPROV_COUNT; i++) {
		LOG_INF("Address refresh loop #%d, waiting for being reprov ...\n", i);
		reprovision_remote_address_server(initial_addr+i);
	}

	PASS();
}

/** @brief Test Node Composition Refresh procedure on Remote Provisioning client:
 * - provision a device over PB-Adv,
 * - provision a remote device over PB-Remote.
 */
static void test_provisioner_pb_remote_client_ncrp_provision(void)
{
	uint16_t pb_remote_server_addr;
	uint8_t status;

	provisioner_pb_remote_client_setup();

	/* Provision the 2nd device over PB-Adv. */
	ASSERT_OK(provision_adv(1, &pb_remote_server_addr));

	/* Provision the 3rd device over PB-Remote. */
	struct bt_mesh_rpr_node srv = {
		.addr = pb_remote_server_addr,
		.net_idx = 0,
		.ttl = 3,
	};

	ASSERT_OK(provision_remote(&srv, 2, &srv.addr));

	/* Check device key by adding appkey. */
	ASSERT_OK(bt_mesh_cfg_cli_app_key_add(0, pb_remote_server_addr, 0, 0, test_app_key,
					      &status));
	ASSERT_OK(status);

	PASS();
}

/** @brief A device running a Remote Provisioning client and server that is used to reprovision
 * another device and it self with the client.
 */
static void test_device_pb_remote_client_server_same_dev(void)
{
	NET_BUF_SIMPLE_DEFINE(dev_comp, BT_MESH_RX_SDU_MAX);
	uint8_t status;
	struct bt_mesh_cdb_node *node;
	uint8_t page;
	uint8_t prev_dev_key[16];
	uint16_t test_vector[] = { 0x0002, 0x0001 };

	k_sem_init(&prov_sem, 0, 1);
	k_sem_init(&reprov_sem, 0, 1);

	bt_mesh_device_setup(&prov, &rpr_cli_srv_comp);

	ASSERT_OK(bt_mesh_cdb_create(test_net_key));
	ASSERT_OK(bt_mesh_provision(test_net_key, 0, 0, 0, 0x0001, dev_key));

	LOG_INF("Enabling PB-Remote server");
	ASSERT_OK(bt_mesh_prov_enable(BT_MESH_PROV_REMOTE));

	/* Provision a remote device with RPR Client and Server with local RPR Server. */
	current_dev_addr = 0x0001;
	struct bt_mesh_rpr_node srv = {
		.addr = current_dev_addr,
		.net_idx = 0,
		.ttl = 3,
	};

	LOG_INF("Provisioner prov, waiting for prov ...\n");
	ASSERT_OK(provision_remote(&srv, 1, &srv.addr));

	ASSERT_OK(k_sem_take(&prov_sem, K_SECONDS(20)));

	/* Check device key by adding bt_mesh_reprovision_remote appkey. */
	ASSERT_OK(bt_mesh_cfg_cli_app_key_add(0, current_dev_addr, 0, 0, test_app_key, &status));
	ASSERT_OK(status);

	/* Swap callback to catch when device reprovisioned. */
	prov.node_added = prov_node_added_rpr;

	/* Reprovision a device with both RPR Client and Server. */
	for (int i = 0; i < ARRAY_SIZE(test_vector); i++) {
		current_dev_addr = test_vector[i];
		srv.addr = current_dev_addr;
		bool self_reprov = (bool)(current_dev_addr == bt_mesh_primary_addr());

		/* Store initial Composition Data Page 0. */
		net_buf_simple_reset(&dev_comp);
		ASSERT_OK(bt_mesh_cfg_cli_comp_data_get(0, current_dev_addr, 0, &page, &dev_comp));

		node = bt_mesh_cdb_node_get(current_dev_addr);
		ASSERT_TRUE(node);

		LOG_INF("Refreshing 0x%04x device key ...\n", srv.addr);
		ASSERT_OK(bt_mesh_key_export(prev_dev_key, &bt_mesh.dev_key));
		reprovision_remote_devkey_client(&srv, node);
		if (self_reprov) {
			uint8_t dev_key[16];

			ASSERT_EQUAL(current_dev_addr, bt_mesh_primary_addr());

			/* Let Configuration Client activate the new Device Key
			 * and verify that it has been changed.
			 */
			ASSERT_OK(bt_mesh_key_export(dev_key, &bt_mesh.dev_key));
			ASSERT_TRUE(memcmp(prev_dev_key, dev_key, sizeof(dev_key)));
		}

		LOG_INF("Changing 0x%04x Composition Data ...\n", srv.addr);
		ASSERT_OK(bt_mesh_key_export(prev_dev_key, &bt_mesh.dev_key));
		reprovision_remote_comp_data_client(&srv, node, &dev_comp);
		if (self_reprov) {
			uint8_t dev_key[16];

			ASSERT_EQUAL(current_dev_addr, bt_mesh_primary_addr());

			/* Let Configuration Client activate the new Device Key
			 * and verify that it has been changed.
			 */
			ASSERT_OK(bt_mesh_key_export(dev_key, &bt_mesh.dev_key));
			ASSERT_TRUE(memcmp(prev_dev_key, dev_key, sizeof(struct bt_mesh_key)));
		}

		LOG_INF("Changing 0x%04x address ...\n", srv.addr);
		ASSERT_OK(bt_mesh_key_export(prev_dev_key, &bt_mesh.dev_key));
		reprovision_remote_address_client(&srv, node);
		if (self_reprov) {
			uint8_t dev_key[16];

			ASSERT_EQUAL(current_dev_addr, bt_mesh_primary_addr());

			/* Let Configuration Client activate the new Device Key
			 * and verify that it has been changed.
			 */
			ASSERT_OK(bt_mesh_key_export(dev_key, &bt_mesh.dev_key));
			ASSERT_TRUE(memcmp(prev_dev_key, dev_key, sizeof(dev_key)));
		}
	}

	PASS();
}

/** @brief Verify that the Remote Provisioning client and server is able to be reprovision
 * by another device with a Remote Provisioning client and server.
 */
static void test_device_pb_remote_server_same_dev(void)
{
	k_sem_init(&prov_sem, 0, 1);
	k_sem_init(&reprov_sem, 0, 1);

	bt_mesh_device_setup(&prov, &rpr_cli_srv_comp);

	ASSERT_OK(bt_mesh_prov_enable(BT_MESH_PROV_ADV));

	LOG_INF("Waiting for being provisioned...");
	ASSERT_OK(k_sem_take(&prov_sem, K_SECONDS(20)));

	LOG_INF("Enabling PB-Remote server");
	ASSERT_OK(bt_mesh_prov_enable(BT_MESH_PROV_REMOTE));

	/* Swap callback to catch when device reprovisioned. */
	prov.node_added = prov_node_added_rpr;

	const uint16_t initial_addr = bt_mesh_primary_addr();

	LOG_INF("Devkey refresh, waiting for being reprov ...\n");
	reprovision_remote_devkey_server(initial_addr);

	LOG_INF("Composition data refresh, waiting for being reprov ...\n");
	reprovision_remote_comp_data_server(initial_addr);

	LOG_INF("Address refresh, waiting for being reprov ...\n");
	reprovision_remote_address_server(initial_addr);

	PASS();
}

static void comp_data_get(uint16_t server_addr, uint8_t page, struct net_buf_simple *comp)
{
	uint8_t page_rsp;

	/* Let complete advertising of the transaction to prevent collisions. */
	k_sleep(K_SECONDS(3));

	net_buf_simple_reset(comp);
	ASSERT_OK(bt_mesh_cfg_cli_comp_data_get(0, server_addr, page, &page_rsp, comp));
	ASSERT_EQUAL(page, page_rsp);
}

static void comp_data_compare(struct net_buf_simple *comp1, struct net_buf_simple *comp2,
			      bool expect_equal)
{
	if (expect_equal) {
		ASSERT_EQUAL(comp1->len, comp2->len);
		if (memcmp(comp1->data, comp2->data, comp1->len)) {
			FAIL("Composition data is not equal");
		}
	} else {
		if (comp1->len == comp2->len) {
			if (!memcmp(comp1->data, comp2->data, comp1->len)) {
				FAIL("Composition data is equal");
			}
		}
	}
}

/** @brief Test Node Composition Refresh procedure on Remote Provisioning client:
 * - initiate Node Composition Refresh procedure on a 3rd device.
 */
static void test_provisioner_pb_remote_client_ncrp(void)
{
	NET_BUF_SIMPLE_DEFINE(dev_comp_p0, BT_MESH_RX_SDU_MAX);
	NET_BUF_SIMPLE_DEFINE(dev_comp_p1, BT_MESH_RX_SDU_MAX);
	NET_BUF_SIMPLE_DEFINE(dev_comp_p2, BT_MESH_RX_SDU_MAX);
	NET_BUF_SIMPLE_DEFINE(dev_comp_p128, BT_MESH_RX_SDU_MAX);
	NET_BUF_SIMPLE_DEFINE(dev_comp_p129, BT_MESH_RX_SDU_MAX);
	NET_BUF_SIMPLE_DEFINE(dev_comp_p130, BT_MESH_RX_SDU_MAX);

	uint16_t pb_remote_server_addr = 0x0003;

	k_sem_init(&prov_sem, 0, 1);
	k_sem_init(&reprov_sem, 0, 1);

	bt_mesh_device_setup(&prov, &rpr_cli_comp);

	/* Store Composition Data Page 0, 1, 2, 128, 129 and 130. */
	comp_data_get(pb_remote_server_addr, 0, &dev_comp_p0);
	comp_data_get(pb_remote_server_addr, 128, &dev_comp_p128);
	comp_data_compare(&dev_comp_p0, &dev_comp_p128, false);

	comp_data_get(pb_remote_server_addr, 1, &dev_comp_p1);
	comp_data_get(pb_remote_server_addr, 129, &dev_comp_p129);
	comp_data_compare(&dev_comp_p1, &dev_comp_p129, false);

	comp_data_get(pb_remote_server_addr, 2, &dev_comp_p2);
	comp_data_get(pb_remote_server_addr, 130, &dev_comp_p130);
	comp_data_compare(&dev_comp_p2, &dev_comp_p130, false);


	LOG_INF("Start Node Composition Refresh procedure...\n");
	struct bt_mesh_rpr_node srv = {
		.addr = pb_remote_server_addr,
		.net_idx = 0,
		.ttl = 3,
	};

	/* Swap callback to catch when device reprovisioned. */
	prov.node_added = prov_node_added_rpr;

	ASSERT_OK(bt_mesh_reprovision_remote(&rpr_cli, &srv, pb_remote_server_addr, true));
	ASSERT_OK(k_sem_take(&reprov_sem, K_SECONDS(20)));

	/* Check that Composition Data Page 128 still exists and is now equal to Page 0. */
	comp_data_get(pb_remote_server_addr, 0, &dev_comp_p0);
	comp_data_compare(&dev_comp_p0, &dev_comp_p128, true);
	comp_data_get(pb_remote_server_addr, 128, &dev_comp_p128);
	comp_data_compare(&dev_comp_p0, &dev_comp_p128, true);

	/* Check that Composition Data Page 129 still exists and is now equal to Page 1. */
	comp_data_get(pb_remote_server_addr, 1, &dev_comp_p1);
	comp_data_compare(&dev_comp_p1, &dev_comp_p129, true);
	comp_data_get(pb_remote_server_addr, 129, &dev_comp_p129);
	comp_data_compare(&dev_comp_p1, &dev_comp_p129, true);

	/* Check that Composition Data Page 130 still exists and is now equal to Page 2. */
	comp_data_get(pb_remote_server_addr, 2, &dev_comp_p2);
	comp_data_compare(&dev_comp_p2, &dev_comp_p130, true);
	comp_data_get(pb_remote_server_addr, 130, &dev_comp_p130);
	comp_data_compare(&dev_comp_p2, &dev_comp_p130, true);

	PASS();
}

static void comp_data_pages_get_and_equal_check(uint16_t server_addr, uint8_t page1, uint8_t page2)
{
	NET_BUF_SIMPLE_DEFINE(comp_1, BT_MESH_RX_SDU_MAX);
	NET_BUF_SIMPLE_DEFINE(comp_2, BT_MESH_RX_SDU_MAX);

	comp_data_get(server_addr, page1, &comp_1);
	comp_data_get(server_addr, page2, &comp_2);
	comp_data_compare(&comp_1, &comp_2, true);
}

/** @brief Test Node Composition Refresh procedure on Remote Provisioning client:
 * - verify that Composition Data Page 0 is now equal to Page 128 after reboot.
 */
static void test_provisioner_pb_remote_client_ncrp_second_time(void)
{
	uint16_t pb_remote_server_addr = 0x0003;
	int err;

	k_sem_init(&prov_sem, 0, 1);
	k_sem_init(&reprov_sem, 0, 1);

	bt_mesh_device_setup(&prov, &rpr_cli_comp);

	comp_data_pages_get_and_equal_check(pb_remote_server_addr, 0, 128);
	comp_data_pages_get_and_equal_check(pb_remote_server_addr, 1, 129);
	comp_data_pages_get_and_equal_check(pb_remote_server_addr, 2, 130);

	LOG_INF("Start Node Composition Refresh procedure...\n");
	struct bt_mesh_rpr_node srv = {
		.addr = pb_remote_server_addr,
		.net_idx = 0,
		.ttl = 3,
	};

	/* Swap callback to catch when device reprovisioned. */
	prov.node_added = prov_node_added_rpr;

	ASSERT_OK(bt_mesh_reprovision_remote(&rpr_cli, &srv, pb_remote_server_addr, true));
	err = k_sem_take(&reprov_sem, K_SECONDS(20));
	ASSERT_EQUAL(-EAGAIN, err);

	PASS();
}

/** @brief Test Node Composition Refresh procedure on Remote Provisioning server:
 * - wait for being provisioned over PB-Adv,
 * - prepare Composition Data Page 128.
 */
static void test_device_pb_remote_server_ncrp_prepare(void)
{
	device_pb_remote_server_setup_unproved(&rpr_srv_comp, &comp_p2_1);

	LOG_INF("Preparing for Composition Data change");
	bt_mesh_comp_change_prepare();

	PASS();
}

/** @brief Test Node Composition Refresh procedure on Remote Provisioning server:
 * - start device with new Composition Data
 * - wait for being re-provisioned.
 */
static void test_device_pb_remote_server_ncrp(void)
{
	device_pb_remote_server_setup_proved(&rpr_srv_comp_2_elem, &comp_p2_2);

	LOG_INF("Waiting for being re-provisioned.");
	ASSERT_OK(k_sem_take(&reprov_sem, K_SECONDS(30)));

	PASS();
}

/** @brief Test Node Composition Refresh procedure on Remote Provisioning server:
 * - verify that Composition Data Page 0 is replaced by Page 128 after being re-provisioned and
 *   rebooted.
 */
static void test_device_pb_remote_server_ncrp_second_time(void)
{
	int err;

	device_pb_remote_server_setup_proved(&rpr_srv_comp_2_elem, &comp_p2_2);

	LOG_INF("Wait to verify that node is not re-provisioned...");
	err = k_sem_take(&reprov_sem, K_SECONDS(30));
	ASSERT_EQUAL(-EAGAIN, err);

	PASS();
}
#endif /* IS_RPR_PRESENT */

/* Test cases by default will run over PB_ADV*/
#define TEST_CASE(role, name, description)                                                         \
	{.test_id = "prov_" #role "_" #name,                                                       \
	 .test_descr = description,                                                                \
	 .test_args_f = test_args_parse,							   \
	 .test_post_init_f = test_##role##_init,                                                   \
	 .test_tick_f = bt_mesh_test_timeout,                                                      \
	 .test_main_f = test_##role##_##name,                                                      \
	 .test_delete_f = test_terminate}
/* Test cases that will run over either PB_ADV or PB_GATT */
#define TEST_CASE_WBACKCHANNEL(role, name, description)                                    \
	{.test_id = "prov_" #role "_" #name,                                           \
	 .test_descr = description,                                                                \
	 .test_args_f = test_args_parse,							   \
	 .test_post_init_f = test_##role##_init,                                                   \
	 .test_pre_init_f = test_back_channel_pre_init,                                            \
	 .test_tick_f = bt_mesh_test_timeout,                                                      \
	 .test_main_f = test_##role##_##name,                                                      \
	 .test_delete_f = test_terminate}

static const struct bst_test_instance test_connect[] = {
	TEST_CASE(device, unresponsive,
		  "Device: provisioning, stops and resumes responding to provisioning"),
	TEST_CASE(device, no_oob, "Device: provisioning use no-oob method"),
	TEST_CASE_WBACKCHANNEL(device, oob_auth,
			       "Device: provisioning use oob authentication"),
	TEST_CASE_WBACKCHANNEL(device, oob_public_key,
			       "Device: provisioning use oob public key"),
	TEST_CASE(device, reprovision, "Device: provisioning, reprovision"),
#if IS_RPR_PRESENT
	TEST_CASE(device, pb_remote_server_unproved,
		  "Device: used for remote provisioning, starts unprovisioned"),
	TEST_CASE(device, pb_remote_server_nppi_robustness,
		  "Device: pb-remote reprovisioning, NPPI robustness"),
	TEST_CASE(device, pb_remote_server_unproved_unresponsive,
		  "Device: used for remote provisioning, starts unprovisioned, stops responding"),
	TEST_CASE(device, pb_remote_client_server_same_dev,
		  "Device: used for remote provisioning, with both client and server"),
	TEST_CASE(device, pb_remote_server_same_dev,
		  "Device: used for remote reprovisioning, with both client and server"),
#endif
	TEST_CASE(provisioner, iv_update_flag_zero,
		  "Provisioner: effect on ivu_duration when IV Update flag is set to zero"),
	TEST_CASE(provisioner, iv_update_flag_one,
		  "Provisioner: effect on ivu_duration when IV Update flag is set to one"),
	TEST_CASE(provisioner, no_oob,
			 "Provisioner: provisioning use no-oob method"),
	TEST_CASE(provisioner, multi,
			 "Provisioner: provisioning multiple devices"),
	TEST_CASE_WBACKCHANNEL(provisioner, oob_auth,
			       "Provisioner: provisioning use oob authentication"),
	TEST_CASE_WBACKCHANNEL(provisioner, oob_public_key,
			       "Provisioner: provisioning use oob public key"),
	TEST_CASE_WBACKCHANNEL(
		provisioner, oob_auth_no_oob_public_key,
		"Provisioner: provisioning use oob authentication, ignore oob public key"),
	TEST_CASE(
		provisioner, reprovision,
		"Provisioner: provisioning, resetting and reprovisioning multiple times."),
#if IS_RPR_PRESENT
	TEST_CASE(provisioner, pb_remote_client_reprovision,
		  "Provisioner: pb-remote provisioning, resetting and reprov-ing multiple times."),
	TEST_CASE(provisioner, pb_remote_client_nppi_robustness,
		  "Provisioner: pb-remote provisioning, NPPI robustness."),
	TEST_CASE(provisioner, pb_remote_client_parallel,
		  "Provisioner: pb-remote provisioning, parallel scanning and provisioning."),
	TEST_CASE(provisioner, pb_remote_client_provision_timeout,
		  "Provisioner: provisioning test, devices stop responding"),
#endif

	BSTEST_END_MARKER};

struct bst_test_list *test_provision_install(struct bst_test_list *tests)
{
	tests = bst_add_tests(tests, test_connect);
	return tests;
}

#if IS_RPR_PRESENT
static const struct bst_test_instance test_connect_pst[] = {
	TEST_CASE(device, pb_remote_server_unproved,
		  "Device: used for remote provisioning, starts unprovisioned"),
	TEST_CASE(device, pb_remote_server_proved,
		  "Device: used for remote provisioning, starts provisioned"),

	TEST_CASE(device, pb_remote_server_ncrp_prepare,
		  "Device: NCRP test, prepares for Composition Data change."),
	TEST_CASE(device, pb_remote_server_ncrp,
		  "Device: NCRP test, Composition Data change."),
	TEST_CASE(device, pb_remote_server_ncrp_second_time,
		  "Device: NCRP test, Composition Data change after reboot."),

	TEST_CASE(provisioner, pb_remote_client_ncrp_provision,
		  "Provisioner: NCRP test, devices provisioning."),
	TEST_CASE(provisioner, pb_remote_client_ncrp,
		  "Provisioner: NCRP test, initiates Node Composition Refresh procedure."),
	TEST_CASE(provisioner, pb_remote_client_ncrp_second_time,
		  "Provisioner: NCRP test, initiates NCR procedure the second time."),

	BSTEST_END_MARKER
};

struct bst_test_list *test_provision_pst_install(struct bst_test_list *tests)
{
	tests = bst_add_tests(tests, test_connect_pst);
	return tests;
}
#endif /* IS_RPR_PRESENT */
