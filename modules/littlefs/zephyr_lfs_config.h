/*
 * Copyright (c) 2017, Arm Limited. All rights reserved.
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Origin of this file is the LittleFS lfs_util.h file adjusted for Zephyr
 * specific needs.
 */

#ifndef ZEPHYR_LFS_CONFIG_H
#define ZEPHYR_LFS_CONFIG_H

/* System includes */
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <inttypes.h>

#ifndef LFS_NO_MALLOC
#include <stdlib.h>
#endif
#ifndef LFS_NO_ASSERT
#include <zephyr/sys/__assert.h>
#endif

#if !defined(LFS_NO_DEBUG) || \
	    !defined(LFS_NO_WARN) || \
	    !defined(LFS_NO_ERROR) || \
	    defined(LFS_YES_TRACE)

#include <zephyr/logging/log.h>

#ifdef LFS_LOG_REGISTER
LOG_MODULE_REGISTER(littlefs, CONFIG_FS_LOG_LEVEL);
#endif

#include <stdio.h>
#endif

#ifdef __cplusplus
extern "C"
{
#endif

#ifdef CONFIG_FS_LITTLEFS_DISK_VERSION
#define LFS_MULTIVERSION
#endif

/* Logging functions when using LittleFS with Zephyr. */
#ifndef LFS_TRACE
#ifdef LFS_YES_TRACE
#define LFS_TRACE(fmt, ...) LOG_DBG("%s:%d:trace: " fmt, __FILE__, __LINE__, ##__VA_ARGS__)
#else
#define LFS_TRACE(...)
#endif
#endif

#ifndef LFS_DEBUG
#define LFS_DEBUG(fmt, ...) LOG_DBG("%s:%d: " fmt, __FILE__, __LINE__, ##__VA_ARGS__)
#endif

#ifndef LFS_WARN
#define LFS_WARN(fmt, ...) LOG_WRN("%s:%d: " fmt, __FILE__, __LINE__, ##__VA_ARGS__)
#endif

#ifndef LFS_ERROR
#define LFS_ERROR(fmt, ...) LOG_ERR("%s:%d: " fmt, __FILE__, __LINE__, ##__VA_ARGS__)
#endif

/* Runtime assertions */
#ifndef LFS_ASSERT
#define LFS_ASSERT(test) __ASSERT_NO_MSG(test)
#endif


/* Builtin functions, these may be replaced by more efficient */
/* toolchain-specific implementations. LFS_NO_INTRINSICS falls back to a more */
/* expensive basic C implementation for debugging purposes */

/* Min/max functions for unsigned 32-bit numbers */
static inline uint32_t lfs_max(uint32_t a, uint32_t b)
{
	return (a > b) ? a : b;
}

static inline uint32_t lfs_min(uint32_t a, uint32_t b)
{
	return (a < b) ? a : b;
}

/* Align to nearest multiple of a size */
static inline uint32_t lfs_aligndown(uint32_t a, uint32_t alignment)
{
	return a - (a % alignment);
}

static inline uint32_t lfs_alignup(uint32_t a, uint32_t alignment)
{
	return lfs_aligndown(a + alignment-1, alignment);
}

/* Find the smallest power of 2 greater than or equal to a */
static inline uint32_t lfs_npw2(uint32_t a)
{
#if !defined(LFS_NO_INTRINSICS) && (defined(__GNUC__) || defined(__CC_ARM))
	return 32 - __builtin_clz(a-1);
#else
	uint32_t r = 0;
	uint32_t s;

	a -= 1;
	s = (a > 0xffff) << 4; a >>= s; r |= s;
	s = (a > 0xff) << 3; a >>= s; r |= s;
	s = (a > 0xf) << 2; a >>= s; r |= s;
	s = (a > 0x3) << 1; a >>= s; r |= s;
	return (r | (a >> 1)) + 1;
#endif
}

/* Count the number of trailing binary zeros in a */
/* lfs_ctz(0) may be undefined */
static inline uint32_t lfs_ctz(uint32_t a)
{
#if !defined(LFS_NO_INTRINSICS) && defined(__GNUC__)
	return __builtin_ctz(a);
#else
	return lfs_npw2((a & -a) + 1) - 1;
#endif
}

/* Count the number of binary ones in a */
static inline uint32_t lfs_popc(uint32_t a)
{
#if !defined(LFS_NO_INTRINSICS) && (defined(__GNUC__) || defined(__CC_ARM))
	return __builtin_popcount(a);
#else
	a = a - ((a >> 1) & 0x55555555);
	a = (a & 0x33333333) + ((a >> 2) & 0x33333333);
	return (((a + (a >> 4)) & 0xf0f0f0f) * 0x1010101) >> 24;
#endif
}

/* Find the sequence comparison of a and b, this is the distance */
/* between a and b ignoring overflow */
static inline int lfs_scmp(uint32_t a, uint32_t b)
{
	return (int)(unsigned int)(a - b);
}

/* Convert between 32-bit little-endian and native order */
static inline uint32_t lfs_fromle32(uint32_t a)
{
#if defined(CONFIG_LITTLE_ENDIAN)
	return a;
#elif !defined(LFS_NO_INTRINSICS)
	return __builtin_bswap32(a);
#else
	return (((uint8_t *)&a)[0] <<  0) |
	       (((uint8_t *)&a)[1] <<  8) |
	       (((uint8_t *)&a)[2] << 16) |
	       (((uint8_t *)&a)[3] << 24);
#endif
}

static inline uint32_t lfs_tole32(uint32_t a)
{
	return lfs_fromle32(a);
}

/* Convert between 32-bit big-endian and native order */
static inline uint32_t lfs_frombe32(uint32_t a)
{
#if defined(CONFIG_BIG_ENDIAN)
	return a;
#elif !defined(LFS_NO_INTRINSICS)
	return __builtin_bswap32(a);
#else
	return (((uint8_t *)&a)[0] << 24) |
	       (((uint8_t *)&a)[1] << 16) |
	       (((uint8_t *)&a)[2] <<  8) |
	       (((uint8_t *)&a)[3] <<  0);
#endif
}

static inline uint32_t lfs_tobe32(uint32_t a)
{
	return lfs_frombe32(a);
}

/* Calculate CRC-32 with polynomial = 0x04c11db7 */
uint32_t lfs_crc(uint32_t crc, const void *buffer, size_t size);

/* Allocate memory, only used if buffers are not provided to littlefs */
/* Note, memory must be 64-bit aligned */
static inline void *lfs_malloc(size_t size)
{
#ifndef LFS_NO_MALLOC
	return malloc(size);
#else
	(void)size;
	return NULL;
#endif
}

/* Deallocate memory, only used if buffers are not provided to littlefs */
static inline void lfs_free(void *p)
{
#ifndef LFS_NO_MALLOC
	free(p);
#else
	(void)p;
#endif
}


#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* ZEPHYR_LFS_CONFIG_H */
