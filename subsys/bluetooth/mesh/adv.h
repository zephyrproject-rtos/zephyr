/*  Bluetooth Mesh */

/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* Maximum advertising data payload for a single data type */
#define BT_MESH_ADV_DATA_SIZE 29

/* The user data is a pointer (4 bytes) to struct bt_mesh_adv */
#define BT_MESH_ADV_USER_DATA_SIZE 4

#define BT_MESH_ADV(buf) (*(struct bt_mesh_adv **)net_buf_user_data(buf))

enum bt_mesh_adv_type {
	BT_MESH_ADV_PROV,
	BT_MESH_ADV_DATA,
	BT_MESH_ADV_BEACON,
	BT_MESH_ADV_URI,
};

typedef void (*bt_mesh_adv_func_t)(struct net_buf *buf, u16_t duration,
				   int err, void *user_data);

struct bt_mesh_adv {
	const struct bt_mesh_send_cb *cb;
	void *cb_data;

	u8_t      type:2,
		  busy:1;
	u8_t      xmit;

	union {
		/* Address, used e.g. for Friend Queue messages */
		u16_t addr;

		/* For transport layer segment sending */
		struct {
			u8_t attempts;
		} seg;
	};
};

typedef struct bt_mesh_adv *(*bt_mesh_adv_alloc_t)(int id);

/* xmit_count: Number of retransmissions, i.e. 0 == 1 transmission */
struct net_buf *bt_mesh_adv_create(enum bt_mesh_adv_type type, u8_t xmit,
				   s32_t timeout);

struct net_buf *bt_mesh_adv_create_from_pool(struct net_buf_pool *pool,
					     bt_mesh_adv_alloc_t get_id,
					     enum bt_mesh_adv_type type,
					     u8_t xmit, s32_t timeout);

void bt_mesh_adv_send(struct net_buf *buf, const struct bt_mesh_send_cb *cb,
		      void *cb_data);

void bt_mesh_adv_update(void);

void bt_mesh_adv_init(void);

int bt_mesh_scan_enable(void);

int bt_mesh_scan_disable(void);
