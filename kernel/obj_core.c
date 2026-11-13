/*
 * Copyright (c) 2023, Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/kernel/obj_core.h>

static struct k_spinlock  lock;

sys_slist_t z_obj_type_list = SYS_SLIST_STATIC_INIT(&z_obj_type_list);

/* Registry of the objects registered at run time. It references the objects
 * and never stores anything inside them, so an object that is discarded
 * without being unregistered leaves a stale entry but cannot corrupt the
 * registry or any other object.
 */
struct obj_core_slot {
	atomic_ptr_t core;          /* struct k_obj_core *, NULL when free */
	struct k_obj_type *type;
};

/* Entries are allocated under the lock. They are released with an
 * atomic exchange, so unregistering and evicting never take the lock and
 * can run from any context, including a walk callback.
 */
static struct obj_core_slot registry[CONFIG_OBJ_CORE_MAX_DYNAMIC_OBJECTS];

static struct k_obj_core *slot_core(const struct obj_core_slot *slot)
{
	return atomic_ptr_get(&slot->core);
}

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

static struct obj_core_slot *slot_find(const struct k_obj_core *core)
{
	for (size_t i = 0; i < ARRAY_SIZE(registry); i++) {
		if (slot_core(&registry[i]) == core) {
			return &registry[i];
		}
	}

	return NULL;
}

/* A registered object whose storage no longer carries its type is stale.
 * The caller passes the object it read from the slot: the slot may be
 * cleared concurrently by an unregistration.
 */
static bool slot_stale(const struct obj_core_slot *slot, const struct k_obj_core *core)
{
	return core->type != slot->type;
}

static struct obj_core_slot *slot_alloc(void)
{
	struct obj_core_slot *slot = slot_find(NULL);

	if (slot != NULL) {
		return slot;
	}

	/* Full: reap every stale entry and reuse one of them */

	for (size_t i = 0; i < ARRAY_SIZE(registry); i++) {
		struct k_obj_core *core = slot_core(&registry[i]);

		if ((core != NULL) && slot_stale(&registry[i], core)) {
			atomic_ptr_set(&registry[i].core, NULL);
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

void k_obj_type_init_range(struct k_obj_type *type, const void *start,
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

	k_spinlock_key_t key = k_spin_lock(&lock);

	slot = slot_find(obj_core);
	if (slot == NULL) {
		slot = slot_alloc();
	}

	if (slot == NULL) {
		type->dropped++;
	} else {
		slot->type = type;
		atomic_ptr_set(&slot->core, obj_core);
	}

	k_spin_unlock(&lock, key);
}

void k_obj_core_init_and_link(struct k_obj_core *obj_core,
			      struct k_obj_type *type)
{
	k_obj_core_init(obj_core, type);
	k_obj_core_link(obj_core);
}

void k_obj_core_unlink(struct k_obj_core *obj_core)
{
	for (size_t i = 0; i < ARRAY_SIZE(registry); i++) {
		if (atomic_ptr_cas(&registry[i].core, obj_core, NULL)) {
			break;
		}
	}
}

void k_obj_core_evict_range(const void *addr, size_t len)
{
	const uint8_t *start = addr;
	const uint8_t *end = start + len;

	for (size_t i = 0; i < ARRAY_SIZE(registry); i++) {
		struct k_obj_core *core = slot_core(&registry[i]);

		if ((core != NULL) && ((const uint8_t *)core >= start) &&
		    ((const uint8_t *)core < end)) {
			(void)atomic_ptr_cas(&registry[i].core, core, NULL);
		}
	}
}

struct k_obj_type *k_obj_type_find(uint32_t type_id)
{
	struct k_obj_type *type;
	struct k_obj_type *rv = NULL;
	sys_snode_t *node;

	k_spinlock_key_t  key = k_spin_lock(&lock);

	SYS_SLIST_FOR_EACH_NODE(&z_obj_type_list, node) {
		type = CONTAINER_OF(node, struct k_obj_type, node);
		if (type->id == type_id) {
			rv = type;
			break;
		}
	}

	k_spin_unlock(&lock, key);

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
			   int (*func)(struct k_obj_core *, void *),
			   void *data)
{
	int  status;

	status = walk_statics(type, func, data);

	/* The lock covers reading and reaping each entry, not the callback,
	 * so the callback is free to use any kernel or object core function.
	 */
	for (size_t i = 0; (status == 0) && (i < ARRAY_SIZE(registry)); i++) {
		struct obj_core_slot *slot = &registry[i];
		k_spinlock_key_t key = k_spin_lock(&lock);
		struct k_obj_core *core = slot_core(slot);

		if ((core == NULL) || (slot->type != type)) {
			k_spin_unlock(&lock, key);
			continue;
		}

		if (slot_stale(slot, core)) {
			atomic_ptr_set(&slot->core, NULL);
			k_spin_unlock(&lock, key);
			continue;
		}

		k_spin_unlock(&lock, key);

		status = func(core, data);
	}

	return status;
}

int k_obj_type_walk_unlocked(struct k_obj_type *type,
			   int (*func)(struct k_obj_core *, void *),
			   void *data)
{
	int  status;

	status = walk_statics(type, func, data);

	for (size_t i = 0; (status == 0) && (i < ARRAY_SIZE(registry)); i++) {
		struct k_obj_core *core = slot_core(&registry[i]);

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
	k_spinlock_key_t key = k_spin_lock(&lock);

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

	k_spin_unlock(&lock, key);

	return rv;
}

int k_obj_core_stats_deregister(struct k_obj_core *obj_core)
{
	int rv;
	k_spinlock_key_t key = k_spin_lock(&lock);

	if (obj_core->type->stats_desc == NULL) {
		/* Object type not configured  for statistics. */
		rv = -ENOTSUP;
	} else {
		obj_core->stats = NULL;
		rv = 0;
	}

	k_spin_unlock(&lock, key);

	return rv;
}

int k_obj_core_stats_raw(struct k_obj_core *obj_core, void *stats,
			 size_t stats_len)
{
	int rv;
	struct k_obj_core_stats_desc *desc;

	k_spinlock_key_t key = k_spin_lock(&lock);

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

	k_spin_unlock(&lock, key);

	return rv;
}

int k_obj_core_stats_query(struct k_obj_core *obj_core, void *stats,
			   size_t stats_len)
{
	int rv;
	struct k_obj_core_stats_desc *desc;

	k_spinlock_key_t key = k_spin_lock(&lock);

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

	k_spin_unlock(&lock, key);

	return rv;
}

int k_obj_core_stats_reset(struct k_obj_core *obj_core)
{
	int rv;
	struct k_obj_core_stats_desc *desc;

	k_spinlock_key_t  key = k_spin_lock(&lock);

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

	k_spin_unlock(&lock, key);

	return rv;
}

int k_obj_core_stats_disable(struct k_obj_core *obj_core)
{
	int rv;
	struct k_obj_core_stats_desc *desc;

	k_spinlock_key_t key = k_spin_lock(&lock);

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

	k_spin_unlock(&lock, key);

	return rv;
}

int k_obj_core_stats_enable(struct k_obj_core *obj_core)
{
	int rv;
	struct k_obj_core_stats_desc *desc;

	k_spinlock_key_t key = k_spin_lock(&lock);

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

	k_spin_unlock(&lock, key);

	return rv;
}
#endif /* CONFIG_OBJ_CORE_STATS */
