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

#ifndef ZEPHYR_INCLUDE_DRIVERS_DMA_H_
#define ZEPHYR_INCLUDE_DRIVERS_DMA_H_

#include <zephyr/kernel.h>
#include <zephyr/device.h>

#ifdef __cplusplus
extern "C" {
#endif


/**
 * @brief DMA Interface
 * @defgroup dma_interface DMA Interface
 * @ingroup io_interfaces
 * @{
 */

enum dma_channel_direction {
	MEMORY_TO_MEMORY = 0x0,
	MEMORY_TO_PERIPHERAL,
	PERIPHERAL_TO_MEMORY,
	PERIPHERAL_TO_PERIPHERAL,
	HOST_TO_MEMORY,
	MEMORY_TO_HOST,

	/**
	 * Number of all common channel directions.
	 */
	DMA_CHANNEL_DIRECTION_COMMON_COUNT,

	/**
	 * This and higher values are dma controller or soc specific.
	 * Refer to the specified dma driver header file.
	 */
	DMA_CHANNEL_DIRECTION_PRIV_START = DMA_CHANNEL_DIRECTION_COMMON_COUNT,

	/**
	 * Maximum allowed value (3 bit field!)
	 */
	DMA_CHANNEL_DIRECTION_MAX = 0x7
};

/** Valid values for @a source_addr_adj and @a dest_addr_adj */
enum dma_addr_adj {
	DMA_ADDR_ADJ_INCREMENT,
	DMA_ADDR_ADJ_DECREMENT,
	DMA_ADDR_ADJ_NO_CHANGE,
};

/* channel attributes */
enum dma_channel_filter {
	DMA_CHANNEL_NORMAL, /* normal DMA channel */
	DMA_CHANNEL_PERIODIC, /* can be triggered by periodic sources */
};

/* DMA attributes */
enum dma_attribute_type {
	DMA_ATTR_BUFFER_ADDRESS_ALIGNMENT,
	DMA_ATTR_BUFFER_SIZE_ALIGNMENT,
	DMA_ATTR_COPY_ALIGNMENT,
	DMA_ATTR_MAX_BLOCK_COUNT,
};

/**
 * @struct dma_block_config
 * @brief DMA block configuration structure.
 *
 * @param source_address is block starting address at source
 * @param source_gather_interval is the address adjustment at gather boundary
 * @param dest_address is block starting address at destination
 * @param dest_scatter_interval is the address adjustment at scatter boundary
 * @param dest_scatter_count is the continuous transfer count between scatter
 *                    boundaries
 * @param source_gather_count is the continuous transfer count between gather
 *                     boundaries
 *
 * @param block_size is the number of bytes to be transferred for this block.
 *
 * @param config is a bit field with the following parts:
 *
 *     source_gather_en   [ 0 ]       - 0-disable, 1-enable.
 *     dest_scatter_en    [ 1 ]       - 0-disable, 1-enable.
 *     source_addr_adj    [ 2 : 3 ]   - 00-increment, 01-decrement,
 *                                      10-no change.
 *     dest_addr_adj      [ 4 : 5 ]   - 00-increment, 01-decrement,
 *                                      10-no change.
 *     source_reload_en   [ 6 ]       - reload source address at the end of
 *                                      block transfer
 *                                      0-disable, 1-enable.
 *     dest_reload_en     [ 7 ]       - reload destination address at the end
 *                                      of block transfer
 *                                      0-disable, 1-enable.
 *     fifo_mode_control  [ 8 : 11 ]  - How full  of the fifo before transfer
 *                                      start. HW specific.
 *     flow_control_mode  [ 12 ]      - 0-source request served upon data
 *                                        availability.
 *                                      1-source request postponed until
 *                                        destination request happens.
 *     reserved           [ 13 : 15 ]
 */
struct dma_block_config {
#ifdef CONFIG_DMA_64BIT
	uint64_t source_address;
	uint64_t dest_address;
#else
	uint32_t source_address;
	uint32_t dest_address;
#endif
	uint32_t source_gather_interval;
	uint32_t dest_scatter_interval;
	uint16_t dest_scatter_count;
	uint16_t source_gather_count;
	uint32_t block_size;
	struct dma_block_config *next_block;
	uint16_t  source_gather_en :  1;
	uint16_t  dest_scatter_en :   1;
	uint16_t  source_addr_adj :   2;
	uint16_t  dest_addr_adj :     2;
	uint16_t  source_reload_en :  1;
	uint16_t  dest_reload_en :    1;
	uint16_t  fifo_mode_control : 4;
	uint16_t  flow_control_mode : 1;
	uint16_t  reserved :          3;
};

/**
 * @typedef dma_callback_t
 * @brief Callback function for DMA transfer completion
 *
 *  If enabled, callback function will be invoked at transfer completion
 *  or when error happens.
 *
 * @param dev Pointer to the DMA device calling the callback.
 * @param user_data A pointer to some user data or NULL
 * @param channel The channel number
 * @param status 0 on success, a negative errno otherwise
 */
typedef void (*dma_callback_t)(const struct device *dev, void *user_data,
			       uint32_t channel, int status);

/**
 * @struct dma_config
 * @brief DMA configuration structure.
 *
 * @param dma_slot             [ 0 : 7 ]   - which peripheral and direction
 *                                        (HW specific)
 * @param channel_direction    [ 8 : 10 ]  - 000-memory to memory,
 *                                        001-memory to peripheral,
 *                                        010-peripheral to memory,
 *                                        011-peripheral to peripheral,
 *                                        100-host to memory
 *                                        101-memory to host
 *                                        ...
 * @param complete_callback_en [ 11 ]       - 0-callback invoked at completion only
 *                                        1-callback invoked at completion of
 *                                          each block
 * @param error_callback_en    [ 12 ]      - 0-error callback enabled
 *                                        1-error callback disabled
 * @param source_handshake     [ 13 ]      - 0-HW, 1-SW
 * @param dest_handshake       [ 14 ]      - 0-HW, 1-SW
 * @param channel_priority     [ 15 : 18 ] - DMA channel priority
 * @param source_chaining_en   [ 19 ]      - enable/disable source block chaining
 *                                        0-disable, 1-enable
 * @param dest_chaining_en     [ 20 ]      - enable/disable destination block
 *                                        chaining.
 *                                        0-disable, 1-enable
 * @param linked_channel       [ 21 : 27 ] - after channel count exhaust will
 *                                        initiate a channel service request
 *                                        at this channel
 * @param cyclic               [ 28 ]      - enable/disable cyclic buffer
 *                                        0-disable, 1-enable
 * @param reserved             [ 29 : 31 ]
 * @param source_data_size    [ 0 : 15 ]   - width of source data (in bytes)
 * @param dest_data_size      [ 16 : 31 ]  - width of dest data (in bytes)
 * @param source_burst_length [ 0 : 15 ]   - number of source data units
 * @param dest_burst_length   [ 16 : 31 ]  - number of destination data units
 * @param block_count  is the number of blocks used for block chaining, this
 *     depends on availability of the DMA controller.
 * @param user_data  private data from DMA client.
 * @param dma_callback see dma_callback_t for details
 */
struct dma_config {
	uint32_t  dma_slot :             8;
	uint32_t  channel_direction :    3;
	uint32_t  complete_callback_en : 1;
	uint32_t  error_callback_en :    1;
	uint32_t  source_handshake :     1;
	uint32_t  dest_handshake :       1;
	uint32_t  channel_priority :     4;
	uint32_t  source_chaining_en :   1;
	uint32_t  dest_chaining_en :     1;
	uint32_t  linked_channel   :     7;
	uint32_t  cyclic :				 1;
	uint32_t  reserved :             3;
	uint32_t  source_data_size :    16;
	uint32_t  dest_data_size :      16;
	uint32_t  source_burst_length : 16;
	uint32_t  dest_burst_length :   16;
	uint32_t block_count;
	struct dma_block_config *head_block;
	void *user_data;
	dma_callback_t dma_callback;
};

/**
 * DMA runtime status structure
 *
 * busy 			- is current DMA transfer busy or idle
 * dir				- DMA transfer direction
 * pending_length 		- data length pending to be transferred in bytes
 * 					or platform dependent.
 * free                         - free buffer space
 * write_position               - write position in a circular dma buffer
 * read_position                - read position in a circular dma buffer
 *
 */
struct dma_status {
	bool busy;
	enum dma_channel_direction dir;
	uint32_t pending_length;
	uint32_t free;
	uint32_t write_position;
	uint32_t read_position;
	uint64_t total_copied;
};

/**
 * DMA context structure
 * Note: the dma_context shall be the first member
 *       of DMA client driver Data, got by dev->data
 *
 * magic			- magic code to identify the context
 * dma_channels		- dma channels
 * atomic			- driver atomic_t pointer
 *
 */
struct dma_context {
	int32_t magic;
	int dma_channels;
	atomic_t *atomic;
};

/* magic code to identify context content */
#define DMA_MAGIC 0x47494749

/**
 * @cond INTERNAL_HIDDEN
 *
 * These are for internal use only, so skip these in
 * public documentation.
 */
typedef int (*dma_api_config)(const struct device *dev, uint32_t channel,
			      struct dma_config *config);

#ifdef CONFIG_DMA_64BIT
typedef int (*dma_api_reload)(const struct device *dev, uint32_t channel,
			      uint64_t src, uint64_t dst, size_t size);
#else
typedef int (*dma_api_reload)(const struct device *dev, uint32_t channel,
			      uint32_t src, uint32_t dst, size_t size);
#endif

typedef int (*dma_api_start)(const struct device *dev, uint32_t channel);

typedef int (*dma_api_stop)(const struct device *dev, uint32_t channel);

typedef int (*dma_api_suspend)(const struct device *dev, uint32_t channel);

typedef int (*dma_api_resume)(const struct device *dev, uint32_t channel);

typedef int (*dma_api_get_status)(const struct device *dev, uint32_t channel,
				  struct dma_status *status);

typedef int (*dma_api_get_attribute)(const struct device *dev, uint32_t type, uint32_t *value);

/**
 * @typedef dma_chan_filter
 * @brief channel filter function call
 *
 * filter function that is used to find the matched internal dma channel
 * provide by caller
 *
 * @param dev Pointer to the DMA device instance
 * @param channel the channel id to use
 * @param filter_param filter function parameter, can be NULL
 *
 * @retval True on filter matched otherwise return False.
 */
typedef bool (*dma_api_chan_filter)(const struct device *dev,
				int channel, void *filter_param);

__subsystem struct dma_driver_api {
	dma_api_config config;
	dma_api_reload reload;
	dma_api_start start;
	dma_api_stop stop;
	dma_api_suspend suspend;
	dma_api_resume resume;
	dma_api_get_status get_status;
	dma_api_get_attribute get_attribute;
	dma_api_chan_filter chan_filter;
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
static inline int dma_config(const struct device *dev, uint32_t channel,
			     struct dma_config *config)
{
	const struct dma_driver_api *api =
		(const struct dma_driver_api *)dev->api;

	return api->config(dev, channel, config);
}

/**
 * @brief Reload buffer(s) for a DMA channel
 *
 * @param dev     Pointer to the device structure for the driver instance.
 * @param channel Numeric identification of the channel to configure
 *                selected channel
 * @param src     source address for the DMA transfer
 * @param dst     destination address for the DMA transfer
 * @param size    size of DMA transfer
 *
 * @retval 0 if successful.
 * @retval Negative errno code if failure.
 */
#ifdef CONFIG_DMA_64BIT
static inline int dma_reload(const struct device *dev, uint32_t channel,
			     uint64_t src, uint64_t dst, size_t size)
#else
static inline int dma_reload(const struct device *dev, uint32_t channel,
		uint32_t src, uint32_t dst, size_t size)
#endif
{
	const struct dma_driver_api *api =
		(const struct dma_driver_api *)dev->api;

	if (api->reload) {
		return api->reload(dev, channel, src, dst, size);
	}

	return -ENOSYS;
}

/**
 * @brief Enables DMA channel and starts the transfer, the channel must be
 *        configured beforehand.
 *
 * Implementations must check the validity of the channel ID passed in and
 * return -EINVAL if it is invalid.
 *
 * Start is allowed on channels that have already been started and must report
 * success.
 *
 * @param dev     Pointer to the device structure for the driver instance.
 * @param channel Numeric identification of the channel where the transfer will
 *                be processed
 *
 * @retval 0 if successful.
 * @retval Negative errno code if failure.
 */
__syscall int dma_start(const struct device *dev, uint32_t channel);

static inline int z_impl_dma_start(const struct device *dev, uint32_t channel)
{
	const struct dma_driver_api *api =
		(const struct dma_driver_api *)dev->api;

	return api->start(dev, channel);
}

/**
 * @brief Stops the DMA transfer and disables the channel.
 *
 * Implementations must check the validity of the channel ID passed in and
 * return -EINVAL if it is invalid.
 *
 * Stop is allowed on channels that have already been stopped and must report
 * success.
 *
 * @param dev     Pointer to the device structure for the driver instance.
 * @param channel Numeric identification of the channel where the transfer was
 *                being processed
 *
 * @retval 0 if successful.
 * @retval Negative errno code if failure.
 */
__syscall int dma_stop(const struct device *dev, uint32_t channel);

static inline int z_impl_dma_stop(const struct device *dev, uint32_t channel)
{
	const struct dma_driver_api *api =
		(const struct dma_driver_api *)dev->api;

	return api->stop(dev, channel);
}


/**
 * @brief Suspend a DMA channel transfer
 *
 * Implementations must check the validity of the channel state and ID passed
 * in and return -EINVAL if either are invalid.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param channel Numeric identification of the channel to suspend
 *
 * @retval 0 If successful.
 * @retval -ENOSYS If not implemented.
 * @retval -EINVAL If invalid channel id or state.
 * @retval -errno Other negative errno code failure.
 */
__syscall int dma_suspend(const struct device *dev, uint32_t channel);

static inline int z_impl_dma_suspend(const struct device *dev, uint32_t channel)
{
	const struct dma_driver_api *api = (const struct dma_driver_api *)dev->api;

	if (api->suspend == NULL) {
		return -ENOSYS;
	}
	return api->suspend(dev, channel);
}

/**
 * @brief Resume a DMA channel transfer
 *
 * Implementations must check the validity of the channel state and ID passed
 * in and return -EINVAL if either are invalid.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param channel Numeric identification of the channel to resume
 *
 * @retval 0 If successful.
 * @retval -ENOSYS If not implemented
 * @retval -EINVAL If invalid channel id or state.
 * @retval -errno Other negative errno code failure.
 */
__syscall int dma_resume(const struct device *dev, uint32_t channel);

static inline int z_impl_dma_resume(const struct device *dev, uint32_t channel)
{
	const struct dma_driver_api *api = (const struct dma_driver_api *)dev->api;

	if (api->resume == NULL) {
		return -ENOSYS;
	}
	return api->resume(dev, channel);
}

/**
 * @brief request DMA channel.
 *
 * request DMA channel resources
 * return -EINVAL if there is no valid channel available.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param filter_param filter function parameter
 *
 * @retval dma channel if successful.
 * @retval Negative errno code if failure.
 */
__syscall int dma_request_channel(const struct device *dev,
				  void *filter_param);

static inline int z_impl_dma_request_channel(const struct device *dev,
					     void *filter_param)
{
	int i = 0;
	int channel = -EINVAL;
	const struct dma_driver_api *api =
		(const struct dma_driver_api *)dev->api;
	/* dma_context shall be the first one in dev data */
	struct dma_context *dma_ctx = (struct dma_context *)dev->data;

	if (dma_ctx->magic != DMA_MAGIC) {
		return channel;
	}

	for (i = 0; i < dma_ctx->dma_channels; i++) {
		if (!atomic_test_and_set_bit(dma_ctx->atomic, i)) {
			if (api->chan_filter &&
			    !api->chan_filter(dev, i, filter_param)) {
				atomic_clear_bit(dma_ctx->atomic, i);
				continue;
			}
			channel = i;
			break;
		}
	}

	return channel;
}

/**
 * @brief release DMA channel.
 *
 * release DMA channel resources
 *
 * @param dev  Pointer to the device structure for the driver instance.
 * @param channel  channel number
 *
 */
__syscall void dma_release_channel(const struct device *dev,
				   uint32_t channel);

static inline void z_impl_dma_release_channel(const struct device *dev,
					      uint32_t channel)
{
	struct dma_context *dma_ctx = (struct dma_context *)dev->data;

	if (dma_ctx->magic != DMA_MAGIC) {
		return;
	}

	if ((int)channel < dma_ctx->dma_channels) {
		atomic_clear_bit(dma_ctx->atomic, channel);
	}

}

/**
 * @brief DMA channel filter.
 *
 * filter channel by attribute
 *
 * @param dev  Pointer to the device structure for the driver instance.
 * @param channel  channel number
 * @param filter_param filter attribute
 *
 * @retval Negative errno code if not support
 *
 */
__syscall int dma_chan_filter(const struct device *dev,
				   int channel, void *filter_param);

static inline int z_impl_dma_chan_filter(const struct device *dev,
					      int channel, void *filter_param)
{
	const struct dma_driver_api *api =
		(const struct dma_driver_api *)dev->api;

	if (api->chan_filter) {
		return api->chan_filter(dev, channel, filter_param);
	}

	return -ENOSYS;
}

/**
 * @brief get current runtime status of DMA transfer
 *
 * Implementations must check the validity of the channel ID passed in and
 * return -EINVAL if it is invalid or -ENOSYS if not supported.
 *
 * @param dev     Pointer to the device structure for the driver instance.
 * @param channel Numeric identification of the channel where the transfer was
 *                being processed
 * @param stat   a non-NULL dma_status object for storing DMA status
 *
 * @retval non-negative if successful.
 * @retval Negative errno code if failure.
 */
static inline int dma_get_status(const struct device *dev, uint32_t channel,
				 struct dma_status *stat)
{
	const struct dma_driver_api *api =
		(const struct dma_driver_api *)dev->api;

	if (api->get_status) {
		return api->get_status(dev, channel, stat);
	}

	return -ENOSYS;
}

/**
 * @brief get attribute of a dma controller
 *
 * This function allows to get a device specific static or runtime attribute like required address
 * and size alignment of a buffer.
 * Implementations must check the validity of the type passed in and
 * return -EINVAL if it is invalid or -ENOSYS if not supported.
 *
 * @param dev     Pointer to the device structure for the driver instance.
 * @param type    Numeric identification of the attribute
 * @param value   A non-NULL pointer to the variable where the read value is to be placed
 *
 * @retval non-negative if successful.
 * @retval Negative errno code if failure.
 */
static inline int dma_get_attribute(const struct device *dev, uint32_t type, uint32_t *value)
{
	const struct dma_driver_api *api = (const struct dma_driver_api *)dev->api;

	if (api->get_attribute) {
		return api->get_attribute(dev, type, value);
	}

	return -ENOSYS;
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
static inline uint32_t dma_width_index(uint32_t size)
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
static inline uint32_t dma_burst_index(uint32_t burst)
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
 * Get the device tree property describing the buffer address alignment
 *
 * Useful when statically defining or allocating buffers for DMA usage where
 * memory alignment often matters.
 *
 * @param node Node identifier, e.g. DT_NODELABEL(dma_0)
 * @return alignment Memory byte alignment required for DMA buffers
 */
#define DMA_BUF_ADDR_ALIGNMENT(node) DT_PROP(node, dma_buf_addr_alignment)

/**
 * Get the device tree property describing the buffer size alignment
 *
 * Useful when statically defining or allocating buffers for DMA usage where
 * memory alignment often matters.
 *
 * @param node Node identifier, e.g. DT_NODELABEL(dma_0)
 * @return alignment Memory byte alignment required for DMA buffers
 */
#define DMA_BUF_SIZE_ALIGNMENT(node) DT_PROP(node, dma_buf_size_alignment)

/**
 * Get the device tree property describing the minimal chunk of data possible to be copied
 *
 * @param node Node identifier, e.g. DT_NODELABEL(dma_0)
 * @return minimal Minimal chunk of data possible to be copied
 */
#define DMA_COPY_ALIGNMENT(node) DT_PROP(node, dma_copy_alignment)

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#include <syscalls/dma.h>

#endif /* ZEPHYR_INCLUDE_DRIVERS_DMA_H_ */
