/*
 * Copyright (c) 2025 Conclusive Engineering Sp. z o.o.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef TI_SIMPLELINK_CC35XX_SOC_PINCTRL_H_
#define TI_SIMPLELINK_CC35XX_SOC_PINCTRL_H_

#include <zephyr/types.h>
#include <zephyr/sys/util.h>

typedef uint32_t pinctrl_soc_pin_t;

#define TI_CC35XX_OPEN_DRAIN BIT(4)
#define TI_CC35XX_PULL_UP    BIT(8)
#define TI_CC35XX_PULL_DOWN  BIT(9)

/**
 * @brief Utility macro to initialize each pin.
 *
 * @param node_id Node identifier.
 * @param prop Property name.
 * @param idx Property entry index.
 */
#define Z_PINCTRL_STATE_PIN_INIT(node_id, prop, idx)                                               \
	(DT_PROP_BY_IDX(node_id, prop, idx) |                                                      \
	 (TI_CC35XX_OPEN_DRAIN * DT_PROP(node_id, drive_open_drain)) |                             \
	 (TI_CC35XX_PULL_UP * DT_PROP(node_id, bias_pull_up)) |                                    \
	 (TI_CC35XX_PULL_DOWN * DT_PROP(node_id, bias_pull_down))),

/**
 * @brief Utility macro to initialize state pins contained in a given property.
 *
 * @param node_id Node identifier.
 * @param prop Property name describing state pins.
 */
#define Z_PINCTRL_STATE_PINS_INIT(node_id, prop)                                                   \
	{DT_FOREACH_CHILD_VARGS(DT_PHANDLE(node_id, prop), DT_FOREACH_PROP_ELEM, pinmux,           \
				Z_PINCTRL_STATE_PIN_INIT)}

#endif /* TI_SIMPLELINK_CC35XX_SOC_PINCTRL_H_ */
