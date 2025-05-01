/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * nRF SoC specific public APIs for Device Memory Management (dmm) subsystem
 */

#ifndef SOC_NORDIC_COMMON_DMM_H_
#define SOC_NORDIC_COMMON_DMM_H_

#include <stdint.h>
#include <zephyr/devicetree.h>
#include <zephyr/linker/devicetree_regions.h>
#include <zephyr/mem_mgmt/mem_attr.h>
#include <zephyr/sys/util.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @cond INTERNAL_HIDDEN */

#ifdef CONFIG_DCACHE

/* Determine if memory region is cacheable. */
#define DMM_IS_REG_CACHEABLE(node_id)					     \
	COND_CODE_1(DT_NODE_HAS_PROP(node_id, zephyr_memory_attr),	     \
		((DT_PROP(node_id, zephyr_memory_attr) & DT_MEM_CACHEABLE)), \
		(0))

/* Determine required alignment of the data buffers in specified memory region.
 * Cache line alignment is required if region is cacheable and data cache is enabled.
 */
#define DMM_REG_ALIGN_SIZE(node_id) \
	(DMM_IS_REG_CACHEABLE(node_id) ? CONFIG_DCACHE_LINE_SIZE : sizeof(uint8_t))

#else

#define DMM_IS_REG_CACHEABLE(node_id) 0
#define DMM_REG_ALIGN_SIZE(node_id) (sizeof(uint8_t))

#endif /* CONFIG_DCACHE */

/* Determine required alignment of the data buffers in memory region
 * associated with specified device node.
 */
#define DMM_ALIGN_SIZE(node_id) DMM_REG_ALIGN_SIZE(DT_PHANDLE(node_id, memory_regions))

/**
 * @brief Get reference to memory region associated with the specified device node
 *
 * @param node_id Device node.
 *
 * @return Reference to memory region. NULL if not defined for given device node.
 */
#define DMM_DEV_TO_REG(node_id)					\
	COND_CODE_1(DT_NODE_HAS_PROP(node_id, memory_regions),	\
		((void *)DT_REG_ADDR(DT_PHANDLE(node_id, memory_regions))), (NULL))

/**
 * @brief Preallocate buffer in memory region associated with the specified device node
 *
 * @param node_id Device node.
 */
#define DMM_MEMORY_SECTION(node_id)					\
	COND_CODE_1(DT_NODE_HAS_PROP(node_id, memory_regions),		\
		(__attribute__((__section__(LINKER_DT_NODE_REGION_NAME(	\
			DT_PHANDLE(node_id, memory_regions)))))		\
			__aligned(DMM_ALIGN_SIZE(node_id))),		\
		())

#ifdef CONFIG_HAS_NORDIC_DMM

/**
 * @brief Prepare a DMA output buffer for the specified device
 *
 * Allocate an output buffer in memory region that given device can perform DMA transfers from.
 * Copy @p user_buffer contents into it.
 * Writeback data cache lines associated with output buffer, if needed.
 *
 * @note Depending on provided user buffer parameters and SoC architecture,
 *       dynamic allocation and cache operations might be skipped.
 *
 * @note @p buffer_out can be released using @ref dmm_buffer_in_release()
 *       to support transmitting and receiving data to the same buffer.
 *
 * @warning It is prohibited to read or write @p user_buffer or @p buffer_out contents
 *          from the time this function is called until @ref dmm_buffer_out_release()
 *          or @ref dmm_buffer_in_release is called on the same buffer
 *          or until this function returns with an error.
 *
 * @param region Memory region associated with device to prepare the buffer for.
 * @param user_buffer CPU address (virtual if applicable) of the buffer containing data
 *                    to be processed by the given device.
 * @param user_length Length of the buffer containing data to be processed by the given device.
 * @param buffer_out Pointer to a bus address of a buffer containing the prepared DMA buffer.
 *
 * @retval 0 If succeeded.
 * @retval -ENOMEM If output buffer could not be allocated.
 * @retval -errno Negative errno for other failures.
 */
int dmm_buffer_out_prepare(void *region, void const *user_buffer, size_t user_length,
			   void **buffer_out);

/**
 * @brief Release the previously prepared DMA output buffer
 *
 * @param region Memory region associated with device to release the buffer for.
 * @param buffer_out Bus address of the DMA output buffer previously prepared
 *                   with @ref dmm_buffer_out_prepare().
 *
 * @retval 0 If succeeded.
 * @retval -errno Negative errno code on failure.
 */
int dmm_buffer_out_release(void *region, void *buffer_out);

/**
 * @brief Prepare a DMA input buffer for the specified device
 *
 * Allocate an input buffer in memory region that given device can perform DMA transfers to.
 *
 * @note Depending on provided user buffer parameters and SoC architecture,
 *       dynamic allocation might be skipped.
 *
 * @warning It is prohibited to read or write @p user_buffer or @p buffer_in contents
 *          from the time this function is called until @ref dmm_buffer_in_release()
 *          is called on the same buffer or until this function returns with an error.
 *
 * @param region Memory region associated with device to prepare the buffer for.
 * @param user_buffer CPU address (virtual if applicable) of the buffer to be filled with data
 *                    from the given device.
 * @param user_length Length of the buffer to be filled with data from the given device.
 * @param buffer_in Pointer to a bus address of a buffer containing the prepared DMA buffer.
 *
 * @retval 0 If succeeded.
 * @retval -ENOMEM If input buffer could not be allocated.
 * @retval -errno Negative errno for other failures.
 */
int dmm_buffer_in_prepare(void *region, void *user_buffer, size_t user_length, void **buffer_in);

/**
 * @brief Release the previously prepared DMA input buffer
 *
 * Invalidate data cache lines associated with input buffer, if needed.
 * Copy @p buffer_in contents into @p user_buffer, if needed.
 *
 * @param region Memory region associated with device to release the buffer for.
 * @param user_buffer CPU address (virtual if applicable) of the buffer to be filled with data
 *                    from the given device.
 * @param user_length Length of the buffer to be filled with data from the given device.
 * @param buffer_in Bus address of the DMA input buffer previously prepared
 *                  with @ref dmm_buffer_in_prepare().
 *
 * @note @p user_buffer and @p buffer_in arguments pair provided in this function call must match
 *       the arguments pair provided in prior call to @ref dmm_buffer_out_prepare()
 *       or @ref dmm_buffer_in_prepare().
 *
 * @retval 0 If succeeded.
 * @retval -errno Negative errno code on failure.
 */
int dmm_buffer_in_release(void *region, void *user_buffer, size_t user_length, void *buffer_in);

/** @endcond */

#else

static ALWAYS_INLINE int dmm_buffer_out_prepare(void *region, void const *user_buffer,
						size_t user_length, void **buffer_out)
{
	ARG_UNUSED(region);
	ARG_UNUSED(user_length);
	*buffer_out = (void *)user_buffer;
	return 0;
}

static ALWAYS_INLINE int dmm_buffer_out_release(void *region, void *buffer_out)
{
	ARG_UNUSED(region);
	ARG_UNUSED(buffer_out);
	return 0;
}

static ALWAYS_INLINE int dmm_buffer_in_prepare(void *region, void *user_buffer, size_t user_length,
					       void **buffer_in)
{
	ARG_UNUSED(region);
	ARG_UNUSED(user_length);
	*buffer_in = user_buffer;
	return 0;
}

static ALWAYS_INLINE int dmm_buffer_in_release(void *region, void *user_buffer, size_t user_length,
					       void *buffer_in)
{
	ARG_UNUSED(region);
	ARG_UNUSED(user_buffer);
	ARG_UNUSED(user_length);
	ARG_UNUSED(buffer_in);
	return 0;
}

#endif

#ifdef __cplusplus
}
#endif

#endif /* SOC_NORDIC_COMMON_DMM_H_ */
