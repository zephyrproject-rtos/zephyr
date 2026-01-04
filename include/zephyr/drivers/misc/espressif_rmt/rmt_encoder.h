/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Espressif RMT encoder public API
 * @ingroup espressif_rmt_encoder_interface
 */

#ifndef ZEPHYR_DRIVERS_MISC_ESPRESSIF_RMT_RMT_ENCODER_H_
#define ZEPHYR_DRIVERS_MISC_ESPRESSIF_RMT_RMT_ENCODER_H_

/**
 * @brief Espressif RMT encoder functions.
 * @defgroup espressif_rmt_encoder_interface Espressif RMT Encoder interface
 * @ingroup misc_interfaces
 * @{
 */

#include <stdint.h>
#include "hal/rmt_types.h"
#include "rmt_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/** @cond */
typedef struct rmt_encoder_t rmt_encoder_t;
/** @endcond */

/**
 * @brief RMT encoding state
 */
typedef enum {
    RMT_ENCODING_RESET = 0,           /**< The encoding session is in reset state */
    RMT_ENCODING_COMPLETE = (1 << 0), /**< The encoding session is finished, the caller can continue with subsequent encoding */
    RMT_ENCODING_MEM_FULL = (1 << 1), /**< The encoding artifact memory is full, the caller should return from current encoding session */
} rmt_encode_state_t;

/**
 * @brief Interface of RMT encoder
 */
struct rmt_encoder_t {
    /**
     * @brief Encode the user data into RMT symbols and write into RMT memory.
     *
     * @note The encoding function will also be called from an ISR context, thus the function must not call any blocking API.
     * @note It's recommended to put this function implementation in the IRAM, to achieve a high performance and less interrupt latency.
     *
     * @param encoder Encoder handle.
     * @param tx_channel RMT TX channel handle, returned from `rmt_new_tx_channel()`.
     * @param primary_data App data to be encoded into RMT symbols.
     * @param data_size Size of primary_data, in bytes.
     * @param ret_state Returned current encoder's state.
     * @retval Number of RMT symbols that the primary data has been encoded into.
     */
    size_t (*encode)(rmt_encoder_t *encoder, rmt_channel_handle_t tx_channel, const void *primary_data, size_t data_size, rmt_encode_state_t *ret_state);

    /**
     * @brief Reset encoding state.
     *
     * @param encoder Encoder handle.
     * @retval 0 If successful.
     * @retval -ENODEV if reset encoder failed.
     */
    int (*reset)(rmt_encoder_t *encoder);

    /**
     * @brief Delete encoder object.
     *
     * @param encoder Encoder handle.
     * @retval 0 If successful.
     * @retval -ENODEV if delete encoder failed.
     */
    int (*del)(rmt_encoder_t *encoder);
};

/**
 * @brief Bytes encoder configuration
 */
typedef struct {
    rmt_symbol_word_t bit0; /**< How to represent BIT0 in RMT symbol */
    rmt_symbol_word_t bit1; /**< How to represent BIT1 in RMT symbol */
    struct {
        uint32_t msb_first: 1; /**< Whether to encode MSB bit first */
    } flags;                   /**< Encoder config flag */
} rmt_bytes_encoder_config_t;

/**
 * @brief Copy encoder configuration
 */
typedef struct {
} rmt_copy_encoder_config_t;

/**
 * @brief Create RMT bytes encoder, which can encode byte stream into RMT symbols.
 *
 * @param config Bytes encoder configuration.
 * @param ret_encoder Returned encoder handle.
 * @retval 0 If successful.
 * @retval -EINVAL Create RMT bytes encoder failed because of invalid argument.
 * @retval -ENOMEM Create RMT bytes encoder failed because out of memory.
 * @retval -ENODEV Create RMT bytes encoder failed because of other error.
 */
int rmt_new_bytes_encoder(const rmt_bytes_encoder_config_t *config, rmt_encoder_handle_t *ret_encoder);

/**
 * @brief Update the configuration of the bytes encoder.
 *
 * @note The configurations of the bytes encoder is also set up by `rmt_new_bytes_encoder()`.
 *       This function is used to update the configuration of the bytes encoder at runtime.
 *
 * @param bytes_encoder Bytes encoder handle, created by e.g `rmt_new_bytes_encoder()`.
 * @param config Bytes encoder configuration.
 * @retval 0 If successful.
 * @retval -EINVAL Update RMT bytes encoder failed because of invalid argument.
 * @retval -ENODEV Update RMT bytes encoder failed because of other error.
 */
int rmt_bytes_encoder_update_config(rmt_encoder_handle_t bytes_encoder, const rmt_bytes_encoder_config_t *config);

/**
 * @brief Create RMT copy encoder, which copies the given RMT symbols into RMT memory.
 *
 * @param config Copy encoder configuration.
 * @param ret_encoder Returned encoder handle.
 * @retval 0 If successful.
 * @retval -EINVAL Create RMT copy encoder failed because of invalid argument.
 * @retval -ENOMEM Create RMT copy encoder failed because out of memory.
 * @retval -ENODEV Create RMT copy encoder failed because of other error.
 */
int rmt_new_copy_encoder(const rmt_copy_encoder_config_t *config, rmt_encoder_handle_t *ret_encoder);

/**
 * @brief Delete RMT encoder.
 *
 * @param encoder RMT encoder handle, created by e.g `rmt_new_bytes_encoder()`.
 * @retval 0 If successful.
 * @retval -EINVAL Delete RMT encoder failed because of invalid argument.
 * @retval -ENODEV Delete RMT encoder failed because of other error.
 */
int rmt_del_encoder(rmt_encoder_handle_t encoder);

/**
 * @brief Reset RMT encoder.
 *
 * @param encoder RMT encoder handle, created by e.g `rmt_new_bytes_encoder()`.
 * @retval 0 If successful.
 * @retval -EINVAL Reset RMT encoder failed because of invalid argument.
 * @retval -ENODEV Reset RMT encoder failed because of other error.
 */
int rmt_encoder_reset(rmt_encoder_handle_t encoder);

/**
 * @brief A helper function to allocate a proper memory for RMT encoder.
 *
 * @param size Size of memory to be allocated.
 * @retval Pointer to the allocated memory if the allocation is successful, NULL otherwise.
 */
void* rmt_alloc_encoder_mem(size_t size);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_DRIVERS_MISC_ESPRESSIF_RMT_RMT_ENCODER_H_ */
