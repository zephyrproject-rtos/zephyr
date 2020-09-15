/*
 * Copyright (c) 2020 Nuvoton Technology Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _NUVOTON_NPCX_SOC_DT_H_
#define _NUVOTON_NPCX_SOC_DT_H_

/**
 * @brief Construct a npcx_clk_cfg item from first item in 'clocks' prop which
 * type is 'phandle-array' to handle "clock-cells" in current driver.
 *
 * Example devicetree fragment:
 *    / {
 *		uart1: serial@400c4000 {
 *			clocks = <&pcc NPCX_CLOCK_BUS_APB2 NPCX_PWDWN_CTL1 4>;
 *			...
 *		};
 *	};
 *
 * Example usage: *
 *      const struct npcx_clk_cfg clk_cfg = DT_NPCX_CLK_CFG_ITEM(inst);
 *
 * @param inst instance number for compatible defined in DT_DRV_COMPAT.
 * @return npcx_clk_cfg item.
 */
#define DT_NPCX_CLK_CFG_ITEM(inst)                                             \
	{                                                                      \
	  .bus  = DT_PHA(DT_DRV_INST(inst), clocks, bus),                      \
	  .ctrl = DT_PHA(DT_DRV_INST(inst), clocks, ctl),                      \
	  .bit  = DT_PHA(DT_DRV_INST(inst), clocks, bit),                      \
	}

/**
 * @brief Construct a npcx_clk_cfg structure from 'clocks' property at index 'i'
 *
 * @param inst instance number for compatible defined in DT_DRV_COMPAT.
 * @param i index of clocks prop which type is 'phandle-array'
 * @return npcx_clk_cfg item from 'clocks' property at index 'i'
 */
#define DT_NPCX_CLK_CFG_ITEM_BY_IDX(inst, i)                                   \
	{                                                                      \
	  .bus  = DT_CLOCKS_CELL_BY_IDX(DT_DRV_INST(inst), i, bus),            \
	  .ctrl = DT_CLOCKS_CELL_BY_IDX(DT_DRV_INST(inst), i, ctl),            \
	  .bit  = DT_CLOCKS_CELL_BY_IDX(DT_DRV_INST(inst), i, bit),            \
	},

/**
 * @brief Length of 'clocks' property which type is 'phandle-array'
 *
 * @param inst instance number for compatible defined in DT_DRV_COMPAT.
 * @return length of 'clocks' property which type is 'phandle-array'
 */
#define DT_NPCX_CLK_CFG_ITEMS_LEN(inst) DT_INST_PROP_LEN(inst, clocks)

/**
 * @brief Macro function to construct npcx_clk_cfg item in UTIL_LISTIFY
 * extension.
 *
 * @param child child index in UTIL_LISTIFY extension.
 * @param inst instance number for compatible defined in DT_DRV_COMPAT.
 * @return macro function to construct a npcx_clk_cfg structure.
 */
#define DT_NPCX_CLK_CFG_ITEMS_FUC(child, inst) \
					DT_NPCX_CLK_CFG_ITEM_BY_IDX(inst, child)

/**
 * @brief Macro function to construct a list of npcx_clk_cfg items by
 * UTIL_LISTIFY func
 *
 * Example devicetree fragment:
 *    / {
 *		host_sub: lpc@400c1000 {
 *			clocks = <&pcc NPCX_CLOCK_BUS_APB3 NPCX_PWDWN_CTL5 3>,
 *				 <&pcc NPCX_CLOCK_BUS_APB3 NPCX_PWDWN_CTL5 4>,
 *				 <&pcc NPCX_CLOCK_BUS_APB3 NPCX_PWDWN_CTL5 5>,
 *				 <&pcc NPCX_CLOCK_BUS_APB3 NPCX_PWDWN_CTL5 6>,
 *				 <&pcc NPCX_CLOCK_BUS_APB3 NPCX_PWDWN_CTL5 7>;
 *			...
 *		};
 * Example usage: *
 *	const struct npcx_clk_cfg clk_cfg[] = DT_NPCX_CLK_CFG_ITEMS_LIST(0);
 *
 * @param inst instance number for compatible defined in DT_DRV_COMPAT.
 * @return an array of npcx_clk_cfg items.
 */
#define DT_NPCX_CLK_CFG_ITEMS_LIST(inst) {             \
	UTIL_LISTIFY(DT_NPCX_CLK_CFG_ITEMS_LEN(inst),  \
		     DT_NPCX_CLK_CFG_ITEMS_FUC,        \
		     inst)                             \
	}

/**
 * @brief Get phandle from 'pinctrl' prop which type is 'phandles' at index 'i'
 *
 * @param inst instance number for compatible defined in DT_DRV_COMPAT.
 * @param i index of pinctrl prop which type is 'phandles'
 * @return phandle from 'pinctrl' prop at index 'i'
 */
#define DT_PHANDLE_FROM_PINCTRL(inst, i) \
	DT_INST_PHANDLE_BY_IDX(inst, pinctrl, i)

/**
 * @brief Construct a npcx_alt structure from 'pinctrl' property at index 'i'
 *
 * @param inst instance number for compatible defined in DT_DRV_COMPAT.
 * @param i index of pinctrl prop which type is 'phandles'
 * @return npcx_alt item from 'pinctrl' property at index 'i'
 */
#define DT_NPCX_ALT_ITEM_BY_IDX(inst, i)                                       \
	{                                                                      \
	  .group    = DT_PHA(DT_PHANDLE_FROM_PINCTRL(inst, i), alts, group),   \
	  .bit      = DT_PHA(DT_PHANDLE_FROM_PINCTRL(inst, i), alts, bit),     \
	  .inverted = DT_PHA(DT_PHANDLE_FROM_PINCTRL(inst, i), alts, inv),     \
	},

/**
 * @brief Length of npcx_alt structures in 'pinctrl' property
 *
 * @param inst instance number for compatible defined in DT_DRV_COMPAT.
 * @return length of 'pinctrl' property which type is 'phandles'
 */
#define DT_NPCX_ALT_ITEMS_LEN(inst) DT_INST_PROP_LEN(inst, pinctrl)

/**
 * @brief Macro function to construct npcx_alt item in UTIL_LISTIFY extension.
 *
 * @param child child index in UTIL_LISTIFY extension.
 * @param inst instance number for compatible defined in DT_DRV_COMPAT.
 * @return macro function to construct a npcx_alt structure.
 */
#define DT_NPCX_ALT_ITEMS_FUC(child, inst) DT_NPCX_ALT_ITEM_BY_IDX(inst, child)

/**
 * @brief Macro function to construct a list of npcx_alt items by UTIL_LISTIFY
 * func
 *
 * Example devicetree fragment:
 *    / {
 *		uart1: serial@400c4000 {
 *			pinctrl = <&alta_uart1_sl1>;
 *			...
 *		};
 *
 *		host_sub: lpc@400c1000 {
 *			pinctrl = <&altb_rxd_sl &altb_txd_sl
 *				   &altb_rts_sl &altb_cts_sl
 *				   &altb_ri_sl &altb_dtr_bout_sl
 *				   &altb_dcd_sl &altb_dsr_sl>;
 *			...
 *		};
 *	};
 *
 * Example usage: *
 *      const struct npcx_alt uart_alts[] = DT_NPCX_ALT_ITEMS_LIST(inst);
 *      const struct npcx_alt host_uart_alts[] = DT_NPCX_ALT_ITEMS_LIST(inst);
 *
 * @param inst instance number for compatible defined in DT_DRV_COMPAT.
 * @return an array of npcx_alt items.
 */
#define DT_NPCX_ALT_ITEMS_LIST(inst) {             \
	UTIL_LISTIFY(DT_NPCX_ALT_ITEMS_LEN(inst),  \
		     DT_NPCX_ALT_ITEMS_FUC,        \
		     inst)                         \
	}

/**
 * @brief Get phandle from "name" property which contains wui information.
 *
 * @param inst instance number for compatible defined in DT_DRV_COMPAT.
 * @param name property 'name' which type is 'phandle' and contains wui info.
 * @return phandle from 'name' property.
 */
#define DT_PHANDLE_FROM_WUI_NAME(inst, name) \
	DT_INST_PHANDLE(inst, name)

/**
 * @brief Construct a npcx_wui structure from 'name' property
 *
 * @param inst instance number for compatible defined in DT_DRV_COMPAT.
 * @param name property 'name'which type is 'phandle' and contains wui info.
 * @return npcx_wui item from 'name' property.
 */
#define DT_NPCX_WUI_ITEM_BY_NAME(inst, name)				       \
	{                                                                      \
	  .table = DT_PROP(DT_PHANDLE(DT_PHANDLE_FROM_WUI_NAME(inst, name),    \
					miwus), index),                        \
	  .group = DT_PHA(DT_PHANDLE_FROM_WUI_NAME(inst, name), miwus, group), \
	  .bit   = DT_PHA(DT_PHANDLE_FROM_WUI_NAME(inst, name), miwus, bit),   \
	}

/**
 * @brief Get phandle from 'wui_maps' prop which type is 'phandles' at index 'i'
 *
 * @param inst instance number for compatible defined in DT_DRV_COMPAT.
 * @param i index of 'wui_maps' prop which type is 'phandles'
 * @return phandle from 'wui_maps' prop at index 'i'
 */
#define DT_PHANDLE_FROM_WUI_MAPS(inst, i) \
	DT_INST_PHANDLE_BY_IDX(inst, wui_maps, i)

/**
 * @brief Construct a npcx_wui structure from wui_maps property at index 'i'
 *
 * @param inst instance number for compatible defined in DT_DRV_COMPAT.
 * @param i index of 'wui_maps' prop which type is 'phandles'
 * @return npcx_wui item at index 'i'
 */
#define DT_NPCX_WUI_ITEM_BY_IDX(inst, i) \
	{                                                                      \
	  .table = DT_PROP(DT_PHANDLE(DT_PHANDLE_FROM_WUI_MAPS(inst, i),       \
					miwus), index),                        \
	  .group = DT_PHA(DT_PHANDLE_FROM_WUI_MAPS(inst, i), miwus, group),    \
	  .bit   = DT_PHA(DT_PHANDLE_FROM_WUI_MAPS(inst, i), miwus, bit),      \
	},

/**
 * @brief Length of npcx_wui structures in 'wui_maps' property
 *
 * @param inst instance number for compatible defined in DT_DRV_COMPAT.
 * @return length of 'wui_maps' prop which type is 'phandles'
 */
#define DT_NPCX_WUI_ITEMS_LEN(inst) DT_INST_PROP_LEN(inst, wui_maps)

/**
 * @brief Macro function to construct a list of npcx_wui items by UTIL_LISTIFY
 *
 * @param child child index in UTIL_LISTIFY extension.
 * @param inst instance number for compatible defined in DT_DRV_COMPAT.
 * @return macro function to construct a npcx_wui structure.
 */
#define DT_NPCX_WUI_ITEMS_FUC(child, inst) DT_NPCX_WUI_ITEM_BY_IDX(inst, child)

/**
 * @brief Macro function to construct a list of npcx_wui items by UTIL_LISTIFY
 * func.
 *
 * Example devicetree fragment:
 *    / {
 *		uart1: serial@400c4000 {
 *			uart_rx = <&wui_cr_sin1>;
 *			...
 *		};
 *
 *		gpio0: gpio@40081000 {
 *			wui_maps = <&wui_io00 &wui_io01 &wui_io02 &wui_io03
 *				    &wui_io04 &wui_io05 &wui_io06 &wui_io07>;
 *			...
 *		};
 *	};
 *
 * Example usage: *
 *      const struct npcx_wui wui_map = DT_PHANDLE_FROM_WUI_NAME(inst, uart_rx);
 *      const struct npcx_wui wui_maps[] = DT_NPCX_WUI_ITEMS_LIST(inst);
 *
 * @param inst instance number for compatible defined in DT_DRV_COMPAT.
 * @return an array of npcx_wui items.
 */
#define DT_NPCX_WUI_ITEMS_LIST(inst) {             \
	UTIL_LISTIFY(DT_NPCX_WUI_ITEMS_LEN(inst),  \
		     DT_NPCX_WUI_ITEMS_FUC,        \
		     inst)                         \
	}

/**
 * @brief Get a node from path '/npcx_miwus_map/map_miwu(0/1/2)_groups'
 *
 * @param i index of npcx miwu devices
 * @return node identifier with that path.
 */
#define DT_NODE_FROM_MIWU_MAP(i)  DT_PATH(npcx7_miwus_int_map, \
					  map_miwu##i##_groups)
/**
 * @brief Get the index prop from parent MIWU device node.
 *
 * @param child index in UTIL_LISTIFY extension.
 * @return 'index' prop value of the node which compatible type is
 * "nuvoton,npcx-miwu".
 */
#define DT_MIWU_IRQ_TABLE_IDX(child) \
	DT_PROP(DT_PHANDLE(DT_PARENT(child), parent), index)

/**
 * @brief Macro function for DT_FOREACH_CHILD to generate a IRQ_CONNECT
 * implementation.
 *
 * @param child index in UTIL_LISTIFY extension.
 * @return implementation to initialize interrupts of MIWU groups and enable
 * them.
 */
#define DT_MIWU_IRQ_CONNECT_IMPL_CHILD_FUNC(child) \
	{                                                                      \
		IRQ_CONNECT(DT_PROP(child, irq),		               \
			DT_PROP(child, irq_prio),		               \
			NPCX_MIWU_ISR_FUNC(DT_MIWU_IRQ_TABLE_IDX(child)),      \
			DT_PROP(child, group_mask),                            \
			0);						       \
		irq_enable(DT_PROP(child, irq));                               \
	}

/**
 * @brief Get a child node from path '/npcx7_espi_vws_map/name'.
 *
 * @param name a path which name is /npcx7_espi_vws_map/'name'.
 * @return child node identifier with that path.
 */
#define DT_NODE_FROM_VWTABLE(name) DT_CHILD(DT_PATH(npcx7_espi_vws_map), name)

/**
 * @brief Get phandle from wui_map property of child node with that path.
 *
 * @param name path which name is /npcx7_espi_vws_map/'name'.
 * @return phandle from "wui_map" prop of child node with that path.
 */
#define DT_PHANDLE_VW_WUI(name) DT_PHANDLE(DT_NODE_FROM_VWTABLE(name), wui_map)

/**
 * @brief Construct a npcx_wui structure from wui_map property of a child node
 * with that path.
 *
 * @param name a path which name is /npcx7_espi_vws_map/'name'.
 * @return npcx_wui item with that path.
 */
#define DT_NPCX_VW_WUI_ITEM(name)			                       \
	{                                                                      \
	  .table = DT_PROP(DT_PHANDLE(DT_PHANDLE_VW_WUI(name), miwus),  index),\
	  .group = DT_PHA(DT_PHANDLE_VW_WUI(name), miwus, group),              \
	  .bit   = DT_PHA(DT_PHANDLE_VW_WUI(name), miwus, bit),                \
	}

/**
 * @brief Construct a npcx espi device configuration of vw input signal from
 * a child node with that path.
 *
 * @signal vw input signal name.
 * @param name a path which name is /npcx7_espi_vws_map/'name'.
 * @return npcx_vw_in_config item with that path.
 */
#define DT_NPCX_VW_IN_CONF(signal, name)                                       \
	{                                                                      \
	  .sig = signal,                                                       \
	  .reg_idx = DT_PROP_BY_IDX(DT_NODE_FROM_VWTABLE(name), vw_reg, 0),    \
	  .bitmask = DT_PROP_BY_IDX(DT_NODE_FROM_VWTABLE(name), vw_reg, 1),    \
	  .vw_wui  = DT_NPCX_VW_WUI_ITEM(name),                                \
	}

/**
 * @brief Construct a npcx espi device configuration of vw output signal from
 * a child node with that path.
 *
 * @signal vw output signal name.
 * @param name a path which name is /npcx7_espi_vws_map/'name'.
 * @return npcx_vw_in_config item with that path.
 */
#define DT_NPCX_VW_OUT_CONF(signal, name)                                      \
	{                                                                      \
	  .sig = signal,                                                       \
	  .reg_idx = DT_PROP_BY_IDX(DT_NODE_FROM_VWTABLE(name), vw_reg, 0),    \
	  .bitmask = DT_PROP_BY_IDX(DT_NODE_FROM_VWTABLE(name), vw_reg, 1),    \
	}

#endif /* _NUVOTON_NPCX_SOC_DT_H_ */
