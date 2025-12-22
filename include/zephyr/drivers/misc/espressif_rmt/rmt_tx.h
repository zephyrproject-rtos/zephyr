/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_MISC_ESPRESSIF_RMT_RMT_TX_H_
#define ZEPHYR_DRIVERS_MISC_ESPRESSIF_RMT_RMT_TX_H_

/**
 * @brief Espressif RMT TX functions.
 * @defgroup espressif_rmt_tx_interface Espressif RMT TX interface
 * @ingroup espressif_rmt
 * @{
 */

#include <stdbool.h>
#include <stdint.h>
#include "rmt_common.h"
#include "rmt_encoder.h"

#include <zephyr/kernel.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Group of RMT TX callbacks
 * @note The callbacks are all running under ISR environment
 * @note When CONFIG_ESPRESSIF_RMT_ISR_IRAM_SAFE is enabled, the callback itself and functions
 *       called by it should be placed in IRAM.
 *	     The variables used in the function should be in the SRAM as well.
 */
typedef struct {
	/* !< Event callback, invoked when transmission is finished */
	rmt_tx_done_callback_t on_trans_done;
} rmt_tx_event_callbacks_t;

/**
 * @brief RMT TX channel specific configuration
 */
typedef struct {
	/* !< GPIO pinmux used by RMT TX channel */
	int gpio_pinmux;
	/* !< Clock source of RMT TX channel, channels in the same group must use the same clock
	 * source
	 */
	rmt_clock_source_t clk_src;
	/* !< Channel clock resolution, in Hz */
	uint32_t resolution_hz;
	/* !< Size of memory block, in number of `rmt_symbol_word_t`, must be an even.
	 * In the DMA mode, this field controls the DMA buffer size, it can be set to a large value;
	 * In the normal mode, this field controls the number of RMT memory block that will be used
	 * by the channel.
	 */
	size_t mem_block_symbols;
	/* !< Depth of internal transfer queue, increase this value can support more transfers
	 * pending in the background
	 */
	size_t trans_queue_depth;
	/* !< RMT interrupt priority, if set to 0, the driver will try to allocate an interrupt with
	 * a relative low priority (1,2,3)
	 */
	int intr_priority;
} rmt_tx_channel_config_t;

/**
 * @brief RMT transmit specific configuration
 */
typedef struct {
	/* !< Specify the times of transmission in a loop, -1 means transmitting in an infinite
	 * loop
	 */
	int loop_count;
	/* !< Transmit specific config flags */
	struct {
		/* !< Set the output level for the "End Of Transmission" */
		uint32_t eot_level : 1;
		/* !< If set, when the transaction queue is full, driver will not block the thread
		 * but return directly
		 */
		uint32_t queue_nonblocking : 1;
	} flags;
} rmt_transmit_config_t;

/**
 * @brief Synchronous manager configuration
 */
typedef struct {
	/* !< Array of TX channels that are about to be managed by a synchronous controller */
	const rmt_channel_handle_t *tx_channel_array;
	/* !< Size of the `tx_channel_array` */
	size_t array_size;
} rmt_sync_manager_config_t;

/**
 * @brief Create a RMT TX channel.
 *
 * @param dev RMT device instance.
 * @param config TX channel configurations.
 * @param ret_chan Returned generic RMT channel handle.
 * @retval 0 If successful.
 * @retval -EINVAL if creating RMT TX channel failed because of invalid argument.
 * @retval -ENOMEM if creating RMT TX channel failed because out of memory, or all RMT channels
 *         are used up and no more free one.
 * @retval -ENODEV if creating RMT TX channel failed because some feature is not supported by
 *         hardware, e.g. DMA feature is not supported by hardware, or other error.
 */
int rmt_new_tx_channel(const struct device *dev, const rmt_tx_channel_config_t *config,
	rmt_channel_handle_t *ret_chan);

/**
 * @brief Transmit data by RMT TX channel.
 *
 * @note This function constructs a transaction descriptor then pushes to a queue.
 *       The transaction will not start immediately if there's another one under processing.
 *       Based on the setting of `rmt_transmit_config_t::queue_nonblocking`,
 *       if there're too many transactions pending in the queue, this function can block until it
 *       has free slot, otherwise just return quickly.
 * @note The data to be transmitted will be encoded into RMT symbols by the specific `encoder`.
 *
 * @param tx_channel RMT TX channel that created by `rmt_new_tx_channel()`.
 * @param encoder RMT encoder that created by various factory APIs like `rmt_new_bytes_encoder()`.
 * @param payload The raw data to be encoded into RMT symbols.
 * @param payload_bytes Size of the `payload` in bytes.
 * @param config Transmission specific configuration.
 * @retval 0 If successful.
 * @retval -EINVAL Transmit data failed because of invalid argument.
 * @retval -ENODEV Transmit data failed because channel is not enabled, or some feature is not
 *         supported by hardware, e.g. unsupported loop count, or other error.
 */
int rmt_transmit(rmt_channel_handle_t tx_channel, rmt_encoder_handle_t encoder,
	const void *payload, size_t payload_bytes, const rmt_transmit_config_t *config);

/**
 * @brief Wait for all pending TX transactions done.
 *
 * @note This function will block forever if the pending transaction can't be finished within a
 *       limited time (e.g. an infinite loop transaction).
 *       See also `rmt_disable()` for how to terminate a working channel.
 *
 * @param tx_channel RMT TX channel that created by `rmt_new_tx_channel()`.
 * @param timeout_ms Wait timeout, in ms. Specially, -1 means to wait forever.
 * @retval 0 If successful.
 * @retval -EINVAL Flush transactions failed because of invalid argument.
 * @retval -ETIMEDOUT Flush transactions failed because of timeout.
 * @retval -ENODEV Flush transactions failed because of other error.
 */
int rmt_tx_wait_all_done(rmt_channel_handle_t tx_channel, k_timeout_t timeout_ms);

/**
 * @brief Set event callbacks for RMT TX channel.
 *
 * @note User can deregister a previously registered callback by calling this function and setting
 *       the callback member in the `cbs` structure to NULL.
 * @note When CONFIG_ESPRESSIF_RMT_ISR_IRAM_SAFE is enabled, the callback itself and functions
 *       called by it should be placed in IRAM.
 *       The variables used in the function should be in the SRAM as well. The `user_data` should
 *       also reside in SRAM.
 *
 * @param tx_channel RMT generic channel that created by `rmt_new_tx_channel()`.
 * @param cbs Group of callback functions.
 * @param user_data User data, which will be passed to callback functions directly.
 * @retval 0 If successful.
 * @retval -EINVAL Set event callbacks failed because of invalid argument.
 * @retval -ENODEV Set event callbacks failed because of other error.
 */
int rmt_tx_register_event_callbacks(rmt_channel_handle_t tx_channel,
	const rmt_tx_event_callbacks_t *cbs, void *user_data);

/**
 * @brief Create a synchronization manager for multiple TX channels, so that the managed channel
 *        can start transmitting at the same time.
 *
 * @note All the channels to be managed should be enabled by `rmt_enable()` before put them into
 *       sync manager.
 *
 * @param config Synchronization manager configuration.
 * @param ret_synchro Returned synchronization manager handle.
 * @retval 0 If successful.
 * @retval -EINVAL Create sync manager failed because of invalid argument.
 * @retval -ENOMEM Create sync manager failed because out of memory, or all sync controllers are
 *         used up and no more free one.
 * @retval -ENODEV Create sync manager failed because it is not supported by hardware, or not all
 *         channels are enabled, or other error.
 */
int rmt_new_sync_manager(const rmt_sync_manager_config_t *config,
	rmt_sync_manager_handle_t *ret_synchro);

/**
 * @brief Delete synchronization manager.
 *
 * @param synchro Synchronization manager handle returned from `rmt_new_sync_manager()`.
 * @retval 0 If successful.
 * @retval -EINVAL delete the synchronization manager failed because of invalid argument.
 * @retval -ENODEV delete the synchronization manager failed because of other error.
 */
int rmt_del_sync_manager(rmt_sync_manager_handle_t synchro);

/**
 * @brief Reset synchronization manager.
 *
 * @param synchro Synchronization manager handle returned from `rmt_new_sync_manager()`.
 * @retval 0 If successful.
 * @retval -EINVAL reset the synchronization manager failed because of invalid argument.
 * @retval -ENODEV reset the synchronization manager failed because of other error.
 */
int rmt_sync_reset(rmt_sync_manager_handle_t synchro);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_DRIVERS_MISC_ESPRESSIF_RMT_RMT_TX_H_ */
