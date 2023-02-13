/*
 * Copyright 2022 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SOC_ARM_NXP_S32_S32ZE_PINCTRL_SOC_H_
#define ZEPHYR_SOC_ARM_NXP_S32_S32ZE_PINCTRL_SOC_H_

#include "pinctrl_soc_common.h"

#if defined(CONFIG_SOC_S32Z27_R52)
#include "S32Z2_SIUL2.h"
#else
#error "SoC not supported"
#endif

#define NXP_S32_SIUL2_IDX(n)							\
	n == 0 ? IP_SIUL2_0 : (n == 1 ? IP_SIUL2_1 : (				\
	n == 3 ? IP_SIUL2_3 : (n == 4 ? IP_SIUL2_4 : (				\
	n == 5 ? IP_SIUL2_5 : (NULL)))))

#define NXP_S32_PULL_SEL(group)								\
	COND_CODE_1(DT_PROP(group, bias_pull_up), (PORT_INTERNAL_PULL_UP_ENABLED),	\
		(COND_CODE_1(DT_PROP(group, bias_pull_down),				\
		(PORT_INTERNAL_PULL_DOWN_ENABLED), (PORT_INTERNAL_PULL_NOT_ENABLED))))

/* To enable open drain both OBE and ODE bits need to be set */
#define NXP_S32_OPEN_DRAIN(group)						\
	DT_PROP(group, drive_open_drain) && DT_PROP(group, output_enable) ?	\
		(PORT_OPEN_DRAIN_ENABLED) : (PORT_OPEN_DRAIN_DISABLED)

#define NXP_S32_SLEW_RATE(group)						\
	COND_CODE_1(DT_NODE_HAS_PROP(group, slew_rate),				\
		(UTIL_CAT(PORT_SLEW_RATE_CONTROL, DT_PROP(group, slew_rate))),	\
		(PORT_SLEW_RATE_CONTROL0))

/*
 * The default values are reset values and don't apply to the type of pads
 * currently supported.
 */
#define NXP_S32_PIN_OPTIONS_INIT(group, mux)					\
	.pullConfig = NXP_S32_PULL_SEL(group),					\
	.openDrain = NXP_S32_OPEN_DRAIN(group),					\
	.slewRateCtrlSel = NXP_S32_SLEW_RATE(group),				\
	.terminationResistor = PORT_TERMINATION_RESISTOR_DISABLED,		\
	.receiverSel = PORT_RECEIVER_ENABLE_SINGLE_ENDED,			\
	.currentReferenceControl = PORT_CURRENT_REFERENCE_CONTROL_DISABLED,	\
	.rxCurrentBoost = PORT_RX_CURRENT_BOOST_DISABLED,			\
	.safeMode = PORT_SAFE_MODE_DISABLED,

#endif /* ZEPHYR_SOC_ARM_NXP_S32_S32ZE_PINCTRL_SOC_H_ */
