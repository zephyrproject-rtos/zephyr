/*
 * Copyright (c) 2023 Nordic Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#include "mesh_test.h"

#include <mesh/access.h>
#include <mesh/net.h>
#include <zephyr/kernel.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(test_suspend, LOG_LEVEL_INF);

#define WAIT_TIME        60 /* seconds */
#define SUSPEND_DURATION 15 /* seconds */
#define NUM_PUB           4 /* Times the DUT will publish per interval. */

#define TEST_MODEL_ID_1 0x2a2a
#define TEST_MODEL_ID_2 0x2b2b
#define TEST_MESSAGE_OP 0x1f

enum dut_mesh_status {
	DUT_SUSPENDED,
	DUT_RUNNING,
};

static int model_1_init(const struct bt_mesh_model *model);
static int model_2_init(const struct bt_mesh_model *model);

static uint8_t app_key[16] = {0xaa};
static uint8_t net_key[16] = {0xcc};

static const struct bt_mesh_test_cfg dut_cfg = {
	.addr = 0x00a0,
	.dev_key = {0x01},
};
static const struct bt_mesh_test_cfg tester_cfg = {
	.addr = 0x00b0,
	.dev_key = {0x02},
};

static struct bt_mesh_prov prov;
static struct k_sem publish_sem;
static enum dut_mesh_status dut_status;

static int model_1_update(const struct bt_mesh_model *model)
{
	model->pub->msg->data[1]++;
	LOG_DBG("Model 1 publishing..., n: %d", model->pub->msg->data[1]);
	k_sem_give(&publish_sem);
	return 0;
}

static int msg_handler(const struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
			    struct net_buf_simple *buf)
{
	static uint8_t prev_num;
	static int64_t uptime;
	uint8_t num = net_buf_simple_pull_u8(buf);

	LOG_DBG("Received msg, n: %d", num);

	/* Ensure that payload changes. */
	ASSERT_TRUE(prev_num != num);
	prev_num = num;

	/* Ensure that no message is received while Mesh is suspended or disabled.
	 * A publication may be sent just before DUT is suspended, which is ignored.
	 */
	if ((dut_status == DUT_SUSPENDED) && !(num == (NUM_PUB + 1))) {
		if (SUSPEND_DURATION * 1000ll <= k_uptime_delta(&uptime)) {
			dut_status = DUT_RUNNING;
			LOG_DBG("Suspend duration passed. Setting status to %d.", dut_status);
		} else {
			FAIL("Received publication while Mesh is suspended.");
		}
	}

	if (num == NUM_PUB) {
		dut_status = DUT_SUSPENDED;
		LOG_DBG("Expected number of pubs received. Setting status to %d.", dut_status);
		uptime = k_uptime_get();
	}

	k_sem_give(&publish_sem);
	return 0;
}

BT_MESH_MODEL_PUB_DEFINE(model_1_pub, model_1_update, 4);

static const struct bt_mesh_model_cb model_1_cb = {
	.init = model_1_init,
};

static const struct bt_mesh_model_cb model_2_cb = {
	.init = model_2_init,
};

static const struct bt_mesh_model_op model_op_1[] = {BT_MESH_MODEL_OP_END};

static const struct bt_mesh_model_op model_op_2[] = {{TEST_MESSAGE_OP, 0, msg_handler},
						     BT_MESH_MODEL_OP_END};

static struct bt_mesh_cfg_cli cfg_cli_dut;
static struct bt_mesh_model dut_models[] = {
	BT_MESH_MODEL_CFG_SRV,
	BT_MESH_MODEL_CFG_CLI(&cfg_cli_dut),
	BT_MESH_MODEL_CB(TEST_MODEL_ID_1, model_op_1, &model_1_pub, NULL, &model_1_cb),
};

static struct bt_mesh_elem dut_elems[] = {
	BT_MESH_ELEM(0, dut_models, BT_MESH_MODEL_NONE),
};

static const struct bt_mesh_comp dut_comp = {
	.cid = TEST_VND_COMPANY_ID,
	.vid = 0xeeee,
	.pid = 0xaaaa,
	.elem = dut_elems,
	.elem_count = ARRAY_SIZE(dut_elems),
};

static struct bt_mesh_cfg_cli cfg_cli_tester;
static struct bt_mesh_model tester_models[] = {
	BT_MESH_MODEL_CFG_SRV,
	BT_MESH_MODEL_CFG_CLI(&cfg_cli_tester),
	BT_MESH_MODEL_CB(TEST_MODEL_ID_2, model_op_2, NULL, NULL, &model_2_cb),
};

static struct bt_mesh_elem tester_elems[] = {
	BT_MESH_ELEM(0, tester_models, BT_MESH_MODEL_NONE),
};

static const struct bt_mesh_comp tester_comp = {
	.cid = TEST_VND_COMPANY_ID,
	.vid = 0xbaaa,
	.pid = 0xb000,
	.elem = tester_elems,
	.elem_count = ARRAY_SIZE(tester_elems),
};

static int model_1_init(const struct bt_mesh_model *model)
{
	bt_mesh_model_msg_init(model->pub->msg, TEST_MESSAGE_OP);
	net_buf_simple_add_u8(model->pub->msg, 1);

	return 0;
}

static int model_2_init(const struct bt_mesh_model *model)
{
	return 0;
}

static void provision_and_configure(struct bt_mesh_test_cfg cfg, bool tester)
{
	int err;
	uint8_t status;

	ASSERT_OK(bt_mesh_provision(net_key, 0, 0, 0, cfg.addr, cfg.dev_key));

	err = bt_mesh_cfg_cli_app_key_add(0, cfg.addr, 0, 0, app_key, &status);
	if (err || status) {
		FAIL("AppKey add failed (err %d, status %u)", err, status);
	}

	const struct bt_mesh_test_cfg *pcfg = tester ? &tester_cfg : &dut_cfg;
	uint16_t model_id = tester ? TEST_MODEL_ID_2 : TEST_MODEL_ID_1;

	err = bt_mesh_cfg_cli_mod_app_bind(0, pcfg->addr, pcfg->addr, 0, model_id, &status);
	if (err || status) {
		FAIL("Model %#4x bind failed (err %d, status %u)", model_id, err, status);
	}
}

struct bt_mesh_cfg_cli_mod_pub pub_params = {
	.addr = tester_cfg.addr,
	.uuid = NULL,
	.cred_flag = false,
	.app_idx = 0,
	.ttl = 5,
	.period = BT_MESH_PUB_PERIOD_SEC(1),
	.transmit = 0,
};

static void dut_pub_common(bool disable_bt)
{
	bt_mesh_test_cfg_set(NULL, WAIT_TIME);
	bt_mesh_device_setup(&prov, &dut_comp);
	provision_and_configure(dut_cfg, 0);

	k_sem_init(&publish_sem, 0, 1);
	uint8_t status;
	int err;

	err = bt_mesh_cfg_cli_mod_pub_set(0, dut_cfg.addr, dut_cfg.addr, TEST_MODEL_ID_1,
					&pub_params, &status);
	if (err || status) {
		FAIL("Mod pub set failed (err %d, status %u)", err, status);
	}

	/* Wait until node has published before suspending. */
	for (int i = 0; i < NUM_PUB; i++) {
		ASSERT_OK_MSG(k_sem_take(&publish_sem, K_SECONDS(30)), "Pub timed out");
	}

	ASSERT_OK_MSG(bt_mesh_suspend(), "Failed to suspend Mesh.");

	if (disable_bt) {
		ASSERT_OK_MSG(bt_disable(), "Failed to disable Bluetooth.");
	}

	k_sleep(K_SECONDS(SUSPEND_DURATION));

	if (disable_bt) {
		ASSERT_OK_MSG(bt_enable(NULL), "Failed to enable Bluetooth.");
	}

	ASSERT_OK_MSG(bt_mesh_resume(), "Failed to resume Mesh.");

	for (int i = 0; i < NUM_PUB; i++) {
		ASSERT_OK_MSG(k_sem_take(&publish_sem, K_SECONDS(30)), "Pub timed out");
	}

	/* Allow publishing to finish before suspending. */
	k_sleep(K_MSEC(100));
	ASSERT_OK(bt_mesh_suspend());
}

static void test_dut_suspend_resume(void)
{
	dut_pub_common(false);
	PASS();
}

static void test_dut_suspend_disable_resume(void)
{
	dut_pub_common(true);
	PASS();
}

static void test_tester_pub(void)
{
	bt_mesh_test_cfg_set(NULL, WAIT_TIME);
	bt_mesh_device_setup(&prov, &tester_comp);
	provision_and_configure(tester_cfg, 1);
	k_sem_init(&publish_sem, 0, 1);
	dut_status = DUT_RUNNING;

	/* Receive messages before and after suspending. A publication may get lost
	 * when suspending immeditiately after publication, thus the "-1".
	 */
	for (int i = 0; i < NUM_PUB * 2 - 1; i++) {
		ASSERT_OK_MSG(k_sem_take(&publish_sem, K_SECONDS(30)), "Receiver timed out");
	}

	PASS();
}

#define TEST_CASE(role, name, description)                                                         \
	{                                                                                          \
		.test_id = "suspend_" #role "_" #name, .test_descr = description,                  \
		.test_tick_f = bt_mesh_test_timeout, .test_main_f = test_##role##_##name,          \
	}

static const struct bst_test_instance test_suspend[] = {
	TEST_CASE(dut, suspend_resume,
			 "Suspend and resume Mesh while publishing periodically"),
	TEST_CASE(dut, suspend_disable_resume,
			 "Suspend and resume Mesh (and disable/enable BT) while publishing periodically"),

	TEST_CASE(tester, pub, "Scan and verify behavior of periodic publishing adv"),

	BSTEST_END_MARKER};

struct bst_test_list *test_suspend_install(struct bst_test_list *tests)
{
	tests = bst_add_tests(tests, test_suspend);
	return tests;
}
