/*
 * Copyright (c) 2023 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/timing/timing.h>
#include <zephyr/sys/spsc_lockfree.h>

/*
 * @brief Produce and Consume a single uint32_t in the same execution context
 *
 * @see spsc_acquire(), spsc_produce(), spsc_consume(), spsc_release()
 *
 * @ingroup tests
 */
ZTEST(spsc, test_produce_consume_size1)
{
	SPSC_DEFINE(ezspsc, uint32_t, 1);

	const uint32_t magic = 43219876;

	uint32_t *acq = spsc_acquire(&ezspsc);

	zassert_not_null(acq, "Acquire should succeed");

	*acq = magic;

	uint32_t *acq2 = spsc_acquire(&ezspsc);

	zassert_is_null(acq2, "Acquire should fail");

	uint32_t *cons = spsc_consume(&ezspsc);

	zassert_is_null(cons, "Consume should fail");

	zassert_equal(spsc_consumable(&ezspsc), 0, "Consumables should be 0");

	spsc_produce(&ezspsc);

	zassert_equal(spsc_consumable(&ezspsc), 1, "Consumables should be 1");

	uint32_t *cons2 = spsc_consume(&ezspsc);

	zassert_equal(spsc_consumable(&ezspsc), 0, "Consumables should be 0");

	zassert_not_null(cons2, "Consume should not fail");
	zassert_equal(*cons2, magic, "Consume value should equal magic");

	uint32_t *cons3 = spsc_consume(&ezspsc);

	zassert_is_null(cons3, "Consume should fail");


	uint32_t *acq3 = spsc_acquire(&ezspsc);

	zassert_is_null(acq3, "Acquire should not succeed");

	spsc_release(&ezspsc);

	uint32_t *acq4 = spsc_acquire(&ezspsc);

	zassert_not_null(acq4, "Acquire should succeed");
}

/*&*
 * @brief Produce and Consume 3 items at a time in a spsc of size 4 to validate masking
 * and wrap around reads/writes.
 *
 * @see spsc_acquire(), spsc_produce(), spsc_consume(), spsc_release()
 *
 * @ingroup tests
 */
ZTEST(spsc, test_produce_consume_wrap_around)
{
	SPSC_DEFINE(ezspsc, uint32_t, 4);

	for (int i = 0; i < 10; i++) {
		zassert_equal(spsc_consumable(&ezspsc), 0, "Consumables should be 0");
		for (int j = 0; j < 3; j++) {
			uint32_t *entry = spsc_acquire(&ezspsc);

			zassert_not_null(entry, "Acquire should succeed");
			*entry = i * 3 + j;
			spsc_produce(&ezspsc);
		}
		zassert_equal(spsc_consumable(&ezspsc), 3, "Consumables should be 3");

		for (int k = 0; k < 3; k++) {
			uint32_t *entry = spsc_consume(&ezspsc);

			zassert_not_null(entry, "Consume should succeed");
			zassert_equal(*entry, i * 3 + k, "Consume value should equal i*3+k");
			spsc_release(&ezspsc);
		}

		zassert_equal(spsc_consumable(&ezspsc), 0, "Consumables should be 0");

	}
}

/**
 * @brief Ensure that integer wraps continue to work.
 *
 * Done by setting all values to UINTPTR_MAX - 2 and writing and reading enough
 * to ensure integer wraps occur.
 */
ZTEST(spsc, test_int_wrap_around)
{
	SPSC_DEFINE(ezspsc, uint32_t, 4);
	ezspsc._spsc.in = ATOMIC_INIT(UINTPTR_MAX - 2);
	ezspsc._spsc.out = ATOMIC_INIT(UINTPTR_MAX - 2);

	for (int j = 0; j < 3; j++) {
		uint32_t *entry = spsc_acquire(&ezspsc);

		zassert_not_null(entry, "Acquire should succeed");
		*entry = j;
		spsc_produce(&ezspsc);
	}

	zassert_equal(atomic_get(&ezspsc._spsc.in), UINTPTR_MAX + 1, "Spsc in should wrap");

	for (int k = 0; k < 3; k++) {
		uint32_t *entry = spsc_consume(&ezspsc);

		zassert_not_null(entry, "Consume should succeed");
		zassert_equal(*entry, k, "Consume value should equal i*3+k");
		spsc_release(&ezspsc);
	}

	zassert_equal(atomic_get(&ezspsc._spsc.out), UINTPTR_MAX + 1, "Spsc out should wrap");
}

#define MAX_RETRIES 5
#define SMP_ITERATIONS 100

SPSC_DEFINE(spsc, uint32_t, 4);

static void t1_consume(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	struct spsc_spsc *ezspsc = p1;
	uint32_t retries = 0;
	uint32_t *val = NULL;

	for (int i = 0; i < SMP_ITERATIONS; i++) {
		val = NULL;
		retries = 0;
		while (val == NULL && retries < MAX_RETRIES) {
			val = spsc_consume(ezspsc);
			retries++;
		}
		if (val != NULL) {
			spsc_release(ezspsc);
		} else {
			k_yield();
		}
	}
}

static void t2_produce(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	struct spsc_spsc *ezspsc = p1;
	uint32_t retries = 0;
	uint32_t *val = NULL;

	for (int i = 0; i < SMP_ITERATIONS; i++) {
		val = NULL;
		retries = 0;
		while (val == NULL && retries < MAX_RETRIES) {
			val = spsc_acquire(ezspsc);
			retries++;
		}
		if (val != NULL) {
			*val = SMP_ITERATIONS;
			spsc_produce(ezspsc);
		} else {
			k_yield();
		}
	}
}


#define STACK_SIZE (384 + CONFIG_TEST_EXTRA_STACK_SIZE)
#define THREADS_NUM 2

struct thread_info {
	k_tid_t tid;
	int executed;
	int priority;
	int cpu_id;
};

static struct thread_info tinfo[THREADS_NUM];
static struct k_thread tthread[THREADS_NUM];
static K_THREAD_STACK_ARRAY_DEFINE(tstack, THREADS_NUM, STACK_SIZE);

/**
 * @brief Test that the producer and consumer are indeed thread safe
 *
 * This can and should be validated on SMP machines where incoherent
 * memory could cause issues.
 */
ZTEST(spsc, test_spsc_threaded)
{

	tinfo[0].tid =
		k_thread_create(&tthread[0], tstack[0], STACK_SIZE,
				t1_consume,
				&spsc, NULL, NULL,
				K_PRIO_PREEMPT(5),
				K_INHERIT_PERMS, K_NO_WAIT);
	tinfo[1].tid =
		k_thread_create(&tthread[1], tstack[1], STACK_SIZE,
				t2_produce,
				&spsc, NULL, NULL,
				K_PRIO_PREEMPT(5),
				K_INHERIT_PERMS, K_NO_WAIT);

	k_thread_join(tinfo[1].tid, K_FOREVER);
	k_thread_join(tinfo[0].tid, K_FOREVER);
}

#define THROUGHPUT_ITERS 100000

ZTEST(spsc, test_spsc_throughput)
{
	timing_t start_time, end_time;

	timing_init();
	timing_start();

	start_time = timing_counter_get();

	uint32_t *x, *y;

	int key = irq_lock();

	for (int i = 0; i < THROUGHPUT_ITERS; i++) {
		x = spsc_acquire(&spsc);
		*x = i;
		spsc_produce(&spsc);

		y = spsc_consume(&spsc);
		spsc_release(&spsc);
	}

	irq_unlock(key);

	end_time = timing_counter_get();

	uint64_t cycles = timing_cycles_get(&start_time, &end_time);
	uint64_t ns = timing_cycles_to_ns(cycles);

	TC_PRINT("%llu ns for %d iterations, %llu ns per op\n", ns,
		 THROUGHPUT_ITERS, ns/THROUGHPUT_ITERS);
}

static void spsc_before(void *data)
{
	ARG_UNUSED(data);

	spsc_reset(&spsc);
}

ZTEST_SUITE(spsc, NULL, NULL, spsc_before, NULL, NULL);
