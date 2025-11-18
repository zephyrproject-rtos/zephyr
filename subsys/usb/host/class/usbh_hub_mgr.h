/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _USBH_HUB_MGR_H_
#define _USBH_HUB_MGR_H_

#include <zephyr/sys/slist.h>
#include "usbh_hub.h"

enum usbh_hub_app_status {
	kHubRunIdle = 0,
	kHubRunInvalid,
	kHubRunWaitSetInterface,
	kHubRunGetDescriptor7,
	kHubRunGetDescriptor,
	kHubRunSetPortPower,
	kHubRunGetStatusDone,
	kHubRunClearDone,
};

enum usbh_port_app_status {
	kPortRunIdle = 0,
	kPortRunInvalid,
	kPortRunWaitPortChange,
	kPortRunCheckCPortConnection,
	kPortRunGetPortConnection,
	kPortRunCheckPortConnection,
	kPortRunWaitPortResetDone,
	kPortRunWaitCPortReset,
	kPortRunCheckCPortReset,
	kPortRunResetAgain,
	kPortRunPortAttached,
	kPortRunCheckPortDetach,
	kPortRunCheckChildHub,
	kPortRunGetConnectionBit,
	kPortRunCheckConnectionBit,
};

enum usbh_prime_status {
	kPrimeNone = 0,
	kPrimeHubControl,
	kPrimePortControl, 
	kPrimeInterrupt,
};

/* Port instance structure (unified structure with necessary fields) */
struct usbh_hub_port_instance {
	struct usb_device *udev;		/* Connected USB device */
	uint8_t port_status;			/* Port application status */
	enum usbh_port_state state;		/* Port overall state */
	uint8_t reset_count;			/* Reset retry count */
	uint8_t speed;				/* Device speed */
	uint8_t port_num;			/* Port number */
};

/**
 * @brief HUB management data structure
 */
struct usbh_hub_mgr_data {
	struct usb_device *hub_udev;
	struct usbh_context *uhs_ctx;

	/* Hub instance data */
	struct usbh_hub_instance hub_instance;

	/* Hub state management */
	enum usbh_hub_state state;
	enum usbh_hub_app_status hub_status;

	/* Port management - unified using port_list */
	struct usbh_hub_port_instance *port_list;	/* Port instance list */
	uint8_t num_ports;				/* Total number of ports */
	uint8_t current_port;				/* Currently processing port */
	uint8_t port_index;				/* Port index */

	/* Work items for state machine processing */
	struct k_work_delayable hub_work;
	struct k_work_delayable port_work;

	/** Interrupt endpoint descriptor */
	struct usb_ep_descriptor *int_ep;
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

	/* Transfer status */
	struct uhc_transfer *control_transfer;		/* Current control transfer handle */
	struct uhc_transfer *interrupt_transfer;	/* Interrupt transfer handle */
	uint8_t int_buffer[8];
};

/* Hub manager API function declarations */

/**
 * @brief Find corresponding Hub management data by USB device
 */
struct usbh_hub_mgr_data *usbh_hub_mgr_get_hub_by_device(struct usb_device *udev);

/**
 * @brief Get high-speed Hub address (for split transactions)
 */
uint8_t usbh_hub_mgr_get_hs_hub_addr(struct usb_device *udev);

/**
 * @brief Get high-speed Hub port (for split transactions)
 */
uint8_t usbh_hub_mgr_get_hs_hub_port(struct usb_device *udev);

/**
 * @brief Print Hub topology structure (for debugging)
 */
void usbh_hub_mgr_print_topology(void);

/**
 * @brief Get Hub statistics information
 */
void usbh_hub_mgr_get_stats(struct usbh_hub_stats *stats);

/**
 * @brief Check if Hub chain depth exceeds limit
 */
static inline bool usbh_hub_mgr_check_depth_limit(uint8_t hub_level)
{
	return hub_level < CONFIG_USBH_HUB_MAX_LEVELS;
}

/**
 * @brief Get Hub level string (for logging)
 */
static inline const char *usbh_hub_mgr_get_level_str(uint8_t hub_level)
{
	switch (hub_level) {
	case 0: return "ROOT";
	case 1: return "L1";
	case 2: return "L2";
	case 3: return "L3";
	case 4: return "L4";
	default: return "Lx";
	}
}

#endif /* _USBH_HUB_MGR_H_ */
