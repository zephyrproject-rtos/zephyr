/*
 * Copyright (c) 2023 Nordic Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Large Composition Data test
 */

#include "mesh_test.h"

#include <stddef.h>
#include <string.h>

#include <mesh/access.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(test_lcd, LOG_LEVEL_INF);

#define CLI_ADDR    0x7728
#define SRV_ADDR    0x18f8
#define WAIT_TIME   60 /* seconds */

#define LCD_STATUS_FIELDS_LEN 5 /* Length of status metadata (offset, page and total size) */
#define DUMMY_2_BYTE_OP	      BT_MESH_MODEL_OP_2(0xff, 0xff)
#define BT_MESH_LCD_PAYLOAD_MAX                                                                \
	(BT_MESH_TX_SDU_MAX - BT_MESH_MODEL_OP_LEN(DUMMY_2_BYTE_OP) -                              \
	 BT_MESH_MIC_SHORT) /* 378 bytes */

/* Empty elements to create large composition data */
#define DUMMY_ELEM(i, _) BT_MESH_ELEM((i) + 2, BT_MESH_MODEL_NONE, BT_MESH_MODEL_NONE)

static const uint8_t dev_key[16] = {0xaa};

static struct bt_mesh_msg_ctx test_ctx = {
	.net_idx = 0,
	.app_idx = 0,
	.addr = SRV_ADDR,
};

static struct bt_mesh_prov prov;
static struct bt_mesh_cfg_cli cfg_cli;

static struct bt_mesh_elem elements[] = {
	BT_MESH_ELEM(1,
		     MODEL_LIST(BT_MESH_MODEL_CFG_SRV,
				BT_MESH_MODEL_CFG_CLI(&cfg_cli),
				BT_MESH_MODEL_LARGE_COMP_DATA_CLI,
				BT_MESH_MODEL_LARGE_COMP_DATA_SRV),
		     BT_MESH_MODEL_NONE),
	LISTIFY(88, DUMMY_ELEM, (,)),
};

static const struct bt_mesh_comp comp = {
	.cid = TEST_VND_COMPANY_ID,
	.elem = elements,
	.elem_count = ARRAY_SIZE(elements),
};

static void prov_and_conf(uint16_t addr)
{
	uint8_t status;

	ASSERT_OK(bt_mesh_provision(test_net_key, 0, 0, 0, addr, dev_key));

	/* Check device key by adding appkey. */
	ASSERT_OK(bt_mesh_cfg_cli_app_key_add(0, addr, 0, 0, test_app_key, &status));
	ASSERT_OK(status);
}

/* Since nodes self-provision in this test and the LCD model uses device keys for crypto, the
 * server node must be added to the client CDB manually.
 */
static void target_node_alloc(uint16_t addr)
{
	struct bt_mesh_cdb_node *node;

	node = bt_mesh_cdb_node_alloc(test_va_uuid, addr, comp.elem_count, test_ctx.net_idx);
	ASSERT_TRUE(node);
	memcpy(node->dev_key, dev_key, 16);
}

/* Assert that received metadata is equal to local values. Buffer state is saved */
static void verify_metadata(struct net_buf_simple *srv_rsp, uint8_t page_local,
			    uint16_t offset_local, uint16_t total_size_local)
{
	struct net_buf_simple_state state = {0};
	uint8_t byte_value[LCD_STATUS_FIELDS_LEN];

	byte_value[0] = page_local;
	memcpy(&byte_value[1], &offset_local, 2);
	memcpy(&byte_value[3], &total_size_local, 2);
	net_buf_simple_save(srv_rsp, &state);
	ASSERT_TRUE(memcmp(srv_rsp->data, byte_value, LCD_STATUS_FIELDS_LEN) == 0);
	net_buf_simple_restore(srv_rsp, &state);
}

static void test_cli_max_sdu_comp_data_request(void)
{
	uint8_t page = 0;
	uint16_t offset, total_size;

	/* comp_add_elem() requires sufficient tailroom for MIC */
	NET_BUF_SIMPLE_DEFINE(comp_local, BT_MESH_LCD_PAYLOAD_MAX + BT_MESH_MIC_SHORT);
	NET_BUF_SIMPLE_DEFINE(srv_rsp, BT_MESH_LCD_PAYLOAD_MAX);
	net_buf_simple_init(&comp_local, 0);
	net_buf_simple_init(&srv_rsp, 0);

	bt_mesh_test_cfg_set(NULL, WAIT_TIME);
	bt_mesh_device_setup(&prov, &comp);
	prov_and_conf(CLI_ADDR);
	target_node_alloc(SRV_ADDR);

	/* Request a max SDU of server composition data.
	 * Note: an offset of 1 is necessary with the status data to be exactly 380 bytes of access
	 * payload.
	 */
	offset = 1;
	ASSERT_OK(bt_mesh_large_comp_data_get(0, SRV_ADDR, page, offset, &srv_rsp));
	ASSERT_EQUAL(srv_rsp.len, BT_MESH_LCD_PAYLOAD_MAX);
	ASSERT_OK(bt_mesh_comp_data_get_page_0(&comp_local, offset));
	total_size = bt_mesh_comp_page_0_size();
	verify_metadata(&srv_rsp, page, offset, total_size);
	net_buf_simple_pull_mem(&srv_rsp, LCD_STATUS_FIELDS_LEN);
	ASSERT_TRUE(memcmp(srv_rsp.data, comp_local.data, srv_rsp.len) == 0);

	PASS();
}

static void test_cli_split_comp_data_request(void)
{
	uint8_t page = 0;
	uint16_t offset, total_size, prev_len;

	NET_BUF_SIMPLE_DEFINE(comp_local, (64 + BT_MESH_MIC_SHORT));
	NET_BUF_SIMPLE_DEFINE(srv_rsp, 64);
	net_buf_simple_init(&comp_local, 0);
	net_buf_simple_init(&srv_rsp, 0);

	bt_mesh_test_cfg_set(NULL, WAIT_TIME);
	bt_mesh_device_setup(&prov, &comp);
	prov_and_conf(CLI_ADDR);
	target_node_alloc(SRV_ADDR);

	offset = 20;

	/* Get first composition data sample, remote and local */
	ASSERT_OK(bt_mesh_large_comp_data_get(0, SRV_ADDR, page, offset, &srv_rsp));
	ASSERT_OK(bt_mesh_comp_data_get_page_0(&comp_local, offset));
	total_size = bt_mesh_comp_page_0_size();
	verify_metadata(&srv_rsp, page, offset, total_size);
	net_buf_simple_pull_mem(&srv_rsp, LCD_STATUS_FIELDS_LEN);
	prev_len = srv_rsp.len;
	ASSERT_TRUE(memcmp(srv_rsp.data, comp_local.data, srv_rsp.len) == 0);

	offset += prev_len;
	net_buf_simple_reset(&comp_local);
	net_buf_simple_reset(&srv_rsp);

	/* Get next composition data sample */
	ASSERT_OK(bt_mesh_large_comp_data_get(0, SRV_ADDR, page, offset, &srv_rsp));
	ASSERT_OK(bt_mesh_comp_data_get_page_0(&comp_local, offset));
	verify_metadata(&srv_rsp, page, offset, total_size);
	net_buf_simple_pull_mem(&srv_rsp, LCD_STATUS_FIELDS_LEN);
	ASSERT_TRUE(memcmp(srv_rsp.data, comp_local.data, srv_rsp.len) == 0);

	PASS();
}

static void test_srv_status_respond(void)
{
	bt_mesh_test_cfg_set(NULL, WAIT_TIME);
	bt_mesh_device_setup(&prov, &comp);
	prov_and_conf(SRV_ADDR);

	/* No server callback available. Wait 10 sec for message to be recived */
	k_sleep(K_SECONDS(10));

	PASS();
}

#define TEST_CASE(role, name, description)                                            \
	{                                                                                 \
		.test_id = "lcd_" #role "_" #name,                                            \
		.test_descr = description,                                                    \
		.test_tick_f = bt_mesh_test_timeout,                                          \
		.test_main_f = test_##role##_##name,                                          \
	}

static const struct bst_test_instance test_lcd[] = {
	TEST_CASE(
		cli, max_sdu_comp_data_request, "Request comp data with max SDU length"),
	TEST_CASE(
		cli, split_comp_data_request, "Request continuous comp data in two samples."),
	TEST_CASE(
		srv, status_respond, "Process incoming GET LCD messages."),

	BSTEST_END_MARKER};

struct bst_test_list *test_lcd_install(struct bst_test_list *tests)
{
	tests = bst_add_tests(tests, test_lcd);
	return tests;
}
