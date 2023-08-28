/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "log_cache.h"

#define LOG_CACHE_DBG 0

#if LOG_CACHE_DBG
#include <sys/printk.h>
#define LOG_CACHE_PRINT(...) printk(__VA_ARGS__)
#define LOG_CACHE_DBG_ENTRY(str, entry) \
	printk(str " entry(%p) id %p\n", entry, (void *)entry->id)
#else
#define LOG_CACHE_PRINT(...)
#define LOG_CACHE_DBG_ENTRY(...)
#endif


int log_cache_init(struct log_cache *cache, const struct log_cache_config *config)
{
	sys_slist_init(&cache->active);
	sys_slist_init(&cache->idle);

	size_t entry_size = ROUND_UP(sizeof(struct log_cache_entry) + config->item_size,
				     sizeof(uintptr_t));
	uint32_t entry_cnt = config->buf_len / entry_size;
	struct log_cache_entry *entry = config->buf;

	/* Add all entries to idle list */
	for (uint32_t i = 0; i < entry_cnt; i++) {
		sys_slist_append(&cache->idle, &entry->node);
		entry = (struct log_cache_entry *)((uintptr_t)entry + entry_size);
	}

	cache->cmp = config->cmp;
	cache->item_size = config->item_size;
	cache->hit = 0;
	cache->miss = 0;

	return 0;
}

bool log_cache_get(struct log_cache *cache, uintptr_t id, uint8_t **data)
{
	sys_snode_t *prev_node = NULL;
	struct log_cache_entry *entry;
	bool hit = false;

	LOG_CACHE_PRINT("cache_get for id %lx\n", id);
	SYS_SLIST_FOR_EACH_CONTAINER(&cache->active, entry, node) {
		LOG_CACHE_DBG_ENTRY("checking", entry);
		if (cache->cmp(entry->id, id)) {
			cache->hit++;
			hit = true;
			break;
		}

		if (&entry->node == sys_slist_peek_tail(&cache->active)) {
			break;
		}
		prev_node = &entry->node;
	}

	if (hit) {
		LOG_CACHE_DBG_ENTRY("moving up", entry);
		sys_slist_remove(&cache->active, prev_node, &entry->node);
		sys_slist_prepend(&cache->active, &entry->node);
	} else {
		cache->miss++;

		sys_snode_t *from_idle = sys_slist_get(&cache->idle);

		if (from_idle) {
			entry = CONTAINER_OF(from_idle, struct log_cache_entry, node);
		} else {
			LOG_CACHE_DBG_ENTRY("removing", entry);
			sys_slist_remove(&cache->active, prev_node, &entry->node);
		}
	}

	*data = entry->data;
	entry->id = id;

	return hit;
}

void log_cache_put(struct log_cache *cache, uint8_t *data)
{
	struct log_cache_entry *entry = CONTAINER_OF(data, struct log_cache_entry, data[0]);

	LOG_CACHE_DBG_ENTRY("cache_put", entry);
	sys_slist_prepend(&cache->active, &entry->node);
}

void log_cache_release(struct log_cache *cache, uint8_t *data)
{
	struct log_cache_entry *entry = CONTAINER_OF(data, struct log_cache_entry, data[0]);

	LOG_CACHE_DBG_ENTRY("cache_release", entry);
	sys_slist_prepend(&cache->idle, &entry->node);
}
