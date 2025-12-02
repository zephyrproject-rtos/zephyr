/*
 * Copyright 2025 - 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _USBH_HUB_H_
#define _USBH_HUB_H_

#include <zephyr/usb/usb_ch9.h>

/* USB HUB Class-specific requests */
#define USB_HUB_REQ_GET_STATUS     0x00
#define USB_HUB_REQ_CLEAR_FEATURE  0x01
#define USB_HUB_REQ_SET_FEATURE    0x03
#define USB_HUB_REQ_GET_DESCRIPTOR 0x06

/* USB HUB Descriptor Type */
#define USB_HUB_DESCRIPTOR_TYPE 0x29

/* USB HUB Port Features */
#define USB_HUB_FEATURE_PORT_CONNECTION   0x00
#define USB_HUB_FEATURE_PORT_ENABLE       0x01
#define USB_HUB_FEATURE_PORT_SUSPEND      0x02
#define USB_HUB_FEATURE_PORT_OVER_CURRENT 0x03
#define USB_HUB_FEATURE_PORT_RESET        0x04
#define USB_HUB_FEATURE_PORT_POWER        0x08
#define USB_HUB_FEATURE_PORT_LOW_SPEED    0x09
#define USB_HUB_FEATURE_PORT_HIGH_SPEED   0x0A

/* USB Hub Status Change */
#define USB_HUB_FEATURE_C_HUB_LOCAL_POWER  0
#define USB_HUB_FEATURE_C_HUB_OVER_CURRENT 1

/* USB HUB Port Change Features */
#define USB_HUB_FEATURE_C_PORT_CONNECTION   0x10
#define USB_HUB_FEATURE_C_PORT_ENABLE       0x11
#define USB_HUB_FEATURE_C_PORT_SUSPEND      0x12
#define USB_HUB_FEATURE_C_PORT_OVER_CURRENT 0x13
#define USB_HUB_FEATURE_C_PORT_RESET        0x14

/* USB HUB Class codes */
#define USB_HUB_CLASS_CODE    0x09
#define USB_HUB_SUBCLASS_CODE 0x00
#define USB_HUB_PROTOCOL_CODE 0x00

/* Maximum ports per hub */
#define USB_HUB_MAX_PORTS 7

/* Maximum hub descriptor size.
 * 7 bytes (fixed) + max 32 bytes (DeviceRemovable) + max 32 bytes (PortPwrCtrlMask)
 */
#define USBH_HUB_DESC_BUF_SIZE 71

/* USB HUB descriptor structure */
struct usb_hub_descriptor {
	uint8_t bDescLength;
	uint8_t bDescriptorType;
	uint8_t bNbrPorts;
	uint16_t wHubCharacteristics;
	uint8_t bPwrOn2PwrGood;
	uint8_t bHubContrCurrent;
	/** uint8_t DeviceRemovable[]; */
	/** uint8_t PortPwrCtrlMask[]; */
} __packed;

/* USB HUB status structure */
struct usb_hub_status {
	uint16_t wHubStatus;
	uint16_t wHubChange;
} __packed;

/* USB HUB port status structure */
struct usb_hub_port_status {
	uint16_t wPortStatus;
	uint16_t wPortChange;
} __packed;

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

/* Hub statistics information */
struct usbh_hub_stats {
	uint16_t total_hubs;
	uint16_t operational_hubs;
	uint16_t total_ports;
	uint16_t active_ports;
	uint8_t max_level;
	uint8_t hubs_by_level[8];
};

/* Hub instance structure */
struct usbh_hub_instance {
	struct usb_device *hub_udev;
	struct usbh_context *uhs_ctx;

	/* Hub properties */
	uint8_t num_ports;
	uint8_t hub_level;

	/* Control transfer handling */
	struct k_sem ctrl_sem;
	struct k_mutex ctrl_lock;
	int ctrl_status;

	/* Hub descriptor buffer */
	uint8_t hub_desc_buf[USBH_HUB_DESC_BUF_SIZE];

	/* Status buffers */
	struct usb_hub_status hub_status;
	struct usb_hub_port_status port_status;
};

/* Hub transfer callback function type */
typedef void (*usbh_hub_callback_t)(void *param, uint8_t *data, uint32_t data_len, int status);

int usbh_hub_init_instance(struct usbh_hub_instance *hub_instance, struct usb_device *udev);

int usbh_hub_get_descriptor(struct usbh_hub_instance *hub_instance, uint8_t *buffer,
			    uint16_t buffer_length);

int usbh_hub_set_port_feature(struct usbh_hub_instance *hub_instance, uint8_t port_number,
			      uint8_t feature);

int usbh_hub_clear_port_feature(struct usbh_hub_instance *hub_instance, uint8_t port_number,
				uint8_t feature);

int usbh_hub_get_port_status(struct usbh_hub_instance *hub_instance, uint8_t port_number,
			     uint16_t *const wPortStatus, uint16_t *const wPortChange);

int usbh_hub_get_hub_status(struct usbh_hub_instance *hub_instance, uint16_t *const wHubStatus,
			    uint16_t *const wHubChange);

int usbh_hub_clear_hub_feature(struct usbh_hub_instance *hub_instance, uint8_t feature);

void usbh_hub_cleanup_instance(struct usbh_hub_instance *hub_instance);

#endif /* _USBH_HUB_H_ */
