/*
 * SPDX-FileCopyrightText: Copyright 2025 - 2026 NXP
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_USBH_CH11_H
#define ZEPHYR_INCLUDE_USBH_CH11_H

#include <zephyr/usb/usb_ch9.h>
#include <zephyr/usb/class/usb_hub.h>

/* Port state enumeration */
enum usbh_port_state {
	PORT_STATE_NOT_CONFIGURED,
	PORT_STATE_POWERED_OFF,
	PORT_STATE_DISCONNECTED,
	PORT_STATE_DISABLED,
	PORT_STATE_RESETTING,
	PORT_STATE_ENABLED,
	PORT_STATE_TRANSMIT,
	PORT_STATE_TRANSMIT_R,
	PORT_STATE_SUSPENDED,
	PORT_STATE_RESUMING,
	PORT_STATE_SEND_EOR,
	PORT_STATE_RESTART_S,
	PORT_STATE_RESTART_E,
	PORT_STATE_TESTING,
};

int usbh_req_desc_hub(struct usb_device *const udev,
		      uint8_t *const buffer,
		      const uint16_t len);

/* Hub features */
int usbh_req_clear_hcfs_c_hub_local_power(struct usb_device *const udev);
int usbh_req_clear_hcfs_c_hub_over_current(struct usb_device *const udev);

/* Port features */
int usbh_req_set_hcfs_port_enable(struct usb_device *const udev, const uint8_t port);
int usbh_req_clear_hcfs_port_enable(struct usb_device *const udev, const uint8_t port);
int usbh_req_set_hcfs_port_suspend(struct usb_device *const udev, const uint8_t port);
int usbh_req_clear_hcfs_port_suspend(struct usb_device *const udev, const uint8_t port);
int usbh_req_set_hcfs_port_reset(struct usb_device *const udev, const uint8_t port);
int usbh_req_clear_hcfs_port_reset(struct usb_device *const udev, const uint8_t port);
int usbh_req_set_hcfs_port_power(struct usb_device *const udev, const uint8_t port);
int usbh_req_clear_hcfs_port_power(struct usb_device *const udev, const uint8_t port);

/* Port change features */
int usbh_req_clear_hcfs_c_port_connection(struct usb_device *const udev, const uint8_t port);
int usbh_req_clear_hcfs_c_port_enable(struct usb_device *const udev, const uint8_t port);
int usbh_req_clear_hcfs_c_port_suspend(struct usb_device *const udev, const uint8_t port);
int usbh_req_clear_hcfs_c_port_over_current(struct usb_device *const udev, const uint8_t port);
int usbh_req_clear_hcfs_c_port_reset(struct usb_device *const udev, const uint8_t port);
int usbh_req_set_hcfs_port_test(struct usb_device *const udev, const uint8_t port);
int usbh_req_set_hcfs_port_indicator(struct usb_device *const udev, const uint8_t port);

int usbh_req_get_port_status(struct usb_device *const udev,
			     const uint8_t port_number, uint16_t *const wPortStatus,
			     uint16_t *const wPortChange);

int usbh_req_get_hub_status(struct usb_device *const udev,
			    uint16_t *const wHubStatus,
			    uint16_t *const wHubChange);

#endif /* ZEPHYR_INCLUDE_USBH_CH11_H */
