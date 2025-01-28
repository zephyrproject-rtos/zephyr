/*
 * Copyright (c) 2024 Tenstorrent AI ULC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stddef.h>

#include <zephyr/posix/grp.h>
#include <zephyr/posix/pwd.h>

int __getgr_r(const char *name, gid_t gid, struct group *grp, char *buffer, size_t bufsize,
	      struct group **result);
int __getpw_r(const char *name, uid_t uid, struct passwd *pwd, char *buffer, size_t bufsize,
	      struct passwd **result);
