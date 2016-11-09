/**
 * @file
 *
 * @brief Public APIs for the DMA drivers.
 */

/*
 * Copyright (c) 2016 Intel Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
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

enum dma_handshake_polarity {
	HANDSHAKE_POLARITY_HIGH = 0x0,
	HANDSHAKE_POLARITY_LOW
};

enum dma_burst_length {
	BURST_TRANS_LENGTH_1 = 0x0,
	BURST_TRANS_LENGTH_4,
	BURST_TRANS_LENGTH_8,
	BURST_TRANS_LENGTH_16,
	BURST_TRANS_LENGTH_32,
	BURST_TRANS_LENGTH_64,
	BURST_TRANS_LENGTH_128,
	BURST_TRANS_LENGTH_256
};

enum dma_transfer_width {
	TRANS_WIDTH_8 = 0x0,
	TRANS_WIDTH_16,
	TRANS_WIDTH_32,
	TRANS_WIDTH_64,
	TRANS_WIDTH_128,
	TRANS_WIDTH_256
};

enum dma_channel_direction {
	MEMORY_TO_MEMORY = 0x0,
	MEMORY_TO_PERIPHERAL,
	PERIPHERAL_TO_MEMORY
};

/**
 * @brief DMA Channel Configuration.
 *
 * This defines a single channel configuration on the DMA controller.
 */
struct dma_channel_config {
	/* Hardware Interface handshake for peripheral (I2C, SPI, etc) */
	uint32_t handshake_interface;
	/* Select active  polarity for handshake (low/high) */
	enum dma_handshake_polarity handshake_polarity;
	/* DMA transfer direction from mem/peripheral to mem/peripheral */
	enum dma_channel_direction channel_direction;
	/* Data item size read from source */
	enum dma_transfer_width source_transfer_width;
	/* Data item size written to destination */
	enum dma_transfer_width destination_transfer_width;
	/* Number of data items read */
	enum dma_burst_length source_burst_length;
	/* Number of data items written */
	enum dma_burst_length destination_burst_length;

	/* Completed transaction callback */
	void (*dma_transfer)(struct device *dev, void *data);
	/* Error callback */
	void (*dma_error)(struct device *dev, void *data);
	/* Client callback private data */
	void *callback_data;
};

/**
 * @brief DMA transfer Configuration.
 *
 * This defines a single transfer on configured channel of the DMA controller.
 */
struct dma_transfer_config {
	/* Total amount of data in bytes to transfer */
	uint32_t block_size;
	/* Source address for the transaction */
	uint32_t *source_address;
	/* Destination address */
	uint32_t *destination_address;
};

/**
 * @cond INTERNAL_HIDDEN
 *
 * These are for internal use only, so skip these in
 * public documentation.
 */

typedef int (*dma_api_channel_config)(struct device *dev, uint32_t channel,
				      struct dma_channel_config *config);

typedef int (*dma_api_transfer_config)(struct device *dev, uint32_t channel,
				       struct dma_transfer_config *config);

typedef int (*dma_api_transfer_start)(struct device *dev, uint32_t channel);

typedef int (*dma_api_transfer_stop)(struct device *dev, uint32_t channel);

struct dma_driver_api {
	dma_api_channel_config channel_config;
	dma_api_transfer_config transfer_config;
	dma_api_transfer_start transfer_start;
	dma_api_transfer_stop transfer_stop;
};
/**
 * @endcond
 */

/**
 * @brief Configure individual channel for DMA transfer.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param channel Numeric identification of the channel to configure
 * @param config Data structure containing the intended configuration for the
 * selected channel
 *
 * @retval 0 If successful.
 * @retval Negative errno code if failure.
 */
static inline int dma_channel_config(struct device *dev, uint32_t channel,
				     struct dma_channel_config *config)
{
	const struct dma_driver_api *api = dev->driver_api;

	return api->channel_config(dev, channel, config);
}

/**
 * @brief Configure DMA transfer for a specific channel that has been
 * configured.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param channel Numeric identification of the channel to configure
 * @param config Data structure containing transfer configuration for the
 * selected channel
 *
 * @retval 0 If successful.
 * @retval Negative errno code if failure.
 */
static inline int dma_transfer_config(struct device *dev, uint32_t channel,
				      struct dma_transfer_config *config)
{
	const struct dma_driver_api *api = dev->driver_api;

	return api->transfer_config(dev, channel, config);
}

/**
 * @brief Enables DMA channel and starts the transfer, the channel must be
 * configured beforehand.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param channel Numeric identification of the channel where the transfer will
 * be processed
 *
 * @retval 0 If successful.
 * @retval Negative errno code if failure.
 */
static inline int dma_transfer_start(struct device *dev, uint32_t channel)
{
	const struct dma_driver_api *api = dev->driver_api;

	return api->transfer_start(dev, channel);
}

/**
 * @brief Stops the DMA transfer and disables the channel.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param channel Numeric identification of the channel where the transfer was
 * being processed
 *
 * @retval 0 If successful.
 * @retval Negative errno code if failure.
 */
static inline int dma_transfer_stop(struct device *dev, uint32_t channel)
{
	const struct dma_driver_api *api = dev->driver_api;

	return api->transfer_stop(dev, channel);
}

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* _DMA_H_ */
