/*
 * Copyright (c) 2022 Emerson Electric Co.
 * SPDX-FileCopyrightText: Copyright 2026 NXP
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief USB Hub Class public header
 */

#ifndef ZEPHYR_INCLUDE_USB_CLASS_USB_HUB_H_
#define ZEPHYR_INCLUDE_USB_CLASS_USB_HUB_H_

/**
 * @name USB Hub Class Codes
 * @{
 */
/** Hub class code. */
#define USB_HUB_CLASS_CODE		0x09
/** Hub subclass code. */
#define USB_HUB_SUBCLASS_CODE		0x00
/** Hub protocol code. */
#define USB_HUB_PROTOCOL_CODE		0x00
/** @} */

/**
 * @name USB Hub Descriptor Types
 * @{
 */
/** Hub descriptor type. */
#define USB_HUB_DESCRIPTOR_TYPE		0x29
/** @} */

/**
 * @name USB Hub Class Request Codes. See Table 11-16 of the specification.
 * @{
 */
/** Get Status request. */
#define USB_HCREQ_GET_STATUS		0x00
/** Clear Feature request. */
#define USB_HCREQ_CLEAR_FEATURE		0x01
/** Set Feature request. */
#define USB_HCREQ_SET_FEATURE		0x03
/** Get Descriptor request. */
#define USB_HCREQ_GET_DESCRIPTOR	0x06
/** Set Descriptor request. */
#define USB_HCREQ_SET_DESCRIPTOR	0x07
/** Clear TT Buffer request. */
#define USB_HCREQ_CLEAR_TT_BUFFER	0x08
/** Reset TT request. */
#define USB_HCREQ_RESET_TT		0x09
/** Get TT State request. */
#define USB_HCREQ_GET_TT_STATE		0x0A
/** Stop TT request. */
#define USB_HCREQ_STOP_TT		0x0B
/** @} */

/**
 * @name USB Hub Class Feature Selectors. See Table 11-17 of the specification.
 * @{
 */
/** Hub local power change feature selector. */
#define USB_HCFS_C_HUB_LOCAL_POWER	0x00
/** Hub over-current change feature selector. */
#define USB_HCFS_C_HUB_OVER_CURRENT	0x01
/** Port connection feature selector. */
#define USB_HCFS_PORT_CONNECTION	0x00
/** Port enable feature selector. */
#define USB_HCFS_PORT_ENABLE		0x01
/** Port suspend feature selector. */
#define USB_HCFS_PORT_SUSPEND		0x02
/** Port over-current feature selector. */
#define USB_HCFS_PORT_OVER_CURRENT	0x03
/** Port reset feature selector. */
#define USB_HCFS_PORT_RESET		0x04
/** Port power feature selector. */
#define USB_HCFS_PORT_POWER		0x08
/** Port low speed feature selector. */
#define USB_HCFS_PORT_LOW_SPEED		0x09
/** Port connection change feature selector. */
#define USB_HCFS_C_PORT_CONNECTION	0x10
/** Port enable change feature selector. */
#define USB_HCFS_C_PORT_ENABLE		0x11
/** Port suspend change feature selector. */
#define USB_HCFS_C_PORT_SUSPEND		0x12
/** Port over-current change feature selector. */
#define USB_HCFS_C_PORT_OVER_CURRENT	0x13
/** Port reset change feature selector. */
#define USB_HCFS_C_PORT_RESET		0x14
/** Port test feature selector. */
#define USB_HCFS_PORT_TEST		0x15
/** Port indicator feature selector. */
#define USB_HCFS_PORT_INDICATOR		0x16
/** @} */

/**
 * @name USB Hub Status Field, wHubStatus. See Table 11-19 of the specification.
 * @{
 */
/** Hub local power status bit. */
#define USB_HUB_STATUS_LOCAL_POWER	BIT(0)
/** Hub over-current status bit. */
#define USB_HUB_STATUS_OVER_CURRENT	BIT(1)
/** @} */

/**
 * @name USB Hub Change Field, wHubChange. See Table 11-20 of the specification.
 * @{
 */
/** Hub local power change bit. */
#define USB_HUB_CHANGE_LOCAL_POWER	BIT(0)
/** Hub over-current change bit. */
#define USB_HUB_CHANGE_OVER_CURRENT	BIT(1)
/** @} */

/**
 * @name USB Hub Port Status Field, wPortStatus. See Table 11-21 of the specification.
 * @{
 */
/** Port connection status bit. */
#define USB_HUB_PORT_STATUS_CONNECTION		BIT(0)
/** Port enable status bit. */
#define USB_HUB_PORT_STATUS_ENABLE		BIT(1)
/** Port suspend status bit. */
#define USB_HUB_PORT_STATUS_SUSPEND		BIT(2)
/** Port over-current status bit. */
#define USB_HUB_PORT_STATUS_OVER_CURRENT	BIT(3)
/** Port reset status bit. */
#define USB_HUB_PORT_STATUS_RESET		BIT(4)
/** Port power status bit. */
#define USB_HUB_PORT_STATUS_POWER		BIT(8)
/** Port low speed device attached bit. */
#define USB_HUB_PORT_STATUS_LOW_SPEED		BIT(9)
/** Port high speed device attached bit. */
#define USB_HUB_PORT_STATUS_HIGH_SPEED		BIT(10)
/** Port test mode bit. */
#define USB_HUB_PORT_STATUS_TEST		BIT(11)
/** Port indicator control bit. */
#define USB_HUB_PORT_STATUS_INDICATOR		BIT(12)
/** @} */

/**
 * @name USB Hub Port Change Field, wPortChange. See Table 11-22 of the specification.
 * @{
 */
/** Port connection change bit. */
#define USB_HUB_PORT_CHANGE_CONNECTION		BIT(0)
/** Port enable change bit. */
#define USB_HUB_PORT_CHANGE_ENABLE		BIT(1)
/** Port suspend change bit. */
#define USB_HUB_PORT_CHANGE_SUSPEND		BIT(2)
/** Port over-current change bit. */
#define USB_HUB_PORT_CHANGE_OVER_CURRENT	BIT(3)
/** Port reset change bit. */
#define USB_HUB_PORT_CHANGE_RESET		BIT(4)
/** @} */

/**
 * Maximum hub descriptor size.
 * 7 bytes (fixed) + max 32 bytes (DeviceRemovable) + max 32 bytes (PortPwrCtrlMask)
 */
#define USB_HUB_DESC_BUF_SIZE 71

/**
 * @brief Get Think Time value from wHubCharacteristics field.
 *
 * @param wHubCharacteristics Hub descriptor field wHubCharacteristics.
 *
 * @return Think time in full-speed bit times (8, 16, 24, or 32).
 */
#define USB_HUB_GET_THINK_TIME(wHubCharacteristics) \
	(((((wHubCharacteristics) & 0x0060U) >> 5) + 1) << 3)

/**
 * USB Hub Descriptor. See Table 11-13 of the specification.
 */
struct usb_hub_descriptor {
	/** Descriptor length. */
	uint8_t bDescLength;
	/** Descriptor type. */
	uint8_t bDescriptorType;
	/** Number of downstream facing ports. */
	uint8_t bNbrPorts;
	/** Hub characteristics bitmap. */
	uint16_t wHubCharacteristics;
	/** Time from power-on to power-good (in 2ms intervals). */
	uint8_t bPwrOn2PwrGood;
	/** Maximum current requirements of the Hub Controller (in mA). */
	uint8_t bHubContrCurrent;
	/** Variable length fields follow:
	 * uint8_t DeviceRemovable[];
	 * uint8_t PortPwrCtrlMask[];
	 */
} __packed;

/**
 * USB Hub Status. See Table 11-19 and 11-20 of the specification.
 */
struct usb_hub_status {
	/** Hub status bitmap. */
	uint16_t wHubStatus;
	/** Hub status change bitmap. */
	uint16_t wHubChange;
} __packed;

/**
 * USB Hub Port Status. See Table 11-21 and 11-22 of the specification.
 */
struct usb_hub_port_status {
	/** Port status bitmap. */
	uint16_t wPortStatus;
	/** Port status change bitmap. */
	uint16_t wPortChange;
} __packed;

#endif /* ZEPHYR_INCLUDE_USB_CLASS_USB_HUB_H_ */
