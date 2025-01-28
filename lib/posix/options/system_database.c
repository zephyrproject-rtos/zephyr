/*
 * Copyright (c) 2024 Tenstorrent AI ULC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "system_database_priv.h"

#include <errno.h>
#include <stddef.h>
#include <string.h>

#include <zephyr/posix/grp.h>
#include <zephyr/posix/pwd.h>
#include <zephyr/sys/util.h>

static char gr_line_buf[CONFIG_POSIX_GETGR_R_SIZE_MAX];
static char pw_line_buf[CONFIG_POSIX_GETPW_R_SIZE_MAX];

static struct group gr;
static struct passwd pw;

struct group *getgrgid(gid_t gid)
{
	int ret;
	struct group *result;

	ret = __getgr_r(NULL, gid, &gr, gr_line_buf, sizeof(gr_line_buf), &result);
	if (ret != 0) {
		errno = ret;
		return NULL;
	}

	return result;
}

struct group *getgrnam(const char *name)
{
	int ret;
	struct group *result;

	if (name == NULL) {
		return NULL;
	}

	ret = __getgr_r(name, -1, &gr, gr_line_buf, sizeof(gr_line_buf), &result);
	if (ret != 0) {
		errno = ret;
		return NULL;
	}

	return result;
}

struct passwd *getpwnam(const char *name)
{
	int ret;
	struct passwd *result;

	if (name == NULL) {
		return NULL;
	}

	ret = __getpw_r(name, -1, &pw, pw_line_buf, sizeof(pw_line_buf), &result);
	if (ret != 0) {
		errno = ret;
		return NULL;
	}

	return result;
}

struct passwd *getpwuid(uid_t uid)
{
	int ret;
	struct passwd *result;

	ret = __getpw_r(NULL, uid, &pw, pw_line_buf, sizeof(pw_line_buf), &result);
	if (ret != 0) {
		errno = ret;
		return NULL;
	}

	return result;
}
