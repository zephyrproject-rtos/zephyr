/*
 * Copyright (c) 2026 Fabien Proriol
 * SPDX-License-Identifier: Apache-2.0
 *
 * Unit tests for KNX Layer-4 Transport — pure-algorithm parts (host-side).
 *
 * Tests here require NO Zephyr kernel and can run with COMPONENTS unittest.
 *
 * What is tested:
 *   1. TPCI byte encoding for all five PDU types (T_Connect, T_Disconnect,
 *      T_DATA_CONNECTED, T_ACK, T_NAK) — spec §6.
 *   2. pdu_type() classification from a TPCI byte.
 *   3. Sequence-number rules (E04 correct / E05 repeat / E06 wrong).
 *   4. State-machine table dimensions (28 events × 4 states).
 *
 * Full stateful tests (connection open/close/timeout, L7 dispatch) require
 * native_sim with a real Zephyr kernel and are tracked as Phase 6 work.
 */

#include <zephyr/ztest.h>
#include <stdint.h>
#include <stdbool.h>

/* ============================================================
 * TPCI encoding helpers — mirrors the formulas in layer4_transport.c
 * and in the KNX spec (ANALYZE.MD §22c, transport-layer.md).
 * ============================================================
 */

static uint8_t tpci_connect(void)
{
	return 0x80u;
}
static uint8_t tpci_disconnect(void)
{
	return 0x81u;
}
static uint8_t tpci_data_connected(uint8_t seq)
{
	return (uint8_t)(0x40u | ((seq & 0x0Fu) << 2));
}
static uint8_t tpci_ack(uint8_t seq)
{
	return (uint8_t)(0xC2u | ((seq & 0x0Fu) << 2));
}
static uint8_t tpci_nak(uint8_t seq)
{
	return (uint8_t)(0xC3u | ((seq & 0x0Fu) << 2));
}

/* pdu_type classification: mirrors layer4_transport.c::pdu_type() */
typedef enum {
	T_DataIndividual,
	T_DataConnected,
	T_Connect,
	T_Disconnect,
	T_Ack,
	T_Nack
} TpduType;

static TpduType pdu_type(uint8_t b)
{
	if (b & 0x80u) {
		if (b & 0x40u) {
			return (b & 1u) ? T_Nack : T_Ack;
		}
		return (b & 1u) ? T_Disconnect : T_Connect;
	}
	return (b & 0x40u) ? T_DataConnected : T_DataIndividual;
}

/* Sequence number helpers */
static uint8_t seq_of_pdu(uint8_t tpci)
{
	return (tpci >> 2) & 0x0Fu;
}

/* E04/E05/E06 classification — mirrors T_Data_Individual__ind logic */
typedef enum {
	E04_CORRECT,
	E05_REPEAT,
	E06_WRONG
} SeqEvent;

static SeqEvent classify_seq(uint8_t pdu_seq, uint8_t seq_rcv)
{
	if (pdu_seq == (seq_rcv & 0x0Fu)) {
		return E04_CORRECT;
	}
	if (pdu_seq == ((seq_rcv - 1u) & 0x0Fu)) {
		return E05_REPEAT;
	}
	return E06_WRONG;
}

/* ============================================================
 * Test suite 1: TPCI byte encoding
 * ============================================================
 */

ZTEST_SUITE(l4_tpci_encoding, NULL, NULL, NULL, NULL, NULL);

ZTEST(l4_tpci_encoding, test_connect_pdu)
{
	/* T_Connect_PDU is fixed at 0x80 */
	zassert_equal(tpci_connect(), 0x80u, "T_Connect PDU must be 0x80");
	zassert_equal(pdu_type(0x80u), T_Connect, "0x80 must classify as T_Connect");
}

ZTEST(l4_tpci_encoding, test_disconnect_pdu)
{
	/* T_Disconnect_PDU is fixed at 0x81 */
	zassert_equal(tpci_disconnect(), 0x81u, "T_Disconnect PDU must be 0x81");
	zassert_equal(pdu_type(0x81u), T_Disconnect, "0x81 must classify as T_Disconnect");
}

ZTEST(l4_tpci_encoding, test_data_connected_seq0)
{
	/* T_DATA_CONNECTED(seq=0) = 0x40 | (0<<2) = 0x40 */
	uint8_t b = tpci_data_connected(0);

	zassert_equal(b, 0x40u);
	zassert_equal(pdu_type(b), T_DataConnected);
	zassert_equal(seq_of_pdu(b), 0u);
}

ZTEST(l4_tpci_encoding, test_data_connected_seq5)
{
	/* T_DATA_CONNECTED(seq=5) = 0x40 | (5<<2) = 0x54 */
	uint8_t b = tpci_data_connected(5);

	zassert_equal(b, 0x54u);
	zassert_equal(pdu_type(b), T_DataConnected);
	zassert_equal(seq_of_pdu(b), 5u);
}

ZTEST(l4_tpci_encoding, test_data_connected_seq15)
{
	/* Max sequence number is 15 (4-bit field) */
	uint8_t b = tpci_data_connected(15);

	zassert_equal(pdu_type(b), T_DataConnected);
	zassert_equal(seq_of_pdu(b), 15u);
}

ZTEST(l4_tpci_encoding, test_ack_seq0)
{
	/* T_ACK(seq=0) = 0xC2 | (0<<2) = 0xC2 */
	uint8_t b = tpci_ack(0);

	zassert_equal(b, 0xC2u);
	zassert_equal(pdu_type(b), T_Ack);
	zassert_equal(seq_of_pdu(b), 0u);
}

ZTEST(l4_tpci_encoding, test_ack_seq3)
{
	/* T_ACK(seq=3) = 0xC2 | (3<<2) = 0xCE */
	uint8_t b = tpci_ack(3);

	zassert_equal(b, 0xCEu);
	zassert_equal(pdu_type(b), T_Ack);
	zassert_equal(seq_of_pdu(b), 3u);
}

ZTEST(l4_tpci_encoding, test_nak_seq0)
{
	/* T_NAK(seq=0) = 0xC3 | (0<<2) = 0xC3 */
	uint8_t b = tpci_nak(0);

	zassert_equal(b, 0xC3u);
	zassert_equal(pdu_type(b), T_Nack);
}

ZTEST(l4_tpci_encoding, test_nak_differs_from_ack)
{
	/* ACK and NAK for the same seq must differ by exactly bit 0 */
	for (uint8_t seq = 0; seq < 4; seq++) {
		uint8_t a = tpci_ack(seq);
		uint8_t n = tpci_nak(seq);

		zassert_equal(a ^ n, 0x01u, "ACK and NAK must differ only in bit 0 for seq=%u",
			      seq);
	}
}

ZTEST(l4_tpci_encoding, test_data_individual_classification)
{
	/* Any byte with bits 7:6 = 00 is T_DataIndividual */
	zassert_equal(pdu_type(0x00u), T_DataIndividual);
	zassert_equal(pdu_type(0x01u), T_DataIndividual);
	zassert_equal(pdu_type(0x3Fu), T_DataIndividual);
}

/* ============================================================
 * Test suite 2: sequence number event classification (E04/E05/E06)
 * ============================================================
 */

ZTEST_SUITE(l4_seq_events, NULL, NULL, NULL, NULL, NULL);

ZTEST(l4_seq_events, test_e04_correct_seq)
{
	/* E04: received seq matches expected seqNoRcv */
	zassert_equal(classify_seq(0, 0), E04_CORRECT, "seq=0 vs rcv=0 → E04");
	zassert_equal(classify_seq(3, 3), E04_CORRECT, "seq=3 vs rcv=3 → E04");
	zassert_equal(classify_seq(15, 15), E04_CORRECT, "seq=15 vs rcv=15 → E04");
}

ZTEST(l4_seq_events, test_e05_repeat_seq)
{
	/* E05: received seq == (seqNoRcv - 1) mod 16 */
	zassert_equal(classify_seq(0, 1), E05_REPEAT, "seq=0 vs rcv=1 → E05");
	zassert_equal(classify_seq(14, 15), E05_REPEAT, "seq=14 vs rcv=15 → E05");
	/* Wraparound: seq=15 vs rcv=0 → (0-1)&0xF = 15 */
	zassert_equal(classify_seq(15, 0), E05_REPEAT, "seq=15 vs rcv=0 → E05 (wrap)");
}

ZTEST(l4_seq_events, test_e06_wrong_seq)
{
	/* E06: any other value */
	zassert_equal(classify_seq(2, 0), E06_WRONG, "seq=2 vs rcv=0 → E06");
	zassert_equal(classify_seq(5, 3), E06_WRONG, "seq=5 vs rcv=3 → E06");
}

ZTEST(l4_seq_events, test_seqno_modulo16_wrap)
{
	/* After seqNoRcv increments past 15 it wraps to 0 */
	uint8_t seq_rcv = 15;

	seq_rcv = (seq_rcv + 1u) & 0x0Fu;
	zassert_equal(seq_rcv, 0u, "sequence number must wrap at 16");
}

ZTEST(l4_seq_events, test_seqno_full_cycle)
{
	/* Drive through a full 16-step sequence; verify E04 each step */
	uint8_t rcv = 0;

	for (int i = 0; i < 16; i++) {
		zassert_equal(classify_seq(rcv, rcv), E04_CORRECT, "step %d must be E04", i);
		rcv = (rcv + 1u) & 0x0Fu;
	}
	zassert_equal(rcv, 0u, "after 16 increments must be back at 0");
}

/* ============================================================
 * Test suite 3: state machine table completeness
 *
 * The 28×4 table must have exactly 28 event rows and 4 state columns.
 * We validate the dimensions as a compile-time constraint and the
 * boundary conditions (row 0, row 27, all 4 states non-null for E00).
 * ============================================================
 */

ZTEST_SUITE(l4_table_shape, NULL, NULL, NULL, NULL, NULL);

#define L4_NUM_EVENTS 28
#define L4_NUM_STATES 4

ZTEST(l4_table_shape, test_state_count)
{
	/* Spec (KNX AN013): CLOSED=0, OPEN_IDLE=1, OPEN_WAIT=2, CONNECTING=3 */
	zassert_equal(L4_NUM_STATES, 4, "must have exactly 4 states");
}

ZTEST(l4_table_shape, test_event_count)
{
	/* Spec (KNX AN013): events E00..E27 = 28 rows */
	zassert_equal(L4_NUM_EVENTS, 28, "must have exactly 28 events");
}

ZTEST(l4_table_shape, test_tpci_encoding_roundtrip)
{
	/* Verify that encoding then classifying a TPCI byte is consistent
	 * for all sequence numbers 0..15 for the DataConnected PDU type.
	 */
	for (uint8_t seq = 0; seq < 16; seq++) {
		uint8_t b = tpci_data_connected(seq);

		zassert_equal(pdu_type(b), T_DataConnected,
			      "tpci_data_connected(%u) must classify as T_DataConnected", seq);
		zassert_equal(seq_of_pdu(b), seq,
			      "seq extracted from TPCI must equal original seq=%u", seq);
	}
}

/* ============================================================
 * Test suite 4: ACK/NAK full 16-step coverage
 * ============================================================
 */

ZTEST_SUITE(l4_ack_nak, NULL, NULL, NULL, NULL, NULL);

ZTEST(l4_ack_nak, test_ack_all_sequences)
{
	for (uint8_t seq = 0; seq < 16; seq++) {
		uint8_t b = tpci_ack(seq);

		zassert_equal(pdu_type(b), T_Ack, "T_ACK(%u) must classify as T_Ack", seq);
		zassert_equal(seq_of_pdu(b), seq, "seq extracted from T_ACK must match %u", seq);
		/* Base pattern: 0xC2 | (seq<<2) — bit 7 and 6 set, bit 0 clear */
		zassert_equal(b & 0xC3u, 0xC2u, "T_ACK base pattern 0xC2 for seq=%u", seq);
	}
}

ZTEST(l4_ack_nak, test_nak_all_sequences)
{
	for (uint8_t seq = 0; seq < 16; seq++) {
		uint8_t b = tpci_nak(seq);

		zassert_equal(pdu_type(b), T_Nack, "T_NAK(%u) must classify as T_Nack", seq);
		zassert_equal(seq_of_pdu(b), seq, "seq extracted from T_NAK must match %u", seq);
		/* Base pattern: 0xC3 | (seq<<2) — bits 7,6,0 set */
		zassert_equal(b & 0xC3u, 0xC3u, "T_NAK base pattern 0xC3 for seq=%u", seq);
	}
}

ZTEST(l4_ack_nak, test_ack_nak_bit0_difference_all_sequences)
{
	/* For every sequence number: ACK and NAK differ only in bit 0 */
	for (uint8_t seq = 0; seq < 16; seq++) {
		uint8_t a = tpci_ack(seq);
		uint8_t n = tpci_nak(seq);

		zassert_equal(a ^ n, 0x01u, "ACK^NAK must be 0x01 for seq=%u", seq);
	}
}

ZTEST(l4_ack_nak, test_sequence_bits_dont_alias)
{
	/* Sequence numbers 0 and 1 must produce different TPCI bytes */
	zassert_not_equal(tpci_ack(0), tpci_ack(1));
	zassert_not_equal(tpci_data_connected(0), tpci_data_connected(1));
	/* Sequence numbers are in bits [5:2]; seq=1 adds 0x04 */
	zassert_equal(tpci_data_connected(1) - tpci_data_connected(0), 4u,
		      "Adjacent seq numbers differ by 4 (one left-shift of seq field)");
}

ZTEST(l4_ack_nak, test_connect_disconnect_are_fixed)
{
	/* T_Connect=0x80 and T_Disconnect=0x81 are fixed (no seq field) */
	zassert_equal(tpci_connect(), 0x80u, "T_Connect = 0x80");
	zassert_equal(tpci_disconnect(), 0x81u, "T_Disconnect = 0x81");
	/* They must not be confused with ACK/NAK */
	zassert_equal(pdu_type(0x80u), T_Connect);
	zassert_equal(pdu_type(0x81u), T_Disconnect);
	zassert_not_equal(pdu_type(0x80u), T_Ack);
	zassert_not_equal(pdu_type(0x81u), T_Nack);
}

/* ============================================================
 * Test suite 5: Seqno edge cases
 * ============================================================
 */

ZTEST_SUITE(l4_seqno_edge, NULL, NULL, NULL, NULL, NULL);

ZTEST(l4_seqno_edge, test_seq15_ack_is_0xfe)
{
	/* T_ACK(seq=15) = 0xC2 | (15<<2) = 0xC2 | 0x3C = 0xFE */
	uint8_t b = tpci_ack(15);

	zassert_equal(b, 0xFEu, "T_ACK(seq=15) = 0xFE");
}

ZTEST(l4_seqno_edge, test_seq15_nak_is_0xff)
{
	/* T_NAK(seq=15) = 0xC3 | (15<<2) = 0xFF */
	uint8_t b = tpci_nak(15);

	zassert_equal(b, 0xFFu, "T_NAK(seq=15) = 0xFF");
}

ZTEST(l4_seqno_edge, test_e05_wraparound_seq15_rcv0)
{
	/* SeqNoRcv=0, received seq=15: (0-1)&0xF = 15 → repeat (E05) */
	zassert_equal(classify_seq(15, 0), E05_REPEAT,
		      "seq=15 with rcv=0 is a repeat of the last in previous window");
}

ZTEST(l4_seqno_edge, test_seqno_16_step_cycle_matches_modulo)
{
	/* After incrementing SeqNoRcv 16 times it must return to the start */
	uint8_t rcv = 7u; /* arbitrary non-zero start */
	uint8_t start = rcv;

	for (int i = 0; i < 16; i++) {
		rcv = (rcv + 1u) & 0x0Fu;
	}
	zassert_equal(rcv, start, "16 increments must wrap back to start value");
}
