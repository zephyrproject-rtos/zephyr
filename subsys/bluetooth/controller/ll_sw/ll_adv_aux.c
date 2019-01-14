/*
 * Copyright (c) 2017 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>

#include <zephyr.h>

#include "util/util.h"
#include "util/memq.h"
#include "pdu.h"
#include "lll.h"
#include "ctrl.h"

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
	struct radio_adv_data *ad;
	struct ext_adv_hdr *h;
	struct pdu_adv *pdu;

	/* TODO: */

	ad = radio_adv_data_get();
	pdu = (void *)ad->data[ad->last];
	p = (void *)&pdu->adv_ext_ind;
	h = (void *)p->ext_hdr_adi_adv_data;

	if (!h->aux_ptr) {
	}

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
