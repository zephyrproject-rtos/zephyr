/*
 * Copyright (c) 2023 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/kernel.h>
#include <zephyr/llext/llext.h>
#include <zephyr/llext/buf_loader.h>

#ifdef CONFIG_ARM /* ARMV7 */
const static uint8_t hello_world_elf[] __aligned(4) = {
#include "hello_world.inc"
};
#endif

/**
 * Attempt to load, list, list symbols, call a fn, and unload a
 * hello world extension for each supported architecture
 *
 * This requires a single linked symbol (printk) and a single
 * exported symbol from the extension ( void hello_world(void))
 */
ZTEST(llext, test_llext_simple)
{
	const char name[16] = {'h', 'e', 'l', 'l', 'o', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
	struct llext_buf_loader buf_loader =
		LLEXT_BUF_LOADER(hello_world_elf, ARRAY_SIZE(hello_world_elf));
	struct llext_loader *loader = &buf_loader.loader;
	struct llext *ext;
	const void * const printk_fn = llext_find_sym(NULL, "printk");

	zassert_equal(printk_fn, printk, "printk should be an exported symbol");

	int res = llext_load(loader, name, &ext);

	zassert_ok(res, "load should succeed");

	const void * const hello_world_fn = llext_find_sym(&ext->sym_tab, "hello_world");

	zassert_not_null(hello_world_fn, "hello_world should be an exported symbol");

	res = llext_call_fn(ext, "hello_world");

	zassert_ok(res, "calling hello world should succeed");

	llext_unload(ext);
}

ZTEST_SUITE(llext, NULL, NULL, NULL, NULL, NULL);
