/*
 * Copyright (c) 2024 Telink Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef SOC_RISCV_TELINK_TLX_PINCTRL_SOC_H
#define SOC_RISCV_TELINK_TLX_PINCTRL_SOC_H

#include <stdint.h>
#include <zephyr/devicetree.h>
#if CONFIG_SOC_RISCV_TELINK_TL321X
#include <zephyr/dt-bindings/pinctrl/tl321x-pinctrl.h>
#elif CONFIG_SOC_RISCV_TELINK_TL721X
#include <zephyr/dt-bindings/pinctrl/tl721x-pinctrl.h>
#endif

/**
 * @brief Telink B9X-series pin type.
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
	 ((TLX_PULL_DOWN * DT_PROP(DT_PROP_BY_IDX(node_id, prop, idx), bias_pull_down))	\
		<< TLX_PULL_POS) |							\
	 ((TLX_PULL_UP * DT_PROP(DT_PROP_BY_IDX(node_id, prop, idx), bias_pull_up))	\
		<< TLX_PULL_POS)							\
	),

/**
 * @brief Utility macro to initialize state pins contained in a given property.
 *
 * @param node_id Node identifier.
 * @param prop Property name describing state pins.
 */
#define Z_PINCTRL_STATE_PINS_INIT(node_id, prop) \
	{ DT_FOREACH_PROP_ELEM(node_id, prop, Z_PINCTRL_STATE_PIN_INIT) }

#endif  /* SOC_RISCV_TELINK_TLX_PINCTRL_SOC_H */
