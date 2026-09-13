/*
 * Copyright (c) 2026 Carl Zeiss Meditec AG
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Clock control definitions for TI cdce9xx devices.
 * @ingroup clock_control_cdce9xx
 */

#ifndef ZEPHYR_DRIVERS_CLOCK_CONTROL_TI_CDCE9XX_H_
#define ZEPHYR_DRIVERS_CLOCK_CONTROL_TI_CDCE9XX_H_

#include <zephyr/drivers/clock_control.h>

/**
 * @defgroup clock_control_cdce9xx TI
 * @ingroup clock_control_interface_ext
 * @{
 */

#ifdef __cplusplus
extern "C" {
#endif

/* Keep the order, the driver relies on it. */
#define CLOCK_CONTROL_TI_CDCE9XX_ALL (CLOCK_CONTROL_SUBSYS_ALL)  /**< Refers to ALL outputs. */
#define CLOCK_CONTROL_TI_CDCE9XX_Y1  ((clock_control_subsys_t)1) /**< Selects Y1 output. */
#define CLOCK_CONTROL_TI_CDCE9XX_Y2  ((clock_control_subsys_t)2) /**< Selects Y2 output. */
#define CLOCK_CONTROL_TI_CDCE9XX_Y3  ((clock_control_subsys_t)3) /**< Selects Y3 output. */
#define CLOCK_CONTROL_TI_CDCE9XX_Y4  ((clock_control_subsys_t)4) /**< Selects Y4 output. */
#define CLOCK_CONTROL_TI_CDCE9XX_Y5  ((clock_control_subsys_t)5) /**< Selects Y5 output. */
#define CLOCK_CONTROL_TI_CDCE9XX_Y6  ((clock_control_subsys_t)6) /**< Selects Y6 output. */
#define CLOCK_CONTROL_TI_CDCE9XX_Y7  ((clock_control_subsys_t)7) /**< Selects Y7 output. */
#define CLOCK_CONTROL_TI_CDCE9XX_Y8  ((clock_control_subsys_t)8) /**< Selects Y8 output. */
#define CLOCK_CONTROL_TI_CDCE9XX_Y9  ((clock_control_subsys_t)9) /**< Selects Y9 output. */

#ifdef __cplusplus
}
#endif

/** @} */

#endif /* ZEPHYR_DRIVERS_CLOCK_CONTROL_TI_CDCE9XX_H_ */
