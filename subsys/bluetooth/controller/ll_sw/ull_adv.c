/*
 * Copyright (c) 2016-2021 Nordic Semiconductor ASA
 * Copyright (c) 2016 Vinayak Kariappa Chettimada
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>

#include <zephyr.h>
#include <soc.h>
#include <bluetooth/hci.h>
#include <sys/byteorder.h>

#include "hal/cpu.h"
#include "hal/ccm.h"
#include "hal/radio.h"
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
#include "lll_scan.h"
#include "lll_conn.h"
#include "lll_filter.h"

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

#define BT_DBG_ENABLED IS_ENABLED(CONFIG_BT_DEBUG_HCI_DRIVER)
#define LOG_MODULE_NAME bt_ctlr_ull_adv
#include "common/log.h"
#include "hal/debug.h"

inline struct ll_adv_set *ull_adv_set_get(uint8_t handle);
inline uint16_t ull_adv_handle_get(struct ll_adv_set *adv);

static int init_reset(void);
static inline struct ll_adv_set *is_disabled_get(uint8_t handle);
static uint16_t adv_time_get(struct pdu_adv *pdu, struct pdu_adv *pdu_scan,
			     uint8_t adv_chn_cnt, uint8_t phy);
static void ticker_cb(uint32_t ticks_at_expire, uint32_t remainder,
		      uint16_t lazy, uint8_t force, void *param);
static void ticker_op_update_cb(uint32_t status, void *param);

#if defined(CONFIG_BT_PERIPHERAL)
static void ticker_stop_cb(uint32_t ticks_at_expire, uint32_t remainder,
			   uint16_t lazy, uint8_t force, void *param);
static void ticker_op_stop_cb(uint32_t status, void *param);
static void disabled_cb(void *param);
static void conn_release(struct ll_adv_set *adv);
#endif /* CONFIG_BT_PERIPHERAL */

#if defined(CONFIG_BT_CTLR_ADV_EXT)
static void ticker_op_aux_stop_cb(uint32_t status, void *param);
static void ticker_op_ext_stop_cb(uint32_t status, void *param);
static void ext_disabled_cb(void *param);
#endif /* CONFIG_BT_CTLR_ADV_EXT */

static inline uint8_t disable(uint8_t handle);

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
			/* lookup evt_prop to PDU type in  pdu_adv_type[] */
			uint8_t const leg_adv_type[] = {
				0x03, /* PDU_ADV_TYPE_NONCONN_IND */
				0x04, /* PDU_ADV_TYPE_DIRECT_IND */
				0x02, /* PDU_ADV_TYPE_SCAN_IND */
				0x00  /* PDU_ADV_TYPE_ADV_IND */
			};

			if (evt_prop & BT_HCI_LE_ADV_PROP_ANON) {
				return BT_HCI_ERR_UNSUPP_FEATURE_PARAM_VAL;
			}

			adv_type = leg_adv_type[evt_prop & 0x03];

			/* high duty cycle directed */
			if (evt_prop & BT_HCI_LE_ADV_PROP_HI_DC_CONN) {
				adv_type = 0x01; /* PDU_ADV_TYPE_DIRECT_IND */
			}

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

#if (CONFIG_BT_CTLR_ADV_AUX_SET == 0)
			/* Connectable or scannable requires aux */
			if (evt_prop & (BT_HCI_LE_ADV_PROP_CONN |
					BT_HCI_LE_ADV_PROP_SCAN)) {
				return BT_HCI_ERR_MEM_CAPACITY_EXCEEDED;
			}
#endif

			adv_type = 0x05; /* PDU_ADV_TYPE_EXT_IND in */
					 /* pdu_adv_type array. */

			adv->lll.phy_p = phy_p;
		}
	} else {
		adv->lll.phy_p = PHY_1M;
	}

	is_new_set = !adv->is_created;
	adv->is_created = 1;
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
		    (!pri_hdr_prev.aux_ptr ||
		     (!(evt_prop & BT_HCI_LE_ADV_PROP_ANON) &&
		      (phy_p != PHY_CODED)))) {
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
			ull_adv_aux_ptr_fill(&pri_dptr, phy_s);
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
			adi->sid = sid;
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
			uint8_t pri_idx;
			uint8_t err;

			err = ull_adv_aux_hdr_set_clear(adv,
							ULL_ADV_PDU_HDR_FIELD_ADVA,
							0, &own_addr_type,
							NULL, &pri_idx);
			if (err) {
				/* TODO: cleanup? */
				return err;
			}

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

	if (0) {
#if defined(CONFIG_BT_CTLR_ADV_EXT)
	} else if (pdu->type == PDU_ADV_TYPE_EXT_IND) {
		/* Make sure new extended advertising set is initialized with no
		 * scan response data. Existing sets keep whatever data was set.
		 */
		if (is_new_set) {
			pdu = lll_adv_scan_rsp_peek(&adv->lll);
			pdu->type = PDU_ADV_TYPE_AUX_SCAN_REQ;
			pdu->len = 0;
		}
#endif /* CONFIG_BT_CTLR_ADV_EXT */
	} else {
		/* Make sure legacy advertising set has scan response data
		 * initialized.
		 */
		pdu = lll_adv_scan_rsp_peek(&adv->lll);
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
	struct pdu_adv *pdu_adv_to_update;
	uint32_t ticks_slot_overhead;
	uint32_t ticks_slot_offset;
	uint32_t volatile ret_cb;
	struct pdu_adv *pdu_scan;
	struct pdu_adv *pdu_adv;
	struct ll_adv_set *adv;
	struct lll_adv *lll;
	uint32_t ret;

	if (!enable) {
		return disable(handle);
	}

	adv = is_disabled_get(handle);
	if (!adv) {
		return BT_HCI_ERR_CMD_DISALLOWED;
	}

	lll = &adv->lll;

	pdu_adv = lll_adv_data_peek(lll);
	pdu_scan = lll_adv_scan_rsp_peek(lll);
	pdu_adv_to_update = NULL;

	if (0) {
#if defined(CONFIG_BT_CTLR_ADV_EXT)
	} else if (pdu_adv->type == PDU_ADV_TYPE_EXT_IND) {
		struct pdu_adv_com_ext_adv *pri_com_hdr;
		struct pdu_adv_ext_hdr pri_hdr_flags;
		struct pdu_adv_ext_hdr *pri_hdr;

		pri_com_hdr = (void *)&pdu_adv->adv_ext_ind;
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
			pdu_adv_to_update = pdu_adv;
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
		pdu_adv_to_update = pdu_adv;
	}

#if defined(CONFIG_BT_CTLR_PRIVACY)
	lll->rl_idx = FILTER_IDX_NONE;

	/* Prepare whitelist and optionally resolving list */
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
		if ((pdu_adv->type != PDU_ADV_TYPE_EXT_IND) ||
		    (pdu_adv->adv_ext_ind.adv_mode & BT_HCI_LE_ADV_PROP_SCAN)) {
#else
		if (1) {
#endif
			ull_adv_pdu_update_addrs(adv, pdu_scan);
		}

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

#if defined(CONFIG_BT_CTLR_DATA_LENGTH)
		conn_lll->max_tx_octets = PDU_DC_PAYLOAD_SIZE_MIN;
		conn_lll->max_rx_octets = PDU_DC_PAYLOAD_SIZE_MIN;

#if defined(CONFIG_BT_CTLR_PHY)
		/* Use the default 1M packet max time */
		conn_lll->max_tx_time = PKT_US(PDU_DC_PAYLOAD_SIZE_MIN, PHY_1M);
		conn_lll->max_rx_time = PKT_US(PDU_DC_PAYLOAD_SIZE_MIN, PHY_1M);
#if defined(CONFIG_BT_CTLR_ADV_EXT)
		conn_lll->max_tx_time = MAX(conn_lll->max_tx_time,
					    PKT_US(PDU_DC_PAYLOAD_SIZE_MIN,
						   lll->phy_s));
		conn_lll->max_rx_time = MAX(conn_lll->max_rx_time,
					    PKT_US(PDU_DC_PAYLOAD_SIZE_MIN,
						   lll->phy_s));
#endif /* CONFIG_BT_CTLR_ADV_EXT */
#endif /* CONFIG_BT_CTLR_PHY */
#endif /* CONFIG_BT_CTLR_DATA_LENGTH */

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
		conn_lll->slave.initiated = 0;
		conn_lll->slave.cancelled = 0;
		conn_lll->data_chan_sel = 0;
		conn_lll->data_chan_use = 0;
		conn_lll->event_counter = 0;

		conn_lll->latency_prepare = 0;
		conn_lll->latency_event = 0;
		conn_lll->slave.latency_enabled = 0;
		conn_lll->slave.window_widening_prepare_us = 0;
		conn_lll->slave.window_widening_event_us = 0;
		conn_lll->slave.window_size_prepare_us = 0;
		/* FIXME: END: Move to ULL? */
#if defined(CONFIG_BT_CTLR_CONN_META)
		memset(&conn_lll->conn_meta, 0, sizeof(conn_lll->conn_meta));
#endif /* CONFIG_BT_CTLR_CONN_META */

		conn->connect_expire = 6;
		conn->supervision_expire = 0;
		conn->procedure_expire = 0;

#if defined(CONFIG_BT_CTLR_LE_PING)
		conn->apto_expire = 0U;
		conn->appto_expire = 0U;
#endif

#if defined(CONFIG_BT_CTLR_CHECK_SAME_PEER_CONN)
		conn->own_addr_type = BT_ADDR_LE_NONE->type;
		memcpy(conn->own_addr, BT_ADDR_LE_NONE->a.val, sizeof(conn->own_addr));
		conn->peer_addr_type = BT_ADDR_LE_NONE->type;
		memcpy(conn->peer_addr, BT_ADDR_LE_NONE->a.val, sizeof(conn->peer_addr));
#endif /* CONFIG_BT_CTLR_CHECK_SAME_PEER_CONN */

		conn->common.fex_valid = 0;
		conn->common.txn_lock = 0;
		conn->slave.latency_cancel = 0;

		conn->llcp_req = conn->llcp_ack = conn->llcp_type = 0;
		conn->llcp_rx = NULL;
		conn->llcp_cu.req = conn->llcp_cu.ack = 0;
		conn->llcp_feature.req = conn->llcp_feature.ack = 0;
		conn->llcp_feature.features_conn = ll_feat_get();
		conn->llcp_feature.features_peer = 0;
		conn->llcp_version.req = conn->llcp_version.ack = 0;
		conn->llcp_version.tx = conn->llcp_version.rx = 0;
		conn->llcp_terminate.req = conn->llcp_terminate.ack = 0;
		conn->llcp_terminate.reason_final = 0;
		/* NOTE: use allocated link for generating dedicated
		 * terminate ind rx node
		 */
		conn->llcp_terminate.node_rx.hdr.link = link;

#if defined(CONFIG_BT_CTLR_LE_ENC)
		conn_lll->enc_rx = conn_lll->enc_tx = 0U;
		conn->llcp_enc.req = conn->llcp_enc.ack = 0U;
		conn->llcp_enc.pause_tx = conn->llcp_enc.pause_rx = 0U;
		conn->llcp_enc.refresh = 0U;
		conn->slave.llcp_type = 0U;
#endif /* CONFIG_BT_CTLR_LE_ENC */

#if defined(CONFIG_BT_CTLR_CONN_PARAM_REQ)
		conn->llcp_conn_param.req = 0;
		conn->llcp_conn_param.ack = 0;
		conn->llcp_conn_param.disabled = 0;
		conn->slave.ticks_to_offset = 0;
#endif /* CONFIG_BT_CTLR_CONN_PARAM_REQ */

#if defined(CONFIG_BT_CTLR_DATA_LENGTH)
		conn->llcp_length.req = conn->llcp_length.ack = 0U;
		conn->llcp_length.disabled = 0U;
		conn->llcp_length.cache.tx_octets = 0U;
		conn->default_tx_octets = ull_conn_default_tx_octets_get();

#if defined(CONFIG_BT_CTLR_PHY)
		conn->default_tx_time = ull_conn_default_tx_time_get();
#endif /* CONFIG_BT_CTLR_PHY */
#endif /* CONFIG_BT_CTLR_DATA_LENGTH */

#if defined(CONFIG_BT_CTLR_PHY)
		conn->llcp_phy.req = conn->llcp_phy.ack = 0;
		conn->llcp_phy.disabled = 0U;
		conn->llcp_phy.pause_tx = 0U;
		conn->phy_pref_tx = ull_conn_default_phy_tx_get();
		conn->phy_pref_rx = ull_conn_default_phy_rx_get();
#endif /* CONFIG_BT_CTLR_PHY */

		conn->tx_head = conn->tx_ctrl = conn->tx_ctrl_last =
		conn->tx_data = conn->tx_data_last = 0;

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

		node_rx_adv_term->hdr.link = (void *)link_adv_term;
		adv->lll.node_rx_adv_term = (void *)node_rx_adv_term;
	}

	const uint8_t phy = lll->phy_p;

	adv->event_counter = 0;
	adv->max_events = max_ext_adv_evts;
	adv->ticks_remain_duration = HAL_TICKER_US_TO_TICKS((uint64_t)duration *
							    10000);
#else
	/* Legacy ADV only supports LE_1M PHY */
	const uint8_t phy = PHY_1M;
#endif

	/* For now we adv on all channels enabled in channel map */
	uint8_t ch_map = lll->chan_map;
	const uint8_t adv_chn_cnt = util_ones_count_get(&ch_map, sizeof(ch_map));

	if (adv_chn_cnt == 0) {
		/* ADV needs at least one channel */
		goto failure_cleanup;
	}

	/* Calculate the advertising time reservation */
	uint16_t time_us = adv_time_get(pdu_adv, pdu_scan, adv_chn_cnt, phy);

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
			interval = (interval_min_us +
				(SCAN_INT_UNIT_US - 1)) /
				SCAN_INT_UNIT_US;
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

	ull_hdr_init(&adv->ull);
	lll_hdr_init(lll, adv);

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
		ret = ticker_start(TICKER_INSTANCE_ID_CTLR,
				   TICKER_USER_ID_THREAD,
				   (TICKER_ID_ADV_BASE + handle),
				   ticks_anchor, 0,
				   (adv->ull.ticks_slot + ticks_slot_overhead),
				   TICKER_NULL_REMAINDER, TICKER_NULL_LAZY,
				   (adv->ull.ticks_slot + ticks_slot_overhead),
				   ticker_cb, adv,
				   ull_ticker_status_give, (void *)&ret_cb);
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
		uint8_t pri_idx = 0U;

		/* Add sync_info into auxiliary PDU */
		if (lll->sync) {
			sync = HDR_LLL2ULL(lll->sync);
			if (sync->is_enabled && !sync->is_started) {
				struct pdu_adv_sync_info *sync_info;
				uint8_t value[1 + sizeof(sync_info)];
				uint8_t err;

				err = ull_adv_aux_hdr_set_clear(adv,
					ULL_ADV_PDU_HDR_FIELD_SYNC_INFO,
					0, value, NULL, &pri_idx);
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

			ticks_slot_overhead_aux = ull_adv_aux_evt_init(aux);

#if defined(CONFIG_BT_CTLR_ADV_PERIODIC)
			/* Start periodic advertising if enabled and not already
			 * started.
			 */
			if (sync) {
				const uint32_t ticks_slot_aux =
					aux->ull.ticks_slot +
					ticks_slot_overhead_aux;

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
				uint32_t ticks_anchor_sync =
					ticks_anchor_aux + ticks_slot_aux +
					HAL_TICKER_US_TO_TICKS(
						MAX(EVENT_MAFS_US,
						    EVENT_OVERHEAD_START_US) -
						EVENT_OVERHEAD_START_US +
						(EVENT_TICKER_RES_MARGIN_US << 1));

				ret = ull_adv_sync_start(adv, sync,
							 ticks_anchor_sync);
				if (ret) {
					goto failure_cleanup;
				}

				sync_is_started = 1U;

				lll_adv_data_enqueue(lll, pri_idx);
			}
#endif /* CONFIG_BT_CTLR_ADV_PERIODIC */

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
				goto failure_cleanup;
			}

			aux_is_started = 1U;
		}
#endif /* (CONFIG_BT_CTLR_ADV_AUX_SET > 0) */

		ret_cb = TICKER_STATUS_BUSY;

#if defined(CONFIG_BT_TICKER_EXT)
		ll_adv_ticker_ext[handle].ticks_slot_window =
			ULL_ADV_RANDOM_DELAY + ticks_slot;

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
				   ticker_cb, adv,
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
	if (IS_ENABLED(CONFIG_BT_OBSERVER) && !ull_scan_is_enabled_get(0)) {
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

#if defined(CONFIG_BT_CTLR_ADV_AUX_SET)
	if (CONFIG_BT_CTLR_ADV_AUX_SET > 0) {
		int err;

		err = ull_adv_aux_reset();
		if (err) {
			return err;
		}
	}

#if defined(CONFIG_BT_CTLR_ADV_PERIODIC)
	{
		int err;

		err = ull_adv_sync_reset();
		if (err) {
			return err;
		}
	}
#endif /* CONFIG_BT_CTLR_ADV_PERIODIC */
#endif /* CONFIG_BT_CTLR_ADV_AUX_SET */
#endif /* CONFIG_BT_CTLR_ADV_EXT */

	return 0;
}

int ull_adv_reset_finalize(void)
{
	uint8_t handle;
	int err;

	for (handle = 0U; handle < BT_CTLR_ADV_SET; handle++) {
		struct ll_adv_set *adv = &ll_adv[handle];
		struct lll_adv *lll = &adv->lll;

#if defined(CONFIG_BT_CTLR_ADV_EXT)
		adv->is_created = 0;
		lll->aux = NULL;
#endif
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
#endif /* CONFIG_BT_CTLR_ADV_EXT */

uint8_t ull_adv_data_set(struct ll_adv_set *adv, uint8_t len,
			 uint8_t const *const data)
{
	struct pdu_adv *prev;
	struct pdu_adv *pdu;
	uint8_t idx;

	/* Check invalid AD Data length */
	if (len > PDU_AC_DATA_SIZE_MAX) {
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

	if (len > PDU_AC_DATA_SIZE_MAX) {
		return BT_HCI_ERR_INVALID_PARAM;
	}

	/* update scan pdu fields. */
	prev = lll_adv_scan_rsp_peek(&adv->lll);
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

#if defined(CONFIG_BT_CTLR_ADV_EXT)
void ull_adv_done(struct node_rx_event_done *done)
{
	struct lll_adv_aux *lll_aux;
	struct node_rx_hdr *rx_hdr;
	struct ll_adv_set *adv;
	struct lll_adv *lll;
	uint8_t handle;
	uint32_t ret;

	/* Get reference to ULL context */
	adv = CONTAINER_OF(done->param, struct ll_adv_set, ull);
	lll = &adv->lll;

	if (adv->max_events && (adv->event_counter >= adv->max_events)) {
		adv->max_events = 0;

		rx_hdr = (void *)lll->node_rx_adv_term;
		rx_hdr->rx_ftr.param_adv_term.status = BT_HCI_ERR_LIMIT_REACHED;
	} else if (adv->ticks_remain_duration &&
		   (adv->ticks_remain_duration <=
		    HAL_TICKER_US_TO_TICKS((uint64_t)adv->interval *
			ADV_INT_UNIT_US))) {
		adv->ticks_remain_duration = 0;

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
				  ticker_op_aux_stop_cb, adv);
	} else {
		ret = ticker_stop(TICKER_INSTANCE_ID_CTLR,
				  TICKER_USER_ID_ULL_HIGH,
				  (TICKER_ID_ADV_BASE + handle),
				  ticker_op_ext_stop_cb, adv);
	}

	LL_ASSERT((ret == TICKER_STATUS_SUCCESS) ||
		  (ret == TICKER_STATUS_BUSY));
}
#endif /* CONFIG_BT_CTLR_ADV_EXT */

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
	uint16_t time_us;
	uint8_t chan_map;
	uint8_t chan_cnt;
	uint32_t ret;

	lll = &adv->lll;
	chan_map = lll->chan_map;
	chan_cnt = util_ones_count_get(&chan_map, sizeof(chan_map));
	time_us = adv_time_get(pdu, pdu_scan, chan_cnt, PHY_1M);
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
		lll_adv_data_init(&ll_adv[handle].lll.scan_rsp);
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
			     uint8_t adv_chn_cnt, uint8_t phy)
{
	uint16_t time_us = EVENT_OVERHEAD_START_US + EVENT_OVERHEAD_END_US;

	/* Calculate the PDU Tx Time and hence the radio event length */
#if defined(CONFIG_BT_CTLR_ADV_EXT)
	if (pdu->type == PDU_ADV_TYPE_EXT_IND) {
		time_us += PKT_AC_US(pdu->len, phy) * adv_chn_cnt +
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

static void ticker_cb(uint32_t ticks_at_expire, uint32_t remainder, uint16_t lazy,
		      uint8_t force, void *param)
{
	static memq_link_t link;
	static struct mayfly mfy = {0, 0, &link, NULL, lll_adv_prepare};
	static struct lll_prepare_param p;
	struct ll_adv_set *adv = param;
	struct lll_adv *lll;
	uint32_t ret;
	uint8_t ref;

	DEBUG_RADIO_PREPARE_A(1);

	lll = &adv->lll;

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

		/* Kick LLL prepare */
		ret = mayfly_enqueue(TICKER_USER_ID_ULL_HIGH,
				     TICKER_USER_ID_LLL, 0, &mfy);
		LL_ASSERT(!ret);
	}

	/* Apply adv random delay */
#if defined(CONFIG_BT_PERIPHERAL)
	if (!lll->is_hdcd)
#endif /* CONFIG_BT_PERIPHERAL */
	{
		uint32_t random_delay;
		uint32_t ret;

		lll_rand_isr_get(&random_delay, sizeof(random_delay));
		random_delay %= ULL_ADV_RANDOM_DELAY;
		random_delay += 1;

		ret = ticker_update(TICKER_INSTANCE_ID_CTLR,
				    TICKER_USER_ID_ULL_HIGH,
				    (TICKER_ID_ADV_BASE +
				     ull_adv_handle_get(adv)),
				    random_delay,
				    0, 0, 0, 0, 0,
				    ticker_op_update_cb, adv);
		LL_ASSERT((ret == TICKER_STATUS_SUCCESS) ||
			  (ret == TICKER_STATUS_BUSY));

#if defined(CONFIG_BT_CTLR_ADV_EXT)
		adv->event_counter += (lazy + 1);

		if (adv->ticks_remain_duration) {
			uint32_t ticks_interval =
				HAL_TICKER_US_TO_TICKS((uint64_t)adv->interval *
						       ADV_INT_UNIT_US);
			uint32_t ticks_elapsed = ticks_interval * (lazy + 1);

			if (adv->ticks_remain_duration > ticks_elapsed) {
				adv->ticks_remain_duration -= ticks_elapsed;

				if (adv->ticks_remain_duration > random_delay) {
					adv->ticks_remain_duration -=
						random_delay;
				}
			} else {
				adv->ticks_remain_duration = ticks_interval;
			}
		}
#endif /* CONFIG_BT_CTLR_ADV_EXT */
	}

#if defined(CONFIG_BT_CTLR_ADV_EXT) && (CONFIG_BT_CTLR_ADV_AUX_SET > 0)
	if (adv->lll.aux) {
		ull_adv_aux_offset_get(adv);
	}
#endif /* CONFIG_BT_CTLR_ADV_EXT && (CONFIG_BT_CTLR_ADV_AUX_SET > 0) */

	DEBUG_RADIO_PREPARE_A(1);
}

static void ticker_op_update_cb(uint32_t status, void *param)
{
	LL_ASSERT(status == TICKER_STATUS_SUCCESS ||
		  param == ull_disable_mark_get());
}

#if defined(CONFIG_BT_PERIPHERAL)
static void ticker_stop_cb(uint32_t ticks_at_expire, uint32_t remainder,
			   uint16_t lazy, uint8_t force, void *param)
{
	struct ll_adv_set *adv = param;
	uint8_t handle;
	uint32_t ret;

	handle = ull_adv_handle_get(adv);
	LL_ASSERT(handle < BT_CTLR_ADV_SET);

	ret = ticker_stop(TICKER_INSTANCE_ID_CTLR, TICKER_USER_ID_ULL_HIGH,
			  TICKER_ID_ADV_BASE + handle,
			  ticker_op_stop_cb, adv);
	LL_ASSERT((ret == TICKER_STATUS_SUCCESS) ||
		  (ret == TICKER_STATUS_BUSY));
}

static void ticker_op_stop_cb(uint32_t status, void *param)
{
	static memq_link_t link;
	static struct mayfly mfy = {0, 0, &link, NULL, NULL};
	struct ll_adv_set *adv;
	struct ull_hdr *hdr;

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

	/* NOTE: We are in ULL_LOW which can be pre-empted by ULL_HIGH.
	 *       As we are in the callback after successful stop of the
	 *       ticker, the ULL reference count will not be modified
	 *       further hence it is safe to check and act on either the need
	 *       to call lll_disable or not.
	 */
	adv = param;
	hdr = &adv->ull;
	mfy.param = &adv->lll;
	if (ull_ref_get(hdr)) {
		uint32_t ret;

		LL_ASSERT(!hdr->disabled_cb);
		hdr->disabled_param = mfy.param;
		hdr->disabled_cb = disabled_cb;

		mfy.fp = lll_disable;
		ret = mayfly_enqueue(TICKER_USER_ID_ULL_LOW,
				     TICKER_USER_ID_LLL, 0, &mfy);
		LL_ASSERT(!ret);
	} else {
		uint32_t ret;

		mfy.fp = disabled_cb;
		ret = mayfly_enqueue(TICKER_USER_ID_ULL_LOW,
				     TICKER_USER_ID_ULL_HIGH, 0, &mfy);
		LL_ASSERT(!ret);
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

	ll_rx_put(link, rx);
	ll_rx_sched();
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
static void ticker_op_aux_stop_cb(uint32_t status, void *param)
{
	uint8_t handle;
	uint32_t ret;

	LL_ASSERT(status == TICKER_STATUS_SUCCESS);

	handle = ull_adv_handle_get(param);
	ret = ticker_stop(TICKER_INSTANCE_ID_CTLR, TICKER_USER_ID_ULL_LOW,
			  (TICKER_ID_ADV_BASE + handle), ticker_op_ext_stop_cb,
			  param);
	LL_ASSERT((ret == TICKER_STATUS_SUCCESS) ||
		  (ret == TICKER_STATUS_BUSY));
}

static void ticker_op_ext_stop_cb(uint32_t status, void *param)
{
	static memq_link_t link;
	static struct mayfly mfy = {0, 0, &link, NULL, NULL};
	struct ll_adv_set *adv;
	uint32_t ret;

	/* Ignore if race between thread and ULL */
	if (status != TICKER_STATUS_SUCCESS) {
		/* TODO: detect race */

		return;
	}

	adv = param;
	mfy.fp = ext_disabled_cb;
	mfy.param = &adv->lll;
	ret = mayfly_enqueue(TICKER_USER_ID_ULL_LOW, TICKER_USER_ID_ULL_HIGH, 0,
			     &mfy);
	LL_ASSERT(!ret);
}

static void ext_disabled_cb(void *param)
{
	struct lll_adv *lll = (void *)param;
	struct node_rx_hdr *rx_hdr = (void *)lll->node_rx_adv_term;

	/* Under race condition, if a connection has been established then
	 * node_rx is already utilized to send terminate event on connection */
	if (!rx_hdr) {
		return;
	}

	/* NOTE: parameters are already populated on disable, just enqueue here
	 */
	ll_rx_put(rx_hdr->link, rx_hdr);
	ll_rx_sched();
}
#endif /* CONFIG_BT_CTLR_ADV_EXT */

static inline uint8_t disable(uint8_t handle)
{
	uint32_t volatile ret_cb;
	struct ll_adv_set *adv;
	void *mark;
	uint32_t ret;

	adv = ull_adv_is_enabled_get(handle);
	if (!adv) {
		return BT_HCI_ERR_CMD_DISALLOWED;
	}

#if defined(CONFIG_BT_PERIPHERAL)
	if (adv->lll.conn) {
		/* Indicate to LLL that a cancellation is requested */
		adv->lll.conn->slave.cancelled = 1U;
		cpu_dmb();

		/* Check if a connection was initiated (connection
		 * establishment race between LLL and ULL).
		 */
		if (unlikely(adv->lll.conn->slave.initiated)) {
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

	ret = ull_disable(&adv->lll);
	LL_ASSERT(!ret);

	mark = ull_disable_unmark(adv);
	LL_ASSERT(mark == adv);

#if defined(CONFIG_BT_CTLR_ADV_EXT) && (CONFIG_BT_CTLR_ADV_AUX_SET > 0)
	struct lll_adv_aux *lll_aux = adv->lll.aux;

	if (lll_aux) {
		struct ll_adv_aux_set *aux;
		uint8_t err;

		aux = HDR_LLL2ULL(lll_aux);

		err = ull_adv_aux_stop(aux);
		if (err) {
			return err;
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
	if (IS_ENABLED(CONFIG_BT_OBSERVER) && !ull_scan_is_enabled_get(0)) {
		ull_filter_adv_scan_state_cb(0);
	}
#endif /* CONFIG_BT_CTLR_PRIVACY */

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
	const uint8_t *rpa = ull_filter_adva_get(adv);
#else
	const uint8_t *rpa = NULL;
#endif
	const uint8_t *own_addr;
	const uint8_t *tx_addr;
	uint8_t *adv_addr;

	if (!rpa || IS_ENABLED(CONFIG_BT_CTLR_CHECK_SAME_PEER_CONN)) {
		if (0) {
#if defined(CONFIG_BT_CTLR_ADV_EXT)
		} else if (ll_adv_cmds_is_ext() && pdu->tx_addr) {
			own_addr = ll_adv_aux_random_addr_get(adv, NULL);
#endif
		} else {
			own_addr = ll_addr_get(pdu->tx_addr, NULL);
		}
	}

#if defined(CONFIG_BT_CTLR_CHECK_SAME_PEER_CONN)
	memcpy(adv->own_addr, own_addr, BDADDR_SIZE);
#endif /* CONFIG_BT_CTLR_CHECK_SAME_PEER_CONN */

	if (rpa) {
		pdu->tx_addr = 1;
		tx_addr = rpa;
	} else {
		tx_addr = own_addr;
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

	rx_addr = ull_filter_tgta_get(adv);
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
	adv->lll.filter_policy = BT_LE_ADV_FP_NO_WHITELIST;

	init_pdu(lll_adv_data_peek(&ll_adv[0].lll), PDU_ADV_TYPE_ADV_IND);
	init_pdu(lll_adv_scan_rsp_peek(&ll_adv[0].lll), PDU_ADV_TYPE_SCAN_RSP);
}
