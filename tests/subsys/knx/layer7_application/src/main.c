/*
 * Copyright (c) 2026 Fabien Proriol
 * SPDX-License-Identifier: Apache-2.0
 *
 * layer7_application.c / knx_group_object.c regression tests — the first
 * native_sim harness for either.
 *
 * Closes two gaps left untested by the pure-logic l7_apdu suite (which has
 * no __memory, no real object_group_table.c, and no L4 to capture outbound
 * APDUs against):
 *   - the User Memory family (A_UserMemory_Read/Write__ind,
 *     A_UserMemoryBit_Write__ind, A_UserManufacturerInfo_Read__ind) and the
 *     A_UserMemory_Write__ind Verify Mode fix;
 *   - group_object_read_on_init_trigger() (knx_group_object.c) and its
 *     object_group_table.c hook (object_group_table_properties_written()).
 *
 * Unlike l2_framing/l4_transitions, this suite does not #include its
 * targets: no test case here needs layer7_application.c's or knx_group_
 * object.c's private statics (both are driven entirely through their public
 * API), so all five files (those two, plus object_group_table.c, object_
 * memory.c and dpt.c) are ordinary compiled sources — see CMakeLists.txt.
 * object_group_table.c is real for real Group Object storage — §2.12's gap
 * note names its object_group_table_properties_written() hook explicitly as
 * part of what must be exercised for real. object_memory.c is real too, for
 * genuine __memory offsets/bounds. dpt.c is real and dependency-free
 * (already proven by the `encoding` suite) since knx_group_object.c's typed
 * accessors call its encode/decode functions at link time.
 */

#include <zephyr/ztest.h>
#include <zephyr/kernel.h>
#include <zephyr/knx/knx_pkt.h>
#include <zephyr/knx/dpt.h>
#include <string.h>
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#include "object_interface.h"
#include "object_memory.h"
#include "object_group_table.h"
#include "layer7_application.h"
#include "knx_group_object.h"

/* ============================================================
 * Kernel objects normally provided by knx_core.c, which this test
 * deliberately does not compile (its SYS_INIT would start the whole stack).
 * ============================================================
 */

#define TEST_PKT_POOL_SIZE 8
K_MEM_SLAB_DEFINE(knx_pkt_slab, sizeof(struct knx_pkt), TEST_PKT_POOL_SIZE, 4);

/* layer7_application.c's A_IndividualAddress_Read__ind takes &knx_app_wq —
 * never exercised by this suite's tests, but the symbol must exist for the
 * link. Not started: nothing here calls a function that schedules on it.
 */
struct k_work_q knx_app_wq;

/* ============================================================
 * Captured outbound APDUs — one array per transport entry point
 * (mirrors l4_transitions' capture pattern, one layer up).
 * ============================================================
 */

#define MAX_CAPTURED 8

struct captured_pkt {
	uint8_t apdu[24];
	uint8_t apdu_len;
	uint16_t addr; /* tsap for the group capture, dst otherwise */
	knx_priority_t priority;
};

static struct captured_pkt s_group[MAX_CAPTURED];
static uint8_t s_group_count;
static struct captured_pkt s_connected[MAX_CAPTURED];
static uint8_t s_connected_count;
static struct captured_pkt s_individual[MAX_CAPTURED];
static uint8_t s_individual_count;

static void capture(struct captured_pkt *arr, uint8_t *count, struct knx_pkt *pkt, uint16_t addr)
{
	if (*count < MAX_CAPTURED) {
		struct captured_pkt *c = &arr[*count];
		uint8_t n = pkt->lsdu_len > sizeof(c->apdu) ? sizeof(c->apdu) : pkt->lsdu_len;

		memcpy(c->apdu, pkt->lsdu, n);
		c->apdu_len = pkt->lsdu_len;
		c->addr = addr;
		c->priority = pkt->priority;
		(*count)++;
	}
}

void T_Data_Group__req(struct knx_pkt *pkt)
{
	capture(s_group, &s_group_count, pkt, pkt->tsap);
	knx_pkt_free(pkt);
}

void T_Data_Connected__req(struct knx_pkt *pkt)
{
	capture(s_connected, &s_connected_count, pkt, pkt->dst);
	knx_pkt_free(pkt);
}

void T_Data_Individual__req(struct knx_pkt *pkt)
{
	capture(s_individual, &s_individual_count, pkt, pkt->dst);
	knx_pkt_free(pkt);
}

void T_Data_Broadcast__req(struct knx_pkt *pkt)
{
	knx_pkt_free(pkt);
}

/* ============================================================
 * object_device.c stand-ins — exactly the functions layer7_application.c
 * actually calls (traced via grep, not guessed).
 * ============================================================
 */

static uint16_t s_ia = 0x1101u;
static bool s_prog_mode;

uint16_t device_individual_address(void)
{
	return s_ia;
}
void device_individual_address_set(uint16_t v)
{
	s_ia = v;
}
uint8_t device_routing_count(void)
{
	return 6u;
}
uint16_t device_manufacturer_id(void)
{
	return 0x00FAu;
} /* this device's real id */
uint8_t device_serial_number(int i)
{
	return (uint8_t)(0x10 + i);
}
uint8_t device_hardware_type(int i)
{
	return (uint8_t)i;
}
uint16_t device_version(void)
{
	return 0x0001u;
}
uint16_t device_descriptor(void)
{
	return 0x07B0u;
}
void device_prog_mode_set(bool v)
{
	s_prog_mode = v;
}

/* ============================================================
 * object_interface.c stand-ins — not this suite's target (already covered
 * by the object_interface/l7_apdu suites); fixed/no-op so the symbols
 * resolve for the handlers this suite does not exercise.
 * ============================================================
 */

uint8_t interface_read_property(uint8_t object_index, PropertyID property_id, uint32_t start_index,
				uint32_t *count, uint8_t *data, uint8_t max_size)
{
	ARG_UNUSED(object_index);
	ARG_UNUSED(property_id);
	ARG_UNUSED(start_index);
	ARG_UNUSED(data);
	ARG_UNUSED(max_size);
	*count = 0;
	return 0;
}

bool interface_write_property(uint8_t object_index, PropertyID property_id, uint16_t start_index,
			      uint8_t number_of_elements, const uint8_t *data, uint8_t data_length)
{
	ARG_UNUSED(object_index);
	ARG_UNUSED(property_id);
	ARG_UNUSED(start_index);
	ARG_UNUSED(number_of_elements);
	ARG_UNUSED(data);
	ARG_UNUSED(data_length);
	return false;
}

void interface_read_property_description(uint8_t object_index, uint8_t property_id,
					 uint8_t property_index, bool *write_enable,
					 uint8_t *property_datatype, uint16_t *max_nr_of_elem,
					 uint8_t *access, uint8_t *property_index_found)
{
	ARG_UNUSED(object_index);
	ARG_UNUSED(property_id);
	ARG_UNUSED(property_index);
	ARG_UNUSED(write_enable);
	ARG_UNUSED(property_datatype);
	ARG_UNUSED(max_nr_of_elem);
	ARG_UNUSED(access);
	*property_index_found = 0xFFu;
}

uint8_t interface_function_property_command(uint8_t object_index, uint8_t property_id,
					    const uint8_t *data_in, uint8_t data_in_len,
					    uint8_t *data_out, uint8_t max_out)
{
	ARG_UNUSED(object_index);
	ARG_UNUSED(property_id);
	ARG_UNUSED(data_in);
	ARG_UNUSED(data_in_len);
	ARG_UNUSED(data_out);
	ARG_UNUSED(max_out);
	return 0;
}

uint8_t interface_function_property_state_read(uint8_t object_index, uint8_t property_id,
					       const uint8_t *data_in, uint8_t data_in_len,
					       uint8_t *data_out, uint8_t max_out)
{
	ARG_UNUSED(object_index);
	ARG_UNUSED(property_id);
	ARG_UNUSED(data_in);
	ARG_UNUSED(data_in_len);
	ARG_UNUSED(data_out);
	ARG_UNUSED(max_out);
	return 0;
}

void interface_unload_object(uint8_t object_index)
{
	ARG_UNUSED(object_index);
}

void interface_set_access_level(uint8_t level)
{
	ARG_UNUSED(level);
}

/* ============================================================
 * object_association_table.c stand-in — test-controlled ASAP -> TSAP map.
 * ============================================================
 */

#define TEST_MAX_ASAP 4 /* matches CONFIG_KNX_GROUP_OBJECT_COUNT in CMakeLists.txt */

static int32_t s_tsap_for_asap[TEST_MAX_ASAP + 1];

int32_t association_table_translate_asap(uint16_t asap)
{
	if (asap >= ARRAY_SIZE(s_tsap_for_asap)) {
		return -1;
	}
	return s_tsap_for_asap[asap];
}

/* ============================================================
 * Test helpers
 * ============================================================
 */

static void reset(void)
{
	memory_reset();
	group_object_data_reset(); /* RAM-only, not part of __memory */
	s_group_count = s_connected_count = s_individual_count = 0;
	memset(s_tsap_for_asap, 0xFF, sizeof(s_tsap_for_asap)); /* -1: no TSAP bound */
}

static struct knx_pkt *build_pkt(bool connectionless, knx_priority_t priority)
{
	struct knx_pkt *pkt = knx_pkt_alloc(K_NO_WAIT);

	zassert_not_null(pkt, "packet slab exhausted — a previous case leaked");
	pkt->lsdu = pkt->buf;
	pkt->src = 0x1234u;
	pkt->dst = 0x0000u;
	pkt->priority = priority;
	pkt->connectionless = connectionless;
	return pkt;
}

/* type code 8 => group_object_size() returns 2 (an ordinary 2-octet GO). */
static void setup_group_object(uint16_t asap, uint8_t flags, int32_t tsap)
{
	__memory.group_object[asap] = htons((uint16_t)(((uint16_t)flags << 8) | 8u));
	s_tsap_for_asap[asap] = tsap;
}

/*
 * ASAP 1: I+C set, TSAP bound      -> must fire.
 * ASAP 2: C set, I clear           -> must NOT fire (no read-on-init).
 * ASAP 3: I set, C clear           -> must NOT fire (Communication disabled).
 * ASAP 4: I+C set, no TSAP bound   -> must NOT fire (nothing to send to).
 */
static void setup_four_asaps(void)
{
	__memory.group_object[0] = htons(TEST_MAX_ASAP);
	setup_group_object(1, GROUP_OBJECT_FLAG_I | GROUP_OBJECT_FLAG_C, 101);
	setup_group_object(2, GROUP_OBJECT_FLAG_C, 102);
	setup_group_object(3, GROUP_OBJECT_FLAG_I, 103);
	setup_group_object(4, GROUP_OBJECT_FLAG_I | GROUP_OBJECT_FLAG_C, -1);
}

/* ============================================================
 * Suite 1: A_UserMemory_Read/Write, A_UserMemoryBit_Write,
 * A_UserManufacturerInfo_Read.
 * ============================================================
 */

ZTEST_SUITE(l7_user_memory, NULL, NULL, NULL, NULL, NULL);

ZTEST(l7_user_memory, test_user_memory_read_returns_requested_octets)
{
	struct knx_pkt *pkt;

	reset();
	pkt = build_pkt(false, KNX_PRIORITY_LOW);
	pkt->lsdu[2] = 4u; /* number=4, address high nibble 0 */
	pkt->lsdu[3] = 0u;
	pkt->lsdu[4] = 0u;
	pkt->lsdu_len = 5u;

	A_UserMemory_Read__ind(pkt);
	knx_pkt_free(pkt); /* __ind borrows pkt — L4 would free it, but we bypass L4 here */

	zassert_equal(s_connected_count, 1);
	zassert_equal(s_connected[0].apdu_len, 9, "5 header octets + 4 data octets");
	zassert_equal(s_connected[0].apdu[2] & 0x0Fu, 4u, "echoed number must be 4");
	/* __memory.start[] holds the magic header after memory_reset(). */
	zassert_equal(s_connected[0].apdu[5], 0xA5u);
	zassert_equal(s_connected[0].apdu[6], 0xADu);
	zassert_equal(s_connected[0].apdu[7], 0xAFu);
	zassert_equal(s_connected[0].apdu[8], 0xFEu);
}

ZTEST(l7_user_memory, test_user_memory_read_refuses_out_of_bounds_count)
{
	struct knx_pkt *pkt;

	reset();
	pkt = build_pkt(false, KNX_PRIORITY_LOW);
	pkt->lsdu[2] = 0u; /* number == 0: always invalid */
	pkt->lsdu[3] = 0u;
	pkt->lsdu[4] = 0u;
	pkt->lsdu_len = 5u;

	A_UserMemory_Read__ind(pkt);
	knx_pkt_free(pkt);

	zassert_equal(s_connected_count, 1, "a read is always answered, even a refused one");
	zassert_equal(s_connected[0].apdu[2] & 0x0Fu, 0u,
		      "an invalid request must reply with number=0 (the error IS the zero count)");
}

ZTEST(l7_user_memory, test_user_memory_bit_write_sets_and_clears_without_disturbing_neighbours)
{
	uint16_t addr = (uint16_t)offsetof(struct Memory, device_control);
	uint8_t before_prev, before_next;
	struct knx_pkt *pkt;

	reset();
	__memory.device_control = 0xB4u; /* 1011 0100 */
	before_prev = __memory.start[3];
	before_next = __memory.routing_count;

	pkt = build_pkt(false, KNX_PRIORITY_LOW);
	pkt->lsdu[2] = 1u; /* number of octets */
	pkt->lsdu[3] = (uint8_t)(addr >> 8);
	pkt->lsdu[4] = (uint8_t)addr;
	pkt->lsdu[5] = 0xFFu; /* and_data: keep every bit */
	pkt->lsdu[6] = 0x01u; /* xor_data: flip bit 0 */
	pkt->lsdu_len = 7u;

	A_UserMemoryBit_Write__ind(pkt);
	knx_pkt_free(pkt);

	zassert_equal(__memory.device_control, 0xB5u, "bit 0 must flip, every other bit unchanged");
	zassert_equal(__memory.start[3], before_prev, "the byte before must be untouched");
	zassert_equal(__memory.routing_count, before_next, "the byte after must be untouched");

	pkt = build_pkt(false, KNX_PRIORITY_LOW);
	pkt->lsdu[2] = 1u;
	pkt->lsdu[3] = (uint8_t)(addr >> 8);
	pkt->lsdu[4] = (uint8_t)addr;
	pkt->lsdu[5] = 0xFEu; /* and_data: clear bit 0 */
	pkt->lsdu[6] = 0x00u; /* xor_data: none */
	pkt->lsdu_len = 7u;

	A_UserMemoryBit_Write__ind(pkt);
	knx_pkt_free(pkt);

	zassert_equal(__memory.device_control, 0xB4u,
		      "bit 0 must clear back to its original value");
}

ZTEST(l7_user_memory, test_user_memory_bit_write_refuses_number_out_of_range)
{
	uint16_t addr = (uint16_t)offsetof(struct Memory, device_control);
	uint8_t canary = 0x78u; /* bit 2 (0x04, Verify Mode) deliberately clear */
	struct knx_pkt *pkt;

	reset();
	__memory.device_control = canary;

	pkt = build_pkt(false, KNX_PRIORITY_LOW);
	pkt->lsdu[2] = 0u; /* number == 0: refused outright, no side effect */
	pkt->lsdu[3] = (uint8_t)(addr >> 8);
	pkt->lsdu[4] = (uint8_t)addr;
	pkt->lsdu_len = 5u;

	A_UserMemoryBit_Write__ind(pkt);
	knx_pkt_free(pkt);

	zassert_equal(__memory.device_control, canary, "number=0 must not write anything");
	zassert_equal(s_connected_count, 0,
		      "Verify Mode is off by default, so an ignored request gets no reply either");
}

ZTEST(l7_user_memory, test_user_manufacturer_info_read_reports_truncated_manufacturer_id)
{
	struct knx_pkt *pkt;

	reset();
	pkt = build_pkt(false, KNX_PRIORITY_LOW);
	pkt->lsdu_len = 2u;

	A_UserManufacturerInfo_Read__ind(pkt);
	knx_pkt_free(pkt);

	zassert_equal(s_connected_count, 1);
	zassert_equal(s_connected[0].apdu_len, 5);
	zassert_equal(s_connected[0].apdu[2], 0xFAu, "device_manufacturer_id() & 0xFF");
	zassert_equal(s_connected[0].apdu[3], 0);
	zassert_equal(s_connected[0].apdu[4], 0);
}

/*
 * The Verify Mode fix: with Verify Mode inactive, 03_03_07
 * §3.5.6.3 says the remote process does not respond at the application
 * layer at all — only the transport-layer ack, which this suite never sees
 * (it calls the __ind handler directly). Before the fix, A_UserMemory_Write
 * __ind sent an A_UserMemory_Response unconditionally; reverting the
 * `if (!verify_mode) return;` guards below must make this test fail.
 */
ZTEST(l7_user_memory, test_user_memory_write_verify_mode_off_sends_no_response)
{
	struct knx_pkt *pkt;

	reset(); /* device_control == 0 -> Verify Mode off */
	pkt = build_pkt(false, KNX_PRIORITY_LOW);
	pkt->lsdu[2] = 4u; /* number=4, address 0 (the magic header — safe to overwrite) */
	pkt->lsdu[3] = 0u;
	pkt->lsdu[4] = 0u;
	pkt->lsdu[5] = 0x11u;
	pkt->lsdu[6] = 0x22u;
	pkt->lsdu[7] = 0x33u;
	pkt->lsdu[8] = 0x44u;
	pkt->lsdu_len = 9u;

	A_UserMemory_Write__ind(pkt);
	knx_pkt_free(pkt);

	zassert_equal(__memory.start[0], 0x11u,
		      "the write must still apply even with Verify Mode off");
	zassert_equal(__memory.start[1], 0x22u);
	zassert_equal(__memory.start[2], 0x33u);
	zassert_equal(__memory.start[3], 0x44u);
	zassert_equal(s_connected_count, 0,
		      "Verify Mode off must send NO response — the fix this test guards");
}

ZTEST(l7_user_memory, test_user_memory_write_verify_mode_on_echoes_written_data)
{
	struct knx_pkt *pkt;

	reset();
	__memory.device_control = 0x04u; /* Verify Mode on */
	pkt = build_pkt(false, KNX_PRIORITY_LOW);
	pkt->lsdu[2] = 2u;
	pkt->lsdu[3] = 0u;
	pkt->lsdu[4] = 0u;
	pkt->lsdu[5] = 0x99u;
	pkt->lsdu[6] = 0x88u;
	pkt->lsdu_len = 7u;

	A_UserMemory_Write__ind(pkt);
	knx_pkt_free(pkt);

	zassert_equal(s_connected_count, 1, "Verify Mode on must echo exactly one response");
	zassert_equal(s_connected[0].apdu_len, 7);
	zassert_equal(s_connected[0].apdu[5], 0x99u);
	zassert_equal(s_connected[0].apdu[6], 0x88u);
}

ZTEST(l7_user_memory, test_user_memory_write_failure_with_verify_mode_on_replies_with_zero_count)
{
	uint32_t bad_addr = (uint32_t)sizeof(__memory); /* one past the end: out of range */
	struct knx_pkt *pkt;

	reset();
	__memory.device_control = 0x04u; /* Verify Mode on */
	pkt = build_pkt(false, KNX_PRIORITY_LOW);
	pkt->lsdu[2] = 1u;
	pkt->lsdu[3] = (uint8_t)(bad_addr >> 8);
	pkt->lsdu[4] = (uint8_t)bad_addr;
	pkt->lsdu[5] = 0xAAu;
	pkt->lsdu_len = 6u;

	A_UserMemory_Write__ind(pkt);
	knx_pkt_free(pkt);

	zassert_equal(s_connected_count, 1, "Verify Mode on must reply even to a failed write");
	zassert_equal(s_connected[0].apdu[2] & 0x0Fu, 0u, "a failed write must report number=0");
}

/* ============================================================
 * Suite 2: group_object_read_on_init_trigger() and its
 * object_group_table_properties_written() hook.
 * ============================================================
 */

ZTEST_SUITE(l7_group_read_on_init, NULL, NULL, NULL, NULL, NULL);

ZTEST(l7_group_read_on_init, test_fires_only_for_i_and_c_flagged_asaps)
{
	reset();
	__memory.load_state[OBJ_IDX_GROUP_OBJ_TABLE] = LS_LOADED;
	setup_four_asaps();

	group_object_read_on_init_trigger();

	zassert_equal(s_group_count, 1, "exactly one ASAP (1, I+C both set) must fire a read");
	zassert_equal(s_group[0].addr, 101, "the read must go to ASAP 1's bound TSAP");
	zassert_equal(s_group[0].apdu_len, 2);
	zassert_equal(s_group[0].apdu[0] & 0x03u, 0u, "A_GroupValue_Read APCI high bits must be 0");
	zassert_equal(s_group[0].apdu[1], 0u, "A_GroupValue_Read APCI low byte must be 0");
}

ZTEST(l7_group_read_on_init, test_skips_asap_with_no_bound_tsap)
{
	reset();
	__memory.load_state[OBJ_IDX_GROUP_OBJ_TABLE] = LS_LOADED;
	__memory.group_object[0] = htons(1);
	setup_group_object(1, GROUP_OBJECT_FLAG_I | GROUP_OBJECT_FLAG_C, -1);

	group_object_read_on_init_trigger();

	zassert_equal(s_group_count, 0, "an I+C ASAP with no bound TSAP must not fire a read");
}

ZTEST(l7_group_read_on_init, test_is_a_noop_when_table_not_loaded)
{
	reset();
	__memory.load_state[OBJ_IDX_GROUP_OBJ_TABLE] = LS_UNLOADED;
	setup_four_asaps();

	group_object_read_on_init_trigger();

	zassert_equal(s_group_count, 0,
		      "read-on-init must do nothing while the table isn't Loaded");
}

ZTEST(l7_group_read_on_init, test_group_table_written_only_fires_when_loaded)
{
	reset();
	__memory.load_state[OBJ_IDX_GROUP_OBJ_TABLE] = LS_LOADING;
	setup_four_asaps();

	object_group_table_properties_written(PID_LOAD_STATE_CONTROL);
	zassert_equal(s_group_count, 0,
		      "the hook must not fire before the table reaches LS_LOADED");

	__memory.load_state[OBJ_IDX_GROUP_OBJ_TABLE] = LS_LOADED;
	object_group_table_properties_written(PID_LOAD_STATE_CONTROL);

	zassert_equal(s_group_count, 1,
		      "reaching LS_LOADED must trigger the same I+C-filtered read-on-init sweep");
	zassert_equal(s_group[0].addr, 101);
}

/* Scaffolding: cheap, reuses the same harness. knx_group_object_write() is
 * the OTHER direction (application -> bus), gated by C+T rather than I+C.
 */
ZTEST(l7_group_read_on_init, test_knx_group_object_write_sends_a_groupvalue_write)
{
	bool ok;

	reset();
	__memory.load_state[OBJ_IDX_GROUP_OBJ_TABLE] = LS_LOADED;
	__memory.group_object[0] = htons(1);
	setup_group_object(1, GROUP_OBJECT_FLAG_C | GROUP_OBJECT_FLAG_T, 55);
	knx_group_object_set_u16(1, 0x1234u);

	ok = knx_group_object_write(1);

	zassert_true(ok);
	zassert_equal(s_group_count, 1);
	zassert_equal(s_group[0].addr, 55);
	zassert_equal(s_group[0].apdu_len, 4, "2 APCI octets + 2 data octets");
	zassert_equal(s_group[0].apdu[1], 0x80u, "A_GroupValue_Write APCI low byte");
	zassert_equal(dpt7_decode(&s_group[0].apdu[2]), 0x1234u);
}
