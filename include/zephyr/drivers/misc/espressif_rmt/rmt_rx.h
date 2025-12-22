/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_MISC_ESPRESSIF_RMT_RMT_RX_H_
#define ZEPHYR_DRIVERS_MISC_ESPRESSIF_RMT_RMT_RX_H_

/**
 * @brief Espressif RMT RX functions.
 * @defgroup espressif_rmt_rx_interface Espressif RMT RX interface
 * @ingroup espressif_rmt
 * @{
 */

#include <stdbool.h>
#include <stdint.h>
#include "rmt_common.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Group of RMT RX callbacks
 * @note The callbacks are all running under ISR environment
 * @note When CONFIG_ESPRESSIF_RMT_ISR_IRAM_SAFE is enabled, the callback itself and functions
 *       called by it should be placed in IRAM.
 *       The variables used in the function should be in the SRAM as well.
 */
typedef struct {
	/* !< Event callback, invoked when one RMT channel receiving transaction completes */
	rmt_rx_done_callback_t on_recv_done;
} rmt_rx_event_callbacks_t;

/**
 * @brief RMT RX channel specific configuration
 */
typedef struct {
	/* !< GPIO pinmux used by RMT RX channel */
	int gpio_pinmux;
	/* !< Clock source of RMT RX channel, channels in the same group must use the same clock
	 * source
	 */
	rmt_clock_source_t clk_src;
	/* !< Channel clock resolution, in Hz */
	uint32_t resolution_hz;
	/* !< Size of memory block, in number of `rmt_symbol_word_t`, must be an even.
	 * In the DMA mode, this field controls the DMA buffer size, it can be set to a large value
	 * (e.g. 1024);
	 * In the normal mode, this field controls the number of RMT memory block that will be used
	 * by the channel.
	 */
	size_t mem_block_symbols;
	/* !< RMT interrupt priority, if set to 0, the driver will try to allocate an interrupt with
	 * a relative low priority (1,2,3)
	 */
	int intr_priority;
} rmt_rx_channel_config_t;

/**
 * @brief RMT receive specific configuration
 */
typedef struct {
	/* !< A pulse whose width is smaller than this threshold will be treated as glitch and
	 * ignored
	 */
	uint32_t signal_range_min_ns;
	/* !< RMT will stop receiving if one symbol level has kept more than
	 * `signal_range_max_ns`
	 */
	uint32_t signal_range_max_ns;
} rmt_receive_config_t;

/**
 * @brief Create a RMT RX channel.
 *
 * @param dev RMT device instance.
 * @param config RX channel configurations.
 * @param ret_chan Returned generic RMT channel handle.
 * @retval 0 If successful.
 * @retval -EINVAL Create RMT RX channel failed because of invalid argument.
 * @retval -ENOMEM Create RMT RX channel failed because out of memory.
 * @retval -ENODEV Create RMT RX channel failed because all RMT channels are used up and no more
 *         free one, or some feature is not supported by hardware, e.g. DMA feature is not
 *         supported by hardware, or other error.
 */
int rmt_new_rx_channel(const struct device *dev, const rmt_rx_channel_config_t *config,
	rmt_channel_handle_t *ret_chan);

/**
 * @brief Initiate a receive job for RMT RX channel.
 *
 * @note This function is non-blocking, it initiates a new receive job and then returns.
 *       User should check the received data from the `on_recv_done` callback that registered by
 *       `rmt_rx_register_event_callbacks()`.
 * @note This function can also be called in ISR context.
 * @note If you want this function to work even when the flash cache is disabled, please enable
 *       the `CONFIG_ESPRESSIF_RMT_RECV_FUNC_IN_IRAM` option.
 *
 * @param rx_channel RMT RX channel that created by `rmt_new_rx_channel()`.
 * @param buffer The buffer to store the received RMT symbols.
 * @param buffer_size size of the `buffer`, in bytes.
 * @param config Receive specific configurations.
 * @retval 0 If successful.
 * @retval -EINVAL Initiate receive job failed because of invalid argument.
 * @retval -ENODEV Initiate receive job failed because channel is not enabled, or other error.
 */
int rmt_receive(rmt_channel_handle_t rx_channel, void *buffer, size_t buffer_size,
	const rmt_receive_config_t *config);

/**
 * @brief Set callbacks for RMT RX channel.
 *
 * @note User can deregister a previously registered callback by calling this function and setting
 *       the callback member in the `cbs` structure to NULL.
 * @note When CONFIG_ESPRESSIF_RMT_ISR_IRAM_SAFE is enabled, the callback itself and functions
 *       called by it should be placed in IRAM.
 *       The variables used in the function should be in the SRAM as well. The `user_data` should
 *       also reside in SRAM.
 *
 * @param rx_channel RMT generic channel that created by `rmt_new_rx_channel()`.
 * @param cbs Group of callback functions.
 * @param user_data User data, which will be passed to callback functions directly.
 * @retval 0 If successful.
 * @retval -EINVAL Set event callbacks failed because of invalid argument.
 * @retval -ENODEV Set event callbacks failed because of other error.
 */
int rmt_rx_register_event_callbacks(rmt_channel_handle_t rx_channel,
	const rmt_rx_event_callbacks_t *cbs, void *user_data);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_DRIVERS_MISC_ESPRESSIF_RMT_RMT_RX_H_ */
