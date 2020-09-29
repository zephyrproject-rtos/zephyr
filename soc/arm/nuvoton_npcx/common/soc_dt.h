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

/* Get phandle from "wui_map" prop */
#define DT_PHANDLE_FROM_WUI_MAP(inst) \
	DT_INST_PHANDLE(inst, wui_map)

/* Construct a npcx_wui structure from wui_map property */
#define DT_NPCX_WUI_ITEM(inst) \
	{                                                                      \
	  .table = DT_PROP(DT_PHANDLE(DT_PHANDLE_FROM_WUI_MAP(inst),           \
					miwus), index),                        \
	  .group = DT_PHA(DT_PHANDLE_FROM_WUI_MAP(inst), miwus, group),        \
	  .bit   = DT_PHA(DT_PHANDLE_FROM_WUI_MAP(inst), miwus, bit),          \
	},

/* Get phandle from "wui_maps" prop which type is 'phandles' at index 'i' */
#define DT_PHANDLE_FROM_WUI_MAPS(inst, i) \
	DT_INST_PHANDLE_BY_IDX(inst, wui_maps, i)

/* Construct a npcx_wui structure from wui_maps property at index 'i' */
#define DT_NPCX_WUI_ITEM_BY_IDX(inst, i) \
	{                                                                      \
	  .table = DT_PROP(DT_PHANDLE(DT_PHANDLE_FROM_WUI_MAPS(inst, i),       \
					miwus), index),                        \
	  .group = DT_PHA(DT_PHANDLE_FROM_WUI_MAPS(inst, i), miwus, group),    \
	  .bit   = DT_PHA(DT_PHANDLE_FROM_WUI_MAPS(inst, i), miwus, bit),      \
	},

/* Length of npcx_wui structures in pinctrl property */
#define DT_NPCX_WUI_ITEMS_LEN(inst) DT_INST_PROP_LEN(inst, wui_maps)

/* Macro function to construct a list of npcx_wui items by UTIL_LISTIFY */
#define DT_NPCX_WUI_ITEMS_FUC(idx, inst) DT_NPCX_WUI_ITEM_BY_IDX(inst, idx)

#define DT_NPCX_WUI_ITEMS_LIST(inst) {             \
	UTIL_LISTIFY(DT_NPCX_WUI_ITEMS_LEN(inst),  \
		     DT_NPCX_WUI_ITEMS_FUC,        \
		     inst)                         \
	}

/* Get a node from path '/npcx_miwus_map/map_miwu(0/1/2)_groups' */
#define DT_NODE_FROM_MIWU_MAP(i)  DT_PATH(npcx7_miwus_int_map, \
					  map_miwu##i##_groups)

/* Get the index prop from parent MIWU device node */
#define DT_MIWU_IRQ_TABLE_IDX(child) \
	DT_PROP(DT_PHANDLE(DT_PARENT(child), parent), index)

/* Functiosn for DT_FOREACH_CHILD to generate a IRQ_CONNECT implementation */
#define DT_MIWU_IRQ_CONNECT_IMPL_CHILD_FUNC(child) \
	{                                                                      \
		IRQ_CONNECT(DT_PROP(child, irq),		               \
			DT_PROP(child, irq_prio),		               \
			NPCX_MIWU_ISR_FUNC(DT_MIWU_IRQ_TABLE_IDX(child)),      \
			DT_PROP(child, group_mask),                            \
			0);						       \
		irq_enable(DT_PROP(child, irq));                               \
	}

#endif /* _NUVOTON_NPCX_SOC_DT_H_ */
