/*
 * Copyright (C) 2024-2025, Xiaohua Semiconductor Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_PINCTRL_HC32_PINCTRL_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_PINCTRL_HC32_PINCTRL_H_

#define HC32_PORT(_mux) \
	(((_mux) >> HC32_PORT_SHIFT) & HC32_PORT_MSK)

#define HC32_PIN(_mux) \
	(((_mux) >> HC32_PIN_SHIFT) & HC32_PIN_MSK)

#define HC32_MODE(_mux) \
	(((_mux) >> HC32_MODE_SHIFT) & HC32_MODE_MSK)

#define HC32_FUNC_NUM(_mux) \
	(((_mux) >> HC32_FUNC_SHIFT) & HC32_FUNC_MSK)

#define HC32_PIN_BIAS(_mux)               (_mux & HC32_PUR_MSK)
#define HC32_PIN_DRV(_mux)                (_mux & HC32_NOD_MSK)
#define HC32_OUT_LEVEL(_mux)              (_mux & HC32_OTYPE_MSK)
#define HC32_PIN_EN_DIR(_mux)             (_mux & HC32_DIREN_MSK)
#define HC32_PIN_DRIVER_STRENGTH(_mux)    (_mux & HC32_STRENGTH_MSK)
#define HC32_INVERT(_mux)                 (_mux & HC32_INVERT_MSK)
#define HC32_CINSEL(_mux)                 (_mux & HC32_CINSEL_MSK)

/* pinmux filed in pinctrl_soc_pin*/
/**
 * @brief Pin modes
 */
#define HC32_ANALOG     0x00U
#define HC32_GPIO       0x01U
#define HC32_FUNC       0x02U
#define HC32_SUBFUNC    0x03U

/**
 * @brief Macro to generate pinmux int using port, pin number and mode arguments
 * This is inspired from Linux equivalent st,HC32f429-pinctrl binding
 */
#define HC32_FUNC_SHIFT  10U
#define HC32_FUNC_MSK    0xFFU
#define HC32_MODE_SHIFT  8U
#define HC32_MODE_MSK    0x03U
#define HC32_PIN_SHIFT   4U
#define HC32_PIN_MSK     0x0FU
#define HC32_PORT_SHIFT  0U
#define HC32_PORT_MSK    0x0FU

/**
 * @brief pinmux bit field.
 *
 * Fields:
 * - cfg [24:31]
 * - func_num [ 10 ：17]
 * - mode [ 8 : 9 ]
 * - pin  [ 4 : 7 ]
 * - port [ 0 : 3 ]
 *
 * @param port Port ('A'..'E', 'H')
 * @param pin Pin (0..15)
 * @param mode Mode (ANALOG, FUC, SUBFUC).
 * @param func_num range:0--63.
 * @param cfg configuration for pin
 */
#define HC32_PINMUX(port, pin, mode, func_num, cfg)					       \
		(((((port) - 'A') & HC32_PORT_MSK) << HC32_PORT_SHIFT) |    \
		(((pin) & HC32_PIN_MSK) << HC32_PIN_SHIFT) |	       \
		(((HC32_ ## mode) & HC32_MODE_MSK) << HC32_MODE_SHIFT) |   \
		(((func_num) & HC32_FUNC_MSK) << HC32_FUNC_SHIFT) | (cfg))


/* cfg filed in pinctrl_soc_pin*/
/**
 * @brief Definitions cfg bit pos and mask
 */
#define HC32_NO_PULL                (0U << HC32_PUR_SHIFT)
#define HC32_PULL_UP                (1U << HC32_PUR_SHIFT)

#define HC32_PUSH_PULL              (0U << HC32_NOD_SHIFT)
#define HC32_OPEN_DRAIN             (1U << HC32_NOD_SHIFT)

#define HC32_OUTPUT_LOW             (0U << HC32_OTYPE_SHIFT)
#define HC32_OUTPUT_HIGH            (1U << HC32_OTYPE_SHIFT)

#define HC32_INPUT_ENABLE           (0U << HC32_DIREN_SHIFT)
#define HC32_OUTPUT_ENABLE          (1U << HC32_DIREN_SHIFT)

#define HC32_DRIVER_STRENGTH_LOW    (0U << HC32_STRENGTH_SHIFT)
#define HC32_DRIVER_STRENGTH_MEDIUM (1U << HC32_STRENGTH_SHIFT)
#define HC32_DRIVER_STRENGTH_HIGH   (2U << HC32_STRENGTH_SHIFT)

#define HC32_INVERT_DISABLE         (0U << HC32_INVERT_SHIFT)
#define HC32_INVERT_ENABLE          (1U << HC32_INVERT_SHIFT)

#define HC32_CINSEL_SCHMITT         (0U << HC32_CINSEL_SHIFT)
#define HC32_CINSEL_CMOS            (1U << HC32_CINSEL_SHIFT)

/**
 * @brief Pin cfg bit field in pinmux.
 *
 * Fields:
 * - CINSEL [31]
 * - INVERT [30]
 * - STRENGTH [28:29]
 * - DIREN [ 27]
 * - OTYPE [ 26 ]
 * - NOD  [ 25 ]
 * - PUR [ 24 ]
*/

/** PUR field position. */
#define HC32_PUR_SHIFT       24U
/** PUR field mask. */
#define HC32_PUR_MSK         (1U << HC32_PUR_SHIFT)

/** NOD field position. */
#define HC32_NOD_SHIFT       25U
/** NOD field mask. */
#define HC32_NOD_MSK         (1U << HC32_NOD_SHIFT)

/** Out default level field position. */
#define HC32_OTYPE_SHIFT     26U
/** Out default level field mask. */
#define HC32_OTYPE_MSK       (1U << HC32_OTYPE_SHIFT)

/** DIREN field position. */
#define HC32_DIREN_SHIFT     27U
/** DIREN(I/O select) field mask. */
#define HC32_DIREN_MSK       (1U << HC32_DIREN_SHIFT)

/** STRENGTH field position. */
#define HC32_STRENGTH_SHIFT  28U
/** STRENGTH field mask. */
#define HC32_STRENGTH_MSK    (3U << HC32_STRENGTH_SHIFT)

/** I\O invert data  */
#define HC32_INVERT_SHIFT    30U
#define HC32_INVERT_MSK      (1U << HC32_INVERT_SHIFT)

/** schmitt or CMOS select */
#define HC32_CINSEL_SHIFT    31U
#define HC32_CINSEL_MSK      (1U << HC32_CINSEL_SHIFT)


#endif	/* ZEPHYR_INCLUDE_DT_BINDINGS_PINCTRL_HC32_PINCTRL_H_ */
