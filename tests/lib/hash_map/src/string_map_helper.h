/*
 * Copyright (c) 2025 SecoMind
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef TEST_STRING_MAP_HELPER
#define TEST_STRING_MAP_HELPER

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <zephyr/sys/base64.h>
#include <zephyr/sys/hash_map.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/util.h>

/**
 * @brief Callback interface for @ref sys_hashmap
 *
 * This callback is used by some Hashmap methods.
 * This callback is used in maps which use a string (char *) as key
 *
 * @param key Key corresponding to @p value
 * @param value Value corresponding to @p key
 * @param cookie User-specified variable
 */
typedef void (*string_map_callback_t)(char *key, uint64_t value, void *cookie);

struct string_map_foreach_user_data {
	void *cookie;
	string_map_callback_t callback;
};

/*
 * Copyright (c) 2025 SecoMind
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "string_map_helper.h"

static inline bool string_map_get(const struct sys_hashmap *map, const char *key, uint64_t *value)
{
	return sys_hashmap_get(map, POINTER_TO_UINT(key), value);
}

static inline bool string_map_contains_key(const struct sys_hashmap *map, const char *key)
{
	return string_map_get(map, key, NULL);
}

static inline bool string_map_remove(struct sys_hashmap *map, const char *key, uint64_t *value,
				     char **old_key)
{
	uint64_t uint_old_key = 0;
	bool res = 0;

	res = sys_hashmap_remove(map, POINTER_TO_UINT(key), value, &uint_old_key);

	if (old_key && uint_old_key) {
		/* NOLINTNEXTLINE(performance-no-int-to-ptr) */
		*old_key = UINT_TO_POINTER(uint_old_key);
	}

	return res;
}

static inline int string_map_insert(struct sys_hashmap *map, const char *key, uint64_t value,
				    uint64_t *old_value,
				    char **old_key)
{
	uint64_t uint_old_key = 0;
	int res = 0;

	res = sys_hashmap_insert(map, POINTER_TO_UINT(key), POINTER_TO_UINT(value), old_value,
				 &uint_old_key);

	if (old_key && uint_old_key) {
		/* NOLINTNEXTLINE(performance-no-int-to-ptr) */
		*old_key = UINT_TO_POINTER(uint_old_key);
	}

	return res;
}

static inline void string_map_free_callback(uint64_t key, uint64_t value, void *cookie)
{
	/* NOLINTNEXTLINE(performance-no-int-to-ptr) */
	char *key_string = UINT_TO_POINTER(key);

	free(key_string);
}

static inline void string_map_callback(uint64_t key, uint64_t value, void *cookie)
{
	struct string_map_foreach_user_data *user_data =
		(struct string_map_foreach_user_data *) cookie;
	char *key_string = UINT_TO_POINTER(key);

	user_data->callback(key_string, value, user_data->cookie);
}

static inline void string_map_foreach(const struct sys_hashmap *map, string_map_callback_t cb,
				      void *cookie)
{
	struct string_map_foreach_user_data user_data = (struct string_map_foreach_user_data){
		.cookie = cookie,
		.callback = cb,
	};

	sys_hashmap_foreach(map, string_map_callback, &user_data);
}

static inline void string_map_clear(struct sys_hashmap *map, string_map_callback_t cb,
				    void *cookie)
{
	struct string_map_foreach_user_data user_data = (struct string_map_foreach_user_data) {
		.cookie = cookie,
		.callback = cb,
	};

	sys_hashmap_clear(map, string_map_callback, &user_data);
}

/* uses base64 encode to generate a unique NULL terminated string from a number */
static inline char *alloc_string_index(uint64_t key)
{
	size_t len = 0;

	base64_encode(NULL, 0, &len, (const uint8_t *) &key, sizeof(key));
	uint8_t *string_index = calloc(len, sizeof(uint8_t));

	(void) base64_encode(string_index, len, &len, (const uint8_t *) &key, sizeof(key));
	return string_index;
}

/* uses base64 decode to retrieve the number key */
static inline uint64_t get_key_index(char *key)
{
	size_t len = 0;
	uint64_t res = 0;

	(void) base64_decode((uint8_t *) &res, sizeof(uint64_t), &len, key, strlen(key));
	return res;
}

#endif /* TEST_STRING_MAP_HELPER */
