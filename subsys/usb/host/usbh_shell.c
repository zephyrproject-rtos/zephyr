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
#include <zephyr/sys/byteorder.h>
#include <zephyr/usb/usbh.h>
#include <zephyr/usb/usb_ch9.h>

#include "usbh_device.h"
#include "usbh_ch9.h"
#include "usbh_desc.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(usbh_shell, CONFIG_USBH_LOG_LEVEL);

#define FOOBAZ_VREQ_OUT		0x5b
#define FOOBAZ_VREQ_IN		0x5c

STRUCT_SECTION_START_EXTERN(usbh_context);
static struct usbh_context *my_uhs_ctx = TYPE_SECTION_START(usbh_context);
static uint8_t vreq_test_buf[1024];

static struct usbh_context *get_uhs_ctx_or_error(const struct shell *sh)
{
	if (my_uhs_ctx != NULL) {
		return my_uhs_ctx;
	}

	shell_error(sh, "There is no USB host context available");

	return NULL;
}

static void print_desc_field(const struct shell *sh, int indent,
			     const char *name, const char *value,
			     const char *comment)
{
	if (comment != NULL && comment[0] != '\0') {
		shell_print(sh, "%*s%-20s %12s %s",
			    indent, "", name, value, comment);
	} else {
		shell_print(sh, "%*s%-20s %12s",
			    indent, "", name, value);
	}
}

static void print_desc_u(const struct shell *sh, int indent,
			 const char *name, unsigned int value,
			 const char *comment)
{
	char buf[16];

	snprintk(buf, sizeof(buf), "%u", value);
	print_desc_field(sh, indent, name, buf, comment);
}

static void print_desc_x8(const struct shell *sh, int indent,
			  const char *name, uint8_t value,
			  const char *comment)
{
	char buf[16];

	snprintk(buf, sizeof(buf), "0x%02X", value);
	print_desc_field(sh, indent, name, buf, comment);
}

static void print_desc_x16(const struct shell *sh, int indent,
			   const char *name, uint16_t value,
			   const char *comment)
{
	char buf[16];

	snprintk(buf, sizeof(buf), "0x%04X", value);
	print_desc_field(sh, indent, name, buf, comment);
}

static void print_desc_bcd(const struct shell *sh, int indent,
			   const char *name, uint16_t value,
			   const char *comment)
{
	char buf[16];

	snprintk(buf, sizeof(buf), "%x.%02x", value >> 8, value & 0xff);
	print_desc_field(sh, indent, name, buf, comment);
}

static void print_dev_desc_indent(const struct shell *sh, const int indent,
				  const struct usb_device_descriptor *const desc)
{
	uint8_t dindent = indent + 2; /* Data has a small indent */

	shell_print(sh, "%*sDevice Descriptor:", indent, "");
	print_desc_u(sh, dindent, "bLength", desc->bLength, NULL);
	print_desc_u(sh, dindent, "bDescriptorType", desc->bDescriptorType, NULL);
	print_desc_bcd(sh, dindent, "bcdUSB", desc->bcdUSB, NULL);
	print_desc_u(sh, dindent, "bDeviceClass", desc->bDeviceClass, NULL);
	print_desc_u(sh, dindent, "bDeviceSubClass", desc->bDeviceSubClass, NULL);
	print_desc_u(sh, dindent, "bDeviceProtocol", desc->bDeviceProtocol, NULL);
	print_desc_u(sh, dindent, "bMaxPacketSize0", desc->bMaxPacketSize0, NULL);
	print_desc_x16(sh, dindent, "idVendor", desc->idVendor, NULL);
	print_desc_x16(sh, dindent, "idProduct", desc->idProduct, NULL);
	print_desc_bcd(sh, dindent, "bcdDevice", desc->bcdDevice, NULL);
	print_desc_u(sh, dindent, "iManufacturer", desc->iManufacturer, NULL);
	print_desc_u(sh, dindent, "iProduct", desc->iProduct, NULL);
	print_desc_u(sh, dindent, "iSerial", desc->iSerialNumber, NULL);
	print_desc_u(sh, dindent, "bNumConfigurations", desc->bNumConfigurations, NULL);
}

static void print_cfg_desc_indent(const struct shell *sh, const int indent,
				  const struct usb_cfg_descriptor *const desc)
{
	uint8_t dindent = indent + 2; /* Data has a small indent */

	shell_print(sh, "%*sConfiguration Descriptor:", indent, "");
	print_desc_u(sh, dindent, "bLength", desc->bLength, NULL);
	print_desc_u(sh, dindent, "bDescriptorType", desc->bDescriptorType, NULL);
	print_desc_x16(sh, dindent, "wTotalLength", desc->wTotalLength, NULL);
	print_desc_u(sh, dindent, "bNumInterfaces", desc->bNumInterfaces, NULL);
	print_desc_u(sh, dindent, "bConfigurationValue", desc->bConfigurationValue, NULL);
	print_desc_u(sh, dindent, "iConfiguration", desc->iConfiguration, NULL);
	print_desc_x8(sh, dindent, "bmAttributes", desc->bmAttributes, NULL);
	print_desc_u(sh, dindent, "bMaxPower", desc->bMaxPower * 2, "mA");
}

static void print_iface_desc_indent(const struct shell *sh, const int indent,
				    const struct usb_if_descriptor *const desc)
{
	uint8_t dindent = indent + 2; /* Data has a small indent */

	shell_print(sh, "%*sInterface Descriptor:", indent, "");
	print_desc_u(sh, dindent, "bLength", desc->bLength, NULL);
	print_desc_u(sh, dindent, "bDescriptorType", desc->bDescriptorType, NULL);
	print_desc_u(sh, dindent, "bInterfaceNumber", desc->bInterfaceNumber, NULL);
	print_desc_u(sh, dindent, "bAlternateSetting", desc->bAlternateSetting, NULL);
	print_desc_u(sh, dindent, "bNumEndpoints", desc->bNumEndpoints, NULL);
	print_desc_u(sh, dindent, "bInterfaceClass", desc->bInterfaceClass, NULL);
	print_desc_u(sh, dindent, "bInterfaceSubClass", desc->bInterfaceSubClass, NULL);
	print_desc_u(sh, dindent, "bInterfaceProtocol", desc->bInterfaceProtocol, NULL);
	print_desc_u(sh, dindent, "iInterface", desc->iInterface, NULL);
}

static void print_assoc_desc_indent(const struct shell *sh, const int indent,
				    const struct usb_association_descriptor *const desc)
{
	uint8_t dindent = indent + 2; /* Data has a small indent */

	shell_print(sh, "%*sInterface Association:", indent, "");
	print_desc_u(sh, dindent, "bLength", desc->bLength, NULL);
	print_desc_u(sh, dindent, "bDescriptorType", desc->bDescriptorType, NULL);
	print_desc_u(sh, dindent, "bFirstInterface", desc->bFirstInterface, NULL);
	print_desc_u(sh, dindent, "bInterfaceCount", desc->bInterfaceCount, NULL);
	print_desc_u(sh, dindent, "bFunctionClass", desc->bFunctionClass, NULL);
	print_desc_u(sh, dindent, "bFunctionSubClass", desc->bFunctionSubClass, NULL);
	print_desc_u(sh, dindent, "bFunctionProtocol", desc->bFunctionProtocol, NULL);
	print_desc_u(sh, dindent, "iFunction", desc->iFunction, NULL);
}

static void print_ep_desc_indent(const struct shell *sh, const int indent,
				 const struct usb_ep_descriptor *const desc)
{
	uint8_t dindent = indent + 2; /* Data has a small indent */

	shell_print(sh, "%*sEndpoint Descriptor:", indent, "");
	print_desc_u(sh, dindent, "bLength", desc->bLength, NULL);
	print_desc_u(sh, dindent, "bDescriptorType", desc->bDescriptorType, NULL);
	print_desc_x8(sh, dindent, "bEndpointAddress", desc->bEndpointAddress, NULL);
	print_desc_x8(sh, dindent, "bmAttributes", desc->bmAttributes, NULL);
	print_desc_u(sh, dindent, "wMaxPacketSize", desc->wMaxPacketSize, NULL);
	print_desc_u(sh, dindent, "bInterval", desc->bInterval, NULL);
}

static void print_unhandled_desc_indent(const struct shell *sh, const int indent,
					const struct usb_desc_header *const dhp)
{
	uint8_t dindent = indent + 2; /* Data has a small indent */

	shell_print(sh, "%*sUnhandled Descriptor:", indent, "");
	print_desc_u(sh, dindent, "bLength", dhp->bLength, NULL);
	print_desc_u(sh, dindent, "bDescriptorType", dhp->bDescriptorType, NULL);
}

static void print_desc(const struct shell *sh, const void *const desc)
{
	const struct usb_desc_header *const dhp = desc;

	switch (dhp->bDescriptorType) {
	case USB_DESC_CONFIGURATION:
		print_cfg_desc_indent(sh, 2, desc);
		break;
	case USB_DESC_INTERFACE:
		print_iface_desc_indent(sh, 4, desc);
		break;
	case USB_DESC_ENDPOINT:
		print_ep_desc_indent(sh, 6, desc);
		break;
	case USB_DESC_INTERFACE_ASSOC:
		print_assoc_desc_indent(sh, 4, desc);
		break;
	default:
		print_unhandled_desc_indent(sh, 4, desc);
	}
}

K_SEM_DEFINE(bulk_req_sync, 0, 1);

static int bulk_req_cb(struct usb_device *const dev, struct uhc_transfer *const xfer)
{
	if (xfer->err == -ECONNRESET) {
		LOG_INF("Bulk transfer canceled");
	} else if (xfer->err) {
		LOG_WRN("Bulk request failed, err %d", xfer->err);
	} else {
		LOG_INF("Bulk request finished");
	}

	usbh_xfer_buf_free(dev, xfer->buf);
	usbh_xfer_free(dev, xfer);
	k_sem_give(&bulk_req_sync);

	return 0;
}

static int cmd_bulk(const struct shell *sh, size_t argc, char **argv)
{
	static struct usb_device *udev;
	struct usbh_context *uhs_ctx;
	struct uhc_transfer *xfer;
	struct net_buf *buf;
	uint8_t addr;
	uint8_t ep;
	size_t len;
	int ret;

	uhs_ctx = get_uhs_ctx_or_error(sh);
	if (uhs_ctx == NULL) {
		return -ENODEV;
	}

	addr = strtol(argv[1], NULL, 10);
	udev = usbh_device_get(uhs_ctx, addr);
	if (udev == NULL) {
		shell_error(sh, "host: No USB device with address %u", addr);
		return -ENOMEM;
	}

	ep = strtol(argv[2], NULL, 16);
	len = MIN(sizeof(vreq_test_buf), strtol(argv[3], NULL, 10));

	xfer = usbh_xfer_alloc(udev, ep, bulk_req_cb, NULL);
	if (!xfer) {
		shell_error(sh, "host: Failed to allocate transfer");
		return -ENOMEM;
	}

	buf = usbh_xfer_buf_alloc(udev, len);
	if (!buf) {
		shell_error(sh, "host: Failed to allocate buffer");
		usbh_xfer_free(udev, xfer);
		return -ENOMEM;
	}

	xfer->buf = buf;
	if (USB_EP_DIR_IS_OUT(ep)) {
		net_buf_add_mem(buf, vreq_test_buf, len);
	}

	k_sem_reset(&bulk_req_sync);
	ret = usbh_xfer_enqueue(udev, xfer);
	if (ret) {
		usbh_xfer_buf_free(udev, xfer->buf);
		usbh_xfer_free(udev, xfer);
		return ret;
	}

	if (k_sem_take(&bulk_req_sync, K_MSEC(1000)) != 0) {
		shell_print(sh, "host: Bulk transfer timeout");
		ret = usbh_xfer_dequeue(udev, xfer);
		if (ret != 0) {
			shell_error(sh, "host: Failed to cancel transfer");
			return ret;
		}

		return -ETIMEDOUT;
	}

	shell_print(sh, "host: Bulk transfer finished");

	return 0;
}

static int cmd_vendor_in(const struct shell *sh,
			 size_t argc, char **argv)
{
	const uint8_t bmRequestType = (USB_REQTYPE_DIR_TO_HOST << 7) |
				      (USB_REQTYPE_TYPE_VENDOR << 5);
	const uint8_t bRequest = FOOBAZ_VREQ_IN;
	static struct usb_device *udev;
	const uint16_t wValue = 0x0000;
	struct usbh_context *uhs_ctx;
	struct net_buf *buf;
	uint16_t wLength;
	uint8_t addr;
	int ret;

	uhs_ctx = get_uhs_ctx_or_error(sh);
	if (uhs_ctx == NULL) {
		return -ENODEV;
	}

	addr = strtol(argv[1], NULL, 10);
	udev = usbh_device_get(uhs_ctx, addr);
	if (udev == NULL) {
		shell_error(sh, "host: No USB device with address %u", addr);
		return -ENOMEM;
	}

	wLength = MIN(sizeof(vreq_test_buf), strtol(argv[2], NULL, 10));
	buf = usbh_xfer_buf_alloc(udev, wLength);
	if (!buf) {
		shell_error(sh, "host: Failed to allocate buffer");
		return -ENOMEM;
	}

	ret = usbh_req_setup(udev, bmRequestType, bRequest, wValue, 0, wLength, buf);
	if (ret == 0) {
		memcpy(vreq_test_buf, buf->data, MIN(buf->len, wLength));
	}

	usbh_xfer_buf_free(udev, buf);

	return ret;
}

static int cmd_vendor_out(const struct shell *sh,
			  size_t argc, char **argv)
{
	const uint8_t bmRequestType = (USB_REQTYPE_DIR_TO_DEVICE << 7) |
				      (USB_REQTYPE_TYPE_VENDOR << 5);
	const uint8_t bRequest = FOOBAZ_VREQ_OUT;
	static struct usb_device *udev;
	const uint16_t wValue = 0x0000;
	struct usbh_context *uhs_ctx;
	struct net_buf *buf;
	uint16_t wLength;
	uint8_t addr;
	int ret;

	uhs_ctx = get_uhs_ctx_or_error(sh);
	if (uhs_ctx == NULL) {
		return -ENODEV;
	}

	addr = strtol(argv[1], NULL, 10);
	udev = usbh_device_get(uhs_ctx, addr);
	if (udev == NULL) {
		shell_error(sh, "host: No USB device with address %u", addr);
		return -ENOMEM;
	}

	wLength = MIN(sizeof(vreq_test_buf), strtol(argv[2], NULL, 10));
	buf = usbh_xfer_buf_alloc(udev, wLength);
	if (!buf) {
		shell_error(sh, "host: Failed to allocate buffer");
		return -ENOMEM;
	}

	net_buf_add_mem(buf, &vreq_test_buf, wLength);
	ret = usbh_req_setup(udev, bmRequestType, bRequest, wValue, 0, wLength, buf);
	usbh_xfer_buf_free(udev, buf);

	return ret;
}

static int cmd_desc_device(const struct shell *sh,
			   size_t argc, char **argv)
{
	struct usb_device_descriptor desc;
	static struct usb_device *udev;
	struct usbh_context *uhs_ctx;
	uint8_t addr;
	int err;

	uhs_ctx = get_uhs_ctx_or_error(sh);
	if (uhs_ctx == NULL) {
		return -ENODEV;
	}

	addr = strtol(argv[1], NULL, 10);
	udev = usbh_device_get(uhs_ctx, addr);
	if (udev == NULL) {
		shell_error(sh, "host: No USB device with address %u", addr);
		return -ENOMEM;
	}

	err = usbh_req_desc_dev(udev, sizeof(desc), &desc);
	if (err) {
		shell_error(sh, "host: Failed to request device descriptor");
	} else {
		print_dev_desc_indent(sh, 0, &desc);
	}

	return err;
}

static int cmd_desc_config(const struct shell *sh,
			   size_t argc, char **argv)
{
	struct usb_cfg_descriptor desc;
	static struct usb_device *udev;
	struct usbh_context *uhs_ctx;
	uint8_t addr;
	uint8_t cfg;
	int err;

	uhs_ctx = get_uhs_ctx_or_error(sh);
	if (uhs_ctx == NULL) {
		return -ENODEV;
	}

	addr = strtol(argv[1], NULL, 10);
	udev = usbh_device_get(uhs_ctx, addr);
	if (udev == NULL) {
		shell_error(sh, "host: No USB device with address %u", addr);
		return -ENOMEM;
	}

	cfg = strtol(argv[2], NULL, 10);

	err = usbh_req_desc_cfg(udev, cfg, sizeof(desc), &desc);
	if (err) {
		shell_error(sh, "host: Failed to request configuration descriptor");
	} else {
		print_cfg_desc_indent(sh, 0, &desc);
	}

	return err;
}

static int cmd_desc_string(const struct shell *sh,
			   size_t argc, char **argv)
{
	const uint8_t type = USB_DESC_STRING;
	static struct usb_device *udev;
	struct usbh_context *uhs_ctx;
	struct net_buf *buf;
	uint8_t addr;
	uint8_t id;
	uint8_t idx;
	int err;

	uhs_ctx = get_uhs_ctx_or_error(sh);
	if (uhs_ctx == NULL) {
		return -ENODEV;
	}

	addr = strtol(argv[1], NULL, 10);
	udev = usbh_device_get(uhs_ctx, addr);
	if (udev == NULL) {
		shell_error(sh, "host: No USB device with address %u", addr);
		return -ENOMEM;
	}

	id = strtol(argv[2], NULL, 10);
	idx = strtol(argv[3], NULL, 10);

	buf = usbh_xfer_buf_alloc(udev, 128);
	if (!buf) {
		return -ENOMEM;
	}

	err = usbh_req_desc(udev, type, idx, id, 128, buf);
	if (err) {
		shell_error(sh, "host: Failed to request string descriptor");
	} else {
		shell_hexdump(sh, buf->data, buf->len);
	}

	usbh_xfer_buf_free(udev, buf);

	return err;
}

static int cmd_feature_clear_halt(const struct shell *sh,
				  size_t argc, char **argv)
{
	static struct usb_device *udev;
	struct usbh_context *uhs_ctx;
	uint8_t addr;
	uint8_t ep;
	int err;

	uhs_ctx = get_uhs_ctx_or_error(sh);
	if (uhs_ctx == NULL) {
		return -ENODEV;
	}

	addr = strtol(argv[1], NULL, 10);
	udev = usbh_device_get(uhs_ctx, addr);
	if (udev == NULL) {
		shell_error(sh, "host: No USB device with address %u", addr);
		return -ENOMEM;
	}

	ep = strtol(argv[2], NULL, 16);

	err = usbh_req_clear_sfs_halt(udev, ep);
	if (err) {
		shell_error(sh, "host: Failed to clear halt feature");
	} else {
		shell_print(sh, "host: Device 0x%02x, ep 0x%02x halt feature cleared",
			    udev->addr, ep);
	}

	return err;
}

static int cmd_feature_set_halt(const struct shell *sh,
				size_t argc, char **argv)
{
	static struct usb_device *udev;
	struct usbh_context *uhs_ctx;
	uint8_t addr;
	uint8_t ep;
	int err;

	uhs_ctx = get_uhs_ctx_or_error(sh);
	if (uhs_ctx == NULL) {
		return -ENODEV;
	}

	addr = strtol(argv[1], NULL, 10);
	udev = usbh_device_get(uhs_ctx, addr);
	if (udev == NULL) {
		shell_error(sh, "host: No USB device with address %u", addr);
		return -ENOMEM;
	}

	ep = strtol(argv[2], NULL, 16);

	err = usbh_req_set_sfs_halt(udev, ep);
	if (err) {
		shell_error(sh, "host: Failed to set halt feature");
	} else {
		shell_print(sh, "host: Device 0x%02x, ep 0x%02x halt feature set",
			    udev->addr, ep);
	}

	return err;
}

static int cmd_feature_clear_rwup(const struct shell *sh,
				  size_t argc, char **argv)
{
	static struct usb_device *udev;
	struct usbh_context *uhs_ctx;
	uint8_t addr;
	int err;

	uhs_ctx = get_uhs_ctx_or_error(sh);
	if (uhs_ctx == NULL) {
		return -ENODEV;
	}

	addr = strtol(argv[1], NULL, 10);
	udev = usbh_device_get(uhs_ctx, addr);
	if (udev == NULL) {
		shell_error(sh, "host: No USB device with address %u", addr);
		return -ENOMEM;
	}

	err = usbh_req_clear_sfs_rwup(udev);
	if (err) {
		shell_error(sh, "host: Failed to clear rwup feature");
	} else {
		shell_print(sh, "host: Device 0x%02x, rwup feature cleared", udev->addr);
	}

	return err;
}

static int cmd_feature_set_rwup(const struct shell *sh,
				size_t argc, char **argv)
{
	static struct usb_device *udev;
	struct usbh_context *uhs_ctx;
	uint8_t addr;
	int err;

	uhs_ctx = get_uhs_ctx_or_error(sh);
	if (uhs_ctx == NULL) {
		return -ENODEV;
	}

	addr = strtol(argv[1], NULL, 10);
	udev = usbh_device_get(uhs_ctx, addr);
	if (udev == NULL) {
		shell_error(sh, "host: No USB device with address %u", addr);
		return -ENOMEM;
	}

	err = usbh_req_set_sfs_rwup(udev);
	if (err) {
		shell_error(sh, "host: Failed to set rwup feature");
	} else {
		shell_print(sh, "host: Device 0x%02x, rwup feature set", udev->addr);
	}

	return err;
}

static int cmd_feature_set_ppwr(const struct shell *sh,
				size_t argc, char **argv)
{
	static struct usb_device *udev;
	struct usbh_context *uhs_ctx;
	uint8_t addr;
	uint8_t port;
	int err;

	uhs_ctx = get_uhs_ctx_or_error(sh);
	if (uhs_ctx == NULL) {
		return -ENODEV;
	}

	addr = strtol(argv[1], NULL, 10);
	udev = usbh_device_get(uhs_ctx, addr);
	if (udev == NULL) {
		shell_error(sh, "host: No USB device with address %u", addr);
		return -ENOMEM;
	}

	port = strtol(argv[2], NULL, 10);

	err = usbh_req_set_hcfs_ppwr(udev, port);
	if (err) {
		shell_error(sh, "host: Failed to set ppwr feature");
	} else {
		shell_print(sh, "host: Device 0x%02x, port %d, ppwr feature set",
			    udev->addr, port);
	}

	return err;
}

static int cmd_feature_set_prst(const struct shell *sh,
				size_t argc, char **argv)
{
	static struct usb_device *udev;
	struct usbh_context *uhs_ctx;
	uint8_t addr;
	uint8_t port;
	int err;

	uhs_ctx = get_uhs_ctx_or_error(sh);
	if (uhs_ctx == NULL) {
		return -ENODEV;
	}

	addr = strtol(argv[1], NULL, 10);
	udev = usbh_device_get(uhs_ctx, addr);
	if (udev == NULL) {
		shell_error(sh, "host: No USB device with address %u", addr);
		return -ENOMEM;
	}

	port = strtol(argv[2], NULL, 10);

	err = usbh_req_set_hcfs_prst(udev, port);
	if (err) {
		shell_error(sh, "host: Failed to set prst feature");
	} else {
		shell_print(sh, "host: Device 0x%02x, port %d, prst feature set",
			    udev->addr, port);
	}

	return err;
}

static int cmd_config_set(const struct shell *sh,
			  size_t argc, char **argv)
{
	static struct usb_device *udev;
	struct usbh_context *uhs_ctx;
	uint8_t addr;
	uint8_t cfg;
	int err;

	uhs_ctx = get_uhs_ctx_or_error(sh);
	if (uhs_ctx == NULL) {
		return -ENODEV;
	}

	addr = strtol(argv[1], NULL, 10);
	udev = usbh_device_get(uhs_ctx, addr);
	if (udev == NULL) {
		shell_error(sh, "host: No USB device with address %u", addr);
		return -ENOMEM;
	}

	cfg = strtol(argv[2], NULL, 10);

	err = usbh_req_set_cfg(udev, cfg);
	if (err) {
		shell_error(sh, "host: Failed to set configuration");
	} else {
		shell_print(sh, "host: Device 0x%02x, new configuration %u",
			    udev->addr, cfg);
	}

	return err;
}

static int cmd_config_get(const struct shell *sh,
			  size_t argc, char **argv)
{
	static struct usb_device *udev;
	struct usbh_context *uhs_ctx;
	uint8_t addr;
	uint8_t cfg;
	int err;

	uhs_ctx = get_uhs_ctx_or_error(sh);
	if (uhs_ctx == NULL) {
		return -ENODEV;
	}

	addr = strtol(argv[1], NULL, 10);
	udev = usbh_device_get(uhs_ctx, addr);
	if (udev == NULL) {
		shell_error(sh, "host: No USB device with address %u", addr);
		return -ENOMEM;
	}

	err = usbh_req_get_cfg(udev, &cfg);
	if (err) {
		shell_error(sh, "host: Failed to get configuration");
	} else {
		shell_print(sh, "host: Device 0x%02x, current configuration %u",
			    udev->addr, cfg);
	}

	return err;
}

static int cmd_device_interface(const struct shell *sh,
				size_t argc, char **argv)
{
	static struct usb_device *udev;
	struct usbh_context *uhs_ctx;
	uint8_t iface;
	uint8_t addr;
	uint8_t alt;
	int err;

	uhs_ctx = get_uhs_ctx_or_error(sh);
	if (uhs_ctx == NULL) {
		return -ENODEV;
	}

	addr = strtol(argv[1], NULL, 10);
	udev = usbh_device_get(uhs_ctx, addr);
	if (udev == NULL) {
		shell_error(sh, "host: No USB device with address %u", addr);
		return -ENOMEM;
	}

	iface = strtol(argv[2], NULL, 10);
	alt = strtol(argv[3], NULL, 10);

	err = usbh_req_set_alt(udev, iface, alt);
	if (err) {
		shell_error(sh, "host: Failed to set interface alternate");
	} else {
		shell_print(sh, "host: Device 0x%02x, new %u alternate %u",
			    udev->addr, iface, alt);
	}

	return err;
}

static int cmd_device_address(const struct shell *sh,
			      size_t argc, char **argv)
{
	static struct usb_device *udev;
	struct usbh_context *uhs_ctx;
	uint8_t new_addr;
	uint8_t addr;
	int err;

	uhs_ctx = get_uhs_ctx_or_error(sh);
	if (uhs_ctx == NULL) {
		return -ENODEV;
	}

	addr = strtol(argv[1], NULL, 10);
	udev = usbh_device_get(uhs_ctx, addr);
	if (udev == NULL) {
		shell_error(sh, "host: No USB device with address %u", addr);
		return -ENOMEM;
	}

	new_addr = strtol(argv[2], NULL, 10);

	err = usbh_device_set_address(udev, new_addr);
	if (err) {
		shell_error(sh, "host: Failed to set address");
	} else {
		shell_print(sh, "host: New device address is %u", new_addr);
	}

	return err;
}

static const char *device_list_speed_str(enum usb_device_speed speed)
{
	switch (speed) {
	case USB_SPEED_SPEED_LS:
		return "LS";
	case USB_SPEED_SPEED_FS:
		return "FS";
	case USB_SPEED_SPEED_HS:
		return "HS";
	case USB_SPEED_SPEED_SS:
		return "SS";
	default:
		return "?";
	}
}

static const char *device_list_state_str(enum usb_device_state state)
{
	switch (state) {
	case USB_STATE_NOTCONNECTED:
		return "disc";
	case USB_STATE_DEFAULT:
		return "def";
	case USB_STATE_ADDRESSED:
		return "addr";
	case USB_STATE_CONFIGURED:
		return "cfg";
	default:
		return "?";
	}
}

static void device_list_class_str(const struct usb_device *udev, char *buf, size_t buflen)
{
	uint8_t cls = udev->dev_desc.bDeviceClass;

	if (cls == USB_BCC_HUB) {
		snprintk(buf, buflen, "hub");
		return;
	}

	if (cls == USB_BCC_MASS_STORAGE) {
		snprintk(buf, buflen, "msc");
		return;
	}

	if (cls != 0) {
		snprintk(buf, buflen, "0x%02x", cls);
		return;
	}

	if (udev->cfg_desc != NULL) {
		const struct usb_cfg_descriptor *cfg = udev->cfg_desc;
		const void *desc_end = usbh_desc_cfg_end(cfg);
		const struct usb_desc_header *dhp = (const struct usb_desc_header *)cfg;

		dhp = usbh_desc_get_next(dhp, desc_end);
		if (dhp != NULL && dhp->bDescriptorType == USB_DESC_INTERFACE) {
			const struct usb_if_descriptor *ifd = (const struct usb_if_descriptor *)dhp;

			if (ifd->bInterfaceClass == USB_BCC_HUB) {
				snprintk(buf, buflen, "hub");
				return;
			}
			if (ifd->bInterfaceClass == USB_BCC_MASS_STORAGE) {
				snprintk(buf, buflen, "msc");
				return;
			}

			snprintk(buf, buflen, "if0x%02x", ifd->bInterfaceClass);
			return;
		}
	}

	snprintk(buf, buflen, "dev");
}

static void device_list_topology_str(const struct usb_device *udev, char *buf, size_t buflen)
{
	if (udev->parent == NULL) {
		snprintk(buf, buflen, "root:%u", udev->hub_port);
	} else {
		snprintk(buf, buflen, "hub%u:%u", udev->parent->addr, udev->hub_port);
	}
}

static void device_list_driver_str(const struct usb_device *udev, const char *class_s, char *buf,
				   size_t buflen)
{
	STRUCT_SECTION_FOREACH(usbh_class_node, c_node) {
		const struct usbh_class_data *const c_data = c_node->c_data;

		if (c_node->state == USBH_CLASS_STATE_BOUND && c_data->udev == udev) {
			snprintk(buf, buflen, "%s", c_data->name);
			return;
		}
	}

	/*
	 * MSC uses a single class instance today; only one udev retains BOUND
	 * state while other MSC devices are still managed by usbh_msc_class.
	 */
	if (strcmp(class_s, "msc") == 0) {
		snprintk(buf, buflen, "usbh_msc_class");
		return;
	}

	snprintk(buf, buflen, "-");
}

static void device_list_volume_str(struct usb_device *udev, char *buf, size_t buflen)
{
	ARG_UNUSED(udev);

	snprintk(buf, buflen, "-");
}

static int cmd_device_list(const struct shell *sh,
			   size_t argc, char **argv)
{
	struct usbh_context *uhs_ctx;
	struct usb_device *udev;
	bool first = true;

	uhs_ctx = get_uhs_ctx_or_error(sh);
	if (uhs_ctx == NULL) {
		return -ENODEV;
	}

	SYS_DLIST_FOR_EACH_CONTAINER(&uhs_ctx->udevs, udev, node) {
		char class_s[16];
		char attach_s[16];
		char driver_s[24];
		char volume_s[16];

		if (first) {
			shell_print(
				sh,
				"addr vid:pid   class state spd attach   driver           volume");
			first = false;
		}

		device_list_class_str(udev, class_s, sizeof(class_s));
		device_list_topology_str(udev, attach_s, sizeof(attach_s));
		device_list_driver_str(udev, class_s, driver_s, sizeof(driver_s));
		device_list_volume_str(udev, volume_s, sizeof(volume_s));

		shell_print(sh, "%4u %04x:%04x %5s %5s %3s %-10s %-16s %s", udev->addr,
			    udev->dev_desc.idVendor, udev->dev_desc.idProduct, class_s,
			    device_list_state_str(udev->state), device_list_speed_str(udev->speed),
			    attach_s, driver_s, volume_s);
	}
	return 0;
}

static int cmd_device_info(const struct shell *sh,
			      size_t argc, char **argv)
{
	struct usb_device *udev;
	const struct usb_desc_header *dhp;
	struct usbh_context *uhs_ctx;
	uint8_t addr;

	uhs_ctx = get_uhs_ctx_or_error(sh);
	if (uhs_ctx == NULL) {
		return -ENODEV;
	}

	addr = strtol(argv[1], NULL, 10);
	udev = usbh_device_get(uhs_ctx, addr);
	if (udev == NULL) {
		shell_error(sh, "host: No USB device with address %u", addr);
		return -ENOMEM;
	}

	/* Print device descriptor */
	print_dev_desc_indent(sh, 0, &udev->dev_desc);

	dhp = udev->cfg_desc;
	if (dhp != NULL) {
		const void *desc_end = usbh_desc_cfg_end(udev->cfg_desc);

	while (dhp != NULL) {
			/* Print every entry within wTotalLength */
		print_desc(sh, dhp);
			dhp = usbh_desc_get_next(dhp, desc_end);
		}
	}

	return 0;
}

static int cmd_bus_suspend(const struct shell *sh,
			   size_t argc, char **argv)
{
	struct usbh_context *uhs_ctx;
	int err;

	uhs_ctx = get_uhs_ctx_or_error(sh);
	if (uhs_ctx == NULL) {
		return -ENODEV;
	}

	err = uhc_bus_suspend(uhs_ctx->dev);
	if (err) {
		shell_error(sh, "host: Failed to perform bus suspend %d", err);
	} else {
		shell_print(sh, "host: USB bus suspended");
	}

	return err;
}

static int cmd_bus_resume(const struct shell *sh,
			  size_t argc, char **argv)
{
	struct usbh_context *uhs_ctx;
	int err;

	uhs_ctx = get_uhs_ctx_or_error(sh);
	if (uhs_ctx == NULL) {
		return -ENODEV;
	}

	err = uhc_bus_resume(uhs_ctx->dev);
	if (err) {
		shell_error(sh, "host: Failed to perform bus resume %d", err);
	} else {
		shell_print(sh, "host: USB bus resumed");
	}

	err = uhc_sof_enable(uhs_ctx->dev);
	if (err) {
		shell_error(sh, "host: Failed to start SoF generator %d", err);
	}

	return err;
}

static int cmd_bus_reset(const struct shell *sh,
			 size_t argc, char **argv)
{
	struct usbh_context *uhs_ctx;
	int err;

	uhs_ctx = get_uhs_ctx_or_error(sh);
	if (uhs_ctx == NULL) {
		return -ENODEV;
	}

	err = uhc_bus_reset(uhs_ctx->dev);
	if (err) {
		shell_error(sh, "host: Failed to perform bus reset %d", err);
	} else {
		shell_print(sh, "host: USB bus reset");
	}

	err = uhc_sof_enable(uhs_ctx->dev);
	if (err) {
		shell_error(sh, "host: Failed to start SoF generator %d", err);
	}

	return err;
}

static int cmd_usbh_init(const struct shell *sh,
			 size_t argc, char **argv)
{
	struct usbh_context *uhs_ctx;
	int err;

	uhs_ctx = get_uhs_ctx_or_error(sh);
	if (uhs_ctx == NULL) {
		return -ENODEV;
	}

	err = usbh_init(uhs_ctx);
	if (err == -EALREADY) {
		shell_error(sh, "host: USB host already initialized");
	} else if (err) {
		shell_error(sh, "host: Failed to initialize %d", err);
	} else {
		shell_print(sh, "host: USB host initialized");
	}

	return err;
}

static int cmd_usbh_enable(const struct shell *sh,
			   size_t argc, char **argv)
{
	struct usbh_context *uhs_ctx;
	int err;

	uhs_ctx = get_uhs_ctx_or_error(sh);
	if (uhs_ctx == NULL) {
		return -ENODEV;
	}

	err = usbh_enable(uhs_ctx);
	if (err) {
		shell_error(sh, "host: Failed to enable USB host support");
	} else {
		shell_print(sh, "host: USB host enabled");
	}

	return err;
}

static int cmd_usbh_disable(const struct shell *sh,
			    size_t argc, char **argv)
{
	struct usbh_context *uhs_ctx;
	int err;

	uhs_ctx = get_uhs_ctx_or_error(sh);
	if (uhs_ctx == NULL) {
		return -ENODEV;
	}

	err = usbh_disable(uhs_ctx);
	if (err) {
		shell_error(sh, "host: Failed to disable USB host support");
	} else {
		shell_print(sh, "host: USB host disabled");
	}

	return err;
}

static int cmd_select(const struct shell *sh, size_t argc, char **argv)
{
	STRUCT_SECTION_FOREACH(usbh_context, ctx) {
		if (strcmp(argv[1], ctx->name) == 0) {
			my_uhs_ctx = ctx;
			shell_print(sh,
				    "host: select %s as my USB host context",
				    argv[1]);

			return 0;
		}
	}

	shell_error(sh, "host: failed to select %s", argv[1]);

	return -ENODEV;
}

static void host_context_lookup(size_t idx, struct shell_static_entry *entry)
{
	size_t match_idx = 0;

	entry->syntax = NULL;
	entry->handler = NULL;
	entry->help = NULL;
	entry->subcmd = NULL;

	STRUCT_SECTION_FOREACH(usbh_context, ctx) {
		if ((ctx->name != NULL) && (strlen(ctx->name) != 0)) {
			if (match_idx == idx) {
				entry->syntax = ctx->name;
				break;
			}

			++match_idx;
		}
	}
}

SHELL_DYNAMIC_CMD_CREATE(hsub_context_name, host_context_lookup);

SHELL_STATIC_SUBCMD_SET_CREATE(desc_cmds,
	SHELL_CMD_ARG(device, NULL,
		SHELL_HELP(
			"Print device descriptor",
			"<addr>\n"
			"addr: Device bus address [dec]"
		),
		cmd_desc_device, 2, 0),
	SHELL_CMD_ARG(configuration, NULL,
		SHELL_HELP(
			"Print configuration descriptor",
			"<addr> <index>\n"
			"addr:  Device bus address [dec]\n"
			"index: Configuration index [dec]"
		),
		cmd_desc_config, 3, 0),
	SHELL_CMD_ARG(string, NULL,
		SHELL_HELP(
			"Print string descriptor",
			"<addr> <id> <index>\n"
			"addr:  Device bus address [dec]\n"
			"id:    Language ID [dec]\n"
			"index: Index of string [dec]"
		),
		cmd_desc_string, 4, 0),
	SHELL_SUBCMD_SET_END
);

SHELL_STATIC_SUBCMD_SET_CREATE(feature_set_cmds,
	SHELL_CMD_ARG(rwup, NULL,
		SHELL_HELP(
			"Set feature - Remote Wakeup",
			"<addr>\n"
			"addr: Device bus address [dec]"
		),
		cmd_feature_set_rwup, 2, 0),
	SHELL_CMD_ARG(ppwr, NULL,
		SHELL_HELP(
			"Set feature - Port Power [Hub Class request]",
			"<addr> <port>\n"
			"addr: Device bus address [dec]\n"
			"port: Port number [dec]"
		),
		cmd_feature_set_ppwr, 3, 0),
	SHELL_CMD_ARG(prst, NULL,
		SHELL_HELP(
			"Set feature - Reset Port [Hub Class request]",
			"<addr> <port>\n"
			"addr: Device bus address [dec]\n"
			"port: Port number [dec]"
		),
		cmd_feature_set_prst, 3, 0),
	SHELL_CMD_ARG(halt, NULL,
		SHELL_HELP(
			"Set feature - Halt Endpoint",
			"<addr> <ep_num>\n"
			"addr:   Device bus address [dec]\n"
			"ep_num: Endpoint number [hex]"
		),
		cmd_feature_set_halt, 3, 0),
	SHELL_SUBCMD_SET_END
);

SHELL_STATIC_SUBCMD_SET_CREATE(feature_clear_cmds,
	SHELL_CMD_ARG(rwup, NULL,
		SHELL_HELP(
			"Clear feature - Remote Wakeup",
			"<addr>\n"
			"addr: Device bus address [dec]"
		),
		cmd_feature_clear_rwup, 2, 0),
	SHELL_CMD_ARG(halt, NULL,
		SHELL_HELP(
			"Clear feature - Halt Endpoint",
			"<addr> <ep_num>\n"
			"addr:   Device bus address [dec]\n"
			"ep_num: Endpoint number [hex]"
		),
		cmd_feature_clear_halt, 3, 0),
	SHELL_SUBCMD_SET_END
);

SHELL_STATIC_SUBCMD_SET_CREATE(config_cmds,
	SHELL_CMD_ARG(get, NULL,
		SHELL_HELP(
			"Get configuration",
			"<addr>\n"
			"addr: Device bus address [dec]"
		),
		cmd_config_get, 2, 0),
	SHELL_CMD_ARG(set, NULL,
		SHELL_HELP(
			"Set configuration",
			"<addr> <value>\n"
			"addr:  Device bus address [dec]\n"
			"value: Value to set [dec]"
		),
		cmd_config_set, 3, 0),
	SHELL_SUBCMD_SET_END
);

SHELL_STATIC_SUBCMD_SET_CREATE(device_cmds,
	SHELL_CMD_ARG(list, NULL,
		SHELL_HELP(
			"List of active devices",
			""
		),
		cmd_device_list, 1, 0),
	SHELL_CMD_ARG(info, NULL,
		SHELL_HELP(
			"Print device information",
			"<addr>\n"
			"addr: Device bus address [dec]\n"
		),
		cmd_device_info, 2, 0),
	SHELL_CMD_ARG(address, NULL,
		SHELL_HELP(
			"Set device address",
			"<addr> <new addr>\n"
			"addr: Device bus address [dec]\n"
			"new:  New device address [dec]"
		),
		cmd_device_address, 3, 0),
	SHELL_CMD_ARG(config, &config_cmds, "Get/Set configuration",
		      NULL, 2, 0),
	SHELL_CMD_ARG(interface, NULL,
		SHELL_HELP(
			"Set alternate interface",
			"<addr> <iface> <alt>\n"
			"addr:  Device bus address [dec]\n"
			"iface: Interface number [dec]\n"
			"alt:   Alternate setting [dec]"
		),
		cmd_device_interface, 4, 0),
	SHELL_CMD_ARG(descriptor, &desc_cmds, "Descriptor commands",
		      NULL, 2, 0),
	SHELL_CMD_ARG(feature-set, &feature_set_cmds, "Set Feature commands",
		      NULL, 2, 0),
	SHELL_CMD_ARG(feature-clear, &feature_clear_cmds, "Clear Feature commands",
		      NULL, 2, 0),
	SHELL_CMD_ARG(vendor_in, NULL,
		SHELL_HELP(
			"Vendor IN transfer",
			"<addr> <len>\n"
			"addr: Device bus address [dec]\n"
			"len:  Buffer length [dec]"
		),
		cmd_vendor_in, 3, 0),
	SHELL_CMD_ARG(vendor_out, NULL,
		SHELL_HELP(
			"Vendor OUT transfer",
			"<addr> <len>\n"
			"addr: Device bus address [dec]\n"
			"len:  Buffer length [dec]"
		),
		cmd_vendor_out, 3, 0),
	SHELL_CMD_ARG(bulk, NULL,
		SHELL_HELP(
			"Bulk IN/OUT transfer",
			"<addr> <ep_num> <len>\n"
			"addr:   Device bus address [dec]\n"
			"ep_num: Endpoint number [hex]\n"
			"len:    Buffer length [dec]"
		),
		cmd_bulk, 4, 0),
	SHELL_SUBCMD_SET_END
);

SHELL_STATIC_SUBCMD_SET_CREATE(bus_cmds,
	SHELL_CMD_ARG(suspend, NULL, "[none]",
		      cmd_bus_suspend, 1, 0),
	SHELL_CMD_ARG(resume, NULL, "[none]",
		      cmd_bus_resume, 1, 0),
	SHELL_CMD_ARG(reset, NULL, "[none]",
		      cmd_bus_reset, 1, 0),
	SHELL_SUBCMD_SET_END
);

SHELL_STATIC_SUBCMD_SET_CREATE(sub_usbh_cmds,
	SHELL_CMD_ARG(init, NULL, "[none]",
		      cmd_usbh_init, 1, 0),
	SHELL_CMD_ARG(enable, NULL, "[none]",
		      cmd_usbh_enable, 1, 0),
	SHELL_CMD_ARG(disable, NULL, "[none]",
		      cmd_usbh_disable, 1, 0),
	SHELL_CMD_ARG(bus, &bus_cmds, "Bus commands",
		      NULL, 1, 0),
	SHELL_CMD_ARG(device, &device_cmds, "Device commands",
		      NULL, 1, 0),
	SHELL_CMD_ARG(select, &hsub_context_name,
		      SHELL_HELP("Selects context used by the shell",
				"<USB host context name>"),
		      cmd_select, 2, 0),
	SHELL_SUBCMD_SET_END
);

SHELL_CMD_REGISTER(usbh, &sub_usbh_cmds, "USBH commands", NULL);
