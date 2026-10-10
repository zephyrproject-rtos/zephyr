/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>
#include <zephyr/kernel.h>
#include <soc.h>
#include <zephyr/bluetooth/hci_types.h>
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
#include "lll/lll_adv_types.h"
#include "lll_adv.h"
#include "lll/lll_adv_pdu.h"
#include "lll_adv_sync.h"
#include "lll_chan.h"

#include "ull_adv_types.h"

#include "ull_internal.h"
#include "ull_chan_internal.h"
#include "ull_adv_internal.h"

#include "ll.h"

#include "hal/debug.h"

#if defined(CONFIG_BT_CTLR_ADV_PERIODIC_RSP)

uint8_t ull_adv_sync_rsp_pdu_alloc(struct subevent_data_meta *se,
			       enum ull_adv_pdu_extra_data_flag extra_data_flag,
			       struct pdu_adv **ter_pdu_prev, struct pdu_adv **ter_pdu_new,
			       void **extra_data_prev, void **extra_data_new, uint8_t *ter_idx)
{
	struct pdu_adv *pdu_prev, *pdu_new;
	struct lll_adv_sync *lll_sync;
	void *ed_prev;
#if defined(CONFIG_BT_CTLR_ADV_EXT_PDU_EXTRA_DATA_MEMORY)
	void *ed_new;
#endif


	/* Get reference to previous periodic advertising PDU data */
	pdu_prev = lll_adv_sync_rsp_data_peek(se, &ed_prev);
	{
		pdu_new = lll_adv_sync_rsp_data_alloc(se, NULL, ter_idx);
		if (!pdu_new) {
			return BT_HCI_ERR_MEM_CAPACITY_EXCEEDED;
		}
	}

#if defined(CONFIG_BT_CTLR_ADV_EXT_PDU_EXTRA_DATA_MEMORY)
	if (extra_data_prev) {
		*extra_data_prev = ed_prev;
	}
	if (extra_data_new) {
		*extra_data_new = ed_new;
	}
#endif /* CONFIG_BT_CTLR_ADV_EXT_PDU_EXTRA_DATA_MEMORY */

	*ter_pdu_prev = pdu_prev;
	*ter_pdu_new = pdu_new;

	return 0;
}
static uint32_t ull_adv_sync_rsp_time_get(const struct ll_adv_sync_set *sync)
{
	const struct lll_adv_sync *lll_sync = &sync->lll;
	uint32_t time_us;

	LL_ASSERT_ERR(lll_sync->num_subevents > 0U);
	LL_ASSERT_ERR(lll_sync->num_response_slots > 0U);
	time_us = (lll_sync->num_subevents - 1U) *
		  (lll_sync->subevent_interval * 1250U) +
		  lll_sync->response_slot_delay * 1250U +
		  lll_sync->num_response_slots *
		  lll_sync->response_slot_spacing * 125U +
		  EVENT_OVERHEAD_START_US + EVENT_OVERHEAD_END_US;

	return time_us;
}
/**
 * @brief Configure Periodic Advertising with Responses (PAwR) parameters.
 *
 * Called from ll_adv_sync_param_set_v2() when num_subevents is non-zero.
 * Implements the Controller-side handling of HCI LE Set Periodic Advertising
 * Parameters v2 PAwR fields for the given extended advertising set.
 *
 * Stores PAwR train timing in ll_adv_sync_set and marks the LLL context as
 * PAwR-capable (lll_sync->is_rsp). Subevent payload is supplied later via
 * ll_adv_sync_subevent_data_set().
 *
 * Must not be called while periodic advertising is started
 * (sync->is_started must be false).
 *
 * @param handle               Extended advertising set handle
 * @param interval             Periodic advertising interval (units of 1.25 ms)
 * @param flags                Periodic advertising properties (currently unused)
 * @param num_subevents        Number of subevents in the PAwR train
 * @param subevent_interval    Interval between subevents (N * 1.25 ms)
 * @param response_slot_delay  Delay from subevent end to first response slot
 *                             (N * 1.25 ms)
 * @param response_slot_spacing Spacing between response slots (N * 0.125 ms)
 * @param num_response_slots   Total number of response slots per PA event
 *
 * @return BT_HCI_ERR_SUCCESS on success, or a Bluetooth HCI error code
 *         (e.g. BT_HCI_ERR_CMD_DISALLOWED if already started).
 */
uint8_t ull_adv_sync_rsp_param_set(uint8_t handle, uint16_t interval, uint16_t flags,
				  uint8_t num_subevents, uint8_t subevent_interval,
				  uint8_t response_slot_delay, uint8_t response_slot_spacing,
				  uint8_t num_response_slots)
{
	void *extra_data_prev, *extra_data;
	struct lll_adv_sync *lll_sync;
	struct ll_adv_sync_set *sync;
	struct ll_adv_set *adv;
	struct subevent_data_meta *se;
	struct pdu_adv *ter_pdu,*pdu_prev,*pdu;
	uint8_t err, ter_idx;
	uint32_t time_us;

	adv = ull_adv_is_created_get(handle);
	if (!adv) {
		return BT_HCI_ERR_UNKNOWN_ADV_IDENTIFIER;
	}

	lll_sync = adv->lll.sync;
	if (!lll_sync) {
		struct lll_adv *lll;
		uint8_t chm_last;

		sync = ull_adv_sync_acquire();
		if (!sync) {
			return BT_HCI_ERR_MEM_CAPACITY_EXCEEDED;
		}

		lll = &adv->lll;
		lll_sync = &sync->lll;
		lll->sync = lll_sync;
		lll_sync->adv = lll;

		lll_hdr_init(lll_sync, sync);

		// err = util_aa_le32(lll_sync->access_addr);
		// LL_ASSERT_DBG(!err);
		lll_sync->access_addr[0] = 0xED;
		lll_sync->access_addr[1] = 0xCE;
		lll_sync->access_addr[2] = 0x2D;
		lll_sync->access_addr[3] = 0x28;
		// err = util_aa_le32(lll_sync->rsp_aa);
		// LL_ASSERT_DBG(!err);
		lll_sync->rsp_aa[0] = 0x98;
		lll_sync->rsp_aa[1] = 0x17;
		lll_sync->rsp_aa[2] = 0x6F;
		lll_sync->rsp_aa[3] = 0x4F;
		lll_sync->data_chan_id = lll_chan_id(lll_sync->access_addr);

		chm_last = lll_sync->chm_first;
		lll_sync->chm_last = chm_last;
		lll_sync->chm[chm_last].data_chan_count =
			ull_chan_map_get(lll_sync->chm[chm_last].data_chan_map);

		lll_csrand_get(lll_sync->crc_init, sizeof(lll_sync->crc_init));
		lll_sync->latency_prepare = 0;
		lll_sync->latency_event = 0;
		lll_sync->event_counter = 0;

		sync->is_enabled = 0U;
		sync->is_started = 0U;

	} else {	//already exists
		sync = HDR_LLL2ULL(lll_sync);
		if (sync->is_started) {
			return BT_HCI_ERR_CMD_DISALLOWED;
		}
		for(uint8_t i = 0; i < lll_sync->num_subevents; i++) {
			se = &lll_sync->se_data[i];
			lll_adv_sync_rsp_data_release(se);
		}
	}

	if (sync->is_started) {
		return BT_HCI_ERR_CMD_DISALLOWED;
	}

	sync->interval = interval;
	lll_sync->num_subevents = num_subevents;
	lll_sync->subevent_interval = subevent_interval;
	lll_sync->response_slot_delay = response_slot_delay;
	lll_sync->response_slot_spacing = response_slot_spacing;
	lll_sync->num_response_slots = num_response_slots;

	for(uint8_t i = 0; i < num_subevents; i++) {
		se = &lll_sync->se_data[i];

		lll_adv_data_reset(&se->data);
		err = lll_adv_sync_rsp_data_init(se);
		if (err) {
			return BT_HCI_ERR_MEM_CAPACITY_EXCEEDED;
		}
		ter_pdu = lll_adv_sync_rsp_data_peek(se, NULL);
		//ter_pdu = (void *)(se->data.pdu[se->data.last]);
		ull_adv_sync_pdu_init(ter_pdu, 0U, 0U, 0U, NULL);
		err = ull_adv_sync_rsp_pdu_alloc(se, ULL_ADV_PDU_EXTRA_DATA_ALLOC_IF_EXIST, &pdu_prev, &pdu,
						&extra_data_prev, &extra_data, &ter_idx);
		if (err) {
			return err;
		}

		/* FIXME - handle flags (i.e. adding TxPower if specified) */
		err = ull_adv_sync_duplicate(pdu_prev, pdu);
		if (err) {
			return err;
		}
		lll_adv_sync_rsp_data_enqueue(se, ter_idx);

		se->response_slot_start = 0U;
		se->response_slot_count = 0U;
		se->is_data_set = 0U;
	}
	lll_sync->is_rsp = 1;
	lll_sync->subevent_curr = 0;
	lll_sync->rsp_slot_curr = 0;
	ARG_UNUSED(flags);

	sync->is_data_cmplt = 1U;

	time_us = ull_adv_sync_rsp_time_get(sync);
	sync->ull.ticks_slot = HAL_TICKER_US_TO_TICKS_CEIL(time_us);

	uint8_t data[] = {
		0x01, 0x02, 0x03, 0x04,
		0x05, 0x06, 0x07, 0x08
	};

	err = ll_adv_sync_subevent_data_set(handle,       /* adv handle */
					    0U,       /* subevent */
					    0U,       /* response slot start */
					    2U,       /* response slot count */
					    sizeof(data),
					    data);
	if (err) {
		return err;
	}
	uint8_t data1[] = {
		0x0a, 0x0b, 0x0c, 0x0d,
		0x0e, 0x0f, 0x10, 0x11
	};
	err = ll_adv_sync_subevent_data_set(handle,       /* adv handle */
					    1U,       /* subevent */
					    0U,       /* response slot start */
					    2U,       /* response slot count */
					    sizeof(data1),
					    data1);
	if (err) {
		return err;
	}
	return BT_HCI_ERR_SUCCESS;
}
static void ull_adv_sync_rsp_copy_pdu_header(struct pdu_adv *target_pdu,
					 const struct pdu_adv *source_pdu,
					 const struct pdu_adv_ext_hdr *skip_fields,
					 bool skip_acad)
{
	const struct pdu_adv_com_ext_adv *source_hdr = &source_pdu->adv_ext_ind;
	struct pdu_adv_com_ext_adv *target_hdr = &target_pdu->adv_ext_ind;
	const uint8_t *source_dptr;
	uint8_t *target_dptr;

	LL_ASSERT_DBG(target_pdu != source_pdu);

	/* Initialize PDU header */
	target_pdu->type = source_pdu->type;
	target_pdu->rfu = 0U;
	target_pdu->chan_sel = 0U;
	target_pdu->tx_addr = 0U;
	target_pdu->rx_addr = 0U;
	target_hdr->adv_mode = source_hdr->adv_mode;

	/* Copy extended header */
	if (source_hdr->ext_hdr_len == 0U) {
		/* No extended header present */
		target_hdr->ext_hdr_len = 0U;
	} else if (!skip_fields && !skip_acad) {
		/* Copy entire extended header */
		memcpy(target_hdr, source_hdr, source_hdr->ext_hdr_len + 1U);
	} else {
		/* Copy field by field */
		source_dptr = source_hdr->ext_hdr.data;
		target_dptr = target_hdr->ext_hdr.data;

		/* Initialize extended header flags to all 0 */
		target_hdr->ext_hdr_adv_data[0U] = 0U;

		/* AdvA and TargetA is RFU for periodic advertising */

#if defined(CONFIG_BT_CTLR_DF_ADV_CTE_TX)
		if (source_hdr->ext_hdr.cte_info) {
			if (!skip_fields || !skip_fields->cte_info) {
				memcpy(target_dptr, source_dptr, sizeof(struct pdu_cte_info));
				target_dptr += sizeof(struct pdu_cte_info);
				target_hdr->ext_hdr.cte_info = 1U;
			}
			source_dptr += sizeof(struct pdu_cte_info);
		}
#endif /* CONFIG_BT_CTLR_DF_ADV_CTE_TX */

#if defined(CONFIG_BT_CTLR_ADV_PERIODIC_ADI_SUPPORT)
		if (source_hdr->ext_hdr.adi) {
			if (!skip_fields || !skip_fields->adi) {
				memcpy(target_dptr, source_dptr, sizeof(struct pdu_adv_adi));
				target_dptr += sizeof(struct pdu_adv_adi);
				target_hdr->ext_hdr.adi = 1U;
			}
			source_dptr += sizeof(struct pdu_adv_adi);
		}
#endif /* CONFIG_BT_CTLR_ADV_PERIODIC_ADI_SUPPORT */

#if defined(CONFIG_BT_CTLR_ADV_SYNC_PDU_LINK)
		if (source_hdr->ext_hdr.aux_ptr) {
			if (!skip_fields || !skip_fields->aux_ptr) {
				memcpy(target_dptr, source_dptr, sizeof(struct pdu_adv_aux_ptr));
				target_dptr += sizeof(struct pdu_adv_aux_ptr);
				target_hdr->ext_hdr.aux_ptr = 1U;
			}
			source_dptr += sizeof(struct pdu_adv_aux_ptr);
		}
#endif /* CONFIG_BT_CTLR_ADV_SYNC_PDU_LINK */

		/* SyncInfo is RFU for periodic advertising */

		if (source_hdr->ext_hdr.tx_pwr) {
			if (!skip_fields || !skip_fields->tx_pwr) {
				*target_dptr = *source_dptr;
				target_dptr++;
				target_hdr->ext_hdr.tx_pwr = 1U;
			}
			source_dptr++;
		}

		/* ACAD is the remainder of the header, if any left */
		if ((source_dptr - source_hdr->ext_hdr_adv_data) < source_hdr->ext_hdr_len &&
		    !skip_acad) {
			uint8_t acad_len = source_hdr->ext_hdr_len -
					   (source_dptr - source_hdr->ext_hdr_adv_data);

			memcpy(target_dptr, source_dptr, acad_len);
			target_dptr += acad_len;
		}

		if (target_dptr == target_hdr->ext_hdr.data) {
			/* Nothing copied, do not include extended header */
			target_hdr->ext_hdr_len = 0U;
		} else {
			target_hdr->ext_hdr_len = target_dptr - target_hdr->ext_hdr_adv_data;
		}
	}

	target_pdu->len = target_hdr->ext_hdr_len + 1U;
}


/**
 * @brief Set subevent data for periodic advertising with responses, function
 *        handles one subevent, caller can call multiple times for each subevent.
 *
 * @param handle         Advertising set handle
 * @param subevent       Subevent index
 * @param response_slot_start First response slot for this subevent
 * @param response_slot_count Number of response slots for this subevent
 * @param subevent_data_len   Length of subevent data
 * @param subevent_data       Pointer to subevent data
 *
 * @return 0 on success, error code otherwise
 */
uint8_t ll_adv_sync_subevent_data_set(uint8_t handle,
				      uint8_t subevent,
				      uint8_t response_slot_start,
				      uint8_t response_slot_count,
				      uint8_t subevent_data_len,
				      uint8_t *subevent_data)
{
	void *extra_data_prev, *extra_data;
	struct ll_adv_sync_set *sync;
	struct ll_adv_set *adv;
	struct lll_adv_sync *lll_sync;
	struct subevent_data_meta *se;
	struct pdu_adv *pdu_prev, *pdu;
	uint8_t ter_idx;
	uint8_t err;
	struct pdu_adv_ext_hdr skip_fields = { 0U };
	struct pdu_adv_com_ext_adv *hdr;
	struct pdu_adv_adi *adi;
	uint8_t *dptr;

	/* Get the advertising set */
	adv = ull_adv_is_created_get(handle);
	if (!adv) {
		return BT_HCI_ERR_UNKNOWN_ADV_IDENTIFIER;
	}
	/* Get sync context */
	sync = HDR_LLL2ULL(adv->lll.sync);
	if (!sync) {
		return BT_HCI_ERR_CMD_DISALLOWED;
	}
	lll_sync = &sync->lll;

	/* Validate subevent index */
	if (subevent >= lll_sync->num_subevents) {
		return BT_HCI_ERR_INVALID_PARAM;
	}
	/* Reject len > 191 bytes if chain PDUs unsupported */
	if (subevent_data_len > PDU_AC_EXT_AD_DATA_LEN_MAX) {
		return BT_HCI_ERR_CMD_DISALLOWED;
	}
	/* Validate response slot parameters */
	if (response_slot_count > 0) {
		uint8_t slot_end = response_slot_start + response_slot_count;

		if (slot_end > lll_sync->num_response_slots) {
			return BT_HCI_ERR_INVALID_PARAM;
		}
	}
	/* Store subevent data */
	if (!lll_sync->is_rsp || (lll_sync->num_subevents == 0U)) {
		return BT_HCI_ERR_CMD_DISALLOWED;
	}
	se = &lll_sync->se_data[subevent];

	/* Allocate new PDU buffer at latest double buffer index */
	err = ull_adv_sync_rsp_pdu_alloc(se, ULL_ADV_PDU_EXTRA_DATA_ALLOC_IF_EXIST,
				     &pdu_prev, &pdu, &extra_data_prev,
				     &extra_data, &ter_idx);
	if (err) {
		return err;
	}

	skip_fields.aux_ptr = 1U;
	/* FIXME - below ignores any configured CTE count */
	if (pdu_prev == pdu) {
		/* Remove adv data and any AuxPtr */
		pdu->len = pdu_prev->adv_ext_ind.ext_hdr_len + 1U;
	} else {
		/* Copy header (only), removing any prior presence of Aux Ptr */
		ull_adv_sync_rsp_copy_pdu_header(pdu, pdu_prev, &skip_fields, false);
	}

#if defined(CONFIG_BT_CTLR_ADV_PERIODIC_ADI_SUPPORT)
	/* New adv data - update ADI if present */
	if (pdu->adv_ext_ind.ext_hdr_len && pdu->adv_ext_ind.ext_hdr.adi) {
		struct ll_adv_set *adv = HDR_LLL2ULL(lll_sync->adv);
		uint16_t did;
		hdr = &pdu->adv_ext_ind;
		/* The DID for a specific SID shall be unique */
		did = sys_cpu_to_le16(ull_adv_aux_did_next_unique_get(adv->sid));

		/* Find ADI in extended header */
		dptr = hdr->ext_hdr.data;

		/* AdvA and TargetA is RFU for periodic advertising */

	#if defined(CONFIG_BT_CTLR_DF_ADV_CTE_TX)
		if (hdr->ext_hdr.cte_info) {
			dptr += sizeof(struct pdu_cte_info);
		}
	#endif /* CONFIG_BT_CTLR_DF_ADV_CTE_TX */
		adi = (struct pdu_adv_adi *)dptr;

		PDU_ADV_ADI_DID_SID_SET(adi, did, adv->sid);
	}
#endif /* CONFIG_BT_CTLR_ADV_PERIODIC_ADI_SUPPORT */

	dptr = pdu->payload + pdu->len;
	memcpy(dptr, subevent_data, subevent_data_len);
	pdu->len += subevent_data_len;

	se->response_slot_start = response_slot_start;
	se->response_slot_count = response_slot_count;
	se->is_data_set = 1U;
	/* commit  */
	lll_adv_pdu_enqueue(&se->data, ter_idx);

	return BT_HCI_ERR_SUCCESS;
}

#endif /* CONFIG_BT_CTLR_ADV_PERIODIC_RSP */
