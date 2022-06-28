/*
 * Copyright (c) 2017-2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/zephyr.h>
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

#define BT_DBG_ENABLED IS_ENABLED(CONFIG_BT_DEBUG_HCI_DRIVER)
#define LOG_MODULE_NAME bt_ctlr_ull_adv_aux
#include "common/log.h"
#include "hal/debug.h"

static int init_reset(void);

#if (CONFIG_BT_CTLR_ADV_AUX_SET > 0)
static inline struct ll_adv_aux_set *aux_acquire(void);
static inline void aux_release(struct ll_adv_aux_set *aux);
static uint8_t set_clear_ad_data_get(const uint8_t *value,
				     uint8_t **const ad_data);
static uint32_t aux_time_get(struct ll_adv_aux_set *aux, struct pdu_adv *pdu,
			     struct pdu_adv *pdu_scan);
static uint8_t aux_time_update(struct ll_adv_aux_set *aux, struct pdu_adv *pdu,
			       struct pdu_adv *pdu_scan);
static void mfy_aux_offset_get(void *param);
static void ticker_cb(uint32_t ticks_at_expire, uint32_t ticks_drift,
		      uint32_t remainder, uint16_t lazy, uint8_t force,
		      void *param);
static void ticker_op_cb(uint32_t status, void *param);

static struct ll_adv_aux_set ll_adv_aux_pool[CONFIG_BT_CTLR_ADV_AUX_SET];
static void *adv_aux_free;
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
	struct ll_adv_set *adv;
	uint8_t value[1 + sizeof(data)];
	uint8_t *val_ptr;
	uint8_t pri_idx;
	uint8_t err;

	/* op param definitions:
	 * 0x00 - Intermediate fragment of fragmented extended advertising data
	 * 0x01 - First fragment of fragmented extended advertising data
	 * 0x02 - Last fragment of fragmented extended advertising data
	 * 0x03 - Complete extended advertising data
	 * 0x04 - Unchanged data (just update the advertising data)
	 * All other values, Reserved for future use
	 */

	/* TODO: handle other op values */
	if ((op != BT_HCI_LE_EXT_ADV_OP_COMPLETE_DATA) &&
	    (op != BT_HCI_LE_EXT_ADV_OP_UNCHANGED_DATA)) {
		/* FIXME: error code */
		return BT_HCI_ERR_CMD_DISALLOWED;
	}

	/* Get the advertising set instance */
	adv = ull_adv_is_created_get(handle);
	if (!adv) {
		return BT_HCI_ERR_UNKNOWN_ADV_IDENTIFIER;
	}

	val_ptr = value;
	*val_ptr++ = len;
	(void)memcpy(val_ptr, &data, sizeof(data));
	err = ull_adv_aux_hdr_set_clear(adv, ULL_ADV_PDU_HDR_FIELD_AD_DATA,
					0, value, NULL, &pri_idx);
	if (err) {
		return err;
	}

	if (!adv->lll.aux) {
		return 0;
	}

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
				ceiling_fraction(((uint64_t)adv->interval *
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
			ticks_anchor = ticker_ticks_now_get();

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

	lll_adv_data_enqueue(&adv->lll, pri_idx);

	return 0;
}

uint8_t ll_adv_aux_sr_data_set(uint8_t handle, uint8_t op, uint8_t frag_pref,
			       uint8_t len, uint8_t const *const data)
{
	struct pdu_adv_com_ext_adv *sr_com_hdr;
	struct pdu_adv *pri_pdu_prev;
	struct pdu_adv_ext_hdr *sr_hdr;
	struct pdu_adv_adi *sr_adi;
	struct pdu_adv *sr_prev;
	struct pdu_adv *aux_pdu;
	struct ll_adv_set *adv;
	struct pdu_adv *sr_pdu;
	struct lll_adv *lll;
	uint8_t ext_hdr_len;
	uint8_t *sr_dptr;
	uint8_t pri_idx;
	uint8_t idx;
	uint8_t err;

	/* TODO: handle other op values */
	if ((op != BT_HCI_LE_EXT_ADV_OP_COMPLETE_DATA) &&
	    (op != BT_HCI_LE_EXT_ADV_OP_UNCHANGED_DATA)) {
		return BT_HCI_ERR_UNSUPP_FEATURE_PARAM_VAL;
	}

	/* Get the advertising set instance */
	adv = ull_adv_is_created_get(handle);
	if (!adv) {
		return BT_HCI_ERR_UNKNOWN_ADV_IDENTIFIER;
	}

	lll = &adv->lll;

	/* Do not use Common Extended Advertising Header Format if not extended
	 * advertising.
	 */
	pri_pdu_prev = lll_adv_data_peek(lll);
	if (pri_pdu_prev->type != PDU_ADV_TYPE_EXT_IND) {
		if ((op != BT_HCI_LE_EXT_ADV_OP_COMPLETE_DATA) ||
		    (len > PDU_AC_DATA_SIZE_MAX)) {
			return BT_HCI_ERR_INVALID_PARAM;
		}
		return ull_scan_rsp_set(adv, len, data);
	}

	LL_ASSERT(lll->aux);

	aux_pdu = lll_adv_aux_data_peek(lll->aux);

	/* Can only discard data on non-scannable instances */
	if (!(aux_pdu->adv_ext_ind.adv_mode & BT_HCI_LE_ADV_PROP_SCAN) && len) {
		return BT_HCI_ERR_INVALID_PARAM;
	}

	/* Data can be discarded only using 0x03 op */
	if ((op != BT_HCI_LE_EXT_ADV_OP_COMPLETE_DATA) && !len) {
		return BT_HCI_ERR_INVALID_PARAM;
	}

	/* Can only set complete data if advertising is enabled */
	if (adv->is_enabled && (op != BT_HCI_LE_EXT_ADV_OP_COMPLETE_DATA)) {
		return BT_HCI_ERR_CMD_DISALLOWED;
	}

	/* Cannot discard scan response if scannable advertising is enabled */
	if (adv->is_enabled &&
	    (aux_pdu->adv_ext_ind.adv_mode & BT_HCI_LE_ADV_PROP_SCAN) && !len) {
		return BT_HCI_ERR_CMD_DISALLOWED;
	}

	/* Update scan response PDU fields. */
	sr_pdu = lll_adv_scan_rsp_alloc(lll, &idx);
	sr_pdu->type = PDU_ADV_TYPE_AUX_SCAN_RSP;
	sr_pdu->rfu = 0;
	sr_pdu->chan_sel = 0;
	sr_pdu->tx_addr = aux_pdu->tx_addr;
	sr_pdu->rx_addr = 0;
	sr_pdu->len = 0;

	/* If no length is provided, discard data */
	if (!len) {
		/* No scan response data, primary channel PDU's ADI update
		 * should not copy into scan response ADI.
		 */
		sr_adi = NULL;

		goto sr_data_set_did_update;
	}

	sr_com_hdr = &sr_pdu->adv_ext_ind;
	sr_hdr = (void *)&sr_com_hdr->ext_hdr_adv_data[0];
	sr_dptr = (void *)sr_hdr;

	/* Flags */
	*sr_dptr = 0;
	sr_hdr->adv_addr = 1;
#if defined(CONFIG_BT_CTRL_ADV_ADI_IN_SCAN_RSP)
	sr_hdr->adi = 1;
#endif
	sr_dptr++;

	sr_prev = lll_adv_scan_rsp_peek(lll);

	/* AdvA */
	(void)memcpy(sr_dptr, &sr_prev->adv_ext_ind.ext_hdr.data[ADVA_OFFSET],
		     BDADDR_SIZE);
	sr_dptr += BDADDR_SIZE;

#if defined(CONFIG_BT_CTRL_ADV_ADI_IN_SCAN_RSP)
	/* ADI */
	sr_adi = (void *)sr_dptr;
	sr_dptr += sizeof(struct pdu_adv_adi);
#else
	sr_adi = NULL;
#endif

	/* Check Max Advertising Data Length */
	if (len > CONFIG_BT_CTLR_ADV_DATA_LEN_MAX) {
		return BT_HCI_ERR_MEM_CAPACITY_EXCEEDED;
	}

	/* Check if data will fit in remaining space */
	/* TODO: need aux_chain_ind support */
	ext_hdr_len = sr_dptr - &sr_com_hdr->ext_hdr_adv_data[0];
	if ((PDU_AC_EXT_HEADER_SIZE_MIN + ext_hdr_len + len) >
	    PDU_AC_PAYLOAD_SIZE_MAX) {
		/* Will use packet too long error to determine fragmenting
		 * long data
		 */
		return BT_HCI_ERR_PACKET_TOO_LONG;
	}

	/* Copy data */
	(void)memcpy(sr_dptr, data, len);
	sr_dptr += len;

	/* Finish Common ExtAdv Payload header */
	sr_com_hdr->adv_mode = 0;
	sr_com_hdr->ext_hdr_len = ext_hdr_len;

	/* Finish PDU */
	sr_pdu->len = sr_dptr - &sr_pdu->payload[0];

sr_data_set_did_update:
	/* Trigger DID update */
	err = ull_adv_aux_hdr_set_clear(adv, 0, 0, NULL, sr_adi, &pri_idx);
	if (err) {
		return err;
	}

	/* NOTE: No update to primary channel PDU time reservation  */

	lll_adv_data_enqueue(&adv->lll, pri_idx);
	lll_adv_scan_rsp_enqueue(&adv->lll, idx);

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
	}

	/* TODO: Should failure due to Channel Map Update being already in
	 *       progress be returned to caller?
	 */
	return 0;
}

uint8_t ull_adv_aux_hdr_set_clear(struct ll_adv_set *adv,
				  uint16_t sec_hdr_add_fields,
				  uint16_t sec_hdr_rem_fields,
				  void *value,
				  struct pdu_adv_adi *adi,
				  uint8_t *pri_idx)
{
	struct pdu_adv_com_ext_adv *pri_com_hdr, *pri_com_hdr_prev;
	struct pdu_adv_com_ext_adv *sec_com_hdr, *sec_com_hdr_prev;
	struct pdu_adv_ext_hdr *pri_hdr, pri_hdr_prev;
	struct pdu_adv_ext_hdr *sec_hdr, sec_hdr_prev;
	struct pdu_adv *pri_pdu, *pri_pdu_prev;
	struct pdu_adv *sec_pdu_prev, *sec_pdu;
	struct pdu_adv_adi *pri_adi, *sec_adi;
	uint8_t *pri_dptr, *pri_dptr_prev;
	uint8_t *sec_dptr, *sec_dptr_prev;
	struct pdu_adv_aux_ptr *aux_ptr;
	uint8_t pri_len, sec_len_prev;
	struct lll_adv_aux *lll_aux;
	struct ll_adv_aux_set *aux;
	struct lll_adv *lll;
	uint8_t is_aux_new;
	uint8_t *ad_data;
	uint16_t sec_len;
	uint8_t sec_idx;
	uint8_t ad_len;
	uint16_t did;

	lll = &adv->lll;

	/* Can't have both flags set here since both use 'value' extra param */
	LL_ASSERT(!(sec_hdr_add_fields & ULL_ADV_PDU_HDR_FIELD_ADVA) ||
		  !(sec_hdr_add_fields & ULL_ADV_PDU_HDR_FIELD_AD_DATA));

	/* Get reference to previous primary PDU data */
	pri_pdu_prev = lll_adv_data_peek(lll);
	if (pri_pdu_prev->type != PDU_ADV_TYPE_EXT_IND) {
		if (sec_hdr_add_fields & ULL_ADV_PDU_HDR_FIELD_AD_DATA) {
			ad_len = set_clear_ad_data_get(value, &ad_data);

			return ull_adv_data_set(adv, ad_len, ad_data);
		}

		return BT_HCI_ERR_CMD_DISALLOWED;
	}

	pri_com_hdr_prev = (void *)&pri_pdu_prev->adv_ext_ind;
	pri_hdr = (void *)pri_com_hdr_prev->ext_hdr_adv_data;
	if (pri_com_hdr_prev->ext_hdr_len) {
		pri_hdr_prev = *pri_hdr;
	} else {
		*(uint8_t *)&pri_hdr_prev = 0U;
	}
	pri_dptr_prev = pri_hdr->data;

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
	pri_hdr = (void *)pri_com_hdr->ext_hdr_adv_data;
	pri_dptr = pri_hdr->data;
	*(uint8_t *)pri_hdr = 0U;

	/* Get the reference to aux instance */
	lll_aux = lll->aux;
	if (!lll_aux) {
		aux = ull_adv_aux_acquire(lll);
		if (!aux) {
			return BT_HCI_ERR_MEM_CAPACITY_EXCEEDED;
		}

		lll_aux = &aux->lll;

		is_aux_new = 1U;
	} else {
		aux = HDR_LLL2ULL(lll_aux);
		is_aux_new = 0U;
	}

	/* Get reference to previous secondary PDU data */
	sec_pdu_prev = lll_adv_aux_data_peek(lll_aux);
	sec_com_hdr_prev = (void *)&sec_pdu_prev->adv_ext_ind;
	sec_hdr = (void *)sec_com_hdr_prev->ext_hdr_adv_data;
	if (!is_aux_new) {
		sec_hdr_prev = *sec_hdr;
	} else {
		/* Initialize only those fields used to copy into new PDU
		 * buffer.
		 */
		sec_pdu_prev->tx_addr = 0U;
		sec_pdu_prev->rx_addr = 0U;
		sec_pdu_prev->len = PDU_AC_EXT_HEADER_SIZE_MIN;
		*(uint8_t *)&sec_hdr_prev = 0U;
	}
	sec_dptr_prev = sec_hdr->data;

	/* Get reference to new secondary PDU data buffer */
	sec_pdu = lll_adv_aux_data_alloc(lll_aux, &sec_idx);
	sec_pdu->type = pri_pdu->type;
	sec_pdu->rfu = 0U;
	sec_pdu->chan_sel = 0U;

	sec_pdu->tx_addr = sec_pdu_prev->tx_addr;
	sec_pdu->rx_addr = sec_pdu_prev->rx_addr;

	sec_com_hdr = (void *)&sec_pdu->adv_ext_ind;
	sec_com_hdr->adv_mode = pri_com_hdr->adv_mode;
	sec_hdr = (void *)sec_com_hdr->ext_hdr_adv_data;
	sec_dptr = sec_hdr->data;
	*(uint8_t *)sec_hdr = 0U;

	/* AdvA flag */
	/* NOTE: as we will use auxiliary packet, we remove AdvA in primary
	 * channel, i.e. do nothing to not add AdvA in the primary PDU.
	 * AdvA can be either set explicitly (i.e. needs own_addr_type to be
	 * set), can be copied from primary PDU (i.e. adding AD to existing set)
	 * or can be copied from previous secondary PDU.
	 */
	sec_hdr->adv_addr = 1;
	if (sec_hdr_add_fields & ULL_ADV_PDU_HDR_FIELD_ADVA) {
		uint8_t own_addr_type = *(uint8_t *)value;

		/* Move to next value */
		value = (uint8_t *)value + sizeof(own_addr_type);

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
		sec_hdr->tgt_addr = 1U;
		sec_pdu->rx_addr = pri_pdu_prev->rx_addr;
		sec_dptr += BDADDR_SIZE;

	/* Retain the target address if present in the previous PDU */
	} else if (!(sec_hdr_add_fields & ULL_ADV_PDU_HDR_FIELD_ADVA) &&
		   sec_hdr_prev.tgt_addr) {
		sec_hdr->tgt_addr = 1U;
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
	pri_hdr->adi = 1;
	pri_dptr += sizeof(struct pdu_adv_adi);
	if (sec_hdr_prev.adi) {
		sec_dptr_prev += sizeof(struct pdu_adv_adi);
	}
	sec_hdr->adi = 1;
	sec_dptr += sizeof(struct pdu_adv_adi);

	/* AuxPtr flag */
	if (pri_hdr_prev.aux_ptr) {
		pri_dptr_prev += sizeof(struct pdu_adv_aux_ptr);
	}
	pri_hdr->aux_ptr = 1;
	pri_dptr += sizeof(struct pdu_adv_aux_ptr);

	if (sec_hdr_add_fields & ULL_ADV_PDU_HDR_FIELD_AUX_PTR) {
		sec_hdr->aux_ptr = 1;
		aux_ptr = NULL;

		/* return the size of aux pointer structure */
		*(uint8_t *)value = sizeof(struct pdu_adv_aux_ptr);
		value = (uint8_t *)value + sizeof(uint8_t);

		/* return the pointer to aux pointer struct inside the PDU
		 * buffer
		 */
		(void)memcpy(value, &sec_dptr, sizeof(sec_dptr));
		value = (uint8_t *)value + sizeof(sec_dptr);
	} else if (!(sec_hdr_rem_fields & ULL_ADV_PDU_HDR_FIELD_AUX_PTR) &&
		   sec_hdr_prev.aux_ptr) {
		sec_hdr->aux_ptr = 1;
		aux_ptr = (void *)sec_dptr_prev;
	} else {
		aux_ptr = NULL;
	}
	if (sec_hdr_prev.aux_ptr) {
		sec_dptr_prev += sizeof(struct pdu_adv_aux_ptr);
	}
	if (sec_hdr->aux_ptr) {
		sec_dptr += sizeof(struct pdu_adv_aux_ptr);
	}

#if defined(CONFIG_BT_CTLR_ADV_PERIODIC)
	struct pdu_adv_sync_info *sync_info;

	/* No SyncInfo flag in primary channel PDU */
	/* Add/Remove SyncInfo flag in secondary channel PDU */
	if (sec_hdr_add_fields & ULL_ADV_PDU_HDR_FIELD_SYNC_INFO) {
		sec_hdr->sync_info = 1;
		sync_info = NULL;

		/* return the size of sync info structure */
		*(uint8_t *)value = sizeof(*sync_info);
		value = (uint8_t *)value + sizeof(uint8_t);

		/* return the pointer to sync info struct inside the PDU
		 * buffer
		 */
		(void)memcpy(value, &sec_dptr, sizeof(sec_dptr));
		value = (uint8_t *)value + sizeof(sec_dptr);
	} else if (!(sec_hdr_rem_fields & ULL_ADV_PDU_HDR_FIELD_SYNC_INFO) &&
		   sec_hdr_prev.sync_info) {
		sec_hdr->sync_info = 1;
		sync_info = (void *)sec_dptr_prev;
	} else {
		sync_info = NULL;
	}
	if (sec_hdr_prev.sync_info) {
		sec_dptr_prev += sizeof(*sync_info);
	}
	if (sec_hdr->sync_info) {
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
			pri_hdr->tx_pwr = 1;
			pri_dptr++;
		} else {
			sec_hdr->tx_pwr = 1;
		}
	}
	if (sec_hdr_prev.tx_pwr) {
		sec_dptr_prev++;

		sec_hdr->tx_pwr = 1;
	}
	if (sec_hdr->tx_pwr) {
		sec_dptr++;
	}

	/* No ACAD in primary channel PDU */
	/* TODO: ACAD in secondary channel PDU */

	/* Calc primary PDU len */
	pri_len = ull_adv_aux_hdr_len_calc(pri_com_hdr, &pri_dptr);
	ull_adv_aux_hdr_len_fill(pri_com_hdr, pri_len);

	/* set the primary PDU len */
	pri_pdu->len = pri_len;

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
	ull_adv_aux_hdr_len_fill(sec_com_hdr, sec_len);

	/* AD Data, add or remove */
	if (sec_hdr_add_fields & ULL_ADV_PDU_HDR_FIELD_AD_DATA) {
		ad_len = set_clear_ad_data_get(value, &ad_data);
	} else {
		/* Calc the previous AD data length in auxiliary PDU */
		ad_len = sec_pdu_prev->len - sec_len_prev;
		ad_data = sec_dptr_prev;
	}

	/* Check Max Advertising Data Length */
	if (ad_len > CONFIG_BT_CTLR_ADV_DATA_LEN_MAX) {
		return BT_HCI_ERR_MEM_CAPACITY_EXCEEDED;
	}

	/* Check AdvData overflow */
	/* TODO: need aux_chain_ind support */
	if ((sec_len + ad_len) > PDU_AC_PAYLOAD_SIZE_MAX) {
		/* Will use packet too long error to determine fragmenting
		 * long data
		 */
		return BT_HCI_ERR_PACKET_TOO_LONG;
	}

	/* set the secondary PDU len */
	sec_pdu->len = sec_len + ad_len;

	/* Start filling pri and sec PDU payload based on flags from here
	 * ==============================================================
	 */

	/* No AdvData in primary channel PDU */
	/* Fill AdvData in secondary PDU */
	(void)memmove(sec_dptr, ad_data, ad_len);

	/* Early exit if no flags set */
	if (!sec_com_hdr->ext_hdr_len) {
		return 0;
	}

	/* No ACAD in primary channel PDU */
	/* TODO: Fill ACAD in secondary channel PDU */

	/* Tx Power */
	if (pri_hdr->tx_pwr) {
		*--pri_dptr = *--pri_dptr_prev;
	} else if (sec_hdr->tx_pwr) {
		*--sec_dptr = *--sec_dptr_prev;
	}

#if defined(CONFIG_BT_CTLR_ADV_PERIODIC)
	/* No SyncInfo in primary channel PDU */
	/* Fill SyncInfo in secondary channel PDU */
	if (sec_hdr_prev.sync_info) {
		sec_dptr_prev -= sizeof(*sync_info);
	}

	if (sec_hdr->sync_info) {
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
	if (sec_hdr->aux_ptr) {
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

	/* The DID for a specific SID shall be unique.
	 */
	did = ull_adv_aux_did_next_unique_get(adv->sid);

	pri_adi->did = sys_cpu_to_le16(did);
	sec_adi->did = sys_cpu_to_le16(did);

	if (adi) {
		*adi = *pri_adi;
	}

	/* No CTEInfo field in primary channel PDU */

	/* No TargetA non-conn non-scan advertising, but present in directed
	 * advertising.
	 */
	if (sec_hdr->tgt_addr) {
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
	if (sec_hdr->adv_addr) {
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

	lll_adv_aux_data_enqueue(lll_aux, sec_idx);

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

	/* NOTE: Channel Index and Aux Offset will be set on every advertiser's
	 * event prepare when finding the auxiliary event's ticker offset.
	 * Here we fill initial values.
	 */
	aux_ptr->chan_idx = 0U;

	aux_ptr->ca = (lll_clock_ppm_local_get() <= SCA_50_PPM) ?
		      SCA_VALUE_50_PPM : SCA_VALUE_500_PPM;

	offs = offs_us / OFFS_UNIT_30_US;
	if (!!(offs >> OFFS_UNIT_BITS)) {
		aux_ptr->offs = offs / (OFFS_UNIT_300_US / OFFS_UNIT_30_US);
		aux_ptr->offs_units = OFFS_UNIT_VALUE_300_US;
	} else {
		aux_ptr->offs = offs;
		aux_ptr->offs_units = OFFS_UNIT_VALUE_30_US;
	}

	aux_ptr->phy = find_lsb_set(phy_s) - 1;
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
	struct lll_adv_aux *lll_aux;
	struct pdu_adv *pdu_scan;
	struct pdu_adv *pdu;
	struct lll_adv *lll;
	uint32_t time_us;

	lll_aux = &aux->lll;
	lll = lll_aux->adv;
	pdu = lll_adv_aux_data_peek(lll_aux);
	pdu_scan = lll_adv_scan_rsp_peek(lll);

	/* Calculate the PDU Tx Time and hence the radio event length */
	time_us = aux_time_get(aux, pdu, pdu_scan);

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
			EVENT_TICKER_RES_MARGIN_US);
	}
#endif /* CONFIG_BT_CTLR_SCHED_ADVANCED */

	return ticks_slot_overhead;
}

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

	ret_cb = TICKER_STATUS_BUSY;
	ret = ticker_start(TICKER_INSTANCE_ID_CTLR, TICKER_USER_ID_THREAD,
			   (TICKER_ID_ADV_AUX_BASE + aux_handle),
			   ticks_anchor, 0U,
			   HAL_TICKER_US_TO_TICKS(interval_us),
			   TICKER_NULL_REMAINDER, TICKER_NULL_LAZY,
			   (aux->ull.ticks_slot + ticks_slot_overhead),
			   ticker_cb, aux,
			   ull_ticker_status_give, (void *)&ret_cb);
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
	err = lll_adv_data_init(&lll_aux->data);
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
	const struct lll_adv_aux *lll_aux;
	const struct lll_adv *lll;
	const struct pdu_adv *pdu;
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

	pdu = lll_adv_aux_data_peek(lll_aux);
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
	}

	return time_us;
}

void ull_adv_aux_offset_get(struct ll_adv_set *adv)
{
	static memq_link_t link;
	static struct mayfly mfy = {0, 0, &link, NULL, mfy_aux_offset_get};
	uint32_t ret;

	/* NOTE: Single mayfly instance is sufficient as primary channel PDUs
	 *       use time reservation, and this mayfly shall complete within
	 *       the radio event. Multiple advertising sets do not need
	 *       independent mayfly allocations.
	 */
	mfy.param = adv;
	ret = mayfly_enqueue(TICKER_USER_ID_ULL_HIGH, TICKER_USER_ID_ULL_LOW, 1,
			     &mfy);
	LL_ASSERT(!ret);
}

struct pdu_adv_aux_ptr *ull_adv_aux_lll_offset_fill(struct pdu_adv *pdu,
						    uint32_t ticks_offset,
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
	offs = HAL_TICKER_TICKS_TO_US(ticks_offset) - start_us;
	offs = offs / OFFS_UNIT_30_US;
	if (!!(offs >> OFFS_UNIT_BITS)) {
		aux_ptr->offs = offs / (OFFS_UNIT_300_US / OFFS_UNIT_30_US);
		aux_ptr->offs_units = OFFS_UNIT_VALUE_300_US;
	} else {
		aux_ptr->offs = offs;
		aux_ptr->offs_units = OFFS_UNIT_VALUE_30_US;
	}

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

static uint8_t set_clear_ad_data_get(const uint8_t *value,
				     uint8_t **const ad_data)
{
	uint8_t ad_len;

	/* pick the data length */
	ad_len = *((uint8_t *)value);
	value = (uint8_t *)value + sizeof(ad_len);

	/* pick the reference to data */
	(void)memcpy(ad_data, value, sizeof(*ad_data));

	return ad_len;
}

static uint32_t aux_time_get(struct ll_adv_aux_set *aux, struct pdu_adv *pdu,
			     struct pdu_adv *pdu_scan)
{
	struct lll_adv_aux *lll_aux;
	struct lll_adv *lll;
	uint32_t time_us;

	/* NOTE: 16-bit values are sufficient for minimum radio event time
	 *       reservation, 32-bit are used here so that reservations for
	 *       whole back-to-back chaining of PDUs can be accomodated where
	 *       the required microseconds could overflow 16-bits, example,
	 *       back-to-back chained Coded PHY PDUs.
	 */
	lll_aux = &aux->lll;
	lll = lll_aux->adv;
	time_us = PDU_AC_US(pdu->len, lll->phy_s, lll->phy_flags) +
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
			PDU_AC_US(pdu_scan->len, lll->phy_s, lll->phy_flags);

		time_us += EVENT_IFS_MAX_US * 2 + scan_req_us + scan_rsp_us;
	}

	return time_us;
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

	time_us = aux_time_get(aux, pdu, pdu_scan);
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

static void mfy_aux_offset_get(void *param)
{
	struct pdu_adv_aux_ptr *aux_ptr;
	struct lll_adv_aux *lll_aux;
	struct ll_adv_aux_set *aux;
	uint32_t ticks_to_expire;
	uint8_t data_chan_count;
	uint8_t *data_chan_map;
	uint32_t ticks_current;
	struct ll_adv_set *adv;
	struct pdu_adv *pdu;
	uint8_t ticker_id;
	uint8_t retry;
	uint8_t id;

	adv = param;
	lll_aux = adv->lll.aux;
	aux = HDR_LLL2ULL(lll_aux);
	ticker_id = TICKER_ID_ADV_AUX_BASE + ull_adv_aux_handle_get(aux);

	id = TICKER_NULL;
	ticks_to_expire = 0U;
	ticks_current = 0U;
	retry = 4U;
	do {
		uint32_t volatile ret_cb;
		uint32_t ticks_previous;
		uint32_t ret;
		bool success;

		ticks_previous = ticks_current;

		ret_cb = TICKER_STATUS_BUSY;
		ret = ticker_next_slot_get(TICKER_INSTANCE_ID_CTLR,
					   TICKER_USER_ID_ULL_LOW,
					   &id,
					   &ticks_current, &ticks_to_expire,
					   ticker_op_cb, (void *)&ret_cb);
		if (ret == TICKER_STATUS_BUSY) {
			while (ret_cb == TICKER_STATUS_BUSY) {
				ticker_job_sched(TICKER_INSTANCE_ID_CTLR,
						 TICKER_USER_ID_ULL_LOW);
			}
		}

		success = (ret_cb == TICKER_STATUS_SUCCESS);
		LL_ASSERT(success);

		LL_ASSERT((ticks_current == ticks_previous) || retry--);

		LL_ASSERT(id != TICKER_NULL);
	} while (id != ticker_id);

	/* Store the ticks offset for population in other advertising primary
	 * channel PDUs.
	 */
	lll_aux->ticks_offset = ticks_to_expire;

	/* NOTE: as remainder used in scheduling primary PDU not available,
	 * compensate with a probable jitter of one ticker resolution unit that
	 * would be included in the packet timer capture when scheduling next
	 * advertising primary channel PDU.
	 */
	lll_aux->ticks_offset +=
		HAL_TICKER_US_TO_TICKS(EVENT_TICKER_RES_MARGIN_US);

	/* FIXME: we are in ULL_LOW context, fill offset in LLL context? */
	pdu = lll_adv_data_latest_peek(&adv->lll);
	aux_ptr = ull_adv_aux_lll_offset_fill(pdu, ticks_to_expire, 0);

	/* Process channel map update, if any */
	if (aux->chm_first != aux->chm_last) {
		/* Use channelMapNew */
		aux->chm_first = aux->chm_last;
	}

	/* Calculate the radio channel to use */
	data_chan_map = aux->chm[aux->chm_first].data_chan_map;
	data_chan_count = aux->chm[aux->chm_first].data_chan_count;
	aux_ptr->chan_idx = lll_chan_sel_2(lll_aux->data_chan_counter,
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
	struct ll_adv_aux_set *aux = param;
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

	/* Kick LLL prepare */
	ret = mayfly_enqueue(TICKER_USER_ID_ULL_HIGH,
			     TICKER_USER_ID_LLL, 0, &mfy);
	LL_ASSERT(!ret);

#if defined(CONFIG_BT_CTLR_ADV_PERIODIC)
	struct ll_adv_set *adv;

	adv = HDR_LLL2ULL(lll->adv);
	if (adv->lll.sync) {
		struct ll_adv_sync_set *sync;

		sync  = HDR_LLL2ULL(adv->lll.sync);
		if (sync->is_started) {
			ull_adv_sync_offset_get(adv);
		}
	}
#endif /* CONFIG_BT_CTLR_ADV_PERIODIC */

	DEBUG_RADIO_PREPARE_A(1);
}

static void ticker_op_cb(uint32_t status, void *param)
{
	*((uint32_t volatile *)param) = status;
}
#else /* !(CONFIG_BT_CTLR_ADV_AUX_SET > 0) */

static int init_reset(void)
{
	return 0;
}
#endif /* !(CONFIG_BT_CTLR_ADV_AUX_SET > 0) */
