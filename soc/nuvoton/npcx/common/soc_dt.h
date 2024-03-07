/*
 * Copyright (c) 2020 Nuvoton Technology Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _NUVOTON_NPCX_SOC_DT_H_
#define _NUVOTON_NPCX_SOC_DT_H_

#include <zephyr/devicetree.h>
#include <zephyr/irq.h>
#include <zephyr/sys/util_macro.h>

/**
 * @brief Like DT_PROP(), but expand parameters with
 *        DT_ENUM_UPPER_TOKEN not DT_PROP
 *
 * If the prop exists, this expands to DT_ENUM_UPPER_TOKEN(node_id, prop).
 * The default_value parameter is not expanded in this case.
 *
 * Otherwise, this expands to default_value.
 *
 * @param node_id node identifier
 * @param prop lowercase-and-underscores property name
 * @param default_value a fallback value to expand to
 * @return the property's enum upper token value or default_value
 */
#define NPCX_DT_PROP_ENUM_OR(node_id, prop, default_value) \
	COND_CODE_1(DT_NODE_HAS_PROP(node_id, prop), \
		    (DT_STRING_UPPER_TOKEN(node_id, prop)), (default_value))

/**
 * @brief Like DT_INST_PROP_OR(), but expand parameters with
 *        NPCX_DT_PROP_ENUM_OR not DT_PROP_OR
 * @param inst instance number
 * @param prop lowercase-and-underscores property name
 * @param default_value a fallback value to expand to
 * @return the property's enum upper token value or default_value
 */
#define NPCX_DT_INST_PROP_ENUM_OR(inst, prop, default_value) \
	NPCX_DT_PROP_ENUM_OR(DT_DRV_INST(inst), prop, default_value)

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
	  .bus  = NPCX_DT_INST_PROP_ENUM_OR(inst, clock_bus,                   \
				DT_PHA(DT_DRV_INST(inst), clocks, bus)),       \
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
	}

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
#define NPCX_DT_CLK_CFG_ITEMS_LIST(inst) {        \
	LISTIFY(NPCX_DT_CLK_CFG_ITEMS_LEN(inst),  \
		NPCX_DT_CLK_CFG_ITEMS_FUNC, (,),  \
		inst)                             \
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
 * @brief Get phandle from 'wui-maps' prop which type is 'phandles' at index 'i'
 *
 * @param inst instance number for compatible defined in DT_DRV_COMPAT.
 * @param i index of 'wui-maps' prop which type is 'phandles'
 * @return phandle from 'wui-maps' prop at index 'i'
 */
#define NPCX_DT_PHANDLE_FROM_WUI_MAPS(inst, i) \
	DT_INST_PHANDLE_BY_IDX(inst, wui_maps, i)

/**
 * @brief Construct a npcx_wui structure from wui-maps property at index 'i'
 *
 * @param inst instance number for compatible defined in DT_DRV_COMPAT.
 * @param i index of 'wui-maps' prop which type is 'phandles'
 * @return npcx_wui item at index 'i'
 */
#define NPCX_DT_WUI_ITEM_BY_IDX(inst, i) \
	{                                                                      \
	  .table = DT_PROP(DT_PHANDLE(NPCX_DT_PHANDLE_FROM_WUI_MAPS(inst, i),  \
					miwus), index),                        \
	  .group = DT_PHA(NPCX_DT_PHANDLE_FROM_WUI_MAPS(inst, i), miwus,       \
							group),                \
	  .bit = DT_PHA(NPCX_DT_PHANDLE_FROM_WUI_MAPS(inst, i), miwus, bit),   \
	}

/**
 * @brief Length of npcx_wui structures in 'wui-maps' property
 *
 * @param inst instance number for compatible defined in DT_DRV_COMPAT.
 * @return length of 'wui-maps' prop which type is 'phandles'
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
 *			uart-rx = <&wui_cr_sin1>;
 *			...
 *		};
 *
 *		gpio0: gpio@40081000 {
 *			wui-maps = <&wui_io00 &wui_io01 &wui_io02 &wui_io03
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
#define NPCX_DT_WUI_ITEMS_LIST(inst) {        \
	LISTIFY(NPCX_DT_WUI_ITEMS_LEN(inst),  \
		NPCX_DT_WUI_ITEMS_FUNC, (,),  \
		inst)                         \
	}

/**
 * @brief Get a node from path '/npcx_miwus_map/map_miwu(0/1/2)_groups'
 *
 * @param i index of npcx miwu devices
 * @return node identifier with that path.
 */
#define NPCX_DT_NODE_FROM_MIWU_MAP(i)  DT_PATH(npcx_miwus_int_map, \
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
	} while (false)

/**
 * @brief Get a child node from path '/npcx-espi-vws-map/name'.
 *
 * @param name a path which name is /npcx-espi-vws-map/'name'.
 * @return child node identifier with that path.
 */
#define NPCX_DT_NODE_FROM_VWTABLE(name) DT_CHILD(DT_PATH(npcx_espi_vws_map),  \
									name)

/**
 * @brief Get phandle from vw-wui property of child node with that path.
 *
 * @param name path which name is /npcx-espi-vws-map/'name'.
 * @return phandle from "vw-wui" prop of child node with that path.
 */
#define NPCX_DT_PHANDLE_VW_WUI(name) DT_PHANDLE(NPCX_DT_NODE_FROM_VWTABLE(     \
								name), vw_wui)

/**
 * @brief Construct a npcx_wui structure from vw-wui property of a child node
 * with that path.
 *
 * @param name a path which name is /npcx-espi-vws-map/'name'.
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
 * @param name a path which name is /npcx-espi-vws-map/'name'.
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
 * @param name a path which name is /npcx-espi-vws-map/'name'.
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
 * @brief Construct a npcx_lvol structure from 'lvol-maps' property at index 'i'.
 *
 * @param node_id Node identifier.
 * @param prop Low voltage configurations property name. (i.e. 'lvol-maps')
 * @param idx Property entry index.
 */
#define NPCX_DT_LVOL_CTRL_NONE \
	DT_PHA(DT_NODELABEL(lvol_none), lvols, ctrl)

/**
 * @brief Length of npcx_lvol structures in 'lvol-maps' property
 *
 * @param inst instance number for compatible defined in DT_DRV_COMPAT.
 * @return length of 'lvol-maps' prop which type is 'phandles'
 */
#define NPCX_DT_LVOL_ITEMS_LEN(inst) DT_INST_PROP_LEN(inst, lvol_maps)

/**
 * @brief Construct a npcx_lvol structure from 'lvol-maps' property at index 'i'.
 *
 * @param node_id Node identifier.
 * @param prop Low voltage configurations property name. (i.e. 'lvol-maps')
 * @param idx Property entry index.
 */
#define NPCX_DT_LVOL_ITEMS_INIT(node_id, prop, idx)				\
	{									\
	  .ctrl = DT_PHA(DT_PROP_BY_IDX(node_id, prop, idx), lvols, ctrl),	\
	  .bit = DT_PHA(DT_PROP_BY_IDX(node_id, prop, idx), lvols, bit),	\
	},

/**
 * @brief Macro function to construct a list of npcx_lvol items  from 'lvol-maps'
 * property.
 *
 * @param inst instance number for compatible defined in DT_DRV_COMPAT.
 * @return an array of npcx_lvol items.
 */
#define NPCX_DT_LVOL_ITEMS_LIST(inst) {						\
	DT_FOREACH_PROP_ELEM(DT_DRV_INST(inst), lvol_maps,			\
						NPCX_DT_LVOL_ITEMS_INIT)}

/**
 * @brief Check if the host interface type is automatically configured by
 * booter.
 *
 * @return TRUE - if the host interface is configured by booter,
 *         FALSE - otherwise.
 */
#define NPCX_BOOTER_IS_HIF_TYPE_SET() \
	DT_PROP(DT_PATH(booter_variant), hif_type_auto)

/**
 * @brief Helper macro to get address of system configuration module which is
 * used by serval peripheral device drivers in npcx series.
 *
 * @return base address of system configuration module.
 */
#define NPCX_SCFG_REG_ADDR DT_REG_ADDR_BY_NAME(DT_NODELABEL(scfg), scfg)

/**
 * @brief Helper macro to get address of system glue module which is
 * used by serval peripheral device drivers in npcx series.
 *
 * @return base address of system glue module.
 */
#define NPCX_GLUE_REG_ADDR DT_REG_ADDR_BY_NAME(DT_NODELABEL(scfg), glue)

#endif /* _NUVOTON_NPCX_SOC_DT_H_ */
