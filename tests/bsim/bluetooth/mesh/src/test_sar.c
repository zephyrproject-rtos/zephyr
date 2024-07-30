/*
 * Copyright (c) 2022 Nordic Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * SAR stress test
 */

#include "mesh_test.h"
#include <string.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(test_sar, LOG_LEVEL_INF);

#define CLI_ADDR    0x7728
#define SRV_ADDR    0x18f8
#define WAIT_TIME   60 /* seconds */
#define SEM_TIMEOUT K_SECONDS(25)
#define RAND_SEED 1

#define DUMMY_VND_MOD_GET_OP	BT_MESH_MODEL_OP_3(0xDC, TEST_VND_COMPANY_ID)
#define DUMMY_VND_MOD_STATUS_OP BT_MESH_MODEL_OP_3(0xCD, TEST_VND_COMPANY_ID)

#define MAX_SDU_MSG_LEN                                                                            \
	BT_MESH_TX_SDU_MAX - BT_MESH_MIC_SHORT - BT_MESH_MODEL_OP_LEN(DUMMY_VND_MOD_GET_OP)

static struct k_sem inst_suspend_sem;
static const uint8_t dev_key[16] = {0xaa};

static uint8_t dummy_msg[MAX_SDU_MSG_LEN] = {0};

static struct bt_mesh_msg_ctx test_ctx = {
	.net_idx = 0,
	.app_idx = 0,
	.addr = SRV_ADDR,
};

/* Segment Interval Step for both Transmitter and Receiver Configuration states must be at least 1,
 * or else network buffers run out.
 */
static struct bt_mesh_sar_tx test_sar_tx = {
	.seg_int_step = 1,
	.unicast_retrans_count = 15,
	.unicast_retrans_without_prog_count = 15,
	.unicast_retrans_int_step = 0,
	.unicast_retrans_int_inc = 0,
};

static struct bt_mesh_sar_rx test_sar_rx = {
	.seg_thresh = 0,
	.ack_delay_inc = 0,
	.discard_timeout = 15,
	.rx_seg_int_step = 1,
	.ack_retrans_count = 3,
};

static struct bt_mesh_sar_tx test_sar_slow_tx = {
	.seg_int_step = 15,
	.unicast_retrans_count = CONFIG_BT_MESH_SAR_TX_UNICAST_RETRANS_COUNT,
	.unicast_retrans_without_prog_count =
		CONFIG_BT_MESH_SAR_TX_UNICAST_RETRANS_WITHOUT_PROG_COUNT,
	.unicast_retrans_int_step = 15,
	.unicast_retrans_int_inc = 15,
};

static struct bt_mesh_sar_rx test_sar_slow_rx = {
	.seg_thresh = 0x1f,
	.ack_delay_inc = 7,
	.discard_timeout = CONFIG_BT_MESH_SAR_RX_DISCARD_TIMEOUT,
	.rx_seg_int_step = 15,
	.ack_retrans_count = CONFIG_BT_MESH_SAR_RX_ACK_RETRANS_COUNT,
};

static struct bt_mesh_prov prov;
static struct bt_mesh_cfg_cli cfg_cli;
static struct bt_mesh_sar_cfg_cli sar_cli;

/* Assert that buffer length and data corresponds with test dummy message.
 * Buffer state is saved.
 */
static void data_integrity_check(struct net_buf_simple *buf)
{
	struct net_buf_simple_state state = {0};

	ASSERT_EQUAL(buf->len, MAX_SDU_MSG_LEN);
	net_buf_simple_save(buf, &state);

	/* Note: Using ASSERT_TRUE since ASSERT_EQUAL would call cond twise if not true. */
	ASSERT_TRUE(memcmp(net_buf_simple_pull_mem(buf, MAX_SDU_MSG_LEN), dummy_msg,
			   MAX_SDU_MSG_LEN) == 0);
	net_buf_simple_restore(buf, &state);
}

static int get_handler(const struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
		       struct net_buf_simple *buf)
{
	data_integrity_check(buf);
	BT_MESH_MODEL_BUF_DEFINE(msg, DUMMY_VND_MOD_STATUS_OP, MAX_SDU_MSG_LEN);
	bt_mesh_model_msg_init(&msg, DUMMY_VND_MOD_STATUS_OP);
	net_buf_simple_add_mem(&msg, buf->data, buf->len);

	k_sem_give(&inst_suspend_sem);

	return bt_mesh_model_send(model, ctx, &msg, NULL, NULL);
}

static int status_handler(const struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
			  struct net_buf_simple *buf)
{
	data_integrity_check(buf);
	k_sem_give(&inst_suspend_sem);
	return 0;
}

static int dummy_vnd_mod_get(const struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
			     uint8_t msg[])
{
	BT_MESH_MODEL_BUF_DEFINE(buf, DUMMY_VND_MOD_GET_OP, MAX_SDU_MSG_LEN);

	bt_mesh_model_msg_init(&buf, DUMMY_VND_MOD_GET_OP);
	net_buf_simple_add_mem(&buf, msg, MAX_SDU_MSG_LEN);

	return bt_mesh_model_send(model, ctx, &buf, NULL, NULL);
}

static const struct bt_mesh_model_op _dummy_vnd_mod_op[] = {
	{DUMMY_VND_MOD_GET_OP, MAX_SDU_MSG_LEN, get_handler},
	{DUMMY_VND_MOD_STATUS_OP, MAX_SDU_MSG_LEN, status_handler},
	BT_MESH_MODEL_OP_END,
};

uint16_t dummy_keys[CONFIG_BT_MESH_MODEL_KEY_COUNT] = { 0 };

static const struct bt_mesh_elem elements[] = {BT_MESH_ELEM(
	0,
	MODEL_LIST(BT_MESH_MODEL_CFG_SRV,
		   BT_MESH_MODEL_CFG_CLI(&cfg_cli),
		   BT_MESH_MODEL_SAR_CFG_CLI(&sar_cli),
		   BT_MESH_MODEL_SAR_CFG_SRV),
	MODEL_LIST(BT_MESH_MODEL_VND_CB(TEST_VND_COMPANY_ID, TEST_VND_MOD_ID, _dummy_vnd_mod_op,
					NULL, NULL, NULL)))};

static const struct bt_mesh_comp comp = {
	.cid = TEST_VND_COMPANY_ID,
	.elem = elements,
	.elem_count = ARRAY_SIZE(elements),
};

static void prov_and_conf(uint16_t addr,
			  struct bt_mesh_sar_rx *sar_rx_config,
			  struct bt_mesh_sar_tx *sar_tx_config)
{
	int err;
	uint8_t status;
	struct bt_mesh_sar_tx tx_rsp;
	struct bt_mesh_sar_rx rx_rsp;

	ASSERT_OK(bt_mesh_provision(test_net_key, 0, 0, 0, addr, dev_key));

	err = bt_mesh_cfg_cli_app_key_add(0, addr, 0, 0, test_app_key, &status);
	if (err || status) {
		FAIL("AppKey add failed (err %d, status %u)", err, status);
	}

	err = bt_mesh_cfg_cli_mod_app_bind_vnd(0, addr, addr, 0, TEST_VND_MOD_ID,
					       TEST_VND_COMPANY_ID, &status);
	if (err || status) {
		FAIL("Failed to bind Dummy vnd model to application (err %d, status %u)", err,
		     status);
	}

	ASSERT_OK(bt_mesh_sar_cfg_cli_transmitter_set(0, addr, sar_tx_config, &tx_rsp));
	ASSERT_OK(bt_mesh_sar_cfg_cli_receiver_set(0, addr, sar_rx_config, &rx_rsp));
}

static void array_random_fill(uint8_t array[], uint16_t len, int seed)
{
	/* Ensures that the same random numbers are used for both client and server instances. */
	srand(seed);
	for (uint16_t i = 0; i < len; i++) {
		array[i] = rand() % 100;
	}
}

static void cli_max_len_sdu_send(struct bt_mesh_sar_rx *sar_rx_config,
				 struct bt_mesh_sar_tx *sar_tx_config)
{
	const struct bt_mesh_model *dummy_vnd_mod = &elements[0].vnd_models[0];

	bt_mesh_test_cfg_set(NULL, WAIT_TIME);
	bt_mesh_device_setup(&prov, &comp);
	prov_and_conf(CLI_ADDR, sar_rx_config, sar_tx_config);

	ASSERT_OK(k_sem_init(&inst_suspend_sem, 0, 1));
	array_random_fill(dummy_msg, ARRAY_SIZE(dummy_msg), RAND_SEED);

	for (int i = 0; i < 2; i++) {
		ASSERT_OK(dummy_vnd_mod_get(dummy_vnd_mod, &test_ctx, dummy_msg));
		/* Wait for message response */
		if (k_sem_take(&inst_suspend_sem, SEM_TIMEOUT)) {
			FAIL("Client suspension timed out.");
		}
		k_sem_reset(&inst_suspend_sem);
	}

	PASS();
}

static void srv_max_len_sdu_receive(struct bt_mesh_sar_rx *sar_rx_config,
				    struct bt_mesh_sar_tx *sar_tx_config)
{
	bt_mesh_test_cfg_set(NULL, WAIT_TIME);
	bt_mesh_device_setup(&prov, &comp);
	prov_and_conf(SRV_ADDR, sar_rx_config, sar_tx_config);

	ASSERT_OK(k_sem_init(&inst_suspend_sem, 0, 1));
	array_random_fill(dummy_msg, ARRAY_SIZE(dummy_msg), RAND_SEED);

	/* Wait for message to be received */
	if (k_sem_take(&inst_suspend_sem, SEM_TIMEOUT)) {
		FAIL("Server suspension timed out.");
	}

	PASS();
}

static void test_cli_max_len_sdu_send(void)
{
	cli_max_len_sdu_send(&test_sar_rx, &test_sar_tx);

	PASS();
}

static void test_srv_max_len_sdu_receive(void)
{
	srv_max_len_sdu_receive(&test_sar_rx, &test_sar_tx);

	PASS();
}

static void test_cli_max_len_sdu_slow_send(void)
{
	cli_max_len_sdu_send(&test_sar_slow_rx, &test_sar_slow_tx);

	PASS();
}

static void test_srv_max_len_sdu_slow_receive(void)
{
	srv_max_len_sdu_receive(&test_sar_slow_rx, &test_sar_slow_tx);

	PASS();
}

#if CONFIG_BT_SETTINGS
static void test_srv_cfg_store(void)
{
	struct bt_mesh_sar_rx rx_cfg;
	struct bt_mesh_sar_tx tx_cfg;

	bt_mesh_test_cfg_set(NULL, WAIT_TIME);
	bt_mesh_device_setup(&prov, &comp);
	prov_and_conf(SRV_ADDR, &test_sar_rx, &test_sar_tx);

	bt_mesh_sar_cfg_cli_receiver_get(0, SRV_ADDR, &rx_cfg);
	bt_mesh_sar_cfg_cli_transmitter_get(0, SRV_ADDR, &tx_cfg);

	ASSERT_EQUAL(0, memcmp(&rx_cfg, &test_sar_rx, sizeof(test_sar_rx)));
	ASSERT_EQUAL(0, memcmp(&tx_cfg, &test_sar_tx, sizeof(test_sar_tx)));

	PASS();
}

static void test_srv_cfg_restore(void)
{
	struct bt_mesh_sar_rx rx_cfg;
	struct bt_mesh_sar_tx tx_cfg;

	bt_mesh_test_cfg_set(NULL, WAIT_TIME);
	bt_mesh_device_setup(&prov, &comp);

	bt_mesh_sar_cfg_cli_receiver_get(0, SRV_ADDR, &rx_cfg);
	bt_mesh_sar_cfg_cli_transmitter_get(0, SRV_ADDR, &tx_cfg);

	ASSERT_EQUAL(0, memcmp(&rx_cfg, &test_sar_rx, sizeof(test_sar_rx)));
	ASSERT_EQUAL(0, memcmp(&tx_cfg, &test_sar_tx, sizeof(test_sar_tx)));

	PASS();
}
#endif

#define TEST_CASE(role, name, description)              \
	{                                                   \
		.test_id = "sar_" #role "_" #name,              \
		.test_descr = description,                      \
		.test_tick_f = bt_mesh_test_timeout,            \
		.test_main_f = test_##role##_##name,            \
	}

static const struct bst_test_instance test_sar[] = {
	TEST_CASE(cli, max_len_sdu_send,
		  "Send a 32-segment message with pre-defined test SAR configurations"),
	TEST_CASE(srv, max_len_sdu_receive,
		  "Receive a 32-segment message with pre-defined test SAR configurations."),
	TEST_CASE(cli, max_len_sdu_slow_send,
		  "Send a 32-segment message with SAR configured with slowest timings."),
	TEST_CASE(srv, max_len_sdu_slow_receive,
		  "Receive a 32-segment message with SAR configured with slowest timings."),

	BSTEST_END_MARKER};

struct bst_test_list *test_sar_install(struct bst_test_list *tests)
{
	tests = bst_add_tests(tests, test_sar);
	return tests;
}

#if CONFIG_BT_SETTINGS
static const struct bst_test_instance test_sar_pst[] = {
	TEST_CASE(srv, cfg_store, "Set and save SAR RX/TX configuration"),
	TEST_CASE(srv, cfg_restore, "Restore SAR RX/TX configuration"),

	BSTEST_END_MARKER};

struct bst_test_list *test_sar_pst_install(struct bst_test_list *tests)
{
	tests = bst_add_tests(tests, test_sar_pst);
	return tests;
}
#endif
