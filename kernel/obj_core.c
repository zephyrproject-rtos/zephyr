/*
 * Copyright (c) 2023, Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/kernel/obj_core.h>

static struct k_spinlock  lock;

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
#endif
}

void k_obj_core_link(struct k_obj_core *obj_core)
{
	k_spinlock_key_t  key = k_spin_lock(&lock);

	sys_slist_append(&obj_core->type->list, &obj_core->node);

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
	k_spinlock_key_t  key = k_spin_lock(&lock);

	sys_slist_find_and_remove(&obj_core->type->list, &obj_core->node);

	k_spin_unlock(&lock, key);
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

int k_obj_type_walk_locked(struct k_obj_type *type,
			   int (*func)(struct k_obj_core *, void *),
			   void *data)
{
	k_spinlock_key_t  key;
	struct k_obj_core *obj_core;
	sys_snode_t *node;
	int  status = 0;

	key = k_spin_lock(&lock);

	SYS_SLIST_FOR_EACH_NODE(&type->list, node) {
		obj_core = CONTAINER_OF(node, struct k_obj_core, node);
		status = func(obj_core, data);
		if (status != 0) {
			break;
		}
	}

	k_spin_unlock(&lock, key);

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
	k_spinlock_key_t  key = k_spin_lock(&lock);

	if (obj_core->type->stats_desc == NULL) {

		/* Object type not configured for statistics. */

		k_spin_unlock(&lock, key);
		return -ENOTSUP;
	}

	if (obj_core->type->stats_desc->raw_size != stats_len) {

		/* Buffer size mismatch */

		k_spin_unlock(&lock, key);
		return -EINVAL;
	}

	obj_core->stats = stats;
	k_spin_unlock(&lock, key);

	return 0;
}

int k_obj_core_stats_deregister(struct k_obj_core *obj_core)
{
	k_spinlock_key_t  key = k_spin_lock(&lock);

	if (obj_core->type->stats_desc == NULL) {

		/* Object type not configured  for statistics. */

		k_spin_unlock(&lock, key);
		return -ENOTSUP;
	}

	obj_core->stats = NULL;
	k_spin_unlock(&lock, key);

	return 0;
}

int k_obj_core_stats_raw(struct k_obj_core *obj_core, void *stats,
			 size_t stats_len)
{
	int  rv;
	struct k_obj_core_stats_desc *desc;

	k_spinlock_key_t  key = k_spin_lock(&lock);

	desc = obj_core->type->stats_desc;
	if ((desc == NULL) || (desc->raw == NULL)) {

		/* The object type is not configured for this operation */

		k_spin_unlock(&lock, key);
		return -ENOTSUP;
	}

	if ((desc->raw_size != stats_len) || (obj_core->stats == NULL)) {

		/*
		 * Either the size of the stats buffer is wrong or
		 * the kernel object was not registered for statistics.
		 */

		k_spin_unlock(&lock, key);
		return -EINVAL;
	}

	rv = desc->raw(obj_core, stats);
	k_spin_unlock(&lock, key);

	return rv;
}

int k_obj_core_stats_query(struct k_obj_core *obj_core, void *stats,
			   size_t stats_len)
{
	int  rv;
	struct k_obj_core_stats_desc *desc;

	k_spinlock_key_t  key = k_spin_lock(&lock);

	desc = obj_core->type->stats_desc;
	if ((desc == NULL) || (desc->query == NULL)) {

		/* The object type is not configured for this operation */

		k_spin_unlock(&lock, key);
		return -ENOTSUP;
	}

	if ((desc->query_size != stats_len) || (obj_core->stats == NULL)) {

		/*
		 * Either the size of the stats buffer is wrong or
		 * the kernel object was not registered for statistics.
		 */

		k_spin_unlock(&lock, key);
		return -EINVAL;
	}

	rv = desc->query(obj_core, stats);
	k_spin_unlock(&lock, key);

	return rv;
}

int k_obj_core_stats_reset(struct k_obj_core *obj_core)
{
	int  rv;
	struct k_obj_core_stats_desc *desc;

	k_spinlock_key_t  key = k_spin_lock(&lock);

	desc = obj_core->type->stats_desc;
	if ((desc == NULL) || (desc->reset == NULL)) {

		/* The object type is not configured for this operation */

		k_spin_unlock(&lock, key);
		return -ENOTSUP;
	}

	if (obj_core->stats == NULL) {

		/* This kernel object is not configured for statistics */

		k_spin_unlock(&lock, key);
		return -EINVAL;
	}

	rv = desc->reset(obj_core);
	k_spin_unlock(&lock, key);

	return rv;
}

int k_obj_core_stats_disable(struct k_obj_core *obj_core)
{
	int  rv;
	struct k_obj_core_stats_desc *desc;

	k_spinlock_key_t  key = k_spin_lock(&lock);

	desc = obj_core->type->stats_desc;
	if ((desc == NULL) || (desc->disable == NULL)) {

		/* The object type is not configured for this operation */

		k_spin_unlock(&lock, key);
		return -ENOTSUP;
	}

	if (obj_core->stats == NULL) {

		/* This kernel object is not configured for statistics */

		k_spin_unlock(&lock, key);
		return -EINVAL;
	}

	rv = desc->disable(obj_core);
	k_spin_unlock(&lock, key);

	return rv;
}

int k_obj_core_stats_enable(struct k_obj_core *obj_core)
{
	int  rv;
	struct k_obj_core_stats_desc *desc;

	k_spinlock_key_t  key = k_spin_lock(&lock);

	desc = obj_core->type->stats_desc;
	if ((desc == NULL) || (desc->enable == NULL)) {

		/* The object type is not configured for this operation */

		k_spin_unlock(&lock, key);
		return -ENOTSUP;
	}

	if (obj_core->stats == NULL) {

		/* This kernel object is not configured for statistics */

		k_spin_unlock(&lock, key);
		return -EINVAL;
	}

	rv = desc->enable(obj_core);
	k_spin_unlock(&lock, key);

	return rv;
}
#endif
