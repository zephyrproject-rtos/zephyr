/*
 * Copyright (c) 2026 Fabien Proriol
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __OBJECT_GROUP_TABLE__
#define __OBJECT_GROUP_TABLE__

#include <stdint.h>
#include <stdbool.h>
#include "object_property_types.h"

typedef void (*group_object_updated_handler)(uint8_t asap);

/*
 * Fired just before A_GroupValue_Read__ind auto-answers an incoming read for
 * this ASAP. Return true if the handler refreshed the cached value (via
 * group_object_data() or the typed knx_group_object_set_* accessors), false
 * for "no change" — either way the stack answers with whatever the cache
 * holds right after the call returns. No handler registered (default) means
 * no application call at all: the stack answers straight from cache.
 */
typedef bool (*group_object_read_handler)(uint8_t asap);

uint8_t group_object_flag(uint16_t asap);
uint8_t *group_object_data(uint16_t asap);
uint8_t group_object_size(uint16_t asap);
bool group_object_is_short_form(uint16_t asap);
uint16_t group_object_count(void);
bool group_object_table_is_loaded(void);
void group_object_updated(uint8_t asap);
void group_object_set_updated_callback(uint8_t asap, group_object_updated_handler callback);
void group_object_set_read_handler(uint8_t asap, group_object_read_handler handler);
void group_object_read_requested(uint8_t asap);
void object_group_table_properties_written(PropertyID id);

/*
 * Implemented in knx_group_object.c, which already has the includes needed
 * to build and send an A_GroupValue_Read (association lookup, L7). Declared
 * here rather than there so object_group_table.c can call it from
 * object_group_table_properties_written() without including the app-facing
 * knx_group_object.h — same "hook declared next to the caller, implemented
 * by the module that needs the extra includes" shape as properties_written()
 * itself. See the Group Object 'I' flag (read-on-init) handling in
 * object_group_table.c.
 */
void group_object_read_on_init_trigger(void);

/* Wipes every Group Object's cached value — RAM-only storage,
 * so a Master Reset zeroes it directly rather than through __memory.
 */
void group_object_data_reset(void);

#define GROUP_OBJECT_FLAG_U        0x80
#define GROUP_OBJECT_FLAG_T        0x40
#define GROUP_OBJECT_FLAG_I        0x20
#define GROUP_OBJECT_FLAG_W        0x10
#define GROUP_OBJECT_FLAG_R        0x08
#define GROUP_OBJECT_FLAG_C        0x04
#define GROUP_OBJECT_FLAG_PRIORITY 0x03

#endif
