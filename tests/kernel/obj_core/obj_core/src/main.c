/*
 * Copyright (c) 2023, Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>
#include <zephyr/ztest.h>
#include <zephyr/sys/mem_blocks.h>

SYS_MEM_BLOCKS_DEFINE(block1, 32, 4, 16);

K_MEM_SLAB_DEFINE(slab1, 32, 4, 16);
static char slab2_buffer[256] __aligned(8);
struct k_mem_slab slab2;

K_TIMER_DEFINE(timer1, NULL, NULL);
struct k_timer timer2;

K_STACK_DEFINE(stack1, 8);
static struct k_stack stack2;
stack_data_t stack2_buffer[8];

static K_FIFO_DEFINE(fifo1);
static struct k_fifo fifo2;

static K_LIFO_DEFINE(lifo1);
static struct k_lifo lifo2;

K_PIPE_DEFINE(pipe1, 16, 8);
static struct k_pipe pipe2;
static uint8_t pipe2_buffer[16];

K_MSGQ_DEFINE(msgq1, 16, 4, 8);
static struct k_msgq msgq2;
static char msgq2_buffer[16];

static K_MBOX_DEFINE(mbox1);
static struct k_mbox mbox2;

static K_CONDVAR_DEFINE(condvar1);
static struct k_condvar condvar2;

static K_EVENT_DEFINE(event1);
static struct k_event event2;

static K_MUTEX_DEFINE(mutex1);
static struct k_mutex mutex2;

static K_SEM_DEFINE(sem1, 0, 1);
static struct k_sem sem2;

static void thread_entry(void *, void *, void *);
K_THREAD_DEFINE(thread1, 512 + CONFIG_TEST_EXTRA_STACK_SIZE,
		thread_entry, NULL, NULL, NULL,
		K_HIGHEST_THREAD_PRIO, 0, 0);
static struct k_thread thread2;
K_THREAD_STACK_DEFINE(thread2_stack, 512 + CONFIG_TEST_EXTRA_STACK_SIZE);

struct obj_core_find_data {
	struct k_obj_core *obj_core;    /* Object core to search for */
	int count;                      /* Number of times it was reported */
};

static void thread_entry(void *p1, void *p2, void *p3)
{
	k_sem_take(&sem1, K_FOREVER);
}

static int obj_core_find_op(struct k_obj_core *obj_core, void *data)
{
	struct obj_core_find_data *find_data = data;

	if (find_data->obj_core == obj_core) {

		/* Object core found. Abort the search. */

		return 1;
	}

	/* Object core not found--continue searching. */

	return 0;
}

static void common_obj_core_test(uint32_t type_id, const char *str,
				 struct k_obj_core *static_obj_core,
				 struct k_obj_core *dyn_obj_core)
{
	struct k_obj_type  *obj_type;
	struct obj_core_find_data  walk_data;
	int  status;

	obj_type = k_obj_type_find(type_id);

	zassert_not_null(obj_type, "%s object type not found\n", str);

	/* Search for statically initialized object */

	if (static_obj_core != NULL) {
		walk_data.obj_core = static_obj_core;
		status = k_obj_type_walk_locked(obj_type, obj_core_find_op,
						&walk_data);
		zassert_equal(status, 1,
			      "static %s not found with locked walk\n", str);

		status = k_obj_type_walk_unlocked(obj_type, obj_core_find_op,
						  &walk_data);
		zassert_equal(status, 1,
			      "static %s not found with unlocked walk\n", str);
	}

	/* Search for dynamically initialized object */

	if (dyn_obj_core != NULL) {
		walk_data.obj_core = dyn_obj_core;
		status = k_obj_type_walk_locked(obj_type, obj_core_find_op,
						&walk_data);
		zassert_equal(status, 1,
			      "dynamic %s not found with locked walk\n", str);

		status = k_obj_type_walk_unlocked(obj_type, obj_core_find_op,
						  &walk_data);
		zassert_equal(status, 1,
			      "dynamic %s not found with unlocked walk\n", str);
	}
}

ZTEST(obj_core, test_obj_core_thread)
{
	k_thread_create(&thread2, thread2_stack,
			K_THREAD_STACK_SIZEOF(thread2_stack), thread_entry,
			NULL, NULL, NULL, K_HIGHEST_THREAD_PRIO, 0, K_NO_WAIT);

	common_obj_core_test(K_OBJ_TYPE_THREAD_ID, "thread",
			     K_OBJ_CORE(thread1), K_OBJ_CORE(&thread2));

	/* Terminate both thread1 and thread 2 */

	k_thread_abort(thread1);
	k_thread_abort(&thread2);

	/*
	 * Neither thread1 nor thread2 should be in the thread object type's
	 * list of threads anymore. Verify this.
	 */

	struct k_obj_type  *obj_type;
	struct obj_core_find_data  walk_data;
	int  status;

	obj_type = k_obj_type_find(K_OBJ_TYPE_THREAD_ID);

	zassert_not_null(obj_type, "thread object type not found\n");

	/* Search for statically initialized object */

	walk_data.obj_core = K_OBJ_CORE(thread1);
	status = k_obj_type_walk_locked(obj_type, obj_core_find_op,
					&walk_data);
	zassert_equal(status, 0, "static thread found with locked walk\n");

	status = k_obj_type_walk_unlocked(obj_type, obj_core_find_op,
					  &walk_data);
	zassert_equal(status, 0, "static thread found with unlocked walk\n");

	walk_data.obj_core = K_OBJ_CORE(&thread2);
	status = k_obj_type_walk_locked(obj_type, obj_core_find_op,
					&walk_data);
	zassert_equal(status, 0, "dynamic thread found with locked walk\n");

	status = k_obj_type_walk_unlocked(obj_type, obj_core_find_op,
					  &walk_data);
	zassert_equal(status, 0, "dynamic thread found with unlocked walk\n");
}

ZTEST(obj_core, test_obj_core_system)
{
	int  i;
	char cpu_str[16];

	/*
	 * Use the existing object cores in the _cpu and z_kerenl structures
	 * as we should not be creating new ones.
	 */

	for (i = 0; i < CONFIG_MP_MAX_NUM_CPUS; i++) {
		sprintf(cpu_str, "CPU%d", i);
		common_obj_core_test(K_OBJ_TYPE_CPU_ID, cpu_str,
				     K_OBJ_CORE(&_kernel.cpus[i]), NULL);
	}

	common_obj_core_test(K_OBJ_TYPE_KERNEL_ID, "_kernel",
			     K_OBJ_CORE(&_kernel), NULL);
}

ZTEST(obj_core, test_obj_core_sys_mem_block)
{
	common_obj_core_test(K_OBJ_TYPE_MEM_BLOCK_ID, "memory block",
			     K_OBJ_CORE(&block1), NULL);
}

ZTEST(obj_core, test_obj_core_mem_slab)
{
	k_mem_slab_init(&slab2, slab2_buffer, 32, 8);
	common_obj_core_test(K_OBJ_TYPE_MEM_SLAB_ID, "memory slab",
			     K_OBJ_CORE(&slab1), K_OBJ_CORE(&slab2));
}

ZTEST(obj_core, test_obj_core_timer)
{
	k_timer_init(&timer2, NULL, NULL);
	common_obj_core_test(K_OBJ_TYPE_TIMER_ID, "timer",
			     K_OBJ_CORE(&timer1), K_OBJ_CORE(&timer2));
}

ZTEST(obj_core, test_obj_core_stack)
{
	k_stack_init(&stack2, stack2_buffer, 8);
	common_obj_core_test(K_OBJ_TYPE_STACK_ID, "stack",
			     K_OBJ_CORE(&stack1), K_OBJ_CORE(&stack2));
}

ZTEST(obj_core, test_obj_core_fifo)
{
	k_fifo_init(&fifo2);
	common_obj_core_test(K_OBJ_TYPE_FIFO_ID, "FIFO",
			     K_OBJ_CORE(&fifo1), K_OBJ_CORE(&fifo2));
}

ZTEST(obj_core, test_obj_core_lifo)
{
	k_lifo_init(&lifo2);
	common_obj_core_test(K_OBJ_TYPE_LIFO_ID, "LIFO",
			     K_OBJ_CORE(&lifo1), K_OBJ_CORE(&lifo2));
}

ZTEST(obj_core, test_obj_core_pipe)
{
	k_pipe_init(&pipe2, pipe2_buffer, sizeof(pipe2_buffer));
	common_obj_core_test(K_OBJ_TYPE_PIPE_ID, "pipe",
			     K_OBJ_CORE(&pipe1), K_OBJ_CORE(&pipe2));
}

ZTEST(obj_core, test_obj_core_msgq)
{
	k_msgq_init(&msgq2, msgq2_buffer, 4, 4);
	common_obj_core_test(K_OBJ_TYPE_MSGQ_ID, "message queue",
			     K_OBJ_CORE(&msgq1), K_OBJ_CORE(&msgq2));
}

ZTEST(obj_core, test_obj_core_mbox)
{
	k_mbox_init(&mbox2);
	common_obj_core_test(K_OBJ_TYPE_MBOX_ID, "mailbox",
			     K_OBJ_CORE(&mbox1), K_OBJ_CORE(&mbox2));
}

ZTEST(obj_core, test_obj_core_condvar)
{
	k_condvar_init(&condvar2);
	common_obj_core_test(K_OBJ_TYPE_CONDVAR_ID, "condition variable",
			     K_OBJ_CORE(&condvar1), K_OBJ_CORE(&condvar2));
}

ZTEST(obj_core, test_obj_core_event)
{
	k_event_init(&event2);
	common_obj_core_test(K_OBJ_TYPE_EVENT_ID, "event",
			     K_OBJ_CORE(&event1), K_OBJ_CORE(&event2));
}

ZTEST(obj_core, test_obj_core_mutex)
{
	k_mutex_init(&mutex2);
	common_obj_core_test(K_OBJ_TYPE_MUTEX_ID, "mutex",
			     K_OBJ_CORE(&mutex1), K_OBJ_CORE(&mutex2));
}

ZTEST(obj_core, test_obj_core_sem)
{
	k_sem_init(&sem2, 0, 1);

	common_obj_core_test(K_OBJ_TYPE_SEM_ID, "semaphore",
			     K_OBJ_CORE(&sem1), K_OBJ_CORE(&sem2));
}

static int obj_core_count_op(struct k_obj_core *obj_core, void *data)
{
	struct obj_core_find_data *find_data = data;

	if (find_data->obj_core == obj_core) {
		find_data->count++;
	}

	return 0;
}

static int count_walk(uint32_t type_id, struct k_obj_core *obj_core)
{
	struct obj_core_find_data walk_data = { .obj_core = obj_core, .count = 0 };

	k_obj_type_walk_locked(k_obj_type_find(type_id), obj_core_count_op, &walk_data);

	return walk_data.count;
}

ZTEST(obj_core, test_obj_core_reinit)
{
	/* Re-initializing a static object and a registered object leaves each
	 * reported exactly once, with the other objects still present.
	 */
	k_mutex_init(&mutex1);
	k_mutex_init(&mutex2);
	k_mutex_init(&mutex2);

	common_obj_core_test(K_OBJ_TYPE_MUTEX_ID, "mutex",
			     K_OBJ_CORE(&mutex1), K_OBJ_CORE(&mutex2));
	zassert_equal(count_walk(K_OBJ_TYPE_MUTEX_ID, K_OBJ_CORE(&mutex1)), 1);
	zassert_equal(count_walk(K_OBJ_TYPE_MUTEX_ID, K_OBJ_CORE(&mutex2)), 1);
}

ZTEST(obj_core, test_obj_core_unlink)
{
	k_mutex_init(&mutex2);
	zassert_equal(count_walk(K_OBJ_TYPE_MUTEX_ID, K_OBJ_CORE(&mutex2)), 1);

	k_obj_core_unlink(K_OBJ_CORE(&mutex2));
	k_obj_core_unlink(K_OBJ_CORE(&mutex2));
	zassert_equal(count_walk(K_OBJ_TYPE_MUTEX_ID, K_OBJ_CORE(&mutex2)), 0);
	zassert_equal(count_walk(K_OBJ_TYPE_MUTEX_ID, K_OBJ_CORE(&mutex1)), 1);

	k_obj_core_link(K_OBJ_CORE(&mutex2));
	zassert_equal(count_walk(K_OBJ_TYPE_MUTEX_ID, K_OBJ_CORE(&mutex2)), 1);
}

static union {
	struct k_sem sem;
	struct k_mutex mutex;
} reused_storage;

ZTEST(obj_core, test_obj_core_storage_reuse)
{
	/* An object discarded without being unregistered leaves a stale entry
	 * that is dropped once its storage holds something else.
	 */
	k_sem_init(&reused_storage.sem, 0, 1);
	zassert_equal(count_walk(K_OBJ_TYPE_SEM_ID, K_OBJ_CORE(&reused_storage.sem)), 1);

	memset(&reused_storage, 0, sizeof(reused_storage));
	k_mutex_init(&reused_storage.mutex);

	zassert_equal(count_walk(K_OBJ_TYPE_SEM_ID, K_OBJ_CORE(&reused_storage.sem)), 0);
	zassert_equal(count_walk(K_OBJ_TYPE_MUTEX_ID, K_OBJ_CORE(&reused_storage.mutex)), 1);
	zassert_equal(count_walk(K_OBJ_TYPE_SEM_ID, K_OBJ_CORE(&sem1)), 1);

	k_obj_core_unlink(K_OBJ_CORE(&reused_storage.mutex));
}

static struct k_sem fill_sems[CONFIG_OBJ_CORE_MAX_DYNAMIC_OBJECTS];
static struct k_sem extra_sem;

ZTEST(obj_core, test_obj_core_registry_full)
{
	struct k_obj_type *type = k_obj_type_find(K_OBJ_TYPE_SEM_ID);
	uint32_t dropped = type->dropped;

	/* The registry already holds the kernel's own objects, so filling it
	 * with as many semaphores as it has entries overflows it.
	 */
	for (size_t i = 0; i < ARRAY_SIZE(fill_sems); i++) {
		k_sem_init(&fill_sems[i], 0, 1);
	}
	zassert_true(type->dropped > dropped, "no registration refused");

	dropped = type->dropped;
	k_sem_init(&extra_sem, 0, 1);
	zassert_equal(type->dropped, dropped + 1);
	zassert_equal(count_walk(K_OBJ_TYPE_SEM_ID, K_OBJ_CORE(&extra_sem)), 0);

	/* The kernel's objects and the static ones are still reported */
	zassert_equal(count_walk(K_OBJ_TYPE_SEM_ID, K_OBJ_CORE(&sem1)), 1);
	zassert_equal(count_walk(K_OBJ_TYPE_THREAD_ID, K_OBJ_CORE(k_current_get())), 1);

	/* Freeing one entry lets the next registration through */
	k_obj_core_unlink(K_OBJ_CORE(&fill_sems[0]));
	k_sem_init(&extra_sem, 0, 1);
	zassert_equal(type->dropped, dropped + 1);
	zassert_equal(count_walk(K_OBJ_TYPE_SEM_ID, K_OBJ_CORE(&extra_sem)), 1);

	k_obj_core_unlink(K_OBJ_CORE(&extra_sem));
	for (size_t i = 1; i < ARRAY_SIZE(fill_sems); i++) {
		k_obj_core_unlink(K_OBJ_CORE(&fill_sems[i]));
	}
}

ZTEST_SUITE(obj_core, NULL, NULL,
	    ztest_simple_1cpu_before, ztest_simple_1cpu_after, NULL);
