/*
 * Copyright (c) 2023 Nordic Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "friendship_common.h"
#include "mesh_test.h"

#define LOG_MODULE_NAME friendship_common

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(LOG_MODULE_NAME);

static uint16_t bt_mesh_test_friend_lpn_addr;
static ATOMIC_DEFINE(bt_mesh_test_friendship_state, BT_MESH_TEST_FRIENDSHIP_FLAGS);
static struct k_sem bt_mesh_test_friendship_events[BT_MESH_TEST_FRIENDSHIP_FLAGS];

static void evt_signal(enum bt_mesh_test_friendship_evt_flags evt)
{
	atomic_set_bit(bt_mesh_test_friendship_state, evt);
	k_sem_give(&bt_mesh_test_friendship_events[evt]);
}

int bt_mesh_test_friendship_evt_wait(enum bt_mesh_test_friendship_evt_flags evt,
				     k_timeout_t timeout)
{
	return k_sem_take(&bt_mesh_test_friendship_events[evt], timeout);
}

void bt_mesh_test_friendship_evt_clear(enum bt_mesh_test_friendship_evt_flags evt)
{
	atomic_clear_bit(bt_mesh_test_friendship_state, evt);
	k_sem_reset(&bt_mesh_test_friendship_events[evt]);
}

bool bt_mesh_test_friendship_state_check(enum bt_mesh_test_friendship_evt_flags evt)
{
	return atomic_test_bit(bt_mesh_test_friendship_state, evt);
}

uint16_t bt_mesh_test_friendship_addr_get(void)
{
	return bt_mesh_test_friend_lpn_addr;
}

void bt_mesh_test_friendship_init(int max_evt_count)
{
	for (int i = 0; i < ARRAY_SIZE(bt_mesh_test_friendship_events); i++) {
		k_sem_init(&bt_mesh_test_friendship_events[i], 0, max_evt_count);
	}
}

static void friend_established(uint16_t net_idx, uint16_t lpn_addr,
			       uint8_t recv_delay, uint32_t polltimeout)
{
	LOG_INF("Friend: established with 0x%04x", lpn_addr);
	bt_mesh_test_friend_lpn_addr = lpn_addr;
	evt_signal(BT_MESH_TEST_FRIEND_ESTABLISHED);
}

static void friend_terminated(uint16_t net_idx, uint16_t lpn_addr)
{
	LOG_INF("Friend: terminated with 0x%04x", lpn_addr);
	evt_signal(BT_MESH_TEST_FRIEND_TERMINATED);
}

static void friend_polled(uint16_t net_idx, uint16_t lpn_addr)
{
	LOG_INF("Friend: Poll from 0x%04x", lpn_addr);
	evt_signal(BT_MESH_TEST_FRIEND_POLLED);
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
	evt_signal(BT_MESH_TEST_LPN_ESTABLISHED);
}

static void lpn_terminated(uint16_t net_idx, uint16_t friend_addr)
{
	LOG_INF("LPN: terminated with 0x%04x", friend_addr);
	evt_signal(BT_MESH_TEST_LPN_TERMINATED);
}

static void lpn_polled(uint16_t net_idx, uint16_t friend_addr, bool retry)
{
	LOG_INF("LPN: Polling 0x%04x (%s)", friend_addr,
		retry ? "retry" : "initial");
	evt_signal(BT_MESH_TEST_LPN_POLLED);
}

BT_MESH_LPN_CB_DEFINE(lpn) = {
	.established = lpn_established,
	.polled = lpn_polled,
	.terminated = lpn_terminated,
};
