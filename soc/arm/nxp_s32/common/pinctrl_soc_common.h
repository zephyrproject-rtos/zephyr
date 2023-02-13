/*
 * Copyright 2022 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SOC_ARM_NXP_S32_COMMON_PINCTRL_SOC_COMMON_H_
#define ZEPHYR_SOC_ARM_NXP_S32_COMMON_PINCTRL_SOC_COMMON_H_

#include <zephyr/devicetree.h>
#include <zephyr/dt-bindings/pinctrl/nxp-s32-pinctrl.h>
#include <zephyr/sys/util_macro.h>
#include <zephyr/types.h>

#include <Siul2_Port_Ip.h>

/** @cond INTERNAL_HIDDEN */

/**
 * @brief NXP S32 common macros for Pinctrl.
 *
 * Each SoC implementing Pinctrl must create a "pinctrl_soc.h" which includes
 * this file and defines the following macros:
 *
 * - NXP_S32_PIN_OPTIONS_INIT(group, mux)
 *   Populates SoC members of Siul2_Port_Ip_PinSettingsConfig struct based on
 *   SoC-specific group properties and the pinmux configuration.
 *
 * - NXP_S32_SIUL2_IDX(n)
 *   Expands to the SIUL2 struct pointer for the instance "n". The number of
 *   instances is SoC specific and may be not contiguous.
 *   Note: "n" may be a preprocessor expression so cannot be concatenated.
 */

/** @brief Type for NXP S32 pin configuration. */
typedef Siul2_Port_Ip_PinSettingsConfig pinctrl_soc_pin_t;

#define NXP_S32_INPUT_BUFFER(group)						\
	COND_CODE_1(DT_PROP(group, input_enable), (PORT_INPUT_BUFFER_ENABLED),	\
		(PORT_INPUT_BUFFER_DISABLED))

#define NXP_S32_OUTPUT_BUFFER(group)						\
	COND_CODE_1(DT_PROP(group, output_enable), (PORT_OUTPUT_BUFFER_ENABLED),\
		(PORT_OUTPUT_BUFFER_DISABLED))

#define NXP_S32_INPUT_MUX_REG(group, val)						\
	COND_CODE_1(DT_PROP(group, input_enable), (NXP_S32_PINMUX_GET_IMCR_IDX(val)),	\
		(0U))

#define NXP_S32_INPUT_MUX(group, val)						\
	COND_CODE_1(DT_PROP(group, input_enable),				\
		((Siul2_Port_Ip_PortInputMux)NXP_S32_PINMUX_GET_IMCR_SSS(val)),	\
		(PORT_INPUT_MUX_NO_INIT))

/* Only a single input mux is configured, the rest is not used */
#define NXP_S32_INPUT_MUX_NO_INIT						\
	[1 ... (FEATURE_SIUL2_MAX_NUMBER_OF_INPUT-1)] = PORT_INPUT_MUX_NO_INIT

#define NXP_S32_PINMUX_INIT(group, val)						\
	.base = NXP_S32_SIUL2_IDX(NXP_S32_PINMUX_GET_SIUL2_IDX(val)),		\
	.pinPortIdx = NXP_S32_PINMUX_GET_MSCR_IDX(val),				\
	.mux = (Siul2_Port_Ip_PortMux)NXP_S32_PINMUX_GET_MSCR_SSS(val),		\
	.inputMux = {								\
		NXP_S32_INPUT_MUX(group, val),					\
		NXP_S32_INPUT_MUX_NO_INIT					\
	},									\
	.inputMuxReg = {							\
		NXP_S32_INPUT_MUX_REG(group, val)				\
	},									\
	.inputBuffer = NXP_S32_INPUT_BUFFER(group),				\
	.outputBuffer = NXP_S32_OUTPUT_BUFFER(group),

/**
 * @brief Utility macro to initialize each pin.
 *
 *
 * @param group Group node identifier.
 * @param prop Property name.
 * @param idx Property entry index.
 */
#define Z_PINCTRL_STATE_PIN_INIT(group, prop, idx)					\
	{										\
		NXP_S32_PINMUX_INIT(group, DT_PROP_BY_IDX(group, prop, idx))		\
		NXP_S32_PIN_OPTIONS_INIT(group, DT_PROP_BY_IDX(group, prop, idx))	\
	},

/**
 * @brief Utility macro to initialize state pins contained in a given property.
 *
 * @param node_id Node identifier.
 * @param prop Property name describing state pins.
 */
#define Z_PINCTRL_STATE_PINS_INIT(node_id, prop)				\
	{DT_FOREACH_CHILD_VARGS(DT_PHANDLE(node_id, prop),			\
				DT_FOREACH_PROP_ELEM, pinmux,			\
				Z_PINCTRL_STATE_PIN_INIT)}

/** @endcond */

#endif /* ZEPHYR_SOC_ARM_NXP_NXP_S32_COMMON_PINCTRL_SOC_COMMON_H_ */
