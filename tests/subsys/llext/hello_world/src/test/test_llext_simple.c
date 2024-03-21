/*
 * Copyright (c) 2023 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/kernel.h>
#include <zephyr/llext/llext.h>
#include <zephyr/llext/buf_loader.h>

#if defined(CONFIG_ARM) /* ARMV7 */ || defined(CONFIG_XTENSA)
#ifndef CONFIG_LLEXT_STORAGE_WRITABLE
const
#endif
static uint8_t hello_world_elf[] __aligned(4) = {
#include "hello_world.inc"
};
#endif

K_THREAD_STACK_DEFINE(llext_stack, 1024);
struct k_thread llext_thread;

#ifdef CONFIG_USERSPACE
void llext_entry(void *arg0, void *arg1, void *arg2)
{
	struct llext *ext = arg0;

	zassert_ok(llext_call_fn(ext, "hello_world"),
		   "hello_world call should succeed");
}
#endif /* CONFIG_USERSPACE */

/**
 * Attempt to load, list, list symbols, call a fn, and unload a
 * hello world extension for each supported architecture
 *
 * This requires a single linked symbol (printk) and a single
 * exported symbol from the extension ( void hello_world(void))
 */
ZTEST(llext, test_llext_simple)
{
	const char name[16] = "hello";
	struct llext_buf_loader buf_loader =
		LLEXT_BUF_LOADER(hello_world_elf, ARRAY_SIZE(hello_world_elf));
	struct llext_loader *loader = &buf_loader.loader;
	struct llext_load_param ldr_parm = LLEXT_LOAD_PARAM_DEFAULT;
	struct llext *ext = NULL;
	const void * const printk_fn = llext_find_sym(NULL, "printk");

	zassert_equal(printk_fn, printk, "printk should be an exported symbol");

	int res = llext_load(loader, name, &ext, &ldr_parm);

	zassert_ok(res, "load should succeed");

	const void * const hello_world_fn = llext_find_sym(&ext->exp_tab, "hello_world");

	zassert_not_null(hello_world_fn, "hello_world should be an exported symbol");

#ifdef CONFIG_USERSPACE
	struct k_mem_domain domain;

	k_mem_domain_init(&domain, 0, NULL);

	res = llext_add_domain(ext, &domain);
	zassert_ok(res, "adding partitions to domain should succeed");

	/* Should be runnable from newly created thread */
	k_thread_create(&llext_thread, llext_stack,
			K_THREAD_STACK_SIZEOF(llext_stack),
			&llext_entry, ext, NULL, NULL,
			1, 0, K_FOREVER);

	k_mem_domain_add_thread(&domain, &llext_thread);

	k_thread_start(&llext_thread);
	k_thread_join(&llext_thread, K_FOREVER);

#else /* CONFIG_USERSPACE */
	zassert_ok(llext_call_fn(ext, "hello_world"),
		   "hello_world call should succeed");
#endif /* CONFIG_USERSPACE */

	llext_unload(&ext);
}

ZTEST_SUITE(llext, NULL, NULL, NULL, NULL, NULL);
