/*
 * Copyright (c) 2025 Microchip Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef MICROCHIP_PIC32CXBZ_PINCTRL_SOC_H_
#define MICROCHIP_PIC32CXBZ_PINCTRL_SOC_H_

#include <zephyr/devicetree.h>
#include <zephyr/types.h>
#include <dt-bindings/pic32c/pic32cx_bz/common/mchp_pinctrl_pinmux_pic32cxbz.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @cond INTERNAL_HIDDEN */

/**
 * @brief Structure representing the pin control settings for a SOC pin.
 *
 * This structure is used to define the pinmux and pin configuration settings
 * for a specific pin on the SOC. It includes information about the port, pin,
 * function, bias, drive, and other configuration options.
 */
typedef struct pinctrl_soc_pin {
	/** Pinmux settings (port, pin and function). */
	uint32_t pinmux;
	/** Pin configuration (bias, drive etc). */
	uint32_t pinflag;
} pinctrl_soc_pin_t;

/**
 * @brief Utility macro to initialize pinmux field.
 *
 * @param node_id Node identifier.
 */
#define Z_PINCTRL_MCHP_PINMUX_INIT(node_id, prop, idx) DT_PROP_BY_IDX(node_id, prop, idx)

/**
 * @brief Utility macro to initialize pinflag field.
 *
 * @param node_id Node identifier.
 */
#define Z_PINCTRL_MCHP_PINFLAG_INIT(node_id, prop, idx)                                            \
	((DT_PROP(node_id, bias_pull_up) << MCHP_PINCTRL_PULLUP_POS) |                             \
	 (DT_PROP(node_id, bias_pull_down) << MCHP_PINCTRL_PULLDOWN_POS) |                         \
	 (DT_PROP(node_id, drive_open_drain) << MCHP_PINCTRL_OPENDRAIN_POS) |                      \
	 (DT_PROP(node_id, input_enable) << MCHP_PINCTRL_INPUTENABLE_POS) |                        \
	 (DT_PROP(node_id, output_enable) << MCHP_PINCTRL_OUTPUTENABLE_POS)),

/**
 * @brief Utility macro to initialize each pin.
 *
 * @param node_id Node identifier.
 * @param prop Property name.
 * @param idx Property entry index.
 */
#define Z_PINCTRL_STATE_PIN_INIT(node_id, prop, idx)                                               \
	{.pinmux = Z_PINCTRL_MCHP_PINMUX_INIT(node_id, prop, idx),                                 \
	 .pinflag = Z_PINCTRL_MCHP_PINFLAG_INIT(node_id, prop, idx)},

/**
 * @brief Utility macro to initialize state pins contained in a given property.
 *
 * @param node_id Node identifier.
 * @param prop Property name describing state pins.
 */
#define Z_PINCTRL_STATE_PINS_INIT(node_id, prop)                                                   \
	{DT_FOREACH_CHILD_VARGS(DT_PHANDLE(node_id, prop), DT_FOREACH_PROP_ELEM, pinmux,           \
				Z_PINCTRL_STATE_PIN_INIT)}

/**
 * @brief Pin flags/attributes
 * @anchor MCHP_PINFLAGS
 *
 * @{
 */

#define MCHP_PINCTRL_FLAGS_DEFAULT    (0U)
#define MCHP_PINCTRL_FLAGS_POS        (0U)
#define MCHP_PINCTRL_FLAGS_MASK       (0xF << MCHP_PINCTRL_FLAGS_POS)
#define MCHP_PINCTRL_FLAG_MASK        (1U)
#define MCHP_PINCTRL_PULLUP_POS       (MCHP_PINCTRL_FLAGS_POS)
#define MCHP_PINCTRL_PULLUP           (1U << MCHP_PINCTRL_PULLUP_POS)
#define MCHP_PINCTRL_PULLDOWN_POS     (1)
#define MCHP_PINCTRL_PULLDOWN         (1U << MCHP_PINCTRL_PULLDOWN_POS)
#define MCHP_PINCTRL_OPENDRAIN_POS    (2)
#define MCHP_PINCTRL_OPENDRAIN        (1U << MCHP_PINCTRL_OPENDRAIN_POS)
#define MCHP_PINCTRL_INPUTENABLE_POS  (3)
#define MCHP_PINCTRL_INPUTENABLE      (1U << MCHP_PINCTRL_INPUTENABLE_POS)
#define MCHP_PINCTRL_OUTPUTENABLE_POS (4)
#define MCHP_PINCTRL_OUTPUTENABLE     (1U << MCHP_PINCTRL_OUTPUTENABLE_POS)

/** @endcond */

#ifdef __cplusplus
}
#endif

#endif /* MICROCHIP_PIC32CXBZ_PINCTRL_SOC_H_ */
