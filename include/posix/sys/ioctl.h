/*
 * Copyright (c) 2019 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_POSIX_SYS_IOCTL_H_
#define ZEPHYR_INCLUDE_POSIX_SYS_IOCTL_H_

#include <stdarg.h>

__syscall int sys_ioctl(int fd, unsigned long request, va_list args);
int ioctl(int fd, unsigned long request, ...);

#define FIONBIO 0x5421

#ifndef CONFIG_ARCH_POSIX
#include <syscalls/ioctl.h>
#endif /* CONFIG_ARCH_POSIX */

#endif /* ZEPHYR_INCLUDE_POSIX_SYS_IOCTL_H_ */
