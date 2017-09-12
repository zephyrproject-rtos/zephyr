/***************************************************************************
 *
 * Copyright(c) 2015,2016 Intel Corporation.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 * * Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in
 * the documentation and/or other materials provided with the
 * distribution.
 * * Neither the name of Intel Corporation nor the names of its
 * contributors may be used to endorse or promote products derived
 * from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 ***************************************************************************/

/**
 * @file
 * @brief WebUSB enabled custom class driver header file
 *
 * Header file for WebUSB enabled custom class driver
 */

#ifndef __WEBUSB_SERIAL_H__
#define __WEBUSB_SERIAL_H__

#include <device.h>
#include <usb/usb_device.h>

/* Set USB version to 2.1 so that the host will request the BOS descriptor. */
#define USB_2_1     0x0210

/* BOS descriptor type */
#define DESCRIPTOR_TYPE_BOS     0x0f

/* Number of interfaces */
#define WEBUSB_NUM_ITF         0x03

/* Number of Endpoints in the custom interface */
#define WEBUSB_NUM_EP          0x02
#define WEBUSB_ENDP_OUT        0x02
#define WEBUSB_ENDP_IN         0x83

/* Size in bytes of the configuration sent to
 * the Host on GetConfiguration() request
 * For Communication Device: CONF + (3 x ITF) +
 * (5 x EP) + HF + CMF + ACMF + UF -> 67 bytes
 */
#define WEBUSB_SERIAL_CONF_SIZE   (USB_CONFIGURATION_DESC_SIZE + \
	(3 * USB_INTERFACE_DESC_SIZE) + (5 * USB_ENDPOINT_DESC_SIZE) + 19)

/* WebUSB enabled Custom Class driver port name */
#define WEBUSB_SERIAL_PORT_NAME "WSERIAL"

/**
 * WebUSB request handlers
 */
struct webusb_req_handlers {
	/* Handler for WebUSB Vendor specific commands */
	usb_request_handler vendor_handler;
	/**
	 * The custom request handler gets a first chance at handling
	 * the request before it is handed over to the 'chapter 9' request
	 * handler
	 */
	usb_request_handler custom_handler;
};

/**
 * @brief Register Custom and Vendor request callbacks
 *
 * Function to register Custom and Vendor request callbacks
 * for handling requests.
 *
 * @param [in] handlers Pointer to WebUSB request handlers structure
 *
 * @return N/A
 */
void webusb_register_request_handlers(struct webusb_req_handlers *handlers);

#endif /* __WEBUSB_SERIAL_H__ */
