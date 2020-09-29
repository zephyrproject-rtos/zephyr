/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef BT_GATT_OTS_L2CAP_H_
#define BT_GATT_OTS_L2CAP_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <zephyr/types.h>
#include <sys/slist.h>

#include <bluetooth/l2cap.h>

struct bt_gatt_ots_l2cap_tx {
	uint8_t *data;
	uint32_t len;
	uint32_t len_sent;
};

struct bt_gatt_ots_l2cap {
	sys_snode_t node;
	struct bt_l2cap_le_chan ot_chan;
	struct bt_gatt_ots_l2cap_tx tx;
	void (*tx_done)(struct bt_gatt_ots_l2cap *l2cap_ctx,
			struct bt_conn *conn);
};

bool bt_gatt_ots_l2cap_is_open(struct bt_gatt_ots_l2cap *l2cap_ctx,
				   struct bt_conn *conn);

int bt_gatt_ots_l2cap_send(struct bt_gatt_ots_l2cap *l2cap_ctx,
			       uint8_t *data,
			       uint32_t len);

int bt_gatt_ots_l2cap_register(struct bt_gatt_ots_l2cap *l2cap_ctx);

int bt_gatt_ots_l2cap_unregister(struct bt_gatt_ots_l2cap *l2cap_ctx);


#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* BT_GATT_OTS_L2CAP_H_ */
