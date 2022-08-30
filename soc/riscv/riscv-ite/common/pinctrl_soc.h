/*
 * Copyright (c) 2022 ITE Technology Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SOC_RISCV_ITE_IT8XXX2_COMMON_PINCTRL_SOC_H_
#define ZEPHYR_SOC_RISCV_ITE_IT8XXX2_COMMON_PINCTRL_SOC_H_

#include <zephyr/devicetree.h>
#include <zephyr/dt-bindings/pinctrl/it8xxx2-pinctrl.h>
#include <zephyr/types.h>

/**
 * @brief ITE IT8XXX2 pin type.
 */
struct pinctrl_soc_pin {
	/* Pinmux control group */
	const struct device *pinctrls;
	/* Pin configuration (impedance, pullup/down, voltate selection, input). */
	uint32_t pincfg;
	/* GPIO pin */
	uint8_t pin;
	/* Alternate function */
	uint8_t alt_func;
};

typedef struct pinctrl_soc_pin pinctrl_soc_pin_t;

/**
 * @brief PIN configuration bitfield.
 *
 * Pin configuration is coded with the following
 * fields.
 *    Pin impedance config     [ 0 ]
 *    Pin pull-up/down config  [ 4 : 5 ]
 *    Pin voltage selection    [ 8 ]
 *    Pin input enable config  [ 12 ]
 */
#define IT8XXX2_HIGH_IMPEDANCE     0x1U
#define IT8XXX2_PULL_PIN_DEFAULT   0x0U
#define IT8XXX2_PULL_UP            0x1U
#define IT8XXX2_PULL_DOWN          0x2U
#define IT8XXX2_VOLTAGE_3V3        0x0U
#define IT8XXX2_VOLTAGE_1V8        0x1U
#define IT8XXX2_INPUT_ENABLE       0x1U

/* Pin tri-state mode. */
#define IT8XXX2_IMPEDANCE_SHIFT    0U
#define IT8XXX2_IMPEDANCE_MASK     0x1U
/* Pin pull-up or pull-down */
#define IT8XXX2_PUPDR_SHIFT        4U
#define IT8XXX2_PUPDR_MASK         0x3U
/* Pin 3.3V or 1.8V */
#define IT8XXX2_VOLTAGE_SHIFT      8U
#define IT8XXX2_VOLTAGE_MASK       0x1U
/* Pin INPUT enable or disable */
#define IT8XXX2_INPUT_SHIFT        12U
#define IT8XXX2_INPUT_MASK         0x1U

/**
 * @brief Utility macro to obtain configuration of tri-state.
 */
#define IT8XXX2_DT_PINCFG_IMPEDANCE(__mode) \
	(((__mode) >> IT8XXX2_IMPEDANCE_SHIFT) & IT8XXX2_IMPEDANCE_MASK)

/**
 * @brief Utility macro to obtain configuration of pull-up or pull-down.
 */
#define IT8XXX2_DT_PINCFG_PUPDR(__mode)     \
	(((__mode) >> IT8XXX2_PUPDR_SHIFT) & IT8XXX2_PUPDR_MASK)

/**
 * @brief Utility macro to obtain input voltage selection.
 */
#define IT8XXX2_DT_PINCFG_VOLTAGE(__mode)   \
	(((__mode) >> IT8XXX2_VOLTAGE_SHIFT) & IT8XXX2_VOLTAGE_MASK)

/**
 * @brief Utility macro to obtain input enable.
 */
#define IT8XXX2_DT_PINCFG_INPUT(__mode)     \
	(((__mode) >> IT8XXX2_INPUT_SHIFT) & IT8XXX2_INPUT_MASK)

/**
 * @brief Utility macro to initialize pincfg field in #pinctrl_pin_t.
 *
 * @param node_id Node identifier.
 */
#define Z_PINCTRL_IT8XXX2_PINCFG_INIT(node_id)                                 \
	(((IT8XXX2_HIGH_IMPEDANCE * DT_PROP(node_id, bias_high_impedance))     \
	 << IT8XXX2_IMPEDANCE_SHIFT) |                                         \
	 ((IT8XXX2_PULL_PIN_DEFAULT * DT_PROP(node_id, bias_pull_pin_default)) \
	 << IT8XXX2_PUPDR_SHIFT) |                                             \
	 ((IT8XXX2_PULL_UP * DT_PROP(node_id, bias_pull_up))                   \
	 << IT8XXX2_PUPDR_SHIFT) |                                             \
	 ((IT8XXX2_PULL_DOWN * DT_PROP(node_id, bias_pull_down))               \
	 << IT8XXX2_PUPDR_SHIFT) |                                             \
	 ((IT8XXX2_VOLTAGE_1V8 * DT_ENUM_IDX(node_id, gpio_voltage))           \
	 << IT8XXX2_VOLTAGE_SHIFT) |                                           \
	 ((IT8XXX2_INPUT_ENABLE * DT_PROP(node_id, input_enable))              \
	 << IT8XXX2_INPUT_SHIFT))

/**
 * @brief Utility macro to initialize pinctrls of pinmuxs field in #pinctrl_pin_t.
 *
 * @param node_id Node identifier.
 */
#define Z_PINCTRL_IT8XXX2_PINCTRL_INIT(node_id)      \
	DEVICE_DT_GET(DT_PHANDLE(node_id, pinmuxs))
/**
 * @brief Utility macro to initialize pin of pinmuxs field in #pinctrl_pin_t.
 *
 * @param node_id Node identifier.
 */
#define Z_PINCTRL_IT8XXX2_PIN_INIT(node_id)          \
	DT_PHA(node_id, pinmuxs, pin)
/**
 * @brief Utility macro to initialize alt_func of pinmuxs field in #pinctrl_pin_t.
 *
 * @param node_id Node identifier.
 */
#define Z_PINCTRL_IT8XXX2_ALT_INIT(node_id)          \
	DT_PHA(node_id, pinmuxs, alt_func)

/**
 * @brief Utility macro to initialize each pin.
 *
 * @param node_id Node identifier.
 * @param prop Property name.
 * @param idx Property entry index.
 */
#define Z_PINCTRL_STATE_PIN_INIT(node_id, prop, idx)          \
	{ .pinctrls = Z_PINCTRL_IT8XXX2_PINCTRL_INIT(         \
		DT_PROP_BY_IDX(node_id, prop, idx)),          \
	  .pincfg = Z_PINCTRL_IT8XXX2_PINCFG_INIT(            \
		DT_PROP_BY_IDX(node_id, prop, idx)),          \
	  .pin = Z_PINCTRL_IT8XXX2_PIN_INIT(                  \
		DT_PROP_BY_IDX(node_id, prop, idx)),          \
	  .alt_func = Z_PINCTRL_IT8XXX2_ALT_INIT(             \
		DT_PROP_BY_IDX(node_id, prop, idx)), },

/**
 * @brief Utility macro to initialize state pins contained in a given property.
 *
 * @param node_id Node identifier.
 * @param prop Property name describing state pins.
 */
#define Z_PINCTRL_STATE_PINS_INIT(node_id, prop)              \
	{DT_FOREACH_PROP_ELEM(node_id, prop, Z_PINCTRL_STATE_PIN_INIT)}

#endif /* ZEPHYR_SOC_RISCV_ITE_IT8XXX2_COMMON_PINCTRL_SOC_H_ */
