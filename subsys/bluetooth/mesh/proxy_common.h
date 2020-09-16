/*  Bluetooth Mesh */

/*
 * Copyright (c) 2017 Intel Corporation
 * Copyright (c) Lingao Meng
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define PDU_TYPE(data)     (data[0] & BIT_MASK(6))

#define BT_MESH_PROXY_NET_PDU   0x00
#define BT_MESH_PROXY_BEACON    0x01
#define BT_MESH_PROXY_CONFIG    0x02
#define BT_MESH_PROXY_PROV      0x03

#define CFG_FILTER_SET     0x00
#define CFG_FILTER_ADD     0x01
#define CFG_FILTER_REMOVE  0x02
#define CFG_FILTER_STATUS  0x03

typedef int (*proxy_send_cb_t)(struct bt_conn *conn, const void *data,
			       uint16_t len);

typedef void (*proxy_recv_cb_t)(struct bt_conn *conn,
				struct bt_mesh_net_rx *rx, struct net_buf_simple *buf);

struct bt_mesh_proxy_object {
	struct bt_conn *conn;
	uint8_t msg_type;
	struct {
		proxy_send_cb_t send_cb;
		proxy_recv_cb_t recv_cb;
	} cb;
	struct k_delayed_work sar_timer;
	struct net_buf_simple buf;
};

int bt_mesh_proxy_common_recv(struct bt_mesh_proxy_object *object,
			      const void *buf, uint16_t len);
int bt_mesh_proxy_common_send(struct bt_mesh_proxy_object *object,
			      uint8_t type, struct net_buf_simple *msg);
void bt_mesh_proxy_common_init(struct bt_mesh_proxy_object *object,
			       uint8_t *buf, uint16_t len);
