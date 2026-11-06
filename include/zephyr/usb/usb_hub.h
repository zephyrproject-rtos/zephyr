/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief USB 2.0 hub definitions (Chapter 11, host core)
 */

#ifndef ZEPHYR_INCLUDE_USB_USB_HUB_H_
#define ZEPHYR_INCLUDE_USB_USB_HUB_H_

#include <stdint.h>
#include <zephyr/sys/util.h>

/**
 * @name USB Hub Class Feature Selectors
 * @anchor usb_hub_class_feature_selectors
 * USB 2.0 Table 11-17.
 * @{
 */
/** Hub local power feature selector. */
#define USB_HCFS_C_HUB_LOCAL_POWER   0x00
/** Hub over-current feature selector. */
#define USB_HCFS_C_HUB_OVER_CURRENT  0x01
/** Port connection feature selector. */
#define USB_HCFS_PORT_CONNECTION     0x00
/** Port enable feature selector. */
#define USB_HCFS_PORT_ENABLE         0x01
/** Port suspend feature selector. */
#define USB_HCFS_PORT_SUSPEND        0x02
/** Port over-current feature selector. */
#define USB_HCFS_PORT_OVER_CURRENT   0x03
/** Port reset feature selector. */
#define USB_HCFS_PORT_RESET          0x04
/** Port power feature selector. */
#define USB_HCFS_PORT_POWER          0x08
/** Port low-speed feature selector. */
#define USB_HCFS_PORT_LOW_SPEED      0x09
/** Port connection change feature selector. */
#define USB_HCFS_C_PORT_CONNECTION   0x10
/** Port enable change feature selector. */
#define USB_HCFS_C_PORT_ENABLE       0x11
/** Port suspend change feature selector. */
#define USB_HCFS_C_PORT_SUSPEND      0x12
/** Port over-current change feature selector. */
#define USB_HCFS_C_PORT_OVER_CURRENT 0x13
/** Port reset change feature selector. */
#define USB_HCFS_C_PORT_RESET        0x14
/** Port test feature selector. */
#define USB_HCFS_PORT_TEST           0x15
/** Port indicator feature selector. */
#define USB_HCFS_PORT_INDICATOR      0x16
/** @} */

/**
 * @name USB Hub Class Request Codes
 * @anchor usb_hub_class_request_codes
 * USB 2.0 Table 11-16.
 * @{
 */
/** Hub GET_STATUS request. */
#define USB_HCREQ_GET_STATUS      0x00
/** Hub CLEAR_FEATURE request. */
#define USB_HCREQ_CLEAR_FEATURE   0x01
/** Hub SET_FEATURE request. */
#define USB_HCREQ_SET_FEATURE     0x03
/** Hub GET_DESCRIPTOR request. */
#define USB_HCREQ_GET_DESCRIPTOR  0x06
/** Hub SET_DESCRIPTOR request. */
#define USB_HCREQ_SET_DESCRIPTOR  0x07
/** Hub CLEAR_TT_BUFFER request. */
#define USB_HCREQ_CLEAR_TT_BUFFER 0x08
/** Hub RESET_TT request. */
#define USB_HCREQ_RESET_TT        0x09
/** Hub GET_TT_STATE request. */
#define USB_HCREQ_GET_TT_STATE    0x0A
/** Hub STOP_TT request. */
#define USB_HCREQ_STOP_TT         0x0B
/** @} */

/** Hub descriptor type (USB 2.0 Table 11-13). */
#define USB_DT_HUB             0x29
/** Fixed-size portion of a USB 2.0 hub descriptor. */
#define USB_DT_HUB_NONVAR_SIZE 7

/**
 * @name Hub class device protocols
 * @anchor usb_hub_class_protocols
 * @{
 */
/** Full-speed hub without transaction translator. */
#define USB_HUB_PR_FS           0
/** High-speed hub without transaction translator. */
#define USB_HUB_PR_HS_NO_TT     0
/** High-speed hub with single transaction translator. */
#define USB_HUB_PR_HS_SINGLE_TT 1
/** High-speed hub with multiple transaction translators. */
#define USB_HUB_PR_HS_MULTI_TT  2
/** SuperSpeed hub protocol. */
#define USB_HUB_PR_SS           3
/** @} */

/**
 * @name USB port status bits
 * @anchor usb_port_status_bits
 * USB 2.0 Table 11-21 (wPortStatus).
 * @{
 */
/** Device connected on the port. */
#define USB_PORT_STAT_CONNECTION  0x0001
/** Port enabled. */
#define USB_PORT_STAT_ENABLE      0x0002
/** Port suspended. */
#define USB_PORT_STAT_SUSPEND     0x0004
/** Over-current condition on the port. */
#define USB_PORT_STAT_OVERCURRENT 0x0008
/** Port reset signaling active. */
#define USB_PORT_STAT_RESET       0x0010
/** Port power enabled. */
#define USB_PORT_STAT_POWER       0x0100
/** Low-speed device attached. */
#define USB_PORT_STAT_LOW_SPEED   0x0200
/** High-speed device attached. */
#define USB_PORT_STAT_HIGH_SPEED  0x0400
/** @} */

/**
 * @name USB port change bits
 * @anchor usb_port_change_bits
 * USB 2.0 Table 11-22 (wPortChange).
 * @{
 */
/** Connection change. */
#define USB_PORT_STAT_C_CONNECTION  0x0001
/** Enable change. */
#define USB_PORT_STAT_C_ENABLE      0x0002
/** Suspend change. */
#define USB_PORT_STAT_C_SUSPEND     0x0004
/** Over-current change. */
#define USB_PORT_STAT_C_OVERCURRENT 0x0008
/** Reset change. */
#define USB_PORT_STAT_C_RESET       0x0010
/** @} */

/**
 * @name Hub characteristics masks
 * @anchor usb_hub_characteristics
 * USB 2.0 Table 11-13 (wHubCharacteristics).
 * @{
 */
/** Logical power switching mode field mask. */
#define HUB_CHAR_LPSM           0x0003
/** Ganged power switching. */
#define HUB_CHAR_COMMON_LPSM    0x0000
/** Individual port power switching. */
#define HUB_CHAR_INDV_PORT_LPSM 0x0001
/** No power switching. */
#define HUB_CHAR_NO_LPSM        0x0002
/** @} */

/** Hub and port GET_STATUS results (USB 2.0 Table 11-19). */
struct usb_port_status {
	/** Port status flags (@ref usb_port_status_bits). */
	uint16_t wPortStatus;
	/** Port change flags (@ref usb_port_change_bits). */
	uint16_t wPortChange;
} __packed;

/** Hub GET_STATUS result (USB 2.0 Table 11-19). */
struct usb_hub_status {
	/** Hub status flags. */
	uint16_t wHubStatus;
	/** Hub change flags. */
	uint16_t wHubChange;
} __packed;

/** USB 2.0 hub descriptor (variable length; see @ref usb_hub_descriptor_size). */
struct usb_hub_descriptor {
	/** Descriptor length in bytes. */
	uint8_t bDescLength;
	/** Descriptor type (@ref USB_DT_HUB). */
	uint8_t bDescriptorType;
	/** Number of downstream facing ports. */
	uint8_t bNbrPorts;
	/** Hub characteristics (@ref usb_hub_characteristics). */
	uint16_t wHubCharacteristics;
	/** Power-on to power-good time in 2 ms units. */
	uint8_t bPwrOn2PwrGood;
	/** Maximum hub controller current in mA. */
	uint8_t bHubContrCurrent;
} __packed;

/**
 * @brief Total hub descriptor length for a given port count.
 *
 * Includes DeviceRemovable and PortPwrCtrlMask bit arrays.
 *
 * @param nports Value of bNbrPorts from the hub descriptor.
 * @return Descriptor length in bytes.
 */
static inline size_t usb_hub_descriptor_size(uint8_t nports)
{
	const size_t var = (2U * ((nports + 1U + 7U) / 8U));

	return USB_DT_HUB_NONVAR_SIZE + var;
}

#endif /* ZEPHYR_INCLUDE_USB_USB_HUB_H_ */
