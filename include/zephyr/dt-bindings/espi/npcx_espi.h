/*
 * Copyright (c) 2020 Nuvoton Technology Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_ESPI_NPCX_ESPI_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_ESPI_NPCX_ESPI_H_

/*
 * Encode virtual wire information into a 16-bit unsigned.
 * index  = bits[7:0], Replacement index number
 * group = bits[11:8], Group number for VWEVMS or VWEVSM
 * dir = bits[13:12], Direction for controller to target or target to controller
 */
#define ESPI_NPCX_VW_EX_VAL(dir, group, index)                                                     \
	(((dir & 0x1) << 12) + ((group & 0xf) << 8) + (index & 0xff))

/* extract specific information from encoded ESPI_NPCX_VW_EX_VAL */
#define ESPI_NPCX_VW_EX_INDEX(e)     ((e) & 0xff)
#define ESPI_NPCX_VW_EX_GROUP_NUM(e) (((e) >> 8) & 0xf)
#define ESPI_NPCX_VW_EX_DIR(e)       (((e) >> 12) & 0x1)

/* eSPI VW Master to Slave Register Index */
#define NPCX_VWEVMS0 0
#define NPCX_VWEVMS1 1
#define NPCX_VWEVMS2 2
#define NPCX_VWEVMS3 3
#define NPCX_VWEVMS4 4
#define NPCX_VWEVMS5 5
#define NPCX_VWEVMS6 6
#define NPCX_VWEVMS7 7
#define NPCX_VWEVMS8 8
#define NPCX_VWEVMS9 9
#define NPCX_VWEVMS10 10
#define NPCX_VWEVMS11 11
#define NPCX_VWEVMS_MAX 12

/* eSPI VW Slave to Master Register Index */
#define NPCX_VWEVSM0 0
#define NPCX_VWEVSM1 1
#define NPCX_VWEVSM2 2
#define NPCX_VWEVSM3 3
#define NPCX_VWEVSM4 4
#define NPCX_VWEVSM5 5
#define NPCX_VWEVSM6 6
#define NPCX_VWEVSM7 7
#define NPCX_VWEVSM8 8
#define NPCX_VWEVSM9 9
#define NPCX_VWEVSM_MAX 10

/* eSPI VW GPIO Slave to Master Register Index */
#define NPCX_VWGPSM0 0
#define NPCX_VWGPSM1 1
#define NPCX_VWGPSM2 2
#define NPCX_VWGPSM3 3
#define NPCX_VWGPSM4 4
#define NPCX_VWGPSM5 5
#define NPCX_VWGPSM6 6
#define NPCX_VWGPSM7 7
#define NPCX_VWGPSM8 8
#define NPCX_VWGPSM9 9
#define NPCX_VWGPSM10 10
#define NPCX_VWGPSM11 11
#define NPCX_VWGPSM12 12
#define NPCX_VWGPSM13 13
#define NPCX_VWGPSM14 14
#define NPCX_VWGPSM15 15

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_ESPI_NPCX_ESPI_H_ */
