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

#include <sys/byteorder.h>
#include <usb_device.h>
#include <usb_common.h>

#include <usb_descriptor.h>
#include <class/usb_hid.h>

#include <stdlib.h>

#define HID_INT_IN_EP_IDX		0
#define HID_INT_OUT_EP_IDX		1

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
	struct usb_dev_data common;
};

static sys_slist_t usb_hid_devlist;

static int hid_on_get_idle(struct hid_device_info *dev_data,
			   struct usb_setup_packet *setup, s32_t *len,
			   u8_t **data)
{
#ifdef CONFIG_USB_DEVICE_SOF
	u8_t report_id = sys_le16_to_cpu(setup->wValue) & 0xFF;

	if (report_id > CONFIG_USB_HID_REPORTS) {
		LOG_ERR("Report id out of limit: %d", report_id);
		return -ENOTSUP;
	}

	u32_t size = sizeof(dev_data->idle_rate[report_id]);

	LOG_DBG("Get Idle callback, report_id: %d", report_id);

	*data = &dev_data->idle_rate[report_id];
	len = &size;
	return 0;
#else
	return -ENOTSUP;
#endif
}

static int hid_on_get_report(struct hid_device_info *dev_data,
			     struct usb_setup_packet *setup, s32_t *len,
			     u8_t **data)
{
	LOG_DBG("Get Report callback");

	/* TODO: Do something. */

	return -ENOTSUP;
}

static int hid_on_get_protocol(struct hid_device_info *dev_data,
			       struct usb_setup_packet *setup, s32_t *len,
			       u8_t **data)
{
#ifdef CONFIG_USB_HID_BOOT_PROTOCOL
	if (setup->wValue) {
		LOG_ERR("wValue should be 0");
		return -ENOTSUP;
	}

	u32_t size = sizeof(dev_data->protocol);

	LOG_DBG("Get Protocol callback, protocol: %d", dev_data->protocol);

	*data = &dev_data->protocol;
	len = &size;
	return 0;
#else
	return -ENOTSUP;
#endif
}

static int hid_on_set_idle(struct hid_device_info *dev_data,
			   struct usb_setup_packet *setup, s32_t *len,
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

	dev_data->idle_rate[report_id] = rate;

	if (rate == 0U) {
		/* Clear idle */
		bool clear = true;

		for (u16_t i = 1; i <= CONFIG_USB_HID_REPORTS; i++) {
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
			     struct usb_setup_packet *setup, s32_t *len,
			     u8_t **data)
{
	LOG_DBG("Set Report callback");

	/* TODO: Do something. */

	return -ENOTSUP;
}

static int hid_on_set_protocol(struct hid_device_info *dev_data,
			       struct usb_setup_packet *setup, s32_t *len,
			       u8_t **data)
{
#ifdef CONFIG_USB_HID_BOOT_PROTOCOL
	u16_t protocol = sys_le16_to_cpu(setup->wValue);

	if (protocol > HID_PROTOCOL_REPORT) {
		LOG_ERR("Unsupported protocol: %u", protocol);
		return -ENOTSUP;
	}

	LOG_DBG("Set Protocol callback, protocol: %u", protocol);

	if (dev_data->protocol != protocol) {
		dev_data->protocol = protocol;

		if (dev_data->ops && dev_data->ops->protocol_change) {
			dev_data->ops->protocol_change(protocol);
		}
	}

	return 0;
#else
	return -ENOTSUP;
#endif
}

static void usb_set_hid_report_size(const struct usb_cfg_data *cfg, u16_t size)
{
	struct usb_if_descriptor *if_desc = (void *)cfg->interface_descriptor;
	struct usb_hid_config *desc =
			CONTAINER_OF(if_desc, struct usb_hid_config, if0);

	LOG_DBG("if_desc %p desc %p size %u", if_desc, desc, size);

	sys_put_le16(size,
		     (u8_t *)&(desc->if0_hid.subdesc[0].wDescriptorLength));
}

#ifdef CONFIG_USB_DEVICE_SOF
void hid_clear_idle_ctx(struct hid_device_info *dev_data)
{
	dev_data->idle_on = false;
	dev_data->idle_id_report = false;
	for (u16_t i = 0; i <= CONFIG_USB_HID_REPORTS; i++) {
		dev_data->sof_cnt[i] = 0U;
		dev_data->idle_rate[i] = 0U;
	}
}

void hid_sof_handler(struct hid_device_info *dev_data)
{
	for (u16_t i = 0; i <= CONFIG_USB_HID_REPORTS; i++) {
		if (dev_data->idle_rate[i]) {
			dev_data->sof_cnt[i]++;
		}

		u32_t diff = abs((dev_data->idle_rate[i] * 4U)
				 - dev_data->sof_cnt[i]);

		if (diff < (2 + (dev_data->idle_rate[i] / 10U))) {
			dev_data->sof_cnt[i] = 0U;
			if (dev_data->ops && dev_data->ops->on_idle) {
				dev_data->ops->on_idle(i);
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
			     const u8_t *param)
{
	switch (status) {
	case USB_DC_ERROR:
		LOG_DBG("USB device error");
		break;
	case USB_DC_RESET:
		LOG_DBG("USB device reset detected");
#ifdef CONFIG_USB_HID_BOOT_PROTOCOL
		dev_data->protocol = HID_PROTOCOL_REPORT;
#endif
#ifdef CONFIG_USB_DEVICE_SOF
		hid_clear_idle_ctx(dev_data);
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
		if (dev_data->idle_on) {
			hid_sof_handler(dev_data);
		}
#endif
		break;
	case USB_DC_UNKNOWN:
	default:
		LOG_DBG("USB unknown state");
		break;
	}

	if (dev_data->ops && dev_data->ops->status_cb) {
		dev_data->ops->status_cb(status, param);
	}
}

static void hid_status_cb(struct usb_cfg_data *cfg,
			  enum usb_dc_status_code status,
			  const u8_t *param)
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
				s32_t *len, u8_t **data)
{
	struct hid_device_info *dev_data;
	struct usb_dev_data *common;

	LOG_DBG("Class request: bRequest 0x%x bmRequestType 0x%x len %d",
		setup->bRequest, setup->bmRequestType, *len);

	common = usb_get_dev_data_by_iface(&usb_hid_devlist,
					   sys_le16_to_cpu(setup->wIndex));
	if (common == NULL) {
		LOG_WRN("Device data not found for interface %u",
			sys_le16_to_cpu(setup->wIndex));
		return -ENODEV;
	}

	dev_data = CONTAINER_OF(common, struct hid_device_info, common);

	if (REQTYPE_GET_DIR(setup->bmRequestType) == REQTYPE_DIR_TO_HOST) {
		switch (setup->bRequest) {
		case HID_GET_IDLE:
			if (dev_data->ops && dev_data->ops->get_idle) {
				return dev_data->ops->get_idle(setup, len,
							       data);
			} else {
				return hid_on_get_idle(dev_data, setup, len,
						       data);
			}
			break;
		case HID_GET_REPORT:
			if (dev_data->ops && dev_data->ops->get_report) {
				return dev_data->ops->get_report(setup, len,
								 data);
			} else {
				return hid_on_get_report(dev_data, setup, len,
							 data);
			}
			break;
		case HID_GET_PROTOCOL:
			if (dev_data->ops && dev_data->ops->get_protocol) {
				return dev_data->ops->get_protocol(setup, len,
								   data);
			} else {
				return hid_on_get_protocol(dev_data, setup, len,
							   data);
			}
			break;
		default:
			LOG_ERR("Unhandled request 0x%x", setup->bRequest);
			break;
		}
	} else {
		switch (setup->bRequest) {
		case HID_SET_IDLE:
			if (dev_data->ops && dev_data->ops->set_idle) {
				return dev_data->ops->set_idle(setup, len,
							       data);
			} else {
				return hid_on_set_idle(dev_data, setup, len,
						       data);
			}
			break;
		case HID_SET_REPORT:
			if (dev_data->ops && dev_data->ops->set_report) {
				return dev_data->ops->set_report(setup, len,
								 data);
			} else {
				return hid_on_set_report(dev_data, setup, len,
							 data);
			}
			break;
		case HID_SET_PROTOCOL:
			if (dev_data->ops && dev_data->ops->set_protocol) {
				return dev_data->ops->set_protocol(setup, len,
								   data);
			} else {
				return hid_on_set_protocol(dev_data, setup, len,
							   data);
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
		u8_t iface_num = sys_le16_to_cpu(setup->wIndex);
		struct hid_device_info *dev_data;
		struct usb_dev_data *common;
		const struct usb_cfg_data *cfg;
		const struct usb_hid_config *hid_desc;

		common = usb_get_dev_data_by_iface(&usb_hid_devlist, iface_num);
		if (common == NULL) {
			LOG_WRN("Device data not found for interface %u",
				sys_le16_to_cpu(setup->wIndex));
			return -ENODEV;
		}

		dev_data = CONTAINER_OF(common, struct hid_device_info, common);

		switch (value) {
		case HID_CLASS_DESCRIPTOR_HID:
			cfg = common->dev->config->config_info;
			hid_desc = cfg->interface_descriptor;

			LOG_DBG("Return HID Descriptor");

			*len = MIN(*len, hid_desc->if0_hid.bLength);
			*data = (u8_t *)&hid_desc->if0_hid;
			break;
		case HID_CLASS_DESCRIPTOR_REPORT:
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
			*data = (u8_t *)dev_data->report_desc;
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

	dev_data->ops->int_in_ready();
}

#ifdef CONFIG_ENABLE_HID_INT_OUT_EP
static void hid_int_out(u8_t ep, enum usb_dc_ep_cb_status_code ep_status)
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

	dev_data->ops->int_out_ready();
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
				 u8_t bInterfaceNumber)
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
	struct usb_cfg_data *cfg = (void *)dev->config->config_info;
	struct hid_device_info *dev_data = dev->driver_data;

	LOG_DBG("Initializing HID Device: dev %p", dev);

	/*
	 * Modify Report Descriptor Size
	 */
	usb_set_hid_report_size(cfg, dev_data->report_size);

	return 0;
}

void usb_hid_register_device(struct device *dev, const u8_t *desc,
			     size_t size, const struct hid_ops *ops)
{
	struct hid_device_info *dev_data = dev->driver_data;

	dev_data->report_desc = desc;
	dev_data->report_size = size;

	dev_data->ops = ops;
	dev_data->common.dev = dev;

	sys_slist_append(&usb_hid_devlist, &dev_data->common.node);

	LOG_DBG("Added dev_data %p dev %p to devlist %p", dev_data, dev,
		&usb_hid_devlist);
}

int hid_int_ep_write(const struct device *dev, const u8_t *data, u32_t data_len,
		     u32_t *bytes_ret)
{
	const struct usb_cfg_data *cfg = dev->config->config_info;

	return usb_write(cfg->endpoint[HID_INT_IN_EP_IDX].ep_addr, data,
			 data_len, bytes_ret);
}

int hid_int_ep_read(const struct device *dev, u8_t *data, u32_t max_data_len,
		    u32_t *ret_bytes)
{
#ifdef CONFIG_ENABLE_HID_INT_OUT_EP
	const struct usb_cfg_data *cfg = dev->config->config_info;

	return usb_read(cfg->endpoint[HID_INT_OUT_EP_IDX].ep_addr,
			data, max_data_len, ret_bytes);
#else
	return -ENOTSUP;
#endif
}

static const struct usb_hid_device_api {
	void (*init)(void);
} hid_api;

static int usb_hid_device_init(struct device *dev)
{
	LOG_DBG("Init HID Device: dev %p (%s)", dev, dev->config->name);

	return 0;
}

#define DEFINE_HID_DEV_DATA(x, _)					\
	struct hid_device_info usb_hid_dev_data_##x;

#define DEFINE_HID_DEVICE(x, _)						\
	DEVICE_AND_API_INIT(usb_hid_device_##x,				\
			    CONFIG_USB_HID_DEVICE_NAME "_" #x,		\
			    &usb_hid_device_init,			\
			    &usb_hid_dev_data_##x,			\
			    &hid_config_##x, POST_KERNEL,		\
			    CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,	\
			    &hid_api);

UTIL_LISTIFY(CONFIG_USB_HID_DEVICE_COUNT, DEFINE_HID_DESCR, _)
UTIL_LISTIFY(CONFIG_USB_HID_DEVICE_COUNT, DEFINE_HID_EP, _)
UTIL_LISTIFY(CONFIG_USB_HID_DEVICE_COUNT, DEFINE_HID_CFG_DATA, _)
UTIL_LISTIFY(CONFIG_USB_HID_DEVICE_COUNT, DEFINE_HID_DEV_DATA, _)
UTIL_LISTIFY(CONFIG_USB_HID_DEVICE_COUNT, DEFINE_HID_DEVICE, _)
