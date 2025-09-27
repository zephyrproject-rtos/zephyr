/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/byteorder.h>

#include <zephyr/usb/usbd.h>
#include <zephyr/usb/usb_ch9.h>
#include <zephyr/usb/class/usbd_mctp.h>
#include <zephyr/drivers/usb/udc.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(usbd_mctp, CONFIG_USBD_MCTP_LOG_LEVEL);

#define DT_DRV_COMPAT zephyr_mctp_usb

#define MCTP_ENABLED 0

struct usbd_mctp_desc {
	struct usb_association_descriptor iad;
	struct usb_if_descriptor if0;
	struct usb_ep_descriptor if0_fs_out_ep;
	struct usb_ep_descriptor if0_fs_in_ep;
	struct usb_ep_descriptor if0_hs_out_ep;
	struct usb_ep_descriptor if0_hs_in_ep;
	struct usb_desc_header nil_desc;
};

struct usbd_mctp_config {
	struct usbd_class_data *const c_data;
	struct usbd_mctp_desc *const desc;
	const struct usb_desc_header **const fs_desc;
	const struct usb_desc_header **const hs_desc;
};

struct usbd_mctp_data {
	const struct device *dev;
	const struct mctp_ops *ops;
	struct k_sem in_sem;
	struct k_work out_work;
	void *user_data;
	atomic_t state;
};

static void usbd_mctp_out_work(struct k_work *work)
{
	struct usbd_mctp_data *data = CONTAINER_OF(work, struct usbd_mctp_data, out_work);
	const struct usbd_mctp_config *config = data->dev->config;
	struct usbd_class_data *c_data = config->c_data;
	struct net_buf *buf;

	if (!atomic_test_bit(&data->state, MCTP_ENABLED)) {
		return;
	}

	buf = usbd_ep_buf_alloc(c_data, CONFIG_USBD_MCTP_BULK_OUT_EP_ADDR, USBD_MAX_BULK_MPS);
	if (buf == NULL) {
		LOG_ERR("Failed to allocate OUT buffer");
		return;
	}

	if (usbd_ep_enqueue(c_data, buf)) {
		net_buf_unref(buf);
		LOG_ERR("Failed to enqueu OUT buffer");
	}
}

static int usbd_mctp_request(struct usbd_class_data *const c_data, struct net_buf *buf, int err)
{
	struct usbd_context *uds_ctx = usbd_class_get_ctx(c_data);
	const struct device *dev = usbd_class_get_private(c_data);
	struct usbd_mctp_data *data = dev->data;
	const struct mctp_ops *ops = data->ops;

	struct udc_buf_info *bi = udc_get_buf_info(buf);

	if (bi->ep == CONFIG_USBD_MCTP_BULK_OUT_EP_ADDR) {
		if (ops->data_recv_cb != NULL) {
			ops->data_recv_cb(dev, buf->data, buf->len, data->user_data);
		}

		k_work_submit(&data->out_work);
	}

	if (bi->ep == CONFIG_USBD_MCTP_BULK_IN_EP_ADDR) {
		if (ops->tx_complete_cb != NULL) {
			ops->tx_complete_cb(dev, data->user_data);
		} else {
			k_sem_give(&data->in_sem);
		}
	}

	return usbd_ep_buf_free(uds_ctx, buf);
}

static void *usbd_mctp_get_desc(struct usbd_class_data *const c_data, const enum usbd_speed speed)
{
	const struct device *dev = usbd_class_get_private(c_data);
	const struct usbd_mctp_config *config = dev->config;

#if USBD_SUPPORTS_HIGH_SPEED
	if (speed == USBD_SPEED_HS) {
		return config->hs_desc;
	}
#endif

	return config->fs_desc;
}

static void usbd_mctp_enable(struct usbd_class_data *const c_data)
{
	const struct device *dev = usbd_class_get_private(c_data);
	struct usbd_mctp_data *data = dev->data;

	if (!atomic_test_and_set_bit(&data->state, MCTP_ENABLED)) {
		k_work_submit(&data->out_work);
	}

	LOG_DBG("Enabled %s", c_data->name);
}

static void usbd_mctp_disable(struct usbd_class_data *const c_data)
{
	const struct device *dev = usbd_class_get_private(c_data);
	struct usbd_mctp_data *data = dev->data;

	atomic_clear_bit(&data->state, MCTP_ENABLED);

	LOG_DBG("Disabled %s", c_data->name);
}

static int usbd_mctp_init(struct usbd_class_data *const c_data)
{
	const struct device *dev = usbd_class_get_private(c_data);
	struct usbd_mctp_data *data = dev->data;

	k_sem_init(&data->in_sem, 1, 1);
	k_work_init(&data->out_work, usbd_mctp_out_work);

	LOG_DBG("MCTP device %s initialized", dev->name);

	return 0;
}

struct usbd_class_api usbd_mctp_api = {.request = usbd_mctp_request,
				       .enable = usbd_mctp_enable,
				       .disable = usbd_mctp_disable,
				       .init = usbd_mctp_init,
				       .get_desc = usbd_mctp_get_desc};

void usbd_mctp_set_ops(const struct device *dev, const struct mctp_ops *ops, void *user_data)
{
	struct usbd_mctp_data *data = dev->data;

	__ASSERT(ops->data_recv_cb, "data_recv_cb is mandatory");

	data->ops = ops;
	data->user_data = user_data;
}

int usbd_mctp_send(const struct device *dev, void *data, uint16_t size)
{
	struct usbd_mctp_data *ddata = dev->data;
	const struct usbd_mctp_config *config = dev->config;
	struct usbd_class_data *c_data = config->c_data;
	struct net_buf *buf = NULL;
	int err;

	if (!atomic_test_bit(&ddata->state, MCTP_ENABLED)) {
		return -EPERM;
	}

	buf = usbd_ep_buf_alloc(c_data, CONFIG_USBD_MCTP_BULK_IN_EP_ADDR, size);
	if (buf == NULL) {
		LOG_ERR("Failed to allocate IN buffer");
		return -ENOMEM;
	}

	net_buf_add_mem(buf, data, size);

	err = usbd_ep_enqueue(c_data, buf);
	if (err) {
		LOG_ERR("Failed to enqueue IN buffer");
		net_buf_unref(buf);
		return err;
	}

	/* If there is no TX complete defined, use the semaphore. */
	if (ddata->ops->tx_complete_cb == NULL) {
		k_sem_take(&ddata->in_sem, K_FOREVER);
	}

	return 0;
}

#define USBD_MCTP_DEFINE_DESCRIPTORS(n)                                                            \
	static struct usbd_mctp_desc usbd_mctp_desc_##n = {                                        \
		.iad =                                                                             \
			{                                                                          \
				.bLength = sizeof(struct usb_association_descriptor),              \
				.bDescriptorType = USB_DESC_INTERFACE_ASSOC,                       \
				.bFirstInterface = 0,                                              \
				.bInterfaceCount = 1,                                              \
				.bFunctionClass = USB_BCC_MCTP,                                    \
				.bFunctionSubClass = CONFIG_USBD_MCTP_INTERFACE_SUBCLASS,          \
				.bFunctionProtocol = CONFIG_USBD_MCTP_INTERFACE_PROTOCOL,          \
				.iFunction = 0,                                                    \
			},                                                                         \
		.if0 = {.bLength = sizeof(struct usb_if_descriptor),                               \
			.bDescriptorType = USB_DESC_INTERFACE,                                     \
			.bInterfaceNumber = 0,                                                     \
			.bAlternateSetting = 0,                                                    \
			.bNumEndpoints = 2,                                                        \
			.bInterfaceClass = USB_BCC_MCTP,                                           \
			.bInterfaceSubClass = CONFIG_USBD_MCTP_INTERFACE_SUBCLASS,                 \
			.bInterfaceProtocol = CONFIG_USBD_MCTP_INTERFACE_PROTOCOL,                 \
			.iInterface = 0},                                                          \
		.if0_fs_out_ep = {.bLength = sizeof(struct usb_ep_descriptor),                     \
				  .bDescriptorType = USB_DESC_ENDPOINT,                            \
				  .bEndpointAddress = CONFIG_USBD_MCTP_BULK_OUT_EP_ADDR,           \
				  .bmAttributes = USB_EP_TYPE_BULK,                                \
				  .wMaxPacketSize = sys_cpu_to_le16(64),                           \
				  .bInterval = 1},                                                 \
		.if0_fs_in_ep = {.bLength = sizeof(struct usb_ep_descriptor),                      \
				 .bDescriptorType = USB_DESC_ENDPOINT,                             \
				 .bEndpointAddress = CONFIG_USBD_MCTP_BULK_IN_EP_ADDR,             \
				 .bmAttributes = USB_EP_TYPE_BULK,                                 \
				 .wMaxPacketSize = sys_cpu_to_le16(64),                            \
				 .bInterval = 1},                                                  \
		.if0_hs_out_ep = {.bLength = sizeof(struct usb_ep_descriptor),                     \
				  .bDescriptorType = USB_DESC_ENDPOINT,                            \
				  .bEndpointAddress = CONFIG_USBD_MCTP_BULK_OUT_EP_ADDR,           \
				  .bmAttributes = USB_EP_TYPE_BULK,                                \
				  .wMaxPacketSize = sys_cpu_to_le16(512),                          \
				  .bInterval = 1},                                                 \
		.if0_hs_in_ep = {.bLength = sizeof(struct usb_ep_descriptor),                      \
				 .bDescriptorType = USB_DESC_ENDPOINT,                             \
				 .bEndpointAddress = CONFIG_USBD_MCTP_BULK_IN_EP_ADDR,             \
				 .bmAttributes = USB_EP_TYPE_BULK,                                 \
				 .wMaxPacketSize = sys_cpu_to_le16(512),                           \
				 .bInterval = 1},                                                  \
		.nil_desc = {.bLength = 0, .bDescriptorType = 0}};                                 \
                                                                                                   \
	const static struct usb_desc_header *usbd_mctp_fs_desc_##n[] = {                           \
		(struct usb_desc_header *)&usbd_mctp_desc_##n.iad,                                 \
		(struct usb_desc_header *)&usbd_mctp_desc_##n.if0,                                 \
		(struct usb_desc_header *)&usbd_mctp_desc_##n.if0_fs_in_ep,                        \
		(struct usb_desc_header *)&usbd_mctp_desc_##n.if0_fs_out_ep,                       \
		(struct usb_desc_header *)&usbd_mctp_desc_##n.nil_desc};                           \
                                                                                                   \
	const static struct usb_desc_header *usbd_mctp_hs_desc_##n[] = {                           \
		(struct usb_desc_header *)&usbd_mctp_desc_##n.iad,                                 \
		(struct usb_desc_header *)&usbd_mctp_desc_##n.if0,                                 \
		(struct usb_desc_header *)&usbd_mctp_desc_##n.if0_hs_in_ep,                        \
		(struct usb_desc_header *)&usbd_mctp_desc_##n.if0_hs_out_ep,                       \
		(struct usb_desc_header *)&usbd_mctp_desc_##n.nil_desc}

#define USBD_MCTP_DT_DEVICE_DEFINE(n)                                                              \
	BUILD_ASSERT(DT_INST_ON_BUS(n, usb),                                                       \
		     "node " DT_NODE_PATH(                                                         \
			     DT_DRV_INST(n)) " is not assigned to a USB device controller");       \
                                                                                                   \
	USBD_MCTP_DEFINE_DESCRIPTORS(n);                                                           \
                                                                                                   \
	USBD_DEFINE_CLASS(mctp_##n, &usbd_mctp_api, (void *)DEVICE_DT_GET(DT_DRV_INST(n)), NULL);  \
                                                                                                   \
	static const struct usbd_mctp_config mctp_config_##n = {.c_data = &mctp_##n,               \
								.desc = &usbd_mctp_desc_##n,       \
								.fs_desc = usbd_mctp_fs_desc_##n,  \
								.hs_desc = usbd_mctp_hs_desc_##n}; \
                                                                                                   \
	static struct usbd_mctp_data mctp_data_##n = {                                             \
		.dev = DEVICE_DT_GET(DT_DRV_INST(n)), .ops = NULL, .user_data = NULL, .state = 0}; \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(n, NULL, NULL, &mctp_data_##n, &mctp_config_##n, POST_KERNEL,        \
			      CONFIG_KERNEL_INIT_PRIORITY_DEVICE, NULL);

DT_INST_FOREACH_STATUS_OKAY(USBD_MCTP_DT_DEVICE_DEFINE);
