/*
 * Copyright 2022-2023 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SOC_ARM_NXP_S32_COMMON_PINCTRL_SOC_H_
#define ZEPHYR_SOC_ARM_NXP_S32_COMMON_PINCTRL_SOC_H_

#include <zephyr/devicetree.h>
#include <zephyr/dt-bindings/pinctrl/nxp-s32-pinctrl.h>
#include <zephyr/sys/util_macro.h>
#include <zephyr/types.h>

#include <Siul2_Port_Ip.h>

/** @cond INTERNAL_HIDDEN */

/** @brief Type for NXP S32 pin configuration. */
typedef Siul2_Port_Ip_PinSettingsConfig pinctrl_soc_pin_t;

/* Alias for compatibility with previous RTD versions */
#if !defined(FEATURE_SIUL2_MAX_NUMBER_OF_INPUT) && defined(FEATURE_SIUL2_MAX_NUMBER_OF_INPUT_U8)
#define FEATURE_SIUL2_MAX_NUMBER_OF_INPUT FEATURE_SIUL2_MAX_NUMBER_OF_INPUT_U8
#endif

#if defined(SIUL2_PORT_IP_MULTIPLE_SIUL2_INSTANCES)
#define NXP_S32_SIUL2_IDX(n)								\
	n == 0 ? IP_SIUL2_0 : (n == 1 ? IP_SIUL2_1 : (					\
	n == 3 ? IP_SIUL2_3 : (n == 4 ? IP_SIUL2_4 : (					\
	n == 5 ? IP_SIUL2_5 : (NULL)))))
#else
#define NXP_S32_SIUL2_IDX(n) (n == 0 ? IP_SIUL2 : NULL)
#endif

#define NXP_S32_INPUT_BUFFER(group)							\
	COND_CODE_1(DT_PROP(group, input_enable), (PORT_INPUT_BUFFER_ENABLED),		\
		(PORT_INPUT_BUFFER_DISABLED))

#define NXP_S32_OUTPUT_BUFFER(group)							\
	COND_CODE_1(DT_PROP(group, output_enable), (PORT_OUTPUT_BUFFER_ENABLED),	\
		(PORT_OUTPUT_BUFFER_DISABLED))

#define NXP_S32_INPUT_MUX_REG(group, val)						\
	COND_CODE_1(DT_PROP(group, input_enable), (NXP_S32_PINMUX_GET_IMCR_IDX(val)),	\
		(0U))

#define NXP_S32_INPUT_MUX(group, val)							\
	COND_CODE_1(DT_PROP(group, input_enable),					\
		((Siul2_Port_Ip_PortInputMux)NXP_S32_PINMUX_GET_IMCR_SSS(val)),		\
		(PORT_INPUT_MUX_NO_INIT))

#define NXP_S32_PULL_SEL(group)								\
	COND_CODE_1(DT_PROP(group, bias_pull_up), (PORT_INTERNAL_PULL_UP_ENABLED),	\
		(COND_CODE_1(DT_PROP(group, bias_pull_down),				\
		(PORT_INTERNAL_PULL_DOWN_ENABLED), (PORT_INTERNAL_PULL_NOT_ENABLED))))

#if defined(SIUL2_PORT_IP_HAS_ONEBIT_SLEWRATE)
#define NXP_S32_SLEW_RATE(group)							\
	COND_CODE_1(DT_NODE_HAS_PROP(group, slew_rate),					\
		(UTIL_CAT(PORT_SLEW_RATE_, DT_STRING_UPPER_TOKEN(group, slew_rate))),	\
		(PORT_SLEW_RATE_FASTEST))
#else
#define NXP_S32_SLEW_RATE(group)							\
	COND_CODE_1(DT_NODE_HAS_PROP(group, slew_rate),					\
		(UTIL_CAT(PORT_SLEW_RATE_CONTROL, DT_PROP(group, slew_rate))),		\
		(PORT_SLEW_RATE_CONTROL0))
#endif

#define NXP_S32_DRIVE_STRENGTH(group)							\
	COND_CODE_1(DT_PROP(group, nxp_drive_strength),					\
		(PORT_DRIVE_STRENTGTH_ENABLED), (PORT_DRIVE_STRENTGTH_DISABLED))

#define NXP_S32_INVERT(group)								\
	COND_CODE_1(DT_PROP(group, nxp_invert),						\
		(PORT_INVERT_ENABLED), (PORT_INVERT_DISABLED))

/* To enable open drain both OBE and ODE bits need to be set */
#define NXP_S32_OPEN_DRAIN(group)							\
	(DT_PROP(group, drive_open_drain) && DT_PROP(group, output_enable) ?		\
		(PORT_OPEN_DRAIN_ENABLED) : (PORT_OPEN_DRAIN_DISABLED))

/* Only a single input mux is configured, the rest is not used */
#define NXP_S32_INPUT_MUX_NO_INIT							\
	[1 ... (FEATURE_SIUL2_MAX_NUMBER_OF_INPUT-1)] = PORT_INPUT_MUX_NO_INIT

#define NXP_S32_PINMUX_INIT(group, val)							\
	.base = NXP_S32_SIUL2_IDX(NXP_S32_PINMUX_GET_SIUL2_IDX(val)),			\
	.pinPortIdx = NXP_S32_PINMUX_GET_MSCR_IDX(val),					\
	.mux = (Siul2_Port_Ip_PortMux)NXP_S32_PINMUX_GET_MSCR_SSS(val),			\
	.inputMux = {									\
		NXP_S32_INPUT_MUX(group, val),						\
		NXP_S32_INPUT_MUX_NO_INIT						\
	},										\
	.inputMuxReg = {								\
		NXP_S32_INPUT_MUX_REG(group, val)					\
	},										\
	.inputBuffer = NXP_S32_INPUT_BUFFER(group),					\
	.outputBuffer = NXP_S32_OUTPUT_BUFFER(group),					\
	.pullConfig = NXP_S32_PULL_SEL(group),						\
	.safeMode = PORT_SAFE_MODE_DISABLED,						\
	.slewRateCtrlSel = NXP_S32_SLEW_RATE(group),					\
	.initValue = PORT_PIN_LEVEL_NOTCHANGED_U8,					\
	IF_ENABLED(__DEBRACKET FEATURE_SIUL2_PORT_IP_HAS_DRIVE_STRENGTH,		\
		(.driveStrength = NXP_S32_DRIVE_STRENGTH(group),))			\
	IF_ENABLED(__DEBRACKET FEATURE_SIUL2_PORT_IP_HAS_INVERT_DATA,			\
		(.invert = NXP_S32_INVERT(group),))					\
	IF_ENABLED(__DEBRACKET FEATURE_SIUL2_PORT_IP_HAS_OPEN_DRAIN,			\
		(.openDrain = NXP_S32_OPEN_DRAIN(group),))				\
	IF_ENABLED(__DEBRACKET FEATURE_SIUL2_PORT_IP_HAS_INPUT_FILTER,			\
		(.inputFilter = PORT_INPUT_FILTER_DISABLED,))				\
	IF_ENABLED(__DEBRACKET FEATURE_SIUL2_PORT_IP_HAS_RECEIVER_SELECT,		\
		(.receiverSel = PORT_RECEIVER_ENABLE_SINGLE_ENDED,))			\
	IF_ENABLED(__DEBRACKET FEATURE_SIUL2_PORT_IP_HAS_HYSTERESIS,			\
		(.hysteresis = PORT_HYSTERESIS_DISABLED,))				\
	IF_ENABLED(__DEBRACKET FEATURE_SIUL2_PORT_IP_HAS_ANALOG_PAD_CONTROL,		\
		(.analogPadControl = PORT_ANALOG_PAD_CONTROL_DISABLED,))		\
	IF_ENABLED(__DEBRACKET FEATURE_SIUL2_PORT_IP_HAS_TERMINATION_RESISTOR,		\
		(.terminationResistor = PORT_TERMINATION_RESISTOR_DISABLED,))		\
	IF_ENABLED(__DEBRACKET FEATURE_SIUL2_PORT_IP_HAS_CURRENT_REFERENCE_CONTROL,	\
		(.currentReferenceControl = PORT_CURRENT_REFERENCE_CONTROL_DISABLED,))	\
	IF_ENABLED(__DEBRACKET FEATURE_SIUL2_PORT_IP_HAS_RX_CURRENT_BOOST,		\
		(.rxCurrentBoost = PORT_RX_CURRENT_BOOST_DISABLED,))			\
	IF_ENABLED(__DEBRACKET FEATURE_SIUL2_PORT_IP_HAS_PULL_KEEPER,			\
		(.pullKeep = PORT_PULL_KEEP_DISABLED,))

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

#endif /* ZEPHYR_SOC_ARM_NXP_S32_COMMON_PINCTRL_SOC_H_ */
