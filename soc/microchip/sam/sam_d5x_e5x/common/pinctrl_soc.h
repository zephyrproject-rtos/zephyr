/*
 * Copyright (c) 2025 Microchip Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef SOC_MICROCHIP_SAM_D5X_E5X_COMMON_PINCTRL_SOC_H_
#define SOC_MICROCHIP_SAM_D5X_E5X_COMMON_PINCTRL_SOC_H_

#include <zephyr/devicetree.h>
#include <zephyr/types.h>
#include "dt-bindings/sam/sam_d5x_e5x/common/mchp_pinctrl_pinmux_sam.h"

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
	uint16_t pinmux;
	/** Pin configuration (bias, drive etc). */
	uint16_t pinflag;
} pinctrl_soc_pin_t;

/**
 * @brief Utility macro to initialize pinmux field.
 *
 * @param node_id Node identifier.
 */
#define Z_PINCTRL_MCHP_PINMUX_INIT(node_id, prop, idx) DT_PROP_BY_IDX(node_id, prop, idx)

/**
 * @brief Utility macro to initialize each pin.
 *
 * @param node_id Node identifier.
 * @param prop Property name.
 * @param idx Property entry index.
 */
#define Z_PINCTRL_MCHP_PINFLAG_INIT(node_id, prop, idx)                                            \
	((DT_PROP(node_id, bias_pull_up) << MCHP_PINCTRL_PULLUP_POS) |                             \
	 (DT_PROP(node_id, bias_pull_down) << MCHP_PINCTRL_PULLDOWN_POS) |                         \
	 (DT_PROP(node_id, input_enable) << MCHP_PINCTRL_INPUTENABLE_POS) |                        \
	 (DT_PROP(node_id, output_enable) << MCHP_PINCTRL_OUTPUTENABLE_POS) |                      \
	 (DT_ENUM_IDX(node_id, drive_strength) << MCHP_PINCTRL_DRIVESTRENGTH_POS)),

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

/** @endcond */

/**
 * @brief Pin flags/attributes
 * @anchor MCHP_PINFLAGS
 *
 * @{
 */

#define MCHP_PINCTRL_FLAGS_DEFAULT     (0U)
#define MCHP_PINCTRL_FLAGS_POS         (0U)
#define MCHP_PINCTRL_FLAGS_MASK        (0x3F << MCHP_PINCTRL_FLAGS_POS)
#define MCHP_PINCTRL_FLAG_MASK         (1U)
#define MCHP_PINCTRL_PULLUP_POS        (MCHP_PINCTRL_FLAGS_POS)
#define MCHP_PINCTRL_PULLUP            (1U << MCHP_PINCTRL_PULLUP_POS)
#define MCHP_PINCTRL_PULLDOWN_POS      (MCHP_PINCTRL_PULLUP_POS + 1U)
#define MCHP_PINCTRL_PULLDOWN          (1U << MCHP_PINCTRL_PULLDOWN_POS)
#define MCHP_PINCTRL_OPENDRAIN_POS     (MCHP_PINCTRL_PULLDOWN_POS + 1U)
#define MCHP_PINCTRL_OPENDRAIN         (1U << MCHP_PINCTRL_OPENDRAIN_POS)
#define MCHP_PINCTRL_INPUTENABLE_POS   (MCHP_PINCTRL_OPENDRAIN_POS + 1U)
#define MCHP_PINCTRL_INPUTENABLE       (1U << MCHP_PINCTRL_INPUTENABLE_POS)
#define MCHP_PINCTRL_OUTPUTENABLE_POS  (MCHP_PINCTRL_INPUTENABLE_POS + 1U)
#define MCHP_PINCTRL_OUTPUTENABLE      (1U << MCHP_PINCTRL_OUTPUTENABLE_POS)
#define MCHP_PINCTRL_DRIVESTRENGTH_POS (MCHP_PINCTRL_OUTPUTENABLE_POS + 1U)
#define MCHP_PINCTRL_DRIVESTRENGTH     (1U << MCHP_PINCTRL_DRIVESTRENGTH_POS)

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* SOC_MICROCHIP_SAM_D5X_E5X_COMMON_PINCTRL_SOC_H_ */
