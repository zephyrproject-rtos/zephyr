/*
 * Copyright (c) 2023 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/kernel.h>
#include <zephyr/llext/llext.h>
#include <zephyr/llext/symbol.h>
#include <zephyr/llext/buf_loader.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/libc-hooks.h>
#include "syscalls_ext.h"
#include "threads_kernel_objects_ext.h"


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

	void (*perm_setup)(struct k_thread *llext_thread);
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


/* syscalls test */

int z_impl_ext_syscall_ok(int a)
{
	return a + 1;
}

#ifdef CONFIG_USERSPACE
static inline int z_vrfy_ext_syscall_ok(int a)
{
	return z_impl_ext_syscall_ok(a);
}
#include <syscalls/ext_syscall_ok_mrsh.c>
#endif /* CONFIG_USERSPACE */


/* threads kernel objects test */

/* For these to be accessible from user space, they must be top-level globals
 * in the Zephyr image. Also, macros that add objects to special linker sections,
 * such as K_THREAD_STACK_DEFINE, do not work properly from extensions code.
 */
K_SEM_DEFINE(my_sem, 1, 1);
EXPORT_SYMBOL(my_sem);
struct k_thread my_thread;
EXPORT_SYMBOL(my_thread);
K_THREAD_STACK_DEFINE(my_thread_stack, MY_THREAD_STACK_SIZE);
EXPORT_SYMBOL(my_thread_stack);

#ifdef CONFIG_USERSPACE
/* Allow the user space test thread to access global objects */
static void threads_objects_perm_setup(struct k_thread *llext_thread)
{
	k_object_access_grant(&my_sem, llext_thread);
	k_object_access_grant(&my_thread, llext_thread);
	k_object_access_grant(&my_thread_stack, llext_thread);
}
#else
/* No need to set up permissions for supervisor mode */
#define threads_objects_perm_setup NULL
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

	/* Even in supervisor mode, initialize permissions on objects used in
	 * the test by this thread, so that user mode descendant threads can
	 * inherit these permissions.
	 */
	if (test_case->perm_setup) {
		test_case->perm_setup(&llext_thread);
	}

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

		if (test_case->perm_setup) {
			test_case->perm_setup(&llext_thread);
		}

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
#define LLEXT_LOAD_UNLOAD(_name, _userspace, _perm_setup)			\
	ZTEST(llext, test_load_unload_##_name)					\
	{									\
		struct llext_test test_case = {					\
			.name = STRINGIFY(_name),				\
			.try_userspace = _userspace,				\
			.buf_len = ARRAY_SIZE(_name ## _ext),			\
			.buf = _name ## _ext,					\
			.perm_setup = _perm_setup,				\
		};								\
		load_call_unload(&test_case);					\
	}
static LLEXT_CONST uint8_t hello_world_ext[] __aligned(4) = {
	#include "hello_world.inc"
};
LLEXT_LOAD_UNLOAD(hello_world, false, NULL)

static LLEXT_CONST uint8_t logging_ext[] __aligned(4) = {
	#include "logging.inc"
};
LLEXT_LOAD_UNLOAD(logging, true, NULL)

static LLEXT_CONST uint8_t relative_jump_ext[] __aligned(4) = {
	#include "relative_jump.inc"
};
LLEXT_LOAD_UNLOAD(relative_jump, true, NULL)

static LLEXT_CONST uint8_t object_ext[] __aligned(4) = {
	#include "object.inc"
};
LLEXT_LOAD_UNLOAD(object, true, NULL)

static LLEXT_CONST uint8_t syscalls_ext[] __aligned(4) = {
	#include "syscalls.inc"
};
LLEXT_LOAD_UNLOAD(syscalls, true, NULL)

static LLEXT_CONST uint8_t threads_kernel_objects_ext[] __aligned(4) = {
	#include "threads_kernel_objects.inc"
};
LLEXT_LOAD_UNLOAD(threads_kernel_objects, true, threads_objects_perm_setup)
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

/*
 * Ensure ext_syscall_fail is exported - as it is picked up by the syscall
 * build machinery - but points to NULL as it is not implemented.
 */
ZTEST(llext, test_ext_syscall_fail)
{
	const void * const esf_fn = llext_find_sym(NULL,
						   "z_impl_ext_syscall_fail");

	zassert_is_null(*(uintptr_t **)esf_fn, NULL,
			"ext_syscall_fail should be NULL");
}

ZTEST_SUITE(llext, NULL, NULL, NULL, NULL, NULL);
