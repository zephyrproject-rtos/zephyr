/*
 * Copyright (c) 2024 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <app_api.h>

#include <zephyr/kernel.h>
#include <zephyr/llext/llext.h>
#include <zephyr/llext/buf_loader.h>
#include <zephyr/app_memory/mem_domain.h>

#ifdef LOAD_AND_RUN_EXTENSION
static const unsigned char extension_llext[] = {
	#include <extension.inc>
};
static const size_t extension_llext_len = ARRAY_SIZE(extension_llext);
#endif

#define STACK_SIZE 1024
#define HEAP_SIZE 1024

#ifdef LOAD_AND_RUN_EXTENSION
struct k_thread kernel_thread, user_thread;

K_THREAD_STACK_DEFINE(stack_kernel, STACK_SIZE);
K_THREAD_STACK_DEFINE(stack_user, STACK_SIZE);

K_HEAP_DEFINE(heap_kernel, HEAP_SIZE);
K_HEAP_DEFINE(heap_user, HEAP_SIZE);

static void thread_entry(void *p1, void *p2, void *p3)
{
	int bar;
	int (*start_fn)(int) = p1;

	printk("Calling extension from %s\n",
	       k_is_user_context() ? "user" : "kernel");

	if (k_is_user_context()) {
		bar = 42;
	} else {
		bar = 43;
	}

	start_fn(bar);
}

void load_and_run_extension(int thread_flags, struct k_thread *thread,
			    struct k_mem_domain *domain,
			    k_thread_stack_t *stack, struct k_heap *heap,
			    struct llext **ext)
{
	struct llext_buf_loader buf_loader = LLEXT_BUF_LOADER(extension_llext,
							      extension_llext_len);
	struct llext_loader *loader = &buf_loader.loader;
	struct llext_load_param ldr_parm = LLEXT_LOAD_PARAM_DEFAULT;
	int (*start_fn)(int bar);

	llext_load(loader, "extension", ext, &ldr_parm);
	start_fn = llext_find_sym(&(*ext)->exp_tab, "start");

	llext_add_domain(*ext, domain);

	k_thread_create(thread, stack, STACK_SIZE,
			thread_entry, start_fn, NULL, NULL, -1,
			K_INHERIT_PERMS | thread_flags,
			K_FOREVER);
	k_mem_domain_add_thread(domain, thread);
	k_thread_heap_assign(thread, heap);

	k_thread_start(thread);
	k_thread_join(thread, K_FOREVER);

	llext_unload(ext);
}
#endif

int main(void)
{
#ifdef LOAD_AND_RUN_EXTENSION
	struct k_mem_domain domain_kernel, domain_user;
	struct llext *ext_kernel, *ext_user;

	k_mem_domain_init(&domain_kernel, 0, NULL);
	k_mem_domain_init(&domain_user, 0, NULL);

	load_and_run_extension(0, &kernel_thread, &domain_kernel,
			       stack_kernel, &heap_kernel, &ext_kernel);
	load_and_run_extension(K_USER, &user_thread, &domain_user,
			       stack_user, &heap_user, &ext_user);

	printk("Done\n");
#else
	printk("Extension not loaded\n");
#endif
	return 0;
}
