/*
 * Copyright (c) 2026 Fabien Proriol
 * SPDX-License-Identifier: Apache-2.0
 *
 * KNX persistent memory — Zephyr settings/NVS backend.
 *
 * knx_mem is stored as a single settings blob under the key "knx/mem".
 * Writes are debounced: knx_memory_modified() schedules a delayed work item;
 * the actual flash write happens CONFIG_KNX_SETTINGS_DEBOUNCE_MS ms later
 * to coalesce rapid ETS-download bursts into one NVS write.
 */

#include "object_memory.h"
#include <string.h>
#include <stddef.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#if defined(CONFIG_SETTINGS)
#include <zephyr/settings/settings.h>
#endif

LOG_MODULE_REGISTER(knx_memory, CONFIG_KNX_STACK_LOG_LEVEL);

#ifndef CONFIG_KNX_SETTINGS_DEBOUNCE_MS
#define CONFIG_KNX_SETTINGS_DEBOUNCE_MS 200
#endif

#define KNX_SETTINGS_KEY "knx/mem"

struct knx_memory knx_mem;

/*
 * PID_TABLE_REFERENCE values (KNX big-endian 32-bit offset into knx_mem).
 *
 * Lifecycle per KNX spec §14.1b / §15.3:
 *   - 0 when the object is LS_UNLOADED  (ETS checks for 0 before download)
 *   - set to the actual offset after LE_ADDITIONAL_LOAD_CONTROLS sub=0x0B
 *     (Relative Allocation) — ETS reads this back to know where to write
 *   - remains non-zero while LS_LOADED (read-back via KNX_A_MEMORY_READ)
 *
 * The values are managed by load_state_machine_event() in object_interface.c
 * via knx_memory_table_offset().  They start at 0 so that an un-programmed device
 * correctly signals "no table allocated" to ETS.
 *
 * With CONFIG_KNX_AUTO_LOAD_TABLES=y the object files set _load_state to
 * LS_LOADED at boot and also set these to their canonical offsets so that
 * PID_TABLE_REFERENCE already reflects the pre-loaded tables.
 */
/*
 * Initialized to the canonical offset when CONFIG_KNX_AUTO_LOAD_TABLES=y
 * (tables start LS_LOADED at boot), or to 0 otherwise (LS_UNLOADED).
 * load_state_machine_event() keeps these in sync with the load state.
 */
uint32_t table_reference_addresses =
	IS_ENABLED(CONFIG_KNX_AUTO_LOAD_TABLES)
		? __builtin_bswap32(offsetof(struct knx_memory, addresses))
		: 0U;
uint32_t table_reference_associations =
	IS_ENABLED(CONFIG_KNX_AUTO_LOAD_TABLES)
		? __builtin_bswap32(offsetof(struct knx_memory, associations))
		: 0U;
uint32_t table_reference_application_program =
	IS_ENABLED(CONFIG_KNX_AUTO_LOAD_TABLES)
		? __builtin_bswap32(offsetof(struct knx_memory, application_program))
		: 0U;
uint32_t table_reference_group_object =
	IS_ENABLED(CONFIG_KNX_AUTO_LOAD_TABLES)
		? __builtin_bswap32(offsetof(struct knx_memory, group_object))
		: 0U;
uint32_t table_reference_interface_program =
	IS_ENABLED(CONFIG_KNX_AUTO_LOAD_TABLES)
		? __builtin_bswap32(offsetof(struct knx_memory, interface_program))
		: 0U;

/*
 * Canonical big-endian offsets — used by load_state_machine_event() and
 * by the auto-load paths in the individual object files.
 */
const uint32_t k_table_ref_addresses = __builtin_bswap32(offsetof(struct knx_memory, addresses));
const uint32_t k_table_ref_associations =
	__builtin_bswap32(offsetof(struct knx_memory, associations));
const uint32_t k_table_ref_application_program =
	__builtin_bswap32(offsetof(struct knx_memory, application_program));
const uint32_t k_table_ref_group_object =
	__builtin_bswap32(offsetof(struct knx_memory, group_object));
const uint32_t k_table_ref_interface_program =
	__builtin_bswap32(offsetof(struct knx_memory, interface_program));

uint32_t knx_memory_table_offset(uint8_t object_index)
{
	/*
	 * Maps interface object index → byte offset of its download table
	 * within struct knx_memory.  Must stay in sync with object_interfaces[]
	 * in object_interface.c.
	 *
	 * Index 0 = Device Object   — no download table
	 * Index 1 = Address Table
	 * Index 2 = Association Table
	 * Index 3 = Group Object Table
	 * Index 4 = Application Program
	 * Index 5 = Interfaceprogram Object ("Application Program 2")
	 */
	static const uint32_t offsets[] = {
		0,
		offsetof(struct knx_memory, addresses),
		offsetof(struct knx_memory, associations),
		offsetof(struct knx_memory, group_object),
		offsetof(struct knx_memory, application_program),
		offsetof(struct knx_memory, interface_program),
	};

	if (object_index < ARRAY_SIZE(offsets)) {
		return offsets[object_index];
	}
	return 0;
}

uint32_t knx_memory_table_capacity(uint8_t object_index)
{
	/*
	 * Usable size in octets of each object's download segment — the upper
	 * bound ETS may request via load control "Data Relative Allocation"
	 * (KNX spec 3/5/3 §3.5.1.2).  A request larger than this must be
	 * refused (PID_TABLE_REFERENCE = 0, Load State = Error) rather than
	 * honoured, or the fill operation writes past the segment.
	 *
	 * Must stay in sync with knx_memory_table_offset() above.
	 */
	static const uint32_t sizes[] = {
		0,
		sizeof(knx_mem.addresses),
		sizeof(knx_mem.associations),
		sizeof(knx_mem.group_object),
		sizeof(knx_mem.application_program),
		sizeof(knx_mem.interface_program),
	};

	if (object_index < ARRAY_SIZE(sizes)) {
		return sizes[object_index];
	}
	return 0;
}

/* -------- Public API -------- */

/*
 * CONFIG_KNX_AUTO_LOAD_TABLES: applied by knx_memory_reset(), i.e. whenever
 * knx_mem has no history to restore — first boot, an incompatible/corrupt
 * NVS blob, a KNX_MEMORY_LAYOUT_VERSION mismatch (knx_mem_set() below), or
 * an explicit device_factory_format(). Once a real Load State has been
 * persisted, knx_memory_read() restores it and this default is never consulted
 * again — this is a "first-boot default" now, decided 2026-08-22: keep the
 * name and the default (y), just document the narrower scope.
 */
static void apply_auto_load_tables_default(void)
{
	if (IS_ENABLED(CONFIG_KNX_AUTO_LOAD_TABLES)) {
		knx_mem.load_state[KNX_OBJ_IDX_ADDRESS_TABLE] = LS_LOADED;
		knx_mem.load_state[KNX_OBJ_IDX_ASSOCIATION_TABLE] = LS_LOADED;
		knx_mem.load_state[KNX_OBJ_IDX_GROUP_OBJ_TABLE] = LS_LOADED;
		knx_mem.load_state[KNX_OBJ_IDX_APPLICATION_PROG] = LS_LOADED;
		knx_mem.load_state[KNX_OBJ_IDX_INTERFACE_PROG] = LS_LOADED;
	}
}

void knx_memory_reset(void)
{
	memset(&knx_mem, 0x00, sizeof(knx_mem));
	knx_mem.routing_count = 6;
	/* KNX spec §14.2: default S-mode IA = 0xFFFF (unregistered).
	 * ETS only accepts KNX_A_INDIVIDUAL_ADDRESS_RESPONSE from SA=0xFFFF;
	 * SA=0.0.0 is ignored.
	 */
	knx_mem.subnet_addr = 0xFFu;
	knx_mem.device_addr = 0xFFu;
	knx_mem.start[0] = 0xA5;
	knx_mem.start[1] = 0xAD;
	knx_mem.start[2] = 0xAF;
	knx_mem.start[3] = 0xFE;
	knx_mem.memory_layout_version = KNX_MEMORY_LAYOUT_VERSION;

	apply_auto_load_tables_default();
}

#if defined(CONFIG_SETTINGS)

/* -------- Debounce work item (settings backend) -------- */

static struct k_work_delayable s_flush_work;

static void flush_work_fn(struct k_work *w)
{
	ARG_UNUSED(w);
	int ret = settings_save_one(KNX_SETTINGS_KEY, &knx_mem, sizeof(knx_mem));

	if (ret != 0) {
		LOG_ERR("knx_memory_write: settings_save_one failed: %d", ret);
	} else {
		LOG_DBG("knx_memory_write: saved %zu bytes", sizeof(knx_mem));
	}
}

static int knx_mem_set(const char *key, size_t len, settings_read_cb read_cb, void *cb_arg)
{
	ARG_UNUSED(key);

	/*
	 * KNX_MEMORY_LAYOUT_VERSION's own comment: any layout
	 * change — including one that happens to leave sizeof(knx_mem)
	 * unchanged — is handled by wiping and re-commissioning, not by a
	 * byte-level migration. A length mismatch alone is grounds to reject
	 * the blob outright, before even looking at its content.
	 */
	if (len != sizeof(knx_mem)) {
		LOG_WRN("knx_memory_read: stored blob is %zu bytes, this firmware's "
			"struct knx_memory is %zu — reinitialising",
			len, sizeof(knx_mem));
		return -EINVAL;
	}

	int rc = read_cb(cb_arg, &knx_mem, len);

	if (rc < 0) {
		LOG_ERR("knx_memory_read: read_cb failed: %d", rc);
		return rc;
	}

	if (knx_mem.memory_layout_version != KNX_MEMORY_LAYOUT_VERSION) {
		LOG_WRN("knx_memory_read: stored layout version %u != current %u "
			"(struct knx_memory's shape changed) — reinitialising",
			knx_mem.memory_layout_version, KNX_MEMORY_LAYOUT_VERSION);
		return -EINVAL;
	}

	LOG_DBG("knx_memory_read: loaded %zu bytes (layout version %u)", len,
		KNX_MEMORY_LAYOUT_VERSION);
	return 0;
}

static struct settings_handler s_knx_settings = {
	.name = "knx",
	.h_set = knx_mem_set,
};

bool knx_memory_read(void)
{
	int ret;
	bool restored = true;

	knx_memory_reset();

	ret = settings_subsys_init();
	if (ret != 0) {
		LOG_ERR("%s: settings_subsys_init failed: %d", __func__, ret);
		return false;
	}

	ret = settings_register(&s_knx_settings);
	if (ret != 0 && ret != -EEXIST) {
		LOG_ERR("%s: settings_register failed: %d", __func__, ret);
		return false;
	}

	ret = settings_load_subtree(KNX_SETTINGS_KEY);
	if (ret != 0) {
		/* Either no stored data at all (first boot), or knx_mem_set()
		 * rejected it (wrong length / KNX_MEMORY_LAYOUT_VERSION) and
		 * already logged why. Either way, defaults it is.
		 */
		LOG_WRN("%s: nothing usable restored, using defaults", __func__);
		knx_memory_reset();
		restored = false;
	}

	/* Validate magic header */
	if (knx_mem.start[0] != 0xA5 || knx_mem.start[1] != 0xAD || knx_mem.start[2] != 0xAF ||
	    knx_mem.start[3] != 0xFE) {
		LOG_WRN("%s: invalid magic, reinitialising", __func__);
		knx_memory_reset();
		restored = false;
	}

	/*
	 * KNX spec: PID_DEVICE_CONTROL (user-stopped, IA-duplication,
	 * Verify Mode, Safe State bits) must reset to 0x00 on every
	 * start-up, even though the rest of knx_mem is persisted NVS
	 * state restored above.
	 */
	knx_mem.device_control = 0x00u;

	k_work_init_delayable(&s_flush_work, flush_work_fn);
	return restored;
}

void knx_memory_modified(void)
{
	k_work_reschedule(&s_flush_work, K_MSEC(CONFIG_KNX_SETTINGS_DEBOUNCE_MS));
}

void knx_memory_write(void)
{
	k_work_cancel_delayable(&s_flush_work);
	flush_work_fn(NULL);
}

#else /* !CONFIG_SETTINGS */

bool knx_memory_read(void)
{
	LOG_WRN("%s: CONFIG_SETTINGS not enabled, using defaults", __func__);
	knx_memory_reset();
	return false;
}

void knx_memory_modified(void)
{
	/* No-op without settings backend. */
}

void knx_memory_write(void)
{
	/* No-op without settings backend. */
	LOG_WRN("%s: CONFIG_SETTINGS not enabled, changes are volatile", __func__);
}

#endif /* CONFIG_SETTINGS */

bool knx_memory_user_write(uint32_t offset, size_t len, uint8_t *data)
{
	if (offset + len > sizeof(knx_mem)) {
		LOG_ERR("%s(0x%08x, %zu): outside memory range", __func__, offset, len);
		return false;
	}
	memcpy(((uint8_t *)&knx_mem) + offset, data, len);
	knx_memory_modified();
	return true;
}
