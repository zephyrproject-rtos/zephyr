/*
 * Copyright 2024-2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SOC_ARM_NXP_IMX_RT7XX_PINCTRL_SOC_H_
#define ZEPHYR_SOC_ARM_NXP_IMX_RT7XX_PINCTRL_SOC_H_

#include <zephyr/devicetree.h>
#include <zephyr/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @cond INTERNAL_HIDDEN */

typedef uint32_t pinctrl_soc_pin_t;

#define IOPCTL_PIO_PUPDENA_MASK (0x10U)
#define IOPCTL_PIO_PUPDSEL_MASK (0x20U)

#define IOPCTL_PIO_ODENA_MASK  (0x400U)
#define IOPCTL_PIO_ODENA_SHIFT (10U)
#define IOPCTL_PIO_ODENA(x)                                                                        \
	(((uint32_t)(((uint32_t)(x)) << IOPCTL_PIO_ODENA_SHIFT)) & IOPCTL_PIO_ODENA_MASK)

#define IOPCTL_PIO_IBENA_MASK  (0x40U)
#define IOPCTL_PIO_IBENA_SHIFT (6U)
#define IOPCTL_PIO_IBENA(x)                                                                        \
	(((uint32_t)(((uint32_t)(x)) << IOPCTL_PIO_IBENA_SHIFT)) & IOPCTL_PIO_IBENA_MASK)

/* Please note there is no SLEWRATE attribution on IOPCTL2 */
#define IOPCTL_PIO_SLEWRATE_MASK  (0x80U)
#define IOPCTL_PIO_SLEWRATE_SHIFT (7U)
#define IOPCTL_PIO_SLEWRATE(x)                                                                     \
	(((uint32_t)(((uint32_t)(x)) << IOPCTL_PIO_SLEWRATE_SHIFT)) & IOPCTL_PIO_SLEWRATE_MASK)

/* Please note there is no FULLDRIVE attribution on IOPCTL2 */
#define IOPCTL_PIO_FULLDRIVE_MASK  (0x100U)
#define IOPCTL_PIO_FULLDRIVE_SHIFT (8U)
#define IOPCTL_PIO_FULLDRIVE(x)                                                                    \
	(((uint32_t)(((uint32_t)(x)) << IOPCTL_PIO_FULLDRIVE_SHIFT)) & IOPCTL_PIO_FULLDRIVE_MASK)

#define IOPCTL_PIO_IIENA_MASK  (0x800U)
#define IOPCTL_PIO_IIENA_SHIFT (11U)
#define IOPCTL_PIO_IIENA(x)                                                                        \
	(((uint32_t)(((uint32_t)(x)) << IOPCTL_PIO_IIENA_SHIFT)) & IOPCTL1_PIO_IIENA_MASK)

/* Please note there is no AMENA attribution on IOPCTL2 */
#define IOPCTL_PIO_AMENA_MASK  (0x200U)
#define IOPCTL_PIO_AMENA_SHIFT (9U)
#define IOPCTL_PIO_AMENA(x)                                                                        \
	(((uint32_t)(((uint32_t)(x)) << IOPCTL_PIO_AMENA_SHIFT)) & IOPCTL_PIO_AMENA_MASK)

#define Z_PINCTRL_IOPCTL_PINCFG(node_id)                                                           \
	(IF_ENABLED(DT_PROP(node_id, bias_pull_down),				\
	(IOPCTL_PIO_PUPDENA_MASK |))                                    /* pull down */     \
		IF_ENABLED(DT_PROP(node_id, bias_pull_up),				\
	(IOPCTL_PIO_PUPDENA_MASK | IOPCTL_PIO_PUPDSEL_MASK |))    /* pull up */       \
			IOPCTL_PIO_ODENA(DT_PROP(node_id, drive_open_drain)) | /* open drain */    \
			IOPCTL_PIO_IBENA(DT_PROP(node_id, input_enable)) |     /* input buffer */  \
			IOPCTL_PIO_SLEWRATE(DT_ENUM_IDX(node_id, slew_rate)) | /* slew rate */     \
			IOPCTL_PIO_FULLDRIVE(                                                      \
				DT_ENUM_IDX(node_id, drive_strength)) |  /* drive strength */      \
			IOPCTL_PIO_IIENA(DT_PROP(node_id, nxp_invert)) | /* invert input */        \
			IOPCTL_PIO_AMENA(                                                          \
				DT_PROP(node_id, nxp_analog_mode))) /* analog multiplexor */

/* MCUX RT parts only have one pin type */
#define Z_PINCTRL_IOCON_D_PIN_MASK (0xFFF)
#define Z_PINCTRL_IOCON_A_PIN_MASK (0)
#define Z_PINCTRL_IOCON_I_PIN_MASK (0)

#define Z_PINCTRL_STATE_PIN_INIT(group, pin_prop, idx)                                             \
	DT_PROP_BY_IDX(group, pin_prop, idx) | Z_PINCTRL_IOPCTL_PINCFG(group),

#define Z_PINCTRL_STATE_PINS_INIT(node_id, prop)                                                   \
	{DT_FOREACH_CHILD_VARGS(DT_PHANDLE(node_id, prop), DT_FOREACH_PROP_ELEM, pinmux,           \
				Z_PINCTRL_STATE_PIN_INIT)}

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_SOC_ARM_NXP_IMX_RT7XX_PINCTRL_SOC_H_ */
