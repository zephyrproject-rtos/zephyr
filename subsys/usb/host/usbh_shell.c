/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 * Copyright (c) 2026 Renesas Electronics Corporation
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
#if defined(CONFIG_USBH_DFU_CLASS)
#include <zephyr/usb/class/usbh_dfu.h>
#endif

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

static int cmd_device_list(const struct shell *sh,
			   size_t argc, char **argv)
{
	struct usbh_context *uhs_ctx;
	struct usb_device *udev;

	uhs_ctx = get_uhs_ctx_or_error(sh);
	if (uhs_ctx == NULL) {
		return -ENODEV;
	}

	SYS_DLIST_FOR_EACH_CONTAINER(&uhs_ctx->udevs, udev, node) {
		shell_print(sh, "%u", udev->addr);
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
	while (dhp != NULL) {
		/* Print every entry */
		print_desc(sh, dhp);
		dhp = usbh_desc_get_next(dhp);
	}

	return 0;
}

#if defined(CONFIG_USBH_DFU_CLASS)

#ifndef MAX_PATH_LEN
#define MAX_PATH_LEN 64
#endif

#include <zephyr/sys/crc.h>
#include <zephyr/fs/fs.h>
#include <zephyr/fs/littlefs.h>
#include <zephyr/storage/flash_map.h>

#if DT_NODE_EXISTS(PARTITION_NODE)
FS_FSTAB_DECLARE_ENTRY(PARTITION_NODE);
#else  /* PARTITION_NODE */
FS_LITTLEFS_DECLARE_DEFAULT_CONFIG(dfufw_storage);
static struct fs_mount_t lfs_storage_mnt = {
	.type = FS_LITTLEFS,
	.fs_data = &dfufw_storage,
	.storage_dev = (void *)PARTITION_ID(dfufw_partition),
	.mnt_point = "/lfs",
	.flags = FS_MOUNT_FLAG_READ_ONLY,
};
#endif /* PARTITION_NODE */

struct fs_mount_t *mountpoint =
#if DT_NODE_EXISTS(PARTITION_NODE)
	&FS_FSTAB_ENTRY(PARTITION_NODE)
#else
	&lfs_storage_mnt
#endif
	;

static int littlefs_mount(struct fs_mount_t *mp, const struct shell *sh)
{
	int rc;

	/* Do not mount if auto-mount has been enabled */
#if (!DT_NODE_EXISTS(PARTITION_NODE) ||                                                            \
	(!(FSTAB_ENTRY_DT_MOUNT_FLAGS(PARTITION_NODE) & FS_MOUNT_FLAG_AUTOMOUNT)))
	rc = fs_mount(mp);
	if (rc < 0) {
		shell_error(sh, "FAIL: mount id %" PRIuPTR " at %s: %d\n",
			    (uintptr_t)mp->storage_dev, mp->mnt_point, rc);
		return rc;
	}
	shell_print(sh, "%s mount: %d\n", mp->mnt_point, rc);
#else
	shell_print(sh, "%s automounted\n", mp->mnt_point);
#endif

	return 0;
}

int cmd_dfu_upload_cb(void *upload_arg, char *data, const size_t len)
{
	const struct shell *sh = (const struct shell *)upload_arg;

	if (len) {
		shell_print(sh, "Upload chunk:");
		shell_hexdump(sh, data, len);
	} else {
		shell_print(sh, "Upload is done");
	}

	return 0;
}

static int cmd_dfu_upload(const struct shell *sh, size_t argc, char **argv)
{
	struct usbh_dfu_settings dfu_settings = {0};
	static const struct device *dev;
	uint8_t alternate_idx;
	int err;

	dev = DEVICE_DT_GET(DT_NODELABEL(any_dfu_device));

	alternate_idx = strtol(argv[1], NULL, 10);
	dfu_settings.alternate_idx = alternate_idx;

	err = usbh_dfu_settings(dev, &dfu_settings);
	if (err) {
		shell_error(sh, "DFU settings error %d", err);
		return err;
	}

	err = usbh_dfu_upload(dev, cmd_dfu_upload_cb, (void *)sh);
	if (err) {
		shell_error(sh, "DFU UPLOAD error %d", err);
		return err;
	}

	return err;
}

struct shell_dfu_dnload_ctx {
	const struct shell *sh;
	size_t block_nr;
	size_t cursor;
	size_t track_block;
	size_t track_len;
	struct fs_file_t *file;
	size_t fw_len;
};

struct dfu_suffix {
	uint16_t bcdDevice;
	uint16_t idProduct;
	uint16_t idVendor;
	uint16_t bcdDFU;
	uint8_t ucDfuSignature[3];
	uint8_t bLength;
	uint32_t dwCRC;
} __packed;

int cmd_dfu_dnload_cb(void *upload_arg, char *data, const size_t data_len)
{
	struct shell_dfu_dnload_ctx *sdd_ctx = (struct shell_dfu_dnload_ctx *)upload_arg;
	int read_len = data_len;

	/* No more FW data to send */
	if (sdd_ctx->track_len >= sdd_ctx->fw_len) {
		return 0;
	}

	read_len = MIN(read_len, sdd_ctx->fw_len - sdd_ctx->track_len);

	/* Pass USB data to fs_read function */
	read_len = fs_read(sdd_ctx->file, data, read_len);
	if (unlikely(read_len < 0)) {
		shell_print(sdd_ctx->sh, "Error while reading file, block: %d, total bytes: %d",
			    sdd_ctx->track_block, sdd_ctx->track_len);
	} else if (read_len == 0) {
		shell_print(sdd_ctx->sh, "File read completed, blocks: %d, total bytes: %d",
			    sdd_ctx->track_block, sdd_ctx->track_len);
	} else {
		sdd_ctx->track_len += read_len;
		shell_print(sdd_ctx->sh, "Downloading block: %d, total bytes: %d",
			    sdd_ctx->track_block, sdd_ctx->track_len);
		shell_hexdump(sdd_ctx->sh, data, read_len);
		sdd_ctx->track_block++;
	}

	/* Return number of copied data */
	return read_len;
}

static int cmd_dfu_file_len(struct fs_file_t *file)
{
	int err = fs_seek(file, 0, FS_SEEK_END);

	if (err < 0) {
		return err;
	}

	return fs_tell(file);
}

static bool cmd_dfu_has_valid_suffix(struct dfu_suffix *suffix, struct fs_file_t *file,
				     size_t file_len, const struct shell *sh,
				     const struct device *dev)
{
	uint32_t crc_hash = 0;
	uint8_t crc_buf[16];
	int err, read_len, remaining_bytes;

	/* Not even a suffix */
	if (file_len < 16) {
		shell_error(sh, "File is too small to contain DFU suffix");
		return false;
	}

	/* Seek to suffix */
	err = fs_seek(file, file_len - sizeof(struct dfu_suffix), FS_SEEK_SET);
	if (err < 0) {
		shell_error(sh, "Cannot seek to DFU suffix");
		return false;
	}

	/* Load suffix */
	err = fs_read(file, suffix, (sizeof(struct dfu_suffix)));
	if (err < 0) {
		shell_error(sh, "Cannot load DFU suffix");
		return false;
	}

	/* Basic suffix check */
	if (suffix->ucDfuSignature[0] != 'U' || suffix->ucDfuSignature[1] != 'F' ||
	    suffix->ucDfuSignature[2] != 'D' ||
	    (suffix->bLength < 16 || suffix->bLength > file_len)) {
		shell_error(sh, "DFU signature does not match");
		return false;
	}

	err = usbh_dfu_match_vid_pid(dev, suffix->idVendor, suffix->idProduct);
	if (err < 0) {
		shell_error(sh, "match_vid_pid call failed");
		return false;
	} else if (err == 0) {
		shell_error(sh, "idVendor %x or idProduct %x does not match", suffix->idVendor,
			    suffix->idProduct);
		return false;
	}

	/* Calculate CRC */
	err = fs_seek(file, 0, FS_SEEK_SET);
	remaining_bytes = file_len - 4;
	while (remaining_bytes > 0) {
		read_len = MIN(sizeof(crc_buf), remaining_bytes);
		err = fs_read(file, crc_buf, read_len);
		if (err < 0) {
			shell_error(sh, "Cannot read the FW file");
			return false;
		}
		crc_hash = crc32_ieee_update(crc_hash, crc_buf, read_len);
		remaining_bytes -= read_len;
	}
	crc_hash ^= 0xFFFFFFFF;

	if (crc_hash != suffix->dwCRC) {
		shell_error(sh, "DFU Suffix CRC does not match");
		return false;
	}

	return true;
}

static bool cmd_dfu_filename_has_dfu_extension(char *filename)
{
	size_t len = strlen(filename);

	if (len < 4) {
		return false;
	}

	return ((filename[len - 4] == '.') &&
		(filename[len - 3] == 'd' || filename[len - 3] == 'D') &&
		(filename[len - 2] == 'f' || filename[len - 2] == 'F') &&
		(filename[len - 1] == 'u' || filename[len - 1] == 'U'));
}

static int cmd_dfu_dnload(const struct shell *sh, size_t argc, char **argv)
{
	char fw_filename[MAX_PATH_LEN];
	struct shell_dfu_dnload_ctx sdd = {0};
	struct usbh_dfu_settings dfu_settings = {0};
	static const struct device *dev;
	uint8_t alternate_idx;
	struct fs_statvfs sbuf;
	struct fs_file_t file;
	struct dfu_suffix suffix = {0};
	int err, file_len;

	dev = DEVICE_DT_GET(DT_NODELABEL(any_dfu_device));

	err = littlefs_mount(mountpoint, sh);
	if (err < 0) {
		shell_error(sh, "Cannot find mountpoint");
		return err;
	}

	snprintf(fw_filename, sizeof(fw_filename), "%s/%s", mountpoint->mnt_point, argv[2]);

	err = fs_statvfs(mountpoint->mnt_point, &sbuf);
	if (err != 0) {
		shell_error(sh, "Cannot not access filesystem");
		goto err_umount;
	}

	fs_file_t_init(&file);
	err = fs_open(&file, fw_filename, FS_O_READ);
	if (err != 0) {
		shell_error(sh, "Cannot not find file: %s", fw_filename);
		goto err_umount;
	}

	file_len = cmd_dfu_file_len(&file);
	if (file_len < 0) {
		shell_error(sh, "Invalid file size %s : %d", fw_filename, file_len);
		goto err_umount;
	}

	/* Validate dfu suffix only for files with .dfu extension */
	if (cmd_dfu_filename_has_dfu_extension(fw_filename)) {
		if (cmd_dfu_has_valid_suffix(&suffix, &file, file_len, sh, dev)) {
			shell_print(sh, "Filename %s has valid DFU suffix", fw_filename);
			file_len = (file_len - suffix.bLength);
		} else {
			shell_print(sh, "Filename %s has invalid DFU suffix, aborting",
				    fw_filename);
			goto err_umount;
		}
	} else {
		shell_print(sh, "Ordinary file %s, uploading as it is", fw_filename);
	}

	err = fs_seek(&file, 0, FS_SEEK_SET);

	alternate_idx = strtol(argv[1], NULL, 10);
	dfu_settings.alternate_idx = alternate_idx;
	/* The Zephyr DFU device incorrectly reports the 'dfuMANIFEST-SYNC' state
	 * in GetStatus after the ZeroPacket download. According to the specification,
	 * it should report the 'dfuMANIFEST' state.
	 */
	dfu_settings.quirks = USBH_DFU_QUIRK_IGNORE_DNLOAD_COMPLETE_CHECK;

	/* Prepare download callback context */
	sdd.sh = sh;
	sdd.track_block = 0;
	sdd.track_len = 0;
	sdd.file = &file;
	sdd.fw_len = file_len;

	err = usbh_dfu_settings(dev, &dfu_settings);
	if (err) {
		shell_error(sh, "DFU settings error %d", err);
		goto err_close_file;
	}

	err = usbh_dfu_dnload(dev, cmd_dfu_dnload_cb, (void *)&sdd);
	if (err) {
		shell_error(sh, "DFU DNLOAD error %d", err);
		goto err_close_file;
	}

err_close_file:
	(void)fs_close(&file);

err_umount:
	(void)fs_unmount(mountpoint);

	return err;
}

static int cmd_dfurt_enter_dfu(const struct shell *sh, size_t argc, char **argv)
{
	struct usbh_dfu_settings dfu_settings = {0};
	static const struct device *dev;
	int err;

	dev = DEVICE_DT_GET(DT_NODELABEL(any_dfurt_device));

	err = usbh_dfurt_settings(dev, &dfu_settings);
	if (err) {
		shell_error(sh, "DFURT settings error %d", err);
		return err;
	}

	err = usbh_dfurt_enter_dfu(dev);
	if (err) {
		shell_error(sh, "DFURT enter dfu error %d", err);
		return err;
	}

	return err;
}
#endif /* defined(USBH_DFU_CLASS) */

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
#if defined(CONFIG_USBH_DFU_CLASS)
	SHELL_CMD_ARG(dfu_upload, NULL,
		      SHELL_HELP("Upload firmware from Device to Host",
				 "<alt>\n"
				 "alt: Alternate setting number, usually 0 [dec]"),
		      cmd_dfu_upload, 2, 0),
	SHELL_CMD_ARG(dfu_dnload, NULL,
		      SHELL_HELP("Download firmware from Host to Device",
				 "<alt> <text>\n"
				 "alt: Alternate setting number, usually 0 [dec]\n"
				 "filename: Filename of FW file [str]"),
		      cmd_dfu_dnload, 3, 0),
	SHELL_CMD_ARG(dfurt_enter_dfu, NULL,
		      SHELL_HELP("Switch DFU-realtime device to DFU mode", ""), cmd_dfurt_enter_dfu,
		      1, 0),
#endif /* defined(CONFIG_USBH_DFU_CLASS) */
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
