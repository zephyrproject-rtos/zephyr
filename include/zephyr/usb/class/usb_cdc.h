/* usb_cdc.h - USB CDC-ACM and CDC-ECM public header */

/*
 * Copyright (c) 2017 PHYTEC Messtechnik GmbH
 * Copyright (c) 2025 - 2026 NXP
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

#ifndef ZEPHYR_INCLUDE_USB_CLASS_USB_CDC_H_
#define ZEPHYR_INCLUDE_USB_CLASS_USB_CDC_H_

/** CDC Specification release number in BCD format */
#define CDC_SRN_1_20 0x0120 __DEPRECATED_MACRO

/** Communications Class Subclass Codes */
#define ACM_SUBCLASS 0x02
#define ECM_SUBCLASS 0x06
#define EEM_SUBCLASS 0x0c
#define NCM_SUBCLASS 0x0d

/** Communications Class Protocol Codes */
#define AT_CMD_V250_PROTOCOL 0x01
#define EEM_PROTOCOL         0x07
#define ACM_VENDOR_PROTOCOL  0xFF
#define NCM_DATA_PROTOCOL    0x01

/**
 * @brief Data Class Interface Codes
 * @note CDC120-20101103-track.pdf, 4.5, Table 6
 */
#define DATA_INTERFACE_CLASS 0x0A

/**
 * @brief bDescriptor SubType for Communications
 * Class Functional Descriptors
 * @note CDC120-20101103-track.pdf, 5.2.3, Table 13
 */
#define HEADER_FUNC_DESC          0x00
#define CALL_MANAGEMENT_FUNC_DESC 0x01
#define ACM_FUNC_DESC             0x02
#define UNION_FUNC_DESC           0x06
#define ETHERNET_FUNC_DESC        0x0F
#define ETHERNET_FUNC_DESC_NCM    0x1a

/**
 * @brief PSTN Subclass Specific Requests
 * for ACM devices
 * @note PSTN120.pdf, 6.3, Table 13
 */
#define CDC_SEND_ENC_CMD       0x00
#define CDC_GET_ENC_RSP        0x01
#define SET_LINE_CODING        0x20
#define GET_LINE_CODING        0x21
#define SET_CONTROL_LINE_STATE 0x22

/**
 * @brief PSTN Subclass Class-Specific Notification Codes
 * @note PSTN120.pdf, 6.5, Table 30
 */
/** Network connection notification */
#define USB_CDC_NETWORK_CONNECTION      0x00
/** Response available notification */
#define USB_CDC_RESPONSE_AVAILABLE      0x01
/** Auxiliary jack hook state notification */
#define USB_CDC_AUX_JACK_HOOK_STATE     0x08
/** Ring detect notification */
#define USB_CDC_RING_DETECT             0x09
/** Serial state notification */
#define USB_CDC_SERIAL_STATE            0x20
/** Line state change notification */
#define USB_CDC_LINE_STATE_CHANGE       0x23
/** Call state change notification */
#define USB_CDC_CALL_STATE_CHANGE       0x28
/** Connection speed change notification */
#define USB_CDC_CONNECTION_SPEED_CHANGE 0x2A

/**
 * @brief PSTN UART State Bitmap Values
 * @note PSTN120.pdf, 6.5.4, Table 31
 */
#define USB_CDC_SERIAL_STATE_OVERRUN    BIT(6)
#define USB_CDC_SERIAL_STATE_PARITY     BIT(5)
#define USB_CDC_SERIAL_STATE_FRAMING    BIT(4)
#define USB_CDC_SERIAL_STATE_RINGSIGNAL BIT(3)
#define USB_CDC_SERIAL_STATE_BREAK      BIT(2)
#define USB_CDC_SERIAL_STATE_TXCARRIER  BIT(1)
#define USB_CDC_SERIAL_STATE_RXCARRIER  BIT(0)

/** Control Signal Bitmap Values for SetControlLineState */
#define SET_CONTROL_LINE_STATE_RTS 0x02
#define SET_CONTROL_LINE_STATE_DTR 0x01

/** Enhance enum uart_line_ctrl with CDC specific values */
#define USB_CDC_LINE_CTRL_BAUD_RATE   UART_LINE_CTRL_BAUD_RATE
#define USB_CDC_LINE_CTRL_DCD         UART_LINE_CTRL_DCD
#define USB_CDC_LINE_CTRL_DSR         UART_LINE_CTRL_DSR
#define USB_CDC_LINE_CTRL_BREAK       BIT(5)
#define USB_CDC_LINE_CTRL_RING_SIGNAL BIT(6)
#define USB_CDC_LINE_CTRL_FRAMING     BIT(7)
#define USB_CDC_LINE_CTRL_PARITY      BIT(8)
#define USB_CDC_LINE_CTRL_OVER_RUN    BIT(9)

/** UART State Bitmap Values */
#define SERIAL_STATE_OVER_RUN    0x40
#define SERIAL_STATE_PARITY      0x20
#define SERIAL_STATE_FRAMING     0x10
#define SERIAL_STATE_RING_SIGNAL 0x08
#define SERIAL_STATE_BREAK       0x04
#define SERIAL_STATE_TX_CARRIER  0x02
#define SERIAL_STATE_RX_CARRIER  0x01

/**
 * @brief PSTN Subclass Line Coding Values
 *
 * @note PSTN120.pdf, 6.3.11, Table 17
 */
#define USB_CDC_LINE_CODING_STOP_BITS_1   0
#define USB_CDC_LINE_CODING_STOP_BITS_1_5 1
#define USB_CDC_LINE_CODING_STOP_BITS_2   2

#define USB_CDC_LINE_CODING_PARITY_NO    0
#define USB_CDC_LINE_CODING_PARITY_ODD   1
#define USB_CDC_LINE_CODING_PARITY_EVEN  2
#define USB_CDC_LINE_CODING_PARITY_MARK  3
#define USB_CDC_LINE_CODING_PARITY_SPACE 4

#define USB_CDC_LINE_CODING_DATA_BITS_5 5
#define USB_CDC_LINE_CODING_DATA_BITS_6 6
#define USB_CDC_LINE_CODING_DATA_BITS_7 7
#define USB_CDC_LINE_CODING_DATA_BITS_8 8

/**
 * @brief Class-Specific Request Codes for Ethernet subclass
 * @note ECM120.pdf, 6.2, Table 6
 */
#define SET_ETHERNET_MULTICAST_FILTERS 0x40
#define SET_ETHERNET_PM_FILTER         0x41
#define GET_ETHERNET_PM_FILTER         0x42
#define SET_ETHERNET_PACKET_FILTER     0x43
#define GET_ETHERNET_STATISTIC         0x44

/**
 * @brief Class-Specific Request Codes for NCM subclass
 * @note NCM100.pdf, 6.2, Table 6-2
 */
#define GET_NTB_PARAMETERS    0x80
#define GET_NET_ADDRESS       0x81
#define SET_NET_ADDRESS       0x82
#define GET_NTB_FORMAT        0x83
#define SET_NTB_FORMAT        0x84
#define GET_NTB_INPUT_SIZE    0x85
#define SET_NTB_INPUT_SIZE    0x86
#define GET_MAX_DATAGRAM_SIZE 0x87
#define SET_MAX_DATAGRAM_SIZE 0x88
#define GET_CRC_MODE          0x89
#define SET_CRC_MODE          0x8A

/**
 * @defgroup ecm_power_management ECM Power Management Pattern Activation
 * @{
 * @brief ECM Subclass Ethernet Power Management Pattern Activation Boolean
 * @note ECM120.pdf, 6.2.3
 */

/** Power management pattern active */
#define ETHERNET_PM_PATTERN_TRUE  0x0001
/** Power management pattern inactive */
#define ETHERNET_PM_PATTERN_FALSE 0x0000

/** @} */

/**
 * @defgroup ecm_packet_filter ECM Packet Filter
 * @{
 * @brief ECM Subclass Ethernet Packet Filter Bitmap
 * @note ECM120.pdf, 6.2.4, Table 8
 */

/** Multicast packets */
#define PACKET_TYPE_MULTICAST     0x10
/** Broadcast packets */
#define PACKET_TYPE_BROADCAST     0x08
/** Directed packets */
#define PACKET_TYPE_DIRECTED      0x04
/** All multicast packets */
#define PACKET_TYPE_ALL_MULTICAST 0x02
/** Promiscuous mode */
#define PACKET_TYPE_PROMISCUOUS   0x01

/** @} */

/**
 * @defgroup ecm_statistics ECM Statistics Feature Selector Codes
 * @{
 * @brief ECM Subclass Ethernet Statistics Feature Selector Codes
 * @note ECM120.pdf, 6.2.5, Table 9
 */

/** Frames transmitted without errors */
#define ETHERNET_STAT_XMIT_OK                0x01
/** Frames received without errors */
#define ETHERNET_STAT_RCV_OK                 0x02
/** Frames transmitted with errors */
#define ETHERNET_STAT_XMIT_ERROR             0x03
/** Frames received with errors */
#define ETHERNET_STAT_RCV_ERROR              0x04
/** Frames missed due to no buffers */
#define ETHERNET_STAT_RCV_NO_BUFFER          0x05
/** Directed bytes transmitted without errors */
#define ETHERNET_STAT_DIRECTED_BYTES_XMIT    0x06
/** Directed frames transmitted without errors */
#define ETHERNET_STAT_DIRECTED_FRAMES_XMIT   0x07
/** Multicast bytes transmitted without errors */
#define ETHERNET_STAT_MULTICAST_BYTES_XMIT   0x08
/** Multicast frames transmitted without errors */
#define ETHERNET_STAT_MULTICAST_FRAMES_XMIT  0x09
/** Broadcast bytes transmitted without errors */
#define ETHERNET_STAT_BROADCAST_BYTES_XMIT   0x0A
/** Broadcast frames transmitted without errors */
#define ETHERNET_STAT_BROADCAST_FRAMES_XMIT  0x0B
/** Directed bytes received without errors */
#define ETHERNET_STAT_DIRECTED_BYTES_RCV     0x0C
/** Directed frames received without errors */
#define ETHERNET_STAT_DIRECTED_FRAMES_RCV    0x0D
/** Multicast bytes received without errors */
#define ETHERNET_STAT_MULTICAST_BYTES_RCV    0x0E
/** Multicast frames received without errors */
#define ETHERNET_STAT_MULTICAST_FRAMES_RCV   0x0F
/** Broadcast bytes received without errors */
#define ETHERNET_STAT_BROADCAST_BYTES_RCV    0x10
/** Broadcast frames received without errors */
#define ETHERNET_STAT_BROADCAST_FRAMES_RCV   0x11
/** Frames received with CRC or FCS error */
#define ETHERNET_STAT_RCV_CRC_ERROR          0x12
/** Length of transmit queue */
#define ETHERNET_STAT_TRANSMIT_QUEUE_LENGTH  0x13
/** Frames received with alignment error */
#define ETHERNET_STAT_RCV_ERROR_ALIGNMENT    0x14
/** Frames transmitted with exactly one collision */
#define ETHERNET_STAT_XMIT_ONE_COLLISION     0x15
/** Frames transmitted with more than one collision */
#define ETHERNET_STAT_XMIT_MORE_COLLISIONS   0x16
/** Frames transmitted after deferral */
#define ETHERNET_STAT_XMIT_DEFERRED          0x17
/** Frames not transmitted due to excessive collisions */
#define ETHERNET_STAT_XMIT_MAX_COLLISIONS    0x18
/** Frames not received due to overrun */
#define ETHERNET_STAT_RCV_OVERRUN            0x19
/** Frames not transmitted due to underrun */
#define ETHERNET_STAT_XMIT_UNDERRUN          0x1A
/** Frames transmitted with heartbeat failure */
#define ETHERNET_STAT_XMIT_HEARTBEAT_FAILURE 0x1B
/** Times carrier sense signal lost during transmission */
#define ETHERNET_STAT_XMIT_TIMES_CRS_LOST    0x1C
/** Frames transmitted with late collisions */
#define ETHERNET_STAT_XMIT_LATE_COLLISIONS   0x1D

/** @} */

/** Header Functional Descriptor */
struct cdc_header_descriptor {
	uint8_t bFunctionLength;
	uint8_t bDescriptorType;
	uint8_t bDescriptorSubtype;
	uint16_t bcdCDC;
} __packed;

/** Union Interface Functional Descriptor */
struct cdc_union_descriptor {
	uint8_t bFunctionLength;
	uint8_t bDescriptorType;
	uint8_t bDescriptorSubtype;
	uint8_t bControlInterface;
	uint8_t bSubordinateInterface0;
} __packed;

/** Call Management Functional Descriptor */
struct cdc_cm_descriptor {
	uint8_t bFunctionLength;
	uint8_t bDescriptorType;
	uint8_t bDescriptorSubtype;
	uint8_t bmCapabilities;
	uint8_t bDataInterface;
} __packed;

/** Abstract Control Management Functional Descriptor */
struct cdc_acm_descriptor {
	uint8_t bFunctionLength;
	uint8_t bDescriptorType;
	uint8_t bDescriptorSubtype;
	uint8_t bmCapabilities;
} __packed;

/** Data structure for GET_LINE_CODING / SET_LINE_CODING class requests */
struct cdc_acm_line_coding {
	uint32_t dwDTERate;
	uint8_t bCharFormat;
	uint8_t bParityType;
	uint8_t bDataBits;
} __packed;

/** Data structure for the notification about SerialState */
struct cdc_acm_notification {
	uint8_t bmRequestType;
	uint8_t bNotificationType;
	uint16_t wValue;
	uint16_t wIndex;
	uint16_t wLength;
	uint16_t data;
} __packed;

/** Ethernet Networking Functional Descriptor */
struct cdc_ecm_descriptor {
	uint8_t bFunctionLength;
	uint8_t bDescriptorType;
	uint8_t bDescriptorSubtype;
	uint8_t iMACAddress;
	uint32_t bmEthernetStatistics;
	uint16_t wMaxSegmentSize;
	uint16_t wNumberMCFilters;
	uint8_t bNumberPowerFilters;
} __packed;

/** Ethernet Network Control Model (NCM) Descriptor */
struct cdc_ncm_descriptor {
	uint8_t bFunctionLength;
	uint8_t bDescriptorType;
	uint8_t bDescriptorSubtype;
	uint16_t bcdNcmVersion;
	uint8_t bmNetworkCapabilities;
} __packed;

/**
 * @brief CDC Notification Packet
 *
 * Generic structure for CDC class-specific notifications
 */
struct cdc_notification_packet {
	/** Request type and direction */
	uint8_t bmRequestType;
	/** Notification type code */
	uint8_t bNotification;
	/** Notification-specific value */
	uint16_t wValue;
	/** Interface number */
	uint16_t wIndex;
	/** Length of data stage */
	uint16_t wLength;
} __packed;

#endif /* ZEPHYR_INCLUDE_USB_CLASS_USB_CDC_H_ */
