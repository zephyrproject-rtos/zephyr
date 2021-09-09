/*
 * Copyright (c) 2021 SILA Embedded Solutions GmbH <office@embedded-solutions.at>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <string.h>
#include <errno.h>
#include <sys/__assert.h>
#include <ztest.h>
#include <fs/fs.h>
#include <fs/fs_sys.h>
#include "test_littlefs_automount.h"

#define DT_DRV_COMPAT zephyr_fstab_littlefs
#define FSTAB_ENTRY_FROM_INST(inst) (&FS_FSTAB_ENTRY(DT_DRV_INST(inst))),
#define FSTAB_ENTRY_DECLARE_FROM_INST(inst) FS_FSTAB_DECLARE_ENTRY(DT_DRV_INST(inst));

DT_INST_FOREACH_STATUS_OKAY(FSTAB_ENTRY_DECLARE_FROM_INST)

static struct fs_mount_t *lfs_partitions[] = {
	DT_INST_FOREACH_STATUS_OKAY(FSTAB_ENTRY_FROM_INST)
};

void test_automount_opendir(void)
{
	int ret;
	struct fs_dir_t directory;
	struct fs_mount_t *mount_point = lfs_partitions[0];

	fs_dir_t_init(&directory);
	ret = fs_opendir(&directory, mount_point->mnt_point);
	zassert_equal(ret, 0, "failed to open directory of mount point");
}

void test_automount_check_mounted(void)
{
	int ret;
	struct fs_mount_t *mount_point;

	mount_point = lfs_partitions[0];

	ret = fs_unmount(mount_point);
	zassert_equal(ret, 0, "failed to unmount lfs");

	ret = fs_mount(mount_point);
	zassert_equal(ret, 0, "failed to re-mount lfs");
}
