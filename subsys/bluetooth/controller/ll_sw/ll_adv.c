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
	u16_t interval;
	u8_t  filter_policy:2;
	u8_t  chl_map:3;
} ll_adv;

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
	struct radio_adv_data *radio_adv_data;
	struct pdu_adv *pdu;

	if (radio_adv_is_enabled()) {
		return 1;
	}

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

	/* update the current adv data */
	radio_adv_data = radio_adv_data_get();
	pdu = (struct pdu_adv *)&radio_adv_data->data[radio_adv_data->last][0];
	pdu->type = pdu_adv_type[adv_type];
	pdu->rfu = 0;

	if (IS_ENABLED(CONFIG_BLUETOOTH_CONTROLLER_CHAN_SEL_2) &&
	    ((pdu->type == PDU_ADV_TYPE_ADV_IND) ||
	     (pdu->type == PDU_ADV_TYPE_DIRECT_IND))) {
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

	memcpy(&pdu_adv->payload.adv_ind.addr[0],
	       ll_addr_get(pdu_adv->tx_addr, NULL), BDADDR_SIZE);
	memcpy(&pdu_scan->payload.scan_rsp.addr[0],
	       ll_addr_get(pdu_adv->tx_addr, NULL), BDADDR_SIZE);

	status = radio_adv_enable(ll_adv.interval, ll_adv.chl_map,
				  ll_adv.filter_policy);

	return status;
}
