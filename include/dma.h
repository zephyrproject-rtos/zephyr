/**
 * @file
 *
 * @brief Public APIs for the DMA drivers.
 */

/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _DMA_H_
#define _DMA_H_

#include <kernel.h>
#include <device.h>

#ifdef __cplusplus
extern "C" {
#endif


/**
 * @brief DMA Interface
 * @defgroup DMA_interface DMA Interface
 * @ingroup io_interfaces
 * @{
 */

enum dma_channel_direction {
	MEMORY_TO_MEMORY = 0x0,
	MEMORY_TO_PERIPHERAL,
	PERIPHERAL_TO_MEMORY
};

/**
 * @brief DMA block configuration structure.
 *
 * source_address is block starting address at source
 * source_gather_interval is the address adjustment at gather boundary
 * dest_address is block starting address at destination
 * dest_scatter_interval is the address adjustment at scatter boundary
 * dest_scatter_count is the continuous transfer count between scatter
 *                    boundaries
 * source_gather_count is the continuous transfer count between gather
 *                     boundaries
 * block_size is the number of bytes to be transferred for this block.
 *
 * config is a bit field with the following parts:
 *     source_gather_en   [ 0 ]       - 0-disable, 1-enable
 *     dest_scatter_en    [ 1 ]       - 0-disable, 1-enable
 *     source_addr_adj    [ 2 : 3 ]   - 00-increment, 01-decrement,
 *                                      10-no change
 *     dest_addr_adj      [ 4 : 5 ]   - 00-increment, 01-decrement,
 *                                      10-no change
 *     source_reload_en   [ 6 ]       - reload source address at the end of
 *                                      block transfer
 *                                      0-disable, 1-enable
 *     dest_reload_en     [ 7 ]       - reload destination address at the end
 *                                      of block transfer
 *                                      0-disable, 1-enable
 *     fifo_mode_control  [ 8 : 11 ]  - How full  of the fifo before transfer
 *                                      start. HW specific.
 *     flow_control_mode  [ 12 ]      - 0-source request served upon data
 *                                        availability
 *                                      1-source request postponed until
 *                                        destination request happens
 *     reserved           [ 13 : 15 ]
 */
struct dma_block_config {
	u32_t source_address;
	u32_t source_gather_interval;
	u32_t dest_address;
	u32_t dest_scatter_interval;
	u16_t dest_scatter_count;
	u16_t source_gather_count;
	u32_t block_size;
	struct dma_block_config *next_block;
	u16_t  source_gather_en :  1;
	u16_t  dest_scatter_en :   1;
	u16_t  source_addr_adj :   2;
	u16_t  dest_addr_adj :     2;
	u16_t  source_reload_en :  1;
	u16_t  dest_reload_en :    1;
	u16_t  fifo_mode_control : 4;
	u16_t  flow_control_mode : 1;
	u16_t  reserved :          3;
};

/**
 * @brief DMA configuration structure.
 *
 *     dma_slot             [ 0 : 5 ]   - which peripheral and direction
 *                                        (HW specific)
 *     channel_direction    [ 6 : 8 ]   - 000-memory to memory, 001-memory to
 *                                        peripheral, 010-peripheral to memory,
 *                                        ...
 *     complete_callback_en [ 9 ]       - 0-callback invoked at completion only
 *                                        1-callback invoked at completion of
 *                                          each block
 *     error_callback_en    [ 10 ]      - 0-error callback enabled
 *                                        1-error callback disabled
 *     source_handshake     [ 11 ]      - 0-HW, 1-SW
 *     dest_handshake       [ 12 ]      - 0-HW, 1-SW
 *     channel_priority     [ 13 : 16 ] - DMA channel priority
 *     source_chaining_en   [ 17 ]      - enable/disable source block chaining
 *                                        0-disable, 1-enable
 *     dest_chaining_en     [ 18 ]      - enable/disable destination block
 *                                        chaining.
 *                                        0-disable, 1-enable
 *     reserved             [ 19 : 31 ]
 *
 *     source_data_size    [ 0 : 15 ]   - width of source data (in bytes)
 *     dest_data_size      [ 16 : 31 ]  - width of dest data (in bytes)
 *     source_burst_length [ 0 : 15 ]   - number of source data units
 *     dest_burst_length   [ 16 : 31 ]  - number of destination data units
 *
 *     block_count  is the number of blocks used for block chaining, this
 *     depends on availability of the DMA controller.
 *
 * dma_callback is the callback function pointer. If enabled, callback function
 *              will be invoked at transfer completion or when error happens
 *              (error_code: zero-transfer success, non zero-error happens).
 */
struct dma_config {
	u32_t  dma_slot :             6;
	u32_t  channel_direction :    3;
	u32_t  complete_callback_en : 1;
	u32_t  error_callback_en :    1;
	u32_t  source_handshake :     1;
	u32_t  dest_handshake :       1;
	u32_t  channel_priority :     4;
	u32_t  source_chaining_en :   1;
	u32_t  dest_chaining_en :     1;
	u32_t  reserved :            13;
	u32_t  source_data_size :    16;
	u32_t  dest_data_size :      16;
	u32_t  source_burst_length : 16;
	u32_t  dest_burst_length :   16;
	u32_t block_count;
	struct dma_block_config *head_block;
	void (*dma_callback)(struct device *dev, u32_t channel,
			     int error_code);
};

/**
 * @cond INTERNAL_HIDDEN
 *
 * These are for internal use only, so skip these in
 * public documentation.
 */

typedef int (*dma_api_config)(struct device *dev, u32_t channel,
			      struct dma_config *config);

typedef int (*dma_api_start)(struct device *dev, u32_t channel);

typedef int (*dma_api_stop)(struct device *dev, u32_t channel);

struct dma_driver_api {
	dma_api_config config;
	dma_api_start start;
	dma_api_stop stop;
};
/**
 * @endcond
 */

/**
 * @brief Configure individual channel for DMA transfer.
 *
 * @param dev     Pointer to the device structure for the driver instance.
 * @param channel Numeric identification of the channel to configure
 * @param config  Data structure containing the intended configuration for the
 *                selected channel
 *
 * @retval 0 if successful.
 * @retval Negative errno code if failure.
 */
static inline int dma_config(struct device *dev, u32_t channel,
			     struct dma_config *config)
{
	const struct dma_driver_api *api = dev->driver_api;

	return api->config(dev, channel, config);
}

/**
 * @brief Enables DMA channel and starts the transfer, the channel must be
 *        configured beforehand.
 *
 * @param dev     Pointer to the device structure for the driver instance.
 * @param channel Numeric identification of the channel where the transfer will
 *                be processed
 *
 * @retval 0 if successful.
 * @retval Negative errno code if failure.
 */
static inline int dma_start(struct device *dev, u32_t channel)
{
	const struct dma_driver_api *api = dev->driver_api;

	return api->start(dev, channel);
}

/**
 * @brief Stops the DMA transfer and disables the channel.
 *
 * @param dev     Pointer to the device structure for the driver instance.
 * @param channel Numeric identification of the channel where the transfer was
 *                being processed
 *
 * @retval 0 if successful.
 * @retval Negative errno code if failure.
 */
static inline int dma_stop(struct device *dev, u32_t channel)
{
	const struct dma_driver_api *api = dev->driver_api;

	return api->stop(dev, channel);
}

/**
 * @brief Look-up generic width index to be used in registers
 *
 * WARNING: This look-up works for most controllers, but *may* not work for
 *          yours.  Ensure your controller expects the most common register
 *          bit values before using this convenience function.  If your
 *          controller does not support these values, you will have to write
 *          your own look-up inside the controller driver.
 *
 * @param size: width of bus (in bytes)
 *
 * @retval common DMA index to be placed into registers.
 */
static inline u32_t dma_width_index(u32_t size)
{
	/* Check boundaries (max supported width is 32 Bytes) */
	if (size < 1 || size > 32) {
		return 0; /* Zero is the default (8 Bytes) */
	}

	/* Ensure size is a power of 2 */
	if (!is_power_of_two(size)) {
		return 0; /* Zero is the default (8 Bytes) */
	}

	/* Convert to bit pattern for writing to a register */
	return find_msb_set(size);
}

/**
 * @brief Look-up generic burst index to be used in registers
 *
 * WARNING: This look-up works for most controllers, but *may* not work for
 *          yours.  Ensure your controller expects the most common register
 *          bit values before using this convenience function.  If your
 *          controller does not support these values, you will have to write
 *          your own look-up inside the controller driver.
 *
 * @param burst: number of bytes to be sent in a single burst
 *
 * @retval common DMA index to be placed into registers.
 */
static inline u32_t dma_burst_index(u32_t burst)
{
	/* Check boundaries (max supported burst length is 256) */
	if (burst < 1 || burst > 256) {
		return 0; /* Zero is the default (1 burst length) */
	}

	/* Ensure burst is a power of 2 */
	if (!(burst & (burst - 1))) {
		return 0; /* Zero is the default (1 burst length) */
	}

	/* Convert to bit pattern for writing to a register */
	return find_msb_set(burst);
}

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* _DMA_H_ */
