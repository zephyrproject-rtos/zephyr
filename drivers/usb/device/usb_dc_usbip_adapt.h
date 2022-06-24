/*
 * Copyright (c) 2018 Intel Corporation
 * Copyright (c) 2022 YuLong Yao<feilongphone@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define __packed __attribute__((__packed__))

#include <stdbool.h>
#include <stdint.h>
#ifdef CONFIG_USB_NATIVE_POSIX
#include <arpa/inet.h>
#include <sys/socket.h>
#else /* CONFIG_USB_NATIVE_POSIX */
#include <zephyr/net/net_ip.h>
#include <zephyr/net/socket.h>
#endif /* CONFIG_USB_NATIVE_POSIX */

struct op_common {
	uint16_t version;
	uint16_t code;
	uint32_t status;
} __packed;

struct devlist_device {
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

#define OP_REQUEST (0x80 << 8)
#define OP_REPLY   (0x00 << 8)

/* Devlist */
#define OP_DEVLIST     0x05
#define OP_REQ_DEVLIST (OP_REQUEST | OP_DEVLIST)
#define OP_REP_DEVLIST (OP_REPLY | OP_DEVLIST)

/* Import USB device */
#define OP_IMPORT     0x03
#define OP_REQ_IMPORT (OP_REQUEST | OP_IMPORT)
#define OP_REP_IMPORT (OP_REPLY | OP_IMPORT)

/* USBIP requests */
#define USBIP_CMD_SUBMIT 0x0001
#define USBIP_CMD_UNLINK 0x0002
#define USBIP_RET_SUBMIT 0x0003
#define USBIP_RET_UNLINK 0x0004

/* USBIP direction */
#define USBIP_DIR_OUT 0x00
#define USBIP_DIR_IN  0x01

struct usbip_header_common {
	uint32_t command;
	uint32_t seqnum;
	uint32_t devid;
	uint32_t direction;
	uint32_t ep;
} __packed;

struct usbip_submit {
	uint32_t transfer_flags;
	int32_t transfer_buffer_length;
	int32_t start_frame;
	int32_t number_of_packets;
	int32_t interval;
	uint8_t bmRequestType;
	uint8_t bRequest;
	uint16_t wValue;
	uint16_t wIndex;
	uint16_t wLength;
} __packed;

struct usbip_unlink {
	uint32_t seqnum;
} __packed;

struct usbip_submit_rsp {
	struct usbip_header_common common;

	int32_t status;
	int32_t actual_length;
	int32_t start_frame;
	int32_t number_of_packets;
	int32_t error_count;

	uint64_t setup;
} __packed;

struct usbip_header {
	struct usbip_header_common common;

	union {
		struct usbip_submit submit;
		struct usbip_unlink unlink;
	} u;
} __packed;

/* Function definitions */

int usbip_recv(uint8_t *buf, size_t len);
bool usbip_send_common(uint8_t ep, uint32_t data_len);
int usbip_send(uint8_t ep, const uint8_t *data, size_t len);

void usbip_start(void);

int handle_usb_control(struct usbip_header *hdr);
int handle_usb_data(struct usbip_header *hdr);

/* Function definitions for socket */
int usbipsocket_socket(void);
int usbipsocket_bind(int sock);
int usbipsocket_connect(int sock, const struct sockaddr *addr,
			socklen_t addrlen);
int usbipsocket_listen(int sock);
int usbipsocket_accept(int sock, struct sockaddr *addr, socklen_t *addrlen);
int usbipsocket_send(int sock, const void *buf, size_t len, int flags);
int usbipsocket_recv(int sock, void *buf, size_t max_len, int flags);
int usbipsocket_setsockopt(int sock, int level, int optname, const void *optval,
			   socklen_t optlen);
int usbipsocket_close(int sock);
