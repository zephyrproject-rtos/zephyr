/*
 * Copyright (c) 2021 ITE Corporation. All Rights Reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _ITE_IT8XXX2_SOC_DT_H_
#define _ITE_IT8XXX2_SOC_DT_H_

/*
 * For it8xxx2 wake-up controller (WUC)
 */
#define IT8XXX2_DEV_WUC(idx, inst)					\
	DEVICE_DT_GET(DT_PHANDLE(IT8XXX2_DT_INST_WUCCTRL(inst, idx), wucs))
#define IT8XXX2_DEV_WUC_MASK(idx, inst)					\
	DT_PHA(IT8XXX2_DT_INST_WUCCTRL(inst, idx), wucs, mask)

/**
 * @brief For it8xxx2, get a node identifier from a wucctrl property
 *        for a DT_DRV_COMPAT instance
 *
 * @param inst instance number
 * @param idx index in the wucctrl property
 * @return node identifier for the phandle at index idx in the wucctrl
 *         property of that DT_DRV_COMPAT instance
 */
#define IT8XXX2_DT_INST_WUCCTRL(inst, idx)				\
	DT_INST_PHANDLE_BY_IDX(inst, wucctrl, idx)

/**
 * @brief For it8xxx2, construct wuc map structure in LISTIFY extension
 *
 * @param idx index in LISTIFY extension
 * @param inst instance number for compatible defined in DT_DRV_COMPAT
 * @return a structure of *_wuc_map_cfg
 */
#define IT8XXX2_DT_WUC_ITEMS_FUNC(idx, inst)				\
	{								\
		.wucs = IT8XXX2_DEV_WUC(idx, inst),			\
		.mask = IT8XXX2_DEV_WUC_MASK(idx, inst),		\
	}

/**
 * @brief For it8xxx2, get the length of wucctrl property which
 *        type is 'phandle-array' for a DT_DRV_COMPAT instance
 *
 * @param inst instance number
 * @return length of wucctrl property which type is 'phandle-array'
 */
#define IT8XXX2_DT_INST_WUCCTRL_LEN(inst)				\
	DT_INST_PROP_LEN(inst, wucctrl)

/**
 * @brief For it8xxx2, construct an array of it8xxx2 wuc map structure
 *        with compatible defined in DT_DRV_COMPAT by LISTIFY func
 *
 * @param inst instance number for compatible defined in DT_DRV_COMPAT
 * @return an array of *_wuc_map_cfg structure
 */
#define IT8XXX2_DT_WUC_ITEMS_LIST(inst) {				\
	LISTIFY(IT8XXX2_DT_INST_WUCCTRL_LEN(inst),			\
		IT8XXX2_DT_WUC_ITEMS_FUNC, (,),				\
		inst)							\
	}

#endif /* _ITE_IT8XXX2_SOC_DT_H_ */
