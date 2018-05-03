/*
 * Human Interface Device (HID) USB class core
 *
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define SYS_LOG_LEVEL CONFIG_SYS_LOG_USB_DEVICE_LEVEL
#define SYS_LOG_DOMAIN "usb/hid"
#include <logging/sys_log.h>

#include <misc/byteorder.h>
#include <usb_device.h>
#include <usb_common.h>

#include <usb_descriptor.h>
#include <class/usb_hid.h>

#define HID_INT_IN_EP_ADDR				0x81
#define HID_INT_OUT_EP_ADDR				0x01

#define HID_INT_IN_EP_IDX			0
#define HID_INT_OUT_EP_IDX			1

struct usb_hid_config {
	struct usb_if_descriptor if0;
	struct usb_hid_descriptor if0_hid;
	struct usb_ep_descriptor if0_int_in_ep;
#ifdef CONFIG_ENABLE_HID_INT_OUT_EP
	struct usb_ep_descriptor if0_int_out_ep;
#endif
} __packed;

USBD_CLASS_DESCR_DEFINE(primary) struct usb_hid_config hid_cfg = {
	/* Interface descriptor */
	.if0 = {
		.bLength = sizeof(struct usb_if_descriptor),
		.bDescriptorType = USB_INTERFACE_DESC,
		.bInterfaceNumber = 0,
		.bAlternateSetting = 0,
		.bNumEndpoints = 1,
		.bInterfaceClass = HID_CLASS,
		.bInterfaceSubClass = 0,
		.bInterfaceProtocol = 0,
		.iInterface = 0,
	},
	.if0_hid = {
		.bLength = sizeof(struct usb_hid_descriptor),
		.bDescriptorType = USB_HID_DESC,
		.bcdHID = sys_cpu_to_le16(USB_1_1),
		.bCountryCode = 0,
		.bNumDescriptors = 1,
		.subdesc[0] = {
			.bDescriptorType = USB_HID_REPORT_DESC,
			/*
			 * descriptor length needs to be set
			 * after initialization
			 */
			.wDescriptorLength = 0,
		},
	},
	.if0_int_in_ep = {
		.bLength = sizeof(struct usb_ep_descriptor),
		.bDescriptorType = USB_ENDPOINT_DESC,
		.bEndpointAddress = HID_INT_IN_EP_ADDR,
		.bmAttributes = USB_DC_EP_INTERRUPT,
		.wMaxPacketSize =
			sys_cpu_to_le16(CONFIG_HID_INTERRUPT_EP_MPS),
		.bInterval = 0x09,
	},
#ifdef CONFIG_ENABLE_HID_INT_OUT_EP
	.if0_int_out_ep = {
		.bLength = sizeof(struct usb_ep_descriptor),
		.bDescriptorType = USB_ENDPOINT_DESC,
		.bEndpointAddress = HID_INT_OUT_EP_ADDR,
		.bmAttributes = USB_DC_EP_INTERRUPT,
		.wMaxPacketSize =
			sys_cpu_to_le16(CONFIG_HID_INTERRUPT_EP_MPS),
		.bInterval = 0x09,
	},
#endif

};

static void usb_set_hid_report_size(u16_t report_desc_size)
{
	hid_cfg.if0_hid.subdesc[0].wDescriptorLength =
		sys_cpu_to_le16(report_desc_size);
}

static struct hid_device_info {
	const u8_t *report_desc;
	size_t report_size;

	const struct hid_ops *ops;
} hid_device;

static void hid_status_cb(enum usb_dc_status_code status, u8_t *param)
{
	/* Check the USB status and do needed action if required */
	switch (status) {
	case USB_DC_ERROR:
		SYS_LOG_DBG("USB device error");
		break;
	case USB_DC_RESET:
		SYS_LOG_DBG("USB device reset detected");
		break;
	case USB_DC_CONNECTED:
		SYS_LOG_DBG("USB device connected");
		break;
	case USB_DC_CONFIGURED:
		SYS_LOG_DBG("USB device configured");
		break;
	case USB_DC_DISCONNECTED:
		SYS_LOG_DBG("USB device disconnected");
		break;
	case USB_DC_SUSPEND:
		SYS_LOG_DBG("USB device suspended");
		break;
	case USB_DC_RESUME:
		SYS_LOG_DBG("USB device resumed");
		break;
	case USB_DC_UNKNOWN:
	default:
		SYS_LOG_DBG("USB unknown state");
		break;
	}
}

static int hid_class_handle_req(struct usb_setup_packet *setup,
				s32_t *len, u8_t **data)
{
	SYS_LOG_DBG("Class request: bRequest 0x%x bmRequestType 0x%x len %d",
		    setup->bRequest, setup->bmRequestType, *len);

	if (REQTYPE_GET_DIR(setup->bmRequestType) == REQTYPE_DIR_TO_HOST) {
		switch (setup->bRequest) {
		case HID_GET_REPORT:
			SYS_LOG_DBG("Get Report");
			if (hid_device.ops->get_report) {
				return hid_device.ops->get_report(setup, len,
								  data);
			} else {
				SYS_LOG_ERR("Mandatory request not supported");
				return -EINVAL;
			}
			break;
		default:
			SYS_LOG_ERR("Unhandled request 0x%x", setup->bRequest);
			break;
		}
	} else {
		switch (setup->bRequest) {
		case HID_SET_IDLE:
			SYS_LOG_DBG("Set Idle");
			if (hid_device.ops->set_idle) {
				return hid_device.ops->set_idle(setup, len,
								data);
			}
			break;
		case HID_SET_REPORT:
			if (hid_device.ops->set_report == NULL) {
				SYS_LOG_ERR("set_report not implemented");
				return -EINVAL;
			}
			return hid_device.ops->set_report(setup, len, data);
		default:
			SYS_LOG_ERR("Unhandled request 0x%x", setup->bRequest);
			break;
		}
	}

	return -ENOTSUP;
}

static int hid_custom_handle_req(struct usb_setup_packet *setup,
				s32_t *len, u8_t **data)
{
	SYS_LOG_DBG("Standard request: bRequest 0x%x bmRequestType 0x%x len %d",
		    setup->bRequest, setup->bmRequestType, *len);

	if (REQTYPE_GET_DIR(setup->bmRequestType) == REQTYPE_DIR_TO_HOST &&
	    REQTYPE_GET_RECIP(setup->bmRequestType) ==
					REQTYPE_RECIP_INTERFACE &&
					setup->bRequest == REQ_GET_DESCRIPTOR) {
		switch (setup->wValue) {
		case 0x2200:
			SYS_LOG_DBG("Return Report Descriptor");

			/* Some buggy system may be pass a larger wLength when
			 * it try read HID report descriptor, although we had
			 * already tell it the right descriptor size.
			 * So truncated wLength if it doesn't match. */
			if (*len != hid_device.report_size) {
				SYS_LOG_WRN("len %d doesn't match"
					    "Report Descriptor size", *len);
				*len = min(*len, hid_device.report_size);
			}
			*data = (u8_t *)hid_device.report_desc;
			break;
		default:
			return -ENOTSUP;
		}

		return 0;
	}

	return -ENOTSUP;
}

static void hid_int_in(u8_t ep, enum usb_dc_ep_cb_status_code ep_status)
{
	if (ep_status != USB_DC_EP_DATA_IN ||
	    hid_device.ops->int_in_ready == NULL) {
		return;
	}
	hid_device.ops->int_in_ready();
}

#ifdef CONFIG_ENABLE_HID_INT_OUT_EP
static void hid_int_out(u8_t ep, enum usb_dc_ep_cb_status_code ep_status)
{
	if (ep_status != USB_DC_EP_DATA_OUT ||
	    hid_device.ops->int_out_ready == NULL) {
		return;
	}
	hid_device.ops->int_out_ready();
}
#endif

/* Describe Endpoints configuration */
static struct usb_ep_cfg_data hid_ep_data[] = {
	{
		.ep_cb = hid_int_in,
		.ep_addr = HID_INT_IN_EP_ADDR
	},
#ifdef CONFIG_ENABLE_HID_INT_OUT_EP
	{
		.ep_cb = hid_int_out,
		.ep_addr = HID_INT_OUT_EP_ADDR,

	}
#endif
};

static void hid_interface_config(u8_t bInterfaceNumber)
{
	hid_cfg.if0.bInterfaceNumber = bInterfaceNumber;
	hid_cfg.if0.bNumEndpoints = ARRAY_SIZE(hid_ep_data);
}

USBD_CFG_DATA_DEFINE(hid) struct usb_cfg_data hid_config = {
	.usb_device_description = NULL,
	.interface_config = hid_interface_config,
	.interface_descriptor = &hid_cfg.if0,
	.cb_usb_status = hid_status_cb,
	.interface = {
		.class_handler = hid_class_handle_req,
		.custom_handler = hid_custom_handle_req,
		.payload_data = NULL,
	},
	.num_endpoints = ARRAY_SIZE(hid_ep_data),
	.endpoint = hid_ep_data,
};

#if !defined(CONFIG_USB_COMPOSITE_DEVICE)
static u8_t interface_data[CONFIG_USB_HID_MAX_PAYLOAD_SIZE];
#endif

int usb_hid_init(void)
{
	SYS_LOG_DBG("Iinitializing HID Device");

	/*
	 * Modify Report Descriptor Size
	 */
	usb_set_hid_report_size(hid_device.report_size);

#ifndef CONFIG_USB_COMPOSITE_DEVICE
	int ret;

	hid_config.interface.payload_data = interface_data;
	hid_config.usb_device_description = usb_get_device_descriptor();

	/* Initialize the USB driver with the right configuration */
	ret = usb_set_config(&hid_config);
	if (ret < 0) {
		SYS_LOG_ERR("Failed to config USB");
		return ret;
	}

	/* Enable USB driver */
	ret = usb_enable(&hid_config);
	if (ret < 0) {
		SYS_LOG_ERR("Failed to enable USB");
		return ret;
	}
#endif

	return 0;
}

void usb_hid_register_device(const u8_t *desc, size_t size,
			     const struct hid_ops *ops)
{
	hid_device.report_desc = desc;
	hid_device.report_size = size;

	hid_device.ops = ops;
}

int hid_int_ep_write(const u8_t *data, u32_t data_len, u32_t *bytes_ret)
{
	return usb_write(hid_ep_data[HID_INT_IN_EP_IDX].ep_addr, data,
			 data_len, bytes_ret);
}

int hid_int_ep_read(u8_t *data, u32_t max_data_len, u32_t *ret_bytes)
{
#ifdef CONFIG_ENABLE_HID_INT_OUT_EP
	return usb_read(hid_ep_data[HID_INT_OUT_EP_IDX].ep_addr,
			data, max_data_len, ret_bytes);
#else
	return -ENOTSUP;
#endif
}
