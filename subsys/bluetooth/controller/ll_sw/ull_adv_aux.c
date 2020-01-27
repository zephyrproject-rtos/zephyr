/*
 * Copyright (c) 2017-2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>

#include <zephyr.h>
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

u8_t ll_adv_aux_random_addr_set(u8_t handle, u8_t *addr)
{
	/* TODO: store in adv set instance */
	return 0;
}

u8_t *ll_adv_aux_random_addr_get(u8_t handle, u8_t *addr)
{
	/* TODO: copy adv set instance addr into addr and/or return reference */
	return NULL;
}

u8_t ll_adv_aux_ad_data_set(u8_t handle, u8_t op, u8_t frag_pref, u8_t len,
			    u8_t *data)
{
	struct pdu_adv_com_ext_adv *p, *_p;
	struct ext_adv_hdr *h, _h;
	struct ll_adv_set *adv;
	u8_t pdu_len, _pdu_len;
	struct pdu_adv *prev;
	struct pdu_adv *pdu;
	u8_t *_ptr, *ptr;
	u8_t idx;

	/* TODO: */

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
	prev = lll_adv_data_peek(&adv->lll);
	if (prev->type != PDU_ADV_TYPE_EXT_IND) {
		/* Advertising Handle has not been created using
		 * Set Extended Advertising Parameter command
		 */
		return BT_HCI_ERR_CMD_DISALLOWED;
	}

	/* Get reference to previous PDU data */
	_p = (void *)&prev->adv_ext_ind;
	h = (void *)_p->ext_hdr_adi_adv_data;
	*(u8_t *)&_h = *(u8_t *)h;
	_ptr = (u8_t *)h + sizeof(*h);

	/* Get reference new PDU data buffer */
	pdu = lll_adv_data_alloc(&adv->lll, &idx);
	p = (void *)&pdu->adv_ext_ind;
	p->adv_mode = _p->adv_mode;
	h = (void *)p->ext_hdr_adi_adv_data;
	ptr = (u8_t *)h + sizeof(*h);
	*(u8_t *)h = 0;

	/* AdvA flag */
	if (_h.adv_addr) {
		_ptr += BDADDR_SIZE;
	}
	/* NOTE: as we will use auxiliary packet, we remove AdvA in primary
	 * channel. i.e. Do nothing to add AdvA in the new PDU.
	 */

	/* No TargetA in primary channel for undirected */
	/* No CTEInfo flag in primary channel PDU */

	/* ADI flag */
	if (_h.adi) {
		_ptr += sizeof(struct ext_adv_adi);
	}
	h->adi = 1;
	ptr += sizeof(struct ext_adv_adi);

	/* AuxPtr flag */
	if (_h.aux_ptr) {
		_ptr += sizeof(struct ext_adv_aux_ptr);
	}
	h->aux_ptr = 1;
	ptr += sizeof(struct ext_adv_aux_ptr);

	/* No SyncInfo flag in primary channel PDU */

	/* Tx Power flag */
	if (_h.tx_pwr) {
		_ptr++;

		/* C1, Tx Power is optional on the LE 1M PHY, and reserved for
		 * for future use on the LE Coded PHY.
		 */
		if (adv->lll.phy_p != BIT(2)) {
			h->tx_pwr = 1;
			ptr++;
		}
	}

	/* Calc primary PDU len */
	_pdu_len = _ptr - (u8_t *)_p;
	pdu_len = ptr - (u8_t *)p;
	p->ext_hdr_len = pdu_len - offsetof(struct pdu_adv_com_ext_adv,
					    ext_hdr_adi_adv_data);
	pdu->len = pdu_len;

	/* Start filling primary PDU payload based on flags */

	/* No AdvData in primary channel PDU */

	/* No ACAD in primary channel PDU */

	/* Tx Power */
	if (h->tx_pwr) {
		*--ptr = *--_ptr;
	}

	/* No SyncInfo in primary channel PDU */

	/* AuxPtr */
	if (_h.aux_ptr) {
		_ptr -= sizeof(struct ext_adv_aux_ptr);
	}
	{
		struct ext_adv_aux_ptr *aux;

		ptr -= sizeof(struct ext_adv_aux_ptr);

		/* NOTE: Channel Index, CA, Offset Units and AUX Offset will be
		 * set in Advertiser Event.
		 */
		aux = (void *)ptr;
		aux->phy = find_lsb_set(adv->phy_s);
	}

	/* ADI */
	{
		struct ext_adv_adi *adi;
		u16_t did = UINT16_MAX;

		ptr -= sizeof(struct ext_adv_adi);

		adi = (void *)ptr;

		if (_h.adi) {
			struct ext_adv_adi *_adi;

			_ptr -= sizeof(struct ext_adv_adi);

			/* NOTE: memcpy shall handle overlapping buffers */
			memcpy(ptr, _ptr, sizeof(struct ext_adv_adi));

			_adi = (void *)_ptr;
			did = _adi->did;
		} else {
			adi->sid = adv->sid;
		}

		if ((op == 0x04) || len || (_pdu_len != pdu_len)) {
			did++;
		}

		adi->did = did;
	}

	/* No CTEInfo field in primary channel PDU */

	/* NOTE: TargetA, filled at enable and RPA timeout */

	/* No AdvA in primary channel due to AuxPtr being added */

	lll_adv_data_enqueue(&adv->lll, idx);

	return 0;
}

u8_t ll_adv_aux_sr_data_set(u8_t handle, u8_t op, u8_t frag_pref, u8_t len,
			    u8_t *data)
{
	/* TODO: */
	return 0;
}

u16_t ll_adv_aux_max_data_length_get(void)
{
	/* TODO: return a Kconfig value */
	return 0;
}

u8_t ll_adv_aux_set_count_get(void)
{
	/*  TODO: return a Kconfig value */
	return 0;
}

u8_t ll_adv_aux_set_remove(u8_t handle)
{
	/* TODO: reset/release primary channel and Aux channel PDUs */
	return 0;
}

u8_t ll_adv_aux_set_clear(void)
{
	/* TODO: reset/release all adv set primary channel and  Aux channel
	 * PDUs
	 */
	return 0;
}
