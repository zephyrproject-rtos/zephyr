/*
 * Copyright (c) 2021 ITE Corporation. All Rights Reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _ITE_IT8XXX2_SOC_DT_H_
#define _ITE_IT8XXX2_SOC_DT_H_

#define IT8XXX2_DEV_PINMUX(idx, inst)    DEVICE_DT_GET(DT_PHANDLE(DT_PHANDLE_BY_IDX \
	(DT_DRV_INST(inst), pinctrl_0, idx), pinctrls))
#define IT8XXX2_DEV_PIN(idx, inst)       DT_PHA(DT_PHANDLE_BY_IDX \
	(DT_DRV_INST(inst), pinctrl_0, idx), pinctrls, pin)
#define IT8XXX2_DEV_ALT_FUNC(idx, inst)  DT_PHA(DT_PHANDLE_BY_IDX \
	(DT_DRV_INST(inst), pinctrl_0, idx), pinctrls, alt_func)

/**
 * @brief Macro function to construct it8xxx2 alt item in UTIL_LISTIFY extension.
 *
 * @param idx index in UTIL_LISTIFY extension.
 * @param inst instance number for compatible defined in DT_DRV_COMPAT.
 * @return macro function to construct a it8xxx2 alt structure.
 */
#define IT8XXX2_DT_ALT_ITEMS_FUNC(idx, inst)                 \
	{                                                    \
		.pinctrls = IT8XXX2_DEV_PINMUX(idx, inst),   \
		.pin = IT8XXX2_DEV_PIN(idx, inst),           \
		.alt_fun = IT8XXX2_DEV_ALT_FUNC(idx, inst),  \
	},

/**
 * @brief Macro function to construct a list of it8xxx2 alt items with
 * compatible defined in DT_DRV_COMPAT by UTIL_LISTIFY func
 *
 * @param inst instance number for compatible defined in DT_DRV_COMPAT.
 * @return an array of it8xxx2 alt items.
 */
#define IT8XXX2_DT_ALT_ITEMS_LIST(inst) {                \
	UTIL_LISTIFY(DT_INST_PROP_LEN(inst, pinctrl_0),  \
		     IT8XXX2_DT_ALT_ITEMS_FUNC,          \
		     inst)                               \
	}

#endif /* _ITE_IT8XXX2_SOC_DT_H_ */
