/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * ZTEST-based unit tests for the Zfence implementation.
 *
 * Tests cover:
 *   - Initialization (zfence_init, ZFENCE_DEFINE)
 *   - Sequence reservation (zfence_next)
 *   - Incremental signalling (zfence_signal)
 *   - Explicit/jump-ahead signalling (zfence_signal_seq)
 *   - Result query (zfence_result, zfence_current)
 *   - Wait context allocation (zfence_wait_ctx_init / zfence_wait_ctx_deinit)
 *   - Synchronous wait: fast-path, blocking, timeout, concurrent (zfence_wait)
 *   - NULL / invalid argument handling
 */

#include <zephyr/kernel.h>
#include <zephyr/zfence/zfence.h>
#include <zephyr/ztest.h>
#include <stdint.h>

/* Shared test state */

/*
 * Global fence instance re-initialised before every test by
 * zfence_test_before().  Using ZFENCE_DEFINE ensures the Zephyr k_event
 * and spinlock are properly initialised at compile time as well.
 */
static ZFENCE_DEFINE(g_fence);

/* -------------------------------------------------------------------------
 * Per-test setup: re-initialise the fence.
 * ----------------------------------------------------------------------
 */

static void zfence_test_before(void *data)
{
	ARG_UNUSED(data);
	zfence_init(&g_fence);
}

/* Thread helpers for blocking-wait tests */

#define WAIT_THREAD_STACK_SIZE  1024
#define WAIT_THREAD_PRIORITY    5   /* preemptive, lower than test thread */

K_THREAD_STACK_DEFINE(wait_thread_stack,  WAIT_THREAD_STACK_SIZE);
K_THREAD_STACK_DEFINE(wait_thread2_stack, WAIT_THREAD_STACK_SIZE);

/* Stacks for concurrent-producer and concurrent-next stress tests. */
K_THREAD_STACK_DEFINE(signal_thread_stack1, WAIT_THREAD_STACK_SIZE);
K_THREAD_STACK_DEFINE(signal_thread_stack2, WAIT_THREAD_STACK_SIZE);
K_THREAD_STACK_DEFINE(next_thread_stack1,   WAIT_THREAD_STACK_SIZE);
K_THREAD_STACK_DEFINE(next_thread_stack2,   WAIT_THREAD_STACK_SIZE);

static struct k_thread wait_thread_data;
static struct k_thread wait_thread2_data;
static struct k_thread signal_thread1_data;
static struct k_thread signal_thread2_data;
static struct k_thread next_thread1_data;
static struct k_thread next_thread2_data;

/* Shared results for concurrent stress tests. */
static int    g_signal_ret1, g_signal_ret2;
static uint64_t g_next_seq1,  g_next_seq2;

struct wait_thread_args {
	struct zfence        *fence;
	uint64_t              seq;
	k_timeout_t           timeout;
	int                   ret;
	int                   result;
	zfence_wait_context_t ctx;
};

static struct wait_thread_args g_wait_args;
static struct wait_thread_args g_wait_args2;

static void wait_thread_fn(void *p1, void *p2, void *p3)
{
	struct wait_thread_args *args = (struct wait_thread_args *)p1;

	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	args->ret = zfence_wait(args->fence, &args->ctx, args->seq, args->timeout, &args->result);
}

/* Thread function for concurrent-producer stress test. */
static void signal_thread_fn(void *p1, void *p2, void *p3)
{
	struct zfence *fence = (struct zfence *)p1;
	int *ret = (int *)p2;

	ARG_UNUSED(p3);
	*ret = zfence_signal(fence, ZFENCE_RESULT_OK);
}

/* Thread function for concurrent-next stress test. */
static void next_thread_fn(void *p1, void *p2, void *p3)
{
	struct zfence *fence = (struct zfence *)p1;
	uint64_t *seq = (uint64_t *)p2;

	ARG_UNUSED(p3);
	*seq = zfence_next(fence);
}

/* Test 1: Initialization */

ZTEST(zfence_suite, test_init_defaults)
{
	struct zfence f;

	zfence_init(&f);

	zassert_equal((long)atomic_get(&f.signal_seq), 0L, "signal_seq must be 0 after init");
	zassert_equal((long)atomic_get(&f.reserved_seq), 0L, "reserved_seq must be 0 after init");
	zassert_equal((int)atomic_get(&f.result), ZFENCE_RESULT_UNSIGNALED,
		"result must be UNSIGNALED after init");
}

ZTEST(zfence_suite, test_init_null_noop)
{
	/* zfence_init(NULL) must not crash */
	zfence_init(NULL);
}

ZTEST(zfence_suite, test_define_initial_state)
{
	/*
	 * ZFENCE_DEFINE must produce a fully initialized fence at compile time,
	 * equivalent to calling zfence_init().  Verify the initial state of a
	 * freshly defined fence before zfence_init() is called on it.
	 * g_fence is defined with ZFENCE_DEFINE; zfence_test_before() calls
	 * zfence_init() on it, so use a separate static instance here.
	 */
	static ZFENCE_DEFINE(defined_fence);

	zassert_equal((long)atomic_get(&defined_fence.signal_seq), 0L,
		"ZFENCE_DEFINE: signal_seq must be 0");
	zassert_equal((long)atomic_get(&defined_fence.reserved_seq), 0L,
		"ZFENCE_DEFINE: reserved_seq must be 0");
	zassert_equal((int)atomic_get(&defined_fence.result), ZFENCE_RESULT_UNSIGNALED,
		"ZFENCE_DEFINE: result must be UNSIGNALED");
}

ZTEST(zfence_suite, test_init_reinit)
{
	/* Signal the fence, then re-initialize and verify all state is reset. */
	zfence_next(&g_fence);
	zfence_signal(&g_fence, ZFENCE_RESULT_OK);

	zassert_equal(zfence_current(&g_fence), 1ULL,
	"signal_seq must be 1 before reinit");

	/* Re-initialize - must reset all state to initial values. */
	zfence_init(&g_fence);

	zassert_equal(zfence_current(&g_fence), 0ULL, "signal_seq must be 0 after reinit");
	zassert_equal((long)atomic_get(&g_fence.reserved_seq), 0L,
		"reserved_seq must be 0 after reinit");
	zassert_equal(zfence_result(&g_fence, 1), ZFENCE_RESULT_UNSIGNALED,
		"seq 1 must be UNSIGNALED after reinit");
}

/* Test 2: zfence_next - sequence reservation */

ZTEST(zfence_suite, test_next_monotonic)
{
	uint64_t s1 = zfence_next(&g_fence);
	uint64_t s2 = zfence_next(&g_fence);
	uint64_t s3 = zfence_next(&g_fence);

	zassert_equal(s1, 1ULL, "first reservation must be 1");
	zassert_equal(s2, 2ULL, "second reservation must be 2");
	zassert_equal(s3, 3ULL, "third reservation must be 3");

	zassert_equal((long)atomic_get(&g_fence.reserved_seq), 3L,
		"reserved_seq must be 3 after three reservations");
}

ZTEST(zfence_suite, test_next_catches_up_to_signal_seq)
{
	/*
	 * Diagram 2: when reserved_seq < signal_seq the next value must be
	 * signal_seq + 1 (not reserved_seq + 1).
	 *
	 * Simulate a jump-ahead signal that advances signal_seq past
	 * reserved_seq, then verify that zfence_next returns signal_seq + 1.
	 */
	zfence_next(&g_fence); /* reserved_seq = 1 */
	zfence_next(&g_fence); /* reserved_seq = 2 */

	/* Force signal_seq ahead of reserved_seq */
	atomic_set(&g_fence.signal_seq, 5);

	uint64_t s = zfence_next(&g_fence);

	zassert_equal(s, 6ULL, "next must be signal_seq+1 when signal_seq > reserved_seq");
	zassert_equal((long)atomic_get(&g_fence.reserved_seq), 6L,
		"reserved_seq must be updated to 6");
}

ZTEST(zfence_suite, test_next_null_returns_einval)
{
	uint64_t ret = zfence_next(NULL);

	zassert_equal(ret, (uint64_t)-EINVAL, "zfence_next(NULL) must return (uint64_t)-EINVAL");
}

ZTEST(zfence_suite, test_next_after_signal_seq_jump)
{
	/*
	 * Use the API (not atomic_set) to advance signal_seq via a jump-ahead
	 * signal, then verify zfence_next returns signal_seq + 1.
	 */
	zfence_next(&g_fence); /* reserved_seq = 1 */
	zfence_next(&g_fence); /* reserved_seq = 2 */
	zfence_next(&g_fence); /* reserved_seq = 3 */

	/* Jump-ahead signal to seq 10 (skips 1, 2, 3). */
	zassert_equal(zfence_signal_seq(&g_fence, 10, ZFENCE_RESULT_OK), 0,
		"jump-ahead signal must succeed");

	uint64_t s = zfence_next(&g_fence);

	zassert_equal(s, 11ULL, "next after jump-ahead must be signal_seq+1 = 11");
}

/* Test 3: zfence_signal - incremental signalling */

ZTEST(zfence_suite, test_signal_increments_seq)
{
	zfence_next(&g_fence); /* reserve seq 1 */

	int ret = zfence_signal(&g_fence, ZFENCE_RESULT_OK);

	zassert_equal(ret, 0, "zfence_signal must return 0 on success");
	zassert_equal((long)atomic_get(&g_fence.signal_seq), 1L,
		"signal_seq must be 1 after first signal");
	zassert_equal((int)atomic_get(&g_fence.result), ZFENCE_RESULT_OK,
		"result must be ZFENCE_RESULT_OK");
}

ZTEST(zfence_suite, test_signal_multiple_increments)
{
	for (int i = 0; i < 3; i++) {
		zfence_next(&g_fence);
	}

	zassert_equal(zfence_signal(&g_fence, ZFENCE_RESULT_OK), 0, "signal 1 must succeed");
	zassert_equal((long)atomic_get(&g_fence.signal_seq), 1L, "signal_seq must be 1");

	zassert_equal(zfence_signal(&g_fence, -EIO), 0, "signal 2 must succeed");
	zassert_equal((long)atomic_get(&g_fence.signal_seq), 2L, "signal_seq must be 2");

	zassert_equal(zfence_signal(&g_fence, ZFENCE_RESULT_OK), 0, "signal 3 must succeed");
	zassert_equal((long)atomic_get(&g_fence.signal_seq), 3L, "signal_seq must be 3");
}

ZTEST(zfence_suite, test_signal_rejects_unsignaled_result)
{
	zfence_next(&g_fence);

	int ret = zfence_signal(&g_fence, ZFENCE_RESULT_UNSIGNALED);

	zassert_equal(ret, -EINVAL, "signalling with UNSIGNALED result must return -EINVAL");
}

ZTEST(zfence_suite, test_signal_rejects_overwritten_result)
{
	zfence_next(&g_fence);

	int ret = zfence_signal(&g_fence, ZFENCE_RESULT_OVERWRITTEN);

	zassert_equal(ret, -EINVAL, "signalling with OVERWRITTEN result must return -EINVAL");
}

ZTEST(zfence_suite, test_signal_null_fence)
{
	int ret = zfence_signal(NULL, ZFENCE_RESULT_OK);

	zassert_equal(ret, -EINVAL, "zfence_signal(NULL) must return -EINVAL");
}

ZTEST(zfence_suite, test_signal_without_next)
{
	/*
	 * zfence_signal() on a fresh fence (signal_seq=0) is valid - it
	 * advances signal_seq to 1 without requiring a prior zfence_next().
	 */
	int ret = zfence_signal(&g_fence, ZFENCE_RESULT_OK);

	zassert_equal(ret, 0, "signal without next must succeed");
	zassert_equal(zfence_current(&g_fence), 1ULL,
		"signal_seq must be 1 after signal without next");
	zassert_equal(zfence_result(&g_fence, 1), ZFENCE_RESULT_OK, "result for seq 1 must be OK");
}

ZTEST(zfence_suite, test_signal_positive_result_code)
{
	/* Positive non-zero values are valid custom result codes.
	 * The implementation must accept any value except ZFENCE_RESULT_UNSIGNALED
	 * and ZFENCE_RESULT_OVERWRITTEN.
	 */
	zfence_wait_context_t ctx = {0};
	int result = -1;

	zfence_next(&g_fence);
	zassert_equal(zfence_signal(&g_fence, 42), 0,
		"signal with positive result code must succeed");

	zassert_equal(zfence_result(&g_fence, 1), 42,
		"zfence_result must return the positive custom code");

	zassert_equal(zfence_wait_ctx_init(&ctx, &g_fence), 0, "ctx init must succeed");
	zassert_equal(zfence_wait(&g_fence, &ctx, 1, K_NO_WAIT, &result), 0,
		"fast-path wait must succeed");
	zassert_equal(result, 42, "zfence_wait must propagate the positive custom code");

	zfence_wait_ctx_deinit(&ctx);
}

/* Test 4: zfence_signal_seq - explicit / jump-ahead signalling */

ZTEST(zfence_suite, test_signal_seq_jump_ahead)
{
	/* Reserve 5 values, then signal seq 5 directly (jump-ahead). */
	for (int i = 0; i < 5; i++) {
		zfence_next(&g_fence);
	}

	int ret = zfence_signal_seq(&g_fence, 5, -EIO);

	zassert_equal(ret, 0, "zfence_signal_seq must return 0 on success");
	zassert_equal((long)atomic_get(&g_fence.signal_seq), 5L,
		"signal_seq must be 5 after jump signal");
	zassert_equal((int)atomic_get(&g_fence.result), -EIO, "result must be -EIO");
}

ZTEST(zfence_suite, test_signal_seq_already_signaled)
{
	zfence_next(&g_fence);
	zfence_signal_seq(&g_fence, 1, ZFENCE_RESULT_OK);

	/* Same seq again */
	int ret = zfence_signal_seq(&g_fence, 1, ZFENCE_RESULT_OK);

	zassert_equal(ret, -EALREADY, "re-signalling same seq must return -EALREADY");
}

ZTEST(zfence_suite, test_signal_seq_stale)
{
	for (int i = 0; i < 3; i++) {
		zfence_next(&g_fence);
	}
	zfence_signal_seq(&g_fence, 3, ZFENCE_RESULT_OK);

	/* Stale (earlier) seq */
	int ret = zfence_signal_seq(&g_fence, 2, ZFENCE_RESULT_OK);

	zassert_equal(ret, -EALREADY, "signalling stale seq must return -EALREADY");
}

ZTEST(zfence_suite, test_signal_seq_invalid_result)
{
	zfence_next(&g_fence);

	int ret = zfence_signal_seq(&g_fence, 1, ZFENCE_RESULT_UNSIGNALED);

	zassert_equal(ret, -EINVAL, "UNSIGNALED result must return -EINVAL");
}

ZTEST(zfence_suite, test_signal_seq_rejects_overwritten_result)
{
	zfence_next(&g_fence);

	int ret = zfence_signal_seq(&g_fence, 1, ZFENCE_RESULT_OVERWRITTEN);

	zassert_equal(ret, -EINVAL, "OVERWRITTEN result must return -EINVAL");
}

ZTEST(zfence_suite, test_signal_seq_null_fence)
{
	int ret = zfence_signal_seq(NULL, 1, ZFENCE_RESULT_OK);

	zassert_equal(ret, -EINVAL, "NULL fence must return -EINVAL");
}

ZTEST(zfence_suite, test_signal_seq_zero_returns_ealready)
{
	/*
	 * signal_seq starts at 0.  Signalling seq=0 must return -EALREADY
	 * because cur_val (0) >= new_val (0).
	 */
	int ret = zfence_signal_seq(&g_fence, 0, ZFENCE_RESULT_OK);

	zassert_equal(ret, -EALREADY, "zfence_signal_seq with seq=0 must return -EALREADY");
}

/* Test 5: zfence_result - result query */

ZTEST(zfence_suite, test_result_unsignaled)
{
	zfence_next(&g_fence);

	int r = zfence_result(&g_fence, 1);

	zassert_equal(r, ZFENCE_RESULT_UNSIGNALED, "unreached seq must return UNSIGNALED");
}

ZTEST(zfence_suite, test_result_current)
{
	zfence_next(&g_fence);
	zfence_signal(&g_fence, ZFENCE_RESULT_OK);

	int r = zfence_result(&g_fence, 1);

	zassert_equal(r, ZFENCE_RESULT_OK, "current seq must return the stored result");
}

ZTEST(zfence_suite, test_result_overwritten)
{
	/* Signal seq 3 via jump-ahead; seq 1 and 2 are implicitly overwritten. */
	for (int i = 0; i < 3; i++) {
		zfence_next(&g_fence);
	}
	zfence_signal_seq(&g_fence, 3, ZFENCE_RESULT_OK);

	zassert_equal(zfence_result(&g_fence, 1), ZFENCE_RESULT_OVERWRITTEN,
		"seq 1 must be OVERWRITTEN");
	zassert_equal(zfence_result(&g_fence, 2), ZFENCE_RESULT_OVERWRITTEN,
		"seq 2 must be OVERWRITTEN");
	zassert_equal(zfence_result(&g_fence, 3), ZFENCE_RESULT_OK,
		"seq 3 must return ZFENCE_RESULT_OK");
	zassert_equal(zfence_result(&g_fence, 4), ZFENCE_RESULT_UNSIGNALED,
		"seq 4 must be UNSIGNALED");
}

ZTEST(zfence_suite, test_result_null_fence)
{
	int r = zfence_result(NULL, 1);

	zassert_equal(r, -EINVAL, "NULL fence must return -EINVAL");
}

ZTEST(zfence_suite, test_result_seq_zero_initial)
{
	/*
	 * In initial state signal_seq=0 and result=ZFENCE_RESULT_UNSIGNALED.
	 * zfence_result(fence, 0) must return ZFENCE_RESULT_UNSIGNALED because
	 * seq==cur_seq and the stored result is UNSIGNALED.
	 */
	zassert_equal(zfence_result(&g_fence, 0), ZFENCE_RESULT_UNSIGNALED,
		"seq 0 in initial state must return UNSIGNALED");
}

ZTEST(zfence_suite, test_result_seq_zero_after_signal)
{
	/* After any signal, seq=0 is overwritten (signal_seq > 0). */
	zfence_next(&g_fence);
	zfence_signal(&g_fence, ZFENCE_RESULT_OK);

	zassert_equal(zfence_result(&g_fence, 0), ZFENCE_RESULT_OVERWRITTEN,
		"seq 0 must be OVERWRITTEN after first signal");
}

ZTEST(zfence_suite, test_result_custom_error_code)
{
	/* zfence_result must return the exact error code passed to zfence_signal. */
	zfence_next(&g_fence);
	zfence_signal(&g_fence, -EIO);

	zassert_equal(zfence_result(&g_fence, 1), -EIO,
		"zfence_result must return the custom error code -EIO");
}

/* Test 6: zfence_current */

ZTEST(zfence_suite, test_current_initial)
{
	zassert_equal(zfence_current(&g_fence), 0ULL,
		"current must be 0 before any signal");
}

ZTEST(zfence_suite, test_current_after_signal)
{
	zfence_next(&g_fence);
	zfence_signal(&g_fence, ZFENCE_RESULT_OK);

	zassert_equal(zfence_current(&g_fence), 1ULL,
		"current must be 1 after first signal");
}

ZTEST(zfence_suite, test_current_null_fence)
{
	zassert_equal(zfence_current(NULL), (uint64_t)-EINVAL,
		"NULL fence must return (uint64_t)-EINVAL");
}

/* Test 7: Wait context allocation / deallocation */

ZTEST(zfence_suite, test_wait_ctx_alloc_all_slots)
{
	/* Zero-initialize so waiter=0 (invalid sentinel) for every element.
	 * An uninitialized value in [1..32] would be treated as already-allocated
	 * by zfence_ctx_is_valid, causing false uniqueness failures.
	 */
	zfence_wait_context_t ctxs[ZFENCE_MAX_SYNC_WAITERS] = {0};
	int ret;

	/* Allocate all 32 slots; each must succeed with a unique bit. */
	for (int i = 0; i < (int)ZFENCE_MAX_SYNC_WAITERS; i++) {
		ret = zfence_wait_ctx_init(&ctxs[i], &g_fence);
		zassert_equal(ret, 0, "ctx init %d must succeed", i);
		zassert_true(ctxs[i].waiter >= 1U &&
			ctxs[i].waiter <= (uint8_t)ZFENCE_MAX_SYNC_WAITERS,
			"waiter bit must be in valid range [1..%d] for ctx %d",
			ZFENCE_MAX_SYNC_WAITERS, i);
	}

	/* Verify uniqueness of allocated bits. */
	for (int i = 0; i < (int)ZFENCE_MAX_SYNC_WAITERS; i++) {
		for (int j = i + 1; j < (int)ZFENCE_MAX_SYNC_WAITERS; j++) {
			zassert_not_equal(ctxs[i].waiter, ctxs[j].waiter,
				"bits %d and %d must be unique", i, j);
		}
	}

	/* 33rd allocation must fail. */
	zfence_wait_context_t extra = {0};

	ret = zfence_wait_ctx_init(&extra, &g_fence);
	zassert_equal(ret, -EINVAL, "33rd ctx init must fail with -EINVAL");

	/* Release all slots. */
	for (int i = 0; i < (int)ZFENCE_MAX_SYNC_WAITERS; i++) {
		ret = zfence_wait_ctx_deinit(&ctxs[i]);
		zassert_equal(ret, 0, "ctx deinit %d must succeed", i);
	}

	/* After release, allocation must succeed again. */
	ret = zfence_wait_ctx_init(&ctxs[0], &g_fence);
	zassert_equal(ret, 0, "ctx init after full deinit must succeed");
	zfence_wait_ctx_deinit(&ctxs[0]);
}

ZTEST(zfence_suite, test_wait_ctx_deinit_twice)
{
	zfence_wait_context_t ctx = {0};

	zfence_wait_ctx_init(&ctx, &g_fence);
	zassert_equal(zfence_wait_ctx_deinit(&ctx), 0, "first deinit must succeed");
	/* Second deinit on an already-released context is a safe no-op. */
	zassert_equal(zfence_wait_ctx_deinit(&ctx), 0, "second deinit must be a no-op returning 0");
}

ZTEST(zfence_suite, test_wait_ctx_null)
{
	zassert_equal(zfence_wait_ctx_init(NULL, &g_fence), -EINVAL,
		"NULL ctx init must return -EINVAL");
	zassert_equal(zfence_wait_ctx_deinit(NULL), -EINVAL,
		"NULL ctx deinit must return -EINVAL");
}

ZTEST(zfence_suite, test_wait_ctx_init_idempotent)
{
	zfence_wait_context_t ctx = {0};
	uint8_t first_waiter;

	zassert_equal(zfence_wait_ctx_init(&ctx, &g_fence), 0, "first init must succeed");
	first_waiter = ctx.waiter;

	/* Second init with the same fence must be a no-op. */
	zassert_equal(zfence_wait_ctx_init(&ctx, &g_fence), 0,
		"second init with same fence must return 0");
	zassert_equal(ctx.waiter, first_waiter, "waiter must be unchanged after idempotent init");

	zfence_wait_ctx_deinit(&ctx);
}

#ifndef CONFIG_ZFENCE_PER_FENCE_WAIT_CTX
ZTEST(zfence_suite, test_wait_ctx_init_null_fence_global_mode)
{
	/*
	 * In global mode the fence argument is not used for pool selection.
	 * zfence_wait_ctx_init(ctx, NULL) must allocate a slot immediately
	 * (unlike per-fence mode where it defers to the first zfence_wait call).
	 */
	zfence_wait_context_t ctx = {0};

	zassert_equal(zfence_wait_ctx_init(&ctx, NULL), 0,
	"init with NULL fence must succeed in global mode");
	zassert_true(ctx.waiter >= 1U && ctx.waiter <= (uint8_t)ZFENCE_MAX_SYNC_WAITERS,
				 "slot must be allocated immediately in global mode");

	zfence_wait_ctx_deinit(&ctx);
}

ZTEST(zfence_suite, test_global_mode_ctx_cross_fence)
{
	/* In global mode the pool is not fence-specific.  A context initialized
	 * for fence_a can be used directly with fence_b - no rebinding occurs
	 * because the fence argument is ignored for pool selection.
	 */
	static ZFENCE_DEFINE(fence_a);
	static ZFENCE_DEFINE(fence_b);
	zfence_wait_context_t ctx = {0};
	int result = -1;

	zfence_init(&fence_a);
	zfence_init(&fence_b);

	/* Initialize ctx against fence_a. */
	zassert_equal(zfence_wait_ctx_init(&ctx, &fence_a), 0, "ctx init for fence_a must succeed");

	/* Pre-signal fence_b so the wait returns immediately. */
	zfence_next(&fence_b);
	zfence_signal(&fence_b, ZFENCE_RESULT_OK);

	/* Use ctx with fence_b - must succeed without re-allocation. */
	int ret = zfence_wait(&fence_b, &ctx, 1, K_NO_WAIT, &result);

	zassert_equal(ret, 0, "cross-fence wait must succeed in global mode");
	zassert_equal(result, ZFENCE_RESULT_OK, "result must be OK");

	zfence_wait_ctx_deinit(&ctx);
}
#endif

ZTEST(zfence_suite, test_sync_wait_pool_exhausted)
{
	/*
	 * Exhaust the wait context pool, then call zfence_wait with a fresh
	 * context.  zfence_wait must return -EINVAL because no slot is available.
	 * All slots are released before asserting so the pool is always restored
	 * to a clean state regardless of whether the assertion passes.
	 */
	zfence_wait_context_t ctxs[ZFENCE_MAX_SYNC_WAITERS] = {0};
	zfence_wait_context_t fresh_ctx = {0};
	int ret;

	for (int i = 0; i < (int)ZFENCE_MAX_SYNC_WAITERS; i++) {
		zassert_equal(zfence_wait_ctx_init(&ctxs[i], &g_fence), 0,
			"ctx %d init must succeed", i);
	}

	zfence_next(&g_fence);
	zfence_signal(&g_fence, ZFENCE_RESULT_OK);

	/* Pool is full; zfence_wait with a fresh context must return -EINVAL. */
	ret = zfence_wait(&g_fence, &fresh_ctx, 1, K_NO_WAIT, NULL);

	/* Release all slots BEFORE asserting to keep the pool clean. */
	for (int i = 0; i < (int)ZFENCE_MAX_SYNC_WAITERS; i++) {
		zfence_wait_ctx_deinit(&ctxs[i]);
	}

	zassert_equal(ret, -EINVAL, "zfence_wait with exhausted pool must return -EINVAL");
}

/* Test 8: Synchronous wait - fast-path (already signaled) */

ZTEST(zfence_suite, test_sync_wait_fast_path_current)
{
	zfence_wait_context_t ctx = {0};
	int result = -1;

	zfence_next(&g_fence);
	zfence_signal(&g_fence, ZFENCE_RESULT_OK);

	zassert_equal(zfence_wait_ctx_init(&ctx, &g_fence), 0, "ctx init must succeed");

	/* seq 1 is already signaled - must return immediately. */
	int ret = zfence_wait(&g_fence, &ctx, 1, K_NO_WAIT, &result);

	zassert_equal(ret, 0, "fast-path wait must return 0");
	zassert_equal(result, ZFENCE_RESULT_OK, "result must be ZFENCE_RESULT_OK");

	zfence_wait_ctx_deinit(&ctx);
}

ZTEST(zfence_suite, test_sync_wait_fast_path_error_result)
{
	zfence_wait_context_t ctx = {0};
	int result = 0;

	zfence_next(&g_fence);
	zfence_signal(&g_fence, -EIO);

	zassert_equal(zfence_wait_ctx_init(&ctx, &g_fence), 0, "ctx init must succeed");

	int ret = zfence_wait(&g_fence, &ctx, 1, K_NO_WAIT, &result);

	zassert_equal(ret, 0, "fast-path wait with error result must return 0");
	zassert_equal(result, -EIO, "error result must be propagated");

	zfence_wait_ctx_deinit(&ctx);
}

ZTEST(zfence_suite, test_sync_wait_ctx_reuse)
{
	zfence_wait_context_t ctx = {0};
	int result = -1;

	zassert_equal(zfence_wait_ctx_init(&ctx, &g_fence), 0, "ctx init must succeed");

	/* First wait: seq 1. */
	zfence_next(&g_fence);
	zfence_signal(&g_fence, ZFENCE_RESULT_OK);
	zassert_equal(zfence_wait(&g_fence, &ctx, 1, K_NO_WAIT, &result), 0,
		"first wait must succeed");
	zassert_equal(result, ZFENCE_RESULT_OK, "first result must be OK");

	/* Second wait: seq 2 - same context, no re-init needed. */
	result = -1;
	zfence_next(&g_fence);
	zfence_signal(&g_fence, -EIO);
	zassert_equal(zfence_wait(&g_fence, &ctx, 2, K_NO_WAIT, &result), 0,
		"second wait must succeed");
	zassert_equal(result, -EIO, "second result must be -EIO");

	zfence_wait_ctx_deinit(&ctx);
}

ZTEST(zfence_suite, test_sync_wait_deinitialized_ctx_reinit)
{
	zfence_wait_context_t ctx = {0};
	int result = -1;

	/* Init, use, then deinit the context. */
	zfence_wait_ctx_init(&ctx, &g_fence);
	zfence_next(&g_fence);
	zfence_signal(&g_fence, ZFENCE_RESULT_OK);
	zfence_wait(&g_fence, &ctx, 1, K_NO_WAIT, NULL);
	zfence_wait_ctx_deinit(&ctx);

	/* ctx is now deinitialized (waiter=0).  zfence_wait must auto-reinit. */
	zfence_next(&g_fence);
	zfence_signal(&g_fence, -EIO);

	int ret = zfence_wait(&g_fence, &ctx, 2, K_NO_WAIT, &result);

	zassert_equal(ret, 0, "wait with deinitialized ctx must auto-reinit and succeed");
	zassert_equal(result, -EIO, "result must be -EIO");

	zfence_wait_ctx_deinit(&ctx);
}

ZTEST(zfence_suite, test_sync_wait_fast_path_overwritten)
{
	zfence_wait_context_t ctx = {0};
	int result = -1;

	/* Signal seq 3 (jump-ahead); seq 1 is overwritten. */
	for (int i = 0; i < 3; i++) {
		zfence_next(&g_fence);
	}
	zfence_signal_seq(&g_fence, 3, ZFENCE_RESULT_OK);

	zassert_equal(zfence_wait_ctx_init(&ctx, &g_fence), 0, "ctx init must succeed");

	int ret = zfence_wait(&g_fence, &ctx, 1, K_NO_WAIT, &result);

	zassert_equal(ret, 0, "wait for overwritten seq must return 0 immediately");
	zassert_equal(result, ZFENCE_RESULT_OVERWRITTEN, "result must be OVERWRITTEN");

	zfence_wait_ctx_deinit(&ctx);
}

ZTEST(zfence_suite, test_sync_wait_null_result_ptr)
{
	zfence_wait_context_t ctx = {0};

	zfence_next(&g_fence);
	zfence_signal(&g_fence, ZFENCE_RESULT_OK);

	zassert_equal(zfence_wait_ctx_init(&ctx, &g_fence), 0, "ctx init must succeed");

	/* result pointer is optional (NULL is valid). */
	int ret = zfence_wait(&g_fence, &ctx, 1, K_NO_WAIT, NULL);

	zassert_equal(ret, 0, "wait with NULL result ptr must return 0");

	zfence_wait_ctx_deinit(&ctx);
}

ZTEST(zfence_suite, test_sync_wait_seq_zero)
{
	/*
	 * Waiting for seq=0 after any signal must return immediately with
	 * ZFENCE_RESULT_OVERWRITTEN (signal_seq > 0, so seq=0 < cur_seq).
	 */
	zfence_wait_context_t ctx = {0};
	int result = -1;

	zassert_equal(zfence_wait_ctx_init(&ctx, &g_fence), 0,
	"ctx init must succeed");

	zfence_next(&g_fence);
	zfence_signal(&g_fence, ZFENCE_RESULT_OK);

	int ret = zfence_wait(&g_fence, &ctx, 0, K_NO_WAIT, &result);

	zassert_equal(ret, 0, "wait for seq=0 after signal must return 0");
	zassert_equal(result, ZFENCE_RESULT_OVERWRITTEN,
		"seq=0 must be OVERWRITTEN after any signal");

	zfence_wait_ctx_deinit(&ctx);
}

/* Test 9: Synchronous wait - timeout */

ZTEST(zfence_suite, test_sync_wait_timeout)
{
	zfence_wait_context_t ctx = {0};

	zfence_next(&g_fence); /* reserve seq 1 but never signal */

	zassert_equal(zfence_wait_ctx_init(&ctx, &g_fence), 0, "ctx init must succeed");

	int ret = zfence_wait(&g_fence, &ctx, 1, K_MSEC(20), NULL);

	zassert_equal(ret, -ETIMEDOUT, "wait without signal must return -ETIMEDOUT");

	zfence_wait_ctx_deinit(&ctx);
}

ZTEST(zfence_suite, test_sync_wait_no_wait_unsignaled)
{
	zfence_wait_context_t ctx = {0};

	zfence_next(&g_fence); /* reserve seq 1 but never signal */

	zassert_equal(zfence_wait_ctx_init(&ctx, &g_fence), 0, "ctx init must succeed");

	/* K_NO_WAIT on an unsignaled fence must return -ETIMEDOUT immediately. */
	int ret = zfence_wait(&g_fence, &ctx, 1, K_NO_WAIT, NULL);

	zassert_equal(ret, -ETIMEDOUT, "K_NO_WAIT on unsignaled fence must return -ETIMEDOUT");

	zfence_wait_ctx_deinit(&ctx);
}

ZTEST(zfence_suite, test_sync_wait_timeout_result_unchanged)
{
	/* When zfence_wait returns -ETIMEDOUT, the result pointer must not
	 * be written.  Initialize result to a sentinel and verify it is
	 * unchanged after a timeout.
	 */
	zfence_wait_context_t ctx = {0};
	int result = 0xDEAD;

	zfence_next(&g_fence); /* reserve seq 1 but never signal */

	zassert_equal(zfence_wait_ctx_init(&ctx, &g_fence), 0, "ctx init must succeed");

	int ret = zfence_wait(&g_fence, &ctx, 1, K_NO_WAIT, &result);

	zassert_equal(ret, -ETIMEDOUT, "wait without signal must return -ETIMEDOUT");
	zassert_equal(result, 0xDEAD, "result must be unchanged after timeout");

	zfence_wait_ctx_deinit(&ctx);
}

/* Test 10: Synchronous wait - blocking, woken by zfence_signal */

ZTEST(zfence_suite, test_sync_wait_blocking_signal)
{
	zfence_next(&g_fence); /* reserve seq 1 */

	zassert_equal(zfence_wait_ctx_init(&g_wait_args.ctx, &g_fence), 0, "ctx init must succeed");

	g_wait_args.fence   = &g_fence;
	g_wait_args.seq     = 1;
	g_wait_args.timeout = K_MSEC(500);
	g_wait_args.ret     = -1;
	g_wait_args.result  = -1;

	k_thread_create(&wait_thread_data, wait_thread_stack, WAIT_THREAD_STACK_SIZE,
		wait_thread_fn, &g_wait_args, NULL, NULL, WAIT_THREAD_PRIORITY, 0, K_NO_WAIT);

	/* Give the thread time to reach the blocking wait. */
	k_sleep(K_MSEC(20));

	/* Signal seq 1 - must wake the waiting thread. */
	zassert_equal(zfence_signal(&g_fence, ZFENCE_RESULT_OK), 0, "signal must succeed");

	k_thread_join(&wait_thread_data, K_MSEC(600));

	zassert_equal(g_wait_args.ret, 0, "blocking wait must return 0 after signal");
	zassert_equal(g_wait_args.result, ZFENCE_RESULT_OK, "result must be ZFENCE_RESULT_OK");

	zfence_wait_ctx_deinit(&g_wait_args.ctx);
}

ZTEST(zfence_suite, test_sync_wait_blocking_signal_seq)
{
	/* Reserve 3 values; signal via jump-ahead to seq 3. */
	for (int i = 0; i < 3; i++) {
		zfence_next(&g_fence);
	}

	zassert_equal(zfence_wait_ctx_init(&g_wait_args.ctx, &g_fence), 0,
	"ctx init must succeed");

	g_wait_args.fence   = &g_fence;
	g_wait_args.seq     = 3;
	g_wait_args.timeout = K_MSEC(500);
	g_wait_args.ret     = -1;
	g_wait_args.result  = -1;

	k_thread_create(&wait_thread_data, wait_thread_stack, WAIT_THREAD_STACK_SIZE,
		wait_thread_fn, &g_wait_args, NULL, NULL, WAIT_THREAD_PRIORITY, 0, K_NO_WAIT);

	k_sleep(K_MSEC(20));

	zassert_equal(zfence_signal_seq(&g_fence, 3, -EIO), 0, "signal_seq must succeed");

	k_thread_join(&wait_thread_data, K_MSEC(600));

	zassert_equal(g_wait_args.ret, 0, "blocking wait must return 0 after signal_seq");
	zassert_equal(g_wait_args.result, -EIO, "result must be -EIO");

	zfence_wait_ctx_deinit(&g_wait_args.ctx);
}

ZTEST(zfence_suite, test_sync_wait_forever)
{
	/*
	 * K_FOREVER blocks indefinitely until signaled.  Verify that the
	 * sys_timepoint_calc / sys_timepoint_timeout path handles K_FOREVER
	 * correctly and the waiter is woken by a normal signal.
	 */
	zfence_next(&g_fence);

	zassert_equal(zfence_wait_ctx_init(&g_wait_args.ctx, &g_fence), 0, "ctx init must succeed");

	g_wait_args.fence   = &g_fence;
	g_wait_args.seq     = 1;
	g_wait_args.timeout = K_FOREVER;
	g_wait_args.ret     = -1;
	g_wait_args.result  = -1;

	k_thread_create(&wait_thread_data, wait_thread_stack, WAIT_THREAD_STACK_SIZE,
		wait_thread_fn, &g_wait_args, NULL, NULL, WAIT_THREAD_PRIORITY, 0, K_NO_WAIT);

	k_sleep(K_MSEC(20));

	zassert_equal(zfence_signal(&g_fence, ZFENCE_RESULT_OK), 0, "signal must succeed");

	k_thread_join(&wait_thread_data, K_MSEC(600));

	zassert_equal(g_wait_args.ret, 0, "K_FOREVER wait must return 0 after signal");
	zassert_equal(g_wait_args.result, ZFENCE_RESULT_OK, "result must be ZFENCE_RESULT_OK");

	zfence_wait_ctx_deinit(&g_wait_args.ctx);
}

ZTEST(zfence_suite, test_sync_wait_intermediate_signals)
{
	/*
	 * A thread waits for seq 3.  The main thread signals seq 1, then seq 2,
	 * then seq 3.  The wait loop must handle the spurious wakeups for seq 1
	 * and seq 2 and return only after seq 3 is signaled.
	 */
	for (int i = 0; i < 3; i++) {
		zfence_next(&g_fence);
	}

	zassert_equal(zfence_wait_ctx_init(&g_wait_args.ctx, &g_fence), 0, "ctx init must succeed");

	g_wait_args.fence   = &g_fence;
	g_wait_args.seq     = 3;
	g_wait_args.timeout = K_MSEC(1000);
	g_wait_args.ret     = -1;
	g_wait_args.result  = -1;

	k_thread_create(&wait_thread_data, wait_thread_stack, WAIT_THREAD_STACK_SIZE,
		wait_thread_fn, &g_wait_args, NULL, NULL, WAIT_THREAD_PRIORITY, 0, K_NO_WAIT);

	k_sleep(K_MSEC(20));

	/* Signal seq 1 and seq 2 - thread must not return yet. */
	zassert_equal(zfence_signal(&g_fence, ZFENCE_RESULT_OK), 0, "signal seq 1 must succeed");
	k_sleep(K_MSEC(10));
	zassert_equal(zfence_signal(&g_fence, ZFENCE_RESULT_OK), 0, "signal seq 2 must succeed");
	k_sleep(K_MSEC(10));

	/* Thread must still be waiting (seq 3 not yet signaled). */
	zassert_equal(g_wait_args.ret, -1, "thread must still be waiting before seq 3 is signaled");

	/* Signal seq 3 - thread must now return. */
	zassert_equal(zfence_signal(&g_fence, -EIO), 0, "signal seq 3 must succeed");

	k_thread_join(&wait_thread_data, K_MSEC(600));

	zassert_equal(g_wait_args.ret, 0, "wait must return 0 after seq 3 is signaled");
	zassert_equal(g_wait_args.result, -EIO, "result must be -EIO (from seq 3 signal)");

	zfence_wait_ctx_deinit(&g_wait_args.ctx);
}

ZTEST(zfence_suite, test_sync_wait_concurrent_waiters)
{
	/*
	 * Two threads both waiting on seq 1.  A single zfence_signal() must
	 * wake both.
	 */
	zfence_next(&g_fence);

	zassert_equal(zfence_wait_ctx_init(&g_wait_args.ctx, &g_fence), 0,
		"ctx1 init must succeed");
	zassert_equal(zfence_wait_ctx_init(&g_wait_args2.ctx, &g_fence), 0,
		"ctx2 init must succeed");

	g_wait_args.fence   = &g_fence;
	g_wait_args.seq     = 1;
	g_wait_args.timeout = K_MSEC(500);
	g_wait_args.ret     = -1;
	g_wait_args.result  = -1;

	g_wait_args2.fence   = &g_fence;
	g_wait_args2.seq     = 1;
	g_wait_args2.timeout = K_MSEC(500);
	g_wait_args2.ret     = -1;
	g_wait_args2.result  = -1;

	k_thread_create(&wait_thread_data, wait_thread_stack, WAIT_THREAD_STACK_SIZE,
		wait_thread_fn, &g_wait_args, NULL, NULL, WAIT_THREAD_PRIORITY, 0, K_NO_WAIT);

	k_thread_create(&wait_thread2_data, wait_thread2_stack, WAIT_THREAD_STACK_SIZE,
		wait_thread_fn, &g_wait_args2, NULL, NULL, WAIT_THREAD_PRIORITY, 0, K_NO_WAIT);

	/* Give both threads time to reach the blocking wait. */
	k_sleep(K_MSEC(20));

	/* One signal must wake both waiters. */
	zassert_equal(zfence_signal(&g_fence, ZFENCE_RESULT_OK), 0, "signal must succeed");

	k_thread_join(&wait_thread_data,  K_MSEC(600));
	k_thread_join(&wait_thread2_data, K_MSEC(600));

	zassert_equal(g_wait_args.ret, 0, "thread 1 must return 0 after signal");
	zassert_equal(g_wait_args.result, ZFENCE_RESULT_OK, "thread 1 result must be OK");
	zassert_equal(g_wait_args2.ret, 0, "thread 2 must return 0 after signal");
	zassert_equal(g_wait_args2.result, ZFENCE_RESULT_OK, "thread 2 result must be OK");

	zfence_wait_ctx_deinit(&g_wait_args.ctx);
	zfence_wait_ctx_deinit(&g_wait_args2.ctx);
}

/* Test 11: Synchronous wait - invalid arguments */

ZTEST(zfence_suite, test_sync_wait_invalid_args)
{
	zfence_wait_context_t ctx = {0};

	zfence_wait_ctx_init(&ctx, &g_fence);

	/* NULL fence */
	zassert_equal(zfence_wait(NULL, &ctx, 1, K_NO_WAIT, NULL), -EINVAL,
		"NULL fence must return -EINVAL");

	/* NULL ctx */
	zassert_equal(zfence_wait(&g_fence, NULL, 1, K_NO_WAIT, NULL), -EINVAL,
		"NULL ctx must return -EINVAL");

	zfence_wait_ctx_deinit(&ctx);
}

/* Tests 12-14: Per-fence wait context pool (CONFIG_ZFENCE_PER_FENCE_WAIT_CTX) */

#ifdef CONFIG_ZFENCE_PER_FENCE_WAIT_CTX

/*
 * Test 12: Two fences each independently serve bit 0.
 *
 * In global mode allocating two contexts would give bits 0 and 1.
 * In per-fence mode each fence has its own pool so both can allocate bit 0.
 */
ZTEST(zfence_suite, test_per_fence_independent_pools)
{
	static ZFENCE_DEFINE(fence_a);
	static ZFENCE_DEFINE(fence_b);
	zfence_wait_context_t ctx_a = {0};
	zfence_wait_context_t ctx_b = {0};

	zfence_init(&fence_a);
	zfence_init(&fence_b);

	zassert_equal(zfence_wait_ctx_init(&ctx_a, &fence_a), 0, "ctx_a init must succeed");
	zassert_equal(zfence_wait_ctx_init(&ctx_b, &fence_b), 0, "ctx_b init must succeed");

	/*
	 * Both contexts should have received the first available bit from their
	 * respective pools.  With 1-indexed storage, bit position 0 is stored
	 * as waiter value 1.
	 */
	zassert_equal(ctx_a.waiter, 1U, "ctx_a must hold waiter value 1 (bit 0)");
	zassert_equal(ctx_b.waiter, 1U, "ctx_b must hold waiter value 1 (bit 0)");

	zfence_wait_ctx_deinit(&ctx_a);
	zfence_wait_ctx_deinit(&ctx_b);
}

/*
 * Test 13: Context initialised for fence A then used with fence B via
 * zfence_wait() - the implementation must transparently re-allocate.
 */
ZTEST(zfence_suite, test_per_fence_realloc_on_fence_change)
{
	static ZFENCE_DEFINE(fence_a);
	static ZFENCE_DEFINE(fence_b);
	zfence_wait_context_t ctx = {0};
	int result = -1;

	zfence_init(&fence_a);
	zfence_init(&fence_b);

	/* Init ctx against fence_a - allocates bit 0 (stored as waiter value 1). */
	zassert_equal(zfence_wait_ctx_init(&ctx, &fence_a), 0, "ctx init for fence_a must succeed");
	zassert_equal(ctx.waiter, 1U, "initial waiter must be 1 (bit 0, 1-indexed)");
	zassert_equal_ptr(ctx.fence, &fence_a, "ctx fence must be fence_a");

	/* Pre-signal fence_b so zfence_wait returns immediately. */
	zfence_next(&fence_b);
	zfence_signal(&fence_b, ZFENCE_RESULT_OK);

	/*
	 * Call zfence_wait with fence_b - the implementation must free the bit
	 * from fence_a's pool and allocate a new bit from fence_b's pool.
	 */
	int ret = zfence_wait(&fence_b, &ctx, 1, K_NO_WAIT, &result);

	zassert_equal(ret, 0, "zfence_wait must succeed after re-alloc");
	zassert_equal(result, ZFENCE_RESULT_OK, "result must be OK");
	zassert_equal_ptr(ctx.fence, &fence_b,
		"ctx fence must have been updated to fence_b");

	/* fence_a's pool must have had its bit released (bit 0 is free again). */
	zfence_wait_context_t tmp = {0};

	zassert_equal(zfence_wait_ctx_init(&tmp, &fence_a), 0,
		"fence_a must have a free slot after ctx migrated away");
	zassert_equal(tmp.waiter, 1U,
		"fence_a bit 0 must be available again (stored as waiter value 1)");

	zfence_wait_ctx_deinit(&tmp);
	zfence_wait_ctx_deinit(&ctx);
}

/*
 * Test 14: Context initialised with NULL fence; first zfence_wait call
 * performs the deferred allocation.
 */
ZTEST(zfence_suite, test_per_fence_deferred_alloc)
{
	/* Zero-initialize so waiter=0 (invalid sentinel) before the first call.
	 * An uninitialized value in [1..32] would make zfence_ctx_is_valid return
	 * true for the NULL-fence path, skipping the deferred-init logic.
	 */
	zfence_wait_context_t ctx = {0};
	int result = -1;

	/* Init with NULL fence - waiter stays 0 (invalid) until zfence_wait. */
	zassert_equal(zfence_wait_ctx_init(&ctx, NULL), 0, "ctx init with NULL fence must succeed");
	zassert_equal(ctx.waiter, 0U, "waiter must be 0 (invalid) before first wait");

	/* Pre-signal g_fence so the fast-path fires immediately. */
	zfence_next(&g_fence);
	zfence_signal(&g_fence, ZFENCE_RESULT_OK);

	int ret = zfence_wait(&g_fence, &ctx, 1, K_NO_WAIT, &result);

	zassert_equal(ret, 0, "deferred-alloc wait must succeed");
	zassert_equal(result, ZFENCE_RESULT_OK, "result must be OK");
	zassert_true(ctx.waiter >= 1U && ctx.waiter <= (uint8_t)ZFENCE_MAX_SYNC_WAITERS,
		"waiter must be in valid range [1..%d] after zfence_wait",
		ZFENCE_MAX_SYNC_WAITERS);
	zassert_equal_ptr(ctx.fence, &g_fence, "ctx fence must be set to g_fence after wait");

	zfence_wait_ctx_deinit(&ctx);

	zassert_equal(ctx.waiter, 0U, "waiter must be 0 (invalid) after deinit");
	zassert_is_null(ctx.fence, "fence must be NULL after deinit");
}

/*
 * Test 15: Exhausting one fence's pool does not affect another fence's pool.
 */
ZTEST(zfence_suite, test_per_fence_pool_exhaustion_isolated)
{
	static ZFENCE_DEFINE(fence_a);
	static ZFENCE_DEFINE(fence_b);
	zfence_wait_context_t ctxs_a[ZFENCE_MAX_SYNC_WAITERS] = {0};
	zfence_wait_context_t ctx_b = {0};

	zfence_init(&fence_a);
	zfence_init(&fence_b);

	/* Exhaust fence_a's pool. */
	for (int i = 0; i < (int)ZFENCE_MAX_SYNC_WAITERS; i++) {
		zassert_equal(zfence_wait_ctx_init(&ctxs_a[i], &fence_a), 0,
			"fence_a ctx %d init must succeed", i);
	}

	/* fence_a is full; fence_b must still have a free slot. */
	zassert_equal(zfence_wait_ctx_init(&ctx_b, &fence_b), 0,
		"fence_b ctx init must succeed even when fence_a is full");

	/* Release all. */
	for (int i = 0; i < (int)ZFENCE_MAX_SYNC_WAITERS; i++) {
		zfence_wait_ctx_deinit(&ctxs_a[i]);
	}
	zfence_wait_ctx_deinit(&ctx_b);
}

/*
 * Test 16: zfence_wait_ctx_init is idempotent in per-fence mode.
 */
ZTEST(zfence_suite, test_per_fence_wait_ctx_init_idempotent)
{
	zfence_wait_context_t ctx = {0};
	uint8_t first_waiter;

	zassert_equal(zfence_wait_ctx_init(&ctx, &g_fence), 0,
		"first init must succeed");
	first_waiter = ctx.waiter;

	/* Second init with the same fence must be a no-op. */
	zassert_equal(zfence_wait_ctx_init(&ctx, &g_fence), 0,
		"second init with same fence must return 0");
	zassert_equal(ctx.waiter, first_waiter, "waiter must be unchanged after idempotent init");
	zassert_equal_ptr(ctx.fence, &g_fence, "fence pointer must be unchanged");

	zfence_wait_ctx_deinit(&ctx);
}

/*
 * Test 17: zfence_wait_ctx_init(ctx, NULL) on an already-initialized context
 * is a no-op - the existing binding is preserved.
 */
ZTEST(zfence_suite, test_per_fence_null_fence_on_initialized_ctx_noop)
{
	zfence_wait_context_t ctx = {0};
	uint8_t first_waiter;

	zassert_equal(zfence_wait_ctx_init(&ctx, &g_fence), 0,
		"init with fence must succeed");
	first_waiter = ctx.waiter;

	/* Calling with NULL fence on an initialized context must be a no-op. */
	zassert_equal(zfence_wait_ctx_init(&ctx, NULL), 0,
		"init with NULL fence on initialized ctx must return 0");
	zassert_equal(ctx.waiter, first_waiter,
		"waiter must be unchanged after NULL-fence no-op");
	zassert_equal_ptr(ctx.fence, &g_fence,
		"fence pointer must be preserved after NULL-fence no-op");

	zfence_wait_ctx_deinit(&ctx);
}

#endif /* CONFIG_ZFENCE_PER_FENCE_WAIT_CTX */

/* Stress tests */

ZTEST(zfence_suite, test_rapid_signal_wait_cycles)
{
	/*
	 * 100 rapid reserve->signal->wait fast-path cycles.  Stresses the event
	 * bit management and the timeout deadline tracking across many iterations.
	 */
	zfence_wait_context_t ctx = {0};
	int result;

	zassert_equal(zfence_wait_ctx_init(&ctx, &g_fence), 0,
		"ctx init must succeed");

	for (int i = 0; i < 100; i++) {
		result = -1;
		uint64_t seq = zfence_next(&g_fence);

		zassert_equal(zfence_signal(&g_fence, ZFENCE_RESULT_OK), 0,
			"signal %d must succeed", i);

		int ret = zfence_wait(&g_fence, &ctx, seq, K_NO_WAIT, &result);

		zassert_equal(ret, 0, "wait %d must succeed", i);
		zassert_equal(result, ZFENCE_RESULT_OK, "result %d must be OK", i);
	}

	zfence_wait_ctx_deinit(&ctx);
}

ZTEST(zfence_suite, test_concurrent_producers)
{
	/*
	 * Two threads both calling zfence_signal() simultaneously.
	 * The lock-free CAS loop must ensure both succeed and signal_seq
	 * advances by exactly 2.
	 */
	zfence_next(&g_fence);
	zfence_next(&g_fence);

	g_signal_ret1 = -1;
	g_signal_ret2 = -1;

	k_thread_create(&signal_thread1_data, signal_thread_stack1, WAIT_THREAD_STACK_SIZE,
		signal_thread_fn, &g_fence, &g_signal_ret1, NULL, WAIT_THREAD_PRIORITY,
		0, K_NO_WAIT);

	k_thread_create(&signal_thread2_data, signal_thread_stack2, WAIT_THREAD_STACK_SIZE,
		signal_thread_fn, &g_fence, &g_signal_ret2, NULL, WAIT_THREAD_PRIORITY,
		0, K_NO_WAIT);

	k_thread_join(&signal_thread1_data, K_MSEC(500));
	k_thread_join(&signal_thread2_data, K_MSEC(500));

	zassert_equal(g_signal_ret1, 0, "producer 1 must succeed");
	zassert_equal(g_signal_ret2, 0, "producer 2 must succeed");
	zassert_equal(zfence_current(&g_fence), 2ULL,
		"signal_seq must be 2 after two concurrent signals");
}

ZTEST(zfence_suite, test_concurrent_next)
{
	/*
	 * Two threads both calling zfence_next() simultaneously.
	 * The lock-free CAS loop must ensure both get unique, valid sequence
	 * values and reserved_seq advances by exactly 2.
	 */
	g_next_seq1 = 0;
	g_next_seq2 = 0;

	k_thread_create(&next_thread1_data, next_thread_stack1, WAIT_THREAD_STACK_SIZE,
		next_thread_fn, &g_fence, &g_next_seq1, NULL, WAIT_THREAD_PRIORITY, 0, K_NO_WAIT);

	k_thread_create(&next_thread2_data, next_thread_stack2, WAIT_THREAD_STACK_SIZE,
		next_thread_fn, &g_fence, &g_next_seq2, NULL, WAIT_THREAD_PRIORITY, 0, K_NO_WAIT);

	k_thread_join(&next_thread1_data, K_MSEC(500));
	k_thread_join(&next_thread2_data, K_MSEC(500));

	zassert_true(g_next_seq1 >= 1ULL && g_next_seq1 <= 2ULL, "seq1 must be 1 or 2");
	zassert_true(g_next_seq2 >= 1ULL && g_next_seq2 <= 2ULL, "seq2 must be 1 or 2");
	zassert_not_equal(g_next_seq1, g_next_seq2, "seq1 and seq2 must be unique");
	zassert_equal((long)atomic_get(&g_fence.reserved_seq), 2L,
		"reserved_seq must be 2 after two concurrent reservations");
}

/* Suite definition */

ZTEST_SUITE(zfence_suite, NULL, NULL, zfence_test_before, NULL, NULL);
