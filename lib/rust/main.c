/*
 * Copyright (c) 2024 Linaro LTD
 * SPDX-License-Identifier: Apache-2.0
 */

/* This main is brought into the Rust application. */
#include <zephyr/kernel.h>

#ifdef CONFIG_RUST

#if defined(CONFIG_USERSPACE) && defined(CONFIG_RUST_ALLOC)
#include <zephyr/app_memory/app_memdomain.h>
#include <zephyr/sys/libc-hooks.h>
#endif

extern void rust_main(void);

int main(void)
{
#if defined(CONFIG_USERSPACE) && defined(CONFIG_RUST_ALLOC)
#ifdef Z_MALLOC_PARTITION_EXISTS
	k_mem_domain_add_partition(&k_mem_domain_default, &z_malloc_partition);
#endif
#endif
	rust_main();
	return 0;
}

#endif
