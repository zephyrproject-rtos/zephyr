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

#include <zephyr/sys/dlist.h>
#include <zephyr/sys/hash_map.h>
#include <zephyr/sys/hash_map_sc.h>
#include <zephyr/sys/util.h>

struct sys_hashmap_sc_entry {
	uint64_t key;
	uint64_t value;
	sys_dnode_t node;
};

static void sys_hashmap_sc_entry_init(struct sys_hashmap_sc_entry *entry, uint64_t key,
				      uint64_t value)
{
	entry->key = key;
	entry->value = value;
	sys_dnode_init(&entry->node);
}

static void sys_hashmap_sc_insert_entry(struct sys_hashmap *map, struct sys_hashmap_sc_entry *entry)
{
	sys_dlist_t *buckets = map->data->buckets;
	uint32_t hash = map->hash_func(&entry->key, sizeof(entry->key));

	sys_dlist_append(&buckets[hash % map->data->n_buckets], &entry->node);
	++map->data->size;
}

static void sys_hashmap_sc_insert_all(struct sys_hashmap *map, sys_dlist_t *list)
{
	__unused int ret;
	struct sys_hashmap_sc_entry *entry;

	while (!sys_dlist_is_empty(list)) {
		entry = CONTAINER_OF(sys_dlist_get(list), struct sys_hashmap_sc_entry, node);
		sys_hashmap_sc_insert_entry(map, entry);
	}
}

static void sys_hashmap_sc_to_list(struct sys_hashmap *map, sys_dlist_t *list)
{
	sys_dlist_t *bucket;
	struct sys_hashmap_sc_entry *entry;
	sys_dlist_t *buckets = map->data->buckets;

	sys_dlist_init(list);

	for (size_t i = 0; i < map->data->n_buckets; ++i) {
		bucket = &buckets[i];
		while (!sys_dlist_is_empty(bucket)) {
			entry = CONTAINER_OF(sys_dlist_get(bucket), struct sys_hashmap_sc_entry,
					     node);
			sys_dlist_append(list, &entry->node);
		}
	}
}

static int sys_hashmap_sc_rehash(struct sys_hashmap *map, bool grow)
{
	sys_dlist_t list;
	sys_dlist_t *bucket;
	size_t new_n_buckets;
	sys_dlist_t *new_buckets;

	if (!sys_hashmap_should_rehash(map, grow, 0, &new_n_buckets)) {
		return 0;
	}

	/* extract all entries from the hashmap */
	sys_hashmap_sc_to_list(map, &list);

	/* reallocate memory */
	new_buckets = (sys_dlist_t *)map->alloc_func(map->data->buckets,
						     new_n_buckets * sizeof(*new_buckets));
	if (new_buckets == NULL && new_n_buckets != 0) {
		/* re-insert all entries into the hashmap if reallocation fails */
		sys_hashmap_sc_insert_all(map, &list);
		return -ENOMEM;
	}

	/* ensure all buckets are empty / initialized */
	map->data->size = 0;
	map->data->buckets = new_buckets;
	map->data->n_buckets = new_n_buckets;
	for (size_t i = 0; i < new_n_buckets; ++i) {
		bucket = &((sys_dlist_t *)(map->data->buckets))[i];
		sys_dlist_init(bucket);
	}

	/* re-insert all entries into the hashmap */
	sys_hashmap_sc_insert_all(map, &list);

	return 0;
}

static struct sys_hashmap_sc_entry *sys_hashmap_sc_find(const struct sys_hashmap *map, uint64_t key)
{
	uint32_t hash;
	sys_dlist_t *bucket;
	sys_dlist_t *buckets;
	struct sys_hashmap_sc_entry *entry;

	if (map->data->n_buckets == 0) {
		return NULL;
	}

	__ASSERT_NO_MSG(map->data->size > 0);

	hash = map->hash_func(&key, sizeof(key));
	buckets = (sys_dlist_t *)map->data->buckets;
	bucket = &buckets[hash % map->data->n_buckets];

	SYS_DLIST_FOR_EACH_CONTAINER(bucket, entry, node) {
		if (entry->key == key) {
			return entry;
		}
	}

	return NULL;
}

static void sys_hashmap_sc_iter_next(struct sys_hashmap_iterator *it)
{
	sys_dlist_t *bucket;
	bool found_previous_key = false;
	struct sys_hashmap_sc_entry *entry;
	const struct sys_hashmap *map = it->map;
	sys_dlist_t *buckets = map->data->buckets;

	__ASSERT(it->size == map->data->size, "Concurrent modification!");
	__ASSERT(sys_hashmap_iterator_has_next(it), "Attempt to access beyond current bound!");

	if (it->pos == 0) {
		/* at position 0, state equals the beginning of the bucket array */
		it->state = buckets;
		found_previous_key = true;
	}

	for (bucket = it->state; bucket < &buckets[map->data->n_buckets]; ++bucket) {
		SYS_DLIST_FOR_EACH_CONTAINER(bucket, entry, node) {
			if (!found_previous_key) {
				if (entry->key == it->key) {
					found_previous_key = true;
				}

				continue;
			}

			/* save the bucket to state so we can restart scanning from a saved position
			 */
			it->state = bucket;
			it->key = entry->key;
			it->value = entry->value;
			++it->pos;

			return;
		}
	}

	__ASSERT(false, "Entire Hashmap traversed and no entry was found");
}

/*
 * Separate Chaining Hashmap API
 */

static void sys_hashmap_sc_iter(const struct sys_hashmap *map, struct sys_hashmap_iterator *it)
{
	it->map = map;
	it->next = sys_hashmap_sc_iter_next;
	it->state = map->data->buckets;
	it->key = 0;
	it->value = 0;
	it->pos = 0;
	*((size_t *)&it->size) = map->data->size;
}

static void sys_hashmap_sc_clear(struct sys_hashmap *map, sys_hashmap_callback_t cb, void *cookie)
{
	sys_dlist_t list;
	struct sys_hashmap_sc_entry *entry;

	sys_hashmap_sc_to_list(map, &list);

	/* free the buckets */
	if (map->data->buckets != NULL) {
		map->alloc_func(map->data->buckets, 0);
		map->data->buckets = NULL;
	}

	map->data->n_buckets = 0;
	map->data->size = 0;

	while (!sys_dlist_is_empty(&list)) {
		entry = CONTAINER_OF(sys_dlist_get(&list), struct sys_hashmap_sc_entry, node);

		/* call the callback for entry */
		if (cb != NULL) {
			cb(entry->key, entry->value, cookie);
		}

		/* free the entry using the Hashmap's allocator */
		map->alloc_func(entry, 0);
	}
}

static int sys_hashmap_sc_insert(struct sys_hashmap *map, uint64_t key, uint64_t value,
				 uint64_t *old_value)
{
	int ret;
	struct sys_hashmap_sc_entry *entry;

	entry = sys_hashmap_sc_find(map, key);
	if (entry != NULL) {
		if (old_value != NULL) {
			*old_value = entry->value;
		}

		entry->value = value;

		return 0;
	}

	ret = sys_hashmap_sc_rehash(map, true);
	if (ret < 0) {
		return ret;
	}

	entry = map->alloc_func(NULL, sizeof(*entry));
	if (entry == NULL) {
		return -ENOMEM;
	}

	sys_hashmap_sc_entry_init(entry, key, value);
	sys_hashmap_sc_insert_entry(map, entry);

	return 1;
}

static bool sys_hashmap_sc_remove(struct sys_hashmap *map, uint64_t key, uint64_t *value)
{
	__unused int ret;
	struct sys_hashmap_sc_entry *entry;

	entry = sys_hashmap_sc_find(map, key);
	if (entry == NULL) {
		return false;
	}

	if (value != NULL) {
		*value = entry->value;
	}

	sys_dlist_remove(&entry->node);
	--map->data->size;

	ret = sys_hashmap_sc_rehash(map, false);
	/* Realloc to a smaller size of memory should *always* work */
	__ASSERT_NO_MSG(ret >= 0);

	/* free the entry */
	map->alloc_func(entry, 0);

	return true;
}

static bool sys_hashmap_sc_get(const struct sys_hashmap *map, uint64_t key, uint64_t *value)
{
	struct sys_hashmap_sc_entry *entry;

	entry = sys_hashmap_sc_find(map, key);
	if (entry == NULL) {
		return false;
	}

	if (value != NULL) {
		*value = entry->value;
	}

	return true;
}

const struct sys_hashmap_api sys_hashmap_sc_api = {
	.iter = sys_hashmap_sc_iter,
	.clear = sys_hashmap_sc_clear,
	.insert = sys_hashmap_sc_insert,
	.remove = sys_hashmap_sc_remove,
	.get = sys_hashmap_sc_get,
};
