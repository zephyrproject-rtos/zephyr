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

#include "usbh_device.h"
#include "usbh_ch9.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(usbh_shell, CONFIG_USBH_LOG_LEVEL);

#define FOOBAZ_VREQ_OUT		0x5b
#define FOOBAZ_VREQ_IN		0x5c

USBH_CONTROLLER_DEFINE(uhs_ctx, DEVICE_DT_GET(DT_NODELABEL(zephyr_uhc0)));
static struct usb_device *udev;
static uint8_t vreq_test_buf[1024];

static void print_dev_desc(const struct shell *sh,
			   const struct usb_device_descriptor *const desc)
{
	shell_print(sh, "bLength\t\t\t%u", desc->bLength);
	shell_print(sh, "bDescriptorType\t\t%u", desc->bDescriptorType);
	shell_print(sh, "bcdUSB\t\t\t%x", desc->bcdUSB);
	shell_print(sh, "bDeviceClass\t\t%u", desc->bDeviceClass);
	shell_print(sh, "bDeviceSubClass\t\t%u", desc->bDeviceSubClass);
	shell_print(sh, "bDeviceProtocol\t\t%u", desc->bDeviceProtocol);
	shell_print(sh, "bMaxPacketSize0\t\t%u", desc->bMaxPacketSize0);
	shell_print(sh, "idVendor\t\t%x", desc->idVendor);
	shell_print(sh, "idProduct\t\t%x", desc->idProduct);
	shell_print(sh, "bcdDevice\t\t%x", desc->bcdDevice);
	shell_print(sh, "iManufacturer\t\t%u", desc->iManufacturer);
	shell_print(sh, "iProduct\t\t%u", desc->iProduct);
	shell_print(sh, "iSerial\t\t\t%u", desc->iSerialNumber);
	shell_print(sh, "bNumConfigurations\t%u", desc->bNumConfigurations);
}

static void print_cfg_desc(const struct shell *sh,
			   const struct usb_cfg_descriptor *const desc)
{
	shell_print(sh, "bLength\t\t\t%u", desc->bLength);
	shell_print(sh, "bDescriptorType\t\t%u", desc->bDescriptorType);
	shell_print(sh, "wTotalLength\t\t%x", desc->wTotalLength);
	shell_print(sh, "bNumInterfaces\t\t%u", desc->bNumInterfaces);
	shell_print(sh, "bConfigurationValue\t%u", desc->bConfigurationValue);
	shell_print(sh, "iConfiguration\t\t%u", desc->iConfiguration);
	shell_print(sh, "bmAttributes\t\t%02x", desc->bmAttributes);
	shell_print(sh, "bMaxPower\t\t%u mA", desc->bMaxPower * 2);
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
	struct uhc_transfer *xfer;
	struct net_buf *buf;
	uint8_t ep;
	size_t len;
	int ret;

	ep = strtol(argv[1], NULL, 16);
	len = MIN(sizeof(vreq_test_buf), strtol(argv[2], NULL, 10));

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
	const uint16_t wValue = 0x0000;
	struct net_buf *buf;
	uint16_t wLength;
	int ret;

	wLength = MIN(sizeof(vreq_test_buf), strtol(argv[1], NULL, 10));
	buf = usbh_xfer_buf_alloc(udev, wLength);
	if (!buf) {
		shell_print(sh, "host: Failed to allocate buffer");
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
	const uint16_t wValue = 0x0000;
	struct net_buf *buf;
	uint16_t wLength;
	int ret;

	wLength = MIN(sizeof(vreq_test_buf), strtol(argv[1], NULL, 10));
	buf = usbh_xfer_buf_alloc(udev, wLength);
	if (!buf) {
		shell_print(sh, "host: Failed to allocate buffer");
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
	int err;

	err = usbh_req_desc_dev(udev, sizeof(desc), &desc);
	if (err) {
		shell_print(sh, "host: Failed to request device descriptor");
	} else {
		print_dev_desc(sh, &desc);
	}

	return err;
}

static int cmd_desc_config(const struct shell *sh,
			   size_t argc, char **argv)
{
	struct usb_cfg_descriptor desc;
	uint8_t cfg;
	int err;

	cfg = strtol(argv[1], NULL, 10);

	err = usbh_req_desc_cfg(udev, cfg, sizeof(desc), &desc);
	if (err) {
		shell_print(sh, "host: Failed to request configuration descriptor");
	} else {
		print_cfg_desc(sh, &desc);
	}

	return err;
}

static int cmd_desc_string(const struct shell *sh,
			   size_t argc, char **argv)
{
	const uint8_t type = USB_DESC_STRING;
	struct net_buf *buf;
	uint8_t id;
	uint8_t idx;
	int err;

	id = strtol(argv[1], NULL, 10);
	idx = strtol(argv[2], NULL, 10);

	buf = usbh_xfer_buf_alloc(udev, 128);
	if (!buf) {
		return -ENOMEM;
	}

	err = usbh_req_desc(udev, type, idx, id, 128, buf);
	if (err) {
		shell_print(sh, "host: Failed to request configuration descriptor");
	} else {
		shell_hexdump(sh, buf->data, buf->len);
	}

	usbh_xfer_buf_free(udev, buf);

	return err;
}

static int cmd_feature_set_halt(const struct shell *sh,
				size_t argc, char **argv)
{
	uint8_t ep;
	int err;

	ep = strtol(argv[1], NULL, 16);

	/* TODO: add usbh_req_set_sfs_halt(&uhs_ctx, NULL, 0); */
	err = usbh_req_set_sfs_rwup(udev);
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
	int err;

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
	int err;

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
	uint8_t port;
	int err;

	port = strtol(argv[1], NULL, 10);

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
	uint8_t port;
	int err;

	port = strtol(argv[1], NULL, 10);

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
	uint8_t cfg;
	int err;

	cfg = strtol(argv[1], NULL, 10);

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
	uint8_t cfg;
	int err;

	err = usbh_req_get_cfg(udev, &cfg);
	if (err) {
		shell_error(sh, "host: Failed to set configuration");
	} else {
		shell_print(sh, "host: Device 0x%02x, current configuration %u",
			    udev->addr, cfg);
	}

	return err;
}

static int cmd_device_interface(const struct shell *sh,
				size_t argc, char **argv)
{
	uint8_t iface;
	uint8_t alt;
	int err;

	iface = strtol(argv[1], NULL, 10);
	alt = strtol(argv[2], NULL, 10);

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
	uint8_t addr;
	int err;

	addr = strtol(argv[1], NULL, 10);

	err = usbh_req_set_address(udev, addr);
	if (err) {
		shell_error(sh, "host: Failed to set address");
	} else {
		shell_print(sh, "host: New device address is 0x%02x", addr);
	}

	return err;
}

static int cmd_bus_suspend(const struct shell *sh,
			   size_t argc, char **argv)
{
	int err;

	err = uhc_bus_suspend(uhs_ctx.dev);
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
	int err;

	err = uhc_bus_resume(uhs_ctx.dev);
	if (err) {
		shell_error(sh, "host: Failed to perform bus resume %d", err);
	} else {
		shell_print(sh, "host: USB bus resumed");
	}

	err = uhc_sof_enable(uhs_ctx.dev);
	if (err) {
		shell_error(sh, "host: Failed to start SoF generator %d", err);
	}

	return err;
}

static int cmd_bus_reset(const struct shell *sh,
			 size_t argc, char **argv)
{
	int err;

	err = uhc_bus_reset(uhs_ctx.dev);
	if (err) {
		shell_error(sh, "host: Failed to perform bus reset %d", err);
	} else {
		shell_print(sh, "host: USB bus reset");
	}

	err = uhc_sof_enable(uhs_ctx.dev);
	if (err) {
		shell_error(sh, "host: Failed to start SoF generator %d", err);
	}

	return err;
}

static int cmd_usbh_init(const struct shell *sh,
			 size_t argc, char **argv)
{
	int err;

	udev = usbh_device_get_any(&uhs_ctx);

	err = usbh_init(&uhs_ctx);
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
	int err;

	err = usbh_enable(&uhs_ctx);
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
	int err;

	err = usbh_disable(&uhs_ctx);
	if (err) {
		shell_error(sh, "host: Failed to disable USB host support");
	} else {
		shell_print(sh, "host: USB host disabled");
	}

	return err;
}

SHELL_STATIC_SUBCMD_SET_CREATE(desc_cmds,
	SHELL_CMD_ARG(device, NULL, NULL,
		      cmd_desc_device, 1, 0),
	SHELL_CMD_ARG(configuration, NULL, "<index>",
		      cmd_desc_config, 2, 0),
	SHELL_CMD_ARG(string, NULL, "<id> <index>",
		      cmd_desc_string, 3, 0),
	SHELL_SUBCMD_SET_END
);

SHELL_STATIC_SUBCMD_SET_CREATE(feature_set_cmds,
	SHELL_CMD_ARG(rwup, NULL, NULL,
		      cmd_feature_set_rwup, 1, 0),
	SHELL_CMD_ARG(ppwr, NULL, "<port>",
		      cmd_feature_set_ppwr, 2, 0),
	SHELL_CMD_ARG(prst, NULL, "<port>",
		      cmd_feature_set_prst, 2, 0),
	SHELL_CMD_ARG(halt, NULL, "<endpoint>",
		      cmd_feature_set_halt, 2, 0),
	SHELL_SUBCMD_SET_END
);

SHELL_STATIC_SUBCMD_SET_CREATE(feature_clear_cmds,
	SHELL_CMD_ARG(rwup, NULL, NULL,
		      cmd_feature_clear_rwup, 1, 0),
	SHELL_CMD_ARG(halt, NULL, "<endpoint>",
		      cmd_feature_set_halt, 2, 0),
	SHELL_SUBCMD_SET_END
);

SHELL_STATIC_SUBCMD_SET_CREATE(config_cmds,
	SHELL_CMD_ARG(get, NULL, NULL,
		      cmd_config_get, 1, 0),
	SHELL_CMD_ARG(set, NULL, "<configuration>",
		      cmd_config_set, 2, 0),
	SHELL_SUBCMD_SET_END
);

SHELL_STATIC_SUBCMD_SET_CREATE(device_cmds,
	SHELL_CMD_ARG(address, NULL, "<address>",
		      cmd_device_address, 2, 0),
	SHELL_CMD_ARG(config, &config_cmds, "get|set configuration",
		      NULL, 1, 0),
	SHELL_CMD_ARG(interface, NULL, "<interface> <alternate>",
		      cmd_device_interface, 3, 0),
	SHELL_CMD_ARG(descriptor, &desc_cmds, "descriptor request",
		      NULL, 1, 0),
	SHELL_CMD_ARG(feature-set, &feature_set_cmds, "feature selector",
		      NULL, 1, 0),
	SHELL_CMD_ARG(feature-clear, &feature_clear_cmds, "feature selector",
		      NULL, 1, 0),
	SHELL_CMD_ARG(vendor_in, NULL, "<length>",
		      cmd_vendor_in, 2, 0),
	SHELL_CMD_ARG(vendor_out, NULL, "<length>",
		      cmd_vendor_out, 2, 0),
	SHELL_CMD_ARG(bulk, NULL, "<endpoint> <length>",
		      cmd_bulk, 3, 0),
	SHELL_SUBCMD_SET_END
);

SHELL_STATIC_SUBCMD_SET_CREATE(bus_cmds,
	SHELL_CMD_ARG(suspend, NULL, "[nono]",
		      cmd_bus_suspend, 1, 0),
	SHELL_CMD_ARG(resume, NULL, "[nono]",
		      cmd_bus_resume, 1, 0),
	SHELL_CMD_ARG(reset, NULL, "[nono]",
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
	SHELL_CMD_ARG(bus, &bus_cmds, "bus commands",
		      NULL, 1, 0),
	SHELL_CMD_ARG(device, &device_cmds, "device commands",
		      NULL, 1, 0),
	SHELL_SUBCMD_SET_END
);

SHELL_CMD_REGISTER(usbh, &sub_usbh_cmds, "USBH commands", NULL);
