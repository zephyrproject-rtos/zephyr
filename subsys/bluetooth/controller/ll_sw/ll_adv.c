/*
 * Copyright (c) 2016-2017 Nordic Semiconductor ASA
 * Copyright (c) 2016 Vinayak Kariappa Chettimada
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>

#include <zephyr.h>
#include <bluetooth/hci.h>

#include "util/util.h"

#include "pdu.h"
#include "ctrl.h"
#include "ll.h"

static struct {
	u8_t  chl_map:3;
	u8_t  filter_policy:2;

#if defined(CONFIG_BLUETOOTH_CONTROLLER_ADV_EXT)
	u8_t  phy_p:3;
	u32_t interval;
#else /* !CONFIG_BLUETOOTH_CONTROLLER_ADV_EXT */
	u16_t interval;
#endif /* !CONFIG_BLUETOOTH_CONTROLLER_ADV_EXT */
} ll_adv;

#if defined(CONFIG_BLUETOOTH_CONTROLLER_ADV_EXT)
u32_t ll_adv_params_set(u8_t handle, u16_t evt_prop, u32_t interval,
			u8_t adv_type, u8_t own_addr_type,
			u8_t direct_addr_type, u8_t const *const direct_addr,
			u8_t chl_map, u8_t filter_policy, u8_t *tx_pwr,
			u8_t phy_p, u8_t skip, u8_t phy_s, u8_t sid, u8_t sreq)
{
	u8_t const pdu_adv_type[] = {PDU_ADV_TYPE_ADV_IND,
				     PDU_ADV_TYPE_DIRECT_IND,
				     PDU_ADV_TYPE_SCAN_IND,
				     PDU_ADV_TYPE_NONCONN_IND,
				     PDU_ADV_TYPE_DIRECT_IND,
				     PDU_ADV_TYPE_EXT_IND};
#else /* !CONFIG_BLUETOOTH_CONTROLLER_ADV_EXT */
u32_t ll_adv_params_set(u16_t interval, u8_t adv_type,
			u8_t own_addr_type, u8_t direct_addr_type,
			u8_t const *const direct_addr, u8_t chl_map,
			u8_t filter_policy)
{
	u8_t const pdu_adv_type[] = {PDU_ADV_TYPE_ADV_IND,
				     PDU_ADV_TYPE_DIRECT_IND,
				     PDU_ADV_TYPE_SCAN_IND,
				     PDU_ADV_TYPE_NONCONN_IND,
				     PDU_ADV_TYPE_DIRECT_IND};
#endif /* !CONFIG_BLUETOOTH_CONTROLLER_ADV_EXT */

	struct radio_adv_data *radio_adv_data;
	struct pdu_adv *pdu;

	if (radio_adv_is_enabled()) {
		return 0x0C; /* Command Disallowed */
	}

#if defined(CONFIG_BLUETOOTH_CONTROLLER_ADV_EXT)
	/* TODO: check and fail (0x12, invalid HCI cmd param) if invalid
	 * evt_prop bits.
	 */

	ll_adv.phy_p = BIT(0);

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

			ll_adv.phy_p = phy_p;
		}
	}
#endif /* CONFIG_BLUETOOTH_CONTROLLER_ADV_EXT */

	/* remember params so that set adv/scan data and adv enable
	 * interface can correctly update adv/scan data in the
	 * double buffer between caller and controller context.
	 */
	/* Set interval for Undirected or Low Duty Cycle Directed Advertising */
	if (adv_type != 0x01) {
		ll_adv.interval = interval;
	} else {
		ll_adv.interval = 0;
	}
	ll_adv.chl_map = chl_map;
	ll_adv.filter_policy = filter_policy;

	/* update the "current" primary adv data */
	radio_adv_data = radio_adv_data_get();
	pdu = (struct pdu_adv *)&radio_adv_data->data[radio_adv_data->last][0];
	pdu->type = pdu_adv_type[adv_type];
	pdu->rfu = 0;

	if (IS_ENABLED(CONFIG_BLUETOOTH_CONTROLLER_CHAN_SEL_2) &&
	    ((pdu->type == PDU_ADV_TYPE_ADV_IND) ||
	     (pdu->type == PDU_ADV_TYPE_DIRECT_IND) ||
	     (IS_ENABLED(CONFIG_BLUETOOTH_CONTROLLER_ADV_EXT) &&
	      (pdu->type == PDU_ADV_TYPE_EXT_IND)))) {
		pdu->chan_sel = 1;
	} else {
		pdu->chan_sel = 0;
	}

	pdu->tx_addr = own_addr_type;
	pdu->rx_addr = 0;
	if (pdu->type == PDU_ADV_TYPE_DIRECT_IND) {
		pdu->rx_addr = direct_addr_type;
		memcpy(&pdu->payload.direct_ind.tgt_addr[0], direct_addr,
		       BDADDR_SIZE);
		pdu->len = sizeof(struct pdu_adv_payload_direct_ind);

#if defined(CONFIG_BLUETOOTH_CONTROLLER_ADV_EXT)
	} else if (pdu->type == PDU_ADV_TYPE_EXT_IND) {
		struct pdu_adv_payload_com_ext_adv *p;
		struct ext_adv_hdr *h;
		u8_t *ptr;
		u8_t len;

		p = (void *)&pdu->payload.adv_ext_ind;
		h = (void *)p->ext_hdr_adi_adv_data;
		ptr = (u8_t *)h + sizeof(*h);

		/* No ACAD and no AdvData */
		p->ext_hdr_len = 0;
		p->adv_mode = evt_prop & 0x03;

		/* Zero-init header flags */
		*(u8_t *)h = 0;

		/* AdvA flag */
		if (!(evt_prop & BIT(5)) && !p->adv_mode && (phy_p != BIT(2))) {
			/* TODO: optional on 1M */
			h->adv_addr = 1;

			/* NOTE: AdvA is filled at enable */
			ptr += BDADDR_SIZE;
		}

		/* TODO: TargetA flag */

		/* TODO: ADI flag */

		/* TODO: AuxPtr flag */

		/* TODO: SyncInfo flag */

		/* Tx Power flag */
		if (evt_prop & BIT(6)) {
			h->tx_pwr = 1;
			ptr++;
		}

		/* Calc primary PDU len */
		len = ptr - (u8_t *)p;
		if (len > (offsetof(struct pdu_adv_payload_com_ext_adv,
				    ext_hdr_adi_adv_data) + sizeof(*h))) {
			p->ext_hdr_len = len -
				offsetof(struct pdu_adv_payload_com_ext_adv,
					 ext_hdr_adi_adv_data);
			pdu->len = len;
		} else {
			pdu->len = offsetof(struct pdu_adv_payload_com_ext_adv,
					    ext_hdr_adi_adv_data);
		}

		/* Start filling primary PDU payload based on flags */

		/* TODO: AdvData */

		/* TODO: ACAD */

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

		/* TODO: SyncInfo */

		/* TODO: AuxPtr */

		/* TODO: ADI */

		/* NOTE: TargetA, filled at enable and RPA timeout */

		/* NOTE: AdvA, filled at enable and RPA timeout */
#endif /* CONFIG_BLUETOOTH_CONTROLLER_ADV_EXT */

	} else if (pdu->len == 0) {
		pdu->len = BDADDR_SIZE;
	}

	/* update the current scan data */
	radio_adv_data = radio_scan_data_get();
	pdu = (struct pdu_adv *)&radio_adv_data->data[radio_adv_data->last][0];
	pdu->type = PDU_ADV_TYPE_SCAN_RSP;
	pdu->rfu = 0;
	pdu->chan_sel = 0;
	pdu->tx_addr = own_addr_type;
	pdu->rx_addr = 0;
	if (pdu->len == 0) {
		pdu->len = BDADDR_SIZE;
	}

	return 0;
}

void ll_adv_data_set(u8_t len, u8_t const *const data)
{
	struct radio_adv_data *radio_adv_data;
	struct pdu_adv *prev;
	struct pdu_adv *pdu;
	u8_t last;

	/* TODO: dont update data if directed adv type. */

	/* use the last index in double buffer, */
	radio_adv_data = radio_adv_data_get();
	if (radio_adv_data->first == radio_adv_data->last) {
		last = radio_adv_data->last + 1;
		if (last == DOUBLE_BUFFER_SIZE) {
			last = 0;
		}
	} else {
		last = radio_adv_data->last;
	}

	/* update adv pdu fields. */
	prev = (struct pdu_adv *)&radio_adv_data->data[radio_adv_data->last][0];
	pdu = (struct pdu_adv *)&radio_adv_data->data[last][0];
	pdu->type = prev->type;
	pdu->rfu = 0;

	if (IS_ENABLED(CONFIG_BLUETOOTH_CONTROLLER_CHAN_SEL_2) &&
	    ((pdu->type == PDU_ADV_TYPE_ADV_IND) ||
	     (pdu->type == PDU_ADV_TYPE_DIRECT_IND))) {
		pdu->chan_sel = 1;
	} else {
		pdu->chan_sel = 0;
	}

	pdu->tx_addr = prev->tx_addr;
	pdu->rx_addr = prev->rx_addr;
	memcpy(&pdu->payload.adv_ind.addr[0],
	       &prev->payload.adv_ind.addr[0], BDADDR_SIZE);
	if (pdu->type == PDU_ADV_TYPE_DIRECT_IND) {
		memcpy(&pdu->payload.direct_ind.tgt_addr[0],
		       &prev->payload.direct_ind.tgt_addr[0], BDADDR_SIZE);
		pdu->len = sizeof(struct pdu_adv_payload_direct_ind);
	} else {
		memcpy(&pdu->payload.adv_ind.data[0], data, len);
		pdu->len = BDADDR_SIZE + len;
	}

	/* commit the update so controller picks it. */
	radio_adv_data->last = last;
}

void ll_scan_data_set(u8_t len, u8_t const *const data)
{
	struct radio_adv_data *radio_scan_data;
	struct pdu_adv *prev;
	struct pdu_adv *pdu;
	u8_t last;

	/* use the last index in double buffer, */
	radio_scan_data = radio_scan_data_get();
	if (radio_scan_data->first == radio_scan_data->last) {
		last = radio_scan_data->last + 1;
		if (last == DOUBLE_BUFFER_SIZE) {
			last = 0;
		}
	} else {
		last = radio_scan_data->last;
	}

	/* update scan pdu fields. */
	prev = (struct pdu_adv *)
	       &radio_scan_data->data[radio_scan_data->last][0];
	pdu = (struct pdu_adv *)&radio_scan_data->data[last][0];
	pdu->type = PDU_ADV_TYPE_SCAN_RSP;
	pdu->rfu = 0;
	pdu->chan_sel = 0;
	pdu->tx_addr = prev->tx_addr;
	pdu->rx_addr = 0;
	pdu->len = BDADDR_SIZE + len;
	memcpy(&pdu->payload.scan_rsp.addr[0],
	       &prev->payload.scan_rsp.addr[0], BDADDR_SIZE);
	memcpy(&pdu->payload.scan_rsp.data[0], data, len);

	/* commit the update so controller picks it. */
	radio_scan_data->last = last;
}

u32_t ll_adv_enable(u8_t enable)
{
	struct radio_adv_data *radio_scan_data;
	struct radio_adv_data *radio_adv_data;
	struct pdu_adv *pdu_scan;
	struct pdu_adv *pdu_adv;
	u32_t status;

	if (!enable) {
		status = radio_adv_disable();

		return status;
	}

	/* TODO: move the addr remembered into controller
	 * this way when implementing Privacy 1.2, generated
	 * new resolvable addresses can be used instantly.
	 */

	/* remember addr to use and also update the addr in
	 * both adv and scan response PDUs.
	 */
	radio_adv_data = radio_adv_data_get();
	radio_scan_data = radio_scan_data_get();
	pdu_adv = (struct pdu_adv *)&radio_adv_data->data
			[radio_adv_data->last][0];
	pdu_scan = (struct pdu_adv *)&radio_scan_data->data
			[radio_scan_data->last][0];

	if (0) {

#if defined(CONFIG_BLUETOOTH_CONTROLLER_ADV_EXT)
	} else if (pdu_adv->type == PDU_ADV_TYPE_EXT_IND) {
		struct pdu_adv_payload_com_ext_adv *p;
		struct ext_adv_hdr *h;
		u8_t *ptr;

		p = (void *)&pdu_adv->payload.adv_ext_ind;
		h = (void *)p->ext_hdr_adi_adv_data;
		ptr = (u8_t *)h + sizeof(*h);

		/* AdvA, fill here at enable */
		if (h->adv_addr) {
			memcpy(ptr, ll_addr_get(pdu_adv->tx_addr, NULL),
			       BDADDR_SIZE);
		}

		/* TODO: TargetA, fill here at enable */
#endif /* CONFIG_BLUETOOTH_CONTROLLER_ADV_EXT */

	} else {
		memcpy(&pdu_adv->payload.adv_ind.addr[0],
		       ll_addr_get(pdu_adv->tx_addr, NULL), BDADDR_SIZE);
		memcpy(&pdu_scan->payload.scan_rsp.addr[0],
		       ll_addr_get(pdu_adv->tx_addr, NULL), BDADDR_SIZE);
	}

#if defined(CONFIG_BLUETOOTH_CONTROLLER_ADV_EXT)
	status = radio_adv_enable(ll_adv.phy_p, ll_adv.interval, ll_adv.chl_map,
				  ll_adv.filter_policy);
#else /* !CONFIG_BLUETOOTH_CONTROLLER_ADV_EXT */
	status = radio_adv_enable(ll_adv.interval, ll_adv.chl_map,
				  ll_adv.filter_policy);
#endif /* !CONFIG_BLUETOOTH_CONTROLLER_ADV_EXT */

	return status;
}
