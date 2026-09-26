/*
 * Copyright (c) 2026 Fabien Proriol
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>
#include <stddef.h>
#include "object_address_table.h"
#include "object_interface.h"
#include "object_memory.h"

static knx_u16_be_t _object_type = KNX_U16_BE(OT_ADDR_TABLE);
KNX_PROP_ASSERT_SIZE(PDT_UNSIGNED_INT, _object_type);
/* PID_LOAD_STATE_CONTROL storage is __memory.load_state[OBJ_IDX_ADDRESS_TABLE]
 * — persisted in NVS, not a RAM-only static.
 */
KNX_PROP_ASSERT_SIZE(PDT_CONTROL, __memory.load_state[OBJ_IDX_ADDRESS_TABLE]);
static ErrorCode _error_code = E_NO_FAULT;
KNX_PROP_ASSERT_SIZE(PDT_ENUM8, _error_code);
static char _name[5] = "GrAT";
KNX_PROP_ASSERT_SIZE(PDT_UNSIGNED_LONG, table_reference_addresses);

/*
 * PID_TABLE — Address Table (KNX spec 3/5/1 §4.16.8, p.237): the real
 * per-entry layout is PDT_UNSIGNED_INT (one uint16 slot per group address),
 * and max_nr_of_elem is the entry capacity, MAX_NUMBER_OF_ADDRESS_GROUP —
 * both fixed here (previously PDT_GENERIC_04 / 1, which told
 * a client reading this property that the table held a single 4-octet
 * entry). It worked only because ETS drives System B through the
 * memory-mapped path (PID_TABLE_REFERENCE + A_Memory_Write), never through
 * this property directly. current_length (addresses[0]) is still wire-order
 * (count_is_be = true) — that part was already correct.
 */
struct property address_table_properties[] = {
	KNX_PROP_SCALAR(PID_OBJECT_TYPE, PDT_UNSIGNED_INT, false, ReadLv3 | WriteLv0,
			&_object_type),
	KNX_PROP_CHARS(PID_OBJECT_NAME, false, ReadLv3 | WriteLv0, _name),
	KNX_PROP_SCALAR(PID_LOAD_STATE_CONTROL, PDT_CONTROL, true, ReadLv3 | WriteLv3,
			&__memory.load_state[OBJ_IDX_ADDRESS_TABLE]),
	KNX_PROP_SCALAR(PID_ERROR_CODE, PDT_ENUM8, false, ReadLv3 | WriteLv0, &_error_code),
	KNX_PROP_SCALAR(PID_TABLE_REFERENCE, PDT_UNSIGNED_LONG, false, ReadLv3 | WriteLv0,
			&table_reference_addresses),
	KNX_PROP_ARRAY(PID_TABLE, PDT_UNSIGNED_INT, false, ReadLv3 | WriteLv0,
		       MAX_NUMBER_OF_ADDRESS_GROUP, &__memory.addresses[0], __memory.addresses,
		       true),
	{.description = {.property_id = PID_LAST}}};

/*
 * The entry count at addresses[0] is stored by ETS in KNX wire (big-endian)
 * byte order, same as the entries — confirmed against the reference libknx
 * stack's entry_count()=ntohs(_groupAddresses[0]). Reading it natively
 * turns a real count of e.g. 9 into 9<<8=2304.
 */
uint16_t address_table_get_group_address(uint16_t tsap)
{
	if (__memory.load_state[OBJ_IDX_ADDRESS_TABLE] != LS_LOADED ||
	    tsap > ntohs(__memory.addresses[0])) {
		return 0;
	}

	return ntohs(__memory.addresses[tsap]);
}

uint16_t address_table_get_tsap(uint16_t addr)
{
	uint16_t n = ntohs(__memory.addresses[0]);

	for (uint16_t i = 1; i <= n; i++) {
		if (ntohs(__memory.addresses[i]) == addr) {
			return i;
		}
	}
	return 0xFFFF;
}

/*
 * KNX spec 3/5/1 §4.16.8.2.4: "If the current length is 0 all Group
 * Addresses shall be accepted." (To accept none, the Load State must be
 * set away from LS_LOADED — current_length==0 while Loaded is a valid
 * "accept everything" state, not "accept nothing".)
 */
bool address_table_contains(uint16_t addr)
{
	uint16_t n = ntohs(__memory.addresses[0]);

	if (n == 0) {
		return true;
	}

	for (uint16_t i = 1; i <= n; i++) {
		if (ntohs(__memory.addresses[i]) == addr) {
			return true;
		}
	}
	return false;
}

void address_table_properties_written(PropertyID id)
{
}
