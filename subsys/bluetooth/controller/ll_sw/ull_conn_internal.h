/*
 * Copyright (c) 2017-2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

struct ll_conn *ll_conn_acquire(void);
void ll_conn_release(struct ll_conn *conn);
uint16_t ll_conn_handle_get(struct ll_conn *conn);
struct ll_conn *ll_conn_get(uint16_t handle);
struct ll_conn *ll_connected_get(uint16_t handle);
void ll_tx_ack_put(uint16_t handle, struct node_tx *node_tx);
int ull_conn_init(void);
int ull_conn_reset(void);
void ull_conn_chan_map_set(uint8_t *chan_map);
uint16_t ull_conn_default_tx_octets_get(void);
uint16_t ull_conn_default_tx_time_get(void);
uint8_t ull_conn_default_phy_tx_get(void);
uint8_t ull_conn_default_phy_rx_get(void);
void ull_conn_setup(memq_link_t *link, struct node_rx_hdr *rx);
int ull_conn_rx(memq_link_t *link, struct node_rx_pdu **rx);
int ull_conn_llcp(struct ll_conn *conn, uint32_t ticks_at_expire, uint16_t lazy);
void ull_conn_done(struct node_rx_event_done *done);
void ull_conn_tx_demux(uint8_t count);
void ull_conn_tx_lll_enqueue(struct ll_conn *conn, uint8_t count);
void ull_conn_link_tx_release(void *link);
uint8_t ull_conn_ack_last_idx_get(void);
memq_link_t *ull_conn_ack_peek(uint8_t *ack_last, uint16_t *handle,
			       struct node_tx **tx);
memq_link_t *ull_conn_ack_by_last_peek(uint8_t last, uint16_t *handle,
				       struct node_tx **tx);
void *ull_conn_ack_dequeue(void);
void ull_conn_tx_ack(uint16_t handle, memq_link_t *link, struct node_tx *tx);
uint8_t ull_conn_llcp_req(void *conn);
void ull_conn_upd_curr_reset(void);
