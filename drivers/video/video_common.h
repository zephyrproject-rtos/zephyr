/*
 * Copyright (c) 2024, tinyVision.ai Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_VIDEO_COMMON_H
#define ZEPHYR_INCLUDE_VIDEO_COMMON_H

/**
 * @brief Provide the minimum/maximum/default 32-bit value depending on the CID.
 *
 * Video CIDs can contain sub-operations. This function facilitates
 * implementation of video controls in the drivers by handling these
 * the range-related CIDs.
 *
 * @param cid The Video Control ID which contains the operation.
 * @param min The minimum value returned if the CID requested it.
 * @param max The maximum value returned if the CID requested it.
 * @param def The default value returned if the CID requested it.
 *
 * @return 0 if the operation was regarding a range CID and the value could
 *         be set. In which case, there is nothing else to do than returning 0.
 * @return 1 if the operation is not for the range, but the current value,
 *         in which case it is the duty of the driver to query the current
 *         value to the hardware
 * @return A negative error code if an error occurred.
 */
int video_get_range_u32(unsigned int cid, uint32_t *u32, uint32_t min, uint32_t max, uint32_t def);

/**
 * @brief Check if the 32-bit value is within range for this CID.
 *
 * Before setting a video control, a driver might be interested in checking
 * if it is within a valid range. This function facilitates it by reusing the
 * video_get_ctrl() API using @c VIDEO_CTRL_GET_MIN and @c VIDEO_CTRL_GET_MAX
 * to validate the input.
 *
 * @param dev The video device to query to learn about the min and max.
 * @param cid The CID for which to check the range.
 * @param u32 The 32-bit value that must be matched against the range.
 */
int video_check_range_u32(const struct device *dev, unsigned int cid, uint32_t u32);

#endif /* ZEPHYR_INCLUDE_VIDEO_COMMON_H */
