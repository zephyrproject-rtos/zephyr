/*
 * Copyright (c) 2024 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/app_memory/mem_domain.h>
#include <zephyr/llext/llext.h>
#include <zephyr/llext/buf_loader.h>

#include <app_api.h>

#include <string.h>

/**
 * Assume that if the extension 1 is not built, we are building the
 * EDK. If others are not built, this will just fail.
 */
#if defined __has_include
#  if __has_include("../../ext1/build/ext1.inc")
#    undef EDK_BUILD
#  else
#    pragma message "Extension 1 not built, assuming EDK build."
#    define EDK_BUILD
#  endif
#endif

#ifndef EDK_BUILD
#include "../../ext1/build/ext1.inc"
#define ext1_inc ext1_llext
#define ext1_len ext1_llext_len
#include "../../ext2/build/ext2.inc"
#define ext2_inc ext2_llext
#define ext2_len ext2_llext_len
#include "../../ext3/build/ext3.inc"
#define ext3_inc ext3_llext
#define ext3_len ext3_llext_len
#ifndef CONFIG_LLEXT_EDK_USERSPACE_ONLY
#include "../../k-ext1/build/kext1.inc"
#define kext1_inc kext1_llext
#define kext1_len kext1_llext_len
#endif
#endif

#define USER_STACKSIZE  2048
#define USER_HEAPSIZE  8192
#define MAX_EXTENSIONS 4

extern k_tid_t start_subscriber_thread(void);

/* Maybe make all this depend on MAX_EXTENSIONS? */
struct k_thread user_thread1, user_thread2, user_thread3, kernel_thread1;
K_THREAD_STACK_DEFINE(user_stack1, USER_STACKSIZE);
K_THREAD_STACK_DEFINE(user_stack2, USER_STACKSIZE);
K_THREAD_STACK_DEFINE(user_stack3, USER_STACKSIZE);
K_THREAD_STACK_DEFINE(kernel_stack1, USER_STACKSIZE);

K_HEAP_DEFINE(user_heap1, USER_HEAPSIZE);
K_HEAP_DEFINE(user_heap2, USER_HEAPSIZE);
K_HEAP_DEFINE(user_heap3, USER_HEAPSIZE);
K_HEAP_DEFINE(kernel_heap1, USER_HEAPSIZE);

struct {
	k_tid_t thread;
	struct llext *ext;
} extension_threads[MAX_EXTENSIONS];
int max_extension_thread_idx;

static const void * const load(const char *name, struct llext **ext, void *buf,
			       size_t len)
{
#ifndef EDK_BUILD
	struct llext_buf_loader buf_loader = LLEXT_BUF_LOADER(buf, len);
	struct llext_loader *loader = &buf_loader.loader;
	struct llext_load_param ldr_parm = LLEXT_LOAD_PARAM_DEFAULT;

	llext_load(loader, name, ext, &ldr_parm);

	return llext_find_sym(&(*ext)->exp_tab, "start");
#else
	return NULL;
#endif
}

static void unload(struct llext **ext)
{
	llext_unload(ext);
}

static void user_function(void *p1, void *p2, void *p3)
{
	int (*start_fn)(void) = p1;

	printk("[app]Thread %p created to run extension [%s], at %s\n",
	       k_current_get(), (char *)p2,
	       k_is_user_context() ? "userspace." : "privileged mode.");

	start_fn();
	printk("[app]Thread %p done\n", k_current_get());
}

void k_sys_fatal_error_handler(unsigned int reason, const struct arch_esf *esf)
{
	int i;

	printk("[app]Fatal handler! Thread: %p\n", k_current_get());

	for (i = 0; i < max_extension_thread_idx; i++) {
		if (extension_threads[i].thread == k_current_get()) {
			unload(&extension_threads[i].ext);
		}
	}
}

void run_extension_on_thread(void *ext_inc, size_t ext_len,
			     struct k_mem_domain *domain,
			     struct k_thread *thread,
			     k_thread_stack_t *stack,
			     struct k_heap *heap,
			     const char *name,
			     k_tid_t subscriber_thread_id,
			     int flag)
{
	int (*start_fn)(void);
	struct llext **ext = &extension_threads[max_extension_thread_idx].ext;

	printk("[app]Loading extension [%s].\n", name);
	start_fn = load(name, ext, ext_inc, ext_len);

	llext_add_domain(*ext, domain);

	k_thread_create(thread, stack, USER_STACKSIZE,
			user_function, start_fn, (void *)name, NULL,
			-1, flag | K_INHERIT_PERMS, K_FOREVER);
	k_mem_domain_add_thread(domain, thread);
	k_mem_domain_add_thread(domain, subscriber_thread_id);

	k_thread_heap_assign(thread, heap);

	extension_threads[max_extension_thread_idx].thread = thread;
	max_extension_thread_idx++;

	k_thread_start(thread);
}

int main(void)
{
	struct k_mem_domain domain1, domain2, domain3, kdomain1;

#ifndef EDK_BUILD
	k_tid_t subscriber_thread_id = start_subscriber_thread();
#endif
	/* This and all other similar sleeps are here to provide a chance for
	 * the newly created thread to run.
	 */
	k_sleep(K_MSEC(1));

	k_mem_domain_init(&domain1, 0, NULL);
	k_mem_domain_init(&domain2, 0, NULL);
	k_mem_domain_init(&domain3, 0, NULL);
	k_mem_domain_init(&kdomain1, 0, NULL);

#ifndef EDK_BUILD
#ifndef CONFIG_LLEXT_EDK_USERSPACE_ONLY
	run_extension_on_thread(kext1_inc, kext1_len, &kdomain1,
				&kernel_thread1, kernel_stack1, &kernel_heap1,
				"kext1", subscriber_thread_id, 0);
	k_sleep(K_MSEC(1));
#endif
	run_extension_on_thread(ext1_inc, ext1_len, &domain1, &user_thread1,
				user_stack1, &user_heap1, "ext1",
				subscriber_thread_id, K_USER);
	k_sleep(K_MSEC(1));
	run_extension_on_thread(ext2_inc, ext2_len, &domain2, &user_thread2,
				user_stack2, &user_heap2, "ext2",
				subscriber_thread_id, K_USER);
	k_sleep(K_MSEC(1));
	run_extension_on_thread(ext3_inc, ext3_len, &domain3, &user_thread3,
				user_stack3, &user_heap3, "ext3",
				subscriber_thread_id, K_USER);
#endif

	k_sleep(K_FOREVER);

	return 0;
}
