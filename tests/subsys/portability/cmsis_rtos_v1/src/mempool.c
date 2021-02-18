/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ztest.h>
#include <kernel.h>
#include <cmsis_os.h>

struct mem_block {
	int member1;
	int member2;
};

#define MAX_BLOCKS 10

osPoolDef(MemPool, MAX_BLOCKS, struct mem_block);

/**
 * @brief Test memory pool allocation and free
 *
 * @see osPoolCreate(), osPoolAlloc(), osPoolFree(),
 * osPoolCAlloc(), memcmp()
 */
void test_mempool(void)
{
	int i;
	osPoolId mempool_id;
	struct mem_block *addr_list[MAX_BLOCKS + 1];
	osStatus status_list[MAX_BLOCKS];
	static struct mem_block zeroblock;

	mempool_id = osPoolCreate(osPool(MemPool));
	zassert_true(mempool_id != NULL, "mempool creation failed");

	for (i = 0; i < MAX_BLOCKS; i++) {
		addr_list[i] = (struct mem_block *)osPoolAlloc(mempool_id);
		zassert_true(addr_list[i] != NULL, "mempool allocation failed");
	}

	/* All blocks in mempool are allocated, any more allocation
	 * without free should fail
	 */
	addr_list[i] = (struct mem_block *)osPoolAlloc(mempool_id);
	zassert_true(addr_list[i] == NULL, "allocation happened."
			" Something's wrong!");

	for (i = 0; i < MAX_BLOCKS; i++) {
		status_list[i] = osPoolFree(mempool_id, addr_list[i]);
		zassert_true(status_list[i] == osOK, "mempool free failed");
	}

	for (i = 0; i < MAX_BLOCKS; i++) {
		addr_list[i] = (struct mem_block *)osPoolCAlloc(mempool_id);
		zassert_true(addr_list[i] != NULL, "mempool allocation failed");
		zassert_true(memcmp(addr_list[i], &zeroblock,
					sizeof(struct mem_block)) == 0,
			     "osPoolCAlloc didn't set mempool to 0");
	}

	for (i = 0; i < MAX_BLOCKS; i++) {
		status_list[i] = osPoolFree(mempool_id, addr_list[i]);
		zassert_true(status_list[i] == osOK, "mempool free failed");
	}
}
