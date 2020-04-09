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

/* Construct a soc_gpio_pin element for pin cfg */
#define ATMEL_SAM_DT_PIN(inst, idx)				\
	{							\
		1 << ATMEL_SAM_PIN(inst, idx),			\
		(Pio *)ATMEL_SAM_PIN_TO_PIO_REG_ADDR(inst, idx),\
		ATMEL_SAM_PIN_2_PIO_PERIPH_ID(inst, idx),	\
		ATMEL_SAM_PIN_PERIPH(inst, idx) << 16		\
	}

#endif /* _ATMEL_SAM_SOC_DT_H_ */
