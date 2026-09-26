/*
 * Copyright (c) 2026 Fabien Proriol
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>
#include <stddef.h>
#include <zephyr/logging/log.h>
#include "object_application_program.h"
#include "object_device.h"
#include "object_memory.h"

LOG_MODULE_REGISTER(knx_app_prog, CONFIG_KNX_STACK_LOG_LEVEL);

/*
 * PID_PROGRAM_VERSION is stored in __memory (i.e. in NVS), not in a RAM static.
 * ETS writes it at download step 05 and reads it back to decide whether the
 * device holds the application its project expects; a volatile copy read back
 * as all zeros after every reboot, and ETS then refused every partial download
 * with "the application program loaded in the device does not match the
 * application projected in ETS".
 */

/*
 * PID_RUN_STATE_CONTROL values (KNX spec §9.7 Run State Machine). LoadState
 * is really just a uint8_t (see object_property_types.h), reused here for
 * storage since RSM and LSM share the same byte-sized property datatype
 * (PDT_CONTROL) — these names are the RSM's own, not LoadState's LS_* ones.
 */
#define RS_HALTED  0
#define RS_RUNNING 1
#define RS_READY   2

static knx_u16_be_t _object_type = KNX_U16_BE(OT_APPLICATION_PROG);
KNX_PROP_ASSERT_SIZE(PDT_UNSIGNED_INT, _object_type);
/* PID_LOAD_STATE_CONTROL storage is __memory.load_state[OBJ_IDX_APPLICATION_PROG]
 * — persisted in NVS, not a RAM-only static.
 */
KNX_PROP_ASSERT_SIZE(PDT_CONTROL, __memory.load_state[OBJ_IDX_APPLICATION_PROG]);
/*
 * Volatile RAM, always starts Halted — Run State itself is never persisted
 * (only Load State is). knx_core.c corrects this to Running
 * right after memory_read() by calling
 * object_application_program_properties_written(PID_LOAD_STATE_CONTROL),
 * which is the same auto Halted -> Ready -> Running rule §9.7 the Run State
 * Machine already applies on an explicit Load State write — reused here so
 * a device that persisted LS_LOADED (regardless of
 * CONFIG_KNX_AUTO_LOAD_TABLES, which only affects a *freshly reset*
 * __memory) boots straight to Running instead of sitting Halted over data
 * it believes is loaded.
 */
static LoadState _run_state = RS_HALTED;
KNX_PROP_ASSERT_SIZE(PDT_CONTROL, _run_state);
static ErrorCode _error_code = E_NO_FAULT;
KNX_PROP_ASSERT_SIZE(PDT_ENUM8, _error_code);
static char _program_name[10] = "";
static uint8_t _pei_type;
KNX_PROP_ASSERT_SIZE(PDT_UNSIGNED_CHAR, _pei_type);
KNX_PROP_ASSERT_SIZE(PDT_GENERIC_05, __memory.program_version);
KNX_PROP_ASSERT_SIZE(PDT_UNSIGNED_LONG, table_reference_application_program);

struct property object_application_program_properties[] = {
	KNX_PROP_SCALAR(PID_OBJECT_TYPE, PDT_UNSIGNED_INT, false, ReadLv3 | WriteLv0,
			&_object_type),
	KNX_PROP_CHARS(PID_OBJECT_NAME, false, ReadLv3 | WriteLv0, _program_name),
	KNX_PROP_SCALAR(PID_ERROR_CODE, PDT_ENUM8, false, ReadLv3 | WriteLv0, &_error_code),
	KNX_PROP_SCALAR(PID_LOAD_STATE_CONTROL, PDT_CONTROL, true, ReadLv3 | WriteLv3,
			&__memory.load_state[OBJ_IDX_APPLICATION_PROG]),
	KNX_PROP_SCALAR(PID_RUN_STATE_CONTROL, PDT_CONTROL, false, ReadLv3 | WriteLv0, &_run_state),
	KNX_PROP_SCALAR(PID_PROGRAM_VERSION, PDT_GENERIC_05, true, ReadLv3 | WriteLv3,
			&__memory.program_version),
	KNX_PROP_SCALAR(PID_PEI_TYPE, PDT_UNSIGNED_CHAR, false, ReadLv3 | WriteLv0, &_pei_type),
	KNX_PROP_SCALAR(PID_TABLE_REFERENCE, PDT_UNSIGNED_LONG, false, ReadLv3 | WriteLv0,
			&table_reference_application_program),
	{.description = {.property_id = PID_LAST}}};

void object_application_program_init(void)
{
	bool unset = true;

	for (size_t i = 0; i < sizeof(__memory.program_version); i++) {
		if (__memory.program_version[i] != 0u) {
			unset = false;
			break;
		}
	}

	if (!unset) {
		/* Either ETS wrote it during a download, or a previous boot declared
		 * it — leave it alone.  ETS is the authority once it has written.
		 */
		LOG_DBG("PID_PROGRAM_VERSION = %02X %02X %02X %02X %02X (stored)",
			__memory.program_version[0], __memory.program_version[1],
			__memory.program_version[2], __memory.program_version[3],
			__memory.program_version[4]);
		return;
	}

#if defined(CONFIG_KNX_DECLARE_APPLICATION_PROGRAM)
	/*
	 * Declare the application this firmware implements.  On this device the
	 * application program is compiled in rather than downloaded, so a
	 * factory-fresh unit can legitimately state which application it runs —
	 * without this, ETS refuses every partial download until a full one has
	 * written the value (KNX spec 3/5/3 §3.5.2 step 05).
	 */
	uint16_t mfr = device_manufacturer_id();

	__memory.program_version[0] = (uint8_t)(mfr >> 8);
	__memory.program_version[1] = (uint8_t)(mfr & 0xFFu);
	__memory.program_version[2] = (uint8_t)((CONFIG_KNX_APPLICATION_PROGRAM_ID >> 8) & 0xFFu);
	__memory.program_version[3] = (uint8_t)(CONFIG_KNX_APPLICATION_PROGRAM_ID & 0xFFu);
	__memory.program_version[4] = (uint8_t)CONFIG_KNX_APPLICATION_PROGRAM_VERSION;

	LOG_INF("PID_PROGRAM_VERSION declared as %02X %02X %02X %02X %02X "
		"(manufacturer %04X, application %04X, version %u.%u)",
		__memory.program_version[0], __memory.program_version[1],
		__memory.program_version[2], __memory.program_version[3],
		__memory.program_version[4], mfr, (unsigned int)CONFIG_KNX_APPLICATION_PROGRAM_ID,
		(unsigned int)(CONFIG_KNX_APPLICATION_PROGRAM_VERSION >> 4),
		(unsigned int)(CONFIG_KNX_APPLICATION_PROGRAM_VERSION & 0x0Fu));
	memory_modified();
#else
	LOG_WRN("PID_PROGRAM_VERSION is unset and "
		"CONFIG_KNX_DECLARE_APPLICATION_PROGRAM is off — ETS will report that "
		"the loaded application does not match the project until a full "
		"download writes it");
#endif
}

void object_application_program_properties_written(PropertyID id)
{
	/*
	 * KNX spec §9.7 Run State Machine: auto Halted -> Ready -> Running
	 * once Load State reaches Loaded (at start-up, or after an ETS
	 * download completes). Ready<->Running is always automatic and this
	 * device defines no other run condition. PID_RUN_STATE_CONTROL is
	 * not write_enable here, so there is no "manually stopped" state to
	 * respect.
	 */
	if (id == PID_LOAD_STATE_CONTROL &&
	    __memory.load_state[OBJ_IDX_APPLICATION_PROG] == LS_LOADED) {
		_run_state = RS_RUNNING;
	}

	/*
	 * PID_PROGRAM_VERSION lives in __memory, so a write must schedule the
	 * debounced NVS flush — otherwise it would only reach flash if some
	 * unrelated change happened to trigger one.
	 */
	if (id == PID_PROGRAM_VERSION) {
		memory_modified();
	}
}
