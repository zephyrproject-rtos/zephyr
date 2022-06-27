/*
 * Copyright (c) 2021 Nordic Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include "mesh_test.h"
#include "mesh/net.h"
#include "mesh/transport.h"
#include <zephyr/sys/byteorder.h>

/*
 * Transport layer tests:
 *   This file contains tests for sending and receiving messages end-to-end in
 *   all permutations. Covers:
 *   - Address resolution
 *   - Segmented messages
 *     - Single segment
 *     - Max length
 *     - Groups
 *   - Virtual addresses
 *   - Loopback
 *
 *   Tests are divided into senders and receivers.
 */

void assert_post_action(const char *file, unsigned int line)
{
	FAIL("Asserted at %s:%u", file, line);
}

#define GROUP_ADDR 0xc000
#define WAIT_TIME 60 /*seconds*/

extern enum bst_result_t bst_result;

static const struct bt_mesh_test_cfg tx_cfg = {
	.addr = 0x0001,
	.dev_key = { 0x01 },
};
static const struct bt_mesh_test_cfg rx_cfg = {
	.addr = 0x0002,
	.dev_key = { 0x02 },
};

static int expected_send_err;

static void test_tx_init(void)
{
	bt_mesh_test_cfg_set(&tx_cfg, WAIT_TIME);
}

static void test_rx_init(void)
{
	bt_mesh_test_cfg_set(&rx_cfg, WAIT_TIME);
}

static void async_send_end(int err, void *data)
{
	struct k_sem *sem = data;

	if (err != expected_send_err) {
		FAIL("Async send failed: got %d, expected", err, expected_send_err);
	}

	if (sem) {
		k_sem_give(sem);
	}
}

static const struct bt_mesh_send_cb async_send_cb = {
	.end = async_send_end,
};

/** Test vector containing various permutations of transport messages. */
static const struct {
	uint16_t len;
	enum bt_mesh_test_send_flags flags;
} test_vector[] = {
	{.len = 1, .flags = 0},
	{.len = 1, .flags = FORCE_SEGMENTATION},
	{.len = BT_MESH_APP_SEG_SDU_MAX, .flags = 0},
	{.len = BT_MESH_APP_SEG_SDU_MAX, .flags = FORCE_SEGMENTATION},

	/* segmented */
	{.len = BT_MESH_APP_SEG_SDU_MAX + 1, .flags = 0},
	{.len = 256, .flags = LONG_MIC},
	{.len = BT_MESH_TX_SDU_MAX - BT_MESH_MIC_SHORT, .flags = 0},
};

/** Test sending of unicast messages using the test vector.
 */
static void test_tx_unicast(void)
{
	int err;

	bt_mesh_test_setup();

	for (int i = 0; i < ARRAY_SIZE(test_vector); i++) {
		err = bt_mesh_test_send(rx_cfg.addr, test_vector[i].len,
					test_vector[i].flags, K_SECONDS(10));
		ASSERT_OK(err, "Failed sending vector %d", i);
	}

	PASS();
}

/** Test sending of group messages using the test vector.
 */
static void test_tx_group(void)
{
	int err;

	bt_mesh_test_setup();

	for (int i = 0; i < ARRAY_SIZE(test_vector); i++) {
		err = bt_mesh_test_send(GROUP_ADDR, test_vector[i].len,
					test_vector[i].flags, K_SECONDS(20));
		ASSERT_OK(err, "Failed sending vector %d", i);
	}

	PASS();
}

/** Test sending of virtual address messages using the test vector.
 */
static void test_tx_va(void)
{
	uint16_t virtual_addr;
	int err;

	bt_mesh_test_setup();

	err = bt_mesh_va_add(test_va_uuid, &virtual_addr);
	ASSERT_OK(err, "Virtual addr add failed (err %d)", err);

	for (int i = 0; i < ARRAY_SIZE(test_vector); i++) {
		err = bt_mesh_test_send(virtual_addr, test_vector[i].len,
					test_vector[i].flags, K_SECONDS(20));
		ASSERT_OK(err, "Failed sending vector %d", i);
	}

	PASS();
}

/** Test sending of messages to own unicast address using the test vector.
 */
static void test_tx_loopback(void)
{
	int err;

	bt_mesh_test_setup();

	for (int i = 0; i < ARRAY_SIZE(test_vector); i++) {
		bt_mesh_test_recv(test_vector[i].len, cfg->addr, K_NO_WAIT);
		err = bt_mesh_test_send(cfg->addr, test_vector[i].len,
					test_vector[i].flags,
					K_SECONDS(20));
		ASSERT_OK(err, "Failed sending vector %d", i);

		if (test_stats.received != i + 1) {
			FAIL("Didn't receive message %d", i);
		}
	}

	PASS();
}

/** Test sending of messages with an app key that's unknown to the receiver.
 *
 *  The sender should be able to send the message successfully, but the receiver
 *  should fail the decryption step and ignore the packet.
 */
static void test_tx_unknown_app(void)
{
	uint8_t app_key[16] = { 0xba, 0xd0, 0x11, 0x22};
	uint8_t status = 0;

	bt_mesh_test_setup();

	ASSERT_OK(bt_mesh_cfg_app_key_add(0, cfg->addr, 0, 1, app_key, &status),
		  "Failed adding additional appkey");
	if (status) {
		FAIL("App key add status: 0x%02x", status);
	}

	ASSERT_OK(bt_mesh_cfg_mod_app_bind(0, cfg->addr, cfg->addr, 1,
					   TEST_MOD_ID, &status),
		  "Failed binding additional appkey");
	if (status) {
		FAIL("App key add status: 0x%02x", status);
	}

	test_send_ctx.app_idx = 1;

	ASSERT_OK(bt_mesh_test_send(rx_cfg.addr, 5, 0, K_SECONDS(1)),
		  "Failed sending unsegmented");

	ASSERT_OK(bt_mesh_test_send(rx_cfg.addr, 25, 0, K_SECONDS(1)),
		  "Failed sending segmented");

	PASS();
}

/** Test sending of messages using the test vector.
 *
 *  Messages are sent to a group address that both the sender and receiver
 *  subscribes to, verifying that the loopback and advertiser paths both work
 *  when used in combination.
 */
static void test_tx_loopback_group(void)
{
	uint8_t status;
	int err;

	bt_mesh_test_setup();

	err = bt_mesh_cfg_mod_sub_add(0, cfg->addr, cfg->addr, GROUP_ADDR,
				      TEST_MOD_ID, &status);
	ASSERT_OK(err || status, "Mod sub add failed (err %d, status %u)",
		  err, status);

	for (int i = 0; i < ARRAY_SIZE(test_vector); i++) {
		err = bt_mesh_test_send(GROUP_ADDR, test_vector[i].len,
					test_vector[i].flags,
					K_SECONDS(20));

		ASSERT_OK(err, "Failed sending vector %d", i);

		k_sleep(K_SECONDS(1));
		ASSERT_OK(bt_mesh_test_recv(test_vector[i].len, GROUP_ADDR,
					    K_SECONDS(1)),
			  "Failed receiving loopback %d", i);

		if (test_stats.received != i + 1) {
			FAIL("Didn't receive message %d", i);
		}
	}

	PASS();
}

/** Start sending multiple segmented messages to the same destination at the
 *  same time.
 *
 *  The second message should be blocked until the first is finished, but
 *  should still succeed.
 */
static void test_tx_seg_block(void)
{
	bt_mesh_test_setup();


	ASSERT_OK(bt_mesh_test_send(rx_cfg.addr, 20, 0, K_NO_WAIT));

	/* Send some more to the same address before the first is finished. */
	ASSERT_OK(bt_mesh_test_send(rx_cfg.addr, 20, 0, K_NO_WAIT));
	ASSERT_OK(bt_mesh_test_send(rx_cfg.addr, 20, 0, K_SECONDS(10)));

	if (test_stats.sent != 3) {
		FAIL("Not all messages completed (%u/3)", test_stats.sent);
	}

	PASS();
}

/** Start sending multiple segmented messages to the same destination at the
 *  same time.
 *
 *  The second message should be blocked until the first is finished, but
 *  should still succeed.
 */
static void test_tx_seg_concurrent(void)
{
	struct k_sem sem;

	k_sem_init(&sem, 0, 1);
	bt_mesh_test_setup();

	ASSERT_OK(bt_mesh_test_send_async(rx_cfg.addr, 20, 0, &async_send_cb, &sem));

	/* Send some more to the same address before the first is finished. */
	ASSERT_OK(bt_mesh_test_send(GROUP_ADDR, 20, 0, K_SECONDS(10)));

	/* Ensure that the first message finishes as well */
	ASSERT_OK(k_sem_take(&sem, K_SECONDS(1)));

	PASS();
}

/** Start sending a segmented message, then before it's finished, start an IV
 *  update.
 *  After the first one finishes, the IV update state shall be active.
 *  Send another message, then end the IV update state before it's finished.
 *  The IV index should change when this message finishes.
 *
 *  The IV update should not interfere with the segmented message, and the
 */
static void test_tx_seg_ivu(void)
{
	struct k_sem sem;
	uint32_t iv_index;

	k_sem_init(&sem, 0, 1);

	bt_mesh_test_setup();

	/* Enable IV update test mode to override IV update timers */
	bt_mesh_iv_update_test(true);

	iv_index = BT_MESH_NET_IVI_TX;

	ASSERT_OK(bt_mesh_test_send_async(rx_cfg.addr, 255, 0, &async_send_cb,
					  &sem));
	/* Start IV update */
	bt_mesh_iv_update();

	if (iv_index != BT_MESH_NET_IVI_TX) {
		FAIL("Should not change TX IV index before IV update ends");
	}

	k_sem_take(&sem, K_SECONDS(20));

	ASSERT_OK(bt_mesh_test_send_async(rx_cfg.addr, 255, 0, &async_send_cb,
					  &sem));

	/* End IV update */
	bt_mesh_iv_update();

	if (iv_index != BT_MESH_NET_IVI_TX) {
		FAIL("Should not change TX IV index until message finishes");
	}

	k_sem_take(&sem, K_SECONDS(20));

	if (iv_index + 1 != BT_MESH_NET_IVI_TX) {
		FAIL("Should have changed TX IV index when the message was completed");
	}

	PASS();
}

/** Send a segmented message to an unknown unicast address, expect it to fail
 *  and return -ETIMEDOUT in the send end callback.
 */
static void test_tx_seg_fail(void)
{
	struct k_sem sem;

	k_sem_init(&sem, 0, 1);
	bt_mesh_test_setup();

	expected_send_err = -ETIMEDOUT;
	ASSERT_OK(bt_mesh_test_send_async(0x0fff, 20, 0, &async_send_cb, &sem));
	ASSERT_OK(k_sem_take(&sem, K_SECONDS(10)));

	PASS();
}

/* Receiver test functions */

/** @brief Receive unicast messages using the test vector.
 */
static void test_rx_unicast(void)
{
	int err;

	bt_mesh_test_setup();

	for (int i = 0; i < ARRAY_SIZE(test_vector); i++) {
		err = bt_mesh_test_recv(test_vector[i].len, cfg->addr,
					K_SECONDS(10));
		ASSERT_OK(err, "Failed receiving vector %d", i);
	}

	PASS();
}

/** @brief Receive group messages using the test vector.
 */
static void test_rx_group(void)
{
	uint8_t status;
	int err;

	bt_mesh_test_setup();

	err = bt_mesh_cfg_mod_sub_add(0, cfg->addr, cfg->addr, GROUP_ADDR,
				      TEST_MOD_ID, &status);
	ASSERT_OK(err || status, "Mod sub add failed (err %d, status %u)",
		  err, status);

	for (int i = 0; i < ARRAY_SIZE(test_vector); i++) {
		err = bt_mesh_test_recv(test_vector[i].len, GROUP_ADDR,
					K_SECONDS(20));
		ASSERT_OK(err, "Failed receiving vector %d", i);
	}

	PASS();
}

/** @brief Receive virtual address messages using the test vector.
 */
static void test_rx_va(void)
{
	uint16_t virtual_addr;
	uint8_t status;
	int err;

	bt_mesh_test_setup();

	err = bt_mesh_cfg_mod_sub_va_add(0, cfg->addr, cfg->addr, test_va_uuid,
					 TEST_MOD_ID, &virtual_addr, &status);
	ASSERT_OK(err || status, "Sub add failed (err %d, status %u)", err,
		  status);

	for (int i = 0; i < ARRAY_SIZE(test_vector); i++) {
		err = bt_mesh_test_recv(test_vector[i].len, virtual_addr,
					K_SECONDS(20));
		ASSERT_OK(err, "Failed receiving vector %d", i);
	}

	PASS();
}

/** @brief Verify that this device doesn't receive any messages.
 */
static void test_rx_none(void)
{
	struct bt_mesh_test_msg msg;
	int err;

	bt_mesh_test_setup();

	err = bt_mesh_test_recv_msg(&msg, K_SECONDS(60));
	if (!err) {
		FAIL("Unexpected rx from 0x%04x", msg.ctx.addr);
	}

	PASS();
}

/** @brief Verify that this device doesn't receive any messages.
 */
static void test_rx_seg_block(void)
{
	bt_mesh_test_setup();

	ASSERT_OK(bt_mesh_test_recv(20, cfg->addr, K_SECONDS(2)), "RX fail");
	ASSERT_OK(bt_mesh_test_recv(20, cfg->addr, K_SECONDS(2)), "RX fail");
	ASSERT_OK(bt_mesh_test_recv(20, cfg->addr, K_SECONDS(2)), "RX fail");

	PASS();
}

/** @brief Verify that this device doesn't receive any messages.
 */
static void test_rx_seg_concurrent(void)
{
	uint8_t status;
	int err;

	bt_mesh_test_setup();

	/* Subscribe to group addr */
	err = bt_mesh_cfg_mod_sub_add(0, cfg->addr, cfg->addr, GROUP_ADDR,
				      TEST_MOD_ID, &status);
	ASSERT_OK(err || status, "Mod sub add failed (err %d, status %u)",
		  err, status);

	/* Receive both messages from the sender.
	 * Note: The receive order is technically irrelevant, but the test_recv
	 * function fails if the order is wrong.
	 */
	ASSERT_OK(bt_mesh_test_recv(20, cfg->addr, K_SECONDS(2)), "RX fail");
	ASSERT_OK(bt_mesh_test_recv(20, GROUP_ADDR, K_SECONDS(2)), "RX fail");

	PASS();
}

/** @brief Verify that this device doesn't receive any messages.
 */
static void test_rx_seg_ivu(void)
{
	bt_mesh_test_setup();

	ASSERT_OK(bt_mesh_test_recv(255, cfg->addr, K_SECONDS(5)), "RX fail");
	ASSERT_OK(bt_mesh_test_recv(255, cfg->addr, K_SECONDS(5)), "RX fail");

	PASS();
}

#define TEST_CASE(role, name, description)                                     \
	{                                                                      \
		.test_id = "transport_" #role "_" #name,                       \
		.test_descr = description,                                     \
		.test_post_init_f = test_##role##_init,                        \
		.test_tick_f = bt_mesh_test_timeout,                           \
		.test_main_f = test_##role##_##name,                           \
	}

static const struct bst_test_instance test_connect[] = {
	TEST_CASE(tx, unicast,        "Transport: send to unicast addr"),
	TEST_CASE(tx, group,          "Transport: send to group addr"),
	TEST_CASE(tx, va,             "Transport: send to virtual addr"),
	TEST_CASE(tx, loopback,       "Transport: send loopback"),
	TEST_CASE(tx, loopback_group, "Transport: send loopback and group"),
	TEST_CASE(tx, unknown_app,    "Transport: send with unknown app key"),
	TEST_CASE(tx, seg_block,      "Transport: send blocked segmented"),
	TEST_CASE(tx, seg_concurrent, "Transport: send concurrent segmented"),
	TEST_CASE(tx, seg_ivu,        "Transport: send segmented during IV update"),
	TEST_CASE(tx, seg_fail,       "Transport: send segmented to unused addr"),

	TEST_CASE(rx, unicast,        "Transport: receive on unicast addr"),
	TEST_CASE(rx, group,          "Transport: receive on group addr"),
	TEST_CASE(rx, va,             "Transport: receive on virtual addr"),
	TEST_CASE(rx, none,           "Transport: receive no messages"),
	TEST_CASE(rx, seg_block,      "Transport: receive blocked segmented"),
	TEST_CASE(rx, seg_concurrent, "Transport: receive concurrent segmented"),
	TEST_CASE(rx, seg_ivu,        "Transport: receive segmented during IV update"),
	BSTEST_END_MARKER
};

struct bst_test_list *test_transport_install(struct bst_test_list *tests)
{
	tests = bst_add_tests(tests, test_connect);
	return tests;
}
