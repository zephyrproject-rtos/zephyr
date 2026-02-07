/*
 * Copyright (c) 2025 Qingdao IotPi Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SOC_ARM_SIFLI_SF32LB58_PINCTRL_SF32LB58_H_
#define ZEPHYR_SOC_ARM_SIFLI_SF32LB58_PINCTRL_SF32LB58_H_

#include <zephyr/sys/util.h>
#include <zephyr/devicetree.h>
#include <zephyr/types.h>

#define SF32LB58_PINCTRL_BIAS_DISABLE_MASK GENMASK(4, 4)
#define SF32LB58_PINCTRL_SEL_MASK          GENMASK(3, 0)

struct pinctrl_soc_pinmux {
	uint32_t reg;
	uint8_t sel;
};

struct pinctrl_soc_pin {
	struct pinctrl_soc_pinmux pinmux;
	uint32_t flags;
};

typedef struct pinctrl_soc_pin pinctrl_soc_pin_t;

#define SF32LB58_PINMUX(node_id)                                                                   \
	{                                                                                          \
		.reg = DT_PROP_BY_IDX(node_id, pinmux, 0),                                         \
		.sel = DT_PROP_BY_IDX(node_id, pinmux, 1),                                         \
	}

#define PINCTRL_PINMUX(group_id, prop, idx)

#define PINCTRL_STATE_PIN_INIT(group_id, prop, idx)                                                \
	{                                                                                          \
		.pinmux = SF32LB58_PINMUX(DT_PHANDLE_BY_IDX(group_id, prop, idx)),                 \
		.flags = FIELD_PREP(SF32LB58_PINCTRL_BIAS_DISABLE_MASK,                            \
				    DT_PROP(group_id, bias_disable)),                              \
	},

#define Z_PINCTRL_STATE_PINS_INIT(node_id, prop)                                                   \
	{DT_FOREACH_CHILD_VARGS(DT_PHANDLE(node_id, prop), DT_FOREACH_PROP_ELEM, pinmux,           \
				PINCTRL_STATE_PIN_INIT)};

#endif /* ZEPHYR_SOC_ARM_SIFLI_SF32LB58_PINCTRL_SF32LB58_H_ */
