/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Process Mission
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Emulated DMA controller helpers.
 * @ingroup dma_interface
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_DMA_DMA_EMUL_H_
#define ZEPHYR_INCLUDE_DRIVERS_DMA_DMA_EMUL_H_

#include <stdbool.h>
#include <zephyr/device.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Check whether the current thread runs an emulated DMA workqueue.
 * @ingroup dma_interface
 *
 * @param dev Initialized zephyr,dma-emul controller.
 * @retval true Current thread runs the controller's workqueue.
 * @retval false Current thread does not run the controller's workqueue.
 */
bool dma_emul_is_in_workqueue(const struct device *dev);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_DMA_DMA_EMUL_H_ */
