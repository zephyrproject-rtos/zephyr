/* composite.c - USB composite device driver relay */

/*
 * Copyright (c) 2017 PHYTEC Messtechnik GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel.h>
#include <init.h>
#include <misc/byteorder.h>
#include <string.h>
#include <usb/usb_common.h>
#include <usb/usb_device.h>
#include <usb/usbstruct.h>
#include "composite.h"
#include "usb_descriptor.h"

#define SYS_LOG_LEVEL CONFIG_SYS_LOG_USB_LEVEL
#include <logging/sys_log.h>

static struct usb_interface_cfg_data function_cfg[NUMOF_IFACES];

usb_status_callback cb_usb_status[NUMOF_IFACES];

static u8_t iface_data_buf[CONFIG_USB_COMPOSITE_BUFFER_SIZE];

static void composite_status_cb(enum usb_dc_status_code status, u8_t *param)
{
	for (u8_t i = 0; i < NUMOF_IFACES; i++) {
		if (cb_usb_status[i]) {
			cb_usb_status[i](status, param);
		}
	}
}

static int composite_class_handler(struct usb_setup_packet *pSetup,
		s32_t *len, u8_t **data)
{
	if (pSetup->wIndex >= NUMOF_IFACES) {
		SYS_LOG_DBG("unknown request 0x%x, value 0x%x",
			pSetup->bRequest, pSetup->wValue);
		return -EINVAL;
	}

	if (function_cfg[pSetup->wIndex].class_handler != NULL) {
		return function_cfg[pSetup->wIndex].class_handler(
			pSetup, len, data);
	}

	return -EINVAL;
}

static int composite_custom_handler(struct usb_setup_packet *pSetup,
		s32_t *len, u8_t **data)
{
	if (pSetup->wIndex >= NUMOF_IFACES) {
		SYS_LOG_DBG("unknown request 0x%x, value 0x%x",
			pSetup->bRequest, pSetup->wValue);
		return -ENOTSUP;
	}

	if (function_cfg[pSetup->wIndex].custom_handler != NULL) {
		return function_cfg[pSetup->wIndex].custom_handler(
			pSetup, len, data);
	}

	return -ENOTSUP;
}

static struct usb_ep_cfg_data ep_data[NUMOF_ENDPOINTS] = {};

static struct usb_cfg_data composite_cfg = {
	.usb_device_description = NULL,
	.cb_usb_status = composite_status_cb,
	.interface = {
		.class_handler = composite_class_handler,
		.custom_handler = composite_custom_handler,
		.payload_data = iface_data_buf,
	},
	.num_endpoints = NUMOF_ENDPOINTS,
	.endpoint = ep_data
};

int composite_add_function(struct usb_cfg_data *cfg_data, u8_t if_num)
{
	int numof_free_ep = 0;
	int ep_idx = 0;

	for (u32_t ep = 0; ep < NUMOF_ENDPOINTS; ep++) {
		if (ep_data[ep].ep_cb == NULL) {
			numof_free_ep++;
		}
	}

	if (numof_free_ep < cfg_data->num_endpoints) {
		SYS_LOG_ERR("Not enough free endpoints (free %d, requested %d)",
			    numof_free_ep, cfg_data->num_endpoints);
		return -ENOMEM;
	}

	for (u32_t ep = 0; ep < NUMOF_ENDPOINTS; ep++) {
		if (ep_data[ep].ep_cb == NULL) {
			ep_data[ep].ep_cb =
				cfg_data->endpoint[ep_idx].ep_cb;
			ep_data[ep].ep_addr =
				cfg_data->endpoint[ep_idx].ep_addr;
			ep_idx++;
		}

		if (ep_idx >= cfg_data->num_endpoints) {
			break;
		}
	}

	if (if_num >= NUMOF_IFACES) {
		SYS_LOG_ERR("Interface number out of range");
		return -ENOMEM;
	}

	cfg_data->interface.payload_data = iface_data_buf;

	memcpy(&function_cfg[if_num], &cfg_data->interface,
	       sizeof(struct usb_interface_cfg_data));
	cb_usb_status[if_num] = cfg_data->cb_usb_status;

	return 0;
}

static int composite_init(struct device *dev)
{
	int ret;

	SYS_LOG_DBG("");

	composite_cfg.usb_device_description = usb_get_device_descriptor();

	ret = usb_set_config(&composite_cfg);
	if (ret < 0) {
		SYS_LOG_ERR("Failed to config USB");
		return ret;
	}

	/* Enable USB driver */
	ret = usb_enable(&composite_cfg);
	if (ret < 0) {
		SYS_LOG_ERR("Failed to enable USB");
		return ret;
	}

	return 0;
}

SYS_INIT(composite_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);
