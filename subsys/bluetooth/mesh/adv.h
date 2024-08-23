/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/sys/util_macro.h>
#include <zephyr/sys_clock.h>

#ifndef ZEPHYR_SUBSYS_BLUETOOTH_MESH_ADV_H_
#define ZEPHYR_SUBSYS_BLUETOOTH_MESH_ADV_H_

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
	BT_MESH_ADV_TAG_LOCAL,
	BT_MESH_ADV_TAG_RELAY,
	BT_MESH_ADV_TAG_PROXY,
	BT_MESH_ADV_TAG_FRIEND,
	BT_MESH_ADV_TAG_PROV,
};

enum bt_mesh_adv_tag_bit {
	BT_MESH_ADV_TAG_BIT_LOCAL	= BIT(BT_MESH_ADV_TAG_LOCAL),
	BT_MESH_ADV_TAG_BIT_RELAY	= BIT(BT_MESH_ADV_TAG_RELAY),
	BT_MESH_ADV_TAG_BIT_PROXY	= BIT(BT_MESH_ADV_TAG_PROXY),
	BT_MESH_ADV_TAG_BIT_FRIEND	= BIT(BT_MESH_ADV_TAG_FRIEND),
	BT_MESH_ADV_TAG_BIT_PROV	= BIT(BT_MESH_ADV_TAG_PROV),
};

struct bt_mesh_adv_ctx {
	const struct bt_mesh_send_cb *cb;
	void *cb_data;

	uint8_t      type:2,
		  started:1,
		  busy:1,
		  tag:4;

	uint8_t      xmit;
};

struct bt_mesh_adv {
	sys_snode_t node;

	struct bt_mesh_adv_ctx ctx;

	struct net_buf_simple b;

	uint8_t __ref;

	uint8_t __bufs[BT_MESH_ADV_DATA_SIZE];
};

/* Lookup table for Advertising data types for bt_mesh_adv_type: */
extern const uint8_t bt_mesh_adv_type[BT_MESH_ADV_TYPES];

struct bt_mesh_adv *bt_mesh_adv_ref(struct bt_mesh_adv *adv);
void bt_mesh_adv_unref(struct bt_mesh_adv *adv);

/* xmit_count: Number of retransmissions, i.e. 0 == 1 transmission */
struct bt_mesh_adv *bt_mesh_adv_create(enum bt_mesh_adv_type type,
				       enum bt_mesh_adv_tag tag,
				       uint8_t xmit, k_timeout_t timeout);

void bt_mesh_adv_send(struct bt_mesh_adv *adv, const struct bt_mesh_send_cb *cb,
		      void *cb_data);
void bt_mesh_adv_send_end(int err, struct bt_mesh_adv_ctx const *ctx);

struct bt_mesh_adv *bt_mesh_adv_get(k_timeout_t timeout);

struct bt_mesh_adv *bt_mesh_adv_get_by_tag(enum bt_mesh_adv_tag_bit tags, k_timeout_t timeout);

void bt_mesh_adv_gatt_update(void);

void bt_mesh_adv_get_cancel(void);

void bt_mesh_adv_init(void);

int bt_mesh_scan_enable(void);

int bt_mesh_scan_disable(void);

int bt_mesh_adv_enable(void);

int bt_mesh_adv_disable(void);

void bt_mesh_adv_local_ready(void);

void bt_mesh_adv_relay_ready(void);

int bt_mesh_adv_terminate(struct bt_mesh_adv *adv);

void bt_mesh_adv_friend_ready(void);

int bt_mesh_adv_gatt_send(void);

int bt_mesh_adv_gatt_start(const struct bt_le_adv_param *param, int32_t duration,
			   const struct bt_data *ad, size_t ad_len,
			   const struct bt_data *sd, size_t sd_len);

void bt_mesh_adv_send_start(uint16_t duration, int err, struct bt_mesh_adv_ctx *ctx);

int bt_mesh_scan_active_set(bool active);

int bt_mesh_adv_bt_data_send(uint8_t num_events, uint16_t adv_interval,
			     const struct bt_data *ad, size_t ad_len);

#endif /* ZEPHYR_SUBSYS_BLUETOOTH_MESH_ADV_H_ */
