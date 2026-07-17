/*
 * Copyright (c) 2023 Meta
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief System name structure.
 * @ingroup posix
 *
 * Defines the utsname structure identifying the operating system name, release,
 * version, hostname, and hardware type, and the uname() function to retrieve it.
 *
 * @posix_header{sys_utsname.h}
 */

#ifndef ZEPHYR_INCLUDE_POSIX_SYS_UTSNAME_H_
#define ZEPHYR_INCLUDE_POSIX_SYS_UTSNAME_H_

#include <zephyr/sys/util_macro.h>

#ifdef __cplusplus
extern "C" {
#endif

/* These are for compatibility / practicality */

/**
 * @brief Length of the nodename member.
 */
#define _UTSNAME_NODENAME_LENGTH                                                                   \
	COND_CODE_1(CONFIG_POSIX_SINGLE_PROCESS, (CONFIG_POSIX_UNAME_VERSION_LEN), (0))

/**
 * @brief Length of the version member.
 */
#define _UTSNAME_VERSION_LENGTH                                                                    \
	COND_CODE_1(CONFIG_POSIX_SINGLE_PROCESS, (CONFIG_POSIX_UNAME_VERSION_LEN), (0))

/**
 * @brief System identification information.
 */
struct utsname {
	/**
	 * @brief Operating system name.
	 */
	char sysname[sizeof("Zephyr")];
	/**
	 * @brief Network node hostname.
	 */
	char nodename[_UTSNAME_NODENAME_LENGTH + 1];
	/**
	 * @brief Operating system release level.
	 */
	char release[sizeof("99.99.99-rc1")];
	/**
	 * @brief Operating system version level.
	 */
	char version[_UTSNAME_VERSION_LENGTH + 1];
	/**
	 * @brief Hardware type name.
	 */
	char machine[sizeof(CONFIG_ARCH)];
};

/**
 * @brief Get the name of the current system.
 *
 * @param name Output: system identification structure to fill in.
 *
 * @return 0 on success, or -1 with errno set on failure.
 *
 * @posix_func{uname}
 */
int uname(struct utsname *name);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_POSIX_SYS_UTSNAME_H_ */
