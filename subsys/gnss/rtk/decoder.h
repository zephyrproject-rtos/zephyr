/*
 * Copyright (c) 2025 Croxel Inc.
 * Copyright (c) 2025 CogniPilot Foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SUBSYS_RTK_DECODER_H_
#define ZEPHYR_SUBSYS_RTK_DECODER_H_

#include <stdint.h>
#include <stdlib.h>

/**
 * @brief Decode RTK frame
 *
 * @note The expected use is to call this call iteratively until all the
 * frames present in the incoming buffer have been decoded.
 *
 * @param[in] buf Buffer holding encoded data.
 * @param[in] buf_len Buffer length.
 * @param[out] data Pointer to the decoded frame.
 * @param[out] data_len Length of the decoded frame
 *
 * @return Zero if successful.
 * @return -ENOENT if no frames have been decoded successfully.
 * @return -EAGAIN if there's an incomplete frame starting at the data pointer.
 * @return Other negative error code if decoding failed.
 */
int gnss_rtk_decoder_frame_get(const uint8_t *buf, size_t buf_len,
			       uint8_t **data, size_t *data_len);

#endif /* ZEPHYR_SUBSYS_RTK_DECODER_H_ */
