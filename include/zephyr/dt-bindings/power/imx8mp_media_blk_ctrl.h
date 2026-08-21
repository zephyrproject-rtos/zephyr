/*
 * Copyright (C) 2026, Savoir-faire Linux
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_POWER_IMX8MP_MEDIA_BLK_CTRL_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_POWER_IMX8MP_MEDIA_BLK_CTRL_H_

/**
 * @file
 * @brief NXP i.MX8MP MEDIA_BLK_CTRL power domain definitions.
 */

/** @cond INTERNAL_HIDDEN */

/* MEDIA_BLK_CTRL power domain IDs */
#define IMX8MP_MEDIABLK_PD_MIPI_DSI_1  0
#define IMX8MP_MEDIABLK_PD_MIPI_CSI2_1 1
#define IMX8MP_MEDIABLK_PD_LCDIF_1     2
#define IMX8MP_MEDIABLK_PD_ISI         3
#define IMX8MP_MEDIABLK_PD_MIPI_CSI2_2 4
#define IMX8MP_MEDIABLK_PD_LCDIF_2     5
#define IMX8MP_MEDIABLK_PD_ISP         6
#define IMX8MP_MEDIABLK_PD_DWE         7
#define IMX8MP_MEDIABLK_PD_MIPI_DSI_2  8

/** @endcond */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_POWER_IMX8MP_MEDIA_BLK_CTRL_H_ */
