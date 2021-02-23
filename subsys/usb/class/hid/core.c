/*
 * Human Interface Device (HID) USB class core
 *
 * Copyright (c) 2018 Intel Corporation
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define LOG_LEVEL CONFIG_USB_HID_LOG_LEVEL
#include <logging/log.h>
LOG_MODULE_REGISTER(usb_hid);

#include <sys/byteorder.h>
#include <usb_device.h>
#include <usb_common.h>

#include <usb_descriptor.h>
#include <class/usb_hid.h>

#include <stdlib.h>

#define HID_INT_IN_EP_IDX		0
#define HID_INT_OUT_EP_IDX		1

struct usb_hid_class_subdescriptor {
	uint8_t bDescriptorType;
	uint16_t wDescriptorLength;
} __packed;

struct usb_hid_descriptor {
	uint8_t bLength;
	uint8_t bDescriptorType;
	uint16_t bcdHID;
	uint8_t bCountryCode;
	uint8_t bNumDescriptors;

	/*
	 * Specification says at least one Class Descriptor needs to
	 * be present (Report Descriptor).
	 */
	struct usb_hid_class_subdescriptor subdesc[1];
} __packed;

struct usb_hid_config {
	struct usb_if_descriptor if0;
	struct usb_hid_descriptor if0_hid;
	struct usb_ep_descriptor if0_int_in_ep;
#ifdef CONFIG_ENABLE_HID_INT_OUT_EP
	struct usb_ep_descriptor if0_int_out_ep;
#endif
} __packed;

#if defined(CONFIG_USB_HID_BOOT_PROTOCOL)
#define INITIALIZER_IF							\
	{								\
		.bLength = sizeof(struct usb_if_descriptor),		\
		.bDescriptorType = USB_INTERFACE_DESC,			\
		.bInterfaceNumber = 0,					\
		.bAlternateSetting = 0,					\
		.bNumEndpoints = 1,					\
		.bInterfaceClass = HID_CLASS,				\
		.bInterfaceSubClass = 1,				\
		.bInterfaceProtocol = CONFIG_USB_HID_PROTOCOL_CODE,	\
		.iInterface = 0,					\
	}
#else
#define INITIALIZER_IF							\
	{								\
		.bLength = sizeof(struct usb_if_descriptor),		\
		.bDescriptorType = USB_INTERFACE_DESC,			\
		.bInterfaceNumber = 0,					\
		.bAlternateSetting = 0,					\
		.bNumEndpoints = 1,					\
		.bInterfaceClass = HID_CLASS,				\
		.bInterfaceSubClass = 0,				\
		.bInterfaceProtocol = 0,				\
		.iInterface = 0,					\
	}
#endif

/* Descriptor length needs to be set after initialization */
#define INITIALIZER_IF_HID						\
	{								\
		.bLength = sizeof(struct usb_hid_descriptor),		\
		.bDescriptorType = USB_HID_DESC,			\
		.bcdHID = sys_cpu_to_le16(USB_1_1),			\
		.bCountryCode = 0,					\
		.bNumDescriptors = 1,					\
		.subdesc[0] = {						\
			.bDescriptorType = USB_HID_REPORT_DESC,		\
			.wDescriptorLength = 0,				\
		},							\
	}

#define INITIALIZER_IF_EP(addr, attr, mps)				\
	{								\
		.bLength = sizeof(struct usb_ep_descriptor),		\
		.bDescriptorType = USB_ENDPOINT_DESC,			\
		.bEndpointAddress = addr,				\
		.bmAttributes = attr,					\
		.wMaxPacketSize = sys_cpu_to_le16(mps),			\
		.bInterval = CONFIG_USB_HID_POLL_INTERVAL_MS,		\
	}

#ifdef CONFIG_ENABLE_HID_INT_OUT_EP
#define DEFINE_HID_DESCR(x, _)						\
	USBD_CLASS_DESCR_DEFINE(primary, x)				\
	struct usb_hid_config hid_cfg_##x = {				\
	/* Interface descriptor */					\
	.if0 = INITIALIZER_IF,						\
	.if0_hid = INITIALIZER_IF_HID,					\
	.if0_int_in_ep =						\
		INITIALIZER_IF_EP(AUTO_EP_IN,				\
				  USB_DC_EP_INTERRUPT,			\
				  CONFIG_HID_INTERRUPT_EP_MPS),		\
	.if0_int_out_ep =						\
		INITIALIZER_IF_EP(AUTO_EP_OUT,				\
				  USB_DC_EP_INTERRUPT,			\
				  CONFIG_HID_INTERRUPT_EP_MPS),		\
	};
#else
#define DEFINE_HID_DESCR(x, _)						\
	USBD_CLASS_DESCR_DEFINE(primary, x)				\
	struct usb_hid_config hid_cfg_##x = {				\
	/* Interface descriptor */					\
	.if0 = INITIALIZER_IF,						\
	.if0_hid = INITIALIZER_IF_HID,					\
	.if0_int_in_ep =						\
		INITIALIZER_IF_EP(AUTO_EP_IN,				\
				  USB_DC_EP_INTERRUPT,			\
				  CONFIG_HID_INTERRUPT_EP_MPS),		\
	};
#endif

struct hid_device_info {
	const uint8_t *report_desc;
	size_t report_size;
	const struct hid_ops *ops;
#ifdef CONFIG_USB_DEVICE_SOF
	uint32_t sof_cnt[CONFIG_USB_HID_REPORTS + 1];
	bool idle_on;
	bool idle_id_report;
	uint8_t idle_rate[CONFIG_USB_HID_REPORTS + 1];
#endif
#ifdef CONFIG_USB_HID_BOOT_PROTOCOL
	uint8_t protocol;
#endif
	bool configured;
	bool suspended;
	struct usb_dev_data common;
};

static sys_slist_t usb_hid_devlist;

static int hid_on_get_idle(struct hid_device_info *dev_data,
			   struct usb_setup_packet *setup, int32_t *len,
			   uint8_t **data)
{
#ifdef CONFIG_USB_DEVICE_SOF
	uint8_t report_id = (uint8_t)setup->wValue;

	if (report_id > CONFIG_USB_HID_REPORTS) {
		LOG_ERR("Report id out of limit: %d", report_id);
		return -ENOTSUP;
	}

	uint32_t size = sizeof(dev_data->idle_rate[report_id]);

	LOG_DBG("Get Idle callback, report_id: %d", report_id);

	*data = &dev_data->idle_rate[report_id];
	*len = size;
	return 0;
#else
	return -ENOTSUP;
#endif
}

static int hid_on_get_report(struct hid_device_info *dev_data,
			     struct usb_setup_packet *setup, int32_t *len,
			     uint8_t **data)
{
	LOG_DBG("Get Report callback");

	/* TODO: Do something. */

	return -ENOTSUP;
}

static int hid_on_get_protocol(struct hid_device_info *dev_data,
			       struct usb_setup_packet *setup, int32_t *len,
			       uint8_t **data)
{
#ifdef CONFIG_USB_HID_BOOT_PROTOCOL
	if (setup->wValue) {
		LOG_ERR("wValue should be 0");
		return -ENOTSUP;
	}

	uint32_t size = sizeof(dev_data->protocol);

	LOG_DBG("Get Protocol callback, protocol: %d", dev_data->protocol);

	*data = &dev_data->protocol;
	*len = size;
	return 0;
#else
	return -ENOTSUP;
#endif
}

static int hid_on_set_idle(struct hid_device_info *dev_data,
			   struct usb_setup_packet *setup, int32_t *len,
			   uint8_t **data)
{
#ifdef CONFIG_USB_DEVICE_SOF
	uint8_t rate = (uint8_t)(setup->wValue >> 8);
	uint8_t report_id = (uint8_t)setup->wValue;

	if (report_id > CONFIG_USB_HID_REPORTS) {
		LOG_ERR("Report id out of limit: %d", report_id);
		return -ENOTSUP;
	}

	LOG_DBG("Set Idle callback, rate: %d, report_id: %d", rate, report_id);

	dev_data->idle_rate[report_id] = rate;

	if (rate == 0U) {
		/* Clear idle */
		bool clear = true;

		for (uint16_t i = 1; i <= CONFIG_USB_HID_REPORTS; i++) {
			if (dev_data->idle_rate[i] != 0U) {
				/* Report with non-zero id has idle rate. */
				clear = false;
				break;
			}
		}
		if (clear) {
			dev_data->idle_id_report = false;
			LOG_DBG("Non-zero report idle rate OFF.");

			if (dev_data->idle_rate[0] == 0U) {
				dev_data->idle_on = false;
				LOG_DBG("Idle rate OFF.");
			}
		}
	} else {
		/* Set idle */
		dev_data->idle_on = true;
		LOG_DBG("Idle rate ON.");
		if (report_id != 0U) {
			/* Report with non-zero id has idle rate set now. */
			dev_data->idle_id_report = true;
			LOG_DBG("Non-zero report idle rate ON.");
		}
	}
	return 0;
#else
	return -ENOTSUP;
#endif
}

static int hid_on_set_report(struct hid_device_info *dev_data,
			     struct usb_setup_packet *setup, int32_t *len,
			     uint8_t **data)
{
	LOG_DBG("Set Report callback");

	/* TODO: Do something. */

	return -ENOTSUP;
}

static int hid_on_set_protocol(const struct device *dev,
			       struct hid_device_info *dev_data,
			       struct usb_setup_packet *setup)
{
#ifdef CONFIG_USB_HID_BOOT_PROTOCOL
	uint16_t protocol = setup->wValue;

	if (protocol > HID_PROTOCOL_REPORT) {
		LOG_ERR("Unsupported protocol: %u", protocol);
		return -ENOTSUP;
	}

	LOG_DBG("Set Protocol callback, protocol: %u", protocol);

	if (dev_data->protocol != protocol) {
		dev_data->protocol = protocol;

		if (dev_data->ops && dev_data->ops->protocol_change) {
			dev_data->ops->protocol_change(dev, protocol);
		}
	}

	return 0;
#else
	return -ENOTSUP;
#endif
}

static void usb_set_hid_report_size(const struct usb_cfg_data *cfg, uint16_t size)
{
	struct usb_if_descriptor *if_desc = (void *)cfg->interface_descriptor;
	struct usb_hid_config *desc =
			CONTAINER_OF(if_desc, struct usb_hid_config, if0);

	LOG_DBG("if_desc %p desc %p size %u", if_desc, desc, size);

	sys_put_le16(size,
		     (uint8_t *)&(desc->if0_hid.subdesc[0].wDescriptorLength));
}

#ifdef CONFIG_USB_DEVICE_SOF
void hid_clear_idle_ctx(struct hid_device_info *dev_data)
{
	dev_data->idle_on = false;
	dev_data->idle_id_report = false;
	for (uint16_t i = 0; i <= CONFIG_USB_HID_REPORTS; i++) {
		dev_data->sof_cnt[i] = 0U;
		dev_data->idle_rate[i] = 0U;
	}
}

void hid_sof_handler(struct hid_device_info *dev_data)
{
	const struct device *dev = dev_data->common.dev;

	for (uint16_t i = 0; i <= CONFIG_USB_HID_REPORTS; i++) {
		if (dev_data->idle_rate[i]) {
			dev_data->sof_cnt[i]++;
		}

		uint32_t diff = abs((dev_data->idle_rate[i] * 4U)
				 - dev_data->sof_cnt[i]);

		if (diff < (2 + (dev_data->idle_rate[i] / 10U))) {
			dev_data->sof_cnt[i] = 0U;
			if (dev_data->ops && dev_data->ops->on_idle) {
				dev_data->ops->on_idle(dev, i);
			}
		}

		if (!dev_data->idle_id_report) {
			/* Only report with 0 id has idle rate.
			 * No need to check the whole array.
			 */
			break;
		}
	}
}
#endif

static void hid_do_status_cb(struct hid_device_info *dev_data,
			     enum usb_dc_status_code status,
			     const uint8_t *param)
{
	switch (status) {
	case USB_DC_ERROR:
		LOG_INF("Device error");
		break;
	case USB_DC_RESET:
		LOG_INF("Device reset detected");
		dev_data->configured = false;
		dev_data->suspended = false;
#ifdef CONFIG_USB_HID_BOOT_PROTOCOL
		dev_data->protocol = HID_PROTOCOL_REPORT;
#endif
#ifdef CONFIG_USB_DEVICE_SOF
		hid_clear_idle_ctx(dev_data);
#endif
		break;
	case USB_DC_CONNECTED:
		LOG_INF("Device connected");
		break;
	case USB_DC_CONFIGURED:
		LOG_INF("Device configured");
		dev_data->configured = true;
		break;
	case USB_DC_DISCONNECTED:
		LOG_INF("Device disconnected");
		dev_data->configured = false;
		dev_data->suspended = false;
		break;
	case USB_DC_SUSPEND:
		LOG_INF("Device suspended");
		dev_data->suspended = true;
		break;
	case USB_DC_RESUME:
		LOG_INF("Device resumed");
		if (dev_data->suspended) {
			LOG_INF("from suspend");
			dev_data->suspended = false;
		} else {
			LOG_DBG("Spurious resume event");
		}
		break;
	case USB_DC_SOF:
#ifdef CONFIG_USB_DEVICE_SOF
		if (dev_data->idle_on) {
			hid_sof_handler(dev_data);
		}
#endif
		break;
	case USB_DC_UNKNOWN:
	default:
		LOG_INF("Unknown event");
		break;
	}

}

static void hid_status_cb(struct usb_cfg_data *cfg,
			  enum usb_dc_status_code status,
			  const uint8_t *param)
{
	struct hid_device_info *dev_data;
	struct usb_dev_data *common;

	LOG_DBG("cfg %p status %d", cfg, status);

	common = usb_get_dev_data_by_cfg(&usb_hid_devlist, cfg);
	if (common == NULL) {
		LOG_WRN("Device data not found for cfg %p", cfg);
		return;
	}

	dev_data = CONTAINER_OF(common, struct hid_device_info, common);

	hid_do_status_cb(dev_data, status, param);
}

static int hid_class_handle_req(struct usb_setup_packet *setup,
				int32_t *len, uint8_t **data)
{
	struct hid_device_info *dev_data;
	struct usb_dev_data *common;
	const struct device *dev;

	LOG_DBG("Class request:"
		"bRequest 0x%02x, bmRequestType 0x%02x len %d",
		setup->bRequest, setup->bmRequestType, *len);

	common = usb_get_dev_data_by_iface(&usb_hid_devlist,
					   (uint8_t)setup->wIndex);
	if (common == NULL) {
		LOG_WRN("Device data not found for interface %u",
			setup->wIndex);
		return -ENODEV;
	}

	dev_data = CONTAINER_OF(common, struct hid_device_info, common);
	dev = common->dev;

	if (REQTYPE_GET_DIR(setup->bmRequestType) == REQTYPE_DIR_TO_HOST) {
		switch (setup->bRequest) {
		case USB_HID_GET_IDLE:
			if (dev_data->ops && dev_data->ops->get_idle) {
				return dev_data->ops->get_idle(dev, setup, len,
							       data);
			} else {
				return hid_on_get_idle(dev_data, setup, len,
						       data);
			}
			break;
		case USB_HID_GET_REPORT:
			if (dev_data->ops && dev_data->ops->get_report) {
				return dev_data->ops->get_report(dev, setup,
								 len, data);
			} else {
				return hid_on_get_report(dev_data, setup, len,
							 data);
			}
			break;
		case USB_HID_GET_PROTOCOL:
			if (dev_data->ops && dev_data->ops->get_protocol) {
				return dev_data->ops->get_protocol(dev, setup,
								   len, data);
			} else {
				return hid_on_get_protocol(dev_data, setup, len,
							   data);
			}
			break;
		default:
			LOG_ERR("Unhandled request 0x%02x", setup->bRequest);
			break;
		}
	} else {
		switch (setup->bRequest) {
		case USB_HID_SET_IDLE:
			if (dev_data->ops && dev_data->ops->set_idle) {
				return dev_data->ops->set_idle(dev, setup, len,
							       data);
			} else {
				return hid_on_set_idle(dev_data, setup, len,
						       data);
			}
			break;
		case USB_HID_SET_REPORT:
			if (dev_data->ops && dev_data->ops->set_report) {
				return dev_data->ops->set_report(dev, setup,
								 len, data);
			} else {
				return hid_on_set_report(dev_data, setup, len,
							 data);
			}
			break;
		case USB_HID_SET_PROTOCOL:
			if (dev_data->ops && dev_data->ops->set_protocol) {
				return dev_data->ops->set_protocol(dev, setup,
								   len, data);
			} else {
				return hid_on_set_protocol(dev, dev_data,
							   setup);
			}
			break;
		default:
			LOG_ERR("Unhandled request 0x%02x", setup->bRequest);
			break;
		}
	}

	return -ENOTSUP;
}

static int hid_custom_handle_req(struct usb_setup_packet *setup,
				 int32_t *len, uint8_t **data)
{
	LOG_DBG("Standard request:"
		"bRequest 0x%02x, bmRequestType 0x%02x, len %d",
		setup->bRequest, setup->bmRequestType, *len);

	if (REQTYPE_GET_DIR(setup->bmRequestType) == REQTYPE_DIR_TO_HOST &&
	    REQTYPE_GET_RECIP(setup->bmRequestType) ==
					REQTYPE_RECIP_INTERFACE &&
					setup->bRequest == REQ_GET_DESCRIPTOR) {
		uint8_t value = (uint8_t)(setup->wValue >> 8);
		uint8_t iface_num = (uint8_t)setup->wIndex;
		struct hid_device_info *dev_data;
		struct usb_dev_data *common;
		const struct usb_cfg_data *cfg;
		const struct usb_hid_config *hid_desc;

		common = usb_get_dev_data_by_iface(&usb_hid_devlist, iface_num);
		if (common == NULL) {
			LOG_WRN("Device data not found for interface %u",
				iface_num);
			return -EINVAL;
		}

		dev_data = CONTAINER_OF(common, struct hid_device_info, common);

		switch (value) {
		case USB_DESC_HID:
			cfg = common->dev->config;
			hid_desc = cfg->interface_descriptor;

			LOG_DBG("Return HID Descriptor");

			*len = MIN(*len, hid_desc->if0_hid.bLength);
			*data = (uint8_t *)&hid_desc->if0_hid;
			break;
		case USB_DESC_HID_REPORT:
			LOG_DBG("Return Report Descriptor");

			/* Some buggy system may be pass a larger wLength when
			 * it try read HID report descriptor, although we had
			 * already tell it the right descriptor size.
			 * So truncated wLength if it doesn't match. */
			if (*len != dev_data->report_size) {
				LOG_WRN("len %d doesn't match "
					"Report Descriptor size", *len);
				*len = MIN(*len, dev_data->report_size);
			}
			*data = (uint8_t *)dev_data->report_desc;
			break;
		default:
			return -ENOTSUP;
		}

		return 0;
	}

	return -EINVAL;
}

static void hid_int_in(uint8_t ep, enum usb_dc_ep_cb_status_code ep_status)
{
	struct hid_device_info *dev_data;
	struct usb_dev_data *common;

	common = usb_get_dev_data_by_ep(&usb_hid_devlist, ep);
	if (common == NULL) {
		LOG_WRN("Device data not found for endpoint %u", ep);
		return;
	}

	dev_data = CONTAINER_OF(common, struct hid_device_info, common);

	if (ep_status != USB_DC_EP_DATA_IN || dev_data->ops == NULL ||
	    dev_data->ops->int_in_ready == NULL) {
		return;
	}

	dev_data->ops->int_in_ready(common->dev);
}

#ifdef CONFIG_ENABLE_HID_INT_OUT_EP
static void hid_int_out(uint8_t ep, enum usb_dc_ep_cb_status_code ep_status)
{
	struct hid_device_info *dev_data;
	struct usb_dev_data *common;

	common = usb_get_dev_data_by_ep(&usb_hid_devlist, ep);
	if (common == NULL) {
		LOG_WRN("Device data not found for endpoint %u", ep);
		return;
	}

	dev_data = CONTAINER_OF(common, struct hid_device_info, common);

	if (ep_status != USB_DC_EP_DATA_OUT || dev_data->ops == NULL ||
	    dev_data->ops->int_out_ready == NULL) {
		return;
	}

	dev_data->ops->int_out_ready(common->dev);
}
#endif

#define INITIALIZER_EP_DATA(cb, addr)			\
	{						\
		.ep_cb = cb,				\
		.ep_addr = addr,			\
	}

/* Describe Endpoints configuration */
#ifdef CONFIG_ENABLE_HID_INT_OUT_EP
#define DEFINE_HID_EP(x, _)						\
	static struct usb_ep_cfg_data hid_ep_data_##x[] = {		\
		INITIALIZER_EP_DATA(hid_int_in, AUTO_EP_IN),		\
		INITIALIZER_EP_DATA(hid_int_out, AUTO_EP_OUT),		\
	};
#else
#define DEFINE_HID_EP(x, _)						\
	static struct usb_ep_cfg_data hid_ep_data_##x[] = {		\
		INITIALIZER_EP_DATA(hid_int_in, AUTO_EP_IN),		\
	};
#endif

static void hid_interface_config(struct usb_desc_header *head,
				 uint8_t bInterfaceNumber)
{
	struct usb_if_descriptor *if_desc = (struct usb_if_descriptor *)head;
	struct usb_hid_config *desc =
		CONTAINER_OF(if_desc, struct usb_hid_config, if0);

	LOG_DBG("");

	desc->if0.bInterfaceNumber = bInterfaceNumber;
#ifdef CONFIG_ENABLE_HID_INT_OUT_EP
	desc->if0.bNumEndpoints = 2;
#endif
}

#define DEFINE_HID_CFG_DATA(x, _)					\
	USBD_CFG_DATA_DEFINE(primary, hid)				\
	struct usb_cfg_data hid_config_##x = {				\
		.usb_device_description = NULL,				\
		.interface_config = hid_interface_config,		\
		.interface_descriptor = &hid_cfg_##x.if0,		\
		.cb_usb_status = hid_status_cb,				\
		.interface = {						\
			.class_handler = hid_class_handle_req,		\
			.custom_handler = hid_custom_handle_req,	\
		},							\
		.num_endpoints = ARRAY_SIZE(hid_ep_data_##x),		\
		.endpoint = hid_ep_data_##x,				\
	};

int usb_hid_init(const struct device *dev)
{
	struct usb_cfg_data *cfg = (void *)dev->config;
	struct hid_device_info *dev_data = dev->data;

	LOG_DBG("Initializing HID Device: dev %p", dev);

	/*
	 * Modify Report Descriptor Size
	 */
	usb_set_hid_report_size(cfg, dev_data->report_size);

	return 0;
}

void usb_hid_register_device(const struct device *dev, const uint8_t *desc,
			     size_t size, const struct hid_ops *ops)
{
	struct hid_device_info *dev_data = dev->data;

	dev_data->report_desc = desc;
	dev_data->report_size = size;

	dev_data->ops = ops;
	dev_data->common.dev = dev;

	sys_slist_append(&usb_hid_devlist, &dev_data->common.node);

	LOG_DBG("Added dev_data %p dev %p to devlist %p", dev_data, dev,
		&usb_hid_devlist);
}

int hid_int_ep_write(const struct device *dev, const uint8_t *data, uint32_t data_len,
		     uint32_t *bytes_ret)
{
	const struct usb_cfg_data *cfg = dev->config;
	struct hid_device_info *hid_dev_data = dev->data;

	if (hid_dev_data->configured && !hid_dev_data->suspended) {
		return usb_write(cfg->endpoint[HID_INT_IN_EP_IDX].ep_addr, data,
			 data_len, bytes_ret);
	} else {
		LOG_WRN("Device is not configured");
		return -EAGAIN;
	}

}

int hid_int_ep_read(const struct device *dev, uint8_t *data, uint32_t max_data_len,
		    uint32_t *ret_bytes)
{
#ifdef CONFIG_ENABLE_HID_INT_OUT_EP
	const struct usb_cfg_data *cfg = dev->config;

	return usb_read(cfg->endpoint[HID_INT_OUT_EP_IDX].ep_addr,
			data, max_data_len, ret_bytes);
#else
	return -ENOTSUP;
#endif
}

static const struct usb_hid_device_api {
	void (*init)(void);
} hid_api;

static int usb_hid_device_init(const struct device *dev)
{
	LOG_DBG("Init HID Device: dev %p (%s)", dev, dev->name);

	return 0;
}

#define DEFINE_HID_DEV_DATA(x, _)					\
	struct hid_device_info usb_hid_dev_data_##x;

#define DEFINE_HID_DEVICE(x, _)						\
	DEVICE_DEFINE(usb_hid_device_##x,				\
			    CONFIG_USB_HID_DEVICE_NAME "_" #x,		\
			    &usb_hid_device_init,			\
			    device_pm_control_nop,			\
			    &usb_hid_dev_data_##x,			\
			    &hid_config_##x, POST_KERNEL,		\
			    CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,	\
			    &hid_api);

UTIL_LISTIFY(CONFIG_USB_HID_DEVICE_COUNT, DEFINE_HID_DESCR, _)
UTIL_LISTIFY(CONFIG_USB_HID_DEVICE_COUNT, DEFINE_HID_EP, _)
UTIL_LISTIFY(CONFIG_USB_HID_DEVICE_COUNT, DEFINE_HID_CFG_DATA, _)
UTIL_LISTIFY(CONFIG_USB_HID_DEVICE_COUNT, DEFINE_HID_DEV_DATA, _)
UTIL_LISTIFY(CONFIG_USB_HID_DEVICE_COUNT, DEFINE_HID_DEVICE, _)
