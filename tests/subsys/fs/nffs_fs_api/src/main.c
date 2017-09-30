/*
 * Copyright (c) 2017 Codecoup
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <nffs/nffs.h>
#include "test_nffs.h"

#if CONFIG_BOARD_QEMU_X86
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
#else
static struct nffs_area_desc nffs_selftest_area_descs[] = {
	{ 0x00020000, 2 * 4096 },
	{ 0x00022000, 2 * 4096 },
	{ 0x00024000, 2 * 4096 },
	{ 0x00026000, 2 * 4096 },
	{ 0x00028000, 2 * 4096 },
	{ 0x0002a000, 2 * 4096 },
	{ 0x0002c000, 2 * 4096 },
	{ 0x00030000, 2 * 4096 },
	{ 0x00032000, 2 * 4096 },
	{ 0x00034000, 2 * 4096 },
	{ 0x00036000, 2 * 4096 },
	{ 0x00038000, 2 * 4096 },
	{ 0, 0 },
};
#endif

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
#ifdef TEST_basic
	ztest_test_suite(nffs_fs_basic_test,
		ztest_unit_test_setup_teardown(test_unlink,
					       test_setup, test_teardown),
		ztest_unit_test_setup_teardown(test_mkdir,
					       test_setup, test_teardown),
		ztest_unit_test_setup_teardown(test_rename,
					       test_setup, test_teardown),
		ztest_unit_test_setup_teardown(test_append,
					       test_setup, test_teardown),
		ztest_unit_test_setup_teardown(test_read,
					       test_setup, test_teardown),
		ztest_unit_test_setup_teardown(test_open,
					       test_setup, test_teardown),
		ztest_unit_test_setup_teardown(test_overwrite_one,
					       test_setup, test_teardown),
		ztest_unit_test_setup_teardown(test_overwrite_two,
					       test_setup, test_teardown),
		ztest_unit_test_setup_teardown(test_overwrite_three,
					       test_setup, test_teardown),
		ztest_unit_test_setup_teardown(test_overwrite_many,
					       test_setup, test_teardown),
		ztest_unit_test_setup_teardown(test_long_filename,
					       test_setup, test_teardown),
		ztest_unit_test_setup_teardown(test_large_write,
					       test_setup, test_teardown),
		ztest_unit_test_setup_teardown(test_many_children,
					       test_setup, test_teardown),
		ztest_unit_test_setup_teardown(test_gc,
					       test_setup, test_teardown),
		ztest_unit_test_setup_teardown(test_wear_level,
					       test_setup, test_teardown),
		ztest_unit_test_setup_teardown(test_corrupt_scratch,
					       test_setup, test_teardown),
		ztest_unit_test_setup_teardown(test_incomplete_block,
					       test_setup, test_teardown),
		ztest_unit_test_setup_teardown(test_corrupt_block,
					       test_setup, test_teardown),
		ztest_unit_test_setup_teardown(test_lost_found,
					       test_setup, test_teardown),
		ztest_unit_test_setup_teardown(test_readdir,
					       test_setup, test_teardown),
		ztest_unit_test_setup_teardown(test_split_file,
					       test_setup, test_teardown),
		ztest_unit_test_setup_teardown(test_gc_on_oom,
					       test_setup, test_teardown));
	ztest_run_test_suite(nffs_fs_basic_test);
#endif

#ifdef TEST_large
	ztest_test_suite(nffs_fs_large_test,
		ztest_unit_test_setup_teardown(test_large_unlink,
					       test_setup, test_teardown),
		ztest_unit_test_setup_teardown(test_large_system,
					       test_setup, test_teardown));
	ztest_run_test_suite(nffs_fs_large_test);
#endif

#ifdef TEST_cache
	ztest_test_suite(nffs_fs_cache_test,
		ztest_unit_test_setup_teardown(test_cache_large_file,
					       test_setup, test_teardown));
	ztest_run_test_suite(nffs_fs_cache_test);
#endif

#ifdef TEST_performance
	ztest_test_suite(nffs_fs_performace_test,
		ztest_unit_test_setup_teardown(test_performance,
					       test_setup, test_teardown));
	ztest_run_test_suite(nffs_fs_performace_test);
#endif
}
