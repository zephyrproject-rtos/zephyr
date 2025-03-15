/*
 * Copyright (c) 2023 Zephyr Project.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DMA_H_
#define ZEPHYR_INCLUDE_DMA_H_

#include <zephyr/device.h>
#include <zephyr/types.h>
#include <zephyr/sys/util.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief DMA device driver API
 * @defgroup dma_interface DMA Interface
 * @ingroup io_interfaces
 * @{
 */

/**
 * @brief DMA channel configuration structure
 */
struct dma_config {
	uint32_t channel_direction; /**< Transfer direction */
	uint32_t source_data_size; /**< Source data size in bytes */
	uint32_t dest_data_size; /**< Destination data size in bytes */
	uint32_t source_burst_length; /**< Source burst length in bytes */
	uint32_t dest_burst_length; /**< Destination burst length in bytes */
	uint32_t dma_slot; /**< DMA slot */
	uint32_t channel_priority; /**< Channel priority */
	bool complete_callback_en; /**< Enable complete callback */
	bool error_callback_dis; /**< Disable error callback */
	dma_callback_t dma_callback; /**< DMA callback function */
	void *user_data; /**< User data for callback */
	struct dma_block_config *head_block; /**< Head block configuration */
};

/**
 * @brief DMA block configuration structure
 */
struct dma_block_config {
	uint32_t source_address; /**< Source address */
	uint32_t dest_address; /**< Destination address */
	uint32_t block_size; /**< Block size in bytes */
	struct dma_block_config *next_block; /**< Pointer to next block */
};

/**
 * @brief DMA status structure
 */
struct dma_status {
	uint32_t pending_length; /**< Pending transfer length in bytes */
	uint32_t dir; /**< Transfer direction */
	uint32_t src_addr; /**< Source address */
	uint32_t dst_addr; /**< Destination address */
};

/**
 * @brief DMA driver API
 */
struct dma_driver_api {
	int (*config)(const struct device *dev, uint32_t channel, struct dma_config *cfg);
	int (*start)(const struct device *dev, uint32_t channel);
	int (*stop)(const struct device *dev, uint32_t channel);
	int (*suspend)(const struct device *dev, uint32_t channel);
	int (*resume)(const struct device *dev, uint32_t channel);
	int (*reload)(const struct device *dev, uint32_t channel, uint32_t src, uint32_t dst, size_t size);
	int (*get_status)(const struct device *dev, uint32_t channel, struct dma_status *stat);
};

/**
 * @brief DMA context structure
 */
struct dma_context {
	uint32_t magic; /**< Magic number for validation */
	atomic_t *atomic; /**< Atomic variable for synchronization */
	uint32_t dma_channels; /**< Number of DMA channels */
};

/**
 * @brief DMA callback function type
 */
typedef void (*dma_callback_t)(const struct device *dev, void *user_data, uint32_t channel, int status);

/**
 * @brief Allocate a context for the DMA device
 *
 * @param dev Pointer to the device structure for the driver instance
 * @param sid Stream ID
 *
 * @retval 0 on success, negative errno code on failure
 */
int deviso_ctx_alloc(const struct device *dev, uint32_t sid);

/**
 * @brief Free a context for the DMA device
 *
 * @param dev Pointer to the device structure for the driver instance
 * @param sid Stream ID
 *
 * @retval 0 on success, negative errno code on failure
 */
int deviso_ctx_free(const struct device *dev, uint32_t sid);

/**
 * @brief Map a DMA buffer for the DMA device
 *
 * @param dev Pointer to the device structure for the driver instance
 * @param sid Stream ID
 * @param base Base address of the DMA buffer
 * @param size Size of the DMA buffer in bytes
 *
 * @retval 0 on success, negative errno code on failure
 */
int deviso_map(const struct device *dev, uint32_t sid, uintptr_t base, size_t size);

/**
 * @brief Unmap a DMA buffer for the DMA device
 *
 * @param dev Pointer to the device structure for the driver instance
 * @param sid Stream ID
 * @param base Base address of the DMA buffer
 * @param size Size of the DMA buffer in bytes
 *
 * @retval 0 on success, negative errno code on failure
 */
int deviso_unmap(const struct device *dev, uint32_t sid, uintptr_t base, size_t size);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DMA_H_ */
