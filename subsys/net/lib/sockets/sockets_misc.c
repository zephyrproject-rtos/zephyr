/*
 * Copyright (c) 2019 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <zephyr/net/socket.h>
#include <zephyr/syscall_handler.h>

int z_impl_zsock_gethostname(char *buf, size_t len)
{
	const char *p = net_hostname_get();

	strncpy(buf, p, len);

	return 0;
}

#ifdef CONFIG_USERSPACE
static inline int z_vrfy_zsock_gethostname(char *buf, size_t len)
{
	Z_OOPS(Z_SYSCALL_MEMORY_WRITE(buf, len));
	return z_impl_zsock_gethostname(buf, len);
}
#include <syscalls/zsock_gethostname_mrsh.c>
#endif
