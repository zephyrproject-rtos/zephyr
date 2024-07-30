/*
 * Copyright (c) 2021 Nordic Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include "mesh_test.h"
#include "mesh/net.h"
#include "mesh/transport.h"
#include "mesh/va.h"
#include <zephyr/sys/byteorder.h>
#include "argparse.h"

#include "friendship_common.h"

#define LOG_MODULE_NAME test_friendship

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(LOG_MODULE_NAME);

/*
 * Friendship tests:
 *   Tests both the friend and the low power role in various scenarios.
 */

#define GROUP_ADDR 0xc000
#define WAIT_TIME 70 /*seconds*/
#define LPN_ADDR_START 0x0003
#define POLL_TIMEOUT_MS (100 * CONFIG_BT_MESH_LPN_POLL_TIMEOUT)

extern enum bst_result_t bst_result;

static const struct bt_mesh_test_cfg friend_cfg = {
	.addr = 0x0001,
	.dev_key = { 0x01 },
};
static const struct bt_mesh_test_cfg other_cfg = {
	.addr = 0x0002,
	.dev_key = { 0x02 },
};
static struct bt_mesh_test_cfg lpn_cfg;

static uint8_t test_va_col_uuid[][16] = {
	{ 0xe3, 0x94, 0xe7, 0xc1, 0xc5, 0x14, 0x72, 0x11,
	  0x68, 0x36, 0x19, 0x30, 0x99, 0x34, 0x53, 0x62 },
	{ 0x5e, 0x49, 0x5a, 0xd9, 0x44, 0xdf, 0xae, 0xc0,
	  0x62, 0xd8, 0x0d, 0xed, 0x16, 0x82, 0xd1, 0x7d },
};
static uint16_t test_va_col_addr = 0x809D;

static void test_friend_init(void)
{
	bt_mesh_test_cfg_set(&friend_cfg, WAIT_TIME);
}

static void test_lpn_init(void)
{
	/* As there may be multiple LPN devices, we'll set the address and
	 * devkey based on the device number, which is guaranteed to be unique
	 * for each device in the simulation.
	 */
	lpn_cfg.addr = LPN_ADDR_START + get_device_nbr();
	lpn_cfg.dev_key[0] = get_device_nbr();
	bt_mesh_test_cfg_set(&lpn_cfg, WAIT_TIME);
}

static void test_other_init(void)
{
	bt_mesh_test_cfg_set(&other_cfg, WAIT_TIME);
}

static void friend_wait_for_polls(int polls)
{
	/* Let LPN poll to get the sent message */
	ASSERT_OK_MSG(bt_mesh_test_friendship_evt_wait(BT_MESH_TEST_FRIEND_POLLED,
						       K_SECONDS(30)), "LPN never polled");

	while (--polls) {
		/* Wait for LPN to poll until the "no more data" message.
		 * At this point, the message has been delivered.
		 */
		ASSERT_OK_MSG(bt_mesh_test_friendship_evt_wait(BT_MESH_TEST_FRIEND_POLLED,
							       K_SECONDS(2)),
			      "LPN missing %d polls", polls);
	}

	if (bt_mesh_test_friendship_evt_wait(BT_MESH_TEST_FRIEND_POLLED,
					     K_SECONDS(2)) != -EAGAIN) {
		FAIL("Unexpected extra poll");
		return;
	}
}

/* Friend test functions */

/** Initialize as a friend and wait for the friendship to be established.
 */
static void test_friend_est(void)
{
	bt_mesh_test_setup();
	bt_mesh_test_friendship_init(CONFIG_BT_MESH_FRIEND_LPN_COUNT);
	bt_mesh_friend_set(BT_MESH_FEATURE_ENABLED);

	ASSERT_OK_MSG(bt_mesh_test_friendship_evt_wait(BT_MESH_TEST_FRIEND_ESTABLISHED,
						       K_SECONDS(5)),
		      "Friendship not established");

	PASS();
}

/** Initialize as a friend, and wait for multiple friendships to be established
 *  concurrently.
 *
 *  Verify that all friendships survive the first poll timeout.
 */
static void test_friend_est_multi(void)
{
	int err;

	bt_mesh_test_setup();
	bt_mesh_test_friendship_init(CONFIG_BT_MESH_FRIEND_LPN_COUNT);
	bt_mesh_friend_set(BT_MESH_FEATURE_ENABLED);

	for (int i = 0; i < CONFIG_BT_MESH_FRIEND_LPN_COUNT; i++) {
		ASSERT_OK_MSG(bt_mesh_test_friendship_evt_wait(BT_MESH_TEST_FRIEND_ESTABLISHED,
							       K_SECONDS(5)),
			      "Friendship %d not established", i);
	}

	/* Wait for all friends to do at least one poll without terminating */
	err = bt_mesh_test_friendship_evt_wait(BT_MESH_TEST_FRIEND_TERMINATED,
		       K_MSEC(POLL_TIMEOUT_MS + 5 * MSEC_PER_SEC));
	if (!err) {
		FAIL("One or more friendships terminated");
	}

	PASS();
}

/** As a friend, send messages to the LPN.
 *
 *  Verifies unsegmented, segmented and multiple packet sending and receiving.
 */
static void test_friend_msg(void)
{
	bt_mesh_test_setup();
	bt_mesh_test_friendship_init(CONFIG_BT_MESH_FRIEND_LPN_COUNT);
	bt_mesh_friend_set(BT_MESH_FEATURE_ENABLED);

	ASSERT_OK_MSG(bt_mesh_test_friendship_evt_wait(BT_MESH_TEST_FRIEND_ESTABLISHED,
						       K_SECONDS(5)),
		      "Friendship not established");
	/* LPN polls on establishment. Clear the poll state */
	bt_mesh_test_friendship_evt_clear(BT_MESH_TEST_FRIEND_POLLED);

	k_sleep(K_SECONDS(1));

	/* Send unsegmented message from friend to LPN: */
	LOG_INF("Sending unsegmented message");
	ASSERT_OK_MSG(bt_mesh_test_send(bt_mesh_test_friendship_addr_get(), NULL, 5, 0,
					K_SECONDS(1)),
		      "Unseg send failed");

	/* Wait for LPN to poll for message and the "no more messages" msg */
	friend_wait_for_polls(2);

	/* Send segmented message */
	ASSERT_OK_MSG(bt_mesh_test_send(bt_mesh_test_friendship_addr_get(), NULL, 13, 0,
					K_SECONDS(1)),
		      "Unseg send failed");

	/* Two segments require 2 polls plus the "no more messages" msg */
	friend_wait_for_polls(3);

	/* Send two unsegmented messages before the next poll.
	 * This tests the friend role's re-encryption mechanism for the second
	 * message, as sending the first message through the network layer
	 * increases the seqnum by one, creating an inconsistency between the
	 * transport and network parts of the second packet.
	 * Ensures coverage for the regression reported in #32033.
	 */
	ASSERT_OK_MSG(bt_mesh_test_send(bt_mesh_test_friendship_addr_get(), NULL,
					BT_MESH_SDU_UNSEG_MAX, 0, K_SECONDS(1)),
		      "Unseg send failed");
	ASSERT_OK_MSG(bt_mesh_test_send(bt_mesh_test_friendship_addr_get(), NULL,
					BT_MESH_SDU_UNSEG_MAX, 0, K_SECONDS(1)),
		      "Unseg send failed");

	/* Two messages require 2 polls plus the "no more messages" msg */
	friend_wait_for_polls(3);

	ASSERT_OK_MSG(bt_mesh_test_recv(5, cfg->addr, NULL, K_SECONDS(10)),
		      "Receive from LPN failed");

	/* Receive a segmented message from the LPN. LPN should poll for the ack
	 * after sending the segments.
	 */
	ASSERT_OK(bt_mesh_test_recv(15, cfg->addr, NULL, K_SECONDS(10)));
	/* - 2 for each SegAck (SegAcks are sent faster than Friend Poll messages);
	 * - The last one with MD == 0;
	 */
	friend_wait_for_polls(2);

	PASS();
}

/** As a friend, overflow the message queue for the LPN with own packets.
 *
 *  Verify that the LPN doesn't terminate the friendship during the poll for
 *  messages.
 */
static void test_friend_overflow(void)
{
	bt_mesh_test_setup();
	bt_mesh_test_friendship_init(CONFIG_BT_MESH_FRIEND_LPN_COUNT);
	bt_mesh_friend_set(BT_MESH_FEATURE_ENABLED);

	ASSERT_OK_MSG(bt_mesh_test_friendship_evt_wait(BT_MESH_TEST_FRIEND_ESTABLISHED,
						       K_SECONDS(5)),
		      "Friendship not established");
	bt_mesh_test_friendship_evt_clear(BT_MESH_TEST_FRIEND_POLLED);

	k_sleep(K_SECONDS(3));

	LOG_INF("Testing overflow with only unsegmented messages...");

	/* Fill the queue */
	for (int i = 0; i < CONFIG_BT_MESH_FRIEND_QUEUE_SIZE; i++) {
		bt_mesh_test_send(bt_mesh_test_friendship_addr_get(), NULL, 5, 0, K_NO_WAIT);
	}

	/* Add one more message, which should overflow the queue and cause the
	 * first message to be discarded.
	 */
	bt_mesh_test_send(bt_mesh_test_friendship_addr_get(), NULL, 5, 0, K_NO_WAIT);

	ASSERT_OK_MSG(bt_mesh_test_friendship_evt_wait(BT_MESH_TEST_FRIEND_POLLED,
						       K_SECONDS(35)),
		      "Friend never polled");

	/* LPN verifies that no more messages are in Friend Queue. */
	k_sleep(K_SECONDS(10));

	LOG_INF("Testing overflow with unsegmented message preempting segmented one...");

	/* Make room in the Friend Queue for only one unsegmented message. */
	bt_mesh_test_send(bt_mesh_test_friendship_addr_get(),
			  NULL, BT_MESH_SDU_UNSEG_MAX *
			  (CONFIG_BT_MESH_FRIEND_QUEUE_SIZE - 1),
			  0, K_SECONDS(1)),
	bt_mesh_test_send(bt_mesh_test_friendship_addr_get(), NULL, 5, 0, K_NO_WAIT);
	/* This message should preempt the segmented one. */
	bt_mesh_test_send(bt_mesh_test_friendship_addr_get(), NULL, 5, 0, K_NO_WAIT);

	ASSERT_OK_MSG(bt_mesh_test_friendship_evt_wait(BT_MESH_TEST_FRIEND_POLLED,
						       K_SECONDS(35)),
		      "Friend never polled");

	/* LPN verifies that there are no more messages in the Friend Queue. */
	k_sleep(K_SECONDS(10));

	LOG_INF("Testing overflow with segmented message preempting another segmented...");

	/* Make space in the Friend Queue for only 2 unsegmented messages so the next unsgemented
	 * message won't preempt this segmented message.
	 */
	bt_mesh_test_send(bt_mesh_test_friendship_addr_get(),
			  NULL, BT_MESH_SDU_UNSEG_MAX *
			  (CONFIG_BT_MESH_FRIEND_QUEUE_SIZE - 2),
			  0, K_SECONDS(1));
	bt_mesh_test_send(bt_mesh_test_friendship_addr_get(), NULL, 5, 0, K_NO_WAIT);
	/* This segmented message should preempt the previous segmented message. */
	bt_mesh_test_send(bt_mesh_test_friendship_addr_get(),
			  NULL, BT_MESH_SDU_UNSEG_MAX *
			  (CONFIG_BT_MESH_FRIEND_QUEUE_SIZE - 2),
			  0, K_SECONDS(1));
	/* This message should fit in Friend Queue as well. */
	bt_mesh_test_send(bt_mesh_test_friendship_addr_get(), NULL, 5, 0, K_NO_WAIT);

	ASSERT_OK_MSG(bt_mesh_test_friendship_evt_wait(BT_MESH_TEST_FRIEND_POLLED,
						       K_SECONDS(35)),
		      "Friend never polled");

	if (bt_mesh_test_friendship_state_check(BT_MESH_TEST_FRIEND_TERMINATED)) {
		FAIL("Friendship terminated unexpectedly");
	}

	PASS();
}

/** Establish a friendship, wait for communication between the LPN and a mesh
 *  device to finish, then send group and virtual addr messages to the LPN.
 *  Let the LPN add another group message, then send to that as well.
 */
static void test_friend_group(void)
{
	const struct bt_mesh_va *va;

	bt_mesh_test_setup();
	bt_mesh_test_friendship_init(CONFIG_BT_MESH_FRIEND_LPN_COUNT);
	bt_mesh_friend_set(BT_MESH_FEATURE_ENABLED);

	ASSERT_OK_MSG(bt_mesh_test_friendship_evt_wait(BT_MESH_TEST_FRIEND_ESTABLISHED,
						       K_SECONDS(5)),
		      "Friendship not established");
	bt_mesh_test_friendship_evt_clear(BT_MESH_TEST_FRIEND_POLLED);

	ASSERT_OK(bt_mesh_va_add(test_va_uuid, &va));

	/* The other mesh device will send its messages in the first poll */
	ASSERT_OK(bt_mesh_test_friendship_evt_wait(BT_MESH_TEST_FRIEND_POLLED,
						   K_SECONDS(10)));

	k_sleep(K_SECONDS(2));

	bt_mesh_test_friendship_evt_clear(BT_MESH_TEST_FRIEND_POLLED);

	/* Send a group message to the LPN */
	ASSERT_OK_MSG(bt_mesh_test_send(GROUP_ADDR, NULL, 5, 0, K_SECONDS(1)),
		      "Failed to send to LPN");
	/* Send a virtual message to the LPN */
	ASSERT_OK_MSG(bt_mesh_test_send(va->addr, va->uuid, 5, 0, K_SECONDS(1)),
		      "Failed to send to LPN");

	/* Wait for the LPN to poll for each message, then for adding the
	 * group address:
	 */
	friend_wait_for_polls(3);

	/* Send a group message to an address the LPN added after the friendship
	 * was established.
	 */
	ASSERT_OK_MSG(bt_mesh_test_send(GROUP_ADDR + 1, NULL, 5, 0, K_SECONDS(1)),
		      "Failed to send to LPN");

	bt_mesh_test_friendship_evt_wait(BT_MESH_TEST_FRIEND_POLLED, K_SECONDS(10));

	PASS();
}


/* Friend no-establish test functions */

/** Initialize as a friend and no friendships to be established.
 */
static void test_friend_no_est(void)
{
	bt_mesh_test_setup();
	bt_mesh_test_friendship_init(CONFIG_BT_MESH_FRIEND_LPN_COUNT);
	bt_mesh_friend_set(BT_MESH_FEATURE_ENABLED);

	if (!bt_mesh_test_friendship_evt_wait(BT_MESH_TEST_FRIEND_ESTABLISHED,
					      K_SECONDS(30))) {
		FAIL("Friendship established unexpectedly");
	}

	PASS();
}

/** Send messages to 2 virtual addresses with collision and check that LPN correctly polls them.
 */
static void test_friend_va_collision(void)
{
	const struct bt_mesh_va *va[2];

	bt_mesh_test_setup();
	bt_mesh_test_friendship_init(CONFIG_BT_MESH_FRIEND_LPN_COUNT);
	bt_mesh_friend_set(BT_MESH_FEATURE_ENABLED);

	ASSERT_OK_MSG(bt_mesh_test_friendship_evt_wait(BT_MESH_TEST_FRIEND_ESTABLISHED,
						       K_SECONDS(5)),
		      "Friendship not established");
	bt_mesh_test_friendship_evt_clear(BT_MESH_TEST_FRIEND_POLLED);

	for (int i = 0; i < ARRAY_SIZE(test_va_col_uuid); i++) {
		ASSERT_OK(bt_mesh_va_add(test_va_col_uuid[i], &va[i]));
		ASSERT_EQUAL(test_va_col_addr, va[i]->addr);
	}

	bt_mesh_test_friendship_evt_wait(BT_MESH_TEST_FRIEND_POLLED, K_SECONDS(10));

	LOG_INF("Step 1: Sending msgs to LPN.");

	/* LPN shall receive the first 2 messages. */
	for (int i = 0; i < ARRAY_SIZE(test_va_col_uuid); i++) {
		/* Send a message to the first virtual address. LPN should receive it. */
		ASSERT_OK_MSG(bt_mesh_test_send(test_va_col_addr, va[i]->uuid, 5, 0, K_SECONDS(1)),
			      "Failed to send to LPN");
	}
	/* One poll per message + Friend Update with md == 0 */
	friend_wait_for_polls(3);

	LOG_INF("Let LPN unsubscribe from the first address.");

	/* Manual poll by LPN test case after removing the first Label UUID from subscription. */
	friend_wait_for_polls(1);

	LOG_INF("Step 2: Sending msgs to LPN.");

	/* Friend will send both messages as the virtual address is the same, but LPN shall
	 * receive only the second message.
	 */
	for (int i = 0; i < ARRAY_SIZE(test_va_col_uuid); i++) {
		ASSERT_OK_MSG(bt_mesh_test_send(test_va_col_addr, va[i]->uuid, 5, 0, K_SECONDS(1)),
			      "Failed to send to LPN");
	}
	/* One poll per message + Friend Update with md == 0 */
	friend_wait_for_polls(3);

	LOG_INF("Let LPN unsubscribe from the second address.");

	/* Manual poll by LPN test case after removing the second Label UUID from subscription.
	 * After this step, the virtual address shall be removed from the subscription list.
	 */
	friend_wait_for_polls(1);

	LOG_INF("Step 3: Sending msgs to LPN.");

	/* Friend shall not send the messages to LPN because it is not subscribed to any virtual
	 * address.
	 */
	for (int i = 0; i < ARRAY_SIZE(test_va_col_uuid); i++) {
		ASSERT_OK_MSG(bt_mesh_test_send(test_va_col_addr, va[i]->uuid, 5, 0, K_SECONDS(1)),
			      "Failed to send to LPN");
	}
	/* Shall be only one Friend Poll as the Friend Queue is empty. */
	friend_wait_for_polls(1);

	PASS();
}


/* LPN test functions */

/** Enable the LPN role, and verify that the friendship is established.
 *
 *  Verify that the friendship survives the first poll timeout.
 */
static void test_lpn_est(void)
{
	bt_mesh_test_setup();
	bt_mesh_test_friendship_init(CONFIG_BT_MESH_FRIEND_LPN_COUNT);

	/* This test is used to establish friendship with single lpn as well as
	 * with many lpn devices. If legacy advertiser is used friendship with
	 * many lpn devices is established normally due to bad precision of advertiser.
	 * If extended advertiser is used simultaneous lpn running causes the situation
	 * when Friend Request from several devices collide in emulated radio channel.
	 * This shift of start moment helps to avoid Friend Request collisions.
	 */
	k_sleep(K_MSEC(10 * get_device_nbr()));

	bt_mesh_lpn_set(true);

	ASSERT_OK_MSG(bt_mesh_test_friendship_evt_wait(BT_MESH_TEST_LPN_ESTABLISHED,
						       K_SECONDS(5)),
		      "LPN not established");
	if (!bt_mesh_test_friendship_evt_wait(BT_MESH_TEST_LPN_TERMINATED,
		      K_MSEC(POLL_TIMEOUT_MS + 5 * MSEC_PER_SEC))) {
		FAIL("Friendship terminated unexpectedly");
	}

	PASS();
}

/** As an LPN, exchange messages with the friend node.
 *
 *  Verifies sending and receiving of unsegmented, segmented and multiple
 *  messages to and from the connected friend node.
 */
static void test_lpn_msg_frnd(void)
{
	bt_mesh_test_setup();
	bt_mesh_test_friendship_init(CONFIG_BT_MESH_FRIEND_LPN_COUNT);
	bt_mesh_lpn_set(true);

	ASSERT_OK_MSG(bt_mesh_test_friendship_evt_wait(BT_MESH_TEST_LPN_ESTABLISHED,
						       K_SECONDS(5)), "LPN not established");
	/* LPN polls on establishment. Clear the poll state */
	bt_mesh_test_friendship_evt_clear(BT_MESH_TEST_LPN_POLLED);

	/* Give friend time to prepare the message */
	k_sleep(K_SECONDS(3));

	/* Receive unsegmented message */
	ASSERT_OK_MSG(bt_mesh_lpn_poll(), "Poll failed");
	ASSERT_OK_MSG(bt_mesh_test_recv(5, cfg->addr, NULL, K_SECONDS(1)),
		      "Failed to receive message");

	/* Give friend time to prepare the message */
	k_sleep(K_SECONDS(3));

	/* Receive segmented message */
	ASSERT_OK_MSG(bt_mesh_lpn_poll(), "Poll failed");
	ASSERT_OK_MSG(bt_mesh_test_recv(13, cfg->addr, NULL, K_SECONDS(2)),
		      "Failed to receive message");

	/* Give friend time to prepare the messages */
	k_sleep(K_SECONDS(3));

	/* Receive two unsegmented messages */
	ASSERT_OK_MSG(bt_mesh_lpn_poll(), "Poll failed");
	ASSERT_OK_MSG(bt_mesh_test_recv(BT_MESH_SDU_UNSEG_MAX, cfg->addr,
					NULL, K_SECONDS(2)),
		      "Failed to receive message");
	ASSERT_OK_MSG(bt_mesh_test_recv(BT_MESH_SDU_UNSEG_MAX, cfg->addr,
					NULL, K_SECONDS(2)),
		      "Failed to receive message");

	k_sleep(K_SECONDS(3));

	/* Send an unsegmented message to the friend.
	 * Should not be affected by the LPN mode at all.
	 */
	ASSERT_OK_MSG(bt_mesh_test_send(friend_cfg.addr, NULL, 5, 0, K_MSEC(500)),
		      "Send to friend failed");

	k_sleep(K_SECONDS(5));

	/* Send a segmented message to the friend. Should trigger a poll for the
	 * ack.
	 */
	ASSERT_OK_MSG(bt_mesh_test_send(friend_cfg.addr, NULL, 15, 0, K_SECONDS(5)),
		      "Send to friend failed");

	PASS();
}

/** As an LPN, exchange messages with a third party mesh node while in a
 *  friendship.
 *
 *  Verifies sending and receiving of unsegmented and segmented messages to and
 *  from the third party node.
 */
static void test_lpn_msg_mesh(void)
{
	bt_mesh_test_setup();
	bt_mesh_test_friendship_init(CONFIG_BT_MESH_FRIEND_LPN_COUNT);
	bt_mesh_lpn_set(true);

	ASSERT_OK_MSG(bt_mesh_test_friendship_evt_wait(BT_MESH_TEST_LPN_ESTABLISHED,
						       K_SECONDS(2)), "LPN not established");
	/* LPN polls on establishment. Clear the poll state */
	bt_mesh_test_friendship_evt_clear(BT_MESH_TEST_LPN_POLLED);

	/* Send an unsegmented message to a third mesh node.
	 * Should not be affected by the LPN mode at all.
	 */
	ASSERT_OK_MSG(bt_mesh_test_send(other_cfg.addr, NULL, 5, 0, K_NO_WAIT),
		      "Send to mesh failed");

	/* Receive an unsegmented message back */
	ASSERT_OK(bt_mesh_test_recv(5, cfg->addr, NULL, K_FOREVER));

	/* Send a segmented message to the mesh node. */
	ASSERT_OK_MSG(bt_mesh_test_send(other_cfg.addr, NULL, 15, 0, K_FOREVER),
		      "Send to other failed");

	/* Receive a segmented message back */
	ASSERT_OK(bt_mesh_test_recv(15, cfg->addr, NULL, K_FOREVER));

	/* Send an unsegmented message with friend credentials to a third mesh
	 * node. The friend shall relay it.
	 */
	test_model->pub->addr = other_cfg.addr;
	test_model->pub->cred = true; /* Use friend credentials */
	test_model->pub->ttl = BT_MESH_TTL_DEFAULT;

	net_buf_simple_reset(test_model->pub->msg);
	bt_mesh_model_msg_init(test_model->pub->msg, TEST_MSG_OP_1);
	ASSERT_OK(bt_mesh_model_publish(test_model));

	PASS();
}

/** As an LPN, establish and terminate a friendship with the same friend
 *  multiple times in a row to ensure that both parties are able to recover.
 */
static void test_lpn_re_est(void)
{
	bt_mesh_test_setup();
	bt_mesh_test_friendship_init(CONFIG_BT_MESH_FRIEND_LPN_COUNT);

	for (int i = 0; i < 4; i++) {
		bt_mesh_lpn_set(true);
		ASSERT_OK_MSG(bt_mesh_test_friendship_evt_wait(BT_MESH_TEST_LPN_ESTABLISHED,
							       K_SECONDS(2)),
			      "LPN not established");

		bt_mesh_lpn_set(false);
		ASSERT_OK_MSG(bt_mesh_test_friendship_evt_wait(BT_MESH_TEST_LPN_TERMINATED,
							       K_SECONDS(5)),
			      "LPN never terminated friendship");

		k_sleep(K_SECONDS(2));
	}

	PASS();
}

/** Establish a friendship as an LPN, and verify that the friendship survives
 *  the first poll timeout without terminating
 */
static void test_lpn_poll(void)
{
	bt_mesh_test_setup();
	bt_mesh_test_friendship_init(CONFIG_BT_MESH_FRIEND_LPN_COUNT);

	bt_mesh_lpn_set(true);
	ASSERT_OK_MSG(bt_mesh_test_friendship_evt_wait(BT_MESH_TEST_LPN_ESTABLISHED,
						       K_SECONDS(5)), "LPN not established");
	bt_mesh_test_friendship_evt_clear(BT_MESH_TEST_LPN_POLLED);

	ASSERT_OK_MSG(bt_mesh_test_friendship_evt_wait(BT_MESH_TEST_LPN_POLLED,
						       K_MSEC(POLL_TIMEOUT_MS)),
		      "LPN failed to poll before the timeout");

	k_sleep(K_SECONDS(10));
	if (bt_mesh_test_friendship_state_check(BT_MESH_TEST_LPN_TERMINATED)) {
		FAIL("LPN terminated.");
	}

	PASS();
}

/** Receive packets from a friend that overflowed its queue. Verify that the
 *  first packet is discarded because of the overflow.
 */
static void test_lpn_overflow(void)
{
	struct bt_mesh_test_msg msg;
	int exp_seq;
	int err;

	bt_mesh_test_setup();
	bt_mesh_test_friendship_init(CONFIG_BT_MESH_FRIEND_LPN_COUNT);

	bt_mesh_lpn_set(true);
	ASSERT_OK_MSG(bt_mesh_test_friendship_evt_wait(BT_MESH_TEST_LPN_ESTABLISHED,
						       K_SECONDS(5)), "LPN not established");
	bt_mesh_test_friendship_evt_clear(BT_MESH_TEST_LPN_POLLED);

	k_sleep(K_SECONDS(5));
	ASSERT_OK_MSG(bt_mesh_lpn_poll(), "Poll failed");

	LOG_INF("Testing overflow with only unsegmented messages...");

	for (int i = 0; i < CONFIG_BT_MESH_FRIEND_QUEUE_SIZE; i++) {
		ASSERT_OK_MSG(bt_mesh_test_recv_msg(&msg, K_SECONDS(2)),
			      "Receive %d failed", i);

		if (msg.len != 5) {
			FAIL("Message %d: Invalid length %d", i, msg.len);
		}

		if (msg.ctx.recv_dst != cfg->addr) {
			FAIL("Message %d: Invalid dst 0x%04x", i,
			     msg.ctx.recv_dst);
		}

		/* The first message (with seq=1) should have been discarded by
		 * the friend, so the first message should have seq=2:
		 */
		if (msg.seq != i + 2) {
			FAIL("Message %d: Invalid seq 0x%02x", i, msg.seq);
		}
	}

	/* Not expecting any more messages from friend */
	err = bt_mesh_test_recv_msg(&msg, K_SECONDS(10));
	if (!err) {
		FAIL("Unexpected additional message 0x%02x from 0x%04x",
		     msg.seq, msg.ctx.addr);
	}

	LOG_INF("Testing overflow with unsegmented message preempting segmented one...");

	ASSERT_OK_MSG(bt_mesh_lpn_poll(), "Poll failed");

	/* Last seq from the previous step. */
	exp_seq = CONFIG_BT_MESH_FRIEND_QUEUE_SIZE + 1;

	exp_seq += 2; /* Skipping the first message in Friend Queue. */
	ASSERT_OK_MSG(bt_mesh_test_recv_msg(&msg, K_SECONDS(2)),
		      "Receive first unseg msg failed");
	ASSERT_EQUAL(5, msg.len);
	ASSERT_EQUAL(cfg->addr, msg.ctx.recv_dst);
	ASSERT_EQUAL(exp_seq, msg.seq);

	exp_seq++;
	ASSERT_OK_MSG(bt_mesh_test_recv_msg(&msg, K_SECONDS(2)),
		      "Receive the second unseg msg failed");
	ASSERT_EQUAL(5, msg.len);
	ASSERT_EQUAL(cfg->addr, msg.ctx.recv_dst);
	ASSERT_EQUAL(exp_seq, msg.seq);

	/* Not expecting any more messages from friend */
	err = bt_mesh_test_recv_msg(&msg, K_SECONDS(10));
	if (!err) {
		FAIL("Unexpected additional message 0x%02x from 0x%04x",
		     msg.seq, msg.ctx.addr);
	}

	LOG_INF("Testing overflow with segmented message preempting another segmented...");

	ASSERT_OK_MSG(bt_mesh_lpn_poll(), "Poll failed");

	exp_seq += 2; /* Skipping the first message in Friend Queue. */
	ASSERT_OK_MSG(bt_mesh_test_recv_msg(&msg, K_SECONDS(2)),
		      "Receive the first unseg msg failed");
	ASSERT_EQUAL(5, msg.len);
	ASSERT_EQUAL(cfg->addr, msg.ctx.recv_dst);
	ASSERT_EQUAL(exp_seq, msg.seq);

	exp_seq++;
	ASSERT_OK_MSG(bt_mesh_test_recv_msg(&msg, K_SECONDS(20)),
		      "Receive the seg msg failed");
	ASSERT_EQUAL(BT_MESH_SDU_UNSEG_MAX * (CONFIG_BT_MESH_FRIEND_QUEUE_SIZE - 2),
		     msg.len);
	ASSERT_EQUAL(cfg->addr, msg.ctx.recv_dst);
	ASSERT_EQUAL(exp_seq, msg.seq);

	ASSERT_OK_MSG(bt_mesh_test_recv_msg(&msg, K_SECONDS(2)),
		      "Receive the second unseg msg failed");
	ASSERT_EQUAL(5, msg.len);
	ASSERT_EQUAL(cfg->addr, msg.ctx.recv_dst);

	/* Not expecting any more messages from friend */
	err = bt_mesh_test_recv_msg(&msg, K_SECONDS(10));
	if (!err) {
		FAIL("Unexpected additional message 0x%02x from 0x%04x",
		     msg.seq, msg.ctx.addr);
	}

	PASS();
}

/** As an LPN, receive packets on group and virtual addresses from mesh device
 *  and friend. Then, add a second group address (while the friendship is
 *  established), and receive on that as well.
 */
static void test_lpn_group(void)
{
	struct bt_mesh_test_msg msg;
	const struct bt_mesh_va *va;
	uint16_t vaddr;
	uint8_t status = 0;
	int err;

	bt_mesh_test_setup();
	bt_mesh_test_friendship_init(CONFIG_BT_MESH_FRIEND_LPN_COUNT);

	err = bt_mesh_cfg_cli_mod_sub_add(0, cfg->addr, cfg->addr, GROUP_ADDR,
				      TEST_MOD_ID, &status);
	if (err || status) {
		FAIL("Group addr add failed with err %d status 0x%x", err,
		     status);
	}

	err = bt_mesh_cfg_cli_mod_sub_va_add(0, cfg->addr, cfg->addr, test_va_uuid,
					 TEST_MOD_ID, &vaddr, &status);
	if (err || status) {
		FAIL("VA addr add failed with err %d status 0x%x", err, status);
	}

	va = bt_mesh_va_find(test_va_uuid);
	ASSERT_TRUE(va != NULL);
	ASSERT_EQUAL(vaddr, va->addr);

	bt_mesh_lpn_set(true);
	ASSERT_OK_MSG(bt_mesh_test_friendship_evt_wait(BT_MESH_TEST_LPN_ESTABLISHED,
						       K_SECONDS(5)), "LPN not established");
	bt_mesh_test_friendship_evt_clear(BT_MESH_TEST_LPN_POLLED);

	/* Send a message to the other mesh device to indicate that the
	 * friendship has been established. Give the other device a time to
	 * start up first.
	 */
	k_sleep(K_MSEC(10));
	ASSERT_OK(bt_mesh_test_send(other_cfg.addr, NULL, 5, 0, K_SECONDS(1)));

	k_sleep(K_SECONDS(5));
	ASSERT_OK_MSG(bt_mesh_lpn_poll(), "Poll failed");

	/* From other device */
	ASSERT_OK(bt_mesh_test_recv_msg(&msg, K_SECONDS(1)));
	if (msg.ctx.recv_dst != GROUP_ADDR || msg.ctx.addr != other_cfg.addr) {
		FAIL("Unexpected message: 0x%04x -> 0x%04x", msg.ctx.addr,
		     msg.ctx.recv_dst);
	}

	ASSERT_OK(bt_mesh_test_recv_msg(&msg, K_SECONDS(1)));
	if (msg.ctx.recv_dst != va->addr || msg.ctx.addr != other_cfg.addr ||
	    msg.ctx.uuid != va->uuid) {
		FAIL("Unexpected message: 0x%04x -> 0x%04x", msg.ctx.addr,
		     msg.ctx.recv_dst);
	}

	k_sleep(K_SECONDS(5));
	ASSERT_OK_MSG(bt_mesh_lpn_poll(), "Poll failed");

	/* From friend */
	ASSERT_OK(bt_mesh_test_recv_msg(&msg, K_SECONDS(1)));
	if (msg.ctx.recv_dst != GROUP_ADDR || msg.ctx.addr != friend_cfg.addr) {
		FAIL("Unexpected message: 0x%04x -> 0x%04x", msg.ctx.addr,
		     msg.ctx.recv_dst);
	}

	ASSERT_OK(bt_mesh_test_recv_msg(&msg, K_SECONDS(1)));
	if (msg.ctx.recv_dst != va->addr || msg.ctx.addr != friend_cfg.addr) {
		FAIL("Unexpected message: 0x%04x -> 0x%04x", msg.ctx.addr,
		     msg.ctx.recv_dst);
	}

	k_sleep(K_SECONDS(1));

	LOG_INF("Adding second group addr");

	/* Add a new group addr, then receive on it to ensure that the friend
	 * has added it to the subscription list.
	 */
	err = bt_mesh_cfg_cli_mod_sub_add(0, cfg->addr, cfg->addr, GROUP_ADDR + 1,
				      TEST_MOD_ID, &status);
	if (err || status) {
		FAIL("Group addr add failed with err %d status 0x%x", err,
		     status);
	}

	k_sleep(K_SECONDS(5));
	ASSERT_OK_MSG(bt_mesh_lpn_poll(), "Poll failed");

	/* From friend on second group address */
	ASSERT_OK(bt_mesh_test_recv_msg(&msg, K_SECONDS(1)));
	if (msg.ctx.recv_dst != GROUP_ADDR + 1 ||
	    msg.ctx.addr != friend_cfg.addr) {
		FAIL("Unexpected message: 0x%04x -> 0x%04x", msg.ctx.addr,
		     msg.ctx.recv_dst);
	}

	PASS();
}

/** As an LPN, send packets to own address to ensure that this is handled by
 *  loopback mechanism, and ignored by friend.
 *
 *  Adds test coverage for regression in #30657.
 */
static void test_lpn_loopback(void)
{
	struct bt_mesh_test_msg msg;
	const struct bt_mesh_va *va;
	uint16_t vaddr;
	uint8_t status = 0;
	int err;

	bt_mesh_test_setup();
	bt_mesh_test_friendship_init(CONFIG_BT_MESH_FRIEND_LPN_COUNT);

	err = bt_mesh_cfg_cli_mod_sub_add(0, cfg->addr, cfg->addr, GROUP_ADDR,
				      TEST_MOD_ID, &status);
	if (err || status) {
		FAIL("Group addr add failed with err %d status 0x%x", err,
		     status);
	}

	err = bt_mesh_cfg_cli_mod_sub_va_add(0, cfg->addr, cfg->addr, test_va_uuid,
					 TEST_MOD_ID, &vaddr, &status);
	if (err || status) {
		FAIL("VA addr add failed with err %d status 0x%x", err, status);
	}

	va = bt_mesh_va_find(test_va_uuid);
	ASSERT_TRUE(va != NULL);
	ASSERT_EQUAL(vaddr, va->addr);

	bt_mesh_lpn_set(true);
	ASSERT_OK_MSG(bt_mesh_test_friendship_evt_wait(BT_MESH_TEST_LPN_ESTABLISHED,
						       K_SECONDS(5)),
		      "LPN not established");
	bt_mesh_test_friendship_evt_clear(BT_MESH_TEST_LPN_POLLED);

	k_sleep(K_SECONDS(1));

	/* Loopback on unicast, shouldn't even leave the device */
	ASSERT_OK(bt_mesh_test_send_async(cfg->addr, NULL, 5, 0, NULL, NULL));
	ASSERT_OK(bt_mesh_test_recv(5, cfg->addr, NULL, K_SECONDS(1)));

	/* Loopback on group address, should not come back from the friend */
	ASSERT_OK(bt_mesh_test_send_async(GROUP_ADDR, NULL, 5, 0, NULL, NULL));
	ASSERT_OK(bt_mesh_test_recv(5, GROUP_ADDR, NULL, K_SECONDS(1)));

	/* Loopback on virtual address, should not come back from the friend */
	ASSERT_OK(bt_mesh_test_send_async(va->addr, va->uuid, 5, 0, NULL, NULL));
	ASSERT_OK(bt_mesh_test_recv(5, va->addr, va->uuid, K_SECONDS(1)));

	ASSERT_OK_MSG(bt_mesh_lpn_poll(), "Poll failed");
	err = bt_mesh_test_recv_msg(&msg, K_SECONDS(2));
	if (err != -ETIMEDOUT) {
		FAIL("Unexpected receive status: %d", err);
	}

	/* Loopback on virtual address, should not come back from the friend */
	ASSERT_OK(bt_mesh_test_send_async(va->addr, va->uuid, 5, 0, NULL, NULL));
	ASSERT_OK(bt_mesh_test_recv(5, va->addr, va->uuid, K_SECONDS(1)));

	k_sleep(K_SECONDS(2));

	/* Poll the friend and make sure we don't receive any messages: */
	ASSERT_OK_MSG(bt_mesh_lpn_poll(), "Poll failed");
	err = bt_mesh_test_recv_msg(&msg, K_SECONDS(5));
	if (err != -ETIMEDOUT) {
		FAIL("Unexpected receive status: %d", err);
	}

	PASS();
}

/* Mesh device test functions */

/** Without engaging in a friendship, communicate with an LPN through a friend
 *  node.
 */
static void test_other_msg(void)
{
	uint8_t status;
	int err;

	bt_mesh_test_setup();
	bt_mesh_test_friendship_init(CONFIG_BT_MESH_FRIEND_LPN_COUNT);

	/* When this device and a friend device receive segments from LPN both start
	 * sending data. This device sends transport ack. Friend relays LPN's segment.
	 * As a consequence of this, the Friend loses transport ack, and the segmented
	 * transaction is never ended. To avoid such behavior this setting will stretch
	 * in time transport ack sending.
	 */
	err = bt_mesh_cfg_cli_net_transmit_set(0, cfg->addr, BT_MESH_TRANSMIT(3, 30), &status);
	if (err || status != BT_MESH_TRANSMIT(3, 30)) {
		FAIL("Net transmit set failed (err %d, status %u)", err, status);
	}

	/* Receive an unsegmented message from the LPN. */
	ASSERT_OK_MSG(bt_mesh_test_recv(5, cfg->addr, NULL, K_FOREVER),
		      "Failed to receive from LPN");

	/* Minor delay that allows LPN's adv to complete sending. */
	k_sleep(K_SECONDS(2));

	/* Send an unsegmented message to the LPN */
	ASSERT_OK_MSG(bt_mesh_test_send(LPN_ADDR_START, NULL, 5, 0, K_NO_WAIT),
		      "Failed to send to LPN");

	/* Receive a segmented message from the LPN. */
	ASSERT_OK_MSG(bt_mesh_test_recv(15, cfg->addr, NULL, K_FOREVER),
		      "Failed to receive from LPN");

	/* Minor delay that allows LPN's adv to complete sending. */
	k_sleep(K_SECONDS(2));

	/* Send a segmented message to the friend. */
	ASSERT_OK_MSG(bt_mesh_test_send(LPN_ADDR_START, NULL, 15, 0, K_FOREVER),
		      "Send to LPN failed");

	/* Receive an unsegmented message from the LPN, originally sent with
	 * friend credentials.
	 */
	ASSERT_OK_MSG(bt_mesh_test_recv(1, cfg->addr, NULL, K_FOREVER),
		      "Failed to receive from LPN");

	PASS();
}

/** Without engaging in a friendship, send group and virtual addr messages to
 *  the LPN.
 */
static void test_other_group(void)
{
	const struct bt_mesh_va *va;

	bt_mesh_test_setup();
	bt_mesh_test_friendship_init(CONFIG_BT_MESH_FRIEND_LPN_COUNT);

	ASSERT_OK(bt_mesh_va_add(test_va_uuid, &va));

	/* Wait for LPN to send us a message after establishing the friendship */
	ASSERT_OK(bt_mesh_test_recv(5, cfg->addr, NULL, K_SECONDS(1)));

	/* Send a group message to the LPN */
	ASSERT_OK_MSG(bt_mesh_test_send(GROUP_ADDR, NULL, 5, 0, K_SECONDS(1)),
		      "Failed to send to LPN");
	/* Send a virtual message to the LPN */
	ASSERT_OK_MSG(bt_mesh_test_send(va->addr, va->uuid, 5, 0, K_SECONDS(1)),
		      "Failed to send to LPN");

	PASS();
}

/** LPN disable test.
 *
 * Check that toggling lpn_set() results in correct disabled state
 */
static void test_lpn_disable(void)
{
	bt_mesh_test_setup();
	bt_mesh_test_friendship_init(CONFIG_BT_MESH_FRIEND_LPN_COUNT);

	bt_mesh_lpn_set(true);
	bt_mesh_lpn_set(false);

	if (!bt_mesh_test_friendship_evt_wait(BT_MESH_TEST_LPN_POLLED, K_SECONDS(30))) {
		FAIL("LPN connection polled unexpectedly");
	}

	PASS();
}

/** LPN terminate cb test.
 *
 * Check that terminate cb is not triggered when there is no established
 * connection.
 */
static void test_lpn_term_cb_check(void)
{
	bt_mesh_test_setup();
	bt_mesh_test_friendship_init(CONFIG_BT_MESH_FRIEND_LPN_COUNT);

	bt_mesh_lpn_set(true);
	ASSERT_OK_MSG(bt_mesh_test_friendship_evt_wait(BT_MESH_TEST_LPN_POLLED,
						       K_MSEC(1000)), "Friend never polled");
	bt_mesh_lpn_set(false);

	if (!bt_mesh_test_friendship_evt_wait(BT_MESH_TEST_LPN_TERMINATED, K_SECONDS(30))) {
		FAIL("LPN terminate CB triggered unexpectedly");
	}

	PASS();
}

/** Test that LPN sends only one Subscription List Add and only one Subscription List Remove message
 * to Friend when LPN is subscribed to 2 virtual addresses with collision.
 */
static void test_lpn_va_collision(void)
{
	struct bt_mesh_test_msg msg;
	const struct bt_mesh_va *va[2];
	uint16_t vaddr;
	uint8_t status = 0;
	int err;

	bt_mesh_test_setup();
	bt_mesh_test_friendship_init(CONFIG_BT_MESH_FRIEND_LPN_COUNT);

	/* Subscripbe LPN on both virtual address with collision. */
	for (int i = 0; i < ARRAY_SIZE(test_va_col_uuid); i++) {
		err = bt_mesh_cfg_cli_mod_sub_va_add(0, cfg->addr, cfg->addr, test_va_col_uuid[i],
						 TEST_MOD_ID, &vaddr, &status);
		if (err || status) {
			FAIL("VA addr add failed with err %d status 0x%x", err, status);
		}

		ASSERT_EQUAL(test_va_col_addr, vaddr);

		va[i] = bt_mesh_va_find(test_va_col_uuid[i]);
		ASSERT_TRUE(va[i] != NULL);
		ASSERT_EQUAL(vaddr, va[i]->addr);
	}

	bt_mesh_lpn_set(true);
	ASSERT_OK_MSG(bt_mesh_test_friendship_evt_wait(BT_MESH_TEST_LPN_ESTABLISHED,
						       K_SECONDS(5)), "LPN not established");
	bt_mesh_test_friendship_evt_clear(BT_MESH_TEST_LPN_POLLED);

	LOG_INF("Step 1: Waiting for msgs from Friend");

	/* Let Friend prepare messages and the poll them. */
	k_sleep(K_SECONDS(3));
	ASSERT_OK_MSG(bt_mesh_lpn_poll(), "Poll failed");
	/* LPN shall receive both messages. */
	for (int i = 0; i < ARRAY_SIZE(test_va_col_uuid); i++) {
		ASSERT_OK(bt_mesh_test_recv_msg(&msg, K_SECONDS(10)));
		if (msg.ctx.recv_dst != va[i]->addr ||
		    msg.ctx.uuid != va[i]->uuid ||
		    msg.ctx.addr != friend_cfg.addr) {
			FAIL("Unexpected message: 0x%04x -> 0x%04x, uuid: %p", msg.ctx.addr,
			     msg.ctx.recv_dst, msg.ctx.uuid);
		}
	}
	/* Wait for the extra poll timeout in friend_wait_for_polls(). */
	k_sleep(K_SECONDS(3));

	LOG_INF("Unsubscribing from the first address.");

	/* Remove the first virtual address from subscription and poll messages from Friend. This
	 * call shall not generate Friend Subscription List Remove message because LPN is still
	 * subscribed to another Label UUID with the same  virtual address.
	 */
	err = bt_mesh_cfg_cli_mod_sub_va_del(0, cfg->addr, cfg->addr, test_va_col_uuid[0],
				      TEST_MOD_ID, &vaddr, &status);
	if (err || status) {
		FAIL("Virtual addr add failed with err %d status 0x%x", err, status);
	}
	ASSERT_OK_MSG(bt_mesh_lpn_poll(), "Poll failed");
	/* Wait for the extra poll timeout in friend_wait_for_polls(). */
	k_sleep(K_SECONDS(3));

	LOG_INF("Step 2: Waiting for msgs from Friend");

	/* LPN will still receive both messages as the virtual address is the same  for both Label
	 * UUIDs, but the first message shall not be decrypted and shall be dropped.
	 */
	ASSERT_OK_MSG(bt_mesh_lpn_poll(), "Poll failed");
	ASSERT_OK(bt_mesh_test_recv_msg(&msg, K_SECONDS(1)));
	if (msg.ctx.recv_dst != va[1]->addr || msg.ctx.uuid != va[1]->uuid ||
	    msg.ctx.addr != friend_cfg.addr) {
		FAIL("Unexpected message: 0x%04x -> 0x%04x, uuid: %p", msg.ctx.addr,
		     msg.ctx.recv_dst, msg.ctx.uuid);
	}

	/* Check that there are no more messages from Friend. */
	err = bt_mesh_test_recv_msg(&msg, K_SECONDS(1));
	if (!err) {
		FAIL("Unexpected message: 0x%04x -> 0x%04x, uuid: %p", msg.ctx.addr,
		     msg.ctx.recv_dst, msg.ctx.uuid);
	}
	/* Wait for the extra poll timeout in friend_wait_for_polls(). */
	k_sleep(K_SECONDS(3));

	LOG_INF("Unsubscribing from the second address.");

	/* Unsubscribe from the second address. Now there are no subscriptions to the same virtual
	 * address. LPN shall send  Subscription List Remove message.
	 */
	err = bt_mesh_cfg_cli_mod_sub_va_del(0, cfg->addr, cfg->addr, test_va_col_uuid[1],
				      TEST_MOD_ID, &vaddr, &status);
	if (err || status) {
		FAIL("Virtual addr del failed with err %d status 0x%x", err, status);
	}
	ASSERT_OK_MSG(bt_mesh_lpn_poll(), "Poll failed");
	/* Wait for the extra poll timeout in friend_wait_for_polls(). */
	k_sleep(K_SECONDS(3));

	LOG_INF("Step 3: Waiting for msgs from Friend");

	/* As now there shall be no virtual addresses in the subscription list, Friend Queue shall
	 * be empty.
	 */
	ASSERT_OK_MSG(bt_mesh_lpn_poll(), "Poll failed");
	for (int i = 0; i < ARRAY_SIZE(test_va_col_uuid); i++) {
		err = bt_mesh_test_recv_msg(&msg, K_SECONDS(1));
		if (!err) {
			FAIL("Unexpected message: 0x%04x -> 0x%04x, uuid: %p", msg.ctx.addr,
			     msg.ctx.recv_dst, msg.ctx.uuid);
		}
	}

	PASS();
}

#define TEST_CASE(role, name, description)                  \
	{                                                   \
		.test_id = "friendship_" #role "_" #name,   \
		.test_descr = description,                  \
		.test_post_init_f = test_##role##_init,     \
		.test_tick_f = bt_mesh_test_timeout,        \
		.test_main_f = test_##role##_##name,        \
	}

static const struct bst_test_instance test_connect[] = {
	TEST_CASE(friend, est,              "Friend: establish friendship"),
	TEST_CASE(friend, est_multi,        "Friend: establish multiple friendships"),
	TEST_CASE(friend, msg,              "Friend: message exchange"),
	TEST_CASE(friend, overflow,         "Friend: message queue overflow"),
	TEST_CASE(friend, group,            "Friend: send to group addrs"),
	TEST_CASE(friend, no_est,           "Friend: do not establish friendship"),
	TEST_CASE(friend, va_collision,     "Friend: send to virtual addrs with collision"),

	TEST_CASE(lpn,    est,              "LPN: establish friendship"),
	TEST_CASE(lpn,    msg_frnd,         "LPN: message exchange with friend"),
	TEST_CASE(lpn,    msg_mesh,         "LPN: message exchange with mesh"),
	TEST_CASE(lpn,    re_est,           "LPN: re-establish friendship"),
	TEST_CASE(lpn,    poll,             "LPN: poll before timeout"),
	TEST_CASE(lpn,    overflow,         "LPN: message queue overflow"),
	TEST_CASE(lpn,    group,            "LPN: receive on group addrs"),
	TEST_CASE(lpn,    loopback,         "LPN: send to loopback addrs"),
	TEST_CASE(lpn,    disable,          "LPN: disable LPN"),
	TEST_CASE(lpn,    term_cb_check,    "LPN: no terminate cb trigger"),
	TEST_CASE(lpn,    va_collision,     "LPN: receive on virtual addrs with collision"),

	TEST_CASE(other,  msg,              "Other mesh device: message exchange"),
	TEST_CASE(other,  group,            "Other mesh device: send to group addrs"),
	BSTEST_END_MARKER
};

struct bst_test_list *test_friendship_install(struct bst_test_list *tests)
{
	tests = bst_add_tests(tests, test_connect);
	return tests;
}
