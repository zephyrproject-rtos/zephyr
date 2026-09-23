/*
 * Copyright (c) 2022 Emerson Electric Co.
 *
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

/** USB Hub Class Feature Selectors defined in spec. Table 11-17 */
#define USB_HCFS_C_HUB_LOCAL_POWER   0x00
#define USB_HCFS_C_HUB_OVER_CURRENT  0x01
#define USB_HCFS_PORT_CONNECTION     0x00
#define USB_HCFS_PORT_ENABLE         0x01
#define USB_HCFS_PORT_SUSPEND        0x02
#define USB_HCFS_PORT_OVER_CURRENT   0x03
#define USB_HCFS_PORT_RESET          0x04
#define USB_HCFS_PORT_POWER          0x08
#define USB_HCFS_PORT_LOW_SPEED      0x09
#define USB_HCFS_C_PORT_CONNECTION   0x10
#define USB_HCFS_C_PORT_ENABLE       0x11
#define USB_HCFS_C_PORT_SUSPEND      0x12
#define USB_HCFS_C_PORT_OVER_CURRENT 0x13
#define USB_HCFS_C_PORT_RESET        0x14
#define USB_HCFS_PORT_TEST           0x15
#define USB_HCFS_PORT_INDICATOR      0x16

/** USB Hub Class Request Codes defined in spec. Table 11-16 */
#define USB_HCREQ_GET_STATUS      0x00
#define USB_HCREQ_CLEAR_FEATURE   0x01
#define USB_HCREQ_SET_FEATURE     0x03
#define USB_HCREQ_GET_DESCRIPTOR  0x06
#define USB_HCREQ_SET_DESCRIPTOR  0x07
#define USB_HCREQ_CLEAR_TT_BUFFER 0x08
#define USB_HCREQ_RESET_TT        0x09
#define USB_HCREQ_GET_TT_STATE    0x0A
#define USB_HCREQ_STOP_TT         0x0B

/** Hub descriptor type (USB 2.0 Table 11-13). */
#define USB_DT_HUB             0x29
/** Fixed-size portion of a USB 2.0 hub descriptor. */
#define USB_DT_HUB_NONVAR_SIZE 7

/** Hub class device protocols (bcdUSB companion). */
#define USB_HUB_PR_FS           0
#define USB_HUB_PR_HS_NO_TT     0
#define USB_HUB_PR_HS_SINGLE_TT 1
#define USB_HUB_PR_HS_MULTI_TT  2
#define USB_HUB_PR_SS           3

/** wPortStatus bits (USB 2.0 Table 11-21). */
#define USB_PORT_STAT_CONNECTION  0x0001
#define USB_PORT_STAT_ENABLE      0x0002
#define USB_PORT_STAT_SUSPEND     0x0004
#define USB_PORT_STAT_OVERCURRENT 0x0008
#define USB_PORT_STAT_RESET       0x0010
#define USB_PORT_STAT_POWER       0x0100
#define USB_PORT_STAT_LOW_SPEED   0x0200
#define USB_PORT_STAT_HIGH_SPEED  0x0400

/** wPortChange bits (USB 2.0 Table 11-22). */
#define USB_PORT_STAT_C_CONNECTION  0x0001
#define USB_PORT_STAT_C_ENABLE      0x0002
#define USB_PORT_STAT_C_SUSPEND     0x0004
#define USB_PORT_STAT_C_OVERCURRENT 0x0008
#define USB_PORT_STAT_C_RESET       0x0010

/** wHubCharacteristics masks (USB 2.0 Table 11-13). */
#define HUB_CHAR_LPSM           0x0003
#define HUB_CHAR_COMMON_LPSM    0x0000
#define HUB_CHAR_INDV_PORT_LPSM 0x0001
#define HUB_CHAR_NO_LPSM        0x0002

/** Hub and port GET_STATUS results (USB 2.0 Table 11-19). */
struct usb_port_status {
	uint16_t wPortStatus;
	uint16_t wPortChange;
} __packed;

struct usb_hub_status {
	uint16_t wHubStatus;
	uint16_t wHubChange;
} __packed;

/** USB 2.0 hub descriptor (variable length; see @ref usb_hub_descriptor_size). */
struct usb_hub_descriptor {
	uint8_t bDescLength;
	uint8_t bDescriptorType;
	uint8_t bNbrPorts;
	uint16_t wHubCharacteristics;
	uint8_t bPwrOn2PwrGood;
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
