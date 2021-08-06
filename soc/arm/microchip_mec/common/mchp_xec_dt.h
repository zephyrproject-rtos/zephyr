/*
 * Copyright (c) 2020 Linaro Ltd.
 * Copyright (c) 2021 Microchip Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _MCHP_XEC_DT_H_
#define _MCHP_XEC_DT_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <devicetree.h>

#define MCHP_XEC_NO_PULL	0x0
#define MCHP_XEC_PULL_UP	0x1
#define MCHP_XEC_PULL_DOWN	0x2
#define MCHP_XEC_REPEATER	0x3
#define MCHP_XEC_PUSH_PULL	0x0
#define MCHP_XEC_OPEN_DRAIN	0x1

/**
 * @brief Internal: Get a node indentifier for an element in a
 *        pinctrl-x property for a given device instance inst
 *
 * @param inst device instance number
 * @param x index of targeted pinctrl- property (eg: pinctrl-<x>)
 * @param i index of soc_gpio_pinctrl element
 * @return elements's node identifier
 */
#define MCHP_XEC_DT_INST_NODE_ID_FROM_PINCTRL(inst, x, i) \
	DT_INST_PHANDLE_BY_IDX(inst, pinctrl_##x, i)

/**
 * @brief Internal: Get a node indentifier for an element in a
 *        pinctrl-x property for a given device
 *
 * @param name device node label identifier
 * @param x index of targeted pinctrl- property (eg: pinctrl-<x>)
 * @param i index of soc_gpio_pinctrl element
 * @return elements's node identifier
 */
#define MCHP_XEC_DT_NODE_ID_FROM_PINCTRL(name, x, i) \
	DT_PHANDLE_BY_IDX(DT_NODELABEL(name), pinctrl_##x, i)

/**
 * @brief Internal: Get pinmux property of a node indentifier for an element
 *        in a pinctrl-x property for a given device instance inst
 *
 * @param inst device instance number
 * @param x index of targeted pinctrl- property (eg: pinctrl-<x>)
 * @param i index of soc_gpio_pinctrl element
 * @return pinmux property value
 */
#define MCHP_XEC_DT_INST_PINMUX(inst, x, i) \
	DT_PROP(MCHP_XEC_DT_INST_NODE_ID_FROM_PINCTRL(inst, x, i), pinmux)

/**
 * @brief Internal: Get pinmux property of a node indentifier for an element
 *        in a pinctrl-x property for a given device
 *
 * @param name device node label identifier
 * @param x index of targeted pinctrl- property (eg: pinctrl-<x>)
 * @param i index of soc_gpio_pinctrl element
 * @return pinmux property value
 */
#define MCHP_XEC_DT_PINMUX(name, x, i) \
	DT_PROP(MCHP_XEC_DT_NODE_ID_FROM_PINCTRL(name, x, i), pinmux)

/**
 * @brief Internal: Get <function> property of a node indentifier for an element
 *        in a pinctrl-x property for a given device instance inst
 *
 * @param inst device instance number
 * @param x index of targeted pinctrl- property (eg: pinctrl-<x>)
 * @param i index of soc_gpio_pinctrl element
 * @param function property of a targeted element
 * @return <function> property value
 */
#define MCHP_XEC_DT_INST_FUNC(inst, x, i, function) \
	DT_PROP(MCHP_XEC_DT_INST_NODE_ID_FROM_PINCTRL(inst, x, i), function)

/**
 * @brief Internal: Get <function> property of a node indentifier for an element
 *        in a pinctrl-x property for a given device
 *
 * @param inst device instance number
 * @param x index of targeted pinctrl- property (eg: pinctrl-<x>)
 * @param name device node label identifier
 * @param function property of a targeted element
 * @return <function> property value
 */
#define MCHP_XEC_DT_FUNC(name, x, i, function) \
	DT_PROP(MCHP_XEC_DT_NODE_ID_FROM_PINCTRL(name, x, i), function)

/**
 * @brief Internal: Provide slew-rate value for a pin with index i of a
 *        pinctrl-x property for a given device instance inst
 *
 * @param inst device instance number
 * @param x index of targeted pinctrl- property (eg: pinctrl-<x>)
 * @param i index of soc_gpio_pinctrl element
 * @return slew rate value
 */
#define MCHP_XEC_DT_INST_SLEW_RATE(inst, x, i) \
	DT_ENUM_IDX(MCHP_XEC_DT_INST_NODE_ID_FROM_PINCTRL(inst, x, i), \
		    slew_rate)

/**
 * @brief Internal: Provide slew-rate value for a pin with index i of a
 *        pinctrl-x property for a given device
 *
 * @param inst device instance number
 * @param x index of targeted pinctrl- property (eg: pinctrl-<x>)
 * @param name device node label identifier
 * @return slew rate value
 */
#define MCHP_XEC_DT_SLEW_RATE(name, x, i) \
	DT_ENUM_IDX(MCHP_XEC_DT_NODE_ID_FROM_PINCTRL(name, x, i), slew_rate)

/**
 * @brief Internal: Provide drive-strength value for a pin with index i of a
 *        pinctrl-x property for a given device instance inst
 *
 * @param inst device instance number
 * @param x index of targeted pinctrl- property (eg: pinctrl-<x>)
 * @param i index of soc_gpio_pinctrl element
 * @return drive strength value
 */
#define MCHP_XEC_DT_INST_DRIVE_STRENGTH(name, x, i) \
	DT_ENUM_IDX(MCHP_XEC_DT_INST_NODE_ID_FROM_PINCTRL(name, x, i), \
		    drive_strength)

/**
 * @brief Internal: Provide drive-strength value for a pin with index i of a
 *        pinctrl-x property for a given device
 *
 * @param inst device instance number
 * @param x index of targeted pinctrl- property (eg: pinctrl-<x>)
 * @param name device node label identifier
 * @return drive strength value
 */
#define MCHP_XEC_DT_DRIVE_STRENGTH(name, x, i) \
	DT_ENUM_IDX(MCHP_XEC_DT_NODE_ID_FROM_PINCTRL(name, x, i), \
		    drive_strength)

/**
 * @brief Internal: Contruct a pincfg field of a soc_gpio_pinctrl element
 *        with index i of a pinctrl-x property for a given device instance inst
 *
 * @param i index of soc_gpio_pinctrl element
 * @param x index of targeted pinctrl- property (eg: pinctrl-<x>)
 * @param inst device instance number
 * @return pincfg field
 */
#define MCHP_XEC_DT_INST_PINCFG(inst, x, i)				\
	(((MCHP_XEC_NO_PULL *						\
	   MCHP_XEC_DT_INST_FUNC(inst, x, i, bias_disable))		\
	   << MCHP_XEC_PUPDR_SHIFT) |					\
	 ((MCHP_XEC_PULL_UP *						\
	   MCHP_XEC_DT_INST_FUNC(inst, x, i, bias_pull_up))		\
	   << MCHP_XEC_PUPDR_SHIFT) |					\
	 ((MCHP_XEC_PULL_DOWN *						\
	   MCHP_XEC_DT_INST_FUNC(inst, x, i, bias_pull_down))		\
	   << MCHP_XEC_PUPDR_SHIFT) |					\
	 ((MCHP_XEC_PUSH_PULL *						\
	   MCHP_XEC_DT_INST_FUNC(inst, x, i, drive_push_pull))		\
	   << MCHP_XEC_OTYPER_SHIFT) |					\
	 ((MCHP_XEC_OPEN_DRAIN *					\
	   MCHP_XEC_DT_INST_FUNC(inst, x, i, drive_open_drain))		\
	   << MCHP_XEC_OTYPER_SHIFT) |					\
	 ((MCHP_XEC_DT_INST_SLEW_RATE(inst, x, i))			\
	  << MCHP_XEC_OSLEWR_SHIFT) |					\
	 ((MCHP_XEC_DT_INST_DRIVE_STRENGTH(inst, x, i))			\
	  << MCHP_XEC_ODSR_SHIFT)					\
	)

/**
 * @brief Internal: Contruct a soc_gpio_pinctrl element index i of
 *        a pinctrl-x property for a given device instance inst
 *
 * @param i element index
 * @param x index of targeted pinctrl- property (eg: pinctrl-<x>)
 * @param inst device instance number
 * @return soc_gpio_pinctrl element
 */
#define MCHP_XEC_DT_INST_PIN_ELEM(i, x, inst)			\
	{							\
		MCHP_XEC_DT_INST_PINMUX(inst, x, i),		\
		MCHP_XEC_DT_INST_PINCFG(inst, x, i)		\
	},

/**
 * @brief Internal: Contruct a soc_gpio_pinctrl element index i of
 *        a pinctrl-x property for a given device
 *
 * @param i element index
 * @param x index of targeted pinctrl- property (eg: pinctrl-<x>)
 * @param name device node label identifier
 * @return soc_gpio_pinctrl element
 */
#define MCHP_XEC_DT_PIN_ELEM(i, x, name)		\
	{						\
		MCHP_XEC_DT_PINMUX(name, x, i),		\
		MCHP_XEC_DT_PINCFG(name, x, i)		\
	},

/**
 * @brief Internal: Return the number of elements of a pinctrl-x property
 *        for a given device instance
 *
 * This macro returns the number of element in property pinctrl-<x> of
 * device instance <inst>
 *
 * @param inst device instance number
 * @param x index of targeted pinctrl- property (eg: pinctrl-<x>)
 * @return number of element in property
 */
#define MCHP_XEC_DT_INST_NUM_PINS(inst, x) DT_INST_PROP_LEN(inst, pinctrl_##x)

/**
 * @brief Internal: Return the number of elements of a pinctrl-x property
 *        for a given device
 *
 * This macro returns the number of element in property pinctrl-<x> of
 * device "name"
 *
 * @param name device node label identifier
 * @param x index of targeted pinctrl- property (eg: pinctrl-<x>)
 * @return number of element in property
 */
#define MCHP_XEC_DT_NUM_PINS(name, x) \
				DT_PROP_LEN(DT_NODELABEL(name), pinctrl_##x)

/**
 * @brief Construct a soc_gpio_pinctrl array of a specific pinctrl property
 *        for a given device instance
 *
 * This macro returns an array of soc_gpio_pinctrl, each line matching a pinctrl
 * configuration provived in property pcintrl-<x> of device instance <inst>
 *
 * @param inst device instance number
 * @param x index of targeted pinctrl- property (eg: pinctrl-<x>)
 * @return array of soc_gpio_pinctrl
 */
#define MCHP_XEC_DT_INST_PINCTRL(inst, x)				\
	{ COND_CODE_1(DT_INST_NODE_HAS_PROP(inst, pinctrl_##x),		\
		      (UTIL_LISTIFY(MCHP_XEC_DT_INST_NUM_PINS(inst, x),	\
				   MCHP_XEC_DT_INST_PIN_ELEM,		\
				   x,					\
				   inst)),				\
		      ())						\
	}

/**
 * @brief Construct a soc_gpio_pinctrl array of a specific pinctrl property
 *        for a given device name
 *
 * This macro returns an array of soc_gpio_pinctrl, each line matching a pinctrl
 * configuration provived in property pcintrl-<x> of a device referenced by
 * its node label identifier.
 *
 * @param name device node label identifier
 * @param x index of targeted pinctrl- property (eg: pinctrl-<x>)
 * @return array of soc_gpio_pinctrl
 */
#define MCHP_XEC_DT_PINCTRL(name, x)					\
	{ COND_CODE_1(DT_NODE_HAS_PROP(DT_NODELABEL(name), pinctrl_##x),\
		      (UTIL_LISTIFY(MCHP_XEC_DT_NUM_PINS(name, x),	\
				   MCHP_XEC_DT_PIN_ELEM,		\
				   x,					\
				   name)),				\
		      ())						\
	}

#ifdef __cplusplus
}
#endif

#endif /* _MCHP_XEC_DT_H_ */
