/*
 * Copyright (c) 2025 Croxel Inc.
 * Copyright (c) 2025 CogniPilot Foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_GNSS_RTK_DECODER_H_
#define ZEPHYR_INCLUDE_GNSS_RTK_DECODER_H_

#include <stdint.h>
#include <stddef.h>

/**
 * @brief Get an RTK frame from buffer
 *
 * Used by RTK clients to extract frames from a data-buffer.
 *
 * @param[in] buf Buffer holding encoded data.
 * @param[in] buf_len Buffer length.
 * @param[out] data Pointer to the decoded frame.
 * @param[out] data_len Length of the decoded frame
 *
 * @return Zero if successful.
 * @return -ENOENT if no frames have been decoded successfully.
 * @return Other negative error code if decoding failed.
 */
int gnss_rtk_decoder_frame_get(uint8_t *buf, size_t buf_len,
			       uint8_t **data, size_t *data_len);

#endif /* ZEPHYR_INCLUDE_GNSS_RTK_DECODER_H_ */
