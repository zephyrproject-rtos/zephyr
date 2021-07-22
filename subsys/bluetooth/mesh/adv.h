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

#define BT_MESH_ADV_SCAN_UNIT(_ms) ((_ms) * 8 / 5)
#define BT_MESH_SCAN_INTERVAL_MS 30
#define BT_MESH_SCAN_WINDOW_MS   30

enum bt_mesh_adv_type {
	BT_MESH_ADV_PROV,
	BT_MESH_ADV_DATA,
	BT_MESH_ADV_BEACON,
	BT_MESH_ADV_URI,

	BT_MESH_ADV_TYPES,
};

typedef void (*bt_mesh_adv_func_t)(struct net_buf *buf, uint16_t duration,
				   int err, void *user_data);

struct bt_mesh_adv {
	const struct bt_mesh_send_cb *cb;
	void *cb_data;

	uint8_t      type:2,
		  started:1,
		  busy:1;

	uint8_t      xmit;
};

typedef struct bt_mesh_adv *(*bt_mesh_adv_alloc_t)(int id);

extern struct k_fifo bt_mesh_adv_queue;

/* Lookup table for Advertising data types for bt_mesh_adv_type: */
extern const uint8_t bt_mesh_adv_type[BT_MESH_ADV_TYPES];

/* xmit_count: Number of retransmissions, i.e. 0 == 1 transmission */
struct net_buf *bt_mesh_adv_create(enum bt_mesh_adv_type type, uint8_t xmit,
				   k_timeout_t timeout);

struct net_buf *bt_mesh_adv_create_from_pool(struct net_buf_pool *pool,
					     bt_mesh_adv_alloc_t get_id,
					     enum bt_mesh_adv_type type,
					     uint8_t xmit, k_timeout_t timeout);

void bt_mesh_adv_send(struct net_buf *buf, const struct bt_mesh_send_cb *cb,
		      void *cb_data);

void bt_mesh_adv_update(void);

void bt_mesh_adv_init(void);

int bt_mesh_scan_enable(void);

int bt_mesh_scan_disable(void);

int bt_mesh_adv_enable(void);

void bt_mesh_adv_buf_ready(void);

int bt_mesh_adv_start(const struct bt_le_adv_param *param, int32_t duration,
		      const struct bt_data *ad, size_t ad_len,
		      const struct bt_data *sd, size_t sd_len);

static inline void bt_mesh_adv_send_start(uint16_t duration, int err,
					  struct bt_mesh_adv *adv)
{
	if (!adv->started) {
		adv->started = 1;

		if (adv->cb && adv->cb->start) {
			adv->cb->start(duration, err, adv->cb_data);
		}

		if (err) {
			adv->cb = NULL;
		}
	}
}

static inline void bt_mesh_adv_send_end(
	int err, struct bt_mesh_adv const *adv)
{
	if (adv->started && adv->cb && adv->cb->end) {
		adv->cb->end(err, adv->cb_data);
	}
}
