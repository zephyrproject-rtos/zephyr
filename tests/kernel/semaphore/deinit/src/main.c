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

ZTEST_SUITE(sem_deinit, NULL, NULL, NULL, NULL, NULL);
