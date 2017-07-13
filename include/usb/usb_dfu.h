/* usb_cdc.h - USB DFU public header */

/*
 * Copyright (c) 2017 PHYTEC Messtechnik GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */


/**
 * @file
 * @brief USB Device Firmware Upgrade (DFU) public header
 *
 * Header follows the Device Class Specification for
 * Device Firmware Upgrade Version 1.1
 */

#ifndef __USB_DFU_H__
#define __USB_DFU_H__

/** DFU Class */
#define DFU_DEVICE_CLASS		0xFE

/** DFU Class Subclass */
#define DFU_SUBCLASS			0x01

/** DFU Class runtime Protocol */
#define DFU_RT_PROTOCOL			0x01

/** DFU Class DFU mode Protocol */
#define DFU_MODE_PROTOCOL		0x02

/**
 * @brief DFU Class Specific Requests
 */
#define DFU_DETACH			0x00
#define DFU_DNLOAD			0x01
#define DFU_UPLOAD			0x02
#define DFU_GETSTATUS			0x03
#define DFU_CLRSTATUS			0x04
#define DFU_GETSTATE			0x05
#define DFU_ABORT			0x06

/** DFU FUNCTIONAL descriptor type */
#define DFU_FUNC_DESC			0x21

/** DFU attributes DFU Functional Descriptor */
#define DFU_ATTR_WILL_DETACH		0x08
#define DFU_ATTR_MANIFESTATION_TOLERANT	0x04
#define DFU_ATTR_CAN_UPLOAD		0x02
#define DFU_ATTR_CAN_DNLOAD		0x01

/** DFU Specification release */
#define DFU_VERSION			0x0110

/** Run-Time Functional Descriptor */
struct dfu_runtime_descriptor {
	u8_t bLength;
	u8_t bDescriptorType;
	u8_t bmAttributes;
	u16_t wDetachTimeOut;
	u16_t wTransferSize;
	u16_t bcdDFUVersion;
} __packed;

/** bStatus values for the DFU_GETSTATUS response */
enum dfu_status {
	statusOK,
	errTARGET,
	errFILE,
	errWRITE,
	errERASE,
	errCHECK_ERASED,
	errPROG,
	errVERIFY,
	errADDRESS,
	errNOTDONE,
	errFIRMWARE,
	errVENDOR,
	errUSB,
	errPOR,
	errUNKNOWN,
	errSTALLEDPKT
};

/** bState values for the DFU_GETSTATUS response */
enum dfu_state {
	appIDLE,
	appDETACH,
	dfuIDLE,
	dfuDNLOAD_SYNC,
	dfuDNBUSY,
	dfuDNLOAD_IDLE,
	dfuMANIFEST_SYNC,
	dfuMANIFEST,
	dfuMANIFEST_WAIT_RST,
	dfuUPLOAD_IDLE,
	dfuERROR,
};

#endif /* __USB_DFU_H__ */
