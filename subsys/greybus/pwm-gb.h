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

#ifndef _GREYBUS_PWM_H_
#define _GREYBUS_PWM_H_

#include <greybus/types.h>

#define GB_PWM_PROTOCOL_VERSION         0x01
#define GB_PWM_PROTOCOL_COUNT           0x02
#define GB_PWM_PROTOCOL_ACTIVATE        0x03
#define GB_PWM_PROTOCOL_DEACTIVATE      0x04
#define GB_PWM_PROTOCOL_CONFIG          0x05
#define GB_PWM_PROTOCOL_POLARITY        0x06
#define GB_PWM_PROTOCOL_ENABLE          0x07
#define GB_PWM_PROTOCOL_DISABLE         0x08

struct gb_pwm_version_request {
    /** Offered PWM Protocol major version. */
    __u8    offer_major;

    /** Offered PWM Protocol minor version. */
    __u8    offer_minor;
} __packed;

struct gb_pwm_version_response {
    /** PWM Protocol major version */
    __u8    major;

    /** PWM Protocol minor version */
    __u8    minor;
} __packed;

/**
 * Count request has no payload.
 */
struct gb_pwm_count_response {
    /** Number of PWM generator minus 1. */
    __u8    count;
} __packed;

/**
 * Activate response has no payload.
 */
struct gb_pwm_activate_request {
    /** Controller-relative PWM generator number. */
    __u8    which;
} __packed;

/**
 * Deactivate response has no payload.
 */
struct gb_pwm_dectivate_request {
    /** Controller-relative PWM generator number. */
    __u8    which;
} __packed;

/**
 * config response has no payload.
 */
struct gb_pwm_config_request {
    /** Controller-relative PWM generator number */
    __u8    which;

    /** Active time (in nanoseconds). */
    __le32  duty;

    /** Period (in nanoseconds). */
    __le32  period;
} __packed;

/**
 * Set polarity response has no payload.
 */
struct gb_pwm_polarity_request {
    /** Controller-relative PWM generator number. */
    __u8    which;

    /** 0 for normal, 1 for inverted. */
    __u8    polarity;
} __packed;

/**
 * Enable response has no payload.
 */
struct gb_pwm_enable_request {
    /** Controller-relative PWM generator number. */
    __u8    which;
} __packed;

/**
 *  Disable response has no payload.
 */
struct gb_pwm_disable_request {
    /** Controller-relative PWM generator number */
    __u8    which;
} __packed;

#endif /* _GREYBUS_PWM_H_ */

