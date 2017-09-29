/*  Bluetooth Mesh */

/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* Maximum advertising data payload for a single data type */
#define BT_MESH_ADV_DATA_SIZE 29

#define BT_MESH_ADV(buf) ((struct bt_mesh_adv *)net_buf_user_data(buf))

enum bt_mesh_adv_type {
	BT_MESH_ADV_PROV,
	BT_MESH_ADV_DATA,
	BT_MESH_ADV_BEACON,
};

typedef void (*bt_mesh_adv_func_t)(struct net_buf *buf, int err);

struct bt_mesh_adv {
	bt_mesh_adv_func_t sent;
	u8_t      type:2,
		  busy:1;
	u8_t      count:3,
		  adv_int:5;
	union {
		/* Generic User Data */
		u8_t user_data[2];

		/* For transport layer segment sending */
		struct {
			u8_t tx_id;
			u8_t attempts:6,
			     new_key:1,
			     friend_cred:1;
		} seg;
	};
};

/* xmit_count: Number of retransmissions, i.e. 0 == 1 transmission */
struct net_buf *bt_mesh_adv_create(enum bt_mesh_adv_type type, u8_t xmit_count,
				   u8_t xmit_int, s32_t timeout);

void bt_mesh_adv_send(struct net_buf *buf, bt_mesh_adv_func_t sent);

void bt_mesh_adv_update(void);

void bt_mesh_adv_init(void);

int bt_mesh_scan_enable(void);

int bt_mesh_scan_disable(void);
