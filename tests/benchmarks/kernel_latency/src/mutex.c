/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Uncontended mutex operations. Splitting lock from unlock is what the
 * setup and teardown hooks are for: neither is timed, so each
 * benchmark reports one operation rather than the pair.
 */

#include "bench.h"

ZTEST_BENCHMARK_SUITE(mutex, NULL, NULL);

static K_MUTEX_DEFINE(mutex);

static void mutex_acquire(void)
{
	(void)k_mutex_lock(&mutex, K_FOREVER);
}

static void mutex_release(void)
{
	(void)k_mutex_unlock(&mutex);
}

ZTEST_BENCHMARK(mutex, lock, BENCH_SAMPLES, NULL, mutex_release)
{
	(void)k_mutex_lock(&mutex, K_FOREVER);
}

ZTEST_BENCHMARK(mutex, unlock, BENCH_SAMPLES, mutex_acquire, NULL)
{
	(void)k_mutex_unlock(&mutex);
}
