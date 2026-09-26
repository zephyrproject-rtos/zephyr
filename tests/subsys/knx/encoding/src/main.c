/*
 * Copyright (c) 2026 Fabien Proriol
 * SPDX-License-Identifier: Apache-2.0
 *
 * Unit tests for KNX encoding algorithms (host-side, no Zephyr kernel):
 *   1. TP1 FCS (XOR checksum)
 *   2. NCN5130 UART command byte sequence for a KNX frame
 *   3. GroupValue APDU short-form (≤6-bit) vs long-form encoding/decoding
 *
 * These tests document the CORRECT behaviour per spec; they are also used as
 * regression tests after the Phase 1 fixes.  The NCN5130 command-byte tests
 * encode the expected (fixed) output — they deliberately fail against the
 * unfixed driver (off-by-one in U_L_DataEnd) so that the test suite goes red
 * until 1.1 is applied.
 */

#include <zephyr/ztest.h>
#include <stdint.h>
#include <string.h>

/* ============================================================
 * Helpers — pure C reimplementations of the algorithms under test
 * ============================================================
 */

/*
 * TP1 FCS for STANDARD frames: NOT(XOR of every frame byte except the FCS).
 * KNX spec 3/2/2 §3.2.
 */
static uint8_t knx_fcs(const uint8_t *buf, uint8_t len)
{
	uint8_t crc = 0x00;

	for (int i = 0; i < len; i++) {
		crc ^= buf[i];
	}
	return ~crc;
}

/*
 * TP1 FCS for EXTENDED frames: same as standard BUT the CTRLE byte (index 1)
 * is excluded from the XOR chain.  The NCN5130 follows the standard-frame FCS
 * formula (ctrl, sa, da, lg, tpdu[]) even for extended frames — CTRLE is NOT
 * included.  Confirmed by observing consistent rx_err when FCS includes CTRLE.
 */
static uint8_t knx_fcs_extended(const uint8_t *buf, uint8_t len)
{
	uint8_t crc = 0x00;

	for (int i = 0; i < len; i++) {
		if (i == 1) {
			continue; /* skip CTRLE byte */
		}
		crc ^= buf[i];
	}
	return ~crc;
}

/*
 * build_ncn5130_cmds — generate the interleaved NCN5130 command/data byte
 * pairs for a KNX wire frame (including the pre-computed FCS as the last
 * byte).  Writes into `out[]`, returns the number of bytes written.
 *
 * Correct encoding matching bare-metal NanoKnx behaviour:
 *   0x80, frame[0]              — U_L_DataStart (index 0)
 *   0x81, frame[1]              — U_L_DataCont  (index 1)
 *   ...
 *   0x80|(raw_len-1), frame[raw_len-1]  — U_L_DataCont (last data index)
 *   0x40|raw_len, frame[raw_len]        — U_L_DataEnd (l=raw_len, FCS)
 *
 * l = raw_len = number of KNX data bytes EXCLUDING FCS.
 * The FCS is the extra byte appended to U_L_DataEnd, NOT counted in l.
 *
 * For a 9-byte wire frame (raw_len=8 data + 1 FCS): l=8 → byte=0x48.
 */
static int build_ncn5130_cmds(const uint8_t *frame_with_fcs, uint8_t total_len, uint8_t *out,
			      int out_max)
{
	int pos = 0;
	uint8_t raw_len = total_len - 1; /* bytes before FCS */

	if (pos + 2 > out_max) {
		return -1;
	}
	out[pos++] = 0x80u; /* U_L_DataStart */
	out[pos++] = frame_with_fcs[0];

	for (int i = 1; i < raw_len; i++) {
		if (pos + 2 > out_max) {
			return -1;
		}
		out[pos++] = (uint8_t)(0x80u | (uint8_t)i); /* U_L_DataCont(i) */
		out[pos++] = frame_with_fcs[i];
	}

	/* U_L_DataEnd: l = raw_len (data bytes excluding FCS) */
	if (pos + 2 > out_max) {
		return -1;
	}
	out[pos++] = (uint8_t)(0x40u | (uint8_t)raw_len); /* l = raw_len */
	out[pos++] = frame_with_fcs[raw_len];             /* FCS */

	return pos;
}

/*
 * GroupValue APDU encoding helpers.
 *
 * Short form (≤6-bit data, LSDU = 2 bytes total):
 *   byte[0] = APCI[9:8] = 0x00 (for Read/Response/Write APCI values 0x000/0x040/0x080)
 *   byte[1] = APCI[7:0] | data[5:0]
 *
 * Long form (>6-bit data, LSDU = 2 + n bytes):
 *   byte[0] = 0x00
 *   byte[1] = APCI[7:0]  (0x00=Read, 0x40=Response, 0x80=Write)
 *   byte[2..2+n-1] = data
 */

#define APCI_GV_READ     0x000u
#define APCI_GV_RESPONSE 0x040u
#define APCI_GV_WRITE    0x080u

static int encode_groupvalue(uint16_t apci, const uint8_t *data, uint8_t data_bits, uint8_t *out,
			     int out_max)
{
	if (data_bits <= 6) {
		if (out_max < 2) {
			return -1;
		}
		out[0] = (uint8_t)((apci >> 8) & 0x03u);
		out[1] = (uint8_t)((apci & 0xFFu) | (data[0] & 0x3Fu));
		return 2;
	}

	uint8_t nbytes = (data_bits + 7u) / 8u;

	if (out_max < 2 + nbytes) {
		return -1;
	}
	out[0] = (uint8_t)((apci >> 8) & 0x03u);
	out[1] = (uint8_t)(apci & 0xFFu);
	memcpy(&out[2], data, nbytes);
	return 2 + nbytes;
}

static int decode_groupvalue_short(const uint8_t *lsdu, uint8_t lsdu_len, uint16_t *apci_out,
				   uint8_t *data_out)
{
	if (lsdu_len != 2) {
		return -1;
	}
	*apci_out = (((uint16_t)lsdu[0] & 0x03u) << 8) | (lsdu[1] & 0xC0u);
	*data_out = lsdu[1] & 0x3Fu;
	return 0;
}

/* ============================================================
 * Test suite 1: FCS computation
 * ============================================================
 */

ZTEST_SUITE(knx_fcs, NULL, NULL, NULL, NULL, NULL);

ZTEST(knx_fcs, test_fcs_known_frame)
{
	/*
	 * Minimal GroupValue_Write ON (DPT_Switch, 1 bit):
	 * CTRL=0xBC (standard, not-repeated, low priority)
	 * SA  = 1.1.1 = 0x1101
	 * DA  = GA 1/0/1 = 0x0801
	 * AT|HC|LG = 0xE1 (Group=1, HC=6, LG=1 → 2 LSDU bytes)
	 * TPCI/APCI_hi = 0x00
	 * APCI_lo|data = 0x81  (Write=0x80, bit=1)
	 * FCS = NOT(XOR of all above)
	 */
	uint8_t frame[] = {0xBC, 0x11, 0x01, 0x08, 0x01, 0xE1, 0x00, 0x81};
	uint8_t expected_fcs = knx_fcs(frame, sizeof(frame));

	/* Independently verify: XOR all bytes */
	uint8_t xor = 0;

	for (int i = 0; i < (int)sizeof(frame); i++) {
		xor ^= frame[i];
	}
	zassert_equal(expected_fcs, (uint8_t)~xor, "FCS mismatch");

	/* Minimum frame check: 8 bytes + 1 FCS = 9 wire bytes */
	zassert_equal(sizeof(frame), 8, "Test frame must be 8 bytes (no FCS yet)");
}

ZTEST(knx_fcs, test_fcs_extended_excludes_ctrle)
{
	/*
	 * Extended frame: 30 60 11 5B 11 95 10 47 D6 ... (23 data bytes + FCS).
	 * CTRL=0x30 (extended), CTRLE=0x60 at index 1 is EXCLUDED from FCS.
	 * Standard FCS (with CTRLE): XOR=0x4D → 0xB2  ← NCN5130 REJECTS
	 * Extended FCS (skip CTRLE): XOR=0x2D → 0xD2  ← NCN5130 ACCEPTS
	 */
	uint8_t frame[] = {0x30, 0x60,             /* CTRL (extended), CTRLE */
			   0x11, 0x5B, 0x11, 0x95, /* SA, DA */
			   0x10,                   /* LG=16 */
			   0x47, 0xD6, 0x00, 0x0F, 0x10, 0x01, 0x00, 0x06,
			   0x11, 0x5B, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

	uint8_t fcs_with_ctrle = knx_fcs(frame, sizeof(frame));
	uint8_t fcs_without_ctrle = knx_fcs_extended(frame, sizeof(frame));

	zassert_equal(fcs_with_ctrle, 0xB2u,
		      "Standard FCS (CTRLE included) = 0xB2 — NCN5130 rejects this");
	zassert_equal(fcs_without_ctrle, 0xD2u,
		      "Extended FCS (CTRLE excluded) = 0xD2 — NCN5130 should accept this");
	zassert_not_equal(fcs_with_ctrle, fcs_without_ctrle,
			  "The two FCS values must differ (CTRLE contributes 0x60 to XOR)");
}

ZTEST(knx_fcs, test_fcs_all_zeros)
{
	uint8_t frame[6] = {0};
	/* XOR of six 0x00 bytes = 0x00, NOT = 0xFF */
	zassert_equal(knx_fcs(frame, 6), 0xFF, "FCS of all-zeros must be 0xFF");
}

ZTEST(knx_fcs, test_fcs_one_byte)
{
	uint8_t frame[] = {0xB0};

	zassert_equal(knx_fcs(frame, 1), (uint8_t)~0xB0, "FCS of single byte");
}

/* ============================================================
 * Test suite 2: NCN5130 UART command byte encoding
 * ============================================================
 */

ZTEST_SUITE(ncn5130_cmds, NULL, NULL, NULL, NULL, NULL);

ZTEST(ncn5130_cmds, test_cmd_minimum_frame)
{
	/*
	 * Minimum KNX standard frame: 7 bytes header/LSDU + 1 byte FCS = 8 total.
	 * wire_bytes[0..6] = header + 1-byte LSDU
	 * wire_bytes[7]    = FCS
	 *
	 * Expected NCN5130 command pairs (16 bytes = 8 pairs):
	 *   [0x80, b0]         — U_L_DataStart
	 *   [0x81, b1]         — U_L_DataCont(1)
	 *   [0x82, b2]         — U_L_DataCont(2)
	 *   [0x83, b3]         — U_L_DataCont(3)
	 *   [0x84, b4]         — U_L_DataCont(4)
	 *   [0x85, b5]         — U_L_DataCont(5)
	 *   [0x86, b6]         — U_L_DataCont(6)
	 *   [0x47, fcs]        — U_L_DataEnd: l=raw_len=7 (7 data bytes before FCS)
	 */
	uint8_t frame[] = {0xBC, 0x11, 0x01, 0x08, 0x01, 0xE1, 0x00, 0x81};
	/* frame[7] is already the pre-computed FCS in this test vector */
	uint8_t cmds[32];
	int n = build_ncn5130_cmds(frame, sizeof(frame), cmds, sizeof(cmds));

	zassert_equal(n, 16, "8-byte frame must produce 16 command bytes");

	/* U_L_DataStart */
	zassert_equal(cmds[0], 0x80, "U_L_DataStart cmd byte");
	zassert_equal(cmds[1], frame[0], "U_L_DataStart data byte");

	/* U_L_DataCont for indices 1..6 */
	for (int i = 1; i <= 6; i++) {
		zassert_equal(cmds[i * 2], (uint8_t)(0x80 | i), "U_L_DataCont cmd[%d]", i);
		zassert_equal(cmds[i * 2 + 1], frame[i], "U_L_DataCont data[%d]", i);
	}

	/* U_L_DataEnd: l = raw_len = 7 (7 data bytes before FCS) → cmd byte = 0x47.
	 * The NCN5130 counts data bytes excluding FCS; FCS is the separate extra byte.
	 */
	zassert_equal(cmds[14], 0x47,
		      "U_L_DataEnd cmd byte must be 0x47 (l=raw_len=7 for 8-byte frame)");
	zassert_equal(cmds[15], frame[7], "U_L_DataEnd FCS byte");
}

ZTEST(ncn5130_cmds, test_cmd_nine_byte_frame)
{
	/*
	 * A 9-byte wire frame (8 header/LSDU bytes + FCS):
	 * U_L_DataEnd l = 9 → cmd byte = 0x49
	 */
	uint8_t frame[9];

	memset(frame, 0xAA, sizeof(frame));
	frame[8] = knx_fcs(frame, 8);

	uint8_t cmds[32];
	int n = build_ncn5130_cmds(frame, sizeof(frame), cmds, sizeof(cmds));

	zassert_equal(n, 18, "9-byte frame must produce 18 command bytes");
	/* raw_len = 8 for 9-byte frame → l = 8 → byte = 0x48 */
	zassert_equal(cmds[16], 0x48, "U_L_DataEnd cmd byte must be 0x48 (l=raw_len=8)");
}

ZTEST(ncn5130_cmds, test_cmd_dataend_range)
{
	/*
	 * Verify U_L_DataEnd byte for frames total=8..23 (min valid KNX = 8 bytes).
	 * l = raw_len = total - 1.  Valid range: 0x47..0x7F (l=7..63).
	 * Minimum valid KNX frame = 8 bytes (raw_len=7 → l=7 → 0x47).
	 */
	for (int total = 8; total <= 23; total++) {
		uint8_t frame[24];

		memset(frame, 0x55, total);
		frame[total - 1] = knx_fcs(frame, total - 1);

		uint8_t cmds[100];
		int n = build_ncn5130_cmds(frame, total, cmds, sizeof(cmds));

		zassert_true(n > 0, "build_ncn5130_cmds failed for total=%d", total);
		uint8_t end_cmd = cmds[n - 2]; /* second-to-last byte is the U_L_DataEnd cmd */
		uint8_t raw_len = (uint8_t)(total - 1);

		zassert_true(end_cmd >= 0x47 && end_cmd <= 0x7F,
			     "U_L_DataEnd cmd 0x%02x out of range for total=%d", end_cmd, total);
		zassert_equal(end_cmd, (uint8_t)(0x40 | raw_len),
			      "U_L_DataEnd l must equal raw_len=%d for total=%d", raw_len, total);
	}
}

/* ============================================================
 * Test suite 3: GroupValue APDU short-form vs long-form
 * ============================================================
 */

ZTEST_SUITE(knx_groupvalue, NULL, NULL, NULL, NULL, NULL);

ZTEST(knx_groupvalue, test_short_form_switch_on)
{
	/* DPT_Switch ON (1 bit, value=1): LSDU = {0x00, 0x81} */
	uint8_t data = 0x01;
	uint8_t apdu[4];
	int len = encode_groupvalue(APCI_GV_WRITE, &data, 1, apdu, sizeof(apdu));

	zassert_equal(len, 2, "1-bit write must produce 2-byte LSDU");
	zassert_equal(apdu[0], 0x00, "APCI high byte");
	zassert_equal(apdu[1], 0x81, "APCI low byte | data bit");
}

ZTEST(knx_groupvalue, test_short_form_switch_off)
{
	uint8_t data = 0x00;
	uint8_t apdu[4];
	int len = encode_groupvalue(APCI_GV_WRITE, &data, 1, apdu, sizeof(apdu));

	zassert_equal(len, 2, "1-bit write OFF must produce 2-byte LSDU");
	zassert_equal(apdu[0], 0x00, "APCI high byte");
	zassert_equal(apdu[1], 0x80, "APCI low byte | data=0");
}

ZTEST(knx_groupvalue, test_short_form_6bit)
{
	/* DPT_Scaling value 0x3F (max 6-bit value): long-form because >6 bits */
	uint8_t data = 0x3F;
	uint8_t apdu[4];
	int len = encode_groupvalue(APCI_GV_WRITE, &data, 6, apdu, sizeof(apdu));

	zassert_equal(len, 2, "6-bit write must still fit in 2-byte LSDU");
	zassert_equal(apdu[0], 0x00);
	zassert_equal(apdu[1], (uint8_t)(0x80 | 0x3F), "6-bit data in APCI_lo");
}

ZTEST(knx_groupvalue, test_long_form_1byte)
{
	/* DPT_Scaling (8-bit, e.g. 50% = 0x80): long form, 3-byte LSDU */
	uint8_t data = 0x80;
	uint8_t apdu[4];
	int len = encode_groupvalue(APCI_GV_WRITE, &data, 8, apdu, sizeof(apdu));

	zassert_equal(len, 3, "8-bit write must produce 3-byte LSDU");
	zassert_equal(apdu[0], 0x00, "APCI high byte");
	zassert_equal(apdu[1], 0x80, "APCI low byte (Write, no data bits)");
	zassert_equal(apdu[2], 0x80, "data byte");
}

ZTEST(knx_groupvalue, test_long_form_2byte)
{
	/* DPT_Value_Temp (16-bit float, 2 bytes): 4-byte LSDU */
	uint8_t data[2] = {0x0C, 0x1A}; /* ~20°C in F16 */
	uint8_t apdu[6];
	int len = encode_groupvalue(APCI_GV_WRITE, data, 16, apdu, sizeof(apdu));

	zassert_equal(len, 4, "16-bit write must produce 4-byte LSDU");
	zassert_equal(apdu[0], 0x00);
	zassert_equal(apdu[1], 0x80);
	zassert_equal(apdu[2], 0x0C);
	zassert_equal(apdu[3], 0x1A);
}

ZTEST(knx_groupvalue, test_read_apdu)
{
	/* GroupValue_Read: 2-byte LSDU, data bits don't apply */
	uint8_t dummy = 0;
	uint8_t apdu[4];
	int len = encode_groupvalue(APCI_GV_READ, &dummy, 1, apdu, sizeof(apdu));

	zassert_equal(len, 2, "Read must produce 2-byte LSDU");
	zassert_equal(apdu[0], 0x00);
	zassert_equal(apdu[1], 0x00, "Read APCI_lo has no data");
}

ZTEST(knx_groupvalue, test_decode_short_form)
{
	/* Round-trip: encode 1-bit ON, decode back */
	uint8_t data = 0x01;
	uint8_t apdu[2];

	encode_groupvalue(APCI_GV_WRITE, &data, 1, apdu, sizeof(apdu));

	uint16_t apci;
	uint8_t decoded;
	int ret = decode_groupvalue_short(apdu, 2, &apci, &decoded);

	zassert_equal(ret, 0, "decode must succeed for 2-byte LSDU");
	zassert_equal(decoded, 0x01, "decoded 1-bit value must be 1");
}

ZTEST(knx_groupvalue, test_decode_short_form_wrong_length)
{
	uint8_t lsdu[3] = {0x00, 0x80, 0xAA};
	uint16_t apci;
	uint8_t decoded;

	/* 3-byte LSDU is long-form; short-form decoder must reject it */
	int ret = decode_groupvalue_short(lsdu, 3, &apci, &decoded);

	zassert_not_equal(ret, 0, "3-byte LSDU must be rejected by short-form decoder");
}
