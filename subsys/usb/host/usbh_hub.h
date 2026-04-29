/*
 * SPDX-FileCopyrightText: Copyright 2025 - 2026 NXP
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _USBH_HUB_H_
#define _USBH_HUB_H_

#include <zephyr/usb/usb_ch9.h>
#include <zephyr/usb/class/usb_hub.h>

/* Hub state enumeration */
enum usbh_hub_state {
	HUB_STATE_INIT,
	HUB_STATE_GET_DESCRIPTOR,
	HUB_STATE_POWER_PORTS,
	HUB_STATE_OPERATIONAL,
	HUB_STATE_ERROR,
};

/* Port state enumeration */
enum usbh_port_state {
	PORT_STATE_DISCONNECTED,
	PORT_STATE_CONNECTED,
	PORT_STATE_RESETTING,
	PORT_STATE_ENABLED,
	PORT_STATE_SUSPENDED,
	PORT_STATE_ERROR,
};

/* Hub instance structure */
struct usb_hub {
	struct usb_device *udev;
	/* Hub port number */
	uint8_t ports;
	union {
		struct usb_hub_descriptor hub_desc;
		uint8_t hub_desc_buf[USB_HUB_DESC_BUF_SIZE];
	};
	/* Status buffers */
	struct usb_hub_status status;
	struct usb_hub_port_status port_status;
};

void usbh_hub_init_instance(struct usb_hub *const hub_instance,
			    struct usb_device *const udev);

void usbh_hub_cleanup_instance(struct usb_hub *hub_instance);

int usbh_req_desc_hub(struct usb_device *const udev,
		      uint8_t *const buffer,
		      const uint16_t len);

/* Hub features */
int usbh_req_clear_hcfs_c_hub_local_power(struct usb_device *const udev);
int usbh_req_clear_hcfs_c_hub_over_current(struct usb_device *const udev);

/* Port features */
int usbh_req_set_hcfs_penable(struct usb_device *const udev, const uint8_t port);
int usbh_req_clear_hcfs_penable(struct usb_device *const udev, const uint8_t port);
int usbh_req_set_hcfs_psuspend(struct usb_device *const udev, const uint8_t port);
int usbh_req_clear_hcfs_psuspend(struct usb_device *const udev, const uint8_t port);
int usbh_req_set_hcfs_prst(struct usb_device *const udev, const uint8_t port);
int usbh_req_clear_hcfs_prst(struct usb_device *const udev, const uint8_t port);
int usbh_req_set_hcfs_ppwr(struct usb_device *const udev, const uint8_t port);
int usbh_req_clear_hcfs_ppwr(struct usb_device *const udev, const uint8_t port);

/* Port change features */
int usbh_req_clear_hcfs_c_pconnection(struct usb_device *const udev, const uint8_t port);
int usbh_req_clear_hcfs_c_penable(struct usb_device *const udev, const uint8_t port);
int usbh_req_clear_hcfs_c_psuspend(struct usb_device *const udev, const uint8_t port);
int usbh_req_clear_hcfs_c_pover_current(struct usb_device *const udev, const uint8_t port);
int usbh_req_clear_hcfs_c_preset(struct usb_device *const udev, const uint8_t port);
int usbh_req_set_hcfs_ptest(struct usb_device *const udev, const uint8_t port);
int usbh_req_set_hcfs_pindicator(struct usb_device *const udev, const uint8_t port);

int usbh_req_get_port_status(struct usb_device *const udev,
			     const uint8_t port_number, uint16_t *const wPortStatus,
			     uint16_t *const wPortChange);

int usbh_req_get_hub_status(struct usb_device *const udev,
			    uint16_t *const wHubStatus,
			    uint16_t *const wHubChange);

#endif /* _USBH_HUB_H_ */
