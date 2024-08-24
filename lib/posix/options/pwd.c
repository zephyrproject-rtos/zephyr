/*
 * Copyright (c) 2024 Meta Platforms
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>

#include <zephyr/sys/util.h>
#include <zephyr/posix/pwd.h>

#ifdef CONFIG_POSIX_THREAD_SAFE_FUNCTIONS

int getpwnam_r(const char *nam, struct passwd *pwd, char *buffer, size_t bufsize,
	       struct passwd **result)
{
	ARG_UNUSED(nam);
	ARG_UNUSED(pwd);
	ARG_UNUSED(buffer);
	ARG_UNUSED(bufsize);
	ARG_UNUSED(result);

	return ENOSYS;
}

int getpwuid_r(uid_t uid, struct passwd *pwd, char *buffer, size_t bufsize, struct passwd **result)
{
	ARG_UNUSED(uid);
	ARG_UNUSED(pwd);
	ARG_UNUSED(buffer);
	ARG_UNUSED(bufsize);
	ARG_UNUSED(result);

	return ENOSYS;
}

#endif /* CONFIG_POSIX_THREAD_SAFE_FUNCTIONS */
