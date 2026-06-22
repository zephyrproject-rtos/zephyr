/*
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_VIDEO_INTERFACES_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_VIDEO_INTERFACES_H_

/**
 * @file
 * @brief Video interface bus type definitions
 */

/**
 * @defgroup video_interface_bus_types Video interface bus types
 * @ingroup video_interface
 * @brief Bus type values for the devicetree @c bus-type property
 *
 * These values identify the type of data bus connecting two video endpoints. They are meant to be
 * used as the @c bus-type property of a video endpoint node in devicetree.
 * @{
 */

/** MIPI CSI-2 C-PHY */
#define VIDEO_BUS_TYPE_CSI2_CPHY 1
/** MIPI CSI-1 */
#define VIDEO_BUS_TYPE_CSI1      2
/** SMIA CCP2 (Compact Camera Port 2) */
#define VIDEO_BUS_TYPE_CCP2      3
/** MIPI CSI-2 D-PHY */
#define VIDEO_BUS_TYPE_CSI2_DPHY 4
/** Parallel */
#define VIDEO_BUS_TYPE_PARALLEL  5
/** BT.656 */
#define VIDEO_BUS_TYPE_BT656     6

/** @} */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_VIDEO_INTERFACES_H_ */
