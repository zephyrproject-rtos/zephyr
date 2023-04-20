/*
 * Copyright (c) 2021 Nordic Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include "mesh_test.h"
#include "argparse.h"
#include "mesh/net.h"
#include "mesh/heartbeat.h"
#include "mesh/lpn.h"

#define LOG_MODULE_NAME test_heartbeat

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(LOG_MODULE_NAME);

#define WAIT_TIME 60 /*seconds*/
#define SUBSCRIBER_ADDR 0x00fe
#define SUBSCRIBE_PERIOD_SEC 30
#define PUBLISHER_ADDR_START 0x0001
#define PUBLISH_PERIOD_SEC 1
#define PUBLISH_MSG_CNT 10
#define PUBLISH_TTL 0
#define EXPECTED_HB_HOPS 0x01

static uint16_t pub_addr = BT_MESH_ADDR_UNASSIGNED;

static const struct bt_mesh_test_cfg subscribe_cfg = {
	.addr = SUBSCRIBER_ADDR,
	.dev_key = { 0xff },
};
static struct bt_mesh_test_cfg pub_cfg;
static int pub_cnt;
struct k_sem sem;

static void test_publish_init(void)
{
	pub_cfg.addr = PUBLISHER_ADDR_START + get_device_nbr();
	pub_cfg.dev_key[0] = get_device_nbr();
	bt_mesh_test_cfg_set(&pub_cfg, WAIT_TIME);
}

static void test_subscribe_init(void)
{
	bt_mesh_test_cfg_set(&subscribe_cfg, WAIT_TIME);
}

static struct sub_context {
	uint8_t count;
	uint8_t min_hops;
	uint8_t max_hops;
} sub_ctx = {
	.count = 0,
	.min_hops = 0xFF,
	.max_hops = 0,
};

static void sub_hb_recv_cb(const struct bt_mesh_hb_sub *sub, uint8_t hops, uint16_t feat)
{
	LOG_INF("Heartbeat received from addr: 0x%04x", sub->src);

	ASSERT_EQUAL(PUBLISHER_ADDR_START, sub->src);
	ASSERT_EQUAL(pub_addr, sub->dst);
	ASSERT_EQUAL(SUBSCRIBE_PERIOD_SEC, sub->period);
	ASSERT_TRUE(sub->remaining <= SUBSCRIBE_PERIOD_SEC);
	ASSERT_EQUAL(sub_ctx.count + 1, sub->count);
	ASSERT_EQUAL(hops, EXPECTED_HB_HOPS);

	uint16_t feature = 0;

	if (bt_mesh_relay_get() == BT_MESH_RELAY_ENABLED) {
		feature |= BT_MESH_FEAT_RELAY;
	}

	if (bt_mesh_gatt_proxy_get() == BT_MESH_GATT_PROXY_ENABLED) {
		feature |= BT_MESH_FEAT_PROXY;
	}

	if (bt_mesh_friend_get() == BT_MESH_FRIEND_ENABLED) {
		feature |= BT_MESH_FEAT_FRIEND;
	}

	if (bt_mesh_lpn_established()) {
		feature |= BT_MESH_FEAT_LOW_POWER;
	}

	ASSERT_EQUAL(feature, feat);

	sub_ctx.count++;
	sub_ctx.min_hops = MIN(sub_ctx.min_hops, sub->min_hops);
	sub_ctx.max_hops = MAX(sub_ctx.max_hops, sub->max_hops);
}

static void sub_hb_end_cb(const struct bt_mesh_hb_sub *sub)
{
	LOG_INF("Heartbeat subscription has ended");
	ASSERT_EQUAL(PUBLISHER_ADDR_START, sub->src);
	ASSERT_EQUAL(pub_addr, sub->dst);
	ASSERT_EQUAL(SUBSCRIBE_PERIOD_SEC, sub->period);
	ASSERT_EQUAL(0, sub->remaining);
	ASSERT_EQUAL(PUBLISH_MSG_CNT, sub->count);
	ASSERT_EQUAL(sub_ctx.count, sub->count);
	ASSERT_EQUAL(sub_ctx.min_hops, sub->min_hops);
	ASSERT_EQUAL(sub_ctx.max_hops, sub->max_hops);
	PASS();
}

static void pub_hb_sent_cb(const struct bt_mesh_hb_pub *pub)
{
	LOG_INF("Heartbeat publication has ended");

	pub_cnt--;

	ASSERT_EQUAL(pub_addr, pub->dst);
	ASSERT_EQUAL(pub_cnt, pub->count);
	ASSERT_EQUAL(PUBLISH_PERIOD_SEC, pub->period);
	ASSERT_EQUAL(0, pub->net_idx);
	ASSERT_EQUAL(PUBLISH_TTL, pub->ttl);
	ASSERT_EQUAL(BT_MESH_FEAT_SUPPORTED, pub->feat);

	if (pub_cnt == 0) {
		k_sem_give(&sem);
	}

	if (pub_cnt < 0) {
		LOG_ERR("Published more times than expected");
		FAIL();
	}
}

BT_MESH_HB_CB_DEFINE(hb_cb) = {
	.recv = sub_hb_recv_cb,
	.sub_end = sub_hb_end_cb,
	.pub_sent = pub_hb_sent_cb
};

static void publish_common(void)
{
	bt_mesh_test_setup();
	struct bt_mesh_hb_pub new_pub = {
		.dst = pub_addr,
		.count = PUBLISH_MSG_CNT,
		.period = PUBLISH_PERIOD_SEC,
		.net_idx = 0,
		.ttl = PUBLISH_TTL,
		.feat = BT_MESH_FEAT_SUPPORTED
	};

	pub_cnt = PUBLISH_MSG_CNT;
	bt_mesh_hb_pub_set(&new_pub);
}

static void publish_process(void)
{
	k_sem_init(&sem, 0, 1);
	publish_common();
	/* +1 to avoid boundary time rally */
	if (k_sem_take(&sem, K_SECONDS(PUBLISH_PERIOD_SEC * (PUBLISH_MSG_CNT + 1)))) {
		LOG_ERR("Publishing timed out");
		FAIL();
	}
}

static void test_publish_unicast(void)
{
	pub_addr = SUBSCRIBER_ADDR;
	publish_process();

	PASS();
}

static void test_publish_all(void)
{
	pub_addr = BT_MESH_ADDR_ALL_NODES;
	publish_process();

	PASS();
}

static void subscribe_commmon(void)
{
	bt_mesh_test_setup();
	bt_mesh_hb_sub_set(PUBLISHER_ADDR_START, pub_addr, SUBSCRIBE_PERIOD_SEC);
}

static void test_subscribe_unicast(void)
{
	pub_addr = SUBSCRIBER_ADDR;
	subscribe_commmon();
}

static void test_subscribe_all(void)
{
	pub_addr = BT_MESH_ADDR_ALL_NODES;
	subscribe_commmon();
}

#define TEST_CASE(role, name, description)                                     \
	{                                                                      \
		.test_id = "heartbeat_" #role "_" #name,                       \
		.test_descr = description,                                     \
		.test_post_init_f = test_##role##_init,                        \
		.test_tick_f = bt_mesh_test_timeout,                           \
		.test_main_f = test_##role##_##name,                           \
	}

static const struct bst_test_instance test_connect[] = {
	TEST_CASE(publish, unicast,     "Heartbeat: Publish heartbeat to unicast"),
	TEST_CASE(subscribe, unicast,   "Heartbeat: Subscribe to heartbeat from unicast"),
	TEST_CASE(publish, all,         "Heartbeat: Publish heartbeat to all nodes"),
	TEST_CASE(subscribe, all,       "Heartbeat: Subscribe to heartbeat all nodes"),
	BSTEST_END_MARKER
};

struct bst_test_list *test_heartbeat_install(struct bst_test_list *tests)
{
	tests = bst_add_tests(tests, test_connect);
	return tests;
}
