/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/slist.h>
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
#include "lll_conn_iso.h"
#include "lll_sync.h"
#include "lll_sync_iso.h"
#include "lll/lll_adv_types.h"
#include "lll_adv.h"
#include "lll/lll_adv_pdu.h"

#include "ll_sw/ull_tx_queue.h"

#include "isoal.h"
#include "ull_scan_types.h"
#include "ull_conn_types.h"
#include "ull_iso_types.h"
#include "ull_conn_iso_types.h"
#include "ull_sync_types.h"
#include "ull_adv_types.h"
#include "ull_adv_internal.h"

#include "ull_internal.h"
#include "ull_scan_internal.h"
#include "ull_conn_internal.h"
#include "ull_sync_internal.h"
#include "ull_sync_iso_internal.h"
#include "ull_df_internal.h"

#include <zephyr/bluetooth/hci_types.h>

#include <soc.h>
#include "hal/debug.h"

static int init_reset(void);
static void ticker_cb(uint32_t ticks_at_expire, uint32_t ticks_drift,
		      uint32_t remainder, uint16_t lazy, uint8_t force,
		      void *param);
static void ticker_op_cb(uint32_t status, void *param);
static void flush_safe(void *param);
static void done_disabled_cb(void *param);

#if !defined(CONFIG_BT_CTLR_SCAN_AUX_USE_CHAINS)

static inline struct ll_scan_aux_set *aux_acquire(void);
static inline void aux_release(struct ll_scan_aux_set *aux);
static inline uint8_t aux_handle_get(struct ll_scan_aux_set *aux);
static void flush(void *param);
static void aux_sync_incomplete(void *param);

/* Auxiliary context pool used for reception of PDUs at aux offsets, common for
 * both Extended Advertising and Periodic Advertising.
 * Increasing the count allows simultaneous reception of interleaved chain PDUs
 * from multiple advertisers.
 */
static struct ll_scan_aux_set ll_scan_aux_pool[CONFIG_BT_CTLR_SCAN_AUX_SET];
static void *scan_aux_free;

#else /* CONFIG_BT_CTLR_SCAN_AUX_USE_CHAINS */

static inline struct ll_scan_aux_chain *aux_chain_acquire(void);
static inline void aux_chain_release(struct ll_scan_aux_chain *chain);
struct ll_scan_aux_chain *scan_aux_chain_is_valid_get(struct ll_scan_aux_chain *chain);
struct ll_scan_aux_chain *lll_scan_aux_chain_is_valid_get(struct lll_scan_aux *lll);
#if defined(CONFIG_BT_CTLR_SYNC_PERIODIC)
static void aux_sync_incomplete(struct ll_scan_aux_chain *chain);
#endif /* CONFIG_BT_CTLR_SYNC_PERIODIC */
static void flush(struct ll_scan_aux_chain *chain);
static void chain_start_ticker(struct ll_scan_aux_chain *chain, bool replace);
static bool chain_insert_in_sched_list(struct ll_scan_aux_chain *chain);
static void chain_remove_from_list(struct ll_scan_aux_chain **head,
				   struct ll_scan_aux_chain *chain);
static void chain_append_to_list(struct ll_scan_aux_chain **head, struct ll_scan_aux_chain *chain);
static bool chain_is_in_list(struct ll_scan_aux_chain *head, struct ll_scan_aux_chain *chain);

/* Auxiliary context pool used for reception of PDUs at aux offsets, common for
 * both Extended Advertising and Periodic Advertising.
 * Increasing the count allows simultaneous reception of interleaved chain PDUs
 * from multiple advertisers.
 */
static struct ll_scan_aux_chain ll_scan_aux_pool[CONFIG_BT_CTLR_SCAN_AUX_CHAIN_COUNT];
static struct ll_scan_aux_set scan_aux_set;
static void *scan_aux_free;

static K_SEM_DEFINE(sem_scan_aux_stop, 0, 1);

#endif /* CONFIG_BT_CTLR_SCAN_AUX_USE_CHAINS */

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

static void rx_release_put(struct node_rx_pdu *rx)
{
	rx->hdr.type = NODE_RX_TYPE_RELEASE;

	ll_rx_put(rx->hdr.link, rx);
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

#if !defined(CONFIG_BT_CTLR_SCAN_AUX_USE_CHAINS)
void ull_scan_aux_setup(memq_link_t *link, struct node_rx_pdu *rx)
{
	struct node_rx_pdu *rx_incomplete;
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
	uint16_t window_size_us;
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
	uint32_t pdu_us;
	uint8_t phy_aux;
	uint8_t *ptr;
	uint8_t phy;

	is_scan_req = false;
	ftr = &rx->rx_ftr;

	switch (rx->hdr.type) {
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
				rx->hdr.type = NODE_RX_TYPE_EXT_1M_REPORT;
				break;
			case PHY_2M:
				rx->hdr.type = NODE_RX_TYPE_EXT_2M_REPORT;
				break;
#if defined(CONFIG_BT_CTLR_PHY_CODED)
			case PHY_CODED:
				rx->hdr.type = NODE_RX_TYPE_EXT_CODED_REPORT;
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
			rx->hdr.type = NODE_RX_TYPE_SYNC_REPORT;
			rx->hdr.handle = ull_sync_handle_get(sync);

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
			rx->hdr.handle = ull_sync_handle_get(ull_sync);

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

	rx->hdr.link = link;
	ftr->extra = NULL;

	ftr->aux_sched = 0U;

	pdu = (void *)rx->pdu;
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

	/* Note: The extended header contains a RFU flag that could potentially cause incorrect
	 * calculation of offset to ACAD field if it gets used to add a new header field; However,
	 * from discussion in BT errata ES-8080 it seems clear that BT SIG is aware that the RFU
	 * bit can not be used to add a new field since existing implementations will not be able
	 * to calculate the start of ACAD in that case
	 */

	ptr = h->data;

	if (h->adv_addr) {
#if defined(CONFIG_BT_CTLR_SYNC_PERIODIC)
		/* Check if Periodic Advertising Synchronization to be created
		 */
		if (sync && (scan->periodic.state != LL_SYNC_STATE_CREATED)) {
			/* Check address and update internal state */
#if defined(CONFIG_BT_CTLR_PRIVACY)
			ull_sync_setup_addr_check(sync, scan, pdu->tx_addr, ptr,
						  ftr->rl_idx);
#else /* !CONFIG_BT_CTLR_PRIVACY */
			ull_sync_setup_addr_check(sync, scan, pdu->tx_addr, ptr, 0U);
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
		    ull_sync_setup_sid_match(sync, scan, PDU_ADV_ADI_SID_GET(adi))) {
			ull_sync_setup(scan, aux->lll.phy, rx, si);
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
	    (rx->hdr.type == NODE_RX_TYPE_SYNC_REPORT) &&
	    acad_len) {
		/* Periodic Advertising Channel Map Indication */
		ull_sync_chm_update(rx->hdr.handle, ptr, acad_len);

#if defined(CONFIG_BT_CTLR_SYNC_ISO)
		struct ll_sync_set *sync_set;
		struct pdu_big_info *bi;
		uint8_t bi_size;

		sync_set = HDR_LLL2ULL(sync_lll);

		/* Provide encryption information for BIG sync creation */
		bi_size = ptr[PDU_ADV_DATA_HEADER_LEN_OFFSET] -
			  PDU_ADV_DATA_HEADER_TYPE_SIZE;
		sync_set->enc = (bi_size == PDU_BIG_INFO_ENCRYPTED_SIZE);

		/* Store number of BISes in the BIG */
		bi = (void *)&ptr[PDU_ADV_DATA_HEADER_DATA_OFFSET];
		sync_set->num_bis = PDU_BIG_INFO_NUM_BIS_GET(bi);

		/* Broadcast ISO synchronize */
		if (sync_iso) {
			ull_sync_iso_setup(sync_iso, rx, ptr, acad_len);
		}
#endif /* CONFIG_BT_CTLR_SYNC_ISO */
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

	/* Determine the window size */
	if (aux_ptr->offs_units) {
		window_size_us = OFFS_UNIT_300_US;
	} else {
		window_size_us = OFFS_UNIT_30_US;
	}

	/* Calculate received aux offset we need to have ULL schedule a reception */
	aux_offset_us = (uint32_t)PDU_ADV_AUX_PTR_OFFSET_GET(aux_ptr) * window_size_us;

	/* Skip reception if invalid aux offset */
	pdu_us = PDU_AC_US(pdu->len, phy, ftr->phy_flags);
	if (unlikely(!AUX_OFFSET_IS_VALID(aux_offset_us, window_size_us, pdu_us))) {
		goto ull_scan_aux_rx_flush;
	}

	/* CA field contains the clock accuracy of the advertiser;
	 * 0 - 51 ppm to 500 ppm
	 * 1 - 0 ppm to 50 ppm
	 */
	if (aux_ptr->ca) {
		window_widening_us = SCA_DRIFT_50_PPM_US(aux_offset_us);
	} else {
		window_widening_us = SCA_DRIFT_500_PPM_US(aux_offset_us);
	}

	phy_aux = BIT(PDU_ADV_AUX_PTR_PHY_GET(aux_ptr));
	ready_delay_us = lll_radio_rx_ready_delay_get(phy_aux, PHY_FLAGS_S8);

	/* Calculate the aux offset from start of the scan window */
	aux_offset_us += ftr->radio_end_us;
	aux_offset_us -= pdu_us;
	aux_offset_us -= EVENT_TICKER_RES_MARGIN_US;
	aux_offset_us -= EVENT_JITTER_US;
	aux_offset_us -= ready_delay_us;
	aux_offset_us -= window_widening_us;

	ticks_aux_offset = HAL_TICKER_US_TO_TICKS(aux_offset_us);

	/* Check if too late to ULL schedule an auxiliary PDU reception */
	if (!ftr->aux_lll_sched) {
		uint32_t ticks_at_expire;
		uint32_t overhead_us;
		uint32_t ticks_now;
		uint32_t diff;

		/* CPU execution overhead to setup the radio for reception plus the
		 * minimum prepare tick offset. And allow one additional event in
		 * between as overhead (say, an advertising event in between got closed
		 * when reception for auxiliary PDU is being setup).
		 */
		overhead_us = (EVENT_OVERHEAD_END_US + EVENT_OVERHEAD_START_US +
			       HAL_TICKER_TICKS_TO_US(HAL_TICKER_CNTR_CMP_OFFSET_MIN)) << 1;

		ticks_now = ticker_ticks_now_get();
		ticks_at_expire = ftr->ticks_anchor + ticks_aux_offset -
				  HAL_TICKER_US_TO_TICKS(overhead_us);
		diff = ticker_ticks_diff_get(ticks_now, ticks_at_expire);
		if ((diff & BIT(HAL_TICKER_CNTR_MSBIT)) == 0U) {
			goto ull_scan_aux_rx_flush;
		}
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
#if defined(CONFIG_BT_CTLR_JIT_SCHEDULING)
		if (lll) {
			lll_aux->hdr.score = lll->scan_aux_score;
		}
#endif /* CONFIG_BT_CTLR_JIT_SCHEDULING */

#if defined(CONFIG_BT_CTLR_SYNC_PERIODIC)
		aux->rx_incomplete = rx_incomplete;
		rx_incomplete = NULL;
#endif /* CONFIG_BT_CTLR_SYNC_PERIODIC */

	} else if (!(IS_ENABLED(CONFIG_BT_CTLR_SYNC_PERIODIC) && sync_lll)) {
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

		/* Flush auxiliary PDU receptions and stop any more ULL
		 * scheduling if accumulated data length exceeds configured
		 * maximum supported.
		 */
		if (sync_set->data_len >= CONFIG_BT_CTLR_SCAN_DATA_LEN_MAX) {
			/* If LLL has already scheduled, then let it proceed.
			 *
			 * TODO: LLL to check accumulated data length and
			 *       stop further reception.
			 *       Currently LLL will schedule as long as there
			 *       are free node rx available.
			 */
			if (!ftr->aux_lll_sched) {
				sync_set->data_len = 0U;
				goto ull_scan_aux_rx_flush;
			}
		}
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
	lll_aux->phy = phy_aux;

	/* See if this was already scheduled from LLL. If so, store aux context
	 * in global scan struct so we can pick it when scanned node is received
	 * with a valid context.
	 */
	if (ftr->aux_lll_sched) {
		if (IS_ENABLED(CONFIG_BT_CTLR_SYNC_PERIODIC) && sync_lll) {
			/* Associate Sync context with the Aux context so that
			 * it can continue reception in LLL scheduling.
			 */
			sync_lll->lll_aux = lll_aux;

			/* AUX_ADV_IND/AUX_CHAIN_IND PDU reception is being
			 * setup
			 */
			ftr->aux_sched = 1U;

			/* In sync context, dispatch immediately */
			ll_rx_put_sched(link, rx);
		} else {
			/* check scan context is not already using LLL
			 * scheduling, or receiving a chain then it will
			 * reuse the aux context.
			 */
			LL_ASSERT(!lll->lll_aux || (lll->lll_aux == lll_aux));

			/* Associate Scan context with the Aux context so that
			 * it can continue reception in LLL scheduling.
			 */
			lll->lll_aux = lll_aux;

			/* AUX_ADV_IND/AUX_CHAIN_IND PDU reception is being
			 * setup
			 */
			ftr->aux_sched = 1U;
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
	} else {
		struct ll_sync_set *sync_set;

		LL_ASSERT(sync_lll &&
			  (!sync_lll->lll_aux || sync_lll->lll_aux == lll_aux));

		/* Do not ULL schedule if sync terminate requested */
		sync_set = HDR_LLL2ULL(sync_lll);
		if (unlikely(sync_set->is_stop)) {
			goto ull_scan_aux_rx_flush;
		}

		/* Associate the auxiliary context with sync context, we do this
		 * for ULL scheduling also in constrast to how extended
		 * advertising only associates when LLL scheduling is used.
		 * Each Periodic Advertising chain is received by unique sync
		 * context, hence LLL and ULL scheduling is always associated
		 * with same unique sync context.
		 */
		sync_lll->lll_aux = lll_aux;

		/* Backup the node rx to be dispatch on successfully ULL
		 * scheduling setup.
		 */
		aux->rx_head = rx;
	}

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

	/* Initialize the window size for the Auxiliary PDU reception. */
	lll_aux->window_size_us = window_size_us;
	lll_aux->window_size_us += ((EVENT_TICKER_RES_MARGIN_US + EVENT_JITTER_US +
				     window_widening_us) << 1);

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

void ull_scan_aux_release(memq_link_t *link, struct node_rx_pdu *rx)
{
	struct lll_scan_aux *lll_aux;
	void *param_ull;

	param_ull = HDR_LLL2ULL(rx->rx_ftr.param);

	if (ull_scan_is_valid_get(param_ull)) {
		struct lll_scan *lll;

		/* Mark for buffer for release */
		rx->hdr.type = NODE_RX_TYPE_RELEASE;

		lll = rx->rx_ftr.param;
		lll_aux = lll->lll_aux;

	} else if (!IS_ENABLED(CONFIG_BT_CTLR_SYNC_PERIODIC) ||
		   ull_scan_aux_is_valid_get(param_ull)) {
		/* Mark for buffer for release */
		rx->hdr.type = NODE_RX_TYPE_RELEASE;

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
		rx->hdr.type = NODE_RX_TYPE_SYNC_REPORT;
		rx->hdr.handle = ull_sync_handle_get(sync);

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
			rx->hdr.link = link;
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
	struct node_rx_pdu *rx;
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

		ll_rx_put(rx->hdr.link, rx);
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
#if defined(CONFIG_BT_CTLR_JIT_SCHEDULING)
		lll->scan_aux_score = aux->lll.hdr.score;
#endif /* CONFIG_BT_CTLR_JIT_SCHEDULING */
	}

	aux_release(aux);
}

static void aux_sync_partial(void *param)
{
	struct ll_scan_aux_set *aux;
	struct node_rx_pdu *rx;

	aux = param;
	rx = aux->rx_head;
	aux->rx_head = NULL;

	LL_ASSERT(rx);
	rx->rx_ftr.aux_sched = 1U;

	ll_rx_put_sched(rx->hdr.link, rx);
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
		struct node_rx_pdu *rx;
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
		rx->hdr.type = NODE_RX_TYPE_SYNC_REPORT;
		rx->hdr.handle = ull_sync_handle_get(sync);
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

#else /* CONFIG_BT_CTLR_SCAN_AUX_USE_CHAINS */

void ull_scan_aux_setup(memq_link_t *link, struct node_rx_pdu *rx)
{
	struct node_rx_pdu *rx_incomplete;
	struct ll_sync_iso_set *sync_iso;
	struct ll_scan_aux_chain *chain;
	struct pdu_adv_aux_ptr *aux_ptr;
	struct pdu_adv_com_ext_adv *p;
	struct lll_scan_aux *lll_aux;
	uint32_t window_widening_us;
	uint32_t ticks_aux_offset;
	struct pdu_adv_ext_hdr *h;
	struct lll_sync *sync_lll;
	struct ll_scan_set *scan;
	struct ll_sync_set *sync;
	struct pdu_adv_adi *adi;
	struct node_rx_ftr *ftr;
	uint32_t ready_delay_us;
	uint16_t window_size_us;
	uint32_t aux_offset_us;
	struct lll_scan *lll;
	struct pdu_adv *pdu;
	uint8_t hdr_buf_len;
	bool is_scan_req;
	uint8_t acad_len;
	uint8_t data_len;
	uint8_t hdr_len;
	uint32_t pdu_us;
	uint8_t phy_aux;
	uint8_t *ptr;
	uint8_t phy;

	is_scan_req = false;
	ftr = &rx->rx_ftr;

	switch (rx->hdr.type) {
	case NODE_RX_TYPE_EXT_1M_REPORT:
		lll_aux = NULL;
		chain = NULL;
		sync_lll = NULL;
		sync_iso = NULL;
		rx_incomplete = NULL;

		lll = ftr->param;
		LL_ASSERT(!lll->lll_aux);

		scan = HDR_LLL2ULL(lll);
		sync = sync_create_get(scan);
		phy = BT_HCI_LE_EXT_SCAN_PHY_1M;
		break;

#if defined(CONFIG_BT_CTLR_PHY_CODED)
	case NODE_RX_TYPE_EXT_CODED_REPORT:
		lll_aux = NULL;
		chain = NULL;
		sync_lll = NULL;
		sync_iso = NULL;
		rx_incomplete = NULL;

		lll = ftr->param;
		LL_ASSERT(!lll->lll_aux);

		scan = HDR_LLL2ULL(lll);
		sync = sync_create_get(scan);
		phy = BT_HCI_LE_EXT_SCAN_PHY_CODED;
		break;
#endif /* CONFIG_BT_CTLR_PHY_CODED */

	case NODE_RX_TYPE_EXT_AUX_REPORT:
		sync_iso = NULL;
		rx_incomplete = NULL;
		if (lll_scan_aux_chain_is_valid_get(ftr->param)) {
			sync_lll = NULL;

			/* Node has valid chain context so its scan was scheduled
			 * from ULL.
			 */
			lll_aux = ftr->param;
			chain = CONTAINER_OF(lll_aux, struct ll_scan_aux_chain, lll);

			/* chain parent will be NULL for periodic sync */
			lll = chain->parent;
			LL_ASSERT(lll);

		} else if (!IS_ENABLED(CONFIG_BT_CTLR_SYNC_PERIODIC) ||
			   ull_scan_is_valid_get(HDR_LLL2ULL(ftr->param))) {
			sync_lll = NULL;

			/* Node that does not have valid chain context but has
			 * valid scan set was scheduled from LLL. We can
			 * retrieve chain context from lll_scan as it was stored
			 * there when superior PDU was handled.
			 */
			lll = ftr->param;

			lll_aux = lll->lll_aux;
			LL_ASSERT(lll_aux);

			chain = CONTAINER_OF(lll_aux, struct ll_scan_aux_chain, lll);
			LL_ASSERT(lll == chain->parent);

		} else {
			lll = NULL;

			/* If none of the above, node is part of sync scanning
			 */
			sync_lll = ftr->param;

			lll_aux = sync_lll->lll_aux;
			LL_ASSERT(lll_aux);

			chain = CONTAINER_OF(lll_aux, struct ll_scan_aux_chain, lll);
			LL_ASSERT(sync_lll == chain->parent);
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
				rx->hdr.type = NODE_RX_TYPE_EXT_1M_REPORT;
				break;
			case PHY_2M:
				rx->hdr.type = NODE_RX_TYPE_EXT_2M_REPORT;
				break;
#if defined(CONFIG_BT_CTLR_PHY_CODED)
			case PHY_CODED:
				rx->hdr.type = NODE_RX_TYPE_EXT_CODED_REPORT;
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
			rx->hdr.type = NODE_RX_TYPE_SYNC_REPORT;
			rx->hdr.handle = ull_sync_handle_get(sync);

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
			rx->hdr.handle = ull_sync_handle_get(ull_sync);

			/* Check if we need to create BIG sync */
			sync_iso = sync_iso_create_get(ull_sync);

			/* FIXME: we will need lll_scan if chain was scheduled
			 *        from LLL; should we store lll_scan_set in
			 *        sync_lll instead?
			 */
			lll = NULL;
			lll_aux = NULL;
			chain = NULL;
			scan = NULL;
			sync = NULL;
			phy =  sync_lll->phy;

			/* backup extra node_rx supplied for generating
			 * incomplete report
			 */
			rx_incomplete = ftr->extra;
#endif /* CONFIG_BT_CTLR_SYNC_PERIODIC */

		}
		break;
	default:
		LL_ASSERT(0);
		return;
	}

	rx->hdr.link = link;
	ftr->extra = NULL;

	ftr->aux_sched = 0U;

	if (chain) {
		chain->aux_sched = 0U;

		if (!is_scan_req) {
			/* Remove chain from active list */
			chain_remove_from_list(&scan_aux_set.active_chains, chain);

			/* Reset LLL scheduled flag */
			chain->is_lll_sched = 0U;
		}
	}

	pdu = (void *)rx->pdu;
	p = (void *)&pdu->adv_ext_ind;
	if (!pdu->len || !p->ext_hdr_len) {
		if (pdu->len) {
			data_len = pdu->len - PDU_AC_EXT_HEADER_SIZE_MIN;
		} else {
			data_len = 0U;
		}

		if (chain) {
			chain->data_len += data_len;
			ftr->aux_data_len = chain->data_len;
		} else {
			ftr->aux_data_len = data_len;
		}

		goto ull_scan_aux_rx_flush;
	}

	h = (void *)p->ext_hdr_adv_data;

	/* Note: The extended header contains a RFU flag that could potentially cause incorrect
	 * calculation of offset to ACAD field if it gets used to add a new header field; However,
	 * from discussion in BT errata ES-8080 it seems clear that BT SIG is aware that the RFU
	 * bit can not be used to add a new field since existing implementations will not be able
	 * to calculate the start of ACAD in that case
	 */

	ptr = h->data;

	if (h->adv_addr) {
#if defined(CONFIG_BT_CTLR_SYNC_PERIODIC)
		/* Check if Periodic Advertising Synchronization to be created
		 */
		if (sync && (scan->periodic.state != LL_SYNC_STATE_CREATED)) {
			/* Check address and update internal state */
#if defined(CONFIG_BT_CTLR_PRIVACY)
			ull_sync_setup_addr_check(sync, scan, pdu->tx_addr, ptr,
						  ftr->rl_idx);
#else /* !CONFIG_BT_CTLR_PRIVACY */
			ull_sync_setup_addr_check(sync, scan, pdu->tx_addr, ptr, 0U);
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
		if (IS_ENABLED(CONFIG_BT_CTLR_SYNC_PERIODIC) && chain && sync && adi &&
		    ull_sync_setup_sid_match(sync, scan, PDU_ADV_ADI_SID_GET(adi))) {
			ull_sync_setup(scan, chain->lll.phy, rx, si);
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

	/* calculate and set total data length */
	if (hdr_len < pdu->len) {
		data_len = pdu->len - hdr_len;
	} else {
		data_len = 0U;
	}

	if (chain) {
		chain->data_len += data_len;
		ftr->aux_data_len = chain->data_len;
	} else {
		ftr->aux_data_len = data_len;
	}

	/* Periodic Advertising Channel Map Indication and/or Broadcast ISO
	 * synchronization
	 */
	if (IS_ENABLED(CONFIG_BT_CTLR_SYNC_PERIODIC) &&
	    (rx->hdr.type == NODE_RX_TYPE_SYNC_REPORT) &&
	    acad_len) {
		/* Periodic Advertising Channel Map Indication */
		ull_sync_chm_update(rx->hdr.handle, ptr, acad_len);

#if defined(CONFIG_BT_CTLR_SYNC_ISO)
		struct ll_sync_set *sync_set;
		struct pdu_big_info *bi;
		uint8_t bi_size;

		sync_set = HDR_LLL2ULL(sync_lll);

		/* Provide encryption information for BIG sync creation */
		bi_size = ptr[PDU_ADV_DATA_HEADER_LEN_OFFSET] -
			  PDU_ADV_DATA_HEADER_TYPE_SIZE;
		sync_set->enc = (bi_size == PDU_BIG_INFO_ENCRYPTED_SIZE);

		/* Store number of BISes in the BIG */
		bi = (void *)&ptr[PDU_ADV_DATA_HEADER_DATA_OFFSET];
		sync_set->num_bis = PDU_BIG_INFO_NUM_BIS_GET(bi);

		/* Broadcast ISO synchronize */
		if (sync_iso) {
			ull_sync_iso_setup(sync_iso, rx, ptr, acad_len);
		}
#endif /* CONFIG_BT_CTLR_SYNC_ISO */
	}

	/* Do not ULL schedule auxiliary PDU reception if no aux pointer
	 * or aux pointer is zero or scannable advertising has erroneous aux
	 * pointer being present or PHY in the aux pointer is invalid or unsupported
	 * or if scanning and scan has been stopped
	 */
	if (!aux_ptr || !PDU_ADV_AUX_PTR_OFFSET_GET(aux_ptr) || is_scan_req ||
	    (PDU_ADV_AUX_PTR_PHY_GET(aux_ptr) > EXT_ADV_AUX_PHY_LE_CODED) ||
		(!IS_ENABLED(CONFIG_BT_CTLR_PHY_CODED) &&
		  PDU_ADV_AUX_PTR_PHY_GET(aux_ptr) == EXT_ADV_AUX_PHY_LE_CODED)) {

		if (is_scan_req) {
			LL_ASSERT(chain && chain->rx_last);

			chain->rx_last->rx_ftr.extra = rx;
			chain->rx_last = rx;

			return;
		}

		goto ull_scan_aux_rx_flush;
	}

	/* Determine the window size */
	if (aux_ptr->offs_units) {
		window_size_us = OFFS_UNIT_300_US;
	} else {
		window_size_us = OFFS_UNIT_30_US;
	}

	/* Calculate received aux offset we need to have ULL schedule a reception */
	aux_offset_us = (uint32_t)PDU_ADV_AUX_PTR_OFFSET_GET(aux_ptr) * window_size_us;

	/* Skip reception if invalid aux offset */
	pdu_us = PDU_AC_US(pdu->len, phy, ftr->phy_flags);
	if (unlikely(!AUX_OFFSET_IS_VALID(aux_offset_us, window_size_us, pdu_us))) {
		goto ull_scan_aux_rx_flush;
	}

	/* CA field contains the clock accuracy of the advertiser;
	 * 0 - 51 ppm to 500 ppm
	 * 1 - 0 ppm to 50 ppm
	 */
	if (aux_ptr->ca) {
		window_widening_us = SCA_DRIFT_50_PPM_US(aux_offset_us);
	} else {
		window_widening_us = SCA_DRIFT_500_PPM_US(aux_offset_us);
	}

	phy_aux = BIT(PDU_ADV_AUX_PTR_PHY_GET(aux_ptr));
	ready_delay_us = lll_radio_rx_ready_delay_get(phy_aux, PHY_FLAGS_S8);

	/* Calculate the aux offset from start of the scan window */
	aux_offset_us += ftr->radio_end_us;
	aux_offset_us -= pdu_us;
	aux_offset_us -= EVENT_TICKER_RES_MARGIN_US;
	aux_offset_us -= EVENT_JITTER_US;
	aux_offset_us -= ready_delay_us;
	aux_offset_us -= window_widening_us;

	ticks_aux_offset = HAL_TICKER_US_TO_TICKS(aux_offset_us);

	/* Check if too late to ULL schedule an auxiliary PDU reception */
	if (!ftr->aux_lll_sched) {
		uint32_t ticks_at_expire;
		uint32_t overhead_us;
		uint32_t ticks_now;
		uint32_t diff;

		/* CPU execution overhead to setup the radio for reception plus the
		 * minimum prepare tick offset. And allow one additional event in
		 * between as overhead (say, an advertising event in between got closed
		 * when reception for auxiliary PDU is being setup).
		 */
		overhead_us = (EVENT_OVERHEAD_END_US + EVENT_OVERHEAD_START_US +
			       HAL_TICKER_TICKS_TO_US(HAL_TICKER_CNTR_CMP_OFFSET_MIN)) << 1;

		ticks_now = ticker_ticks_now_get();
		ticks_at_expire = ftr->ticks_anchor + ticks_aux_offset -
				  HAL_TICKER_US_TO_TICKS(overhead_us);
		diff = ticker_ticks_diff_get(ticks_now, ticks_at_expire);
		if ((diff & BIT(HAL_TICKER_CNTR_MSBIT)) == 0U) {
			goto ull_scan_aux_rx_flush;
		}
	}

	if (!chain) {
		chain = aux_chain_acquire();
		if (!chain) {
			/* As LLL scheduling has been used and will fail due to
			 * non-allocation of a new chain context, a sync report with
			 * aux_failed flag set will be generated. Let the
			 * current sync report be set as partial, and the
			 * sync report corresponding to ull_scan_aux_release
			 * have the incomplete data status.
			 */
			if (ftr->aux_lll_sched) {
				ftr->aux_sched = 1U;
			}

			goto ull_scan_aux_rx_flush;
		}

		chain->rx_head = chain->rx_last = NULL;
		chain->data_len = data_len;
		chain->is_lll_sched = ftr->aux_lll_sched;
		lll_aux = &chain->lll;
		lll_aux->is_chain_sched = 0U;

		lll_hdr_init(lll_aux, &scan_aux_set);

		chain->parent = lll ? (void *)lll : (void *)sync_lll;
#if defined(CONFIG_BT_CTLR_JIT_SCHEDULING)
		if (lll) {
			lll_aux->hdr.score = lll->scan_aux_score;
		}
#endif /* CONFIG_BT_CTLR_JIT_SCHEDULING */

#if defined(CONFIG_BT_CTLR_SYNC_PERIODIC)
		if (sync_lll) {
			struct ll_sync_set *sync_set = HDR_LLL2ULL(sync_lll);

			sync_set->rx_incomplete = rx_incomplete;
			rx_incomplete = NULL;
		}
#endif /* CONFIG_BT_CTLR_SYNC_PERIODIC */

		/* See if this was already scheduled from LLL. If so, store aux context
		 * in global scan/sync struct so we can pick it when scanned node is received
		 * with a valid context.
		 */
		if (ftr->aux_lll_sched) {

			if (IS_ENABLED(CONFIG_BT_CTLR_SYNC_PERIODIC) && sync_lll) {
				sync_lll->lll_aux = lll_aux;
			} else {
				lll->lll_aux = lll_aux;
			}

			/* Reset auxiliary channel PDU scan state which otherwise is
			 * done in the prepare_cb when ULL scheduling is used.
			 */
			lll_aux->state = 0U;
		}
	} else if (chain->data_len >= CONFIG_BT_CTLR_SCAN_DATA_LEN_MAX) {

		/* Flush auxiliary PDU receptions and stop any more ULL
		 * scheduling if accumulated data length exceeds configured
		 * maximum supported.
		 */

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

	/* In sync context we can dispatch rx immediately, in scan context we
	 * enqueue rx in aux context and will flush them after scan is complete.
	 */
	if (!IS_ENABLED(CONFIG_BT_CTLR_SYNC_PERIODIC) || scan) {
		if (chain->rx_last) {
			chain->rx_last->rx_ftr.extra = rx;
		} else {
			chain->rx_head = rx;
		}
		chain->rx_last = rx;
	}

	/* Initialize the channel index and PHY for the Auxiliary PDU reception.
	 */
	lll_aux->chan = aux_ptr->chan_idx;
	lll_aux->phy = phy_aux;

	if (ftr->aux_lll_sched) {
		/* AUX_ADV_IND/AUX_CHAIN_IND PDU reception is being setup */
		ftr->aux_sched = 1U;
		chain->aux_sched = 1U;

		chain->next = scan_aux_set.active_chains;
		scan_aux_set.active_chains = chain;

		if (IS_ENABLED(CONFIG_BT_CTLR_SYNC_PERIODIC) && sync_lll) {
			/* In sync context, dispatch immediately */
			ll_rx_put_sched(link, rx);
		}

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
		if (lll->lll_aux == &chain->lll) {
			lll->lll_aux = NULL;
		}
	} else {
		struct ll_sync_set *sync_set;

		LL_ASSERT(sync_lll &&
			  (!sync_lll->lll_aux || sync_lll->lll_aux == lll_aux));

		/* Do not ULL schedule if sync terminate requested */
		sync_set = HDR_LLL2ULL(sync_lll);
		if (unlikely(sync_set->is_stop)) {
			goto ull_scan_aux_rx_flush;
		}

		/* Associate the auxiliary context with sync context, we do this
		 * for ULL scheduling also in constrast to how extended
		 * advertising only associates when LLL scheduling is used.
		 * Each Periodic Advertising chain is received by unique sync
		 * context, hence LLL and ULL scheduling is always associated
		 * with same unique sync context.
		 */
		sync_lll->lll_aux = lll_aux;

	}

	lll_aux->window_size_us = window_size_us;
	lll_aux->window_size_us += ((EVENT_TICKER_RES_MARGIN_US + EVENT_JITTER_US +
				     window_widening_us) << 1);

	chain->ticker_ticks = (ftr->ticks_anchor + ticks_aux_offset) & HAL_TICKER_CNTR_MASK;

	if (!chain_insert_in_sched_list(chain)) {
		/* Failed to add chain - flush */
		goto ull_scan_aux_rx_flush;
	}

#if defined(CONFIG_BT_CTLR_SYNC_PERIODIC)
	if (sync_lll) {
		/* In sync context, dispatch immediately */
		rx->rx_ftr.aux_sched = 1U;
		chain->aux_sched = 1U;
		ll_rx_put_sched(link, rx);
	}
#endif /* CONFIG_BT_CTLR_SYNC_PERIODIC */

	return;

ull_scan_aux_rx_flush:
#if defined(CONFIG_BT_CTLR_SYNC_PERIODIC)
	if (sync && (scan->periodic.state != LL_SYNC_STATE_CREATED)) {
		scan->periodic.state = LL_SYNC_STATE_IDLE;
	}
#endif /* CONFIG_BT_CTLR_SYNC_PERIODIC */

	if (chain) {
		/* Enqueue last rx in chain context if possible, otherwise send
		 * immediately since we are in sync context.
		 */
		if (!IS_ENABLED(CONFIG_BT_CTLR_SYNC_PERIODIC) || chain->rx_last) {
			LL_ASSERT(scan);

			/* rx could already be enqueued before coming here -
			 * check if rx not the last in the list of received PDUs
			 * then add it, else do not add it, to avoid duplicate
			 * report generation, release and probable infinite loop
			 * processing of the list.
			 */
			if (chain->rx_last != rx) {
				chain->rx_last->rx_ftr.extra = rx;
				chain->rx_last = rx;
			}
		} else {
			LL_ASSERT(sync_lll);

			ll_rx_put_sched(link, rx);
		}

		LL_ASSERT(chain->parent);

		flush_safe(chain);

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
	struct ll_scan_aux_chain *chain;

	/* Get reference to chain */
	chain = CONTAINER_OF(done->extra.lll, struct ll_scan_aux_chain, lll);
	LL_ASSERT(scan_aux_chain_is_valid_get(chain));

	/* Remove chain from active list */
	chain_remove_from_list(&scan_aux_set.active_chains, chain);

	flush(chain);
}

struct ll_scan_aux_chain *lll_scan_aux_chain_is_valid_get(struct lll_scan_aux *lll)
{
	return scan_aux_chain_is_valid_get(CONTAINER_OF(lll, struct ll_scan_aux_chain, lll));
}

void *ull_scan_aux_lll_parent_get(struct lll_scan_aux *lll,
				  uint8_t *is_lll_scan)
{
	struct ll_scan_aux_chain *chain;

	chain = CONTAINER_OF(lll, struct ll_scan_aux_chain, lll);

	if (is_lll_scan) {
		struct ll_scan_set *scan;
		struct lll_scan *lllscan;

		lllscan = chain->parent;
		LL_ASSERT(lllscan);

		scan = HDR_LLL2ULL(lllscan);
		*is_lll_scan = !!ull_scan_is_valid_get(scan);
	}

	return chain->parent;
}

struct ll_scan_aux_chain *scan_aux_chain_is_valid_get(struct ll_scan_aux_chain *chain)
{
	if (((uint8_t *)chain < (uint8_t *)ll_scan_aux_pool) ||
	    ((uint8_t *)chain > ((uint8_t *)ll_scan_aux_pool +
			       (sizeof(struct ll_scan_aux_chain) *
				(CONFIG_BT_CTLR_SCAN_AUX_CHAIN_COUNT - 1))))) {
		return NULL;
	}

	return chain;
}

struct lll_scan_aux *ull_scan_aux_lll_is_valid_get(struct lll_scan_aux *lll)
{
	struct ll_scan_aux_chain *chain;

	chain = CONTAINER_OF(lll, struct ll_scan_aux_chain, lll);
	chain = scan_aux_chain_is_valid_get(chain);
	if (chain) {
		return &chain->lll;
	}

	return NULL;
}

void ull_scan_aux_release(memq_link_t *link, struct node_rx_pdu *rx)
{
	struct lll_scan_aux *lll_aux;
	void *param_ull;

	param_ull = HDR_LLL2ULL(rx->rx_ftr.param);

	/* Mark for buffer for release */
	rx->hdr.type = NODE_RX_TYPE_RELEASE;

	if (ull_scan_is_valid_get(param_ull)) {
		struct lll_scan *lll;

		lll = rx->rx_ftr.param;
		lll_aux = lll->lll_aux;

	} else if (!IS_ENABLED(CONFIG_BT_CTLR_SYNC_PERIODIC) ||
		   param_ull == &scan_aux_set) {

		lll_aux = rx->rx_ftr.param;

	} else if (ull_sync_is_valid_get(param_ull)) {
		struct lll_sync *lll;

		lll = rx->rx_ftr.param;
		lll_aux = lll->lll_aux;

		if (!lll_aux) {
			struct ll_sync_set *sync = param_ull;

			/* Change node type so HCI can dispatch report for truncated
			 * data properly.
			 */
			rx->hdr.type = NODE_RX_TYPE_SYNC_REPORT;
			rx->hdr.handle = ull_sync_handle_get(sync);

			/* Dequeue will try releasing list of node rx, set the extra
			 * pointer to NULL.
			 */
			rx->rx_ftr.extra = NULL;
		}
	} else {
		LL_ASSERT(0);
		lll_aux = NULL;
	}

	if (lll_aux) {
		struct ll_scan_aux_chain *chain;
		struct ll_scan_set *scan;
		struct lll_scan *lll;
		uint8_t is_stop;

		chain = CONTAINER_OF(lll_aux, struct ll_scan_aux_chain, lll);
		lll = chain->parent;
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
			LL_ASSERT(chain->parent);

			/* Remove chain from active list and flush */
			chain_remove_from_list(&scan_aux_set.active_chains, chain);
			flush(chain);
		}
	}

	ll_rx_put(link, rx);
	ll_rx_sched();
}

static void scan_aux_stop_all_chains_for_parent(void *parent)
{
	static memq_link_t link;
	static struct mayfly mfy = {0, 0, &link, NULL, lll_disable};
	struct ll_scan_aux_chain *curr = scan_aux_set.sched_chains;
	struct ll_scan_aux_chain *prev = NULL;
	bool ticker_stopped = false;
	bool disabling = false;

	if (curr && curr->parent == parent) {
		uint8_t ticker_status;

		/* Scheduled head is about to be removed - stop running ticker */
		ticker_status = ticker_stop(TICKER_INSTANCE_ID_CTLR, TICKER_USER_ID_ULL_HIGH,
				    TICKER_ID_SCAN_AUX, NULL, NULL);
		LL_ASSERT((ticker_status == TICKER_STATUS_SUCCESS) ||
			  (ticker_status == TICKER_STATUS_BUSY));
		ticker_stopped = true;
	}

	while (curr) {
		if (curr->parent == parent) {
			if (curr == scan_aux_set.sched_chains) {
				scan_aux_set.sched_chains = curr->next;
				flush(curr);
				curr = scan_aux_set.sched_chains;
			} else {
				prev->next = curr->next;
				flush(curr);
				curr = prev->next;
			}
		} else {
			prev = curr;
			curr = curr->next;
		}
	}

	if (ticker_stopped && scan_aux_set.sched_chains) {
		/* Start ticker using new head */
		chain_start_ticker(scan_aux_set.sched_chains, false);
	}

	/* Check active chains */
	prev = NULL;
	curr = scan_aux_set.active_chains;
	while (curr) {
		if (curr->parent == parent) {
			struct ll_scan_aux_chain *chain = curr;
			uint32_t ret;

			if (curr == scan_aux_set.active_chains) {
				scan_aux_set.active_chains = curr->next;
				curr = scan_aux_set.active_chains;
			} else {
				prev->next = curr->next;
				curr = prev->next;
			}

			if (chain->is_lll_sched || ull_ref_get(&scan_aux_set.ull) == 0) {
				/* Disable called by parent disable or race with scan stop */
				flush(chain);
			} else {
				/* Flush on disabled callback */
				chain->next = scan_aux_set.flushing_chains;
				scan_aux_set.flushing_chains = chain;
				scan_aux_set.ull.disabled_cb = done_disabled_cb;

				/* Call lll_disable */
				disabling = true;
				mfy.param = &curr->lll;
				ret = mayfly_enqueue(TICKER_USER_ID_ULL_HIGH, TICKER_USER_ID_LLL, 0,
				     &mfy);
				LL_ASSERT(!ret);
			}
		} else {
			prev = curr;
			curr = curr->next;
		}
	}

	if (!disabling) {
		/* Signal completion */
		k_sem_give(&sem_scan_aux_stop);
	}
}

/* Stops all chains with the given parent */
int ull_scan_aux_stop(void *parent)
{
	static memq_link_t link;
	static struct mayfly mfy = {0, 0, &link, NULL, scan_aux_stop_all_chains_for_parent};
	uint32_t ret;

	/* Stop chains in ULL execution context */
	mfy.param = parent;
	ret = mayfly_enqueue(TICKER_USER_ID_THREAD, TICKER_USER_ID_ULL_HIGH, 0, &mfy);
	LL_ASSERT(!ret);

	/* Wait for chains to be stopped before returning */
	(void)k_sem_take(&sem_scan_aux_stop, K_FOREVER);

	return 0;
}

static int init_reset(void)
{
	ull_hdr_init(&scan_aux_set.ull);
	scan_aux_set.sched_chains = NULL;
	scan_aux_set.active_chains = NULL;

	/* Initialize scan aux chains pool */
	mem_init(ll_scan_aux_pool, sizeof(struct ll_scan_aux_chain),
		 sizeof(ll_scan_aux_pool) / sizeof(struct ll_scan_aux_chain),
		 &scan_aux_free);

	return 0;
}

static inline struct ll_scan_aux_chain *aux_chain_acquire(void)
{
	return mem_acquire(&scan_aux_free);
}

static inline void aux_chain_release(struct ll_scan_aux_chain *chain)
{
	/* Clear the parent so that when scan is being disabled then this
	 * auxiliary context shall not associate itself from being disable.
	 */
	LL_ASSERT(chain->parent);
	chain->parent = NULL;

	mem_release(chain, &scan_aux_free);
}

static void done_disabled_cb(void *param)
{
	ARG_UNUSED(param);

	while (scan_aux_set.flushing_chains) {
		flush(scan_aux_set.flushing_chains);
	}

	scan_aux_set.ull.disabled_cb = NULL;

	/* Release semaphore if it is locked */
	if (k_sem_count_get(&sem_scan_aux_stop) == 0) {
		k_sem_give(&sem_scan_aux_stop);
	}
}

static void flush_safe(void *param)
{
	struct ll_scan_aux_chain *chain;

	chain = param;
	LL_ASSERT(chain->parent);

	if (chain_is_in_list(scan_aux_set.flushing_chains, chain)) {
		/* Chain already marked for flushing */
		return;
	}

	/* If chain is active we need to flush from disabled callback */
	if (chain_is_in_list(scan_aux_set.active_chains, chain) &&
	    ull_ref_get(&scan_aux_set.ull)) {

		chain->next = scan_aux_set.flushing_chains;
		scan_aux_set.flushing_chains = chain;
		scan_aux_set.ull.disabled_cb = done_disabled_cb;
	} else {
		flush(chain);
	}
}

static void flush(struct ll_scan_aux_chain *chain)
{
	struct ll_scan_set *scan;
	struct node_rx_pdu *rx;
	struct lll_scan *lll;
	bool sched = false;

	/* Debug check that parent was assigned when allocated for reception of
	 * auxiliary channel PDUs.
	 */
	LL_ASSERT(chain->parent);

	/* Chain is being flushed now - remove from flushing_chains if present */
	chain_remove_from_list(&scan_aux_set.flushing_chains, chain);

	lll = chain->parent;
	scan = HDR_LLL2ULL(lll);
	scan = ull_scan_is_valid_get(scan);

#if defined(CONFIG_BT_CTLR_SYNC_PERIODIC)
	if (!scan && chain->aux_sched) {
		/* Send incomplete sync message */
		aux_sync_incomplete(chain);
	}
#endif /* CONFIG_BT_CTLR_SYNC_PERIODIC */

	rx = chain->rx_head;
	if (rx) {
		chain->rx_head = NULL;

		ll_rx_put(rx->hdr.link, rx);
		sched = true;
	}

#if defined(CONFIG_BT_CTLR_SYNC_PERIODIC)
	if (!scan) {
		struct ll_sync_set *sync = HDR_LLL2ULL(lll);

		rx = sync->rx_incomplete;
		if (rx) {
			sync->rx_incomplete = NULL;

			rx_release_put(rx);
			sched = true;
		}
	}
#endif /* CONFIG_BT_CTLR_SYNC_PERIODIC */

	if (sched) {
		ll_rx_sched();
	}

	if (!IS_ENABLED(CONFIG_BT_CTLR_SYNC_PERIODIC) || scan) {
		if (lll->lll_aux == &chain->lll) {
			lll->lll_aux = NULL;
		}
#if defined(CONFIG_BT_CTLR_JIT_SCHEDULING)
		lll->scan_aux_score = chain->lll.hdr.score;
#endif /* CONFIG_BT_CTLR_JIT_SCHEDULING */
	} else {
		struct lll_sync *sync_lll;
		struct ll_sync_set *sync;

		sync_lll = chain->parent;
		sync = HDR_LLL2ULL(sync_lll);

		LL_ASSERT(sync->is_stop || sync_lll->lll_aux);
		sync_lll->lll_aux = NULL;
	}

	aux_chain_release(chain);
}

#if defined(CONFIG_BT_CTLR_SYNC_PERIODIC)
static void aux_sync_incomplete(struct ll_scan_aux_chain *chain)
{
	struct ll_sync_set *sync;
	struct node_rx_pdu *rx;
	struct lll_sync *lll;

	LL_ASSERT(chain->parent);

	/* get reference to sync context */
	lll = chain->parent;
	LL_ASSERT(lll);
	sync = HDR_LLL2ULL(lll);

	/* pick extra node rx stored in sync context */
	rx = sync->rx_incomplete;
	LL_ASSERT(rx);
	sync->rx_incomplete = NULL;

	/* prepare sync report with failure */
	rx->hdr.type = NODE_RX_TYPE_SYNC_REPORT;
	rx->hdr.handle = ull_sync_handle_get(sync);
	rx->rx_ftr.param = lll;

	/* flag chain reception failure */
	rx->rx_ftr.aux_failed = 1U;

	/* Dequeue will try releasing list of node rx,
	 * set the extra pointer to NULL.
	 */
	rx->rx_ftr.extra = NULL;

	/* add to rx list, will be flushed */
	chain->rx_head = rx;
}
#endif /* CONFIG_BT_CTLR_SYNC_PERIODIC */

static void chain_start_ticker(struct ll_scan_aux_chain *chain, bool replace)
{
#if !defined(CONFIG_BT_TICKER_SLOT_AGNOSTIC)
	uint8_t ticker_yield_handle = TICKER_NULL;
#endif /* !CONFIG_BT_TICKER_SLOT_AGNOSTIC */
	uint32_t ticks_slot_overhead;
	uint32_t ticks_slot_offset;
	uint32_t ready_delay_us;
	uint8_t ticker_status;

#if !defined(CONFIG_BT_TICKER_SLOT_AGNOSTIC)
	if (ull_scan_is_valid_get(HDR_LLL2ULL(chain->parent))) {
		if (chain->rx_head == chain->rx_last) {
			struct ll_scan_set *scan = HDR_LLL2ULL(chain->parent);

			ticker_yield_handle = TICKER_ID_SCAN_BASE +
					      ull_scan_handle_get(scan);
		} else {
			ticker_yield_handle = TICKER_ID_SCAN_AUX;
		}
#if defined(CONFIG_BT_CTLR_SYNC_PERIODIC)
	} else {
		/* Periodic sync context */
		struct ll_sync_set *ull_sync = HDR_LLL2ULL(chain->parent);

		ticker_yield_handle = TICKER_ID_SCAN_SYNC_BASE +
				      ull_sync_handle_get(ull_sync);
#endif /* CONFIG_BT_CTLR_SYNC_PERIODIC */
	}
#endif /* !CONFIG_BT_TICKER_SLOT_AGNOSTIC */

	ready_delay_us = lll_radio_rx_ready_delay_get(chain->lll.phy, PHY_FLAGS_S8);

	/* TODO: active_to_start feature port */
	scan_aux_set.ull.ticks_active_to_start = 0;
	scan_aux_set.ull.ticks_prepare_to_start =
		HAL_TICKER_US_TO_TICKS(EVENT_OVERHEAD_XTAL_US);
	scan_aux_set.ull.ticks_preempt_to_start =
		HAL_TICKER_US_TO_TICKS(EVENT_OVERHEAD_PREEMPT_MIN_US);
	scan_aux_set.ull.ticks_slot = HAL_TICKER_US_TO_TICKS_CEIL(
		EVENT_OVERHEAD_START_US + ready_delay_us +
		PDU_AC_MAX_US(PDU_AC_EXT_PAYLOAD_RX_SIZE, chain->lll.phy) +
		EVENT_OVERHEAD_END_US);

	ticks_slot_offset = MAX(scan_aux_set.ull.ticks_active_to_start,
				scan_aux_set.ull.ticks_prepare_to_start);
	if (IS_ENABLED(CONFIG_BT_CTLR_LOW_LAT)) {
		ticks_slot_overhead = ticks_slot_offset;
	} else {
		ticks_slot_overhead = 0U;
	}
	ticks_slot_offset += HAL_TICKER_US_TO_TICKS(EVENT_OVERHEAD_START_US);

#if (CONFIG_BT_CTLR_ULL_HIGH_PRIO == CONFIG_BT_CTLR_ULL_LOW_PRIO)
	/* disable ticker job, in order to chain yield and start to reduce
	 * CPU use by reducing successive calls to ticker_job().
	 */
	mayfly_enable(TICKER_USER_ID_ULL_HIGH, TICKER_USER_ID_ULL_LOW, 0);
#endif

#if !defined(CONFIG_BT_TICKER_SLOT_AGNOSTIC)
	/* Yield the primary scan window or auxiliary or periodic sync event
	 * in ticker.
	 */
	if (ticker_yield_handle != TICKER_NULL) {
		ticker_status = ticker_yield_abs(TICKER_INSTANCE_ID_CTLR,
						 TICKER_USER_ID_ULL_HIGH,
						 ticker_yield_handle,
						 (chain->ticker_ticks -
						  ticks_slot_offset),
						 NULL, NULL);
		LL_ASSERT((ticker_status == TICKER_STATUS_SUCCESS) ||
			  (ticker_status == TICKER_STATUS_BUSY));
	}
#endif /* !CONFIG_BT_TICKER_SLOT_AGNOSTIC */

	if (replace) {
		ticker_status = ticker_stop(TICKER_INSTANCE_ID_CTLR, TICKER_USER_ID_ULL_HIGH,
				    TICKER_ID_SCAN_AUX, NULL, NULL);
		LL_ASSERT((ticker_status == TICKER_STATUS_SUCCESS) ||
			  (ticker_status == TICKER_STATUS_BUSY));
	}

	ticker_status = ticker_start(TICKER_INSTANCE_ID_CTLR,
				     TICKER_USER_ID_ULL_HIGH,
				     TICKER_ID_SCAN_AUX,
				     chain->ticker_ticks - ticks_slot_offset,
				     0,
				     TICKER_NULL_PERIOD,
				     TICKER_NULL_REMAINDER,
				     TICKER_NULL_LAZY,
				     (scan_aux_set.ull.ticks_slot +
				      ticks_slot_overhead),
				     ticker_cb, chain, ticker_op_cb, chain);
#if defined(CONFIG_BT_TICKER_SLOT_AGNOSTIC)
	LL_ASSERT((ticker_status == TICKER_STATUS_SUCCESS) ||
		  (ticker_status == TICKER_STATUS_BUSY));
#else
	LL_ASSERT((ticker_status == TICKER_STATUS_SUCCESS) ||
		  (ticker_status == TICKER_STATUS_BUSY) ||
		  ((ticker_status == TICKER_STATUS_FAILURE) &&
		   IS_ENABLED(CONFIG_BT_TICKER_LOW_LAT)));
#endif /* !CONFIG_BT_TICKER_SLOT_AGNOSTIC */

#if (CONFIG_BT_CTLR_ULL_HIGH_PRIO == CONFIG_BT_CTLR_ULL_LOW_PRIO)
	/* enable ticker job, queued ticker operation will be handled
	 * thereafter.
	 */
	mayfly_enable(TICKER_USER_ID_ULL_HIGH, TICKER_USER_ID_ULL_LOW, 1);
#endif
}

static void ticker_cb(uint32_t ticks_at_expire, uint32_t ticks_drift,
		      uint32_t remainder, uint16_t lazy, uint8_t force,
		      void *param)
{
	static memq_link_t link;
	static struct mayfly mfy = {0, 0, &link, NULL, lll_scan_aux_prepare};
	struct ll_scan_aux_chain *chain = param;
	static struct lll_prepare_param p;
	uint32_t ret;
	uint8_t ref;

	DEBUG_RADIO_PREPARE_O(1);

	/* Increment prepare reference count */
	ref = ull_ref_inc(&scan_aux_set.ull);
	LL_ASSERT(ref);

	/* The chain should always be the first in the sched_chains list */
	LL_ASSERT(scan_aux_set.sched_chains == chain);

	/* Move chain to active list */
	chain_remove_from_list(&scan_aux_set.sched_chains, chain);
	chain_append_to_list(&scan_aux_set.active_chains, chain);

	/* Append timing parameters */
	p.ticks_at_expire = ticks_at_expire;
	p.remainder = 0; /* FIXME: remainder; */
	p.lazy = lazy;
	p.force = force;
	p.param = &chain->lll;
	mfy.param = &p;

	/* Kick LLL prepare */
	ret = mayfly_enqueue(TICKER_USER_ID_ULL_HIGH, TICKER_USER_ID_LLL,
			     0, &mfy);
	LL_ASSERT(!ret);

	if (scan_aux_set.sched_chains) {
		/* Start ticker for next chain */
		chain_start_ticker(scan_aux_set.sched_chains, false);
	}

	DEBUG_RADIO_PREPARE_O(1);
}

#if !defined(CONFIG_BT_TICKER_SLOT_AGNOSTIC)
static void ticker_start_failed(void *param)
{
	struct ll_scan_aux_chain *chain;

	/* Ticker start failed, so remove this chain from scheduled chains */
	chain = param;
	chain_remove_from_list(&scan_aux_set.sched_chains, chain);

	flush(chain);

	if (scan_aux_set.sched_chains) {
		chain_start_ticker(scan_aux_set.sched_chains, false);
	}
}
#endif /* !CONFIG_BT_TICKER_SLOT_AGNOSTIC */

static void ticker_op_cb(uint32_t status, void *param)
{
#if defined(CONFIG_BT_TICKER_SLOT_AGNOSTIC)
	LL_ASSERT(status == TICKER_STATUS_SUCCESS);
#else /* !CONFIG_BT_TICKER_SLOT_AGNOSTIC */
	static memq_link_t link;
	static struct mayfly mfy = {0, 0, &link, NULL, ticker_start_failed};
	uint32_t ret;

	if (status == TICKER_STATUS_SUCCESS) {
		return;
	}

	mfy.param = param;

	ret = mayfly_enqueue(TICKER_USER_ID_ULL_LOW, TICKER_USER_ID_ULL_HIGH,
			     1, &mfy);
	LL_ASSERT(!ret);
#endif /* !CONFIG_BT_TICKER_SLOT_AGNOSTIC */
}

static int32_t chain_ticker_ticks_diff(uint32_t ticks_a, uint32_t ticks_b)
{
	if ((ticks_a - ticks_b) & BIT(HAL_TICKER_CNTR_MSBIT)) {
		return -ticker_ticks_diff_get(ticks_b, ticks_a);
	} else {
		return ticker_ticks_diff_get(ticks_a, ticks_b);
	}
}

/* Sorted insertion into sched list, starting/replacing the ticker when needed
 * Returns:
 *  - false for no insertion (conflict with existing entry)
 *  - true for inserted
 */
static bool chain_insert_in_sched_list(struct ll_scan_aux_chain *chain)
{
	struct ll_scan_aux_chain *curr = scan_aux_set.sched_chains;
	struct ll_scan_aux_chain *prev = NULL;
	uint32_t ticks_min_delta;

	if (!scan_aux_set.sched_chains) {
		chain->next = NULL;
		scan_aux_set.sched_chains = chain;
		chain_start_ticker(chain, false);
		return true;
	}

	/* Find insertion point */
	while (curr && chain_ticker_ticks_diff(chain->ticker_ticks, curr->ticker_ticks) > 0) {
		prev = curr;
		curr = curr->next;
	}

	/* Check for conflict with existing entry */
	ticks_min_delta = HAL_TICKER_US_TO_TICKS(EVENT_OVERHEAD_END_US + EVENT_OVERHEAD_START_US);
	if ((prev &&
	    ticker_ticks_diff_get(chain->ticker_ticks, prev->ticker_ticks) < ticks_min_delta) ||
	    (curr &&
	    ticker_ticks_diff_get(curr->ticker_ticks, chain->ticker_ticks) < ticks_min_delta)) {
		return false;
	}

	if (prev) {
		chain->next = prev->next;
		prev->next = chain;
	} else {
		chain->next = scan_aux_set.sched_chains;
		scan_aux_set.sched_chains = chain;
		chain_start_ticker(chain, true);
	}

	return true;
}

static void chain_remove_from_list(struct ll_scan_aux_chain **head, struct ll_scan_aux_chain *chain)
{
	struct ll_scan_aux_chain *curr = *head;
	struct ll_scan_aux_chain *prev = NULL;

	while (curr && curr != chain) {
		prev = curr;
		curr = curr->next;
	}

	if (curr) {
		if (prev) {
			prev->next = curr->next;
		} else {
			*head = curr->next;
		}
	}

	chain->next = NULL;
}

static void chain_append_to_list(struct ll_scan_aux_chain **head, struct ll_scan_aux_chain *chain)
{
	struct ll_scan_aux_chain *prev = *head;

	if (!*head) {
		chain->next = NULL;
		*head = chain;
		return;
	}

	while (prev->next) {
		prev = prev->next;
	}

	prev->next = chain;
}

static bool chain_is_in_list(struct ll_scan_aux_chain *head, struct ll_scan_aux_chain *chain)
{
	while (head) {
		if (head == chain) {
			return true;
		}
		head = head->next;
	}
	return false;
}
#endif /* CONFIG_BT_CTLR_SCAN_AUX_USE_CHAINS */
