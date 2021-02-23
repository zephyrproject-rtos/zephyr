/*
 * Copyright (c) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define LOG_LEVEL LOG_LEVEL_INF
#include <logging/log.h>
LOG_MODULE_REGISTER(usb_transport);

#include <zephyr.h>

#include <usb/usb_device.h>
#include <usb/class/usb_hid.h>

#include "usb_transport.h"

/* callback function list */
static int usb_transport_get_report(const struct device *dev,
		struct usb_setup_packet *setup,
		int32_t *len, uint8_t **data);
static int usb_transport_get_idle(const struct device *dev,
		struct usb_setup_packet *setup,
		int32_t *len, uint8_t **data);
static int usb_transport_get_protocol(const struct device *dev,
		struct usb_setup_packet *setup,
		int32_t *len, uint8_t **data);
static int usb_transport_set_report(const struct device *dev,
		struct usb_setup_packet *setup,
		int32_t *len, uint8_t **data);
static int usb_transport_set_idle(const struct device *dev,
		struct usb_setup_packet *setup,
		int32_t *len, uint8_t **data);
static int usb_transport_set_protocol(const struct device *dev,
		struct usb_setup_packet *setup,
		int32_t *len, uint8_t **data);
static void usb_transport_host_ready(const struct device *dev);

static const struct hid_ops usb_transport_callbacks = {
	.get_report	= usb_transport_get_report,
	.get_idle	= usb_transport_get_idle,
	.get_protocol	= usb_transport_get_protocol,
	.set_report	= usb_transport_set_report,
	.set_idle	= usb_transport_set_idle,
	.set_protocol	= usb_transport_set_protocol,
	.int_in_ready	= usb_transport_host_ready,
};

/* create an HID report descriptor with input and output reports */
static const uint8_t usb_transport_hid_report_desc[] = {
	/* Usage page: vendor defined */
	HID_ITEM(HID_ITEM_TAG_USAGE_PAGE, HID_ITEM_TYPE_GLOBAL, 2), 0x00, 0xFF,

	/* Usage: vendor specific */
	HID_USAGE(0x01),
	/* Collection: Application Collection */
	HID_COLLECTION(HID_COLLECTION_APPLICATION),
	/* Logical Minimum: 0 */
	HID_LOGICAL_MIN8(0x00),
	/* Logical Maximum: 255 */
	HID_LOGICAL_MAX8(0xFF),
	/* Report Size: 8 bits */
	HID_REPORT_SIZE(8),
	/* Report Count (in bytes) */
	HID_REPORT_COUNT(USB_TPORT_HID_REPORT_COUNT - 1),

	/* Report ID: 1 */
	HID_REPORT_ID(USB_TPORT_HID_REPORT_ID),
	/* Vendor Usage 2 */
	HID_USAGE(0x02),
	/* Input: Data, Variable, Absolute & Buffered bytes*/
	HID_INPUT(0x86),

	/* Report ID: 1 */
	HID_REPORT_ID(USB_TPORT_HID_REPORT_ID),
	/* Vendor Usage 2 */
	HID_USAGE(0x02),
	/* Output: Data, Variable, Absolute & Buffered bytes*/
	HID_OUTPUT(0x86),

	/* End collection */
	HID_END_COLLECTION,
};

static usb_transport_receive_callback_t receive_data_cb;

static const struct device *hid_device;

int usb_transport_init(usb_transport_receive_callback_t callback)
{
	int ret;

	hid_device = device_get_binding("HID_0");

	if (hid_device == NULL) {
		LOG_ERR("USB HID Device not found");
		return -ENODEV;
	}

	/* register a HID device and the callback functions */
	usb_hid_register_device(hid_device, usb_transport_hid_report_desc,
			sizeof(usb_transport_hid_report_desc),
			&usb_transport_callbacks);

	receive_data_cb = callback;

	ret = usb_enable(NULL);
	if (ret != 0) {
		LOG_ERR("Failed to enable USB");
		return -EIO;
	}

	/* initialize USB interface and HID class */
	return usb_hid_init(hid_device);
}

int usb_transport_send_reply(uint8_t *data, uint32_t len)
{
	int ret = 0;
	uint32_t written = 0;
	union usb_hid_report_hdr *header;

	/* roll back buffer to point to header */
	data = data - sizeof(union usb_hid_report_hdr);
	header = (union usb_hid_report_hdr *)data;

	/* send reply in one or more HID reports */
	while (len) {
		/* populate header */
		header->byte.report_id = USB_TPORT_HID_REPORT_ID;
		header->byte.unused[0] = header->byte.unused[1] = 0;
		if (len < USB_TPORT_HID_REPORT_DATA_LEN) {
			header->byte.byte_count = len;
			len = 0;
		} else {
			header->byte.byte_count = USB_TPORT_HID_REPORT_DATA_LEN;
			len -= USB_TPORT_HID_REPORT_DATA_LEN;
		}

		ret = hid_int_ep_write(hid_device, (uint8_t *)header,
				USB_TPORT_HID_REPORT_COUNT, &written);

		if (ret) {
			LOG_DBG("usb_write failed with error %d", ret);
			break;
		}

		if (written != USB_TPORT_HID_REPORT_COUNT) {
			/* usb_write is expected to send all data */
			LOG_ERR("usb_write: requested %u sent %u",
					USB_TPORT_HID_REPORT_COUNT, written);
			ret = -EIO;
			break;
		}

		/* move pointer to fill next header */
		header = (union usb_hid_report_hdr *)((uint8_t *)header +
				USB_TPORT_HID_REPORT_DATA_LEN);
	}

	return ret;
}

static int usb_transport_get_report(const struct device *dev,
		struct usb_setup_packet *setup,
		int32_t *len, uint8_t **data)
{
	LOG_DBG("usb_transport_get_report");
	return 0;
}

static int usb_transport_get_idle(const struct device *dev,
		struct usb_setup_packet *setup,
		int32_t *len, uint8_t **data)
{
	LOG_DBG("usb_transport_get_idle");
	return 0;
}

static int usb_transport_get_protocol(const struct device *dev,
		struct usb_setup_packet *setup,
		int32_t *len, uint8_t **data)
{
	LOG_DBG("usb_transport_get_protocol");
	return 0;
}

static int usb_transport_set_report(const struct device *dev,
		struct usb_setup_packet *setup,
		int32_t *len, uint8_t **data)
{
	uint8_t	*buffer;
	int32_t	size;

	/* strip the HID header */
	buffer = *data + sizeof(union usb_hid_report_hdr);
	size = *len - sizeof(union usb_hid_report_hdr);

	receive_data_cb(buffer, size);
	return 0;
}

static int usb_transport_set_idle(const struct device *dev,
		struct usb_setup_packet *setup,
		int32_t *len, uint8_t **data)
{
	LOG_DBG("usb_transport_set_idle");
	return 0;
}

static int usb_transport_set_protocol(const struct device *dev,
		struct usb_setup_packet *setup,
		int32_t *len, uint8_t **data)
{
	LOG_DBG("usb_transport_set_protocol");
	return 0;
}

static void usb_transport_host_ready(const struct device *dev)
{
	LOG_DBG("usb_transport_host_ready");
}
