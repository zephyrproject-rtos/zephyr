/*
 * Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/kernel/obj_core.h>
#include <zephyr/sys/mem_stats.h>
#include <zephyr/sys/printk.h>

/* Statically defined objects are reported from their storage without any
 * registration.
 */
K_SEM_DEFINE(static_sem, 0, 1);
K_MUTEX_DEFINE(static_mutex);
K_MSGQ_DEFINE(static_msgq, sizeof(uint32_t), 4, 4);
K_TIMER_DEFINE(tick_timer, NULL, NULL);
K_MEM_SLAB_DEFINE(slab, 64, 4, 8);

/* Objects initialized at run time are registered by their init function */
static struct k_sem runtime_sem;

struct request {
	uint32_t id;
	struct k_mutex lock;
};

static void worker_fn(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	while (true) {
		k_sem_take(&static_sem, K_FOREVER);
	}
}

K_THREAD_DEFINE(worker, 1024, worker_fn, NULL, NULL, NULL, 5, 0, 0);

static struct k_thread dyn_thread;
K_THREAD_STACK_DEFINE(dyn_stack, 1024);

static const struct {
	uint32_t id;
	const char *name;
} types[] = {
	{ K_OBJ_TYPE_THREAD_ID, "thread" },
	{ K_OBJ_TYPE_SEM_ID, "semaphore" },
	{ K_OBJ_TYPE_MUTEX_ID, "mutex" },
	{ K_OBJ_TYPE_MSGQ_ID, "message queue" },
	{ K_OBJ_TYPE_TIMER_ID, "timer" },
	{ K_OBJ_TYPE_MEM_SLAB_ID, "memory slab" },
	{ K_OBJ_TYPE_CPU_ID, "cpu" },
};

/* The object that an object core belongs to */
static void *object_of(struct k_obj_core *obj_core)
{
	return (uint8_t *)obj_core - obj_core->type->obj_core_offset;
}

static int count_op(struct k_obj_core *obj_core, void *data)
{
	ARG_UNUSED(obj_core);
	(*(uint32_t *)data)++;

	return 0;
}

static int find_op(struct k_obj_core *obj_core, void *data)
{
	/* A non-zero return stops the walk and is returned to the caller */
	return (obj_core == data) ? 1 : 0;
}

static bool is_reported(uint32_t type_id, struct k_obj_core *obj_core)
{
	struct k_obj_type *type = k_obj_type_find(type_id);

	return k_obj_type_walk_locked(type, find_op, obj_core) == 1;
}

static void report(const char *what, uint32_t type_id, struct k_obj_core *obj_core)
{
	printk("  %-46s reported: %s\n", what, is_reported(type_id, obj_core) ? "yes" : "no");
}

static void print_inventory(void)
{
	printk("Object inventory\n");
	printk("  %-14s %7s %8s %8s\n", "type", "objects", "skipped", "dropped");

	for (size_t i = 0; i < ARRAY_SIZE(types); i++) {
		struct k_obj_type *type = k_obj_type_find(types[i].id);
		uint32_t count = 0;

		if (type == NULL) {
			/* The type's kernel code is not linked into this image */
			printk("  %-14s not present\n", types[i].name);
			continue;
		}

		k_obj_type_walk_locked(type, count_op, &count);
		printk("  %-14s %7u %8u %8u\n", types[i].name, count, type->skipped,
		       type->dropped);
	}
}

static int print_thread_op(struct k_obj_core *obj_core, void *data)
{
	struct k_thread *thread = object_of(obj_core);
	struct k_thread_runtime_stats stats;
	const char *name = k_thread_name_get(thread);

	ARG_UNUSED(data);

	if (k_obj_core_stats_query(obj_core, &stats, sizeof(stats)) == 0) {
		printk("  %-16s %p %12llu cycles\n", (name != NULL) ? name : "?", thread,
		       (unsigned long long)stats.total_cycles);
	} else {
		printk("  %-16s %p (no statistics)\n", (name != NULL) ? name : "?", thread);
	}

	return 0;
}

static void wait_with_stack_semaphore(void)
{
	struct k_sem sem;

	/* A semaphore in a stack frame ends with the frame, so it is not
	 * registered: the type counts it as skipped instead.
	 */
	k_sem_init(&sem, 0, 1);
	report("stack semaphore", K_OBJ_TYPE_SEM_ID, K_OBJ_CORE(&sem));
}

static void lifetime_demo(void)
{
	struct request *req;
	struct k_obj_type *sem_type = k_obj_type_find(K_OBJ_TYPE_SEM_ID);
	uint32_t skipped = sem_type->skipped;

	printk("\nStatic and run-time objects\n");
	report("static semaphore", K_OBJ_TYPE_SEM_ID, K_OBJ_CORE(&static_sem));
	report("runtime semaphore before k_sem_init", K_OBJ_TYPE_SEM_ID,
	       K_OBJ_CORE(&runtime_sem));
	k_sem_init(&runtime_sem, 0, 1);
	report("runtime semaphore", K_OBJ_TYPE_SEM_ID, K_OBJ_CORE(&runtime_sem));

	printk("\nTransient objects\n");
	wait_with_stack_semaphore();
	printk("  semaphores skipped: %u -> %u\n", skipped, sem_type->skipped);

	printk("\nThreads\n");
	report("worker thread", K_OBJ_TYPE_THREAD_ID, K_OBJ_CORE(worker));
	k_thread_create(&dyn_thread, dyn_stack, K_THREAD_STACK_SIZEOF(dyn_stack), worker_fn,
			NULL, NULL, NULL, 5, 0, K_NO_WAIT);
	k_thread_name_set(&dyn_thread, "dynamic");
	report("dynamic thread", K_OBJ_TYPE_THREAD_ID, K_OBJ_CORE(&dyn_thread));
	k_thread_abort(&dyn_thread);
	report("aborted thread", K_OBJ_TYPE_THREAD_ID, K_OBJ_CORE(&dyn_thread));

	printk("\nReleased memory\n");
	req = k_malloc(sizeof(*req));
	if (req == NULL) {
		printk("  allocation failed\n");
		return;
	}
	k_mutex_init(&req->lock);
	report("heap mutex", K_OBJ_TYPE_MUTEX_ID, K_OBJ_CORE(&req->lock));
	k_free(req);
	report("heap mutex after k_free", K_OBJ_TYPE_MUTEX_ID, K_OBJ_CORE(&req->lock));

	printk("\nExplicit unregistration\n");
	k_obj_core_unlink(K_OBJ_CORE(&runtime_sem));
	report("runtime semaphore after k_obj_core_unlink", K_OBJ_TYPE_SEM_ID,
	       K_OBJ_CORE(&runtime_sem));
}

static void stats_demo(void)
{
	struct sys_memory_stats mem_stats;
	struct k_thread_runtime_stats sys_stats;
	void *blocks[2];

	printk("\nThread statistics\n");
	k_obj_type_walk_locked(k_obj_type_find(K_OBJ_TYPE_THREAD_ID), print_thread_op, NULL);

	printk("\nMemory slab statistics\n");
	for (size_t i = 0; i < ARRAY_SIZE(blocks); i++) {
		if (k_mem_slab_alloc(&slab, &blocks[i], K_NO_WAIT) != 0) {
			printk("  allocation failed\n");
			return;
		}
	}
	if (k_obj_core_stats_query(K_OBJ_CORE(&slab), &mem_stats, sizeof(mem_stats)) == 0) {
		printk("  slab %p: allocated %zu bytes, free %zu bytes, peak %zu bytes\n",
		       &slab, mem_stats.allocated_bytes, mem_stats.free_bytes,
		       mem_stats.max_allocated_bytes);
	}
	for (size_t i = 0; i < ARRAY_SIZE(blocks); i++) {
		k_mem_slab_free(&slab, blocks[i]);
	}

	printk("\nSystem statistics\n");
	if (k_obj_core_stats_query(K_OBJ_CORE(&_kernel), &sys_stats, sizeof(sys_stats)) == 0) {
		printk("  all CPUs: %llu cycles, %llu non-idle\n",
		       (unsigned long long)sys_stats.execution_cycles,
		       (unsigned long long)sys_stats.total_cycles);
	}
}

int main(void)
{
	uint32_t msg = 0U;

	printk("Object cores sample%s\n\n",
	       IS_ENABLED(CONFIG_TRACING_OBJECT_TRACKING) ? " (enabled by object tracking)" : "");

	/* Use the message queue, mutex and timer so that their kernel code and
	 * object types are part of the image.
	 */
	k_msgq_put(&static_msgq, &msg, K_NO_WAIT);
	k_mutex_lock(&static_mutex, K_NO_WAIT);
	k_mutex_unlock(&static_mutex);
	k_timer_start(&tick_timer, K_MSEC(100), K_MSEC(100));

	print_inventory();
	lifetime_demo();
	stats_demo();

	printk("\nDone\n");

	return 0;
}
