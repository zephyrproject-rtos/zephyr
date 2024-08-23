/*
 * Copyright (c) 2023-2024 MUNIC SA
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */
#ifndef __ZEPHYR_SOC_ARM_RENESAS_RA_PINCTRL_SOC_H__
#define __ZEPHYR_SOC_ARM_RENESAS_RA_PINCTRL_SOC_H__

#include <zephyr/devicetree.h>
#include <zephyr/sys/util_macro.h>
#include <zephyr/dt-bindings/pinctrl/renesas/pinctrl-ra.h>

#define RA_PIN_FLAGS_PODR	BIT(0)
#define RA_PIN_FLAGS_PIDR	BIT(1)
#define RA_PIN_FLAGS_PDR	BIT(2)
#define RA_PIN_FLAGS_PCR	BIT(4)
#define RA_PIN_FLAGS_NCODR	BIT(6)
#define RA_PIN_FLAGS_EOFR(e)	(((e) & GENMASK(1, 0)) << 12)
#define RA_PIN_FLAGS_ISEL	BIT(14)
#define RA_PIN_FLAGS_ASEL	BIT(15)
#define RA_PIN_FLAGS_PMR	BIT(16)
#define RA_PIN_FLAGS_PSEL(f)	(((f) & GENMASK(4, 0)) << 24)

#define RA_PIN_FLAGS_MASK	(RA_PIN_FLAGS_PODR	|		       \
				 RA_PIN_FLAGS_PIDR	|		       \
				 RA_PIN_FLAGS_PDR	|		       \
				 RA_PIN_FLAGS_PCR	|		       \
				 RA_PIN_FLAGS_NCODR	|		       \
				 RA_PIN_FLAGS_EOFR((uint32_t)-1) |	       \
				 RA_PIN_FLAGS_ISEL	|		       \
				 RA_PIN_FLAGS_ASEL	|		       \
				 RA_PIN_FLAGS_PMR	|		       \
				 RA_PIN_FLAGS_PSEL((uint32_t)-1))

/** Additional info, must be cleaned up before writing to the register.
 *
 * In the most fully featured RA8, only 7 consecutive unused bits are available,
 * starting from bit 17. So, pin numbers will use the 4 bits from 17 to 20.
 * Following 3, starting from 21, used for the lower part of the port number,
 * and for the higher part it was necessary to use reserved bits
 * starting from 29.
 */
#define RA_PIN_FLAGS_PIN(id)	(((id) & GENMASK(3, 0)) << 17)
#define RA_PIN_GET_PIN(pinctrl)	 (((pinctrl)>>17) & GENMASK(3, 0))

/* Ditto. */
#define RA_PIN_FLAGS_PORT(id)	((((id) & BIT(3))<<(29-3)) | \
				 (((id) & GENMASK(2, 0)) << 21))
#define RA_PIN_GET_PORT(pinctrl) (((pinctrl) >> 21) & GENMASK(2, 0)) | \
				  (((pinctrl) >> (29-3)) & BIT(3))

/** Type for RA2 pin. */
typedef uint32_t pinctrl_soc_pin_t;

/**
 * @brief Utility macro to initialize each pin.
 *
 * @param node_id Node identifier.
 * @param state_prop State property name.
 * @param idx State property entry index.
 */
#define Z_PINCTRL_STATE_PIN_INIT(node_id, state_prop, idx)	(	       \
		RA_PIN_FLAGS_PODR  * DT_PROP(node_id, output_high)	     | \
		RA_PIN_FLAGS_PDR   * DT_PROP(node_id, output_enable)	     | \
		RA_PIN_FLAGS_PCR   * DT_PROP(node_id, bias_pull_up)	     | \
		RA_PIN_FLAGS_NCODR * DT_PROP(node_id, drive_open_drain)	     | \
		RA_PIN_FLAGS_ISEL * DT_PROP(node_id, irq_enable)	     | \
		RA_PIN_FLAGS_ASEL * DT_PROP(node_id, analog_enable)	     | \
		RA_PIN_FLAGS_PMR *					       \
				(!!DT_PHA_BY_IDX(node_id, pinmux, idx, fn))  | \
		RA_PIN_FLAGS_PSEL(DT_PHA_BY_IDX(node_id, pinmux, idx, fn))   | \
		RA_PIN_FLAGS_PIN(DT_PHA_BY_IDX(node_id, pinmux, idx, pin))   | \
		RA_PIN_FLAGS_PORT(DT_NODE_CHILD_IDX(			       \
				   DT_PHANDLE_BY_IDX(node_id, pinmux, idx)))   \
	),

/**
 * @brief Utility macro to initialize state pins contained in a given property.
 *
 * @param node_id Node identifier.
 * @param prop Property name describing state pins.
 */
#define Z_PINCTRL_STATE_PINS_INIT(node_id, prop)	{		       \
	DT_FOREACH_CHILD_VARGS(DT_PHANDLE(node_id, prop),		       \
				DT_FOREACH_PROP_ELEM, pinmux,		       \
				Z_PINCTRL_STATE_PIN_INIT) }

#endif /* __ZEPHYR_SOC_ARM_RENESAS_RA_PINCTRL_SOC_H__ */
