/*
 * Copyright (c) 2016-2017 Piotr Mienkowski
 * Copyright (c) 2021 ATL Electronics
 * Copyright (c) 2022 Cypress Semiconductor Corporation (an Infineon company) or
 * an affiliate of Cypress Semiconductor Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

/**
 * @brief Infineon CAT1 SoC specific helpers for pinctrl driver.
 */

#ifndef ZEPHYR_SOC_ARM_INFINEON_CAT1_COMMON_PINCTRL_SOC_H_
#define ZEPHYR_SOC_ARM_INFINEON_CAT1_COMMON_PINCTRL_SOC_H_

#include <stdint.h>
#include <zephyr/devicetree.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @cond INTERNAL_HIDDEN */

/**
 * Bit definition in PINMUX field
 */
#define SOC_PINMUX_PORT_POS             (0)
#define SOC_PINMUX_PORT_MASK            (0xFFul << SOC_PINMUX_PORT_POS)
#define SOC_PINMUX_PIN_POS              (8)
#define SOC_PINMUX_PIN_MASK             (0xFFul << SOC_PINMUX_PIN_POS)
#define SOC_PINMUX_HSIOM_FUNC_POS       (16)
#define SOC_PINMUX_HSIOM_MASK           (0xFFul << SOC_PINMUX_HSIOM_FUNC_POS)
#define SOC_PINMUX_SIGNAL_POS           (24)
#define SOC_PINMUX_SIGNAL_MASK          (0xFFul << SOC_PINMUX_SIGNAL_POS)

/*
 * Pin flags/attributes
 */
#define SOC_GPIO_DEFAULT                (0)
#define SOC_GPIO_FLAGS_POS              (0)
#define SOC_GPIO_FLAGS_MASK             (0x3F << SOC_GPIO_FLAGS_POS)
#define SOC_GPIO_PULLUP_POS             (0)
#define SOC_GPIO_PULLUP                 (1 << SOC_GPIO_PULLUP_POS)
#define SOC_GPIO_PULLDOWN_POS           (1)
#define SOC_GPIO_PULLDOWN               (1 << SOC_GPIO_PULLDOWN_POS)
#define SOC_GPIO_OPENDRAIN_POS          (2)
#define SOC_GPIO_OPENDRAIN              (1 << SOC_GPIO_OPENDRAIN_POS)
#define SOC_GPIO_OPENSOURCE_POS         (3)
#define SOC_GPIO_OPENSOURCE             (1 << SOC_GPIO_OPENSOURCE_POS)

/* Push-Pull means Strong, see dts/pinctrl/pincfg-node.yaml */
#define SOC_GPIO_PUSHPULL_POS           (4)
#define SOC_GPIO_PUSHPULL               (1 << SOC_GPIO_PUSHPULL_POS)

/* Input-Enable means Input-Buffer, see dts/pinctrl/pincfg-node.yaml */
#define SOC_GPIO_INPUTENABLE_POS        (5)
#define SOC_GPIO_INPUTENABLE            (1 << SOC_GPIO_INPUTENABLE_POS)

#define SOC_GPIO_HIGHZ_POS              (6)
#define SOC_GPIO_HIGHZ                  (1 << SOC_GPIO_HIGHZ_POS)

/** Type for CAT1 Soc pin. */
typedef struct {
	/**
	 * Pinmux settings (port, pin and function).
	 * [0..7]  - Port nunder
	 * [8..15] - Pin number
	 * [16..23]- HSIOM function
	 */
	uint32_t pinmux;

	/** Pin configuration (bias, drive and slew rate). */
	uint32_t pincfg;
} pinctrl_soc_pin_t;

#define CAT1_PINMUX_GET_PORT_NUM(pinmux) \
	(((pinmux) & SOC_PINMUX_PORT_MASK) >> SOC_PINMUX_PORT_POS)
#define CAT1_PINMUX_GET_PIN_NUM(pinmux)	\
	(((pinmux) & SOC_PINMUX_PIN_MASK) >> SOC_PINMUX_PIN_POS)
#define CAT1_PINMUX_GET_HSIOM_FUNC(pinmux) \
	(((pinmux) & SOC_PINMUX_HSIOM_MASK) >> SOC_PINMUX_HSIOM_FUNC_POS)

/**
 * @brief Utility macro to initialize pinmux field in #pinctrl_pin_t.
 * @param node_id Node identifier.
 */
#define Z_PINCTRL_CAT1_PINMUX_INIT(node_id) DT_PROP(node_id, pinmux)

/**
 * @brief Utility macro to initialize pincfg field in #pinctrl_pin_t.
 * @param node_id Node identifier.
 */
#define Z_PINCTRL_CAT1_PINCFG_INIT(node_id) (					   \
		(DT_PROP(node_id, bias_pull_up) << SOC_GPIO_PULLUP_POS) |	   \
		(DT_PROP(node_id, bias_pull_down) << SOC_GPIO_PULLDOWN_POS) |	   \
		(DT_PROP(node_id, drive_open_drain) << SOC_GPIO_OPENDRAIN_POS) |   \
		(DT_PROP(node_id, drive_open_source) << SOC_GPIO_OPENSOURCE_POS) | \
		(DT_PROP(node_id, drive_push_pull) << SOC_GPIO_PUSHPULL_POS) |	   \
		(DT_PROP(node_id, input_enable) << SOC_GPIO_INPUTENABLE_POS) | \
		(DT_PROP(node_id, bias_high_impedance) << SOC_GPIO_HIGHZ_POS))

/**
 * @brief Utility macro to initialize each pin.
 *
 * @param node_id Node identifier.
 * @param state_prop State property name.
 * @param idx State property entry index.
 */
#define Z_PINCTRL_STATE_PIN_INIT(node_id, state_prop, idx)   \
	{ .pinmux = Z_PINCTRL_CAT1_PINMUX_INIT(		     \
		  DT_PROP_BY_IDX(node_id, state_prop, idx)), \
	  .pincfg = Z_PINCTRL_CAT1_PINCFG_INIT(		     \
		  DT_PROP_BY_IDX(node_id, state_prop, idx)) },

/**
 * @brief Utility macro to initialize state pins contained in a given property.
 *
 * @param node_id Node identifier.
 * @param prop Property name describing state pins.
 */
#define Z_PINCTRL_STATE_PINS_INIT(node_id, prop) \
	{ DT_FOREACH_PROP_ELEM(node_id, prop, Z_PINCTRL_STATE_PIN_INIT) }

/** @endcond */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_SOC_ARM_INFINEON_CAT1_COMMON_PINCTRL_SOC_H_ */
