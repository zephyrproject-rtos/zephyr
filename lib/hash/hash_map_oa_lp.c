/*
 * Copyright (c) 2022 Meta
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <zephyr/sys/hash_map.h>
#include <zephyr/sys/hash_map_oa_lp.h>
#include <zephyr/sys/util.h>

enum bucket_state {
	UNUSED,
	USED,
	TOMBSTONE,
};

struct oalp_entry {
	uint64_t key;
	uint64_t value;
	enum bucket_state state;
};

BUILD_ASSERT(offsetof(struct sys_hashmap_oa_lp_data, buckets) ==
	     offsetof(struct sys_hashmap_data, buckets));
BUILD_ASSERT(offsetof(struct sys_hashmap_oa_lp_data, n_buckets) ==
	     offsetof(struct sys_hashmap_data, n_buckets));
BUILD_ASSERT(offsetof(struct sys_hashmap_oa_lp_data, size) ==
	     offsetof(struct sys_hashmap_data, size));

static struct oalp_entry *sys_hashmap_oa_lp_find(const struct sys_hashmap *map, uint64_t key,
						 bool used_ok, bool unused_ok, bool tombstone_ok)
{
	struct oalp_entry *entry = NULL;
	const size_t n_buckets = map->data->n_buckets;
	uint32_t hash = map->hash_func(&key, sizeof(key));
	struct oalp_entry *const buckets = map->data->buckets;

	for (size_t i = 0, j = hash; i < n_buckets; ++i, ++j) {
		j &= (n_buckets - 1);
		__ASSERT_NO_MSG(j < n_buckets);

		entry = &buckets[j];

		switch (entry->state) {
		case USED:
			if (used_ok && entry->key == key) {
				return entry;
			}
			break;
		case UNUSED:
			if (unused_ok) {
				return entry;
			}
			break;
		case TOMBSTONE:
			if (tombstone_ok) {
				return entry;
			}
			break;
		default:
			__ASSERT(false, "Invalid entry state. Memory has been corrupted");
			break;
		}
	}

	return NULL;
}

static int sys_hashmap_oa_lp_insert_no_rehash(struct sys_hashmap *map, uint64_t key, uint64_t value,
					      uint64_t *old_value)
{
	int ret;
	struct oalp_entry *entry = NULL;
	struct sys_hashmap_oa_lp_data *data = (struct sys_hashmap_oa_lp_data *)map->data;

	entry = sys_hashmap_oa_lp_find(map, key, true, true, true);
	__ASSERT_NO_MSG(entry != NULL);

	switch (entry->state) {
	case UNUSED:
		++data->size;
		ret = 1;
		break;
	case TOMBSTONE:
		--data->n_tombstones;
		++data->size;
		ret = 0;
		break;
	case USED:
	default:
		ret = 0;
		break;
	}

	if (old_value != NULL) {
		*old_value = entry->value;
	}

	entry->state = USED;
	entry->key = key;
	entry->value = value;

	return ret;
}

static int sys_hashmap_oa_lp_rehash(struct sys_hashmap *map, bool grow)
{
	size_t old_size;
	size_t old_n_buckets;
	size_t new_n_buckets = 0;
	struct oalp_entry *entry;
	struct oalp_entry *old_buckets;
	struct oalp_entry *new_buckets;
	struct sys_hashmap_oa_lp_data *data = (struct sys_hashmap_oa_lp_data *)map->data;

	if (!sys_hashmap_should_rehash(map, grow, data->n_tombstones, &new_n_buckets)) {
		return 0;
	}

	if (map->data->size != SIZE_MAX && map->data->size == map->config->max_size) {
		return -ENOSPC;
	}

	/* extract all entries from the hashmap */
	old_size = data->size;
	old_n_buckets = data->n_buckets;
	old_buckets = (struct oalp_entry *)data->buckets;

	new_buckets = (struct oalp_entry *)map->alloc_func(NULL, new_n_buckets * sizeof(*entry));
	if (new_buckets == NULL && new_n_buckets != 0) {
		return -ENOMEM;
	}

	if (new_buckets != NULL) {
		/* ensure all buckets are empty / initialized */
		memset(new_buckets, 0, new_n_buckets * sizeof(*new_buckets));
	}

	data->size = 0;
	data->buckets = new_buckets;
	data->n_buckets = new_n_buckets;

	/* re-insert all entries into the hashmap */
	for (size_t i = 0, j = 0; i < old_n_buckets && j < old_size; ++i) {
		entry = &old_buckets[i];

		if (entry->state == USED) {
			sys_hashmap_oa_lp_insert_no_rehash(map, entry->key, entry->value, NULL);
			++j;
		}
	}

	/* free the old Hashmap */
	map->alloc_func(old_buckets, 0);

	return 0;
}

static void sys_hashmap_oa_lp_iter_next(struct sys_hashmap_iterator *it)
{
	size_t i;
	struct oalp_entry *entry;
	const struct sys_hashmap *map = (const struct sys_hashmap *)it->map;
	struct oalp_entry *buckets = map->data->buckets;

	__ASSERT(it->size == map->data->size, "Concurrent modification!");
	__ASSERT(sys_hashmap_iterator_has_next(it), "Attempt to access beyond current bound!");

	if (it->pos == 0) {
		it->state = buckets;
	}

	i = (struct oalp_entry *)it->state - buckets;
	__ASSERT(i < map->data->n_buckets, "Invalid iterator state %p", it->state);

	for (; i < map->data->n_buckets; ++i) {
		entry = &buckets[i];
		if (entry->state == USED) {
			it->state = &buckets[i + 1];
			it->key = entry->key;
			it->value = entry->value;
			++it->pos;
			return;
		}
	}

	__ASSERT(false, "Entire Hashmap traversed and no entry was found");
}

/*
 * Open Addressing / Linear Probe Hashmap API
 */

static void sys_hashmap_oa_lp_iter(const struct sys_hashmap *map, struct sys_hashmap_iterator *it)
{
	it->map = map;
	it->next = sys_hashmap_oa_lp_iter_next;
	it->pos = 0;
	*((size_t *)&it->size) = map->data->size;
}

static void sys_hashmap_oa_lp_clear(struct sys_hashmap *map, sys_hashmap_callback_t cb,
				    void *cookie)
{
	struct oalp_entry *entry;
	struct sys_hashmap_oa_lp_data *data = (struct sys_hashmap_oa_lp_data *)map->data;
	struct oalp_entry *buckets = data->buckets;

	for (size_t i = 0, j = 0; cb != NULL && i < data->n_buckets && j < data->size; ++i) {
		entry = &buckets[i];
		if (entry->state == USED) {
			cb(entry->key, entry->value, cookie);
			++j;
		}
	}

	if (data->buckets != NULL) {
		map->alloc_func(data->buckets, 0);
		data->buckets = NULL;
	}

	data->n_buckets = 0;
	data->size = 0;
	data->n_tombstones = 0;
}

static inline int sys_hashmap_oa_lp_insert(struct sys_hashmap *map, uint64_t key, uint64_t value,
					   uint64_t *old_value)
{
	int ret;

	ret = sys_hashmap_oa_lp_rehash(map, true);
	if (ret < 0) {
		return ret;
	}

	return sys_hashmap_oa_lp_insert_no_rehash(map, key, value, old_value);
}

static bool sys_hashmap_oa_lp_remove(struct sys_hashmap *map, uint64_t key, uint64_t *value)
{
	struct oalp_entry *entry;
	struct sys_hashmap_oa_lp_data *data = (struct sys_hashmap_oa_lp_data *)map->data;

	entry = sys_hashmap_oa_lp_find(map, key, true, true, false);
	if (entry == NULL || entry->state == UNUSED) {
		return false;
	}

	if (value != NULL) {
		*value = entry->value;
	}

	entry->state = TOMBSTONE;
	--data->size;
	++data->n_tombstones;

	/* ignore a possible -ENOMEM since the table will remain intact */
	(void)sys_hashmap_oa_lp_rehash(map, false);

	return true;
}

static bool sys_hashmap_oa_lp_get(const struct sys_hashmap *map, uint64_t key, uint64_t *value)
{
	struct oalp_entry *entry;

	entry = sys_hashmap_oa_lp_find(map, key, true, true, false);
	if (entry == NULL || entry->state == UNUSED) {
		return false;
	}

	if (value != NULL) {
		*value = entry->value;
	}

	return true;
}

const struct sys_hashmap_api sys_hashmap_oa_lp_api = {
	.iter = sys_hashmap_oa_lp_iter,
	.clear = sys_hashmap_oa_lp_clear,
	.insert = sys_hashmap_oa_lp_insert,
	.remove = sys_hashmap_oa_lp_remove,
	.get = sys_hashmap_oa_lp_get,
};
