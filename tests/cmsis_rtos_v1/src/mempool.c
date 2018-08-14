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

osPoolDef(MemPool, 8, struct mem_block);

void test_mempool(void)
{
	osPoolId mempool_id;
	osStatus status;
	struct mem_block *addr;

	mempool_id = osPoolCreate(osPool(MemPool));
	zassert_true(mempool_id != NULL, "mempool creation failed\n");

	addr = (struct mem_block *)osPoolAlloc(mempool_id);
	zassert_true(addr != NULL, "mempool allocation failed\n");

	status = osPoolFree(mempool_id, addr);
	zassert_true(status == osOK, "mempool free failed\n");

	addr = (struct mem_block *)osPoolCAlloc(mempool_id);
	zassert_true(addr != NULL, "mempool allocation failed\n");

	status = osPoolFree(mempool_id, addr);
	zassert_true(status == osOK, "mempool free failed\n");
}
