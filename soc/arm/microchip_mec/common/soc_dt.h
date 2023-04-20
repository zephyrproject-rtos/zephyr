/*
 * Copyright (c) 2021 Microchip Technology Inc. All Rights Reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _MICROCHIP_XEC_SOC_DT_H_
#define _MICROCHIP_XEC_SOC_DT_H_

#include <zephyr/devicetree.h>

#define MCHP_XEC_NO_PULL	0x0
#define MCHP_XEC_PULL_UP	0x1
#define MCHP_XEC_PULL_DOWN	0x2
#define MCHP_XEC_REPEATER	0x3
#define MCHP_XEC_PUSH_PULL	0x0
#define MCHP_XEC_OPEN_DRAIN	0x1
#define MCHP_XEC_NO_OVAL	0x0
#define MCHP_XEC_OVAL_LOW	0x1
#define MCHP_XEC_OVAL_HIGH	0x2
#define MCHP_XEC_DRVSTR_NONE	0x0
#define MCHP_XEC_DRVSTR_2MA	0x1
#define MCHP_XEC_DRVSTR_4MA	0x2
#define MCHP_XEC_DRVSTR_8MA	0x3
#define MCHP_XEC_DRVSTR_12MA	0x4
#define MCHP_XEC_FUNC_INVERT	0x1

#define MCHP_DT_ESPI_VW_FLAG_STATUS_POS		0
#define MCHP_DT_ESPI_VW_FLAG_DIR_POS		1
#define MCHP_DT_ESPI_VW_FLAG_RST_STATE_POS	2
#define MCHP_DT_ESPI_VW_FLAG_RST_STATE_MSK0	0x3
#define MCHP_DT_ESPI_VW_FLAG_RST_SRC_POS	4
#define MCHP_DT_ESPI_VW_FLAG_RST_SRC_MSK0	0x7

#define MCHP_DT_NODE_FROM_VWTABLE(name) DT_CHILD(DT_PATH(mchp_xec_espi_vw_routing), name)
#define MCHP_DT_VW_NODE_HAS_STATUS(name) DT_NODE_HAS_STATUS(MCHP_DT_NODE_FROM_VWTABLE(name), okay)

/* Macro to store eSPI virtual wire DT flags
 * b[0] = DT status property 0 is disabled, 1 enabled,
 * b[1] = VW direction 0(EC target to host controller), 1(host controller to EC target)
 * b[3:2] = default virtual wire state 0(HW default), 1(low), 2(high)
 * b[6:4] = virtual wire state reset event:
 *          0(HW default), 1(ESPI_RESET), 2(RESET_SYS), 3(RESET_SIO), 4(PLTRST)
 */
#define MCHP_DT_ESPI_VW_FLAGS(vw)                                                                  \
	((uint8_t)(MCHP_DT_VW_NODE_HAS_STATUS(vw)) & 0x01U) |                                      \
	((((uint8_t)DT_PROP_BY_IDX(MCHP_DT_NODE_FROM_VWTABLE(vw), vw_reg, 1)) & 0x1) << 1) |       \
	((((uint8_t)DT_ENUM_IDX_OR(MCHP_DT_NODE_FROM_VWTABLE(vw), reset_state, 0)) & 0x3) << 2) |  \
	((((uint8_t)DT_ENUM_IDX_OR(MCHP_DT_NODE_FROM_VWTABLE(vw), reset_source, 0)) & 0x7) << 4)

/* Macro for the eSPI driver VW table entries.
 * e is a symbol from enum espi_vwire_signal.
 * vw is a node from the XEC ESPI VW routing file.
 */
#define MCHP_DT_ESPI_VW_ENTRY(e, vw)							\
[(e)] = {										\
	.host_idx = DT_PROP_BY_IDX(MCHP_DT_NODE_FROM_VWTABLE(vw), vw_reg, 0),		\
	.bit = DT_PROP_BY_IDX(MCHP_DT_NODE_FROM_VWTABLE(vw), vw_reg, 3),		\
	.xec_reg_idx = DT_PROP_BY_IDX(MCHP_DT_NODE_FROM_VWTABLE(vw), vw_reg, 2),	\
	.flags = MCHP_DT_ESPI_VW_FLAGS(vw),						\
}

#endif /* _MICROCHIP_XEC_SOC_DT_H_ */
