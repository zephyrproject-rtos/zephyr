/*
 * SPDX-FileCopyrightText: 2026 Texas Instruments Incorporated
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_DMA_TI_MSPM0_DMA_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_DMA_TI_MSPM0_DMA_H_

/** @brief Mask for DMATSEL trigger selection bits within dma_slot. */
#define TI_MSPM0_DMA_SLOT_DMATSEL_MASK GENMASK(5, 0)

/** @brief Mask for encoding extended modes within dma_slot. */
#define TI_MSPM0_DMA_SLOT_EXTMODE_MASK GENMASK(7, 6)

/**
 * @brief Build a dma_config.dma_slot value from a DMATSEL trigger and an extended mode.
 *
 * @param dmatsel DMATSEL trigger selection for the channel.
 * @param extmode One of the TI_MSPM0_DMA_SLOT_EXTMODE_* values.
 */
#define TI_MSPM0_DMA_SLOT(dmatsel, extmode)                                                        \
	FIELD_PREP(TI_MSPM0_DMA_SLOT_DMATSEL_MASK, dmatsel) |                                      \
		FIELD_PREP(TI_MSPM0_DMA_SLOT_EXTMODE_MASK, TI_MSPM0_DMA_SLOT_EXTMODE_##extmode)

/** @brief Extract the DMATSEL trigger selection from a dma_config.dma_slot value. */
#define TI_MSPM0_DMA_SLOT_DMATSEL(dma_slot) FIELD_GET(TI_MSPM0_DMA_SLOT_DMATSEL_MASK, dma_slot)

/** @brief Extract the channel mode from a dma_config.dma_slot value. */
#define TI_MSPM0_DMA_SLOT_EXTMODE(dma_slot) FIELD_GET(TI_MSPM0_DMA_SLOT_EXTMODE_MASK, dma_slot)

/** @brief Normal DMA transfer */
#define TI_MSPM0_DMA_SLOT_EXTMODE_NORMAL 0U

/** @brief Fills a memory block with the pattern in dma_block_config.source_address. */
#define TI_MSPM0_DMA_SLOT_EXTMODE_FILL 1U

/**
 * @brief Reads a table of {addr, data} pairs from dma_block_config.source_address and writes
 * each word to its addr.
 */
#define TI_MSPM0_DMA_SLOT_EXTMODE_TABLE 2U

/**
 * @brief Gathers data via an address table at dma_block_config.source_address into
 * dma_block_config.dest_address.
 */
#define TI_MSPM0_DMA_SLOT_EXTMODE_GATHER 3U

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_DMA_TI_MSPM0_DMA_H_ */
