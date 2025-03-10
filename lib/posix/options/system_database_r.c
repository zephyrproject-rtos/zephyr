/*
 * Copyright (c) 2024 Tenstorrent AI ULC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "system_database_priv.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <zephyr/sys/util.h>
#include <zephyr/posix/grp.h>
#include <zephyr/posix/pwd.h>

int getpwnam_r(const char *name, struct passwd *pwd, char *buffer, size_t bufsize,
	       struct passwd **result)
{
	return __getpw_r(name, -1, pwd, buffer, bufsize, result);
}

int getpwuid_r(uid_t uid, struct passwd *pwd, char *buffer, size_t bufsize, struct passwd **result)
{
	return __getpw_r(NULL, uid, pwd, buffer, bufsize, result);
}

int getgrnam_r(const char *name, struct group *grp, char *buffer, size_t bufsize,
	       struct group **result)
{
	return __getgr_r(name, -1, grp, buffer, bufsize, result);
}

int getgrgid_r(gid_t gid, struct group *grp, char *buffer, size_t bufsize, struct group **result)
{
	return __getgr_r(NULL, gid, grp, buffer, bufsize, result);
}
