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
#include "ull_adv_internal.h"

#include "ll.h"

#define BT_DBG_ENABLED IS_ENABLED(CONFIG_BT_DEBUG_HCI_DRIVER)
#define LOG_MODULE_NAME bt_ctlr_ull_adv_sync
#include "common/log.h"
#include "hal/debug.h"

static int init_reset(void);
static uint8_t adv_type_check(struct ll_adv_set *adv);
static inline struct ll_adv_sync_set *sync_acquire(void);
static inline void sync_release(struct ll_adv_sync_set *sync);
static inline uint16_t sync_handle_get(struct ll_adv_sync_set *sync);
static inline uint8_t sync_remove(struct ll_adv_sync_set *sync,
				  struct ll_adv_set *adv, uint8_t enable);
static uint8_t sync_chm_update(uint8_t handle);

static void mfy_sync_offset_get(void *param);
static inline struct pdu_adv_sync_info *sync_info_get(struct pdu_adv *pdu);
static inline void sync_info_offset_fill(struct pdu_adv_sync_info *si,
					 uint32_t ticks_offset,
					 uint32_t remainder_us,
					 uint32_t start_us);
static void ticker_cb(uint32_t ticks_at_expire, uint32_t ticks_drift,
		      uint32_t remainder, uint16_t lazy, uint8_t force,
		      void *param);
static void ticker_op_cb(uint32_t status, void *param);

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
		uint8_t err;

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

#if defined(CONFIG_BT_CTLR_ADV_SYNC_PDU_LINK)
	/* Duplicate chain PDUs */
	do {
		struct pdu_adv *pdu_chain;

#endif /* CONFIG_BT_CTLR_ADV_SYNC_PDU_LINK */
		err = ull_adv_sync_pdu_set_clear(lll_sync, pdu_prev, pdu,
						 0U, 0U, NULL);
		if (err) {
			return err;
		}

#if defined(CONFIG_BT_CTLR_ADV_SYNC_PDU_LINK)
		pdu_prev = lll_adv_pdu_linked_next_get(pdu_prev);
		pdu_chain = lll_adv_pdu_linked_next_get(pdu);

		/* Allocate new chain PDU if required */
		if (pdu_prev) {
			/* Prior PDU chain allocation valid */
			if (pdu_chain) {
				pdu = pdu_chain;

				continue;
			}

			/* Get a new chain PDU */
			pdu_chain = lll_adv_pdu_alloc_pdu_adv();
			if (!pdu_chain) {
				return BT_HCI_ERR_INSUFFICIENT_RESOURCES;
			}

			/* Link the chain PDU to parent PDU */
			lll_adv_pdu_linked_append(pdu_chain, pdu);

			/* continue back to update the new PDU */
			pdu = pdu_chain;
		}
	} while (pdu_prev);
#endif /* CONFIG_BT_CTLR_ADV_SYNC_PDU_LINK */

	lll_adv_sync_data_enqueue(lll_sync, ter_idx);

	sync->is_data_cmplt = 1U;

	return 0;
}

uint8_t ll_adv_sync_ad_data_set(uint8_t handle, uint8_t op, uint8_t len,
				uint8_t const *const data)
{
	uint8_t hdr_data[ULL_ADV_HDR_DATA_LEN_SIZE +
			 ULL_ADV_HDR_DATA_DATA_PTR_SIZE +
			 ULL_ADV_HDR_DATA_LEN_SIZE +
			 ULL_ADV_HDR_DATA_AUX_PTR_PTR_SIZE +
			 ULL_ADV_HDR_DATA_LEN_SIZE];
	void *extra_data_prev, *extra_data;
	struct pdu_adv *pdu_prev, *pdu;
	struct lll_adv_sync *lll_sync;
	struct ll_adv_sync_set *sync;
	struct ll_adv_set *adv;
	uint8_t *val_ptr;
	uint8_t ter_idx;
	uint8_t err;

#if defined(CONFIG_BT_CTLR_ADV_SYNC_PDU_LINK)
	uint8_t ad_len_overflow;
	uint8_t ad_len_chain;
#endif /* CONFIG_BT_CTLR_ADV_SYNC_PDU_LINK */

	/* Check for valid advertising set */
	adv =  ull_adv_is_created_get(handle);
	if (!adv) {
		return BT_HCI_ERR_UNKNOWN_ADV_IDENTIFIER;
	}

	/* Check for advertising set type */
	if (IS_ENABLED(CONFIG_BT_CTLR_PARAM_CHECK)) {
		uint8_t err;

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

	/* Reject first, intermediate, last operation and len > 191 bytes if
	 * chain PDUs unsupported.
	 */
	if (!IS_ENABLED(CONFIG_BT_CTLR_ADV_SYNC_PDU_LINK) &&
	    ((op < BT_HCI_LE_EXT_ADV_OP_COMPLETE_DATA) ||
	     (len > PDU_AC_EXT_AD_DATA_LEN_MAX))) {
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

	/* Prepare the AD data as parameter to update in PDU */
	/* Use length = 0 and NULL pointer to retain old data in the PDU.
	 * Use length = 0 and valid pointer of `data` (auto/local variable) to
	 * remove old data.
	 * User length > 0 and valid pointer of `data` (auto/local variable) to
	 * set new data.
	 */
	val_ptr = hdr_data;
	if (op == BT_HCI_LE_EXT_ADV_OP_INTERM_FRAG ||
	    op == BT_HCI_LE_EXT_ADV_OP_LAST_FRAG ||
	    op == BT_HCI_LE_EXT_ADV_OP_UNCHANGED_DATA) {
		*val_ptr++ = 0U;
		(void)memset((void *)val_ptr, 0U,
			     ULL_ADV_HDR_DATA_DATA_PTR_SIZE);
	} else {
		*val_ptr++ = len;
		(void)memcpy(val_ptr, &data, sizeof(data));
	}

	if (!IS_ENABLED(CONFIG_BT_CTLR_ADV_SYNC_PDU_LINK) ||
	    (op == BT_HCI_LE_EXT_ADV_OP_UNCHANGED_DATA)) {
		err = ull_adv_sync_pdu_set_clear(lll_sync, pdu_prev, pdu,
						 ULL_ADV_PDU_HDR_FIELD_AD_DATA,
						 0U, hdr_data);
#if defined(CONFIG_BT_CTLR_ADV_SYNC_PDU_LINK)
		/* No AD data overflow */
		ad_len_overflow = 0U;
#endif /* CONFIG_BT_CTLR_ADV_SYNC_PDU_LINK */
	} else if (!IS_ENABLED(CONFIG_BT_CTLR_ADV_SYNC_PDU_LINK) ||
		   (op == BT_HCI_LE_EXT_ADV_OP_FIRST_FRAG ||
		    op == BT_HCI_LE_EXT_ADV_OP_COMPLETE_DATA)) {
		/* Add AD Data and remove any prior presence of Aux Ptr */
		err = ull_adv_sync_pdu_set_clear(lll_sync, pdu_prev, pdu,
						 ULL_ADV_PDU_HDR_FIELD_AD_DATA,
						 ULL_ADV_PDU_HDR_FIELD_AUX_PTR,
						 hdr_data);
#if defined(CONFIG_BT_CTLR_ADV_SYNC_PDU_LINK)
		if (!err) {
			/* Fragment into chain PDU if len > 191 bytes */
			if (len > PDU_AC_EXT_AD_DATA_LEN_MAX) {
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
			} else {
				struct pdu_adv *pdu_chain;

				/* Remove/Release any previous chain PDU
				 * allocations
				 */
				pdu_chain = lll_adv_pdu_linked_next_get(pdu);
				if (pdu_chain) {
					lll_adv_pdu_linked_append(NULL, pdu);
					lll_adv_pdu_linked_release_all(pdu_chain);
				}

				/* No AD data overflow */
				ad_len_overflow = 0U;
			}
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
			err = ull_adv_sync_pdu_set_clear(lll_sync, pdu_prev,
					pdu, (ULL_ADV_PDU_HDR_FIELD_AD_DATA |
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
					lll_adv_sync_data_enqueue(lll_sync, ter_idx);
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
			ad_len_overflow = 0U;
		}
#endif /* CONFIG_BT_CTLR_ADV_SYNC_PDU_LINK */
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
	     (hdr_data[ULL_ADV_HDR_DATA_LEN_OFFSET] == 0U) ||
	     (len != 0U))) {
		/* NOTE: latest PDU was not consumed by LLL and as
		 * ull_adv_sync_pdu_alloc() has reverted back the double buffer
		 * with the first PDU, and returned the latest PDU as the new
		 * PDU, we need to enqueue back the new PDU which is infact
		 * the latest PDU.
		 */
		if (pdu_prev == pdu) {
			lll_adv_sync_data_enqueue(lll_sync, ter_idx);
		}

		return BT_HCI_ERR_INVALID_PARAM;
	}

#if defined(CONFIG_BT_CTLR_ADV_SYNC_PDU_LINK)
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
		uint16_t ter_len;
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

		/* Initialize Flags */
		*dptr_chain = 0U;

		/* No CTE Info.
		 * CTE count is given by HCI LE Set Connectionless CTE Transmit
		 * Parameters, hence it is not altered due to change on PDUs
		 * count in Periodic Advertising chain.
		 */

		/* ADI flag, mandatory if superior PDU has it */
		if (hdr.adi) {
			hdr_chain->adi = 1U;
		}

		/* Proceed to next byte if any flags present */
		if (*dptr_chain) {
			dptr_chain++;
		}

		/* Start adding fields corresponding to flags here, if any */

		/* No AdvA */
		/* No TgtA */
		/* No CTEInfo */

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
		ter_len = ull_adv_aux_hdr_len_calc(com_hdr_chain, &dptr_chain);

		/* Prefix overflowed data to chain PDU and reduce the AD data in
		 * in the current PDU.
		 */
		if (ad_len_overflow) {
			uint8_t *ad_overflow;

			/* Copy overflowed AD data from previous PDU into
			 * new chain PDU
			 */
			(void)memcpy(&ad_overflow,
				     &val_ptr[ULL_ADV_HDR_DATA_DATA_PTR_OFFSET],
				     sizeof(ad_overflow));
			ad_overflow += *val_ptr;
			(void)memcpy(dptr_chain, ad_overflow, ad_len_overflow);
			dptr_chain += ad_len_overflow;

			/* Reduce the AD data in the current PDU that will
			 * become the current parent PDU for the new chain PDU.
			 */
			err = ull_adv_sync_pdu_set_clear(lll_sync, pdu_prev, pdu,
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
					lll_adv_sync_data_enqueue(lll_sync, ter_idx);
				}

				return err;
			}

			/* AD data len in chain PDU besides the overflow */
			len = ad_len_chain;
		}

		/* Check AdvData overflow */
		if ((ter_len + ad_len_overflow + len) >
		    PDU_AC_PAYLOAD_SIZE_MAX) {
			/* NOTE: latest PDU was not consumed by LLL and
			 * as ull_adv_sync_pdu_alloc() has reverted back
			 * the double buffer with the first PDU, and
			 * returned the latest PDU as the new PDU, we
			 * need to enqueue back the new PDU which is
			 * infact the latest PDU.
			 */
			if (pdu_prev == pdu) {
				lll_adv_sync_data_enqueue(lll_sync, ter_idx);
			}

			return BT_HCI_ERR_PACKET_TOO_LONG;
		}

		/* Fill the chain PDU length */
		ull_adv_aux_hdr_len_fill(com_hdr_chain, ter_len);
		pdu_chain->len = ter_len + ad_len_overflow + len;

		/* Fill AD Data in chain PDU */
		(void)memcpy(dptr_chain, data, len);

		/* Get reference to aux ptr in superior PDU */
		(void)memcpy(&aux_ptr,
			     &hdr_data[ULL_ADV_HDR_DATA_AUX_PTR_PTR_OFFSET],
			     sizeof(aux_ptr));

		/* Fill the aux offset in the previous AUX_SYNC_IND PDU */
		offs_us = PDU_AC_US(pdu->len, adv->lll.phy_s,
				    adv->lll.phy_flags) +
			  EVENT_SYNC_B2B_MAFS_US;
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
#endif /* CONFIG_BT_CTLR_ADV_SYNC_PDU_LINK */

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
	void *extra_data_prev, *extra_data;
	struct pdu_adv *pdu_prev, *pdu;
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
		uint8_t err;

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

	/* Add/Remove ADI */
	if (IS_ENABLED(CONFIG_BT_CTLR_ADV_PERIODIC_ADI_SUPPORT)) {
		uint8_t hdr_data[ULL_ADV_HDR_DATA_LEN_SIZE +
				 ULL_ADV_HDR_DATA_ADI_PTR_SIZE] = {0, };
		uint16_t hdr_add_fields;
		uint16_t hdr_rem_fields;

		if (enable & BT_HCI_LE_SET_PER_ADV_ENABLE_ADI) {
			hdr_add_fields = ULL_ADV_PDU_HDR_FIELD_ADI;
			hdr_rem_fields = 0U;
		} else {
			hdr_add_fields = 0U;
			hdr_rem_fields = ULL_ADV_PDU_HDR_FIELD_ADI;
		}

		err = ull_adv_sync_pdu_alloc(adv, ULL_ADV_PDU_EXTRA_DATA_ALLOC_IF_EXIST,
					     &pdu_prev, &pdu, &extra_data_prev,
					     &extra_data, &ter_idx);
		if (err) {
			return err;
		}

#if defined(CONFIG_BT_CTLR_DF_ADV_CTE_TX)
		if (extra_data) {
			ull_adv_sync_extra_data_set_clear(extra_data_prev,
							  extra_data, 0U, 0U,
							  NULL);
		}
#endif /* CONFIG_BT_CTLR_DF_ADV_CTE_TX */

#if defined(CONFIG_BT_CTLR_ADV_SYNC_PDU_LINK)
		/* Update ADI while duplicating chain PDUs */
		do {
			struct pdu_adv *pdu_chain;

#endif /* CONFIG_BT_CTLR_ADV_SYNC_PDU_LINK */
			err = ull_adv_sync_pdu_set_clear(lll_sync, pdu_prev,
							 pdu, hdr_add_fields,
							 hdr_rem_fields,
							 hdr_data);
			if (err) {
				return err;
			}

#if defined(CONFIG_BT_CTLR_ADV_SYNC_PDU_LINK)
			pdu_prev = lll_adv_pdu_linked_next_get(pdu_prev);
			pdu_chain = lll_adv_pdu_linked_next_get(pdu);

			/* Allocate new chain PDU if required */
			if (pdu_prev) {
				/* Prior PDU chain allocation valid */
				if (pdu_chain) {
					pdu = pdu_chain;

					continue;
				}

				/* Get a new chain PDU */
				pdu_chain = lll_adv_pdu_alloc_pdu_adv();
				if (!pdu_chain) {
					return BT_HCI_ERR_INSUFFICIENT_RESOURCES;
				}

				/* Link the chain PDU to parent PDU */
				lll_adv_pdu_linked_append(pdu_chain, pdu);

				/* continue back to update the new PDU */
				pdu = pdu_chain;
			}
		} while (pdu_prev);
#endif /* CONFIG_BT_CTLR_ADV_SYNC_PDU_LINK */
	}

	/* Start Periodic Advertising events if Extended Advertising events are
	 * active.
	 */
	if (adv->is_enabled && !sync->is_started) {
		struct pdu_adv_sync_info *sync_info;
		uint8_t value[1 + sizeof(sync_info)];
		uint32_t ticks_slot_overhead_aux;
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

		if (lll_aux) {
			/* Auxiliary set already active (due to other fields
			 * being already present or being started prior).
			 */
			aux = NULL;
			ticks_anchor_aux = 0U; /* unused in this path */
			ticks_slot_overhead_aux = 0U; /* unused in this path */

			/* TODO: Find the anchor after the group of active
			 *       auxiliary sets such that Periodic Advertising
			 *       events are placed in non-overlapping timeline
			 *       when auxiliary and Periodic Advertising have
			 *       similar event interval.
			 */
			ticks_anchor_sync =
				ticker_ticks_now_get() +
				HAL_TICKER_US_TO_TICKS(EVENT_OVERHEAD_START_US);
		} else {
			/* Auxiliary set will be started due to inclusion of
			 * sync info field.
			 */
			lll_aux = adv->lll.aux;
			aux = HDR_LLL2ULL(lll_aux);
			ticks_anchor_aux =
				ticker_ticks_now_get() +
				HAL_TICKER_US_TO_TICKS(EVENT_OVERHEAD_START_US);
			ticks_slot_overhead_aux =
				ull_adv_aux_evt_init(aux, &ticks_anchor_aux);
			ticks_anchor_sync = ticks_anchor_aux +
				ticks_slot_overhead_aux + aux->ull.ticks_slot +
				HAL_TICKER_US_TO_TICKS(
					MAX(EVENT_MAFS_US,
					    EVENT_OVERHEAD_START_US) -
					(EVENT_TICKER_RES_MARGIN_US << 1));
		}

		ret = ull_adv_sync_start(adv, sync, ticks_anchor_sync);
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

uint16_t ull_adv_sync_lll_handle_get(struct lll_adv_sync *lll)
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

uint32_t ull_adv_sync_start(struct ll_adv_set *adv,
			    struct ll_adv_sync_set *sync,
			    uint32_t ticks_anchor)
{
	struct lll_adv_sync *lll_sync;
	uint32_t ticks_slot_overhead;
	uint32_t ticks_slot_offset;
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
	time_us = ull_adv_sync_time_get(sync, ter_pdu->len);

	/* TODO: active_to_start feature port */
	sync->ull.ticks_active_to_start = 0U;
	sync->ull.ticks_prepare_to_start =
		HAL_TICKER_US_TO_TICKS(EVENT_OVERHEAD_XTAL_US);
	sync->ull.ticks_preempt_to_start =
		HAL_TICKER_US_TO_TICKS(EVENT_OVERHEAD_PREEMPT_MIN_US);
	sync->ull.ticks_slot = HAL_TICKER_US_TO_TICKS(time_us);

	ticks_slot_offset = MAX(sync->ull.ticks_active_to_start,
				sync->ull.ticks_prepare_to_start);
	if (IS_ENABLED(CONFIG_BT_CTLR_LOW_LAT)) {
		ticks_slot_overhead = ticks_slot_offset;
	} else {
		ticks_slot_overhead = 0U;
	}

	interval_us = (uint32_t)sync->interval * PERIODIC_INT_UNIT_US;

	sync_handle = sync_handle_get(sync);

	ret_cb = TICKER_STATUS_BUSY;
	ret = ticker_start(TICKER_INSTANCE_ID_CTLR, TICKER_USER_ID_THREAD,
			   (TICKER_ID_ADV_SYNC_BASE + sync_handle),
			   ticks_anchor, 0U,
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
	uint32_t time_us;
	uint32_t ret;

	time_us = ull_adv_sync_time_get(sync, pdu->len);
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

void ull_adv_sync_chm_complete(struct node_rx_hdr *rx)
{
	uint8_t hdr_data[ULL_ADV_HDR_DATA_LEN_SIZE +
			 ULL_ADV_HDR_DATA_ACAD_PTR_SIZE];
	struct lll_adv_sync *lll_sync;
	struct pdu_adv *pdu_prev;
	struct ll_adv_set *adv;
	struct pdu_adv *pdu;
	uint8_t others_len;
	uint8_t acad_len;
	uint8_t *others;
	uint8_t ter_idx;
	uint8_t ad_len;
	uint8_t *acad;
	uint8_t *ad;
	uint8_t len;
	uint8_t err;

	/* Allocate next Sync PDU */
	pdu_prev = NULL;
	pdu = NULL;
	lll_sync = rx->rx_ftr.param;
	adv = HDR_LLL2ULL(lll_sync->adv);
	err = ull_adv_sync_pdu_alloc(adv, ULL_ADV_PDU_EXTRA_DATA_ALLOC_IF_EXIST,
				     &pdu_prev, &pdu, NULL, NULL, &ter_idx);
	LL_ASSERT(!err);

	/* Get the size of current ACAD, first octet returns the old length and
	 * followed by pointer to previous offset to ACAD in the PDU.
	 */
	hdr_data[ULL_ADV_HDR_DATA_LEN_OFFSET] = 0U;
	err = ull_adv_sync_pdu_set_clear(lll_sync, pdu_prev, pdu,
					 ULL_ADV_PDU_HDR_FIELD_ACAD, 0U,
					 &hdr_data);
	LL_ASSERT(!err);

	/* Dev assert if ACAD empty */
	LL_ASSERT(hdr_data[ULL_ADV_HDR_DATA_LEN_OFFSET]);

	/* Get the pointer, prev content and size of current ACAD */
	err = ull_adv_sync_pdu_set_clear(lll_sync, pdu_prev, pdu,
					 ULL_ADV_PDU_HDR_FIELD_ACAD, 0U,
					 &hdr_data);
	LL_ASSERT(!err);

	/* Find the Channel Map Update Indication */
	acad_len = hdr_data[ULL_ADV_HDR_DATA_LEN_OFFSET];
	len = acad_len;
	(void)memcpy(&acad, &hdr_data[ULL_ADV_HDR_DATA_ACAD_PTR_OFFSET],
		     sizeof(acad));
	ad = acad;
	do {
		ad_len = ad[PDU_ADV_DATA_HEADER_LEN_OFFSET];
		if (ad_len &&
		    (ad[PDU_ADV_DATA_HEADER_TYPE_OFFSET] ==
		     BT_DATA_CHANNEL_MAP_UPDATE_IND)) {
			break;
		}

		ad_len += 1U;

		LL_ASSERT(ad_len <= len);

		ad += ad_len;
		len -= ad_len;
	} while (len);
	LL_ASSERT(len);

	/* Remove Channel Map Update Indication by moving other AD types that
	 * are after it.
	 */
	ad_len += 1U;
	others = ad + ad_len;
	acad_len -= ad_len;
	others_len = acad_len - (ad - acad);
	(void)memmove(ad, others, others_len);

	/* Adjust the next PDU for ACAD length, this is done by using the next
	 * PDU to copy ACAD into same next PDU.
	 */
	hdr_data[ULL_ADV_HDR_DATA_LEN_OFFSET] = acad_len;
	err = ull_adv_sync_pdu_set_clear(lll_sync, pdu, pdu,
					 ULL_ADV_PDU_HDR_FIELD_ACAD, 0U,
					 &hdr_data);
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
	si->offs_units = OFFS_UNIT_VALUE_30_US;
	si->offs_adjust = 0U;
	si->offs = 0U;

	/* Fill the interval, access address and CRC init */
	si->interval = sys_cpu_to_le16(sync->interval);
	lll_sync = &sync->lll;
	(void)memcpy(&si->aa, lll_sync->access_addr, sizeof(si->aa));
	(void)memcpy(si->crc_init, lll_sync->crc_init, sizeof(si->crc_init));

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
 * @param[in]  ter_pdu_         Pointer to PDU to fill fields.
 * @param[in]  hdr_add_fields   Flag with information which fields add.
 * @param[in]  hdr_rem_fields   Flag with information which fields remove.
 * @param[in]  hdr_data         Pointer to data to be added to header. Content
 *                              depends on the value of @p hdr_add_fields.
 *
 * @Note
 * @p hdr_data content depends on the flag provided by @p hdr_add_fields:
 * - ULL_ADV_PDU_HDR_FIELD_CTE_INFO:
 *   # @p hdr_data points to single byte with CTEInfo field
 * - ULL_ADV_PDU_HDR_FIELD_ADI:
 *   # @p hdr_data points to memory where first byte is size of ADI structure,
 *     following bytes are the pointer reference to the new ADI structure to be
 *     updated in the PDU.
 *     In return, the first byte returns the size of ADI structure, following
 *     bytes returns the pointer reference to ADI structure offset inside the
 *     updated current PDU.
 * - ULL_ADV_PDU_HDR_FIELD_AD_DATA:
 *   # @p hdr_data points to memory where first byte
 *     is size of advertising data, following byte is a pointer to actual
 *     advertising data.
 * - ULL_ADV_PDU_HDR_FIELD_AUX_PTR:
 *   # @p hdr_data points to memory where first byte is size of aux ptr
 *     structure, following bytes are the pointer reference to the new aux ptr
 *     structure to be updated in the PDU.
 *     In return, the first byte returns the size of aux ptr structure,
 *     following bytes returns the pointer reference to aux ptr structure offset
 *     inside the updated current PDU.
 * - ULL_ADV_PDU_HDR_FIELD_ACAD:
 *   # @p hdr_data points to memory where first byte is size of ACAD, second
 *     byte is used to return offset to ACAD field.
 *   # @p hdr_data memory returns previous ACAD length back in the first byte
 *     and offset to new ACAD in the next PDU.
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
	struct pdu_adv_ext_hdr ter_hdr = { 0 }, ter_hdr_prev = { 0 };
	struct pdu_adv_aux_ptr *aux_ptr, *aux_ptr_prev;
	uint8_t *ter_dptr, *ter_dptr_prev;
	struct pdu_adv_adi *adi;
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
	if (ter_com_hdr_prev->ext_hdr_len != 0) {
		ter_hdr_prev = ter_com_hdr_prev->ext_hdr;
	}
	ter_dptr_prev = ter_com_hdr_prev->ext_hdr.data;

	/* Set common fields in reference to new tertiary PDU data buffer */
	ter_pdu->type = ter_pdu_prev->type;
	ter_pdu->rfu = 0U;
	ter_pdu->chan_sel = 0U;

	ter_pdu->tx_addr = ter_pdu_prev->tx_addr;
	ter_pdu->rx_addr = ter_pdu_prev->rx_addr;

	/* Get common pointers from current tertiary PDU data.
	 * It is possible that the current tertiary is the same as
	 * previous one. It may happen if update periodic advertising
	 * chain in place.
	 */
	ter_com_hdr = (void *)&ter_pdu->adv_ext_ind;
	ter_com_hdr->adv_mode = ter_com_hdr_prev->adv_mode;
	ter_dptr = ter_com_hdr->ext_hdr.data;

	/* No AdvA in AUX_SYNC_IND */
	/* No TargetA in AUX_SYNC_IND */

#if defined(CONFIG_BT_CTLR_DF_ADV_CTE_TX)
	/* If requested add or update CTEInfo */
	if (hdr_add_fields & ULL_ADV_PDU_HDR_FIELD_CTE_INFO) {
		ter_hdr.cte_info = 1;
		cte_info = *(uint8_t *)hdr_data;
		hdr_data = (uint8_t *)hdr_data + 1;
		ter_dptr += sizeof(struct pdu_cte_info);
	/* If CTEInfo exists in prev and is not requested to be removed */
	} else if (!(hdr_rem_fields & ULL_ADV_PDU_HDR_FIELD_CTE_INFO) &&
		   ter_hdr_prev.cte_info) {
		ter_hdr.cte_info = 1;
		cte_info = 0U; /* value not used, will be read from prev PDU */
		ter_dptr += sizeof(struct pdu_cte_info);
	} else {
		cte_info = 0U; /* value not used */
	}

	/* If CTEInfo exists in prev PDU */
	if (ter_hdr_prev.cte_info) {
		ter_dptr_prev += sizeof(struct pdu_cte_info);
	}
#endif /* CONFIG_BT_CTLR_DF_ADV_CTE_TX */

	/* ADI */
	if (hdr_add_fields & ULL_ADV_PDU_HDR_FIELD_ADI) {
		ter_hdr.adi = 1U;
		/* return the size of ADI structure */
		*(uint8_t *)hdr_data = sizeof(struct pdu_adv_adi);
		hdr_data = (uint8_t *)hdr_data + sizeof(uint8_t);
		/* pick the reference to ADI param */
		(void)memcpy(&adi, hdr_data, sizeof(struct pdu_adv_adi *));
		/* return the pointer to ADI struct inside the PDU */
		(void)memcpy(hdr_data, &ter_dptr, sizeof(ter_dptr));
		hdr_data = (uint8_t *)hdr_data + sizeof(ter_dptr);
		ter_dptr += sizeof(struct pdu_adv_adi);
	} else if (!(hdr_rem_fields & ULL_ADV_PDU_HDR_FIELD_ADI) &&
		   ter_hdr_prev.adi) {
		ter_hdr.adi = 1U;
		adi = NULL;
		ter_dptr += sizeof(struct pdu_adv_adi);
	} else {
		adi = NULL;
	}
	if (ter_hdr_prev.adi) {
		ter_dptr_prev += sizeof(struct pdu_adv_adi);
	}

	/* AuxPtr - will be added if AUX_CHAIN_IND is required */
	if (hdr_add_fields & ULL_ADV_PDU_HDR_FIELD_AUX_PTR) {
		ter_hdr.aux_ptr = 1;
		aux_ptr_prev = NULL;
		aux_ptr = (void *)ter_dptr;

		/* return the size of aux pointer structure */
		*(uint8_t *)hdr_data = sizeof(struct pdu_adv_aux_ptr);
		hdr_data = (uint8_t *)hdr_data + sizeof(uint8_t);

		/* return the pointer to aux pointer struct inside the PDU
		 * buffer
		 */
		(void)memcpy(hdr_data, &ter_dptr, sizeof(ter_dptr));
		hdr_data = (uint8_t *)hdr_data + sizeof(ter_dptr);
	} else if (!(hdr_rem_fields & ULL_ADV_PDU_HDR_FIELD_AUX_PTR) &&
		   ter_hdr_prev.aux_ptr) {
		ter_hdr.aux_ptr = 1;
		aux_ptr_prev = (void *)ter_dptr_prev;
		aux_ptr = (void *)ter_dptr;
	} else {
		aux_ptr_prev = NULL;
		aux_ptr = NULL;
	}
	if (ter_hdr_prev.aux_ptr) {
		ter_dptr_prev += sizeof(struct pdu_adv_aux_ptr);
	}
	if (ter_hdr.aux_ptr) {
		ter_dptr += sizeof(struct pdu_adv_aux_ptr);
	}

	/* No SyncInfo in AUX_SYNC_IND */

	/* Tx Power flag */
	if (ter_hdr_prev.tx_pwr) {
		ter_dptr_prev++;

		ter_hdr.tx_pwr = 1;
		ter_dptr++;
	}

	/* Calc previous ACAD len and update PDU len */
	ter_len_prev = ter_dptr_prev - (uint8_t *)ter_com_hdr_prev;
	hdr_buf_len = ter_com_hdr_prev->ext_hdr_len +
		      PDU_AC_EXT_HEADER_SIZE_MIN;
	if (ter_len_prev <= hdr_buf_len) {
		/* There are some data, except ACAD, in extended header if ter_len_prev
		 * equals to hdr_buf_len. There is ACAD if the size of ter_len_prev
		 * is smaller than hdr_buf_len.
		 */
		acad_len_prev = hdr_buf_len - ter_len_prev;
		ter_len_prev += acad_len_prev;
		ter_dptr_prev += acad_len_prev;
	} else {
		/* There are no data in extended header, all flags are zeros. */
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
		(void)memcpy(hdr_data, &ter_dptr, sizeof(ter_dptr));
		hdr_data = (uint8_t *)hdr_data + sizeof(ter_dptr);
		ter_dptr += acad_len;
	} else if (!(hdr_rem_fields & ULL_ADV_PDU_HDR_FIELD_ACAD)) {
		acad_len = acad_len_prev;
		ter_dptr += acad_len_prev;
	} else {
		acad_len = 0U;
	}

	/* Calc current tertiary PDU len so far without AD data added */
	ter_len = ull_adv_aux_hdr_len_calc(ter_com_hdr, &ter_dptr);

	/* Get Adv data from function parameters */
	if (hdr_add_fields & ULL_ADV_PDU_HDR_FIELD_AD_DATA) {
		uint8_t ad_len_prev;

		/* remember the new ad data len */
		ad_len = *(uint8_t *)hdr_data;

		/* return prev ad data length */
		ad_len_prev = ter_pdu_prev->len - ter_len_prev;
		*(uint8_t *)hdr_data = ad_len_prev;
		hdr_data = (uint8_t *)hdr_data + sizeof(ad_len);

		/* remember the reference to new ad data */
		(void)memcpy(&ad_data, hdr_data, sizeof(ad_data));

		/* return the reference to prev ad data */
		(void)memcpy(hdr_data, &ter_dptr_prev, sizeof(ter_dptr_prev));
		hdr_data = (uint8_t *)hdr_data + sizeof(ter_dptr_prev);

		/* unchanged data */
		if (!ad_len && !ad_data) {
			ad_len = ad_len_prev;
			ad_data = ter_dptr_prev;
		}
	} else if (!(hdr_rem_fields & ULL_ADV_PDU_HDR_FIELD_AD_DATA)) {
		ad_len = ter_pdu_prev->len - ter_len_prev;
		ad_data = ter_dptr_prev;
	} else {
		ad_len = 0;
		ad_data = NULL;
	}

	/* Check Max Advertising Data Length */
	if (ad_len > CONFIG_BT_CTLR_ADV_DATA_LEN_MAX) {
		return BT_HCI_ERR_MEM_CAPACITY_EXCEEDED;
	}

	/* Check AdvData overflow */
	if ((ter_len + ad_len) > PDU_AC_PAYLOAD_SIZE_MAX) {
		/* return excess length */
		*(uint8_t *)hdr_data = ter_len + ad_len -
				       PDU_AC_PAYLOAD_SIZE_MAX;

		/* Will use packet too long error to determine fragmenting
		 * long data
		 */
		return BT_HCI_ERR_PACKET_TOO_LONG;
	}

	/* set the tertiary extended header and PDU length */
	ull_adv_aux_hdr_len_fill(ter_com_hdr, ter_len);
	ter_pdu->len = ter_len + ad_len;

	/* Start filling tertiary PDU payload based on flags from here
	 * ==============================================================
	 */

	/* Fill AdvData in tertiary PDU */
	(void)memmove(ter_dptr, ad_data, ad_len);

	/* Early exit if no flags set */
	if (!ter_com_hdr->ext_hdr_len) {
		return 0;
	}

	/* Retain ACAD in tertiary PDU */
	ter_dptr_prev -= acad_len_prev;
	if (acad_len) {
		ter_dptr -= acad_len;
		(void)memmove(ter_dptr, ter_dptr_prev, acad_len_prev);
	}

	/* Tx Power */
	if (ter_hdr.tx_pwr) {
		*--ter_dptr = *--ter_dptr_prev;
	}

	/* No SyncInfo in AUX_SYNC_IND */

	/* AuxPtr */
	if (ter_hdr_prev.aux_ptr) {
		ter_dptr_prev -= sizeof(struct pdu_adv_aux_ptr);
	}
	if (ter_hdr.aux_ptr) {
		ter_dptr -= sizeof(struct pdu_adv_aux_ptr);
	}
	if (aux_ptr_prev) {
		(void)memmove(ter_dptr, aux_ptr_prev, sizeof(*aux_ptr_prev));
	}

	/* ADI */
	if (ter_hdr_prev.adi) {
		ter_dptr_prev -= sizeof(struct pdu_adv_adi);
	}
	if (ter_hdr.adi) {
		struct pdu_adv_adi *adi_pdu;

		ter_dptr -= sizeof(struct pdu_adv_adi);
		adi_pdu = (void *)ter_dptr;

		if (!adi) {
			struct ll_adv_set *adv;

			adv = HDR_LLL2ULL(lll_sync->adv);

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
	if (ter_hdr.cte_info) {
		if (hdr_add_fields & ULL_ADV_PDU_HDR_FIELD_CTE_INFO) {
			*--ter_dptr = cte_info;
		} else {
			*--ter_dptr = *--ter_dptr_prev;
		}
	}
#endif /* CONFIG_BT_CTLR_DF_ADV_CTE_TX */

	/* No TargetA in AUX_SYNC_IND */
	/* No AdvA in AUX_SYNC_IND */

	if (ter_com_hdr->ext_hdr_len != 0) {
		ter_com_hdr->ext_hdr = ter_hdr;
	}

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
	}

	if (!enable) {
		sync->is_enabled = 0U;
	}

	return 0U;
}

static uint8_t sync_chm_update(uint8_t handle)
{
	uint8_t hdr_data[ULL_ADV_HDR_DATA_LEN_SIZE +
			 ULL_ADV_HDR_DATA_ACAD_PTR_SIZE];
	struct pdu_adv_sync_chm_upd_ind *chm_upd_ind;
	struct lll_adv_sync *lll_sync;
	struct pdu_adv *pdu_prev;
	struct ll_adv_set *adv;
	uint8_t acad_len_prev;
	struct pdu_adv *pdu;
	uint16_t instant;
	uint8_t chm_last;
	uint8_t ter_idx;
	uint8_t *acad;
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

	/* Try to allocate ACAD for channel map update indication, previous
	 * ACAD length with be returned back.
	 */
	hdr_data[ULL_ADV_HDR_DATA_LEN_OFFSET] = sizeof(*chm_upd_ind) + 2U;
	err = ull_adv_sync_pdu_set_clear(lll_sync, pdu_prev, pdu,
					 ULL_ADV_PDU_HDR_FIELD_ACAD, 0U,
					 &hdr_data);
	if (err) {
		return err;
	}

	/* Check if there are other ACAD data previously */
	acad_len_prev = hdr_data[ULL_ADV_HDR_DATA_LEN_OFFSET];
	if (acad_len_prev) {
		/* Append to end of other ACAD already present */
		hdr_data[ULL_ADV_HDR_DATA_LEN_OFFSET] = acad_len_prev +
							sizeof(*chm_upd_ind) +
							2U;

		err = ull_adv_sync_pdu_set_clear(lll_sync, pdu_prev, pdu,
						 ULL_ADV_PDU_HDR_FIELD_ACAD, 0U,
						 &hdr_data);
		if (err) {
			return err;
		}
	}

	/* Populate the AD data length and opcode */
	(void)memcpy(&acad, &hdr_data[ULL_ADV_HDR_DATA_ACAD_PTR_OFFSET],
		     sizeof(acad));
	acad += acad_len_prev;
	acad[PDU_ADV_DATA_HEADER_LEN_OFFSET] = sizeof(*chm_upd_ind) + 1U;
	acad[PDU_ADV_DATA_HEADER_TYPE_OFFSET] = BT_DATA_CHANNEL_MAP_UPDATE_IND;

	/* Populate the Channel Map Indication structure */
	chm_upd_ind = (void *)&acad[PDU_ADV_DATA_HEADER_DATA_OFFSET];
	(void)ull_chan_map_get(chm_upd_ind->chm);
	instant = lll_sync->event_counter + 6U;
	chm_upd_ind->instant = sys_cpu_to_le16(instant);

	/* Update the LLL to reflect the Channel Map and Instant to use */
	chm_last = lll_sync->chm_last + 1;
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

	return 0;
}

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

static inline struct pdu_adv_sync_info *sync_info_get(struct pdu_adv *pdu)
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

static inline void sync_info_offset_fill(struct pdu_adv_sync_info *si,
					 uint32_t ticks_offset,
					 uint32_t remainder_us,
					 uint32_t start_us)
{
	uint32_t offs;

	offs = HAL_TICKER_TICKS_TO_US(ticks_offset) + remainder_us - start_us;

	if (offs >= OFFS_ADJUST_US) {
		offs -= OFFS_ADJUST_US;
		si->offs_adjust = 1U;
	}

	offs = offs / OFFS_UNIT_30_US;
	if (!!(offs >> OFFS_UNIT_BITS)) {
		si->offs = sys_cpu_to_le16(offs / (OFFS_UNIT_300_US /
						   OFFS_UNIT_30_US));
		si->offs_units = OFFS_UNIT_VALUE_300_US;
	} else {
		si->offs = sys_cpu_to_le16(offs);
		si->offs_units = OFFS_UNIT_VALUE_30_US;
	}
}

static void ticker_cb(uint32_t ticks_at_expire, uint32_t ticks_drift,
		      uint32_t remainder, uint16_t lazy, uint8_t force,
		      void *param)
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

#if defined(CONFIG_BT_CTLR_ADV_ISO)
	if (lll->iso) {
		ull_adv_iso_offset_get(sync);
	}
#endif /* CONFIG_BT_CTLR_ADV_ISO */

	DEBUG_RADIO_PREPARE_A(1);
}

static void ticker_op_cb(uint32_t status, void *param)
{
	*((uint32_t volatile *)param) = status;
}
