/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Sample app for WebUSB enabled custom class driver.
 *
 * Sample app for WebUSB enabled custom class driver. The received
 * data is echoed back to the WebUSB based application running in
 * the browser at host.
 */

#include <stdio.h>
#include <string.h>
#include <device.h>
#include <uart.h>
#include <zephyr.h>
#include "webusb_serial.h"

static volatile bool data_transmitted;
static volatile bool data_arrived;
static u8_t data_buf[64];

/* WebUSB + MS Platform Capability Descriptors */
static const u8_t webusb_bos_descriptor[] = {
	0x05, /* Descriptor size */
	0x0F, /* Descriptor type (Binary device Object Store) */
	(0x05 + 0x18 + 0x1C), 0x00, /* Total length of BOS */
	0x02, /* Number of capability descriptors that follow */

	/* WebUSB Platform Capability Descriptor:
	 * https://wicg.github.io/webusb/#webusb-platform-capability-descriptor
	 */
	0x18, /* Descriptor size (24 bytes)          */
	0x10, /* Descriptor type (Device Capability) */
	0x05, /* Capability type (Platform)          */
	0x00, /* Reserved                            */

	/* WebUSB Platform Capability UUID (3408b638-09a9-47a0-8bfd-a0768815b665) */
	0x38, 0xB6, 0x08, 0x34,
	0xA9, 0x09,
	0xA0, 0x47,
	0x8B, 0xFD,
	0xA0, 0x76, 0x88, 0x15, 0xB6, 0x65,

	0x00, 0x01, /* BCD WebUSB version: 1.0  */
	0x01, /* Vendor-assigned WebUSB request code, for control requests */
	0x01, /* Landing page index (refers to "http://localhost:8000") */

	/* Microsoft OS 2.0 Platform Capability Descriptor
	 * See https://docs.microsoft.com/en-us/windows-hardware/drivers/usbcon/microsoft-defined-usb-descriptors
	 * Adapted from https://github.com/sowbug/weblight/blob/master/firmware/webusb.c (BSD-2)
	 * Thanks http://janaxelson.com/files/ms_os_20_descriptors.c
	 */
	0x1C, /* Descriptor size (28 bytes)          */
	0x10, /* Descriptor type (Device Capability) */
	0x05, /* Capability type (Platform)          */
	0x00, /* Reserved                            */

	/* MS OS 2.0 Platform Capability ID (D8DD60DF-4589-4CC7-9CD2-659D9E648A9F) */
	0xDF, 0x60, 0xDD, 0xD8,
	0x89, 0x45,
	0xC7, 0x4C,
	0x9C, 0xD2,
	0x65, 0x9D, 0x9E, 0x64, 0x8A, 0x9F,

	0x00, 0x00, 0x03, 0x06, /* Windows version (8.1) (0x06030000) */
	(0x0A + 0x14 + 0x08), 0x00, /* Length of the MS OS 2.0 descriptor set */
	0x02, /* Vendor-assigned bMS_VendorCode, used for a control request */
	0x00  /* Alternate enumeration support. 0 = Doesn’t support it */
};

/* WebUSB Device Requests */
static const u8_t webusb_allowed_origins[] = {
	/* Allowed Origins Header:
	 * https://wicg.github.io/webusb/#get-allowed-origins
	 */
	0x05, 0x00, 0x0D, 0x00, 0x01,

	/* Configuration Subset Header:
	 * https://wicg.github.io/webusb/#configuration-subset-header
	 */
	0x04, 0x01, 0x01, 0x01,

	/* Function Subset Header:
	 * https://wicg.github.io/webusb/#function-subset-header
	 */
	0x04, 0x02, 0x02, 0x01
};

/* Number of allowed origins */
#define NUMBER_OF_ALLOWED_ORIGINS   1

/* URL Descriptor: https://wicg.github.io/webusb/#url-descriptor */
static const u8_t webusb_origin_url[] = {
	/* Length, DescriptorType, Scheme */
	0x11, 0x03, 0x00,
	'l', 'o', 'c', 'a', 'l', 'h', 'o', 's', 't', ':', '8', '0', '0', '0'
};

/* Predefined response to control commands related to MS OS 1.0 descriptors
 * Please note that this code only defines "extended compat ID OS feature
 * descriptors" and not "extended properties OS features descriptors"
 */
static const u8_t msos1_string_descriptor[] = {
	0x12, /* Descriptor size (18 bytes) */
	0x03, /* Descriptor type (string)   */
	'M',  0x00, 'S',  0x00, 'F',  0x00, 'T',  0x00, '1',  0x00, '0',  0x00, '0', 0x00,
	0x03, /* Vendor-assigned bMS_VendorCode, used for a control request */
	0x00, /* Padding byte, so bMS_VendorCode looks UTF16-compliant. */
};

static const u8_t msos1_compatid_descriptor[] = {
	/* See https://github.com/pbatard/libwdi/wiki/WCID-Devices */
	/* MS OS 1.0 header section */
	0x28, 0x00, 0x00, 0x00, /* Descriptor size (40 bytes)          */
	0x00, 0x01,             /* Version 1.00                        */
	0x04, 0x00,             /* Type: Extended compat ID descriptor */
	0x01,                   /* Number of function sections         */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,       /* reserved    */

	/* MS OS 1.0 function section */
	0x02,     /* Index of interface this section applies to. */
	0x01,     /* reserved */
	/* 8-byte compatible ID string, then 8-byte sub-compatible ID string */
	'W',  'I',  'N',  'U',  'S',  'B',  0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00 /* reserved */
};

/* Predefined response to control commands related to MS OS 2.0 descriptors */
static const u8_t msos2_descriptor[] = {
	/* MS OS 2.0 set header descriptor   */
	0x0A, 0x00,             /* Descriptor size (10 bytes)                 */
	0x00, 0x00,             /* MS_OS_20_SET_HEADER_DESCRIPTOR             */
	0x00, 0x00, 0x03, 0x06, /* Windows version (8.1) (0x06030000)         */
	(0x0A + 0x14 + 0x08), 0x00, /* Length of the MS OS 2.0 descriptor set */

	/* MS OS 2.0 function subset ID descriptor
	 * This means that the descriptors below will only apply to one
	 * set of interfaces
	 */
	0x08, 0x00, /* Descriptor size (8 bytes) */
	0x02, 0x00, /* MS_OS_20_SUBSET_HEADER_FUNCTION */
	0x02,       /* Index of first interface this subset applies to. */
	0x00,       /* reserved */
	(0x08 + 0x14), 0x00, /* Length of the MS OS 2.0 descriptor subset */

	/* MS OS 2.0 compatible ID descriptor */
	0x14, 0x00, /* Descriptor size                */
	0x03, 0x00, /* MS_OS_20_FEATURE_COMPATIBLE_ID */
	/* 8-byte compatible ID string, then 8-byte sub-compatible ID string */
	'W',  'I',  'N',  'U',  'S',  'B',  0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

/**
 * @brief Custom handler for standard requests in
 *        order to catch the request and return the
 *        WebUSB Platform Capability Descriptor.
 *
 * @param pSetup    Information about the request to execute.
 * @param len       Size of the buffer.
 * @param data      Buffer containing the request result.
 *
 * @return  0 on success, negative errno code on fail
 */
int custom_handle_req(struct usb_setup_packet *pSetup,
		s32_t *len, u8_t **data)
{
	if (GET_DESC_TYPE(pSetup->wValue) == DESCRIPTOR_TYPE_BOS) {
		*data = (u8_t *)(&webusb_bos_descriptor);
		*len = sizeof(webusb_bos_descriptor);

		return 0;
	}

	if (GET_DESC_TYPE(pSetup->wValue) == DESC_STRING &&
		GET_DESC_INDEX(pSetup->wValue) == 0xEE) {
		*data = (u8_t *)(&msos1_string_descriptor);
		*len = sizeof(msos1_string_descriptor);

		return 0;
	}

	return -ENOTSUP;
}

/**
 * @brief Handler called for vendor specific commands. This includes
 *        WebUSB allowed origins and MS OS 1.0 and 2.0 descriptors.
 *
 * @param pSetup    Information about the request to execute.
 * @param len       Size of the buffer.
 * @param data      Buffer containing the request result.
 *
 * @return  0 on success, negative errno code on fail.
 */
int vendor_handle_req(struct usb_setup_packet *pSetup,
		s32_t *len, u8_t **data)
{
	/* Get Allowed origins request */
	if (pSetup->bRequest == 0x01 && pSetup->wIndex == 0x01) {
		*data = (u8_t *)(&webusb_allowed_origins);
		*len = sizeof(webusb_allowed_origins);

		return 0;
	} else if (pSetup->bRequest == 0x01 && pSetup->wIndex == 0x02) {
		/* Get URL request */
		u8_t index = GET_DESC_INDEX(pSetup->wValue);

		if (index == 0 || index > NUMBER_OF_ALLOWED_ORIGINS)
			return -ENOTSUP;

		*data = (u8_t *)(&webusb_origin_url);
		*len = sizeof(webusb_origin_url);

		return 0;
	} else if (pSetup->bRequest == 0x02 && pSetup->wIndex == 0x07) {
		/* Get MS OS 2.0 Descriptors request */
		/* 0x07 means "MS_OS_20_DESCRIPTOR_INDEX" */
		*data = (u8_t *)(&msos2_descriptor);
		*len = sizeof(msos2_descriptor);

		return 0;
	} else if (pSetup->bRequest == 0x03 && pSetup->wIndex == 0x04) {
		/* Get MS OS 1.0 Descriptors request */
		/* 0x04 means "Extended compat ID".
		 * Use 0x05 instead for "Extended properties".
		 */
		*data = (u8_t *)(&msos1_compatid_descriptor);
		*len = sizeof(msos1_compatid_descriptor);

		return 0;
	}

	return -ENOTSUP;
}

static void interrupt_handler(struct device *dev)
{
	uart_irq_update(dev);

	if (uart_irq_tx_ready(dev)) {
		data_transmitted = true;
	}

	if (uart_irq_rx_ready(dev)) {
		data_arrived = true;
	}
}

static void write_data(struct device *dev, const u8_t *buf, int len)
{
	uart_irq_tx_enable(dev);

	data_transmitted = false;
	uart_fifo_fill(dev, buf, len);
	while (data_transmitted == false)
		;

	uart_irq_tx_disable(dev);
}

static void read_and_echo_data(struct device *dev, int *bytes_read)
{
	while (data_arrived == false)
		;

	data_arrived = false;

	/* Read all data and echo it back */
	while ((*bytes_read = uart_fifo_read(dev,
	    data_buf, sizeof(data_buf)))) {
		write_data(dev, data_buf, *bytes_read);
	}
}

/* Custom and Vendor request handlers */
static struct webusb_req_handlers req_handlers = {
	.custom_handler = custom_handle_req,
	.vendor_handler = vendor_handle_req,
};

void main(void)
{
	struct device *dev;
	u32_t baudrate, dtr = 0;
	int ret, bytes_read;

	dev = device_get_binding(WEBUSB_SERIAL_PORT_NAME);
	if (!dev) {
		printf("WebUSB device not found\n");
		return;
	}

	/* Set the custom and vendor request handlers */
	webusb_register_request_handlers(&req_handlers);

#ifdef CONFIG_UART_LINE_CTRL
	printf("Wait for DTR\n");
	while (1) {
		uart_line_ctrl_get(dev, LINE_CTRL_DTR, &dtr);
		if (dtr) {
			break;
		}
	}
	printf("DTR set, start test\n");

	/* Wait 1 sec for the host to do all settings */
	k_busy_wait(1000000);

	ret = uart_line_ctrl_get(dev, LINE_CTRL_BAUD_RATE, &baudrate);
	if (ret) {
		printf("Failed to get baudrate, ret code %d\n", ret);
	} else {
		printf("Baudrate detected: %d\n", baudrate);
	}
#endif

	uart_irq_callback_set(dev, interrupt_handler);

	/* Enable rx interrupts */
	uart_irq_rx_enable(dev);

	/* Echo the received data */
	while (1) {
		read_and_echo_data(dev, &bytes_read);
	}
}
