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
 *
 * Author: Fabien Parent <fparent@baylibre.com>
 */

#include <sys/byteorder.h>
#include <greybus/greybus.h>
#include <usb.h>
#include "usb-gb.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(GB_USB_DEBUG)
#define gb_usb_debug(x...) printf(x)
#else
#define gb_usb_debug(x...)
#endif

static struct device *usbdev;

static uint8_t gb_usb_protocol_version(struct gb_operation *operation)
{
    struct gb_usb_proto_version_response *response;

    response = gb_operation_alloc_response(operation, sizeof(*response));
    if (!response) {
        return GB_OP_NO_MEMORY;
    }

    response->major = GB_USB_VERSION_MAJOR;
    response->minor = GB_USB_VERSION_MINOR;
    return GB_OP_SUCCESS;
}

static uint8_t gb_usb_hcd_stop(struct gb_operation *operation)
{
    gb_usb_debug("%s()\n", __func__);

    device_usb_hcd_stop(usbdev);

    return GB_OP_SUCCESS;
}

static uint8_t gb_usb_hcd_start(struct gb_operation *operation)
{
    int retval;

    gb_usb_debug("%s()\n", __func__);

    retval = device_usb_hcd_start(usbdev);
    if (retval) {
        return GB_OP_UNKNOWN_ERROR;
    }

    return GB_OP_SUCCESS;
}

static uint8_t gb_usb_hub_control(struct gb_operation *operation)
{
    struct gb_usb_hub_control_response *response;
    struct gb_usb_hub_control_request *request =
        gb_operation_get_request_payload(operation);
    uint16_t typeReq;
    uint16_t wValue;
    uint16_t wIndex;
    uint16_t wLength;
    int status;

    if (gb_operation_get_request_payload_size(operation) < sizeof(*request)) {
        return GB_OP_INVALID;
    }

    typeReq = sys_le16_to_cpu(request->typeReq);
    wValue = sys_le16_to_cpu(request->wValue);
    wIndex = sys_le16_to_cpu(request->wIndex);
    wLength = sys_le16_to_cpu(request->wLength);

    response =
        gb_operation_alloc_response(operation, sizeof(*response) + wLength);
    if (!response) {
        return GB_OP_NO_MEMORY;
    }

    gb_usb_debug("%s(%hX, %hX, %hX, %hX)\n", __func__, typeReq, wValue, wIndex,
                 wLength);

    status = device_usb_hcd_hub_control(usbdev, typeReq, wValue,  wIndex,
                                        (char*) response->buf, wLength);
    if (status) {
        return GB_OP_UNKNOWN_ERROR;
    }

    return GB_OP_SUCCESS;
}

static int gb_usb_init(unsigned int cport, struct gb_bundle *bundle)
{
    usbdev = device_open(DEVICE_TYPE_USB_HCD, 0);
    if (!usbdev) {
        return -ENODEV;
    }

    return 0;
}

static void gb_usb_exit(unsigned int cport, struct gb_bundle *bundle)
{
    if (usbdev) {
        device_close(usbdev);
    }
}

static struct gb_operation_handler gb_usb_handlers[] = {
    GB_HANDLER(GB_USB_TYPE_PROTOCOL_VERSION, gb_usb_protocol_version),
    GB_HANDLER(GB_USB_TYPE_HCD_STOP, gb_usb_hcd_stop),
    GB_HANDLER(GB_USB_TYPE_HCD_START, gb_usb_hcd_start),
    GB_HANDLER(GB_USB_TYPE_HUB_CONTROL, gb_usb_hub_control),
};

struct gb_driver usb_driver = {
    .op_handlers = (struct gb_operation_handler*) gb_usb_handlers,
    .op_handlers_count = ARRAY_SIZE(gb_usb_handlers),

    .init = gb_usb_init,
    .exit = gb_usb_exit,
};

void gb_usb_register(int cport, int bundle)
{
    gb_register_driver(cport, bundle, &usb_driver);
}
