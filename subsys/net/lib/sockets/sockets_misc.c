/*
 * Copyright (c) 2019 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <net/socket.h>
#include <syscall_handler.h>

int z_impl_zsock_gethostname(char *buf, size_t len)
{
	const char *p = net_hostname_get();

	strncpy(buf, p, len);

	return 0;
}

#ifdef CONFIG_USERSPACE
Z_SYSCALL_HANDLER(zsock_gethostname, buf, len)
{
	Z_OOPS(Z_SYSCALL_MEMORY_WRITE(buf, len));
	return z_impl_zsock_gethostname((char *)buf, len);
}
#endif
