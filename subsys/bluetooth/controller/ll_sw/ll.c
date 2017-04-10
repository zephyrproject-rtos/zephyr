/*
 * Copyright (c) 2016-2017 Nordic Semiconductor ASA
 * Copyright (c) 2016 Vinayak Kariappa Chettimada
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>
#include <string.h>

#include <soc.h>
#include <device.h>
#include <clock_control.h>
#ifdef CONFIG_CLOCK_CONTROL_NRF5
#include <drivers/clock_control/nrf5_clock_control.h>
#endif

#define BT_DBG_ENABLED IS_ENABLED(CONFIG_BLUETOOTH_DEBUG_HCI_DRIVER)
#include <bluetooth/log.h>

#include "hal/cpu.h"
#include "hal/cntr.h"
#include "hal/rand.h"
#include "hal/ccm.h"
#include "hal/radio.h"
#include "hal/debug.h"

#include "util/util.h"
#include "util/mem.h"
#include "util/mayfly.h"

#include "ticker/ticker.h"

#include "pdu.h"
#include "ctrl.h"
#include "ctrl_internal.h"
#include "ll.h"

/* Global singletons */
static uint8_t MALIGN(4) _rand_context[3 + 4 + 1];
static uint8_t MALIGN(4) _ticker_nodes[RADIO_TICKER_NODES][TICKER_NODE_T_SIZE];
static uint8_t MALIGN(4) _ticker_users[MAYFLY_CALLER_COUNT]
						[TICKER_USER_T_SIZE];
static uint8_t MALIGN(4) _ticker_user_ops[RADIO_TICKER_USER_OPS]
						[TICKER_USER_OP_T_SIZE];
static uint8_t MALIGN(4) _radio[LL_MEM_TOTAL];

static struct k_sem *sem_recv;

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

void mayfly_enable_cb(uint8_t caller_id, uint8_t callee_id, uint8_t enable)
{
	(void)caller_id;

	LL_ASSERT(callee_id == MAYFLY_CALL_ID_1);

	if (enable) {
		irq_enable(SWI4_IRQn);
	} else {
		irq_disable(SWI4_IRQn);
	}
}

uint32_t mayfly_is_enabled(uint8_t caller_id, uint8_t callee_id)
{
	(void)caller_id;

	if (callee_id == MAYFLY_CALL_ID_0) {
		return irq_is_enabled(RTC0_IRQn);
	} else if (callee_id == MAYFLY_CALL_ID_1) {
		return irq_is_enabled(SWI4_IRQn);
	}

	LL_ASSERT(0);

	return 0;
}

uint32_t mayfly_prio_is_equal(uint8_t caller_id, uint8_t callee_id)
{
	return (caller_id == callee_id) ||
	       ((caller_id == MAYFLY_CALL_ID_0) &&
		(callee_id == MAYFLY_CALL_ID_1)) ||
	       ((caller_id == MAYFLY_CALL_ID_1) &&
		(callee_id == MAYFLY_CALL_ID_0));
}

void mayfly_pend(uint8_t caller_id, uint8_t callee_id)
{
	(void)caller_id;

	switch (callee_id) {
	case MAYFLY_CALL_ID_0:
		NVIC_SetPendingIRQ(RTC0_IRQn);
		break;

	case MAYFLY_CALL_ID_1:
		NVIC_SetPendingIRQ(SWI4_IRQn);
		break;

	case MAYFLY_CALL_ID_PROGRAM:
	default:
		LL_ASSERT(0);
		break;
	}
}

void radio_active_callback(uint8_t active)
{
}

void radio_event_callback(void)
{
	k_sem_give(sem_recv);
}

ISR_DIRECT_DECLARE(radio_nrf5_isr)
{
	isr_radio();

	ISR_DIRECT_PM();
	return 1;
}

static void rtc0_nrf5_isr(void *arg)
{
	uint32_t compare0, compare1;

	/* store interested events */
	compare0 = NRF_RTC0->EVENTS_COMPARE[0];
	compare1 = NRF_RTC0->EVENTS_COMPARE[1];

	/* On compare0 run ticker worker instance0 */
	if (compare0) {
		NRF_RTC0->EVENTS_COMPARE[0] = 0;

		ticker_trigger(0);
	}

	/* On compare1 run ticker worker instance1 */
	if (compare1) {
		NRF_RTC0->EVENTS_COMPARE[1] = 0;

		ticker_trigger(1);
	}

	mayfly_run(MAYFLY_CALL_ID_0);
}

static void rng_nrf5_isr(void *arg)
{
	isr_rand(arg);
}

static void swi4_nrf5_isr(void *arg)
{
	mayfly_run(MAYFLY_CALL_ID_1);
}

int ll_init(struct k_sem *sem_rx)
{
	struct device *clk_k32;
	struct device *clk_m16;
	uint32_t err;

	sem_recv = sem_rx;

	/* TODO: bind and use RNG driver */
	rand_init(_rand_context, sizeof(_rand_context));

	clk_k32 = device_get_binding(CONFIG_CLOCK_CONTROL_NRF5_K32SRC_DRV_NAME);
	if (!clk_k32) {
		return -ENODEV;
	}

	clock_control_on(clk_k32, (void *)CLOCK_CONTROL_NRF5_K32SRC);

	/* TODO: bind and use counter driver */
	cntr_init();

	mayfly_init();

	_ticker_users[MAYFLY_CALL_ID_0][0] = RADIO_TICKER_USER_WORKER_OPS;
	_ticker_users[MAYFLY_CALL_ID_1][0] = RADIO_TICKER_USER_JOB_OPS;
	_ticker_users[MAYFLY_CALL_ID_2][0] = 0;
	_ticker_users[MAYFLY_CALL_ID_PROGRAM][0] = RADIO_TICKER_USER_APP_OPS;

	ticker_init(RADIO_TICKER_INSTANCE_ID_RADIO, RADIO_TICKER_NODES,
		    &_ticker_nodes[0], MAYFLY_CALLER_COUNT, &_ticker_users[0],
		    RADIO_TICKER_USER_OPS, &_ticker_user_ops[0]);

	clk_m16 = device_get_binding(CONFIG_CLOCK_CONTROL_NRF5_M16SRC_DRV_NAME);
	if (!clk_m16) {
		return -ENODEV;
	}

	err = radio_init(clk_m16, CLOCK_CONTROL_NRF5_K32SRC_ACCURACY,
			 RADIO_CONNECTION_CONTEXT_MAX,
			 RADIO_PACKET_COUNT_RX_MAX,
			 RADIO_PACKET_COUNT_TX_MAX,
			 RADIO_LL_LENGTH_OCTETS_RX_MAX,
			 RADIO_PACKET_TX_DATA_SIZE, &_radio[0], sizeof(_radio));
	if (err) {
		BT_ERR("Required RAM size: %d, supplied: %u.", err,
		       sizeof(_radio));
		return -ENOMEM;
	}

	IRQ_DIRECT_CONNECT(NRF5_IRQ_RADIO_IRQn,
			   CONFIG_BLUETOOTH_CONTROLLER_WORKER_PRIO,
			   radio_nrf5_isr, 0);
	IRQ_CONNECT(NRF5_IRQ_RTC0_IRQn, CONFIG_BLUETOOTH_CONTROLLER_WORKER_PRIO,
		    rtc0_nrf5_isr, NULL, 0);
	IRQ_CONNECT(NRF5_IRQ_SWI4_IRQn, CONFIG_BLUETOOTH_CONTROLLER_JOB_PRIO,
		    swi4_nrf5_isr, NULL, 0);
	IRQ_CONNECT(NRF5_IRQ_RNG_IRQn, 1, rng_nrf5_isr, NULL, 0);

	irq_enable(NRF5_IRQ_RADIO_IRQn);
	irq_enable(NRF5_IRQ_RTC0_IRQn);
	irq_enable(NRF5_IRQ_SWI4_IRQn);
	irq_enable(NRF5_IRQ_RNG_IRQn);

	return 0;
}

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
	pdu->rfu = 0;
	pdu->ch_sel = 0;
	pdu->tx_addr = _ll_adv_params.tx_addr;
	if (adv_type == PDU_ADV_TYPE_DIRECT_IND) {
		_ll_adv_params.rx_addr = direct_addr_type;
		memcpy(&_ll_adv_params.direct_addr[0], direct_addr,
			 BDADDR_SIZE);
		memcpy(&pdu->payload.direct_ind.tgt_addr[0],
			 direct_addr, BDADDR_SIZE);
		pdu->len = sizeof(struct pdu_adv_payload_direct_ind);
	} else if (pdu->len == 0) {
		pdu->len = BDADDR_SIZE;
	}
	pdu->rx_addr = _ll_adv_params.rx_addr;
	pdu->resv = 0;

	/* update the current scan data */
	radio_adv_data = radio_scan_data_get();
	pdu = (struct pdu_adv *)&radio_adv_data->data[radio_adv_data->last][0];
	pdu->type = PDU_ADV_TYPE_SCAN_RSP;
	pdu->rfu = 0;
	pdu->ch_sel = 0;
	pdu->tx_addr = _ll_adv_params.tx_addr;
	pdu->rx_addr = 0;
	if (pdu->len == 0) {
		pdu->len = BDADDR_SIZE;
	}
	pdu->resv = 0;
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
	pdu->rfu = 0;
	pdu->ch_sel = 0;
	pdu->tx_addr = _ll_adv_params.tx_addr;
	pdu->rx_addr = _ll_adv_params.rx_addr;
	memcpy(&pdu->payload.adv_ind.addr[0],
		 &_ll_adv_params.adv_addr[0], BDADDR_SIZE);
	if (_ll_adv_params.adv_type == PDU_ADV_TYPE_DIRECT_IND) {
		memcpy(&pdu->payload.direct_ind.tgt_addr[0],
			 &_ll_adv_params.direct_addr[0], BDADDR_SIZE);
		pdu->len = sizeof(struct pdu_adv_payload_direct_ind);
	} else {
		memcpy(&pdu->payload.adv_ind.data[0], data, len);
		pdu->len = BDADDR_SIZE + len;
	}
	pdu->resv = 0;

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
	pdu->type = PDU_ADV_TYPE_SCAN_RSP;
	pdu->rfu = 0;
	pdu->ch_sel = 0;
	pdu->tx_addr = _ll_adv_params.tx_addr;
	pdu->rx_addr = 0;
	pdu->len = BDADDR_SIZE + len;
	memcpy(&pdu->payload.scan_rsp.addr[0],
		 &_ll_adv_params.adv_addr[0], BDADDR_SIZE);
	memcpy(&pdu->payload.scan_rsp.data[0], data, len);
	pdu->resv = 0;

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
			memcpy(&pdu_scan->payload.scan_rsp.addr[0],
				 &_ll_context.rnd_addr[0], BDADDR_SIZE);
		} else {
			memcpy(&_ll_adv_params.adv_addr[0],
				 &_ll_context.pub_addr[0], BDADDR_SIZE);
			memcpy(&pdu_adv->payload.adv_ind.addr[0],
				 &_ll_context.pub_addr[0], BDADDR_SIZE);
			memcpy(&pdu_scan->payload.scan_rsp.addr[0],
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
