/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/device.h>
#include <zephyr/fs/fs.h>
#include <zephyr/fs/virtiofs.h>
#include "test_virtiofs.h"

#define VIRTIO_DEV DEVICE_DT_GET(DT_NODELABEL(virtio_pci))

static struct virtiofs_fs_data fs_data;

/*
 * The host share is exported by the virtiofsd daemon that the twister virtiofs
 * harness starts before QEMU boots. The guest reaches it through the
 * vhost-user-fs PCI device described in the board overlay.
 *
 * virtiofsd keeps a single FUSE session per connection: mounting sends
 * FUSE_INIT and unmounting sends FUSE_DESTROY, after which the session cannot
 * be re-initialised. The suite therefore mounts once in setup() and unmounts
 * once in teardown(), and the individual tests operate on that single mount.
 */
struct fs_mount_t testfs_mnt = {
	.type = FS_VIRTIOFS,
	.fs_data = &fs_data,
	.flags = 0,
	.storage_dev = (void *)VIRTIO_DEV,
	.mnt_point = VIRTIOFS_MNT,
};

static void *virtiofs_setup(void)
{
	zassert_ok(fs_mount(&testfs_mnt), "mount failed");
	/* Start from a known empty share regardless of host contents. */
	zassert_ok(delete_dir_recursive(VIRTIOFS_MNT), "wiping share failed");

	return NULL;
}

static void virtiofs_teardown(void *f)
{
	ARG_UNUSED(f);

	(void)fs_unmount(&testfs_mnt);
}

ZTEST_SUITE(virtiofs, NULL, virtiofs_setup, NULL, NULL, virtiofs_teardown);
