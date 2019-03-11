/**
 *
 * Copyright (c) 2019 Microchip Technology Inc. and its subsidiaries.
 *
 * \asf_license_start
 *
 * \page License
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may
 * not use this file except in compliance with the License.
 * You may obtain a copy of the Licence at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an AS IS BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * \asf_license_stop
 *
 */

/** @file espi_vw.h
 *MEC1501 eSPI Virtual Wire definitions
 */
/** @defgroup MEC1501 Peripherals eSPI VW
 */

#include <stdint.h>
#include <stddef.h>

#ifndef _ESPI_VW_H
#define _ESPI_VW_H


/*------------------------------------------------------------------*/


/* Master to Slave VW register: 96-bit (3 32 bit registers) */
/* 32-bit word 0 (bits[31:0]) */
#define ESPI_M2SW0_OFS                  0ul
#define ESPI_M2SW0_IDX_POS              0
#define ESPI_M2SW0_IDX_MASK             0xFFu
#define ESPI_M2SW0_MTOS_SRC_POS         8u
#define ESPI_M2SW0_MTOS_SRC_MASK0       0x03u
#define ESPI_M2SW0_MTOS_SRC_MASK        ((ESPI_VW_M2S_MTOS_SRC_MASK0) << (ESPI_VW_M2S_MTOS_SRC_POS))
#define ESPI_M2SW0_MTOS_SRC_ESPI_RST    ((0ul) << (ESPI_VW_M2S_MTOS_SRC_POS))
#define ESPI_M2SW0_MTOS_SRC_SYS_RST     ((1ul) << (ESPI_VW_M2S_MTOS_SRC_POS))
#define ESPI_M2SW0_MTOS_SRC_SIO_RST     ((2ul) << (ESPI_VW_M2S_MTOS_SRC_POS))
#define ESPI_M2SW0_MTOS_SRC_PLTRST      ((3ul) << (ESPI_VW_M2S_MTOS_SRC_POS))
#define ESPI_M2SW0_MTOS_STATE_POS       12u
#define ESPI_M2SW0_MTOS_STATE_MASK0     0x0Ful
#define ESPI_M2SW0_MTOS_STATE_MASK      ((ESPI_VW_M2S_MTOS_STATE_MASK0) << (ESPI_VW_M2S_MTOS_STATE_POS))
/* 32-bit word 1 (bits[63:32]) */
#define ESPI_M2SW1_OFS                  4ul
#define ESPI_M2SW1_SRC0_SEL_POS         0
#define ESPI_M2SW1_SRC_SEL_MASK0        0x0Ful
#define ESPI_M2SW1_SRC0_SEL_MASK        ((ESPI_M2SW1_SRC_SEL_MASK0) << (ESPI_M2SW1_SRC0_SEL_POS))
#define ESPI_M2SW1_SRC1_SEL_POS         8
#define ESPI_M2SW1_SRC1_SEL_MASK        ((ESPI_M2SW1_SRC_SEL_MASK0) << (ESPI_M2SW1_SRC1_SEL_POS))
#define ESPI_M2SW1_SRC2_SEL_POS         16
#define ESPI_M2SW1_SRC2_SEL_MASK        ((ESPI_M2SW1_SRC_SEL_MASK0) << (ESPI_M2SW1_SRC2_SEL_POS))
#define ESPI_M2SW1_SRC3_SEL_POS         24
#define ESPI_M2SW1_SRC3_SEL_MASK        ((ESPI_M2SW1_SRC_SEL_MASK0) << (ESPI_M2SW1_SRC3_SEL_POS))
/* 0 <= n < 4 */
#define ESPI_M2SW1_SRC_SEL_POS(n)       ((n) << 3)
#define ESPI_M2SW1_SRC_SEL_MASK(n)      ((0x0Ful) << (ESPI_M2SW1_SRC_SEL_POS(n)))
#define ESPI_M2SW1_SRC_SEL_VAL(n, v)    (((uint32_t)(v) & 0x0Ful) << (ESPI_M2SW1_SRC_SEL_POS(n)))
/* 32-bit word 2 (bits[95:64]) */
#define ESPI_M2SW2_OFS                  8ul
#define ESPI_M2SW2_SRC_MASK0            0x0Ful
#define ESPI_M2SW2_SRC0_POS             0
#define ESPI_M2SW2_SRC0_MASK            ((ESPI_M2SW1_SRC_MASK0) << (ESPI_M2SW1_SRC0_POS))
#define ESPI_M2SW2_SRC1_POS             8u
#define ESPI_M2SW2_SRC1_MASK            ((ESPI_M2SW1_SRC_MASK0) << (ESPI_M2SW1_SRC1_POS))
#define ESPI_M2SW2_SRC2_POS             16u
#define ESPI_M2SW2_SRC2_MASK            ((ESPI_M2SW1_SRC_MASK0) << (ESPI_M2SW2_SRC1_POS))
#define ESPI_M2SW2_SRC3_POS             24u
#define ESPI_M2SW2_SRC3_MASK            ((ESPI_M2SW1_SRC_MASK0) << (ESPI_M2SW2_SRC3_POS))
/* 0 <= n < 4 */
#define ESPI_M2SW2_SRC_POS(n)           ((n) << 3)
#define ESPI_M2SW2_SRC_MASK(n)          ((ESPI_M2SW2_SRC_MASK0) << (ESPI_M2SW2_SRC_POS(n)))
#define ESPI_M2SW2_SRC_VAL(n, v)        (((uint32_t)(v) & 0x0Ful) << (ESPI_M2SW2_SRC_POS(n)))

/*
 * Zero based values used for above SRC_SEL fields.
 * These values select the interrupt sensitivity for the VWire.
 * Example: Set SRC1 to Level High
 *
 * r = read MSVW1 register
 * r &= ESPI_M2SW1_SRC_SEL_MASK(1)
 * r |= ESPI_MSVW1_SRC_SEL_VAL(1, ESPI_IRQ_SEL_LVL_HI)
 * write r to MSVW1 register
 */
#define ESPI_IRQ_SEL_LVL_LO         0
#define ESPI_IRQ_SEL_LVL_HI         1
#define ESPI_IRQ_SEL_DIS            4
/* NOTE: Edge trigger modes allow VWires to wake MEC1501 from deep sleep */
#define ESPI_IRQ_SEL_REDGE          0x0D
#define ESPI_IRQ_SEL_FEDGE          0x0E
#define ESPI_IRQ_SEL_BEDGE          0x0F

/* Slave to Master VW register: 64-bit (2 32 bit registers) */
/* 32-bit word 0 (bits[31:0]) */
#define ESPI_S2MW0_OFS                  0
#define ESPI_S2MW0_IDX_POS              0
#define ESPI_S2MW0_IDX_MASK             0xFFu
#define ESPI_S2MW0_STOM_POS             8u
#define ESPI_S2MW0_STOM_SRC_POS         8u
#define ESPI_S2MW0_STOM_MASK0           0xF3u
#define ESPI_S2MW0_STOM_MASK            ((ESPI_S2MW0_STOM_MASK0) << (ESPI_S2MW0_STOM_SRC_POS))
#define ESPI_S2MW0_STOM_SRC_MASK0       0x03ul
#define ESPI_S2MW0_STOM_SRC_MASK        ((ESPI_S2MW0_STOM_SRC_MASK0) << (ESPI_S2MW0_STOM_SRC_POS))
#define ESPI_S2MW0_STOM_SRC_ESPI_RST    ((0ul) << (ESPI_S2MW0_STOM_SRC_POS))
#define ESPI_S2MW0_STOM_SRC_SYS_RST     ((1ul) << (ESPI_S2MW0_STOM_SRC_POS))
#define ESPI_S2MW0_STOM_SRC_SIO_RST     ((2ul) << (ESPI_S2MW0_STOM_SRC_POS))
#define ESPI_S2MW0_STOM_SRC_PLTRST      ((3ul) << (ESPI_S2MW0_STOM_SRC_POS))
#define ESPI_S2MW0_STOM_STATE_POS       12u
#define ESPI_S2MW0_STOM_STATE_MASK0     0x0Ful
#define ESPI_S2MW0_STOM_STATE_MASK      ((ESPI_S2MW0_STOM_STATE_MASK0) << (ESPI_S2MW0_STOM_STATE_POS))
#define ESPI_S2MW0_CHG0_POS             16u
#define ESPI_S2MW0_CHG0                 (1ul << (ESPI_S2MW0_CHG0_POS))
#define ESPI_S2MW0_CHG1_POS             17u
#define ESPI_S2MW0_CHG1                 (1ul << (ESPI_S2MW0_CHG1_POS))
#define ESPI_S2MW0_CHG2_POS             18u
#define ESPI_S2MW0_CHG2                 (1ul << (ESPI_S2MW0_CHG2_POS))
#define ESPI_S2MW0_CHG3_POS             19u
#define ESPI_S2MW0_CHG3                 (1ul << (ESPI_S2MW0_CHG3_POS))
#define ESPI_S2MW0_CHG_ALL_POS          16u
#define ESPI_S2MW0_CHG_ALL_MASK0        0x0Ful
#define ESPI_S2MW0_CHG_ALL_MASK         ((ESPI_S2MW0_CHG_ALL_MASK0) << (ESPI_S2MW0_CHG0_POS))
/* 0 <= n < 4 */
#define ESPI_S2MW1_CHG_POS(n)           ((n) + 16u)
#define ESPI_S2MW1_CHG(v, n)            (((uint32_t)(v) >> ESPI_S2MW1_CHG_POS(n)) & 0x01)

/* 32-bit word 1 (bits[63:32]) */
#define ESPI_S2MW1_OFS                  4ul
#define ESPI_S2MW1_SRC0_POS             0u
#define ESPI_S2MW1_SRC0                 (1ul << (ESPI_S2MW1_SRC0_POS))
#define ESPI_S2MW1_SRC1_POS             8u
#define ESPI_S2MW1_SRC1                 (1ul << (ESPI_S2MW1_SRC1_POS))
#define ESPI_S2MW1_SRC2_POS             16u
#define ESPI_S2MW1_SRC2                 (1ul << (ESPI_S2MW1_SRC2_POS))
#define ESPI_S2MW1_SRC3_POS             24u
#define ESPI_S2MW1_SRC3                 (1ul << (ESPI_S2MW1_SRC3_POS))
/* 0 <= n < 4 */
#define ESPI_S2MW1_SRC_POS(n)           ((n) << 3)
#define ESPI_S2MW1_SRC(v, n)            (((uint32_t)(v) & 0x01) << (ESPI_S2MW1_SRC_POS(n)))

/* =========================================================================*/
/* ================           ESPI_VW                      ================ */
/* =========================================================================*/

/**
  * @brief eSPI Virtual Wires (ESPI_VW)
  */

#define ESPI_MSVW_IDX_MAX   10
#define ESPI_SMVW_IDX_MAX   10

#define ESPI_NUM_MSVW       11
#define ESPI_NUM_SMVW       11

/* Master-to-Slave VW byte indices */
#define MSVW_INDEX          0
#define MSVW_MTOS           1
#define MSVW_SRC0_IRQ_SEL   4
#define MSVW_SRC1_IRQ_SEL   5
#define MSVW_SRC2_IRQ_SEL   6
#define MSVW_SRC3_IRQ_SEL   7
#define MSVW_SRC0           8
#define MSVW_SRC1           9
#define MSVW_SRC2           10
#define MSVW_SRC3           11

/* Slave-to-Master VW byte indices */
#define SMVW_INDEX          0
#define SMVW_STOM           1
#define SMVW_CHANGED        2
#define SMVW_SRC0           4
#define SMVW_SRC1           5
#define SMVW_SRC2           6
#define SMVW_SRC3           7

/*
 * ESPI_IO_VW - eSPI IO component registers related to VW channel @ 0x400F36B0
 */
typedef struct
{
    __IOM uint32_t  VW_EN_STS;      /*! (@ 0x0000) Virtual Wire Enable Status */
    uint8_t             RSVD1[0x30];
    __IOM uint8_t   VW_CAP;         /*! (@ 0x0034) Virtual Wire Chan Capabilities */
    uint8_t             RSVD2[8];
    __IOM uint8_t   VW_RDY;         /*! (@ 0x003D) VW ready */
    uint8_t             RSVD3[0x102];
    __IOM uint32_t  VW_ERR_STS;     /*! (@ 0x0140) IO Virtual Wire Error */
} ESPI_IO_VW;

/* Master-to-Slave Virtual Wire 96-bit register */
typedef struct espi_msvw_reg
{
    __IOM uint8_t  INDEX;
    __IOM uint8_t  MTOS;
          uint8_t  RSVD1[2];
    __IOM uint8_t  SRC_IRQ_SEL[4];
    __IOM uint8_t  SRC[4];    
} ESPI_MSVW_REG;

typedef struct
{
    ESPI_MSVW_REG   M2S[ESPI_NUM_MSVW];
} ESPI_M2S_VW;

/* Slave-to-Master Virtual Wire 64-bit register */
typedef struct espi_smvw_reg
{
    __IOM uint8_t  INDEX;
    __IOM uint8_t  STOM;
    __IM  uint8_t  SRC_CHG;
          uint8_t  RSVD1[1];
    __IOM uint8_t  SRC[4];
} ESPI_SMVW_REG;

typedef struct
{
    ESPI_SMVW_REG   S2M[ESPI_NUM_SMVW];
} ESPI_S2M_VW;


#endif /* #ifndef _ESPI_VW_H */
/* end espi_vw.h */
/**   @}
 */

