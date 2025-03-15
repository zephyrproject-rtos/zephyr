/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define USBIP_PORT		3240
#define USBIP_VERSION		0x0111U

/* Retrieve the list of exported devices command code */
#define USBIP_OP_REQ_DEVLIST	0x8005U
/* Reply the list of exported devices command code */
#define USBIP_OP_REP_DEVLIST	0x0005U
/* Request to import a remote device command code */
#define USBIP_OP_REQ_IMPORT	0x8003U
/* Reply to import a remote device command code */
#define USBIP_OP_REP_IMPORT	0x0003U

/* Submit an URB command code */
#define USBIP_CMD_SUBMIT	0x0001UL
/* Reply for submitting an URB command code */
#define USBIP_RET_SUBMIT	0x0003UL
/* Unlink an URB command code */
#define USBIP_CMD_UNLINK	0x0002UL
/* Reply for unlink an URB command code */
#define USBIP_RET_UNLINK	0x0004UL

/* Command direction */
#define USBIP_DIR_OUT		0UL
#define USBIP_DIR_IN		1UL

struct usbip_req_header {
	uint16_t version;
	uint16_t code;
	uint32_t status;
} __packed;

struct usbip_devlist_header {
	uint16_t version;
	uint16_t code;
	uint32_t status;
	uint32_t ndev;
} __packed;

struct usbip_devlist_data {
	char path[256];
	char busid[32];

	uint32_t busnum;
	uint32_t devnum;
	uint32_t speed;

	uint16_t idVendor;
	uint16_t idProduct;
	uint16_t bcdDevice;

	uint8_t bDeviceClass;
	uint8_t bDeviceSubClass;
	uint8_t bDeviceProtocol;
	uint8_t bConfigurationValue;
	uint8_t bNumConfigurations;
	uint8_t bNumInterfaces;
} __packed;

struct usbip_devlist_iface_data {
	uint8_t bInterfaceClass;
	uint8_t bInterfaceSubClass;
	uint8_t bInterfaceProtocol;
	uint8_t padding;
} __packed;

struct usbip_cmd_header {
	uint32_t command;
	uint32_t seqnum;
	uint32_t devid;
	uint32_t direction;
	uint32_t ep;
} __packed;

struct usbip_cmd_submit {
	uint32_t flags;
	uint32_t length;
	int32_t start_frame;
	int32_t numof_iso_pkts;
	int32_t interval;
	uint8_t setup[8];
} __packed;

struct usbip_cmd_unlink {
	uint32_t seqnum;
	uint32_t padding[6];
} __packed;

struct usbip_command {
	struct usbip_cmd_header hdr;

	union {
		struct usbip_cmd_submit submit;
		struct usbip_cmd_unlink unlink;
	};
} __packed;

struct usbip_ret_submit {
	int32_t status;
	uint32_t actual_length;
	int32_t start_frame;
	int32_t numof_iso_pkts;
	int32_t error_count;
	uint64_t setup;
} __packed;

struct usbip_ret_unlink {
	int32_t status;
	uint32_t padding[6];
} __packed;

struct usbip_return {
	struct usbip_cmd_header hdr;

	union {
		struct usbip_ret_submit submit;
		struct usbip_ret_unlink unlink;
	};
} __packed;
