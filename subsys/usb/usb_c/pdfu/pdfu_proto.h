/*
 * Copyright (c) 2026 Meta Platforms, Inc.
 * SPDX-License-Identifier: Apache-2.0
 *
 * PDFU wire format shared by the responder and the initiator: USB PD Firmware
 * Update Specification, Revision 1.0, sections 5.2 and 5.3.
 */

#ifndef ZEPHYR_SUBSYS_USB_USB_C_PDFU_PROTO_H_
#define ZEPHYR_SUBSYS_USB_USB_C_PDFU_PROTO_H_

/* Table 5-2 */
#define PDFU_PROTOCOL_V1_0 0x01

/* Table 5-3, Requests */
#define PDFU_REQ_GET_FW_ID  0x81
#define PDFU_REQ_INITIATE   0x82
#define PDFU_REQ_DATA       0x83
#define PDFU_REQ_DATA_NR    0x84
#define PDFU_REQ_VALIDATE   0x85
#define PDFU_REQ_ABORT      0x86
#define PDFU_REQ_DATA_PAUSE 0x87
#define PDFU_REQ_VENDOR     0xFF

/* Table 5-16, Responses */
#define PDFU_RSP_GET_FW_ID  0x01
#define PDFU_RSP_INITIATE   0x02
#define PDFU_RSP_DATA       0x03
#define PDFU_RSP_VALIDATE   0x05
#define PDFU_RSP_DATA_PAUSE 0x07
#define PDFU_RSP_VENDOR     0x7F

/* Table 5-29, Status */
#define PDFU_OK                     0x00
#define PDFU_ERR_TARGET             0x01
#define PDFU_ERR_FILE               0x02
#define PDFU_ERR_WRITE              0x03
#define PDFU_ERR_ADDRESS            0x08
#define PDFU_ERR_NOTDONE            0x09
#define PDFU_ERR_UNKNOWN            0x0E
#define PDFU_ERR_UNEXPECTED_REQUEST 0x82
#define PDFU_ERR_REJECT_PAUSE       0x83

/* A Data Block is always 256 bytes except the last one, section 5.2.3. */
#define PDFU_DATA_BLOCK_LEN 256

/* WaitTime = 255 means the responder cannot continue, Tables 5-20 and 5-22. */
#define PDFU_WAIT_STOP 255

#endif /* ZEPHYR_SUBSYS_USB_USB_C_PDFU_PROTO_H_ */
