/**
 * Copyright (c) 2015 Google, Inc.
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

#ifndef _GREYBUS_LIGHTS_H_
#define _GREYBUS_LIGHTS_H_

#include <greybus/types.h>

#define NAME_LENGTH                             32

/* Greybus Lights request types */
#define GB_LIGHTS_TYPE_INVALID                  GB_INVALID_TYPE
#define GB_LIGHTS_TYPE_PROTOCOL_VERSION         0x01
#define GB_LIGHTS_TYPE_GET_LIGHTS               0x02
#define GB_LIGHTS_TYPE_GET_LIGHT_CONFIG         0x03
#define GB_LIGHTS_TYPE_GET_CHANNEL_CONFIG       0x04
#define GB_LIGHTS_TYPE_GET_CHANNEL_FLASH_CONFIG 0x05
#define GB_LIGHTS_TYPE_SET_BRIGHTNESS           0x06
#define GB_LIGHTS_TYPE_SET_BLINK                0x07
#define GB_LIGHTS_TYPE_SET_COLOR                0x08
#define GB_LIGHTS_TYPE_SET_FADE                 0x09
#define GB_LIGHTS_TYPE_EVENT                    0x0A
#define GB_LIGHTS_TYPE_SET_FLASH_INTENSITY      0x0B
#define GB_LIGHTS_TYPE_SET_FLASH_STROBE         0x0C
#define GB_LIGHTS_TYPE_SET_FLASH_TIMEOUT        0x0D
#define GB_LIGHTS_TYPE_GET_FLASH_FAULT          0x0E

/**
 * Lights Protocol version response payload, version request has no payload
 */
struct gb_lights_version_response {
    /** Lights Protocol major version */
    __u8    major;
    /** Lights Protocol major version */
    __u8    minor;
} __packed;

/**
 * Lights Protocol get lights response payload
 */
struct gb_lights_get_lights_response {
    /** the number of lights */
    __u8    lights_count;
} __packed;

/**
 * Lights Protocol get light config request payload
 */
struct gb_lights_get_light_config_request {
    /** light identification number */
    __u8    id;
} __packed;

/**
 * Lights Protocol get light config response payload
 */
struct gb_lights_get_light_config_response {
    /** the number of channels in this light */
    __u8    channel_count;
    /** light name, 31 characters with null character */
    __u8    name[NAME_LENGTH];
} __packed;

/**
 * Lights Protocol get channel config request payload
 */
struct gb_lights_get_channel_config_request {
    /** light identification number */
    __u8    light_id;
    /** channel identification number */
    __u8    channel_id;
} __packed;

/**
 * Lights Protocol get channel config response payload
 */
struct gb_lights_get_channel_config_response {
    /** max brightness */
    __u8    max_brightness;
    /** light channels flags bits */
    __le32  flags;
    /** color code value */
    __le32  color;
    /** color name, 31 characters with null character */
    __u8    color_name[NAME_LENGTH];
    /** lights channel mode bits */
    __le32  mode;
    /** mode name, 31 characters with null character */
    __u8    mode_name[NAME_LENGTH];
} __packed;

/**
 * Lights Protocol get channel flash config request payload
 */
struct gb_lights_get_channel_flash_config_request {
    /** light identification number */
    __u8    light_id;
    /** channel identification number */
    __u8    channel_id;
} __packed;

/**
 * Lights Protocol get channel flash config response payload
 */
struct gb_lights_get_channel_flash_config_response {
    /** min current intensity in microamps */
    __le32  intensity_min_uA;
    /** max current intensity in microamps */
    __le32  intensity_max_uA;
    /** current intensity step in microamps */
    __le32  intensity_step_uA;
    /** min strobe flash timeout in microseconds */
    __le32  timeout_min_us;
    /** max strobe flash timeout in microseconds */
    __le32  timeout_max_us;
    /** strobe flash timeout step in microseconds */
    __le32  timeout_step_us;
} __packed;

/**
 * Lights Protocol set brightness request payload, response have no payload
 */
struct gb_lights_set_brightness_request {
    /** light identification number */
    __u8    light_id;
    /** channel identification number */
    __u8    channel_id;
    /** brightness to be set */
    __u8    brightness;
} __packed;

/**
 * Lights Protocol set blink request payload, response have no payload
 */
struct gb_lights_set_blink_request {
    /** light identification number */
    __u8    light_id;
    /** channel identification number */
    __u8    channel_id;
    /** light time on period for blink mode */
    __le16  time_on_ms;
    /** light time off period for blink mode */
    __le16  time_off_ms;
} __packed;

/**
 * Lights Protocol set color request payload, response have no payload
 */
struct gb_lights_set_color_request {
    /** light identification number */
    __u8    light_id;
    /** channel identification number */
    __u8    channel_id;
    /** channel color code to be set */
    __le32  color;
} __packed;

/**
 * Lights Protocol set fade request payload, response have no payload
 */
struct gb_lights_set_fade_request {
    /** light identification number */
    __u8    light_id;
    /** channel identification number */
    __u8    channel_id;
    /** fade-in level */
    __u8    fade_in;
    /** fade-out level */
    __u8    fade_out;
} __packed;

/**
 * Lights Protocol event request payload, response have no payload
 */
struct gb_lights_event_request {
    /** light identification number */
    __u8    light_id;
    /** Greybus Lights event bit masks */
    __u8    event;
} __packed;

/**
 * Lights Protocol set flash intensity request payload, response have no
 * payload
 */
struct gb_lights_set_flash_intensity_request {
    /** light identification number */
    __u8    light_id;
    /** channel identification number */
    __u8    channel_id;
    /** current intensity in microamperes */
    __le32  intensity_uA;
} __packed;

/**
 * Lights Protocol set flash strobe request payload, response have no payload
 */
struct gb_lights_set_flash_strobe_request {
    /** light identification number */
    __u8    light_id;
    /** channel identification number */
    __u8    channel_id;
    /** strobe state to be set */
    __u8    state;
} __packed;

/**
 * Lights Protocol set flash timeout request payload, response have no payload
 */
struct gb_lights_set_flash_timeout_request {
    /** light identification number */
    __u8    light_id;
    /** channel identification number */
    __u8    channel_id;
    /** timeout value in microseconds */
    __le32  timeout_us;
} __packed;

/**
 * Lights Protocol get flash fault request payload
 */
struct gb_lights_get_flash_fault_request {
    /** light identification number */
    __u8    light_id;
    /** channel identification number */
    __u8    channel_id;
} __packed;

/**
 * Lights Protocol get flash fault response payload
 */
struct gb_lights_get_flash_fault_response {
    /** Greybus Lights flash fault bit masks */
    __le32  fault;
} __packed;

#endif /* _GREYBUS_LIGHTS_H_ */

