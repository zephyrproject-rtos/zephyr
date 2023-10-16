/*
 * Copyright (c) 2021 Nordic Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include "mesh_test.h"
#include "mesh/net.h"
#include "mesh/access.h"
#include "mesh/foundation.h"

#define LOG_MODULE_NAME test_access

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(LOG_MODULE_NAME, LOG_LEVEL_INF);

#define GROUP_ADDR 0xc000
#define UNICAST_ADDR1 0x0001
#define UNICAST_ADDR2 0x0006
#define WAIT_TIME 10 /*seconds*/

#define TEST_MODEL_ID_1 0x2a2a
#define TEST_MODEL_ID_2 0x2b2b
#define TEST_MODEL_ID_3 0x2c2c
#define TEST_MODEL_ID_4 0x2d2d
#define TEST_MODEL_ID_5 0x2e2e

#define TEST_MESSAGE_OP_1  BT_MESH_MODEL_OP_1(0x11)
#define TEST_MESSAGE_OP_2  BT_MESH_MODEL_OP_1(0x12)
#define TEST_MESSAGE_OP_3  BT_MESH_MODEL_OP_1(0x13)
#define TEST_MESSAGE_OP_4  BT_MESH_MODEL_OP_1(0x14)
#define TEST_MESSAGE_OP_5  BT_MESH_MODEL_OP_1(0x15)
#define TEST_MESSAGE_OP_F  BT_MESH_MODEL_OP_1(0x1F)

#define PUB_PERIOD_COUNT 3
#define RX_JITTER_MAX (10 + CONFIG_BT_MESH_NETWORK_TRANSMIT_COUNT * \
		       (CONFIG_BT_MESH_NETWORK_TRANSMIT_INTERVAL + 10))

static int model1_init(struct bt_mesh_model *model);
static int model2_init(struct bt_mesh_model *model);
static int model3_init(struct bt_mesh_model *model);
static int model4_init(struct bt_mesh_model *model);
static int model5_init(struct bt_mesh_model *model);
static int test_msg_handler(struct bt_mesh_model *model,
			struct bt_mesh_msg_ctx *ctx,
			struct net_buf_simple *buf);
static int test_msg_ne_handler(struct bt_mesh_model *model,
			struct bt_mesh_msg_ctx *ctx,
			struct net_buf_simple *buf);

struct k_poll_signal model_pub_signal;

static uint8_t dev_key[16] = { 0xdd };
static uint8_t app_key[16] = { 0xaa };
static uint8_t net_key[16] = { 0xcc };
static struct bt_mesh_prov prov;

/* Test vector for periodic publication tests. */
static const struct {
	uint8_t period;
	uint8_t div;
	int32_t period_ms;
} test_period[] = {
	{ BT_MESH_PUB_PERIOD_100MS(5), 0, 500 },
	{ BT_MESH_PUB_PERIOD_SEC(2),   0, 2000 },
	{ BT_MESH_PUB_PERIOD_10SEC(1), 0, 10000 },
	{ BT_MESH_PUB_PERIOD_SEC(3),   1, 1500 },
	{ BT_MESH_PUB_PERIOD_10SEC(3), 3, 3750 },
};

/* Test vector for publication retransmissions tests. */
static const uint8_t test_transmit[] = {
	BT_MESH_PUB_TRANSMIT(4, 50),
	BT_MESH_PUB_TRANSMIT(3, 100),
	BT_MESH_PUB_TRANSMIT(2, 200),
};

/* Test vector for canceling a message publication. */
static const struct {
	uint8_t period;
	uint8_t transmit;
	uint8_t msgs;
	int32_t sleep;
	int32_t duration;
} test_cancel[] = {
	/* Test canceling periodic publication. */
	{
		BT_MESH_PUB_PERIOD_SEC(2), 0, 2,
		2000 /* period */ + 100 /* margin */,
		3 /* messages */ * 2000 /* period */
	},
	/* Test canceling publication retransmission. */
	{
		BT_MESH_PUB_PERIOD_SEC(3), BT_MESH_PUB_TRANSMIT(3, 200), 3,
		200 /* retransmission interval */ + 50 /* margin */,
		3000 /* one period */
	},
};

static struct k_sem publish_sem;
static bool publish_allow;

static int model1_update(struct bt_mesh_model *model)
{
	model->pub->msg->data[1]++;

	LOG_DBG("New pub: n: %d t: %d", model->pub->msg->data[1], k_uptime_get_32());

	return publish_allow ? k_sem_give(&publish_sem), 0 : -1;
}

static int test_msgf_handler(struct bt_mesh_model *model,
			     struct bt_mesh_msg_ctx *ctx,
			     struct net_buf_simple *buf)
{
	static uint8_t prev_num;
	uint8_t num = net_buf_simple_pull_u8(buf);

	LOG_DBG("Recv msg: n: %d t: %u", num, k_uptime_get_32());

	/* Ensure that payload changes. */
	ASSERT_TRUE(prev_num != num);
	prev_num = num;

	k_sem_give(&publish_sem);
	return 0;
}

static struct bt_mesh_model_pub model_pub1 = {
	.msg = NET_BUF_SIMPLE(BT_MESH_TX_SDU_MAX),
	.update = model1_update,
};

static const struct bt_mesh_model_cb test_model1_cb = {
	.init = model1_init,
};

static const struct bt_mesh_model_cb test_model2_cb = {
	.init = model2_init,
};

static const struct bt_mesh_model_cb test_model3_cb = {
	.init = model3_init,
};

static const struct bt_mesh_model_cb test_model4_cb = {
	.init = model4_init,
};

static const struct bt_mesh_model_cb test_model5_cb = {
	.init = model5_init,
};

static const struct bt_mesh_model_op model_op1[] = {
	{ TEST_MESSAGE_OP_1, 0, test_msg_handler },
	{ TEST_MESSAGE_OP_F, 0, test_msgf_handler },
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

static const struct bt_mesh_model_op model_op4[] = {
	{ TEST_MESSAGE_OP_4, 0, test_msg_handler },
	BT_MESH_MODEL_OP_END
};

static const struct bt_mesh_model_op model_op5[] = {
	{ TEST_MESSAGE_OP_5, 0, test_msg_handler },
	BT_MESH_MODEL_OP_END
};

static const struct bt_mesh_model_op model_ne_op1[] = {
	{ TEST_MESSAGE_OP_1, 0, test_msg_ne_handler },
	BT_MESH_MODEL_OP_END
};

static const struct bt_mesh_model_op model_ne_op2[] = {
	{ TEST_MESSAGE_OP_2, 0, test_msg_ne_handler },
	BT_MESH_MODEL_OP_END
};

static const struct bt_mesh_model_op model_ne_op3[] = {
	{ TEST_MESSAGE_OP_3, 0, test_msg_ne_handler },
	BT_MESH_MODEL_OP_END
};

static const struct bt_mesh_model_op model_ne_op4[] = {
	{ TEST_MESSAGE_OP_4, 0, test_msg_ne_handler },
	BT_MESH_MODEL_OP_END
};

static const struct bt_mesh_model_op model_ne_op5[] = {
	{ TEST_MESSAGE_OP_5, 0, test_msg_ne_handler },
	BT_MESH_MODEL_OP_END
};

static struct bt_mesh_cfg_cli cfg_cli;

/* do not change model sequence. it will break pointer arithmetic. */
static struct bt_mesh_model models[] = {
	BT_MESH_MODEL_CFG_SRV,
	BT_MESH_MODEL_CFG_CLI(&cfg_cli),
	BT_MESH_MODEL_CB(TEST_MODEL_ID_1, model_op1, &model_pub1, NULL, &test_model1_cb),
	BT_MESH_MODEL_CB(TEST_MODEL_ID_2, model_op2, NULL, NULL, &test_model2_cb),
	BT_MESH_MODEL_CB(TEST_MODEL_ID_3, model_op3, NULL, NULL, &test_model3_cb),
	BT_MESH_MODEL_CB(TEST_MODEL_ID_4, model_op4, NULL, NULL, &test_model4_cb),
	BT_MESH_MODEL_CB(TEST_MODEL_ID_5, model_op5, NULL, NULL, &test_model5_cb),
};

/* do not change model sequence. it will break pointer arithmetic. */
static struct bt_mesh_model models_ne[] = {
	BT_MESH_MODEL_CB(TEST_MODEL_ID_1, model_ne_op1, NULL, NULL, &test_model1_cb),
	BT_MESH_MODEL_CB(TEST_MODEL_ID_2, model_ne_op2, NULL, NULL, &test_model2_cb),
	BT_MESH_MODEL_CB(TEST_MODEL_ID_3, model_ne_op3, NULL, NULL, &test_model3_cb),
	BT_MESH_MODEL_CB(TEST_MODEL_ID_4, model_ne_op4, NULL, NULL, &test_model4_cb),
	BT_MESH_MODEL_CB(TEST_MODEL_ID_5, model_ne_op5, NULL, NULL, &test_model5_cb),
};

static struct bt_mesh_model vnd_models[] = {};

static struct bt_mesh_elem elems[] = {
	BT_MESH_ELEM(0, models, vnd_models),
	BT_MESH_ELEM(1, models_ne, vnd_models),
};

const struct bt_mesh_comp local_comp = {
	.elem = elems,
	.elem_count = ARRAY_SIZE(elems),
};
/*     extension dependency (basic models are on top)
 *
 *        element idx0  element idx1
 *
 *         m1    m2     mne2  mne1
 *        / \    /       |   /  \
 *       /   \  /        |  /    \
 *      m5    m3------->mne3    mne5
 *            |          |
 *            m4        mne4
 */

static int model1_init(struct bt_mesh_model *model)
{
	return 0;
}

static int model2_init(struct bt_mesh_model *model)
{
	return 0;
}

static int model3_init(struct bt_mesh_model *model)
{
	ASSERT_OK(bt_mesh_model_extend(model, model - 2));
	ASSERT_OK(bt_mesh_model_extend(model, model - 1));

	if (model->elem_idx == 1) {
		ASSERT_OK(bt_mesh_model_extend(model, &models[4]));
	}

	return 0;
}

static int model4_init(struct bt_mesh_model *model)
{
	ASSERT_OK(bt_mesh_model_extend(model, model - 1));

	return 0;
}

static int model5_init(struct bt_mesh_model *model)
{
	ASSERT_OK(bt_mesh_model_extend(model, model - 4));

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

static int test_msg_ne_handler(struct bt_mesh_model *model,
			struct bt_mesh_msg_ctx *ctx,
			struct net_buf_simple *buf)
{
	FAIL("Model %#4x on neighbor element received msg", model->id);

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
	uint16_t model_ids[] = {TEST_MODEL_ID_1, TEST_MODEL_ID_2,
			TEST_MODEL_ID_3, TEST_MODEL_ID_4, TEST_MODEL_ID_5};

	err = bt_mesh_cfg_cli_app_key_add(0, addr, 0, 0, app_key, &status);
	if (err || status) {
		FAIL("AppKey add failed (err %d, status %u)", err, status);
		return;
	}

	for (int i = 0; i < ARRAY_SIZE(model_ids); i++) {
		err = bt_mesh_cfg_cli_mod_app_bind(0, addr, addr, 0, model_ids[i], &status);
		if (err || status) {
			FAIL("Model %#4x bind failed (err %d, status %u)",
					model_ids[i], err, status);
			return;
		}

		err = bt_mesh_cfg_cli_mod_app_bind(0, addr, addr + 1, 0, model_ids[i], &status);
		if (err || status) {
			FAIL("Model %#4x bind failed (err %d, status %u)",
					model_ids[i], err, status);
			return;
		}
	}

	err = bt_mesh_cfg_cli_net_transmit_set(0, addr, BT_MESH_TRANSMIT(2, 20), &status);
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

	err = bt_mesh_cfg_cli_mod_sub_add(0, addr, addr, GROUP_ADDR, TEST_MODEL_ID_2, &status);

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

	bt_mesh_model_msg_init(&msg, TEST_MESSAGE_OP_4);
	bt_mesh_model_send(&models[5], &ctx, &msg, NULL, NULL);

	bt_mesh_model_msg_init(&msg, TEST_MESSAGE_OP_5);
	bt_mesh_model_send(&models[6], &ctx, &msg, NULL, NULL);

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
	bool m4_fired = false;
	bool m5_fired = false;

	while (!m1_fired || !m2_fired || !m3_fired || !m4_fired || !m5_fired) {
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
		case TEST_MODEL_ID_4:
			ASSERT_FALSE(m4_fired);
			m4_fired = true;
			break;
		case TEST_MODEL_ID_5:
			ASSERT_FALSE(m5_fired);
			m5_fired = true;
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

static void test_sub_capacity_ext_model(void)
{
	bt_mesh_test_cfg_set(NULL, WAIT_TIME);
	bt_mesh_device_setup(&prov, &local_comp);
	provision(UNICAST_ADDR2);
	common_configure(UNICAST_ADDR2);

	uint8_t status;
	int i;

	/* Models in the extension linked list use the subscription list capacity of
	 * each other to the full extent. If a model cannot put a subscription address in
	 * its own subscription list it looks for the closest empty cell in model
	 * in the extension linked list.
	 */
	for (i = 0; i < 5 * CONFIG_BT_MESH_MODEL_GROUP_COUNT; i++) {
		ASSERT_OK_MSG(bt_mesh_cfg_cli_mod_sub_add(0, UNICAST_ADDR2, UNICAST_ADDR2,
							  GROUP_ADDR + i, TEST_MODEL_ID_2, &status),
			      "Can't deliver subscription on address %#4x", GROUP_ADDR + i);

		ASSERT_EQUAL(STATUS_SUCCESS, status);
	}

	uint16_t model_ids[] = {TEST_MODEL_ID_1, TEST_MODEL_ID_2,
			TEST_MODEL_ID_3, TEST_MODEL_ID_4, TEST_MODEL_ID_5};

	for (int j = 0; j < ARRAY_SIZE(model_ids); j++) {
		ASSERT_OK_MSG(bt_mesh_cfg_cli_mod_sub_add(0, UNICAST_ADDR2, UNICAST_ADDR2,
							  GROUP_ADDR + i, model_ids[j], &status),
			      "Can't deliver subscription on address %#4x", GROUP_ADDR + i);

		ASSERT_EQUAL(STATUS_INSUFF_RESOURCES, status);
	}

	PASS();
}

static void pub_param_set(uint8_t period, uint8_t transmit)
{
	struct bt_mesh_cfg_cli_mod_pub pub_params = {
		.addr = UNICAST_ADDR2,
		.uuid = NULL,
		.cred_flag = false,
		.app_idx = 0,
		.ttl = 5,
		.period = period,
		.transmit = transmit,
	};
	uint8_t status;
	int err;

	err = bt_mesh_cfg_cli_mod_pub_set(0, UNICAST_ADDR1, UNICAST_ADDR1, TEST_MODEL_ID_1,
					  &pub_params, &status);
	if (err || status) {
		FAIL("Mod pub set failed (err %d, status %u)", err, status);
	}
}

static void msgf_publish(void)
{
	struct bt_mesh_model *model = &models[2];

	bt_mesh_model_msg_init(model->pub->msg, TEST_MESSAGE_OP_F);
	net_buf_simple_add_u8(model->pub->msg, 1);
	bt_mesh_model_publish(model);
}

static void pub_jitter_check(int32_t interval, uint8_t count)
{
	int64_t timestamp = k_uptime_get();
	int32_t jitter = 0;
	int err;

	for (size_t j = 0; j < count; j++) {
		/* Every new publication will release semaphore in the update handler and the time
		 * between two consecutive publications will be measured.
		 */
		err = k_sem_take(&publish_sem, K_SECONDS(20));
		if (err) {
			FAIL("Send timed out");
		}

		int32_t time_delta = k_uptime_delta(&timestamp);
		int32_t pub_delta = llabs(time_delta - interval);

		jitter = MAX(pub_delta, jitter);

		LOG_DBG("Send time: %d delta: %d jitter: %d", (int32_t)timestamp, time_delta,
			jitter);
	}

	LOG_INF("Send jitter: %d", jitter);
	ASSERT_TRUE(jitter <= 10);
}

static void recv_jitter_check(int32_t interval, uint8_t count)
{
	int64_t timestamp;
	uint32_t jitter = 0;
	int err;

	/* The measurement starts by the first received message. */
	err = k_sem_take(&publish_sem, K_SECONDS(20));
	if (err) {
		FAIL("Recv timed out");
	}

	timestamp = k_uptime_get();

	for (size_t j = 0; j < count; j++) {
		/* Every new received message will release semaphore in the message handler and
		 * the time between two consecutive publications will be measured.
		 */
		err = k_sem_take(&publish_sem, K_SECONDS(20));
		if (err) {
			FAIL("Recv timed out");
		}

		int32_t time_delta = k_uptime_delta(&timestamp);
		int32_t pub_delta = llabs(time_delta - interval);

		jitter = MAX(pub_delta, jitter);

		LOG_DBG("Recv time: %d delta: %d jitter: %d", (int32_t)timestamp, time_delta,
			jitter);
	}

	LOG_INF("Recv jitter: %d", jitter);
	ASSERT_TRUE(jitter <= RX_JITTER_MAX);
}

/* Test publish period states by publishing a message and checking interval between update handler
 * calls.
 */
static void test_tx_period(void)
{
	struct bt_mesh_model *model = &models[2];

	bt_mesh_test_cfg_set(NULL, 60);
	bt_mesh_device_setup(&prov, &local_comp);
	provision(UNICAST_ADDR1);
	common_configure(UNICAST_ADDR1);

	k_sem_init(&publish_sem, 0, 1);

	for (size_t i = 0; i < ARRAY_SIZE(test_period); i++) {
		pub_param_set(test_period[i].period, 0);

		model->pub->fast_period = test_period[i].div > 0;
		model->pub->period_div = test_period[i].div;

		LOG_INF("Publication period: %d", test_period[i].period_ms);

		/* Start publishing messages and measure jitter. */
		msgf_publish();
		publish_allow = true;
		pub_jitter_check(test_period[i].period_ms, PUB_PERIOD_COUNT);

		/* Disable periodic publication before the next test iteration. */
		publish_allow = false;

		/* Let the receiver hit the first semaphore. */
		k_sleep(K_SECONDS(1));
	}

	PASS();
}

/* Receive a periodically published message and check publication period by measuring interval
 * between message handler calls.
 */
static void test_rx_period(void)
{
	bt_mesh_test_cfg_set(NULL, 60);
	bt_mesh_device_setup(&prov, &local_comp);
	provision(UNICAST_ADDR2);
	common_configure(UNICAST_ADDR2);

	k_sem_init(&publish_sem, 0, 1);

	for (size_t i = 0; i < ARRAY_SIZE(test_period); i++) {
		recv_jitter_check(test_period[i].period_ms, PUB_PERIOD_COUNT);
	}

	PASS();
}

/* Test publish retransmit interval and count states by publishing a message and checking interval
 * between update handler calls.
 */
static void test_tx_transmit(void)
{
	struct bt_mesh_model *model = &models[2];
	uint8_t status;
	int err;

	bt_mesh_test_cfg_set(NULL, 60);
	bt_mesh_device_setup(&prov, &local_comp);
	provision(UNICAST_ADDR1);
	common_configure(UNICAST_ADDR1);

	k_sem_init(&publish_sem, 0, 1);

	/* Network retransmissions has to be disabled so that the legacy advertiser sleeps for the
	 * least possible time, which is 50ms. This will let the access layer publish a message
	 * with 50ms retransmission interval.
	 */
	err = bt_mesh_cfg_cli_net_transmit_set(0, UNICAST_ADDR1,
				BT_MESH_TRANSMIT(0, CONFIG_BT_MESH_NETWORK_TRANSMIT_INTERVAL),
				&status);
	if (err || status != BT_MESH_TRANSMIT(0, CONFIG_BT_MESH_NETWORK_TRANSMIT_INTERVAL)) {
		FAIL("Net transmit set failed (err %d, status %u)", err,
		     status);
	}

	publish_allow = true;
	model->pub->retr_update = true;

	for (size_t i = 0; i < ARRAY_SIZE(test_transmit); i++) {
		pub_param_set(0, test_transmit[i]);

		int32_t interval = BT_MESH_PUB_TRANSMIT_INT(test_transmit[i]);
		int count = BT_MESH_PUB_TRANSMIT_COUNT(test_transmit[i]);

		LOG_INF("Retransmission interval: %d, count: %d", interval, count);

		/* Start publishing messages and measure jitter. */
		msgf_publish();
		pub_jitter_check(interval, count);

		/* Let the receiver hit the first semaphore. */
		k_sleep(K_SECONDS(1));
	}

	PASS();
}

/* Receive a published message and check retransmission interval by measuring interval between
 * message handler calls.
 */
static void test_rx_transmit(void)
{
	bt_mesh_test_cfg_set(NULL, 60);
	bt_mesh_device_setup(&prov, &local_comp);
	provision(UNICAST_ADDR2);
	common_configure(UNICAST_ADDR2);

	k_sem_init(&publish_sem, 0, 1);

	for (size_t i = 0; i < ARRAY_SIZE(test_transmit); i++) {
		int32_t interval = BT_MESH_PUB_TRANSMIT_INT(test_transmit[i]);
		int count = BT_MESH_PUB_TRANSMIT_COUNT(test_transmit[i]);

		recv_jitter_check(interval, count);
	}

	PASS();
}

/* Cancel one of messages to be published and check that the next one is published when next period
 * starts.
 */
static void test_tx_cancel(void)
{
	struct bt_mesh_model *model = &models[2];
	int err;

	bt_mesh_test_cfg_set(NULL, 20);
	bt_mesh_device_setup(&prov, &local_comp);
	provision(UNICAST_ADDR1);
	common_configure(UNICAST_ADDR1);

	k_sem_init(&publish_sem, 0, 1);

	model->pub->retr_update = true;

	for (size_t i = 0; i < ARRAY_SIZE(test_cancel); i++) {
		pub_param_set(test_cancel[i].period, test_cancel[i].transmit);

		msgf_publish();
		publish_allow = true;
		int64_t timestamp = k_uptime_get();

		/* Send few messages except one that is to be cancelled. */
		for (size_t j = 0; j < test_cancel[i].msgs - 1; j++) {
			err = k_sem_take(&publish_sem, K_SECONDS(20));
			if (err) {
				FAIL("Send timed out");
			}
		}

		/* Cancel the next publication. */
		publish_allow = false;
		k_sleep(K_MSEC(test_cancel[i].sleep));

		/* Reenable publication a wait for a next message to be published. */
		publish_allow = true;
		err = k_sem_take(&publish_sem, K_SECONDS(20));
		if (err) {
			FAIL("Send timed out");
		}

		/* Disable periodic publication before the next test iteration. */
		publish_allow = false;

		/* If the canceled message is also sent, the semaphore will be released earlier than
		 * expected.
		 */
		int32_t time_delta = k_uptime_delta(&timestamp);
		int32_t jitter = llabs(time_delta - test_cancel[i].duration);

		LOG_DBG("Send time: %d delta: %d", (int32_t)timestamp, time_delta);
		LOG_INF("Send jitter: %d", jitter);
		ASSERT_TRUE(jitter <= 10);

		/* Let the receiver hit the first semaphore. */
		k_sleep(K_SECONDS(1));
	}

	PASS();
}

/* Receive all published messages and ensure that cancelled message is not received. */
static void test_rx_cancel(void)
{
	bt_mesh_test_cfg_set(NULL, 20);
	bt_mesh_device_setup(&prov, &local_comp);
	provision(UNICAST_ADDR2);
	common_configure(UNICAST_ADDR2);

	k_sem_init(&publish_sem, 0, 1);

	for (size_t i = 0; i < ARRAY_SIZE(test_cancel); i++) {
		int64_t timestamp;
		int err;

		/* Wait for the first published message. */
		err = k_sem_take(&publish_sem, K_SECONDS(20));
		if (err) {
			FAIL("Recv timed out");
		}

		timestamp = k_uptime_get();

		/* Wait for the rest messages to be published (incl. the next after cancelled one).
		 */
		for (size_t j = 0; j < test_cancel[i].msgs; j++) {
			err = k_sem_take(&publish_sem, K_SECONDS(20));
			if (err) {
				FAIL("Recv timed out");
			}
		}

		/* If the canceled message is received, the semaphore will be released earlier than
		 * expected.
		 */
		int32_t time_delta = k_uptime_delta(&timestamp);
		int32_t jitter = llabs(time_delta - test_cancel[i].duration);

		LOG_DBG("Recv time: %d delta: %d", (int32_t)timestamp, time_delta);
		LOG_INF("Recv jitter: %d", jitter);
		ASSERT_TRUE(jitter <= RX_JITTER_MAX);
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
	TEST_CASE(sub_capacity, ext_model, "Access: subscription capacity of extended models"),
	TEST_CASE(tx, period, "Access: Publish a message periodically"),
	TEST_CASE(rx, period, "Access: Receive periodically published message"),
	TEST_CASE(tx, transmit, "Access: Publish and retransmit message"),
	TEST_CASE(rx, transmit, "Access: Receive retransmitted messages"),
	TEST_CASE(tx, cancel, "Access: Cancel a message during publication"),
	TEST_CASE(rx, cancel, "Access: Receive published messages except cancelled"),

	BSTEST_END_MARKER
};

struct bst_test_list *test_access_install(struct bst_test_list *tests)
{
	tests = bst_add_tests(tests, test_access);
	return tests;
}
