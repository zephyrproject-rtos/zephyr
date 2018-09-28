/***************************************************************************//**
* \file cyip_gpio_v2.h
*
* \brief
* GPIO IP definitions
*
* \note
* Generator version: 1.3.0.1146
* Database revision: rev#1050929
*
********************************************************************************
* \copyright
* Copyright 2016-2018, Cypress Semiconductor Corporation. All rights reserved.
* SPDX-License-Identifier: Apache-2.0



*******************************************************************************/

#ifndef _CYIP_GPIO_V2_H_
#define _CYIP_GPIO_V2_H_

#include "cyip_headers.h"

/*******************************************************************************
*                                     GPIO
*******************************************************************************/

#define GPIO_PRT_V2_SECTION_SIZE                0x00000080UL
#define GPIO_V2_SECTION_SIZE                    0x00010000UL

/**
  * \brief GPIO port registers (GPIO_PRT)
  */
typedef struct {
  __IOM uint32_t OUT;                           /*!< 0x00000000 Port output data register */
  __IOM uint32_t OUT_CLR;                       /*!< 0x00000004 Port output data set register */
  __IOM uint32_t OUT_SET;                       /*!< 0x00000008 Port output data clear register */
  __IOM uint32_t OUT_INV;                       /*!< 0x0000000C Port output data invert register */
   __IM uint32_t IN;                            /*!< 0x00000010 Port input state register */
  __IOM uint32_t INTR;                          /*!< 0x00000014 Port interrupt status register */
  __IOM uint32_t INTR_MASK;                     /*!< 0x00000018 Port interrupt mask register */
   __IM uint32_t INTR_MASKED;                   /*!< 0x0000001C Port interrupt masked status register */
  __IOM uint32_t INTR_SET;                      /*!< 0x00000020 Port interrupt set register */
   __IM uint32_t RESERVED[7];
  __IOM uint32_t INTR_CFG;                      /*!< 0x00000040 Port interrupt configuration register */
  __IOM uint32_t CFG;                           /*!< 0x00000044 Port configuration register */
  __IOM uint32_t CFG_IN;                        /*!< 0x00000048 Port input buffer configuration register */
  __IOM uint32_t CFG_OUT;                       /*!< 0x0000004C Port output buffer configuration register */
  __IOM uint32_t CFG_SIO;                       /*!< 0x00000050 Port SIO configuration register */
   __IM uint32_t RESERVED1;
  __IOM uint32_t CFG_IN_AUTOLVL;                /*!< 0x00000058 Port GPIO5V input buffer configuration register */
   __IM uint32_t RESERVED2[9];
} GPIO_PRT_V2_Type;                             /*!< Size = 128 (0x80) */

/**
  * \brief GPIO port control/configuration (GPIO)
  */
typedef struct {
        GPIO_PRT_V2_Type PRT[128];              /*!< 0x00000000 GPIO port registers */
   __IM uint32_t INTR_CAUSE0;                   /*!< 0x00004000 Interrupt port cause register 0 */
   __IM uint32_t INTR_CAUSE1;                   /*!< 0x00004004 Interrupt port cause register 1 */
   __IM uint32_t INTR_CAUSE2;                   /*!< 0x00004008 Interrupt port cause register 2 */
   __IM uint32_t INTR_CAUSE3;                   /*!< 0x0000400C Interrupt port cause register 3 */
   __IM uint32_t VDD_ACTIVE;                    /*!< 0x00004010 Extern power supply detection register */
  __IOM uint32_t VDD_INTR;                      /*!< 0x00004014 Supply detection interrupt register */
  __IOM uint32_t VDD_INTR_MASK;                 /*!< 0x00004018 Supply detection interrupt mask register */
   __IM uint32_t VDD_INTR_MASKED;               /*!< 0x0000401C Supply detection interrupt masked register */
  __IOM uint32_t VDD_INTR_SET;                  /*!< 0x00004020 Supply detection interrupt set register */
} GPIO_V2_Type;                                 /*!< Size = 16420 (0x4024) */


/* GPIO_PRT.OUT */
#define GPIO_PRT_V2_OUT_OUT0_Pos                0UL
#define GPIO_PRT_V2_OUT_OUT0_Msk                0x1UL
#define GPIO_PRT_V2_OUT_OUT1_Pos                1UL
#define GPIO_PRT_V2_OUT_OUT1_Msk                0x2UL
#define GPIO_PRT_V2_OUT_OUT2_Pos                2UL
#define GPIO_PRT_V2_OUT_OUT2_Msk                0x4UL
#define GPIO_PRT_V2_OUT_OUT3_Pos                3UL
#define GPIO_PRT_V2_OUT_OUT3_Msk                0x8UL
#define GPIO_PRT_V2_OUT_OUT4_Pos                4UL
#define GPIO_PRT_V2_OUT_OUT4_Msk                0x10UL
#define GPIO_PRT_V2_OUT_OUT5_Pos                5UL
#define GPIO_PRT_V2_OUT_OUT5_Msk                0x20UL
#define GPIO_PRT_V2_OUT_OUT6_Pos                6UL
#define GPIO_PRT_V2_OUT_OUT6_Msk                0x40UL
#define GPIO_PRT_V2_OUT_OUT7_Pos                7UL
#define GPIO_PRT_V2_OUT_OUT7_Msk                0x80UL
/* GPIO_PRT.OUT_CLR */
#define GPIO_PRT_V2_OUT_CLR_OUT0_Pos            0UL
#define GPIO_PRT_V2_OUT_CLR_OUT0_Msk            0x1UL
#define GPIO_PRT_V2_OUT_CLR_OUT1_Pos            1UL
#define GPIO_PRT_V2_OUT_CLR_OUT1_Msk            0x2UL
#define GPIO_PRT_V2_OUT_CLR_OUT2_Pos            2UL
#define GPIO_PRT_V2_OUT_CLR_OUT2_Msk            0x4UL
#define GPIO_PRT_V2_OUT_CLR_OUT3_Pos            3UL
#define GPIO_PRT_V2_OUT_CLR_OUT3_Msk            0x8UL
#define GPIO_PRT_V2_OUT_CLR_OUT4_Pos            4UL
#define GPIO_PRT_V2_OUT_CLR_OUT4_Msk            0x10UL
#define GPIO_PRT_V2_OUT_CLR_OUT5_Pos            5UL
#define GPIO_PRT_V2_OUT_CLR_OUT5_Msk            0x20UL
#define GPIO_PRT_V2_OUT_CLR_OUT6_Pos            6UL
#define GPIO_PRT_V2_OUT_CLR_OUT6_Msk            0x40UL
#define GPIO_PRT_V2_OUT_CLR_OUT7_Pos            7UL
#define GPIO_PRT_V2_OUT_CLR_OUT7_Msk            0x80UL
/* GPIO_PRT.OUT_SET */
#define GPIO_PRT_V2_OUT_SET_OUT0_Pos            0UL
#define GPIO_PRT_V2_OUT_SET_OUT0_Msk            0x1UL
#define GPIO_PRT_V2_OUT_SET_OUT1_Pos            1UL
#define GPIO_PRT_V2_OUT_SET_OUT1_Msk            0x2UL
#define GPIO_PRT_V2_OUT_SET_OUT2_Pos            2UL
#define GPIO_PRT_V2_OUT_SET_OUT2_Msk            0x4UL
#define GPIO_PRT_V2_OUT_SET_OUT3_Pos            3UL
#define GPIO_PRT_V2_OUT_SET_OUT3_Msk            0x8UL
#define GPIO_PRT_V2_OUT_SET_OUT4_Pos            4UL
#define GPIO_PRT_V2_OUT_SET_OUT4_Msk            0x10UL
#define GPIO_PRT_V2_OUT_SET_OUT5_Pos            5UL
#define GPIO_PRT_V2_OUT_SET_OUT5_Msk            0x20UL
#define GPIO_PRT_V2_OUT_SET_OUT6_Pos            6UL
#define GPIO_PRT_V2_OUT_SET_OUT6_Msk            0x40UL
#define GPIO_PRT_V2_OUT_SET_OUT7_Pos            7UL
#define GPIO_PRT_V2_OUT_SET_OUT7_Msk            0x80UL
/* GPIO_PRT.OUT_INV */
#define GPIO_PRT_V2_OUT_INV_OUT0_Pos            0UL
#define GPIO_PRT_V2_OUT_INV_OUT0_Msk            0x1UL
#define GPIO_PRT_V2_OUT_INV_OUT1_Pos            1UL
#define GPIO_PRT_V2_OUT_INV_OUT1_Msk            0x2UL
#define GPIO_PRT_V2_OUT_INV_OUT2_Pos            2UL
#define GPIO_PRT_V2_OUT_INV_OUT2_Msk            0x4UL
#define GPIO_PRT_V2_OUT_INV_OUT3_Pos            3UL
#define GPIO_PRT_V2_OUT_INV_OUT3_Msk            0x8UL
#define GPIO_PRT_V2_OUT_INV_OUT4_Pos            4UL
#define GPIO_PRT_V2_OUT_INV_OUT4_Msk            0x10UL
#define GPIO_PRT_V2_OUT_INV_OUT5_Pos            5UL
#define GPIO_PRT_V2_OUT_INV_OUT5_Msk            0x20UL
#define GPIO_PRT_V2_OUT_INV_OUT6_Pos            6UL
#define GPIO_PRT_V2_OUT_INV_OUT6_Msk            0x40UL
#define GPIO_PRT_V2_OUT_INV_OUT7_Pos            7UL
#define GPIO_PRT_V2_OUT_INV_OUT7_Msk            0x80UL
/* GPIO_PRT.IN */
#define GPIO_PRT_V2_IN_IN0_Pos                  0UL
#define GPIO_PRT_V2_IN_IN0_Msk                  0x1UL
#define GPIO_PRT_V2_IN_IN1_Pos                  1UL
#define GPIO_PRT_V2_IN_IN1_Msk                  0x2UL
#define GPIO_PRT_V2_IN_IN2_Pos                  2UL
#define GPIO_PRT_V2_IN_IN2_Msk                  0x4UL
#define GPIO_PRT_V2_IN_IN3_Pos                  3UL
#define GPIO_PRT_V2_IN_IN3_Msk                  0x8UL
#define GPIO_PRT_V2_IN_IN4_Pos                  4UL
#define GPIO_PRT_V2_IN_IN4_Msk                  0x10UL
#define GPIO_PRT_V2_IN_IN5_Pos                  5UL
#define GPIO_PRT_V2_IN_IN5_Msk                  0x20UL
#define GPIO_PRT_V2_IN_IN6_Pos                  6UL
#define GPIO_PRT_V2_IN_IN6_Msk                  0x40UL
#define GPIO_PRT_V2_IN_IN7_Pos                  7UL
#define GPIO_PRT_V2_IN_IN7_Msk                  0x80UL
#define GPIO_PRT_V2_IN_FLT_IN_Pos               8UL
#define GPIO_PRT_V2_IN_FLT_IN_Msk               0x100UL
/* GPIO_PRT.INTR */
#define GPIO_PRT_V2_INTR_EDGE0_Pos              0UL
#define GPIO_PRT_V2_INTR_EDGE0_Msk              0x1UL
#define GPIO_PRT_V2_INTR_EDGE1_Pos              1UL
#define GPIO_PRT_V2_INTR_EDGE1_Msk              0x2UL
#define GPIO_PRT_V2_INTR_EDGE2_Pos              2UL
#define GPIO_PRT_V2_INTR_EDGE2_Msk              0x4UL
#define GPIO_PRT_V2_INTR_EDGE3_Pos              3UL
#define GPIO_PRT_V2_INTR_EDGE3_Msk              0x8UL
#define GPIO_PRT_V2_INTR_EDGE4_Pos              4UL
#define GPIO_PRT_V2_INTR_EDGE4_Msk              0x10UL
#define GPIO_PRT_V2_INTR_EDGE5_Pos              5UL
#define GPIO_PRT_V2_INTR_EDGE5_Msk              0x20UL
#define GPIO_PRT_V2_INTR_EDGE6_Pos              6UL
#define GPIO_PRT_V2_INTR_EDGE6_Msk              0x40UL
#define GPIO_PRT_V2_INTR_EDGE7_Pos              7UL
#define GPIO_PRT_V2_INTR_EDGE7_Msk              0x80UL
#define GPIO_PRT_V2_INTR_FLT_EDGE_Pos           8UL
#define GPIO_PRT_V2_INTR_FLT_EDGE_Msk           0x100UL
#define GPIO_PRT_V2_INTR_IN_IN0_Pos             16UL
#define GPIO_PRT_V2_INTR_IN_IN0_Msk             0x10000UL
#define GPIO_PRT_V2_INTR_IN_IN1_Pos             17UL
#define GPIO_PRT_V2_INTR_IN_IN1_Msk             0x20000UL
#define GPIO_PRT_V2_INTR_IN_IN2_Pos             18UL
#define GPIO_PRT_V2_INTR_IN_IN2_Msk             0x40000UL
#define GPIO_PRT_V2_INTR_IN_IN3_Pos             19UL
#define GPIO_PRT_V2_INTR_IN_IN3_Msk             0x80000UL
#define GPIO_PRT_V2_INTR_IN_IN4_Pos             20UL
#define GPIO_PRT_V2_INTR_IN_IN4_Msk             0x100000UL
#define GPIO_PRT_V2_INTR_IN_IN5_Pos             21UL
#define GPIO_PRT_V2_INTR_IN_IN5_Msk             0x200000UL
#define GPIO_PRT_V2_INTR_IN_IN6_Pos             22UL
#define GPIO_PRT_V2_INTR_IN_IN6_Msk             0x400000UL
#define GPIO_PRT_V2_INTR_IN_IN7_Pos             23UL
#define GPIO_PRT_V2_INTR_IN_IN7_Msk             0x800000UL
#define GPIO_PRT_V2_INTR_FLT_IN_IN_Pos          24UL
#define GPIO_PRT_V2_INTR_FLT_IN_IN_Msk          0x1000000UL
/* GPIO_PRT.INTR_MASK */
#define GPIO_PRT_V2_INTR_MASK_EDGE0_Pos         0UL
#define GPIO_PRT_V2_INTR_MASK_EDGE0_Msk         0x1UL
#define GPIO_PRT_V2_INTR_MASK_EDGE1_Pos         1UL
#define GPIO_PRT_V2_INTR_MASK_EDGE1_Msk         0x2UL
#define GPIO_PRT_V2_INTR_MASK_EDGE2_Pos         2UL
#define GPIO_PRT_V2_INTR_MASK_EDGE2_Msk         0x4UL
#define GPIO_PRT_V2_INTR_MASK_EDGE3_Pos         3UL
#define GPIO_PRT_V2_INTR_MASK_EDGE3_Msk         0x8UL
#define GPIO_PRT_V2_INTR_MASK_EDGE4_Pos         4UL
#define GPIO_PRT_V2_INTR_MASK_EDGE4_Msk         0x10UL
#define GPIO_PRT_V2_INTR_MASK_EDGE5_Pos         5UL
#define GPIO_PRT_V2_INTR_MASK_EDGE5_Msk         0x20UL
#define GPIO_PRT_V2_INTR_MASK_EDGE6_Pos         6UL
#define GPIO_PRT_V2_INTR_MASK_EDGE6_Msk         0x40UL
#define GPIO_PRT_V2_INTR_MASK_EDGE7_Pos         7UL
#define GPIO_PRT_V2_INTR_MASK_EDGE7_Msk         0x80UL
#define GPIO_PRT_V2_INTR_MASK_FLT_EDGE_Pos      8UL
#define GPIO_PRT_V2_INTR_MASK_FLT_EDGE_Msk      0x100UL
/* GPIO_PRT.INTR_MASKED */
#define GPIO_PRT_V2_INTR_MASKED_EDGE0_Pos       0UL
#define GPIO_PRT_V2_INTR_MASKED_EDGE0_Msk       0x1UL
#define GPIO_PRT_V2_INTR_MASKED_EDGE1_Pos       1UL
#define GPIO_PRT_V2_INTR_MASKED_EDGE1_Msk       0x2UL
#define GPIO_PRT_V2_INTR_MASKED_EDGE2_Pos       2UL
#define GPIO_PRT_V2_INTR_MASKED_EDGE2_Msk       0x4UL
#define GPIO_PRT_V2_INTR_MASKED_EDGE3_Pos       3UL
#define GPIO_PRT_V2_INTR_MASKED_EDGE3_Msk       0x8UL
#define GPIO_PRT_V2_INTR_MASKED_EDGE4_Pos       4UL
#define GPIO_PRT_V2_INTR_MASKED_EDGE4_Msk       0x10UL
#define GPIO_PRT_V2_INTR_MASKED_EDGE5_Pos       5UL
#define GPIO_PRT_V2_INTR_MASKED_EDGE5_Msk       0x20UL
#define GPIO_PRT_V2_INTR_MASKED_EDGE6_Pos       6UL
#define GPIO_PRT_V2_INTR_MASKED_EDGE6_Msk       0x40UL
#define GPIO_PRT_V2_INTR_MASKED_EDGE7_Pos       7UL
#define GPIO_PRT_V2_INTR_MASKED_EDGE7_Msk       0x80UL
#define GPIO_PRT_V2_INTR_MASKED_FLT_EDGE_Pos    8UL
#define GPIO_PRT_V2_INTR_MASKED_FLT_EDGE_Msk    0x100UL
/* GPIO_PRT.INTR_SET */
#define GPIO_PRT_V2_INTR_SET_EDGE0_Pos          0UL
#define GPIO_PRT_V2_INTR_SET_EDGE0_Msk          0x1UL
#define GPIO_PRT_V2_INTR_SET_EDGE1_Pos          1UL
#define GPIO_PRT_V2_INTR_SET_EDGE1_Msk          0x2UL
#define GPIO_PRT_V2_INTR_SET_EDGE2_Pos          2UL
#define GPIO_PRT_V2_INTR_SET_EDGE2_Msk          0x4UL
#define GPIO_PRT_V2_INTR_SET_EDGE3_Pos          3UL
#define GPIO_PRT_V2_INTR_SET_EDGE3_Msk          0x8UL
#define GPIO_PRT_V2_INTR_SET_EDGE4_Pos          4UL
#define GPIO_PRT_V2_INTR_SET_EDGE4_Msk          0x10UL
#define GPIO_PRT_V2_INTR_SET_EDGE5_Pos          5UL
#define GPIO_PRT_V2_INTR_SET_EDGE5_Msk          0x20UL
#define GPIO_PRT_V2_INTR_SET_EDGE6_Pos          6UL
#define GPIO_PRT_V2_INTR_SET_EDGE6_Msk          0x40UL
#define GPIO_PRT_V2_INTR_SET_EDGE7_Pos          7UL
#define GPIO_PRT_V2_INTR_SET_EDGE7_Msk          0x80UL
#define GPIO_PRT_V2_INTR_SET_FLT_EDGE_Pos       8UL
#define GPIO_PRT_V2_INTR_SET_FLT_EDGE_Msk       0x100UL
/* GPIO_PRT.INTR_CFG */
#define GPIO_PRT_V2_INTR_CFG_EDGE0_SEL_Pos      0UL
#define GPIO_PRT_V2_INTR_CFG_EDGE0_SEL_Msk      0x3UL
#define GPIO_PRT_V2_INTR_CFG_EDGE1_SEL_Pos      2UL
#define GPIO_PRT_V2_INTR_CFG_EDGE1_SEL_Msk      0xCUL
#define GPIO_PRT_V2_INTR_CFG_EDGE2_SEL_Pos      4UL
#define GPIO_PRT_V2_INTR_CFG_EDGE2_SEL_Msk      0x30UL
#define GPIO_PRT_V2_INTR_CFG_EDGE3_SEL_Pos      6UL
#define GPIO_PRT_V2_INTR_CFG_EDGE3_SEL_Msk      0xC0UL
#define GPIO_PRT_V2_INTR_CFG_EDGE4_SEL_Pos      8UL
#define GPIO_PRT_V2_INTR_CFG_EDGE4_SEL_Msk      0x300UL
#define GPIO_PRT_V2_INTR_CFG_EDGE5_SEL_Pos      10UL
#define GPIO_PRT_V2_INTR_CFG_EDGE5_SEL_Msk      0xC00UL
#define GPIO_PRT_V2_INTR_CFG_EDGE6_SEL_Pos      12UL
#define GPIO_PRT_V2_INTR_CFG_EDGE6_SEL_Msk      0x3000UL
#define GPIO_PRT_V2_INTR_CFG_EDGE7_SEL_Pos      14UL
#define GPIO_PRT_V2_INTR_CFG_EDGE7_SEL_Msk      0xC000UL
#define GPIO_PRT_V2_INTR_CFG_FLT_EDGE_SEL_Pos   16UL
#define GPIO_PRT_V2_INTR_CFG_FLT_EDGE_SEL_Msk   0x30000UL
#define GPIO_PRT_V2_INTR_CFG_FLT_SEL_Pos        18UL
#define GPIO_PRT_V2_INTR_CFG_FLT_SEL_Msk        0x1C0000UL
/* GPIO_PRT.CFG */
#define GPIO_PRT_V2_CFG_DRIVE_MODE0_Pos         0UL
#define GPIO_PRT_V2_CFG_DRIVE_MODE0_Msk         0x7UL
#define GPIO_PRT_V2_CFG_IN_EN0_Pos              3UL
#define GPIO_PRT_V2_CFG_IN_EN0_Msk              0x8UL
#define GPIO_PRT_V2_CFG_DRIVE_MODE1_Pos         4UL
#define GPIO_PRT_V2_CFG_DRIVE_MODE1_Msk         0x70UL
#define GPIO_PRT_V2_CFG_IN_EN1_Pos              7UL
#define GPIO_PRT_V2_CFG_IN_EN1_Msk              0x80UL
#define GPIO_PRT_V2_CFG_DRIVE_MODE2_Pos         8UL
#define GPIO_PRT_V2_CFG_DRIVE_MODE2_Msk         0x700UL
#define GPIO_PRT_V2_CFG_IN_EN2_Pos              11UL
#define GPIO_PRT_V2_CFG_IN_EN2_Msk              0x800UL
#define GPIO_PRT_V2_CFG_DRIVE_MODE3_Pos         12UL
#define GPIO_PRT_V2_CFG_DRIVE_MODE3_Msk         0x7000UL
#define GPIO_PRT_V2_CFG_IN_EN3_Pos              15UL
#define GPIO_PRT_V2_CFG_IN_EN3_Msk              0x8000UL
#define GPIO_PRT_V2_CFG_DRIVE_MODE4_Pos         16UL
#define GPIO_PRT_V2_CFG_DRIVE_MODE4_Msk         0x70000UL
#define GPIO_PRT_V2_CFG_IN_EN4_Pos              19UL
#define GPIO_PRT_V2_CFG_IN_EN4_Msk              0x80000UL
#define GPIO_PRT_V2_CFG_DRIVE_MODE5_Pos         20UL
#define GPIO_PRT_V2_CFG_DRIVE_MODE5_Msk         0x700000UL
#define GPIO_PRT_V2_CFG_IN_EN5_Pos              23UL
#define GPIO_PRT_V2_CFG_IN_EN5_Msk              0x800000UL
#define GPIO_PRT_V2_CFG_DRIVE_MODE6_Pos         24UL
#define GPIO_PRT_V2_CFG_DRIVE_MODE6_Msk         0x7000000UL
#define GPIO_PRT_V2_CFG_IN_EN6_Pos              27UL
#define GPIO_PRT_V2_CFG_IN_EN6_Msk              0x8000000UL
#define GPIO_PRT_V2_CFG_DRIVE_MODE7_Pos         28UL
#define GPIO_PRT_V2_CFG_DRIVE_MODE7_Msk         0x70000000UL
#define GPIO_PRT_V2_CFG_IN_EN7_Pos              31UL
#define GPIO_PRT_V2_CFG_IN_EN7_Msk              0x80000000UL
/* GPIO_PRT.CFG_IN */
#define GPIO_PRT_V2_CFG_IN_VTRIP_SEL0_0_Pos     0UL
#define GPIO_PRT_V2_CFG_IN_VTRIP_SEL0_0_Msk     0x1UL
#define GPIO_PRT_V2_CFG_IN_VTRIP_SEL1_0_Pos     1UL
#define GPIO_PRT_V2_CFG_IN_VTRIP_SEL1_0_Msk     0x2UL
#define GPIO_PRT_V2_CFG_IN_VTRIP_SEL2_0_Pos     2UL
#define GPIO_PRT_V2_CFG_IN_VTRIP_SEL2_0_Msk     0x4UL
#define GPIO_PRT_V2_CFG_IN_VTRIP_SEL3_0_Pos     3UL
#define GPIO_PRT_V2_CFG_IN_VTRIP_SEL3_0_Msk     0x8UL
#define GPIO_PRT_V2_CFG_IN_VTRIP_SEL4_0_Pos     4UL
#define GPIO_PRT_V2_CFG_IN_VTRIP_SEL4_0_Msk     0x10UL
#define GPIO_PRT_V2_CFG_IN_VTRIP_SEL5_0_Pos     5UL
#define GPIO_PRT_V2_CFG_IN_VTRIP_SEL5_0_Msk     0x20UL
#define GPIO_PRT_V2_CFG_IN_VTRIP_SEL6_0_Pos     6UL
#define GPIO_PRT_V2_CFG_IN_VTRIP_SEL6_0_Msk     0x40UL
#define GPIO_PRT_V2_CFG_IN_VTRIP_SEL7_0_Pos     7UL
#define GPIO_PRT_V2_CFG_IN_VTRIP_SEL7_0_Msk     0x80UL
/* GPIO_PRT.CFG_OUT */
#define GPIO_PRT_V2_CFG_OUT_SLOW0_Pos           0UL
#define GPIO_PRT_V2_CFG_OUT_SLOW0_Msk           0x1UL
#define GPIO_PRT_V2_CFG_OUT_SLOW1_Pos           1UL
#define GPIO_PRT_V2_CFG_OUT_SLOW1_Msk           0x2UL
#define GPIO_PRT_V2_CFG_OUT_SLOW2_Pos           2UL
#define GPIO_PRT_V2_CFG_OUT_SLOW2_Msk           0x4UL
#define GPIO_PRT_V2_CFG_OUT_SLOW3_Pos           3UL
#define GPIO_PRT_V2_CFG_OUT_SLOW3_Msk           0x8UL
#define GPIO_PRT_V2_CFG_OUT_SLOW4_Pos           4UL
#define GPIO_PRT_V2_CFG_OUT_SLOW4_Msk           0x10UL
#define GPIO_PRT_V2_CFG_OUT_SLOW5_Pos           5UL
#define GPIO_PRT_V2_CFG_OUT_SLOW5_Msk           0x20UL
#define GPIO_PRT_V2_CFG_OUT_SLOW6_Pos           6UL
#define GPIO_PRT_V2_CFG_OUT_SLOW6_Msk           0x40UL
#define GPIO_PRT_V2_CFG_OUT_SLOW7_Pos           7UL
#define GPIO_PRT_V2_CFG_OUT_SLOW7_Msk           0x80UL
#define GPIO_PRT_V2_CFG_OUT_DRIVE_SEL0_Pos      16UL
#define GPIO_PRT_V2_CFG_OUT_DRIVE_SEL0_Msk      0x30000UL
#define GPIO_PRT_V2_CFG_OUT_DRIVE_SEL1_Pos      18UL
#define GPIO_PRT_V2_CFG_OUT_DRIVE_SEL1_Msk      0xC0000UL
#define GPIO_PRT_V2_CFG_OUT_DRIVE_SEL2_Pos      20UL
#define GPIO_PRT_V2_CFG_OUT_DRIVE_SEL2_Msk      0x300000UL
#define GPIO_PRT_V2_CFG_OUT_DRIVE_SEL3_Pos      22UL
#define GPIO_PRT_V2_CFG_OUT_DRIVE_SEL3_Msk      0xC00000UL
#define GPIO_PRT_V2_CFG_OUT_DRIVE_SEL4_Pos      24UL
#define GPIO_PRT_V2_CFG_OUT_DRIVE_SEL4_Msk      0x3000000UL
#define GPIO_PRT_V2_CFG_OUT_DRIVE_SEL5_Pos      26UL
#define GPIO_PRT_V2_CFG_OUT_DRIVE_SEL5_Msk      0xC000000UL
#define GPIO_PRT_V2_CFG_OUT_DRIVE_SEL6_Pos      28UL
#define GPIO_PRT_V2_CFG_OUT_DRIVE_SEL6_Msk      0x30000000UL
#define GPIO_PRT_V2_CFG_OUT_DRIVE_SEL7_Pos      30UL
#define GPIO_PRT_V2_CFG_OUT_DRIVE_SEL7_Msk      0xC0000000UL
/* GPIO_PRT.CFG_SIO */
#define GPIO_PRT_V2_CFG_SIO_VREG_EN01_Pos       0UL
#define GPIO_PRT_V2_CFG_SIO_VREG_EN01_Msk       0x1UL
#define GPIO_PRT_V2_CFG_SIO_IBUF_SEL01_Pos      1UL
#define GPIO_PRT_V2_CFG_SIO_IBUF_SEL01_Msk      0x2UL
#define GPIO_PRT_V2_CFG_SIO_VTRIP_SEL01_Pos     2UL
#define GPIO_PRT_V2_CFG_SIO_VTRIP_SEL01_Msk     0x4UL
#define GPIO_PRT_V2_CFG_SIO_VREF_SEL01_Pos      3UL
#define GPIO_PRT_V2_CFG_SIO_VREF_SEL01_Msk      0x18UL
#define GPIO_PRT_V2_CFG_SIO_VOH_SEL01_Pos       5UL
#define GPIO_PRT_V2_CFG_SIO_VOH_SEL01_Msk       0xE0UL
#define GPIO_PRT_V2_CFG_SIO_VREG_EN23_Pos       8UL
#define GPIO_PRT_V2_CFG_SIO_VREG_EN23_Msk       0x100UL
#define GPIO_PRT_V2_CFG_SIO_IBUF_SEL23_Pos      9UL
#define GPIO_PRT_V2_CFG_SIO_IBUF_SEL23_Msk      0x200UL
#define GPIO_PRT_V2_CFG_SIO_VTRIP_SEL23_Pos     10UL
#define GPIO_PRT_V2_CFG_SIO_VTRIP_SEL23_Msk     0x400UL
#define GPIO_PRT_V2_CFG_SIO_VREF_SEL23_Pos      11UL
#define GPIO_PRT_V2_CFG_SIO_VREF_SEL23_Msk      0x1800UL
#define GPIO_PRT_V2_CFG_SIO_VOH_SEL23_Pos       13UL
#define GPIO_PRT_V2_CFG_SIO_VOH_SEL23_Msk       0xE000UL
#define GPIO_PRT_V2_CFG_SIO_VREG_EN45_Pos       16UL
#define GPIO_PRT_V2_CFG_SIO_VREG_EN45_Msk       0x10000UL
#define GPIO_PRT_V2_CFG_SIO_IBUF_SEL45_Pos      17UL
#define GPIO_PRT_V2_CFG_SIO_IBUF_SEL45_Msk      0x20000UL
#define GPIO_PRT_V2_CFG_SIO_VTRIP_SEL45_Pos     18UL
#define GPIO_PRT_V2_CFG_SIO_VTRIP_SEL45_Msk     0x40000UL
#define GPIO_PRT_V2_CFG_SIO_VREF_SEL45_Pos      19UL
#define GPIO_PRT_V2_CFG_SIO_VREF_SEL45_Msk      0x180000UL
#define GPIO_PRT_V2_CFG_SIO_VOH_SEL45_Pos       21UL
#define GPIO_PRT_V2_CFG_SIO_VOH_SEL45_Msk       0xE00000UL
#define GPIO_PRT_V2_CFG_SIO_VREG_EN67_Pos       24UL
#define GPIO_PRT_V2_CFG_SIO_VREG_EN67_Msk       0x1000000UL
#define GPIO_PRT_V2_CFG_SIO_IBUF_SEL67_Pos      25UL
#define GPIO_PRT_V2_CFG_SIO_IBUF_SEL67_Msk      0x2000000UL
#define GPIO_PRT_V2_CFG_SIO_VTRIP_SEL67_Pos     26UL
#define GPIO_PRT_V2_CFG_SIO_VTRIP_SEL67_Msk     0x4000000UL
#define GPIO_PRT_V2_CFG_SIO_VREF_SEL67_Pos      27UL
#define GPIO_PRT_V2_CFG_SIO_VREF_SEL67_Msk      0x18000000UL
#define GPIO_PRT_V2_CFG_SIO_VOH_SEL67_Pos       29UL
#define GPIO_PRT_V2_CFG_SIO_VOH_SEL67_Msk       0xE0000000UL
/* GPIO_PRT.CFG_IN_AUTOLVL */
#define GPIO_PRT_V2_CFG_IN_AUTOLVL_VTRIP_SEL0_1_Pos 0UL
#define GPIO_PRT_V2_CFG_IN_AUTOLVL_VTRIP_SEL0_1_Msk 0x1UL
#define GPIO_PRT_V2_CFG_IN_AUTOLVL_VTRIP_SEL1_1_Pos 1UL
#define GPIO_PRT_V2_CFG_IN_AUTOLVL_VTRIP_SEL1_1_Msk 0x2UL
#define GPIO_PRT_V2_CFG_IN_AUTOLVL_VTRIP_SEL2_1_Pos 2UL
#define GPIO_PRT_V2_CFG_IN_AUTOLVL_VTRIP_SEL2_1_Msk 0x4UL
#define GPIO_PRT_V2_CFG_IN_AUTOLVL_VTRIP_SEL3_1_Pos 3UL
#define GPIO_PRT_V2_CFG_IN_AUTOLVL_VTRIP_SEL3_1_Msk 0x8UL
#define GPIO_PRT_V2_CFG_IN_AUTOLVL_VTRIP_SEL4_1_Pos 4UL
#define GPIO_PRT_V2_CFG_IN_AUTOLVL_VTRIP_SEL4_1_Msk 0x10UL
#define GPIO_PRT_V2_CFG_IN_AUTOLVL_VTRIP_SEL5_1_Pos 5UL
#define GPIO_PRT_V2_CFG_IN_AUTOLVL_VTRIP_SEL5_1_Msk 0x20UL
#define GPIO_PRT_V2_CFG_IN_AUTOLVL_VTRIP_SEL6_1_Pos 6UL
#define GPIO_PRT_V2_CFG_IN_AUTOLVL_VTRIP_SEL6_1_Msk 0x40UL
#define GPIO_PRT_V2_CFG_IN_AUTOLVL_VTRIP_SEL7_1_Pos 7UL
#define GPIO_PRT_V2_CFG_IN_AUTOLVL_VTRIP_SEL7_1_Msk 0x80UL


/* GPIO.INTR_CAUSE0 */
#define GPIO_V2_INTR_CAUSE0_PORT_INT_Pos        0UL
#define GPIO_V2_INTR_CAUSE0_PORT_INT_Msk        0xFFFFFFFFUL
/* GPIO.INTR_CAUSE1 */
#define GPIO_V2_INTR_CAUSE1_PORT_INT_Pos        0UL
#define GPIO_V2_INTR_CAUSE1_PORT_INT_Msk        0xFFFFFFFFUL
/* GPIO.INTR_CAUSE2 */
#define GPIO_V2_INTR_CAUSE2_PORT_INT_Pos        0UL
#define GPIO_V2_INTR_CAUSE2_PORT_INT_Msk        0xFFFFFFFFUL
/* GPIO.INTR_CAUSE3 */
#define GPIO_V2_INTR_CAUSE3_PORT_INT_Pos        0UL
#define GPIO_V2_INTR_CAUSE3_PORT_INT_Msk        0xFFFFFFFFUL
/* GPIO.VDD_ACTIVE */
#define GPIO_V2_VDD_ACTIVE_VDDIO_ACTIVE_Pos     0UL
#define GPIO_V2_VDD_ACTIVE_VDDIO_ACTIVE_Msk     0xFFFFUL
#define GPIO_V2_VDD_ACTIVE_VDDA_ACTIVE_Pos      30UL
#define GPIO_V2_VDD_ACTIVE_VDDA_ACTIVE_Msk      0x40000000UL
#define GPIO_V2_VDD_ACTIVE_VDDD_ACTIVE_Pos      31UL
#define GPIO_V2_VDD_ACTIVE_VDDD_ACTIVE_Msk      0x80000000UL
/* GPIO.VDD_INTR */
#define GPIO_V2_VDD_INTR_VDDIO_ACTIVE_Pos       0UL
#define GPIO_V2_VDD_INTR_VDDIO_ACTIVE_Msk       0xFFFFUL
#define GPIO_V2_VDD_INTR_VDDA_ACTIVE_Pos        30UL
#define GPIO_V2_VDD_INTR_VDDA_ACTIVE_Msk        0x40000000UL
#define GPIO_V2_VDD_INTR_VDDD_ACTIVE_Pos        31UL
#define GPIO_V2_VDD_INTR_VDDD_ACTIVE_Msk        0x80000000UL
/* GPIO.VDD_INTR_MASK */
#define GPIO_V2_VDD_INTR_MASK_VDDIO_ACTIVE_Pos  0UL
#define GPIO_V2_VDD_INTR_MASK_VDDIO_ACTIVE_Msk  0xFFFFUL
#define GPIO_V2_VDD_INTR_MASK_VDDA_ACTIVE_Pos   30UL
#define GPIO_V2_VDD_INTR_MASK_VDDA_ACTIVE_Msk   0x40000000UL
#define GPIO_V2_VDD_INTR_MASK_VDDD_ACTIVE_Pos   31UL
#define GPIO_V2_VDD_INTR_MASK_VDDD_ACTIVE_Msk   0x80000000UL
/* GPIO.VDD_INTR_MASKED */
#define GPIO_V2_VDD_INTR_MASKED_VDDIO_ACTIVE_Pos 0UL
#define GPIO_V2_VDD_INTR_MASKED_VDDIO_ACTIVE_Msk 0xFFFFUL
#define GPIO_V2_VDD_INTR_MASKED_VDDA_ACTIVE_Pos 30UL
#define GPIO_V2_VDD_INTR_MASKED_VDDA_ACTIVE_Msk 0x40000000UL
#define GPIO_V2_VDD_INTR_MASKED_VDDD_ACTIVE_Pos 31UL
#define GPIO_V2_VDD_INTR_MASKED_VDDD_ACTIVE_Msk 0x80000000UL
/* GPIO.VDD_INTR_SET */
#define GPIO_V2_VDD_INTR_SET_VDDIO_ACTIVE_Pos   0UL
#define GPIO_V2_VDD_INTR_SET_VDDIO_ACTIVE_Msk   0xFFFFUL
#define GPIO_V2_VDD_INTR_SET_VDDA_ACTIVE_Pos    30UL
#define GPIO_V2_VDD_INTR_SET_VDDA_ACTIVE_Msk    0x40000000UL
#define GPIO_V2_VDD_INTR_SET_VDDD_ACTIVE_Pos    31UL
#define GPIO_V2_VDD_INTR_SET_VDDD_ACTIVE_Msk    0x80000000UL


#endif /* _CYIP_GPIO_V2_H_ */


/* [] END OF FILE */
