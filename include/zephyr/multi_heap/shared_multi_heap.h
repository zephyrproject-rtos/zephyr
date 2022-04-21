/*
 * Copyright (c) 2021 Carlo Caione, <ccaione@baylibre.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_MULTI_HEAP_MANAGER_SMH_H_
#define ZEPHYR_INCLUDE_MULTI_HEAP_MANAGER_SMH_H_


#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Shared multi-heap interface
 * @defgroup shared_multi_heap Shared multi-heap interface
 * @ingroup multi_heap
 * @{
 *
 * The shared multi-heap manager uses the multi-heap allocator to manage a set
 * of reserved memory regions with different capabilities / attributes
 * (cacheable, non-cacheable, etc...) defined in the DT.
 *
 * The user can request allocation from the shared pool specifying the
 * capability / attribute of interest for the memory (cacheable / non-cacheable
 * memory, etc...)
 *
 */

/**
 * @brief Memory region attributes / capabilities
 *
 * ** This list needs to be kept in sync with shared-multi-heap.yaml **
 */
enum smh_reg_attr {
	/** cacheable */
	SMH_REG_ATTR_CACHEABLE,

	/** non-cacheable */
	SMH_REG_ATTR_NON_CACHEABLE,

	/** must be the last item */
	SMH_REG_ATTR_NUM,
};

/**
 * @brief SMH region struct
 *
 * This struct is carrying information about the memory region to be added in
 * the multi-heap pool. This is filled by the manager with the information
 * coming from the reserved memory children nodes in the DT.
 */
struct shared_multi_heap_region {
	enum smh_reg_attr attr;
	uintptr_t addr;
	size_t size;
};

/**
 * @brief Region init function
 *
 * This is a user-provided function whose responsibility is to setup or
 * initialize the memory region passed in input before this is added to the
 * heap pool by the shared multi-heap manager. This function can be used by
 * architectures using MMU / MPU that must correctly map the region before this
 * is considered valid and accessible.
 *
 * @param reg Pointer to the SMH region structure.
 * @param v_addr Virtual address obtained after mapping. For non-MMU
 *               architectures this value is the physical address of the
 *               region.
 * @param size Size of the region after mapping.
 *
 * @return True if the region is ready to be added to the heap pool.
 *         False if the region must be skipped.
 */
typedef bool (*smh_init_reg_fn_t)(struct shared_multi_heap_region *reg,
				  uint8_t **v_addr, size_t *size);


/**
 * @brief Init the pool
 *
 * Initialize the shared multi-heap pool and hook-up the region init function.
 *
 * @param smh_init_reg_fn The function pointer to the region init function. Can
 *                        be NULL for non-MPU / non-MMU architectures.
 */
int shared_multi_heap_pool_init(smh_init_reg_fn_t smh_init_reg_fn);

/**
 * @brief Allocate memory from the memory shared multi-heap pool
 *
 * Allocate a block of memory of the specified size in bytes and with a
 * specified capability / attribute.
 *
 * @param attr Capability / attribute requested for the memory block.
 * @param bytes Requested size of the allocation in bytes.
 *
 * @return A valid pointer to heap memory or NULL if no memory is available.
 */
void *shared_multi_heap_alloc(enum smh_reg_attr attr, size_t bytes);

/**
 * @brief Free memory from the shared multi-heap pool
 *
 * Free the passed block of memory.
 *
 * @param block Block to free.
 */
void shared_multi_heap_free(void *block);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_MULTI_HEAP_MANAGER_SMH_H_ */
