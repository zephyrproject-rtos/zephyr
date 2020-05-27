/*
 * Copyright (c) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_MEMPOOL_SYS_H_

/**
 * @defgroup mem_pool_apis Memory Pool APIs
 * @ingroup kernel_apis
 * @{
 */

/* Note on sizing: the use of a 20 bit field for block means that,
 * assuming a reasonable minimum block size of 16 bytes, we're limited
 * to 16M of memory managed by a single pool.  Long term it would be
 * good to move to a variable bit size based on configuration.
 */
struct k_mem_block_id {
	uint32_t pool : 8;
	uint32_t level : 4;
	uint32_t block : 20;
};

struct k_mem_block {
	void *data;
	struct k_mem_block_id id;
};

/** @} */

struct k_mem_pool {
	struct sys_mem_pool_base base;
	_wait_q_t wait_q;
};

#define Z_MEM_POOL_DEFINE(name, minsz, maxsz, nmax, align)		\
	char __aligned(WB_UP(align)) _mpool_buf_##name[WB_UP(maxsz) * nmax \
				  + _MPOOL_BITS_SIZE(maxsz, minsz, nmax)]; \
	struct sys_mem_pool_lvl						\
		_mpool_lvls_##name[Z_MPOOL_LVLS(maxsz, minsz)]; \
	Z_STRUCT_SECTION_ITERABLE(k_mem_pool, name) = { \
		.base = {						\
			.buf = _mpool_buf_##name,			\
			.max_sz = WB_UP(maxsz),				\
			.n_max = nmax,					\
			.n_levels = Z_MPOOL_LVLS(maxsz, minsz),		\
			.levels = _mpool_lvls_##name,			\
			.flags = SYS_MEM_POOL_KERNEL			\
		} \
	}; \
	BUILD_ASSERT(WB_UP(maxsz) >= _MPOOL_MINBLK)

#endif /* ZEPHYR_INCLUDE_MEMPOOL_SYS_H_ */
