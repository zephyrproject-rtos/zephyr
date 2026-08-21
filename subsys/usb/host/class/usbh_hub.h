/*
 * Copyright 2025 - 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _USBH_HUB_H_
#define _USBH_HUB_H_

#include <zephyr/sys/slist.h>
#include "usbh_ch11.h"

#define USBH_HUB_INT_BUFFER_SIZE 8

enum usbh_hub_state {
	HUB_STATE_INIT,
	HUB_STATE_OPERATIONAL,
	HUB_STATE_ERROR,
};

struct usb_hub_port {
	struct usb_device *udev;
	struct usbh_hub_data *hub;
	struct k_work_delayable port_work;
	enum usbh_port_state state;
	uint8_t reset_count;
	uint8_t num;
};

struct usbh_hub_data {
	sys_snode_t node;
	sys_snode_t child_node;
	sys_slist_t child_hubs;
	struct usbh_context *uhs_ctx;
	struct usb_device *udev;
	struct usb_hub_port port_list[7];
	const struct usb_ep_descriptor *int_ep;
	struct uhc_transfer *interrupt_transfer;
	struct k_work hub_work;
	struct k_mutex lock;
	union {
		struct usb_hub_descriptor hub_desc;
		uint8_t hub_desc_buf[USB_HUB_DESC_BUF_SIZE];
	};
	struct usb_hub_status status;
	uint8_t int_buffer[USBH_HUB_INT_BUFFER_SIZE];
	enum usbh_hub_state state;
	uint8_t pending_ports;
	uint8_t port_count;
	bool connected;
};

#endif /* _USBH_HUB_H_ */
