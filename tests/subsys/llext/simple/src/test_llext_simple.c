/*
 * Copyright (c) 2023 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/kernel.h>
#include <zephyr/llext/llext.h>
#include <zephyr/llext/buf_loader.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/libc-hooks.h>

LOG_MODULE_REGISTER(test_llext_simple);


#ifdef CONFIG_LLEXT_STORAGE_WRITABLE
#define LLEXT_CONST
#else
#define LLEXT_CONST const
#endif

struct llext_test {
	const char *name;
	bool try_userspace;
	size_t buf_len;

	LLEXT_CONST uint8_t *buf;
};



K_THREAD_STACK_DEFINE(llext_stack, 1024);
struct k_thread llext_thread;

#ifdef CONFIG_USERSPACE
void llext_entry(void *arg0, void *arg1, void *arg2)
{
	void (*fn)(void) = arg0;

	LOG_INF("calling fn %p from thread %p", fn, k_current_get());
	fn();
}
#endif /* CONFIG_USERSPACE */

void load_call_unload(struct llext_test *test_case)
{
	struct llext_buf_loader buf_loader =
		LLEXT_BUF_LOADER(test_case->buf, test_case->buf_len);
	struct llext_loader *loader = &buf_loader.loader;
	struct llext_load_param ldr_parm = LLEXT_LOAD_PARAM_DEFAULT;
	struct llext *ext = NULL;

	int res = llext_load(loader, test_case->name, &ext, &ldr_parm);

	zassert_ok(res, "load should succeed");

	void (*test_entry_fn)() = llext_find_sym(&ext->exp_tab, "test_entry");

	zassert_not_null(test_entry_fn, "test_entry should be an exported symbol");

#ifdef CONFIG_USERSPACE
	/*
	 * Due to the number of MPU regions on some parts with MPU (USERSPACE)
	 * enabled we need to always call into the extension from a new dedicated
	 * thread to avoid running out of MPU regions on some parts.
	 *
	 * This is part dependent behavior and certainly on MMU capable parts
	 * this should not be needed! This test however is here to be generic
	 * across as many parts as possible.
	 */
	struct k_mem_domain domain;

	k_mem_domain_init(&domain, 0, NULL);

#ifdef Z_LIBC_PARTITION_EXISTS
	k_mem_domain_add_partition(&domain, &z_libc_partition);
#endif

	res = llext_add_domain(ext, &domain);
	if (res == -ENOSPC) {
		TC_PRINT("Too many memory partitions for this particular hardware\n");
		ztest_test_skip();
		return;
	}
	zassert_ok(res, "adding partitions to domain should succeed");

	/* Should be runnable from newly created thread */
	k_thread_create(&llext_thread, llext_stack,
			K_THREAD_STACK_SIZEOF(llext_stack),
			&llext_entry, test_entry_fn, NULL, NULL,
			1, 0, K_FOREVER);

	k_mem_domain_add_thread(&domain, &llext_thread);

	k_thread_start(&llext_thread);
	k_thread_join(&llext_thread, K_FOREVER);

	/* Some extensions may wish to be tried from the context
	 * of a userspace thread along with the usual supervisor context
	 * tried above.
	 */
	if (test_case->try_userspace) {
		k_thread_create(&llext_thread, llext_stack,
				K_THREAD_STACK_SIZEOF(llext_stack),
				&llext_entry, test_entry_fn, NULL, NULL,
				1, K_USER, K_FOREVER);

		k_mem_domain_add_thread(&domain, &llext_thread);

		k_thread_start(&llext_thread);
		k_thread_join(&llext_thread, K_FOREVER);
	}


#else /* CONFIG_USERSPACE */
	zassert_ok(llext_call_fn(ext, "test_entry"),
		   "test_entry call should succeed");
#endif /* CONFIG_USERSPACE */

	llext_unload(&ext);
}

#ifndef LOADER_BUILD_ONLY
/*
 * Attempt to load, list, list symbols, call a fn, and unload each
 * extension in the test table. This excercises loading, calling into, and
 * unloading each extension which may itself excercise various APIs provided by
 * Zephyr.
 */
#define LLEXT_LOAD_UNLOAD(_name, _userspace)					\
	ZTEST(llext, test_load_unload_##_name)					\
	{									\
		struct llext_test test_case = {					\
			.name = STRINGIFY(_name),				\
			.try_userspace = _userspace,				\
			.buf_len = ARRAY_SIZE(_name ## _ext),			\
			.buf = _name ## _ext,					\
		};								\
		load_call_unload(&test_case);					\
	}
static LLEXT_CONST uint8_t hello_world_ext[] __aligned(4) = {
	#include "hello_world.inc"
};
LLEXT_LOAD_UNLOAD(hello_world, false)

static LLEXT_CONST uint8_t logging_ext[] __aligned(4) = {
	#include "logging.inc"
};
LLEXT_LOAD_UNLOAD(logging, true)

static LLEXT_CONST uint8_t relative_jump_ext[] __aligned(4) = {
	#include "relative_jump.inc"
};
LLEXT_LOAD_UNLOAD(relative_jump, true)

static LLEXT_CONST uint8_t object_ext[] __aligned(4) = {
	#include "object.inc"
};
LLEXT_LOAD_UNLOAD(object, true)
#endif /* ! LOADER_BUILD_ONLY */

/*
 * Ensure that EXPORT_SYMBOL does indeed provide a symbol and a valid address
 * to it.
 */
ZTEST(llext, test_printk_exported)
{
	const void * const printk_fn = llext_find_sym(NULL, "printk");

	zassert_equal(printk_fn, printk, "printk should be an exported symbol");
}


ZTEST_SUITE(llext, NULL, NULL, NULL, NULL, NULL);
