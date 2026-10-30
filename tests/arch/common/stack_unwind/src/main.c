/*
 * Copyright (c) 2024 Meta
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#include <zephyr/kernel.h>

#if defined(STACK_WALK_API_TEST)

#include <zephyr/arch/arch_interface.h>
#include <zephyr/debug/symtab.h>

#define API_FRAMES 8

struct api_ctx {
	uintptr_t addrs[API_FRAMES];
	size_t count;
	size_t limit;
	size_t calls;
	size_t after_stop;
	bool stopped;
};

static bool api_cb(void *cookie, unsigned long addr)
{
	struct api_ctx *ctx = cookie;

	ctx->calls++;
	if (ctx->stopped) {
		ctx->after_stop++;
		return false;
	}

	if (ctx->count >= ctx->limit) {
		ctx->stopped = true;
		return false;
	}

	ctx->addrs[ctx->count++] = (uintptr_t)addr;
	return true;
}

#if CONFIG_ARCH_STACKWALK_MAX_FRAMES > 0
static const char *api_name(unsigned long addr)
{
	uint32_t offset = 0;
	const char *name = symtab_find_symbol_name(addr, &offset);

	return (name != NULL) ? name : "?";
}

static void __noinline api_walk_here(struct api_ctx *ctx)
{
	arch_stack_walk(api_cb, ctx, k_current_get(), NULL);
}

static void __noinline api_level2(struct api_ctx *ctx)
{
	api_walk_here(ctx);
}

static void __noinline api_level1(struct api_ctx *ctx)
{
	api_level2(ctx);
}

static void __noinline api_stop_here(struct api_ctx *ctx)
{
	arch_stack_walk(api_cb, ctx, k_current_get(), NULL);
}

static void __noinline api_stop_test(struct api_ctx *ctx)
{
	api_stop_here(ctx);
}

static int api_find(const struct api_ctx *ctx, const char *name)
{
	for (size_t i = 0U; i < ctx->count; i++) {
		if (strstr(api_name(ctx->addrs[i]), name) != NULL) {
			return (int)i;
		}
	}

	return -1;
}

K_THREAD_STACK_DEFINE(api_worker_stack, 1024);
static struct k_thread api_worker;
static K_SEM_DEFINE(api_worker_ready, 0, 1);

static void __noinline api_worker_deep(void)
{
	k_sem_give(&api_worker_ready);
	k_sleep(K_MSEC(500));
}

static void api_worker_entry(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	api_worker_deep();
}
#endif /* CONFIG_ARCH_STACKWALK_MAX_FRAMES > 0 */

int main(void)
{
	printf("stack walk API test\n");

#if CONFIG_ARCH_STACKWALK_MAX_FRAMES == 0
	struct api_ctx zero = {.limit = API_FRAMES};

	arch_stack_walk(api_cb, &zero, k_current_get(), NULL);
	printf("ZERO: count=%zu\n", zero.count);
#else
	struct api_ctx ctx = {.limit = API_FRAMES};
	struct api_ctx stop = {.limit = 3U};
	k_tid_t tid;

	api_level1(&ctx);
	if (ctx.count >= 3U) {
		printf("CUR: %s %s %s\n", api_name(ctx.addrs[0]), api_name(ctx.addrs[1]),
		       api_name(ctx.addrs[2]));
	} else {
		printf("CUR: FAIL (count=%zu)\n", ctx.count);
	}

	tid = k_thread_create(&api_worker, api_worker_stack,
			      K_THREAD_STACK_SIZEOF(api_worker_stack), api_worker_entry, NULL, NULL,
			      NULL, K_PRIO_PREEMPT(1), 0, K_NO_WAIT);
	if (k_sem_take(&api_worker_ready, K_SECONDS(1)) != 0) {
		printf("THR: FAIL (worker did not run)\n");
	} else {
		int deep;
		int entry;

		k_msleep(20);
		memset(&ctx, 0, sizeof(ctx));
		ctx.limit = API_FRAMES;
		arch_stack_walk(api_cb, &ctx, tid, NULL);

		deep = api_find(&ctx, "api_worker_deep");
		entry = api_find(&ctx, "api_worker_entry");
		if (deep >= 0 && entry >= 0 && deep < entry) {
			printf("THR: ok\n");
		} else {
			printf("THR: FAIL (deep=%d entry=%d)\n", deep, entry);
		}
		k_thread_abort(tid);
	}

	api_stop_test(&stop);
	printf("STOP: count=%zu calls=%zu after=%zu\n", stop.count, stop.calls, stop.after_stop);
#endif

	return 0;
}

#else /* STACK_WALK_API_TEST */

#if defined(STACK_UNWIND_LEAF_TEST)
extern void leaf_fault(void);

static void (*volatile leaf_fault_call)(void) = leaf_fault;

static void __noinline leaf_caller(void)
{
	leaf_fault_call();
	printf("unexpected return\n");
}
#else
static void func1(int a);
static void func2(int a);

static void __noinline func2(int a)
{
	printf("%d: %s\n", a, __func__);

	if (a >= 5) {
#if defined(STACK_UNWIND_NONLEAF_TEST)
		__asm__ volatile(".word 0");
#else
		k_oops();
#endif
	}

	func1(a + 1);
	printf("bottom %d: %s\n", a, __func__);
}

static void __noinline func1(int a)
{
	printf("%d: %s\n", a, __func__);
	func2(a + 1);
	printf("bottom %d: %s\n", a, __func__);
}
#endif

int main(void)
{
	printf("Hello World! %s\n", CONFIG_BOARD);

#if defined(STACK_UNWIND_LEAF_TEST)
	leaf_caller();
#else
	func1(1);
#endif

	return 0;
}

#endif /* STACK_WALK_API_TEST */
