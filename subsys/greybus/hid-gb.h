/*
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

#ifndef __HID_GB_H__
#define __HID_GB_H__

/* Greybus HID operation types */
#define GB_HID_TYPE_PROTOCOL_VERSION    0x01    /* Protocol Version */
#define GB_HID_TYPE_GET_DESC            0x02    /* Get Descriptor */
#define GB_HID_TYPE_GET_REPORT_DESC     0x03    /* Get Report Descriptor */
#define GB_HID_TYPE_PWR_ON              0x04    /* Power On */
#define GB_HID_TYPE_PWR_OFF             0x05    /* Power Off */
#define GB_HID_TYPE_GET_REPORT          0x06    /* Get Report */
#define GB_HID_TYPE_SET_REPORT          0x07    /* Set Report */
#define GB_HID_TYPE_IRQ_EVENT           0x08    /* Irq Event */

/* Greybus HID Report type */
#define GB_HID_INPUT_REPORT             0       /* Input Report */
#define GB_HID_OUTPUT_REPORT            1       /* Output Report */
#define GB_HID_FEATURE_REPORT           2       /* Feature Report */

/**
 * Greybus HID Protocol Version Response
 */
struct gb_hid_proto_version_response {
    __u8 major; /**< Greybus HID Protocol major version */
    __u8 minor; /**< Greybus HID Protocol minor version */
} __packed;

/**
 * Greybus HID Get Descriptor Response
 */
struct gb_hid_desc_response {
    __u8 length; /**< Length of this descriptor */
    __le16 report_desc_length; /**< Length of the report descriptor */
    __le16 hid_version; /**< Version of the HID Protocol */
    __le16 product_id; /**< Product ID of the device */
    __le16 vendor_id; /**< Vendor ID of the device */
    __u8 country_code; /**< Country code of the localized hardware */
} __packed;

/**
 * Greybus HID Get Report Request
 */
struct gb_hid_get_report_request {
    __u8 report_type; /**< Greybus HID Report Type */
    __u8 report_id; /**< Report ID */
} __packed;

/**
 * Greybus HID Set Report Request
 */
struct gb_hid_set_report_request {
    __u8 report_type; /**< Greybus HID Report Type */
    __u8 report_id; /**< Report ID */
    __u8 report[0]; /**< Report data */
} __packed;

/**
 * HID Input Report Request (IRQ Event request)
 */
struct gb_hid_input_report_request {
    __u8 report[0]; /**< data */
} __packed;

#endif /* __HID_GB_H__ */
