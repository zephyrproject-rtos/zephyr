/*
 * Copyright (c) 2020 Nuvoton Technology Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _NUVOTON_NPCX_SOC_DT_H_
#define _NUVOTON_NPCX_SOC_DT_H_

/*
 * Construct a npcx_clk_cfg item from clocks prop which type is 'phandle-array'
 * to handle "clock-cells" in current driver device
 */
#define DT_NPCX_CLK_CFG_ITEM(inst)                                             \
	{                                                                      \
	  .bus  = DT_PHA(DT_DRV_INST(inst), clocks, bus),                      \
	  .ctrl = DT_PHA(DT_DRV_INST(inst), clocks, ctl),                      \
	  .bit  = DT_PHA(DT_DRV_INST(inst), clocks, bit),                      \
	}

/* Get phandle from "pinctrl" prop which type is 'phandles' at index 'i' */
#define DT_PHANDLE_FROM_PINCTRL(inst, i) \
	DT_INST_PHANDLE_BY_IDX(inst, pinctrl, i)

/* Construct a npcx_alt structure from pinctrl property at index 'i' */
#define DT_NPCX_ALT_ITEM_BY_IDX(inst, i)                                       \
	{                                                                      \
	  .group    = DT_PHA(DT_PHANDLE_FROM_PINCTRL(inst, i), alts, group),   \
	  .bit      = DT_PHA(DT_PHANDLE_FROM_PINCTRL(inst, i), alts, bit),     \
	  .inverted = DT_PHA(DT_PHANDLE_FROM_PINCTRL(inst, i), alts, inv),     \
	},

/* Length of npcx_alt structures in pinctrl property */
#define DT_NPCX_ALT_ITEMS_LEN(inst) DT_INST_PROP_LEN(inst, pinctrl)

/* Macro function to construct a list of npcx_alt items by UTIL_LISTIFY */
#define DT_NPCX_ALT_ITEMS_FUC(idx, inst) DT_NPCX_ALT_ITEM_BY_IDX(inst, idx)

#define DT_NPCX_ALT_ITEMS_LIST(inst) {             \
	UTIL_LISTIFY(DT_NPCX_ALT_ITEMS_LEN(inst),  \
		     DT_NPCX_ALT_ITEMS_FUC,        \
		     inst)                         \
	}


#endif /* _NUVOTON_NPCX_SOC_DT_H_ */
