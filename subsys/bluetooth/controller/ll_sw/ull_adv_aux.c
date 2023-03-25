/*
 * Copyright (c) 2017-2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <soc.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/sys/byteorder.h>

#include "hal/cpu.h"
#include "hal/ccm.h"
#include "hal/ticker.h"

#include "util/util.h"
#include "util/mem.h"
#include "util/memq.h"
#include "util/mayfly.h"
#include "util/dbuf.h"

#include "ticker/ticker.h"

#include "pdu_df.h"
#include "lll/pdu_vendor.h"
#include "pdu.h"

#include "lll.h"
#include "lll_clock.h"
#include "lll/lll_vendor.h"
#include "lll_chan.h"
#include "lll/lll_adv_types.h"
#include "lll_adv.h"
#include "lll/lll_adv_pdu.h"
#include "lll_adv_aux.h"
#include "lll/lll_df_types.h"
#include "lll_conn.h"

#include "ull_adv_types.h"

#include "ull_internal.h"
#include "ull_chan_internal.h"
#include "ull_adv_internal.h"
#include "ull_sched_internal.h"

#include "ll.h"

#include "hal/debug.h"

static int init_reset(void);

#if (CONFIG_BT_CTLR_ADV_AUX_SET > 0)
static inline struct ll_adv_aux_set *aux_acquire(void);
static inline void aux_release(struct ll_adv_aux_set *aux);
static uint32_t aux_time_get(const struct ll_adv_aux_set *aux,
			     const struct pdu_adv *pdu,
			     uint8_t pdu_len, uint8_t pdu_scan_len);
static uint32_t aux_time_min_get(const struct ll_adv_aux_set *aux);
static uint8_t aux_time_update(struct ll_adv_aux_set *aux, struct pdu_adv *pdu,
			       struct pdu_adv *pdu_scan);
static void ticker_cb(uint32_t ticks_at_expire, uint32_t ticks_drift,
		      uint32_t remainder, uint16_t lazy, uint8_t force,
		      void *param);

static struct ll_adv_aux_set ll_adv_aux_pool[CONFIG_BT_CTLR_ADV_AUX_SET];
static void *adv_aux_free;

#if defined(CONFIG_BT_CTLR_ADV_PERIODIC)
static struct ticker_ext ll_adv_aux_ticker_ext[CONFIG_BT_CTLR_ADV_AUX_SET];
#endif /* CONFIG_BT_CTLR_ADV_PERIODIC */
#endif /* (CONFIG_BT_CTLR_ADV_AUX_SET > 0) */

static uint16_t did_unique[PDU_ADV_SID_COUNT];

uint8_t ll_adv_aux_random_addr_set(uint8_t handle, uint8_t const *const addr)
{
	struct ll_adv_set *adv;

	adv = ull_adv_is_created_get(handle);
	if (!adv) {
		return BT_HCI_ERR_UNKNOWN_ADV_IDENTIFIER;
	}

	/* TODO: Fail if connectable advertising is enabled */
	if (0) {
		return BT_HCI_ERR_CMD_DISALLOWED;
	}

	(void)memcpy(adv->rnd_addr, addr, BDADDR_SIZE);

	return 0;
}

uint8_t ll_adv_aux_ad_data_set(uint8_t handle, uint8_t op, uint8_t frag_pref,
			       uint8_t len, uint8_t const *const data)
{
	uint8_t hdr_data[ULL_ADV_HDR_DATA_LEN_SIZE +
			 ULL_ADV_HDR_DATA_DATA_PTR_SIZE +
			 ULL_ADV_HDR_DATA_LEN_SIZE +
			 ULL_ADV_HDR_DATA_AUX_PTR_PTR_SIZE +
			 ULL_ADV_HDR_DATA_LEN_SIZE];
	uint8_t pri_idx, sec_idx;
	struct ll_adv_set *adv;
	uint8_t *val_ptr;
	uint8_t err;

#if defined(CONFIG_BT_CTLR_ADV_AUX_PDU_LINK)
	struct pdu_adv *pdu_prev;
	uint8_t ad_len_overflow;
	uint8_t ad_len_chain;
	struct pdu_adv *pdu;
#endif /* CONFIG_BT_CTLR_ADV_AUX_PDU_LINK */

	/* Get the advertising set instance */
	adv = ull_adv_is_created_get(handle);
	if (!adv) {
		return BT_HCI_ERR_UNKNOWN_ADV_IDENTIFIER;
	}

	/* Reject setting fragment when Extended Advertising is enabled */
	if (adv->is_enabled && (op <= BT_HCI_LE_EXT_ADV_OP_LAST_FRAG)) {
		return BT_HCI_ERR_CMD_DISALLOWED;
	}

	/* Reject intermediate op before first op */
	if (adv->is_ad_data_cmplt &&
	    ((op == BT_HCI_LE_EXT_ADV_OP_INTERM_FRAG) ||
	     (op == BT_HCI_LE_EXT_ADV_OP_LAST_FRAG))) {
		return BT_HCI_ERR_CMD_DISALLOWED;
	}

	/* Reject unchanged op before complete status */
	if (!adv->is_ad_data_cmplt &&
	    (op == BT_HCI_LE_EXT_ADV_OP_UNCHANGED_DATA)) {
		return BT_HCI_ERR_CMD_DISALLOWED;
	}

	/* Reject len > 191 bytes if chain PDUs unsupported */
	if (!IS_ENABLED(CONFIG_BT_CTLR_ADV_AUX_PDU_LINK) &&
	    (len > PDU_AC_EXT_AD_DATA_LEN_MAX)) {
		return BT_HCI_ERR_CMD_DISALLOWED;
	}

	/* Prepare the AD data as parameter to update in PDU */
	/* Use length = 0 and NULL pointer to retain old data in the PDU.
	 * Use length = 0 and valid pointer of `data` (auto/local variable) to
	 * remove old data.
	 * User length > 0 and valid pointer of `data` (auto/local variable) to
	 * set new data.
	 */
	val_ptr = hdr_data;
	if ((IS_ENABLED(CONFIG_BT_CTLR_ADV_AUX_PDU_LINK) && (
	     op == BT_HCI_LE_EXT_ADV_OP_INTERM_FRAG ||
	     op == BT_HCI_LE_EXT_ADV_OP_LAST_FRAG)) ||
	    op == BT_HCI_LE_EXT_ADV_OP_UNCHANGED_DATA) {
		*val_ptr++ = 0U;
		(void)memset((void *)val_ptr, 0U,
			     ULL_ADV_HDR_DATA_DATA_PTR_SIZE);
	} else {
		*val_ptr++ = len;
		(void)memcpy(val_ptr, &data, sizeof(data));
	}

	if ((!IS_ENABLED(CONFIG_BT_CTLR_ADV_AUX_PDU_LINK) &&
	     (op == BT_HCI_LE_EXT_ADV_OP_COMPLETE_DATA ||
	      op == BT_HCI_LE_EXT_ADV_OP_FIRST_FRAG)) ||
	    (op == BT_HCI_LE_EXT_ADV_OP_UNCHANGED_DATA)) {
		err = ull_adv_aux_hdr_set_clear(adv,
						ULL_ADV_PDU_HDR_FIELD_AD_DATA,
						0U, hdr_data, &pri_idx,
						&sec_idx);
#if defined(CONFIG_BT_CTLR_ADV_AUX_PDU_LINK)
		/* No AD data overflow */
		ad_len_overflow = 0U;

		/* No AD data in chain PDU */
		ad_len_chain = 0U;

		/* local variables not used due to overflow being 0 */
		pdu_prev = NULL;
		pdu = NULL;
	} else if (op == BT_HCI_LE_EXT_ADV_OP_FIRST_FRAG ||
		    op == BT_HCI_LE_EXT_ADV_OP_COMPLETE_DATA) {
		/* Add AD Data and remove any prior presence of Aux Ptr */
		err = ull_adv_aux_hdr_set_clear(adv,
						ULL_ADV_PDU_HDR_FIELD_AD_DATA,
						ULL_ADV_PDU_HDR_FIELD_AUX_PTR,
						hdr_data, &pri_idx, &sec_idx);
		if (err == BT_HCI_ERR_PACKET_TOO_LONG) {
			ad_len_overflow =
				hdr_data[ULL_ADV_HDR_DATA_DATA_PTR_OFFSET +
					 ULL_ADV_HDR_DATA_DATA_PTR_SIZE];
			/* Prepare the AD data as parameter to update in
			 * PDU
			 */
			val_ptr = hdr_data;
			*val_ptr++ = len - ad_len_overflow;
			(void)memcpy(val_ptr, &data, sizeof(data));

			err = ull_adv_aux_hdr_set_clear(adv,
						ULL_ADV_PDU_HDR_FIELD_AD_DATA,
						ULL_ADV_PDU_HDR_FIELD_AUX_PTR,
						hdr_data, &pri_idx, &sec_idx);
		}

		if (!err && adv->lll.aux) {
			/* Fragment into chain PDU if len > 191 bytes */
			if (len > PDU_AC_EXT_AD_DATA_LEN_MAX) {
				uint8_t idx;

				/* Prepare the AD data as parameter to update in
				 * PDU
				 */
				val_ptr = hdr_data;
				*val_ptr++ = PDU_AC_EXT_AD_DATA_LEN_MAX;
				(void)memcpy(val_ptr, &data, sizeof(data));

				/* Traverse to next set clear hdr data
				 * parameter, as aux ptr reference to be
				 * returned, hence second parameter will be for
				 * AD data field.
				 */
				val_ptr += sizeof(data);

				*val_ptr = PDU_AC_EXT_AD_DATA_LEN_MAX;
				(void)memcpy(&val_ptr[ULL_ADV_HDR_DATA_DATA_PTR_OFFSET],
					     &data, sizeof(data));

				/* Calculate the overflow chain PDU's AD data
				 * length
				 */
				ad_len_overflow =
					len - PDU_AC_EXT_AD_DATA_LEN_MAX;

				/* No AD data in chain PDU besides the
				 * overflow
				 */
				ad_len_chain = 0U;

				/* Get the reference to auxiliary PDU chain */
				pdu_prev = lll_adv_aux_data_alloc(adv->lll.aux,
								  &idx);
				LL_ASSERT(idx == sec_idx);

				/* Current auxiliary PDU */
				pdu = pdu_prev;
			} else {
				struct pdu_adv *pdu_parent;
				struct pdu_adv *pdu_chain;
				uint8_t idx;

				/* Get the reference to auxiliary PDU chain */
				pdu_parent =
					lll_adv_aux_data_alloc(adv->lll.aux,
							       &idx);
				LL_ASSERT(idx == sec_idx);

				/* Remove/Release any previous chain PDU
				 * allocations
				 */
				pdu_chain =
					lll_adv_pdu_linked_next_get(pdu_parent);
				if (pdu_chain) {
					lll_adv_pdu_linked_append(NULL,
								  pdu_parent);
					lll_adv_pdu_linked_release_all(pdu_chain);
				}

				/* No AD data overflow */
				ad_len_overflow = 0U;

				/* No AD data in chain PDU */
				ad_len_chain = 0U;

				/* local variables not used due to overflow
				 * being 0
				 */
				pdu_prev = NULL;
				pdu = NULL;
			}
		} else {
			/* No AD data overflow */
			ad_len_overflow = 0U;

			/* No AD data in chain PDU */
			ad_len_chain = 0U;

			/* local variables not used due to overflow being 0 */
			pdu_prev = NULL;
			pdu = NULL;
		}
	} else {
		struct pdu_adv *pdu_chain_prev;
		struct pdu_adv *pdu_chain;
		uint16_t ad_len_total;
		uint8_t ad_len_prev;

		/* Traverse to next set clear hdr data parameter */
		val_ptr += sizeof(data);

		/* Get reference to previous secondary PDU data */
		pdu_prev = lll_adv_aux_data_peek(adv->lll.aux);

		/* Get the reference to auxiliary PDU chain */
		pdu = lll_adv_aux_data_alloc(adv->lll.aux,
					     &sec_idx);

		/* Traverse to the last chain PDU */
		ad_len_total = 0U;
		pdu_chain_prev = pdu_prev;
		pdu_chain = pdu;
		do {
			/* Prepare for aux ptr field reference to be returned, hence
			 * second parameter will be for AD data field.
			 */
			*val_ptr = 0U;
			(void)memset((void *)&val_ptr[ULL_ADV_HDR_DATA_DATA_PTR_OFFSET],
				     0U, ULL_ADV_HDR_DATA_DATA_PTR_SIZE);

			pdu_prev = pdu_chain_prev;
			pdu = pdu_chain;

			/* Add Aux Ptr field if not already present */
			err = ull_adv_aux_pdu_set_clear(adv, pdu_prev, pdu,
						(ULL_ADV_PDU_HDR_FIELD_AD_DATA |
						 ULL_ADV_PDU_HDR_FIELD_AUX_PTR),
						0, hdr_data);
			LL_ASSERT(!err || (err == BT_HCI_ERR_PACKET_TOO_LONG));

			/* Get PDUs previous AD data length */
			ad_len_prev =
				hdr_data[ULL_ADV_HDR_DATA_AUX_PTR_PTR_OFFSET +
					 ULL_ADV_HDR_DATA_AUX_PTR_PTR_SIZE];

			/* Check of max supported AD data len */
			ad_len_total += ad_len_prev;
			if ((ad_len_total + len) >
			    CONFIG_BT_CTLR_ADV_DATA_LEN_MAX) {
				/* NOTE: latest PDU was not consumed by LLL and
				 * as ull_adv_sync_pdu_alloc() has reverted back
				 * the double buffer with the first PDU, and
				 * returned the latest PDU as the new PDU, we
				 * need to enqueue back the new PDU which is
				 * infact the latest PDU.
				 */
				if (pdu_prev == pdu) {
					lll_adv_aux_data_enqueue(adv->lll.aux,
								 sec_idx);
				}

				return BT_HCI_ERR_MEM_CAPACITY_EXCEEDED;
			}

			pdu_chain_prev = lll_adv_pdu_linked_next_get(pdu_prev);
			pdu_chain = lll_adv_pdu_linked_next_get(pdu);
			LL_ASSERT((pdu_chain_prev && pdu_chain) ||
				  (!pdu_chain_prev && !pdu_chain));
		} while (pdu_chain_prev);

		if (err == BT_HCI_ERR_PACKET_TOO_LONG) {
			ad_len_overflow =
				hdr_data[ULL_ADV_HDR_DATA_AUX_PTR_PTR_OFFSET +
					 ULL_ADV_HDR_DATA_AUX_PTR_PTR_SIZE +
					 ULL_ADV_HDR_DATA_DATA_PTR_OFFSET +
					 ULL_ADV_HDR_DATA_DATA_PTR_SIZE];

			/* Prepare for aux ptr field reference to be returned,
			 * hence second parameter will be for AD data field.
			 * Fill it with reduced AD data length.
			 */
			*val_ptr = ad_len_prev - ad_len_overflow;

			/* AD data len in chain PDU */
			ad_len_chain = len;

			/* Proceed to add chain PDU */
			err = 0U;
		} else {
			/* No AD data overflow */
			ad_len_overflow = 0U;

			/* No AD data in chain PDU */
			ad_len_chain = 0U;
		}
	}
#else /* !CONFIG_BT_CTLR_ADV_AUX_PDU_LINK */
	} else {
		/* Append new fragment */
		err = ull_adv_aux_hdr_set_clear(adv,
						ULL_ADV_PDU_HDR_FIELD_AD_DATA_APPEND,
						0U, hdr_data, &pri_idx,
						&sec_idx);
	}
#endif /* !CONFIG_BT_CTLR_ADV_AUX_PDU_LINK */

	if (err) {
		return err;
	}

	if (!adv->lll.aux) {
		return 0;
	}

#if defined(CONFIG_BT_CTLR_ADV_AUX_PDU_LINK)
	if ((op == BT_HCI_LE_EXT_ADV_OP_INTERM_FRAG) ||
	    (op == BT_HCI_LE_EXT_ADV_OP_LAST_FRAG) ||
	    ad_len_overflow) {
		struct pdu_adv_com_ext_adv *com_hdr_chain;
		struct pdu_adv_com_ext_adv *com_hdr;
		struct pdu_adv_ext_hdr *hdr_chain;
		struct pdu_adv_aux_ptr *aux_ptr;
		struct pdu_adv *pdu_chain_prev;
		struct pdu_adv_ext_hdr hdr;
		struct pdu_adv *pdu_chain;
		uint8_t *dptr_chain;
		uint32_t offs_us;
		uint16_t sec_len;
		uint8_t *dptr;

		/* Get reference to flags in superior PDU */
		com_hdr = &pdu->adv_ext_ind;
		if (com_hdr->ext_hdr_len) {
			hdr = com_hdr->ext_hdr;
		} else {
			*(uint8_t *)&hdr = 0U;
		}
		dptr = com_hdr->ext_hdr.data;

		/* Allocate new PDU */
		pdu_chain = lll_adv_pdu_alloc_pdu_adv();
		LL_ASSERT(pdu_chain);

		/* Populate the appended chain PDU */
		pdu_chain->type = PDU_ADV_TYPE_AUX_CHAIN_IND;
		pdu_chain->rfu = 0U;
		pdu_chain->chan_sel = 0U;
		pdu_chain->tx_addr = 0U;
		pdu_chain->rx_addr = 0U;
		pdu_chain->len = 0U;

		com_hdr_chain = &pdu_chain->adv_ext_ind;
		hdr_chain = (void *)&com_hdr_chain->ext_hdr_adv_data[0];
		dptr_chain = (void *)hdr_chain;

		/* Flags */
		*dptr_chain = 0U;

		/* ADI flag, mandatory if superior PDU has it */
		if (hdr.adi) {
			hdr_chain->adi = 1U;
		}

		/* Proceed to next byte if any flags present */
		if (*dptr_chain) {
			dptr_chain++;
		}

		/* Start adding fields corresponding to flags here, if any */

		/* AdvA flag */
		if (hdr.adv_addr) {
			dptr += BDADDR_SIZE;
		}

		/* TgtA flag */
		if (hdr.tgt_addr) {
			dptr += BDADDR_SIZE;
		}

		/* No CTEInfo in Extended Advertising */

		/* ADI flag */
		if (hdr_chain->adi) {
			(void)memcpy(dptr_chain, dptr,
				     sizeof(struct pdu_adv_adi));

			dptr += sizeof(struct pdu_adv_adi);
			dptr_chain += sizeof(struct pdu_adv_adi);
		}

		/* non-connectable non-scannable chain pdu */
		com_hdr_chain->adv_mode = 0;

		/* Calc current chain PDU len */
		sec_len = ull_adv_aux_hdr_len_calc(com_hdr_chain, &dptr_chain);

		/* Prefix overflowed data to chain PDU and reduce the AD data in
		 * in the current PDU.
		 */
		if (ad_len_overflow) {
			uint8_t *ad_overflow;

			/* Copy overflowed AD data from previous PDU into this
			 * new chain PDU
			 */
			(void)memcpy(&ad_overflow,
				     &val_ptr[ULL_ADV_HDR_DATA_DATA_PTR_OFFSET],
				     sizeof(ad_overflow));
			ad_overflow += *val_ptr;
			(void)memcpy(dptr_chain, ad_overflow, ad_len_overflow);
			dptr_chain += ad_len_overflow;

			/* Reduce the AD data in the previous PDU */
			err = ull_adv_aux_pdu_set_clear(adv, pdu_prev, pdu,
						(ULL_ADV_PDU_HDR_FIELD_AD_DATA |
						 ULL_ADV_PDU_HDR_FIELD_AUX_PTR),
						0, hdr_data);
			if (err) {
				/* NOTE: latest PDU was not consumed by LLL and
				 * as ull_adv_sync_pdu_alloc() has reverted back
				 * the double buffer with the first PDU, and
				 * returned the latest PDU as the new PDU, we
				 * need to enqueue back the new PDU which is
				 * infact the latest PDU.
				 */
				if (pdu_prev == pdu) {
					lll_adv_aux_data_enqueue(adv->lll.aux,
								 sec_idx);
				}

				return err;
			}

			/* AD data len in chain PDU besides the overflow */
			len = ad_len_chain;
		}

		/* Check AdvData overflow */
		if ((sec_len + ad_len_overflow + len) >
		    PDU_AC_PAYLOAD_SIZE_MAX) {
			/* NOTE: latest PDU was not consumed by LLL and
			 * as ull_adv_sync_pdu_alloc() has reverted back
			 * the double buffer with the first PDU, and
			 * returned the latest PDU as the new PDU, we
			 * need to enqueue back the new PDU which is
			 * infact the latest PDU.
			 */
			if (pdu_prev == pdu) {
				lll_adv_aux_data_enqueue(adv->lll.aux, sec_idx);
			}

			return BT_HCI_ERR_PACKET_TOO_LONG;
		}

		/* Fill the chain PDU length */
		ull_adv_aux_hdr_len_fill(com_hdr_chain, sec_len);
		pdu_chain->len = sec_len + ad_len_overflow + len;

		/* Fill AD Data in chain PDU */
		(void)memcpy(dptr_chain, data, len);

		/* Get reference to aux ptr in superior PDU */
		(void)memcpy(&aux_ptr,
			     &hdr_data[ULL_ADV_HDR_DATA_AUX_PTR_PTR_OFFSET],
			     sizeof(aux_ptr));

		/* Fill the aux offset in the previous AUX_SYNC_IND PDU */
		offs_us = PDU_AC_US(pdu->len, adv->lll.phy_s,
				    adv->lll.phy_flags) +
			  EVENT_B2B_MAFS_US;
		ull_adv_aux_ptr_fill(aux_ptr, offs_us, adv->lll.phy_s);

		/* Remove/Release any previous chain PDUs */
		pdu_chain_prev = lll_adv_pdu_linked_next_get(pdu);
		if (pdu_chain_prev) {
			lll_adv_pdu_linked_append(NULL, pdu);
			lll_adv_pdu_linked_release_all(pdu_chain_prev);
		}

		/* Chain the PDU */
		lll_adv_pdu_linked_append(pdu_chain, pdu);
	}
#endif /* CONFIG_BT_CTLR_ADV_AUX_PDU_LINK */

	if (adv->is_enabled) {
		struct ll_adv_aux_set *aux;
		struct pdu_adv *pdu;
		uint8_t tmp_idx;

		aux = HDR_LLL2ULL(adv->lll.aux);
		if (!aux->is_started) {
			uint32_t ticks_slot_overhead;
			uint32_t ticks_anchor;
			uint32_t ret;

			/* Keep aux interval equal or higher than primary PDU
			 * interval.
			 * Use periodic interval units to represent the
			 * periodic behavior of scheduling of AUX_ADV_IND PDUs
			 * so that it is grouped with similar interval units
			 * used for ACL Connections, Periodic Advertising and
			 * BIG radio events.
			 */
			aux->interval =
				DIV_ROUND_UP(((uint64_t)adv->interval *
						  ADV_INT_UNIT_US) +
						 HAL_TICKER_TICKS_TO_US(
							ULL_ADV_RANDOM_DELAY),
						 PERIODIC_INT_UNIT_US);

			/* TODO: Find the anchor before the group of
			 *       active Periodic Advertising events, so
			 *       that auxiliary sets are grouped such
			 *       that auxiliary sets and Periodic
			 *       Advertising sets are non-overlapping
			 *       for the same event interval.
			 */
			ticks_anchor =
				ticker_ticks_now_get() +
				HAL_TICKER_US_TO_TICKS(EVENT_OVERHEAD_START_US);

			ticks_slot_overhead =
				ull_adv_aux_evt_init(aux, &ticks_anchor);

			ret = ull_adv_aux_start(aux, ticks_anchor,
						ticks_slot_overhead);
			if (ret) {
				/* NOTE: This failure, to start an auxiliary
				 * channel radio event shall not occur unless
				 * a defect in the controller design.
				 */
				return BT_HCI_ERR_INSUFFICIENT_RESOURCES;
			}

			aux->is_started = 1;
		}

		/* Update primary channel reservation */
		pdu = lll_adv_data_alloc(&adv->lll, &tmp_idx);
		err = ull_adv_time_update(adv, pdu, NULL);
		if (err) {
			return err;
		}

		ARG_UNUSED(tmp_idx);
	}

	lll_adv_aux_data_enqueue(adv->lll.aux, sec_idx);

	if (!IS_ENABLED(CONFIG_BT_CTLR_ADV_AUX_PDU_LINK) ||
	    op == BT_HCI_LE_EXT_ADV_OP_FIRST_FRAG ||
	    op == BT_HCI_LE_EXT_ADV_OP_COMPLETE_DATA ||
	    op == BT_HCI_LE_EXT_ADV_OP_UNCHANGED_DATA) {
		lll_adv_data_enqueue(&adv->lll, pri_idx);
	}

	/* Check if Extended Advertising Data is complete */
	adv->is_ad_data_cmplt = (op >= BT_HCI_LE_EXT_ADV_OP_LAST_FRAG);

	return 0;
}

uint8_t ll_adv_aux_sr_data_set(uint8_t handle, uint8_t op, uint8_t frag_pref,
			       uint8_t len, uint8_t const *const data)
{
	uint8_t hdr_data[ULL_ADV_HDR_DATA_LEN_SIZE +
			 ULL_ADV_HDR_DATA_ADI_PTR_SIZE +
			 ULL_ADV_HDR_DATA_LEN_SIZE +
			 ULL_ADV_HDR_DATA_AUX_PTR_PTR_SIZE +
			 ULL_ADV_HDR_DATA_LEN_SIZE +
			 ULL_ADV_HDR_DATA_DATA_PTR_SIZE +
			 ULL_ADV_HDR_DATA_LEN_SIZE];
	struct pdu_adv *pri_pdu_prev;
	struct pdu_adv *sec_pdu_prev;
	struct pdu_adv *sr_pdu_prev;
	uint8_t pri_idx, sec_idx;
	uint16_t hdr_add_fields;
	struct ll_adv_set *adv;
	struct pdu_adv *sr_pdu;
	struct lll_adv *lll;
	uint8_t *val_ptr;
	uint8_t sr_idx;
	uint8_t err;

#if defined(CONFIG_BT_CTLR_ADV_AUX_PDU_LINK)
	uint8_t ad_len_overflow;
	uint8_t ad_len_chain;
#endif /* CONFIG_BT_CTLR_ADV_AUX_PDU_LINK */

	/* Get the advertising set instance */
	adv = ull_adv_is_created_get(handle);
	if (!adv) {
		return BT_HCI_ERR_UNKNOWN_ADV_IDENTIFIER;
	}

	/* Do not use Common Extended Advertising Header Format if not extended
	 * advertising.
	 */
	lll = &adv->lll;
	pri_pdu_prev = lll_adv_data_peek(lll);
	if (pri_pdu_prev->type != PDU_ADV_TYPE_EXT_IND) {
		if ((op != BT_HCI_LE_EXT_ADV_OP_COMPLETE_DATA) ||
		    (len > PDU_AC_LEG_DATA_SIZE_MAX)) {
			return BT_HCI_ERR_INVALID_PARAM;
		}
		return ull_scan_rsp_set(adv, len, data);
	}

	/* Can only set complete data if advertising is enabled */
	if (adv->is_enabled && (op != BT_HCI_LE_EXT_ADV_OP_COMPLETE_DATA)) {
		return BT_HCI_ERR_CMD_DISALLOWED;
	}

	/* Data can be discarded only using 0x03 op */
	if ((op != BT_HCI_LE_EXT_ADV_OP_COMPLETE_DATA) && !len) {
		return BT_HCI_ERR_INVALID_PARAM;
	}

	/* Scannable advertising shall have aux context allocated */
	LL_ASSERT(lll->aux);

	/* Get reference to previous secondary channel PDU */
	sec_pdu_prev = lll_adv_aux_data_peek(lll->aux);

	/* Can only discard data on non-scannable instances */
	if (!(sec_pdu_prev->adv_ext_ind.adv_mode & BT_HCI_LE_ADV_PROP_SCAN) &&
	    len) {
		return BT_HCI_ERR_INVALID_PARAM;
	}

	/* Cannot discard scan response if scannable advertising is enabled */
	if (adv->is_enabled &&
	    (sec_pdu_prev->adv_ext_ind.adv_mode & BT_HCI_LE_ADV_PROP_SCAN) &&
	    !len) {
		return BT_HCI_ERR_CMD_DISALLOWED;
	}

	/* Get reference to previous scan response PDU */
	sr_pdu_prev = lll_adv_scan_rsp_peek(lll);

	/* Get reference to next scan response  PDU */
	sr_pdu = lll_adv_aux_scan_rsp_alloc(lll, &sr_idx);

	/* Prepare the AD data as parameter to update in PDU */
	/* Use length = 0 and NULL pointer to retain old data in the PDU.
	 * Use length = 0 and valid pointer of `data` (auto/local variable) to
	 * remove old data.
	 * User length > 0 and valid pointer of `data` (auto/local variable) to
	 * set new data.
	 */
	val_ptr = hdr_data;
	if ((IS_ENABLED(CONFIG_BT_CTLR_ADV_AUX_PDU_LINK) && (
	     op == BT_HCI_LE_EXT_ADV_OP_INTERM_FRAG ||
	     op == BT_HCI_LE_EXT_ADV_OP_LAST_FRAG)) ||
	    op == BT_HCI_LE_EXT_ADV_OP_UNCHANGED_DATA) {
		*val_ptr++ = 0U;
		(void)memset((void *)val_ptr, 0U,
			     ULL_ADV_HDR_DATA_DATA_PTR_SIZE);
	} else {
		*val_ptr++ = len;
		(void)memcpy(val_ptr, &data, sizeof(data));
	}

	if (false) {

#if defined(CONFIG_BT_CTLR_ADV_AUX_PDU_LINK)
	} else if (op == BT_HCI_LE_EXT_ADV_OP_UNCHANGED_DATA) {
		hdr_add_fields = ULL_ADV_PDU_HDR_FIELD_AD_DATA;
		err = ull_adv_aux_pdu_set_clear(adv, sr_pdu_prev, sr_pdu,
						hdr_add_fields, 0U, hdr_data);

		/* No AD data overflow */
		ad_len_overflow = 0U;

		/* No AD data in chain PDU */
		ad_len_chain = 0U;

		/* pri_idx and sec_idx not used later in code in this function
		 */
		pri_idx = 0U;
		sec_idx = 0U;
#endif /* CONFIG_BT_CTLR_ADV_AUX_PDU_LINK */

	} else if (!IS_ENABLED(CONFIG_BT_CTLR_ADV_AUX_PDU_LINK) ||
		   (op == BT_HCI_LE_EXT_ADV_OP_FIRST_FRAG ||
		    op == BT_HCI_LE_EXT_ADV_OP_COMPLETE_DATA)) {
		struct pdu_adv_adi *adi;

		/* If ADI in scan response is not supported then we do not
		 * need reference to ADI in auxiliary PDU
		 */
		hdr_add_fields = 0U;

		/* Add ADI if support enabled */
		if (IS_ENABLED(CONFIG_BT_CTRL_ADV_ADI_IN_SCAN_RSP)) {
			/* We need to get reference to ADI in auxiliary PDU */
			hdr_add_fields |= ULL_ADV_PDU_HDR_FIELD_ADI;

			/* Update DID by passing NULL reference for ADI */
			(void)memset((void *)val_ptr, 0,
				     sizeof(struct pdu_adv_adi *));

			/* Data place holder is after ADI */
			val_ptr += sizeof(struct pdu_adv_adi *);

			/* Place holder and reference to data passed and
			 * old reference to be returned
			 */
			*val_ptr++ = len;
			(void)memcpy(val_ptr, &data, sizeof(data));
		}

		/* Trigger DID update */
		err = ull_adv_aux_hdr_set_clear(adv, hdr_add_fields, 0U,
						hdr_data, &pri_idx, &sec_idx);
		if (err) {
			return err;
		}

		if ((op == BT_HCI_LE_EXT_ADV_OP_COMPLETE_DATA) && !len) {
			sr_pdu->len = 0;
			goto sr_data_set_did_update;
		}

		if (IS_ENABLED(CONFIG_BT_CTRL_ADV_ADI_IN_SCAN_RSP)) {
			(void)memcpy(&adi,
				     &hdr_data[ULL_ADV_HDR_DATA_ADI_PTR_OFFSET],
				     sizeof(struct pdu_adv_adi *));
		}

		if (op == BT_HCI_LE_EXT_ADV_OP_INTERM_FRAG ||
		    op == BT_HCI_LE_EXT_ADV_OP_LAST_FRAG) {
			/* Append fragment to existing data */
			hdr_add_fields |= ULL_ADV_PDU_HDR_FIELD_ADVA |
					  ULL_ADV_PDU_HDR_FIELD_AD_DATA_APPEND;
			err = ull_adv_aux_pdu_set_clear(adv, sr_pdu_prev, sr_pdu,
							hdr_add_fields,
							0,
							hdr_data);
		} else {
			/* Add AD Data and remove any prior presence of Aux Ptr */
			hdr_add_fields |= ULL_ADV_PDU_HDR_FIELD_ADVA |
					  ULL_ADV_PDU_HDR_FIELD_AD_DATA;
			err = ull_adv_aux_pdu_set_clear(adv, sr_pdu_prev, sr_pdu,
						hdr_add_fields,
						ULL_ADV_PDU_HDR_FIELD_AUX_PTR,
						hdr_data);
		}
#if defined(CONFIG_BT_CTLR_ADV_AUX_PDU_LINK)
		if (err == BT_HCI_ERR_PACKET_TOO_LONG) {
			uint8_t ad_len_offset;

			ad_len_offset = ULL_ADV_HDR_DATA_DATA_PTR_OFFSET +
					ULL_ADV_HDR_DATA_DATA_PTR_SIZE;
			if (hdr_add_fields & ULL_ADV_PDU_HDR_FIELD_ADI) {
				ad_len_offset +=
					ULL_ADV_HDR_DATA_ADI_PTR_OFFSET +
					ULL_ADV_HDR_DATA_ADI_PTR_SIZE;
			}
			ad_len_overflow = hdr_data[ad_len_offset];

			/* Prepare the AD data as parameter to update in
			 * PDU
			 */
			val_ptr = hdr_data;

			/* Place holder for ADI field reference to be
			 * returned
			 */
			if (hdr_add_fields &
			    ULL_ADV_PDU_HDR_FIELD_ADI) {
				val_ptr++;
				(void)memcpy(val_ptr, &adi,
					     sizeof(struct pdu_adv_adi *));
				val_ptr += sizeof(struct pdu_adv_adi *);
			}

			/* Place holder and reference to data passed and
			 * old reference to be returned
			 */
			*val_ptr++ = len - ad_len_overflow;
			(void)memcpy(val_ptr, &data, sizeof(data));

			err = ull_adv_aux_pdu_set_clear(adv, sr_pdu_prev,
						sr_pdu, hdr_add_fields,
						ULL_ADV_PDU_HDR_FIELD_AUX_PTR,
						hdr_data);
		}

		if (!err) {
			/* Fragment into chain PDU if len > 191 bytes */
			if (len > PDU_AC_EXT_AD_DATA_LEN_MAX) {
				/* Prepare the AD data as parameter to update in
				 * PDU
				 */
				val_ptr = hdr_data;

				/* Place holder for ADI field reference to be
				 * returned
				 */
				if (hdr_add_fields &
				    ULL_ADV_PDU_HDR_FIELD_ADI) {
					val_ptr++;
					val_ptr += sizeof(struct pdu_adv_adi *);
				}

				/* Place holder for aux ptr reference to be
				 * returned
				 */
				val_ptr++;
				val_ptr += sizeof(uint8_t *);

				/* Place holder and reference to data passed and
				 * old reference to be returned
				 */
				*val_ptr = PDU_AC_EXT_AD_DATA_LEN_MAX;
				(void)memcpy(&val_ptr[ULL_ADV_HDR_DATA_DATA_PTR_OFFSET],
					     &data, sizeof(data));

				/* Calculate the overflow chain PDU's AD data
				 * length
				 */
				ad_len_overflow =
					len - PDU_AC_EXT_AD_DATA_LEN_MAX;

				/* No AD data in chain PDU besides the
				 * overflow
				 */
				ad_len_chain = 0U;
			} else {
				struct pdu_adv *pdu_chain;

				/* Remove/Release any previous chain PDU
				 * allocations
				 */
				pdu_chain = lll_adv_pdu_linked_next_get(sr_pdu);
				if (pdu_chain) {
					lll_adv_pdu_linked_append(NULL, sr_pdu);
					lll_adv_pdu_linked_release_all(pdu_chain);
				}

				/* No AD data overflow */
				ad_len_overflow = 0U;

				/* No AD data in chain PDU */
				ad_len_chain = 0U;
			}
		} else {
			/* No AD data overflow */
			ad_len_overflow = 0U;

			/* No AD data in chain PDU */
			ad_len_chain = 0U;
		}
	} else {
		struct pdu_adv *pdu_chain_prev;
		struct pdu_adv *pdu_chain;
		uint16_t ad_len_total;
		uint8_t ad_len_prev;

		/* Traverse to next set clear hdr data parameter */
		val_ptr += sizeof(data);

		/* Traverse to the last chain PDU */
		ad_len_total = 0U;
		pdu_chain_prev = sr_pdu_prev;
		pdu_chain = sr_pdu;
		do {
			/* Prepare for aux ptr field reference to be returned, hence
			 * second parameter will be for AD data field.
			 */
			*val_ptr = 0U;
			(void)memset((void *)&val_ptr[ULL_ADV_HDR_DATA_DATA_PTR_OFFSET],
				     0U, ULL_ADV_HDR_DATA_DATA_PTR_SIZE);

			sr_pdu_prev = pdu_chain_prev;
			sr_pdu = pdu_chain;

			/* Add Aux Ptr field if not already present */
			hdr_add_fields = ULL_ADV_PDU_HDR_FIELD_AD_DATA |
					 ULL_ADV_PDU_HDR_FIELD_AUX_PTR;
			err = ull_adv_aux_pdu_set_clear(adv, sr_pdu_prev,
							sr_pdu, hdr_add_fields,
							0U, hdr_data);
			LL_ASSERT(!err || (err == BT_HCI_ERR_PACKET_TOO_LONG));

			/* Get PDUs previous AD data length */
			ad_len_prev =
				hdr_data[ULL_ADV_HDR_DATA_AUX_PTR_PTR_OFFSET +
					 ULL_ADV_HDR_DATA_AUX_PTR_PTR_SIZE];

			/* Check of max supported AD data len */
			ad_len_total += ad_len_prev;
			if ((ad_len_total + len) >
			    CONFIG_BT_CTLR_ADV_DATA_LEN_MAX) {
				/* NOTE: latest PDU was not consumed by LLL and
				 * as ull_adv_sync_pdu_alloc() has reverted back
				 * the double buffer with the first PDU, and
				 * returned the latest PDU as the new PDU, we
				 * need to enqueue back the new PDU which is
				 * infact the latest PDU.
				 */
				if (sr_pdu_prev == sr_pdu) {
					lll_adv_scan_rsp_enqueue(lll, sr_idx);
				}

				return BT_HCI_ERR_MEM_CAPACITY_EXCEEDED;
			}

			pdu_chain_prev = lll_adv_pdu_linked_next_get(sr_pdu_prev);
			pdu_chain = lll_adv_pdu_linked_next_get(sr_pdu);
			LL_ASSERT((pdu_chain_prev && pdu_chain) ||
				  (!pdu_chain_prev && !pdu_chain));
		} while (pdu_chain_prev);

		if (err == BT_HCI_ERR_PACKET_TOO_LONG) {
			ad_len_overflow =
				hdr_data[ULL_ADV_HDR_DATA_AUX_PTR_PTR_OFFSET +
					 ULL_ADV_HDR_DATA_AUX_PTR_PTR_SIZE +
					 ULL_ADV_HDR_DATA_DATA_PTR_OFFSET +
					 ULL_ADV_HDR_DATA_DATA_PTR_SIZE];

			/* Prepare for aux ptr field reference to be returned,
			 * hence second parameter will be for AD data field.
			 * Fill it with reduced AD data length.
			 */
			*val_ptr = ad_len_prev - ad_len_overflow;

			/* AD data len in chain PDU */
			ad_len_chain = len;

			/* Proceed to add chain PDU */
			err = 0U;
		} else {
			/* No AD data overflow */
			ad_len_overflow = 0U;

			/* No AD data in chain PDU */
			ad_len_chain = 0U;
		}

		/* pri_idx and sec_idx not used later in code in this function
		 */
		pri_idx = 0U;
		sec_idx = 0U;
#endif /* CONFIG_BT_CTLR_ADV_AUX_PDU_LINK */
	}
	if (err) {
		return err;
	}

#if defined(CONFIG_BT_CTLR_ADV_AUX_PDU_LINK)
	if ((op == BT_HCI_LE_EXT_ADV_OP_INTERM_FRAG) ||
	    (op == BT_HCI_LE_EXT_ADV_OP_LAST_FRAG) ||
	    ad_len_overflow) {
		struct pdu_adv_com_ext_adv *com_hdr_chain;
		struct pdu_adv_com_ext_adv *com_hdr;
		struct pdu_adv_ext_hdr *hdr_chain;
		struct pdu_adv_aux_ptr *aux_ptr;
		struct pdu_adv *pdu_chain_prev;
		struct pdu_adv_ext_hdr hdr;
		struct pdu_adv *pdu_chain;
		uint8_t aux_ptr_offset;
		uint8_t *dptr_chain;
		uint32_t offs_us;
		uint16_t sec_len;
		uint8_t *dptr;

		/* Get reference to flags in superior PDU */
		com_hdr = &sr_pdu->adv_ext_ind;
		if (com_hdr->ext_hdr_len) {
			hdr = com_hdr->ext_hdr;
		} else {
			*(uint8_t *)&hdr = 0U;
		}
		dptr = com_hdr->ext_hdr.data;

		/* Allocate new PDU */
		pdu_chain = lll_adv_pdu_alloc_pdu_adv();
		LL_ASSERT(pdu_chain);

		/* Populate the appended chain PDU */
		pdu_chain->type = PDU_ADV_TYPE_AUX_CHAIN_IND;
		pdu_chain->rfu = 0U;
		pdu_chain->chan_sel = 0U;
		pdu_chain->tx_addr = 0U;
		pdu_chain->rx_addr = 0U;
		pdu_chain->len = 0U;

		com_hdr_chain = &pdu_chain->adv_ext_ind;
		hdr_chain = (void *)&com_hdr_chain->ext_hdr_adv_data[0];
		dptr_chain = (void *)hdr_chain;

		/* Flags */
		*dptr_chain = 0U;

		/* ADI flag, mandatory if superior PDU has it */
		if (hdr.adi) {
			hdr_chain->adi = 1U;
		}

		/* Proceed to next byte if any flags present */
		if (*dptr_chain) {
			dptr_chain++;
		}

		/* Start adding fields corresponding to flags here, if any */

		/* AdvA flag */
		if (hdr.adv_addr) {
			dptr += BDADDR_SIZE;
		}

		/* TgtA flag */
		if (hdr.tgt_addr) {
			dptr += BDADDR_SIZE;
		}

		/* No CTEInfo in Extended Advertising */

		/* ADI flag */
		if (hdr_chain->adi) {
			(void)memcpy(dptr_chain, dptr,
				     sizeof(struct pdu_adv_adi));

			dptr += sizeof(struct pdu_adv_adi);
			dptr_chain += sizeof(struct pdu_adv_adi);
		}

		/* non-connectable non-scannable chain pdu */
		com_hdr_chain->adv_mode = 0;

		/* Calc current chain PDU len */
		sec_len = ull_adv_aux_hdr_len_calc(com_hdr_chain, &dptr_chain);

		/* Prefix overflowed data to chain PDU and reduce the AD data in
		 * in the current PDU.
		 */
		if (ad_len_overflow) {
			uint8_t *ad_overflow;

			/* Copy overflowed AD data from previous PDU into this
			 * new chain PDU
			 */
			(void)memcpy(&ad_overflow,
				     &val_ptr[ULL_ADV_HDR_DATA_DATA_PTR_OFFSET],
				     sizeof(ad_overflow));
			ad_overflow += *val_ptr;
			(void)memcpy(dptr_chain, ad_overflow, ad_len_overflow);
			dptr_chain += ad_len_overflow;

			hdr_add_fields |= ULL_ADV_PDU_HDR_FIELD_AUX_PTR;

			/* Reduce the AD data in the previous PDU */
			err = ull_adv_aux_pdu_set_clear(adv, sr_pdu_prev,
							sr_pdu, hdr_add_fields,
							0U, hdr_data);
			if (err) {
				/* NOTE: latest PDU was not consumed by LLL and
				 * as ull_adv_sync_pdu_alloc() has reverted back
				 * the double buffer with the first PDU, and
				 * returned the latest PDU as the new PDU, we
				 * need to enqueue back the new PDU which is
				 * infact the latest PDU.
				 */
				if (sr_pdu_prev == sr_pdu) {
					lll_adv_scan_rsp_enqueue(lll, sr_idx);
				}

				return err;
			}

			/* AD data len in chain PDU besides the overflow */
			len = ad_len_chain;
		}

		/* Check AdvData overflow */
		if ((sec_len + ad_len_overflow + len) >
		    PDU_AC_PAYLOAD_SIZE_MAX) {
			/* NOTE: latest PDU was not consumed by LLL and
			 * as ull_adv_sync_pdu_alloc() has reverted back
			 * the double buffer with the first PDU, and
			 * returned the latest PDU as the new PDU, we
			 * need to enqueue back the new PDU which is
			 * infact the latest PDU.
			 */
			if (sr_pdu_prev == sr_pdu) {
				lll_adv_aux_data_enqueue(adv->lll.aux, sr_idx);
			}

			return BT_HCI_ERR_PACKET_TOO_LONG;
		}

		/* Fill the chain PDU length */
		ull_adv_aux_hdr_len_fill(com_hdr_chain, sec_len);
		pdu_chain->len = sec_len + ad_len_overflow + len;

		/* Fill AD Data in chain PDU */
		(void)memcpy(dptr_chain, data, len);

		/* Get reference to aux ptr in superior PDU */
		aux_ptr_offset = ULL_ADV_HDR_DATA_AUX_PTR_PTR_OFFSET;
		if (hdr_add_fields & ULL_ADV_PDU_HDR_FIELD_ADI) {
			aux_ptr_offset +=  ULL_ADV_HDR_DATA_ADI_PTR_OFFSET +
					   ULL_ADV_HDR_DATA_ADI_PTR_SIZE;
		}
		(void)memcpy(&aux_ptr, &hdr_data[aux_ptr_offset],
			     sizeof(aux_ptr));

		/* Fill the aux offset in the previous AUX_SYNC_IND PDU */
		offs_us = PDU_AC_US(sr_pdu->len, adv->lll.phy_s,
				    adv->lll.phy_flags) +
			  EVENT_B2B_MAFS_US;
		ull_adv_aux_ptr_fill(aux_ptr, offs_us, adv->lll.phy_s);

		/* Remove/Release any previous chain PDUs */
		pdu_chain_prev = lll_adv_pdu_linked_next_get(sr_pdu);
		if (pdu_chain_prev) {
			lll_adv_pdu_linked_append(NULL, sr_pdu);
			lll_adv_pdu_linked_release_all(pdu_chain_prev);
		}

		/* Chain the PDU */
		lll_adv_pdu_linked_append(pdu_chain, sr_pdu);
	}
#endif /* CONFIG_BT_CTLR_ADV_AUX_PDU_LINK */

sr_data_set_did_update:
	if ((!IS_ENABLED(CONFIG_BT_CTLR_ADV_AUX_PDU_LINK) &&
	     (op == BT_HCI_LE_EXT_ADV_OP_INTERM_FRAG ||
	      op == BT_HCI_LE_EXT_ADV_OP_LAST_FRAG)) ||
	    (op == BT_HCI_LE_EXT_ADV_OP_FIRST_FRAG) ||
	    (op == BT_HCI_LE_EXT_ADV_OP_COMPLETE_DATA)) {
		/* NOTE: No update to primary channel PDU time reservation  */

		lll_adv_aux_data_enqueue(adv->lll.aux, sec_idx);
		lll_adv_data_enqueue(&adv->lll, pri_idx);

		sr_pdu->type = PDU_ADV_TYPE_AUX_SCAN_RSP;
		sr_pdu->rfu = 0U;
		sr_pdu->chan_sel = 0U;
		sr_pdu->rx_addr = 0U;
		if (sr_pdu->len) {
			sr_pdu->adv_ext_ind.adv_mode = 0U;
			sr_pdu->tx_addr = sec_pdu_prev->tx_addr;
			(void)memcpy(&sr_pdu->adv_ext_ind.ext_hdr.data[ADVA_OFFSET],
				     &sec_pdu_prev->adv_ext_ind.ext_hdr.data[ADVA_OFFSET],
				     BDADDR_SIZE);
		} else {
			sr_pdu->tx_addr = 0U;
		}
	}

	lll_adv_scan_rsp_enqueue(lll, sr_idx);

	return 0;
}

uint16_t ll_adv_aux_max_data_length_get(void)
{
	return CONFIG_BT_CTLR_ADV_DATA_LEN_MAX;
}

uint8_t ll_adv_aux_set_count_get(void)
{
	return BT_CTLR_ADV_SET;
}

uint8_t ll_adv_aux_set_remove(uint8_t handle)
{
	struct ll_adv_set *adv;
	struct lll_adv *lll;

	/* Get the advertising set instance */
	adv = ull_adv_is_created_get(handle);
	if (!adv) {
		return BT_HCI_ERR_UNKNOWN_ADV_IDENTIFIER;
	}

	if (adv->is_enabled) {
		return BT_HCI_ERR_CMD_DISALLOWED;
	}

	lll = &adv->lll;

#if defined(CONFIG_BT_CTLR_ADV_PERIODIC)
	if (lll->sync) {
		struct ll_adv_sync_set *sync;

		sync = HDR_LLL2ULL(lll->sync);

		if (sync->is_enabled) {
			return BT_HCI_ERR_CMD_DISALLOWED;
		}
		lll->sync = NULL;

		ull_adv_sync_release(sync);
	}
#endif /* CONFIG_BT_CTLR_ADV_PERIODIC */

#if defined(CONFIG_BT_CTLR_DF_ADV_CTE_TX)
	if (adv->df_cfg) {
		if (adv->df_cfg->is_enabled) {
			return BT_HCI_ERR_CMD_DISALLOWED;
		}

		ull_df_adv_cfg_release(adv->df_cfg);
		adv->df_cfg = NULL;
	}
#endif /* CONFIG_BT_CTLR_DF_ADV_CTE_TX */

	/* Release auxiliary channel set */
	if (lll->aux) {
		struct ll_adv_aux_set *aux;

		aux = HDR_LLL2ULL(lll->aux);
		lll->aux = NULL;

		ull_adv_aux_release(aux);
	}

	/* Dequeue and release, advertising and scan response data, to keep
	 * one initial primary channel PDU each for the advertising set.
	 * This is done to prevent common extended payload format contents from
	 * being overwritten and corrupted due to same primary PDU buffer being
	 * used to remove AdvA and other fields are moved over in its place when
	 * auxiliary PDU is allocated to new advertising set.
	 */
	(void)lll_adv_data_dequeue(&adv->lll.adv_data);
	(void)lll_adv_data_dequeue(&adv->lll.scan_rsp);

	/* Make the advertising set available for new advertisements */
	adv->is_created = 0;

	return BT_HCI_ERR_SUCCESS;
}

uint8_t ll_adv_aux_set_clear(void)
{
	uint8_t retval = BT_HCI_ERR_SUCCESS;
	uint8_t handle;
	uint8_t err;

	for (handle = 0; handle < BT_CTLR_ADV_SET; ++handle) {
		err = ll_adv_aux_set_remove(handle);
		if (err == BT_HCI_ERR_CMD_DISALLOWED) {
			retval = err;
		}
	}

	return retval;
}

int ull_adv_aux_init(void)
{
	int err;

	err = lll_rand_get(&did_unique, sizeof(did_unique));
	if (err) {
		return err;
	}

	err = init_reset();
	if (err) {
		return err;
	}

	return 0;
}

int ull_adv_aux_reset_finalize(void)
{
	int err;

	err = init_reset();
	if (err) {
		return err;
	}

	return 0;
}

uint8_t ull_adv_aux_chm_update(void)
{
	/* For each created extended advertising set */
	for (uint8_t handle = 0; handle < BT_CTLR_ADV_SET; ++handle) {
		struct ll_adv_aux_set *aux;
		struct ll_adv_set *adv;
		uint8_t chm_last;

		adv = ull_adv_is_created_get(handle);
		if (!adv || !adv->lll.aux) {
			continue;
		}

		aux = HDR_LLL2ULL(adv->lll.aux);
		if (aux->chm_last != aux->chm_first) {
			/* TODO: Handle previous Channel Map Update being in
			 * progress
			 */
			continue;
		}

		/* Append the channelMapNew that will be picked up by ULL */
		chm_last = aux->chm_last + 1;
		if (chm_last == DOUBLE_BUFFER_SIZE) {
			chm_last = 0U;
		}
		aux->chm[chm_last].data_chan_count =
			ull_chan_map_get(aux->chm[chm_last].data_chan_map);
		aux->chm_last = chm_last;

		if (!aux->is_started) {
			/* Ticker not started yet, apply new channel map now
			 * Note that it should be safe to modify chm_first here
			 * since advertising is not active
			 */
			aux->chm_first = aux->chm_last;
		}
	}

	/* TODO: Should failure due to Channel Map Update being already in
	 *       progress be returned to caller?
	 */
	return 0;
}

uint8_t ull_adv_aux_hdr_set_clear(struct ll_adv_set *adv,
				  uint16_t sec_hdr_add_fields,
				  uint16_t sec_hdr_rem_fields,
				  void *hdr_data,
				  uint8_t *pri_idx, uint8_t *sec_idx)
{
	struct pdu_adv_com_ext_adv *pri_com_hdr, *pri_com_hdr_prev;
	struct pdu_adv_com_ext_adv *sec_com_hdr, *sec_com_hdr_prev;
	struct pdu_adv_ext_hdr *hdr, pri_hdr, pri_hdr_prev;
	struct pdu_adv_ext_hdr sec_hdr, sec_hdr_prev;
	struct pdu_adv *pri_pdu, *pri_pdu_prev;
	struct pdu_adv *sec_pdu_prev, *sec_pdu;
	struct pdu_adv_adi *pri_adi, *sec_adi;
	uint8_t *pri_dptr, *pri_dptr_prev;
	uint8_t *sec_dptr, *sec_dptr_prev;
	struct pdu_adv_aux_ptr *aux_ptr;
	uint8_t pri_len, sec_len_prev;
	struct lll_adv_aux *lll_aux;
	uint8_t *ad_fragment = NULL;
	uint8_t ad_fragment_len = 0;
	struct ll_adv_aux_set *aux;
	struct pdu_adv_adi *adi;
	struct lll_adv *lll;
	uint8_t is_aux_new;
	uint8_t *ad_data;
	uint16_t sec_len;
	uint8_t ad_len;
	uint16_t did;

	lll = &adv->lll;

	/* Can't have both flags set here since both use 'hdr_data' param */
	LL_ASSERT(!(sec_hdr_add_fields & ULL_ADV_PDU_HDR_FIELD_ADVA) ||
		  !(sec_hdr_add_fields & ULL_ADV_PDU_HDR_FIELD_AD_DATA));

	/* Get reference to previous primary PDU data */
	pri_pdu_prev = lll_adv_data_peek(lll);
	if (pri_pdu_prev->type != PDU_ADV_TYPE_EXT_IND) {
		if (sec_hdr_add_fields & ULL_ADV_PDU_HDR_FIELD_AD_DATA) {
			/* pick the data length */
			ad_len = *((uint8_t *)hdr_data);
			hdr_data = (uint8_t *)hdr_data + sizeof(ad_len);

			/* pick the reference to data */
			(void)memcpy(&ad_data, hdr_data, sizeof(ad_data));

			return ull_adv_data_set(adv, ad_len, ad_data);
		}

		return BT_HCI_ERR_CMD_DISALLOWED;
	}

	pri_com_hdr_prev = (void *)&pri_pdu_prev->adv_ext_ind;
	hdr = (void *)pri_com_hdr_prev->ext_hdr_adv_data;
	if (pri_com_hdr_prev->ext_hdr_len) {
		pri_hdr_prev = *hdr;
	} else {
		*(uint8_t *)&pri_hdr_prev = 0U;
	}
	pri_dptr_prev = hdr->data;

	/* Advertising data are not supported by scannable instances */
	if ((sec_hdr_add_fields & ULL_ADV_PDU_HDR_FIELD_AD_DATA) &&
	    (pri_com_hdr_prev->adv_mode & BT_HCI_LE_ADV_PROP_SCAN)) {
		return BT_HCI_ERR_INVALID_PARAM;
	}

	/* Get reference to new primary PDU data buffer */
	pri_pdu = lll_adv_data_alloc(lll, pri_idx);
	pri_pdu->type = pri_pdu_prev->type;
	pri_pdu->rfu = 0U;
	pri_pdu->chan_sel = 0U;
	pri_com_hdr = (void *)&pri_pdu->adv_ext_ind;
	pri_com_hdr->adv_mode = pri_com_hdr_prev->adv_mode;
	hdr = (void *)pri_com_hdr->ext_hdr_adv_data;
	pri_dptr = hdr->data;
	*(uint8_t *)&pri_hdr = 0U;

	/* Get the reference to aux instance */
	lll_aux = lll->aux;
	if (!lll_aux) {
		aux = ull_adv_aux_acquire(lll);
		if (!aux) {
			LL_ASSERT(pri_pdu != pri_pdu_prev);

			return BT_HCI_ERR_MEM_CAPACITY_EXCEEDED;
		}

		lll_aux = &aux->lll;
		ull_adv_aux_created(adv);

		is_aux_new = 1U;
	} else {
		aux = HDR_LLL2ULL(lll_aux);
		is_aux_new = 0U;
	}

	/* Get reference to previous secondary PDU data */
	sec_pdu_prev = lll_adv_aux_data_peek(lll_aux);
	sec_com_hdr_prev = (void *)&sec_pdu_prev->adv_ext_ind;
	hdr = (void *)sec_com_hdr_prev->ext_hdr_adv_data;
	if (!is_aux_new) {
		sec_hdr_prev = *hdr;
	} else {
		/* Initialize only those fields used to copy into new PDU
		 * buffer.
		 */
		sec_pdu_prev->tx_addr = 0U;
		sec_pdu_prev->rx_addr = 0U;
		sec_pdu_prev->len = PDU_AC_EXT_HEADER_SIZE_MIN;
		*(uint8_t *)hdr = 0U;
		*(uint8_t *)&sec_hdr_prev = 0U;
	}
	sec_dptr_prev = hdr->data;

	/* Get reference to new secondary PDU data buffer */
	sec_pdu = lll_adv_aux_data_alloc(lll_aux, sec_idx);
	sec_pdu->type = pri_pdu->type;
	sec_pdu->rfu = 0U;
	sec_pdu->chan_sel = 0U;

	sec_pdu->tx_addr = sec_pdu_prev->tx_addr;
	sec_pdu->rx_addr = sec_pdu_prev->rx_addr;

	sec_com_hdr = (void *)&sec_pdu->adv_ext_ind;
	sec_com_hdr->adv_mode = pri_com_hdr->adv_mode;
	hdr = (void *)sec_com_hdr->ext_hdr_adv_data;
	sec_dptr = hdr->data;
	*(uint8_t *)&sec_hdr = 0U;

	/* AdvA flag */
	/* NOTE: as we will use auxiliary packet, we remove AdvA in primary
	 * channel, i.e. do nothing to not add AdvA in the primary PDU.
	 * AdvA can be either set explicitly (i.e. needs own_addr_type to be
	 * set), can be copied from primary PDU (i.e. adding AD to existing set)
	 * or can be copied from previous secondary PDU.
	 */
	sec_hdr.adv_addr = 1;
	if (sec_hdr_add_fields & ULL_ADV_PDU_HDR_FIELD_ADVA) {
		uint8_t own_addr_type = *(uint8_t *)hdr_data;

		/* Move to next hdr_data */
		hdr_data = (uint8_t *)hdr_data + sizeof(own_addr_type);

		sec_pdu->tx_addr = own_addr_type & 0x1;
	} else if (pri_hdr_prev.adv_addr) {
		sec_pdu->tx_addr = pri_pdu_prev->tx_addr;
	} else if (sec_hdr_prev.adv_addr) {
		sec_pdu->tx_addr = sec_pdu_prev->tx_addr;
	} else {
		/* We do not have valid address info, this should not happen */
		return BT_HCI_ERR_UNSPECIFIED;
	}
	pri_pdu->tx_addr = 0U;

	if (pri_hdr_prev.adv_addr) {
		pri_dptr_prev += BDADDR_SIZE;
	}
	if (sec_hdr_prev.adv_addr) {
		sec_dptr_prev += BDADDR_SIZE;
	}
	sec_dptr += BDADDR_SIZE;

	/* No TargetA in primary and secondary channel for undirected.
	 * Move from primary to secondary PDU, if present in primary PDU.
	 */
	if (pri_hdr_prev.tgt_addr) {
		sec_hdr.tgt_addr = 1U;
		sec_pdu->rx_addr = pri_pdu_prev->rx_addr;
		sec_dptr += BDADDR_SIZE;

	/* Retain the target address if present in the previous PDU */
	} else if (!(sec_hdr_add_fields & ULL_ADV_PDU_HDR_FIELD_ADVA) &&
		   sec_hdr_prev.tgt_addr) {
		sec_hdr.tgt_addr = 1U;
		sec_pdu->rx_addr = sec_pdu_prev->rx_addr;
		sec_dptr += BDADDR_SIZE;
	}
	pri_pdu->rx_addr = 0U;

	if (pri_hdr_prev.tgt_addr) {
		pri_dptr_prev += BDADDR_SIZE;
	}

	if (sec_hdr_prev.tgt_addr) {
		sec_dptr_prev += BDADDR_SIZE;
	}

	/* No CTEInfo flag in primary and secondary channel PDU */

	/* ADI flag */
	if (pri_hdr_prev.adi) {
		pri_dptr_prev += sizeof(struct pdu_adv_adi);
	}
	pri_hdr.adi = 1;
	pri_dptr += sizeof(struct pdu_adv_adi);
	if (sec_hdr_add_fields & ULL_ADV_PDU_HDR_FIELD_ADI) {
		sec_hdr.adi = 1U;
		/* return the size of ADI structure */
		*(uint8_t *)hdr_data = sizeof(struct pdu_adv_adi);
		hdr_data = (uint8_t *)hdr_data + sizeof(uint8_t);
		/* pick the reference to ADI param */
		(void)memcpy(&adi, hdr_data, sizeof(struct pdu_adv_adi *));
		/* return the pointer to ADI struct inside the PDU */
		(void)memcpy(hdr_data, &sec_dptr, sizeof(sec_dptr));
		hdr_data = (uint8_t *)hdr_data + sizeof(sec_dptr);
		sec_dptr += sizeof(struct pdu_adv_adi);
	} else {
		sec_hdr.adi = 1;
		adi = NULL;
		sec_dptr += sizeof(struct pdu_adv_adi);
	}
	if (sec_hdr_prev.adi) {
		sec_dptr_prev += sizeof(struct pdu_adv_adi);
	}

	/* AuxPtr flag */
	if (pri_hdr_prev.aux_ptr) {
		pri_dptr_prev += sizeof(struct pdu_adv_aux_ptr);
	}
	pri_hdr.aux_ptr = 1;
	pri_dptr += sizeof(struct pdu_adv_aux_ptr);

	if (sec_hdr_add_fields & ULL_ADV_PDU_HDR_FIELD_AUX_PTR) {
		sec_hdr.aux_ptr = 1;
		aux_ptr = NULL;

		/* return the size of aux pointer structure */
		*(uint8_t *)hdr_data = sizeof(struct pdu_adv_aux_ptr);
		hdr_data = (uint8_t *)hdr_data + sizeof(uint8_t);

		/* return the pointer to aux pointer struct inside the PDU
		 * buffer
		 */
		(void)memcpy(hdr_data, &sec_dptr, sizeof(sec_dptr));
		hdr_data = (uint8_t *)hdr_data + sizeof(sec_dptr);
	} else if (!(sec_hdr_rem_fields & ULL_ADV_PDU_HDR_FIELD_AUX_PTR) &&
		   sec_hdr_prev.aux_ptr) {
		sec_hdr.aux_ptr = 1;
		aux_ptr = (void *)sec_dptr_prev;
	} else {
		aux_ptr = NULL;
	}
	if (sec_hdr_prev.aux_ptr) {
		sec_dptr_prev += sizeof(struct pdu_adv_aux_ptr);
	}
	if (sec_hdr.aux_ptr) {
		sec_dptr += sizeof(struct pdu_adv_aux_ptr);
	}

#if defined(CONFIG_BT_CTLR_ADV_PERIODIC)
	struct pdu_adv_sync_info *sync_info;

	/* No SyncInfo flag in primary channel PDU */
	/* Add/Remove SyncInfo flag in secondary channel PDU */
	if (sec_hdr_add_fields & ULL_ADV_PDU_HDR_FIELD_SYNC_INFO) {
		sec_hdr.sync_info = 1;
		sync_info = NULL;

		/* return the size of sync info structure */
		*(uint8_t *)hdr_data = sizeof(*sync_info);
		hdr_data = (uint8_t *)hdr_data + sizeof(uint8_t);

		/* return the pointer to sync info struct inside the PDU
		 * buffer
		 */
		(void)memcpy(hdr_data, &sec_dptr, sizeof(sec_dptr));
		hdr_data = (uint8_t *)hdr_data + sizeof(sec_dptr);
	} else if (!(sec_hdr_rem_fields & ULL_ADV_PDU_HDR_FIELD_SYNC_INFO) &&
		   sec_hdr_prev.sync_info) {
		sec_hdr.sync_info = 1;
		sync_info = (void *)sec_dptr_prev;
	} else {
		sync_info = NULL;
	}
	if (sec_hdr_prev.sync_info) {
		sec_dptr_prev += sizeof(*sync_info);
	}
	if (sec_hdr.sync_info) {
		sec_dptr += sizeof(*sync_info);
	}
#endif /* CONFIG_BT_CTLR_ADV_PERIODIC */

	/* Tx Power flag */
	if (pri_hdr_prev.tx_pwr) {
		pri_dptr_prev++;

		/* C1, Tx Power is optional on the LE 1M PHY, and
		 * reserved for future use on the LE Coded PHY.
		 */
		if (lll->phy_p != PHY_CODED) {
			pri_hdr.tx_pwr = 1;
			pri_dptr++;
		} else {
			sec_hdr.tx_pwr = 1;
		}
	}
	if (sec_hdr_prev.tx_pwr) {
		sec_dptr_prev++;

		sec_hdr.tx_pwr = 1;
	}
	if (sec_hdr.tx_pwr) {
		sec_dptr++;
	}

	/* No ACAD in primary channel PDU */
	/* TODO: ACAD in secondary channel PDU */

	/* Calc primary PDU len */
	pri_len = ull_adv_aux_hdr_len_calc(pri_com_hdr, &pri_dptr);

	/* Calc previous secondary PDU len */
	sec_len_prev = ull_adv_aux_hdr_len_calc(sec_com_hdr_prev,
					       &sec_dptr_prev);

	/* Did we parse beyond PDU length? */
	if (sec_len_prev > sec_pdu_prev->len) {
		/* we should not encounter invalid length */
		/* FIXME: release allocations */
		return BT_HCI_ERR_UNSPECIFIED;
	}

	/* Calc current secondary PDU len */
	sec_len = ull_adv_aux_hdr_len_calc(sec_com_hdr, &sec_dptr);

	/* AD Data, add or remove */
	if (sec_hdr_add_fields & ULL_ADV_PDU_HDR_FIELD_AD_DATA) {
		uint8_t ad_len_prev;

		/* remember the new ad data len */
		ad_len = *(uint8_t *)hdr_data;

		/* return prev ad data length */
		ad_len_prev = sec_pdu_prev->len - sec_len_prev;
		*(uint8_t *)hdr_data = ad_len_prev;
		hdr_data = (uint8_t *)hdr_data + sizeof(ad_len);

		/* remember the reference to new ad data */
		(void)memcpy(&ad_data, hdr_data, sizeof(ad_data));

		/* return the reference to prev ad data */
		(void)memcpy(hdr_data, &sec_dptr_prev, sizeof(sec_dptr_prev));
		hdr_data = (uint8_t *)hdr_data + sizeof(sec_dptr_prev);

		/* unchanged data */
		if (!ad_len && !ad_data) {
			ad_len = ad_len_prev;
			ad_data = sec_dptr_prev;
		}
	} else if (sec_hdr_add_fields & ULL_ADV_PDU_HDR_FIELD_AD_DATA_APPEND) {
		/* Calc the previous AD data length in auxiliary PDU */
		ad_len = sec_pdu_prev->len - sec_len_prev;
		ad_data = sec_dptr_prev;

		/* Append the new ad data fragment */
		ad_fragment_len = *(uint8_t *)hdr_data;
		hdr_data = (uint8_t *)hdr_data + sizeof(ad_fragment_len);
		(void)memcpy(&ad_fragment, hdr_data, sizeof(ad_fragment));
		hdr_data = (uint8_t *)hdr_data + sizeof(ad_fragment);
	} else if (!(sec_hdr_rem_fields & ULL_ADV_PDU_HDR_FIELD_AD_DATA)) {
		/* Calc the previous AD data length in auxiliary PDU */
		ad_len = sec_pdu_prev->len - sec_len_prev;
		ad_data = sec_dptr_prev;
	} else {
		ad_len = 0U;
		ad_data = NULL;
	}

	/* Check Max Advertising Data Length */
	if (ad_len + ad_fragment_len > CONFIG_BT_CTLR_ADV_DATA_LEN_MAX) {
		return BT_HCI_ERR_MEM_CAPACITY_EXCEEDED;
	}

	/* Check AdvData overflow */
	/* TODO: need aux_chain_ind support */
	if ((sec_len + ad_len + ad_fragment_len) > PDU_AC_PAYLOAD_SIZE_MAX) {
		/* return excess length */
		*(uint8_t *)hdr_data = sec_len + ad_len -
				       PDU_AC_PAYLOAD_SIZE_MAX;

		if (pri_pdu == pri_pdu_prev) {
			lll_adv_data_enqueue(&adv->lll, *pri_idx);
		}
		if (sec_pdu == sec_pdu_prev) {
			lll_adv_aux_data_enqueue(adv->lll.aux, *sec_idx);
		}

		/* Will use packet too long error to determine fragmenting
		 * long data
		 */
		return BT_HCI_ERR_PACKET_TOO_LONG;
	}

	/* set the primary PDU len */
	ull_adv_aux_hdr_len_fill(pri_com_hdr, pri_len);
	pri_pdu->len = pri_len;

	/* set the secondary PDU len */
	ull_adv_aux_hdr_len_fill(sec_com_hdr, sec_len);
	sec_pdu->len = sec_len + ad_len + ad_fragment_len;

	/* Start filling pri and sec PDU payload based on flags from here
	 * ==============================================================
	 */

	/* No AdvData in primary channel PDU */
	/* Fill AdvData in secondary PDU */
	(void)memmove(sec_dptr, ad_data, ad_len);

	if (ad_fragment) {
		(void)memcpy(sec_dptr + ad_len, ad_fragment, ad_fragment_len);
	}

	/* Early exit if no flags set */
	if (!sec_com_hdr->ext_hdr_len) {
		return 0;
	}

	/* No ACAD in primary channel PDU */
	/* TODO: Fill ACAD in secondary channel PDU */

	/* Tx Power */
	if (pri_hdr.tx_pwr) {
		*--pri_dptr = *--pri_dptr_prev;
	} else if (sec_hdr.tx_pwr) {
		*--sec_dptr = *--sec_dptr_prev;
	}

#if defined(CONFIG_BT_CTLR_ADV_PERIODIC)
	/* No SyncInfo in primary channel PDU */
	/* Fill SyncInfo in secondary channel PDU */
	if (sec_hdr_prev.sync_info) {
		sec_dptr_prev -= sizeof(*sync_info);
	}

	if (sec_hdr.sync_info) {
		sec_dptr -= sizeof(*sync_info);
	}

	if (sync_info) {
		(void)memmove(sec_dptr, sync_info, sizeof(*sync_info));
	}
#endif /* CONFIG_BT_CTLR_ADV_PERIODIC */

	/* AuxPtr */
	if (pri_hdr_prev.aux_ptr) {
		pri_dptr_prev -= sizeof(struct pdu_adv_aux_ptr);
	}
	pri_dptr -= sizeof(struct pdu_adv_aux_ptr);
	ull_adv_aux_ptr_fill((void *)pri_dptr, 0U, lll->phy_s);

	if (sec_hdr_prev.aux_ptr) {
		sec_dptr_prev -= sizeof(struct pdu_adv_aux_ptr);
	}
	if (sec_hdr.aux_ptr) {
		sec_dptr -= sizeof(struct pdu_adv_aux_ptr);
	}

	if (aux_ptr) {
		(void)memmove(sec_dptr, aux_ptr, sizeof(*aux_ptr));
	}

	/* ADI */
	if (pri_hdr_prev.adi) {
		pri_dptr_prev -= sizeof(struct pdu_adv_adi);
	}
	if (sec_hdr_prev.adi) {
		sec_dptr_prev -= sizeof(struct pdu_adv_adi);
	}

	pri_dptr -= sizeof(struct pdu_adv_adi);
	sec_dptr -= sizeof(struct pdu_adv_adi);

	pri_adi = (void *)pri_dptr;
	sec_adi = (void *)sec_dptr;

	pri_adi->sid = adv->sid;
	sec_adi->sid = adv->sid;

	if (!adi) {
		/* The DID for a specific SID shall be unique.
		 */
		did = ull_adv_aux_did_next_unique_get(adv->sid);
	} else {
		did = adi->did;
	}

	pri_adi->did = sys_cpu_to_le16(did);
	sec_adi->did = sys_cpu_to_le16(did);

	/* No CTEInfo field in primary channel PDU */

	/* No TargetA non-conn non-scan advertising, but present in directed
	 * advertising.
	 */
	if (sec_hdr.tgt_addr) {
		void *bdaddr;

		if (sec_hdr_prev.tgt_addr) {
			sec_dptr_prev -= BDADDR_SIZE;
			bdaddr = sec_dptr_prev;
		} else {
			pri_dptr_prev -= BDADDR_SIZE;
			bdaddr = pri_dptr_prev;
		}

		sec_dptr -= BDADDR_SIZE;

		(void)memcpy(sec_dptr, bdaddr, BDADDR_SIZE);
	}

	/* No AdvA in primary channel due to AuxPtr being added */

	/* NOTE: AdvA in aux channel is also filled at enable and RPA
	 * timeout
	 */
	if (sec_hdr.adv_addr) {
		void *bdaddr;

		if (sec_hdr_prev.adv_addr) {
			sec_dptr_prev -= BDADDR_SIZE;
			bdaddr = sec_dptr_prev;
		} else {
			pri_dptr_prev -= BDADDR_SIZE;
			bdaddr = pri_dptr_prev;
		}

		sec_dptr -= BDADDR_SIZE;

		(void)memcpy(sec_dptr, bdaddr, BDADDR_SIZE);
	}

	/* Set the common extended header format flags in the current primary
	 * PDU
	 */
	if (pri_com_hdr->ext_hdr_len != 0) {
		pri_com_hdr->ext_hdr = pri_hdr;
	}

	/* Set the common extended header format flags in the current secondary
	 * PDU
	 */
	if (sec_com_hdr->ext_hdr_len != 0) {
		sec_com_hdr->ext_hdr = sec_hdr;
	}

#if defined(CONFIG_BT_CTLR_ADV_AUX_PDU_LINK)
	ull_adv_aux_chain_pdu_duplicate(sec_pdu_prev, sec_pdu, aux_ptr,
					adv->lll.phy_s, adv->lll.phy_flags,
					EVENT_B2B_MAFS_US);
#endif /* CONFIG_BT_CTLR_ADV_AUX_PDU_LINK */

	/* Update auxiliary channel event time reservation */
	if (aux->is_started) {
		struct pdu_adv *pdu_scan;
		uint8_t err;

		pdu_scan = lll_adv_scan_rsp_peek(lll);
		err = aux_time_update(aux, sec_pdu, pdu_scan);
		if (err) {
			return err;
		}
	}

	return 0;
}

uint8_t ull_adv_aux_pdu_set_clear(struct ll_adv_set *adv,
				  struct pdu_adv *pdu_prev,
				  struct pdu_adv *pdu,
				  uint16_t hdr_add_fields,
				  uint16_t hdr_rem_fields,
				  void *hdr_data)
{
	struct pdu_adv_com_ext_adv *com_hdr, *com_hdr_prev;
	struct pdu_adv_ext_hdr hdr = { 0 }, hdr_prev = { 0 };
	struct pdu_adv_aux_ptr *aux_ptr, *aux_ptr_prev;
	uint8_t *ad_fragment = NULL;
	uint8_t ad_fragment_len = 0;
	uint8_t *dptr, *dptr_prev;
	struct pdu_adv_adi *adi;
	uint8_t acad_len_prev;
	uint8_t hdr_buf_len;
	uint8_t len_prev;
	uint8_t *ad_data;
	uint8_t acad_len;
#if defined(CONFIG_BT_CTLR_DF_ADV_CTE_TX)
	uint8_t cte_info;
#endif /* CONFIG_BT_CTLR_DF_ADV_CTE_TX */
	uint8_t ad_len;
	uint16_t len;

	/* Get common pointers from reference to previous tertiary PDU data */
	com_hdr_prev = (void *)&pdu_prev->adv_ext_ind;
	if (pdu_prev->len && com_hdr_prev->ext_hdr_len) {
		hdr_prev = com_hdr_prev->ext_hdr;
	} else {
		com_hdr_prev->ext_hdr_len = 0U;
	}
	dptr_prev = com_hdr_prev->ext_hdr.data;

	/* Set common fields in reference to new tertiary PDU data buffer */
	pdu->type = pdu_prev->type;
	pdu->rfu = 0U;
	pdu->chan_sel = 0U;

	pdu->tx_addr = pdu_prev->tx_addr;
	pdu->rx_addr = pdu_prev->rx_addr;

	/* Get common pointers from current tertiary PDU data.
	 * It is possible that the current tertiary is the same as
	 * previous one. It may happen if update periodic advertising
	 * chain in place.
	 */
	com_hdr = (void *)&pdu->adv_ext_ind;
	com_hdr->adv_mode = com_hdr_prev->adv_mode;
	dptr = com_hdr->ext_hdr.data;

	/* AdvA flag */
	if (hdr_add_fields & ULL_ADV_PDU_HDR_FIELD_ADVA) {
		hdr.adv_addr = 1U;
		dptr += BDADDR_SIZE;
	} else if (!(hdr_rem_fields & ULL_ADV_PDU_HDR_FIELD_ADVA) &&
		   hdr_prev.adv_addr) {
		hdr.adv_addr = 1U;
		pdu->tx_addr = pdu_prev->tx_addr;

		dptr += BDADDR_SIZE;
	}
	if (hdr_prev.adv_addr) {
		dptr_prev += BDADDR_SIZE;
	}

	/* TargetA flag */
	if (hdr_add_fields & ULL_ADV_PDU_HDR_FIELD_TARGETA) {
		hdr.tgt_addr = 1U;
		dptr += BDADDR_SIZE;
	} else if (!(hdr_rem_fields & ULL_ADV_PDU_HDR_FIELD_TARGETA) &&
		   hdr_prev.tgt_addr) {
		hdr.tgt_addr = 1U;
		pdu->rx_addr = pdu_prev->rx_addr;

		dptr += BDADDR_SIZE;
	}
	if (hdr_prev.tgt_addr) {
		dptr_prev += BDADDR_SIZE;
	}

#if defined(CONFIG_BT_CTLR_DF_ADV_CTE_TX)
	/* If requested add or update CTEInfo */
	if (hdr_add_fields & ULL_ADV_PDU_HDR_FIELD_CTE_INFO) {
		hdr.cte_info = 1;
		cte_info = *(uint8_t *)hdr_data;
		hdr_data = (uint8_t *)hdr_data + 1;
		dptr += sizeof(struct pdu_cte_info);
	/* If CTEInfo exists in prev and is not requested to be removed */
	} else if (!(hdr_rem_fields & ULL_ADV_PDU_HDR_FIELD_CTE_INFO) &&
		   hdr_prev.cte_info) {
		hdr.cte_info = 1;
		cte_info = 0U; /* value not used, will be read from prev PDU */
		dptr += sizeof(struct pdu_cte_info);
	} else {
		cte_info = 0U; /* value not used */
	}

	/* If CTEInfo exists in prev PDU */
	if (hdr_prev.cte_info) {
		dptr_prev += sizeof(struct pdu_cte_info);
	}
#endif /* CONFIG_BT_CTLR_DF_ADV_CTE_TX */

	/* ADI */
	if (hdr_add_fields & ULL_ADV_PDU_HDR_FIELD_ADI) {
		hdr.adi = 1U;
		/* return the size of ADI structure */
		*(uint8_t *)hdr_data = sizeof(struct pdu_adv_adi);
		hdr_data = (uint8_t *)hdr_data + sizeof(uint8_t);
		/* pick the reference to ADI param */
		(void)memcpy(&adi, hdr_data, sizeof(struct pdu_adv_adi *));
		/* return the pointer to ADI struct inside the PDU */
		(void)memcpy(hdr_data, &dptr, sizeof(dptr));
		hdr_data = (uint8_t *)hdr_data + sizeof(dptr);
		dptr += sizeof(struct pdu_adv_adi);
	} else if (!(hdr_rem_fields & ULL_ADV_PDU_HDR_FIELD_ADI) &&
		   hdr_prev.adi) {
		hdr.adi = 1U;
		adi = (void *)dptr_prev;
		dptr += sizeof(struct pdu_adv_adi);
	} else {
		adi = NULL;
	}
	if (hdr_prev.adi) {
		dptr_prev += sizeof(struct pdu_adv_adi);
	}

	/* AuxPtr - will be added if AUX_CHAIN_IND is required */
	if (hdr_add_fields & ULL_ADV_PDU_HDR_FIELD_AUX_PTR) {
		hdr.aux_ptr = 1;
		aux_ptr_prev = NULL;
		aux_ptr = (void *)dptr;

		/* return the size of aux pointer structure */
		*(uint8_t *)hdr_data = sizeof(struct pdu_adv_aux_ptr);
		hdr_data = (uint8_t *)hdr_data + sizeof(uint8_t);

		/* return the pointer to aux pointer struct inside the PDU
		 * buffer
		 */
		(void)memcpy(hdr_data, &dptr, sizeof(dptr));
		hdr_data = (uint8_t *)hdr_data + sizeof(dptr);
	} else if (!(hdr_rem_fields & ULL_ADV_PDU_HDR_FIELD_AUX_PTR) &&
		   hdr_prev.aux_ptr) {
		hdr.aux_ptr = 1;
		aux_ptr_prev = (void *)dptr_prev;
		aux_ptr = (void *)dptr;
	} else {
		aux_ptr_prev = NULL;
		aux_ptr = NULL;
	}
	if (hdr_prev.aux_ptr) {
		dptr_prev += sizeof(struct pdu_adv_aux_ptr);
	}
	if (hdr.aux_ptr) {
		dptr += sizeof(struct pdu_adv_aux_ptr);
	}

	/* SyncInfo flag */
	if (hdr_prev.sync_info) {
		hdr.sync_info = 1;
		dptr_prev += sizeof(struct pdu_adv_sync_info);
		dptr += sizeof(struct pdu_adv_sync_info);
	}

	/* Tx Power flag */
	if (hdr_prev.tx_pwr) {
		dptr_prev++;

		hdr.tx_pwr = 1;
		dptr++;
	}

	/* Calc previous ACAD len and update PDU len */
	len_prev = dptr_prev - (uint8_t *)com_hdr_prev;
	hdr_buf_len = com_hdr_prev->ext_hdr_len +
		      PDU_AC_EXT_HEADER_SIZE_MIN;
	if (len_prev <= hdr_buf_len) {
		/* There are some data, except ACAD, in extended header if len_prev
		 * equals to hdr_buf_len. There is ACAD if the size of len_prev
		 * is smaller than hdr_buf_len.
		 */
		acad_len_prev = hdr_buf_len - len_prev;
		len_prev += acad_len_prev;
		dptr_prev += acad_len_prev;
	} else {
		/* There are no data in extended header, all flags are zeros. */
		acad_len_prev = 0;
		/* NOTE: If no flags are set then extended header length will be
		 *       zero. Under this condition the current len_prev
		 *       value will be greater than extended header length,
		 *       hence set len_prev to size of the length/mode
		 *       field.
		 */
		len_prev = (pdu_prev->len) ? PDU_AC_EXT_HEADER_SIZE_MIN : 0U;
		dptr_prev = (uint8_t *)com_hdr_prev + len_prev;
	}

	/* Did we parse beyond PDU length? */
	if (len_prev > pdu_prev->len) {
		/* we should not encounter invalid length */
		return BT_HCI_ERR_UNSPECIFIED;
	}

	/* Add/Retain/Remove ACAD */
	if (hdr_add_fields & ULL_ADV_PDU_HDR_FIELD_ACAD) {
		acad_len = *(uint8_t *)hdr_data;
		/* If zero length ACAD then do not reduce ACAD but return
		 * return previous ACAD length.
		 */
		if (!acad_len) {
			acad_len = acad_len_prev;
		}
		/* return prev ACAD length */
		*(uint8_t *)hdr_data = acad_len_prev;
		hdr_data = (uint8_t *)hdr_data + 1;
		/* return the pointer to ACAD offset */
		(void)memcpy(hdr_data, &dptr, sizeof(dptr));
		hdr_data = (uint8_t *)hdr_data + sizeof(dptr);
		dptr += acad_len;
	} else if (!(hdr_rem_fields & ULL_ADV_PDU_HDR_FIELD_ACAD)) {
		acad_len = acad_len_prev;
		dptr += acad_len_prev;
	} else {
		acad_len = 0U;
	}

	/* Calc current tertiary PDU len so far without AD data added */
	len = ull_adv_aux_hdr_len_calc(com_hdr, &dptr);

	/* Get Adv data from function parameters */
	if (hdr_add_fields & ULL_ADV_PDU_HDR_FIELD_AD_DATA) {
		uint8_t ad_len_prev;

		/* remember the new ad data len */
		ad_len = *(uint8_t *)hdr_data;

		/* return prev ad data length */
		ad_len_prev = pdu_prev->len - len_prev;
		*(uint8_t *)hdr_data = ad_len_prev;
		hdr_data = (uint8_t *)hdr_data + sizeof(ad_len);

		/* remember the reference to new ad data */
		(void)memcpy(&ad_data, hdr_data, sizeof(ad_data));

		/* return the reference to prev ad data */
		(void)memcpy(hdr_data, &dptr_prev, sizeof(dptr_prev));
		hdr_data = (uint8_t *)hdr_data + sizeof(dptr_prev);

		/* unchanged data */
		if (!ad_len && !ad_data) {
			ad_len = ad_len_prev;
			ad_data = dptr_prev;
		}
	} else if (hdr_add_fields & ULL_ADV_PDU_HDR_FIELD_AD_DATA_APPEND) {
		ad_len = pdu_prev->len - len_prev;
		ad_data = dptr_prev;

		/* Append the new ad data fragment */
		ad_fragment_len = *(uint8_t *)hdr_data;
		hdr_data = (uint8_t *)hdr_data + sizeof(ad_fragment_len);
		(void)memcpy(&ad_fragment, hdr_data, sizeof(ad_fragment));
		hdr_data = (uint8_t *)hdr_data + sizeof(ad_fragment);
	} else if (!(hdr_rem_fields & ULL_ADV_PDU_HDR_FIELD_AD_DATA)) {
		ad_len = pdu_prev->len - len_prev;
		ad_data = dptr_prev;
	} else {
		ad_len = 0;
		ad_data = NULL;
	}

	/* Check Max Advertising Data Length */
	if (ad_len + ad_fragment_len > CONFIG_BT_CTLR_ADV_DATA_LEN_MAX) {
		return BT_HCI_ERR_MEM_CAPACITY_EXCEEDED;
	}

	/* Check AdvData overflow */
	if ((len + ad_len + ad_fragment_len) > PDU_AC_PAYLOAD_SIZE_MAX) {
		/* return excess length */
		*(uint8_t *)hdr_data = len + ad_len -
				       PDU_AC_PAYLOAD_SIZE_MAX;

		/* Will use packet too long error to determine fragmenting
		 * long data
		 */
		return BT_HCI_ERR_PACKET_TOO_LONG;
	}

	/* set the tertiary extended header and PDU length */
	ull_adv_aux_hdr_len_fill(com_hdr, len);
	pdu->len = len + ad_len + ad_fragment_len;

	/* Start filling tertiary PDU payload based on flags from here
	 * ==============================================================
	 */

	/* Fill AdvData in tertiary PDU */
	(void)memmove(dptr, ad_data, ad_len);

	if (ad_fragment) {
		(void)memcpy(dptr + ad_len, ad_fragment, ad_fragment_len);
	}

	/* Early exit if no flags set */
	if (!com_hdr->ext_hdr_len) {
		return 0;
	}

	/* Retain ACAD in tertiary PDU */
	dptr_prev -= acad_len_prev;
	if (acad_len) {
		dptr -= acad_len;
		(void)memmove(dptr, dptr_prev, acad_len_prev);
	}

	/* Tx Power */
	if (hdr.tx_pwr) {
		*--dptr = *--dptr_prev;
	}

	/* SyncInfo */
	if (hdr.sync_info) {
		dptr_prev -= sizeof(struct pdu_adv_sync_info);
		dptr -= sizeof(struct pdu_adv_sync_info);

		(void)memmove(dptr, dptr_prev,
			      sizeof(struct pdu_adv_sync_info));
	}

	/* AuxPtr */
	if (hdr_prev.aux_ptr) {
		dptr_prev -= sizeof(struct pdu_adv_aux_ptr);
	}
	if (hdr.aux_ptr) {
		dptr -= sizeof(struct pdu_adv_aux_ptr);
	}
	if (aux_ptr_prev) {
		(void)memmove(dptr, aux_ptr_prev, sizeof(*aux_ptr_prev));
	}

	/* ADI */
	if (hdr_prev.adi) {
		dptr_prev -= sizeof(struct pdu_adv_adi);
	}
	if (hdr.adi) {
		struct pdu_adv_adi *adi_pdu;

		dptr -= sizeof(struct pdu_adv_adi);
		adi_pdu = (void *)dptr;

		if (!adi) {
			adi_pdu->sid = adv->sid;

			/* The DID for a specific SID shall be unique.
			 */
			const uint16_t did =
				ull_adv_aux_did_next_unique_get(adv->sid);
			adi_pdu->did = sys_cpu_to_le16(did);
		} else {
			adi_pdu->sid = adi->sid;
			adi_pdu->did = adi->did;
		}
	}

#if defined(CONFIG_BT_CTLR_DF_ADV_CTE_TX)
	if (hdr.cte_info) {
		if (hdr_add_fields & ULL_ADV_PDU_HDR_FIELD_CTE_INFO) {
			*--dptr = cte_info;
		} else {
			*--dptr = *--dptr_prev;
		}
	}
#endif /* CONFIG_BT_CTLR_DF_ADV_CTE_TX */

	/* No TargetA in non-conn non-scan advertising, but present in directed
	 * advertising.
	 */
	if (hdr.tgt_addr) {
		dptr_prev -= BDADDR_SIZE;
		dptr -= BDADDR_SIZE;

		(void)memmove(dptr, dptr_prev, BDADDR_SIZE);
	}

	/* NOTE: AdvA in aux channel is also filled at enable and RPA
	 * timeout
	 */
	if (hdr.adv_addr) {
		dptr_prev -= BDADDR_SIZE;
		dptr -= BDADDR_SIZE;

		(void)memmove(dptr, dptr_prev, BDADDR_SIZE);
	}

	if (com_hdr->ext_hdr_len != 0) {
		com_hdr->ext_hdr = hdr;
	}

	return 0;
}

uint16_t ull_adv_aux_did_next_unique_get(uint8_t sid)
{
	/* The DID is 12 bits and did_unique may overflow without any negative
	 * consequences.
	 */
	return BIT_MASK(12) & did_unique[sid]++;
}

void ull_adv_aux_ptr_fill(struct pdu_adv_aux_ptr *aux_ptr, uint32_t offs_us,
			  uint8_t phy_s)
{
	uint32_t offs;
	uint8_t phy;

	/* NOTE: Channel Index and Aux Offset will be set on every advertiser's
	 * event prepare when finding the auxiliary event's ticker offset.
	 * Here we fill initial values.
	 */
	aux_ptr->chan_idx = 0U;

	aux_ptr->ca = (lll_clock_ppm_local_get() <= SCA_50_PPM) ?
		      SCA_VALUE_50_PPM : SCA_VALUE_500_PPM;

	offs = offs_us / OFFS_UNIT_30_US;
	if (!!(offs >> OFFS_UNIT_BITS)) {
		offs = offs / (OFFS_UNIT_300_US / OFFS_UNIT_30_US);
		aux_ptr->offs_units = OFFS_UNIT_VALUE_300_US;
	} else {
		aux_ptr->offs_units = OFFS_UNIT_VALUE_30_US;
	}
	phy = find_lsb_set(phy_s) - 1;

	aux_ptr->offs_phy_packed[0] = offs & 0xFF;
	aux_ptr->offs_phy_packed[1] = ((offs>>8) & 0x1F) + (phy << 5);
}

#if (CONFIG_BT_CTLR_ADV_AUX_SET > 0)
inline uint8_t ull_adv_aux_handle_get(struct ll_adv_aux_set *aux)
{
	return mem_index_get(aux, ll_adv_aux_pool,
			     sizeof(struct ll_adv_aux_set));
}

uint8_t ull_adv_aux_lll_handle_get(struct lll_adv_aux *lll)
{
	return ull_adv_aux_handle_get((void *)lll->hdr.parent);
}

uint32_t ull_adv_aux_evt_init(struct ll_adv_aux_set *aux,
			      uint32_t *ticks_anchor)
{
	uint32_t ticks_slot_overhead;
	uint32_t time_us;

	time_us = aux_time_min_get(aux);

	/* TODO: active_to_start feature port */
	aux->ull.ticks_active_to_start = 0;
	aux->ull.ticks_prepare_to_start =
		HAL_TICKER_US_TO_TICKS(EVENT_OVERHEAD_XTAL_US);
	aux->ull.ticks_preempt_to_start =
		HAL_TICKER_US_TO_TICKS(EVENT_OVERHEAD_PREEMPT_MIN_US);
	aux->ull.ticks_slot = HAL_TICKER_US_TO_TICKS(time_us);

	if (IS_ENABLED(CONFIG_BT_CTLR_LOW_LAT)) {
		ticks_slot_overhead = MAX(aux->ull.ticks_active_to_start,
					  aux->ull.ticks_prepare_to_start);
	} else {
		ticks_slot_overhead = 0;
	}

#if defined(CONFIG_BT_CTLR_SCHED_ADVANCED)
	uint32_t ticks_anchor_aux;
	uint32_t ticks_slot;
	int err;

#if defined(CONFIG_BT_CTLR_ADV_RESERVE_MAX)
	time_us = ull_adv_aux_time_get(aux, PDU_AC_PAYLOAD_SIZE_MAX,
				       PDU_AC_PAYLOAD_SIZE_MAX);
	ticks_slot = HAL_TICKER_US_TO_TICKS(time_us);
#else
	ticks_slot = aux->ull.ticks_slot;
#endif

	err = ull_sched_adv_aux_sync_free_slot_get(TICKER_USER_ID_THREAD,
						   (ticks_slot +
						    ticks_slot_overhead),
						   &ticks_anchor_aux);
	if (!err) {
		*ticks_anchor = ticks_anchor_aux;
		*ticks_anchor += HAL_TICKER_US_TO_TICKS(
					MAX(EVENT_MAFS_US,
					    EVENT_OVERHEAD_START_US) -
					EVENT_OVERHEAD_START_US +
					(EVENT_TICKER_RES_MARGIN_US << 1));
	}
#endif /* CONFIG_BT_CTLR_SCHED_ADVANCED */

	return ticks_slot_overhead;
}

#if defined(CONFIG_BT_CTLR_ADV_PERIODIC)
static void ticker_update_op_cb(uint32_t status, void *param)
{
	LL_ASSERT(status == TICKER_STATUS_SUCCESS ||
		  param == ull_disable_mark_get());
}

void ull_adv_sync_started_stopped(struct ll_adv_aux_set *aux)
{
	if (aux->is_started) {
		struct lll_adv_sync *lll_sync = aux->lll.adv->sync;
		struct ll_adv_sync_set *sync;
		uint8_t aux_handle;

		LL_ASSERT(lll_sync);

		sync = HDR_LLL2ULL(lll_sync);
		aux_handle = ull_adv_aux_handle_get(aux);

		if (sync->is_started) {
			uint8_t sync_handle = ull_adv_sync_handle_get(sync);

			ticker_update_ext(TICKER_INSTANCE_ID_CTLR, TICKER_USER_ID_THREAD,
			   (TICKER_ID_ADV_AUX_BASE + aux_handle), 0, 0, 0, 0, 0, 0,
			   ticker_update_op_cb, aux, 0,
			   TICKER_ID_ADV_SYNC_BASE + sync_handle);
		} else {
			ticker_update_ext(TICKER_INSTANCE_ID_CTLR, TICKER_USER_ID_THREAD,
			   (TICKER_ID_ADV_AUX_BASE + aux_handle), 0, 0, 0, 0, 0, 0,
			   ticker_update_op_cb, aux, 0,
			   TICKER_NULL);
		}
	}
}
#endif /* CONFIG_BT_CTLR_ADV_PERIODIC */

uint32_t ull_adv_aux_start(struct ll_adv_aux_set *aux, uint32_t ticks_anchor,
			   uint32_t ticks_slot_overhead)
{
	uint32_t volatile ret_cb;
	uint32_t interval_us;
	uint8_t aux_handle;
	uint32_t ret;

	ull_hdr_init(&aux->ull);
	aux_handle = ull_adv_aux_handle_get(aux);
	interval_us = aux->interval * PERIODIC_INT_UNIT_US;

#if defined(CONFIG_BT_CTLR_ADV_PERIODIC)
	if (aux->lll.adv->sync) {
		struct ll_adv_sync_set *sync = HDR_LLL2ULL(aux->lll.adv->sync);
		uint8_t sync_handle = ull_adv_sync_handle_get(sync);

		ll_adv_aux_ticker_ext[aux_handle].expire_info_id = TICKER_ID_ADV_SYNC_BASE +
								  sync_handle;
	} else {
		ll_adv_aux_ticker_ext[aux_handle].expire_info_id = TICKER_NULL;
	}

	ll_adv_aux_ticker_ext[aux_handle].ext_timeout_func = ticker_cb;
#endif /* CONFIG_BT_CTLR_ADV_PERIODIC */

	ret_cb = TICKER_STATUS_BUSY;
	ret = ticker_start_ext(
			   TICKER_INSTANCE_ID_CTLR, TICKER_USER_ID_THREAD,
			   (TICKER_ID_ADV_AUX_BASE + aux_handle),
			   ticks_anchor, 0U,
			   HAL_TICKER_US_TO_TICKS(interval_us),
			   HAL_TICKER_REMAINDER(interval_us), TICKER_NULL_LAZY,
			   (aux->ull.ticks_slot + ticks_slot_overhead),
			   ticker_cb,
			   aux,
			   ull_ticker_status_give, (void *)&ret_cb,
#if defined(CONFIG_BT_CTLR_ADV_PERIODIC)
			   &ll_adv_aux_ticker_ext[aux_handle]);
#else
			   NULL);
#endif /* CONFIG_BT_CTLR_ADV_PERIODIC */

	ret = ull_ticker_status_take(ret, &ret_cb);

	return ret;
}

int ull_adv_aux_stop(struct ll_adv_aux_set *aux)
{
	uint8_t aux_handle;
	int err;

	aux_handle = ull_adv_aux_handle_get(aux);

	err = ull_ticker_stop_with_mark(TICKER_ID_ADV_AUX_BASE + aux_handle,
					aux, &aux->lll);
	LL_ASSERT(err == 0 || err == -EALREADY);
	if (err) {
		return err;
	}

	aux->is_started = 0U;

	return 0;
}

struct ll_adv_aux_set *ull_adv_aux_acquire(struct lll_adv *lll)
{
	struct lll_adv_aux *lll_aux;
	struct ll_adv_aux_set *aux;
	uint8_t chm_last;
	int err;

	aux = aux_acquire();
	if (!aux) {
		return aux;
	}

	lll_aux = &aux->lll;
	lll->aux = lll_aux;
	lll_aux->adv = lll;

	lll_adv_data_reset(&lll_aux->data);
	err = lll_adv_aux_data_init(&lll_aux->data);
	if (err) {
		return NULL;
	}

	/* Initialize data channel calculation counter, data channel identifier,
	 * and channel map to use.
	 */
	lll_csrand_get(&lll_aux->data_chan_counter,
		       sizeof(lll_aux->data_chan_counter));
	lll_csrand_get(&aux->data_chan_id, sizeof(aux->data_chan_id));
	chm_last = aux->chm_first;
	aux->chm_last = chm_last;
	aux->chm[chm_last].data_chan_count =
		ull_chan_map_get(aux->chm[chm_last].data_chan_map);


	/* NOTE: ull_hdr_init(&aux->ull); is done on start */
	lll_hdr_init(lll_aux, aux);

	aux->is_started = 0U;

	return aux;
}

void ull_adv_aux_release(struct ll_adv_aux_set *aux)
{
	lll_adv_data_release(&aux->lll.data);
	aux_release(aux);
}

struct ll_adv_aux_set *ull_adv_aux_get(uint8_t handle)
{
	if (handle >= CONFIG_BT_CTLR_ADV_AUX_SET) {
		return NULL;
	}

	return &ll_adv_aux_pool[handle];
}

uint32_t ull_adv_aux_time_get(const struct ll_adv_aux_set *aux, uint8_t pdu_len,
			      uint8_t pdu_scan_len)
{
	const struct pdu_adv *pdu;

	pdu = lll_adv_aux_data_peek(&aux->lll);

	return aux_time_get(aux, pdu, pdu_len, pdu_scan_len);
}

struct pdu_adv_aux_ptr *ull_adv_aux_lll_offset_fill(struct pdu_adv *pdu,
						    uint32_t ticks_offset,
						    uint32_t remainder_us,
						    uint32_t start_us)
{
	struct pdu_adv_com_ext_adv *pri_com_hdr;
	struct pdu_adv_aux_ptr *aux_ptr;
	struct pdu_adv_ext_hdr *h;
	uint32_t offs;
	uint8_t *ptr;

	pri_com_hdr = (void *)&pdu->adv_ext_ind;
	h = (void *)pri_com_hdr->ext_hdr_adv_data;
	ptr = h->data;

	/* traverse through adv_addr, if present */
	if (h->adv_addr) {
		ptr += BDADDR_SIZE;
	}

	/* traverse through tgt_addr, if present */
	if (h->tgt_addr) {
		ptr += BDADDR_SIZE;
	}

	/* No CTEInfo flag in primary and secondary channel PDU */

	/* traverse through adi, if present */
	if (h->adi) {
		ptr += sizeof(struct pdu_adv_adi);
	}

	aux_ptr = (void *)ptr;
	offs = HAL_TICKER_TICKS_TO_US(ticks_offset) + remainder_us - start_us;
	offs = offs / OFFS_UNIT_30_US;
	if (!!(offs >> OFFS_UNIT_BITS)) {
		offs = offs / (OFFS_UNIT_300_US / OFFS_UNIT_30_US);
		aux_ptr->offs_units = OFFS_UNIT_VALUE_300_US;
	} else {
		aux_ptr->offs_units = OFFS_UNIT_VALUE_30_US;
	}
	aux_ptr->offs_phy_packed[0] = offs & 0xFF;
	aux_ptr->offs_phy_packed[1] = ((offs>>8) & 0x1F) + (aux_ptr->offs_phy_packed[1] & 0xE0);

	return aux_ptr;
}

void ull_adv_aux_done(struct node_rx_event_done *done)
{
	struct lll_adv_aux *lll_aux;
	struct ll_adv_aux_set *aux;
	struct ll_adv_set *adv;

	/* Get reference to ULL context */
	aux = CONTAINER_OF(done->param, struct ll_adv_aux_set, ull);
	lll_aux = &aux->lll;
	adv = HDR_LLL2ULL(lll_aux->adv);

	/* Call the primary channel advertising done */
	done->param = &adv->ull;
	ull_adv_done(done);
}

#if defined(CONFIG_BT_CTLR_ADV_PDU_LINK)
/* @brief Duplicate previous chain of PDUs into current chain of PDUs, fill the
 *        aux ptr field of the parent primary channel PDU with the aux offset,
 *        and the secondary channel PDU's PHY.
 *
 * @param[in] pdu_prev    Pointer to previous PDU's chain PDU
 * @param[in] pdu         Pointer to current PDU's chain PDU
 * @param[in] aux_ptr     Pointer to aux ptr field in the primary channel PDU
 * @param[in] phy_s       Secondary/auxiliary PDU PHY
 * @param[in] phy_flags   Secondary/auxiliary PDU coded PHY encoding (S2/S8)
 * @param[in] mafs_us     Minimum Aux Frame Spacing to use, in microseconds
 */
void ull_adv_aux_chain_pdu_duplicate(struct pdu_adv *pdu_prev,
				     struct pdu_adv *pdu,
				     struct pdu_adv_aux_ptr *aux_ptr,
				     uint8_t phy_s, uint8_t phy_flags,
				     uint32_t mafs_us)
{
	/* Duplicate any chain PDUs */
	while (aux_ptr) {
		struct pdu_adv_com_ext_adv *com_hdr_chain;
		struct pdu_adv_com_ext_adv *com_hdr;
		struct pdu_adv_ext_hdr *hdr_chain;
		struct pdu_adv_adi *adi_parent;
		struct pdu_adv *pdu_chain_prev;
		struct pdu_adv_ext_hdr *hdr;
		struct pdu_adv *pdu_chain;
		uint8_t *dptr_chain;
		uint32_t offs_us;
		uint8_t *dptr;

		/* Get the next chain PDU */
		pdu_chain_prev = lll_adv_pdu_linked_next_get(pdu_prev);
		if (!pdu_chain_prev) {
			break;
		}

		/* Fill the aux offset in the (first iteration, it is the
		 * primary channel ADV_EXT_IND PDU, rest it is AUX_ADV_IND and
		 * AUX_CHAIN_IND) parent PDU
		 */
		offs_us = PDU_AC_US(pdu->len, phy_s, phy_flags) + mafs_us;
		ull_adv_aux_ptr_fill(aux_ptr, offs_us, phy_s);

		/* Get reference to flags in superior PDU */
		com_hdr = &pdu->adv_ext_ind;
		hdr = (void *)&com_hdr->ext_hdr_adv_data[0];
		dptr = (void *)hdr;

		/* Get the next new chain PDU */
		pdu_chain = lll_adv_pdu_linked_next_get(pdu);
		if (!pdu_chain) {
			/* Get a new chain PDU */
			pdu_chain = lll_adv_pdu_alloc_pdu_adv();
			LL_ASSERT(pdu_chain);

			/* Copy previous chain PDU into new chain PDU */
			(void)memcpy(pdu_chain, pdu_chain_prev,
				     offsetof(struct pdu_adv, payload) +
				     pdu_chain_prev->len);

			/* Link the chain PDU to parent PDU */
			lll_adv_pdu_linked_append(pdu_chain, pdu);
		}

		/* Get reference to common header format */
		com_hdr_chain = &pdu_chain_prev->adv_ext_ind;
		hdr_chain = (void *)&com_hdr_chain->ext_hdr_adv_data[0];
		dptr_chain = (void *)hdr_chain;

		/* Check for no Flags */
		if (!com_hdr_chain->ext_hdr_len) {
			break;
		}

		/* Proceed to next byte if any flags present */
		if (*dptr) {
			dptr++;
		}
		if (*dptr_chain) {
			dptr_chain++;
		}

		/* AdvA flag */
		if (hdr->adv_addr) {
			dptr += BDADDR_SIZE;
		}
		if (hdr_chain->adv_addr) {
			dptr_chain += BDADDR_SIZE;
		}

		/* TgtA flag */
		if (hdr->tgt_addr) {
			dptr += BDADDR_SIZE;
		}
		if (hdr_chain->tgt_addr) {
			dptr_chain += BDADDR_SIZE;
		}

		/* CTE Info */
		if (hdr->cte_info) {
			dptr += sizeof(struct pdu_cte_info);
		}
		if (hdr_chain->cte_info) {
			dptr_chain += sizeof(struct pdu_cte_info);
		}

		/* ADI */
		if (hdr->adi) {
			adi_parent = (void *)dptr;

			dptr += sizeof(struct pdu_adv_adi);
		} else {
			adi_parent = NULL;
		}
		if (hdr_chain->adi) {
			struct pdu_adv_adi *adi;

			/* update DID to superior PDU DID */
			adi = (void *)dptr_chain;
			if (adi_parent) {
				adi->did = adi_parent->did;
			}

			dptr_chain += sizeof(struct pdu_adv_adi);
		}

		/* No aux ptr, no further chain PDUs */
		if (!hdr_chain->aux_ptr) {
			break;
		}

		/* Remember the aux ptr to be populated */
		aux_ptr = (void *)dptr_chain;

		/* Progress to next chain PDU */
		pdu_prev = pdu_chain_prev;
		pdu = pdu_chain;
	}
}
#endif /* CONFIG_BT_CTLR_ADV_PDU_LINK */

static int init_reset(void)
{
	/* Initialize adv aux pool. */
	mem_init(ll_adv_aux_pool, sizeof(struct ll_adv_aux_set),
		 sizeof(ll_adv_aux_pool) / sizeof(struct ll_adv_aux_set),
		 &adv_aux_free);

	return 0;
}

static inline struct ll_adv_aux_set *aux_acquire(void)
{
	return mem_acquire(&adv_aux_free);
}

static inline void aux_release(struct ll_adv_aux_set *aux)
{
	mem_release(aux, &adv_aux_free);
}

static uint32_t aux_time_get(const struct ll_adv_aux_set *aux,
			     const struct pdu_adv *pdu,
			     uint8_t pdu_len, uint8_t pdu_scan_len)
{
	const struct lll_adv_aux *lll_aux;
	const struct lll_adv *lll;
	uint32_t time_us;

	lll_aux = &aux->lll;
	lll = lll_aux->adv;

	if (IS_ENABLED(CONFIG_BT_CTLR_ADV_RESERVE_MAX) &&
	    (lll->phy_s == PHY_CODED)) {
		pdu_len = PDU_AC_EXT_PAYLOAD_OVERHEAD;
		pdu_scan_len = PDU_AC_EXT_PAYLOAD_OVERHEAD;
	}

	/* NOTE: 16-bit values are sufficient for minimum radio event time
	 *       reservation, 32-bit are used here so that reservations for
	 *       whole back-to-back chaining of PDUs can be accomodated where
	 *       the required microseconds could overflow 16-bits, example,
	 *       back-to-back chained Coded PHY PDUs.
	 */
	time_us = PDU_AC_US(pdu_len, lll->phy_s, lll->phy_flags) +
		  EVENT_OVERHEAD_START_US + EVENT_OVERHEAD_END_US;

	if ((pdu->adv_ext_ind.adv_mode & BT_HCI_LE_ADV_PROP_CONN) ==
	    BT_HCI_LE_ADV_PROP_CONN) {
		const uint16_t conn_req_us =
			PDU_AC_MAX_US((INITA_SIZE + ADVA_SIZE + LLDATA_SIZE),
				      lll->phy_s);
		const uint16_t conn_rsp_us =
			PDU_AC_US((PDU_AC_EXT_HEADER_SIZE_MIN + ADVA_SIZE +
				   TARGETA_SIZE), lll->phy_s, lll->phy_flags);

		time_us += EVENT_IFS_MAX_US * 2 + conn_req_us + conn_rsp_us;
	} else if ((pdu->adv_ext_ind.adv_mode & BT_HCI_LE_ADV_PROP_SCAN) ==
		   BT_HCI_LE_ADV_PROP_SCAN) {
		const uint16_t scan_req_us  =
			PDU_AC_MAX_US((SCANA_SIZE + ADVA_SIZE), lll->phy_s);
		const uint16_t scan_rsp_us =
			PDU_AC_US(pdu_scan_len, lll->phy_s, lll->phy_flags);

		time_us += EVENT_IFS_MAX_US * 2 + scan_req_us + scan_rsp_us;

		/* FIXME: Calculate additional time reservations for scan
		 *        response chain PDUs, if any.
		 */
	} else {
		/* Non-connectable Non-Scannable */

		/* FIXME: Calculate additional time reservations for chain PDUs,
		 *        if any.
		 */
	}

	return time_us;
}

static uint32_t aux_time_min_get(const struct ll_adv_aux_set *aux)
{
	const struct lll_adv_aux *lll_aux;
	const struct pdu_adv *pdu_scan;
	const struct lll_adv *lll;
	const struct pdu_adv *pdu;
	uint8_t pdu_scan_len;
	uint8_t pdu_len;

	lll_aux = &aux->lll;
	lll = lll_aux->adv;
	pdu = lll_adv_aux_data_peek(lll_aux);
	pdu_scan = lll_adv_scan_rsp_peek(lll);

	/* Calculate the PDU Tx Time and hence the radio event length,
	 * Always use maximum length for common extended header format so that
	 * ACAD could be update when periodic advertising is active and the
	 * time reservation need not be updated everytime avoiding overlapping
	 * with other active states/roles.
	 */
	pdu_len = pdu->len - pdu->adv_ext_ind.ext_hdr_len -
		  PDU_AC_EXT_HEADER_SIZE_MIN + PDU_AC_EXT_HEADER_SIZE_MAX;
	pdu_scan_len = pdu_scan->len - pdu_scan->adv_ext_ind.ext_hdr_len -
		       PDU_AC_EXT_HEADER_SIZE_MIN + PDU_AC_EXT_HEADER_SIZE_MAX;

	return aux_time_get(aux, pdu, pdu_len, pdu_scan_len);
}

static uint8_t aux_time_update(struct ll_adv_aux_set *aux, struct pdu_adv *pdu,
			       struct pdu_adv *pdu_scan)
{
	uint32_t volatile ret_cb;
	uint32_t ticks_minus;
	uint32_t ticks_plus;
	uint32_t time_ticks;
	uint32_t time_us;
	uint32_t ret;

	time_us = aux_time_min_get(aux);
	time_ticks = HAL_TICKER_US_TO_TICKS(time_us);
	if (aux->ull.ticks_slot > time_ticks) {
		ticks_minus = aux->ull.ticks_slot - time_ticks;
		ticks_plus = 0U;
	} else if (aux->ull.ticks_slot < time_ticks) {
		ticks_minus = 0U;
		ticks_plus = time_ticks - aux->ull.ticks_slot;
	} else {
		return BT_HCI_ERR_SUCCESS;
	}

	ret_cb = TICKER_STATUS_BUSY;
	ret = ticker_update(TICKER_INSTANCE_ID_CTLR,
			    TICKER_USER_ID_THREAD,
			    (TICKER_ID_ADV_AUX_BASE +
			     ull_adv_aux_handle_get(aux)),
			    0, 0, ticks_plus, ticks_minus, 0, 0,
			    ull_ticker_status_give, (void *)&ret_cb);
	ret = ull_ticker_status_take(ret, &ret_cb);
	if (ret != TICKER_STATUS_SUCCESS) {
		return BT_HCI_ERR_CMD_DISALLOWED;
	}

	aux->ull.ticks_slot = time_ticks;

	return BT_HCI_ERR_SUCCESS;
}

void ull_adv_aux_lll_auxptr_fill(struct pdu_adv *pdu, struct lll_adv *adv)
{
	struct lll_adv_aux *lll_aux = adv->aux;
	struct pdu_adv_aux_ptr *aux_ptr;
	struct ll_adv_aux_set *aux;
	uint8_t data_chan_count;
	uint8_t *data_chan_map;
	uint16_t chan_counter;
	uint32_t offset_us;
	uint16_t pdu_us;

	aux = HDR_LLL2ULL(lll_aux);

	chan_counter = lll_aux->data_chan_counter;

	/* The offset has to be at least T_MAFS microseconds from the end of packet
	 * In addition, the offset recorded in the aux ptr has the same requirement and this
	 * offset is in steps of 30 microseconds; So use the quantized value in check
	 */
	pdu_us = PDU_AC_US(pdu->len, adv->phy_p, adv->phy_flags);
	offset_us = HAL_TICKER_TICKS_TO_US(lll_aux->ticks_pri_pdu_offset) +
		    lll_aux->us_pri_pdu_offset;
	if ((offset_us/OFFS_UNIT_30_US)*OFFS_UNIT_30_US < EVENT_MAFS_US + pdu_us) {
		struct ll_adv_aux_set *aux = HDR_LLL2ULL(lll_aux);
		uint32_t interval_us;

		/* Offset too small, point to next aux packet instead */
		interval_us = aux->interval * PERIODIC_INT_UNIT_US;
		offset_us = offset_us + interval_us;
		lll_aux->ticks_pri_pdu_offset = HAL_TICKER_US_TO_TICKS(offset_us);
		lll_aux->us_pri_pdu_offset = offset_us -
					     HAL_TICKER_TICKS_TO_US(lll_aux->ticks_pri_pdu_offset);
		chan_counter++;
	}

	/* Fill the aux offset */
	aux_ptr = ull_adv_aux_lll_offset_fill(pdu, lll_aux->ticks_pri_pdu_offset,
					      lll_aux->us_pri_pdu_offset, 0U);


	/* Calculate and fill the radio channel to use */
	data_chan_map = aux->chm[aux->chm_first].data_chan_map;
	data_chan_count = aux->chm[aux->chm_first].data_chan_count;
	aux_ptr->chan_idx = lll_chan_sel_2(chan_counter,
					   aux->data_chan_id,
					   data_chan_map, data_chan_count);
}

static void ticker_cb(uint32_t ticks_at_expire, uint32_t ticks_drift,
		      uint32_t remainder, uint16_t lazy, uint8_t force,
		      void *param)
{
	static memq_link_t link;
	static struct mayfly mfy = {0, 0, &link, NULL, lll_adv_aux_prepare};
	static struct lll_prepare_param p;
#if defined(CONFIG_BT_TICKER_EXT_EXPIRE_INFO)
	struct ticker_ext_context *context = param;
	struct ll_adv_aux_set *aux = context->context;
#else /* !CONFIG_BT_TICKER_EXT_EXPIRE_INFO */
	struct ll_adv_aux_set *aux = param;
#endif /* !CONFIG_BT_TICKER_EXT_EXPIRE_INFO */
	struct lll_adv_aux *lll;
	uint32_t ret;
	uint8_t ref;

	DEBUG_RADIO_PREPARE_A(1);

	lll = &aux->lll;

	/* Increment prepare reference count */
	ref = ull_ref_inc(&aux->ull);
	LL_ASSERT(ref);

	/* Append timing parameters */
	p.ticks_at_expire = ticks_at_expire;
	p.remainder = remainder;
	p.lazy = lazy;
	p.force = force;
	p.param = lll;
	mfy.param = &p;

#if defined(CONFIG_BT_CTLR_ADV_PERIODIC)
	struct ll_adv_set *adv;

	adv = HDR_LLL2ULL(lll->adv);
	if (adv->lll.sync) {
		struct lll_adv_sync *lll_sync = adv->lll.sync;
		struct ll_adv_sync_set *sync;

		sync = HDR_LLL2ULL(adv->lll.sync);
		if (sync->is_started) {
			uint32_t ticks_to_expire;
			uint32_t sync_remainder_us;

			LL_ASSERT(context->other_expire_info);

			/* Reduce a tick for negative remainder and return positive remainder
			 * value.
			 */
			ticks_to_expire = context->other_expire_info->ticks_to_expire;
			sync_remainder_us = context->other_expire_info->remainder;
			hal_ticker_remove_jitter(&ticks_to_expire, &sync_remainder_us);

			/* Add a tick for negative remainder and return positive remainder
			 * value.
			 */
			hal_ticker_add_jitter(&ticks_to_expire, &remainder);

			/* Store the offset in us */
			lll_sync->us_adv_sync_pdu_offset = HAL_TICKER_TICKS_TO_US(ticks_to_expire) +
						  sync_remainder_us - remainder;

			/* store the lazy value */
			lll_sync->sync_lazy = context->other_expire_info->lazy;
		}
	}
#endif /* CONFIG_BT_CTLR_ADV_PERIODIC */

	/* Process channel map update, if any */
	if (aux->chm_first != aux->chm_last) {
		/* Use channelMapNew */
		aux->chm_first = aux->chm_last;
	}

	/* Kick LLL prepare */
	ret = mayfly_enqueue(TICKER_USER_ID_ULL_HIGH,
			     TICKER_USER_ID_LLL, 0, &mfy);
	LL_ASSERT(!ret);

	DEBUG_RADIO_PREPARE_A(1);
}

#else /* !(CONFIG_BT_CTLR_ADV_AUX_SET > 0) */

static int init_reset(void)
{
	return 0;
}
#endif /* !(CONFIG_BT_CTLR_ADV_AUX_SET > 0) */
