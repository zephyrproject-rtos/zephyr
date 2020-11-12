/*
 * Copyright (c) 2017-2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* Macro to convert time in us to connection interval units */
#define RADIO_CONN_EVENTS(x, y) ((uint16_t)(((x) + (y) - 1) / (y)))

/* Macro to convert time in us to periodic advertising interval units */
#define RADIO_SYNC_EVENTS(x, y) ((uint16_t)(((x) + (y) - 1) / (y)))

static inline uint8_t ull_ref_get(struct ull_hdr *hdr)
{
	return hdr->ref;
}

static inline uint8_t ull_ref_inc(struct ull_hdr *hdr)
{
	return ++hdr->ref;
}

static inline uint8_t ull_ref_dec(struct ull_hdr *hdr)
{
	return hdr->ref--;
}

static inline void ull_hdr_init(struct ull_hdr *hdr)
{
	hdr->disabled_cb = hdr->disabled_param = NULL;
}

void *ll_rx_link_alloc(void);
void ll_rx_link_release(void *link);
void *ll_rx_alloc(void);
void ll_rx_release(void *node_rx);
void *ll_pdu_rx_alloc_peek(uint8_t count);
void *ll_pdu_rx_alloc(void);
void ll_rx_put(memq_link_t *link, void *rx);
void ll_rx_sched(void);
void ull_ticker_status_give(uint32_t status, void *param);
uint32_t ull_ticker_status_take(uint32_t ret, uint32_t volatile *ret_cb);
void *ull_disable_mark(void *param);
void *ull_disable_unmark(void *param);
void *ull_disable_mark_get(void);
int ull_ticker_stop_with_mark(uint8_t ticker_handle, void *param,
			      void *lll_disable);
void *ull_update_mark(void *param);
void *ull_update_unmark(void *param);
void *ull_update_mark_get(void);
int ull_disable(void *param);
void ull_drift_ticks_get(struct node_rx_event_done *done,
			 uint32_t *ticks_drift_plus,
			 uint32_t *ticks_drift_minus);
