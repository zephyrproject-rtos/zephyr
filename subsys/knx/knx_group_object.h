/*
 * Copyright (c) 2026 Fabien Proriol
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __KNX_GROUP_OBJECT__
#define __KNX_GROUP_OBJECT__

/*
 * Application-facing Group Object API.
 *
 * Group Objects are identified by their ASAP number (1-based, assigned by
 * ETS in the Group Object Table) — see knx_app_data.h in the application
 * for the KNX_GO_* name macros bound to this project's ETS program.
 *
 * - The typed set/get accessors below read/write the object's cached value
 *   in place, encoding/decoding via the DPT family matching the accessor's
 *   C type. They never send anything on the bus by themselves.
 * - knx_group_object_write() sends the current cached value as an
 *   A_GroupValue_Write telegram (respects the ETS Transmit-enable flag).
 * - knx_group_object_set_update_handler() registers a callback fired when
 *   a Write telegram (or a response to our own read request) updates the
 *   cached value.
 * - knx_group_object_set_read_handler() registers a callback fired just
 *   before the stack auto-answers an incoming GroupValue_Read for this
 *   ASAP. The handler may refresh the cached value (via the setters below)
 *   and return true, or leave it untouched and return false — either way,
 *   the stack answers with whatever the cache holds right after the call.
 *   Leaving no handler registered (default) answers straight from cache,
 *   with no application call at all.
 */

#include <stdint.h>
#include <stdbool.h>
#include "object_group_table.h"

typedef group_object_updated_handler knx_group_object_update_handler_t;
typedef group_object_read_handler knx_group_object_read_handler_t;

static inline void knx_group_object_set_update_handler(uint8_t asap,
						       knx_group_object_update_handler_t handler)
{
	group_object_set_updated_callback(asap, handler);
}

static inline void knx_group_object_set_read_handler(uint8_t asap,
						     knx_group_object_read_handler_t handler)
{
	group_object_set_read_handler(asap, handler);
}

/* Send the current cached value as an A_GroupValue_Write telegram.
 * Returns false if the ASAP is invalid, unresolvable (no TSAP bound to it),
 * or its Transmit-enable (T) flag is not set.
 */
bool knx_group_object_write(uint8_t asap);

/* Typed setters — encode into the cached buffer, no bus traffic.
 * Return false if the ASAP is invalid or its stored size doesn't match the
 * accessor's DPT family (wrong accessor for this ASAP's ETS-configured DPT).
 */
bool knx_group_object_set_bool(uint8_t asap, bool value);     /* DPT 1.x  (1 bit)   */
bool knx_group_object_set_u8(uint8_t asap, uint8_t value);    /* DPT 5.x  (1 byte)  */
bool knx_group_object_set_u16(uint8_t asap, uint16_t value);  /* DPT 7.x  (2 bytes) */
bool knx_group_object_set_i16(uint8_t asap, int16_t value);   /* DPT 8.x  (2 bytes) */
bool knx_group_object_set_u32(uint8_t asap, uint32_t value);  /* DPT 12.x (4 bytes) */
bool knx_group_object_set_i32(uint8_t asap, int32_t value);   /* DPT 13.x (4 bytes) */
bool knx_group_object_set_float16(uint8_t asap, float value); /* DPT 9.x  (2 bytes) */
bool knx_group_object_set_float32(uint8_t asap, float value); /* DPT 14.x (4 bytes) */

/* Typed getters — decode from the cached buffer, no bus traffic.
 * Return 0/false on invalid ASAP or DPT-size mismatch.
 */
bool knx_group_object_get_bool(uint8_t asap);
uint8_t knx_group_object_get_u8(uint8_t asap);
uint16_t knx_group_object_get_u16(uint8_t asap);
int16_t knx_group_object_get_i16(uint8_t asap);
uint32_t knx_group_object_get_u32(uint8_t asap);
int32_t knx_group_object_get_i32(uint8_t asap);
float knx_group_object_get_float16(uint8_t asap);
float knx_group_object_get_float32(uint8_t asap);

#endif
