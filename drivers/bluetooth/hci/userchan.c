/* userchan.c - HCI User Channel based Bluetooth driver */

/*
 * Copyright (c) 2018 Intel Corporation
 * Copyright (c) 2023 Victor Chavez
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/sys/util.h>

#include <errno.h>
#include <stddef.h>
#include <stdlib.h>
#include <poll.h>
#include <errno.h>
#include <sys/socket.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <limits.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <zephyr/sys/byteorder.h>

#include "soc.h"
#include "cmdline.h" /* native_posix command line options header */

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/drivers/bluetooth.h>

#define LOG_LEVEL CONFIG_BT_HCI_DRIVER_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(bt_driver);

#define DT_DRV_COMPAT zephyr_bt_hci_userchan

struct uc_data {
	int           fd;
	bt_hci_recv_t recv;

};

#define BTPROTO_HCI      1
struct sockaddr_hci {
	sa_family_t     hci_family;
	unsigned short  hci_dev;
	unsigned short  hci_channel;
};
#define HCI_CHANNEL_USER 1

#define SOL_HCI          0

static K_KERNEL_STACK_DEFINE(rx_thread_stack,
			     CONFIG_ARCH_POSIX_RECOMMENDED_STACK_SIZE);
static struct k_thread rx_thread_data;

static unsigned short bt_dev_index;

#define TCP_ADDR_BUFF_SIZE 16
static bool hci_socket;
static char ip_addr[TCP_ADDR_BUFF_SIZE];
static unsigned int port;
static bool arg_found;

static struct net_buf *get_rx(const uint8_t *buf)
{
	bool discardable = false;
	k_timeout_t timeout = K_FOREVER;

	switch (buf[0]) {
	case BT_HCI_H4_EVT:
		if (buf[1] == BT_HCI_EVT_LE_META_EVENT &&
		    (buf[3] == BT_HCI_EVT_LE_ADVERTISING_REPORT)) {
			discardable = true;
			timeout = K_NO_WAIT;
		}

		return bt_buf_get_evt(buf[1], discardable, timeout);
	case BT_HCI_H4_ACL:
		return bt_buf_get_rx(BT_BUF_ACL_IN, K_FOREVER);
	case BT_HCI_H4_ISO:
		if (IS_ENABLED(CONFIG_BT_ISO)) {
			return bt_buf_get_rx(BT_BUF_ISO_IN, K_FOREVER);
		}
		__fallthrough;
	default:
		LOG_ERR("Unknown packet type: %u", buf[0]);
	}

	return NULL;
}

/**
 * @brief Decode the length of an HCI H4 packet and check it's complete
 * @details Decodes packet length according to Bluetooth spec v5.4 Vol 4 Part E
 * @param buf	Pointer to a HCI packet buffer
 * @param buf_len	Bytes available in the buffer
 * @return Length of the complete HCI packet in bytes, -1 if cannot find an HCI
 *         packet, 0 if more data required.
 */
static int32_t hci_packet_complete(const uint8_t *buf, uint16_t buf_len)
{
	uint16_t payload_len = 0;
	const uint8_t type = buf[0];
	uint8_t header_len = sizeof(type);
	const uint8_t *hdr = &buf[sizeof(type)];

	switch (type) {
	case BT_HCI_H4_CMD: {
		const struct bt_hci_cmd_hdr *cmd = (const struct bt_hci_cmd_hdr *)hdr;

		/* Parameter Total Length */
		payload_len = cmd->param_len;
		header_len += BT_HCI_CMD_HDR_SIZE;
		break;
	}
	case BT_HCI_H4_ACL: {
		const struct bt_hci_acl_hdr *acl = (const struct bt_hci_acl_hdr *)hdr;

		/* Data Total Length */
		payload_len = sys_le16_to_cpu(acl->len);
		header_len += BT_HCI_ACL_HDR_SIZE;
		break;
	}
	case BT_HCI_H4_SCO: {
		const struct bt_hci_sco_hdr *sco = (const struct bt_hci_sco_hdr *)hdr;

		/* Data_Total_Length */
		payload_len = sco->len;
		header_len += BT_HCI_SCO_HDR_SIZE;
		break;
	}
	case BT_HCI_H4_EVT: {
		const struct bt_hci_evt_hdr *evt = (const struct bt_hci_evt_hdr *)hdr;

		/* Parameter Total Length */
		payload_len = evt->len;
		header_len += BT_HCI_EVT_HDR_SIZE;
		break;
	}
	case BT_HCI_H4_ISO: {
		const struct bt_hci_iso_hdr *iso = (const struct bt_hci_iso_hdr *)hdr;

		/* ISO_Data_Load_Length parameter */
		payload_len =  bt_iso_hdr_len(sys_le16_to_cpu(iso->len));
		header_len += BT_HCI_ISO_HDR_SIZE;
		break;
	}
	/* If no valid packet type found */
	default:
		LOG_WRN("Unknown packet type 0x%02x", type);
		return -1;
	}

	/* Request more data */
	if (buf_len < header_len || buf_len - header_len < payload_len) {
		return 0;
	}

	return (int32_t)header_len + payload_len;
}

static bool uc_ready(int fd)
{
	struct pollfd pollfd = { .fd = fd, .events = POLLIN };

	return (poll(&pollfd, 1, 0) == 1);
}

static void rx_thread(void *p1, void *p2, void *p3)
{
	const struct device *dev = p1;
	struct uc_data *uc = dev->data;

	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	LOG_DBG("started");

	uint16_t frame_size = 0;

	while (1) {
		static uint8_t frame[512];
		struct net_buf *buf;
		size_t buf_tailroom;
		size_t buf_add_len;
		k_ssize_t len;
		const uint8_t *frame_start = frame;

		if (!uc_ready(uc->fd)) {
			k_sleep(K_MSEC(1));
			continue;
		}

		LOG_DBG("calling read()");

		len = read(uc->fd, frame + frame_size, sizeof(frame) - frame_size);
		if (len < 0) {
			if (errno == EINTR) {
				k_yield();
				continue;
			}

			LOG_ERR("Reading socket failed, errno %d", errno);
			close(uc->fd);
			uc->fd = -1;
			return;
		}

		frame_size += len;

		while (frame_size > 0) {
			const uint8_t *buf_add;
			const uint8_t packet_type = frame_start[0];
			const int32_t decoded_len = hci_packet_complete(frame_start, frame_size);

			if (decoded_len == -1) {
				LOG_ERR("HCI Packet type is invalid, length could not be decoded");
				frame_size = 0; /* Drop buffer */
				break;
			}

			if (decoded_len == 0) {
				if (frame_size == sizeof(frame)) {
					LOG_ERR("HCI Packet (%d bytes) is too big for frame (%d "
						"bytes)",
						decoded_len, sizeof(frame));
					frame_size = 0; /* Drop buffer */
					break;
				}
				if (frame_start != frame) {
					memmove(frame, frame_start, frame_size);
				}
				/* Read more */
				break;
			}

			buf_add = frame_start + sizeof(packet_type);
			buf_add_len = decoded_len - sizeof(packet_type);

			buf = get_rx(frame_start);

			frame_size -= decoded_len;
			frame_start += decoded_len;

			if (!buf) {
				LOG_DBG("Discard adv report due to insufficient buf");
				continue;
			}

			buf_tailroom = net_buf_tailroom(buf);
			if (buf_tailroom < buf_add_len) {
				LOG_ERR("Not enough space in buffer %zu/%zu",
					buf_add_len, buf_tailroom);
				net_buf_unref(buf);
				continue;
			}

			net_buf_add_mem(buf, buf_add, buf_add_len);

			LOG_DBG("Calling bt_recv(%p)", buf);

			uc->recv(dev, buf);
		}

		k_yield();
	}
}

static int uc_send(const struct device *dev, struct net_buf *buf)
{
	struct uc_data *uc = dev->data;

	LOG_DBG("buf %p type %u len %u", buf, bt_buf_get_type(buf), buf->len);

	if (uc->fd < 0) {
		LOG_ERR("User channel not open");
		return -EIO;
	}

	switch (bt_buf_get_type(buf)) {
	case BT_BUF_ACL_OUT:
		net_buf_push_u8(buf, BT_HCI_H4_ACL);
		break;
	case BT_BUF_CMD:
		net_buf_push_u8(buf, BT_HCI_H4_CMD);
		break;
	case BT_BUF_ISO_OUT:
		if (IS_ENABLED(CONFIG_BT_ISO)) {
			net_buf_push_u8(buf, BT_HCI_H4_ISO);
			break;
		}
		__fallthrough;
	default:
		LOG_ERR("Unknown buffer type");
		return -EINVAL;
	}

	if (write(uc->fd, buf->data, buf->len) < 0) {
		return -errno;
	}

	net_buf_unref(buf);
	return 0;
}

static int user_chan_open(void)
{
	int fd;

	if (hci_socket) {
		struct sockaddr_hci addr;

		fd = socket(PF_BLUETOOTH, SOCK_RAW | SOCK_CLOEXEC | SOCK_NONBLOCK,
			    BTPROTO_HCI);
		if (fd < 0) {
			return -errno;
		}

		(void)memset(&addr, 0, sizeof(addr));
		addr.hci_family = AF_BLUETOOTH;
		addr.hci_dev = bt_dev_index;
		addr.hci_channel = HCI_CHANNEL_USER;

		if (bind(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
			int err = -errno;

			close(fd);
			return err;
		}
	} else {
		struct sockaddr_in addr;

		fd = socket(AF_INET, SOCK_STREAM, 0);
		if (fd < 0) {
			return -errno;
		}

		addr.sin_family = AF_INET;
		addr.sin_port = htons(port);
		if (inet_pton(AF_INET, ip_addr, &(addr.sin_addr)) <= 0) {
			int err = -errno;

			close(fd);
			return err;
		}

		if (connect(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
			int err = -errno;

			close(fd);
			return err;
		}
	}

	return fd;
}

static int uc_open(const struct device *dev, bt_hci_recv_t recv)
{
	struct uc_data *uc = dev->data;

	if (hci_socket) {
		LOG_DBG("hci%d", bt_dev_index);
	} else {
		LOG_DBG("hci %s:%d", ip_addr, port);
	}

	uc->fd = user_chan_open();
	if (uc->fd < 0) {
		return uc->fd;
	}

	uc->recv = recv;

	LOG_DBG("User Channel opened as fd %d", uc->fd);

	k_thread_create(&rx_thread_data, rx_thread_stack,
			K_KERNEL_STACK_SIZEOF(rx_thread_stack),
			rx_thread, (void *)dev, NULL, NULL,
			K_PRIO_COOP(CONFIG_BT_DRIVER_RX_HIGH_PRIO),
			0, K_NO_WAIT);

	LOG_DBG("returning");

	return 0;
}

static const struct bt_hci_driver_api uc_drv_api = {
	.open = uc_open,
	.send = uc_send,
};

static int uc_init(const struct device *dev)
{
	if (!arg_found) {
		posix_print_warning("Warning: Bluetooth device missing.\n"
				    "Specify either a local hci interface --bt-dev=hciN\n"
				    "or a valid hci tcp server --bt-dev=ip_address:port\n");
		return -ENODEV;
	}

	return 0;
}

#define UC_DEVICE_INIT(inst) \
	static struct uc_data uc_data_##inst = { \
		.fd = -1, \
	}; \
	DEVICE_DT_INST_DEFINE(inst, uc_init, NULL, &uc_data_##inst, NULL, \
			      POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &uc_drv_api)

DT_INST_FOREACH_STATUS_OKAY(UC_DEVICE_INIT)

static void cmd_bt_dev_found(char *argv, int offset)
{
	arg_found = true;
	if (strncmp(&argv[offset], "hci", 3) == 0 && strlen(&argv[offset]) >= 4) {
		long arg_hci_idx = strtol(&argv[offset + 3], NULL, 10);

		if (arg_hci_idx >= 0 && arg_hci_idx <= USHRT_MAX) {
			bt_dev_index = arg_hci_idx;
			hci_socket = true;
		} else {
			posix_print_error_and_exit("Invalid argument value for --bt-dev. "
						  "hci idx must be within range 0 to 65536.\n");
		}
	} else if (sscanf(&argv[offset], "%15[^:]:%d", ip_addr, &port) == 2) {
		if (port > USHRT_MAX) {
			posix_print_error_and_exit("Error: IP port for bluetooth "
						   "hci tcp server is out of range.\n");
		}
		struct in_addr addr;

		if (inet_pton(AF_INET, ip_addr, &addr) != 1) {
			posix_print_error_and_exit("Error: IP address for bluetooth "
						   "hci tcp server is incorrect.\n");
		}
	} else {
		posix_print_error_and_exit("Invalid option %s for --bt-dev. "
					   "An hci interface or hci tcp server is expected.\n",
					   &argv[offset]);
	}
}

static void add_btuserchan_arg(void)
{
	static struct args_struct_t btuserchan_args[] = {
		/*
		 * Fields:
		 * manual, mandatory, switch,
		 * option_name, var_name ,type,
		 * destination, callback,
		 * description
		 */
		{ false, true, false,
		"bt-dev", "hciX", 's',
		NULL, cmd_bt_dev_found,
		"A local HCI device to be used for Bluetooth (e.g. hci0) "
		"or an HCI TCP Server (e.g. 127.0.0.1:9000)"},
		ARG_TABLE_ENDMARKER
	};

	native_add_command_line_opts(btuserchan_args);
}

NATIVE_TASK(add_btuserchan_arg, PRE_BOOT_1, 10);
