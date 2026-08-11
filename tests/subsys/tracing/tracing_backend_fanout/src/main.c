/*
 * Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/ztest.h>
#include <zephyr/sys/atomic.h>
#include <tracing_backend.h>

/*
 * A second tracing backend registered the same way an out-of-tree module would,
 * with TRACING_BACKEND_DEFINE(). The core fans every trace packet out to all
 * registered backends (here the in-tree RAM backend plus this one), so this
 * backend must observe data with no edits to the Zephyr tree.
 */
static atomic_t fanout_bytes;

static void test_backend_output(const struct tracing_backend *backend,
				uint8_t *data, uint32_t length)
{
	ARG_UNUSED(backend);
	ARG_UNUSED(data);
	atomic_add(&fanout_bytes, (atomic_val_t)length);
}

static const struct tracing_backend_api test_backend_api = {
	.output = test_backend_output,
};

TRACING_BACKEND_DEFINE(test_fanout_backend, test_backend_api);

K_SEM_DEFINE(fanout_sem, 0, 1);

ZTEST(tracing_backend_fanout, test_second_backend_receives)
{
	atomic_set(&fanout_bytes, 0);

	/* Generate trace traffic the CTF format will emit through every backend. */
	for (int i = 0; i < 50; i++) {
		k_sem_give(&fanout_sem);
		(void)k_sem_take(&fanout_sem, K_NO_WAIT);
	}

	zassert_true(atomic_get(&fanout_bytes) > 0,
		     "fan-out backend received no tracing data");
}

ZTEST_SUITE(tracing_backend_fanout, NULL, NULL, NULL, NULL, NULL);
