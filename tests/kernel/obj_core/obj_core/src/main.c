/*
 * Copyright (c) 2023, Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>
#include <zephyr/ztest.h>
#include <zephyr/sys/mem_blocks.h>

/**
 * @brief Kernel Object Inspection Tests
 * @defgroup obj_core_tests Kernel Object Inspection
 * @ingroup all_tests
 */

SYS_MEM_BLOCKS_DEFINE(block1, 32, 4, 16);
SYS_MEM_BLOCKS_DEFINE(block2, 128, 2, 8);

K_MEM_SLAB_DEFINE(slab1, 32, 4, 16);
static char slab2_buffer[256] __aligned(8);
struct k_mem_slab slab2;

K_TIMER_DEFINE(timer1, NULL, NULL);
struct k_timer timer2;

K_STACK_DEFINE(stack1, 8);
static struct k_stack stack2;
stack_data_t stack2_buffer[8];

static K_QUEUE_DEFINE(queue1);
static struct k_queue queue2;

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

/**
 * @brief Verify thread objects are tracked and untracked by the object core
 * framework.
 *
 * @details
 * Thread objects are registered with the object core framework when created.
 * This test confirms a registered thread can be located through its object
 * type, and that aborting the thread unregisters it from the framework.
 *
 * Test steps:
 * - Locate the thread object type with k_obj_type_find().
 * - Walk the type with k_obj_type_walk_locked()/_unlocked() and confirm both
 *   the statically and dynamically created threads are found.
 * - Abort both threads.
 * - Walk the type again and confirm neither thread is found.
 *
 * Expected result:
 * - Both threads are found while alive and absent after being aborted.
 *
 * @see k_obj_type_find()
 * @see k_obj_type_walk_locked()
 * @see k_obj_type_walk_unlocked()
 * @verifies ZEP-SRS-35-1
 * @verifies ZEP-SRS-35-2
 * @verifies ZEP-SRS-35-3
 * @verifies ZEP-SRS-35-4
 * @verifies ZEP-SRS-35-8
 * @verifies ZEP-SRS-35-9
 * @verifies ZEP-SRS-35-15
 * @verifies ZEP-SRS-35-16
 */
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

/**
 * @brief Verify per-CPU and kernel objects are registered with the object core
 * framework.
 *
 * @details
 * The per-CPU structures and the global kernel structure are registered with
 * the object core framework at boot. Using the existing object cores (no new
 * ones are created), confirm each CPU object and the kernel object can be
 * found by walking their respective object types.
 *
 * Test steps:
 * - Find the CPU object type and walk it for each CPU's object core.
 * - Find the kernel object type and walk it for the kernel object core.
 *
 * Expected result:
 * - Every CPU object core and the kernel object core are found.
 *
 * @see k_obj_type_find()
 * @see k_obj_type_walk_locked()
 * @see k_obj_type_walk_unlocked()
 * @verifies ZEP-SRS-35-1
 * @verifies ZEP-SRS-35-3
 * @verifies ZEP-SRS-35-4
 */
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

/**
 * @brief Verify system memory block objects are registered with the object core
 * framework.
 *
 * @details
 * A statically defined system memory block is registered with the object core
 * framework. Confirm the memory block object type can be found and that walking
 * it locates the registered object.
 *
 * Test steps:
 * - Find the memory block object type with k_obj_type_find().
 * - Walk the type (locked and unlocked) for the statically defined block.
 *
 * Expected result:
 * - The registered system memory block object core is found.
 *
 * @see k_obj_type_find()
 * @see k_obj_type_walk_locked()
 * @see k_obj_type_walk_unlocked()
 * @verifies ZEP-SRS-35-1
 * @verifies ZEP-SRS-35-3
 * @verifies ZEP-SRS-35-4
 */
ZTEST(obj_core, test_obj_core_sys_mem_block)
{
	common_obj_core_test(K_OBJ_TYPE_MEM_BLOCK_ID, "memory block",
			     K_OBJ_CORE(&block1), NULL);
}

/**
 * @brief Verify memory slab objects are registered with the object core
 * framework.
 *
 * @details
 * Both a statically defined and a dynamically initialized memory slab are
 * registered with the object core framework. Confirm the memory slab object
 * type can be found and that walking it locates both objects.
 *
 * Test steps:
 * - Initialize a memory slab dynamically with k_mem_slab_init().
 * - Find the memory slab object type and walk it (locked and unlocked) for both
 *   the static and dynamic slabs.
 *
 * Expected result:
 * - Both the static and dynamic memory slab object cores are found.
 *
 * @see k_obj_type_find()
 * @see k_obj_type_walk_locked()
 * @see k_obj_type_walk_unlocked()
 * @verifies ZEP-SRS-35-1
 * @verifies ZEP-SRS-35-3
 * @verifies ZEP-SRS-35-4
 */
ZTEST(obj_core, test_obj_core_mem_slab)
{
	k_mem_slab_init(&slab2, slab2_buffer, 32, 8);
	common_obj_core_test(K_OBJ_TYPE_MEM_SLAB_ID, "memory slab",
			     K_OBJ_CORE(&slab1), K_OBJ_CORE(&slab2));
}

/**
 * @brief Verify timer objects are registered with the object core framework.
 *
 * @details
 * Both a statically defined and a dynamically initialized timer are registered
 * with the object core framework. Confirm the timer object type can be found
 * and that walking it locates both objects.
 *
 * Test steps:
 * - Initialize a timer dynamically with k_timer_init().
 * - Find the timer object type and walk it (locked and unlocked) for both the
 *   static and dynamic timers.
 *
 * Expected result:
 * - Both the static and dynamic timer object cores are found.
 *
 * @see k_obj_type_find()
 * @see k_obj_type_walk_locked()
 * @see k_obj_type_walk_unlocked()
 * @verifies ZEP-SRS-35-1
 * @verifies ZEP-SRS-35-3
 * @verifies ZEP-SRS-35-4
 */
ZTEST(obj_core, test_obj_core_timer)
{
	k_timer_init(&timer2, NULL, NULL);
	common_obj_core_test(K_OBJ_TYPE_TIMER_ID, "timer",
			     K_OBJ_CORE(&timer1), K_OBJ_CORE(&timer2));
}

/**
 * @brief Verify stack objects are registered with the object core framework.
 *
 * @details
 * Both a statically defined and a dynamically initialized stack are registered
 * with the object core framework. Confirm the stack object type can be found
 * and that walking it locates both objects.
 *
 * Test steps:
 * - Initialize a stack dynamically with k_stack_init().
 * - Find the stack object type and walk it (locked and unlocked) for both the
 *   static and dynamic stacks.
 *
 * Expected result:
 * - Both the static and dynamic stack object cores are found.
 *
 * @see k_obj_type_find()
 * @see k_obj_type_walk_locked()
 * @see k_obj_type_walk_unlocked()
 * @verifies ZEP-SRS-35-1
 * @verifies ZEP-SRS-35-3
 * @verifies ZEP-SRS-35-4
 */
ZTEST(obj_core, test_obj_core_stack)
{
	k_stack_init(&stack2, stack2_buffer, 8);
	common_obj_core_test(K_OBJ_TYPE_STACK_ID, "stack",
			     K_OBJ_CORE(&stack1), K_OBJ_CORE(&stack2));
}

ZTEST(obj_core, test_obj_core_queue)
{
	k_queue_init(&queue2);
	common_obj_core_test(K_OBJ_TYPE_QUEUE_ID, "queue",
			     K_OBJ_CORE(&queue1), K_OBJ_CORE(&queue2));
}

/**
 * @brief Verify FIFO objects are registered with the object core framework.
 *
 * @details
 * Both a statically defined and a dynamically initialized FIFO are registered
 * with the object core framework. Confirm the FIFO object type can be found
 * and that walking it locates both objects.
 *
 * Test steps:
 * - Initialize a FIFO dynamically with k_fifo_init().
 * - Find the FIFO object type and walk it (locked and unlocked) for both the
 *   static and dynamic FIFOs.
 *
 * Expected result:
 * - Both the static and dynamic FIFO object cores are found.
 *
 * @see k_obj_type_find()
 * @see k_obj_type_walk_locked()
 * @see k_obj_type_walk_unlocked()
 * @verifies ZEP-SRS-35-1
 * @verifies ZEP-SRS-35-3
 * @verifies ZEP-SRS-35-4
 */
ZTEST(obj_core, test_obj_core_fifo)
{
	k_fifo_init(&fifo2);
	common_obj_core_test(K_OBJ_TYPE_FIFO_ID, "FIFO",
			     K_OBJ_CORE(&fifo1), K_OBJ_CORE(&fifo2));

	/* The embedded queues are reported by the queue type as well */
	common_obj_core_test(K_OBJ_TYPE_QUEUE_ID, "FIFO queue",
			     K_OBJ_CORE(&fifo1._queue), K_OBJ_CORE(&fifo2._queue));
}

/**
 * @brief Verify LIFO objects are registered with the object core framework.
 *
 * @details
 * Both a statically defined and a dynamically initialized LIFO are registered
 * with the object core framework. Confirm the LIFO object type can be found
 * and that walking it locates both objects.
 *
 * Test steps:
 * - Initialize a LIFO dynamically with k_lifo_init().
 * - Find the LIFO object type and walk it (locked and unlocked) for both the
 *   static and dynamic LIFOs.
 *
 * Expected result:
 * - Both the static and dynamic LIFO object cores are found.
 *
 * @see k_obj_type_find()
 * @see k_obj_type_walk_locked()
 * @see k_obj_type_walk_unlocked()
 * @verifies ZEP-SRS-35-1
 * @verifies ZEP-SRS-35-3
 * @verifies ZEP-SRS-35-4
 */
ZTEST(obj_core, test_obj_core_lifo)
{
	k_lifo_init(&lifo2);
	common_obj_core_test(K_OBJ_TYPE_LIFO_ID, "LIFO",
			     K_OBJ_CORE(&lifo1), K_OBJ_CORE(&lifo2));
}

/**
 * @brief Verify pipe objects are registered with the object core framework.
 *
 * @details
 * Both a statically defined and a dynamically initialized pipe are registered
 * with the object core framework. Confirm the pipe object type can be found
 * and that walking it locates both objects.
 *
 * Test steps:
 * - Initialize a pipe dynamically with k_pipe_init().
 * - Find the pipe object type and walk it (locked and unlocked) for both the
 *   static and dynamic pipes.
 *
 * Expected result:
 * - Both the static and dynamic pipe object cores are found.
 *
 * @see k_obj_type_find()
 * @see k_obj_type_walk_locked()
 * @see k_obj_type_walk_unlocked()
 * @verifies ZEP-SRS-35-1
 * @verifies ZEP-SRS-35-3
 * @verifies ZEP-SRS-35-4
 */
ZTEST(obj_core, test_obj_core_pipe)
{
	k_pipe_init(&pipe2, pipe2_buffer, sizeof(pipe2_buffer));
	common_obj_core_test(K_OBJ_TYPE_PIPE_ID, "pipe",
			     K_OBJ_CORE(&pipe1), K_OBJ_CORE(&pipe2));
}

/**
 * @brief Verify message queue objects are registered with the object core
 * framework.
 *
 * @details
 * Both a statically defined and a dynamically initialized message queue are
 * registered with the object core framework. Confirm the message queue object
 * type can be found and that walking it locates both objects.
 *
 * Test steps:
 * - Initialize a message queue dynamically with k_msgq_init().
 * - Find the message queue object type and walk it (locked and unlocked) for
 *   both the static and dynamic message queues.
 *
 * Expected result:
 * - Both the static and dynamic message queue object cores are found.
 *
 * @see k_obj_type_find()
 * @see k_obj_type_walk_locked()
 * @see k_obj_type_walk_unlocked()
 * @verifies ZEP-SRS-35-1
 * @verifies ZEP-SRS-35-3
 * @verifies ZEP-SRS-35-4
 */
ZTEST(obj_core, test_obj_core_msgq)
{
	k_msgq_init(&msgq2, msgq2_buffer, 4, 4);
	common_obj_core_test(K_OBJ_TYPE_MSGQ_ID, "message queue",
			     K_OBJ_CORE(&msgq1), K_OBJ_CORE(&msgq2));
}

/**
 * @brief Verify mailbox objects are registered with the object core framework.
 *
 * @details
 * Both a statically defined and a dynamically initialized mailbox are
 * registered with the object core framework. Confirm the mailbox object type
 * can be found and that walking it locates both objects.
 *
 * Test steps:
 * - Initialize a mailbox dynamically with k_mbox_init().
 * - Find the mailbox object type and walk it (locked and unlocked) for both the
 *   static and dynamic mailboxes.
 *
 * Expected result:
 * - Both the static and dynamic mailbox object cores are found.
 *
 * @see k_obj_type_find()
 * @see k_obj_type_walk_locked()
 * @see k_obj_type_walk_unlocked()
 * @verifies ZEP-SRS-35-1
 * @verifies ZEP-SRS-35-3
 * @verifies ZEP-SRS-35-4
 */
ZTEST(obj_core, test_obj_core_mbox)
{
	k_mbox_init(&mbox2);
	common_obj_core_test(K_OBJ_TYPE_MBOX_ID, "mailbox",
			     K_OBJ_CORE(&mbox1), K_OBJ_CORE(&mbox2));
}

/**
 * @brief Verify condition variable objects are registered with the object core
 * framework.
 *
 * @details
 * Both a statically defined and a dynamically initialized condition variable
 * are registered with the object core framework. Confirm the condition variable
 * object type can be found and that walking it locates both objects.
 *
 * Test steps:
 * - Initialize a condition variable dynamically with k_condvar_init().
 * - Find the condition variable object type and walk it (locked and unlocked)
 *   for both the static and dynamic condition variables.
 *
 * Expected result:
 * - Both the static and dynamic condition variable object cores are found.
 *
 * @see k_obj_type_find()
 * @see k_obj_type_walk_locked()
 * @see k_obj_type_walk_unlocked()
 * @verifies ZEP-SRS-35-1
 * @verifies ZEP-SRS-35-3
 * @verifies ZEP-SRS-35-4
 */
ZTEST(obj_core, test_obj_core_condvar)
{
	k_condvar_init(&condvar2);
	common_obj_core_test(K_OBJ_TYPE_CONDVAR_ID, "condition variable",
			     K_OBJ_CORE(&condvar1), K_OBJ_CORE(&condvar2));
}

/**
 * @brief Verify event objects are registered with the object core framework.
 *
 * @details
 * Both a statically defined and a dynamically initialized event are registered
 * with the object core framework. Confirm the event object type can be found
 * and that walking it locates both objects.
 *
 * Test steps:
 * - Initialize an event dynamically with k_event_init().
 * - Find the event object type and walk it (locked and unlocked) for both the
 *   static and dynamic events.
 *
 * Expected result:
 * - Both the static and dynamic event object cores are found.
 *
 * @see k_obj_type_find()
 * @see k_obj_type_walk_locked()
 * @see k_obj_type_walk_unlocked()
 * @verifies ZEP-SRS-35-1
 * @verifies ZEP-SRS-35-3
 * @verifies ZEP-SRS-35-4
 */
ZTEST(obj_core, test_obj_core_event)
{
	k_event_init(&event2);
	common_obj_core_test(K_OBJ_TYPE_EVENT_ID, "event",
			     K_OBJ_CORE(&event1), K_OBJ_CORE(&event2));
}

/**
 * @brief Verify mutex objects are registered with the object core framework.
 *
 * @details
 * Both a statically defined and a dynamically initialized mutex are registered
 * with the object core framework. Confirm the mutex object type can be found
 * and that walking it locates both objects.
 *
 * Test steps:
 * - Initialize a mutex dynamically with k_mutex_init().
 * - Find the mutex object type and walk it (locked and unlocked) for both the
 *   static and dynamic mutexes.
 *
 * Expected result:
 * - Both the static and dynamic mutex object cores are found.
 *
 * @see k_obj_type_find()
 * @see k_obj_type_walk_locked()
 * @see k_obj_type_walk_unlocked()
 * @verifies ZEP-SRS-35-1
 * @verifies ZEP-SRS-35-3
 * @verifies ZEP-SRS-35-4
 */
ZTEST(obj_core, test_obj_core_mutex)
{
	k_mutex_init(&mutex2);
	common_obj_core_test(K_OBJ_TYPE_MUTEX_ID, "mutex",
			     K_OBJ_CORE(&mutex1), K_OBJ_CORE(&mutex2));
}

/**
 * @brief Verify semaphore objects are registered with the object core
 * framework.
 *
 * @details
 * Both a statically defined and a dynamically initialized semaphore are
 * registered with the object core framework. Confirm the semaphore object type
 * can be found and that walking it locates both objects.
 *
 * Test steps:
 * - Initialize a semaphore dynamically with k_sem_init().
 * - Find the semaphore object type and walk it (locked and unlocked) for both
 *   the static and dynamic semaphores.
 *
 * Expected result:
 * - Both the static and dynamic semaphore object cores are found.
 *
 * @see k_obj_type_find()
 * @see k_obj_type_walk_locked()
 * @see k_obj_type_walk_unlocked()
 * @verifies ZEP-SRS-35-1
 * @verifies ZEP-SRS-35-3
 * @verifies ZEP-SRS-35-4
 */
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

/**
 * @brief Verify re-initializing an object leaves it reported exactly once.
 *
 * @ingroup obj_core_tests
 * @verifies ZEP-SRS-35-8
 */
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

/**
 * @brief Verify an unregistered object disappears from iteration and can be registered again.
 *
 * @ingroup obj_core_tests
 * @verifies ZEP-SRS-35-2
 * @verifies ZEP-SRS-35-8
 */
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

/**
 * @brief Verify registrations are refused and counted once the registry is full.
 *
 * @ingroup obj_core_tests
 * @verifies ZEP-SRS-35-11
 */
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

static void init_stack_sem(void)
{
	struct k_sem sem;

	k_sem_init(&sem, 0, 1);
}

static struct k_sem *isr_sem;

static void init_isr_stack_sem(const void *arg)
{
	struct k_sem sem;

	ARG_UNUSED(arg);
	isr_sem = &sem;
	k_sem_init(&sem, 0, 1);
}

static void work_handler(struct k_work *work)
{
	ARG_UNUSED(work);
}

/**
 * @brief Verify objects in thread or interrupt stack storage are not registered.
 *
 * @ingroup obj_core_tests
 * @verifies ZEP-SRS-35-10
 */
ZTEST(obj_core, test_obj_core_stack_storage)
{
	struct k_obj_type *type = k_obj_type_find(K_OBJ_TYPE_SEM_ID);
	uint32_t skipped = type->skipped;
	struct k_sem sem;
	struct k_work work;
	struct k_work_sync sync;

	/* Objects in stack storage are counted, not registered, and leave
	 * the registry intact once their frames are gone.
	 */
	init_stack_sem();
	init_stack_sem();
	k_sem_init(&sem, 0, 1);
	zassert_equal(type->skipped, skipped + 3);
	zassert_equal(count_walk(K_OBJ_TYPE_SEM_ID, K_OBJ_CORE(&sem)), 0);

	/* An object in the interrupt stack is skipped too. Some architectures
	 * run offloaded interrupts on another exception stack, which the
	 * kernel does not recognize: the object is then registered and stays
	 * reported until its storage is reused.
	 */
	irq_offload(init_isr_stack_sem, NULL);
	if (count_walk(K_OBJ_TYPE_SEM_ID, K_OBJ_CORE(isr_sem)) == 0) {
		zassert_equal(type->skipped, skipped + 4);
	} else {
		zassert_equal(type->skipped, skipped + 3);
		k_obj_core_unlink(K_OBJ_CORE(isr_sem));
	}
	skipped = type->skipped;

	if (!IS_ENABLED(CONFIG_KERNEL_COHERENCE)) {
		/* If CONFIG_KERNEL_COHERENCE is enabled, kernel requires
		 * kernel objects to be in coherent memory due to
		 * sys_cache_is_mem_coherent() check. Stack memory is not
		 * considered coherent memory so k_work_flush() will
		 * fail. So skip this part of test if stack is not in
		 * coherent memory.
		 */
		k_work_init(&work, work_handler);
		k_work_submit(&work);
		k_work_flush(&work, &sync);
		zassert_equal(type->skipped, skipped + 1);
	}

	zassert_equal(count_walk(K_OBJ_TYPE_SEM_ID, K_OBJ_CORE(&sem1)), 1);
}

static struct k_msgq msgq3;
static struct k_stack stack3;
static struct k_timer timer3;

/**
 * @brief Verify object cleanup ends the object's registration.
 *
 * @ingroup obj_core_tests
 * @verifies ZEP-SRS-35-2
 */
ZTEST(obj_core, test_obj_core_cleanup)
{
	/* Releasing an object's allocated buffer, or cleaning up a timer,
	 * ends its registration. Static objects stay reported.
	 */
	zassert_equal(k_msgq_alloc_init(&msgq3, 4, 4), 0);
	zassert_equal(count_walk(K_OBJ_TYPE_MSGQ_ID, K_OBJ_CORE(&msgq3)), 1);
	zassert_equal(k_msgq_cleanup(&msgq3), 0);
	zassert_equal(count_walk(K_OBJ_TYPE_MSGQ_ID, K_OBJ_CORE(&msgq3)), 0);
	zassert_equal(k_msgq_cleanup(&msgq1), 0);
	zassert_equal(count_walk(K_OBJ_TYPE_MSGQ_ID, K_OBJ_CORE(&msgq1)), 1);

	zassert_equal(k_stack_alloc_init(&stack3, 4), 0);
	zassert_equal(count_walk(K_OBJ_TYPE_STACK_ID, K_OBJ_CORE(&stack3)), 1);
	zassert_equal(k_stack_cleanup(&stack3), 0);
	zassert_equal(count_walk(K_OBJ_TYPE_STACK_ID, K_OBJ_CORE(&stack3)), 0);

	k_timer_init(&timer3, NULL, NULL);
	zassert_equal(count_walk(K_OBJ_TYPE_TIMER_ID, K_OBJ_CORE(&timer3)), 1);
	zassert_equal(k_timer_cleanup(&timer3), 0);
	zassert_equal(count_walk(K_OBJ_TYPE_TIMER_ID, K_OBJ_CORE(&timer3)), 0);
}

/**
 * @brief Verify a freed dynamic kernel object stops being reported.
 *
 * @ingroup obj_core_tests
 * @verifies ZEP-SRS-35-13
 */
ZTEST(obj_core, test_obj_core_object_free)
{
#ifdef CONFIG_DYNAMIC_OBJECTS
	struct k_sem *sem = k_object_alloc(K_OBJ_SEM);

	zassert_not_null(sem, "semaphore allocation failed");
	k_sem_init(sem, 0, 1);
	zassert_equal(count_walk(K_OBJ_TYPE_SEM_ID, K_OBJ_CORE(sem)), 1);

	k_object_free(sem);
	zassert_equal(count_walk(K_OBJ_TYPE_SEM_ID, K_OBJ_CORE(sem)), 0);
	zassert_equal(count_walk(K_OBJ_TYPE_SEM_ID, K_OBJ_CORE(&sem1)), 1);
#else
	ztest_test_skip();
#endif /* CONFIG_DYNAMIC_OBJECTS */
}

struct embedded {
	uint32_t pad;
	struct k_mutex mutex;
};

K_MEM_SLAB_DEFINE(slab3, ROUND_UP(sizeof(struct embedded), 8), 2, 8);

/**
 * @brief Verify objects in memory released to an allocator stop being reported.
 *
 * @ingroup obj_core_tests
 * @verifies ZEP-SRS-35-12
 * @verifies ZEP-SRS-35-13
 */
ZTEST(obj_core, test_obj_core_released_memory)
{
	struct embedded *heap_obj = k_malloc(sizeof(struct embedded));
	struct embedded *slab_obj;
	struct embedded *blocks_obj;

	/* Objects in memory returned to an allocator stop being reported */
	zassert_not_null(heap_obj, "allocation failed");
	k_mutex_init(&heap_obj->mutex);
	zassert_equal(count_walk(K_OBJ_TYPE_MUTEX_ID, K_OBJ_CORE(&heap_obj->mutex)), 1);
	k_free(heap_obj);
	zassert_equal(count_walk(K_OBJ_TYPE_MUTEX_ID, K_OBJ_CORE(&heap_obj->mutex)), 0);

	zassert_equal(k_mem_slab_alloc(&slab3, (void **)&slab_obj, K_NO_WAIT), 0);
	k_mutex_init(&slab_obj->mutex);
	zassert_equal(count_walk(K_OBJ_TYPE_MUTEX_ID, K_OBJ_CORE(&slab_obj->mutex)), 1);
	k_mem_slab_free(&slab3, slab_obj);
	zassert_equal(count_walk(K_OBJ_TYPE_MUTEX_ID, K_OBJ_CORE(&slab_obj->mutex)), 0);

	zassert_equal(sys_mem_blocks_alloc(&block2, 1, (void **)&blocks_obj), 0);
	k_mutex_init(&blocks_obj->mutex);
	zassert_equal(count_walk(K_OBJ_TYPE_MUTEX_ID, K_OBJ_CORE(&blocks_obj->mutex)), 1);
	zassert_equal(sys_mem_blocks_free(&block2, 1, (void **)&blocks_obj), 0);
	zassert_equal(count_walk(K_OBJ_TYPE_MUTEX_ID, K_OBJ_CORE(&blocks_obj->mutex)), 0);

	zassert_equal(count_walk(K_OBJ_TYPE_MUTEX_ID, K_OBJ_CORE(&mutex1)), 1);
}

static int free_in_walk_op(struct k_obj_core *obj_core, void *data)
{
	struct embedded **objp = data;

	if (obj_core == K_OBJ_CORE(&(*objp)->mutex)) {
		k_free(*objp);
	} else if (obj_core == K_OBJ_CORE(&mutex2)) {
		k_obj_core_unlink(obj_core);
	}

	return 0;
}

ZTEST(obj_core, test_obj_core_walk_callback)
{
	struct embedded *obj = k_malloc(sizeof(*obj));

	/* A walk callback may release memory holding registered objects and
	 * unregister objects itself: neither takes the registry lock.
	 */
	zassert_not_null(obj, "allocation failed");
	k_mutex_init(&obj->mutex);
	k_mutex_init(&mutex2);

	k_obj_type_walk_locked(k_obj_type_find(K_OBJ_TYPE_MUTEX_ID), free_in_walk_op, &obj);

	zassert_equal(count_walk(K_OBJ_TYPE_MUTEX_ID, K_OBJ_CORE(&obj->mutex)), 0);
	zassert_equal(count_walk(K_OBJ_TYPE_MUTEX_ID, K_OBJ_CORE(&mutex2)), 0);
	zassert_equal(count_walk(K_OBJ_TYPE_MUTEX_ID, K_OBJ_CORE(&mutex1)), 1);
}

ZTEST_SUITE(obj_core, NULL, NULL,
	    ztest_simple_1cpu_before, ztest_simple_1cpu_after, NULL);
