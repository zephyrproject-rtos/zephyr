/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_TESTS_SUBSYS_FS_VIRTIOFS_TEST_VIRTIOFS_H_
#define ZEPHYR_TESTS_SUBSYS_FS_VIRTIOFS_TEST_VIRTIOFS_H_

#include <zephyr/fs/fs.h>

/* Mount point of the shared host directory. */
#define VIRTIOFS_MNT "/virtiofs"

/* Mounted once by the suite setup, unmounted by the suite teardown. */
extern struct fs_mount_t testfs_mnt;

/* Remove every entry under path so the share starts empty. */
int delete_dir_recursive(const char *path);

#endif /* ZEPHYR_TESTS_SUBSYS_FS_VIRTIOFS_TEST_VIRTIOFS_H_ */
