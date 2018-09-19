/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

struct op_common {
	u16_t version;
	u16_t code;
	u32_t status;
} __packed;

struct devlist_device {
	char path[256];
	char busid[32];

	u32_t busnum;
	u32_t devnum;
	u32_t speed;

	u16_t idVendor;
	u16_t idProduct;
	u16_t bcdDevice;

	u8_t bDeviceClass;
	u8_t bDeviceSubClass;
	u8_t bDeviceProtocol;
	u8_t bConfigurationValue;
	u8_t bNumConfigurations;
	u8_t bNumInterfaces;
} __packed;

#define OP_REQUEST		(0x80 << 8)
#define OP_REPLY		(0x00 << 8)

/* Devlist */
#define OP_DEVLIST		0x05
#define OP_REQ_DEVLIST		(OP_REQUEST | OP_DEVLIST)
#define OP_REP_DEVLIST		(OP_REPLY | OP_DEVLIST)

/* Import USB device */
#define OP_IMPORT		0x03
#define OP_REQ_IMPORT		(OP_REQUEST | OP_IMPORT)
#define OP_REP_IMPORT		(OP_REPLY | OP_IMPORT)

/* USBIP requests */
#define USBIP_CMD_SUBMIT	0x0001
#define USBIP_CMD_UNLINK	0x0002
#define USBIP_RET_SUBMIT	0x0003
#define USBIP_RET_UNLINK	0x0004

/* USBIP direction */
#define USBIP_DIR_OUT		0x00
#define USBIP_DIR_IN		0x01

struct usbip_header_common {
	u32_t command;
	u32_t seqnum;
	u32_t devid;
	u32_t direction;
	u32_t ep;
} __packed;

struct usbip_submit {
	u32_t transfer_flags;
	s32_t transfer_buffer_length;
	s32_t start_frame;
	s32_t number_of_packets;
	s32_t interval;
} __packed;

struct usbip_unlink {
	u32_t seqnum;
} __packed;

struct usbip_submit_rsp {
	struct usbip_header_common common;

	s32_t status;
	s32_t actual_length;
	s32_t start_frame;
	s32_t number_of_packets;
	s32_t error_count;

	u64_t setup;
} __packed;

struct usbip_header {
	struct usbip_header_common common;

	union {
		struct usbip_submit submit;
		struct usbip_unlink unlink;
	} u;
} __packed;

/* Function definitions */

int usbip_recv(u8_t *buf, size_t len);
int usbip_send_common(u8_t ep, u32_t data_len);
int usbip_send(u8_t ep, const u8_t *data, size_t len);

void usbip_start(void);

int handle_usb_control(struct usbip_header *hdr);
int handle_usb_data(struct usbip_header *hdr);
