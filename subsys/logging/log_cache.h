/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_SUBSYS_LOGGING_LOG_CACHE_H_
#define ZEPHYR_SUBSYS_LOGGING_LOG_CACHE_H_

#include <zephyr/sys/slist.h>

#ifdef __cplusplus
extern "C" {
#endif

struct log_cache_entry {
	sys_snode_t node;
	uintptr_t id;
	uint8_t data[];
};

typedef bool (*log_cache_cmp_func_t)(uintptr_t id0, uintptr_t id1);

struct log_cache_config {
	void *buf;
	size_t buf_len;
	size_t item_size;
	log_cache_cmp_func_t cmp;
};

struct log_cache {
	sys_slist_t active;
	sys_slist_t idle;
	log_cache_cmp_func_t cmp;
	uint32_t hit;
	uint32_t miss;
	size_t item_size;
};

int log_cache_init(struct log_cache *cache, const struct log_cache_config *config);

/** @brief Get cached entry or buffer to fill new data for caching.
 *
 * @param[in] cache Cache object.
 * @param[in] id Identication.
 * @param[out] data Location.
 *
 * @retval true if entry with given @p id was found and @p data contains pointer to it.
 * @retval false if entry was not found and @p data points to the empty location.
 */
bool log_cache_get(struct log_cache *cache, uintptr_t id, uint8_t **data);

/** @brief Put new entry into cache.
 *
 * @param cache Cache object.
 * @param data Buffer filled with new data. It must point to the location returned by
 *	       @ref log_cache_get.
 */
void log_cache_put(struct log_cache *cache, uint8_t *data);

/** @brief Release entry.
 *
 * Release entry that was allocated by @ref log_cache_get when requested id was
 * not found in the cache. Releasing puts entry in idle list.
 *
 * @param cache Cache object.
 * @param data It must point to the location returned by @ref log_cache_get.
 */
void log_cache_release(struct log_cache *cache, uint8_t *data);

/** @brief Get hit count.
 *
 * @param cache Cache object.
 *
 * @return Number of hits.
 */
static inline uint32_t log_cache_get_hit(struct log_cache *cache)
{
	return cache->hit;
}

/** @brief Get miss count.
 *
 * @param cache Cache object.
 *
 * @return Number of misses.
 */
static inline uint32_t log_cache_get_miss(struct log_cache *cache)
{
	return cache->miss;
}

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_SUBSYS_LOGGING_LOG_CACHE_H_ */
