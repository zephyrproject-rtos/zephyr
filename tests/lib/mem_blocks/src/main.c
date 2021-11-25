/*
 * Copyright (c) 2021 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <ztest.h>

#include <sys/mem_blocks.h>
#include <sys/util.h>

#define BLK_SZ     64
#define NUM_BLOCKS 4

SYS_MEM_BLOCKS_DEFINE(mem_block_01, BLK_SZ, NUM_BLOCKS, 4);

static uint8_t mem_block_02_buf[BLK_SZ * NUM_BLOCKS];
SYS_MEM_BLOCKS_DEFINE_STATIC_WITH_EXT_BUF(mem_block_02,
					  BLK_SZ, NUM_BLOCKS,
					  mem_block_02_buf);

static sys_multi_mem_blocks_t alloc_group;

sys_mem_blocks_t *choice_fn(struct sys_multi_mem_blocks *group, void *cfg)
{
	/* mem_block_"01" or mem_block_"02" */
	uintptr_t num = POINTER_TO_UINT(cfg) - 1;

	if (num >= group->num_allocators) {
		return NULL;
	} else {
		return group->allocators[num];
	}
}

static bool check_buffer_bound(sys_mem_blocks_t *mem_block, void *ptr)
{
	uint8_t *start, *end, *ptr_u8;

	start = mem_block->buffer;
	end = start + (BIT(mem_block->blk_sz_shift) * mem_block->num_blocks);

	ptr_u8 = (uint8_t *)ptr;

	if ((ptr_u8 >= start) && (ptr_u8 < end)) {
		return true;
	} else {
		return false;
	}
}

static void alloc_free(sys_mem_blocks_t *mem_block,
		       int num_blocks, int num_iters)
{
	int i, j, ret;
	void *blocks[NUM_BLOCKS][1];
	int val;

	for (j = 0; j < num_iters; j++) {
		for (i = 0; i < num_blocks; i++) {
			ret = sys_mem_blocks_alloc(mem_block, 1, blocks[i]);
			zassert_equal(ret, 0,
				      "sys_mem_blocks_alloc failed (%d)", ret);

			zassert_true(check_buffer_bound(mem_block, blocks[i][0]),
				     "allocated memory is out of bound");

			ret = sys_bitarray_test_bit(mem_block->bitmap,
						    i, &val);
			zassert_equal(val, 1,
				      "sys_mem_blockss_alloc bitmap failed");
		}

		if (num_blocks >= NUM_BLOCKS) {
			ret = sys_mem_blocks_alloc(mem_block, 1, blocks[i]);
			zassert_equal(ret, -ENOMEM,
				"sys_mem_blocks_alloc should fail with -ENOMEM but not");
		}

		for (i = 0; i < num_blocks; i++) {
			ret = sys_mem_blocks_free(mem_block, 1, blocks[i]);
			zassert_equal(ret, 0,
				      "sys_mem_blocks_free failed (%d)", ret);

			ret = sys_bitarray_test_bit(mem_block->bitmap,
						    i, &val);
			zassert_equal(val, 0,
				      "sys_mem_blocks_free bitmap failed");
		}
	}
}

static void test_mem_block_alloc_free(void)
{
	alloc_free(&mem_block_01, 1, 1);
}

static void test_mem_block_alloc_free_alt_buf(void)
{
	alloc_free(&mem_block_02, 1, 1);
}

static void test_mem_block_multi_alloc_free(void)
{
	alloc_free(&mem_block_01, NUM_BLOCKS, 10);
}

static void test_mem_block_multi_alloc_free_alt_buf(void)
{
	alloc_free(&mem_block_02, NUM_BLOCKS, 10);
}

static void test_multi_mem_block_alloc_free(void)
{
	int ret;
	void *blocks[2][1] = {0};
	size_t blk_size;

	ret = sys_multi_mem_blocks_alloc(&alloc_group, UINT_TO_POINTER(16),
					 1, blocks[0], &blk_size);
	zassert_equal(ret, -EINVAL,
		      "sys_multi_mem_blocks_alloc should fail with -EINVAL but not");

	ret = sys_multi_mem_blocks_alloc(&alloc_group, UINT_TO_POINTER(1),
					 1, blocks[0], &blk_size);
	zassert_equal(ret, 0,
		      "sys_multi_mem_blocks_alloc failed (%d)", ret);
	zassert_true(check_buffer_bound(&mem_block_01, blocks[0][0]),
		     "allocated memory is out of bound");
	zassert_equal(blk_size, BLK_SZ,
		      "returned block size is not %d", BLK_SZ);

	ret = sys_multi_mem_blocks_alloc(&alloc_group, UINT_TO_POINTER(2),
					 1, blocks[1], &blk_size);
	zassert_equal(ret, 0,
		      "sys_multi_mem_blocks_alloc failed (%d)", ret);
	zassert_true(check_buffer_bound(&mem_block_02, blocks[1][0]),
		     "allocated memory is out of bound");
	zassert_equal(blk_size, BLK_SZ,
		      "returned block size is not %d", BLK_SZ);

	ret = sys_multi_mem_blocks_free(&alloc_group, 1, blocks[0]);
	zassert_equal(ret, 0,
		      "sys_multi_mem_blocks_free failed (%d)", ret);

	ret = sys_multi_mem_blocks_free(&alloc_group, 1, blocks[1]);
	zassert_equal(ret, 0,
		      "sys_multi_mem_blocks_free failed (%d)", ret);
}

static void test_mem_block_invalid_params(void)
{
	int ret;
	void *blocks[2] = {0};

	ret = sys_mem_blocks_alloc(NULL, 1, blocks);
	zassert_equal(ret, -EINVAL,
		      "sys_mem_blocks_alloc should fail with -EINVAL but not");

	ret = sys_mem_blocks_alloc(&mem_block_01, 1, NULL);
	zassert_equal(ret, -EINVAL,
		      "sys_mem_blocks_alloc should fail with -EINVAL but not");

	ret = sys_mem_blocks_alloc(&mem_block_01, 0, blocks);
	zassert_equal(ret, 0,
		      "sys_mem_blocks_alloc failed (%d)", ret);

	ret = sys_mem_blocks_alloc(&mem_block_01, NUM_BLOCKS + 1, blocks);
	zassert_equal(ret, -ENOMEM,
		      "sys_mem_blocks_alloc should fail with -ENOMEM but not");

	ret = sys_mem_blocks_alloc(&mem_block_01, 1, blocks);
	zassert_equal(ret, 0,
		      "sys_mem_blocks_alloc failed (%d)", ret);

	ret = sys_mem_blocks_free(NULL, 1, blocks);
	zassert_equal(ret, -EINVAL,
		      "sys_mem_blocks_free should fail with -EINVAL but not");

	ret = sys_mem_blocks_free(&mem_block_01, 1, NULL);
	zassert_equal(ret, -EINVAL,
		      "sys_mem_blocks_free should fail with -EINVAL but not");

	ret = sys_mem_blocks_free(&mem_block_01, 0, blocks);
	zassert_equal(ret, 0,
		      "sys_mem_blocks_free failed (%d)", ret);

	ret = sys_mem_blocks_free(&mem_block_01, NUM_BLOCKS + 1, blocks);
	zassert_equal(ret, -EINVAL,
		      "sys_mem_blocks_free should fail with -EINVAL but not");

	ret = sys_mem_blocks_free(&mem_block_01, 1, blocks);
	zassert_equal(ret, 0,
		      "sys_mem_blocks_free failed (%d)", ret);

	ret = sys_mem_blocks_free(&mem_block_01, 1, blocks);
	zassert_equal(ret, -EFAULT,
		      "sys_mem_blocks_free should fail with -EFAULT but not");

	/* Fake a pointer */
	blocks[0] = mem_block_01.buffer +
			(BIT(mem_block_01.blk_sz_shift) * mem_block_01.num_blocks);
	ret = sys_mem_blocks_free(&mem_block_01, 1, blocks);
	zassert_equal(ret, -EFAULT,
		      "sys_mem_blocks_free should fail with -EFAULT but not");
}

static void test_multi_mem_block_invalid_params(void)
{
	int ret;
	void *blocks[2] = {0};

	ret = sys_multi_mem_blocks_alloc(NULL, UINT_TO_POINTER(16),
					 1, blocks, NULL);
	zassert_equal(ret, -EINVAL,
		      "sys_multi_mem_blocks_alloc should fail with -EINVAL but not");

	ret = sys_multi_mem_blocks_alloc(&alloc_group, UINT_TO_POINTER(16),
					 1, NULL, NULL);
	zassert_equal(ret, -EINVAL,
		      "sys_multi_mem_blocks_alloc should fail with -EINVAL but not");

	ret = sys_multi_mem_blocks_alloc(&alloc_group, UINT_TO_POINTER(16),
					 0, blocks, NULL);
	zassert_equal(ret, 0,
		      "sys_multi_mem_blocks_alloc failed (%d)", ret);

	ret = sys_multi_mem_blocks_alloc(&alloc_group, UINT_TO_POINTER(1),
					 NUM_BLOCKS + 1, blocks, NULL);
	zassert_equal(ret, -ENOMEM,
		      "sys_multi_mem_blocks_alloc should fail with -ENOMEM but not");

	ret = sys_multi_mem_blocks_alloc(&alloc_group, UINT_TO_POINTER(1),
					 1, blocks, NULL);
	zassert_equal(ret, 0,
		      "sys_multi_mem_blocks_alloc failed (%d)", ret);

	ret = sys_multi_mem_blocks_free(NULL, 1, blocks);
	zassert_equal(ret, -EINVAL,
		      "sys_multi_mem_blocks_free should fail with -EINVAL but not");

	ret = sys_multi_mem_blocks_free(&alloc_group, 1, NULL);
	zassert_equal(ret, -EINVAL,
		      "sys_multi_mem_blocks_free should fail with -EINVAL but not");

	ret = sys_multi_mem_blocks_free(&alloc_group, 0, blocks);
	zassert_equal(ret, 0,
		      "sys_multi_mem_blocks_free failed (%d)", ret);

	ret = sys_multi_mem_blocks_free(&alloc_group, NUM_BLOCKS + 1, blocks);
	zassert_equal(ret, -EINVAL,
		      "sys_multi_mem_blocks_free should fail with -EINVAL but not");

	ret = sys_multi_mem_blocks_free(&alloc_group, 1, blocks);
	zassert_equal(ret, 0,
		      "sys_multi_mem_blocks_free failed (%d)", ret);

	ret = sys_multi_mem_blocks_free(&alloc_group, 1, blocks);
	zassert_equal(ret, -EFAULT,
		      "sys_multi_mem_blocks_free should fail with -EFAULT but not");

	/* Fake a pointer */
	blocks[0] = mem_block_01.buffer +
			(BIT(mem_block_01.blk_sz_shift) * mem_block_01.num_blocks);
	ret = sys_multi_mem_blocks_free(&alloc_group, 1, blocks);
	zassert_equal(ret, -EINVAL,
		      "sys_multi_mem_blocks_free should fail with -EINVAL but not");
}

void test_main(void)
{
	sys_multi_mem_blocks_init(&alloc_group, choice_fn);
	sys_multi_mem_blocks_add_allocator(&alloc_group, &mem_block_01);
	sys_multi_mem_blocks_add_allocator(&alloc_group, &mem_block_02);

	ztest_test_suite(lib_mem_block_test,
			 ztest_unit_test(test_mem_block_alloc_free),
			 ztest_unit_test(test_mem_block_alloc_free_alt_buf),
			 ztest_unit_test(test_mem_block_multi_alloc_free),
			 ztest_unit_test(test_mem_block_multi_alloc_free_alt_buf),
			 ztest_unit_test(test_multi_mem_block_alloc_free),
			 ztest_unit_test(test_mem_block_invalid_params),
			 ztest_unit_test(test_multi_mem_block_invalid_params)
			 );

	ztest_run_test_suite(lib_mem_block_test);
}
