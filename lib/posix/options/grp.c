/*
 * Copyright (c) 2024 Meta Platforms
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>

#include <zephyr/sys/util.h>
#include <zephyr/posix/grp.h>

#ifdef CONFIG_POSIX_THREAD_SAFE_FUNCTIONS

int getgrnam_r(const char *name, struct group *grp, char *buffer, size_t bufsize,
	       struct group **result)
{
	ARG_UNUSED(name);
	ARG_UNUSED(grp);
	ARG_UNUSED(buffer);
	ARG_UNUSED(bufsize);
	ARG_UNUSED(result);

	return ENOSYS;
}

int getgrgid_r(gid_t gid, struct group *grp, char *buffer, size_t bufsize, struct group **result)
{
	ARG_UNUSED(gid);
	ARG_UNUSED(grp);
	ARG_UNUSED(buffer);
	ARG_UNUSED(bufsize);
	ARG_UNUSED(result);

	return ENOSYS;
}

#endif /* CONFIG_POSIX_THREAD_SAFE_FUNCTIONS */
