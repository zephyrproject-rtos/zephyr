/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/types.h>
#include <sys/util.h>

#include "util/mem.h"
#include "util/memq.h"
#include "util/mayfly.h"

#include "hal/ticker.h"

#include "ticker/ticker.h"

#include "pdu.h"

#include "lll.h"
#include "lll_vendor.h"
#include "lll_scan.h"
#include "lll_scan_aux.h"

#include "ull_scan_types.h"

#include "ull_internal.h"
#include "ull_scan_internal.h"

#define BT_DBG_ENABLED IS_ENABLED(CONFIG_BT_DEBUG_HCI_DRIVER)
#define LOG_MODULE_NAME bt_ctlr_ull_scan_aux
#include "common/log.h"
#include <soc.h>
#include "hal/debug.h"

static int init_reset(void);
static inline struct ll_scan_aux_set *aux_acquire(void);
static inline void aux_release(struct ll_scan_aux_set *aux);
static inline u8_t aux_handle_get(struct ll_scan_aux_set *aux);
static void rx_flush_release(struct ll_scan_aux_set *aux);
static void ticker_cb(u32_t ticks_at_expire, u32_t remainder, u16_t lazy,
		      void *param);
static void ticker_op_cb(u32_t status, void *param);
static void ticker_op_aux_failure(void *param);

static struct ll_scan_aux_set ll_scan_aux_pool[CONFIG_BT_CTLR_SCAN_AUX_SET];
static void *scan_aux_free;

int ull_scan_aux_init(void)
{
	int err;

	err = init_reset();
	if (err) {
		return err;
	}

	return 0;
}

int ull_scan_aux_reset(void)
{
	int err;

	err = init_reset();
	if (err) {
		return err;
	}

	return 0;
}

void ull_scan_aux_setup(memq_link_t *link, struct node_rx_hdr *rx, u8_t phy)
{
	struct ext_adv_aux_ptr *aux_ptr;
	struct pdu_adv_com_ext_adv *p;
	struct ll_scan_aux_set *aux;
	u32_t ticks_slot_overhead;
	u32_t window_widening_us;
	struct lll_scan_aux *lll;
	struct node_rx_ftr *ftr;
	u32_t ticks_slot_offset;
	struct ext_adv_hdr *h;
	u32_t ready_delay_us;
	struct pdu_adv *pdu;
	u32_t aux_offset_us;
	u32_t ticker_status;
	u8_t aux_handle;
	u8_t *ptr;

	ftr = &rx->rx_ftr;
	lll = ftr->param;
	aux = lll->hdr.parent;
	if (((u8_t *)aux < (u8_t *)ll_scan_aux_pool) ||
	    ((u8_t *)aux > ((u8_t *)ll_scan_aux_pool +
			    (sizeof(struct ll_scan_aux_set) *
			     (CONFIG_BT_CTLR_SCAN_AUX_SET - 1))))) {
		aux = NULL;
	}

	pdu = (void *)((struct node_rx_pdu *)rx)->pdu;
	if (pdu->type != PDU_ADV_TYPE_EXT_IND) {
		aux = NULL;
		goto ull_scan_aux_rx_flush;
	}

	p = (void *)&pdu->adv_ext_ind;
	if (!p->ext_hdr_len) {
		goto ull_scan_aux_rx_flush;
	}

	h = (void *)p->ext_hdr_adi_adv_data;
	if (!h->aux_ptr) {
		goto ull_scan_aux_rx_flush;
	}

	if (!aux) {
		aux = aux_acquire();
		if (!aux) {
			goto ull_scan_aux_rx_flush;
		}

		aux->rx_last = NULL;
		lll = &aux->lll;

		ull_hdr_init((void *)lll);
		lll_hdr_init(lll, aux);
	} else {
		LL_ASSERT(0);
	}

	ptr = (u8_t *)h + sizeof(*h);

	if (h->adv_addr) {
		ptr += BDADDR_SIZE;
	}

	if (h->tgt_addr) {
		ptr += BDADDR_SIZE;
	}

	if (h->adi) {
		struct ext_adv_adi *adi;

		adi = (void *)ptr;
		ptr += sizeof(*adi);
	}

	aux_ptr = (void *)ptr;
	if (!aux_ptr->offs) {
		goto ull_scan_aux_rx_flush;
	}

	/* Enqueue the rx in aux context */
	link->next = NULL;
	link->mem = rx;
	if (aux->rx_last) {
		aux->rx_last->next = link;
	} else {
		aux->rx_head = link;
	}
	aux->rx_last = link;

	lll->chan = aux_ptr->chan_idx;
	lll->phy = BIT(aux_ptr->phy);

	aux_offset_us = ftr->radio_end_us - PKT_AC_US(pdu->len, 0, phy);
	if (aux_ptr->offs_units) {
		lll->window_size_us = 300U;
	} else {
		lll->window_size_us = 30U;
	}
	aux_offset_us += (u32_t)aux_ptr->offs * lll->window_size_us;

	if (aux_ptr->ca) {
		window_widening_us = aux_offset_us / 2000U;
	} else {
		window_widening_us = aux_offset_us / 20000U;
	}

	lll->window_size_us += (EVENT_TICKER_RES_MARGIN_US +
				(EVENT_JITTER_US << 1) +
				window_widening_us);

	ready_delay_us = lll_radio_rx_ready_delay_get(lll->phy, 1);

	aux_offset_us -= EVENT_OVERHEAD_START_US;
	aux_offset_us -= EVENT_JITTER_US;
	aux_offset_us -= ready_delay_us;
	aux_offset_us -= window_widening_us;

	/* TODO: active_to_start feature port */
	aux->evt.ticks_active_to_start = 0;
	aux->evt.ticks_xtal_to_start =
		HAL_TICKER_US_TO_TICKS(EVENT_OVERHEAD_XTAL_US);
	aux->evt.ticks_preempt_to_start =
		HAL_TICKER_US_TO_TICKS(EVENT_OVERHEAD_PREEMPT_MIN_US);
	aux->evt.ticks_slot =
		HAL_TICKER_US_TO_TICKS(EVENT_OVERHEAD_START_US +
				       ready_delay_us +
				       PKT_AC_US(PDU_AC_EXT_PAYLOAD_SIZE_MAX,
						 0, lll->phy) +
				       EVENT_OVERHEAD_END_US);

	ticks_slot_offset = MAX(aux->evt.ticks_active_to_start,
				aux->evt.ticks_xtal_to_start);

	if (IS_ENABLED(CONFIG_BT_CTLR_LOW_LAT)) {
		ticks_slot_overhead = ticks_slot_offset;
	} else {
		ticks_slot_overhead = 0U;
	}

	/* TODO: unreserve the primary scan window ticks in ticker */

	aux_handle = aux_handle_get(aux);

	ticker_status = ticker_start(TICKER_INSTANCE_ID_CTLR,
				     TICKER_USER_ID_ULL_HIGH,
				     TICKER_ID_SCAN_AUX_BASE + aux_handle,
				     ftr->ticks_anchor - ticks_slot_offset,
				     HAL_TICKER_US_TO_TICKS(aux_offset_us),
				     TICKER_NULL_PERIOD,
				     TICKER_NULL_REMAINDER,
				     TICKER_NULL_LAZY,
				     (aux->evt.ticks_slot +
				      ticks_slot_overhead),
				     ticker_cb, aux, ticker_op_cb, aux);
	LL_ASSERT((ticker_status == TICKER_STATUS_SUCCESS) ||
		  (ticker_status == TICKER_STATUS_BUSY));

	return;

ull_scan_aux_rx_flush:
	if (aux) {
		rx_flush_release(aux);
	}

	ll_rx_put(link, rx);
	ll_rx_sched();
}

void ull_scan_aux_done(struct node_rx_event_done *done)
{
	struct lll_scan_aux *lll = (void *)HDR_ULL2LLL(done->param);
	struct ll_scan_aux_set *aux = (void *)HDR_LLL2EVT(lll);

	rx_flush_release(aux);
}

u8_t ull_scan_aux_lll_handle_get(struct lll_scan_aux *lll)
{
	return aux_handle_get((void *)lll->hdr.parent);
}

static int init_reset(void)
{
	/* Initialize adv aux pool. */
	mem_init(ll_scan_aux_pool, sizeof(struct ll_scan_aux_set),
		 sizeof(ll_scan_aux_pool) / sizeof(struct ll_scan_aux_set),
		 &scan_aux_free);

	return 0;
}

static inline struct ll_scan_aux_set *aux_acquire(void)
{
	return mem_acquire(&scan_aux_free);
}

static inline void aux_release(struct ll_scan_aux_set *aux)
{
	mem_release(aux, &scan_aux_free);
}

static inline u8_t aux_handle_get(struct ll_scan_aux_set *aux)
{
	return mem_index_get(aux, ll_scan_aux_pool,
			     sizeof(struct ll_scan_aux_set));
}

static void rx_flush_release(struct ll_scan_aux_set *aux)
{
	if (aux->rx_last) {
		memq_link_t *head = aux->rx_head;

		do {
			struct node_rx_hdr *rx = head->mem;
			memq_link_t *link = head;

			head = link->next;

			ll_rx_put(link, rx);
		} while (head);

		ll_rx_sched();
	}

	aux_release(aux);
}

static void ticker_cb(u32_t ticks_at_expire, u32_t remainder, u16_t lazy,
		      void *param)
{
	static memq_link_t link;
	static struct mayfly mfy = {0, 0, &link, NULL, lll_scan_aux_prepare};
	struct ll_scan_aux_set *aux = param;
	static struct lll_prepare_param p;
	u32_t ret;
	u8_t ref;

	DEBUG_RADIO_PREPARE_O(1);

	/* Increment prepare reference count */
	ref = ull_ref_inc(&aux->ull);
	LL_ASSERT(ref);

	/* Append timing parameters */
	p.ticks_at_expire = ticks_at_expire;
	p.remainder = 0; /* FIXME: remainder; */
	p.lazy = lazy;
	p.param = &aux->lll;
	mfy.param = &p;

	/* Kick LLL prepare */
	ret = mayfly_enqueue(TICKER_USER_ID_ULL_HIGH, TICKER_USER_ID_LLL,
			     0, &mfy);
	LL_ASSERT(!ret);

	DEBUG_RADIO_PREPARE_O(1);
}

static void ticker_op_cb(u32_t status, void *param)
{
	static memq_link_t link;
	static struct mayfly mfy = {0, 0, &link, NULL, ticker_op_aux_failure};
	u32_t ret;

	if (status == TICKER_STATUS_SUCCESS) {
		return;
	}

	mfy.param = param;

	ret = mayfly_enqueue(TICKER_USER_ID_ULL_LOW, TICKER_USER_ID_ULL_HIGH,
			     0, &mfy);
	LL_ASSERT(!ret);
}

static void ticker_op_aux_failure(void *param)
{
	rx_flush_release(param);
}
