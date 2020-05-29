/*
 * Copyright (c) 2015 Google Inc.
 * All rights reserved.
 * Author: Eli Sennesh <esennesh@leaflabs.com>
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

#include <errno.h>
#include <debug.h>
#include <stdlib.h>

#include <greybus/greybus.h>
#include <greybus/debug.h>
#include <apps/greybus-utils/utils.h>
#include <gpio.h>
#include <sys/byteorder.h>

#include "vibrator-gb.h"


/* Version of the Greybus vibrator protocol we support */
#define GB_VIBRATOR_VERSION_MAJOR    0x00
#define GB_VIBRATOR_VERSION_MINOR    0x01

/* Pick a GPIO line exposed on APBridge2 via the schematics */
#define GB_VIBRATOR_DUMMY_GPIO       0x00

static uint8_t gb_vibrator_protocol_version(struct gb_operation *operation)
{
    struct gb_vibrator_proto_version_response *response;

    response = gb_operation_alloc_response(operation, sizeof(*response));
    if (!response)
        return GB_OP_NO_MEMORY;

    response->major = GB_VIBRATOR_VERSION_MAJOR;
    response->minor = GB_VIBRATOR_VERSION_MINOR;

    return GB_OP_SUCCESS;
}

static uint8_t gb_vibrator_vibrator_on(struct gb_operation *operation)
{
    struct gb_vibrator_on_request *request =
            gb_operation_get_request_payload(operation);

    if (gb_operation_get_request_payload_size(operation) < sizeof(*request)) {
        gb_error("dropping short message\n");
        return GB_OP_INVALID;
    }

    gpio_activate(GB_VIBRATOR_DUMMY_GPIO);
    gpio_set_value(GB_VIBRATOR_DUMMY_GPIO, 1);

    usleep(sys_le16_to_cpu(request->timeout_ms));

    gpio_deactivate(GB_VIBRATOR_DUMMY_GPIO);

    return GB_OP_SUCCESS;
}

static uint8_t gb_vibrator_vibrator_off(struct gb_operation *operation)
{
    // Deactivate the GPIO line, somehow.

    gpio_activate(GB_VIBRATOR_DUMMY_GPIO);
    gpio_set_value(GB_VIBRATOR_DUMMY_GPIO, 0);
    gpio_deactivate(GB_VIBRATOR_DUMMY_GPIO);

    return GB_OP_SUCCESS;
}

static struct gb_operation_handler gb_vibrator_handlers[] = {
    GB_HANDLER(GB_VIBRATOR_TYPE_PROTOCOL_VERSION, gb_vibrator_protocol_version),
    GB_HANDLER(GB_VIBRATOR_TYPE_VIBRATOR_ON, gb_vibrator_vibrator_on),
    GB_HANDLER(GB_VIBRATOR_TYPE_VIBRATOR_OFF, gb_vibrator_vibrator_off),
};

static struct gb_driver gb_vibrator_driver = {
    .op_handlers = gb_vibrator_handlers,
    .op_handlers_count = ARRAY_SIZE(gb_vibrator_handlers),
};

void gb_vibrator_register(int cport, int bundle)
{
    gb_register_driver(cport, bundle, &gb_vibrator_driver);
}
