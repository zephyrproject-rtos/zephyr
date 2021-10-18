/*
 * Copyright (c) 2021 Demant
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* Helper functions to initialize and reset ull_conn_iso module */
int ull_conn_iso_init(void);
int ull_conn_iso_reset(void);

struct ll_conn_iso_group *ll_conn_iso_group_acquire(void);
void ll_conn_iso_group_release(struct ll_conn_iso_group *cig);
uint16_t ll_conn_iso_group_handle_get(struct ll_conn_iso_group *cig);
struct ll_conn_iso_group *ll_conn_iso_group_get(uint16_t handle);
struct ll_conn_iso_group *ll_conn_iso_group_get_by_id(uint8_t id);

struct ll_conn_iso_stream *ll_conn_iso_stream_acquire(void);
void ll_conn_iso_stream_release(struct ll_conn_iso_stream *cis);
uint16_t ll_conn_iso_stream_handle_get(struct ll_conn_iso_stream *cis);
struct ll_conn_iso_stream *ll_conn_iso_stream_get(uint16_t handle);
struct ll_conn_iso_stream *ll_iso_stream_connected_get(uint16_t handle);
struct ll_conn_iso_stream *ll_conn_iso_stream_get_by_acl(struct ll_conn *conn,
							 uint16_t *cis_iter);
struct ll_conn_iso_stream *ll_conn_iso_stream_get_by_group(struct ll_conn_iso_group *cig,
							   uint16_t *handle_iter);

void ull_conn_iso_done(struct node_rx_event_done *done);
void ull_conn_iso_cis_established(struct ll_conn_iso_stream *cis);
void ull_conn_iso_cis_stop(struct ll_conn_iso_stream *cis,
			   ll_iso_stream_released_cb_t cis_released_cb,
			   uint8_t reason);

void ull_conn_iso_resume_ticker_start(struct lll_event *resume_event,
				      uint16_t cis_handle,
				      uint32_t ticks_anchor,
				      uint32_t resume_timeout);
