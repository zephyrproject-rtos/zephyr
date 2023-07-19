/*
 * Copyright (c) 2022 Telink Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef SOC_RISCV_TELINK_B91_PINCTRL_SOC_H
#define SOC_RISCV_TELINK_B91_PINCTRL_SOC_H

#include <stdint.h>
#include <zephyr/devicetree.h>
#include <zephyr/dt-bindings/pinctrl/b91-pinctrl.h>

/**
 * @brief Telink B91 pin type.
 */
typedef uint32_t pinctrl_soc_pin_t;

/**
 * @brief Utility macro to initialize each pin.
 *
 * @param node_id Node identifier.
 * @param prop Property name.
 * @param idx Property entry index.
 */
#define Z_PINCTRL_STATE_PIN_INIT(node_id, prop, idx)					\
	(DT_PROP(DT_PROP_BY_IDX(node_id, prop, idx), pinmux) |				\
	 ((B91_PULL_DOWN * DT_PROP(DT_PROP_BY_IDX(node_id, prop, idx), bias_pull_down))	\
		<< B91_PULL_POS) |							\
	 ((B91_PULL_UP * DT_PROP(DT_PROP_BY_IDX(node_id, prop, idx), bias_pull_up))	\
		<< B91_PULL_POS)							\
	),

/**
 * @brief Utility macro to initialize state pins contained in a given property.
 *
 * @param node_id Node identifier.
 * @param prop Property name describing state pins.
 */
#define Z_PINCTRL_STATE_PINS_INIT(node_id, prop) \
	{ DT_FOREACH_PROP_ELEM(node_id, prop, Z_PINCTRL_STATE_PIN_INIT) }

#endif  /* SOC_RISCV_TELINK_B91_PINCTRL_SOC_H */
