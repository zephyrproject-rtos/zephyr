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
	STRUCT_SECTION_FOREACH(rtio, r) {
		for (int i = 0; i < r->sqe_pool_sz; i++) {
			rtio_mpsc_push(&r->sq_free, &r->sqe_pool[i].q);
		}
		for (int i = 0; i < r->cqe_pool_sz; i++) {
			rtio_mpsc_push(&r->cq_free, &r->cqe_pool[i].q);
		}
		r->sqe_pool_free = r->sqe_pool_sz;
		r->cqe_pool_used = 0;
	}
	return 0;
}

SYS_INIT(rtio_init, POST_KERNEL, 0);
