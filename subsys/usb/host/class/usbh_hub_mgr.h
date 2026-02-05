/*
 * Copyright 2025 - 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _USBH_HUB_MGR_H_
#define _USBH_HUB_MGR_H_

#include <zephyr/sys/slist.h>
#include "usbh_hub.h"

enum usbh_hub_app_status {
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

enum usbh_hub_prime_status {
	HUB_PRIME_NONE = 0,
	HUB_PRIME_CONTROL,
	HUB_PRIME_PORT_CONTROL,
	HUB_PRIME_INTERRUPT,
};

/* Port instance structure (unified structure with necessary fields) */
struct usbh_hub_port_instance {
	struct usb_device *udev;    /* Connected USB device */
	uint8_t port_status;        /* Port application status */
	enum usbh_port_state state; /* Port overall state */
	uint8_t reset_count;        /* Reset retry count */
	uint8_t speed;              /* Device speed */
	uint8_t port_num;           /* Port number */
};

/* Hub management data structure */
struct usbh_hub_mgr_data {
	struct usb_device *hub_udev;
	struct usbh_context *uhs_ctx;

	/* Hub instance data */
	struct usbh_hub_instance hub_instance;

	/* Hub state management */
	enum usbh_hub_state state;
	enum usbh_hub_app_status hub_status;

	/* Port management - unified using port_list */
	struct usbh_hub_port_instance *port_list; /* Port instance list */
	uint8_t num_ports;                        /* Total number of ports */
	uint8_t current_port;                     /* Currently processing port */
	uint8_t port_index;                       /* Port index */

	/* Work items for state machine processing */
	struct k_work_delayable hub_work;
	struct k_work_delayable port_work;

	/** Interrupt endpoint descriptor */
	const struct usb_ep_descriptor *int_ep;
	/** Device connection status */
	bool connected;
	/** Interrupt transfer active flag */
	bool int_active;

	/* Hierarchy structure management fields */
	struct usbh_hub_mgr_data *parent_hub;
	uint8_t parent_port;
	sys_slist_t child_hubs;
	sys_snode_t child_node;
	uint8_t hub_level;

	/* Global Hub chain management list node */
	sys_snode_t node;

	/* Synchronization and state management */
	struct k_mutex lock;
	bool being_removed;
	uint32_t last_error_time;
	uint8_t error_count;
	uint8_t prime_status;

	struct uhc_transfer *interrupt_transfer;
	uint8_t int_buffer[8];
};

#endif /* _USBH_HUB_MGR_H_ */
