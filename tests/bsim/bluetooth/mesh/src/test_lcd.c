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

#include <zephyr/logging/log.h>

#include <mesh/access.h>
#include <mesh/net.h>

LOG_MODULE_REGISTER(test_lcd, LOG_LEVEL_INF);

#define CLI_ADDR  0x7728
#define SRV_ADDR  0x18f8
#define WAIT_TIME 60 /* seconds */

/* Length of additional status fields (offset, page and total size) */
#define LCD_STATUS_FIELDS_LEN 5
#define DUMMY_2_BYTE_OP BT_MESH_MODEL_OP_2(0xff, 0xff)
#define BT_MESH_LCD_PAYLOAD_MAX                                                                    \
	(BT_MESH_TX_SDU_MAX - BT_MESH_MODEL_OP_LEN(DUMMY_2_BYTE_OP) -                              \
	 LCD_STATUS_FIELDS_LEN -                                                                   \
	 BT_MESH_MIC_SHORT) /* 378 bytes */

#define TEST_MODEL_CNT_CB(_dummy_op, _metadata) \
{                                               \
	.id = 0x1234,                               \
	.pub = NULL,                                \
	.keys = NULL,                               \
	.keys_cnt = 0,                              \
	.groups = NULL,                             \
	.groups_cnt = 0,                            \
	.op = _dummy_op,                            \
	.cb = NULL,                                 \
	.user_data = NULL,                          \
	.metadata = _metadata,			    \
	.ctx = &(struct bt_mesh_model_ctx){ 0 },    \
}

const struct bt_mesh_model_op dummy_op[] = {
	{ 0xfeed, BT_MESH_LEN_MIN(1), NULL },
	{ 0xface, BT_MESH_LEN_MIN(1), NULL },
	BT_MESH_MODEL_OP_END,
};

static struct bt_mesh_models_metadata_entry *dummy_meta_entry[] = {};

/* Empty elements to create large composition/meta data */
#define DUMMY_ELEM(i, _) BT_MESH_ELEM((i) + 2,	\
	MODEL_LIST(TEST_MODEL_CNT_CB(dummy_op, dummy_meta_entry)), BT_MESH_MODEL_NONE)

static const struct bt_mesh_test_cfg cli_cfg = {
	.addr = CLI_ADDR,
	.dev_key = {0xaa},
};

static const struct bt_mesh_test_cfg srv_cfg = {
	.addr = SRV_ADDR,
	.dev_key = {0xab},
};

static struct bt_mesh_prov prov;
static struct bt_mesh_cfg_cli cfg_cli;
static struct bt_mesh_large_comp_data_cli lcd_cli;

/* Creates enough composition data to send a max SDU comp status message + 1 byte */
static const struct bt_mesh_elem elements_1[] = {
	BT_MESH_ELEM(1,
		     MODEL_LIST(BT_MESH_MODEL_CFG_SRV,
				BT_MESH_MODEL_CFG_CLI(&cfg_cli),
				BT_MESH_MODEL_LARGE_COMP_DATA_CLI(&lcd_cli),
				BT_MESH_MODEL_LARGE_COMP_DATA_SRV),
		     BT_MESH_MODEL_NONE),
	LISTIFY(88, DUMMY_ELEM, (,)),
};

/* Creates enough metadata to send a max SDU metadata status message + 1 byte */
static const struct bt_mesh_elem elements_2[] = {
	BT_MESH_ELEM(1,
		     MODEL_LIST(BT_MESH_MODEL_CFG_SRV,
				BT_MESH_MODEL_CFG_CLI(&cfg_cli),
				BT_MESH_MODEL_LARGE_COMP_DATA_CLI(&lcd_cli),
				BT_MESH_MODEL_LARGE_COMP_DATA_SRV),
		     BT_MESH_MODEL_NONE),
	LISTIFY(186, DUMMY_ELEM, (,)),
};

static const struct bt_mesh_comp comp_1 = {
	.cid = TEST_VND_COMPANY_ID,
	.vid = 0xabba,
	.pid = 0xdead,
	.elem = elements_1,
	.elem_count = ARRAY_SIZE(elements_1),
};

static const struct bt_mesh_comp comp_2 = {
	.cid = TEST_VND_COMPANY_ID,
	.elem = elements_2,
	.elem_count = ARRAY_SIZE(elements_2),
};

static void prov_and_conf(struct bt_mesh_test_cfg cfg)
{
	uint8_t status;

	ASSERT_OK(bt_mesh_provision(test_net_key, 0, 0, 0, cfg.addr, cfg.dev_key));

	/* Check device key by adding appkey. */
	ASSERT_OK(bt_mesh_cfg_cli_app_key_add(0, cfg.addr, 0, 0, test_app_key, &status));
	ASSERT_OK(status);
}

/* Since nodes self-provision in this test and the LCD model uses device keys for crypto, the
 * server node must be added to the client CDB manually.
 */
static void target_node_alloc(struct bt_mesh_comp comp, struct bt_mesh_test_cfg cfg)
{
	struct bt_mesh_cdb_node *node;

	node = bt_mesh_cdb_node_alloc(test_va_uuid, cfg.addr, comp.elem_count, 0);
	ASSERT_TRUE(node);
	memcpy(node->dev_key, cfg.dev_key, 16);
}

/* Assert equality between local data and merged sample data */
static void merge_and_compare_assert(struct net_buf_simple *sample1, struct net_buf_simple *sample2,
				     struct net_buf_simple *local_data)
{
	uint8_t merged_data[sample1->len + sample2->len];

	memcpy(&merged_data[0], sample1->data, sample1->len);
	memcpy(&merged_data[sample1->len], sample2->data, sample2->len);
	ASSERT_TRUE(memcmp(local_data->data, merged_data, ARRAY_SIZE(merged_data)) == 0);
}

/* Assert that the received status fields are equal to local values. Buffer state is saved.
 */
static void verify_status_fields(struct bt_mesh_large_comp_data_rsp *srv_rsp, uint8_t page_local,
				 uint16_t offset_local, uint16_t total_size_local)
{
	ASSERT_EQUAL(page_local, srv_rsp->page);
	ASSERT_EQUAL(offset_local, srv_rsp->offset);
	ASSERT_EQUAL(total_size_local, srv_rsp->total_size);
}

/* Compare response data with local data.
 * Note:
 * * srv_rsp: Status field data (5 bytes) is removed form buffer.
 * * local_data: state is preserved.
 * * prev_len: Set to NULL if irrelevant. Used for split and merge testing.
 */
static void rsp_equals_local_data_assert(uint16_t addr, struct bt_mesh_large_comp_data_rsp *srv_rsp,
					 struct net_buf_simple *local_data, uint8_t page,
					 uint16_t offset, uint16_t total_size, uint16_t *prev_len)
{
	struct net_buf_simple_state local_state = {0};

	/* Check that status field data matches local values. */
	verify_status_fields(srv_rsp, page, offset, total_size);

	net_buf_simple_save(local_data, &local_state);

	if (prev_len != NULL) {
		size_t len = *prev_len;

		net_buf_simple_pull_mem(local_data, len);
	}

	/* Check that local and rsp data are equal */
	ASSERT_TRUE(memcmp(srv_rsp->data->data, local_data->data, srv_rsp->data->len) == 0);

	net_buf_simple_restore(local_data, &local_state);
}

static void test_srv_init(void)
{
	bt_mesh_test_cfg_set(&srv_cfg, WAIT_TIME);
}

static void test_cli_init(void)
{
	bt_mesh_test_cfg_set(&cli_cfg, WAIT_TIME);
}

static void test_cli_max_sdu_comp_data_request(void)
{
	int err;
	uint8_t page = 0;
	uint16_t offset, total_size;

	NET_BUF_SIMPLE_DEFINE(local_comp, 500);
	NET_BUF_SIMPLE_DEFINE(srv_rsp_comp, 500);
	net_buf_simple_init(&local_comp, 0);
	net_buf_simple_init(&srv_rsp_comp, 0);

	struct bt_mesh_large_comp_data_rsp srv_rsp = {
		.data = &srv_rsp_comp,
	};

	bt_mesh_device_setup(&prov, &comp_1);
	prov_and_conf(cli_cfg);
	target_node_alloc(comp_1, srv_cfg);

	/* Note: an offset of 3 is necessary with the status data to be exactly
	 * 380 bytes of access payload.
	 */
	offset = 3;

	/* Get local data */
	err = bt_mesh_comp_data_get_page_0(&local_comp, offset);
	/* Operation is successful even if all data cannot fit in the buffer (-E2BIG) */
	if (err && err != -E2BIG) {
		FAIL("CLIENT: Failed to get comp data Page 0: %d", err);
	}
	total_size = bt_mesh_comp_page_0_size();

	/* Get server composition data and check integrity */
	ASSERT_OK(bt_mesh_large_comp_data_get(0, SRV_ADDR, page, offset, &srv_rsp));
	ASSERT_EQUAL(srv_rsp_comp.len, BT_MESH_LCD_PAYLOAD_MAX);
	rsp_equals_local_data_assert(SRV_ADDR, &srv_rsp, &local_comp, page, offset, total_size,
				     NULL);

	PASS();
}

static void test_cli_split_comp_data_request(void)
{
	int err;
	uint8_t page = 0;
	uint16_t offset, total_size, prev_len = 0;

	NET_BUF_SIMPLE_DEFINE(local_comp, 200);
	NET_BUF_SIMPLE_DEFINE(srv_rsp_comp_1, 64);
	NET_BUF_SIMPLE_DEFINE(srv_rsp_comp_2, 64);
	net_buf_simple_init(&local_comp, 0);
	net_buf_simple_init(&srv_rsp_comp_1, 0);
	net_buf_simple_init(&srv_rsp_comp_2, 0);

	struct bt_mesh_large_comp_data_rsp srv_rsp_1 = {
		.data = &srv_rsp_comp_1,
	};
	struct bt_mesh_large_comp_data_rsp srv_rsp_2 = {
		.data = &srv_rsp_comp_2,
	};

	bt_mesh_device_setup(&prov, &comp_1);
	prov_and_conf(cli_cfg);
	target_node_alloc(comp_1, srv_cfg);

	offset = 0;

	/* Get local data */
	err = bt_mesh_comp_data_get_page_0(&local_comp, offset);
	/* Operation is successful even if all data cannot fit in the buffer (-E2BIG) */
	if (err && err != -E2BIG) {
		FAIL("CLIENT: Failed to get comp data Page 0: %d", err);
	}
	total_size = bt_mesh_comp_page_0_size();

	/* Get first server composition data sample and verify data */
	ASSERT_OK(bt_mesh_large_comp_data_get(0, SRV_ADDR, page, offset, &srv_rsp_1));
	rsp_equals_local_data_assert(SRV_ADDR, &srv_rsp_1, &local_comp, page, offset, total_size,
				     &prev_len);

	prev_len = srv_rsp_comp_1.len;
	offset += prev_len;

	/* Get next server composition data sample */
	ASSERT_OK(bt_mesh_large_comp_data_get(0, SRV_ADDR, page, offset, &srv_rsp_2));
	rsp_equals_local_data_assert(SRV_ADDR, &srv_rsp_2, &local_comp, page, offset, total_size,
				     &prev_len);

	/* Check data integrity of merged sample data */
	merge_and_compare_assert(&srv_rsp_comp_1, &srv_rsp_comp_2, &local_comp);

	PASS();
}

static void test_cli_max_sdu_metadata_request(void)
{
	int err;
	uint8_t page = 0;
	uint16_t offset, total_size;

	NET_BUF_SIMPLE_DEFINE(local_metadata, 500);
	NET_BUF_SIMPLE_DEFINE(srv_rsp_metadata, 500);
	net_buf_simple_init(&local_metadata, 0);
	net_buf_simple_init(&srv_rsp_metadata, 0);

	struct bt_mesh_large_comp_data_rsp srv_rsp = {
		.data = &srv_rsp_metadata,
	};

	bt_mesh_device_setup(&prov, &comp_2);
	prov_and_conf(cli_cfg);
	target_node_alloc(comp_2, srv_cfg);

	/* Note: an offset of 4 is necessary for the status data to be exactly
	 * 380 bytes of access payload.
	 */
	offset = 4;

	/* Get local data */
	err = bt_mesh_metadata_get_page_0(&local_metadata, offset);
	/* Operation is successful even if all data cannot fit in the buffer (-E2BIG) */
	if (err && err != -E2BIG) {
		FAIL("CLIENT: Failed to get Models Metadata Page 0: %d", err);
	}
	total_size = bt_mesh_metadata_page_0_size();

	/* Get server metadata and check integrity */
	ASSERT_OK(bt_mesh_models_metadata_get(0, SRV_ADDR, page, offset, &srv_rsp));
	ASSERT_EQUAL(srv_rsp_metadata.len, BT_MESH_LCD_PAYLOAD_MAX);
	rsp_equals_local_data_assert(SRV_ADDR, &srv_rsp, &local_metadata, page, offset, total_size,
				     NULL);

	PASS();
}

static void test_cli_split_metadata_request(void)
{
	uint8_t page = 0;
	uint16_t offset, total_size, prev_len = 0;

	NET_BUF_SIMPLE_DEFINE(local_metadata, 500);
	NET_BUF_SIMPLE_DEFINE(srv_rsp_metadata_1, 64);
	NET_BUF_SIMPLE_DEFINE(srv_rsp_metadata_2, 64);
	net_buf_simple_init(&local_metadata, 0);
	net_buf_simple_init(&srv_rsp_metadata_1, 0);
	net_buf_simple_init(&srv_rsp_metadata_2, 0);

	struct bt_mesh_large_comp_data_rsp srv_rsp_1 = {
		.data = &srv_rsp_metadata_1,
	};
	struct bt_mesh_large_comp_data_rsp srv_rsp_2 = {
		.data = &srv_rsp_metadata_2,
	};

	bt_mesh_device_setup(&prov, &comp_2);
	prov_and_conf(cli_cfg);
	target_node_alloc(comp_2, srv_cfg);

	offset = 0;

	/* Get local data */
	int err = bt_mesh_metadata_get_page_0(&local_metadata, offset);
	/* Operation is successful even if not all metadata could fit in the buffer (-E2BIG) */
	if (err && err != -E2BIG) {
		FAIL("CLIENT: Failed to get Models Metadata Page 0: %d", err);
	}
	total_size = bt_mesh_metadata_page_0_size();

	/* Get first server composition data sample and check integrity */
	ASSERT_OK(bt_mesh_models_metadata_get(0, SRV_ADDR, page, offset, &srv_rsp_1));
	rsp_equals_local_data_assert(SRV_ADDR, &srv_rsp_1, &local_metadata, page, offset,
				     total_size, &prev_len);

	prev_len = srv_rsp_metadata_1.len;
	offset += prev_len;

	/* Get next server composition data sample and check integrity */
	ASSERT_OK(bt_mesh_models_metadata_get(0, SRV_ADDR, page, offset, &srv_rsp_2));
	rsp_equals_local_data_assert(SRV_ADDR, &srv_rsp_2, &local_metadata, page, offset,
				     total_size, &prev_len);

	/* Check data integrity of merged sample data */
	merge_and_compare_assert(&srv_rsp_metadata_1, &srv_rsp_metadata_2, &local_metadata);

	PASS();
}

static void test_srv_comp_data_status_respond(void)
{
	bt_mesh_device_setup(&prov, &comp_1);
	prov_and_conf(srv_cfg);

	/* No server callback available. Wait 10 sec for message to be recived */
	k_sleep(K_SECONDS(10));

	PASS();
}

static void test_srv_metadata_status_respond(void)
{
	bt_mesh_device_setup(&prov, &comp_2);
	prov_and_conf(srv_cfg);

	if (atomic_test_bit(bt_mesh.flags, BT_MESH_METADATA_DIRTY)) {
		FAIL("Metadata is dirty. Test is not suited for this purpose.");
	}

	/* No server callback available. Wait 10 sec for message to be recived */
	k_sleep(K_SECONDS(10));

	PASS();
}

#define TEST_CASE(role, name, description)                                            \
	{                                                                                 \
		.test_id = "lcd_" #role "_" #name,                                            \
		.test_descr = description,                                                    \
		.test_tick_f = bt_mesh_test_timeout,                                          \
		.test_post_init_f = test_##role##_init,                                       \
		.test_main_f = test_##role##_##name,                                          \
	}

static const struct bst_test_instance test_lcd[] = {
	TEST_CASE(cli, max_sdu_comp_data_request, "Request comp data with max SDU length"),
	TEST_CASE(cli, split_comp_data_request, "Request continuous comp data in two samples."),
	TEST_CASE(cli, max_sdu_metadata_request, "Request metadata with max SDU length"),
	TEST_CASE(cli, split_metadata_request, "Request continuous metadata in two samples."),

	TEST_CASE(srv, comp_data_status_respond, "Process incoming GET LCD messages."),
	TEST_CASE(srv, metadata_status_respond, "Process incoming GET metadata messages."),

	BSTEST_END_MARKER};

struct bst_test_list *test_lcd_install(struct bst_test_list *tests)
{
	tests = bst_add_tests(tests, test_lcd);
	return tests;
}
