/*
 * Copyright (c) 2020 Linaro Ltd.
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * STM32 SoC specific helpers for pinctrl driver
 */

#ifndef ZEPHYR_SOC_ARM_ST_STM32_COMMON_PINCTRL_SOC_H_
#define ZEPHYR_SOC_ARM_ST_STM32_COMMON_PINCTRL_SOC_H_

#include <zephyr/devicetree.h>
#include <zephyr/types.h>

#ifdef CONFIG_SOC_SERIES_STM32F1X
#include <zephyr/dt-bindings/pinctrl/stm32f1-pinctrl.h>
#else
#include <zephyr/dt-bindings/pinctrl/stm32-pinctrl.h>
#endif

#if DT_HAS_COMPAT_STATUS_OKAY(st_stm32n6_pinctrl)
/* Required for GPIO LL definitions we use */
#include <stm32_ll_gpio.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

/** @cond INTERNAL_HIDDEN */

/** Type for STM32 pin. */
typedef struct pinctrl_soc_pin {
	/** Pinmux settings (port, pin and function). */
	uint32_t pinmux;
	/** Pin configuration (bias, drive, slew rate, I/O retime) */
	uint32_t pincfg;
} pinctrl_soc_pin_t;

/**
 * @brief Utility macro to initialize pinmux field in #pinctrl_pin_t.
 *
 * @param node_id Node identifier.
 */
#define Z_PINCTRL_STM32_PINMUX_INIT(node_id) DT_PROP(node_id, pinmux)

/**
 * @brief Definitions used to initialize fields in #pinctrl_pin_t
 */
#define STM32_NO_PULL     0x0
#define STM32_PULL_UP     0x1
#define STM32_PULL_DOWN   0x2
#define STM32_PUSH_PULL   0x0
#define STM32_OPEN_DRAIN  0x1
#define STM32_OUTPUT_LOW  0x0
#define STM32_OUTPUT_HIGH 0x1
#define STM32_GPIO_OUTPUT 0x1

#ifdef CONFIG_SOC_SERIES_STM32F1X
/**
 * @brief PIN configuration bitfield
 *
 * Pin configuration is coded with the following
 * fields
 *    GPIO I/O Mode       [ 0 ]
 *    GPIO Input config   [ 1 : 2 ]
 *    GPIO Output speed   [ 3 : 4 ]
 *    GPIO Output PP/OD   [ 5 ]
 *    GPIO Output AF/GP   [ 6 ]
 *    GPIO PUPD Config    [ 7 : 8 ]
 *    GPIO ODR            [ 9 ]
 *
 * Applicable to STM32F1 series
 */

/* Port Mode */
#define STM32_MODE_INPUT		(0x0 << STM32_MODE_INOUT_SHIFT)
#define STM32_MODE_OUTPUT		(0x1 << STM32_MODE_INOUT_SHIFT)
#define STM32_MODE_INOUT_MASK		0x1
#define STM32_MODE_INOUT_SHIFT		0

/* Input Port configuration */
#define STM32_CNF_IN_ANALOG		(0x0 << STM32_CNF_IN_SHIFT)
#define STM32_CNF_IN_FLOAT		(0x1 << STM32_CNF_IN_SHIFT)
#define STM32_CNF_IN_PUPD		(0x2 << STM32_CNF_IN_SHIFT)
#define STM32_CNF_IN_MASK		0x3
#define STM32_CNF_IN_SHIFT		1

/* Output Port configuration */
#define STM32_MODE_OUTPUT_MAX_10	(0x0 << STM32_MODE_OSPEED_SHIFT)
#define STM32_MODE_OUTPUT_MAX_2		(0x1 << STM32_MODE_OSPEED_SHIFT)
#define STM32_MODE_OUTPUT_MAX_50	(0x2 << STM32_MODE_OSPEED_SHIFT)
#define STM32_MODE_OSPEED_MASK		0x3
#define STM32_MODE_OSPEED_SHIFT		3

#define STM32_CNF_PUSH_PULL		(0x0 << STM32_CNF_OUT_0_SHIFT)
#define STM32_CNF_OPEN_DRAIN		(0x1 << STM32_CNF_OUT_0_SHIFT)
#define STM32_CNF_OUT_0_MASK		0x1
#define STM32_CNF_OUT_0_SHIFT		5

#define STM32_CNF_GP_OUTPUT		(0x0 << STM32_CNF_OUT_1_SHIFT)
#define STM32_CNF_ALT_FUNC		(0x1 << STM32_CNF_OUT_1_SHIFT)
#define STM32_CNF_OUT_1_MASK		0x1
#define STM32_CNF_OUT_1_SHIFT		6

/* GPIO High impedance/Pull-up/Pull-down */
#define STM32_PUPD_NO_PULL		(0x0 << STM32_PUPD_SHIFT)
#define STM32_PUPD_PULL_UP		(0x1 << STM32_PUPD_SHIFT)
#define STM32_PUPD_PULL_DOWN		(0x2 << STM32_PUPD_SHIFT)
#define STM32_PUPD_MASK			0x3
#define STM32_PUPD_SHIFT		7

/* GPIO plain output value */
#define STM32_ODR_0			(0x0 << STM32_ODR_SHIFT)
#define STM32_ODR_1			(0x1 << STM32_ODR_SHIFT)
#define STM32_ODR_MASK			0x1
#define STM32_ODR_SHIFT			9

/**
 * @brief Utility macro to initialize pincfg field in #pinctrl_pin_t (F1).
 *
 * @param node_id Node identifier.
 */
#define Z_PINCTRL_STM32_PINCFG_INIT(node_id)				       \
	(((STM32_NO_PULL * DT_PROP(node_id, bias_disable)) << STM32_PUPD_SHIFT) | \
	 ((STM32_PULL_UP * DT_PROP(node_id, bias_pull_up)) << STM32_PUPD_SHIFT) | \
	 ((STM32_PULL_DOWN * DT_PROP(node_id, bias_pull_down)) << STM32_PUPD_SHIFT) | \
	 ((STM32_PUSH_PULL * DT_PROP(node_id, drive_push_pull)) << STM32_CNF_OUT_0_SHIFT) | \
	 ((STM32_OPEN_DRAIN * DT_PROP(node_id, drive_open_drain)) << STM32_CNF_OUT_0_SHIFT) | \
	 ((STM32_OUTPUT_LOW * DT_PROP(node_id, output_low)) << STM32_ODR_SHIFT) | \
	 ((STM32_OUTPUT_HIGH * DT_PROP(node_id, output_high)) << STM32_ODR_SHIFT) | \
	 (DT_ENUM_IDX(node_id, slew_rate) << STM32_MODE_OSPEED_SHIFT))
#else

/**
 * @brief PIN configuration bitfield
 *
 * Pin configuration is coded with the following
 * fields
 *	[03:00] Alternate Functions
 *	[05:04] GPIO Mode
 *	[   06] GPIO Output type
 *	[08:07] GPIO Speed
 *	[10:09] GPIO PUPD config
 *	[   11] GPIO Output data
 *
 * These fields are only used when pinctrl with compatible
 * "st,stm32n6-pinctrl" is in use:
 *	[15:12] I/O delay length
 *	[   16] I/O delay direction
 *	[18:17] I/O retime edge
 *	[   19] I/O retime enable
 *
 * These fields are only used when pinctrl with compatible
 * "st,stm32h5-pinctrl" is in use:
 *	[   12] High-speed low-voltage mode
 */

/* GPIO Mode */
#define STM32_MODER_INPUT_MODE		(0x0 << STM32_MODER_SHIFT)
#define STM32_MODER_OUTPUT_MODE		(0x1 << STM32_MODER_SHIFT)
#define STM32_MODER_ALT_MODE		(0x2 << STM32_MODER_SHIFT)
#define STM32_MODER_ANALOG_MODE		(0x3 << STM32_MODER_SHIFT)
#define STM32_MODER_MASK		0x3
#define STM32_MODER_SHIFT		4

/* GPIO Output type */
#define STM32_OTYPER_PUSH_PULL		(0x0 << STM32_OTYPER_SHIFT)
#define STM32_OTYPER_OPEN_DRAIN		(0x1 << STM32_OTYPER_SHIFT)
#define STM32_OTYPER_MASK		0x1
#define STM32_OTYPER_SHIFT		6

/* GPIO speed */
#define STM32_OSPEEDR_LOW_SPEED		(0x0 << STM32_OSPEEDR_SHIFT)
#define STM32_OSPEEDR_MEDIUM_SPEED	(0x1 << STM32_OSPEEDR_SHIFT)
#define STM32_OSPEEDR_HIGH_SPEED	(0x2 << STM32_OSPEEDR_SHIFT)
#define STM32_OSPEEDR_VERY_HIGH_SPEED	(0x3 << STM32_OSPEEDR_SHIFT)
#define STM32_OSPEEDR_MASK		0x3
#define STM32_OSPEEDR_SHIFT		7

/* GPIO High impedance/Pull-up/pull-down */
#define STM32_PUPDR_NO_PULL		(0x0 << STM32_PUPDR_SHIFT)
#define STM32_PUPDR_PULL_UP		(0x1 << STM32_PUPDR_SHIFT)
#define STM32_PUPDR_PULL_DOWN		(0x2 << STM32_PUPDR_SHIFT)
#define STM32_PUPDR_MASK		0x3
#define STM32_PUPDR_SHIFT		9

/* GPIO plain output value */
#define STM32_ODR_0			(0x0 << STM32_ODR_SHIFT)
#define STM32_ODR_1			(0x1 << STM32_ODR_SHIFT)
#define STM32_ODR_MASK			0x1
#define STM32_ODR_SHIFT			11

/* I/O delay length (DELAYR) */
#define STM32_IODELAY_LENGTH_MASK	0xFU
#define STM32_IODELAY_LENGTH_SHIFT	12

/* I/O delay & retime configuration (ADVCFGR) */
#define STM32_IORETIME_ADVCFGR_MASK	0xFU
#define STM32_IORETIME_ADVCFGR_SHIFT	16

#define STM32_IODELAY_DIRECTION_SHIFT	STM32_IORETIME_ADVCFGR_SHIFT
#define STM32_IORETIME_EDGE_SHIFT	17
#define STM32_IORETIME_ENABLE_SHIFT	19

/* High-speed low-voltage mode (HSLVR) */
#define STM32_HSLVR_DISABLE		(0x0 << STM32_HSLVR_SHIFT)
#define STM32_HSLVR_ENABLED		(0x1 << STM32_HSLVR_SHIFT)
#define STM32_HSLVR_MASK		0x1
#define STM32_HSLVR_SHIFT		12

/**
 * @brief Definitions for the various fields related to I/O synchronization
 *
 * NOTES:
 *  (1) Series-specific CMSIS and GPIO LL definitions are used here.
 *      This is OK as long as the macros are never used on series where
 *      the underlying definitions do not exist.
 *  (2) I/O delay values definitions from GPIO LL definitions are used
 *      directly to ensure greater portability to other platforms.
 */
#define STM32_IOSYNC_DELAY_DIRECTION_OUTPUT	0x0
#define STM32_IOSYNC_DELAY_DIRECTION_INPUT	GPIO_ADVCFGRL_DLYPATH0
#define STM32_IOSYNC_RETIME_EDGE_RISING		0x0
#define STM32_IOSYNC_RETIME_EDGE_FALLING	GPIO_ADVCFGRL_INVCLK0
#define STM32_IOSYNC_RETIME_EDGE_BOTH		GPIO_ADVCFGRL_DE0
#define STM32_IOSYNC_RETIME_ENABLE		GPIO_ADVCFGRL_RET0

#if DT_HAS_COMPAT_STATUS_OKAY(st_stm32n6_pinctrl)
/* Inner helper macro for Z_PINCTRL_STM32_IOSYNC_INIT */
#define Z_PINCTRL_STM32_IOSYNC_INIT_INNER(delay_path, retime_edge, retime_enable, delay_ps)	\
	((CONCAT(STM32_IOSYNC_DELAY_DIRECTION_, delay_path) << STM32_IODELAY_DIRECTION_SHIFT) | \
	 ((retime_enable) << STM32_IORETIME_ENABLE_SHIFT) |					\
	 (CONCAT(STM32_IOSYNC_RETIME_EDGE_, retime_edge) << STM32_IORETIME_EDGE_SHIFT) |	\
	 (CONCAT(LL_GPIO_DELAY_, delay_ps) << STM32_IODELAY_LENGTH_SHIFT))

/**
 * @brief Utility macro to initialize fields of @ref{pinctrl_pin_t}.pincfg
 *        related to the I/O synchronization feature
 *
 * @param node_id Pinctrl node identifier
 *
 * NOTE: a default value for st,retime-edge is specified to ensure the macro expands properly.
 * However, this default value is never used, as I/O retiming is not enabled unless the property
 * was explicitly specified in Device Tree.
 */
#define Z_PINCTRL_STM32_IOSYNC_INIT(node_id)							\
	Z_PINCTRL_STM32_IOSYNC_INIT_INNER(							\
		DT_STRING_UPPER_TOKEN(node_id, st_io_delay_path),				\
		DT_STRING_UPPER_TOKEN_OR(node_id, st_retime_edge, RISING),			\
		STM32_IOSYNC_RETIME_ENABLE * DT_NODE_HAS_PROP(node_id, st_retime_edge),		\
		DT_PROP(node_id, st_io_delay_ps))
#else /* DT_HAS_COMPAT_STATUS_OKAY(st_stm32n6_pinctrl) */
/** Dummy value for series without I/O synchronization feature */
#define Z_PINCTRL_STM32_IOSYNC_INIT(node_id)	0
#endif /* DT_HAS_COMPAT_STATUS_OKAY(st_stm32n6_pinctrl) */

#if DT_HAS_COMPAT_STATUS_OKAY(st_stm32h5_pinctrl)
#define Z_PINCTRL_STM32_HSLV_INIT(node_id)                                                         \
	(STM32_HSLVR_ENABLED * DT_PROP(node_id, st_hslv_enable))
#else /* DT_HAS_COMPAT_STATUS_OKAY(st_stm32h5_pinctrl) */
/** Dummy value for series without high-speed low-voltage mode */
#define Z_PINCTRL_STM32_HSLV_INIT(node_id)	0
#endif /* DT_HAS_COMPAT_STATUS_OKAY(st_stm32h5_pinctrl) */

/**
 * @brief Utility macro to initialize pincfg field in #pinctrl_pin_t (non-F1).
 *
 * @param node_id Node identifier.
 */
#define Z_PINCTRL_STM32_PINCFG_INIT(node_id)				       \
	(((STM32_NO_PULL * DT_PROP(node_id, bias_disable)) << STM32_PUPDR_SHIFT) | \
	 ((STM32_PULL_UP * DT_PROP(node_id, bias_pull_up)) << STM32_PUPDR_SHIFT) | \
	 ((STM32_PULL_DOWN * DT_PROP(node_id, bias_pull_down)) << STM32_PUPDR_SHIFT) | \
	 ((STM32_PUSH_PULL * DT_PROP(node_id, drive_push_pull)) << STM32_OTYPER_SHIFT) | \
	 ((STM32_OPEN_DRAIN * DT_PROP(node_id, drive_open_drain)) << STM32_OTYPER_SHIFT) | \
	 ((STM32_OUTPUT_LOW * DT_PROP(node_id, output_low)) << STM32_ODR_SHIFT) | \
	 ((STM32_OUTPUT_HIGH * DT_PROP(node_id, output_high)) << STM32_ODR_SHIFT) | \
	 ((STM32_GPIO_OUTPUT * DT_PROP(node_id, output_low)) << STM32_MODER_SHIFT) | \
	 ((STM32_GPIO_OUTPUT * DT_PROP(node_id, output_high)) << STM32_MODER_SHIFT) | \
	 (DT_ENUM_IDX(node_id, slew_rate) << STM32_OSPEEDR_SHIFT) | \
	 (Z_PINCTRL_STM32_IOSYNC_INIT(node_id)) | \
	 (Z_PINCTRL_STM32_HSLV_INIT(node_id)))
#endif /* CONFIG_SOC_SERIES_STM32F1X */

/**
 * @brief Utility macro to initialize each pin.
 *
 * @param node_id Node identifier.
 * @param state_prop State property name.
 * @param idx State property entry index.
 */
#define Z_PINCTRL_STATE_PIN_INIT(node_id, state_prop, idx)		       \
	{ .pinmux = Z_PINCTRL_STM32_PINMUX_INIT(			       \
		DT_PROP_BY_IDX(node_id, state_prop, idx)),		       \
	  .pincfg = Z_PINCTRL_STM32_PINCFG_INIT(			       \
		DT_PROP_BY_IDX(node_id, state_prop, idx)) },

/**
 * @brief Utility macro to initialize state pins contained in a given property.
 *
 * @param node_id Node identifier.
 * @param prop Property name describing state pins.
 */
#define Z_PINCTRL_STATE_PINS_INIT(node_id, prop)			       \
	{DT_FOREACH_PROP_ELEM(node_id, prop, Z_PINCTRL_STATE_PIN_INIT)}

/** @endcond */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_SOC_ARM_ST_STM32_COMMON_PINCTRL_SOC_H_ */
