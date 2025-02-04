/*
 * Copyright (c) 2025 Silicon Laboratories Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_DMA_SILABS_LDMA_H_
#define ZEPHYR_INCLUDE_DRIVERS_DMA_SILABS_LDMA_H_

#include <zephyr/drivers/dma.h>

#define SILABS_LDMA_SOURCE_MASK  GENMASK(21, 16)
#define SILABS_LDMA_SIG_MASK     GENMASK(3, 0)

#define SILABS_DMA_SLOT_SOURCE_MASK  GENMASK(7, 3)
#define SILABS_DMA_SLOT_SIG_MASK     GENMASK(2, 0)

#define SILABS_LDMA_REQSEL_TO_SLOT(signal)             \
	FIELD_PREP(SILABS_DMA_SLOT_SOURCE_MASK, FIELD_GET(SILABS_LDMA_SOURCE_MASK, signal)) | \
	FIELD_PREP(SILABS_DMA_SLOT_SIG_MASK, FIELD_GET(SILABS_LDMA_SIG_MASK, signal))

#define SILABS_LDMA_SLOT_TO_REQSEL(slot)     \
	FIELD_PREP(SILABS_LDMA_SOURCE_MASK, FIELD_GET(SILABS_DMA_SLOT_SOURCE_MASK, slot)) | \
	FIELD_PREP(SILABS_LDMA_SIG_MASK, FIELD_GET(SILABS_DMA_SLOT_SIG_MASK, slot))

/**
 * @brief Append a new block to the current channel
 *
 * This function allows to append a block to the current DMA transfer. It allows a user/driver
 * to register the next DMA transfer while a transfer in being held without stopping or restarting
 * DMA engine. It is very suitable for Zephyr Uart API where user gives buffers while the DMA engine
 * is running. Because this function changes dynamically the link to the block that DMA engine would
 * load as the next transfer, it is only working with channel that didn't have linked block list.
 *
 * In the case that the DMA engine naturally stopped because the previous transfer is finished, this
 * function simply restart the DMA engine with the given block. If the DMA engine stopped while
 * reconfiguring the next transfer, the DMA engine will restart too.
 *
 * @param dev: dma device
 * @param channel: channel
 * @param config: configuration of the channel with the block to append as the head_block.
 */
int silabs_ldma_append_block(const struct device *dev, uint32_t channel,
					struct dma_config *config);

#endif /* ZEPHYR_INCLUDE_DRIVERS_DMA_SILABS_LDMA_H_*/
