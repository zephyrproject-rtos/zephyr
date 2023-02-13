/* userchan.c - HCI User Channel based Bluetooth driver */

/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/byteorder.h>

#include <errno.h>
#include <stddef.h>
#include <stdlib.h>
#include <poll.h>
#include <errno.h>
#include <sys/socket.h>
#include <string.h>
#include <unistd.h>

#include "soc.h"
#include "cmdline.h" /* native_posix command line options header */

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/drivers/bluetooth/hci_driver.h>

#define LOG_LEVEL CONFIG_BT_HCI_DRIVER_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(bt_driver);

#define BTPROTO_HCI      1
struct sockaddr_hci {
	sa_family_t     hci_family;
	unsigned short  hci_dev;
	unsigned short  hci_channel;
};
#define HCI_CHANNEL_USER 1

#define SOL_HCI          0

#define H4_CMD           0x01
#define H4_ACL           0x02
#define H4_SCO           0x03
#define H4_EVT           0x04
#define H4_ISO           0x05

static K_KERNEL_STACK_DEFINE(rx_thread_stack,
			     CONFIG_ARCH_POSIX_RECOMMENDED_STACK_SIZE);
static struct k_thread rx_thread_data;

static int uc_fd = -1;

static int bt_dev_index = -1;

static struct net_buf *get_rx(const uint8_t *buf)
{
	bool discardable = false;
	k_timeout_t timeout = K_FOREVER;

	switch (buf[0]) {
	case H4_EVT:
		if (buf[1] == BT_HCI_EVT_LE_META_EVENT &&
		    (buf[3] == BT_HCI_EVT_LE_ADVERTISING_REPORT)) {
			discardable = true;
			timeout = K_NO_WAIT;
		}

		return bt_buf_get_evt(buf[1], discardable, timeout);
	case H4_ACL:
		return bt_buf_get_rx(BT_BUF_ACL_IN, K_FOREVER);
	case H4_ISO:
		if (IS_ENABLED(CONFIG_BT_ISO)) {
			return bt_buf_get_rx(BT_BUF_ISO_IN, K_FOREVER);
		}
		__fallthrough;
	default:
		LOG_ERR("Unknown packet type: %u", buf[0]);
	}

	return NULL;
}

static bool uc_ready(void)
{
	struct pollfd pollfd = { .fd = uc_fd, .events = POLLIN };

	return (poll(&pollfd, 1, 0) == 1);
}

static void rx_thread(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	LOG_DBG("started");

	while (1) {
		static uint8_t frame[512];
		struct net_buf *buf;
		size_t buf_tailroom;
		size_t buf_add_len;
		ssize_t len;

		if (!uc_ready()) {
			k_sleep(K_MSEC(1));
			continue;
		}

		LOG_DBG("calling read()");

		len = read(uc_fd, frame, sizeof(frame));
		if (len < 0) {
			if (errno == EINTR) {
				k_yield();
				continue;
			}

			LOG_ERR("Reading socket failed, errno %d", errno);
			close(uc_fd);
			uc_fd = -1;
			return;
		}

		buf = get_rx(frame);
		if (!buf) {
			LOG_DBG("Discard adv report due to insufficient buf");
			continue;
		}

		buf_tailroom = net_buf_tailroom(buf);
		buf_add_len = len - 1;
		if (buf_tailroom < buf_add_len) {
			LOG_ERR("Not enough space in buffer %zu/%zu", buf_add_len, buf_tailroom);
			net_buf_unref(buf);
			continue;
		}

		net_buf_add_mem(buf, &frame[1], buf_add_len);

		LOG_DBG("Calling bt_recv(%p)", buf);

		bt_recv(buf);

		k_yield();
	}
}

static int uc_send(struct net_buf *buf)
{
	LOG_DBG("buf %p type %u len %u", buf, bt_buf_get_type(buf), buf->len);

	if (uc_fd < 0) {
		LOG_ERR("User channel not open");
		return -EIO;
	}

	switch (bt_buf_get_type(buf)) {
	case BT_BUF_ACL_OUT:
		net_buf_push_u8(buf, H4_ACL);
		break;
	case BT_BUF_CMD:
		net_buf_push_u8(buf, H4_CMD);
		break;
	case BT_BUF_ISO_OUT:
		if (IS_ENABLED(CONFIG_BT_ISO)) {
			net_buf_push_u8(buf, H4_ISO);
			break;
		}
		__fallthrough;
	default:
		LOG_ERR("Unknown buffer type");
		return -EINVAL;
	}

	if (write(uc_fd, buf->data, buf->len) < 0) {
		return -errno;
	}

	net_buf_unref(buf);
	return 0;
}

static int user_chan_open(uint16_t index)
{
	struct sockaddr_hci addr;
	int fd;

	fd = socket(PF_BLUETOOTH, SOCK_RAW | SOCK_CLOEXEC | SOCK_NONBLOCK,
		    BTPROTO_HCI);
	if (fd < 0) {
		return -errno;
	}

	(void)memset(&addr, 0, sizeof(addr));
	addr.hci_family = AF_BLUETOOTH;
	addr.hci_dev = index;
	addr.hci_channel = HCI_CHANNEL_USER;

	if (bind(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
		int err = -errno;

		close(fd);
		return err;
	}

	return fd;
}

static int uc_open(void)
{
	if (bt_dev_index < 0) {
		LOG_ERR("No Bluetooth device specified");
		return -ENODEV;
	}

	LOG_DBG("hci%d", bt_dev_index);

	uc_fd = user_chan_open(bt_dev_index);
	if (uc_fd < 0) {
		return uc_fd;
	}

	LOG_DBG("User Channel opened as fd %d", uc_fd);

	k_thread_create(&rx_thread_data, rx_thread_stack,
			K_KERNEL_STACK_SIZEOF(rx_thread_stack),
			rx_thread, NULL, NULL, NULL,
			K_PRIO_COOP(CONFIG_BT_DRIVER_RX_HIGH_PRIO),
			0, K_NO_WAIT);

	LOG_DBG("returning");

	return 0;
}

static const struct bt_hci_driver drv = {
	.name		= "HCI User Channel",
	.bus		= BT_HCI_DRIVER_BUS_UART,
	.open		= uc_open,
	.send		= uc_send,
};

static int bt_uc_init(const struct device *unused)
{
	ARG_UNUSED(unused);

	bt_hci_driver_register(&drv);

	return 0;
}

SYS_INIT(bt_uc_init, POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE);

static void cmd_bt_dev_found(char *argv, int offset)
{
	if (strncmp(&argv[offset], "hci", 3) || strlen(&argv[offset]) < 4) {
		posix_print_error_and_exit("Error: Invalid Bluetooth device "
					   "name '%s' (should be e.g. hci0)\n",
					   &argv[offset]);
		return;
	}

	bt_dev_index = strtol(&argv[offset + 3], NULL, 10);
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
		"A local HCI device to be used for Bluetooth (e.g. hci0)" },
		ARG_TABLE_ENDMARKER
	};

	native_add_command_line_opts(btuserchan_args);
}

static void btuserchan_check_arg(void)
{
	if (bt_dev_index < 0) {
		posix_print_error_and_exit("Error: Bluetooth device missing. "
					   "Specify one using --bt-dev=hciN\n");
	}
}

NATIVE_TASK(add_btuserchan_arg, PRE_BOOT_1, 10);
NATIVE_TASK(btuserchan_check_arg, PRE_BOOT_2, 10);
