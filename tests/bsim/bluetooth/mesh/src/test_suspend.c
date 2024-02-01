/*
 * Copyright (c) 2023 Nordic Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#include "mesh_test.h"
#include "gatt_common.h"

#include <mesh/access.h>
#include <mesh/net.h>

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

static uint8_t test_prov_uuid[16] = { 0x6c, 0x69, 0x6e, 0x67, 0x61, 0xaa };
static struct bt_mesh_prov prov = {
		.uuid = test_prov_uuid,
	};
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

extern const struct bt_mesh_comp comp;
/* For legacy adv, pb-gatt advs are sent with a 1000ms interval. For ext adv, they are sent
 * with a 100ms interval.
 */
static struct bt_mesh_test_gatt gatt_param = {
#if defined(CONFIG_BT_EXT_ADV)
	/* (total transmit duration) / (transmit interval) */
	.transmits = 1500 / 100,
	.interval = 100,
#else
	.transmits = 2000 / 1000,
	.interval = 1000,
#endif
	.service = MESH_SERVICE_PROVISIONING,
};

static bool gatt_check_rx_count(uint8_t transmit)
{
	static int cnt;

	LOG_DBG("rx: cnt(%d)", cnt);
	cnt++;

	if (cnt >= transmit) {
		cnt = 0;
		return true;
	}

	return false;
}

static void gatt_scan_cb(const bt_addr_le_t *addr, int8_t rssi,
			  uint8_t adv_type, struct net_buf_simple *buf)
{
	if (adv_type != BT_GAP_ADV_TYPE_ADV_IND) {
		return;
	}

	/* Ensure that no message is received while Mesh is suspended or disabled. */
	ASSERT_FALSE_MSG(dut_status == DUT_SUSPENDED, "Received adv while Mesh is suspended.");

	bt_mesh_test_parse_mesh_gatt_preamble(buf);

	if (gatt_param.service == MESH_SERVICE_PROVISIONING) {
		LOG_DBG("Parsing pb_gatt adv");
		bt_mesh_test_parse_mesh_pb_gatt_service(buf);
	} else {
		LOG_DBG("Parsing proxy adv");
		bt_mesh_test_parse_mesh_proxy_service(buf);
	}

	if (gatt_check_rx_count(gatt_param.transmits)) {
		LOG_DBG("rx completed, stopping scan...");
		k_sem_give(&publish_sem);
	}
}

static void suspend_state_change_cb(const bt_addr_le_t *addr, int8_t rssi, uint8_t adv_type,
			struct net_buf_simple *buf)
{
	uint8_t length;

	if (adv_type != BT_GAP_ADV_TYPE_ADV_NONCONN_IND) {
		return;
	}

	length = net_buf_simple_pull_u8(buf);
	ASSERT_EQUAL(buf->len, length);
	ASSERT_EQUAL(length, sizeof(uint8_t) + sizeof(enum dut_mesh_status));
	ASSERT_EQUAL(BT_DATA_MESH_MESSAGE, net_buf_simple_pull_u8(buf));

	enum dut_mesh_status *msg_status =
		net_buf_simple_pull_mem(buf, sizeof(enum dut_mesh_status));

	if ((*msg_status == DUT_RUNNING) || (*msg_status == DUT_SUSPENDED)) {
		dut_status = *msg_status;
	} else {
		FAIL("Received unexpected data");
	}

	LOG_DBG("Received %d from DUT, setting status to %s",
		*msg_status, (dut_status == DUT_SUSPENDED) ? "true" : "false");
	k_sem_give(&publish_sem);
}

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

static void dut_gatt_common(bool disable_bt)
{
	bt_mesh_test_cfg_set(NULL, WAIT_TIME);
	bt_mesh_device_setup(&prov, &comp);
	ASSERT_OK_MSG(bt_mesh_prov_enable(BT_MESH_PROV_GATT), "Failed to enable GATT provisioner");
	dut_status = DUT_RUNNING;
	/* Let the Tester observe pb gatt advertisements before provisioning. The node should
	 * advertise pb gatt service with 100 msec (ext adv) or 1000msec (legacy adv) interval.
	 */
	k_sleep(K_MSEC(1800));

	ASSERT_OK(bt_mesh_provision(test_net_key, 0, 0, 0, dut_cfg.addr, dut_cfg.dev_key));

	/* Let the Tester observe proxy advertisements */
	k_sleep(K_MSEC(6500));

	/* Send a mesh message to notify Tester that DUT is about to be suspended. */
	dut_status = DUT_SUSPENDED;
	bt_mesh_test_send_over_adv(&dut_status, sizeof(enum dut_mesh_status));
	k_sleep(K_MSEC(150));

	ASSERT_OK_MSG(bt_mesh_suspend(), "Failed to suspend Mesh.");

	if (disable_bt) {
		ASSERT_OK_MSG(bt_disable(), "Failed to disable Bluetooth.");
	}

	k_sleep(K_SECONDS(SUSPEND_DURATION));

	if (disable_bt) {
		ASSERT_OK_MSG(bt_enable(NULL), "Failed to enable Bluetooth.");
	}

	ASSERT_OK_MSG(bt_mesh_resume(), "Failed to resume Mesh.");

	/* Send a mesh message to notify Tester that device is resumed */
	dut_status = DUT_RUNNING;
	bt_mesh_test_send_over_adv(&dut_status, sizeof(enum dut_mesh_status));

	/* Let the Tester observe that proxy advertisement resumes */
	k_sleep(K_MSEC(6000));
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

static void test_dut_gatt_suspend_resume(void)
{
	dut_gatt_common(false);
	PASS();
}

static void test_dut_gatt_suspend_disable_resume(void)
{
	dut_gatt_common(true);
	PASS();
}

static void test_tester_gatt(void)
{
	k_sem_init(&publish_sem, 0, 1);
	dut_status = DUT_RUNNING;
	int err;

	ASSERT_OK_MSG(bt_enable(NULL), "Bluetooth init failed");

	/* Scan pb gatt beacons. */
	ASSERT_OK(bt_mesh_test_wait_for_packet(gatt_scan_cb, &publish_sem, 10));

	/* Delay to provision DUT */
	k_sleep(K_MSEC(1000));

	/* Scan gatt proxy beacons. */
	/* (total transmit duration) / (transmit interval) */
	gatt_param.transmits = 5000 / 1000;
	gatt_param.interval = 1000;
	gatt_param.service = MESH_SERVICE_PROXY;
	ASSERT_OK(bt_mesh_test_wait_for_packet(gatt_scan_cb, &publish_sem, 10));

	/* Allow DUT to suspend before scanning for gatt proxy beacons */
	ASSERT_OK(bt_mesh_test_wait_for_packet(suspend_state_change_cb, &publish_sem, 20));
	ASSERT_EQUAL(dut_status, DUT_SUSPENDED);
	k_sleep(K_MSEC(500));
	err = bt_mesh_test_wait_for_packet(gatt_scan_cb, &publish_sem, 10);
	ASSERT_FALSE(err && err != -ETIMEDOUT);

	/* Wait for DUT to resume Mesh and notify Tester, then scan for gatt proxy beacons */
	ASSERT_OK(bt_mesh_test_wait_for_packet(suspend_state_change_cb, &publish_sem, 20));
	ASSERT_EQUAL(dut_status, DUT_RUNNING);
	ASSERT_OK(bt_mesh_test_wait_for_packet(gatt_scan_cb, &publish_sem, 10));

	PASS();
}

#define TEST_CASE(role, name, description)                                                         \
	{                                                                                          \
		.test_id = "suspend_" #role "_" #name, .test_descr = description,                  \
		.test_tick_f = bt_mesh_test_timeout, .test_main_f = test_##role##_##name,          \
	}

static const struct bst_test_instance test_suspend[] = {
	TEST_CASE(dut, suspend_resume,
			 "Suspend and resume Mesh with periodic pub"),
	TEST_CASE(dut, suspend_disable_resume,
			 "Suspend and resume Mesh (and disable/enable BT) with periodic pub"),
	TEST_CASE(dut, gatt_suspend_resume,
			 "Suspend and resume Mesh with GATT proxy advs"),
	TEST_CASE(dut, gatt_suspend_disable_resume,
			 "Suspend and resume Mesh (and disable/enable BT) with GATT proxy advs"),

	TEST_CASE(tester, pub, "Scan and verify behavior of periodic publishing adv"),
	TEST_CASE(tester, gatt, "Scan and verify behavior of GATT proxy adv"),

	BSTEST_END_MARKER};

struct bst_test_list *test_suspend_install(struct bst_test_list *tests)
{
	tests = bst_add_tests(tests, test_suspend);
	return tests;
}
