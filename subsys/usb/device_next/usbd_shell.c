/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <zephyr/sys/util.h>
#include <zephyr/shell/shell.h>
#include <zephyr/usb/usbd.h>
#include <zephyr/drivers/usb/udc.h>
#include <zephyr/sys/byteorder.h>

const struct shell *ctx_shell;

/*
 * We define foobaz USB device class which is to be used for the
 * specific shell commands like register and submit.
 */

#define FOOBAZ_VREQ_OUT		0x5b
#define FOOBAZ_VREQ_IN		0x5c

static uint8_t foobaz_buf[512];

/* Make supported vendor request visible for the device stack */
static const struct usbd_cctx_vendor_req foobaz_vregs =
	USBD_VENDOR_REQ(FOOBAZ_VREQ_OUT, FOOBAZ_VREQ_IN);

struct foobaz_iface_desc {
	struct usb_if_descriptor if0;
	struct usb_if_descriptor if1;
	struct usb_ep_descriptor if1_out_ep;
	struct usb_ep_descriptor if1_in_ep;
	struct usb_desc_header term_desc;
} __packed;

static struct foobaz_iface_desc foobaz_desc = {
	.if0 = {
		.bLength = sizeof(struct usb_if_descriptor),
		.bDescriptorType = USB_DESC_INTERFACE,
		.bInterfaceNumber = 0,
		.bAlternateSetting = 0,
		.bNumEndpoints = 0,
		.bInterfaceClass = USB_BCC_VENDOR,
		.bInterfaceSubClass = 0,
		.bInterfaceProtocol = 0,
		.iInterface = 0,
	},

	.if1 = {
		.bLength = sizeof(struct usb_if_descriptor),
		.bDescriptorType = USB_DESC_INTERFACE,
		.bInterfaceNumber = 0,
		.bAlternateSetting = 1,
		.bNumEndpoints = 2,
		.bInterfaceClass = USB_BCC_VENDOR,
		.bInterfaceSubClass = 0,
		.bInterfaceProtocol = 0,
		.iInterface = 0,
	},

	.if1_out_ep = {
		.bLength = sizeof(struct usb_ep_descriptor),
		.bDescriptorType = USB_DESC_ENDPOINT,
		.bEndpointAddress = 0x01,
		.bmAttributes = USB_EP_TYPE_BULK,
		.wMaxPacketSize = 0,
		.bInterval = 0x00,
	},

	.if1_in_ep = {
		.bLength = sizeof(struct usb_ep_descriptor),
		.bDescriptorType = USB_DESC_ENDPOINT,
		.bEndpointAddress = 0x81,
		.bmAttributes = USB_EP_TYPE_BULK,
		.wMaxPacketSize = 0,
		.bInterval = 0x00,
	},

	/* Termination descriptor */
	.term_desc = {
		.bLength = 0,
		.bDescriptorType = 0,
	},
};

static size_t foobaz_get_ep_mps(struct usbd_class_data *const data)
{
	struct foobaz_iface_desc *desc = data->desc;

	return desc->if1_out_ep.wMaxPacketSize;
}

static size_t foobaz_ep_addr_out(struct usbd_class_data *const data)
{
	struct foobaz_iface_desc *desc = data->desc;

	return desc->if1_out_ep.bEndpointAddress;
}

static size_t foobaz_ep_addr_in(struct usbd_class_data *const data)
{
	struct foobaz_iface_desc *desc = data->desc;

	return desc->if1_in_ep.bEndpointAddress;
}

static void foobaz_update(struct usbd_class_node *const node,
			  uint8_t iface, uint8_t alternate)
{
	shell_info(ctx_shell,
		   "dev: New configuration, interface %u alternate %u",
		   iface, alternate);
}

static int foobaz_cth(struct usbd_class_node *const node,
		      const struct usb_setup_packet *const setup,
		      struct net_buf *const buf)
{
	size_t min_len = MIN(sizeof(foobaz_buf), setup->wLength);

	if (setup->bRequest == FOOBAZ_VREQ_IN) {
		if (buf == NULL) {
			errno = -ENOMEM;
			return 0;
		}

		net_buf_add_mem(buf, foobaz_buf, min_len);
		shell_info(ctx_shell,
			   "dev: conrol transfer to host, wLength %u | %u",
			   setup->wLength, min_len);

		return 0;
	}

	errno = -ENOTSUP;
	return 0;
}

static int foobaz_ctd(struct usbd_class_node *const node,
		      const struct usb_setup_packet *const setup,
		      const struct net_buf *const buf)
{
	size_t min_len = MIN(sizeof(foobaz_buf), setup->wLength);

	if (setup->bRequest == FOOBAZ_VREQ_OUT) {
		shell_info(ctx_shell,
			   "dev: control transfer to device, wLength %u | %u",
			   setup->wLength, min_len);
		memcpy(foobaz_buf, buf->data, min_len);
		return 0;
	}

	errno = -ENOTSUP;
	return 0;
}

static int foobaz_request_cancelled(struct usbd_class_node *const node,
				    struct net_buf *buf)
{
	struct udc_buf_info *bi;

	bi = udc_get_buf_info(buf);
	shell_warn(ctx_shell, "Request ep 0x%02x cancelled", bi->ep);
	shell_warn(ctx_shell, "|-> %p", buf);
	for (struct net_buf *n = buf; n->frags != NULL; n = n->frags) {
		shell_warn(ctx_shell, "|-> %p", n->frags);
	}

	return 0;
}

static int foobaz_ep_request(struct usbd_class_node *const node,
			     struct net_buf *buf, int err)
{
	struct usbd_contex *uds_ctx = node->data->uds_ctx;
	struct udc_buf_info *bi;

	bi = udc_get_buf_info(buf);
	shell_info(ctx_shell, "dev: Handle request ep 0x%02x, len %u",
		   bi->ep, buf->len);

	if (err) {
		if (err == -ECONNABORTED) {
			foobaz_request_cancelled(node, buf);
		} else {
			shell_error(ctx_shell,
				    "dev: Request failed (%d) ep 0x%02x, len %u",
				    err, bi->ep, buf->len);
		}
	}

	if (err == 0 && USB_EP_DIR_IS_OUT(bi->ep)) {
		shell_hexdump(ctx_shell, buf->data, buf->len);
	}

	return usbd_ep_buf_free(uds_ctx, buf);
}

static void foobaz_suspended(struct usbd_class_node *const node)
{
	shell_info(ctx_shell, "dev: Device suspended");
}

static void foobaz_resumed(struct usbd_class_node *const node)
{
	shell_info(ctx_shell, "dev: Device resumed");
}

static int foobaz_init(struct usbd_class_node *const node)
{
	return 0;
}

/* Define foobaz interface to USB device class API */
struct usbd_class_api foobaz_api = {
	.update = foobaz_update,
	.control_to_host = foobaz_cth,
	.control_to_dev = foobaz_ctd,
	.request = foobaz_ep_request,
	.suspended = foobaz_suspended,
	.resumed = foobaz_resumed,
	.init = foobaz_init,
};

static struct usbd_class_data foobaz_data = {
	.desc = (struct usb_desc_header *)&foobaz_desc,
	.v_reqs = &foobaz_vregs,
};

USBD_DEFINE_CLASS(foobaz, &foobaz_api, &foobaz_data);

USBD_CONFIGURATION_DEFINE(config_foo, USB_SCD_SELF_POWERED, 200);
USBD_CONFIGURATION_DEFINE(config_baz, USB_SCD_REMOTE_WAKEUP, 200);

USBD_DESC_LANG_DEFINE(lang);
USBD_DESC_STRING_DEFINE(mfr, "ZEPHYR", 1);
USBD_DESC_STRING_DEFINE(product, "Zephyr USBD foobaz", 2);
USBD_DESC_STRING_DEFINE(sn, "0123456789ABCDEF", 3);

USBD_DEVICE_DEFINE(uds_ctx, DEVICE_DT_GET(DT_NODELABEL(zephyr_udc0)),
		   0x2fe3, 0xffff);

int cmd_wakeup_request(const struct shell *sh,
		       size_t argc, char **argv)
{
	int err;

	err = usbd_wakeup_request(&uds_ctx);
	if (err) {
		shell_error(sh, "dev: Failed to wakeup remote %d", err);
	} else {
		shell_print(sh, "dev: Requested remote wakeup");
	}

	return err;
}

static int cmd_submit_request(const struct shell *sh,
			      size_t argc, char **argv)
{
	struct net_buf *buf;
	size_t len;
	uint8_t ep;
	int ret;

	ep = strtol(argv[1], NULL, 16);

	if (ep != foobaz_ep_addr_out(&foobaz_data) &&
	    ep != foobaz_ep_addr_in(&foobaz_data)) {
		struct foobaz_iface_desc *desc = foobaz_data.desc;

		shell_error(sh, "dev: Endpoint address not valid %x",
			    desc->if1_out_ep.bEndpointAddress);


		return -EINVAL;
	}

	if (argc > 2) {
		len = strtol(argv[2], NULL, 10);
	} else {
		len = foobaz_get_ep_mps(&foobaz_data);
	}

	if (USB_EP_DIR_IS_IN(ep)) {
		len = MIN(len, sizeof(foobaz_buf));
	}

	buf = usbd_ep_buf_alloc(&foobaz, ep, len);
	if (buf == NULL) {
		return -ENOMEM;
	}

	if (USB_EP_DIR_IS_IN(ep)) {
		net_buf_add_mem(buf, foobaz_buf, len);
	}

	shell_print(sh, "dev: Submit ep 0x%02x len %u buf %p", ep, len, buf);
	ret = usbd_ep_enqueue(&foobaz, buf);
	if (ret) {
		shell_print(sh, "dev: Failed to queue request buffer");
		usbd_ep_buf_free(&uds_ctx, buf);
	}

	return ret;
}

static int cmd_cancel_request(const struct shell *sh,
			      size_t argc, char **argv)
{
	uint8_t ep;
	int ret;

	shell_print(sh, "dev: Request %s %s", argv[1], argv[2]);

	ep = strtol(argv[1], NULL, 16);
	if (ep != foobaz_ep_addr_out(&foobaz_data) &&
	    ep != foobaz_ep_addr_in(&foobaz_data)) {
		shell_error(sh, "dev: Endpoint address not valid");
		return -EINVAL;
	}

	ret = usbd_ep_dequeue(&uds_ctx, ep);
	if (ret) {
		shell_print(sh, "dev: Failed to dequeue request buffer");
	}

	return ret;
}

static int cmd_endpoint_halt(const struct shell *sh,
			     size_t argc, char **argv)
{
	uint8_t ep;
	int ret = 0;

	ep = strtol(argv[1], NULL, 16);
	if (ep != foobaz_ep_addr_out(&foobaz_data) &&
	    ep != foobaz_ep_addr_in(&foobaz_data)) {
		shell_error(sh, "dev: Endpoint address not valid");
		return -EINVAL;
	}

	if (!strcmp(argv[2], "set")) {
		ret = usbd_ep_set_halt(&uds_ctx, ep);
	} else if (!strcmp(argv[2], "clear")) {
		ret = usbd_ep_clear_halt(&uds_ctx, ep);
	} else {
		shell_error(sh, "dev: Invalid argument: %s", argv[1]);
		return -EINVAL;
	}

	if (ret) {
		shell_print(sh, "dev: endpoint %s %s halt failed",
			    argv[2], argv[1]);
	} else {
		shell_print(sh, "dev: endpoint %s %s halt successful",
			    argv[2], argv[1]);
	}

	return ret;
}

static int cmd_register(const struct shell *sh,
			size_t argc, char **argv)
{
	uint8_t cfg;
	int ret;

	cfg = strtol(argv[2], NULL, 10);
	ret = usbd_register_class(&uds_ctx, argv[1], cfg);

	if (ret) {
		shell_error(sh,
			    "dev: failed to add USB class %s to configuration %u",
			    argv[1], cfg);
	} else {
		shell_print(sh,
			    "dev: added USB class %s to configuration %u",
			    argv[1], cfg);
	}

	return ret;
}

static int cmd_unregister(const struct shell *sh,
			  size_t argc, char **argv)
{
	uint8_t cfg;
	int ret;

	cfg = strtol(argv[2], NULL, 10);
	ret = usbd_unregister_class(&uds_ctx, argv[1], cfg);
	if (ret) {
		shell_error(sh,
			    "dev: failed to remove USB class %s from configuration %u",
			    argv[1], cfg);
	} else {
		shell_print(sh,
			    "dev: removed USB class %s from configuration %u",
			    argv[1], cfg);
	}

	return ret;
}

static int cmd_usbd_magic(const struct shell *sh,
			  size_t argc, char **argv)
{
	int err;

	err = usbd_add_descriptor(&uds_ctx, &lang);
	err |= usbd_add_descriptor(&uds_ctx, &mfr);
	err |= usbd_add_descriptor(&uds_ctx, &product);
	err |= usbd_add_descriptor(&uds_ctx, &sn);
	if (err) {
		shell_error(sh, "dev: Failed to initialize descriptors, %d", err);
	}

	err = usbd_add_configuration(&uds_ctx, &config_foo);
	if (err) {
		shell_error(sh, "dev: Failed to add configuration");
	}

	err = usbd_register_class(&uds_ctx, "foobaz", 1);
	if (err) {
		shell_error(sh, "dev: Failed to add foobaz class");
	}

	ctx_shell = sh;
	err = usbd_init(&uds_ctx);
	if (err) {
		shell_error(sh, "dev: Failed to initialize device support");
	}

	err = usbd_enable(&uds_ctx);
	if (err) {
		shell_error(sh, "dev: Failed to enable device support");
	}

	return err;
}

static int cmd_usbd_defaults(const struct shell *sh,
			     size_t argc, char **argv)
{
	int err;

	err = usbd_add_descriptor(&uds_ctx, &lang);
	err |= usbd_add_descriptor(&uds_ctx, &mfr);
	err |= usbd_add_descriptor(&uds_ctx, &product);
	err |= usbd_add_descriptor(&uds_ctx, &sn);

	if (err) {
		shell_error(sh, "dev: Failed to initialize descriptors, %d", err);
	} else {
		shell_print(sh, "dev: USB descriptors initialized");
	}

	return err;
}

static int cmd_usbd_init(const struct shell *sh,
			 size_t argc, char **argv)
{
	int err;

	ctx_shell = sh;
	err = usbd_init(&uds_ctx);

	if (err == -EALREADY) {
		shell_error(sh, "dev: USB already initialized");
	} else if (err) {
		shell_error(sh, "dev: Failed to initialize %d", err);
	} else {
		shell_print(sh, "dev: USB initialized");
	}

	return err;
}

static int cmd_usbd_enable(const struct shell *sh,
			   size_t argc, char **argv)
{
	int err;

	err = usbd_enable(&uds_ctx);

	if (err == -EALREADY) {
		shell_error(sh, "dev: USB already enabled");
	} else if (err) {
		shell_error(sh, "dev: Failed to enable USB, error %d", err);
	} else {
		shell_print(sh, "dev: USB enabled");
	}

	return err;
}

static int cmd_usbd_disable(const struct shell *sh,
			    size_t argc, char **argv)
{
	int err;

	err = usbd_disable(&uds_ctx);

	if (err) {
		shell_error(sh, "dev: Failed to disable USB");
		return err;
	}

	shell_print(sh, "dev: USB disabled");

	return 0;
}

static int cmd_usbd_shutdown(const struct shell *sh,
			     size_t argc, char **argv)
{
	int err;

	err = usbd_shutdown(&uds_ctx);

	if (err) {
		shell_error(sh, "dev: Failed to shutdown USB");
		return err;
	}

	shell_print(sh, "dev: USB completely disabled");

	return 0;
}

static int cmd_device_bcd(const struct shell *sh, size_t argc,
			  char *argv[])
{
	uint16_t bcd;
	int ret;

	bcd = strtol(argv[1], NULL, 16);
	ret = usbd_device_set_bcd(&uds_ctx, bcd);
	if (ret) {
		shell_error(sh, "dev: failed to set device bcdUSB to %x", bcd);
	}

	return ret;
}

static int cmd_device_pid(const struct shell *sh, size_t argc,
			  char *argv[])
{
	uint16_t pid;
	int ret;

	pid = strtol(argv[1], NULL, 16);
	ret = usbd_device_set_pid(&uds_ctx, pid);
	if (ret) {
		shell_error(sh, "dev: failed to set device idProduct to %x", pid);
	}

	return ret;
}

static int cmd_device_vid(const struct shell *sh, size_t argc,
			  char *argv[])
{
	uint16_t vid;
	int ret;

	vid = strtol(argv[1], NULL, 16);
	ret = usbd_device_set_vid(&uds_ctx, vid);
	if (ret) {
		shell_error(sh, "dev: failed to set device idVendor to %x", vid);
	}

	return ret;
}

static int cmd_device_class(const struct shell *sh, size_t argc,
			    char *argv[])
{
	uint8_t value;
	int ret;

	value = strtol(argv[1], NULL, 16);
	ret = usbd_device_set_class(&uds_ctx, value);
	if (ret) {
		shell_error(sh, "dev: failed to set device class to %x", value);
	}

	return ret;
}

static int cmd_device_subclass(const struct shell *sh, size_t argc,
			       char *argv[])
{
	uint8_t value;
	int ret;

	value = strtol(argv[1], NULL, 16);
	ret = usbd_device_set_subclass(&uds_ctx, value);
	if (ret) {
		shell_error(sh, "dev: failed to set device subclass to %x", value);
	}

	return ret;
}

static int cmd_device_proto(const struct shell *sh, size_t argc,
			    char *argv[])
{
	uint8_t value;
	int ret;

	value = strtol(argv[1], NULL, 16);
	ret = usbd_device_set_proto(&uds_ctx, value);
	if (ret) {
		shell_error(sh, "dev: failed to set device proto to %x", value);
	}

	return ret;
}

static int cmd_config_add(const struct shell *sh, size_t argc,
			  char *argv[])
{
	uint8_t cfg;
	int ret;

	cfg = strtol(argv[1], NULL, 10);

	if (cfg == 1) {
		ret = usbd_add_configuration(&uds_ctx, &config_foo);
	} else if (cfg == 2) {
		ret = usbd_add_configuration(&uds_ctx, &config_baz);
	} else {
		shell_error(sh, "dev: Configuration %u not available", cfg);
		return -EINVAL;
	}

	if (ret) {
		shell_error(sh, "dev: failed to add configuration %u", cfg);
	}

	return ret;
}

static int cmd_config_self(const struct shell *sh, size_t argc,
			   char *argv[])
{
	bool self;
	uint8_t cfg;
	int ret;

	cfg = strtol(argv[1], NULL, 10);
	if (!strcmp(argv[2], "yes")) {
		self = true;
	} else {
		self = false;
	}

	ret = usbd_config_attrib_self(&uds_ctx, cfg, self);
	if (ret) {
		shell_error(sh,
			    "dev: failed to set attribute self powered to %u",
			    cfg);
	}

	return ret;
}

static int cmd_config_rwup(const struct shell *sh, size_t argc,
			   char *argv[])
{
	bool rwup;
	uint8_t cfg;
	int ret;

	cfg = strtol(argv[1], NULL, 10);
	if (!strcmp(argv[2], "yes")) {
		rwup = true;
	} else {
		rwup = false;
	}

	ret = usbd_config_attrib_rwup(&uds_ctx, cfg, rwup);
	if (ret) {
		shell_error(sh,
			    "dev: failed to set attribute remote wakeup to %x",
			    cfg);
	}

	return ret;
}

static int cmd_config_power(const struct shell *sh, size_t argc,
			    char *argv[])
{
	uint8_t cfg;
	uint8_t power;
	int ret;

	cfg = strtol(argv[1], NULL, 10);
	power = strtol(argv[1], NULL, 10);

	ret = usbd_config_maxpower(&uds_ctx, cfg, power);
	if (ret) {
		shell_error(sh, "dev: failed to set bMaxPower value to %u", cfg);
	}

	return ret;
}

static void class_node_name_lookup(size_t idx, struct shell_static_entry *entry)
{
	size_t match_idx = 0;

	entry->syntax = NULL;
	entry->handler = NULL;
	entry->help = NULL;
	entry->subcmd = NULL;

	STRUCT_SECTION_FOREACH(usbd_class_node, node) {
		if ((node->name != NULL) && (strlen(node->name) != 0)) {
			if (match_idx == idx) {
				entry->syntax = node->name;
				break;
			}

			++match_idx;
		}
	}
}

SHELL_DYNAMIC_CMD_CREATE(dsub_node_name, class_node_name_lookup);

SHELL_STATIC_SUBCMD_SET_CREATE(device_cmds,
	SHELL_CMD_ARG(bcd, NULL, "<bcdUSB>",
		      cmd_device_bcd, 2, 0),
	SHELL_CMD_ARG(pid, NULL, "<idProduct>",
		      cmd_device_pid, 2, 0),
	SHELL_CMD_ARG(vid, NULL, "<idVendor>",
		      cmd_device_vid, 2, 0),
	SHELL_CMD_ARG(class, NULL, "<bDeviceClass>",
		      cmd_device_class, 2, 0),
	SHELL_CMD_ARG(subclass, NULL, "<bDeviceSubClass>",
		      cmd_device_subclass, 2, 0),
	SHELL_CMD_ARG(proto, NULL, "<bDeviceProtocol>",
		      cmd_device_proto, 2, 0),
	SHELL_SUBCMD_SET_END
);

SHELL_STATIC_SUBCMD_SET_CREATE(config_cmds,
	SHELL_CMD_ARG(add, NULL, "<configuration>",
		      cmd_config_add, 2, 0),
	SHELL_CMD_ARG(power, NULL, "<configuration> <bMaxPower>",
		      cmd_config_power, 3, 0),
	SHELL_CMD_ARG(rwup, NULL, "<configuration> <yes, no>",
		      cmd_config_rwup, 3, 0),
	SHELL_CMD_ARG(self, NULL, "<configuration> <yes, no>",
		      cmd_config_self, 3, 0),
	SHELL_SUBCMD_SET_END
);

SHELL_STATIC_SUBCMD_SET_CREATE(class_cmds,
	SHELL_CMD_ARG(add, &dsub_node_name, "<name> <configuration>",
		      cmd_register, 3, 0),
	SHELL_CMD_ARG(add, &dsub_node_name, "<name> <configuration>",
		      cmd_unregister, 3, 0),
	SHELL_SUBCMD_SET_END
);

SHELL_STATIC_SUBCMD_SET_CREATE(endpoint_cmds,
	SHELL_CMD_ARG(halt, NULL, "<endpoint> <set clear>",
		      cmd_endpoint_halt, 3, 0),
	SHELL_CMD_ARG(submit, NULL, "<endpoint> [length]",
		      cmd_submit_request, 2, 1),
	SHELL_CMD_ARG(cancel, NULL, "<endpoint>",
		      cmd_cancel_request, 2, 0),
	SHELL_SUBCMD_SET_END
);

SHELL_STATIC_SUBCMD_SET_CREATE(sub_usbd_cmds,
	SHELL_CMD_ARG(wakeup, NULL, "[none]",
		      cmd_wakeup_request, 1, 0),
	SHELL_CMD_ARG(magic, NULL, "[none]",
		      cmd_usbd_magic, 1, 0),
	SHELL_CMD_ARG(defaults, NULL, "[none]",
		      cmd_usbd_defaults, 1, 0),
	SHELL_CMD_ARG(init, NULL, "[none]",
		      cmd_usbd_init, 1, 0),
	SHELL_CMD_ARG(enable, NULL, "[none]",
		      cmd_usbd_enable, 1, 0),
	SHELL_CMD_ARG(disable, NULL, "[none]",
		      cmd_usbd_disable, 1, 0),
	SHELL_CMD_ARG(shutdown, NULL, "[none]",
		      cmd_usbd_shutdown, 1, 0),
	SHELL_CMD_ARG(device, &device_cmds, "device commands",
		      NULL, 1, 0),
	SHELL_CMD_ARG(config, &config_cmds, "configuration commands",
		      NULL, 1, 0),
	SHELL_CMD_ARG(class, &class_cmds, "class commands",
		      NULL, 1, 0),
	SHELL_CMD_ARG(endpoint, &endpoint_cmds, "endpoint commands",
		      NULL, 1, 0),
	SHELL_SUBCMD_SET_END
);

SHELL_CMD_REGISTER(usbd, &sub_usbd_cmds, "USB device support commands", NULL);
