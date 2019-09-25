/*
 * Copyright (c) 2019 Peter Bigot Consulting, LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <fs/fs.h>
#include "fs_impl.h"

const char *fs_impl_strip_prefix(const char *path,
				 const struct fs_mount_t *mp)
{
	static const char *const root = "/";

	if ((path == NULL) || (mp == NULL)) {
		return path;
	}

	path += mp->mountp_len;
	return *path ? path : root;
}
