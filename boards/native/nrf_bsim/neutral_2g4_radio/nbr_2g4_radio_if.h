/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Silicon Laboratories Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * There are no vendor radio registers, so that the same Link Layer code runs
 * on any BabbleSim board: the embedded software requests the Tx or Rx of a
 * packet at an absolute time, and an interrupt signals its end.
 *
 * All times are the simulation time of the device in microseconds, which
 * the counter of nbr_cntr_if.h counts, so that the embedded software needs
 * no other time base.
 *
 * The runner and the embedded image share these structures, hence the fixed
 * size types only.
 */

#ifndef BOARDS_NATIVE_NRF_BSIM_NEUTRAL_2G4_RADIO_NBR_2G4_RADIO_IF_H
#define BOARDS_NATIVE_NRF_BSIM_NEUTRAL_2G4_RADIO_NBR_2G4_RADIO_IF_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Supported PHYs */
#define NBR_PHY_1M 1U
#define NBR_PHY_2M 2U

/* Packet configuration, common to Tx and Rx */
struct nbr_pkt_cfg {
	uint32_t aa;       /* Access address */
	uint32_t crc_init; /* CRC24 initial value */
	uint16_t freq_mhz; /* Center frequency */
	uint8_t phy;       /* NBR_PHY_* */
	int8_t tx_power;   /* dBm, Tx only */
	uint8_t max_len;   /* Maximum payload length accepted on Rx */
};

/* nbr_op_result.status values */
#define NBR_STATUS_OK      0U /* Packet sent, or received with valid CRC */
#define NBR_STATUS_CRC_ERR 1U /* Packet received, but with errors */
#define NBR_STATUS_NO_SYNC 2U /* Rx ended without finding a packet */
#define NBR_STATUS_ABORTED 3U /* Never reported by the model, for its user to mark an abort */

/* Result of an operation that ended, read with nbr_op_result_get() */
struct nbr_op_result {
	uint32_t id;        /* Id returned by nbr_tx()/nbr_rx() */
	uint8_t status;     /* NBR_STATUS_* */
	uint64_t ts_end;    /* The us after the last us of the operation */
	/* Set by the model only for an Rx that received an access address */
	uint64_t ts_start;  /* Time of the first preamble bit */
	uint64_t ts_aa_end; /* The us after the last access address bit */
	double rssi;        /* dBm */
};

/*
 * Initialize the model, giving the interrupt line used to signal the end of a
 * radio operation.
 */
void nbr_init(unsigned int irq);

/*
 * Transmit a PDU (2 byte header followed by the payload) with its first
 * preamble bit at time 'at'.
 * The model reads the PDU when the transmission starts, hence pdu must
 * stay valid until then, and changes made to it before then are sent. The
 * length of the packet is parsed from the packet content itself. The pdu
 * is not expected to have the CRC. A MIC, if any, is part of the payload:
 * the embedded software appends it, and the model sends it as any other
 * payload byte.
 *
 * Returns an operation id, or 0 if 'at' is not in the future. Requesting an
 * operation while another one is pending, or before the result of the last
 * one has been read, is an error.
 */
uint32_t nbr_tx(const struct nbr_pkt_cfg *cfg, uint64_t at, const uint8_t *pdu);

/*
 * Receive a PDU into buf, listening from time 'start'. If window_us is not 0,
 * the Rx ends with NBR_STATUS_NO_SYNC unless an access address has ended
 * within window_us from start. If window_us is 0, the Rx continues until
 * aborted.
 *
 * buf must have room for a 2 byte header and cfg->max_len payload bytes. It
 * is only written when the packet has ended, and as long as any packet
 * with the right address was sync'ed irrespective of header or CRC errors.
 * A packet with a payload longer than cfg->max_len ends with
 * NBR_STATUS_CRC_ERR: buf then has the header as received, with a length above
 * cfg->max_len, and the first cfg->max_len payload bytes.
 * A MIC, if any, is received as part of the payload. The model does not check
 * it, that is up to the embedded software.
 *
 * Returns an operation id, or 0 if 'start' is not in the future, with the
 * same rules as nbr_tx().
 */
uint32_t nbr_rx(const struct nbr_pkt_cfg *cfg, uint64_t start, uint32_t window_us,
		uint8_t *buf);

/*
 * Abort the pending operation, if any, also when its result has not been
 * read yet. No result is given for it.
 */
void nbr_abort(void);

/*
 * Get the result of the operation that ended, which frees the operation.
 * Returns false if there is none, or it has already been read.
 */
bool nbr_op_result_get(struct nbr_op_result *res);

#ifdef __cplusplus
}
#endif

#endif /* BOARDS_NATIVE_NRF_BSIM_NEUTRAL_2G4_RADIO_NBR_2G4_RADIO_IF_H */
