/*
 * Copyright (c) 2022 Nordic Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Opcode aggregator test
 */

#include "mesh_test.h"

#include <string.h>

#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(test_op_agg, LOG_LEVEL_INF);

#define CLI_ADDR    0x7728
#define SRV_ADDR    0x18f8
#define WAIT_TIME   15 /* seconds */
#define SEM_TIMEOUT K_SECONDS(10)

#define BT_MESH_DUMMY_VND_MOD_GET_OP	BT_MESH_MODEL_OP_3(0xDC, TEST_VND_COMPANY_ID)
#define BT_MESH_DUMMY_VND_MOD_STATUS_OP BT_MESH_MODEL_OP_3(0xCD, TEST_VND_COMPANY_ID)

#define BT_MESH_DUMMY_VND_MOD_MSG_MINLEN 7
#define BT_MESH_DUMMY_VND_MOD_MSG_MAXLEN 8

/* The 34 messages make up the aggregated message sequence, expecting a 380 byte status response. */
#define TEST_SEND_ITR 34

/* Spec: 4.3.9.4: Table 4.273 defines the structure of the OPCODES_AGGREGATOR_STATUS message. */
#define OPCODES_AGG_STATUS_MSG_BASE_STRUCTURE_LEN 5
/* SPEC: 4.3.9.1: Length_format + Length_Short.*/
#define OPCODES_AGG_ITEM_SHORT_FORMAT_LEN	  1
/* SPEC: 4.3.9.1: The structure of an Aggregator Item field is defined in Table 4.270 */
#define OPCODES_STATUS_ITEM_LEN(param_len)                                                         \
	(OPCODES_AGG_ITEM_SHORT_FORMAT_LEN +                                                       \
	 BT_MESH_MODEL_OP_LEN(BT_MESH_DUMMY_VND_MOD_STATUS_OP) + param_len)
/* Spec: 4.3.9.3 OPCODES_AGGREGATOR_STATUS. The test initiates 33+1 get/status message iterations.*/
#define OP_AGG_STATUS_ACCESS_PAYLOAD                                                               \
	(OPCODES_AGG_STATUS_MSG_BASE_STRUCTURE_LEN +                                               \
	 (OPCODES_STATUS_ITEM_LEN(BT_MESH_DUMMY_VND_MOD_MSG_MINLEN) * (TEST_SEND_ITR - 1)) +       \
	 OPCODES_STATUS_ITEM_LEN(BT_MESH_DUMMY_VND_MOD_MSG_MAXLEN))

/* Ensure that a 380-byte opcode aggregator get/status access payload is being sent. */
BUILD_ASSERT(OP_AGG_STATUS_ACCESS_PAYLOAD == (BT_MESH_TX_SDU_MAX - BT_MESH_MIC_SHORT));

static int status_rcvd_count;
static int get_rcvd_count;
static struct k_sem cli_suspend_sem;
static struct k_sem srv_suspend_sem;
static const uint8_t dev_key[16] = {0xaa};
static uint8_t cli_sent_array[TEST_SEND_ITR], cli_rcvd_array[TEST_SEND_ITR];

static struct bt_mesh_msg_ctx test_ctx = {
	.net_idx = 0,
	.app_idx = 0,
	.addr = SRV_ADDR,
};

static struct bt_mesh_prov prov;
static struct bt_mesh_cfg_cli cfg_cli;

static int get_handler(struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
		       struct net_buf_simple *buf)
{
	uint8_t seq = net_buf_simple_pull_u8(buf);

	get_rcvd_count++;

	BT_MESH_MODEL_BUF_DEFINE(msg, BT_MESH_DUMMY_VND_MOD_STATUS_OP,
				 BT_MESH_DUMMY_VND_MOD_MSG_MAXLEN);
	bt_mesh_model_msg_init(&msg, BT_MESH_DUMMY_VND_MOD_STATUS_OP);

	net_buf_simple_add_u8(&msg, seq);
	memset(net_buf_simple_add(&msg, BT_MESH_DUMMY_VND_MOD_MSG_MINLEN - 1), 0,
		BT_MESH_DUMMY_VND_MOD_MSG_MINLEN);

	/* Last message: One additional byte is added to fill the available access payload.*/
	if (get_rcvd_count >= TEST_SEND_ITR) {
		net_buf_simple_add(&msg, 1);
		k_sem_give(&srv_suspend_sem);
	}

	return bt_mesh_model_send(model, ctx, &msg, NULL, NULL);
}

static int status_handler(struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
			  struct net_buf_simple *buf)
{
	uint8_t seq = net_buf_simple_pull_u8(buf);

	status_rcvd_count++;
	cli_rcvd_array[status_rcvd_count - 1] = seq;

	if (status_rcvd_count >= TEST_SEND_ITR) {
		k_sem_give(&cli_suspend_sem);
	}

	return 0;
}

static int dummy_vnd_mod_get(struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx, uint8_t seq)
{
	BT_MESH_MODEL_BUF_DEFINE(msg, BT_MESH_DUMMY_VND_MOD_GET_OP,
				 BT_MESH_DUMMY_VND_MOD_MSG_MAXLEN);

	bt_mesh_model_msg_init(&msg, BT_MESH_DUMMY_VND_MOD_GET_OP);

	net_buf_simple_add_u8(&msg, seq);
	memset(net_buf_simple_add(&msg, BT_MESH_DUMMY_VND_MOD_MSG_MINLEN - 1), 0,
		BT_MESH_DUMMY_VND_MOD_MSG_MINLEN);

	/* Last message: One additional byte is added to fill the available access payload.*/
	if (seq >= TEST_SEND_ITR - 1) {
		net_buf_simple_add(&msg, 1);
	}

	return bt_mesh_model_send(model, ctx, &msg, NULL, NULL);
}

const struct bt_mesh_model_op _dummy_vnd_mod_op[] = {
	{BT_MESH_DUMMY_VND_MOD_GET_OP, BT_MESH_DUMMY_VND_MOD_MSG_MINLEN, get_handler},
	{BT_MESH_DUMMY_VND_MOD_STATUS_OP, BT_MESH_DUMMY_VND_MOD_MSG_MINLEN, status_handler},
	BT_MESH_MODEL_OP_END,
};

static struct bt_mesh_elem elements[] = {BT_MESH_ELEM(
	0,
	MODEL_LIST(BT_MESH_MODEL_CFG_SRV, BT_MESH_MODEL_CFG_CLI(&cfg_cli), BT_MESH_MODEL_OP_AGG_SRV,
		   BT_MESH_MODEL_OP_AGG_CLI),
	MODEL_LIST(BT_MESH_MODEL_VND_CB(TEST_VND_COMPANY_ID, TEST_VND_MOD_ID, _dummy_vnd_mod_op,
					NULL, NULL, NULL)))};

static const struct bt_mesh_comp comp = {
	.cid = TEST_VND_COMPANY_ID,
	.elem = elements,
	.elem_count = ARRAY_SIZE(elements),
};

static void op_agg_test_prov_and_conf(uint16_t addr)
{
	uint8_t status;
	int err;

	ASSERT_OK(bt_mesh_provision(test_net_key, 0, 0, 0, addr, dev_key));

	err = bt_mesh_cfg_cli_app_key_add(0, addr, 0, 0, test_app_key, &status);
	if (err || status) {
		FAIL("AppKey add failed (err %d, status %u)", err, status);
	}

	err = bt_mesh_cfg_cli_mod_app_bind(0, addr, addr, 0, BT_MESH_MODEL_ID_OP_AGG_CLI,
					    &status);
	if (err || status) {
		FAIL("Failed to bind OP_AGG_CLI to application (err %d, status %u)", err, status);
	}
	err = bt_mesh_cfg_cli_mod_app_bind(0, addr, addr, 0, BT_MESH_MODEL_ID_OP_AGG_SRV,
					    &status);
	if (err || status) {
		FAIL("Failed to bind OP_AGG_SRV to application (err %d, status %u)", err, status);
	}
	err = bt_mesh_cfg_cli_mod_app_bind_vnd(0, addr, addr, 0, TEST_VND_MOD_ID,
						TEST_VND_COMPANY_ID, &status);
	if (err || status) {
		FAIL("Failed to bind OP_AGG_TEST_MOD to application (err %d, status %u)", err,
		     status);
	}
}

static void test_cli_max_len_sequence_msg_send(void)
{
	struct bt_mesh_model *dummy_vnd_model = &elements[0].vnd_models[0];
	uint8_t seq;

	bt_mesh_test_cfg_set(NULL, WAIT_TIME);
	bt_mesh_device_setup(&prov, &comp);
	op_agg_test_prov_and_conf(CLI_ADDR);

	ASSERT_OK(k_sem_init(&cli_suspend_sem, 0, 1));
	ASSERT_OK(bt_mesh_op_agg_cli_seq_start(0, 0, SRV_ADDR, SRV_ADDR));

	for (int i = 0; i < TEST_SEND_ITR; i++) {
		seq = cli_sent_array[i] = i;
		ASSERT_OK(dummy_vnd_mod_get(dummy_vnd_model, &test_ctx, seq));
	}

	ASSERT_OK(bt_mesh_op_agg_cli_seq_send());

	/* Wait for all expected STATUS messages to be received */
	if (k_sem_take(&cli_suspend_sem, SEM_TIMEOUT)) {
		FAIL("Client suspension timed out. Status-messages received: %d",
		     status_rcvd_count);
	}

	if (memcmp(cli_sent_array, cli_rcvd_array, ARRAY_SIZE(cli_rcvd_array))) {
		FAIL("Message arrays (sent / rcvd) are not equal.");
	}

	PASS();
}

static void test_srv_max_len_status_msg_send(void)
{
	bt_mesh_test_cfg_set(NULL, WAIT_TIME);
	bt_mesh_device_setup(&prov, &comp);
	op_agg_test_prov_and_conf(SRV_ADDR);

	ASSERT_OK(k_sem_init(&srv_suspend_sem, 0, 1));

	/* Wait for all expected GET messages to be received */
	if (k_sem_take(&srv_suspend_sem, SEM_TIMEOUT)) {
		FAIL("Server suspension timed out. Get-messages received: %d", get_rcvd_count);
	}

	PASS();
}

#define TEST_CASE(role, name, description)         \
	{                                              \
		.test_id = "op_agg_" #role "_" #name,      \
		.test_descr = description,                 \
		.test_tick_f = bt_mesh_test_timeout,       \
		.test_main_f = test_##role##_##name,       \
	}

static const struct bst_test_instance test_op_agg[] = {
	TEST_CASE(cli, max_len_sequence_msg_send,
		  "OpAggCli composes a sequence request list, expecting a 380 Byte status message "
		  "in return."),
	TEST_CASE(srv, max_len_status_msg_send,
		  "OpAggSrv will respond with a 380 Byte status message. "),

	BSTEST_END_MARKER};

struct bst_test_list *test_op_agg_install(struct bst_test_list *tests)
{
	tests = bst_add_tests(tests, test_op_agg);
	return tests;
}
