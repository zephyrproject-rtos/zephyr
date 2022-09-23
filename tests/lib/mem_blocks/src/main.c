/*
 * Copyright (c) 2021 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/ztest.h>

#include <zephyr/sys/heap_listener.h>
#include <zephyr/sys/mem_blocks.h>
#include <zephyr/sys/util.h>

#define BLK_SZ     64
#define NUM_BLOCKS 8

SYS_MEM_BLOCKS_DEFINE(mem_block_01, BLK_SZ, NUM_BLOCKS, 4);

static uint8_t mem_block_02_buf[BLK_SZ * NUM_BLOCKS];
SYS_MEM_BLOCKS_DEFINE_STATIC_WITH_EXT_BUF(mem_block_02,
					  BLK_SZ, NUM_BLOCKS,
					  mem_block_02_buf);

static sys_multi_mem_blocks_t alloc_group;

static ZTEST_DMEM volatile int expected_reason = -1;

void k_sys_fatal_error_handler(unsigned int reason, const z_arch_esf_t *pEsf)
{
	printk("Caught system error -- reason %d\n", reason);

	if (expected_reason == -1) {
		printk("Was not expecting a crash\n");
		ztest_test_fail();
	}

	if (reason != expected_reason) {
		printk("Wrong crash type got %d expected %d\n", reason,
		       expected_reason);
		ztest_test_fail();
	}

	expected_reason = -1;
	ztest_test_pass();
}

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

#ifdef CONFIG_SYS_MEM_BLOCKS_LISTENER
#define HEAP_LISTENER_LOG_SIZE 64

static uintptr_t listener_heap_id[HEAP_LISTENER_LOG_SIZE];
static void *listener_mem[HEAP_LISTENER_LOG_SIZE];
static size_t listener_size[HEAP_LISTENER_LOG_SIZE];
static uint8_t listener_idx;

static void mem_block_alloc_free_cb(uintptr_t heap_id, void *mem, size_t bytes)
{
	listener_heap_id[listener_idx] = heap_id;
	listener_mem[listener_idx] = mem;
	listener_size[listener_idx] = bytes;

#ifdef CONFIG_DEBUG
	TC_PRINT("[%u] Heap 0x%" PRIxPTR ", alloc %p, size %u\n",
		 listener_idx, heap_id, mem, (uint32_t)bytes);
#endif

	listener_idx++;
}

HEAP_LISTENER_ALLOC_DEFINE(mem_block_01_alloc,
			   HEAP_ID_FROM_POINTER(&mem_block_01),
			   mem_block_alloc_free_cb);

HEAP_LISTENER_FREE_DEFINE(mem_block_01_free,
			  HEAP_ID_FROM_POINTER(&mem_block_01),
			  mem_block_alloc_free_cb);

HEAP_LISTENER_ALLOC_DEFINE(mem_block_02_alloc,
			   HEAP_ID_FROM_POINTER(&mem_block_02),
			   mem_block_alloc_free_cb);

HEAP_LISTENER_FREE_DEFINE(mem_block_02_free,
			  HEAP_ID_FROM_POINTER(&mem_block_02),
			  mem_block_alloc_free_cb);
#endif /* CONFIG_SYS_MEM_BLOCKS_LISTENER */

static void alloc_free(sys_mem_blocks_t *mem_block,
		       int num_blocks, int num_iters)
{
	int i, j, ret;
	void *blocks[NUM_BLOCKS][1];
	int val;

#ifdef CONFIG_SYS_MEM_BLOCKS_LISTENER
	if (mem_block == &mem_block_01) {
		heap_listener_register(&mem_block_01_alloc);
		heap_listener_register(&mem_block_01_free);
	} else if (mem_block == &mem_block_02) {
		heap_listener_register(&mem_block_02_alloc);
		heap_listener_register(&mem_block_02_free);
	}
#endif

	for (j = 0; j < num_iters; j++) {
#ifdef CONFIG_SYS_MEM_BLOCKS_LISTENER
		listener_idx = 0;
#endif

		for (i = 0; i < num_blocks; i++) {
			ret = sys_mem_blocks_alloc(mem_block, 1, blocks[i]);
			zassert_equal(ret, 0,
				      "sys_mem_blocks_alloc failed (%d)", ret);

			zassert_true(check_buffer_bound(mem_block, blocks[i][0]),
				     "allocated memory is out of bound");

			ret = sys_bitarray_test_bit(mem_block->bitmap,
						    i, &val);
			zassert_equal(ret, 0, "API failure");
			zassert_equal(val, 1,
				      "sys_mem_blockss_alloc bitmap failed");

#ifdef CONFIG_SYS_MEM_BLOCKS_LISTENER
			zassert_equal(listener_heap_id[i],
				      HEAP_ID_FROM_POINTER(mem_block),
				      "Heap ID mismatched: 0x%lx != %p",
				      listener_heap_id[i], mem_block);
			zassert_equal(listener_mem[i], blocks[i][0],
				      "Heap allocated pointer mismatched: %p != %p",
				      listener_mem[i], blocks[i][0]);
			zassert_equal(listener_size[i], BIT(mem_block->blk_sz_shift),
				      "Heap allocated sized: %u != %u",
				      listener_size[i], BIT(mem_block->blk_sz_shift));
#endif
		}

		if (num_blocks >= NUM_BLOCKS) {
			ret = sys_mem_blocks_alloc(mem_block, 1, blocks[i]);
			zassert_equal(ret, -ENOMEM,
				"sys_mem_blocks_alloc should fail with -ENOMEM but not");
		}

#ifdef CONFIG_SYS_MEM_BLOCKS_LISTENER
		listener_idx = 0;
#endif

		for (i = 0; i < num_blocks; i++) {
			ret = sys_mem_blocks_free(mem_block, 1, blocks[i]);
			zassert_equal(ret, 0,
				      "sys_mem_blocks_free failed (%d)", ret);

			ret = sys_bitarray_test_bit(mem_block->bitmap,
						    i, &val);
			zassert_equal(ret, 0, "API failure");
			zassert_equal(val, 0,
				      "sys_mem_blocks_free bitmap failed");

#ifdef CONFIG_SYS_MEM_BLOCKS_LISTENER
			zassert_equal(listener_heap_id[i],
				      HEAP_ID_FROM_POINTER(mem_block),
				      "Heap ID mismatched: 0x%lx != %p",
				      listener_heap_id[i], mem_block);
			zassert_equal(listener_mem[i], blocks[i][0],
				      "Heap allocated pointer mismatched: %p != %p",
				      listener_mem[i], blocks[i][0]);
			zassert_equal(listener_size[i], BIT(mem_block->blk_sz_shift),
				      "Heap allocated sized: %u != %u",
				      listener_size[i], BIT(mem_block->blk_sz_shift));
#endif
		}
	}

#ifdef CONFIG_SYS_MEM_BLOCKS_LISTENER
	if (mem_block == &mem_block_01) {
		heap_listener_unregister(&mem_block_01_alloc);
		heap_listener_unregister(&mem_block_01_free);
	} else if (mem_block == &mem_block_02) {
		heap_listener_unregister(&mem_block_02_alloc);
		heap_listener_unregister(&mem_block_02_free);
	}
#endif
}

ZTEST(lib_mem_block, test_mem_block_alloc_free)
{
	alloc_free(&mem_block_01, 1, 1);
}

ZTEST(lib_mem_block, test_mem_block_alloc_free_alt_buf)
{
	alloc_free(&mem_block_02, 1, 1);
}

ZTEST(lib_mem_block, test_mem_block_multi_alloc_free)
{
	alloc_free(&mem_block_01, NUM_BLOCKS, 10);
}

ZTEST(lib_mem_block, test_mem_block_multi_alloc_free_alt_buf)
{
	alloc_free(&mem_block_02, NUM_BLOCKS, 10);
}

ZTEST(lib_mem_block, test_mem_block_get)
{
	int i, ret, val;

#ifdef CONFIG_SYS_MEM_BLOCKS_LISTENER
	listener_idx = 0;
	heap_listener_register(&mem_block_01_alloc);
	heap_listener_register(&mem_block_01_free);
#endif

	/* get a 2 entiries memory block starting from 0 */
	ret = sys_mem_blocks_get(&mem_block_01, mem_block_01.buffer, 2);
	zassert_equal(ret, 0,
		      "sys_mem_blocks_get failed (%d)", ret);

	/* blocks 0 and 1 should be taken */
	for (i = 0; i < NUM_BLOCKS; i++) {
		ret = sys_bitarray_test_bit(mem_block_01.bitmap,
					    i, &val);
		zassert_equal(ret, 0, "API failure");
		switch (i) {
		case 0:
		case 1:
			zassert_equal(val, 1,
			      "sys_mem_blocks_get bitmap failed, bit %i should be set", i);
			break;
		default:
			zassert_equal(val, 0,
			     "sys_mem_blocks_get bitmap failed, bit %i should be cleared", i);
			break;

		}
	}

	/* get a 2 entiries memory block starting from 1 - should fail  */
	ret = sys_mem_blocks_get(&mem_block_01, mem_block_01.buffer + BLK_SZ, 2);
	zassert_equal(ret, -ENOMEM,
		      "sys_mem_blocks_get failed (%d), memory block taken twice", ret);

	/* blocks 0 and 1 should be taken */
	for (i = 0; i < NUM_BLOCKS; i++) {
		ret = sys_bitarray_test_bit(mem_block_01.bitmap,
					    i, &val);
		zassert_equal(ret, 0, "API failure");
		switch (i) {
		case 0:
		case 1:
			zassert_equal(val, 1,
			     "sys_mem_blocks_get bitmap failed, bit %i should be set", i);
			break;
		default:
			zassert_equal(val, 0,
			     "sys_mem_blocks_get bitmap failed, bit %i should be cleared", i);
			break;

		}
	}

	/* get a 2 slots block starting from the last one - should fail */
	ret = sys_mem_blocks_get(&mem_block_01, mem_block_01.buffer + (BLK_SZ * (NUM_BLOCKS-1)), 2);
	zassert_equal(ret, -ENOMEM,
		      "sys_mem_blocks_get failed - out of bounds (%d)", ret);

	/* blocks 0 and 1 should be taken */
	for (i = 0; i < NUM_BLOCKS; i++) {
		ret = sys_bitarray_test_bit(mem_block_01.bitmap,
					    i, &val);
		zassert_equal(ret, 0, "API failure");
		switch (i) {
		case 0:
		case 1:
			zassert_equal(val, 1,
			     "sys_mem_blocks_get bitmap failed, bit %i should be set", i);
			break;
		default:
			zassert_equal(val, 0,
			     "sys_mem_blocks_get bitmap failed, bit %i should be cleared", i);
			break;

		}
	}

	/* get a 1 slots block starting from 3 */
	ret = sys_mem_blocks_get(&mem_block_01, mem_block_01.buffer + (BLK_SZ * 3), 1);
	zassert_equal(ret, 0,
		      "sys_mem_blocks_get failed (%d)", ret);

	/* blocks 0,1,3 should be taken */
	for (i = 0; i < NUM_BLOCKS; i++) {
		ret = sys_bitarray_test_bit(mem_block_01.bitmap,
					    i, &val);
		zassert_equal(ret, 0, "API failure");
		switch (i) {
		case 0:
		case 1:
		case 3:
			zassert_equal(val, 1,
			     "sys_mem_blocks_get bitmap failed, bit %i should be set", i);
			break;
		default:
			zassert_equal(val, 0,
			     "sys_mem_blocks_get bitmap failed, bit %i should be cleared", i);
			break;
		}
	}

	/* get a 1 slots block starting from 2 */
	ret = sys_mem_blocks_get(&mem_block_01, mem_block_01.buffer + (BLK_SZ * 2), 1);
	zassert_equal(ret, 0,
		      "sys_mem_blocks_get failed (%d)", ret);

	/* blocks 0,1,2, 3 should be taken */
	for (i = 0; i < NUM_BLOCKS; i++) {
		ret = sys_bitarray_test_bit(mem_block_01.bitmap,
					    i, &val);
		zassert_equal(ret, 0, "API failure");
		switch (i) {
		case 0:
		case 1:
		case 2:
		case 3:
			zassert_equal(val, 1,
			     "sys_mem_blocks_get bitmap failed, bit %i should be set", i);
			break;
		default:
			zassert_equal(val, 0,
			     "sys_mem_blocks_get bitmap failed, bit %i should be cleared", i);
			break;
		}
	}

	/* cleanup - free all blocks */
	ret = sys_mem_blocks_free_contiguous(&mem_block_01, mem_block_01.buffer, 4);
	zassert_equal(ret, 0,
		      "sys_mem_blocks_get failed (%d)", ret);

	/* all blocks should be cleared */
	for (i = 0; i < NUM_BLOCKS; i++) {
		ret = sys_bitarray_test_bit(mem_block_01.bitmap,
					    i, &val);
		zassert_equal(ret, 0, "API failure");
		zassert_equal(val, 0,
		     "sys_mem_blocks_get bitmap failed, bit %i should be cleared", i);
	}

#ifdef CONFIG_SYS_MEM_BLOCKS_LISTENER
	heap_listener_unregister(&mem_block_01_alloc);
	heap_listener_unregister(&mem_block_01_free);

	/* verify alloc/free log */
	zassert_equal(listener_mem[0], mem_block_01.buffer,
			"sys_mem_blocks_get bitmap failed, %p != %p",
			listener_mem[0], mem_block_01.buffer);
	zassert_equal(listener_size[0], BLK_SZ*2,
			"sys_mem_blocks_get bitmap failed, %u != %u",
			listener_size[0], BLK_SZ*2);

	zassert_equal(listener_mem[1], mem_block_01.buffer + BLK_SZ*3,
			"sys_mem_blocks_get bitmap failed, %p != %p",
			listener_mem[1], mem_block_01.buffer + BLK_SZ*2);
	zassert_equal(listener_size[1], BLK_SZ,
			"sys_mem_blocks_get bitmap failed, %u != %u",
			listener_size[1], BLK_SZ);

	zassert_equal(listener_mem[2], mem_block_01.buffer + BLK_SZ*2,
			"sys_mem_blocks_get bitmap failed, %p != %p",
			listener_mem[2], mem_block_01.buffer + BLK_SZ);
	zassert_equal(listener_size[2], BLK_SZ,
			"sys_mem_blocks_get bitmap failed, %u != %u",
			listener_size[2], BLK_SZ);

	zassert_equal(listener_mem[3], mem_block_01.buffer,
			"sys_mem_blocks_get bitmap failed, %p != %p",
			listener_mem[3], mem_block_01.buffer);
	zassert_equal(listener_size[3], BLK_SZ*4,
			"sys_mem_blocks_get bitmap failed, %u != %u",
			listener_size[3], BLK_SZ*4);

#endif
}

ZTEST(lib_mem_block, test_mem_block_alloc_free_contiguous)
{
	int i, ret, val;
	void *block;

#ifdef CONFIG_SYS_MEM_BLOCKS_LISTENER
	listener_idx = 0;
	heap_listener_register(&mem_block_01_alloc);
	heap_listener_register(&mem_block_01_free);
#endif

	/* allocate all available blocks */
	ret = sys_mem_blocks_alloc_contiguous(&mem_block_01, NUM_BLOCKS, &block);
	zassert_equal(ret, 0,
		      "sys_mem_blocks_alloc_contiguous failed (%d)", ret);

	/* all blocks should be taken */
	for (i = 0; i < NUM_BLOCKS; i++) {
		ret = sys_bitarray_test_bit(mem_block_01.bitmap,
					    i, &val);
		zassert_equal(val, 1,
		     "sys_mem_blocks_alloc_contiguous failed, bit %i should be set", i);
		break;

	}

	/* free first 3 memory blocks, use a pointer provided by previous case */
	ret = sys_mem_blocks_free_contiguous(&mem_block_01, block, 3);
	zassert_equal(ret, 0,
		      "sys_mem_blocks_free_contiguous failed (%d)", ret);

	/* all blocks extept 0,1,2 should be taken */
	for (i = 0; i < NUM_BLOCKS; i++) {
		ret = sys_bitarray_test_bit(mem_block_01.bitmap,
					    i, &val);
		switch (i) {
		case 0:
		case 1:
		case 2:
			zassert_equal(val, 0,
			     "sys_mem_blocks_alloc_contiguous failed, bit %i should be cleared", i);
			break;
		default:
			zassert_equal(val, 1,
			     "sys_mem_blocks_alloc_contiguous failed, bit %i should be set", i);
			break;
		}
	}

	/* free a memory block, starting from 4, size 4 */
	ret = sys_mem_blocks_free_contiguous(&mem_block_01, mem_block_01.buffer+BLK_SZ*4, 4);
	zassert_equal(ret, 0,
		      "sys_mem_blocks_free_contiguous failed (%d)", ret);

	/* all blocks extept 0,1,2,4,5,6,7 should be taken */
	for (i = 0; i < NUM_BLOCKS; i++) {
		ret = sys_bitarray_test_bit(mem_block_01.bitmap,
					    i, &val);
		switch (i) {
		case 0:
		case 1:
		case 2:
		case 4:
		case 5:
		case 6:
		case 7:
			zassert_equal(val, 0,
			     "sys_mem_blocks_alloc_contiguous failed, bit %i should be cleared", i);
			break;
		default:
			zassert_equal(val, 1,
			     "sys_mem_blocks_alloc_contiguous failed, bit %i should be set", i);
			break;
		}
	}

	/* at this point, regardless of the memory size, there are 2 free continuous blocks
	 * sizes: 3 and 4 slots.
	 * try to allocate 5 blocks, should fail
	 */
	ret = sys_mem_blocks_alloc_contiguous(&mem_block_01, 5, &block);
	zassert_equal(ret, -ENOMEM,
		      "sys_mem_blocks_free_contiguous failed (%d)", ret);


	/* allocate 3 blocks */
	ret = sys_mem_blocks_alloc_contiguous(&mem_block_01, 3, &block);
	zassert_equal(ret, 0,
		      "sys_mem_blocks_free_contiguous failed (%d)", ret);
	/* all blocks extept 4,5,6,7 should be taken */
	for (i = 0; i < NUM_BLOCKS; i++) {
		ret = sys_bitarray_test_bit(mem_block_01.bitmap,
					    i, &val);
		switch (i) {
		case 4:
		case 5:
		case 6:
		case 7:
			zassert_equal(val, 0,
			     "sys_mem_blocks_alloc_contiguous failed, bit %i should be cleared", i);
			break;
		default:
			zassert_equal(val, 1,
			     "sys_mem_blocks_alloc_contiguous failed, bit %i should be set", i);
			break;
		}
	}

	/* allocate 4 blocks */
	ret = sys_mem_blocks_alloc_contiguous(&mem_block_01, 4, &block);
	zassert_equal(ret, 0,
		      "sys_mem_blocks_free_contiguous failed (%d)", ret);
	/* all blocks  should be taken */
	for (i = 0; i < NUM_BLOCKS; i++) {
		ret = sys_bitarray_test_bit(mem_block_01.bitmap,
					    i, &val);
		zassert_equal(val, 1,
		     "sys_mem_blocks_alloc_contiguous failed, bit %i should be set", i);
	}

	/* cleanup - free all blocks */
	ret = sys_mem_blocks_free_contiguous(&mem_block_01, mem_block_01.buffer, NUM_BLOCKS);
	zassert_equal(ret, 0,
		      "sys_mem_blocks_alloc_contiguous failed (%d)", ret);

	/* all blocks should be cleared */
	for (i = 0; i < NUM_BLOCKS; i++) {
		ret = sys_bitarray_test_bit(mem_block_01.bitmap,
					    i, &val);
		zassert_equal(val, 0,
		     "sys_mem_blocks_alloc_contiguous failed, bit %i should be cleared", i);
	}

#ifdef CONFIG_SYS_MEM_BLOCKS_LISTENER
	heap_listener_unregister(&mem_block_01_alloc);
	heap_listener_unregister(&mem_block_01_free);

	/* verify alloc/free log */
	zassert_equal(listener_mem[0], mem_block_01.buffer,
			"sys_mem_blocks_alloc_contiguous failed, %p != %p",
			listener_mem[0], mem_block_01.buffer);
	zassert_equal(listener_size[0], BLK_SZ*NUM_BLOCKS,
			"sys_mem_blocks_alloc_contiguous failed, %u != %u",
			listener_size[0], BLK_SZ*NUM_BLOCKS);

	zassert_equal(listener_mem[1], mem_block_01.buffer,
			"sys_mem_blocks_alloc_contiguous failed, %p != %p",
			listener_mem[1], mem_block_01.buffer);
	zassert_equal(listener_size[1], BLK_SZ*3,
			"sys_mem_blocks_alloc_contiguous failed, %u != %u",
			listener_size[1], BLK_SZ*3);

	zassert_equal(listener_mem[2], mem_block_01.buffer+BLK_SZ*4,
			"sys_mem_blocks_alloc_contiguous failed, %p != %p",
			listener_mem[2], mem_block_01.buffer+BLK_SZ*4);
	zassert_equal(listener_size[2], BLK_SZ*4,
			"sys_mem_blocks_alloc_contiguous failed, %u != %u",
			listener_size[2], BLK_SZ*4);

	zassert_equal(listener_mem[3], mem_block_01.buffer,
			"sys_mem_blocks_alloc_contiguous failed, %p != %p",
			listener_mem[3], mem_block_01.buffer);
	zassert_equal(listener_size[3], BLK_SZ*3,
			"sys_mem_blocks_alloc_contiguous failed, %u != %u",
			listener_size[3], BLK_SZ*3);

	zassert_equal(listener_mem[4], mem_block_01.buffer+BLK_SZ*4,
			"sys_mem_blocks_alloc_contiguous failed, %p != %p",
			listener_mem[4], mem_block_01.buffer+BLK_SZ*4);
	zassert_equal(listener_size[4], BLK_SZ*4,
			"sys_mem_blocks_alloc_contiguous failed, %u != %u",
			listener_size[4], BLK_SZ*4);

	zassert_equal(listener_mem[5], mem_block_01.buffer,
			"sys_mem_blocks_alloc_contiguous failed, %p != %p",
			listener_mem[5], mem_block_01.buffer);
	zassert_equal(listener_size[5], BLK_SZ*NUM_BLOCKS,
			"sys_mem_blocks_alloc_contiguous failed, %u != %u",
			listener_size[5], BLK_SZ*NUM_BLOCKS);
#endif
}

ZTEST(lib_mem_block, test_multi_mem_block_alloc_free)
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

ZTEST(lib_mem_block, test_mem_block_invalid_params_panic_1)
{
	void *blocks[2] = {0};

	expected_reason = K_ERR_KERNEL_PANIC;
	sys_mem_blocks_alloc(NULL, 1, blocks);

	/* test should raise an exception and should not reach this line */
	ztest_test_fail();
}

ZTEST(lib_mem_block, test_mem_block_invalid_params_panic_2)
{
	expected_reason = K_ERR_KERNEL_PANIC;
	sys_mem_blocks_alloc(&mem_block_01, 1, NULL);

	/* test should raise an exception and should not reach this line */
	ztest_test_fail();
}

ZTEST(lib_mem_block, test_mem_block_invalid_params_panic_3)
{
	void *blocks[2] = {0};

	expected_reason = K_ERR_KERNEL_PANIC;
	sys_mem_blocks_free(NULL, 1, blocks);

	/* test should raise an exception and should not reach this line */
	ztest_test_fail();
}

ZTEST(lib_mem_block, test_mem_block_invalid_params_panic_4)
{
	expected_reason = K_ERR_KERNEL_PANIC;
	sys_mem_blocks_free(&mem_block_01, 1, NULL);

	/* test should raise an exception and should not reach this line */
	ztest_test_fail();
}

ZTEST(lib_mem_block, test_mem_block_invalid_params)
{
	int ret;
	void *blocks[2] = {0};

	ret = sys_mem_blocks_alloc(&mem_block_01, 0, blocks);
	zassert_equal(ret, 0,
		      "sys_mem_blocks_alloc failed (%d)", ret);

	ret = sys_mem_blocks_alloc(&mem_block_01, NUM_BLOCKS + 1, blocks);
	zassert_equal(ret, -ENOMEM,
		      "sys_mem_blocks_alloc should fail with -ENOMEM but not");

	ret = sys_mem_blocks_alloc(&mem_block_01, 1, blocks);
	zassert_equal(ret, 0,
		      "sys_mem_blocks_alloc failed (%d)", ret);

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

ZTEST(lib_mem_block, test_multi_mem_block_invalid_params_panic_1)
{
	void *blocks[2] = {0};

	expected_reason = K_ERR_KERNEL_PANIC;
	sys_multi_mem_blocks_alloc(NULL, UINT_TO_POINTER(16),
						 1, blocks, NULL);

	/* test should raise an exception and should not reach this line */
	ztest_test_fail();
}


ZTEST(lib_mem_block, test_multi_mem_block_invalid_params_panic_2)
{
	expected_reason = K_ERR_KERNEL_PANIC;

	sys_multi_mem_blocks_alloc(&alloc_group, UINT_TO_POINTER(16),
						 1, NULL, NULL);

	/* test should raise an exception and should not reach this line */
	ztest_test_fail();
}

ZTEST(lib_mem_block, test_multi_mem_block_invalid_params_panic_3)
{
	void *blocks[2] = {0};

	expected_reason = K_ERR_KERNEL_PANIC;
	sys_multi_mem_blocks_free(NULL, 1, blocks);

	/* test should raise an exception and should not reach this line */
	ztest_test_fail();
}


ZTEST(lib_mem_block, test_multi_mem_block_invalid_params_panic_4)
{
	expected_reason = K_ERR_KERNEL_PANIC;

	sys_multi_mem_blocks_free(&alloc_group, 1, NULL);

	/* test should raise an exception and should not reach this line */
	ztest_test_fail();
}

ZTEST(lib_mem_block, test_multi_mem_block_invalid_params)
{
	int ret;
	void *blocks[2] = {0};

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

static void *lib_mem_block_setup(void)
{
	sys_multi_mem_blocks_init(&alloc_group, choice_fn);
	sys_multi_mem_blocks_add_allocator(&alloc_group, &mem_block_01);
	sys_multi_mem_blocks_add_allocator(&alloc_group, &mem_block_02);
	return NULL;
}

ZTEST_SUITE(lib_mem_block, NULL, lib_mem_block_setup, NULL, NULL, NULL);
