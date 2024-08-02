/*
 * Copyright (c) 2017-2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

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
#include "lll/lll_df_types.h"
#include "lll_conn.h"
#include "lll_chan.h"

#include "ull_adv_types.h"

#include "ull_internal.h"
#include "ull_chan_internal.h"
#include "ull_sched_internal.h"
#include "ull_adv_internal.h"

#include "ll.h"

#include "hal/debug.h"

#if defined(CONFIG_BT_CTLR_ADV_SYNC_PDU_LINK)
/* First PDU contains up to PDU_AC_EXT_AD_DATA_LEN_MAX, next ones PDU_AC_EXT_PAYLOAD_SIZE_MAX */
#define PAYLOAD_BASED_FRAG_COUNT \
	1U + DIV_ROUND_UP(MAX(0U, CONFIG_BT_CTLR_ADV_DATA_LEN_MAX - PDU_AC_EXT_AD_DATA_LEN_MAX), \
			 PDU_AC_EXT_PAYLOAD_SIZE_MAX)
#if defined(CONFIG_BT_CTLR_DF_ADV_CTE_TX)
#define MAX_FRAG_COUNT MAX(PAYLOAD_BASED_FRAG_COUNT, CONFIG_BT_CTLR_DF_PER_ADV_CTE_NUM_MAX)
#else
#define MAX_FRAG_COUNT PAYLOAD_BASED_FRAG_COUNT
#endif /* CONFIG_BT_CTLR_DF_ADV_CTE_TX */
#endif /* CONFIG_BT_CTLR_ADV_SYNC_PDU_LINK */

static int init_reset(void);
static void ull_adv_sync_copy_pdu(const struct pdu_adv *pdu_prev,
				  struct pdu_adv *pdu);
#if defined(CONFIG_BT_CTLR_ADV_SYNC_PDU_LINK)
static uint8_t ull_adv_sync_duplicate_chain(const struct pdu_adv *pdu_prev,
					    struct pdu_adv *pdu);
#endif /* CONFIG_BT_CTLR_ADV_SYNC_PDU_LINK */
static uint8_t ull_adv_sync_ad_add(struct lll_adv_sync *lll_sync,
				   struct pdu_adv *ter_pdu_prev,
				   struct pdu_adv *ter_pdu,
				   const uint8_t *ad, uint8_t ad_len);
static uint8_t ull_adv_sync_ad_replace(struct lll_adv_sync *lll_sync,
				       struct pdu_adv *ter_pdu_prev,
				       struct pdu_adv *ter_pdu,
				       const uint8_t *ad, uint8_t ad_len);
#if defined(CONFIG_BT_CTLR_ADV_PERIODIC_ADI_SUPPORT)
static uint8_t ull_adv_sync_update_adi(struct lll_adv_sync *lll_sync,
				       struct pdu_adv *ter_pdu_prev,
				       struct pdu_adv *ter_pdu);
static uint8_t ull_adv_sync_add_adi(struct lll_adv_sync *lll_sync,
				    struct pdu_adv *pdu_prev,
				    struct pdu_adv *pdu);
static uint8_t ull_adv_sync_remove_adi(struct lll_adv_sync *lll_sync,
				       struct pdu_adv *pdu_prev,
				       struct pdu_adv *pdu);
#endif /* CONFIG_BT_CTLR_ADV_PERIODIC_ADI_SUPPORT */
static uint8_t adv_type_check(struct ll_adv_set *adv);
static inline struct ll_adv_sync_set *sync_acquire(void);
static inline void sync_release(struct ll_adv_sync_set *sync);
static inline uint16_t sync_handle_get(const struct ll_adv_sync_set *sync);
static uint32_t sync_time_get(const struct ll_adv_sync_set *sync,
			      const struct pdu_adv *pdu);
static inline uint8_t sync_remove(struct ll_adv_sync_set *sync,
				  struct ll_adv_set *adv, uint8_t enable);
static uint8_t sync_chm_update(uint8_t handle);

#if defined(CONFIG_BT_TICKER_EXT_EXPIRE_INFO)
static void sync_info_offset_fill(struct pdu_adv_sync_info *si, uint32_t offs);

#else /* !CONFIG_BT_TICKER_EXT_EXPIRE_INFO */
static void mfy_sync_offset_get(void *param);
static void sync_info_offset_fill(struct pdu_adv_sync_info *si,
				  uint32_t ticks_offset,
				  uint32_t remainder_us,
				  uint32_t start_us);
static void ticker_op_cb(uint32_t status, void *param);
#endif /* !CONFIG_BT_TICKER_EXT_EXPIRE_INFO */

static struct pdu_adv_sync_info *sync_info_get(struct pdu_adv *pdu);
static void ticker_cb(uint32_t ticks_at_expire, uint32_t ticks_drift,
		      uint32_t remainder, uint16_t lazy, uint8_t force,
		      void *param);

#if defined(CONFIG_BT_CTLR_ADV_ISO) && defined(CONFIG_BT_TICKER_EXT_EXPIRE_INFO)
static void ticker_update_op_cb(uint32_t status, void *param);

static struct ticker_ext ll_adv_sync_ticker_ext[CONFIG_BT_CTLR_ADV_SYNC_SET];
#endif /* !CONFIG_BT_CTLR_ADV_ISO  && CONFIG_BT_TICKER_EXT_EXPIRE_INFO */

static struct ll_adv_sync_set ll_adv_sync_pool[CONFIG_BT_CTLR_ADV_SYNC_SET];
static void *adv_sync_free;

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

	if (IS_ENABLED(CONFIG_BT_CTLR_PARAM_CHECK)) {
		err = adv_type_check(adv);
		if (err) {
			return err;
		}
	}

	lll_sync = adv->lll.sync;
	if (!lll_sync) {
		struct pdu_adv *ter_pdu;
		struct lll_adv *lll;
		uint8_t chm_last;

		sync = sync_acquire();
		if (!sync) {
			return BT_HCI_ERR_MEM_CAPACITY_EXCEEDED;
		}

		lll = &adv->lll;
		lll_sync = &sync->lll;
		lll->sync = lll_sync;
		lll_sync->adv = lll;

		lll_adv_data_reset(&lll_sync->data);
		err = lll_adv_sync_data_init(&lll_sync->data);
		if (err) {
			return BT_HCI_ERR_MEM_CAPACITY_EXCEEDED;
		}

		/* NOTE: ull_hdr_init(&sync->ull); is done on start */
		lll_hdr_init(lll_sync, sync);

		err = util_aa_le32(lll_sync->access_addr);
		LL_ASSERT(!err);

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

		ter_pdu = lll_adv_sync_data_peek(lll_sync, NULL);
		ull_adv_sync_pdu_init(ter_pdu, 0U, 0U, 0U, NULL);
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
						  0U, 0U, NULL);
	}
#endif /* CONFIG_BT_CTLR_DF_ADV_CTE_TX */

	/* FIXME - handle flags (i.e. adding TxPower if specified) */
	err = ull_adv_sync_duplicate(pdu_prev, pdu);
	if (err) {
		return err;
	}

	lll_adv_sync_data_enqueue(lll_sync, ter_idx);

	sync->is_data_cmplt = 1U;

	return 0;
}

#if defined(CONFIG_BT_CTLR_ADV_ISO) && defined(CONFIG_BT_TICKER_EXT_EXPIRE_INFO)
void ull_adv_sync_iso_created(struct ll_adv_sync_set *sync)
{
	if (sync->lll.iso && sync->is_started) {
		uint8_t iso_handle = sync->lll.iso->handle;
		uint8_t handle = sync_handle_get(sync);

		ticker_update_ext(TICKER_INSTANCE_ID_CTLR, TICKER_USER_ID_THREAD,
			   (TICKER_ID_ADV_SYNC_BASE + handle), 0, 0, 0, 0, 0, 0,
			   ticker_update_op_cb, sync, 0, TICKER_ID_ADV_ISO_BASE + iso_handle);
	}
}
#endif /* CONFIG_BT_CTLR_ADV_ISO && CONFIG_BT_TICKER_EXT_EXPIRE_INFO */

uint8_t ll_adv_sync_ad_data_set(uint8_t handle, uint8_t op, uint8_t len,
				uint8_t const *const data)
{
	void *extra_data_prev, *extra_data;
	struct pdu_adv *pdu_prev, *pdu;
	struct lll_adv_sync *lll_sync;
	struct ll_adv_sync_set *sync;
	struct ll_adv_set *adv;
	uint8_t ter_idx;
	uint8_t err;

	/* Check for valid advertising set */
	adv =  ull_adv_is_created_get(handle);
	if (!adv) {
		return BT_HCI_ERR_UNKNOWN_ADV_IDENTIFIER;
	}

	/* Check for advertising set type */
	if (IS_ENABLED(CONFIG_BT_CTLR_PARAM_CHECK)) {
		err = adv_type_check(adv);
		if (err) {
			return err;
		}
	}

	/* Check if periodic advertising is associated with advertising set */
	lll_sync = adv->lll.sync;
	if (!lll_sync) {
		return BT_HCI_ERR_UNKNOWN_ADV_IDENTIFIER;
	}

	sync = HDR_LLL2ULL(lll_sync);

	/* Reject setting fragment when periodic advertising is enabled */
	if (sync->is_enabled && (op <= BT_HCI_LE_EXT_ADV_OP_LAST_FRAG)) {
		return BT_HCI_ERR_CMD_DISALLOWED;
	}

	/* Reject intermediate op before first op */
	if (sync->is_data_cmplt &&
	    ((op == BT_HCI_LE_EXT_ADV_OP_INTERM_FRAG) ||
	     (op == BT_HCI_LE_EXT_ADV_OP_LAST_FRAG))) {
		return BT_HCI_ERR_CMD_DISALLOWED;
	}

	/* Reject unchanged op before complete status */
	if (!sync->is_data_cmplt &&
	    (op == BT_HCI_LE_EXT_ADV_OP_UNCHANGED_DATA)) {
		return BT_HCI_ERR_CMD_DISALLOWED;
	}

	/* Reject len > 191 bytes if chain PDUs unsupported */
	if (!IS_ENABLED(CONFIG_BT_CTLR_ADV_SYNC_PDU_LINK) &&
	    (len > PDU_AC_EXT_AD_DATA_LEN_MAX)) {
		return BT_HCI_ERR_CMD_DISALLOWED;
	}

	/* Allocate new PDU buffer at latest double buffer index */
	err = ull_adv_sync_pdu_alloc(adv, ULL_ADV_PDU_EXTRA_DATA_ALLOC_IF_EXIST,
				     &pdu_prev, &pdu, &extra_data_prev,
				     &extra_data, &ter_idx);
	if (err) {
		return err;
	}

#if defined(CONFIG_BT_CTLR_DF_ADV_CTE_TX)
	if (extra_data) {
		ull_adv_sync_extra_data_set_clear(extra_data_prev, extra_data,
						  0U, 0U, NULL);
	}
#endif /* CONFIG_BT_CTLR_DF_ADV_CTE_TX */

	if (op == BT_HCI_LE_EXT_ADV_OP_UNCHANGED_DATA) {
		/* Only update ADI */
#if defined(CONFIG_BT_CTLR_ADV_PERIODIC_ADI_SUPPORT)
		err = ull_adv_sync_update_adi(lll_sync, pdu_prev, pdu);
#endif /* CONFIG_BT_CTLR_ADV_PERIODIC_ADI_SUPPORT */
	} else if (op == BT_HCI_LE_EXT_ADV_OP_FIRST_FRAG ||
		   op == BT_HCI_LE_EXT_ADV_OP_COMPLETE_DATA) {
		err = ull_adv_sync_ad_replace(lll_sync, pdu_prev, pdu, data, len);
	} else {
		err = ull_adv_sync_ad_add(lll_sync, pdu_prev, pdu, data, len);
	}
	if (err) {
		return err;
	}

	/* Parameter validation, if operation is 0x04 (unchanged data)
	 * - periodic advertising is disabled, or
	 * - periodic advertising contains no data, or
	 * - Advertising Data Length is not zero
	 */
	if (IS_ENABLED(CONFIG_BT_CTLR_PARAM_CHECK) &&
	    (op == BT_HCI_LE_EXT_ADV_OP_UNCHANGED_DATA) &&
	    ((!sync->is_enabled) ||
	     (pdu->len == pdu->adv_ext_ind.ext_hdr_len + 1U) ||
	     (len != 0U))) {
		/* NOTE: latest PDU was not consumed by LLL and as
		 * ull_adv_sync_pdu_alloc() has reverted back the double buffer
		 * with the first PDU, and returned the latest PDU as the new
		 * PDU, we need to enqueue back the new PDU which is in fact
		 * the latest PDU.
		 */
		if (pdu_prev == pdu) {
			lll_adv_sync_data_enqueue(lll_sync, ter_idx);
		}

		return BT_HCI_ERR_INVALID_PARAM;
	}

	/* Update time reservation if Periodic Advertising events are active */
	if (sync->is_started) {
		err = ull_adv_sync_time_update(sync, pdu);
		if (err) {
			return err;
		}
	}

	/* Commit the updated Periodic Advertising Data */
	lll_adv_sync_data_enqueue(lll_sync, ter_idx);

	/* Check if Periodic Advertising Data is complete */
	sync->is_data_cmplt = (op >= BT_HCI_LE_EXT_ADV_OP_LAST_FRAG);

	return 0;
}

uint8_t ll_adv_sync_enable(uint8_t handle, uint8_t enable)
{
	struct pdu_adv *ter_pdu = NULL;
	struct lll_adv_sync *lll_sync;
	struct ll_adv_sync_set *sync;
	uint8_t sync_got_enabled;
	struct ll_adv_set *adv;
	uint8_t ter_idx;
	uint8_t err;

	/* Check for valid advertising set */
	adv = ull_adv_is_created_get(handle);
	if (!adv) {
		return BT_HCI_ERR_UNKNOWN_ADV_IDENTIFIER;
	}

	/* Check if periodic advertising is associated with advertising set */
	lll_sync = adv->lll.sync;
	if (!lll_sync) {
		return BT_HCI_ERR_CMD_DISALLOWED;
	}

	/* Check for invalid enable bit fields */
	if ((enable > (BT_HCI_LE_SET_PER_ADV_ENABLE_ENABLE |
		       BT_HCI_LE_SET_PER_ADV_ENABLE_ADI)) ||
	    (!IS_ENABLED(CONFIG_BT_CTLR_ADV_PERIODIC_ADI_SUPPORT) &&
	     (enable > BT_HCI_LE_SET_PER_ADV_ENABLE_ENABLE))) {
		return BT_HCI_ERR_UNSUPP_FEATURE_PARAM_VAL;
	}

	sync = HDR_LLL2ULL(lll_sync);

	/* Handle periodic advertising being disable */
	if (!(enable & BT_HCI_LE_SET_PER_ADV_ENABLE_ENABLE)) {
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

	/* Check for advertising set type */
	if (IS_ENABLED(CONFIG_BT_CTLR_PARAM_CHECK)) {
		err = adv_type_check(adv);
		if (err) {
			return BT_HCI_ERR_CMD_DISALLOWED;
		}
	}

	/* Check for periodic data being complete */
	if (!sync->is_data_cmplt) {
		return BT_HCI_ERR_CMD_DISALLOWED;
	}

	/* TODO: Check packet too long */

	/* Check for already enabled periodic advertising set */
	sync_got_enabled = 0U;
	if (sync->is_enabled) {
		/* TODO: Enabling an already enabled advertising changes its
		 * random address.
		 */
	} else {
		sync_got_enabled = 1U;
	}

#if defined(CONFIG_BT_CTLR_ADV_PERIODIC_ADI_SUPPORT)
	/* Add/Remove ADI */
	{
		void *extra_data_prev, *extra_data;
		struct pdu_adv *pdu_prev, *pdu;

		err = ull_adv_sync_pdu_alloc(adv, ULL_ADV_PDU_EXTRA_DATA_ALLOC_IF_EXIST,
					     &pdu_prev, &pdu, &extra_data_prev,
					     &extra_data, &ter_idx);
		if (err) {
			return err;
		}

		/* Use PDU to calculate time reservation */
		ter_pdu = pdu;

#if defined(CONFIG_BT_CTLR_DF_ADV_CTE_TX)
		if (extra_data) {
			ull_adv_sync_extra_data_set_clear(extra_data_prev,
							  extra_data, 0U, 0U,
							  NULL);
		}
#endif /* CONFIG_BT_CTLR_DF_ADV_CTE_TX */

		if (enable & BT_HCI_LE_SET_PER_ADV_ENABLE_ADI) {
			ull_adv_sync_add_adi(lll_sync, pdu_prev, pdu);
		} else {
			ull_adv_sync_remove_adi(lll_sync, pdu_prev, pdu);
		}
	}
#endif /* CONFIG_BT_CTLR_ADV_PERIODIC_ADI_SUPPORT */

	/* Start Periodic Advertising events if Extended Advertising events are
	 * active.
	 */
	if (adv->is_enabled && !sync->is_started) {
		struct pdu_adv_sync_info *sync_info;
		uint8_t value[1 + sizeof(sync_info)];
		uint32_t ticks_slot_overhead_aux;
		uint32_t ticks_slot_overhead;
		struct lll_adv_aux *lll_aux;
		struct ll_adv_aux_set *aux;
		uint32_t ticks_anchor_sync;
		uint32_t ticks_anchor_aux;
		uint8_t pri_idx, sec_idx;
		uint32_t ret;

		lll_aux = adv->lll.aux;

		/* Add sync_info into auxiliary PDU */
		err = ull_adv_aux_hdr_set_clear(adv,
						ULL_ADV_PDU_HDR_FIELD_SYNC_INFO,
						0U, value, &pri_idx, &sec_idx);
		if (err) {
			return err;
		}

		/* First byte in the length-value encoded parameter is size of
		 * sync_info structure, followed by pointer to sync_info in the
		 * PDU.
		 */
		(void)memcpy(&sync_info, &value[1], sizeof(sync_info));
		ull_adv_sync_info_fill(sync, sync_info);

		/* Calculate the ticks_slot and return slot overhead */
		ticks_slot_overhead = ull_adv_sync_evt_init(adv, sync, ter_pdu);

		/* If Auxiliary PDU already active, find and schedule Periodic
		 * advertising follow it.
		 */
		if (lll_aux) {
			/* Auxiliary set already active (due to other fields
			 * being already present or being started prior).
			 */
			aux = NULL;
			ticks_anchor_aux = 0U; /* unused in this path */
			ticks_slot_overhead_aux = 0U; /* unused in this path */

			/* Find the anchor after the group of active auxiliary
			 * sets such that Periodic Advertising events are placed
			 * in non-overlapping timeline when auxiliary and
			 * Periodic Advertising have similar event interval.
			 */
			ticks_anchor_sync = ticker_ticks_now_get() +
				HAL_TICKER_US_TO_TICKS(EVENT_OVERHEAD_START_US);

#if defined(CONFIG_BT_CTLR_SCHED_ADVANCED)
			err = ull_sched_adv_aux_sync_free_anchor_get(sync->ull.ticks_slot,
								     &ticks_anchor_sync);
			if (!err) {
				ticks_anchor_sync += HAL_TICKER_US_TO_TICKS(
					MAX(EVENT_MAFS_US,
					    EVENT_OVERHEAD_START_US) -
					EVENT_OVERHEAD_START_US +
					(EVENT_TICKER_RES_MARGIN_US << 1));
			}
#endif /* CONFIG_BT_CTLR_SCHED_ADVANCED */

		} else {
			/* Auxiliary set will be started due to inclusion of
			 * sync info field.
			 */
			lll_aux = adv->lll.aux;
			aux = HDR_LLL2ULL(lll_aux);
			ticks_anchor_aux = ticker_ticks_now_get() +
				HAL_TICKER_US_TO_TICKS(EVENT_OVERHEAD_START_US);
			ticks_slot_overhead_aux =
				ull_adv_aux_evt_init(aux, &ticks_anchor_aux);

#if !defined(CONFIG_BT_CTLR_ADV_AUX_SYNC_OFFSET) || \
	(CONFIG_BT_CTLR_ADV_AUX_SYNC_OFFSET == 0)
			ticks_anchor_sync = ticks_anchor_aux +
				ticks_slot_overhead_aux + aux->ull.ticks_slot +
				HAL_TICKER_US_TO_TICKS(
					MAX(EVENT_MAFS_US,
					    EVENT_OVERHEAD_START_US) -
					EVENT_OVERHEAD_START_US +
					(EVENT_TICKER_RES_MARGIN_US << 1));

#else /* CONFIG_BT_CTLR_ADV_AUX_SYNC_OFFSET */
			ticks_anchor_sync = ticks_anchor_aux +
				HAL_TICKER_US_TO_TICKS(
					CONFIG_BT_CTLR_ADV_AUX_SYNC_OFFSET);

#endif /* CONFIG_BT_CTLR_ADV_AUX_SYNC_OFFSET */
		}

		ret = ull_adv_sync_start(adv, sync, ticks_anchor_sync,
					 ticks_slot_overhead);
		if (ret) {
			sync_remove(sync, adv, 1U);

			return BT_HCI_ERR_INSUFFICIENT_RESOURCES;
		}

		sync->is_started = 1U;

		lll_adv_aux_data_enqueue(lll_aux, sec_idx);
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

		} else if (IS_ENABLED(CONFIG_BT_TICKER_EXT_EXPIRE_INFO)) {
			/* notify the auxiliary set */
			ull_adv_sync_started_stopped(HDR_LLL2ULL(lll_aux));
		}
	}

	/* Commit the Periodic Advertising data if ADI supported and has been
	 * updated.
	 */
	if (IS_ENABLED(CONFIG_BT_CTLR_ADV_PERIODIC_ADI_SUPPORT)) {
		lll_adv_sync_data_enqueue(lll_sync, ter_idx);
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
	struct lll_adv_sync *lll_sync;
	struct ll_adv_sync_set *sync;
	struct ll_adv_set *adv;
	uint8_t handle;
	int err;

	for (handle = 0U; handle < BT_CTLR_ADV_SET; handle++) {
		adv = ull_adv_is_created_get(handle);
		if (!adv) {
			continue;
		}

		lll_sync = adv->lll.sync;
		if (!lll_sync) {
			continue;
		}

		sync = HDR_LLL2ULL(lll_sync);

		if (!sync->is_started) {
			sync->is_enabled = 0U;

			continue;
		}

		err = sync_remove(sync, adv, 0U);
		if (err) {
			return err;
		}
	}

	return 0;
}

int ull_adv_sync_reset_finalize(void)
{
	int err;

	err = init_reset();
	if (err) {
		return err;
	}

	return 0;
}

struct ll_adv_sync_set *ull_adv_sync_get(uint8_t handle)
{
	if (handle >= CONFIG_BT_CTLR_ADV_SYNC_SET) {
		return NULL;
	}

	return &ll_adv_sync_pool[handle];
}

uint16_t ull_adv_sync_handle_get(const struct ll_adv_sync_set *sync)
{
	return sync_handle_get(sync);
}

uint16_t ull_adv_sync_lll_handle_get(const struct lll_adv_sync *lll)
{
	return sync_handle_get((void *)lll->hdr.parent);
}

void ull_adv_sync_release(struct ll_adv_sync_set *sync)
{
	lll_adv_sync_data_release(&sync->lll);
	sync_release(sync);
}

uint32_t ull_adv_sync_time_get(const struct ll_adv_sync_set *sync,
			       uint8_t pdu_len)
{
	const struct lll_adv_sync *lll_sync = &sync->lll;
	const struct lll_adv *lll = lll_sync->adv;
	uint32_t time_us;

	/* NOTE: 16-bit values are sufficient for minimum radio event time
	 *       reservation, 32-bit are used here so that reservations for
	 *       whole back-to-back chaining of PDUs can be accommodated where
	 *       the required microseconds could overflow 16-bits, example,
	 *       back-to-back chained Coded PHY PDUs.
	 */

	time_us = PDU_AC_US(pdu_len, lll->phy_s, lll->phy_flags) +
		  EVENT_OVERHEAD_START_US + EVENT_OVERHEAD_END_US;

#if defined(CONFIG_BT_CTLR_DF_ADV_CTE_TX)
	struct ll_adv_set *adv = HDR_LLL2ULL(lll);
	struct lll_df_adv_cfg *df_cfg = adv->df_cfg;

	if (df_cfg && df_cfg->is_enabled) {
		time_us += CTE_LEN_US(df_cfg->cte_length);
	}
#endif /* CONFIG_BT_CTLR_DF_ADV_CTE_TX */

	return time_us;
}

uint32_t ull_adv_sync_evt_init(struct ll_adv_set *adv,
			       struct ll_adv_sync_set *sync,
			       struct pdu_adv *pdu)
{
	uint32_t ticks_slot_overhead;
	uint32_t ticks_slot_offset;
	uint32_t time_us;

	ull_hdr_init(&sync->ull);

	if (!pdu) {
		pdu = lll_adv_sync_data_peek(&sync->lll, NULL);
	}

	time_us = sync_time_get(sync, pdu);

	/* TODO: active_to_start feature port */
	sync->ull.ticks_active_to_start = 0U;
	sync->ull.ticks_prepare_to_start =
		HAL_TICKER_US_TO_TICKS(EVENT_OVERHEAD_XTAL_US);
	sync->ull.ticks_preempt_to_start =
		HAL_TICKER_US_TO_TICKS(EVENT_OVERHEAD_PREEMPT_MIN_US);
	sync->ull.ticks_slot = HAL_TICKER_US_TO_TICKS_CEIL(time_us);

	ticks_slot_offset = MAX(sync->ull.ticks_active_to_start,
				sync->ull.ticks_prepare_to_start);
	if (IS_ENABLED(CONFIG_BT_CTLR_LOW_LAT)) {
		ticks_slot_overhead = ticks_slot_offset;
	} else {
		ticks_slot_overhead = 0U;
	}

	return ticks_slot_overhead;
}

uint32_t ull_adv_sync_start(struct ll_adv_set *adv,
			    struct ll_adv_sync_set *sync,
			    uint32_t ticks_anchor,
			    uint32_t ticks_slot_overhead)
{
	uint32_t volatile ret_cb;
	uint32_t interval_us;
	uint8_t sync_handle;
	uint32_t ret;

	interval_us = (uint32_t)sync->interval * PERIODIC_INT_UNIT_US;

	sync_handle = sync_handle_get(sync);

#if defined(CONFIG_BT_CTLR_ADV_ISO) && \
	defined(CONFIG_BT_TICKER_EXT_EXPIRE_INFO)
	if (sync->lll.iso) {
		ll_adv_sync_ticker_ext[sync_handle].expire_info_id =
			TICKER_ID_ADV_ISO_BASE + sync->lll.iso->handle;
	} else {
		ll_adv_sync_ticker_ext[sync_handle].expire_info_id = TICKER_NULL;
	}

	ll_adv_sync_ticker_ext[sync_handle].ext_timeout_func = ticker_cb;

	ret_cb = TICKER_STATUS_BUSY;
	ret = ticker_start_ext(
#else /* !CONFIG_BT_CTLR_ADV_ISO || !CONFIG_BT_TICKER_EXT_EXPIRE_INFO */

	ret_cb = TICKER_STATUS_BUSY;
	ret = ticker_start(
#endif /* !CONFIG_BT_CTLR_ADV_ISO || !CONFIG_BT_TICKER_EXT_EXPIRE_INFO */
			   TICKER_INSTANCE_ID_CTLR, TICKER_USER_ID_THREAD,
			   (TICKER_ID_ADV_SYNC_BASE + sync_handle),
			   ticks_anchor, 0U,
			   HAL_TICKER_US_TO_TICKS(interval_us),
			   HAL_TICKER_REMAINDER(interval_us), TICKER_NULL_LAZY,
			   (sync->ull.ticks_slot + ticks_slot_overhead),
			   ticker_cb, sync,
			   ull_ticker_status_give, (void *)&ret_cb
#if defined(CONFIG_BT_CTLR_ADV_ISO) && \
	defined(CONFIG_BT_TICKER_EXT_EXPIRE_INFO)
			   ,
			   &ll_adv_sync_ticker_ext[sync_handle]
#endif /* !CONFIG_BT_CTLR_ADV_ISO || !CONFIG_BT_TICKER_EXT_EXPIRE_INFO */
			   );
	ret = ull_ticker_status_take(ret, &ret_cb);

	return ret;
}

uint8_t ull_adv_sync_time_update(struct ll_adv_sync_set *sync,
				 struct pdu_adv *pdu)
{
	uint32_t time_ticks;
	uint32_t time_us;

	time_us = sync_time_get(sync, pdu);
	time_ticks = HAL_TICKER_US_TO_TICKS(time_us);

#if !defined(CONFIG_BT_CTLR_JIT_SCHEDULING)
	uint32_t volatile ret_cb;
	uint32_t ticks_minus;
	uint32_t ticks_plus;
	uint32_t ret;

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
#endif /* !CONFIG_BT_CTLR_JIT_SCHEDULING */

	sync->ull.ticks_slot = time_ticks;

	return BT_HCI_ERR_SUCCESS;
}

uint8_t ull_adv_sync_chm_update(void)
{
	uint8_t handle;

	handle = CONFIG_BT_CTLR_ADV_SYNC_SET;
	while (handle--) {
		(void)sync_chm_update(handle);
	}

	/* TODO: Should failure due to Channel Map Update being already in
	 *       progress be returned to caller?
	 */
	return 0;
}

void ull_adv_sync_chm_complete(struct node_rx_pdu *rx)
{
	struct lll_adv_sync *lll_sync;
	struct pdu_adv *pdu_prev;
	struct ll_adv_set *adv;
	struct pdu_adv *pdu;
	uint8_t ter_idx;
	uint8_t err;

	/* Allocate next Sync PDU */
	pdu_prev = NULL;
	pdu = NULL;
	lll_sync = rx->rx_ftr.param;
	adv = HDR_LLL2ULL(lll_sync->adv);
	err = ull_adv_sync_pdu_alloc(adv, ULL_ADV_PDU_EXTRA_DATA_ALLOC_IF_EXIST,
				     &pdu_prev, &pdu, NULL, NULL, &ter_idx);
	LL_ASSERT(!err);

	err = ull_adv_sync_remove_from_acad(lll_sync, pdu_prev, pdu,
					    PDU_ADV_DATA_TYPE_CHANNEL_MAP_UPDATE_IND);
	LL_ASSERT(!err);

	lll_adv_sync_data_enqueue(lll_sync, ter_idx);
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
	PDU_ADV_SYNC_INFO_OFFS_SET(si, 0U, OFFS_UNIT_VALUE_30_US, 0U);

	/* Fill the interval, access address and CRC init */
	si->interval = sys_cpu_to_le16(sync->interval);
	lll_sync = &sync->lll;
	(void)memcpy(&si->aa, lll_sync->access_addr, sizeof(si->aa));
	(void)memcpy(si->crc_init, lll_sync->crc_init, sizeof(si->crc_init));

	/* NOTE: Filled by secondary prepare */
	si->evt_cntr = 0U;
}

#if !defined(CONFIG_BT_TICKER_EXT_EXPIRE_INFO)
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
#endif /* CONFIG_BT_TICKER_EXT_EXPIRE_INFO */

void ull_adv_sync_pdu_init(struct pdu_adv *pdu, uint8_t ext_hdr_flags,
			   uint8_t phy_s, uint8_t phy_flags,
			   struct pdu_cte_info *cte_info)
{
	struct pdu_adv_com_ext_adv *com_hdr;
	struct pdu_adv_ext_hdr *ext_hdr;
	struct pdu_adv_aux_ptr *aux_ptr;
	uint32_t cte_len_us;
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

	LL_ASSERT(!(ext_hdr_flags & (ULL_ADV_PDU_HDR_FIELD_ADVA | ULL_ADV_PDU_HDR_FIELD_TARGETA |
#if !defined(CONFIG_BT_CTLR_ADV_PERIODIC_ADI_SUPPORT)
				     ULL_ADV_PDU_HDR_FIELD_ADI |
#endif /* CONFIG_BT_CTLR_ADV_PERIODIC_ADI_SUPPORT */
				     ULL_ADV_PDU_HDR_FIELD_SYNC_INFO)));

	if (IS_ENABLED(CONFIG_BT_CTLR_DF_ADV_CTE_TX) &&
	    (ext_hdr_flags & ULL_ADV_PDU_HDR_FIELD_CTE_INFO)) {
		(void)memcpy(dptr, cte_info, sizeof(*cte_info));
		cte_len_us = CTE_LEN_US(cte_info->time);
		dptr += sizeof(struct pdu_cte_info);
	} else {
		cte_len_us = 0U;
	}
	if (IS_ENABLED(CONFIG_BT_CTLR_ADV_PERIODIC_ADI_SUPPORT) &&
	    (ext_hdr_flags & ULL_ADV_PDU_HDR_FIELD_ADI)) {
		dptr += sizeof(struct pdu_adv_adi);
	}
	if (IS_ENABLED(CONFIG_BT_CTLR_ADV_SYNC_PDU_LINK) &&
	    (ext_hdr_flags & ULL_ADV_PDU_HDR_FIELD_AUX_PTR)) {
		aux_ptr = (void *)dptr;
		dptr += sizeof(struct pdu_adv_aux_ptr);
	}
	if (ext_hdr_flags & ULL_ADV_PDU_HDR_FIELD_TX_POWER) {
		dptr += sizeof(uint8_t);
	}

	/* Calc tertiary PDU len */
	len = ull_adv_aux_hdr_len_calc(com_hdr, &dptr);
	ull_adv_aux_hdr_len_fill(com_hdr, len);

	pdu->len = len;

#if defined(CONFIG_BT_CTLR_ADV_SYNC_PDU_BACK2BACK)
	/* Fill aux offset in aux pointer field */
	if (ext_hdr_flags & ULL_ADV_PDU_HDR_FIELD_AUX_PTR) {
		uint32_t offs_us;

		offs_us = PDU_AC_US(pdu->len, phy_s, phy_flags) +
			  EVENT_SYNC_B2B_MAFS_US;
		offs_us += cte_len_us;
		ull_adv_aux_ptr_fill(aux_ptr, offs_us, phy_s);
	}
#endif /* CONFIG_BT_CTLR_ADV_SYNC_PDU_BACK2BACK */
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
		 * by the hdr_add_fields then allocate memory for it.
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

uint8_t ull_adv_sync_duplicate(const struct pdu_adv *pdu_prev, struct pdu_adv *pdu_new)
{
#if defined(CONFIG_BT_CTLR_ADV_SYNC_PDU_LINK)
	/* Duplicate chain PDUs */
	return ull_adv_sync_duplicate_chain(pdu_prev, pdu_new);
#else
	ull_adv_sync_copy_pdu(pdu_prev, pdu_new);

	return 0U;
#endif /* CONFIG_BT_CTLR_ADV_SYNC_PDU_LINK */
}

#if defined(CONFIG_BT_CTLR_ADV_SYNC_PDU_LINK) || defined(CONFIG_BT_CTLR_DF_ADV_CTE_TX) || \
	defined(CONFIG_BT_CTLR_ADV_PERIODIC_ADI_SUPPORT)
static void ull_adv_sync_add_to_header(struct pdu_adv *pdu,
				       const struct pdu_adv_ext_hdr *fields,
				       uint8_t *ad_overflow, uint8_t *overflow_len)
{
	struct pdu_adv_com_ext_adv *hdr = &pdu->adv_ext_ind;
	uint8_t delta = 0U;
	uint8_t *dptr;

	if (overflow_len) {
		*overflow_len = 0U;
	}

	/* AdvA, TargetA and SyncInfo is RFU for periodic advertising */
	if (fields->cte_info && (hdr->ext_hdr_len == 0U || hdr->ext_hdr.cte_info == 0U)) {
		delta += sizeof(struct pdu_cte_info);
	}
	if (fields->adi && (hdr->ext_hdr_len == 0U || hdr->ext_hdr.adi == 0U)) {
		delta += sizeof(struct pdu_adv_adi);
	}
	if (fields->aux_ptr && (hdr->ext_hdr_len == 0U || hdr->ext_hdr.aux_ptr == 0U)) {
		delta += sizeof(struct pdu_adv_aux_ptr);
	}
	if (fields->tx_pwr && (hdr->ext_hdr_len == 0U || hdr->ext_hdr.tx_pwr == 0U)) {
		delta += 1U;
	}

	if (delta == 0U) {
		/* No new fields to add */
		return;
	}

	if (hdr->ext_hdr_len == 0) {
		/* Add one byte for the header flags */
		delta++;
	}

	/* Push back any adv data - overflow will be returned via ad_overflow */
	if (pdu->len > hdr->ext_hdr_len + 1U) {
		if (pdu->len > PDU_AC_EXT_PAYLOAD_SIZE_MAX - delta) {
			LL_ASSERT(ad_overflow);
			LL_ASSERT(overflow_len);
#if defined(CONFIG_BT_CTLR_ADV_SYNC_PDU_LINK)
			*overflow_len = delta - (PDU_AC_EXT_PAYLOAD_SIZE_MAX - pdu->len);
			memcpy(ad_overflow,
			       pdu->payload + PDU_AC_EXT_PAYLOAD_SIZE_MAX - *overflow_len,
			       *overflow_len);
			pdu->len -= *overflow_len;
#endif /* CONFIG_BT_CTLR_ADV_SYNC_PDU_LINK */
		}
		dptr = pdu->payload + hdr->ext_hdr_len + 1U;
		memmove(dptr + delta,
			dptr, pdu->len - hdr->ext_hdr_len - 1U);
	}

	pdu->len += delta;

	if (hdr->ext_hdr_len == 0U) {
		/* No extended header present, adding one */
		memcpy(&hdr->ext_hdr, fields, sizeof(struct pdu_adv_ext_hdr));
		hdr->ext_hdr_len = delta;
	} else {
		/* Go to end of current extended header and push back fields/ACAD in reverse */
		dptr = hdr->ext_hdr.data;

		/* AdvA and TargetA is RFU for periodic advertising */

#if defined(CONFIG_BT_CTLR_DF_ADV_CTE_TX)
		if (hdr->ext_hdr.cte_info) {
			dptr += sizeof(struct pdu_cte_info);
		}
#endif /* CONFIG_BT_CTLR_DF_ADV_CTE_TX */

		if (hdr->ext_hdr.adi) {
			dptr += sizeof(struct pdu_adv_adi);
		}

		if (hdr->ext_hdr.aux_ptr) {
			dptr += sizeof(struct pdu_adv_aux_ptr);
		}

		/* SyncInfo is RFU for periodic advertising */

		if (hdr->ext_hdr.tx_pwr) {
			dptr++;
		}

		/* Push back ACAD if any */
		if ((dptr - hdr->ext_hdr_adv_data) < hdr->ext_hdr_len) {
			uint8_t acad_len = hdr->ext_hdr_len - (dptr - hdr->ext_hdr_adv_data);

			memmove(dptr + delta, dptr, acad_len);
		}

		/* Set new header size now before starting to decrement delta */
		hdr->ext_hdr_len += delta;

		/* Now push back or add fields as needed */

		if (hdr->ext_hdr.tx_pwr) {
			dptr--;
			*(dptr + delta) = *dptr;
		} else if (fields->tx_pwr) {
			hdr->ext_hdr.tx_pwr = 1U;
			delta -= 1U;
		}

		if (hdr->ext_hdr.aux_ptr) {
			dptr -= sizeof(struct pdu_adv_aux_ptr);
			memmove(dptr + delta, dptr, sizeof(struct pdu_adv_aux_ptr));
		} else if (fields->aux_ptr) {
			hdr->ext_hdr.aux_ptr = 1U;
			delta -= sizeof(struct pdu_adv_aux_ptr);
		}

		if (hdr->ext_hdr.adi) {
			dptr -= sizeof(struct pdu_adv_adi);
			memmove(dptr + delta, dptr, sizeof(struct pdu_adv_adi));
		} else if (fields->adi) {
			hdr->ext_hdr.adi = 1U;
			delta -= sizeof(struct pdu_adv_adi);
		}

#if defined(CONFIG_BT_CTLR_DF_ADV_CTE_TX)
		if (hdr->ext_hdr.cte_info) {
			dptr -= sizeof(struct pdu_cte_info);
			memmove(dptr + delta, dptr, sizeof(struct pdu_cte_info));
		} else if (fields->cte_info) {
			hdr->ext_hdr.cte_info = 1U;
			delta -= sizeof(struct pdu_cte_info);
		}
#endif /* CONFIG_BT_CTLR_DF_ADV_CTE_TX */
	}
}

static void ull_adv_sync_remove_from_header(struct pdu_adv *pdu,
					    const struct pdu_adv_ext_hdr *fields,
					    bool acad)
{
	struct pdu_adv_com_ext_adv *hdr = &pdu->adv_ext_ind;
	uint8_t orig_hdr_len = hdr->ext_hdr_len;
	uint8_t *dptr;

	if (orig_hdr_len == 0U) {
		return;
	}

	dptr = hdr->ext_hdr.data;

	/* AdvA and TargetA is RFU for periodic advertising */

#if defined(CONFIG_BT_CTLR_DF_ADV_CTE_TX)
	if (hdr->ext_hdr.cte_info) {
		if (fields->cte_info) {
			memmove(dptr, dptr + sizeof(struct pdu_cte_info),
				hdr->ext_hdr_len - (hdr->ext_hdr.data - dptr));
			hdr->ext_hdr.cte_info = 0U;
			hdr->ext_hdr_len -= sizeof(struct pdu_cte_info);
		} else {
			dptr += sizeof(struct pdu_cte_info);
		}
	}
#endif /* CONFIG_BT_CTLR_DF_ADV_CTE_TX */

#if defined(CONFIG_BT_CTLR_ADV_PERIODIC_ADI_SUPPORT)
	if (hdr->ext_hdr.adi) {
		if (fields->adi) {
			memmove(dptr, dptr + sizeof(struct pdu_adv_adi),
				hdr->ext_hdr_len - (hdr->ext_hdr.data - dptr));
			hdr->ext_hdr.adi = 0U;
			hdr->ext_hdr_len -= sizeof(struct pdu_adv_adi);
		} else {
			dptr += sizeof(struct pdu_adv_adi);
		}
	}
#endif /* CONFIG_BT_CTLR_ADV_PERIODIC_ADI_SUPPORT */

#if defined(CONFIG_BT_CTLR_ADV_SYNC_PDU_LINK)
	if (hdr->ext_hdr.aux_ptr) {
		if (fields->aux_ptr) {
			memmove(dptr, dptr + sizeof(struct pdu_adv_aux_ptr),
				hdr->ext_hdr_len - (hdr->ext_hdr.data - dptr));
			hdr->ext_hdr.aux_ptr = 0U;
			hdr->ext_hdr_len -= sizeof(struct pdu_adv_aux_ptr);
		} else {
			dptr += sizeof(struct pdu_adv_aux_ptr);
		}
	}
#endif /* CONFIG_BT_CTLR_ADV_SYNC_PDU_LINK */

	/* SyncInfo is RFU for periodic advertising */

	if (hdr->ext_hdr.tx_pwr) {
		if (fields->tx_pwr) {
			memmove(dptr, dptr + 1U,
				hdr->ext_hdr_len - (hdr->ext_hdr.data - dptr));
			hdr->ext_hdr.tx_pwr = 0U;
			hdr->ext_hdr_len -= 1U;
		} else {
			dptr++;
		}
	}

	/* ACAD is the remainder of the header, if any left */
	if (acad) {
		/* Drop any ACAD */
		hdr->ext_hdr_len = dptr - hdr->ext_hdr_adv_data;
	}

	if (hdr->ext_hdr_len == 1U) {
		/* Only flags left in header, remove it completely */
		hdr->ext_hdr_len = 0U;
	}

	if (orig_hdr_len != hdr->ext_hdr_len) {
		/* Move adv data if any */
		if (pdu->len > orig_hdr_len + 1U) {
			memmove(hdr->ext_hdr_adv_data + hdr->ext_hdr_len,
				hdr->ext_hdr_adv_data + orig_hdr_len,
				pdu->len - orig_hdr_len - 1U);
		}

		/* Update total PDU len */
		pdu->len -= orig_hdr_len - hdr->ext_hdr_len;
	}

}
#endif /* CONFIG_BT_CTLR_ADV_SYNC_PDU_LINK || CONFIG_BT_CTLR_DF_ADV_CTE_TX || \
	*  CONFIG_BT_CTLR_ADV_PERIODIC_ADI_SUPPORT
	*/

static void ull_adv_sync_copy_pdu_header(struct pdu_adv *target_pdu,
					 const struct pdu_adv *source_pdu,
					 const struct pdu_adv_ext_hdr *skip_fields,
					 bool skip_acad)
{
	const struct pdu_adv_com_ext_adv *source_hdr = &source_pdu->adv_ext_ind;
	struct pdu_adv_com_ext_adv *target_hdr = &target_pdu->adv_ext_ind;
	const uint8_t *source_dptr;
	uint8_t *target_dptr;

	LL_ASSERT(target_pdu != source_pdu);

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
			if (!skip_fields->cte_info) {
				memcpy(target_dptr, source_dptr, sizeof(struct pdu_cte_info));
				target_dptr += sizeof(struct pdu_cte_info);
				target_hdr->ext_hdr.cte_info = 1U;
			}
			source_dptr += sizeof(struct pdu_cte_info);
		}
#endif /* CONFIG_BT_CTLR_DF_ADV_CTE_TX */

#if defined(CONFIG_BT_CTLR_ADV_PERIODIC_ADI_SUPPORT)
		if (source_hdr->ext_hdr.adi) {
			if (!skip_fields->adi) {
				memcpy(target_dptr, source_dptr, sizeof(struct pdu_adv_adi));
				target_dptr += sizeof(struct pdu_adv_adi);
				target_hdr->ext_hdr.adi = 1U;
			}
			source_dptr += sizeof(struct pdu_adv_adi);
		}
#endif /* CONFIG_BT_CTLR_ADV_PERIODIC_ADI_SUPPORT */

#if defined(CONFIG_BT_CTLR_ADV_SYNC_PDU_LINK)
		if (source_hdr->ext_hdr.aux_ptr) {
			if (!skip_fields->aux_ptr) {
				memcpy(target_dptr, source_dptr, sizeof(struct pdu_adv_aux_ptr));
				target_dptr += sizeof(struct pdu_adv_aux_ptr);
				target_hdr->ext_hdr.aux_ptr = 1U;
			}
			source_dptr += sizeof(struct pdu_adv_aux_ptr);
		}
#endif /* CONFIG_BT_CTLR_ADV_SYNC_PDU_LINK */

		/* SyncInfo is RFU for periodic advertising */

		if (source_hdr->ext_hdr.tx_pwr) {
			if (!skip_fields->tx_pwr) {
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

#if defined(CONFIG_BT_CTLR_ADV_PERIODIC_ADI_SUPPORT)
static void ull_adv_sync_update_pdu_adi(struct lll_adv_sync *lll_sync, struct pdu_adv *pdu,
					uint16_t did)
{
	struct pdu_adv_com_ext_adv *hdr = &pdu->adv_ext_ind;
	struct ll_adv_set *adv = HDR_LLL2ULL(lll_sync->adv);
	struct pdu_adv_adi *adi;
	uint8_t *dptr;

	if (hdr->ext_hdr_len == 0U || hdr->ext_hdr.adi == 0U) {
		/* No ADI field present, nothing to do */
		return;
	}

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

#if defined(CONFIG_BT_CTLR_ADV_SYNC_PDU_LINK)
static void ull_adv_sync_add_aux_ptr(struct pdu_adv *pdu, uint8_t *ad_overflow,
				     uint8_t *overflow_len)
{
	struct pdu_adv_ext_hdr fields = { 0U };

	fields.aux_ptr = 1U;
	ull_adv_sync_add_to_header(pdu, &fields, ad_overflow, overflow_len);
}

static void ull_adv_sync_update_aux_ptr(struct lll_adv_sync *lll_sync, struct pdu_adv *pdu)
{
	struct pdu_adv_com_ext_adv *hdr = &pdu->adv_ext_ind;
	struct pdu_adv_aux_ptr *aux_ptr;
	struct ll_adv_set *adv;
	uint32_t offs_us;
	uint8_t *dptr;
#if defined(CONFIG_BT_CTLR_DF_ADV_CTE_TX)
	struct pdu_cte_info *cte_info = NULL;
#endif /* CONFIG_BT_CTLR_DF_ADV_CTE_TX */

	if (!hdr->ext_hdr_len || !hdr->ext_hdr.aux_ptr) {
		/* Nothing to update */
		return;
	}

	/* Find AuxPtr */
	dptr = hdr->ext_hdr.data;

	/* AdvA and TargetA is RFU for periodic advertising */

#if defined(CONFIG_BT_CTLR_DF_ADV_CTE_TX)
	if (hdr->ext_hdr.cte_info) {
		cte_info = (struct pdu_cte_info *)dptr;
		dptr += sizeof(struct pdu_cte_info);
	}
#endif /* CONFIG_BT_CTLR_DF_ADV_CTE_TX */

	if (hdr->ext_hdr.adi) {
		dptr += sizeof(struct pdu_adv_adi);
	}

	/* Now at AuxPtr */
	aux_ptr = (struct pdu_adv_aux_ptr *)dptr;

	/* Calculate and set offset */
	adv = HDR_LLL2ULL(lll_sync->adv);
	offs_us = PDU_AC_US(pdu->len, adv->lll.phy_s, adv->lll.phy_flags) + EVENT_SYNC_B2B_MAFS_US;

#if defined(CONFIG_BT_CTLR_DF_ADV_CTE_TX)
	/* Add CTE time if relevant */
	if (cte_info) {
		offs_us += CTE_LEN_US(cte_info->time);
	}
#endif /* CONFIG_BT_CTLR_DF_ADV_CTE_TX */

	ull_adv_aux_ptr_fill(aux_ptr, offs_us, adv->lll.phy_s);
}
#endif /* CONFIG_BT_CTLR_ADV_SYNC_PDU_LINK */

static uint8_t ull_adv_sync_append_ad_data(struct lll_adv_sync *lll_sync, struct pdu_adv *pdu,
					   const uint8_t *ad, uint8_t ad_len, uint8_t max_ad_len)
{
#if defined(CONFIG_BT_CTLR_ADV_SYNC_PDU_LINK)
	uint8_t ad_overflow[sizeof(struct pdu_adv_aux_ptr) + 1U];
	uint8_t overflow_len = 0U;
#endif /* CONFIG_BT_CTLR_ADV_SYNC_PDU_LINK */

	while (ad_len) {
		uint8_t pdu_ad_len;
		uint8_t *dptr;
		uint8_t ext_hdr_len;
		uint8_t pdu_add_len = ad_len;

		ext_hdr_len = pdu->adv_ext_ind.ext_hdr_len + 1U;
		pdu_ad_len = pdu->len - ext_hdr_len;

		/* Check if the adv data exceeds the configured maximum */
		/* FIXME - this check should include the entire chain */
		if (pdu_ad_len + ad_len > CONFIG_BT_CTLR_ADV_DATA_LEN_MAX) {
			return BT_HCI_ERR_MEM_CAPACITY_EXCEEDED;
		}

		/* Only allow up to max_ad_len adv data per PDU */
		if (pdu_ad_len + ad_len > max_ad_len ||
		    PDU_AC_EXT_PAYLOAD_SIZE_MAX - pdu->len < ad_len) {
#if !defined(CONFIG_BT_CTLR_ADV_SYNC_PDU_LINK)
			return BT_HCI_ERR_MEM_CAPACITY_EXCEEDED;
#else
			/* Will fragment into chain PDU */

			/* Add aux_ptr to existing PDU */
			ull_adv_sync_add_aux_ptr(pdu, ad_overflow, &overflow_len);

			pdu_add_len = MAX(0U, MIN(max_ad_len - pdu_ad_len,
					  PDU_AC_EXT_PAYLOAD_SIZE_MAX - pdu->len));
#endif /* CONFIG_BT_CTLR_ADV_SYNC_PDU_LINK */
		}

		dptr = pdu->payload + pdu->len;

#if defined(CONFIG_BT_CTLR_ADV_SYNC_PDU_LINK)
		if (pdu_add_len && overflow_len) {
			/* Overflow from previous PDU in chain, add this first */
			memcpy(dptr, ad, overflow_len);
			pdu->len += overflow_len;
			dptr += overflow_len;
			overflow_len = 0U;
		}
#endif /* CONFIG_BT_CTLR_ADV_SYNC_PDU_LINK */

		if (pdu_add_len) {
			memcpy(dptr, ad, pdu_add_len);
			pdu->len += pdu_add_len;
			ad_len -= pdu_add_len;
			ad += pdu_add_len;
		}
#if defined(CONFIG_BT_CTLR_ADV_SYNC_PDU_LINK)
		if (ad_len) {
			struct pdu_adv_ext_hdr skip_fields = { 0U };
			struct pdu_adv *pdu_chain;

			/* Fill the aux offset in superior PDU */
			ull_adv_sync_update_aux_ptr(lll_sync, pdu);

			/* Allocate new PDU */
			pdu_chain = lll_adv_pdu_alloc_pdu_adv();
			if (!pdu_chain) {
				return BT_HCI_ERR_MEM_CAPACITY_EXCEEDED;
			}

			/* Copy header, removing AuxPtr, CTEInfo and ACAD */
			skip_fields.aux_ptr = 1U;
			skip_fields.cte_info = 1U;
			ull_adv_sync_copy_pdu_header(pdu_chain, pdu, &skip_fields, true);

			/* Chain the PDU */
			lll_adv_pdu_linked_append(pdu_chain, pdu);

			pdu = pdu_chain;
		}
#endif /* CONFIG_BT_CTLR_ADV_SYNC_PDU_LINK */
	}

	return 0U;
}

static void ull_adv_sync_copy_pdu(const struct pdu_adv *pdu_prev,
				  struct pdu_adv *pdu)
{
	if (pdu == pdu_prev) {
		return;
	}

	/* Initialize PDU header */
	pdu->type = pdu_prev->type;
	pdu->rfu = 0U;
	pdu->chan_sel = 0U;
	pdu->tx_addr = 0U;
	pdu->rx_addr = 0U;
	pdu->len = pdu_prev->len;

	/* Copy PDU payload */
	memcpy(pdu->payload, pdu_prev->payload, pdu_prev->len);
}

#if defined(CONFIG_BT_CTLR_ADV_SYNC_PDU_LINK)
static uint8_t ull_adv_sync_duplicate_chain(const struct pdu_adv *pdu_prev,
					    struct pdu_adv *pdu)
{
	/* If pdu_prev == pdu we are done */
	if (pdu == pdu_prev) {
		return 0U;
	}

	/* Copy existing PDU chain */
	while (pdu_prev) {
		struct pdu_adv *pdu_chain;

		ull_adv_sync_copy_pdu(pdu_prev, pdu);

		pdu_prev = lll_adv_pdu_linked_next_get(pdu_prev);
		pdu_chain = lll_adv_pdu_linked_next_get(pdu);
		if (pdu_prev && !pdu_chain) {
			/* Get a new chain PDU */
			pdu_chain = lll_adv_pdu_alloc_pdu_adv();
			if (!pdu_chain) {
				return BT_HCI_ERR_MEM_CAPACITY_EXCEEDED;
			}

			/* Link the chain PDU to parent PDU */
			lll_adv_pdu_linked_append(pdu_chain, pdu);
			pdu = pdu_chain;
		}
	}

	return 0U;
}
#endif /* CONFIG_BT_CTLR_ADV_SYNC_PDU_LINK */

static uint8_t ull_adv_sync_ad_add(struct lll_adv_sync *lll_sync,
				   struct pdu_adv *ter_pdu_prev,
				   struct pdu_adv *ter_pdu,
				   const uint8_t *ad, uint8_t ad_len)
{
	uint8_t pdu_ad_max_len = PDU_AC_EXT_AD_DATA_LEN_MAX;
#if defined(CONFIG_BT_CTLR_ADV_SYNC_PDU_LINK)
	uint8_t err;

	err = ull_adv_sync_duplicate_chain(ter_pdu_prev, ter_pdu);
	if (err) {
		return err;
	}

	/* Find end of current advertising data */
	while (lll_adv_pdu_linked_next_get(ter_pdu)) {
		ter_pdu = lll_adv_pdu_linked_next_get(ter_pdu);
		ter_pdu_prev = lll_adv_pdu_linked_next_get(ter_pdu_prev);

		/* Use the full PDU payload for AUX_CHAIN_IND */
		pdu_ad_max_len = PDU_AC_EXT_PAYLOAD_SIZE_MAX;

#if defined(CONFIG_BT_CTLR_DF_ADV_CTE_TX)
		/* Detect end of current advertising data */
		if (ter_pdu->len < PDU_AC_EXT_PAYLOAD_SIZE_MAX) {
			break;
		}
#endif /* CONFIG_BT_CTLR_DF_ADV_CTE_TX */
	}
#else /* !CONFIG_BT_CTLR_ADV_SYNC_PDU_LINK */
	/* Initialize PDU header */
	ter_pdu->type = ter_pdu_prev->type;
	ter_pdu->rfu = 0U;
	ter_pdu->chan_sel = 0U;
	ter_pdu->tx_addr = 0U;
	ter_pdu->rx_addr = 0U;
	ter_pdu->len = ter_pdu_prev->len;

	/* Copy PDU payload */
	memcpy(ter_pdu->payload, ter_pdu_prev->payload, ter_pdu_prev->len);
#endif /* !CONFIG_BT_CTLR_ADV_SYNC_PDU_LINK */

	/* At end of copied chain, append new adv data */
	return ull_adv_sync_append_ad_data(lll_sync, ter_pdu, ad, ad_len, pdu_ad_max_len);
}

static uint8_t ull_adv_sync_ad_replace(struct lll_adv_sync *lll_sync,
				       struct pdu_adv *ter_pdu_prev,
				       struct pdu_adv *ter_pdu,
				       const uint8_t *ad, uint8_t ad_len)
{
	struct pdu_adv_ext_hdr skip_fields = { 0U };

	skip_fields.aux_ptr = 1U;

	/* FIXME - below ignores any configured CTE count */
	if (ter_pdu_prev == ter_pdu) {
		/* Remove adv data and any AuxPtr */
		ter_pdu->len = ter_pdu_prev->adv_ext_ind.ext_hdr_len + 1U;
#if defined(CONFIG_BT_CTLR_ADV_SYNC_PDU_LINK)
		ull_adv_sync_remove_from_header(ter_pdu, &skip_fields, false);
		/* Delete any existing PDU chain */
		if (lll_adv_pdu_linked_next_get(ter_pdu)) {
			lll_adv_pdu_linked_release_all(lll_adv_pdu_linked_next_get(ter_pdu));
			lll_adv_pdu_linked_append(NULL, ter_pdu);
		}
#endif /* CONFIG_BT_CTLR_ADV_SYNC_PDU_LINK */
	} else {
		/* Copy header (only), removing any prior presence of Aux Ptr */
		ull_adv_sync_copy_pdu_header(ter_pdu, ter_pdu_prev, &skip_fields, false);
	}

#if defined(CONFIG_BT_CTLR_ADV_PERIODIC_ADI_SUPPORT)
	/* New adv data - update ADI if present */
	if (ter_pdu->adv_ext_ind.ext_hdr_len && ter_pdu->adv_ext_ind.ext_hdr.adi) {
		struct ll_adv_set *adv = HDR_LLL2ULL(lll_sync->adv);
		uint16_t did;

		/* The DID for a specific SID shall be unique */
		did = sys_cpu_to_le16(ull_adv_aux_did_next_unique_get(adv->sid));
		ull_adv_sync_update_pdu_adi(lll_sync, ter_pdu, did);
	}
#endif /* CONFIG_BT_CTLR_ADV_PERIODIC_ADI_SUPPORT */

	/* Set advertising data (without copying any existing adv data) */
	return ull_adv_sync_append_ad_data(lll_sync, ter_pdu, ad, ad_len,
					   PDU_AC_EXT_AD_DATA_LEN_MAX);
}

#if defined(CONFIG_BT_CTLR_ADV_PERIODIC_ADI_SUPPORT)
static uint8_t ull_adv_sync_update_adi(struct lll_adv_sync *lll_sync,
				       struct pdu_adv *ter_pdu_prev,
				       struct pdu_adv *ter_pdu)
{
	struct ll_adv_set *adv = HDR_LLL2ULL(lll_sync->adv);
	uint16_t did;

	/* The DID for a specific SID shall be unique */
	did = sys_cpu_to_le16(ull_adv_aux_did_next_unique_get(adv->sid));

#if defined(CONFIG_BT_CTLR_ADV_SYNC_PDU_LINK)
	uint8_t err;

	err = ull_adv_sync_duplicate_chain(ter_pdu_prev, ter_pdu);
	if (err) {
		return err;
	}

	/* Loop through chain and set new ADI for all */
	while (ter_pdu) {
		ull_adv_sync_update_pdu_adi(lll_sync, ter_pdu, did);
		ter_pdu = lll_adv_pdu_linked_next_get(ter_pdu);
	}
#else /* !CONFIG_BT_CTLR_ADV_SYNC_PDU_LINK */
	/* Initialize PDU header */
	ter_pdu->type = ter_pdu_prev->type;
	ter_pdu->rfu = 0U;
	ter_pdu->chan_sel = 0U;
	ter_pdu->tx_addr = 0U;
	ter_pdu->rx_addr = 0U;
	ter_pdu->len = ter_pdu_prev->len;

	/* Copy PDU payload */
	memcpy(ter_pdu->payload, ter_pdu_prev->payload, ter_pdu_prev->len);

	/* Set new ADI */
	ull_adv_sync_update_pdu_adi(lll_sync, ter_pdu, did);
#endif /*  CONFIG_BT_CTLR_ADV_SYNC_PDU_LINK*/
	return 0U;
}

static uint8_t ull_adv_sync_add_adi(struct lll_adv_sync *lll_sync,
				    struct pdu_adv *pdu_prev,
				    struct pdu_adv *pdu)
{
	struct ll_adv_set *adv = HDR_LLL2ULL(lll_sync->adv);
	struct pdu_adv_ext_hdr add_fields = { 0U };
#if defined(CONFIG_BT_CTLR_ADV_SYNC_PDU_LINK)
	uint8_t ad_overflow[sizeof(struct pdu_adv_adi)*MAX_FRAG_COUNT];
	uint8_t total_overflow_len;
	struct pdu_adv *last_pdu;
	uint8_t overflow_len;
	uint8_t err;
#endif /* CONFIG_BT_CTLR_ADV_SYNC_PDU_LINK */
	uint16_t did;

	add_fields.adi = 1U;

	/* The DID for a specific SID shall be unique */
	did = sys_cpu_to_le16(ull_adv_aux_did_next_unique_get(adv->sid));

#if defined(CONFIG_BT_CTLR_ADV_SYNC_PDU_LINK)
	err = ull_adv_sync_duplicate_chain(pdu_prev, pdu);
	if (err) {
		return err;
	}

	total_overflow_len = 0U;
	/* Loop through chain and add ADI for all */
	while (pdu) {
		last_pdu = pdu;

		/* We should always have enough available overflow space to fit an ADI */
		LL_ASSERT(total_overflow_len + sizeof(struct pdu_adv_adi) <= sizeof(ad_overflow));

		ull_adv_sync_add_to_header(pdu, &add_fields,
					   &ad_overflow[total_overflow_len], &overflow_len);
		total_overflow_len += overflow_len;
		ull_adv_sync_update_pdu_adi(lll_sync, pdu, did);
		pdu = lll_adv_pdu_linked_next_get(pdu);
		if (pdu) {
			uint8_t ad_overflow_tmp[sizeof(struct pdu_adv_adi)*MAX_FRAG_COUNT];
			uint8_t overflow_tmp_len = 0U;
			uint8_t pdu_avail = PDU_AC_EXT_PAYLOAD_SIZE_MAX - pdu->len;
			uint8_t pdu_needed = total_overflow_len;
			uint8_t *dptr;

			if (!pdu->adv_ext_ind.ext_hdr.adi) {
				pdu_needed += sizeof(struct pdu_adv_adi);
			}
			if (pdu->adv_ext_ind.ext_hdr_len == 0U) {
				/* Make room for flags as well */
				pdu_needed++;
			}

			if (total_overflow_len > 0U) {
				if (pdu_avail < pdu_needed) {
					/* Make room by removing last part of adv data */
					overflow_tmp_len = pdu_needed - pdu_avail;
					memcpy(ad_overflow_tmp,
					       pdu->payload + (pdu->len - overflow_tmp_len),
					       overflow_tmp_len);
					pdu->len -= overflow_tmp_len;
				}

				/* Prepend overflow from last PDU */
				dptr = pdu->payload + pdu->adv_ext_ind.ext_hdr_len + 1U;
				memmove(dptr + total_overflow_len, dptr,
					pdu->len - pdu->adv_ext_ind.ext_hdr_len - 1U +
					 total_overflow_len);
				pdu->len += total_overflow_len;
				memcpy(dptr, ad_overflow, total_overflow_len);

				/* Carry forward overflow from this PDU */
				total_overflow_len = overflow_tmp_len;
				if (overflow_tmp_len) {
					memcpy(ad_overflow, ad_overflow_tmp, overflow_tmp_len);
				}
			}
		}
	}

	/* Push any remaining overflow on to last PDU */
	ull_adv_sync_append_ad_data(lll_sync, pdu, ad_overflow, total_overflow_len,
				    PDU_AC_EXT_PAYLOAD_SIZE_MAX);

#else /* !CONFIG_BT_CTLR_ADV_SYNC_PDU_LINK */

	if (pdu->len > PDU_AC_EXT_PAYLOAD_SIZE_MAX - sizeof(struct pdu_adv_adi)) {
		/* No room for ADI */
		return BT_HCI_ERR_PACKET_TOO_LONG;
	}

	/* Initialize PDU header */
	pdu->type = pdu_prev->type;
	pdu->rfu = 0U;
	pdu->chan_sel = 0U;
	pdu->tx_addr = 0U;
	pdu->rx_addr = 0U;
	pdu->len = pdu_prev->len;

	/* Copy PDU payload */
	memcpy(pdu->payload, pdu_prev->payload, pdu_prev->len);

	/* Add and set new ADI */
	ull_adv_sync_add_to_header(pdu, &add_fields, NULL, NULL);
	ull_adv_sync_update_pdu_adi(lll_sync, pdu, did);
#endif /* CONFIG_BT_CTLR_ADV_SYNC_PDU_LINK */
	return 0U;
}

static uint8_t ull_adv_sync_remove_adi(struct lll_adv_sync *lll_sync,
				       struct pdu_adv *pdu_prev,
				       struct pdu_adv *pdu)
{
	struct pdu_adv_ext_hdr remove_fields = { 0U };
#if defined(CONFIG_BT_CTLR_ADV_SYNC_PDU_LINK)
	uint8_t err;
#endif /* CONFIG_BT_CTLR_ADV_SYNC_PDU_LINK */

	remove_fields.adi = 1U;

#if defined(CONFIG_BT_CTLR_ADV_SYNC_PDU_LINK)
	err = ull_adv_sync_duplicate_chain(pdu_prev, pdu);
	if (err) {
		return err;
	}

	/* Loop through chain and remove ADI for all */
	while (pdu) {
		ull_adv_sync_remove_from_header(pdu, &remove_fields, false);
		if (pdu->adv_ext_ind.ext_hdr_len && pdu->adv_ext_ind.ext_hdr.aux_ptr) {
			ull_adv_sync_update_aux_ptr(lll_sync, pdu);
		}
		pdu = lll_adv_pdu_linked_next_get(pdu);
	}
#else /* !CONFIG_BT_CTLR_ADV_SYNC_PDU_LINK */
	ull_adv_sync_remove_from_header(pdu, &remove_fields, false);
#endif /* CONFIG_BT_CTLR_ADV_SYNC_PDU_LINK */

	return 0U;
}
#endif /* CONFIG_BT_CTLR_ADV_PERIODIC_ADI_SUPPORT */

uint8_t *ull_adv_sync_get_acad(struct pdu_adv *pdu, uint8_t *acad_len)
{
	struct pdu_adv_com_ext_adv *hdr = &pdu->adv_ext_ind;
	uint8_t *dptr;

	/* Skip to ACAD */
	dptr = hdr->ext_hdr.data;

	/* AdvA and TargetA is RFU for periodic advertising */

#if defined(CONFIG_BT_CTLR_DF_ADV_CTE_TX)
	if (hdr->ext_hdr.cte_info) {
		dptr += sizeof(struct pdu_cte_info);
	}
#endif /* CONFIG_BT_CTLR_DF_ADV_CTE_TX */

#if defined(CONFIG_BT_CTLR_ADV_PERIODIC_ADI_SUPPORT)
	if (hdr->ext_hdr.adi) {
		dptr += sizeof(struct pdu_adv_adi);
	}
#endif /* CONFIG_BT_CTLR_ADV_PERIODIC_ADI_SUPPORT */

#if defined(CONFIG_BT_CTLR_ADV_SYNC_PDU_LINK)
	if (hdr->ext_hdr.aux_ptr) {
		dptr += sizeof(struct pdu_adv_aux_ptr);
	}
#endif /* CONFIG_BT_CTLR_ADV_SYNC_PDU_LINK */

	/* SyncInfo is RFU for periodic advertising */

	if (hdr->ext_hdr.tx_pwr) {
		dptr++;
	}

	/* ACAD is the remainder of the header, if any left */
	if ((dptr - hdr->ext_hdr_adv_data) < hdr->ext_hdr_len) {
		*acad_len = hdr->ext_hdr_len - (dptr - hdr->ext_hdr_adv_data);
	} else {
		*acad_len = 0U;
	}

	return dptr;
}

uint8_t ull_adv_sync_remove_from_acad(struct lll_adv_sync *lll_sync,
				      struct pdu_adv *pdu_prev,
				      struct pdu_adv *pdu,
				      uint8_t ad_type)
{
	uint8_t acad_len;
	uint8_t ad_len;
	uint8_t *acad;
	uint8_t len;
	uint8_t *ad;
#if defined(CONFIG_BT_CTLR_ADV_SYNC_PDU_LINK)
	uint8_t err;

	err = ull_adv_sync_duplicate_chain(pdu_prev, pdu);
	if (err) {
		return err;
	}
#else
	ull_adv_sync_copy_pdu(pdu_prev, pdu);
#endif /* CONFIG_BT_CTLR_ADV_SYNC_PDU_LINK */

	acad = ull_adv_sync_get_acad(pdu, &acad_len);

	if (acad_len == 0U) {
		return 0U;
	}

	/* Find the relevant entry */
	len = acad_len;
	ad = acad;
	do {
		ad_len = ad[PDU_ADV_DATA_HEADER_LEN_OFFSET];
		if (ad_len && (ad[PDU_ADV_DATA_HEADER_TYPE_OFFSET] == ad_type)) {
			break;
		}

		ad_len += 1U;

		LL_ASSERT(ad_len <= len);

		ad += ad_len;
		len -= ad_len;
	} while (len);

	if (len == 0U) {
		/* Entry is not present */
		return 0U;
	}

	/* Remove entry by moving the rest of the PDU content forward */
	ad_len += 1U;
	(void)memmove(ad, ad + ad_len, pdu->len - ((ad + ad_len) - pdu->payload));

	/* Adjust lengths */
	pdu->len -= ad_len;
	pdu->adv_ext_ind.ext_hdr_len -= ad_len;

	return 0U;
}

uint8_t ull_adv_sync_add_to_acad(struct lll_adv_sync *lll_sync,
				 struct pdu_adv *pdu_prev,
				 struct pdu_adv *pdu,
				 const uint8_t *new_ad,
				 uint8_t new_ad_len)
{
	uint8_t ad_len;
	uint8_t delta;
	uint8_t *dptr;
#if defined(CONFIG_BT_CTLR_ADV_SYNC_PDU_LINK)
	uint8_t err;

	err = ull_adv_sync_duplicate_chain(pdu_prev, pdu);
	if (err) {
		return err;
	}
#else
	ull_adv_sync_copy_pdu(pdu_prev, pdu);
#endif /* CONFIG_BT_CTLR_ADV_SYNC_PDU_LINK */

	delta = new_ad_len;
	if (pdu->adv_ext_ind.ext_hdr_len == 0U) {
		/* Add one byte for the header flags */
		delta++;
	}

	if (pdu->len > PDU_AC_EXT_PAYLOAD_SIZE_MAX - delta) {
		return BT_HCI_ERR_PACKET_TOO_LONG;
	}

	dptr = pdu->payload + pdu->adv_ext_ind.ext_hdr_len + 1U;

	/* Make room in ACAD by moving any advertising data back */
	ad_len = pdu->len - pdu->adv_ext_ind.ext_hdr_len - 1U;
	if (ad_len) {
		(void)memmove(dptr + delta, dptr, ad_len);
	}

	if (pdu->adv_ext_ind.ext_hdr_len == 0U) {
		/* Set all extended header flags to 0 */
		*dptr = 0U;
		dptr++;
	}

	/* Copy in ACAD data */
	memcpy(dptr, new_ad, new_ad_len);

	/* Adjust lengths */
	pdu->len += delta;
	pdu->adv_ext_ind.ext_hdr_len += delta;

	return 0U;
}

#if defined(CONFIG_BT_CTLR_DF_ADV_CTE_TX)
static void ull_adv_sync_update_pdu_cteinfo(struct lll_adv_sync *lll_sync, struct pdu_adv *pdu,
					    const struct pdu_cte_info *cte_info)
{
	struct pdu_adv_com_ext_adv *hdr = &pdu->adv_ext_ind;
	uint8_t *dptr;

	if (hdr->ext_hdr_len == 0U || hdr->ext_hdr.cte_info == 0U) {
		/* No CTEInfo field present, nothing to do */
		return;
	}

	/* Find CTEInfo in extended header */
	dptr = hdr->ext_hdr.data;

	/* AdvA and TargetA is RFU for periodic advertising */

	/* Copy supplied data into extended header */
	memcpy(dptr, (uint8_t *)cte_info, sizeof(struct pdu_cte_info));
}

uint8_t ull_adv_sync_add_cteinfo(struct lll_adv_sync *lll_sync,
				 struct pdu_adv *pdu_prev,
				 struct pdu_adv *pdu,
				 const struct pdu_cte_info *cte_info,
				 uint8_t cte_count)
{
	struct pdu_adv_ext_hdr add_fields = { 0U };
#if defined(CONFIG_BT_CTLR_ADV_SYNC_PDU_LINK)
	uint8_t ad_overflow[sizeof(struct pdu_cte_info)*MAX_FRAG_COUNT];
	uint8_t total_overflow_len;
	struct pdu_adv *last_pdu = pdu;
	uint8_t overflow_len;
	uint8_t err;
#endif /* CONFIG_BT_CTLR_ADV_SYNC_PDU_LINK */

	add_fields.cte_info = 1U;

#if defined(CONFIG_BT_CTLR_ADV_SYNC_PDU_LINK)
	err = ull_adv_sync_duplicate_chain(pdu_prev, pdu);
	if (err) {
		return err;
	}

	total_overflow_len = 0U;
	/* Loop through chain and add CTEInfo for PDUs up to cte_count */
	while (pdu && cte_count) {
		last_pdu = pdu;

		/* We should always have enough available overflow space to fit CTEInfo */
		LL_ASSERT(total_overflow_len + sizeof(struct pdu_cte_info) <= sizeof(ad_overflow));

		ull_adv_sync_add_to_header(pdu, &add_fields,
					   &ad_overflow[total_overflow_len], &overflow_len);
		total_overflow_len += overflow_len;
		ull_adv_sync_update_pdu_cteinfo(lll_sync, pdu, cte_info);
		cte_count--;

		/* Update AuxPtr if present */
		ull_adv_sync_update_aux_ptr(lll_sync, pdu);

		pdu = lll_adv_pdu_linked_next_get(pdu);
		if (pdu) {
			uint8_t ad_overflow_tmp[sizeof(struct pdu_cte_info)*MAX_FRAG_COUNT];
			uint8_t overflow_tmp_len = 0U;
			uint8_t pdu_avail = PDU_AC_EXT_PAYLOAD_SIZE_MAX - pdu->len;
			uint8_t pdu_needed = total_overflow_len;
			uint8_t *dptr;

			if (!pdu->adv_ext_ind.ext_hdr.cte_info) {
				pdu_needed += sizeof(struct pdu_cte_info);
			}
			if (pdu->adv_ext_ind.ext_hdr_len == 0U) {
				/* Make room for flags as well */
				pdu_needed++;
			}

			if (total_overflow_len > 0U) {
				if (pdu_avail < pdu_needed) {
					/* Make room by removing last part of adv data */
					overflow_tmp_len = pdu_needed - pdu_avail;
					memcpy(ad_overflow_tmp,
					       pdu->payload + (pdu->len - overflow_tmp_len),
					       overflow_tmp_len);
					pdu->len -= overflow_tmp_len;
				}

				/* Prepend overflow from last PDU */
				dptr = pdu->payload + pdu->adv_ext_ind.ext_hdr_len + 1U;
				memmove(dptr + total_overflow_len, dptr,
					pdu->len - pdu->adv_ext_ind.ext_hdr_len - 1U +
					 total_overflow_len);
				pdu->len += total_overflow_len;
				memcpy(dptr, ad_overflow, total_overflow_len);

				/* Carry forward overflow from this PDU */
				total_overflow_len = overflow_tmp_len;
				if (overflow_tmp_len) {
					memcpy(ad_overflow, ad_overflow_tmp, overflow_tmp_len);
				}
			}

		}
	}

	pdu = last_pdu;

	/* Push any remaining overflow on to last PDU */
	ull_adv_sync_append_ad_data(lll_sync, pdu, ad_overflow, total_overflow_len,
				    PDU_AC_EXT_PAYLOAD_SIZE_MAX);

#if (CONFIG_BT_CTLR_DF_PER_ADV_CTE_NUM_MAX > 1)
	/* Add PDUs up to cte_count if needed */
	while (cte_count) {
		struct pdu_adv_ext_hdr skip_fields = { 0U };
		struct pdu_adv *pdu_chain;

		skip_fields.adi = 1U;
		skip_fields.aux_ptr = 1U;
		skip_fields.tx_pwr = 1U;

		/* Get a new chain PDU */
		pdu_chain = lll_adv_pdu_alloc_pdu_adv();
		if (!pdu_chain) {
			return BT_HCI_ERR_MEM_CAPACITY_EXCEEDED;
		}

		/* Link the chain PDU to parent PDU */
		lll_adv_pdu_linked_append(pdu_chain, pdu);

		/* Copy header to new PDU, skipping all fields except CTEInfo */
		ull_adv_sync_copy_pdu_header(pdu_chain, pdu, &skip_fields, true);

		/* Add and set aux_ptr to existing PDU */
		ull_adv_sync_add_aux_ptr(pdu, ad_overflow, &overflow_len);
		ull_adv_sync_update_aux_ptr(lll_sync, pdu);

		if (overflow_len) {
			ull_adv_sync_append_ad_data(lll_sync, pdu_chain, ad_overflow, overflow_len,
				    PDU_AC_EXT_PAYLOAD_SIZE_MAX);
		}

		pdu = pdu_chain;
		cte_count--;
	}
#endif /* CONFIG_BT_CTLR_DF_PER_ADV_CTE_NUM_MAX > 1 */

#else /* !CONFIG_BT_CTLR_ADV_SYNC_PDU_LINK */

	if (pdu->len > PDU_AC_EXT_PAYLOAD_SIZE_MAX - sizeof(struct pdu_cte_info)) {
		/* No room for CTEInfo */
		return BT_HCI_ERR_PACKET_TOO_LONG;
	}

	/* Initialize PDU header */
	pdu->type = pdu_prev->type;
	pdu->rfu = 0U;
	pdu->chan_sel = 0U;
	pdu->tx_addr = 0U;
	pdu->rx_addr = 0U;
	pdu->len = pdu_prev->len;

	/* Copy PDU payload */
	memcpy(pdu->payload, pdu_prev->payload, pdu_prev->len);

	/* Add and set CTEInfo */
	ull_adv_sync_add_to_header(pdu, &add_fields, NULL, NULL);
	ull_adv_sync_update_pdu_cteinfo(lll_sync, pdu, cte_info);
#endif /* !CONFIG_BT_CTLR_ADV_SYNC_PDU_LINK */
	return 0U;
}

uint8_t ull_adv_sync_remove_cteinfo(struct lll_adv_sync *lll_sync,
				    struct pdu_adv *pdu_prev,
				    struct pdu_adv *pdu)
{
	struct pdu_adv_ext_hdr remove_fields = { 0U };
#if defined(CONFIG_BT_CTLR_ADV_SYNC_PDU_LINK)
	uint8_t err;
#endif /* CONFIG_BT_CTLR_ADV_SYNC_PDU_LINK */

	remove_fields.cte_info = 1U;

#if defined(CONFIG_BT_CTLR_ADV_SYNC_PDU_LINK)
	err = ull_adv_sync_duplicate_chain(pdu_prev, pdu);
	if (err) {
		return err;
	}

	/* Loop through chain and remove CTEInfo for all */
	while (pdu) {
		struct pdu_adv *pdu_chain;

		ull_adv_sync_remove_from_header(pdu, &remove_fields, false);

		if (pdu->adv_ext_ind.ext_hdr_len && pdu->adv_ext_ind.ext_hdr.aux_ptr) {
			ull_adv_sync_update_aux_ptr(lll_sync, pdu);
		}

		pdu_chain = lll_adv_pdu_linked_next_get(pdu);
#if (CONFIG_BT_CTLR_DF_PER_ADV_CTE_NUM_MAX > 1)
		/* If the next PDU in the chain contains no adv data, any remaining PDUs
		 *  in the chain are only present for CTE purposes
		 */
		if (pdu_chain && pdu_chain->len == pdu_chain->adv_ext_ind.ext_hdr_len + 1U) {
			/* Remove AuxPtr and clean up remaining PDUs in chain */
			remove_fields.aux_ptr = 1U;
			ull_adv_sync_remove_from_header(pdu, &remove_fields, false);
			lll_adv_pdu_linked_release_all(pdu_chain);
			lll_adv_pdu_linked_append(NULL, pdu);
			pdu_chain = NULL;
		}
#endif /* CONFIG_BT_CTLR_DF_PER_ADV_CTE_NUM_MAX > 1 */
		pdu = pdu_chain;
	}
#else /* !CONFIG_BT_CTLR_ADV_SYNC_PDU_LINK */
	ull_adv_sync_remove_from_header(pdu, &remove_fields, false);
#endif /* !CONFIG_BT_CTLR_ADV_SYNC_PDU_LINK */

	return 0U;
}
#endif /* CONFIG_BT_CTLR_DF_ADV_CTE_TX */

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
 * @p hdr_data content depends on the flag provided by @p hdr_add_fields:
 * - ULL_ADV_PDU_HDR_FIELD_CTE_INFO:
 *   # @p hdr_data points to single byte with CTEInfo field
 *
 * @return Zero in case of success, other value in case of failure.
 * @p data depends on the flag provided by @p hdr_add_fields.
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
		(void)memcpy(extra_data_new, data, sizeof(struct lll_df_adv_cfg));
	} else if (!(hdr_rem_fields & ULL_ADV_PDU_HDR_FIELD_CTE_INFO) ||
		   extra_data_prev) {
		(void)memmove(extra_data_new, extra_data_prev,
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

static uint8_t adv_type_check(struct ll_adv_set *adv)
{
	struct pdu_adv_com_ext_adv *pri_com_hdr;
	struct pdu_adv_ext_hdr *pri_hdr;
	struct pdu_adv *pri_pdu;

	pri_pdu = lll_adv_data_latest_peek(&adv->lll);
	if (pri_pdu->type != PDU_ADV_TYPE_EXT_IND) {
		return BT_HCI_ERR_INVALID_PARAM;
	}

	pri_com_hdr = (void *)&pri_pdu->adv_ext_ind;
	if (pri_com_hdr->adv_mode != 0U) {
		return BT_HCI_ERR_INVALID_PARAM;
	}

	pri_hdr = (void *)pri_com_hdr->ext_hdr_adv_data;
	if (pri_hdr->aux_ptr) {
		struct pdu_adv_com_ext_adv *sec_com_hdr;
		struct pdu_adv_ext_hdr *sec_hdr;
		struct pdu_adv *sec_pdu;

		sec_pdu = lll_adv_aux_data_latest_peek(adv->lll.aux);
		sec_com_hdr = (void *)&sec_pdu->adv_ext_ind;
		sec_hdr = (void *)sec_com_hdr->ext_hdr_adv_data;
		if (!pri_hdr->adv_addr && !sec_hdr->adv_addr) {
			return BT_HCI_ERR_INVALID_PARAM;
		}
	} else if (!pri_hdr->adv_addr) {
		return BT_HCI_ERR_INVALID_PARAM;
	}

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

static inline uint16_t sync_handle_get(const struct ll_adv_sync_set *sync)
{
	return mem_index_get(sync, ll_adv_sync_pool,
			     sizeof(struct ll_adv_sync_set));
}

static uint32_t sync_time_get(const struct ll_adv_sync_set *sync,
			      const struct pdu_adv *pdu)
{
	uint8_t len;

	/* Calculate the PDU Tx Time and hence the radio event length,
	 * Always use maximum length for common extended header format so that
	 * ACAD could be update when periodic advertising is active and the
	 * time reservation need not be updated every time avoiding overlapping
	 * with other active states/roles.
	 */
	len = pdu->len - pdu->adv_ext_ind.ext_hdr_len -
	      PDU_AC_EXT_HEADER_SIZE_MIN + PDU_AC_EXT_HEADER_SIZE_MAX;

	return ull_adv_sync_time_get(sync, len);
}

static uint8_t sync_stop(struct ll_adv_sync_set *sync)
{
	uint8_t sync_handle;
	int err;

	sync_handle = sync_handle_get(sync);

	err = ull_ticker_stop_with_mark(TICKER_ID_ADV_SYNC_BASE + sync_handle,
					sync, &sync->lll);
	LL_ASSERT_INFO2(err == 0 || err == -EALREADY, sync_handle, err);
	if (err) {
		return BT_HCI_ERR_CMD_DISALLOWED;
	}

	return 0;
}

static inline uint8_t sync_remove(struct ll_adv_sync_set *sync,
				  struct ll_adv_set *adv, uint8_t enable)
{
	uint8_t pri_idx;
	uint8_t sec_idx;
	uint8_t err;

	/* Remove sync_info from auxiliary PDU */
	err = ull_adv_aux_hdr_set_clear(adv, 0U,
					ULL_ADV_PDU_HDR_FIELD_SYNC_INFO, NULL,
					&pri_idx, &sec_idx);
	if (err) {
		return err;
	}

	lll_adv_aux_data_enqueue(adv->lll.aux, sec_idx);
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

#if defined(CONFIG_BT_TICKER_EXT_EXPIRE_INFO)
		if (adv->lll.aux) {
			/* notify the auxiliary set */
			ull_adv_sync_started_stopped(HDR_LLL2ULL(adv->lll.aux));
		}
#endif /* CONFIG_BT_TICKER_EXT_EXPIRE_INFO */
	}

	if (!enable) {
		sync->is_enabled = 0U;
	}

	return 0U;
}

static uint8_t sync_chm_update(uint8_t handle)
{
	struct pdu_adv_sync_chm_upd_ind *chm_upd_ind;
	uint8_t ad[sizeof(*chm_upd_ind) + 2U];
	struct lll_adv_sync *lll_sync;
	struct pdu_adv *pdu_prev;
	struct ll_adv_set *adv;
	struct pdu_adv *pdu;
	uint16_t instant;
	uint8_t chm_last;
	uint8_t ter_idx;
	uint8_t err;

	/* Check for valid advertising instance */
	adv = ull_adv_is_created_get(handle);
	if (!adv) {
		return BT_HCI_ERR_UNKNOWN_ADV_IDENTIFIER;
	}

	/* Check for valid periodic advertising */
	lll_sync = adv->lll.sync;
	if (!lll_sync) {
		return BT_HCI_ERR_UNKNOWN_ADV_IDENTIFIER;
	}

	/* Fail if already in progress */
	if (lll_sync->chm_last != lll_sync->chm_first) {
		return BT_HCI_ERR_CMD_DISALLOWED;
	}

	/* Allocate next Sync PDU */
	err = ull_adv_sync_pdu_alloc(adv, ULL_ADV_PDU_EXTRA_DATA_ALLOC_IF_EXIST,
				     &pdu_prev, &pdu, NULL, NULL, &ter_idx);
	if (err) {
		return err;
	}

	/* Populate the AD data length and opcode */
	ad[PDU_ADV_DATA_HEADER_LEN_OFFSET] = sizeof(*chm_upd_ind) + 1U;
	ad[PDU_ADV_DATA_HEADER_TYPE_OFFSET] = PDU_ADV_DATA_TYPE_CHANNEL_MAP_UPDATE_IND;

	/* Populate the Channel Map Indication structure */
	chm_upd_ind = (void *)&ad[PDU_ADV_DATA_HEADER_DATA_OFFSET];
	(void)ull_chan_map_get(chm_upd_ind->chm);
	instant = lll_sync->event_counter + 6U;
	chm_upd_ind->instant = sys_cpu_to_le16(instant);

	/* Try to add channel map update indication to ACAD */
	err = ull_adv_sync_add_to_acad(lll_sync, pdu_prev, pdu, ad, sizeof(*chm_upd_ind) + 2U);
	if (err) {
		return err;
	}

	/* Update the LLL to reflect the Channel Map and Instant to use */
	chm_last = lll_sync->chm_last + 1U;
	if (chm_last == DOUBLE_BUFFER_SIZE) {
		chm_last = 0U;
	}
	lll_sync->chm[chm_last].data_chan_count =
		ull_chan_map_get(lll_sync->chm[chm_last].data_chan_map);
	lll_sync->chm_instant = instant;

	/* Commit the Channel Map Indication in the ACAD field of Periodic
	 * Advertising
	 */
	lll_adv_sync_data_enqueue(lll_sync, ter_idx);

	/* Initiate the Channel Map Indication */
	lll_sync->chm_last = chm_last;

#if defined(CONFIG_BT_TICKER_EXT_EXPIRE_INFO)
	struct ll_adv_sync_set *sync = HDR_LLL2ULL(lll_sync);

	if (!sync->is_started) {
		/* Sync not started yet, apply new channel map now */
		lll_sync->chm_first = lll_sync->chm_last;
	}
#endif /* CONFIG_BT_TICKER_EXT_EXPIRE_INFO */

	return 0;
}

#if defined(CONFIG_BT_TICKER_EXT_EXPIRE_INFO)
void ull_adv_sync_lll_syncinfo_fill(struct pdu_adv *pdu, struct lll_adv_aux *lll_aux)
{
	struct lll_adv_sync *lll_sync;
	struct pdu_adv_sync_info *si;
	uint8_t chm_first;

	lll_sync = lll_aux->adv->sync;

	si = sync_info_get(pdu);
	sync_info_offset_fill(si, lll_sync->us_adv_sync_pdu_offset);
	si->evt_cntr = lll_sync->event_counter + lll_sync->latency_prepare +
		       lll_sync->sync_lazy;

	/* Fill the correct channel map to use if at or past the instant */
	if (lll_sync->chm_first != lll_sync->chm_last) {
		uint16_t instant_latency;

		instant_latency = (si->evt_cntr - lll_sync->chm_instant) &
				  EVENT_INSTANT_MAX;
		if (instant_latency <= EVENT_INSTANT_LATENCY_MAX) {
			chm_first = lll_sync->chm_last;
		} else {
			chm_first = lll_sync->chm_first;
		}
	} else {
		chm_first = lll_sync->chm_first;
	}
	(void)memcpy(si->sca_chm, lll_sync->chm[chm_first].data_chan_map,
		     sizeof(si->sca_chm));
	si->sca_chm[PDU_SYNC_INFO_SCA_CHM_SCA_BYTE_OFFSET] &=
		~PDU_SYNC_INFO_SCA_CHM_SCA_BIT_MASK;
	si->sca_chm[PDU_SYNC_INFO_SCA_CHM_SCA_BYTE_OFFSET] |=
		((lll_clock_sca_local_get() <<
		  PDU_SYNC_INFO_SCA_CHM_SCA_BIT_POS) &
		 PDU_SYNC_INFO_SCA_CHM_SCA_BIT_MASK);
}

static void sync_info_offset_fill(struct pdu_adv_sync_info *si, uint32_t offs)
{
	uint8_t offs_adjust = 0U;

	if (offs >= OFFS_ADJUST_US) {
		offs -= OFFS_ADJUST_US;
		offs_adjust = 1U;
	}

	offs = offs / OFFS_UNIT_30_US;
	if (!!(offs >> OFFS_UNIT_BITS)) {
		PDU_ADV_SYNC_INFO_OFFS_SET(si, offs / (OFFS_UNIT_300_US / OFFS_UNIT_30_US),
					   OFFS_UNIT_VALUE_300_US, offs_adjust);
	} else {
		PDU_ADV_SYNC_INFO_OFFS_SET(si, offs, OFFS_UNIT_VALUE_30_US, offs_adjust);
	}
}

#else /* !CONFIG_BT_TICKER_EXT_EXPIRE_INFO */
static void mfy_sync_offset_get(void *param)
{
	struct ll_adv_set *adv = param;
	struct lll_adv_sync *lll_sync;
	struct ll_adv_sync_set *sync;
	struct pdu_adv_sync_info *si;
	uint32_t sync_remainder_us;
	uint32_t aux_remainder_us;
	uint32_t ticks_to_expire;
	uint32_t ticks_current;
	struct pdu_adv *pdu;
	uint32_t remainder;
	uint8_t chm_first;
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
					       &ticks_to_expire, &remainder,
					       &lazy, NULL, NULL,
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

	/* Reduced a tick for negative remainder and return positive remainder
	 * value.
	 */
	hal_ticker_remove_jitter(&ticks_to_expire, &remainder);
	sync_remainder_us = remainder;

	/* Add a tick for negative remainder and return positive remainder
	 * value.
	 */
	remainder = sync->aux_remainder;
	hal_ticker_add_jitter(&ticks_to_expire, &remainder);
	aux_remainder_us = remainder;

	pdu = lll_adv_aux_data_latest_peek(adv->lll.aux);
	si = sync_info_get(pdu);
	sync_info_offset_fill(si, ticks_to_expire, sync_remainder_us,
			      aux_remainder_us);
	si->evt_cntr = lll_sync->event_counter + lll_sync->latency_prepare +
		       lazy;

	/* Fill the correct channel map to use if at or past the instant */
	if (lll_sync->chm_first != lll_sync->chm_last) {
		uint16_t instant_latency;

		instant_latency = (si->evt_cntr - lll_sync->chm_instant) &
				  EVENT_INSTANT_MAX;
		if (instant_latency <= EVENT_INSTANT_LATENCY_MAX) {
			chm_first = lll_sync->chm_last;
		} else {
			chm_first = lll_sync->chm_first;
		}
	} else {
		chm_first = lll_sync->chm_first;
	}
	(void)memcpy(si->sca_chm, lll_sync->chm[chm_first].data_chan_map,
		     sizeof(si->sca_chm));
	si->sca_chm[PDU_SYNC_INFO_SCA_CHM_SCA_BYTE_OFFSET] &=
		~PDU_SYNC_INFO_SCA_CHM_SCA_BIT_MASK;
	si->sca_chm[PDU_SYNC_INFO_SCA_CHM_SCA_BYTE_OFFSET] |=
		((lll_clock_sca_local_get() <<
		  PDU_SYNC_INFO_SCA_CHM_SCA_BIT_POS) &
		 PDU_SYNC_INFO_SCA_CHM_SCA_BIT_MASK);
}

static void sync_info_offset_fill(struct pdu_adv_sync_info *si,
				  uint32_t ticks_offset,
				  uint32_t remainder_us,
				  uint32_t start_us)
{
	uint8_t offs_adjust = 0U;
	uint32_t offs;

	offs = HAL_TICKER_TICKS_TO_US(ticks_offset) + remainder_us - start_us;

	if (offs >= OFFS_ADJUST_US) {
		offs -= OFFS_ADJUST_US;
		offs_adjust = 1U;
	}

	offs = offs / OFFS_UNIT_30_US;
	if (!!(offs >> OFFS_UNIT_BITS)) {
		PDU_ADV_SYNC_INFO_OFFS_SET(si, offs / (OFFS_UNIT_300_US / OFFS_UNIT_30_US),
					   OFFS_UNIT_VALUE_300_US, offs_adjust);
	} else {
		PDU_ADV_SYNC_INFO_OFFS_SET(si, offs, OFFS_UNIT_VALUE_30_US, offs_adjust);
	}
}

static void ticker_op_cb(uint32_t status, void *param)
{
	*((uint32_t volatile *)param) = status;
}
#endif /* !CONFIG_BT_TICKER_EXT_EXPIRE_INFO */

static struct pdu_adv_sync_info *sync_info_get(struct pdu_adv *pdu)
{
	struct pdu_adv_com_ext_adv *p;
	struct pdu_adv_ext_hdr *h;
	uint8_t *ptr;

	p = (void *)&pdu->adv_ext_ind;
	h = (void *)p->ext_hdr_adv_data;
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

	/* traverse through aux ptr, if present */
	if (h->aux_ptr) {
		ptr += sizeof(struct pdu_adv_aux_ptr);
	}

	/* return pointer offset to sync_info */
	return (void *)ptr;
}

static void ticker_cb(uint32_t ticks_at_expire, uint32_t ticks_drift,
		      uint32_t remainder, uint16_t lazy, uint8_t force,
		      void *param)
{
	static memq_link_t link;
	static struct mayfly mfy = {0, 0, &link, NULL, lll_adv_sync_prepare};
	static struct lll_prepare_param p;
#if defined(CONFIG_BT_CTLR_ADV_ISO) && \
	defined(CONFIG_BT_TICKER_EXT_EXPIRE_INFO)
	struct ticker_ext_context *context = param;
	struct ll_adv_sync_set *sync = context->context;
#else /* !CONFIG_BT_CTLR_ADV_ISO || !CONFIG_BT_TICKER_EXT_EXPIRE_INFO */
	struct ll_adv_sync_set *sync = param;
#endif /* !CONFIG_BT_CTLR_ADV_ISO || !CONFIG_BT_TICKER_EXT_EXPIRE_INFO */
	struct lll_adv_sync *lll;
	uint32_t ret;
	uint8_t ref;

	DEBUG_RADIO_PREPARE_A(1);

	lll = &sync->lll;

	/* Increment prepare reference count */
	ref = ull_ref_inc(&sync->ull);
	LL_ASSERT(ref);

#if defined(CONFIG_BT_CTLR_ADV_ISO) && \
	defined(CONFIG_BT_TICKER_EXT_EXPIRE_INFO)
	if (lll->iso) {
		struct lll_adv_iso *lll_iso = lll->iso;

		LL_ASSERT(context->other_expire_info);

		/* Check: No need for remainder in this case? */
		lll_iso->ticks_sync_pdu_offset = context->other_expire_info->ticks_to_expire;
		lll_iso->iso_lazy = context->other_expire_info->lazy;
	}
#endif /* CONFIG_BT_CTLR_ADV_ISO && CONFIG_BT_TICKER_EXT_EXPIRE_INFO */

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

#if defined(CONFIG_BT_CTLR_ADV_ISO) && \
	!defined(CONFIG_BT_TICKER_EXT_EXPIRE_INFO)
	if (lll->iso) {
		ull_adv_iso_offset_get(sync);
	}
#endif /* CONFIG_BT_CTLR_ADV_ISO && !CONFIG_BT_TICKER_EXT_EXPIRE_INFO */

	DEBUG_RADIO_PREPARE_A(1);
}

#if defined(CONFIG_BT_CTLR_ADV_ISO) && \
	defined(CONFIG_BT_TICKER_EXT_EXPIRE_INFO)
static void ticker_update_op_cb(uint32_t status, void *param)
{
	LL_ASSERT(status == TICKER_STATUS_SUCCESS ||
		  param == ull_disable_mark_get());
}
#endif /* !CONFIG_BT_CTLR_ADV_ISO && CONFIG_BT_TICKER_EXT_EXPIRE_INFO */
