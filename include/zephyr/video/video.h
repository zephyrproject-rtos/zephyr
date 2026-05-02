/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Public APIs for video
 *
 * This file contains public APIs for video area
 */

#ifndef ZEPHYR_INCLUDE_VIDEO_VIDEO_H_
#define ZEPHYR_INCLUDE_VIDEO_VIDEO_H_

/**
 * @brief Video public APIs.
 * @defgroup video_api Video APIs
 * @since 4.4
 * @version 0.1.0
 * @ingroup video_interface
 * @{
 */

/*
 * TODO: Temporarily include this driver APIs header so that application just needs to
 * import this file instead of both. It should be removed once video APIs are correctly
 * re-organized into public and drivers APIs.
 */
#include <zephyr/drivers/video.h>

/**
 * @brief Import an external memory to the video buffer pool
 *
 * Import an externally allocated memory as a @ref video_buffer in the video buffer pool
 *
 * @param mem Pointer to the external memory
 * @param sz Size of the external memory
 * @param idx Returned index of the imported video buffer in the video buffer pool
 *
 * @retval 0 on success or a negative errno code on failure.
 */
int video_import_buffer(uint8_t *mem, size_t sz, uint16_t *idx);

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_VIDEO_VIDEO_H_ */
