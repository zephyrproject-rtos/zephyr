/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef MEMOBJ_SCAN_H_
#define MEMOBJ_SCAN_H_

#include <kernel.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MEMOBJ_MAX_SIZE_BITS 12
#define MEMOBJ_MAX_CHUNK_SIZE_BITS 20

/** @brief Maximum memory object size */
#define MEMOBJ_MAX_SIZE ((1 << MEMOBJ_MAX_SIZE_BITS) - 1)

/** @brief Maximum chunk size supported by the memory object. */
#define MEMOBJ_MAX_CHUNK_SIZE ((1 << MEMOBJ_MAX_CHUNK_SIZE_BITS) - 1)

#define MEMOBJ1_OVERHEAD (sizeof(u32_t) + sizeof(void *))
#define MEMOBJ1_TOTAL_SIZE(len) (MEMOBJ1_OVERHEAD + len)

typedef void memobj_t;

/**
 * @brief Allocate memory object.
 *
 * Memory object is allocated from provided memory slab. It will consist of
 * one or multiple chunks. First chunk contains header with memory object size.
 * Each chunk contains pointer to the next chunk in the chain. Last chunk has
 * pointer to the memory slab from which memory object was allocated thus it can
 * be freed without access to memory slab handle.
 *
 * @param[in] slab	Memory slab instance.
 * @param[out] memobj	Memory object handle.
 * @param[in] size	Size of the memory object to allocate.
 * @param[in] timeout	Allocation timeout passed to memory slab.
 *
 * @retval -EINVAL if requested size is too big or memory slab has too bit slabs
 *		(see @ref MEMOBJ_MAX_CHUNK_SIZE).
 * @retval -ENOMEM if failed to allocate chunk with K_NO_WAIT timeout.
 * @retval -EAGAIN if failed to allocate chunk with timeout.
 */
int memobj_alloc(struct k_mem_slab *slab, memobj_t **memobj,
		 u32_t size, s32_t timeout);

memobj_t *memobjectize(u8_t *data, u32_t len);

/** @brief Free memory object.
 *
 * All chunks which builds memory object are freed. Handle to memory slab is
 * stored in memory object.
 *
 * @param memobj Memory object.
 */
void memobj_free(memobj_t *memobj);

/** @brief Write data to a memory object.
 *
 * If requested length with offset exceeds memory object capabilities then only
 * partial write will occur.
 *
 * @param memobj	Memory object.
 * @param data		Data to be written to the memory object.
 * @param len		Amount of bytes to write.
 * @param offset	Write offset.
 *
 * @return Number of bytes written.
 */
u32_t memobj_write(memobj_t *memobj, u8_t *data, u32_t len, u32_t offset);

/** @brief Read data from a memory object.
 *
 * If requested length with offset exceeds memory object capabilities then only
 * partial read will occur.
 *
 * @param memobj	Memory object.
 * @param data		Buffer for data from the memory object.
 * @param len		Amount of bytes to read.
 * @param offset	Read offset.
 *
 * @return Number of bytes read.
 */
u32_t memobj_read(memobj_t *memobj, u8_t *data, u32_t len, u32_t offset);

#ifdef __cplusplus
}
#endif

#endif /* MEMOBJ_SCAN_H_ */
