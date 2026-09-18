/*
 * Copyright (c) 2026 Fabien Proriol
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __OBJECT_INTERFACE__
#define __OBJECT_INTERFACE__

#include <stddef.h>
#include "object_property_types.h"

#if !defined(NUMBER_OF_INTERFACE)
#define NUMBER_OF_INTERFACE 6
#endif

/*
 * Interface object indices — must match object_interfaces[] in
 * object_interface.c. The Device Object must stay at index 0 (03_03_08 AIL
 * §6). OBJ_IDX_INTERFACE_PROG is appended last so existing
 * indices, and ETS projects already downloaded against them, are not
 * renumbered.
 */
#define OBJ_IDX_DEVICE            0
#define OBJ_IDX_ADDRESS_TABLE     1
#define OBJ_IDX_ASSOCIATION_TABLE 2
#define OBJ_IDX_GROUP_OBJ_TABLE   3
#define OBJ_IDX_APPLICATION_PROG  4
#define OBJ_IDX_INTERFACE_PROG    5

void object_device_properties_written(PropertyID id);
void address_table_properties_written(PropertyID id);
void object_association_table_properties_written(PropertyID id);
void object_application_program_properties_written(PropertyID id);
void object_interface_program_properties_written(PropertyID id);
/**
 * load_state_machine_event — feed one load event to an object's Load SM.
 *
 * @param data        Event octets, straight from the PDU: data[0] is the
 *                    event, and for LE_ADDITIONAL_LOAD_CONTROLS data[1..]
 *                    carry the sub-command.
 * @param data_length Number of octets actually present in @p data.
 * @return false when @p data_length is too short for the event requested, in
 *         which case the Load State is left untouched (a malformed PDU must
 *         not drive the state machine, and must not be answered as a success).
 *
 * @p data_length is not decoration: the octets come from the bus, and
 * LE_ADDITIONAL_LOAD_CONTROLS reads up to data[7]. Without the bound a
 * 1-octet PDU made the allocation size and fill byte come from whatever
 * followed in the packet buffer.
 */
bool load_state_machine_event(uint8_t object_index, const uint8_t *data, uint8_t data_length);

/**
 * interface_unload_object — drive one object's Load SM back to LS_UNLOADED.
 *
 * Wraps the LE_UNLOAD event so callers that erase a segment's memory do not
 * have to know the load-event encoding.  Also clears PID_TABLE_REFERENCE, via
 * the same sync that every other load event goes through.
 *
 * Erasing a segment WITHOUT this leaves the object reporting LS_LOADED over
 * zeroed tables, which is confirmed in the field to make ETS abort a partial
 * download — it trusts Load State to skip tables it believes are present, then
 * finds no data.
 */
void interface_unload_object(uint8_t object_index);

extern struct property *object_interfaces[NUMBER_OF_INTERFACE];
/* PID_IO_LIST backing store, in KNX wire order — see object_interface.c. */
extern knx_u16_be_t object_interfaces_types[NUMBER_OF_INTERFACE + 1];

#include <zephyr/sys/byteorder.h>
#define htons(x) sys_cpu_to_be16(x)
#define ntohs(x) sys_be16_to_cpu(x)
#define htonl(x) sys_cpu_to_be32(x)
#define ntohl(x) sys_be32_to_cpu(x)

/**
 * Access level of the current management connection, 0 (most privileged) to 3.
 *
 * Established by A_Authorize_Request and reverted when the Transport Layer
 * connection closes. interface_write_property() refuses a write whose property
 * requires a level more privileged than the current one.
 */
void interface_set_access_level(uint8_t level);
void interface_reset_access_level(void);
uint8_t interface_access_level(void);

void interface_read_property_description(uint8_t object_index, uint8_t property_id,
					 uint8_t property_index, bool *write_enable,
					 uint8_t *property_datatype, uint16_t *max_nr_of_elem,
					 uint8_t *access, uint8_t *property_index_found);

uint8_t interface_read_property(uint8_t object_index, PropertyID property_id, uint32_t start_index,
				uint32_t *count, uint8_t *data, uint8_t max_size);

/**
 * interface_write_property — write property elements from a received PDU.
 *
 * @param start_index        1-based element index (0 addresses the element
 *                           count, which this generic path does not write).
 * @param number_of_elements Element count from the PDU's 4-bit field — NOT a
 *                           byte count.
 * @param data               Received octets.
 * @param data_length        Number of octets actually present in @p data.
 * @return false when the property does not exist, is read-only, needs a more
 *         privileged access level, or when @p data_length is too short for
 *         @p number_of_elements elements of the property's datatype.
 *
 * Elements and octets are separate arguments on purpose: the
 * caller used to pass the byte count as the element count, which only ever
 * worked because every writeable property today has either
 * max_nr_of_elem == 1 or a 1-octet datatype. A writeable PDT_UNSIGNED_INT[]
 * such as PID_TABLE would have written 2x the elements requested — reading
 * past the received octets and past the backing array.
 */
bool interface_write_property(uint8_t object_index, PropertyID property_id, uint16_t start_index,
			      uint8_t number_of_elements, const uint8_t *data, uint8_t data_length);

/**
 * interface_function_property_command — invoke a PDT_FUNCTION property.
 *
 * @param object_index  Interface object index.
 * @param property_id   Property identifier.
 * @param data_in       Input data from the request PDU.
 * @param data_in_len   Length of input data.
 * @param data_out      Buffer for the response data.
 * @param max_out       Size of data_out.
 * @return Length of response data written to data_out, or 0 if not found.
 *
 * The default implementation returns 0 (not implemented) for all properties.
 * Applications override by providing a function property handler registration.
 */
uint8_t interface_function_property_command(uint8_t object_index, uint8_t property_id,
					    const uint8_t *data_in, uint8_t data_in_len,
					    uint8_t *data_out, uint8_t max_out);

/**
 * interface_function_property_state_read — read the state of a PDT_FUNCTION property.
 * Same signature as command; the distinction is made by the caller (APCI).
 */
uint8_t interface_function_property_state_read(uint8_t object_index, uint8_t property_id,
					       const uint8_t *data_in, uint8_t data_in_len,
					       uint8_t *data_out, uint8_t max_out);

#endif
