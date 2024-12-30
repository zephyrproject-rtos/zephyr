/*
 * Copyright (c) 2022, 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/usb/usbd.h>
#include <zephyr/shell/shell.h>
#include <zephyr/drivers/usb/udc.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(usb_loopback, CONFIG_USBD_LOOPBACK_LOG_LEVEL);

/*
 * This function does not define its own pool and requires a large enough UDC
 * pool. To use it with the Linux kernel tool testusb, we need about 4096 bytes
 * in the current configuration.
 */

/*
 * NOTE: this class is experimental and is in development.
 * Primary purpose currently is testing of the class initialization and
 * interface and endpoint configuration.
 */

/* Internal buffer for intermediate test data */
static uint8_t lb_buf[1024];

#define LB_VENDOR_REQ_OUT		0x5b
#define LB_VENDOR_REQ_IN		0x5c

#define LB_ISO_EP_MPS			256
#define LB_ISO_EP_INTERVAL		1

#define LB_FUNCTION_ENABLED		0
#define LB_FUNCTION_BULK_MANUAL		1
#define LB_FUNCTION_IN_ENGAGED		2
#define LB_FUNCTION_OUT_ENGAGED		3

/* Make supported vendor request visible for the device stack */
static const struct usbd_cctx_vendor_req lb_vregs =
	USBD_VENDOR_REQ(LB_VENDOR_REQ_OUT, LB_VENDOR_REQ_IN);

struct loopback_desc {
	struct usb_association_descriptor iad;
	struct usb_if_descriptor if0;
	struct usb_ep_descriptor if0_out_ep;
	struct usb_ep_descriptor if0_in_ep;
	struct usb_ep_descriptor if0_hs_out_ep;
	struct usb_ep_descriptor if0_hs_in_ep;
	struct usb_if_descriptor if1;
	struct usb_ep_descriptor if1_int_out_ep;
	struct usb_ep_descriptor if1_int_in_ep;
	struct usb_if_descriptor if2_0;
	struct usb_ep_descriptor if2_0_iso_in_ep;
	struct usb_ep_descriptor if2_0_iso_out_ep;
	struct usb_if_descriptor if2_1;
	struct usb_ep_descriptor if2_1_iso_in_ep;
	struct usb_ep_descriptor if2_1_iso_out_ep;
	struct usb_desc_header nil_desc;
};

struct lb_data {
	struct loopback_desc *const desc;
	const struct usb_desc_header **const fs_desc;
	const struct usb_desc_header **const hs_desc;
	atomic_t state;
};

static uint8_t lb_get_bulk_out(struct usbd_class_data *const c_data)
{
	struct lb_data *data = usbd_class_get_private(c_data);
	struct usbd_context *uds_ctx = usbd_class_get_ctx(c_data);
	struct loopback_desc *desc = data->desc;

	if (usbd_bus_speed(uds_ctx) == USBD_SPEED_HS) {
		return desc->if0_hs_out_ep.bEndpointAddress;
	}

	return desc->if0_out_ep.bEndpointAddress;
}

static uint8_t lb_get_bulk_in(struct usbd_class_data *const c_data)
{
	struct lb_data *data = usbd_class_get_private(c_data);
	struct usbd_context *uds_ctx = usbd_class_get_ctx(c_data);
	struct loopback_desc *desc = data->desc;

	if (usbd_bus_speed(uds_ctx) == USBD_SPEED_HS) {
		return desc->if0_hs_in_ep.bEndpointAddress;
	}

	return desc->if0_in_ep.bEndpointAddress;
}

static int lb_submit_bulk_out(struct usbd_class_data *const c_data)
{
	struct lb_data *data = usbd_class_get_private(c_data);
	struct net_buf *buf;
	int err;

	if (!atomic_test_bit(&data->state, LB_FUNCTION_ENABLED)) {
		return -EPERM;
	}

	if (atomic_test_and_set_bit(&data->state, LB_FUNCTION_OUT_ENGAGED)) {
		return -EBUSY;
	}

	buf = usbd_ep_buf_alloc(c_data, lb_get_bulk_out(c_data), sizeof(lb_buf));
	if (buf == NULL) {
		LOG_ERR("Failed to allocate buffer");
		return -ENOMEM;
	}

	err = usbd_ep_enqueue(c_data, buf);
	if (err) {
		LOG_ERR("Failed to enqueue buffer");
		net_buf_unref(buf);
	}

	return err;
}

static int lb_submit_bulk_in(struct usbd_class_data *const c_data)
{
	struct lb_data *data = usbd_class_get_private(c_data);
	struct net_buf *buf;
	int err;

	if (!atomic_test_bit(&data->state, LB_FUNCTION_ENABLED)) {
		return -EPERM;
	}

	if (atomic_test_and_set_bit(&data->state, LB_FUNCTION_IN_ENGAGED)) {
		return -EBUSY;
	}

	buf = usbd_ep_buf_alloc(c_data, lb_get_bulk_in(c_data), sizeof(lb_buf));
	if (buf == NULL) {
		LOG_ERR("Failed to allocate buffer");
		return -ENOMEM;
	}

	net_buf_add_mem(buf, lb_buf, MIN(sizeof(lb_buf), net_buf_tailroom(buf)));
	err = usbd_ep_enqueue(c_data, buf);
	if (err) {
		LOG_ERR("Failed to enqueue buffer");
		net_buf_unref(buf);
	}

	return err;
}

static int lb_request_handler(struct usbd_class_data *const c_data,
			      struct net_buf *const buf, const int err)
{
	struct udc_buf_info *bi = (struct udc_buf_info *)net_buf_user_data(buf);
	struct lb_data *data = usbd_class_get_private(c_data);

	LOG_DBG("Transfer finished %s -> ep 0x%02x, len %u, err %d",
		c_data->name, bi->ep, buf->len, err);

	if (bi->ep == lb_get_bulk_out(c_data)) {
		atomic_clear_bit(&data->state, LB_FUNCTION_OUT_ENGAGED);
	}

	if (bi->ep == lb_get_bulk_in(c_data)) {
		atomic_clear_bit(&data->state, LB_FUNCTION_IN_ENGAGED);
	}

	if (err) {
		if (err == -ECONNABORTED) {
			LOG_INF("request ep 0x%02x, len %u cancelled",
				bi->ep, buf->len);
		} else {
			LOG_ERR("request ep 0x%02x, len %u failed",
				bi->ep, buf->len);
		}

		net_buf_unref(buf);

		return err;
	}

	if (bi->ep == lb_get_bulk_out(c_data)) {
		memcpy(lb_buf, buf->data, MIN(sizeof(lb_buf), buf->len));
		net_buf_unref(buf);
		if (!atomic_test_bit(&data->state, LB_FUNCTION_BULK_MANUAL)) {
			lb_submit_bulk_out(c_data);
		}
	}

	if (bi->ep == lb_get_bulk_in(c_data)) {
		bi->ep = lb_get_bulk_out(c_data);
		net_buf_unref(buf);
		if (!atomic_test_bit(&data->state, LB_FUNCTION_BULK_MANUAL)) {
			lb_submit_bulk_in(c_data);
		}
	}

	return 0;
}

static void lb_update(struct usbd_class_data *c_data,
		      uint8_t iface, uint8_t alternate)
{
	LOG_DBG("Instance %p, interface %u alternate %u changed",
		c_data, iface, alternate);
}

static int lb_control_to_host(struct usbd_class_data *c_data,
			      const struct usb_setup_packet *const setup,
			      struct net_buf *const buf)
{
	if (setup->RequestType.recipient != USB_REQTYPE_RECIPIENT_DEVICE) {
		errno = -ENOTSUP;
		return 0;
	}

	if (setup->bRequest == LB_VENDOR_REQ_IN) {
		net_buf_add_mem(buf, lb_buf,
				MIN(sizeof(lb_buf), setup->wLength));

		LOG_WRN("Device-to-Host, wLength %u | %zu", setup->wLength,
			MIN(sizeof(lb_buf), setup->wLength));

		return 0;
	}

	LOG_ERR("Class request 0x%x not supported", setup->bRequest);
	errno = -ENOTSUP;

	return 0;
}

static int lb_control_to_dev(struct usbd_class_data *c_data,
			     const struct usb_setup_packet *const setup,
			     const struct net_buf *const buf)
{
	if (setup->RequestType.recipient != USB_REQTYPE_RECIPIENT_DEVICE) {
		errno = -ENOTSUP;
		return 0;
	}

	if (setup->bRequest == LB_VENDOR_REQ_OUT) {
		LOG_WRN("Host-to-Device, wLength %u | %zu", setup->wLength,
			MIN(sizeof(lb_buf), buf->len));
		memcpy(lb_buf, buf->data, MIN(sizeof(lb_buf), buf->len));
		return 0;
	}

	LOG_ERR("Class request 0x%x not supported", setup->bRequest);
	errno = -ENOTSUP;

	return 0;
}

static void *lb_get_desc(struct usbd_class_data *const c_data,
			 const enum usbd_speed speed)
{
	struct lb_data *data = usbd_class_get_private(c_data);

	if (speed == USBD_SPEED_HS) {
		return data->hs_desc;
	}

	return data->fs_desc;
}

static void lb_enable(struct usbd_class_data *const c_data)
{
	struct lb_data *data = usbd_class_get_private(c_data);

	LOG_INF("Enable %s", c_data->name);
	if (!atomic_test_and_set_bit(&data->state, LB_FUNCTION_ENABLED) &&
	    !atomic_test_bit(&data->state, LB_FUNCTION_BULK_MANUAL)) {
		lb_submit_bulk_out(c_data);
		lb_submit_bulk_in(c_data);
	}
}

static void lb_disable(struct usbd_class_data *const c_data)
{
	struct lb_data *data = usbd_class_get_private(c_data);

	atomic_clear_bit(&data->state, LB_FUNCTION_ENABLED);
	LOG_INF("Disable %s", c_data->name);
}

static int lb_init(struct usbd_class_data *c_data)
{
	LOG_DBG("Init class instance %p", c_data);

	return 0;
}

struct usbd_class_api lb_api = {
	.update = lb_update,
	.control_to_host = lb_control_to_host,
	.control_to_dev = lb_control_to_dev,
	.request = lb_request_handler,
	.get_desc = lb_get_desc,
	.enable = lb_enable,
	.disable = lb_disable,
	.init = lb_init,
};

#define DEFINE_LOOPBACK_DESCRIPTOR(x, _)					\
static struct loopback_desc lb_desc_##x = {					\
	.iad = {								\
		.bLength = sizeof(struct usb_association_descriptor),		\
		.bDescriptorType = USB_DESC_INTERFACE_ASSOC,			\
		.bFirstInterface = 0,						\
		.bInterfaceCount = 3,						\
		.bFunctionClass = USB_BCC_VENDOR,				\
		.bFunctionSubClass = 0,						\
		.bFunctionProtocol = 0,						\
		.iFunction = 0,							\
	},									\
										\
	/* Interface descriptor 0 */						\
	.if0 = {								\
		.bLength = sizeof(struct usb_if_descriptor),			\
		.bDescriptorType = USB_DESC_INTERFACE,				\
		.bInterfaceNumber = 0,						\
		.bAlternateSetting = 0,						\
		.bNumEndpoints = 2,						\
		.bInterfaceClass = USB_BCC_VENDOR,				\
		.bInterfaceSubClass = 0,					\
		.bInterfaceProtocol = 0,					\
		.iInterface = 0,						\
	},									\
										\
	/* Data Endpoint OUT */							\
	.if0_out_ep = {								\
		.bLength = sizeof(struct usb_ep_descriptor),			\
		.bDescriptorType = USB_DESC_ENDPOINT,				\
		.bEndpointAddress = 0x01,					\
		.bmAttributes = USB_EP_TYPE_BULK,				\
		.wMaxPacketSize = sys_cpu_to_le16(64U),				\
		.bInterval = 0x00,						\
	},									\
										\
	/* Data Endpoint IN */							\
	.if0_in_ep = {								\
		.bLength = sizeof(struct usb_ep_descriptor),			\
		.bDescriptorType = USB_DESC_ENDPOINT,				\
		.bEndpointAddress = 0x81,					\
		.bmAttributes = USB_EP_TYPE_BULK,				\
		.wMaxPacketSize = sys_cpu_to_le16(64U),				\
		.bInterval = 0x00,						\
	},									\
										\
	/* Data Endpoint OUT */							\
	.if0_hs_out_ep = {							\
		.bLength = sizeof(struct usb_ep_descriptor),			\
		.bDescriptorType = USB_DESC_ENDPOINT,				\
		.bEndpointAddress = 0x01,					\
		.bmAttributes = USB_EP_TYPE_BULK,				\
		.wMaxPacketSize = sys_cpu_to_le16(512),				\
		.bInterval = 0x00,						\
	},									\
										\
	/* Data Endpoint IN */							\
	.if0_hs_in_ep = {							\
		.bLength = sizeof(struct usb_ep_descriptor),			\
		.bDescriptorType = USB_DESC_ENDPOINT,				\
		.bEndpointAddress = 0x81,					\
		.bmAttributes = USB_EP_TYPE_BULK,				\
		.wMaxPacketSize = sys_cpu_to_le16(512),				\
		.bInterval = 0x00,						\
	},									\
										\
	/* Interface descriptor 1 */						\
	.if1 = {								\
		.bLength = sizeof(struct usb_if_descriptor),			\
		.bDescriptorType = USB_DESC_INTERFACE,				\
		.bInterfaceNumber = 1,						\
		.bAlternateSetting = 0,						\
		.bNumEndpoints = 2,						\
		.bInterfaceClass = USB_BCC_VENDOR,				\
		.bInterfaceSubClass = 0,					\
		.bInterfaceProtocol = 0,					\
		.iInterface = 0,						\
	},									\
										\
	/* Interface Interrupt Endpoint OUT */					\
	.if1_int_out_ep = {							\
		.bLength = sizeof(struct usb_ep_descriptor),			\
		.bDescriptorType = USB_DESC_ENDPOINT,				\
		.bEndpointAddress = 0x02,					\
		.bmAttributes = USB_EP_TYPE_INTERRUPT,				\
		.wMaxPacketSize = sys_cpu_to_le16(64),				\
		.bInterval = 0x01,						\
	},									\
										\
	/* Interrupt Interrupt Endpoint IN */					\
	.if1_int_in_ep = {							\
		.bLength = sizeof(struct usb_ep_descriptor),			\
		.bDescriptorType = USB_DESC_ENDPOINT,				\
		.bEndpointAddress = 0x82,					\
		.bmAttributes = USB_EP_TYPE_INTERRUPT,				\
		.wMaxPacketSize = sys_cpu_to_le16(64),				\
		.bInterval = 0x01,						\
	},									\
										\
	.if2_0 = {								\
		.bLength = sizeof(struct usb_if_descriptor),			\
		.bDescriptorType = USB_DESC_INTERFACE,				\
		.bInterfaceNumber = 2,						\
		.bAlternateSetting = 0,						\
		.bNumEndpoints = 2,						\
		.bInterfaceClass = USB_BCC_VENDOR,				\
		.bInterfaceSubClass = 0,					\
		.bInterfaceProtocol = 0,					\
		.iInterface = 0,						\
	},									\
										\
	.if2_0_iso_in_ep = {							\
		.bLength = sizeof(struct usb_ep_descriptor),			\
		.bDescriptorType = USB_DESC_ENDPOINT,				\
		.bEndpointAddress = 0x83,					\
		.bmAttributes = USB_EP_TYPE_ISO,				\
		.wMaxPacketSize = sys_cpu_to_le16(0),				\
		.bInterval = LB_ISO_EP_INTERVAL,				\
	},									\
										\
	.if2_0_iso_out_ep = {							\
		.bLength = sizeof(struct usb_ep_descriptor),			\
		.bDescriptorType = USB_DESC_ENDPOINT,				\
		.bEndpointAddress = 0x03,					\
		.bmAttributes = USB_EP_TYPE_ISO,				\
		.wMaxPacketSize = sys_cpu_to_le16(0),				\
		.bInterval = LB_ISO_EP_INTERVAL,				\
	},									\
										\
	.if2_1 = {								\
		.bLength = sizeof(struct usb_if_descriptor),			\
		.bDescriptorType = USB_DESC_INTERFACE,				\
		.bInterfaceNumber = 2,						\
		.bAlternateSetting = 1,						\
		.bNumEndpoints = 2,						\
		.bInterfaceClass = USB_BCC_VENDOR,				\
		.bInterfaceSubClass = 0,					\
		.bInterfaceProtocol = 0,					\
		.iInterface = 0,						\
	},									\
										\
	.if2_1_iso_in_ep = {							\
		.bLength = sizeof(struct usb_ep_descriptor),			\
		.bDescriptorType = USB_DESC_ENDPOINT,				\
		.bEndpointAddress = 0x83,					\
		.bmAttributes = USB_EP_TYPE_ISO,				\
		.wMaxPacketSize = sys_cpu_to_le16(LB_ISO_EP_MPS),		\
		.bInterval = LB_ISO_EP_INTERVAL,				\
	},									\
										\
	.if2_1_iso_out_ep = {							\
		.bLength = sizeof(struct usb_ep_descriptor),			\
		.bDescriptorType = USB_DESC_ENDPOINT,				\
		.bEndpointAddress = 0x03,					\
		.bmAttributes = USB_EP_TYPE_ISO,				\
		.wMaxPacketSize = sys_cpu_to_le16(LB_ISO_EP_MPS),		\
		.bInterval = LB_ISO_EP_INTERVAL,				\
	},									\
										\
	/* Termination descriptor */						\
	.nil_desc = {								\
		.bLength = 0,							\
		.bDescriptorType = 0,						\
	},									\
};										\
										\
const static struct usb_desc_header *lb_fs_desc_##x[] = {			\
	(struct usb_desc_header *) &lb_desc_##x.iad,				\
	(struct usb_desc_header *) &lb_desc_##x.if0,				\
	(struct usb_desc_header *) &lb_desc_##x.if0_in_ep,			\
	(struct usb_desc_header *) &lb_desc_##x.if0_out_ep,			\
	(struct usb_desc_header *) &lb_desc_##x.if1,				\
	(struct usb_desc_header *) &lb_desc_##x.if1_int_in_ep,			\
	(struct usb_desc_header *) &lb_desc_##x.if1_int_out_ep,			\
	(struct usb_desc_header *) &lb_desc_##x.if2_0,				\
	(struct usb_desc_header *) &lb_desc_##x.if2_0_iso_in_ep,		\
	(struct usb_desc_header *) &lb_desc_##x.if2_0_iso_out_ep,		\
	(struct usb_desc_header *) &lb_desc_##x.if2_1,				\
	(struct usb_desc_header *) &lb_desc_##x.if2_1_iso_in_ep,		\
	(struct usb_desc_header *) &lb_desc_##x.if2_1_iso_out_ep,		\
	(struct usb_desc_header *) &lb_desc_##x.nil_desc,			\
};										\
										\
const static struct usb_desc_header *lb_hs_desc_##x[] = {			\
	(struct usb_desc_header *) &lb_desc_##x.iad,				\
	(struct usb_desc_header *) &lb_desc_##x.if0,				\
	(struct usb_desc_header *) &lb_desc_##x.if0_hs_in_ep,			\
	(struct usb_desc_header *) &lb_desc_##x.if0_hs_out_ep,			\
	(struct usb_desc_header *) &lb_desc_##x.if1,				\
	(struct usb_desc_header *) &lb_desc_##x.if1_int_in_ep,			\
	(struct usb_desc_header *) &lb_desc_##x.if1_int_out_ep,			\
	(struct usb_desc_header *) &lb_desc_##x.if2_0,				\
	(struct usb_desc_header *) &lb_desc_##x.if2_0_iso_in_ep,		\
	(struct usb_desc_header *) &lb_desc_##x.if2_0_iso_out_ep,		\
	(struct usb_desc_header *) &lb_desc_##x.if2_1,				\
	(struct usb_desc_header *) &lb_desc_##x.if2_1_iso_in_ep,		\
	(struct usb_desc_header *) &lb_desc_##x.if2_1_iso_out_ep,		\
	(struct usb_desc_header *) &lb_desc_##x.nil_desc,			\
};


#define DEFINE_LOOPBACK_CLASS_DATA(x, _)					\
	static struct lb_data lb_data_##x = {					\
		.desc = &lb_desc_##x,						\
		.fs_desc = lb_fs_desc_##x,					\
		.hs_desc = lb_hs_desc_##x,					\
	};									\
										\
	USBD_DEFINE_CLASS(loopback_##x, &lb_api, &lb_data_##x, &lb_vregs);

LISTIFY(CONFIG_USBD_LOOPBACK_INSTANCES_COUNT, DEFINE_LOOPBACK_DESCRIPTOR, ())
LISTIFY(CONFIG_USBD_LOOPBACK_INSTANCES_COUNT, DEFINE_LOOPBACK_CLASS_DATA, ())

#if CONFIG_USBD_SHELL

/*
 * Device and Host Troubleshooting Shell Commands
 *
 * When set to manual mode, the function does not automatically submit new
 * transfers. The user can manually enqueue or not enqueue new transfers, so
 * the NAK behavior can also be tested.
 *
 * Only bulk endpoints are supported at this time.
 */

static void set_manual(struct usbd_class_data *const c_data, const bool on)
{
	struct lb_data *data = usbd_class_get_private(c_data);

	if (on) {
		atomic_set_bit(&data->state, LB_FUNCTION_BULK_MANUAL);
	} else {
		atomic_clear_bit(&data->state, LB_FUNCTION_BULK_MANUAL);
	}
}

static struct usbd_class_node *lb_get_node(const struct shell *const sh,
					   const char *const name)
{
	STRUCT_SECTION_FOREACH_ALTERNATE(usbd_class_fs, usbd_class_node, c_nd) {
		if (strcmp(c_nd->c_data->name, name) == 0) {
			return c_nd;
		}
	}

	shell_error(sh, "Function %s could not be found", name);

	return NULL;
}

static int cmd_manual_on(const struct shell *sh, size_t argc, char **argv)
{
	struct usbd_class_node *c_nd;

	c_nd = lb_get_node(sh, argv[1]);
	if (c_nd == NULL) {
		return ENODEV;
	}

	shell_print(sh, "%s bulk transfers can be submitted from the shell", argv[1]);
	set_manual(c_nd->c_data, true);

	return 0;
}

static int cmd_manual_off(const struct shell *sh, size_t argc, char **argv)
{
	struct usbd_class_node *c_nd;

	c_nd = lb_get_node(sh, argv[1]);
	if (c_nd == NULL) {
		return ENODEV;
	}

	shell_print(sh, "%s bulk endpoints are automatically engaged", argv[1]);
	set_manual(c_nd->c_data, false);

	return 0;
}

static int cmd_enqueue_out(const struct shell *sh, size_t argc, char **argv)
{
	struct usbd_class_node *c_nd;
	int err;

	c_nd = lb_get_node(sh, argv[1]);
	if (c_nd == NULL) {
		return ENODEV;
	}

	err = lb_submit_bulk_out(c_nd->c_data);
	if (err == -EPERM) {
		shell_error(sh, "%s is not enabled", argv[1]);
	} else if (err == -EBUSY) {
		shell_error(sh, "%s bulk OUT endpoint is busy", argv[1]);
	} else if (err == -ENOMEM) {
		shell_error(sh, "%s failed to allocate transfer", argv[1]);
	} else if (err) {
		shell_error(sh, "%s failed to enqueue transfer", argv[1]);
	} else {
		shell_print(sh, "%s, new transfer enqueued", argv[1]);
	}

	return err;
}

static int cmd_enqueue_in(const struct shell *sh, size_t argc, char **argv)
{
	struct usbd_class_node *c_nd;
	int err;

	c_nd = lb_get_node(sh, argv[1]);
	if (c_nd == NULL) {
		return ENODEV;
	}

	err = lb_submit_bulk_in(c_nd->c_data);
	if (err == -EPERM) {
		shell_error(sh, "%s is not enabled", argv[1]);
	} else if (err == -EBUSY) {
		shell_error(sh, "%s bulk IN endpoint is busy", argv[1]);
	} else if (err == -ENOMEM) {
		shell_error(sh, "%s failed to allocate transfer", argv[1]);
	} else if (err) {
		shell_error(sh, "%s failed to enqueue transfer", argv[1]);
	} else {
		shell_print(sh, "%s, new transfer enqueued", argv[1]);
	}

	return err;
}

static void lb_node_name_lookup(size_t idx, struct shell_static_entry *entry)
{
	size_t match_idx = 0;

	entry->syntax = NULL;
	entry->handler = NULL;
	entry->help = NULL;
	entry->subcmd = NULL;

	STRUCT_SECTION_FOREACH_ALTERNATE(usbd_class_fs, usbd_class_node, c_nd) {
		if (c_nd->c_data->name != NULL && strlen(c_nd->c_data->name) != 0) {
			if (match_idx == idx) {
				entry->syntax = c_nd->c_data->name;
				break;
			}

			++match_idx;
		}
	}
}

SHELL_DYNAMIC_CMD_CREATE(dsub_node_name, lb_node_name_lookup);

SHELL_STATIC_SUBCMD_SET_CREATE(sub_cmd_manual,
	SHELL_CMD_ARG(off, &dsub_node_name, "<function name>", cmd_manual_off, 2, 0),
	SHELL_CMD_ARG(on, &dsub_node_name, "<function name>", cmd_manual_on, 2, 0),
	SHELL_SUBCMD_SET_END
);

SHELL_STATIC_SUBCMD_SET_CREATE(sub_cmd_enqueue,
	SHELL_CMD_ARG(out, &dsub_node_name, "<function name>", cmd_enqueue_out, 2, 0),
	SHELL_CMD_ARG(in, &dsub_node_name, "<function name>", cmd_enqueue_in, 2, 0),
	SHELL_SUBCMD_SET_END
);

SHELL_STATIC_SUBCMD_SET_CREATE(lb_bulk_cmds,
	SHELL_CMD_ARG(manual, &sub_cmd_manual, "off  on", NULL, 2, 0),
	SHELL_CMD_ARG(enqueue, &sub_cmd_enqueue, "out  in", NULL, 2, 0),
	SHELL_SUBCMD_SET_END
);

SHELL_STATIC_SUBCMD_SET_CREATE(sub_lb_cmds,
	SHELL_CMD_ARG(bulk, &lb_bulk_cmds, "bulk endpoint commands", NULL, 2, 0),
	SHELL_SUBCMD_SET_END
);

SHELL_CMD_REGISTER(lb, &sub_lb_cmds, "USB device loopback function commands", NULL);

#endif
