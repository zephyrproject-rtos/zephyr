/*
 * Copyright (c) 2021 Nordic Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include "mesh_test.h"
#include "mesh/net.h"
#include "mesh/transport.h"
#include <sys/byteorder.h>

#define LOG_MODULE_NAME test_friendship

#include <logging/log.h>
LOG_MODULE_REGISTER(LOG_MODULE_NAME);

/*
 * Friendship tests:
 *   Tests both the friend and the low power role in various scenarios.
 */

#define GROUP_ADDR 0xc000
#define WAIT_TIME 60 /*seconds*/
#define LPN_ADDR_START 0x0003
#define POLL_TIMEOUT_MS (100 * CONFIG_BT_MESH_LPN_POLL_TIMEOUT)

extern enum bst_result_t bst_result;

enum test_flags {
	LPN_ESTABLISHED,
	LPN_TERMINATED,
	LPN_POLLED,
	FRIEND_ESTABLISHED,
	FRIEND_TERMINATED,
	FRIEND_POLLED,

	TEST_FLAGS,
};

static ATOMIC_DEFINE(state, TEST_FLAGS);
static struct k_sem events[TEST_FLAGS];

static const struct bt_mesh_test_cfg friend_cfg = {
	.addr = 0x0001,
	.dev_key = { 0x01 },
};
static const struct bt_mesh_test_cfg other_cfg = {
	.addr = 0x0002,
	.dev_key = { 0x02 },
};
static struct bt_mesh_test_cfg lpn_cfg;
static uint16_t friend_lpn_addr;

static void test_common_init(const struct bt_mesh_test_cfg *cfg)
{
	for (int i = 0; i < ARRAY_SIZE(events); i++) {
		k_sem_init(&events[i], 0, 1);
	}

	bt_mesh_test_cfg_set(cfg, WAIT_TIME);
}

static void test_friend_init(void)
{
	test_common_init(&friend_cfg);
}

static void test_lpn_init(void)
{
	/* As there may be multiple LPN devices, we'll set the address and
	 * devkey based on the device number, which is guaranteed to be unique
	 * for each device in the simulation.
	 */
	extern uint global_device_nbr;

	lpn_cfg.addr = LPN_ADDR_START + global_device_nbr;
	lpn_cfg.dev_key[0] = global_device_nbr;
	test_common_init(&lpn_cfg);
}

static void test_other_init(void)
{
	test_common_init(&other_cfg);
}

static void evt_signal(enum test_flags evt)
{
	atomic_set_bit(state, evt);
	k_sem_give(&events[evt]);
}

static int evt_wait(enum test_flags evt, k_timeout_t timeout)
{
	return k_sem_take(&events[evt], timeout);
}

static void evt_clear(enum test_flags evt)
{
	atomic_clear_bit(state, evt);
	k_sem_reset(&events[evt]);
}

static void friend_established(uint16_t net_idx, uint16_t lpn_addr,
			    uint8_t recv_delay, uint32_t polltimeout)
{
	LOG_INF("Friend: established with 0x%04x", lpn_addr);
	friend_lpn_addr = lpn_addr;
	evt_signal(FRIEND_ESTABLISHED);
}

static void friend_terminated(uint16_t net_idx, uint16_t lpn_addr)
{
	LOG_INF("Friend: terminated with 0x%04x", lpn_addr);
	evt_signal(FRIEND_TERMINATED);
}

static void friend_polled(uint16_t net_idx, uint16_t lpn_addr)
{
	LOG_INF("Friend: Poll from 0x%04x", lpn_addr);
	evt_signal(FRIEND_POLLED);
}

BT_MESH_FRIEND_CB_DEFINE(friend) = {
	.established = friend_established,
	.terminated = friend_terminated,
	.polled = friend_polled,
};

static void lpn_established(uint16_t net_idx, uint16_t friend_addr,
			    uint8_t queue_size, uint8_t recv_window)
{
	LOG_INF("LPN: established with 0x%04x", friend_addr);
	evt_signal(LPN_ESTABLISHED);
}

static void lpn_terminated(uint16_t net_idx, uint16_t friend_addr)
{
	LOG_INF("LPN: terminated with 0x%04x", friend_addr);
	evt_signal(LPN_TERMINATED);
}

static void lpn_polled(uint16_t net_idx, uint16_t friend_addr, bool retry)
{
	LOG_INF("LPN: Polling 0x%04x (%s)", friend_addr,
		retry ? "retry" : "initial");
	evt_signal(LPN_POLLED);
}

BT_MESH_LPN_CB_DEFINE(lpn) = {
	.established = lpn_established,
	.polled = lpn_polled,
	.terminated = lpn_terminated,
};

static void friend_wait_for_polls(int polls)
{
	/* Let LPN poll to get the sent message */
	ASSERT_OK(evt_wait(FRIEND_POLLED, K_SECONDS(30)), "LPN never polled");

	while (--polls) {
		/* Wait for LPN to poll until the "no more data" message.
		 * At this point, the message has been delivered.
		 */
		ASSERT_OK(evt_wait(FRIEND_POLLED, K_SECONDS(2)),
			  "LPN missing %d polls", polls);
	}

	if (evt_wait(FRIEND_POLLED, K_SECONDS(2)) != -EAGAIN) {
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

	bt_mesh_friend_set(BT_MESH_FEATURE_ENABLED);

	ASSERT_OK(evt_wait(FRIEND_ESTABLISHED, K_SECONDS(5)),
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

	k_sem_init(&events[FRIEND_ESTABLISHED], 0,
		   CONFIG_BT_MESH_FRIEND_LPN_COUNT);

	bt_mesh_friend_set(BT_MESH_FEATURE_ENABLED);

	for (int i = 0; i < CONFIG_BT_MESH_FRIEND_LPN_COUNT; i++) {
		ASSERT_OK(evt_wait(FRIEND_ESTABLISHED, K_SECONDS(5)),
			  "Friendship %d not established", i);
	}

	/* Wait for all friends to do at least one poll without terminating */
	err = evt_wait(FRIEND_TERMINATED,
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

	bt_mesh_friend_set(BT_MESH_FEATURE_ENABLED);

	ASSERT_OK(evt_wait(FRIEND_ESTABLISHED, K_SECONDS(5)),
		  "Friendship not established");
	/* LPN polls on establishment. Clear the poll state */
	evt_clear(FRIEND_POLLED);

	k_sleep(K_SECONDS(1));

	/* Send unsegmented message from friend to LPN: */
	LOG_INF("Sending unsegmented message");
	ASSERT_OK(bt_mesh_test_send(friend_lpn_addr, 5, 0, K_SECONDS(1)),
		  "Unseg send failed");

	/* Wait for LPN to poll for message and the "no more messages" msg */
	friend_wait_for_polls(2);

	/* Send segmented message */
	ASSERT_OK(bt_mesh_test_send(friend_lpn_addr, 13, 0, K_SECONDS(1)),
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
	ASSERT_OK(bt_mesh_test_send(friend_lpn_addr, 5, 0, K_SECONDS(1)),
		  "Unseg send failed");
	ASSERT_OK(bt_mesh_test_send(friend_lpn_addr, 5, 0, K_SECONDS(1)),
		  "Unseg send failed");

	/* Two messages require 2 polls plus the "no more messages" msg */
	friend_wait_for_polls(3);

	ASSERT_OK(bt_mesh_test_recv(5, cfg->addr, K_SECONDS(10)),
		  "Receive from LPN failed");

	/* Receive a segmented message from the LPN. LPN should poll for the ack
	 * after sending the segments.
	 */
	ASSERT_OK(bt_mesh_test_recv(15, cfg->addr, K_SECONDS(10)),
		  "Receive from LPN failed");
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

	bt_mesh_friend_set(BT_MESH_FEATURE_ENABLED);

	ASSERT_OK(evt_wait(FRIEND_ESTABLISHED, K_SECONDS(5)),
		  "Friendship not established");
	evt_clear(FRIEND_POLLED);

	k_sleep(K_SECONDS(3));

	/* Fill the queue */
	for (int i = 0; i < CONFIG_BT_MESH_FRIEND_QUEUE_SIZE; i++) {
		bt_mesh_test_send(friend_lpn_addr, 5, 0, K_NO_WAIT);
	}

	/* Add one more message, which should overflow the queue and cause the
	 * first message to be discarded.
	 */
	bt_mesh_test_send(friend_lpn_addr, 5, 0, K_NO_WAIT);

	ASSERT_OK(evt_wait(FRIEND_POLLED, K_SECONDS(35)),
		  "Friend never polled");

	if (atomic_test_bit(state, FRIEND_TERMINATED)) {
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
	uint16_t virtual_addr;

	bt_mesh_test_setup();

	bt_mesh_friend_set(BT_MESH_FEATURE_ENABLED);

	ASSERT_OK(evt_wait(FRIEND_ESTABLISHED, K_SECONDS(5)),
		  "Friendship not established");
	evt_clear(FRIEND_POLLED);

	ASSERT_OK(bt_mesh_va_add(test_va_uuid, &virtual_addr));

	/* The other mesh device will send its messages in the first poll */
	ASSERT_OK(evt_wait(FRIEND_POLLED, K_SECONDS(10)));

	k_sleep(K_SECONDS(2));

	evt_clear(FRIEND_POLLED);

	/* Send a group message to the LPN */
	ASSERT_OK(bt_mesh_test_send(GROUP_ADDR, 5, 0, K_SECONDS(1)),
		  "Failed to send to LPN");
	/* Send a virtual message to the LPN */
	ASSERT_OK(bt_mesh_test_send(virtual_addr, 5, 0, K_SECONDS(1)),
		  "Failed to send to LPN");

	/* Wait for the LPN to poll for each message, then for adding the
	 * group address:
	 */
	friend_wait_for_polls(3);

	/* Send a group message to an address the LPN added after the friendship
	 * was established.
	 */
	ASSERT_OK(bt_mesh_test_send(GROUP_ADDR + 1, 5, 0, K_SECONDS(1)),
		  "Failed to send to LPN");

	evt_wait(FRIEND_POLLED, K_SECONDS(10));

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

	bt_mesh_lpn_set(true);

	ASSERT_OK(evt_wait(LPN_ESTABLISHED, K_SECONDS(5)),
		  "LPN not established");
	if (!evt_wait(LPN_TERMINATED,
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

	bt_mesh_lpn_set(true);

	ASSERT_OK(evt_wait(LPN_ESTABLISHED, K_SECONDS(5)),
		  "LPN not established");
	/* LPN polls on establishment. Clear the poll state */
	evt_clear(LPN_POLLED);

	/* Give friend time to prepare the message */
	k_sleep(K_SECONDS(3));

	/* Receive unsegmented message */
	ASSERT_OK(bt_mesh_lpn_poll(), "Poll failed");
	ASSERT_OK(bt_mesh_test_recv(5, cfg->addr, K_SECONDS(1)),
		  "Failed to receive message");

	/* Give friend time to prepare the message */
	k_sleep(K_SECONDS(3));

	/* Receive segmented message */
	ASSERT_OK(bt_mesh_lpn_poll(), "Poll failed");
	ASSERT_OK(bt_mesh_test_recv(13, cfg->addr, K_SECONDS(2)),
		  "Failed to receive message");

	/* Give friend time to prepare the messages */
	k_sleep(K_SECONDS(3));

	/* Receive two unsegmented messages */
	ASSERT_OK(bt_mesh_lpn_poll(), "Poll failed");
	ASSERT_OK(bt_mesh_test_recv(5, cfg->addr, K_SECONDS(2)),
		  "Failed to receive message");
	ASSERT_OK(bt_mesh_test_recv(5, cfg->addr, K_SECONDS(2)),
		  "Failed to receive message");

	k_sleep(K_SECONDS(3));

	/* Send an unsegmented message to the friend.
	 * Should not be affected by the LPN mode at all.
	 */
	ASSERT_OK(bt_mesh_test_send(friend_cfg.addr, 5, 0, K_MSEC(500)),
		  "Send to friend failed");

	k_sleep(K_SECONDS(5));

	/* Send a segmented message to the friend. Should trigger a poll for the
	 * ack.
	 */
	ASSERT_OK(bt_mesh_test_send(friend_cfg.addr, 15, 0, K_SECONDS(5)),
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

	bt_mesh_lpn_set(true);

	ASSERT_OK(evt_wait(LPN_ESTABLISHED, K_SECONDS(2)),
		  "LPN not established");
	/* LPN polls on establishment. Clear the poll state */
	evt_clear(LPN_POLLED);

	/* Send an unsegmented message to a third mesh node.
	 * Should not be affected by the LPN mode at all.
	 */
	ASSERT_OK(bt_mesh_test_send(other_cfg.addr, 5, 0, K_MSEC(500)),
		  "Send to mesh failed");

	/* Receive an unsegmented message back */
	k_sleep(K_SECONDS(1));
	ASSERT_OK(bt_mesh_lpn_poll());
	ASSERT_OK(bt_mesh_test_recv(5, cfg->addr, K_SECONDS(2)));

	k_sleep(K_SECONDS(1));

	/* Send a segmented message to the mesh node.
	 * Should trigger a poll for the ack.
	 */
	ASSERT_OK(bt_mesh_test_send(other_cfg.addr, 15, 0, K_SECONDS(5)),
		  "Send to other failed");

	/* Receive a segmented message back */
	k_sleep(K_SECONDS(1));
	ASSERT_OK(bt_mesh_lpn_poll());
	ASSERT_OK(bt_mesh_test_recv(15, cfg->addr, K_SECONDS(5)));

	/* Send an unsegmented message with friend credentials to a third mesh
	 * node. The friend shall relay it.
	 */
	test_model->pub->addr = other_cfg.addr;
	test_model->pub->cred = true; /* Use friend credentials */
	test_model->pub->ttl = BT_MESH_TTL_DEFAULT;

	net_buf_simple_reset(test_model->pub->msg);
	bt_mesh_model_msg_init(test_model->pub->msg, TEST_MSG_OP);
	ASSERT_OK(bt_mesh_model_publish(test_model));

	PASS();
}

/** As an LPN, establish and terminate a friendship with the same friend
 *  multiple times in a row to ensure that both parties are able to recover.
 */
static void test_lpn_re_est(void)
{
	bt_mesh_test_setup();

	for (int i = 0; i < 4; i++) {
		bt_mesh_lpn_set(true);
		ASSERT_OK(evt_wait(LPN_ESTABLISHED, K_SECONDS(2)),
			"LPN not established");

		bt_mesh_lpn_set(false);
		ASSERT_OK(evt_wait(LPN_TERMINATED, K_SECONDS(5)),
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

	bt_mesh_lpn_set(true);
	ASSERT_OK(evt_wait(LPN_ESTABLISHED, K_SECONDS(5)),
		  "LPN not established");
	evt_clear(LPN_POLLED);

	ASSERT_OK(evt_wait(LPN_POLLED, K_MSEC(POLL_TIMEOUT_MS)),
		  "LPN failed to poll before the timeout");

	k_sleep(K_SECONDS(10));
	if (atomic_test_bit(state, LPN_TERMINATED)) {
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
	int err;

	bt_mesh_test_setup();

	bt_mesh_lpn_set(true);
	ASSERT_OK(evt_wait(LPN_ESTABLISHED, K_SECONDS(5)),
		  "LPN not established");
	evt_clear(LPN_POLLED);

	k_sleep(K_SECONDS(5));
	ASSERT_OK(bt_mesh_lpn_poll(), "Poll failed");

	for (int i = 0; i < CONFIG_BT_MESH_FRIEND_QUEUE_SIZE; i++) {
		ASSERT_OK(bt_mesh_test_recv_msg(&msg, K_SECONDS(2)),
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

	PASS();
}

/** As an LPN, receive packets on group and virtual addresses from mesh device
 *  and friend. Then, add a second group address (while the friendship is
 *  established), and receive on that as well.
 */
static void test_lpn_group(void)
{
	struct bt_mesh_test_msg msg;
	uint16_t vaddr;
	uint8_t status = 0;
	int err;

	bt_mesh_test_setup();

	err = bt_mesh_cfg_mod_sub_add(0, cfg->addr, cfg->addr, GROUP_ADDR,
				      TEST_MOD_ID, &status);
	if (err || status) {
		FAIL("Group addr add failed with err %d status 0x%x", err,
		     status);
	}

	err = bt_mesh_cfg_mod_sub_va_add(0, cfg->addr, cfg->addr, test_va_uuid,
					 TEST_MOD_ID, &vaddr, &status);
	if (err || status) {
		FAIL("VA addr add failed with err %d status 0x%x", err, status);
	}

	bt_mesh_lpn_set(true);
	ASSERT_OK(evt_wait(LPN_ESTABLISHED, K_SECONDS(5)),
		  "LPN not established");
	evt_clear(LPN_POLLED);

	/* Send a message to the other mesh device to indicate that the
	 * friendship has been established. Give the other device a time to
	 * start up first.
	 */
	k_sleep(K_MSEC(10));
	ASSERT_OK(bt_mesh_test_send(other_cfg.addr, 5, 0, K_SECONDS(1)));

	k_sleep(K_SECONDS(5));
	ASSERT_OK(bt_mesh_lpn_poll(), "Poll failed");

	/* From other device */
	ASSERT_OK(bt_mesh_test_recv_msg(&msg, K_SECONDS(1)));
	if (msg.ctx.recv_dst != GROUP_ADDR || msg.ctx.addr != other_cfg.addr) {
		FAIL("Unexpected message: 0x%04x -> 0x%04x", msg.ctx.addr,
		     msg.ctx.recv_dst);
	}

	ASSERT_OK(bt_mesh_test_recv_msg(&msg, K_SECONDS(1)));
	if (msg.ctx.recv_dst != vaddr || msg.ctx.addr != other_cfg.addr) {
		FAIL("Unexpected message: 0x%04x -> 0x%04x", msg.ctx.addr,
		     msg.ctx.recv_dst);
	}

	k_sleep(K_SECONDS(5));
	ASSERT_OK(bt_mesh_lpn_poll(), "Poll failed");

	/* From friend */
	ASSERT_OK(bt_mesh_test_recv_msg(&msg, K_SECONDS(1)));
	if (msg.ctx.recv_dst != GROUP_ADDR || msg.ctx.addr != friend_cfg.addr) {
		FAIL("Unexpected message: 0x%04x -> 0x%04x", msg.ctx.addr,
		     msg.ctx.recv_dst);
	}

	ASSERT_OK(bt_mesh_test_recv_msg(&msg, K_SECONDS(1)));
	if (msg.ctx.recv_dst != vaddr || msg.ctx.addr != friend_cfg.addr) {
		FAIL("Unexpected message: 0x%04x -> 0x%04x", msg.ctx.addr,
		     msg.ctx.recv_dst);
	}

	k_sleep(K_SECONDS(1));

	LOG_INF("Adding second group addr");

	/* Add a new group addr, then receive on it to ensure that the friend
	 * has added it to the subscription list.
	 */
	err = bt_mesh_cfg_mod_sub_add(0, cfg->addr, cfg->addr, GROUP_ADDR + 1,
				      TEST_MOD_ID, &status);
	if (err || status) {
		FAIL("Group addr add failed with err %d status 0x%x", err,
		     status);
	}

	k_sleep(K_SECONDS(5));
	ASSERT_OK(bt_mesh_lpn_poll(), "Poll failed");

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
	uint16_t vaddr;
	uint8_t status = 0;
	int err;

	bt_mesh_test_setup();

	err = bt_mesh_cfg_mod_sub_add(0, cfg->addr, cfg->addr, GROUP_ADDR,
				      TEST_MOD_ID, &status);
	if (err || status) {
		FAIL("Group addr add failed with err %d status 0x%x", err,
		     status);
	}

	err = bt_mesh_cfg_mod_sub_va_add(0, cfg->addr, cfg->addr, test_va_uuid,
					 TEST_MOD_ID, &vaddr, &status);
	if (err || status) {
		FAIL("VA addr add failed with err %d status 0x%x", err, status);
	}

	bt_mesh_lpn_set(true);
	ASSERT_OK(evt_wait(LPN_ESTABLISHED, K_SECONDS(5)),
		  "LPN not established");
	evt_clear(LPN_POLLED);

	k_sleep(K_SECONDS(1));

	/* Loopback on unicast, shouldn't even leave the device */
	ASSERT_OK(bt_mesh_test_send_async(cfg->addr, 5, 0, NULL, NULL));
	ASSERT_OK(bt_mesh_test_recv(5, cfg->addr, K_SECONDS(1)));

	/* Loopback on group address, should not come back from the friend */
	ASSERT_OK(bt_mesh_test_send_async(GROUP_ADDR, 5, 0, NULL, NULL));
	ASSERT_OK(bt_mesh_test_recv(5, GROUP_ADDR, K_SECONDS(1)));

	ASSERT_OK(bt_mesh_lpn_poll(), "Poll failed");
	err = bt_mesh_test_recv_msg(&msg, K_SECONDS(2));
	if (err != -ETIMEDOUT) {
		FAIL("Unexpected receive status: %d", err);
	}

	/* Loopback on virtual address, should not come back from the friend */
	ASSERT_OK(bt_mesh_test_send_async(vaddr, 5, 0, NULL, NULL));
	ASSERT_OK(bt_mesh_test_recv(5, vaddr, K_SECONDS(1)));

	k_sleep(K_SECONDS(2));

	/* Poll the friend and make sure we don't receive any messages: */
	ASSERT_OK(bt_mesh_lpn_poll(), "Poll failed");
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
	bt_mesh_test_setup();

	/* Receive an unsegmented message from the LPN. */
	ASSERT_OK(bt_mesh_test_recv(5, cfg->addr, K_SECONDS(4)),
		  "Failed to receive from LPN");

	/* Send an unsegmented message to the LPN */
	ASSERT_OK(bt_mesh_test_send(LPN_ADDR_START, 5, 0, K_SECONDS(1)),
		  "Failed to send to LPN");

	/* Receive a segmented message from the LPN. */
	ASSERT_OK(bt_mesh_test_recv(15, cfg->addr, K_SECONDS(10)),
		  "Failed to receive from LPN");

	/* Send a segmented message to the friend. Should trigger a poll for the
	 * ack.
	 */
	ASSERT_OK(bt_mesh_test_send(LPN_ADDR_START, 15, 0, K_SECONDS(10)),
		  "Send to LPN failed");

	/* Receive an unsegmented message from the LPN, originally sent with
	 * friend credentials.
	 */
	ASSERT_OK(bt_mesh_test_recv(1, cfg->addr, K_SECONDS(10)),
		  "Failed to receive from LPN");

	PASS();
}

/** Without engaging in a friendship, send group and virtual addr messages to
 *  the LPN.
 */
static void test_other_group(void)
{
	uint16_t virtual_addr;

	bt_mesh_test_setup();

	ASSERT_OK(bt_mesh_va_add(test_va_uuid, &virtual_addr));

	/* Wait for LPN to send us a message after establishing the friendship */
	ASSERT_OK(bt_mesh_test_recv(5, cfg->addr, K_SECONDS(1)));

	/* Send a group message to the LPN */
	ASSERT_OK(bt_mesh_test_send(GROUP_ADDR, 5, 0, K_SECONDS(1)),
		  "Failed to send to LPN");
	/* Send a virtual message to the LPN */
	ASSERT_OK(bt_mesh_test_send(virtual_addr, 5, 0, K_SECONDS(1)),
		  "Failed to send to LPN");

	PASS();
}

#define TEST_CASE(role, name, description)                                     \
	{                                                                      \
		.test_id = "friendship_" #role "_" #name,                      \
		.test_descr = description,                                     \
		.test_post_init_f = test_##role##_init,                        \
		.test_tick_f = bt_mesh_test_timeout,                           \
		.test_main_f = test_##role##_##name,                           \
	}

static const struct bst_test_instance test_connect[] = {
	TEST_CASE(friend, est,       "Friend: establish friendship"),
	TEST_CASE(friend, est_multi, "Friend: establish multiple friendships"),
	TEST_CASE(friend, msg,       "Friend: message exchange"),
	TEST_CASE(friend, overflow,  "Friend: message queue overflow"),
	TEST_CASE(friend, group,     "Friend: send to group addrs"),

	TEST_CASE(lpn,    est,       "LPN: establish friendship"),
	TEST_CASE(lpn,    msg_frnd,  "LPN: message exchange with friend"),
	TEST_CASE(lpn,    msg_mesh,  "LPN: message exchange with mesh"),
	TEST_CASE(lpn,    re_est,    "LPN: re-establish friendship"),
	TEST_CASE(lpn,    poll,      "LPN: poll before timeout"),
	TEST_CASE(lpn,    overflow,  "LPN: message queue overflow"),
	TEST_CASE(lpn,    group,     "LPN: receive on group addrs"),
	TEST_CASE(lpn,    loopback,  "LPN: send to loopback addrs"),

	TEST_CASE(other,  msg,       "Other mesh device: message exchange"),
	TEST_CASE(other,  group,     "Other mesh device: send to group addrs"),
	BSTEST_END_MARKER
};

struct bst_test_list *test_friendship_install(struct bst_test_list *tests)
{
	tests = bst_add_tests(tests, test_connect);
	return tests;
}
