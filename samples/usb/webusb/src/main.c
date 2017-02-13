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
static uint8_t data_buf[64];

/* WebUSB Platform Capability Descriptor */
static const uint8_t webusb_bos_descriptor[] = {
	/* Binary Object Store descriptor */
	0x05, 0x0F, 0x1D, 0x00, 0x01,

	/* WebUSB Platform Capability Descriptor:
	 * https://wicg.github.io/webusb/#webusb-platform-capability-descriptor
	 */
	0x18, 0x10, 0x05, 0x00,

	/* PlatformCapability UUID */
	0x38, 0xB6, 0x08, 0x34, 0xA9, 0x09, 0xA0, 0x47,
	0x8B, 0xFD, 0xA0, 0x76, 0x88, 0x15, 0xB6, 0x65,

	/* Version, VendorCode, Landing Page */
	0x00, 0x01, 0x01, 0x01,
};

/* WebUSB Device Requests */
static const uint8_t webusb_allowed_origins[] = {
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
static const uint8_t webusb_origin_url[] = {
	/* Length, DescriptorType, Scheme */
	0x11, 0x03, 0x00,
	'l', 'o', 'c', 'a', 'l', 'h', 'o', 's', 't', ':', '8', '0', '0', '0'
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
		int32_t *len, uint8_t **data)
{
	if (GET_DESC_TYPE(pSetup->wValue) == DESCRIPTOR_TYPE_BOS) {
		*data = (uint8_t *)(&webusb_bos_descriptor);
		*len = sizeof(webusb_bos_descriptor);

		return 0;
	}

	return -ENOTSUP;
}

/**
 * @brief Handler called for WebUSB vendor specific commands.
 *
 * @param pSetup    Information about the request to execute.
 * @param len       Size of the buffer.
 * @param data      Buffer containing the request result.
 *
 * @return  0 on success, negative errno code on fail.
 */
int vendor_handle_req(struct usb_setup_packet *pSetup,
		int32_t *len, uint8_t **data)
{
	/* Get Allowed origins request */
	if (pSetup->bRequest == 0x01 && pSetup->wIndex == 0x01) {
		*data = (uint8_t *)(&webusb_allowed_origins);
		*len = sizeof(webusb_allowed_origins);

		return 0;
	} else if (pSetup->bRequest == 0x01 && pSetup->wIndex == 0x02) {
		/* Get URL request */
		uint8_t index = GET_DESC_INDEX(pSetup->wValue);

		if (index == 0 || index > NUMBER_OF_ALLOWED_ORIGINS)
			return -ENOTSUP;

		*data = (uint8_t *)(&webusb_origin_url);
		*len = sizeof(webusb_origin_url);

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

static void write_data(struct device *dev, const uint8_t *buf, int len)
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
	uint32_t baudrate, dtr = 0;
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
