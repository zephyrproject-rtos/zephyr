/**
 * Copyright (c) 2023 Arducam Technology Co., Ltd. <www.arducam.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @ingroup video_interface
 * @brief Arducam Mega vendor-specific controls.
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_CAMERA_ARDUCAM_MEGA_H_
#define ZEPHYR_INCLUDE_DRIVERS_CAMERA_ARDUCAM_MEGA_H_

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Trigger a reset fo the camera */
#define VIDEO_CID_ARDUCAM_RESET    (VIDEO_CID_PRIVATE_BASE + 1)

/** @brief Trigger the entry (1) or exit (0) of low-power mode */
#define VIDEO_CID_ARDUCAM_LOWPOWER (VIDEO_CID_PRIVATE_BASE + 2)

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_CAMERA_ARDUCAM_MEGA_H_ */
