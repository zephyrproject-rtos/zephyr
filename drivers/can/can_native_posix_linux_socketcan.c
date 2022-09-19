/*
 * Copyright (c) 2019 Intel Corporation
 * Copyright (c) 2022 Martin Jäger <martin@libre.solar>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 *
 * Routines setting up the host system. Those are placed in separate file
 * because there is naming conflicts between host and zephyr network stacks.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <errno.h>
#include <string.h>
#include <stdbool.h>

/* Linux host include files. */
#ifdef __linux
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <net/if.h>
#include <linux/can.h>
#include <linux/can/raw.h>
#else
#error "This driver can only be built on Linux systems"
#endif

#include "can_native_posix_linux_socketcan.h"

int linux_socketcan_iface_open(const char *if_name)
{
	struct sockaddr_can addr;
	struct ifreq ifr;
	int fd, opt, ret = -EINVAL;

	fd = socket(PF_CAN, SOCK_RAW, CAN_RAW);
	if (fd < 0) {
		return -errno;
	}

	(void)memset(&ifr, 0, sizeof(ifr));
	(void)memset(&addr, 0, sizeof(addr));

	strncpy(ifr.ifr_name, if_name, IFNAMSIZ);

	ret = ioctl(fd, SIOCGIFINDEX, (void *)&ifr);
	if (ret < 0) {
		close(fd);
		return -errno;
	}

	addr.can_ifindex = ifr.ifr_ifindex;
	addr.can_family = PF_CAN;

	ret = bind(fd, (struct sockaddr *)&addr, sizeof(addr));
	if (ret < 0) {
		close(fd);
		return -errno;
	}

	/* this option must always be enabled in order to receive TX confirmations */
	opt = 1;
	ret = setsockopt(fd, SOL_CAN_RAW, CAN_RAW_RECV_OWN_MSGS, &opt, sizeof(opt));
	if (ret < 0) {
		close(fd);
		return -errno;
	}

	return fd;
}

int linux_socketcan_iface_close(int fd)
{
	return close(fd);
}

int linux_socketcan_poll_data(int fd)
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

ssize_t linux_socketcan_read_data(int fd, void *buf, size_t buf_len, bool *msg_confirm)
{
	struct canfd_frame *frame = (struct canfd_frame *)buf;
	struct msghdr msg = {0};

	struct iovec iov = {
		.iov_base = buf,
		.iov_len = buf_len,
	};
	msg.msg_iov = &iov;
	msg.msg_iovlen = 1;

	int ret = recvmsg(fd, &msg, MSG_WAITALL);

	if (msg_confirm != NULL) {
		*msg_confirm = (msg.msg_flags & MSG_CONFIRM) != 0;
	}

	/* Make sure to set the flags for all frames received via the Linux API.
	 *
	 * Zephyr relies on defined flags field of the SocketCAN data for both FD and classical CAN
	 * frames. In Linux the flags field is undefined for legacy frames.
	 */
	if (ret == CANFD_MTU) {
		frame->flags |= CANFD_FDF;
	} else if (ret == CAN_MTU) {
		frame->flags = 0;
	}

	return ret;
}

ssize_t linux_socketcan_write_data(int fd, void *buf, size_t buf_len)
{
	return write(fd, buf, buf_len);
}

int linux_socketcan_setsockopt(int fd, int level, int optname,
			       const void *optval, socklen_t optlen)
{
	return setsockopt(fd, level, optname, optval, optlen);
}

int linux_socketcan_getsockopt(int fd, int level, int optname,
			       void *optval, socklen_t *optlen)
{
	return getsockopt(fd, level, optname, optval, optlen);
}

int linux_socketcan_set_mode_fd(int fd, bool mode_fd)
{
	int opt = mode_fd ? 1 : 0;

	return setsockopt(fd, SOL_CAN_RAW, CAN_RAW_FD_FRAMES, &opt, sizeof(opt));
}
