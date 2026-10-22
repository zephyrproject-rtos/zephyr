/*
 * Copyright 2022-2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SOC_NXP_S32_S32K3_PINCTRL_SOC_H_
#define ZEPHYR_SOC_NXP_S32_S32K3_PINCTRL_SOC_H_

#include <zephyr/drivers/pinctrl/pinctrl_nxp_siul2_common.h>
#include <zephyr/dt-bindings/pinctrl/nxp-siul2-pinctrl.h>
#include <zephyr/sys/util.h>

/* SIUL2 Multiplexed Signal Configuration */
#define SIUL2_MSCR_SSS_MASK GENMASK(3, 0)
#define SIUL2_MSCR_SSS(v)   FIELD_PREP(SIUL2_MSCR_SSS_MASK, (v))
#define SIUL2_MSCR_SMC_MASK BIT(5)
#define SIUL2_MSCR_SMC(v)   FIELD_PREP(SIUL2_MSCR_SMC_MASK, (v))
#define SIUL2_MSCR_IFE_MASK BIT(6)
#define SIUL2_MSCR_IFE(v)   FIELD_PREP(SIUL2_MSCR_IFE_MASK, (v))
#define SIUL2_MSCR_DSE_MASK BIT(8)
#define SIUL2_MSCR_DSE(v)   FIELD_PREP(SIUL2_MSCR_DSE_MASK, (v))
#define SIUL2_MSCR_PUS_MASK BIT(11)
#define SIUL2_MSCR_PUS(v)   FIELD_PREP(SIUL2_MSCR_PUS_MASK, (v))
#define SIUL2_MSCR_PUE_MASK BIT(13)
#define SIUL2_MSCR_PUE(v)   FIELD_PREP(SIUL2_MSCR_PUE_MASK, (v))
#define SIUL2_MSCR_SRC_MASK BIT(14)
#define SIUL2_MSCR_SRC(v)   FIELD_PREP(SIUL2_MSCR_SRC_MASK, (v))
#define SIUL2_MSCR_PKE_MASK BIT(16)
#define SIUL2_MSCR_PKE(v)   FIELD_PREP(SIUL2_MSCR_PKE_MASK, (v))
#define SIUL2_MSCR_INV_MASK BIT(17)
#define SIUL2_MSCR_INV(v)   FIELD_PREP(SIUL2_MSCR_INV_MASK, (v))
#define SIUL2_MSCR_IBE_MASK BIT(19)
#define SIUL2_MSCR_IBE(v)   FIELD_PREP(SIUL2_MSCR_IBE_MASK, (v))
#define SIUL2_MSCR_OBE_MASK BIT(21)
#define SIUL2_MSCR_OBE(v)   FIELD_PREP(SIUL2_MSCR_OBE_MASK, (v))
/* SIUL2 Input Multiplexed Signal Configuration */
#define SIUL2_IMCR_SSS_MASK GENMASK(3, 0)
#define SIUL2_IMCR_SSS(v)   FIELD_PREP(SIUL2_IMCR_SSS_MASK, (v))

#define NXP_SIUL2_PINMUX_INIT(group, value)                                                        \
	.mscr = {                                                                                  \
		.inst = NXP_SIUL2_PINMUX_GET_MSCR_SIUL2_IDX(value),                                \
		.idx = NXP_SIUL2_PINMUX_GET_MSCR_IDX(value),                                       \
		.val = SIUL2_MSCR_SSS(NXP_SIUL2_PINMUX_GET_MSCR_SSS(value)) |                      \
		       SIUL2_MSCR_OBE(DT_PROP(group, output_enable)) |                             \
		       SIUL2_MSCR_IBE(DT_PROP(group, input_enable)) |                              \
		       SIUL2_MSCR_PUE(DT_PROP(group, bias_pull_up) ||                              \
				      DT_PROP(group, bias_pull_down)) |                            \
		       SIUL2_MSCR_PUS(DT_PROP(group, bias_pull_up)) |                              \
		       SIUL2_MSCR_SRC(DT_ENUM_IDX(group, slew_rate)) |                             \
		       SIUL2_MSCR_DSE(DT_PROP(group, nxp_drive_strength)) |                        \
		       SIUL2_MSCR_INV(DT_PROP(group, nxp_invert))                                  \
	},                                                                                         \
	.imcr = {                                                                                  \
		.inst = NXP_SIUL2_PINMUX_GET_IMCR_SIUL2_IDX(value),                                \
		.idx = NXP_SIUL2_PINMUX_GET_IMCR_IDX(value),                                       \
		.val = SIUL2_IMCR_SSS(NXP_SIUL2_PINMUX_GET_IMCR_SSS(value)),                       \
	}

#endif /* ZEPHYR_SOC_NXP_S32_S32K3_PINCTRL_SOC_H_ */
