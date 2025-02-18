/*
 * Copyright (c) 2025 SecoMind
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "_main.h"

#include <zephyr/sys/base64.h>
#include <zephyr/sys/hash_map.h>

#include <string.h>

uint32_t hash_string(const void *uint64_ptr_key, size_t len)
{
	__ASSERT(len == sizeof(uint64_t),
		 "Received an invalid length for the hasmap key, expected only uint64_t");

	uint64_t key_pointer = {0};

	memcpy(&key_pointer, uint64_ptr_key, sizeof(uint64_t));

	const char *key_string = UINT_TO_POINTER(key_pointer);
	size_t key_string_len = strlen(key_string);

	return sys_hash32(key_string, key_string_len);
}

bool eq_string(uint64_t key_left, uint64_t key_right)
{
	if (key_left == key_right) {
		return true;
	} else if (key_left == 0 || key_right == 0) {
		return false;
	}

	const char *key_left_string = UINT_TO_POINTER(key_left);
	const char *key_right_string = UINT_TO_POINTER(key_right);

	return strcmp(key_left_string, key_right_string) == 0;
}
