/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/ztest.h>
#if defined(CONFIG_TRACING_OBJECT_TRACKING)
#include <zephyr/tracing/tracking.h>
#endif

struct ctx {
	struct k_sem done;
	int status;
};

/* Permanent semaphore, used to check that tracking still works. */
static struct k_sem permanent_sem;

static int count_op(struct k_obj_core *obj_core, void *data)
{
	size_t *count = data;

	ARG_UNUSED(obj_core);
	(*count)++;

	return 0;
}

/* Walks the whole semaphore type list, which also proves it is intact. */
static size_t obj_core_sem_count(void)
{
	struct k_obj_type *type = k_obj_type_find(K_OBJ_TYPE_SEM_ID);
	size_t count = 0;

	if (type != NULL) {
		k_obj_type_walk_locked(type, count_op, &count);
	}

	return count;
}

static size_t track_sem_count(void)
{
	size_t count = 0;

#if defined(CONFIG_TRACING_OBJECT_TRACKING)
	for (struct k_sem *sem = _track_list_k_sem; sem != NULL;
	     sem = SYS_PORT_TRACK_NEXT(sem)) {
		count++;
	}
#endif

	return count;
}

/* Brings up transient semaphores and lets them go out of scope. */
static void use_transient_sems(void)
{
	struct k_sem sem = K_SEM_INITIALIZER(sem, 0, 1);
	struct ctx ctx = {
		.done = K_SEM_INITIALIZER(ctx.done, 0, 1),
	};

	k_sem_give(&sem);
	zassert_ok(k_sem_take(&sem, K_NO_WAIT));

	k_sem_give(&ctx.done);
	zassert_ok(k_sem_take(&ctx.done, K_NO_WAIT));
}

ZTEST(sem_transient, test_sem_type_is_registered)
{
	zassert_not_null(k_obj_type_find(K_OBJ_TYPE_SEM_ID),
			 "semaphore object type not registered");
}

ZTEST(sem_transient, test_initializer_behaves_like_k_sem_init)
{
	struct k_sem sem = K_SEM_INITIALIZER(sem, 1, 2);

	zassert_equal(k_sem_count_get(&sem), 1);

	zassert_ok(k_sem_take(&sem, K_NO_WAIT));
	zassert_equal(k_sem_count_get(&sem), 0);
	zassert_equal(k_sem_take(&sem, K_NO_WAIT), -EBUSY);

	/* The count must saturate at the limit. */
	k_sem_give(&sem);
	k_sem_give(&sem);
	k_sem_give(&sem);
	zassert_equal(k_sem_count_get(&sem), 2);

	k_sem_reset(&sem);
	zassert_equal(k_sem_count_get(&sem), 0);
}

static void giver(void *p1, void *p2, void *p3)
{
	struct k_sem *sem = p1;

	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	k_sem_give(sem);
}

K_THREAD_STACK_DEFINE(giver_stack, 1024 + CONFIG_TEST_EXTRA_STACK_SIZE);
static struct k_thread giver_thread;

ZTEST(sem_transient, test_initializer_wait_q_works)
{
	struct k_sem sem = K_SEM_INITIALIZER(sem, 0, 1);

	k_thread_create(&giver_thread, giver_stack, K_THREAD_STACK_SIZEOF(giver_stack), giver, &sem,
			NULL, NULL, K_PRIO_PREEMPT(0), 0, K_NO_WAIT);

	zassert_ok(k_sem_take(&sem, K_FOREVER));
	zassert_ok(k_thread_join(&giver_thread, K_FOREVER));
}

ZTEST(sem_transient, test_transient_sems_are_not_tracked)
{
	size_t obj_core_before = obj_core_sem_count();
	size_t track_before = track_sem_count();

	for (int i = 0; i < 4; i++) {
		use_transient_sems();
	}

	zassert_equal(obj_core_sem_count(), obj_core_before,
		      "transient semaphores entered the object core list");
	zassert_equal(track_sem_count(), track_before,
		      "transient semaphores entered the object tracking list");
}

ZTEST(sem_transient, test_permanent_sems_are_still_tracked)
{
	size_t obj_core_before = obj_core_sem_count();
	size_t track_before = track_sem_count();

	zassert_ok(k_sem_init(&permanent_sem, 0, 1));

	zassert_equal(obj_core_sem_count(), obj_core_before + 1,
		      "k_sem_init() did not link the semaphore");

	if (IS_ENABLED(CONFIG_TRACING_OBJECT_TRACKING)) {
		zassert_equal(track_sem_count(), track_before + 1,
			      "k_sem_init() did not track the semaphore");
	}
}

ZTEST_SUITE(sem_transient, NULL, NULL, NULL, NULL, NULL);
