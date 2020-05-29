/**
 * Copyright (c) 2015 Google Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * 1. Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 * 3. Neither the name of the copyright holder nor the names of its
 * contributors may be used to endorse or promote products derived from this
 * software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef __POWER_SUPPLY_GB_H__
#define __POWER_SUPPLY_GB_H__

#include <greybus/types.h>

/* Greybus power supply operation types */
#define GB_POWER_SUPPLY_TYPE_INVALID              GB_INVALID_TYPE
#define GB_POWER_SUPPLY_TYPE_VERSION              0x01
#define GB_POWER_SUPPLY_TYPE_GET_SUPPLIES         0x02
#define GB_POWER_SUPPLY_TYPE_GET_DESCRIPTION      0x03
#define GB_POWER_SUPPLY_TYPE_GET_PROP_DESCRIPTORS 0x04
#define GB_POWER_SUPPLY_TYPE_GET_PROPERTY         0x05
#define GB_POWER_SUPPLY_TYPE_SET_PROPERTY         0x06
#define GB_POWER_SUPPLY_TYPE_EVENT                0x07

struct gb_power_supply_version_response {
    __u8 major;
    __u8 minor;
} __packed;

struct gb_power_supply_get_supplies_response {
    __u8 supplies_count;
} __packed;

struct gb_power_supply_get_description_request {
    __u8 psy_id;
} __packed;

struct gb_power_supply_get_description_response {
    __u8   manufacturer[32];
    __u8   model[32];
    __u8   serial_number[32];
    __le16 type;
    __u8   properties_count;
} __packed;

struct gb_power_supply_props_desc {
    __u8 property;
    __u8 is_writeable;
} __packed;

struct gb_power_supply_get_property_descriptors_request {
    __u8 psy_id;
} __packed;

struct gb_power_supply_get_property_descriptors_response {
    __u8   properties_count;
    struct gb_power_supply_props_desc props[];
} __packed;

struct gb_power_supply_get_property_request {
    __u8 psy_id;
    __u8 property;
} __packed;

struct gb_power_supply_get_property_response {
    __le32 prop_val;
};

struct gb_power_supply_set_property_request {
    __u8   psy_id;
    __u8   property;
    __le32 prop_val;
} __packed;

struct gb_power_supply_event_request {
    __u8 psy_id;
    __u8 event;
} __packed;

#endif /* __POWER_SUPPLY_GB_H__ */
