/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/sys/iterable_sections.h>

#define BT_MESH_KEY_PRIMARY 0x0000

enum bt_mesh_key_evt {
	BT_MESH_KEY_ADDED,   /* New key added */
	BT_MESH_KEY_DELETED, /* Existing key deleted */
	BT_MESH_KEY_UPDATED, /* KR phase 1, second key added */
	BT_MESH_KEY_SWAPPED, /* KR phase 2, now sending on second key */
	BT_MESH_KEY_REVOKED, /* KR phase 3, old key removed */
};

/** Appkey callback. Instantiate with @ref BT_MESH_APP_KEY_CB */
struct bt_mesh_app_key_cb {
	void (*evt_handler)(uint16_t app_idx, uint16_t net_idx,
			    enum bt_mesh_key_evt evt);
};

/**
 *  @brief Register an AppKey event callback.
 *
 *  @param _handler Handler function, see @ref bt_mesh_app_key_cb::evt_handler.
 */
#define BT_MESH_APP_KEY_CB_DEFINE(_handler)                                    \
	static const STRUCT_SECTION_ITERABLE(bt_mesh_app_key_cb,               \
					     _CONCAT(bt_mesh_app_key_cb_,      \
						       _handler)) = {          \
		.evt_handler = (_handler),                                     \
	}

struct bt_mesh_net;

int bt_mesh_start(void);
void bt_mesh_reprovision(uint16_t addr);
void bt_mesh_dev_key_cand(const uint8_t *key);
void bt_mesh_dev_key_cand_remove(void);
void bt_mesh_dev_key_cand_activate(void);
