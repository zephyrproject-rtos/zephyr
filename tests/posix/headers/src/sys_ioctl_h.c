/*
 * Copyright (c) 2022 Meta
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "_common.h"

#ifdef CONFIG_POSIX_API
#include <sys/ioctl.h>
#else
#include <zephyr/posix/sys/ioctl.h>
#endif

/**
 * @brief existence test for `<sys/ioctl.h>`
 *
 * @see <a href="https://pubs.opengroup.org/onlinepubs/9699919799/functions/ioctl.html">ioctl</a>
 * @see <a href="https://man7.org/linux/man-pages/man2/ioctl.2.html">ioctl(2)</a>
 */
ZTEST(posix_headers, test_sys_ioctl_h)
{
	if (IS_ENABLED(CONFIG_POSIX_API)) {
		zassert_not_null(ioctl);
	}
}
