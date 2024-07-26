/*
 * Copyright (c) 2024 Linaro LTD
 * SPDX-License-Identifier: Apache-2.0
 */

/* This main is brought into the Rust application. */
#include <zephyr/kernel.h>

#ifdef CONFIG_RUST

extern void rust_main(void);

#ifdef CONFIG_USERSPACE

#include <zephyr/app_memory/app_memdomain.h>

K_APPMEM_PARTITION_DEFINE(rust_mem_part);

#ifdef CONFIG_RUST_ALLOC
K_APP_BMEM(rust_mem_part) uint8_t rust_heap_buf[CONFIG_RUST_ALLOC_HEAP_SIZE];
K_APP_BMEM(rust_mem_part) struct k_heap rust_heap;
#endif

struct k_mem_domain rust_mem_domain;

#else

#ifdef CONFIG_RUST_ALLOC
K_HEAP_DEFINE(rust_heap, CONFIG_RUST_ALLOC_HEAP_SIZE);
#endif

#endif

int main(void)
{
	k_tid_t main_thread = k_current_get();

#ifdef CONFIG_USERSPACE
	k_mem_domain_init(&rust_mem_domain, 0, NULL);

	for (uint8_t i = 0; i < k_mem_domain_default.num_partitions; i++) {
		k_mem_domain_add_partition(&rust_mem_domain, &k_mem_domain_default.partitions[i]);
	}

	k_mem_domain_add_partition(&rust_mem_domain, &rust_mem_part);
	k_mem_domain_add_thread(&rust_mem_domain, main_thread);

#ifdef CONFIG_RUST_ALLOC
	k_heap_init(&rust_heap, rust_heap_buf, sizeof(rust_heap_buf));
#endif
#endif

#ifdef CONFIG_RUST_ALLOC
	k_thread_heap_assign(main_thread, &rust_heap);
#endif

	rust_main();
	return 0;
}

#endif
