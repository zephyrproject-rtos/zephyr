/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SOC_NXP_MCXE31X_PINCTRL_SOC_H_
#define ZEPHYR_SOC_NXP_MCXE31X_PINCTRL_SOC_H_

#include <zephyr/dt-bindings/pinctrl/nxp-s32-pinctrl.h>
#include <zephyr/sys/util.h>

#include "../../../s32/common/siul2_pinctrl.h"

#define NXP_S32_PINMUX_INIT(group, value)                                                          \
	.mscr = {.inst = NXP_S32_PINMUX_GET_MSCR_SIUL2_IDX(value),                                 \
		 .idx = NXP_S32_PINMUX_GET_MSCR_IDX(value),                                        \
		 .val = SIUL2_MSCR_SSS(NXP_S32_PINMUX_GET_MSCR_SSS(value)) |                       \
			SIUL2_MSCR_OBE(DT_PROP(group, output_enable)) |                            \
			SIUL2_MSCR_IBE(DT_PROP(group, input_enable)) |                             \
			SIUL2_MSCR_PUE(DT_PROP(group, bias_pull_up) ||                             \
				       DT_PROP(group, bias_pull_down)) |                           \
			SIUL2_MSCR_PUS(DT_PROP(group, bias_pull_up)) |                             \
			SIUL2_MSCR_SRC(DT_ENUM_IDX(group, slew_rate)) |                            \
			SIUL2_MSCR_DSE(DT_PROP(group, nxp_drive_strength)) |                       \
			SIUL2_MSCR_INV(DT_PROP(group, nxp_invert))},                               \
	.imcr = {                                                                                  \
		.inst = NXP_S32_PINMUX_GET_IMCR_SIUL2_IDX(value),                                  \
		.idx = NXP_S32_PINMUX_GET_IMCR_IDX(value),                                         \
		.val = SIUL2_IMCR_SSS(NXP_S32_PINMUX_GET_IMCR_SSS(value)),                         \
	}

#endif /* ZEPHYR_SOC_NXP_MCXE31X_PINCTRL_SOC_H_ */
