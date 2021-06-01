/*
 * Copyright (c) 2021 Piotr Mienkowski
 * SPDX-License-Identifier: Apache-2.0
 */

/** @file
 * @brief Atmel SAM MCU family Pin Controller macros.
 */

#ifndef ATMEL_SAM_SOC_PINCTRL_H_
#define ATMEL_SAM_SOC_PINCTRL_H_

#include <atmel_sam_dt.h>
#include <soc_gpio.h>

typedef struct soc_gpio_pin pinctrl_soc_pins_t;

#if defined(CONFIG_SOC_SERIES_SAM4L)
#define ATMEL_PIO_TYPE Gpio
#else
#define ATMEL_PIO_TYPE Pio
#endif

#define PINCTRL_SOC_PINS_ELEM_INIT(node_id)					\
{										\
	.mask = 1 << DT_PHA(node_id, atmel_pins, pin),				\
	.regs = (ATMEL_PIO_TYPE *)DT_REG_ADDR(DT_PHANDLE(node_id, atmel_pins)),	\
	.periph_id = DT_PHA(node_id, atmel_pins, peripheral),			\
	.flags = DT_PHA(node_id, atmel_pins, peripheral) << SOC_GPIO_FUNC_POS | \
		 DT_PROP(node_id, bias_pull_up) << SOC_GPIO_PULLUP_POS | 	\
		 DT_PROP(node_id, bias_pull_down) << SOC_GPIO_PULLUP_POS | 	\
		 DT_PROP(node_id, drive_open_drain) << SOC_GPIO_OPENDRAIN_POS 	\
},

#endif /* ATMEL_SAM_SOC_PINCTRL_H_ */
