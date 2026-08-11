/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 *
 * Routines setting up the host system.
 */

/* Host include files */
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <errno.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <netpacket/packet.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <net/if.h>
#include <time.h>
#include <inttypes.h>
#include <nsi_tracing.h>
#include <nsi_errno.h>

#ifdef __linux
#include <linux/if.h>
#include <linux/if_tun.h>
#endif

#include "wifi_drv_priv.h"

/* We are attaching here to the wireless network interface created
 * by mac80211_hwsim. In order to send data to the network interface,
 * we create a packet socket that is used to send and receive data.
 */
int wifi_iface_create(const char *if_name)
{
	struct ifreq ifr;
	struct sockaddr_ll ll;
	int fd, ret = -EINVAL;

	fd = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
	if (fd < 0) {
		ret = -nsi_errno_to_mid(errno);
		nsi_print_warning("%s: %s: %s (%d)\n", __func__,
				  "socket[AF_PACKET]",
				  strerror(errno), ret);
		return ret;
	}

	(void)memset(&ifr, 0, sizeof(ifr));

	strncpy(ifr.ifr_name, if_name, sizeof(ifr.ifr_name) - 1);

	ret = ioctl(fd, SIOCGIFINDEX, &ifr);
	if (ret < 0) {
		ret = -nsi_errno_to_mid(errno);
		nsi_print_warning("%s: %s: %s (%d)\n", __func__,
				  "ioctl[SIOCGIFINDEX]",
				  strerror(errno), ret);
		close(fd);
		return ret;
	}

	memset(&ll, 0, sizeof(ll));

	ll.sll_family = AF_PACKET;
	ll.sll_ifindex = ifr.ifr_ifindex;
	ll.sll_protocol = htons(ETH_P_ALL);

	nsi_print_trace("%s: Connecting to %s (%d)\n", __func__,
			if_name, ifr.ifr_ifindex);

	ret = bind(fd, (struct sockaddr *)&ll, sizeof(ll));
	if (ret < 0) {
		ret = -nsi_errno_to_mid(errno);
		nsi_print_warning("%s: %s: %s (%d)\n", __func__,
				  "bind[AF_PACKET]",
				  strerror(errno), ret);
		close(fd);
		return ret;
	}

	return fd;
}

int wifi_iface_get_mac(const char *if_name, unsigned char *mac)
{
	struct ifreq ifr;
	int fd, ret;

	fd = socket(AF_INET, SOCK_DGRAM, 0);
	if (fd < 0) {
		return -nsi_errno_to_mid(errno);
	}

	(void)memset(&ifr, 0, sizeof(ifr));
	strncpy(ifr.ifr_name, if_name, sizeof(ifr.ifr_name) - 1);

	ret = ioctl(fd, SIOCGIFHWADDR, &ifr);
	if (ret < 0) {
		ret = -nsi_errno_to_mid(errno);
		close(fd);
		return ret;
	}

	memcpy(mac, ifr.ifr_hwaddr.sa_data, 6);
	close(fd);

	return 0;
}

static int ssystem(const char *fmt, ...)
	__attribute__((__format__(__printf__, 1, 2)));

static int ssystem(const char *fmt, ...)
{
	char cmd[255];
	va_list ap;
	int ret;

	va_start(ap, fmt);
	vsnprintf(cmd, sizeof(cmd), fmt, ap);
	va_end(ap);

	nsi_print_trace("Running: '%s'\n", cmd);

	ret = system(cmd);
	if (ret == -1) {
		return -nsi_errno_to_mid(errno);
	}

	return (ret == 0) ? 0 : -nsi_errno_to_mid(EIO);
}

int wifi_wait_data(int fd)
{
	struct timeval timeout;
	fd_set rset;
	int ret;

	FD_ZERO(&rset);

	FD_SET(fd, &rset);

	timeout.tv_sec = 0;
	timeout.tv_usec = 0;

	ret = select(fd + 1, &rset, NULL, NULL, &timeout);
	if (ret < 0 && errno != EINTR) {
		return -nsi_errno_to_mid(errno);
	} else if (ret > 0) {
		if (FD_ISSET(fd, &rset)) {
			return 0;
		}
	}

	return -nsi_errno_to_mid(EAGAIN);
}

ssize_t wifi_read_data(int fd, void *buf, size_t buf_len)
{
	int ret;

	ret = recv(fd, buf, buf_len, 0);
	if (ret < 0) {
		ret = -nsi_errno_to_mid(errno);
		nsi_print_warning("%s: %s: %s (%d)\n", __func__,
				  "recv",
				  strerror(errno), ret);
	}

	return ret;
}

ssize_t wifi_write_data(int fd, void *buf, size_t buf_len)
{
	int ret;

	ret = send(fd, buf, buf_len, 0);
	if (ret < 0) {
		ret = -nsi_errno_to_mid(errno);
		nsi_print_warning("%s: %s: %s (%d)\n", __func__,
				  "send",
				  strerror(errno), ret);
	}

	return ret;
}

int wifi_promisc_mode(const char *if_name, bool enable)
{
	return ssystem("ip link set dev %s promisc %s",
		       if_name, enable ? "on" : "off");
}
