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
	struct pdu_adv_com_ext_adv *p;
	struct ll_adv_set *adv;
	struct ext_adv_hdr *h;
	struct pdu_adv *prev;
	struct pdu_adv *pdu;
	u8_t idx;

	/* TODO: */

	adv = ull_adv_set_get(handle);
	if (!adv) {
		return BT_HCI_ERR_CMD_DISALLOWED;
	}

	/* Dont update data if not extended advertising. */
	prev = lll_adv_data_peek(&adv->lll);
	if (prev->type != PDU_ADV_TYPE_EXT_IND) {
		return 0;
	}

	pdu = lll_adv_data_alloc(&adv->lll, &idx);
	p = (void *)&pdu->adv_ext_ind;
	h = (void *)p->ext_hdr_adi_adv_data;

	if (!h->aux_ptr) {
		if (!len) {
			return 0;
		}
	}

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
