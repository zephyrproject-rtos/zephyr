/*
 * Copyright (c) 2023 Google LLC
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/rtio/rtio.h>
#include <zephyr/sys/util.h>
#include <zephyr/app_memory/app_memdomain.h>

#ifdef CONFIG_USERSPACE
K_APPMEM_PARTITION_DEFINE(rtio_partition);
#endif

int rtio_init(void)
{
	STRUCT_SECTION_FOREACH(rtio_sqe_pool, sqe_pool) {
		for (int i = 0; i < sqe_pool->pool_size; i++) {
			rtio_mpsc_push(&sqe_pool->free_q, &sqe_pool->pool[i].q);
		}
	}

	STRUCT_SECTION_FOREACH(rtio_cqe_pool, cqe_pool) {
		for (int i = 0; i < cqe_pool->pool_size; i++) {
			rtio_mpsc_push(&cqe_pool->free_q, &cqe_pool->pool[i].q);
		}
	}

	return 0;
}

SYS_INIT(rtio_init, POST_KERNEL, 0);
