/*
 * Copyright (c) 2024 Antmicro <www.antmicro.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SOC_ARM_XLNX_ZYNQMP_SOC_PINCTRL_H_
#define ZEPHYR_SOC_ARM_XLNX_ZYNQMP_SOC_PINCTRL_H_

#include <zephyr/devicetree.h>
#include <zephyr/sys/util.h>
#include <zephyr/types.h>

#define MIO_L0_SEL     BIT(1)
#define MIO_L1_SEL     BIT(2)
#define MIO_L2_SEL     GENMASK(4, 3)
#define MIO_L3_SEL     GENMASK(7, 5)

/* All other selectors should be zeroed and FIELD_PREP does that */
#define UARTX_SEL FIELD_PREP(MIO_L3_SEL, 6)

/*
 * Each peripheral PINCTRL mask is defined as such:
 * [7 ... 0]  MIO register number
 * [15 ... 8] Function, mapped as:
 *     1 - UART
 *
 * The function numbers serve as an enumerator in the pinctrl driver
 * and the defines controling those are listed in `pinctrl-zynqmp.h`.
 * Currently, one function for UART is specified and subsequent ones
 * can be added when the need arises.
 */

typedef struct pinctrl_soc_pin_t {
	uint32_t pin;
	uint32_t func;
} pinctrl_soc_pin_t;


#define ZYNQMP_GET_PIN(pinctrl)  (pinctrl & 0xff)
#define ZYNQMP_GET_FUNC(pinctrl) ((pinctrl >> 8) & 0xff)

#define Z_PINCTRL_STATE_PIN_INIT(node_id, prop, idx) \
		{ \
				.pin = ZYNQMP_GET_PIN(DT_PROP_BY_IDX(node_id, prop, idx)), \
				.func = ZYNQMP_GET_FUNC(DT_PROP_BY_IDX(node_id, prop, idx)), \
		},

#define Z_PINCTRL_STATE_PINS_INIT(node_id, prop) { \
				DT_FOREACH_CHILD_VARGS(DT_PHANDLE(node_id, prop), \
				DT_FOREACH_PROP_ELEM, pinmux, \
				Z_PINCTRL_STATE_PIN_INIT)}

#endif
