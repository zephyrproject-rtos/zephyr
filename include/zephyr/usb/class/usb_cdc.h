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
 * PSTN Devices Specification (PSTN120.pdf),
 * Ethernet Control Model Devices Specification (ECM120.pdf) and
 * Network Control Model Devices Specification (NCM11_20260205.pdf).
 * Header is limited to ACM, ECM and NCM Subclasses.
 */

#ifndef ZEPHYR_INCLUDE_USB_CLASS_USB_CDC_H_
#define ZEPHYR_INCLUDE_USB_CLASS_USB_CDC_H_

/** CDC Specification release number in BCD format */
#define CDC_SRN_1_20			0x0120 __DEPRECATED_MACRO

/** Communications Class Subclass Codes */
#define ACM_SUBCLASS			0x02
#define ECM_SUBCLASS			0x06
#define EEM_SUBCLASS			0x0C
#define NCM_SUBCLASS			0x0D

/** Communications Class Protocol Codes */
#define AT_CMD_V250_PROTOCOL		0x01
#define EEM_PROTOCOL			0x07
#define ACM_VENDOR_PROTOCOL		0xFF

/**
 * @brief Data Class Interface Codes
 * @note CDC120-20101103-track.pdf, 4.5, Table 6
 */
#define DATA_INTERFACE_CLASS		0x0A

/** Data Class Protocol Codes */
#define NCM_NTB_PROTOCOL		0x01

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
#define NCM_FUNC_DESC			0x1A

/**
 * @brief bDescriptor SubType for NCM Functional Descriptors
 * @note NCM11_20260205.pdf, 5.4, Table 5-4
 */
#define NCM_EXT_CAPABILITY_DESC	0x1D
#define NCM_EXT_FEATURE_DESC	0x1E

/**
 * @brief Communications Class Notification Codes
 * @note CDC120-20101103-track.pdf, 6.3, Table 20
 */

/** Network connection notification */
#define USB_CDC_NETWORK_CONNECTION	0x00
/** Response available notification */
#define USB_CDC_RESPONSE_AVAILABLE	0x01
/** Connection speed change notification */
#define USB_CDC_CONNECTION_SPEED_CHANGE	0x2A

/**
 * @brief PSTN Subclass Specific Requests
 * for ACM devices
 * @note PSTN120.pdf, 6.3, Table 13
 */
#define CDC_SEND_ENC_CMD		0x00
#define CDC_GET_ENC_RSP			0x01
#define SET_LINE_CODING			0x20
#define GET_LINE_CODING			0x21
#define SET_CONTROL_LINE_STATE		0x22

/**
 * @brief PSTN Subclass Class-Specific Notification Codes
 * @note PSTN120.pdf, 6.5, Table 30
 */

/** Auxiliary jack hook state notification */
#define USB_CDC_AUX_JACK_HOOK_STATE	0x08
/** Ring detect notification */
#define USB_CDC_RING_DETECT		0x09
/** Serial state notification */
#define USB_CDC_SERIAL_STATE		0x20
/** Line state change notification */
#define USB_CDC_LINE_STATE_CHANGE	0x23
/** Call state change notification */
#define USB_CDC_CALL_STATE_CHANGE	0x28

/**
 * @brief PSTN UART State Bitmap Values
 * @note PSTN120.pdf, 6.5.4, Table 31
 */
#define USB_CDC_SERIAL_STATE_OVERRUN	BIT(6)
#define USB_CDC_SERIAL_STATE_PARITY	BIT(5)
#define USB_CDC_SERIAL_STATE_FRAMING	BIT(4)
#define USB_CDC_SERIAL_STATE_RINGSIGNAL	BIT(3)
#define USB_CDC_SERIAL_STATE_BREAK	BIT(2)
#define USB_CDC_SERIAL_STATE_TXCARRIER	BIT(1)
#define USB_CDC_SERIAL_STATE_RXCARRIER	BIT(0)

/** Control Signal Bitmap Values for SetControlLineState */
#define SET_CONTROL_LINE_STATE_RTS	0x02
#define SET_CONTROL_LINE_STATE_DTR	0x01

/** Enhance enum uart_line_ctrl with CDC specific values */
#define USB_CDC_LINE_CTRL_BAUD_RATE	UART_LINE_CTRL_BAUD_RATE
#define USB_CDC_LINE_CTRL_DCD		UART_LINE_CTRL_DCD
#define USB_CDC_LINE_CTRL_DSR		UART_LINE_CTRL_DSR
#define USB_CDC_LINE_CTRL_BREAK		BIT(5)
#define USB_CDC_LINE_CTRL_RING_SIGNAL	BIT(6)
#define USB_CDC_LINE_CTRL_FRAMING	BIT(7)
#define USB_CDC_LINE_CTRL_PARITY	BIT(8)
#define USB_CDC_LINE_CTRL_OVER_RUN	BIT(9)

/** UART State Bitmap Values */
#define SERIAL_STATE_OVER_RUN		0x40
#define SERIAL_STATE_PARITY		0x20
#define SERIAL_STATE_FRAMING		0x10
#define SERIAL_STATE_RING_SIGNAL	0x08
#define SERIAL_STATE_BREAK		0x04
#define SERIAL_STATE_TX_CARRIER		0x02
#define SERIAL_STATE_RX_CARRIER		0x01

/**
 * @brief PSTN Subclass Line Coding Values
 *
 * @note PSTN120.pdf, 6.3.11, Table 17
 */
#define USB_CDC_LINE_CODING_STOP_BITS_1		0
#define USB_CDC_LINE_CODING_STOP_BITS_1_5	1
#define USB_CDC_LINE_CODING_STOP_BITS_2		2

#define USB_CDC_LINE_CODING_PARITY_NO		0
#define USB_CDC_LINE_CODING_PARITY_ODD		1
#define USB_CDC_LINE_CODING_PARITY_EVEN		2
#define USB_CDC_LINE_CODING_PARITY_MARK		3
#define USB_CDC_LINE_CODING_PARITY_SPACE	4

#define USB_CDC_LINE_CODING_DATA_BITS_5		5
#define USB_CDC_LINE_CODING_DATA_BITS_6		6
#define USB_CDC_LINE_CODING_DATA_BITS_7		7
#define USB_CDC_LINE_CODING_DATA_BITS_8		8

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

/**
 * @brief ECM Subclass Ethernet Statistics Feature Selector Codes
 * @note ECM120.pdf, 6.2.5, Table 9
 */

/** Reserved for future use */
#define USB_CDC_ECM_STAT_RESERVED		0x00
/** Frames transmitted without errors */
#define USB_CDC_ECM_STAT_XMIT_OK		0x01
/** Frames received without errors */
#define USB_CDC_ECM_STAT_RCV_OK			0x02
/** Frames not transmitted, or transmitted with errors */
#define USB_CDC_ECM_STAT_XMIT_ERROR		0x03
/** Frames received with errors */
#define USB_CDC_ECM_STAT_RCV_ERROR		0x04
/** Frames missed, no buffers */
#define USB_CDC_ECM_STAT_RCV_NO_BUFFER		0x05
/** Directed bytes transmitted without errors */
#define USB_CDC_ECM_STAT_DIRECTED_BYTES_XMIT	0x06
/** Directed frames transmitted without errors */
#define USB_CDC_ECM_STAT_DIRECTED_FRAMES_XMIT	0x07
/** Multicast bytes transmitted without errors */
#define USB_CDC_ECM_STAT_MULTICAST_BYTES_XMIT	0x08
/** Multicast frames transmitted without errors */
#define USB_CDC_ECM_STAT_MULTICAST_FRAMES_XMIT	0x09
/** Broadcast bytes transmitted without errors */
#define USB_CDC_ECM_STAT_BROADCAST_BYTES_XMIT	0x0A
/** Broadcast frames transmitted without errors */
#define USB_CDC_ECM_STAT_BROADCAST_FRAMES_XMIT	0x0B
/** Directed bytes received without errors */
#define USB_CDC_ECM_STAT_DIRECTED_BYTES_RCV	0x0C
/** Directed frames received without errors */
#define USB_CDC_ECM_STAT_DIRECTED_FRAMES_RCV	0x0D
/** Multicast bytes received without errors */
#define USB_CDC_ECM_STAT_MULTICAST_BYTES_RCV	0x0E
/** Multicast frames received without errors */
#define USB_CDC_ECM_STAT_MULTICAST_FRAMES_RCV	0x0F
/** Broadcast bytes received without errors */
#define USB_CDC_ECM_STAT_BROADCAST_BYTES_RCV	0x10
/** Broadcast frames received without errors */
#define USB_CDC_ECM_STAT_BROADCAST_FRAMES_RCV	0x11
/** Frames received with circular redundancy check (CRC) or frame check sequence (FCS) error */
#define USB_CDC_ECM_STAT_RCV_CRC_ERROR		0x12
/** Length of transmit queue */
#define USB_CDC_ECM_STAT_TRANSMIT_QUEUE_LENGTH	0x13
/** Frames received with alignment error */
#define USB_CDC_ECM_STAT_RCV_ERROR_ALIGNMENT	0x14
/** Frames transmitted with one collision */
#define USB_CDC_ECM_STAT_XMIT_ONE_COLLISION	0x15
/** Frames transmitted with more than one collision */
#define USB_CDC_ECM_STAT_XMIT_MORE_COLLISIONS	0x16
/** Frames transmitted after deferral */
#define USB_CDC_ECM_STAT_XMIT_DEFERRED		0x17
/** Frames not transmitted due to collisions */
#define USB_CDC_ECM_STAT_XMIT_MAX_COLLISIONS	0x18
/** Frames not received due to overrun */
#define USB_CDC_ECM_STAT_RCV_OVERRUN		0x19
/** Frames not transmitted due to underrun */
#define USB_CDC_ECM_STAT_XMIT_UNDERRUN		0x1A
/** Frames transmitted with heartbeat failure */
#define USB_CDC_ECM_STAT_XMIT_HEARTBEAT_FAILURE	0x1B
/** Times carrier sense signal lost during transmission */
#define USB_CDC_ECM_STAT_XMIT_TIMES_CRS_LOST	0x1C
/** Late collisions detected */
#define USB_CDC_ECM_STAT_XMIT_LATE_COLLISIONS	0x1D

/**
 * @brief Class-Specific Request Codes for NCM 1.0 subclass
 * @note NCM11_20260205.pdf, 7.2, Table 7-2
 */
#define GET_NTB_PARAMETERS	0x80
#define GET_NET_ADDRESS		0x81
#define SET_NET_ADDRESS		0x82
#define GET_NTB_FORMAT		0x83
#define SET_NTB_FORMAT		0x84
#define GET_NTB_INPUT_SIZE	0x85
#define SET_NTB_INPUT_SIZE	0x86
#define GET_MAX_DATAGRAM_SIZE	0x87
#define SET_MAX_DATAGRAM_SIZE	0x88
#define GET_CRC_MODE		0x89
#define SET_CRC_MODE 		0x8A

/**
 * @brief Class-Specific Request Codes for NCM 1.1 subclass
 * @note NCM11_20260205.pdf, 8.2, Table 8-2
 */
#define GET_EXTENDED_CAPABILITY_MODE	0x8B
#define SET_EXTENDED_CAPABILITY_MODE	0x8C
#define GET_EXTENDED_CAPABILTIES	0x8D
#define GET_EXTENDED_FEATURE		0x8E
#define	SET_EXTENDED_FEATURE		0x8F

/**
 * @name Network Capabilities Bitmap of the NCM Functional Descriptor
 * @note NCM11_20260205.pdf, 6.2.1, Table 6-2
 * @{
 */
/**
 * Function can process SetEthernetPacketFilter requests, as defined in [USBECM12].
 * If not set, broadcast, directed and multicast packets are always passed to the host.
 */
#define USB_CDC_NCM_NCAP_ETH_FILTER		BIT(0)
/** Function can process GetNetAddress and SetNetAddress requests */
#define USB_CDC_NCM_NCAP_NET_ADDRESS		BIT(1)
/** Function can process SendEncapsulatedCommand and GetEncapsulatedResponse requests */
#define USB_CDC_NCM_NCAP_ENCAP_COMMAND		BIT(2)
/** Function can process SetMaxDatagramSize and GetMaxDatagramSize requests */
#define USB_CDC_NCM_NCAP_MAX_DATAGRAM_SIZE	BIT(3)
/** Function can process SetCrcMode and GetCrcMode requests */
#define USB_CDC_NCM_NCAP_CRC_MODE		BIT(4)
/** Function can process 8-byte forms of GetNtbInputSize and SetNtbInputSize requests */
#define USB_CDC_NCM_NCAP_NTB_INPUT_SIZE		BIT(5)
/** Function supports extended NCM capabilities (NCM 1.1) */
#define USB_CDC_NCM_NCAP_EXTENDED_CAPABILITIES	BIT(7)
/** @} */

/** bmNtbFormatsSupported bits (GetNtbParameters) */
#define USB_CDC_NCM_NTB16_SUPPORTED	BIT(0)
#define USB_CDC_NCM_NTB32_SUPPORTED	BIT(1)

/** NTB format selection values (GetNtbFormat/SetNtbFormat) */
#define USB_CDC_NCM_NTB16_FORMAT	0x0000
#define USB_CDC_NCM_NTB32_FORMAT	0x0001

/** NTB format CRC mode values (GetCrcMode/SetCrcMode) */
#define USB_CDC_NCM_CRC_NOT_APPENDED	0x0000
#define USB_CDC_NCM_CRC_APPENDED	0x0001

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

/** NCM Functional Descriptor */
struct cdc_ncm_descriptor {
	uint8_t bFunctionLength;
	uint8_t bDescriptorType;
	uint8_t bDescriptorSubtype;
	uint16_t bcdNcmVersion;
	uint8_t bmNetworkCapabilities;
} __packed;

/** NCM Transfer Header for 16-bit NTBs (NTH16) */
struct usb_cdc_ncm_nth16 {
	uint32_t dwSignature;
	uint16_t wHeaderLength;
	uint16_t wSequence;
	uint16_t wBlockLength;
	uint16_t wNdpIndex;
} __packed;

/** NCM Transfer Header for 32-bit NTBs (NTH32) */
struct usb_cdc_ncm_nth32 {
	uint32_t dwSignature;
	uint16_t wHeaderLength;
	uint16_t wSequence;
	uint32_t wBlockLength;
	uint32_t wNdpIndex;
} __packed;

/** Datagram Pointer Entry used by NDP16 (DPE16) */
struct usb_cdc_ncm_ndp16_dpe {
	uint16_t wDatagramIndex;
	uint16_t wDatagramLength;
} __packed;

/** NTB Parameter Structure */
struct usb_cdc_ncm_ntb_parameters {
	uint16_t wLength;
	uint16_t bmNtbFormatsSupported;
	uint32_t dwNtbInMaxSize;
	uint16_t wNdpInDivisor;
	uint16_t wNdpInPayloadRemainder;
	uint16_t wNdpInAlignment;
	uint16_t wReserved;
	uint32_t dwNtbOutMaxSize;
	uint16_t wNdpOutDivisor;
	uint16_t wNdpOutPayloadRemainder;
	uint16_t wNdpOutAlignment;
	uint16_t wNtbOutMaxDatagrams;
} __packed;

/**
 * NTB Input Size Structure, Get/SetNtbInputSize 8-byte form (NCM11_20260205.pdf, 7.2.7,
 * Table 7-4)
 */
struct usb_cdc_ncm_ntb_input_size {
	uint32_t dwNtbInMaxSize;
	uint16_t wNtbInMaxDatagrams;
	uint16_t wReserved;
} __packed;

#endif /* ZEPHYR_INCLUDE_USB_CLASS_USB_CDC_H_ */
