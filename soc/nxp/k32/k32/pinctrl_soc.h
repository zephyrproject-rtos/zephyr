/*
 * Copyright (c) 2023 Sendrato
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SOC_ARM_NXP_K32_PINCTRL_SOC_H_
#define ZEPHYR_SOC_ARM_NXP_K32_PINCTRL_SOC_H_

#include <zephyr/devicetree.h>
#include <zephyr/types.h>
#include <QN9090.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @cond INTERNAL_HIDDEN */

typedef uint32_t pinctrl_soc_pin_t;

#define Z_PINCTRL_IOCON_PINCFG(node_id)                                                            \
	(IF_ENABLED(DT_PROP(node_id, bias_pull_up), (IOCON_PIO_MODE(0x0) |)) IF_ENABLED(           \
		 DT_PROP(node_id, nxp_repeater_mode), (IOCON_PIO_MODE(0x1) |))                     \
		 IF_ENABLED(DT_PROP(node_id, nxp_plain_mode), (IOCON_PIO_MODE(0x2) |))             \
			 IF_ENABLED(DT_PROP(node_id, bias_pull_down), (IOCON_PIO_MODE(0x3) |))     \
				 IF_ENABLED(DT_PROP(node_id, nxp_i2c_pin),                         \
					    (IF_DISABLED(DT_PROP(node_id, nxp_i2c_mode),           \
							 (IOCON_PIO_EGP(0x1) |))))                 \
					 IOCON_PIO_SLEW0(DT_ENUM_IDX(node_id, slew_rate)) |        \
	 IOCON_PIO_INVERT(DT_PROP(node_id, nxp_invert)) |                                          \
	 IOCON_PIO_DIGIMODE(!DT_PROP(node_id, nxp_analog_mode)) |                                  \
	 IOCON_PIO_FILTEROFF(!DT_NODE_HAS_PROP(node_id, nxp_i2c_filter)) |                         \
	 IOCON_PIO_OD(DT_PROP(node_id, drive_open_drain)) |                                        \
	 IOCON_PIO_FSEL(DT_NODE_HAS_PROP(node_id, nxp_i2c_filter)))

/* Mask for standard IO pins configuration register */
#define Z_PINCTRL_IOCON_D_PIN_MASK                                                                 \
	(IOCON_PIO_FUNC_MASK | IOCON_PIO_MODE_MASK | IOCON_PIO_SLEW0_MASK |                        \
	 IOCON_PIO_INVERT_MASK | IOCON_PIO_DIGIMODE_MASK | IOCON_PIO_FILTEROFF_MASK |              \
	 IOCON_PIO_SLEW1_MASK | IOCON_PIO_OD_MASK | IOCON_PIO_IO_CLAMP_MASK)

/* Mask for special i2c pins (pin 10 and 11) configuration register */
#define Z_PINCTRL_IOCON_I_PIN_MASK                                                                 \
	(IOCON_PIO_FUNC_MASK | IOCON_PIO_EGP_MASK | IOCON_PIO_ECS_MASK | IOCON_PIO_EHS_MASK |      \
	 IOCON_PIO_INVERT_MASK | IOCON_PIO_DIGIMODE_MASK | IOCON_PIO_FILTEROFF_MASK |              \
	 IOCON_PIO_FSEL_MASK | IOCON_PIO_OD_MASK | IOCON_PIO_SSEL_MASK | IOCON_PIO_IO_CLAMP_MASK)

#define Z_PINCTRL_STATE_PIN_INIT(group, pin_prop, idx)                                             \
	DT_PROP_BY_IDX(group, pin_prop, idx) | Z_PINCTRL_IOCON_PINCFG(group),

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

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_SOC_ARM_NXP_K32_PINCTRL_SOC_H_ */
