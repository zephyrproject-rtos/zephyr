/*
 * Copyright (c) 2020 Linaro Ltd.
 * Copyright (c) 2021 Nordic Semiconductor ASA
 * Copyright (c) 2026 STMicroelectronics
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

/**
 * STM32 pin configuration type
 *
 * The pin configuration is a bitfield encoded as follows:
 *	[04:00] Port (STM32_PORTx)
 *	[08:05] Pin
 *	[31:09] <Hardware-specific fields>
 *
 * The hardware-specific fields are described in sections below.
 */
typedef uint32_t pinctrl_soc_pin_t;

/* Definitions related to common fields */
#define STM32_PORT_Pos			0
#define STM32_PORT_Msk			(0x1F << STM32_PORT_Pos)

#define STM32_PIN_Pos			5
#define STM32_PIN_Msk			(0xF << STM32_PIN_Pos)

/*
 * Hardware-specific implementations
 */
#ifdef CONFIG_SOC_SERIES_STM32F1X
/**
 * STM32F1-specific pin configuration bitfield
 *
 * The hardware-specific fields are as follows:
 *	[12:09] CNFMODE
 *	[   13] PUPD Configuration / Output data
 *	 When CNFMODE is `0b1000: Input with pull-up/down`:
 *	   0b0 = Pull-up
 *	   0b1 = Pull-down
 *	 When CNFMODE is `0b1xxx: GP Output`:
 *	   0b0 = Output low level
 *	   0b1 = Output high level
 *	 When CNFMODE is any other value, this field is ignored.
 *	[24:14] REMAP information
 *	 (see `include/zephyr/dt-bindings/pinctrl/stm32f1-afio.h`)
 */

/*
 * Pin configuration (GPIOx_CRL / GPIOx_CRH)
 *
 * Each pin's configuration is controlled by two fields in
 * the GPIOx_CRL/GPIOx_CRH register: CNFn and MODEn. These
 * fields are next to each other, so we pack them together
 * in a single "CNFMODE" field in the pinctrl_soc_pin_t to
 * allow configuring pins with a single write. Definitions
 * for CNF and MODE will however be described separately.
 */
#define STM32_MODE_Pos			9
#define STM32_MODE_Msk			(0x3 << STM32_MODE_Pos)
#define STM32_MODE_INPUT		(0x0 << STM32_MODE_Pos)
#define STM32_MODE_OUT_MAX_SPEED_10MHZ	(0x1 << STM32_MODE_Pos)
#define STM32_MODE_OUT_MAX_SPEED_2MHZ	(0x2 << STM32_MODE_Pos)
#define STM32_MODE_OUT_MAX_SPEED_50MHZ	(0x3 << STM32_MODE_Pos)

#define STM32_CNF_Pos			11
#define STM32_CNF_Msk			(0x3 << STM32_CNF_Pos)
	/* These values apply when MODE is INPUT */
#define STM32_CNF_IN_ANALOG		(0x0 << STM32_CNF_Pos)
#define STM32_CNF_IN_FLOAT		(0x1 << STM32_CNF_Pos)
#define STM32_CNF_IN_PUPD		(0x2 << STM32_CNF_Pos)
	/* These values apply when MODE is OUT_MAX_SPEED_nMHZ */
#define STM32_CNF_OUT_PUSH_PULL		(0x0 << STM32_CNF_Pos)
#define STM32_CNF_OUT_OPEN_DRAIN	(0x1 << STM32_CNF_Pos)
#define STM32_CNF_OUT_GENERAL_PURPOSE	(0x0 << STM32_CNF_Pos)
#define STM32_CNF_OUT_ALT_FUNCTION	(0x2 << STM32_CNF_Pos)

/* Combined CNF + MODE field in pinctrl_soc_pin_t */
#define STM32_CNFMODE_Pos		STM32_MODE_Pos
#define STM32_CNFMODE_Msk		(STM32_CNF_Msk | STM32_MODE_Msk)
	/* Well-known values of CNFMODE for Input modes */
#define STM32_CNFMODE_ANALOG		(STM32_CNF_IN_ANALOG | STM32_MODE_INPUT)
#define STM32_CNFMODE_INPUT_FLOAT	(STM32_CNF_IN_FLOAT | STM32_MODE_INPUT)
#define STM32_CNFMODE_INPUT_PUPD	(STM32_CNF_IN_PUPD | STM32_MODE_INPUT)

#define STM32_ODR_Pos			13
#define STM32_ODR_Msk			(0x1 << STM32_ODR_Pos)
#define STM32_ODR_0			(0x0 << STM32_ODR_Pos)
#define STM32_ODR_1			(0x1 << STM32_ODR_Pos)
	/* PUPD is stored in the same bit as ODR (`union`-like) */
#define STM32_PUPD_Pos			STM32_ODR_Pos
#define STM32_PUPD_Msk			STM32_ODR_Msk
#define STM32_PUPD_PULL_DOWN		STM32_ODR_0
#define STM32_PUPD_PULL_UP		STM32_ODR_1

/* REMAP information */
#define STM32_REMAP_Pos			14
#define STM32_REMAP_Msk			(0x3FF << STM32_REMAP_Pos)

/**
 * Encode GPIO port, pin and remap information in `pinctrl_soc_pin_t`
 * format based on information stored in the `pinmux` property.
 *
 * @param pinmux Value of the `pinmux` property
 */
#define Z_PINCTRL_STM32_PMUX2PCFG_PORT_LINE_REMAP(pinmux)				\
	(((((pinmux) >> STM32_PORT_SHIFT) & STM32_PORT_MASK) << STM32_PORT_Pos) |	\
	 ((((pinmux) >> STM32_LINE_SHIFT) & STM32_LINE_MASK) << STM32_PIN_Pos) |	\
	 ((((pinmux) >> STM32_REMAP_SHIFT) & STM32_REMAP_MASK) << STM32_REMAP_Pos))

/**
 * Encode INPUT mode information in `pinctrl_soc_pin_t` format
 * based on pinctrl pin node properties.
 *
 * @pre The `mode` in `pinmux` property is GPIO_IN
 * @param node_id Pinctrl pin node ID
 */
#define Z_PINCTRL_STM32F1_GET_INPUT_CNFMODE_PUPD(node_id)				\
	(COND_CODE_0(									\
	UTIL_OR(DT_PROP(node_id, bias_pull_down), DT_PROP(node_id, bias_pull_up)),	\
		(STM32_CNFMODE_INPUT_FLOAT),						\
		(STM32_CNFMODE_INPUT_PUPD |						\
		  DT_PROP(node_id, bias_pull_up) * STM32_PUPD_PULL_UP)))

/**
 * Encode OUTPUT/ALTERNATE mode information in `pinctrl_soc_pin_t` format
 * based on pinctrl pin node properties.
 *
 * @param node_id Pinctrl pin node ID
 * @param pinmux_mode `mode` extracted from `pinmux` property
 * (must be GPIO_OUT or ALTERNATE)
 */
#define Z_PINCTRL_STM32F1_GET_OUTALT_CNFMODE_ODR(node_id, pinmux_mode)			\
	(CONCAT(STM32_MODE_OUT_, DT_STRING_UPPER_TOKEN(node_id, slew_rate)) |		\
	 (STM32_CNF_OUT_OPEN_DRAIN * DT_PROP(node_id, drive_open_drain)) |		\
	 (STM32_CNF_OUT_ALT_FUNCTION * (!!(pinmux_mode == ALTERNATE))) |		\
	 (STM32_ODR_1 * DT_PROP(node_id, output_high)))

/* Computes CNFMODE and {PUPD Configuration / Output data} fields */
#define Z_PINCTRL_STM32_CNFMODE_PUPDODR(node_id, pinmux_mode)				\
	((pinmux_mode == ANALOG)							\
	? (STM32_CNFMODE_ANALOG)							\
	: ((pinmux_mode == GPIO_IN)							\
		? Z_PINCTRL_STM32F1_GET_INPUT_CNFMODE_PUPD(node_id)			\
		: Z_PINCTRL_STM32F1_GET_OUTALT_CNFMODE_ODR(node_id, pinmux_mode)))

/**
 * @brief Utility macro to initialize `pinctrl_soc_pin_t` (STM32F1)
 *
 * @param node_id Pinctrl pin node identifier
 */
#define Z_PINCTRL_STM32_PIN_INIT(node_id)						\
	(Z_PINCTRL_STM32_PMUX2PCFG_PORT_LINE_REMAP(DT_PROP(node_id, pinmux)) |		\
	 Z_PINCTRL_STM32_CNFMODE_PUPDODR(node_id,					\
		((DT_PROP(node_id, pinmux) >> STM32_MODE_SHIFT) & STM32_MODE_MASK)))

#else /* CONFIG_SOC_SERIES_STM32F1X */
/**
 * STM32 pin configuration bitfield
 *
 * The following hardware-specific fields are common to all series:
 *	[12:09] Alternate Function
 *	[14:13] GPIO Mode
 *	[   15] GPIO Output type
 *	[17:16] GPIO Speed
 *	[19:18] GPIO PUPD config
 *	[   20] GPIO Output data
 *
 * The following fields apply only to compatible "st,stm32h5-pinctrl":
 *	[   21] High-speed low-voltage mode
 *
 * The following fields apply only to compatible "st,stm32n6-pinctrl":
 *	[24:21] I/O delay length
 *	[28:25] ADVCFG (I/O delay & retime configuration)
 */

/* GPIO AF */
#define STM32_AF_Pos			9
#define STM32_AF_Msk			(0xF << STM32_AF_Pos)

/* GPIO Mode */
#define STM32_MODER_Pos			13
#define STM32_MODER_Msk			(0x3 << STM32_MODER_Pos)
#define STM32_MODER_INPUT_MODE		(0x0 << STM32_MODER_Pos)
#define STM32_MODER_OUTPUT_MODE		(0x1 << STM32_MODER_Pos)
#define STM32_MODER_ALT_MODE		(0x2 << STM32_MODER_Pos)
#define STM32_MODER_ANALOG_MODE		(0x3 << STM32_MODER_Pos)

/* GPIO Output type */
#define STM32_OTYPER_Pos		15
#define STM32_OTYPER_Msk		(0x1 << STM32_OTYPER_Pos)
#define STM32_OTYPER_PUSH_PULL		(0x0 << STM32_OTYPER_Pos)
#define STM32_OTYPER_OPEN_DRAIN		(0x1 << STM32_OTYPER_Pos)

/* GPIO speed */
#define STM32_OSPEEDR_Pos		16
#define STM32_OSPEEDR_Msk		(0x3 << STM32_OSPEEDR_Pos)
#define STM32_OSPEEDR_LOW_SPEED		(0x0 << STM32_OSPEEDR_Pos)
#define STM32_OSPEEDR_MEDIUM_SPEED	(0x1 << STM32_OSPEEDR_Pos)
#define STM32_OSPEEDR_HIGH_SPEED	(0x2 << STM32_OSPEEDR_Pos)
#define STM32_OSPEEDR_VERY_HIGH_SPEED	(0x3 << STM32_OSPEEDR_Pos)

/* GPIO High impedance/Pull-up/pull-down */
#define STM32_PUPDR_Pos			18
#define STM32_PUPDR_Msk			(0x3 << STM32_PUPDR_Pos)
#define STM32_PUPDR_NO_PULL		(0x0 << STM32_PUPDR_Pos)
#define STM32_PUPDR_PULL_UP		(0x1 << STM32_PUPDR_Pos)
#define STM32_PUPDR_PULL_DOWN		(0x2 << STM32_PUPDR_Pos)

/* GPIO plain output value */
#define STM32_ODR_Pos			20
#define STM32_ODR_Msk			(0x1 << STM32_ODR_Pos)
#define STM32_ODR_0			(0x0 << STM32_ODR_Pos)
#define STM32_ODR_1			(0x1 << STM32_ODR_Pos)

/* I/O delay length (DELAYR) */
#define STM32_DELAYR_Pos		21
#define STM32_DELAYR_Msk		(0xFU << STM32_DELAYR_Pos)

/*
 * I/O delay & retime configuration (GPIOx_ADVCFGRL / GPIOx_ADVCFGRH)
 *
 * GPIOx_ADVCFGR{L,H} registers contain four bits per pin to configure
 * the I/O delay and retime feature: DLYPATHn, DEn, INVCLKn and RETn.
 * Since these fields are next to each other, we pack them in a single
 * 4-bit "ADVCFG" field in pinctrl_soc_pin_t to allow applying this
 * configuration with a single write.
 *
 * Since DEn and INVCLKn are related to the same aspect, we combine them
 * in a single "IORETIME_EDGE" pseudo-field which is part of ADVCFG.
 */
#define STM32_ADVCFGR_Pos		25
#define STM32_ADVCFGR_Msk		(0xFU << STM32_ADVCFGR_Pos)

#define STM32_IODELAY_DIRECTION_Pos	25 /* DLYPATH */
#define STM32_IORETIME_EDGE_Pos		26 /* INVCLK and DE */
#define STM32_IORETIME_ENABLE_Pos	28 /* RET */

/* High-speed low-voltage mode (HSLVR) */
#define STM32_HSLVR_Pos			21
#define STM32_HSLVR_Msk			(0x1 << STM32_HSLVR_Pos)
#define STM32_HSLVR_DISABLE		(0x0 << STM32_HSLVR_Pos)
#define STM32_HSLVR_ENABLED		(0x1 << STM32_HSLVR_Pos)

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
	((CONCAT(STM32_IOSYNC_DELAY_DIRECTION_, delay_path) << STM32_IODELAY_DIRECTION_Pos) | \
	 ((retime_enable) << STM32_IORETIME_ENABLE_Pos) |					\
	 (CONCAT(STM32_IOSYNC_RETIME_EDGE_, retime_edge) << STM32_IORETIME_EDGE_Pos) |	\
	 (CONCAT(LL_GPIO_DELAY_, delay_ps) << STM32_DELAYR_Pos))

/**
 * @brief Utility macro to initialize fields of `pinctrl_soc_pin_t`
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
 * Encode GPIO mode and AF index in pinctrl_soc_pin_t format based on
 * information stored in the `pinmux` property.
 *
 * Note that we extract `mode` using shift+mask which does not exist
 * as compile-time macro constructs. As such, IS_EQ() and other such
 * macros won't work so we have to use ternaries.
 *
 * @param pmux_mode `mode` extracted from the `pinmux` property
 * @param has_out_prop 1 if pinmux node has property `output-low`
 * or `output-high`, 0 otherwise
 */
#define Z_PINCTRL_STM32_PMUX2PCFG_MODE_AF(pmux_mode, has_out_prop)			\
	((pmux_mode == STM32_GPIO)							\
	? (COND_CODE_1(has_out_prop,							\
		(STM32_MODER_OUTPUT_MODE),						\
		(STM32_MODER_INPUT_MODE)))						\
	: (pmux_mode == STM32_ANALOG)							\
		? (STM32_MODER_ANALOG_MODE)						\
		: (STM32_MODER_ALT_MODE | ((pmux_mode) << STM32_AF_Pos)))

/**
 * Encode GPIO port and pin in @ref pinctrl_soc_pin_t format based on
 * information stored in the `pinmux` property.
 *
 * @param pinmux Value of the `pinmux` property
 */
#define Z_PINCTRL_STM32_PMUX2PCFG_PORT_LINE(pinmux)					\
	(((((pinmux) >> STM32_PORT_SHIFT) & STM32_PORT_MASK) << STM32_PORT_Pos) |	\
	 ((((pinmux) >> STM32_LINE_SHIFT) & STM32_LINE_MASK) << STM32_PIN_Pos))

#define Z_PINCTRL_STM32_PINMUX_TO_PINCFG(node_id)	\
	(Z_PINCTRL_STM32_PMUX2PCFG_PORT_LINE(DT_PROP(node_id, pinmux)) |		\
	 Z_PINCTRL_STM32_PMUX2PCFG_MODE_AF(						\
		((DT_PROP(node_id, pinmux) >> STM32_MODE_SHIFT) & STM32_MODE_MASK),	\
		UTIL_OR(DT_PROP(node_id, output_low), DT_PROP(node_id, output_high))))

/**
 * @brief Utility macro to initialize `pinctrl_soc_pin_t` for non-F1 series
 *
 * @param node_id Node identifier.
 */
#define Z_PINCTRL_STM32_PIN_INIT(node_id)					\
	(Z_PINCTRL_STM32_PINMUX_TO_PINCFG(node_id) |				\
	 CONCAT(STM32_OSPEEDR_, DT_STRING_UPPER_TOKEN(node_id, slew_rate)) |	\
	 (STM32_OTYPER_OPEN_DRAIN * DT_PROP(node_id, drive_open_drain)) |	\
	 (STM32_PUPDR_PULL_UP * DT_PROP(node_id, bias_pull_up)) |		\
	 (STM32_PUPDR_PULL_DOWN * DT_PROP(node_id, bias_pull_down)) |		\
	 (STM32_ODR_1 * DT_PROP(node_id, output_high)) |			\
	 Z_PINCTRL_STM32_IOSYNC_INIT(node_id) |					\
	 Z_PINCTRL_STM32_HSLV_INIT(node_id))

#endif /* CONFIG_SOC_SERIES_STM32F1X */

/* Generic macros bridging HW-specific implementation to pinctrl subsystem */

/**
 * @brief Utility macro to initialize each pin.
 *
 * @param node_id Node identifier.
 * @param state_prop State property name.
 * @param idx State property entry index.
 */
#define Z_PINCTRL_STATE_PIN_INIT(node_id, state_prop, idx)			\
	Z_PINCTRL_STM32_PIN_INIT(DT_PROP_BY_IDX(node_id, state_prop, idx)),

/**
 * @brief Utility macro to initialize state pins contained in a given property.
 *
 * @param node_id Node identifier.
 * @param prop Property name describing state pins.
 */
#define Z_PINCTRL_STATE_PINS_INIT(node_id, prop)				\
	{DT_FOREACH_PROP_ELEM(node_id, prop, Z_PINCTRL_STATE_PIN_INIT)}

/** INTERNAL_HIDDEN @endcond */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_SOC_ARM_ST_STM32_COMMON_PINCTRL_SOC_H_ */
