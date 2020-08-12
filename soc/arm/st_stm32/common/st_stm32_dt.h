/*
 * Copyright (c) 2020 Linaro Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/** @file
 * @brief ST STM32 MCU family devicetree helper macros
 */

#ifndef _ST_STM32_DT_H_
#define _ST_STM32_DT_H_

#include <devicetree.h>

/* Devicetree related macros to construct pinctrl config data */

#define STM32_NO_PULL    0x0
#define STM32_PULL_UP    0x1
#define STM32_PULL_DOWN  0x2
#define STM32_PUSH_PULL  0x0
#define STM32_OPEN_DRAIN 0x1

/* Get a node id from a pinctrl-0 prop at index 'i' */
#define NODE_ID_FROM_PINCTRL_0(inst, i) \
	DT_INST_PHANDLE_BY_IDX(inst, pinctrl_0, i)

/* Get PIN associated with pinctrl-0 pin at index 'i' */

#define ST_STM32_PINMUX(inst, i) \
	DT_PROP(NODE_ID_FROM_PINCTRL_0(inst, i), pinmux)

#define ST_STM32_FUNC(inst, i, function) \
	DT_PROP(NODE_ID_FROM_PINCTRL_0(inst, i), function)

#define ST_STM32_SLEW_RATE(inst, i) \
	DT_ENUM_IDX(NODE_ID_FROM_PINCTRL_0(inst, i), slew_rate)

#ifndef CONFIG_SOC_SERIES_STM32F1X
#define ST_STM32_PINCFG(inst, i) \
	(((STM32_NO_PULL * ST_STM32_FUNC(inst, i, bias_disable))	 \
						<< STM32_PUPDR_SHIFT) |	 \
	((STM32_PULL_UP * ST_STM32_FUNC(inst, i, bias_pull_up))		 \
						<< STM32_PUPDR_SHIFT) |	 \
	((STM32_PULL_DOWN * ST_STM32_FUNC(inst, i, bias_pull_down))	 \
						<< STM32_PUPDR_SHIFT) |	 \
	((STM32_PUSH_PULL * ST_STM32_FUNC(inst, i, drive_push_pull))	 \
						<< STM32_OTYPER_SHIFT) | \
	((STM32_OPEN_DRAIN * ST_STM32_FUNC(inst, i, drive_open_drain))	 \
						<< STM32_OTYPER_SHIFT) | \
	(ST_STM32_SLEW_RATE(inst, i) << STM32_OSPEEDR_SHIFT))
#else
#define ST_STM32_PINCFG(inst, i) \
	(((STM32_NO_PULL * ST_STM32_FUNC(inst, i, bias_disable))	 \
						<< STM32_PUPD_SHIFT) |	 \
	((STM32_PULL_UP * ST_STM32_FUNC(inst, i, bias_pull_up))		 \
						<< STM32_PUPD_SHIFT) |	 \
	((STM32_PULL_DOWN * ST_STM32_FUNC(inst, i, bias_pull_down))	 \
						<< STM32_PUPD_SHIFT) |	 \
	((STM32_PUSH_PULL * ST_STM32_FUNC(inst, i, drive_push_pull))	 \
						<< STM32_CNF_OUT_0_SHIFT) | \
	((STM32_OPEN_DRAIN * ST_STM32_FUNC(inst, i, drive_open_drain))	 \
						<< STM32_CNF_OUT_0_SHIFT) | \
	(ST_STM32_SLEW_RATE(inst, i) << STM32_MODE_OSPEED_SHIFT))
#endif /* CONFIG_SOC_SERIES_STM32F1X */

/* Construct a soc_gpio_pinctrl element for pin cfg */
#define ST_STM32_DT_PIN(inst, idx)				\
	{							\
		ST_STM32_PINMUX(inst, idx),			\
		ST_STM32_PINCFG(inst, idx)			\
	}

/* Get the number of pins for pinctrl-0 */
#define ST_STM32_DT_NUM_PINS(inst) DT_INST_PROP_LEN(inst, pinctrl_0)

/* internal macro to structure things for use with UTIL_LISTIFY */
#define ST_STM32_DT_PIN_ELEM(idx, inst) ST_STM32_DT_PIN(inst, idx),

/* Construct an array intializer for soc_gpio_pinctrl for a device instance */
#define ST_STM32_DT_PINCTRL(pinctrl_idx, inst)				\
	{ COND_CODE_1(DT_INST_NODE_HAS_PROP(inst, pinctrl_##pinctrl_idx),\
		      (UTIL_LISTIFY(ST_STM32_DT_NUM_PINS(inst),		\
				   ST_STM32_DT_PIN_ELEM,		\
				   inst)),				\
		      ())						\
	}

#endif /* _ST_STM32_DT_H_ */
