/*
 * Copyright (c) 2024 Tenstorrent AI ULC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <pthread.h>
#include <stdio.h>
#include <time.h>

#include <zephyr/ztest.h>
#include <zephyr/kernel.h>

/* Picolibc has a conflicting macro definition with the wrong number of arguments */
#undef putchar_unlocked
int putchar_unlocked(int c);

#define N 2

struct thread_ctx {
	int delay_ms;
	int hold_ms;
	int lock_count;
	FILE *fp;
	bool retry;
	bool success;
};

static size_t fp_idx;
static FILE *fp[N];
static struct thread_ctx ctx[N];

/*
 * > The functions shall behave as if there is a lock count associated with each (FILE *)
 * > object. This count is implicitly initialized to zero when the (FILE *) object is
 * > created.
 *
 * 1. A newly opened FILE object will have negligible delay associated with locking it.
 *
 * > When the count is positive, a single thread owns the (FILE *) object
 *
 * 1. Two threads can call flockfile() on separate files with neglible delay.
 * 2. When two threads call flockfile() simultaneously on the same file, the second thread
 *    to acquire the lock will experience a delay equal to or greater than the amount of
 *    time that the first thread holds the lock.
 *
 * > Each call to funlockfile() shall decrement the count
 *
 * 1. If a thread owns the lock associated with a file, that thread can call funlockfile()
 *    with negligible delay.
 * 2. If a thread has called flockfile() multiple times for the same file, that thread must
 *    call funlockfile() the same number of times to release the lock.
 *
 * > The behavior is undefined if a thread other than the current owner calls the
 * > funlockfile() function.
 *
 * 1. Likely what we'll do in this situation is have a Kconfig option to verify the caller
 *    of funlockfile(). If the option is enabled, we will assert. If the option is not
 *    enabled, enter an infinite loop.
 */

static void *flockfile_thread_entry(void *arg)
{
	struct thread_ctx *const ctx = (struct thread_ctx *)arg;

	if (ctx->delay_ms > 0) {
		zassert_ok(k_msleep(ctx->delay_ms));
	}

	for (int i = 0; i < ctx->lock_count; ++i) {
		flockfile(ctx->fp);
		if (ctx->hold_ms > 0) {
			k_sleep(K_MSEC(ctx->hold_ms));
		}
		funlockfile(ctx->fp);
	}

	return NULL;
}

static int flockfile_retry(FILE *fp, bool retry)
{
	while (true) {
		int ret = ftrylockfile(fp);

		if (!retry) {
			return ret;
		}
		if (ret == 0) {
			return 0;
		}
		k_yield();
	}

	CODE_UNREACHABLE;
}

static void *ftrylockfile_thread_entry(void *arg)
{
	struct thread_ctx *const ctx = (struct thread_ctx *)arg;

	zassert_ok(k_msleep(ctx->delay_ms));

	for (int i = 0; i < ctx->lock_count; ++i) {
		int ret = flockfile_retry(ctx->fp, ctx->retry);

		if (ret == 0) {
			ctx->success = true;
		}

		if (ctx->hold_ms > 0) {
			k_sleep(K_MSEC(ctx->hold_ms));
		}
		funlockfile(ctx->fp);
	}

	return NULL;
}

static void flockfile_common(bool try)
{
	int64_t now, then;
	pthread_t th[2];
	void *(*thread_entry)(void *) = try ? ftrylockfile_thread_entry : flockfile_thread_entry;

	if (false) {
		/* degenerate cases */
		flockfile_retry(NULL, try); /* this will throw an assertion if enabled */
	}

	/*
	 * > The functions shall behave as if there is a lock count associated with each
	 * (FILE *) > object. This count is implicitly initialized to zero when the (FILE *)
	 * object is > created.
	 *
	 * 1. A newly opened FILE object will have negligible delay associated with locking
	 * it, because they are not waiting for another thread to release the lock.
	 *
	 * > When the count is positive, a single thread owns the (FILE *) object
	 *
	 * 1. Two threads can call flockfile() on separate files with neglible delay.
	 * 2. When two threads call flockfile() simultaneously on the same file, the second
	 * thread to acquire the lock will experience a delay equal to or greater than the
	 * amount of time that the first thread holds the lock.
	 */

	/* check for locking in parallel - should be no contention */
	for (int i = 0; i < N; ++i) {
		ctx[i] = (struct thread_ctx){
			.lock_count = 1,
			.hold_ms = 0,
			.fp = fp[i],
			.retry = try,
		};
	}

	then = k_uptime_get();
	for (int i = 0; i < N; ++i) {
		zassert_ok(pthread_create(&th[i], NULL, thread_entry, &ctx[i]));
	}
	k_yield();
	ARRAY_FOR_EACH_PTR(th, it) {
		zassert_ok(pthread_join(*it, NULL));
	}
	now = k_uptime_get();
	zexpect_true(now - then <= CONFIG_TEST_NEGLIGIBLE_DELAY_MS, "delay of %d ms exceeds %d ms",
		     now - then, CONFIG_TEST_NEGLIGIBLE_DELAY_MS);

	/*
	 * Check for locking the same file once - should be contention resulting in a delay
	 * of twice CONFIG_TEST_LOCK_PERIOD_MS.
	 */
	ARRAY_FOR_EACH_PTR(ctx, it) {
		*it = (struct thread_ctx){
			.lock_count = 1,
			.hold_ms = CONFIG_TEST_LOCK_PERIOD_MS,
			.fp = fp[0],
			.retry = try,
		};
	}

	then = k_uptime_get();
	ARRAY_FOR_EACH(th, i) {
		zassert_ok(pthread_create(&th[i], NULL, thread_entry, &ctx[i]));
	}
	k_yield();
	ARRAY_FOR_EACH_PTR(th, it) {
		zassert_ok(pthread_join(*it, NULL));
	}
	now = k_uptime_get();
	zexpect_true(now - then >= 2 * CONFIG_TEST_LOCK_PERIOD_MS, "delay of %d ms less than %d ms",
		     now - then, 2 * CONFIG_TEST_LOCK_PERIOD_MS);

	/*
	 * Check for locking the same file twice - should be contention resulting in a delay
	 * of four times CONFIG_TEST_LOCK_PERIOD_MS.
	 */
	ARRAY_FOR_EACH_PTR(ctx, it) {
		*it = (struct thread_ctx){
			.lock_count = 2,
			.hold_ms = CONFIG_TEST_LOCK_PERIOD_MS,
			.fp = fp[0],
			.retry = try,
		};
	}

	then = k_uptime_get();
	ARRAY_FOR_EACH(ctx, i) {
		zassert_ok(pthread_create(&th[i], NULL, thread_entry, &ctx[i]));
	}
	k_yield();
	ARRAY_FOR_EACH_PTR(th, it) {
		zassert_ok(pthread_join(*it, NULL));
	}
	now = k_uptime_get();
	zexpect_true(now - then >= 4 * CONFIG_TEST_LOCK_PERIOD_MS, "delay of %d ms less than %d ms",
		     now - then, 4 * CONFIG_TEST_LOCK_PERIOD_MS);
}

ZTEST(posix_file_locking, test_flockfile)
{
	flockfile_common(false);
}

ZTEST(posix_file_locking, test_ftrylockfile)
{
	int64_t now, then;
	pthread_t th[2];

	flockfile_common(true);

	/* additional, special cases for ftrylockfile() */
	ARRAY_FOR_EACH_PTR(ctx, it) {
		*it = (struct thread_ctx){
			.lock_count = 1,
			.hold_ms = CONFIG_TEST_LOCK_PERIOD_MS,
			.fp = fp[0],
			.retry = false,
		};
	}

	then = k_uptime_get();
	ARRAY_FOR_EACH(ctx, i) {
		zassert_ok(pthread_create(&th[i], NULL, ftrylockfile_thread_entry, &ctx[i]));
	}
	k_yield();
	ARRAY_FOR_EACH_PTR(th, it) {
		zassert_ok(pthread_join(*it, NULL));
	}
	now = k_uptime_get();
	zexpect_true(now - then >= CONFIG_TEST_LOCK_PERIOD_MS, "delay of %d ms less than %d ms",
		     now - then, CONFIG_TEST_LOCK_PERIOD_MS);

	int success = 0;
	int fail = 0;

	ARRAY_FOR_EACH_PTR(ctx, it) {
		success += it->success;
		fail += !it->success;
	}
	zexpect_true(N > 1);
	if (IS_ENABLED(CONFIG_PICOLIBC)) {
		if (success != 1) {
			TC_PRINT("Note: successes equal to %d\n", success);
		}
		if (fail < 1) {
			TC_PRINT("Note: failures equal %d\n", fail);
		}
	} else {
		zexpect_equal(success, 1);
		zexpect_true(fail >= 1);
	}
}

ZTEST(posix_file_locking, test_funlockfile)
{
	/* this is already excercised in test_flockfile_common(), so just a simple check here */
	zexpect_ok(ftrylockfile(fp[0]));
	funlockfile(fp[0]);
}

ZTEST(posix_file_locking, test_getc_unlocked)
{
	flockfile(fp[0]);
	zassert_equal(EOF, getc_unlocked(fp[0]));
	funlockfile(fp[0]);

	static const char msg[] = "Hello";
	int expect = strlen(msg);
	int actual = fwrite(msg, 1, expect, fp[0]);

	zassert_equal(actual, expect, "wrote %d bytes, expected %d", actual, expect);
	rewind(fp[0]);

	flockfile(fp[0]);
	ARRAY_FOR_EACH(msg, i) {
		if (msg[i] == '\0') {
			break;
		}

		int actual = getc_unlocked(fp[0]);

		zassert_equal((int)msg[i], actual, "expected %c, got %c", msg[i], actual);
	}
	funlockfile(fp[0]);
}

ZTEST(posix_file_locking, test_getchar_unlocked)
{
	flockfile(stdin);
	zassert_equal(EOF, getchar_unlocked());
	funlockfile(stdin);
}

ZTEST(posix_file_locking, test_putc_unlocked)
{
	flockfile(fp[0]);
	zassert_equal('*', putc_unlocked('*', fp[0]));
	funlockfile(fp[0]);
}

ZTEST(posix_file_locking, test_putchar_unlocked)
{
	flockfile(stdout);
	zassert_equal('*', putchar_unlocked('*'));
	funlockfile(stdout);
}

void *setup(void);
void before(void *arg);
void after(void *arg);
void teardown(void *arg);

void setup_callback(void *arg)
{
	ARG_UNUSED(arg);

	fp_idx = 0;
}

void before_callback(void *arg)
{
	FILE *file = arg;

	zassert_true(fp_idx < ARRAY_SIZE(fp), "fps[] overflow");
	fp[fp_idx++] = file;
}

void after_callback(void *arg)
{
	ARG_UNUSED(arg);

	fp_idx = 0;
}

ZTEST_SUITE(posix_file_locking, NULL, setup, before, after, teardown);
