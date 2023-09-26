/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/kernel.h>
#include <cmsis_os2.h>

#define MAX_BLOCKS      10
#define TIMEOUT_TICKS   10

struct mem_block {
	int member1;
	int member2;
};

static char __aligned(sizeof(void *)) sample_mem[sizeof(struct mem_block) * MAX_BLOCKS];
static const osMemoryPoolAttr_t mp_attrs = {
	.name = "TestMempool",
	.attr_bits = 0,
	.cb_mem = NULL,
	.cb_size = 0,
	.mp_mem = sample_mem,
	.mp_size = sizeof(struct mem_block) * MAX_BLOCKS,
};

static void mempool_common_tests(osMemoryPoolId_t mp_id,
				 const char *expected_name)
{
	int i;
	osMemoryPoolId_t dummy_id = NULL;
	const char *name;
	struct mem_block *addr_list[MAX_BLOCKS + 1];
	osStatus_t status;

	zassert_true(osMemoryPoolGetName(dummy_id) == NULL,
		     "Something's wrong with osMemoryPoolGetName!");

	name = osMemoryPoolGetName(mp_id);
	zassert_true(strcmp(expected_name, name) == 0,
		     "Error getting mempool name");

	zassert_equal(osMemoryPoolGetCapacity(dummy_id), 0,
		      "Something's wrong with osMemoryPoolGetCapacity!");

	zassert_equal(osMemoryPoolGetCapacity(mp_id), MAX_BLOCKS,
		      "Something's wrong with osMemoryPoolGetCapacity!");

	zassert_equal(osMemoryPoolGetBlockSize(dummy_id), 0,
		      "Something's wrong with osMemoryPoolGetBlockSize!");

	zassert_equal(osMemoryPoolGetBlockSize(mp_id),
		      sizeof(struct mem_block),
		      "Something's wrong with osMemoryPoolGetBlockSize!");

	/* The memory pool should be completely available at this point */
	zassert_equal(osMemoryPoolGetCount(mp_id), 0,
		      "Something's wrong with osMemoryPoolGetCount!");
	zassert_equal(osMemoryPoolGetSpace(mp_id), MAX_BLOCKS,
		      "Something's wrong with osMemoryPoolGetSpace!");

	for (i = 0; i < MAX_BLOCKS; i++) {
		addr_list[i] = (struct mem_block *)osMemoryPoolAlloc(mp_id,
								     osWaitForever);
		zassert_true(addr_list[i] != NULL, "mempool allocation failed");
	}

	/* The memory pool should be completely in use at this point */
	zassert_equal(osMemoryPoolGetCount(mp_id), MAX_BLOCKS,
		      "Something's wrong with osMemoryPoolGetCount!");
	zassert_equal(osMemoryPoolGetSpace(mp_id), 0,
		      "Something's wrong with osMemoryPoolGetSpace!");

	/* All blocks in mempool are allocated, any more allocation
	 * without free should fail
	 */
	addr_list[i] = (struct mem_block *)osMemoryPoolAlloc(mp_id,
							     TIMEOUT_TICKS);
	zassert_true(addr_list[i] == NULL, "allocation happened."
		     " Something's wrong!");

	zassert_equal(osMemoryPoolFree(dummy_id, addr_list[0]),
		      osErrorParameter, "mempool free worked unexpectedly!");

	for (i = 0; i < MAX_BLOCKS; i++) {
		status = osMemoryPoolFree(mp_id, addr_list[i]);
		zassert_true(status == osOK, "mempool free failed");
	}

	zassert_equal(osMemoryPoolDelete(dummy_id), osErrorParameter,
		      "mempool delete worked unexpectedly!");

	status = osMemoryPoolDelete(mp_id);
	zassert_true(status == osOK, "mempool delete failure");
}

/**
 * @brief Test dynamic memory pool allocation and free
 *
 * @see osMemoryPoolNew(), osMemoryPoolAlloc(), osMemoryPoolFree(),
 */
ZTEST(cmsis_mempool, test_mempool_dynamic)
{
	osMemoryPoolId_t mp_id;

	mp_id = osMemoryPoolNew(MAX_BLOCKS, sizeof(struct mem_block),
				NULL);
	zassert_true(mp_id != NULL, "mempool creation failed");

	mempool_common_tests(mp_id, "ZephyrMemPool");
}

/**
 * @brief Test memory pool allocation and free
 *
 * @see osMemoryPoolNew(), osMemoryPoolAlloc(), osMemoryPoolFree(),
 */
ZTEST(cmsis_mempool, test_mempool)
{
	osMemoryPoolId_t mp_id;

	/* Create memory pool with invalid block size */
	mp_id = osMemoryPoolNew(MAX_BLOCKS + 1, sizeof(struct mem_block),
				&mp_attrs);
	zassert_true(mp_id == NULL, "osMemoryPoolNew worked unexpectedly!");

	mp_id = osMemoryPoolNew(MAX_BLOCKS, sizeof(struct mem_block),
				&mp_attrs);
	zassert_true(mp_id != NULL, "mempool creation failed");

	mempool_common_tests(mp_id, mp_attrs.name);
}
ZTEST_SUITE(cmsis_mempool, NULL, NULL, NULL, NULL, NULL);
