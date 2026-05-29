/*
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SOC_ARM_NXP_IMX_RT6XX_PINCTRL_SOC_H_
#define ZEPHYR_SOC_ARM_NXP_IMX_RT6XX_PINCTRL_SOC_H_

#include <zephyr/devicetree.h>
#include <zephyr/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @cond INTERNAL_HIDDEN */

typedef uint32_t pinctrl_soc_pin_t;


#define Z_PINCTRL_IOPCTL_PINCFG(node_id)						\
	(IF_ENABLED(DT_PROP(node_id, bias_pull_down),				\
	(IOPCTL_PIO_PUPDENA_MASK |)) /* pull down */				\
	IF_ENABLED(DT_PROP(node_id, bias_pull_up),				\
	(IOPCTL_PIO_PUPDENA_MASK | IOPCTL_PIO_PUPDSEL_MASK |)) /* pull up */	\
	IOPCTL_PIO_ODENA(DT_PROP(node_id, drive_open_drain)) | /* open drain */	\
	IOPCTL_PIO_IBENA(DT_PROP(node_id, input_enable)) | /* input buffer */	\
	IOPCTL_PIO_SLEWRATE(DT_ENUM_IDX(node_id, slew_rate)) | /* slew rate */	\
	IOPCTL_PIO_FULLDRIVE(DT_ENUM_IDX(node_id, drive_strength)) | /* drive strength */ \
	IOPCTL_PIO_IIENA(DT_PROP(node_id, nxp_invert)) | /* invert input */	\
	IOPCTL_PIO_AMENA(DT_PROP(node_id, nxp_analog_mode))) /* analog multiplexor */

/* MCUX RT parts only have one pin type */
#define Z_PINCTRL_IOCON_D_PIN_MASK (0xFFF)
#define Z_PINCTRL_IOCON_A_PIN_MASK (0)
#define Z_PINCTRL_IOCON_I_PIN_MASK (0)

#define Z_PINCTRL_STATE_PIN_INIT(group, pin_prop, idx)		\
	DT_PROP_BY_IDX(group, pin_prop, idx) | Z_PINCTRL_IOPCTL_PINCFG(group),


#define Z_PINCTRL_STATE_PINS_INIT(node_id, prop)			\
	{DT_FOREACH_CHILD_VARGS(DT_PHANDLE(node_id, prop),		\
		DT_FOREACH_PROP_ELEM, pinmux, Z_PINCTRL_STATE_PIN_INIT)}


/*!<@brief Analog mux is disabled */
#define IOPCTL_PIO_ANAMUX_DI 0x00u
/*!<@brief Analog mux is enabled */
#define IOPCTL_PIO_ANAMUX_EN 0x0200u
/*!<@brief Normal drive */
#define IOPCTL_PIO_FULLDRIVE_DI 0x00u
/*!<@brief Full drive */
#define IOPCTL_PIO_FULLDRIVE_EN 0x0100u
/*!<@brief Selects pin function 0 */
#define IOPCTL_PIO_FUNC0 0x00u
/*!<@brief Selects pin function 1 */
#define IOPCTL_PIO_FUNC1 0x01u
/*!<@brief Selects pin function 2 */
#define IOPCTL_PIO_FUNC2 0x02u
/*!<@brief Selects pin function 3 */
#define IOPCTL_PIO_FUNC3 0x03u
/*!<@brief Selects pin function 4 */
#define IOPCTL_PIO_FUNC4 0x04u
/*!<@brief Selects pin function 5 */
#define IOPCTL_PIO_FUNC5 0x05u
/*!<@brief Selects pin function 6 */
#define IOPCTL_PIO_FUNC6 0x06u
/*!<@brief Selects pin function 7 */
#define IOPCTL_PIO_FUNC7 0x07u
/*!<@brief Selects pin function 8 */
#define IOPCTL_PIO_FUNC8 0x08u
/*!<@brief Disable input buffer function */
#define IOPCTL_PIO_INBUF_DI 0x00u
/*!<@brief Enables input buffer function */
#define IOPCTL_PIO_INBUF_EN 0x40u
/*!<@brief Input function is not inverted */
#define IOPCTL_PIO_INV_DI 0x00u
/*!<@brief Input function is inverted */
#define IOPCTL_PIO_INV_EN 0x0800u
/*!<@brief Pseudo Output Drain is disabled */
#define IOPCTL_PIO_PSEDRAIN_DI 0x00u
/*!<@brief Pseudo Output Drain is enabled */
#define IOPCTL_PIO_PSEDRAIN_EN 0x0400u
/*!<@brief Enable pull-down function */
#define IOPCTL_PIO_PULLDOWN_EN 0x00u
/*!<@brief Enable pull-up function */
#define IOPCTL_PIO_PULLUP_EN 0x20u
/*!<@brief Disable pull-up / pull-down function */
#define IOPCTL_PIO_PUPD_DI 0x00u
/*!<@brief Enable pull-up / pull-down function */
#define IOPCTL_PIO_PUPD_EN 0x10u
/*!<@brief Normal mode */
#define IOPCTL_PIO_SLEW_RATE_NORMAL 0x00u
/*!<@brief Slow mode */
#define IOPCTL_PIO_SLEW_RATE_SLOW 0x80u

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_SOC_ARM_NXP_IMX_RT6XX_PINCTRL_SOC_H_ */
