/*
 * Copyright (c) 2017-2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <soc.h>
#include <bluetooth/hci.h>
#include <sys/byteorder.h>

#include "hal/cpu.h"
#include "hal/ticker.h"

#include "util/util.h"
#include "util/mem.h"
#include "util/memq.h"
#include "util/mayfly.h"

#include "ticker/ticker.h"

#include "pdu.h"
#include "ll.h"
#include "lll.h"
#include "lll_vendor.h"
#include "lll_adv.h"
#include "lll_adv_aux.h"
#include "lll_adv_internal.h"

#include "ull_adv_types.h"

#include "ull_internal.h"
#include "ull_adv_internal.h"

#define BT_DBG_ENABLED IS_ENABLED(CONFIG_BT_DEBUG_HCI_DRIVER)
#define LOG_MODULE_NAME bt_ctlr_ull_adv_aux
#include "common/log.h"
#include "hal/debug.h"

static int init_reset(void);

#if (CONFIG_BT_CTLR_ADV_AUX_SET > 0)
static inline struct ll_adv_aux_set *aux_acquire(void);
static inline void aux_release(struct ll_adv_aux_set *aux);
static inline uint8_t aux_handle_get(struct ll_adv_aux_set *aux);
#if defined(CONFIG_BT_CTLR_ADV_PERIODIC)
static inline void sync_info_fill(struct lll_adv_sync *lll_sync,
				  uint8_t **dptr);
#endif /* CONFIG_BT_CTLR_ADV_PERIODIC */
static void mfy_aux_offset_get(void *param);
static void ticker_cb(uint32_t ticks_at_expire, uint32_t remainder,
		      uint16_t lazy, void *param);
static void ticker_op_cb(uint32_t status, void *param);

static struct ll_adv_aux_set ll_adv_aux_pool[CONFIG_BT_CTLR_ADV_AUX_SET];
static void *adv_aux_free;
#endif /* (CONFIG_BT_CTLR_ADV_AUX_SET > 0) */

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

	memcpy(adv->rnd_addr, addr, BDADDR_SIZE);

	return 0;
}

uint8_t const *ll_adv_aux_random_addr_get(struct ll_adv_set const *const adv,
				       uint8_t *const addr)
{
	if (addr) {
		memcpy(addr, adv->rnd_addr, BDADDR_SIZE);
	}

	return adv->rnd_addr;
}

uint8_t ll_adv_aux_ad_data_set(uint8_t handle, uint8_t op, uint8_t frag_pref, uint8_t len,
			    uint8_t const *const data)
{
	struct ll_adv_aux_set *aux;
	struct ll_adv_set *adv;
	uint8_t value[5];
	uint8_t *val_ptr;
	uint8_t err;

	/* op param definitions:
	 * 0x00 - Intermediate fragment of fragmented extended advertising data
	 * 0x01 - First fragment of fragmented extended advertising data
	 * 0x02 - Last fragemnt of fragemented extended advertising data
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
	*((uint32_t *)val_ptr) = (uint32_t)data;
	err = ull_adv_aux_hdr_set_clear(adv, ULL_ADV_PDU_HDR_FIELD_AD_DATA,
					0, value, NULL);
	if (err) {
		return err;
	}

	aux = (void *)HDR_LLL2EVT(adv->lll.aux);
	if (adv->is_enabled && !aux->is_started) {
		uint32_t ticks_slot_overhead;
		uint32_t volatile ret_cb;
		uint32_t ticks_anchor;
		uint32_t ret;

		ull_hdr_init(&aux->ull);

		aux->interval =	adv->interval +
				(HAL_TICKER_TICKS_TO_US(ULL_ADV_RANDOM_DELAY) /
				 625U);

		ticks_anchor = ticker_ticks_now_get();

		ticks_slot_overhead = ull_adv_aux_evt_init(aux);

		ret = ull_adv_aux_start(aux, ticks_anchor, ticks_slot_overhead,
					&ret_cb);
		ret = ull_ticker_status_take(ret, &ret_cb);
		if (ret != TICKER_STATUS_SUCCESS) {
			/* NOTE: This failure, to start an auxiliary channel
			 * radio event shall not occur unless a defect in the
			 * controller design.
			 */
			return BT_HCI_ERR_INSUFFICIENT_RESOURCES;
		}

		aux->is_started = 1;
	}

	return 0;
}

uint8_t ll_adv_aux_sr_data_set(uint8_t handle, uint8_t op, uint8_t frag_pref, uint8_t len,
			    uint8_t const *const data)
{
	struct pdu_adv_com_ext_adv *sr_com_hdr;
	struct pdu_adv *pri_pdu_prev;
	struct pdu_adv_hdr *sr_hdr;
	struct pdu_adv_adi *sr_adi;
	struct pdu_adv *sr_prev;
	struct pdu_adv *aux_pdu;
	struct ll_adv_set *adv;
	struct pdu_adv *sr_pdu;
	struct lll_adv *lll;
	uint8_t ext_hdr_len;
	uint8_t *sr_dptr;
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
		if ((op != BT_HCI_LE_EXT_ADV_OP_COMPLETE_DATA) || (len > 31)) {
			return BT_HCI_ERR_INVALID_PARAM;
		}
		return ull_scan_rsp_set(adv, len, data);
	}

	/* Can only set complete data and cannot discard data on enabled set */
	if (adv->is_enabled && ((op != BT_HCI_LE_EXT_ADV_OP_COMPLETE_DATA) ||
				(len == 0))) {
		return BT_HCI_ERR_CMD_DISALLOWED;
	}

	LL_ASSERT(lll->aux);
	aux_pdu = lll_adv_aux_data_peek(lll->aux);
	sr_prev = lll_adv_scan_rsp_peek(lll);

	/* Can only discard data on non-scannable instances */
	if (!(aux_pdu->adv_ext_ind.adv_mode & BT_HCI_LE_ADV_PROP_SCAN) && len) {
		return BT_HCI_ERR_INVALID_PARAM;
	}

	/* Update scan response PDU fields. */
	sr_pdu = lll_adv_scan_rsp_alloc(lll, &idx);
	sr_pdu->type = PDU_ADV_TYPE_AUX_SCAN_RSP;
	sr_pdu->rfu = 0;
	sr_pdu->chan_sel = 0;
	sr_pdu->tx_addr = aux_pdu->tx_addr;
	sr_pdu->rx_addr = 0;
	sr_pdu->len = 0;

	sr_com_hdr = &sr_pdu->adv_ext_ind;
	sr_hdr = (void *)&sr_com_hdr->ext_hdr_adi_adv_data[0];
	sr_dptr = (void *)sr_hdr;

	/* Flags */
	*sr_dptr = 0;
	sr_hdr->adv_addr = 1;
#if defined(CONFIG_BT_CTRL_ADV_ADI_IN_SCAN_RSP)
	sr_hdr->adi = 1;
#endif
	sr_dptr++;

	/* AdvA */
	memcpy(sr_dptr, &sr_prev->adv_ext_ind.ext_hdr_adi_adv_data[1],
	       BDADDR_SIZE);
	sr_dptr += BDADDR_SIZE;

#if defined(CONFIG_BT_CTRL_ADV_ADI_IN_SCAN_RSP)
	/* ADI */
	sr_adi = (void *)sr_dptr;
	sr_dptr += sizeof(struct pdu_adv_adi);
#else
	sr_adi = NULL;
#endif

	/* Check if data will fit in remaining space */
	/* TODO: need aux_chain_ind support */
	ext_hdr_len = sr_dptr - &sr_com_hdr->ext_hdr_adi_adv_data[0];
	if (sizeof(sr_com_hdr->ext_hdr_adi_adv_data) -
	    sr_com_hdr->ext_hdr_len < len) {
		return BT_HCI_ERR_PACKET_TOO_LONG;
	}

	/* Copy data */
	memcpy(sr_dptr, data, len);
	sr_dptr += len;

	/* Finish Common ExtAdv Payload header */
	sr_com_hdr->adv_mode = 0;
	sr_com_hdr->ext_hdr_len = ext_hdr_len;

	/* Finish PDU */
	sr_pdu->len = sr_dptr - &sr_pdu->payload[0];

	/* Trigger DID update */
	err = ull_adv_aux_hdr_set_clear(adv, 0, 0, NULL, sr_adi);
	if (err) {
		return err;
	}

	lll_adv_scan_rsp_enqueue(&adv->lll, idx);

	return 0;
}

uint16_t ll_adv_aux_max_data_length_get(void)
{
	return CONFIG_BT_CTLR_ADV_DATA_LEN_MAX;
}

uint8_t ll_adv_aux_set_count_get(void)
{
	return CONFIG_BT_CTLR_ADV_SET;
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

		sync = (void *)HDR_LLL2EVT(lll->sync);

		if (sync->is_enabled) {
			return BT_HCI_ERR_CMD_DISALLOWED;
		}
	}
#endif /* CONFIG_BT_CTLR_ADV_PERIODIC */

	/* Release auxiliary channel set */
	if (lll->aux) {
		struct ll_adv_aux_set *aux;

		aux = (void *)HDR_LLL2EVT(lll->aux);

		ull_adv_aux_release(aux);
	}

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

	err = init_reset();
	if (err) {
		return err;
	}

	return 0;
}

int ull_adv_aux_reset(void)
{
	int err;

	err = init_reset();
	if (err) {
		return err;
	}

	return 0;
}

uint8_t ull_adv_aux_hdr_set_clear(struct ll_adv_set *adv,
				  uint16_t sec_hdr_add_fields,
				  uint16_t sec_hdr_rem_fields,
				  void *value, struct pdu_adv_adi *adi)
{
	struct pdu_adv_com_ext_adv *pri_com_hdr, *pri_com_hdr_prev;
	struct pdu_adv_com_ext_adv *sec_com_hdr, *sec_com_hdr_prev;
	struct pdu_adv_hdr *pri_hdr, pri_hdr_prev;
	struct pdu_adv_hdr *sec_hdr, sec_hdr_prev;
	struct pdu_adv *pri_pdu, *pri_pdu_prev;
	struct pdu_adv *sec_pdu_prev, *sec_pdu;
	uint8_t pri_len, sec_len, sec_len_prev;
	uint8_t *pri_dptr, *pri_dptr_prev;
	uint8_t *sec_dptr, *sec_dptr_prev;
	uint8_t pri_idx, sec_idx, ad_len;
	struct lll_adv_aux *lll_aux;
	struct lll_adv *lll;
	uint8_t is_aux_new;
	uint8_t *ad_data;

	lll = &adv->lll;

	/* Can't have both flags set here since both use 'value' extra param */
	LL_ASSERT(!(sec_hdr_add_fields & ULL_ADV_PDU_HDR_FIELD_ADVA) ||
		  !(sec_hdr_add_fields & ULL_ADV_PDU_HDR_FIELD_AD_DATA));

	/* Get reference to previous primary PDU data */
	pri_pdu_prev = lll_adv_data_peek(lll);
	if (pri_pdu_prev->type != PDU_ADV_TYPE_EXT_IND) {
		if (sec_hdr_add_fields & ULL_ADV_PDU_HDR_FIELD_AD_DATA) {
			uint8_t *val_ptr = value;

			ad_len = *val_ptr;
			val_ptr++;

			ad_data = (void *)*((uint32_t *)val_ptr);

			return ull_adv_data_set(adv, ad_len, ad_data);
		}

		return BT_HCI_ERR_CMD_DISALLOWED;
	}

	pri_com_hdr_prev = (void *)&pri_pdu_prev->adv_ext_ind;
	pri_hdr = (void *)pri_com_hdr_prev->ext_hdr_adi_adv_data;
	pri_hdr_prev = *pri_hdr;
	pri_dptr_prev = (uint8_t *)pri_hdr + sizeof(*pri_hdr);

	/* Advertising data are not supported by scannable instances */
	if ((sec_hdr_add_fields & ULL_ADV_PDU_HDR_FIELD_AD_DATA) &&
	    (pri_com_hdr_prev->adv_mode & BT_HCI_LE_ADV_PROP_SCAN)) {
		return BT_HCI_ERR_INVALID_PARAM;
	}

	/* Get reference to new primary PDU data buffer */
	pri_pdu = lll_adv_data_alloc(lll, &pri_idx);
	pri_pdu->type = pri_pdu_prev->type;
	pri_pdu->rfu = 0U;
	pri_pdu->chan_sel = 0U;
	pri_com_hdr = (void *)&pri_pdu->adv_ext_ind;
	pri_com_hdr->adv_mode = pri_com_hdr_prev->adv_mode;
	pri_hdr = (void *)pri_com_hdr->ext_hdr_adi_adv_data;
	pri_dptr = (uint8_t *)pri_hdr + sizeof(*pri_hdr);
	*(uint8_t *)pri_hdr = 0U;

	/* Get the reference to aux instance */
	lll_aux = lll->aux;
	if (!lll_aux) {
		struct ll_adv_aux_set *aux;

		aux = ull_adv_aux_acquire(lll);
		if (!aux) {
			return BT_HCI_ERR_MEM_CAPACITY_EXCEEDED;
		}

		lll_aux = &aux->lll;

		is_aux_new = 1U;
	} else {
		is_aux_new = 0U;
	}

	/* Get reference to previous secondary PDU data */
	sec_pdu_prev = lll_adv_aux_data_peek(lll_aux);
	sec_com_hdr_prev = (void *)&sec_pdu_prev->adv_ext_ind;
	sec_hdr = (void *)sec_com_hdr_prev->ext_hdr_adi_adv_data;
	if (!is_aux_new) {
		sec_hdr_prev = *sec_hdr;
	} else {
		/* Initialize only those fields used to copy into new PDU
		 * buffer.
		 */
		sec_pdu_prev->tx_addr = 0U;
		sec_pdu_prev->rx_addr = 0U;
		sec_pdu_prev->len = offsetof(struct pdu_adv_com_ext_adv,
					     ext_hdr_adi_adv_data);
		*(uint8_t *)&sec_hdr_prev = 0U;
	}
	sec_dptr_prev = (uint8_t *)sec_hdr + sizeof(*sec_hdr);

	/* Get reference to new secondary PDU data buffer */
	sec_pdu = lll_adv_aux_data_alloc(lll_aux, &sec_idx);
	sec_pdu->type = pri_pdu->type;
	sec_pdu->rfu = 0U;
	sec_pdu->chan_sel = 0U;

	sec_pdu->tx_addr = sec_pdu_prev->tx_addr;
	sec_pdu->rx_addr = sec_pdu_prev->rx_addr;

	sec_com_hdr = (void *)&sec_pdu->adv_ext_ind;
	sec_com_hdr->adv_mode = pri_com_hdr->adv_mode;
	sec_hdr = (void *)sec_com_hdr->ext_hdr_adi_adv_data;
	sec_dptr = (uint8_t *)sec_hdr + sizeof(*sec_hdr);
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
	pri_pdu->rx_addr = 0U;

	if (pri_hdr_prev.adv_addr) {
		pri_dptr_prev += BDADDR_SIZE;
	}
	if (sec_hdr_prev.adv_addr) {
		sec_dptr_prev += BDADDR_SIZE;
	}
	sec_dptr += BDADDR_SIZE;

	/* No TargetA in primary and secondary channel for undirected */
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
	if (sec_hdr_prev.aux_ptr) {
		sec_dptr_prev += sizeof(struct pdu_adv_aux_ptr);

		sec_hdr->aux_ptr = 1;
		sec_dptr += sizeof(struct pdu_adv_aux_ptr);
	}

	/* No SyncInfo flag in primary channel PDU */
	/* Add/Remove SyncInfo flag in secondary channel PDU */
	if ((sec_hdr_add_fields & ULL_ADV_PDU_HDR_FIELD_SYNC_INFO) ||
	    (!(sec_hdr_rem_fields & ULL_ADV_PDU_HDR_FIELD_SYNC_INFO) &&
	     sec_hdr_prev.sync_info)) {
		sec_hdr->sync_info = 1;
	}
	if (sec_hdr_prev.sync_info) {
		sec_dptr_prev += sizeof(struct pdu_adv_sync_info);
	}
	if (sec_hdr->sync_info) {
		sec_dptr += sizeof(struct pdu_adv_sync_info);
	}

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
	pri_len = ull_adv_aux_hdr_len_get(pri_com_hdr, pri_dptr);
	ull_adv_aux_hdr_len_fill(pri_com_hdr, pri_len);

	/* set the primary PDU len */
	pri_pdu->len = pri_len;

	/* Calc previous secondary PDU len */
	sec_len_prev = ull_adv_aux_hdr_len_get(sec_com_hdr_prev, sec_dptr_prev);

	/* Did we parse beyond PDU length? */
	if (sec_len_prev > sec_pdu_prev->len) {
		/* we should not encounter invalid length */
		/* FIXME: release allocations */
		return BT_HCI_ERR_UNSPECIFIED;
	}

	/* Calc current secondary PDU len */
	sec_len = ull_adv_aux_hdr_len_get(sec_com_hdr, sec_dptr);
	ull_adv_aux_hdr_len_fill(sec_com_hdr, sec_len);

	/* AD Data, add or remove */
	if (sec_hdr_add_fields & ULL_ADV_PDU_HDR_FIELD_AD_DATA) {
		uint8_t *val_ptr = value;

		ad_len = *val_ptr;
		val_ptr++;

		ad_data = (void *)*((uint32_t *)val_ptr);
	} else {
		/* Calc the previous AD data length in auxiliary PDU */
		ad_len = sec_pdu_prev->len - sec_len_prev;
		ad_data = sec_dptr_prev;
	}

	/* set the secondary PDU len */
	sec_pdu->len = sec_len + ad_len;

	/* Check AdvData overflow */
	if (sec_pdu->len > CONFIG_BT_CTLR_ADV_DATA_LEN_MAX) {
		/* FIXME: release allocations */
		return BT_HCI_ERR_PACKET_TOO_LONG;
	}

	/* Start filling pri and sec PDU payload based on flags from here
	 * ==============================================================
	 */

	/* No AdvData in primary channel PDU */
	/* Fill AdvData in secondary PDU */
	memcpy(sec_dptr, ad_data, ad_len);

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
		sec_dptr_prev -= sizeof(struct pdu_adv_sync_info);
	}
	if (sec_hdr->sync_info) {
		sync_info_fill(lll->sync, &sec_dptr);
	}
#endif /* CONFIG_BT_CTLR_ADV_PERIODIC */

	/* AuxPtr */
	if (pri_hdr_prev.aux_ptr) {
		pri_dptr_prev -= sizeof(struct pdu_adv_aux_ptr);
	}
	ull_adv_aux_ptr_fill(&pri_dptr, lll->phy_s);

	if (sec_hdr_prev.aux_ptr) {
		sec_dptr_prev -= sizeof(struct pdu_adv_aux_ptr);

		ull_adv_aux_ptr_fill(&sec_dptr, lll->phy_s);
	}

	/* ADI */
	{
		struct pdu_adv_adi *pri_adi, *sec_adi;
		uint16_t did = UINT16_MAX;

		pri_dptr -= sizeof(struct pdu_adv_adi);
		sec_dptr -= sizeof(struct pdu_adv_adi);

		pri_adi = (void *)pri_dptr;
		sec_adi = (void *)sec_dptr;

		if (pri_hdr_prev.adi) {
			struct pdu_adv_adi *pri_adi_prev;

			pri_dptr_prev -= sizeof(struct pdu_adv_adi);
			sec_dptr_prev -= sizeof(struct pdu_adv_adi);

			/* NOTE: memcpy shall handle overlapping buffers
			 */
			memcpy(pri_dptr, pri_dptr_prev,
			       sizeof(struct pdu_adv_adi));
			memcpy(sec_dptr, sec_dptr_prev,
			       sizeof(struct pdu_adv_adi));

			pri_adi_prev = (void *)pri_dptr_prev;
			did = sys_le16_to_cpu(pri_adi_prev->did);
		} else {
			pri_adi->sid = adv->sid;
			sec_adi->sid = adv->sid;
		}

		did++;

		pri_adi->did = sys_cpu_to_le16(did);
		sec_adi->did = sys_cpu_to_le16(did);

		if (adi) {
			*adi = *pri_adi;
		}
	}

	/* No CTEInfo field in primary channel PDU */

	/* No TargetA non-conn non-scan advertising  */

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

		memcpy(sec_dptr, bdaddr, BDADDR_SIZE);
	}

	lll_adv_aux_data_enqueue(lll_aux, sec_idx);
	lll_adv_data_enqueue(lll, pri_idx);

	return 0;
}

void ull_adv_aux_ptr_fill(uint8_t **dptr, uint8_t phy_s)
{
	struct pdu_adv_aux_ptr *aux_ptr;

	*dptr -= sizeof(struct pdu_adv_aux_ptr);

	/* NOTE: Aux Offset will be set in advertiser LLL event
	 */
	aux_ptr = (void *)*dptr;

	/* FIXME: implementation defined */
	aux_ptr->chan_idx = 0U;
	aux_ptr->ca = 0U;
	aux_ptr->offs_units = 0U;

	aux_ptr->phy = find_lsb_set(phy_s) - 1;
}

#if (CONFIG_BT_CTLR_ADV_AUX_SET > 0)
uint8_t ull_adv_aux_lll_handle_get(struct lll_adv_aux *lll)
{
	return aux_handle_get((void *)lll->hdr.parent);
}

uint32_t ull_adv_aux_evt_init(struct ll_adv_aux_set *aux)
{
	uint32_t slot_us = EVENT_OVERHEAD_START_US + EVENT_OVERHEAD_END_US;
	uint32_t ticks_slot_overhead;

	/* TODO: Calc AUX_ADV_IND slot_us */
	slot_us += 1000;

	/* TODO: active_to_start feature port */
	aux->evt.ticks_active_to_start = 0;
	aux->evt.ticks_xtal_to_start =
		HAL_TICKER_US_TO_TICKS(EVENT_OVERHEAD_XTAL_US);
	aux->evt.ticks_preempt_to_start =
		HAL_TICKER_US_TO_TICKS(EVENT_OVERHEAD_PREEMPT_MIN_US);
	aux->evt.ticks_slot = HAL_TICKER_US_TO_TICKS(slot_us);

	if (IS_ENABLED(CONFIG_BT_CTLR_LOW_LAT)) {
		ticks_slot_overhead = MAX(aux->evt.ticks_active_to_start,
					  aux->evt.ticks_xtal_to_start);
	} else {
		ticks_slot_overhead = 0;
	}

	return ticks_slot_overhead;
}

uint32_t ull_adv_aux_start(struct ll_adv_aux_set *aux, uint32_t ticks_anchor,
			   uint32_t ticks_slot_overhead,
			   uint32_t volatile *ret_cb)
{
	uint8_t aux_handle;
	uint32_t ret;

	aux_handle = aux_handle_get(aux);

	*ret_cb = TICKER_STATUS_BUSY;
	ret = ticker_start(TICKER_INSTANCE_ID_CTLR, TICKER_USER_ID_THREAD,
			   (TICKER_ID_ADV_AUX_BASE + aux_handle),
			   ticks_anchor, 0,
			   HAL_TICKER_US_TO_TICKS((uint64_t)aux->interval *
						  625),
			   TICKER_NULL_REMAINDER, TICKER_NULL_LAZY,
			   (aux->evt.ticks_slot + ticks_slot_overhead),
			   ticker_cb, aux,
			   ull_ticker_status_give, (void *)ret_cb);

	return ret;
}

uint8_t ull_adv_aux_stop(struct ll_adv_aux_set *aux)
{
	uint32_t volatile ret_cb;
	uint8_t aux_handle;
	void *mark;
	uint32_t ret;

	mark = ull_disable_mark(aux);
	LL_ASSERT(mark == aux);

	aux_handle = aux_handle_get(aux);

	ret_cb = TICKER_STATUS_BUSY;
	ret = ticker_stop(TICKER_INSTANCE_ID_CTLR, TICKER_USER_ID_THREAD,
			  TICKER_ID_ADV_AUX_BASE + aux_handle,
			  ull_ticker_status_give, (void *)&ret_cb);
	ret = ull_ticker_status_take(ret, &ret_cb);
	if (ret) {
		mark = ull_disable_mark(aux);
		LL_ASSERT(mark == aux);

		return BT_HCI_ERR_CMD_DISALLOWED;
	}

	ret = ull_disable(&aux->lll);
	LL_ASSERT(!ret);

	mark = ull_disable_unmark(aux);
	LL_ASSERT(mark == aux);

	aux->is_started = 0U;

	return 0;
}

struct ll_adv_aux_set *ull_adv_aux_acquire(struct lll_adv *lll)
{
	struct lll_adv_aux *lll_aux;
	struct ll_adv_aux_set *aux;

	aux = aux_acquire();
	if (!aux) {
		return aux;
	}

	lll_aux = &aux->lll;
	lll->aux = lll_aux;
	lll_aux->adv = lll;

	/* NOTE: ull_hdr_init(&aux->ull); is done on start */
	lll_hdr_init(lll_aux, aux);

	aux->is_started = 0U;

	return aux;
}

void ull_adv_aux_release(struct ll_adv_aux_set *aux)
{
	aux_release(aux);
}

void ull_adv_aux_offset_get(struct ll_adv_set *adv)
{
	static memq_link_t link;
	static struct mayfly mfy = {0, 0, &link, NULL, mfy_aux_offset_get};
	uint32_t ret;

	mfy.param = adv;
	ret = mayfly_enqueue(TICKER_USER_ID_ULL_HIGH, TICKER_USER_ID_ULL_LOW, 1,
			     &mfy);
	LL_ASSERT(!ret);
}

struct pdu_adv_aux_ptr *ull_adv_aux_lll_offset_fill(uint32_t ticks_offset,
						    uint32_t start_us,
						    struct pdu_adv *pdu)
{
	struct pdu_adv_com_ext_adv *pri_com_hdr;
	struct pdu_adv_aux_ptr *aux;
	struct pdu_adv_hdr *h;
	uint8_t *ptr;

	pri_com_hdr = (void *)&pdu->adv_ext_ind;
	h = (void *)pri_com_hdr->ext_hdr_adi_adv_data;
	ptr = (uint8_t *)h + sizeof(*h);

	if (h->adv_addr) {
		ptr += BDADDR_SIZE;
	}

	if (h->adi) {
		ptr += sizeof(struct pdu_adv_adi);
	}

	aux = (void *)ptr;
	aux->offs = (HAL_TICKER_TICKS_TO_US(ticks_offset) - start_us) / 30;
	if (aux->offs_units) {
		aux->offs /= 10;
	}

	return aux;
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

static inline uint8_t aux_handle_get(struct ll_adv_aux_set *aux)
{
	return mem_index_get(aux, ll_adv_aux_pool,
			     sizeof(struct ll_adv_aux_set));
}

#if defined(CONFIG_BT_CTLR_ADV_PERIODIC)
static inline void sync_info_fill(struct lll_adv_sync *lll_sync,
				  uint8_t **dptr)
{
	struct ll_adv_sync_set *sync;
	struct pdu_adv_sync_info *si;

	*dptr -= sizeof(*si);

	sync = (void *)HDR_LLL2EVT(lll_sync);

	si = (void *)*dptr;
	si->offs = 0U; /* NOTE: Filled by secondary prepare */
	si->offs_units = 0U; /* TODO: implementation defined */
	si->interval = sys_cpu_to_le16(sync->interval);
	memcpy(si->sca_chm, lll_sync->data_chan_map,
	       sizeof(si->sca_chm));
	memcpy(&si->aa, lll_sync->access_addr, sizeof(si->aa));
	memcpy(si->crc_init, lll_sync->crc_init, sizeof(si->crc_init));

	si->evt_cntr = 0U; /* NOTE: Filled by secondary prepare */
}
#endif /* CONFIG_BT_CTLR_ADV_PERIODIC */

static void mfy_aux_offset_get(void *param)
{
	struct ll_adv_set *adv = param;
	struct ll_adv_aux_set *aux;
	uint32_t ticks_to_expire;
	uint32_t ticks_current;
	struct pdu_adv *pdu;
	uint8_t ticker_id;
	uint8_t retry;
	uint8_t id;

	aux = (void *)HDR_LLL2EVT(adv->lll.aux);
	ticker_id = TICKER_ID_ADV_AUX_BASE + aux_handle_get(aux);

	id = TICKER_NULL;
	ticks_to_expire = 0U;
	ticks_current = 0U;
	retry = 4U;
	do {
		uint32_t volatile ret_cb;
		uint32_t ticks_previous;
		uint32_t ret;

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

		LL_ASSERT(ret_cb == TICKER_STATUS_SUCCESS);

		LL_ASSERT((ticks_current == ticks_previous) || retry--);

		LL_ASSERT(id != TICKER_NULL);
	} while (id != ticker_id);

	/* Store the ticks offset for population in other advertising primary
	 * channel PDUs.
	 */
	aux->lll.ticks_offset = ticks_to_expire;

	/* NOTE: as remainder used in scheduling primary PDU not available,
	 * compensate with a probable jitter of one ticker resolution unit that
	 * would be included in the packet timer capture when scheduling next
	 * advertising primary channel PDU.
	 */
	aux->lll.ticks_offset +=
		HAL_TICKER_US_TO_TICKS(EVENT_TICKER_RES_MARGIN_US);

	/* FIXME: we are in ULL_LOW context, fill offset in LLL context */
	pdu = lll_adv_data_curr_get(&adv->lll);
	ull_adv_aux_lll_offset_fill(ticks_to_expire, 0, pdu);
}

static void ticker_cb(uint32_t ticks_at_expire, uint32_t remainder,
		      uint16_t lazy, void *param)
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
	p.param = lll;
	mfy.param = &p;

	/* Kick LLL prepare */
	ret = mayfly_enqueue(TICKER_USER_ID_ULL_HIGH,
			     TICKER_USER_ID_LLL, 0, &mfy);
	LL_ASSERT(!ret);

#if defined(CONFIG_BT_CTLR_ADV_PERIODIC)
	struct ll_adv_set *adv;

	adv = (void *)HDR_LLL2EVT(lll->adv);
	if (adv->lll.sync) {
		struct ll_adv_sync_set *sync;

		sync  = (void *)HDR_LLL2EVT(adv->lll.sync);
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
