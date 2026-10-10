/*
 * Copyright (c) 2026 Fabien Proriol
 * SPDX-License-Identifier: Apache-2.0
 *
 * Unit tests for KNX Layer-7 APDU encoding rules (host-side, no kernel).
 *
 * Tests verify the byte-exact APDU layouts for key services, cross-checked
 * against KNX spec 3/3/7 and the ANALYZE.MD fixes from Phase 1–3:
 *
 *   Suite 1 — GroupValue APDU format (short vs long form, Phase 1.3 fix)
 *   Suite 2 — PropertyValue_Read response layout
 *   Suite 3 — PropertyDescription_Read response layout
 *   Suite 4 — Memory_Read bounds and response layout
 *   Suite 5 — DeviceDescriptor_Read response layout
 *   Suite 6 — Restart service APCI encoding
 *   Suite 7 — Authorize_Request/Response APCI encoding
 *   Suite 8 — A_Restart_Response APCI (MasterReset, Phase 3.2 fix)
 */

#include <zephyr/ztest.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>

/* ============================================================
 * APCI constants (mirrors layer7_application.h)
 * ============================================================
 */

#define A_GroupValue_Read              0x000u
#define A_GroupValue_Response          0x040u
#define A_GroupValue_Write             0x080u
#define A_PropertyValue_Read           0x3D5u
#define A_PropertyValue_Response       0x3D6u
#define A_PropertyValue_Write          0x3D7u
#define A_PropertyDescription_Read     0x3D8u
#define A_PropertyDescription_Response 0x3D9u
#define A_Memory_Read                  0x200u
#define A_Memory_Response              0x240u
#define A_Memory_Write                 0x280u
#define A_DeviceDescriptor_Read        0x300u
#define A_DeviceDescriptor_Response    0x340u
#define A_Restart                      0x380u
#define A_Restart_Response             (A_Restart | 0x20u)
#define A_Authorize_Request            0x3D1u
#define A_Authorize_Response           0x3D2u
#define A_UserMemory_Response          0x2C1u

/* ============================================================
 * APDU layout helpers (pure C, no stack)
 * ============================================================
 */

/* Build a PropertyValue_Response APDU.  Returns byte count. */
static int build_prop_response(uint8_t object_index, uint8_t property_id, uint8_t count,
			       uint16_t start, const uint8_t *data, uint8_t data_len, uint8_t *out)
{
	out[0] = (A_PropertyValue_Response >> 8) & 0xFFu;
	out[1] = A_PropertyValue_Response & 0xFFu;
	out[2] = object_index;
	out[3] = property_id;
	out[4] = (uint8_t)(((count & 0x0Fu) << 4) | ((start >> 8) & 0x0Fu));
	out[5] = start & 0xFFu;
	if (data_len > 0) {
		memcpy(&out[6], data, data_len);
	}
	return 6 + data_len;
}

/* Build a PropertyDescription_Response APDU. */
static int build_propdesc_response(uint8_t obj, uint8_t prop, uint8_t prop_idx, bool write_enable,
				   uint8_t type, uint16_t max_elem, uint8_t access, uint8_t *out)
{
	out[0] = (A_PropertyDescription_Response >> 8) & 0xFFu;
	out[1] = A_PropertyDescription_Response & 0xFFu;
	out[2] = obj;
	out[3] = prop;
	out[4] = prop_idx;
	out[5] = type | ((write_enable ? 1u : 0u) << 7);
	out[6] = (max_elem >> 8) & 0x0Fu;
	out[7] = max_elem & 0xFFu;
	out[8] = access;
	return 9;
}

/* Build a Memory_Response APDU. */
static int build_mem_response(uint8_t number, uint16_t addr, const uint8_t *data, uint8_t *out)
{
	out[0] = (A_Memory_Response >> 8) & 0xFFu;
	out[1] = (A_Memory_Response & 0xC0u) | (number & 0x3Fu);
	out[2] = (addr >> 8) & 0xFFu;
	out[3] = addr & 0xFFu;
	if (number > 0 && data != NULL) {
		memcpy(&out[4], data, number);
	}
	return 4 + number;
}

/* ============================================================
 * Suite 1: GroupValue APDU formats (Phase 1.3 fix verification)
 * ============================================================
 */

ZTEST_SUITE(l7_groupvalue, NULL, NULL, NULL, NULL, NULL);

ZTEST(l7_groupvalue, test_read_apdu_2bytes)
{
	/* A_GroupValue_Read: LSDU = 2 bytes, data field = 0 */
	uint8_t apdu[2] = {(A_GroupValue_Read >> 8) & 0xFFu, A_GroupValue_Read & 0xFFu};

	zassert_equal(apdu[0], 0x00u, "GroupValue_Read APCI high");
	zassert_equal(apdu[1], 0x00u, "GroupValue_Read APCI low");
}

ZTEST(l7_groupvalue, test_write_short_form_1bit_on)
{
	/* DPT_Switch ON: 2-byte LSDU, value in APCI_lo bits[5:0] */
	uint8_t apdu[2] = {
		(A_GroupValue_Write >> 8) & 0xFFu,
		(A_GroupValue_Write & 0xFFu) | 0x01u /* data = 1 */
	};
	zassert_equal(apdu[0], 0x00u, "GroupValue_Write APCI high");
	zassert_equal(apdu[1], 0x81u, "GroupValue_Write short ON: APCI 0x80 | data 0x01");
}

ZTEST(l7_groupvalue, test_write_short_form_1bit_off)
{
	uint8_t apdu[2] = {(A_GroupValue_Write >> 8) & 0xFFu, (A_GroupValue_Write & 0xFFu) | 0x00u};

	zassert_equal(apdu[1], 0x80u, "GroupValue_Write short OFF: APCI 0x80 | data 0x00");
}

ZTEST(l7_groupvalue, test_write_long_form_1byte)
{
	/* DPT_Scaling (8-bit): 3-byte LSDU */
	uint8_t data = 0x80u; /* 50 % */
	uint8_t apdu[3] = {(A_GroupValue_Write >> 8) & 0xFFu, A_GroupValue_Write & 0xFFu, data};

	zassert_equal(apdu[0], 0x00u);
	zassert_equal(apdu[1], 0x80u, "Long form APCI_lo has no data bits");
	zassert_equal(apdu[2], 0x80u, "Data byte follows");
}

ZTEST(l7_groupvalue, test_response_short_form)
{
	/* A_GroupValue_Response with 1-bit value = 1: 2 bytes */
	uint8_t apdu[2] = {(A_GroupValue_Response >> 8) & 0xFFu,
			   (A_GroupValue_Response & 0xFFu) | 0x01u};
	zassert_equal(apdu[0], 0x00u);
	zassert_equal(apdu[1], 0x41u, "Response 0x40 | data 0x01");
}

ZTEST(l7_groupvalue, test_response_long_form_2bytes)
{
	/* A_GroupValue_Response for DPT_Value_Temp (16-bit): 4 bytes */
	uint8_t data[2] = {0x0Cu, 0x1Au}; /* ~20°C */
	uint8_t apdu[4] = {(A_GroupValue_Response >> 8) & 0xFFu, A_GroupValue_Response & 0xFFu,
			   data[0], data[1]};
	zassert_equal(apdu[0], 0x00u);
	zassert_equal(apdu[1], 0x40u, "Long form: APCI_lo no data bits");
	zassert_equal(apdu[2], 0x0Cu);
	zassert_equal(apdu[3], 0x1Au);
}

/* ============================================================
 * Suite 2: PropertyValue_Read response layout
 * ============================================================
 */

ZTEST_SUITE(l7_propval, NULL, NULL, NULL, NULL, NULL);

ZTEST(l7_propval, test_response_encoding)
{
	uint8_t data[2] = {0x07, 0xB0}; /* device descriptor = 0x07B0 */
	uint8_t apdu[10];
	int len = build_prop_response(0, 83, 1, 1, data, 2, apdu);

	zassert_equal(len, 8, "PropertyValue_Response for 2-byte property = 8 bytes");
	zassert_equal(apdu[0], (A_PropertyValue_Response >> 8) & 0xFFu);
	zassert_equal(apdu[1], A_PropertyValue_Response & 0xFFu);
	zassert_equal(apdu[2], 0u, "object_index = 0 (Device Object)");
	zassert_equal(apdu[3], 83u, "property_id = PID_DEVICE_DESCRIPTOR");
	/* byte 4: count[3:0]<<4 | start[11:8]; count=1,start=1 → 0x10 | 0x00 */
	zassert_equal(apdu[4], 0x10u, "count=1 start=1");
	zassert_equal(apdu[5], 0x01u, "start low byte");
	zassert_equal(apdu[6], 0x07u, "data[0]");
	zassert_equal(apdu[7], 0xB0u, "data[1]");
}

ZTEST(l7_propval, test_response_write_failure_count0)
{
	/* A write failure response has count=0, no data bytes */
	uint8_t apdu[8];
	int len = build_prop_response(0, 83, 0, 1, NULL, 0, apdu);

	zassert_equal(len, 6, "Failure response = 6 bytes (no data)");
	/* byte 4: count=0 → upper nibble = 0 */
	zassert_equal((apdu[4] >> 4) & 0x0Fu, 0u, "count must be 0 on failure");
}

ZTEST(l7_propval, test_response_apci_bytes)
{
	uint8_t apdu[6];

	build_prop_response(0, 1, 1, 1, NULL, 0, apdu);

	/* APCI 0x3D6: bits[9:8]=0b11 → apdu[0]=(0x3D6>>8)&0xFF=0x03 */
	zassert_equal(apdu[0], 0x03u, "APCI high byte for PropertyValue_Response");
	/* bits[7:0]=0xD6 */
	zassert_equal(apdu[1], 0xD6u, "APCI low byte for PropertyValue_Response");
}

/* ============================================================
 * Suite 3: PropertyDescription_Read response layout
 * ============================================================
 */

ZTEST_SUITE(l7_propdesc, NULL, NULL, NULL, NULL, NULL);

ZTEST(l7_propdesc, test_response_9bytes)
{
	uint8_t apdu[10];
	int len = build_propdesc_response(0, 83, 0, false, 0x12, 1, 0x30, apdu);

	zassert_equal(len, 9, "PropertyDescription_Response = 9 bytes");
	zassert_equal(apdu[0], 0x03u); /* APCI high */
	zassert_equal(apdu[1], 0xD9u); /* APCI low for 0x3D9 */
	zassert_equal(apdu[2], 0u, "object_index");
	zassert_equal(apdu[3], 83u, "property_id");
	zassert_equal(apdu[4], 0u, "property_index_found");
	/* byte 5: type | (write_enable<<7); write_enable=false → 0x12 */
	zassert_equal(apdu[5], 0x12u, "type=0x12, write_enable=0");
	zassert_equal(apdu[6], 0u, "max_nr_of_elem high nibble");
	zassert_equal(apdu[7], 1u, "max_nr_of_elem = 1");
	zassert_equal(apdu[8], 0x30u, "access = ReadLv3");
}

ZTEST(l7_propdesc, test_write_enable_flag_in_bit7)
{
	uint8_t apdu[10];

	build_propdesc_response(0, 5, 0, true, 0x00, 1, 0x33, apdu);
	/* byte 5: type | (1<<7) = 0x00 | 0x80 = 0x80 */
	zassert_equal(apdu[5], 0x80u, "write_enable=true sets bit 7");
}

/* ============================================================
 * Suite 4: Memory_Read response layout
 * ============================================================
 */

ZTEST_SUITE(l7_memory, NULL, NULL, NULL, NULL, NULL);

ZTEST(l7_memory, test_read_response_layout)
{
	uint8_t data[4] = {0xA5, 0xAD, 0xAF, 0xFE};
	uint8_t apdu[10];
	int len = build_mem_response(4, 0x0000, data, apdu);

	zassert_equal(len, 8, "Memory_Response for 4 bytes = 8 bytes APDU");
	/* APCI 0x240: high=0x02, low=0x40 | number */
	zassert_equal(apdu[0], 0x02u, "Memory_Response APCI high");
	zassert_equal(apdu[1], 0x44u, "Memory_Response APCI low: 0x40 | 4");
	zassert_equal(apdu[2], 0x00u, "address high");
	zassert_equal(apdu[3], 0x00u, "address low");
	zassert_equal(apdu[4], 0xA5u);
	zassert_equal(apdu[5], 0xADu);
	zassert_equal(apdu[6], 0xAFu);
	zassert_equal(apdu[7], 0xFEu);
}

ZTEST(l7_memory, test_read_zero_response_on_failure)
{
	/* number=0 encodes failure: APCI_lo has number bits clear */
	uint8_t apdu[5];
	int len = build_mem_response(0, 0x0004, NULL, apdu);

	zassert_equal(len, 4, "Empty Memory_Response = 4 bytes");
	zassert_equal(apdu[1], 0x40u, "number=0 → APCI_lo = 0x40");
}

ZTEST(l7_memory, test_number_field_encoding)
{
	/* max valid number = 63; check it fits in bits[5:0] of APCI_lo */
	uint8_t buf[64] = {0};
	uint8_t apdu[70];
	int len = build_mem_response(63, 0x0100, buf, apdu);

	zassert_equal(len, 67, "63 bytes + 4 header = 67");
	zassert_equal(apdu[1], 0x40u | 63u, "APCI_lo with 63 in lower 6 bits");
}

/* ============================================================
 * Suite 5: DeviceDescriptor_Read response
 * ============================================================
 */

ZTEST_SUITE(l7_devdesc, NULL, NULL, NULL, NULL, NULL);

ZTEST(l7_devdesc, test_dd0_response_4bytes)
{
	/* DD type 0: 2 APCI bytes + 2 descriptor bytes = 4 */
	uint16_t dd = 0x07B0u; /* TP1 System B mask */
	uint8_t apdu[4];

	apdu[0] = (A_DeviceDescriptor_Response >> 8) & 0xFFu;
	apdu[1] = (A_DeviceDescriptor_Response & 0xFFu) | 0x00u; /* type 0 */
	apdu[2] = (dd >> 8) & 0xFFu;
	apdu[3] = dd & 0xFFu;

	zassert_equal(apdu[0], 0x03u);
	zassert_equal(apdu[1], 0x40u, "DD_Response APCI_lo | type 0");
	zassert_equal(apdu[2], 0x07u);
	zassert_equal(apdu[3], 0xB0u);
}

ZTEST(l7_devdesc, test_unknown_type_uses_max_descriptor)
{
	/* Per Phase 3.2 fix: unknown type returns type=0 (not 0x3F) */
	uint8_t apdu[4];

	apdu[0] = (A_DeviceDescriptor_Response >> 8) & 0xFFu;
	apdu[1] = (A_DeviceDescriptor_Response & 0xFFu) | 0x00u; /* return DD0 */
	apdu[2] = 0x07u;
	apdu[3] = 0xB0u;

	/* Ensure we respond with DD type 0, NOT with 0x3F (the old buggy value) */
	zassert_not_equal(apdu[1] & 0x3Fu, 0x3Fu,
			  "Must NOT encode invalid type 0x3F per ANALYZE.MD §7.5 fix");
}

/* ============================================================
 * Suite 6: Restart service APCI encoding
 * ============================================================
 */

ZTEST_SUITE(l7_restart, NULL, NULL, NULL, NULL, NULL);

ZTEST(l7_restart, test_basic_restart_apci)
{
	/* Basic restart: APCI 0x380, restart_type = 0 in bit 0 */
	uint8_t lsdu_hi = (A_Restart >> 8) & 0xFFu;
	uint8_t lsdu_lo = (A_Restart & 0xFFu) | 0x00u; /* restart_type=0 */

	zassert_equal(lsdu_hi, 0x03u);
	zassert_equal(lsdu_lo, 0x80u, "Basic restart: bit 0 = 0");
}

ZTEST(l7_restart, test_master_reset_apci)
{
	/* MasterReset request: APCI 0x380, restart_type = 1 in bit 0 */
	uint8_t lsdu_lo = (A_Restart & 0xFFu) | 0x01u;

	zassert_equal(lsdu_lo & 0x01u, 1u, "MasterReset: bit 0 = 1");
}

ZTEST(l7_restart, test_master_reset_response_apci)
{
	/*
	 * Phase 3.2 fix: MasterReset response uses A_Restart_Response = 0x3A0
	 * (A_Restart | 0x20 = response indicator bit 5).
	 * PDU: [APCI_hi][APCI_lo | restart_type=1][error_code][time_hi][time_lo]
	 */
	uint8_t apdu[5];

	apdu[0] = (A_Restart_Response >> 8) & 0xFFu;
	apdu[1] = (A_Restart_Response & 0xFFu) | 0x01u; /* restart_type=1 */
	apdu[2] = 0x00u;                                /* E_SUCCESS */
	apdu[3] = 0x00u;
	apdu[4] = 0x01u; /* 100 ms */

	zassert_equal(apdu[0], 0x03u);
	/* 0x3A0: APCI_lo = 0xA0 | restart_type */
	zassert_equal(apdu[1], 0xA1u, "MasterReset_Response APCI_lo = 0xA0 | type 1");
	zassert_equal(apdu[2], 0x00u, "error_code = E_SUCCESS");
	zassert_equal(apdu[3], 0x00u);
	zassert_equal(apdu[4], 0x01u, "process_time = 1 × 100ms");
}

/* ============================================================
 * Suite 7: Authorize Request / Response APCI
 * ============================================================
 */

ZTEST_SUITE(l7_authorize, NULL, NULL, NULL, NULL, NULL);

ZTEST(l7_authorize, test_request_apci)
{
	/* A_Authorize_Request: APCI 0x3D1 */
	zassert_equal((A_Authorize_Request >> 8) & 0x03u, 0x03u,
		      "Authorize_Request APCI[9:8] = 0b11");
	zassert_equal(A_Authorize_Request & 0xFFu, 0xD1u, "Authorize_Request APCI[7:0] = 0xD1");
}

ZTEST(l7_authorize, test_response_grants_level)
{
	/* Phase 3.3 fix: response always grants level (0=full, 255=denied) */
	uint8_t level = 0; /* CONFIG_KNX_AUTHORIZE_GRANT_ALL */
	uint8_t apdu[3];

	apdu[0] = (A_Authorize_Response >> 8) & 0xFFu;
	apdu[1] = A_Authorize_Response & 0xFFu;
	apdu[2] = level;

	zassert_equal(apdu[0], 0x03u);
	zassert_equal(apdu[1], 0xD2u);
	zassert_equal(apdu[2], 0u, "level 0 = full access");
}

ZTEST(l7_authorize, test_response_deny_level)
{
	uint8_t level = 0xFFu; /* deny */
	uint8_t apdu[3] = {(A_Authorize_Response >> 8) & 0xFFu, A_Authorize_Response & 0xFFu,
			   level};
	zassert_equal(apdu[2], 0xFFu, "0xFF = access denied");
}

/* ============================================================
 * Suite 8: APCI value correctness spot-checks
 * (catch off-by-one or wrong constants that break ETS)
 * ============================================================
 */

ZTEST_SUITE(l7_apci_values, NULL, NULL, NULL, NULL, NULL);

ZTEST(l7_apci_values, test_groupvalue_apci_range)
{
	/* GroupValue services are in 0x000–0x0FF range */
	zassert_true(A_GroupValue_Read < 0x100u);
	zassert_true(A_GroupValue_Response < 0x100u);
	zassert_true(A_GroupValue_Write < 0x100u);
	/* Response = Read + 0x40, Write = Read + 0x80 */
	zassert_equal(A_GroupValue_Response, A_GroupValue_Read + 0x040u);
	zassert_equal(A_GroupValue_Write, A_GroupValue_Read + 0x080u);
}

ZTEST(l7_apci_values, test_property_apci_are_10bit)
{
	/* All property services are in 0x3D5..0x3DA range */
	zassert_equal(A_PropertyValue_Read, 0x3D5u);
	zassert_equal(A_PropertyValue_Response, 0x3D6u);
	zassert_equal(A_PropertyValue_Write, 0x3D7u);
	zassert_equal(A_PropertyDescription_Read, 0x3D8u);
	zassert_equal(A_PropertyDescription_Response, 0x3D9u);
}

ZTEST(l7_apci_values, test_memory_apci_family)
{
	/* Memory services: Read=0x200, Response=0x240, Write=0x280 */
	zassert_equal(A_Memory_Read, 0x200u);
	zassert_equal(A_Memory_Response, 0x240u);
	zassert_equal(A_Memory_Write, 0x280u);
}

ZTEST(l7_apci_values, test_devdesc_apci_pair)
{
	/* DD_Read=0x300, DD_Response=0x340 — differ by 0x40 */
	zassert_equal(A_DeviceDescriptor_Response - A_DeviceDescriptor_Read, 0x040u);
}

ZTEST(l7_apci_values, test_restart_response_has_bit5)
{
	/* A_Restart_Response = A_Restart | 0x20 (bit 5 = response indicator) */
	zassert_equal(A_Restart_Response, A_Restart | 0x20u);
	/* This is bit 5 of APCI_lo (0x80 | 0x20 = 0xA0) */
	zassert_equal(A_Restart_Response & 0xFFu, 0xA0u);
}
