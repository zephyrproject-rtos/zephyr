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

/* Construct a npcx_clk_cfg structure from clocks property at index 'i' */
#define DT_NPCX_CLK_CFG_ITEM_BY_IDX(inst, i)                                   \
	{                                                                      \
	  .bus  = DT_CLOCKS_CELL_BY_IDX(DT_DRV_INST(inst), i, bus),            \
	  .ctrl = DT_CLOCKS_CELL_BY_IDX(DT_DRV_INST(inst), i, ctl),            \
	  .bit  = DT_CLOCKS_CELL_BY_IDX(DT_DRV_INST(inst), i, bit),            \
	},

/* Length of npcx_clk_cfg structures in clocks property */
#define DT_NPCX_CLK_CFG_ITEMS_LEN(inst) DT_INST_PROP_LEN(inst, clocks)

/* Macro function to construct a list of npcx_clk_cfg items by UTIL_LISTIFY */
#define DT_NPCX_CLK_CFG_ITEMS_FUC(idx, inst) \
					DT_NPCX_CLK_CFG_ITEM_BY_IDX(inst, idx)

#define DT_NPCX_CLK_CFG_ITEMS_LIST(inst) {             \
	UTIL_LISTIFY(DT_NPCX_CLK_CFG_ITEMS_LEN(inst),  \
		     DT_NPCX_CLK_CFG_ITEMS_FUC,        \
		     inst)                             \
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

/* Get a child node from path '/npcx7_espi_vws_map/name' */
#define DT_NODE_FROM_VWTABLE(name) DT_CHILD(DT_PATH(npcx7_espi_vws_map), name)

/* Get a handle from wui_map property of child node */
#define DT_PHANDLE_VW_WUI(name) DT_PHANDLE(DT_NODE_FROM_VWTABLE(name), wui_map)

/* Construct a npcx_wui structure from wui_map property of vw table by name */
#define DT_NPCX_VW_WUI_ITEM(name)			                       \
	{                                                                      \
	  .table = DT_PROP(DT_PHANDLE(DT_PHANDLE_VW_WUI(name), miwus),  index),\
	  .group = DT_PHA(DT_PHANDLE_VW_WUI(name), miwus, group),              \
	  .bit   = DT_PHA(DT_PHANDLE_VW_WUI(name), miwus, bit),                \
	}

/* Construct a npcx espi device configuration for vw input signal by name */
#define DT_NPCX_VW_IN_CONF(signal, name)                                       \
	{                                                                      \
	  .sig = signal,                                                       \
	  .reg_idx = DT_PROP_BY_IDX(DT_NODE_FROM_VWTABLE(name), vw_reg, 0),    \
	  .bitmask = DT_PROP_BY_IDX(DT_NODE_FROM_VWTABLE(name), vw_reg, 1),    \
	  .vw_wui  = DT_NPCX_VW_WUI_ITEM(name),                                \
	}

/* Construct a npcx espi device configuration for vw output signal by name */
#define DT_NPCX_VW_OUT_CONF(signal, name)                                      \
	{                                                                      \
	  .sig = signal,                                                       \
	  .reg_idx = DT_PROP_BY_IDX(DT_NODE_FROM_VWTABLE(name), vw_reg, 0),    \
	  .bitmask = DT_PROP_BY_IDX(DT_NODE_FROM_VWTABLE(name), vw_reg, 1),    \
	}

/* Get phandle from "name" prop */
#define DT_PHANDLE_FROM_WUI_NAME(name) \
	DT_INST_PHANDLE(0, name)

/* Construct a npcx_wui structure from wui_map property */
#define DT_NPCX_ESPI_WUI_ITEM(name)				               \
	{                                                                      \
	  .table = DT_PROP(DT_PHANDLE(DT_PHANDLE_FROM_WUI_NAME(name),          \
					miwus), index),                        \
	  .group = DT_PHA(DT_PHANDLE_FROM_WUI_NAME(name), miwus, group),       \
	  .bit   = DT_PHA(DT_PHANDLE_FROM_WUI_NAME(name), miwus, bit),         \
	}

#endif /* _NUVOTON_NPCX_SOC_DT_H_ */
