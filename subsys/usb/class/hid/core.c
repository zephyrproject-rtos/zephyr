/*
 * Human Interface Device (HID) USB class core
 *
 * Copyright (c) 2018 Intel Corporation
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define LOG_LEVEL CONFIG_USB_DEVICE_LOG_LEVEL
#include <logging/log.h>
LOG_MODULE_REGISTER(usb_hid);

#include <misc/byteorder.h>
#include <usb_device.h>
#include <usb_common.h>

#include <usb_descriptor.h>
#include <class/usb_hid.h>

#include <stdlib.h>

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
#ifdef CONFIG_USB_HID_BOOT_PROTOCOL
		.bInterfaceSubClass = 1,
		.bInterfaceProtocol = CONFIG_USB_HID_PROTOCOL_CODE,
#else
		.bInterfaceSubClass = 0,
		.bInterfaceProtocol = 0,
#endif
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
		.bInterval = CONFIG_USB_HID_POLL_INTERVAL_MS,
	},
#ifdef CONFIG_ENABLE_HID_INT_OUT_EP
	.if0_int_out_ep = {
		.bLength = sizeof(struct usb_ep_descriptor),
		.bDescriptorType = USB_ENDPOINT_DESC,
		.bEndpointAddress = HID_INT_OUT_EP_ADDR,
		.bmAttributes = USB_DC_EP_INTERRUPT,
		.wMaxPacketSize =
			sys_cpu_to_le16(CONFIG_HID_INTERRUPT_EP_MPS),
		.bInterval = CONFIG_USB_HID_POLL_INTERVAL_MS,
	},
#endif
};

static struct hid_device_info {
	const u8_t *report_desc;
	size_t report_size;
	const struct hid_ops *ops;
#ifdef CONFIG_USB_DEVICE_SOF
	u32_t sof_cnt[CONFIG_USB_HID_REPORTS + 1];
	bool idle_on;
	bool idle_id_report;
	u8_t idle_rate[CONFIG_USB_HID_REPORTS + 1];
#endif
#ifdef CONFIG_USB_HID_BOOT_PROTOCOL
	u8_t protocol;
#endif
} hid_device;

static int hid_on_get_idle(struct usb_setup_packet *setup, s32_t *len,
			   u8_t **data)
{
#ifdef CONFIG_USB_DEVICE_SOF
	u8_t report_id = sys_le16_to_cpu(setup->wValue) & 0xFF;

	if (report_id > CONFIG_USB_HID_REPORTS) {
		LOG_ERR("Report id out of limit: %d", report_id);
		return -ENOTSUP;
	}

	u32_t size = sizeof(hid_device.idle_rate[report_id]);

	LOG_DBG("Get Idle callback, report_id: %d", report_id);

	*data = &hid_device.idle_rate[report_id];
	len = &size;
	return 0;
#else
	return -ENOTSUP;
#endif
}

static int hid_on_get_report(struct usb_setup_packet *setup, s32_t *len,
			     u8_t **data)
{
	LOG_DBG("Get Report callback");

	/* TODO: Do something. */

	return -ENOTSUP;
}

static int hid_on_get_protocol(struct usb_setup_packet *setup, s32_t *len,
			       u8_t **data)
{
#ifdef CONFIG_USB_HID_BOOT_PROTOCOL
	if (setup->wValue) {
		LOG_ERR("wValue should be 0");
		return -ENOTSUP;
	}

	u32_t size = sizeof(hid_device.protocol);

	LOG_DBG("Get Protocol callback, protocol: %d", hid_device.protocol);

	*data = &hid_device.protocol;
	len = &size;
	return 0;
#else
	return -ENOTSUP;
#endif
}

static int hid_on_set_idle(struct usb_setup_packet *setup, s32_t *len,
			   u8_t **data)
{
#ifdef CONFIG_USB_DEVICE_SOF
	u8_t rate = ((sys_le16_to_cpu(setup->wValue) & 0xFF00) >> 8);
	u8_t report_id = sys_le16_to_cpu(setup->wValue) & 0xFF;

	if (report_id > CONFIG_USB_HID_REPORTS) {
		LOG_ERR("Report id out of limit: %d", report_id);
		return -ENOTSUP;
	}

	LOG_DBG("Set Idle callback, rate: %d, report_id: %d", rate, report_id);

	hid_device.idle_rate[report_id] = rate;

	if (rate == 0) {
		/* Clear idle */
		bool clear = true;

		for (u16_t i = 1; i <= CONFIG_USB_HID_REPORTS; i++) {
			if (hid_device.idle_rate[i] != 0) {
				/* Report with non-zero id has idle rate. */
				clear = false;
				break;
			}
		}
		if (clear) {
			hid_device.idle_id_report = false;
			LOG_DBG("Non-zero report idle rate OFF.");

			if (hid_device.idle_rate[0] == 0) {
				hid_device.idle_on = false;
				LOG_DBG("Idle rate OFF.");
			}
		}
	} else {
		/* Set idle */
		hid_device.idle_on = true;
		LOG_DBG("Idle rate ON.");
		if (report_id != 0) {
			/* Report with non-zero id has idle rate set now. */
			hid_device.idle_id_report = true;
			LOG_DBG("Non-zero report idle rate ON.");
		}
	}
	return 0;
#else
	return -ENOTSUP;
#endif
}

static int hid_on_set_report(struct usb_setup_packet *setup, s32_t *len,
			     u8_t **data)
{
	LOG_DBG("Set Report callback");

	/* TODO: Do something. */

	return -ENOTSUP;
}

static int hid_on_set_protocol(struct usb_setup_packet *setup, s32_t *len,
			       u8_t **data)
{
#ifdef CONFIG_USB_HID_BOOT_PROTOCOL
	u16_t protocol = sys_le16_to_cpu(setup->wValue);

	if (protocol > HID_PROTOCOL_REPORT) {
		LOG_ERR("Unsupported protocol: %u", protocol);
		return -ENOTSUP;
	}

	LOG_DBG("Set Protocol callback, protocol: %u", protocol);

	if (hid_device.protocol != protocol) {
		hid_device.protocol = protocol;

		if (hid_device.ops && hid_device.ops->protocol_change) {
			hid_device.ops->protocol_change(protocol);
		}
	}

	return 0;
#else
	return -ENOTSUP;
#endif
}

static void usb_set_hid_report_size(u16_t size)
{
	sys_put_le16(size,
		     (u8_t *)&(hid_cfg.if0_hid.subdesc[0].wDescriptorLength));
}

#ifdef CONFIG_USB_DEVICE_SOF
void hid_clear_idle_ctx(void)
{
	hid_device.idle_on = false;
	hid_device.idle_id_report = false;
	for (u16_t i = 0; i <= CONFIG_USB_HID_REPORTS; i++) {
		hid_device.sof_cnt[i] = 0;
		hid_device.idle_rate[i] = 0;
	}
}

void hid_sof_handler(void)
{
	for (u16_t i = 0; i <= CONFIG_USB_HID_REPORTS; i++) {
		if (hid_device.idle_rate[i]) {
			hid_device.sof_cnt[i]++;
		}

		u32_t diff = abs((hid_device.idle_rate[i] * 4)
				 - hid_device.sof_cnt[i]);

		if (diff < (2 + (hid_device.idle_rate[i] / 10))) {
			hid_device.sof_cnt[i] = 0;
			if (hid_device.ops && hid_device.ops->on_idle) {
				hid_device.ops->on_idle(i);
			}
		}

		if (!hid_device.idle_id_report) {
			/* Only report with 0 id has idle rate.
			 * No need to check the whole array.
			 */
			break;
		}
	}
}
#endif

static void hid_status_cb(enum usb_dc_status_code status, const u8_t *param)
{
	switch (status) {
	case USB_DC_ERROR:
		LOG_DBG("USB device error");
		break;
	case USB_DC_RESET:
		LOG_DBG("USB device reset detected");
#ifdef CONFIG_USB_HID_BOOT_PROTOCOL
		hid_device.protocol = HID_PROTOCOL_REPORT;
#endif
#ifdef CONFIG_USB_DEVICE_SOF
		hid_clear_idle_ctx();
#endif
		break;
	case USB_DC_CONNECTED:
		LOG_DBG("USB device connected");
		break;
	case USB_DC_CONFIGURED:
		LOG_DBG("USB device configured");
		break;
	case USB_DC_DISCONNECTED:
		LOG_DBG("USB device disconnected");
		break;
	case USB_DC_SUSPEND:
		LOG_DBG("USB device suspended");
		break;
	case USB_DC_RESUME:
		LOG_DBG("USB device resumed");
		break;
	case USB_DC_SOF:
#ifdef CONFIG_USB_DEVICE_SOF
		if (hid_device.idle_on) {
			hid_sof_handler();
		}
#endif
		break;
	case USB_DC_UNKNOWN:
	default:
		LOG_DBG("USB unknown state");
		break;
	}

	if (hid_device.ops && hid_device.ops->status_cb) {
		hid_device.ops->status_cb(status, param);
	}
}

static int hid_class_handle_req(struct usb_setup_packet *setup,
				s32_t *len, u8_t **data)
{
	LOG_DBG("Class request: bRequest 0x%x bmRequestType 0x%x len %d",
		setup->bRequest, setup->bmRequestType, *len);

	if (REQTYPE_GET_DIR(setup->bmRequestType) == REQTYPE_DIR_TO_HOST) {
		switch (setup->bRequest) {
		case HID_GET_IDLE:
			if (hid_device.ops && hid_device.ops->get_idle) {
				return hid_device.ops->get_idle(setup, len,
								data);
			} else {
				return hid_on_get_idle(setup, len, data);
			}
			break;
		case HID_GET_REPORT:
			if (hid_device.ops && hid_device.ops->get_report) {
				return hid_device.ops->get_report(setup, len,
								  data);
			} else {
				return hid_on_get_report(setup, len, data);
			}
			break;
		case HID_GET_PROTOCOL:
			if (hid_device.ops && hid_device.ops->get_protocol) {
				return hid_device.ops->get_protocol(setup, len,
								    data);
			} else {
				return hid_on_get_protocol(setup, len, data);
			}
			break;
		default:
			LOG_ERR("Unhandled request 0x%x", setup->bRequest);
			break;
		}
	} else {
		switch (setup->bRequest) {
		case HID_SET_IDLE:
			if (hid_device.ops && hid_device.ops->set_idle) {
				return hid_device.ops->set_idle(setup, len,
								data);
			} else {
				return hid_on_set_idle(setup, len, data);
			}
			break;
		case HID_SET_REPORT:
			if (hid_device.ops && hid_device.ops->set_report) {
				return hid_device.ops->set_report(setup, len,
								  data);
			} else {
				return hid_on_set_report(setup, len, data);
			}
			break;
		case HID_SET_PROTOCOL:
			if (hid_device.ops && hid_device.ops->set_protocol) {
				return hid_device.ops->set_protocol(setup, len,
								    data);
			} else {
				return hid_on_set_protocol(setup, len, data);
			}
			break;
		default:
			LOG_ERR("Unhandled request 0x%x", setup->bRequest);
			break;
		}
	}

	return -ENOTSUP;
}

static int hid_custom_handle_req(struct usb_setup_packet *setup,
				 s32_t *len, u8_t **data)
{
	LOG_DBG("Standard request: bRequest 0x%x bmRequestType 0x%x len %d",
		setup->bRequest, setup->bmRequestType, *len);

	if (REQTYPE_GET_DIR(setup->bmRequestType) == REQTYPE_DIR_TO_HOST &&
	    REQTYPE_GET_RECIP(setup->bmRequestType) ==
					REQTYPE_RECIP_INTERFACE &&
					setup->bRequest == REQ_GET_DESCRIPTOR) {
		u8_t value = sys_le16_to_cpu(setup->wValue) >> 8;

		switch (value) {
		case HID_CLASS_DESCRIPTOR_HID:
			LOG_DBG("Return HID Descriptor");

			*len = min(*len, hid_cfg.if0_hid.bLength);
			*data = (u8_t *)&hid_cfg.if0_hid;
			break;
		case HID_CLASS_DESCRIPTOR_REPORT:
			LOG_DBG("Return Report Descriptor");

			/* Some buggy system may be pass a larger wLength when
			 * it try read HID report descriptor, although we had
			 * already tell it the right descriptor size.
			 * So truncated wLength if it doesn't match. */
			if (*len != hid_device.report_size) {
				LOG_WRN("len %d doesn't match "
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
	    hid_device.ops == NULL ||
	    hid_device.ops->int_in_ready == NULL) {
		return;
	}
	hid_device.ops->int_in_ready();
}

#ifdef CONFIG_ENABLE_HID_INT_OUT_EP
static void hid_int_out(u8_t ep, enum usb_dc_ep_cb_status_code ep_status)
{
	if (ep_status != USB_DC_EP_DATA_OUT ||
	    hid_device.ops == NULL ||
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
	LOG_DBG("Initializing HID Device");

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
		LOG_ERR("Failed to config USB");
		return ret;
	}

	/* Enable USB driver */
	ret = usb_enable(&hid_config);
	if (ret < 0) {
		LOG_ERR("Failed to enable USB");
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
