/*
 * Copyright (c) 2022 Meta
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <memory>
#include <unordered_map>

#include <zephyr/sys/hash_map.h>
#include <zephyr/sys/hash_map_cxx.h>

using cxx_map = std::unordered_map<uint64_t, uint64_t>;

static void sys_hashmap_cxx_iter_next(struct sys_hashmap_iterator *it)
{
	cxx_map *umap = static_cast<cxx_map *>(it->map->data->buckets);

	__ASSERT_NO_MSG(umap != nullptr);

	__ASSERT(it->size == it->map->data->size, "Concurrent modification!");
	__ASSERT(sys_hashmap_iterator_has_next(it), "Attempt to access beyond current bound!");

	auto it2 = umap->begin();
	for (size_t i = 0; i < it->pos; ++i, it2++)
		;

	it->key = it2->first;
	it->value = it2->second;
	++it->pos;
}

/*
 * C++ Wrapped Hashmap API
 */

static void sys_hashmap_cxx_iter(const struct sys_hashmap *map, struct sys_hashmap_iterator *it)
{
	it->map = map;
	it->next = sys_hashmap_cxx_iter_next;
	it->state = nullptr;
	it->key = 0;
	it->value = 0;
	it->pos = 0;
	*((size_t *)&it->size) = map->data->size;
}

static void sys_hashmap_cxx_clear(struct sys_hashmap *map, sys_hashmap_callback_t cb, void *cookie)
{
	cxx_map *umap = static_cast<cxx_map *>(map->data->buckets);

	if (umap == nullptr) {
		return;
	}

	if (cb != nullptr) {
		for (auto &kv : *umap) {
			cb(kv.first, kv.second, cookie);
		}
	}

	delete umap;

	map->data->buckets = nullptr;
	map->data->n_buckets = 0;
	map->data->size = 0;
}

static int sys_hashmap_cxx_insert(struct sys_hashmap *map, uint64_t key, uint64_t value,
				  uint64_t *old_value)
{
	cxx_map *umap = static_cast<cxx_map *>(map->data->buckets);

	if (umap == nullptr) {
		umap = new cxx_map;
		umap->max_load_factor(map->config->load_factor / 100.0f);
		map->data->buckets = umap;
	}

	auto it = umap->find(key);
	if (it != umap->end() && old_value != nullptr) {
		*old_value = it->second;
		it->second = value;
		return 0;
	}

	try {
		(*umap)[key] = value;
	} catch(...) {
		return -ENOMEM;
	}

	++map->data->size;
	map->data->n_buckets = umap->bucket_count();

	return 1;
}

static bool sys_hashmap_cxx_remove(struct sys_hashmap *map, uint64_t key, uint64_t *value)
{
	cxx_map *umap = static_cast<cxx_map *>(map->data->buckets);

	if (umap == nullptr) {
		return false;
	}

	auto it = umap->find(key);
	if (it == umap->end()) {
		return false;
	}

	if (value != nullptr) {
		*value = it->second;
	}

	umap->erase(key);
	--map->data->size;
	map->data->n_buckets = umap->bucket_count();

	if (map->data->size == 0) {
		delete umap;
		map->data->n_buckets = 0;
		map->data->buckets = nullptr;
	}

	return true;
}

static bool sys_hashmap_cxx_get(const struct sys_hashmap *map, uint64_t key, uint64_t *value)
{
	cxx_map *umap = static_cast<cxx_map *>(map->data->buckets);

	if (umap == nullptr) {
		return false;
	}

	auto it = umap->find(key);
	if (it == umap->end()) {
		return false;
	}

	if (value != nullptr) {
		*value = it->second;
	}

	return true;
}

extern "C" {
const struct sys_hashmap_api sys_hashmap_cxx_api = {
	.iter = sys_hashmap_cxx_iter,
	.clear = sys_hashmap_cxx_clear,
	.insert = sys_hashmap_cxx_insert,
	.remove = sys_hashmap_cxx_remove,
	.get = sys_hashmap_cxx_get,
};
}
