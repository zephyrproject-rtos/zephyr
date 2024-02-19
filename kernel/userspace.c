/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */


#include <zephyr/kernel.h>
#include <string.h>
#include <zephyr/sys/math_extras.h>
#include <zephyr/sys/rb.h>
#include <zephyr/kernel_structs.h>
#include <zephyr/sys/sys_io.h>
#include <ksched.h>
#include <zephyr/syscall.h>
#include <zephyr/internal/syscall_handler.h>
#include <zephyr/device.h>
#include <zephyr/init.h>
#include <stdbool.h>
#include <zephyr/app_memory/app_memdomain.h>
#include <zephyr/sys/libc-hooks.h>
#include <zephyr/sys/mutex.h>
#include <inttypes.h>
#include <zephyr/linker/linker-defs.h>

#ifdef Z_LIBC_PARTITION_EXISTS
K_APPMEM_PARTITION_DEFINE(z_libc_partition);
#endif

/* TODO: Find a better place to put this. Since we pull the entire
 * lib..__modules__crypto__mbedtls.a  globals into app shared memory
 * section, we can't put this in zephyr_init.c of the mbedtls module.
 */
#ifdef CONFIG_MBEDTLS
K_APPMEM_PARTITION_DEFINE(k_mbedtls_partition);
#endif

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(os, CONFIG_KERNEL_LOG_LEVEL);

/* The originally synchronization strategy made heavy use of recursive
 * irq_locking, which ports poorly to spinlocks which are
 * non-recursive.  Rather than try to redesign as part of
 * spinlockification, this uses multiple locks to preserve the
 * original semantics exactly.  The locks are named for the data they
 * protect where possible, or just for the code that uses them where
 * not.
 */
#ifdef CONFIG_DYNAMIC_OBJECTS
static struct k_spinlock lists_lock;       /* kobj dlist */
static struct k_spinlock objfree_lock;     /* k_object_free */

#ifdef CONFIG_GEN_PRIV_STACKS
/* On ARM & ARC MPU we may have two different alignment requirement
 * when dynamically allocating thread stacks, one for the privileged
 * stack and other for the user stack, so we need to account the
 * worst alignment scenario and reserve space for that.
 */
#if defined(CONFIG_ARM_MPU) || defined(CONFIG_ARC_MPU)
#define STACK_ELEMENT_DATA_SIZE(size) \
	(sizeof(struct z_stack_data) + CONFIG_PRIVILEGED_STACK_SIZE + \
	Z_THREAD_STACK_OBJ_ALIGN(size) + Z_THREAD_STACK_SIZE_ADJUST(size))
#else
#define STACK_ELEMENT_DATA_SIZE(size) (sizeof(struct z_stack_data) + \
	Z_THREAD_STACK_SIZE_ADJUST(size))
#endif /* CONFIG_ARM_MPU || CONFIG_ARC_MPU */
#else
#define STACK_ELEMENT_DATA_SIZE(size) Z_THREAD_STACK_SIZE_ADJUST(size)
#endif /* CONFIG_GEN_PRIV_STACKS */

#endif
static struct k_spinlock obj_lock;         /* kobj struct data */

#define MAX_THREAD_BITS		(CONFIG_MAX_THREAD_BYTES * 8)

#ifdef CONFIG_DYNAMIC_OBJECTS
extern uint8_t _thread_idx_map[CONFIG_MAX_THREAD_BYTES];
#endif

static void clear_perms_cb(struct k_object *ko, void *ctx_ptr);

const char *otype_to_str(enum k_objects otype)
{
	const char *ret;
	/* -fdata-sections doesn't work right except in very very recent
	 * GCC and these literal strings would appear in the binary even if
	 * otype_to_str was omitted by the linker
	 */
#ifdef CONFIG_LOG
	switch (otype) {
	/* otype-to-str.h is generated automatically during build by
	 * gen_kobject_list.py
	 */
	case K_OBJ_ANY:
		ret = "generic";
		break;
#include <otype-to-str.h>
	default:
		ret = "?";
		break;
	}
#else
	ARG_UNUSED(otype);
	ret = NULL;
#endif
	return ret;
}

struct perm_ctx {
	int parent_id;
	int child_id;
	struct k_thread *parent;
};

#ifdef CONFIG_GEN_PRIV_STACKS
/* See write_gperf_table() in scripts/build/gen_kobject_list.py. The privilege
 * mode stacks are allocated as an array. The base of the array is
 * aligned to Z_PRIVILEGE_STACK_ALIGN, and all members must be as well.
 */
uint8_t *z_priv_stack_find(k_thread_stack_t *stack)
{
	struct k_object *obj = k_object_find(stack);

	__ASSERT(obj != NULL, "stack object not found");
	__ASSERT(obj->type == K_OBJ_THREAD_STACK_ELEMENT,
		 "bad stack object");

	return obj->data.stack_data->priv;
}
#endif /* CONFIG_GEN_PRIV_STACKS */

#ifdef CONFIG_DYNAMIC_OBJECTS

/*
 * Note that dyn_obj->data is where the kernel object resides
 * so it is the one that actually needs to be aligned.
 * Due to the need to get the the fields inside struct dyn_obj
 * from kernel object pointers (i.e. from data[]), the offset
 * from data[] needs to be fixed at build time. Therefore,
 * data[] is declared with __aligned(), such that when dyn_obj
 * is allocated with alignment, data[] is also aligned.
 * Due to this requirement, data[] needs to be aligned with
 * the maximum alignment needed for all kernel objects
 * (hence the following DYN_OBJ_DATA_ALIGN).
 */
#ifdef ARCH_DYNAMIC_OBJ_K_THREAD_ALIGNMENT
#define DYN_OBJ_DATA_ALIGN_K_THREAD	(ARCH_DYNAMIC_OBJ_K_THREAD_ALIGNMENT)
#else
#define DYN_OBJ_DATA_ALIGN_K_THREAD	(sizeof(void *))
#endif

#ifdef CONFIG_DYNAMIC_THREAD_STACK_SIZE
#ifndef CONFIG_MPU_STACK_GUARD
#define DYN_OBJ_DATA_ALIGN_K_THREAD_STACK \
	Z_THREAD_STACK_OBJ_ALIGN(CONFIG_PRIVILEGED_STACK_SIZE)
#else
#define DYN_OBJ_DATA_ALIGN_K_THREAD_STACK \
	Z_THREAD_STACK_OBJ_ALIGN(CONFIG_DYNAMIC_THREAD_STACK_SIZE)
#endif /* !CONFIG_MPU_STACK_GUARD */
#else
#define DYN_OBJ_DATA_ALIGN_K_THREAD_STACK \
	Z_THREAD_STACK_OBJ_ALIGN(ARCH_STACK_PTR_ALIGN)
#endif /* CONFIG_DYNAMIC_THREAD_STACK_SIZE */

#define DYN_OBJ_DATA_ALIGN		\
	MAX(DYN_OBJ_DATA_ALIGN_K_THREAD, (sizeof(void *)))

struct dyn_obj {
	struct k_object kobj;
	sys_dnode_t dobj_list;

	/* The object itself */
	void *data;
};

extern struct k_object *z_object_gperf_find(const void *obj);
extern void z_object_gperf_wordlist_foreach(_wordlist_cb_func_t func,
					     void *context);

/*
 * Linked list of allocated kernel objects, for iteration over all allocated
 * objects (and potentially deleting them during iteration).
 */
static sys_dlist_t obj_list = SYS_DLIST_STATIC_INIT(&obj_list);

/*
 * TODO: Write some hash table code that will replace obj_list.
 */

static size_t obj_size_get(enum k_objects otype)
{
	size_t ret;

	switch (otype) {
#include <otype-to-size.h>
	default:
		ret = sizeof(const struct device);
		break;
	}

	return ret;
}

static size_t obj_align_get(enum k_objects otype)
{
	size_t ret;

	switch (otype) {
	case K_OBJ_THREAD:
#ifdef ARCH_DYNAMIC_OBJ_K_THREAD_ALIGNMENT
		ret = ARCH_DYNAMIC_OBJ_K_THREAD_ALIGNMENT;
#else
		ret = __alignof(struct dyn_obj);
#endif
		break;
	default:
		ret = __alignof(struct dyn_obj);
		break;
	}

	return ret;
}

static struct dyn_obj *dyn_object_find(void *obj)
{
	struct dyn_obj *node;
	k_spinlock_key_t key;

	/* For any dynamically allocated kernel object, the object
	 * pointer is just a member of the containing struct dyn_obj,
	 * so just a little arithmetic is necessary to locate the
	 * corresponding struct rbnode
	 */
	key = k_spin_lock(&lists_lock);

	SYS_DLIST_FOR_EACH_CONTAINER(&obj_list, node, dobj_list) {
		if (node->kobj.name == obj) {
			goto end;
		}
	}

	/* No object found */
	node = NULL;

 end:
	k_spin_unlock(&lists_lock, key);

	return node;
}

/**
 * @internal
 *
 * @brief Allocate a new thread index for a new thread.
 *
 * This finds an unused thread index that can be assigned to a new
 * thread. If too many threads have been allocated, the kernel will
 * run out of indexes and this function will fail.
 *
 * Note that if an unused index is found, that index will be marked as
 * used after return of this function.
 *
 * @param tidx The new thread index if successful
 *
 * @return true if successful, false if failed
 **/
static bool thread_idx_alloc(uintptr_t *tidx)
{
	int i;
	int idx;
	int base;

	base = 0;
	for (i = 0; i < CONFIG_MAX_THREAD_BYTES; i++) {
		idx = find_lsb_set(_thread_idx_map[i]);

		if (idx != 0) {
			*tidx = base + (idx - 1);

			sys_bitfield_clear_bit((mem_addr_t)_thread_idx_map,
					       *tidx);

			/* Clear permission from all objects */
			k_object_wordlist_foreach(clear_perms_cb,
						   (void *)*tidx);

			return true;
		}

		base += 8;
	}

	return false;
}

/**
 * @internal
 *
 * @brief Free a thread index.
 *
 * This frees a thread index so it can be used by another
 * thread.
 *
 * @param tidx The thread index to be freed
 **/
static void thread_idx_free(uintptr_t tidx)
{
	/* To prevent leaked permission when index is recycled */
	k_object_wordlist_foreach(clear_perms_cb, (void *)tidx);

	sys_bitfield_set_bit((mem_addr_t)_thread_idx_map, tidx);
}

static struct k_object *dynamic_object_create(enum k_objects otype, size_t align,
					      size_t size)
{
	struct dyn_obj *dyn;

	dyn = z_thread_aligned_alloc(align, sizeof(struct dyn_obj));
	if (dyn == NULL) {
		return NULL;
	}

	if (otype == K_OBJ_THREAD_STACK_ELEMENT) {
		size_t adjusted_size;

		if (size == 0) {
			k_free(dyn);
			return NULL;
		}

		adjusted_size = STACK_ELEMENT_DATA_SIZE(size);
		dyn->data = z_thread_aligned_alloc(DYN_OBJ_DATA_ALIGN_K_THREAD_STACK,
						     adjusted_size);
		if (dyn->data == NULL) {
			k_free(dyn);
			return NULL;
		}

#ifdef CONFIG_GEN_PRIV_STACKS
		struct z_stack_data *stack_data = (struct z_stack_data *)
			((uint8_t *)dyn->data + adjusted_size - sizeof(*stack_data));
		stack_data->priv = (uint8_t *)dyn->data;
		dyn->kobj.data.stack_data = stack_data;
#if defined(CONFIG_ARM_MPU) || defined(CONFIG_ARC_MPU)
		dyn->kobj.name = (void *)ROUND_UP(
			  ((uint8_t *)dyn->data + CONFIG_PRIVILEGED_STACK_SIZE),
			  Z_THREAD_STACK_OBJ_ALIGN(size));
#else
		dyn->kobj.name = dyn->data;
#endif
#else
		dyn->kobj.name = dyn->data;
#endif
	} else {
		dyn->data = z_thread_aligned_alloc(align, obj_size_get(otype) + size);
		if (dyn->data == NULL) {
			k_free(dyn->data);
			return NULL;
		}
		dyn->kobj.name = dyn->data;
	}

	dyn->kobj.type = otype;
	dyn->kobj.flags = 0;
	(void)memset(dyn->kobj.perms, 0, CONFIG_MAX_THREAD_BYTES);

	k_spinlock_key_t key = k_spin_lock(&lists_lock);

	sys_dlist_append(&obj_list, &dyn->dobj_list);
	k_spin_unlock(&lists_lock, key);

	return &dyn->kobj;
}

struct k_object *k_object_create_dynamic_aligned(size_t align, size_t size)
{
	struct k_object *obj = dynamic_object_create(K_OBJ_ANY, align, size);

	if (obj == NULL) {
		LOG_ERR("could not allocate kernel object, out of memory");
	}

	return obj;
}

static void *z_object_alloc(enum k_objects otype, size_t size)
{
	struct k_object *zo;
	uintptr_t tidx = 0;

	if (otype <= K_OBJ_ANY || otype >= K_OBJ_LAST) {
		LOG_ERR("bad object type %d requested", otype);
		return NULL;
	}

	switch (otype) {
	case K_OBJ_THREAD:
		if (!thread_idx_alloc(&tidx)) {
			LOG_ERR("out of free thread indexes");
			return NULL;
		}
		break;
	/* The following are currently not allowed at all */
	case K_OBJ_FUTEX:			/* Lives in user memory */
	case K_OBJ_SYS_MUTEX:			/* Lives in user memory */
	case K_OBJ_NET_SOCKET:			/* Indeterminate size */
		LOG_ERR("forbidden object type '%s' requested",
			otype_to_str(otype));
		return NULL;
	default:
		/* Remainder within bounds are permitted */
		break;
	}

	zo = dynamic_object_create(otype, obj_align_get(otype), size);
	if (zo == NULL) {
		if (otype == K_OBJ_THREAD) {
			thread_idx_free(tidx);
		}
		return NULL;
	}

	if (otype == K_OBJ_THREAD) {
		zo->data.thread_id = tidx;
	}

	/* The allocating thread implicitly gets permission on kernel objects
	 * that it allocates
	 */
	k_thread_perms_set(zo, _current);

	/* Activates reference counting logic for automatic disposal when
	 * all permissions have been revoked
	 */
	zo->flags |= K_OBJ_FLAG_ALLOC;

	return zo->name;
}

void *z_impl_k_object_alloc(enum k_objects otype)
{
	return z_object_alloc(otype, 0);
}

void *z_impl_k_object_alloc_size(enum k_objects otype, size_t size)
{
	return z_object_alloc(otype, size);
}

void k_object_free(void *obj)
{
	struct dyn_obj *dyn;

	/* This function is intentionally not exposed to user mode.
	 * There's currently no robust way to track that an object isn't
	 * being used by some other thread
	 */

	k_spinlock_key_t key = k_spin_lock(&objfree_lock);

	dyn = dyn_object_find(obj);
	if (dyn != NULL) {
		sys_dlist_remove(&dyn->dobj_list);

		if (dyn->kobj.type == K_OBJ_THREAD) {
			thread_idx_free(dyn->kobj.data.thread_id);
		}
	}
	k_spin_unlock(&objfree_lock, key);

	if (dyn != NULL) {
		k_free(dyn->data);
		k_free(dyn);
	}
}

struct k_object *k_object_find(const void *obj)
{
	struct k_object *ret;

	ret = z_object_gperf_find(obj);

	if (ret == NULL) {
		struct dyn_obj *dyn;

		/* The cast to pointer-to-non-const violates MISRA
		 * 11.8 but is justified since we know dynamic objects
		 * were not declared with a const qualifier.
		 */
		dyn = dyn_object_find((void *)obj);
		if (dyn != NULL) {
			ret = &dyn->kobj;
		}
	}

	return ret;
}

void k_object_wordlist_foreach(_wordlist_cb_func_t func, void *context)
{
	struct dyn_obj *obj, *next;

	z_object_gperf_wordlist_foreach(func, context);

	k_spinlock_key_t key = k_spin_lock(&lists_lock);

	SYS_DLIST_FOR_EACH_CONTAINER_SAFE(&obj_list, obj, next, dobj_list) {
		func(&obj->kobj, context);
	}
	k_spin_unlock(&lists_lock, key);
}
#endif /* CONFIG_DYNAMIC_OBJECTS */

static unsigned int thread_index_get(struct k_thread *thread)
{
	struct k_object *ko;

	ko = k_object_find(thread);

	if (ko == NULL) {
		return -1;
	}

	return ko->data.thread_id;
}

static void unref_check(struct k_object *ko, uintptr_t index)
{
	k_spinlock_key_t key = k_spin_lock(&obj_lock);

	sys_bitfield_clear_bit((mem_addr_t)&ko->perms, index);

#ifdef CONFIG_DYNAMIC_OBJECTS
	if ((ko->flags & K_OBJ_FLAG_ALLOC) == 0U) {
		/* skip unref check for static kernel object */
		goto out;
	}

	void *vko = ko;

	struct dyn_obj *dyn = CONTAINER_OF(vko, struct dyn_obj, kobj);

	__ASSERT(IS_PTR_ALIGNED(dyn, struct dyn_obj), "unaligned z_object");

	for (int i = 0; i < CONFIG_MAX_THREAD_BYTES; i++) {
		if (ko->perms[i] != 0U) {
			goto out;
		}
	}

	/* This object has no more references. Some objects may have
	 * dynamically allocated resources, require cleanup, or need to be
	 * marked as uninitialized when all references are gone. What
	 * specifically needs to happen depends on the object type.
	 */
	switch (ko->type) {
#ifdef CONFIG_PIPES
	case K_OBJ_PIPE:
		k_pipe_cleanup((struct k_pipe *)ko->name);
		break;
#endif
	case K_OBJ_MSGQ:
		k_msgq_cleanup((struct k_msgq *)ko->name);
		break;
	case K_OBJ_STACK:
		k_stack_cleanup((struct k_stack *)ko->name);
		break;
	default:
		/* Nothing to do */
		break;
	}

	sys_dlist_remove(&dyn->dobj_list);
	k_free(dyn->data);
	k_free(dyn);
out:
#endif
	k_spin_unlock(&obj_lock, key);
}

static void wordlist_cb(struct k_object *ko, void *ctx_ptr)
{
	struct perm_ctx *ctx = (struct perm_ctx *)ctx_ptr;

	if (sys_bitfield_test_bit((mem_addr_t)&ko->perms, ctx->parent_id) &&
				  (struct k_thread *)ko->name != ctx->parent) {
		sys_bitfield_set_bit((mem_addr_t)&ko->perms, ctx->child_id);
	}
}

void k_thread_perms_inherit(struct k_thread *parent, struct k_thread *child)
{
	struct perm_ctx ctx = {
		thread_index_get(parent),
		thread_index_get(child),
		parent
	};

	if ((ctx.parent_id != -1) && (ctx.child_id != -1)) {
		k_object_wordlist_foreach(wordlist_cb, &ctx);
	}
}

void k_thread_perms_set(struct k_object *ko, struct k_thread *thread)
{
	int index = thread_index_get(thread);

	if (index != -1) {
		sys_bitfield_set_bit((mem_addr_t)&ko->perms, index);
	}
}

void k_thread_perms_clear(struct k_object *ko, struct k_thread *thread)
{
	int index = thread_index_get(thread);

	if (index != -1) {
		sys_bitfield_clear_bit((mem_addr_t)&ko->perms, index);
		unref_check(ko, index);
	}
}

static void clear_perms_cb(struct k_object *ko, void *ctx_ptr)
{
	uintptr_t id = (uintptr_t)ctx_ptr;

	unref_check(ko, id);
}

void k_thread_perms_all_clear(struct k_thread *thread)
{
	uintptr_t index = thread_index_get(thread);

	if ((int)index != -1) {
		k_object_wordlist_foreach(clear_perms_cb, (void *)index);
	}
}

static int thread_perms_test(struct k_object *ko)
{
	int index;

	if ((ko->flags & K_OBJ_FLAG_PUBLIC) != 0U) {
		return 1;
	}

	index = thread_index_get(_current);
	if (index != -1) {
		return sys_bitfield_test_bit((mem_addr_t)&ko->perms, index);
	}
	return 0;
}

static void dump_permission_error(struct k_object *ko)
{
	int index = thread_index_get(_current);
	LOG_ERR("thread %p (%d) does not have permission on %s %p",
		_current, index,
		otype_to_str(ko->type), ko->name);
	LOG_HEXDUMP_ERR(ko->perms, sizeof(ko->perms), "permission bitmap");
}

void k_object_dump_error(int retval, const void *obj, struct k_object *ko,
			enum k_objects otype)
{
	switch (retval) {
	case -EBADF:
		LOG_ERR("%p is not a valid %s", obj, otype_to_str(otype));
		if (ko == NULL) {
			LOG_ERR("address is not a known kernel object");
		} else {
			LOG_ERR("address is actually a %s",
				otype_to_str(ko->type));
		}
		break;
	case -EPERM:
		dump_permission_error(ko);
		break;
	case -EINVAL:
		LOG_ERR("%p used before initialization", obj);
		break;
	case -EADDRINUSE:
		LOG_ERR("%p %s in use", obj, otype_to_str(otype));
		break;
	default:
		/* Not handled error */
		break;
	}
}

void z_impl_k_object_access_grant(const void *object, struct k_thread *thread)
{
	struct k_object *ko = k_object_find(object);

	if (ko != NULL) {
		k_thread_perms_set(ko, thread);
	}
}

void k_object_access_revoke(const void *object, struct k_thread *thread)
{
	struct k_object *ko = k_object_find(object);

	if (ko != NULL) {
		k_thread_perms_clear(ko, thread);
	}
}

void z_impl_k_object_release(const void *object)
{
	k_object_access_revoke(object, _current);
}

void k_object_access_all_grant(const void *object)
{
	struct k_object *ko = k_object_find(object);

	if (ko != NULL) {
		ko->flags |= K_OBJ_FLAG_PUBLIC;
	}
}

int k_object_validate(struct k_object *ko, enum k_objects otype,
		       enum _obj_init_check init)
{
	if (unlikely((ko == NULL) ||
		(otype != K_OBJ_ANY && ko->type != otype))) {
		return -EBADF;
	}

	/* Manipulation of any kernel objects by a user thread requires that
	 * thread be granted access first, even for uninitialized objects
	 */
	if (unlikely(thread_perms_test(ko) == 0)) {
		return -EPERM;
	}

	/* Initialization state checks. _OBJ_INIT_ANY, we don't care */
	if (likely(init == _OBJ_INIT_TRUE)) {
		/* Object MUST be initialized */
		if (unlikely((ko->flags & K_OBJ_FLAG_INITIALIZED) == 0U)) {
			return -EINVAL;
		}
	} else if (init == _OBJ_INIT_FALSE) { /* _OBJ_INIT_FALSE case */
		/* Object MUST NOT be initialized */
		if (unlikely((ko->flags & K_OBJ_FLAG_INITIALIZED) != 0U)) {
			return -EADDRINUSE;
		}
	} else {
		/* _OBJ_INIT_ANY */
	}

	return 0;
}

void k_object_init(const void *obj)
{
	struct k_object *ko;

	/* By the time we get here, if the caller was from userspace, all the
	 * necessary checks have been done in k_object_validate(), which takes
	 * place before the object is initialized.
	 *
	 * This function runs after the object has been initialized and
	 * finalizes it
	 */

	ko = k_object_find(obj);
	if (ko == NULL) {
		/* Supervisor threads can ignore rules about kernel objects
		 * and may declare them on stacks, etc. Such objects will never
		 * be usable from userspace, but we shouldn't explode.
		 */
		return;
	}

	/* Allows non-initialization system calls to be made on this object */
	ko->flags |= K_OBJ_FLAG_INITIALIZED;
}

void k_object_recycle(const void *obj)
{
	struct k_object *ko = k_object_find(obj);

	if (ko != NULL) {
		(void)memset(ko->perms, 0, sizeof(ko->perms));
		k_thread_perms_set(ko, _current);
		ko->flags |= K_OBJ_FLAG_INITIALIZED;
	}
}

void k_object_uninit(const void *obj)
{
	struct k_object *ko;

	/* See comments in k_object_init() */
	ko = k_object_find(obj);
	if (ko == NULL) {
		return;
	}

	ko->flags &= ~K_OBJ_FLAG_INITIALIZED;
}

/*
 * Copy to/from helper functions used in syscall handlers
 */
void *k_usermode_alloc_from_copy(const void *src, size_t size)
{
	void *dst = NULL;

	/* Does the caller in user mode have access to read this memory? */
	if (K_SYSCALL_MEMORY_READ(src, size)) {
		goto out_err;
	}

	dst = z_thread_malloc(size);
	if (dst == NULL) {
		LOG_ERR("out of thread resource pool memory (%zu)", size);
		goto out_err;
	}

	(void)memcpy(dst, src, size);
out_err:
	return dst;
}

static int user_copy(void *dst, const void *src, size_t size, bool to_user)
{
	int ret = EFAULT;

	/* Does the caller in user mode have access to this memory? */
	if (to_user ? K_SYSCALL_MEMORY_WRITE(dst, size) :
			K_SYSCALL_MEMORY_READ(src, size)) {
		goto out_err;
	}

	(void)memcpy(dst, src, size);
	ret = 0;
out_err:
	return ret;
}

int k_usermode_from_copy(void *dst, const void *src, size_t size)
{
	return user_copy(dst, src, size, false);
}

int k_usermode_to_copy(void *dst, const void *src, size_t size)
{
	return user_copy(dst, src, size, true);
}

char *k_usermode_string_alloc_copy(const char *src, size_t maxlen)
{
	size_t actual_len;
	int err;
	char *ret = NULL;

	actual_len = k_usermode_string_nlen(src, maxlen, &err);
	if (err != 0) {
		goto out;
	}
	if (actual_len == maxlen) {
		/* Not NULL terminated */
		LOG_ERR("string too long %p (%zu)", src, actual_len);
		goto out;
	}
	if (size_add_overflow(actual_len, 1, &actual_len)) {
		LOG_ERR("overflow");
		goto out;
	}

	ret = k_usermode_alloc_from_copy(src, actual_len);

	/* Someone may have modified the source string during the above
	 * checks. Ensure what we actually copied is still terminated
	 * properly.
	 */
	if (ret != NULL) {
		ret[actual_len - 1U] = '\0';
	}
out:
	return ret;
}

int k_usermode_string_copy(char *dst, const char *src, size_t maxlen)
{
	size_t actual_len;
	int ret, err;

	actual_len = k_usermode_string_nlen(src, maxlen, &err);
	if (err != 0) {
		ret = EFAULT;
		goto out;
	}
	if (actual_len == maxlen) {
		/* Not NULL terminated */
		LOG_ERR("string too long %p (%zu)", src, actual_len);
		ret = EINVAL;
		goto out;
	}
	if (size_add_overflow(actual_len, 1, &actual_len)) {
		LOG_ERR("overflow");
		ret = EINVAL;
		goto out;
	}

	ret = k_usermode_from_copy(dst, src, actual_len);

	/* See comment above in k_usermode_string_alloc_copy() */
	dst[actual_len - 1] = '\0';
out:
	return ret;
}

/*
 * Application memory region initialization
 */

extern char __app_shmem_regions_start[];
extern char __app_shmem_regions_end[];

static int app_shmem_bss_zero(void)
{
	struct z_app_region *region, *end;


	end = (struct z_app_region *)&__app_shmem_regions_end;
	region = (struct z_app_region *)&__app_shmem_regions_start;

	for ( ; region < end; region++) {
#if defined(CONFIG_DEMAND_PAGING) && !defined(CONFIG_LINKER_GENERIC_SECTIONS_PRESENT_AT_BOOT)
		/* When BSS sections are not present at boot, we need to wait for
		 * paging mechanism to be initialized before we can zero out BSS.
		 */
		extern bool z_sys_post_kernel;
		bool do_clear = z_sys_post_kernel;

		/* During pre-kernel init, z_sys_post_kernel == false, but
		 * with pinned rodata region, so clear. Otherwise skip.
		 * In post-kernel init, z_sys_post_kernel == true,
		 * skip those in pinned rodata region as they have already
		 * been cleared and possibly already in use. Otherwise clear.
		 */
		if (((uint8_t *)region->bss_start >= (uint8_t *)_app_smem_pinned_start) &&
		    ((uint8_t *)region->bss_start < (uint8_t *)_app_smem_pinned_end)) {
			do_clear = !do_clear;
		}

		if (do_clear)
#endif /* CONFIG_DEMAND_PAGING && !CONFIG_LINKER_GENERIC_SECTIONS_PRESENT_AT_BOOT */
		{
			(void)memset(region->bss_start, 0, region->bss_size);
		}
	}

	return 0;
}

SYS_INIT_NAMED(app_shmem_bss_zero_pre, app_shmem_bss_zero,
	       PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);

#if defined(CONFIG_DEMAND_PAGING) && !defined(CONFIG_LINKER_GENERIC_SECTIONS_PRESENT_AT_BOOT)
/* When BSS sections are not present at boot, we need to wait for
 * paging mechanism to be initialized before we can zero out BSS.
 */
SYS_INIT_NAMED(app_shmem_bss_zero_post, app_shmem_bss_zero,
	       POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
#endif /* CONFIG_DEMAND_PAGING && !CONFIG_LINKER_GENERIC_SECTIONS_PRESENT_AT_BOOT */

/*
 * Default handlers if otherwise unimplemented
 */

static uintptr_t handler_bad_syscall(uintptr_t bad_id, uintptr_t arg2,
				     uintptr_t arg3, uintptr_t arg4,
				     uintptr_t arg5, uintptr_t arg6,
				     void *ssf)
{
	LOG_ERR("Bad system call id %" PRIuPTR " invoked", bad_id);
	arch_syscall_oops(ssf);
	CODE_UNREACHABLE; /* LCOV_EXCL_LINE */
}

static uintptr_t handler_no_syscall(uintptr_t arg1, uintptr_t arg2,
				    uintptr_t arg3, uintptr_t arg4,
				    uintptr_t arg5, uintptr_t arg6, void *ssf)
{
	LOG_ERR("Unimplemented system call");
	arch_syscall_oops(ssf);
	CODE_UNREACHABLE; /* LCOV_EXCL_LINE */
}

#include <syscall_dispatch.c>
