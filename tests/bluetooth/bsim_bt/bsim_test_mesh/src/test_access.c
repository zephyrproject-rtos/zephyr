/*
 * Copyright (c) 2021 Nordic Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include "mesh_test.h"
#include "mesh/net.h"
#include "mesh/access.h"

#define LOG_MODULE_NAME test_access

#include <logging/log.h>
LOG_MODULE_REGISTER(LOG_MODULE_NAME, LOG_LEVEL_INF);

#define GROUP_ADDR 0xc000
#define UNICAST_ADDR1 0x0001
#define UNICAST_ADDR2 0x0006
#define WAIT_TIME 10 /*seconds*/

#define TEST_MODEL_ID_1 0x2b2b
#define TEST_MODEL_ID_2 0x2a2a
#define TEST_MODEL_ID_3 0x2c2c

#define TEST_MESSAGE_OP_1  BT_MESH_MODEL_OP_1(0x11)
#define TEST_MESSAGE_OP_2  BT_MESH_MODEL_OP_1(0x12)
#define TEST_MESSAGE_OP_3  BT_MESH_MODEL_OP_1(0x13)

static int model1_init(struct bt_mesh_model *model);
static int model2_init(struct bt_mesh_model *model);
static int model3_init(struct bt_mesh_model *model);
static int test_msg_handler(struct bt_mesh_model *model,
			struct bt_mesh_msg_ctx *ctx,
			struct net_buf_simple *buf);

struct k_poll_signal model_pub_signal;

static uint8_t dev_key[16] = { 0xdd };
static uint8_t app_key[16] = { 0xaa };
static uint8_t net_key[16] = { 0xcc };
static struct bt_mesh_prov prov;

static const struct bt_mesh_model_cb test_model1_cb = {
	.init = model1_init,
};

static const struct bt_mesh_model_cb test_model2_cb = {
	.init = model2_init,
};

static const struct bt_mesh_model_cb test_model3_cb = {
	.init = model3_init,
};

static const struct bt_mesh_model_op model_op1[] = {
	{ TEST_MESSAGE_OP_1, 0, test_msg_handler },
	BT_MESH_MODEL_OP_END
};

static const struct bt_mesh_model_op model_op2[] = {
	{ TEST_MESSAGE_OP_2, 0, test_msg_handler },
	BT_MESH_MODEL_OP_END
};

static const struct bt_mesh_model_op model_op3[] = {
	{ TEST_MESSAGE_OP_3, 0, test_msg_handler },
	BT_MESH_MODEL_OP_END
};

static struct bt_mesh_cfg_cli cfg_cli;

static struct bt_mesh_model models[] = {
	BT_MESH_MODEL_CFG_SRV,
	BT_MESH_MODEL_CFG_CLI(&cfg_cli),
	BT_MESH_MODEL_CB(TEST_MODEL_ID_1, model_op1, NULL, NULL, &test_model1_cb),
	BT_MESH_MODEL_CB(TEST_MODEL_ID_2, model_op2, NULL, NULL, &test_model2_cb),
	BT_MESH_MODEL_CB(TEST_MODEL_ID_3, model_op3, NULL, NULL, &test_model3_cb),
};

static struct bt_mesh_model vnd_models[] = {};

static struct bt_mesh_elem elems[] = {
	BT_MESH_ELEM(0, models, vnd_models),
};

const struct bt_mesh_comp local_comp = {
	.elem = elems,
	.elem_count = ARRAY_SIZE(elems),
};

static int model1_init(struct bt_mesh_model *model)
{
	return bt_mesh_model_extend(&models[2], &models[3]);
}

static int model2_init(struct bt_mesh_model *model)
{
	return bt_mesh_model_extend(&models[3], &models[4]);
}

static int model3_init(struct bt_mesh_model *model)
{
	return 0;
}

static int test_msg_handler(struct bt_mesh_model *model,
			struct bt_mesh_msg_ctx *ctx,
			struct net_buf_simple *buf)
{
	LOG_DBG("msg rx model id: %u", model->id);
	k_poll_signal_raise(&model_pub_signal, model->id);

	return 0;
}

static void provision(uint16_t addr)
{
	int err;

	err = bt_mesh_provision(net_key, 0, 0, 0, addr, dev_key);
	if (err) {
		FAIL("Provisioning failed (err %d)", err);
		return;
	}
}

static void common_configure(uint16_t addr)
{
	uint8_t status;
	int err;
	uint16_t models[] = {TEST_MODEL_ID_1, TEST_MODEL_ID_2, TEST_MODEL_ID_3};

	err = bt_mesh_cfg_app_key_add(0, addr, 0, 0, app_key,
				      &status);
	if (err || status) {
		FAIL("AppKey add failed (err %d, status %u)", err, status);
		return;
	}

	for (int i = 0; i < ARRAY_SIZE(models); i++) {
		err = bt_mesh_cfg_mod_app_bind(0, addr, addr, 0, models[i],
					       &status);
		if (err || status) {
			FAIL("Model %#4x bind failed (err %d, status %u)",
					models[i], err, status);
			return;
		}
	}

	err = bt_mesh_cfg_net_transmit_set(0, addr,
					   BT_MESH_TRANSMIT(2, 20), &status);
	if (err || status != BT_MESH_TRANSMIT(2, 20)) {
		FAIL("Net transmit set failed (err %d, status %u)", err,
		     status);
		return;
	}
}

static void subscription_configure(uint16_t addr)
{
	uint8_t status;
	int err;

	err = bt_mesh_cfg_mod_sub_add(0, addr, addr, GROUP_ADDR, TEST_MODEL_ID_2, &status);

	if (err || status) {
		FAIL("Model %#4x subscription configuration failed (err %d, status %u)",
				TEST_MODEL_ID_2, err, status);
		return;
	}
}

static void test_tx_ext_model(void)
{
	bt_mesh_test_cfg_set(NULL, WAIT_TIME);
	bt_mesh_device_setup(&prov, &local_comp);
	provision(UNICAST_ADDR1);
	common_configure(UNICAST_ADDR1);

	struct bt_mesh_msg_ctx ctx = {
		.net_idx = 0,
		.app_idx = 0,
		.addr = GROUP_ADDR,
		.send_rel = false,
		.send_ttl = BT_MESH_TTL_DEFAULT,
	};
	BT_MESH_MODEL_BUF_DEFINE(msg, TEST_MESSAGE_OP_1, 0);

	bt_mesh_model_msg_init(&msg, TEST_MESSAGE_OP_1);
	bt_mesh_model_send(&models[2], &ctx, &msg, NULL, NULL);

	bt_mesh_model_msg_init(&msg, TEST_MESSAGE_OP_2);
	bt_mesh_model_send(&models[3], &ctx, &msg, NULL, NULL);

	bt_mesh_model_msg_init(&msg, TEST_MESSAGE_OP_3);
	bt_mesh_model_send(&models[4], &ctx, &msg, NULL, NULL);

	PASS();
}

static void test_sub_ext_model(void)
{
	k_poll_signal_init(&model_pub_signal);

	struct k_poll_event events[1] = {
		K_POLL_EVENT_INITIALIZER(K_POLL_TYPE_SIGNAL,
			K_POLL_MODE_NOTIFY_ONLY, &model_pub_signal)
	};

	bt_mesh_test_cfg_set(NULL, WAIT_TIME);
	bt_mesh_device_setup(&prov, &local_comp);
	provision(UNICAST_ADDR2);
	common_configure(UNICAST_ADDR2);
	subscription_configure(UNICAST_ADDR2);

	bool m1_fired = false;
	bool m2_fired = false;
	bool m3_fired = false;

	while (!m1_fired || !m2_fired || !m3_fired) {
		ASSERT_OK(k_poll(events, 1, K_SECONDS(3)));

		switch (model_pub_signal.result) {
		case TEST_MODEL_ID_1:
			ASSERT_FALSE(m1_fired);
			m1_fired = true;
			break;
		case TEST_MODEL_ID_2:
			ASSERT_FALSE(m2_fired);
			m2_fired = true;
			break;
		case TEST_MODEL_ID_3:
			ASSERT_FALSE(m3_fired);
			m3_fired = true;
			break;
		default:
			FAIL();
			break;
		}

		events[0].signal->signaled = 0;
		events[0].state = K_POLL_STATE_NOT_READY;
	}


	PASS();
}

#define TEST_CASE(role, name, description)                     \
	{                                                      \
		.test_id = "access_" #role "_" #name,          \
		.test_descr = description,                     \
		.test_tick_f = bt_mesh_test_timeout,           \
		.test_main_f = test_##role##_##name,           \
	}

static const struct bst_test_instance test_access[] = {
	TEST_CASE(tx, ext_model, "Access: tx data of extended models"),

	TEST_CASE(sub, ext_model, "Access: data subscription of extended models"),
	BSTEST_END_MARKER
};

struct bst_test_list *test_access_install(struct bst_test_list *tests)
{
	tests = bst_add_tests(tests, test_access);
	return tests;
}
