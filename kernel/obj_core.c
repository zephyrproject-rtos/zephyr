/*
 * Copyright (c) 2023, Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/kernel/obj_core.h>
#include <kernel_internal.h>

static struct k_spinlock  obj_core_lock;

sys_slist_t z_obj_type_list = SYS_SLIST_STATIC_INIT(&z_obj_type_list);

/* Registry of the objects registered at run time. It references the objects
 * and never stores anything inside them, so an object that is discarded
 * without being unregistered leaves a stale entry but cannot corrupt the
 * registry or any other object.
 */
struct obj_core_slot {
	struct k_obj_core *core;
	struct k_obj_type *type;
};

static struct obj_core_slot registry[CONFIG_OBJ_CORE_MAX_DYNAMIC_OBJECTS];

static bool range_contains(const struct k_obj_range *range, const void *ptr)
{
	if (range->indirect) {
		for (const void *const *pp = range->start; pp < (const void *const *)range->end;
		     pp = (const void *const *)((const uint8_t *)pp + range->stride)) {
			if (*pp == ptr) {
				return true;
			}
		}
		return false;
	}

	return (ptr >= range->start) && (ptr < range->end);
}

/* Object core of the range element */
static struct k_obj_core *range_core(const struct k_obj_type *type, const void *elem)
{
	const uint8_t *obj = type->statics.indirect ? *(const uint8_t *const *)elem : elem;

	return (struct k_obj_core *)(obj + type->obj_core_offset);
}

/* Objects in the running thread's stack or in the interrupt stack are
 * transient by construction. Called with the registry lock held, which pins
 * the current CPU. There is no current thread before the kernel has set one
 * up, and never with CONFIG_MULTITHREADING=n.
 */
static bool in_stack_storage(const void *ptr)
{
	uintptr_t addr = (uintptr_t)ptr;
	const struct k_thread *thread = _current;
	uintptr_t irq_stack = (uintptr_t)K_KERNEL_STACK_BUFFER(
		z_interrupt_stacks[_current_cpu->id]);
	uintptr_t irq_stack_end =
		irq_stack + K_KERNEL_STACK_SIZEOF(z_interrupt_stacks[0]);

	if ((thread != NULL) && (thread->stack_info.size != 0U) &&
	    (addr >= thread->stack_info.start) &&
	    (addr < thread->stack_info.start + thread->stack_info.size)) {
		return true;
	}

	return (addr >= irq_stack) && (addr < irq_stack_end);
}

static struct obj_core_slot *slot_find(const struct k_obj_core *core)
{
	for (size_t i = 0; i < ARRAY_SIZE(registry); i++) {
		if (registry[i].core == core) {
			return &registry[i];
		}
	}

	return NULL;
}

/* A registered object whose storage no longer carries its type is stale */
static bool slot_stale(const struct obj_core_slot *slot)
{
	return slot->core->type != slot->type;
}

static struct obj_core_slot *slot_alloc(void)
{
	struct obj_core_slot *slot = slot_find(NULL);

	if (slot != NULL) {
		return slot;
	}

	/* Full: reap every stale entry and reuse one of them */

	for (size_t i = 0; i < ARRAY_SIZE(registry); i++) {
		if (slot_stale(&registry[i])) {
			registry[i].core = NULL;
			slot = &registry[i];
		}
	}

	return slot;
}

struct k_obj_type *z_obj_type_init(struct k_obj_type *type,
				   uint32_t id, size_t off)
{
	sys_slist_append(&z_obj_type_list, &type->node);
	type->id = id;
	type->obj_core_offset = off;
	type->statics = (struct k_obj_range){ 0 };
	type->dropped = 0;
	type->skipped = 0;

	return type;
}

void z_obj_type_init_range(struct k_obj_type *type, const void *start,
			   const void *end, size_t stride, bool indirect)
{
	type->statics.start = start;
	type->statics.end = end;
	type->statics.stride = stride;
	type->statics.indirect = indirect;
}

void k_obj_core_init(struct k_obj_core *obj_core, struct k_obj_type *type)
{
	obj_core->type = type;
#ifdef CONFIG_OBJ_CORE_STATS
	obj_core->stats = NULL;
#endif /* CONFIG_OBJ_CORE_STATS */
}

void k_obj_core_link(struct k_obj_core *obj_core)
{
	struct k_obj_type *type = obj_core->type;
	struct obj_core_slot *slot;

	if (range_contains(&type->statics,
			   (const uint8_t *)obj_core - type->obj_core_offset)) {
		return;
	}

	k_spinlock_key_t key = k_spin_lock(&obj_core_lock);

	if (in_stack_storage(obj_core)) {
		type->skipped++;
		k_spin_unlock(&obj_core_lock, key);
		return;
	}

	slot = slot_find(obj_core);
	if (slot == NULL) {
		slot = slot_alloc();
	}

	if (slot == NULL) {
		type->dropped++;
	} else {
		slot->core = obj_core;
		slot->type = type;
	}

	k_spin_unlock(&obj_core_lock, key);
}

void k_obj_core_init_and_link(struct k_obj_core *obj_core,
			      struct k_obj_type *type)
{
	k_obj_core_init(obj_core, type);
	k_obj_core_link(obj_core);
}

void k_obj_core_unlink(struct k_obj_core *obj_core)
{
	k_spinlock_key_t  key = k_spin_lock(&obj_core_lock);
	struct obj_core_slot *slot = slot_find(obj_core);

	if (slot != NULL) {
		slot->core = NULL;
	}

	k_spin_unlock(&obj_core_lock, key);
}

void k_obj_core_evict_range(const void *addr, size_t len)
{
	const uint8_t *start = addr;
	const uint8_t *end = start + len;
	k_spinlock_key_t key = k_spin_lock(&obj_core_lock);

	for (size_t i = 0; i < ARRAY_SIZE(registry); i++) {
		const uint8_t *core = (const uint8_t *)registry[i].core;

		if ((core != NULL) && (core >= start) && (core < end)) {
			registry[i].core = NULL;
		}
	}

	k_spin_unlock(&obj_core_lock, key);
}

/* Add the object types defined at build time to the type list and initialize
 * the object cores of their permanent objects.
 */
static void z_obj_core_init_all(void)
{
	STRUCT_SECTION_FOREACH(k_obj_type, type) {
		const struct k_obj_range *range = &type->statics;

		sys_slist_append(&z_obj_type_list, &type->node);

		for (const uint8_t *obj = range->start;
		     obj < (const uint8_t *)range->end; obj += range->stride) {
			struct k_obj_core *obj_core =
				(struct k_obj_core *)(obj + type->obj_core_offset);

			k_obj_core_init(obj_core, type);
#ifdef CONFIG_OBJ_CORE_STATS
			if ((type->stats_desc != NULL) &&
			    (type->stats_size != 0)) {
				k_obj_core_stats_register(
					obj_core,
					(void *)(obj + type->stats_offset),
					type->stats_size);
			}
#endif /* CONFIG_OBJ_CORE_STATS */
		}
	}
}

K_KERNEL_INIT_PRE(z_obj_core_init_all);

struct k_obj_type *k_obj_type_find(uint32_t type_id)
{
	struct k_obj_type *rv = NULL;
	sys_snode_t *node;

	/* Types defined at build time are valid before the type list is */

	STRUCT_SECTION_FOREACH(k_obj_type, type) {
		if (type->id == type_id) {
			return type;
		}
	}

	k_spinlock_key_t  key = k_spin_lock(&obj_core_lock);

	SYS_SLIST_FOR_EACH_NODE(&z_obj_type_list, node) {
		struct k_obj_type *type = CONTAINER_OF(node, struct k_obj_type, node);

		if (type->id == type_id) {
			rv = type;
			break;
		}
	}

	k_spin_unlock(&obj_core_lock, key);

	return rv;
}

/* Invoke func on every permanent object of the type that carries its type
 * tag. Objects of a zero-initialized array that were never initialized do
 * not.
 */
static int walk_statics(struct k_obj_type *type,
			int (*func)(struct k_obj_core *obj_core, void *data),
			void *data)
{
	const struct k_obj_range *range = &type->statics;
	int status = 0;

	for (const uint8_t *elem = range->start; elem < (const uint8_t *)range->end;
	     elem += range->stride) {
		struct k_obj_core *obj_core = range_core(type, elem);

		if (obj_core->type != type) {
			continue;
		}

		status = func(obj_core, data);
		if (status != 0) {
			break;
		}
	}

	return status;
}

int k_obj_type_walk_locked(struct k_obj_type *type,
			   int (*func)(struct k_obj_core *obj_core, void *data),
			   void *data)
{
	k_spinlock_key_t  key;
	int  status;

	key = k_spin_lock(&obj_core_lock);

	status = walk_statics(type, func, data);

	for (size_t i = 0; (status == 0) && (i < ARRAY_SIZE(registry)); i++) {
		struct obj_core_slot *slot = &registry[i];

		if ((slot->core == NULL) || (slot->type != type)) {
			continue;
		}

		if (slot_stale(slot)) {
			slot->core = NULL;
			continue;
		}

		status = func(slot->core, data);
	}

	k_spin_unlock(&obj_core_lock, key);

	return status;
}

int k_obj_type_walk_unlocked(struct k_obj_type *type,
			   int (*func)(struct k_obj_core *obj_core, void *data),
			   void *data)
{
	int  status;

	status = walk_statics(type, func, data);

	for (size_t i = 0; (status == 0) && (i < ARRAY_SIZE(registry)); i++) {
		struct k_obj_core *core = registry[i].core;

		if ((core == NULL) || (registry[i].type != type) ||
		    (core->type != type)) {
			continue;
		}

		status = func(core, data);
	}

	return status;
}

#ifdef CONFIG_OBJ_CORE_STATS
int k_obj_core_stats_register(struct k_obj_core *obj_core, void *stats,
			      size_t stats_len)
{
	int rv;
	k_spinlock_key_t key = k_spin_lock(&obj_core_lock);

	if (obj_core->type->stats_desc == NULL) {
		/* Object type not configured for statistics. */
		rv = -ENOTSUP;
	} else if (obj_core->type->stats_desc->raw_size != stats_len) {
		/* Buffer size mismatch */
		rv = -EINVAL;
	} else {
		obj_core->stats = stats;
		rv = 0;
	}

	k_spin_unlock(&obj_core_lock, key);

	return rv;
}

int k_obj_core_stats_deregister(struct k_obj_core *obj_core)
{
	int rv;
	k_spinlock_key_t key = k_spin_lock(&obj_core_lock);

	if (obj_core->type->stats_desc == NULL) {
		/* Object type not configured  for statistics. */
		rv = -ENOTSUP;
	} else {
		obj_core->stats = NULL;
		rv = 0;
	}

	k_spin_unlock(&obj_core_lock, key);

	return rv;
}

int k_obj_core_stats_raw(struct k_obj_core *obj_core, void *stats,
			 size_t stats_len)
{
	int rv;
	struct k_obj_core_stats_desc *desc;

	k_spinlock_key_t key = k_spin_lock(&obj_core_lock);

	desc = obj_core->type->stats_desc;
	if ((desc == NULL) || (desc->raw == NULL)) {
		/* The object type is not configured for this operation */
		rv = -ENOTSUP;
	} else if ((desc->raw_size != stats_len) || (obj_core->stats == NULL)) {
		/*
		 * Either the size of the stats buffer is wrong or
		 * the kernel object was not registered for statistics.
		 */
		rv = -EINVAL;
	} else {
		rv = desc->raw(obj_core, stats);
	}

	k_spin_unlock(&obj_core_lock, key);

	return rv;
}

int k_obj_core_stats_query(struct k_obj_core *obj_core, void *stats,
			   size_t stats_len)
{
	int rv;
	struct k_obj_core_stats_desc *desc;

	k_spinlock_key_t key = k_spin_lock(&obj_core_lock);

	desc = obj_core->type->stats_desc;
	if ((desc == NULL) || (desc->query == NULL)) {
		/* The object type is not configured for this operation */
		rv = -ENOTSUP;
	} else if ((desc->query_size != stats_len) || (obj_core->stats == NULL)) {
		/*
		 * Either the size of the stats buffer is wrong or
		 * the kernel object was not registered for statistics.
		 */
		rv = -EINVAL;
	} else {
		rv = desc->query(obj_core, stats);
	}

	k_spin_unlock(&obj_core_lock, key);

	return rv;
}

int k_obj_core_stats_reset(struct k_obj_core *obj_core)
{
	int rv;
	struct k_obj_core_stats_desc *desc;

	k_spinlock_key_t  key = k_spin_lock(&obj_core_lock);

	desc = obj_core->type->stats_desc;
	if ((desc == NULL) || (desc->reset == NULL)) {
		/* The object type is not configured for this operation */
		rv = -ENOTSUP;
	} else if (obj_core->stats == NULL) {
		/* This kernel object is not configured for statistics */
		rv = -EINVAL;
	} else {
		rv = desc->reset(obj_core);
	}

	k_spin_unlock(&obj_core_lock, key);

	return rv;
}

int k_obj_core_stats_disable(struct k_obj_core *obj_core)
{
	int rv;
	struct k_obj_core_stats_desc *desc;

	k_spinlock_key_t key = k_spin_lock(&obj_core_lock);

	desc = obj_core->type->stats_desc;
	if ((desc == NULL) || (desc->disable == NULL)) {
		/* The object type is not configured for this operation */
		rv = -ENOTSUP;
	} else if (obj_core->stats == NULL) {
		/* This kernel object is not configured for statistics */
		rv = -EINVAL;
	} else {
		rv = desc->disable(obj_core);
	}

	k_spin_unlock(&obj_core_lock, key);

	return rv;
}

int k_obj_core_stats_enable(struct k_obj_core *obj_core)
{
	int rv;
	struct k_obj_core_stats_desc *desc;

	k_spinlock_key_t key = k_spin_lock(&obj_core_lock);

	desc = obj_core->type->stats_desc;
	if ((desc == NULL) || (desc->enable == NULL)) {
		/* The object type is not configured for this operation */
		rv = -ENOTSUP;
	} else if (obj_core->stats == NULL) {
		/* This kernel object is not configured for statistics */
		rv = -EINVAL;
	} else {
		rv = desc->enable(obj_core);
	}

	k_spin_unlock(&obj_core_lock, key);

	return rv;
}
#endif /* CONFIG_OBJ_CORE_STATS */

#ifdef CONFIG_OBJ_CORE_SYSTEM
static struct k_obj_type obj_type_kernel;

#ifdef CONFIG_OBJ_CORE_STATS_SYSTEM
static struct k_obj_core_stats_desc kernel_stats_desc = {
	.raw_size = sizeof(struct k_cycle_stats) * CONFIG_MP_MAX_NUM_CPUS,
	.query_size = sizeof(struct k_thread_runtime_stats),
	.raw   = z_kernel_stats_raw,
	.query = z_kernel_stats_query,
	.reset = NULL,
	.disable = NULL,
	.enable  = NULL,
};
#endif /* CONFIG_OBJ_CORE_STATS_SYSTEM */

/* The kernel object is a singleton (_kernel), so it is registered and linked
 * here directly rather than through the object type table.
 */
static void init_kernel_obj_core_list(void)
{
	/* Initialize kernel object type */

	z_obj_type_init(&obj_type_kernel, K_OBJ_TYPE_KERNEL_ID,
			offsetof(struct z_kernel, obj_core));

#ifdef CONFIG_OBJ_CORE_STATS_SYSTEM
	k_obj_type_stats_init(&obj_type_kernel, &kernel_stats_desc);
#endif /* CONFIG_OBJ_CORE_STATS_SYSTEM */

	k_obj_core_init_and_link(K_OBJ_CORE(&_kernel), &obj_type_kernel);
#ifdef CONFIG_OBJ_CORE_STATS_SYSTEM
	k_obj_core_stats_register(K_OBJ_CORE(&_kernel), _kernel.usage,
				  sizeof(_kernel.usage));
#endif /* CONFIG_OBJ_CORE_STATS_SYSTEM */
}

K_KERNEL_INIT_PRE(init_kernel_obj_core_list);
#endif /* CONFIG_OBJ_CORE_SYSTEM */
