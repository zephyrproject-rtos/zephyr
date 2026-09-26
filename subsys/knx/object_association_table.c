/*
 * Copyright (c) 2026 Fabien Proriol
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>
#include <stddef.h>
#include "object_association_table.h"
#include "object_interface.h"
#include "object_memory.h"

static knx_u16_be_t _object_type = KNX_U16_BE(OT_ASSOC_TABLE);
KNX_PROP_ASSERT_SIZE(PDT_UNSIGNED_INT, _object_type);
/* PID_LOAD_STATE_CONTROL storage is __memory.load_state[OBJ_IDX_ASSOCIATION_TABLE]
 * — persisted in NVS, not a RAM-only static.
 */
KNX_PROP_ASSERT_SIZE(PDT_CONTROL, __memory.load_state[OBJ_IDX_ASSOCIATION_TABLE]);
static ErrorCode _error_code = E_NO_FAULT;
KNX_PROP_ASSERT_SIZE(PDT_ENUM8, _error_code);
static char _name[6] = "GrOAT";
KNX_PROP_ASSERT_SIZE(PDT_UNSIGNED_LONG, table_reference_associations);

struct property object_association_table_properties[] = {
	KNX_PROP_SCALAR(PID_OBJECT_TYPE, PDT_UNSIGNED_INT, false, ReadLv3 | WriteLv0,
			&_object_type),
	KNX_PROP_CHARS(PID_OBJECT_NAME, false, ReadLv3 | WriteLv0, _name),
	KNX_PROP_SCALAR(PID_LOAD_STATE_CONTROL, PDT_CONTROL, true, ReadLv3 | WriteLv3,
			&__memory.load_state[OBJ_IDX_ASSOCIATION_TABLE]),
	KNX_PROP_SCALAR(PID_ERROR_CODE, PDT_ENUM8, false, ReadLv3 | WriteLv0, &_error_code),
	KNX_PROP_SCALAR(PID_TABLE_REFERENCE, PDT_UNSIGNED_LONG, false, ReadLv3 | WriteLv0,
			&table_reference_associations),
	/*
	 * PDT_GENERIC_04 and max_nr_of_elem = MAX_NUMBER_OF_ASSOCIATIONS were
	 * already correct — only the explicit count_is_be = true
	 * is new here, replacing the implicit PDT_GENERIC_04-keyed check
	 * interface_read_property()/interface_write_property() used to apply.
	 *
	 * Not fixed, and out of scope for §2.11: the generic array-element path
	 * in those two functions indexes this property as
	 * data + property_data_size(pdt) * start_index, i.e. one PDT_GENERIC_04
	 * (4-octet) slot per 1-based start_index. That is correct for the
	 * Address/Group Object tables (one slot per entry, fixed alongside this
	 * one) but not for this table: entry 0 occupies associations[1..2], so
	 * start_index=1 lands on associations[2..3], not associations[1..2] —
	 * off by one TSAP/ASAP slot. Never exercised in practice, same reason
	 * §2.11's main fix wasn't needed for correctness until now: ETS drives
	 * System B through PID_TABLE_REFERENCE + A_Memory_Write, not through
	 * A_PropertyValue_Read/_Write on PID_TABLE.
	 */
	KNX_PROP_ARRAY(PID_TABLE, PDT_GENERIC_04, false, ReadLv3 | WriteLv0,
		       MAX_NUMBER_OF_ASSOCIATIONS, &__memory.associations[0], __memory.associations,
		       true),
	{.description = {.property_id = PID_LAST}}};

/*
 * KNX spec 3/5/1 §4.17.5.2.5, "PID_TABLE for the Group Object Association
 * Table — format 1" (this Interface Object declares PDT_GENERIC_04 for
 * PID_TABLE above, i.e. format 1): index 0 = entry count, each entry
 * occupies TWO uint16 slots [2*idx+1]=TSAP, [2*idx+2]=ASAP — NOT one byte
 * each packed into a single uint16 (that would be format 0, PDT_GENERIC_02).
 * Confirmed against the reference libknx and Thelsing knx-stack, which both
 * declare PDT_GENERIC_04 and use this exact 2-uint16-per-entry layout.
 * All fields are stored by ETS in KNX wire (big-endian) order.
 */
static uint16_t entry_count(void)
{
	return ntohs(__memory.associations[0]);
}

static uint16_t get_TSAP(uint16_t idx)
{
	if (idx >= entry_count()) {
		return 0;
	}

	return ntohs(__memory.associations[2 * idx + 1]);
}

static uint16_t get_ASAP(uint16_t idx)
{
	if (idx >= entry_count()) {
		return 0;
	}

	return ntohs(__memory.associations[2 * idx + 2]);
}

/*
 * KNX spec 3/5/1 §4.17.5.2.5: "If the current length is set to 0 a
 * 'standard Association Table' shall be used (this is: ASAP = TSAP
 * number)." — a 1:1 identity mapping, not "no association at all".
 */
int32_t association_table_next_asap(uint16_t tsap, uint16_t *startIdx)
{
	uint16_t entries = entry_count();

	if (entries == 0) {
		if (*startIdx == 0) {
			*startIdx = 1;
			return tsap;
		}
		return -1;
	}

	for (uint16_t i = *startIdx; i < entries; i++) {
		*startIdx = i + 1;

		if (get_TSAP(i) == tsap) {
			return get_ASAP(i);
		}
	}
	return -1;
}

/* return type is int32 so that we can return uint16 and -1 */
int32_t association_table_translate_asap(uint16_t asap)
{
	uint16_t entries = entry_count();

	if (entries == 0) {
		return asap;
	}

	for (uint16_t i = 0; i < entries; i++) {
		if (get_ASAP(i) == asap) {
			return get_TSAP(i);
		}
	}
	return -1;
}

void object_association_table_properties_written(PropertyID id)
{
}
