/*
 * Copyright (c) 2017-2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>

#include <zephyr/types.h>
#include <bluetooth/hci.h>

#include "hal/ccm.h"

#include "util/util.h"
#include "util/memq.h"

#include "pdu.h"
#include "lll.h"
#include "lll_adv.h"
#include "lll_conn.h"
#include "ull_internal.h"
#include "ull_adv_types.h"
#include "ull_adv_internal.h"

uint8_t ll_adv_aux_random_addr_set(uint8_t handle, uint8_t *addr)
{
	/* TODO: store in adv set instance */
	return 0;
}

uint8_t *ll_adv_aux_random_addr_get(uint8_t handle, uint8_t *addr)
{
	/* TODO: copy adv set instance addr into addr and/or return reference */
	return NULL;
}

uint8_t ll_adv_aux_ad_data_set(uint8_t handle, uint8_t op, uint8_t frag_pref, uint8_t len,
			    uint8_t *data)
{
	struct pdu_adv_com_ext_adv *p, *_p, *s, *_s;
	uint8_t pri_len, _pri_len, sec_len, _sec_len;
	struct pdu_adv *_pri, *pri, *_sec, *sec;
	struct ext_adv_hdr *hp, _hp, *hs, _hs;
	uint8_t *_pp, *pp, *ps, *_ps;
	struct ll_adv_set *adv;
	uint8_t ip, is;

	/* op param definitions:
	 * 0x00 - Intermediate fragment of fragmented extended advertising data
	 * 0x01 - First fragment of fragmented extended advertising data
	 * 0x02 - Last fragemnt of fragemented extended advertising data
	 * 0x03 - Complete extended advertising data
	 * 0x04 - Unchanged data (just update the advertising data)
	 * All other values, Reserved for future use
	 */

	/* TODO: handle other op values */
	if ((op != 0x03) && (op != 0x04)) {
		/* FIXME: error code */
		return BT_HCI_ERR_CMD_DISALLOWED;
	}

	/* Get the advertising set instance */
	adv = ull_adv_set_get(handle);
	if (!adv) {
		return BT_HCI_ERR_CMD_DISALLOWED;
	}

	/* Do not update data if not extended advertising. */
	_pri = lll_adv_data_peek(&adv->lll);
	if (_pri->type != PDU_ADV_TYPE_EXT_IND) {
		/* Advertising Handle has not been created using
		 * Set Extended Advertising Parameter command
		 */
		return BT_HCI_ERR_CMD_DISALLOWED;
	}

	/* Get reference to previous primary PDU data */
	_p = (void *)&_pri->adv_ext_ind;
	hp = (void *)_p->ext_hdr_adi_adv_data;
	*(uint8_t *)&_hp = *(uint8_t *)hp;
	_pp = (uint8_t *)hp + sizeof(*hp);

	/* Get reference to new primary PDU data buffer */
	pri = lll_adv_data_alloc(&adv->lll, &ip);
	pri->type = _pri->type;
	pri->rfu = 0U;
	pri->chan_sel = 0U;
	p = (void *)&pri->adv_ext_ind;
	p->adv_mode = _p->adv_mode;
	hp = (void *)p->ext_hdr_adi_adv_data;
	pp = (uint8_t *)hp + sizeof(*hp);
	*(uint8_t *)hp = 0U;

	/* Get reference to previous secondary PDU data */
	_sec = lll_adv_aux_data_peek(&adv->lll);
	_s = (void *)&_sec->adv_ext_ind;
	hs = (void *)_s->ext_hdr_adi_adv_data;
	*(uint8_t *)&_hs = *(uint8_t *)hs;
	_ps = (uint8_t *)hs + sizeof(*hs);

	/* Get reference to new secondary PDU data buffer */
	sec = lll_adv_aux_data_alloc(&adv->lll, &is);
	sec->type = pri->type;
	sec->rfu = 0U;

	if (IS_ENABLED(CONFIG_BT_CTLR_CHAN_SEL_2)) {
		sec->chan_sel = _sec->chan_sel;
	} else {
		sec->chan_sel = 0U;
	}

	sec->tx_addr = _sec->tx_addr;
	sec->rx_addr = _sec->rx_addr;

	s = (void *)&sec->adv_ext_ind;
	s->adv_mode = p->adv_mode;
	hs = (void *)s->ext_hdr_adi_adv_data;
	ps = (uint8_t *)hs + sizeof(*hs);
	*(uint8_t *)hs = 0U;

	/* AdvA flag */
	/* NOTE: as we will use auxiliary packet, we remove AdvA in primary
	 * channel. i.e. Do nothing to add AdvA in the primary PDU.
	 */
	if (_hp.adv_addr) {
		_pp += BDADDR_SIZE;

		/* Prepare to add AdvA in secondary PDU */
		hs->adv_addr = 1;

		/* NOTE: AdvA is filled at enable */
		sec->tx_addr = pri->tx_addr;
	}
	pri->tx_addr = 0U;
	pri->rx_addr = 0U;

	if (_hs.adv_addr) {
		_ps += BDADDR_SIZE;
		hs->adv_addr = 1;
	}
	if (hs->adv_addr) {
		ps += BDADDR_SIZE;
	}

	/* No TargetA in primary and secondary channel for undirected */
	/* No CTEInfo flag in primary and secondary channel PDU */

	/* ADI flag */
	if (_hp.adi) {
		_pp += sizeof(struct ext_adv_adi);
	}
	hp->adi = 1;
	pp += sizeof(struct ext_adv_adi);
	if (_hs.adi) {
		_ps += sizeof(struct ext_adv_adi);
	}
	hs->adi = 1;
	ps += sizeof(struct ext_adv_adi);

	/* AuxPtr flag */
	if (_hp.aux_ptr) {
		_pp += sizeof(struct ext_adv_aux_ptr);
	}
	hp->aux_ptr = 1;
	pp += sizeof(struct ext_adv_aux_ptr);
	if (_hs.aux_ptr) {
		_ps += sizeof(struct ext_adv_aux_ptr);

		hs->aux_ptr = 1;
		ps += sizeof(struct ext_adv_aux_ptr);
	}

	/* No SyncInfo flag in primary channel PDU */
	/* SyncInfo flag in secondary channel PDU */
	if (_hs.sync_info) {
		_ps += sizeof(struct ext_adv_sync_info);

		hs->sync_info = 1;
		ps += sizeof(struct ext_adv_sync_info);
	}

	/* Tx Power flag */
	if (_hp.tx_pwr) {
		_pp++;

		/* C1, Tx Power is optional on the LE 1M PHY, and reserved for
		 * for future use on the LE Coded PHY.
		 */
		if (adv->lll.phy_p != BIT(2)) {
			hp->tx_pwr = 1;
			pp++;
		} else {
			hs->tx_pwr = 1;
		}
	}
	if (_hs.tx_pwr) {
		_ps++;

		hs->tx_pwr = 1;
	}
	if (hs->tx_pwr) {
		ps++;
	}

	/* No ACAD in Primary channel PDU */
	/* TODO: ACAD in Secondary channel PDU */

	/* Calc primary PDU len */
	_pri_len = _pp - (uint8_t *)_p;
	pri_len = pp - (uint8_t *)p;
	p->ext_hdr_len = pri_len - offsetof(struct pdu_adv_com_ext_adv,
					    ext_hdr_adi_adv_data);

	/* set the primary PDU len */
	pri->len = pri_len;

	/* Calc secondary PDU len */
	_sec_len = _ps - (uint8_t *)_s;
	sec_len = ps - (uint8_t *)s;
	s->ext_hdr_len = sec_len - offsetof(struct pdu_adv_com_ext_adv,
					    ext_hdr_adi_adv_data);

	/* TODO: Check AdvData overflow */

	/* Fill AdvData in secondary PDU */
	memcpy(ps, data, len);

	/* set the secondary PDU len */
	sec->len = sec_len + len;

	/* Start filling primary PDU extended header  based on flags */

	/* No AdvData in primary channel PDU */

	/* No ACAD in primary channel PDU */

	/* Tx Power */
	if (hp->tx_pwr) {
		*--pp = *--_pp;
	} else if (hs->tx_pwr) {
		*--ps = *--_ps;
	}

	/* No SyncInfo in primary channel PDU */
	/* SyncInfo in secondary channel PDU */
	if (hs->sync_info) {
		_ps -= sizeof(struct ext_adv_sync_info);
		ps -= sizeof(struct ext_adv_sync_info);

		memcpy(ps, _ps, sizeof(struct ext_adv_sync_info));
	}

	/* AuxPtr */
	if (_hp.aux_ptr) {
		_pp -= sizeof(struct ext_adv_aux_ptr);
	}
	{
		struct ext_adv_aux_ptr *aux;

		pp -= sizeof(struct ext_adv_aux_ptr);

		/* NOTE: Aux Offset will be set in advertiser LLL event */
		aux = (void *)pp;
		aux->chan_idx = 0; /* FIXME: implementation defined */
		aux->ca = 0; /* FIXME: implementation defined */
		aux->offs_units = 0; /* FIXME: implementation defined */
		aux->phy = find_lsb_set(adv->lll.phy_s) - 1;
	}
	if (_hs.aux_ptr) {
		struct ext_adv_aux_ptr *aux;

		_ps -= sizeof(struct ext_adv_aux_ptr);
		ps -= sizeof(struct ext_adv_aux_ptr);

		/* NOTE: Aux Offset will be set in advertiser LLL event */
		aux = (void *)ps;
		aux->chan_idx = 0; /* FIXME: implementation defined */
		aux->ca = 0; /* FIXME: implementation defined */
		aux->offs_units = 0; /* FIXME: implementation defined */
		aux->phy = find_lsb_set(adv->lll.phy_s) - 1;
	}

	/* ADI */
	{
		struct ext_adv_adi *ap, *as;
		uint16_t did = UINT16_MAX;

		pp -= sizeof(struct ext_adv_adi);
		ps -= sizeof(struct ext_adv_adi);

		ap = (void *)pp;
		as = (void *)ps;

		if (_hp.adi) {
			struct ext_adv_adi *_adi;

			_pp -= sizeof(struct ext_adv_adi);
			_ps -= sizeof(struct ext_adv_adi);

			/* NOTE: memcpy shall handle overlapping buffers */
			memcpy(pp, _pp, sizeof(struct ext_adv_adi));
			memcpy(ps, _ps, sizeof(struct ext_adv_adi));

			_adi = (void *)_pp;
			did = _adi->did;
		} else {
			ap->sid = adv->sid;
			as->sid = adv->sid;
		}

		if ((op == 0x04) || len || (_pri_len != pri_len)) {
			did++;
		}

		ap->did = did;
		as->did = did;
	}

	/* No CTEInfo field in primary channel PDU */

	/* NOTE: TargetA, filled at enable and RPA timeout */

	/* No AdvA in primary channel due to AuxPtr being added */

	/* NOTE: AdvA in aux channel is also filled at enable and RPA timeout */
	if (hs->adv_addr) {
		void *bdaddr;

		if (_hs.adv_addr) {
			_ps -= BDADDR_SIZE;
			bdaddr = _ps;
		} else {
			_pp -= BDADDR_SIZE;
			bdaddr = _pp;
		}

		ps -= BDADDR_SIZE;

		memcpy(ps, bdaddr, BDADDR_SIZE);
	}

	lll_adv_aux_data_enqueue(&adv->lll, is);
	lll_adv_data_enqueue(&adv->lll, ip);

	return 0;
}

uint8_t ll_adv_aux_sr_data_set(uint8_t handle, uint8_t op, uint8_t frag_pref, uint8_t len,
			    uint8_t *data)
{
	/* TODO: */
	return 0;
}

uint16_t ll_adv_aux_max_data_length_get(void)
{
	/* TODO: return a Kconfig value */
	return 0;
}

uint8_t ll_adv_aux_set_count_get(void)
{
	/*  TODO: return a Kconfig value */
	return 0;
}

uint8_t ll_adv_aux_set_remove(uint8_t handle)
{
	/* TODO: reset/release primary channel and Aux channel PDUs */
	return 0;
}

uint8_t ll_adv_aux_set_clear(void)
{
	/* TODO: reset/release all adv set primary channel and  Aux channel
	 * PDUs
	 */
	return 0;
}
