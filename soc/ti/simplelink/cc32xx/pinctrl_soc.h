/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SOC_ARM_TI_SIMPLELINK_CC32XX_PINCTRL_SOC_H_
#define ZEPHYR_SOC_ARM_TI_SIMPLELINK_CC32XX_PINCTRL_SOC_H_

#include <stdint.h>

#include <zephyr/devicetree.h>
#include <zephyr/sys/util.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @cond INTERNAL_HIDDEN */

/** Type for TI CC32XX pin. */
typedef uint32_t pinctrl_soc_pin_t;

/**
 * @name TI CC32XX pin configuration bit field positions and masks.
 */

#define TI_CC32XX_OPEN_DRAIN           BIT(4)
#define TI_CC32XX_DRIVE_STRENGTH_MSK   0x7U
#define TI_CC32XX_DRIVE_STRENGTH_POS   5U
#define TI_CC32XX_PULL_UP              BIT(8)
#define TI_CC32XX_PULL_DOWN            BIT(9)
#define TI_CC32XX_PAD_OUT_OVERRIDE     BIT(10)
#define TI_CC32XX_PAD_OUT_BUF_OVERRIDE BIT(11)

/** @} */

/**
 * @brief Utility macro to initialize each pin.
 *
 * @param node_id Node identifier.
 * @param prop Property name.
 * @param idx Property entry index.
 */
#define Z_PINCTRL_STATE_PIN_INIT(node_id, prop, idx)                                               \
	(DT_PROP_BY_IDX(node_id, prop, idx) |                                                      \
	 (TI_CC32XX_OPEN_DRAIN * DT_PROP(node_id, drive_open_drain)) |                             \
	 (TI_CC32XX_PULL_UP * DT_PROP(node_id, bias_pull_down)) |                                  \
	 (TI_CC32XX_PULL_DOWN * DT_PROP(node_id, bias_pull_up)) |                                  \
	 ((DT_ENUM_IDX(node_id, drive_strength) & TI_CC32XX_DRIVE_STRENGTH_MSK)                    \
	  << TI_CC32XX_DRIVE_STRENGTH_POS) |                                                       \
	 TI_CC32XX_PAD_OUT_OVERRIDE | TI_CC32XX_PAD_OUT_BUF_OVERRIDE),

/**
 * @brief Utility macro to initialize state pins contained in a given property.
 *
 * @param node_id Node identifier.
 * @param prop Property name describing state pins.
 */
#define Z_PINCTRL_STATE_PINS_INIT(node_id, prop)                                                   \
	{                                                                                          \
		DT_FOREACH_CHILD_VARGS(DT_PHANDLE(node_id, prop), DT_FOREACH_PROP_ELEM, pinmux,    \
				       Z_PINCTRL_STATE_PIN_INIT)                                   \
	}

/** @endcond */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_SOC_ARM_TI_SIMPLELINK_CC32XX_PINCTRL_SOC_H_ */
