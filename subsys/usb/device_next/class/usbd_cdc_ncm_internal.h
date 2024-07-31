/*
 * Copyright (c) 2024 Hardy Griech
 *
 * SPDX-License-Identifier: Apache-2.0
 */


#ifndef __USBD_CDC_NCM_LOCAL_H_
#define __USBD_CDC_NCM_LOCAL_H_


#ifndef CFG_CDC_NCM_ALIGNMENT
#define CFG_CDC_NCM_ALIGNMENT                 4
#endif
#if (CFG_CDC_NCM_ALIGNMENT != 4)
/*  headers and start of datagrams in structs have to be aligned differently */
#error "CFG_CDC_NCM_ALIGNMENT must be 4"
#endif

#define CFG_CDC_NCM_XMT_MAX_DATAGRAMS_PER_NTB     1
#define CFG_CDC_NCM_RCV_MAX_DATAGRAMS_PER_NTB     1

/* discussion about NTB size: https://github.com/hathach/tinyusb/pull/2227 */
#define CFG_CDC_NCM_XMT_NTB_MAX_SIZE              2048        /* min 2048 according to spec 6.2.7 */
#define CFG_CDC_NCM_RCV_NTB_MAX_SIZE              3200

/* Table 6.2 Class-Specific Request Codes for Network Control Model subclass */
typedef enum {
	NCM_SET_ETHERNET_MULTICAST_FILTERS               = 0x40,
	NCM_SET_ETHERNET_POWER_MANAGEMENT_PATTERN_FILTER = 0x41,
	NCM_GET_ETHERNET_POWER_MANAGEMENT_PATTERN_FILTER = 0x42,
	NCM_SET_ETHERNET_PACKET_FILTER                   = 0x43,
	NCM_GET_ETHERNET_STATISTIC                       = 0x44,
	NCM_GET_NTB_PARAMETERS                           = 0x80,    /* required */
	NCM_GET_NET_ADDRESS                              = 0x81,
	NCM_SET_NET_ADDRESS                              = 0x82,
	NCM_GET_NTB_FORMAT                               = 0x83,
	NCM_SET_NTB_FORMAT                               = 0x84,
	NCM_GET_NTB_INPUT_SIZE                           = 0x85,    /* required according to spec */
	NCM_SET_NTB_INPUT_SIZE                           = 0x86,    /* required according to spec */
	NCM_GET_MAX_DATAGRAM_SIZE                        = 0x87,
	NCM_SET_MAX_DATAGRAM_SIZE                        = 0x88,
	NCM_GET_CRC_MODE                                 = 0x89,
	NCM_SET_CRC_MODE                                 = 0x8A,
} ncm_request_code_t;


/* Table 6.6 Class-Specific Notification Codes for Networking Control Model subclass */
typedef enum {
	NCM_NOTIFICATION_NETWORK_CONNECTION              = 0x00,
	NCM_NOTIFICATION_RESPONSE_AVAILABLE              = 0x01,
	NCM_NOTIFICATION_CONNECTION_SPEED_CHANGE         = 0x2a,
} ncm_notification_code_t;


#define NTH16_SIGNATURE      0x484D434E
#define NDP16_SIGNATURE_NCM0 0x304D434E
#define NDP16_SIGNATURE_NCM1 0x314D434E

/* network endianness = LE! */
struct ntb_parameters_t {
	uint16_t wLength;
	uint16_t bmNtbFormatsSupported;
	uint32_t dwNtbInMaxSize;
	uint16_t wNdbInDivisor;
	uint16_t wNdbInPayloadRemainder;
	uint16_t wNdbInAlignment;
	uint16_t wReserved;
	uint32_t dwNtbOutMaxSize;
	uint16_t wNdbOutDivisor;
	uint16_t wNdbOutPayloadRemainder;
	uint16_t wNdbOutAlignment;
	uint16_t wNtbOutMaxDatagrams;
} __packed;

/* network endianness = LE! */
struct nth16_t {
	uint32_t dwSignature;
	uint16_t wHeaderLength;
	uint16_t wSequence;
	uint16_t wBlockLength;
	uint16_t wNdpIndex;
} __packed;

/* network endianness = LE! */
struct ndp16_datagram_t {
	uint16_t wDatagramIndex;
	uint16_t wDatagramLength;
} __packed;

/* network endianness = LE! */
struct ndp16_t {
	uint32_t dwSignature;
	uint16_t wLength;
	uint16_t wNextNdpIndex;
	/* ndp16_datagram_t datagram[]; */
} __packed;

union xmit_ntb_t {
	struct {
		struct nth16_t          nth;
		struct ndp16_t          ndp;
		struct ndp16_datagram_t ndp_datagram[CFG_CDC_NCM_XMT_MAX_DATAGRAMS_PER_NTB + 1];
	};
	uint8_t data[CFG_CDC_NCM_XMT_NTB_MAX_SIZE];
} __packed;

union recv_ntb_t {
	struct {
		struct nth16_t nth;
		/* only the header is at a guaranteed position */
	};
	uint8_t data[CFG_CDC_NCM_RCV_NTB_MAX_SIZE];
} __packed;

/* network endianness = LE! */
struct ncm_notify_connection_speed_change_t {
	struct usb_setup_packet header;
	uint32_t                downlink, uplink;
} __packed;

/* network endianness = LE! */
struct ncm_notify_network_connection_t {
	struct usb_setup_packet header;
} __packed;


#endif
