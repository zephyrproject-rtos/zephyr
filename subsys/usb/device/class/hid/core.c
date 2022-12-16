/*
 * Human Interface Device (HID) USB class core
 *
 * Copyright (c) 2018 Intel Corporation
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(usb_hid, CONFIG_USB_HID_LOG_LEVEL);

#include <zephyr/sys/byteorder.h>
#include <usb_device.h>

#include <usb_descriptor.h>
#include <class/usb_hid.h>

#include <stdlib.h>

#define DT_DRV_COMPAT zephyr_usb_hid

#define HID_INT_IN_EP_IDX		0
#define HID_INT_OUT_EP_IDX		1

/* Helpers used to compile in code if at least 1 interface supports
 * that feature
 */
#define HID_BOOT_HELPER(inst)	DT_INST_PROP(inst, boot_protocol) ||
#define HID_ANY_INST_HAS_BOOT	DT_INST_FOREACH_STATUS_OKAY(HID_BOOT_HELPER) 0

#define HID_OUT_HELPER(inst)	DT_INST_PROP(inst, int_out_ep) ||
#define HID_ANY_INST_HAS_OUT	DT_INST_FOREACH_STATUS_OKAY(HID_OUT_HELPER) 0

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
} __packed;

/* USB HID can contain an optional OUT EP */
#if HID_ANY_INST_HAS_OUT
struct usb_hid_config_out {
	struct usb_if_descriptor if0;
	struct usb_hid_descriptor if0_hid;
	struct usb_ep_descriptor if0_int_in_ep;
	struct usb_ep_descriptor if0_int_out_ep;
} __packed;
#endif

#define INITIALIZER_IF(int_out_en, boot_protocol)			\
	{								\
		.bLength = sizeof(struct usb_if_descriptor),		\
		.bDescriptorType = USB_DESC_INTERFACE,			\
		.bInterfaceNumber = 0,					\
		.bAlternateSetting = 0,					\
		.bNumEndpoints = int_out_en ? 2 : 1,			\
		.bInterfaceClass = USB_BCC_HID,				\
		.bInterfaceSubClass = boot_protocol ?			\
			USB_HID_BOOT_SUBCLASS : USB_HID_NO_SUBCLASS,	\
		.bInterfaceProtocol = 0,				\
		.iInterface = 0,					\
	}

struct hid_device_info {
	const uint8_t *report_desc;
	size_t report_size;
	const struct hid_ops *ops;
#ifdef CONFIG_USB_DEVICE_SOF
	uint32_t sof_cnt[CONFIG_USB_HID_REPORTS];
	bool idle_on;
	uint8_t idle_rate[CONFIG_USB_HID_REPORTS];
#endif
#if HID_ANY_INST_HAS_BOOT
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

	LOG_DBG("Get Idle callback, report_id: %d", report_id);

	if (report_id == 0U) {
		/*
		 * Common value, stored on Set Idle request with Report ID 0,
		 * can be outdated because the duration can also
		 * be set individually for each Report ID.
		 */
		*data = &dev_data->idle_rate[0];
		*len = sizeof(dev_data->idle_rate[0]);
	} else {
		*data = &dev_data->idle_rate[report_id - 1];
		*len = sizeof(dev_data->idle_rate[report_id - 1]);
	}

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
#if HID_ANY_INST_HAS_BOOT
	struct device *dev = CONTAINER_OF(dev_data, struct device, data);
	const struct usb_cfg_data *cfg = dev->config;
	struct usb_if_descriptor *if_desc = cfg->interface_descriptor;

	if (if_desc->bInterfaceSubClass == USB_HID_BOOT_SUBCLASS) {
		uint32_t size = sizeof(dev_data->protocol);

		if (setup->wValue) {
			LOG_ERR("wValue should be 0");
			return -ENOTSUP;
		}

		LOG_DBG("Get Protocol: %d", dev_data->protocol);

		*data = &dev_data->protocol;
		*len = size;
		return 0;
	}
#endif
	return -ENOTSUP;
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

	if (report_id == 0U) {
		for (uint16_t i = 0; i < CONFIG_USB_HID_REPORTS; i++) {
			dev_data->idle_rate[i] = rate;
			dev_data->sof_cnt[i] = 0U;
		}
	} else {
		dev_data->idle_rate[report_id - 1] = rate;
		dev_data->sof_cnt[report_id - 1] = 0U;
	}

	dev_data->idle_on = (bool)setup->wValue;

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
#if HID_ANY_INST_HAS_BOOT
	const struct usb_cfg_data *cfg = dev->config;
	struct usb_if_descriptor *if_desc = cfg->interface_descriptor;

	if (if_desc->bInterfaceSubClass == USB_HID_BOOT_SUBCLASS) {
		uint16_t protocol = setup->wValue;

		if (protocol > HID_PROTOCOL_REPORT) {
			LOG_ERR("Unsupported protocol: %u", protocol);
			return -ENOTSUP;
		}

		LOG_DBG("Set Protocol: %u", protocol);

		if (dev_data->protocol != protocol) {
			dev_data->protocol = protocol;

			if (dev_data->ops && dev_data->ops->protocol_change) {
				dev_data->ops->protocol_change(dev, protocol);
			}
		}

		return 0;
	}
#endif
	return -ENOTSUP;
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
	for (uint16_t i = 0; i < CONFIG_USB_HID_REPORTS; i++) {
		dev_data->sof_cnt[i] = 0U;
		dev_data->idle_rate[i] = 0U;
	}
}

void hid_sof_handler(struct hid_device_info *dev_data)
{
	const struct device *dev = dev_data->common.dev;
	bool reported = false;

	if (dev_data->ops == NULL || dev_data->ops->on_idle == NULL) {
		return;
	}

	for (uint16_t i = 0; i < CONFIG_USB_HID_REPORTS; i++) {
		if (dev_data->idle_rate[i]) {
			dev_data->sof_cnt[i]++;
		} else {
			continue;
		}

		int32_t diff = abs((int32_t) ((uint32_t) dev_data->idle_rate[i] * 4U -
					      dev_data->sof_cnt[i]));

		if (diff < 2 && reported == false) {
			dev_data->sof_cnt[i] = 0U;
			/*
			 * We can submit only one report at a time
			 * because we have only one endpoint and
			 * there is no queue for the packets/reports.
			 */
			reported = true;
			dev_data->ops->on_idle(dev, i + 1);
		} else if (diff == 0 && reported == true) {
			/* Delay it once until it has spread. */
			dev_data->sof_cnt[i]--;
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
#if HID_ANY_INST_HAS_BOOT
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

	if (usb_reqtype_is_to_host(setup)) {
		switch (setup->bRequest) {
		case USB_HID_GET_IDLE:
			return hid_on_get_idle(dev_data, setup, len, data);
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
			return hid_on_get_protocol(dev_data, setup, len, data);
		default:
			LOG_ERR("Unhandled request 0x%02x", setup->bRequest);
			break;
		}
	} else {
		switch (setup->bRequest) {
		case USB_HID_SET_IDLE:
			return hid_on_set_idle(dev_data, setup, len, data);
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
			return hid_on_set_protocol(dev, dev_data, setup);
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
		setup->bRequest, setup->bmRequestType, setup->wLength);

	if (usb_reqtype_is_to_host(setup) &&
	    setup->RequestType.recipient == USB_REQTYPE_RECIPIENT_INTERFACE &&
	    setup->bRequest == USB_SREQ_GET_DESCRIPTOR) {
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

			*len = MIN(setup->wLength, hid_desc->if0_hid.bLength);
			*data = (uint8_t *)&hid_desc->if0_hid;
			break;
		case USB_DESC_HID_REPORT:
			LOG_DBG("Return Report Descriptor");

			*len = MIN(setup->wLength, dev_data->report_size);
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

#if HID_ANY_INST_HAS_OUT
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

static void hid_interface_config(struct usb_desc_header *head,
				 uint8_t bInterfaceNumber)
{
	struct usb_if_descriptor *if_desc = (struct usb_if_descriptor *)head;
	struct usb_hid_config *desc =
		CONTAINER_OF(if_desc, struct usb_hid_config, if0);

	desc->if0.bInterfaceNumber = bInterfaceNumber;
}

int usb_hid_set_proto_code(const struct device *dev, uint8_t proto_code)
{
#if HID_ANY_INST_HAS_BOOT
	const struct usb_cfg_data *cfg = dev->config;
	struct usb_if_descriptor *if_desc = cfg->interface_descriptor;

	if (if_desc->bInterfaceSubClass == USB_HID_BOOT_SUBCLASS) {
		if_desc->bInterfaceProtocol = proto_code;
		return 0;
	}
#endif
	return -ENOTSUP;
}

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
#if HID_ANY_INST_HAS_OUT
	const struct usb_cfg_data *cfg = dev->config;

	/* if EP is 2, then that must mean there is an out EP */
	if (cfg->num_endpoints == 2) {
		return usb_read(cfg->endpoint[HID_INT_OUT_EP_IDX].ep_addr,
				data, max_data_len, ret_bytes);
	}
#endif
	return -ENOTSUP;
}

static const struct usb_hid_device_api {
	void (*init)(void);
} hid_api;

static int usb_hid_device_init(const struct device *dev)
{
	LOG_DBG("Init HID Device: dev %p (%s)", dev, dev->name);

	return 0;
}

/* Descriptor length needs to be set after initialization */
#define INITIALIZER_IF_HID						\
	{								\
		.bLength = sizeof(struct usb_hid_descriptor),		\
		.bDescriptorType = USB_DESC_HID,			\
		.bcdHID = sys_cpu_to_le16(USB_HID_VERSION),		\
		.bCountryCode = 0,					\
		.bNumDescriptors = 1,					\
		.subdesc[0] = {						\
			.bDescriptorType = USB_DESC_HID_REPORT,	\
			.wDescriptorLength = 0,				\
		},							\
	}

#define INITIALIZER_IF_EP(addr, attr, mps, poll_interval)		\
	{								\
		.bLength = sizeof(struct usb_ep_descriptor),		\
		.bDescriptorType = USB_DESC_ENDPOINT,			\
		.bEndpointAddress = addr,				\
		.bmAttributes = attr,					\
		.wMaxPacketSize = sys_cpu_to_le16(mps),			\
		.bInterval = poll_interval,				\
	}

#define DEFINE_HID_DESCR(inst, int_out_en, boot_protocol, ep_mps_in,	\
	poll_interval_in, ep_mps_out, poll_interval_out)		\
	USBD_CLASS_DESCR_DEFINE(primary, inst)				\
	COND_CODE_1(int_out_en,						\
	(struct usb_hid_config_out hid_cfg_##inst =			\
	{								\
	/* Interface descriptor */					\
	.if0 = INITIALIZER_IF(int_out_en, boot_protocol),		\
	.if0_hid = INITIALIZER_IF_HID,					\
	.if0_int_in_ep =						\
		INITIALIZER_IF_EP(AUTO_EP_IN,				\
				  USB_DC_EP_INTERRUPT,			\
				  ep_mps_in,				\
				  poll_interval_in),			\
	.if0_int_out_ep =						\
		INITIALIZER_IF_EP(AUTO_EP_OUT,				\
				  USB_DC_EP_INTERRUPT,			\
				  ep_mps_out,				\
				  poll_interval_out),			\
	}),								\
	(struct usb_hid_config hid_cfg_##inst =	{			\
	/* Interface descriptor */					\
	.if0 = INITIALIZER_IF(int_out_en, boot_protocol),		\
	.if0_hid = INITIALIZER_IF_HID,					\
	.if0_int_in_ep =						\
		INITIALIZER_IF_EP(AUTO_EP_IN,				\
				  USB_DC_EP_INTERRUPT,			\
				  ep_mps_in,				\
				  poll_interval_in),			\
	}))

#define INITIALIZER_EP_DATA(cb, addr)			\
	{						\
		.ep_cb = cb,				\
		.ep_addr = addr,			\
	}

/* Describe Endpoints configuration */
#define DEFINE_HID_EP(inst, int_out_en)					\
	static struct usb_ep_cfg_data hid_ep_data_##inst[] =		\
	COND_CODE_1(int_out_en,						\
	({								\
		INITIALIZER_EP_DATA(hid_int_in, AUTO_EP_IN),		\
		INITIALIZER_EP_DATA(hid_int_out, AUTO_EP_OUT),		\
	}),								\
	({								\
		INITIALIZER_EP_DATA(hid_int_in, AUTO_EP_IN),		\
	}))

#define USB_HID_DT_DEVICE_DEFINE(idx)					\
	DEFINE_HID_DESCR(idx, DT_INST_PROP(idx, int_out_ep),		\
		DT_INST_PROP(idx, boot_protocol),			\
		DT_INST_PROP(idx, ep_mps_in),				\
		DT_INST_PROP(idx, poll_interval_in),			\
		DT_INST_PROP(idx, ep_mps_out),				\
		DT_INST_PROP(idx, poll_interval_out));			\
	DEFINE_HID_EP(idx, DT_INST_PROP(idx, int_out_ep));		\
									\
	struct hid_device_info usb_hid_dev_data_##idx;			\
	USBD_DEFINE_CFG_DATA(hid_config_##idx) = {			\
		.usb_device_description = NULL,				\
		.interface_config = hid_interface_config,		\
		.interface_descriptor = &hid_cfg_##idx.if0,		\
		.cb_usb_status = hid_status_cb,				\
		.interface = {						\
			.class_handler = hid_class_handle_req,		\
			.custom_handler = hid_custom_handle_req,	\
		},							\
		.num_endpoints = ARRAY_SIZE(hid_ep_data_##idx),		\
		.endpoint = hid_ep_data_##idx,				\
	};								\
									\
	DEVICE_DT_INST_DEFINE(idx, usb_hid_device_init, NULL,		\
		&usb_hid_dev_data_##idx, &hid_config_##idx,		\
		POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,	\
		&hid_api);

DT_INST_FOREACH_STATUS_OKAY(USB_HID_DT_DEVICE_DEFINE);
