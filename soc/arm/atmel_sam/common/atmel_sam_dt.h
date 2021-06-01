/*
 * Copyright (c) 2020 Linaro Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/** @file
 * @brief Atmel SAM MCU family devicetree helper macros
 */

#ifndef _ATMEL_SAM_DT_H_
#define _ATMEL_SAM_DT_H_

#include <devicetree.h>

/* Devicetree related macros to construct pin mux config data */

/* Get a node id from a pinctrl-0 prop at index 'i' */
#define NODE_ID_FROM_PINCTRL_0(node_id, i) \
	DT_PHANDLE_BY_IDX(node_id, pinctrl_0, i)

/* Get PIN associated with pinctrl-0 pin at index 'i' */
#define ATMEL_SAM_PIN(node_id, i) \
	DT_PHA(NODE_ID_FROM_PINCTRL_0(node_id, i), atmel_pins, pin)

/* Get PIO register address associated with pinctrl-0 pin at index 'i' */
#define ATMEL_SAM_PIN_TO_PIO_REG_ADDR(node_id, i) \
	DT_REG_ADDR(DT_PHANDLE(NODE_ID_FROM_PINCTRL_0(node_id, i), atmel_pins))

/* Get peripheral id for PIO associated with pinctrl-0 pin at index 'i' */
#define ATMEL_SAM_PIN_2_PIO_PERIPH_ID(node_id, i) \
	DT_PROP_BY_PHANDLE(NODE_ID_FROM_PINCTRL_0(node_id, i),\
			   atmel_pins, peripheral_id)

/* Get peripheral cfg associated wiith pinctrl-0 pin at index 'i' */
#define ATMEL_SAM_PIN_PERIPH(node_id, i) \
	DT_PHA(NODE_ID_FROM_PINCTRL_0(node_id, i), atmel_pins, peripheral)

/* Helper function for ATMEL_SAM_PIN_FLAGS */
#define ATMEL_SAM_PIN_FLAG(node_id, i, flag) \
	DT_PROP(NODE_ID_FROM_PINCTRL_0(node_id, i), flag)

/* Convert DT flags to SoC flags */
#define ATMEL_SAM_PIN_FLAGS(node_id, i) \
	(ATMEL_SAM_PIN_FLAG(node_id, i, bias_pull_up) << SOC_GPIO_PULLUP_POS | \
	 ATMEL_SAM_PIN_FLAG(node_id, i, bias_pull_down) << SOC_GPIO_PULLUP_POS | \
	 ATMEL_SAM_PIN_FLAG(node_id, i, drive_open_drain) << SOC_GPIO_OPENDRAIN_POS)

#if defined(CONFIG_SOC_SERIES_SAM4L)
/* Construct a soc_gpio_pin element for pin cfg */
#define ATMEL_SAM_DT_GPIO(node_id, idx)					\
	{								\
		1 << ATMEL_SAM_PIN(node_id, idx),			\
		(Gpio *)ATMEL_SAM_PIN_TO_PIO_REG_ADDR(node_id, idx),	\
		ATMEL_SAM_PIN_2_PIO_PERIPH_ID(node_id, idx),		\
		ATMEL_SAM_PIN_PERIPH(node_id, idx) << SOC_GPIO_FUNC_POS | \
		ATMEL_SAM_PIN_FLAGS(node_id, idx)			\
	}

#define ATMEL_SAM_DT_INST_GPIO(inst, idx) \
	ATMEL_SAM_DT_GPIO(DT_DRV_INST(inst), idx)

#define ATMEL_SAM_DT_PIN ATMEL_SAM_DT_GPIO
#define ATMEL_SAM_DT_INST_PIN ATMEL_SAM_DT_INST_GPIO
#else
/* Construct a soc_pio_pin element for pin cfg */
#define ATMEL_SAM_DT_PIO(node_id, idx)					\
	{								\
		1 << ATMEL_SAM_PIN(node_id, idx),			\
		(Pio *)ATMEL_SAM_PIN_TO_PIO_REG_ADDR(node_id, idx),	\
		ATMEL_SAM_PIN_2_PIO_PERIPH_ID(node_id, idx),		\
		ATMEL_SAM_PIN_PERIPH(node_id, idx) << SOC_GPIO_FUNC_POS | \
		ATMEL_SAM_PIN_FLAGS(node_id, idx)			\
	}

#define ATMEL_SAM_DT_INST_PIO(inst, idx) \
	ATMEL_SAM_DT_PIO(DT_DRV_INST(inst), idx)

#define ATMEL_SAM_DT_PIN ATMEL_SAM_DT_PIO
#define ATMEL_SAM_DT_INST_PIN ATMEL_SAM_DT_INST_PIO
#endif

/* Get the number of pins for pinctrl-0 */
#define ATMEL_SAM_DT_NUM_PINS(node_id) DT_PROP_LEN(node_id, pinctrl_0)

#define ATMEL_SAM_DT_INST_NUM_PINS(inst) \
	ATMEL_SAM_DT_NUM_PINS(DT_DRV_INST(inst))

/* internal macro to structure things for use with UTIL_LISTIFY */
#define ATMEL_SAM_DT_PIN_ELEM(idx, node_id) ATMEL_SAM_DT_PIN(node_id, idx),

/* Construct an array intializer for soc_gpio_pin for a device instance */
#define ATMEL_SAM_DT_PINS(node_id)				\
	{ UTIL_LISTIFY(ATMEL_SAM_DT_NUM_PINS(node_id),		\
		       ATMEL_SAM_DT_PIN_ELEM, node_id)		\
	}

#define ATMEL_SAM_DT_INST_PINS(inst) \
	ATMEL_SAM_DT_PINS(DT_DRV_INST(inst))

/* Devicetree macros related to clock */

#define ATMEL_SAM_DT_CPU_CLK_FREQ_HZ \
		DT_PROP(DT_PATH(cpus, cpu_0), clock_frequency)

#endif /* _ATMEL_SAM_SOC_DT_H_ */
