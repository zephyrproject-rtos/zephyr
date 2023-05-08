/*
 * Copyright (c) 2021 Demant
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define IS_PERIPHERAL(cig) \
	(IS_ENABLED(CONFIG_BT_CTLR_PERIPHERAL_ISO) && \
	 (cig->lll.role == BT_HCI_ROLE_PERIPHERAL))

#define IS_CENTRAL(cig) \
	(IS_ENABLED(CONFIG_BT_CTLR_CENTRAL_ISO) && \
	 (cig->lll.role == BT_HCI_ROLE_CENTRAL))

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

void ull_conn_iso_start(struct ll_conn *acl, uint16_t cis_handle,
			uint32_t ticks_at_expire, uint32_t remainder,
			uint16_t instant_latency);
void ull_conn_iso_done(struct node_rx_event_done *done);
void ull_conn_iso_cis_stop(struct ll_conn_iso_stream *cis,
			   ll_iso_stream_released_cb_t cis_released_cb,
			   uint8_t reason);

void ull_conn_iso_resume_ticker_start(struct lll_event *resume_event,
				      uint16_t cis_handle,
				      uint32_t ticks_anchor,
				      uint32_t resume_timeout);
void ull_conn_iso_transmit_test_cig_interval(uint16_t handle,
					     uint32_t ticks_at_expire);

void ull_conn_iso_ticker_cb(uint32_t ticks_at_expire, uint32_t ticks_drift,
			    uint32_t remainder, uint16_t lazy, uint8_t force,
			    void *param);
