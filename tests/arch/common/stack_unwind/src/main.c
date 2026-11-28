/*
 * Copyright (c) 2024 Meta
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#if defined(STACK_UNWIND_BUSY_LOG_TEST) || defined(STACK_UNWIND_BLOCKED_LOG_TEST)
LOG_MODULE_REGISTER(stack_unwind, LOG_LEVEL_INF);
#endif

#if defined(STACK_UNWIND_BLOCKED_LOG_TEST)
#include <zephyr/logging/log_backend.h>
#include <zephyr/logging/log_ctrl.h>
#include <zephyr/sys/atomic.h>

static atomic_t block_log_thread;
static atomic_t log_thread_blocked;

/* Once told to, blocks the logging thread for good in the middle of a message */
static void blocking_backend_process(const struct log_backend *const backend,
				     union log_msg_generic *msg)
{
	ARG_UNUSED(backend);
	ARG_UNUSED(msg);

	if (atomic_cas(&block_log_thread, 1, 0)) {
		atomic_set(&log_thread_blocked, 1);
		k_sleep(K_FOREVER);
	}
}

static void blocking_backend_panic(const struct log_backend *const backend)
{
	ARG_UNUSED(backend);
}

static const struct log_backend_api blocking_backend_api = {
	.process = blocking_backend_process,
	.panic = blocking_backend_panic,
};

LOG_BACKEND_DEFINE(blocking_backend, blocking_backend_api, true);
#endif

#if defined(STACK_WALK_API_TEST)

#include <zephyr/arch/arch_interface.h>
#include <zephyr/arch/tricore/cr.h>
#include <zephyr/debug/symtab.h>
#include <zephyr/irq_offload.h>

#include <kernel_arch_data.h>

#define API_FRAMES           16
#define API_DEEP_FRAME_COUNT 40

struct api_ctx {
	uintptr_t addrs[API_FRAMES];
	size_t count;
	size_t limit;
	size_t calls;
	size_t after_stop;
	bool stopped;
};

struct api_irq_ctx {
	struct api_ctx outer;
	struct api_ctx nested;
	struct api_ctx exception;
	struct api_ctx timer;
	uint32_t pcxi;
	bool preserved;
	bool returned;
	volatile bool sampled;
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

static void api_irq_inner(const void *arg)
{
	struct api_irq_ctx *const *param = arg;
	struct api_irq_ctx *ctx = *param;
	uint32_t pcxi = cr_read(TRICORE_PCXI);

	ctx->preserved = k_current_get()->callee_saved.pcxi == ctx->pcxi;
	arch_stack_walk(api_cb, &ctx->nested, k_current_get(), NULL);

	/* Start an exception walk at the nested trap's lower context. */
	for (size_t i = 0U; i < CONFIG_TRICORE_CSA_COUNT && pcxi != 0U; i++) {
		const union z_tricore_context *csa = UINT_TO_POINTER(z_tricore_pcx_to_addr(pcxi));

		if ((pcxi & BIT(20)) == 0U) {
			arch_stack_walk(api_cb, &ctx->exception, k_current_get(),
					(const struct arch_esf *)csa);
			break;
		}
		pcxi = csa->upper.pcxi;
	}
}

static void api_irq_outer(const void *arg)
{
	struct api_irq_ctx *const *param = arg;
	struct api_irq_ctx *ctx = *param;

	ctx->pcxi = k_current_get()->callee_saved.pcxi;
	arch_stack_walk(api_cb, &ctx->outer, k_current_get(), NULL);
	irq_offload(api_irq_inner, arg);
	ctx->preserved = ctx->preserved && k_current_get()->callee_saved.pcxi == ctx->pcxi;
}

static void __noinline api_irq_thread(struct api_irq_ctx *ctx)
{
	irq_offload(api_irq_outer, &ctx);
	ctx->returned = !k_is_in_isr();
}

static void api_timer_handler(struct k_timer *timer)
{
	struct api_irq_ctx *ctx = k_timer_user_data_get(timer);

	arch_stack_walk(api_cb, &ctx->timer, k_current_get(), NULL);
	ctx->sampled = true;
}

static void __noinline api_timer_thread(struct api_irq_ctx *ctx)
{
	for (uint32_t i = 0U; i < 1000U && !ctx->sampled; i++) {
		k_busy_wait(1000U);
	}
}

static void api_irq_test(void)
{
	static K_TIMER_DEFINE(timer, api_timer_handler, NULL);
	struct api_irq_ctx ctx = {
		.outer.limit = API_FRAMES,
		.nested.limit = API_FRAMES,
		.exception.limit = API_FRAMES,
		.timer.limit = API_FRAMES,
	};
	int handler;
	int thread;

	api_irq_thread(&ctx);
	k_timer_user_data_set(&timer, &ctx);
	k_timer_start(&timer, K_MSEC(10), K_NO_WAIT);
	api_timer_thread(&ctx);
	k_timer_stop(&timer);
	k_timer_user_data_set(&timer, NULL);
	handler = api_find(&ctx.exception, "api_irq_outer");
	thread = api_find(&ctx.exception, "api_irq_thread");
	if (ctx.preserved && ctx.returned && ctx.sampled &&
	    api_find(&ctx.outer, "api_irq_thread") >= 0 &&
	    api_find(&ctx.outer, "api_irq_outer") < 0 &&
	    api_find(&ctx.nested, "api_irq_thread") >= 0 &&
	    api_find(&ctx.nested, "api_irq_inner") < 0 &&
	    api_find(&ctx.nested, "api_irq_outer") < 0 && handler >= 0 && thread > handler &&
	    api_find(&ctx.timer, "api_timer_thread") >= 0 &&
	    api_find(&ctx.timer, "api_timer_handler") < 0) {
		printf("IRQ: ok\n");
	} else {
		printf("IRQ: FAIL (preserved=%u returned=%u handler=%d thread=%d)\n", ctx.preserved,
		       ctx.returned, handler, thread);
	}
}

K_THREAD_STACK_DEFINE(api_worker_stack, 1024);
static struct k_thread api_worker;
static K_SEM_DEFINE(api_worker_ready, 0, 1);

static void __noinline api_worker_deep(void)
{
	k_sem_give(&api_worker_ready);
	k_sleep(K_FOREVER);
}

#define api_stack_frame_0 api_worker_deep
#define API_DEFINE_STACK_FRAME(n, _)                                                               \
	static void __noinline UTIL_CAT(api_stack_frame_, UTIL_INC(n))(void)                       \
	{                                                                                          \
		UTIL_CAT(api_stack_frame_, n)();                                                   \
		/* Prevent a tail call so this frame stays on the CSA chain. */                    \
		k_busy_wait(1U);                                                                   \
	}

LISTIFY(API_DEEP_FRAME_COUNT, API_DEFINE_STACK_FRAME, ())

#undef API_DEFINE_STACK_FRAME
#undef api_stack_frame_0

static void api_worker_entry(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	api_worker_deep();
}

static void api_deep_worker_entry(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	UTIL_CAT(api_stack_frame_, API_DEEP_FRAME_COUNT)();
}

static k_tid_t api_start_worker(k_thread_entry_t entry)
{
	k_tid_t tid = k_thread_create(&api_worker, api_worker_stack,
				      K_THREAD_STACK_SIZEOF(api_worker_stack), entry, NULL, NULL,
				      NULL, K_PRIO_PREEMPT(1), 0, K_NO_WAIT);

	if (k_sem_take(&api_worker_ready, K_SECONDS(1)) != 0) {
		k_thread_abort(tid);
		return NULL;
	}

	k_msleep(20);
	return tid;
}

struct api_abort_ctx {
	struct api_ctx trace;
	k_tid_t thread;
};

static void api_abort_worker(const void *arg)
{
	const k_tid_t *thread = arg;

	k_thread_abort(*thread);
}

static bool api_abort_cb(void *cookie, unsigned long addr)
{
	struct api_abort_ctx *ctx = cookie;

	if (ctx->trace.calls == 0U) {
		if (!arch_cpu_irqs_are_enabled()) {
			return false;
		}

		irq_offload(api_abort_worker, &ctx->thread);
	}

	return api_cb(&ctx->trace, addr);
}

static bool api_same_trace(const struct api_ctx *first, const struct api_ctx *second)
{
	return first->count > 0U && first->count == second->count &&
	       memcmp(first->addrs, second->addrs, first->count * sizeof(first->addrs[0])) == 0;
}

static bool api_abort_test(k_tid_t tid, const struct arch_esf *esf, const struct api_ctx *expected)
{
	struct api_abort_ctx ctx = {.trace.limit = API_FRAMES, .thread = tid};

	/* Reclaim the worker's CSAs before consuming the rest of the trace. */
	arch_stack_walk(api_abort_cb, &ctx, tid, esf);
	if (tid->callee_saved.pcxi != 0U) {
		k_thread_abort(tid);
		return false;
	}

	return api_same_trace(expected, &ctx.trace);
}

static void api_esf_abort_test(void)
{
	struct api_ctx expected = {.limit = API_FRAMES};
	struct api_ctx reclaimed = {.limit = API_FRAMES};
	const struct arch_esf *esf;
	k_tid_t tid = api_start_worker(api_worker_entry);
	uint32_t pcxi;
	bool passed;

	if (tid == NULL) {
		printf("ESF ABORT: FAIL (worker did not run)\n");
		return;
	}

	pcxi = tid->callee_saved.pcxi;
	if (pcxi == 0U || (pcxi & BIT(20)) != 0U) {
		k_thread_abort(tid);
		printf("ESF ABORT: FAIL (expected a saved lower context)\n");
		return;
	}

	esf = UINT_TO_POINTER(z_tricore_pcx_to_addr(pcxi));
	arch_stack_walk(api_cb, &expected, tid, esf);
	passed = api_abort_test(tid, esf, &expected);
	printf("ESF ABORT: %s\n", passed ? "ok" : "FAIL");

	/* The same address remains in the pool after the worker is gone. */
	arch_stack_walk(api_cb, &reclaimed, tid, esf);
	printf("ESF RECLAIMED: count=%zu\n", reclaimed.count);
}

static bool api_count_frame(void *cookie, unsigned long addr)
{
	size_t *count = cookie;

	ARG_UNUSED(addr);
	(*count)++;
	return true;
}

static void api_snapshot_limit_test(void)
{
	struct api_ctx stop = {.limit = 3U};
	size_t count = 0U;
	k_tid_t tid = api_start_worker(api_deep_worker_entry);

	if (tid == NULL) {
		printf("SNAPSHOT LIMIT: FAIL (worker did not run)\n");
		return;
	}

	arch_stack_walk(api_count_frame, &count, tid, NULL);
	arch_stack_walk(api_cb, &stop, tid, NULL);
	k_thread_abort(tid);
	printf("SNAPSHOT LIMIT: count=%zu\n", count);
	printf("SNAPSHOT STOP: count=%zu calls=%zu after=%zu\n", stop.count, stop.calls,
	       stop.after_stop);
}

struct api_irq_abort_ctx {
	struct api_ctx before;
	struct api_ctx after;
};

static void api_irq_abort_inner(const void *arg)
{
	struct api_irq_abort_ctx *const *param = arg;
	struct api_irq_abort_ctx *ctx = *param;

	arch_stack_walk(api_cb, &ctx->after, k_current_get(), NULL);
}

static void api_irq_abort_outer(const void *arg)
{
	struct api_irq_abort_ctx *const *param = arg;
	struct api_irq_abort_ctx *ctx = *param;

	arch_stack_walk(api_cb, &ctx->before, k_current_get(), NULL);
	k_thread_abort(k_current_get());
	irq_offload(api_irq_abort_inner, arg);
}

static void api_irq_abort_entry(void *p1, void *p2, void *p3)
{
	struct api_irq_abort_ctx *ctx = p1;

	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	irq_offload(api_irq_abort_outer, &ctx);
}

static void api_irq_abort_test(void)
{
	struct api_irq_abort_ctx ctx = {.before.limit = API_FRAMES, .after.limit = API_FRAMES};
	k_tid_t tid = k_thread_create(&api_worker, api_worker_stack,
				      K_THREAD_STACK_SIZEOF(api_worker_stack), api_irq_abort_entry,
				      &ctx, NULL, NULL, K_PRIO_PREEMPT(1), 0, K_NO_WAIT);

	if (k_thread_join(tid, K_SECONDS(1)) != 0) {
		k_thread_abort(tid);
		printf("IRQ ABORT: FAIL (worker did not exit)\n");
		return;
	}

	printf("IRQ ABORT: %s\n", api_same_trace(&ctx.before, &ctx.after) ? "ok" : "FAIL");
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

	tid = api_start_worker(api_worker_entry);
	if (tid == NULL) {
		printf("THR: FAIL (worker did not run)\n");
	} else {
		int deep;
		int entry;
		bool passed;

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
		passed = api_abort_test(tid, NULL, &ctx);
		printf("ABORT: %s\n", passed ? "ok" : "FAIL");
	}

	api_esf_abort_test();
	api_snapshot_limit_test();
	api_stop_test(&stop);
	printf("STOP: count=%zu calls=%zu after=%zu\n", stop.count, stop.calls, stop.after_stop);
	api_irq_test();
	api_irq_abort_test();
#endif

	return 0;
}

#elif defined(STACK_UNWIND_TRICORE_SPURIOUS_TEST)

#include <zephyr/arch/exception.h>
#include <zephyr/irq_offload.h>
#include <zephyr/sw_isr_table.h>

static void frame_dump_hook(const char *format, va_list args)
{
	if (strncmp(format, "     %2d:", sizeof("     %2d:") - 1U) == 0) {
		printf("HOOK FRAME:");
		vprintf(format, args);
	}
}

int main(void)
{
	arch_exception_set_dump_hook(frame_dump_hook, NULL);
	irq_offload(z_irq_spurious, NULL);

	return 0;
}

#else

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

#if defined(STACK_UNWIND_BUSY_LOG_TEST)
	/* Keep the logging thread printing on another CPU while the fault is reported */
	for (int i = 0; i < 20; i++) {
		LOG_INF("message %d before the fault", i);
	}
#endif

#if defined(STACK_UNWIND_BLOCKED_LOG_TEST)
	/*
	 * Leave the logging thread blocked with a message claimed. This thread
	 * keeps its CPU meanwhile, so the logging thread runs on another one.
	 */
	atomic_set(&block_log_thread, 1);
	LOG_INF("message before the fault");
	log_thread_trigger();
	while (atomic_get(&log_thread_blocked) == 0) {
		k_busy_wait(USEC_PER_MSEC);
	}
#endif

#if defined(STACK_UNWIND_LEAF_TEST)
	leaf_caller();
#else
	func1(1);
#endif

	return 0;
}

#endif
