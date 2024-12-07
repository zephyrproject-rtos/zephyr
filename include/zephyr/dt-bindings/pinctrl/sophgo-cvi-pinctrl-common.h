/*
 * Copyright (c) 2023-2024 Chen Xingyu <hi@xingrz.me>
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DT_SOPHGO_CVI_PINCTRL_COMMON_H_
#define ZEPHYR_DT_SOPHGO_CVI_PINCTRL_COMMON_H_

#include <zephyr/dt-bindings/dt-util.h>

#define CVI_PINMUX_FMUX_IDX_S (0)
#define CVI_PINMUX_FMUX_IDX_M BIT_MASK(8)

#define CVI_PINMUX_FMUX_SEL_S (8)
#define CVI_PINMUX_FMUX_SEL_M BIT_MASK(3)

#define CVI_PINMUX(pin, func)                                                                      \
	((((FMUX_IDX_##pin) & CVI_PINMUX_FMUX_IDX_M) << CVI_PINMUX_FMUX_IDX_S) |                   \
	 (((FMUX_SEL_##pin##__##func) & CVI_PINMUX_FMUX_SEL_M) << CVI_PINMUX_FMUX_SEL_S))

#endif /* ZEPHYR_DT_SOPHGO_CVI_PINCTRL_COMMON_H_ */
