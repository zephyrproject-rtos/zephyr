/**
 * Copyright (c) 2023 Arducam Technology Co., Ltd. <www.arducam.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_CAMERA_ARDUCAM_MEGA_H_
#define ZEPHYR_INCLUDE_DRIVERS_CAMERA_ARDUCAM_MEGA_H_

#ifdef __cplusplus
extern "C" {
#endif

/* Arducam specific camera controls */
#define VIDEO_CID_ARDUCAM_RESET    (VIDEO_CID_PRIVATE_BASE + 1)
#define VIDEO_CID_ARDUCAM_LOWPOWER (VIDEO_CID_PRIVATE_BASE + 2)

/* Read only registers */
#define VIDEO_CID_ARDUCAM_SUPP_RES    (VIDEO_CID_PRIVATE_BASE + 3)
#define VIDEO_CID_ARDUCAM_SUPP_SP_EFF (VIDEO_CID_PRIVATE_BASE + 4)

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_CAMERA_ARDUCAM_MEGA_H_ */
