/*
 * Copyright (c) 2025 Texas Instruments
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/devicetree.h>
#include <zephyr/types.h>

#ifndef ZEPHYR_SOC_ARM64_TI_AM26X_PINCTRL_SOC_H_
#define ZEPHYR_SOC_ARM64_TI_AM26X_PINCTRL_SOC_H_
/* Lock Vals and Unlock Vals*/

struct pinctrl_soc_pin {
	const struct device *dev;
	uint32_t offset;
	uint32_t value;
};

typedef struct pinctrl_soc_pin pinctrl_soc_pin_t;

#define TI_AM26X_DT_PIN(node_id)                                                                   \
	{.dev = DEVICE_DT_GET(DT_PARENT(node_id)),                                                 \
	 .offset = DT_PROP_BY_IDX(node_id, pinmux, 0),                                             \
	 .value = DT_PROP_BY_IDX(node_id, pinmux, 1)},

#define Z_PINCTRL_STATE_PIN_INIT(node_id, prop, idx)                                               \
	TI_AM26X_DT_PIN(DT_PROP_BY_IDX(node_id, prop, idx))

#define Z_PINCTRL_STATE_PINS_INIT(node_id, prop)                                                   \
	{DT_FOREACH_PROP_ELEM(node_id, prop, Z_PINCTRL_STATE_PIN_INIT)}

#endif /* ZEPHYR_SOC_ARM64_TI_AM26X_PINCTRL_SOC_H_ */
