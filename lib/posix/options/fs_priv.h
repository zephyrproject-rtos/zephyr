/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_LIB_POSIX_OPTIONS_FS_PRIV_H_
#define ZEPHYR_LIB_POSIX_OPTIONS_FS_PRIV_H_

#include <stdbool.h>

#include <zephyr/fs/fs.h>

struct posix_fs_desc {
	union {
		struct fs_file_t file;
		struct fs_dir_t dir;
	};
	bool is_dir: 1;
	bool used: 1;
};

#endif
