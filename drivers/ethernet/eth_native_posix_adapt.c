/*
 * Copyright (c) 2018 Intel Corporation
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
#include <time.h>
#include <inttypes.h>
#include <nsi_tracing.h>

#ifdef __linux
#include <linux/if.h>
#include <linux/if_tun.h>
#endif

#include "eth_native_posix_priv.h"

/* Note that we cannot create the TUN/TAP device from the setup script
 * as we need to get a file descriptor to communicate with the interface.
 */
int eth_iface_create(const char *dev_name, const char *if_name, bool tun_only)
{
	struct ifreq ifr;
	int fd, ret = -EINVAL;

	fd = open(dev_name, O_RDWR);
	if (fd < 0) {
		return -errno;
	}

	(void)memset(&ifr, 0, sizeof(ifr));

#ifdef __linux
	ifr.ifr_flags = (tun_only ? IFF_TUN : IFF_TAP) | IFF_NO_PI;

	strncpy(ifr.ifr_name, if_name, IFNAMSIZ - 1);

	ret = ioctl(fd, TUNSETIFF, (void *)&ifr);
	if (ret < 0) {
		ret = -errno;
		close(fd);
		return ret;
	}
#endif

	return fd;
}

int eth_iface_remove(int fd)
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

	nsi_print_trace("%s\n", cmd);

	ret = system(cmd);

	return -WEXITSTATUS(ret);
}

int eth_wait_data(int fd)
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

int eth_clock_gettime(uint64_t *second, uint32_t *nanosecond)
{
	struct timespec tp;
	int ret;

	ret = clock_gettime(CLOCK_MONOTONIC_RAW, &tp);
	if (ret < 0) {
		return -errno;
	}

	*second = (uint64_t)tp.tv_sec;
	*nanosecond = (uint32_t)tp.tv_nsec;

	return 0;
}

int eth_promisc_mode(const char *if_name, bool enable)
{
	return ssystem("ip link set dev %s promisc %s",
		       if_name, enable ? "on" : "off");
}
