/*
 * Copyright (c) 2022 Emerson Electric Co.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief USB Hub Class device API header
 */

#ifndef ZEPHYR_INCLUDE_USB_CLASS_USB_HUB_H_
#define ZEPHYR_INCLUDE_USB_CLASS_USB_HUB_H_

#include <stdint.h>
#include <zephyr/sys/util.h>

/** USB Hub descriptor type defined in spec. 11.23.2.1 */
#define USB_DESC_HUB			0x29

/** USB Hub descriptor defined in spec. Table 11-13, without the variable size bitmaps */
struct usb_hub_descriptor {
	/** Number of bytes in this descriptor, including the variable size bitmaps */
	uint8_t bDescLength;
	/** Descriptor type, @ref USB_DESC_HUB */
	uint8_t bDescriptorType;
	/** Number of downstream ports */
	uint8_t bNbrPorts;
	/** Hub characteristics, see the USB_HUB_CHAR_* definitions */
	uint16_t wHubCharacteristics;
	/** Time from power on to power good on a port, in 2 ms units */
	uint8_t bPwrOn2PwrGood;
	/** Maximum current of the hub controller, in mA */
	uint8_t bHubContrCurrent;
} __packed;

/** Logical power switching mode, wHubCharacteristics defined in spec. Table 11-13 */
#define USB_HUB_CHAR_LPSM_MASK		GENMASK(1, 0)
/** Hub is part of a compound device, wHubCharacteristics */
#define USB_HUB_CHAR_COMPOUND		BIT(2)
/** Over-current protection mode, wHubCharacteristics */
#define USB_HUB_CHAR_OCPM_MASK		GENMASK(4, 3)
/** Transaction translator think time, wHubCharacteristics */
#define USB_HUB_CHAR_TTTT_MASK		GENMASK(6, 5)
/** Port indicators are supported, wHubCharacteristics */
#define USB_HUB_CHAR_PORT_INDICATORS	BIT(7)

/** Local power source is good, wHubStatus defined in spec. Table 11-19 */
#define USB_HUB_STAT_LOCAL_POWER	BIT(0)
/** Over-current condition exists, wHubStatus */
#define USB_HUB_STAT_OVER_CURRENT	BIT(1)

/** Local power status changed, wHubChange defined in spec. Table 11-20 */
#define USB_HUB_CHANGE_LOCAL_POWER	BIT(0)
/** Over-current status changed, wHubChange */
#define USB_HUB_CHANGE_OVER_CURRENT	BIT(1)

/** Device is present on the port, wPortStatus defined in spec. Table 11-21 */
#define USB_HUB_PORT_STAT_CONNECTION	BIT(0)
/** Port is enabled, wPortStatus */
#define USB_HUB_PORT_STAT_ENABLE	BIT(1)
/** Port is suspended, wPortStatus */
#define USB_HUB_PORT_STAT_SUSPEND	BIT(2)
/** Over-current condition exists on the port, wPortStatus */
#define USB_HUB_PORT_STAT_OVER_CURRENT	BIT(3)
/** Reset signaling is asserted on the port, wPortStatus */
#define USB_HUB_PORT_STAT_RESET		BIT(4)
/** Port is powered, wPortStatus */
#define USB_HUB_PORT_STAT_POWER		BIT(8)
/** Low-speed device is attached to the port, wPortStatus */
#define USB_HUB_PORT_STAT_LOW_SPEED	BIT(9)
/** High-speed device is attached to the port, wPortStatus */
#define USB_HUB_PORT_STAT_HIGH_SPEED	BIT(10)
/** Port is in test mode, wPortStatus */
#define USB_HUB_PORT_STAT_TEST		BIT(11)
/** Port indicator is controlled by software, wPortStatus */
#define USB_HUB_PORT_STAT_INDICATOR	BIT(12)

/** Connection status changed, wPortChange defined in spec. Table 11-22 */
#define USB_HUB_PORT_CHANGE_CONNECTION	BIT(0)
/** Port enable status changed, wPortChange */
#define USB_HUB_PORT_CHANGE_ENABLE	BIT(1)
/** Suspend status changed, wPortChange */
#define USB_HUB_PORT_CHANGE_SUSPEND	BIT(2)
/** Over-current status changed, wPortChange */
#define USB_HUB_PORT_CHANGE_OVER_CURRENT BIT(3)
/** Reset completed, wPortChange */
#define USB_HUB_PORT_CHANGE_RESET	BIT(4)

/** USB Hub Class Feature Selectors defined in spec. Table 11-17 */
#define USB_HCFS_C_HUB_LOCAL_POWER	0x00
#define USB_HCFS_C_HUB_OVER_CURRENT	0x01
#define USB_HCFS_PORT_CONNECTION	0x00
#define USB_HCFS_PORT_ENABLE		0x01
#define USB_HCFS_PORT_SUSPEND		0x02
#define USB_HCFS_PORT_OVER_CURRENT	0x03
#define USB_HCFS_PORT_RESET		0x04
#define USB_HCFS_PORT_POWER		0x08
#define USB_HCFS_PORT_LOW_SPEED		0x09
#define USB_HCFS_C_PORT_CONNECTION	0x10
#define USB_HCFS_C_PORT_ENABLE		0x11
#define USB_HCFS_C_PORT_SUSPEND		0x12
#define USB_HCFS_C_PORT_OVER_CURRENT	0x13
#define USB_HCFS_C_PORT_RESET		0x14
#define USB_HCFS_PORT_TEST		0x15
#define USB_HCFS_PORT_INDICATOR		0x16

/** USB Hub Class Request Codes defined in spec. Table 11-16 */
#define USB_HCREQ_GET_STATUS		0x00
#define USB_HCREQ_CLEAR_FEATURE		0x01
#define USB_HCREQ_SET_FEATURE		0x03
#define USB_HCREQ_GET_DESCRIPTOR	0x06
#define USB_HCREQ_SET_DESCRIPTOR	0x07
#define USB_HCREQ_CLEAR_TT_BUFFER	0x08
#define USB_HCREQ_RESET_TT		0x09
#define USB_HCREQ_GET_TT_STATE		0x0A
#define USB_HCREQ_STOP_TT		0x0B

#endif /* ZEPHYR_INCLUDE_USB_CLASS_USB_HUB_H_ */
