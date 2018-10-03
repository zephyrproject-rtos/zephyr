/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_MISC_MEMPOOL_BASE_H_
#define ZEPHYR_INCLUDE_MISC_MEMPOOL_BASE_H_

#include <zephyr/types.h>
#include <stddef.h>

/*
 * Definitions and macros used by both the IRQ-safe k_mem_pool and user-mode
 * compatible sys_mem_pool implementations
 */

struct sys_mem_pool_lvl {
	union {
		u32_t *bits_p;
		u32_t bits;
	};
	sys_dlist_t free_list;
};

#define SYS_MEM_POOL_KERNEL	BIT(0)
#define SYS_MEM_POOL_USER	BIT(1)

struct sys_mem_pool_base {
	void *buf;
	size_t max_sz;
	u16_t n_max;
	u8_t n_levels;
	s8_t max_inline_level;
	struct sys_mem_pool_lvl *levels;
	u8_t flags;
};

#define _ALIGN4(n) ((((n)+3)/4)*4)

#define _MPOOL_HAVE_LVL(max, min, l) (((max) >> (2*(l))) >= (min) ? 1 : 0)

#define __MPOOL_LVLS(maxsz, minsz)		\
	(_MPOOL_HAVE_LVL((maxsz), (minsz), 0) +	\
	_MPOOL_HAVE_LVL((maxsz), (minsz), 1) +	\
	_MPOOL_HAVE_LVL((maxsz), (minsz), 2) +	\
	_MPOOL_HAVE_LVL((maxsz), (minsz), 3) +	\
	_MPOOL_HAVE_LVL((maxsz), (minsz), 4) +	\
	_MPOOL_HAVE_LVL((maxsz), (minsz), 5) +	\
	_MPOOL_HAVE_LVL((maxsz), (minsz), 6) +	\
	_MPOOL_HAVE_LVL((maxsz), (minsz), 7) +	\
	_MPOOL_HAVE_LVL((maxsz), (minsz), 8) +	\
	_MPOOL_HAVE_LVL((maxsz), (minsz), 9) +	\
	_MPOOL_HAVE_LVL((maxsz), (minsz), 10) +	\
	_MPOOL_HAVE_LVL((maxsz), (minsz), 11) +	\
	_MPOOL_HAVE_LVL((maxsz), (minsz), 12) +	\
	_MPOOL_HAVE_LVL((maxsz), (minsz), 13) +	\
	_MPOOL_HAVE_LVL((maxsz), (minsz), 14) +	\
	_MPOOL_HAVE_LVL((maxsz), (minsz), 15))

#define _MPOOL_MINBLK sizeof(sys_dnode_t)

#define _MPOOL_LVLS(max, min)		\
	__MPOOL_LVLS((max), (min) >= _MPOOL_MINBLK ? (min) : _MPOOL_MINBLK)

/* Rounds the needed bits up to integer multiples of u32_t */
#define _MPOOL_LBIT_WORDS_UNCLAMPED(n_max, l) \
	((((n_max) << (2*(l))) + 31) / 32)

/* One word gets stored free unioned with the pointer, otherwise the
 * calculated unclamped value
 */
#define _MPOOL_LBIT_WORDS(n_max, l)			\
	(_MPOOL_LBIT_WORDS_UNCLAMPED(n_max, l) < 2 ? 0	\
	 : _MPOOL_LBIT_WORDS_UNCLAMPED(n_max, l))

/* How many bytes for the bitfields of a single level? */
#define _MPOOL_LBIT_BYTES(maxsz, minsz, l, n_max)	\
	(_MPOOL_LVLS((maxsz), (minsz)) >= (l) ?		\
	 4 * _MPOOL_LBIT_WORDS((n_max), l) : 0)

/* Size of the bitmap array that follows the buffer in allocated memory */
#define _MPOOL_BITS_SIZE(maxsz, minsz, n_max) \
	(_MPOOL_LBIT_BYTES(maxsz, minsz, 0, n_max) +	\
	_MPOOL_LBIT_BYTES(maxsz, minsz, 1, n_max) +	\
	_MPOOL_LBIT_BYTES(maxsz, minsz, 2, n_max) +	\
	_MPOOL_LBIT_BYTES(maxsz, minsz, 3, n_max) +	\
	_MPOOL_LBIT_BYTES(maxsz, minsz, 4, n_max) +	\
	_MPOOL_LBIT_BYTES(maxsz, minsz, 5, n_max) +	\
	_MPOOL_LBIT_BYTES(maxsz, minsz, 6, n_max) +	\
	_MPOOL_LBIT_BYTES(maxsz, minsz, 7, n_max) +	\
	_MPOOL_LBIT_BYTES(maxsz, minsz, 8, n_max) +	\
	_MPOOL_LBIT_BYTES(maxsz, minsz, 9, n_max) +	\
	_MPOOL_LBIT_BYTES(maxsz, minsz, 10, n_max) +	\
	_MPOOL_LBIT_BYTES(maxsz, minsz, 11, n_max) +	\
	_MPOOL_LBIT_BYTES(maxsz, minsz, 12, n_max) +	\
	_MPOOL_LBIT_BYTES(maxsz, minsz, 13, n_max) +	\
	_MPOOL_LBIT_BYTES(maxsz, minsz, 14, n_max) +	\
	_MPOOL_LBIT_BYTES(maxsz, minsz, 15, n_max))


void _sys_mem_pool_base_init(struct sys_mem_pool_base *p);

int _sys_mem_pool_block_alloc(struct sys_mem_pool_base *p, size_t size,
			      u32_t *level_p, u32_t *block_p, void **data_p);

void _sys_mem_pool_block_free(struct sys_mem_pool_base *p, u32_t level,
			      u32_t block);

#endif /* ZEPHYR_INCLUDE_MISC_MEMPOOL_BASE_H_ */
