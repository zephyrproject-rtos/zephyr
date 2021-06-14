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

typedef struct soc_gpio_pin pinctrl_dt_pin_spec_t;

/* TODO Add support for n parameter */
#define PINCTRL_DT_SPEC_GET(node_id, n) \
	ATMEL_SAM_DT_PINS(node_id)

#endif /* ATMEL_SAM_SOC_PINCTRL_H_ */
