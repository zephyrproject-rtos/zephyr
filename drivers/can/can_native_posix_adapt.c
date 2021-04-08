/*
 * Copyright (c) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 *
 * Routines setting up the host system. Those are placed in separate file
 * because there is naming conflicts between host and zephyr network stacks.
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
#include <sys/select.h>
#include <net/if.h>
#include "arch/posix/posix_trace.h"

#ifdef __linux
#include <linux/can.h>
#endif

/* Zephyr include files. Be very careful here and only include minimum
 * things needed.
 */
#define LOG_MODULE_NAME canbus_posix_adapt
#define LOG_LEVEL CONFIG_CAN_LOG_LEVEL

#include <logging/log.h>
LOG_MODULE_REGISTER(LOG_MODULE_NAME);

#include <zephyr/types.h>

#include "can_native_posix_priv.h"

int canbus_np_iface_open(const char *if_name)
{
	struct sockaddr_can addr;
	struct ifreq ifr;
	int fd, ret = -EINVAL;

	fd = socket(PF_CAN, SOCK_RAW, CAN_RAW);
	if (fd < 0) {
		return -errno;
	}

	(void)memset(&ifr, 0, sizeof(ifr));
	(void)memset(&addr, 0, sizeof(addr));

#ifdef __linux
	strncpy(ifr.ifr_name, if_name, IFNAMSIZ);

	ret = ioctl(fd, SIOCGIFINDEX, (void *)&ifr);
	if (ret < 0) {
		ret = -errno;
		close(fd);
		return ret;
	}

	/* Setup address for bind */
	addr.can_ifindex = ifr.ifr_ifindex;
	addr.can_family = PF_CAN;

	/* bind socket to the zcan interface */
	ret = bind(fd, (struct sockaddr *)&addr, sizeof(addr));
	if (ret < 0) {
		ret = -errno;
		close(fd);
		return ret;
	}
#endif

	return fd;
}

int canbus_np_iface_remove(int fd)
{
	return close(fd);
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

	posix_print_trace("%s\n", cmd);

	ret = system(cmd);

	return -WEXITSTATUS(ret);
}

int canbus_np_wait_data(int fd)
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
		return -errno;
	} else if (ret > 0) {
		if (FD_ISSET(fd, &rset)) {
			return 0;
		}
	}

	return -EAGAIN;
}

ssize_t canbus_np_read_data(int fd, void *buf, size_t buf_len)
{
	return read(fd, buf, buf_len);
}

ssize_t canbus_np_write_data(int fd, void *buf, size_t buf_len)
{
	return write(fd, buf, buf_len);
}

int canbus_np_setsockopt(int fd, int level, int optname,
			 const void *optval, socklen_t optlen)
{
	return setsockopt(fd, level, optname, optval, optlen);
}

int canbus_np_getsockopt(int fd, int level, int optname,
			 void *optval, socklen_t *optlen)
{
	return getsockopt(fd, level, optname, optval, optlen);
}

#if defined(CONFIG_NET_PROMISCUOUS_MODE)
int canbus_np_promisc_mode(const char *if_name, bool enable)
{
	return ssystem("ip link set dev %s promisc %s",
		       if_name, enable ? "on" : "off");
}
#endif /* CONFIG_NET_PROMISCUOUS_MODE */

/* If we have enabled manual setup, then interface cannot be
 * taken up or down by the driver as we normally do not have
 * enough permissions.
 */

int canbus_np_if_up(const char *if_name)
{
	return ssystem("ip link set dev %s up", if_name);
}

int canbus_np_if_down(const char *if_name)
{
	return ssystem("ip link set dev %s down", if_name);
}
