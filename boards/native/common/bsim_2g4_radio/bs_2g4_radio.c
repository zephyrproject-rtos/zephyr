/*
 * Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
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

#include "bs_2g4_radio_if.h"
#include "bs_2g4_radio_platform.h"

#define BSR_PDU_HEADER_LEN 2U
#define BSR_CRC_LEN        3U
#define BSR_AA_LEN         4U
#define BSR_PDU_LEN_MAX    (BSR_PDU_HEADER_LEN + 255U)
#define BSR_PKT_LEN_MAX    (BSR_PDU_LEN_MAX + BSR_CRC_LEN)

#define BSR_EVT_QUEUE_LEN 4U

/* Packets from other devices may be longer than any Bluetooth LE packet */
#define BSR_PHY_RX_BUF_LEN 4096U

enum op_state {
	OP_IDLE,
	OP_WAIT_START, /* Waiting for the start time to issue the Phy request */
	OP_IN_PHY,     /* Phy request ongoing */
	OP_WAIT_END,   /* Phy request done, waiting to signal the end */
};

enum phy_wait {
	PHY_WAIT_NONE,
	PHY_WAIT_REEVAL,  /* Phy waits for an abort reevaluation response */
	PHY_WAIT_ADDRESS, /* Phy waits for an address found response */
};

struct op {
	enum op_state state;
	enum phy_wait phy_wait;
	bool is_rx;
	bool aborted;
	uint32_t id;
	struct bsr_pkt_cfg cfg;
	bs_time_t start;    /* Device time of the first preamble bit */
	uint32_t window_us; /* Rx only */
	uint8_t *rx_buf;    /* Rx only, embedded side buffer */
	const uint8_t *tx_pdu; /* Tx only, embedded side PDU, read at Tx start */
	struct bsr_evt evt;

	uint8_t pkt[BSR_PKT_LEN_MAX];
	uint16_t pkt_len;

	union {
		p2G4_txv2_t tx_req;
		p2G4_rxv2_t rx_req;
	};
	p2G4_tx_done_t tx_done;
	p2G4_rxv2_done_t rx_done;
	p2G4_address_t rx_addr[1];
};

static uint8_t phy_rx_buf_mem[BSR_PHY_RX_BUF_LEN];
static uint8_t *phy_rx_buf = phy_rx_buf_mem;

static struct {
	unsigned int radio_irq;
	unsigned int cntr_irq;
	bool initialized;

	/* Operation the Phy is busy with (it may already be aborted) */
	struct op *phy_op;
	/* Operation requested by the embedded software and not aborted */
	struct op *sw_op;
	struct op ops[2];
	uint32_t last_id;

	struct bsr_evt evt_q[BSR_EVT_QUEUE_LEN];
	uint8_t evt_first;
	uint8_t evt_count;

	bs_time_t timer_start;
	bs_time_t timer_end;
} bsr;

/* Test cheats (bsr_testcheat_*()), kept over bsr_init() */
static struct {
	double tx_power_offset;
	double rx_power_offset;
	int64_t tx_disabled;
	int64_t rx_dont_sync;
	int64_t rx_fail_crc;
} cheat;

static bs_time_t timer_cntr = TIME_NEVER;
static bool cntr_cmp_evt;
static bs_time_t timer_radio = TIME_NEVER;
/* Answer to the Phy. This must be the last HW event in a given microsecond */
static bs_time_t timer_phy = TIME_NEVER;

static void radio_timer_update(void)
{
	timer_radio = BS_MIN(bsr.timer_start, bsr.timer_end);
	nsi_hws_find_next_event();
}

static bool cheat_applies(int64_t *count)
{
	if (*count == 0) {
		return false;
	}

	if (*count > 0) {
		(*count)--;
	}

	return true;
}

static int8_t rssi_dbm_get(p2G4_rssi_power_t rssi)
{
	double dbm = p2G4_RSSI_value_to_dBm(rssi) + cheat.rx_power_offset;

	if (dbm < INT8_MIN) {
		return INT8_MIN;
	}

	if (dbm > INT8_MAX) {
		return INT8_MAX;
	}

	return (int8_t)dbm;
}

static bs_time_t time_from_cntr(uint32_t value)
{
	bs_time_t now = nsi_hws_get_time();

	return now + (int32_t)(value - (uint32_t)now);
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

static void radio_params_set(p2G4_radioparams_t *params, const struct bsr_pkt_cfg *cfg)
{
	p2G4_freq_t freq;

	/* Frequency given as offset from 2400MHz */
	(void)p2G4_freq_from_d(2.0 + 2.0 * rf_chan_get(cfg->chan), 1, &freq);

	params->center_freq = freq;
	params->modulation = (cfg->phy == BSR_PHY_2M) ? P2G4_MOD_BLE2M : P2G4_MOD_BLE;
}

static uint32_t pream_and_addr_us(uint8_t phy)
{
	return (phy == BSR_PHY_2M) ? ((2U + BSR_AA_LEN) * 8U / 2U) : ((1U + BSR_AA_LEN) * 8U);
}

static uint32_t bytes_us(uint8_t phy, uint32_t bytes)
{
	return (phy == BSR_PHY_2M) ? (bytes * 4U) : (bytes * 8U);
}

static void evt_put(const struct bsr_evt *evt)
{
	uint8_t idx;

	if (bsr.evt_count == BSR_EVT_QUEUE_LEN) {
		bs_trace_error_time_line("bs_2g4_radio: event queue overflow\n");
	}

	idx = (bsr.evt_first + bsr.evt_count) % BSR_EVT_QUEUE_LEN;
	bsr.evt_q[idx] = *evt;
	bsr.evt_count++;

	bsr_plat_irq_raise(bsr.radio_irq);
}

static void op_free(struct op *op)
{
	if (bsr.phy_op == op) {
		bsr.phy_op = NULL;
	}
	if (bsr.sw_op == op) {
		bsr.sw_op = NULL;
	}

	op->state = OP_IDLE;
	op->phy_wait = PHY_WAIT_NONE;
}

static struct op *op_alloc(void)
{
	for (size_t i = 0U; i < NSI_ARRAY_SIZE(bsr.ops); i++) {
		if (bsr.ops[i].state == OP_IDLE) {
			return &bsr.ops[i];
		}
	}

	return NULL;
}

static void abort_struct_update(struct op *op, p2G4_abort_t *abort, bs_time_t *recheck_dev)
{
	bs_time_t now = nsi_hws_get_time();

	/* Recheck next time anything in this device may decide to abort */
	nsi_hws_find_next_event();
	*recheck_dev = BS_MAX(nsi_hws_get_next_event_time(), now);
	abort->recheck_time = bsr_plat_phy_time_from_dev(*recheck_dev);

	if (op->aborted) {
		abort->abort_time = bsr_plat_phy_time_from_dev(now);
	} else {
		abort->abort_time = TIME_NEVER;
	}
}

static void phy_wait_set(struct op *op, enum phy_wait wait, bs_time_t when)
{
	op->phy_wait = wait;
	timer_phy = BS_MAX(when, nsi_hws_get_time());
	nsi_hws_find_next_event();
}

static void phy_op_ended(struct op *op, bs_time_t end_dev)
{
	op->phy_wait = PHY_WAIT_NONE;

	if (op->aborted) {
		op_free(op);
		radio_timer_update();
		return;
	}

	bsr.phy_op = NULL;
	op->state = OP_WAIT_END;
	op->evt.ts_end = (uint32_t)end_dev;
	bsr.timer_end = BS_MAX(end_dev, nsi_hws_get_time());
	radio_timer_update();
}

static void tx_response_handle(struct op *op, int ret)
{
	bs_time_t recheck;

	if (ret == -1) {
		bsr_plat_phy_disconnected();
		return;
	}

	if (ret == P2G4_MSG_ABORTREEVAL) {
		recheck = bsr_plat_dev_time_from_phy(op->tx_req.abort.recheck_time);
		bsr_plat_phy_synced(recheck);
		phy_wait_set(op, PHY_WAIT_REEVAL, recheck);
		return;
	}

	/* P2G4_MSG_TX_END: end_time is the last us of the packet */
	bs_time_t end = bsr_plat_dev_time_from_phy(op->tx_done.end_time) + 1;

	bsr_plat_phy_synced(end - 1);
	op->evt.status = BSR_STATUS_OK;
	phy_op_ended(op, end);
}

static void rx_end_handle(struct op *op)
{
	bs_time_t end = bsr_plat_dev_time_from_phy(op->rx_done.end_time) + 1;
	uint8_t status;

	bsr_plat_phy_synced(end - 1);

	switch (op->rx_done.status) {
	case P2G4_RXSTATUS_OK:
		status = BSR_STATUS_OK;
		break;
	case P2G4_RXSTATUS_NOSYNC:
		status = BSR_STATUS_NO_SYNC;
		break;
	default:
		status = BSR_STATUS_CRC_ERR;
		break;
	}

	if (status != BSR_STATUS_NO_SYNC) {
		uint16_t size = op->rx_done.packet_size;

		if (size > sizeof(op->pkt)) {
			size = sizeof(op->pkt);
		}
		memcpy(op->pkt, phy_rx_buf, size);
		op->pkt_len = size;

		/* The Phy only reports bit errors, verify the CRC ourselves so
		 * that CRC init or access address mismatches are detected too.
		 */
		if ((status == BSR_STATUS_OK) &&
		    ((size < (BSR_PDU_HEADER_LEN + BSR_CRC_LEN)) ||
		     ((BSR_PDU_HEADER_LEN + op->pkt[1] + BSR_CRC_LEN) != size))) {
			status = BSR_STATUS_CRC_ERR;
		}

		if (status == BSR_STATUS_OK) {
			uint8_t rx_crc[BSR_CRC_LEN];

			/* The CRC computed replaces the received one */
			(void)memcpy(rx_crc, &op->pkt[size - BSR_CRC_LEN], BSR_CRC_LEN);
			append_crc_ble(op->pkt, size - BSR_CRC_LEN, op->cfg.crc_init);
			if (memcmp(rx_crc, &op->pkt[size - BSR_CRC_LEN], BSR_CRC_LEN) != 0) {
				status = BSR_STATUS_CRC_ERR;
			}
		}

		/* As with a real radio, a too long packet is truncated and
		 * reported with a CRC error.
		 */
		if ((size >= BSR_PDU_HEADER_LEN) && (op->pkt[1] > op->cfg.max_len)) {
			status = BSR_STATUS_CRC_ERR;
		}

		if ((status == BSR_STATUS_OK) && cheat_applies(&cheat.rx_fail_crc)) {
			status = BSR_STATUS_CRC_ERR;
		}

		op->evt.rssi = rssi_dbm_get(op->rx_done.rssi.RSSI);
		op->evt.len =
			(size >= BSR_PDU_HEADER_LEN) ? NSI_MIN(op->pkt[1], op->cfg.max_len) : 0U;
	}

	op->evt.status = status;
	phy_op_ended(op, end);
}

static void rx_response_handle(struct op *op, int ret)
{
	bs_time_t recheck;

	if (ret == -1) {
		bsr_plat_phy_disconnected();
		return;
	}

	if (ret == P2G4_MSG_ABORTREEVAL) {
		recheck = bsr_plat_dev_time_from_phy(op->rx_req.abort.recheck_time);
		bsr_plat_phy_synced(recheck);
		phy_wait_set(op, PHY_WAIT_REEVAL, recheck);
		return;
	}

	if (ret == P2G4_MSG_RXV2_ADDRESSFOUND) {
		/* rx_time_stamp is the last us of the access address */
		bs_time_t aa_last = bsr_plat_dev_time_from_phy(op->rx_done.rx_time_stamp);

		bsr_plat_phy_synced(aa_last);
		op->evt.ts_aa_end = (uint32_t)(aa_last + 1);
		op->evt.ts_start = (uint32_t)(aa_last + 1 - pream_and_addr_us(op->cfg.phy));
		phy_wait_set(op, PHY_WAIT_ADDRESS, aa_last);
		return;
	}

	/* P2G4_MSG_RXV2_END */
	rx_end_handle(op);
}

static void tx_start(struct op *op)
{
	p2G4_txv2_t *req = &op->tx_req;
	bs_time_t recheck;
	uint32_t dur;
	uint8_t len;
	int ret;

	/* The PDU is read when the transmission starts, as with DMA */
	len = op->tx_pdu[1];
	memcpy(op->pkt, op->tx_pdu, BSR_PDU_HEADER_LEN + len);

	op->pkt_len = BSR_PDU_HEADER_LEN + len + BSR_CRC_LEN;
	append_crc_ble(op->pkt, BSR_PDU_HEADER_LEN + len, op->cfg.crc_init);

	dur = pream_and_addr_us(op->cfg.phy) + bytes_us(op->cfg.phy, op->pkt_len);

	memset(req, 0, sizeof(*req));
	radio_params_set(&req->radio_params, &op->cfg);
	req->phy_address = op->cfg.aa;
	req->power_level = p2G4_power_from_d(op->cfg.tx_power + cheat.tx_power_offset);
	req->packet_size = op->pkt_len;
	req->coding_rate = 0;
	req->start_tx_time = bsr_plat_phy_time_from_dev(op->start);
	req->start_packet_time = req->start_tx_time;
	req->end_tx_time = req->start_tx_time + dur - 1;
	req->end_packet_time = req->end_tx_time;
	abort_struct_update(op, &req->abort, &recheck);

	op->evt.ts_start = (uint32_t)op->start;
	op->evt.ts_aa_end = (uint32_t)(op->start + pream_and_addr_us(op->cfg.phy));

	if (cheat_applies(&cheat.tx_disabled)) {
		/* Not sent on air, but it ends as if it had been */
		op->evt.status = BSR_STATUS_OK;
		phy_op_ended(op, op->start + dur);
		return;
	}

	op->state = OP_IN_PHY;
	bsr.phy_op = op;

	ret = p2G4_dev_req_txv2_nc_b(req, op->pkt, &op->tx_done);
	tx_response_handle(op, ret);
}

static void rx_start(struct op *op)
{
	p2G4_rxv2_t *req = &op->rx_req;
	bs_time_t recheck;
	int ret;

	memset(req, 0, sizeof(*req));
	radio_params_set(&req->radio_params, &op->cfg);
	req->start_time = bsr_plat_phy_time_from_dev(op->start);
	/* The access address must have ended by start + window_us */
	req->scan_duration = (op->window_us != 0U) ? (op->window_us + 1U) : UINT32_MAX;
	req->forced_packet_duration = UINT32_MAX;
	req->error_calc_rate = (op->cfg.phy == BSR_PHY_2M) ? 2000000U : 1000000U;
	req->antenna_gain = 0;
	req->coding_rate = 0;
	req->pream_and_addr_duration = pream_and_addr_us(op->cfg.phy);
	req->header_duration = bytes_us(op->cfg.phy, BSR_PDU_HEADER_LEN);
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
	op->rx_addr[0] = cheat_applies(&cheat.rx_dont_sync) ? 0xDEADBEAFU : op->cfg.aa;
	abort_struct_update(op, &req->abort, &recheck);

	op->state = OP_IN_PHY;
	bsr.phy_op = op;

	ret = p2G4_dev_req_rxv2_nc_b(req, op->rx_addr, &op->rx_done, &phy_rx_buf,
				     sizeof(phy_rx_buf_mem));
	rx_response_handle(op, ret);
}

static void phy_respond(struct op *op)
{
	enum phy_wait wait = op->phy_wait;
	bs_time_t recheck;
	int ret;

	op->phy_wait = PHY_WAIT_NONE;

	if (wait == PHY_WAIT_ADDRESS) {
		if (op->aborted) {
			(void)p2G4_dev_rxv2_cont_after_addr_nc_b(false, NULL);
			bsr_plat_phy_synced(nsi_hws_get_time());
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
	struct op *op;

	if (bsr.timer_end <= now) {
		bsr.timer_end = TIME_NEVER;

		for (size_t i = 0U; i < NSI_ARRAY_SIZE(bsr.ops); i++) {
			op = &bsr.ops[i];
			if (op->state != OP_WAIT_END) {
				continue;
			}

			struct bsr_evt evt = op->evt;

			if (op->is_rx && (evt.status != BSR_STATUS_NO_SYNC)) {
				memcpy(op->rx_buf, op->pkt, BSR_PDU_HEADER_LEN + evt.len);
			}

			/* Raising the interrupt runs the embedded SW right
			 * away, which may already request the next operation.
			 */
			op_free(op);
			evt_put(&evt);
		}
	}

	if (bsr.timer_start <= now) {
		op = bsr.sw_op;

		if ((op != NULL) && (op->state == OP_WAIT_START)) {
			if (bsr.phy_op != NULL) {
				/* A previous aborted op still holds the Phy. This
				 * can not happen as the Phy is always answered at
				 * or before the next HW event of this device.
				 */
				bs_trace_error_time_line("bs_2g4_radio: Phy busy at op start\n");
			} else {
				bsr.timer_start = TIME_NEVER;
				radio_timer_update();
				if (op->is_rx) {
					rx_start(op);
				} else {
					tx_start(op);
				}
			}
		} else {
			bsr.timer_start = TIME_NEVER;
		}
	}

	radio_timer_update();
}

NSI_HW_EVENT(timer_radio, radio_timer_triggered, 998);

static void phy_timer_triggered(void)
{
	timer_phy = TIME_NEVER;
	nsi_hws_find_next_event();

	if (bsr.phy_op != NULL) {
		phy_respond(bsr.phy_op);
	}
}

NSI_HW_EVENT(timer_phy, phy_timer_triggered, 999);

static void cntr_timer_triggered(void)
{
	/* As a HW compare register, it would match again after a wrap */
	timer_cntr += (1ULL << 32);
	nsi_hws_find_next_event();

	cntr_cmp_evt = true;
	bsr_plat_irq_raise(bsr.cntr_irq);
}

NSI_HW_EVENT(timer_cntr, cntr_timer_triggered, 50);

void bsr_init(unsigned int radio_irq, unsigned int cntr_irq)
{
	memset(&bsr, 0, sizeof(bsr));

	bsr.radio_irq = radio_irq;
	bsr.cntr_irq = cntr_irq;
	bsr.timer_start = TIME_NEVER;
	bsr.timer_end = TIME_NEVER;
	bsr.initialized = true;

	timer_cntr = TIME_NEVER;
	cntr_cmp_evt = false;
	timer_phy = TIME_NEVER;
	radio_timer_update();
}

uint32_t bsr_cntr_get(void)
{
	return (uint32_t)nsi_hws_get_time();
}

void bsr_cntr_cmp_set(uint32_t value)
{
	bs_time_t now = nsi_hws_get_time();
	uint32_t delta = value - (uint32_t)now;

	timer_cntr = now + ((delta != 0U) ? delta : (1ULL << 32));
	nsi_hws_find_next_event();
}

void bsr_cntr_cmp_disable(void)
{
	timer_cntr = TIME_NEVER;
	nsi_hws_find_next_event();
}

bool bsr_cntr_cmp_evt_get_clear(void)
{
	bool evt = cntr_cmp_evt;

	cntr_cmp_evt = false;

	return evt;
}

static struct op *op_request(const struct bsr_pkt_cfg *cfg, uint32_t at, bool is_rx)
{
	bs_time_t start = time_from_cntr(at);
	struct op *op;

	if (!bsr.initialized || (bsr.sw_op != NULL) || (start <= nsi_hws_get_time())) {
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
	bsr.last_id++;
	if (bsr.last_id == 0U) {
		bsr.last_id = 1U;
	}
	op->id = bsr.last_id;

	op->evt.id = op->id;
	op->evt.is_rx = is_rx;

	bsr.sw_op = op;
	bsr.timer_start = start;
	radio_timer_update();

	return op;
}

uint32_t bsr_tx(const struct bsr_pkt_cfg *cfg, uint32_t at, const uint8_t *pdu)
{
	struct op *op = op_request(cfg, at, false);

	if (op == NULL) {
		return 0U;
	}

	op->tx_pdu = pdu;

	return op->id;
}

uint32_t bsr_rx(const struct bsr_pkt_cfg *cfg, uint32_t start, uint32_t window_us,
		uint8_t *buf)
{
	struct op *op = op_request(cfg, start, true);

	if (op == NULL) {
		return 0U;
	}

	op->window_us = window_us;
	op->rx_buf = buf;

	return op->id;
}

void bsr_abort(void)
{
	struct op *op = bsr.sw_op;

	if (op == NULL) {
		return;
	}

	bsr.sw_op = NULL;

	switch (op->state) {
	case OP_WAIT_START:
		bsr.timer_start = TIME_NEVER;
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
		bsr.timer_end = TIME_NEVER;
		op_free(op);
		break;
	default:
		break;
	}

	radio_timer_update();
}

void bsr_testcheat_set_tx_power_gain(double power_offset)
{
	cheat.tx_power_offset = power_offset;
}

void bsr_testcheat_set_rx_power_gain(double power_offset)
{
	cheat.rx_power_offset = power_offset;
}

void bsr_testcheat_disable_tx(int64_t count)
{
	cheat.tx_disabled = count;
}

void bsr_testcheat_disable_rx(int64_t count_dont_sync, int64_t count_fail_crc)
{
	cheat.rx_dont_sync = count_dont_sync;
	cheat.rx_fail_crc = count_fail_crc;
}

bool bsr_evt_get(struct bsr_evt *evt)
{
	if (bsr.evt_count == 0U) {
		return false;
	}

	*evt = bsr.evt_q[bsr.evt_first];
	bsr.evt_first = (bsr.evt_first + 1U) % BSR_EVT_QUEUE_LEN;
	bsr.evt_count--;

	return true;
}
