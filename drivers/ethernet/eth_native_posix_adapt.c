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

#define _DEFAULT_SOURCE

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
#include <net/if.h>
#include <time.h>
#include "posix_trace.h"

#ifdef __linux
#include <linux/if_tun.h>
#endif

/* Zephyr include files. Be very careful here and only include minimum
 * things needed.
 */
#define SYS_LOG_DOMAIN "eth-posix-adapt"
#define SYS_LOG_LEVEL CONFIG_SYS_LOG_ETHERNET_LEVEL

#include <zephyr/types.h>
#include <sys_clock.h>
#include <logging/sys_log.h>

#if defined(CONFIG_NET_GPTP)
#include <net/gptp.h>
#endif

#include "eth_native_posix_priv.h"

/* Note that we cannot create the TUN/TAP device from the setup script
 * as we need to get a file descriptor to communicate with the interface.
 */
int eth_iface_create(const char *if_name, bool tun_only)
{
	struct ifreq ifr;
	int fd, ret = -EINVAL;

	fd = open(CONFIG_ETH_NATIVE_POSIX_DEV_NAME, O_RDWR);
	if (fd < 0) {
		return -errno;
	}

	(void)memset(&ifr, 0, sizeof(ifr));

#ifdef __linux
	ifr.ifr_flags = (tun_only ? IFF_TUN : IFF_TAP) | IFF_NO_PI;

	strncpy(ifr.ifr_name, if_name, IFNAMSIZ);

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

	posix_print_trace("%s\n", cmd);

	ret = system(cmd);

	return -WEXITSTATUS(ret);
}

int eth_setup_host(const char *if_name)
{
	/* User might have added -i option to setup script string, so
	 * check that situation in the script itself so that the -i option
	 * we add here is ignored in that case.
	 */
	return ssystem("%s -i %s", CONFIG_ETH_NATIVE_POSIX_SETUP_SCRIPT,
		       if_name);
}

int eth_start_script(const char *if_name)
{
	if (CONFIG_ETH_NATIVE_POSIX_STARTUP_SCRIPT[0] == '\0') {
		return 0;
	}

	if (CONFIG_ETH_NATIVE_POSIX_STARTUP_SCRIPT_USER[0] == '\0') {
		return ssystem("%s %s", CONFIG_ETH_NATIVE_POSIX_STARTUP_SCRIPT,
			       if_name);
	} else {
		return ssystem("sudo -u %s %s %s",
			       CONFIG_ETH_NATIVE_POSIX_STARTUP_SCRIPT_USER,
			       CONFIG_ETH_NATIVE_POSIX_STARTUP_SCRIPT,
			       if_name);
	}
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

ssize_t eth_read_data(int fd, void *buf, size_t buf_len)
{
	return read(fd, buf, buf_len);
}

ssize_t eth_write_data(int fd, void *buf, size_t buf_len)
{
	return write(fd, buf, buf_len);
}

#if defined(CONFIG_NET_GPTP)
int eth_clock_gettime(struct net_ptp_time *time)
{
	struct timespec tp;
	int ret;

	ret = clock_gettime(CLOCK_MONOTONIC_RAW, &tp);
	if (ret < 0) {
		return -errno;
	}

	time->second = tp.tv_sec;
	time->nanosecond = tp.tv_nsec;

	return 0;
}
#endif /* CONFIG_NET_GPTP */

#if defined(CONFIG_NET_PROMISCUOUS_MODE)
int eth_promisc_mode(const char *if_name, bool enable)
{
	return ssystem("ip link set dev %s promisc %s",
		       if_name, enable ? "on" : "off");
}
#endif /* CONFIG_NET_PROMISCUOUS_MODE */

int eth_if_up(const char *if_name)
{
	return ssystem("ip link set dev %s up", if_name);
}

int eth_if_down(const char *if_name)
{
	return ssystem("ip link set dev %s down", if_name);
}
