/*
 * Copyright (c) 2024 Nordic Semiconductor ASA.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DEBUG_CORESIGHT_CS_TRACE_DEFMT_H__
#define ZEPHYR_INCLUDE_DEBUG_CORESIGHT_CS_TRACE_DEFMT_H__

#include <zephyr/kernel.h>
#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup coresight_apis Coresight APIs
 * @ingroup debug
 * @{
 * @}
 * @defgroup cs_trace_defmt Coresight Trace Deformatter
 * @ingroup coresight_apis
 * @{
 */

/** @brief Callback signature.
 *
 * @param id	Stream ID.
 * @param data	Data.
 * @param len	Data length.
 */
typedef void (*cs_trace_defmt_cb)(uint32_t id, const uint8_t *data, size_t len);

/** @brief Size of trace deformatter frame size in 32 bit words. */
#define CORESIGHT_TRACE_FRAME_SIZE32 4

/** @brief Size of trace deformatter frame size in bytes. */
#define CORESIGHT_TRACE_FRAME_SIZE (CORESIGHT_TRACE_FRAME_SIZE32 * sizeof(uint32_t))

/** @brief Initialize Coresight Trace Deformatter.
 *
 * @param cb Callback.
 */
int cs_trace_defmt_init(cs_trace_defmt_cb cb);

/** @brief Decode data from the stream.
 *
 * Trace formatter puts data in the 16 byte long blocks.
 *
 * Callback is called with decoded data.
 *
 * @param data	Data.
 * @param len	Data length. Must equal 16.
 *
 * @retval 0		On successful deformatting.
 * @retval -EINVAL	If wrong length is provided.
 */
int cs_trace_defmt_process(const uint8_t *data, size_t len);

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DEBUG_CORESIGHT_CS_TRACE_DEFMT_H__ */
