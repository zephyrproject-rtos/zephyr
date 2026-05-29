/*
 * Copyright (c) 2022 Meta
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_SYS_HASH_FUNCTION_H_
#define ZEPHYR_INCLUDE_SYS_HASH_FUNCTION_H_

#include <stddef.h>
#include <stdint.h>

#include <zephyr/sys/__assert.h>
#include <zephyr/sys/util_macro.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @ingroup hashmap_apis
 * @defgroup hash_functions Hash Functions
 * @{
 */

/**
 * @brief 32-bit Hash function interface
 *
 * Hash functions are used to map data from an arbitrarily large space to a
 * (typically smaller) fixed-size space. For a given input, a hash function will
 * consistently generate the same, semi-unique numerical value. Even for
 * marginally different data, a good hash function will distribute the entropy
 * almost evenly over all bits in the hashed value when combined with modulo
 * arithmetic over a finite-sized numeric field.
 *
 * @param str a string of input data
 * @param n the number of bytes in @p str
 *
 * @return the numeric hash associated with @p str
 */
typedef uint32_t (*sys_hash_func32_t)(const void *str, size_t n);

/**
 * @brief The naive identity hash function
 *
 * This hash function requires that @p n is equal to the size of a primitive
 * type, such as `[u]int8_t`, `[u]int16_t`, `[u]int32_t`, `[u]int64_t`,
 * `float`, `double`, or `void *`, and that the alignment of @p str agrees
 * with that of the respective native type.
 *
 * @note The identity hash function is used for testing @ref sys_hashmap.
 *
 * @param str a string of input data
 * @param n the number of bytes in @p str
 *
 * @return the numeric hash associated with @p str
 */
static inline uint32_t sys_hash32_identity(const void *str, size_t n)
{
	switch (n) {
	case sizeof(uint8_t):
		return *(uint8_t *)str;
	case sizeof(uint16_t):
		return *(uint16_t *)str;
	case sizeof(uint32_t):
		return *(uint32_t *)str;
	case sizeof(uint64_t):
		return (uint32_t)(*(uint64_t *)str);
	default:
		break;
	}

	__ASSERT(false, "invalid str length %zu", n);

	return 0;
}

/**
 * @brief Daniel J.\ Bernstein's hash function
 *
 * Some notes:
 * - normally, this hash function is used on NUL-terminated strings
 * - it has been modified to support arbitrary sequences of bytes
 * - it has been modified to use XOR rather than addition
 *
 * @param str a string of input data
 * @param n the number of bytes in @p str
 *
 * @return the numeric hash associated with @p str
 *
 * @note enable with @kconfig{CONFIG_SYS_HASH_FUNC32_DJB2}
 *
 * @see https://theartincode.stanis.me/008-djb2/
 */
uint32_t sys_hash32_djb2(const void *str, size_t n);

/**
 * @brief Murmur3 hash function
 *
 * @param str a string of input data
 * @param n the number of bytes in @p str
 *
 * @return the numeric hash associated with @p str
 *
 * @note enable with @kconfig{CONFIG_SYS_HASH_FUNC32_MURMUR3}
 *
 * @see https://en.wikipedia.org/wiki/MurmurHash
 */
uint32_t sys_hash32_murmur3(const void *str, size_t n);

/**
 * @brief System default 32-bit hash function
 *
 * @param str a string of input data
 * @param n the number of bytes in @p str
 *
 * @return the numeric hash associated with @p str
 */
static inline uint32_t sys_hash32(const void *str, size_t n)
{
	if (IS_ENABLED(CONFIG_SYS_HASH_FUNC32_CHOICE_IDENTITY)) {
		return sys_hash32_identity(str, n);
	}

	if (IS_ENABLED(CONFIG_SYS_HASH_FUNC32_CHOICE_DJB2)) {
		return sys_hash32_djb2(str, n);
	}

	if (IS_ENABLED(CONFIG_SYS_HASH_FUNC32_CHOICE_MURMUR3)) {
		return sys_hash32_murmur3(str, n);
	}

	__ASSERT(0, "No default 32-bit hash. See CONFIG_SYS_HASH_FUNC32_CHOICE");

	return 0;
}

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_SYS_HASH_FUNCTION_H_ */
