/*
 * Copyright (c) 2026 Fabien Proriol
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SUBSYS_KNX_OBJECT_MEMORY_H_
#define ZEPHYR_SUBSYS_KNX_OBJECT_MEMORY_H_

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <zephyr/sys/util.h>
#include "object_interface.h"

void memory_write(void);

/* Restores knx_mem from NVS. Returns true if a previously persisted blob
 * of the right size, magic, and KNX_MEMORY_LAYOUT_VERSION was found and
 * restored; false if knx_mem was reset to defaults instead (first boot,
 * corrupt/incompatible/wrong-version NVS blob, or no settings backend).
 * Load State lives inside knx_mem (see struct knx_memory.load_state),
 * so either way the returned knx_mem already carries the
 * correct Load State for every table object — callers do not need to
 * special-case the false branch.
 */
bool memory_read(void);

void memory_modified(void);

/* Wipes knx_mem to defaults (individual address = 0xFFFF, all tables
 * cleared, magic header rewritten). Does NOT persist to flash — call
 * memory_write() afterward if the reset must survive a power cycle.
 */
void memory_reset(void);

/*
 * These sizes/counts come from Kconfig (KNX_STACK) so that this header never
 * needs to include an application-provided file. CONFIG_KNX_* is undefined
 * when CONFIG_KNX_STACK=n (e.g. host-side unit tests that don't enable the
 * stack's Kconfig), hence the fallback defaults below.
 */
#if !defined(KNX_MAX_NUMBER_OF_ADDRESS_GROUP)
#if defined(CONFIG_KNX_MAX_ADDRESS_GROUP)
#define KNX_MAX_NUMBER_OF_ADDRESS_GROUP CONFIG_KNX_MAX_ADDRESS_GROUP
#else
#define KNX_MAX_NUMBER_OF_ADDRESS_GROUP 256
#endif
#endif

#if !defined(KNX_MAX_NUMBER_OF_ASSOCIATIONS)
#if defined(CONFIG_KNX_MAX_ASSOCIATIONS)
#define KNX_MAX_NUMBER_OF_ASSOCIATIONS CONFIG_KNX_MAX_ASSOCIATIONS
#else
#define KNX_MAX_NUMBER_OF_ASSOCIATIONS 255
#endif
#endif

#ifndef KNX_GROUP_OBJECT_COUNT
#if defined(CONFIG_KNX_GROUP_OBJECT_COUNT)
#define KNX_GROUP_OBJECT_COUNT CONFIG_KNX_GROUP_OBJECT_COUNT
#else
#define KNX_GROUP_OBJECT_COUNT 1
#endif
#endif

#if defined(CONFIG_KNX_APPLICATION_PROGRAM_DATA_SIZE)
#define KNX_APPLICATION_PROGRAM_DATA_SIZE CONFIG_KNX_APPLICATION_PROGRAM_DATA_SIZE
#else
#define KNX_APPLICATION_PROGRAM_DATA_SIZE 1
#endif

#if defined(CONFIG_KNX_GROUP_OBJECTS_DATA_SIZE)
#define KNX_GROUP_OBJECTS_DATA_SIZE CONFIG_KNX_GROUP_OBJECTS_DATA_SIZE
#else
#define KNX_GROUP_OBJECTS_DATA_SIZE 1
#endif

#if defined(CONFIG_KNX_INTERFACE_PROGRAM_DATA_SIZE)
#define KNX_INTERFACE_PROGRAM_DATA_SIZE CONFIG_KNX_INTERFACE_PROGRAM_DATA_SIZE
#else
#define KNX_INTERFACE_PROGRAM_DATA_SIZE 1
#endif

/*
 * struct knx_memory layout version — bump this EVERY time struct knx_memory's shape
 * changes (a field's type/size changes, fields are reordered, a field's
 * meaning changes) even if sizeof(struct knx_memory) happens to stay the same.
 *
 * On mismatch (or a stored blob of the wrong length), knx_mem_set()
 * discards the stored blob entirely and boots from a clean memory_reset()
 * instead of attempting a byte-level migration. The previous
 * "re-default a shorter blob's tail" approach was fragile enough in
 * practice to be worse than starting clean: it silently reset a genuinely
 * persisted Load State back to LS_UNLOADED on a real device, because a
 * newly appended field (load_state[]) landed inside what the migration
 * code zeroed. Downloaded tables and the
 * individual address are lost on a version-mismatch wipe — deliberate, not
 * a bug: re-commissioning after a firmware update that changes the storage
 * format is expected, and is far easier to reason about than trying to
 * predict every way a "shorter blob" migration could silently mis-default
 * a new field.
 *
 * History:
 *   1 — 2026-08-22: first version-checked layout (load_state[],
 *       program_version_2[], interface_program[] already present).
 *   2 — 2026-08-27: removed group_object_data[] — Group
 *       Object values are RAM-only now (see object_group_table.c), so a
 *       power cycle wipes them by design rather than by accident. Never
 *       addressed via PID_TABLE_REFERENCE/KNX_A_MEMORY_WRITE (memory_table_
 *       offset()/_capacity() for KNX_OBJ_IDX_GROUP_OBJ_TABLE only ever covered
 *       group_object[], the descriptor table) — removing it does not move
 *       anything ETS actually writes to.
 */
#define KNX_MEMORY_LAYOUT_VERSION ((uint8_t)2)

struct knx_memory {
	uint8_t start[4];
	uint8_t device_control;
	uint8_t routing_count; /* Have to be initialized to 6 */
	uint8_t subnet_addr;
	uint8_t device_addr;
	/*
	 * KNX spec 3/5/1 §4.16.7 "Group Address Table" — index 0 = length,
	 * indices 1..N = entries (2 octets each) → N+1 slots.
	 */
	uint16_t addresses[KNX_MAX_NUMBER_OF_ADDRESS_GROUP + 1];
	/*
	 * KNX spec 3/5/1 §4.17.5.2.5 "PID_TABLE for the Group Object
	 * Association Table — format 1" (PDT_GENERIC_04, as declared by
	 * object_association_table_properties[]): index 0 = length (1 slot),
	 * each entry = TSAP (2 octets) + ASAP (2 octets) = 2 slots → 2N+1 slots.
	 * Confirmed against the reference libknx/Thelsing knx-stack, which both
	 * declare PDT_GENERIC_04 and store TSAP/ASAP as separate uint16 fields.
	 */
	uint16_t associations[KNX_MAX_NUMBER_OF_ASSOCIATIONS * 2 + 1];
	uint8_t application_program[KNX_APPLICATION_PROGRAM_DATA_SIZE];
	/*
	 * KNX spec 3/5/1 §4.18.5 "Group Object Table — Realisation Type 6":
	 * index 0 = length, indices 1..N = descriptors (2 octets each) → N+1 slots.
	 */
	uint16_t group_object[KNX_GROUP_OBJECT_COUNT + 1];
	uint8_t end;

	/*
	 * ---- Fields appended after the original layout ----
	 *
	 * Everything above keeps its historical offset, because those offsets ARE
	 * the KNX memory addresses ETS uses (PID_TABLE_REFERENCE + KNX_A_MEMORY_WRITE).
	 * New persisted state goes here. Unlike some earlier revisions of this
	 * file, appending a field here no longer implies knx_mem_set() will try
	 * to migrate an older, shorter blob — any layout change bumps
	 * KNX_MEMORY_LAYOUT_VERSION above, and a version (or length) mismatch
	 * wipes knx_mem back to defaults instead. Keeping new fields appended
	 * at the end remains good practice regardless — it is what keeps a
	 * `struct knx_memory` dump readable across versions during development —
	 * but is no longer load-bearing for correctness the way it was before.
	 */

	/*
	 * PID_PROGRAM_VERSION (PID 13) of the Application Program object:
	 * manufacturer ID (2) + application ID (2) + version (1).
	 *
	 * ETS writes it at download step 05 (KNX spec 3/5/3 §3.5.2) and reads it
	 * back to decide whether the loaded application matches the project. It
	 * MUST survive a power cycle: as a RAM-only static it read back as
	 * 00 00 00 00 00 after every reboot, and ETS refused any partial download
	 * with "the application program loaded in the device does not match the
	 * application projected in ETS".
	 */
	uint8_t program_version[5];

	/*
	 * PID_PROGRAM_VERSION of the Interfaceprogram Object ("Application
	 * Program 2", object index 5) — same format and persistence rationale
	 * as `program_version` above, for the second application program slot
	 * Volume 6 §A.2.7 requires.
	 */
	uint8_t program_version_2[5];

	/*
	 * Interfaceprogram Object (Object Type 4, "Application Program 2")
	 * download segment — Volume 6 §A.2.7. Nothing on this
	 * product's ETS download path (PtSensor.xml) targets it; the segment
	 * exists so the object is mandatory-conformant and so ETS's Unload /
	 * Start Loading of object index 5 has somewhere to allocate.
	 */
	uint8_t interface_program[KNX_INTERFACE_PROGRAM_DATA_SIZE];

	/*
	 * PID_LOAD_STATE_CONTROL for every table object, indexed by object_index
	 * directly (index 0 = Device Object, unused — it has no Load State).
	 *
	 * 03_05_01 Resources §4.23.2 (p.293): "The value of the Property Load
	 * Control shall be stored in non-volatile memory, because it shall be
	 * preserved also on power fail." Before this field existed, each
	 * object_*.c kept its own RAM-only `_load_state` static, independent of
	 * whether knx_mem's real table data survived a reboot — with
	 * CONFIG_KNX_AUTO_LOAD_TABLES=n a fully programmed device reported
	 * LS_UNLOADED after every power cycle despite the tables still being
	 * there.
	 *
	 * memory_reset() is the only place that sets these away from 0
	 * (LS_UNLOADED) — see its CONFIG_KNX_AUTO_LOAD_TABLES handling — so
	 * whatever memory_read() returns (freshly reset or genuinely restored
	 * from NVS) already carries the correct value for every object.
	 */
	uint8_t load_state[KNX_NUMBER_OF_INTERFACE];

	/*
	 * Set by memory_reset() to KNX_MEMORY_LAYOUT_VERSION, checked by
	 * knx_mem_set() against the firmware's current KNX_MEMORY_LAYOUT_VERSION
	 * before trusting anything else in a restored blob. Must stay the LAST
	 * field: a blob from a firmware that predates this field entirely is
	 * exactly the "wrong version" case (its stored length falls short of
	 * this field's offset), and that must keep working without special-casing.
	 */
	uint8_t memory_layout_version;
};

extern struct knx_memory knx_mem;

/* Mutable PID_TABLE_REFERENCE values — 0 when unloaded, offset after Relative Allocation */
extern uint32_t table_reference_addresses;
extern uint32_t table_reference_associations;
extern uint32_t table_reference_application_program;
extern uint32_t table_reference_group_object;
extern uint32_t table_reference_interface_program;

/* Canonical big-endian offsets for use by auto-load paths */
extern const uint32_t k_table_ref_addresses;
extern const uint32_t k_table_ref_associations;
extern const uint32_t k_table_ref_application_program;
extern const uint32_t k_table_ref_group_object;
extern const uint32_t k_table_ref_interface_program;

bool memory_user_write(uint32_t address, size_t len, uint8_t *data);

/**
 * memory_table_offset - byte offset of the download table for object_index
 *                       within struct knx_memory (= the KNX memory address ETS
 *                       uses with KNX_A_MEMORY_WRITE after Relative Allocation).
 *
 * Returns 0 for objects that have no associated table (e.g. Device Object).
 */
uint32_t memory_table_offset(uint8_t object_index);

/**
 * memory_table_capacity - usable size in octets of the download segment for
 *                         object_index within struct knx_memory.
 *
 * This is the largest allocation the load state machine may accept for that
 * segment; a larger request must be refused with PID_TABLE_REFERENCE = 0 and
 * Load State = Error (KNX spec 3/5/3 §3.5.1.2).
 *
 * Returns 0 for objects that have no associated table (e.g. Device Object).
 */
uint32_t memory_table_capacity(uint8_t object_index);

#endif /* ZEPHYR_SUBSYS_KNX_OBJECT_MEMORY_H_ */
