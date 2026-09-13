/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Basalte bv
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/tracing/tracking.h>

#ifdef CONFIG_OBJ_CORE_SEM
struct obj_core_count_data {
	/* Object core to count occurrences of */
	struct k_obj_core *target;
	/* Number of occurrences found */
	size_t count;
};

static int obj_core_count_op(struct k_obj_core *obj_core, void *data)
{
	struct obj_core_count_data *count_data = data;

	if (obj_core == count_data->target) {
		count_data->count++;
	}

	return 0;
}

static size_t obj_core_count(struct k_sem *sem)
{
	struct k_obj_type *obj_type = k_obj_type_find(K_OBJ_TYPE_SEM_ID);
	struct obj_core_count_data count_data = {
		.target = K_OBJ_CORE(sem),
		.count = 0,
	};

	zassert_not_null(obj_type, "semaphore object type not found");
	k_obj_type_walk_locked(obj_type, obj_core_count_op, &count_data);

	return count_data.count;
}
#endif /* CONFIG_OBJ_CORE_SEM */

#ifdef CONFIG_TRACING_OBJECT_TRACKING
static bool track_list_contains(struct k_sem *sem)
{
	for (struct k_sem *cur = _track_list_k_sem; cur != NULL;
	     cur = SYS_PORT_TRACK_NEXT(cur)) {
		if (cur == sem) {
			return true;
		}
	}

	return false;
}
#endif /* CONFIG_TRACING_OBJECT_TRACKING */

static void check_tracked(struct k_sem *sem, bool tracked)
{
#ifdef CONFIG_OBJ_CORE_SEM
	zassert_equal(obj_core_count(sem), tracked ? 1 : 0,
		      "unexpected semaphore object core link state");
#endif /* CONFIG_OBJ_CORE_SEM */

#ifdef CONFIG_TRACING_OBJECT_TRACKING
	zassert_equal(track_list_contains(sem), tracked,
		      "unexpected semaphore object tracking state");
#endif /* CONFIG_TRACING_OBJECT_TRACKING */

	ARG_UNUSED(sem);
	ARG_UNUSED(tracked);
}

ZTEST(sem_deinit, test_sem_deinit)
{
	struct k_sem sem;

	zassert_ok(k_sem_init(&sem, 0, 1));
	check_tracked(&sem, true);

	k_sem_give(&sem);
	zassert_ok(k_sem_take(&sem, K_NO_WAIT));

	/* Deinitializing releases the semaphore from the tracking
	 * facilities; its memory (here, this stack frame) may be reused
	 * afterwards.
	 */
	k_sem_deinit(&sem);
	check_tracked(&sem, false);

	/* The semaphore can be initialized and used again */
	zassert_ok(k_sem_init(&sem, 1, 1));
	check_tracked(&sem, true);

	zassert_ok(k_sem_take(&sem, K_NO_WAIT));

	k_sem_deinit(&sem);
	check_tracked(&sem, false);
}

static struct k_sem pending_sem;
static struct k_thread pending_thread;
static K_THREAD_STACK_DEFINE(pending_stack, 1024 + CONFIG_TEST_EXTRA_STACK_SIZE);

static void pending_entry(void *p1, void *p2, void *p3)
{
	int *ret = p1;

	*ret = k_sem_take(&pending_sem, K_FOREVER);
}

ZTEST(sem_deinit, test_sem_deinit_waiters)
{
	int take_ret = 0xb4d;

	zassert_ok(k_sem_init(&pending_sem, 0, 1));

	k_thread_create(&pending_thread, pending_stack,
			K_THREAD_STACK_SIZEOF(pending_stack), pending_entry,
			&take_ret, NULL, NULL,
			k_thread_priority_get(k_current_get()) - 1, 0,
			K_NO_WAIT);

	/* Wait until the thread is pending on the semaphore */
	k_msleep(50);

	/* Deinitializing releases waiters as if the semaphore was reset */
	k_sem_deinit(&pending_sem);
	check_tracked(&pending_sem, false);

	zassert_ok(k_thread_join(&pending_thread, K_SECONDS(1)));
	zassert_equal(take_ret, -EAGAIN, "waiter not released with -EAGAIN");
}

static struct k_sem user_sem;

ZTEST_USER(sem_deinit, test_sem_deinit_user)
{
	zassert_ok(k_sem_init(&user_sem, 0, 1));

	k_sem_give(&user_sem);
	zassert_ok(k_sem_take(&user_sem, K_NO_WAIT));

	k_sem_deinit(&user_sem);

	/* The semaphore can be initialized and used again */
	zassert_ok(k_sem_init(&user_sem, 1, 1));
	zassert_ok(k_sem_take(&user_sem, K_NO_WAIT));

	k_sem_deinit(&user_sem);
}

static void *sem_deinit_setup(void)
{
#ifdef CONFIG_USERSPACE
	k_thread_access_grant(k_current_get(), &user_sem);
#endif /* CONFIG_USERSPACE */

	return NULL;
}

ZTEST_SUITE(sem_deinit, NULL, sem_deinit_setup, NULL, NULL, NULL);
