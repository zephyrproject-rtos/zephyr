/* usb_cp210x.h - USB CP210x public header */

/*
 * Copyright (c) 2025 Sergey Matsievskiy
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>
#include <zephyr/kernel.h>

/**
 * @file
 * @brief USB CP210x public header
 *
 * Header uses information from Silicon Laboratories's
 * CP210X Virtual COM Port Interface AN571 specification.
 */

#ifndef ZEPHYR_INCLUDE_USB_CLASS_USB_CP210X_H_
#define ZEPHYR_INCLUDE_USB_CLASS_USB_CP210X_H_

/**
 * @brief CP210x Virtual COM Port Interface Control Commands
 * @note AN571, 5
 */
#define USB_CP210X_IFC_ENABLE		0x00
#define USB_CP210X_SET_BAUDDIV		0x01
#define USB_CP210X_GET_BAUDDIV		0x02
#define USB_CP210X_SET_LINE_CTL		0x03
#define USB_CP210X_GET_LINE_CTL		0x04
#define USB_CP210X_SET_BREAK		0x05
#define USB_CP210X_IMM_CHAR		0x06
#define USB_CP210X_SET_MHS		0x07
#define USB_CP210X_GET_MDMSTS		0x08
#define USB_CP210X_SET_XON		0x09
#define USB_CP210X_SET_XOFF		0x0A
#define USB_CP210X_SET_EVENTMASK	0x0B
#define USB_CP210X_GET_EVENTMASK	0x0C
#define USB_CP210X_SET_CHAR		0x0D
#define USB_CP210X_GET_CHARS		0x0E
#define USB_CP210X_GET_PROPS		0x0F
#define USB_CP210X_GET_COMM_STATUS	0x10
#define USB_CP210X_RESET		0x11
#define USB_CP210X_PURGE		0x12
#define USB_CP210X_SET_FLOW		0x13
#define USB_CP210X_GET_FLOW		0x14
#define USB_CP210X_EMBED_EVENTS		0x15
#define USB_CP210X_GET_EVENTSTATE	0x16
#define USB_CP210X_SET_RECEIVE		0x17
#define USB_CP210X_GET_RECEIVE		0x18
#define USB_CP210X_SET_CHARS		0x19
#define USB_CP210X_GET_BAUDRATE		0x1D
#define USB_CP210X_SET_BAUDRATE		0x1E
#define USB_CP210X_VENDOR_SPECIFIC	0xFF

/**
 * @brief CP210x Virtual COM Port Interface Line Control
 * @note AN571, 5.1 IFC_ENABLE
 */
#define USB_CP210X_ENABLE		0x0001
#define USB_CP210X_DISABLE		0x0000

/**
 * @brief CP210x Virtual COM Port Baud Divisor
 * @note AN571, 5.3 SET_BAUDDIV; 5.4 GET_BAUDDIV
 */
#define USB_CP210X_BAUDDIV_FREQ		3686400

/**
 * @brief CP210x Virtual COM Port Interface Line Control
 * @note AN571, 5.7 SET_LINE_CTL
 */
#define USB_CP210X_BITS_DATA		0xff00
#define USB_CP210X_BITS_DATA_5		5
#define USB_CP210X_BITS_DATA_6		6
#define USB_CP210X_BITS_DATA_7		7
#define USB_CP210X_BITS_DATA_8		8

#define USB_CP210X_BITS_PARITY		0x00f0
#define USB_CP210X_BITS_PARITY_NONE	0
#define USB_CP210X_BITS_PARITY_ODD	1
#define USB_CP210X_BITS_PARITY_EVEN	2
#define USB_CP210X_BITS_PARITY_MARK	3
#define USB_CP210X_BITS_PARITY_SPACE	4

#define USB_CP210X_BITS_STOP		0x000f
#define USB_CP210X_BITS_STOP_1		0
#define USB_CP210X_BITS_STOP_1_5	1
#define USB_CP210X_BITS_STOP_2		2

union usb_cp210x_line_ctl {
	struct {
		uint8_t stop_bits: 4;
		uint8_t parity: 4;
		uint8_t word_length;
	} __packed fld;
	uint16_t val;
};

BUILD_ASSERT(sizeof(union usb_cp210x_line_ctl) == sizeof(uint16_t),
	     "usb_cp210x_line_ctl inconsistent");

/**
 * @brief CP210x Virtual COM Port Interface Line Control
 * @note AN571, 5.20 SET_BREAK
 */
#define USB_CP210X_BREAK_ON		0x0001
#define USB_CP210X_BREAK_OFF		0x0000

/**
 * @brief CP210x Virtual COM Modem Status
 * @note AN571, 5.9 SET_MHS
 */
union usb_cp210x_mhs {
	struct {
		bool dtr_state: 1;
		bool rts_state: 1;
		uint8_t: 6;
		bool dtr_mask: 1;
		bool rts_mask: 1;
	} __packed fld;
	uint16_t val;
};

BUILD_ASSERT(sizeof(union usb_cp210x_mhs) == sizeof(uint16_t),
	     "usb_cp210x_mhs inconsistent");

/**
 * @brief CP210x Virtual COM Modem Status Report
 * @note AN571, 5.10 GET_MDMSTS
 */
union usb_cp210x_mdmsts {
	struct {
		bool dtr: 1;
		bool rts: 1;
		uint8_t: 2;
		bool cts: 1;
		bool dsr: 1;
		bool ri: 1;
		bool dcd: 1;
	} __packed fld;
	uint8_t val;
};

BUILD_ASSERT(sizeof(union usb_cp210x_mdmsts) == sizeof(uint8_t),
	     "usb_cp210x_mhs inconsistent");

/**
 * @brief CP210x Virtual COM Purge
 * @note AN571, 5.27 PURGE
 */
union usb_cp210x_purge {
	struct {
		bool tx1: 1;
		bool rx1: 1;
		bool tx2: 1;
		bool rx2: 1;
	} __packed fld;
	uint8_t val;
};

BUILD_ASSERT(sizeof(union usb_cp210x_purge) == sizeof(uint8_t),
	     "usb_cp210x_purge inconsistent");

/**
 * @brief CP210x Virtual COM Vendor Specific
 */
#define USB_CP210X_GET_FW_VER		0x000E
#define USB_CP210X_READ_2NCONFIG	0x000E
#define USB_CP210X_GET_FW_VER_2N	0x0010
#define USB_CP210X_READ_LATCH		0x00C2
#define USB_CP210X_GET_PARTNUM		0x370B
#define USB_CP210X_GET_PORTCONFIG	0x370C
#define USB_CP210X_GET_DEVICEMODE	0x3711
#define USB_CP210X_WRITE_LATCH		0x37E1

/**
 * @brief CP210x Virtual COM Vendor Part Number Definitions
 */
#define USB_CP210X_PARTNUM_CP2101		0x01
#define USB_CP210X_PARTNUM_CP2102		0x02
#define USB_CP210X_PARTNUM_CP2103		0x03
#define USB_CP210X_PARTNUM_CP2104		0x04
#define USB_CP210X_PARTNUM_CP2105		0x05
#define USB_CP210X_PARTNUM_CP2108		0x08
#define USB_CP210X_PARTNUM_CP2102N_QFN28	0x20
#define USB_CP210X_PARTNUM_CP2102N_QFN24	0x21
#define USB_CP210X_PARTNUM_CP2102N_QFN20	0x22

/**
 * @brief CP210x Virtual COM Modem Status
 * @note AN571, 6, Table 10 Control Handshake
 */
#define USB_CP210X_FCS_DTR_MASK_INACTIVE	0x0
#define USB_CP210X_FCS_DTR_MASK_ACTIVE		0x1
#define USB_CP210X_FCS_DTR_MASK_DEV		0x2

union usb_cp210x_ul_control_handshake {
	struct {
		uint8_t dtr_mask: 2;
		bool: 1;
		bool cts_hsk: 1;
		bool dsr_hsk: 1;
		bool dsd_hsk: 1;
		bool dsr_sens: 1;
	} __packed fld;
	uint32_t val;
};

BUILD_ASSERT(sizeof(union usb_cp210x_ul_control_handshake) == sizeof(uint32_t),
	     "usb_cp210x_mhs inconsistent");

/**
 * @brief CP210x Virtual COM Modem Status
 * @note AN571, 6, Table 11 Flow Replace
 */
#define USB_CP210X_RTS_MASK_INACTIVE		0x0
#define USB_CP210X_RTS_MASK_ACTIVE		0x1
#define USB_CP210X_RTS_MASK_RCV			0x2
#define USB_CP210X_RTS_MASK_TRNS		0x3

union usb_cp210x_ul_flow_replace {
	struct {
		bool auto_transmit: 1;
		bool auto_receive: 1;
		bool error_char: 1;
		bool null_stripping: 1;
		bool break_char: 1;
		bool: 1;
		bool: 1;
		uint8_t rts_mask: 1;
		uint8_t: 7;
		uint16_t: 15;
		bool xoff_continue: 1;
	} __packed fld;
	uint32_t val;
};

BUILD_ASSERT(sizeof(union usb_cp210x_ul_flow_replace) == sizeof(uint32_t),
	     "usb_cp210x_ul_flow_replace inconsistent");

/**
 * @brief CP210x Virtual COM Modem Status
 * @note AN571, 6, Table 9 Flow Control State
 */
struct usb_cp210x_flow_control {
	union usb_cp210x_ul_control_handshake ulControlHandshake;
	union usb_cp210x_ul_flow_replace ulFlowReplace;
	uint32_t ulXonLimit;
	uint32_t ulXoffLimit;
} __packed;

BUILD_ASSERT(sizeof(struct usb_cp210x_flow_control) == 4 * sizeof(uint32_t),
	     "usb_cp210x_flow_control inconsistent");

/**
 * @brief CP210x Virtual COM Event
 * @note AN571, 5.15 SET_EVENTMASK
 */
union usb_cp210x_event {
	struct {
		bool ri_trailing_edge_occurred: 1;
		bool: 1;
		bool rcv_buf_80pct_full: 1;
		uint8_t: 5;
		bool char_received: 1;
		bool special_char_received: 1;
		bool transmit_queue_empty: 1;
		bool cts_changed: 1;
		bool dsr_changed: 1;
		bool dsd_changed: 1;
		bool line_break_received: 1;
		bool line_status_error_occurred: 1;
	} __packed fld;
	uint16_t val;
};

BUILD_ASSERT(sizeof(union usb_cp210x_event) == sizeof(uint16_t),
	     "usb_cp210x_event inconsistent");

/**
 * @brief CP210x Virtual COM Char
 * @note AN571, 5.23 SET_CHAR
 */
struct usb_cp210x_char {
	uint8_t char_idx;
	uint8_t char_val;
} __packed;

BUILD_ASSERT(sizeof(struct usb_cp210x_char) == sizeof(uint16_t),
	     "usb_cp210x_char inconsistent");

union usb_cp210x_char_vals {
	struct {
		uint8_t eof;
		uint8_t error;
		uint8_t brk;
		uint8_t event;
		uint8_t xon;
		uint8_t xoff;
	} __packed fld;
	uint8_t chr[6];
};

BUILD_ASSERT(sizeof(union usb_cp210x_char_vals) == 6 * sizeof(uint8_t),
	     "usb_cp210x_char_vals inconsistent");

/**
 * @brief CP210x Virtual COM Communication Properties
 * @note AN571, 6, Table 7 Communication Properties
 */
#define USB_CP210X_PROPS_BSD_VERSION			0x0100
#define USB_CP210X_PROPS_SERVICE_MASK			1
#define USB_CP210X_PROPS_MAX_BAUD			115200
#define USB_CP210X_PROPS_PROVSUBTYPE_UNSPECIFIED	0
#define USB_CP210X_PROPS_PROVSUBTYPE_RS232		1
#define USB_CP210X_PROPS_PROVSUBTYPE_MODEM		6

struct usb_cp210x_props {
	uint16_t wLength;
	uint16_t bcdVersion;
	uint32_t ulServiceMask;
	uint32_t: 32;
	uint32_t ulMaxTxQueue;
	uint32_t ulMaxRxQueue;
	uint32_t ulMaBaud;
	uint32_t ulProvSubType;
	union {
		struct {
			bool dtr_dsr_support: 1;
			bool rts_cts_support: 1;
			bool dcd_support: 1;
			bool can_check_parity: 1;
			bool xon_xoff_support: 1;
			bool can_set_xon_xoff_characters: 1;
			bool: 1;
			bool: 1;
			bool can_set_special_characters: 1;
			bool bit16_mode_supports: 1;
		} __packed fld;
		uint32_t val;
	} ulProvCapabilities;
	union {
		struct {
			bool can_set_parity_type: 1;
			bool can_set_baud: 1;
			bool can_set_number_of_data_bits: 1;
			bool can_set_stop_bits: 1;
			bool can_set_handshaking: 1;
			bool can_set_parity_checking: 1;
			bool can_set_carrier_detect_checking: 1;
		} __packed fld;
		uint32_t val;
	} ulSettableParams;
	union {
		struct {
			bool baud_75: 1;
			bool baud_110: 1;
			bool baud_134_5: 1;
			bool baud_150: 1;
			bool baud_300: 1;
			bool baud_600: 1;
			bool baud_1200: 1;
			bool baud_1800: 1;
			bool baud_2400: 1;
			bool baud_4800: 1;
			bool baud_7200: 1;
			bool baud_9600: 1;
			bool baud_14400: 1;
			bool baud_19200: 1;
			bool baud_38400: 1;
			bool baud_56000: 1;
			bool baud_128000: 1;
			bool baud_115200: 1;
			bool baud_57600: 1;
		} __packed fld;
		uint32_t val;
	} ulSettableBaud;
	union {
		struct {
			bool data_bits_5: 1;
			bool data_bits_6: 1;
			bool data_bits_7: 1;
			bool data_bits_8: 1;
			bool data_bits_16: 1;
			bool data_bits_16_extended: 1;
		} __packed fld;
		uint32_t val;
	} wSettableData;
	uint32_t ulCurrentTxQueue;
	uint32_t ulCurrentRxQueue;
	uint32_t: 32;
	uint32_t: 32;
	char uniProvName[15];
} __packed;

BUILD_ASSERT(sizeof(struct usb_cp210x_props) == 75 * sizeof(uint8_t),
	     "usb_cp210x_props inconsistent");

/**
 * @brief CP210x Virtual COM Serial Status Response
 * @note AN571, 6, Table 8 Serial Status Response
 */
struct usb_cp210x_serial_status {
	union {
		struct {
			bool break_event: 1;
			bool framing_error: 1;
			bool hardware_overrun: 1;
			bool queue_overrun: 1;
			bool parity_error: 1;
		} __packed fld;
		uint32_t val;
	} ulErrors;
	union {
		struct {
			bool wait_cts: 1;
			bool wait_dsr: 1;
			bool wait_dsd: 1;
			bool wait_xon: 1;
			bool wait_xoff: 1;
			bool wait_break: 1;
			bool wait_dsr_rcv: 1;
		} __packed fld;
		uint32_t val;
	} ulHoldReasons;
	uint32_t ulAmountInInQueue;
	uint32_t ulAmountInOutQueue;
	uint8_t bEofReceived;
	uint8_t bWaitForImmediate;
	uint8_t: 8;
} __packed;

BUILD_ASSERT(sizeof(struct usb_cp210x_serial_status) == 19 * sizeof(uint8_t),
	     "usb_cp210x_serial_status inconsistent");

#endif /* ZEPHYR_INCLUDE_USB_CLASS_USB_CP210X_H_ */
