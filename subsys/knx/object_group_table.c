/*
 * Copyright (c) 2026 Fabien Proriol
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>
#include <stddef.h>
#include "object_group_table.h"
#include "object_interface.h"
#include "object_memory.h"
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(knx_grp_tbl, CONFIG_KNX_STACK_LOG_LEVEL);

/*
 * Group Object values, RAM-only — decided volatile, not
 * persisted. Previously lived inside __memory (hence NVS-persisted) but
 * nothing ever triggered a save for it, so a value survived a reboot only
 * by accident, if an unrelated write happened to flush the blob first.
 * A power cycle now wipes these values by design: this device is a sensor
 * board, and the Group Object 'I' flag (group_object_read_on_init_trigger(),
 * knx_group_object.c) already re-reads current state from the bus at boot
 * instead of trusting a stale cache. Never addressed via PID_TABLE_REFERENCE
 * or A_Memory_Write — memory_table_offset()/_capacity() for
 * OBJ_IDX_GROUP_OBJ_TABLE only ever covered group_object[], the descriptor
 * table, not this value storage.
 */
static uint8_t s_group_object_data[KNX_GROUP_OBJECTS_DATA_SIZE];

static knx_u16_be_t _object_type = KNX_U16_BE(OT_GRP_OBJ_TABLE);
KNX_PROP_ASSERT_SIZE(PDT_UNSIGNED_INT, _object_type);
/* PID_LOAD_STATE_CONTROL storage is __memory.load_state[OBJ_IDX_GROUP_OBJ_TABLE]
 * — persisted in NVS, not a RAM-only static.
 */
KNX_PROP_ASSERT_SIZE(PDT_CONTROL, __memory.load_state[OBJ_IDX_GROUP_OBJ_TABLE]);
static ErrorCode _error_code = E_NO_FAULT;
KNX_PROP_ASSERT_SIZE(PDT_ENUM8, _error_code);
static char _name[13] = "GroupObjects";
KNX_PROP_ASSERT_SIZE(PDT_UNSIGNED_LONG, table_reference_group_object);

/* Indexed 1..GROUP_OBJECT_COUNT (asap is 1-based, per KNX convention) → needs +1. */
static group_object_updated_handler _group_object_updated_callback[GROUP_OBJECT_COUNT + 1] = {
	0,
};
static group_object_read_handler _group_object_read_handler[GROUP_OBJECT_COUNT + 1] = {
	0,
};

/*
 * PID_TABLE — Group Object Table (KNX spec 3/5/1 §4.18.6, p.269): the real
 * per-entry layout is PDT_GENERIC_02 ("Format 1", one uint16 descriptor per
 * ASAP), and max_nr_of_elem is the entry capacity, GROUP_OBJECT_COUNT — both
 * fixed here (previously PDT_GENERIC_04 / 1, same class of
 * mistake as the Address Table's, see the historical note there). Harmless
 * in practice because ETS drives System B through the memory-mapped path.
 * current_length (group_object[0]) is still wire-order (count_is_be = true).
 */
struct property object_group_device_properties[] = {
	KNX_PROP_SCALAR(PID_OBJECT_TYPE, PDT_UNSIGNED_INT, false, ReadLv3 | WriteLv0,
			&_object_type),
	KNX_PROP_CHARS(PID_OBJECT_NAME, false, ReadLv3 | WriteLv0, _name),
	KNX_PROP_SCALAR(PID_LOAD_STATE_CONTROL, PDT_CONTROL, true, ReadLv3 | WriteLv3,
			&__memory.load_state[OBJ_IDX_GROUP_OBJ_TABLE]),
	KNX_PROP_SCALAR(PID_ERROR_CODE, PDT_ENUM8, false, ReadLv3 | WriteLv0, &_error_code),
	KNX_PROP_SCALAR(PID_TABLE_REFERENCE, PDT_UNSIGNED_LONG, false, ReadLv3 | WriteLv0,
			&table_reference_group_object),
	KNX_PROP_ARRAY(PID_TABLE, PDT_GENERIC_02, false, ReadLv3 | WriteLv0, GROUP_OBJECT_COUNT,
		       &__memory.group_object[0], __memory.group_object, true),
	{.description = {.property_id = PID_LAST}}};

/*
 * Group Object descriptors and the entry count are stored by ETS in KNX wire
 * (big-endian) byte order — same convention as Address/Association Table
 * entries (confirmed against the reference libknx stack's
 * group_object_table_entry_count()/ntohs(group_object_table_data()) and
 * object_address_table.c's entry_count()=ntohs(_groupAddresses[0])).
 * Reading __memory.group_object[] natively without ntohs() swaps the flag
 * byte and the size byte, and misreads the entry count.
 *
 * Valid indices are 1..count inclusive (index 0 holds the count) — the
 * previous "asap >= count" bound excluded the last valid entry.
 */
uint8_t group_object_flag(uint16_t asap)
{
	if (asap == 0 || asap > ntohs(__memory.group_object[0])) {
		return 0;
	}
	return ntohs(__memory.group_object[asap]) >> 8;
}

uint8_t group_object_size(uint16_t asap)
{
	if (asap == 0 || asap > ntohs(__memory.group_object[0])) {
		return 0;
	}
	switch (ntohs(__memory.group_object[asap]) & 0xFF) {
	case 0: /* 1 bit */
	case 1: /* 2 bit */
	case 2: /* 3 bit */
	case 3: /* 4 bit */
	case 4: /* 5 bit */
	case 5: /* 6 bit */
	case 6: /* 7 bit */
	case 7: /* 1 octet */
		return 1;
	case 8: /* 2 octets */
		return 2;
	case 9: /* 3 octets */
		return 3;
	case 10: /* 4 octets */
		return 4;
	case 11: /* 6 octets */
		return 6;
	case 12: /* 8 octets */
		return 8;
	case 13: /* 10 octets */
		return 10;
	case 14: /* 14 octets */
		return 14;
	case 15: /* 5 octets */
		return 5;
	case 16: /* 7 octets */
		return 7;
	case 17: /* 9 octets */
		return 9;
	case 18: /* 11 octets */
		return 11;
	case 19: /* 12 octets */
		return 12;
	case 20: /* 13 octets */
		return 13;
	default:
		return (ntohs(__memory.group_object[asap]) & 0xFF) - 6;
	}
}

/*
 * Returns true if the group object uses the KNX GroupValue short form:
 * type codes 0-5 = 1..6 bit data, encoded in APCI_lo bits[5:0].
 * Type code 6 = 7-bit (long form), type code 7+ = ≥1 byte (long form).
 */
bool group_object_is_short_form(uint16_t asap)
{
	if (asap == 0 || asap > ntohs(__memory.group_object[0])) {
		return false;
	}
	return (ntohs(__memory.group_object[asap]) & 0xFF) < 6;
}

uint16_t group_object_count(void)
{
	return ntohs(__memory.group_object[0]);
}

bool group_object_table_is_loaded(void)
{
	return __memory.load_state[OBJ_IDX_GROUP_OBJ_TABLE] == LS_LOADED;
}

uint8_t *group_object_data(uint16_t asap)
{
	uint8_t *data = s_group_object_data;

	if (asap == 0 || asap > ntohs(__memory.group_object[0])) {
		return NULL;
	}

	for (int i = 1; i < asap; i++) {
		data += group_object_size(i);
	}
	return data;
}

/* Wipes every Group Object's cached value — the RAM counterpart of erasing
 * __memory.group_object_data before this field was removed.
 * Called from A_Restart__ind's Master Reset paths (layer7_application.c).
 */
void group_object_data_reset(void)
{
	memset(s_group_object_data, 0, sizeof(s_group_object_data));
}

void group_object_updated(uint8_t asap)
{
	if (_group_object_updated_callback[asap] != NULL) {
		_group_object_updated_callback[asap](asap);
	}
}

void group_object_set_updated_callback(uint8_t asap, group_object_updated_handler callback)
{
	if (_group_object_updated_callback[asap] != NULL) {
		LOG_WRN("Override updated callback for ASAP %u", asap);
	}
	_group_object_updated_callback[asap] = callback;
}

void group_object_set_read_handler(uint8_t asap, group_object_read_handler handler)
{
	if (_group_object_read_handler[asap] != NULL) {
		LOG_WRN("Override read handler for ASAP %u", asap);
	}
	_group_object_read_handler[asap] = handler;
}

void group_object_read_requested(uint8_t asap)
{
	if (_group_object_read_handler[asap] != NULL) {
		bool updated = _group_object_read_handler[asap](asap);

		LOG_DBG("read handler for ASAP %u %s the value", asap,
			updated ? "updated" : "left unchanged");
	}
}

/*
 * Group Object 'I' flag (read-on-init). Every PID_LOAD_STATE_
 * CONTROL write reaches here regardless of the event, so gate on the state
 * actually being LS_LOADED afterwards rather than on which event was sent —
 * matches the reference stack's GroupObjectTableObject::loadEvent(), which
 * re-derives read-on-init the same way on every transition INTO Loaded,
 * including a redundant Load Completed while already Loaded.
 */
void object_group_table_properties_written(PropertyID id)
{
	if (id != PID_LOAD_STATE_CONTROL) {
		return;
	}
	if (__memory.load_state[OBJ_IDX_GROUP_OBJ_TABLE] != LS_LOADED) {
		return;
	}
	group_object_read_on_init_trigger();
}
