/*
 * Copyright (c) 2026 Realtek Semiconductor, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @ingroup rts5817_wuc
 * @brief RTS5817 Wakeup Controller devicetree binding definitions.
 *
 * Defines the wakeup source identifiers, wakeup modes, and other constants
 * used in the "wakeup-ctrls" devicetree property cell for the Realtek
 * RTS5817 WUC driver (compatible "realtek,rts5817-wuc").
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_WUC_RTS5817_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_WUC_RTS5817_H_

/**
 * @name RTS5817 wakeup source identifiers
 * @{
 */

/** No wakeup source (invalid ID). */
#define RTS5817_WKUP_SRC_NONE        0
/** GPIO AL2 wakeup source. */
#define RTS5817_WKUP_SRC_GPIO_AL2    1
/** GPIO AL1 wakeup source. */
#define RTS5817_WKUP_SRC_GPIO_AL1    2
/** GPIO AL0 wakeup source. */
#define RTS5817_WKUP_SRC_GPIO_AL0    3
/** Sensor GPIO wakeup source. */
#define RTS5817_WKUP_SRC_SENSOR_GPIO 4
/** Sensor CS wakeup source. */
#define RTS5817_WKUP_SRC_SENSOR_CS   5
/** GPI WAKE2 wakeup source. */
#define RTS5817_WKUP_SRC_GPI_WAKE2   6
/** GPI WAKE1 wakeup source. */
#define RTS5817_WKUP_SRC_GPI_WAKE1   7
/** RC timer wakeup source. */
#define RTS5817_WKUP_SRC_RC_TIMER    8
/** USB host wakeup source. */
#define RTS5817_WKUP_SRC_USB_HOST    9
/** Over-current protection wakeup source. */
#define RTS5817_WKUP_SRC_OCP         10
/** @} */

/** Maximum number of wakeup sources. */
#define RTS5817_WKUP_SRC_MAX_NUM 11

/**
 * @name Wakeup mode values (for GPIO-type wakeup sources)
 * @{
 */

/** Wake on rising edge. */
#define RTS5817_WAKEUP_MODE_RISING_EDGE  0
/** Wake on falling edge. */
#define RTS5817_WAKEUP_MODE_FALLING_EDGE 1
/** Wake on high level. */
#define RTS5817_WAKEUP_MODE_HIGH_LEVEL   2
/** Wake on low level. */
#define RTS5817_WAKEUP_MODE_LOW_LEVEL    3
/** @} */

/** Maximum RC timer wakeup time in milliseconds. */
#define RTS5817_WKUP_RC_TIMER_MAX_MS 41943

/**
 * @name S2RAM substate identifiers
 * @{
 */

/** Suspend substate. */
#define RTS5817_S2RAM_SUB_ID_SUSPEND 1
/** Sleep substate. */
#define RTS5817_S2RAM_SUB_ID_SLEEP   2
/** @} */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_WUC_RTS5817_H_ */
