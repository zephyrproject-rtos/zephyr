/*
 * Copyright (c) 2026, Friedt Professional Engineering Services, Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "../../kernel/include/kthread.h"
#include <ksched.h>

#include <zephyr/internal/syscall_handler.h>
#include <zephyr/logging/log.h>
#include <zephyr/kernel.h>
#include <zephyr/kernel/mm.h>
#include <zephyr/kernel/thread_stack.h>
#include <zephyr/kernel/internal/mm.h>
#include <zephyr/sys/check.h>
#include <zephyr/sys/internal/kobject_internal.h>
#include <zephyr/sys/process.h>
#include <zephyr/sys/thread.h>
#include <zephyr/sys/elastipool.h>
#include <zephyr/toolchain.h>

#ifdef CONFIG_USERSPACE
#define K_THREAD_STACK_ALIGN(size) Z_THREAD_STACK_OBJ_ALIGN(size)
#else
#define K_THREAD_STACK_ALIGN(size) Z_KERNEL_STACK_OBJ_ALIGN
#endif

/* For declaring an array of k_thread */
#define K_THREAD_ARRAY_DEFINE(name, num) __maybe_unused struct k_thread name[num]

static void *heap_obj_allocator(void *obj, size_t size, enum k_objects otype, uint32_t flags);
static void heap_obj_free(void *obj, bool sched_locked);

LOG_MODULE_REGISTER(sys_thread, CONFIG_SYS_THREAD_LOG_LEVEL);

/* FIXME: should probably change the signature of sys_elastipool_api_heap_alloc_t so that
 * the first argument is non-const */
static inline void *stack_pool_heap_alloc(const void *ptr, size_t size, size_t align,
					  uint32_t flags)
{
	return heap_obj_allocator((void *)ptr, size, K_OBJ_THREAD_STACK_ELEMENT, flags);
}

static inline void *thread_pool_heap_alloc(const void *ptr, size_t size, size_t align,
					   uint32_t flags)
{
	return heap_obj_allocator((void *)ptr, size, K_OBJ_THREAD, flags);
}

static const char *const heap_obj_str(enum k_objects otype);

BUILD_ASSERT(SYS_THREAD_STACK_MIN >= 0);
BUILD_ASSERT(CONFIG_SYS_THREAD_STACK_MAX >= SYS_THREAD_STACK_MIN);

/*
 * Both pool locks are leaf locks: the kernel-object collector calls back into
 * the pool (z_kobj_owner_release) while holding its own locks, so nothing here
 * may take a kernel-object lock with a pool lock held. Elastic entries are
 * therefore reserved under the lock, allocated without it, and registered
 * under it again.
 */
static struct k_spinlock stack_pool_lock;
K_THREAD_STACK_ARRAY_DEFINE(sys_stack_pool, SYS_THREAD_STACK_MIN, CONFIG_SYS_THREAD_STACK_SIZE);
SYS_ELASTIPOOL_DEFINE_ADVANCED(stack_pool, K_THREAD_STACK_LEN(CONFIG_SYS_THREAD_STACK_SIZE),
			       K_THREAD_STACK_ALIGN(CONFIG_SYS_THREAD_STACK_SIZE),
			       SYS_THREAD_STACK_MIN, CONFIG_SYS_THREAD_STACK_MAX,
			       stack_pool_heap_alloc, sys_stack_pool, );

BUILD_ASSERT(SYS_THREAD_THREAD_MIN >= 0);
BUILD_ASSERT(CONFIG_SYS_THREAD_THREAD_MAX >= SYS_THREAD_THREAD_MIN);

static struct k_spinlock thread_pool_lock;
K_THREAD_ARRAY_DEFINE(sys_thread_pool, SYS_THREAD_THREAD_MIN);
SYS_ELASTIPOOL_DEFINE_ADVANCED(thread_pool, sizeof(struct k_thread), __alignof(struct k_thread),
			       SYS_THREAD_THREAD_MIN, CONFIG_SYS_THREAD_THREAD_MAX,
			       thread_pool_heap_alloc, sys_thread_pool, static);

/*
 * How an elastic stack was obtained, kept in the pool's map so that the release
 * path never has to ask the kernel-object layer (whose locks it may be under).
 */
#define STACK_KIND_HEAP   0U /* heap block, not a kernel object */
#define STACK_KIND_OBJECT 1U /* kernel object: the collector frees it */
#define STACK_KIND_MAPPED 2U /* anonymous mapping, not a kernel object */
#define STACK_KIND_MASK   3U
#define STACK_VALUE(kind, size) ((uint64_t)(size) << 2 | (kind))
#define STACK_KIND(value) ((uint32_t)((value) & STACK_KIND_MASK))
#define STACK_SIZE(value) ((size_t)((value) >> 2))

/* the pool frees nothing itself: the last reference going away asks it first */
static void *pool_obj_own(void *obj)
{
#ifdef CONFIG_USERSPACE
	struct k_object *ko = (obj != NULL) ? k_object_find(obj) : NULL;

	if (ko != NULL) {
		ko->flags |= K_OBJ_FLAG_OWNED;
	}
#endif /* CONFIG_USERSPACE */
	return obj;
}

static bool pool_is_elastic(const struct sys_elastipool *pool)
{
	return (pool->config->map != NULL) && (pool->config->max_obj > pool->config->min_obj);
}

/* lock held: reserve an elastic slot */
static int pool_reserve_locked(const struct sys_elastipool *pool)
{
	if (!pool_is_elastic(pool) || (pool->data->pool_size >= pool->config->max_obj)) {
		return -ENOMEM;
	}
	++pool->data->pool_size;

	return 0;
}

/* lock held: register an allocation in a reserved slot, or give the slot back */
static int pool_register_locked(const struct sys_elastipool *pool, const void *obj, uint64_t value)
{
	if ((obj == NULL) || (sys_hashmap_insert(pool->config->map, (uint64_t)(uintptr_t)obj,
						  value, NULL) < 0)) {
		--pool->data->pool_size;
		return -ENOMEM;
	}

	return 0;
}

/* lock held: drop the bookkeeping of a pool entry; its map value when elastic */
static int pool_forget_locked(const struct sys_elastipool *pool, const void *obj, uint64_t *value)
{
	if ((pool->config->min_obj > 0) && (sys_elastipool_api_free_static(pool, obj) == 0)) {
		return 1;
	}

	if (pool_is_elastic(pool) &&
	    sys_hashmap_remove(pool->config->map, (uint64_t)(uintptr_t)obj, value)) {
		--pool->data->pool_size;
		return 0;
	}

	return -EINVAL;
}

static void stack_free_memory(void *stack, uint64_t value, bool sched_locked)
{
	switch (STACK_KIND(value)) {
	case STACK_KIND_HEAP:
		heap_obj_free(stack, sched_locked);
		break;
#ifdef CONFIG_MMU
	case STACK_KIND_MAPPED:
		k_mem_unmap(stack, STACK_SIZE(value));
		break;
#endif /* CONFIG_MMU */
	default:
		/* a kernel object: the collector frees it */
		break;
	}
}

/*
 * Give a pool stack back: bookkeeping, and the memory when the pool allocated
 * it and it is not a kernel object (the collector frees those on its own).
 */
static int stack_pool_release(void *stack, bool sched_locked)
{
	int ret;
	uint64_t value = 0;

	K_SPINLOCK(&stack_pool_lock) {
		ret = pool_forget_locked(&stack_pool, stack, &value);
	}
	if (ret < 0) {
		return ret;
	}
	if (ret == 0) {
		stack_free_memory(stack, value, sched_locked);
	}

	return 0;
}

/*
 * A pool stack that is not a kernel object (a kernel thread's heap or mapped
 * stack) follows the thread that ran on it; a stack object is released by the
 * collector through its own references (it is bound to the thread, 3.3).
 */
/* without the lock: in the static region, or elastic stacks exist at all */
static bool stack_maybe_pooled(const k_thread_stack_t *stack)
{
	const struct sys_hashmap *map = stack_pool.config->map;

	if ((SYS_THREAD_STACK_MIN > 0) &&
	    (((uintptr_t)stack - (uintptr_t)stack_pool.config->storage) < sizeof(sys_stack_pool))) {
		return true;
	}

	return (map != NULL) && (sys_hashmap_size(map) != 0);
}

static void stack_follow_thread(k_thread_stack_t *stack, bool sched_locked)
{
	bool object = true;

	if ((stack == NULL) || !stack_maybe_pooled(stack)) {
		return;
	}

	K_SPINLOCK(&stack_pool_lock) {
		uint64_t value = 0;

		if (sys_elastipool_api_check_dyn(&stack_pool, stack) == 0) {
			(void)sys_hashmap_get(stack_pool.config->map, (uint64_t)(uintptr_t)stack,
					      &value);
			object = STACK_KIND(value) == STACK_KIND_OBJECT;
		} else if ((SYS_THREAD_STACK_MIN > 0) &&
			   (sys_elastipool_api_check_static(&stack_pool, stack) == 0)) {
			object = IS_ENABLED(CONFIG_USERSPACE);
		}
	}
	if (!object) {
		(void)stack_pool_release(stack, sched_locked);
	}
}

/* the thread object leaves the pool: bookkeeping, and its memory when asked */
static int thread_pool_release(struct k_thread *th, bool free_memory, bool sched_locked)
{
	int ret;
	uint64_t value = 0;

	K_SPINLOCK(&thread_pool_lock) {
		ret = pool_forget_locked(&thread_pool, th, &value);
	}
	if (ret < 0) {
		return ret;
	}
	if ((ret == 0) && free_memory) {
		heap_obj_free(th, sched_locked);
	}

	return 0;
}

/* heap_obj_allocator() hands out a kernel object for these */
static bool stack_is_object(uint32_t flags)
{
#ifdef CONFIG_USERSPACE
	return k_is_in_user_syscall() || ((flags & K_USER) != 0);
#else
	ARG_UNUSED(flags);
	return false;
#endif /* CONFIG_USERSPACE */
}

#ifdef CONFIG_USERSPACE
/* bookkeeping only: the stack object's memory is freed by the collector */
static int stack_pool_forget(void *stack)
{
	int ret;

	K_SPINLOCK(&stack_pool_lock) {
		ret = pool_forget_locked(&stack_pool, stack, NULL);
	}

	return ret;
}

void z_kobj_owner_release(struct k_object *ko, bool sched_locked)
{
	switch (ko->type) {
	case K_OBJ_THREAD:
		/* bookkeeping only; a non-object pool stack follows through z_sys_thread_reclaim() */
		(void)thread_pool_release(ko->name, false, sched_locked);
		break;
	case K_OBJ_THREAD_STACK_ELEMENT:
		(void)stack_pool_forget(ko->name);
		break;
	default:
		break;
	}
}
#endif /* CONFIG_USERSPACE */

bool z_sys_thread_owned(const struct k_thread *thread)
{
	bool owned;

	if (!thread->base.pool_hint) {
		return false;
	}

	K_SPINLOCK(&thread_pool_lock) {
		owned = (sys_elastipool_check(&thread_pool, thread) == 0);
	}

	return owned;
}

int z_sys_thread_reclaim(struct k_thread *thread, bool sched_locked)
{
	stack_follow_thread(thread->stack_obj, sched_locked);
#ifdef CONFIG_USERSPACE
	/* the thread object is released by the collector, through its references */
	return 0;
#else
	if (!thread->base.pool_hint) {
		return -EINVAL;
	}

	return thread_pool_release(thread, true, sched_locked);
#endif /* CONFIG_USERSPACE */
}

static int stack_pool_draw(k_thread_stack_t **stack, uint32_t *stack_size, uint32_t flags)
{
	int ret = -ENOMEM;
	bool elastic = false;

	if (*stack_size < CONFIG_SYS_THREAD_STACK_SIZE) {
		*stack_size = CONFIG_SYS_THREAD_STACK_SIZE;
	}

	K_SPINLOCK(&stack_pool_lock) {
		if ((*stack_size <= CONFIG_SYS_THREAD_STACK_SIZE) && (SYS_THREAD_STACK_MIN > 0)) {
			/* Try the guaranteed-static entries first */
			ret = sys_elastipool_api_alloc_static(&stack_pool, (void **)stack, flags);
			if (ret == 0) {
				K_SPINLOCK_BREAK;
			}
		}
		ret = pool_reserve_locked(&stack_pool);
		elastic = (ret == 0);
	}

	if (ret < 0) {
		return ret;
	}
	if (!elastic) {
		(void)pool_obj_own(*stack);
		z_kobj_bind_stack(*stack);
		return 0;
	}

	uint64_t value;

	if (IS_ENABLED(CONFIG_SYS_THREAD_STACK_ALLOC_ANON) && IS_ENABLED(CONFIG_MMU)) {
#ifdef CONFIG_MMU
		if (IS_ENABLED(CONFIG_USERSPACE) && ((flags & K_USER) != 0)) {
			*stack = pool_obj_own(k_object_map_size(K_OBJ_THREAD_STACK_ELEMENT, *stack_size));
			value = STACK_VALUE(STACK_KIND_OBJECT, 0);
		} else {
			size_t map_size = ROUND_UP(K_KERNEL_STACK_LEN(*stack_size), CONFIG_MMU_PAGE_SIZE);

			*stack = k_mem_map(map_size, K_MEM_PERM_RW | K_MEM_MAP_LOCK | K_MEM_MAP_UNINIT);
			value = STACK_VALUE(STACK_KIND_MAPPED, map_size);
		}
#endif /* CONFIG_MMU */
	} else if (IS_ENABLED(CONFIG_SYS_THREAD_STACK_ALLOC_HEAP)) {
		/* Forward flags so K_USER reaches the heap allocator: a user
		 * thread must not be given a kernel-only stack object.
		 */
		*stack = heap_obj_allocator(NULL, *stack_size, K_OBJ_THREAD_STACK_ELEMENT, flags);
		value = STACK_VALUE(stack_is_object(flags) ? STACK_KIND_OBJECT : STACK_KIND_HEAP, 0);
	} else {
		__ASSERT(false, "Invalid CONFIG_SYS_THREAD_STACK_ALLOC");
		*stack = NULL;
		value = 0;
	}

	K_SPINLOCK(&stack_pool_lock) {
		ret = pool_register_locked(&stack_pool, *stack, value);
	}
	if (ret < 0) {
		if (*stack != NULL) {
			stack_free_memory(*stack, value, false);
			if (STACK_KIND(value) == STACK_KIND_OBJECT) {
				k_object_free(*stack);
			}
			*stack = NULL;
		}
		return ret;
	}
	/* a pool stack belongs to the thread that runs on it: reaped with it */
	z_kobj_bind_stack(*stack);

	return 0;
}

int z_impl_sys_thread_stack_alloc(k_thread_stack_t **stack, uint32_t *stack_size,
				  uint32_t guard_size, uint32_t flags)
{
	ARG_UNUSED(guard_size);

	if (CONFIG_SYS_THREAD_STACK_MAX == 0) {
		LOG_WRN_ONCE("%s pool is disabled", heap_obj_str(K_OBJ_THREAD_STACK_ELEMENT));
		return -ENOTSUP;
	}

	if ((*stack_size > CONFIG_SYS_THREAD_STACK_SIZE) &&
	    (CONFIG_SYS_THREAD_STACK_MAX == SYS_THREAD_STACK_MIN)) {
		/* If the pool is not configured to allow allocations from heap or anonymous memory
		 * then the request cannot be satisfied */
		LOG_WRN_ONCE("%s of size %u exceeds CONFIG_SYS_THREAD_STACK_SIZE (%u)",
			     heap_obj_str(K_OBJ_THREAD_STACK_ELEMENT), *stack_size,
			     CONFIG_SYS_THREAD_STACK_SIZE);
		return -ENOMEM;
	}

	int ret = stack_pool_draw(stack, stack_size, flags);

	if ((ret == 0) && IS_ENABLED(CONFIG_USERSPACE) && ((flags & K_USER) != 0)) {
		/* the caller's reference: released by sys_thread_stack_destroy() or its reap */
		k_object_access_grant(*stack, _current);
	}

	return ret;
}

#ifdef CONFIG_USERSPACE
int z_vrfy_sys_thread_stack_alloc(k_thread_stack_t **stack, uint32_t *stack_size,
				  uint32_t guard_size, uint32_t flags)
{
	K_OOPS(K_SYSCALL_MEMORY_WRITE(stack, sizeof(*stack)));
	K_OOPS(K_SYSCALL_MEMORY_WRITE(stack_size, sizeof(*stack_size)));
	return z_impl_sys_thread_stack_alloc(stack, stack_size, guard_size, flags);
}
#include <zephyr/syscalls/sys_thread_stack_alloc_mrsh.c>
#endif /* CONFIG_USERSPACE */

struct stack_user_cb_data {
	k_tid_t tid;
	k_thread_stack_t *stack;
};

static void stack_user_cb(const struct k_thread *thread, void *user_data)
{
	struct stack_user_cb_data *const data = (struct stack_user_cb_data *)user_data;

	if (data->stack == (k_thread_stack_t *)thread->stack_info.start) {
		data->tid = (k_tid_t)thread;
	}
}

/* a stack that is not a kernel object has no liveness flag: ask the threads */
static bool stack_in_use(k_thread_stack_t *stack)
{
	struct stack_user_cb_data data = {.stack = stack};

	k_thread_foreach(stack_user_cb, &data);

	return (data.tid != NULL) && !(z_is_thread_state_set(data.tid, _THREAD_DUMMY) ||
				       z_is_thread_state_set(data.tid, _THREAD_DEAD));
}

int z_impl_sys_thread_stack_destroy(k_thread_stack_t *stack)
{
	if (CONFIG_SYS_THREAD_STACK_MAX == 0) {
		LOG_WRN_ONCE("%s pool is disabled", heap_obj_str(K_OBJ_THREAD_STACK_ELEMENT));
		return -ENOTSUP;
	}

#ifdef CONFIG_USERSPACE
	if (k_object_find(stack) != NULL) {
		/* drop the caller's reference: the collector disposes of the last one */
		k_object_access_revoke(stack, _current);
		return 0;
	}
#endif /* CONFIG_USERSPACE */

	CHECKIF(stack_in_use(stack)) {
		return -EBUSY;
	}

	return stack_pool_release(stack, false);
}

#ifdef CONFIG_USERSPACE
static inline int z_vrfy_sys_thread_stack_destroy(k_thread_stack_t *stack)
{
	K_OOPS(K_SYSCALL_OBJ_NEVER_INIT(stack, K_OBJ_THREAD_STACK_ELEMENT));
	return z_impl_sys_thread_stack_destroy(stack);
}
#include <zephyr/syscalls/sys_thread_stack_destroy_mrsh.c>
#endif /* CONFIG_USERSPACE */

static int thread_pool_draw(struct k_thread **th)
{
	int ret = -ENOMEM;
	bool elastic = false;

	K_SPINLOCK(&thread_pool_lock) {
		if (SYS_THREAD_THREAD_MIN > 0) {
			ret = sys_elastipool_api_alloc_static(&thread_pool, (void **)th, 0);
			if (ret == 0) {
				K_SPINLOCK_BREAK;
			}
		}
		ret = pool_reserve_locked(&thread_pool);
		elastic = (ret == 0);
	}

	if (ret < 0) {
		return ret;
	}
	if (elastic) {
		*th = heap_obj_allocator(NULL, 0, K_OBJ_THREAD, 0);
		K_SPINLOCK(&thread_pool_lock) {
			ret = pool_register_locked(&thread_pool, *th, 0);
		}
		if (ret < 0) {
			if (*th != NULL) {
				heap_obj_free(*th, false);
				*th = NULL;
			}
			return ret;
		}
	} else {
		(void)pool_obj_own(*th);
	}
	/* nothing may be read from it before k_thread_create() */
	(*th)->stack_obj = NULL;
	(*th)->base.pool_hint = true;

	return 0;
}

int z_sys_thread_create_common(struct k_thread **th, k_thread_stack_t *stack,
				    bool trusted,
				    uint32_t stack_size, uint32_t guard_size,
				    k_thread_entry_t entry, void *p1, void *p2, void *p3, int prio,
				    uint32_t options, k_timeout_t delay)
{
	int ret;

	/* deferred disposals of dead threads come back to the pools first */
	z_thread_graveyard_flush();

	ret = thread_pool_draw(th);
	if (ret < 0) {
		return ret;
	}

	if (stack == NULL) {
		ret = sys_thread_stack_alloc(&stack, &stack_size, guard_size, options);
		if (ret < 0) {
			(void)thread_pool_release(*th, true, false);
			return ret;
		}
		/* sys_thread_stack_alloc() normalizes stack_size up to at least
		 * CONFIG_SYS_THREAD_STACK_SIZE; reflect that here so k_thread_create()
		 * receives the correct usable size. */
		if (stack_size < CONFIG_SYS_THREAD_STACK_SIZE) {
			stack_size = CONFIG_SYS_THREAD_STACK_SIZE;
		}
	}

#ifdef CONFIG_USERSPACE
	if (k_is_in_user_syscall() && !trusted) {
		k_tid_t z_vrfy_k_thread_create(struct k_thread * new_thread,
					       k_thread_stack_t * stack, size_t stack_size,
					       k_thread_entry_t entry, void *p1, void *p2, void *p3,
					       int prio, uint32_t options, k_timeout_t delay);

		/* the verifier checks the caller's grants; the thread gets its own at setup */
		k_object_access_grant(stack, _current);
		k_object_access_grant(*th, _current);
		k_object_uninit(*th);
		z_vrfy_k_thread_create(*th, stack, stack_size, entry, p1, p2, p3, prio, options,
				       delay);
		return 0;
	}
#endif

	k_thread_create(*th, stack, stack_size, entry, p1, p2, p3, prio, options, delay);

	return 0;
}

static int thread_create_clone(struct k_thread **th, k_thread_stack_t *stack,
			       uint32_t stack_size, uint32_t guard_size, k_thread_entry_t entry,
			       void *p1, void *p2, void *p3, int prio, uint32_t options,
			       uint64_t clone_flags)
{
	int ret;
	k_pid_t child = NULL;
	struct sys_clone_args args;

	/* field by field: a whole-struct initializer becomes a memset call on every create */
	args.flags = SYS_CLONE_THREAD | clone_flags;
	args.entry = entry;
	args.p1 = p1;
	args.p2 = p2;
	args.p3 = p3;
	args.thread = NULL;
	args.stack = stack;
	args.stack_size = stack_size;
	args.guard_size = guard_size;
	args.prio = prio;
	args.options = options;
	args.sigmask = NULL;

	ret = z_impl_sys_clone(&args, &child);
	if (ret == 0) {
		*th = child;
	}

	return ret;
}

int z_impl_sys_thread_create(struct k_thread **th, k_thread_stack_t *stack, uint32_t stack_size,
			     uint32_t guard_size, k_thread_entry_t entry, void *p1, void *p2,
			     void *p3, int prio, uint32_t options)
{
	return thread_create_clone(th, stack, stack_size, guard_size, entry, p1, p2, p3, prio,
				   options, 0);
}

int sys_thread_create_paused(struct k_thread **th, k_thread_stack_t *stack, uint32_t stack_size,
			     uint32_t guard_size, k_thread_entry_t entry, void *p1, void *p2,
			     void *p3, int prio, uint32_t options)
{
	/* kernel-mode only: there is no system call for deferred-start creation */
	return thread_create_clone(th, stack, stack_size, guard_size, entry, p1, p2, p3, prio,
				   options, SYS_CLONE_PAUSED);
}

int sys_thread_create_paused_trusted(struct k_thread **th, k_thread_stack_t *stack,
				     uint32_t stack_size, uint32_t guard_size,
				     k_thread_entry_t entry, void *p1, void *p2, void *p3, int prio,
				     uint32_t options)
{
	/*
	 * Kernel-generated arguments (pool-drawn objects): skip the
	 * user-syscall revalidation even when called mid-syscall, so the
	 * kernel may construct e.g. a forked child's supervisor bootstrap.
	 */
	return z_sys_thread_create_common(th, stack, true, stack_size, guard_size, entry, p1, p2, p3,
					prio, options, K_FOREVER);
}

#ifdef CONFIG_USERSPACE
int z_vrfy_sys_thread_create(struct k_thread **th, k_thread_stack_t *stack, uint32_t stack_size,
			     uint32_t guard_size, k_thread_entry_t entry, void *p1, void *p2,
			     void *p3, int prio, uint32_t options)
{
	K_OOPS(K_SYSCALL_MEMORY_WRITE(th, sizeof(*th)));

	/* user threads may only create other user threads and also must inherit permissions */
	options |= K_USER | K_INHERIT_PERMS;

	/* Further verification is performed in z_vrfy_k_thread_create() */
	return z_impl_sys_thread_create(th, stack, stack_size, guard_size, entry, p1, p2, p3, prio,
					options);
}
#include <zephyr/syscalls/sys_thread_create_mrsh.c>
#endif

int z_impl_sys_thread_destroy(struct k_thread *th)
{
	if ((CONFIG_SYS_THREAD_STACK_MAX == 0) && (CONFIG_SYS_THREAD_THREAD_MAX == 0)) {
		LOG_WRN_ONCE("%s and %s pool are disabled", heap_obj_str(K_OBJ_THREAD),
			     heap_obj_str(K_OBJ_THREAD_STACK_ELEMENT));
		return -ENOTSUP;
	}

#ifdef CONFIG_USERSPACE
	/* drop the caller's reference: the collector disposes of the last one */
	k_object_access_revoke(th, _current);

	return 0;
#else
	CHECKIF(!(z_is_thread_state_set(th, _THREAD_DUMMY) ||
		  z_is_thread_state_set(th, _THREAD_DEAD))) {
		return -EBUSY;
	}

	z_thread_reap(th, false);

	return 0;
#endif /* CONFIG_USERSPACE */
}

#ifdef CONFIG_USERSPACE
int z_vrfy_sys_thread_destroy(struct k_thread *th)
{
	K_OOPS(K_SYSCALL_OBJ(th, K_OBJ_THREAD));
	return z_impl_sys_thread_destroy(th);
}
#include <zephyr/syscalls/sys_thread_destroy_mrsh.c>
#endif

static inline const char *const heap_obj_str(enum k_objects otype)
{
	switch (otype) {
	case K_OBJ_THREAD_STACK_ELEMENT:
		return "k_thread_stack_t";
	case K_OBJ_THREAD:
		return "k_thread";
	default:
		__ASSERT(false, "invalid object type %d", otype);
	}

	return "";
}

static void *heap_obj_allocator(void *obj, size_t size, enum k_objects otype, uint32_t flags)
{
	__ASSERT(obj == NULL, "the pool only allocates here");

	if (IS_ENABLED(CONFIG_DYNAMIC_OBJECTS)) {
		switch (otype) {
		case K_OBJ_THREAD:
			return pool_obj_own(k_object_alloc(otype));
		case K_OBJ_THREAD_STACK_ELEMENT:
#ifdef CONFIG_USERSPACE
			if (k_is_in_user_syscall() || ((flags & K_USER) != 0)) {
				return pool_obj_own(k_object_alloc_size(otype, size));
			} else
#endif
			{
				return z_thread_aligned_alloc(Z_KERNEL_STACK_OBJ_ALIGN,
							      K_KERNEL_STACK_LEN(size));
			}
		default:
			__ASSERT(false, "invalid object type %d", otype);
			return NULL;
		}
	}

	if (IS_ENABLED(CONFIG_USERSPACE) && k_is_user_context()) {
		LOG_WRN_ONCE("CONFIG_DYNAMIC_OBJECTS is not enabled");
		return NULL;
	}

	size_t align;

	switch (otype) {
	case K_OBJ_THREAD:
		size = sizeof(struct k_thread);
		align = __alignof(struct k_thread);
		break;
	case K_OBJ_THREAD_STACK_ELEMENT:
		size = K_KERNEL_STACK_LEN(size);
		align = Z_KERNEL_STACK_OBJ_ALIGN;
		break;
	default:
		__ASSERT(false, "invalid object type %d", otype);
		return NULL;
	}

	return z_thread_aligned_alloc(align, size);
}

/* memory the pool allocated and that is not a kernel object */
static void heap_obj_free(void *obj, bool sched_locked)
{
#ifdef CONFIG_USERSPACE
	if (!sched_locked && (k_object_find(obj) != NULL)) {
		/* only reached on an allocation that failed to register */
		k_object_free(obj);
		return;
	}
#endif /* CONFIG_USERSPACE */
	__ASSERT(!sched_locked, "heap release under the scheduler lock");
	k_free(obj);
}

#define ONCE_RUNNING BIT(0)
#define ONCE_DONE    BIT(1)

BUILD_ASSERT(sizeof(sys_thread_once_t) == sizeof(struct k_futex) &&
	     offsetof(sys_thread_once_t, val) == offsetof(struct k_futex, val) &&
	     offsetof(sys_thread_once_t, waiters) == offsetof(struct k_futex, waiters));

static void once_wait(sys_thread_once_t *flag, atomic_val_t val)
{
	(void)k_futex_wait((struct k_futex *)flag, val, K_FOREVER);
}

static void once_wake(sys_thread_once_t *flag)
{
	if (atomic_get(&flag->waiters) != 0) {
		(void)k_futex_wake((struct k_futex *)flag, true);
	}
}

#ifdef CONFIG_THREAD_CLEANUP
static void once_cancelled(void *arg)
{
	sys_thread_once_t *flag = arg;

	atomic_set(&flag->val, 0);
	once_wake(flag);
}

static void once_run(sys_thread_once_t *flag, void (*func)(void))
{
	void *cleanup[3];

	k_thread_cleanup_push(cleanup, once_cancelled, flag);
	func();
	(void)k_thread_cleanup_pop(0);
}
#else
static void once_run(sys_thread_once_t *flag, void (*func)(void))
{
	ARG_UNUSED(flag);
	func();
}
#endif

void sys_thread_once(sys_thread_once_t *flag, void (*func)(void))
{
	atomic_val_t val;

	while (true) {
		val = atomic_get(&flag->val);
		if (val == ONCE_DONE) {
			return;
		}

		CHECKIF(val != 0 && val != ONCE_RUNNING) {
			/* an uninitialized flag: the routine is not run */
			return;
		}

		if (val == 0) {
			if (!atomic_cas(&flag->val, 0, ONCE_RUNNING)) {
				continue;
			}
			once_run(flag, func);
			atomic_set(&flag->val, ONCE_DONE);
			once_wake(flag);
			return;
		}

		once_wait(flag, val);
	}
}

#ifdef CONFIG_ZTEST
#include <zephyr/ztest.h>

/* pool usage before each test: what the test must leave behind */
static size_t pool_baseline[2];
static const struct sys_elastipool *const pools[] = {&stack_pool, &thread_pool};
static const enum k_objects pool_types[] = {K_OBJ_THREAD_STACK_ELEMENT, K_OBJ_THREAD};

static void record_stats(const struct ztest_unit_test *test, void *data)
{
	ARG_UNUSED(test);
	ARG_UNUSED(data);

	ARRAY_FOR_EACH(pools, i) {
		pool_baseline[i] = pools[i]->data->pool_size;
	}
}

static bool pools_at_baseline(void)
{
	ARRAY_FOR_EACH(pools, i) {
		if (pools[i]->data->pool_size != pool_baseline[i]) {
			return false;
		}
	}

	return true;
}

/*
 * Only observes: a thread that exited on another CPU may still be halting, and
 * its objects come back once it has switched out, so the pools get a bounded
 * time to settle before a leak is called.
 */
static void print_stats(const struct ztest_unit_test *test, void *data)
{
	ARG_UNUSED(test);
	ARG_UNUSED(data);

	for (uint32_t t0 = k_uptime_get_32(); !pools_at_baseline();) {
		z_thread_graveyard_flush();
		if (pools_at_baseline() || ((k_uptime_get_32() - t0) > 200U)) {
			break;
		}
		k_msleep(1);
	}

	bool leak = !pools_at_baseline();

	ARRAY_FOR_EACH(pools, i) {
		if (pools[i]->data->pool_size != pool_baseline[i]) {
			TC_PRINT("%s pool usage: %zu/%zu (was %zu)\n", heap_obj_str(pool_types[i]),
				 pools[i]->data->pool_size, pools[i]->config->max_obj,
				 pool_baseline[i]);
		}
	}

	/* This will assert if any tests leak objects */
	__ASSERT(!leak, "object pool leak detected");
}

ZTEST_RULE(sys_thread_rule, record_stats, print_stats);
#endif /* CONFIG_ZTEST */
