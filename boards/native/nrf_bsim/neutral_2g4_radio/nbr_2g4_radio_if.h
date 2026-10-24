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
	uint8_t phy;       /* NBR_PHY_* */
	uint8_t chan;      /* Bluetooth LE channel index, 0..39 */
	int8_t tx_power;   /* dBm, Tx only */
	uint8_t max_len;   /* Maximum payload length accepted on Rx */
};

/* nbr_evt.status values */
#define NBR_STATUS_OK      0U /* Packet sent, or received with valid CRC */
#define NBR_STATUS_CRC_ERR 1U /* Packet received, but with errors */
#define NBR_STATUS_NO_SYNC 2U /* Rx ended without finding a packet */
#define NBR_STATUS_ABORTED 3U /* Operation aborted with nbr_abort() */

struct nbr_evt {
	uint32_t id;        /* Id returned by nbr_tx()/nbr_rx() */
	uint64_t ts_start;  /* Time of the first preamble bit */
	uint64_t ts_aa_end; /* Time at which the access address ended */
	uint64_t ts_end;    /* Time at which the operation ended */
	uint8_t status;     /* NBR_STATUS_* */
	uint8_t is_rx;
	uint8_t len;        /* Received payload length (PDU header excluded) */
	int8_t rssi;        /* dBm, Rx only */
};

/*
 * Initialize the model, giving the interrupt line used to signal the end of a
 * radio operation.
 */
void nbr_init(unsigned int irq);

/*
 * Transmit a PDU (2 byte header followed by the payload, without CRC) with
 * its first preamble bit at time 'at'. As a radio reading the packet with
 * DMA, the model reads the PDU when the transmission starts, hence pdu must
 * stay valid until then, and changes made to it before then are sent. The
 * model appends the CRC.
 *
 * Returns an operation id, or 0 if the request could not be accepted (an
 * operation is already pending, or 'at' is not in the future).
 */
uint32_t nbr_tx(const struct nbr_pkt_cfg *cfg, uint64_t at, const uint8_t *pdu);

/*
 * Receive a PDU into buf, listening from time 'start'. If window_us is not 0,
 * the Rx ends with NBR_STATUS_NO_SYNC unless an access address has ended
 * within window_us from start. If window_us is 0, the Rx continues until
 * aborted.
 *
 * buf must have room for a 2 byte header and cfg->max_len payload bytes. It
 * is only written when the packet has ended.
 *
 * Returns an operation id, or 0 if the request could not be accepted.
 */
uint32_t nbr_rx(const struct nbr_pkt_cfg *cfg, uint64_t start, uint32_t window_us,
		uint8_t *buf);

/*
 * Abort the pending operation, if any. No event will be generated for it.
 */
void nbr_abort(void);

/* Get the next radio event, returning false if there is none */
bool nbr_evt_get(struct nbr_evt *evt);

#ifdef __cplusplus
}
#endif

#endif /* BOARDS_NATIVE_NRF_BSIM_NEUTRAL_2G4_RADIO_NBR_2G4_RADIO_IF_H */
