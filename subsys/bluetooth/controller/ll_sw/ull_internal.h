/*
 * Copyright (c) 2017-2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

static inline u8_t ull_ref_inc(struct ull_hdr *hdr)
{
	return ++hdr->ref;
}

static inline u8_t ull_ref_dec(struct ull_hdr *hdr)
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
void *ll_pdu_rx_alloc_peek(u8_t count);
void *ll_pdu_rx_alloc(void);
void ll_rx_put(memq_link_t *link, void *rx);
void ll_rx_sched(void);
void ll_tx_ack_put(u16_t handle, struct node_tx *node_tx);
void ull_ticker_status_give(u32_t status, void *param);
u32_t ull_ticker_status_take(u32_t ret, u32_t volatile *ret_cb);
void *ull_disable_mark(void *param);
void *ull_disable_unmark(void *param);
void *ull_disable_mark_get(void);
void *ull_update_mark(void *param);
void *ull_update_unmark(void *param);
void *ull_update_mark_get(void);
int ull_disable(void *param);
