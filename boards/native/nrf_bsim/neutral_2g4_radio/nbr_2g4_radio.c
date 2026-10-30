/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Silicon Laboratories Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * The model uses the Phy connection and the interrupt controller of the nRF HW
 * models. The nRF RADIO model uses the same Phy connection, so only one of the
 * two radio models can be in use.
 *
 * The model talks to the 2G4 Phy with the same rules as the nRF RADIO model,
 * so that devices on either model see the same air:
 *  - A possible abort is re-evaluated whenever another HW event of this
 *    device may happen, and answered from the lowest priority HW event of
 *    that microsecond, so that the CPU had a chance to abort first.
 *  - An address match is answered at the device time the address ended.
 *  - The received bytes reach the embedded software when the packet ended.
 */

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "bs_types.h"
#include "bs_tracing.h"
#include "bs_pc_2G4.h"
#include "bs_pc_2G4_types.h"
#include "bs_pc_2G4_utils.h"
#include "crc.h"
#include "nsi_hw_scheduler.h"
#include "nsi_hws_models_if.h"
#include "nsi_utils.h"
#include "NHW_common_types.h"
#include "NHW_config.h"
#include "NRF_HWLowL.h"
#include "irq_ctrl.h"
#include "phy_sync_ctrl.h"

#include "nbr_2g4_radio_if.h"

#define NBR_PDU_HEADER_LEN 2U
#define NBR_CRC_LEN        3U
#define NBR_AA_LEN         4U
#define NBR_PDU_LEN_MAX    (NBR_PDU_HEADER_LEN + 255U)
#define NBR_PKT_LEN_MAX    (NBR_PDU_LEN_MAX + NBR_CRC_LEN)

#define NBR_EVT_QUEUE_LEN 4U

/* Packets from other devices may be longer than any Bluetooth LE packet */
#define NBR_PHY_RX_BUF_LEN 4096U

enum op_state {
	OP_IDLE,
	OP_WAIT_START, /* Waiting for the start time to issue the Phy request */
	OP_IN_PHY,     /* Phy request ongoing */
	OP_WAIT_END,   /* Phy request done, waiting to signal the end */
};

enum nbr_phy_wait {
	PHY_WAIT_NONE,
	PHY_WAIT_REEVAL,  /* Phy waits for an abort reevaluation response */
	PHY_WAIT_ADDRESS, /* Phy waits for an address found response */
};

struct nbr_op {
	enum op_state state;
	enum nbr_phy_wait phy_wait;
	bool is_rx;
	bool aborted;
	uint32_t id;
	struct nbr_pkt_cfg cfg;
	bs_time_t start;    /* Device time of the first preamble bit */
	uint32_t window_us; /* Rx only */
	uint8_t *rx_buf;    /* Rx only, embedded side buffer */
	const uint8_t *tx_pdu; /* Tx only, embedded side PDU, read at Tx start */
	struct nbr_evt evt;

	uint8_t pkt[NBR_PKT_LEN_MAX];
	uint16_t pkt_len;

	union {
		p2G4_txv2_t tx_req;
		p2G4_rxv2_t rx_req;
	};
	p2G4_tx_done_t tx_done;
	p2G4_rxv2_done_t rx_done;
	p2G4_address_t rx_addr[1];
};

static uint8_t phy_rx_buf_mem[NBR_PHY_RX_BUF_LEN];
static uint8_t *phy_rx_buf = phy_rx_buf_mem;

static struct {
	unsigned int irq;
	bool initialized;

	/* Operation the Phy is busy with (it may already be aborted) */
	struct nbr_op *phy_op;
	/* Operation requested by the embedded software and not aborted */
	struct nbr_op *sw_op;
	struct nbr_op ops[2];
	uint32_t last_id;

	struct nbr_evt evt_q[NBR_EVT_QUEUE_LEN];
	uint8_t evt_first;
	uint8_t evt_count;

	bs_time_t timer_start;
	bs_time_t timer_end;
} nbr;

/* The interrupts go to the CPU that the nRF RADIO would interrupt */
static const struct nhw_irq_mapping irq_map[] = NHW_RADIO_INT_MAP;

static bs_time_t timer_radio = TIME_NEVER;
/* Answer to the Phy. This must be the last HW event in a given microsecond */
static bs_time_t timer_phy = TIME_NEVER;

static void radio_timer_update(void)
{
	timer_radio = BS_MIN(nbr.timer_start, nbr.timer_end);
	nsi_hws_find_next_event();
}

static int8_t rssi_dbm_get(p2G4_rssi_power_t rssi)
{
	double dbm = p2G4_RSSI_value_to_dBm(rssi);

	if (dbm < INT8_MIN) {
		return INT8_MIN;
	}

	if (dbm > INT8_MAX) {
		return INT8_MAX;
	}

	return (int8_t)dbm;
}

static uint16_t rf_chan_get(uint8_t chan)
{
	switch (chan) {
	case 37:
		return 0U;
	case 38:
		return 12U;
	case 39:
		return 39U;
	default:
		return (chan < 11U) ? (chan + 1U) : (chan + 2U);
	}
}

static void radio_params_set(p2G4_radioparams_t *params, const struct nbr_pkt_cfg *cfg)
{
	p2G4_freq_t freq;

	/* Frequency given as offset from 2400MHz */
	(void)p2G4_freq_from_d(2.0 + 2.0 * rf_chan_get(cfg->chan), 1, &freq);

	params->center_freq = freq;
	params->modulation = (cfg->phy == NBR_PHY_2M) ? P2G4_MOD_BLE2M : P2G4_MOD_BLE;
}

static uint32_t pream_and_addr_us(uint8_t phy)
{
	return (phy == NBR_PHY_2M) ? ((2U + NBR_AA_LEN) * 8U / 2U) : ((1U + NBR_AA_LEN) * 8U);
}

static uint32_t bytes_us(uint8_t phy, uint32_t bytes)
{
	return (phy == NBR_PHY_2M) ? (bytes * 4U) : (bytes * 8U);
}

static void phy_disconnected(void)
{
	bs_trace_raw_manual_time(3, nsi_hws_get_time(), "The phy disconnected us\n");
	hwll_disconnect_phy_and_exit();
}

static void evt_put(const struct nbr_evt *evt)
{
	uint8_t idx;

	if (nbr.evt_count == NBR_EVT_QUEUE_LEN) {
		bs_trace_error_time_line("nbr_2g4_radio: event queue overflow\n");
	}

	idx = (nbr.evt_first + nbr.evt_count) % NBR_EVT_QUEUE_LEN;
	nbr.evt_q[idx] = *evt;
	nbr.evt_count++;

	hw_irq_ctrl_raise_im(irq_map[0].cntl_inst, nbr.irq);
}

static void op_free(struct nbr_op *op)
{
	if (nbr.phy_op == op) {
		nbr.phy_op = NULL;
	}
	if (nbr.sw_op == op) {
		nbr.sw_op = NULL;
	}

	op->state = OP_IDLE;
	op->phy_wait = PHY_WAIT_NONE;
}

static struct nbr_op *op_alloc(void)
{
	for (size_t i = 0U; i < NSI_ARRAY_SIZE(nbr.ops); i++) {
		if (nbr.ops[i].state == OP_IDLE) {
			return &nbr.ops[i];
		}
	}

	return NULL;
}

static void abort_struct_update(struct nbr_op *op, p2G4_abort_t *phy_abort, bs_time_t *recheck_dev)
{
	bs_time_t now = nsi_hws_get_time();

	/* Recheck next time anything in this device may decide to abort */
	nsi_hws_find_next_event();
	*recheck_dev = BS_MAX(nsi_hws_get_next_event_time(), now);
	phy_abort->recheck_time = hwll_phy_time_from_dev(*recheck_dev);

	if (op->aborted) {
		phy_abort->abort_time = hwll_phy_time_from_dev(now);
	} else {
		phy_abort->abort_time = TIME_NEVER;
	}
}

static void phy_wait_set(struct nbr_op *op, enum nbr_phy_wait wait, bs_time_t when)
{
	op->phy_wait = wait;
	timer_phy = BS_MAX(when, nsi_hws_get_time());
	nsi_hws_find_next_event();
}

static void phy_op_ended(struct nbr_op *op, bs_time_t end_dev)
{
	op->phy_wait = PHY_WAIT_NONE;

	if (op->aborted) {
		op_free(op);
		radio_timer_update();
		return;
	}

	nbr.phy_op = NULL;
	op->state = OP_WAIT_END;
	op->evt.ts_end = end_dev;
	nbr.timer_end = BS_MAX(end_dev, nsi_hws_get_time());
	radio_timer_update();
}

static void tx_response_handle(struct nbr_op *op, int ret)
{
	bs_time_t recheck;

	if (ret == -1) {
		phy_disconnected();
		return;
	}

	if (ret == P2G4_MSG_ABORTREEVAL) {
		recheck = hwll_dev_time_from_phy(op->tx_req.abort.recheck_time);
		phy_sync_ctrl_set_last_phy_sync_time(recheck);
		phy_wait_set(op, PHY_WAIT_REEVAL, recheck);
		return;
	}

	/* P2G4_MSG_TX_END: end_time is the last us of the packet */
	bs_time_t end = hwll_dev_time_from_phy(op->tx_done.end_time) + 1;

	phy_sync_ctrl_set_last_phy_sync_time(end - 1);
	op->evt.status = NBR_STATUS_OK;
	phy_op_ended(op, end);
}

static void rx_end_handle(struct nbr_op *op)
{
	bs_time_t end = hwll_dev_time_from_phy(op->rx_done.end_time) + 1;
	uint8_t status;

	phy_sync_ctrl_set_last_phy_sync_time(end - 1);

	switch (op->rx_done.status) {
	case P2G4_RXSTATUS_OK:
		status = NBR_STATUS_OK;
		break;
	case P2G4_RXSTATUS_NOSYNC:
		status = NBR_STATUS_NO_SYNC;
		break;
	default:
		status = NBR_STATUS_CRC_ERR;
		break;
	}

	if (status != NBR_STATUS_NO_SYNC) {
		uint16_t size = op->rx_done.packet_size;

		if (size > sizeof(op->pkt)) {
			size = sizeof(op->pkt);
		}
		memcpy(op->pkt, phy_rx_buf, size);
		op->pkt_len = size;

		/* The Phy only reports bit errors, verify the CRC ourselves so
		 * that CRC init or access address mismatches are detected too.
		 */
		if ((status == NBR_STATUS_OK) &&
		    ((size < (NBR_PDU_HEADER_LEN + NBR_CRC_LEN)) ||
		     ((NBR_PDU_HEADER_LEN + op->pkt[1] + NBR_CRC_LEN) != size))) {
			status = NBR_STATUS_CRC_ERR;
		}

		if (status == NBR_STATUS_OK) {
			uint8_t rx_crc[NBR_CRC_LEN];

			/* The CRC computed replaces the received one */
			(void)memcpy(rx_crc, &op->pkt[size - NBR_CRC_LEN], NBR_CRC_LEN);
			append_crc_ble(op->pkt, size - NBR_CRC_LEN, op->cfg.crc_init);
			if (memcmp(rx_crc, &op->pkt[size - NBR_CRC_LEN], NBR_CRC_LEN) != 0) {
				status = NBR_STATUS_CRC_ERR;
			}
		}

		/* As with a real radio, a too long packet is truncated and
		 * reported with a CRC error.
		 */
		if ((size >= NBR_PDU_HEADER_LEN) && (op->pkt[1] > op->cfg.max_len)) {
			status = NBR_STATUS_CRC_ERR;
		}

		op->evt.rssi = rssi_dbm_get(op->rx_done.rssi.RSSI);
		op->evt.len =
			(size >= NBR_PDU_HEADER_LEN) ? NSI_MIN(op->pkt[1], op->cfg.max_len) : 0U;
	}

	op->evt.status = status;
	phy_op_ended(op, end);
}

static void rx_response_handle(struct nbr_op *op, int ret)
{
	bs_time_t recheck;

	if (ret == -1) {
		phy_disconnected();
		return;
	}

	if (ret == P2G4_MSG_ABORTREEVAL) {
		recheck = hwll_dev_time_from_phy(op->rx_req.abort.recheck_time);
		phy_sync_ctrl_set_last_phy_sync_time(recheck);
		phy_wait_set(op, PHY_WAIT_REEVAL, recheck);
		return;
	}

	if (ret == P2G4_MSG_RXV2_ADDRESSFOUND) {
		/* rx_time_stamp is the last us of the access address */
		bs_time_t aa_last = hwll_dev_time_from_phy(op->rx_done.rx_time_stamp);

		phy_sync_ctrl_set_last_phy_sync_time(aa_last);
		op->evt.ts_aa_end = aa_last + 1;
		op->evt.ts_start = aa_last + 1 - pream_and_addr_us(op->cfg.phy);
		phy_wait_set(op, PHY_WAIT_ADDRESS, aa_last);
		return;
	}

	/* P2G4_MSG_RXV2_END */
	rx_end_handle(op);
}

static void tx_start(struct nbr_op *op)
{
	p2G4_txv2_t *req = &op->tx_req;
	bs_time_t recheck;
	uint32_t dur;
	uint8_t len;
	int ret;

	/* The PDU is read when the transmission starts, as with DMA */
	len = op->tx_pdu[1];
	memcpy(op->pkt, op->tx_pdu, NBR_PDU_HEADER_LEN + len);

	op->pkt_len = NBR_PDU_HEADER_LEN + len + NBR_CRC_LEN;
	append_crc_ble(op->pkt, NBR_PDU_HEADER_LEN + len, op->cfg.crc_init);

	dur = pream_and_addr_us(op->cfg.phy) + bytes_us(op->cfg.phy, op->pkt_len);

	memset(req, 0, sizeof(*req));
	radio_params_set(&req->radio_params, &op->cfg);
	req->phy_address = op->cfg.aa;
	req->power_level = p2G4_power_from_d(op->cfg.tx_power);
	req->packet_size = op->pkt_len;
	req->coding_rate = 0;
	req->start_tx_time = hwll_phy_time_from_dev(op->start);
	req->start_packet_time = req->start_tx_time;
	req->end_tx_time = req->start_tx_time + dur - 1;
	req->end_packet_time = req->end_tx_time;
	abort_struct_update(op, &req->abort, &recheck);

	op->evt.ts_start = op->start;
	op->evt.ts_aa_end = op->start + pream_and_addr_us(op->cfg.phy);

	op->state = OP_IN_PHY;
	nbr.phy_op = op;

	ret = p2G4_dev_req_txv2_nc_b(req, op->pkt, &op->tx_done);
	tx_response_handle(op, ret);
}

static void rx_start(struct nbr_op *op)
{
	p2G4_rxv2_t *req = &op->rx_req;
	bs_time_t recheck;
	int ret;

	memset(req, 0, sizeof(*req));
	radio_params_set(&req->radio_params, &op->cfg);
	req->start_time = hwll_phy_time_from_dev(op->start);
	/* The access address must have ended by start + window_us */
	req->scan_duration = (op->window_us != 0U) ? (op->window_us + 1U) : UINT32_MAX;
	req->forced_packet_duration = UINT32_MAX;
	req->error_calc_rate = (op->cfg.phy == NBR_PHY_2M) ? 2000000U : 1000000U;
	req->antenna_gain = 0;
	req->coding_rate = 0;
	req->pream_and_addr_duration = pream_and_addr_us(op->cfg.phy);
	req->header_duration = bytes_us(op->cfg.phy, NBR_PDU_HEADER_LEN);
	/* Never stop at the header: bit errors in it are reported at the end of
	 * the packet as a CRC error, as if the length was received correctly,
	 * which is what the nRF RADIO model does too.
	 */
	req->header_threshold = 0xFFFF;
	req->sync_threshold = 2;
	req->acceptable_pre_truncation = 0;
	req->prelocked_tx = false;
	req->resp_type = 0;
	req->n_addr = 1;
	op->rx_addr[0] = op->cfg.aa;
	abort_struct_update(op, &req->abort, &recheck);

	op->state = OP_IN_PHY;
	nbr.phy_op = op;

	ret = p2G4_dev_req_rxv2_nc_b(req, op->rx_addr, &op->rx_done, &phy_rx_buf,
				     sizeof(phy_rx_buf_mem));
	rx_response_handle(op, ret);
}

static void phy_respond(struct nbr_op *op)
{
	enum nbr_phy_wait wait = op->phy_wait;
	bs_time_t recheck;
	int ret;

	op->phy_wait = PHY_WAIT_NONE;

	if (wait == PHY_WAIT_ADDRESS) {
		if (op->aborted) {
			(void)p2G4_dev_rxv2_cont_after_addr_nc_b(false, NULL);
			phy_sync_ctrl_set_last_phy_sync_time(nsi_hws_get_time());
			op_free(op);
			radio_timer_update();
			return;
		}

		abort_struct_update(op, &op->rx_req.abort, &recheck);
		ret = p2G4_dev_rxv2_cont_after_addr_nc_b(true, &op->rx_req.abort);
		rx_response_handle(op, ret);
		return;
	}

	if (op->is_rx) {
		abort_struct_update(op, &op->rx_req.abort, &recheck);
		ret = p2G4_dev_provide_new_rxv2_abort_nc_b(&op->rx_req.abort);
		rx_response_handle(op, ret);
	} else {
		abort_struct_update(op, &op->tx_req.abort, &recheck);
		ret = p2G4_dev_provide_new_tx_abort_nc_b(&op->tx_req.abort);
		tx_response_handle(op, ret);
	}
}

static void radio_timer_triggered(void)
{
	bs_time_t now = nsi_hws_get_time();
	struct nbr_op *op;

	if (nbr.timer_end <= now) {
		nbr.timer_end = TIME_NEVER;

		for (size_t i = 0U; i < NSI_ARRAY_SIZE(nbr.ops); i++) {
			op = &nbr.ops[i];
			if (op->state != OP_WAIT_END) {
				continue;
			}

			struct nbr_evt evt = op->evt;

			if (op->is_rx && (evt.status != NBR_STATUS_NO_SYNC)) {
				memcpy(op->rx_buf, op->pkt, NBR_PDU_HEADER_LEN + evt.len);
			}

			/* Raising the interrupt runs the embedded SW right
			 * away, which may already request the next operation.
			 */
			op_free(op);
			evt_put(&evt);
		}
	}

	if (nbr.timer_start <= now) {
		op = nbr.sw_op;

		if ((op != NULL) && (op->state == OP_WAIT_START)) {
			if (nbr.phy_op != NULL) {
				/* A previous aborted op still holds the Phy. This
				 * can not happen as the Phy is always answered at
				 * or before the next HW event of this device.
				 */
				bs_trace_error_time_line("nbr_2g4_radio: Phy busy at op start\n");
			} else {
				nbr.timer_start = TIME_NEVER;
				radio_timer_update();
				if (op->is_rx) {
					rx_start(op);
				} else {
					tx_start(op);
				}
			}
		} else {
			nbr.timer_start = TIME_NEVER;
		}
	}

	radio_timer_update();
}

NSI_HW_EVENT(timer_radio, radio_timer_triggered, 998);

static void phy_timer_triggered(void)
{
	timer_phy = TIME_NEVER;
	nsi_hws_find_next_event();

	if (nbr.phy_op != NULL) {
		phy_respond(nbr.phy_op);
	}
}

NSI_HW_EVENT(timer_phy, phy_timer_triggered, 999);

void nbr_init(unsigned int irq)
{
	memset(&nbr, 0, sizeof(nbr));

	nbr.irq = irq;
	nbr.timer_start = TIME_NEVER;
	nbr.timer_end = TIME_NEVER;
	nbr.initialized = true;

	timer_phy = TIME_NEVER;
	radio_timer_update();
}

static struct nbr_op *op_request(const struct nbr_pkt_cfg *cfg, bs_time_t start, bool is_rx)
{
	struct nbr_op *op;

	if (!nbr.initialized || (nbr.sw_op != NULL) || (start <= nsi_hws_get_time())) {
		return NULL;
	}

	op = op_alloc();
	if (op == NULL) {
		return NULL;
	}

	memset(&op->evt, 0, sizeof(op->evt));
	op->state = OP_WAIT_START;
	op->phy_wait = PHY_WAIT_NONE;
	op->is_rx = is_rx;
	op->aborted = false;
	op->cfg = *cfg;
	op->start = start;
	nbr.last_id++;
	if (nbr.last_id == 0U) {
		nbr.last_id = 1U;
	}
	op->id = nbr.last_id;

	op->evt.id = op->id;
	op->evt.is_rx = is_rx;

	nbr.sw_op = op;
	nbr.timer_start = start;
	radio_timer_update();

	return op;
}

uint32_t nbr_tx(const struct nbr_pkt_cfg *cfg, uint64_t at, const uint8_t *pdu)
{
	struct nbr_op *op = op_request(cfg, at, false);

	if (op == NULL) {
		return 0U;
	}

	op->tx_pdu = pdu;

	return op->id;
}

uint32_t nbr_rx(const struct nbr_pkt_cfg *cfg, uint64_t start, uint32_t window_us,
		uint8_t *buf)
{
	struct nbr_op *op = op_request(cfg, start, true);

	if (op == NULL) {
		return 0U;
	}

	op->window_us = window_us;
	op->rx_buf = buf;

	return op->id;
}

void nbr_abort(void)
{
	struct nbr_op *op = nbr.sw_op;

	if (op == NULL) {
		return;
	}

	nbr.sw_op = NULL;

	switch (op->state) {
	case OP_WAIT_START:
		nbr.timer_start = TIME_NEVER;
		op_free(op);
		break;
	case OP_IN_PHY:
		/* The abort is given to the Phy when it is next answered,
		 * which is at the latest at the time of the next HW event of
		 * this device.
		 */
		op->aborted = true;
		break;
	case OP_WAIT_END:
		nbr.timer_end = TIME_NEVER;
		op_free(op);
		break;
	default:
		break;
	}

	radio_timer_update();
}

bool nbr_evt_get(struct nbr_evt *evt)
{
	if (nbr.evt_count == 0U) {
		return false;
	}

	*evt = nbr.evt_q[nbr.evt_first];
	nbr.evt_first = (nbr.evt_first + 1U) % NBR_EVT_QUEUE_LEN;
	nbr.evt_count--;

	return true;
}
