/*
 * Copyright (c) 2026 Nuvoton Technology Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _NUVOTON_NPCM_SOC_DT_H_
#define _NUVOTON_NPCM_SOC_DT_H_

#include <zephyr/devicetree.h>
#include <zephyr/sys/util_macro.h>

/**
 * @brief Construct a npcm_wui structure from a node of the wake-up input map
 *
 * @param wui_node node identifier of a child of '/npcm-miwus-wui-map'.
 * @return npcm_wui item of that node.
 */
#define NPCM_DT_WUI_ITEM(wui_node)                                                                 \
	{                                                                                          \
		.table = DT_PROP(DT_PHANDLE(wui_node, miwus), index),                              \
		.group = DT_PHA(wui_node, miwus, group),                                           \
		.bit = DT_PHA(wui_node, miwus, bit),                                               \
	}

/**
 * @brief Construct a npcm_wui structure from 'name' property
 *
 * @param inst instance number for compatible defined in DT_DRV_COMPAT.
 * @param name property 'name' which type is 'phandle' and contains wui info.
 * @return npcm_wui item from 'name' property.
 */
#define NPCM_DT_WUI_ITEM_BY_NAME(inst, name) NPCM_DT_WUI_ITEM(DT_INST_PHANDLE(inst, name))

/**
 * @brief Construct a npcm_wui structure from wui-maps property at index 'i'
 *
 * @param inst instance number for compatible defined in DT_DRV_COMPAT.
 * @param i index of 'wui-maps' prop which type is 'phandles'
 * @return npcm_wui item at index 'i'
 */
#define NPCM_DT_WUI_ITEM_BY_IDX(inst, i) NPCM_DT_WUI_ITEM(DT_INST_PHANDLE_BY_IDX(inst, wui_maps, i))

/**
 * @brief Length of npcm_wui structures in 'wui-maps' property
 *
 * @param inst instance number for compatible defined in DT_DRV_COMPAT.
 * @return length of 'wui-maps' prop which type is 'phandles'
 */
#define NPCM_DT_WUI_ITEMS_LEN(inst) DT_INST_PROP_LEN(inst, wui_maps)

/**
 * @brief Macro function to construct a list of npcm_wui items by LISTIFY
 *
 * @param i index in LISTIFY extension.
 * @param inst instance number for compatible defined in DT_DRV_COMPAT.
 * @return macro function to construct a npcm_wui structure.
 */
#define NPCM_DT_WUI_ITEMS_FUNC(i, inst) NPCM_DT_WUI_ITEM_BY_IDX(inst, i)

/**
 * @brief Construct an array of npcm_wui items from 'wui-maps' property
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
 * const struct npcm_wui wui_map = NPCM_DT_WUI_ITEM_BY_NAME(inst, uart_rx);
 * const struct npcm_wui wui_maps[] = NPCM_DT_WUI_ITEMS_LIST(inst);
 *
 * @param inst instance number for compatible defined in DT_DRV_COMPAT.
 * @return an array of npcm_wui items.
 */
#define NPCM_DT_WUI_ITEMS_LIST(inst)                                                               \
	{LISTIFY(NPCM_DT_WUI_ITEMS_LEN(inst), NPCM_DT_WUI_ITEMS_FUNC, (,), inst) }

#endif /* _NUVOTON_NPCM_SOC_DT_H_ */
