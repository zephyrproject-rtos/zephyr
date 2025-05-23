/*
 * Copyright (c) 2023 Nordic Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Composition Data Page 1 (CDP1) test
 */

#include "mesh_test.h"

#include <stddef.h>
#include <string.h>
#include <mesh/access.h>
#include <mesh/net.h>

#include <zephyr/logging/log.h>
#define LOG_MODULE_NAME test_cdp1
LOG_MODULE_REGISTER(LOG_MODULE_NAME, LOG_LEVEL_INF);

#define NODE_ADDR 0x00a1
#define WAIT_TIME 60 /* seconds */

#define TEST_MODEL_ID_1     0x2a2a
#define TEST_MODEL_ID_2     0x2b2b
#define TEST_MODEL_ID_3     0x2c2c
#define TEST_MODEL_ID_4     0x2d2d
#define TEST_MODEL_ID_5     0x2e2e
#define TEST_MODEL_ID_6     0x2f2f
#define TEST_VND_MODEL_ID_1 0x3a3a

#define TEST_MODEL_DECLARE(number)                                                                 \
	static int model_##number##_init(const struct bt_mesh_model *model);			   \
	static const struct bt_mesh_model_cb test_model_##number##_cb = {                          \
		.init = model_##number##_init,                                                     \
	};                                                                                         \
	static const struct bt_mesh_model_op model_op_##number[] = {                               \
		BT_MESH_MODEL_OP_END,                                                              \
	};

TEST_MODEL_DECLARE(1);
TEST_MODEL_DECLARE(2);
TEST_MODEL_DECLARE(3);
TEST_MODEL_DECLARE(4);
TEST_MODEL_DECLARE(5);
TEST_MODEL_DECLARE(6);
TEST_MODEL_DECLARE(vnd1);

static uint8_t app_key[16] = {0xaa};
static uint8_t net_key[16] = {0xcc};

static const struct bt_mesh_test_cfg node_1_cfg = {
	.addr = NODE_ADDR,
	.dev_key = {0xaa},
};

static struct bt_mesh_prov prov;
static struct bt_mesh_cfg_cli cfg_cli;

static const struct bt_mesh_model models_1[] = {
	BT_MESH_MODEL_CFG_SRV,
	BT_MESH_MODEL_CFG_CLI(&cfg_cli),
	BT_MESH_MODEL_CB(TEST_MODEL_ID_1, model_op_1, NULL, NULL, &test_model_1_cb),
	BT_MESH_MODEL_CB(TEST_MODEL_ID_2, model_op_2, NULL, NULL, &test_model_2_cb),
	BT_MESH_MODEL_CB(TEST_MODEL_ID_3, model_op_3, NULL, NULL, &test_model_3_cb),
};

static const struct bt_mesh_model models_2[] = {
	BT_MESH_MODEL_CB(TEST_MODEL_ID_4, model_op_4, NULL, NULL, &test_model_4_cb),
};

static const struct bt_mesh_model models_3[] = {
	BT_MESH_MODEL_CB(TEST_MODEL_ID_5, model_op_5, NULL, NULL, &test_model_5_cb),
};

static const struct bt_mesh_model models_4[] = {
	BT_MESH_MODEL_CB(TEST_MODEL_ID_6, model_op_6, NULL, NULL, &test_model_6_cb),
};

static const struct bt_mesh_model models_vnd1[] = {
	BT_MESH_MODEL_VND_CB(TEST_VND_COMPANY_ID, TEST_VND_MODEL_ID_1, model_op_vnd1, NULL, NULL,
			     &test_model_vnd1_cb),
};

static const struct bt_mesh_elem elems[] = {
	BT_MESH_ELEM(0, models_1, models_vnd1),
	BT_MESH_ELEM(1, models_2, BT_MESH_MODEL_NONE),
	BT_MESH_ELEM(2, models_3, BT_MESH_MODEL_NONE),
	BT_MESH_ELEM(3, models_3, BT_MESH_MODEL_NONE),
	BT_MESH_ELEM(4, models_4, BT_MESH_MODEL_NONE),
};

static const struct bt_mesh_comp comp = {
	.cid = TEST_VND_COMPANY_ID,
	.vid = 0xdead,
	.pid = 0xface,
	.elem = elems,
	.elem_count = ARRAY_SIZE(elems),
};

/* The extensions and correspondence between models are as follows:
 * Within elements:
 * E0: M2 extends M1. VND1 extends M2. M3 and VND1 corresponds.
 *
 * Between elements:
 * M3 on E0 extends M4 on E1.
 * M2 on E0 and M4 on E1 corresponds.
 * M6 on E4 extends M1 on E0
 */
static int model_1_init(const struct bt_mesh_model *model)
{
	return 0;
}

static int model_2_init(const struct bt_mesh_model *model)
{
	ASSERT_OK(bt_mesh_model_extend(model, bt_mesh_model_find(&elems[0], TEST_MODEL_ID_1)));
	return 0;
}

static int model_3_init(const struct bt_mesh_model *model)
{
	return 0;
}

static int model_4_init(const struct bt_mesh_model *model)
{
	ASSERT_OK(bt_mesh_model_extend(bt_mesh_model_find(&elems[0], TEST_MODEL_ID_3), model));
	ASSERT_OK(bt_mesh_model_correspond(model, bt_mesh_model_find(&elems[0], TEST_MODEL_ID_2)));
	return 0;
}

static int model_5_init(const struct bt_mesh_model *model)
{
	return 0;
}

static int model_6_init(const struct bt_mesh_model *model)
{
	ASSERT_OK(bt_mesh_model_extend(model, bt_mesh_model_find(&elems[0], TEST_MODEL_ID_1)));
	return 0;
}

static int model_vnd1_init(const struct bt_mesh_model *model)
{
	ASSERT_OK(bt_mesh_model_extend(model, bt_mesh_model_find(&elems[0], TEST_MODEL_ID_1)));
	ASSERT_OK(bt_mesh_model_correspond(model, bt_mesh_model_find(&elems[0], TEST_MODEL_ID_3)));
	return 0;
}

/* Hardcoded version of the CDP1 fields.
 * Extensions are named extending-model_base-model.
 */

static struct bt_mesh_comp_p1_ext_item test_p1_ext_mod2_mod1 = {.type = SHORT,
								.short_item = {
									.elem_offset = 0,
									.mod_item_idx = 2,
								}};

static struct bt_mesh_comp_p1_ext_item test_p1_ext_vnd1_mod1 = {.type = SHORT,
								.short_item = {
									.elem_offset = 0,
									.mod_item_idx = 2,
								}};

static struct bt_mesh_comp_p1_ext_item test_p1_ext_mod3_mod4 = {.type = SHORT,
								.short_item = {
									.elem_offset = 7,
									.mod_item_idx = 0,
								}};

static struct bt_mesh_comp_p1_ext_item test_p1_ext_mod6_mod1 = {.type = LONG,
								.long_item = {
									.elem_offset = 4,
									.mod_item_idx = 2,
								}};

static const struct bt_mesh_comp_p1_model_item test_p1_cfg_srv_mod = {
	.cor_present = 0,
	.format = 0,
	.ext_item_cnt = 0,
};

static const struct bt_mesh_comp_p1_model_item test_p1_cfg_cli_mod = {
	.cor_present = 0,
	.format = 0,
	.ext_item_cnt = 0,
};

static const struct bt_mesh_comp_p1_model_item test_p1_mod1 = {
	.cor_present = 0,
	.format = 0,
	.ext_item_cnt = 0,
};

static const struct bt_mesh_comp_p1_model_item test_p1_mod2 = {
	.cor_present = 1,
	.format = 0,
	.ext_item_cnt = 1,
	.cor_id = 1,
};

static const struct bt_mesh_comp_p1_model_item test_p1_mod3 = {
	.cor_present = 1,
	.format = 0,
	.ext_item_cnt = 1,
	.cor_id = 0,
};

static const struct bt_mesh_comp_p1_model_item test_p1_mod4 = {
	.cor_present = 1,
	.format = 0,
	.ext_item_cnt = 0,
	.cor_id = 1,
};

static const struct bt_mesh_comp_p1_model_item test_p1_mod5 = {
	.cor_present = 0,
	.format = 0,
	.ext_item_cnt = 0,
};

static const struct bt_mesh_comp_p1_model_item test_p1_mod6 = {
	.cor_present = 0,
	.format = 1,
	.ext_item_cnt = 1,
};

static const struct bt_mesh_comp_p1_model_item test_p1_vnd1 = {
	.cor_present = 1,
	.format = 0,
	.ext_item_cnt = 1,
	.cor_id = 0,
};

static const struct bt_mesh_comp_p1_model_item test_p1_elem0_models[] = {
	test_p1_cfg_srv_mod, test_p1_cfg_cli_mod, test_p1_mod1,
	test_p1_mod2,        test_p1_mod3,        test_p1_vnd1,
};

static const struct bt_mesh_comp_p1_model_item test_p1_elem1_models[] = {
	test_p1_mod4,
};

static const struct bt_mesh_comp_p1_model_item test_p1_elem2_models[] = {
	test_p1_mod5,
};

static const struct bt_mesh_comp_p1_model_item test_p1_elem3_models[] = {
	test_p1_mod5,
};

static const struct bt_mesh_comp_p1_model_item test_p1_elem4_models[] = {
	test_p1_mod6,
};

static const struct bt_mesh_comp_p1_model_item *test_p1_elem_models[] = {
	test_p1_elem0_models, test_p1_elem1_models, test_p1_elem2_models,
	test_p1_elem3_models, test_p1_elem4_models,
};

static const struct bt_mesh_comp_p1_elem test_p1_elem0 = {
	.nsig = 5,
	.nvnd = 1,
};

static const struct bt_mesh_comp_p1_elem test_p1_elem1 = {
	.nsig = 1,
	.nvnd = 0,
};

static const struct bt_mesh_comp_p1_elem test_p1_elem2 = {
	.nsig = 1,
	.nvnd = 0,
};

static const struct bt_mesh_comp_p1_elem test_p1_elem3 = {
	.nsig = 1,
	.nvnd = 0,
};

static const struct bt_mesh_comp_p1_elem test_p1_elem4 = {
	.nsig = 1,
	.nvnd = 0,
};

static struct bt_mesh_comp_p1_elem test_p1_elems[] = {
	test_p1_elem0, test_p1_elem1, test_p1_elem2, test_p1_elem3, test_p1_elem4,
};

static void provision_and_configure(struct bt_mesh_test_cfg cfg)
{
	int err;
	uint8_t status;

	err = bt_mesh_provision(net_key, 0, 0, 0, cfg.addr, cfg.dev_key);
	if (err) {
		FAIL("Provisioning failed (err %d)", err);
	}

	err = bt_mesh_cfg_cli_app_key_add(0, cfg.addr, 0, 0, app_key, &status);
	if (err || status) {
		FAIL("AppKey add failed (err %d, status %u)", err, status);
	}
}

static void verify_model_item(struct bt_mesh_comp_p1_model_item *mod_item, int elem_idx,
			      int mod_idx, int offset)
{
	ASSERT_EQUAL(test_p1_elem_models[elem_idx][mod_idx + offset].cor_present,
		     mod_item->cor_present);
	ASSERT_EQUAL(test_p1_elem_models[elem_idx][mod_idx + offset].format, mod_item->format);
	ASSERT_EQUAL(test_p1_elem_models[elem_idx][mod_idx + offset].ext_item_cnt,
		     mod_item->ext_item_cnt);
	if (mod_item->cor_present) {
		ASSERT_EQUAL(test_p1_elem_models[elem_idx][mod_idx + offset].cor_id,
			     mod_item->cor_id);
	}
}

static void verify_ext_item(struct bt_mesh_comp_p1_ext_item *ext_item, int elem_idx, int mod_idx,
			    int offset)
{
	struct bt_mesh_comp_p1_ext_item *test_p1_ext_item;

	switch (elem_idx * 100 + (mod_idx + offset)) {
	case 3: /* elem_idx=0, mod_idx=3, offset = 0 */
		test_p1_ext_item = &test_p1_ext_mod2_mod1;
		break;
	case 4: /* elem_idx=0, mod_idx=4, offset = 0 */
		test_p1_ext_item = &test_p1_ext_mod3_mod4;
		break;
	case 5: /* elem_idx=0, mod_idx=0, offset = 5 */
		test_p1_ext_item = &test_p1_ext_vnd1_mod1;
		break;
	case 400: /* elem_idx=4, mod_idx=0, offset = 0 */
		test_p1_ext_item = &test_p1_ext_mod6_mod1;
		break;
	default:
		FAIL("Unexpected call to %s (elem %d, mod %d, offset %d)", __func__, elem_idx,
		     mod_idx, offset);
	}

	ASSERT_EQUAL(test_p1_ext_item->type, ext_item->type);
	if (ext_item->type == SHORT) {
		ASSERT_EQUAL(test_p1_ext_item->short_item.elem_offset,
			     ext_item->short_item.elem_offset);
		ASSERT_EQUAL(test_p1_ext_item->short_item.mod_item_idx,
			     ext_item->short_item.mod_item_idx);
	} else {
		ASSERT_EQUAL(test_p1_ext_item->long_item.elem_offset,
			     ext_item->long_item.elem_offset);
		ASSERT_EQUAL(test_p1_ext_item->long_item.mod_item_idx,
			     ext_item->long_item.mod_item_idx);
	}
}

static void verify_cdp1(struct bt_mesh_comp_p1_elem *p1_elem,
			struct bt_mesh_comp_p1_model_item *mod_item,
			struct bt_mesh_comp_p1_ext_item *ext_item,
			struct net_buf_simple *p1_dev_comp)
{
	int elem_idx = 0;

	while (bt_mesh_comp_p1_elem_pull(p1_dev_comp, p1_elem)) {
		ASSERT_EQUAL(test_p1_elems[elem_idx].nsig, p1_elem->nsig);
		ASSERT_EQUAL(test_p1_elems[elem_idx].nvnd, p1_elem->nvnd);

		for (int mod_idx = 0; mod_idx < p1_elem->nsig; mod_idx++) {
			if (bt_mesh_comp_p1_item_pull(p1_elem, mod_item)) {
				verify_model_item(mod_item, elem_idx, mod_idx, 0);
			}

			for (int ext_mod_idx = 0; ext_mod_idx < mod_item->ext_item_cnt;
			     ext_mod_idx++) {
				bt_mesh_comp_p1_pull_ext_item(mod_item, ext_item);
				verify_ext_item(ext_item, elem_idx, mod_idx, 0);
			}
		}

		for (int mod_idx = 0; mod_idx < p1_elem->nvnd; mod_idx++) {
			if (bt_mesh_comp_p1_item_pull(p1_elem, mod_item)) {
				verify_model_item(mod_item, elem_idx, mod_idx, p1_elem->nsig);
			}

			for (int ext_mod_idx = 0; ext_mod_idx < mod_item->ext_item_cnt;
			     ext_mod_idx++) {
				bt_mesh_comp_p1_pull_ext_item(mod_item, ext_item);
				verify_ext_item(ext_item, elem_idx, mod_idx, p1_elem->nsig);
			}
		}
		elem_idx++;
	}
}

static void test_node_data_comparison(void)
{
	bt_mesh_test_cfg_set(NULL, WAIT_TIME);
	bt_mesh_device_setup(&prov, &comp);
	provision_and_configure(node_1_cfg);

	NET_BUF_SIMPLE_DEFINE(p1_dev_comp, 500);
	uint8_t page_rsp;

	ASSERT_OK(bt_mesh_cfg_cli_comp_data_get(0, node_1_cfg.addr, 1, &page_rsp, &p1_dev_comp));
	ASSERT_EQUAL(1, page_rsp);

	NET_BUF_SIMPLE_DEFINE(p1_buf, 500);
	NET_BUF_SIMPLE_DEFINE(p1_item_buf, 500);
	struct bt_mesh_comp_p1_elem p1_elem = {._buf = &p1_buf};
	struct bt_mesh_comp_p1_model_item mod_item = {._buf = &p1_item_buf};
	struct bt_mesh_comp_p1_ext_item ext_item = {0};

	verify_cdp1(&p1_elem, &mod_item, &ext_item, &p1_dev_comp);

	PASS();
}

#define TEST_CASE(role, name, description)                                                         \
	{                                                                                          \
		.test_id = "cdp1_" #role "_" #name, .test_descr = description,                     \
		.test_tick_f = bt_mesh_test_timeout, .test_main_f = test_##role##_##name,          \
	}

static const struct bst_test_instance test_cdp1[] = {
	TEST_CASE(node, data_comparison, "Compare encoded and decoded CDP1 data."),

	BSTEST_END_MARKER};

struct bst_test_list *test_cdp1_install(struct bst_test_list *tests)
{
	tests = bst_add_tests(tests, test_cdp1);
	return tests;
}
