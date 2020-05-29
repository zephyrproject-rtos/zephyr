/*
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

#ifndef __CONTROL_GB_H__
#define __CONTROL_GB_H__

#include <greybus/types.h>
#include <greybus/greybus.h>

/* Version of the Greybus control protocol we support */
#define GB_CONTROL_VERSION_MAJOR                0x00
#define GB_CONTROL_VERSION_MINOR                0x01

/* Greybus control request types */
#define GB_CONTROL_TYPE_INVALID                 GB_INVALID_TYPE
#define GB_CONTROL_TYPE_PROTOCOL_VERSION        0x01
/* RESERVED                                     0x02 */
#define GB_CONTROL_TYPE_GET_MANIFEST_SIZE       0x03
#define GB_CONTROL_TYPE_GET_MANIFEST            0x04
#define GB_CONTROL_TYPE_CONNECTED               0x05
#define GB_CONTROL_TYPE_DISCONNECTED            0x06
#define GB_CONTROL_TYPE_TIMESYNC_ENABLE         0x07
#define GB_CONTROL_TYPE_TIMESYNC_DISABLE        0x08
#define GB_CONTROL_TYPE_TIMESYNC_AUTHORITATIVE  0x09
#define GB_CONTROL_TYPE_INTERFACE_VERSION       0x0a
#define GB_CONTROL_TYPE_BUNDLE_VERSION		0x0b
#define GB_CONTROL_TYPE_DISCONNECTING		0x0c
#define GB_CONTROL_TYPE_TIMESYNC_GET_LAST_EVENT	0x0d
#define GB_CONTROL_TYPE_MODE_SWITCH		0x0e
#define GB_CONTROL_TYPE_BUNDLE_SUSPEND		0x0f
#define GB_CONTROL_TYPE_BUNDLE_RESUME		0x10
#define GB_CONTROL_TYPE_BUNDLE_DEACTIVATE	0x11
#define GB_CONTROL_TYPE_BUNDLE_ACTIVATE		0x12
#define GB_CONTROL_TYPE_INTF_SUSPEND_PREPARE		0x13
#define GB_CONTROL_TYPE_INTF_DEACTIVATE_PREPARE	0x14
#define GB_CONTROL_TYPE_INTF_HIBERNATE_ABORT	0x15

/* version request has no payload */
struct gb_control_proto_version_response {
    __u8      major;
    __u8      minor;
} __packed;

/* Control protocol get manifest size request has no payload*/
struct gb_control_get_manifest_size_response {
    __le16    size;
} __packed;

/* Control protocol get manifest request has no payload */
struct gb_control_get_manifest_response {
    __u8      data[0];
} __packed;

/* Control protocol [dis]connected request */
struct gb_control_connected_request {
    __le16    cport_id;
} __packed;
/* Control protocol [dis]connected response has no payload */

/*
 * All Bundle power management operations use the same request and response
 * layout and status codes.
 */

#define GB_CONTROL_BUNDLE_PM_OK		0x00
#define GB_CONTROL_BUNDLE_PM_INVAL	0x01
#define GB_CONTROL_BUNDLE_PM_BUSY	0x02
#define GB_CONTROL_BUNDLE_PM_FAIL	0x03
#define GB_CONTROL_BUNDLE_PM_NA		0x04

struct gb_control_bundle_pm_request {
	__u8	bundle_id;
} __packed;

struct gb_control_bundle_pm_response {
	__u8	status;
} __packed;

/* Control protocol interface (firmware) version request has no payload */
struct gb_control_interface_version_response {
    __le16    major;
    __le16    minor;
} __packed;

#define GB_CONTROL_PWR_STATE_OFF        0x00
#define GB_CONTROL_PWR_STATE_SUSPEND    0x01
#define GB_CONTROL_PWR_STATE_ON         0xff

#define GB_CONTROL_PWR_OK               0x00
#define GB_CONTROL_PWR_BUSY             0x01
#define GB_CONTROL_PWR_ERROR_CAP        0x02
#define GB_CONTROL_PWR_NOSUPP           0x03
#define GB_CONTROL_PWR_FAIL             0x04

struct gb_control_intf_pwr_set_request {
    __u8      pwr_state;
} __packed;

struct gb_control_intf_pwr_set_response {
    __u8      result_code;
} __packed;

struct gb_control_bundle_pwr_set_request {
    __u8      bundle_id;
    __u8      pwr_state;
} __packed;

struct gb_control_bundle_pwr_set_response {
    __u8      result_code;
} __packed;

/* Control protocol Interface timesync enable request */
struct gb_control_timesync_enable_request {
    __u8    count;
    __le64  frame_time;
    __le32  strobe_delay;
    __le32  refclk;
} __packed;
/* Control protocol Interface timesync enable response has no payload */

/* Control protocol Interface timesync authoritative request */
struct gb_control_timesync_authoritative_request {
    __le64  frame_time[GB_TIMESYNC_MAX_STROBES];
} __packed;
/* Control protocol Interface timesync authoritative response has no payload */

/* Control protocol Interface timesync get_last_event request has no payload */
struct gb_control_timesync_get_last_event_response {
    __le64  frame_time;
} __packed;

#endif /* __CONTROL_GB_H__ */

