/** @file aodv_control_messages.h
 *  @brief Rouitng Control Messages File
 *
 *  Bluetooth routing control messages following AODV protocol.
 *	The file contains RREQ, RREP, RWAIT and RERR data and functions.
 *  @bug No known bugs.
 */


/* FUNCTIONS PROTOTYPES */
/* RREQ FUNCTIONS */
int bt_mesh_trans_rreq_recv(struct bt_mesh_net_rx *rx, struct net_buf_simple *buf);
int bt_mesh_trans_ring_search(struct bt_mesh_net_tx *tx);
int bt_mesh_trans_ring_buf_alloc(struct bt_mesh_net_tx *tx, struct net_buf_simple *msg,
						const struct bt_mesh_send_cb *cb, void *cb_data);

/* RREP FUNCTIONS */
int bt_mesh_trans_rrep_recv(struct bt_mesh_net_rx *rx, struct net_buf_simple *buf);
void bt_mesh_trans_routing_init();

/* RWAIT FUNCTIONS */
void bt_mesh_trans_rwait_recv(struct bt_mesh_net_rx *rx, struct net_buf_simple *buf);

/*RERR FUNCTIONS*/
int bt_mesh_trans_rerr_recv(struct bt_mesh_net_rx *rx, struct net_buf_simple *buf);
void view_hello_msg_list();
void bt_mesh_trans_hello_msg_recv(u16_t src);
