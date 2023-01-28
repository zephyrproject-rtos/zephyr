/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* Maximum advertising data payload for a single data type */
#define BT_MESH_ADV_DATA_SIZE 29

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

enum bt_mesh_adv_tag {
	BT_MESH_LOCAL_ADV = BIT(0),
	BT_MESH_RELAY_ADV = BIT(1),
	BT_MESH_PROXY_ADV = BIT(2),
	BT_MESH_FRIEND_ADV = BIT(3),
};

struct bt_mesh_adv_meta {
	const struct bt_mesh_send_cb *cb;
	void *cb_data;

	uint8_t      type:2,
		  started:1,
		  busy:1,
		  tag:4;

	uint8_t      xmit;
	uint8_t      prio;
};

struct bt_mesh_adv {
	sys_snode_t node;

	struct net_buf_simple b;
	uint8_t _bufs[BT_MESH_ADV_DATA_SIZE];

	uint8_t ref;

	struct bt_mesh_adv_meta meta;
};

/* Lookup table for Advertising data types for bt_mesh_adv_type: */
extern const uint8_t bt_mesh_adv_type[BT_MESH_ADV_TYPES];

struct bt_mesh_adv *bt_mesh_adv_ref(struct bt_mesh_adv *adv);
void bt_mesh_adv_unref(struct bt_mesh_adv *adv);

/* xmit_count: Number of retransmissions, i.e. 0 == 1 transmission */
struct bt_mesh_adv *bt_mesh_adv_main_create(enum bt_mesh_adv_type type,
					    uint8_t xmit, k_timeout_t timeout);

struct bt_mesh_adv *bt_mesh_adv_relay_create(uint8_t prio, uint8_t xmit);
struct bt_mesh_adv *bt_mesh_adv_frnd_create(uint8_t xmit, k_timeout_t timeout);

void bt_mesh_adv_send(struct bt_mesh_adv *adv, const struct bt_mesh_send_cb *cb,
		      void *cb_data);

struct bt_mesh_adv *bt_mesh_adv_get(k_timeout_t timeout);

struct bt_mesh_adv *bt_mesh_adv_get_by_tag(uint8_t tag, k_timeout_t timeout);

void bt_mesh_adv_gatt_update(void);

void bt_mesh_adv_get_cancel(void);

void bt_mesh_adv_init(void);

int bt_mesh_scan_enable(void);

int bt_mesh_scan_disable(void);

int bt_mesh_adv_enable(void);

void bt_mesh_adv_local_ready(void);

void bt_mesh_adv_relay_ready(void);

void bt_mesh_adv_friend_ready(void);

int bt_mesh_adv_gatt_send(void);

int bt_mesh_adv_gatt_start(const struct bt_le_adv_param *param, int32_t duration,
			   const struct bt_data *ad, size_t ad_len,
			   const struct bt_data *sd, size_t sd_len);

static inline void bt_mesh_adv_send_start(uint16_t duration, int err,
					  struct bt_mesh_adv_meta *meta)
{
	if (!meta->started) {
		meta->started = 1;

		if (meta->cb && meta->cb->start) {
			meta->cb->start(duration, err, meta->cb_data);
		}

		if (err) {
			meta->cb = NULL;
		}
	}
}

static inline void bt_mesh_adv_send_end(
	int err, struct bt_mesh_adv_meta const *meta)
{
	if (meta->started && meta->cb && meta->cb->end) {
		meta->cb->end(err, meta->cb_data);
	}
}
