/*
 * Copyright (c) 2026 Fabien Proriol
 * SPDX-License-Identifier: Apache-2.0
 *
 * Load State Machine (object_interface.c's load_state_machine_event()) and
 * Run State Machine (object_application_program.c's / object_interface_
 * program.c's auto Halted -> Running coupling) regression tests.
 *
 * `run_state_machine` could be a separate suite, but the actual
 * Run State Machine is a 4-line coupling reachable only through the same
 * Load State harness this suite already needs — see the `rs_coupling` ZTEST_
 * SUITE below, which is that suite folded in rather than duplicated (user
 * decision, 2026-08-26). References: `08_TSSG` (load), `08_TSSI` (run),
 * 03_05_01 §4.24.2.3.4 (Load<->Run coupling).
 *
 * Unlike l2_framing/l4_transitions, this suite never #includes its targets:
 * every case drives the state machines through their real public API
 * (load_state_machine_event(), interface_write_property(),
 * interface_read_property(), object_{application,interface}_program_
 * properties_written()), so the five table-object files plus object_memory.c
 * and object_interface.c are ordinary compiled sources (see CMakeLists.txt).
 * That also means genuine struct Memory offsets/capacities, not a
 * unit_testing-style KNX_TEST_INTERFACE_TABLES stand-in — the whole point of
 * §2.4b's PID_TABLE_REFERENCE and §1.3's allocation-clamping tests.
 */

#include <zephyr/ztest.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

#include "object_interface.h"
#include "object_memory.h"
#include "object_application_program.h"
#include "object_interface_program.h"

/* ============================================================
 * Device Object stand-in.
 *
 * object_interface.c's object_interfaces[]/properties_written[] tables
 * reference these two symbols by name for OBJ_IDX_DEVICE. Neither is ever
 * exercised here: no object under test is the Device Object, and
 * load_state_machine_event() is never called with OBJ_IDX_DEVICE (it has no
 * PID_LOAD_STATE_CONTROL). A one-entry terminator is enough — pulling in the
 * real object_device.c would drag in GPIO/hwinfo/input drivers this host
 * suite has no board for.
 * ============================================================
 */

struct property object_device_properties[] = {{.description = {.property_id = PID_LAST}}};

void object_device_properties_written(PropertyID id)
{
	ARG_UNUSED(id);
}

/* ============================================================
 * object_group_table.c's live-download hook calls this (declared in
 * object_group_table.h, implemented in knx_group_object.c — deliberately not
 * part of this suite, see the layer7_application suite). Record that it
 * fired; its real logic is not this suite's target.
 * ============================================================
 */

static uint8_t s_read_on_init_calls;

void group_object_read_on_init_trigger(void)
{
	s_read_on_init_calls++;
}

/* ============================================================
 * Test helpers
 * ============================================================
 */

/* RS_HALTED/RS_RUNNING are private #defines in object_application_program.c
 * and object_interface_program.c (both reuse LoadState's storage width for a
 * value that is not itself a LoadState). Duplicated here with the same
 * values for the same reason those two files duplicate them from each other.
 */
#define TEST_RS_HALTED  0
#define TEST_RS_RUNNING 1

/* Mutable PID_TABLE_REFERENCE mirrors, one per real table object, indexed by
 * OBJ_IDX_*. NULL for OBJ_IDX_DEVICE, which has none.
 */
static uint32_t *const k_table_ref[NUMBER_OF_INTERFACE] = {
	NULL,
	&table_reference_addresses,
	&table_reference_associations,
	&table_reference_group_object,
	&table_reference_application_program,
	&table_reference_interface_program,
};

static void reset(void)
{
	memory_reset();
	s_read_on_init_calls = 0;
}

/* Builds a valid (>= 8 octet) LE_ADDITIONAL_LOAD_CONTROLS / Data Relative
 * Allocation payload (KNX spec 3/5/2 §3.31.3.4).
 */
static void set_alc(uint8_t *buf, uint32_t size, bool fill, uint8_t fill_byte)
{
	buf[0] = LE_ADDITIONAL_LOAD_CONTROLS;
	buf[1] = 0x0B;
	buf[2] = (uint8_t)(size >> 24);
	buf[3] = (uint8_t)(size >> 16);
	buf[4] = (uint8_t)(size >> 8);
	buf[5] = (uint8_t)size;
	buf[6] = fill ? 0x01 : 0x00;
	buf[7] = fill_byte;
}

/* Reads PID_ERROR_CODE via the real property path. *exists is false for an
 * object with no such property (the Interfaceprogram Object, by design —
 * Volume 6 Annex A.2.7 has no row for it at 07B0h).
 */
static ErrorCode read_error_code(uint8_t object_index, bool *exists)
{
	uint32_t count = 1;
	uint8_t data[4] = {0};
	uint8_t n = interface_read_property(object_index, PID_ERROR_CODE, 1, &count, data,
					    sizeof(data));

	*exists = (n > 0);
	return (ErrorCode)data[0];
}

/* Reads PID_RUN_STATE_CONTROL via the real property path (both Application
 * Program objects keep _run_state as a private static, so this is the only
 * way in from a test).
 */
static uint8_t read_run_state(uint8_t object_index)
{
	uint32_t count = 1;
	uint8_t data[1] = {0xFFu};

	interface_read_property(object_index, PID_RUN_STATE_CONTROL, 1, &count, data, sizeof(data));
	return data[0];
}

static void write_load_event(uint8_t object_index, const uint8_t *data, uint8_t len, bool expect_ok)
{
	bool ok = interface_write_property(object_index, PID_LOAD_STATE_CONTROL, 0, 1, data, len);

	zassert_equal(ok, expect_ok,
		      "obj=%u: interface_write_property(PID_LOAD_STATE_CONTROL) "
		      "returned %d, expected %d",
		      object_index, ok, expect_ok);
}

/* ============================================================
 * Suite 1: the full 5-event x 4-state sweep (mirrors l4_transitions'
 * 28x4 table sweep).
 * ============================================================
 */

ZTEST_SUITE(ls_sweep, NULL, NULL, NULL, NULL, NULL);

/*
 * Expected next state, from object_interface.c's load_state_machine_event()
 * switch, indexed [event][[LoadState value]. LS_UNLOADING/LS_LOADCOMPLETING
 * are never reachable (this implementation resolves them synchronously) and
 * are not part of the table.
 *
 * The LE_ADDITIONAL_LOAD_CONTROLS row uses a valid (sub=0x0B, size=0,
 * fill=false) payload, so the cell reflects the switch's own behaviour, not
 * the "PDU too short" refusal (covered separately by the ls_truncated suite).
 */
static const int8_t k_next_state[5][4] = {
	/*                       LS_UNLOADED  LS_LOADED   LS_LOADING  LS_ERROR */
	/* LE_NOOP                */ {LS_UNLOADED, LS_LOADED, LS_LOADING, LS_ERROR},
	/* LE_START_LOADING       */ {LS_LOADING, LS_LOADING, LS_LOADING, LS_ERROR},
	/* LE_LOAD_COMPLETED      */ {LS_UNLOADED, LS_LOADED, LS_LOADED, LS_ERROR},
	/* LE_ADDITIONAL_LOAD_CTL */ {LS_UNLOADED, LS_ERROR, LS_LOADING, LS_ERROR},
	/* LE_UNLOAD              */ {LS_UNLOADED, LS_UNLOADED, LS_UNLOADED, LS_UNLOADED},
};

static const uint8_t k_sweep_objects[] = {
	OBJ_IDX_ADDRESS_TABLE,    OBJ_IDX_ASSOCIATION_TABLE, OBJ_IDX_GROUP_OBJ_TABLE,
	OBJ_IDX_APPLICATION_PROG, OBJ_IDX_INTERFACE_PROG,
};

static const uint8_t k_sweep_events[] = {
	LE_NOOP, LE_START_LOADING, LE_LOAD_COMPLETED, LE_ADDITIONAL_LOAD_CONTROLS, LE_UNLOAD,
};

static const LoadState k_sweep_states[] = {LS_UNLOADED, LS_LOADED, LS_LOADING, LS_ERROR};

ZTEST(ls_sweep, test_all_20_cells_land_in_the_documented_state)
{
	for (size_t oi = 0; oi < ARRAY_SIZE(k_sweep_objects); oi++) {
		uint8_t idx = k_sweep_objects[oi];

		for (size_t ei = 0; ei < ARRAY_SIZE(k_sweep_events); ei++) {
			for (size_t si = 0; si < ARRAY_SIZE(k_sweep_states); si++) {
				uint8_t buf[8];
				uint8_t len;
				LoadState start = k_sweep_states[si];
				LoadState expect = (LoadState)k_next_state[ei][start];

				reset();
				__memory.load_state[idx] = start;

				if (k_sweep_events[ei] == LE_ADDITIONAL_LOAD_CONTROLS) {
					set_alc(buf, 0, false, 0);
					len = 8;
				} else {
					buf[0] = k_sweep_events[ei];
					len = 1;
				}

				bool ok = load_state_machine_event(idx, buf, len);

				zassert_true(ok, "obj=%u event=%u from state=%u: event refused",
					     idx, k_sweep_events[ei], start);
				zassert_equal(__memory.load_state[idx], expect,
					      "obj=%u event=%u from state=%u: expected %u, got %u",
					      idx, k_sweep_events[ei], start, expect,
					      __memory.load_state[idx]);
			}
		}
	}
}

/*
 * Error-code invariant, checked generically over the same sweep:
 *   - a cell that newly lands in LS_ERROR must read E_GOT_UNDEF_LOAD_CMD
 *     (the only such cell in this event set: LS_LOADED + Additional Load
 *     Controls, an event that is never expected once already Loaded);
 *   - a cell that does NOT end in LS_ERROR must read E_NO_FAULT — the
 *     unconditional clear at the end of load_state_machine_event().
 * A cell that starts AND ends in LS_ERROR is deliberately excluded: whether
 * the error code is touched there depends on how the object arrived at
 * LS_ERROR before this call, not on this cell alone.
 */
ZTEST(ls_sweep, test_error_code_matches_the_landing_or_clears_on_recovery)
{
	for (size_t oi = 0; oi < ARRAY_SIZE(k_sweep_objects); oi++) {
		uint8_t idx = k_sweep_objects[oi];

		for (size_t ei = 0; ei < ARRAY_SIZE(k_sweep_events); ei++) {
			for (size_t si = 0; si < ARRAY_SIZE(k_sweep_states); si++) {
				uint8_t buf[8];
				uint8_t len;
				LoadState start = k_sweep_states[si];
				LoadState expect = (LoadState)k_next_state[ei][start];
				bool exists;
				ErrorCode code;

				reset();
				__memory.load_state[idx] = start;

				if (k_sweep_events[ei] == LE_ADDITIONAL_LOAD_CONTROLS) {
					set_alc(buf, 0, false, 0);
					len = 8;
				} else {
					buf[0] = k_sweep_events[ei];
					len = 1;
				}

				(void)load_state_machine_event(idx, buf, len);
				code = read_error_code(idx, &exists);

				if (!exists) {
					continue; /* Interfaceprogram Object: no PID_ERROR_CODE */
				}
				if (expect == LS_ERROR && start != LS_ERROR) {
					zassert_equal(code, E_GOT_UNDEF_LOAD_CMD,
						      "obj=%u event=%u from state=%u: newly-Error "
						      "code should be E_GOT_UNDEF_LOAD_CMD, got %u",
						      idx, k_sweep_events[ei], start, code);
				} else if (expect != LS_ERROR) {
					zassert_equal(
						code, E_NO_FAULT,
						"obj=%u event=%u from state=%u: leaving/staying "
						"out of Error must read E_NO_FAULT, got %u",
						idx, k_sweep_events[ei], start, code);
				}
			}
		}
	}
}

ZTEST(ls_sweep, test_table_reference_is_zero_only_for_unloaded_or_error)
{
	for (size_t oi = 0; oi < ARRAY_SIZE(k_sweep_objects); oi++) {
		uint8_t idx = k_sweep_objects[oi];

		for (size_t ei = 0; ei < ARRAY_SIZE(k_sweep_events); ei++) {
			for (size_t si = 0; si < ARRAY_SIZE(k_sweep_states); si++) {
				uint8_t buf[8];
				uint8_t len;
				LoadState start = k_sweep_states[si];
				LoadState expect = (LoadState)k_next_state[ei][start];
				uint32_t expect_ref, got_ref;

				reset();
				__memory.load_state[idx] = start;

				if (k_sweep_events[ei] == LE_ADDITIONAL_LOAD_CONTROLS) {
					set_alc(buf, 0, false, 0);
					len = 8;
				} else {
					buf[0] = k_sweep_events[ei];
					len = 1;
				}

				(void)load_state_machine_event(idx, buf, len);

				expect_ref = (expect == LS_UNLOADED || expect == LS_ERROR)
						     ? 0U
						     : memory_table_offset(idx);
				got_ref = ntohl(*k_table_ref[idx]);

				zassert_equal(got_ref, expect_ref,
					      "obj=%u event=%u from state=%u: PID_TABLE_REFERENCE "
					      "expected 0x%08x, got 0x%08x",
					      idx, k_sweep_events[ei], start, expect_ref, got_ref);
			}
		}
	}
}

/*
 * A malformed event byte (outside the 5 defined LoadEvents) must still force
 * LS_ERROR with E_GOT_UNDEF_LOAD_CMD from every reachable state — the
 * "default:" arm of every per-state switch.
 */
ZTEST(ls_sweep, test_unknown_event_byte_forces_error)
{
	for (size_t oi = 0; oi < ARRAY_SIZE(k_sweep_objects); oi++) {
		uint8_t idx = k_sweep_objects[oi];

		for (size_t si = 0; si < ARRAY_SIZE(k_sweep_states); si++) {
			uint8_t buf[1] = {0xEFu}; /* not a defined LoadEvents value */
			bool exists;
			ErrorCode code;

			reset();
			__memory.load_state[idx] = k_sweep_states[si];

			bool ok = load_state_machine_event(idx, buf, sizeof(buf));

			zassert_true(ok);
			zassert_equal(
				__memory.load_state[idx], LS_ERROR,
				"obj=%u state=%u: an undefined event byte must force LS_ERROR", idx,
				k_sweep_states[si]);
			code = read_error_code(idx, &exists);
			if (exists) {
				zassert_equal(code, E_GOT_UNDEF_LOAD_CMD);
			}
		}
	}
}

/* ============================================================
 * Suite 2: a realistic ETS download narrative, through the real
 * interface_write_property() entry point.
 * ============================================================
 */

ZTEST_SUITE(ls_download, NULL, NULL, NULL, NULL, NULL);

/*
 * complete_load = false stops right after the fill check and unloads from
 * LS_LOADING directly, WITHOUT ever sending LE_LOAD_COMPLETED. Used for
 * OBJ_IDX_APPLICATION_PROG: reaching LS_LOADED there fires the Run State
 * coupling (object_application_program_properties_written() latches
 * _run_state = RS_RUNNING permanently — it is never reset back to Halted,
 * by design, since Run State is not persisted). rs_coupling's own tests
 * need that object's Run State to still be virgin (Halted) when they run,
 * so this narrative deliberately never completes that object's load.
 */
static void download_narrative(uint8_t idx, bool complete_load)
{
	uint8_t ev;
	uint8_t alc[8];
	uint8_t *base;

	reset();

	ev = LE_UNLOAD;
	write_load_event(idx, &ev, 1, true);
	zassert_equal(__memory.load_state[idx], LS_UNLOADED);
	zassert_equal(ntohl(*k_table_ref[idx]), 0U);

	ev = LE_START_LOADING;
	write_load_event(idx, &ev, 1, true);
	zassert_equal(__memory.load_state[idx], LS_LOADING);
	zassert_equal(ntohl(*k_table_ref[idx]), memory_table_offset(idx),
		      "obj=%u: PID_TABLE_REFERENCE must already be valid right after "
		      "Start Loading, before any allocation event (§2.4b)",
		      idx);

	set_alc(alc, 4, true, 0xAAu);
	write_load_event(idx, alc, sizeof(alc), true);
	zassert_equal(__memory.load_state[idx], LS_LOADING);
	base = (uint8_t *)&__memory + memory_table_offset(idx);
	for (int i = 0; i < 4; i++) {
		zassert_equal(base[i], 0xAAu, "obj=%u: fill byte %d not applied", idx, i);
	}
	zassert_equal(base[4], 0x00u, "obj=%u: fill must not spill past the requested size", idx);

	if (complete_load) {
		ev = LE_LOAD_COMPLETED;
		write_load_event(idx, &ev, 1, true);
		zassert_equal(__memory.load_state[idx], LS_LOADED);
		zassert_equal(ntohl(*k_table_ref[idx]), memory_table_offset(idx));
	}

	ev = LE_UNLOAD;
	write_load_event(idx, &ev, 1, true);
	zassert_equal(__memory.load_state[idx], LS_UNLOADED);
	zassert_equal(ntohl(*k_table_ref[idx]), 0U);
}

ZTEST(ls_download, test_address_table_download_narrative)
{
	download_narrative(OBJ_IDX_ADDRESS_TABLE, true);
}

ZTEST(ls_download, test_application_program_download_narrative)
{
	/* §1.3's own fix comment calls this object out specifically: it has no
	 * PID_TABLE, so an allocation fill going through the property path
	 * instead of memory_table_offset() used to silently skip it. Stops
	 * before Load Completed — see download_narrative()'s comment — leaving
	 * this object's Run State untouched for the rs_coupling suite.
	 */
	download_narrative(OBJ_IDX_APPLICATION_PROG, false);
}

/* ============================================================
 * Suite 3: allocation clamping — the unbounded-memset fix.
 * ============================================================
 */

ZTEST_SUITE(ls_clamp, NULL, NULL, NULL, NULL, NULL);

static void oversized_allocation_case(uint8_t idx, uint8_t *canary, size_t canary_len)
{
	uint8_t ev;
	uint8_t alc[8];
	uint32_t oversized;
	bool exists;
	ErrorCode code;

	reset();

	ev = LE_START_LOADING;
	write_load_event(idx, &ev, 1, true);

	memset(canary, 0x55u, canary_len);

	oversized = memory_table_capacity(idx) + 1U;
	set_alc(alc, oversized, true, 0xAAu);
	write_load_event(idx, alc, sizeof(alc), true);

	zassert_equal(__memory.load_state[idx], LS_ERROR,
		      "obj=%u: an oversized allocation must force LS_ERROR", idx);
	code = read_error_code(idx, &exists);
	if (exists) {
		zassert_equal(code, E_MAX_TABLE_LENGTH_EXEEDED);
	}
	zassert_equal(ntohl(*k_table_ref[idx]), 0U);

	for (size_t i = 0; i < canary_len; i++) {
		zassert_equal(canary[i], 0x55u,
			      "obj=%u: an oversized allocation must not memset past its own "
			      "segment — byte %zu past the segment was overwritten",
			      idx, i);
	}
}

ZTEST(ls_clamp, test_address_table_oversized_allocation_does_not_corrupt_associations)
{
	/* struct Memory's declared layout puts associations[] immediately after
	 * addresses[] — the exact field an unclamped memset used to run into.
	 */
	oversized_allocation_case(OBJ_IDX_ADDRESS_TABLE, (uint8_t *)&__memory.associations[0], 4);
}

ZTEST(ls_clamp, test_application_program_oversized_allocation_does_not_corrupt_group_object_table)
{
	/* group_object[] is the field immediately after application_program[]. */
	oversized_allocation_case(OBJ_IDX_APPLICATION_PROG, (uint8_t *)&__memory.group_object[0],
				  4);
}

/* ============================================================
 * Suite 4: re-allocating the same segment a second time.
 * ============================================================
 */

ZTEST_SUITE(ls_realloc, NULL, NULL, NULL, NULL, NULL);

ZTEST(ls_realloc, test_reallocating_after_unload_keeps_the_reference_and_refreshes_content)
{
	uint8_t idx = OBJ_IDX_ADDRESS_TABLE;
	uint8_t ev;
	uint8_t alc[8];
	uint32_t ref1, ref2;
	uint8_t *base = (uint8_t *)&__memory + memory_table_offset(idx);

	reset();

	ev = LE_START_LOADING;
	write_load_event(idx, &ev, 1, true);
	set_alc(alc, 4, true, 0x11u);
	write_load_event(idx, alc, sizeof(alc), true);
	ev = LE_LOAD_COMPLETED;
	write_load_event(idx, &ev, 1, true);
	ref1 = ntohl(*k_table_ref[idx]);

	ev = LE_UNLOAD;
	write_load_event(idx, &ev, 1, true);
	zassert_equal(ntohl(*k_table_ref[idx]), 0U);

	ev = LE_START_LOADING;
	write_load_event(idx, &ev, 1, true);
	set_alc(alc, 4, true, 0x22u);
	write_load_event(idx, alc, sizeof(alc), true);
	ev = LE_LOAD_COMPLETED;
	write_load_event(idx, &ev, 1, true);
	ref2 = ntohl(*k_table_ref[idx]);

	zassert_equal(ref1, ref2,
		      "PID_TABLE_REFERENCE is a pure function of the state (§2.4b) — "
		      "re-allocating the same segment must produce the same reference");
	for (int i = 0; i < 4; i++) {
		zassert_equal(base[i], 0x22u, "byte %d must reflect the SECOND fill, not the first",
			      i);
	}
}

/* ============================================================
 * Suite 5: a truncated Additional Load Controls PDU.
 * ============================================================
 */

ZTEST_SUITE(ls_truncated, NULL, NULL, NULL, NULL, NULL);

ZTEST(ls_truncated, test_short_additional_load_controls_pdu_is_refused_without_touching_state)
{
	uint8_t idx = OBJ_IDX_ADDRESS_TABLE;
	uint8_t ev = LE_START_LOADING;
	uint8_t short_alc[3] = {LE_ADDITIONAL_LOAD_CONTROLS, 0x0Bu, 0x00u};

	reset();
	write_load_event(idx, &ev, 1, true);

	write_load_event(idx, short_alc, sizeof(short_alc), false);

	zassert_equal(__memory.load_state[idx], LS_LOADING,
		      "a truncated PDU must leave the Load State untouched, not force LS_ERROR "
		      "(a truncated PDU must not look like a failed allocation to ETS)");
}

/* ============================================================
 * Suite 6 (the folded run_state_machine suite — see the file header):
 * the Load -> Run State coupling and the boot-restore path.
 * ============================================================
 */

ZTEST_SUITE(rs_coupling, NULL, NULL, NULL, NULL, NULL);

/*
 * Uses the Interfaceprogram Object (index 5) rather than the primary
 * Application Program object: each keeps its OWN private `_run_state`
 * static (object_interface_program.c / object_application_program.c), never
 * reset back to Halted once Running (by design — Run State is not
 * persisted, and nothing walks it backwards). Using a separate object here
 * keeps this test independent of test_load_run_state_coupling_narrative
 * below regardless of ZTEST's execution order.
 */
ZTEST(rs_coupling, test_boot_restore_path_corrects_run_state_to_running)
{
	uint8_t idx = OBJ_IDX_INTERFACE_PROG;

	reset();
	/* Simulate memory_read() restoring an already-LOADED table from NVS,
	 * bypassing the event path entirely.
	 */
	__memory.load_state[idx] = LS_LOADED;
	zassert_equal(read_run_state(idx), TEST_RS_HALTED,
		      "run state starts Halted even over a restored Loaded state");

	/* The exact call knx_core.c:85 makes right after memory_read(). */
	object_interface_program_properties_written(PID_LOAD_STATE_CONTROL);

	zassert_equal(read_run_state(idx), TEST_RS_RUNNING,
		      "the boot-restore call must correct run state to Running (§9.7)");
}

ZTEST(rs_coupling, test_load_run_state_coupling_narrative)
{
	uint8_t idx = OBJ_IDX_APPLICATION_PROG;
	uint8_t ev;
	uint8_t alc[8];

	reset();

	ev = LE_START_LOADING;
	write_load_event(idx, &ev, 1, true);
	zassert_equal(read_run_state(idx), TEST_RS_HALTED,
		      "run state must not advance before Load State reaches Loaded");

	set_alc(alc, memory_table_capacity(idx) + 1U, true, 0xAAu);
	write_load_event(idx, alc, sizeof(alc), true);
	zassert_equal(__memory.load_state[idx], LS_ERROR);
	zassert_equal(read_run_state(idx), TEST_RS_HALTED,
		      "an object stuck in LS_ERROR must not report Running");

	ev = LE_UNLOAD;
	write_load_event(idx, &ev, 1, true);
	ev = LE_START_LOADING;
	write_load_event(idx, &ev, 1, true);
	ev = LE_LOAD_COMPLETED;
	write_load_event(idx, &ev, 1, true);

	zassert_equal(__memory.load_state[idx], LS_LOADED);
	zassert_equal(read_run_state(idx), TEST_RS_RUNNING,
		      "Load State reaching Loaded must auto-advance Run State to Running (§9.7)");
}
