/*
 * Copyright (c) 2021 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_DMA_DMA_STM32_H_
#define ZEPHYR_INCLUDE_DRIVERS_DMA_DMA_STM32_H_

#include <stdint.h>

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/devicetree/dma.h>

/* @brief linked_channel value to inform zephyr dma driver that
 * DMA channel will be handled by HAL
 */
#define STM32_DMA_HAL_OVERRIDE      0x7F

/* @brief gives the first DMA channel : 0 or 1 in the register map
 * when counting channels from 1 to N or from 0 to N-1
 */
#if defined(CONFIG_DMA_STM32U5)
/* from DTS the dma stream id is in range 0..N-1 */
#define STM32_DMA_STREAM_OFFSET 0
#elif !defined(CONFIG_DMA_STM32_V1)
/* from DTS the dma stream id is in range 1..N */
/* so decrease to set range from 0 from now on */
#define STM32_DMA_STREAM_OFFSET 1
#elif defined(CONFIG_DMA_STM32_V1) && defined(CONFIG_DMAMUX_STM32)
/* typically on the stm32H7 series, DMA V1 with mux */
#define STM32_DMA_STREAM_OFFSET 1
#else
/* from DTS the dma stream id is in range 0..N-1 */
#define STM32_DMA_STREAM_OFFSET 0
#endif /* ! CONFIG_DMA_STM32_V1 */

/* macro for dma slot (only for dma-v1 or dma-v2 types) */
#if DT_HAS_COMPAT_STATUS_OKAY(st_stm32_dma_v2bis)
#define STM32_DMA_SLOT(id, dir, slot) 0
#define STM32_DMA_SLOT_BY_IDX(id, idx, slot) 0
#else
#define STM32_DMA_SLOT(id, dir, slot) DT_INST_DMAS_CELL_BY_NAME(id, dir, slot)
#define STM32_DMA_SLOT_BY_IDX(id, idx, slot) DT_INST_DMAS_CELL_BY_IDX(id, idx, slot)
#endif

#if DT_HAS_COMPAT_STATUS_OKAY(st_stm32_dma_v2) || \
	DT_HAS_COMPAT_STATUS_OKAY(st_stm32_dma_v2bis) || \
	DT_HAS_COMPAT_STATUS_OKAY(st_stm32_dmamux)
#define STM32_DMA_FEATURES(id, dir) 0
#else
#define STM32_DMA_FEATURES(id, dir)						\
		DT_INST_DMAS_CELL_BY_NAME(id, dir, features)
#endif

#define STM32_DMA_CTLR(id, dir)						\
		DT_INST_DMAS_CTLR_BY_NAME(id, dir)
#define STM32_DMA_CHANNEL_CONFIG(id, dir)					\
		DT_INST_DMAS_CELL_BY_NAME(id, dir, channel_config)
#define STM32_DMA_CHANNEL_CONFIG_BY_IDX(id, idx)				\
		DT_INST_DMAS_CELL_BY_IDX(id, idx, channel_config)

/* macros for channel-config */
/* enable circular buffer */
#define STM32_DMA_CONFIG_CYCLIC(config)                 ((config >> 5) & 0x1)
/* direction defined on bits 6-7 */
/* 0 -> MEM_TO_MEM, 1 -> MEM_TO_PERIPH, 2 -> PERIPH_TO_MEM */
#define STM32_DMA_CONFIG_DIRECTION(config)		((config >> 6) & 0x3)
/* periph increment defined on bit 9 as true/false */
#define STM32_DMA_CONFIG_PERIPHERAL_ADDR_INC(config)	((config >> 9) & 0x1)
/* mem increment defined on bit 10 as true/false */
#define STM32_DMA_CONFIG_MEMORY_ADDR_INC(config)	((config >> 10) & 0x1)
/* periph data size defined on bits 11-12 */
/* 0 -> 1 byte, 1 -> 2 bytes, 2 -> 4 bytes */
#define STM32_DMA_CONFIG_PERIPHERAL_DATA_SIZE(config)	\
						(1 << ((config >> 11) & 0x3))
/* memory data size defined on bits 13, 14 */
/* 0 -> 1 byte, 1 -> 2 bytes, 2 -> 4 bytes */
#define STM32_DMA_CONFIG_MEMORY_DATA_SIZE(config)	\
						(1 << ((config >> 13) & 0x3))
/* priority increment offset defined on bit 15 */
#define STM32_DMA_CONFIG_PERIPHERAL_INC_FIXED(config)	((config >> 15) & 0x1)
/* priority defined on bits 16-17 as 0, 1, 2, 3 */
#define STM32_DMA_CONFIG_PRIORITY(config)		((config >> 16) & 0x3)

/* macro for features (only for dma-v1) */
#if DT_HAS_COMPAT_STATUS_OKAY(st_stm32_dma_v1)
#define STM32_DMA_FEATURES_FIFO_THRESHOLD(features)	(features & 0x3)
#else
#define STM32_DMA_FEATURES_FIFO_THRESHOLD(features)	0
#endif

#if defined(CONFIG_SOC_SERIES_STM32H5X) || defined(CONFIG_SOC_SERIES_STM32H7RSX) ||                \
	defined(CONFIG_SOC_SERIES_STM32MP2X) || defined(CONFIG_SOC_SERIES_STM32N6X) ||             \
	defined(CONFIG_SOC_SERIES_STM32U3X) || defined(CONFIG_SOC_SERIES_STM32U5X) ||              \
	defined(CONFIG_SOC_SERIES_STM32WBAX)
#define STM32_DMA_GET_CHANNEL_INSTANCE LL_DMA_GET_CHANNEL_INSTANCE
#else
#define STM32_DMA_GET_CHANNEL_INSTANCE __LL_DMA_GET_CHANNEL_INSTANCE
#endif

#if DT_HAS_COMPAT_STATUS_OKAY(st_stm32_dma_v1)
#define STM32_DMA_GET_INSTANCE(reg, channel)				\
		__LL_DMA_GET_STREAM_INSTANCE((reg), (channel) - STM32_DMA_STREAM_OFFSET);
#else
#define STM32_DMA_GET_INSTANCE(reg, channel)				\
		STM32_DMA_GET_CHANNEL_INSTANCE((reg), (channel) - STM32_DMA_STREAM_OFFSET);
#endif

#if defined(CONFIG_DMA_STM32_DOUBLE_BUFFER) || defined(__DOXYGEN__)

/*
 * Stream-based STM32 DMA controllers can alternate between two memory buffers
 * on their own (DMA_SxCR DBM/CT). Compared to plain circular mode, the buffers
 * are two independent addresses rather than the two halves of one allocation,
 * and the address of the buffer that the stream is not currently using may be
 * replaced without stopping the transfer.
 *
 * The stream must first be set up with dma_config() in cyclic mode; the buffer
 * described by head_block becomes memory target 0. Both buffers are always the
 * size given to dma_config(), as the stream has a single item counter.
 */

/** Memory target 0 of a double-buffered stream (DMA_SxM0AR). */
#define STM32_DMA_MEM_TARGET_0 0U
/** Memory target 1 of a double-buffered stream (DMA_SxM1AR). */
#define STM32_DMA_MEM_TARGET_1 1U

/**
 * @brief Enable hardware double buffer mode on a stream.
 *
 * The stream must have been configured in cyclic mode and must be stopped, as
 * the hardware only accepts the DBM bit while the stream is disabled.
 *
 * @param dev     DMA device.
 * @param id      Stream to configure, numbered as in the devicetree.
 * @param buffer1 Address of memory target 1. Must meet the same size and
 *                alignment constraints as the buffer passed to dma_config().
 *
 * @retval 0 on success.
 * @retval -EINVAL if @p id or @p buffer1 is invalid.
 * @retval -ENOTSUP if the stream is not cyclic or is memory-to-memory.
 * @retval -EBUSY if the stream is running.
 */
int dma_stm32_double_buffer_enable(const struct device *dev, uint32_t id, uint32_t buffer1);

/**
 * @brief Disable hardware double buffer mode on a stream.
 *
 * The stream keeps its configuration and runs as a plain circular transfer on
 * memory target 0.
 *
 * @param dev DMA device.
 * @param id  Stream to configure, numbered as in the devicetree.
 *
 * @retval 0 on success.
 * @retval -EINVAL if @p id is invalid.
 * @retval -EBUSY if the stream is running.
 */
int dma_stm32_double_buffer_disable(const struct device *dev, uint32_t id);

/**
 * @brief Get the memory target a double-buffered stream is currently filling.
 *
 * The other target is the one that just completed, so it is the one that is
 * safe to consume and to hand back to dma_stm32_double_buffer_set_target().
 *
 * @param dev DMA device.
 * @param id  Stream to query, numbered as in the devicetree.
 *
 * @retval STM32_DMA_MEM_TARGET_0 or STM32_DMA_MEM_TARGET_1 on success.
 * @retval -EINVAL if @p id is invalid.
 * @retval -ENOTSUP if double buffer mode is not enabled on the stream.
 */
int dma_stm32_double_buffer_get_target(const struct device *dev, uint32_t id);

/**
 * @brief Point a memory target of a double-buffered stream at a new buffer.
 *
 * Only the target that the stream is not currently filling may be changed; the
 * transfer is not interrupted.
 *
 * @param dev    DMA device.
 * @param id     Stream to configure, numbered as in the devicetree.
 * @param target STM32_DMA_MEM_TARGET_0 or STM32_DMA_MEM_TARGET_1.
 * @param buffer Address of the new buffer.
 *
 * @retval 0 on success.
 * @retval -EINVAL if @p id, @p target or @p buffer is invalid.
 * @retval -ENOTSUP if double buffer mode is not enabled on the stream.
 * @retval -EBUSY if @p target is the one the stream is currently filling.
 */
int dma_stm32_double_buffer_set_target(const struct device *dev, uint32_t id,
				       uint32_t target, uint32_t buffer);

#endif /* CONFIG_DMA_STM32_DOUBLE_BUFFER */

#endif /* ZEPHYR_INCLUDE_DRIVERS_DMA_DMA_STM32_H_ */
