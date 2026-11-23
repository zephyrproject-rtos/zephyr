/*
 * Copyright (c) 2026 STMicroelectronics.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @ingroup video_interface
 * @brief STM32 DCMI specific controls.
 */

#ifndef ZEPHYR_INCLUDE_VIDEO_STM32_DCMI_H_
#define ZEPHYR_INCLUDE_VIDEO_STM32_DCMI_H_

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Switch the camera between video mode(0) and snapshot mode(1). */
#define VIDEO_CID_ST_SNAPSHOT_MODE (VIDEO_CID_PRIVATE_BASE + 1)

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_VIDEO_STM32_DCMI_H_ */
