/*
 * Copyright (c) 2017 Codecoup
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <ztest.h>
#include <fs/fs.h>

void test_fs_mount(void);
void test_mkdir(void);
void test_gc_on_oom(void);
void test_incomplete_block(void);
void test_lost_found(void);
void test_readdir(void);
void test_large_system(void);
void test_corrupt_block(void);
void test_split_file(void);
void test_large_unlink(void);
void test_corrupt_scratch(void);
void test_unlink(void);
void test_append(void);
void test_rename(void);
void test_truncate(void);
void test_open(void);
void test_wear_level(void);
void test_long_filename(void);
void test_overwrite_one(void);
void test_many_children(void);
void test_gc(void);
void test_overwrite_many(void);
void test_overwrite_two(void);
void test_overwrite_three(void);
void test_read(void);
void test_cache_large_file(void);
void test_large_write(void);
void test_performance(void);

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
