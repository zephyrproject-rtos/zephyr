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
#include <usb/usb_device.h>

#include <posix_board_if.h>
#include "usb_dc_native_posix_adapt.h"

#define LOG_LEVEL CONFIG_USB_DRIVER_LOG_LEVEL
LOG_MODULE_REGISTER(native_posix_adapt);

#define USBIP_PORT	3240
#define USBIP_VERSION	273

#define VERBOSE_DEBUG

int connfd_global;
int seqnum_global;
int devid_global;

/* Helpers */

#ifdef VERBOSE_DEBUG
static void usbip_header_dump(struct usbip_header *hdr)
{
	LOG_DBG("cmd %x seq %u dir %u ep %x", ntohl(hdr->common.command),
		ntohl(hdr->common.seqnum), ntohl(hdr->common.direction),
		ntohl(hdr->common.ep));

	switch (ntohl(hdr->common.command)) {
	case USBIP_CMD_SUBMIT:
		LOG_DBG("flags %x np %u int %u buflen %u",
			ntohl(hdr->u.submit.transfer_flags),
			ntohl(hdr->u.submit.number_of_packets),
			ntohl(hdr->u.submit.interval),
			ntohl(hdr->u.submit.transfer_buffer_length));
		break;
	case USBIP_CMD_UNLINK:
		LOG_DBG("seq %d", ntohl(hdr->u.unlink.seqnum));
		break;
	default:
		break;
	}
}
#else
#define usbip_header_dump(x)
#endif

void get_interface(uint8_t *descriptors)
{
	while (descriptors[0]) {
		if (descriptors[1] == USB_INTERFACE_DESC) {
			LOG_DBG("interface found");
		}

		/* skip to next descriptor */
		descriptors += descriptors[0];
	}
}

static int send_interfaces(const uint8_t *descriptors, int connfd)
{
	struct devlist_interface {
		uint8_t bInterfaceClass;
		uint8_t bInterfaceSubClass;
		uint8_t bInterfaceProtocol;
		uint8_t padding;	/* alignment */
	} __packed iface;

	while (descriptors[0]) {
		if (descriptors[1] == USB_INTERFACE_DESC) {
			struct usb_if_descriptor *desc = (void *)descriptors;

			iface.bInterfaceClass = desc->bInterfaceClass;
			iface.bInterfaceSubClass = desc->bInterfaceSubClass;
			iface.bInterfaceProtocol = desc->bInterfaceProtocol;
			iface.padding = 0U;

			if (send(connfd, &iface, sizeof(iface), 0) !=
			    sizeof(iface)) {
				LOG_ERR("send() failed: %s", strerror(errno));
				return errno;
			}
		}

		/* skip to next descriptor */
		descriptors += descriptors[0];
	}

	return 0;
}

static void fill_device(struct devlist_device *dev, const uint8_t *desc)
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

static int send_device(const uint8_t *desc, int connfd)
{
	struct devlist_device dev;

	fill_device(&dev, desc);

	if (send(connfd, &dev, sizeof(dev), 0) != sizeof(dev)) {
		LOG_ERR("send() device failed: %s", strerror(errno));
		return errno;
	}

	return 0;
}

static int handle_device_list(const uint8_t *desc, int connfd)
{
	struct op_common header = {
		.version = htons(USBIP_VERSION),
		.code = htons(OP_REP_DEVLIST),
		.status = 0,
	};

	LOG_DBG("desc %p", desc);

	if (send(connfd, &header, sizeof(header), 0) != sizeof(header)) {
		LOG_ERR("send() header failed: %s", strerror(errno));
		return errno;
	}

	/* Send number of devices */
	uint32_t ndev = htonl(1);

	if (send(connfd, &ndev, sizeof(ndev), 0) != sizeof(ndev)) {
		LOG_ERR("send() ndev failed: %s", strerror(errno));
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

	LOG_DBG("");

	read = recv(connfd, req, sizeof(*req), 0);
	if (read != sizeof(*req)) {
		LOG_ERR("recv() failed: %s", strerror(errno));
		return;
	}

	usbip_header_dump((void *)hdr);

	if (ntohl(hdr->common.ep) == 0) {
		handle_usb_control(hdr);
	} else {
		handle_usb_data(hdr);
	}
}

bool usbip_skip_setup(void)
{
	uint64_t setup;

	LOG_DBG("Skip 8 bytes");

	if (usbip_recv((void *)&setup, sizeof(setup)) != sizeof(setup)) {
		return false;
	}

	return true;
}

static void handle_usbip_unlink(int connfd, struct usbip_header *hdr)
{
	int read;

	LOG_DBG("");

	/* Need to read the whole structure */
	read = recv(connfd, &hdr->u, sizeof(hdr->u), 0);
	if (read != sizeof(hdr->u)) {
		LOG_ERR("recv() failed: %s", strerror(errno));
		return;
	}

	/* Read USB setup, not handled */
	if (!usbip_skip_setup()) {
		LOG_ERR("setup skipping failed");
		return;
	}

	usbip_header_dump((void *)hdr);

	/* TODO: unlink */
}

static int handle_import(const uint8_t *desc, int connfd)
{
	struct op_common header = {
		.version = htons(USBIP_VERSION),
		.code = htons(OP_REP_IMPORT),
		.status = 0,
	};
	char busid[32];

	LOG_DBG("attach device");

	if (recv(connfd, busid, 32, 0) != sizeof(busid)) {
		LOG_ERR("recv() failed: %s", strerror(errno));
		return errno;
	}

	if (send(connfd, &header, sizeof(header), 0) != sizeof(header)) {
		LOG_ERR("send() header failed: %s", strerror(errno));
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
	const uint8_t *desc;
	int reuse = 1;

	LOG_DBG("Starting");

	/*
	 * Do not use usb_get_device_descriptor();
	 * to prevent double string fixing
	 */
	desc = (const uint8_t *)__usb_descriptor_start;
	if (!desc) {
		LOG_ERR("Descriptors are not set");
		posix_exit(EXIT_FAILURE);
	}

	listenfd = socket(PF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
	if (listenfd < 0) {
		LOG_ERR("socket() failed: %s", strerror(errno));
		posix_exit(EXIT_FAILURE);
	}

	if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR,
		       (const char *)&reuse, sizeof(reuse)) < 0) {
		LOG_WRN("setsockopt() failed: %s", strerror(errno));
	}

	memset(&srv, 0, sizeof(srv));
	srv.sin_family = AF_INET;
	srv.sin_addr.s_addr = htonl(INADDR_ANY);
	srv.sin_port = htons(USBIP_PORT);

	if (bind(listenfd, (struct sockaddr *)&srv, sizeof(srv)) < 0) {
		LOG_ERR("bind() failed: %s", strerror(errno));
		posix_exit(EXIT_FAILURE);
	}

	if (listen(listenfd, SOMAXCONN) < 0) {
		LOG_ERR("listen() failed: %s", strerror(errno));
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
				k_sleep(K_MSEC(100));

				continue;
			}

			LOG_ERR("accept() failed: %s", strerror(errno));
			posix_exit(EXIT_FAILURE);
		}

		connfd_global = connfd;

		LOG_DBG("Connection: %s", inet_ntoa(client_addr.sin_addr));

		/* Set attached 0 */
		attached = 0U;

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
						k_sleep(K_MSEC(100));

						continue;
					}
				}

				if (read != sizeof(req)) {
					LOG_WRN("wrong length, %d", read);

					/* Closing connection */
					break;
				}

				LOG_HEXDUMP_DBG((uint8_t *)&req, sizeof(req),
						"Got request");

				LOG_DBG("Code: 0x%x", ntohs(req.code));

				switch (ntohs(req.code)) {
				case OP_REQ_DEVLIST:
					handle_device_list(desc, connfd);
					break;
				case OP_REQ_IMPORT:
					if (!handle_import(desc, connfd)) {
						attached = 1U;
					}
					break;
				default:
					LOG_ERR("Unhandled code: 0x%x",
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
					k_sleep(K_MSEC(100));

					continue;
				}
			}

			LOG_HEXDUMP_DBG((uint8_t *)hdr, read, "Got cmd");

			if (read != sizeof(*hdr)) {
				LOG_ERR("recv wrong length: %d", read);

				/* Closing connection */
				break;
			}

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
				LOG_ERR("Unknown command: 0x%x",
					ntohl(hdr->command));
				close(connfd);
				return;
			}
		}

		LOG_DBG("Closing connection");
		close(connfd);
	}
}

int usbip_recv(uint8_t *buf, size_t len)
{
	return recv(connfd_global, buf, len, 0);
}

int usbip_send(uint8_t ep, const uint8_t *data, size_t len)
{
	return send(connfd_global, data, len, 0);
}

bool usbip_send_common(uint8_t ep, uint32_t data_len)
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

	rsp.setup = htonl(0);

	if (usbip_send(ep, (uint8_t *)&rsp, sizeof(rsp)) == sizeof(rsp)) {
		return true;
	}

	return false;
}
