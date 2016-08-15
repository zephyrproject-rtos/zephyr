/*
 * Copyright (c) 2016 Nordic Semiconductor ASA
 * Copyright (c) 2016 Vinayak Kariappa Chettimada
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <stdint.h>
#include <string.h>
#include "defines.h"
#include "mem.h"
#include "ticker.h"
#include "ccm.h"
#include "radio.h"
#include "pdu.h"
#include "ctrl.h"

#include "ll.h"

#include "debug.h"

static struct {
	uint8_t pub_addr[BDADDR_SIZE];
	uint8_t rnd_addr[BDADDR_SIZE];
} _ll_context;

static struct {
	uint16_t interval;
	uint8_t adv_type:4;
	uint8_t tx_addr:1;
	uint8_t rx_addr:1;
	uint8_t filter_policy:2;
	uint8_t chl_map:3;
	uint8_t adv_addr[BDADDR_SIZE];
	uint8_t direct_addr[BDADDR_SIZE];
} _ll_adv_params;

static struct {
	uint16_t interval;
	uint16_t window;
	uint8_t scan_type:1;
	uint8_t tx_addr:1;
	uint8_t filter_policy:1;
} _ll_scan_params;

void ll_address_get(uint8_t addr_type, uint8_t *bdaddr)
{
	if (addr_type) {
		memcpy(bdaddr, &_ll_context.rnd_addr[0], BDADDR_SIZE);
	} else {
		memcpy(bdaddr, &_ll_context.pub_addr[0], BDADDR_SIZE);
	}
}

void ll_address_set(uint8_t addr_type, uint8_t const *const bdaddr)
{
	if (addr_type) {
		memcpy(&_ll_context.rnd_addr[0], bdaddr, BDADDR_SIZE);
	} else {
		memcpy(&_ll_context.pub_addr[0], bdaddr, BDADDR_SIZE);
	}
}

void ll_adv_params_set(uint16_t interval, uint8_t adv_type,
		       uint8_t own_addr_type, uint8_t direct_addr_type,
		       uint8_t const *const direct_addr, uint8_t chl_map,
		       uint8_t filter_policy)
{
	struct radio_adv_data *radio_adv_data;
	struct pdu_adv *pdu;

	/** @todo check and fail if adv role active else
	 * update (implemented below) current index elements for
	 * both adv and scan data.
	 */

	/* remember params so that set adv/scan data and adv enable
	 * interface can correctly update adv/scan data in the
	 * double buffer between caller and controller context.
	 */
	_ll_adv_params.interval = interval;
	_ll_adv_params.chl_map = chl_map;
	_ll_adv_params.filter_policy = filter_policy;
	_ll_adv_params.adv_type = adv_type;
	_ll_adv_params.tx_addr = own_addr_type;
	_ll_adv_params.rx_addr = 0;

	/* update the current adv data */
	radio_adv_data = radio_adv_data_get();
	pdu = (struct pdu_adv *)&radio_adv_data->data[radio_adv_data->last][0];
	pdu->type = _ll_adv_params.adv_type;
	pdu->tx_addr = _ll_adv_params.tx_addr;
	if (adv_type == PDU_ADV_TYPE_DIRECT_IND) {
		_ll_adv_params.rx_addr = direct_addr_type;
		memcpy(&_ll_adv_params.direct_addr[0], direct_addr,
			 BDADDR_SIZE);
		memcpy(&pdu->payload.direct_ind.init_addr[0],
			 direct_addr, BDADDR_SIZE);
		pdu->len = sizeof(struct pdu_adv_payload_direct_ind);
	} else if (pdu->len == 0) {
		pdu->len = BDADDR_SIZE;
	}
	pdu->rx_addr = _ll_adv_params.rx_addr;

	/* update the current scan data */
	radio_adv_data = radio_scan_data_get();
	pdu = (struct pdu_adv *)&radio_adv_data->data[radio_adv_data->last][0];
	pdu->type = PDU_ADV_TYPE_SCAN_RESP;
	pdu->tx_addr = _ll_adv_params.tx_addr;
	pdu->rx_addr = 0;
	if (pdu->len == 0) {
		pdu->len = BDADDR_SIZE;
	}
}

void ll_adv_data_set(uint8_t len, uint8_t const *const data)
{
	struct radio_adv_data *radio_adv_data;
	struct pdu_adv *pdu;
	uint8_t last;

	/** @todo dont update data if directed adv type. */

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
	pdu = (struct pdu_adv *)&radio_adv_data->data[last][0];
	pdu->type = _ll_adv_params.adv_type;
	pdu->tx_addr = _ll_adv_params.tx_addr;
	pdu->rx_addr = _ll_adv_params.rx_addr;
	memcpy(&pdu->payload.adv_ind.addr[0],
		 &_ll_adv_params.adv_addr[0], BDADDR_SIZE);
	if (_ll_adv_params.adv_type == PDU_ADV_TYPE_DIRECT_IND) {
		memcpy(&pdu->payload.direct_ind.init_addr[0],
			 &_ll_adv_params.direct_addr[0], BDADDR_SIZE);
		pdu->len = sizeof(struct pdu_adv_payload_direct_ind);
	} else {
		memcpy(&pdu->payload.adv_ind.data[0], data, len);
		pdu->len = BDADDR_SIZE + len;
	}

	/* commit the update so controller picks it. */
	radio_adv_data->last = last;
}

void ll_scan_data_set(uint8_t len, uint8_t const *const data)
{
	struct radio_adv_data *radio_scan_data;
	struct pdu_adv *pdu;
	uint8_t last;

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
	pdu = (struct pdu_adv *)&radio_scan_data->data[last][0];
	pdu->type = PDU_ADV_TYPE_SCAN_RESP;
	pdu->tx_addr = _ll_adv_params.tx_addr;
	pdu->rx_addr = 0;
	pdu->len = BDADDR_SIZE + len;
	memcpy(&pdu->payload.scan_resp.addr[0],
		 &_ll_adv_params.adv_addr[0], BDADDR_SIZE);
	memcpy(&pdu->payload.scan_resp.data[0], data, len);

	/* commit the update so controller picks it. */
	radio_scan_data->last = last;
}

uint32_t ll_adv_enable(uint8_t enable)
{
	uint32_t status;

	if (enable) {
		struct radio_adv_data *radio_adv_data;
		struct radio_adv_data *radio_scan_data;
		struct pdu_adv *pdu_adv;
		struct pdu_adv *pdu_scan;

		/** @todo move the addr remembered into controller
		 * this way when implementing Privacy 1.2, generated
		 * new resolvable addresses can be used instantly.
		 */

		/* remember addr to use and also update the addr in
		 * both adv and scan PDUs.
		 */
		radio_adv_data = radio_adv_data_get();
		radio_scan_data = radio_scan_data_get();
		pdu_adv = (struct pdu_adv *)&radio_adv_data->data
				[radio_adv_data->last][0];
		pdu_scan = (struct pdu_adv *)&radio_scan_data->data
				[radio_scan_data->last][0];
		if (_ll_adv_params.tx_addr) {
			memcpy(&_ll_adv_params.adv_addr[0],
				 &_ll_context.rnd_addr[0], BDADDR_SIZE);
			memcpy(&pdu_adv->payload.adv_ind.addr[0],
				 &_ll_context.rnd_addr[0], BDADDR_SIZE);
			memcpy(&pdu_scan->payload.scan_resp.addr[0],
				 &_ll_context.rnd_addr[0], BDADDR_SIZE);
		} else {
			memcpy(&_ll_adv_params.adv_addr[0],
				 &_ll_context.pub_addr[0], BDADDR_SIZE);
			memcpy(&pdu_adv->payload.adv_ind.addr[0],
				 &_ll_context.pub_addr[0], BDADDR_SIZE);
			memcpy(&pdu_scan->payload.scan_resp.addr[0],
				 &_ll_context.pub_addr[0], BDADDR_SIZE);
		}

		status = radio_adv_enable(_ll_adv_params.interval,
						_ll_adv_params.chl_map,
						_ll_adv_params.filter_policy);
	} else {
		status = radio_adv_disable();
	}

	return status;
}

void ll_scan_params_set(uint8_t scan_type, uint16_t interval, uint16_t window,
			uint8_t own_addr_type, uint8_t filter_policy)
{
	_ll_scan_params.scan_type = scan_type;
	_ll_scan_params.interval = interval;
	_ll_scan_params.window = window;
	_ll_scan_params.tx_addr = own_addr_type;
	_ll_scan_params.filter_policy = filter_policy;
}

uint32_t ll_scan_enable(uint8_t enable)
{
	uint32_t status;

	if (enable) {
		status = radio_scan_enable(_ll_scan_params.scan_type,
				_ll_scan_params.tx_addr,
				(_ll_scan_params.tx_addr) ?
					&_ll_context.rnd_addr[0] :
					&_ll_context.pub_addr[0],
				_ll_scan_params.interval,
				_ll_scan_params.window,
				_ll_scan_params.filter_policy);
	} else {
		status = radio_scan_disable();
	}

	return status;
}

uint32_t ll_create_connection(uint16_t scan_interval, uint16_t scan_window,
			      uint8_t filter_policy, uint8_t peer_addr_type,
			      uint8_t *peer_addr, uint8_t own_addr_type,
			      uint16_t interval, uint16_t latency,
			      uint16_t timeout)
{
	uint32_t status;

	status = radio_connect_enable(peer_addr_type, peer_addr, interval,
					latency, timeout);

	if (status) {
		return status;
	}

	return radio_scan_enable(0, own_addr_type, (own_addr_type) ?
			&_ll_context.rnd_addr[0] :
			&_ll_context.pub_addr[0],
		scan_interval, scan_window, filter_policy);
}
