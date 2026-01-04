/*
 * Copyright (c) 2020 Tobias Svehagen
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef TESTS_POSIX_TIMERFD_SRC__MAIN_H_
#define TESTS_POSIX_TIMERFD_SRC__MAIN_H_

#include <errno.h>
#include <stdio.h>

#include <zephyr/net/socket.h>
#include <zephyr/posix/fcntl.h>
#include <zephyr/posix/sys/ioctl.h>
#include <zephyr/posix/poll.h>
#include <zephyr/posix/sys/timerfd.h>
#include <zephyr/posix/unistd.h>
#include <zephyr/ztest.h>

#define TESTVAL 10

#ifdef __cplusplus
extern "C" {
#endif

struct timerfd_fixture {
	int fd;
};

void reopen(int *fd, int clockid, int flags);
int is_blocked(int fd, short *event);
void timerfd_poll_set_common(int fd);
void timerfd_poll_unset_common(int fd);

#ifdef __cplusplus
}
#endif

#endif
