/*
 * Copyright (c) 2022 ITE Technology Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_PINCTRL_IT8XXX2_PINCTRL_KSCAN_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_PINCTRL_IT8XXX2_PINCTRL_KSCAN_H_

/* It8xxx2 KSI[7:0] don't support push-pull and open-drain settings in kbs mode */
#define IT8XXX2_PINCTRL_KSCAN_NOT_SUPPORT_PP_OD 0

#endif	/* ZEPHYR_INCLUDE_DT_BINDINGS_PINCTRL_IT8XXX2_PINCTRL_KSCAN_H_ */
