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
 * @since 1.5
 * @version 1.0.0
 * @ingroup io_interfaces
 * @{
 */

/**
 * @brief DMA channel direction
 */
enum dma_channel_direction {
	/** Memory to memory */
	MEMORY_TO_MEMORY = 0x0,
	/** Memory to peripheral */
	MEMORY_TO_PERIPHERAL,
	/** Peripheral to memory */
	PERIPHERAL_TO_MEMORY,
	/** Peripheral to peripheral */
	PERIPHERAL_TO_PERIPHERAL,
	/** Host to memory */
	HOST_TO_MEMORY,
	/** Memory to host */
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

/**
 * @brief DMA address adjustment
 *
 * Valid values for @a source_addr_adj and @a dest_addr_adj
 */
enum dma_addr_adj {
	/** Increment the address */
	DMA_ADDR_ADJ_INCREMENT,
	/** Decrement the address */
	DMA_ADDR_ADJ_DECREMENT,
	/** No change the address */
	DMA_ADDR_ADJ_NO_CHANGE,
};

/**
 * @brief DMA channel attributes
 */
enum dma_channel_filter {
	DMA_CHANNEL_NORMAL, /* normal DMA channel */
	DMA_CHANNEL_PERIODIC, /* can be triggered by periodic sources */
};

/**
 * @brief DMA attributes
 */
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
 * Aside from source address, destination address, and block size many of these options are hardware
 * and driver dependent.
 */
struct dma_block_config {
#ifdef CONFIG_DMA_64BIT
	/** block starting address at source */
	uint64_t source_address;
	/** block starting address at destination */
	uint64_t dest_address;
#else
	/** block starting address at source */
	uint32_t source_address;
	/** block starting address at destination */
	uint32_t dest_address;
#endif
	/** Address adjustment at gather boundary */
	uint32_t source_gather_interval;
	/** Address adjustment at scatter boundary */
	uint32_t dest_scatter_interval;
	/** Continuous transfer count between scatter boundaries */
	uint16_t dest_scatter_count;
	/** Continuous transfer count between gather boundaries */
	uint16_t source_gather_count;
	/** Number of bytes to be transferred for this block */
	uint32_t block_size;
	/** Pointer to next block in a transfer list */
	struct dma_block_config *next_block;
	/** Enable source gathering when set to 1 */
	uint16_t  source_gather_en :  1;
	/** Enable destination scattering when set to 1 */
	uint16_t  dest_scatter_en :   1;
	/**
	 * Source address adjustment option
	 *
	 * - 0b00 increment
	 * - 0b01 decrement
	 * - 0b10 no change
	 */
	uint16_t  source_addr_adj :   2;
	/**
	 * Destination address adjustment
	 *
	 * - 0b00 increment
	 * - 0b01 decrement
	 * - 0b10 no change
	 */
	uint16_t  dest_addr_adj :     2;
	/** Reload source address at the end of block transfer */
	uint16_t  source_reload_en :  1;
	/** Reload destination address at the end of block transfer */
	uint16_t  dest_reload_en :    1;
	/** FIFO fill before starting transfer, HW specific meaning */
	uint16_t  fifo_mode_control : 4;
	/**
	 * Transfer flow control mode
	 *
	 * - 0b0 source request service upon data availability
	 * - 0b1 source request postponed until destination request happens
	 */
	uint16_t  flow_control_mode : 1;

	uint16_t  _reserved :          3;
};

/** The DMA callback event has occurred at the completion of a transfer list */
#define DMA_STATUS_COMPLETE	0
/** The DMA callback has occurred at the completion of a single transfer block in a transfer list */
#define DMA_STATUS_BLOCK	1

/**
 * @typedef dma_callback_t
 * @brief Callback function for DMA transfer completion
 *
 *  If enabled, callback function will be invoked at transfer or block completion,
 *  or when an error happens.
 *  In circular mode, @p status indicates that the DMA device has reached either
 *  the end of the buffer (DMA_STATUS_COMPLETE) or a water mark (DMA_STATUS_BLOCK).
 *
 * @param dev           Pointer to the DMA device calling the callback.
 * @param user_data     A pointer to some user data or NULL
 * @param channel       The channel number
 * @param status        Status of the transfer
 *                      - DMA_STATUS_COMPLETE buffer fully consumed
 *                      - DMA_STATUS_BLOCK buffer consumption reached a configured block
 *                        or water mark
 *                      - A negative errno otherwise
 */
typedef void (*dma_callback_t)(const struct device *dev, void *user_data,
			       uint32_t channel, int status);

/**
 * @struct dma_config
 * @brief DMA configuration structure.
 */
struct dma_config {
	/** Which peripheral and direction, HW specific */
	uint32_t  dma_slot :             8;
	/**
	 * Direction the transfers are occurring
	 *
	 * - 0b000 memory to memory,
	 * - 0b001 memory to peripheral,
	 * - 0b010 peripheral to memory,
	 * - 0b011 peripheral to peripheral,
	 * - 0b100 host to memory
	 * - 0b101 memory to host
	 * - others hardware specific
	 */
	uint32_t  channel_direction :    3;
	/**
	 * Completion callback enable
	 *
	 * - 0b0 callback invoked at transfer list completion only
	 * - 0b1 callback invoked at completion of each block
	 */
	uint32_t  complete_callback_en : 1;
	/**
	 * Error callback enable
	 *
	 * - 0b0 error callback enabled
	 * - 0b1 error callback disabled
	 */
	uint32_t  error_callback_en :    1;
	/**
	 * Source handshake, HW specific
	 *
	 * - 0b0 HW
	 * - 0b1 SW
	 */
	uint32_t  source_handshake :     1;
	/**
	 * Destination handshake, HW specific
	 *
	 * - 0b0 HW
	 * - 0b1 SW
	 */
	uint32_t  dest_handshake :       1;
	/**
	 * Channel priority for arbitration, HW specific
	 */
	uint32_t  channel_priority :     4;
	/** Source chaining enable, HW specific */
	uint32_t  source_chaining_en :   1;
	/** Destination chaining enable, HW specific */
	uint32_t  dest_chaining_en :     1;
	/** Linked channel, HW specific */
	uint32_t  linked_channel   :     7;
	/** Cyclic transfer list, HW specific */
	uint32_t  cyclic :				 1;

	uint32_t  _reserved :             3;
	/** Width of source data (in bytes) */
	uint32_t  source_data_size :    16;
	/** Width of destination data (in bytes) */
	uint32_t  dest_data_size :      16;
	/** Source burst length in bytes */
	uint32_t  source_burst_length : 16;
	/** Destination burst length in bytes */
	uint32_t  dest_burst_length :   16;
	/** Number of blocks in transfer list */
	uint32_t block_count;
	/** Pointer to the first block in the transfer list */
	struct dma_block_config *head_block;
	/** Optional attached user data for callbacks */
	void *user_data;
	/** Optional callback for completion and error events */
	dma_callback_t dma_callback;
};

/**
 * DMA runtime status structure
 */
struct dma_status {
	/** Is the current DMA transfer busy or idle */
	bool busy;
	/** Direction fo the transfer */
	enum dma_channel_direction dir;
	/** Pending length to be transferred in bytes, HW specific */
	uint32_t pending_length;
	/** Available buffers space, HW specific */
	uint32_t free;
	/** Write position in circular DMA buffer, HW specific */
	uint32_t write_position;
	/** Read position in circular DMA buffer, HW specific */
	uint32_t read_position;
	/** Total copied, HW specific */
	uint64_t total_copied;
};

/**
 * DMA context structure
 * Note: the dma_context shall be the first member
 *       of DMA client driver Data, got by dev->data
 */
struct dma_context {
	/** magic code to identify the context */
	int32_t magic;
	/** number of dma channels */
	int dma_channels;
	/** atomic holding bit flags for each channel to mark as used/unused */
	atomic_t *atomic;
};

/** Magic code to identify context content */
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
 * @funcprops \isr_ok
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
 * @funcprops \isr_ok
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
 * @funcprops \isr_ok
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
 * @funcprops \isr_ok
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
 * @funcprops \isr_ok
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
 * @funcprops \isr_ok
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
 * @funcprops \isr_ok
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
 * @funcprops \isr_ok
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
 * @warning This look-up works for most controllers, but *may* not work for
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
 * @warning This look-up works for most controllers, but *may* not work for
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
 * @brief Get the device tree property describing the buffer address alignment
 *
 * Useful when statically defining or allocating buffers for DMA usage where
 * memory alignment often matters.
 *
 * @param node Node identifier, e.g. DT_NODELABEL(dma_0)
 * @return alignment Memory byte alignment required for DMA buffers
 */
#define DMA_BUF_ADDR_ALIGNMENT(node) DT_PROP(node, dma_buf_addr_alignment)

/**
 * @brief Get the device tree property describing the buffer size alignment
 *
 * Useful when statically defining or allocating buffers for DMA usage where
 * memory alignment often matters.
 *
 * @param node Node identifier, e.g. DT_NODELABEL(dma_0)
 * @return alignment Memory byte alignment required for DMA buffers
 */
#define DMA_BUF_SIZE_ALIGNMENT(node) DT_PROP(node, dma_buf_size_alignment)

/**
 * @brief Get the device tree property describing the minimal chunk of data possible to be copied
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
