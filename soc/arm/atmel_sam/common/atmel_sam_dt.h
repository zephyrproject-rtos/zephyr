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
#define NODE_ID_FROM_PINCTRL_0(inst, i) \
	DT_INST_PHANDLE_BY_IDX(inst, pinctrl_0, i)

/* Get PIN associated with pinctrl-0 pin at index 'i' */
#define ATMEL_SAM_PIN(inst, i) \
	DT_PHA(NODE_ID_FROM_PINCTRL_0(inst, i), atmel_pins, pin)

/* Get PIO register address associated with pinctrl-0 pin at index 'i' */
#define ATMEL_SAM_PIN_TO_PIO_REG_ADDR(inst, i) \
	DT_REG_ADDR(DT_PHANDLE(NODE_ID_FROM_PINCTRL_0(inst, i), atmel_pins))

/* Get peripheral id for PIO associated with pinctrl-0 pin at index 'i' */
#define ATMEL_SAM_PIN_2_PIO_PERIPH_ID(inst, i) \
	DT_PROP_BY_PHANDLE(NODE_ID_FROM_PINCTRL_0(inst, i),\
			   atmel_pins, peripheral_id)

/* Get peripheral cfg associated wiith pinctrl-0 pin at index 'i' */
#define ATMEL_SAM_PIN_PERIPH(inst, i) \
	DT_PHA(NODE_ID_FROM_PINCTRL_0(inst, i), atmel_pins, peripheral)

/* Helper function for ATMEL_SAM_PIN_FLAGS */
#define ATMEL_SAM_PIN_FLAG(inst, i, flag) \
	DT_PROP(NODE_ID_FROM_PINCTRL_0(inst, i), flag)

/* Convert DT flags to SoC flags */
#define ATMEL_SAM_PIN_FLAGS(inst, i) \
	(ATMEL_SAM_PIN_FLAG(inst, i, bias_pull_up) << SOC_GPIO_PULLUP_POS | \
	 ATMEL_SAM_PIN_FLAG(inst, i, bias_pull_down) << SOC_GPIO_PULLUP_POS | \
	 ATMEL_SAM_PIN_FLAG(inst, i, drive_open_drain) << SOC_GPIO_OPENDRAIN_POS)

/* Construct a soc_pio_pin element for pin cfg */
#define ATMEL_SAM_DT_PIO(inst, idx)					\
	{								\
		1 << ATMEL_SAM_PIN(inst, idx),				\
		(Pio *)ATMEL_SAM_PIN_TO_PIO_REG_ADDR(inst, idx),	\
		ATMEL_SAM_PIN_2_PIO_PERIPH_ID(inst, idx),		\
		ATMEL_SAM_PIN_PERIPH(inst, idx) << SOC_GPIO_FUNC_POS |	\
		ATMEL_SAM_PIN_FLAGS(inst, idx)				\
	}

/* Construct a soc_gpio_pin element for pin cfg */
#define ATMEL_SAM_DT_GPIO(inst, idx)					\
	{								\
		1 << ATMEL_SAM_PIN(inst, idx),				\
		(Gpio *)ATMEL_SAM_PIN_TO_PIO_REG_ADDR(inst, idx),	\
		ATMEL_SAM_PIN_2_PIO_PERIPH_ID(inst, idx),		\
		ATMEL_SAM_PIN_PERIPH(inst, idx) << SOC_GPIO_FUNC_POS |	\
		ATMEL_SAM_PIN_FLAGS(inst, idx)				\
	}

#if defined(CONFIG_SOC_SERIES_SAM4L)
#define ATMEL_SAM_DT_PIN ATMEL_SAM_DT_GPIO
#else
#define ATMEL_SAM_DT_PIN ATMEL_SAM_DT_PIO
#endif

/* Get the number of pins for pinctrl-0 */
#define ATMEL_SAM_DT_NUM_PINS(inst) DT_INST_PROP_LEN(inst, pinctrl_0)

/* internal macro to structure things for use with UTIL_LISTIFY */
#define ATMEL_SAM_DT_PIN_ELEM(idx, inst) ATMEL_SAM_DT_PIN(inst, idx),

/* Construct an array intializer for soc_gpio_pin for a device instance */
#define ATMEL_SAM_DT_PINS(inst)					\
	{ UTIL_LISTIFY(ATMEL_SAM_DT_NUM_PINS(inst),		\
		       ATMEL_SAM_DT_PIN_ELEM, inst)		\
	}

/* Devicetree macros related to clock */

#define ATMEL_SAM_DT_CPU_CLK_FREQ_HZ \
		DT_PROP(DT_PATH(cpus, cpu_0), clock_frequency)

#endif /* _ATMEL_SAM_SOC_DT_H_ */
