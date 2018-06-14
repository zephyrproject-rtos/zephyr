/*
 * Copyright (c) 2018 Codecoup
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "test_fat.h"
#include "test_nffs.h"
#include "test_fs_shell.h"

static struct nffs_area_desc nffs_selftest_area_descs[] = {
	{ 0x00000000, 16 * 1024 },
	{ 0x00004000, 16 * 1024 },
	{ 0x00008000, 16 * 1024 },
	{ 0x0000c000, 16 * 1024 },
	{ 0x00010000, 64 * 1024 },
	{ 0x00020000, 128 * 1024 },
	{ 0x00040000, 128 * 1024 },
	{ 0x00060000, 128 * 1024 },
	{ 0x00080000, 128 * 1024 },
	{ 0x000a0000, 128 * 1024 },
	{ 0x000c0000, 128 * 1024 },
	{ 0x000e0000, 128 * 1024 },
	{ 0, 0 },
};

static struct nffs_area_desc *save_area_descs;

static void test_setup(void)
{
	save_area_descs = nffs_current_area_descs;
	nffs_current_area_descs = nffs_selftest_area_descs;
}

static void test_teardown(void)
{
	nffs_current_area_descs = save_area_descs;
}

void test_main(void)
{
	ztest_test_suite(multifs_fs_test,
			 ztest_unit_test(test_fat_mount),
			 ztest_unit_test_setup_teardown(test_nffs_mount,
							test_setup, test_teardown),
			 ztest_unit_test(test_fat_mkdir),
			 ztest_unit_test_setup_teardown(test_nffs_mkdir,
							test_setup, test_teardown),
			 ztest_unit_test(test_fat_readdir),
			 ztest_unit_test(test_fat_rmdir),
			 ztest_unit_test_setup_teardown(test_nffs_readdir,
							test_setup, test_teardown),
			 ztest_unit_test(test_fat_open),
			 ztest_unit_test_setup_teardown(test_nffs_open,
							test_setup, test_teardown),
			 ztest_unit_test(test_fat_write),
			 ztest_unit_test_setup_teardown(test_nffs_write,
							test_setup, test_teardown),
			 ztest_unit_test(test_fat_read),
			 ztest_unit_test(test_fat_close),
			 ztest_unit_test_setup_teardown(test_nffs_read,
							test_setup, test_teardown),
			 ztest_unit_test(test_fat_unlink),
			 ztest_unit_test_setup_teardown(test_nffs_unlink,
							test_setup, test_teardown),
			 ztest_unit_test(test_fs_help),
			 ztest_unit_test(test_fs_shell_exit));
	ztest_run_test_suite(multifs_fs_test);
}
