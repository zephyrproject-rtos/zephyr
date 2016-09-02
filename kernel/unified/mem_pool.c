/*
 * Copyright (c) 2016 Wind River Systems, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/**
 * @brief Memory pools.
 */

#include <kernel.h>
#include <nano_private.h>
#include <misc/debug/object_tracing_common.h>
#include <toolchain.h>
#include <sections.h>
#include <wait_q.h>
#include <misc/dlist.h>

void k_mem_pool_init(struct k_mem_pool *mem, int max_block_size,
				int num_max_blocks)
{
}

int k_mem_pool_alloc(k_mem_pool_t id, struct k_block *block, int size,
			int32_t timeout)
{
	return 0;
}

void k_mem_pool_free(struct k_block *block)
{
}

void k_mem_pool_defrag(k_mem_pool_t id)
{
}

void *k_malloc(uint32_t size)
{
	return NULL;
}

void k_free(void *p)
{
}
