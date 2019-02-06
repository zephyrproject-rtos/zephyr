/*
 * Copyright (c) 2017-2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* Macro to convert time in us to connection interval units */
#define RADIO_CONN_EVENTS(x, y) ((u16_t)(((x) + (y) - 1) / (y)))

u8_t ull_conn_allowed_check(void *conn);
struct ll_conn *ll_conn_acquire(void);
void ll_conn_release(struct ll_conn *conn);
u16_t ll_conn_handle_get(struct ll_conn *conn);
struct ll_conn *ll_conn_get(u16_t handle);
struct ll_conn *ll_connected_get(u16_t handle);
int ull_conn_init(void);
int ull_conn_reset(void);
u8_t ull_conn_chan_map_cpy(u8_t *chan_map);
void ull_conn_chan_map_set(u8_t *chan_map);
u8_t ull_conn_default_phy_tx_get(void);
u8_t ull_conn_default_phy_rx_get(void);
void ull_conn_setup(memq_link_t *link, struct node_rx_hdr *rx);
int ull_conn_rx(memq_link_t *link, struct node_rx_pdu **rx);
int ull_conn_llcp(struct ll_conn *conn, u32_t ticks_at_expire, u16_t lazy);
void ull_conn_done(struct node_rx_event_done *done);
void ull_conn_tx_demux(u8_t count);
void ull_conn_tx_lll_enqueue(struct ll_conn *conn, u8_t count);
void ull_conn_link_tx_release(void *link);
void ull_conn_tx_ack(struct ll_conn *conn, memq_link_t *link,
		     struct node_tx *tx);
