/*
 * Copyright 2025 - 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _USBH_HUB_MGR_H_
#define _USBH_HUB_MGR_H_

#include <zephyr/sys/slist.h>
#include "usbh_hub.h"

enum usbh_hub_run_status {
	HUB_RUN_IDLE = 0,
	HUB_RUN_INVALID,
	HUB_RUN_WAIT_SET_INTERFACE,
	HUB_RUN_GET_DESCRIPTOR_7,
	HUB_RUN_SET_PORT_POWER,
	HUB_RUN_CLEAR_DONE,
};

enum usbh_hub_port_app_status {
	HUB_PORT_RUN_IDLE = 0,
	HUB_PORT_RUN_INVALID,
	HUB_PORT_RUN_WAIT_PORT_CHANGE,
	HUB_PORT_RUN_CHECK_C_PORT_CONNECTION,
	HUB_PORT_RUN_GET_PORT_CONNECTION,
	HUB_PORT_RUN_CHECK_PORT_CONNECTION,
	HUB_PORT_RUN_WAIT_PORT_RESET_DONE,
	HUB_PORT_RUN_WAIT_C_PORT_RESET,
	HUB_PORT_RUN_CHECK_C_PORT_RESET,
	HUB_PORT_RUN_RESET_AGAIN,
	HUB_PORT_RUN_PORT_ATTACHED,
	HUB_PORT_RUN_CHECK_PORT_DETACH,
	HUB_PORT_RUN_CHECK_CHILD_HUB,
};

/* Hub port structure */
struct usb_hub_port {
	struct usb_device *udev;
	uint8_t status;             /* Port status */
	enum usbh_port_state state; /* Port overall state */
	uint8_t reset_count;        /* Reset retry count */
	uint8_t speed;              /* Device speed */
	uint8_t num;                /* Port number */
};

/* Hub management data structure */
struct usbh_hub_mgr_data {
	struct usbh_context *uhs_ctx;

	struct usb_hub hub_instance;

	enum usbh_hub_state state;
	enum usbh_hub_run_status hub_status;

	struct usb_hub_port *port_list;
	uint8_t current_port;
	uint8_t port_index;

	struct k_work_delayable hub_work;
	struct k_work_delayable port_work;

	const struct usb_ep_descriptor *int_ep;
	struct uhc_transfer *interrupt_transfer;
	uint8_t int_buffer[8];
	bool int_active;

	sys_slist_t child_hubs;
	sys_snode_t child_node;
	sys_snode_t node;

	struct k_mutex lock;
	bool connected;
	bool being_removed;
};

#endif /* _USBH_HUB_MGR_H_ */
