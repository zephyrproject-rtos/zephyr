/*
 * Copyright (c) 2024, Meta
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_DMA_DMA_EMUL_H_
#define ZEPHYR_INCLUDE_DRIVERS_DMA_DMA_EMUL_H_

#include <zephyr/device.h>
#include <zephyr/kernel.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Backend callback for dma_emul data transfers.
 *
 * Backends are expected to transfer data completely.
 *
 * @param dev dma_emul device with which to transfer data
 * @param user_data user-provided data (may be NULL)
 * @param dst destination address
 * @param src source address
 * @param size number of bytes to transfer
 *
 * @return 0 on success, otherwise a negative error number.
 */
typedef int (*dma_emul_backend_t)(const struct device *dev, void *user_data, uint64_t dst,
				  uint64_t src, size_t size);

/**
 * @brief Set the transfer backend for a dma_emul device
 *
 * @param dev dma_emul device with which to transfer data
 * @param backend the dma_emul backend transfer function
 * @param user_data user-provided data (may be NULL)
 *
 * @return 0 on success, otherwise a negative error number.
 */
__syscall int dma_emul_set_backend(const struct device *dev, dma_emul_backend_t backend,
				   void *user_data);

#include <syscalls/dma_emul.h>

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_DMA_DMA_EMUL_H_ */
