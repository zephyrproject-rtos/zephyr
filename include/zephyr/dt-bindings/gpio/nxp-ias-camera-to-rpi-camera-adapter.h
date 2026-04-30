/*
 * Copyright 2026 NXP
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_GPIO_NXP_IAS_CAMERA_TO_RPI_CAMERA_ADAPTER_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_GPIO_NXP_IAS_CAMERA_TO_RPI_CAMERA_ADAPTER_H_

/**
 * @file
 * @brief GPIO nexus pin definitions for the NXP IAS Camera to RPi Camera Adapter.
 */

/** AP1302 RESET control pin exposed via nxp_cam_connector gpio-nexus */
#define NXP_IAS_CAM_RESET 0

/** AP1302 ISP enable control pin exposed via nxp_cam_connector gpio-nexus */
#define NXP_IAS_CAM_ISP_EN 1

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_GPIO_NXP_IAS_CAMERA_TO_RPI_CAMERA_ADAPTER_H_ */
