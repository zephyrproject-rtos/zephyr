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

struct k_obj_type *z_obj_type_init(struct k_obj_type *type,
				   uint32_t id, size_t off)
{
	sys_slist_init(&type->list);
	sys_slist_append(&z_obj_type_list, &type->node);
	type->id = id;
	type->obj_core_offset = off;

	return type;
}

void k_obj_core_init(struct k_obj_core *obj_core, struct k_obj_type *type)
{
	obj_core->node.next = NULL;
	obj_core->type = type;
#ifdef CONFIG_OBJ_CORE_STATS
	obj_core->stats = NULL;
#endif /* CONFIG_OBJ_CORE_STATS */
}

void k_obj_core_link(struct k_obj_core *obj_core)
{
	k_spinlock_key_t  key = k_spin_lock(&obj_core_lock);

	sys_slist_append(&obj_core->type->list, &obj_core->node);

	k_spin_unlock(&obj_core_lock, key);
}

void k_obj_core_init_and_link(struct k_obj_core *obj_core,
			      struct k_obj_type *type)
{
	k_obj_core_init(obj_core, type);
	k_obj_core_link(obj_core);
}

static void z_obj_core_init_all(void)
{
	STRUCT_SECTION_FOREACH(k_obj_core_desc, desc) {
		z_obj_type_init(desc->type, desc->type_id,
				desc->obj_core_offset);
#ifdef CONFIG_OBJ_CORE_STATS
		if (desc->stats_desc != NULL) {
			k_obj_type_stats_init(desc->type, desc->stats_desc);
		}
#endif /* CONFIG_OBJ_CORE_STATS */

		/* Initialize and link every statically defined object */

		for (const uint8_t *obj = desc->objs_start;
		     obj < (const uint8_t *)desc->objs_end;
		     obj += desc->obj_size) {
			struct k_obj_core *obj_core =
				(struct k_obj_core *)(obj + desc->obj_core_offset);

			k_obj_core_init_and_link(obj_core, desc->type);
#ifdef CONFIG_OBJ_CORE_STATS
			if ((desc->stats_desc != NULL) &&
			    (desc->stats_size != 0)) {
				k_obj_core_stats_register(
					obj_core,
					(void *)(obj + desc->stats_offset),
					desc->stats_size);
			}
#endif /* CONFIG_OBJ_CORE_STATS */
		}
	}
}

K_KERNEL_INIT_PRE(z_obj_core_init_all);

void k_obj_core_unlink(struct k_obj_core *obj_core)
{
	k_spinlock_key_t  key = k_spin_lock(&obj_core_lock);

	sys_slist_find_and_remove(&obj_core->type->list, &obj_core->node);

	k_spin_unlock(&obj_core_lock, key);
}

struct k_obj_type *k_obj_type_find(uint32_t type_id)
{
	struct k_obj_type *type;
	struct k_obj_type *rv = NULL;
	sys_snode_t *node;

	k_spinlock_key_t  key = k_spin_lock(&obj_core_lock);

	SYS_SLIST_FOR_EACH_NODE(&z_obj_type_list, node) {
		type = CONTAINER_OF(node, struct k_obj_type, node);
		if (type->id == type_id) {
			rv = type;
			break;
		}
	}

	k_spin_unlock(&obj_core_lock, key);

	return rv;
}

int k_obj_type_walk_locked(struct k_obj_type *type,
			   int (*func)(struct k_obj_core *, void *),
			   void *data)
{
	k_spinlock_key_t  key;
	struct k_obj_core *obj_core;
	sys_snode_t *node;
	int  status = 0;

	key = k_spin_lock(&obj_core_lock);

	SYS_SLIST_FOR_EACH_NODE(&type->list, node) {
		obj_core = CONTAINER_OF(node, struct k_obj_core, node);
		status = func(obj_core, data);
		if (status != 0) {
			break;
		}
	}

	k_spin_unlock(&obj_core_lock, key);

	return status;
}

int k_obj_type_walk_unlocked(struct k_obj_type *type,
			   int (*func)(struct k_obj_core *, void *),
			   void *data)
{
	struct k_obj_core *obj_core;
	sys_snode_t *node;
	sys_snode_t *next;
	int  status = 0;

	SYS_SLIST_FOR_EACH_NODE_SAFE(&type->list, node, next) {
		obj_core = CONTAINER_OF(node, struct k_obj_core, node);
		status = func(obj_core, data);
		if (status != 0) {
			break;
		}
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
