/*
 * Copyright (c) 2026 Adrien RICCIARDI
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/drivers/usb/udc.h>
#include <zephyr/logging/log.h>
#include <zephyr/usb/class/usbd_xbox360_controller.h>
#include <zephyr/usb/usbd.h>

LOG_MODULE_REGISTER(usbd_xbox360, CONFIG_USBD_XBOX360_CONTROLLER_LOG_LEVEL);

#define DT_DRV_COMPAT microsoft_xbox360_controller_device

/* All custom vendor setup requests */
#define XBOX360_USB_VENDOR_REQUEST_01 0x01

/*
 * Based on
 * https://www.partsnotincluded.com/understanding-the-xbox-360-wired-controllers-usb-data/
 */
struct xbox360_usb_descriptor {
	/* Interface 0 */
	struct usb_if_descriptor control_data_interface;
	unsigned char unknown_descriptor_0[17];
	struct usb_ep_descriptor control_surface_in_endpoint;
	struct usb_ep_descriptor control_surface_out_endpoint;

	/* Interface 1 */
	struct usb_if_descriptor interface_1;
	unsigned char unknown_descriptor_1[27];
	struct usb_ep_descriptor iface_1_endpoint_0;
	struct usb_ep_descriptor iface_1_endpoint_1;
	struct usb_ep_descriptor iface_1_endpoint_2;
	struct usb_ep_descriptor iface_1_endpoint_3;

	/* Interface 2 */
	struct usb_if_descriptor interface_2;
	unsigned char unknown_descriptor_2[9];
	struct usb_ep_descriptor iface_2_endpoint_0;

	/* Interface 3 */
	struct usb_if_descriptor interface_3;
	unsigned char unknown_descriptor_3[6];
};

struct xbox360_device_data {
	struct xbox360_controller_control_surface control_surface;
	struct k_work work_queue;
	const struct device *dev;
	const struct xbox360_controller_device_ops *device_ops;
};

struct xbox360_device_config {
	struct usb_desc_header **usb_descriptor_headers;
	struct net_buf_pool *buffers_pool_in;
	struct usbd_class_data *usb_class_data;
};

static struct xbox360_usb_descriptor usb_descriptor = {
	.control_data_interface = {
		.bLength = sizeof(struct usb_if_descriptor),
		.bDescriptorType = USB_DESC_INTERFACE,
		.bInterfaceNumber = 0,
		.bAlternateSetting = 0,
		.bNumEndpoints = 2,
		.bInterfaceClass = 0xFF,
		.bInterfaceSubClass = 0x5D,
		.bInterfaceProtocol = 0x01,
		.iInterface = 0
	},
	.unknown_descriptor_0 = {
		0x11, /* bLength */
		0x21, /* bDescriptorType */
		0x00, 0x01, 0x01, 0x25, /* ? */
		0x81, /* bEndpointAddress (IN, 3) */
		0x14, /* bMaxDataSize */
		0x00, 0x00, 0x00, 0x00, 0x13, /* ? */
		0x01, /* bEndpointAddress (OUT, 1) */
		0x08, /* bMaxDataSize */
		0x00, 0x00 /* ? */
	},
	.control_surface_in_endpoint = {
		.bLength = sizeof(struct usb_ep_descriptor),
		.bDescriptorType = USB_DESC_ENDPOINT,
		.bEndpointAddress = 0x81, /* The USB stack automatically increments the address */
		.bmAttributes = USB_EP_TYPE_INTERRUPT,
		.wMaxPacketSize = 32,
		.bInterval = 4
	},
	.control_surface_out_endpoint = {
		.bLength = sizeof(struct usb_ep_descriptor),
		.bDescriptorType = USB_DESC_ENDPOINT,
		.bEndpointAddress = 0x01, /* The USB stack automatically increments the address */
		.bmAttributes = USB_EP_TYPE_INTERRUPT,
		.wMaxPacketSize = 32,
		.bInterval = 8
	},

	.interface_1 = {
		.bLength = sizeof(struct usb_if_descriptor),
		.bDescriptorType = USB_DESC_INTERFACE,
		.bInterfaceNumber = 0, /* The USB stack automatically increments this value */
		.bAlternateSetting = 0,
		.bNumEndpoints = 4,
		.bInterfaceClass = 0xFF,
		.bInterfaceSubClass = 0x5D,
		.bInterfaceProtocol = 0x03,
		.iInterface = 0
	},
	.unknown_descriptor_1 = {
		0x1B, /* bLength */
		0x21, /* bDescriptorType */
		0x00, 0x01, 0x01, 0x01, /* ? */
		0x82, /* bEndpointAddress (IN, 2) */
		0x40, /* bMaxDataSize */
		0x01, /* ? */
		0x02, /* bEndpointAddress (OUT, 2) */
		0x20, /* bMaxDataSize */
		0x16, /* ? */
		0x83, /* bEndpointAddress (IN, 3) */
		0x00, /* bMaxDataSize */
		0x00, 0x00, 0x00, 0x00, 0x00, 0x16, /* ? */
		0x03, /* bEndpointAddress (OUT, 3) */
		0x00, /* bMaxDataSize */
		0x00, 0x00, 0x00, 0x00, 0x00, /* ? */
	},
	.iface_1_endpoint_0 = {
		.bLength = sizeof(struct usb_ep_descriptor),
		.bDescriptorType = USB_DESC_ENDPOINT,
		.bEndpointAddress = 0x81, /* The USB stack automatically increments the address */
		.bmAttributes = USB_EP_TYPE_INTERRUPT,
		.wMaxPacketSize = 32,
		.bInterval = 2
	},
	.iface_1_endpoint_1 = {
		.bLength = sizeof(struct usb_ep_descriptor),
		.bDescriptorType = USB_DESC_ENDPOINT,
		.bEndpointAddress = 0x01, /* The USB stack automatically increments the address */
		.bmAttributes = USB_EP_TYPE_INTERRUPT,
		.wMaxPacketSize = 32,
		.bInterval = 4
	},
	.iface_1_endpoint_2 = {
		.bLength = sizeof(struct usb_ep_descriptor),
		.bDescriptorType = USB_DESC_ENDPOINT,
		.bEndpointAddress = 0x81, /* The USB stack automatically increments the address */
		.bmAttributes = USB_EP_TYPE_INTERRUPT,
		.wMaxPacketSize = 32,
		.bInterval = 64
	},
	.iface_1_endpoint_3 = {
		.bLength = sizeof(struct usb_ep_descriptor),
		.bDescriptorType = USB_DESC_ENDPOINT,
		.bEndpointAddress = 0x01, /* The USB stack automatically increments the address */
		.bmAttributes = USB_EP_TYPE_INTERRUPT,
		.wMaxPacketSize = 32,
		.bInterval = 16
	},

	.interface_2 = {
		.bLength = sizeof(struct usb_if_descriptor),
		.bDescriptorType = USB_DESC_INTERFACE,
		.bInterfaceNumber = 0, /* The USB stack automatically increments this value */
		.bAlternateSetting = 0,
		.bNumEndpoints = 1,
		.bInterfaceClass = 0xFF,
		.bInterfaceSubClass = 0x5D,
		.bInterfaceProtocol = 0x02,
		.iInterface = 0
	},
	.unknown_descriptor_2 = {
		0x09, /* bLength */
		0x21, /* bDescriptorType */
		0x00, 0x01, 0x01, 0x22, /* ? */
		0x84, /* bEndpointAddress (IN, 4) */
		0x07, /* bMaxDataSize */
		0x00 /* ? */
	},
	.iface_2_endpoint_0 = {
		.bLength = sizeof(struct usb_ep_descriptor),
		.bDescriptorType = USB_DESC_ENDPOINT,
		.bEndpointAddress = 0x81, /* The USB stack automatically increments the address */
		.bmAttributes = USB_EP_TYPE_INTERRUPT,
		.wMaxPacketSize = 32,
		.bInterval = 16
	},

	.interface_3 = {
		.bLength = sizeof(struct usb_if_descriptor),
		.bDescriptorType = USB_DESC_INTERFACE,
		.bInterfaceNumber = 0, /* The USB stack automatically increments this value */
		.bAlternateSetting = 0,
		.bNumEndpoints = 0,
		.bInterfaceClass = 0xFF,
		.bInterfaceSubClass = 0xFD,
		.bInterfaceProtocol = 0x13,
		.iInterface = 4
	},
	.unknown_descriptor_3 = {
		0x06, /* bLength */
		0x41, /* bDescriptorType */
		0x00, 0x01, 0x01, 0x03 /* ? */
	}
};

static struct usb_desc_header *xbox360_usb_descriptor_headers[] = {
	(struct usb_desc_header *)&usb_descriptor.control_data_interface,
	(struct usb_desc_header *)usb_descriptor.unknown_descriptor_0,
	(struct usb_desc_header *)&usb_descriptor.control_surface_in_endpoint,
	(struct usb_desc_header *)&usb_descriptor.control_surface_out_endpoint,
	(struct usb_desc_header *)&usb_descriptor.interface_1,
	(struct usb_desc_header *)usb_descriptor.unknown_descriptor_1,
	(struct usb_desc_header *)&usb_descriptor.iface_1_endpoint_0,
	(struct usb_desc_header *)&usb_descriptor.iface_1_endpoint_1,
	(struct usb_desc_header *)&usb_descriptor.iface_1_endpoint_2,
	(struct usb_desc_header *)&usb_descriptor.iface_1_endpoint_3,
	(struct usb_desc_header *)&usb_descriptor.interface_2,
	(struct usb_desc_header *)usb_descriptor.unknown_descriptor_2,
	(struct usb_desc_header *)&usb_descriptor.iface_2_endpoint_0,
	(struct usb_desc_header *)&usb_descriptor.interface_3,
	(struct usb_desc_header *)usb_descriptor.unknown_descriptor_3,
	NULL
};

static const struct usbd_cctx_vendor_req xbox360_vendor_requests = USBD_VENDOR_REQ(
	XBOX360_USB_VENDOR_REQUEST_01
);

static int xbox360_device_init(const struct device *dev)
{
	struct xbox360_device_data *data = dev->data;

	LOG_DBG("Starting initialization.");

	data->dev = dev;

	return 0;
}

static struct net_buf *xbox360_class_control_to_host(struct usbd_class_data *const c_data,
						     const struct usb_setup_packet *const setup)
{
	LOG_DBG("Received a setup request 0x%02X with a length of %u.",
		setup->bRequest, setup->wLength);

	switch (setup->bRequest) {
	case XBOX360_USB_VENDOR_REQUEST_01: {
		struct xbox360_controller_control_surface empty = {0};
		struct net_buf *buf;
		uint16_t answer_length;

		LOG_DBG("Handling the \"get initial state\" request.");

		/* Do not send more data than what the host asked for */
		answer_length = MIN(setup->wLength, sizeof(empty));

		buf = usbd_ep_ctrl_data_in_alloc(usbd_class_get_ctx(c_data),
			answer_length);
		if (buf == NULL) {
			return NULL;
		}
		net_buf_add_mem(buf, &empty, answer_length);
		return buf;
	}

	default:
		LOG_ERR("Unknown request.");
		return NULL;
	}
}

static void xbox360_class_disable(struct usbd_class_data *const c_data)
{
	const struct device *dev = usbd_class_get_private(c_data);
	struct xbox360_device_data *data = dev->data;

	LOG_DBG("The device is disabled.");

	if ((data->device_ops != NULL) && (data->device_ops->iface_ready != NULL)) {
		data->device_ops->iface_ready(dev, false);
	}
}

static void xbox360_class_enable(struct usbd_class_data *const c_data)
{
	const struct device *dev = usbd_class_get_private(c_data);
	struct xbox360_device_data *data = dev->data;

	LOG_DBG("The device is enabled.");

	if ((data->device_ops != NULL) && (data->device_ops->iface_ready != NULL)) {
		data->device_ops->iface_ready(dev, true);
	}
}

static const void *xbox360_class_get_desc(struct usbd_class_data *const c_data,
					  const enum usbd_speed speed)
{
	const struct device *dev = usbd_class_get_private(c_data);
	const struct xbox360_device_config *config = dev->config;

	return config->usb_descriptor_headers;
}

static int xbox360_class_init(struct usbd_class_data *const c_data)
{
	ARG_UNUSED(c_data);

	return 0;
}

static int xbox360_class_request(struct usbd_class_data *const c_data, struct net_buf *buf,
				 int err)
{
	struct usbd_context *uds_ctx = usbd_class_get_ctx(c_data);

	if (err != 0) {
		LOG_DBG("Reported error : %d.", err);
	}

	return usbd_ep_buf_free(uds_ctx, buf);
}

static struct usbd_class_api xbox360_usbd_class_api = {
	.control_to_host = xbox360_class_control_to_host,
	.disable = xbox360_class_disable,
	.enable = xbox360_class_enable,
	.get_desc = xbox360_class_get_desc,
	.init = xbox360_class_init,
	.request = xbox360_class_request
};

int xbox360_controller_device_register(const struct device *dev,
				       const struct xbox360_controller_device_ops *const ops)
{
	struct xbox360_device_data *data = dev->data;

	data->device_ops = ops;

	return 0;
}

int xbox360_controller_submit_report(const struct device *dev,
				     struct xbox360_controller_control_surface *control_surface)
{
	const struct xbox360_device_config *config = dev->config;
	struct net_buf *buffer;
	struct udc_buf_info *buffer_info;
	int ret;

	/* Make sure that the non-data fields are set */
	control_surface->message_type = 0;
	control_surface->length = sizeof(struct xbox360_controller_control_surface);

	buffer = net_buf_alloc_with_data(config->buffers_pool_in, (void *)control_surface,
		sizeof(struct xbox360_controller_control_surface), K_NO_WAIT);
	if (buffer == NULL) {
		LOG_ERR("Failed to allocate a buffer from the pool.");
		return -ENOMEM;
	}
	buffer_info = udc_get_buf_info(buffer);
	buffer_info->ep = usb_descriptor.control_surface_in_endpoint.bEndpointAddress;

	ret = usbd_ep_enqueue(config->usb_class_data, buffer);
	if (ret != 0) {
		net_buf_unref(buffer);
		LOG_ERR("Failed to enqueue the buffer (%d).", ret);
		return -ENOMEM;
	}

	return 0;
}

#define USBD_XBOX360_INSTANCE_DEFINE(inst)							\
	USBD_DEFINE_CLASS(xbox360_##inst, &xbox360_usbd_class_api,				\
		(void *)DEVICE_DT_GET(DT_DRV_INST(inst)), &xbox360_vendor_requests);		\
												\
	NET_BUF_POOL_DEFINE(xbox360_buffers_pool_in_##inst, 3, 0,				\
		sizeof(struct xbox360_controller_control_surface), NULL);			\
												\
	static const struct xbox360_device_config xbox360_device_config_##inst = {		\
		.usb_descriptor_headers = xbox360_usb_descriptor_headers,			\
		.buffers_pool_in = &xbox360_buffers_pool_in_##inst,				\
		.usb_class_data = &xbox360_##inst						\
	};											\
												\
	static struct xbox360_device_data xbox360_device_data_##inst;				\
												\
	DEVICE_DT_INST_DEFINE(inst, xbox360_device_init, NULL, &xbox360_device_data_##inst,	\
		&xbox360_device_config_##inst, POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,	\
		NULL);

DT_INST_FOREACH_STATUS_OKAY(USBD_XBOX360_INSTANCE_DEFINE)
