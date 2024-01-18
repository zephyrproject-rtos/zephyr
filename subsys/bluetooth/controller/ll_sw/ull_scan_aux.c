/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>

#include "util/mem.h"
#include "util/memq.h"
#include "util/mayfly.h"
#include "util/util.h"
#include "util/dbuf.h"

#include "hal/ticker.h"
#include "hal/ccm.h"

#include "ticker/ticker.h"

#include "pdu_df.h"
#include "lll/pdu_vendor.h"
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
#include "ull_df_internal.h"

#include <zephyr/bluetooth/hci_types.h>

#include <soc.h>
#include "hal/debug.h"

static int init_reset(void);
static inline struct ll_scan_aux_set *aux_acquire(void);
static inline void aux_release(struct ll_scan_aux_set *aux);
static inline uint8_t aux_handle_get(struct ll_scan_aux_set *aux);
static inline struct ll_sync_set *sync_create_get(struct ll_scan_set *scan);
#if defined(CONFIG_BT_CTLR_SYNC_PERIODIC)
static inline struct ll_sync_iso_set *
	sync_iso_create_get(struct ll_sync_set *sync);
#endif /* CONFIG_BT_CTLR_SYNC_PERIODIC */
static void done_disabled_cb(void *param);
static void flush_safe(void *param);
static void flush(void *param);
static void rx_release_put(struct node_rx_hdr *rx);
static void aux_sync_incomplete(void *param);
static void ticker_cb(uint32_t ticks_at_expire, uint32_t ticks_drift,
		      uint32_t remainder, uint16_t lazy, uint8_t force,
		      void *param);
static void ticker_op_cb(uint32_t status, void *param);

/* Auxiliary context pool used for reception of PDUs at aux offsets, common for
 * both Extended Advertising and Periodic Advertising.
 * Increasing the count allows simultaneous reception of interleaved chain PDUs
 * from multiple advertisers.
 */
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
	struct node_rx_hdr *rx_incomplete;
	struct ll_sync_iso_set *sync_iso;
	struct pdu_adv_aux_ptr *aux_ptr;
	struct pdu_adv_com_ext_adv *p;
	uint32_t ticks_slot_overhead;
	struct lll_scan_aux *lll_aux;
	struct ll_scan_aux_set *aux;
	uint8_t ticker_yield_handle;
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
	uint8_t hdr_buf_len;
	uint8_t aux_handle;
	bool is_scan_req;
	uint8_t acad_len;
	uint8_t data_len;
	uint8_t hdr_len;
	uint8_t *ptr;
	uint8_t phy;

	is_scan_req = false;
	ftr = &rx->rx_ftr;

	switch (rx->type) {
	case NODE_RX_TYPE_EXT_1M_REPORT:
		lll_aux = NULL;
		aux = NULL;
		sync_lll = NULL;
		sync_iso = NULL;
		rx_incomplete = NULL;

		lll = ftr->param;
		LL_ASSERT(!lll->lll_aux);

		scan = HDR_LLL2ULL(lll);
		sync = sync_create_get(scan);
		phy = BT_HCI_LE_EXT_SCAN_PHY_1M;

		ticker_yield_handle = TICKER_ID_SCAN_BASE +
				      ull_scan_handle_get(scan);
		break;

#if defined(CONFIG_BT_CTLR_PHY_CODED)
	case NODE_RX_TYPE_EXT_CODED_REPORT:
		lll_aux = NULL;
		aux = NULL;
		sync_lll = NULL;
		sync_iso = NULL;
		rx_incomplete = NULL;

		lll = ftr->param;
		LL_ASSERT(!lll->lll_aux);

		scan = HDR_LLL2ULL(lll);
		sync = sync_create_get(scan);
		phy = BT_HCI_LE_EXT_SCAN_PHY_CODED;

		ticker_yield_handle = TICKER_ID_SCAN_BASE +
				      ull_scan_handle_get(scan);
		break;
#endif /* CONFIG_BT_CTLR_PHY_CODED */

	case NODE_RX_TYPE_EXT_AUX_REPORT:
		sync_iso = NULL;
		rx_incomplete = NULL;
		if (ull_scan_aux_is_valid_get(HDR_LLL2ULL(ftr->param))) {
			sync_lll = NULL;

			/* Node has valid aux context so its scan was scheduled
			 * from ULL.
			 */
			lll_aux = ftr->param;
			aux = HDR_LLL2ULL(lll_aux);

			/* aux parent will be NULL for periodic sync */
			lll = aux->parent;
			LL_ASSERT(lll);

			ticker_yield_handle = TICKER_ID_SCAN_AUX_BASE +
					      aux_handle_get(aux);

		} else if (!IS_ENABLED(CONFIG_BT_CTLR_SYNC_PERIODIC) ||
			   ull_scan_is_valid_get(HDR_LLL2ULL(ftr->param))) {
			sync_lll = NULL;

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

			ticker_yield_handle = TICKER_NULL;

		} else {
			lll = NULL;

			/* If none of the above, node is part of sync scanning
			 */
			sync_lll = ftr->param;

			lll_aux = sync_lll->lll_aux;
			LL_ASSERT(lll_aux);

			aux = HDR_LLL2ULL(lll_aux);
			LL_ASSERT(sync_lll == aux->parent);

			ticker_yield_handle = TICKER_NULL;
		}

		if (!IS_ENABLED(CONFIG_BT_CTLR_SYNC_PERIODIC) || lll) {
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
		if (!IS_ENABLED(CONFIG_BT_CTLR_SYNC_PERIODIC) || scan) {
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
#if defined(CONFIG_BT_CTLR_PHY_CODED)
			case PHY_CODED:
				rx->type = NODE_RX_TYPE_EXT_CODED_REPORT;
				break;
#endif /* CONFIG_BT_CTLR_PHY_CODED */
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
			LL_ASSERT(!sync_lll->lll_aux);

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

			/* backup extra node_rx supplied for generating
			 * incomplete report
			 */
			rx_incomplete = ftr->extra;

			ticker_yield_handle = TICKER_ID_SCAN_SYNC_BASE +
					      ull_sync_handle_get(ull_sync);
#endif /* CONFIG_BT_CTLR_SYNC_PERIODIC */

		}
		break;
	default:
		LL_ASSERT(0);
		return;
	}

	rx->link = link;
	ftr->extra = NULL;

	ftr->aux_sched = 0U;

	pdu = (void *)((struct node_rx_pdu *)rx)->pdu;
	p = (void *)&pdu->adv_ext_ind;
	if (!pdu->len || !p->ext_hdr_len) {
		if (pdu->len) {
			data_len = pdu->len - PDU_AC_EXT_HEADER_SIZE_MIN;
		} else {
			data_len = 0U;
		}

		if (IS_ENABLED(CONFIG_BT_CTLR_SYNC_PERIODIC) && sync_lll) {
			struct ll_sync_set *sync_set;

			sync_set = HDR_LLL2ULL(sync_lll);
			ftr->aux_data_len = sync_set->data_len + data_len;
			sync_set->data_len = 0U;
		} else if (aux) {
			aux->data_len += data_len;
			ftr->aux_data_len = aux->data_len;
		} else {
			ftr->aux_data_len = data_len;
		}

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
		if (sync && (scan->periodic.state != LL_SYNC_STATE_CREATED)) {
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

		/* Check if Periodic Advertising Synchronization to be created.
		 * Setup synchronization if address and SID match in the
		 * Periodic Advertiser List or with the explicitly supplied.
		 */
		if (IS_ENABLED(CONFIG_BT_CTLR_SYNC_PERIODIC) && aux && sync && adi &&
		    ull_sync_setup_sid_match(scan, PDU_ADV_ADI_SID_GET(adi))) {
			ull_sync_setup(scan, aux, rx, si);
		}
	}

	if (h->tx_pwr) {
		ptr++;
	}

	/* Calculate ACAD Len */
	hdr_len = ptr - (uint8_t *)p;
	hdr_buf_len = PDU_AC_EXT_HEADER_SIZE_MIN + p->ext_hdr_len;
	if (hdr_len > hdr_buf_len) {
		/* FIXME: Handle invalid header length */
		acad_len = 0U;
	} else {
		acad_len = hdr_buf_len - hdr_len;
		hdr_len += acad_len;
	}

	/* calculate total data length */
	if (hdr_len < pdu->len) {
		data_len = pdu->len - hdr_len;
	} else {
		data_len = 0U;
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
	 * pointer being present or PHY in the aux pointer is invalid or unsupported.
	 */
	if (!aux_ptr || !PDU_ADV_AUX_PTR_OFFSET_GET(aux_ptr) || is_scan_req ||
	    (PDU_ADV_AUX_PTR_PHY_GET(aux_ptr) > EXT_ADV_AUX_PHY_LE_CODED) ||
		(!IS_ENABLED(CONFIG_BT_CTLR_PHY_CODED) &&
		  PDU_ADV_AUX_PTR_PHY_GET(aux_ptr) == EXT_ADV_AUX_PHY_LE_CODED)) {
		if (IS_ENABLED(CONFIG_BT_CTLR_SYNC_PERIODIC) && sync_lll) {
			struct ll_sync_set *sync_set;

			sync_set = HDR_LLL2ULL(sync_lll);
			ftr->aux_data_len = sync_set->data_len + data_len;
			sync_set->data_len = 0U;
		} else if (aux) {
			aux->data_len += data_len;
			ftr->aux_data_len = aux->data_len;
		} else {
			ftr->aux_data_len = data_len;
		}

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
			/* As LLL scheduling has been used and will fail due to
			 * non-allocation of aux context, a sync report with
			 * aux_failed flag set will be generated. Let the
			 * current sync report be set as partial, and the
			 * sync report corresponding to ull_scan_aux_release
			 * have the incomplete data status.
			 */
			if (ftr->aux_lll_sched) {
				ftr->aux_sched = 1U;
			}

			if (IS_ENABLED(CONFIG_BT_CTLR_SYNC_PERIODIC) &&
			    sync_lll) {
				struct ll_sync_set *sync_set;

				sync_set = HDR_LLL2ULL(sync_lll);
				ftr->aux_data_len = sync_set->data_len + data_len;
				sync_set->data_len = 0U;

			}

			goto ull_scan_aux_rx_flush;
		}

		aux->rx_head = aux->rx_last = NULL;
		aux->data_len = data_len;
		lll_aux = &aux->lll;
		lll_aux->is_chain_sched = 0U;

		ull_hdr_init(&aux->ull);
		lll_hdr_init(lll_aux, aux);

		aux->parent = lll ? (void *)lll : (void *)sync_lll;

#if defined(CONFIG_BT_CTLR_SYNC_PERIODIC)
		aux->rx_incomplete = rx_incomplete;
		rx_incomplete = NULL;
#endif /* CONFIG_BT_CTLR_SYNC_PERIODIC */

	} else {
		aux->data_len += data_len;

		/* Flush auxiliary PDU receptions and stop any more ULL
		 * scheduling if accumulated data length exceeds configured
		 * maximum supported.
		 */
		if (aux->data_len >= CONFIG_BT_CTLR_SCAN_DATA_LEN_MAX) {
			/* If LLL has already scheduled, then let it proceed.
			 *
			 * TODO: LLL to check accumulated data length and
			 *       stop further reception.
			 *       Currently LLL will schedule as long as there
			 *       are free node rx available.
			 */
			if (!ftr->aux_lll_sched) {
				goto ull_scan_aux_rx_flush;
			}
		}
	}

	/* In sync context we can dispatch rx immediately, in scan context we
	 * enqueue rx in aux context and will flush them after scan is complete.
	 */
	if (IS_ENABLED(CONFIG_BT_CTLR_SYNC_PERIODIC) && sync_lll) {
		struct ll_sync_set *sync_set;

		sync_set = HDR_LLL2ULL(sync_lll);
		sync_set->data_len += data_len;
		ftr->aux_data_len = sync_set->data_len;
	} else {
		if (aux->rx_last) {
			aux->rx_last->rx_ftr.extra = rx;
		} else {
			aux->rx_head = rx;
		}
		aux->rx_last = rx;

		ftr->aux_data_len = aux->data_len;
	}

	/* Initialize the channel index and PHY for the Auxiliary PDU reception.
	 */
	lll_aux->chan = aux_ptr->chan_idx;
	lll_aux->phy = BIT(PDU_ADV_AUX_PTR_PHY_GET(aux_ptr));

	/* See if this was already scheduled from LLL. If so, store aux context
	 * in global scan struct so we can pick it when scanned node is received
	 * with a valid context.
	 */
	if (ftr->aux_lll_sched) {
		/* AUX_ADV_IND/AUX_CHAIN_IND PDU reception is being setup */
		ftr->aux_sched = 1U;

		if (IS_ENABLED(CONFIG_BT_CTLR_SYNC_PERIODIC) && sync_lll) {
			sync_lll->lll_aux = lll_aux;

			/* In sync context, dispatch immediately */
			ll_rx_put_sched(link, rx);
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
	if (!IS_ENABLED(CONFIG_BT_CTLR_SYNC_PERIODIC) || lll) {
		LL_ASSERT(scan);

		/* Do not ULL schedule if scan disable requested */
		if (unlikely(scan->is_stop)) {
			goto ull_scan_aux_rx_flush;
		}

		/* Remove auxiliary context association with scan context so
		 * that LLL can differentiate it to being ULL scheduling.
		 */
		lll->lll_aux = NULL;
	} else {
		struct ll_sync_set *sync_set;

		LL_ASSERT(sync_lll &&
			  (!sync_lll->lll_aux || sync_lll->lll_aux == lll_aux));

		/* Do not ULL schedule if sync terminate requested */
		sync_set = HDR_LLL2ULL(sync_lll);
		if (unlikely(sync_set->is_stop)) {
			goto ull_scan_aux_rx_flush;
		}

		/* Associate the auxiliary context with sync context */
		sync_lll->lll_aux = lll_aux;

		/* Backup the node rx to be dispatch on successfully ULL
		 * scheduling setup.
		 */
		aux->rx_head = rx;
	}

	/* Determine the window size */
	if (aux_ptr->offs_units) {
		lll_aux->window_size_us = OFFS_UNIT_300_US;
	} else {
		lll_aux->window_size_us = OFFS_UNIT_30_US;
	}

	aux_offset_us = (uint32_t)PDU_ADV_AUX_PTR_OFFSET_GET(aux_ptr) * lll_aux->window_size_us;

	/* CA field contains the clock accuracy of the advertiser;
	 * 0 - 51 ppm to 500 ppm
	 * 1 - 0 ppm to 50 ppm
	 */
	if (aux_ptr->ca) {
		window_widening_us = SCA_DRIFT_50_PPM_US(aux_offset_us);
	} else {
		window_widening_us = SCA_DRIFT_500_PPM_US(aux_offset_us);
	}

	lll_aux->window_size_us += ((EVENT_TICKER_RES_MARGIN_US + EVENT_JITTER_US +
				     window_widening_us) << 1);

	ready_delay_us = lll_radio_rx_ready_delay_get(lll_aux->phy,
						      PHY_FLAGS_S8);

	/* Calculate the aux offset from start of the scan window */
	aux_offset_us += ftr->radio_end_us;
	aux_offset_us -= PDU_AC_US(pdu->len, phy, ftr->phy_flags);
	aux_offset_us -= EVENT_TICKER_RES_MARGIN_US;
	aux_offset_us -= EVENT_JITTER_US;
	aux_offset_us -= ready_delay_us;
	aux_offset_us -= window_widening_us;

	/* TODO: active_to_start feature port */
	aux->ull.ticks_active_to_start = 0;
	aux->ull.ticks_prepare_to_start =
		HAL_TICKER_US_TO_TICKS(EVENT_OVERHEAD_XTAL_US);
	aux->ull.ticks_preempt_to_start =
		HAL_TICKER_US_TO_TICKS(EVENT_OVERHEAD_PREEMPT_MIN_US);
	aux->ull.ticks_slot = HAL_TICKER_US_TO_TICKS_CEIL(
		EVENT_OVERHEAD_START_US + ready_delay_us +
		PDU_AC_MAX_US(PDU_AC_EXT_PAYLOAD_RX_SIZE, lll_aux->phy) +
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

#if (CONFIG_BT_CTLR_ULL_HIGH_PRIO == CONFIG_BT_CTLR_ULL_LOW_PRIO)
	/* disable ticker job, in order to chain yield and start to reduce
	 * CPU use by reducing successive calls to ticker_job().
	 */
	mayfly_enable(TICKER_USER_ID_ULL_HIGH, TICKER_USER_ID_ULL_LOW, 0);
#endif

	/* Yield the primary scan window or auxiliary or periodic sync event
	 * in ticker.
	 */
	if (ticker_yield_handle != TICKER_NULL) {
		ticker_status = ticker_yield_abs(TICKER_INSTANCE_ID_CTLR,
						 TICKER_USER_ID_ULL_HIGH,
						 ticker_yield_handle,
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
		  (ticker_status == TICKER_STATUS_BUSY) ||
		  ((ticker_status == TICKER_STATUS_FAILURE) &&
		   IS_ENABLED(CONFIG_BT_TICKER_LOW_LAT)));

#if (CONFIG_BT_CTLR_ULL_HIGH_PRIO == CONFIG_BT_CTLR_ULL_LOW_PRIO)
	/* enable ticker job, queued ticker operation will be handled
	 * thereafter.
	 */
	mayfly_enable(TICKER_USER_ID_ULL_HIGH, TICKER_USER_ID_ULL_LOW, 1);
#endif

	return;

ull_scan_aux_rx_flush:
#if defined(CONFIG_BT_CTLR_SYNC_PERIODIC)
	if (sync && (scan->periodic.state != LL_SYNC_STATE_CREATED)) {
		scan->periodic.state = LL_SYNC_STATE_IDLE;
	}
#endif /* CONFIG_BT_CTLR_SYNC_PERIODIC */

	if (aux) {
		/* Enqueue last rx in aux context if possible, otherwise send
		 * immediately since we are in sync context.
		 */
		if (!IS_ENABLED(CONFIG_BT_CTLR_SYNC_PERIODIC) || aux->rx_last) {
			LL_ASSERT(scan);

			/* If scan is being disabled, rx could already be
			 * enqueued before coming here to ull_scan_aux_rx_flush.
			 * Check if rx not the last in the list of received PDUs
			 * then add it, else do not add it, to avoid duplicate
			 * report generation, release and probable infinite loop
			 * processing of the list.
			 */
			if (unlikely(scan->is_stop)) {
				/* Add the node rx to aux context list of node
				 * rx if not already added when coming here to
				 * ull_scan_aux_rx_flush. This is handling a
				 * race condition where in the last PDU in
				 * chain is received and at the same time scan
				 * is being disabled.
				 */
				if (aux->rx_last != rx) {
					aux->rx_last->rx_ftr.extra = rx;
					aux->rx_last = rx;
				}

				return;
			}

			aux->rx_last->rx_ftr.extra = rx;
			aux->rx_last = rx;
		} else {
			const struct ll_sync_set *sync_set;

			LL_ASSERT(sync_lll);

			ll_rx_put_sched(link, rx);

			sync_set = HDR_LLL2ULL(sync_lll);
			if (unlikely(sync_set->is_stop && sync_lll->lll_aux)) {
				return;
			}
		}

		LL_ASSERT(aux->parent);

		flush_safe(aux);

		return;
	}

	ll_rx_put(link, rx);

	if (IS_ENABLED(CONFIG_BT_CTLR_SYNC_PERIODIC) && rx_incomplete) {
		rx_release_put(rx_incomplete);
	}

	ll_rx_sched();
}

void ull_scan_aux_done(struct node_rx_event_done *done)
{
	struct ll_scan_aux_set *aux;

	/* Get reference to ULL context */
	aux = CONTAINER_OF(done->param, struct ll_scan_aux_set, ull);

	if (IS_ENABLED(CONFIG_BT_CTLR_SYNC_PERIODIC) &&
	    !ull_scan_aux_is_valid_get(aux)) {
		struct ll_sync_set *sync;

		sync = CONTAINER_OF(done->param, struct ll_sync_set, ull);
		LL_ASSERT(ull_sync_is_valid_get(sync));

		/* Auxiliary context will be flushed by ull_scan_aux_stop() */
		if (unlikely(sync->is_stop) || !sync->lll.lll_aux) {
			return;
		}

		aux = HDR_LLL2ULL(sync->lll.lll_aux);
		LL_ASSERT(aux->parent);
	} else {
		struct ll_scan_set *scan;
		struct lll_scan *lll;

		lll = aux->parent;
		LL_ASSERT(lll);

		scan = HDR_LLL2ULL(lll);
		LL_ASSERT(ull_scan_is_valid_get(scan));

		/* Auxiliary context will be flushed by ull_scan_aux_stop() */
		if (unlikely(scan->is_stop)) {
			return;
		}
	}

	flush(aux);
}

struct ll_scan_aux_set *ull_scan_aux_set_get(uint8_t handle)
{
	if (handle >= CONFIG_BT_CTLR_SCAN_AUX_SET) {
		return NULL;
	}

	return &ll_scan_aux_pool[handle];
}

uint8_t ull_scan_aux_lll_handle_get(struct lll_scan_aux *lll)
{
	struct ll_scan_aux_set *aux;

	aux = HDR_LLL2ULL(lll);

	return aux_handle_get(aux);
}

void *ull_scan_aux_lll_parent_get(struct lll_scan_aux *lll,
				  uint8_t *is_lll_scan)
{
	struct ll_scan_aux_set *aux;

	aux = HDR_LLL2ULL(lll);

	if (is_lll_scan) {
		struct ll_scan_set *scan;
		struct lll_scan *lllscan;

		lllscan = aux->parent;
		LL_ASSERT(lllscan);

		scan = HDR_LLL2ULL(lllscan);
		*is_lll_scan = !!ull_scan_is_valid_get(scan);
	}

	return aux->parent;
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

struct lll_scan_aux *ull_scan_aux_lll_is_valid_get(struct lll_scan_aux *lll)
{
	struct ll_scan_aux_set *aux;

	aux = HDR_LLL2ULL(lll);
	aux = ull_scan_aux_is_valid_get(aux);
	if (aux) {
		return &aux->lll;
	}

	return NULL;
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

	} else if (!IS_ENABLED(CONFIG_BT_CTLR_SYNC_PERIODIC) ||
		   ull_scan_aux_is_valid_get(param_ull)) {
		/* Mark for buffer for release */
		rx->type = NODE_RX_TYPE_RELEASE;

		lll_aux = rx->rx_ftr.param;

	} else if (ull_sync_is_valid_get(param_ull)) {
		struct ll_sync_set *sync;
		struct lll_sync *lll;

		sync = param_ull;

		/* reset data len total */
		sync->data_len = 0U;

		lll = rx->rx_ftr.param;
		lll_aux = lll->lll_aux;

		/* Change node type so HCI can dispatch report for truncated
		 * data properly.
		 */
		rx->type = NODE_RX_TYPE_SYNC_REPORT;
		rx->handle = ull_sync_handle_get(sync);

		/* Dequeue will try releasing list of node rx, set the extra
		 * pointer to NULL.
		 */
		rx->rx_ftr.extra = NULL;

	} else {
		LL_ASSERT(0);
		lll_aux = NULL;
	}

	if (lll_aux) {
		struct ll_scan_aux_set *aux;
		struct ll_scan_set *scan;
		struct lll_scan *lll;
		uint8_t is_stop;

		aux = HDR_LLL2ULL(lll_aux);
		lll = aux->parent;
		LL_ASSERT(lll);

		scan = HDR_LLL2ULL(lll);
		scan = ull_scan_is_valid_get(scan);
		if (scan) {
			is_stop = scan->is_stop;
		} else {
			struct lll_sync *sync_lll;
			struct ll_sync_set *sync;

			sync_lll = (void *)lll;
			sync = HDR_LLL2ULL(sync_lll);
			is_stop = sync->is_stop;
		}

		if (!is_stop) {
			LL_ASSERT(aux->parent);

			flush_safe(aux);

		} else if (!scan) {
			/* Sync terminate requested, enqueue node rx so that it
			 * be flushed by ull_scan_aux_stop().
			 */
			rx->link = link;
			if (aux->rx_last) {
				aux->rx_last->rx_ftr.extra = rx;
			} else {
				aux->rx_head = rx;
			}
			aux->rx_last = rx;

			return;
		}
	}

	ll_rx_put(link, rx);
	ll_rx_sched();
}

int ull_scan_aux_stop(struct ll_scan_aux_set *aux)
{
	static memq_link_t link;
	static struct mayfly mfy = {0, 0, &link, NULL, NULL};
	uint8_t aux_handle;
	uint32_t ret;
	int err;

	/* Stop any ULL scheduling of auxiliary PDU scan */
	aux_handle = aux_handle_get(aux);
	err = ull_ticker_stop_with_mark(TICKER_ID_SCAN_AUX_BASE + aux_handle,
					aux, &aux->lll);
	if (err && (err != -EALREADY)) {
		return err;
	}

	/* Abort LLL event if ULL scheduling not used or already in prepare */
	if (err == -EALREADY) {
		err = ull_disable(&aux->lll);
		if (err && (err != -EALREADY)) {
			return err;
		}

		mfy.fp = flush;

	} else if (!IS_ENABLED(CONFIG_BT_CTLR_SYNC_PERIODIC)) {
		/* ULL scan auxiliary PDU reception scheduling stopped
		 * before prepare.
		 */
		mfy.fp = flush;

	} else {
		struct ll_scan_set *scan;
		struct lll_scan *lll;

		lll = aux->parent;
		LL_ASSERT(lll);

		scan = HDR_LLL2ULL(lll);
		scan = ull_scan_is_valid_get(scan);
		if (scan) {
			/* ULL scan auxiliary PDU reception scheduling stopped
			 * before prepare.
			 */
			mfy.fp = flush;
		} else {
			/* ULL sync chain reception scheduling stopped before
			 * prepare.
			 */
			mfy.fp = aux_sync_incomplete;
		}
	}

	/* Release auxiliary context in ULL execution context */
	mfy.param = aux;
	ret = mayfly_enqueue(TICKER_USER_ID_THREAD, TICKER_USER_ID_ULL_HIGH,
			     0, &mfy);
	LL_ASSERT(!ret);

	return 0;
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
	/* Clear the parent so that when scan is being disabled then this
	 * auxiliary context shall not associate itself from being disable.
	 */
	LL_ASSERT(aux->parent);
	aux->parent = NULL;

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
	return (!scan->periodic.cancelled) ? scan->periodic.sync : NULL;
#else /* !CONFIG_BT_CTLR_SYNC_PERIODIC */
	return NULL;
#endif /* !CONFIG_BT_CTLR_SYNC_PERIODIC */
}

#if defined(CONFIG_BT_CTLR_SYNC_PERIODIC)
static inline struct ll_sync_iso_set *
	sync_iso_create_get(struct ll_sync_set *sync)
{
#if defined(CONFIG_BT_CTLR_SYNC_ISO)
	return sync->iso.sync_iso;
#else /* !CONFIG_BT_CTLR_SYNC_ISO */
	return NULL;
#endif /* !CONFIG_BT_CTLR_SYNC_ISO */
}
#endif /* CONFIG_BT_CTLR_SYNC_PERIODIC */

static void done_disabled_cb(void *param)
{
	struct ll_scan_aux_set *aux;

	aux = param;
	LL_ASSERT(aux->parent);

	flush(aux);
}

static void flush_safe(void *param)
{
	struct ll_scan_aux_set *aux;
	struct ull_hdr *hdr;
	uint8_t ref;

	aux = param;
	LL_ASSERT(aux->parent);

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
	ref = ull_ref_get(hdr);
	if (ref == 0U) {
		flush(aux);
	} else {
		/* A specific single shot scheduled aux context
		 * cannot overlap, i.e. ULL reference count
		 * shall be less than 2.
		 */
		LL_ASSERT(ref < 2U);

		LL_ASSERT(!hdr->disabled_cb);
		hdr->disabled_param = aux;
		hdr->disabled_cb = done_disabled_cb;
	}
}

static void flush(void *param)
{
	struct ll_scan_aux_set *aux;
	struct ll_scan_set *scan;
	struct node_rx_hdr *rx;
	struct lll_scan *lll;
	bool sched = false;

	/* Debug check that parent was assigned when allocated for reception of
	 * auxiliary channel PDUs.
	 */
	aux = param;
	LL_ASSERT(aux->parent);

	rx = aux->rx_head;
	if (rx) {
		aux->rx_head = NULL;

		ll_rx_put(rx->link, rx);
		sched = true;
	}

#if defined(CONFIG_BT_CTLR_SYNC_PERIODIC)
	rx = aux->rx_incomplete;
	if (rx) {
		aux->rx_incomplete = NULL;

		rx_release_put(rx);
		sched = true;
	}
#endif /* CONFIG_BT_CTLR_SYNC_PERIODIC */

	if (sched) {
		ll_rx_sched();
	}

	lll = aux->parent;
	scan = HDR_LLL2ULL(lll);
	scan = ull_scan_is_valid_get(scan);
	if (!IS_ENABLED(CONFIG_BT_CTLR_SYNC_PERIODIC) || scan) {
		lll->lll_aux = NULL;
	} else {
		struct lll_sync *sync_lll;
		struct ll_sync_set *sync;

		sync_lll = aux->parent;
		sync = HDR_LLL2ULL(sync_lll);

		LL_ASSERT(sync->is_stop || sync_lll->lll_aux);
		sync_lll->lll_aux = NULL;
	}

	aux_release(aux);
}

static void rx_release_put(struct node_rx_hdr *rx)
{
	rx->type = NODE_RX_TYPE_RELEASE;

	ll_rx_put(rx->link, rx);
}

static void aux_sync_partial(void *param)
{
	struct ll_scan_aux_set *aux;
	struct node_rx_hdr *rx;

	aux = param;
	rx = aux->rx_head;
	aux->rx_head = NULL;

	LL_ASSERT(rx);
	rx->rx_ftr.aux_sched = 1U;

	ll_rx_put_sched(rx->link, rx);
}

static void aux_sync_incomplete(void *param)
{
#if defined(CONFIG_BT_CTLR_SYNC_PERIODIC)
	struct ll_scan_aux_set *aux;

	aux = param;
	LL_ASSERT(aux->parent);

	/* ULL scheduling succeeded hence no backup node rx present, use the
	 * extra node rx reserved for incomplete data status generation.
	 */
	if (!aux->rx_head) {
		struct ll_sync_set *sync;
		struct node_rx_hdr *rx;
		struct lll_sync *lll;

		/* get reference to sync context */
		lll = aux->parent;
		LL_ASSERT(lll);
		sync = HDR_LLL2ULL(lll);

		/* reset data len total */
		sync->data_len = 0U;

		/* pick extra node rx stored in aux context */
		rx = aux->rx_incomplete;
		LL_ASSERT(rx);
		aux->rx_incomplete = NULL;

		/* prepare sync report with failure */
		rx->type = NODE_RX_TYPE_SYNC_REPORT;
		rx->handle = ull_sync_handle_get(sync);
		rx->rx_ftr.param = lll;

		/* flag chain reception failure */
		rx->rx_ftr.aux_failed = 1U;

		/* Dequeue will try releasing list of node rx,
		 * set the extra pointer to NULL.
		 */
		rx->rx_ftr.extra = NULL;

		/* add to rx list, will be flushed */
		aux->rx_head = rx;
	}

	LL_ASSERT(!ull_ref_get(&aux->ull));

	flush(aux);
#endif /* CONFIG_BT_CTLR_SYNC_PERIODIC */
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
	static struct mayfly mfy = {0, 0, &link, NULL, NULL};
	struct ll_sync_set *sync;
	uint32_t ret;

	if (IS_ENABLED(CONFIG_BT_CTLR_SYNC_PERIODIC)) {
		struct ll_scan_aux_set *aux;
		struct lll_sync *sync_lll;

		aux = param;
		sync_lll = aux->parent;
		LL_ASSERT(sync_lll);

		sync = HDR_LLL2ULL(sync_lll);
		sync = ull_sync_is_valid_get(sync);
	} else {
		sync = NULL;
	}

	if (status == TICKER_STATUS_SUCCESS) {
		if (IS_ENABLED(CONFIG_BT_CTLR_SYNC_PERIODIC) && sync) {
			mfy.fp = aux_sync_partial;
		} else {
			return;
		}
	} else {
		if (IS_ENABLED(CONFIG_BT_CTLR_SYNC_PERIODIC) && sync) {
			mfy.fp = aux_sync_incomplete;
		} else {
			struct ll_scan_aux_set *aux;

			aux = param;
			LL_ASSERT(aux->parent);

			mfy.fp = flush_safe;
		}
	}

	mfy.param = param;

	ret = mayfly_enqueue(TICKER_USER_ID_ULL_LOW, TICKER_USER_ID_ULL_HIGH,
			     0, &mfy);
	LL_ASSERT(!ret);
}
