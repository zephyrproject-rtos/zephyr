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
 * Example usage:
 *      const struct npcx_clk_cfg clk_cfg = NPCX_DT_CLK_CFG_ITEM(inst);
 *
 * @param inst instance number for compatible defined in DT_DRV_COMPAT.
 * @return npcx_clk_cfg item.
 */
#define NPCX_DT_CLK_CFG_ITEM(inst)                                             \
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
#define NPCX_DT_CLK_CFG_ITEM_BY_IDX(inst, i)                                   \
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
#define NPCX_DT_CLK_CFG_ITEMS_LEN(inst) DT_INST_PROP_LEN(inst, clocks)

/**
 * @brief Macro function to construct npcx_clk_cfg item in UTIL_LISTIFY
 * extension.
 *
 * @param child child index in UTIL_LISTIFY extension.
 * @param inst instance number for compatible defined in DT_DRV_COMPAT.
 * @return macro function to construct a npcx_clk_cfg structure.
 */
#define NPCX_DT_CLK_CFG_ITEMS_FUNC(child, inst) \
					NPCX_DT_CLK_CFG_ITEM_BY_IDX(inst, child)

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
 * Example usage:
 *	const struct npcx_clk_cfg clk_cfg[] = NPCX_DT_CLK_CFG_ITEMS_LIST(0);
 *
 * @param inst instance number for compatible defined in DT_DRV_COMPAT.
 * @return an array of npcx_clk_cfg items.
 */
#define NPCX_DT_CLK_CFG_ITEMS_LIST(inst) {             \
	UTIL_LISTIFY(NPCX_DT_CLK_CFG_ITEMS_LEN(inst),  \
		     NPCX_DT_CLK_CFG_ITEMS_FUNC,       \
		     inst)                             \
	}

/**
 * @brief Get phandle from 'pinctrl-0' prop which type is 'phandles' at index
 *        'i'
 *
 * @param inst instance number for compatible defined in DT_DRV_COMPAT.
 * @param i index of 'pinctrl-0' prop which type is 'phandles'
 * @return phandle from 'pinctrl-0' prop at index 'i'
 */
#define NPCX_DT_PHANDLE_FROM_PINCTRL(inst, i) \
	DT_INST_PHANDLE_BY_IDX(inst, pinctrl_0, i)

/**
 * @brief Construct a npcx_alt structure from 'pinctrl-0' property at index 'i'
 *
 * @param inst instance number for compatible defined in DT_DRV_COMPAT.
 * @param i index of 'pinctrl-0' prop which type is 'phandles'
 * @return npcx_alt item from 'pinctrl-0' property at index 'i'
 */
#define NPCX_DT_ALT_ITEM_BY_IDX(inst, i)                                       \
	{                                                                      \
	 .group = DT_PHA(NPCX_DT_PHANDLE_FROM_PINCTRL(inst, i), alts, group),  \
	  .bit = DT_PHA(NPCX_DT_PHANDLE_FROM_PINCTRL(inst, i), alts, bit),     \
	  .inverted = DT_PHA(NPCX_DT_PHANDLE_FROM_PINCTRL(inst, i), alts, inv),\
	},

/**
 * @brief Length of npcx_alt structures in 'pinctrl-0' property
 *
 * @param inst instance number for compatible defined in DT_DRV_COMPAT.
 * @return length of 'pinctrl-0' property which type is 'phandles'
 */
#define NPCX_DT_ALT_ITEMS_LEN(inst) DT_INST_PROP_LEN(inst, pinctrl_0)

/**
 * @brief Macro function to construct npcx_alt item in UTIL_LISTIFY extension.
 *
 * @param child child index in UTIL_LISTIFY extension.
 * @param inst instance number for compatible defined in DT_DRV_COMPAT.
 * @return macro function to construct a npcx_alt structure.
 */
#define NPCX_DT_ALT_ITEMS_FUNC(child, inst) NPCX_DT_ALT_ITEM_BY_IDX(inst, child)

/**
 * @brief Macro function to construct a list of npcx_alt items with compatible
 * defined in DT_DRV_COMPAT by UTIL_LISTIFY func
 *
 * Example devicetree fragment:
 *    / {
 *		uart1: serial@400c4000 {
 *			pinctrl-0 = <&alta_uart1_sl1>;
 *			...
 *		};
 *	};
 *
 * Example usage:
 *      const struct npcx_alt uart_alts[] = NPCX_DT_ALT_ITEMS_LIST(inst);
 *
 * @param inst instance number for compatible defined in DT_DRV_COMPAT.
 * @return an array of npcx_alt items.
 */
#define NPCX_DT_ALT_ITEMS_LIST(inst) {             \
	UTIL_LISTIFY(NPCX_DT_ALT_ITEMS_LEN(inst),  \
		     NPCX_DT_ALT_ITEMS_FUNC,       \
		     inst)                         \
	}

/**
 * @brief Node identifier for an instance of a specific compatible
 *
 * @param compat specific compatible of devices in device-tree file
 * @param inst instance number
 * @return a node identifier for the node with "io_comp" compatible and
 *         instance number "inst"
 */
#define NPCX_DT_COMP_INST(compat, inst) DT_INST(inst, compat)

/**
 * @brief Get a specific compatible instance's node identifier for a phandle in
 * a property.
 *
 * @param compat specific compatible of devices in device-tree file
 * @param inst instance number
 * @param prop lowercase-and-underscores property name in "inst"
 *             with type "phandle", "phandles" or "phandle-array"
 * @param idx index into "prop"
 * @return a node identifier for the phandle at index "idx" in "prop"
 */
#define NPCX_DT_COMP_INST_PHANDLE_BY_IDX(compat, inst, prop, idx) \
	DT_PHANDLE_BY_IDX(NPCX_DT_COMP_INST(compat, inst), prop, idx)

/**
 * @brief Get phandle from 'pinctrl-0' prop which type is 'phandles' at index
 *        'i' from io-pads device with specific compatible.
 *
 * @param io_comp compatible string in devicetree file for io-pads device
 * @param inst instance number for compatible defined in io_comp.
 * @param i index of 'pinctrl-0' prop which type is 'phandles'
 * @return phandle from 'pinctrl-0' prop at index 'i'
 */
#define NPCX_DT_IO_PHANDLE_FROM_PINCTRL(io_comp, inst, i) \
	NPCX_DT_COMP_INST_PHANDLE_BY_IDX(io_comp, inst, pinctrl_0, i)

/**
 * @brief Construct a npcx_alt structure from 'pinctrl-0' property at index 'i'
 *        from io-pads device with specific compatible.
 *
 * @param io_comp compatible string in devicetree file for io-pads device
 * @param inst instance number for compatible defined in io_comp.
 * @param i index of 'pinctrl_0' prop which type is 'phandles'
 * @return npcx_alt item from 'pinctrl-0' property at index 'i'
 */
#define NPCX_DT_IO_ALT_ITEM_BY_IDX(io_comp, inst, i)                           \
	{                                                                      \
	  .group = DT_PHA(NPCX_DT_IO_PHANDLE_FROM_PINCTRL(io_comp, inst, i),   \
								alts, group),  \
	  .bit = DT_PHA(NPCX_DT_IO_PHANDLE_FROM_PINCTRL(io_comp, inst, i),     \
								alts, bit),    \
	  .inverted = DT_PHA(NPCX_DT_IO_PHANDLE_FROM_PINCTRL(io_comp, inst, i),\
								alts, inv),    \
	},

/**
 * @brief Length of npcx_alt structures in 'pinctrl-0' property of specific
 *        compatible io-pads device
 *
 * @param io_comp compatible string in devicetree file for io-pads device
 * @param inst instance number for compatible defined in io_comp.
 * @return length of 'pinctrl-0' property which type is 'phandles'
 */
#define NPCX_DT_IO_ALT_ITEMS_LEN(io_comp, inst) \
			DT_PROP_LEN(NPCX_DT_COMP_INST(io_comp, inst), pinctrl_0)

/**
 * @brief Macro function to construct npcx_alt item with specific compatible
 *        string in UTIL_LISTIFY extension.
 *
 * @param child child index in UTIL_LISTIFY extension.
 * @param inst instance number for compatible defined in io_comp.
 * @param io_comp compatible string in devicetree file for io-pads device
 * @return macro function to construct a npcx_alt structure.
 */
#define NPCX_DT_IO_ALT_ITEMS_FUNC(child, inst, io_comp) \
			NPCX_DT_IO_ALT_ITEM_BY_IDX(io_comp, inst, child)

/**
 * @brief Macro function to construct a list of npcx_alt items with specific
 *        compatible string by UTIL_LISTIFY func
 *
 * Example devicetree fragment:
 *    / {
 *		host_uart: io_host_uart {
 *			compatible = "nuvoton,npcx-host-uart";
 *
 *			pinctrl-0 = <&altb_rxd_sl &altb_txd_sl
 *				   &altb_rts_sl &altb_cts_sl
 *				   &altb_ri_sl &altb_dtr_bout_sl
 *				   &altb_dcd_sl &altb_dsr_sl>;
 *			...
 *		};
 *	};
 *
 * Example usage:
 *      const struct npcx_alt host_uart_alts[] =
 *                   NPCX_DT_IO_ALT_ITEMS_LIST(nuvoton_npcx_host_uart, 0);
 * @param io_comp compatible string in devicetree file for io-pads device
 * @param inst instance number for compatible defined in io_comp.
 * @return an array of npcx_alt items.
 */
#define NPCX_DT_IO_ALT_ITEMS_LIST(io_comp, inst) {             \
	UTIL_LISTIFY(NPCX_DT_IO_ALT_ITEMS_LEN(io_comp, inst),  \
		     NPCX_DT_IO_ALT_ITEMS_FUNC,                \
		     inst, io_comp)                            \
	}

/**
 * @brief Get phandle from "name" property which contains wui information.
 *
 * @param inst instance number for compatible defined in DT_DRV_COMPAT.
 * @param name property 'name' which type is 'phandle' and contains wui info.
 * @return phandle from 'name' property.
 */
#define NPCX_DT_PHANDLE_FROM_WUI_NAME(inst, name) \
	DT_INST_PHANDLE(inst, name)

/**
 * @brief Construct a npcx_wui structure from 'name' property
 *
 * @param inst instance number for compatible defined in DT_DRV_COMPAT.
 * @param name property 'name'which type is 'phandle' and contains wui info.
 * @return npcx_wui item from 'name' property.
 */
#define NPCX_DT_WUI_ITEM_BY_NAME(inst, name)				       \
	{                                                                      \
	  .table = DT_PROP(DT_PHANDLE(NPCX_DT_PHANDLE_FROM_WUI_NAME(inst,      \
					name), miwus), index),                 \
	  .group = DT_PHA(NPCX_DT_PHANDLE_FROM_WUI_NAME(inst, name), miwus,    \
					group),                                \
	  .bit   = DT_PHA(NPCX_DT_PHANDLE_FROM_WUI_NAME(inst, name), miwus,    \
					bit),                                  \
	}

/**
 * @brief Get phandle from 'wui_maps' prop which type is 'phandles' at index 'i'
 *
 * @param inst instance number for compatible defined in DT_DRV_COMPAT.
 * @param i index of 'wui_maps' prop which type is 'phandles'
 * @return phandle from 'wui_maps' prop at index 'i'
 */
#define NPCX_DT_PHANDLE_FROM_WUI_MAPS(inst, i) \
	DT_INST_PHANDLE_BY_IDX(inst, wui_maps, i)

/**
 * @brief Construct a npcx_wui structure from wui_maps property at index 'i'
 *
 * @param inst instance number for compatible defined in DT_DRV_COMPAT.
 * @param i index of 'wui_maps' prop which type is 'phandles'
 * @return npcx_wui item at index 'i'
 */
#define NPCX_DT_WUI_ITEM_BY_IDX(inst, i) \
	{                                                                      \
	  .table = DT_PROP(DT_PHANDLE(NPCX_DT_PHANDLE_FROM_WUI_MAPS(inst, i),  \
					miwus), index),                        \
	  .group = DT_PHA(NPCX_DT_PHANDLE_FROM_WUI_MAPS(inst, i), miwus,       \
							group),                \
	  .bit = DT_PHA(NPCX_DT_PHANDLE_FROM_WUI_MAPS(inst, i), miwus, bit),   \
	},

/**
 * @brief Length of npcx_wui structures in 'wui_maps' property
 *
 * @param inst instance number for compatible defined in DT_DRV_COMPAT.
 * @return length of 'wui_maps' prop which type is 'phandles'
 */
#define NPCX_DT_WUI_ITEMS_LEN(inst) DT_INST_PROP_LEN(inst, wui_maps)

/**
 * @brief Macro function to construct a list of npcx_wui items by UTIL_LISTIFY
 *
 * @param child child index in UTIL_LISTIFY extension.
 * @param inst instance number for compatible defined in DT_DRV_COMPAT.
 * @return macro function to construct a npcx_wui structure.
 */
#define NPCX_DT_WUI_ITEMS_FUNC(child, inst) NPCX_DT_WUI_ITEM_BY_IDX(inst, child)

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
 * Example usage:
 * const struct npcx_wui wui_map = NPCX_DT_PHANDLE_FROM_WUI_NAME(inst, uart_rx);
 * const struct npcx_wui wui_maps[] = NPCX_DT_WUI_ITEMS_LIST(inst);
 *
 * @param inst instance number for compatible defined in DT_DRV_COMPAT.
 * @return an array of npcx_wui items.
 */
#define NPCX_DT_WUI_ITEMS_LIST(inst) {             \
	UTIL_LISTIFY(NPCX_DT_WUI_ITEMS_LEN(inst),  \
		     NPCX_DT_WUI_ITEMS_FUNC,       \
		     inst)                         \
	}

/**
 * @brief Get a node from path '/npcx_miwus_map/map_miwu(0/1/2)_groups'
 *
 * @param i index of npcx miwu devices
 * @return node identifier with that path.
 */
#define NPCX_DT_NODE_FROM_MIWU_MAP(i)  DT_PATH(npcx7_miwus_int_map, \
						map_miwu##i##_groups)
/**
 * @brief Get the index prop from parent MIWU device node.
 *
 * @param child index in UTIL_LISTIFY extension.
 * @return 'index' prop value of the node which compatible type is
 * "nuvoton,npcx-miwu".
 */
#define NPCX_DT_MIWU_IRQ_TABLE_IDX(child) \
	DT_PROP(DT_PHANDLE(DT_PARENT(child), parent), index)

/**
 * @brief Macro function for DT_FOREACH_CHILD to generate a IRQ_CONNECT
 * implementation.
 *
 * @param child index in UTIL_LISTIFY extension.
 * @return implementation to initialize interrupts of MIWU groups and enable
 * them.
 */
#define NPCX_DT_MIWU_IRQ_CONNECT_IMPL_CHILD_FUNC(child) \
	NPCX_DT_MIWU_IRQ_CONNECT_IMPL_CHILD_FUNC_OBJ(child);

#define NPCX_DT_MIWU_IRQ_CONNECT_IMPL_CHILD_FUNC_OBJ(child) \
	do {                                                                   \
		IRQ_CONNECT(DT_PROP(child, irq),		               \
			DT_PROP(child, irq_prio),		               \
			NPCX_MIWU_ISR_FUNC(NPCX_DT_MIWU_IRQ_TABLE_IDX(child)), \
			DT_PROP(child, group_mask),                            \
			0);						       \
		irq_enable(DT_PROP(child, irq));                               \
	} while (0)

/**
 * @brief Get a child node from path '/npcx7_espi_vws_map/name'.
 *
 * @param name a path which name is /npcx7_espi_vws_map/'name'.
 * @return child node identifier with that path.
 */
#define NPCX_DT_NODE_FROM_VWTABLE(name) DT_CHILD(DT_PATH(npcx7_espi_vws_map),  \
									name)

/**
 * @brief Get phandle from wui_map property of child node with that path.
 *
 * @param name path which name is /npcx7_espi_vws_map/'name'.
 * @return phandle from "wui_map" prop of child node with that path.
 */
#define NPCX_DT_PHANDLE_VW_WUI(name) DT_PHANDLE(NPCX_DT_NODE_FROM_VWTABLE(     \
								name), wui_map)

/**
 * @brief Construct a npcx_wui structure from wui_map property of a child node
 * with that path.
 *
 * @param name a path which name is /npcx7_espi_vws_map/'name'.
 * @return npcx_wui item with that path.
 */
#define NPCX_DT_VW_WUI_ITEM(name)			                       \
	{                                                                      \
	  .table = DT_PROP(DT_PHANDLE(NPCX_DT_PHANDLE_VW_WUI(name), miwus),    \
									index),\
	  .group = DT_PHA(NPCX_DT_PHANDLE_VW_WUI(name), miwus, group),         \
	  .bit = DT_PHA(NPCX_DT_PHANDLE_VW_WUI(name), miwus, bit),             \
	}

/**
 * @brief Construct a npcx espi device configuration of vw input signal from
 * a child node with that path.
 *
 * @signal vw input signal name.
 * @param name a path which name is /npcx7_espi_vws_map/'name'.
 * @return npcx_vw_in_config item with that path.
 */
#define NPCX_DT_VW_IN_CONF(signal, name)                                       \
	{                                                                      \
	  .sig = signal,                                                       \
	  .reg_idx = DT_PROP_BY_IDX(NPCX_DT_NODE_FROM_VWTABLE(name), vw_reg,   \
									0),    \
	  .bitmask = DT_PROP_BY_IDX(NPCX_DT_NODE_FROM_VWTABLE(name), vw_reg,   \
									1),    \
	  .vw_wui  = NPCX_DT_VW_WUI_ITEM(name),                                \
	}

/**
 * @brief Construct a npcx espi device configuration of vw output signal from
 * a child node with that path.
 *
 * @signal vw output signal name.
 * @param name a path which name is /npcx7_espi_vws_map/'name'.
 * @return npcx_vw_in_config item with that path.
 */
#define NPCX_DT_VW_OUT_CONF(signal, name)                                      \
	{                                                                      \
	  .sig = signal,                                                       \
	  .reg_idx = DT_PROP_BY_IDX(NPCX_DT_NODE_FROM_VWTABLE(name), vw_reg,   \
									0),    \
	  .bitmask = DT_PROP_BY_IDX(NPCX_DT_NODE_FROM_VWTABLE(name), vw_reg,   \
									1),    \
	}

/**
 * @brief Get a node from path '/def_lvol_io_list' which has a property
 *        'lvol_io_pads' contains low-voltage configurations and need to set
 *        by default.
 *
 * @return node identifier with that path.
 */
#define NPCX_DT_NODE_DEF_LVOL_LIST  DT_PATH(def_lvol_io_list)

/**
 * @brief Length of npcx_lvol structures in 'lvol_io_pads' property
 *
 * @return length of 'lvol_io_pads' prop which type is 'phandles'
 */
#define NPCX_DT_LVOL_ITEMS_LEN DT_PROP_LEN(NPCX_DT_NODE_DEF_LVOL_LIST, \
								lvol_io_pads)

/**
 * @brief Get phandle from 'lvol_io_pads' prop which type is 'phandles' at index
 *        'i'
 *
 * @param i index of 'lvol_io_pads' prop which type is 'phandles'
 * @return phandle from 'lvol_io_pads' prop at index 'i'
 */
#define NPCX_DT_PHANDLE_FROM_LVOL_IO_PADS(i) \
	DT_PHANDLE_BY_IDX(NPCX_DT_NODE_DEF_LVOL_LIST, lvol_io_pads, i)

/**
 * @brief Construct a npcx_lvol structure from 'lvol_io_pads' property at index
 *        'i'.
 *
 * @param i index of 'lvol_io_pads' prop which type is 'phandles'
 * @return npcx_lvol item from 'lvol_io_pads' property at index 'i'
 */
#define NPCX_DT_LVOL_ITEMS_BY_IDX(i, _)                                        \
	{                                                                      \
	  .io_port = DT_PHA(NPCX_DT_PHANDLE_FROM_LVOL_IO_PADS(i),              \
							lvols, io_port),       \
	  .io_bit = DT_PHA(NPCX_DT_PHANDLE_FROM_LVOL_IO_PADS(i),               \
							lvols, io_bit),        \
	  .ctrl = DT_PHA(NPCX_DT_PHANDLE_FROM_LVOL_IO_PADS(i),                 \
							lvols, ctrl),          \
	  .bit = DT_PHA(NPCX_DT_PHANDLE_FROM_LVOL_IO_PADS(i),                  \
							lvols, bit),           \
	},

/**
 * @brief Macro function to construct a list of npcx_lvol items by UTIL_LISTIFY
 *        func.
 *
 * Example devicetree fragment:
 *    / {
 *          def_lvol_io_list {
 *              compatible = "nuvoton,npcx-lvolctrl-def";
 *              lvol_io_pads = <&lvol_io90   // I2C1_SCL0 1.8V support
 *                              &lvol_io87>; // I2C1_SDA0 1,8V support
 *          };
 *	};
 *
 * Example usage:
 * static const struct npcx_lvol def_lvols[] = NPCX_DT_IO_LVOL_ITEMS_DEF_LIST;
 *
 * @return an array of npcx_lvol items which configure low-voltage support
 */
#define NPCX_DT_IO_LVOL_ITEMS_DEF_LIST {                \
		UTIL_LISTIFY(NPCX_DT_LVOL_ITEMS_LEN,    \
			NPCX_DT_LVOL_ITEMS_BY_IDX, _)   \
	}

#endif /* _NUVOTON_NPCX_SOC_DT_H_ */
