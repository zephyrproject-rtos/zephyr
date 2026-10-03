/*
 * Copyright (c) 2026 Fabien Proriol
 * SPDX-License-Identifier: Apache-2.0
 *
 * Interfaceprogram Object (Object Type 4, "Application Program 2") —
 * KNX Volume 6 Annex A.2.7 (p.148).
 *
 * Mandatory for mask 07B0h even though System B Annex A.2.2's object list
 * omits it: footnote 67 on Annex A.2.6 states "System B foresees two
 * Application Programs", and the complete download procedure (03_05_03
 * §3.5.2, p.42) unloads it first (step 05) and loads it first (step 06).
 * Without this object ETS's Unload of object index 5 has no target.
 *
 * Mirrors object_application_program.c's property set and Run State Machine
 * behaviour, minus PID_OBJECT_NAME and PID_ERROR_CODE — Annex A.2.7 has no
 * row for either at 07B0h, unlike the primary Application Program object.
 */

#include <stddef.h>
#include "object_interface_program.h"
#include "object_memory.h"

/*
 * PID_RUN_STATE_CONTROL values (KNX spec §9.7 Run State Machine) — same
 * reuse of LoadState's storage as object_application_program.c's RS_HALTED
 * / RS_RUNNING, duplicated locally rather than shared since neither header
 * exports them.
 */
#define RS_HALTED  0
#define RS_RUNNING 1

static knx_u16_be_t _object_type = KNX_U16_BE(OT_INTERFACE_PROG);
KNX_PROP_ASSERT_SIZE(PDT_UNSIGNED_INT, _object_type);
/* PID_LOAD_STATE_CONTROL storage is __memory.load_state[OBJ_IDX_INTERFACE_PROG]
 * — persisted in NVS, not a RAM-only static.
 */
KNX_PROP_ASSERT_SIZE(PDT_CONTROL, __memory.load_state[OBJ_IDX_INTERFACE_PROG]);
/* Same auto Halted -> Running behaviour as object_application_program.c —
 * corrected at boot from the (possibly persisted) Load State, not from
 * CONFIG_KNX_AUTO_LOAD_TABLES directly; see the comment there.
 */
static LoadState _run_state = RS_HALTED;
KNX_PROP_ASSERT_SIZE(PDT_CONTROL, _run_state);
static uint8_t _pei_type;
KNX_PROP_ASSERT_SIZE(PDT_UNSIGNED_CHAR, _pei_type);
KNX_PROP_ASSERT_SIZE(PDT_GENERIC_05, __memory.program_version_2);
KNX_PROP_ASSERT_SIZE(PDT_UNSIGNED_LONG, table_reference_interface_program);

struct property object_interface_program_properties[] = {
	KNX_PROP_SCALAR(PID_OBJECT_TYPE, PDT_UNSIGNED_INT, false, ReadLv3 | WriteLv0,
			&_object_type),
	KNX_PROP_SCALAR(PID_LOAD_STATE_CONTROL, PDT_CONTROL, true, ReadLv3 | WriteLv3,
			&__memory.load_state[OBJ_IDX_INTERFACE_PROG]),
	KNX_PROP_SCALAR(PID_RUN_STATE_CONTROL, PDT_CONTROL, false, ReadLv3 | WriteLv0, &_run_state),
	KNX_PROP_SCALAR(PID_PROGRAM_VERSION, PDT_GENERIC_05, true, ReadLv3 | WriteLv3,
			&__memory.program_version_2),
	KNX_PROP_SCALAR(PID_PEI_TYPE, PDT_UNSIGNED_CHAR, false, ReadLv3 | WriteLv0, &_pei_type),
	KNX_PROP_SCALAR(PID_TABLE_REFERENCE, PDT_UNSIGNED_LONG, false, ReadLv3 | WriteLv0,
			&table_reference_interface_program),
	{.description = {.property_id = PID_LAST}}};

void object_interface_program_properties_written(PropertyID id)
{
	/* Same auto Halted -> Ready -> Running rule as the primary Application
	 * Program object (KNX spec §9.7 Run State Machine).
	 */
	if (id == PID_LOAD_STATE_CONTROL &&
	    __memory.load_state[OBJ_IDX_INTERFACE_PROG] == LS_LOADED) {
		_run_state = RS_RUNNING;
	}

	if (id == PID_PROGRAM_VERSION) {
		memory_modified();
	}
}
