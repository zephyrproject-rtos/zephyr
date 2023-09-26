/*
 * Copyright (c) 2023 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/kernel.h>
#include <zephyr/modules/module.h>
#include <zephyr/modules/buf_stream.h>

#if defined(CONFIG_ARM) /* ARMV7 */
const static uint8_t hello_world_elf[] = {
#include "hello_world_armv7_thumb.elf.inc"
};
#elif defined(CONFIG_XTENSA)
const static uint8_t hello_world_elf[] __attribute__((aligned(4))) = {
#include "hello_world_xtensa.elf.inc"
};
#endif

/**
 * Attempt to load, list, list symbols, call a fn, and unload a
 * hello world module for each supported architecture
 *
 * This requires a single linked symbol (printk) and a single
 * exported symbol from the module ( void hello_world(void))
 */
ZTEST(modules, test_modules_simple)
{
	const char name[16] = "hello";
	struct module_buf_stream buf_stream =
		MODULE_BUF_STREAM(hello_world_elf, ARRAY_SIZE(hello_world_elf));
	struct module_stream *stream = &buf_stream.stream;
	struct module *m;

	int res = module_load(stream, name, &m, true);

	zassert_ok(res, "Load should succeed");

	void *hello_world_fn = module_find_sym(&m->sym_tab, "hello_world");

	zassert_not_null(hello_world_fn, "hello_world should be an exported module symbol");

	res = module_call_fn(m, "hello_world");

	zassert_ok(res, "Calling hello world should succeed");

	module_unload(m);
}

ZTEST_SUITE(modules, NULL, NULL, NULL, NULL, NULL);
