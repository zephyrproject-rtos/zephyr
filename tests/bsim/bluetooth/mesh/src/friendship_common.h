/*
 * Copyright (c) 2023 Nordic Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>

enum bt_mesh_test_friendship_evt_flags {
	BT_MESH_TEST_LPN_ESTABLISHED,
	BT_MESH_TEST_LPN_TERMINATED,
	BT_MESH_TEST_LPN_POLLED,
	BT_MESH_TEST_FRIEND_ESTABLISHED,
	BT_MESH_TEST_FRIEND_TERMINATED,
	BT_MESH_TEST_FRIEND_POLLED,

	BT_MESH_TEST_FRIENDSHIP_FLAGS,
};

int bt_mesh_test_friendship_evt_wait(enum bt_mesh_test_friendship_evt_flags evt,
				     k_timeout_t timeout);

void bt_mesh_test_friendship_evt_clear(enum bt_mesh_test_friendship_evt_flags evt);

bool bt_mesh_test_friendship_state_check(enum bt_mesh_test_friendship_evt_flags evt);

void bt_mesh_test_friendship_init(int max_evt_count);

uint16_t bt_mesh_test_friendship_addr_get(void);
