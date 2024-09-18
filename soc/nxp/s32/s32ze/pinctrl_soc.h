/*
 * Copyright 2022-2024 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SOC_NXP_S32_S32ZE_PINCTRL_SOC_H_
#define ZEPHYR_SOC_NXP_S32_S32ZE_PINCTRL_SOC_H_

#include <zephyr/dt-bindings/pinctrl/nxp-s32-pinctrl.h>
#include <zephyr/sys/util.h>

#include "../common/siul2_pinctrl.h"

/* SIUL2 Multiplexed Signal Configuration */
#define SIUL2_MSCR_SSS_MASK  GENMASK(2, 0)
#define SIUL2_MSCR_SSS(v)    FIELD_PREP(SIUL2_MSCR_SSS_MASK, (v))
#define SIUL2_MSCR_SMC_MASK  GENMASK(7, 4)
#define SIUL2_MSCR_SMC(v)    FIELD_PREP(SIUL2_MSCR_SMC_MASK, (v))
#define SIUL2_MSCR_TRC_MASK  BIT(8)
#define SIUL2_MSCR_TRC(v)    FIELD_PREP(SIUL2_MSCR_TRC_MASK, (v))
#define SIUL2_MSCR_RCVR_MASK BIT(10)
#define SIUL2_MSCR_RCVR(v)   FIELD_PREP(SIUL2_MSCR_RCVR_MASK, (v))
#define SIUL2_MSCR_CREF_MASK BIT(11)
#define SIUL2_MSCR_CREF(v)   FIELD_PREP(SIUL2_MSCR_CREF_MASK, (v))
#define SIUL2_MSCR_PUS_MASK  BIT(12)
#define SIUL2_MSCR_PUS(v)    FIELD_PREP(SIUL2_MSCR_PUS_MASK, (v))
#define SIUL2_MSCR_PUE_MASK  BIT(13)
#define SIUL2_MSCR_PUE(v)    FIELD_PREP(SIUL2_MSCR_PUE_MASK, (v))
#define SIUL2_MSCR_SRE_MASK  GENMASK(16, 14)
#define SIUL2_MSCR_SRE(v)    FIELD_PREP(SIUL2_MSCR_SRE_MASK, (v))
#define SIUL2_MSCR_RXCB_MASK BIT(17)
#define SIUL2_MSCR_RXCB(v)   FIELD_PREP(SIUL2_MSCR_RXCB_MASK, (v))
#define SIUL2_MSCR_IBE_MASK  BIT(19)
#define SIUL2_MSCR_IBE(v)    FIELD_PREP(SIUL2_MSCR_IBE_MASK, (v))
#define SIUL2_MSCR_ODE_MASK  BIT(20)
#define SIUL2_MSCR_ODE(v)    FIELD_PREP(SIUL2_MSCR_ODE_MASK, (v))
#define SIUL2_MSCR_OBE_MASK  BIT(21)
#define SIUL2_MSCR_OBE(v)    FIELD_PREP(SIUL2_MSCR_OBE_MASK, (v))
/* SIUL2 Input Multiplexed Signal Configuration */
#define SIUL2_IMCR_SSS_MASK  GENMASK(2, 0)
#define SIUL2_IMCR_SSS(v)    FIELD_PREP(SIUL2_IMCR_SSS_MASK, (v))

#define NXP_S32_PINMUX_INIT(group, value)                                                          \
	.mscr = {                                                                                  \
		.inst = NXP_S32_PINMUX_GET_MSCR_SIUL2_IDX(value),                                  \
		.idx = NXP_S32_PINMUX_GET_MSCR_IDX(value),                                         \
		.val = SIUL2_MSCR_SSS(NXP_S32_PINMUX_GET_MSCR_SSS(value)) |                        \
		       SIUL2_MSCR_OBE(DT_PROP(group, output_enable)) |                             \
		       SIUL2_MSCR_IBE(DT_PROP(group, input_enable)) |                              \
		       SIUL2_MSCR_PUE(DT_PROP(group, bias_pull_up) ||                              \
				      DT_PROP(group, bias_pull_down)) |                            \
		       SIUL2_MSCR_PUS(DT_PROP(group, bias_pull_up)) |                              \
		       SIUL2_MSCR_SRE(DT_PROP(group, slew_rate)) |                                 \
		       SIUL2_MSCR_ODE(DT_PROP(group, drive_open_drain) &&                          \
				      DT_PROP(group, output_enable))                               \
	},                                                                                         \
	.imcr = {                                                                                  \
		.inst = NXP_S32_PINMUX_GET_IMCR_SIUL2_IDX(value),                                  \
		.idx = NXP_S32_PINMUX_GET_IMCR_IDX(value),                                         \
		.val = SIUL2_IMCR_SSS(NXP_S32_PINMUX_GET_IMCR_SSS(value)),                         \
	}

#endif /* ZEPHYR_SOC_NXP_S32_S32ZE_PINCTRL_SOC_H_ */
