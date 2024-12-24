/*
 * Copyright (c) 2024 Tenstorrent AI ULC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>

#include <zephyr/sys/util.h>
#include <zephyr/posix/grp.h>
#include <zephyr/posix/pwd.h>

void endgrent(void)
{
}

void endpwent(void)
{
}

struct group *getgrent(void)
{
	errno = ENOSYS;
	return NULL;
}

struct group *getgrgid(gid_t gid)
{
	errno = ENOSYS;
	return NULL;
}

struct group *getgrnam(const char *name)
{
	errno = ENOSYS;
	return NULL;
}

struct passwd *getpwent(void)
{
	errno = ENOSYS;
	return NULL;
}

struct passwd *getpwnam(const char *name)
{
	errno = ENOSYS;
	return NULL;
}

struct passwd *getpwuid(uid_t uid)
{
	errno = ENOSYS;
	return NULL;
}

void setgrent(void)
{
}

void setpwent(void)
{
}
