/*
 * Copyright (c) 2023 Carlo Caione <ccaione@baylibre.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DMM_DMM_H_
#define ZEPHYR_INCLUDE_DMM_DMM_H_

#include <stdio.h>
#include <zephyr/device.h>
#include <zephyr/kernel.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Get a DMA handle for outbound data transfer
 *
 *  This function is used to translate the user-provided buffer into something
 *  that is usable for a DMA outbound data transfer and return that to the user
 *  as an opaque DMA handle (usually a physical/bus address).
 *
 *  This function should:
 *
 *  - allocate bounce buffers if the user-provided buffer cannot be used
 *    directly by the DMA (for example the memory is not DMA accessible, not
 *    aligned, not padded to cache line size, etc...)
 *  - copy the data from the user-provided buffer to the bounce buffers
 *  - translate CPU buffer addresses to DMA addresses if needed
 *  - do the proper cache management if buffers are allocated in cacheable
 *    memory (for example flushing cache lines associated with the memory
 *    backing the DMA accessible buffers)
 *
 *  @param[in] dev Device pointer
 *  @param[in] flags Flags
 *  @param[out] dma_handle DMA handle.
 *  @param[in] buffer User-provided buffer.
 *  @param[in] buffer_size Size of the user-provided buffer.
 *
 *  @retval 0 on success
 *  @retval other errno codes depending on the implementation
 */
int dmm_get_buffer_for_dma_out(const struct device *dev, uint32_t flags,
			       void **dma_handle, const void *buffer,
			       size_t buffer_size);

/** @brief Release a DMA handle (outbound data transfer)
 *
 *  This function is used to release a DMA handle and the associated memory,
 *  for example freeing the bounce buffers if used.
 *
 *  @param[in] dev Device pointer
 *  @param[in] flags Flags
 *  @param[in] dma_handle DMA handle.
 *
 *  @retval 0 on success
 *  @retval other errno codes depending on the implementation
 */
int dmm_release_buffer_for_dma_out(const struct device *dev, uint32_t flags,
				   void *dma_handle);

/** @brief Get a DMA handle for inbound data transfer
 *
 *  This function is used to translate the user-provided buffer into something
 *  that is usable for a DMA inbound data transfer and return that to the user
 *  as an opaque DMA handle (usually a physical/bus address).
 *
 *  This function should:
 *
 *  - allocate bounce buffers if the user-provided buffer cannot be used
 *    directly by the DMA (for example the memory is not DMA accessible, not
 *    aligned, not padded to cache line size, etc...)
 *  - translate CPU buffer addresses to DMA addresses if needed
 *  - do the proper cache management if buffers are allocated in cacheable
 *    memory (for example invalidating cache lines associated with the memory
 *    backing the DMA accessible buffers)
 *
 *  @param[in] dev Device pointer
 *  @param[in] flags Flags
 *  @param[out] dma_handle DMA handle.
 *  @param[in] buffer User-provided buffer.
 *  @param[in] buffer_size Size of the user-provided buffer.
 *
 *  @retval 0 on success
 *  @retval other errno codes depending on the implementation
 */
int dmm_get_buffer_for_dma_in(const struct device *dev, uint32_t flags,
			      void **dma_handle, const void *buffer,
			      size_t buffer_size);

/** @brief Get data and release a DMA handle (inbound data transfer)
 *
 *  This function is used to retrieve the user-accessible data from the DMA
 *  handle, release it and free the associated memory.
 *
 *  This function should:
 *
 *  - do the proper cache management if buffers are allocated in cacheable
 *    memory (for example invalidating cache lines associated with the memory
 *    backing the DMA accessible buffers)
 *  - copy the data from the DMA buffer to the user-provided buffer
 *  - update the @p buffer_size pointer with the size of the retrieved data
 *  - free the bounce buffers if used
 *
 *  @param[in] dev Device pointer
 *  @param[in] flags Flags
 *  @param[in] dma_handle DMA handle.
 *  @param[out] buffer User-provided buffer
 *  @param[in,out] buffer_size Size of the input buffer and returned data
 *
 *  @retval 0 on success
 *  @retval other errno codes depending on the implementation
 */
int dmm_release_buffer_for_dma_in(const struct device *dev, uint32_t flags,
				  void *dma_handle, void *buffer,
				  size_t *buffer_size);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DMM_DMM_H_ */
