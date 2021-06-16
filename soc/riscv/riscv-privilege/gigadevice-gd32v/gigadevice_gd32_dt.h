/*
 * Copyright (c) 2020 Linaro Ltd.
 * Copyright (c) 2021 Tokita, Hiroshi <tokita.hiroshi@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/** @file
 * @brief GigaDevice GD32 MCU family devicetree helper macros
 */

#ifndef _GIGADEVICE_GD32_DT_H_
#define _GIGADEVICE_GD32_DT_H_

#include <devicetree.h>

/* Devicetree related macros to construct pinctrl config data */

#define GD32_NO_PULL    0x0
#define GD32_PULL_UP    0x1
#define GD32_PULL_DOWN  0x2
#define GD32_PUSH_PULL  0x0
#define GD32_OPEN_DRAIN 0x1


/**
 * @brief Internal: Get a node indentifier for an element in a
 *        pinctrl-x property for a given device instance inst
 *
 * @param inst device instance number
 * @param x index of targeted pinctrl- property (eg: pinctrl-<x>)
 * @param i index of soc_gpio_pinctrl element
 * @return elements's node identifier
 */
#define GIGADEVICE_GD32_DT_INST_NODE_ID_FROM_PINCTRL(inst, x, i) \
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
#define GIGADEVICE_GD32_DT_NODE_ID_FROM_PINCTRL(name, x, i) \
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
#define GIGADEVICE_GD32_DT_INST_PINMUX(inst, x, i) \
	DT_PROP(GIGADEVICE_GD32_DT_INST_NODE_ID_FROM_PINCTRL(inst, x, i), pinmux)

/**
 * @brief Internal: Get pinmux property of a node indentifier for an element
 *        in a pinctrl-x property for a given device
 *
 * @param name device node label identifier
 * @param x index of targeted pinctrl- property (eg: pinctrl-<x>)
 * @param i index of soc_gpio_pinctrl element
 * @return pinmux property value
 */
#define GIGADEVICE_GD32_DT_PINMUX(name, x, i) \
	DT_PROP(GIGADEVICE_GD32_DT_NODE_ID_FROM_PINCTRL(name, x, i), pinmux)

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
#define GIGADEVICE_GD32_DT_INST_FUNC(inst, x, i, function) \
	DT_PROP(GIGADEVICE_GD32_DT_INST_NODE_ID_FROM_PINCTRL(inst, x, i), function)

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
#define GIGADEVICE_GD32_DT_FUNC(name, x, i, function) \
	DT_PROP(GIGADEVICE_GD32_DT_NODE_ID_FROM_PINCTRL(name, x, i), function)

/**
 * @brief Internal: Provide slew-rate value for a pin with index i of a
 *        pinctrl-x property for a given device instance inst
 *
 * @param inst device instance number
 * @param x index of targeted pinctrl- property (eg: pinctrl-<x>)
 * @param i index of soc_gpio_pinctrl element
 * @return slew rate value
 */
#define GIGADEVICE_GD32_DT_INST_SLEW_RATE(inst, x, i)			      \
	DT_ENUM_IDX(GIGADEVICE_GD32_DT_INST_NODE_ID_FROM_PINCTRL(inst, x, i), \
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
#define GIGADEVICE_GD32_DT_SLEW_RATE(name, x, i) \
	DT_ENUM_IDX(GIGADEVICE_GD32_DT_NODE_ID_FROM_PINCTRL(name, x, i), slew_rate)

/**
 * @brief Internal: Contruct a pincfg field of a soc_gpio_pinctrl element
 *        with index i of a pinctrl-x property for a given device instance inst
 *
 * @param i index of soc_gpio_pinctrl element
 * @param x index of targeted pinctrl- property (eg: pinctrl-<x>)
 * @param inst device instance number
 * @return pincfg field
 */
#define GIGADEVICE_GD32_DT_INST_PINCFG(inst, x, i)				       \
	(((GD32_NO_PULL * GIGADEVICE_GD32_DT_INST_FUNC(inst, x, i, bias_disable))      \
		<< GD32_PUPD_SHIFT) |						       \
	 ((GD32_PULL_UP * GIGADEVICE_GD32_DT_INST_FUNC(inst, x, i, bias_pull_up))      \
		<< GD32_PUPD_SHIFT) |						       \
	 ((GD32_PULL_DOWN * GIGADEVICE_GD32_DT_INST_FUNC(inst, x, i, bias_pull_down))  \
		<< GD32_PUPD_SHIFT) |						       \
	 ((GD32_PUSH_PULL * GIGADEVICE_GD32_DT_INST_FUNC(inst, x, i, drive_push_pull)) \
		<< GD32_CNF_OUT_0_SHIFT) |					       \
	 ((GD32_OPEN_DRAIN * GIGADEVICE_GD32_DT_INST_FUNC(inst, x, i,		       \
							  drive_open_drain))	       \
		<< GD32_CNF_OUT_0_SHIFT) |					       \
	 (GIGADEVICE_GD32_DT_INST_SLEW_RATE(inst, x, i) << GD32_MODE_OSPEED_SHIFT))

/**
 * @brief Internal: Contruct a pincfg field of a soc_gpio_pinctrl element
 *        with index i of a pinctrl-x property for a given device
 *
 * @param i index of soc_gpio_pinctrl element
 * @param x index of targeted pinctrl- property (eg: pinctrl-<x>)
 * @param name device node label identifier
 * @return pincfg field
 */
#define GIGADEVICE_GD32_DT_PINCFG(name, x, i)					    \
	(((GD32_NO_PULL * GIGADEVICE_GD32_DT_FUNC(name, x, i, bias_disable))	    \
		<< GD32_PUPD_SHIFT) |						    \
	 ((GD32_PULL_UP * GIGADEVICE_GD32_DT_FUNC(name, x, i, bias_pull_up))	    \
		<< GD32_PUPD_SHIFT) |						    \
	 ((GD32_PULL_DOWN * GIGADEVICE_GD32_DT_FUNC(name, x, i, bias_pull_down))    \
		<< GD32_PUPD_SHIFT) |						    \
	 ((GD32_PUSH_PULL * GIGADEVICE_GD32_DT_FUNC(name, x, i, drive_push_pull))   \
		<< GD32_CNF_OUT_0_SHIFT) |					    \
	 ((GD32_OPEN_DRAIN * GIGADEVICE_GD32_DT_FUNC(name, x, i, drive_open_drain)) \
		<< GD32_CNF_OUT_0_SHIFT) |					    \
	 (GIGADEVICE_GD32_DT_SLEW_RATE(name, x, i) << GD32_MODE_OSPEED_SHIFT))

/**
 * @brief Internal: Contruct a soc_gpio_pinctrl element index i of
 *        a pinctrl-x property for a given device instance inst
 *
 * @param i element index
 * @param x index of targeted pinctrl- property (eg: pinctrl-<x>)
 * @param inst device instance number
 * @return soc_gpio_pinctrl element
 */
#define GIGADEVICE_GD32_DT_INST_PIN_ELEM(i, x, inst)	    \
	{						    \
		GIGADEVICE_GD32_DT_INST_PINMUX(inst, x, i), \
		GIGADEVICE_GD32_DT_INST_PINCFG(inst, x, i)  \
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
#define GIGADEVICE_GD32_DT_PIN_ELEM(i, x, name)	       \
	{					       \
		GIGADEVICE_GD32_DT_PINMUX(name, x, i), \
		GIGADEVICE_GD32_DT_PINCFG(name, x, i)  \
	},

/**
 * @brief Internal: Return the number of elements of a pinctrl-x property
 *        for a given device instance
 *
 * This macro returns the number of element in property pcintrl-<x> of
 * device instance <inst>
 *
 * @param inst device instance number
 * @param x index of targeted pinctrl- property (eg: pinctrl-<x>)
 * @return number of element in property
 */
#define GIGADEVICE_GD32_DT_INST_NUM_PINS(inst, x) DT_INST_PROP_LEN(inst, pinctrl_##x)

/**
 * @brief Internal: Return the number of elements of a pinctrl-x property
 *        for a given device
 *
 * This macro returns the number of element in property pcintrl-<x> of
 * device "name"
 *
 * @param name device node label identifier
 * @param x index of targeted pinctrl- property (eg: pinctrl-<x>)
 * @return number of element in property
 */
#define GIGADEVICE_GD32_DT_NUM_PINS(name, x) \
	DT_PROP_LEN(DT_NODELABEL(name), pinctrl_##x)

/**
 * @brief Construct a soc_gpio_pinctrl array of a specific pcintrl property
 *        for a given device instance
 *
 * This macro returns an array of soc_gpio_pinctrl, each line matching a pinctrl
 * configuration provived in property pcintrl-<x> of device instance <inst>
 *
 * @param inst device instance number
 * @param x index of targeted pinctrl- property (eg: pinctrl-<x>)
 * @return array of soc_gpio_pinctrl
 */
#define GIGADEVICE_GD32_DT_INST_PINCTRL(inst, x)			       \
	{ COND_CODE_1(DT_INST_NODE_HAS_PROP(inst, pinctrl_##x),		       \
		      (UTIL_LISTIFY(GIGADEVICE_GD32_DT_INST_NUM_PINS(inst, x), \
				    GIGADEVICE_GD32_DT_INST_PIN_ELEM,	       \
				    x,					       \
				    inst)),				       \
		      ())						       \
	}

/**
 * @brief Construct a soc_gpio_pinctrl array of a specific pcintrl property
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
#define GIGADEVICE_GD32_DT_PINCTRL(name, x)				  \
	{ COND_CODE_1(DT_NODE_HAS_PROP(DT_NODELABEL(name), pinctrl_##x),  \
		      (UTIL_LISTIFY(GIGADEVICE_GD32_DT_NUM_PINS(name, x), \
				    GIGADEVICE_GD32_DT_PIN_ELEM,	  \
				    x,					  \
				    name)),				  \
		      ())						  \
	}

#endif /* _GIGADEVICE_GD32_DT_H_ */
