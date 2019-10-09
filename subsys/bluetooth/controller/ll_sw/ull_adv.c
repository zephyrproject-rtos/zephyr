/*
 * Copyright (c) 2016-2019 Nordic Semiconductor ASA
 * Copyright (c) 2016 Vinayak Kariappa Chettimada
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>

#include <zephyr.h>
#include <bluetooth/hci.h>

#include "hal/ccm.h"
#include "hal/ticker.h"

#include "util/util.h"
#include "util/mem.h"
#include "util/memq.h"
#include "util/mayfly.h"

#include "ticker/ticker.h"

#include "pdu.h"
#include "ll.h"
#include "ll_feat.h"
#include "lll.h"
#include "lll_vendor.h"
#include "lll_clock.h"
#include "lll_adv.h"
#include "lll_scan.h"
#include "lll_conn.h"
#include "lll_internal.h"
#include "lll_filter.h"

#include "ull_adv_types.h"
#include "ull_scan_types.h"
#include "ull_conn_types.h"
#include "ull_filter.h"

#include "ull_adv_internal.h"
#include "ull_scan_internal.h"
#include "ull_conn_internal.h"
#include "ull_internal.h"

#define LOG_MODULE_NAME bt_ctlr_llsw_ull_adv
#include "common/log.h"
#include <soc.h>
#include "hal/debug.h"

inline struct ll_adv_set *ull_adv_set_get(u16_t handle);
inline u16_t ull_adv_handle_get(struct ll_adv_set *adv);

static int init_reset(void);
static inline struct ll_adv_set *is_disabled_get(u16_t handle);
static void ticker_cb(u32_t ticks_at_expire, u32_t remainder, u16_t lazy,
		      void *param);
static void ticker_op_update_cb(u32_t status, void *params);

#if defined(CONFIG_BT_PERIPHERAL)
static void ticker_stop_cb(u32_t ticks_at_expire, u32_t remainder, u16_t lazy,
			   void *param);
static void ticker_op_stop_cb(u32_t status, void *params);
static void disabled_cb(void *param);
static inline void conn_release(struct ll_adv_set *adv);
#endif /* CONFIG_BT_PERIPHERAL */

static inline u8_t disable(u16_t handle);

static struct ll_adv_set ll_adv[BT_CTLR_ADV_MAX];

#if defined(CONFIG_BT_CTLR_ADV_EXT)
u8_t ll_adv_params_set(u8_t handle, u16_t evt_prop, u32_t interval,
		       u8_t adv_type, u8_t own_addr_type,
		       u8_t direct_addr_type, u8_t const *const direct_addr,
		       u8_t chan_map, u8_t filter_policy, u8_t *tx_pwr,
		       u8_t phy_p, u8_t skip, u8_t phy_s, u8_t sid, u8_t sreq)
{
	u8_t const pdu_adv_type[] = {PDU_ADV_TYPE_ADV_IND,
				     PDU_ADV_TYPE_DIRECT_IND,
				     PDU_ADV_TYPE_SCAN_IND,
				     PDU_ADV_TYPE_NONCONN_IND,
				     PDU_ADV_TYPE_DIRECT_IND,
				     PDU_ADV_TYPE_EXT_IND};
#else /* !CONFIG_BT_CTLR_ADV_EXT */
u8_t ll_adv_params_set(u16_t interval, u8_t adv_type,
		       u8_t own_addr_type, u8_t direct_addr_type,
		       u8_t const *const direct_addr, u8_t chan_map,
		       u8_t filter_policy)
{
	u8_t const pdu_adv_type[] = {PDU_ADV_TYPE_ADV_IND,
				     PDU_ADV_TYPE_DIRECT_IND,
				     PDU_ADV_TYPE_SCAN_IND,
				     PDU_ADV_TYPE_NONCONN_IND,
				     PDU_ADV_TYPE_DIRECT_IND};
	u16_t const handle = 0;
#endif /* !CONFIG_BT_CTLR_ADV_EXT */

	struct ll_adv_set *adv;
	struct pdu_adv *pdu;

	adv = is_disabled_get(handle);
	if (!adv) {
		return BT_HCI_ERR_CMD_DISALLOWED;
	}

#if defined(CONFIG_BT_CTLR_ADV_EXT)
	/* TODO: check and fail (0x12, invalid HCI cmd param) if invalid
	 * evt_prop bits.
	 */

	adv->lll.phy_p = BIT(0);

	/* extended */
	if (adv_type > 0x04) {
		/* legacy */
		if (evt_prop & BIT(4)) {
			u8_t const leg_adv_type[] = { 0x03, 0x04, 0x02, 0x00};

			adv_type = leg_adv_type[evt_prop & 0x03];

			/* high duty cycle directed */
			if (evt_prop & BIT(3)) {
				adv_type = 0x01;
			}
		} else {
			/* - Connectable and scannable not allowed;
			 * - High duty cycle directed connectable not allowed
			 */
			if (((evt_prop & 0x03) == 0x03) ||
			    ((evt_prop & 0x0C) == 0x0C)) {
				return 0x12; /* invalid HCI cmd param */
			}

			adv_type = 0x05; /* PDU_ADV_TYPE_EXT_IND */

			adv->lll.phy_p = phy_p;
		}
	}
#endif /* CONFIG_BT_CTLR_ADV_EXT */

	/* remember params so that set adv/scan data and adv enable
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

	/* update the "current" primary adv data */
	pdu = lll_adv_data_peek(&adv->lll);
	pdu->type = pdu_adv_type[adv_type];
	pdu->rfu = 0;

	if (IS_ENABLED(CONFIG_BT_CTLR_CHAN_SEL_2) &&
	    ((pdu->type == PDU_ADV_TYPE_ADV_IND) ||
	     (pdu->type == PDU_ADV_TYPE_DIRECT_IND))) {
		pdu->chan_sel = 1;
	} else {
		pdu->chan_sel = 0;
	}

#if defined(CONFIG_BT_CTLR_PRIVACY)
	adv->own_addr_type = own_addr_type;
	if (adv->own_addr_type == BT_ADDR_LE_PUBLIC_ID ||
	    adv->own_addr_type == BT_ADDR_LE_RANDOM_ID) {
		adv->id_addr_type = direct_addr_type;
		memcpy(&adv->id_addr, direct_addr, BDADDR_SIZE);
	}
#endif /* CONFIG_BT_CTLR_PRIVACY */
	pdu->tx_addr = own_addr_type & 0x1;
	pdu->rx_addr = 0;
	if (pdu->type == PDU_ADV_TYPE_DIRECT_IND) {
		pdu->rx_addr = direct_addr_type;
		memcpy(&pdu->direct_ind.tgt_addr[0], direct_addr, BDADDR_SIZE);
		pdu->len = sizeof(struct pdu_adv_direct_ind);

#if defined(CONFIG_BT_CTLR_ADV_EXT)
	} else if (pdu->type == PDU_ADV_TYPE_EXT_IND) {
		struct pdu_adv_com_ext_adv *p;
		struct ext_adv_hdr _h, *h;
		u8_t *_ptr, *ptr;
		u8_t len;

		p = (void *)&pdu->adv_ext_ind;
		h = (void *)p->ext_hdr_adi_adv_data;
		ptr = (u8_t *)h + sizeof(*h);
		_ptr = ptr;

		/* No ACAD and no AdvData */
		p->ext_hdr_len = 0;
		p->adv_mode = evt_prop & 0x03;

		/* Zero-init header flags */
		*(u8_t *)&_h = *(u8_t *)h;
		*(u8_t *)h = 0;

		/* AdvA flag */
		if (_h.adv_addr) {
			_ptr += BDADDR_SIZE;
		}
		if (!p->adv_mode &&
		    (!_h.aux_ptr ||
		     (!(evt_prop & BIT(5)) && (phy_p != BIT(2))))) {
			/* TODO: optional on 1M with Aux Ptr */
			h->adv_addr = 1;

			/* NOTE: AdvA is filled at enable */
			ptr += BDADDR_SIZE;
		}

		/* TODO: TargetA flag */

		/* ADI flag */
		if (_h.adi) {
			h->adi = 1;
			ptr += sizeof(struct ext_adv_adi);
		}

		/* AuxPtr flag */
		if (_h.aux_ptr) {
			h->aux_ptr = 1;
			ptr += sizeof(struct ext_adv_aux_ptr);
		}

		/* No SyncInfo flag in primary channel PDU */

		/* Tx Power flag */
		if (evt_prop & BIT(6) &&
		    (!_h.aux_ptr || (phy_p != BIT(2)))) {
			h->tx_pwr = 1;
			ptr++;
		}

		/* Calc primary PDU len */
		len = ptr - (u8_t *)p;
		if (len > (offsetof(struct pdu_adv_com_ext_adv,
				    ext_hdr_adi_adv_data) + sizeof(*h))) {
			p->ext_hdr_len = len -
				offsetof(struct pdu_adv_com_ext_adv,
					 ext_hdr_adi_adv_data);
			pdu->len = len;
		} else {
			pdu->len = offsetof(struct pdu_adv_com_ext_adv,
					    ext_hdr_adi_adv_data);
		}

		/* Start filling primary PDU payload based on flags */

		/* No AdvData in primary channel PDU */

		/* No ACAD in primary channel PDU */

		/* Tx Power */
		if (h->tx_pwr) {
			u8_t _tx_pwr;

			_tx_pwr = 0;
			if (tx_pwr) {
				if (*tx_pwr != 0x7F) {
					_tx_pwr = *tx_pwr;
				} else {
					*tx_pwr = _tx_pwr;
				}
			}

			ptr--;
			*ptr = _tx_pwr;
		}

		/* No SyncInfo in primary channel PDU */

		/* AuxPtr */
		if (h->aux_ptr) {
			struct ext_adv_aux_ptr *aux;

			ptr -= sizeof(struct ext_adv_aux_ptr);

			/* NOTE: Channel Index, CA, Offset Units and AUX Offset
			 * will be set in Advertiser Event.
			 */
			aux = (void *)ptr;
			aux->phy = find_lsb_set(phy_s);
		}

		/* ADI */
		if (h->adi) {
			struct ext_adv_adi *adi;

			ptr -= sizeof(struct ext_adv_adi);
			/* NOTE: memcpy shall handle overlapping buffers */
			memcpy(ptr, _ptr, sizeof(struct ext_adv_adi));

			adi = (void *)ptr;
			adi->sid = sid;
		}

		/* NOTE: TargetA, filled at enable and RPA timeout */

		/* NOTE: AdvA, filled at enable and RPA timeout */
#endif /* CONFIG_BT_CTLR_ADV_EXT */

	} else if (pdu->len == 0) {
		pdu->len = BDADDR_SIZE;
	}

	/* update the current scan data */
	pdu = lll_adv_scan_rsp_peek(&adv->lll);
	pdu->type = PDU_ADV_TYPE_SCAN_RSP;
	pdu->rfu = 0;
	pdu->chan_sel = 0;
	pdu->tx_addr = own_addr_type & 0x1;
	pdu->rx_addr = 0;
	if (pdu->len == 0) {
		pdu->len = BDADDR_SIZE;
	}

	return 0;
}

#if defined(CONFIG_BT_CTLR_ADV_EXT)
u8_t ll_adv_data_set(u16_t handle, u8_t len, u8_t const *const data)
{
#else /* !CONFIG_BT_CTLR_ADV_EXT */
u8_t ll_adv_data_set(u8_t len, u8_t const *const data)
{
	const u16_t handle = 0;
#endif /* !CONFIG_BT_CTLR_ADV_EXT */
	struct ll_adv_set *adv;
	struct pdu_adv *prev;
	struct pdu_adv *pdu;
	u8_t idx;

	adv = ull_adv_set_get(handle);
	if (!adv) {
		return BT_HCI_ERR_CMD_DISALLOWED;
	}

	/* Dont update data if directed or extended advertising. */
	prev = lll_adv_data_peek(&adv->lll);
	if ((prev->type == PDU_ADV_TYPE_DIRECT_IND) ||
	    (IS_ENABLED(CONFIG_BT_CTLR_ADV_EXT) &&
	     (prev->type == PDU_ADV_TYPE_EXT_IND))) {
		/* TODO: remember data, to be used if type is changed using
		 * parameter set function ll_adv_params_set afterwards.
		 */
		return 0;
	}

	/* update adv pdu fields. */
	pdu = lll_adv_data_alloc(&adv->lll, &idx);
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

	lll_adv_data_enqueue(&adv->lll, idx);

	return 0;
}

#if defined(CONFIG_BT_CTLR_ADV_EXT)
u8_t ll_adv_scan_rsp_set(u16_t handle, u8_t len, u8_t const *const data)
{
#else /* !CONFIG_BT_CTLR_ADV_EXT */
u8_t ll_adv_scan_rsp_set(u8_t len, u8_t const *const data)
{
	const u16_t handle = 0;
#endif /* !CONFIG_BT_CTLR_ADV_EXT */
	struct ll_adv_set *adv;
	struct pdu_adv *prev;
	struct pdu_adv *pdu;
	u8_t idx;

	adv = ull_adv_set_get(handle);
	if (!adv) {
		return BT_HCI_ERR_CMD_DISALLOWED;
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

	lll_adv_scan_rsp_enqueue(&adv->lll, idx);

	return 0;
}

#if defined(CONFIG_BT_CTLR_ADV_EXT) || defined(CONFIG_BT_HCI_MESH_EXT)
#if defined(CONFIG_BT_HCI_MESH_EXT)
u8_t ll_adv_enable(u16_t handle, u8_t enable,
		   u8_t at_anchor, u32_t ticks_anchor, u8_t retry,
		   u8_t scan_window, u8_t scan_delay)
{
#else /* !CONFIG_BT_HCI_MESH_EXT */
u8_t ll_adv_enable(u16_t handle, u8_t enable)
{
	u32_t ticks_anchor;
#endif /* !CONFIG_BT_HCI_MESH_EXT */
#else /* !CONFIG_BT_CTLR_ADV_EXT || !CONFIG_BT_HCI_MESH_EXT */
u8_t ll_adv_enable(u8_t enable)
{
	u16_t const handle = 0;
	u32_t ticks_anchor;
#endif /* !CONFIG_BT_CTLR_ADV_EXT || !CONFIG_BT_HCI_MESH_EXT */
	volatile u32_t ret_cb = TICKER_STATUS_BUSY;
	u8_t   rl_idx = FILTER_IDX_NONE;
	u32_t ticks_slot_overhead;
	struct pdu_adv *pdu_scan;
	struct pdu_adv *pdu_adv;
	u32_t ticks_slot_offset;
	struct ll_adv_set *adv;
	struct lll_adv *lll;
	u32_t ret;

	if (!enable) {
		return disable(handle);
	}

	adv = is_disabled_get(handle);
	if (!adv) {
		return BT_HCI_ERR_CMD_DISALLOWED;
	}

	lll = &adv->lll;

	pdu_adv = lll_adv_data_peek(lll);
	if (pdu_adv->tx_addr) {
		if (!mem_nz(ll_addr_get(1, NULL), BDADDR_SIZE)) {
			return BT_HCI_ERR_INVALID_PARAM;
		}
	}

	pdu_scan = lll_adv_scan_rsp_peek(lll);

	if (0) {

#if defined(CONFIG_BT_CTLR_ADV_EXT)
	} else if (pdu_adv->type == PDU_ADV_TYPE_EXT_IND) {
		struct pdu_adv_com_ext_adv *p;
		struct ext_adv_hdr *h;
		u8_t *ptr;

		p = (void *)&pdu_adv->adv_ext_ind;
		h = (void *)p->ext_hdr_adi_adv_data;
		ptr = (u8_t *)h + sizeof(*h);

		/* AdvA, fill here at enable */
		if (h->adv_addr) {
			memcpy(ptr, ll_addr_get(pdu_adv->tx_addr, NULL),
			       BDADDR_SIZE);
		}

		/* TODO: TargetA, fill here at enable */
#endif /* CONFIG_BT_CTLR_ADV_EXT */
	} else {
		bool priv = false;

#if defined(CONFIG_BT_CTLR_PRIVACY)
		/* Prepare whitelist and optionally resolving list */
		ull_filter_adv_update(lll->filter_policy);

		if (adv->own_addr_type == BT_ADDR_LE_PUBLIC_ID ||
		    adv->own_addr_type == BT_ADDR_LE_RANDOM_ID) {
			/* Look up the resolving list */
			rl_idx = ull_filter_rl_find(adv->id_addr_type,
						    adv->id_addr, NULL);

			if (rl_idx != FILTER_IDX_NONE) {
				/* Generate RPAs if required */
				ull_filter_rpa_update(false);
			}

			ull_filter_adv_pdu_update(adv, rl_idx, pdu_adv);
			ull_filter_adv_pdu_update(adv, rl_idx, pdu_scan);
			priv = true;
		}
#endif /* !CONFIG_BT_CTLR_PRIVACY */

		if (!priv) {
			memcpy(&pdu_adv->adv_ind.addr[0],
			       ll_addr_get(pdu_adv->tx_addr, NULL),
			       BDADDR_SIZE);
			memcpy(&pdu_scan->scan_rsp.addr[0],
			       ll_addr_get(pdu_adv->tx_addr, NULL),
			       BDADDR_SIZE);
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
	    (pdu_adv->type == PDU_ADV_TYPE_DIRECT_IND)) {
		struct node_rx_pdu *node_rx;
		struct ll_conn *conn;
		struct lll_conn *conn_lll;
		void *link;

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
		/* Use the default 1M packet max time. Value of 0 is
		 * equivalent to using BIT(0).
		 */
		conn_lll->max_tx_time = PKT_US(PDU_DC_PAYLOAD_SIZE_MIN, 0);
		conn_lll->max_rx_time = PKT_US(PDU_DC_PAYLOAD_SIZE_MIN, 0);
#endif /* CONFIG_BT_CTLR_PHY */
#endif /* CONFIG_BT_CTLR_DATA_LENGTH */

#if defined(CONFIG_BT_CTLR_PHY)
		conn_lll->phy_tx = BIT(0);
		conn_lll->phy_flags = 0;
		conn_lll->phy_tx_time = BIT(0);
		conn_lll->phy_rx = BIT(0);
#endif /* CONFIG_BT_CTLR_PHY */

#if defined(CONFIG_BT_CTLR_CONN_RSSI)
		conn_lll->rssi_latest = 0x7F;
		conn_lll->rssi_reported = 0x7F;
		conn_lll->rssi_sample_count = 0;
#endif /* CONFIG_BT_CTLR_CONN_RSSI */

		/* FIXME: BEGIN: Move to ULL? */
		conn_lll->role = 1;
		conn_lll->data_chan_sel = 0;
		conn_lll->data_chan_use = 0;
		conn_lll->event_counter = 0;

		conn_lll->latency_prepare = 0;
		conn_lll->latency_event = 0;
		conn_lll->slave.latency_enabled = 0;
		conn_lll->slave.latency_cancel = 0;
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

		conn->common.fex_valid = 0;

		conn->llcp_req = conn->llcp_ack = conn->llcp_type = 0;
		conn->llcp_rx = NULL;
		conn->llcp_features = LL_FEAT;
		conn->llcp_version.tx = conn->llcp_version.rx = 0;
		conn->llcp_terminate.reason_peer = 0;
		/* NOTE: use allocated link for generating dedicated
		 * terminate ind rx node
		 */
		conn->llcp_terminate.node_rx.hdr.link = link;

#if defined(CONFIG_BT_CTLR_LE_ENC)
		conn_lll->enc_rx = conn_lll->enc_tx = 0U;
		conn->llcp_enc.req = conn->llcp_enc.ack = 0U;
		conn->llcp_enc.pause_tx = conn->llcp_enc.pause_rx = 0U;
		conn->llcp_enc.refresh = 0U;
#endif /* CONFIG_BT_CTLR_LE_ENC */

#if defined(CONFIG_BT_CTLR_CONN_PARAM_REQ)
		conn->llcp_conn_param.req = 0;
		conn->llcp_conn_param.ack = 0;
		conn->llcp_conn_param.disabled = 0;
#endif /* CONFIG_BT_CTLR_CONN_PARAM_REQ */

#if defined(CONFIG_BT_CTLR_DATA_LENGTH)
		conn->llcp_length.req = conn->llcp_length.ack = 0U;
		conn->llcp_length.pause_tx = 0U;
		conn->default_tx_octets = ull_conn_default_tx_octets_get();

#if defined(CONFIG_BT_CTLR_PHY)
		conn->default_tx_time = ull_conn_default_tx_time_get();
#endif /* CONFIG_BT_CTLR_PHY */
#endif /* CONFIG_BT_CTLR_DATA_LENGTH */

#if defined(CONFIG_BT_CTLR_PHY)
		conn->llcp_phy.req = conn->llcp_phy.ack = 0;
		conn->llcp_phy.pause_tx = 0U;
		conn->phy_pref_tx = ull_conn_default_phy_tx_get();
		conn->phy_pref_rx = ull_conn_default_phy_rx_get();
		conn->phy_pref_flags = 0;
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
		lll_clock_wait();
	}
#endif /* CONFIG_BT_PERIPHERAL */

#if defined(CONFIG_BT_CTLR_PRIVACY)
	adv->rl_idx = rl_idx;
#else
	ARG_UNUSED(rl_idx);
#endif /* CONFIG_BT_CTLR_PRIVACY */

#if defined(CONFIG_BT_CTLR_ADV_EXT)
	const u8_t phy = lll->phy_p;
#else
	/* Legacy ADV only supports LE_1M PHY */
	const u8_t phy = 1;
#endif

	/* For now we adv on all channels enabled in channel map */
	u8_t ch_map = lll->chan_map;
	const u8_t adv_chn_cnt = util_ones_count_get(&ch_map, sizeof(ch_map));
	u32_t slot_us	= EVENT_OVERHEAD_START_US + EVENT_OVERHEAD_END_US;

	if (adv_chn_cnt == 0) {
		/* ADV needs at least one channel */
		goto failure_cleanup;
	}

#if defined(CONFIG_BT_CTLR_ADV_EXT)
	if (pdu_adv->type == PDU_ADV_TYPE_EXT_IND) {
		/* TBD */
	} else
#endif
	{
		const u8_t adv_data_len = pdu_adv->len;
		const u8_t rsp_data_len = pdu_scan->len;
		const u8_t ll_hdr_size  = LL_HEADER_SIZE(phy);
		u32_t adv_size		= ll_hdr_size + ADVA_SIZE;
		const u8_t ll_hdr_us	= BYTES2US(ll_hdr_size, phy);
		const u8_t rx_to_us	= EVENT_RX_TO_US(phy);
		const u8_t rxtx_turn_us = EVENT_RX_TX_TURNARROUND(phy);
		const u16_t conn_ind_us = ll_hdr_us +
					  BYTES2US(INITA_SIZE + ADVA_SIZE +
						   LLDATA_SIZE, phy);
		const u8_t scan_req_us  = ll_hdr_us +
					  BYTES2US(SCANA_SIZE + ADVA_SIZE, phy);
		/* ll_header plus AdvA and scan response data */
		const u16_t scan_rsp_us  = ll_hdr_us +
					  BYTES2US(ADVA_SIZE + rsp_data_len,
						   phy);

		if (phy != 0x01) {
			/* Legacy ADV only supports LE_1M PHY */
			goto failure_cleanup;
		}

		if (pdu_adv->type == PDU_ADV_TYPE_NONCONN_IND) {
			adv_size += adv_data_len;
			slot_us += BYTES2US(adv_size, phy) * adv_chn_cnt +
				    EVENT_IFS_MAX_US * (adv_chn_cnt - 1);
		} else {
			if (pdu_adv->type == PDU_ADV_TYPE_DIRECT_IND) {
				adv_size += TARGETA_SIZE;
				slot_us += conn_ind_us;
			} else if (pdu_adv->type == PDU_ADV_TYPE_ADV_IND) {
				adv_size += adv_data_len;
				slot_us += MAX(scan_req_us + EVENT_IFS_MAX_US +
						scan_rsp_us, conn_ind_us);
			} else if (pdu_adv->type == PDU_ADV_TYPE_SCAN_IND) {
				adv_size += adv_data_len;
				slot_us += scan_req_us + EVENT_IFS_MAX_US +
					   scan_rsp_us;
			}

			slot_us += (BYTES2US(adv_size, phy) + EVENT_IFS_MAX_US
				  + rx_to_us + rxtx_turn_us) * (adv_chn_cnt-1)
				  + BYTES2US(adv_size, phy) + EVENT_IFS_MAX_US;
		}
	}

	u16_t interval = adv->interval;
#if defined(CONFIG_BT_HCI_MESH_EXT)
	if (lll->is_mesh) {
		u16_t interval_min_us;

		_radio.advertiser.retry = retry;
		_radio.advertiser.scan_delay_ms = scan_delay;
		_radio.advertiser.scan_window_ms = scan_window;

		interval_min_us = slot_us + (scan_delay + scan_window) * 1000;
		if ((interval * 625) < interval_min_us) {
			interval = (interval_min_us + (625 - 1)) / 625;
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
	adv->evt.ticks_active_to_start = 0;
	adv->evt.ticks_xtal_to_start =
		HAL_TICKER_US_TO_TICKS(EVENT_OVERHEAD_XTAL_US);
	adv->evt.ticks_preempt_to_start =
		HAL_TICKER_US_TO_TICKS(EVENT_OVERHEAD_PREEMPT_MIN_US);
	adv->evt.ticks_slot = HAL_TICKER_US_TO_TICKS(slot_us);

	ticks_slot_offset = MAX(adv->evt.ticks_active_to_start,
				adv->evt.ticks_xtal_to_start);

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
		ret = ticker_start(TICKER_INSTANCE_ID_CTLR,
				   TICKER_USER_ID_THREAD,
				   (TICKER_ID_ADV_BASE + handle),
				   ticks_anchor, 0,
				   adv->evt.ticks_slot,
				   TICKER_NULL_REMAINDER, TICKER_NULL_LAZY,
				   (adv->evt.ticks_slot + ticks_slot_overhead),
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
		ret = ticker_start(TICKER_INSTANCE_ID_CTLR,
				   TICKER_USER_ID_THREAD,
				   (TICKER_ID_ADV_BASE + handle),
				   ticks_anchor, 0,
				   HAL_TICKER_US_TO_TICKS((u64_t)interval *
							  625),
				   TICKER_NULL_REMAINDER,
#if !defined(CONFIG_BT_TICKER_COMPATIBILITY_MODE) && \
	!defined(CONFIG_BT_CTLR_LOW_LAT)
				   /* Force expiry to ensure timing update */
				   TICKER_LAZY_MUST_EXPIRE,
#else
				   TICKER_NULL_LAZY,
#endif
				   (adv->evt.ticks_slot + ticks_slot_overhead),
				   ticker_cb, adv,
				   ull_ticker_status_give, (void *)&ret_cb);
	}

	ret = ull_ticker_status_take(ret, &ret_cb);
	if (ret != TICKER_STATUS_SUCCESS) {
		goto failure_cleanup;
	}

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

	err = init_reset();
	if (err) {
		return err;
	}

	return 0;
}

int ull_adv_reset(void)
{
	u16_t handle;
	int err;

	for (handle = 0U; handle < BT_CTLR_ADV_MAX; handle++) {
		(void)disable(handle);
	}

	err = init_reset();
	if (err) {
		return err;
	}

	return 0;
}

inline struct ll_adv_set *ull_adv_set_get(u16_t handle)
{
	if (handle >= BT_CTLR_ADV_MAX) {
		return NULL;
	}

	return &ll_adv[handle];
}

inline u16_t ull_adv_handle_get(struct ll_adv_set *adv)
{
	return ((u8_t *)adv - (u8_t *)ll_adv) / sizeof(*adv);
}

u16_t ull_adv_lll_handle_get(struct lll_adv *lll)
{
	return ull_adv_handle_get((void *)lll->hdr.parent);
}

inline struct ll_adv_set *ull_adv_is_enabled_get(u16_t handle)
{
	struct ll_adv_set *adv;

	adv = ull_adv_set_get(handle);
	if (!adv || !adv->is_enabled) {
		return NULL;
	}

	return adv;
}

u32_t ull_adv_is_enabled(u16_t handle)
{
	struct ll_adv_set *adv;

	adv = ull_adv_is_enabled_get(handle);
	if (!adv) {
		return 0;
	}

	return BIT(0);
}

u32_t ull_adv_filter_pol_get(u16_t handle)
{
	struct ll_adv_set *adv;

	adv = ull_adv_is_enabled_get(handle);
	if (!adv) {
		return 0;
	}

	return adv->lll.filter_policy;
}

static int init_reset(void)
{
	return 0;
}

static inline struct ll_adv_set *is_disabled_get(u16_t handle)
{
	struct ll_adv_set *adv;

	adv = ull_adv_set_get(handle);
	if (!adv || adv->is_enabled) {
		return NULL;
	}

	return adv;
}

static void ticker_cb(u32_t ticks_at_expire, u32_t remainder, u16_t lazy,
		      void *param)
{
	static memq_link_t link;
	static struct mayfly mfy = {0, 0, &link, NULL, lll_adv_prepare};
	static struct lll_prepare_param p;
	struct ll_adv_set *adv = param;
	struct lll_adv *lll;
	u32_t ret;
	u8_t ref;

	DEBUG_RADIO_PREPARE_A(1);

	lll = &adv->lll;

	if (IS_ENABLED(CONFIG_BT_TICKER_COMPATIBILITY_MODE) ||
	    (lazy != TICKER_LAZY_MUST_EXPIRE)) {
		/* Increment prepare reference count */
		ref = ull_ref_inc(&adv->ull);
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
	}

	/* Apply adv random delay */
#if defined(CONFIG_BT_PERIPHERAL)
	if (!lll->is_hdcd)
#endif /* CONFIG_BT_PERIPHERAL */
	{
		u32_t random_delay;
		u32_t ret;

		lll_entropy_get(sizeof(random_delay), &random_delay);
		random_delay %= HAL_TICKER_US_TO_TICKS(10000);
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
	}

	DEBUG_RADIO_PREPARE_A(1);
}

static void ticker_op_update_cb(u32_t status, void *param)
{
	LL_ASSERT(status == TICKER_STATUS_SUCCESS ||
		  param == ull_disable_mark_get());
}

#if defined(CONFIG_BT_PERIPHERAL)
static void ticker_stop_cb(u32_t ticks_at_expire, u32_t remainder, u16_t lazy,
			   void *param)
{
	struct ll_adv_set *adv = param;
	u16_t handle;
	u32_t ret;

#if 0
	/* NOTE: abort the event, so as to permit ticker_job execution, if
	 *       disabled inside events.
	 */
	if (adv->ull.ref) {
		static memq_link_t _link;
		static struct mayfly _mfy = {0, 0, &_link, NULL, lll_disable};

		_mfy.param = &adv->lll;
		ret = mayfly_enqueue(TICKER_USER_ID_ULL_HIGH,
				     TICKER_USER_ID_LLL, 0, &_mfy);
		LL_ASSERT(!ret);
	}
#endif

	handle = ull_adv_handle_get(adv);
	LL_ASSERT(handle < BT_CTLR_ADV_MAX);

	ret = ticker_stop(TICKER_INSTANCE_ID_CTLR, TICKER_USER_ID_ULL_HIGH,
			  TICKER_ID_ADV_BASE + handle,
			  ticker_op_stop_cb, adv);
	LL_ASSERT((ret == TICKER_STATUS_SUCCESS) ||
		  (ret == TICKER_STATUS_BUSY));
}

static void ticker_op_stop_cb(u32_t status, void *param)
{
	static memq_link_t link;
	static struct mayfly mfy = {0, 0, &link, NULL, NULL};
	struct ll_adv_set *adv;
	struct ull_hdr *hdr;
	u32_t ret;

	/* Ignore if race between thread and ULL */
	if (status != TICKER_STATUS_SUCCESS) {
		/* TODO: detect race */

		return;
	}

#if defined(CONFIG_BT_HCI_MESH_EXT)
	/* FIXME: why is this here for Mesh commands? */
	if (params) {
		return;
	}
#endif /* CONFIG_BT_HCI_MESH_EXT */

	adv = param;
	hdr = &adv->ull;
	mfy.param = &adv->lll;
	if (hdr->ref) {
		LL_ASSERT(!hdr->disabled_cb);
		hdr->disabled_param = mfy.param;
		hdr->disabled_cb = disabled_cb;

		mfy.fp = lll_disable;
		ret = mayfly_enqueue(TICKER_USER_ID_ULL_LOW,
				     TICKER_USER_ID_LLL, 0, &mfy);
		LL_ASSERT(!ret);
	} else {
		mfy.fp = disabled_cb;
		ret = mayfly_enqueue(TICKER_USER_ID_ULL_LOW,
				     TICKER_USER_ID_ULL_HIGH, 0, &mfy);
		LL_ASSERT(!ret);
	}
}

static void disabled_cb(void *param)
{
	struct node_rx_ftr *ftr;
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
	cc->status = 0x3c;

	ftr = &(rx->hdr.rx_ftr);
	ftr->param = param;

	ll_rx_put(link, rx);
	ll_rx_sched();
}

static inline void conn_release(struct ll_adv_set *adv)
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

static inline u8_t disable(u16_t handle)
{
	volatile u32_t ret_cb = TICKER_STATUS_BUSY;
	struct ll_adv_set *adv;
	void *mark;
	u32_t ret;

	adv = ull_adv_is_enabled_get(handle);
	if (!adv) {
		return BT_HCI_ERR_CMD_DISALLOWED;
	}

	mark = ull_disable_mark(adv);
	LL_ASSERT(mark == adv);

#if defined(CONFIG_BT_PERIPHERAL)
	if (adv->lll.is_hdcd) {
		ret = ticker_stop(TICKER_INSTANCE_ID_CTLR,
				  TICKER_USER_ID_THREAD, TICKER_ID_ADV_STOP,
				  ull_ticker_status_give, (void *)&ret_cb);
		ret = ull_ticker_status_take(ret, &ret_cb);
		if (ret) {
			mark = ull_disable_mark(adv);
			LL_ASSERT(mark == adv);

			return BT_HCI_ERR_CMD_DISALLOWED;
		}
		ret_cb = TICKER_STATUS_BUSY;
	}
#endif

	ret = ticker_stop(TICKER_INSTANCE_ID_CTLR, TICKER_USER_ID_THREAD,
			  TICKER_ID_ADV_BASE + handle,
			  ull_ticker_status_give, (void *)&ret_cb);

	ret = ull_ticker_status_take(ret, &ret_cb);
	if (ret) {
		mark = ull_disable_mark(adv);
		LL_ASSERT(mark == adv);

		return BT_HCI_ERR_CMD_DISALLOWED;
	}

	ret = ull_disable(&adv->lll);
	LL_ASSERT(!ret);

	mark = ull_disable_unmark(adv);
	LL_ASSERT(mark == adv);

#if defined(CONFIG_BT_PERIPHERAL)
	if (adv->lll.conn) {
		conn_release(adv);
	}
#endif /* CONFIG_BT_PERIPHERAL */

	adv->is_enabled = 0U;

#if defined(CONFIG_BT_CTLR_PRIVACY)
	if (IS_ENABLED(CONFIG_BT_OBSERVER) && !ull_scan_is_enabled_get(0)) {
		ull_filter_adv_scan_state_cb(0);
	}
#endif /* CONFIG_BT_CTLR_PRIVACY */

	return 0;
}
