/** @file
 *  @brief Internal APIs for Bluetooth L2CAP handling.
 */

/*
 * Copyright (c) 2015 Intel Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <bluetooth/l2cap.h>

#define BT_L2CAP_CID_ATT		0x0004
#define BT_L2CAP_CID_LE_SIG		0x0005
#define BT_L2CAP_CID_SMP		0x0006

struct bt_l2cap_hdr {
	uint16_t len;
	uint16_t cid;
} __packed;

struct bt_l2cap_sig_hdr {
	uint8_t  code;
	uint8_t  ident;
	uint16_t len;
} __packed;

#define BT_L2CAP_REJ_NOT_UNDERSTOOD	0x0000
#define BT_L2CAP_REJ_MTU_EXCEEDED	0x0001
#define BT_L2CAP_REJ_INVALID_CID	0x0002

#define BT_L2CAP_CMD_REJECT		0x01
struct bt_l2cap_cmd_reject {
	uint16_t reason;
	uint8_t  data[0];
} __packed;

#define BT_L2CAP_DISCONN_REQ		0x06
struct bt_l2cap_disconn_req {
	uint16_t dcid;
	uint16_t scid;
} __packed;

#define BT_L2CAP_DISCONN_RSP		0x07
struct bt_l2cap_disconn_rsp {
	uint16_t dcid;
	uint16_t scid;
} __packed;

#define BT_L2CAP_CONN_PARAM_REQ		0x12
struct bt_l2cap_conn_param_req {
	uint16_t min_interval;
	uint16_t max_interval;
	uint16_t latency;
	uint16_t timeout;
} __packed;

#define BT_L2CAP_CONN_PARAM_ACCEPTED	0x0000
#define BT_L2CAP_CONN_PARAM_REJECTED	0x0001

#define BT_L2CAP_CONN_PARAM_RSP		0x13
struct bt_l2cap_conn_param_rsp {
	uint16_t result;
} __packed;

#define BT_L2CAP_LE_CONN_REQ		0x14
struct bt_l2cap_le_conn_req {
	uint16_t psm;
	uint16_t scid;
	uint16_t mtu;
	uint16_t mps;
	uint16_t credits;
} __packed;

#define BT_L2CAP_SUCCESS		0x0000
#define BT_L2CAP_ERR_PSM_NOT_SUPP	0x0002
#define BT_L2CAP_ERR_NO_RESOURCES	0x0004
#define BT_L2CAP_ERR_AUTHENTICATION	0x0005
#define BT_L2CAP_ERR_AUTHORIZATION	0x0006
#define BT_L2CAP_ERR_KEY_SIZE		0x0007
#define BT_L2CAP_ERR_ENCRYPTION		0x0008
#define BT_L2CAP_ERR_INVALID_SCID	0x0009
#define BT_L2CAP_ERR_SCID_IN_USE	0x000A

#define BT_L2CAP_LE_CONN_RSP		0x15
struct bt_l2cap_le_conn_rsp {
	uint16_t dcid;
	uint16_t mtu;
	uint16_t mps;
	uint16_t credits;
	uint16_t result;
};

struct bt_l2cap_fixed_chan {
	uint16_t		cid;

	int (*accept)(struct bt_conn *conn, struct bt_l2cap_chan **chan);

	struct bt_l2cap_fixed_chan	*_next;
};

/* Register a fixed L2CAP channel for L2CAP */
void bt_l2cap_fixed_chan_register(struct bt_l2cap_fixed_chan *chan);

/* Notify L2CAP channels of a new connection */
void bt_l2cap_connected(struct bt_conn *conn);

/* Notify L2CAP channels of a disconnect event */
void bt_l2cap_disconnected(struct bt_conn *conn);

/* Notify L2CAP channels of a change in encryption state */
void bt_l2cap_encrypt_change(struct bt_conn *conn);

/* Prepare an L2CAP PDU to be sent over a connection */
struct net_buf *bt_l2cap_create_pdu(struct bt_conn *conn);

/* Send L2CAP PDU over a connection */
void bt_l2cap_send(struct bt_conn *conn, uint16_t cid, struct net_buf *buf);

/* Receive a new L2CAP PDU from a connection */
void bt_l2cap_recv(struct bt_conn *conn, struct net_buf *buf);

/* Perform connection parameter update request */
int bt_l2cap_update_conn_param(struct bt_conn *conn);

/* Initialize L2CAP and supported channels */
int bt_l2cap_init(void);

/* Lookup channel by Transmission CID */
struct bt_l2cap_chan *bt_l2cap_lookup_tx_cid(struct bt_conn *conn,
					     uint16_t cid);

/* Lookup channel by Receiver CID */
struct bt_l2cap_chan *bt_l2cap_lookup_rx_cid(struct bt_conn *conn,
					     uint16_t cid);
