/* usb_cdc.h - USB CDC-ACM and CDC-ECM public header */

/*
 * Copyright (c) 2017 PHYTEC Messtechnik GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */


/**
 * @file
 * @brief USB Communications Device Class (CDC) public header
 *
 * Header follows the Class Definitions for
 * Communications Devices Specification (CDC120-20101103-track.pdf),
 * PSTN Devices Specification (PSTN120.pdf) and
 * Ethernet Control Model Devices Specification (ECM120.pdf).
 * Header is limited to ACM and ECM Subclasses.
 */

#ifndef __USB_CDC_H__
#define __USB_CDC_H__

/** CDC Specification release number in BCD format */
#define CDC_SRN_1_20			0x0120

/** CDC Class Code */
#define COMMUNICATION_DEVICE_CLASS	0x02

/** Communications Class Subclass Codes */
#define ACM_SUBCLASS			0x02
#define ECM_SUBCLASS			0x06
#define EEM_SUBCLASS			0x0c

/** Communications Class Protocol Codes */
#define AT_CMD_V250_PROTOCOL		0x01
#define EEM_PROTOCOL			0x07

/**
 * @brief Data Class Interface Codes
 * @note CDC120-20101103-track.pdf, 4.5, Table 6
 */
#define DATA_INTERFACE_CLASS		0x0A

/**
 * @brief Values for the bDescriptorType Field
 * @note CDC120-20101103-track.pdf, 5.2.3, Table 12
 */
#define CS_INTERFACE			0x24
#define CS_ENDPOINT			0x25

/**
 * @brief bDescriptor SubType for Communications
 * Class Functional Descriptors
 * @note CDC120-20101103-track.pdf, 5.2.3, Table 13
 */
#define HEADER_FUNC_DESC		0x00
#define CALL_MANAGEMENT_FUNC_DESC	0x01
#define ACM_FUNC_DESC			0x02
#define UNION_FUNC_DESC			0x06
#define ETHERNET_FUNC_DESC		0x0F

/**
 * @brief PSTN Subclass Specific Requests
 * for ACM devices
 * @note PSTN120.pdf, 6.3, Table 13
 */
#define SET_LINE_CODING			0x20
#define GET_LINE_CODING			0x21
#define SET_CONTROL_LINE_STATE		0x22

/** Control Signal Bitmap Values for SetControlLineState */
#define SET_CONTROL_LINE_STATE_RTS	0x02
#define SET_CONTROL_LINE_STATE_DTR	0x01

/** UART State Bitmap Values */
#define SERIAL_STATE_OVERRUN		0x40
#define SERIAL_STATE_PARITY		0x20
#define SERIAL_STATE_FRAMING		0x10
#define SERIAL_STATE_RING		0x08
#define SERIAL_STATE_BREAK		0x04
#define SERIAL_STATE_TX_CARRIER		0x02
#define SERIAL_STATE_RX_CARRIER		0x01

/**
 * @brief Class-Specific Request Codes for Ethernet subclass
 * @note ECM120.pdf, 6.2, Table 6
 */
#define SET_ETHERNET_MULTICAST_FILTERS	0x40
#define SET_ETHERNET_PM_FILTER		0x41
#define GET_ETHERNET_PM_FILTER		0x42
#define SET_ETHERNET_PACKET_FILTER	0x43
#define GET_ETHERNET_STATISTIC		0x44

/** Ethernet Packet Filter Bitmap */
#define PACKET_TYPE_MULTICAST		0x10
#define PACKET_TYPE_BROADCAST		0x08
#define PACKET_TYPE_DIRECTED		0x04
#define PACKET_TYPE_ALL_MULTICAST	0x02
#define PACKET_TYPE_PROMISCUOUS		0x01

/** Header Functional Descriptor */
struct cdc_header_descriptor {
	u8_t bFunctionLength;
	u8_t bDescriptorType;
	u8_t bDescriptorSubtype;
	u16_t bcdCDC;
} __packed;

/** Union Interface Functional Descriptor */
struct cdc_union_descriptor {
	u8_t bFunctionLength;
	u8_t bDescriptorType;
	u8_t bDescriptorSubtype;
	u8_t bControlInterface;
	u8_t bSubordinateInterface0;
} __packed;

/** Call Management Functional Descriptor */
struct cdc_cm_descriptor {
	u8_t bFunctionLength;
	u8_t bDescriptorType;
	u8_t bDescriptorSubtype;
	u8_t bmCapabilities;
	u8_t bDataInterface;
} __packed;

/** Abstract Control Management Functional Descriptor */
struct cdc_acm_descriptor {
	u8_t bFunctionLength;
	u8_t bDescriptorType;
	u8_t bDescriptorSubtype;
	u8_t bmCapabilities;
} __packed;

/** Data structure for GET_LINE_CODING / SET_LINE_CODING class requests */
struct cdc_acm_line_coding {
	u32_t dwDTERate;
	u8_t bCharFormat;
	u8_t bParityType;
	u8_t bDataBits;
} __packed;

/** Data structure for the notification about SerialState */
struct cdc_acm_notification {
	u8_t bmRequestType;
	u8_t bNotificationType;
	u16_t wValue;
	u16_t wIndex;
	u16_t wLength;
	u16_t data;
} __packed;

/** Ethernet Networking Functional Descriptor */
struct cdc_ecm_descriptor {
	u8_t bFunctionLength;
	u8_t bDescriptorType;
	u8_t bDescriptorSubtype;
	u8_t iMACAddress;
	u32_t bmEthernetStatistics;
	u16_t wMaxSegmentSize;
	u16_t wNumberMCFilters;
	u8_t bNumberPowerFilters;
} __packed;

#endif /* __USB_CDC_H__ */
