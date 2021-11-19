/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <sys/byteorder.h>
#include <sys/util.h>

#include "util/mem.h"
#include "util/memq.h"
#include "util/mayfly.h"
#include "util/util.h"

#include "hal/ticker.h"
#include "hal/ccm.h"

#include "ticker/ticker.h"

#include "pdu.h"

#include "lll.h"
#include "lll/lll_vendor.h"
#include "lll_scan.h"
#include "lll_scan_aux.h"
#include "lll/lll_df_types.h"
#include "lll_conn.h"
#include "lll_sync.h"
#include "lll_sync_iso.h"

#include "ull_scan_types.h"
#include "ull_sync_types.h"

#include "ull_internal.h"
#include "ull_scan_internal.h"
#include "ull_sync_internal.h"
#include "ull_sync_iso_internal.h"

#define BT_DBG_ENABLED IS_ENABLED(CONFIG_BT_DEBUG_HCI_DRIVER)
#define LOG_MODULE_NAME bt_ctlr_ull_scan_aux
#include "common/log.h"
#include <soc.h>
#include "hal/debug.h"

static int init_reset(void);
static inline struct ll_scan_aux_set *aux_acquire(void);
static inline void aux_release(struct ll_scan_aux_set *aux);
static inline uint8_t aux_handle_get(struct ll_scan_aux_set *aux);
static inline struct ll_sync_set *sync_create_get(struct ll_scan_set *scan);
static inline struct ll_sync_iso_set *
	sync_iso_create_get(struct ll_sync_set *sync);
static void last_disabled_cb(void *param);
static void done_disabled_cb(void *param);
static void flush(struct ll_scan_aux_set *aux);
static void ticker_cb(uint32_t ticks_at_expire, uint32_t ticks_drift,
		      uint32_t remainder, uint16_t lazy, uint8_t force,
		      void *param);
static void ticker_op_cb(uint32_t status, void *param);
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

void ull_scan_aux_setup(memq_link_t *link, struct node_rx_hdr *rx)
{
	struct ll_sync_iso_set *sync_iso;
	struct pdu_adv_aux_ptr *aux_ptr;
	struct pdu_adv_com_ext_adv *p;
	uint32_t ticks_slot_overhead;
	struct lll_scan_aux *lll_aux;
	struct ll_scan_aux_set *aux;
	uint32_t window_widening_us;
	uint32_t ticks_slot_offset;
	uint32_t ticks_aux_offset;
	struct pdu_adv_ext_hdr *h;
	struct lll_sync *sync_lll;
	struct ll_scan_set *scan;
	struct ll_sync_set *sync;
	struct pdu_adv_adi *adi;
	struct node_rx_ftr *ftr;
	uint32_t ready_delay_us;
	uint32_t aux_offset_us;
	uint32_t ticker_status;
	struct lll_scan *lll;
	struct pdu_adv *pdu;
	uint8_t aux_handle;
	bool is_scan_req;
	uint8_t acad_len;
	uint8_t hdr_len;
	uint8_t *ptr;
	uint8_t phy;

	is_scan_req = false;
	ftr = &rx->rx_ftr;

	sync_lll = NULL;

	switch (rx->type) {
	case NODE_RX_TYPE_EXT_1M_REPORT:
		lll_aux = NULL;
		aux = NULL;
		sync_iso = NULL;
		lll = ftr->param;
		scan = HDR_LLL2ULL(lll);
		sync = sync_create_get(scan);
		phy = BT_HCI_LE_EXT_SCAN_PHY_1M;
		break;
	case NODE_RX_TYPE_EXT_CODED_REPORT:
		lll_aux = NULL;
		aux = NULL;
		sync_iso = NULL;
		lll = ftr->param;
		scan = HDR_LLL2ULL(lll);
		sync = sync_create_get(scan);
		phy = BT_HCI_LE_EXT_SCAN_PHY_CODED;
		break;
	case NODE_RX_TYPE_EXT_AUX_REPORT:
		sync_iso = NULL;
		if (ull_scan_aux_is_valid_get(HDR_LLL2ULL(ftr->param))) {
			/* Node has valid aux context so its scan was scheduled
			 * from ULL.
			 */
			lll_aux = ftr->param;
			aux = HDR_LLL2ULL(lll_aux);

			/* aux parent will be NULL for periodic sync */
			lll = aux->parent;
		} else if (ull_scan_is_valid_get(HDR_LLL2ULL(ftr->param))) {
			/* Node that does not have valid aux context but has
			 * valid scan set was scheduled from LLL. We can
			 * retrieve aux context from lll_scan as it was stored
			 * there when superior PDU was handled.
			 */
			lll = ftr->param;

			lll_aux = lll->lll_aux;
			LL_ASSERT(lll_aux);

			aux = HDR_LLL2ULL(lll_aux);
			LL_ASSERT(lll == aux->parent);
		} else {
			/* If none of the above, node is part of sync scanning
			 */
			lll = NULL;
			sync_lll = ftr->param;

			lll_aux = sync_lll->lll_aux;
			aux = HDR_LLL2ULL(lll_aux);
		}

		if (lll) {
			scan = HDR_LLL2ULL(lll);
			sync = (void *)scan;
			scan = ull_scan_is_valid_get(scan);
			if (scan) {
				sync = NULL;
			}
		} else {
			scan = NULL;
			sync = HDR_LLL2ULL(sync_lll);
		}

		phy = lll_aux->phy;
		if (scan) {
			/* Here we are scanner context */
			sync = sync_create_get(scan);

			/* Generate report based on PHY scanned */
			switch (phy) {
			case PHY_1M:
				rx->type = NODE_RX_TYPE_EXT_1M_REPORT;
				break;
			case PHY_2M:
				rx->type = NODE_RX_TYPE_EXT_2M_REPORT;
				break;
			case PHY_CODED:
				rx->type = NODE_RX_TYPE_EXT_CODED_REPORT;
				break;
			default:
				LL_ASSERT(0);
				return;
			}

			/* Backup scan requested flag as it is in union with
			 * `extra` struct member which will be set to NULL
			 * in subsequent code.
			 */
			is_scan_req = !!ftr->scan_req;

#if defined(CONFIG_BT_CTLR_SYNC_PERIODIC)
		} else {
			/* Here we are periodic sync context */
			rx->type = NODE_RX_TYPE_SYNC_REPORT;
			rx->handle = ull_sync_handle_get(sync);

			sync_lll = &sync->lll;

			/* Check if we need to create BIG sync */
			sync_iso = sync_iso_create_get(sync);

			/* lll_aux and aux are auxiliary channel context,
			 * reuse the existing aux context to scan the chain.
			 * hence lll_aux and aux are not released or set to NULL.
			 */
			sync = NULL;
		}
		break;

	case NODE_RX_TYPE_SYNC_REPORT:
		{
			struct ll_sync_set *ull_sync;

			/* set the sync handle corresponding to the LLL context
			 * passed in the node rx footer field.
			 */
			sync_lll = ftr->param;
			ull_sync = HDR_LLL2ULL(sync_lll);
			rx->handle = ull_sync_handle_get(ull_sync);

			/* Check if we need to create BIG sync */
			sync_iso = sync_iso_create_get(ull_sync);

			/* FIXME: we will need lll_scan if chain was scheduled
			 *        from LLL; should we store lll_scan_set in
			 *        sync_lll instead?
			 */
			lll = NULL;
			lll_aux = NULL;
			aux = NULL;
			scan = NULL;
			sync = NULL;
			phy =  sync_lll->phy;
#endif /* CONFIG_BT_CTLR_SYNC_PERIODIC */

		}
		break;
	default:
		LL_ASSERT(0);
		return;
	}

	rx->link = link;
	ftr->extra = NULL;

	ftr->aux_w4next = 0;

	pdu = (void *)((struct node_rx_pdu *)rx)->pdu;
	p = (void *)&pdu->adv_ext_ind;
	if (!p->ext_hdr_len) {
		goto ull_scan_aux_rx_flush;
	}

	h = (void *)p->ext_hdr_adv_data;

	/* Regard PDU as invalid if a RFU field is set, we do not know the
	 * size of this future field, hence will cause incorrect calculation of
	 * offset to ACAD field.
	 */
	if (h->rfu) {
		goto ull_scan_aux_rx_flush;
	}

	ptr = h->data;

	if (h->adv_addr) {
#if defined(CONFIG_BT_CTLR_SYNC_PERIODIC)
		/* Check if Periodic Advertising Synchronization to be created
		 */
		if (sync && (scan->per_scan.state != LL_SYNC_STATE_CREATED)) {
			/* Check address and update internal state */
#if defined(CONFIG_BT_CTLR_PRIVACY)
			ull_sync_setup_addr_check(scan, pdu->tx_addr, ptr,
						  ftr->rl_idx);
#else /* !CONFIG_BT_CTLR_PRIVACY */
			ull_sync_setup_addr_check(scan, pdu->tx_addr, ptr, 0U);
#endif /* !CONFIG_BT_CTLR_PRIVACY */

		}
#endif /* CONFIG_BT_CTLR_SYNC_PERIODIC */

		ptr += BDADDR_SIZE;
	}

	if (h->tgt_addr) {
		ptr += BDADDR_SIZE;
	}

	if (h->cte_info) {
		ptr += sizeof(struct pdu_cte_info);
	}

	adi = NULL;
	if (h->adi) {
		adi = (void *)ptr;
		ptr += sizeof(*adi);
	}

	aux_ptr = NULL;
	if (h->aux_ptr) {
		aux_ptr = (void *)ptr;
		ptr += sizeof(*aux_ptr);
	}

	if (h->sync_info) {
		struct pdu_adv_sync_info *si;

		si = (void *)ptr;
		ptr += sizeof(*si);

#if defined(CONFIG_BT_CTLR_SYNC_PERIODIC)
		/* Check if Periodic Advertising Synchronization to be created.
		 * Setup synchronization if address and SID match in the
		 * Periodic Advertiser List or with the explicitly supplied.
		 */
		if (sync && adi && ull_sync_setup_sid_match(scan, adi->sid)) {
			ull_sync_setup(scan, aux, rx, si);
		}
#endif /* CONFIG_BT_CTLR_SYNC_PERIODIC */
	}

	if (h->tx_pwr) {
		ptr++;
	}

	/* Calculate ACAD Len */
	hdr_len = ptr - (uint8_t *)p;
	if (hdr_len <= (p->ext_hdr_len + offsetof(struct pdu_adv_com_ext_adv,
						  ext_hdr_adv_data))) {
		acad_len = p->ext_hdr_len +
			   offsetof(struct pdu_adv_com_ext_adv,
				    ext_hdr_adv_data) -
			   hdr_len;
	} else {
		acad_len = 0U;
	}

	/* Periodic Advertising Channel Map Indication and/or Broadcast ISO
	 * synchronization
	 */
	if (IS_ENABLED(CONFIG_BT_CTLR_SYNC_PERIODIC) &&
	    (rx->type == NODE_RX_TYPE_SYNC_REPORT) &&
	    acad_len) {
		/* Periodic Advertising Channel Map Indication */
		ull_sync_chm_update(rx->handle, ptr, acad_len);

		/* Broadcast ISO synchronize */
		if (IS_ENABLED(CONFIG_BT_CTLR_SYNC_ISO) && sync_iso) {
			ull_sync_iso_setup(sync_iso, rx, ptr, acad_len);
		}
	}

	/* Do not ULL schedule auxiliary PDU reception if no aux pointer
	 * or aux pointer is zero or scannable advertising has erroneous aux
	 * pointer being present or PHY in the aux pointer is invalid.
	 */
	if (!aux_ptr || !aux_ptr->offs || is_scan_req ||
	    (aux_ptr->phy > EXT_ADV_AUX_PHY_LE_CODED)) {
		if (is_scan_req) {
			LL_ASSERT(aux && aux->rx_last);

			aux->rx_last->rx_ftr.extra = rx;
			aux->rx_last = rx;

			return;
		}

		goto ull_scan_aux_rx_flush;
	}

	if (!aux) {
		aux = aux_acquire();
		if (!aux) {
			goto ull_scan_aux_rx_flush;
		}

		aux->rx_head = aux->rx_last = NULL;
		lll_aux = &aux->lll;
		lll_aux->is_chain_sched = 0U;

		ull_hdr_init(&aux->ull);
		lll_hdr_init(lll_aux, aux);

		aux->parent = lll ? (void *)lll : (void *)sync_lll;
	}

	/* In sync context we can dispatch rx immediately, in scan context we
	 * enqueue rx in aux context and will flush them after scan is complete.
	 */
	if (0) {
#if defined(CONFIG_BT_CTLR_SYNC_PERIODIC)
	} else if (sync_lll) {
		ll_rx_put(link, rx);
		ll_rx_sched();
#endif /* CONFIG_BT_CTLR_SYNC_PERIODIC */
	} else {
		if (aux->rx_last) {
			aux->rx_last->rx_ftr.extra = rx;
		} else {
			aux->rx_head = rx;
		}
		aux->rx_last = rx;
	}

	lll_aux->chan = aux_ptr->chan_idx;
	lll_aux->phy = BIT(aux_ptr->phy);

	ftr->aux_w4next = 1;

	/* See if this was already scheduled from LLL. If so, store aux context
	 * in global scan struct so we can pick it when scanned node is received
	 * with a valid context.
	 */
	if (ftr->aux_lll_sched) {
		if (0) {
#if defined(CONFIG_BT_CTLR_SYNC_PERIODIC)
		} else if (sync_lll) {
			sync_lll->lll_aux = lll_aux;
#endif /* CONFIG_BT_CTLR_SYNC_PERIODIC */
		} else {
			lll->lll_aux = lll_aux;
		}

		/* Reset auxiliary channel PDU scan state which otherwise is
		 * done in the prepare_cb when ULL scheduling is used.
		 */
		lll_aux->state = 0U;

		return;
	}

	/* Switching to ULL scheduling to receive auxiliary PDUs */
	if (lll) {
		lll->lll_aux = NULL;
	} else {
		LL_ASSERT(sync_lll);
		/* XXX: keep lll_aux for now since node scheduled from ULL has
		 *      sync_lll as ftr->param and we still need to restore
		 *      lll_aux somehow.
		 */
		/* sync_lll->lll_aux = NULL; */
	}

	/* Determine the window size */
	if (aux_ptr->offs_units) {
		lll_aux->window_size_us = OFFS_UNIT_300_US;
	} else {
		lll_aux->window_size_us = OFFS_UNIT_30_US;
	}

	aux_offset_us = (uint32_t)aux_ptr->offs * lll_aux->window_size_us;

	/* CA field contains the clock accuracy of the advertiser;
	 * 0 - 51 ppm to 500 ppm
	 * 1 - 0 ppm to 50 ppm
	 */
	if (aux_ptr->ca) {
		window_widening_us = SCA_DRIFT_50_PPM_US(aux_offset_us);
	} else {
		window_widening_us = SCA_DRIFT_500_PPM_US(aux_offset_us);
	}

	lll_aux->window_size_us += (EVENT_TICKER_RES_MARGIN_US +
				    ((EVENT_JITTER_US + window_widening_us) << 1));

	ready_delay_us = lll_radio_rx_ready_delay_get(lll_aux->phy, 1);

	/* Calculate the aux offset from start of the scan window */
	aux_offset_us += ftr->radio_end_us;
	aux_offset_us -= PDU_AC_US(pdu->len, phy, ftr->phy_flags);
	aux_offset_us -= EVENT_JITTER_US;
	aux_offset_us -= ready_delay_us;
	aux_offset_us -= window_widening_us;

	/* TODO: active_to_start feature port */
	aux->ull.ticks_active_to_start = 0;
	aux->ull.ticks_prepare_to_start =
		HAL_TICKER_US_TO_TICKS(EVENT_OVERHEAD_XTAL_US);
	aux->ull.ticks_preempt_to_start =
		HAL_TICKER_US_TO_TICKS(EVENT_OVERHEAD_PREEMPT_MIN_US);
	aux->ull.ticks_slot =
		HAL_TICKER_US_TO_TICKS(EVENT_OVERHEAD_START_US +
				       ready_delay_us +
				       PDU_AC_MAX_US(PDU_AC_EXT_PAYLOAD_SIZE_MAX,
						     lll_aux->phy) +
				       EVENT_OVERHEAD_END_US);

	ticks_slot_offset = MAX(aux->ull.ticks_active_to_start,
				aux->ull.ticks_prepare_to_start);
	if (IS_ENABLED(CONFIG_BT_CTLR_LOW_LAT)) {
		ticks_slot_overhead = ticks_slot_offset;
	} else {
		ticks_slot_overhead = 0U;
	}
	ticks_slot_offset += HAL_TICKER_US_TO_TICKS(EVENT_OVERHEAD_START_US);

	ticks_aux_offset = HAL_TICKER_US_TO_TICKS(aux_offset_us);

	/* Yield the primary scan window ticks in ticker */
	if (scan) {
		uint8_t handle;

		handle = ull_scan_handle_get(scan);

		ticker_status = ticker_yield_abs(TICKER_INSTANCE_ID_CTLR,
						 TICKER_USER_ID_ULL_HIGH,
						 (TICKER_ID_SCAN_BASE + handle),
						 (ftr->ticks_anchor +
						  ticks_aux_offset -
						  ticks_slot_offset),
						 NULL, NULL);
		LL_ASSERT((ticker_status == TICKER_STATUS_SUCCESS) ||
			  (ticker_status == TICKER_STATUS_BUSY));
	}

	aux_handle = aux_handle_get(aux);

	ticker_status = ticker_start(TICKER_INSTANCE_ID_CTLR,
				     TICKER_USER_ID_ULL_HIGH,
				     TICKER_ID_SCAN_AUX_BASE + aux_handle,
				     ftr->ticks_anchor - ticks_slot_offset,
				     ticks_aux_offset,
				     TICKER_NULL_PERIOD,
				     TICKER_NULL_REMAINDER,
				     TICKER_NULL_LAZY,
				     (aux->ull.ticks_slot +
				      ticks_slot_overhead),
				     ticker_cb, aux, ticker_op_cb, aux);
	LL_ASSERT((ticker_status == TICKER_STATUS_SUCCESS) ||
		  (ticker_status == TICKER_STATUS_BUSY));

	return;

ull_scan_aux_rx_flush:
#if defined(CONFIG_BT_CTLR_SYNC_PERIODIC)
	if (sync && (scan->per_scan.state != LL_SYNC_STATE_CREATED)) {
		scan->per_scan.state = LL_SYNC_STATE_IDLE;
	}
#endif /* CONFIG_BT_CTLR_SYNC_PERIODIC */

	if (aux) {
		struct ull_hdr *hdr;

		/* Enqueue last rx in aux context if possible, otherwise send
		 * immediately since we are in sync context.
		 */
		if (aux->rx_last) {
			aux->rx_last->rx_ftr.extra = rx;
		} else {
			LL_ASSERT(sync_lll);
			ll_rx_put(link, rx);
			ll_rx_sched();
		}

		/* ref == 0
		 * All PDUs were scheduled from LLL and there is no pending done
		 * event, we can flush here.
		 *
		 * ref == 1
		 * There is pending done event so we need to flush from disabled
		 * callback. Flushing here would release aux context and thus
		 * ull_hdr before done event was processed.
		 */
		hdr = &aux->ull;
		LL_ASSERT(ull_ref_get(hdr) < 2);
		if (ull_ref_get(hdr) == 0) {
			flush(aux);
		} else {
			LL_ASSERT(!hdr->disabled_cb);

			hdr->disabled_param = aux;
			hdr->disabled_cb = last_disabled_cb;
		}

		return;
	}

	ll_rx_put(link, rx);
	ll_rx_sched();
}

void ull_scan_aux_done(struct node_rx_event_done *done)
{
	struct ll_scan_aux_set *aux;
	struct ull_hdr *hdr;

	/* Get reference to ULL context */
	aux = CONTAINER_OF(done->param, struct ll_scan_aux_set, ull);

	if (0) {
#if defined(CONFIG_BT_CTLR_SYNC_PERIODIC)
	} else if (!ull_scan_aux_is_valid_get(aux)) {
		struct ll_sync_set *sync;

		sync = CONTAINER_OF(done->param, struct ll_sync_set, ull);
		LL_ASSERT(ull_sync_is_valid_get(sync));
		hdr = &sync->ull;

		if (!sync->lll.lll_aux) {
			return;
		}

		aux = HDR_LLL2ULL(sync->lll.lll_aux);
#endif /* CONFIG_BT_CTLR_SYNC_PERIODIC */
	} else {
		/* Setup the disabled callback to flush the auxiliary PDUs */
		hdr = &aux->ull;
	}

	LL_ASSERT(!hdr->disabled_cb);
	hdr->disabled_param = aux;
	hdr->disabled_cb = done_disabled_cb;
}

uint8_t ull_scan_aux_lll_handle_get(struct lll_scan_aux *lll)
{
	return aux_handle_get((void *)lll->hdr.parent);
}

void *ull_scan_aux_lll_parent_get(struct lll_scan_aux *lll,
				  uint8_t *is_lll_scan)
{
	struct ll_scan_aux_set *aux_set;
	struct ll_scan_set *scan_set;

	aux_set = HDR_LLL2ULL(lll);
	scan_set = HDR_LLL2ULL(aux_set->parent);

	if (is_lll_scan) {
		*is_lll_scan = !!ull_scan_is_valid_get(scan_set);
	}

	return aux_set->parent;
}

struct ll_scan_aux_set *ull_scan_aux_is_valid_get(struct ll_scan_aux_set *aux)
{
	if (((uint8_t *)aux < (uint8_t *)ll_scan_aux_pool) ||
	    ((uint8_t *)aux > ((uint8_t *)ll_scan_aux_pool +
			       (sizeof(struct ll_scan_aux_set) *
				(CONFIG_BT_CTLR_SCAN_AUX_SET - 1))))) {
		return NULL;
	}

	return aux;
}

void ull_scan_aux_release(memq_link_t *link, struct node_rx_hdr *rx)
{
	struct lll_scan_aux *lll_aux;
	void *param_ull;

	param_ull = HDR_LLL2ULL(rx->rx_ftr.param);

	if (ull_scan_is_valid_get(param_ull)) {
		struct lll_scan *lll;

		/* Mark for buffer for release */
		rx->type = NODE_RX_TYPE_RELEASE;

		lll = rx->rx_ftr.param;
		lll_aux = lll->lll_aux;
	} else if (ull_scan_aux_is_valid_get(param_ull)) {
		/* Mark for buffer for release */
		rx->type = NODE_RX_TYPE_RELEASE;

		lll_aux = rx->rx_ftr.param;
#if defined(CONFIG_BT_CTLR_SYNC_PERIODIC)
	} else if (ull_sync_is_valid_get(param_ull)) {
		struct lll_sync *lll;

		lll = rx->rx_ftr.param;
		lll_aux = lll->lll_aux;

		/* Change node type so HCI can dispatch report for truncated
		 * data properly.
		 */
		rx->type = NODE_RX_TYPE_SYNC_REPORT;
		rx->handle = ull_sync_handle_get(param_ull);

		/* Dequeue will try releasing list of node rx, set the extra
		 * pointer to NULL.
		 */
		rx->rx_ftr.extra = NULL;
#endif /* CONFIG_BT_CTLR_SYNC_PERIODIC */
	} else {
		LL_ASSERT(0);
		lll_aux = NULL;
	}

	if (lll_aux) {
		struct ll_scan_aux_set *aux;
		struct ull_hdr *hdr;

		aux = HDR_LLL2ULL(lll_aux);
		hdr = &aux->ull;

		LL_ASSERT(ull_ref_get(hdr) < 2);

		/* Flush from here of from done event, if one is pending */
		if (ull_ref_get(hdr) == 0) {
			flush(aux);
		} else {
			LL_ASSERT(!hdr->disabled_cb);

			hdr->disabled_param = aux;
			hdr->disabled_cb = last_disabled_cb;
		}
	}

	ll_rx_put(link, rx);
	ll_rx_sched();
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

static inline uint8_t aux_handle_get(struct ll_scan_aux_set *aux)
{
	return mem_index_get(aux, ll_scan_aux_pool,
			     sizeof(struct ll_scan_aux_set));
}

static inline struct ll_sync_set *sync_create_get(struct ll_scan_set *scan)
{
#if defined(CONFIG_BT_CTLR_SYNC_PERIODIC)
	return scan->per_scan.sync;
#else /* !CONFIG_BT_CTLR_SYNC_PERIODIC */
	return NULL;
#endif /* !CONFIG_BT_CTLR_SYNC_PERIODIC */
}

static inline struct ll_sync_iso_set *
	sync_iso_create_get(struct ll_sync_set *sync)
{
#if defined(CONFIG_BT_CTLR_SYNC_ISO)
	return sync->iso.sync_iso;
#else /* !CONFIG_BT_CTLR_SYNC_ISO */
	return NULL;
#endif /* !CONFIG_BT_CTLR_SYNC_ISO */
}

static void last_disabled_cb(void *param)
{
	flush(param);
}

static void done_disabled_cb(void *param)
{
	struct ll_scan_aux_set *aux;

	aux = param;
	LL_ASSERT(ull_scan_aux_is_valid_get(aux));

	aux = ull_scan_aux_is_valid_get(aux);
#if defined(CONFIG_BT_CTLR_SYNC_PERIODIC)
	if (!aux) {
		struct lll_sync *sync_lll;

		sync_lll = param;
		LL_ASSERT(sync_lll->lll_aux);
		aux = HDR_LLL2ULL(sync_lll->lll_aux);
	}
#endif /* CONFIG_BT_CTLR_SYNC_PERIODIC */

	flush(aux);
}

static void flush(struct ll_scan_aux_set *aux)
{
	struct node_rx_hdr *rx;

	/* Nodes are enqueued only in scan context so need to send them now */
	rx = aux->rx_head;
	if (rx) {
		struct lll_scan *lll;

		lll = aux->parent;
		lll->lll_aux = NULL;

		ll_rx_put(rx->link, rx);
		ll_rx_sched();
	} else {
		struct lll_sync *lll;

		lll = aux->parent;
		lll->lll_aux = NULL;
	}

	aux_release(aux);
}

static void ticker_cb(uint32_t ticks_at_expire, uint32_t ticks_drift,
		      uint32_t remainder, uint16_t lazy, uint8_t force,
		      void *param)
{
	static memq_link_t link;
	static struct mayfly mfy = {0, 0, &link, NULL, lll_scan_aux_prepare};
	struct ll_scan_aux_set *aux = param;
	static struct lll_prepare_param p;
	uint32_t ret;
	uint8_t ref;

	DEBUG_RADIO_PREPARE_O(1);

	/* Increment prepare reference count */
	ref = ull_ref_inc(&aux->ull);
	LL_ASSERT(ref);

	/* Append timing parameters */
	p.ticks_at_expire = ticks_at_expire;
	p.remainder = 0; /* FIXME: remainder; */
	p.lazy = lazy;
	p.force = force;
	p.param = &aux->lll;
	mfy.param = &p;

	/* Kick LLL prepare */
	ret = mayfly_enqueue(TICKER_USER_ID_ULL_HIGH, TICKER_USER_ID_LLL,
			     0, &mfy);
	LL_ASSERT(!ret);

	DEBUG_RADIO_PREPARE_O(1);
}

static void ticker_op_cb(uint32_t status, void *param)
{
	static memq_link_t link;
	static struct mayfly mfy = {0, 0, &link, NULL, ticker_op_aux_failure};
	uint32_t ret;

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
	flush(param);
}
