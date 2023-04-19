/*
 * Copyright (c) 2016-2021 Nordic Semiconductor ASA
 * Copyright (c) 2016 Vinayak Kariappa Chettimada
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>

#include <zephyr/kernel.h>
#include <soc.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/sys/byteorder.h>

#include "hal/cpu.h"
#include "hal/ccm.h"
#include "hal/radio.h"
#include "hal/ticker.h"
#include "hal/cntr.h"

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
#include "lll_scan.h"
#include "lll/lll_df_types.h"
#include "lll_conn.h"
#include "lll_filter.h"
#include "lll_conn_iso.h"

#include "ll_sw/ull_tx_queue.h"

#include "ull_adv_types.h"
#include "ull_scan_types.h"
#include "ull_conn_types.h"
#include "ull_filter.h"

#include "ull_adv_internal.h"
#include "ull_scan_internal.h"
#include "ull_conn_internal.h"
#include "ull_internal.h"

#include "ll.h"
#include "ll_feat.h"
#include "ll_settings.h"

#include "ll_sw/isoal.h"
#include "ll_sw/ull_iso_types.h"
#include "ll_sw/ull_conn_iso_types.h"

#include "ll_sw/ull_llcp.h"


#include "hal/debug.h"

inline struct ll_adv_set *ull_adv_set_get(uint8_t handle);
inline uint16_t ull_adv_handle_get(struct ll_adv_set *adv);

static int init_reset(void);
static inline struct ll_adv_set *is_disabled_get(uint8_t handle);
static uint16_t adv_time_get(struct pdu_adv *pdu, struct pdu_adv *pdu_scan,
			     uint8_t adv_chn_cnt, uint8_t phy,
			     uint8_t phy_flags);

static void ticker_cb(uint32_t ticks_at_expire, uint32_t ticks_drift,
		      uint32_t remainder, uint16_t lazy, uint8_t force,
		      void *param);

static void ticker_update_op_cb(uint32_t status, void *param);

#if defined(CONFIG_BT_PERIPHERAL)
static void ticker_stop_cb(uint32_t ticks_at_expire, uint32_t ticks_drift,
			   uint32_t remainder, uint16_t lazy, uint8_t force,
			   void *param);
static void ticker_stop_op_cb(uint32_t status, void *param);
static void adv_disable(void *param);
static void disabled_cb(void *param);
static void conn_release(struct ll_adv_set *adv);
#endif /* CONFIG_BT_PERIPHERAL */

#if defined(CONFIG_BT_CTLR_ADV_EXT)
static uint8_t leg_adv_type_get(uint8_t evt_prop);
static void adv_max_events_duration_set(struct ll_adv_set *adv,
					uint16_t duration,
					uint8_t max_ext_adv_evts);
static void ticker_stop_aux_op_cb(uint32_t status, void *param);
static void aux_disable(void *param);
static void aux_disabled_cb(void *param);
static void ticker_stop_ext_op_cb(uint32_t status, void *param);
static void ext_disable(void *param);
static void ext_disabled_cb(void *param);
#endif /* CONFIG_BT_CTLR_ADV_EXT */

static inline uint8_t disable(uint8_t handle);

static uint8_t adv_scan_pdu_addr_update(struct ll_adv_set *adv,
					struct pdu_adv *pdu,
					struct pdu_adv *pdu_scan);
static const uint8_t *adva_update(struct ll_adv_set *adv, struct pdu_adv *pdu);
static void tgta_update(struct ll_adv_set *adv, struct pdu_adv *pdu);

static void init_pdu(struct pdu_adv *pdu, uint8_t pdu_type);
static void init_set(struct ll_adv_set *adv);

static struct ll_adv_set ll_adv[BT_CTLR_ADV_SET];

#if defined(CONFIG_BT_TICKER_EXT)
static struct ticker_ext ll_adv_ticker_ext[BT_CTLR_ADV_SET];
#endif /* CONFIG_BT_TICKER_EXT */

#if defined(CONFIG_BT_HCI_RAW) && defined(CONFIG_BT_CTLR_ADV_EXT)
static uint8_t ll_adv_cmds;

int ll_adv_cmds_set(uint8_t adv_cmds)
{
	if (!ll_adv_cmds) {
		ll_adv_cmds = adv_cmds;

		if (adv_cmds == LL_ADV_CMDS_LEGACY) {
			struct ll_adv_set *adv = &ll_adv[0];

#if defined(CONFIG_BT_CTLR_HCI_ADV_HANDLE_MAPPING)
			adv->hci_handle = 0;
#endif
			adv->is_created = 1;
		}
	}

	if (ll_adv_cmds != adv_cmds) {
		return -EINVAL;
	}

	return 0;
}

int ll_adv_cmds_is_ext(void)
{
	return ll_adv_cmds == LL_ADV_CMDS_EXT;
}
#endif

#if defined(CONFIG_BT_CTLR_HCI_ADV_HANDLE_MAPPING)
uint8_t ll_adv_set_by_hci_handle_get(uint8_t hci_handle, uint8_t *handle)
{
	struct ll_adv_set *adv;
	uint8_t idx;

	adv =  &ll_adv[0];

	for (idx = 0U; idx < BT_CTLR_ADV_SET; idx++, adv++) {
		if (adv->is_created && (adv->hci_handle == hci_handle)) {
			*handle = idx;
			return 0;
		}
	}

	return BT_HCI_ERR_UNKNOWN_ADV_IDENTIFIER;
}

uint8_t ll_adv_set_by_hci_handle_get_or_new(uint8_t hci_handle, uint8_t *handle)
{
	struct ll_adv_set *adv, *adv_empty;
	uint8_t idx;

	adv =  &ll_adv[0];
	adv_empty = NULL;

	for (idx = 0U; idx < BT_CTLR_ADV_SET; idx++, adv++) {
		if (adv->is_created) {
			if (adv->hci_handle == hci_handle) {
				*handle = idx;
				return 0;
			}
		} else if (!adv_empty) {
			adv_empty = adv;
		}
	}

	if (adv_empty) {
		adv_empty->hci_handle = hci_handle;
		*handle = ull_adv_handle_get(adv_empty);
		return 0;
	}

	return BT_HCI_ERR_MEM_CAPACITY_EXCEEDED;
}

uint8_t ll_adv_set_hci_handle_get(uint8_t handle)
{
	struct ll_adv_set *adv;

	adv = ull_adv_set_get(handle);
	LL_ASSERT(adv && adv->is_created);

	return adv->hci_handle;
}
#endif

#if defined(CONFIG_BT_CTLR_ADV_EXT)
uint8_t ll_adv_params_set(uint8_t handle, uint16_t evt_prop, uint32_t interval,
		       uint8_t adv_type, uint8_t own_addr_type,
		       uint8_t direct_addr_type, uint8_t const *const direct_addr,
		       uint8_t chan_map, uint8_t filter_policy,
		       uint8_t *const tx_pwr, uint8_t phy_p, uint8_t skip,
		       uint8_t phy_s, uint8_t sid, uint8_t sreq)
{
	uint8_t const pdu_adv_type[] = {PDU_ADV_TYPE_ADV_IND,
				     PDU_ADV_TYPE_DIRECT_IND,
				     PDU_ADV_TYPE_SCAN_IND,
				     PDU_ADV_TYPE_NONCONN_IND,
				     PDU_ADV_TYPE_DIRECT_IND,
				     PDU_ADV_TYPE_EXT_IND};
	uint8_t is_pdu_type_changed = 0;
	uint8_t is_new_set;
#else /* !CONFIG_BT_CTLR_ADV_EXT */
uint8_t ll_adv_params_set(uint16_t interval, uint8_t adv_type,
		       uint8_t own_addr_type, uint8_t direct_addr_type,
		       uint8_t const *const direct_addr, uint8_t chan_map,
		       uint8_t filter_policy)
{
	uint8_t const pdu_adv_type[] = {PDU_ADV_TYPE_ADV_IND,
				     PDU_ADV_TYPE_DIRECT_IND,
				     PDU_ADV_TYPE_SCAN_IND,
				     PDU_ADV_TYPE_NONCONN_IND,
				     PDU_ADV_TYPE_DIRECT_IND};
	uint8_t const handle = 0;
#endif /* !CONFIG_BT_CTLR_ADV_EXT */

	struct ll_adv_set *adv;
	uint8_t pdu_type_prev;
	struct pdu_adv *pdu;

	adv = is_disabled_get(handle);
	if (!adv) {
		return BT_HCI_ERR_CMD_DISALLOWED;
	}

#if defined(CONFIG_BT_CTLR_ADV_EXT)
	/* TODO: check and fail (0x12, invalid HCI cmd param) if invalid
	 * evt_prop bits.
	 */

	/* Extended adv param set command used */
	if (adv_type == PDU_ADV_TYPE_EXT_IND) {
		/* legacy */
		if (evt_prop & BT_HCI_LE_ADV_PROP_LEGACY) {
			if (evt_prop & BT_HCI_LE_ADV_PROP_ANON) {
				return BT_HCI_ERR_UNSUPP_FEATURE_PARAM_VAL;
			}

#if defined(CONFIG_BT_CTLR_ADV_PERIODIC)
			/* disallow changing to legacy advertising while
			 * periodic advertising enabled.
			 */
			if (adv->lll.sync) {
				const struct ll_adv_sync_set *sync;

				sync = HDR_LLL2ULL(adv->lll.sync);
				if (sync->is_enabled) {
					return BT_HCI_ERR_INVALID_PARAM;
				}
			}
#endif /* CONFIG_BT_CTLR_ADV_PERIODIC */

			adv_type = leg_adv_type_get(evt_prop);

			adv->lll.phy_p = PHY_1M;
		} else {
			/* - Connectable and scannable not allowed;
			 * - High duty cycle directed connectable not allowed
			 */
			if (((evt_prop & (BT_HCI_LE_ADV_PROP_CONN |
					 BT_HCI_LE_ADV_PROP_SCAN)) ==
			     (BT_HCI_LE_ADV_PROP_CONN |
			      BT_HCI_LE_ADV_PROP_SCAN)) ||
			    (evt_prop & BT_HCI_LE_ADV_PROP_HI_DC_CONN)) {
				return BT_HCI_ERR_INVALID_PARAM;
			}

#if defined(CONFIG_BT_CTLR_ADV_PERIODIC)
			if (adv->lll.sync &&
			    (evt_prop & (BT_HCI_LE_ADV_PROP_ANON |
					 BT_HCI_LE_ADV_PROP_CONN |
					 BT_HCI_LE_ADV_PROP_SCAN))) {
				const struct ll_adv_sync_set *sync;

				sync = HDR_LLL2ULL(adv->lll.sync);
				if (sync->is_enabled) {
					return BT_HCI_ERR_INVALID_PARAM;
				}
			}
#endif /* CONFIG_BT_CTLR_ADV_PERIODIC */

#if (CONFIG_BT_CTLR_ADV_AUX_SET == 0)
			/* Connectable or scannable requires aux */
			if (evt_prop & (BT_HCI_LE_ADV_PROP_CONN |
					BT_HCI_LE_ADV_PROP_SCAN)) {
				return BT_HCI_ERR_MEM_CAPACITY_EXCEEDED;
			}
#endif

			adv_type = 0x05; /* index of PDU_ADV_TYPE_EXT_IND in */
					 /* pdu_adv_type[] */

			adv->lll.phy_p = phy_p;
			adv->lll.phy_flags = PHY_FLAGS_S8;
		}
	} else {
		adv->lll.phy_p = PHY_1M;
	}

	is_new_set = !adv->is_created;
	adv->is_created = 1;
	adv->is_ad_data_cmplt = 1U;
#endif /* CONFIG_BT_CTLR_ADV_EXT */

	/* remember parameters so that set adv/scan data and adv enable
	 * interface can correctly update adv/scan data in the
	 * double buffer between caller and controller context.
	 */
	/* Set interval for Undirected or Low Duty Cycle Directed Advertising */
	if (adv_type != 0x01) {
		adv->interval = interval;
	} else {
		adv->interval = 0;
	}
	adv->lll.chan_map = chan_map;
	adv->lll.filter_policy = filter_policy;

#if defined(CONFIG_BT_CTLR_SCAN_REQ_NOTIFY)
	adv->lll.scan_req_notify = sreq;
#endif

	/* update the "current" primary adv PDU */
	pdu = lll_adv_data_peek(&adv->lll);
	pdu_type_prev = pdu->type;
#if defined(CONFIG_BT_CTLR_ADV_EXT)
	if (is_new_set) {
		is_pdu_type_changed = 1;

		pdu->type = pdu_adv_type[adv_type];
		if (pdu->type != PDU_ADV_TYPE_EXT_IND) {
			pdu->len = 0U;
		}
	/* check if new PDU type is different that past one */
	} else if (pdu->type != pdu_adv_type[adv_type]) {
		is_pdu_type_changed = 1;

		/* If old PDU was extended advertising PDU, release
		 * auxiliary and periodic advertising sets.
		 */
		if (pdu->type == PDU_ADV_TYPE_EXT_IND) {
			struct lll_adv_aux *lll_aux = adv->lll.aux;

			if (lll_aux) {
				struct ll_adv_aux_set *aux;

				/* FIXME: copy AD data from auxiliary channel
				 * PDU.
				 */
				pdu->len = 0;

#if defined(CONFIG_BT_CTLR_ADV_PERIODIC)
				if (adv->lll.sync) {
					struct ll_adv_sync_set *sync;

					sync = HDR_LLL2ULL(adv->lll.sync);
					adv->lll.sync = NULL;

					ull_adv_sync_release(sync);
				}
#endif /* CONFIG_BT_CTLR_ADV_PERIODIC */

				/* Release auxiliary channel set */
				aux = HDR_LLL2ULL(lll_aux);
				adv->lll.aux = NULL;

				ull_adv_aux_release(aux);
			} else {
				/* No previous AD data in auxiliary channel
				 * PDU.
				 */
				pdu->len = 0;
			}
		}

		pdu->type = pdu_adv_type[adv_type];
	}

#else /* !CONFIG_BT_CTLR_ADV_EXT */
	pdu->type = pdu_adv_type[adv_type];
#endif /* !CONFIG_BT_CTLR_ADV_EXT */

	pdu->rfu = 0;

	if (IS_ENABLED(CONFIG_BT_CTLR_CHAN_SEL_2) &&
	    ((pdu->type == PDU_ADV_TYPE_ADV_IND) ||
	     (pdu->type == PDU_ADV_TYPE_DIRECT_IND))) {
		pdu->chan_sel = 1;
	} else {
		pdu->chan_sel = 0;
	}

#if defined(CONFIG_BT_CTLR_AD_DATA_BACKUP)
	/* Backup the legacy AD Data if switching to legacy directed advertising
	 * or to Extended Advertising.
	 */
	if (((pdu->type == PDU_ADV_TYPE_DIRECT_IND) ||
	     (IS_ENABLED(CONFIG_BT_CTLR_ADV_EXT) &&
	      (pdu->type == PDU_ADV_TYPE_EXT_IND))) &&
	    (pdu_type_prev != PDU_ADV_TYPE_DIRECT_IND) &&
	    (!IS_ENABLED(CONFIG_BT_CTLR_ADV_EXT) ||
	     (pdu_type_prev != PDU_ADV_TYPE_EXT_IND))) {
		if (pdu->len == 0U) {
			adv->ad_data_backup.len = 0U;
		} else {
			LL_ASSERT(pdu->len >=
				  offsetof(struct pdu_adv_adv_ind, data));

			adv->ad_data_backup.len = pdu->len -
				offsetof(struct pdu_adv_adv_ind, data);
			memcpy(adv->ad_data_backup.data, pdu->adv_ind.data,
			       adv->ad_data_backup.len);
		}
	}
#endif /* CONFIG_BT_CTLR_AD_DATA_BACKUP */

#if defined(CONFIG_BT_CTLR_PRIVACY)
	adv->own_addr_type = own_addr_type;
	if (adv->own_addr_type == BT_ADDR_LE_PUBLIC_ID ||
	    adv->own_addr_type == BT_ADDR_LE_RANDOM_ID) {
		adv->peer_addr_type = direct_addr_type;
		memcpy(&adv->peer_addr, direct_addr, BDADDR_SIZE);
	}
#endif /* CONFIG_BT_CTLR_PRIVACY */

	if (pdu->type == PDU_ADV_TYPE_DIRECT_IND) {
		pdu->tx_addr = own_addr_type & 0x1;
		pdu->rx_addr = direct_addr_type;
		memcpy(&pdu->direct_ind.tgt_addr[0], direct_addr, BDADDR_SIZE);
		pdu->len = sizeof(struct pdu_adv_direct_ind);

#if defined(CONFIG_BT_CTLR_ADV_EXT)
	} else if (pdu->type == PDU_ADV_TYPE_EXT_IND) {
		struct pdu_adv_ext_hdr *pri_hdr, pri_hdr_prev;
		struct pdu_adv_com_ext_adv *pri_com_hdr;
		uint8_t *pri_dptr_prev, *pri_dptr;
		uint8_t len;

		pri_com_hdr = (void *)&pdu->adv_ext_ind;
		pri_hdr = (void *)pri_com_hdr->ext_hdr_adv_data;
		pri_dptr = pri_hdr->data;
		pri_dptr_prev = pri_dptr;

		/* No ACAD and no AdvData */
		pri_com_hdr->adv_mode = evt_prop & 0x03;

		/* Zero-init header flags */
		if (is_pdu_type_changed) {
			*(uint8_t *)&pri_hdr_prev = 0U;
		} else {
			pri_hdr_prev = *pri_hdr;
		}
		*(uint8_t *)pri_hdr = 0U;

		/* AdvA flag */
		if (pri_hdr_prev.adv_addr) {
			pri_dptr_prev += BDADDR_SIZE;
		}
		if (!pri_com_hdr->adv_mode &&
		    !(evt_prop & BT_HCI_LE_ADV_PROP_ANON) &&
		    (!pri_hdr_prev.aux_ptr || (phy_p != PHY_CODED))) {
			/* TODO: optional on 1M with Aux Ptr */
			pri_hdr->adv_addr = 1;

			/* NOTE: AdvA is filled at enable */
			pdu->tx_addr = own_addr_type & 0x1;
			pri_dptr += BDADDR_SIZE;
		} else {
			pdu->tx_addr = 0;
		}

		/* TargetA flag */
		if (pri_hdr_prev.tgt_addr) {
			pri_dptr_prev += BDADDR_SIZE;
		}
		/* TargetA flag in primary channel PDU only for directed */
		if (evt_prop & BT_HCI_LE_ADV_PROP_DIRECT) {
			pri_hdr->tgt_addr = 1;
			pdu->rx_addr = direct_addr_type;
			pri_dptr += BDADDR_SIZE;
		} else {
			pdu->rx_addr = 0;
		}

		/* No CTEInfo flag in primary channel PDU */

		/* ADI flag */
		if (pri_hdr_prev.adi) {
			pri_dptr_prev += sizeof(struct pdu_adv_adi);

			pri_hdr->adi = 1;
			pri_dptr += sizeof(struct pdu_adv_adi);
		}

#if (CONFIG_BT_CTLR_ADV_AUX_SET > 0)
		/* AuxPtr flag */
		if (pri_hdr_prev.aux_ptr) {
			pri_dptr_prev += sizeof(struct pdu_adv_aux_ptr);
		}
		/* Need aux for connectable or scannable extended advertising */
		if (pri_hdr_prev.aux_ptr ||
		    ((evt_prop & (BT_HCI_LE_ADV_PROP_CONN |
				  BT_HCI_LE_ADV_PROP_SCAN)))) {
			pri_hdr->aux_ptr = 1;
			pri_dptr += sizeof(struct pdu_adv_aux_ptr);
		}
#endif /* (CONFIG_BT_CTLR_ADV_AUX_SET > 0) */

		/* No SyncInfo flag in primary channel PDU */

		/* Tx Power flag */
		if (pri_hdr_prev.tx_pwr) {
			pri_dptr_prev += sizeof(uint8_t);
		}
		/* C1, Tx Power is optional on the LE 1M PHY, and reserved for
		 * for future use on the LE Coded PHY.
		 */
		if ((evt_prop & BT_HCI_LE_ADV_PROP_TX_POWER) &&
		    (!pri_hdr_prev.aux_ptr || (phy_p != PHY_CODED))) {
			pri_hdr->tx_pwr = 1;
			pri_dptr += sizeof(uint8_t);
		}

		/* Calc primary PDU len */
		len = ull_adv_aux_hdr_len_calc(pri_com_hdr, &pri_dptr);
		ull_adv_aux_hdr_len_fill(pri_com_hdr, len);

		/* Set PDU length */
		pdu->len = len;

		/* Start filling primary PDU payload based on flags */

		/* No AdvData in primary channel PDU */

		/* No ACAD in primary channel PDU */

		/* Tx Power */
		if (pri_hdr_prev.tx_pwr) {
			pri_dptr_prev -= sizeof(uint8_t);
		}
		if (pri_hdr->tx_pwr) {
			uint8_t _tx_pwr;

			_tx_pwr = 0;
			if (tx_pwr) {
				if (*tx_pwr != BT_HCI_LE_ADV_TX_POWER_NO_PREF) {
					_tx_pwr = *tx_pwr;
				} else {
					*tx_pwr = _tx_pwr;
				}
			}

			pri_dptr -= sizeof(uint8_t);
			*pri_dptr = _tx_pwr;
		}

		/* No SyncInfo in primary channel PDU */

#if (CONFIG_BT_CTLR_ADV_AUX_SET > 0)
		/* AuxPtr */
		if (pri_hdr_prev.aux_ptr) {
			pri_dptr_prev -= sizeof(struct pdu_adv_aux_ptr);
		}
		if (pri_hdr->aux_ptr) {
			pri_dptr -= sizeof(struct pdu_adv_aux_ptr);
			ull_adv_aux_ptr_fill((void *)pri_dptr, 0U, phy_s);
		}
		adv->lll.phy_s = phy_s;
#endif /* (CONFIG_BT_CTLR_ADV_AUX_SET > 0) */

		/* ADI */
		if (pri_hdr_prev.adi) {
			pri_dptr_prev -= sizeof(struct pdu_adv_adi);
		}
		if (pri_hdr->adi) {
			struct pdu_adv_adi *adi;

			pri_dptr -= sizeof(struct pdu_adv_adi);

			/* NOTE: memmove shall handle overlapping buffers */
			memmove(pri_dptr, pri_dptr_prev,
				sizeof(struct pdu_adv_adi));

			adi = (void *)pri_dptr;
			PDU_ADV_ADI_SID_SET(adi, sid);
		}
		adv->sid = sid;

		/* No CTEInfo field in primary channel PDU */

		/* TargetA */
		if (pri_hdr_prev.tgt_addr) {
			pri_dptr_prev -= BDADDR_SIZE;
		}
		if (pri_hdr->tgt_addr) {
			pri_dptr -= BDADDR_SIZE;
			/* NOTE: RPA will be updated on enable, if needed */
			memcpy(pri_dptr, direct_addr, BDADDR_SIZE);
		}

		/* NOTE: AdvA, filled at enable and RPA timeout */

#if (CONFIG_BT_CTLR_ADV_AUX_SET > 0)
		/* Make sure aux is created if we have AuxPtr */
		if (pri_hdr->aux_ptr) {
			uint8_t pri_idx, sec_idx;
			uint8_t err;

			err = ull_adv_aux_hdr_set_clear(adv,
						ULL_ADV_PDU_HDR_FIELD_ADVA,
						0U, &own_addr_type,
						&pri_idx, &sec_idx);
			if (err) {
				/* TODO: cleanup? */
				return err;
			}

			lll_adv_aux_data_enqueue(adv->lll.aux, sec_idx);
			lll_adv_data_enqueue(&adv->lll, pri_idx);
		}
#endif /* (CONFIG_BT_CTLR_ADV_AUX_SET > 0) */

#endif /* CONFIG_BT_CTLR_ADV_EXT */

	} else if (pdu->len == 0) {
		pdu->tx_addr = own_addr_type & 0x1;
		pdu->rx_addr = 0;
		pdu->len = BDADDR_SIZE;
	} else {

#if defined(CONFIG_BT_CTLR_AD_DATA_BACKUP)
		if (((pdu_type_prev == PDU_ADV_TYPE_DIRECT_IND) ||
		     (IS_ENABLED(CONFIG_BT_CTLR_ADV_EXT) &&
		      (pdu_type_prev == PDU_ADV_TYPE_EXT_IND))) &&
		    (pdu->type != PDU_ADV_TYPE_DIRECT_IND) &&
		    (!IS_ENABLED(CONFIG_BT_CTLR_ADV_EXT) ||
		     (pdu->type != PDU_ADV_TYPE_EXT_IND))) {
			/* Restore the legacy AD Data */
			memcpy(pdu->adv_ind.data, adv->ad_data_backup.data,
			       adv->ad_data_backup.len);
			pdu->len = offsetof(struct pdu_adv_adv_ind, data) +
				   adv->ad_data_backup.len;
		}
#endif /* CONFIG_BT_CTLR_AD_DATA_BACKUP */

		pdu->tx_addr = own_addr_type & 0x1;
		pdu->rx_addr = 0;
	}

	/* Initialize LLL header with parent pointer so that ULL contexts
	 * can be referenced in functions having the LLL context reference.
	 */
	lll_hdr_init(&adv->lll, adv);

	if (0) {
#if defined(CONFIG_BT_CTLR_ADV_EXT)
	} else if (pdu->type == PDU_ADV_TYPE_EXT_IND) {
		/* Make sure new extended advertising set is initialized with no
		 * scan response data. Existing sets keep whatever data was set.
		 */
		if (is_pdu_type_changed) {
			uint8_t err;

			/* Make sure the scan response PDU is allocated from the right pool */
			(void)lll_adv_data_release(&adv->lll.scan_rsp);
			lll_adv_data_reset(&adv->lll.scan_rsp);
			err = lll_adv_aux_data_init(&adv->lll.scan_rsp);
			if (err) {
				return err;
			}

			pdu = lll_adv_scan_rsp_peek(&adv->lll);
			pdu->type = PDU_ADV_TYPE_AUX_SCAN_RSP;
			pdu->len = 0;
		}
#endif /* CONFIG_BT_CTLR_ADV_EXT */
	} else {
		pdu = lll_adv_scan_rsp_peek(&adv->lll);

#if defined(CONFIG_BT_CTLR_ADV_EXT)
		if (is_pdu_type_changed || !pdu) {
			uint8_t err;

			/* Make sure the scan response PDU is allocated from the right pool */
			(void)lll_adv_data_release(&adv->lll.scan_rsp);
			lll_adv_data_reset(&adv->lll.scan_rsp);
			err = lll_adv_data_init(&adv->lll.scan_rsp);
			if (err) {
				return err;
			}

			pdu = lll_adv_scan_rsp_peek(&adv->lll);
		}
#endif /* CONFIG_BT_CTLR_ADV_EXT */

		/* Make sure legacy advertising set has scan response data
		 * initialized.
		 */
		pdu->type = PDU_ADV_TYPE_SCAN_RSP;
		pdu->rfu = 0;
		pdu->chan_sel = 0;
		pdu->tx_addr = own_addr_type & 0x1;
		pdu->rx_addr = 0;
		if (pdu->len == 0) {
			pdu->len = BDADDR_SIZE;
		}
	}

	return 0;
}

#if defined(CONFIG_BT_CTLR_ADV_EXT)
uint8_t ll_adv_data_set(uint8_t handle, uint8_t len, uint8_t const *const data)
{
#else /* !CONFIG_BT_CTLR_ADV_EXT */
uint8_t ll_adv_data_set(uint8_t len, uint8_t const *const data)
{
	const uint8_t handle = 0;
#endif /* !CONFIG_BT_CTLR_ADV_EXT */
	struct ll_adv_set *adv;

	adv = ull_adv_set_get(handle);
	if (!adv) {
		return BT_HCI_ERR_CMD_DISALLOWED;
	}

	return ull_adv_data_set(adv, len, data);
}

#if defined(CONFIG_BT_CTLR_ADV_EXT)
uint8_t ll_adv_scan_rsp_set(uint8_t handle, uint8_t len,
			    uint8_t const *const data)
{
#else /* !CONFIG_BT_CTLR_ADV_EXT */
uint8_t ll_adv_scan_rsp_set(uint8_t len, uint8_t const *const data)
{
	const uint8_t handle = 0;
#endif /* !CONFIG_BT_CTLR_ADV_EXT */
	struct ll_adv_set *adv;

	adv = ull_adv_set_get(handle);
	if (!adv) {
		return BT_HCI_ERR_CMD_DISALLOWED;
	}

	return ull_scan_rsp_set(adv, len, data);
}

#if defined(CONFIG_BT_CTLR_ADV_EXT) || defined(CONFIG_BT_HCI_MESH_EXT)
#if defined(CONFIG_BT_HCI_MESH_EXT)
uint8_t ll_adv_enable(uint8_t handle, uint8_t enable,
		   uint8_t at_anchor, uint32_t ticks_anchor, uint8_t retry,
		   uint8_t scan_window, uint8_t scan_delay)
{
#else /* !CONFIG_BT_HCI_MESH_EXT */
uint8_t ll_adv_enable(uint8_t handle, uint8_t enable,
		   uint16_t duration, uint8_t max_ext_adv_evts)
{
#if defined(CONFIG_BT_CTLR_ADV_PERIODIC)
	struct ll_adv_sync_set *sync = NULL;
	uint8_t sync_is_started = 0U;
#endif /* CONFIG_BT_CTLR_ADV_PERIODIC */
	struct ll_adv_aux_set *aux = NULL;
	uint8_t aux_is_started = 0U;
	uint32_t ticks_anchor;
#endif /* !CONFIG_BT_HCI_MESH_EXT */
#else /* !CONFIG_BT_CTLR_ADV_EXT || !CONFIG_BT_HCI_MESH_EXT */
uint8_t ll_adv_enable(uint8_t enable)
{
	uint8_t const handle = 0;
	uint32_t ticks_anchor;
#endif /* !CONFIG_BT_CTLR_ADV_EXT || !CONFIG_BT_HCI_MESH_EXT */
	uint32_t ticks_slot_overhead;
	uint32_t ticks_slot_offset;
	uint32_t volatile ret_cb;
	struct pdu_adv *pdu_scan;
	struct pdu_adv *pdu_adv;
	struct ll_adv_set *adv;
	struct lll_adv *lll;
	uint8_t hci_err;
	uint32_t ret;

	if (!enable) {
		return disable(handle);
	}

	adv = is_disabled_get(handle);
	if (!adv) {
		/* Bluetooth Specification v5.0 Vol 2 Part E Section 7.8.9
		 * Enabling advertising when it is already enabled can cause the
		 * random address to change. As the current implementation does
		 * does not update RPAs on every advertising enable, only on
		 * `rpa_timeout_ms` timeout, we are not going to implement the
		 * "can cause the random address to change" for legacy
		 * advertisements.
		 */

		/* If HCI LE Set Extended Advertising Enable command is sent
		 * again for an advertising set while that set is enabled, the
		 * timer used for duration and the number of events counter are
		 * reset and any change to the random address shall take effect.
		 */
		if (!IS_ENABLED(CONFIG_BT_CTLR_ADV_ENABLE_STRICT) ||
		    IS_ENABLED(CONFIG_BT_CTLR_ADV_EXT)) {
#if defined(CONFIG_BT_CTLR_ADV_EXT)
			if (ll_adv_cmds_is_ext()) {
				enum node_rx_type volatile *type;

				adv = ull_adv_is_enabled_get(handle);
				if (!adv) {
					/* This should not be happening as
					 * is_disabled_get failed.
					 */
					return BT_HCI_ERR_CMD_DISALLOWED;
				}

				/* Change random address in the primary or
				 * auxiliary PDU as necessary.
				 */
				lll = &adv->lll;
				pdu_adv = lll_adv_data_peek(lll);
				pdu_scan = lll_adv_scan_rsp_peek(lll);
				hci_err = adv_scan_pdu_addr_update(adv,
								   pdu_adv,
								   pdu_scan);
				if (hci_err) {
					return hci_err;
				}

				if (!adv->lll.node_rx_adv_term) {
					/* This should not be happening,
					 * adv->is_enabled would be 0 if
					 * node_rx_adv_term is released back to
					 * pool.
					 */
					return BT_HCI_ERR_CMD_DISALLOWED;
				}

				/* Check advertising not terminated */
				type = &adv->lll.node_rx_adv_term->type;
				if (*type == NODE_RX_TYPE_NONE) {
					/* Reset event counter, update duration,
					 * and max events
					 */
					adv_max_events_duration_set(adv,
						duration, max_ext_adv_evts);
				}

				/* Check the counter reset did not race with
				 * advertising terminated.
				 */
				if (*type != NODE_RX_TYPE_NONE) {
					/* Race with advertising terminated */
					return BT_HCI_ERR_CMD_DISALLOWED;
				}
			}
#endif /* CONFIG_BT_CTLR_ADV_EXT */

			return 0;
		}

		/* Fail on being strict as a legacy controller, valid only under
		 * Bluetooth Specification v4.x.
		 * Bluetooth Specification v5.0 and above shall not fail to
		 * enable already enabled advertising.
		 */
		return BT_HCI_ERR_CMD_DISALLOWED;
	}

	lll = &adv->lll;

#if defined(CONFIG_BT_CTLR_PRIVACY)
	lll->rl_idx = FILTER_IDX_NONE;

	/* Prepare filter accept list and optionally resolving list */
	ull_filter_adv_update(lll->filter_policy);

	if (adv->own_addr_type == BT_ADDR_LE_PUBLIC_ID ||
	    adv->own_addr_type == BT_ADDR_LE_RANDOM_ID) {
		/* Look up the resolving list */
		lll->rl_idx = ull_filter_rl_find(adv->peer_addr_type,
						 adv->peer_addr, NULL);

		if (lll->rl_idx != FILTER_IDX_NONE) {
			/* Generate RPAs if required */
			ull_filter_rpa_update(false);
		}
	}
#endif /* !CONFIG_BT_CTLR_PRIVACY */

	pdu_adv = lll_adv_data_peek(lll);
	pdu_scan = lll_adv_scan_rsp_peek(lll);

#if defined(CONFIG_BT_CTLR_ADV_EXT)
	if (!pdu_scan) {
		uint8_t err;

		if (pdu_adv->type == PDU_ADV_TYPE_EXT_IND) {
			/* Should never happen */
			return BT_HCI_ERR_CMD_DISALLOWED;
		}

		err = lll_adv_data_init(&adv->lll.scan_rsp);
		if (err) {
			return err;
		}

		pdu_scan = lll_adv_scan_rsp_peek(lll);
		init_pdu(pdu_scan, PDU_ADV_TYPE_SCAN_RSP);
	}
#endif /* CONFIG_BT_CTLR_ADV_EXT */

	/* Update Bluetooth Device address in advertising and scan response
	 * PDUs.
	 */
	hci_err = adv_scan_pdu_addr_update(adv, pdu_adv, pdu_scan);
	if (hci_err) {
		return hci_err;
	}

#if defined(CONFIG_BT_HCI_MESH_EXT)
	if (scan_delay) {
		if (ull_scan_is_enabled(0)) {
			return BT_HCI_ERR_CMD_DISALLOWED;
		}

		lll->is_mesh = 1;
	}
#endif /* CONFIG_BT_HCI_MESH_EXT */

#if defined(CONFIG_BT_PERIPHERAL)
	/* prepare connectable advertising */
	if ((pdu_adv->type == PDU_ADV_TYPE_ADV_IND) ||
	    (pdu_adv->type == PDU_ADV_TYPE_DIRECT_IND) ||
#if defined(CONFIG_BT_CTLR_ADV_EXT)
	    ((pdu_adv->type == PDU_ADV_TYPE_EXT_IND) &&
	     (pdu_adv->adv_ext_ind.adv_mode & BT_HCI_LE_ADV_PROP_CONN))
#else
	    0
#endif
	     ) {
		struct node_rx_pdu *node_rx;
		struct ll_conn *conn;
		struct lll_conn *conn_lll;
		void *link;
		int err;

		if (lll->conn) {
			return BT_HCI_ERR_CMD_DISALLOWED;
		}

		link = ll_rx_link_alloc();
		if (!link) {
			return BT_HCI_ERR_MEM_CAPACITY_EXCEEDED;
		}

		node_rx = ll_rx_alloc();
		if (!node_rx) {
			ll_rx_link_release(link);

			return BT_HCI_ERR_MEM_CAPACITY_EXCEEDED;
		}

		conn = ll_conn_acquire();
		if (!conn) {
			ll_rx_release(node_rx);
			ll_rx_link_release(link);

			return BT_HCI_ERR_MEM_CAPACITY_EXCEEDED;
		}

		conn_lll = &conn->lll;
		conn_lll->handle = 0xFFFF;

		if (!conn_lll->link_tx_free) {
			conn_lll->link_tx_free = &conn_lll->link_tx;
		}

		memq_init(conn_lll->link_tx_free, &conn_lll->memq_tx.head,
			  &conn_lll->memq_tx.tail);
		conn_lll->link_tx_free = NULL;

		conn_lll->packet_tx_head_len = 0;
		conn_lll->packet_tx_head_offset = 0;

		conn_lll->sn = 0;
		conn_lll->nesn = 0;
		conn_lll->empty = 0;

#if defined(CONFIG_BT_CTLR_PHY)
		conn_lll->phy_flags = 0;
		if (0) {
#if defined(CONFIG_BT_CTLR_ADV_EXT)
		} else if (pdu_adv->type == PDU_ADV_TYPE_EXT_IND) {
			conn_lll->phy_tx = lll->phy_s;
			conn_lll->phy_tx_time = lll->phy_s;
			conn_lll->phy_rx = lll->phy_s;
#endif /* CONFIG_BT_CTLR_ADV_EXT */
		} else {
			conn_lll->phy_tx = PHY_1M;
			conn_lll->phy_tx_time = PHY_1M;
			conn_lll->phy_rx = PHY_1M;
		}
#endif /* CONFIG_BT_CTLR_PHY */

#if defined(CONFIG_BT_CTLR_CONN_RSSI)
		conn_lll->rssi_latest = BT_HCI_LE_RSSI_NOT_AVAILABLE;
#if defined(CONFIG_BT_CTLR_CONN_RSSI_EVENT)
		conn_lll->rssi_reported = BT_HCI_LE_RSSI_NOT_AVAILABLE;
		conn_lll->rssi_sample_count = 0;
#endif /* CONFIG_BT_CTLR_CONN_RSSI_EVENT */
#endif /* CONFIG_BT_CTLR_CONN_RSSI */

#if defined(CONFIG_BT_CTLR_TX_PWR_DYNAMIC_CONTROL)
		conn_lll->tx_pwr_lvl = RADIO_TXP_DEFAULT;
#endif /* CONFIG_BT_CTLR_TX_PWR_DYNAMIC_CONTROL */

		/* FIXME: BEGIN: Move to ULL? */
		conn_lll->role = 1;
		conn_lll->periph.initiated = 0;
		conn_lll->periph.cancelled = 0;
		conn_lll->data_chan_sel = 0;
		conn_lll->data_chan_use = 0;
		conn_lll->event_counter = 0;

		conn_lll->latency_prepare = 0;
		conn_lll->latency_event = 0;
		conn_lll->periph.latency_enabled = 0;
		conn_lll->periph.window_widening_prepare_us = 0;
		conn_lll->periph.window_widening_event_us = 0;
		conn_lll->periph.window_size_prepare_us = 0;
		/* FIXME: END: Move to ULL? */
#if defined(CONFIG_BT_CTLR_CONN_META)
		memset(&conn_lll->conn_meta, 0, sizeof(conn_lll->conn_meta));
#endif /* CONFIG_BT_CTLR_CONN_META */
#if defined(CONFIG_BT_CTLR_DF_CONN_CTE_RX)
		conn_lll->df_rx_cfg.is_initialized = 0U;
		conn_lll->df_rx_cfg.hdr.elem_size = sizeof(struct lll_df_conn_rx_params);
#endif /* CONFIG_BT_CTLR_DF_CONN_CTE_RX */
#if defined(CONFIG_BT_CTLR_DF_CONN_CTE_TX)
		conn_lll->df_tx_cfg.is_initialized = 0U;
		conn_lll->df_tx_cfg.cte_rsp_en = 0U;
#endif /* CONFIG_BT_CTLR_DF_CONN_CTE_TX */
		conn->connect_expire = 6;
		conn->supervision_expire = 0;

#if defined(CONFIG_BT_CTLR_LE_PING)
		conn->apto_expire = 0U;
		conn->appto_expire = 0U;
#endif

#if defined(CONFIG_BT_CTLR_CHECK_SAME_PEER_CONN)
		conn->own_id_addr_type = BT_ADDR_LE_NONE->type;
		(void)memcpy(conn->own_id_addr, BT_ADDR_LE_NONE->a.val,
			     sizeof(conn->own_id_addr));
		conn->peer_id_addr_type = BT_ADDR_LE_NONE->type;
		(void)memcpy(conn->peer_id_addr, BT_ADDR_LE_NONE->a.val,
			     sizeof(conn->peer_id_addr));
#endif /* CONFIG_BT_CTLR_CHECK_SAME_PEER_CONN */

		/* Re-initialize the control procedure data structures */
		ull_llcp_init(conn);

		conn->llcp_terminate.reason_final = 0;
		/* NOTE: use allocated link for generating dedicated
		 * terminate ind rx node
		 */
		conn->llcp_terminate.node_rx.hdr.link = link;

#if defined(CONFIG_BT_CTLR_PHY)
		conn->phy_pref_tx = ull_conn_default_phy_tx_get();
		conn->phy_pref_rx = ull_conn_default_phy_rx_get();
#endif /* CONFIG_BT_CTLR_PHY */

#if defined(CONFIG_BT_CTLR_LE_ENC)
		conn->pause_rx_data = 0U;
#endif /* CONFIG_BT_CTLR_LE_ENC */

#if defined(CONFIG_BT_CTLR_DATA_LENGTH)
		uint8_t phy_in_use = PHY_1M;


#if defined(CONFIG_BT_CTLR_ADV_EXT)
		if (pdu_adv->type == PDU_ADV_TYPE_EXT_IND) {
			phy_in_use = lll->phy_s;
		}
#endif /* CONFIG_BT_CTLR_ADV_EXT */

		ull_dle_init(conn, phy_in_use);
#endif /* CONFIG_BT_CTLR_DATA_LENGTH */

		/* Re-initialize the Tx Q */
		ull_tx_q_init(&conn->tx_q);

		/* NOTE: using same link as supplied for terminate ind */
		adv->link_cc_free = link;
		adv->node_rx_cc_free = node_rx;
		lll->conn = conn_lll;

		ull_hdr_init(&conn->ull);
		lll_hdr_init(&conn->lll, conn);

		/* wait for stable clocks */
		err = lll_clock_wait();
		if (err) {
			conn_release(adv);

			return BT_HCI_ERR_HW_FAILURE;
		}
	}
#endif /* CONFIG_BT_PERIPHERAL */

#if defined(CONFIG_BT_CTLR_ADV_EXT)
	if (ll_adv_cmds_is_ext()) {
		struct node_rx_pdu *node_rx_adv_term;
		void *link_adv_term;

		/* The alloc here used for ext adv termination event */
		link_adv_term = ll_rx_link_alloc();
		if (!link_adv_term) {
#if defined(CONFIG_BT_PERIPHERAL)
			if (adv->lll.conn) {
				conn_release(adv);
			}
#endif /* CONFIG_BT_PERIPHERAL */

			/* TODO: figure out right return value */
			return BT_HCI_ERR_MEM_CAPACITY_EXCEEDED;
		}

		node_rx_adv_term = ll_rx_alloc();
		if (!node_rx_adv_term) {
#if defined(CONFIG_BT_PERIPHERAL)
			if (adv->lll.conn) {
				conn_release(adv);
			}
#endif /* CONFIG_BT_PERIPHERAL */

			ll_rx_link_release(link_adv_term);

			/* TODO: figure out right return value */
			return BT_HCI_ERR_MEM_CAPACITY_EXCEEDED;
		}

		node_rx_adv_term->hdr.type = NODE_RX_TYPE_NONE;

		node_rx_adv_term->hdr.link = (void *)link_adv_term;
		adv->lll.node_rx_adv_term = (void *)node_rx_adv_term;

		if (0) {
#if defined(CONFIG_BT_PERIPHERAL)
		} else if (lll->is_hdcd) {
			adv_max_events_duration_set(adv, 0U, 0U);
#endif /* CONFIG_BT_PERIPHERAL */
		} else {
			adv_max_events_duration_set(adv, duration,
						    max_ext_adv_evts);
		}
	} else {
		adv->lll.node_rx_adv_term = NULL;
		adv_max_events_duration_set(adv, 0U, 0U);
	}

	const uint8_t phy = lll->phy_p;
	const uint8_t phy_flags = lll->phy_flags;

	adv->event_counter = 0U;
#else
	/* Legacy ADV only supports LE_1M PHY */
	const uint8_t phy = PHY_1M;
	const uint8_t phy_flags = 0U;
#endif

	/* For now we adv on all channels enabled in channel map */
	uint8_t ch_map = lll->chan_map;
	const uint8_t adv_chn_cnt = util_ones_count_get(&ch_map, sizeof(ch_map));

	if (adv_chn_cnt == 0) {
		/* ADV needs at least one channel */
		goto failure_cleanup;
	}

	/* Calculate the advertising time reservation */
	uint16_t time_us = adv_time_get(pdu_adv, pdu_scan, adv_chn_cnt, phy,
					phy_flags);

	uint16_t interval = adv->interval;
#if defined(CONFIG_BT_HCI_MESH_EXT)
	if (lll->is_mesh) {
		uint16_t interval_min_us;

		_radio.advertiser.retry = retry;
		_radio.advertiser.scan_delay_ms = scan_delay;
		_radio.advertiser.scan_window_ms = scan_window;

		interval_min_us = time_us +
				  (scan_delay + scan_window) * USEC_PER_MSEC;
		if ((interval * SCAN_INT_UNIT_US) < interval_min_us) {
			interval = DIV_ROUND_UP(interval_min_us,
						    SCAN_INT_UNIT_US);
		}

		/* passive scanning */
		_radio.scanner.type = 0;

#if defined(CONFIG_BT_CTLR_ADV_EXT)
		/* TODO: Coded PHY support */
		_radio.scanner.phy = 0;
#endif /* CONFIG_BT_CTLR_ADV_EXT */

#if defined(CONFIG_BT_CTLR_PRIVACY)
		/* TODO: Privacy support */
		_radio.scanner.rpa_gen = 0;
		_radio.scanner.rl_idx = rl_idx;
#endif /* CONFIG_BT_CTLR_PRIVACY */

		_radio.scanner.filter_policy = filter_policy;
	}
#endif /* CONFIG_BT_HCI_MESH_EXT */

	/* Initialize ULL context before radio event scheduling is started. */
	ull_hdr_init(&adv->ull);

	/* TODO: active_to_start feature port */
	adv->ull.ticks_active_to_start = 0;
	adv->ull.ticks_prepare_to_start =
		HAL_TICKER_US_TO_TICKS(EVENT_OVERHEAD_XTAL_US);
	adv->ull.ticks_preempt_to_start =
		HAL_TICKER_US_TO_TICKS(EVENT_OVERHEAD_PREEMPT_MIN_US);
	adv->ull.ticks_slot = HAL_TICKER_US_TO_TICKS(time_us);

	ticks_slot_offset = MAX(adv->ull.ticks_active_to_start,
				adv->ull.ticks_prepare_to_start);

	if (IS_ENABLED(CONFIG_BT_CTLR_LOW_LAT)) {
		ticks_slot_overhead = ticks_slot_offset;
	} else {
		ticks_slot_overhead = 0;
	}

#if !defined(CONFIG_BT_HCI_MESH_EXT)
	ticks_anchor = ticker_ticks_now_get();
	ticks_anchor += HAL_TICKER_US_TO_TICKS(EVENT_OVERHEAD_START_US);

#else /* CONFIG_BT_HCI_MESH_EXT */
	if (!at_anchor) {
		ticks_anchor = ticker_ticks_now_get();
	}
#endif /* !CONFIG_BT_HCI_MESH_EXT */

	/* High Duty Cycle Directed Advertising if interval is 0. */
#if defined(CONFIG_BT_PERIPHERAL)
	lll->is_hdcd = !interval && (pdu_adv->type == PDU_ADV_TYPE_DIRECT_IND);
	if (lll->is_hdcd) {
		ret_cb = TICKER_STATUS_BUSY;
#if defined(CONFIG_BT_TICKER_EXT)
#if !defined(CONFIG_BT_CTLR_JIT_SCHEDULING)
		ll_adv_ticker_ext[handle].ticks_slot_window = 0;
#endif /* CONFIG_BT_CTLR_JIT_SCHEDULING */
		ll_adv_ticker_ext[handle].ext_timeout_func = ticker_cb;
		ll_adv_ticker_ext[handle].expire_info_id = TICKER_NULL;

		ret = ticker_start_ext(
#else
		ret = ticker_start(
#endif
				   TICKER_INSTANCE_ID_CTLR,
				   TICKER_USER_ID_THREAD,
				   (TICKER_ID_ADV_BASE + handle),
				   ticks_anchor, 0,
				   (adv->ull.ticks_slot + ticks_slot_overhead),
				   TICKER_NULL_REMAINDER, TICKER_NULL_LAZY,
				   (adv->ull.ticks_slot + ticks_slot_overhead),
#if defined(CONFIG_BT_TICKER_EXT)
				   NULL,
#else
				   ticker_cb,
#endif /* CONFIG_BT_TICKER_EXT */
				   adv,
				   ull_ticker_status_give, (void *)&ret_cb
#if defined(CONFIG_BT_TICKER_EXT)
				   ,
				   &ll_adv_ticker_ext[handle]
#endif /* CONFIG_BT_TICKER_EXT */
				   );
		ret = ull_ticker_status_take(ret, &ret_cb);
		if (ret != TICKER_STATUS_SUCCESS) {
			goto failure_cleanup;
		}

		ret_cb = TICKER_STATUS_BUSY;
		ret = ticker_start(TICKER_INSTANCE_ID_CTLR,
				   TICKER_USER_ID_THREAD,
				   TICKER_ID_ADV_STOP, ticks_anchor,
				   HAL_TICKER_US_TO_TICKS(ticks_slot_offset +
							  (1280 * 1000)),
				   TICKER_NULL_PERIOD, TICKER_NULL_REMAINDER,
				   TICKER_NULL_LAZY, TICKER_NULL_SLOT,
				   ticker_stop_cb, adv,
				   ull_ticker_status_give, (void *)&ret_cb);
	} else
#endif /* CONFIG_BT_PERIPHERAL */
	{
		const uint32_t ticks_slot = adv->ull.ticks_slot +
					 ticks_slot_overhead;
#if (CONFIG_BT_CTLR_ADV_AUX_SET > 0)
#if defined(CONFIG_BT_CTLR_ADV_PERIODIC)
		uint8_t pri_idx, sec_idx;

		/* Add sync_info into auxiliary PDU */
		if (lll->sync) {
			sync = HDR_LLL2ULL(lll->sync);
			if (sync->is_enabled && !sync->is_started) {
				struct pdu_adv_sync_info *sync_info;
				uint8_t value[1 + sizeof(sync_info)];
				uint8_t err;

				err = ull_adv_aux_hdr_set_clear(adv,
						ULL_ADV_PDU_HDR_FIELD_SYNC_INFO,
						0U, value, &pri_idx, &sec_idx);
				if (err) {
					return err;
				}

				/* First byte in the length-value encoded
				 * parameter is size of sync_info structure,
				 * followed by pointer to sync_info in the
				 * PDU.
				 */
				memcpy(&sync_info, &value[1], sizeof(sync_info));
				ull_adv_sync_info_fill(sync, sync_info);
			} else {
				/* Do not start periodic advertising */
				sync = NULL;
			}
		}
#endif /* CONFIG_BT_CTLR_ADV_PERIODIC */

		if (lll->aux) {
			struct lll_adv_aux *lll_aux = lll->aux;
			uint32_t ticks_slot_overhead_aux;
			uint32_t ticks_anchor_aux;

			aux = HDR_LLL2ULL(lll_aux);

			/* Schedule auxiliary PDU after primary channel
			 * PDUs.
			 * Reduce the MAFS offset by the Event Overhead
			 * so that actual radio air packet start as
			 * close as possible after the MAFS gap.
			 * Add 2 ticks offset as compensation towards
			 * the +/- 1 tick ticker scheduling jitter due
			 * to accumulation of remainder to maintain
			 * average ticker interval.
			 */
			ticks_anchor_aux =
				ticks_anchor + ticks_slot +
				HAL_TICKER_US_TO_TICKS(
					MAX(EVENT_MAFS_US,
					    EVENT_OVERHEAD_START_US) -
					EVENT_OVERHEAD_START_US +
					(EVENT_TICKER_RES_MARGIN_US << 1));

			ticks_slot_overhead_aux =
				ull_adv_aux_evt_init(aux, &ticks_anchor_aux);

#if defined(CONFIG_BT_CTLR_ADV_PERIODIC)
			/* Start periodic advertising if enabled and not already
			 * started.
			 */
			if (sync) {
				uint32_t ticks_slot_overhead;
				uint32_t ticks_slot_aux;

#if defined(CONFIG_BT_CTLR_ADV_RESERVE_MAX)
				uint32_t us_slot;

				us_slot = ull_adv_aux_time_get(aux,
						PDU_AC_PAYLOAD_SIZE_MAX,
						PDU_AC_PAYLOAD_SIZE_MAX);
				ticks_slot_aux =
					HAL_TICKER_US_TO_TICKS(us_slot) +
					ticks_slot_overhead_aux;
#else
				ticks_slot_aux = aux->ull.ticks_slot +
						 ticks_slot_overhead_aux;
#endif

#if !defined(CONFIG_BT_CTLR_ADV_AUX_SYNC_OFFSET) || \
	(CONFIG_BT_CTLR_ADV_AUX_SYNC_OFFSET == 0)
				/* Schedule periodic advertising PDU after
				 * auxiliary PDUs.
				 * Reduce the MAFS offset by the Event Overhead
				 * so that actual radio air packet start as
				 * close as possible after the MAFS gap.
				 * Add 2 ticks offset as compensation towards
				 * the +/- 1 tick ticker scheduling jitter due
				 * to accumulation of remainder to maintain
				 * average ticker interval.
				 */
				uint32_t ticks_anchor_sync = ticks_anchor_aux +
					ticks_slot_aux +
					HAL_TICKER_US_TO_TICKS(
						MAX(EVENT_MAFS_US,
						    EVENT_OVERHEAD_START_US) -
						EVENT_OVERHEAD_START_US +
						(EVENT_TICKER_RES_MARGIN_US << 1));

#else /* CONFIG_BT_CTLR_ADV_AUX_SYNC_OFFSET */
				uint32_t ticks_anchor_sync = ticks_anchor_aux +
					HAL_TICKER_US_TO_TICKS(
						CONFIG_BT_CTLR_ADV_AUX_SYNC_OFFSET);

#endif /* CONFIG_BT_CTLR_ADV_AUX_SYNC_OFFSET */

				ticks_slot_overhead = ull_adv_sync_evt_init(adv, sync, NULL);
				ret = ull_adv_sync_start(adv, sync,
							 ticks_anchor_sync,
							 ticks_slot_overhead);
				if (ret) {
					goto failure_cleanup;
				}

				sync_is_started = 1U;

				lll_adv_aux_data_enqueue(adv->lll.aux, sec_idx);
				lll_adv_data_enqueue(lll, pri_idx);
			} else {
				/* TODO: Find the anchor before the group of
				 *       active Periodic Advertising events, so
				 *       that auxiliary sets are grouped such
				 *       that auxiliary sets and Periodic
				 *       Advertising sets are non-overlapping
				 *       for the same event interval.
				 */
			}
#endif /* CONFIG_BT_CTLR_ADV_PERIODIC */

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

			ret = ull_adv_aux_start(aux, ticks_anchor_aux,
						ticks_slot_overhead_aux);
			if (ret) {
				goto failure_cleanup;
			}

			aux_is_started = 1U;
		}
#endif /* (CONFIG_BT_CTLR_ADV_AUX_SET > 0) */

		ret_cb = TICKER_STATUS_BUSY;

#if defined(CONFIG_BT_TICKER_EXT)
#if !defined(CONFIG_BT_CTLR_JIT_SCHEDULING)
		ll_adv_ticker_ext[handle].ticks_slot_window =
			ULL_ADV_RANDOM_DELAY + ticks_slot;
#endif /* CONFIG_BT_CTLR_JIT_SCHEDULING */

		ll_adv_ticker_ext[handle].ext_timeout_func = ticker_cb;

#if defined(CONFIG_BT_CTLR_ADV_EXT)
		if (lll->aux) {
			uint8_t aux_handle = ull_adv_aux_handle_get(aux);

			ll_adv_ticker_ext[handle].expire_info_id = TICKER_ID_ADV_AUX_BASE +
								  aux_handle;
		} else
#endif /* CONFIG_BT_CTLR_ADV_EXT */
		{
			ll_adv_ticker_ext[handle].expire_info_id = TICKER_NULL;
		}

		ret = ticker_start_ext(
#else
		ret = ticker_start(
#endif /* CONFIG_BT_TICKER_EXT */
				   TICKER_INSTANCE_ID_CTLR,
				   TICKER_USER_ID_THREAD,
				   (TICKER_ID_ADV_BASE + handle),
				   ticks_anchor, 0,
				   HAL_TICKER_US_TO_TICKS((uint64_t)interval *
							  ADV_INT_UNIT_US),
				   TICKER_NULL_REMAINDER,
#if !defined(CONFIG_BT_TICKER_LOW_LAT) && \
	!defined(CONFIG_BT_CTLR_LOW_LAT)
				   /* Force expiry to ensure timing update */
				   TICKER_LAZY_MUST_EXPIRE,
#else
				   TICKER_NULL_LAZY,
#endif /* !CONFIG_BT_TICKER_LOW_LAT && !CONFIG_BT_CTLR_LOW_LAT */
				   ticks_slot,
#if defined(CONFIG_BT_TICKER_EXT)
				   NULL,
#else
				   ticker_cb,
#endif /* CONFIG_BT_TICKER_EXT */
				   adv,
				   ull_ticker_status_give,
				   (void *)&ret_cb
#if defined(CONFIG_BT_TICKER_EXT)
				   ,
				   &ll_adv_ticker_ext[handle]
#endif /* CONFIG_BT_TICKER_EXT */
				   );
	}

	ret = ull_ticker_status_take(ret, &ret_cb);
	if (ret != TICKER_STATUS_SUCCESS) {
		goto failure_cleanup;
	}

#if defined(CONFIG_BT_CTLR_ADV_EXT)
	if (aux_is_started) {
		aux->is_started = aux_is_started;

#if defined(CONFIG_BT_CTLR_ADV_PERIODIC)
		if (sync_is_started) {
			sync->is_started = sync_is_started;
		}
#endif /* CONFIG_BT_CTLR_ADV_PERIODIC */
	}
#endif /* CONFIG_BT_CTLR_ADV_EXT */

	adv->is_enabled = 1;

#if defined(CONFIG_BT_CTLR_PRIVACY)
#if defined(CONFIG_BT_HCI_MESH_EXT)
	if (_radio.advertiser.is_mesh) {
		_radio.scanner.is_enabled = 1;

		ull_filter_adv_scan_state_cb(BIT(0) | BIT(1));
	}
#else /* !CONFIG_BT_HCI_MESH_EXT */
	if (!IS_ENABLED(CONFIG_BT_OBSERVER) || !ull_scan_is_enabled_get(0)) {
		ull_filter_adv_scan_state_cb(BIT(0));
	}
#endif /* !CONFIG_BT_HCI_MESH_EXT */
#endif /* CONFIG_BT_CTLR_PRIVACY */

	return 0;

failure_cleanup:
#if (CONFIG_BT_CTLR_ADV_AUX_SET > 0)
	if (aux_is_started) {
		/* TODO: Stop extended advertising and release resources */
	}

#if defined(CONFIG_BT_CTLR_ADV_PERIODIC)
	if (sync_is_started) {
		/* TODO: Stop periodic advertising and release resources */
	}
#endif /* CONFIG_BT_CTLR_ADV_PERIODIC */
#endif /* (CONFIG_BT_CTLR_ADV_AUX_SET > 0) */

#if defined(CONFIG_BT_PERIPHERAL)
	if (adv->lll.conn) {
		conn_release(adv);
	}
#endif /* CONFIG_BT_PERIPHERAL */

	return BT_HCI_ERR_CMD_DISALLOWED;
}

int ull_adv_init(void)
{
	int err;

#if defined(CONFIG_BT_CTLR_ADV_EXT)
#if defined(CONFIG_BT_CTLR_ADV_AUX_SET)
	if (CONFIG_BT_CTLR_ADV_AUX_SET > 0) {
		err = ull_adv_aux_init();
		if (err) {
			return err;
		}
	}

#if defined(CONFIG_BT_CTLR_ADV_PERIODIC)
	err = ull_adv_sync_init();
	if (err) {
		return err;
	}
#endif /* CONFIG_BT_CTLR_ADV_PERIODIC */
#endif /* CONFIG_BT_CTLR_ADV_AUX_SET */
#endif /* CONFIG_BT_CTLR_ADV_EXT */

	err = init_reset();
	if (err) {
		return err;
	}

	return 0;
}

int ull_adv_reset(void)
{
	uint8_t handle;

	for (handle = 0U; handle < BT_CTLR_ADV_SET; handle++) {
		(void)disable(handle);
	}

#if defined(CONFIG_BT_CTLR_ADV_EXT)
#if defined(CONFIG_BT_HCI_RAW)
	ll_adv_cmds = LL_ADV_CMDS_ANY;
#endif
#if defined(CONFIG_BT_CTLR_ADV_PERIODIC)
	{
		int err;

		err = ull_adv_sync_reset();
		if (err) {
			return err;
		}
	}
#endif /* CONFIG_BT_CTLR_ADV_PERIODIC */
#endif /* CONFIG_BT_CTLR_ADV_EXT */

	return 0;
}

int ull_adv_reset_finalize(void)
{
	uint8_t handle;
	int err;

#if defined(CONFIG_BT_CTLR_ADV_EXT)
#if defined(CONFIG_BT_CTLR_ADV_AUX_SET)
	if (CONFIG_BT_CTLR_ADV_AUX_SET > 0) {
		err = ull_adv_aux_reset_finalize();
		if (err) {
			return err;
		}
#if defined(CONFIG_BT_CTLR_ADV_PERIODIC)
		err = ull_adv_sync_reset_finalize();
		if (err) {
			return err;
		}
#endif /* CONFIG_BT_CTLR_ADV_PERIODIC */
	}
#endif /* CONFIG_BT_CTLR_ADV_AUX_SET */
#endif /* CONFIG_BT_CTLR_ADV_EXT */

	for (handle = 0U; handle < BT_CTLR_ADV_SET; handle++) {
		struct ll_adv_set *adv = &ll_adv[handle];
		struct lll_adv *lll = &adv->lll;

#if defined(CONFIG_BT_CTLR_ADV_EXT)
		adv->is_created = 0;
		lll->aux = NULL;
#if defined(CONFIG_BT_CTLR_ADV_PERIODIC)
		lll->sync = NULL;
#endif /* CONFIG_BT_CTLR_ADV_PERIODIC */
#endif /* CONFIG_BT_CTLR_ADV_EXT */
		lll_adv_data_reset(&lll->adv_data);
		lll_adv_data_reset(&lll->scan_rsp);
	}

	err = init_reset();
	if (err) {
		return err;
	}

	return 0;
}

inline struct ll_adv_set *ull_adv_set_get(uint8_t handle)
{
	if (handle >= BT_CTLR_ADV_SET) {
		return NULL;
	}

	return &ll_adv[handle];
}

inline uint16_t ull_adv_handle_get(struct ll_adv_set *adv)
{
	return ((uint8_t *)adv - (uint8_t *)ll_adv) / sizeof(*adv);
}

uint16_t ull_adv_lll_handle_get(struct lll_adv *lll)
{
	return ull_adv_handle_get(HDR_LLL2ULL(lll));
}

inline struct ll_adv_set *ull_adv_is_enabled_get(uint8_t handle)
{
	struct ll_adv_set *adv;

	adv = ull_adv_set_get(handle);
	if (!adv || !adv->is_enabled) {
		return NULL;
	}

	return adv;
}

int ull_adv_is_enabled(uint8_t handle)
{
	struct ll_adv_set *adv;

	adv = ull_adv_is_enabled_get(handle);

	return adv != NULL;
}

uint32_t ull_adv_filter_pol_get(uint8_t handle)
{
	struct ll_adv_set *adv;

	adv = ull_adv_is_enabled_get(handle);
	if (!adv) {
		return 0;
	}

	return adv->lll.filter_policy;
}

#if defined(CONFIG_BT_CTLR_ADV_EXT)
struct ll_adv_set *ull_adv_is_created_get(uint8_t handle)
{
	struct ll_adv_set *adv;

	adv = ull_adv_set_get(handle);
	if (!adv || !adv->is_created) {
		return NULL;
	}

	return adv;
}

void ull_adv_aux_created(struct ll_adv_set *adv)
{
	if (adv->lll.aux && adv->is_enabled) {
		uint8_t aux_handle = ull_adv_aux_handle_get(HDR_LLL2ULL(adv->lll.aux));
		uint8_t handle = ull_adv_handle_get(adv);

		ticker_update_ext(TICKER_INSTANCE_ID_CTLR, TICKER_USER_ID_THREAD,
			   (TICKER_ID_ADV_BASE + handle), 0, 0, 0, 0, 0, 0,
			   ticker_update_op_cb, adv, 0,
			   TICKER_ID_ADV_AUX_BASE + aux_handle);
	}
}
#endif /* CONFIG_BT_CTLR_ADV_EXT */

uint8_t ull_adv_data_set(struct ll_adv_set *adv, uint8_t len,
			 uint8_t const *const data)
{
	struct pdu_adv *prev;
	struct pdu_adv *pdu;
	uint8_t idx;

	/* Check invalid AD Data length */
	if (len > PDU_AC_LEG_DATA_SIZE_MAX) {
		return BT_HCI_ERR_INVALID_PARAM;
	}

	prev = lll_adv_data_peek(&adv->lll);

	/* Dont update data if directed, back it up */
	if ((prev->type == PDU_ADV_TYPE_DIRECT_IND) ||
	    (IS_ENABLED(CONFIG_BT_CTLR_ADV_EXT) &&
	     (prev->type == PDU_ADV_TYPE_EXT_IND))) {
#if defined(CONFIG_BT_CTLR_AD_DATA_BACKUP)
		/* Update the backup AD Data */
		adv->ad_data_backup.len = len;
		memcpy(adv->ad_data_backup.data, data, adv->ad_data_backup.len);
		return 0;

#else /* !CONFIG_BT_CTLR_AD_DATA_BACKUP */
		return BT_HCI_ERR_CMD_DISALLOWED;
#endif /* !CONFIG_BT_CTLR_AD_DATA_BACKUP */
	}

	/* update adv pdu fields. */
	pdu = lll_adv_data_alloc(&adv->lll, &idx);

	/* check for race condition with LLL ISR */
	if (IS_ENABLED(CONFIG_ASSERT)) {
		uint8_t idx_test;

		lll_adv_data_alloc(&adv->lll, &idx_test);
		__ASSERT((idx == idx_test), "Probable AD Data Corruption.\n");
	}

	pdu->type = prev->type;
	pdu->rfu = 0U;

	if (IS_ENABLED(CONFIG_BT_CTLR_CHAN_SEL_2)) {
		pdu->chan_sel = prev->chan_sel;
	} else {
		pdu->chan_sel = 0U;
	}

	pdu->tx_addr = prev->tx_addr;
	pdu->rx_addr = prev->rx_addr;
	memcpy(&pdu->adv_ind.addr[0], &prev->adv_ind.addr[0], BDADDR_SIZE);
	memcpy(&pdu->adv_ind.data[0], data, len);
	pdu->len = BDADDR_SIZE + len;

	/* Update time reservation */
	if (adv->is_enabled) {
		struct pdu_adv *pdu_scan;
		struct lll_adv *lll;
		uint8_t err;

		lll = &adv->lll;
		pdu_scan = lll_adv_scan_rsp_peek(lll);

		err = ull_adv_time_update(adv, pdu, pdu_scan);
		if (err) {
			return err;
		}
	}

	lll_adv_data_enqueue(&adv->lll, idx);

	return 0;
}

uint8_t ull_scan_rsp_set(struct ll_adv_set *adv, uint8_t len,
			 uint8_t const *const data)
{
	struct pdu_adv *prev;
	struct pdu_adv *pdu;
	uint8_t idx;

	if (len > PDU_AC_LEG_DATA_SIZE_MAX) {
		return BT_HCI_ERR_INVALID_PARAM;
	}

	/* update scan pdu fields. */
	prev = lll_adv_scan_rsp_peek(&adv->lll);
	if (!prev) {
		uint8_t err;

		err = lll_adv_data_init(&adv->lll.scan_rsp);
		if (err) {
			return err;
		}

		prev = lll_adv_scan_rsp_peek(&adv->lll);
		init_pdu(prev, PDU_ADV_TYPE_SCAN_RSP);
	}

	pdu = lll_adv_scan_rsp_alloc(&adv->lll, &idx);
	pdu->type = PDU_ADV_TYPE_SCAN_RSP;
	pdu->rfu = 0;
	pdu->chan_sel = 0;
	pdu->tx_addr = prev->tx_addr;
	pdu->rx_addr = 0;
	pdu->len = BDADDR_SIZE + len;
	memcpy(&pdu->scan_rsp.addr[0], &prev->scan_rsp.addr[0], BDADDR_SIZE);
	memcpy(&pdu->scan_rsp.data[0], data, len);

	/* Update time reservation */
	if (adv->is_enabled) {
		struct pdu_adv *pdu_adv_scan;
		struct lll_adv *lll;
		uint8_t err;

		lll = &adv->lll;
		pdu_adv_scan = lll_adv_data_peek(lll);

		if ((pdu_adv_scan->type == PDU_ADV_TYPE_ADV_IND) ||
		    (pdu_adv_scan->type == PDU_ADV_TYPE_SCAN_IND)) {
			err = ull_adv_time_update(adv, pdu_adv_scan, pdu);
			if (err) {
				return err;
			}
		}
	}

	lll_adv_scan_rsp_enqueue(&adv->lll, idx);

	return 0;
}

static uint32_t ticker_update_rand(struct ll_adv_set *adv, uint32_t ticks_delay_window,
				   uint32_t ticks_delay_window_offset,
				   uint32_t ticks_adjust_minus,
				   ticker_op_func fp_op_func)
{
	uint32_t random_delay;
	uint32_t ret;

	/* Get pseudo-random number in the range [0..ticks_delay_window].
	 * Please note that using modulo of 2^32 sample space has an uneven
	 * distribution, slightly favoring smaller values.
	 */
	lll_rand_isr_get(&random_delay, sizeof(random_delay));
	random_delay %= ticks_delay_window;
	random_delay += (ticks_delay_window_offset + 1);

	ret = ticker_update(TICKER_INSTANCE_ID_CTLR,
			    TICKER_USER_ID_ULL_HIGH,
			    TICKER_ID_ADV_BASE + ull_adv_handle_get(adv),
			    random_delay,
			    ticks_adjust_minus, 0, 0, 0, 0,
			    fp_op_func, adv);

	LL_ASSERT((ret == TICKER_STATUS_SUCCESS) ||
		  (ret == TICKER_STATUS_BUSY) ||
		  (fp_op_func == NULL));

#if defined(CONFIG_BT_CTLR_JIT_SCHEDULING)
	adv->delay = random_delay;
#endif
	return random_delay;
}

#if defined(CONFIG_BT_CTLR_ADV_EXT) || \
	defined(CONFIG_BT_CTLR_JIT_SCHEDULING)
void ull_adv_done(struct node_rx_event_done *done)
{
#if defined(CONFIG_BT_CTLR_ADV_EXT)
	struct lll_adv_aux *lll_aux;
	struct node_rx_hdr *rx_hdr;
	uint8_t handle;
	uint32_t ret;
#endif /* CONFIG_BT_CTLR_ADV_EXT */
	struct ll_adv_set *adv;
	struct lll_adv *lll;

	/* Get reference to ULL context */
	adv = CONTAINER_OF(done->param, struct ll_adv_set, ull);
	lll = &adv->lll;

#if defined(CONFIG_BT_CTLR_JIT_SCHEDULING)
	if (done->extra.type == EVENT_DONE_EXTRA_TYPE_ADV && done->extra.result != DONE_COMPLETED) {
		/* Event aborted or too late - try to re-schedule */
		uint32_t ticks_elapsed;
		uint32_t ticks_now;
		uint32_t delay_remain;

		const uint32_t prepare_overhead =
			HAL_TICKER_US_TO_TICKS(EVENT_OVERHEAD_START_US);
		const uint32_t ticks_adv_airtime = adv->ticks_at_expire +
			prepare_overhead;

		ticks_elapsed = 0U;

		ticks_now = cntr_cnt_get();
		if ((int32_t)(ticks_now - ticks_adv_airtime) > 0) {
			ticks_elapsed = ticks_now - ticks_adv_airtime;
		}

		if (adv->delay_at_expire + ticks_elapsed <= ULL_ADV_RANDOM_DELAY) {
			/* The perturbation window is still open */
			delay_remain = ULL_ADV_RANDOM_DELAY - (adv->delay_at_expire +
							       ticks_elapsed);
		} else {
			delay_remain = 0U;
		}

		/* Check if we have enough time to re-schedule */
		if (delay_remain > prepare_overhead) {
			uint32_t ticks_adjust_minus;
			uint32_t interval_us = adv->interval * ADV_INT_UNIT_US;

			/* Get negative ticker adjustment needed to pull back ADV one
			 * interval plus the randomized delay. This means that the ticker
			 * will be updated to expire in time frame of now + start
			 * overhead, until 10 ms window is exhausted.
			 */
			ticks_adjust_minus = HAL_TICKER_US_TO_TICKS(interval_us) + adv->delay;

#if defined(CONFIG_BT_CTLR_ADV_EXT)
			if (adv->remain_duration_us > interval_us) {
				/* Reset remain_duration_us to value before last ticker expire
				 * to correct for the re-scheduling
				 */
				adv->remain_duration_us += interval_us +
							   HAL_TICKER_TICKS_TO_US(
								adv->delay_at_expire);
			}
#endif /* CONFIG_BT_CTLR_ADV_EXT */

			/* Apply random delay in range [prepare_overhead..delay_remain].
			 * NOTE: This ticker_update may fail if update races with
			 * ticker_stop, e.g. from ull_periph_setup. This is not a problem
			 * and we can safely ignore the operation result.
			 */
			ticker_update_rand(adv, delay_remain - prepare_overhead,
					   prepare_overhead, ticks_adjust_minus, NULL);

			/* Delay from ticker_update_rand is in addition to the last random delay */
			adv->delay += adv->delay_at_expire;

			/* Score of the event was increased due to the result, but since
			 * we're getting a another chance we'll set it back.
			 */
			adv->lll.hdr.score -= 1;
		}
	}
#if defined(CONFIG_BT_CTLR_ADV_EXT)
	if (done->extra.type == EVENT_DONE_EXTRA_TYPE_ADV && adv->lll.aux) {
		/* Primary event of extended advertising done - wait for aux done */
		return;
	}
#endif /* CONFIG_BT_CTLR_ADV_EXT */
#endif /* CONFIG_BT_CTLR_JIT_SCHEDULING */

#if defined(CONFIG_BT_CTLR_ADV_EXT)
	if (adv->max_events && (adv->event_counter >= adv->max_events)) {
		adv->max_events = 0U;

		rx_hdr = (void *)lll->node_rx_adv_term;
		rx_hdr->rx_ftr.param_adv_term.status = BT_HCI_ERR_LIMIT_REACHED;
	} else if (adv->remain_duration_us &&
		   (adv->remain_duration_us <=
		    ((uint64_t)adv->interval * ADV_INT_UNIT_US))) {
		adv->remain_duration_us = 0U;

		rx_hdr = (void *)lll->node_rx_adv_term;
		rx_hdr->rx_ftr.param_adv_term.status = BT_HCI_ERR_ADV_TIMEOUT;
	} else {
		return;
	}

	handle = ull_adv_handle_get(adv);
	LL_ASSERT(handle < BT_CTLR_ADV_SET);

	rx_hdr->type = NODE_RX_TYPE_EXT_ADV_TERMINATE;
	rx_hdr->handle = handle;
	rx_hdr->rx_ftr.param_adv_term.conn_handle = 0xffff;
	rx_hdr->rx_ftr.param_adv_term.num_events = adv->event_counter;

	lll_aux = lll->aux;
	if (lll_aux) {
		struct ll_adv_aux_set *aux;
		uint8_t aux_handle;

		aux = HDR_LLL2ULL(lll_aux);
		aux_handle = ull_adv_aux_handle_get(aux);
		ret = ticker_stop(TICKER_INSTANCE_ID_CTLR,
				  TICKER_USER_ID_ULL_HIGH,
				  (TICKER_ID_ADV_AUX_BASE + aux_handle),
				  ticker_stop_aux_op_cb, adv);
	} else {
		ret = ticker_stop(TICKER_INSTANCE_ID_CTLR,
				  TICKER_USER_ID_ULL_HIGH,
				  (TICKER_ID_ADV_BASE + handle),
				  ticker_stop_ext_op_cb, adv);
	}

	LL_ASSERT((ret == TICKER_STATUS_SUCCESS) ||
		  (ret == TICKER_STATUS_BUSY));
#endif /* CONFIG_BT_CTLR_ADV_EXT */
}
#endif /* CONFIG_BT_CTLR_ADV_EXT || CONFIG_BT_CTLR_JIT_SCHEDULING */

const uint8_t *ull_adv_pdu_update_addrs(struct ll_adv_set *adv,
					struct pdu_adv *pdu)
{
	const uint8_t *adv_addr;

#if defined(CONFIG_BT_CTLR_ADV_EXT)
	struct pdu_adv_com_ext_adv *com_hdr = (void *)&pdu->adv_ext_ind;
	struct pdu_adv_ext_hdr *hdr = (void *)com_hdr->ext_hdr_adv_data;
	struct pdu_adv_ext_hdr hdr_flags;

	if (com_hdr->ext_hdr_len) {
		hdr_flags = *hdr;
	} else {
		*(uint8_t *)&hdr_flags = 0U;
	}
#endif

	adv_addr = adva_update(adv, pdu);

	/* Update TargetA only if directed advertising PDU is supplied. Note
	 * that AUX_SCAN_REQ does not have TargetA flag set so it will be
	 * ignored here as expected.
	 */
	if ((pdu->type == PDU_ADV_TYPE_DIRECT_IND) ||
#if defined(CONFIG_BT_CTLR_ADV_EXT)
	    ((pdu->type == PDU_ADV_TYPE_EXT_IND) && hdr_flags.tgt_addr) ||
#endif
	    0) {
		tgta_update(adv, pdu);
	}

	return adv_addr;
}

uint8_t ull_adv_time_update(struct ll_adv_set *adv, struct pdu_adv *pdu,
			    struct pdu_adv *pdu_scan)
{
	uint32_t volatile ret_cb;
	uint32_t ticks_minus;
	uint32_t ticks_plus;
	struct lll_adv *lll;
	uint32_t time_ticks;
	uint8_t phy_flags;
	uint16_t time_us;
	uint8_t chan_map;
	uint8_t chan_cnt;
	uint32_t ret;
	uint8_t phy;

	lll = &adv->lll;

#if defined(CONFIG_BT_CTLR_ADV_EXT)
	phy = lll->phy_p;
	phy_flags = lll->phy_flags;
#else
	phy = PHY_1M;
	phy_flags = 0U;
#endif

	chan_map = lll->chan_map;
	chan_cnt = util_ones_count_get(&chan_map, sizeof(chan_map));
	time_us = adv_time_get(pdu, pdu_scan, chan_cnt, phy, phy_flags);
	time_ticks = HAL_TICKER_US_TO_TICKS(time_us);
	if (adv->ull.ticks_slot > time_ticks) {
		ticks_minus = adv->ull.ticks_slot - time_ticks;
		ticks_plus = 0U;
	} else if (adv->ull.ticks_slot < time_ticks) {
		ticks_minus = 0U;
		ticks_plus = time_ticks - adv->ull.ticks_slot;
	} else {
		return BT_HCI_ERR_SUCCESS;
	}

	ret_cb = TICKER_STATUS_BUSY;
	ret = ticker_update(TICKER_INSTANCE_ID_CTLR,
			    TICKER_USER_ID_THREAD,
			    (TICKER_ID_ADV_BASE +
			     ull_adv_handle_get(adv)),
			    0, 0, ticks_plus, ticks_minus, 0, 0,
			    ull_ticker_status_give, (void *)&ret_cb);
	ret = ull_ticker_status_take(ret, &ret_cb);
	if (ret != TICKER_STATUS_SUCCESS) {
		return BT_HCI_ERR_CMD_DISALLOWED;
	}

	adv->ull.ticks_slot = time_ticks;

	return BT_HCI_ERR_SUCCESS;
}

static int init_reset(void)
{
	uint8_t handle;

#if defined(CONFIG_BT_CTLR_TX_PWR_DYNAMIC_CONTROL) && \
	!defined(CONFIG_BT_CTLR_ADV_EXT)
	ll_adv[0].lll.tx_pwr_lvl = RADIO_TXP_DEFAULT;
#endif /* CONFIG_BT_CTLR_TX_PWR_DYNAMIC_CONTROL && !CONFIG_BT_CTLR_ADV_EXT */

	for (handle = 0U; handle < BT_CTLR_ADV_SET; handle++) {
		lll_adv_data_init(&ll_adv[handle].lll.adv_data);

#if defined(CONFIG_BT_CTLR_ADV_EXT)
		/* scan_rsp is not init'ed until we know if it is a legacy or extended scan rsp */
		memset(&ll_adv[handle].lll.scan_rsp, 0, sizeof(ll_adv[handle].lll.scan_rsp));
#else
		lll_adv_data_init(&ll_adv[handle].lll.scan_rsp);
#endif /* !CONFIG_BT_CTLR_ADV_EXT */

#if defined(CONFIG_BT_CTLR_DF_ADV_CTE_TX)
		/* Pointer to DF configuration must be cleared on reset. In other case it will point
		 * to a memory pool address that should be released. It may be used by the pool
		 * itself. In such situation it may cause error.
		 */
		ll_adv[handle].df_cfg = NULL;
#endif /* CONFIG_BT_CTLR_DF_ADV_CTE_TX */
	}

	/* Make sure that set #0 is initialized with empty legacy PDUs. This is
	 * especially important if legacy HCI interface is used for advertising
	 * because it allows to enable advertising without any configuration,
	 * thus we need to have PDUs already initialized.
	 */
	init_set(&ll_adv[0]);

	return 0;
}

static inline struct ll_adv_set *is_disabled_get(uint8_t handle)
{
	struct ll_adv_set *adv;

	adv = ull_adv_set_get(handle);
	if (!adv || adv->is_enabled) {
		return NULL;
	}

	return adv;
}

static uint16_t adv_time_get(struct pdu_adv *pdu, struct pdu_adv *pdu_scan,
			     uint8_t adv_chn_cnt, uint8_t phy,
			     uint8_t phy_flags)
{
	uint16_t time_us = EVENT_OVERHEAD_START_US + EVENT_OVERHEAD_END_US;

	/* NOTE: 16-bit value is sufficient to calculate the maximum radio
	 *       event time reservation for PDUs on primary advertising
	 *       channels (37, 38, and 39 channel indices of 1M and Coded PHY).
	 */

	/* Calculate the PDU Tx Time and hence the radio event length */
#if defined(CONFIG_BT_CTLR_ADV_EXT)
	if (pdu->type == PDU_ADV_TYPE_EXT_IND) {
		time_us += PDU_AC_US(pdu->len, phy, phy_flags) * adv_chn_cnt +
			   EVENT_RX_TX_TURNAROUND(phy) * (adv_chn_cnt - 1);
	} else
#endif
	{
		uint16_t adv_size =
			PDU_OVERHEAD_SIZE(PHY_1M) + ADVA_SIZE;
		const uint16_t conn_ind_us =
			BYTES2US((PDU_OVERHEAD_SIZE(PHY_1M) +
				 INITA_SIZE + ADVA_SIZE + LLDATA_SIZE), PHY_1M);
		const uint8_t scan_req_us  =
			BYTES2US((PDU_OVERHEAD_SIZE(PHY_1M) +
				 SCANA_SIZE + ADVA_SIZE), PHY_1M);
		const uint16_t scan_rsp_us =
			BYTES2US((PDU_OVERHEAD_SIZE(PHY_1M) +
				 ADVA_SIZE + pdu_scan->len), PHY_1M);
		const uint8_t rx_to_us	= EVENT_RX_TO_US(PHY_1M);
		const uint8_t rxtx_turn_us = EVENT_RX_TX_TURNAROUND(PHY_1M);

		if (pdu->type == PDU_ADV_TYPE_NONCONN_IND) {
			adv_size += pdu->len;
			time_us += BYTES2US(adv_size, PHY_1M) * adv_chn_cnt +
				   rxtx_turn_us * (adv_chn_cnt - 1);
		} else {
			if (pdu->type == PDU_ADV_TYPE_DIRECT_IND) {
				adv_size += TARGETA_SIZE;
				time_us += conn_ind_us;
			} else if (pdu->type == PDU_ADV_TYPE_ADV_IND) {
				adv_size += pdu->len;
				time_us += MAX(scan_req_us + EVENT_IFS_MAX_US +
						scan_rsp_us, conn_ind_us);
			} else if (pdu->type == PDU_ADV_TYPE_SCAN_IND) {
				adv_size += pdu->len;
				time_us += scan_req_us + EVENT_IFS_MAX_US +
					   scan_rsp_us;
			}

			time_us += (BYTES2US(adv_size, PHY_1M) +
				    EVENT_IFS_MAX_US + rx_to_us +
				    rxtx_turn_us) * (adv_chn_cnt - 1) +
				   BYTES2US(adv_size, PHY_1M) + EVENT_IFS_MAX_US;
		}
	}

	return time_us;
}

static void ticker_cb(uint32_t ticks_at_expire, uint32_t ticks_drift,
		      uint32_t remainder, uint16_t lazy, uint8_t force,
		      void *param)
{
	static memq_link_t link;
	static struct mayfly mfy = {0, 0, &link, NULL, lll_adv_prepare};
	static struct lll_prepare_param p;
#if defined(CONFIG_BT_TICKER_EXT)
	struct ticker_ext_context *context = param;
	struct ll_adv_set *adv = context->context;
#else
	struct ll_adv_set *adv = param;
#endif /* CONFIG_BT_TICKER_EXT */
	uint32_t random_delay;
	struct lll_adv *lll;
	uint32_t ret;
	uint8_t ref;

	DEBUG_RADIO_PREPARE_A(1);

	lll = &adv->lll;

#if defined(CONFIG_BT_CTLR_ADV_EXT)
	if (lll->aux) {
		/* Check if we are about to exceed the duration or max events limit
		 * Usually this will be handled in ull_adv_done(), but in cases where
		 * the extended advertising events overlap (ie. several primary advertisings
		 * point to the same AUX_ADV_IND packet) the ticker will not be stopped
		 * in time. To handle this, we simply ignore the extra ticker callback and
		 * wait for the usual ull_adv_done() handling to run
		 */
		if ((adv->max_events && adv->event_counter >= adv->max_events) ||
		    (adv->remain_duration_us &&
		     adv->remain_duration_us <= (uint64_t)adv->interval * ADV_INT_UNIT_US)) {
			return;
		}
	}
#endif /* CONFIG_BT_CTLR_ADV_EXT */

	if (IS_ENABLED(CONFIG_BT_TICKER_LOW_LAT) ||
	    (lazy != TICKER_LAZY_MUST_EXPIRE)) {
		/* Increment prepare reference count */
		ref = ull_ref_inc(&adv->ull);
		LL_ASSERT(ref);

		/* Append timing parameters */
		p.ticks_at_expire = ticks_at_expire;
		p.remainder = remainder;
		p.lazy = lazy;
		p.force = force;
		p.param = lll;
		mfy.param = &p;

#if defined(CONFIG_BT_CTLR_ADV_EXT) && (CONFIG_BT_CTLR_ADV_AUX_SET > 0)
		if (adv->lll.aux) {
			uint32_t ticks_to_expire;
			uint32_t other_remainder;

			LL_ASSERT(context->other_expire_info);

			/* Adjust ticks to expire based on remainder value */
			ticks_to_expire = context->other_expire_info->ticks_to_expire;
			other_remainder = context->other_expire_info->remainder;
			hal_ticker_remove_jitter(&ticks_to_expire, &other_remainder);

			/* Store the ticks and remainder offset for aux ptr population in LLL */
			adv->lll.aux->ticks_pri_pdu_offset = ticks_to_expire;
			adv->lll.aux->us_pri_pdu_offset = other_remainder;
		}
#endif /* CONFIG_BT_CTLR_ADV_EXT && (CONFIG_BT_CTLR_ADV_AUX_SET > 0) */

		/* Kick LLL prepare */
		ret = mayfly_enqueue(TICKER_USER_ID_ULL_HIGH,
				     TICKER_USER_ID_LLL, 0, &mfy);
		LL_ASSERT(!ret);

#if defined(CONFIG_BT_CTLR_JIT_SCHEDULING)
		adv->ticks_at_expire = ticks_at_expire;
		adv->delay_at_expire = adv->delay;
#endif /* CONFIG_BT_CTLR_JIT_SCHEDULING */
	}

	/* Apply adv random delay */
#if defined(CONFIG_BT_PERIPHERAL)
	if (!lll->is_hdcd)
#endif /* CONFIG_BT_PERIPHERAL */
	{
		/* Apply random delay in range [0..ULL_ADV_RANDOM_DELAY] */
		random_delay = ticker_update_rand(adv, ULL_ADV_RANDOM_DELAY,
						  0, 0, ticker_update_op_cb);

#if defined(CONFIG_BT_CTLR_ADV_EXT)
		if (adv->remain_duration_us && adv->event_counter > 0U) {
#if defined(CONFIG_BT_CTLR_JIT_SCHEDULING)
			/* ticks_drift is always 0 with JIT scheduling, populate manually */
			ticks_drift = adv->delay_at_expire;
#endif /* CONFIG_BT_CTLR_JIT_SCHEDULING */
			uint32_t interval_us = (uint64_t)adv->interval * ADV_INT_UNIT_US;
			uint32_t elapsed_us = interval_us * (lazy + 1U) +
						 HAL_TICKER_TICKS_TO_US(ticks_drift);

			/* End advertising if the added random delay pushes us beyond the limit */
			if (adv->remain_duration_us > elapsed_us + interval_us +
						      HAL_TICKER_TICKS_TO_US(random_delay)) {
				adv->remain_duration_us -= elapsed_us;
			} else {
				adv->remain_duration_us = interval_us;
			}
		}

		adv->event_counter += (lazy + 1U);
#endif /* CONFIG_BT_CTLR_ADV_EXT */
	}

	DEBUG_RADIO_PREPARE_A(1);
}

static void ticker_update_op_cb(uint32_t status, void *param)
{
	LL_ASSERT(status == TICKER_STATUS_SUCCESS ||
		  param == ull_disable_mark_get());
}

#if defined(CONFIG_BT_PERIPHERAL)
static void ticker_stop_cb(uint32_t ticks_at_expire, uint32_t ticks_drift,
			   uint32_t remainder, uint16_t lazy, uint8_t force,
			   void *param)
{
	struct ll_adv_set *adv = param;
	uint8_t handle;
	uint32_t ret;

	handle = ull_adv_handle_get(adv);
	LL_ASSERT(handle < BT_CTLR_ADV_SET);

	ret = ticker_stop(TICKER_INSTANCE_ID_CTLR, TICKER_USER_ID_ULL_HIGH,
			  TICKER_ID_ADV_BASE + handle,
			  ticker_stop_op_cb, adv);
	LL_ASSERT((ret == TICKER_STATUS_SUCCESS) ||
		  (ret == TICKER_STATUS_BUSY));
}

static void ticker_stop_op_cb(uint32_t status, void *param)
{
	static memq_link_t link;
	static struct mayfly mfy = {0, 0, &link, NULL, adv_disable};
	uint32_t ret;

	/* Ignore if race between thread and ULL */
	if (status != TICKER_STATUS_SUCCESS) {
		/* TODO: detect race */

		return;
	}

#if defined(CONFIG_BT_HCI_MESH_EXT)
	/* FIXME: why is this here for Mesh commands? */
	if (param) {
		return;
	}
#endif /* CONFIG_BT_HCI_MESH_EXT */

	/* Check if any pending LLL events that need to be aborted */
	mfy.param = param;
	ret = mayfly_enqueue(TICKER_USER_ID_ULL_LOW,
			     TICKER_USER_ID_ULL_HIGH, 0, &mfy);
	LL_ASSERT(!ret);
}

static void adv_disable(void *param)
{
	struct ll_adv_set *adv;
	struct ull_hdr *hdr;

	/* Check ref count to determine if any pending LLL events in pipeline */
	adv = param;
	hdr = &adv->ull;
	if (ull_ref_get(hdr)) {
		static memq_link_t link;
		static struct mayfly mfy = {0, 0, &link, NULL, lll_disable};
		uint32_t ret;

		mfy.param = &adv->lll;

		/* Setup disabled callback to be called when ref count
		 * returns to zero.
		 */
		LL_ASSERT(!hdr->disabled_cb);
		hdr->disabled_param = mfy.param;
		hdr->disabled_cb = disabled_cb;

		/* Trigger LLL disable */
		ret = mayfly_enqueue(TICKER_USER_ID_ULL_HIGH,
				     TICKER_USER_ID_LLL, 0, &mfy);
		LL_ASSERT(!ret);
	} else {
		/* No pending LLL events */
		disabled_cb(&adv->lll);
	}
}

static void disabled_cb(void *param)
{
	struct ll_adv_set *adv;
	struct node_rx_pdu *rx;
	struct node_rx_cc *cc;
	memq_link_t *link;

	adv = ((struct lll_hdr *)param)->parent;

	LL_ASSERT(adv->link_cc_free);
	link = adv->link_cc_free;
	adv->link_cc_free = NULL;

	LL_ASSERT(adv->node_rx_cc_free);
	rx = adv->node_rx_cc_free;
	adv->node_rx_cc_free = NULL;

	rx->hdr.type = NODE_RX_TYPE_CONNECTION;
	rx->hdr.handle = 0xffff;

	cc = (void *)rx->pdu;
	memset(cc, 0x00, sizeof(struct node_rx_cc));
	cc->status = BT_HCI_ERR_ADV_TIMEOUT;

	rx->hdr.rx_ftr.param = param;

#if defined(CONFIG_BT_CTLR_ADV_EXT)
	if (adv->lll.node_rx_adv_term) {
		uint8_t handle;

		ll_rx_put(link, rx);

		handle = ull_adv_handle_get(adv);
		LL_ASSERT(handle < BT_CTLR_ADV_SET);

		rx = (void *)adv->lll.node_rx_adv_term;
		rx->hdr.type = NODE_RX_TYPE_EXT_ADV_TERMINATE;
		rx->hdr.handle = handle;
		rx->hdr.rx_ftr.param_adv_term.status = BT_HCI_ERR_ADV_TIMEOUT;
		rx->hdr.rx_ftr.param_adv_term.conn_handle = 0xffff;
		rx->hdr.rx_ftr.param_adv_term.num_events = adv->event_counter;

		link = rx->hdr.link;
	}
#endif /* CONFIG_BT_CTLR_ADV_EXT */

	ll_rx_put_sched(link, rx);
}

static void conn_release(struct ll_adv_set *adv)
{
	struct lll_conn *lll = adv->lll.conn;
	memq_link_t *link;

	LL_ASSERT(!lll->link_tx_free);
	link = memq_deinit(&lll->memq_tx.head, &lll->memq_tx.tail);
	LL_ASSERT(link);
	lll->link_tx_free = link;

	ll_conn_release(lll->hdr.parent);
	adv->lll.conn = NULL;

	ll_rx_release(adv->node_rx_cc_free);
	adv->node_rx_cc_free = NULL;
	ll_rx_link_release(adv->link_cc_free);
	adv->link_cc_free = NULL;
}
#endif /* CONFIG_BT_PERIPHERAL */

#if defined(CONFIG_BT_CTLR_ADV_EXT)
static uint8_t leg_adv_type_get(uint8_t evt_prop)
{
	/* We take advantage of the fact that 2 LS bits
	 * of evt_prop can be used in a lookup to return
	 * PDU type value in the pdu_adv_type[] lookup.
	 */
	uint8_t const leg_adv_type[] = {
		0x03, /* index of PDU_ADV_TYPE_NONCONN_IND in pdu_adv_type[] */
		0x04, /* index of PDU_ADV_TYPE_DIRECT_IND in pdu_adv_type[] */
		0x02, /* index of PDU_ADV_TYPE_SCAN_IND in pdu_adv_type[] */
		0x00  /* index of PDU_ADV_TYPE_ADV_IND in pdu_adv_type[] */
	};

	/* if high duty cycle directed */
	if (evt_prop & BT_HCI_LE_ADV_PROP_HI_DC_CONN) {
		/* index of PDU_ADV_TYPE_DIRECT_IND in pdu_adv_type[] */
		return 0x01;
	}

	return leg_adv_type[evt_prop & 0x03];
}

static void adv_max_events_duration_set(struct ll_adv_set *adv,
					uint16_t duration,
					uint8_t max_ext_adv_evts)
{
	adv->event_counter = 0;
	adv->max_events = max_ext_adv_evts;
	adv->remain_duration_us = (uint32_t)duration * 10U * USEC_PER_MSEC;
}

static void ticker_stop_aux_op_cb(uint32_t status, void *param)
{
	static memq_link_t link;
	static struct mayfly mfy = {0, 0, &link, NULL, aux_disable};
	uint32_t ret;

	LL_ASSERT(status == TICKER_STATUS_SUCCESS);

	/* Check if any pending LLL events that need to be aborted */
	mfy.param = param;
	ret = mayfly_enqueue(TICKER_USER_ID_ULL_LOW,
			     TICKER_USER_ID_ULL_HIGH, 0, &mfy);
	LL_ASSERT(!ret);
}

static void aux_disable(void *param)
{
	struct lll_adv_aux *lll_aux;
	struct ll_adv_aux_set *aux;
	struct ll_adv_set *adv;
	struct ull_hdr *hdr;

	adv = param;
	lll_aux = adv->lll.aux;
	aux = HDR_LLL2ULL(lll_aux);
	hdr = &aux->ull;
	if (ull_ref_get(hdr)) {
		LL_ASSERT(!hdr->disabled_cb);
		hdr->disabled_param = adv;
		hdr->disabled_cb = aux_disabled_cb;
	} else {
		aux_disabled_cb(param);
	}
}

static void aux_disabled_cb(void *param)
{
	uint8_t handle;
	uint32_t ret;

	handle = ull_adv_handle_get(param);
	ret = ticker_stop(TICKER_INSTANCE_ID_CTLR,
			  TICKER_USER_ID_ULL_HIGH,
			  (TICKER_ID_ADV_BASE + handle),
			  ticker_stop_ext_op_cb, param);
	LL_ASSERT((ret == TICKER_STATUS_SUCCESS) ||
		  (ret == TICKER_STATUS_BUSY));
}

static void ticker_stop_ext_op_cb(uint32_t status, void *param)
{
	static memq_link_t link;
	static struct mayfly mfy = {0, 0, &link, NULL, ext_disable};
	uint32_t ret;

	/* Ignore if race between thread and ULL */
	if (status != TICKER_STATUS_SUCCESS) {
		/* TODO: detect race */

		return;
	}

	/* Check if any pending LLL events that need to be aborted */
	mfy.param = param;
	ret = mayfly_enqueue(TICKER_USER_ID_ULL_LOW,
			     TICKER_USER_ID_ULL_HIGH, 0, &mfy);
	LL_ASSERT(!ret);
}

static void ext_disable(void *param)
{
	struct ll_adv_set *adv;
	struct ull_hdr *hdr;

	/* Check ref count to determine if any pending LLL events in pipeline */
	adv = param;
	hdr = &adv->ull;
	if (ull_ref_get(hdr)) {
		static memq_link_t link;
		static struct mayfly mfy = {0, 0, &link, NULL, lll_disable};
		uint32_t ret;

		mfy.param = &adv->lll;

		/* Setup disabled callback to be called when ref count
		 * returns to zero.
		 */
		LL_ASSERT(!hdr->disabled_cb);
		hdr->disabled_param = mfy.param;
		hdr->disabled_cb = ext_disabled_cb;

		/* Trigger LLL disable */
		ret = mayfly_enqueue(TICKER_USER_ID_ULL_HIGH,
				     TICKER_USER_ID_LLL, 0, &mfy);
		LL_ASSERT(!ret);
	} else {
		/* No pending LLL events */
		ext_disabled_cb(&adv->lll);
	}
}

static void ext_disabled_cb(void *param)
{
	struct lll_adv *lll = (void *)param;
	struct node_rx_hdr *rx_hdr = (void *)lll->node_rx_adv_term;

	/* Under race condition, if a connection has been established then
	 * node_rx is already utilized to send terminate event on connection
	 */
	if (!rx_hdr) {
		return;
	}

	/* NOTE: parameters are already populated on disable, just enqueue here
	 */
	ll_rx_put_sched(rx_hdr->link, rx_hdr);
}
#endif /* CONFIG_BT_CTLR_ADV_EXT */

static inline uint8_t disable(uint8_t handle)
{
	uint32_t volatile ret_cb;
	struct ll_adv_set *adv;
	uint32_t ret;
	void *mark;
	int err;

	adv = ull_adv_is_enabled_get(handle);
	if (!adv) {
		/* Bluetooth Specification v5.0 Vol 2 Part E Section 7.8.9
		 * Disabling advertising when it is already disabled has no
		 * effect.
		 */
		if (!IS_ENABLED(CONFIG_BT_CTLR_ADV_ENABLE_STRICT)) {
			return 0;
		}

		return BT_HCI_ERR_CMD_DISALLOWED;
	}

#if defined(CONFIG_BT_PERIPHERAL)
	if (adv->lll.conn) {
		/* Indicate to LLL that a cancellation is requested */
		adv->lll.conn->periph.cancelled = 1U;
		cpu_dmb();

		/* Check if a connection was initiated (connection
		 * establishment race between LLL and ULL).
		 */
		if (unlikely(adv->lll.conn->periph.initiated)) {
			return BT_HCI_ERR_CMD_DISALLOWED;
		}
	}
#endif /* CONFIG_BT_PERIPHERAL */

	mark = ull_disable_mark(adv);
	LL_ASSERT(mark == adv);

#if defined(CONFIG_BT_PERIPHERAL)
	if (adv->lll.is_hdcd) {
		ret_cb = TICKER_STATUS_BUSY;
		ret = ticker_stop(TICKER_INSTANCE_ID_CTLR,
				  TICKER_USER_ID_THREAD, TICKER_ID_ADV_STOP,
				  ull_ticker_status_give, (void *)&ret_cb);
		ret = ull_ticker_status_take(ret, &ret_cb);
		if (ret) {
			mark = ull_disable_unmark(adv);
			LL_ASSERT(mark == adv);

			return BT_HCI_ERR_CMD_DISALLOWED;
		}
	}
#endif /* CONFIG_BT_PERIPHERAL */

	ret_cb = TICKER_STATUS_BUSY;
	ret = ticker_stop(TICKER_INSTANCE_ID_CTLR, TICKER_USER_ID_THREAD,
			  TICKER_ID_ADV_BASE + handle,
			  ull_ticker_status_give, (void *)&ret_cb);
	ret = ull_ticker_status_take(ret, &ret_cb);
	if (ret) {
		mark = ull_disable_unmark(adv);
		LL_ASSERT(mark == adv);

		return BT_HCI_ERR_CMD_DISALLOWED;
	}

	err = ull_disable(&adv->lll);
	LL_ASSERT(!err || (err == -EALREADY));

	mark = ull_disable_unmark(adv);
	LL_ASSERT(mark == adv);

#if defined(CONFIG_BT_CTLR_ADV_EXT) && (CONFIG_BT_CTLR_ADV_AUX_SET > 0)
	struct lll_adv_aux *lll_aux = adv->lll.aux;

	if (lll_aux) {
		struct ll_adv_aux_set *aux;

		aux = HDR_LLL2ULL(lll_aux);

		err = ull_adv_aux_stop(aux);
		if (err && (err != -EALREADY)) {
			return BT_HCI_ERR_CMD_DISALLOWED;
		}
	}
#endif /* CONFIG_BT_CTLR_ADV_EXT && (CONFIG_BT_CTLR_ADV_AUX_SET > 0) */

#if defined(CONFIG_BT_PERIPHERAL)
	if (adv->lll.conn) {
		conn_release(adv);
	}
#endif /* CONFIG_BT_PERIPHERAL */

#if defined(CONFIG_BT_CTLR_ADV_EXT)
	struct lll_adv *lll = &adv->lll;

	if (lll->node_rx_adv_term) {
		struct node_rx_pdu *node_rx_adv_term =
			(void *)lll->node_rx_adv_term;

		lll->node_rx_adv_term = NULL;

		ll_rx_link_release(node_rx_adv_term->hdr.link);
		ll_rx_release(node_rx_adv_term);
	}
#endif /* CONFIG_BT_CTLR_ADV_EXT */

	adv->is_enabled = 0U;

#if defined(CONFIG_BT_CTLR_PRIVACY)
	if (!IS_ENABLED(CONFIG_BT_OBSERVER) || !ull_scan_is_enabled_get(0)) {
		ull_filter_adv_scan_state_cb(0);
	}
#endif /* CONFIG_BT_CTLR_PRIVACY */

	return 0;
}

static uint8_t adv_scan_pdu_addr_update(struct ll_adv_set *adv,
					struct pdu_adv *pdu,
					struct pdu_adv *pdu_scan)
{
	struct pdu_adv *pdu_adv_to_update;
	struct lll_adv *lll;

	pdu_adv_to_update = NULL;
	lll = &adv->lll;

	if (0) {
#if defined(CONFIG_BT_CTLR_ADV_EXT)
	} else if (pdu->type == PDU_ADV_TYPE_EXT_IND) {
		struct pdu_adv_com_ext_adv *pri_com_hdr;
		struct pdu_adv_ext_hdr pri_hdr_flags;
		struct pdu_adv_ext_hdr *pri_hdr;

		pri_com_hdr = (void *)&pdu->adv_ext_ind;
		pri_hdr = (void *)pri_com_hdr->ext_hdr_adv_data;
		if (pri_com_hdr->ext_hdr_len) {
			pri_hdr_flags = *pri_hdr;
		} else {
			*(uint8_t *)&pri_hdr_flags = 0U;
		}

		if (pri_com_hdr->adv_mode & BT_HCI_LE_ADV_PROP_SCAN) {
			struct pdu_adv *sr = lll_adv_scan_rsp_peek(lll);

			if (!sr->len) {
				return BT_HCI_ERR_CMD_DISALLOWED;
			}
		}

		/* AdvA, fill here at enable */
		if (pri_hdr_flags.adv_addr) {
			pdu_adv_to_update = pdu;
#if (CONFIG_BT_CTLR_ADV_AUX_SET > 0)
		} else if (pri_hdr_flags.aux_ptr) {
			struct pdu_adv_com_ext_adv *sec_com_hdr;
			struct pdu_adv_ext_hdr sec_hdr_flags;
			struct pdu_adv_ext_hdr *sec_hdr;
			struct pdu_adv *sec_pdu;

			sec_pdu = lll_adv_aux_data_peek(lll->aux);

			sec_com_hdr = (void *)&sec_pdu->adv_ext_ind;
			sec_hdr = (void *)sec_com_hdr->ext_hdr_adv_data;
			if (sec_com_hdr->ext_hdr_len) {
				sec_hdr_flags = *sec_hdr;
			} else {
				*(uint8_t *)&sec_hdr_flags = 0U;
			}

			if (sec_hdr_flags.adv_addr) {
				pdu_adv_to_update = sec_pdu;
			}
#endif /* (CONFIG_BT_CTLR_ADV_AUX_SET > 0) */
		}
#endif /* CONFIG_BT_CTLR_ADV_EXT */
	} else {
		pdu_adv_to_update = pdu;
	}

	if (pdu_adv_to_update) {
		const uint8_t *adv_addr;

		adv_addr = ull_adv_pdu_update_addrs(adv, pdu_adv_to_update);

		/* In case the local IRK was not set or no match was
		 * found the fallback address was used instead, check
		 * that a valid address has been set.
		 */
		if (pdu_adv_to_update->tx_addr &&
		    !mem_nz((void *)adv_addr, BDADDR_SIZE)) {
			return BT_HCI_ERR_INVALID_PARAM;
		}

#if defined(CONFIG_BT_CTLR_ADV_EXT)
		/* Do not update scan response for extended non-scannable since
		 * there may be no scan response set.
		 */
		if ((pdu->type != PDU_ADV_TYPE_EXT_IND) ||
		    (pdu->adv_ext_ind.adv_mode & BT_HCI_LE_ADV_PROP_SCAN)) {
#else
		if (1) {
#endif
			ull_adv_pdu_update_addrs(adv, pdu_scan);
		}

	}

	return 0;
}

static inline uint8_t *adv_pdu_adva_get(struct pdu_adv *pdu)
{
#if defined(CONFIG_BT_CTLR_ADV_EXT)
	struct pdu_adv_com_ext_adv *com_hdr = (void *)&pdu->adv_ext_ind;
	struct pdu_adv_ext_hdr *hdr = (void *)com_hdr->ext_hdr_adv_data;
	struct pdu_adv_ext_hdr hdr_flags;

	if (com_hdr->ext_hdr_len) {
		hdr_flags = *hdr;
	} else {
		*(uint8_t *)&hdr_flags = 0U;
	}

	/* All extended PDUs have AdvA at the same offset in common header */
	if (pdu->type == PDU_ADV_TYPE_EXT_IND) {
		LL_ASSERT(hdr_flags.adv_addr);

		return &com_hdr->ext_hdr_adv_data[1];
	}
#endif

	/* All legacy PDUs have AdvA at the same offset */
	return pdu->adv_ind.addr;
}

static const uint8_t *adva_update(struct ll_adv_set *adv, struct pdu_adv *pdu)
{
#if defined(CONFIG_BT_CTLR_PRIVACY)
	const uint8_t *rpa = ull_filter_adva_get(adv->lll.rl_idx);
#else
	const uint8_t *rpa = NULL;
#endif
	const uint8_t *own_id_addr;
	const uint8_t *tx_addr;
	uint8_t *adv_addr;

	if (!rpa || IS_ENABLED(CONFIG_BT_CTLR_CHECK_SAME_PEER_CONN)) {
		if (0) {
#if defined(CONFIG_BT_CTLR_ADV_EXT)
		} else if (ll_adv_cmds_is_ext() && pdu->tx_addr) {
			own_id_addr = adv->rnd_addr;
#endif
		} else {
			own_id_addr = ll_addr_get(pdu->tx_addr);
		}
	}

#if defined(CONFIG_BT_CTLR_CHECK_SAME_PEER_CONN)
	(void)memcpy(adv->own_id_addr, own_id_addr, BDADDR_SIZE);
#endif /* CONFIG_BT_CTLR_CHECK_SAME_PEER_CONN */

	if (rpa) {
		pdu->tx_addr = 1;
		tx_addr = rpa;
	} else {
		tx_addr = own_id_addr;
	}

	adv_addr = adv_pdu_adva_get(pdu);
	memcpy(adv_addr, tx_addr, BDADDR_SIZE);

	return adv_addr;
}

static void tgta_update(struct ll_adv_set *adv, struct pdu_adv *pdu)
{
#if defined(CONFIG_BT_CTLR_PRIVACY)
	const uint8_t *rx_addr = NULL;
	uint8_t *tgt_addr;

	rx_addr = ull_filter_tgta_get(adv->lll.rl_idx);
	if (rx_addr) {
		pdu->rx_addr = 1;

		/* TargetA always follows AdvA in all PDUs */
		tgt_addr = adv_pdu_adva_get(pdu) + BDADDR_SIZE;
		memcpy(tgt_addr, rx_addr, BDADDR_SIZE);
	}
#endif

	/* NOTE: identity TargetA is set when configuring advertising set, no
	 *       need to update if LL Privacy is not supported.
	 */
}

static void init_pdu(struct pdu_adv *pdu, uint8_t pdu_type)
{
	/* TODO: Add support for extended advertising PDU if needed */
	pdu->type = pdu_type;
	pdu->rfu = 0;
	pdu->chan_sel = 0;
	pdu->tx_addr = 0;
	pdu->rx_addr = 0;
	pdu->len = BDADDR_SIZE;
}

static void init_set(struct ll_adv_set *adv)
{
	adv->interval = BT_LE_ADV_INTERVAL_DEFAULT;
#if defined(CONFIG_BT_CTLR_PRIVACY)
	adv->own_addr_type = BT_ADDR_LE_PUBLIC;
#endif /* CONFIG_BT_CTLR_PRIVACY */
	adv->lll.chan_map = BT_LE_ADV_CHAN_MAP_ALL;
	adv->lll.filter_policy = BT_LE_ADV_FP_NO_FILTER;
#if defined(CONFIG_BT_CTLR_JIT_SCHEDULING)
	adv->delay = 0U;
#endif /* ONFIG_BT_CTLR_JIT_SCHEDULING */

	init_pdu(lll_adv_data_peek(&ll_adv[0].lll), PDU_ADV_TYPE_ADV_IND);

#if !defined(CONFIG_BT_CTLR_ADV_EXT)
	init_pdu(lll_adv_scan_rsp_peek(&ll_adv[0].lll), PDU_ADV_TYPE_SCAN_RSP);
#endif /* !CONFIG_BT_CTLR_ADV_EXT */
}
