/*
 * Copyright (c) 2023 Meta
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_POSIX_SYS_UTSNAME_H_
#define ZEPHYR_INCLUDE_POSIX_SYS_UTSNAME_H_

#ifdef __cplusplus
extern "C" {
#endif

struct utsname {
	char sysname[sizeof("Zephyr")];
	char nodename[CONFIG_POSIX_UNAME_NODENAME_LEN + 1];
	char release[sizeof("99.99.99-rc1")];
	char version[CONFIG_POSIX_UNAME_VERSION_LEN + 1];
	char machine[sizeof(CONFIG_ARCH)];
};

int uname(struct utsname *name);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_POSIX_SYS_UTSNAME_H_ */
