/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define TRANS_SEQ_AUTH_NVAL            0xffffffffffffffff

#define BT_MESH_SDU_UNSEG_MAX          11
#define BT_MESH_CTL_SEG_SDU_MAX	       8

#define BT_MESH_RX_CTL_MAX	       MAX((BT_MESH_RX_SEG_MAX *	\
					    BT_MESH_CTL_SEG_SDU_MAX),	\
					     BT_MESH_SDU_UNSEG_MAX)

#define TRANS_SEQ_ZERO_MASK            ((uint16_t)BIT_MASK(13))
#define TRANS_CTL_OP_MASK              ((uint8_t)BIT_MASK(7))
#define TRANS_CTL_OP(data)             ((data)[0] & TRANS_CTL_OP_MASK)
#define TRANS_CTL_HDR(op, seg)         ((op & TRANS_CTL_OP_MASK) | (seg << 7))

#define TRANS_CTL_OP_ACK               0x00
#define TRANS_CTL_OP_FRIEND_POLL       0x01
#define TRANS_CTL_OP_FRIEND_UPDATE     0x02
#define TRANS_CTL_OP_FRIEND_REQ        0x03
#define TRANS_CTL_OP_FRIEND_OFFER      0x04
#define TRANS_CTL_OP_FRIEND_CLEAR      0x05
#define TRANS_CTL_OP_FRIEND_CLEAR_CFM  0x06
#define TRANS_CTL_OP_FRIEND_SUB_ADD    0x07
#define TRANS_CTL_OP_FRIEND_SUB_REM    0x08
#define TRANS_CTL_OP_FRIEND_SUB_CFM    0x09
#define TRANS_CTL_OP_HEARTBEAT         0x0a

struct bt_mesh_ctl_friend_poll {
	uint8_t  fsn;
} __packed;

struct bt_mesh_ctl_friend_update {
	uint8_t  flags;
	uint32_t iv_index;
	uint8_t  md;
} __packed;

struct bt_mesh_ctl_friend_req {
	uint8_t  criteria;
	uint8_t  recv_delay;
	uint8_t  poll_to[3];
	uint16_t prev_addr;
	uint8_t  num_elem;
	uint16_t lpn_counter;
} __packed;

struct bt_mesh_ctl_friend_offer {
	uint8_t  recv_win;
	uint8_t  queue_size;
	uint8_t  sub_list_size;
	int8_t  rssi;
	uint16_t frnd_counter;
} __packed;

struct bt_mesh_ctl_friend_clear {
	uint16_t lpn_addr;
	uint16_t lpn_counter;
} __packed;

struct bt_mesh_ctl_friend_clear_confirm {
	uint16_t lpn_addr;
	uint16_t lpn_counter;
} __packed;

#define BT_MESH_FRIEND_SUB_MIN_LEN (1 + 2)
struct bt_mesh_ctl_friend_sub {
	uint8_t  xact;
	uint16_t addr_list[5];
} __packed;

struct bt_mesh_ctl_friend_sub_confirm {
	uint8_t xact;
} __packed;

bool bt_mesh_tx_in_progress(void);

void bt_mesh_rx_reset(void);

int bt_mesh_ctl_send(struct bt_mesh_net_tx *tx, uint8_t ctl_op, void *data,
		     size_t data_len, const struct bt_mesh_send_cb *cb,
		     void *cb_data);

/** @brief Send an access payload message.
 *
 *  @param tx      Network TX parameters. Only @c ctx, @c src and @c friend_cred
 *                 have to be filled.
 *  @param msg     Access payload to send.
 *  @param cb      Message callback.
 *  @param cb_data Message callback data.
 *
 *  @return 0 on success, or (negative) error code otherwise.
 */
int bt_mesh_trans_send(struct bt_mesh_net_tx *tx, struct net_buf_simple *msg,
		       const struct bt_mesh_send_cb *cb, void *cb_data);

int bt_mesh_trans_recv(struct net_buf_simple *buf, struct bt_mesh_net_rx *rx);

void bt_mesh_trans_init(void);
void bt_mesh_trans_reset(void);
