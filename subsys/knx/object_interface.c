/*
 * Copyright (c) 2026 Fabien Proriol
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>
#include "object_interface.h"
#include "object_memory.h"
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(knx_iface, CONFIG_KNX_STACK_LOG_LEVEL);

/* KNX_TEST_INTERFACE_TABLES: defined by unit tests that provide their own
 * object_interfaces / object_interfaces_types / properties_written tables.
 * Never set in production builds.
 */
#ifndef KNX_TEST_INTERFACE_TABLES

extern struct property object_device_properties[];
extern struct property address_table_properties[];
extern struct property object_association_table_properties[];
extern struct property object_application_program_properties[];
extern struct property object_group_device_properties[];
extern struct property object_interface_program_properties[];

#include "object_group_table.h"

/*
 * PID_IO_LIST — the Interface Object types this device implements, in object
 * index order.  Stored in KNX wire order: interface_read_property() copies the
 * bytes straight out, so a native uint16_t array served OT_ADDR_TABLE (1) as
 * 0x0100 = 256 and OT_GRP_OBJ_TABLE (9) as 0x0900 = 2304.
 *
 * Element 0 mirrors the entry count, which is how the downloadable tables in
 * struct Memory are laid out; the count ETS actually reads comes from
 * `nr_of_elem` (_nb_interface in object_device.c), so elements 1..N are the
 * types.  The array is therefore N+1 long and index N is the last valid one.
 */
knx_u16_be_t object_interfaces_types[NUMBER_OF_INTERFACE + 1] = {
	KNX_U16_BE(NUMBER_OF_INTERFACE), KNX_U16_BE(OT_DEVICE),
	KNX_U16_BE(OT_ADDR_TABLE),       KNX_U16_BE(OT_ASSOC_TABLE),
	KNX_U16_BE(OT_GRP_OBJ_TABLE),    KNX_U16_BE(OT_APPLICATION_PROG),
	KNX_U16_BE(OT_INTERFACE_PROG)};

struct property *object_interfaces[NUMBER_OF_INTERFACE] = {object_device_properties,
							   address_table_properties,
							   object_association_table_properties,
							   object_group_device_properties,
							   object_application_program_properties,
							   object_interface_program_properties};

void (*properties_written[NUMBER_OF_INTERFACE])(PropertyID id) = {
	object_device_properties_written,              /* OT_DEVICE        index 0 */
	address_table_properties_written,              /* OT_ADDR_TABLE    index 1 */
	object_association_table_properties_written,   /* OT_ASSOC_TABLE   index 2 */
	object_group_table_properties_written,         /* OT_GRP_OBJ_TABLE index 3 */
	object_application_program_properties_written, /* OT_APPLICATION_PROG index 4 */
	object_interface_program_properties_written,   /* OT_INTERFACE_PROG   index 5 */
};

#endif /* KNX_TEST_INTERFACE_TABLES */

/*
 * Current access level of the management client, 0 (most privileged) to 3.
 *
 * KNX spec 3/3/7 §3.5.7: the level is established per connection by
 * A_Authorize_Request and reverts to the default when the connection closes.
 * Volume 6 §4.2 row 12 requires Authorization with 4 access levels for
 * System B, and footnote 10 permits a device with no protected areas to grant
 * level 0 for any key — which is what CONFIG_KNX_AUTHORIZE_GRANT_ALL does.
 *
 * The default is the LEAST privileged level, so an unauthenticated client can
 * still write the properties declared WriteLv3 (PID_PROGMODE,
 * PID_LOAD_STATE_CONTROL, PID_DEVICE_CONTROL…) but not the WriteLv0 ones.
 */
#define KNX_ACCESS_LEVEL_DEFAULT 3u

static uint8_t s_access_level = KNX_ACCESS_LEVEL_DEFAULT;

void interface_set_access_level(uint8_t level)
{
	s_access_level = (level > KNX_ACCESS_LEVEL_DEFAULT) ? KNX_ACCESS_LEVEL_DEFAULT : level;
	LOG_DBG("access level now %u", s_access_level);
}

void interface_reset_access_level(void)
{
	if (s_access_level != KNX_ACCESS_LEVEL_DEFAULT) {
		LOG_DBG("access level reset to %u on disconnect", KNX_ACCESS_LEVEL_DEFAULT);
	}
	s_access_level = KNX_ACCESS_LEVEL_DEFAULT;
}

uint8_t interface_access_level(void)
{
	return s_access_level;
}

/*
 * KNX_PDT_SIZE() (object_property_types.h) is the one
 * canonical "PDT -> storage octets" table, shared with the
 * KNX_PROP_ASSERT_SIZE() compile-time checks in the object_*.c property
 * tables. The only case it does not cover is PDT_CONTROL's write-PDU
 * length: 10 octets is the Additional Load Controls PDU size (KNX spec
 * 3/5/1 §4.23.2), a protocol detail of the write path, not the 1-octet
 * LoadState/RunState storage width KNX_PDT_SIZE() reports.
 *
 * 0 means "no fixed element size" — variable-length types
 * (PDT_VARIABLE_LENGTH, PDT_UTF8), PDT_FUNCTION, PDT_ESCAPE and anything
 * unknown.  Callers MUST treat 0 as "cannot serve this property" and bail
 * out before dividing by it.
 */
static uint8_t property_data_size(PropertyDataType type, bool write)
{
	if (write && type == PDT_CONTROL) {
		return 10;
	}
	return KNX_PDT_SIZE(type);
}

static struct property *get_property(uint8_t object_index, uint8_t property_id,
				     uint8_t property_index, uint8_t *property_index_found)
{
	if (object_index >= NUMBER_OF_INTERFACE) {
		return NULL;
	}

	int index = 0;
	struct property *object = object_interfaces[object_index];

	if (object == NULL) {
		return NULL;
	}

	if (property_id != 0) {
		while (object[index].description.property_id != PID_LAST) {
			if (object[index].description.property_id == property_id) {
				if (property_index == 0) {
					if (property_index_found) {
						*property_index_found = index;
					}
					return &object[index];
				}
				property_index--;
			}
			index++;
		}
	} else {
		while (object[index].description.property_id != PID_LAST) {
			if (index == property_index) {
				if (property_index_found) {
					*property_index_found = index;
				}
				return &object[index];
			}
			index++;
		}
	}

	return NULL;
}

/*
 * sync_table_reference — keep PID_TABLE_REFERENCE consistent with the Load
 * State, which is what the standard actually ties it to.
 *
 * KNX spec 3/5/3 §3.5.1.2: a successful allocation sets PID_TABLE_REFERENCE to
 * the segment's base address, and "in any other case" — notably unloaded, or
 * allocation failed — it "shall be zero".  On this device every segment is
 * statically carved out of struct Memory, so allocation cannot fail for a size
 * that fits (an oversized request is rejected into LS_ERROR, see
 * LE_ADDITIONAL_LOAD_CONTROLS).  The reference is therefore a pure function of
 * the state:
 *
 *     LS_UNLOADED, LS_ERROR -> 0
 *     anything else         -> memory_table_offset(object_index)
 *
 * Deriving it this way rather than only setting it from the explicit allocation
 * event is what the reference (Thelsing) stack does — TableObject::readProperty
 * returns 0 for LS_UNLOADED and tableReference() otherwise, so the value is
 * valid as soon as the segment enters LS_LOADING.  That matters: an ETS product
 * whose LdCtrlRelSegment is tagged AppliesTo="full" issues no allocation during
 * a PARTIAL download, so it expects Unload -> Start Loading alone to leave a
 * usable base address.  Clearing on Unload and only restoring on allocation
 * made ETS read 0 there and abort with "writing to a memory block failed".
 *
 * Stored big-endian: PID_TABLE_REFERENCE is PDT_UNSIGNED_LONG on the wire.
 */
/*
 * set_error_code — records the reason for a Load State Machine failure in
 * the object's PID_ERROR_CODE, instead of leaving it forever E_NO_FAULT
 * with only a LOG_ERR to explain what happened on the bench (03_05_01
 * §4.23.2 / Volume 6 §A.2.3.2 p.142). A no-op for objects
 * that declare no PID_ERROR_CODE (the Device Object never calls
 * load_state_machine_event() at all, so this never actually fires for it).
 */
static void set_error_code(uint8_t object_index, ErrorCode code)
{
	struct property *err = get_property(object_index, PID_ERROR_CODE, 0, NULL);

	if (err != NULL && err->value.data != NULL) {
		*(ErrorCode *)err->value.data = code;
	}
}

static void sync_table_reference(uint8_t object_index, LoadState state)
{
	struct property *tref = get_property(object_index, PID_TABLE_REFERENCE, 0, NULL);

	if (tref == NULL || tref->value.data == NULL) {
		return;
	}

	if (state == LS_UNLOADED || state == LS_ERROR) {
		*(uint32_t *)tref->value.data = 0U;
	} else {
		*(uint32_t *)tref->value.data =
			__builtin_bswap32(memory_table_offset(object_index));
	}
}

/*
 * Octets read out of `data` per event, so a short PDU cannot make the state
 * machine act on bytes that were never received.  The Additional Load Control
 * structure is 10 octets in KNX spec 3/5/2 §3.31.3.4, but only data[0..7] are
 * actually used here — bounding on what is read rather than on the nominal
 * length keeps a tool that omits the two reserved octets working.
 */
#define LSM_MIN_EVENT_OCTETS 1u
#define LSM_MIN_ALLOC_OCTETS 8u

bool load_state_machine_event(uint8_t object_index, const uint8_t *data, uint8_t data_length)
{
	struct property *load_state = get_property(object_index, PID_LOAD_STATE_CONTROL, 0, NULL);

	if (load_state == NULL) {
		LOG_ERR("%s: no PID_LOAD_STATE_CONTROL for obj %u", __func__, object_index);
		return false;
	}

	if (data_length < LSM_MIN_EVENT_OCTETS) {
		LOG_WRN("%s obj=%u: empty PDU, no event to apply", __func__, object_index);
		return false;
	}

	LoadState *state = (LoadState *)(load_state->value.data);
	uint8_t event = data[0];

	if (event == LE_ADDITIONAL_LOAD_CONTROLS && data_length < LSM_MIN_ALLOC_OCTETS) {
		/*
		 * Refuse rather than fall into LS_ERROR: the state machine never saw a
		 * valid event, so the segment's state is still whatever it was, and the
		 * caller answers with nr_of_elem = 0.  Driving LS_ERROR here would make
		 * a truncated PDU look like a failed allocation to ETS.
		 */
		LOG_WRN("%s obj=%u: LE_ADDITIONAL_LOAD_CONTROLS needs %u "
			"octets, got %u",
			__func__, object_index, LSM_MIN_ALLOC_OCTETS, data_length);
		return false;
	}

	switch (*state) {
	case LS_UNLOADED:
		switch (event) {
		case LE_NOOP:
		case LE_LOAD_COMPLETED:
		case LE_ADDITIONAL_LOAD_CONTROLS:
			break;
		case LE_UNLOAD:
			/* Already unloaded — nothing to do; the sync below keeps
			 * PID_TABLE_REFERENCE at 0.
			 */
			break;
		case LE_START_LOADING:
			*state = LS_LOADING;
			break;
		default:
			LOG_ERR("LS_UNLOADED: unexpected event %u", event);
			*state = LS_ERROR;
			set_error_code(object_index, E_GOT_UNDEF_LOAD_CMD);
		}
		break;

	case LS_LOADING:
		switch (event) {
		case LE_NOOP:
		case LE_START_LOADING:
			break;
		case LE_LOAD_COMPLETED:
			*state = LS_LOADED;
			break;
		case LE_UNLOAD:
			*state = LS_UNLOADED;
			break;
		case LE_ADDITIONAL_LOAD_CONTROLS: {
			if (data[1] != 0x0B) {
				LOG_ERR("LS_LOADING LE_ADDITIONAL_LOAD_CONTROLS: bad cmd 0x%02x",
					data[1]);
				*state = LS_ERROR;
				set_error_code(object_index, E_GOT_UNDEF_LOAD_CMD);
				break;
			}
			/*
			 * Data Relative Allocation (sub=0x0B) — wire format from
			 * KNX spec 3/5/2 Management Procedures §3.31.3.4 (p.141):
			 *   data[0]    = 0x03 (event)
			 *   data[1]    = 0x0B (subtype)
			 *   data[2..5] = requested memory size (big-endian uint32_t)
			 *   data[6]    = mode, bit 0: 0 = keep contents, 1 = fill
			 *   data[7]    = fill byte
			 *   data[8..9] = reserved
			 *
			 * On success PID_TABLE_REFERENCE becomes the segment's base
			 * address; per 3/5/3 §3.5.1.2 it shall be ZERO in every other
			 * case, and an allocation that does not fit shall take the Load
			 * State Machine to Error.
			 *
			 * The size check is not optional: `size` arrives from the bus as
			 * a full 32-bit value, so an unclamped fill would memset far past
			 * the segment and corrupt the rest of struct Memory.
			 */
			uint32_t size = ((uint32_t)data[2] << 24) | ((uint32_t)data[3] << 16) |
					((uint32_t)data[4] << 8) | (uint32_t)data[5];
			bool fill = (data[6] & 0x01u) != 0u;
			uint8_t fill_byte = data[7];
			uint32_t capacity = memory_table_capacity(object_index);

			if (size > capacity) {
				LOG_ERR("LE_ADDITIONAL_LOAD_CONTROLS obj=%u: requested %u octets "
					"> capacity %u — refusing allocation",
					object_index, (unsigned int)size, (unsigned int)capacity);
				*state = LS_ERROR;
				set_error_code(object_index, E_MAX_TABLE_LENGTH_EXEEDED);
				break;
			}

			if (fill) {
				/*
				 * Fill through the segment base rather than through
				 * PID_TABLE: the Application Program object has no PID_TABLE
				 * property, so the old property-based path silently skipped
				 * it.  memory_table_offset()/_capacity() cover every segment
				 * uniformly.
				 */
				uint8_t *base =
					(uint8_t *)&__memory + memory_table_offset(object_index);

				memset(base, fill_byte, size);
				memory_modified();
			}
			LOG_DBG("LE_ADDITIONAL_LOAD_CONTROLS obj=%u size=%u/%u fill=%d ref=0x%08x",
				object_index, (unsigned int)size, (unsigned int)capacity, fill,
				memory_table_offset(object_index));
			break;
		}
		default:
			LOG_ERR("LS_LOADING: unexpected event %u", event);
			*state = LS_ERROR;
			set_error_code(object_index, E_GOT_UNDEF_LOAD_CMD);
		}
		break;

	case LS_LOADED:
		switch (event) {
		case LE_NOOP:
		case LE_LOAD_COMPLETED:
			break;
		case LE_START_LOADING:
			*state = LS_LOADING;
			break;
		case LE_UNLOAD:
			*state = LS_UNLOADED;
			break;
		case LE_ADDITIONAL_LOAD_CONTROLS:
			LOG_ERR("LS_LOADED LE_ADDITIONAL_LOAD_CONTROLS unexpected");
			*state = LS_ERROR;
			set_error_code(object_index, E_GOT_UNDEF_LOAD_CMD);
			break;
		default:
			LOG_ERR("LS_LOADED: unexpected event %u", event);
			*state = LS_ERROR;
			set_error_code(object_index, E_GOT_UNDEF_LOAD_CMD);
		}
		break;

	case LS_ERROR:
		switch (event) {
		case LE_NOOP:
		case LE_LOAD_COMPLETED:
		case LE_ADDITIONAL_LOAD_CONTROLS:
		case LE_START_LOADING:
			break;
		case LE_UNLOAD:
			*state = LS_UNLOADED;
			break;
		default:
			*state = LS_ERROR;
		}
		break;

	case LS_UNLOADING:
	case LS_LOADCOMPLETING:
		break;
	}

	/* Single place that keeps PID_TABLE_REFERENCE consistent with the state
	 * this event produced — see sync_table_reference().
	 */
	sync_table_reference(object_index, *state);

	/*
	 * Whatever specific fault caused an LS_ERROR was recorded above, at the
	 * point of failure — that is the only place that knows WHICH fault it
	 * was. Clearing PID_ERROR_CODE back to E_NO_FAULT belongs here instead,
	 * unconditionally, the same way sync_table_reference() derives its
	 * value purely from the resulting state: any event that does NOT leave
	 * the object in LS_ERROR means whatever fault used to apply no longer
	 * does (LE_UNLOAD in particular, from either LS_LOADING or LS_ERROR).
	 */
	if (*state != LS_ERROR) {
		set_error_code(object_index, E_NO_FAULT);
	}

	return true;
}

void interface_read_property_description(uint8_t object_index, uint8_t property_id,
					 uint8_t property_index, bool *write_enable,
					 uint8_t *property_datatype, uint16_t *max_nr_of_elem,
					 uint8_t *access, uint8_t *property_index_found)
{
	struct property *object =
		get_property(object_index, property_id, property_index, property_index_found);

	LOG_DBG("obj=%u prop=%u idx=%u %s", object_index, property_id, property_index,
		object ? "found" : "not found");

	if (object) {
		*write_enable = object->description.write_enable;
		*property_datatype = object->description.property_datatype;
		*max_nr_of_elem = object->description.max_nr_of_elem;
		*access = object->description.access;
	}
}

uint8_t interface_read_property(uint8_t object_index, PropertyID property_id, uint32_t start_index,
				uint32_t *count, uint8_t *data, uint8_t max_size)
{
	uint8_t property_index_found;
	struct property *object = get_property(object_index, property_id, 0, &property_index_found);

	LOG_DBG("obj=%u prop=%u start=%u %s", object_index, property_id, start_index,
		object ? "found" : "not found");

	if (!object) {
		return 0;
	}

	uint16_t nb = 1;

	if (object->value.nr_of_elem) {
		nb = *object->value.nr_of_elem;
		if (object->description.count_is_be) {
			/*
			 * The GrAT/GrOAT/GrOT PID_TABLE current_length is stored in
			 * KNX wire (big-endian) byte order, like every other entry
			 * in these tables — see the ntohs() calls in
			 * object_address_table.c / object_association_table.c /
			 * object_group_table.c. Reading it natively on this
			 * little-endian target turns e.g. a real count of 9 into
			 * 9<<8 = 2304.
			 */
			nb = ntohs(nb);
		}
	}
	/*
	 * 03_03_07 §3.4.4.1 (p.63): start_index = 0 requests the property's
	 * current element count, uniformly — including a scalar, whose count
	 * is always 1. This used to be gated on max_nr_of_elem > 1, so reading
	 * a scalar at start_index = 0 fell through to the element path below
	 * and returned the VALUE instead of the count 1. `nb`
	 * already holds the right answer either way: 1 for a genuine scalar
	 * (nr_of_elem == NULL leaves the uint16_t nb = 1 default above), the
	 * array's real current length otherwise.
	 */
	if (start_index == 0) {
		*count = 1;
		data[0] = nb >> 8;
		data[1] = nb & 0xFF;
		return 2;
	}
	if (object->description.max_nr_of_elem == 1 && start_index > 0) {
		start_index--;
	}
	if (start_index <= nb) {
		uint8_t data_size =
			property_data_size(object->description.property_datatype, false);
		uint8_t *start_data;

		if (data_size == 0) {
			/* Variable-length / function / unknown PDT: nothing we can
			 * serialise here, and dividing by data_size below would trap.
			 */
			LOG_WRN("obj=%u prop=%u: unsupported PDT %u", object_index, property_id,
				object->description.property_datatype);
			return 0;
		}
		start_data = (uint8_t *)object->value.data + (data_size * start_index);

		if (*count * data_size > max_size) {
			*count = max_size / data_size;
		}
		for (uint32_t elem = 0; elem < *count; elem++) {
			for (int i = 0; i < data_size; i++) {
				data[(data_size * elem) + i] = start_data[(data_size * elem) + i];
			}
		}
		return (uint8_t)(*count * data_size);
	}

	return 0;
}

uint8_t interface_function_property_command(uint8_t object_index, uint8_t property_id,
					    const uint8_t *data_in, uint8_t data_in_len,
					    uint8_t *data_out, uint8_t max_out)
{
	LOG_DBG("function_property_command obj=%u prop=%u (not implemented)", object_index,
		property_id);
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
	LOG_DBG("function_property_state_read obj=%u prop=%u (not implemented)", object_index,
		property_id);
	ARG_UNUSED(data_in);
	ARG_UNUSED(data_in_len);
	ARG_UNUSED(data_out);
	ARG_UNUSED(max_out);
	return 0;
}

void interface_unload_object(uint8_t object_index)
{
	uint8_t unload[1] = {LE_UNLOAD};

	(void)load_state_machine_event(object_index, unload, sizeof(unload));
}

bool interface_write_property(uint8_t object_index, PropertyID property_id, uint16_t start_index,
			      uint8_t number_of_elements, const uint8_t *data, uint8_t data_length)
{
	uint8_t property_index_found;
	struct property *object = get_property(object_index, property_id, 0, &property_index_found);

	LOG_DBG("obj=%u prop=%u start=%u nelem=%u data_len=%u %s", object_index, property_id,
		start_index, number_of_elements, data_length, object ? "found" : "not found");

	if (!object) {
		return false;
	}

	/*
	 * Access control (KNX spec 3/3/7 §3.4.4, p.65 and 3/5/1 §4.2 "access
	 * levels").  Both checks come BEFORE the PID_LOAD_STATE_CONTROL
	 * short-circuit: a load-control write must be refused for the same reasons
	 * as any other write, and the caller answers a refusal with nr_of_elem = 0
	 * (there is no error code — the zero element count IS the error).
	 *
	 * Until now both fields were published in A_PropertyDescription_Response
	 * and then ignored, so anything on the bus could overwrite
	 * PID_SERIAL_NUMBER, PID_OBJECT_TYPE or PID_MANUFACTURER_ID — all of them
	 * mandatory read-only (m/x) in Volume 6 §A.2.3.
	 */
	if (!object->description.write_enable) {
		LOG_WRN("obj=%u prop=%u: read-only property, write refused", object_index,
			property_id);
		return false;
	}

	/*
	 * Access levels are numbered with 0 as the MOST privileged, so a caller may
	 * write when its level is numerically <= the level the property requires.
	 * The write level lives in the low nibble of `access` (AccessLevel:
	 * WriteLv0..WriteLv3 = 0..3; the read level is the high nibble).
	 */
	{
		uint8_t required = (uint8_t)(object->description.access & 0x0Fu);

		if (s_access_level > required) {
			LOG_WRN("obj=%u prop=%u: access level %u insufficient (needs <= %u)",
				object_index, property_id, s_access_level, required);
			return false;
		}
	}

	if (property_id == PID_LOAD_STATE_CONTROL) {
		if (!load_state_machine_event(object_index, data, data_length)) {
			return false;
		}
		if (properties_written[object_index]) {
			properties_written[object_index](property_id);
		}
		return true;
	}

	/* Scalar properties have nr_of_elem == NULL (= single element).
	 * Array properties have nr_of_elem pointing to the count, stored
	 * big-endian when description.count_is_be (see interface_read_property()).
	 */
	if (object->value.nr_of_elem != NULL) {
		uint16_t nb = *object->value.nr_of_elem;

		if (object->description.count_is_be) {
			nb = ntohs(nb);
		}
		if (start_index > nb) {
			return false;
		}
	}

	uint8_t data_size = property_data_size(object->description.property_datatype, false);

	if (data_size == 0) {
		/* Variable-length / function / unknown PDT — cannot be written
		 * through this generic path (see property_data_size()).
		 */
		LOG_WRN("obj=%u prop=%u: unsupported PDT %u for write", object_index, property_id,
			object->description.property_datatype);
		return false;
	}

	/*
	 * Array backing stores are 1-based by convention in this stack: slot 0
	 * holds the element count and the elements live at 1..n (see
	 * object_interfaces_types[] and __memory.addresses[]).  So a 1-based
	 * start_index indexes the backing store directly, with no -1, and
	 * interface_read_property() does the same.  Scalars ignore start_index —
	 * the read path decrements 1 to 0 there instead.
	 */
	uint8_t *start_data =
		(uint8_t *)object->value.data +
		(data_size * ((object->description.max_nr_of_elem > 1) ? start_index : 0));

	if (number_of_elements > object->description.max_nr_of_elem) {
		number_of_elements = (uint8_t)object->description.max_nr_of_elem;
	}

	/*
	 * Clamping the count alone does not bound the write: start_index is a
	 * 12-bit PDU field and, for an array, indexes the backing store directly.
	 * max_nr_of_elem is the backing store's slot count, so the last slot
	 * touched must stay below it.  Scalars are excluded — they ignore
	 * start_index above, and for them start_index is normally 1.
	 */
	if (object->description.max_nr_of_elem > 1 &&
	    (uint32_t)start_index + number_of_elements > object->description.max_nr_of_elem) {
		LOG_WRN("obj=%u prop=%u: start %u + %u elements exceeds %u slots", object_index,
			property_id, start_index, number_of_elements,
			object->description.max_nr_of_elem);
		return false;
	}

	/*
	 * number_of_elements is an element count, data_length an
	 * octet count; the write needs number_of_elements * data_size octets, and
	 * they must all have been received.  A short PDU is a malformed request,
	 * so refuse it — the caller answers nr_of_elem = 0 (3/3/7 §3.4.4) — rather
	 * than silently writing whatever followed in the packet buffer.
	 */
	if ((uint16_t)number_of_elements * data_size > data_length) {
		LOG_WRN("obj=%u prop=%u: %u elements of %u octets need %u octets, PDU carries %u",
			object_index, property_id, number_of_elements, data_size,
			(unsigned int)((uint16_t)number_of_elements * data_size), data_length);
		return false;
	}

	for (uint8_t elem = 0; elem < number_of_elements; elem++) {
		for (uint8_t i = 0; i < data_size; i++) {
			start_data[(data_size * elem) + i] = data[(data_size * elem) + i];
		}
	}

	/* Notify the object's written callback (e.g. to propagate IA changes). */
	if (object_index < NUMBER_OF_INTERFACE && properties_written[object_index]) {
		properties_written[object_index](property_id);
	}
	return true;
}
