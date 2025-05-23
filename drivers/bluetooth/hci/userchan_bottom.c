/**
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * "Bottom"/Linux/Host side of the userchannel driver. This file is built in the native_simulator
 * runner context with the host libC, but it's functionality can be called from the "embedded" side
 */

#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include <poll.h>
#include <unistd.h>
#include <sys/socket.h>
#include <limits.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <nsi_errno.h>

#define BTPROTO_HCI      1
#define HCI_CHANNEL_USER 1

struct sockaddr_hci {
	sa_family_t    hci_family;
	unsigned short hci_dev;
	unsigned short hci_channel;
};

bool user_chan_rx_ready(int fd)
{
	struct pollfd pollfd = {.fd = fd, .events = POLLIN};

	return (poll(&pollfd, 1, 0) == 1);
}

int user_chan_is_ipaddr_ok(char ip_addr[])
{
	struct in_addr addr;

	return inet_pton(AF_INET, ip_addr, &addr);
}

int user_chan_socket_open(unsigned short bt_dev_index)
{
	int fd;
	struct sockaddr_hci addr;

	fd = socket(PF_BLUETOOTH, SOCK_RAW | SOCK_CLOEXEC | SOCK_NONBLOCK, BTPROTO_HCI);
	if (fd < 0) {
		return -nsi_errno_to_mid(errno);
	}

	(void)memset(&addr, 0, sizeof(addr));
	addr.hci_family = AF_BLUETOOTH;
	addr.hci_dev = bt_dev_index;
	addr.hci_channel = HCI_CHANNEL_USER;

	if (bind(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
		int err = -nsi_errno_to_mid(errno);

		close(fd);
		return err;
	}

	return fd;
}


int user_chan_net_connect(char ip_addr[], unsigned int port)
{
	int fd;
	struct sockaddr_in addr;

	fd = socket(AF_INET, SOCK_STREAM, 0);
	if (fd < 0) {
		return -nsi_errno_to_mid(errno);
	}

	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	if (inet_pton(AF_INET, ip_addr, &(addr.sin_addr)) <= 0) {
		int err = -nsi_errno_to_mid(errno);

		close(fd);
		return err;
	}

	if (connect(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
		int err = -nsi_errno_to_mid(errno);

		close(fd);
		return err;
	}

	return fd;
}
