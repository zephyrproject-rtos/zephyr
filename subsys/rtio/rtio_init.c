/*
 * Copyright (c) 2023 Google LLC
 * Copyright (c) 2026 Antmicro <antmicro.com>
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/init.h>
#include <zephyr/rtio/rtio.h>
#include <zephyr/sys/mpsc_lockfree.h>
#include <zephyr/sys/util.h>
#include <zephyr/app_memory/app_memdomain.h>
#include <zephyr/sys/iterable_sections.h>
#include <zephyr/linker/linker-defs.h>

/* We use `K_MEM_PARTITION_DEFINE_UNCHECKED` instead of `K_MEM_PARTITION_DEFINE  because the latter
 * does `static_assert`s on the address and size, which would fail because here they are only known
 * at link-time, and we know that the alignment is okay; nocache is explicitly aligned in the linker
 * script, and DTCM on all platforms is known to be MPU-aligned.
 */

#ifdef CONFIG_USERSPACE
#ifdef CONFIG_RTIO_BLOCK_POOL_PLACEMENT_DTCM
K_MEM_PARTITION_DEFINE_UNCHECKED(rtio_partition, __dtcm_start, (size_t)__dtcm_size,
				 K_MEM_PARTITION_P_RW_U_RW_NOCACHE);
#elif defined(CONFIG_RTIO_BLOCK_POOL_PLACEMENT_NOCACHE)
K_MEM_PARTITION_DEFINE_UNCHECKED(rtio_partition, _nocache_ram_start, (size_t)_nocache_ram_size,
				 K_MEM_PARTITION_P_RW_U_RW_NOCACHE);
#else
K_APPMEM_PARTITION_DEFINE(rtio_partition);
#endif
#endif

int rtio_init(void)
{
	STRUCT_SECTION_FOREACH(rtio_sqe_pool, sqe_pool) {
		for (int i = 0; i < sqe_pool->pool_size; i++) {
			mpsc_push(&sqe_pool->free_q, &sqe_pool->pool[i].q);
		}
	}

	STRUCT_SECTION_FOREACH(rtio_cqe_pool, cqe_pool) {
		for (int i = 0; i < cqe_pool->pool_size; i++) {
			mpsc_push(&cqe_pool->free_q, &cqe_pool->pool[i].q);
		}
	}

	return 0;
}

SYS_INIT(rtio_init, POST_KERNEL, 0);
