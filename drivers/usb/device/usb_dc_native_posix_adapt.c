/*
 * Copyright (c) 2018 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* For accept4() */
#define _GNU_SOURCE 1

#define __packed __attribute__((__packed__))

#include <stdio.h>
#include <stdarg.h>
#include <stdbool.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <arpa/inet.h>

/* Zephyr headers */
#include <kernel.h>
#include <usb/usb_common.h>
#include <usb/usbstruct.h>

#define SYS_LOG_LEVEL CONFIG_SYS_LOG_USB_DRIVER_LEVEL
#define SYS_LOG_DOMAIN "usb/native_posix_adapt"
#include <logging/sys_log.h>

#include <posix_board_if.h>
#include "usb_dc_native_posix_adapt.h"

#define USBIP_PORT	3240
#define USBIP_VERSION	273

#define VERBOSE_DEBUG

int connfd_global;
int seqnum_global;
int devid_global;

/* Helpers */

#ifdef VERBOSE_DEBUG
static void hexdump(const u8_t *data, size_t len)
{
	int n = 0;

	while (len--) {
		if (n % 16 == 0) {
			printf("%08X ", n);
		}

		printf("%02X ", *data++);

		n++;
		if (n % 8 == 0) {
			if (n % 16 == 0) {
				printf("\n");
			} else {
				printf(" ");
			}
		}
	}

	if (n % 16) {
		printf("\n");
	}
}

static void usbip_header_dump(struct usbip_header *hdr)
{
	printf("USBIP header dump:\n");
	printf("\tcommand %u\n", ntohl(hdr->common.command));
	printf("\tseqnum %u\n", ntohl(hdr->common.seqnum));
	printf("\tdevid %x\n", ntohl(hdr->common.devid));
	printf("\tdirection %u\n", ntohl(hdr->common.direction));
	printf("\tep %x\n", ntohl(hdr->common.ep));

	switch (ntohl(hdr->common.command)) {
	case USBIP_CMD_SUBMIT:
		printf("\tflags %x\n", ntohl(hdr->u.submit.transfer_flags));
		printf("\tnumber of packets %u\n",
		       ntohl(hdr->u.submit.number_of_packets));
		printf("\tinterval %u\n", ntohl(hdr->u.submit.interval));
		printf("\tbuffer length %u\n",
		       ntohl(hdr->u.submit.transfer_buffer_length));
		break;
	case USBIP_CMD_UNLINK:
		printf("\tseqnum %d\n", ntohl(hdr->u.unlink.seqnum));
		break;
	default:
		break;
	}
}
#else
#define hexdump(x, y)
#define usbip_header_dump(x)
#endif

void get_interface(u8_t *descriptors)
{
	while (descriptors[0]) {
		if (descriptors[1] == DESC_INTERFACE) {
			SYS_LOG_DBG("interface found");
		}

		/* skip to next descriptor */
		descriptors += descriptors[0];
	}
}

static int send_interfaces(const u8_t *descriptors, int connfd)
{
	struct devlist_interface {
		u8_t bInterfaceClass;
		u8_t bInterfaceSubClass;
		u8_t bInterfaceProtocol;
		u8_t padding;	/* alignment */
	} __packed iface;

	while (descriptors[0]) {
		if (descriptors[1] == DESC_INTERFACE) {
			struct usb_if_descriptor *desc = (void *)descriptors;

			iface.bInterfaceClass = desc->bInterfaceClass;
			iface.bInterfaceSubClass = desc->bInterfaceSubClass;
			iface.bInterfaceProtocol = desc->bInterfaceProtocol;
			iface.padding = 0;

			if (send(connfd, &iface, sizeof(iface), 0) !=
			    sizeof(iface)) {
				SYS_LOG_ERR("send() failed: %s",
					    strerror(errno));
				return errno;
			}
		}

		/* skip to next descriptor */
		descriptors += descriptors[0];
	}

	return 0;
}

static void fill_device(struct devlist_device *dev, const u8_t *desc)
{
	struct usb_device_descriptor *dev_dsc = (void *)desc;
	struct usb_cfg_descriptor *cfg =
		(void *)(desc + sizeof(struct usb_device_descriptor));

	memset(dev->path, 0, 256);
	strcpy(dev->path, "/sys/devices/pci0000:00/0000:00:01.2/usb1/1-1");
	memset(dev->busid, 0, 32);
	strcpy(dev->busid, "1-1");

	dev->busnum = htonl(1);
	dev->devnum = htonl(2);
	dev->speed = htonl(2);

	dev->idVendor = htons(dev_dsc->idVendor);
	dev->idProduct = htons(dev_dsc->idProduct);
	dev->bcdDevice = htons(dev_dsc->bcdDevice);
	dev->bDeviceClass = dev_dsc->bDeviceClass;
	dev->bDeviceSubClass = dev_dsc->bDeviceSubClass;
	dev->bDeviceProtocol = dev_dsc->bDeviceProtocol;

	dev->bConfigurationValue = cfg->bConfigurationValue;
	dev->bNumConfigurations = dev_dsc->bNumConfigurations;
	dev->bNumInterfaces = cfg->bNumInterfaces;
}

static int send_device(const u8_t *desc, int connfd)
{
	struct devlist_device dev;

	fill_device(&dev, desc);

	if (send(connfd, &dev, sizeof(dev), 0) != sizeof(dev)) {
		SYS_LOG_ERR("send() device failed: %s", strerror(errno));
		return errno;
	}

	return 0;
}

static int handle_device_list(const u8_t *desc, int connfd)
{
	struct op_common header = {
		.version = htons(USBIP_VERSION),
		.code = htons(OP_REP_DEVLIST),
		.status = 0,
	};

	SYS_LOG_DBG("desc %p", desc);

	if (send(connfd, &header, sizeof(header), 0) != sizeof(header)) {
		SYS_LOG_ERR("send() header failed: %s", strerror(errno));
		return errno;
	}

	/* Send number of devices */
	u32_t ndev = htonl(1);

	if (send(connfd, &ndev, sizeof(ndev), 0) != sizeof(ndev)) {
		SYS_LOG_ERR("send() ndev failed: %s", strerror(errno));
		return errno;
	}

	send_device(desc, connfd);

	send_interfaces(desc, connfd);

	return 0;
}

static void handle_usbip_submit(int connfd, struct usbip_header *hdr)
{
	struct usbip_submit *req = &hdr->u.submit;
	int read;

	SYS_LOG_DBG("");

	read = recv(connfd, req, sizeof(*req), 0);
	if (read != sizeof(*req)) {
		SYS_LOG_ERR("recv() failed: %s", strerror(errno));
		return;
	}

	usbip_header_dump((void *)hdr);

	if (ntohl(hdr->common.ep) == 0) {
		handle_usb_control(hdr);
	} else {
		handle_usb_data(hdr);
	}
}

static void handle_usbip_unlink(int connfd, struct usbip_header *hdr)
{
	struct usbip_unlink *req = &hdr->u.unlink;
	u64_t setup_padding;
	int read;

	SYS_LOG_DBG("");

	read = recv(connfd, req, sizeof(hdr->u), 0);
	if (read != sizeof(hdr->u)) {
		SYS_LOG_ERR("recv() failed: %s", strerror(errno));
		return;
	}

	/* Read also padding */
	read = recv(connfd, &setup_padding, sizeof(setup_padding), 0);
	if (read != sizeof(setup_padding)) {
		SYS_LOG_ERR("recv() failed: %s", strerror(errno));
		return;
	}

	usbip_header_dump((void *)hdr);

	/* TODO: unlink */
}

static int handle_import(const u8_t *desc, int connfd)
{
	struct op_common header = {
		.version = htons(USBIP_VERSION),
		.code = htons(OP_REP_IMPORT),
		.status = 0,
	};
	char busid[32];

	SYS_LOG_DBG("attach device");

	if (recv(connfd, busid, 32, 0) != sizeof(busid)) {
		SYS_LOG_ERR("recv() failed: %s", strerror(errno));
		return errno;
	}

	if (send(connfd, &header, sizeof(header), 0) != sizeof(header)) {
		SYS_LOG_ERR("send() header failed: %s", strerror(errno));
		return errno;
	}

	send_device(desc, connfd);

	return 0;
}

extern struct usb_desc_header __usb_descriptor_start[];

void usbip_start(void)
{
	struct sockaddr_in srv;
	unsigned char attached;
	int listenfd, connfd;
	const u8_t *desc;
	int reuse = 1;

	SYS_LOG_DBG("Starting");

	/*
	 * Do not use usb_get_device_descriptor();
	 * to prevent double string fixing
	 */
	desc = (const u8_t *)__usb_descriptor_start;
	if (!desc) {
		SYS_LOG_ERR("Descriptors are not set");
		posix_exit(EXIT_FAILURE);
	}

	listenfd = socket(PF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
	if (listenfd < 0) {
		SYS_LOG_ERR("socket() failed: %s", strerror(errno));
		posix_exit(EXIT_FAILURE);
	}

	if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR,
		       (const char *)&reuse, sizeof(reuse)) < 0) {
		SYS_LOG_WRN("setsockopt() failed: %s", strerror(errno));
	}

	memset(&srv, 0, sizeof(srv));
	srv.sin_family = AF_INET;
	srv.sin_addr.s_addr = htonl(INADDR_ANY);
	srv.sin_port = htons(USBIP_PORT);

	if (bind(listenfd, (struct sockaddr *)&srv, sizeof(srv)) < 0) {
		SYS_LOG_ERR("bind() failed: %s", strerror(errno));
		posix_exit(EXIT_FAILURE);
	}

	if (listen(listenfd, SOMAXCONN) < 0) {
		SYS_LOG_ERR("listen() failed: %s", strerror(errno));
		posix_exit(EXIT_FAILURE);
	}

	while (true) {
		struct sockaddr_in client_addr;
		socklen_t client_addr_len = sizeof(client_addr);

		connfd = accept4(listenfd, (struct sockaddr *)&client_addr,
				 &client_addr_len, SOCK_NONBLOCK);
		if (connfd < 0) {
			if (errno == EAGAIN || errno == EWOULDBLOCK) {
				/* Non-blocking accept */
				k_sleep(100);

				continue;
			}

			SYS_LOG_ERR("accept() failed: %s", strerror(errno));
			posix_exit(EXIT_FAILURE);
		}

		connfd_global = connfd;

		SYS_LOG_DBG("Connection: %s", inet_ntoa(client_addr.sin_addr));

		/* Set attached 0 */
		attached = 0;

		while (true) {
			struct usbip_header cmd;
			struct usbip_header_common *hdr = &cmd.common;
			int read;

			if (!attached) {
				struct op_common req;

				read = recv(connfd, &req, sizeof(req), 0);
				if (read < 0) {
					if (errno == EAGAIN ||
					    errno == EWOULDBLOCK) {
						/* Non-blocking accept */
						k_sleep(100);

						continue;
					}
				}

				if (read != sizeof(req)) {
					SYS_LOG_WRN("wrong length, %d", read);

					/* Closing connection */
					break;
				}

				hexdump((u8_t *)&req, sizeof(req));

				SYS_LOG_DBG("Code: 0x%x", ntohs(req.code));

				switch (ntohs(req.code)) {
				case OP_REQ_DEVLIST:
					handle_device_list(desc, connfd);
					break;
				case OP_REQ_IMPORT:
					if (!handle_import(desc, connfd)) {
						attached = 1;
					}
					break;
				default:
					SYS_LOG_ERR("Unhandled code: 0x%x",
						    ntohs(req.code));
					break;
				}

				continue;
			}

			/* Handle attached case */

			read = recv(connfd, hdr, sizeof(*hdr), 0);
			if (read < 0) {
				if (errno == EAGAIN || errno == EWOULDBLOCK) {
					/* Non-blocking accept */
					k_sleep(100);

					continue;
				}
			}

			if (read != sizeof(*hdr)) {
				SYS_LOG_ERR("recv wrong length: %d", read);

				/* Closing connection */
				break;
			}

			hexdump((u8_t *)hdr, sizeof(*hdr));

			devid_global = ntohl(hdr->devid);
			seqnum_global = ntohl(hdr->seqnum);

			switch (ntohl(hdr->command)) {
			case USBIP_CMD_SUBMIT:
				handle_usbip_submit(connfd, &cmd);
				break;
			case USBIP_CMD_UNLINK:
				handle_usbip_unlink(connfd, &cmd);
				break;
			default:
				SYS_LOG_ERR("Unknown command: 0x%x",
					    ntohl(hdr->command));
				close(connfd);
				return;
			}
		}

		SYS_LOG_DBG("Closing connection");
		close(connfd);
	}
}

int usbip_recv(u8_t *buf, size_t len)
{
	return recv(connfd_global, buf, len, 0);
}

int usbip_send(u8_t ep, const u8_t *data, size_t len)
{
	return send(connfd_global, data, len, 0);
}

int usbip_send_common(u8_t ep, u32_t data_len)
{
	struct usbip_submit_rsp rsp;

	rsp.common.command = htonl(USBIP_RET_SUBMIT);
	rsp.common.seqnum = htonl(seqnum_global);
	rsp.common.devid = htonl(0);
	rsp.common.direction = htonl(0); /* TODO get from ep */
	rsp.common.ep = htonl(ep);

	rsp.status = htonl(0);
	rsp.actual_length = htonl(data_len);
	rsp.start_frame = htonl(0);
	rsp.number_of_packets = htonl(0);
	rsp.error_count = htonl(0);

	return usbip_send(ep, (u8_t *)&rsp, sizeof(rsp));
}
