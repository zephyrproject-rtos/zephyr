/*
 * Copyright (c) 2017-2021 Nordic Semiconductor ASA
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

#include "lll.h"
#include "lll_clock.h"
#include "lll/lll_vendor.h"
#include "lll/lll_adv_types.h"
#include "lll_adv.h"
#include "lll/lll_adv_pdu.h"
#include "lll_adv_sync.h"
#include "lll/lll_df_types.h"
#include "lll_chan.h"

#include "ull_adv_types.h"

#include "ull_internal.h"
#include "ull_chan_internal.h"
#include "ull_adv_internal.h"

#include "ll.h"

#define BT_DBG_ENABLED IS_ENABLED(CONFIG_BT_DEBUG_HCI_DRIVER)
#define LOG_MODULE_NAME bt_ctlr_ull_adv_sync
#include "common/log.h"
#include "hal/debug.h"

static int init_reset(void);
static inline struct ll_adv_sync_set *sync_acquire(void);
static inline void sync_release(struct ll_adv_sync_set *sync);
static inline uint16_t sync_handle_get(struct ll_adv_sync_set *sync);
static inline uint8_t sync_remove(struct ll_adv_sync_set *sync,
				  struct ll_adv_set *adv, uint8_t enable);
static uint16_t sync_time_get(struct ll_adv_sync_set *sync,
			      struct pdu_adv *pdu);

static void mfy_sync_offset_get(void *param);
static inline struct pdu_adv_sync_info *sync_info_get(struct pdu_adv *pdu);
static inline void sync_info_offset_fill(struct pdu_adv_sync_info *si,
					 uint32_t ticks_offset,
					 uint32_t start_us);
static void ticker_cb(uint32_t ticks_at_expire, uint32_t remainder,
		      uint16_t lazy, uint8_t force, void *param);
static void ticker_op_cb(uint32_t status, void *param);

static struct ll_adv_sync_set ll_adv_sync_pool[CONFIG_BT_CTLR_ADV_SYNC_SET];
static void *adv_sync_free;

static void adv_sync_pdu_init(struct pdu_adv *pdu, uint8_t ext_hdr_flags)
{
	struct pdu_adv_com_ext_adv *com_hdr;
	struct pdu_adv_ext_hdr *ext_hdr;
	uint8_t *dptr;
	uint8_t len;

	pdu->type = PDU_ADV_TYPE_AUX_SYNC_IND;
	pdu->rfu = 0U;
	pdu->chan_sel = 0U;

	pdu->tx_addr = 0U;
	pdu->rx_addr = 0U;

	com_hdr = &pdu->adv_ext_ind;
	/* Non-connectable and Non-scannable adv mode */
	com_hdr->adv_mode = 0U;

	ext_hdr = &com_hdr->ext_hdr;
	*(uint8_t *)ext_hdr = ext_hdr_flags;
	dptr = ext_hdr->data;

	LL_ASSERT(!(ext_hdr_flags & (ULL_ADV_PDU_HDR_FIELD_ADVA |
				     ULL_ADV_PDU_HDR_FIELD_TARGETA |
				     ULL_ADV_PDU_HDR_FIELD_ADI |
				     ULL_ADV_PDU_HDR_FIELD_SYNC_INFO)));

	if (ext_hdr_flags & ULL_ADV_PDU_HDR_FIELD_CTE_INFO) {
		dptr += sizeof(struct pdu_cte_info);
	}
	if (ext_hdr_flags & ULL_ADV_PDU_HDR_FIELD_AUX_PTR) {
		dptr += sizeof(struct pdu_adv_aux_ptr);
	}
	if (ext_hdr_flags & ULL_ADV_PDU_HDR_FIELD_TX_POWER) {
		dptr += sizeof(uint8_t);
	}

	/* Calc tertiary PDU len */
	len = ull_adv_aux_hdr_len_calc(com_hdr, &dptr);
	ull_adv_aux_hdr_len_fill(com_hdr, len);

	pdu->len = len;
}

#if defined(CONFIG_BT_CTLR_ADV_PDU_LINK)
static uint8_t adv_sync_pdu_init_from_prev_pdu(struct pdu_adv *pdu,
					       struct pdu_adv *pdu_prev,
					       uint16_t ext_hdr_flags_add,
					       uint16_t ext_hdr_flags_rem)
{
	struct pdu_adv_com_ext_adv *com_hdr_prev;
	struct pdu_adv_ext_hdr *ext_hdr_prev;
	struct pdu_adv_com_ext_adv *com_hdr;
	struct pdu_adv_ext_hdr *ext_hdr;
	uint8_t ext_hdr_flags_prev;
	uint8_t ext_hdr_flags;
	uint8_t *dptr_prev;
	uint8_t len_prev;
	uint8_t *dptr;
	uint8_t len;

	/* Copy complete header, assume it was set properly in old PDU */
	*(uint8_t *)pdu = *(uint8_t *)pdu_prev;

	com_hdr_prev = &pdu_prev->adv_ext_ind;
	com_hdr = &pdu->adv_ext_ind;

	com_hdr->adv_mode = 0U;

	ext_hdr_prev = &com_hdr_prev->ext_hdr;
	ext_hdr = &com_hdr->ext_hdr;

	if (com_hdr_prev->ext_hdr_len) {
		ext_hdr_flags_prev = *(uint8_t *) ext_hdr_prev;
	} else {
		ext_hdr_flags_prev = 0;
	}
	ext_hdr_flags = ext_hdr_flags_prev |
			(ext_hdr_flags_add & (~ext_hdr_flags_rem));

	*(uint8_t *)ext_hdr = ext_hdr_flags;

	LL_ASSERT(!ext_hdr->adv_addr);
	LL_ASSERT(!ext_hdr->tgt_addr);
	LL_ASSERT(!ext_hdr->adi);
	LL_ASSERT(!ext_hdr->sync_info);

	dptr = ext_hdr->data;
	dptr_prev = ext_hdr_prev->data;

	/* Note: skip length verification of ext header writes as we assume that
	 *       all PDUs are large enough to store at least complete ext header.
	 */

	/* Copy CTEInfo, if applicable */
	if (ext_hdr->cte_info) {
		if (ext_hdr_prev->cte_info) {
			memcpy(dptr, dptr_prev, sizeof(struct pdu_cte_info));
		}
		dptr += sizeof(struct pdu_cte_info);
	}
	if (ext_hdr_prev->cte_info) {
		dptr_prev += sizeof(struct pdu_cte_info);
	}

	/* Add AuxPtr, if applicable. Do not copy since it will be updated later
	 * anyway.
	 */
	if (ext_hdr->aux_ptr) {
		dptr += sizeof(struct pdu_adv_aux_ptr);
	}
	if (ext_hdr_prev->aux_ptr) {
		dptr_prev += sizeof(struct pdu_adv_aux_ptr);
	}

	/* Copy TxPower, if applicable */
	if (ext_hdr->tx_pwr) {
		if (ext_hdr_prev->tx_pwr) {
			memcpy(dptr, dptr_prev, sizeof(uint8_t));
		}
		dptr += sizeof(uint8_t);
	}
	if (ext_hdr_prev->tx_pwr) {
		dptr_prev += sizeof(uint8_t);
	}

	LL_ASSERT(ext_hdr_prev  >= 0);

	/* Copy ACAD */
	len = com_hdr_prev->ext_hdr_len - (dptr_prev - (uint8_t *)ext_hdr_prev);
	memcpy(dptr, dptr_prev, len);
	dptr += len;

	/* Check populated ext header length excluding length itself. If 0, then
	 * there was neither field nor ACAD populated and we skip ext header
	 * entirely.
	 */
	len = dptr - ext_hdr->data;
	if (len == 0) {
		com_hdr->ext_hdr_len = 0;
	} else {
		com_hdr->ext_hdr_len = len +
				       offsetof(struct pdu_adv_ext_hdr, data);
	}

	/* Both PDUs have now ext header length calculated properly, reset
	 * pointers to start of AD.
	 */
	dptr = &com_hdr->ext_hdr_adv_data[com_hdr->ext_hdr_len];
	dptr_prev = &com_hdr_prev->ext_hdr_adv_data[com_hdr_prev->ext_hdr_len];

	/* Calculate length of AD to copy and AD length available in new PDU */
	len_prev = pdu_prev->len - (dptr_prev - pdu_prev->payload);
	len = PDU_AC_PAYLOAD_SIZE_MAX - (dptr - pdu->payload);

	/* TODO: we should allow partial copy and let caller refragment data */
	if (len < len_prev) {
		return BT_HCI_ERR_PACKET_TOO_LONG;
	}

	/* Copy AD */
	if (!(ext_hdr_flags_rem & ULL_ADV_PDU_HDR_FIELD_AD_DATA)) {
		len = MIN(len, len_prev);
		memcpy(dptr, dptr_prev, len);
		dptr += len;
	}

	/* Finalize PDU */
	pdu->len = dptr - pdu->payload;

	return 0;
}

static uint8_t adv_sync_pdu_ad_data_set(struct pdu_adv *pdu,
					const uint8_t *data, uint8_t len)
{
	struct pdu_adv_com_ext_adv *com_hdr;
	uint8_t len_max;
	uint8_t *dptr;

	com_hdr = &pdu->adv_ext_ind;

	dptr = &com_hdr->ext_hdr_adv_data[com_hdr->ext_hdr_len];

	len_max = PDU_AC_PAYLOAD_SIZE_MAX - (dptr - pdu->payload);
	/* TODO: we should allow partial copy and let caller refragment data */
	if (len > len_max) {
		return BT_HCI_ERR_PACKET_TOO_LONG;
	}

	memcpy(dptr, data, len);
	dptr += len;

	pdu->len = dptr - pdu->payload;

	return 0;
}

static uint8_t adv_sync_pdu_cte_info_set(struct pdu_adv *pdu,
					 const struct pdu_cte_info *cte_info)
{
	struct pdu_adv_com_ext_adv *com_hdr;
	struct pdu_adv_ext_hdr *ext_hdr;
	uint8_t *dptr;

	com_hdr = &pdu->adv_ext_ind;
	ext_hdr = &com_hdr->ext_hdr;
	dptr = ext_hdr->data;

	/* Periodic adv PDUs do not have AdvA/TargetA */
	LL_ASSERT(!ext_hdr->adv_addr);
	LL_ASSERT(!ext_hdr->tgt_addr);

	if (ext_hdr->cte_info) {
		memcpy(dptr, cte_info, sizeof(*cte_info));
	}

	return 0;
}

static struct pdu_adv *adv_sync_pdu_duplicate_chain(struct pdu_adv *pdu)
{
	struct pdu_adv *pdu_dup = NULL;
	uint8_t err;

	while (pdu) {
		struct pdu_adv *pdu_new;

		pdu_new = lll_adv_pdu_alloc_pdu_adv();

		/* We make exact copy of old PDU, there's really nothing that
		 * can go wrong there assuming original PDU was created properly
		 */
		err = adv_sync_pdu_init_from_prev_pdu(pdu_new, pdu, 0, 0);
		LL_ASSERT(err == 0);

		if (pdu_dup) {
			lll_adv_pdu_linked_append_end(pdu_new, pdu_dup);
		} else {
			pdu_dup = pdu_new;
		}

		pdu = lll_adv_pdu_linked_next_get(pdu);
	}

	return pdu_dup;
}
#endif /* CONFIG_BT_CTLR_ADV_PDU_LINK */

uint8_t ll_adv_sync_param_set(uint8_t handle, uint16_t interval, uint16_t flags)
{
	void *extra_data_prev, *extra_data;
	struct pdu_adv *pdu_prev, *pdu;
	struct lll_adv_sync *lll_sync;
	struct ll_adv_sync_set *sync;
	struct ll_adv_set *adv;
	uint8_t err, ter_idx;

	adv = ull_adv_is_created_get(handle);
	if (!adv) {
		return BT_HCI_ERR_UNKNOWN_ADV_IDENTIFIER;
	}

	lll_sync = adv->lll.sync;
	if (!lll_sync) {
		struct pdu_adv *ter_pdu;
		struct lll_adv *lll;
		int err;

		sync = sync_acquire();
		if (!sync) {
			return BT_HCI_ERR_MEM_CAPACITY_EXCEEDED;
		}

		lll = &adv->lll;
		lll_sync = &sync->lll;
		lll->sync = lll_sync;
		lll_sync->adv = lll;

		lll_adv_data_reset(&lll_sync->data);
		err = lll_adv_data_init(&lll_sync->data);
		if (err) {
			return BT_HCI_ERR_MEM_CAPACITY_EXCEEDED;
		}

		/* NOTE: ull_hdr_init(&sync->ull); is done on start */
		lll_hdr_init(lll_sync, sync);

		err = util_aa_le32(lll_sync->access_addr);
		LL_ASSERT(!err);

		lll_sync->data_chan_id = lll_chan_id(lll_sync->access_addr);
		lll_sync->data_chan_count =
			ull_chan_map_get(lll_sync->data_chan_map);

		lll_csrand_get(lll_sync->crc_init, sizeof(lll_sync->crc_init));

		lll_sync->latency_prepare = 0;
		lll_sync->latency_event = 0;
		lll_sync->event_counter = 0;

		sync->is_enabled = 0U;
		sync->is_started = 0U;

		ter_pdu = lll_adv_sync_data_peek(lll_sync, NULL);
		adv_sync_pdu_init(ter_pdu, 0);
	} else {
		sync = HDR_LLL2ULL(lll_sync);
	}

	/* Periodic Advertising is already started */
	if (sync->is_started) {
		return BT_HCI_ERR_CMD_DISALLOWED;
	}

	sync->interval = interval;

	err = ull_adv_sync_pdu_alloc(adv, ULL_ADV_PDU_EXTRA_DATA_ALLOC_IF_EXIST, &pdu_prev, &pdu,
				     &extra_data_prev, &extra_data, &ter_idx);
	if (err) {
		return err;
	}

#if defined(CONFIG_BT_CTLR_DF_ADV_CTE_TX)
	if (extra_data) {
		ull_adv_sync_extra_data_set_clear(extra_data_prev, extra_data,
						  0, 0, NULL);
	}
#endif /* CONFIG_BT_CTLR_DF_ADV_CTE_TX */

	err = ull_adv_sync_pdu_set_clear(lll_sync, pdu_prev, pdu, 0, 0, NULL);
	if (err) {
		return err;
	}

	lll_adv_sync_data_enqueue(lll_sync, ter_idx);

	return 0;
}

uint8_t ll_adv_sync_ad_data_set(uint8_t handle, uint8_t op, uint8_t len,
				uint8_t const *const data)
{
	uint8_t hdr_data[1 + sizeof(data)];
	void *extra_data_prev, *extra_data;
	struct pdu_adv *pdu_prev, *pdu;
	struct lll_adv_sync *lll_sync;
	struct ll_adv_sync_set *sync;
	struct ll_adv_set *adv;
	uint8_t ter_idx;
	uint8_t err;

	/* TODO: handle other op values */
	if (op != BT_HCI_LE_EXT_ADV_OP_COMPLETE_DATA &&
	    op != BT_HCI_LE_EXT_ADV_OP_UNCHANGED_DATA) {
		/* FIXME: error code */
		return BT_HCI_ERR_CMD_DISALLOWED;
	}

	adv =  ull_adv_is_created_get(handle);
	if (!adv) {
		return BT_HCI_ERR_UNKNOWN_ADV_IDENTIFIER;
	}

	lll_sync = adv->lll.sync;
	if (!lll_sync) {
		return BT_HCI_ERR_UNKNOWN_ADV_IDENTIFIER;
	}

	hdr_data[0] = len;
	memcpy(&hdr_data[1], &data, sizeof(data));

	err = ull_adv_sync_pdu_alloc(adv, ULL_ADV_PDU_EXTRA_DATA_ALLOC_IF_EXIST, &pdu_prev, &pdu,
				     &extra_data_prev, &extra_data, &ter_idx);
	if (err) {
		return err;
	}

#if defined(CONFIG_BT_CTLR_DF_ADV_CTE_TX)
	if (extra_data) {
		ull_adv_sync_extra_data_set_clear(extra_data_prev, extra_data,
						  ULL_ADV_PDU_HDR_FIELD_AD_DATA, 0, NULL);
	}
#endif /* CONFIG_BT_CTLR_DF_ADV_CTE_TX */

	err = ull_adv_sync_pdu_set_clear(lll_sync, pdu_prev, pdu, ULL_ADV_PDU_HDR_FIELD_AD_DATA, 0,
					 hdr_data);
	if (err) {
		return err;
	}

#if defined(CONFIG_BT_CTLR_ADV_PDU_LINK)
	/* alloc() will return the same PDU as peek() in case there was PDU
	 * queued but not switched to current before alloc() - no need to deal
	 * with chain as it's already there. In other case we need to duplicate
	 * chain from current PDU and append it to new PDU.
	 */
	if (pdu != pdu_prev) {
		struct pdu_adv *next, *next_dup;

		LL_ASSERT(lll_adv_pdu_linked_next_get(pdu) == NULL);

		next = lll_adv_pdu_linked_next_get(pdu_prev);
		next_dup = adv_sync_pdu_duplicate_chain(next);

		lll_adv_pdu_linked_append(next_dup, pdu);
	}
#endif /* CONFIG_BT_CTLR_ADV_PDU_LINK */

	sync = HDR_LLL2ULL(lll_sync);
	if (sync->is_started) {
		err = ull_adv_sync_time_update(sync, pdu);
		if (err) {
			return err;
		}
	}

	lll_adv_sync_data_enqueue(lll_sync, ter_idx);

	return err;
}

uint8_t ll_adv_sync_enable(uint8_t handle, uint8_t enable)
{
	struct lll_adv_sync *lll_sync;
	struct ll_adv_sync_set *sync;
	uint8_t sync_got_enabled;
	struct ll_adv_set *adv;
	uint8_t pri_idx;
	uint8_t err;

	adv = ull_adv_is_created_get(handle);
	if (!adv) {
		return BT_HCI_ERR_UNKNOWN_ADV_IDENTIFIER;
	}

	lll_sync = adv->lll.sync;
	if (!lll_sync) {
		return BT_HCI_ERR_UNKNOWN_ADV_IDENTIFIER;
	}

	sync = HDR_LLL2ULL(lll_sync);

	if (!enable) {
		if (!sync->is_enabled) {
			return BT_HCI_ERR_CMD_DISALLOWED;
		}

		if (!sync->is_started) {
			sync->is_enabled = 0U;

			return 0;
		}

		err = sync_remove(sync, adv, 0U);
		return err;
	}

	/* TODO: Check for periodic data being complete */

	/* TODO: Check packet too long */

	sync_got_enabled = 0U;
	if (sync->is_enabled) {
		/* TODO: Enabling an already enabled advertising changes its
		 * random address.
		 */
	} else {
		sync_got_enabled = 1U;
	}

	if (adv->is_enabled && !sync->is_started) {
		struct pdu_adv_sync_info *sync_info;
		uint8_t value[1 + sizeof(sync_info)];
		uint32_t ticks_slot_overhead_aux;
		struct lll_adv_aux *lll_aux;
		struct ll_adv_aux_set *aux;
		uint32_t ticks_anchor_sync;
		uint32_t ticks_anchor_aux;
		uint32_t ret;

		lll_aux = adv->lll.aux;

		/* Add sync_info into auxiliary PDU */
		err = ull_adv_aux_hdr_set_clear(adv,
						ULL_ADV_PDU_HDR_FIELD_SYNC_INFO,
						0, value, NULL, &pri_idx);
		if (err) {
			return err;
		}

		/* First byte in the length-value encoded parameter is size of
		 * sync_info structure, followed by pointer to sync_info in the
		 * PDU.
		 */
		memcpy(&sync_info, &value[1], sizeof(sync_info));
		ull_adv_sync_info_fill(sync, sync_info);

		if (lll_aux) {
			/* FIXME: Find absolute ticks until after auxiliary PDU
			 *        on air to place the periodic advertising PDU.
			 */
			ticks_anchor_aux = 0U; /* unused in this path */
			ticks_slot_overhead_aux = 0U; /* unused in this path */
			ticks_anchor_sync = ticker_ticks_now_get();
			aux = NULL;
		} else {
			lll_aux = adv->lll.aux;
			aux = HDR_LLL2ULL(lll_aux);
			ticks_anchor_aux = ticker_ticks_now_get();
			ticks_slot_overhead_aux = ull_adv_aux_evt_init(aux);
			ticks_anchor_sync =
				ticks_anchor_aux + ticks_slot_overhead_aux +
				aux->ull.ticks_slot +
				HAL_TICKER_US_TO_TICKS(EVENT_MAFS_US);
		}

		ret = ull_adv_sync_start(adv, sync, ticks_anchor_sync);
		if (ret) {
			sync_remove(sync, adv, 1U);

			return BT_HCI_ERR_INSUFFICIENT_RESOURCES;
		}

		sync->is_started = 1U;

		lll_adv_data_enqueue(&adv->lll, pri_idx);

		if (aux) {
			/* Keep aux interval equal or higher than primary PDU
			 * interval.
			 */
			aux->interval = adv->interval +
					(HAL_TICKER_TICKS_TO_US(
						ULL_ADV_RANDOM_DELAY) /
						ADV_INT_UNIT_US);

			ret = ull_adv_aux_start(aux, ticks_anchor_aux,
						ticks_slot_overhead_aux);
			if (ret) {
				sync_remove(sync, adv, 1U);

				return BT_HCI_ERR_INSUFFICIENT_RESOURCES;
			}

			aux->is_started = 1U;
		}
	}

	if (sync_got_enabled) {
		sync->is_enabled = sync_got_enabled;
	}

	return 0;
}

int ull_adv_sync_init(void)
{
	int err;

	err = init_reset();
	if (err) {
		return err;
	}

	return 0;
}

int ull_adv_sync_reset(void)
{
	int err;

	err = init_reset();
	if (err) {
		return err;
	}

	return 0;
}

uint16_t ull_adv_sync_lll_handle_get(struct lll_adv_sync *lll)
{
	return sync_handle_get((void *)lll->hdr.parent);
}

void ull_adv_sync_release(struct ll_adv_sync_set *sync)
{
	lll_adv_sync_data_release(&sync->lll);
	sync_release(sync);
}

uint32_t ull_adv_sync_start(struct ll_adv_set *adv,
			    struct ll_adv_sync_set *sync,
			    uint32_t ticks_anchor)
{
	struct lll_adv_sync *lll_sync;
	uint32_t ticks_slot_overhead;
	uint32_t volatile ret_cb;
	struct pdu_adv *ter_pdu;
	uint32_t interval_us;
	uint8_t sync_handle;
	uint32_t time_us;
	uint32_t ret;

	ull_hdr_init(&sync->ull);

	lll_sync = &sync->lll;
	ter_pdu = lll_adv_sync_data_peek(lll_sync, NULL);

	/* Calculate the PDU Tx Time and hence the radio event length */
	time_us = sync_time_get(sync, ter_pdu);

	/* TODO: active_to_start feature port */
	sync->ull.ticks_active_to_start = 0;
	sync->ull.ticks_prepare_to_start =
		HAL_TICKER_US_TO_TICKS(EVENT_OVERHEAD_XTAL_US);
	sync->ull.ticks_preempt_to_start =
		HAL_TICKER_US_TO_TICKS(EVENT_OVERHEAD_PREEMPT_MIN_US);
	sync->ull.ticks_slot = HAL_TICKER_US_TO_TICKS(time_us);

	if (IS_ENABLED(CONFIG_BT_CTLR_LOW_LAT)) {
		ticks_slot_overhead = MAX(sync->ull.ticks_active_to_start,
					  sync->ull.ticks_prepare_to_start);
	} else {
		ticks_slot_overhead = 0;
	}

	interval_us = (uint32_t)sync->interval * CONN_INT_UNIT_US;

	sync_handle = sync_handle_get(sync);

	ret_cb = TICKER_STATUS_BUSY;
	ret = ticker_start(TICKER_INSTANCE_ID_CTLR, TICKER_USER_ID_THREAD,
			   (TICKER_ID_ADV_SYNC_BASE + sync_handle),
			   ticks_anchor, 0,
			   HAL_TICKER_US_TO_TICKS(interval_us),
			   HAL_TICKER_REMAINDER(interval_us), TICKER_NULL_LAZY,
			   (sync->ull.ticks_slot + ticks_slot_overhead),
			   ticker_cb, sync,
			   ull_ticker_status_give, (void *)&ret_cb);
	ret = ull_ticker_status_take(ret, &ret_cb);

	return ret;
}

uint8_t ull_adv_sync_time_update(struct ll_adv_sync_set *sync,
				 struct pdu_adv *pdu)
{
	uint32_t volatile ret_cb;
	uint32_t ticks_minus;
	uint32_t ticks_plus;
	uint32_t time_ticks;
	uint16_t time_us;
	uint32_t ret;

	time_us = sync_time_get(sync, pdu);
	time_ticks = HAL_TICKER_US_TO_TICKS(time_us);
	if (sync->ull.ticks_slot > time_ticks) {
		ticks_minus = sync->ull.ticks_slot - time_ticks;
		ticks_plus = 0U;
	} else if (sync->ull.ticks_slot < time_ticks) {
		ticks_minus = 0U;
		ticks_plus = time_ticks - sync->ull.ticks_slot;
	} else {
		return BT_HCI_ERR_SUCCESS;
	}

	ret_cb = TICKER_STATUS_BUSY;
	ret = ticker_update(TICKER_INSTANCE_ID_CTLR,
			    TICKER_USER_ID_THREAD,
			    (TICKER_ID_ADV_SYNC_BASE + sync_handle_get(sync)),
			    0, 0, ticks_plus, ticks_minus, 0, 0,
			    ull_ticker_status_give, (void *)&ret_cb);
	ret = ull_ticker_status_take(ret, &ret_cb);
	if (ret != TICKER_STATUS_SUCCESS) {
		return BT_HCI_ERR_CMD_DISALLOWED;
	}

	sync->ull.ticks_slot = time_ticks;

	return BT_HCI_ERR_SUCCESS;
}

void ull_adv_sync_info_fill(struct ll_adv_sync_set *sync,
			    struct pdu_adv_sync_info *si)
{
	struct lll_adv_sync *lll_sync;

	/* NOTE: sync offset and offset unit filled by secondary prepare.
	 *
	 * If sync_info is part of ADV PDU the offs_adjust field
	 * is always set to 0.
	 */
	si->offs_units = 0U;
	si->offs_adjust = 0U;
	si->offs = 0U;

	/* Fill the interval, channel map, access address and CRC init */
	si->interval = sys_cpu_to_le16(sync->interval);
	lll_sync = &sync->lll;
	memcpy(si->sca_chm, lll_sync->data_chan_map,
	       sizeof(si->sca_chm));
	si->sca_chm[PDU_SYNC_INFO_SCA_CHM_SCA_BYTE_OFFSET] &=
		~PDU_SYNC_INFO_SCA_CHM_SCA_BIT_MASK;
	si->sca_chm[PDU_SYNC_INFO_SCA_CHM_SCA_BYTE_OFFSET] |=
		((lll_clock_sca_local_get() <<
		  PDU_SYNC_INFO_SCA_CHM_SCA_BIT_POS) &
		 PDU_SYNC_INFO_SCA_CHM_SCA_BIT_MASK);
	memcpy(&si->aa, lll_sync->access_addr, sizeof(si->aa));
	memcpy(si->crc_init, lll_sync->crc_init, sizeof(si->crc_init));

	/* NOTE: Filled by secondary prepare */
	si->evt_cntr = 0U;
}

void ull_adv_sync_offset_get(struct ll_adv_set *adv)
{
	static memq_link_t link;
	static struct mayfly mfy = {0, 0, &link, NULL, mfy_sync_offset_get};
	uint32_t ret;

	mfy.param = adv;
	ret = mayfly_enqueue(TICKER_USER_ID_ULL_HIGH, TICKER_USER_ID_ULL_LOW, 1,
			     &mfy);
	LL_ASSERT(!ret);
}

uint8_t ull_adv_sync_pdu_alloc(struct ll_adv_set *adv,
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

	lll_sync = adv->lll.sync;
	if (!lll_sync) {
		return BT_HCI_ERR_UNKNOWN_ADV_IDENTIFIER;
	}

	/* Get reference to previous periodic advertising PDU data */
	pdu_prev = lll_adv_sync_data_peek(lll_sync, &ed_prev);

#if defined(CONFIG_BT_CTLR_DF_ADV_CTE_TX)
	/* Get reference to new periodic advertising PDU data buffer */
	if (extra_data_flag == ULL_ADV_PDU_EXTRA_DATA_ALLOC_ALWAYS ||
	    (extra_data_flag == ULL_ADV_PDU_EXTRA_DATA_ALLOC_IF_EXIST && ed_prev)) {
		/* If there was an extra data in past PDU data or it is required
		 * by the hdr_add_fields then allocate memmory for it.
		 */
		pdu_new = lll_adv_sync_data_alloc(lll_sync, &ed_new,
						  ter_idx);
		if (!pdu_new) {
			return BT_HCI_ERR_MEM_CAPACITY_EXCEEDED;
		}
	} else {
		ed_new = NULL;
#else
	{
#endif /* CONFIG_BT_CTLR_DF_ADV_CTE_TX */
		pdu_new = lll_adv_sync_data_alloc(lll_sync, NULL, ter_idx);
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

/* @brief Set or clear fields in extended advertising header and store
 *        extra_data if requested.
 *
 * @param[in]  lll_sync         Reference to periodic advertising sync.
 * @param[in]  ter_pdu_prev     Pointer to previous PDU.
 * @param[in]  ter_pdu_         Pointer to PDU to fill fileds.
 * @param[in]  hdr_add_fields   Flag with information which fields add.
 * @param[in]  hdr_rem_fields   Flag with information which fields remove.
 * @param[in]  hdr_data         Pointer to data to be added to header. Content
 *                              depends on the value of @p hdr_add_fields.
 *
 * @Note
 * @p hdr_data content depends on the flag provided by @p hdr_add_fields:
 * - ULL_ADV_PDU_HDR_FIELD_CTE_INFO:
 *   # @p hdr_data points to single byte with CTEInfo field
 * - ULL_ADV_PDU_HDR_FIELD_AD_DATA:
 *   # @p hdr_data points to memory where first byte
 *     is size of advertising data, following byte is a pointer to actual
 *     advertising data.
 * - ULL_ADV_PDU_HDR_FIELD_AUX_PTR:
 *   # @p hdr_data parameter is not used
 * - ULL_ADV_PDU_HDR_FIELD_ACAD:
 *   # @p hdr_data points to memory where first byte is size of ACAD, second
 *     byte is used to return offset to ACAD field.
 *
 * @return Zero in case of success, other value in case of failure.
 */
uint8_t ull_adv_sync_pdu_set_clear(struct lll_adv_sync *lll_sync,
				   struct pdu_adv *ter_pdu_prev,
				   struct pdu_adv *ter_pdu,
				   uint16_t hdr_add_fields,
				   uint16_t hdr_rem_fields,
				   void *hdr_data)
{
	struct pdu_adv_com_ext_adv *ter_com_hdr, *ter_com_hdr_prev;
	struct pdu_adv_ext_hdr *ter_hdr, ter_hdr_prev;
	uint8_t *ter_dptr, *ter_dptr_prev;
	uint8_t acad_len_prev;
	uint8_t ter_len_prev;
	uint8_t hdr_buf_len;
	uint16_t ter_len;
	uint8_t *ad_data;
	uint8_t acad_len;
#if defined(CONFIG_BT_CTLR_DF_ADV_CTE_TX)
	uint8_t cte_info;
#endif /* CONFIG_BT_CTLR_DF_ADV_CTE_TX */
	uint8_t ad_len;

	/* Get common pointers from reference to previous tertiary PDU data */
	ter_com_hdr_prev = (void *)&ter_pdu_prev->adv_ext_ind;
	ter_hdr = (void *)ter_com_hdr_prev->ext_hdr_adv_data;
	if (ter_com_hdr_prev->ext_hdr_len) {
		ter_hdr_prev = *ter_hdr;
	} else {
		*(uint8_t *)&ter_hdr_prev = 0U;
	}
	ter_dptr_prev = ter_hdr->data;

	/* Set common fields in reference to new tertiary PDU data buffer */
	ter_pdu->type = ter_pdu_prev->type;
	ter_pdu->rfu = 0U;
	ter_pdu->chan_sel = 0U;

	ter_pdu->tx_addr = ter_pdu_prev->tx_addr;
	ter_pdu->rx_addr = ter_pdu_prev->rx_addr;

	ter_com_hdr = (void *)&ter_pdu->adv_ext_ind;
	ter_com_hdr->adv_mode = ter_com_hdr_prev->adv_mode;
	ter_hdr = (void *)ter_com_hdr->ext_hdr_adv_data;
	ter_dptr = ter_hdr->data;
	*(uint8_t *)ter_hdr = 0U;

	/* No AdvA in AUX_SYNC_IND */
	/* No TargetA in AUX_SYNC_IND */

#if defined(CONFIG_BT_CTLR_DF_ADV_CTE_TX)
	/* If requested add or update CTEInfo */
	if (hdr_add_fields & ULL_ADV_PDU_HDR_FIELD_CTE_INFO) {
		ter_hdr->cte_info = 1;
		cte_info = *(uint8_t *)hdr_data;
		hdr_data = (uint8_t *)hdr_data + 1;
		ter_dptr += sizeof(struct pdu_cte_info);
	/* If CTEInfo exists in prev and is not requested to be removed */
	} else if (!(hdr_rem_fields & ULL_ADV_PDU_HDR_FIELD_CTE_INFO) &&
		   ter_hdr_prev.cte_info) {
		ter_hdr->cte_info = 1;
		ter_dptr += sizeof(struct pdu_cte_info);
	}

	/* If CTEInfo exists in prev PDU */
	if (ter_hdr_prev.cte_info) {
		ter_dptr_prev += sizeof(struct pdu_cte_info);
	}
#endif /* CONFIG_BT_CTLR_DF_ADV_CTE_TX */

	/* No ADI in AUX_SYNC_IND */

	/* AuxPtr - will be added if AUX_CHAIN_IND is required */
	if ((hdr_add_fields & ULL_ADV_PDU_HDR_FIELD_AUX_PTR) ||
	    (!(hdr_rem_fields & ULL_ADV_PDU_HDR_FIELD_AUX_PTR) &&
	     ter_hdr_prev.aux_ptr)) {
		ter_hdr->aux_ptr = 1;
	}
	if (ter_hdr->aux_ptr) {
		ter_dptr += sizeof(struct pdu_adv_aux_ptr);
	}
	if (ter_hdr_prev.aux_ptr) {
		ter_dptr_prev += sizeof(struct pdu_adv_aux_ptr);
	}

	/* No SyncInfo in AUX_SYNC_IND */

	/* Tx Power flag */
	if (ter_hdr_prev.tx_pwr) {
		ter_dptr_prev++;

		ter_hdr->tx_pwr = 1;
		ter_dptr++;
	}

	/* Calc previous ACAD len and update PDU len */
	ter_len_prev = ter_dptr_prev - (uint8_t *)ter_com_hdr_prev;
	hdr_buf_len = ter_com_hdr_prev->ext_hdr_len +
		      PDU_AC_EXT_HEADER_SIZE_MIN;
	if (ter_len_prev <= hdr_buf_len) {
		acad_len_prev = hdr_buf_len - ter_len_prev;
		ter_len_prev += acad_len_prev;
		ter_dptr_prev += acad_len_prev;
	} else {
		acad_len_prev = 0;
		/* NOTE: If no flags are set then extended header length will be
		 *       zero. Under this condition the current ter_len_prev
		 *       value will be greater than extended header length,
		 *       hence set ter_len_prev to size of the length/mode
		 *       field.
		 */
		ter_len_prev = PDU_AC_EXT_HEADER_SIZE_MIN;
		ter_dptr_prev = (uint8_t *)ter_com_hdr_prev + ter_len_prev;
	}

	/* Did we parse beyond PDU length? */
	if (ter_len_prev > ter_pdu_prev->len) {
		/* we should not encounter invalid length */
		return BT_HCI_ERR_UNSPECIFIED;
	}

	/* Add/Retain/Remove ACAD */
	if (hdr_add_fields & ULL_ADV_PDU_HDR_FIELD_ACAD) {
		acad_len = *(uint8_t *)hdr_data;
		hdr_data = (uint8_t *)hdr_data + 1;
		/* return the pointer to ACAD offset */
		memcpy(hdr_data, &ter_dptr, sizeof(ter_dptr));
		hdr_data = (uint8_t *)hdr_data + sizeof(ter_dptr);
		ter_dptr += acad_len;
	} else if (!(hdr_rem_fields & ULL_ADV_PDU_HDR_FIELD_ACAD)) {
		acad_len = acad_len_prev;
		ter_dptr += acad_len_prev;
	} else {
		acad_len = 0U;
	}

	/* Calc current tertiary PDU len */
	ter_len = ull_adv_aux_hdr_len_calc(ter_com_hdr, &ter_dptr);
	ull_adv_aux_hdr_len_fill(ter_com_hdr, ter_len);

	/* Get Adv data from function parameters */
	if (hdr_add_fields & ULL_ADV_PDU_HDR_FIELD_AD_DATA) {
		ad_data = hdr_data;
		ad_len = *ad_data;
		++ad_data;

		ad_data = (void *)sys_get_le32(ad_data);
	} else if (!(hdr_rem_fields & ULL_ADV_PDU_HDR_FIELD_AD_DATA)) {
		ad_len = ter_pdu_prev->len - ter_len_prev;
		ad_data = ter_dptr_prev;
	} else {
		ad_len = 0;
		ad_data = NULL;
	}

	/* Add AD len to tertiary PDU length */
	ter_len += ad_len;

	/* Check AdvData overflow */
	if (ter_len > PDU_AC_PAYLOAD_SIZE_MAX) {
		return BT_HCI_ERR_PACKET_TOO_LONG;
	}

	/* set the tertiary PDU len */
	ter_pdu->len = ter_len;

	/* Start filling tertiary PDU payload based on flags from here
	 * ==============================================================
	 */

	/* Fill AdvData in tertiary PDU */
	memmove(ter_dptr, ad_data, ad_len);

	/* Early exit if no flags set */
	if (!ter_com_hdr->ext_hdr_len) {
		return 0;
	}

	/* Retain ACAD in tertiary PDU */
	ter_dptr_prev -= acad_len_prev;
	if (acad_len) {
		ter_dptr -= acad_len;
		memmove(ter_dptr, ter_dptr_prev, acad_len_prev);
	}

	/* Tx Power */
	if (ter_hdr->tx_pwr) {
		*--ter_dptr = *--ter_dptr_prev;
	}

	/* No SyncInfo in AUX_SYNC_IND */

	/* AuxPtr */
	if (ter_hdr->aux_ptr) {
		/* ToDo Update setup of aux_ptr - check documentation */
		if (ter_hdr_prev.aux_ptr) {
			ter_dptr_prev -= sizeof(struct pdu_adv_aux_ptr);
			ter_dptr -= sizeof(struct pdu_adv_aux_ptr);
			memmove(ter_dptr, ter_dptr_prev,
				sizeof(struct pdu_adv_aux_ptr));
		} else {
			ull_adv_aux_ptr_fill(&ter_dptr, lll_sync->adv->phy_s);
		}
	}

	/* No ADI in AUX_SYNC_IND*/

#if defined(CONFIG_BT_CTLR_DF_ADV_CTE_TX)
	if (ter_hdr->cte_info) {
		if (hdr_add_fields & ULL_ADV_PDU_HDR_FIELD_CTE_INFO) {
			*--ter_dptr = cte_info;
		} else {
			*--ter_dptr = *--ter_dptr_prev;
		}
	}
#endif /* CONFIG_BT_CTLR_DF_ADV_CTE_TX */

	/* No TargetA in AUX_SYNC_IND */
	/* No AdvA in AUX_SYNC_IND */

	return 0;
}

#if defined(CONFIG_BT_CTLR_DF_ADV_CTE_TX)
/* @brief Set or clear fields in extended advertising header and store
 *        extra_data if requested.
 *
 * @param[in]  extra_data_prev  Pointer to previous content of extra_data.
 * @param[in]  hdr_add_fields   Flag with information which fields add.
 * @param[in]  hdr_rem_fields   Flag with information which fields remove.
 * @param[in]  data             Pointer to data to be stored in extra_data.
 *                              Content depends on the data depends on
 *                              @p hdr_add_fields.
 *
 * @Note
 * @p data depends on the flag provided by @p hdr_add_fields.
 * Information about content of value may be found in description of
 * @ref ull_adv_sync_pdu_set_clear.
 *
 * @return Zero in case of success, other value in case of failure.
 */
void ull_adv_sync_extra_data_set_clear(void *extra_data_prev,
				       void *extra_data_new,
				       uint16_t hdr_add_fields,
				       uint16_t hdr_rem_fields,
				       void *data)
{
	/* Currently only CTE enable requires extra_data. Due to that fact
	 * CTE additional data are just copied to extra_data memory.
	 */
	if (hdr_add_fields & ULL_ADV_PDU_HDR_FIELD_CTE_INFO) {
		memcpy(extra_data_new, data, sizeof(struct lll_df_adv_cfg));
	} else if (!(hdr_rem_fields & ULL_ADV_PDU_HDR_FIELD_CTE_INFO) ||
		   extra_data_prev) {
		memmove(extra_data_new, extra_data_prev,
			sizeof(struct lll_df_adv_cfg));
	}
}
#endif /* CONFIG_BT_CTLR_DF_ADV_CTE_TX */

static int init_reset(void)
{
	/* Initialize adv sync pool. */
	mem_init(ll_adv_sync_pool, sizeof(struct ll_adv_sync_set),
		 sizeof(ll_adv_sync_pool) / sizeof(struct ll_adv_sync_set),
		 &adv_sync_free);

	return 0;
}

static inline struct ll_adv_sync_set *sync_acquire(void)
{
	return mem_acquire(&adv_sync_free);
}

static inline void sync_release(struct ll_adv_sync_set *sync)
{
	mem_release(sync, &adv_sync_free);
}

static inline uint16_t sync_handle_get(struct ll_adv_sync_set *sync)
{
	return mem_index_get(sync, ll_adv_sync_pool,
			     sizeof(struct ll_adv_sync_set));
}

static uint8_t sync_stop(struct ll_adv_sync_set *sync)
{
	uint8_t sync_handle;
	int err;

	sync_handle = sync_handle_get(sync);

	err = ull_ticker_stop_with_mark(TICKER_ID_ADV_SYNC_BASE + sync_handle,
					sync, &sync->lll);
	LL_ASSERT(err == 0 || err == -EALREADY);
	if (err) {
		return BT_HCI_ERR_CMD_DISALLOWED;
	}

	return 0;
}

static inline uint8_t sync_remove(struct ll_adv_sync_set *sync,
				  struct ll_adv_set *adv, uint8_t enable)
{
	uint8_t pri_idx;
	uint8_t err;

	/* Remove sync_info from auxiliary PDU */
	err = ull_adv_aux_hdr_set_clear(adv, 0,
					ULL_ADV_PDU_HDR_FIELD_SYNC_INFO,
					NULL, NULL, &pri_idx);
	if (err) {
		return err;
	}

	lll_adv_data_enqueue(&adv->lll, pri_idx);

	if (sync->is_started) {
		/* TODO: we removed sync info, but if sync_stop() fails, what do
		 * we do?
		 */
		err = sync_stop(sync);
		if (err) {
			return err;
		}

		sync->is_started = 0U;
	}

	if (!enable) {
		sync->is_enabled = 0U;
	}

	return 0U;
}

static uint16_t sync_time_get(struct ll_adv_sync_set *sync,
			      struct pdu_adv *pdu)
{
	struct lll_adv_sync *lll_sync;
	struct lll_adv *lll;
	uint32_t time_us;

	lll_sync = &sync->lll;
	lll = lll_sync->adv; /* or use &adv->lll, of adv parameter passed */
	time_us = PKT_AC_US(pdu->len, lll->phy_s) + EVENT_OVERHEAD_START_US +
		  EVENT_OVERHEAD_END_US;

#if defined(CONFIG_BT_CTLR_DF_ADV_CTE_TX)
	struct ll_adv_set *adv = HDR_LLL2ULL(lll);
	struct lll_df_adv_cfg *df_cfg = adv->df_cfg;

	if (df_cfg && df_cfg->is_enabled) {
		time_us += CTE_LEN_US(df_cfg->cte_length);
	}
#endif /* CONFIG_BT_CTLR_DF_ADV_CTE_TX */

	return time_us;
}

static void mfy_sync_offset_get(void *param)
{
	struct ll_adv_set *adv = param;
	struct lll_adv_sync *lll_sync;
	struct ll_adv_sync_set *sync;
	struct pdu_adv_sync_info *si;
	uint32_t ticks_to_expire;
	uint32_t ticks_current;
	struct pdu_adv *pdu;
	uint8_t ticker_id;
	uint16_t lazy;
	uint8_t retry;
	uint8_t id;

	lll_sync = adv->lll.sync;
	sync = HDR_LLL2ULL(lll_sync);
	ticker_id = TICKER_ID_ADV_SYNC_BASE + sync_handle_get(sync);

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
		ret = ticker_next_slot_get_ext(TICKER_INSTANCE_ID_CTLR,
					       TICKER_USER_ID_ULL_LOW,
					       &id, &ticks_current,
					       &ticks_to_expire, &lazy,
					       NULL, NULL,
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

	/* NOTE: as remainder not used in scheduling primary PDU
	 * packet timer starts transmission after 1 tick hence the +1.
	 */
	lll_sync->ticks_offset = ticks_to_expire + 1;

	pdu = lll_adv_aux_data_curr_get(adv->lll.aux);
	si = sync_info_get(pdu);
	sync_info_offset_fill(si, ticks_to_expire, 0);
	si->evt_cntr = lll_sync->event_counter + lll_sync->latency_prepare +
		       lazy;
}

static inline struct pdu_adv_sync_info *sync_info_get(struct pdu_adv *pdu)
{
	struct pdu_adv_com_ext_adv *p;
	struct pdu_adv_ext_hdr *h;
	uint8_t *ptr;

	p = (void *)&pdu->adv_ext_ind;
	h = (void *)p->ext_hdr_adv_data;
	ptr = h->data;

	if (h->adv_addr) {
		ptr += BDADDR_SIZE;
	}

	if (h->adi) {
		ptr += sizeof(struct pdu_adv_adi);
	}

	if (h->aux_ptr) {
		ptr += sizeof(struct pdu_adv_aux_ptr);
	}

	return (void *)ptr;
}

static inline void sync_info_offset_fill(struct pdu_adv_sync_info *si,
					 uint32_t ticks_offset,
					 uint32_t start_us)
{
	uint32_t offs;

	offs = HAL_TICKER_TICKS_TO_US(ticks_offset) - start_us;
	offs = offs / OFFS_UNIT_30_US;
	if (!!(offs >> 13)) {
		si->offs = offs / (OFFS_UNIT_300_US / OFFS_UNIT_30_US);
		si->offs_units = 1U;
	} else {
		si->offs = offs;
		si->offs_units = 0U;
	}
}

static void ticker_cb(uint32_t ticks_at_expire, uint32_t remainder,
		      uint16_t lazy, uint8_t force, void *param)
{
	static memq_link_t link;
	static struct mayfly mfy = {0, 0, &link, NULL, lll_adv_sync_prepare};
	static struct lll_prepare_param p;
	struct ll_adv_sync_set *sync = param;
	struct lll_adv_sync *lll;
	uint32_t ret;
	uint8_t ref;

	DEBUG_RADIO_PREPARE_A(1);

	lll = &sync->lll;

	/* Increment prepare reference count */
	ref = ull_ref_inc(&sync->ull);
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

	DEBUG_RADIO_PREPARE_A(1);
}

static void ticker_op_cb(uint32_t status, void *param)
{
	*((uint32_t volatile *)param) = status;
}
