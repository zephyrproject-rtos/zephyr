/* l2cap_br.c - L2CAP BREDR internal interface
 *
 * This is the only interface between l2cap.c and l2cap_br.c.
 *
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* Initialize BR/EDR L2CAP signal layer */
void bt_l2cap_br_init(void);

/* Notify BR/EDR L2CAP channels about established new ACL connection */
void bt_l2cap_br_connected(struct bt_conn *conn);

/* Notify BR/EDR L2CAP channels about ACL disconnection*/
void bt_l2cap_br_disconnected(struct bt_conn *conn);

/* Lookup BR/EDR L2CAP channel by Receiver CID */
struct bt_l2cap_chan *bt_l2cap_br_lookup_rx_cid(struct bt_conn *conn,
						uint16_t cid);

/* Disconnects dynamic channel */
int bt_l2cap_br_chan_disconnect(struct bt_l2cap_chan *chan);

/* Make connection to peer psm server */
int bt_l2cap_br_chan_connect(struct bt_conn *conn, struct bt_l2cap_chan *chan,
			     uint16_t psm);

/* Send packet data to connected peer */
int bt_l2cap_br_chan_send(struct bt_l2cap_chan *chan, struct net_buf *buf);
int bt_l2cap_br_chan_send_cb(struct bt_l2cap_chan *chan, struct net_buf *buf, bt_conn_tx_cb_t cb,
			     void *user_data);

/*
 * Handle security level changed on link passing HCI status of performed
 * security procedure.
 */
void l2cap_br_encrypt_change(struct bt_conn *conn, uint8_t hci_status);

/* Handle received data */
void bt_l2cap_br_recv(struct bt_conn *conn, struct net_buf *buf);
