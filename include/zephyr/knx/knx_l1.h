/*
 * Copyright (c) 2026 Fabien Proriol
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief KNX Physical Layer (L1) binding — public interface.
 *
 * This header is the contract between the KNX L1 driver (e.g. NCN5130) and
 * the KNX L2 subsystem.  Both sides include only this file; neither reaches
 * into the other's source directory.
 *
 * Contents:
 *   - Ph_Data_Req_Class / Ph_Data_Ind_Class / P_Status enums
 *   - FRAME_ACK / NACK / BUSY / NAK_BUSY constants
 *   - Ph_Data__req / __con / __ind primitives
 *   - Ph_Reset__req / __con
 *   - Ph_Bus_Free, Ph_Bus_L_Data_con_wait
 *   - U_SetAddress__req
 *   - knx_l1_send_frame (burst-TX function for NCN5130)
 */

#ifndef ZEPHYR_INCLUDE_KNX_L1_H_
#define ZEPHYR_INCLUDE_KNX_L1_H_

#include <stdint.h>
#include <zephyr/kernel.h>

/* ============================================================
 * KNX Physical Layer enums (KNX spec 3/3/1)
 * ============================================================
 */

/** @brief Byte class carried by a Ph_Data__req (L2 → L1 TX). */
typedef enum {
	Req_start_of_Frame,   /**< First byte of a frame */
	Req_inner_Frame_char, /**< A non-first, non-last byte of a frame */
	Req_ack_char,         /**< Bus ACK/NAK/BUSY octet */
	Req_poll_data_char,   /**< Polling-mode data octet (unused, no poll master) */
	Req_fill_char,        /**< Padding byte (unused in burst TX) */
	Req_last_Frame_char   /**< Last byte of a frame */
} Ph_Data_Req_Class;

/** @brief Byte class carried by a Ph_Data__ind (L1 → L2 RX). */
typedef enum {
	Ind_start_of_Frame,   /**< First byte of a frame */
	Ind_inner_Frame_char, /**< A non-first, non-last byte of a frame */
	Ind_end_of_Frame,     /**< Silence-based end-of-frame detection fired */
	Ind_ack_char,         /**< Bus ACK/NAK/BUSY octet received */
	Ind_poll_data_char,   /**< Polling-mode data octet (unused, no poll master) */
	Ind_parity_error,     /**< 9th-bit even parity mismatch on this octet */
	Ind_framing_error,    /**< UART framing error on this octet */
	Ind_bit_error         /**< TX echo did not match what was sent (bus collision) */
} Ph_Data_Ind_Class;

/** @brief Physical-layer result code returned by Ph_Reset__con and friends. */
typedef enum {
	p_ok = 0,                 /**< Success */
	p_transceiver_fault = 1,  /**< NCN5130 reported a transceiver fault */
	p_collision_detected = 2, /**< Bus collision detected during TX */
	P_bus_not_free = 3,       /**< Bus was not free when TX was attempted */
	p_error = 4,              /**< Generic/other error */
} P_Status;

/* ============================================================
 * Bus-level ACK frame values (KNX spec 3/2/2 §3.3)
 * ============================================================
 */

#define FRAME_ACK      0xCCu /**< Positive acknowledge */
#define FRAME_NACK     0x0Cu /**< Negative acknowledge */
#define FRAME_BUSY     0xC0u /**< Receiver busy */
#define FRAME_NAK_BUSY 0x00u /**< Negative acknowledge and busy */

/* ============================================================
 * Ph_Data service (L1 primitive interface)
 * ============================================================
 */

/** Send one byte to the NCN5130 (old byte-by-byte interface, kept for
 *  Req_ack_char; burst TX uses knx_l1_send_frame instead).
 */
void Ph_Data__req(Ph_Data_Req_Class p_class, uint8_t p_data);

/** L1 → L2 per-byte UART completion callback (from driver). */
void Ph_Data__con(P_Status p_status);

/** L1 → L2 receive indication (from driver ISR / EOF timer). */
void Ph_Data__ind(Ph_Data_Ind_Class p_class, uint8_t p_data);

/* ============================================================
 * Ph_Reset service
 * ============================================================
 */

/** Trigger NCN5130 reset (U_Reset.req). */
void Ph_Reset__req(void);

/** NCN5130 has entered Normal state; send U_SetAddress and init L2. */
void Ph_Reset__con(P_Status p_status);

/* ============================================================
 * Miscellaneous L1 services
 * ============================================================
 */

/** Force bus state to FREE (called after successful TX in old byte loop). */
void Ph_Bus_Free(void);

/**
 * Block until NCN5130 sends L_Data.con (0x0B = NACK, 0x8B = positive ACK)
 * or the timeout expires.  Call from L2 TX thread after the last frame byte.
 *
 * @param timeout  k_timeout_t (e.g. K_MSEC(50))
 * @return p_ok, p_error (negative L_Data.con — no device ACKed),
 *         p_collision_detected (genuine NCN5130 tx_err/rx_err), or
 *         p_transceiver_fault on timeout.
 */
P_Status Ph_Bus_L_Data_con_wait(k_timeout_t timeout);

/** Program the NCN5130's auto-ACK address. */
void U_SetAddress__req(unsigned char addr_low, unsigned char addr_high);

/* ============================================================
 * Burst TX (replaces Req_start/inner/last_Frame_char in Phase 1.5)
 * ============================================================
 */

/**
 * Send a complete KNX wire frame to the NCN5130 as a single DMA burst.
 *
 * @param frame     Wire bytes: CTRL, SA, DA, AT|HC|LG, TPDU..., FCS
 *                  (caller has already appended the FCS byte at frame[len-1]).
 * @param len       Total wire byte count (header + LSDU + FCS).
 *                  Range: 7..263 (standard 7..24, extended 25..263 — KNX
 *                  spec 3/2/2: LG field max = 254, 255 is a reserved escape
 *                  code, so 9 + 254 = 263 is the true extended-frame max).
 * @return 0 on positive bus ACK, -EBUSY (bus not free), -EMSGSIZE (len out of range),
 *         -ECONNREFUSED (negative L_Data.con — no device ACKed, not a collision),
 *         -EIO (genuine NCN5130-reported collision/transceiver error),
 *         -ETIMEDOUT (no L_Data.con at all).
 */
int knx_l1_send_frame(const uint8_t *frame, uint16_t len);

#endif /* ZEPHYR_INCLUDE_KNX_L1_H_ */
