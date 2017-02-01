/**
 * \file
 *
 * \brief Component description for PIO
 *
 * Copyright (c) 2016 Atmel Corporation, a wholly owned subsidiary of Microchip Technology Inc.
 *
 * \license_start
 *
 * \page License
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * \license_stop
 *
 */

#ifndef _SAME70_PIO_COMPONENT_H_
#define _SAME70_PIO_COMPONENT_H_
#define _SAME70_PIO_COMPONENT_         /**< \deprecated  Backward compatibility for ASF */

/** \addtogroup SAME70_PIO Parallel Input/Output Controller
 *  @{
 */
/* ========================================================================== */
/**  SOFTWARE API DEFINITION FOR PIO */
/* ========================================================================== */
#ifndef COMPONENT_TYPEDEF_STYLE
  #define COMPONENT_TYPEDEF_STYLE 'R'  /**< Defines default style of typedefs for the component header files ('R' = RFO, 'N' = NTO)*/
#endif

#define PIO_11004                      /**< (PIO) Module ID */
#define REV_PIO U                      /**< (PIO) Module revision */

/* -------- PIO_PER : (PIO Offset: 0x00) (/W 32) PIO Enable Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t P0:1;                      /**< bit:      0  PIO Enable                               */
    uint32_t P1:1;                      /**< bit:      1  PIO Enable                               */
    uint32_t P2:1;                      /**< bit:      2  PIO Enable                               */
    uint32_t P3:1;                      /**< bit:      3  PIO Enable                               */
    uint32_t P4:1;                      /**< bit:      4  PIO Enable                               */
    uint32_t P5:1;                      /**< bit:      5  PIO Enable                               */
    uint32_t P6:1;                      /**< bit:      6  PIO Enable                               */
    uint32_t P7:1;                      /**< bit:      7  PIO Enable                               */
    uint32_t P8:1;                      /**< bit:      8  PIO Enable                               */
    uint32_t P9:1;                      /**< bit:      9  PIO Enable                               */
    uint32_t P10:1;                     /**< bit:     10  PIO Enable                               */
    uint32_t P11:1;                     /**< bit:     11  PIO Enable                               */
    uint32_t P12:1;                     /**< bit:     12  PIO Enable                               */
    uint32_t P13:1;                     /**< bit:     13  PIO Enable                               */
    uint32_t P14:1;                     /**< bit:     14  PIO Enable                               */
    uint32_t P15:1;                     /**< bit:     15  PIO Enable                               */
    uint32_t P16:1;                     /**< bit:     16  PIO Enable                               */
    uint32_t P17:1;                     /**< bit:     17  PIO Enable                               */
    uint32_t P18:1;                     /**< bit:     18  PIO Enable                               */
    uint32_t P19:1;                     /**< bit:     19  PIO Enable                               */
    uint32_t P20:1;                     /**< bit:     20  PIO Enable                               */
    uint32_t P21:1;                     /**< bit:     21  PIO Enable                               */
    uint32_t P22:1;                     /**< bit:     22  PIO Enable                               */
    uint32_t P23:1;                     /**< bit:     23  PIO Enable                               */
    uint32_t P24:1;                     /**< bit:     24  PIO Enable                               */
    uint32_t P25:1;                     /**< bit:     25  PIO Enable                               */
    uint32_t P26:1;                     /**< bit:     26  PIO Enable                               */
    uint32_t P27:1;                     /**< bit:     27  PIO Enable                               */
    uint32_t P28:1;                     /**< bit:     28  PIO Enable                               */
    uint32_t P29:1;                     /**< bit:     29  PIO Enable                               */
    uint32_t P30:1;                     /**< bit:     30  PIO Enable                               */
    uint32_t P31:1;                     /**< bit:     31  PIO Enable                               */
  } bit;                                /**< Structure used for bit  access */
  struct {
    uint32_t P:32;                      /**< bit:  0..31  PIO Enable                               */
  } vec;                                /**< Structure used for vec  access  */
  uint32_t reg;                         /**< Type used for register access */
} PIO_PER_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define PIO_PER_OFFSET                      (0x00)                                        /**<  (PIO_PER) PIO Enable Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define PIO_PER_P0_Pos                      0                                              /**< (PIO_PER) PIO Enable Position */
#define PIO_PER_P0_Msk                      (0x1U << PIO_PER_P0_Pos)                       /**< (PIO_PER) PIO Enable Mask */
#define PIO_PER_P0                          PIO_PER_P0_Msk                                 /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PER_P0_Msk instead */
#define PIO_PER_P1_Pos                      1                                              /**< (PIO_PER) PIO Enable Position */
#define PIO_PER_P1_Msk                      (0x1U << PIO_PER_P1_Pos)                       /**< (PIO_PER) PIO Enable Mask */
#define PIO_PER_P1                          PIO_PER_P1_Msk                                 /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PER_P1_Msk instead */
#define PIO_PER_P2_Pos                      2                                              /**< (PIO_PER) PIO Enable Position */
#define PIO_PER_P2_Msk                      (0x1U << PIO_PER_P2_Pos)                       /**< (PIO_PER) PIO Enable Mask */
#define PIO_PER_P2                          PIO_PER_P2_Msk                                 /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PER_P2_Msk instead */
#define PIO_PER_P3_Pos                      3                                              /**< (PIO_PER) PIO Enable Position */
#define PIO_PER_P3_Msk                      (0x1U << PIO_PER_P3_Pos)                       /**< (PIO_PER) PIO Enable Mask */
#define PIO_PER_P3                          PIO_PER_P3_Msk                                 /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PER_P3_Msk instead */
#define PIO_PER_P4_Pos                      4                                              /**< (PIO_PER) PIO Enable Position */
#define PIO_PER_P4_Msk                      (0x1U << PIO_PER_P4_Pos)                       /**< (PIO_PER) PIO Enable Mask */
#define PIO_PER_P4                          PIO_PER_P4_Msk                                 /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PER_P4_Msk instead */
#define PIO_PER_P5_Pos                      5                                              /**< (PIO_PER) PIO Enable Position */
#define PIO_PER_P5_Msk                      (0x1U << PIO_PER_P5_Pos)                       /**< (PIO_PER) PIO Enable Mask */
#define PIO_PER_P5                          PIO_PER_P5_Msk                                 /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PER_P5_Msk instead */
#define PIO_PER_P6_Pos                      6                                              /**< (PIO_PER) PIO Enable Position */
#define PIO_PER_P6_Msk                      (0x1U << PIO_PER_P6_Pos)                       /**< (PIO_PER) PIO Enable Mask */
#define PIO_PER_P6                          PIO_PER_P6_Msk                                 /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PER_P6_Msk instead */
#define PIO_PER_P7_Pos                      7                                              /**< (PIO_PER) PIO Enable Position */
#define PIO_PER_P7_Msk                      (0x1U << PIO_PER_P7_Pos)                       /**< (PIO_PER) PIO Enable Mask */
#define PIO_PER_P7                          PIO_PER_P7_Msk                                 /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PER_P7_Msk instead */
#define PIO_PER_P8_Pos                      8                                              /**< (PIO_PER) PIO Enable Position */
#define PIO_PER_P8_Msk                      (0x1U << PIO_PER_P8_Pos)                       /**< (PIO_PER) PIO Enable Mask */
#define PIO_PER_P8                          PIO_PER_P8_Msk                                 /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PER_P8_Msk instead */
#define PIO_PER_P9_Pos                      9                                              /**< (PIO_PER) PIO Enable Position */
#define PIO_PER_P9_Msk                      (0x1U << PIO_PER_P9_Pos)                       /**< (PIO_PER) PIO Enable Mask */
#define PIO_PER_P9                          PIO_PER_P9_Msk                                 /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PER_P9_Msk instead */
#define PIO_PER_P10_Pos                     10                                             /**< (PIO_PER) PIO Enable Position */
#define PIO_PER_P10_Msk                     (0x1U << PIO_PER_P10_Pos)                      /**< (PIO_PER) PIO Enable Mask */
#define PIO_PER_P10                         PIO_PER_P10_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PER_P10_Msk instead */
#define PIO_PER_P11_Pos                     11                                             /**< (PIO_PER) PIO Enable Position */
#define PIO_PER_P11_Msk                     (0x1U << PIO_PER_P11_Pos)                      /**< (PIO_PER) PIO Enable Mask */
#define PIO_PER_P11                         PIO_PER_P11_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PER_P11_Msk instead */
#define PIO_PER_P12_Pos                     12                                             /**< (PIO_PER) PIO Enable Position */
#define PIO_PER_P12_Msk                     (0x1U << PIO_PER_P12_Pos)                      /**< (PIO_PER) PIO Enable Mask */
#define PIO_PER_P12                         PIO_PER_P12_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PER_P12_Msk instead */
#define PIO_PER_P13_Pos                     13                                             /**< (PIO_PER) PIO Enable Position */
#define PIO_PER_P13_Msk                     (0x1U << PIO_PER_P13_Pos)                      /**< (PIO_PER) PIO Enable Mask */
#define PIO_PER_P13                         PIO_PER_P13_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PER_P13_Msk instead */
#define PIO_PER_P14_Pos                     14                                             /**< (PIO_PER) PIO Enable Position */
#define PIO_PER_P14_Msk                     (0x1U << PIO_PER_P14_Pos)                      /**< (PIO_PER) PIO Enable Mask */
#define PIO_PER_P14                         PIO_PER_P14_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PER_P14_Msk instead */
#define PIO_PER_P15_Pos                     15                                             /**< (PIO_PER) PIO Enable Position */
#define PIO_PER_P15_Msk                     (0x1U << PIO_PER_P15_Pos)                      /**< (PIO_PER) PIO Enable Mask */
#define PIO_PER_P15                         PIO_PER_P15_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PER_P15_Msk instead */
#define PIO_PER_P16_Pos                     16                                             /**< (PIO_PER) PIO Enable Position */
#define PIO_PER_P16_Msk                     (0x1U << PIO_PER_P16_Pos)                      /**< (PIO_PER) PIO Enable Mask */
#define PIO_PER_P16                         PIO_PER_P16_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PER_P16_Msk instead */
#define PIO_PER_P17_Pos                     17                                             /**< (PIO_PER) PIO Enable Position */
#define PIO_PER_P17_Msk                     (0x1U << PIO_PER_P17_Pos)                      /**< (PIO_PER) PIO Enable Mask */
#define PIO_PER_P17                         PIO_PER_P17_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PER_P17_Msk instead */
#define PIO_PER_P18_Pos                     18                                             /**< (PIO_PER) PIO Enable Position */
#define PIO_PER_P18_Msk                     (0x1U << PIO_PER_P18_Pos)                      /**< (PIO_PER) PIO Enable Mask */
#define PIO_PER_P18                         PIO_PER_P18_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PER_P18_Msk instead */
#define PIO_PER_P19_Pos                     19                                             /**< (PIO_PER) PIO Enable Position */
#define PIO_PER_P19_Msk                     (0x1U << PIO_PER_P19_Pos)                      /**< (PIO_PER) PIO Enable Mask */
#define PIO_PER_P19                         PIO_PER_P19_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PER_P19_Msk instead */
#define PIO_PER_P20_Pos                     20                                             /**< (PIO_PER) PIO Enable Position */
#define PIO_PER_P20_Msk                     (0x1U << PIO_PER_P20_Pos)                      /**< (PIO_PER) PIO Enable Mask */
#define PIO_PER_P20                         PIO_PER_P20_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PER_P20_Msk instead */
#define PIO_PER_P21_Pos                     21                                             /**< (PIO_PER) PIO Enable Position */
#define PIO_PER_P21_Msk                     (0x1U << PIO_PER_P21_Pos)                      /**< (PIO_PER) PIO Enable Mask */
#define PIO_PER_P21                         PIO_PER_P21_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PER_P21_Msk instead */
#define PIO_PER_P22_Pos                     22                                             /**< (PIO_PER) PIO Enable Position */
#define PIO_PER_P22_Msk                     (0x1U << PIO_PER_P22_Pos)                      /**< (PIO_PER) PIO Enable Mask */
#define PIO_PER_P22                         PIO_PER_P22_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PER_P22_Msk instead */
#define PIO_PER_P23_Pos                     23                                             /**< (PIO_PER) PIO Enable Position */
#define PIO_PER_P23_Msk                     (0x1U << PIO_PER_P23_Pos)                      /**< (PIO_PER) PIO Enable Mask */
#define PIO_PER_P23                         PIO_PER_P23_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PER_P23_Msk instead */
#define PIO_PER_P24_Pos                     24                                             /**< (PIO_PER) PIO Enable Position */
#define PIO_PER_P24_Msk                     (0x1U << PIO_PER_P24_Pos)                      /**< (PIO_PER) PIO Enable Mask */
#define PIO_PER_P24                         PIO_PER_P24_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PER_P24_Msk instead */
#define PIO_PER_P25_Pos                     25                                             /**< (PIO_PER) PIO Enable Position */
#define PIO_PER_P25_Msk                     (0x1U << PIO_PER_P25_Pos)                      /**< (PIO_PER) PIO Enable Mask */
#define PIO_PER_P25                         PIO_PER_P25_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PER_P25_Msk instead */
#define PIO_PER_P26_Pos                     26                                             /**< (PIO_PER) PIO Enable Position */
#define PIO_PER_P26_Msk                     (0x1U << PIO_PER_P26_Pos)                      /**< (PIO_PER) PIO Enable Mask */
#define PIO_PER_P26                         PIO_PER_P26_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PER_P26_Msk instead */
#define PIO_PER_P27_Pos                     27                                             /**< (PIO_PER) PIO Enable Position */
#define PIO_PER_P27_Msk                     (0x1U << PIO_PER_P27_Pos)                      /**< (PIO_PER) PIO Enable Mask */
#define PIO_PER_P27                         PIO_PER_P27_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PER_P27_Msk instead */
#define PIO_PER_P28_Pos                     28                                             /**< (PIO_PER) PIO Enable Position */
#define PIO_PER_P28_Msk                     (0x1U << PIO_PER_P28_Pos)                      /**< (PIO_PER) PIO Enable Mask */
#define PIO_PER_P28                         PIO_PER_P28_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PER_P28_Msk instead */
#define PIO_PER_P29_Pos                     29                                             /**< (PIO_PER) PIO Enable Position */
#define PIO_PER_P29_Msk                     (0x1U << PIO_PER_P29_Pos)                      /**< (PIO_PER) PIO Enable Mask */
#define PIO_PER_P29                         PIO_PER_P29_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PER_P29_Msk instead */
#define PIO_PER_P30_Pos                     30                                             /**< (PIO_PER) PIO Enable Position */
#define PIO_PER_P30_Msk                     (0x1U << PIO_PER_P30_Pos)                      /**< (PIO_PER) PIO Enable Mask */
#define PIO_PER_P30                         PIO_PER_P30_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PER_P30_Msk instead */
#define PIO_PER_P31_Pos                     31                                             /**< (PIO_PER) PIO Enable Position */
#define PIO_PER_P31_Msk                     (0x1U << PIO_PER_P31_Pos)                      /**< (PIO_PER) PIO Enable Mask */
#define PIO_PER_P31                         PIO_PER_P31_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PER_P31_Msk instead */
#define PIO_PER_P_Pos                       0                                              /**< (PIO_PER Position) PIO Enable */
#define PIO_PER_P_Msk                       (0xFFFFFFFFU << PIO_PER_P_Pos)                 /**< (PIO_PER Mask) P */
#define PIO_PER_P(value)                    (PIO_PER_P_Msk & ((value) << PIO_PER_P_Pos))   
#define PIO_PER_MASK                        (0xFFFFFFFFU)                                  /**< \deprecated (PIO_PER) Register MASK  (Use PIO_PER_Msk instead)  */
#define PIO_PER_Msk                         (0xFFFFFFFFU)                                  /**< (PIO_PER) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- PIO_PDR : (PIO Offset: 0x04) (/W 32) PIO Disable Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t P0:1;                      /**< bit:      0  PIO Disable                              */
    uint32_t P1:1;                      /**< bit:      1  PIO Disable                              */
    uint32_t P2:1;                      /**< bit:      2  PIO Disable                              */
    uint32_t P3:1;                      /**< bit:      3  PIO Disable                              */
    uint32_t P4:1;                      /**< bit:      4  PIO Disable                              */
    uint32_t P5:1;                      /**< bit:      5  PIO Disable                              */
    uint32_t P6:1;                      /**< bit:      6  PIO Disable                              */
    uint32_t P7:1;                      /**< bit:      7  PIO Disable                              */
    uint32_t P8:1;                      /**< bit:      8  PIO Disable                              */
    uint32_t P9:1;                      /**< bit:      9  PIO Disable                              */
    uint32_t P10:1;                     /**< bit:     10  PIO Disable                              */
    uint32_t P11:1;                     /**< bit:     11  PIO Disable                              */
    uint32_t P12:1;                     /**< bit:     12  PIO Disable                              */
    uint32_t P13:1;                     /**< bit:     13  PIO Disable                              */
    uint32_t P14:1;                     /**< bit:     14  PIO Disable                              */
    uint32_t P15:1;                     /**< bit:     15  PIO Disable                              */
    uint32_t P16:1;                     /**< bit:     16  PIO Disable                              */
    uint32_t P17:1;                     /**< bit:     17  PIO Disable                              */
    uint32_t P18:1;                     /**< bit:     18  PIO Disable                              */
    uint32_t P19:1;                     /**< bit:     19  PIO Disable                              */
    uint32_t P20:1;                     /**< bit:     20  PIO Disable                              */
    uint32_t P21:1;                     /**< bit:     21  PIO Disable                              */
    uint32_t P22:1;                     /**< bit:     22  PIO Disable                              */
    uint32_t P23:1;                     /**< bit:     23  PIO Disable                              */
    uint32_t P24:1;                     /**< bit:     24  PIO Disable                              */
    uint32_t P25:1;                     /**< bit:     25  PIO Disable                              */
    uint32_t P26:1;                     /**< bit:     26  PIO Disable                              */
    uint32_t P27:1;                     /**< bit:     27  PIO Disable                              */
    uint32_t P28:1;                     /**< bit:     28  PIO Disable                              */
    uint32_t P29:1;                     /**< bit:     29  PIO Disable                              */
    uint32_t P30:1;                     /**< bit:     30  PIO Disable                              */
    uint32_t P31:1;                     /**< bit:     31  PIO Disable                              */
  } bit;                                /**< Structure used for bit  access */
  struct {
    uint32_t P:32;                      /**< bit:  0..31  PIO Disable                              */
  } vec;                                /**< Structure used for vec  access  */
  uint32_t reg;                         /**< Type used for register access */
} PIO_PDR_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define PIO_PDR_OFFSET                      (0x04)                                        /**<  (PIO_PDR) PIO Disable Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define PIO_PDR_P0_Pos                      0                                              /**< (PIO_PDR) PIO Disable Position */
#define PIO_PDR_P0_Msk                      (0x1U << PIO_PDR_P0_Pos)                       /**< (PIO_PDR) PIO Disable Mask */
#define PIO_PDR_P0                          PIO_PDR_P0_Msk                                 /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PDR_P0_Msk instead */
#define PIO_PDR_P1_Pos                      1                                              /**< (PIO_PDR) PIO Disable Position */
#define PIO_PDR_P1_Msk                      (0x1U << PIO_PDR_P1_Pos)                       /**< (PIO_PDR) PIO Disable Mask */
#define PIO_PDR_P1                          PIO_PDR_P1_Msk                                 /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PDR_P1_Msk instead */
#define PIO_PDR_P2_Pos                      2                                              /**< (PIO_PDR) PIO Disable Position */
#define PIO_PDR_P2_Msk                      (0x1U << PIO_PDR_P2_Pos)                       /**< (PIO_PDR) PIO Disable Mask */
#define PIO_PDR_P2                          PIO_PDR_P2_Msk                                 /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PDR_P2_Msk instead */
#define PIO_PDR_P3_Pos                      3                                              /**< (PIO_PDR) PIO Disable Position */
#define PIO_PDR_P3_Msk                      (0x1U << PIO_PDR_P3_Pos)                       /**< (PIO_PDR) PIO Disable Mask */
#define PIO_PDR_P3                          PIO_PDR_P3_Msk                                 /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PDR_P3_Msk instead */
#define PIO_PDR_P4_Pos                      4                                              /**< (PIO_PDR) PIO Disable Position */
#define PIO_PDR_P4_Msk                      (0x1U << PIO_PDR_P4_Pos)                       /**< (PIO_PDR) PIO Disable Mask */
#define PIO_PDR_P4                          PIO_PDR_P4_Msk                                 /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PDR_P4_Msk instead */
#define PIO_PDR_P5_Pos                      5                                              /**< (PIO_PDR) PIO Disable Position */
#define PIO_PDR_P5_Msk                      (0x1U << PIO_PDR_P5_Pos)                       /**< (PIO_PDR) PIO Disable Mask */
#define PIO_PDR_P5                          PIO_PDR_P5_Msk                                 /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PDR_P5_Msk instead */
#define PIO_PDR_P6_Pos                      6                                              /**< (PIO_PDR) PIO Disable Position */
#define PIO_PDR_P6_Msk                      (0x1U << PIO_PDR_P6_Pos)                       /**< (PIO_PDR) PIO Disable Mask */
#define PIO_PDR_P6                          PIO_PDR_P6_Msk                                 /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PDR_P6_Msk instead */
#define PIO_PDR_P7_Pos                      7                                              /**< (PIO_PDR) PIO Disable Position */
#define PIO_PDR_P7_Msk                      (0x1U << PIO_PDR_P7_Pos)                       /**< (PIO_PDR) PIO Disable Mask */
#define PIO_PDR_P7                          PIO_PDR_P7_Msk                                 /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PDR_P7_Msk instead */
#define PIO_PDR_P8_Pos                      8                                              /**< (PIO_PDR) PIO Disable Position */
#define PIO_PDR_P8_Msk                      (0x1U << PIO_PDR_P8_Pos)                       /**< (PIO_PDR) PIO Disable Mask */
#define PIO_PDR_P8                          PIO_PDR_P8_Msk                                 /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PDR_P8_Msk instead */
#define PIO_PDR_P9_Pos                      9                                              /**< (PIO_PDR) PIO Disable Position */
#define PIO_PDR_P9_Msk                      (0x1U << PIO_PDR_P9_Pos)                       /**< (PIO_PDR) PIO Disable Mask */
#define PIO_PDR_P9                          PIO_PDR_P9_Msk                                 /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PDR_P9_Msk instead */
#define PIO_PDR_P10_Pos                     10                                             /**< (PIO_PDR) PIO Disable Position */
#define PIO_PDR_P10_Msk                     (0x1U << PIO_PDR_P10_Pos)                      /**< (PIO_PDR) PIO Disable Mask */
#define PIO_PDR_P10                         PIO_PDR_P10_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PDR_P10_Msk instead */
#define PIO_PDR_P11_Pos                     11                                             /**< (PIO_PDR) PIO Disable Position */
#define PIO_PDR_P11_Msk                     (0x1U << PIO_PDR_P11_Pos)                      /**< (PIO_PDR) PIO Disable Mask */
#define PIO_PDR_P11                         PIO_PDR_P11_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PDR_P11_Msk instead */
#define PIO_PDR_P12_Pos                     12                                             /**< (PIO_PDR) PIO Disable Position */
#define PIO_PDR_P12_Msk                     (0x1U << PIO_PDR_P12_Pos)                      /**< (PIO_PDR) PIO Disable Mask */
#define PIO_PDR_P12                         PIO_PDR_P12_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PDR_P12_Msk instead */
#define PIO_PDR_P13_Pos                     13                                             /**< (PIO_PDR) PIO Disable Position */
#define PIO_PDR_P13_Msk                     (0x1U << PIO_PDR_P13_Pos)                      /**< (PIO_PDR) PIO Disable Mask */
#define PIO_PDR_P13                         PIO_PDR_P13_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PDR_P13_Msk instead */
#define PIO_PDR_P14_Pos                     14                                             /**< (PIO_PDR) PIO Disable Position */
#define PIO_PDR_P14_Msk                     (0x1U << PIO_PDR_P14_Pos)                      /**< (PIO_PDR) PIO Disable Mask */
#define PIO_PDR_P14                         PIO_PDR_P14_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PDR_P14_Msk instead */
#define PIO_PDR_P15_Pos                     15                                             /**< (PIO_PDR) PIO Disable Position */
#define PIO_PDR_P15_Msk                     (0x1U << PIO_PDR_P15_Pos)                      /**< (PIO_PDR) PIO Disable Mask */
#define PIO_PDR_P15                         PIO_PDR_P15_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PDR_P15_Msk instead */
#define PIO_PDR_P16_Pos                     16                                             /**< (PIO_PDR) PIO Disable Position */
#define PIO_PDR_P16_Msk                     (0x1U << PIO_PDR_P16_Pos)                      /**< (PIO_PDR) PIO Disable Mask */
#define PIO_PDR_P16                         PIO_PDR_P16_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PDR_P16_Msk instead */
#define PIO_PDR_P17_Pos                     17                                             /**< (PIO_PDR) PIO Disable Position */
#define PIO_PDR_P17_Msk                     (0x1U << PIO_PDR_P17_Pos)                      /**< (PIO_PDR) PIO Disable Mask */
#define PIO_PDR_P17                         PIO_PDR_P17_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PDR_P17_Msk instead */
#define PIO_PDR_P18_Pos                     18                                             /**< (PIO_PDR) PIO Disable Position */
#define PIO_PDR_P18_Msk                     (0x1U << PIO_PDR_P18_Pos)                      /**< (PIO_PDR) PIO Disable Mask */
#define PIO_PDR_P18                         PIO_PDR_P18_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PDR_P18_Msk instead */
#define PIO_PDR_P19_Pos                     19                                             /**< (PIO_PDR) PIO Disable Position */
#define PIO_PDR_P19_Msk                     (0x1U << PIO_PDR_P19_Pos)                      /**< (PIO_PDR) PIO Disable Mask */
#define PIO_PDR_P19                         PIO_PDR_P19_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PDR_P19_Msk instead */
#define PIO_PDR_P20_Pos                     20                                             /**< (PIO_PDR) PIO Disable Position */
#define PIO_PDR_P20_Msk                     (0x1U << PIO_PDR_P20_Pos)                      /**< (PIO_PDR) PIO Disable Mask */
#define PIO_PDR_P20                         PIO_PDR_P20_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PDR_P20_Msk instead */
#define PIO_PDR_P21_Pos                     21                                             /**< (PIO_PDR) PIO Disable Position */
#define PIO_PDR_P21_Msk                     (0x1U << PIO_PDR_P21_Pos)                      /**< (PIO_PDR) PIO Disable Mask */
#define PIO_PDR_P21                         PIO_PDR_P21_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PDR_P21_Msk instead */
#define PIO_PDR_P22_Pos                     22                                             /**< (PIO_PDR) PIO Disable Position */
#define PIO_PDR_P22_Msk                     (0x1U << PIO_PDR_P22_Pos)                      /**< (PIO_PDR) PIO Disable Mask */
#define PIO_PDR_P22                         PIO_PDR_P22_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PDR_P22_Msk instead */
#define PIO_PDR_P23_Pos                     23                                             /**< (PIO_PDR) PIO Disable Position */
#define PIO_PDR_P23_Msk                     (0x1U << PIO_PDR_P23_Pos)                      /**< (PIO_PDR) PIO Disable Mask */
#define PIO_PDR_P23                         PIO_PDR_P23_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PDR_P23_Msk instead */
#define PIO_PDR_P24_Pos                     24                                             /**< (PIO_PDR) PIO Disable Position */
#define PIO_PDR_P24_Msk                     (0x1U << PIO_PDR_P24_Pos)                      /**< (PIO_PDR) PIO Disable Mask */
#define PIO_PDR_P24                         PIO_PDR_P24_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PDR_P24_Msk instead */
#define PIO_PDR_P25_Pos                     25                                             /**< (PIO_PDR) PIO Disable Position */
#define PIO_PDR_P25_Msk                     (0x1U << PIO_PDR_P25_Pos)                      /**< (PIO_PDR) PIO Disable Mask */
#define PIO_PDR_P25                         PIO_PDR_P25_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PDR_P25_Msk instead */
#define PIO_PDR_P26_Pos                     26                                             /**< (PIO_PDR) PIO Disable Position */
#define PIO_PDR_P26_Msk                     (0x1U << PIO_PDR_P26_Pos)                      /**< (PIO_PDR) PIO Disable Mask */
#define PIO_PDR_P26                         PIO_PDR_P26_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PDR_P26_Msk instead */
#define PIO_PDR_P27_Pos                     27                                             /**< (PIO_PDR) PIO Disable Position */
#define PIO_PDR_P27_Msk                     (0x1U << PIO_PDR_P27_Pos)                      /**< (PIO_PDR) PIO Disable Mask */
#define PIO_PDR_P27                         PIO_PDR_P27_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PDR_P27_Msk instead */
#define PIO_PDR_P28_Pos                     28                                             /**< (PIO_PDR) PIO Disable Position */
#define PIO_PDR_P28_Msk                     (0x1U << PIO_PDR_P28_Pos)                      /**< (PIO_PDR) PIO Disable Mask */
#define PIO_PDR_P28                         PIO_PDR_P28_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PDR_P28_Msk instead */
#define PIO_PDR_P29_Pos                     29                                             /**< (PIO_PDR) PIO Disable Position */
#define PIO_PDR_P29_Msk                     (0x1U << PIO_PDR_P29_Pos)                      /**< (PIO_PDR) PIO Disable Mask */
#define PIO_PDR_P29                         PIO_PDR_P29_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PDR_P29_Msk instead */
#define PIO_PDR_P30_Pos                     30                                             /**< (PIO_PDR) PIO Disable Position */
#define PIO_PDR_P30_Msk                     (0x1U << PIO_PDR_P30_Pos)                      /**< (PIO_PDR) PIO Disable Mask */
#define PIO_PDR_P30                         PIO_PDR_P30_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PDR_P30_Msk instead */
#define PIO_PDR_P31_Pos                     31                                             /**< (PIO_PDR) PIO Disable Position */
#define PIO_PDR_P31_Msk                     (0x1U << PIO_PDR_P31_Pos)                      /**< (PIO_PDR) PIO Disable Mask */
#define PIO_PDR_P31                         PIO_PDR_P31_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PDR_P31_Msk instead */
#define PIO_PDR_P_Pos                       0                                              /**< (PIO_PDR Position) PIO Disable */
#define PIO_PDR_P_Msk                       (0xFFFFFFFFU << PIO_PDR_P_Pos)                 /**< (PIO_PDR Mask) P */
#define PIO_PDR_P(value)                    (PIO_PDR_P_Msk & ((value) << PIO_PDR_P_Pos))   
#define PIO_PDR_MASK                        (0xFFFFFFFFU)                                  /**< \deprecated (PIO_PDR) Register MASK  (Use PIO_PDR_Msk instead)  */
#define PIO_PDR_Msk                         (0xFFFFFFFFU)                                  /**< (PIO_PDR) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- PIO_PSR : (PIO Offset: 0x08) (R/ 32) PIO Status Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t P0:1;                      /**< bit:      0  PIO Status                               */
    uint32_t P1:1;                      /**< bit:      1  PIO Status                               */
    uint32_t P2:1;                      /**< bit:      2  PIO Status                               */
    uint32_t P3:1;                      /**< bit:      3  PIO Status                               */
    uint32_t P4:1;                      /**< bit:      4  PIO Status                               */
    uint32_t P5:1;                      /**< bit:      5  PIO Status                               */
    uint32_t P6:1;                      /**< bit:      6  PIO Status                               */
    uint32_t P7:1;                      /**< bit:      7  PIO Status                               */
    uint32_t P8:1;                      /**< bit:      8  PIO Status                               */
    uint32_t P9:1;                      /**< bit:      9  PIO Status                               */
    uint32_t P10:1;                     /**< bit:     10  PIO Status                               */
    uint32_t P11:1;                     /**< bit:     11  PIO Status                               */
    uint32_t P12:1;                     /**< bit:     12  PIO Status                               */
    uint32_t P13:1;                     /**< bit:     13  PIO Status                               */
    uint32_t P14:1;                     /**< bit:     14  PIO Status                               */
    uint32_t P15:1;                     /**< bit:     15  PIO Status                               */
    uint32_t P16:1;                     /**< bit:     16  PIO Status                               */
    uint32_t P17:1;                     /**< bit:     17  PIO Status                               */
    uint32_t P18:1;                     /**< bit:     18  PIO Status                               */
    uint32_t P19:1;                     /**< bit:     19  PIO Status                               */
    uint32_t P20:1;                     /**< bit:     20  PIO Status                               */
    uint32_t P21:1;                     /**< bit:     21  PIO Status                               */
    uint32_t P22:1;                     /**< bit:     22  PIO Status                               */
    uint32_t P23:1;                     /**< bit:     23  PIO Status                               */
    uint32_t P24:1;                     /**< bit:     24  PIO Status                               */
    uint32_t P25:1;                     /**< bit:     25  PIO Status                               */
    uint32_t P26:1;                     /**< bit:     26  PIO Status                               */
    uint32_t P27:1;                     /**< bit:     27  PIO Status                               */
    uint32_t P28:1;                     /**< bit:     28  PIO Status                               */
    uint32_t P29:1;                     /**< bit:     29  PIO Status                               */
    uint32_t P30:1;                     /**< bit:     30  PIO Status                               */
    uint32_t P31:1;                     /**< bit:     31  PIO Status                               */
  } bit;                                /**< Structure used for bit  access */
  struct {
    uint32_t P:32;                      /**< bit:  0..31  PIO Status                               */
  } vec;                                /**< Structure used for vec  access  */
  uint32_t reg;                         /**< Type used for register access */
} PIO_PSR_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define PIO_PSR_OFFSET                      (0x08)                                        /**<  (PIO_PSR) PIO Status Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define PIO_PSR_P0_Pos                      0                                              /**< (PIO_PSR) PIO Status Position */
#define PIO_PSR_P0_Msk                      (0x1U << PIO_PSR_P0_Pos)                       /**< (PIO_PSR) PIO Status Mask */
#define PIO_PSR_P0                          PIO_PSR_P0_Msk                                 /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PSR_P0_Msk instead */
#define PIO_PSR_P1_Pos                      1                                              /**< (PIO_PSR) PIO Status Position */
#define PIO_PSR_P1_Msk                      (0x1U << PIO_PSR_P1_Pos)                       /**< (PIO_PSR) PIO Status Mask */
#define PIO_PSR_P1                          PIO_PSR_P1_Msk                                 /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PSR_P1_Msk instead */
#define PIO_PSR_P2_Pos                      2                                              /**< (PIO_PSR) PIO Status Position */
#define PIO_PSR_P2_Msk                      (0x1U << PIO_PSR_P2_Pos)                       /**< (PIO_PSR) PIO Status Mask */
#define PIO_PSR_P2                          PIO_PSR_P2_Msk                                 /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PSR_P2_Msk instead */
#define PIO_PSR_P3_Pos                      3                                              /**< (PIO_PSR) PIO Status Position */
#define PIO_PSR_P3_Msk                      (0x1U << PIO_PSR_P3_Pos)                       /**< (PIO_PSR) PIO Status Mask */
#define PIO_PSR_P3                          PIO_PSR_P3_Msk                                 /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PSR_P3_Msk instead */
#define PIO_PSR_P4_Pos                      4                                              /**< (PIO_PSR) PIO Status Position */
#define PIO_PSR_P4_Msk                      (0x1U << PIO_PSR_P4_Pos)                       /**< (PIO_PSR) PIO Status Mask */
#define PIO_PSR_P4                          PIO_PSR_P4_Msk                                 /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PSR_P4_Msk instead */
#define PIO_PSR_P5_Pos                      5                                              /**< (PIO_PSR) PIO Status Position */
#define PIO_PSR_P5_Msk                      (0x1U << PIO_PSR_P5_Pos)                       /**< (PIO_PSR) PIO Status Mask */
#define PIO_PSR_P5                          PIO_PSR_P5_Msk                                 /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PSR_P5_Msk instead */
#define PIO_PSR_P6_Pos                      6                                              /**< (PIO_PSR) PIO Status Position */
#define PIO_PSR_P6_Msk                      (0x1U << PIO_PSR_P6_Pos)                       /**< (PIO_PSR) PIO Status Mask */
#define PIO_PSR_P6                          PIO_PSR_P6_Msk                                 /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PSR_P6_Msk instead */
#define PIO_PSR_P7_Pos                      7                                              /**< (PIO_PSR) PIO Status Position */
#define PIO_PSR_P7_Msk                      (0x1U << PIO_PSR_P7_Pos)                       /**< (PIO_PSR) PIO Status Mask */
#define PIO_PSR_P7                          PIO_PSR_P7_Msk                                 /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PSR_P7_Msk instead */
#define PIO_PSR_P8_Pos                      8                                              /**< (PIO_PSR) PIO Status Position */
#define PIO_PSR_P8_Msk                      (0x1U << PIO_PSR_P8_Pos)                       /**< (PIO_PSR) PIO Status Mask */
#define PIO_PSR_P8                          PIO_PSR_P8_Msk                                 /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PSR_P8_Msk instead */
#define PIO_PSR_P9_Pos                      9                                              /**< (PIO_PSR) PIO Status Position */
#define PIO_PSR_P9_Msk                      (0x1U << PIO_PSR_P9_Pos)                       /**< (PIO_PSR) PIO Status Mask */
#define PIO_PSR_P9                          PIO_PSR_P9_Msk                                 /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PSR_P9_Msk instead */
#define PIO_PSR_P10_Pos                     10                                             /**< (PIO_PSR) PIO Status Position */
#define PIO_PSR_P10_Msk                     (0x1U << PIO_PSR_P10_Pos)                      /**< (PIO_PSR) PIO Status Mask */
#define PIO_PSR_P10                         PIO_PSR_P10_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PSR_P10_Msk instead */
#define PIO_PSR_P11_Pos                     11                                             /**< (PIO_PSR) PIO Status Position */
#define PIO_PSR_P11_Msk                     (0x1U << PIO_PSR_P11_Pos)                      /**< (PIO_PSR) PIO Status Mask */
#define PIO_PSR_P11                         PIO_PSR_P11_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PSR_P11_Msk instead */
#define PIO_PSR_P12_Pos                     12                                             /**< (PIO_PSR) PIO Status Position */
#define PIO_PSR_P12_Msk                     (0x1U << PIO_PSR_P12_Pos)                      /**< (PIO_PSR) PIO Status Mask */
#define PIO_PSR_P12                         PIO_PSR_P12_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PSR_P12_Msk instead */
#define PIO_PSR_P13_Pos                     13                                             /**< (PIO_PSR) PIO Status Position */
#define PIO_PSR_P13_Msk                     (0x1U << PIO_PSR_P13_Pos)                      /**< (PIO_PSR) PIO Status Mask */
#define PIO_PSR_P13                         PIO_PSR_P13_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PSR_P13_Msk instead */
#define PIO_PSR_P14_Pos                     14                                             /**< (PIO_PSR) PIO Status Position */
#define PIO_PSR_P14_Msk                     (0x1U << PIO_PSR_P14_Pos)                      /**< (PIO_PSR) PIO Status Mask */
#define PIO_PSR_P14                         PIO_PSR_P14_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PSR_P14_Msk instead */
#define PIO_PSR_P15_Pos                     15                                             /**< (PIO_PSR) PIO Status Position */
#define PIO_PSR_P15_Msk                     (0x1U << PIO_PSR_P15_Pos)                      /**< (PIO_PSR) PIO Status Mask */
#define PIO_PSR_P15                         PIO_PSR_P15_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PSR_P15_Msk instead */
#define PIO_PSR_P16_Pos                     16                                             /**< (PIO_PSR) PIO Status Position */
#define PIO_PSR_P16_Msk                     (0x1U << PIO_PSR_P16_Pos)                      /**< (PIO_PSR) PIO Status Mask */
#define PIO_PSR_P16                         PIO_PSR_P16_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PSR_P16_Msk instead */
#define PIO_PSR_P17_Pos                     17                                             /**< (PIO_PSR) PIO Status Position */
#define PIO_PSR_P17_Msk                     (0x1U << PIO_PSR_P17_Pos)                      /**< (PIO_PSR) PIO Status Mask */
#define PIO_PSR_P17                         PIO_PSR_P17_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PSR_P17_Msk instead */
#define PIO_PSR_P18_Pos                     18                                             /**< (PIO_PSR) PIO Status Position */
#define PIO_PSR_P18_Msk                     (0x1U << PIO_PSR_P18_Pos)                      /**< (PIO_PSR) PIO Status Mask */
#define PIO_PSR_P18                         PIO_PSR_P18_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PSR_P18_Msk instead */
#define PIO_PSR_P19_Pos                     19                                             /**< (PIO_PSR) PIO Status Position */
#define PIO_PSR_P19_Msk                     (0x1U << PIO_PSR_P19_Pos)                      /**< (PIO_PSR) PIO Status Mask */
#define PIO_PSR_P19                         PIO_PSR_P19_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PSR_P19_Msk instead */
#define PIO_PSR_P20_Pos                     20                                             /**< (PIO_PSR) PIO Status Position */
#define PIO_PSR_P20_Msk                     (0x1U << PIO_PSR_P20_Pos)                      /**< (PIO_PSR) PIO Status Mask */
#define PIO_PSR_P20                         PIO_PSR_P20_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PSR_P20_Msk instead */
#define PIO_PSR_P21_Pos                     21                                             /**< (PIO_PSR) PIO Status Position */
#define PIO_PSR_P21_Msk                     (0x1U << PIO_PSR_P21_Pos)                      /**< (PIO_PSR) PIO Status Mask */
#define PIO_PSR_P21                         PIO_PSR_P21_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PSR_P21_Msk instead */
#define PIO_PSR_P22_Pos                     22                                             /**< (PIO_PSR) PIO Status Position */
#define PIO_PSR_P22_Msk                     (0x1U << PIO_PSR_P22_Pos)                      /**< (PIO_PSR) PIO Status Mask */
#define PIO_PSR_P22                         PIO_PSR_P22_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PSR_P22_Msk instead */
#define PIO_PSR_P23_Pos                     23                                             /**< (PIO_PSR) PIO Status Position */
#define PIO_PSR_P23_Msk                     (0x1U << PIO_PSR_P23_Pos)                      /**< (PIO_PSR) PIO Status Mask */
#define PIO_PSR_P23                         PIO_PSR_P23_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PSR_P23_Msk instead */
#define PIO_PSR_P24_Pos                     24                                             /**< (PIO_PSR) PIO Status Position */
#define PIO_PSR_P24_Msk                     (0x1U << PIO_PSR_P24_Pos)                      /**< (PIO_PSR) PIO Status Mask */
#define PIO_PSR_P24                         PIO_PSR_P24_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PSR_P24_Msk instead */
#define PIO_PSR_P25_Pos                     25                                             /**< (PIO_PSR) PIO Status Position */
#define PIO_PSR_P25_Msk                     (0x1U << PIO_PSR_P25_Pos)                      /**< (PIO_PSR) PIO Status Mask */
#define PIO_PSR_P25                         PIO_PSR_P25_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PSR_P25_Msk instead */
#define PIO_PSR_P26_Pos                     26                                             /**< (PIO_PSR) PIO Status Position */
#define PIO_PSR_P26_Msk                     (0x1U << PIO_PSR_P26_Pos)                      /**< (PIO_PSR) PIO Status Mask */
#define PIO_PSR_P26                         PIO_PSR_P26_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PSR_P26_Msk instead */
#define PIO_PSR_P27_Pos                     27                                             /**< (PIO_PSR) PIO Status Position */
#define PIO_PSR_P27_Msk                     (0x1U << PIO_PSR_P27_Pos)                      /**< (PIO_PSR) PIO Status Mask */
#define PIO_PSR_P27                         PIO_PSR_P27_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PSR_P27_Msk instead */
#define PIO_PSR_P28_Pos                     28                                             /**< (PIO_PSR) PIO Status Position */
#define PIO_PSR_P28_Msk                     (0x1U << PIO_PSR_P28_Pos)                      /**< (PIO_PSR) PIO Status Mask */
#define PIO_PSR_P28                         PIO_PSR_P28_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PSR_P28_Msk instead */
#define PIO_PSR_P29_Pos                     29                                             /**< (PIO_PSR) PIO Status Position */
#define PIO_PSR_P29_Msk                     (0x1U << PIO_PSR_P29_Pos)                      /**< (PIO_PSR) PIO Status Mask */
#define PIO_PSR_P29                         PIO_PSR_P29_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PSR_P29_Msk instead */
#define PIO_PSR_P30_Pos                     30                                             /**< (PIO_PSR) PIO Status Position */
#define PIO_PSR_P30_Msk                     (0x1U << PIO_PSR_P30_Pos)                      /**< (PIO_PSR) PIO Status Mask */
#define PIO_PSR_P30                         PIO_PSR_P30_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PSR_P30_Msk instead */
#define PIO_PSR_P31_Pos                     31                                             /**< (PIO_PSR) PIO Status Position */
#define PIO_PSR_P31_Msk                     (0x1U << PIO_PSR_P31_Pos)                      /**< (PIO_PSR) PIO Status Mask */
#define PIO_PSR_P31                         PIO_PSR_P31_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PSR_P31_Msk instead */
#define PIO_PSR_P_Pos                       0                                              /**< (PIO_PSR Position) PIO Status */
#define PIO_PSR_P_Msk                       (0xFFFFFFFFU << PIO_PSR_P_Pos)                 /**< (PIO_PSR Mask) P */
#define PIO_PSR_P(value)                    (PIO_PSR_P_Msk & ((value) << PIO_PSR_P_Pos))   
#define PIO_PSR_MASK                        (0xFFFFFFFFU)                                  /**< \deprecated (PIO_PSR) Register MASK  (Use PIO_PSR_Msk instead)  */
#define PIO_PSR_Msk                         (0xFFFFFFFFU)                                  /**< (PIO_PSR) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- PIO_OER : (PIO Offset: 0x10) (/W 32) Output Enable Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t P0:1;                      /**< bit:      0  Output Enable                            */
    uint32_t P1:1;                      /**< bit:      1  Output Enable                            */
    uint32_t P2:1;                      /**< bit:      2  Output Enable                            */
    uint32_t P3:1;                      /**< bit:      3  Output Enable                            */
    uint32_t P4:1;                      /**< bit:      4  Output Enable                            */
    uint32_t P5:1;                      /**< bit:      5  Output Enable                            */
    uint32_t P6:1;                      /**< bit:      6  Output Enable                            */
    uint32_t P7:1;                      /**< bit:      7  Output Enable                            */
    uint32_t P8:1;                      /**< bit:      8  Output Enable                            */
    uint32_t P9:1;                      /**< bit:      9  Output Enable                            */
    uint32_t P10:1;                     /**< bit:     10  Output Enable                            */
    uint32_t P11:1;                     /**< bit:     11  Output Enable                            */
    uint32_t P12:1;                     /**< bit:     12  Output Enable                            */
    uint32_t P13:1;                     /**< bit:     13  Output Enable                            */
    uint32_t P14:1;                     /**< bit:     14  Output Enable                            */
    uint32_t P15:1;                     /**< bit:     15  Output Enable                            */
    uint32_t P16:1;                     /**< bit:     16  Output Enable                            */
    uint32_t P17:1;                     /**< bit:     17  Output Enable                            */
    uint32_t P18:1;                     /**< bit:     18  Output Enable                            */
    uint32_t P19:1;                     /**< bit:     19  Output Enable                            */
    uint32_t P20:1;                     /**< bit:     20  Output Enable                            */
    uint32_t P21:1;                     /**< bit:     21  Output Enable                            */
    uint32_t P22:1;                     /**< bit:     22  Output Enable                            */
    uint32_t P23:1;                     /**< bit:     23  Output Enable                            */
    uint32_t P24:1;                     /**< bit:     24  Output Enable                            */
    uint32_t P25:1;                     /**< bit:     25  Output Enable                            */
    uint32_t P26:1;                     /**< bit:     26  Output Enable                            */
    uint32_t P27:1;                     /**< bit:     27  Output Enable                            */
    uint32_t P28:1;                     /**< bit:     28  Output Enable                            */
    uint32_t P29:1;                     /**< bit:     29  Output Enable                            */
    uint32_t P30:1;                     /**< bit:     30  Output Enable                            */
    uint32_t P31:1;                     /**< bit:     31  Output Enable                            */
  } bit;                                /**< Structure used for bit  access */
  struct {
    uint32_t P:32;                      /**< bit:  0..31  Output Enable                            */
  } vec;                                /**< Structure used for vec  access  */
  uint32_t reg;                         /**< Type used for register access */
} PIO_OER_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define PIO_OER_OFFSET                      (0x10)                                        /**<  (PIO_OER) Output Enable Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define PIO_OER_P0_Pos                      0                                              /**< (PIO_OER) Output Enable Position */
#define PIO_OER_P0_Msk                      (0x1U << PIO_OER_P0_Pos)                       /**< (PIO_OER) Output Enable Mask */
#define PIO_OER_P0                          PIO_OER_P0_Msk                                 /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_OER_P0_Msk instead */
#define PIO_OER_P1_Pos                      1                                              /**< (PIO_OER) Output Enable Position */
#define PIO_OER_P1_Msk                      (0x1U << PIO_OER_P1_Pos)                       /**< (PIO_OER) Output Enable Mask */
#define PIO_OER_P1                          PIO_OER_P1_Msk                                 /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_OER_P1_Msk instead */
#define PIO_OER_P2_Pos                      2                                              /**< (PIO_OER) Output Enable Position */
#define PIO_OER_P2_Msk                      (0x1U << PIO_OER_P2_Pos)                       /**< (PIO_OER) Output Enable Mask */
#define PIO_OER_P2                          PIO_OER_P2_Msk                                 /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_OER_P2_Msk instead */
#define PIO_OER_P3_Pos                      3                                              /**< (PIO_OER) Output Enable Position */
#define PIO_OER_P3_Msk                      (0x1U << PIO_OER_P3_Pos)                       /**< (PIO_OER) Output Enable Mask */
#define PIO_OER_P3                          PIO_OER_P3_Msk                                 /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_OER_P3_Msk instead */
#define PIO_OER_P4_Pos                      4                                              /**< (PIO_OER) Output Enable Position */
#define PIO_OER_P4_Msk                      (0x1U << PIO_OER_P4_Pos)                       /**< (PIO_OER) Output Enable Mask */
#define PIO_OER_P4                          PIO_OER_P4_Msk                                 /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_OER_P4_Msk instead */
#define PIO_OER_P5_Pos                      5                                              /**< (PIO_OER) Output Enable Position */
#define PIO_OER_P5_Msk                      (0x1U << PIO_OER_P5_Pos)                       /**< (PIO_OER) Output Enable Mask */
#define PIO_OER_P5                          PIO_OER_P5_Msk                                 /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_OER_P5_Msk instead */
#define PIO_OER_P6_Pos                      6                                              /**< (PIO_OER) Output Enable Position */
#define PIO_OER_P6_Msk                      (0x1U << PIO_OER_P6_Pos)                       /**< (PIO_OER) Output Enable Mask */
#define PIO_OER_P6                          PIO_OER_P6_Msk                                 /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_OER_P6_Msk instead */
#define PIO_OER_P7_Pos                      7                                              /**< (PIO_OER) Output Enable Position */
#define PIO_OER_P7_Msk                      (0x1U << PIO_OER_P7_Pos)                       /**< (PIO_OER) Output Enable Mask */
#define PIO_OER_P7                          PIO_OER_P7_Msk                                 /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_OER_P7_Msk instead */
#define PIO_OER_P8_Pos                      8                                              /**< (PIO_OER) Output Enable Position */
#define PIO_OER_P8_Msk                      (0x1U << PIO_OER_P8_Pos)                       /**< (PIO_OER) Output Enable Mask */
#define PIO_OER_P8                          PIO_OER_P8_Msk                                 /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_OER_P8_Msk instead */
#define PIO_OER_P9_Pos                      9                                              /**< (PIO_OER) Output Enable Position */
#define PIO_OER_P9_Msk                      (0x1U << PIO_OER_P9_Pos)                       /**< (PIO_OER) Output Enable Mask */
#define PIO_OER_P9                          PIO_OER_P9_Msk                                 /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_OER_P9_Msk instead */
#define PIO_OER_P10_Pos                     10                                             /**< (PIO_OER) Output Enable Position */
#define PIO_OER_P10_Msk                     (0x1U << PIO_OER_P10_Pos)                      /**< (PIO_OER) Output Enable Mask */
#define PIO_OER_P10                         PIO_OER_P10_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_OER_P10_Msk instead */
#define PIO_OER_P11_Pos                     11                                             /**< (PIO_OER) Output Enable Position */
#define PIO_OER_P11_Msk                     (0x1U << PIO_OER_P11_Pos)                      /**< (PIO_OER) Output Enable Mask */
#define PIO_OER_P11                         PIO_OER_P11_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_OER_P11_Msk instead */
#define PIO_OER_P12_Pos                     12                                             /**< (PIO_OER) Output Enable Position */
#define PIO_OER_P12_Msk                     (0x1U << PIO_OER_P12_Pos)                      /**< (PIO_OER) Output Enable Mask */
#define PIO_OER_P12                         PIO_OER_P12_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_OER_P12_Msk instead */
#define PIO_OER_P13_Pos                     13                                             /**< (PIO_OER) Output Enable Position */
#define PIO_OER_P13_Msk                     (0x1U << PIO_OER_P13_Pos)                      /**< (PIO_OER) Output Enable Mask */
#define PIO_OER_P13                         PIO_OER_P13_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_OER_P13_Msk instead */
#define PIO_OER_P14_Pos                     14                                             /**< (PIO_OER) Output Enable Position */
#define PIO_OER_P14_Msk                     (0x1U << PIO_OER_P14_Pos)                      /**< (PIO_OER) Output Enable Mask */
#define PIO_OER_P14                         PIO_OER_P14_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_OER_P14_Msk instead */
#define PIO_OER_P15_Pos                     15                                             /**< (PIO_OER) Output Enable Position */
#define PIO_OER_P15_Msk                     (0x1U << PIO_OER_P15_Pos)                      /**< (PIO_OER) Output Enable Mask */
#define PIO_OER_P15                         PIO_OER_P15_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_OER_P15_Msk instead */
#define PIO_OER_P16_Pos                     16                                             /**< (PIO_OER) Output Enable Position */
#define PIO_OER_P16_Msk                     (0x1U << PIO_OER_P16_Pos)                      /**< (PIO_OER) Output Enable Mask */
#define PIO_OER_P16                         PIO_OER_P16_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_OER_P16_Msk instead */
#define PIO_OER_P17_Pos                     17                                             /**< (PIO_OER) Output Enable Position */
#define PIO_OER_P17_Msk                     (0x1U << PIO_OER_P17_Pos)                      /**< (PIO_OER) Output Enable Mask */
#define PIO_OER_P17                         PIO_OER_P17_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_OER_P17_Msk instead */
#define PIO_OER_P18_Pos                     18                                             /**< (PIO_OER) Output Enable Position */
#define PIO_OER_P18_Msk                     (0x1U << PIO_OER_P18_Pos)                      /**< (PIO_OER) Output Enable Mask */
#define PIO_OER_P18                         PIO_OER_P18_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_OER_P18_Msk instead */
#define PIO_OER_P19_Pos                     19                                             /**< (PIO_OER) Output Enable Position */
#define PIO_OER_P19_Msk                     (0x1U << PIO_OER_P19_Pos)                      /**< (PIO_OER) Output Enable Mask */
#define PIO_OER_P19                         PIO_OER_P19_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_OER_P19_Msk instead */
#define PIO_OER_P20_Pos                     20                                             /**< (PIO_OER) Output Enable Position */
#define PIO_OER_P20_Msk                     (0x1U << PIO_OER_P20_Pos)                      /**< (PIO_OER) Output Enable Mask */
#define PIO_OER_P20                         PIO_OER_P20_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_OER_P20_Msk instead */
#define PIO_OER_P21_Pos                     21                                             /**< (PIO_OER) Output Enable Position */
#define PIO_OER_P21_Msk                     (0x1U << PIO_OER_P21_Pos)                      /**< (PIO_OER) Output Enable Mask */
#define PIO_OER_P21                         PIO_OER_P21_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_OER_P21_Msk instead */
#define PIO_OER_P22_Pos                     22                                             /**< (PIO_OER) Output Enable Position */
#define PIO_OER_P22_Msk                     (0x1U << PIO_OER_P22_Pos)                      /**< (PIO_OER) Output Enable Mask */
#define PIO_OER_P22                         PIO_OER_P22_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_OER_P22_Msk instead */
#define PIO_OER_P23_Pos                     23                                             /**< (PIO_OER) Output Enable Position */
#define PIO_OER_P23_Msk                     (0x1U << PIO_OER_P23_Pos)                      /**< (PIO_OER) Output Enable Mask */
#define PIO_OER_P23                         PIO_OER_P23_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_OER_P23_Msk instead */
#define PIO_OER_P24_Pos                     24                                             /**< (PIO_OER) Output Enable Position */
#define PIO_OER_P24_Msk                     (0x1U << PIO_OER_P24_Pos)                      /**< (PIO_OER) Output Enable Mask */
#define PIO_OER_P24                         PIO_OER_P24_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_OER_P24_Msk instead */
#define PIO_OER_P25_Pos                     25                                             /**< (PIO_OER) Output Enable Position */
#define PIO_OER_P25_Msk                     (0x1U << PIO_OER_P25_Pos)                      /**< (PIO_OER) Output Enable Mask */
#define PIO_OER_P25                         PIO_OER_P25_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_OER_P25_Msk instead */
#define PIO_OER_P26_Pos                     26                                             /**< (PIO_OER) Output Enable Position */
#define PIO_OER_P26_Msk                     (0x1U << PIO_OER_P26_Pos)                      /**< (PIO_OER) Output Enable Mask */
#define PIO_OER_P26                         PIO_OER_P26_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_OER_P26_Msk instead */
#define PIO_OER_P27_Pos                     27                                             /**< (PIO_OER) Output Enable Position */
#define PIO_OER_P27_Msk                     (0x1U << PIO_OER_P27_Pos)                      /**< (PIO_OER) Output Enable Mask */
#define PIO_OER_P27                         PIO_OER_P27_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_OER_P27_Msk instead */
#define PIO_OER_P28_Pos                     28                                             /**< (PIO_OER) Output Enable Position */
#define PIO_OER_P28_Msk                     (0x1U << PIO_OER_P28_Pos)                      /**< (PIO_OER) Output Enable Mask */
#define PIO_OER_P28                         PIO_OER_P28_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_OER_P28_Msk instead */
#define PIO_OER_P29_Pos                     29                                             /**< (PIO_OER) Output Enable Position */
#define PIO_OER_P29_Msk                     (0x1U << PIO_OER_P29_Pos)                      /**< (PIO_OER) Output Enable Mask */
#define PIO_OER_P29                         PIO_OER_P29_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_OER_P29_Msk instead */
#define PIO_OER_P30_Pos                     30                                             /**< (PIO_OER) Output Enable Position */
#define PIO_OER_P30_Msk                     (0x1U << PIO_OER_P30_Pos)                      /**< (PIO_OER) Output Enable Mask */
#define PIO_OER_P30                         PIO_OER_P30_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_OER_P30_Msk instead */
#define PIO_OER_P31_Pos                     31                                             /**< (PIO_OER) Output Enable Position */
#define PIO_OER_P31_Msk                     (0x1U << PIO_OER_P31_Pos)                      /**< (PIO_OER) Output Enable Mask */
#define PIO_OER_P31                         PIO_OER_P31_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_OER_P31_Msk instead */
#define PIO_OER_P_Pos                       0                                              /**< (PIO_OER Position) Output Enable */
#define PIO_OER_P_Msk                       (0xFFFFFFFFU << PIO_OER_P_Pos)                 /**< (PIO_OER Mask) P */
#define PIO_OER_P(value)                    (PIO_OER_P_Msk & ((value) << PIO_OER_P_Pos))   
#define PIO_OER_MASK                        (0xFFFFFFFFU)                                  /**< \deprecated (PIO_OER) Register MASK  (Use PIO_OER_Msk instead)  */
#define PIO_OER_Msk                         (0xFFFFFFFFU)                                  /**< (PIO_OER) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- PIO_ODR : (PIO Offset: 0x14) (/W 32) Output Disable Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t P0:1;                      /**< bit:      0  Output Disable                           */
    uint32_t P1:1;                      /**< bit:      1  Output Disable                           */
    uint32_t P2:1;                      /**< bit:      2  Output Disable                           */
    uint32_t P3:1;                      /**< bit:      3  Output Disable                           */
    uint32_t P4:1;                      /**< bit:      4  Output Disable                           */
    uint32_t P5:1;                      /**< bit:      5  Output Disable                           */
    uint32_t P6:1;                      /**< bit:      6  Output Disable                           */
    uint32_t P7:1;                      /**< bit:      7  Output Disable                           */
    uint32_t P8:1;                      /**< bit:      8  Output Disable                           */
    uint32_t P9:1;                      /**< bit:      9  Output Disable                           */
    uint32_t P10:1;                     /**< bit:     10  Output Disable                           */
    uint32_t P11:1;                     /**< bit:     11  Output Disable                           */
    uint32_t P12:1;                     /**< bit:     12  Output Disable                           */
    uint32_t P13:1;                     /**< bit:     13  Output Disable                           */
    uint32_t P14:1;                     /**< bit:     14  Output Disable                           */
    uint32_t P15:1;                     /**< bit:     15  Output Disable                           */
    uint32_t P16:1;                     /**< bit:     16  Output Disable                           */
    uint32_t P17:1;                     /**< bit:     17  Output Disable                           */
    uint32_t P18:1;                     /**< bit:     18  Output Disable                           */
    uint32_t P19:1;                     /**< bit:     19  Output Disable                           */
    uint32_t P20:1;                     /**< bit:     20  Output Disable                           */
    uint32_t P21:1;                     /**< bit:     21  Output Disable                           */
    uint32_t P22:1;                     /**< bit:     22  Output Disable                           */
    uint32_t P23:1;                     /**< bit:     23  Output Disable                           */
    uint32_t P24:1;                     /**< bit:     24  Output Disable                           */
    uint32_t P25:1;                     /**< bit:     25  Output Disable                           */
    uint32_t P26:1;                     /**< bit:     26  Output Disable                           */
    uint32_t P27:1;                     /**< bit:     27  Output Disable                           */
    uint32_t P28:1;                     /**< bit:     28  Output Disable                           */
    uint32_t P29:1;                     /**< bit:     29  Output Disable                           */
    uint32_t P30:1;                     /**< bit:     30  Output Disable                           */
    uint32_t P31:1;                     /**< bit:     31  Output Disable                           */
  } bit;                                /**< Structure used for bit  access */
  struct {
    uint32_t P:32;                      /**< bit:  0..31  Output Disable                           */
  } vec;                                /**< Structure used for vec  access  */
  uint32_t reg;                         /**< Type used for register access */
} PIO_ODR_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define PIO_ODR_OFFSET                      (0x14)                                        /**<  (PIO_ODR) Output Disable Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define PIO_ODR_P0_Pos                      0                                              /**< (PIO_ODR) Output Disable Position */
#define PIO_ODR_P0_Msk                      (0x1U << PIO_ODR_P0_Pos)                       /**< (PIO_ODR) Output Disable Mask */
#define PIO_ODR_P0                          PIO_ODR_P0_Msk                                 /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_ODR_P0_Msk instead */
#define PIO_ODR_P1_Pos                      1                                              /**< (PIO_ODR) Output Disable Position */
#define PIO_ODR_P1_Msk                      (0x1U << PIO_ODR_P1_Pos)                       /**< (PIO_ODR) Output Disable Mask */
#define PIO_ODR_P1                          PIO_ODR_P1_Msk                                 /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_ODR_P1_Msk instead */
#define PIO_ODR_P2_Pos                      2                                              /**< (PIO_ODR) Output Disable Position */
#define PIO_ODR_P2_Msk                      (0x1U << PIO_ODR_P2_Pos)                       /**< (PIO_ODR) Output Disable Mask */
#define PIO_ODR_P2                          PIO_ODR_P2_Msk                                 /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_ODR_P2_Msk instead */
#define PIO_ODR_P3_Pos                      3                                              /**< (PIO_ODR) Output Disable Position */
#define PIO_ODR_P3_Msk                      (0x1U << PIO_ODR_P3_Pos)                       /**< (PIO_ODR) Output Disable Mask */
#define PIO_ODR_P3                          PIO_ODR_P3_Msk                                 /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_ODR_P3_Msk instead */
#define PIO_ODR_P4_Pos                      4                                              /**< (PIO_ODR) Output Disable Position */
#define PIO_ODR_P4_Msk                      (0x1U << PIO_ODR_P4_Pos)                       /**< (PIO_ODR) Output Disable Mask */
#define PIO_ODR_P4                          PIO_ODR_P4_Msk                                 /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_ODR_P4_Msk instead */
#define PIO_ODR_P5_Pos                      5                                              /**< (PIO_ODR) Output Disable Position */
#define PIO_ODR_P5_Msk                      (0x1U << PIO_ODR_P5_Pos)                       /**< (PIO_ODR) Output Disable Mask */
#define PIO_ODR_P5                          PIO_ODR_P5_Msk                                 /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_ODR_P5_Msk instead */
#define PIO_ODR_P6_Pos                      6                                              /**< (PIO_ODR) Output Disable Position */
#define PIO_ODR_P6_Msk                      (0x1U << PIO_ODR_P6_Pos)                       /**< (PIO_ODR) Output Disable Mask */
#define PIO_ODR_P6                          PIO_ODR_P6_Msk                                 /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_ODR_P6_Msk instead */
#define PIO_ODR_P7_Pos                      7                                              /**< (PIO_ODR) Output Disable Position */
#define PIO_ODR_P7_Msk                      (0x1U << PIO_ODR_P7_Pos)                       /**< (PIO_ODR) Output Disable Mask */
#define PIO_ODR_P7                          PIO_ODR_P7_Msk                                 /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_ODR_P7_Msk instead */
#define PIO_ODR_P8_Pos                      8                                              /**< (PIO_ODR) Output Disable Position */
#define PIO_ODR_P8_Msk                      (0x1U << PIO_ODR_P8_Pos)                       /**< (PIO_ODR) Output Disable Mask */
#define PIO_ODR_P8                          PIO_ODR_P8_Msk                                 /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_ODR_P8_Msk instead */
#define PIO_ODR_P9_Pos                      9                                              /**< (PIO_ODR) Output Disable Position */
#define PIO_ODR_P9_Msk                      (0x1U << PIO_ODR_P9_Pos)                       /**< (PIO_ODR) Output Disable Mask */
#define PIO_ODR_P9                          PIO_ODR_P9_Msk                                 /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_ODR_P9_Msk instead */
#define PIO_ODR_P10_Pos                     10                                             /**< (PIO_ODR) Output Disable Position */
#define PIO_ODR_P10_Msk                     (0x1U << PIO_ODR_P10_Pos)                      /**< (PIO_ODR) Output Disable Mask */
#define PIO_ODR_P10                         PIO_ODR_P10_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_ODR_P10_Msk instead */
#define PIO_ODR_P11_Pos                     11                                             /**< (PIO_ODR) Output Disable Position */
#define PIO_ODR_P11_Msk                     (0x1U << PIO_ODR_P11_Pos)                      /**< (PIO_ODR) Output Disable Mask */
#define PIO_ODR_P11                         PIO_ODR_P11_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_ODR_P11_Msk instead */
#define PIO_ODR_P12_Pos                     12                                             /**< (PIO_ODR) Output Disable Position */
#define PIO_ODR_P12_Msk                     (0x1U << PIO_ODR_P12_Pos)                      /**< (PIO_ODR) Output Disable Mask */
#define PIO_ODR_P12                         PIO_ODR_P12_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_ODR_P12_Msk instead */
#define PIO_ODR_P13_Pos                     13                                             /**< (PIO_ODR) Output Disable Position */
#define PIO_ODR_P13_Msk                     (0x1U << PIO_ODR_P13_Pos)                      /**< (PIO_ODR) Output Disable Mask */
#define PIO_ODR_P13                         PIO_ODR_P13_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_ODR_P13_Msk instead */
#define PIO_ODR_P14_Pos                     14                                             /**< (PIO_ODR) Output Disable Position */
#define PIO_ODR_P14_Msk                     (0x1U << PIO_ODR_P14_Pos)                      /**< (PIO_ODR) Output Disable Mask */
#define PIO_ODR_P14                         PIO_ODR_P14_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_ODR_P14_Msk instead */
#define PIO_ODR_P15_Pos                     15                                             /**< (PIO_ODR) Output Disable Position */
#define PIO_ODR_P15_Msk                     (0x1U << PIO_ODR_P15_Pos)                      /**< (PIO_ODR) Output Disable Mask */
#define PIO_ODR_P15                         PIO_ODR_P15_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_ODR_P15_Msk instead */
#define PIO_ODR_P16_Pos                     16                                             /**< (PIO_ODR) Output Disable Position */
#define PIO_ODR_P16_Msk                     (0x1U << PIO_ODR_P16_Pos)                      /**< (PIO_ODR) Output Disable Mask */
#define PIO_ODR_P16                         PIO_ODR_P16_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_ODR_P16_Msk instead */
#define PIO_ODR_P17_Pos                     17                                             /**< (PIO_ODR) Output Disable Position */
#define PIO_ODR_P17_Msk                     (0x1U << PIO_ODR_P17_Pos)                      /**< (PIO_ODR) Output Disable Mask */
#define PIO_ODR_P17                         PIO_ODR_P17_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_ODR_P17_Msk instead */
#define PIO_ODR_P18_Pos                     18                                             /**< (PIO_ODR) Output Disable Position */
#define PIO_ODR_P18_Msk                     (0x1U << PIO_ODR_P18_Pos)                      /**< (PIO_ODR) Output Disable Mask */
#define PIO_ODR_P18                         PIO_ODR_P18_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_ODR_P18_Msk instead */
#define PIO_ODR_P19_Pos                     19                                             /**< (PIO_ODR) Output Disable Position */
#define PIO_ODR_P19_Msk                     (0x1U << PIO_ODR_P19_Pos)                      /**< (PIO_ODR) Output Disable Mask */
#define PIO_ODR_P19                         PIO_ODR_P19_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_ODR_P19_Msk instead */
#define PIO_ODR_P20_Pos                     20                                             /**< (PIO_ODR) Output Disable Position */
#define PIO_ODR_P20_Msk                     (0x1U << PIO_ODR_P20_Pos)                      /**< (PIO_ODR) Output Disable Mask */
#define PIO_ODR_P20                         PIO_ODR_P20_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_ODR_P20_Msk instead */
#define PIO_ODR_P21_Pos                     21                                             /**< (PIO_ODR) Output Disable Position */
#define PIO_ODR_P21_Msk                     (0x1U << PIO_ODR_P21_Pos)                      /**< (PIO_ODR) Output Disable Mask */
#define PIO_ODR_P21                         PIO_ODR_P21_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_ODR_P21_Msk instead */
#define PIO_ODR_P22_Pos                     22                                             /**< (PIO_ODR) Output Disable Position */
#define PIO_ODR_P22_Msk                     (0x1U << PIO_ODR_P22_Pos)                      /**< (PIO_ODR) Output Disable Mask */
#define PIO_ODR_P22                         PIO_ODR_P22_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_ODR_P22_Msk instead */
#define PIO_ODR_P23_Pos                     23                                             /**< (PIO_ODR) Output Disable Position */
#define PIO_ODR_P23_Msk                     (0x1U << PIO_ODR_P23_Pos)                      /**< (PIO_ODR) Output Disable Mask */
#define PIO_ODR_P23                         PIO_ODR_P23_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_ODR_P23_Msk instead */
#define PIO_ODR_P24_Pos                     24                                             /**< (PIO_ODR) Output Disable Position */
#define PIO_ODR_P24_Msk                     (0x1U << PIO_ODR_P24_Pos)                      /**< (PIO_ODR) Output Disable Mask */
#define PIO_ODR_P24                         PIO_ODR_P24_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_ODR_P24_Msk instead */
#define PIO_ODR_P25_Pos                     25                                             /**< (PIO_ODR) Output Disable Position */
#define PIO_ODR_P25_Msk                     (0x1U << PIO_ODR_P25_Pos)                      /**< (PIO_ODR) Output Disable Mask */
#define PIO_ODR_P25                         PIO_ODR_P25_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_ODR_P25_Msk instead */
#define PIO_ODR_P26_Pos                     26                                             /**< (PIO_ODR) Output Disable Position */
#define PIO_ODR_P26_Msk                     (0x1U << PIO_ODR_P26_Pos)                      /**< (PIO_ODR) Output Disable Mask */
#define PIO_ODR_P26                         PIO_ODR_P26_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_ODR_P26_Msk instead */
#define PIO_ODR_P27_Pos                     27                                             /**< (PIO_ODR) Output Disable Position */
#define PIO_ODR_P27_Msk                     (0x1U << PIO_ODR_P27_Pos)                      /**< (PIO_ODR) Output Disable Mask */
#define PIO_ODR_P27                         PIO_ODR_P27_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_ODR_P27_Msk instead */
#define PIO_ODR_P28_Pos                     28                                             /**< (PIO_ODR) Output Disable Position */
#define PIO_ODR_P28_Msk                     (0x1U << PIO_ODR_P28_Pos)                      /**< (PIO_ODR) Output Disable Mask */
#define PIO_ODR_P28                         PIO_ODR_P28_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_ODR_P28_Msk instead */
#define PIO_ODR_P29_Pos                     29                                             /**< (PIO_ODR) Output Disable Position */
#define PIO_ODR_P29_Msk                     (0x1U << PIO_ODR_P29_Pos)                      /**< (PIO_ODR) Output Disable Mask */
#define PIO_ODR_P29                         PIO_ODR_P29_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_ODR_P29_Msk instead */
#define PIO_ODR_P30_Pos                     30                                             /**< (PIO_ODR) Output Disable Position */
#define PIO_ODR_P30_Msk                     (0x1U << PIO_ODR_P30_Pos)                      /**< (PIO_ODR) Output Disable Mask */
#define PIO_ODR_P30                         PIO_ODR_P30_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_ODR_P30_Msk instead */
#define PIO_ODR_P31_Pos                     31                                             /**< (PIO_ODR) Output Disable Position */
#define PIO_ODR_P31_Msk                     (0x1U << PIO_ODR_P31_Pos)                      /**< (PIO_ODR) Output Disable Mask */
#define PIO_ODR_P31                         PIO_ODR_P31_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_ODR_P31_Msk instead */
#define PIO_ODR_P_Pos                       0                                              /**< (PIO_ODR Position) Output Disable */
#define PIO_ODR_P_Msk                       (0xFFFFFFFFU << PIO_ODR_P_Pos)                 /**< (PIO_ODR Mask) P */
#define PIO_ODR_P(value)                    (PIO_ODR_P_Msk & ((value) << PIO_ODR_P_Pos))   
#define PIO_ODR_MASK                        (0xFFFFFFFFU)                                  /**< \deprecated (PIO_ODR) Register MASK  (Use PIO_ODR_Msk instead)  */
#define PIO_ODR_Msk                         (0xFFFFFFFFU)                                  /**< (PIO_ODR) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- PIO_OSR : (PIO Offset: 0x18) (R/ 32) Output Status Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t P0:1;                      /**< bit:      0  Output Status                            */
    uint32_t P1:1;                      /**< bit:      1  Output Status                            */
    uint32_t P2:1;                      /**< bit:      2  Output Status                            */
    uint32_t P3:1;                      /**< bit:      3  Output Status                            */
    uint32_t P4:1;                      /**< bit:      4  Output Status                            */
    uint32_t P5:1;                      /**< bit:      5  Output Status                            */
    uint32_t P6:1;                      /**< bit:      6  Output Status                            */
    uint32_t P7:1;                      /**< bit:      7  Output Status                            */
    uint32_t P8:1;                      /**< bit:      8  Output Status                            */
    uint32_t P9:1;                      /**< bit:      9  Output Status                            */
    uint32_t P10:1;                     /**< bit:     10  Output Status                            */
    uint32_t P11:1;                     /**< bit:     11  Output Status                            */
    uint32_t P12:1;                     /**< bit:     12  Output Status                            */
    uint32_t P13:1;                     /**< bit:     13  Output Status                            */
    uint32_t P14:1;                     /**< bit:     14  Output Status                            */
    uint32_t P15:1;                     /**< bit:     15  Output Status                            */
    uint32_t P16:1;                     /**< bit:     16  Output Status                            */
    uint32_t P17:1;                     /**< bit:     17  Output Status                            */
    uint32_t P18:1;                     /**< bit:     18  Output Status                            */
    uint32_t P19:1;                     /**< bit:     19  Output Status                            */
    uint32_t P20:1;                     /**< bit:     20  Output Status                            */
    uint32_t P21:1;                     /**< bit:     21  Output Status                            */
    uint32_t P22:1;                     /**< bit:     22  Output Status                            */
    uint32_t P23:1;                     /**< bit:     23  Output Status                            */
    uint32_t P24:1;                     /**< bit:     24  Output Status                            */
    uint32_t P25:1;                     /**< bit:     25  Output Status                            */
    uint32_t P26:1;                     /**< bit:     26  Output Status                            */
    uint32_t P27:1;                     /**< bit:     27  Output Status                            */
    uint32_t P28:1;                     /**< bit:     28  Output Status                            */
    uint32_t P29:1;                     /**< bit:     29  Output Status                            */
    uint32_t P30:1;                     /**< bit:     30  Output Status                            */
    uint32_t P31:1;                     /**< bit:     31  Output Status                            */
  } bit;                                /**< Structure used for bit  access */
  struct {
    uint32_t P:32;                      /**< bit:  0..31  Output Status                            */
  } vec;                                /**< Structure used for vec  access  */
  uint32_t reg;                         /**< Type used for register access */
} PIO_OSR_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define PIO_OSR_OFFSET                      (0x18)                                        /**<  (PIO_OSR) Output Status Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define PIO_OSR_P0_Pos                      0                                              /**< (PIO_OSR) Output Status Position */
#define PIO_OSR_P0_Msk                      (0x1U << PIO_OSR_P0_Pos)                       /**< (PIO_OSR) Output Status Mask */
#define PIO_OSR_P0                          PIO_OSR_P0_Msk                                 /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_OSR_P0_Msk instead */
#define PIO_OSR_P1_Pos                      1                                              /**< (PIO_OSR) Output Status Position */
#define PIO_OSR_P1_Msk                      (0x1U << PIO_OSR_P1_Pos)                       /**< (PIO_OSR) Output Status Mask */
#define PIO_OSR_P1                          PIO_OSR_P1_Msk                                 /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_OSR_P1_Msk instead */
#define PIO_OSR_P2_Pos                      2                                              /**< (PIO_OSR) Output Status Position */
#define PIO_OSR_P2_Msk                      (0x1U << PIO_OSR_P2_Pos)                       /**< (PIO_OSR) Output Status Mask */
#define PIO_OSR_P2                          PIO_OSR_P2_Msk                                 /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_OSR_P2_Msk instead */
#define PIO_OSR_P3_Pos                      3                                              /**< (PIO_OSR) Output Status Position */
#define PIO_OSR_P3_Msk                      (0x1U << PIO_OSR_P3_Pos)                       /**< (PIO_OSR) Output Status Mask */
#define PIO_OSR_P3                          PIO_OSR_P3_Msk                                 /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_OSR_P3_Msk instead */
#define PIO_OSR_P4_Pos                      4                                              /**< (PIO_OSR) Output Status Position */
#define PIO_OSR_P4_Msk                      (0x1U << PIO_OSR_P4_Pos)                       /**< (PIO_OSR) Output Status Mask */
#define PIO_OSR_P4                          PIO_OSR_P4_Msk                                 /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_OSR_P4_Msk instead */
#define PIO_OSR_P5_Pos                      5                                              /**< (PIO_OSR) Output Status Position */
#define PIO_OSR_P5_Msk                      (0x1U << PIO_OSR_P5_Pos)                       /**< (PIO_OSR) Output Status Mask */
#define PIO_OSR_P5                          PIO_OSR_P5_Msk                                 /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_OSR_P5_Msk instead */
#define PIO_OSR_P6_Pos                      6                                              /**< (PIO_OSR) Output Status Position */
#define PIO_OSR_P6_Msk                      (0x1U << PIO_OSR_P6_Pos)                       /**< (PIO_OSR) Output Status Mask */
#define PIO_OSR_P6                          PIO_OSR_P6_Msk                                 /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_OSR_P6_Msk instead */
#define PIO_OSR_P7_Pos                      7                                              /**< (PIO_OSR) Output Status Position */
#define PIO_OSR_P7_Msk                      (0x1U << PIO_OSR_P7_Pos)                       /**< (PIO_OSR) Output Status Mask */
#define PIO_OSR_P7                          PIO_OSR_P7_Msk                                 /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_OSR_P7_Msk instead */
#define PIO_OSR_P8_Pos                      8                                              /**< (PIO_OSR) Output Status Position */
#define PIO_OSR_P8_Msk                      (0x1U << PIO_OSR_P8_Pos)                       /**< (PIO_OSR) Output Status Mask */
#define PIO_OSR_P8                          PIO_OSR_P8_Msk                                 /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_OSR_P8_Msk instead */
#define PIO_OSR_P9_Pos                      9                                              /**< (PIO_OSR) Output Status Position */
#define PIO_OSR_P9_Msk                      (0x1U << PIO_OSR_P9_Pos)                       /**< (PIO_OSR) Output Status Mask */
#define PIO_OSR_P9                          PIO_OSR_P9_Msk                                 /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_OSR_P9_Msk instead */
#define PIO_OSR_P10_Pos                     10                                             /**< (PIO_OSR) Output Status Position */
#define PIO_OSR_P10_Msk                     (0x1U << PIO_OSR_P10_Pos)                      /**< (PIO_OSR) Output Status Mask */
#define PIO_OSR_P10                         PIO_OSR_P10_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_OSR_P10_Msk instead */
#define PIO_OSR_P11_Pos                     11                                             /**< (PIO_OSR) Output Status Position */
#define PIO_OSR_P11_Msk                     (0x1U << PIO_OSR_P11_Pos)                      /**< (PIO_OSR) Output Status Mask */
#define PIO_OSR_P11                         PIO_OSR_P11_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_OSR_P11_Msk instead */
#define PIO_OSR_P12_Pos                     12                                             /**< (PIO_OSR) Output Status Position */
#define PIO_OSR_P12_Msk                     (0x1U << PIO_OSR_P12_Pos)                      /**< (PIO_OSR) Output Status Mask */
#define PIO_OSR_P12                         PIO_OSR_P12_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_OSR_P12_Msk instead */
#define PIO_OSR_P13_Pos                     13                                             /**< (PIO_OSR) Output Status Position */
#define PIO_OSR_P13_Msk                     (0x1U << PIO_OSR_P13_Pos)                      /**< (PIO_OSR) Output Status Mask */
#define PIO_OSR_P13                         PIO_OSR_P13_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_OSR_P13_Msk instead */
#define PIO_OSR_P14_Pos                     14                                             /**< (PIO_OSR) Output Status Position */
#define PIO_OSR_P14_Msk                     (0x1U << PIO_OSR_P14_Pos)                      /**< (PIO_OSR) Output Status Mask */
#define PIO_OSR_P14                         PIO_OSR_P14_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_OSR_P14_Msk instead */
#define PIO_OSR_P15_Pos                     15                                             /**< (PIO_OSR) Output Status Position */
#define PIO_OSR_P15_Msk                     (0x1U << PIO_OSR_P15_Pos)                      /**< (PIO_OSR) Output Status Mask */
#define PIO_OSR_P15                         PIO_OSR_P15_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_OSR_P15_Msk instead */
#define PIO_OSR_P16_Pos                     16                                             /**< (PIO_OSR) Output Status Position */
#define PIO_OSR_P16_Msk                     (0x1U << PIO_OSR_P16_Pos)                      /**< (PIO_OSR) Output Status Mask */
#define PIO_OSR_P16                         PIO_OSR_P16_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_OSR_P16_Msk instead */
#define PIO_OSR_P17_Pos                     17                                             /**< (PIO_OSR) Output Status Position */
#define PIO_OSR_P17_Msk                     (0x1U << PIO_OSR_P17_Pos)                      /**< (PIO_OSR) Output Status Mask */
#define PIO_OSR_P17                         PIO_OSR_P17_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_OSR_P17_Msk instead */
#define PIO_OSR_P18_Pos                     18                                             /**< (PIO_OSR) Output Status Position */
#define PIO_OSR_P18_Msk                     (0x1U << PIO_OSR_P18_Pos)                      /**< (PIO_OSR) Output Status Mask */
#define PIO_OSR_P18                         PIO_OSR_P18_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_OSR_P18_Msk instead */
#define PIO_OSR_P19_Pos                     19                                             /**< (PIO_OSR) Output Status Position */
#define PIO_OSR_P19_Msk                     (0x1U << PIO_OSR_P19_Pos)                      /**< (PIO_OSR) Output Status Mask */
#define PIO_OSR_P19                         PIO_OSR_P19_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_OSR_P19_Msk instead */
#define PIO_OSR_P20_Pos                     20                                             /**< (PIO_OSR) Output Status Position */
#define PIO_OSR_P20_Msk                     (0x1U << PIO_OSR_P20_Pos)                      /**< (PIO_OSR) Output Status Mask */
#define PIO_OSR_P20                         PIO_OSR_P20_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_OSR_P20_Msk instead */
#define PIO_OSR_P21_Pos                     21                                             /**< (PIO_OSR) Output Status Position */
#define PIO_OSR_P21_Msk                     (0x1U << PIO_OSR_P21_Pos)                      /**< (PIO_OSR) Output Status Mask */
#define PIO_OSR_P21                         PIO_OSR_P21_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_OSR_P21_Msk instead */
#define PIO_OSR_P22_Pos                     22                                             /**< (PIO_OSR) Output Status Position */
#define PIO_OSR_P22_Msk                     (0x1U << PIO_OSR_P22_Pos)                      /**< (PIO_OSR) Output Status Mask */
#define PIO_OSR_P22                         PIO_OSR_P22_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_OSR_P22_Msk instead */
#define PIO_OSR_P23_Pos                     23                                             /**< (PIO_OSR) Output Status Position */
#define PIO_OSR_P23_Msk                     (0x1U << PIO_OSR_P23_Pos)                      /**< (PIO_OSR) Output Status Mask */
#define PIO_OSR_P23                         PIO_OSR_P23_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_OSR_P23_Msk instead */
#define PIO_OSR_P24_Pos                     24                                             /**< (PIO_OSR) Output Status Position */
#define PIO_OSR_P24_Msk                     (0x1U << PIO_OSR_P24_Pos)                      /**< (PIO_OSR) Output Status Mask */
#define PIO_OSR_P24                         PIO_OSR_P24_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_OSR_P24_Msk instead */
#define PIO_OSR_P25_Pos                     25                                             /**< (PIO_OSR) Output Status Position */
#define PIO_OSR_P25_Msk                     (0x1U << PIO_OSR_P25_Pos)                      /**< (PIO_OSR) Output Status Mask */
#define PIO_OSR_P25                         PIO_OSR_P25_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_OSR_P25_Msk instead */
#define PIO_OSR_P26_Pos                     26                                             /**< (PIO_OSR) Output Status Position */
#define PIO_OSR_P26_Msk                     (0x1U << PIO_OSR_P26_Pos)                      /**< (PIO_OSR) Output Status Mask */
#define PIO_OSR_P26                         PIO_OSR_P26_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_OSR_P26_Msk instead */
#define PIO_OSR_P27_Pos                     27                                             /**< (PIO_OSR) Output Status Position */
#define PIO_OSR_P27_Msk                     (0x1U << PIO_OSR_P27_Pos)                      /**< (PIO_OSR) Output Status Mask */
#define PIO_OSR_P27                         PIO_OSR_P27_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_OSR_P27_Msk instead */
#define PIO_OSR_P28_Pos                     28                                             /**< (PIO_OSR) Output Status Position */
#define PIO_OSR_P28_Msk                     (0x1U << PIO_OSR_P28_Pos)                      /**< (PIO_OSR) Output Status Mask */
#define PIO_OSR_P28                         PIO_OSR_P28_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_OSR_P28_Msk instead */
#define PIO_OSR_P29_Pos                     29                                             /**< (PIO_OSR) Output Status Position */
#define PIO_OSR_P29_Msk                     (0x1U << PIO_OSR_P29_Pos)                      /**< (PIO_OSR) Output Status Mask */
#define PIO_OSR_P29                         PIO_OSR_P29_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_OSR_P29_Msk instead */
#define PIO_OSR_P30_Pos                     30                                             /**< (PIO_OSR) Output Status Position */
#define PIO_OSR_P30_Msk                     (0x1U << PIO_OSR_P30_Pos)                      /**< (PIO_OSR) Output Status Mask */
#define PIO_OSR_P30                         PIO_OSR_P30_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_OSR_P30_Msk instead */
#define PIO_OSR_P31_Pos                     31                                             /**< (PIO_OSR) Output Status Position */
#define PIO_OSR_P31_Msk                     (0x1U << PIO_OSR_P31_Pos)                      /**< (PIO_OSR) Output Status Mask */
#define PIO_OSR_P31                         PIO_OSR_P31_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_OSR_P31_Msk instead */
#define PIO_OSR_P_Pos                       0                                              /**< (PIO_OSR Position) Output Status */
#define PIO_OSR_P_Msk                       (0xFFFFFFFFU << PIO_OSR_P_Pos)                 /**< (PIO_OSR Mask) P */
#define PIO_OSR_P(value)                    (PIO_OSR_P_Msk & ((value) << PIO_OSR_P_Pos))   
#define PIO_OSR_MASK                        (0xFFFFFFFFU)                                  /**< \deprecated (PIO_OSR) Register MASK  (Use PIO_OSR_Msk instead)  */
#define PIO_OSR_Msk                         (0xFFFFFFFFU)                                  /**< (PIO_OSR) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- PIO_IFER : (PIO Offset: 0x20) (/W 32) Glitch Input Filter Enable Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t P0:1;                      /**< bit:      0  Input Filter Enable                      */
    uint32_t P1:1;                      /**< bit:      1  Input Filter Enable                      */
    uint32_t P2:1;                      /**< bit:      2  Input Filter Enable                      */
    uint32_t P3:1;                      /**< bit:      3  Input Filter Enable                      */
    uint32_t P4:1;                      /**< bit:      4  Input Filter Enable                      */
    uint32_t P5:1;                      /**< bit:      5  Input Filter Enable                      */
    uint32_t P6:1;                      /**< bit:      6  Input Filter Enable                      */
    uint32_t P7:1;                      /**< bit:      7  Input Filter Enable                      */
    uint32_t P8:1;                      /**< bit:      8  Input Filter Enable                      */
    uint32_t P9:1;                      /**< bit:      9  Input Filter Enable                      */
    uint32_t P10:1;                     /**< bit:     10  Input Filter Enable                      */
    uint32_t P11:1;                     /**< bit:     11  Input Filter Enable                      */
    uint32_t P12:1;                     /**< bit:     12  Input Filter Enable                      */
    uint32_t P13:1;                     /**< bit:     13  Input Filter Enable                      */
    uint32_t P14:1;                     /**< bit:     14  Input Filter Enable                      */
    uint32_t P15:1;                     /**< bit:     15  Input Filter Enable                      */
    uint32_t P16:1;                     /**< bit:     16  Input Filter Enable                      */
    uint32_t P17:1;                     /**< bit:     17  Input Filter Enable                      */
    uint32_t P18:1;                     /**< bit:     18  Input Filter Enable                      */
    uint32_t P19:1;                     /**< bit:     19  Input Filter Enable                      */
    uint32_t P20:1;                     /**< bit:     20  Input Filter Enable                      */
    uint32_t P21:1;                     /**< bit:     21  Input Filter Enable                      */
    uint32_t P22:1;                     /**< bit:     22  Input Filter Enable                      */
    uint32_t P23:1;                     /**< bit:     23  Input Filter Enable                      */
    uint32_t P24:1;                     /**< bit:     24  Input Filter Enable                      */
    uint32_t P25:1;                     /**< bit:     25  Input Filter Enable                      */
    uint32_t P26:1;                     /**< bit:     26  Input Filter Enable                      */
    uint32_t P27:1;                     /**< bit:     27  Input Filter Enable                      */
    uint32_t P28:1;                     /**< bit:     28  Input Filter Enable                      */
    uint32_t P29:1;                     /**< bit:     29  Input Filter Enable                      */
    uint32_t P30:1;                     /**< bit:     30  Input Filter Enable                      */
    uint32_t P31:1;                     /**< bit:     31  Input Filter Enable                      */
  } bit;                                /**< Structure used for bit  access */
  struct {
    uint32_t P:32;                      /**< bit:  0..31  Input Filter Enable                      */
  } vec;                                /**< Structure used for vec  access  */
  uint32_t reg;                         /**< Type used for register access */
} PIO_IFER_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define PIO_IFER_OFFSET                     (0x20)                                        /**<  (PIO_IFER) Glitch Input Filter Enable Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define PIO_IFER_P0_Pos                     0                                              /**< (PIO_IFER) Input Filter Enable Position */
#define PIO_IFER_P0_Msk                     (0x1U << PIO_IFER_P0_Pos)                      /**< (PIO_IFER) Input Filter Enable Mask */
#define PIO_IFER_P0                         PIO_IFER_P0_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_IFER_P0_Msk instead */
#define PIO_IFER_P1_Pos                     1                                              /**< (PIO_IFER) Input Filter Enable Position */
#define PIO_IFER_P1_Msk                     (0x1U << PIO_IFER_P1_Pos)                      /**< (PIO_IFER) Input Filter Enable Mask */
#define PIO_IFER_P1                         PIO_IFER_P1_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_IFER_P1_Msk instead */
#define PIO_IFER_P2_Pos                     2                                              /**< (PIO_IFER) Input Filter Enable Position */
#define PIO_IFER_P2_Msk                     (0x1U << PIO_IFER_P2_Pos)                      /**< (PIO_IFER) Input Filter Enable Mask */
#define PIO_IFER_P2                         PIO_IFER_P2_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_IFER_P2_Msk instead */
#define PIO_IFER_P3_Pos                     3                                              /**< (PIO_IFER) Input Filter Enable Position */
#define PIO_IFER_P3_Msk                     (0x1U << PIO_IFER_P3_Pos)                      /**< (PIO_IFER) Input Filter Enable Mask */
#define PIO_IFER_P3                         PIO_IFER_P3_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_IFER_P3_Msk instead */
#define PIO_IFER_P4_Pos                     4                                              /**< (PIO_IFER) Input Filter Enable Position */
#define PIO_IFER_P4_Msk                     (0x1U << PIO_IFER_P4_Pos)                      /**< (PIO_IFER) Input Filter Enable Mask */
#define PIO_IFER_P4                         PIO_IFER_P4_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_IFER_P4_Msk instead */
#define PIO_IFER_P5_Pos                     5                                              /**< (PIO_IFER) Input Filter Enable Position */
#define PIO_IFER_P5_Msk                     (0x1U << PIO_IFER_P5_Pos)                      /**< (PIO_IFER) Input Filter Enable Mask */
#define PIO_IFER_P5                         PIO_IFER_P5_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_IFER_P5_Msk instead */
#define PIO_IFER_P6_Pos                     6                                              /**< (PIO_IFER) Input Filter Enable Position */
#define PIO_IFER_P6_Msk                     (0x1U << PIO_IFER_P6_Pos)                      /**< (PIO_IFER) Input Filter Enable Mask */
#define PIO_IFER_P6                         PIO_IFER_P6_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_IFER_P6_Msk instead */
#define PIO_IFER_P7_Pos                     7                                              /**< (PIO_IFER) Input Filter Enable Position */
#define PIO_IFER_P7_Msk                     (0x1U << PIO_IFER_P7_Pos)                      /**< (PIO_IFER) Input Filter Enable Mask */
#define PIO_IFER_P7                         PIO_IFER_P7_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_IFER_P7_Msk instead */
#define PIO_IFER_P8_Pos                     8                                              /**< (PIO_IFER) Input Filter Enable Position */
#define PIO_IFER_P8_Msk                     (0x1U << PIO_IFER_P8_Pos)                      /**< (PIO_IFER) Input Filter Enable Mask */
#define PIO_IFER_P8                         PIO_IFER_P8_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_IFER_P8_Msk instead */
#define PIO_IFER_P9_Pos                     9                                              /**< (PIO_IFER) Input Filter Enable Position */
#define PIO_IFER_P9_Msk                     (0x1U << PIO_IFER_P9_Pos)                      /**< (PIO_IFER) Input Filter Enable Mask */
#define PIO_IFER_P9                         PIO_IFER_P9_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_IFER_P9_Msk instead */
#define PIO_IFER_P10_Pos                    10                                             /**< (PIO_IFER) Input Filter Enable Position */
#define PIO_IFER_P10_Msk                    (0x1U << PIO_IFER_P10_Pos)                     /**< (PIO_IFER) Input Filter Enable Mask */
#define PIO_IFER_P10                        PIO_IFER_P10_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_IFER_P10_Msk instead */
#define PIO_IFER_P11_Pos                    11                                             /**< (PIO_IFER) Input Filter Enable Position */
#define PIO_IFER_P11_Msk                    (0x1U << PIO_IFER_P11_Pos)                     /**< (PIO_IFER) Input Filter Enable Mask */
#define PIO_IFER_P11                        PIO_IFER_P11_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_IFER_P11_Msk instead */
#define PIO_IFER_P12_Pos                    12                                             /**< (PIO_IFER) Input Filter Enable Position */
#define PIO_IFER_P12_Msk                    (0x1U << PIO_IFER_P12_Pos)                     /**< (PIO_IFER) Input Filter Enable Mask */
#define PIO_IFER_P12                        PIO_IFER_P12_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_IFER_P12_Msk instead */
#define PIO_IFER_P13_Pos                    13                                             /**< (PIO_IFER) Input Filter Enable Position */
#define PIO_IFER_P13_Msk                    (0x1U << PIO_IFER_P13_Pos)                     /**< (PIO_IFER) Input Filter Enable Mask */
#define PIO_IFER_P13                        PIO_IFER_P13_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_IFER_P13_Msk instead */
#define PIO_IFER_P14_Pos                    14                                             /**< (PIO_IFER) Input Filter Enable Position */
#define PIO_IFER_P14_Msk                    (0x1U << PIO_IFER_P14_Pos)                     /**< (PIO_IFER) Input Filter Enable Mask */
#define PIO_IFER_P14                        PIO_IFER_P14_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_IFER_P14_Msk instead */
#define PIO_IFER_P15_Pos                    15                                             /**< (PIO_IFER) Input Filter Enable Position */
#define PIO_IFER_P15_Msk                    (0x1U << PIO_IFER_P15_Pos)                     /**< (PIO_IFER) Input Filter Enable Mask */
#define PIO_IFER_P15                        PIO_IFER_P15_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_IFER_P15_Msk instead */
#define PIO_IFER_P16_Pos                    16                                             /**< (PIO_IFER) Input Filter Enable Position */
#define PIO_IFER_P16_Msk                    (0x1U << PIO_IFER_P16_Pos)                     /**< (PIO_IFER) Input Filter Enable Mask */
#define PIO_IFER_P16                        PIO_IFER_P16_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_IFER_P16_Msk instead */
#define PIO_IFER_P17_Pos                    17                                             /**< (PIO_IFER) Input Filter Enable Position */
#define PIO_IFER_P17_Msk                    (0x1U << PIO_IFER_P17_Pos)                     /**< (PIO_IFER) Input Filter Enable Mask */
#define PIO_IFER_P17                        PIO_IFER_P17_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_IFER_P17_Msk instead */
#define PIO_IFER_P18_Pos                    18                                             /**< (PIO_IFER) Input Filter Enable Position */
#define PIO_IFER_P18_Msk                    (0x1U << PIO_IFER_P18_Pos)                     /**< (PIO_IFER) Input Filter Enable Mask */
#define PIO_IFER_P18                        PIO_IFER_P18_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_IFER_P18_Msk instead */
#define PIO_IFER_P19_Pos                    19                                             /**< (PIO_IFER) Input Filter Enable Position */
#define PIO_IFER_P19_Msk                    (0x1U << PIO_IFER_P19_Pos)                     /**< (PIO_IFER) Input Filter Enable Mask */
#define PIO_IFER_P19                        PIO_IFER_P19_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_IFER_P19_Msk instead */
#define PIO_IFER_P20_Pos                    20                                             /**< (PIO_IFER) Input Filter Enable Position */
#define PIO_IFER_P20_Msk                    (0x1U << PIO_IFER_P20_Pos)                     /**< (PIO_IFER) Input Filter Enable Mask */
#define PIO_IFER_P20                        PIO_IFER_P20_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_IFER_P20_Msk instead */
#define PIO_IFER_P21_Pos                    21                                             /**< (PIO_IFER) Input Filter Enable Position */
#define PIO_IFER_P21_Msk                    (0x1U << PIO_IFER_P21_Pos)                     /**< (PIO_IFER) Input Filter Enable Mask */
#define PIO_IFER_P21                        PIO_IFER_P21_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_IFER_P21_Msk instead */
#define PIO_IFER_P22_Pos                    22                                             /**< (PIO_IFER) Input Filter Enable Position */
#define PIO_IFER_P22_Msk                    (0x1U << PIO_IFER_P22_Pos)                     /**< (PIO_IFER) Input Filter Enable Mask */
#define PIO_IFER_P22                        PIO_IFER_P22_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_IFER_P22_Msk instead */
#define PIO_IFER_P23_Pos                    23                                             /**< (PIO_IFER) Input Filter Enable Position */
#define PIO_IFER_P23_Msk                    (0x1U << PIO_IFER_P23_Pos)                     /**< (PIO_IFER) Input Filter Enable Mask */
#define PIO_IFER_P23                        PIO_IFER_P23_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_IFER_P23_Msk instead */
#define PIO_IFER_P24_Pos                    24                                             /**< (PIO_IFER) Input Filter Enable Position */
#define PIO_IFER_P24_Msk                    (0x1U << PIO_IFER_P24_Pos)                     /**< (PIO_IFER) Input Filter Enable Mask */
#define PIO_IFER_P24                        PIO_IFER_P24_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_IFER_P24_Msk instead */
#define PIO_IFER_P25_Pos                    25                                             /**< (PIO_IFER) Input Filter Enable Position */
#define PIO_IFER_P25_Msk                    (0x1U << PIO_IFER_P25_Pos)                     /**< (PIO_IFER) Input Filter Enable Mask */
#define PIO_IFER_P25                        PIO_IFER_P25_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_IFER_P25_Msk instead */
#define PIO_IFER_P26_Pos                    26                                             /**< (PIO_IFER) Input Filter Enable Position */
#define PIO_IFER_P26_Msk                    (0x1U << PIO_IFER_P26_Pos)                     /**< (PIO_IFER) Input Filter Enable Mask */
#define PIO_IFER_P26                        PIO_IFER_P26_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_IFER_P26_Msk instead */
#define PIO_IFER_P27_Pos                    27                                             /**< (PIO_IFER) Input Filter Enable Position */
#define PIO_IFER_P27_Msk                    (0x1U << PIO_IFER_P27_Pos)                     /**< (PIO_IFER) Input Filter Enable Mask */
#define PIO_IFER_P27                        PIO_IFER_P27_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_IFER_P27_Msk instead */
#define PIO_IFER_P28_Pos                    28                                             /**< (PIO_IFER) Input Filter Enable Position */
#define PIO_IFER_P28_Msk                    (0x1U << PIO_IFER_P28_Pos)                     /**< (PIO_IFER) Input Filter Enable Mask */
#define PIO_IFER_P28                        PIO_IFER_P28_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_IFER_P28_Msk instead */
#define PIO_IFER_P29_Pos                    29                                             /**< (PIO_IFER) Input Filter Enable Position */
#define PIO_IFER_P29_Msk                    (0x1U << PIO_IFER_P29_Pos)                     /**< (PIO_IFER) Input Filter Enable Mask */
#define PIO_IFER_P29                        PIO_IFER_P29_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_IFER_P29_Msk instead */
#define PIO_IFER_P30_Pos                    30                                             /**< (PIO_IFER) Input Filter Enable Position */
#define PIO_IFER_P30_Msk                    (0x1U << PIO_IFER_P30_Pos)                     /**< (PIO_IFER) Input Filter Enable Mask */
#define PIO_IFER_P30                        PIO_IFER_P30_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_IFER_P30_Msk instead */
#define PIO_IFER_P31_Pos                    31                                             /**< (PIO_IFER) Input Filter Enable Position */
#define PIO_IFER_P31_Msk                    (0x1U << PIO_IFER_P31_Pos)                     /**< (PIO_IFER) Input Filter Enable Mask */
#define PIO_IFER_P31                        PIO_IFER_P31_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_IFER_P31_Msk instead */
#define PIO_IFER_P_Pos                      0                                              /**< (PIO_IFER Position) Input Filter Enable */
#define PIO_IFER_P_Msk                      (0xFFFFFFFFU << PIO_IFER_P_Pos)                /**< (PIO_IFER Mask) P */
#define PIO_IFER_P(value)                   (PIO_IFER_P_Msk & ((value) << PIO_IFER_P_Pos))  
#define PIO_IFER_MASK                       (0xFFFFFFFFU)                                  /**< \deprecated (PIO_IFER) Register MASK  (Use PIO_IFER_Msk instead)  */
#define PIO_IFER_Msk                        (0xFFFFFFFFU)                                  /**< (PIO_IFER) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- PIO_IFDR : (PIO Offset: 0x24) (/W 32) Glitch Input Filter Disable Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t P0:1;                      /**< bit:      0  Input Filter Disable                     */
    uint32_t P1:1;                      /**< bit:      1  Input Filter Disable                     */
    uint32_t P2:1;                      /**< bit:      2  Input Filter Disable                     */
    uint32_t P3:1;                      /**< bit:      3  Input Filter Disable                     */
    uint32_t P4:1;                      /**< bit:      4  Input Filter Disable                     */
    uint32_t P5:1;                      /**< bit:      5  Input Filter Disable                     */
    uint32_t P6:1;                      /**< bit:      6  Input Filter Disable                     */
    uint32_t P7:1;                      /**< bit:      7  Input Filter Disable                     */
    uint32_t P8:1;                      /**< bit:      8  Input Filter Disable                     */
    uint32_t P9:1;                      /**< bit:      9  Input Filter Disable                     */
    uint32_t P10:1;                     /**< bit:     10  Input Filter Disable                     */
    uint32_t P11:1;                     /**< bit:     11  Input Filter Disable                     */
    uint32_t P12:1;                     /**< bit:     12  Input Filter Disable                     */
    uint32_t P13:1;                     /**< bit:     13  Input Filter Disable                     */
    uint32_t P14:1;                     /**< bit:     14  Input Filter Disable                     */
    uint32_t P15:1;                     /**< bit:     15  Input Filter Disable                     */
    uint32_t P16:1;                     /**< bit:     16  Input Filter Disable                     */
    uint32_t P17:1;                     /**< bit:     17  Input Filter Disable                     */
    uint32_t P18:1;                     /**< bit:     18  Input Filter Disable                     */
    uint32_t P19:1;                     /**< bit:     19  Input Filter Disable                     */
    uint32_t P20:1;                     /**< bit:     20  Input Filter Disable                     */
    uint32_t P21:1;                     /**< bit:     21  Input Filter Disable                     */
    uint32_t P22:1;                     /**< bit:     22  Input Filter Disable                     */
    uint32_t P23:1;                     /**< bit:     23  Input Filter Disable                     */
    uint32_t P24:1;                     /**< bit:     24  Input Filter Disable                     */
    uint32_t P25:1;                     /**< bit:     25  Input Filter Disable                     */
    uint32_t P26:1;                     /**< bit:     26  Input Filter Disable                     */
    uint32_t P27:1;                     /**< bit:     27  Input Filter Disable                     */
    uint32_t P28:1;                     /**< bit:     28  Input Filter Disable                     */
    uint32_t P29:1;                     /**< bit:     29  Input Filter Disable                     */
    uint32_t P30:1;                     /**< bit:     30  Input Filter Disable                     */
    uint32_t P31:1;                     /**< bit:     31  Input Filter Disable                     */
  } bit;                                /**< Structure used for bit  access */
  struct {
    uint32_t P:32;                      /**< bit:  0..31  Input Filter Disable                     */
  } vec;                                /**< Structure used for vec  access  */
  uint32_t reg;                         /**< Type used for register access */
} PIO_IFDR_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define PIO_IFDR_OFFSET                     (0x24)                                        /**<  (PIO_IFDR) Glitch Input Filter Disable Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define PIO_IFDR_P0_Pos                     0                                              /**< (PIO_IFDR) Input Filter Disable Position */
#define PIO_IFDR_P0_Msk                     (0x1U << PIO_IFDR_P0_Pos)                      /**< (PIO_IFDR) Input Filter Disable Mask */
#define PIO_IFDR_P0                         PIO_IFDR_P0_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_IFDR_P0_Msk instead */
#define PIO_IFDR_P1_Pos                     1                                              /**< (PIO_IFDR) Input Filter Disable Position */
#define PIO_IFDR_P1_Msk                     (0x1U << PIO_IFDR_P1_Pos)                      /**< (PIO_IFDR) Input Filter Disable Mask */
#define PIO_IFDR_P1                         PIO_IFDR_P1_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_IFDR_P1_Msk instead */
#define PIO_IFDR_P2_Pos                     2                                              /**< (PIO_IFDR) Input Filter Disable Position */
#define PIO_IFDR_P2_Msk                     (0x1U << PIO_IFDR_P2_Pos)                      /**< (PIO_IFDR) Input Filter Disable Mask */
#define PIO_IFDR_P2                         PIO_IFDR_P2_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_IFDR_P2_Msk instead */
#define PIO_IFDR_P3_Pos                     3                                              /**< (PIO_IFDR) Input Filter Disable Position */
#define PIO_IFDR_P3_Msk                     (0x1U << PIO_IFDR_P3_Pos)                      /**< (PIO_IFDR) Input Filter Disable Mask */
#define PIO_IFDR_P3                         PIO_IFDR_P3_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_IFDR_P3_Msk instead */
#define PIO_IFDR_P4_Pos                     4                                              /**< (PIO_IFDR) Input Filter Disable Position */
#define PIO_IFDR_P4_Msk                     (0x1U << PIO_IFDR_P4_Pos)                      /**< (PIO_IFDR) Input Filter Disable Mask */
#define PIO_IFDR_P4                         PIO_IFDR_P4_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_IFDR_P4_Msk instead */
#define PIO_IFDR_P5_Pos                     5                                              /**< (PIO_IFDR) Input Filter Disable Position */
#define PIO_IFDR_P5_Msk                     (0x1U << PIO_IFDR_P5_Pos)                      /**< (PIO_IFDR) Input Filter Disable Mask */
#define PIO_IFDR_P5                         PIO_IFDR_P5_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_IFDR_P5_Msk instead */
#define PIO_IFDR_P6_Pos                     6                                              /**< (PIO_IFDR) Input Filter Disable Position */
#define PIO_IFDR_P6_Msk                     (0x1U << PIO_IFDR_P6_Pos)                      /**< (PIO_IFDR) Input Filter Disable Mask */
#define PIO_IFDR_P6                         PIO_IFDR_P6_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_IFDR_P6_Msk instead */
#define PIO_IFDR_P7_Pos                     7                                              /**< (PIO_IFDR) Input Filter Disable Position */
#define PIO_IFDR_P7_Msk                     (0x1U << PIO_IFDR_P7_Pos)                      /**< (PIO_IFDR) Input Filter Disable Mask */
#define PIO_IFDR_P7                         PIO_IFDR_P7_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_IFDR_P7_Msk instead */
#define PIO_IFDR_P8_Pos                     8                                              /**< (PIO_IFDR) Input Filter Disable Position */
#define PIO_IFDR_P8_Msk                     (0x1U << PIO_IFDR_P8_Pos)                      /**< (PIO_IFDR) Input Filter Disable Mask */
#define PIO_IFDR_P8                         PIO_IFDR_P8_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_IFDR_P8_Msk instead */
#define PIO_IFDR_P9_Pos                     9                                              /**< (PIO_IFDR) Input Filter Disable Position */
#define PIO_IFDR_P9_Msk                     (0x1U << PIO_IFDR_P9_Pos)                      /**< (PIO_IFDR) Input Filter Disable Mask */
#define PIO_IFDR_P9                         PIO_IFDR_P9_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_IFDR_P9_Msk instead */
#define PIO_IFDR_P10_Pos                    10                                             /**< (PIO_IFDR) Input Filter Disable Position */
#define PIO_IFDR_P10_Msk                    (0x1U << PIO_IFDR_P10_Pos)                     /**< (PIO_IFDR) Input Filter Disable Mask */
#define PIO_IFDR_P10                        PIO_IFDR_P10_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_IFDR_P10_Msk instead */
#define PIO_IFDR_P11_Pos                    11                                             /**< (PIO_IFDR) Input Filter Disable Position */
#define PIO_IFDR_P11_Msk                    (0x1U << PIO_IFDR_P11_Pos)                     /**< (PIO_IFDR) Input Filter Disable Mask */
#define PIO_IFDR_P11                        PIO_IFDR_P11_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_IFDR_P11_Msk instead */
#define PIO_IFDR_P12_Pos                    12                                             /**< (PIO_IFDR) Input Filter Disable Position */
#define PIO_IFDR_P12_Msk                    (0x1U << PIO_IFDR_P12_Pos)                     /**< (PIO_IFDR) Input Filter Disable Mask */
#define PIO_IFDR_P12                        PIO_IFDR_P12_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_IFDR_P12_Msk instead */
#define PIO_IFDR_P13_Pos                    13                                             /**< (PIO_IFDR) Input Filter Disable Position */
#define PIO_IFDR_P13_Msk                    (0x1U << PIO_IFDR_P13_Pos)                     /**< (PIO_IFDR) Input Filter Disable Mask */
#define PIO_IFDR_P13                        PIO_IFDR_P13_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_IFDR_P13_Msk instead */
#define PIO_IFDR_P14_Pos                    14                                             /**< (PIO_IFDR) Input Filter Disable Position */
#define PIO_IFDR_P14_Msk                    (0x1U << PIO_IFDR_P14_Pos)                     /**< (PIO_IFDR) Input Filter Disable Mask */
#define PIO_IFDR_P14                        PIO_IFDR_P14_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_IFDR_P14_Msk instead */
#define PIO_IFDR_P15_Pos                    15                                             /**< (PIO_IFDR) Input Filter Disable Position */
#define PIO_IFDR_P15_Msk                    (0x1U << PIO_IFDR_P15_Pos)                     /**< (PIO_IFDR) Input Filter Disable Mask */
#define PIO_IFDR_P15                        PIO_IFDR_P15_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_IFDR_P15_Msk instead */
#define PIO_IFDR_P16_Pos                    16                                             /**< (PIO_IFDR) Input Filter Disable Position */
#define PIO_IFDR_P16_Msk                    (0x1U << PIO_IFDR_P16_Pos)                     /**< (PIO_IFDR) Input Filter Disable Mask */
#define PIO_IFDR_P16                        PIO_IFDR_P16_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_IFDR_P16_Msk instead */
#define PIO_IFDR_P17_Pos                    17                                             /**< (PIO_IFDR) Input Filter Disable Position */
#define PIO_IFDR_P17_Msk                    (0x1U << PIO_IFDR_P17_Pos)                     /**< (PIO_IFDR) Input Filter Disable Mask */
#define PIO_IFDR_P17                        PIO_IFDR_P17_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_IFDR_P17_Msk instead */
#define PIO_IFDR_P18_Pos                    18                                             /**< (PIO_IFDR) Input Filter Disable Position */
#define PIO_IFDR_P18_Msk                    (0x1U << PIO_IFDR_P18_Pos)                     /**< (PIO_IFDR) Input Filter Disable Mask */
#define PIO_IFDR_P18                        PIO_IFDR_P18_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_IFDR_P18_Msk instead */
#define PIO_IFDR_P19_Pos                    19                                             /**< (PIO_IFDR) Input Filter Disable Position */
#define PIO_IFDR_P19_Msk                    (0x1U << PIO_IFDR_P19_Pos)                     /**< (PIO_IFDR) Input Filter Disable Mask */
#define PIO_IFDR_P19                        PIO_IFDR_P19_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_IFDR_P19_Msk instead */
#define PIO_IFDR_P20_Pos                    20                                             /**< (PIO_IFDR) Input Filter Disable Position */
#define PIO_IFDR_P20_Msk                    (0x1U << PIO_IFDR_P20_Pos)                     /**< (PIO_IFDR) Input Filter Disable Mask */
#define PIO_IFDR_P20                        PIO_IFDR_P20_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_IFDR_P20_Msk instead */
#define PIO_IFDR_P21_Pos                    21                                             /**< (PIO_IFDR) Input Filter Disable Position */
#define PIO_IFDR_P21_Msk                    (0x1U << PIO_IFDR_P21_Pos)                     /**< (PIO_IFDR) Input Filter Disable Mask */
#define PIO_IFDR_P21                        PIO_IFDR_P21_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_IFDR_P21_Msk instead */
#define PIO_IFDR_P22_Pos                    22                                             /**< (PIO_IFDR) Input Filter Disable Position */
#define PIO_IFDR_P22_Msk                    (0x1U << PIO_IFDR_P22_Pos)                     /**< (PIO_IFDR) Input Filter Disable Mask */
#define PIO_IFDR_P22                        PIO_IFDR_P22_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_IFDR_P22_Msk instead */
#define PIO_IFDR_P23_Pos                    23                                             /**< (PIO_IFDR) Input Filter Disable Position */
#define PIO_IFDR_P23_Msk                    (0x1U << PIO_IFDR_P23_Pos)                     /**< (PIO_IFDR) Input Filter Disable Mask */
#define PIO_IFDR_P23                        PIO_IFDR_P23_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_IFDR_P23_Msk instead */
#define PIO_IFDR_P24_Pos                    24                                             /**< (PIO_IFDR) Input Filter Disable Position */
#define PIO_IFDR_P24_Msk                    (0x1U << PIO_IFDR_P24_Pos)                     /**< (PIO_IFDR) Input Filter Disable Mask */
#define PIO_IFDR_P24                        PIO_IFDR_P24_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_IFDR_P24_Msk instead */
#define PIO_IFDR_P25_Pos                    25                                             /**< (PIO_IFDR) Input Filter Disable Position */
#define PIO_IFDR_P25_Msk                    (0x1U << PIO_IFDR_P25_Pos)                     /**< (PIO_IFDR) Input Filter Disable Mask */
#define PIO_IFDR_P25                        PIO_IFDR_P25_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_IFDR_P25_Msk instead */
#define PIO_IFDR_P26_Pos                    26                                             /**< (PIO_IFDR) Input Filter Disable Position */
#define PIO_IFDR_P26_Msk                    (0x1U << PIO_IFDR_P26_Pos)                     /**< (PIO_IFDR) Input Filter Disable Mask */
#define PIO_IFDR_P26                        PIO_IFDR_P26_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_IFDR_P26_Msk instead */
#define PIO_IFDR_P27_Pos                    27                                             /**< (PIO_IFDR) Input Filter Disable Position */
#define PIO_IFDR_P27_Msk                    (0x1U << PIO_IFDR_P27_Pos)                     /**< (PIO_IFDR) Input Filter Disable Mask */
#define PIO_IFDR_P27                        PIO_IFDR_P27_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_IFDR_P27_Msk instead */
#define PIO_IFDR_P28_Pos                    28                                             /**< (PIO_IFDR) Input Filter Disable Position */
#define PIO_IFDR_P28_Msk                    (0x1U << PIO_IFDR_P28_Pos)                     /**< (PIO_IFDR) Input Filter Disable Mask */
#define PIO_IFDR_P28                        PIO_IFDR_P28_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_IFDR_P28_Msk instead */
#define PIO_IFDR_P29_Pos                    29                                             /**< (PIO_IFDR) Input Filter Disable Position */
#define PIO_IFDR_P29_Msk                    (0x1U << PIO_IFDR_P29_Pos)                     /**< (PIO_IFDR) Input Filter Disable Mask */
#define PIO_IFDR_P29                        PIO_IFDR_P29_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_IFDR_P29_Msk instead */
#define PIO_IFDR_P30_Pos                    30                                             /**< (PIO_IFDR) Input Filter Disable Position */
#define PIO_IFDR_P30_Msk                    (0x1U << PIO_IFDR_P30_Pos)                     /**< (PIO_IFDR) Input Filter Disable Mask */
#define PIO_IFDR_P30                        PIO_IFDR_P30_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_IFDR_P30_Msk instead */
#define PIO_IFDR_P31_Pos                    31                                             /**< (PIO_IFDR) Input Filter Disable Position */
#define PIO_IFDR_P31_Msk                    (0x1U << PIO_IFDR_P31_Pos)                     /**< (PIO_IFDR) Input Filter Disable Mask */
#define PIO_IFDR_P31                        PIO_IFDR_P31_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_IFDR_P31_Msk instead */
#define PIO_IFDR_P_Pos                      0                                              /**< (PIO_IFDR Position) Input Filter Disable */
#define PIO_IFDR_P_Msk                      (0xFFFFFFFFU << PIO_IFDR_P_Pos)                /**< (PIO_IFDR Mask) P */
#define PIO_IFDR_P(value)                   (PIO_IFDR_P_Msk & ((value) << PIO_IFDR_P_Pos))  
#define PIO_IFDR_MASK                       (0xFFFFFFFFU)                                  /**< \deprecated (PIO_IFDR) Register MASK  (Use PIO_IFDR_Msk instead)  */
#define PIO_IFDR_Msk                        (0xFFFFFFFFU)                                  /**< (PIO_IFDR) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- PIO_IFSR : (PIO Offset: 0x28) (R/ 32) Glitch Input Filter Status Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t P0:1;                      /**< bit:      0  Input Filter Status                      */
    uint32_t P1:1;                      /**< bit:      1  Input Filter Status                      */
    uint32_t P2:1;                      /**< bit:      2  Input Filter Status                      */
    uint32_t P3:1;                      /**< bit:      3  Input Filter Status                      */
    uint32_t P4:1;                      /**< bit:      4  Input Filter Status                      */
    uint32_t P5:1;                      /**< bit:      5  Input Filter Status                      */
    uint32_t P6:1;                      /**< bit:      6  Input Filter Status                      */
    uint32_t P7:1;                      /**< bit:      7  Input Filter Status                      */
    uint32_t P8:1;                      /**< bit:      8  Input Filter Status                      */
    uint32_t P9:1;                      /**< bit:      9  Input Filter Status                      */
    uint32_t P10:1;                     /**< bit:     10  Input Filter Status                      */
    uint32_t P11:1;                     /**< bit:     11  Input Filter Status                      */
    uint32_t P12:1;                     /**< bit:     12  Input Filter Status                      */
    uint32_t P13:1;                     /**< bit:     13  Input Filter Status                      */
    uint32_t P14:1;                     /**< bit:     14  Input Filter Status                      */
    uint32_t P15:1;                     /**< bit:     15  Input Filter Status                      */
    uint32_t P16:1;                     /**< bit:     16  Input Filter Status                      */
    uint32_t P17:1;                     /**< bit:     17  Input Filter Status                      */
    uint32_t P18:1;                     /**< bit:     18  Input Filter Status                      */
    uint32_t P19:1;                     /**< bit:     19  Input Filter Status                      */
    uint32_t P20:1;                     /**< bit:     20  Input Filter Status                      */
    uint32_t P21:1;                     /**< bit:     21  Input Filter Status                      */
    uint32_t P22:1;                     /**< bit:     22  Input Filter Status                      */
    uint32_t P23:1;                     /**< bit:     23  Input Filter Status                      */
    uint32_t P24:1;                     /**< bit:     24  Input Filter Status                      */
    uint32_t P25:1;                     /**< bit:     25  Input Filter Status                      */
    uint32_t P26:1;                     /**< bit:     26  Input Filter Status                      */
    uint32_t P27:1;                     /**< bit:     27  Input Filter Status                      */
    uint32_t P28:1;                     /**< bit:     28  Input Filter Status                      */
    uint32_t P29:1;                     /**< bit:     29  Input Filter Status                      */
    uint32_t P30:1;                     /**< bit:     30  Input Filter Status                      */
    uint32_t P31:1;                     /**< bit:     31  Input Filter Status                      */
  } bit;                                /**< Structure used for bit  access */
  struct {
    uint32_t P:32;                      /**< bit:  0..31  Input Filter Status                      */
  } vec;                                /**< Structure used for vec  access  */
  uint32_t reg;                         /**< Type used for register access */
} PIO_IFSR_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define PIO_IFSR_OFFSET                     (0x28)                                        /**<  (PIO_IFSR) Glitch Input Filter Status Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define PIO_IFSR_P0_Pos                     0                                              /**< (PIO_IFSR) Input Filter Status Position */
#define PIO_IFSR_P0_Msk                     (0x1U << PIO_IFSR_P0_Pos)                      /**< (PIO_IFSR) Input Filter Status Mask */
#define PIO_IFSR_P0                         PIO_IFSR_P0_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_IFSR_P0_Msk instead */
#define PIO_IFSR_P1_Pos                     1                                              /**< (PIO_IFSR) Input Filter Status Position */
#define PIO_IFSR_P1_Msk                     (0x1U << PIO_IFSR_P1_Pos)                      /**< (PIO_IFSR) Input Filter Status Mask */
#define PIO_IFSR_P1                         PIO_IFSR_P1_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_IFSR_P1_Msk instead */
#define PIO_IFSR_P2_Pos                     2                                              /**< (PIO_IFSR) Input Filter Status Position */
#define PIO_IFSR_P2_Msk                     (0x1U << PIO_IFSR_P2_Pos)                      /**< (PIO_IFSR) Input Filter Status Mask */
#define PIO_IFSR_P2                         PIO_IFSR_P2_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_IFSR_P2_Msk instead */
#define PIO_IFSR_P3_Pos                     3                                              /**< (PIO_IFSR) Input Filter Status Position */
#define PIO_IFSR_P3_Msk                     (0x1U << PIO_IFSR_P3_Pos)                      /**< (PIO_IFSR) Input Filter Status Mask */
#define PIO_IFSR_P3                         PIO_IFSR_P3_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_IFSR_P3_Msk instead */
#define PIO_IFSR_P4_Pos                     4                                              /**< (PIO_IFSR) Input Filter Status Position */
#define PIO_IFSR_P4_Msk                     (0x1U << PIO_IFSR_P4_Pos)                      /**< (PIO_IFSR) Input Filter Status Mask */
#define PIO_IFSR_P4                         PIO_IFSR_P4_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_IFSR_P4_Msk instead */
#define PIO_IFSR_P5_Pos                     5                                              /**< (PIO_IFSR) Input Filter Status Position */
#define PIO_IFSR_P5_Msk                     (0x1U << PIO_IFSR_P5_Pos)                      /**< (PIO_IFSR) Input Filter Status Mask */
#define PIO_IFSR_P5                         PIO_IFSR_P5_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_IFSR_P5_Msk instead */
#define PIO_IFSR_P6_Pos                     6                                              /**< (PIO_IFSR) Input Filter Status Position */
#define PIO_IFSR_P6_Msk                     (0x1U << PIO_IFSR_P6_Pos)                      /**< (PIO_IFSR) Input Filter Status Mask */
#define PIO_IFSR_P6                         PIO_IFSR_P6_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_IFSR_P6_Msk instead */
#define PIO_IFSR_P7_Pos                     7                                              /**< (PIO_IFSR) Input Filter Status Position */
#define PIO_IFSR_P7_Msk                     (0x1U << PIO_IFSR_P7_Pos)                      /**< (PIO_IFSR) Input Filter Status Mask */
#define PIO_IFSR_P7                         PIO_IFSR_P7_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_IFSR_P7_Msk instead */
#define PIO_IFSR_P8_Pos                     8                                              /**< (PIO_IFSR) Input Filter Status Position */
#define PIO_IFSR_P8_Msk                     (0x1U << PIO_IFSR_P8_Pos)                      /**< (PIO_IFSR) Input Filter Status Mask */
#define PIO_IFSR_P8                         PIO_IFSR_P8_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_IFSR_P8_Msk instead */
#define PIO_IFSR_P9_Pos                     9                                              /**< (PIO_IFSR) Input Filter Status Position */
#define PIO_IFSR_P9_Msk                     (0x1U << PIO_IFSR_P9_Pos)                      /**< (PIO_IFSR) Input Filter Status Mask */
#define PIO_IFSR_P9                         PIO_IFSR_P9_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_IFSR_P9_Msk instead */
#define PIO_IFSR_P10_Pos                    10                                             /**< (PIO_IFSR) Input Filter Status Position */
#define PIO_IFSR_P10_Msk                    (0x1U << PIO_IFSR_P10_Pos)                     /**< (PIO_IFSR) Input Filter Status Mask */
#define PIO_IFSR_P10                        PIO_IFSR_P10_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_IFSR_P10_Msk instead */
#define PIO_IFSR_P11_Pos                    11                                             /**< (PIO_IFSR) Input Filter Status Position */
#define PIO_IFSR_P11_Msk                    (0x1U << PIO_IFSR_P11_Pos)                     /**< (PIO_IFSR) Input Filter Status Mask */
#define PIO_IFSR_P11                        PIO_IFSR_P11_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_IFSR_P11_Msk instead */
#define PIO_IFSR_P12_Pos                    12                                             /**< (PIO_IFSR) Input Filter Status Position */
#define PIO_IFSR_P12_Msk                    (0x1U << PIO_IFSR_P12_Pos)                     /**< (PIO_IFSR) Input Filter Status Mask */
#define PIO_IFSR_P12                        PIO_IFSR_P12_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_IFSR_P12_Msk instead */
#define PIO_IFSR_P13_Pos                    13                                             /**< (PIO_IFSR) Input Filter Status Position */
#define PIO_IFSR_P13_Msk                    (0x1U << PIO_IFSR_P13_Pos)                     /**< (PIO_IFSR) Input Filter Status Mask */
#define PIO_IFSR_P13                        PIO_IFSR_P13_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_IFSR_P13_Msk instead */
#define PIO_IFSR_P14_Pos                    14                                             /**< (PIO_IFSR) Input Filter Status Position */
#define PIO_IFSR_P14_Msk                    (0x1U << PIO_IFSR_P14_Pos)                     /**< (PIO_IFSR) Input Filter Status Mask */
#define PIO_IFSR_P14                        PIO_IFSR_P14_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_IFSR_P14_Msk instead */
#define PIO_IFSR_P15_Pos                    15                                             /**< (PIO_IFSR) Input Filter Status Position */
#define PIO_IFSR_P15_Msk                    (0x1U << PIO_IFSR_P15_Pos)                     /**< (PIO_IFSR) Input Filter Status Mask */
#define PIO_IFSR_P15                        PIO_IFSR_P15_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_IFSR_P15_Msk instead */
#define PIO_IFSR_P16_Pos                    16                                             /**< (PIO_IFSR) Input Filter Status Position */
#define PIO_IFSR_P16_Msk                    (0x1U << PIO_IFSR_P16_Pos)                     /**< (PIO_IFSR) Input Filter Status Mask */
#define PIO_IFSR_P16                        PIO_IFSR_P16_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_IFSR_P16_Msk instead */
#define PIO_IFSR_P17_Pos                    17                                             /**< (PIO_IFSR) Input Filter Status Position */
#define PIO_IFSR_P17_Msk                    (0x1U << PIO_IFSR_P17_Pos)                     /**< (PIO_IFSR) Input Filter Status Mask */
#define PIO_IFSR_P17                        PIO_IFSR_P17_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_IFSR_P17_Msk instead */
#define PIO_IFSR_P18_Pos                    18                                             /**< (PIO_IFSR) Input Filter Status Position */
#define PIO_IFSR_P18_Msk                    (0x1U << PIO_IFSR_P18_Pos)                     /**< (PIO_IFSR) Input Filter Status Mask */
#define PIO_IFSR_P18                        PIO_IFSR_P18_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_IFSR_P18_Msk instead */
#define PIO_IFSR_P19_Pos                    19                                             /**< (PIO_IFSR) Input Filter Status Position */
#define PIO_IFSR_P19_Msk                    (0x1U << PIO_IFSR_P19_Pos)                     /**< (PIO_IFSR) Input Filter Status Mask */
#define PIO_IFSR_P19                        PIO_IFSR_P19_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_IFSR_P19_Msk instead */
#define PIO_IFSR_P20_Pos                    20                                             /**< (PIO_IFSR) Input Filter Status Position */
#define PIO_IFSR_P20_Msk                    (0x1U << PIO_IFSR_P20_Pos)                     /**< (PIO_IFSR) Input Filter Status Mask */
#define PIO_IFSR_P20                        PIO_IFSR_P20_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_IFSR_P20_Msk instead */
#define PIO_IFSR_P21_Pos                    21                                             /**< (PIO_IFSR) Input Filter Status Position */
#define PIO_IFSR_P21_Msk                    (0x1U << PIO_IFSR_P21_Pos)                     /**< (PIO_IFSR) Input Filter Status Mask */
#define PIO_IFSR_P21                        PIO_IFSR_P21_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_IFSR_P21_Msk instead */
#define PIO_IFSR_P22_Pos                    22                                             /**< (PIO_IFSR) Input Filter Status Position */
#define PIO_IFSR_P22_Msk                    (0x1U << PIO_IFSR_P22_Pos)                     /**< (PIO_IFSR) Input Filter Status Mask */
#define PIO_IFSR_P22                        PIO_IFSR_P22_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_IFSR_P22_Msk instead */
#define PIO_IFSR_P23_Pos                    23                                             /**< (PIO_IFSR) Input Filter Status Position */
#define PIO_IFSR_P23_Msk                    (0x1U << PIO_IFSR_P23_Pos)                     /**< (PIO_IFSR) Input Filter Status Mask */
#define PIO_IFSR_P23                        PIO_IFSR_P23_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_IFSR_P23_Msk instead */
#define PIO_IFSR_P24_Pos                    24                                             /**< (PIO_IFSR) Input Filter Status Position */
#define PIO_IFSR_P24_Msk                    (0x1U << PIO_IFSR_P24_Pos)                     /**< (PIO_IFSR) Input Filter Status Mask */
#define PIO_IFSR_P24                        PIO_IFSR_P24_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_IFSR_P24_Msk instead */
#define PIO_IFSR_P25_Pos                    25                                             /**< (PIO_IFSR) Input Filter Status Position */
#define PIO_IFSR_P25_Msk                    (0x1U << PIO_IFSR_P25_Pos)                     /**< (PIO_IFSR) Input Filter Status Mask */
#define PIO_IFSR_P25                        PIO_IFSR_P25_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_IFSR_P25_Msk instead */
#define PIO_IFSR_P26_Pos                    26                                             /**< (PIO_IFSR) Input Filter Status Position */
#define PIO_IFSR_P26_Msk                    (0x1U << PIO_IFSR_P26_Pos)                     /**< (PIO_IFSR) Input Filter Status Mask */
#define PIO_IFSR_P26                        PIO_IFSR_P26_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_IFSR_P26_Msk instead */
#define PIO_IFSR_P27_Pos                    27                                             /**< (PIO_IFSR) Input Filter Status Position */
#define PIO_IFSR_P27_Msk                    (0x1U << PIO_IFSR_P27_Pos)                     /**< (PIO_IFSR) Input Filter Status Mask */
#define PIO_IFSR_P27                        PIO_IFSR_P27_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_IFSR_P27_Msk instead */
#define PIO_IFSR_P28_Pos                    28                                             /**< (PIO_IFSR) Input Filter Status Position */
#define PIO_IFSR_P28_Msk                    (0x1U << PIO_IFSR_P28_Pos)                     /**< (PIO_IFSR) Input Filter Status Mask */
#define PIO_IFSR_P28                        PIO_IFSR_P28_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_IFSR_P28_Msk instead */
#define PIO_IFSR_P29_Pos                    29                                             /**< (PIO_IFSR) Input Filter Status Position */
#define PIO_IFSR_P29_Msk                    (0x1U << PIO_IFSR_P29_Pos)                     /**< (PIO_IFSR) Input Filter Status Mask */
#define PIO_IFSR_P29                        PIO_IFSR_P29_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_IFSR_P29_Msk instead */
#define PIO_IFSR_P30_Pos                    30                                             /**< (PIO_IFSR) Input Filter Status Position */
#define PIO_IFSR_P30_Msk                    (0x1U << PIO_IFSR_P30_Pos)                     /**< (PIO_IFSR) Input Filter Status Mask */
#define PIO_IFSR_P30                        PIO_IFSR_P30_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_IFSR_P30_Msk instead */
#define PIO_IFSR_P31_Pos                    31                                             /**< (PIO_IFSR) Input Filter Status Position */
#define PIO_IFSR_P31_Msk                    (0x1U << PIO_IFSR_P31_Pos)                     /**< (PIO_IFSR) Input Filter Status Mask */
#define PIO_IFSR_P31                        PIO_IFSR_P31_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_IFSR_P31_Msk instead */
#define PIO_IFSR_P_Pos                      0                                              /**< (PIO_IFSR Position) Input Filter Status */
#define PIO_IFSR_P_Msk                      (0xFFFFFFFFU << PIO_IFSR_P_Pos)                /**< (PIO_IFSR Mask) P */
#define PIO_IFSR_P(value)                   (PIO_IFSR_P_Msk & ((value) << PIO_IFSR_P_Pos))  
#define PIO_IFSR_MASK                       (0xFFFFFFFFU)                                  /**< \deprecated (PIO_IFSR) Register MASK  (Use PIO_IFSR_Msk instead)  */
#define PIO_IFSR_Msk                        (0xFFFFFFFFU)                                  /**< (PIO_IFSR) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- PIO_SODR : (PIO Offset: 0x30) (/W 32) Set Output Data Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t P0:1;                      /**< bit:      0  Set Output Data                          */
    uint32_t P1:1;                      /**< bit:      1  Set Output Data                          */
    uint32_t P2:1;                      /**< bit:      2  Set Output Data                          */
    uint32_t P3:1;                      /**< bit:      3  Set Output Data                          */
    uint32_t P4:1;                      /**< bit:      4  Set Output Data                          */
    uint32_t P5:1;                      /**< bit:      5  Set Output Data                          */
    uint32_t P6:1;                      /**< bit:      6  Set Output Data                          */
    uint32_t P7:1;                      /**< bit:      7  Set Output Data                          */
    uint32_t P8:1;                      /**< bit:      8  Set Output Data                          */
    uint32_t P9:1;                      /**< bit:      9  Set Output Data                          */
    uint32_t P10:1;                     /**< bit:     10  Set Output Data                          */
    uint32_t P11:1;                     /**< bit:     11  Set Output Data                          */
    uint32_t P12:1;                     /**< bit:     12  Set Output Data                          */
    uint32_t P13:1;                     /**< bit:     13  Set Output Data                          */
    uint32_t P14:1;                     /**< bit:     14  Set Output Data                          */
    uint32_t P15:1;                     /**< bit:     15  Set Output Data                          */
    uint32_t P16:1;                     /**< bit:     16  Set Output Data                          */
    uint32_t P17:1;                     /**< bit:     17  Set Output Data                          */
    uint32_t P18:1;                     /**< bit:     18  Set Output Data                          */
    uint32_t P19:1;                     /**< bit:     19  Set Output Data                          */
    uint32_t P20:1;                     /**< bit:     20  Set Output Data                          */
    uint32_t P21:1;                     /**< bit:     21  Set Output Data                          */
    uint32_t P22:1;                     /**< bit:     22  Set Output Data                          */
    uint32_t P23:1;                     /**< bit:     23  Set Output Data                          */
    uint32_t P24:1;                     /**< bit:     24  Set Output Data                          */
    uint32_t P25:1;                     /**< bit:     25  Set Output Data                          */
    uint32_t P26:1;                     /**< bit:     26  Set Output Data                          */
    uint32_t P27:1;                     /**< bit:     27  Set Output Data                          */
    uint32_t P28:1;                     /**< bit:     28  Set Output Data                          */
    uint32_t P29:1;                     /**< bit:     29  Set Output Data                          */
    uint32_t P30:1;                     /**< bit:     30  Set Output Data                          */
    uint32_t P31:1;                     /**< bit:     31  Set Output Data                          */
  } bit;                                /**< Structure used for bit  access */
  struct {
    uint32_t P:32;                      /**< bit:  0..31  Set Output Data                          */
  } vec;                                /**< Structure used for vec  access  */
  uint32_t reg;                         /**< Type used for register access */
} PIO_SODR_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define PIO_SODR_OFFSET                     (0x30)                                        /**<  (PIO_SODR) Set Output Data Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define PIO_SODR_P0_Pos                     0                                              /**< (PIO_SODR) Set Output Data Position */
#define PIO_SODR_P0_Msk                     (0x1U << PIO_SODR_P0_Pos)                      /**< (PIO_SODR) Set Output Data Mask */
#define PIO_SODR_P0                         PIO_SODR_P0_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_SODR_P0_Msk instead */
#define PIO_SODR_P1_Pos                     1                                              /**< (PIO_SODR) Set Output Data Position */
#define PIO_SODR_P1_Msk                     (0x1U << PIO_SODR_P1_Pos)                      /**< (PIO_SODR) Set Output Data Mask */
#define PIO_SODR_P1                         PIO_SODR_P1_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_SODR_P1_Msk instead */
#define PIO_SODR_P2_Pos                     2                                              /**< (PIO_SODR) Set Output Data Position */
#define PIO_SODR_P2_Msk                     (0x1U << PIO_SODR_P2_Pos)                      /**< (PIO_SODR) Set Output Data Mask */
#define PIO_SODR_P2                         PIO_SODR_P2_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_SODR_P2_Msk instead */
#define PIO_SODR_P3_Pos                     3                                              /**< (PIO_SODR) Set Output Data Position */
#define PIO_SODR_P3_Msk                     (0x1U << PIO_SODR_P3_Pos)                      /**< (PIO_SODR) Set Output Data Mask */
#define PIO_SODR_P3                         PIO_SODR_P3_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_SODR_P3_Msk instead */
#define PIO_SODR_P4_Pos                     4                                              /**< (PIO_SODR) Set Output Data Position */
#define PIO_SODR_P4_Msk                     (0x1U << PIO_SODR_P4_Pos)                      /**< (PIO_SODR) Set Output Data Mask */
#define PIO_SODR_P4                         PIO_SODR_P4_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_SODR_P4_Msk instead */
#define PIO_SODR_P5_Pos                     5                                              /**< (PIO_SODR) Set Output Data Position */
#define PIO_SODR_P5_Msk                     (0x1U << PIO_SODR_P5_Pos)                      /**< (PIO_SODR) Set Output Data Mask */
#define PIO_SODR_P5                         PIO_SODR_P5_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_SODR_P5_Msk instead */
#define PIO_SODR_P6_Pos                     6                                              /**< (PIO_SODR) Set Output Data Position */
#define PIO_SODR_P6_Msk                     (0x1U << PIO_SODR_P6_Pos)                      /**< (PIO_SODR) Set Output Data Mask */
#define PIO_SODR_P6                         PIO_SODR_P6_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_SODR_P6_Msk instead */
#define PIO_SODR_P7_Pos                     7                                              /**< (PIO_SODR) Set Output Data Position */
#define PIO_SODR_P7_Msk                     (0x1U << PIO_SODR_P7_Pos)                      /**< (PIO_SODR) Set Output Data Mask */
#define PIO_SODR_P7                         PIO_SODR_P7_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_SODR_P7_Msk instead */
#define PIO_SODR_P8_Pos                     8                                              /**< (PIO_SODR) Set Output Data Position */
#define PIO_SODR_P8_Msk                     (0x1U << PIO_SODR_P8_Pos)                      /**< (PIO_SODR) Set Output Data Mask */
#define PIO_SODR_P8                         PIO_SODR_P8_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_SODR_P8_Msk instead */
#define PIO_SODR_P9_Pos                     9                                              /**< (PIO_SODR) Set Output Data Position */
#define PIO_SODR_P9_Msk                     (0x1U << PIO_SODR_P9_Pos)                      /**< (PIO_SODR) Set Output Data Mask */
#define PIO_SODR_P9                         PIO_SODR_P9_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_SODR_P9_Msk instead */
#define PIO_SODR_P10_Pos                    10                                             /**< (PIO_SODR) Set Output Data Position */
#define PIO_SODR_P10_Msk                    (0x1U << PIO_SODR_P10_Pos)                     /**< (PIO_SODR) Set Output Data Mask */
#define PIO_SODR_P10                        PIO_SODR_P10_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_SODR_P10_Msk instead */
#define PIO_SODR_P11_Pos                    11                                             /**< (PIO_SODR) Set Output Data Position */
#define PIO_SODR_P11_Msk                    (0x1U << PIO_SODR_P11_Pos)                     /**< (PIO_SODR) Set Output Data Mask */
#define PIO_SODR_P11                        PIO_SODR_P11_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_SODR_P11_Msk instead */
#define PIO_SODR_P12_Pos                    12                                             /**< (PIO_SODR) Set Output Data Position */
#define PIO_SODR_P12_Msk                    (0x1U << PIO_SODR_P12_Pos)                     /**< (PIO_SODR) Set Output Data Mask */
#define PIO_SODR_P12                        PIO_SODR_P12_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_SODR_P12_Msk instead */
#define PIO_SODR_P13_Pos                    13                                             /**< (PIO_SODR) Set Output Data Position */
#define PIO_SODR_P13_Msk                    (0x1U << PIO_SODR_P13_Pos)                     /**< (PIO_SODR) Set Output Data Mask */
#define PIO_SODR_P13                        PIO_SODR_P13_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_SODR_P13_Msk instead */
#define PIO_SODR_P14_Pos                    14                                             /**< (PIO_SODR) Set Output Data Position */
#define PIO_SODR_P14_Msk                    (0x1U << PIO_SODR_P14_Pos)                     /**< (PIO_SODR) Set Output Data Mask */
#define PIO_SODR_P14                        PIO_SODR_P14_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_SODR_P14_Msk instead */
#define PIO_SODR_P15_Pos                    15                                             /**< (PIO_SODR) Set Output Data Position */
#define PIO_SODR_P15_Msk                    (0x1U << PIO_SODR_P15_Pos)                     /**< (PIO_SODR) Set Output Data Mask */
#define PIO_SODR_P15                        PIO_SODR_P15_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_SODR_P15_Msk instead */
#define PIO_SODR_P16_Pos                    16                                             /**< (PIO_SODR) Set Output Data Position */
#define PIO_SODR_P16_Msk                    (0x1U << PIO_SODR_P16_Pos)                     /**< (PIO_SODR) Set Output Data Mask */
#define PIO_SODR_P16                        PIO_SODR_P16_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_SODR_P16_Msk instead */
#define PIO_SODR_P17_Pos                    17                                             /**< (PIO_SODR) Set Output Data Position */
#define PIO_SODR_P17_Msk                    (0x1U << PIO_SODR_P17_Pos)                     /**< (PIO_SODR) Set Output Data Mask */
#define PIO_SODR_P17                        PIO_SODR_P17_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_SODR_P17_Msk instead */
#define PIO_SODR_P18_Pos                    18                                             /**< (PIO_SODR) Set Output Data Position */
#define PIO_SODR_P18_Msk                    (0x1U << PIO_SODR_P18_Pos)                     /**< (PIO_SODR) Set Output Data Mask */
#define PIO_SODR_P18                        PIO_SODR_P18_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_SODR_P18_Msk instead */
#define PIO_SODR_P19_Pos                    19                                             /**< (PIO_SODR) Set Output Data Position */
#define PIO_SODR_P19_Msk                    (0x1U << PIO_SODR_P19_Pos)                     /**< (PIO_SODR) Set Output Data Mask */
#define PIO_SODR_P19                        PIO_SODR_P19_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_SODR_P19_Msk instead */
#define PIO_SODR_P20_Pos                    20                                             /**< (PIO_SODR) Set Output Data Position */
#define PIO_SODR_P20_Msk                    (0x1U << PIO_SODR_P20_Pos)                     /**< (PIO_SODR) Set Output Data Mask */
#define PIO_SODR_P20                        PIO_SODR_P20_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_SODR_P20_Msk instead */
#define PIO_SODR_P21_Pos                    21                                             /**< (PIO_SODR) Set Output Data Position */
#define PIO_SODR_P21_Msk                    (0x1U << PIO_SODR_P21_Pos)                     /**< (PIO_SODR) Set Output Data Mask */
#define PIO_SODR_P21                        PIO_SODR_P21_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_SODR_P21_Msk instead */
#define PIO_SODR_P22_Pos                    22                                             /**< (PIO_SODR) Set Output Data Position */
#define PIO_SODR_P22_Msk                    (0x1U << PIO_SODR_P22_Pos)                     /**< (PIO_SODR) Set Output Data Mask */
#define PIO_SODR_P22                        PIO_SODR_P22_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_SODR_P22_Msk instead */
#define PIO_SODR_P23_Pos                    23                                             /**< (PIO_SODR) Set Output Data Position */
#define PIO_SODR_P23_Msk                    (0x1U << PIO_SODR_P23_Pos)                     /**< (PIO_SODR) Set Output Data Mask */
#define PIO_SODR_P23                        PIO_SODR_P23_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_SODR_P23_Msk instead */
#define PIO_SODR_P24_Pos                    24                                             /**< (PIO_SODR) Set Output Data Position */
#define PIO_SODR_P24_Msk                    (0x1U << PIO_SODR_P24_Pos)                     /**< (PIO_SODR) Set Output Data Mask */
#define PIO_SODR_P24                        PIO_SODR_P24_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_SODR_P24_Msk instead */
#define PIO_SODR_P25_Pos                    25                                             /**< (PIO_SODR) Set Output Data Position */
#define PIO_SODR_P25_Msk                    (0x1U << PIO_SODR_P25_Pos)                     /**< (PIO_SODR) Set Output Data Mask */
#define PIO_SODR_P25                        PIO_SODR_P25_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_SODR_P25_Msk instead */
#define PIO_SODR_P26_Pos                    26                                             /**< (PIO_SODR) Set Output Data Position */
#define PIO_SODR_P26_Msk                    (0x1U << PIO_SODR_P26_Pos)                     /**< (PIO_SODR) Set Output Data Mask */
#define PIO_SODR_P26                        PIO_SODR_P26_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_SODR_P26_Msk instead */
#define PIO_SODR_P27_Pos                    27                                             /**< (PIO_SODR) Set Output Data Position */
#define PIO_SODR_P27_Msk                    (0x1U << PIO_SODR_P27_Pos)                     /**< (PIO_SODR) Set Output Data Mask */
#define PIO_SODR_P27                        PIO_SODR_P27_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_SODR_P27_Msk instead */
#define PIO_SODR_P28_Pos                    28                                             /**< (PIO_SODR) Set Output Data Position */
#define PIO_SODR_P28_Msk                    (0x1U << PIO_SODR_P28_Pos)                     /**< (PIO_SODR) Set Output Data Mask */
#define PIO_SODR_P28                        PIO_SODR_P28_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_SODR_P28_Msk instead */
#define PIO_SODR_P29_Pos                    29                                             /**< (PIO_SODR) Set Output Data Position */
#define PIO_SODR_P29_Msk                    (0x1U << PIO_SODR_P29_Pos)                     /**< (PIO_SODR) Set Output Data Mask */
#define PIO_SODR_P29                        PIO_SODR_P29_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_SODR_P29_Msk instead */
#define PIO_SODR_P30_Pos                    30                                             /**< (PIO_SODR) Set Output Data Position */
#define PIO_SODR_P30_Msk                    (0x1U << PIO_SODR_P30_Pos)                     /**< (PIO_SODR) Set Output Data Mask */
#define PIO_SODR_P30                        PIO_SODR_P30_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_SODR_P30_Msk instead */
#define PIO_SODR_P31_Pos                    31                                             /**< (PIO_SODR) Set Output Data Position */
#define PIO_SODR_P31_Msk                    (0x1U << PIO_SODR_P31_Pos)                     /**< (PIO_SODR) Set Output Data Mask */
#define PIO_SODR_P31                        PIO_SODR_P31_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_SODR_P31_Msk instead */
#define PIO_SODR_P_Pos                      0                                              /**< (PIO_SODR Position) Set Output Data */
#define PIO_SODR_P_Msk                      (0xFFFFFFFFU << PIO_SODR_P_Pos)                /**< (PIO_SODR Mask) P */
#define PIO_SODR_P(value)                   (PIO_SODR_P_Msk & ((value) << PIO_SODR_P_Pos))  
#define PIO_SODR_MASK                       (0xFFFFFFFFU)                                  /**< \deprecated (PIO_SODR) Register MASK  (Use PIO_SODR_Msk instead)  */
#define PIO_SODR_Msk                        (0xFFFFFFFFU)                                  /**< (PIO_SODR) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- PIO_CODR : (PIO Offset: 0x34) (/W 32) Clear Output Data Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t P0:1;                      /**< bit:      0  Clear Output Data                        */
    uint32_t P1:1;                      /**< bit:      1  Clear Output Data                        */
    uint32_t P2:1;                      /**< bit:      2  Clear Output Data                        */
    uint32_t P3:1;                      /**< bit:      3  Clear Output Data                        */
    uint32_t P4:1;                      /**< bit:      4  Clear Output Data                        */
    uint32_t P5:1;                      /**< bit:      5  Clear Output Data                        */
    uint32_t P6:1;                      /**< bit:      6  Clear Output Data                        */
    uint32_t P7:1;                      /**< bit:      7  Clear Output Data                        */
    uint32_t P8:1;                      /**< bit:      8  Clear Output Data                        */
    uint32_t P9:1;                      /**< bit:      9  Clear Output Data                        */
    uint32_t P10:1;                     /**< bit:     10  Clear Output Data                        */
    uint32_t P11:1;                     /**< bit:     11  Clear Output Data                        */
    uint32_t P12:1;                     /**< bit:     12  Clear Output Data                        */
    uint32_t P13:1;                     /**< bit:     13  Clear Output Data                        */
    uint32_t P14:1;                     /**< bit:     14  Clear Output Data                        */
    uint32_t P15:1;                     /**< bit:     15  Clear Output Data                        */
    uint32_t P16:1;                     /**< bit:     16  Clear Output Data                        */
    uint32_t P17:1;                     /**< bit:     17  Clear Output Data                        */
    uint32_t P18:1;                     /**< bit:     18  Clear Output Data                        */
    uint32_t P19:1;                     /**< bit:     19  Clear Output Data                        */
    uint32_t P20:1;                     /**< bit:     20  Clear Output Data                        */
    uint32_t P21:1;                     /**< bit:     21  Clear Output Data                        */
    uint32_t P22:1;                     /**< bit:     22  Clear Output Data                        */
    uint32_t P23:1;                     /**< bit:     23  Clear Output Data                        */
    uint32_t P24:1;                     /**< bit:     24  Clear Output Data                        */
    uint32_t P25:1;                     /**< bit:     25  Clear Output Data                        */
    uint32_t P26:1;                     /**< bit:     26  Clear Output Data                        */
    uint32_t P27:1;                     /**< bit:     27  Clear Output Data                        */
    uint32_t P28:1;                     /**< bit:     28  Clear Output Data                        */
    uint32_t P29:1;                     /**< bit:     29  Clear Output Data                        */
    uint32_t P30:1;                     /**< bit:     30  Clear Output Data                        */
    uint32_t P31:1;                     /**< bit:     31  Clear Output Data                        */
  } bit;                                /**< Structure used for bit  access */
  struct {
    uint32_t P:32;                      /**< bit:  0..31  Clear Output Data                        */
  } vec;                                /**< Structure used for vec  access  */
  uint32_t reg;                         /**< Type used for register access */
} PIO_CODR_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define PIO_CODR_OFFSET                     (0x34)                                        /**<  (PIO_CODR) Clear Output Data Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define PIO_CODR_P0_Pos                     0                                              /**< (PIO_CODR) Clear Output Data Position */
#define PIO_CODR_P0_Msk                     (0x1U << PIO_CODR_P0_Pos)                      /**< (PIO_CODR) Clear Output Data Mask */
#define PIO_CODR_P0                         PIO_CODR_P0_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_CODR_P0_Msk instead */
#define PIO_CODR_P1_Pos                     1                                              /**< (PIO_CODR) Clear Output Data Position */
#define PIO_CODR_P1_Msk                     (0x1U << PIO_CODR_P1_Pos)                      /**< (PIO_CODR) Clear Output Data Mask */
#define PIO_CODR_P1                         PIO_CODR_P1_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_CODR_P1_Msk instead */
#define PIO_CODR_P2_Pos                     2                                              /**< (PIO_CODR) Clear Output Data Position */
#define PIO_CODR_P2_Msk                     (0x1U << PIO_CODR_P2_Pos)                      /**< (PIO_CODR) Clear Output Data Mask */
#define PIO_CODR_P2                         PIO_CODR_P2_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_CODR_P2_Msk instead */
#define PIO_CODR_P3_Pos                     3                                              /**< (PIO_CODR) Clear Output Data Position */
#define PIO_CODR_P3_Msk                     (0x1U << PIO_CODR_P3_Pos)                      /**< (PIO_CODR) Clear Output Data Mask */
#define PIO_CODR_P3                         PIO_CODR_P3_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_CODR_P3_Msk instead */
#define PIO_CODR_P4_Pos                     4                                              /**< (PIO_CODR) Clear Output Data Position */
#define PIO_CODR_P4_Msk                     (0x1U << PIO_CODR_P4_Pos)                      /**< (PIO_CODR) Clear Output Data Mask */
#define PIO_CODR_P4                         PIO_CODR_P4_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_CODR_P4_Msk instead */
#define PIO_CODR_P5_Pos                     5                                              /**< (PIO_CODR) Clear Output Data Position */
#define PIO_CODR_P5_Msk                     (0x1U << PIO_CODR_P5_Pos)                      /**< (PIO_CODR) Clear Output Data Mask */
#define PIO_CODR_P5                         PIO_CODR_P5_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_CODR_P5_Msk instead */
#define PIO_CODR_P6_Pos                     6                                              /**< (PIO_CODR) Clear Output Data Position */
#define PIO_CODR_P6_Msk                     (0x1U << PIO_CODR_P6_Pos)                      /**< (PIO_CODR) Clear Output Data Mask */
#define PIO_CODR_P6                         PIO_CODR_P6_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_CODR_P6_Msk instead */
#define PIO_CODR_P7_Pos                     7                                              /**< (PIO_CODR) Clear Output Data Position */
#define PIO_CODR_P7_Msk                     (0x1U << PIO_CODR_P7_Pos)                      /**< (PIO_CODR) Clear Output Data Mask */
#define PIO_CODR_P7                         PIO_CODR_P7_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_CODR_P7_Msk instead */
#define PIO_CODR_P8_Pos                     8                                              /**< (PIO_CODR) Clear Output Data Position */
#define PIO_CODR_P8_Msk                     (0x1U << PIO_CODR_P8_Pos)                      /**< (PIO_CODR) Clear Output Data Mask */
#define PIO_CODR_P8                         PIO_CODR_P8_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_CODR_P8_Msk instead */
#define PIO_CODR_P9_Pos                     9                                              /**< (PIO_CODR) Clear Output Data Position */
#define PIO_CODR_P9_Msk                     (0x1U << PIO_CODR_P9_Pos)                      /**< (PIO_CODR) Clear Output Data Mask */
#define PIO_CODR_P9                         PIO_CODR_P9_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_CODR_P9_Msk instead */
#define PIO_CODR_P10_Pos                    10                                             /**< (PIO_CODR) Clear Output Data Position */
#define PIO_CODR_P10_Msk                    (0x1U << PIO_CODR_P10_Pos)                     /**< (PIO_CODR) Clear Output Data Mask */
#define PIO_CODR_P10                        PIO_CODR_P10_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_CODR_P10_Msk instead */
#define PIO_CODR_P11_Pos                    11                                             /**< (PIO_CODR) Clear Output Data Position */
#define PIO_CODR_P11_Msk                    (0x1U << PIO_CODR_P11_Pos)                     /**< (PIO_CODR) Clear Output Data Mask */
#define PIO_CODR_P11                        PIO_CODR_P11_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_CODR_P11_Msk instead */
#define PIO_CODR_P12_Pos                    12                                             /**< (PIO_CODR) Clear Output Data Position */
#define PIO_CODR_P12_Msk                    (0x1U << PIO_CODR_P12_Pos)                     /**< (PIO_CODR) Clear Output Data Mask */
#define PIO_CODR_P12                        PIO_CODR_P12_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_CODR_P12_Msk instead */
#define PIO_CODR_P13_Pos                    13                                             /**< (PIO_CODR) Clear Output Data Position */
#define PIO_CODR_P13_Msk                    (0x1U << PIO_CODR_P13_Pos)                     /**< (PIO_CODR) Clear Output Data Mask */
#define PIO_CODR_P13                        PIO_CODR_P13_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_CODR_P13_Msk instead */
#define PIO_CODR_P14_Pos                    14                                             /**< (PIO_CODR) Clear Output Data Position */
#define PIO_CODR_P14_Msk                    (0x1U << PIO_CODR_P14_Pos)                     /**< (PIO_CODR) Clear Output Data Mask */
#define PIO_CODR_P14                        PIO_CODR_P14_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_CODR_P14_Msk instead */
#define PIO_CODR_P15_Pos                    15                                             /**< (PIO_CODR) Clear Output Data Position */
#define PIO_CODR_P15_Msk                    (0x1U << PIO_CODR_P15_Pos)                     /**< (PIO_CODR) Clear Output Data Mask */
#define PIO_CODR_P15                        PIO_CODR_P15_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_CODR_P15_Msk instead */
#define PIO_CODR_P16_Pos                    16                                             /**< (PIO_CODR) Clear Output Data Position */
#define PIO_CODR_P16_Msk                    (0x1U << PIO_CODR_P16_Pos)                     /**< (PIO_CODR) Clear Output Data Mask */
#define PIO_CODR_P16                        PIO_CODR_P16_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_CODR_P16_Msk instead */
#define PIO_CODR_P17_Pos                    17                                             /**< (PIO_CODR) Clear Output Data Position */
#define PIO_CODR_P17_Msk                    (0x1U << PIO_CODR_P17_Pos)                     /**< (PIO_CODR) Clear Output Data Mask */
#define PIO_CODR_P17                        PIO_CODR_P17_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_CODR_P17_Msk instead */
#define PIO_CODR_P18_Pos                    18                                             /**< (PIO_CODR) Clear Output Data Position */
#define PIO_CODR_P18_Msk                    (0x1U << PIO_CODR_P18_Pos)                     /**< (PIO_CODR) Clear Output Data Mask */
#define PIO_CODR_P18                        PIO_CODR_P18_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_CODR_P18_Msk instead */
#define PIO_CODR_P19_Pos                    19                                             /**< (PIO_CODR) Clear Output Data Position */
#define PIO_CODR_P19_Msk                    (0x1U << PIO_CODR_P19_Pos)                     /**< (PIO_CODR) Clear Output Data Mask */
#define PIO_CODR_P19                        PIO_CODR_P19_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_CODR_P19_Msk instead */
#define PIO_CODR_P20_Pos                    20                                             /**< (PIO_CODR) Clear Output Data Position */
#define PIO_CODR_P20_Msk                    (0x1U << PIO_CODR_P20_Pos)                     /**< (PIO_CODR) Clear Output Data Mask */
#define PIO_CODR_P20                        PIO_CODR_P20_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_CODR_P20_Msk instead */
#define PIO_CODR_P21_Pos                    21                                             /**< (PIO_CODR) Clear Output Data Position */
#define PIO_CODR_P21_Msk                    (0x1U << PIO_CODR_P21_Pos)                     /**< (PIO_CODR) Clear Output Data Mask */
#define PIO_CODR_P21                        PIO_CODR_P21_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_CODR_P21_Msk instead */
#define PIO_CODR_P22_Pos                    22                                             /**< (PIO_CODR) Clear Output Data Position */
#define PIO_CODR_P22_Msk                    (0x1U << PIO_CODR_P22_Pos)                     /**< (PIO_CODR) Clear Output Data Mask */
#define PIO_CODR_P22                        PIO_CODR_P22_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_CODR_P22_Msk instead */
#define PIO_CODR_P23_Pos                    23                                             /**< (PIO_CODR) Clear Output Data Position */
#define PIO_CODR_P23_Msk                    (0x1U << PIO_CODR_P23_Pos)                     /**< (PIO_CODR) Clear Output Data Mask */
#define PIO_CODR_P23                        PIO_CODR_P23_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_CODR_P23_Msk instead */
#define PIO_CODR_P24_Pos                    24                                             /**< (PIO_CODR) Clear Output Data Position */
#define PIO_CODR_P24_Msk                    (0x1U << PIO_CODR_P24_Pos)                     /**< (PIO_CODR) Clear Output Data Mask */
#define PIO_CODR_P24                        PIO_CODR_P24_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_CODR_P24_Msk instead */
#define PIO_CODR_P25_Pos                    25                                             /**< (PIO_CODR) Clear Output Data Position */
#define PIO_CODR_P25_Msk                    (0x1U << PIO_CODR_P25_Pos)                     /**< (PIO_CODR) Clear Output Data Mask */
#define PIO_CODR_P25                        PIO_CODR_P25_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_CODR_P25_Msk instead */
#define PIO_CODR_P26_Pos                    26                                             /**< (PIO_CODR) Clear Output Data Position */
#define PIO_CODR_P26_Msk                    (0x1U << PIO_CODR_P26_Pos)                     /**< (PIO_CODR) Clear Output Data Mask */
#define PIO_CODR_P26                        PIO_CODR_P26_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_CODR_P26_Msk instead */
#define PIO_CODR_P27_Pos                    27                                             /**< (PIO_CODR) Clear Output Data Position */
#define PIO_CODR_P27_Msk                    (0x1U << PIO_CODR_P27_Pos)                     /**< (PIO_CODR) Clear Output Data Mask */
#define PIO_CODR_P27                        PIO_CODR_P27_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_CODR_P27_Msk instead */
#define PIO_CODR_P28_Pos                    28                                             /**< (PIO_CODR) Clear Output Data Position */
#define PIO_CODR_P28_Msk                    (0x1U << PIO_CODR_P28_Pos)                     /**< (PIO_CODR) Clear Output Data Mask */
#define PIO_CODR_P28                        PIO_CODR_P28_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_CODR_P28_Msk instead */
#define PIO_CODR_P29_Pos                    29                                             /**< (PIO_CODR) Clear Output Data Position */
#define PIO_CODR_P29_Msk                    (0x1U << PIO_CODR_P29_Pos)                     /**< (PIO_CODR) Clear Output Data Mask */
#define PIO_CODR_P29                        PIO_CODR_P29_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_CODR_P29_Msk instead */
#define PIO_CODR_P30_Pos                    30                                             /**< (PIO_CODR) Clear Output Data Position */
#define PIO_CODR_P30_Msk                    (0x1U << PIO_CODR_P30_Pos)                     /**< (PIO_CODR) Clear Output Data Mask */
#define PIO_CODR_P30                        PIO_CODR_P30_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_CODR_P30_Msk instead */
#define PIO_CODR_P31_Pos                    31                                             /**< (PIO_CODR) Clear Output Data Position */
#define PIO_CODR_P31_Msk                    (0x1U << PIO_CODR_P31_Pos)                     /**< (PIO_CODR) Clear Output Data Mask */
#define PIO_CODR_P31                        PIO_CODR_P31_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_CODR_P31_Msk instead */
#define PIO_CODR_P_Pos                      0                                              /**< (PIO_CODR Position) Clear Output Data */
#define PIO_CODR_P_Msk                      (0xFFFFFFFFU << PIO_CODR_P_Pos)                /**< (PIO_CODR Mask) P */
#define PIO_CODR_P(value)                   (PIO_CODR_P_Msk & ((value) << PIO_CODR_P_Pos))  
#define PIO_CODR_MASK                       (0xFFFFFFFFU)                                  /**< \deprecated (PIO_CODR) Register MASK  (Use PIO_CODR_Msk instead)  */
#define PIO_CODR_Msk                        (0xFFFFFFFFU)                                  /**< (PIO_CODR) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- PIO_ODSR : (PIO Offset: 0x38) (R/W 32) Output Data Status Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t P0:1;                      /**< bit:      0  Output Data Status                       */
    uint32_t P1:1;                      /**< bit:      1  Output Data Status                       */
    uint32_t P2:1;                      /**< bit:      2  Output Data Status                       */
    uint32_t P3:1;                      /**< bit:      3  Output Data Status                       */
    uint32_t P4:1;                      /**< bit:      4  Output Data Status                       */
    uint32_t P5:1;                      /**< bit:      5  Output Data Status                       */
    uint32_t P6:1;                      /**< bit:      6  Output Data Status                       */
    uint32_t P7:1;                      /**< bit:      7  Output Data Status                       */
    uint32_t P8:1;                      /**< bit:      8  Output Data Status                       */
    uint32_t P9:1;                      /**< bit:      9  Output Data Status                       */
    uint32_t P10:1;                     /**< bit:     10  Output Data Status                       */
    uint32_t P11:1;                     /**< bit:     11  Output Data Status                       */
    uint32_t P12:1;                     /**< bit:     12  Output Data Status                       */
    uint32_t P13:1;                     /**< bit:     13  Output Data Status                       */
    uint32_t P14:1;                     /**< bit:     14  Output Data Status                       */
    uint32_t P15:1;                     /**< bit:     15  Output Data Status                       */
    uint32_t P16:1;                     /**< bit:     16  Output Data Status                       */
    uint32_t P17:1;                     /**< bit:     17  Output Data Status                       */
    uint32_t P18:1;                     /**< bit:     18  Output Data Status                       */
    uint32_t P19:1;                     /**< bit:     19  Output Data Status                       */
    uint32_t P20:1;                     /**< bit:     20  Output Data Status                       */
    uint32_t P21:1;                     /**< bit:     21  Output Data Status                       */
    uint32_t P22:1;                     /**< bit:     22  Output Data Status                       */
    uint32_t P23:1;                     /**< bit:     23  Output Data Status                       */
    uint32_t P24:1;                     /**< bit:     24  Output Data Status                       */
    uint32_t P25:1;                     /**< bit:     25  Output Data Status                       */
    uint32_t P26:1;                     /**< bit:     26  Output Data Status                       */
    uint32_t P27:1;                     /**< bit:     27  Output Data Status                       */
    uint32_t P28:1;                     /**< bit:     28  Output Data Status                       */
    uint32_t P29:1;                     /**< bit:     29  Output Data Status                       */
    uint32_t P30:1;                     /**< bit:     30  Output Data Status                       */
    uint32_t P31:1;                     /**< bit:     31  Output Data Status                       */
  } bit;                                /**< Structure used for bit  access */
  struct {
    uint32_t P:32;                      /**< bit:  0..31  Output Data Status                       */
  } vec;                                /**< Structure used for vec  access  */
  uint32_t reg;                         /**< Type used for register access */
} PIO_ODSR_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define PIO_ODSR_OFFSET                     (0x38)                                        /**<  (PIO_ODSR) Output Data Status Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define PIO_ODSR_P0_Pos                     0                                              /**< (PIO_ODSR) Output Data Status Position */
#define PIO_ODSR_P0_Msk                     (0x1U << PIO_ODSR_P0_Pos)                      /**< (PIO_ODSR) Output Data Status Mask */
#define PIO_ODSR_P0                         PIO_ODSR_P0_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_ODSR_P0_Msk instead */
#define PIO_ODSR_P1_Pos                     1                                              /**< (PIO_ODSR) Output Data Status Position */
#define PIO_ODSR_P1_Msk                     (0x1U << PIO_ODSR_P1_Pos)                      /**< (PIO_ODSR) Output Data Status Mask */
#define PIO_ODSR_P1                         PIO_ODSR_P1_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_ODSR_P1_Msk instead */
#define PIO_ODSR_P2_Pos                     2                                              /**< (PIO_ODSR) Output Data Status Position */
#define PIO_ODSR_P2_Msk                     (0x1U << PIO_ODSR_P2_Pos)                      /**< (PIO_ODSR) Output Data Status Mask */
#define PIO_ODSR_P2                         PIO_ODSR_P2_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_ODSR_P2_Msk instead */
#define PIO_ODSR_P3_Pos                     3                                              /**< (PIO_ODSR) Output Data Status Position */
#define PIO_ODSR_P3_Msk                     (0x1U << PIO_ODSR_P3_Pos)                      /**< (PIO_ODSR) Output Data Status Mask */
#define PIO_ODSR_P3                         PIO_ODSR_P3_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_ODSR_P3_Msk instead */
#define PIO_ODSR_P4_Pos                     4                                              /**< (PIO_ODSR) Output Data Status Position */
#define PIO_ODSR_P4_Msk                     (0x1U << PIO_ODSR_P4_Pos)                      /**< (PIO_ODSR) Output Data Status Mask */
#define PIO_ODSR_P4                         PIO_ODSR_P4_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_ODSR_P4_Msk instead */
#define PIO_ODSR_P5_Pos                     5                                              /**< (PIO_ODSR) Output Data Status Position */
#define PIO_ODSR_P5_Msk                     (0x1U << PIO_ODSR_P5_Pos)                      /**< (PIO_ODSR) Output Data Status Mask */
#define PIO_ODSR_P5                         PIO_ODSR_P5_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_ODSR_P5_Msk instead */
#define PIO_ODSR_P6_Pos                     6                                              /**< (PIO_ODSR) Output Data Status Position */
#define PIO_ODSR_P6_Msk                     (0x1U << PIO_ODSR_P6_Pos)                      /**< (PIO_ODSR) Output Data Status Mask */
#define PIO_ODSR_P6                         PIO_ODSR_P6_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_ODSR_P6_Msk instead */
#define PIO_ODSR_P7_Pos                     7                                              /**< (PIO_ODSR) Output Data Status Position */
#define PIO_ODSR_P7_Msk                     (0x1U << PIO_ODSR_P7_Pos)                      /**< (PIO_ODSR) Output Data Status Mask */
#define PIO_ODSR_P7                         PIO_ODSR_P7_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_ODSR_P7_Msk instead */
#define PIO_ODSR_P8_Pos                     8                                              /**< (PIO_ODSR) Output Data Status Position */
#define PIO_ODSR_P8_Msk                     (0x1U << PIO_ODSR_P8_Pos)                      /**< (PIO_ODSR) Output Data Status Mask */
#define PIO_ODSR_P8                         PIO_ODSR_P8_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_ODSR_P8_Msk instead */
#define PIO_ODSR_P9_Pos                     9                                              /**< (PIO_ODSR) Output Data Status Position */
#define PIO_ODSR_P9_Msk                     (0x1U << PIO_ODSR_P9_Pos)                      /**< (PIO_ODSR) Output Data Status Mask */
#define PIO_ODSR_P9                         PIO_ODSR_P9_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_ODSR_P9_Msk instead */
#define PIO_ODSR_P10_Pos                    10                                             /**< (PIO_ODSR) Output Data Status Position */
#define PIO_ODSR_P10_Msk                    (0x1U << PIO_ODSR_P10_Pos)                     /**< (PIO_ODSR) Output Data Status Mask */
#define PIO_ODSR_P10                        PIO_ODSR_P10_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_ODSR_P10_Msk instead */
#define PIO_ODSR_P11_Pos                    11                                             /**< (PIO_ODSR) Output Data Status Position */
#define PIO_ODSR_P11_Msk                    (0x1U << PIO_ODSR_P11_Pos)                     /**< (PIO_ODSR) Output Data Status Mask */
#define PIO_ODSR_P11                        PIO_ODSR_P11_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_ODSR_P11_Msk instead */
#define PIO_ODSR_P12_Pos                    12                                             /**< (PIO_ODSR) Output Data Status Position */
#define PIO_ODSR_P12_Msk                    (0x1U << PIO_ODSR_P12_Pos)                     /**< (PIO_ODSR) Output Data Status Mask */
#define PIO_ODSR_P12                        PIO_ODSR_P12_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_ODSR_P12_Msk instead */
#define PIO_ODSR_P13_Pos                    13                                             /**< (PIO_ODSR) Output Data Status Position */
#define PIO_ODSR_P13_Msk                    (0x1U << PIO_ODSR_P13_Pos)                     /**< (PIO_ODSR) Output Data Status Mask */
#define PIO_ODSR_P13                        PIO_ODSR_P13_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_ODSR_P13_Msk instead */
#define PIO_ODSR_P14_Pos                    14                                             /**< (PIO_ODSR) Output Data Status Position */
#define PIO_ODSR_P14_Msk                    (0x1U << PIO_ODSR_P14_Pos)                     /**< (PIO_ODSR) Output Data Status Mask */
#define PIO_ODSR_P14                        PIO_ODSR_P14_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_ODSR_P14_Msk instead */
#define PIO_ODSR_P15_Pos                    15                                             /**< (PIO_ODSR) Output Data Status Position */
#define PIO_ODSR_P15_Msk                    (0x1U << PIO_ODSR_P15_Pos)                     /**< (PIO_ODSR) Output Data Status Mask */
#define PIO_ODSR_P15                        PIO_ODSR_P15_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_ODSR_P15_Msk instead */
#define PIO_ODSR_P16_Pos                    16                                             /**< (PIO_ODSR) Output Data Status Position */
#define PIO_ODSR_P16_Msk                    (0x1U << PIO_ODSR_P16_Pos)                     /**< (PIO_ODSR) Output Data Status Mask */
#define PIO_ODSR_P16                        PIO_ODSR_P16_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_ODSR_P16_Msk instead */
#define PIO_ODSR_P17_Pos                    17                                             /**< (PIO_ODSR) Output Data Status Position */
#define PIO_ODSR_P17_Msk                    (0x1U << PIO_ODSR_P17_Pos)                     /**< (PIO_ODSR) Output Data Status Mask */
#define PIO_ODSR_P17                        PIO_ODSR_P17_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_ODSR_P17_Msk instead */
#define PIO_ODSR_P18_Pos                    18                                             /**< (PIO_ODSR) Output Data Status Position */
#define PIO_ODSR_P18_Msk                    (0x1U << PIO_ODSR_P18_Pos)                     /**< (PIO_ODSR) Output Data Status Mask */
#define PIO_ODSR_P18                        PIO_ODSR_P18_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_ODSR_P18_Msk instead */
#define PIO_ODSR_P19_Pos                    19                                             /**< (PIO_ODSR) Output Data Status Position */
#define PIO_ODSR_P19_Msk                    (0x1U << PIO_ODSR_P19_Pos)                     /**< (PIO_ODSR) Output Data Status Mask */
#define PIO_ODSR_P19                        PIO_ODSR_P19_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_ODSR_P19_Msk instead */
#define PIO_ODSR_P20_Pos                    20                                             /**< (PIO_ODSR) Output Data Status Position */
#define PIO_ODSR_P20_Msk                    (0x1U << PIO_ODSR_P20_Pos)                     /**< (PIO_ODSR) Output Data Status Mask */
#define PIO_ODSR_P20                        PIO_ODSR_P20_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_ODSR_P20_Msk instead */
#define PIO_ODSR_P21_Pos                    21                                             /**< (PIO_ODSR) Output Data Status Position */
#define PIO_ODSR_P21_Msk                    (0x1U << PIO_ODSR_P21_Pos)                     /**< (PIO_ODSR) Output Data Status Mask */
#define PIO_ODSR_P21                        PIO_ODSR_P21_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_ODSR_P21_Msk instead */
#define PIO_ODSR_P22_Pos                    22                                             /**< (PIO_ODSR) Output Data Status Position */
#define PIO_ODSR_P22_Msk                    (0x1U << PIO_ODSR_P22_Pos)                     /**< (PIO_ODSR) Output Data Status Mask */
#define PIO_ODSR_P22                        PIO_ODSR_P22_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_ODSR_P22_Msk instead */
#define PIO_ODSR_P23_Pos                    23                                             /**< (PIO_ODSR) Output Data Status Position */
#define PIO_ODSR_P23_Msk                    (0x1U << PIO_ODSR_P23_Pos)                     /**< (PIO_ODSR) Output Data Status Mask */
#define PIO_ODSR_P23                        PIO_ODSR_P23_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_ODSR_P23_Msk instead */
#define PIO_ODSR_P24_Pos                    24                                             /**< (PIO_ODSR) Output Data Status Position */
#define PIO_ODSR_P24_Msk                    (0x1U << PIO_ODSR_P24_Pos)                     /**< (PIO_ODSR) Output Data Status Mask */
#define PIO_ODSR_P24                        PIO_ODSR_P24_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_ODSR_P24_Msk instead */
#define PIO_ODSR_P25_Pos                    25                                             /**< (PIO_ODSR) Output Data Status Position */
#define PIO_ODSR_P25_Msk                    (0x1U << PIO_ODSR_P25_Pos)                     /**< (PIO_ODSR) Output Data Status Mask */
#define PIO_ODSR_P25                        PIO_ODSR_P25_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_ODSR_P25_Msk instead */
#define PIO_ODSR_P26_Pos                    26                                             /**< (PIO_ODSR) Output Data Status Position */
#define PIO_ODSR_P26_Msk                    (0x1U << PIO_ODSR_P26_Pos)                     /**< (PIO_ODSR) Output Data Status Mask */
#define PIO_ODSR_P26                        PIO_ODSR_P26_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_ODSR_P26_Msk instead */
#define PIO_ODSR_P27_Pos                    27                                             /**< (PIO_ODSR) Output Data Status Position */
#define PIO_ODSR_P27_Msk                    (0x1U << PIO_ODSR_P27_Pos)                     /**< (PIO_ODSR) Output Data Status Mask */
#define PIO_ODSR_P27                        PIO_ODSR_P27_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_ODSR_P27_Msk instead */
#define PIO_ODSR_P28_Pos                    28                                             /**< (PIO_ODSR) Output Data Status Position */
#define PIO_ODSR_P28_Msk                    (0x1U << PIO_ODSR_P28_Pos)                     /**< (PIO_ODSR) Output Data Status Mask */
#define PIO_ODSR_P28                        PIO_ODSR_P28_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_ODSR_P28_Msk instead */
#define PIO_ODSR_P29_Pos                    29                                             /**< (PIO_ODSR) Output Data Status Position */
#define PIO_ODSR_P29_Msk                    (0x1U << PIO_ODSR_P29_Pos)                     /**< (PIO_ODSR) Output Data Status Mask */
#define PIO_ODSR_P29                        PIO_ODSR_P29_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_ODSR_P29_Msk instead */
#define PIO_ODSR_P30_Pos                    30                                             /**< (PIO_ODSR) Output Data Status Position */
#define PIO_ODSR_P30_Msk                    (0x1U << PIO_ODSR_P30_Pos)                     /**< (PIO_ODSR) Output Data Status Mask */
#define PIO_ODSR_P30                        PIO_ODSR_P30_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_ODSR_P30_Msk instead */
#define PIO_ODSR_P31_Pos                    31                                             /**< (PIO_ODSR) Output Data Status Position */
#define PIO_ODSR_P31_Msk                    (0x1U << PIO_ODSR_P31_Pos)                     /**< (PIO_ODSR) Output Data Status Mask */
#define PIO_ODSR_P31                        PIO_ODSR_P31_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_ODSR_P31_Msk instead */
#define PIO_ODSR_P_Pos                      0                                              /**< (PIO_ODSR Position) Output Data Status */
#define PIO_ODSR_P_Msk                      (0xFFFFFFFFU << PIO_ODSR_P_Pos)                /**< (PIO_ODSR Mask) P */
#define PIO_ODSR_P(value)                   (PIO_ODSR_P_Msk & ((value) << PIO_ODSR_P_Pos))  
#define PIO_ODSR_MASK                       (0xFFFFFFFFU)                                  /**< \deprecated (PIO_ODSR) Register MASK  (Use PIO_ODSR_Msk instead)  */
#define PIO_ODSR_Msk                        (0xFFFFFFFFU)                                  /**< (PIO_ODSR) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- PIO_PDSR : (PIO Offset: 0x3c) (R/ 32) Pin Data Status Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t P0:1;                      /**< bit:      0  Output Data Status                       */
    uint32_t P1:1;                      /**< bit:      1  Output Data Status                       */
    uint32_t P2:1;                      /**< bit:      2  Output Data Status                       */
    uint32_t P3:1;                      /**< bit:      3  Output Data Status                       */
    uint32_t P4:1;                      /**< bit:      4  Output Data Status                       */
    uint32_t P5:1;                      /**< bit:      5  Output Data Status                       */
    uint32_t P6:1;                      /**< bit:      6  Output Data Status                       */
    uint32_t P7:1;                      /**< bit:      7  Output Data Status                       */
    uint32_t P8:1;                      /**< bit:      8  Output Data Status                       */
    uint32_t P9:1;                      /**< bit:      9  Output Data Status                       */
    uint32_t P10:1;                     /**< bit:     10  Output Data Status                       */
    uint32_t P11:1;                     /**< bit:     11  Output Data Status                       */
    uint32_t P12:1;                     /**< bit:     12  Output Data Status                       */
    uint32_t P13:1;                     /**< bit:     13  Output Data Status                       */
    uint32_t P14:1;                     /**< bit:     14  Output Data Status                       */
    uint32_t P15:1;                     /**< bit:     15  Output Data Status                       */
    uint32_t P16:1;                     /**< bit:     16  Output Data Status                       */
    uint32_t P17:1;                     /**< bit:     17  Output Data Status                       */
    uint32_t P18:1;                     /**< bit:     18  Output Data Status                       */
    uint32_t P19:1;                     /**< bit:     19  Output Data Status                       */
    uint32_t P20:1;                     /**< bit:     20  Output Data Status                       */
    uint32_t P21:1;                     /**< bit:     21  Output Data Status                       */
    uint32_t P22:1;                     /**< bit:     22  Output Data Status                       */
    uint32_t P23:1;                     /**< bit:     23  Output Data Status                       */
    uint32_t P24:1;                     /**< bit:     24  Output Data Status                       */
    uint32_t P25:1;                     /**< bit:     25  Output Data Status                       */
    uint32_t P26:1;                     /**< bit:     26  Output Data Status                       */
    uint32_t P27:1;                     /**< bit:     27  Output Data Status                       */
    uint32_t P28:1;                     /**< bit:     28  Output Data Status                       */
    uint32_t P29:1;                     /**< bit:     29  Output Data Status                       */
    uint32_t P30:1;                     /**< bit:     30  Output Data Status                       */
    uint32_t P31:1;                     /**< bit:     31  Output Data Status                       */
  } bit;                                /**< Structure used for bit  access */
  struct {
    uint32_t P:32;                      /**< bit:  0..31  Output Data Status                       */
  } vec;                                /**< Structure used for vec  access  */
  uint32_t reg;                         /**< Type used for register access */
} PIO_PDSR_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define PIO_PDSR_OFFSET                     (0x3C)                                        /**<  (PIO_PDSR) Pin Data Status Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define PIO_PDSR_P0_Pos                     0                                              /**< (PIO_PDSR) Output Data Status Position */
#define PIO_PDSR_P0_Msk                     (0x1U << PIO_PDSR_P0_Pos)                      /**< (PIO_PDSR) Output Data Status Mask */
#define PIO_PDSR_P0                         PIO_PDSR_P0_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PDSR_P0_Msk instead */
#define PIO_PDSR_P1_Pos                     1                                              /**< (PIO_PDSR) Output Data Status Position */
#define PIO_PDSR_P1_Msk                     (0x1U << PIO_PDSR_P1_Pos)                      /**< (PIO_PDSR) Output Data Status Mask */
#define PIO_PDSR_P1                         PIO_PDSR_P1_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PDSR_P1_Msk instead */
#define PIO_PDSR_P2_Pos                     2                                              /**< (PIO_PDSR) Output Data Status Position */
#define PIO_PDSR_P2_Msk                     (0x1U << PIO_PDSR_P2_Pos)                      /**< (PIO_PDSR) Output Data Status Mask */
#define PIO_PDSR_P2                         PIO_PDSR_P2_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PDSR_P2_Msk instead */
#define PIO_PDSR_P3_Pos                     3                                              /**< (PIO_PDSR) Output Data Status Position */
#define PIO_PDSR_P3_Msk                     (0x1U << PIO_PDSR_P3_Pos)                      /**< (PIO_PDSR) Output Data Status Mask */
#define PIO_PDSR_P3                         PIO_PDSR_P3_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PDSR_P3_Msk instead */
#define PIO_PDSR_P4_Pos                     4                                              /**< (PIO_PDSR) Output Data Status Position */
#define PIO_PDSR_P4_Msk                     (0x1U << PIO_PDSR_P4_Pos)                      /**< (PIO_PDSR) Output Data Status Mask */
#define PIO_PDSR_P4                         PIO_PDSR_P4_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PDSR_P4_Msk instead */
#define PIO_PDSR_P5_Pos                     5                                              /**< (PIO_PDSR) Output Data Status Position */
#define PIO_PDSR_P5_Msk                     (0x1U << PIO_PDSR_P5_Pos)                      /**< (PIO_PDSR) Output Data Status Mask */
#define PIO_PDSR_P5                         PIO_PDSR_P5_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PDSR_P5_Msk instead */
#define PIO_PDSR_P6_Pos                     6                                              /**< (PIO_PDSR) Output Data Status Position */
#define PIO_PDSR_P6_Msk                     (0x1U << PIO_PDSR_P6_Pos)                      /**< (PIO_PDSR) Output Data Status Mask */
#define PIO_PDSR_P6                         PIO_PDSR_P6_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PDSR_P6_Msk instead */
#define PIO_PDSR_P7_Pos                     7                                              /**< (PIO_PDSR) Output Data Status Position */
#define PIO_PDSR_P7_Msk                     (0x1U << PIO_PDSR_P7_Pos)                      /**< (PIO_PDSR) Output Data Status Mask */
#define PIO_PDSR_P7                         PIO_PDSR_P7_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PDSR_P7_Msk instead */
#define PIO_PDSR_P8_Pos                     8                                              /**< (PIO_PDSR) Output Data Status Position */
#define PIO_PDSR_P8_Msk                     (0x1U << PIO_PDSR_P8_Pos)                      /**< (PIO_PDSR) Output Data Status Mask */
#define PIO_PDSR_P8                         PIO_PDSR_P8_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PDSR_P8_Msk instead */
#define PIO_PDSR_P9_Pos                     9                                              /**< (PIO_PDSR) Output Data Status Position */
#define PIO_PDSR_P9_Msk                     (0x1U << PIO_PDSR_P9_Pos)                      /**< (PIO_PDSR) Output Data Status Mask */
#define PIO_PDSR_P9                         PIO_PDSR_P9_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PDSR_P9_Msk instead */
#define PIO_PDSR_P10_Pos                    10                                             /**< (PIO_PDSR) Output Data Status Position */
#define PIO_PDSR_P10_Msk                    (0x1U << PIO_PDSR_P10_Pos)                     /**< (PIO_PDSR) Output Data Status Mask */
#define PIO_PDSR_P10                        PIO_PDSR_P10_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PDSR_P10_Msk instead */
#define PIO_PDSR_P11_Pos                    11                                             /**< (PIO_PDSR) Output Data Status Position */
#define PIO_PDSR_P11_Msk                    (0x1U << PIO_PDSR_P11_Pos)                     /**< (PIO_PDSR) Output Data Status Mask */
#define PIO_PDSR_P11                        PIO_PDSR_P11_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PDSR_P11_Msk instead */
#define PIO_PDSR_P12_Pos                    12                                             /**< (PIO_PDSR) Output Data Status Position */
#define PIO_PDSR_P12_Msk                    (0x1U << PIO_PDSR_P12_Pos)                     /**< (PIO_PDSR) Output Data Status Mask */
#define PIO_PDSR_P12                        PIO_PDSR_P12_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PDSR_P12_Msk instead */
#define PIO_PDSR_P13_Pos                    13                                             /**< (PIO_PDSR) Output Data Status Position */
#define PIO_PDSR_P13_Msk                    (0x1U << PIO_PDSR_P13_Pos)                     /**< (PIO_PDSR) Output Data Status Mask */
#define PIO_PDSR_P13                        PIO_PDSR_P13_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PDSR_P13_Msk instead */
#define PIO_PDSR_P14_Pos                    14                                             /**< (PIO_PDSR) Output Data Status Position */
#define PIO_PDSR_P14_Msk                    (0x1U << PIO_PDSR_P14_Pos)                     /**< (PIO_PDSR) Output Data Status Mask */
#define PIO_PDSR_P14                        PIO_PDSR_P14_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PDSR_P14_Msk instead */
#define PIO_PDSR_P15_Pos                    15                                             /**< (PIO_PDSR) Output Data Status Position */
#define PIO_PDSR_P15_Msk                    (0x1U << PIO_PDSR_P15_Pos)                     /**< (PIO_PDSR) Output Data Status Mask */
#define PIO_PDSR_P15                        PIO_PDSR_P15_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PDSR_P15_Msk instead */
#define PIO_PDSR_P16_Pos                    16                                             /**< (PIO_PDSR) Output Data Status Position */
#define PIO_PDSR_P16_Msk                    (0x1U << PIO_PDSR_P16_Pos)                     /**< (PIO_PDSR) Output Data Status Mask */
#define PIO_PDSR_P16                        PIO_PDSR_P16_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PDSR_P16_Msk instead */
#define PIO_PDSR_P17_Pos                    17                                             /**< (PIO_PDSR) Output Data Status Position */
#define PIO_PDSR_P17_Msk                    (0x1U << PIO_PDSR_P17_Pos)                     /**< (PIO_PDSR) Output Data Status Mask */
#define PIO_PDSR_P17                        PIO_PDSR_P17_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PDSR_P17_Msk instead */
#define PIO_PDSR_P18_Pos                    18                                             /**< (PIO_PDSR) Output Data Status Position */
#define PIO_PDSR_P18_Msk                    (0x1U << PIO_PDSR_P18_Pos)                     /**< (PIO_PDSR) Output Data Status Mask */
#define PIO_PDSR_P18                        PIO_PDSR_P18_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PDSR_P18_Msk instead */
#define PIO_PDSR_P19_Pos                    19                                             /**< (PIO_PDSR) Output Data Status Position */
#define PIO_PDSR_P19_Msk                    (0x1U << PIO_PDSR_P19_Pos)                     /**< (PIO_PDSR) Output Data Status Mask */
#define PIO_PDSR_P19                        PIO_PDSR_P19_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PDSR_P19_Msk instead */
#define PIO_PDSR_P20_Pos                    20                                             /**< (PIO_PDSR) Output Data Status Position */
#define PIO_PDSR_P20_Msk                    (0x1U << PIO_PDSR_P20_Pos)                     /**< (PIO_PDSR) Output Data Status Mask */
#define PIO_PDSR_P20                        PIO_PDSR_P20_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PDSR_P20_Msk instead */
#define PIO_PDSR_P21_Pos                    21                                             /**< (PIO_PDSR) Output Data Status Position */
#define PIO_PDSR_P21_Msk                    (0x1U << PIO_PDSR_P21_Pos)                     /**< (PIO_PDSR) Output Data Status Mask */
#define PIO_PDSR_P21                        PIO_PDSR_P21_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PDSR_P21_Msk instead */
#define PIO_PDSR_P22_Pos                    22                                             /**< (PIO_PDSR) Output Data Status Position */
#define PIO_PDSR_P22_Msk                    (0x1U << PIO_PDSR_P22_Pos)                     /**< (PIO_PDSR) Output Data Status Mask */
#define PIO_PDSR_P22                        PIO_PDSR_P22_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PDSR_P22_Msk instead */
#define PIO_PDSR_P23_Pos                    23                                             /**< (PIO_PDSR) Output Data Status Position */
#define PIO_PDSR_P23_Msk                    (0x1U << PIO_PDSR_P23_Pos)                     /**< (PIO_PDSR) Output Data Status Mask */
#define PIO_PDSR_P23                        PIO_PDSR_P23_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PDSR_P23_Msk instead */
#define PIO_PDSR_P24_Pos                    24                                             /**< (PIO_PDSR) Output Data Status Position */
#define PIO_PDSR_P24_Msk                    (0x1U << PIO_PDSR_P24_Pos)                     /**< (PIO_PDSR) Output Data Status Mask */
#define PIO_PDSR_P24                        PIO_PDSR_P24_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PDSR_P24_Msk instead */
#define PIO_PDSR_P25_Pos                    25                                             /**< (PIO_PDSR) Output Data Status Position */
#define PIO_PDSR_P25_Msk                    (0x1U << PIO_PDSR_P25_Pos)                     /**< (PIO_PDSR) Output Data Status Mask */
#define PIO_PDSR_P25                        PIO_PDSR_P25_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PDSR_P25_Msk instead */
#define PIO_PDSR_P26_Pos                    26                                             /**< (PIO_PDSR) Output Data Status Position */
#define PIO_PDSR_P26_Msk                    (0x1U << PIO_PDSR_P26_Pos)                     /**< (PIO_PDSR) Output Data Status Mask */
#define PIO_PDSR_P26                        PIO_PDSR_P26_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PDSR_P26_Msk instead */
#define PIO_PDSR_P27_Pos                    27                                             /**< (PIO_PDSR) Output Data Status Position */
#define PIO_PDSR_P27_Msk                    (0x1U << PIO_PDSR_P27_Pos)                     /**< (PIO_PDSR) Output Data Status Mask */
#define PIO_PDSR_P27                        PIO_PDSR_P27_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PDSR_P27_Msk instead */
#define PIO_PDSR_P28_Pos                    28                                             /**< (PIO_PDSR) Output Data Status Position */
#define PIO_PDSR_P28_Msk                    (0x1U << PIO_PDSR_P28_Pos)                     /**< (PIO_PDSR) Output Data Status Mask */
#define PIO_PDSR_P28                        PIO_PDSR_P28_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PDSR_P28_Msk instead */
#define PIO_PDSR_P29_Pos                    29                                             /**< (PIO_PDSR) Output Data Status Position */
#define PIO_PDSR_P29_Msk                    (0x1U << PIO_PDSR_P29_Pos)                     /**< (PIO_PDSR) Output Data Status Mask */
#define PIO_PDSR_P29                        PIO_PDSR_P29_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PDSR_P29_Msk instead */
#define PIO_PDSR_P30_Pos                    30                                             /**< (PIO_PDSR) Output Data Status Position */
#define PIO_PDSR_P30_Msk                    (0x1U << PIO_PDSR_P30_Pos)                     /**< (PIO_PDSR) Output Data Status Mask */
#define PIO_PDSR_P30                        PIO_PDSR_P30_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PDSR_P30_Msk instead */
#define PIO_PDSR_P31_Pos                    31                                             /**< (PIO_PDSR) Output Data Status Position */
#define PIO_PDSR_P31_Msk                    (0x1U << PIO_PDSR_P31_Pos)                     /**< (PIO_PDSR) Output Data Status Mask */
#define PIO_PDSR_P31                        PIO_PDSR_P31_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PDSR_P31_Msk instead */
#define PIO_PDSR_P_Pos                      0                                              /**< (PIO_PDSR Position) Output Data Status */
#define PIO_PDSR_P_Msk                      (0xFFFFFFFFU << PIO_PDSR_P_Pos)                /**< (PIO_PDSR Mask) P */
#define PIO_PDSR_P(value)                   (PIO_PDSR_P_Msk & ((value) << PIO_PDSR_P_Pos))  
#define PIO_PDSR_MASK                       (0xFFFFFFFFU)                                  /**< \deprecated (PIO_PDSR) Register MASK  (Use PIO_PDSR_Msk instead)  */
#define PIO_PDSR_Msk                        (0xFFFFFFFFU)                                  /**< (PIO_PDSR) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- PIO_IER : (PIO Offset: 0x40) (/W 32) Interrupt Enable Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t P0:1;                      /**< bit:      0  Input Change Interrupt Enable            */
    uint32_t P1:1;                      /**< bit:      1  Input Change Interrupt Enable            */
    uint32_t P2:1;                      /**< bit:      2  Input Change Interrupt Enable            */
    uint32_t P3:1;                      /**< bit:      3  Input Change Interrupt Enable            */
    uint32_t P4:1;                      /**< bit:      4  Input Change Interrupt Enable            */
    uint32_t P5:1;                      /**< bit:      5  Input Change Interrupt Enable            */
    uint32_t P6:1;                      /**< bit:      6  Input Change Interrupt Enable            */
    uint32_t P7:1;                      /**< bit:      7  Input Change Interrupt Enable            */
    uint32_t P8:1;                      /**< bit:      8  Input Change Interrupt Enable            */
    uint32_t P9:1;                      /**< bit:      9  Input Change Interrupt Enable            */
    uint32_t P10:1;                     /**< bit:     10  Input Change Interrupt Enable            */
    uint32_t P11:1;                     /**< bit:     11  Input Change Interrupt Enable            */
    uint32_t P12:1;                     /**< bit:     12  Input Change Interrupt Enable            */
    uint32_t P13:1;                     /**< bit:     13  Input Change Interrupt Enable            */
    uint32_t P14:1;                     /**< bit:     14  Input Change Interrupt Enable            */
    uint32_t P15:1;                     /**< bit:     15  Input Change Interrupt Enable            */
    uint32_t P16:1;                     /**< bit:     16  Input Change Interrupt Enable            */
    uint32_t P17:1;                     /**< bit:     17  Input Change Interrupt Enable            */
    uint32_t P18:1;                     /**< bit:     18  Input Change Interrupt Enable            */
    uint32_t P19:1;                     /**< bit:     19  Input Change Interrupt Enable            */
    uint32_t P20:1;                     /**< bit:     20  Input Change Interrupt Enable            */
    uint32_t P21:1;                     /**< bit:     21  Input Change Interrupt Enable            */
    uint32_t P22:1;                     /**< bit:     22  Input Change Interrupt Enable            */
    uint32_t P23:1;                     /**< bit:     23  Input Change Interrupt Enable            */
    uint32_t P24:1;                     /**< bit:     24  Input Change Interrupt Enable            */
    uint32_t P25:1;                     /**< bit:     25  Input Change Interrupt Enable            */
    uint32_t P26:1;                     /**< bit:     26  Input Change Interrupt Enable            */
    uint32_t P27:1;                     /**< bit:     27  Input Change Interrupt Enable            */
    uint32_t P28:1;                     /**< bit:     28  Input Change Interrupt Enable            */
    uint32_t P29:1;                     /**< bit:     29  Input Change Interrupt Enable            */
    uint32_t P30:1;                     /**< bit:     30  Input Change Interrupt Enable            */
    uint32_t P31:1;                     /**< bit:     31  Input Change Interrupt Enable            */
  } bit;                                /**< Structure used for bit  access */
  struct {
    uint32_t P:32;                      /**< bit:  0..31  Input Change Interrupt Enable            */
  } vec;                                /**< Structure used for vec  access  */
  uint32_t reg;                         /**< Type used for register access */
} PIO_IER_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define PIO_IER_OFFSET                      (0x40)                                        /**<  (PIO_IER) Interrupt Enable Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define PIO_IER_P0_Pos                      0                                              /**< (PIO_IER) Input Change Interrupt Enable Position */
#define PIO_IER_P0_Msk                      (0x1U << PIO_IER_P0_Pos)                       /**< (PIO_IER) Input Change Interrupt Enable Mask */
#define PIO_IER_P0                          PIO_IER_P0_Msk                                 /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_IER_P0_Msk instead */
#define PIO_IER_P1_Pos                      1                                              /**< (PIO_IER) Input Change Interrupt Enable Position */
#define PIO_IER_P1_Msk                      (0x1U << PIO_IER_P1_Pos)                       /**< (PIO_IER) Input Change Interrupt Enable Mask */
#define PIO_IER_P1                          PIO_IER_P1_Msk                                 /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_IER_P1_Msk instead */
#define PIO_IER_P2_Pos                      2                                              /**< (PIO_IER) Input Change Interrupt Enable Position */
#define PIO_IER_P2_Msk                      (0x1U << PIO_IER_P2_Pos)                       /**< (PIO_IER) Input Change Interrupt Enable Mask */
#define PIO_IER_P2                          PIO_IER_P2_Msk                                 /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_IER_P2_Msk instead */
#define PIO_IER_P3_Pos                      3                                              /**< (PIO_IER) Input Change Interrupt Enable Position */
#define PIO_IER_P3_Msk                      (0x1U << PIO_IER_P3_Pos)                       /**< (PIO_IER) Input Change Interrupt Enable Mask */
#define PIO_IER_P3                          PIO_IER_P3_Msk                                 /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_IER_P3_Msk instead */
#define PIO_IER_P4_Pos                      4                                              /**< (PIO_IER) Input Change Interrupt Enable Position */
#define PIO_IER_P4_Msk                      (0x1U << PIO_IER_P4_Pos)                       /**< (PIO_IER) Input Change Interrupt Enable Mask */
#define PIO_IER_P4                          PIO_IER_P4_Msk                                 /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_IER_P4_Msk instead */
#define PIO_IER_P5_Pos                      5                                              /**< (PIO_IER) Input Change Interrupt Enable Position */
#define PIO_IER_P5_Msk                      (0x1U << PIO_IER_P5_Pos)                       /**< (PIO_IER) Input Change Interrupt Enable Mask */
#define PIO_IER_P5                          PIO_IER_P5_Msk                                 /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_IER_P5_Msk instead */
#define PIO_IER_P6_Pos                      6                                              /**< (PIO_IER) Input Change Interrupt Enable Position */
#define PIO_IER_P6_Msk                      (0x1U << PIO_IER_P6_Pos)                       /**< (PIO_IER) Input Change Interrupt Enable Mask */
#define PIO_IER_P6                          PIO_IER_P6_Msk                                 /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_IER_P6_Msk instead */
#define PIO_IER_P7_Pos                      7                                              /**< (PIO_IER) Input Change Interrupt Enable Position */
#define PIO_IER_P7_Msk                      (0x1U << PIO_IER_P7_Pos)                       /**< (PIO_IER) Input Change Interrupt Enable Mask */
#define PIO_IER_P7                          PIO_IER_P7_Msk                                 /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_IER_P7_Msk instead */
#define PIO_IER_P8_Pos                      8                                              /**< (PIO_IER) Input Change Interrupt Enable Position */
#define PIO_IER_P8_Msk                      (0x1U << PIO_IER_P8_Pos)                       /**< (PIO_IER) Input Change Interrupt Enable Mask */
#define PIO_IER_P8                          PIO_IER_P8_Msk                                 /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_IER_P8_Msk instead */
#define PIO_IER_P9_Pos                      9                                              /**< (PIO_IER) Input Change Interrupt Enable Position */
#define PIO_IER_P9_Msk                      (0x1U << PIO_IER_P9_Pos)                       /**< (PIO_IER) Input Change Interrupt Enable Mask */
#define PIO_IER_P9                          PIO_IER_P9_Msk                                 /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_IER_P9_Msk instead */
#define PIO_IER_P10_Pos                     10                                             /**< (PIO_IER) Input Change Interrupt Enable Position */
#define PIO_IER_P10_Msk                     (0x1U << PIO_IER_P10_Pos)                      /**< (PIO_IER) Input Change Interrupt Enable Mask */
#define PIO_IER_P10                         PIO_IER_P10_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_IER_P10_Msk instead */
#define PIO_IER_P11_Pos                     11                                             /**< (PIO_IER) Input Change Interrupt Enable Position */
#define PIO_IER_P11_Msk                     (0x1U << PIO_IER_P11_Pos)                      /**< (PIO_IER) Input Change Interrupt Enable Mask */
#define PIO_IER_P11                         PIO_IER_P11_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_IER_P11_Msk instead */
#define PIO_IER_P12_Pos                     12                                             /**< (PIO_IER) Input Change Interrupt Enable Position */
#define PIO_IER_P12_Msk                     (0x1U << PIO_IER_P12_Pos)                      /**< (PIO_IER) Input Change Interrupt Enable Mask */
#define PIO_IER_P12                         PIO_IER_P12_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_IER_P12_Msk instead */
#define PIO_IER_P13_Pos                     13                                             /**< (PIO_IER) Input Change Interrupt Enable Position */
#define PIO_IER_P13_Msk                     (0x1U << PIO_IER_P13_Pos)                      /**< (PIO_IER) Input Change Interrupt Enable Mask */
#define PIO_IER_P13                         PIO_IER_P13_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_IER_P13_Msk instead */
#define PIO_IER_P14_Pos                     14                                             /**< (PIO_IER) Input Change Interrupt Enable Position */
#define PIO_IER_P14_Msk                     (0x1U << PIO_IER_P14_Pos)                      /**< (PIO_IER) Input Change Interrupt Enable Mask */
#define PIO_IER_P14                         PIO_IER_P14_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_IER_P14_Msk instead */
#define PIO_IER_P15_Pos                     15                                             /**< (PIO_IER) Input Change Interrupt Enable Position */
#define PIO_IER_P15_Msk                     (0x1U << PIO_IER_P15_Pos)                      /**< (PIO_IER) Input Change Interrupt Enable Mask */
#define PIO_IER_P15                         PIO_IER_P15_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_IER_P15_Msk instead */
#define PIO_IER_P16_Pos                     16                                             /**< (PIO_IER) Input Change Interrupt Enable Position */
#define PIO_IER_P16_Msk                     (0x1U << PIO_IER_P16_Pos)                      /**< (PIO_IER) Input Change Interrupt Enable Mask */
#define PIO_IER_P16                         PIO_IER_P16_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_IER_P16_Msk instead */
#define PIO_IER_P17_Pos                     17                                             /**< (PIO_IER) Input Change Interrupt Enable Position */
#define PIO_IER_P17_Msk                     (0x1U << PIO_IER_P17_Pos)                      /**< (PIO_IER) Input Change Interrupt Enable Mask */
#define PIO_IER_P17                         PIO_IER_P17_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_IER_P17_Msk instead */
#define PIO_IER_P18_Pos                     18                                             /**< (PIO_IER) Input Change Interrupt Enable Position */
#define PIO_IER_P18_Msk                     (0x1U << PIO_IER_P18_Pos)                      /**< (PIO_IER) Input Change Interrupt Enable Mask */
#define PIO_IER_P18                         PIO_IER_P18_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_IER_P18_Msk instead */
#define PIO_IER_P19_Pos                     19                                             /**< (PIO_IER) Input Change Interrupt Enable Position */
#define PIO_IER_P19_Msk                     (0x1U << PIO_IER_P19_Pos)                      /**< (PIO_IER) Input Change Interrupt Enable Mask */
#define PIO_IER_P19                         PIO_IER_P19_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_IER_P19_Msk instead */
#define PIO_IER_P20_Pos                     20                                             /**< (PIO_IER) Input Change Interrupt Enable Position */
#define PIO_IER_P20_Msk                     (0x1U << PIO_IER_P20_Pos)                      /**< (PIO_IER) Input Change Interrupt Enable Mask */
#define PIO_IER_P20                         PIO_IER_P20_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_IER_P20_Msk instead */
#define PIO_IER_P21_Pos                     21                                             /**< (PIO_IER) Input Change Interrupt Enable Position */
#define PIO_IER_P21_Msk                     (0x1U << PIO_IER_P21_Pos)                      /**< (PIO_IER) Input Change Interrupt Enable Mask */
#define PIO_IER_P21                         PIO_IER_P21_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_IER_P21_Msk instead */
#define PIO_IER_P22_Pos                     22                                             /**< (PIO_IER) Input Change Interrupt Enable Position */
#define PIO_IER_P22_Msk                     (0x1U << PIO_IER_P22_Pos)                      /**< (PIO_IER) Input Change Interrupt Enable Mask */
#define PIO_IER_P22                         PIO_IER_P22_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_IER_P22_Msk instead */
#define PIO_IER_P23_Pos                     23                                             /**< (PIO_IER) Input Change Interrupt Enable Position */
#define PIO_IER_P23_Msk                     (0x1U << PIO_IER_P23_Pos)                      /**< (PIO_IER) Input Change Interrupt Enable Mask */
#define PIO_IER_P23                         PIO_IER_P23_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_IER_P23_Msk instead */
#define PIO_IER_P24_Pos                     24                                             /**< (PIO_IER) Input Change Interrupt Enable Position */
#define PIO_IER_P24_Msk                     (0x1U << PIO_IER_P24_Pos)                      /**< (PIO_IER) Input Change Interrupt Enable Mask */
#define PIO_IER_P24                         PIO_IER_P24_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_IER_P24_Msk instead */
#define PIO_IER_P25_Pos                     25                                             /**< (PIO_IER) Input Change Interrupt Enable Position */
#define PIO_IER_P25_Msk                     (0x1U << PIO_IER_P25_Pos)                      /**< (PIO_IER) Input Change Interrupt Enable Mask */
#define PIO_IER_P25                         PIO_IER_P25_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_IER_P25_Msk instead */
#define PIO_IER_P26_Pos                     26                                             /**< (PIO_IER) Input Change Interrupt Enable Position */
#define PIO_IER_P26_Msk                     (0x1U << PIO_IER_P26_Pos)                      /**< (PIO_IER) Input Change Interrupt Enable Mask */
#define PIO_IER_P26                         PIO_IER_P26_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_IER_P26_Msk instead */
#define PIO_IER_P27_Pos                     27                                             /**< (PIO_IER) Input Change Interrupt Enable Position */
#define PIO_IER_P27_Msk                     (0x1U << PIO_IER_P27_Pos)                      /**< (PIO_IER) Input Change Interrupt Enable Mask */
#define PIO_IER_P27                         PIO_IER_P27_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_IER_P27_Msk instead */
#define PIO_IER_P28_Pos                     28                                             /**< (PIO_IER) Input Change Interrupt Enable Position */
#define PIO_IER_P28_Msk                     (0x1U << PIO_IER_P28_Pos)                      /**< (PIO_IER) Input Change Interrupt Enable Mask */
#define PIO_IER_P28                         PIO_IER_P28_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_IER_P28_Msk instead */
#define PIO_IER_P29_Pos                     29                                             /**< (PIO_IER) Input Change Interrupt Enable Position */
#define PIO_IER_P29_Msk                     (0x1U << PIO_IER_P29_Pos)                      /**< (PIO_IER) Input Change Interrupt Enable Mask */
#define PIO_IER_P29                         PIO_IER_P29_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_IER_P29_Msk instead */
#define PIO_IER_P30_Pos                     30                                             /**< (PIO_IER) Input Change Interrupt Enable Position */
#define PIO_IER_P30_Msk                     (0x1U << PIO_IER_P30_Pos)                      /**< (PIO_IER) Input Change Interrupt Enable Mask */
#define PIO_IER_P30                         PIO_IER_P30_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_IER_P30_Msk instead */
#define PIO_IER_P31_Pos                     31                                             /**< (PIO_IER) Input Change Interrupt Enable Position */
#define PIO_IER_P31_Msk                     (0x1U << PIO_IER_P31_Pos)                      /**< (PIO_IER) Input Change Interrupt Enable Mask */
#define PIO_IER_P31                         PIO_IER_P31_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_IER_P31_Msk instead */
#define PIO_IER_P_Pos                       0                                              /**< (PIO_IER Position) Input Change Interrupt Enable */
#define PIO_IER_P_Msk                       (0xFFFFFFFFU << PIO_IER_P_Pos)                 /**< (PIO_IER Mask) P */
#define PIO_IER_P(value)                    (PIO_IER_P_Msk & ((value) << PIO_IER_P_Pos))   
#define PIO_IER_MASK                        (0xFFFFFFFFU)                                  /**< \deprecated (PIO_IER) Register MASK  (Use PIO_IER_Msk instead)  */
#define PIO_IER_Msk                         (0xFFFFFFFFU)                                  /**< (PIO_IER) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- PIO_IDR : (PIO Offset: 0x44) (/W 32) Interrupt Disable Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t P0:1;                      /**< bit:      0  Input Change Interrupt Disable           */
    uint32_t P1:1;                      /**< bit:      1  Input Change Interrupt Disable           */
    uint32_t P2:1;                      /**< bit:      2  Input Change Interrupt Disable           */
    uint32_t P3:1;                      /**< bit:      3  Input Change Interrupt Disable           */
    uint32_t P4:1;                      /**< bit:      4  Input Change Interrupt Disable           */
    uint32_t P5:1;                      /**< bit:      5  Input Change Interrupt Disable           */
    uint32_t P6:1;                      /**< bit:      6  Input Change Interrupt Disable           */
    uint32_t P7:1;                      /**< bit:      7  Input Change Interrupt Disable           */
    uint32_t P8:1;                      /**< bit:      8  Input Change Interrupt Disable           */
    uint32_t P9:1;                      /**< bit:      9  Input Change Interrupt Disable           */
    uint32_t P10:1;                     /**< bit:     10  Input Change Interrupt Disable           */
    uint32_t P11:1;                     /**< bit:     11  Input Change Interrupt Disable           */
    uint32_t P12:1;                     /**< bit:     12  Input Change Interrupt Disable           */
    uint32_t P13:1;                     /**< bit:     13  Input Change Interrupt Disable           */
    uint32_t P14:1;                     /**< bit:     14  Input Change Interrupt Disable           */
    uint32_t P15:1;                     /**< bit:     15  Input Change Interrupt Disable           */
    uint32_t P16:1;                     /**< bit:     16  Input Change Interrupt Disable           */
    uint32_t P17:1;                     /**< bit:     17  Input Change Interrupt Disable           */
    uint32_t P18:1;                     /**< bit:     18  Input Change Interrupt Disable           */
    uint32_t P19:1;                     /**< bit:     19  Input Change Interrupt Disable           */
    uint32_t P20:1;                     /**< bit:     20  Input Change Interrupt Disable           */
    uint32_t P21:1;                     /**< bit:     21  Input Change Interrupt Disable           */
    uint32_t P22:1;                     /**< bit:     22  Input Change Interrupt Disable           */
    uint32_t P23:1;                     /**< bit:     23  Input Change Interrupt Disable           */
    uint32_t P24:1;                     /**< bit:     24  Input Change Interrupt Disable           */
    uint32_t P25:1;                     /**< bit:     25  Input Change Interrupt Disable           */
    uint32_t P26:1;                     /**< bit:     26  Input Change Interrupt Disable           */
    uint32_t P27:1;                     /**< bit:     27  Input Change Interrupt Disable           */
    uint32_t P28:1;                     /**< bit:     28  Input Change Interrupt Disable           */
    uint32_t P29:1;                     /**< bit:     29  Input Change Interrupt Disable           */
    uint32_t P30:1;                     /**< bit:     30  Input Change Interrupt Disable           */
    uint32_t P31:1;                     /**< bit:     31  Input Change Interrupt Disable           */
  } bit;                                /**< Structure used for bit  access */
  struct {
    uint32_t P:32;                      /**< bit:  0..31  Input Change Interrupt Disable           */
  } vec;                                /**< Structure used for vec  access  */
  uint32_t reg;                         /**< Type used for register access */
} PIO_IDR_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define PIO_IDR_OFFSET                      (0x44)                                        /**<  (PIO_IDR) Interrupt Disable Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define PIO_IDR_P0_Pos                      0                                              /**< (PIO_IDR) Input Change Interrupt Disable Position */
#define PIO_IDR_P0_Msk                      (0x1U << PIO_IDR_P0_Pos)                       /**< (PIO_IDR) Input Change Interrupt Disable Mask */
#define PIO_IDR_P0                          PIO_IDR_P0_Msk                                 /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_IDR_P0_Msk instead */
#define PIO_IDR_P1_Pos                      1                                              /**< (PIO_IDR) Input Change Interrupt Disable Position */
#define PIO_IDR_P1_Msk                      (0x1U << PIO_IDR_P1_Pos)                       /**< (PIO_IDR) Input Change Interrupt Disable Mask */
#define PIO_IDR_P1                          PIO_IDR_P1_Msk                                 /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_IDR_P1_Msk instead */
#define PIO_IDR_P2_Pos                      2                                              /**< (PIO_IDR) Input Change Interrupt Disable Position */
#define PIO_IDR_P2_Msk                      (0x1U << PIO_IDR_P2_Pos)                       /**< (PIO_IDR) Input Change Interrupt Disable Mask */
#define PIO_IDR_P2                          PIO_IDR_P2_Msk                                 /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_IDR_P2_Msk instead */
#define PIO_IDR_P3_Pos                      3                                              /**< (PIO_IDR) Input Change Interrupt Disable Position */
#define PIO_IDR_P3_Msk                      (0x1U << PIO_IDR_P3_Pos)                       /**< (PIO_IDR) Input Change Interrupt Disable Mask */
#define PIO_IDR_P3                          PIO_IDR_P3_Msk                                 /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_IDR_P3_Msk instead */
#define PIO_IDR_P4_Pos                      4                                              /**< (PIO_IDR) Input Change Interrupt Disable Position */
#define PIO_IDR_P4_Msk                      (0x1U << PIO_IDR_P4_Pos)                       /**< (PIO_IDR) Input Change Interrupt Disable Mask */
#define PIO_IDR_P4                          PIO_IDR_P4_Msk                                 /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_IDR_P4_Msk instead */
#define PIO_IDR_P5_Pos                      5                                              /**< (PIO_IDR) Input Change Interrupt Disable Position */
#define PIO_IDR_P5_Msk                      (0x1U << PIO_IDR_P5_Pos)                       /**< (PIO_IDR) Input Change Interrupt Disable Mask */
#define PIO_IDR_P5                          PIO_IDR_P5_Msk                                 /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_IDR_P5_Msk instead */
#define PIO_IDR_P6_Pos                      6                                              /**< (PIO_IDR) Input Change Interrupt Disable Position */
#define PIO_IDR_P6_Msk                      (0x1U << PIO_IDR_P6_Pos)                       /**< (PIO_IDR) Input Change Interrupt Disable Mask */
#define PIO_IDR_P6                          PIO_IDR_P6_Msk                                 /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_IDR_P6_Msk instead */
#define PIO_IDR_P7_Pos                      7                                              /**< (PIO_IDR) Input Change Interrupt Disable Position */
#define PIO_IDR_P7_Msk                      (0x1U << PIO_IDR_P7_Pos)                       /**< (PIO_IDR) Input Change Interrupt Disable Mask */
#define PIO_IDR_P7                          PIO_IDR_P7_Msk                                 /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_IDR_P7_Msk instead */
#define PIO_IDR_P8_Pos                      8                                              /**< (PIO_IDR) Input Change Interrupt Disable Position */
#define PIO_IDR_P8_Msk                      (0x1U << PIO_IDR_P8_Pos)                       /**< (PIO_IDR) Input Change Interrupt Disable Mask */
#define PIO_IDR_P8                          PIO_IDR_P8_Msk                                 /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_IDR_P8_Msk instead */
#define PIO_IDR_P9_Pos                      9                                              /**< (PIO_IDR) Input Change Interrupt Disable Position */
#define PIO_IDR_P9_Msk                      (0x1U << PIO_IDR_P9_Pos)                       /**< (PIO_IDR) Input Change Interrupt Disable Mask */
#define PIO_IDR_P9                          PIO_IDR_P9_Msk                                 /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_IDR_P9_Msk instead */
#define PIO_IDR_P10_Pos                     10                                             /**< (PIO_IDR) Input Change Interrupt Disable Position */
#define PIO_IDR_P10_Msk                     (0x1U << PIO_IDR_P10_Pos)                      /**< (PIO_IDR) Input Change Interrupt Disable Mask */
#define PIO_IDR_P10                         PIO_IDR_P10_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_IDR_P10_Msk instead */
#define PIO_IDR_P11_Pos                     11                                             /**< (PIO_IDR) Input Change Interrupt Disable Position */
#define PIO_IDR_P11_Msk                     (0x1U << PIO_IDR_P11_Pos)                      /**< (PIO_IDR) Input Change Interrupt Disable Mask */
#define PIO_IDR_P11                         PIO_IDR_P11_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_IDR_P11_Msk instead */
#define PIO_IDR_P12_Pos                     12                                             /**< (PIO_IDR) Input Change Interrupt Disable Position */
#define PIO_IDR_P12_Msk                     (0x1U << PIO_IDR_P12_Pos)                      /**< (PIO_IDR) Input Change Interrupt Disable Mask */
#define PIO_IDR_P12                         PIO_IDR_P12_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_IDR_P12_Msk instead */
#define PIO_IDR_P13_Pos                     13                                             /**< (PIO_IDR) Input Change Interrupt Disable Position */
#define PIO_IDR_P13_Msk                     (0x1U << PIO_IDR_P13_Pos)                      /**< (PIO_IDR) Input Change Interrupt Disable Mask */
#define PIO_IDR_P13                         PIO_IDR_P13_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_IDR_P13_Msk instead */
#define PIO_IDR_P14_Pos                     14                                             /**< (PIO_IDR) Input Change Interrupt Disable Position */
#define PIO_IDR_P14_Msk                     (0x1U << PIO_IDR_P14_Pos)                      /**< (PIO_IDR) Input Change Interrupt Disable Mask */
#define PIO_IDR_P14                         PIO_IDR_P14_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_IDR_P14_Msk instead */
#define PIO_IDR_P15_Pos                     15                                             /**< (PIO_IDR) Input Change Interrupt Disable Position */
#define PIO_IDR_P15_Msk                     (0x1U << PIO_IDR_P15_Pos)                      /**< (PIO_IDR) Input Change Interrupt Disable Mask */
#define PIO_IDR_P15                         PIO_IDR_P15_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_IDR_P15_Msk instead */
#define PIO_IDR_P16_Pos                     16                                             /**< (PIO_IDR) Input Change Interrupt Disable Position */
#define PIO_IDR_P16_Msk                     (0x1U << PIO_IDR_P16_Pos)                      /**< (PIO_IDR) Input Change Interrupt Disable Mask */
#define PIO_IDR_P16                         PIO_IDR_P16_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_IDR_P16_Msk instead */
#define PIO_IDR_P17_Pos                     17                                             /**< (PIO_IDR) Input Change Interrupt Disable Position */
#define PIO_IDR_P17_Msk                     (0x1U << PIO_IDR_P17_Pos)                      /**< (PIO_IDR) Input Change Interrupt Disable Mask */
#define PIO_IDR_P17                         PIO_IDR_P17_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_IDR_P17_Msk instead */
#define PIO_IDR_P18_Pos                     18                                             /**< (PIO_IDR) Input Change Interrupt Disable Position */
#define PIO_IDR_P18_Msk                     (0x1U << PIO_IDR_P18_Pos)                      /**< (PIO_IDR) Input Change Interrupt Disable Mask */
#define PIO_IDR_P18                         PIO_IDR_P18_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_IDR_P18_Msk instead */
#define PIO_IDR_P19_Pos                     19                                             /**< (PIO_IDR) Input Change Interrupt Disable Position */
#define PIO_IDR_P19_Msk                     (0x1U << PIO_IDR_P19_Pos)                      /**< (PIO_IDR) Input Change Interrupt Disable Mask */
#define PIO_IDR_P19                         PIO_IDR_P19_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_IDR_P19_Msk instead */
#define PIO_IDR_P20_Pos                     20                                             /**< (PIO_IDR) Input Change Interrupt Disable Position */
#define PIO_IDR_P20_Msk                     (0x1U << PIO_IDR_P20_Pos)                      /**< (PIO_IDR) Input Change Interrupt Disable Mask */
#define PIO_IDR_P20                         PIO_IDR_P20_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_IDR_P20_Msk instead */
#define PIO_IDR_P21_Pos                     21                                             /**< (PIO_IDR) Input Change Interrupt Disable Position */
#define PIO_IDR_P21_Msk                     (0x1U << PIO_IDR_P21_Pos)                      /**< (PIO_IDR) Input Change Interrupt Disable Mask */
#define PIO_IDR_P21                         PIO_IDR_P21_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_IDR_P21_Msk instead */
#define PIO_IDR_P22_Pos                     22                                             /**< (PIO_IDR) Input Change Interrupt Disable Position */
#define PIO_IDR_P22_Msk                     (0x1U << PIO_IDR_P22_Pos)                      /**< (PIO_IDR) Input Change Interrupt Disable Mask */
#define PIO_IDR_P22                         PIO_IDR_P22_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_IDR_P22_Msk instead */
#define PIO_IDR_P23_Pos                     23                                             /**< (PIO_IDR) Input Change Interrupt Disable Position */
#define PIO_IDR_P23_Msk                     (0x1U << PIO_IDR_P23_Pos)                      /**< (PIO_IDR) Input Change Interrupt Disable Mask */
#define PIO_IDR_P23                         PIO_IDR_P23_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_IDR_P23_Msk instead */
#define PIO_IDR_P24_Pos                     24                                             /**< (PIO_IDR) Input Change Interrupt Disable Position */
#define PIO_IDR_P24_Msk                     (0x1U << PIO_IDR_P24_Pos)                      /**< (PIO_IDR) Input Change Interrupt Disable Mask */
#define PIO_IDR_P24                         PIO_IDR_P24_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_IDR_P24_Msk instead */
#define PIO_IDR_P25_Pos                     25                                             /**< (PIO_IDR) Input Change Interrupt Disable Position */
#define PIO_IDR_P25_Msk                     (0x1U << PIO_IDR_P25_Pos)                      /**< (PIO_IDR) Input Change Interrupt Disable Mask */
#define PIO_IDR_P25                         PIO_IDR_P25_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_IDR_P25_Msk instead */
#define PIO_IDR_P26_Pos                     26                                             /**< (PIO_IDR) Input Change Interrupt Disable Position */
#define PIO_IDR_P26_Msk                     (0x1U << PIO_IDR_P26_Pos)                      /**< (PIO_IDR) Input Change Interrupt Disable Mask */
#define PIO_IDR_P26                         PIO_IDR_P26_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_IDR_P26_Msk instead */
#define PIO_IDR_P27_Pos                     27                                             /**< (PIO_IDR) Input Change Interrupt Disable Position */
#define PIO_IDR_P27_Msk                     (0x1U << PIO_IDR_P27_Pos)                      /**< (PIO_IDR) Input Change Interrupt Disable Mask */
#define PIO_IDR_P27                         PIO_IDR_P27_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_IDR_P27_Msk instead */
#define PIO_IDR_P28_Pos                     28                                             /**< (PIO_IDR) Input Change Interrupt Disable Position */
#define PIO_IDR_P28_Msk                     (0x1U << PIO_IDR_P28_Pos)                      /**< (PIO_IDR) Input Change Interrupt Disable Mask */
#define PIO_IDR_P28                         PIO_IDR_P28_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_IDR_P28_Msk instead */
#define PIO_IDR_P29_Pos                     29                                             /**< (PIO_IDR) Input Change Interrupt Disable Position */
#define PIO_IDR_P29_Msk                     (0x1U << PIO_IDR_P29_Pos)                      /**< (PIO_IDR) Input Change Interrupt Disable Mask */
#define PIO_IDR_P29                         PIO_IDR_P29_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_IDR_P29_Msk instead */
#define PIO_IDR_P30_Pos                     30                                             /**< (PIO_IDR) Input Change Interrupt Disable Position */
#define PIO_IDR_P30_Msk                     (0x1U << PIO_IDR_P30_Pos)                      /**< (PIO_IDR) Input Change Interrupt Disable Mask */
#define PIO_IDR_P30                         PIO_IDR_P30_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_IDR_P30_Msk instead */
#define PIO_IDR_P31_Pos                     31                                             /**< (PIO_IDR) Input Change Interrupt Disable Position */
#define PIO_IDR_P31_Msk                     (0x1U << PIO_IDR_P31_Pos)                      /**< (PIO_IDR) Input Change Interrupt Disable Mask */
#define PIO_IDR_P31                         PIO_IDR_P31_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_IDR_P31_Msk instead */
#define PIO_IDR_P_Pos                       0                                              /**< (PIO_IDR Position) Input Change Interrupt Disable */
#define PIO_IDR_P_Msk                       (0xFFFFFFFFU << PIO_IDR_P_Pos)                 /**< (PIO_IDR Mask) P */
#define PIO_IDR_P(value)                    (PIO_IDR_P_Msk & ((value) << PIO_IDR_P_Pos))   
#define PIO_IDR_MASK                        (0xFFFFFFFFU)                                  /**< \deprecated (PIO_IDR) Register MASK  (Use PIO_IDR_Msk instead)  */
#define PIO_IDR_Msk                         (0xFFFFFFFFU)                                  /**< (PIO_IDR) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- PIO_IMR : (PIO Offset: 0x48) (R/ 32) Interrupt Mask Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t P0:1;                      /**< bit:      0  Input Change Interrupt Mask              */
    uint32_t P1:1;                      /**< bit:      1  Input Change Interrupt Mask              */
    uint32_t P2:1;                      /**< bit:      2  Input Change Interrupt Mask              */
    uint32_t P3:1;                      /**< bit:      3  Input Change Interrupt Mask              */
    uint32_t P4:1;                      /**< bit:      4  Input Change Interrupt Mask              */
    uint32_t P5:1;                      /**< bit:      5  Input Change Interrupt Mask              */
    uint32_t P6:1;                      /**< bit:      6  Input Change Interrupt Mask              */
    uint32_t P7:1;                      /**< bit:      7  Input Change Interrupt Mask              */
    uint32_t P8:1;                      /**< bit:      8  Input Change Interrupt Mask              */
    uint32_t P9:1;                      /**< bit:      9  Input Change Interrupt Mask              */
    uint32_t P10:1;                     /**< bit:     10  Input Change Interrupt Mask              */
    uint32_t P11:1;                     /**< bit:     11  Input Change Interrupt Mask              */
    uint32_t P12:1;                     /**< bit:     12  Input Change Interrupt Mask              */
    uint32_t P13:1;                     /**< bit:     13  Input Change Interrupt Mask              */
    uint32_t P14:1;                     /**< bit:     14  Input Change Interrupt Mask              */
    uint32_t P15:1;                     /**< bit:     15  Input Change Interrupt Mask              */
    uint32_t P16:1;                     /**< bit:     16  Input Change Interrupt Mask              */
    uint32_t P17:1;                     /**< bit:     17  Input Change Interrupt Mask              */
    uint32_t P18:1;                     /**< bit:     18  Input Change Interrupt Mask              */
    uint32_t P19:1;                     /**< bit:     19  Input Change Interrupt Mask              */
    uint32_t P20:1;                     /**< bit:     20  Input Change Interrupt Mask              */
    uint32_t P21:1;                     /**< bit:     21  Input Change Interrupt Mask              */
    uint32_t P22:1;                     /**< bit:     22  Input Change Interrupt Mask              */
    uint32_t P23:1;                     /**< bit:     23  Input Change Interrupt Mask              */
    uint32_t P24:1;                     /**< bit:     24  Input Change Interrupt Mask              */
    uint32_t P25:1;                     /**< bit:     25  Input Change Interrupt Mask              */
    uint32_t P26:1;                     /**< bit:     26  Input Change Interrupt Mask              */
    uint32_t P27:1;                     /**< bit:     27  Input Change Interrupt Mask              */
    uint32_t P28:1;                     /**< bit:     28  Input Change Interrupt Mask              */
    uint32_t P29:1;                     /**< bit:     29  Input Change Interrupt Mask              */
    uint32_t P30:1;                     /**< bit:     30  Input Change Interrupt Mask              */
    uint32_t P31:1;                     /**< bit:     31  Input Change Interrupt Mask              */
  } bit;                                /**< Structure used for bit  access */
  struct {
    uint32_t P:32;                      /**< bit:  0..31  Input Change Interrupt Mask              */
  } vec;                                /**< Structure used for vec  access  */
  uint32_t reg;                         /**< Type used for register access */
} PIO_IMR_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define PIO_IMR_OFFSET                      (0x48)                                        /**<  (PIO_IMR) Interrupt Mask Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define PIO_IMR_P0_Pos                      0                                              /**< (PIO_IMR) Input Change Interrupt Mask Position */
#define PIO_IMR_P0_Msk                      (0x1U << PIO_IMR_P0_Pos)                       /**< (PIO_IMR) Input Change Interrupt Mask Mask */
#define PIO_IMR_P0                          PIO_IMR_P0_Msk                                 /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_IMR_P0_Msk instead */
#define PIO_IMR_P1_Pos                      1                                              /**< (PIO_IMR) Input Change Interrupt Mask Position */
#define PIO_IMR_P1_Msk                      (0x1U << PIO_IMR_P1_Pos)                       /**< (PIO_IMR) Input Change Interrupt Mask Mask */
#define PIO_IMR_P1                          PIO_IMR_P1_Msk                                 /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_IMR_P1_Msk instead */
#define PIO_IMR_P2_Pos                      2                                              /**< (PIO_IMR) Input Change Interrupt Mask Position */
#define PIO_IMR_P2_Msk                      (0x1U << PIO_IMR_P2_Pos)                       /**< (PIO_IMR) Input Change Interrupt Mask Mask */
#define PIO_IMR_P2                          PIO_IMR_P2_Msk                                 /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_IMR_P2_Msk instead */
#define PIO_IMR_P3_Pos                      3                                              /**< (PIO_IMR) Input Change Interrupt Mask Position */
#define PIO_IMR_P3_Msk                      (0x1U << PIO_IMR_P3_Pos)                       /**< (PIO_IMR) Input Change Interrupt Mask Mask */
#define PIO_IMR_P3                          PIO_IMR_P3_Msk                                 /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_IMR_P3_Msk instead */
#define PIO_IMR_P4_Pos                      4                                              /**< (PIO_IMR) Input Change Interrupt Mask Position */
#define PIO_IMR_P4_Msk                      (0x1U << PIO_IMR_P4_Pos)                       /**< (PIO_IMR) Input Change Interrupt Mask Mask */
#define PIO_IMR_P4                          PIO_IMR_P4_Msk                                 /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_IMR_P4_Msk instead */
#define PIO_IMR_P5_Pos                      5                                              /**< (PIO_IMR) Input Change Interrupt Mask Position */
#define PIO_IMR_P5_Msk                      (0x1U << PIO_IMR_P5_Pos)                       /**< (PIO_IMR) Input Change Interrupt Mask Mask */
#define PIO_IMR_P5                          PIO_IMR_P5_Msk                                 /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_IMR_P5_Msk instead */
#define PIO_IMR_P6_Pos                      6                                              /**< (PIO_IMR) Input Change Interrupt Mask Position */
#define PIO_IMR_P6_Msk                      (0x1U << PIO_IMR_P6_Pos)                       /**< (PIO_IMR) Input Change Interrupt Mask Mask */
#define PIO_IMR_P6                          PIO_IMR_P6_Msk                                 /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_IMR_P6_Msk instead */
#define PIO_IMR_P7_Pos                      7                                              /**< (PIO_IMR) Input Change Interrupt Mask Position */
#define PIO_IMR_P7_Msk                      (0x1U << PIO_IMR_P7_Pos)                       /**< (PIO_IMR) Input Change Interrupt Mask Mask */
#define PIO_IMR_P7                          PIO_IMR_P7_Msk                                 /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_IMR_P7_Msk instead */
#define PIO_IMR_P8_Pos                      8                                              /**< (PIO_IMR) Input Change Interrupt Mask Position */
#define PIO_IMR_P8_Msk                      (0x1U << PIO_IMR_P8_Pos)                       /**< (PIO_IMR) Input Change Interrupt Mask Mask */
#define PIO_IMR_P8                          PIO_IMR_P8_Msk                                 /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_IMR_P8_Msk instead */
#define PIO_IMR_P9_Pos                      9                                              /**< (PIO_IMR) Input Change Interrupt Mask Position */
#define PIO_IMR_P9_Msk                      (0x1U << PIO_IMR_P9_Pos)                       /**< (PIO_IMR) Input Change Interrupt Mask Mask */
#define PIO_IMR_P9                          PIO_IMR_P9_Msk                                 /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_IMR_P9_Msk instead */
#define PIO_IMR_P10_Pos                     10                                             /**< (PIO_IMR) Input Change Interrupt Mask Position */
#define PIO_IMR_P10_Msk                     (0x1U << PIO_IMR_P10_Pos)                      /**< (PIO_IMR) Input Change Interrupt Mask Mask */
#define PIO_IMR_P10                         PIO_IMR_P10_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_IMR_P10_Msk instead */
#define PIO_IMR_P11_Pos                     11                                             /**< (PIO_IMR) Input Change Interrupt Mask Position */
#define PIO_IMR_P11_Msk                     (0x1U << PIO_IMR_P11_Pos)                      /**< (PIO_IMR) Input Change Interrupt Mask Mask */
#define PIO_IMR_P11                         PIO_IMR_P11_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_IMR_P11_Msk instead */
#define PIO_IMR_P12_Pos                     12                                             /**< (PIO_IMR) Input Change Interrupt Mask Position */
#define PIO_IMR_P12_Msk                     (0x1U << PIO_IMR_P12_Pos)                      /**< (PIO_IMR) Input Change Interrupt Mask Mask */
#define PIO_IMR_P12                         PIO_IMR_P12_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_IMR_P12_Msk instead */
#define PIO_IMR_P13_Pos                     13                                             /**< (PIO_IMR) Input Change Interrupt Mask Position */
#define PIO_IMR_P13_Msk                     (0x1U << PIO_IMR_P13_Pos)                      /**< (PIO_IMR) Input Change Interrupt Mask Mask */
#define PIO_IMR_P13                         PIO_IMR_P13_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_IMR_P13_Msk instead */
#define PIO_IMR_P14_Pos                     14                                             /**< (PIO_IMR) Input Change Interrupt Mask Position */
#define PIO_IMR_P14_Msk                     (0x1U << PIO_IMR_P14_Pos)                      /**< (PIO_IMR) Input Change Interrupt Mask Mask */
#define PIO_IMR_P14                         PIO_IMR_P14_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_IMR_P14_Msk instead */
#define PIO_IMR_P15_Pos                     15                                             /**< (PIO_IMR) Input Change Interrupt Mask Position */
#define PIO_IMR_P15_Msk                     (0x1U << PIO_IMR_P15_Pos)                      /**< (PIO_IMR) Input Change Interrupt Mask Mask */
#define PIO_IMR_P15                         PIO_IMR_P15_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_IMR_P15_Msk instead */
#define PIO_IMR_P16_Pos                     16                                             /**< (PIO_IMR) Input Change Interrupt Mask Position */
#define PIO_IMR_P16_Msk                     (0x1U << PIO_IMR_P16_Pos)                      /**< (PIO_IMR) Input Change Interrupt Mask Mask */
#define PIO_IMR_P16                         PIO_IMR_P16_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_IMR_P16_Msk instead */
#define PIO_IMR_P17_Pos                     17                                             /**< (PIO_IMR) Input Change Interrupt Mask Position */
#define PIO_IMR_P17_Msk                     (0x1U << PIO_IMR_P17_Pos)                      /**< (PIO_IMR) Input Change Interrupt Mask Mask */
#define PIO_IMR_P17                         PIO_IMR_P17_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_IMR_P17_Msk instead */
#define PIO_IMR_P18_Pos                     18                                             /**< (PIO_IMR) Input Change Interrupt Mask Position */
#define PIO_IMR_P18_Msk                     (0x1U << PIO_IMR_P18_Pos)                      /**< (PIO_IMR) Input Change Interrupt Mask Mask */
#define PIO_IMR_P18                         PIO_IMR_P18_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_IMR_P18_Msk instead */
#define PIO_IMR_P19_Pos                     19                                             /**< (PIO_IMR) Input Change Interrupt Mask Position */
#define PIO_IMR_P19_Msk                     (0x1U << PIO_IMR_P19_Pos)                      /**< (PIO_IMR) Input Change Interrupt Mask Mask */
#define PIO_IMR_P19                         PIO_IMR_P19_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_IMR_P19_Msk instead */
#define PIO_IMR_P20_Pos                     20                                             /**< (PIO_IMR) Input Change Interrupt Mask Position */
#define PIO_IMR_P20_Msk                     (0x1U << PIO_IMR_P20_Pos)                      /**< (PIO_IMR) Input Change Interrupt Mask Mask */
#define PIO_IMR_P20                         PIO_IMR_P20_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_IMR_P20_Msk instead */
#define PIO_IMR_P21_Pos                     21                                             /**< (PIO_IMR) Input Change Interrupt Mask Position */
#define PIO_IMR_P21_Msk                     (0x1U << PIO_IMR_P21_Pos)                      /**< (PIO_IMR) Input Change Interrupt Mask Mask */
#define PIO_IMR_P21                         PIO_IMR_P21_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_IMR_P21_Msk instead */
#define PIO_IMR_P22_Pos                     22                                             /**< (PIO_IMR) Input Change Interrupt Mask Position */
#define PIO_IMR_P22_Msk                     (0x1U << PIO_IMR_P22_Pos)                      /**< (PIO_IMR) Input Change Interrupt Mask Mask */
#define PIO_IMR_P22                         PIO_IMR_P22_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_IMR_P22_Msk instead */
#define PIO_IMR_P23_Pos                     23                                             /**< (PIO_IMR) Input Change Interrupt Mask Position */
#define PIO_IMR_P23_Msk                     (0x1U << PIO_IMR_P23_Pos)                      /**< (PIO_IMR) Input Change Interrupt Mask Mask */
#define PIO_IMR_P23                         PIO_IMR_P23_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_IMR_P23_Msk instead */
#define PIO_IMR_P24_Pos                     24                                             /**< (PIO_IMR) Input Change Interrupt Mask Position */
#define PIO_IMR_P24_Msk                     (0x1U << PIO_IMR_P24_Pos)                      /**< (PIO_IMR) Input Change Interrupt Mask Mask */
#define PIO_IMR_P24                         PIO_IMR_P24_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_IMR_P24_Msk instead */
#define PIO_IMR_P25_Pos                     25                                             /**< (PIO_IMR) Input Change Interrupt Mask Position */
#define PIO_IMR_P25_Msk                     (0x1U << PIO_IMR_P25_Pos)                      /**< (PIO_IMR) Input Change Interrupt Mask Mask */
#define PIO_IMR_P25                         PIO_IMR_P25_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_IMR_P25_Msk instead */
#define PIO_IMR_P26_Pos                     26                                             /**< (PIO_IMR) Input Change Interrupt Mask Position */
#define PIO_IMR_P26_Msk                     (0x1U << PIO_IMR_P26_Pos)                      /**< (PIO_IMR) Input Change Interrupt Mask Mask */
#define PIO_IMR_P26                         PIO_IMR_P26_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_IMR_P26_Msk instead */
#define PIO_IMR_P27_Pos                     27                                             /**< (PIO_IMR) Input Change Interrupt Mask Position */
#define PIO_IMR_P27_Msk                     (0x1U << PIO_IMR_P27_Pos)                      /**< (PIO_IMR) Input Change Interrupt Mask Mask */
#define PIO_IMR_P27                         PIO_IMR_P27_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_IMR_P27_Msk instead */
#define PIO_IMR_P28_Pos                     28                                             /**< (PIO_IMR) Input Change Interrupt Mask Position */
#define PIO_IMR_P28_Msk                     (0x1U << PIO_IMR_P28_Pos)                      /**< (PIO_IMR) Input Change Interrupt Mask Mask */
#define PIO_IMR_P28                         PIO_IMR_P28_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_IMR_P28_Msk instead */
#define PIO_IMR_P29_Pos                     29                                             /**< (PIO_IMR) Input Change Interrupt Mask Position */
#define PIO_IMR_P29_Msk                     (0x1U << PIO_IMR_P29_Pos)                      /**< (PIO_IMR) Input Change Interrupt Mask Mask */
#define PIO_IMR_P29                         PIO_IMR_P29_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_IMR_P29_Msk instead */
#define PIO_IMR_P30_Pos                     30                                             /**< (PIO_IMR) Input Change Interrupt Mask Position */
#define PIO_IMR_P30_Msk                     (0x1U << PIO_IMR_P30_Pos)                      /**< (PIO_IMR) Input Change Interrupt Mask Mask */
#define PIO_IMR_P30                         PIO_IMR_P30_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_IMR_P30_Msk instead */
#define PIO_IMR_P31_Pos                     31                                             /**< (PIO_IMR) Input Change Interrupt Mask Position */
#define PIO_IMR_P31_Msk                     (0x1U << PIO_IMR_P31_Pos)                      /**< (PIO_IMR) Input Change Interrupt Mask Mask */
#define PIO_IMR_P31                         PIO_IMR_P31_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_IMR_P31_Msk instead */
#define PIO_IMR_P_Pos                       0                                              /**< (PIO_IMR Position) Input Change Interrupt Mask */
#define PIO_IMR_P_Msk                       (0xFFFFFFFFU << PIO_IMR_P_Pos)                 /**< (PIO_IMR Mask) P */
#define PIO_IMR_P(value)                    (PIO_IMR_P_Msk & ((value) << PIO_IMR_P_Pos))   
#define PIO_IMR_MASK                        (0xFFFFFFFFU)                                  /**< \deprecated (PIO_IMR) Register MASK  (Use PIO_IMR_Msk instead)  */
#define PIO_IMR_Msk                         (0xFFFFFFFFU)                                  /**< (PIO_IMR) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- PIO_ISR : (PIO Offset: 0x4c) (R/ 32) Interrupt Status Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t P0:1;                      /**< bit:      0  Input Change Interrupt Status            */
    uint32_t P1:1;                      /**< bit:      1  Input Change Interrupt Status            */
    uint32_t P2:1;                      /**< bit:      2  Input Change Interrupt Status            */
    uint32_t P3:1;                      /**< bit:      3  Input Change Interrupt Status            */
    uint32_t P4:1;                      /**< bit:      4  Input Change Interrupt Status            */
    uint32_t P5:1;                      /**< bit:      5  Input Change Interrupt Status            */
    uint32_t P6:1;                      /**< bit:      6  Input Change Interrupt Status            */
    uint32_t P7:1;                      /**< bit:      7  Input Change Interrupt Status            */
    uint32_t P8:1;                      /**< bit:      8  Input Change Interrupt Status            */
    uint32_t P9:1;                      /**< bit:      9  Input Change Interrupt Status            */
    uint32_t P10:1;                     /**< bit:     10  Input Change Interrupt Status            */
    uint32_t P11:1;                     /**< bit:     11  Input Change Interrupt Status            */
    uint32_t P12:1;                     /**< bit:     12  Input Change Interrupt Status            */
    uint32_t P13:1;                     /**< bit:     13  Input Change Interrupt Status            */
    uint32_t P14:1;                     /**< bit:     14  Input Change Interrupt Status            */
    uint32_t P15:1;                     /**< bit:     15  Input Change Interrupt Status            */
    uint32_t P16:1;                     /**< bit:     16  Input Change Interrupt Status            */
    uint32_t P17:1;                     /**< bit:     17  Input Change Interrupt Status            */
    uint32_t P18:1;                     /**< bit:     18  Input Change Interrupt Status            */
    uint32_t P19:1;                     /**< bit:     19  Input Change Interrupt Status            */
    uint32_t P20:1;                     /**< bit:     20  Input Change Interrupt Status            */
    uint32_t P21:1;                     /**< bit:     21  Input Change Interrupt Status            */
    uint32_t P22:1;                     /**< bit:     22  Input Change Interrupt Status            */
    uint32_t P23:1;                     /**< bit:     23  Input Change Interrupt Status            */
    uint32_t P24:1;                     /**< bit:     24  Input Change Interrupt Status            */
    uint32_t P25:1;                     /**< bit:     25  Input Change Interrupt Status            */
    uint32_t P26:1;                     /**< bit:     26  Input Change Interrupt Status            */
    uint32_t P27:1;                     /**< bit:     27  Input Change Interrupt Status            */
    uint32_t P28:1;                     /**< bit:     28  Input Change Interrupt Status            */
    uint32_t P29:1;                     /**< bit:     29  Input Change Interrupt Status            */
    uint32_t P30:1;                     /**< bit:     30  Input Change Interrupt Status            */
    uint32_t P31:1;                     /**< bit:     31  Input Change Interrupt Status            */
  } bit;                                /**< Structure used for bit  access */
  struct {
    uint32_t P:32;                      /**< bit:  0..31  Input Change Interrupt Status            */
  } vec;                                /**< Structure used for vec  access  */
  uint32_t reg;                         /**< Type used for register access */
} PIO_ISR_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define PIO_ISR_OFFSET                      (0x4C)                                        /**<  (PIO_ISR) Interrupt Status Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define PIO_ISR_P0_Pos                      0                                              /**< (PIO_ISR) Input Change Interrupt Status Position */
#define PIO_ISR_P0_Msk                      (0x1U << PIO_ISR_P0_Pos)                       /**< (PIO_ISR) Input Change Interrupt Status Mask */
#define PIO_ISR_P0                          PIO_ISR_P0_Msk                                 /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_ISR_P0_Msk instead */
#define PIO_ISR_P1_Pos                      1                                              /**< (PIO_ISR) Input Change Interrupt Status Position */
#define PIO_ISR_P1_Msk                      (0x1U << PIO_ISR_P1_Pos)                       /**< (PIO_ISR) Input Change Interrupt Status Mask */
#define PIO_ISR_P1                          PIO_ISR_P1_Msk                                 /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_ISR_P1_Msk instead */
#define PIO_ISR_P2_Pos                      2                                              /**< (PIO_ISR) Input Change Interrupt Status Position */
#define PIO_ISR_P2_Msk                      (0x1U << PIO_ISR_P2_Pos)                       /**< (PIO_ISR) Input Change Interrupt Status Mask */
#define PIO_ISR_P2                          PIO_ISR_P2_Msk                                 /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_ISR_P2_Msk instead */
#define PIO_ISR_P3_Pos                      3                                              /**< (PIO_ISR) Input Change Interrupt Status Position */
#define PIO_ISR_P3_Msk                      (0x1U << PIO_ISR_P3_Pos)                       /**< (PIO_ISR) Input Change Interrupt Status Mask */
#define PIO_ISR_P3                          PIO_ISR_P3_Msk                                 /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_ISR_P3_Msk instead */
#define PIO_ISR_P4_Pos                      4                                              /**< (PIO_ISR) Input Change Interrupt Status Position */
#define PIO_ISR_P4_Msk                      (0x1U << PIO_ISR_P4_Pos)                       /**< (PIO_ISR) Input Change Interrupt Status Mask */
#define PIO_ISR_P4                          PIO_ISR_P4_Msk                                 /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_ISR_P4_Msk instead */
#define PIO_ISR_P5_Pos                      5                                              /**< (PIO_ISR) Input Change Interrupt Status Position */
#define PIO_ISR_P5_Msk                      (0x1U << PIO_ISR_P5_Pos)                       /**< (PIO_ISR) Input Change Interrupt Status Mask */
#define PIO_ISR_P5                          PIO_ISR_P5_Msk                                 /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_ISR_P5_Msk instead */
#define PIO_ISR_P6_Pos                      6                                              /**< (PIO_ISR) Input Change Interrupt Status Position */
#define PIO_ISR_P6_Msk                      (0x1U << PIO_ISR_P6_Pos)                       /**< (PIO_ISR) Input Change Interrupt Status Mask */
#define PIO_ISR_P6                          PIO_ISR_P6_Msk                                 /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_ISR_P6_Msk instead */
#define PIO_ISR_P7_Pos                      7                                              /**< (PIO_ISR) Input Change Interrupt Status Position */
#define PIO_ISR_P7_Msk                      (0x1U << PIO_ISR_P7_Pos)                       /**< (PIO_ISR) Input Change Interrupt Status Mask */
#define PIO_ISR_P7                          PIO_ISR_P7_Msk                                 /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_ISR_P7_Msk instead */
#define PIO_ISR_P8_Pos                      8                                              /**< (PIO_ISR) Input Change Interrupt Status Position */
#define PIO_ISR_P8_Msk                      (0x1U << PIO_ISR_P8_Pos)                       /**< (PIO_ISR) Input Change Interrupt Status Mask */
#define PIO_ISR_P8                          PIO_ISR_P8_Msk                                 /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_ISR_P8_Msk instead */
#define PIO_ISR_P9_Pos                      9                                              /**< (PIO_ISR) Input Change Interrupt Status Position */
#define PIO_ISR_P9_Msk                      (0x1U << PIO_ISR_P9_Pos)                       /**< (PIO_ISR) Input Change Interrupt Status Mask */
#define PIO_ISR_P9                          PIO_ISR_P9_Msk                                 /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_ISR_P9_Msk instead */
#define PIO_ISR_P10_Pos                     10                                             /**< (PIO_ISR) Input Change Interrupt Status Position */
#define PIO_ISR_P10_Msk                     (0x1U << PIO_ISR_P10_Pos)                      /**< (PIO_ISR) Input Change Interrupt Status Mask */
#define PIO_ISR_P10                         PIO_ISR_P10_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_ISR_P10_Msk instead */
#define PIO_ISR_P11_Pos                     11                                             /**< (PIO_ISR) Input Change Interrupt Status Position */
#define PIO_ISR_P11_Msk                     (0x1U << PIO_ISR_P11_Pos)                      /**< (PIO_ISR) Input Change Interrupt Status Mask */
#define PIO_ISR_P11                         PIO_ISR_P11_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_ISR_P11_Msk instead */
#define PIO_ISR_P12_Pos                     12                                             /**< (PIO_ISR) Input Change Interrupt Status Position */
#define PIO_ISR_P12_Msk                     (0x1U << PIO_ISR_P12_Pos)                      /**< (PIO_ISR) Input Change Interrupt Status Mask */
#define PIO_ISR_P12                         PIO_ISR_P12_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_ISR_P12_Msk instead */
#define PIO_ISR_P13_Pos                     13                                             /**< (PIO_ISR) Input Change Interrupt Status Position */
#define PIO_ISR_P13_Msk                     (0x1U << PIO_ISR_P13_Pos)                      /**< (PIO_ISR) Input Change Interrupt Status Mask */
#define PIO_ISR_P13                         PIO_ISR_P13_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_ISR_P13_Msk instead */
#define PIO_ISR_P14_Pos                     14                                             /**< (PIO_ISR) Input Change Interrupt Status Position */
#define PIO_ISR_P14_Msk                     (0x1U << PIO_ISR_P14_Pos)                      /**< (PIO_ISR) Input Change Interrupt Status Mask */
#define PIO_ISR_P14                         PIO_ISR_P14_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_ISR_P14_Msk instead */
#define PIO_ISR_P15_Pos                     15                                             /**< (PIO_ISR) Input Change Interrupt Status Position */
#define PIO_ISR_P15_Msk                     (0x1U << PIO_ISR_P15_Pos)                      /**< (PIO_ISR) Input Change Interrupt Status Mask */
#define PIO_ISR_P15                         PIO_ISR_P15_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_ISR_P15_Msk instead */
#define PIO_ISR_P16_Pos                     16                                             /**< (PIO_ISR) Input Change Interrupt Status Position */
#define PIO_ISR_P16_Msk                     (0x1U << PIO_ISR_P16_Pos)                      /**< (PIO_ISR) Input Change Interrupt Status Mask */
#define PIO_ISR_P16                         PIO_ISR_P16_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_ISR_P16_Msk instead */
#define PIO_ISR_P17_Pos                     17                                             /**< (PIO_ISR) Input Change Interrupt Status Position */
#define PIO_ISR_P17_Msk                     (0x1U << PIO_ISR_P17_Pos)                      /**< (PIO_ISR) Input Change Interrupt Status Mask */
#define PIO_ISR_P17                         PIO_ISR_P17_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_ISR_P17_Msk instead */
#define PIO_ISR_P18_Pos                     18                                             /**< (PIO_ISR) Input Change Interrupt Status Position */
#define PIO_ISR_P18_Msk                     (0x1U << PIO_ISR_P18_Pos)                      /**< (PIO_ISR) Input Change Interrupt Status Mask */
#define PIO_ISR_P18                         PIO_ISR_P18_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_ISR_P18_Msk instead */
#define PIO_ISR_P19_Pos                     19                                             /**< (PIO_ISR) Input Change Interrupt Status Position */
#define PIO_ISR_P19_Msk                     (0x1U << PIO_ISR_P19_Pos)                      /**< (PIO_ISR) Input Change Interrupt Status Mask */
#define PIO_ISR_P19                         PIO_ISR_P19_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_ISR_P19_Msk instead */
#define PIO_ISR_P20_Pos                     20                                             /**< (PIO_ISR) Input Change Interrupt Status Position */
#define PIO_ISR_P20_Msk                     (0x1U << PIO_ISR_P20_Pos)                      /**< (PIO_ISR) Input Change Interrupt Status Mask */
#define PIO_ISR_P20                         PIO_ISR_P20_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_ISR_P20_Msk instead */
#define PIO_ISR_P21_Pos                     21                                             /**< (PIO_ISR) Input Change Interrupt Status Position */
#define PIO_ISR_P21_Msk                     (0x1U << PIO_ISR_P21_Pos)                      /**< (PIO_ISR) Input Change Interrupt Status Mask */
#define PIO_ISR_P21                         PIO_ISR_P21_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_ISR_P21_Msk instead */
#define PIO_ISR_P22_Pos                     22                                             /**< (PIO_ISR) Input Change Interrupt Status Position */
#define PIO_ISR_P22_Msk                     (0x1U << PIO_ISR_P22_Pos)                      /**< (PIO_ISR) Input Change Interrupt Status Mask */
#define PIO_ISR_P22                         PIO_ISR_P22_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_ISR_P22_Msk instead */
#define PIO_ISR_P23_Pos                     23                                             /**< (PIO_ISR) Input Change Interrupt Status Position */
#define PIO_ISR_P23_Msk                     (0x1U << PIO_ISR_P23_Pos)                      /**< (PIO_ISR) Input Change Interrupt Status Mask */
#define PIO_ISR_P23                         PIO_ISR_P23_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_ISR_P23_Msk instead */
#define PIO_ISR_P24_Pos                     24                                             /**< (PIO_ISR) Input Change Interrupt Status Position */
#define PIO_ISR_P24_Msk                     (0x1U << PIO_ISR_P24_Pos)                      /**< (PIO_ISR) Input Change Interrupt Status Mask */
#define PIO_ISR_P24                         PIO_ISR_P24_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_ISR_P24_Msk instead */
#define PIO_ISR_P25_Pos                     25                                             /**< (PIO_ISR) Input Change Interrupt Status Position */
#define PIO_ISR_P25_Msk                     (0x1U << PIO_ISR_P25_Pos)                      /**< (PIO_ISR) Input Change Interrupt Status Mask */
#define PIO_ISR_P25                         PIO_ISR_P25_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_ISR_P25_Msk instead */
#define PIO_ISR_P26_Pos                     26                                             /**< (PIO_ISR) Input Change Interrupt Status Position */
#define PIO_ISR_P26_Msk                     (0x1U << PIO_ISR_P26_Pos)                      /**< (PIO_ISR) Input Change Interrupt Status Mask */
#define PIO_ISR_P26                         PIO_ISR_P26_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_ISR_P26_Msk instead */
#define PIO_ISR_P27_Pos                     27                                             /**< (PIO_ISR) Input Change Interrupt Status Position */
#define PIO_ISR_P27_Msk                     (0x1U << PIO_ISR_P27_Pos)                      /**< (PIO_ISR) Input Change Interrupt Status Mask */
#define PIO_ISR_P27                         PIO_ISR_P27_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_ISR_P27_Msk instead */
#define PIO_ISR_P28_Pos                     28                                             /**< (PIO_ISR) Input Change Interrupt Status Position */
#define PIO_ISR_P28_Msk                     (0x1U << PIO_ISR_P28_Pos)                      /**< (PIO_ISR) Input Change Interrupt Status Mask */
#define PIO_ISR_P28                         PIO_ISR_P28_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_ISR_P28_Msk instead */
#define PIO_ISR_P29_Pos                     29                                             /**< (PIO_ISR) Input Change Interrupt Status Position */
#define PIO_ISR_P29_Msk                     (0x1U << PIO_ISR_P29_Pos)                      /**< (PIO_ISR) Input Change Interrupt Status Mask */
#define PIO_ISR_P29                         PIO_ISR_P29_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_ISR_P29_Msk instead */
#define PIO_ISR_P30_Pos                     30                                             /**< (PIO_ISR) Input Change Interrupt Status Position */
#define PIO_ISR_P30_Msk                     (0x1U << PIO_ISR_P30_Pos)                      /**< (PIO_ISR) Input Change Interrupt Status Mask */
#define PIO_ISR_P30                         PIO_ISR_P30_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_ISR_P30_Msk instead */
#define PIO_ISR_P31_Pos                     31                                             /**< (PIO_ISR) Input Change Interrupt Status Position */
#define PIO_ISR_P31_Msk                     (0x1U << PIO_ISR_P31_Pos)                      /**< (PIO_ISR) Input Change Interrupt Status Mask */
#define PIO_ISR_P31                         PIO_ISR_P31_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_ISR_P31_Msk instead */
#define PIO_ISR_P_Pos                       0                                              /**< (PIO_ISR Position) Input Change Interrupt Status */
#define PIO_ISR_P_Msk                       (0xFFFFFFFFU << PIO_ISR_P_Pos)                 /**< (PIO_ISR Mask) P */
#define PIO_ISR_P(value)                    (PIO_ISR_P_Msk & ((value) << PIO_ISR_P_Pos))   
#define PIO_ISR_MASK                        (0xFFFFFFFFU)                                  /**< \deprecated (PIO_ISR) Register MASK  (Use PIO_ISR_Msk instead)  */
#define PIO_ISR_Msk                         (0xFFFFFFFFU)                                  /**< (PIO_ISR) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- PIO_MDER : (PIO Offset: 0x50) (/W 32) Multi-driver Enable Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t P0:1;                      /**< bit:      0  Multi-drive Enable                       */
    uint32_t P1:1;                      /**< bit:      1  Multi-drive Enable                       */
    uint32_t P2:1;                      /**< bit:      2  Multi-drive Enable                       */
    uint32_t P3:1;                      /**< bit:      3  Multi-drive Enable                       */
    uint32_t P4:1;                      /**< bit:      4  Multi-drive Enable                       */
    uint32_t P5:1;                      /**< bit:      5  Multi-drive Enable                       */
    uint32_t P6:1;                      /**< bit:      6  Multi-drive Enable                       */
    uint32_t P7:1;                      /**< bit:      7  Multi-drive Enable                       */
    uint32_t P8:1;                      /**< bit:      8  Multi-drive Enable                       */
    uint32_t P9:1;                      /**< bit:      9  Multi-drive Enable                       */
    uint32_t P10:1;                     /**< bit:     10  Multi-drive Enable                       */
    uint32_t P11:1;                     /**< bit:     11  Multi-drive Enable                       */
    uint32_t P12:1;                     /**< bit:     12  Multi-drive Enable                       */
    uint32_t P13:1;                     /**< bit:     13  Multi-drive Enable                       */
    uint32_t P14:1;                     /**< bit:     14  Multi-drive Enable                       */
    uint32_t P15:1;                     /**< bit:     15  Multi-drive Enable                       */
    uint32_t P16:1;                     /**< bit:     16  Multi-drive Enable                       */
    uint32_t P17:1;                     /**< bit:     17  Multi-drive Enable                       */
    uint32_t P18:1;                     /**< bit:     18  Multi-drive Enable                       */
    uint32_t P19:1;                     /**< bit:     19  Multi-drive Enable                       */
    uint32_t P20:1;                     /**< bit:     20  Multi-drive Enable                       */
    uint32_t P21:1;                     /**< bit:     21  Multi-drive Enable                       */
    uint32_t P22:1;                     /**< bit:     22  Multi-drive Enable                       */
    uint32_t P23:1;                     /**< bit:     23  Multi-drive Enable                       */
    uint32_t P24:1;                     /**< bit:     24  Multi-drive Enable                       */
    uint32_t P25:1;                     /**< bit:     25  Multi-drive Enable                       */
    uint32_t P26:1;                     /**< bit:     26  Multi-drive Enable                       */
    uint32_t P27:1;                     /**< bit:     27  Multi-drive Enable                       */
    uint32_t P28:1;                     /**< bit:     28  Multi-drive Enable                       */
    uint32_t P29:1;                     /**< bit:     29  Multi-drive Enable                       */
    uint32_t P30:1;                     /**< bit:     30  Multi-drive Enable                       */
    uint32_t P31:1;                     /**< bit:     31  Multi-drive Enable                       */
  } bit;                                /**< Structure used for bit  access */
  struct {
    uint32_t P:32;                      /**< bit:  0..31  Multi-drive Enable                       */
  } vec;                                /**< Structure used for vec  access  */
  uint32_t reg;                         /**< Type used for register access */
} PIO_MDER_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define PIO_MDER_OFFSET                     (0x50)                                        /**<  (PIO_MDER) Multi-driver Enable Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define PIO_MDER_P0_Pos                     0                                              /**< (PIO_MDER) Multi-drive Enable Position */
#define PIO_MDER_P0_Msk                     (0x1U << PIO_MDER_P0_Pos)                      /**< (PIO_MDER) Multi-drive Enable Mask */
#define PIO_MDER_P0                         PIO_MDER_P0_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_MDER_P0_Msk instead */
#define PIO_MDER_P1_Pos                     1                                              /**< (PIO_MDER) Multi-drive Enable Position */
#define PIO_MDER_P1_Msk                     (0x1U << PIO_MDER_P1_Pos)                      /**< (PIO_MDER) Multi-drive Enable Mask */
#define PIO_MDER_P1                         PIO_MDER_P1_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_MDER_P1_Msk instead */
#define PIO_MDER_P2_Pos                     2                                              /**< (PIO_MDER) Multi-drive Enable Position */
#define PIO_MDER_P2_Msk                     (0x1U << PIO_MDER_P2_Pos)                      /**< (PIO_MDER) Multi-drive Enable Mask */
#define PIO_MDER_P2                         PIO_MDER_P2_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_MDER_P2_Msk instead */
#define PIO_MDER_P3_Pos                     3                                              /**< (PIO_MDER) Multi-drive Enable Position */
#define PIO_MDER_P3_Msk                     (0x1U << PIO_MDER_P3_Pos)                      /**< (PIO_MDER) Multi-drive Enable Mask */
#define PIO_MDER_P3                         PIO_MDER_P3_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_MDER_P3_Msk instead */
#define PIO_MDER_P4_Pos                     4                                              /**< (PIO_MDER) Multi-drive Enable Position */
#define PIO_MDER_P4_Msk                     (0x1U << PIO_MDER_P4_Pos)                      /**< (PIO_MDER) Multi-drive Enable Mask */
#define PIO_MDER_P4                         PIO_MDER_P4_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_MDER_P4_Msk instead */
#define PIO_MDER_P5_Pos                     5                                              /**< (PIO_MDER) Multi-drive Enable Position */
#define PIO_MDER_P5_Msk                     (0x1U << PIO_MDER_P5_Pos)                      /**< (PIO_MDER) Multi-drive Enable Mask */
#define PIO_MDER_P5                         PIO_MDER_P5_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_MDER_P5_Msk instead */
#define PIO_MDER_P6_Pos                     6                                              /**< (PIO_MDER) Multi-drive Enable Position */
#define PIO_MDER_P6_Msk                     (0x1U << PIO_MDER_P6_Pos)                      /**< (PIO_MDER) Multi-drive Enable Mask */
#define PIO_MDER_P6                         PIO_MDER_P6_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_MDER_P6_Msk instead */
#define PIO_MDER_P7_Pos                     7                                              /**< (PIO_MDER) Multi-drive Enable Position */
#define PIO_MDER_P7_Msk                     (0x1U << PIO_MDER_P7_Pos)                      /**< (PIO_MDER) Multi-drive Enable Mask */
#define PIO_MDER_P7                         PIO_MDER_P7_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_MDER_P7_Msk instead */
#define PIO_MDER_P8_Pos                     8                                              /**< (PIO_MDER) Multi-drive Enable Position */
#define PIO_MDER_P8_Msk                     (0x1U << PIO_MDER_P8_Pos)                      /**< (PIO_MDER) Multi-drive Enable Mask */
#define PIO_MDER_P8                         PIO_MDER_P8_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_MDER_P8_Msk instead */
#define PIO_MDER_P9_Pos                     9                                              /**< (PIO_MDER) Multi-drive Enable Position */
#define PIO_MDER_P9_Msk                     (0x1U << PIO_MDER_P9_Pos)                      /**< (PIO_MDER) Multi-drive Enable Mask */
#define PIO_MDER_P9                         PIO_MDER_P9_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_MDER_P9_Msk instead */
#define PIO_MDER_P10_Pos                    10                                             /**< (PIO_MDER) Multi-drive Enable Position */
#define PIO_MDER_P10_Msk                    (0x1U << PIO_MDER_P10_Pos)                     /**< (PIO_MDER) Multi-drive Enable Mask */
#define PIO_MDER_P10                        PIO_MDER_P10_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_MDER_P10_Msk instead */
#define PIO_MDER_P11_Pos                    11                                             /**< (PIO_MDER) Multi-drive Enable Position */
#define PIO_MDER_P11_Msk                    (0x1U << PIO_MDER_P11_Pos)                     /**< (PIO_MDER) Multi-drive Enable Mask */
#define PIO_MDER_P11                        PIO_MDER_P11_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_MDER_P11_Msk instead */
#define PIO_MDER_P12_Pos                    12                                             /**< (PIO_MDER) Multi-drive Enable Position */
#define PIO_MDER_P12_Msk                    (0x1U << PIO_MDER_P12_Pos)                     /**< (PIO_MDER) Multi-drive Enable Mask */
#define PIO_MDER_P12                        PIO_MDER_P12_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_MDER_P12_Msk instead */
#define PIO_MDER_P13_Pos                    13                                             /**< (PIO_MDER) Multi-drive Enable Position */
#define PIO_MDER_P13_Msk                    (0x1U << PIO_MDER_P13_Pos)                     /**< (PIO_MDER) Multi-drive Enable Mask */
#define PIO_MDER_P13                        PIO_MDER_P13_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_MDER_P13_Msk instead */
#define PIO_MDER_P14_Pos                    14                                             /**< (PIO_MDER) Multi-drive Enable Position */
#define PIO_MDER_P14_Msk                    (0x1U << PIO_MDER_P14_Pos)                     /**< (PIO_MDER) Multi-drive Enable Mask */
#define PIO_MDER_P14                        PIO_MDER_P14_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_MDER_P14_Msk instead */
#define PIO_MDER_P15_Pos                    15                                             /**< (PIO_MDER) Multi-drive Enable Position */
#define PIO_MDER_P15_Msk                    (0x1U << PIO_MDER_P15_Pos)                     /**< (PIO_MDER) Multi-drive Enable Mask */
#define PIO_MDER_P15                        PIO_MDER_P15_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_MDER_P15_Msk instead */
#define PIO_MDER_P16_Pos                    16                                             /**< (PIO_MDER) Multi-drive Enable Position */
#define PIO_MDER_P16_Msk                    (0x1U << PIO_MDER_P16_Pos)                     /**< (PIO_MDER) Multi-drive Enable Mask */
#define PIO_MDER_P16                        PIO_MDER_P16_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_MDER_P16_Msk instead */
#define PIO_MDER_P17_Pos                    17                                             /**< (PIO_MDER) Multi-drive Enable Position */
#define PIO_MDER_P17_Msk                    (0x1U << PIO_MDER_P17_Pos)                     /**< (PIO_MDER) Multi-drive Enable Mask */
#define PIO_MDER_P17                        PIO_MDER_P17_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_MDER_P17_Msk instead */
#define PIO_MDER_P18_Pos                    18                                             /**< (PIO_MDER) Multi-drive Enable Position */
#define PIO_MDER_P18_Msk                    (0x1U << PIO_MDER_P18_Pos)                     /**< (PIO_MDER) Multi-drive Enable Mask */
#define PIO_MDER_P18                        PIO_MDER_P18_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_MDER_P18_Msk instead */
#define PIO_MDER_P19_Pos                    19                                             /**< (PIO_MDER) Multi-drive Enable Position */
#define PIO_MDER_P19_Msk                    (0x1U << PIO_MDER_P19_Pos)                     /**< (PIO_MDER) Multi-drive Enable Mask */
#define PIO_MDER_P19                        PIO_MDER_P19_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_MDER_P19_Msk instead */
#define PIO_MDER_P20_Pos                    20                                             /**< (PIO_MDER) Multi-drive Enable Position */
#define PIO_MDER_P20_Msk                    (0x1U << PIO_MDER_P20_Pos)                     /**< (PIO_MDER) Multi-drive Enable Mask */
#define PIO_MDER_P20                        PIO_MDER_P20_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_MDER_P20_Msk instead */
#define PIO_MDER_P21_Pos                    21                                             /**< (PIO_MDER) Multi-drive Enable Position */
#define PIO_MDER_P21_Msk                    (0x1U << PIO_MDER_P21_Pos)                     /**< (PIO_MDER) Multi-drive Enable Mask */
#define PIO_MDER_P21                        PIO_MDER_P21_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_MDER_P21_Msk instead */
#define PIO_MDER_P22_Pos                    22                                             /**< (PIO_MDER) Multi-drive Enable Position */
#define PIO_MDER_P22_Msk                    (0x1U << PIO_MDER_P22_Pos)                     /**< (PIO_MDER) Multi-drive Enable Mask */
#define PIO_MDER_P22                        PIO_MDER_P22_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_MDER_P22_Msk instead */
#define PIO_MDER_P23_Pos                    23                                             /**< (PIO_MDER) Multi-drive Enable Position */
#define PIO_MDER_P23_Msk                    (0x1U << PIO_MDER_P23_Pos)                     /**< (PIO_MDER) Multi-drive Enable Mask */
#define PIO_MDER_P23                        PIO_MDER_P23_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_MDER_P23_Msk instead */
#define PIO_MDER_P24_Pos                    24                                             /**< (PIO_MDER) Multi-drive Enable Position */
#define PIO_MDER_P24_Msk                    (0x1U << PIO_MDER_P24_Pos)                     /**< (PIO_MDER) Multi-drive Enable Mask */
#define PIO_MDER_P24                        PIO_MDER_P24_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_MDER_P24_Msk instead */
#define PIO_MDER_P25_Pos                    25                                             /**< (PIO_MDER) Multi-drive Enable Position */
#define PIO_MDER_P25_Msk                    (0x1U << PIO_MDER_P25_Pos)                     /**< (PIO_MDER) Multi-drive Enable Mask */
#define PIO_MDER_P25                        PIO_MDER_P25_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_MDER_P25_Msk instead */
#define PIO_MDER_P26_Pos                    26                                             /**< (PIO_MDER) Multi-drive Enable Position */
#define PIO_MDER_P26_Msk                    (0x1U << PIO_MDER_P26_Pos)                     /**< (PIO_MDER) Multi-drive Enable Mask */
#define PIO_MDER_P26                        PIO_MDER_P26_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_MDER_P26_Msk instead */
#define PIO_MDER_P27_Pos                    27                                             /**< (PIO_MDER) Multi-drive Enable Position */
#define PIO_MDER_P27_Msk                    (0x1U << PIO_MDER_P27_Pos)                     /**< (PIO_MDER) Multi-drive Enable Mask */
#define PIO_MDER_P27                        PIO_MDER_P27_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_MDER_P27_Msk instead */
#define PIO_MDER_P28_Pos                    28                                             /**< (PIO_MDER) Multi-drive Enable Position */
#define PIO_MDER_P28_Msk                    (0x1U << PIO_MDER_P28_Pos)                     /**< (PIO_MDER) Multi-drive Enable Mask */
#define PIO_MDER_P28                        PIO_MDER_P28_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_MDER_P28_Msk instead */
#define PIO_MDER_P29_Pos                    29                                             /**< (PIO_MDER) Multi-drive Enable Position */
#define PIO_MDER_P29_Msk                    (0x1U << PIO_MDER_P29_Pos)                     /**< (PIO_MDER) Multi-drive Enable Mask */
#define PIO_MDER_P29                        PIO_MDER_P29_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_MDER_P29_Msk instead */
#define PIO_MDER_P30_Pos                    30                                             /**< (PIO_MDER) Multi-drive Enable Position */
#define PIO_MDER_P30_Msk                    (0x1U << PIO_MDER_P30_Pos)                     /**< (PIO_MDER) Multi-drive Enable Mask */
#define PIO_MDER_P30                        PIO_MDER_P30_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_MDER_P30_Msk instead */
#define PIO_MDER_P31_Pos                    31                                             /**< (PIO_MDER) Multi-drive Enable Position */
#define PIO_MDER_P31_Msk                    (0x1U << PIO_MDER_P31_Pos)                     /**< (PIO_MDER) Multi-drive Enable Mask */
#define PIO_MDER_P31                        PIO_MDER_P31_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_MDER_P31_Msk instead */
#define PIO_MDER_P_Pos                      0                                              /**< (PIO_MDER Position) Multi-drive Enable */
#define PIO_MDER_P_Msk                      (0xFFFFFFFFU << PIO_MDER_P_Pos)                /**< (PIO_MDER Mask) P */
#define PIO_MDER_P(value)                   (PIO_MDER_P_Msk & ((value) << PIO_MDER_P_Pos))  
#define PIO_MDER_MASK                       (0xFFFFFFFFU)                                  /**< \deprecated (PIO_MDER) Register MASK  (Use PIO_MDER_Msk instead)  */
#define PIO_MDER_Msk                        (0xFFFFFFFFU)                                  /**< (PIO_MDER) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- PIO_MDDR : (PIO Offset: 0x54) (/W 32) Multi-driver Disable Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t P0:1;                      /**< bit:      0  Multi-drive Disable                      */
    uint32_t P1:1;                      /**< bit:      1  Multi-drive Disable                      */
    uint32_t P2:1;                      /**< bit:      2  Multi-drive Disable                      */
    uint32_t P3:1;                      /**< bit:      3  Multi-drive Disable                      */
    uint32_t P4:1;                      /**< bit:      4  Multi-drive Disable                      */
    uint32_t P5:1;                      /**< bit:      5  Multi-drive Disable                      */
    uint32_t P6:1;                      /**< bit:      6  Multi-drive Disable                      */
    uint32_t P7:1;                      /**< bit:      7  Multi-drive Disable                      */
    uint32_t P8:1;                      /**< bit:      8  Multi-drive Disable                      */
    uint32_t P9:1;                      /**< bit:      9  Multi-drive Disable                      */
    uint32_t P10:1;                     /**< bit:     10  Multi-drive Disable                      */
    uint32_t P11:1;                     /**< bit:     11  Multi-drive Disable                      */
    uint32_t P12:1;                     /**< bit:     12  Multi-drive Disable                      */
    uint32_t P13:1;                     /**< bit:     13  Multi-drive Disable                      */
    uint32_t P14:1;                     /**< bit:     14  Multi-drive Disable                      */
    uint32_t P15:1;                     /**< bit:     15  Multi-drive Disable                      */
    uint32_t P16:1;                     /**< bit:     16  Multi-drive Disable                      */
    uint32_t P17:1;                     /**< bit:     17  Multi-drive Disable                      */
    uint32_t P18:1;                     /**< bit:     18  Multi-drive Disable                      */
    uint32_t P19:1;                     /**< bit:     19  Multi-drive Disable                      */
    uint32_t P20:1;                     /**< bit:     20  Multi-drive Disable                      */
    uint32_t P21:1;                     /**< bit:     21  Multi-drive Disable                      */
    uint32_t P22:1;                     /**< bit:     22  Multi-drive Disable                      */
    uint32_t P23:1;                     /**< bit:     23  Multi-drive Disable                      */
    uint32_t P24:1;                     /**< bit:     24  Multi-drive Disable                      */
    uint32_t P25:1;                     /**< bit:     25  Multi-drive Disable                      */
    uint32_t P26:1;                     /**< bit:     26  Multi-drive Disable                      */
    uint32_t P27:1;                     /**< bit:     27  Multi-drive Disable                      */
    uint32_t P28:1;                     /**< bit:     28  Multi-drive Disable                      */
    uint32_t P29:1;                     /**< bit:     29  Multi-drive Disable                      */
    uint32_t P30:1;                     /**< bit:     30  Multi-drive Disable                      */
    uint32_t P31:1;                     /**< bit:     31  Multi-drive Disable                      */
  } bit;                                /**< Structure used for bit  access */
  struct {
    uint32_t P:32;                      /**< bit:  0..31  Multi-drive Disable                      */
  } vec;                                /**< Structure used for vec  access  */
  uint32_t reg;                         /**< Type used for register access */
} PIO_MDDR_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define PIO_MDDR_OFFSET                     (0x54)                                        /**<  (PIO_MDDR) Multi-driver Disable Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define PIO_MDDR_P0_Pos                     0                                              /**< (PIO_MDDR) Multi-drive Disable Position */
#define PIO_MDDR_P0_Msk                     (0x1U << PIO_MDDR_P0_Pos)                      /**< (PIO_MDDR) Multi-drive Disable Mask */
#define PIO_MDDR_P0                         PIO_MDDR_P0_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_MDDR_P0_Msk instead */
#define PIO_MDDR_P1_Pos                     1                                              /**< (PIO_MDDR) Multi-drive Disable Position */
#define PIO_MDDR_P1_Msk                     (0x1U << PIO_MDDR_P1_Pos)                      /**< (PIO_MDDR) Multi-drive Disable Mask */
#define PIO_MDDR_P1                         PIO_MDDR_P1_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_MDDR_P1_Msk instead */
#define PIO_MDDR_P2_Pos                     2                                              /**< (PIO_MDDR) Multi-drive Disable Position */
#define PIO_MDDR_P2_Msk                     (0x1U << PIO_MDDR_P2_Pos)                      /**< (PIO_MDDR) Multi-drive Disable Mask */
#define PIO_MDDR_P2                         PIO_MDDR_P2_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_MDDR_P2_Msk instead */
#define PIO_MDDR_P3_Pos                     3                                              /**< (PIO_MDDR) Multi-drive Disable Position */
#define PIO_MDDR_P3_Msk                     (0x1U << PIO_MDDR_P3_Pos)                      /**< (PIO_MDDR) Multi-drive Disable Mask */
#define PIO_MDDR_P3                         PIO_MDDR_P3_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_MDDR_P3_Msk instead */
#define PIO_MDDR_P4_Pos                     4                                              /**< (PIO_MDDR) Multi-drive Disable Position */
#define PIO_MDDR_P4_Msk                     (0x1U << PIO_MDDR_P4_Pos)                      /**< (PIO_MDDR) Multi-drive Disable Mask */
#define PIO_MDDR_P4                         PIO_MDDR_P4_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_MDDR_P4_Msk instead */
#define PIO_MDDR_P5_Pos                     5                                              /**< (PIO_MDDR) Multi-drive Disable Position */
#define PIO_MDDR_P5_Msk                     (0x1U << PIO_MDDR_P5_Pos)                      /**< (PIO_MDDR) Multi-drive Disable Mask */
#define PIO_MDDR_P5                         PIO_MDDR_P5_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_MDDR_P5_Msk instead */
#define PIO_MDDR_P6_Pos                     6                                              /**< (PIO_MDDR) Multi-drive Disable Position */
#define PIO_MDDR_P6_Msk                     (0x1U << PIO_MDDR_P6_Pos)                      /**< (PIO_MDDR) Multi-drive Disable Mask */
#define PIO_MDDR_P6                         PIO_MDDR_P6_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_MDDR_P6_Msk instead */
#define PIO_MDDR_P7_Pos                     7                                              /**< (PIO_MDDR) Multi-drive Disable Position */
#define PIO_MDDR_P7_Msk                     (0x1U << PIO_MDDR_P7_Pos)                      /**< (PIO_MDDR) Multi-drive Disable Mask */
#define PIO_MDDR_P7                         PIO_MDDR_P7_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_MDDR_P7_Msk instead */
#define PIO_MDDR_P8_Pos                     8                                              /**< (PIO_MDDR) Multi-drive Disable Position */
#define PIO_MDDR_P8_Msk                     (0x1U << PIO_MDDR_P8_Pos)                      /**< (PIO_MDDR) Multi-drive Disable Mask */
#define PIO_MDDR_P8                         PIO_MDDR_P8_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_MDDR_P8_Msk instead */
#define PIO_MDDR_P9_Pos                     9                                              /**< (PIO_MDDR) Multi-drive Disable Position */
#define PIO_MDDR_P9_Msk                     (0x1U << PIO_MDDR_P9_Pos)                      /**< (PIO_MDDR) Multi-drive Disable Mask */
#define PIO_MDDR_P9                         PIO_MDDR_P9_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_MDDR_P9_Msk instead */
#define PIO_MDDR_P10_Pos                    10                                             /**< (PIO_MDDR) Multi-drive Disable Position */
#define PIO_MDDR_P10_Msk                    (0x1U << PIO_MDDR_P10_Pos)                     /**< (PIO_MDDR) Multi-drive Disable Mask */
#define PIO_MDDR_P10                        PIO_MDDR_P10_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_MDDR_P10_Msk instead */
#define PIO_MDDR_P11_Pos                    11                                             /**< (PIO_MDDR) Multi-drive Disable Position */
#define PIO_MDDR_P11_Msk                    (0x1U << PIO_MDDR_P11_Pos)                     /**< (PIO_MDDR) Multi-drive Disable Mask */
#define PIO_MDDR_P11                        PIO_MDDR_P11_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_MDDR_P11_Msk instead */
#define PIO_MDDR_P12_Pos                    12                                             /**< (PIO_MDDR) Multi-drive Disable Position */
#define PIO_MDDR_P12_Msk                    (0x1U << PIO_MDDR_P12_Pos)                     /**< (PIO_MDDR) Multi-drive Disable Mask */
#define PIO_MDDR_P12                        PIO_MDDR_P12_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_MDDR_P12_Msk instead */
#define PIO_MDDR_P13_Pos                    13                                             /**< (PIO_MDDR) Multi-drive Disable Position */
#define PIO_MDDR_P13_Msk                    (0x1U << PIO_MDDR_P13_Pos)                     /**< (PIO_MDDR) Multi-drive Disable Mask */
#define PIO_MDDR_P13                        PIO_MDDR_P13_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_MDDR_P13_Msk instead */
#define PIO_MDDR_P14_Pos                    14                                             /**< (PIO_MDDR) Multi-drive Disable Position */
#define PIO_MDDR_P14_Msk                    (0x1U << PIO_MDDR_P14_Pos)                     /**< (PIO_MDDR) Multi-drive Disable Mask */
#define PIO_MDDR_P14                        PIO_MDDR_P14_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_MDDR_P14_Msk instead */
#define PIO_MDDR_P15_Pos                    15                                             /**< (PIO_MDDR) Multi-drive Disable Position */
#define PIO_MDDR_P15_Msk                    (0x1U << PIO_MDDR_P15_Pos)                     /**< (PIO_MDDR) Multi-drive Disable Mask */
#define PIO_MDDR_P15                        PIO_MDDR_P15_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_MDDR_P15_Msk instead */
#define PIO_MDDR_P16_Pos                    16                                             /**< (PIO_MDDR) Multi-drive Disable Position */
#define PIO_MDDR_P16_Msk                    (0x1U << PIO_MDDR_P16_Pos)                     /**< (PIO_MDDR) Multi-drive Disable Mask */
#define PIO_MDDR_P16                        PIO_MDDR_P16_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_MDDR_P16_Msk instead */
#define PIO_MDDR_P17_Pos                    17                                             /**< (PIO_MDDR) Multi-drive Disable Position */
#define PIO_MDDR_P17_Msk                    (0x1U << PIO_MDDR_P17_Pos)                     /**< (PIO_MDDR) Multi-drive Disable Mask */
#define PIO_MDDR_P17                        PIO_MDDR_P17_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_MDDR_P17_Msk instead */
#define PIO_MDDR_P18_Pos                    18                                             /**< (PIO_MDDR) Multi-drive Disable Position */
#define PIO_MDDR_P18_Msk                    (0x1U << PIO_MDDR_P18_Pos)                     /**< (PIO_MDDR) Multi-drive Disable Mask */
#define PIO_MDDR_P18                        PIO_MDDR_P18_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_MDDR_P18_Msk instead */
#define PIO_MDDR_P19_Pos                    19                                             /**< (PIO_MDDR) Multi-drive Disable Position */
#define PIO_MDDR_P19_Msk                    (0x1U << PIO_MDDR_P19_Pos)                     /**< (PIO_MDDR) Multi-drive Disable Mask */
#define PIO_MDDR_P19                        PIO_MDDR_P19_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_MDDR_P19_Msk instead */
#define PIO_MDDR_P20_Pos                    20                                             /**< (PIO_MDDR) Multi-drive Disable Position */
#define PIO_MDDR_P20_Msk                    (0x1U << PIO_MDDR_P20_Pos)                     /**< (PIO_MDDR) Multi-drive Disable Mask */
#define PIO_MDDR_P20                        PIO_MDDR_P20_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_MDDR_P20_Msk instead */
#define PIO_MDDR_P21_Pos                    21                                             /**< (PIO_MDDR) Multi-drive Disable Position */
#define PIO_MDDR_P21_Msk                    (0x1U << PIO_MDDR_P21_Pos)                     /**< (PIO_MDDR) Multi-drive Disable Mask */
#define PIO_MDDR_P21                        PIO_MDDR_P21_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_MDDR_P21_Msk instead */
#define PIO_MDDR_P22_Pos                    22                                             /**< (PIO_MDDR) Multi-drive Disable Position */
#define PIO_MDDR_P22_Msk                    (0x1U << PIO_MDDR_P22_Pos)                     /**< (PIO_MDDR) Multi-drive Disable Mask */
#define PIO_MDDR_P22                        PIO_MDDR_P22_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_MDDR_P22_Msk instead */
#define PIO_MDDR_P23_Pos                    23                                             /**< (PIO_MDDR) Multi-drive Disable Position */
#define PIO_MDDR_P23_Msk                    (0x1U << PIO_MDDR_P23_Pos)                     /**< (PIO_MDDR) Multi-drive Disable Mask */
#define PIO_MDDR_P23                        PIO_MDDR_P23_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_MDDR_P23_Msk instead */
#define PIO_MDDR_P24_Pos                    24                                             /**< (PIO_MDDR) Multi-drive Disable Position */
#define PIO_MDDR_P24_Msk                    (0x1U << PIO_MDDR_P24_Pos)                     /**< (PIO_MDDR) Multi-drive Disable Mask */
#define PIO_MDDR_P24                        PIO_MDDR_P24_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_MDDR_P24_Msk instead */
#define PIO_MDDR_P25_Pos                    25                                             /**< (PIO_MDDR) Multi-drive Disable Position */
#define PIO_MDDR_P25_Msk                    (0x1U << PIO_MDDR_P25_Pos)                     /**< (PIO_MDDR) Multi-drive Disable Mask */
#define PIO_MDDR_P25                        PIO_MDDR_P25_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_MDDR_P25_Msk instead */
#define PIO_MDDR_P26_Pos                    26                                             /**< (PIO_MDDR) Multi-drive Disable Position */
#define PIO_MDDR_P26_Msk                    (0x1U << PIO_MDDR_P26_Pos)                     /**< (PIO_MDDR) Multi-drive Disable Mask */
#define PIO_MDDR_P26                        PIO_MDDR_P26_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_MDDR_P26_Msk instead */
#define PIO_MDDR_P27_Pos                    27                                             /**< (PIO_MDDR) Multi-drive Disable Position */
#define PIO_MDDR_P27_Msk                    (0x1U << PIO_MDDR_P27_Pos)                     /**< (PIO_MDDR) Multi-drive Disable Mask */
#define PIO_MDDR_P27                        PIO_MDDR_P27_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_MDDR_P27_Msk instead */
#define PIO_MDDR_P28_Pos                    28                                             /**< (PIO_MDDR) Multi-drive Disable Position */
#define PIO_MDDR_P28_Msk                    (0x1U << PIO_MDDR_P28_Pos)                     /**< (PIO_MDDR) Multi-drive Disable Mask */
#define PIO_MDDR_P28                        PIO_MDDR_P28_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_MDDR_P28_Msk instead */
#define PIO_MDDR_P29_Pos                    29                                             /**< (PIO_MDDR) Multi-drive Disable Position */
#define PIO_MDDR_P29_Msk                    (0x1U << PIO_MDDR_P29_Pos)                     /**< (PIO_MDDR) Multi-drive Disable Mask */
#define PIO_MDDR_P29                        PIO_MDDR_P29_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_MDDR_P29_Msk instead */
#define PIO_MDDR_P30_Pos                    30                                             /**< (PIO_MDDR) Multi-drive Disable Position */
#define PIO_MDDR_P30_Msk                    (0x1U << PIO_MDDR_P30_Pos)                     /**< (PIO_MDDR) Multi-drive Disable Mask */
#define PIO_MDDR_P30                        PIO_MDDR_P30_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_MDDR_P30_Msk instead */
#define PIO_MDDR_P31_Pos                    31                                             /**< (PIO_MDDR) Multi-drive Disable Position */
#define PIO_MDDR_P31_Msk                    (0x1U << PIO_MDDR_P31_Pos)                     /**< (PIO_MDDR) Multi-drive Disable Mask */
#define PIO_MDDR_P31                        PIO_MDDR_P31_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_MDDR_P31_Msk instead */
#define PIO_MDDR_P_Pos                      0                                              /**< (PIO_MDDR Position) Multi-drive Disable */
#define PIO_MDDR_P_Msk                      (0xFFFFFFFFU << PIO_MDDR_P_Pos)                /**< (PIO_MDDR Mask) P */
#define PIO_MDDR_P(value)                   (PIO_MDDR_P_Msk & ((value) << PIO_MDDR_P_Pos))  
#define PIO_MDDR_MASK                       (0xFFFFFFFFU)                                  /**< \deprecated (PIO_MDDR) Register MASK  (Use PIO_MDDR_Msk instead)  */
#define PIO_MDDR_Msk                        (0xFFFFFFFFU)                                  /**< (PIO_MDDR) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- PIO_MDSR : (PIO Offset: 0x58) (R/ 32) Multi-driver Status Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t P0:1;                      /**< bit:      0  Multi-drive Status                       */
    uint32_t P1:1;                      /**< bit:      1  Multi-drive Status                       */
    uint32_t P2:1;                      /**< bit:      2  Multi-drive Status                       */
    uint32_t P3:1;                      /**< bit:      3  Multi-drive Status                       */
    uint32_t P4:1;                      /**< bit:      4  Multi-drive Status                       */
    uint32_t P5:1;                      /**< bit:      5  Multi-drive Status                       */
    uint32_t P6:1;                      /**< bit:      6  Multi-drive Status                       */
    uint32_t P7:1;                      /**< bit:      7  Multi-drive Status                       */
    uint32_t P8:1;                      /**< bit:      8  Multi-drive Status                       */
    uint32_t P9:1;                      /**< bit:      9  Multi-drive Status                       */
    uint32_t P10:1;                     /**< bit:     10  Multi-drive Status                       */
    uint32_t P11:1;                     /**< bit:     11  Multi-drive Status                       */
    uint32_t P12:1;                     /**< bit:     12  Multi-drive Status                       */
    uint32_t P13:1;                     /**< bit:     13  Multi-drive Status                       */
    uint32_t P14:1;                     /**< bit:     14  Multi-drive Status                       */
    uint32_t P15:1;                     /**< bit:     15  Multi-drive Status                       */
    uint32_t P16:1;                     /**< bit:     16  Multi-drive Status                       */
    uint32_t P17:1;                     /**< bit:     17  Multi-drive Status                       */
    uint32_t P18:1;                     /**< bit:     18  Multi-drive Status                       */
    uint32_t P19:1;                     /**< bit:     19  Multi-drive Status                       */
    uint32_t P20:1;                     /**< bit:     20  Multi-drive Status                       */
    uint32_t P21:1;                     /**< bit:     21  Multi-drive Status                       */
    uint32_t P22:1;                     /**< bit:     22  Multi-drive Status                       */
    uint32_t P23:1;                     /**< bit:     23  Multi-drive Status                       */
    uint32_t P24:1;                     /**< bit:     24  Multi-drive Status                       */
    uint32_t P25:1;                     /**< bit:     25  Multi-drive Status                       */
    uint32_t P26:1;                     /**< bit:     26  Multi-drive Status                       */
    uint32_t P27:1;                     /**< bit:     27  Multi-drive Status                       */
    uint32_t P28:1;                     /**< bit:     28  Multi-drive Status                       */
    uint32_t P29:1;                     /**< bit:     29  Multi-drive Status                       */
    uint32_t P30:1;                     /**< bit:     30  Multi-drive Status                       */
    uint32_t P31:1;                     /**< bit:     31  Multi-drive Status                       */
  } bit;                                /**< Structure used for bit  access */
  struct {
    uint32_t P:32;                      /**< bit:  0..31  Multi-drive Status                       */
  } vec;                                /**< Structure used for vec  access  */
  uint32_t reg;                         /**< Type used for register access */
} PIO_MDSR_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define PIO_MDSR_OFFSET                     (0x58)                                        /**<  (PIO_MDSR) Multi-driver Status Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define PIO_MDSR_P0_Pos                     0                                              /**< (PIO_MDSR) Multi-drive Status Position */
#define PIO_MDSR_P0_Msk                     (0x1U << PIO_MDSR_P0_Pos)                      /**< (PIO_MDSR) Multi-drive Status Mask */
#define PIO_MDSR_P0                         PIO_MDSR_P0_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_MDSR_P0_Msk instead */
#define PIO_MDSR_P1_Pos                     1                                              /**< (PIO_MDSR) Multi-drive Status Position */
#define PIO_MDSR_P1_Msk                     (0x1U << PIO_MDSR_P1_Pos)                      /**< (PIO_MDSR) Multi-drive Status Mask */
#define PIO_MDSR_P1                         PIO_MDSR_P1_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_MDSR_P1_Msk instead */
#define PIO_MDSR_P2_Pos                     2                                              /**< (PIO_MDSR) Multi-drive Status Position */
#define PIO_MDSR_P2_Msk                     (0x1U << PIO_MDSR_P2_Pos)                      /**< (PIO_MDSR) Multi-drive Status Mask */
#define PIO_MDSR_P2                         PIO_MDSR_P2_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_MDSR_P2_Msk instead */
#define PIO_MDSR_P3_Pos                     3                                              /**< (PIO_MDSR) Multi-drive Status Position */
#define PIO_MDSR_P3_Msk                     (0x1U << PIO_MDSR_P3_Pos)                      /**< (PIO_MDSR) Multi-drive Status Mask */
#define PIO_MDSR_P3                         PIO_MDSR_P3_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_MDSR_P3_Msk instead */
#define PIO_MDSR_P4_Pos                     4                                              /**< (PIO_MDSR) Multi-drive Status Position */
#define PIO_MDSR_P4_Msk                     (0x1U << PIO_MDSR_P4_Pos)                      /**< (PIO_MDSR) Multi-drive Status Mask */
#define PIO_MDSR_P4                         PIO_MDSR_P4_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_MDSR_P4_Msk instead */
#define PIO_MDSR_P5_Pos                     5                                              /**< (PIO_MDSR) Multi-drive Status Position */
#define PIO_MDSR_P5_Msk                     (0x1U << PIO_MDSR_P5_Pos)                      /**< (PIO_MDSR) Multi-drive Status Mask */
#define PIO_MDSR_P5                         PIO_MDSR_P5_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_MDSR_P5_Msk instead */
#define PIO_MDSR_P6_Pos                     6                                              /**< (PIO_MDSR) Multi-drive Status Position */
#define PIO_MDSR_P6_Msk                     (0x1U << PIO_MDSR_P6_Pos)                      /**< (PIO_MDSR) Multi-drive Status Mask */
#define PIO_MDSR_P6                         PIO_MDSR_P6_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_MDSR_P6_Msk instead */
#define PIO_MDSR_P7_Pos                     7                                              /**< (PIO_MDSR) Multi-drive Status Position */
#define PIO_MDSR_P7_Msk                     (0x1U << PIO_MDSR_P7_Pos)                      /**< (PIO_MDSR) Multi-drive Status Mask */
#define PIO_MDSR_P7                         PIO_MDSR_P7_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_MDSR_P7_Msk instead */
#define PIO_MDSR_P8_Pos                     8                                              /**< (PIO_MDSR) Multi-drive Status Position */
#define PIO_MDSR_P8_Msk                     (0x1U << PIO_MDSR_P8_Pos)                      /**< (PIO_MDSR) Multi-drive Status Mask */
#define PIO_MDSR_P8                         PIO_MDSR_P8_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_MDSR_P8_Msk instead */
#define PIO_MDSR_P9_Pos                     9                                              /**< (PIO_MDSR) Multi-drive Status Position */
#define PIO_MDSR_P9_Msk                     (0x1U << PIO_MDSR_P9_Pos)                      /**< (PIO_MDSR) Multi-drive Status Mask */
#define PIO_MDSR_P9                         PIO_MDSR_P9_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_MDSR_P9_Msk instead */
#define PIO_MDSR_P10_Pos                    10                                             /**< (PIO_MDSR) Multi-drive Status Position */
#define PIO_MDSR_P10_Msk                    (0x1U << PIO_MDSR_P10_Pos)                     /**< (PIO_MDSR) Multi-drive Status Mask */
#define PIO_MDSR_P10                        PIO_MDSR_P10_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_MDSR_P10_Msk instead */
#define PIO_MDSR_P11_Pos                    11                                             /**< (PIO_MDSR) Multi-drive Status Position */
#define PIO_MDSR_P11_Msk                    (0x1U << PIO_MDSR_P11_Pos)                     /**< (PIO_MDSR) Multi-drive Status Mask */
#define PIO_MDSR_P11                        PIO_MDSR_P11_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_MDSR_P11_Msk instead */
#define PIO_MDSR_P12_Pos                    12                                             /**< (PIO_MDSR) Multi-drive Status Position */
#define PIO_MDSR_P12_Msk                    (0x1U << PIO_MDSR_P12_Pos)                     /**< (PIO_MDSR) Multi-drive Status Mask */
#define PIO_MDSR_P12                        PIO_MDSR_P12_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_MDSR_P12_Msk instead */
#define PIO_MDSR_P13_Pos                    13                                             /**< (PIO_MDSR) Multi-drive Status Position */
#define PIO_MDSR_P13_Msk                    (0x1U << PIO_MDSR_P13_Pos)                     /**< (PIO_MDSR) Multi-drive Status Mask */
#define PIO_MDSR_P13                        PIO_MDSR_P13_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_MDSR_P13_Msk instead */
#define PIO_MDSR_P14_Pos                    14                                             /**< (PIO_MDSR) Multi-drive Status Position */
#define PIO_MDSR_P14_Msk                    (0x1U << PIO_MDSR_P14_Pos)                     /**< (PIO_MDSR) Multi-drive Status Mask */
#define PIO_MDSR_P14                        PIO_MDSR_P14_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_MDSR_P14_Msk instead */
#define PIO_MDSR_P15_Pos                    15                                             /**< (PIO_MDSR) Multi-drive Status Position */
#define PIO_MDSR_P15_Msk                    (0x1U << PIO_MDSR_P15_Pos)                     /**< (PIO_MDSR) Multi-drive Status Mask */
#define PIO_MDSR_P15                        PIO_MDSR_P15_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_MDSR_P15_Msk instead */
#define PIO_MDSR_P16_Pos                    16                                             /**< (PIO_MDSR) Multi-drive Status Position */
#define PIO_MDSR_P16_Msk                    (0x1U << PIO_MDSR_P16_Pos)                     /**< (PIO_MDSR) Multi-drive Status Mask */
#define PIO_MDSR_P16                        PIO_MDSR_P16_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_MDSR_P16_Msk instead */
#define PIO_MDSR_P17_Pos                    17                                             /**< (PIO_MDSR) Multi-drive Status Position */
#define PIO_MDSR_P17_Msk                    (0x1U << PIO_MDSR_P17_Pos)                     /**< (PIO_MDSR) Multi-drive Status Mask */
#define PIO_MDSR_P17                        PIO_MDSR_P17_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_MDSR_P17_Msk instead */
#define PIO_MDSR_P18_Pos                    18                                             /**< (PIO_MDSR) Multi-drive Status Position */
#define PIO_MDSR_P18_Msk                    (0x1U << PIO_MDSR_P18_Pos)                     /**< (PIO_MDSR) Multi-drive Status Mask */
#define PIO_MDSR_P18                        PIO_MDSR_P18_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_MDSR_P18_Msk instead */
#define PIO_MDSR_P19_Pos                    19                                             /**< (PIO_MDSR) Multi-drive Status Position */
#define PIO_MDSR_P19_Msk                    (0x1U << PIO_MDSR_P19_Pos)                     /**< (PIO_MDSR) Multi-drive Status Mask */
#define PIO_MDSR_P19                        PIO_MDSR_P19_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_MDSR_P19_Msk instead */
#define PIO_MDSR_P20_Pos                    20                                             /**< (PIO_MDSR) Multi-drive Status Position */
#define PIO_MDSR_P20_Msk                    (0x1U << PIO_MDSR_P20_Pos)                     /**< (PIO_MDSR) Multi-drive Status Mask */
#define PIO_MDSR_P20                        PIO_MDSR_P20_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_MDSR_P20_Msk instead */
#define PIO_MDSR_P21_Pos                    21                                             /**< (PIO_MDSR) Multi-drive Status Position */
#define PIO_MDSR_P21_Msk                    (0x1U << PIO_MDSR_P21_Pos)                     /**< (PIO_MDSR) Multi-drive Status Mask */
#define PIO_MDSR_P21                        PIO_MDSR_P21_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_MDSR_P21_Msk instead */
#define PIO_MDSR_P22_Pos                    22                                             /**< (PIO_MDSR) Multi-drive Status Position */
#define PIO_MDSR_P22_Msk                    (0x1U << PIO_MDSR_P22_Pos)                     /**< (PIO_MDSR) Multi-drive Status Mask */
#define PIO_MDSR_P22                        PIO_MDSR_P22_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_MDSR_P22_Msk instead */
#define PIO_MDSR_P23_Pos                    23                                             /**< (PIO_MDSR) Multi-drive Status Position */
#define PIO_MDSR_P23_Msk                    (0x1U << PIO_MDSR_P23_Pos)                     /**< (PIO_MDSR) Multi-drive Status Mask */
#define PIO_MDSR_P23                        PIO_MDSR_P23_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_MDSR_P23_Msk instead */
#define PIO_MDSR_P24_Pos                    24                                             /**< (PIO_MDSR) Multi-drive Status Position */
#define PIO_MDSR_P24_Msk                    (0x1U << PIO_MDSR_P24_Pos)                     /**< (PIO_MDSR) Multi-drive Status Mask */
#define PIO_MDSR_P24                        PIO_MDSR_P24_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_MDSR_P24_Msk instead */
#define PIO_MDSR_P25_Pos                    25                                             /**< (PIO_MDSR) Multi-drive Status Position */
#define PIO_MDSR_P25_Msk                    (0x1U << PIO_MDSR_P25_Pos)                     /**< (PIO_MDSR) Multi-drive Status Mask */
#define PIO_MDSR_P25                        PIO_MDSR_P25_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_MDSR_P25_Msk instead */
#define PIO_MDSR_P26_Pos                    26                                             /**< (PIO_MDSR) Multi-drive Status Position */
#define PIO_MDSR_P26_Msk                    (0x1U << PIO_MDSR_P26_Pos)                     /**< (PIO_MDSR) Multi-drive Status Mask */
#define PIO_MDSR_P26                        PIO_MDSR_P26_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_MDSR_P26_Msk instead */
#define PIO_MDSR_P27_Pos                    27                                             /**< (PIO_MDSR) Multi-drive Status Position */
#define PIO_MDSR_P27_Msk                    (0x1U << PIO_MDSR_P27_Pos)                     /**< (PIO_MDSR) Multi-drive Status Mask */
#define PIO_MDSR_P27                        PIO_MDSR_P27_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_MDSR_P27_Msk instead */
#define PIO_MDSR_P28_Pos                    28                                             /**< (PIO_MDSR) Multi-drive Status Position */
#define PIO_MDSR_P28_Msk                    (0x1U << PIO_MDSR_P28_Pos)                     /**< (PIO_MDSR) Multi-drive Status Mask */
#define PIO_MDSR_P28                        PIO_MDSR_P28_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_MDSR_P28_Msk instead */
#define PIO_MDSR_P29_Pos                    29                                             /**< (PIO_MDSR) Multi-drive Status Position */
#define PIO_MDSR_P29_Msk                    (0x1U << PIO_MDSR_P29_Pos)                     /**< (PIO_MDSR) Multi-drive Status Mask */
#define PIO_MDSR_P29                        PIO_MDSR_P29_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_MDSR_P29_Msk instead */
#define PIO_MDSR_P30_Pos                    30                                             /**< (PIO_MDSR) Multi-drive Status Position */
#define PIO_MDSR_P30_Msk                    (0x1U << PIO_MDSR_P30_Pos)                     /**< (PIO_MDSR) Multi-drive Status Mask */
#define PIO_MDSR_P30                        PIO_MDSR_P30_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_MDSR_P30_Msk instead */
#define PIO_MDSR_P31_Pos                    31                                             /**< (PIO_MDSR) Multi-drive Status Position */
#define PIO_MDSR_P31_Msk                    (0x1U << PIO_MDSR_P31_Pos)                     /**< (PIO_MDSR) Multi-drive Status Mask */
#define PIO_MDSR_P31                        PIO_MDSR_P31_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_MDSR_P31_Msk instead */
#define PIO_MDSR_P_Pos                      0                                              /**< (PIO_MDSR Position) Multi-drive Status */
#define PIO_MDSR_P_Msk                      (0xFFFFFFFFU << PIO_MDSR_P_Pos)                /**< (PIO_MDSR Mask) P */
#define PIO_MDSR_P(value)                   (PIO_MDSR_P_Msk & ((value) << PIO_MDSR_P_Pos))  
#define PIO_MDSR_MASK                       (0xFFFFFFFFU)                                  /**< \deprecated (PIO_MDSR) Register MASK  (Use PIO_MDSR_Msk instead)  */
#define PIO_MDSR_Msk                        (0xFFFFFFFFU)                                  /**< (PIO_MDSR) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- PIO_PUDR : (PIO Offset: 0x60) (/W 32) Pull-up Disable Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t P0:1;                      /**< bit:      0  Pull-Up Disable                          */
    uint32_t P1:1;                      /**< bit:      1  Pull-Up Disable                          */
    uint32_t P2:1;                      /**< bit:      2  Pull-Up Disable                          */
    uint32_t P3:1;                      /**< bit:      3  Pull-Up Disable                          */
    uint32_t P4:1;                      /**< bit:      4  Pull-Up Disable                          */
    uint32_t P5:1;                      /**< bit:      5  Pull-Up Disable                          */
    uint32_t P6:1;                      /**< bit:      6  Pull-Up Disable                          */
    uint32_t P7:1;                      /**< bit:      7  Pull-Up Disable                          */
    uint32_t P8:1;                      /**< bit:      8  Pull-Up Disable                          */
    uint32_t P9:1;                      /**< bit:      9  Pull-Up Disable                          */
    uint32_t P10:1;                     /**< bit:     10  Pull-Up Disable                          */
    uint32_t P11:1;                     /**< bit:     11  Pull-Up Disable                          */
    uint32_t P12:1;                     /**< bit:     12  Pull-Up Disable                          */
    uint32_t P13:1;                     /**< bit:     13  Pull-Up Disable                          */
    uint32_t P14:1;                     /**< bit:     14  Pull-Up Disable                          */
    uint32_t P15:1;                     /**< bit:     15  Pull-Up Disable                          */
    uint32_t P16:1;                     /**< bit:     16  Pull-Up Disable                          */
    uint32_t P17:1;                     /**< bit:     17  Pull-Up Disable                          */
    uint32_t P18:1;                     /**< bit:     18  Pull-Up Disable                          */
    uint32_t P19:1;                     /**< bit:     19  Pull-Up Disable                          */
    uint32_t P20:1;                     /**< bit:     20  Pull-Up Disable                          */
    uint32_t P21:1;                     /**< bit:     21  Pull-Up Disable                          */
    uint32_t P22:1;                     /**< bit:     22  Pull-Up Disable                          */
    uint32_t P23:1;                     /**< bit:     23  Pull-Up Disable                          */
    uint32_t P24:1;                     /**< bit:     24  Pull-Up Disable                          */
    uint32_t P25:1;                     /**< bit:     25  Pull-Up Disable                          */
    uint32_t P26:1;                     /**< bit:     26  Pull-Up Disable                          */
    uint32_t P27:1;                     /**< bit:     27  Pull-Up Disable                          */
    uint32_t P28:1;                     /**< bit:     28  Pull-Up Disable                          */
    uint32_t P29:1;                     /**< bit:     29  Pull-Up Disable                          */
    uint32_t P30:1;                     /**< bit:     30  Pull-Up Disable                          */
    uint32_t P31:1;                     /**< bit:     31  Pull-Up Disable                          */
  } bit;                                /**< Structure used for bit  access */
  struct {
    uint32_t P:32;                      /**< bit:  0..31  Pull-Up Disable                          */
  } vec;                                /**< Structure used for vec  access  */
  uint32_t reg;                         /**< Type used for register access */
} PIO_PUDR_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define PIO_PUDR_OFFSET                     (0x60)                                        /**<  (PIO_PUDR) Pull-up Disable Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define PIO_PUDR_P0_Pos                     0                                              /**< (PIO_PUDR) Pull-Up Disable Position */
#define PIO_PUDR_P0_Msk                     (0x1U << PIO_PUDR_P0_Pos)                      /**< (PIO_PUDR) Pull-Up Disable Mask */
#define PIO_PUDR_P0                         PIO_PUDR_P0_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PUDR_P0_Msk instead */
#define PIO_PUDR_P1_Pos                     1                                              /**< (PIO_PUDR) Pull-Up Disable Position */
#define PIO_PUDR_P1_Msk                     (0x1U << PIO_PUDR_P1_Pos)                      /**< (PIO_PUDR) Pull-Up Disable Mask */
#define PIO_PUDR_P1                         PIO_PUDR_P1_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PUDR_P1_Msk instead */
#define PIO_PUDR_P2_Pos                     2                                              /**< (PIO_PUDR) Pull-Up Disable Position */
#define PIO_PUDR_P2_Msk                     (0x1U << PIO_PUDR_P2_Pos)                      /**< (PIO_PUDR) Pull-Up Disable Mask */
#define PIO_PUDR_P2                         PIO_PUDR_P2_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PUDR_P2_Msk instead */
#define PIO_PUDR_P3_Pos                     3                                              /**< (PIO_PUDR) Pull-Up Disable Position */
#define PIO_PUDR_P3_Msk                     (0x1U << PIO_PUDR_P3_Pos)                      /**< (PIO_PUDR) Pull-Up Disable Mask */
#define PIO_PUDR_P3                         PIO_PUDR_P3_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PUDR_P3_Msk instead */
#define PIO_PUDR_P4_Pos                     4                                              /**< (PIO_PUDR) Pull-Up Disable Position */
#define PIO_PUDR_P4_Msk                     (0x1U << PIO_PUDR_P4_Pos)                      /**< (PIO_PUDR) Pull-Up Disable Mask */
#define PIO_PUDR_P4                         PIO_PUDR_P4_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PUDR_P4_Msk instead */
#define PIO_PUDR_P5_Pos                     5                                              /**< (PIO_PUDR) Pull-Up Disable Position */
#define PIO_PUDR_P5_Msk                     (0x1U << PIO_PUDR_P5_Pos)                      /**< (PIO_PUDR) Pull-Up Disable Mask */
#define PIO_PUDR_P5                         PIO_PUDR_P5_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PUDR_P5_Msk instead */
#define PIO_PUDR_P6_Pos                     6                                              /**< (PIO_PUDR) Pull-Up Disable Position */
#define PIO_PUDR_P6_Msk                     (0x1U << PIO_PUDR_P6_Pos)                      /**< (PIO_PUDR) Pull-Up Disable Mask */
#define PIO_PUDR_P6                         PIO_PUDR_P6_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PUDR_P6_Msk instead */
#define PIO_PUDR_P7_Pos                     7                                              /**< (PIO_PUDR) Pull-Up Disable Position */
#define PIO_PUDR_P7_Msk                     (0x1U << PIO_PUDR_P7_Pos)                      /**< (PIO_PUDR) Pull-Up Disable Mask */
#define PIO_PUDR_P7                         PIO_PUDR_P7_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PUDR_P7_Msk instead */
#define PIO_PUDR_P8_Pos                     8                                              /**< (PIO_PUDR) Pull-Up Disable Position */
#define PIO_PUDR_P8_Msk                     (0x1U << PIO_PUDR_P8_Pos)                      /**< (PIO_PUDR) Pull-Up Disable Mask */
#define PIO_PUDR_P8                         PIO_PUDR_P8_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PUDR_P8_Msk instead */
#define PIO_PUDR_P9_Pos                     9                                              /**< (PIO_PUDR) Pull-Up Disable Position */
#define PIO_PUDR_P9_Msk                     (0x1U << PIO_PUDR_P9_Pos)                      /**< (PIO_PUDR) Pull-Up Disable Mask */
#define PIO_PUDR_P9                         PIO_PUDR_P9_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PUDR_P9_Msk instead */
#define PIO_PUDR_P10_Pos                    10                                             /**< (PIO_PUDR) Pull-Up Disable Position */
#define PIO_PUDR_P10_Msk                    (0x1U << PIO_PUDR_P10_Pos)                     /**< (PIO_PUDR) Pull-Up Disable Mask */
#define PIO_PUDR_P10                        PIO_PUDR_P10_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PUDR_P10_Msk instead */
#define PIO_PUDR_P11_Pos                    11                                             /**< (PIO_PUDR) Pull-Up Disable Position */
#define PIO_PUDR_P11_Msk                    (0x1U << PIO_PUDR_P11_Pos)                     /**< (PIO_PUDR) Pull-Up Disable Mask */
#define PIO_PUDR_P11                        PIO_PUDR_P11_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PUDR_P11_Msk instead */
#define PIO_PUDR_P12_Pos                    12                                             /**< (PIO_PUDR) Pull-Up Disable Position */
#define PIO_PUDR_P12_Msk                    (0x1U << PIO_PUDR_P12_Pos)                     /**< (PIO_PUDR) Pull-Up Disable Mask */
#define PIO_PUDR_P12                        PIO_PUDR_P12_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PUDR_P12_Msk instead */
#define PIO_PUDR_P13_Pos                    13                                             /**< (PIO_PUDR) Pull-Up Disable Position */
#define PIO_PUDR_P13_Msk                    (0x1U << PIO_PUDR_P13_Pos)                     /**< (PIO_PUDR) Pull-Up Disable Mask */
#define PIO_PUDR_P13                        PIO_PUDR_P13_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PUDR_P13_Msk instead */
#define PIO_PUDR_P14_Pos                    14                                             /**< (PIO_PUDR) Pull-Up Disable Position */
#define PIO_PUDR_P14_Msk                    (0x1U << PIO_PUDR_P14_Pos)                     /**< (PIO_PUDR) Pull-Up Disable Mask */
#define PIO_PUDR_P14                        PIO_PUDR_P14_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PUDR_P14_Msk instead */
#define PIO_PUDR_P15_Pos                    15                                             /**< (PIO_PUDR) Pull-Up Disable Position */
#define PIO_PUDR_P15_Msk                    (0x1U << PIO_PUDR_P15_Pos)                     /**< (PIO_PUDR) Pull-Up Disable Mask */
#define PIO_PUDR_P15                        PIO_PUDR_P15_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PUDR_P15_Msk instead */
#define PIO_PUDR_P16_Pos                    16                                             /**< (PIO_PUDR) Pull-Up Disable Position */
#define PIO_PUDR_P16_Msk                    (0x1U << PIO_PUDR_P16_Pos)                     /**< (PIO_PUDR) Pull-Up Disable Mask */
#define PIO_PUDR_P16                        PIO_PUDR_P16_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PUDR_P16_Msk instead */
#define PIO_PUDR_P17_Pos                    17                                             /**< (PIO_PUDR) Pull-Up Disable Position */
#define PIO_PUDR_P17_Msk                    (0x1U << PIO_PUDR_P17_Pos)                     /**< (PIO_PUDR) Pull-Up Disable Mask */
#define PIO_PUDR_P17                        PIO_PUDR_P17_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PUDR_P17_Msk instead */
#define PIO_PUDR_P18_Pos                    18                                             /**< (PIO_PUDR) Pull-Up Disable Position */
#define PIO_PUDR_P18_Msk                    (0x1U << PIO_PUDR_P18_Pos)                     /**< (PIO_PUDR) Pull-Up Disable Mask */
#define PIO_PUDR_P18                        PIO_PUDR_P18_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PUDR_P18_Msk instead */
#define PIO_PUDR_P19_Pos                    19                                             /**< (PIO_PUDR) Pull-Up Disable Position */
#define PIO_PUDR_P19_Msk                    (0x1U << PIO_PUDR_P19_Pos)                     /**< (PIO_PUDR) Pull-Up Disable Mask */
#define PIO_PUDR_P19                        PIO_PUDR_P19_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PUDR_P19_Msk instead */
#define PIO_PUDR_P20_Pos                    20                                             /**< (PIO_PUDR) Pull-Up Disable Position */
#define PIO_PUDR_P20_Msk                    (0x1U << PIO_PUDR_P20_Pos)                     /**< (PIO_PUDR) Pull-Up Disable Mask */
#define PIO_PUDR_P20                        PIO_PUDR_P20_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PUDR_P20_Msk instead */
#define PIO_PUDR_P21_Pos                    21                                             /**< (PIO_PUDR) Pull-Up Disable Position */
#define PIO_PUDR_P21_Msk                    (0x1U << PIO_PUDR_P21_Pos)                     /**< (PIO_PUDR) Pull-Up Disable Mask */
#define PIO_PUDR_P21                        PIO_PUDR_P21_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PUDR_P21_Msk instead */
#define PIO_PUDR_P22_Pos                    22                                             /**< (PIO_PUDR) Pull-Up Disable Position */
#define PIO_PUDR_P22_Msk                    (0x1U << PIO_PUDR_P22_Pos)                     /**< (PIO_PUDR) Pull-Up Disable Mask */
#define PIO_PUDR_P22                        PIO_PUDR_P22_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PUDR_P22_Msk instead */
#define PIO_PUDR_P23_Pos                    23                                             /**< (PIO_PUDR) Pull-Up Disable Position */
#define PIO_PUDR_P23_Msk                    (0x1U << PIO_PUDR_P23_Pos)                     /**< (PIO_PUDR) Pull-Up Disable Mask */
#define PIO_PUDR_P23                        PIO_PUDR_P23_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PUDR_P23_Msk instead */
#define PIO_PUDR_P24_Pos                    24                                             /**< (PIO_PUDR) Pull-Up Disable Position */
#define PIO_PUDR_P24_Msk                    (0x1U << PIO_PUDR_P24_Pos)                     /**< (PIO_PUDR) Pull-Up Disable Mask */
#define PIO_PUDR_P24                        PIO_PUDR_P24_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PUDR_P24_Msk instead */
#define PIO_PUDR_P25_Pos                    25                                             /**< (PIO_PUDR) Pull-Up Disable Position */
#define PIO_PUDR_P25_Msk                    (0x1U << PIO_PUDR_P25_Pos)                     /**< (PIO_PUDR) Pull-Up Disable Mask */
#define PIO_PUDR_P25                        PIO_PUDR_P25_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PUDR_P25_Msk instead */
#define PIO_PUDR_P26_Pos                    26                                             /**< (PIO_PUDR) Pull-Up Disable Position */
#define PIO_PUDR_P26_Msk                    (0x1U << PIO_PUDR_P26_Pos)                     /**< (PIO_PUDR) Pull-Up Disable Mask */
#define PIO_PUDR_P26                        PIO_PUDR_P26_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PUDR_P26_Msk instead */
#define PIO_PUDR_P27_Pos                    27                                             /**< (PIO_PUDR) Pull-Up Disable Position */
#define PIO_PUDR_P27_Msk                    (0x1U << PIO_PUDR_P27_Pos)                     /**< (PIO_PUDR) Pull-Up Disable Mask */
#define PIO_PUDR_P27                        PIO_PUDR_P27_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PUDR_P27_Msk instead */
#define PIO_PUDR_P28_Pos                    28                                             /**< (PIO_PUDR) Pull-Up Disable Position */
#define PIO_PUDR_P28_Msk                    (0x1U << PIO_PUDR_P28_Pos)                     /**< (PIO_PUDR) Pull-Up Disable Mask */
#define PIO_PUDR_P28                        PIO_PUDR_P28_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PUDR_P28_Msk instead */
#define PIO_PUDR_P29_Pos                    29                                             /**< (PIO_PUDR) Pull-Up Disable Position */
#define PIO_PUDR_P29_Msk                    (0x1U << PIO_PUDR_P29_Pos)                     /**< (PIO_PUDR) Pull-Up Disable Mask */
#define PIO_PUDR_P29                        PIO_PUDR_P29_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PUDR_P29_Msk instead */
#define PIO_PUDR_P30_Pos                    30                                             /**< (PIO_PUDR) Pull-Up Disable Position */
#define PIO_PUDR_P30_Msk                    (0x1U << PIO_PUDR_P30_Pos)                     /**< (PIO_PUDR) Pull-Up Disable Mask */
#define PIO_PUDR_P30                        PIO_PUDR_P30_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PUDR_P30_Msk instead */
#define PIO_PUDR_P31_Pos                    31                                             /**< (PIO_PUDR) Pull-Up Disable Position */
#define PIO_PUDR_P31_Msk                    (0x1U << PIO_PUDR_P31_Pos)                     /**< (PIO_PUDR) Pull-Up Disable Mask */
#define PIO_PUDR_P31                        PIO_PUDR_P31_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PUDR_P31_Msk instead */
#define PIO_PUDR_P_Pos                      0                                              /**< (PIO_PUDR Position) Pull-Up Disable */
#define PIO_PUDR_P_Msk                      (0xFFFFFFFFU << PIO_PUDR_P_Pos)                /**< (PIO_PUDR Mask) P */
#define PIO_PUDR_P(value)                   (PIO_PUDR_P_Msk & ((value) << PIO_PUDR_P_Pos))  
#define PIO_PUDR_MASK                       (0xFFFFFFFFU)                                  /**< \deprecated (PIO_PUDR) Register MASK  (Use PIO_PUDR_Msk instead)  */
#define PIO_PUDR_Msk                        (0xFFFFFFFFU)                                  /**< (PIO_PUDR) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- PIO_PUER : (PIO Offset: 0x64) (/W 32) Pull-up Enable Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t P0:1;                      /**< bit:      0  Pull-Up Enable                           */
    uint32_t P1:1;                      /**< bit:      1  Pull-Up Enable                           */
    uint32_t P2:1;                      /**< bit:      2  Pull-Up Enable                           */
    uint32_t P3:1;                      /**< bit:      3  Pull-Up Enable                           */
    uint32_t P4:1;                      /**< bit:      4  Pull-Up Enable                           */
    uint32_t P5:1;                      /**< bit:      5  Pull-Up Enable                           */
    uint32_t P6:1;                      /**< bit:      6  Pull-Up Enable                           */
    uint32_t P7:1;                      /**< bit:      7  Pull-Up Enable                           */
    uint32_t P8:1;                      /**< bit:      8  Pull-Up Enable                           */
    uint32_t P9:1;                      /**< bit:      9  Pull-Up Enable                           */
    uint32_t P10:1;                     /**< bit:     10  Pull-Up Enable                           */
    uint32_t P11:1;                     /**< bit:     11  Pull-Up Enable                           */
    uint32_t P12:1;                     /**< bit:     12  Pull-Up Enable                           */
    uint32_t P13:1;                     /**< bit:     13  Pull-Up Enable                           */
    uint32_t P14:1;                     /**< bit:     14  Pull-Up Enable                           */
    uint32_t P15:1;                     /**< bit:     15  Pull-Up Enable                           */
    uint32_t P16:1;                     /**< bit:     16  Pull-Up Enable                           */
    uint32_t P17:1;                     /**< bit:     17  Pull-Up Enable                           */
    uint32_t P18:1;                     /**< bit:     18  Pull-Up Enable                           */
    uint32_t P19:1;                     /**< bit:     19  Pull-Up Enable                           */
    uint32_t P20:1;                     /**< bit:     20  Pull-Up Enable                           */
    uint32_t P21:1;                     /**< bit:     21  Pull-Up Enable                           */
    uint32_t P22:1;                     /**< bit:     22  Pull-Up Enable                           */
    uint32_t P23:1;                     /**< bit:     23  Pull-Up Enable                           */
    uint32_t P24:1;                     /**< bit:     24  Pull-Up Enable                           */
    uint32_t P25:1;                     /**< bit:     25  Pull-Up Enable                           */
    uint32_t P26:1;                     /**< bit:     26  Pull-Up Enable                           */
    uint32_t P27:1;                     /**< bit:     27  Pull-Up Enable                           */
    uint32_t P28:1;                     /**< bit:     28  Pull-Up Enable                           */
    uint32_t P29:1;                     /**< bit:     29  Pull-Up Enable                           */
    uint32_t P30:1;                     /**< bit:     30  Pull-Up Enable                           */
    uint32_t P31:1;                     /**< bit:     31  Pull-Up Enable                           */
  } bit;                                /**< Structure used for bit  access */
  struct {
    uint32_t P:32;                      /**< bit:  0..31  Pull-Up Enable                           */
  } vec;                                /**< Structure used for vec  access  */
  uint32_t reg;                         /**< Type used for register access */
} PIO_PUER_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define PIO_PUER_OFFSET                     (0x64)                                        /**<  (PIO_PUER) Pull-up Enable Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define PIO_PUER_P0_Pos                     0                                              /**< (PIO_PUER) Pull-Up Enable Position */
#define PIO_PUER_P0_Msk                     (0x1U << PIO_PUER_P0_Pos)                      /**< (PIO_PUER) Pull-Up Enable Mask */
#define PIO_PUER_P0                         PIO_PUER_P0_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PUER_P0_Msk instead */
#define PIO_PUER_P1_Pos                     1                                              /**< (PIO_PUER) Pull-Up Enable Position */
#define PIO_PUER_P1_Msk                     (0x1U << PIO_PUER_P1_Pos)                      /**< (PIO_PUER) Pull-Up Enable Mask */
#define PIO_PUER_P1                         PIO_PUER_P1_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PUER_P1_Msk instead */
#define PIO_PUER_P2_Pos                     2                                              /**< (PIO_PUER) Pull-Up Enable Position */
#define PIO_PUER_P2_Msk                     (0x1U << PIO_PUER_P2_Pos)                      /**< (PIO_PUER) Pull-Up Enable Mask */
#define PIO_PUER_P2                         PIO_PUER_P2_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PUER_P2_Msk instead */
#define PIO_PUER_P3_Pos                     3                                              /**< (PIO_PUER) Pull-Up Enable Position */
#define PIO_PUER_P3_Msk                     (0x1U << PIO_PUER_P3_Pos)                      /**< (PIO_PUER) Pull-Up Enable Mask */
#define PIO_PUER_P3                         PIO_PUER_P3_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PUER_P3_Msk instead */
#define PIO_PUER_P4_Pos                     4                                              /**< (PIO_PUER) Pull-Up Enable Position */
#define PIO_PUER_P4_Msk                     (0x1U << PIO_PUER_P4_Pos)                      /**< (PIO_PUER) Pull-Up Enable Mask */
#define PIO_PUER_P4                         PIO_PUER_P4_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PUER_P4_Msk instead */
#define PIO_PUER_P5_Pos                     5                                              /**< (PIO_PUER) Pull-Up Enable Position */
#define PIO_PUER_P5_Msk                     (0x1U << PIO_PUER_P5_Pos)                      /**< (PIO_PUER) Pull-Up Enable Mask */
#define PIO_PUER_P5                         PIO_PUER_P5_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PUER_P5_Msk instead */
#define PIO_PUER_P6_Pos                     6                                              /**< (PIO_PUER) Pull-Up Enable Position */
#define PIO_PUER_P6_Msk                     (0x1U << PIO_PUER_P6_Pos)                      /**< (PIO_PUER) Pull-Up Enable Mask */
#define PIO_PUER_P6                         PIO_PUER_P6_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PUER_P6_Msk instead */
#define PIO_PUER_P7_Pos                     7                                              /**< (PIO_PUER) Pull-Up Enable Position */
#define PIO_PUER_P7_Msk                     (0x1U << PIO_PUER_P7_Pos)                      /**< (PIO_PUER) Pull-Up Enable Mask */
#define PIO_PUER_P7                         PIO_PUER_P7_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PUER_P7_Msk instead */
#define PIO_PUER_P8_Pos                     8                                              /**< (PIO_PUER) Pull-Up Enable Position */
#define PIO_PUER_P8_Msk                     (0x1U << PIO_PUER_P8_Pos)                      /**< (PIO_PUER) Pull-Up Enable Mask */
#define PIO_PUER_P8                         PIO_PUER_P8_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PUER_P8_Msk instead */
#define PIO_PUER_P9_Pos                     9                                              /**< (PIO_PUER) Pull-Up Enable Position */
#define PIO_PUER_P9_Msk                     (0x1U << PIO_PUER_P9_Pos)                      /**< (PIO_PUER) Pull-Up Enable Mask */
#define PIO_PUER_P9                         PIO_PUER_P9_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PUER_P9_Msk instead */
#define PIO_PUER_P10_Pos                    10                                             /**< (PIO_PUER) Pull-Up Enable Position */
#define PIO_PUER_P10_Msk                    (0x1U << PIO_PUER_P10_Pos)                     /**< (PIO_PUER) Pull-Up Enable Mask */
#define PIO_PUER_P10                        PIO_PUER_P10_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PUER_P10_Msk instead */
#define PIO_PUER_P11_Pos                    11                                             /**< (PIO_PUER) Pull-Up Enable Position */
#define PIO_PUER_P11_Msk                    (0x1U << PIO_PUER_P11_Pos)                     /**< (PIO_PUER) Pull-Up Enable Mask */
#define PIO_PUER_P11                        PIO_PUER_P11_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PUER_P11_Msk instead */
#define PIO_PUER_P12_Pos                    12                                             /**< (PIO_PUER) Pull-Up Enable Position */
#define PIO_PUER_P12_Msk                    (0x1U << PIO_PUER_P12_Pos)                     /**< (PIO_PUER) Pull-Up Enable Mask */
#define PIO_PUER_P12                        PIO_PUER_P12_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PUER_P12_Msk instead */
#define PIO_PUER_P13_Pos                    13                                             /**< (PIO_PUER) Pull-Up Enable Position */
#define PIO_PUER_P13_Msk                    (0x1U << PIO_PUER_P13_Pos)                     /**< (PIO_PUER) Pull-Up Enable Mask */
#define PIO_PUER_P13                        PIO_PUER_P13_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PUER_P13_Msk instead */
#define PIO_PUER_P14_Pos                    14                                             /**< (PIO_PUER) Pull-Up Enable Position */
#define PIO_PUER_P14_Msk                    (0x1U << PIO_PUER_P14_Pos)                     /**< (PIO_PUER) Pull-Up Enable Mask */
#define PIO_PUER_P14                        PIO_PUER_P14_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PUER_P14_Msk instead */
#define PIO_PUER_P15_Pos                    15                                             /**< (PIO_PUER) Pull-Up Enable Position */
#define PIO_PUER_P15_Msk                    (0x1U << PIO_PUER_P15_Pos)                     /**< (PIO_PUER) Pull-Up Enable Mask */
#define PIO_PUER_P15                        PIO_PUER_P15_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PUER_P15_Msk instead */
#define PIO_PUER_P16_Pos                    16                                             /**< (PIO_PUER) Pull-Up Enable Position */
#define PIO_PUER_P16_Msk                    (0x1U << PIO_PUER_P16_Pos)                     /**< (PIO_PUER) Pull-Up Enable Mask */
#define PIO_PUER_P16                        PIO_PUER_P16_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PUER_P16_Msk instead */
#define PIO_PUER_P17_Pos                    17                                             /**< (PIO_PUER) Pull-Up Enable Position */
#define PIO_PUER_P17_Msk                    (0x1U << PIO_PUER_P17_Pos)                     /**< (PIO_PUER) Pull-Up Enable Mask */
#define PIO_PUER_P17                        PIO_PUER_P17_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PUER_P17_Msk instead */
#define PIO_PUER_P18_Pos                    18                                             /**< (PIO_PUER) Pull-Up Enable Position */
#define PIO_PUER_P18_Msk                    (0x1U << PIO_PUER_P18_Pos)                     /**< (PIO_PUER) Pull-Up Enable Mask */
#define PIO_PUER_P18                        PIO_PUER_P18_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PUER_P18_Msk instead */
#define PIO_PUER_P19_Pos                    19                                             /**< (PIO_PUER) Pull-Up Enable Position */
#define PIO_PUER_P19_Msk                    (0x1U << PIO_PUER_P19_Pos)                     /**< (PIO_PUER) Pull-Up Enable Mask */
#define PIO_PUER_P19                        PIO_PUER_P19_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PUER_P19_Msk instead */
#define PIO_PUER_P20_Pos                    20                                             /**< (PIO_PUER) Pull-Up Enable Position */
#define PIO_PUER_P20_Msk                    (0x1U << PIO_PUER_P20_Pos)                     /**< (PIO_PUER) Pull-Up Enable Mask */
#define PIO_PUER_P20                        PIO_PUER_P20_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PUER_P20_Msk instead */
#define PIO_PUER_P21_Pos                    21                                             /**< (PIO_PUER) Pull-Up Enable Position */
#define PIO_PUER_P21_Msk                    (0x1U << PIO_PUER_P21_Pos)                     /**< (PIO_PUER) Pull-Up Enable Mask */
#define PIO_PUER_P21                        PIO_PUER_P21_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PUER_P21_Msk instead */
#define PIO_PUER_P22_Pos                    22                                             /**< (PIO_PUER) Pull-Up Enable Position */
#define PIO_PUER_P22_Msk                    (0x1U << PIO_PUER_P22_Pos)                     /**< (PIO_PUER) Pull-Up Enable Mask */
#define PIO_PUER_P22                        PIO_PUER_P22_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PUER_P22_Msk instead */
#define PIO_PUER_P23_Pos                    23                                             /**< (PIO_PUER) Pull-Up Enable Position */
#define PIO_PUER_P23_Msk                    (0x1U << PIO_PUER_P23_Pos)                     /**< (PIO_PUER) Pull-Up Enable Mask */
#define PIO_PUER_P23                        PIO_PUER_P23_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PUER_P23_Msk instead */
#define PIO_PUER_P24_Pos                    24                                             /**< (PIO_PUER) Pull-Up Enable Position */
#define PIO_PUER_P24_Msk                    (0x1U << PIO_PUER_P24_Pos)                     /**< (PIO_PUER) Pull-Up Enable Mask */
#define PIO_PUER_P24                        PIO_PUER_P24_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PUER_P24_Msk instead */
#define PIO_PUER_P25_Pos                    25                                             /**< (PIO_PUER) Pull-Up Enable Position */
#define PIO_PUER_P25_Msk                    (0x1U << PIO_PUER_P25_Pos)                     /**< (PIO_PUER) Pull-Up Enable Mask */
#define PIO_PUER_P25                        PIO_PUER_P25_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PUER_P25_Msk instead */
#define PIO_PUER_P26_Pos                    26                                             /**< (PIO_PUER) Pull-Up Enable Position */
#define PIO_PUER_P26_Msk                    (0x1U << PIO_PUER_P26_Pos)                     /**< (PIO_PUER) Pull-Up Enable Mask */
#define PIO_PUER_P26                        PIO_PUER_P26_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PUER_P26_Msk instead */
#define PIO_PUER_P27_Pos                    27                                             /**< (PIO_PUER) Pull-Up Enable Position */
#define PIO_PUER_P27_Msk                    (0x1U << PIO_PUER_P27_Pos)                     /**< (PIO_PUER) Pull-Up Enable Mask */
#define PIO_PUER_P27                        PIO_PUER_P27_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PUER_P27_Msk instead */
#define PIO_PUER_P28_Pos                    28                                             /**< (PIO_PUER) Pull-Up Enable Position */
#define PIO_PUER_P28_Msk                    (0x1U << PIO_PUER_P28_Pos)                     /**< (PIO_PUER) Pull-Up Enable Mask */
#define PIO_PUER_P28                        PIO_PUER_P28_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PUER_P28_Msk instead */
#define PIO_PUER_P29_Pos                    29                                             /**< (PIO_PUER) Pull-Up Enable Position */
#define PIO_PUER_P29_Msk                    (0x1U << PIO_PUER_P29_Pos)                     /**< (PIO_PUER) Pull-Up Enable Mask */
#define PIO_PUER_P29                        PIO_PUER_P29_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PUER_P29_Msk instead */
#define PIO_PUER_P30_Pos                    30                                             /**< (PIO_PUER) Pull-Up Enable Position */
#define PIO_PUER_P30_Msk                    (0x1U << PIO_PUER_P30_Pos)                     /**< (PIO_PUER) Pull-Up Enable Mask */
#define PIO_PUER_P30                        PIO_PUER_P30_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PUER_P30_Msk instead */
#define PIO_PUER_P31_Pos                    31                                             /**< (PIO_PUER) Pull-Up Enable Position */
#define PIO_PUER_P31_Msk                    (0x1U << PIO_PUER_P31_Pos)                     /**< (PIO_PUER) Pull-Up Enable Mask */
#define PIO_PUER_P31                        PIO_PUER_P31_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PUER_P31_Msk instead */
#define PIO_PUER_P_Pos                      0                                              /**< (PIO_PUER Position) Pull-Up Enable */
#define PIO_PUER_P_Msk                      (0xFFFFFFFFU << PIO_PUER_P_Pos)                /**< (PIO_PUER Mask) P */
#define PIO_PUER_P(value)                   (PIO_PUER_P_Msk & ((value) << PIO_PUER_P_Pos))  
#define PIO_PUER_MASK                       (0xFFFFFFFFU)                                  /**< \deprecated (PIO_PUER) Register MASK  (Use PIO_PUER_Msk instead)  */
#define PIO_PUER_Msk                        (0xFFFFFFFFU)                                  /**< (PIO_PUER) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- PIO_PUSR : (PIO Offset: 0x68) (R/ 32) Pad Pull-up Status Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t P0:1;                      /**< bit:      0  Pull-Up Status                           */
    uint32_t P1:1;                      /**< bit:      1  Pull-Up Status                           */
    uint32_t P2:1;                      /**< bit:      2  Pull-Up Status                           */
    uint32_t P3:1;                      /**< bit:      3  Pull-Up Status                           */
    uint32_t P4:1;                      /**< bit:      4  Pull-Up Status                           */
    uint32_t P5:1;                      /**< bit:      5  Pull-Up Status                           */
    uint32_t P6:1;                      /**< bit:      6  Pull-Up Status                           */
    uint32_t P7:1;                      /**< bit:      7  Pull-Up Status                           */
    uint32_t P8:1;                      /**< bit:      8  Pull-Up Status                           */
    uint32_t P9:1;                      /**< bit:      9  Pull-Up Status                           */
    uint32_t P10:1;                     /**< bit:     10  Pull-Up Status                           */
    uint32_t P11:1;                     /**< bit:     11  Pull-Up Status                           */
    uint32_t P12:1;                     /**< bit:     12  Pull-Up Status                           */
    uint32_t P13:1;                     /**< bit:     13  Pull-Up Status                           */
    uint32_t P14:1;                     /**< bit:     14  Pull-Up Status                           */
    uint32_t P15:1;                     /**< bit:     15  Pull-Up Status                           */
    uint32_t P16:1;                     /**< bit:     16  Pull-Up Status                           */
    uint32_t P17:1;                     /**< bit:     17  Pull-Up Status                           */
    uint32_t P18:1;                     /**< bit:     18  Pull-Up Status                           */
    uint32_t P19:1;                     /**< bit:     19  Pull-Up Status                           */
    uint32_t P20:1;                     /**< bit:     20  Pull-Up Status                           */
    uint32_t P21:1;                     /**< bit:     21  Pull-Up Status                           */
    uint32_t P22:1;                     /**< bit:     22  Pull-Up Status                           */
    uint32_t P23:1;                     /**< bit:     23  Pull-Up Status                           */
    uint32_t P24:1;                     /**< bit:     24  Pull-Up Status                           */
    uint32_t P25:1;                     /**< bit:     25  Pull-Up Status                           */
    uint32_t P26:1;                     /**< bit:     26  Pull-Up Status                           */
    uint32_t P27:1;                     /**< bit:     27  Pull-Up Status                           */
    uint32_t P28:1;                     /**< bit:     28  Pull-Up Status                           */
    uint32_t P29:1;                     /**< bit:     29  Pull-Up Status                           */
    uint32_t P30:1;                     /**< bit:     30  Pull-Up Status                           */
    uint32_t P31:1;                     /**< bit:     31  Pull-Up Status                           */
  } bit;                                /**< Structure used for bit  access */
  struct {
    uint32_t P:32;                      /**< bit:  0..31  Pull-Up Status                           */
  } vec;                                /**< Structure used for vec  access  */
  uint32_t reg;                         /**< Type used for register access */
} PIO_PUSR_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define PIO_PUSR_OFFSET                     (0x68)                                        /**<  (PIO_PUSR) Pad Pull-up Status Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define PIO_PUSR_P0_Pos                     0                                              /**< (PIO_PUSR) Pull-Up Status Position */
#define PIO_PUSR_P0_Msk                     (0x1U << PIO_PUSR_P0_Pos)                      /**< (PIO_PUSR) Pull-Up Status Mask */
#define PIO_PUSR_P0                         PIO_PUSR_P0_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PUSR_P0_Msk instead */
#define PIO_PUSR_P1_Pos                     1                                              /**< (PIO_PUSR) Pull-Up Status Position */
#define PIO_PUSR_P1_Msk                     (0x1U << PIO_PUSR_P1_Pos)                      /**< (PIO_PUSR) Pull-Up Status Mask */
#define PIO_PUSR_P1                         PIO_PUSR_P1_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PUSR_P1_Msk instead */
#define PIO_PUSR_P2_Pos                     2                                              /**< (PIO_PUSR) Pull-Up Status Position */
#define PIO_PUSR_P2_Msk                     (0x1U << PIO_PUSR_P2_Pos)                      /**< (PIO_PUSR) Pull-Up Status Mask */
#define PIO_PUSR_P2                         PIO_PUSR_P2_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PUSR_P2_Msk instead */
#define PIO_PUSR_P3_Pos                     3                                              /**< (PIO_PUSR) Pull-Up Status Position */
#define PIO_PUSR_P3_Msk                     (0x1U << PIO_PUSR_P3_Pos)                      /**< (PIO_PUSR) Pull-Up Status Mask */
#define PIO_PUSR_P3                         PIO_PUSR_P3_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PUSR_P3_Msk instead */
#define PIO_PUSR_P4_Pos                     4                                              /**< (PIO_PUSR) Pull-Up Status Position */
#define PIO_PUSR_P4_Msk                     (0x1U << PIO_PUSR_P4_Pos)                      /**< (PIO_PUSR) Pull-Up Status Mask */
#define PIO_PUSR_P4                         PIO_PUSR_P4_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PUSR_P4_Msk instead */
#define PIO_PUSR_P5_Pos                     5                                              /**< (PIO_PUSR) Pull-Up Status Position */
#define PIO_PUSR_P5_Msk                     (0x1U << PIO_PUSR_P5_Pos)                      /**< (PIO_PUSR) Pull-Up Status Mask */
#define PIO_PUSR_P5                         PIO_PUSR_P5_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PUSR_P5_Msk instead */
#define PIO_PUSR_P6_Pos                     6                                              /**< (PIO_PUSR) Pull-Up Status Position */
#define PIO_PUSR_P6_Msk                     (0x1U << PIO_PUSR_P6_Pos)                      /**< (PIO_PUSR) Pull-Up Status Mask */
#define PIO_PUSR_P6                         PIO_PUSR_P6_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PUSR_P6_Msk instead */
#define PIO_PUSR_P7_Pos                     7                                              /**< (PIO_PUSR) Pull-Up Status Position */
#define PIO_PUSR_P7_Msk                     (0x1U << PIO_PUSR_P7_Pos)                      /**< (PIO_PUSR) Pull-Up Status Mask */
#define PIO_PUSR_P7                         PIO_PUSR_P7_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PUSR_P7_Msk instead */
#define PIO_PUSR_P8_Pos                     8                                              /**< (PIO_PUSR) Pull-Up Status Position */
#define PIO_PUSR_P8_Msk                     (0x1U << PIO_PUSR_P8_Pos)                      /**< (PIO_PUSR) Pull-Up Status Mask */
#define PIO_PUSR_P8                         PIO_PUSR_P8_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PUSR_P8_Msk instead */
#define PIO_PUSR_P9_Pos                     9                                              /**< (PIO_PUSR) Pull-Up Status Position */
#define PIO_PUSR_P9_Msk                     (0x1U << PIO_PUSR_P9_Pos)                      /**< (PIO_PUSR) Pull-Up Status Mask */
#define PIO_PUSR_P9                         PIO_PUSR_P9_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PUSR_P9_Msk instead */
#define PIO_PUSR_P10_Pos                    10                                             /**< (PIO_PUSR) Pull-Up Status Position */
#define PIO_PUSR_P10_Msk                    (0x1U << PIO_PUSR_P10_Pos)                     /**< (PIO_PUSR) Pull-Up Status Mask */
#define PIO_PUSR_P10                        PIO_PUSR_P10_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PUSR_P10_Msk instead */
#define PIO_PUSR_P11_Pos                    11                                             /**< (PIO_PUSR) Pull-Up Status Position */
#define PIO_PUSR_P11_Msk                    (0x1U << PIO_PUSR_P11_Pos)                     /**< (PIO_PUSR) Pull-Up Status Mask */
#define PIO_PUSR_P11                        PIO_PUSR_P11_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PUSR_P11_Msk instead */
#define PIO_PUSR_P12_Pos                    12                                             /**< (PIO_PUSR) Pull-Up Status Position */
#define PIO_PUSR_P12_Msk                    (0x1U << PIO_PUSR_P12_Pos)                     /**< (PIO_PUSR) Pull-Up Status Mask */
#define PIO_PUSR_P12                        PIO_PUSR_P12_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PUSR_P12_Msk instead */
#define PIO_PUSR_P13_Pos                    13                                             /**< (PIO_PUSR) Pull-Up Status Position */
#define PIO_PUSR_P13_Msk                    (0x1U << PIO_PUSR_P13_Pos)                     /**< (PIO_PUSR) Pull-Up Status Mask */
#define PIO_PUSR_P13                        PIO_PUSR_P13_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PUSR_P13_Msk instead */
#define PIO_PUSR_P14_Pos                    14                                             /**< (PIO_PUSR) Pull-Up Status Position */
#define PIO_PUSR_P14_Msk                    (0x1U << PIO_PUSR_P14_Pos)                     /**< (PIO_PUSR) Pull-Up Status Mask */
#define PIO_PUSR_P14                        PIO_PUSR_P14_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PUSR_P14_Msk instead */
#define PIO_PUSR_P15_Pos                    15                                             /**< (PIO_PUSR) Pull-Up Status Position */
#define PIO_PUSR_P15_Msk                    (0x1U << PIO_PUSR_P15_Pos)                     /**< (PIO_PUSR) Pull-Up Status Mask */
#define PIO_PUSR_P15                        PIO_PUSR_P15_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PUSR_P15_Msk instead */
#define PIO_PUSR_P16_Pos                    16                                             /**< (PIO_PUSR) Pull-Up Status Position */
#define PIO_PUSR_P16_Msk                    (0x1U << PIO_PUSR_P16_Pos)                     /**< (PIO_PUSR) Pull-Up Status Mask */
#define PIO_PUSR_P16                        PIO_PUSR_P16_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PUSR_P16_Msk instead */
#define PIO_PUSR_P17_Pos                    17                                             /**< (PIO_PUSR) Pull-Up Status Position */
#define PIO_PUSR_P17_Msk                    (0x1U << PIO_PUSR_P17_Pos)                     /**< (PIO_PUSR) Pull-Up Status Mask */
#define PIO_PUSR_P17                        PIO_PUSR_P17_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PUSR_P17_Msk instead */
#define PIO_PUSR_P18_Pos                    18                                             /**< (PIO_PUSR) Pull-Up Status Position */
#define PIO_PUSR_P18_Msk                    (0x1U << PIO_PUSR_P18_Pos)                     /**< (PIO_PUSR) Pull-Up Status Mask */
#define PIO_PUSR_P18                        PIO_PUSR_P18_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PUSR_P18_Msk instead */
#define PIO_PUSR_P19_Pos                    19                                             /**< (PIO_PUSR) Pull-Up Status Position */
#define PIO_PUSR_P19_Msk                    (0x1U << PIO_PUSR_P19_Pos)                     /**< (PIO_PUSR) Pull-Up Status Mask */
#define PIO_PUSR_P19                        PIO_PUSR_P19_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PUSR_P19_Msk instead */
#define PIO_PUSR_P20_Pos                    20                                             /**< (PIO_PUSR) Pull-Up Status Position */
#define PIO_PUSR_P20_Msk                    (0x1U << PIO_PUSR_P20_Pos)                     /**< (PIO_PUSR) Pull-Up Status Mask */
#define PIO_PUSR_P20                        PIO_PUSR_P20_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PUSR_P20_Msk instead */
#define PIO_PUSR_P21_Pos                    21                                             /**< (PIO_PUSR) Pull-Up Status Position */
#define PIO_PUSR_P21_Msk                    (0x1U << PIO_PUSR_P21_Pos)                     /**< (PIO_PUSR) Pull-Up Status Mask */
#define PIO_PUSR_P21                        PIO_PUSR_P21_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PUSR_P21_Msk instead */
#define PIO_PUSR_P22_Pos                    22                                             /**< (PIO_PUSR) Pull-Up Status Position */
#define PIO_PUSR_P22_Msk                    (0x1U << PIO_PUSR_P22_Pos)                     /**< (PIO_PUSR) Pull-Up Status Mask */
#define PIO_PUSR_P22                        PIO_PUSR_P22_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PUSR_P22_Msk instead */
#define PIO_PUSR_P23_Pos                    23                                             /**< (PIO_PUSR) Pull-Up Status Position */
#define PIO_PUSR_P23_Msk                    (0x1U << PIO_PUSR_P23_Pos)                     /**< (PIO_PUSR) Pull-Up Status Mask */
#define PIO_PUSR_P23                        PIO_PUSR_P23_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PUSR_P23_Msk instead */
#define PIO_PUSR_P24_Pos                    24                                             /**< (PIO_PUSR) Pull-Up Status Position */
#define PIO_PUSR_P24_Msk                    (0x1U << PIO_PUSR_P24_Pos)                     /**< (PIO_PUSR) Pull-Up Status Mask */
#define PIO_PUSR_P24                        PIO_PUSR_P24_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PUSR_P24_Msk instead */
#define PIO_PUSR_P25_Pos                    25                                             /**< (PIO_PUSR) Pull-Up Status Position */
#define PIO_PUSR_P25_Msk                    (0x1U << PIO_PUSR_P25_Pos)                     /**< (PIO_PUSR) Pull-Up Status Mask */
#define PIO_PUSR_P25                        PIO_PUSR_P25_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PUSR_P25_Msk instead */
#define PIO_PUSR_P26_Pos                    26                                             /**< (PIO_PUSR) Pull-Up Status Position */
#define PIO_PUSR_P26_Msk                    (0x1U << PIO_PUSR_P26_Pos)                     /**< (PIO_PUSR) Pull-Up Status Mask */
#define PIO_PUSR_P26                        PIO_PUSR_P26_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PUSR_P26_Msk instead */
#define PIO_PUSR_P27_Pos                    27                                             /**< (PIO_PUSR) Pull-Up Status Position */
#define PIO_PUSR_P27_Msk                    (0x1U << PIO_PUSR_P27_Pos)                     /**< (PIO_PUSR) Pull-Up Status Mask */
#define PIO_PUSR_P27                        PIO_PUSR_P27_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PUSR_P27_Msk instead */
#define PIO_PUSR_P28_Pos                    28                                             /**< (PIO_PUSR) Pull-Up Status Position */
#define PIO_PUSR_P28_Msk                    (0x1U << PIO_PUSR_P28_Pos)                     /**< (PIO_PUSR) Pull-Up Status Mask */
#define PIO_PUSR_P28                        PIO_PUSR_P28_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PUSR_P28_Msk instead */
#define PIO_PUSR_P29_Pos                    29                                             /**< (PIO_PUSR) Pull-Up Status Position */
#define PIO_PUSR_P29_Msk                    (0x1U << PIO_PUSR_P29_Pos)                     /**< (PIO_PUSR) Pull-Up Status Mask */
#define PIO_PUSR_P29                        PIO_PUSR_P29_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PUSR_P29_Msk instead */
#define PIO_PUSR_P30_Pos                    30                                             /**< (PIO_PUSR) Pull-Up Status Position */
#define PIO_PUSR_P30_Msk                    (0x1U << PIO_PUSR_P30_Pos)                     /**< (PIO_PUSR) Pull-Up Status Mask */
#define PIO_PUSR_P30                        PIO_PUSR_P30_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PUSR_P30_Msk instead */
#define PIO_PUSR_P31_Pos                    31                                             /**< (PIO_PUSR) Pull-Up Status Position */
#define PIO_PUSR_P31_Msk                    (0x1U << PIO_PUSR_P31_Pos)                     /**< (PIO_PUSR) Pull-Up Status Mask */
#define PIO_PUSR_P31                        PIO_PUSR_P31_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PUSR_P31_Msk instead */
#define PIO_PUSR_P_Pos                      0                                              /**< (PIO_PUSR Position) Pull-Up Status */
#define PIO_PUSR_P_Msk                      (0xFFFFFFFFU << PIO_PUSR_P_Pos)                /**< (PIO_PUSR Mask) P */
#define PIO_PUSR_P(value)                   (PIO_PUSR_P_Msk & ((value) << PIO_PUSR_P_Pos))  
#define PIO_PUSR_MASK                       (0xFFFFFFFFU)                                  /**< \deprecated (PIO_PUSR) Register MASK  (Use PIO_PUSR_Msk instead)  */
#define PIO_PUSR_Msk                        (0xFFFFFFFFU)                                  /**< (PIO_PUSR) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- PIO_ABCDSR : (PIO Offset: 0x70) (R/W 32) Peripheral Select Register 0 -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t P0:1;                      /**< bit:      0  Peripheral Select                        */
    uint32_t P1:1;                      /**< bit:      1  Peripheral Select                        */
    uint32_t P2:1;                      /**< bit:      2  Peripheral Select                        */
    uint32_t P3:1;                      /**< bit:      3  Peripheral Select                        */
    uint32_t P4:1;                      /**< bit:      4  Peripheral Select                        */
    uint32_t P5:1;                      /**< bit:      5  Peripheral Select                        */
    uint32_t P6:1;                      /**< bit:      6  Peripheral Select                        */
    uint32_t P7:1;                      /**< bit:      7  Peripheral Select                        */
    uint32_t P8:1;                      /**< bit:      8  Peripheral Select                        */
    uint32_t P9:1;                      /**< bit:      9  Peripheral Select                        */
    uint32_t P10:1;                     /**< bit:     10  Peripheral Select                        */
    uint32_t P11:1;                     /**< bit:     11  Peripheral Select                        */
    uint32_t P12:1;                     /**< bit:     12  Peripheral Select                        */
    uint32_t P13:1;                     /**< bit:     13  Peripheral Select                        */
    uint32_t P14:1;                     /**< bit:     14  Peripheral Select                        */
    uint32_t P15:1;                     /**< bit:     15  Peripheral Select                        */
    uint32_t P16:1;                     /**< bit:     16  Peripheral Select                        */
    uint32_t P17:1;                     /**< bit:     17  Peripheral Select                        */
    uint32_t P18:1;                     /**< bit:     18  Peripheral Select                        */
    uint32_t P19:1;                     /**< bit:     19  Peripheral Select                        */
    uint32_t P20:1;                     /**< bit:     20  Peripheral Select                        */
    uint32_t P21:1;                     /**< bit:     21  Peripheral Select                        */
    uint32_t P22:1;                     /**< bit:     22  Peripheral Select                        */
    uint32_t P23:1;                     /**< bit:     23  Peripheral Select                        */
    uint32_t P24:1;                     /**< bit:     24  Peripheral Select                        */
    uint32_t P25:1;                     /**< bit:     25  Peripheral Select                        */
    uint32_t P26:1;                     /**< bit:     26  Peripheral Select                        */
    uint32_t P27:1;                     /**< bit:     27  Peripheral Select                        */
    uint32_t P28:1;                     /**< bit:     28  Peripheral Select                        */
    uint32_t P29:1;                     /**< bit:     29  Peripheral Select                        */
    uint32_t P30:1;                     /**< bit:     30  Peripheral Select                        */
    uint32_t P31:1;                     /**< bit:     31  Peripheral Select                        */
  } bit;                                /**< Structure used for bit  access */
  struct {
    uint32_t P:32;                      /**< bit:  0..31  Peripheral Select                        */
  } vec;                                /**< Structure used for vec  access  */
  uint32_t reg;                         /**< Type used for register access */
} PIO_ABCDSR_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define PIO_ABCDSR_OFFSET                   (0x70)                                        /**<  (PIO_ABCDSR) Peripheral Select Register 0  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define PIO_ABCDSR_P0_Pos                   0                                              /**< (PIO_ABCDSR) Peripheral Select Position */
#define PIO_ABCDSR_P0_Msk                   (0x1U << PIO_ABCDSR_P0_Pos)                    /**< (PIO_ABCDSR) Peripheral Select Mask */
#define PIO_ABCDSR_P0                       PIO_ABCDSR_P0_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_ABCDSR_P0_Msk instead */
#define PIO_ABCDSR_P1_Pos                   1                                              /**< (PIO_ABCDSR) Peripheral Select Position */
#define PIO_ABCDSR_P1_Msk                   (0x1U << PIO_ABCDSR_P1_Pos)                    /**< (PIO_ABCDSR) Peripheral Select Mask */
#define PIO_ABCDSR_P1                       PIO_ABCDSR_P1_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_ABCDSR_P1_Msk instead */
#define PIO_ABCDSR_P2_Pos                   2                                              /**< (PIO_ABCDSR) Peripheral Select Position */
#define PIO_ABCDSR_P2_Msk                   (0x1U << PIO_ABCDSR_P2_Pos)                    /**< (PIO_ABCDSR) Peripheral Select Mask */
#define PIO_ABCDSR_P2                       PIO_ABCDSR_P2_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_ABCDSR_P2_Msk instead */
#define PIO_ABCDSR_P3_Pos                   3                                              /**< (PIO_ABCDSR) Peripheral Select Position */
#define PIO_ABCDSR_P3_Msk                   (0x1U << PIO_ABCDSR_P3_Pos)                    /**< (PIO_ABCDSR) Peripheral Select Mask */
#define PIO_ABCDSR_P3                       PIO_ABCDSR_P3_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_ABCDSR_P3_Msk instead */
#define PIO_ABCDSR_P4_Pos                   4                                              /**< (PIO_ABCDSR) Peripheral Select Position */
#define PIO_ABCDSR_P4_Msk                   (0x1U << PIO_ABCDSR_P4_Pos)                    /**< (PIO_ABCDSR) Peripheral Select Mask */
#define PIO_ABCDSR_P4                       PIO_ABCDSR_P4_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_ABCDSR_P4_Msk instead */
#define PIO_ABCDSR_P5_Pos                   5                                              /**< (PIO_ABCDSR) Peripheral Select Position */
#define PIO_ABCDSR_P5_Msk                   (0x1U << PIO_ABCDSR_P5_Pos)                    /**< (PIO_ABCDSR) Peripheral Select Mask */
#define PIO_ABCDSR_P5                       PIO_ABCDSR_P5_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_ABCDSR_P5_Msk instead */
#define PIO_ABCDSR_P6_Pos                   6                                              /**< (PIO_ABCDSR) Peripheral Select Position */
#define PIO_ABCDSR_P6_Msk                   (0x1U << PIO_ABCDSR_P6_Pos)                    /**< (PIO_ABCDSR) Peripheral Select Mask */
#define PIO_ABCDSR_P6                       PIO_ABCDSR_P6_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_ABCDSR_P6_Msk instead */
#define PIO_ABCDSR_P7_Pos                   7                                              /**< (PIO_ABCDSR) Peripheral Select Position */
#define PIO_ABCDSR_P7_Msk                   (0x1U << PIO_ABCDSR_P7_Pos)                    /**< (PIO_ABCDSR) Peripheral Select Mask */
#define PIO_ABCDSR_P7                       PIO_ABCDSR_P7_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_ABCDSR_P7_Msk instead */
#define PIO_ABCDSR_P8_Pos                   8                                              /**< (PIO_ABCDSR) Peripheral Select Position */
#define PIO_ABCDSR_P8_Msk                   (0x1U << PIO_ABCDSR_P8_Pos)                    /**< (PIO_ABCDSR) Peripheral Select Mask */
#define PIO_ABCDSR_P8                       PIO_ABCDSR_P8_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_ABCDSR_P8_Msk instead */
#define PIO_ABCDSR_P9_Pos                   9                                              /**< (PIO_ABCDSR) Peripheral Select Position */
#define PIO_ABCDSR_P9_Msk                   (0x1U << PIO_ABCDSR_P9_Pos)                    /**< (PIO_ABCDSR) Peripheral Select Mask */
#define PIO_ABCDSR_P9                       PIO_ABCDSR_P9_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_ABCDSR_P9_Msk instead */
#define PIO_ABCDSR_P10_Pos                  10                                             /**< (PIO_ABCDSR) Peripheral Select Position */
#define PIO_ABCDSR_P10_Msk                  (0x1U << PIO_ABCDSR_P10_Pos)                   /**< (PIO_ABCDSR) Peripheral Select Mask */
#define PIO_ABCDSR_P10                      PIO_ABCDSR_P10_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_ABCDSR_P10_Msk instead */
#define PIO_ABCDSR_P11_Pos                  11                                             /**< (PIO_ABCDSR) Peripheral Select Position */
#define PIO_ABCDSR_P11_Msk                  (0x1U << PIO_ABCDSR_P11_Pos)                   /**< (PIO_ABCDSR) Peripheral Select Mask */
#define PIO_ABCDSR_P11                      PIO_ABCDSR_P11_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_ABCDSR_P11_Msk instead */
#define PIO_ABCDSR_P12_Pos                  12                                             /**< (PIO_ABCDSR) Peripheral Select Position */
#define PIO_ABCDSR_P12_Msk                  (0x1U << PIO_ABCDSR_P12_Pos)                   /**< (PIO_ABCDSR) Peripheral Select Mask */
#define PIO_ABCDSR_P12                      PIO_ABCDSR_P12_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_ABCDSR_P12_Msk instead */
#define PIO_ABCDSR_P13_Pos                  13                                             /**< (PIO_ABCDSR) Peripheral Select Position */
#define PIO_ABCDSR_P13_Msk                  (0x1U << PIO_ABCDSR_P13_Pos)                   /**< (PIO_ABCDSR) Peripheral Select Mask */
#define PIO_ABCDSR_P13                      PIO_ABCDSR_P13_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_ABCDSR_P13_Msk instead */
#define PIO_ABCDSR_P14_Pos                  14                                             /**< (PIO_ABCDSR) Peripheral Select Position */
#define PIO_ABCDSR_P14_Msk                  (0x1U << PIO_ABCDSR_P14_Pos)                   /**< (PIO_ABCDSR) Peripheral Select Mask */
#define PIO_ABCDSR_P14                      PIO_ABCDSR_P14_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_ABCDSR_P14_Msk instead */
#define PIO_ABCDSR_P15_Pos                  15                                             /**< (PIO_ABCDSR) Peripheral Select Position */
#define PIO_ABCDSR_P15_Msk                  (0x1U << PIO_ABCDSR_P15_Pos)                   /**< (PIO_ABCDSR) Peripheral Select Mask */
#define PIO_ABCDSR_P15                      PIO_ABCDSR_P15_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_ABCDSR_P15_Msk instead */
#define PIO_ABCDSR_P16_Pos                  16                                             /**< (PIO_ABCDSR) Peripheral Select Position */
#define PIO_ABCDSR_P16_Msk                  (0x1U << PIO_ABCDSR_P16_Pos)                   /**< (PIO_ABCDSR) Peripheral Select Mask */
#define PIO_ABCDSR_P16                      PIO_ABCDSR_P16_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_ABCDSR_P16_Msk instead */
#define PIO_ABCDSR_P17_Pos                  17                                             /**< (PIO_ABCDSR) Peripheral Select Position */
#define PIO_ABCDSR_P17_Msk                  (0x1U << PIO_ABCDSR_P17_Pos)                   /**< (PIO_ABCDSR) Peripheral Select Mask */
#define PIO_ABCDSR_P17                      PIO_ABCDSR_P17_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_ABCDSR_P17_Msk instead */
#define PIO_ABCDSR_P18_Pos                  18                                             /**< (PIO_ABCDSR) Peripheral Select Position */
#define PIO_ABCDSR_P18_Msk                  (0x1U << PIO_ABCDSR_P18_Pos)                   /**< (PIO_ABCDSR) Peripheral Select Mask */
#define PIO_ABCDSR_P18                      PIO_ABCDSR_P18_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_ABCDSR_P18_Msk instead */
#define PIO_ABCDSR_P19_Pos                  19                                             /**< (PIO_ABCDSR) Peripheral Select Position */
#define PIO_ABCDSR_P19_Msk                  (0x1U << PIO_ABCDSR_P19_Pos)                   /**< (PIO_ABCDSR) Peripheral Select Mask */
#define PIO_ABCDSR_P19                      PIO_ABCDSR_P19_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_ABCDSR_P19_Msk instead */
#define PIO_ABCDSR_P20_Pos                  20                                             /**< (PIO_ABCDSR) Peripheral Select Position */
#define PIO_ABCDSR_P20_Msk                  (0x1U << PIO_ABCDSR_P20_Pos)                   /**< (PIO_ABCDSR) Peripheral Select Mask */
#define PIO_ABCDSR_P20                      PIO_ABCDSR_P20_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_ABCDSR_P20_Msk instead */
#define PIO_ABCDSR_P21_Pos                  21                                             /**< (PIO_ABCDSR) Peripheral Select Position */
#define PIO_ABCDSR_P21_Msk                  (0x1U << PIO_ABCDSR_P21_Pos)                   /**< (PIO_ABCDSR) Peripheral Select Mask */
#define PIO_ABCDSR_P21                      PIO_ABCDSR_P21_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_ABCDSR_P21_Msk instead */
#define PIO_ABCDSR_P22_Pos                  22                                             /**< (PIO_ABCDSR) Peripheral Select Position */
#define PIO_ABCDSR_P22_Msk                  (0x1U << PIO_ABCDSR_P22_Pos)                   /**< (PIO_ABCDSR) Peripheral Select Mask */
#define PIO_ABCDSR_P22                      PIO_ABCDSR_P22_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_ABCDSR_P22_Msk instead */
#define PIO_ABCDSR_P23_Pos                  23                                             /**< (PIO_ABCDSR) Peripheral Select Position */
#define PIO_ABCDSR_P23_Msk                  (0x1U << PIO_ABCDSR_P23_Pos)                   /**< (PIO_ABCDSR) Peripheral Select Mask */
#define PIO_ABCDSR_P23                      PIO_ABCDSR_P23_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_ABCDSR_P23_Msk instead */
#define PIO_ABCDSR_P24_Pos                  24                                             /**< (PIO_ABCDSR) Peripheral Select Position */
#define PIO_ABCDSR_P24_Msk                  (0x1U << PIO_ABCDSR_P24_Pos)                   /**< (PIO_ABCDSR) Peripheral Select Mask */
#define PIO_ABCDSR_P24                      PIO_ABCDSR_P24_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_ABCDSR_P24_Msk instead */
#define PIO_ABCDSR_P25_Pos                  25                                             /**< (PIO_ABCDSR) Peripheral Select Position */
#define PIO_ABCDSR_P25_Msk                  (0x1U << PIO_ABCDSR_P25_Pos)                   /**< (PIO_ABCDSR) Peripheral Select Mask */
#define PIO_ABCDSR_P25                      PIO_ABCDSR_P25_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_ABCDSR_P25_Msk instead */
#define PIO_ABCDSR_P26_Pos                  26                                             /**< (PIO_ABCDSR) Peripheral Select Position */
#define PIO_ABCDSR_P26_Msk                  (0x1U << PIO_ABCDSR_P26_Pos)                   /**< (PIO_ABCDSR) Peripheral Select Mask */
#define PIO_ABCDSR_P26                      PIO_ABCDSR_P26_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_ABCDSR_P26_Msk instead */
#define PIO_ABCDSR_P27_Pos                  27                                             /**< (PIO_ABCDSR) Peripheral Select Position */
#define PIO_ABCDSR_P27_Msk                  (0x1U << PIO_ABCDSR_P27_Pos)                   /**< (PIO_ABCDSR) Peripheral Select Mask */
#define PIO_ABCDSR_P27                      PIO_ABCDSR_P27_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_ABCDSR_P27_Msk instead */
#define PIO_ABCDSR_P28_Pos                  28                                             /**< (PIO_ABCDSR) Peripheral Select Position */
#define PIO_ABCDSR_P28_Msk                  (0x1U << PIO_ABCDSR_P28_Pos)                   /**< (PIO_ABCDSR) Peripheral Select Mask */
#define PIO_ABCDSR_P28                      PIO_ABCDSR_P28_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_ABCDSR_P28_Msk instead */
#define PIO_ABCDSR_P29_Pos                  29                                             /**< (PIO_ABCDSR) Peripheral Select Position */
#define PIO_ABCDSR_P29_Msk                  (0x1U << PIO_ABCDSR_P29_Pos)                   /**< (PIO_ABCDSR) Peripheral Select Mask */
#define PIO_ABCDSR_P29                      PIO_ABCDSR_P29_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_ABCDSR_P29_Msk instead */
#define PIO_ABCDSR_P30_Pos                  30                                             /**< (PIO_ABCDSR) Peripheral Select Position */
#define PIO_ABCDSR_P30_Msk                  (0x1U << PIO_ABCDSR_P30_Pos)                   /**< (PIO_ABCDSR) Peripheral Select Mask */
#define PIO_ABCDSR_P30                      PIO_ABCDSR_P30_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_ABCDSR_P30_Msk instead */
#define PIO_ABCDSR_P31_Pos                  31                                             /**< (PIO_ABCDSR) Peripheral Select Position */
#define PIO_ABCDSR_P31_Msk                  (0x1U << PIO_ABCDSR_P31_Pos)                   /**< (PIO_ABCDSR) Peripheral Select Mask */
#define PIO_ABCDSR_P31                      PIO_ABCDSR_P31_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_ABCDSR_P31_Msk instead */
#define PIO_ABCDSR_P_Pos                    0                                              /**< (PIO_ABCDSR Position) Peripheral Select */
#define PIO_ABCDSR_P_Msk                    (0xFFFFFFFFU << PIO_ABCDSR_P_Pos)              /**< (PIO_ABCDSR Mask) P */
#define PIO_ABCDSR_P(value)                 (PIO_ABCDSR_P_Msk & ((value) << PIO_ABCDSR_P_Pos))  
#define PIO_ABCDSR_MASK                     (0xFFFFFFFFU)                                  /**< \deprecated (PIO_ABCDSR) Register MASK  (Use PIO_ABCDSR_Msk instead)  */
#define PIO_ABCDSR_Msk                      (0xFFFFFFFFU)                                  /**< (PIO_ABCDSR) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- PIO_IFSCDR : (PIO Offset: 0x80) (/W 32) Input Filter Slow Clock Disable Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t P0:1;                      /**< bit:      0  Peripheral Clock Glitch Filtering Select */
    uint32_t P1:1;                      /**< bit:      1  Peripheral Clock Glitch Filtering Select */
    uint32_t P2:1;                      /**< bit:      2  Peripheral Clock Glitch Filtering Select */
    uint32_t P3:1;                      /**< bit:      3  Peripheral Clock Glitch Filtering Select */
    uint32_t P4:1;                      /**< bit:      4  Peripheral Clock Glitch Filtering Select */
    uint32_t P5:1;                      /**< bit:      5  Peripheral Clock Glitch Filtering Select */
    uint32_t P6:1;                      /**< bit:      6  Peripheral Clock Glitch Filtering Select */
    uint32_t P7:1;                      /**< bit:      7  Peripheral Clock Glitch Filtering Select */
    uint32_t P8:1;                      /**< bit:      8  Peripheral Clock Glitch Filtering Select */
    uint32_t P9:1;                      /**< bit:      9  Peripheral Clock Glitch Filtering Select */
    uint32_t P10:1;                     /**< bit:     10  Peripheral Clock Glitch Filtering Select */
    uint32_t P11:1;                     /**< bit:     11  Peripheral Clock Glitch Filtering Select */
    uint32_t P12:1;                     /**< bit:     12  Peripheral Clock Glitch Filtering Select */
    uint32_t P13:1;                     /**< bit:     13  Peripheral Clock Glitch Filtering Select */
    uint32_t P14:1;                     /**< bit:     14  Peripheral Clock Glitch Filtering Select */
    uint32_t P15:1;                     /**< bit:     15  Peripheral Clock Glitch Filtering Select */
    uint32_t P16:1;                     /**< bit:     16  Peripheral Clock Glitch Filtering Select */
    uint32_t P17:1;                     /**< bit:     17  Peripheral Clock Glitch Filtering Select */
    uint32_t P18:1;                     /**< bit:     18  Peripheral Clock Glitch Filtering Select */
    uint32_t P19:1;                     /**< bit:     19  Peripheral Clock Glitch Filtering Select */
    uint32_t P20:1;                     /**< bit:     20  Peripheral Clock Glitch Filtering Select */
    uint32_t P21:1;                     /**< bit:     21  Peripheral Clock Glitch Filtering Select */
    uint32_t P22:1;                     /**< bit:     22  Peripheral Clock Glitch Filtering Select */
    uint32_t P23:1;                     /**< bit:     23  Peripheral Clock Glitch Filtering Select */
    uint32_t P24:1;                     /**< bit:     24  Peripheral Clock Glitch Filtering Select */
    uint32_t P25:1;                     /**< bit:     25  Peripheral Clock Glitch Filtering Select */
    uint32_t P26:1;                     /**< bit:     26  Peripheral Clock Glitch Filtering Select */
    uint32_t P27:1;                     /**< bit:     27  Peripheral Clock Glitch Filtering Select */
    uint32_t P28:1;                     /**< bit:     28  Peripheral Clock Glitch Filtering Select */
    uint32_t P29:1;                     /**< bit:     29  Peripheral Clock Glitch Filtering Select */
    uint32_t P30:1;                     /**< bit:     30  Peripheral Clock Glitch Filtering Select */
    uint32_t P31:1;                     /**< bit:     31  Peripheral Clock Glitch Filtering Select */
  } bit;                                /**< Structure used for bit  access */
  struct {
    uint32_t P:32;                      /**< bit:  0..31  Peripheral Clock Glitch Filtering Select */
  } vec;                                /**< Structure used for vec  access  */
  uint32_t reg;                         /**< Type used for register access */
} PIO_IFSCDR_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define PIO_IFSCDR_OFFSET                   (0x80)                                        /**<  (PIO_IFSCDR) Input Filter Slow Clock Disable Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define PIO_IFSCDR_P0_Pos                   0                                              /**< (PIO_IFSCDR) Peripheral Clock Glitch Filtering Select Position */
#define PIO_IFSCDR_P0_Msk                   (0x1U << PIO_IFSCDR_P0_Pos)                    /**< (PIO_IFSCDR) Peripheral Clock Glitch Filtering Select Mask */
#define PIO_IFSCDR_P0                       PIO_IFSCDR_P0_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_IFSCDR_P0_Msk instead */
#define PIO_IFSCDR_P1_Pos                   1                                              /**< (PIO_IFSCDR) Peripheral Clock Glitch Filtering Select Position */
#define PIO_IFSCDR_P1_Msk                   (0x1U << PIO_IFSCDR_P1_Pos)                    /**< (PIO_IFSCDR) Peripheral Clock Glitch Filtering Select Mask */
#define PIO_IFSCDR_P1                       PIO_IFSCDR_P1_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_IFSCDR_P1_Msk instead */
#define PIO_IFSCDR_P2_Pos                   2                                              /**< (PIO_IFSCDR) Peripheral Clock Glitch Filtering Select Position */
#define PIO_IFSCDR_P2_Msk                   (0x1U << PIO_IFSCDR_P2_Pos)                    /**< (PIO_IFSCDR) Peripheral Clock Glitch Filtering Select Mask */
#define PIO_IFSCDR_P2                       PIO_IFSCDR_P2_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_IFSCDR_P2_Msk instead */
#define PIO_IFSCDR_P3_Pos                   3                                              /**< (PIO_IFSCDR) Peripheral Clock Glitch Filtering Select Position */
#define PIO_IFSCDR_P3_Msk                   (0x1U << PIO_IFSCDR_P3_Pos)                    /**< (PIO_IFSCDR) Peripheral Clock Glitch Filtering Select Mask */
#define PIO_IFSCDR_P3                       PIO_IFSCDR_P3_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_IFSCDR_P3_Msk instead */
#define PIO_IFSCDR_P4_Pos                   4                                              /**< (PIO_IFSCDR) Peripheral Clock Glitch Filtering Select Position */
#define PIO_IFSCDR_P4_Msk                   (0x1U << PIO_IFSCDR_P4_Pos)                    /**< (PIO_IFSCDR) Peripheral Clock Glitch Filtering Select Mask */
#define PIO_IFSCDR_P4                       PIO_IFSCDR_P4_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_IFSCDR_P4_Msk instead */
#define PIO_IFSCDR_P5_Pos                   5                                              /**< (PIO_IFSCDR) Peripheral Clock Glitch Filtering Select Position */
#define PIO_IFSCDR_P5_Msk                   (0x1U << PIO_IFSCDR_P5_Pos)                    /**< (PIO_IFSCDR) Peripheral Clock Glitch Filtering Select Mask */
#define PIO_IFSCDR_P5                       PIO_IFSCDR_P5_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_IFSCDR_P5_Msk instead */
#define PIO_IFSCDR_P6_Pos                   6                                              /**< (PIO_IFSCDR) Peripheral Clock Glitch Filtering Select Position */
#define PIO_IFSCDR_P6_Msk                   (0x1U << PIO_IFSCDR_P6_Pos)                    /**< (PIO_IFSCDR) Peripheral Clock Glitch Filtering Select Mask */
#define PIO_IFSCDR_P6                       PIO_IFSCDR_P6_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_IFSCDR_P6_Msk instead */
#define PIO_IFSCDR_P7_Pos                   7                                              /**< (PIO_IFSCDR) Peripheral Clock Glitch Filtering Select Position */
#define PIO_IFSCDR_P7_Msk                   (0x1U << PIO_IFSCDR_P7_Pos)                    /**< (PIO_IFSCDR) Peripheral Clock Glitch Filtering Select Mask */
#define PIO_IFSCDR_P7                       PIO_IFSCDR_P7_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_IFSCDR_P7_Msk instead */
#define PIO_IFSCDR_P8_Pos                   8                                              /**< (PIO_IFSCDR) Peripheral Clock Glitch Filtering Select Position */
#define PIO_IFSCDR_P8_Msk                   (0x1U << PIO_IFSCDR_P8_Pos)                    /**< (PIO_IFSCDR) Peripheral Clock Glitch Filtering Select Mask */
#define PIO_IFSCDR_P8                       PIO_IFSCDR_P8_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_IFSCDR_P8_Msk instead */
#define PIO_IFSCDR_P9_Pos                   9                                              /**< (PIO_IFSCDR) Peripheral Clock Glitch Filtering Select Position */
#define PIO_IFSCDR_P9_Msk                   (0x1U << PIO_IFSCDR_P9_Pos)                    /**< (PIO_IFSCDR) Peripheral Clock Glitch Filtering Select Mask */
#define PIO_IFSCDR_P9                       PIO_IFSCDR_P9_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_IFSCDR_P9_Msk instead */
#define PIO_IFSCDR_P10_Pos                  10                                             /**< (PIO_IFSCDR) Peripheral Clock Glitch Filtering Select Position */
#define PIO_IFSCDR_P10_Msk                  (0x1U << PIO_IFSCDR_P10_Pos)                   /**< (PIO_IFSCDR) Peripheral Clock Glitch Filtering Select Mask */
#define PIO_IFSCDR_P10                      PIO_IFSCDR_P10_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_IFSCDR_P10_Msk instead */
#define PIO_IFSCDR_P11_Pos                  11                                             /**< (PIO_IFSCDR) Peripheral Clock Glitch Filtering Select Position */
#define PIO_IFSCDR_P11_Msk                  (0x1U << PIO_IFSCDR_P11_Pos)                   /**< (PIO_IFSCDR) Peripheral Clock Glitch Filtering Select Mask */
#define PIO_IFSCDR_P11                      PIO_IFSCDR_P11_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_IFSCDR_P11_Msk instead */
#define PIO_IFSCDR_P12_Pos                  12                                             /**< (PIO_IFSCDR) Peripheral Clock Glitch Filtering Select Position */
#define PIO_IFSCDR_P12_Msk                  (0x1U << PIO_IFSCDR_P12_Pos)                   /**< (PIO_IFSCDR) Peripheral Clock Glitch Filtering Select Mask */
#define PIO_IFSCDR_P12                      PIO_IFSCDR_P12_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_IFSCDR_P12_Msk instead */
#define PIO_IFSCDR_P13_Pos                  13                                             /**< (PIO_IFSCDR) Peripheral Clock Glitch Filtering Select Position */
#define PIO_IFSCDR_P13_Msk                  (0x1U << PIO_IFSCDR_P13_Pos)                   /**< (PIO_IFSCDR) Peripheral Clock Glitch Filtering Select Mask */
#define PIO_IFSCDR_P13                      PIO_IFSCDR_P13_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_IFSCDR_P13_Msk instead */
#define PIO_IFSCDR_P14_Pos                  14                                             /**< (PIO_IFSCDR) Peripheral Clock Glitch Filtering Select Position */
#define PIO_IFSCDR_P14_Msk                  (0x1U << PIO_IFSCDR_P14_Pos)                   /**< (PIO_IFSCDR) Peripheral Clock Glitch Filtering Select Mask */
#define PIO_IFSCDR_P14                      PIO_IFSCDR_P14_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_IFSCDR_P14_Msk instead */
#define PIO_IFSCDR_P15_Pos                  15                                             /**< (PIO_IFSCDR) Peripheral Clock Glitch Filtering Select Position */
#define PIO_IFSCDR_P15_Msk                  (0x1U << PIO_IFSCDR_P15_Pos)                   /**< (PIO_IFSCDR) Peripheral Clock Glitch Filtering Select Mask */
#define PIO_IFSCDR_P15                      PIO_IFSCDR_P15_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_IFSCDR_P15_Msk instead */
#define PIO_IFSCDR_P16_Pos                  16                                             /**< (PIO_IFSCDR) Peripheral Clock Glitch Filtering Select Position */
#define PIO_IFSCDR_P16_Msk                  (0x1U << PIO_IFSCDR_P16_Pos)                   /**< (PIO_IFSCDR) Peripheral Clock Glitch Filtering Select Mask */
#define PIO_IFSCDR_P16                      PIO_IFSCDR_P16_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_IFSCDR_P16_Msk instead */
#define PIO_IFSCDR_P17_Pos                  17                                             /**< (PIO_IFSCDR) Peripheral Clock Glitch Filtering Select Position */
#define PIO_IFSCDR_P17_Msk                  (0x1U << PIO_IFSCDR_P17_Pos)                   /**< (PIO_IFSCDR) Peripheral Clock Glitch Filtering Select Mask */
#define PIO_IFSCDR_P17                      PIO_IFSCDR_P17_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_IFSCDR_P17_Msk instead */
#define PIO_IFSCDR_P18_Pos                  18                                             /**< (PIO_IFSCDR) Peripheral Clock Glitch Filtering Select Position */
#define PIO_IFSCDR_P18_Msk                  (0x1U << PIO_IFSCDR_P18_Pos)                   /**< (PIO_IFSCDR) Peripheral Clock Glitch Filtering Select Mask */
#define PIO_IFSCDR_P18                      PIO_IFSCDR_P18_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_IFSCDR_P18_Msk instead */
#define PIO_IFSCDR_P19_Pos                  19                                             /**< (PIO_IFSCDR) Peripheral Clock Glitch Filtering Select Position */
#define PIO_IFSCDR_P19_Msk                  (0x1U << PIO_IFSCDR_P19_Pos)                   /**< (PIO_IFSCDR) Peripheral Clock Glitch Filtering Select Mask */
#define PIO_IFSCDR_P19                      PIO_IFSCDR_P19_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_IFSCDR_P19_Msk instead */
#define PIO_IFSCDR_P20_Pos                  20                                             /**< (PIO_IFSCDR) Peripheral Clock Glitch Filtering Select Position */
#define PIO_IFSCDR_P20_Msk                  (0x1U << PIO_IFSCDR_P20_Pos)                   /**< (PIO_IFSCDR) Peripheral Clock Glitch Filtering Select Mask */
#define PIO_IFSCDR_P20                      PIO_IFSCDR_P20_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_IFSCDR_P20_Msk instead */
#define PIO_IFSCDR_P21_Pos                  21                                             /**< (PIO_IFSCDR) Peripheral Clock Glitch Filtering Select Position */
#define PIO_IFSCDR_P21_Msk                  (0x1U << PIO_IFSCDR_P21_Pos)                   /**< (PIO_IFSCDR) Peripheral Clock Glitch Filtering Select Mask */
#define PIO_IFSCDR_P21                      PIO_IFSCDR_P21_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_IFSCDR_P21_Msk instead */
#define PIO_IFSCDR_P22_Pos                  22                                             /**< (PIO_IFSCDR) Peripheral Clock Glitch Filtering Select Position */
#define PIO_IFSCDR_P22_Msk                  (0x1U << PIO_IFSCDR_P22_Pos)                   /**< (PIO_IFSCDR) Peripheral Clock Glitch Filtering Select Mask */
#define PIO_IFSCDR_P22                      PIO_IFSCDR_P22_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_IFSCDR_P22_Msk instead */
#define PIO_IFSCDR_P23_Pos                  23                                             /**< (PIO_IFSCDR) Peripheral Clock Glitch Filtering Select Position */
#define PIO_IFSCDR_P23_Msk                  (0x1U << PIO_IFSCDR_P23_Pos)                   /**< (PIO_IFSCDR) Peripheral Clock Glitch Filtering Select Mask */
#define PIO_IFSCDR_P23                      PIO_IFSCDR_P23_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_IFSCDR_P23_Msk instead */
#define PIO_IFSCDR_P24_Pos                  24                                             /**< (PIO_IFSCDR) Peripheral Clock Glitch Filtering Select Position */
#define PIO_IFSCDR_P24_Msk                  (0x1U << PIO_IFSCDR_P24_Pos)                   /**< (PIO_IFSCDR) Peripheral Clock Glitch Filtering Select Mask */
#define PIO_IFSCDR_P24                      PIO_IFSCDR_P24_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_IFSCDR_P24_Msk instead */
#define PIO_IFSCDR_P25_Pos                  25                                             /**< (PIO_IFSCDR) Peripheral Clock Glitch Filtering Select Position */
#define PIO_IFSCDR_P25_Msk                  (0x1U << PIO_IFSCDR_P25_Pos)                   /**< (PIO_IFSCDR) Peripheral Clock Glitch Filtering Select Mask */
#define PIO_IFSCDR_P25                      PIO_IFSCDR_P25_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_IFSCDR_P25_Msk instead */
#define PIO_IFSCDR_P26_Pos                  26                                             /**< (PIO_IFSCDR) Peripheral Clock Glitch Filtering Select Position */
#define PIO_IFSCDR_P26_Msk                  (0x1U << PIO_IFSCDR_P26_Pos)                   /**< (PIO_IFSCDR) Peripheral Clock Glitch Filtering Select Mask */
#define PIO_IFSCDR_P26                      PIO_IFSCDR_P26_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_IFSCDR_P26_Msk instead */
#define PIO_IFSCDR_P27_Pos                  27                                             /**< (PIO_IFSCDR) Peripheral Clock Glitch Filtering Select Position */
#define PIO_IFSCDR_P27_Msk                  (0x1U << PIO_IFSCDR_P27_Pos)                   /**< (PIO_IFSCDR) Peripheral Clock Glitch Filtering Select Mask */
#define PIO_IFSCDR_P27                      PIO_IFSCDR_P27_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_IFSCDR_P27_Msk instead */
#define PIO_IFSCDR_P28_Pos                  28                                             /**< (PIO_IFSCDR) Peripheral Clock Glitch Filtering Select Position */
#define PIO_IFSCDR_P28_Msk                  (0x1U << PIO_IFSCDR_P28_Pos)                   /**< (PIO_IFSCDR) Peripheral Clock Glitch Filtering Select Mask */
#define PIO_IFSCDR_P28                      PIO_IFSCDR_P28_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_IFSCDR_P28_Msk instead */
#define PIO_IFSCDR_P29_Pos                  29                                             /**< (PIO_IFSCDR) Peripheral Clock Glitch Filtering Select Position */
#define PIO_IFSCDR_P29_Msk                  (0x1U << PIO_IFSCDR_P29_Pos)                   /**< (PIO_IFSCDR) Peripheral Clock Glitch Filtering Select Mask */
#define PIO_IFSCDR_P29                      PIO_IFSCDR_P29_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_IFSCDR_P29_Msk instead */
#define PIO_IFSCDR_P30_Pos                  30                                             /**< (PIO_IFSCDR) Peripheral Clock Glitch Filtering Select Position */
#define PIO_IFSCDR_P30_Msk                  (0x1U << PIO_IFSCDR_P30_Pos)                   /**< (PIO_IFSCDR) Peripheral Clock Glitch Filtering Select Mask */
#define PIO_IFSCDR_P30                      PIO_IFSCDR_P30_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_IFSCDR_P30_Msk instead */
#define PIO_IFSCDR_P31_Pos                  31                                             /**< (PIO_IFSCDR) Peripheral Clock Glitch Filtering Select Position */
#define PIO_IFSCDR_P31_Msk                  (0x1U << PIO_IFSCDR_P31_Pos)                   /**< (PIO_IFSCDR) Peripheral Clock Glitch Filtering Select Mask */
#define PIO_IFSCDR_P31                      PIO_IFSCDR_P31_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_IFSCDR_P31_Msk instead */
#define PIO_IFSCDR_P_Pos                    0                                              /**< (PIO_IFSCDR Position) Peripheral Clock Glitch Filtering Select */
#define PIO_IFSCDR_P_Msk                    (0xFFFFFFFFU << PIO_IFSCDR_P_Pos)              /**< (PIO_IFSCDR Mask) P */
#define PIO_IFSCDR_P(value)                 (PIO_IFSCDR_P_Msk & ((value) << PIO_IFSCDR_P_Pos))  
#define PIO_IFSCDR_MASK                     (0xFFFFFFFFU)                                  /**< \deprecated (PIO_IFSCDR) Register MASK  (Use PIO_IFSCDR_Msk instead)  */
#define PIO_IFSCDR_Msk                      (0xFFFFFFFFU)                                  /**< (PIO_IFSCDR) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- PIO_IFSCER : (PIO Offset: 0x84) (/W 32) Input Filter Slow Clock Enable Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t P0:1;                      /**< bit:      0  Slow Clock Debouncing Filtering Select   */
    uint32_t P1:1;                      /**< bit:      1  Slow Clock Debouncing Filtering Select   */
    uint32_t P2:1;                      /**< bit:      2  Slow Clock Debouncing Filtering Select   */
    uint32_t P3:1;                      /**< bit:      3  Slow Clock Debouncing Filtering Select   */
    uint32_t P4:1;                      /**< bit:      4  Slow Clock Debouncing Filtering Select   */
    uint32_t P5:1;                      /**< bit:      5  Slow Clock Debouncing Filtering Select   */
    uint32_t P6:1;                      /**< bit:      6  Slow Clock Debouncing Filtering Select   */
    uint32_t P7:1;                      /**< bit:      7  Slow Clock Debouncing Filtering Select   */
    uint32_t P8:1;                      /**< bit:      8  Slow Clock Debouncing Filtering Select   */
    uint32_t P9:1;                      /**< bit:      9  Slow Clock Debouncing Filtering Select   */
    uint32_t P10:1;                     /**< bit:     10  Slow Clock Debouncing Filtering Select   */
    uint32_t P11:1;                     /**< bit:     11  Slow Clock Debouncing Filtering Select   */
    uint32_t P12:1;                     /**< bit:     12  Slow Clock Debouncing Filtering Select   */
    uint32_t P13:1;                     /**< bit:     13  Slow Clock Debouncing Filtering Select   */
    uint32_t P14:1;                     /**< bit:     14  Slow Clock Debouncing Filtering Select   */
    uint32_t P15:1;                     /**< bit:     15  Slow Clock Debouncing Filtering Select   */
    uint32_t P16:1;                     /**< bit:     16  Slow Clock Debouncing Filtering Select   */
    uint32_t P17:1;                     /**< bit:     17  Slow Clock Debouncing Filtering Select   */
    uint32_t P18:1;                     /**< bit:     18  Slow Clock Debouncing Filtering Select   */
    uint32_t P19:1;                     /**< bit:     19  Slow Clock Debouncing Filtering Select   */
    uint32_t P20:1;                     /**< bit:     20  Slow Clock Debouncing Filtering Select   */
    uint32_t P21:1;                     /**< bit:     21  Slow Clock Debouncing Filtering Select   */
    uint32_t P22:1;                     /**< bit:     22  Slow Clock Debouncing Filtering Select   */
    uint32_t P23:1;                     /**< bit:     23  Slow Clock Debouncing Filtering Select   */
    uint32_t P24:1;                     /**< bit:     24  Slow Clock Debouncing Filtering Select   */
    uint32_t P25:1;                     /**< bit:     25  Slow Clock Debouncing Filtering Select   */
    uint32_t P26:1;                     /**< bit:     26  Slow Clock Debouncing Filtering Select   */
    uint32_t P27:1;                     /**< bit:     27  Slow Clock Debouncing Filtering Select   */
    uint32_t P28:1;                     /**< bit:     28  Slow Clock Debouncing Filtering Select   */
    uint32_t P29:1;                     /**< bit:     29  Slow Clock Debouncing Filtering Select   */
    uint32_t P30:1;                     /**< bit:     30  Slow Clock Debouncing Filtering Select   */
    uint32_t P31:1;                     /**< bit:     31  Slow Clock Debouncing Filtering Select   */
  } bit;                                /**< Structure used for bit  access */
  struct {
    uint32_t P:32;                      /**< bit:  0..31  Slow Clock Debouncing Filtering Select   */
  } vec;                                /**< Structure used for vec  access  */
  uint32_t reg;                         /**< Type used for register access */
} PIO_IFSCER_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define PIO_IFSCER_OFFSET                   (0x84)                                        /**<  (PIO_IFSCER) Input Filter Slow Clock Enable Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define PIO_IFSCER_P0_Pos                   0                                              /**< (PIO_IFSCER) Slow Clock Debouncing Filtering Select Position */
#define PIO_IFSCER_P0_Msk                   (0x1U << PIO_IFSCER_P0_Pos)                    /**< (PIO_IFSCER) Slow Clock Debouncing Filtering Select Mask */
#define PIO_IFSCER_P0                       PIO_IFSCER_P0_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_IFSCER_P0_Msk instead */
#define PIO_IFSCER_P1_Pos                   1                                              /**< (PIO_IFSCER) Slow Clock Debouncing Filtering Select Position */
#define PIO_IFSCER_P1_Msk                   (0x1U << PIO_IFSCER_P1_Pos)                    /**< (PIO_IFSCER) Slow Clock Debouncing Filtering Select Mask */
#define PIO_IFSCER_P1                       PIO_IFSCER_P1_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_IFSCER_P1_Msk instead */
#define PIO_IFSCER_P2_Pos                   2                                              /**< (PIO_IFSCER) Slow Clock Debouncing Filtering Select Position */
#define PIO_IFSCER_P2_Msk                   (0x1U << PIO_IFSCER_P2_Pos)                    /**< (PIO_IFSCER) Slow Clock Debouncing Filtering Select Mask */
#define PIO_IFSCER_P2                       PIO_IFSCER_P2_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_IFSCER_P2_Msk instead */
#define PIO_IFSCER_P3_Pos                   3                                              /**< (PIO_IFSCER) Slow Clock Debouncing Filtering Select Position */
#define PIO_IFSCER_P3_Msk                   (0x1U << PIO_IFSCER_P3_Pos)                    /**< (PIO_IFSCER) Slow Clock Debouncing Filtering Select Mask */
#define PIO_IFSCER_P3                       PIO_IFSCER_P3_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_IFSCER_P3_Msk instead */
#define PIO_IFSCER_P4_Pos                   4                                              /**< (PIO_IFSCER) Slow Clock Debouncing Filtering Select Position */
#define PIO_IFSCER_P4_Msk                   (0x1U << PIO_IFSCER_P4_Pos)                    /**< (PIO_IFSCER) Slow Clock Debouncing Filtering Select Mask */
#define PIO_IFSCER_P4                       PIO_IFSCER_P4_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_IFSCER_P4_Msk instead */
#define PIO_IFSCER_P5_Pos                   5                                              /**< (PIO_IFSCER) Slow Clock Debouncing Filtering Select Position */
#define PIO_IFSCER_P5_Msk                   (0x1U << PIO_IFSCER_P5_Pos)                    /**< (PIO_IFSCER) Slow Clock Debouncing Filtering Select Mask */
#define PIO_IFSCER_P5                       PIO_IFSCER_P5_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_IFSCER_P5_Msk instead */
#define PIO_IFSCER_P6_Pos                   6                                              /**< (PIO_IFSCER) Slow Clock Debouncing Filtering Select Position */
#define PIO_IFSCER_P6_Msk                   (0x1U << PIO_IFSCER_P6_Pos)                    /**< (PIO_IFSCER) Slow Clock Debouncing Filtering Select Mask */
#define PIO_IFSCER_P6                       PIO_IFSCER_P6_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_IFSCER_P6_Msk instead */
#define PIO_IFSCER_P7_Pos                   7                                              /**< (PIO_IFSCER) Slow Clock Debouncing Filtering Select Position */
#define PIO_IFSCER_P7_Msk                   (0x1U << PIO_IFSCER_P7_Pos)                    /**< (PIO_IFSCER) Slow Clock Debouncing Filtering Select Mask */
#define PIO_IFSCER_P7                       PIO_IFSCER_P7_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_IFSCER_P7_Msk instead */
#define PIO_IFSCER_P8_Pos                   8                                              /**< (PIO_IFSCER) Slow Clock Debouncing Filtering Select Position */
#define PIO_IFSCER_P8_Msk                   (0x1U << PIO_IFSCER_P8_Pos)                    /**< (PIO_IFSCER) Slow Clock Debouncing Filtering Select Mask */
#define PIO_IFSCER_P8                       PIO_IFSCER_P8_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_IFSCER_P8_Msk instead */
#define PIO_IFSCER_P9_Pos                   9                                              /**< (PIO_IFSCER) Slow Clock Debouncing Filtering Select Position */
#define PIO_IFSCER_P9_Msk                   (0x1U << PIO_IFSCER_P9_Pos)                    /**< (PIO_IFSCER) Slow Clock Debouncing Filtering Select Mask */
#define PIO_IFSCER_P9                       PIO_IFSCER_P9_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_IFSCER_P9_Msk instead */
#define PIO_IFSCER_P10_Pos                  10                                             /**< (PIO_IFSCER) Slow Clock Debouncing Filtering Select Position */
#define PIO_IFSCER_P10_Msk                  (0x1U << PIO_IFSCER_P10_Pos)                   /**< (PIO_IFSCER) Slow Clock Debouncing Filtering Select Mask */
#define PIO_IFSCER_P10                      PIO_IFSCER_P10_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_IFSCER_P10_Msk instead */
#define PIO_IFSCER_P11_Pos                  11                                             /**< (PIO_IFSCER) Slow Clock Debouncing Filtering Select Position */
#define PIO_IFSCER_P11_Msk                  (0x1U << PIO_IFSCER_P11_Pos)                   /**< (PIO_IFSCER) Slow Clock Debouncing Filtering Select Mask */
#define PIO_IFSCER_P11                      PIO_IFSCER_P11_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_IFSCER_P11_Msk instead */
#define PIO_IFSCER_P12_Pos                  12                                             /**< (PIO_IFSCER) Slow Clock Debouncing Filtering Select Position */
#define PIO_IFSCER_P12_Msk                  (0x1U << PIO_IFSCER_P12_Pos)                   /**< (PIO_IFSCER) Slow Clock Debouncing Filtering Select Mask */
#define PIO_IFSCER_P12                      PIO_IFSCER_P12_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_IFSCER_P12_Msk instead */
#define PIO_IFSCER_P13_Pos                  13                                             /**< (PIO_IFSCER) Slow Clock Debouncing Filtering Select Position */
#define PIO_IFSCER_P13_Msk                  (0x1U << PIO_IFSCER_P13_Pos)                   /**< (PIO_IFSCER) Slow Clock Debouncing Filtering Select Mask */
#define PIO_IFSCER_P13                      PIO_IFSCER_P13_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_IFSCER_P13_Msk instead */
#define PIO_IFSCER_P14_Pos                  14                                             /**< (PIO_IFSCER) Slow Clock Debouncing Filtering Select Position */
#define PIO_IFSCER_P14_Msk                  (0x1U << PIO_IFSCER_P14_Pos)                   /**< (PIO_IFSCER) Slow Clock Debouncing Filtering Select Mask */
#define PIO_IFSCER_P14                      PIO_IFSCER_P14_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_IFSCER_P14_Msk instead */
#define PIO_IFSCER_P15_Pos                  15                                             /**< (PIO_IFSCER) Slow Clock Debouncing Filtering Select Position */
#define PIO_IFSCER_P15_Msk                  (0x1U << PIO_IFSCER_P15_Pos)                   /**< (PIO_IFSCER) Slow Clock Debouncing Filtering Select Mask */
#define PIO_IFSCER_P15                      PIO_IFSCER_P15_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_IFSCER_P15_Msk instead */
#define PIO_IFSCER_P16_Pos                  16                                             /**< (PIO_IFSCER) Slow Clock Debouncing Filtering Select Position */
#define PIO_IFSCER_P16_Msk                  (0x1U << PIO_IFSCER_P16_Pos)                   /**< (PIO_IFSCER) Slow Clock Debouncing Filtering Select Mask */
#define PIO_IFSCER_P16                      PIO_IFSCER_P16_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_IFSCER_P16_Msk instead */
#define PIO_IFSCER_P17_Pos                  17                                             /**< (PIO_IFSCER) Slow Clock Debouncing Filtering Select Position */
#define PIO_IFSCER_P17_Msk                  (0x1U << PIO_IFSCER_P17_Pos)                   /**< (PIO_IFSCER) Slow Clock Debouncing Filtering Select Mask */
#define PIO_IFSCER_P17                      PIO_IFSCER_P17_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_IFSCER_P17_Msk instead */
#define PIO_IFSCER_P18_Pos                  18                                             /**< (PIO_IFSCER) Slow Clock Debouncing Filtering Select Position */
#define PIO_IFSCER_P18_Msk                  (0x1U << PIO_IFSCER_P18_Pos)                   /**< (PIO_IFSCER) Slow Clock Debouncing Filtering Select Mask */
#define PIO_IFSCER_P18                      PIO_IFSCER_P18_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_IFSCER_P18_Msk instead */
#define PIO_IFSCER_P19_Pos                  19                                             /**< (PIO_IFSCER) Slow Clock Debouncing Filtering Select Position */
#define PIO_IFSCER_P19_Msk                  (0x1U << PIO_IFSCER_P19_Pos)                   /**< (PIO_IFSCER) Slow Clock Debouncing Filtering Select Mask */
#define PIO_IFSCER_P19                      PIO_IFSCER_P19_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_IFSCER_P19_Msk instead */
#define PIO_IFSCER_P20_Pos                  20                                             /**< (PIO_IFSCER) Slow Clock Debouncing Filtering Select Position */
#define PIO_IFSCER_P20_Msk                  (0x1U << PIO_IFSCER_P20_Pos)                   /**< (PIO_IFSCER) Slow Clock Debouncing Filtering Select Mask */
#define PIO_IFSCER_P20                      PIO_IFSCER_P20_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_IFSCER_P20_Msk instead */
#define PIO_IFSCER_P21_Pos                  21                                             /**< (PIO_IFSCER) Slow Clock Debouncing Filtering Select Position */
#define PIO_IFSCER_P21_Msk                  (0x1U << PIO_IFSCER_P21_Pos)                   /**< (PIO_IFSCER) Slow Clock Debouncing Filtering Select Mask */
#define PIO_IFSCER_P21                      PIO_IFSCER_P21_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_IFSCER_P21_Msk instead */
#define PIO_IFSCER_P22_Pos                  22                                             /**< (PIO_IFSCER) Slow Clock Debouncing Filtering Select Position */
#define PIO_IFSCER_P22_Msk                  (0x1U << PIO_IFSCER_P22_Pos)                   /**< (PIO_IFSCER) Slow Clock Debouncing Filtering Select Mask */
#define PIO_IFSCER_P22                      PIO_IFSCER_P22_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_IFSCER_P22_Msk instead */
#define PIO_IFSCER_P23_Pos                  23                                             /**< (PIO_IFSCER) Slow Clock Debouncing Filtering Select Position */
#define PIO_IFSCER_P23_Msk                  (0x1U << PIO_IFSCER_P23_Pos)                   /**< (PIO_IFSCER) Slow Clock Debouncing Filtering Select Mask */
#define PIO_IFSCER_P23                      PIO_IFSCER_P23_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_IFSCER_P23_Msk instead */
#define PIO_IFSCER_P24_Pos                  24                                             /**< (PIO_IFSCER) Slow Clock Debouncing Filtering Select Position */
#define PIO_IFSCER_P24_Msk                  (0x1U << PIO_IFSCER_P24_Pos)                   /**< (PIO_IFSCER) Slow Clock Debouncing Filtering Select Mask */
#define PIO_IFSCER_P24                      PIO_IFSCER_P24_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_IFSCER_P24_Msk instead */
#define PIO_IFSCER_P25_Pos                  25                                             /**< (PIO_IFSCER) Slow Clock Debouncing Filtering Select Position */
#define PIO_IFSCER_P25_Msk                  (0x1U << PIO_IFSCER_P25_Pos)                   /**< (PIO_IFSCER) Slow Clock Debouncing Filtering Select Mask */
#define PIO_IFSCER_P25                      PIO_IFSCER_P25_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_IFSCER_P25_Msk instead */
#define PIO_IFSCER_P26_Pos                  26                                             /**< (PIO_IFSCER) Slow Clock Debouncing Filtering Select Position */
#define PIO_IFSCER_P26_Msk                  (0x1U << PIO_IFSCER_P26_Pos)                   /**< (PIO_IFSCER) Slow Clock Debouncing Filtering Select Mask */
#define PIO_IFSCER_P26                      PIO_IFSCER_P26_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_IFSCER_P26_Msk instead */
#define PIO_IFSCER_P27_Pos                  27                                             /**< (PIO_IFSCER) Slow Clock Debouncing Filtering Select Position */
#define PIO_IFSCER_P27_Msk                  (0x1U << PIO_IFSCER_P27_Pos)                   /**< (PIO_IFSCER) Slow Clock Debouncing Filtering Select Mask */
#define PIO_IFSCER_P27                      PIO_IFSCER_P27_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_IFSCER_P27_Msk instead */
#define PIO_IFSCER_P28_Pos                  28                                             /**< (PIO_IFSCER) Slow Clock Debouncing Filtering Select Position */
#define PIO_IFSCER_P28_Msk                  (0x1U << PIO_IFSCER_P28_Pos)                   /**< (PIO_IFSCER) Slow Clock Debouncing Filtering Select Mask */
#define PIO_IFSCER_P28                      PIO_IFSCER_P28_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_IFSCER_P28_Msk instead */
#define PIO_IFSCER_P29_Pos                  29                                             /**< (PIO_IFSCER) Slow Clock Debouncing Filtering Select Position */
#define PIO_IFSCER_P29_Msk                  (0x1U << PIO_IFSCER_P29_Pos)                   /**< (PIO_IFSCER) Slow Clock Debouncing Filtering Select Mask */
#define PIO_IFSCER_P29                      PIO_IFSCER_P29_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_IFSCER_P29_Msk instead */
#define PIO_IFSCER_P30_Pos                  30                                             /**< (PIO_IFSCER) Slow Clock Debouncing Filtering Select Position */
#define PIO_IFSCER_P30_Msk                  (0x1U << PIO_IFSCER_P30_Pos)                   /**< (PIO_IFSCER) Slow Clock Debouncing Filtering Select Mask */
#define PIO_IFSCER_P30                      PIO_IFSCER_P30_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_IFSCER_P30_Msk instead */
#define PIO_IFSCER_P31_Pos                  31                                             /**< (PIO_IFSCER) Slow Clock Debouncing Filtering Select Position */
#define PIO_IFSCER_P31_Msk                  (0x1U << PIO_IFSCER_P31_Pos)                   /**< (PIO_IFSCER) Slow Clock Debouncing Filtering Select Mask */
#define PIO_IFSCER_P31                      PIO_IFSCER_P31_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_IFSCER_P31_Msk instead */
#define PIO_IFSCER_P_Pos                    0                                              /**< (PIO_IFSCER Position) Slow Clock Debouncing Filtering Select */
#define PIO_IFSCER_P_Msk                    (0xFFFFFFFFU << PIO_IFSCER_P_Pos)              /**< (PIO_IFSCER Mask) P */
#define PIO_IFSCER_P(value)                 (PIO_IFSCER_P_Msk & ((value) << PIO_IFSCER_P_Pos))  
#define PIO_IFSCER_MASK                     (0xFFFFFFFFU)                                  /**< \deprecated (PIO_IFSCER) Register MASK  (Use PIO_IFSCER_Msk instead)  */
#define PIO_IFSCER_Msk                      (0xFFFFFFFFU)                                  /**< (PIO_IFSCER) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- PIO_IFSCSR : (PIO Offset: 0x88) (R/ 32) Input Filter Slow Clock Status Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t P0:1;                      /**< bit:      0  Glitch or Debouncing Filter Selection Status */
    uint32_t P1:1;                      /**< bit:      1  Glitch or Debouncing Filter Selection Status */
    uint32_t P2:1;                      /**< bit:      2  Glitch or Debouncing Filter Selection Status */
    uint32_t P3:1;                      /**< bit:      3  Glitch or Debouncing Filter Selection Status */
    uint32_t P4:1;                      /**< bit:      4  Glitch or Debouncing Filter Selection Status */
    uint32_t P5:1;                      /**< bit:      5  Glitch or Debouncing Filter Selection Status */
    uint32_t P6:1;                      /**< bit:      6  Glitch or Debouncing Filter Selection Status */
    uint32_t P7:1;                      /**< bit:      7  Glitch or Debouncing Filter Selection Status */
    uint32_t P8:1;                      /**< bit:      8  Glitch or Debouncing Filter Selection Status */
    uint32_t P9:1;                      /**< bit:      9  Glitch or Debouncing Filter Selection Status */
    uint32_t P10:1;                     /**< bit:     10  Glitch or Debouncing Filter Selection Status */
    uint32_t P11:1;                     /**< bit:     11  Glitch or Debouncing Filter Selection Status */
    uint32_t P12:1;                     /**< bit:     12  Glitch or Debouncing Filter Selection Status */
    uint32_t P13:1;                     /**< bit:     13  Glitch or Debouncing Filter Selection Status */
    uint32_t P14:1;                     /**< bit:     14  Glitch or Debouncing Filter Selection Status */
    uint32_t P15:1;                     /**< bit:     15  Glitch or Debouncing Filter Selection Status */
    uint32_t P16:1;                     /**< bit:     16  Glitch or Debouncing Filter Selection Status */
    uint32_t P17:1;                     /**< bit:     17  Glitch or Debouncing Filter Selection Status */
    uint32_t P18:1;                     /**< bit:     18  Glitch or Debouncing Filter Selection Status */
    uint32_t P19:1;                     /**< bit:     19  Glitch or Debouncing Filter Selection Status */
    uint32_t P20:1;                     /**< bit:     20  Glitch or Debouncing Filter Selection Status */
    uint32_t P21:1;                     /**< bit:     21  Glitch or Debouncing Filter Selection Status */
    uint32_t P22:1;                     /**< bit:     22  Glitch or Debouncing Filter Selection Status */
    uint32_t P23:1;                     /**< bit:     23  Glitch or Debouncing Filter Selection Status */
    uint32_t P24:1;                     /**< bit:     24  Glitch or Debouncing Filter Selection Status */
    uint32_t P25:1;                     /**< bit:     25  Glitch or Debouncing Filter Selection Status */
    uint32_t P26:1;                     /**< bit:     26  Glitch or Debouncing Filter Selection Status */
    uint32_t P27:1;                     /**< bit:     27  Glitch or Debouncing Filter Selection Status */
    uint32_t P28:1;                     /**< bit:     28  Glitch or Debouncing Filter Selection Status */
    uint32_t P29:1;                     /**< bit:     29  Glitch or Debouncing Filter Selection Status */
    uint32_t P30:1;                     /**< bit:     30  Glitch or Debouncing Filter Selection Status */
    uint32_t P31:1;                     /**< bit:     31  Glitch or Debouncing Filter Selection Status */
  } bit;                                /**< Structure used for bit  access */
  struct {
    uint32_t P:32;                      /**< bit:  0..31  Glitch or Debouncing Filter Selection Status */
  } vec;                                /**< Structure used for vec  access  */
  uint32_t reg;                         /**< Type used for register access */
} PIO_IFSCSR_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define PIO_IFSCSR_OFFSET                   (0x88)                                        /**<  (PIO_IFSCSR) Input Filter Slow Clock Status Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define PIO_IFSCSR_P0_Pos                   0                                              /**< (PIO_IFSCSR) Glitch or Debouncing Filter Selection Status Position */
#define PIO_IFSCSR_P0_Msk                   (0x1U << PIO_IFSCSR_P0_Pos)                    /**< (PIO_IFSCSR) Glitch or Debouncing Filter Selection Status Mask */
#define PIO_IFSCSR_P0                       PIO_IFSCSR_P0_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_IFSCSR_P0_Msk instead */
#define PIO_IFSCSR_P1_Pos                   1                                              /**< (PIO_IFSCSR) Glitch or Debouncing Filter Selection Status Position */
#define PIO_IFSCSR_P1_Msk                   (0x1U << PIO_IFSCSR_P1_Pos)                    /**< (PIO_IFSCSR) Glitch or Debouncing Filter Selection Status Mask */
#define PIO_IFSCSR_P1                       PIO_IFSCSR_P1_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_IFSCSR_P1_Msk instead */
#define PIO_IFSCSR_P2_Pos                   2                                              /**< (PIO_IFSCSR) Glitch or Debouncing Filter Selection Status Position */
#define PIO_IFSCSR_P2_Msk                   (0x1U << PIO_IFSCSR_P2_Pos)                    /**< (PIO_IFSCSR) Glitch or Debouncing Filter Selection Status Mask */
#define PIO_IFSCSR_P2                       PIO_IFSCSR_P2_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_IFSCSR_P2_Msk instead */
#define PIO_IFSCSR_P3_Pos                   3                                              /**< (PIO_IFSCSR) Glitch or Debouncing Filter Selection Status Position */
#define PIO_IFSCSR_P3_Msk                   (0x1U << PIO_IFSCSR_P3_Pos)                    /**< (PIO_IFSCSR) Glitch or Debouncing Filter Selection Status Mask */
#define PIO_IFSCSR_P3                       PIO_IFSCSR_P3_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_IFSCSR_P3_Msk instead */
#define PIO_IFSCSR_P4_Pos                   4                                              /**< (PIO_IFSCSR) Glitch or Debouncing Filter Selection Status Position */
#define PIO_IFSCSR_P4_Msk                   (0x1U << PIO_IFSCSR_P4_Pos)                    /**< (PIO_IFSCSR) Glitch or Debouncing Filter Selection Status Mask */
#define PIO_IFSCSR_P4                       PIO_IFSCSR_P4_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_IFSCSR_P4_Msk instead */
#define PIO_IFSCSR_P5_Pos                   5                                              /**< (PIO_IFSCSR) Glitch or Debouncing Filter Selection Status Position */
#define PIO_IFSCSR_P5_Msk                   (0x1U << PIO_IFSCSR_P5_Pos)                    /**< (PIO_IFSCSR) Glitch or Debouncing Filter Selection Status Mask */
#define PIO_IFSCSR_P5                       PIO_IFSCSR_P5_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_IFSCSR_P5_Msk instead */
#define PIO_IFSCSR_P6_Pos                   6                                              /**< (PIO_IFSCSR) Glitch or Debouncing Filter Selection Status Position */
#define PIO_IFSCSR_P6_Msk                   (0x1U << PIO_IFSCSR_P6_Pos)                    /**< (PIO_IFSCSR) Glitch or Debouncing Filter Selection Status Mask */
#define PIO_IFSCSR_P6                       PIO_IFSCSR_P6_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_IFSCSR_P6_Msk instead */
#define PIO_IFSCSR_P7_Pos                   7                                              /**< (PIO_IFSCSR) Glitch or Debouncing Filter Selection Status Position */
#define PIO_IFSCSR_P7_Msk                   (0x1U << PIO_IFSCSR_P7_Pos)                    /**< (PIO_IFSCSR) Glitch or Debouncing Filter Selection Status Mask */
#define PIO_IFSCSR_P7                       PIO_IFSCSR_P7_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_IFSCSR_P7_Msk instead */
#define PIO_IFSCSR_P8_Pos                   8                                              /**< (PIO_IFSCSR) Glitch or Debouncing Filter Selection Status Position */
#define PIO_IFSCSR_P8_Msk                   (0x1U << PIO_IFSCSR_P8_Pos)                    /**< (PIO_IFSCSR) Glitch or Debouncing Filter Selection Status Mask */
#define PIO_IFSCSR_P8                       PIO_IFSCSR_P8_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_IFSCSR_P8_Msk instead */
#define PIO_IFSCSR_P9_Pos                   9                                              /**< (PIO_IFSCSR) Glitch or Debouncing Filter Selection Status Position */
#define PIO_IFSCSR_P9_Msk                   (0x1U << PIO_IFSCSR_P9_Pos)                    /**< (PIO_IFSCSR) Glitch or Debouncing Filter Selection Status Mask */
#define PIO_IFSCSR_P9                       PIO_IFSCSR_P9_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_IFSCSR_P9_Msk instead */
#define PIO_IFSCSR_P10_Pos                  10                                             /**< (PIO_IFSCSR) Glitch or Debouncing Filter Selection Status Position */
#define PIO_IFSCSR_P10_Msk                  (0x1U << PIO_IFSCSR_P10_Pos)                   /**< (PIO_IFSCSR) Glitch or Debouncing Filter Selection Status Mask */
#define PIO_IFSCSR_P10                      PIO_IFSCSR_P10_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_IFSCSR_P10_Msk instead */
#define PIO_IFSCSR_P11_Pos                  11                                             /**< (PIO_IFSCSR) Glitch or Debouncing Filter Selection Status Position */
#define PIO_IFSCSR_P11_Msk                  (0x1U << PIO_IFSCSR_P11_Pos)                   /**< (PIO_IFSCSR) Glitch or Debouncing Filter Selection Status Mask */
#define PIO_IFSCSR_P11                      PIO_IFSCSR_P11_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_IFSCSR_P11_Msk instead */
#define PIO_IFSCSR_P12_Pos                  12                                             /**< (PIO_IFSCSR) Glitch or Debouncing Filter Selection Status Position */
#define PIO_IFSCSR_P12_Msk                  (0x1U << PIO_IFSCSR_P12_Pos)                   /**< (PIO_IFSCSR) Glitch or Debouncing Filter Selection Status Mask */
#define PIO_IFSCSR_P12                      PIO_IFSCSR_P12_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_IFSCSR_P12_Msk instead */
#define PIO_IFSCSR_P13_Pos                  13                                             /**< (PIO_IFSCSR) Glitch or Debouncing Filter Selection Status Position */
#define PIO_IFSCSR_P13_Msk                  (0x1U << PIO_IFSCSR_P13_Pos)                   /**< (PIO_IFSCSR) Glitch or Debouncing Filter Selection Status Mask */
#define PIO_IFSCSR_P13                      PIO_IFSCSR_P13_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_IFSCSR_P13_Msk instead */
#define PIO_IFSCSR_P14_Pos                  14                                             /**< (PIO_IFSCSR) Glitch or Debouncing Filter Selection Status Position */
#define PIO_IFSCSR_P14_Msk                  (0x1U << PIO_IFSCSR_P14_Pos)                   /**< (PIO_IFSCSR) Glitch or Debouncing Filter Selection Status Mask */
#define PIO_IFSCSR_P14                      PIO_IFSCSR_P14_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_IFSCSR_P14_Msk instead */
#define PIO_IFSCSR_P15_Pos                  15                                             /**< (PIO_IFSCSR) Glitch or Debouncing Filter Selection Status Position */
#define PIO_IFSCSR_P15_Msk                  (0x1U << PIO_IFSCSR_P15_Pos)                   /**< (PIO_IFSCSR) Glitch or Debouncing Filter Selection Status Mask */
#define PIO_IFSCSR_P15                      PIO_IFSCSR_P15_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_IFSCSR_P15_Msk instead */
#define PIO_IFSCSR_P16_Pos                  16                                             /**< (PIO_IFSCSR) Glitch or Debouncing Filter Selection Status Position */
#define PIO_IFSCSR_P16_Msk                  (0x1U << PIO_IFSCSR_P16_Pos)                   /**< (PIO_IFSCSR) Glitch or Debouncing Filter Selection Status Mask */
#define PIO_IFSCSR_P16                      PIO_IFSCSR_P16_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_IFSCSR_P16_Msk instead */
#define PIO_IFSCSR_P17_Pos                  17                                             /**< (PIO_IFSCSR) Glitch or Debouncing Filter Selection Status Position */
#define PIO_IFSCSR_P17_Msk                  (0x1U << PIO_IFSCSR_P17_Pos)                   /**< (PIO_IFSCSR) Glitch or Debouncing Filter Selection Status Mask */
#define PIO_IFSCSR_P17                      PIO_IFSCSR_P17_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_IFSCSR_P17_Msk instead */
#define PIO_IFSCSR_P18_Pos                  18                                             /**< (PIO_IFSCSR) Glitch or Debouncing Filter Selection Status Position */
#define PIO_IFSCSR_P18_Msk                  (0x1U << PIO_IFSCSR_P18_Pos)                   /**< (PIO_IFSCSR) Glitch or Debouncing Filter Selection Status Mask */
#define PIO_IFSCSR_P18                      PIO_IFSCSR_P18_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_IFSCSR_P18_Msk instead */
#define PIO_IFSCSR_P19_Pos                  19                                             /**< (PIO_IFSCSR) Glitch or Debouncing Filter Selection Status Position */
#define PIO_IFSCSR_P19_Msk                  (0x1U << PIO_IFSCSR_P19_Pos)                   /**< (PIO_IFSCSR) Glitch or Debouncing Filter Selection Status Mask */
#define PIO_IFSCSR_P19                      PIO_IFSCSR_P19_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_IFSCSR_P19_Msk instead */
#define PIO_IFSCSR_P20_Pos                  20                                             /**< (PIO_IFSCSR) Glitch or Debouncing Filter Selection Status Position */
#define PIO_IFSCSR_P20_Msk                  (0x1U << PIO_IFSCSR_P20_Pos)                   /**< (PIO_IFSCSR) Glitch or Debouncing Filter Selection Status Mask */
#define PIO_IFSCSR_P20                      PIO_IFSCSR_P20_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_IFSCSR_P20_Msk instead */
#define PIO_IFSCSR_P21_Pos                  21                                             /**< (PIO_IFSCSR) Glitch or Debouncing Filter Selection Status Position */
#define PIO_IFSCSR_P21_Msk                  (0x1U << PIO_IFSCSR_P21_Pos)                   /**< (PIO_IFSCSR) Glitch or Debouncing Filter Selection Status Mask */
#define PIO_IFSCSR_P21                      PIO_IFSCSR_P21_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_IFSCSR_P21_Msk instead */
#define PIO_IFSCSR_P22_Pos                  22                                             /**< (PIO_IFSCSR) Glitch or Debouncing Filter Selection Status Position */
#define PIO_IFSCSR_P22_Msk                  (0x1U << PIO_IFSCSR_P22_Pos)                   /**< (PIO_IFSCSR) Glitch or Debouncing Filter Selection Status Mask */
#define PIO_IFSCSR_P22                      PIO_IFSCSR_P22_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_IFSCSR_P22_Msk instead */
#define PIO_IFSCSR_P23_Pos                  23                                             /**< (PIO_IFSCSR) Glitch or Debouncing Filter Selection Status Position */
#define PIO_IFSCSR_P23_Msk                  (0x1U << PIO_IFSCSR_P23_Pos)                   /**< (PIO_IFSCSR) Glitch or Debouncing Filter Selection Status Mask */
#define PIO_IFSCSR_P23                      PIO_IFSCSR_P23_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_IFSCSR_P23_Msk instead */
#define PIO_IFSCSR_P24_Pos                  24                                             /**< (PIO_IFSCSR) Glitch or Debouncing Filter Selection Status Position */
#define PIO_IFSCSR_P24_Msk                  (0x1U << PIO_IFSCSR_P24_Pos)                   /**< (PIO_IFSCSR) Glitch or Debouncing Filter Selection Status Mask */
#define PIO_IFSCSR_P24                      PIO_IFSCSR_P24_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_IFSCSR_P24_Msk instead */
#define PIO_IFSCSR_P25_Pos                  25                                             /**< (PIO_IFSCSR) Glitch or Debouncing Filter Selection Status Position */
#define PIO_IFSCSR_P25_Msk                  (0x1U << PIO_IFSCSR_P25_Pos)                   /**< (PIO_IFSCSR) Glitch or Debouncing Filter Selection Status Mask */
#define PIO_IFSCSR_P25                      PIO_IFSCSR_P25_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_IFSCSR_P25_Msk instead */
#define PIO_IFSCSR_P26_Pos                  26                                             /**< (PIO_IFSCSR) Glitch or Debouncing Filter Selection Status Position */
#define PIO_IFSCSR_P26_Msk                  (0x1U << PIO_IFSCSR_P26_Pos)                   /**< (PIO_IFSCSR) Glitch or Debouncing Filter Selection Status Mask */
#define PIO_IFSCSR_P26                      PIO_IFSCSR_P26_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_IFSCSR_P26_Msk instead */
#define PIO_IFSCSR_P27_Pos                  27                                             /**< (PIO_IFSCSR) Glitch or Debouncing Filter Selection Status Position */
#define PIO_IFSCSR_P27_Msk                  (0x1U << PIO_IFSCSR_P27_Pos)                   /**< (PIO_IFSCSR) Glitch or Debouncing Filter Selection Status Mask */
#define PIO_IFSCSR_P27                      PIO_IFSCSR_P27_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_IFSCSR_P27_Msk instead */
#define PIO_IFSCSR_P28_Pos                  28                                             /**< (PIO_IFSCSR) Glitch or Debouncing Filter Selection Status Position */
#define PIO_IFSCSR_P28_Msk                  (0x1U << PIO_IFSCSR_P28_Pos)                   /**< (PIO_IFSCSR) Glitch or Debouncing Filter Selection Status Mask */
#define PIO_IFSCSR_P28                      PIO_IFSCSR_P28_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_IFSCSR_P28_Msk instead */
#define PIO_IFSCSR_P29_Pos                  29                                             /**< (PIO_IFSCSR) Glitch or Debouncing Filter Selection Status Position */
#define PIO_IFSCSR_P29_Msk                  (0x1U << PIO_IFSCSR_P29_Pos)                   /**< (PIO_IFSCSR) Glitch or Debouncing Filter Selection Status Mask */
#define PIO_IFSCSR_P29                      PIO_IFSCSR_P29_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_IFSCSR_P29_Msk instead */
#define PIO_IFSCSR_P30_Pos                  30                                             /**< (PIO_IFSCSR) Glitch or Debouncing Filter Selection Status Position */
#define PIO_IFSCSR_P30_Msk                  (0x1U << PIO_IFSCSR_P30_Pos)                   /**< (PIO_IFSCSR) Glitch or Debouncing Filter Selection Status Mask */
#define PIO_IFSCSR_P30                      PIO_IFSCSR_P30_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_IFSCSR_P30_Msk instead */
#define PIO_IFSCSR_P31_Pos                  31                                             /**< (PIO_IFSCSR) Glitch or Debouncing Filter Selection Status Position */
#define PIO_IFSCSR_P31_Msk                  (0x1U << PIO_IFSCSR_P31_Pos)                   /**< (PIO_IFSCSR) Glitch or Debouncing Filter Selection Status Mask */
#define PIO_IFSCSR_P31                      PIO_IFSCSR_P31_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_IFSCSR_P31_Msk instead */
#define PIO_IFSCSR_P_Pos                    0                                              /**< (PIO_IFSCSR Position) Glitch or Debouncing Filter Selection Status */
#define PIO_IFSCSR_P_Msk                    (0xFFFFFFFFU << PIO_IFSCSR_P_Pos)              /**< (PIO_IFSCSR Mask) P */
#define PIO_IFSCSR_P(value)                 (PIO_IFSCSR_P_Msk & ((value) << PIO_IFSCSR_P_Pos))  
#define PIO_IFSCSR_MASK                     (0xFFFFFFFFU)                                  /**< \deprecated (PIO_IFSCSR) Register MASK  (Use PIO_IFSCSR_Msk instead)  */
#define PIO_IFSCSR_Msk                      (0xFFFFFFFFU)                                  /**< (PIO_IFSCSR) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- PIO_SCDR : (PIO Offset: 0x8c) (R/W 32) Slow Clock Divider Debouncing Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t DIV:14;                    /**< bit:  0..13  Slow Clock Divider Selection for Debouncing */
    uint32_t :18;                       /**< bit: 14..31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} PIO_SCDR_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define PIO_SCDR_OFFSET                     (0x8C)                                        /**<  (PIO_SCDR) Slow Clock Divider Debouncing Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define PIO_SCDR_DIV_Pos                    0                                              /**< (PIO_SCDR) Slow Clock Divider Selection for Debouncing Position */
#define PIO_SCDR_DIV_Msk                    (0x3FFFU << PIO_SCDR_DIV_Pos)                  /**< (PIO_SCDR) Slow Clock Divider Selection for Debouncing Mask */
#define PIO_SCDR_DIV(value)                 (PIO_SCDR_DIV_Msk & ((value) << PIO_SCDR_DIV_Pos))
#define PIO_SCDR_MASK                       (0x3FFFU)                                      /**< \deprecated (PIO_SCDR) Register MASK  (Use PIO_SCDR_Msk instead)  */
#define PIO_SCDR_Msk                        (0x3FFFU)                                      /**< (PIO_SCDR) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- PIO_PPDDR : (PIO Offset: 0x90) (/W 32) Pad Pull-down Disable Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t P0:1;                      /**< bit:      0  Pull-Down Disable                        */
    uint32_t P1:1;                      /**< bit:      1  Pull-Down Disable                        */
    uint32_t P2:1;                      /**< bit:      2  Pull-Down Disable                        */
    uint32_t P3:1;                      /**< bit:      3  Pull-Down Disable                        */
    uint32_t P4:1;                      /**< bit:      4  Pull-Down Disable                        */
    uint32_t P5:1;                      /**< bit:      5  Pull-Down Disable                        */
    uint32_t P6:1;                      /**< bit:      6  Pull-Down Disable                        */
    uint32_t P7:1;                      /**< bit:      7  Pull-Down Disable                        */
    uint32_t P8:1;                      /**< bit:      8  Pull-Down Disable                        */
    uint32_t P9:1;                      /**< bit:      9  Pull-Down Disable                        */
    uint32_t P10:1;                     /**< bit:     10  Pull-Down Disable                        */
    uint32_t P11:1;                     /**< bit:     11  Pull-Down Disable                        */
    uint32_t P12:1;                     /**< bit:     12  Pull-Down Disable                        */
    uint32_t P13:1;                     /**< bit:     13  Pull-Down Disable                        */
    uint32_t P14:1;                     /**< bit:     14  Pull-Down Disable                        */
    uint32_t P15:1;                     /**< bit:     15  Pull-Down Disable                        */
    uint32_t P16:1;                     /**< bit:     16  Pull-Down Disable                        */
    uint32_t P17:1;                     /**< bit:     17  Pull-Down Disable                        */
    uint32_t P18:1;                     /**< bit:     18  Pull-Down Disable                        */
    uint32_t P19:1;                     /**< bit:     19  Pull-Down Disable                        */
    uint32_t P20:1;                     /**< bit:     20  Pull-Down Disable                        */
    uint32_t P21:1;                     /**< bit:     21  Pull-Down Disable                        */
    uint32_t P22:1;                     /**< bit:     22  Pull-Down Disable                        */
    uint32_t P23:1;                     /**< bit:     23  Pull-Down Disable                        */
    uint32_t P24:1;                     /**< bit:     24  Pull-Down Disable                        */
    uint32_t P25:1;                     /**< bit:     25  Pull-Down Disable                        */
    uint32_t P26:1;                     /**< bit:     26  Pull-Down Disable                        */
    uint32_t P27:1;                     /**< bit:     27  Pull-Down Disable                        */
    uint32_t P28:1;                     /**< bit:     28  Pull-Down Disable                        */
    uint32_t P29:1;                     /**< bit:     29  Pull-Down Disable                        */
    uint32_t P30:1;                     /**< bit:     30  Pull-Down Disable                        */
    uint32_t P31:1;                     /**< bit:     31  Pull-Down Disable                        */
  } bit;                                /**< Structure used for bit  access */
  struct {
    uint32_t P:32;                      /**< bit:  0..31  Pull-Down Disable                        */
  } vec;                                /**< Structure used for vec  access  */
  uint32_t reg;                         /**< Type used for register access */
} PIO_PPDDR_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define PIO_PPDDR_OFFSET                    (0x90)                                        /**<  (PIO_PPDDR) Pad Pull-down Disable Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define PIO_PPDDR_P0_Pos                    0                                              /**< (PIO_PPDDR) Pull-Down Disable Position */
#define PIO_PPDDR_P0_Msk                    (0x1U << PIO_PPDDR_P0_Pos)                     /**< (PIO_PPDDR) Pull-Down Disable Mask */
#define PIO_PPDDR_P0                        PIO_PPDDR_P0_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PPDDR_P0_Msk instead */
#define PIO_PPDDR_P1_Pos                    1                                              /**< (PIO_PPDDR) Pull-Down Disable Position */
#define PIO_PPDDR_P1_Msk                    (0x1U << PIO_PPDDR_P1_Pos)                     /**< (PIO_PPDDR) Pull-Down Disable Mask */
#define PIO_PPDDR_P1                        PIO_PPDDR_P1_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PPDDR_P1_Msk instead */
#define PIO_PPDDR_P2_Pos                    2                                              /**< (PIO_PPDDR) Pull-Down Disable Position */
#define PIO_PPDDR_P2_Msk                    (0x1U << PIO_PPDDR_P2_Pos)                     /**< (PIO_PPDDR) Pull-Down Disable Mask */
#define PIO_PPDDR_P2                        PIO_PPDDR_P2_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PPDDR_P2_Msk instead */
#define PIO_PPDDR_P3_Pos                    3                                              /**< (PIO_PPDDR) Pull-Down Disable Position */
#define PIO_PPDDR_P3_Msk                    (0x1U << PIO_PPDDR_P3_Pos)                     /**< (PIO_PPDDR) Pull-Down Disable Mask */
#define PIO_PPDDR_P3                        PIO_PPDDR_P3_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PPDDR_P3_Msk instead */
#define PIO_PPDDR_P4_Pos                    4                                              /**< (PIO_PPDDR) Pull-Down Disable Position */
#define PIO_PPDDR_P4_Msk                    (0x1U << PIO_PPDDR_P4_Pos)                     /**< (PIO_PPDDR) Pull-Down Disable Mask */
#define PIO_PPDDR_P4                        PIO_PPDDR_P4_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PPDDR_P4_Msk instead */
#define PIO_PPDDR_P5_Pos                    5                                              /**< (PIO_PPDDR) Pull-Down Disable Position */
#define PIO_PPDDR_P5_Msk                    (0x1U << PIO_PPDDR_P5_Pos)                     /**< (PIO_PPDDR) Pull-Down Disable Mask */
#define PIO_PPDDR_P5                        PIO_PPDDR_P5_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PPDDR_P5_Msk instead */
#define PIO_PPDDR_P6_Pos                    6                                              /**< (PIO_PPDDR) Pull-Down Disable Position */
#define PIO_PPDDR_P6_Msk                    (0x1U << PIO_PPDDR_P6_Pos)                     /**< (PIO_PPDDR) Pull-Down Disable Mask */
#define PIO_PPDDR_P6                        PIO_PPDDR_P6_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PPDDR_P6_Msk instead */
#define PIO_PPDDR_P7_Pos                    7                                              /**< (PIO_PPDDR) Pull-Down Disable Position */
#define PIO_PPDDR_P7_Msk                    (0x1U << PIO_PPDDR_P7_Pos)                     /**< (PIO_PPDDR) Pull-Down Disable Mask */
#define PIO_PPDDR_P7                        PIO_PPDDR_P7_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PPDDR_P7_Msk instead */
#define PIO_PPDDR_P8_Pos                    8                                              /**< (PIO_PPDDR) Pull-Down Disable Position */
#define PIO_PPDDR_P8_Msk                    (0x1U << PIO_PPDDR_P8_Pos)                     /**< (PIO_PPDDR) Pull-Down Disable Mask */
#define PIO_PPDDR_P8                        PIO_PPDDR_P8_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PPDDR_P8_Msk instead */
#define PIO_PPDDR_P9_Pos                    9                                              /**< (PIO_PPDDR) Pull-Down Disable Position */
#define PIO_PPDDR_P9_Msk                    (0x1U << PIO_PPDDR_P9_Pos)                     /**< (PIO_PPDDR) Pull-Down Disable Mask */
#define PIO_PPDDR_P9                        PIO_PPDDR_P9_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PPDDR_P9_Msk instead */
#define PIO_PPDDR_P10_Pos                   10                                             /**< (PIO_PPDDR) Pull-Down Disable Position */
#define PIO_PPDDR_P10_Msk                   (0x1U << PIO_PPDDR_P10_Pos)                    /**< (PIO_PPDDR) Pull-Down Disable Mask */
#define PIO_PPDDR_P10                       PIO_PPDDR_P10_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PPDDR_P10_Msk instead */
#define PIO_PPDDR_P11_Pos                   11                                             /**< (PIO_PPDDR) Pull-Down Disable Position */
#define PIO_PPDDR_P11_Msk                   (0x1U << PIO_PPDDR_P11_Pos)                    /**< (PIO_PPDDR) Pull-Down Disable Mask */
#define PIO_PPDDR_P11                       PIO_PPDDR_P11_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PPDDR_P11_Msk instead */
#define PIO_PPDDR_P12_Pos                   12                                             /**< (PIO_PPDDR) Pull-Down Disable Position */
#define PIO_PPDDR_P12_Msk                   (0x1U << PIO_PPDDR_P12_Pos)                    /**< (PIO_PPDDR) Pull-Down Disable Mask */
#define PIO_PPDDR_P12                       PIO_PPDDR_P12_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PPDDR_P12_Msk instead */
#define PIO_PPDDR_P13_Pos                   13                                             /**< (PIO_PPDDR) Pull-Down Disable Position */
#define PIO_PPDDR_P13_Msk                   (0x1U << PIO_PPDDR_P13_Pos)                    /**< (PIO_PPDDR) Pull-Down Disable Mask */
#define PIO_PPDDR_P13                       PIO_PPDDR_P13_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PPDDR_P13_Msk instead */
#define PIO_PPDDR_P14_Pos                   14                                             /**< (PIO_PPDDR) Pull-Down Disable Position */
#define PIO_PPDDR_P14_Msk                   (0x1U << PIO_PPDDR_P14_Pos)                    /**< (PIO_PPDDR) Pull-Down Disable Mask */
#define PIO_PPDDR_P14                       PIO_PPDDR_P14_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PPDDR_P14_Msk instead */
#define PIO_PPDDR_P15_Pos                   15                                             /**< (PIO_PPDDR) Pull-Down Disable Position */
#define PIO_PPDDR_P15_Msk                   (0x1U << PIO_PPDDR_P15_Pos)                    /**< (PIO_PPDDR) Pull-Down Disable Mask */
#define PIO_PPDDR_P15                       PIO_PPDDR_P15_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PPDDR_P15_Msk instead */
#define PIO_PPDDR_P16_Pos                   16                                             /**< (PIO_PPDDR) Pull-Down Disable Position */
#define PIO_PPDDR_P16_Msk                   (0x1U << PIO_PPDDR_P16_Pos)                    /**< (PIO_PPDDR) Pull-Down Disable Mask */
#define PIO_PPDDR_P16                       PIO_PPDDR_P16_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PPDDR_P16_Msk instead */
#define PIO_PPDDR_P17_Pos                   17                                             /**< (PIO_PPDDR) Pull-Down Disable Position */
#define PIO_PPDDR_P17_Msk                   (0x1U << PIO_PPDDR_P17_Pos)                    /**< (PIO_PPDDR) Pull-Down Disable Mask */
#define PIO_PPDDR_P17                       PIO_PPDDR_P17_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PPDDR_P17_Msk instead */
#define PIO_PPDDR_P18_Pos                   18                                             /**< (PIO_PPDDR) Pull-Down Disable Position */
#define PIO_PPDDR_P18_Msk                   (0x1U << PIO_PPDDR_P18_Pos)                    /**< (PIO_PPDDR) Pull-Down Disable Mask */
#define PIO_PPDDR_P18                       PIO_PPDDR_P18_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PPDDR_P18_Msk instead */
#define PIO_PPDDR_P19_Pos                   19                                             /**< (PIO_PPDDR) Pull-Down Disable Position */
#define PIO_PPDDR_P19_Msk                   (0x1U << PIO_PPDDR_P19_Pos)                    /**< (PIO_PPDDR) Pull-Down Disable Mask */
#define PIO_PPDDR_P19                       PIO_PPDDR_P19_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PPDDR_P19_Msk instead */
#define PIO_PPDDR_P20_Pos                   20                                             /**< (PIO_PPDDR) Pull-Down Disable Position */
#define PIO_PPDDR_P20_Msk                   (0x1U << PIO_PPDDR_P20_Pos)                    /**< (PIO_PPDDR) Pull-Down Disable Mask */
#define PIO_PPDDR_P20                       PIO_PPDDR_P20_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PPDDR_P20_Msk instead */
#define PIO_PPDDR_P21_Pos                   21                                             /**< (PIO_PPDDR) Pull-Down Disable Position */
#define PIO_PPDDR_P21_Msk                   (0x1U << PIO_PPDDR_P21_Pos)                    /**< (PIO_PPDDR) Pull-Down Disable Mask */
#define PIO_PPDDR_P21                       PIO_PPDDR_P21_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PPDDR_P21_Msk instead */
#define PIO_PPDDR_P22_Pos                   22                                             /**< (PIO_PPDDR) Pull-Down Disable Position */
#define PIO_PPDDR_P22_Msk                   (0x1U << PIO_PPDDR_P22_Pos)                    /**< (PIO_PPDDR) Pull-Down Disable Mask */
#define PIO_PPDDR_P22                       PIO_PPDDR_P22_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PPDDR_P22_Msk instead */
#define PIO_PPDDR_P23_Pos                   23                                             /**< (PIO_PPDDR) Pull-Down Disable Position */
#define PIO_PPDDR_P23_Msk                   (0x1U << PIO_PPDDR_P23_Pos)                    /**< (PIO_PPDDR) Pull-Down Disable Mask */
#define PIO_PPDDR_P23                       PIO_PPDDR_P23_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PPDDR_P23_Msk instead */
#define PIO_PPDDR_P24_Pos                   24                                             /**< (PIO_PPDDR) Pull-Down Disable Position */
#define PIO_PPDDR_P24_Msk                   (0x1U << PIO_PPDDR_P24_Pos)                    /**< (PIO_PPDDR) Pull-Down Disable Mask */
#define PIO_PPDDR_P24                       PIO_PPDDR_P24_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PPDDR_P24_Msk instead */
#define PIO_PPDDR_P25_Pos                   25                                             /**< (PIO_PPDDR) Pull-Down Disable Position */
#define PIO_PPDDR_P25_Msk                   (0x1U << PIO_PPDDR_P25_Pos)                    /**< (PIO_PPDDR) Pull-Down Disable Mask */
#define PIO_PPDDR_P25                       PIO_PPDDR_P25_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PPDDR_P25_Msk instead */
#define PIO_PPDDR_P26_Pos                   26                                             /**< (PIO_PPDDR) Pull-Down Disable Position */
#define PIO_PPDDR_P26_Msk                   (0x1U << PIO_PPDDR_P26_Pos)                    /**< (PIO_PPDDR) Pull-Down Disable Mask */
#define PIO_PPDDR_P26                       PIO_PPDDR_P26_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PPDDR_P26_Msk instead */
#define PIO_PPDDR_P27_Pos                   27                                             /**< (PIO_PPDDR) Pull-Down Disable Position */
#define PIO_PPDDR_P27_Msk                   (0x1U << PIO_PPDDR_P27_Pos)                    /**< (PIO_PPDDR) Pull-Down Disable Mask */
#define PIO_PPDDR_P27                       PIO_PPDDR_P27_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PPDDR_P27_Msk instead */
#define PIO_PPDDR_P28_Pos                   28                                             /**< (PIO_PPDDR) Pull-Down Disable Position */
#define PIO_PPDDR_P28_Msk                   (0x1U << PIO_PPDDR_P28_Pos)                    /**< (PIO_PPDDR) Pull-Down Disable Mask */
#define PIO_PPDDR_P28                       PIO_PPDDR_P28_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PPDDR_P28_Msk instead */
#define PIO_PPDDR_P29_Pos                   29                                             /**< (PIO_PPDDR) Pull-Down Disable Position */
#define PIO_PPDDR_P29_Msk                   (0x1U << PIO_PPDDR_P29_Pos)                    /**< (PIO_PPDDR) Pull-Down Disable Mask */
#define PIO_PPDDR_P29                       PIO_PPDDR_P29_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PPDDR_P29_Msk instead */
#define PIO_PPDDR_P30_Pos                   30                                             /**< (PIO_PPDDR) Pull-Down Disable Position */
#define PIO_PPDDR_P30_Msk                   (0x1U << PIO_PPDDR_P30_Pos)                    /**< (PIO_PPDDR) Pull-Down Disable Mask */
#define PIO_PPDDR_P30                       PIO_PPDDR_P30_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PPDDR_P30_Msk instead */
#define PIO_PPDDR_P31_Pos                   31                                             /**< (PIO_PPDDR) Pull-Down Disable Position */
#define PIO_PPDDR_P31_Msk                   (0x1U << PIO_PPDDR_P31_Pos)                    /**< (PIO_PPDDR) Pull-Down Disable Mask */
#define PIO_PPDDR_P31                       PIO_PPDDR_P31_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PPDDR_P31_Msk instead */
#define PIO_PPDDR_P_Pos                     0                                              /**< (PIO_PPDDR Position) Pull-Down Disable */
#define PIO_PPDDR_P_Msk                     (0xFFFFFFFFU << PIO_PPDDR_P_Pos)               /**< (PIO_PPDDR Mask) P */
#define PIO_PPDDR_P(value)                  (PIO_PPDDR_P_Msk & ((value) << PIO_PPDDR_P_Pos))  
#define PIO_PPDDR_MASK                      (0xFFFFFFFFU)                                  /**< \deprecated (PIO_PPDDR) Register MASK  (Use PIO_PPDDR_Msk instead)  */
#define PIO_PPDDR_Msk                       (0xFFFFFFFFU)                                  /**< (PIO_PPDDR) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- PIO_PPDER : (PIO Offset: 0x94) (/W 32) Pad Pull-down Enable Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t P0:1;                      /**< bit:      0  Pull-Down Enable                         */
    uint32_t P1:1;                      /**< bit:      1  Pull-Down Enable                         */
    uint32_t P2:1;                      /**< bit:      2  Pull-Down Enable                         */
    uint32_t P3:1;                      /**< bit:      3  Pull-Down Enable                         */
    uint32_t P4:1;                      /**< bit:      4  Pull-Down Enable                         */
    uint32_t P5:1;                      /**< bit:      5  Pull-Down Enable                         */
    uint32_t P6:1;                      /**< bit:      6  Pull-Down Enable                         */
    uint32_t P7:1;                      /**< bit:      7  Pull-Down Enable                         */
    uint32_t P8:1;                      /**< bit:      8  Pull-Down Enable                         */
    uint32_t P9:1;                      /**< bit:      9  Pull-Down Enable                         */
    uint32_t P10:1;                     /**< bit:     10  Pull-Down Enable                         */
    uint32_t P11:1;                     /**< bit:     11  Pull-Down Enable                         */
    uint32_t P12:1;                     /**< bit:     12  Pull-Down Enable                         */
    uint32_t P13:1;                     /**< bit:     13  Pull-Down Enable                         */
    uint32_t P14:1;                     /**< bit:     14  Pull-Down Enable                         */
    uint32_t P15:1;                     /**< bit:     15  Pull-Down Enable                         */
    uint32_t P16:1;                     /**< bit:     16  Pull-Down Enable                         */
    uint32_t P17:1;                     /**< bit:     17  Pull-Down Enable                         */
    uint32_t P18:1;                     /**< bit:     18  Pull-Down Enable                         */
    uint32_t P19:1;                     /**< bit:     19  Pull-Down Enable                         */
    uint32_t P20:1;                     /**< bit:     20  Pull-Down Enable                         */
    uint32_t P21:1;                     /**< bit:     21  Pull-Down Enable                         */
    uint32_t P22:1;                     /**< bit:     22  Pull-Down Enable                         */
    uint32_t P23:1;                     /**< bit:     23  Pull-Down Enable                         */
    uint32_t P24:1;                     /**< bit:     24  Pull-Down Enable                         */
    uint32_t P25:1;                     /**< bit:     25  Pull-Down Enable                         */
    uint32_t P26:1;                     /**< bit:     26  Pull-Down Enable                         */
    uint32_t P27:1;                     /**< bit:     27  Pull-Down Enable                         */
    uint32_t P28:1;                     /**< bit:     28  Pull-Down Enable                         */
    uint32_t P29:1;                     /**< bit:     29  Pull-Down Enable                         */
    uint32_t P30:1;                     /**< bit:     30  Pull-Down Enable                         */
    uint32_t P31:1;                     /**< bit:     31  Pull-Down Enable                         */
  } bit;                                /**< Structure used for bit  access */
  struct {
    uint32_t P:32;                      /**< bit:  0..31  Pull-Down Enable                         */
  } vec;                                /**< Structure used for vec  access  */
  uint32_t reg;                         /**< Type used for register access */
} PIO_PPDER_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define PIO_PPDER_OFFSET                    (0x94)                                        /**<  (PIO_PPDER) Pad Pull-down Enable Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define PIO_PPDER_P0_Pos                    0                                              /**< (PIO_PPDER) Pull-Down Enable Position */
#define PIO_PPDER_P0_Msk                    (0x1U << PIO_PPDER_P0_Pos)                     /**< (PIO_PPDER) Pull-Down Enable Mask */
#define PIO_PPDER_P0                        PIO_PPDER_P0_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PPDER_P0_Msk instead */
#define PIO_PPDER_P1_Pos                    1                                              /**< (PIO_PPDER) Pull-Down Enable Position */
#define PIO_PPDER_P1_Msk                    (0x1U << PIO_PPDER_P1_Pos)                     /**< (PIO_PPDER) Pull-Down Enable Mask */
#define PIO_PPDER_P1                        PIO_PPDER_P1_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PPDER_P1_Msk instead */
#define PIO_PPDER_P2_Pos                    2                                              /**< (PIO_PPDER) Pull-Down Enable Position */
#define PIO_PPDER_P2_Msk                    (0x1U << PIO_PPDER_P2_Pos)                     /**< (PIO_PPDER) Pull-Down Enable Mask */
#define PIO_PPDER_P2                        PIO_PPDER_P2_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PPDER_P2_Msk instead */
#define PIO_PPDER_P3_Pos                    3                                              /**< (PIO_PPDER) Pull-Down Enable Position */
#define PIO_PPDER_P3_Msk                    (0x1U << PIO_PPDER_P3_Pos)                     /**< (PIO_PPDER) Pull-Down Enable Mask */
#define PIO_PPDER_P3                        PIO_PPDER_P3_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PPDER_P3_Msk instead */
#define PIO_PPDER_P4_Pos                    4                                              /**< (PIO_PPDER) Pull-Down Enable Position */
#define PIO_PPDER_P4_Msk                    (0x1U << PIO_PPDER_P4_Pos)                     /**< (PIO_PPDER) Pull-Down Enable Mask */
#define PIO_PPDER_P4                        PIO_PPDER_P4_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PPDER_P4_Msk instead */
#define PIO_PPDER_P5_Pos                    5                                              /**< (PIO_PPDER) Pull-Down Enable Position */
#define PIO_PPDER_P5_Msk                    (0x1U << PIO_PPDER_P5_Pos)                     /**< (PIO_PPDER) Pull-Down Enable Mask */
#define PIO_PPDER_P5                        PIO_PPDER_P5_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PPDER_P5_Msk instead */
#define PIO_PPDER_P6_Pos                    6                                              /**< (PIO_PPDER) Pull-Down Enable Position */
#define PIO_PPDER_P6_Msk                    (0x1U << PIO_PPDER_P6_Pos)                     /**< (PIO_PPDER) Pull-Down Enable Mask */
#define PIO_PPDER_P6                        PIO_PPDER_P6_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PPDER_P6_Msk instead */
#define PIO_PPDER_P7_Pos                    7                                              /**< (PIO_PPDER) Pull-Down Enable Position */
#define PIO_PPDER_P7_Msk                    (0x1U << PIO_PPDER_P7_Pos)                     /**< (PIO_PPDER) Pull-Down Enable Mask */
#define PIO_PPDER_P7                        PIO_PPDER_P7_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PPDER_P7_Msk instead */
#define PIO_PPDER_P8_Pos                    8                                              /**< (PIO_PPDER) Pull-Down Enable Position */
#define PIO_PPDER_P8_Msk                    (0x1U << PIO_PPDER_P8_Pos)                     /**< (PIO_PPDER) Pull-Down Enable Mask */
#define PIO_PPDER_P8                        PIO_PPDER_P8_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PPDER_P8_Msk instead */
#define PIO_PPDER_P9_Pos                    9                                              /**< (PIO_PPDER) Pull-Down Enable Position */
#define PIO_PPDER_P9_Msk                    (0x1U << PIO_PPDER_P9_Pos)                     /**< (PIO_PPDER) Pull-Down Enable Mask */
#define PIO_PPDER_P9                        PIO_PPDER_P9_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PPDER_P9_Msk instead */
#define PIO_PPDER_P10_Pos                   10                                             /**< (PIO_PPDER) Pull-Down Enable Position */
#define PIO_PPDER_P10_Msk                   (0x1U << PIO_PPDER_P10_Pos)                    /**< (PIO_PPDER) Pull-Down Enable Mask */
#define PIO_PPDER_P10                       PIO_PPDER_P10_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PPDER_P10_Msk instead */
#define PIO_PPDER_P11_Pos                   11                                             /**< (PIO_PPDER) Pull-Down Enable Position */
#define PIO_PPDER_P11_Msk                   (0x1U << PIO_PPDER_P11_Pos)                    /**< (PIO_PPDER) Pull-Down Enable Mask */
#define PIO_PPDER_P11                       PIO_PPDER_P11_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PPDER_P11_Msk instead */
#define PIO_PPDER_P12_Pos                   12                                             /**< (PIO_PPDER) Pull-Down Enable Position */
#define PIO_PPDER_P12_Msk                   (0x1U << PIO_PPDER_P12_Pos)                    /**< (PIO_PPDER) Pull-Down Enable Mask */
#define PIO_PPDER_P12                       PIO_PPDER_P12_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PPDER_P12_Msk instead */
#define PIO_PPDER_P13_Pos                   13                                             /**< (PIO_PPDER) Pull-Down Enable Position */
#define PIO_PPDER_P13_Msk                   (0x1U << PIO_PPDER_P13_Pos)                    /**< (PIO_PPDER) Pull-Down Enable Mask */
#define PIO_PPDER_P13                       PIO_PPDER_P13_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PPDER_P13_Msk instead */
#define PIO_PPDER_P14_Pos                   14                                             /**< (PIO_PPDER) Pull-Down Enable Position */
#define PIO_PPDER_P14_Msk                   (0x1U << PIO_PPDER_P14_Pos)                    /**< (PIO_PPDER) Pull-Down Enable Mask */
#define PIO_PPDER_P14                       PIO_PPDER_P14_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PPDER_P14_Msk instead */
#define PIO_PPDER_P15_Pos                   15                                             /**< (PIO_PPDER) Pull-Down Enable Position */
#define PIO_PPDER_P15_Msk                   (0x1U << PIO_PPDER_P15_Pos)                    /**< (PIO_PPDER) Pull-Down Enable Mask */
#define PIO_PPDER_P15                       PIO_PPDER_P15_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PPDER_P15_Msk instead */
#define PIO_PPDER_P16_Pos                   16                                             /**< (PIO_PPDER) Pull-Down Enable Position */
#define PIO_PPDER_P16_Msk                   (0x1U << PIO_PPDER_P16_Pos)                    /**< (PIO_PPDER) Pull-Down Enable Mask */
#define PIO_PPDER_P16                       PIO_PPDER_P16_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PPDER_P16_Msk instead */
#define PIO_PPDER_P17_Pos                   17                                             /**< (PIO_PPDER) Pull-Down Enable Position */
#define PIO_PPDER_P17_Msk                   (0x1U << PIO_PPDER_P17_Pos)                    /**< (PIO_PPDER) Pull-Down Enable Mask */
#define PIO_PPDER_P17                       PIO_PPDER_P17_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PPDER_P17_Msk instead */
#define PIO_PPDER_P18_Pos                   18                                             /**< (PIO_PPDER) Pull-Down Enable Position */
#define PIO_PPDER_P18_Msk                   (0x1U << PIO_PPDER_P18_Pos)                    /**< (PIO_PPDER) Pull-Down Enable Mask */
#define PIO_PPDER_P18                       PIO_PPDER_P18_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PPDER_P18_Msk instead */
#define PIO_PPDER_P19_Pos                   19                                             /**< (PIO_PPDER) Pull-Down Enable Position */
#define PIO_PPDER_P19_Msk                   (0x1U << PIO_PPDER_P19_Pos)                    /**< (PIO_PPDER) Pull-Down Enable Mask */
#define PIO_PPDER_P19                       PIO_PPDER_P19_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PPDER_P19_Msk instead */
#define PIO_PPDER_P20_Pos                   20                                             /**< (PIO_PPDER) Pull-Down Enable Position */
#define PIO_PPDER_P20_Msk                   (0x1U << PIO_PPDER_P20_Pos)                    /**< (PIO_PPDER) Pull-Down Enable Mask */
#define PIO_PPDER_P20                       PIO_PPDER_P20_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PPDER_P20_Msk instead */
#define PIO_PPDER_P21_Pos                   21                                             /**< (PIO_PPDER) Pull-Down Enable Position */
#define PIO_PPDER_P21_Msk                   (0x1U << PIO_PPDER_P21_Pos)                    /**< (PIO_PPDER) Pull-Down Enable Mask */
#define PIO_PPDER_P21                       PIO_PPDER_P21_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PPDER_P21_Msk instead */
#define PIO_PPDER_P22_Pos                   22                                             /**< (PIO_PPDER) Pull-Down Enable Position */
#define PIO_PPDER_P22_Msk                   (0x1U << PIO_PPDER_P22_Pos)                    /**< (PIO_PPDER) Pull-Down Enable Mask */
#define PIO_PPDER_P22                       PIO_PPDER_P22_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PPDER_P22_Msk instead */
#define PIO_PPDER_P23_Pos                   23                                             /**< (PIO_PPDER) Pull-Down Enable Position */
#define PIO_PPDER_P23_Msk                   (0x1U << PIO_PPDER_P23_Pos)                    /**< (PIO_PPDER) Pull-Down Enable Mask */
#define PIO_PPDER_P23                       PIO_PPDER_P23_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PPDER_P23_Msk instead */
#define PIO_PPDER_P24_Pos                   24                                             /**< (PIO_PPDER) Pull-Down Enable Position */
#define PIO_PPDER_P24_Msk                   (0x1U << PIO_PPDER_P24_Pos)                    /**< (PIO_PPDER) Pull-Down Enable Mask */
#define PIO_PPDER_P24                       PIO_PPDER_P24_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PPDER_P24_Msk instead */
#define PIO_PPDER_P25_Pos                   25                                             /**< (PIO_PPDER) Pull-Down Enable Position */
#define PIO_PPDER_P25_Msk                   (0x1U << PIO_PPDER_P25_Pos)                    /**< (PIO_PPDER) Pull-Down Enable Mask */
#define PIO_PPDER_P25                       PIO_PPDER_P25_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PPDER_P25_Msk instead */
#define PIO_PPDER_P26_Pos                   26                                             /**< (PIO_PPDER) Pull-Down Enable Position */
#define PIO_PPDER_P26_Msk                   (0x1U << PIO_PPDER_P26_Pos)                    /**< (PIO_PPDER) Pull-Down Enable Mask */
#define PIO_PPDER_P26                       PIO_PPDER_P26_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PPDER_P26_Msk instead */
#define PIO_PPDER_P27_Pos                   27                                             /**< (PIO_PPDER) Pull-Down Enable Position */
#define PIO_PPDER_P27_Msk                   (0x1U << PIO_PPDER_P27_Pos)                    /**< (PIO_PPDER) Pull-Down Enable Mask */
#define PIO_PPDER_P27                       PIO_PPDER_P27_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PPDER_P27_Msk instead */
#define PIO_PPDER_P28_Pos                   28                                             /**< (PIO_PPDER) Pull-Down Enable Position */
#define PIO_PPDER_P28_Msk                   (0x1U << PIO_PPDER_P28_Pos)                    /**< (PIO_PPDER) Pull-Down Enable Mask */
#define PIO_PPDER_P28                       PIO_PPDER_P28_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PPDER_P28_Msk instead */
#define PIO_PPDER_P29_Pos                   29                                             /**< (PIO_PPDER) Pull-Down Enable Position */
#define PIO_PPDER_P29_Msk                   (0x1U << PIO_PPDER_P29_Pos)                    /**< (PIO_PPDER) Pull-Down Enable Mask */
#define PIO_PPDER_P29                       PIO_PPDER_P29_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PPDER_P29_Msk instead */
#define PIO_PPDER_P30_Pos                   30                                             /**< (PIO_PPDER) Pull-Down Enable Position */
#define PIO_PPDER_P30_Msk                   (0x1U << PIO_PPDER_P30_Pos)                    /**< (PIO_PPDER) Pull-Down Enable Mask */
#define PIO_PPDER_P30                       PIO_PPDER_P30_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PPDER_P30_Msk instead */
#define PIO_PPDER_P31_Pos                   31                                             /**< (PIO_PPDER) Pull-Down Enable Position */
#define PIO_PPDER_P31_Msk                   (0x1U << PIO_PPDER_P31_Pos)                    /**< (PIO_PPDER) Pull-Down Enable Mask */
#define PIO_PPDER_P31                       PIO_PPDER_P31_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PPDER_P31_Msk instead */
#define PIO_PPDER_P_Pos                     0                                              /**< (PIO_PPDER Position) Pull-Down Enable */
#define PIO_PPDER_P_Msk                     (0xFFFFFFFFU << PIO_PPDER_P_Pos)               /**< (PIO_PPDER Mask) P */
#define PIO_PPDER_P(value)                  (PIO_PPDER_P_Msk & ((value) << PIO_PPDER_P_Pos))  
#define PIO_PPDER_MASK                      (0xFFFFFFFFU)                                  /**< \deprecated (PIO_PPDER) Register MASK  (Use PIO_PPDER_Msk instead)  */
#define PIO_PPDER_Msk                       (0xFFFFFFFFU)                                  /**< (PIO_PPDER) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- PIO_PPDSR : (PIO Offset: 0x98) (R/ 32) Pad Pull-down Status Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t P0:1;                      /**< bit:      0  Pull-Down Status                         */
    uint32_t P1:1;                      /**< bit:      1  Pull-Down Status                         */
    uint32_t P2:1;                      /**< bit:      2  Pull-Down Status                         */
    uint32_t P3:1;                      /**< bit:      3  Pull-Down Status                         */
    uint32_t P4:1;                      /**< bit:      4  Pull-Down Status                         */
    uint32_t P5:1;                      /**< bit:      5  Pull-Down Status                         */
    uint32_t P6:1;                      /**< bit:      6  Pull-Down Status                         */
    uint32_t P7:1;                      /**< bit:      7  Pull-Down Status                         */
    uint32_t P8:1;                      /**< bit:      8  Pull-Down Status                         */
    uint32_t P9:1;                      /**< bit:      9  Pull-Down Status                         */
    uint32_t P10:1;                     /**< bit:     10  Pull-Down Status                         */
    uint32_t P11:1;                     /**< bit:     11  Pull-Down Status                         */
    uint32_t P12:1;                     /**< bit:     12  Pull-Down Status                         */
    uint32_t P13:1;                     /**< bit:     13  Pull-Down Status                         */
    uint32_t P14:1;                     /**< bit:     14  Pull-Down Status                         */
    uint32_t P15:1;                     /**< bit:     15  Pull-Down Status                         */
    uint32_t P16:1;                     /**< bit:     16  Pull-Down Status                         */
    uint32_t P17:1;                     /**< bit:     17  Pull-Down Status                         */
    uint32_t P18:1;                     /**< bit:     18  Pull-Down Status                         */
    uint32_t P19:1;                     /**< bit:     19  Pull-Down Status                         */
    uint32_t P20:1;                     /**< bit:     20  Pull-Down Status                         */
    uint32_t P21:1;                     /**< bit:     21  Pull-Down Status                         */
    uint32_t P22:1;                     /**< bit:     22  Pull-Down Status                         */
    uint32_t P23:1;                     /**< bit:     23  Pull-Down Status                         */
    uint32_t P24:1;                     /**< bit:     24  Pull-Down Status                         */
    uint32_t P25:1;                     /**< bit:     25  Pull-Down Status                         */
    uint32_t P26:1;                     /**< bit:     26  Pull-Down Status                         */
    uint32_t P27:1;                     /**< bit:     27  Pull-Down Status                         */
    uint32_t P28:1;                     /**< bit:     28  Pull-Down Status                         */
    uint32_t P29:1;                     /**< bit:     29  Pull-Down Status                         */
    uint32_t P30:1;                     /**< bit:     30  Pull-Down Status                         */
    uint32_t P31:1;                     /**< bit:     31  Pull-Down Status                         */
  } bit;                                /**< Structure used for bit  access */
  struct {
    uint32_t P:32;                      /**< bit:  0..31  Pull-Down Status                         */
  } vec;                                /**< Structure used for vec  access  */
  uint32_t reg;                         /**< Type used for register access */
} PIO_PPDSR_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define PIO_PPDSR_OFFSET                    (0x98)                                        /**<  (PIO_PPDSR) Pad Pull-down Status Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define PIO_PPDSR_P0_Pos                    0                                              /**< (PIO_PPDSR) Pull-Down Status Position */
#define PIO_PPDSR_P0_Msk                    (0x1U << PIO_PPDSR_P0_Pos)                     /**< (PIO_PPDSR) Pull-Down Status Mask */
#define PIO_PPDSR_P0                        PIO_PPDSR_P0_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PPDSR_P0_Msk instead */
#define PIO_PPDSR_P1_Pos                    1                                              /**< (PIO_PPDSR) Pull-Down Status Position */
#define PIO_PPDSR_P1_Msk                    (0x1U << PIO_PPDSR_P1_Pos)                     /**< (PIO_PPDSR) Pull-Down Status Mask */
#define PIO_PPDSR_P1                        PIO_PPDSR_P1_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PPDSR_P1_Msk instead */
#define PIO_PPDSR_P2_Pos                    2                                              /**< (PIO_PPDSR) Pull-Down Status Position */
#define PIO_PPDSR_P2_Msk                    (0x1U << PIO_PPDSR_P2_Pos)                     /**< (PIO_PPDSR) Pull-Down Status Mask */
#define PIO_PPDSR_P2                        PIO_PPDSR_P2_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PPDSR_P2_Msk instead */
#define PIO_PPDSR_P3_Pos                    3                                              /**< (PIO_PPDSR) Pull-Down Status Position */
#define PIO_PPDSR_P3_Msk                    (0x1U << PIO_PPDSR_P3_Pos)                     /**< (PIO_PPDSR) Pull-Down Status Mask */
#define PIO_PPDSR_P3                        PIO_PPDSR_P3_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PPDSR_P3_Msk instead */
#define PIO_PPDSR_P4_Pos                    4                                              /**< (PIO_PPDSR) Pull-Down Status Position */
#define PIO_PPDSR_P4_Msk                    (0x1U << PIO_PPDSR_P4_Pos)                     /**< (PIO_PPDSR) Pull-Down Status Mask */
#define PIO_PPDSR_P4                        PIO_PPDSR_P4_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PPDSR_P4_Msk instead */
#define PIO_PPDSR_P5_Pos                    5                                              /**< (PIO_PPDSR) Pull-Down Status Position */
#define PIO_PPDSR_P5_Msk                    (0x1U << PIO_PPDSR_P5_Pos)                     /**< (PIO_PPDSR) Pull-Down Status Mask */
#define PIO_PPDSR_P5                        PIO_PPDSR_P5_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PPDSR_P5_Msk instead */
#define PIO_PPDSR_P6_Pos                    6                                              /**< (PIO_PPDSR) Pull-Down Status Position */
#define PIO_PPDSR_P6_Msk                    (0x1U << PIO_PPDSR_P6_Pos)                     /**< (PIO_PPDSR) Pull-Down Status Mask */
#define PIO_PPDSR_P6                        PIO_PPDSR_P6_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PPDSR_P6_Msk instead */
#define PIO_PPDSR_P7_Pos                    7                                              /**< (PIO_PPDSR) Pull-Down Status Position */
#define PIO_PPDSR_P7_Msk                    (0x1U << PIO_PPDSR_P7_Pos)                     /**< (PIO_PPDSR) Pull-Down Status Mask */
#define PIO_PPDSR_P7                        PIO_PPDSR_P7_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PPDSR_P7_Msk instead */
#define PIO_PPDSR_P8_Pos                    8                                              /**< (PIO_PPDSR) Pull-Down Status Position */
#define PIO_PPDSR_P8_Msk                    (0x1U << PIO_PPDSR_P8_Pos)                     /**< (PIO_PPDSR) Pull-Down Status Mask */
#define PIO_PPDSR_P8                        PIO_PPDSR_P8_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PPDSR_P8_Msk instead */
#define PIO_PPDSR_P9_Pos                    9                                              /**< (PIO_PPDSR) Pull-Down Status Position */
#define PIO_PPDSR_P9_Msk                    (0x1U << PIO_PPDSR_P9_Pos)                     /**< (PIO_PPDSR) Pull-Down Status Mask */
#define PIO_PPDSR_P9                        PIO_PPDSR_P9_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PPDSR_P9_Msk instead */
#define PIO_PPDSR_P10_Pos                   10                                             /**< (PIO_PPDSR) Pull-Down Status Position */
#define PIO_PPDSR_P10_Msk                   (0x1U << PIO_PPDSR_P10_Pos)                    /**< (PIO_PPDSR) Pull-Down Status Mask */
#define PIO_PPDSR_P10                       PIO_PPDSR_P10_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PPDSR_P10_Msk instead */
#define PIO_PPDSR_P11_Pos                   11                                             /**< (PIO_PPDSR) Pull-Down Status Position */
#define PIO_PPDSR_P11_Msk                   (0x1U << PIO_PPDSR_P11_Pos)                    /**< (PIO_PPDSR) Pull-Down Status Mask */
#define PIO_PPDSR_P11                       PIO_PPDSR_P11_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PPDSR_P11_Msk instead */
#define PIO_PPDSR_P12_Pos                   12                                             /**< (PIO_PPDSR) Pull-Down Status Position */
#define PIO_PPDSR_P12_Msk                   (0x1U << PIO_PPDSR_P12_Pos)                    /**< (PIO_PPDSR) Pull-Down Status Mask */
#define PIO_PPDSR_P12                       PIO_PPDSR_P12_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PPDSR_P12_Msk instead */
#define PIO_PPDSR_P13_Pos                   13                                             /**< (PIO_PPDSR) Pull-Down Status Position */
#define PIO_PPDSR_P13_Msk                   (0x1U << PIO_PPDSR_P13_Pos)                    /**< (PIO_PPDSR) Pull-Down Status Mask */
#define PIO_PPDSR_P13                       PIO_PPDSR_P13_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PPDSR_P13_Msk instead */
#define PIO_PPDSR_P14_Pos                   14                                             /**< (PIO_PPDSR) Pull-Down Status Position */
#define PIO_PPDSR_P14_Msk                   (0x1U << PIO_PPDSR_P14_Pos)                    /**< (PIO_PPDSR) Pull-Down Status Mask */
#define PIO_PPDSR_P14                       PIO_PPDSR_P14_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PPDSR_P14_Msk instead */
#define PIO_PPDSR_P15_Pos                   15                                             /**< (PIO_PPDSR) Pull-Down Status Position */
#define PIO_PPDSR_P15_Msk                   (0x1U << PIO_PPDSR_P15_Pos)                    /**< (PIO_PPDSR) Pull-Down Status Mask */
#define PIO_PPDSR_P15                       PIO_PPDSR_P15_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PPDSR_P15_Msk instead */
#define PIO_PPDSR_P16_Pos                   16                                             /**< (PIO_PPDSR) Pull-Down Status Position */
#define PIO_PPDSR_P16_Msk                   (0x1U << PIO_PPDSR_P16_Pos)                    /**< (PIO_PPDSR) Pull-Down Status Mask */
#define PIO_PPDSR_P16                       PIO_PPDSR_P16_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PPDSR_P16_Msk instead */
#define PIO_PPDSR_P17_Pos                   17                                             /**< (PIO_PPDSR) Pull-Down Status Position */
#define PIO_PPDSR_P17_Msk                   (0x1U << PIO_PPDSR_P17_Pos)                    /**< (PIO_PPDSR) Pull-Down Status Mask */
#define PIO_PPDSR_P17                       PIO_PPDSR_P17_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PPDSR_P17_Msk instead */
#define PIO_PPDSR_P18_Pos                   18                                             /**< (PIO_PPDSR) Pull-Down Status Position */
#define PIO_PPDSR_P18_Msk                   (0x1U << PIO_PPDSR_P18_Pos)                    /**< (PIO_PPDSR) Pull-Down Status Mask */
#define PIO_PPDSR_P18                       PIO_PPDSR_P18_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PPDSR_P18_Msk instead */
#define PIO_PPDSR_P19_Pos                   19                                             /**< (PIO_PPDSR) Pull-Down Status Position */
#define PIO_PPDSR_P19_Msk                   (0x1U << PIO_PPDSR_P19_Pos)                    /**< (PIO_PPDSR) Pull-Down Status Mask */
#define PIO_PPDSR_P19                       PIO_PPDSR_P19_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PPDSR_P19_Msk instead */
#define PIO_PPDSR_P20_Pos                   20                                             /**< (PIO_PPDSR) Pull-Down Status Position */
#define PIO_PPDSR_P20_Msk                   (0x1U << PIO_PPDSR_P20_Pos)                    /**< (PIO_PPDSR) Pull-Down Status Mask */
#define PIO_PPDSR_P20                       PIO_PPDSR_P20_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PPDSR_P20_Msk instead */
#define PIO_PPDSR_P21_Pos                   21                                             /**< (PIO_PPDSR) Pull-Down Status Position */
#define PIO_PPDSR_P21_Msk                   (0x1U << PIO_PPDSR_P21_Pos)                    /**< (PIO_PPDSR) Pull-Down Status Mask */
#define PIO_PPDSR_P21                       PIO_PPDSR_P21_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PPDSR_P21_Msk instead */
#define PIO_PPDSR_P22_Pos                   22                                             /**< (PIO_PPDSR) Pull-Down Status Position */
#define PIO_PPDSR_P22_Msk                   (0x1U << PIO_PPDSR_P22_Pos)                    /**< (PIO_PPDSR) Pull-Down Status Mask */
#define PIO_PPDSR_P22                       PIO_PPDSR_P22_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PPDSR_P22_Msk instead */
#define PIO_PPDSR_P23_Pos                   23                                             /**< (PIO_PPDSR) Pull-Down Status Position */
#define PIO_PPDSR_P23_Msk                   (0x1U << PIO_PPDSR_P23_Pos)                    /**< (PIO_PPDSR) Pull-Down Status Mask */
#define PIO_PPDSR_P23                       PIO_PPDSR_P23_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PPDSR_P23_Msk instead */
#define PIO_PPDSR_P24_Pos                   24                                             /**< (PIO_PPDSR) Pull-Down Status Position */
#define PIO_PPDSR_P24_Msk                   (0x1U << PIO_PPDSR_P24_Pos)                    /**< (PIO_PPDSR) Pull-Down Status Mask */
#define PIO_PPDSR_P24                       PIO_PPDSR_P24_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PPDSR_P24_Msk instead */
#define PIO_PPDSR_P25_Pos                   25                                             /**< (PIO_PPDSR) Pull-Down Status Position */
#define PIO_PPDSR_P25_Msk                   (0x1U << PIO_PPDSR_P25_Pos)                    /**< (PIO_PPDSR) Pull-Down Status Mask */
#define PIO_PPDSR_P25                       PIO_PPDSR_P25_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PPDSR_P25_Msk instead */
#define PIO_PPDSR_P26_Pos                   26                                             /**< (PIO_PPDSR) Pull-Down Status Position */
#define PIO_PPDSR_P26_Msk                   (0x1U << PIO_PPDSR_P26_Pos)                    /**< (PIO_PPDSR) Pull-Down Status Mask */
#define PIO_PPDSR_P26                       PIO_PPDSR_P26_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PPDSR_P26_Msk instead */
#define PIO_PPDSR_P27_Pos                   27                                             /**< (PIO_PPDSR) Pull-Down Status Position */
#define PIO_PPDSR_P27_Msk                   (0x1U << PIO_PPDSR_P27_Pos)                    /**< (PIO_PPDSR) Pull-Down Status Mask */
#define PIO_PPDSR_P27                       PIO_PPDSR_P27_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PPDSR_P27_Msk instead */
#define PIO_PPDSR_P28_Pos                   28                                             /**< (PIO_PPDSR) Pull-Down Status Position */
#define PIO_PPDSR_P28_Msk                   (0x1U << PIO_PPDSR_P28_Pos)                    /**< (PIO_PPDSR) Pull-Down Status Mask */
#define PIO_PPDSR_P28                       PIO_PPDSR_P28_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PPDSR_P28_Msk instead */
#define PIO_PPDSR_P29_Pos                   29                                             /**< (PIO_PPDSR) Pull-Down Status Position */
#define PIO_PPDSR_P29_Msk                   (0x1U << PIO_PPDSR_P29_Pos)                    /**< (PIO_PPDSR) Pull-Down Status Mask */
#define PIO_PPDSR_P29                       PIO_PPDSR_P29_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PPDSR_P29_Msk instead */
#define PIO_PPDSR_P30_Pos                   30                                             /**< (PIO_PPDSR) Pull-Down Status Position */
#define PIO_PPDSR_P30_Msk                   (0x1U << PIO_PPDSR_P30_Pos)                    /**< (PIO_PPDSR) Pull-Down Status Mask */
#define PIO_PPDSR_P30                       PIO_PPDSR_P30_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PPDSR_P30_Msk instead */
#define PIO_PPDSR_P31_Pos                   31                                             /**< (PIO_PPDSR) Pull-Down Status Position */
#define PIO_PPDSR_P31_Msk                   (0x1U << PIO_PPDSR_P31_Pos)                    /**< (PIO_PPDSR) Pull-Down Status Mask */
#define PIO_PPDSR_P31                       PIO_PPDSR_P31_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PPDSR_P31_Msk instead */
#define PIO_PPDSR_P_Pos                     0                                              /**< (PIO_PPDSR Position) Pull-Down Status */
#define PIO_PPDSR_P_Msk                     (0xFFFFFFFFU << PIO_PPDSR_P_Pos)               /**< (PIO_PPDSR Mask) P */
#define PIO_PPDSR_P(value)                  (PIO_PPDSR_P_Msk & ((value) << PIO_PPDSR_P_Pos))  
#define PIO_PPDSR_MASK                      (0xFFFFFFFFU)                                  /**< \deprecated (PIO_PPDSR) Register MASK  (Use PIO_PPDSR_Msk instead)  */
#define PIO_PPDSR_Msk                       (0xFFFFFFFFU)                                  /**< (PIO_PPDSR) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- PIO_OWER : (PIO Offset: 0xa0) (/W 32) Output Write Enable -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t P0:1;                      /**< bit:      0  Output Write Enable                      */
    uint32_t P1:1;                      /**< bit:      1  Output Write Enable                      */
    uint32_t P2:1;                      /**< bit:      2  Output Write Enable                      */
    uint32_t P3:1;                      /**< bit:      3  Output Write Enable                      */
    uint32_t P4:1;                      /**< bit:      4  Output Write Enable                      */
    uint32_t P5:1;                      /**< bit:      5  Output Write Enable                      */
    uint32_t P6:1;                      /**< bit:      6  Output Write Enable                      */
    uint32_t P7:1;                      /**< bit:      7  Output Write Enable                      */
    uint32_t P8:1;                      /**< bit:      8  Output Write Enable                      */
    uint32_t P9:1;                      /**< bit:      9  Output Write Enable                      */
    uint32_t P10:1;                     /**< bit:     10  Output Write Enable                      */
    uint32_t P11:1;                     /**< bit:     11  Output Write Enable                      */
    uint32_t P12:1;                     /**< bit:     12  Output Write Enable                      */
    uint32_t P13:1;                     /**< bit:     13  Output Write Enable                      */
    uint32_t P14:1;                     /**< bit:     14  Output Write Enable                      */
    uint32_t P15:1;                     /**< bit:     15  Output Write Enable                      */
    uint32_t P16:1;                     /**< bit:     16  Output Write Enable                      */
    uint32_t P17:1;                     /**< bit:     17  Output Write Enable                      */
    uint32_t P18:1;                     /**< bit:     18  Output Write Enable                      */
    uint32_t P19:1;                     /**< bit:     19  Output Write Enable                      */
    uint32_t P20:1;                     /**< bit:     20  Output Write Enable                      */
    uint32_t P21:1;                     /**< bit:     21  Output Write Enable                      */
    uint32_t P22:1;                     /**< bit:     22  Output Write Enable                      */
    uint32_t P23:1;                     /**< bit:     23  Output Write Enable                      */
    uint32_t P24:1;                     /**< bit:     24  Output Write Enable                      */
    uint32_t P25:1;                     /**< bit:     25  Output Write Enable                      */
    uint32_t P26:1;                     /**< bit:     26  Output Write Enable                      */
    uint32_t P27:1;                     /**< bit:     27  Output Write Enable                      */
    uint32_t P28:1;                     /**< bit:     28  Output Write Enable                      */
    uint32_t P29:1;                     /**< bit:     29  Output Write Enable                      */
    uint32_t P30:1;                     /**< bit:     30  Output Write Enable                      */
    uint32_t P31:1;                     /**< bit:     31  Output Write Enable                      */
  } bit;                                /**< Structure used for bit  access */
  struct {
    uint32_t P:32;                      /**< bit:  0..31  Output Write Enable                      */
  } vec;                                /**< Structure used for vec  access  */
  uint32_t reg;                         /**< Type used for register access */
} PIO_OWER_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define PIO_OWER_OFFSET                     (0xA0)                                        /**<  (PIO_OWER) Output Write Enable  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define PIO_OWER_P0_Pos                     0                                              /**< (PIO_OWER) Output Write Enable Position */
#define PIO_OWER_P0_Msk                     (0x1U << PIO_OWER_P0_Pos)                      /**< (PIO_OWER) Output Write Enable Mask */
#define PIO_OWER_P0                         PIO_OWER_P0_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_OWER_P0_Msk instead */
#define PIO_OWER_P1_Pos                     1                                              /**< (PIO_OWER) Output Write Enable Position */
#define PIO_OWER_P1_Msk                     (0x1U << PIO_OWER_P1_Pos)                      /**< (PIO_OWER) Output Write Enable Mask */
#define PIO_OWER_P1                         PIO_OWER_P1_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_OWER_P1_Msk instead */
#define PIO_OWER_P2_Pos                     2                                              /**< (PIO_OWER) Output Write Enable Position */
#define PIO_OWER_P2_Msk                     (0x1U << PIO_OWER_P2_Pos)                      /**< (PIO_OWER) Output Write Enable Mask */
#define PIO_OWER_P2                         PIO_OWER_P2_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_OWER_P2_Msk instead */
#define PIO_OWER_P3_Pos                     3                                              /**< (PIO_OWER) Output Write Enable Position */
#define PIO_OWER_P3_Msk                     (0x1U << PIO_OWER_P3_Pos)                      /**< (PIO_OWER) Output Write Enable Mask */
#define PIO_OWER_P3                         PIO_OWER_P3_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_OWER_P3_Msk instead */
#define PIO_OWER_P4_Pos                     4                                              /**< (PIO_OWER) Output Write Enable Position */
#define PIO_OWER_P4_Msk                     (0x1U << PIO_OWER_P4_Pos)                      /**< (PIO_OWER) Output Write Enable Mask */
#define PIO_OWER_P4                         PIO_OWER_P4_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_OWER_P4_Msk instead */
#define PIO_OWER_P5_Pos                     5                                              /**< (PIO_OWER) Output Write Enable Position */
#define PIO_OWER_P5_Msk                     (0x1U << PIO_OWER_P5_Pos)                      /**< (PIO_OWER) Output Write Enable Mask */
#define PIO_OWER_P5                         PIO_OWER_P5_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_OWER_P5_Msk instead */
#define PIO_OWER_P6_Pos                     6                                              /**< (PIO_OWER) Output Write Enable Position */
#define PIO_OWER_P6_Msk                     (0x1U << PIO_OWER_P6_Pos)                      /**< (PIO_OWER) Output Write Enable Mask */
#define PIO_OWER_P6                         PIO_OWER_P6_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_OWER_P6_Msk instead */
#define PIO_OWER_P7_Pos                     7                                              /**< (PIO_OWER) Output Write Enable Position */
#define PIO_OWER_P7_Msk                     (0x1U << PIO_OWER_P7_Pos)                      /**< (PIO_OWER) Output Write Enable Mask */
#define PIO_OWER_P7                         PIO_OWER_P7_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_OWER_P7_Msk instead */
#define PIO_OWER_P8_Pos                     8                                              /**< (PIO_OWER) Output Write Enable Position */
#define PIO_OWER_P8_Msk                     (0x1U << PIO_OWER_P8_Pos)                      /**< (PIO_OWER) Output Write Enable Mask */
#define PIO_OWER_P8                         PIO_OWER_P8_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_OWER_P8_Msk instead */
#define PIO_OWER_P9_Pos                     9                                              /**< (PIO_OWER) Output Write Enable Position */
#define PIO_OWER_P9_Msk                     (0x1U << PIO_OWER_P9_Pos)                      /**< (PIO_OWER) Output Write Enable Mask */
#define PIO_OWER_P9                         PIO_OWER_P9_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_OWER_P9_Msk instead */
#define PIO_OWER_P10_Pos                    10                                             /**< (PIO_OWER) Output Write Enable Position */
#define PIO_OWER_P10_Msk                    (0x1U << PIO_OWER_P10_Pos)                     /**< (PIO_OWER) Output Write Enable Mask */
#define PIO_OWER_P10                        PIO_OWER_P10_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_OWER_P10_Msk instead */
#define PIO_OWER_P11_Pos                    11                                             /**< (PIO_OWER) Output Write Enable Position */
#define PIO_OWER_P11_Msk                    (0x1U << PIO_OWER_P11_Pos)                     /**< (PIO_OWER) Output Write Enable Mask */
#define PIO_OWER_P11                        PIO_OWER_P11_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_OWER_P11_Msk instead */
#define PIO_OWER_P12_Pos                    12                                             /**< (PIO_OWER) Output Write Enable Position */
#define PIO_OWER_P12_Msk                    (0x1U << PIO_OWER_P12_Pos)                     /**< (PIO_OWER) Output Write Enable Mask */
#define PIO_OWER_P12                        PIO_OWER_P12_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_OWER_P12_Msk instead */
#define PIO_OWER_P13_Pos                    13                                             /**< (PIO_OWER) Output Write Enable Position */
#define PIO_OWER_P13_Msk                    (0x1U << PIO_OWER_P13_Pos)                     /**< (PIO_OWER) Output Write Enable Mask */
#define PIO_OWER_P13                        PIO_OWER_P13_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_OWER_P13_Msk instead */
#define PIO_OWER_P14_Pos                    14                                             /**< (PIO_OWER) Output Write Enable Position */
#define PIO_OWER_P14_Msk                    (0x1U << PIO_OWER_P14_Pos)                     /**< (PIO_OWER) Output Write Enable Mask */
#define PIO_OWER_P14                        PIO_OWER_P14_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_OWER_P14_Msk instead */
#define PIO_OWER_P15_Pos                    15                                             /**< (PIO_OWER) Output Write Enable Position */
#define PIO_OWER_P15_Msk                    (0x1U << PIO_OWER_P15_Pos)                     /**< (PIO_OWER) Output Write Enable Mask */
#define PIO_OWER_P15                        PIO_OWER_P15_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_OWER_P15_Msk instead */
#define PIO_OWER_P16_Pos                    16                                             /**< (PIO_OWER) Output Write Enable Position */
#define PIO_OWER_P16_Msk                    (0x1U << PIO_OWER_P16_Pos)                     /**< (PIO_OWER) Output Write Enable Mask */
#define PIO_OWER_P16                        PIO_OWER_P16_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_OWER_P16_Msk instead */
#define PIO_OWER_P17_Pos                    17                                             /**< (PIO_OWER) Output Write Enable Position */
#define PIO_OWER_P17_Msk                    (0x1U << PIO_OWER_P17_Pos)                     /**< (PIO_OWER) Output Write Enable Mask */
#define PIO_OWER_P17                        PIO_OWER_P17_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_OWER_P17_Msk instead */
#define PIO_OWER_P18_Pos                    18                                             /**< (PIO_OWER) Output Write Enable Position */
#define PIO_OWER_P18_Msk                    (0x1U << PIO_OWER_P18_Pos)                     /**< (PIO_OWER) Output Write Enable Mask */
#define PIO_OWER_P18                        PIO_OWER_P18_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_OWER_P18_Msk instead */
#define PIO_OWER_P19_Pos                    19                                             /**< (PIO_OWER) Output Write Enable Position */
#define PIO_OWER_P19_Msk                    (0x1U << PIO_OWER_P19_Pos)                     /**< (PIO_OWER) Output Write Enable Mask */
#define PIO_OWER_P19                        PIO_OWER_P19_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_OWER_P19_Msk instead */
#define PIO_OWER_P20_Pos                    20                                             /**< (PIO_OWER) Output Write Enable Position */
#define PIO_OWER_P20_Msk                    (0x1U << PIO_OWER_P20_Pos)                     /**< (PIO_OWER) Output Write Enable Mask */
#define PIO_OWER_P20                        PIO_OWER_P20_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_OWER_P20_Msk instead */
#define PIO_OWER_P21_Pos                    21                                             /**< (PIO_OWER) Output Write Enable Position */
#define PIO_OWER_P21_Msk                    (0x1U << PIO_OWER_P21_Pos)                     /**< (PIO_OWER) Output Write Enable Mask */
#define PIO_OWER_P21                        PIO_OWER_P21_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_OWER_P21_Msk instead */
#define PIO_OWER_P22_Pos                    22                                             /**< (PIO_OWER) Output Write Enable Position */
#define PIO_OWER_P22_Msk                    (0x1U << PIO_OWER_P22_Pos)                     /**< (PIO_OWER) Output Write Enable Mask */
#define PIO_OWER_P22                        PIO_OWER_P22_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_OWER_P22_Msk instead */
#define PIO_OWER_P23_Pos                    23                                             /**< (PIO_OWER) Output Write Enable Position */
#define PIO_OWER_P23_Msk                    (0x1U << PIO_OWER_P23_Pos)                     /**< (PIO_OWER) Output Write Enable Mask */
#define PIO_OWER_P23                        PIO_OWER_P23_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_OWER_P23_Msk instead */
#define PIO_OWER_P24_Pos                    24                                             /**< (PIO_OWER) Output Write Enable Position */
#define PIO_OWER_P24_Msk                    (0x1U << PIO_OWER_P24_Pos)                     /**< (PIO_OWER) Output Write Enable Mask */
#define PIO_OWER_P24                        PIO_OWER_P24_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_OWER_P24_Msk instead */
#define PIO_OWER_P25_Pos                    25                                             /**< (PIO_OWER) Output Write Enable Position */
#define PIO_OWER_P25_Msk                    (0x1U << PIO_OWER_P25_Pos)                     /**< (PIO_OWER) Output Write Enable Mask */
#define PIO_OWER_P25                        PIO_OWER_P25_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_OWER_P25_Msk instead */
#define PIO_OWER_P26_Pos                    26                                             /**< (PIO_OWER) Output Write Enable Position */
#define PIO_OWER_P26_Msk                    (0x1U << PIO_OWER_P26_Pos)                     /**< (PIO_OWER) Output Write Enable Mask */
#define PIO_OWER_P26                        PIO_OWER_P26_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_OWER_P26_Msk instead */
#define PIO_OWER_P27_Pos                    27                                             /**< (PIO_OWER) Output Write Enable Position */
#define PIO_OWER_P27_Msk                    (0x1U << PIO_OWER_P27_Pos)                     /**< (PIO_OWER) Output Write Enable Mask */
#define PIO_OWER_P27                        PIO_OWER_P27_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_OWER_P27_Msk instead */
#define PIO_OWER_P28_Pos                    28                                             /**< (PIO_OWER) Output Write Enable Position */
#define PIO_OWER_P28_Msk                    (0x1U << PIO_OWER_P28_Pos)                     /**< (PIO_OWER) Output Write Enable Mask */
#define PIO_OWER_P28                        PIO_OWER_P28_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_OWER_P28_Msk instead */
#define PIO_OWER_P29_Pos                    29                                             /**< (PIO_OWER) Output Write Enable Position */
#define PIO_OWER_P29_Msk                    (0x1U << PIO_OWER_P29_Pos)                     /**< (PIO_OWER) Output Write Enable Mask */
#define PIO_OWER_P29                        PIO_OWER_P29_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_OWER_P29_Msk instead */
#define PIO_OWER_P30_Pos                    30                                             /**< (PIO_OWER) Output Write Enable Position */
#define PIO_OWER_P30_Msk                    (0x1U << PIO_OWER_P30_Pos)                     /**< (PIO_OWER) Output Write Enable Mask */
#define PIO_OWER_P30                        PIO_OWER_P30_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_OWER_P30_Msk instead */
#define PIO_OWER_P31_Pos                    31                                             /**< (PIO_OWER) Output Write Enable Position */
#define PIO_OWER_P31_Msk                    (0x1U << PIO_OWER_P31_Pos)                     /**< (PIO_OWER) Output Write Enable Mask */
#define PIO_OWER_P31                        PIO_OWER_P31_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_OWER_P31_Msk instead */
#define PIO_OWER_P_Pos                      0                                              /**< (PIO_OWER Position) Output Write Enable */
#define PIO_OWER_P_Msk                      (0xFFFFFFFFU << PIO_OWER_P_Pos)                /**< (PIO_OWER Mask) P */
#define PIO_OWER_P(value)                   (PIO_OWER_P_Msk & ((value) << PIO_OWER_P_Pos))  
#define PIO_OWER_MASK                       (0xFFFFFFFFU)                                  /**< \deprecated (PIO_OWER) Register MASK  (Use PIO_OWER_Msk instead)  */
#define PIO_OWER_Msk                        (0xFFFFFFFFU)                                  /**< (PIO_OWER) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- PIO_OWDR : (PIO Offset: 0xa4) (/W 32) Output Write Disable -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t P0:1;                      /**< bit:      0  Output Write Disable                     */
    uint32_t P1:1;                      /**< bit:      1  Output Write Disable                     */
    uint32_t P2:1;                      /**< bit:      2  Output Write Disable                     */
    uint32_t P3:1;                      /**< bit:      3  Output Write Disable                     */
    uint32_t P4:1;                      /**< bit:      4  Output Write Disable                     */
    uint32_t P5:1;                      /**< bit:      5  Output Write Disable                     */
    uint32_t P6:1;                      /**< bit:      6  Output Write Disable                     */
    uint32_t P7:1;                      /**< bit:      7  Output Write Disable                     */
    uint32_t P8:1;                      /**< bit:      8  Output Write Disable                     */
    uint32_t P9:1;                      /**< bit:      9  Output Write Disable                     */
    uint32_t P10:1;                     /**< bit:     10  Output Write Disable                     */
    uint32_t P11:1;                     /**< bit:     11  Output Write Disable                     */
    uint32_t P12:1;                     /**< bit:     12  Output Write Disable                     */
    uint32_t P13:1;                     /**< bit:     13  Output Write Disable                     */
    uint32_t P14:1;                     /**< bit:     14  Output Write Disable                     */
    uint32_t P15:1;                     /**< bit:     15  Output Write Disable                     */
    uint32_t P16:1;                     /**< bit:     16  Output Write Disable                     */
    uint32_t P17:1;                     /**< bit:     17  Output Write Disable                     */
    uint32_t P18:1;                     /**< bit:     18  Output Write Disable                     */
    uint32_t P19:1;                     /**< bit:     19  Output Write Disable                     */
    uint32_t P20:1;                     /**< bit:     20  Output Write Disable                     */
    uint32_t P21:1;                     /**< bit:     21  Output Write Disable                     */
    uint32_t P22:1;                     /**< bit:     22  Output Write Disable                     */
    uint32_t P23:1;                     /**< bit:     23  Output Write Disable                     */
    uint32_t P24:1;                     /**< bit:     24  Output Write Disable                     */
    uint32_t P25:1;                     /**< bit:     25  Output Write Disable                     */
    uint32_t P26:1;                     /**< bit:     26  Output Write Disable                     */
    uint32_t P27:1;                     /**< bit:     27  Output Write Disable                     */
    uint32_t P28:1;                     /**< bit:     28  Output Write Disable                     */
    uint32_t P29:1;                     /**< bit:     29  Output Write Disable                     */
    uint32_t P30:1;                     /**< bit:     30  Output Write Disable                     */
    uint32_t P31:1;                     /**< bit:     31  Output Write Disable                     */
  } bit;                                /**< Structure used for bit  access */
  struct {
    uint32_t P:32;                      /**< bit:  0..31  Output Write Disable                     */
  } vec;                                /**< Structure used for vec  access  */
  uint32_t reg;                         /**< Type used for register access */
} PIO_OWDR_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define PIO_OWDR_OFFSET                     (0xA4)                                        /**<  (PIO_OWDR) Output Write Disable  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define PIO_OWDR_P0_Pos                     0                                              /**< (PIO_OWDR) Output Write Disable Position */
#define PIO_OWDR_P0_Msk                     (0x1U << PIO_OWDR_P0_Pos)                      /**< (PIO_OWDR) Output Write Disable Mask */
#define PIO_OWDR_P0                         PIO_OWDR_P0_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_OWDR_P0_Msk instead */
#define PIO_OWDR_P1_Pos                     1                                              /**< (PIO_OWDR) Output Write Disable Position */
#define PIO_OWDR_P1_Msk                     (0x1U << PIO_OWDR_P1_Pos)                      /**< (PIO_OWDR) Output Write Disable Mask */
#define PIO_OWDR_P1                         PIO_OWDR_P1_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_OWDR_P1_Msk instead */
#define PIO_OWDR_P2_Pos                     2                                              /**< (PIO_OWDR) Output Write Disable Position */
#define PIO_OWDR_P2_Msk                     (0x1U << PIO_OWDR_P2_Pos)                      /**< (PIO_OWDR) Output Write Disable Mask */
#define PIO_OWDR_P2                         PIO_OWDR_P2_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_OWDR_P2_Msk instead */
#define PIO_OWDR_P3_Pos                     3                                              /**< (PIO_OWDR) Output Write Disable Position */
#define PIO_OWDR_P3_Msk                     (0x1U << PIO_OWDR_P3_Pos)                      /**< (PIO_OWDR) Output Write Disable Mask */
#define PIO_OWDR_P3                         PIO_OWDR_P3_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_OWDR_P3_Msk instead */
#define PIO_OWDR_P4_Pos                     4                                              /**< (PIO_OWDR) Output Write Disable Position */
#define PIO_OWDR_P4_Msk                     (0x1U << PIO_OWDR_P4_Pos)                      /**< (PIO_OWDR) Output Write Disable Mask */
#define PIO_OWDR_P4                         PIO_OWDR_P4_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_OWDR_P4_Msk instead */
#define PIO_OWDR_P5_Pos                     5                                              /**< (PIO_OWDR) Output Write Disable Position */
#define PIO_OWDR_P5_Msk                     (0x1U << PIO_OWDR_P5_Pos)                      /**< (PIO_OWDR) Output Write Disable Mask */
#define PIO_OWDR_P5                         PIO_OWDR_P5_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_OWDR_P5_Msk instead */
#define PIO_OWDR_P6_Pos                     6                                              /**< (PIO_OWDR) Output Write Disable Position */
#define PIO_OWDR_P6_Msk                     (0x1U << PIO_OWDR_P6_Pos)                      /**< (PIO_OWDR) Output Write Disable Mask */
#define PIO_OWDR_P6                         PIO_OWDR_P6_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_OWDR_P6_Msk instead */
#define PIO_OWDR_P7_Pos                     7                                              /**< (PIO_OWDR) Output Write Disable Position */
#define PIO_OWDR_P7_Msk                     (0x1U << PIO_OWDR_P7_Pos)                      /**< (PIO_OWDR) Output Write Disable Mask */
#define PIO_OWDR_P7                         PIO_OWDR_P7_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_OWDR_P7_Msk instead */
#define PIO_OWDR_P8_Pos                     8                                              /**< (PIO_OWDR) Output Write Disable Position */
#define PIO_OWDR_P8_Msk                     (0x1U << PIO_OWDR_P8_Pos)                      /**< (PIO_OWDR) Output Write Disable Mask */
#define PIO_OWDR_P8                         PIO_OWDR_P8_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_OWDR_P8_Msk instead */
#define PIO_OWDR_P9_Pos                     9                                              /**< (PIO_OWDR) Output Write Disable Position */
#define PIO_OWDR_P9_Msk                     (0x1U << PIO_OWDR_P9_Pos)                      /**< (PIO_OWDR) Output Write Disable Mask */
#define PIO_OWDR_P9                         PIO_OWDR_P9_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_OWDR_P9_Msk instead */
#define PIO_OWDR_P10_Pos                    10                                             /**< (PIO_OWDR) Output Write Disable Position */
#define PIO_OWDR_P10_Msk                    (0x1U << PIO_OWDR_P10_Pos)                     /**< (PIO_OWDR) Output Write Disable Mask */
#define PIO_OWDR_P10                        PIO_OWDR_P10_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_OWDR_P10_Msk instead */
#define PIO_OWDR_P11_Pos                    11                                             /**< (PIO_OWDR) Output Write Disable Position */
#define PIO_OWDR_P11_Msk                    (0x1U << PIO_OWDR_P11_Pos)                     /**< (PIO_OWDR) Output Write Disable Mask */
#define PIO_OWDR_P11                        PIO_OWDR_P11_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_OWDR_P11_Msk instead */
#define PIO_OWDR_P12_Pos                    12                                             /**< (PIO_OWDR) Output Write Disable Position */
#define PIO_OWDR_P12_Msk                    (0x1U << PIO_OWDR_P12_Pos)                     /**< (PIO_OWDR) Output Write Disable Mask */
#define PIO_OWDR_P12                        PIO_OWDR_P12_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_OWDR_P12_Msk instead */
#define PIO_OWDR_P13_Pos                    13                                             /**< (PIO_OWDR) Output Write Disable Position */
#define PIO_OWDR_P13_Msk                    (0x1U << PIO_OWDR_P13_Pos)                     /**< (PIO_OWDR) Output Write Disable Mask */
#define PIO_OWDR_P13                        PIO_OWDR_P13_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_OWDR_P13_Msk instead */
#define PIO_OWDR_P14_Pos                    14                                             /**< (PIO_OWDR) Output Write Disable Position */
#define PIO_OWDR_P14_Msk                    (0x1U << PIO_OWDR_P14_Pos)                     /**< (PIO_OWDR) Output Write Disable Mask */
#define PIO_OWDR_P14                        PIO_OWDR_P14_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_OWDR_P14_Msk instead */
#define PIO_OWDR_P15_Pos                    15                                             /**< (PIO_OWDR) Output Write Disable Position */
#define PIO_OWDR_P15_Msk                    (0x1U << PIO_OWDR_P15_Pos)                     /**< (PIO_OWDR) Output Write Disable Mask */
#define PIO_OWDR_P15                        PIO_OWDR_P15_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_OWDR_P15_Msk instead */
#define PIO_OWDR_P16_Pos                    16                                             /**< (PIO_OWDR) Output Write Disable Position */
#define PIO_OWDR_P16_Msk                    (0x1U << PIO_OWDR_P16_Pos)                     /**< (PIO_OWDR) Output Write Disable Mask */
#define PIO_OWDR_P16                        PIO_OWDR_P16_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_OWDR_P16_Msk instead */
#define PIO_OWDR_P17_Pos                    17                                             /**< (PIO_OWDR) Output Write Disable Position */
#define PIO_OWDR_P17_Msk                    (0x1U << PIO_OWDR_P17_Pos)                     /**< (PIO_OWDR) Output Write Disable Mask */
#define PIO_OWDR_P17                        PIO_OWDR_P17_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_OWDR_P17_Msk instead */
#define PIO_OWDR_P18_Pos                    18                                             /**< (PIO_OWDR) Output Write Disable Position */
#define PIO_OWDR_P18_Msk                    (0x1U << PIO_OWDR_P18_Pos)                     /**< (PIO_OWDR) Output Write Disable Mask */
#define PIO_OWDR_P18                        PIO_OWDR_P18_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_OWDR_P18_Msk instead */
#define PIO_OWDR_P19_Pos                    19                                             /**< (PIO_OWDR) Output Write Disable Position */
#define PIO_OWDR_P19_Msk                    (0x1U << PIO_OWDR_P19_Pos)                     /**< (PIO_OWDR) Output Write Disable Mask */
#define PIO_OWDR_P19                        PIO_OWDR_P19_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_OWDR_P19_Msk instead */
#define PIO_OWDR_P20_Pos                    20                                             /**< (PIO_OWDR) Output Write Disable Position */
#define PIO_OWDR_P20_Msk                    (0x1U << PIO_OWDR_P20_Pos)                     /**< (PIO_OWDR) Output Write Disable Mask */
#define PIO_OWDR_P20                        PIO_OWDR_P20_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_OWDR_P20_Msk instead */
#define PIO_OWDR_P21_Pos                    21                                             /**< (PIO_OWDR) Output Write Disable Position */
#define PIO_OWDR_P21_Msk                    (0x1U << PIO_OWDR_P21_Pos)                     /**< (PIO_OWDR) Output Write Disable Mask */
#define PIO_OWDR_P21                        PIO_OWDR_P21_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_OWDR_P21_Msk instead */
#define PIO_OWDR_P22_Pos                    22                                             /**< (PIO_OWDR) Output Write Disable Position */
#define PIO_OWDR_P22_Msk                    (0x1U << PIO_OWDR_P22_Pos)                     /**< (PIO_OWDR) Output Write Disable Mask */
#define PIO_OWDR_P22                        PIO_OWDR_P22_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_OWDR_P22_Msk instead */
#define PIO_OWDR_P23_Pos                    23                                             /**< (PIO_OWDR) Output Write Disable Position */
#define PIO_OWDR_P23_Msk                    (0x1U << PIO_OWDR_P23_Pos)                     /**< (PIO_OWDR) Output Write Disable Mask */
#define PIO_OWDR_P23                        PIO_OWDR_P23_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_OWDR_P23_Msk instead */
#define PIO_OWDR_P24_Pos                    24                                             /**< (PIO_OWDR) Output Write Disable Position */
#define PIO_OWDR_P24_Msk                    (0x1U << PIO_OWDR_P24_Pos)                     /**< (PIO_OWDR) Output Write Disable Mask */
#define PIO_OWDR_P24                        PIO_OWDR_P24_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_OWDR_P24_Msk instead */
#define PIO_OWDR_P25_Pos                    25                                             /**< (PIO_OWDR) Output Write Disable Position */
#define PIO_OWDR_P25_Msk                    (0x1U << PIO_OWDR_P25_Pos)                     /**< (PIO_OWDR) Output Write Disable Mask */
#define PIO_OWDR_P25                        PIO_OWDR_P25_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_OWDR_P25_Msk instead */
#define PIO_OWDR_P26_Pos                    26                                             /**< (PIO_OWDR) Output Write Disable Position */
#define PIO_OWDR_P26_Msk                    (0x1U << PIO_OWDR_P26_Pos)                     /**< (PIO_OWDR) Output Write Disable Mask */
#define PIO_OWDR_P26                        PIO_OWDR_P26_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_OWDR_P26_Msk instead */
#define PIO_OWDR_P27_Pos                    27                                             /**< (PIO_OWDR) Output Write Disable Position */
#define PIO_OWDR_P27_Msk                    (0x1U << PIO_OWDR_P27_Pos)                     /**< (PIO_OWDR) Output Write Disable Mask */
#define PIO_OWDR_P27                        PIO_OWDR_P27_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_OWDR_P27_Msk instead */
#define PIO_OWDR_P28_Pos                    28                                             /**< (PIO_OWDR) Output Write Disable Position */
#define PIO_OWDR_P28_Msk                    (0x1U << PIO_OWDR_P28_Pos)                     /**< (PIO_OWDR) Output Write Disable Mask */
#define PIO_OWDR_P28                        PIO_OWDR_P28_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_OWDR_P28_Msk instead */
#define PIO_OWDR_P29_Pos                    29                                             /**< (PIO_OWDR) Output Write Disable Position */
#define PIO_OWDR_P29_Msk                    (0x1U << PIO_OWDR_P29_Pos)                     /**< (PIO_OWDR) Output Write Disable Mask */
#define PIO_OWDR_P29                        PIO_OWDR_P29_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_OWDR_P29_Msk instead */
#define PIO_OWDR_P30_Pos                    30                                             /**< (PIO_OWDR) Output Write Disable Position */
#define PIO_OWDR_P30_Msk                    (0x1U << PIO_OWDR_P30_Pos)                     /**< (PIO_OWDR) Output Write Disable Mask */
#define PIO_OWDR_P30                        PIO_OWDR_P30_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_OWDR_P30_Msk instead */
#define PIO_OWDR_P31_Pos                    31                                             /**< (PIO_OWDR) Output Write Disable Position */
#define PIO_OWDR_P31_Msk                    (0x1U << PIO_OWDR_P31_Pos)                     /**< (PIO_OWDR) Output Write Disable Mask */
#define PIO_OWDR_P31                        PIO_OWDR_P31_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_OWDR_P31_Msk instead */
#define PIO_OWDR_P_Pos                      0                                              /**< (PIO_OWDR Position) Output Write Disable */
#define PIO_OWDR_P_Msk                      (0xFFFFFFFFU << PIO_OWDR_P_Pos)                /**< (PIO_OWDR Mask) P */
#define PIO_OWDR_P(value)                   (PIO_OWDR_P_Msk & ((value) << PIO_OWDR_P_Pos))  
#define PIO_OWDR_MASK                       (0xFFFFFFFFU)                                  /**< \deprecated (PIO_OWDR) Register MASK  (Use PIO_OWDR_Msk instead)  */
#define PIO_OWDR_Msk                        (0xFFFFFFFFU)                                  /**< (PIO_OWDR) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- PIO_OWSR : (PIO Offset: 0xa8) (R/ 32) Output Write Status Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t P0:1;                      /**< bit:      0  Output Write Status                      */
    uint32_t P1:1;                      /**< bit:      1  Output Write Status                      */
    uint32_t P2:1;                      /**< bit:      2  Output Write Status                      */
    uint32_t P3:1;                      /**< bit:      3  Output Write Status                      */
    uint32_t P4:1;                      /**< bit:      4  Output Write Status                      */
    uint32_t P5:1;                      /**< bit:      5  Output Write Status                      */
    uint32_t P6:1;                      /**< bit:      6  Output Write Status                      */
    uint32_t P7:1;                      /**< bit:      7  Output Write Status                      */
    uint32_t P8:1;                      /**< bit:      8  Output Write Status                      */
    uint32_t P9:1;                      /**< bit:      9  Output Write Status                      */
    uint32_t P10:1;                     /**< bit:     10  Output Write Status                      */
    uint32_t P11:1;                     /**< bit:     11  Output Write Status                      */
    uint32_t P12:1;                     /**< bit:     12  Output Write Status                      */
    uint32_t P13:1;                     /**< bit:     13  Output Write Status                      */
    uint32_t P14:1;                     /**< bit:     14  Output Write Status                      */
    uint32_t P15:1;                     /**< bit:     15  Output Write Status                      */
    uint32_t P16:1;                     /**< bit:     16  Output Write Status                      */
    uint32_t P17:1;                     /**< bit:     17  Output Write Status                      */
    uint32_t P18:1;                     /**< bit:     18  Output Write Status                      */
    uint32_t P19:1;                     /**< bit:     19  Output Write Status                      */
    uint32_t P20:1;                     /**< bit:     20  Output Write Status                      */
    uint32_t P21:1;                     /**< bit:     21  Output Write Status                      */
    uint32_t P22:1;                     /**< bit:     22  Output Write Status                      */
    uint32_t P23:1;                     /**< bit:     23  Output Write Status                      */
    uint32_t P24:1;                     /**< bit:     24  Output Write Status                      */
    uint32_t P25:1;                     /**< bit:     25  Output Write Status                      */
    uint32_t P26:1;                     /**< bit:     26  Output Write Status                      */
    uint32_t P27:1;                     /**< bit:     27  Output Write Status                      */
    uint32_t P28:1;                     /**< bit:     28  Output Write Status                      */
    uint32_t P29:1;                     /**< bit:     29  Output Write Status                      */
    uint32_t P30:1;                     /**< bit:     30  Output Write Status                      */
    uint32_t P31:1;                     /**< bit:     31  Output Write Status                      */
  } bit;                                /**< Structure used for bit  access */
  struct {
    uint32_t P:32;                      /**< bit:  0..31  Output Write Status                      */
  } vec;                                /**< Structure used for vec  access  */
  uint32_t reg;                         /**< Type used for register access */
} PIO_OWSR_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define PIO_OWSR_OFFSET                     (0xA8)                                        /**<  (PIO_OWSR) Output Write Status Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define PIO_OWSR_P0_Pos                     0                                              /**< (PIO_OWSR) Output Write Status Position */
#define PIO_OWSR_P0_Msk                     (0x1U << PIO_OWSR_P0_Pos)                      /**< (PIO_OWSR) Output Write Status Mask */
#define PIO_OWSR_P0                         PIO_OWSR_P0_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_OWSR_P0_Msk instead */
#define PIO_OWSR_P1_Pos                     1                                              /**< (PIO_OWSR) Output Write Status Position */
#define PIO_OWSR_P1_Msk                     (0x1U << PIO_OWSR_P1_Pos)                      /**< (PIO_OWSR) Output Write Status Mask */
#define PIO_OWSR_P1                         PIO_OWSR_P1_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_OWSR_P1_Msk instead */
#define PIO_OWSR_P2_Pos                     2                                              /**< (PIO_OWSR) Output Write Status Position */
#define PIO_OWSR_P2_Msk                     (0x1U << PIO_OWSR_P2_Pos)                      /**< (PIO_OWSR) Output Write Status Mask */
#define PIO_OWSR_P2                         PIO_OWSR_P2_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_OWSR_P2_Msk instead */
#define PIO_OWSR_P3_Pos                     3                                              /**< (PIO_OWSR) Output Write Status Position */
#define PIO_OWSR_P3_Msk                     (0x1U << PIO_OWSR_P3_Pos)                      /**< (PIO_OWSR) Output Write Status Mask */
#define PIO_OWSR_P3                         PIO_OWSR_P3_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_OWSR_P3_Msk instead */
#define PIO_OWSR_P4_Pos                     4                                              /**< (PIO_OWSR) Output Write Status Position */
#define PIO_OWSR_P4_Msk                     (0x1U << PIO_OWSR_P4_Pos)                      /**< (PIO_OWSR) Output Write Status Mask */
#define PIO_OWSR_P4                         PIO_OWSR_P4_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_OWSR_P4_Msk instead */
#define PIO_OWSR_P5_Pos                     5                                              /**< (PIO_OWSR) Output Write Status Position */
#define PIO_OWSR_P5_Msk                     (0x1U << PIO_OWSR_P5_Pos)                      /**< (PIO_OWSR) Output Write Status Mask */
#define PIO_OWSR_P5                         PIO_OWSR_P5_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_OWSR_P5_Msk instead */
#define PIO_OWSR_P6_Pos                     6                                              /**< (PIO_OWSR) Output Write Status Position */
#define PIO_OWSR_P6_Msk                     (0x1U << PIO_OWSR_P6_Pos)                      /**< (PIO_OWSR) Output Write Status Mask */
#define PIO_OWSR_P6                         PIO_OWSR_P6_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_OWSR_P6_Msk instead */
#define PIO_OWSR_P7_Pos                     7                                              /**< (PIO_OWSR) Output Write Status Position */
#define PIO_OWSR_P7_Msk                     (0x1U << PIO_OWSR_P7_Pos)                      /**< (PIO_OWSR) Output Write Status Mask */
#define PIO_OWSR_P7                         PIO_OWSR_P7_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_OWSR_P7_Msk instead */
#define PIO_OWSR_P8_Pos                     8                                              /**< (PIO_OWSR) Output Write Status Position */
#define PIO_OWSR_P8_Msk                     (0x1U << PIO_OWSR_P8_Pos)                      /**< (PIO_OWSR) Output Write Status Mask */
#define PIO_OWSR_P8                         PIO_OWSR_P8_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_OWSR_P8_Msk instead */
#define PIO_OWSR_P9_Pos                     9                                              /**< (PIO_OWSR) Output Write Status Position */
#define PIO_OWSR_P9_Msk                     (0x1U << PIO_OWSR_P9_Pos)                      /**< (PIO_OWSR) Output Write Status Mask */
#define PIO_OWSR_P9                         PIO_OWSR_P9_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_OWSR_P9_Msk instead */
#define PIO_OWSR_P10_Pos                    10                                             /**< (PIO_OWSR) Output Write Status Position */
#define PIO_OWSR_P10_Msk                    (0x1U << PIO_OWSR_P10_Pos)                     /**< (PIO_OWSR) Output Write Status Mask */
#define PIO_OWSR_P10                        PIO_OWSR_P10_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_OWSR_P10_Msk instead */
#define PIO_OWSR_P11_Pos                    11                                             /**< (PIO_OWSR) Output Write Status Position */
#define PIO_OWSR_P11_Msk                    (0x1U << PIO_OWSR_P11_Pos)                     /**< (PIO_OWSR) Output Write Status Mask */
#define PIO_OWSR_P11                        PIO_OWSR_P11_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_OWSR_P11_Msk instead */
#define PIO_OWSR_P12_Pos                    12                                             /**< (PIO_OWSR) Output Write Status Position */
#define PIO_OWSR_P12_Msk                    (0x1U << PIO_OWSR_P12_Pos)                     /**< (PIO_OWSR) Output Write Status Mask */
#define PIO_OWSR_P12                        PIO_OWSR_P12_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_OWSR_P12_Msk instead */
#define PIO_OWSR_P13_Pos                    13                                             /**< (PIO_OWSR) Output Write Status Position */
#define PIO_OWSR_P13_Msk                    (0x1U << PIO_OWSR_P13_Pos)                     /**< (PIO_OWSR) Output Write Status Mask */
#define PIO_OWSR_P13                        PIO_OWSR_P13_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_OWSR_P13_Msk instead */
#define PIO_OWSR_P14_Pos                    14                                             /**< (PIO_OWSR) Output Write Status Position */
#define PIO_OWSR_P14_Msk                    (0x1U << PIO_OWSR_P14_Pos)                     /**< (PIO_OWSR) Output Write Status Mask */
#define PIO_OWSR_P14                        PIO_OWSR_P14_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_OWSR_P14_Msk instead */
#define PIO_OWSR_P15_Pos                    15                                             /**< (PIO_OWSR) Output Write Status Position */
#define PIO_OWSR_P15_Msk                    (0x1U << PIO_OWSR_P15_Pos)                     /**< (PIO_OWSR) Output Write Status Mask */
#define PIO_OWSR_P15                        PIO_OWSR_P15_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_OWSR_P15_Msk instead */
#define PIO_OWSR_P16_Pos                    16                                             /**< (PIO_OWSR) Output Write Status Position */
#define PIO_OWSR_P16_Msk                    (0x1U << PIO_OWSR_P16_Pos)                     /**< (PIO_OWSR) Output Write Status Mask */
#define PIO_OWSR_P16                        PIO_OWSR_P16_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_OWSR_P16_Msk instead */
#define PIO_OWSR_P17_Pos                    17                                             /**< (PIO_OWSR) Output Write Status Position */
#define PIO_OWSR_P17_Msk                    (0x1U << PIO_OWSR_P17_Pos)                     /**< (PIO_OWSR) Output Write Status Mask */
#define PIO_OWSR_P17                        PIO_OWSR_P17_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_OWSR_P17_Msk instead */
#define PIO_OWSR_P18_Pos                    18                                             /**< (PIO_OWSR) Output Write Status Position */
#define PIO_OWSR_P18_Msk                    (0x1U << PIO_OWSR_P18_Pos)                     /**< (PIO_OWSR) Output Write Status Mask */
#define PIO_OWSR_P18                        PIO_OWSR_P18_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_OWSR_P18_Msk instead */
#define PIO_OWSR_P19_Pos                    19                                             /**< (PIO_OWSR) Output Write Status Position */
#define PIO_OWSR_P19_Msk                    (0x1U << PIO_OWSR_P19_Pos)                     /**< (PIO_OWSR) Output Write Status Mask */
#define PIO_OWSR_P19                        PIO_OWSR_P19_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_OWSR_P19_Msk instead */
#define PIO_OWSR_P20_Pos                    20                                             /**< (PIO_OWSR) Output Write Status Position */
#define PIO_OWSR_P20_Msk                    (0x1U << PIO_OWSR_P20_Pos)                     /**< (PIO_OWSR) Output Write Status Mask */
#define PIO_OWSR_P20                        PIO_OWSR_P20_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_OWSR_P20_Msk instead */
#define PIO_OWSR_P21_Pos                    21                                             /**< (PIO_OWSR) Output Write Status Position */
#define PIO_OWSR_P21_Msk                    (0x1U << PIO_OWSR_P21_Pos)                     /**< (PIO_OWSR) Output Write Status Mask */
#define PIO_OWSR_P21                        PIO_OWSR_P21_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_OWSR_P21_Msk instead */
#define PIO_OWSR_P22_Pos                    22                                             /**< (PIO_OWSR) Output Write Status Position */
#define PIO_OWSR_P22_Msk                    (0x1U << PIO_OWSR_P22_Pos)                     /**< (PIO_OWSR) Output Write Status Mask */
#define PIO_OWSR_P22                        PIO_OWSR_P22_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_OWSR_P22_Msk instead */
#define PIO_OWSR_P23_Pos                    23                                             /**< (PIO_OWSR) Output Write Status Position */
#define PIO_OWSR_P23_Msk                    (0x1U << PIO_OWSR_P23_Pos)                     /**< (PIO_OWSR) Output Write Status Mask */
#define PIO_OWSR_P23                        PIO_OWSR_P23_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_OWSR_P23_Msk instead */
#define PIO_OWSR_P24_Pos                    24                                             /**< (PIO_OWSR) Output Write Status Position */
#define PIO_OWSR_P24_Msk                    (0x1U << PIO_OWSR_P24_Pos)                     /**< (PIO_OWSR) Output Write Status Mask */
#define PIO_OWSR_P24                        PIO_OWSR_P24_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_OWSR_P24_Msk instead */
#define PIO_OWSR_P25_Pos                    25                                             /**< (PIO_OWSR) Output Write Status Position */
#define PIO_OWSR_P25_Msk                    (0x1U << PIO_OWSR_P25_Pos)                     /**< (PIO_OWSR) Output Write Status Mask */
#define PIO_OWSR_P25                        PIO_OWSR_P25_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_OWSR_P25_Msk instead */
#define PIO_OWSR_P26_Pos                    26                                             /**< (PIO_OWSR) Output Write Status Position */
#define PIO_OWSR_P26_Msk                    (0x1U << PIO_OWSR_P26_Pos)                     /**< (PIO_OWSR) Output Write Status Mask */
#define PIO_OWSR_P26                        PIO_OWSR_P26_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_OWSR_P26_Msk instead */
#define PIO_OWSR_P27_Pos                    27                                             /**< (PIO_OWSR) Output Write Status Position */
#define PIO_OWSR_P27_Msk                    (0x1U << PIO_OWSR_P27_Pos)                     /**< (PIO_OWSR) Output Write Status Mask */
#define PIO_OWSR_P27                        PIO_OWSR_P27_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_OWSR_P27_Msk instead */
#define PIO_OWSR_P28_Pos                    28                                             /**< (PIO_OWSR) Output Write Status Position */
#define PIO_OWSR_P28_Msk                    (0x1U << PIO_OWSR_P28_Pos)                     /**< (PIO_OWSR) Output Write Status Mask */
#define PIO_OWSR_P28                        PIO_OWSR_P28_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_OWSR_P28_Msk instead */
#define PIO_OWSR_P29_Pos                    29                                             /**< (PIO_OWSR) Output Write Status Position */
#define PIO_OWSR_P29_Msk                    (0x1U << PIO_OWSR_P29_Pos)                     /**< (PIO_OWSR) Output Write Status Mask */
#define PIO_OWSR_P29                        PIO_OWSR_P29_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_OWSR_P29_Msk instead */
#define PIO_OWSR_P30_Pos                    30                                             /**< (PIO_OWSR) Output Write Status Position */
#define PIO_OWSR_P30_Msk                    (0x1U << PIO_OWSR_P30_Pos)                     /**< (PIO_OWSR) Output Write Status Mask */
#define PIO_OWSR_P30                        PIO_OWSR_P30_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_OWSR_P30_Msk instead */
#define PIO_OWSR_P31_Pos                    31                                             /**< (PIO_OWSR) Output Write Status Position */
#define PIO_OWSR_P31_Msk                    (0x1U << PIO_OWSR_P31_Pos)                     /**< (PIO_OWSR) Output Write Status Mask */
#define PIO_OWSR_P31                        PIO_OWSR_P31_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_OWSR_P31_Msk instead */
#define PIO_OWSR_P_Pos                      0                                              /**< (PIO_OWSR Position) Output Write Status */
#define PIO_OWSR_P_Msk                      (0xFFFFFFFFU << PIO_OWSR_P_Pos)                /**< (PIO_OWSR Mask) P */
#define PIO_OWSR_P(value)                   (PIO_OWSR_P_Msk & ((value) << PIO_OWSR_P_Pos))  
#define PIO_OWSR_MASK                       (0xFFFFFFFFU)                                  /**< \deprecated (PIO_OWSR) Register MASK  (Use PIO_OWSR_Msk instead)  */
#define PIO_OWSR_Msk                        (0xFFFFFFFFU)                                  /**< (PIO_OWSR) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- PIO_AIMER : (PIO Offset: 0xb0) (/W 32) Additional Interrupt Modes Enable Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t P0:1;                      /**< bit:      0  Additional Interrupt Modes Enable        */
    uint32_t P1:1;                      /**< bit:      1  Additional Interrupt Modes Enable        */
    uint32_t P2:1;                      /**< bit:      2  Additional Interrupt Modes Enable        */
    uint32_t P3:1;                      /**< bit:      3  Additional Interrupt Modes Enable        */
    uint32_t P4:1;                      /**< bit:      4  Additional Interrupt Modes Enable        */
    uint32_t P5:1;                      /**< bit:      5  Additional Interrupt Modes Enable        */
    uint32_t P6:1;                      /**< bit:      6  Additional Interrupt Modes Enable        */
    uint32_t P7:1;                      /**< bit:      7  Additional Interrupt Modes Enable        */
    uint32_t P8:1;                      /**< bit:      8  Additional Interrupt Modes Enable        */
    uint32_t P9:1;                      /**< bit:      9  Additional Interrupt Modes Enable        */
    uint32_t P10:1;                     /**< bit:     10  Additional Interrupt Modes Enable        */
    uint32_t P11:1;                     /**< bit:     11  Additional Interrupt Modes Enable        */
    uint32_t P12:1;                     /**< bit:     12  Additional Interrupt Modes Enable        */
    uint32_t P13:1;                     /**< bit:     13  Additional Interrupt Modes Enable        */
    uint32_t P14:1;                     /**< bit:     14  Additional Interrupt Modes Enable        */
    uint32_t P15:1;                     /**< bit:     15  Additional Interrupt Modes Enable        */
    uint32_t P16:1;                     /**< bit:     16  Additional Interrupt Modes Enable        */
    uint32_t P17:1;                     /**< bit:     17  Additional Interrupt Modes Enable        */
    uint32_t P18:1;                     /**< bit:     18  Additional Interrupt Modes Enable        */
    uint32_t P19:1;                     /**< bit:     19  Additional Interrupt Modes Enable        */
    uint32_t P20:1;                     /**< bit:     20  Additional Interrupt Modes Enable        */
    uint32_t P21:1;                     /**< bit:     21  Additional Interrupt Modes Enable        */
    uint32_t P22:1;                     /**< bit:     22  Additional Interrupt Modes Enable        */
    uint32_t P23:1;                     /**< bit:     23  Additional Interrupt Modes Enable        */
    uint32_t P24:1;                     /**< bit:     24  Additional Interrupt Modes Enable        */
    uint32_t P25:1;                     /**< bit:     25  Additional Interrupt Modes Enable        */
    uint32_t P26:1;                     /**< bit:     26  Additional Interrupt Modes Enable        */
    uint32_t P27:1;                     /**< bit:     27  Additional Interrupt Modes Enable        */
    uint32_t P28:1;                     /**< bit:     28  Additional Interrupt Modes Enable        */
    uint32_t P29:1;                     /**< bit:     29  Additional Interrupt Modes Enable        */
    uint32_t P30:1;                     /**< bit:     30  Additional Interrupt Modes Enable        */
    uint32_t P31:1;                     /**< bit:     31  Additional Interrupt Modes Enable        */
  } bit;                                /**< Structure used for bit  access */
  struct {
    uint32_t P:32;                      /**< bit:  0..31  Additional Interrupt Modes Enable        */
  } vec;                                /**< Structure used for vec  access  */
  uint32_t reg;                         /**< Type used for register access */
} PIO_AIMER_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define PIO_AIMER_OFFSET                    (0xB0)                                        /**<  (PIO_AIMER) Additional Interrupt Modes Enable Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define PIO_AIMER_P0_Pos                    0                                              /**< (PIO_AIMER) Additional Interrupt Modes Enable Position */
#define PIO_AIMER_P0_Msk                    (0x1U << PIO_AIMER_P0_Pos)                     /**< (PIO_AIMER) Additional Interrupt Modes Enable Mask */
#define PIO_AIMER_P0                        PIO_AIMER_P0_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_AIMER_P0_Msk instead */
#define PIO_AIMER_P1_Pos                    1                                              /**< (PIO_AIMER) Additional Interrupt Modes Enable Position */
#define PIO_AIMER_P1_Msk                    (0x1U << PIO_AIMER_P1_Pos)                     /**< (PIO_AIMER) Additional Interrupt Modes Enable Mask */
#define PIO_AIMER_P1                        PIO_AIMER_P1_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_AIMER_P1_Msk instead */
#define PIO_AIMER_P2_Pos                    2                                              /**< (PIO_AIMER) Additional Interrupt Modes Enable Position */
#define PIO_AIMER_P2_Msk                    (0x1U << PIO_AIMER_P2_Pos)                     /**< (PIO_AIMER) Additional Interrupt Modes Enable Mask */
#define PIO_AIMER_P2                        PIO_AIMER_P2_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_AIMER_P2_Msk instead */
#define PIO_AIMER_P3_Pos                    3                                              /**< (PIO_AIMER) Additional Interrupt Modes Enable Position */
#define PIO_AIMER_P3_Msk                    (0x1U << PIO_AIMER_P3_Pos)                     /**< (PIO_AIMER) Additional Interrupt Modes Enable Mask */
#define PIO_AIMER_P3                        PIO_AIMER_P3_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_AIMER_P3_Msk instead */
#define PIO_AIMER_P4_Pos                    4                                              /**< (PIO_AIMER) Additional Interrupt Modes Enable Position */
#define PIO_AIMER_P4_Msk                    (0x1U << PIO_AIMER_P4_Pos)                     /**< (PIO_AIMER) Additional Interrupt Modes Enable Mask */
#define PIO_AIMER_P4                        PIO_AIMER_P4_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_AIMER_P4_Msk instead */
#define PIO_AIMER_P5_Pos                    5                                              /**< (PIO_AIMER) Additional Interrupt Modes Enable Position */
#define PIO_AIMER_P5_Msk                    (0x1U << PIO_AIMER_P5_Pos)                     /**< (PIO_AIMER) Additional Interrupt Modes Enable Mask */
#define PIO_AIMER_P5                        PIO_AIMER_P5_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_AIMER_P5_Msk instead */
#define PIO_AIMER_P6_Pos                    6                                              /**< (PIO_AIMER) Additional Interrupt Modes Enable Position */
#define PIO_AIMER_P6_Msk                    (0x1U << PIO_AIMER_P6_Pos)                     /**< (PIO_AIMER) Additional Interrupt Modes Enable Mask */
#define PIO_AIMER_P6                        PIO_AIMER_P6_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_AIMER_P6_Msk instead */
#define PIO_AIMER_P7_Pos                    7                                              /**< (PIO_AIMER) Additional Interrupt Modes Enable Position */
#define PIO_AIMER_P7_Msk                    (0x1U << PIO_AIMER_P7_Pos)                     /**< (PIO_AIMER) Additional Interrupt Modes Enable Mask */
#define PIO_AIMER_P7                        PIO_AIMER_P7_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_AIMER_P7_Msk instead */
#define PIO_AIMER_P8_Pos                    8                                              /**< (PIO_AIMER) Additional Interrupt Modes Enable Position */
#define PIO_AIMER_P8_Msk                    (0x1U << PIO_AIMER_P8_Pos)                     /**< (PIO_AIMER) Additional Interrupt Modes Enable Mask */
#define PIO_AIMER_P8                        PIO_AIMER_P8_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_AIMER_P8_Msk instead */
#define PIO_AIMER_P9_Pos                    9                                              /**< (PIO_AIMER) Additional Interrupt Modes Enable Position */
#define PIO_AIMER_P9_Msk                    (0x1U << PIO_AIMER_P9_Pos)                     /**< (PIO_AIMER) Additional Interrupt Modes Enable Mask */
#define PIO_AIMER_P9                        PIO_AIMER_P9_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_AIMER_P9_Msk instead */
#define PIO_AIMER_P10_Pos                   10                                             /**< (PIO_AIMER) Additional Interrupt Modes Enable Position */
#define PIO_AIMER_P10_Msk                   (0x1U << PIO_AIMER_P10_Pos)                    /**< (PIO_AIMER) Additional Interrupt Modes Enable Mask */
#define PIO_AIMER_P10                       PIO_AIMER_P10_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_AIMER_P10_Msk instead */
#define PIO_AIMER_P11_Pos                   11                                             /**< (PIO_AIMER) Additional Interrupt Modes Enable Position */
#define PIO_AIMER_P11_Msk                   (0x1U << PIO_AIMER_P11_Pos)                    /**< (PIO_AIMER) Additional Interrupt Modes Enable Mask */
#define PIO_AIMER_P11                       PIO_AIMER_P11_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_AIMER_P11_Msk instead */
#define PIO_AIMER_P12_Pos                   12                                             /**< (PIO_AIMER) Additional Interrupt Modes Enable Position */
#define PIO_AIMER_P12_Msk                   (0x1U << PIO_AIMER_P12_Pos)                    /**< (PIO_AIMER) Additional Interrupt Modes Enable Mask */
#define PIO_AIMER_P12                       PIO_AIMER_P12_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_AIMER_P12_Msk instead */
#define PIO_AIMER_P13_Pos                   13                                             /**< (PIO_AIMER) Additional Interrupt Modes Enable Position */
#define PIO_AIMER_P13_Msk                   (0x1U << PIO_AIMER_P13_Pos)                    /**< (PIO_AIMER) Additional Interrupt Modes Enable Mask */
#define PIO_AIMER_P13                       PIO_AIMER_P13_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_AIMER_P13_Msk instead */
#define PIO_AIMER_P14_Pos                   14                                             /**< (PIO_AIMER) Additional Interrupt Modes Enable Position */
#define PIO_AIMER_P14_Msk                   (0x1U << PIO_AIMER_P14_Pos)                    /**< (PIO_AIMER) Additional Interrupt Modes Enable Mask */
#define PIO_AIMER_P14                       PIO_AIMER_P14_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_AIMER_P14_Msk instead */
#define PIO_AIMER_P15_Pos                   15                                             /**< (PIO_AIMER) Additional Interrupt Modes Enable Position */
#define PIO_AIMER_P15_Msk                   (0x1U << PIO_AIMER_P15_Pos)                    /**< (PIO_AIMER) Additional Interrupt Modes Enable Mask */
#define PIO_AIMER_P15                       PIO_AIMER_P15_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_AIMER_P15_Msk instead */
#define PIO_AIMER_P16_Pos                   16                                             /**< (PIO_AIMER) Additional Interrupt Modes Enable Position */
#define PIO_AIMER_P16_Msk                   (0x1U << PIO_AIMER_P16_Pos)                    /**< (PIO_AIMER) Additional Interrupt Modes Enable Mask */
#define PIO_AIMER_P16                       PIO_AIMER_P16_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_AIMER_P16_Msk instead */
#define PIO_AIMER_P17_Pos                   17                                             /**< (PIO_AIMER) Additional Interrupt Modes Enable Position */
#define PIO_AIMER_P17_Msk                   (0x1U << PIO_AIMER_P17_Pos)                    /**< (PIO_AIMER) Additional Interrupt Modes Enable Mask */
#define PIO_AIMER_P17                       PIO_AIMER_P17_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_AIMER_P17_Msk instead */
#define PIO_AIMER_P18_Pos                   18                                             /**< (PIO_AIMER) Additional Interrupt Modes Enable Position */
#define PIO_AIMER_P18_Msk                   (0x1U << PIO_AIMER_P18_Pos)                    /**< (PIO_AIMER) Additional Interrupt Modes Enable Mask */
#define PIO_AIMER_P18                       PIO_AIMER_P18_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_AIMER_P18_Msk instead */
#define PIO_AIMER_P19_Pos                   19                                             /**< (PIO_AIMER) Additional Interrupt Modes Enable Position */
#define PIO_AIMER_P19_Msk                   (0x1U << PIO_AIMER_P19_Pos)                    /**< (PIO_AIMER) Additional Interrupt Modes Enable Mask */
#define PIO_AIMER_P19                       PIO_AIMER_P19_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_AIMER_P19_Msk instead */
#define PIO_AIMER_P20_Pos                   20                                             /**< (PIO_AIMER) Additional Interrupt Modes Enable Position */
#define PIO_AIMER_P20_Msk                   (0x1U << PIO_AIMER_P20_Pos)                    /**< (PIO_AIMER) Additional Interrupt Modes Enable Mask */
#define PIO_AIMER_P20                       PIO_AIMER_P20_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_AIMER_P20_Msk instead */
#define PIO_AIMER_P21_Pos                   21                                             /**< (PIO_AIMER) Additional Interrupt Modes Enable Position */
#define PIO_AIMER_P21_Msk                   (0x1U << PIO_AIMER_P21_Pos)                    /**< (PIO_AIMER) Additional Interrupt Modes Enable Mask */
#define PIO_AIMER_P21                       PIO_AIMER_P21_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_AIMER_P21_Msk instead */
#define PIO_AIMER_P22_Pos                   22                                             /**< (PIO_AIMER) Additional Interrupt Modes Enable Position */
#define PIO_AIMER_P22_Msk                   (0x1U << PIO_AIMER_P22_Pos)                    /**< (PIO_AIMER) Additional Interrupt Modes Enable Mask */
#define PIO_AIMER_P22                       PIO_AIMER_P22_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_AIMER_P22_Msk instead */
#define PIO_AIMER_P23_Pos                   23                                             /**< (PIO_AIMER) Additional Interrupt Modes Enable Position */
#define PIO_AIMER_P23_Msk                   (0x1U << PIO_AIMER_P23_Pos)                    /**< (PIO_AIMER) Additional Interrupt Modes Enable Mask */
#define PIO_AIMER_P23                       PIO_AIMER_P23_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_AIMER_P23_Msk instead */
#define PIO_AIMER_P24_Pos                   24                                             /**< (PIO_AIMER) Additional Interrupt Modes Enable Position */
#define PIO_AIMER_P24_Msk                   (0x1U << PIO_AIMER_P24_Pos)                    /**< (PIO_AIMER) Additional Interrupt Modes Enable Mask */
#define PIO_AIMER_P24                       PIO_AIMER_P24_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_AIMER_P24_Msk instead */
#define PIO_AIMER_P25_Pos                   25                                             /**< (PIO_AIMER) Additional Interrupt Modes Enable Position */
#define PIO_AIMER_P25_Msk                   (0x1U << PIO_AIMER_P25_Pos)                    /**< (PIO_AIMER) Additional Interrupt Modes Enable Mask */
#define PIO_AIMER_P25                       PIO_AIMER_P25_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_AIMER_P25_Msk instead */
#define PIO_AIMER_P26_Pos                   26                                             /**< (PIO_AIMER) Additional Interrupt Modes Enable Position */
#define PIO_AIMER_P26_Msk                   (0x1U << PIO_AIMER_P26_Pos)                    /**< (PIO_AIMER) Additional Interrupt Modes Enable Mask */
#define PIO_AIMER_P26                       PIO_AIMER_P26_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_AIMER_P26_Msk instead */
#define PIO_AIMER_P27_Pos                   27                                             /**< (PIO_AIMER) Additional Interrupt Modes Enable Position */
#define PIO_AIMER_P27_Msk                   (0x1U << PIO_AIMER_P27_Pos)                    /**< (PIO_AIMER) Additional Interrupt Modes Enable Mask */
#define PIO_AIMER_P27                       PIO_AIMER_P27_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_AIMER_P27_Msk instead */
#define PIO_AIMER_P28_Pos                   28                                             /**< (PIO_AIMER) Additional Interrupt Modes Enable Position */
#define PIO_AIMER_P28_Msk                   (0x1U << PIO_AIMER_P28_Pos)                    /**< (PIO_AIMER) Additional Interrupt Modes Enable Mask */
#define PIO_AIMER_P28                       PIO_AIMER_P28_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_AIMER_P28_Msk instead */
#define PIO_AIMER_P29_Pos                   29                                             /**< (PIO_AIMER) Additional Interrupt Modes Enable Position */
#define PIO_AIMER_P29_Msk                   (0x1U << PIO_AIMER_P29_Pos)                    /**< (PIO_AIMER) Additional Interrupt Modes Enable Mask */
#define PIO_AIMER_P29                       PIO_AIMER_P29_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_AIMER_P29_Msk instead */
#define PIO_AIMER_P30_Pos                   30                                             /**< (PIO_AIMER) Additional Interrupt Modes Enable Position */
#define PIO_AIMER_P30_Msk                   (0x1U << PIO_AIMER_P30_Pos)                    /**< (PIO_AIMER) Additional Interrupt Modes Enable Mask */
#define PIO_AIMER_P30                       PIO_AIMER_P30_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_AIMER_P30_Msk instead */
#define PIO_AIMER_P31_Pos                   31                                             /**< (PIO_AIMER) Additional Interrupt Modes Enable Position */
#define PIO_AIMER_P31_Msk                   (0x1U << PIO_AIMER_P31_Pos)                    /**< (PIO_AIMER) Additional Interrupt Modes Enable Mask */
#define PIO_AIMER_P31                       PIO_AIMER_P31_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_AIMER_P31_Msk instead */
#define PIO_AIMER_P_Pos                     0                                              /**< (PIO_AIMER Position) Additional Interrupt Modes Enable */
#define PIO_AIMER_P_Msk                     (0xFFFFFFFFU << PIO_AIMER_P_Pos)               /**< (PIO_AIMER Mask) P */
#define PIO_AIMER_P(value)                  (PIO_AIMER_P_Msk & ((value) << PIO_AIMER_P_Pos))  
#define PIO_AIMER_MASK                      (0xFFFFFFFFU)                                  /**< \deprecated (PIO_AIMER) Register MASK  (Use PIO_AIMER_Msk instead)  */
#define PIO_AIMER_Msk                       (0xFFFFFFFFU)                                  /**< (PIO_AIMER) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- PIO_AIMDR : (PIO Offset: 0xb4) (/W 32) Additional Interrupt Modes Disable Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t P0:1;                      /**< bit:      0  Additional Interrupt Modes Disable       */
    uint32_t P1:1;                      /**< bit:      1  Additional Interrupt Modes Disable       */
    uint32_t P2:1;                      /**< bit:      2  Additional Interrupt Modes Disable       */
    uint32_t P3:1;                      /**< bit:      3  Additional Interrupt Modes Disable       */
    uint32_t P4:1;                      /**< bit:      4  Additional Interrupt Modes Disable       */
    uint32_t P5:1;                      /**< bit:      5  Additional Interrupt Modes Disable       */
    uint32_t P6:1;                      /**< bit:      6  Additional Interrupt Modes Disable       */
    uint32_t P7:1;                      /**< bit:      7  Additional Interrupt Modes Disable       */
    uint32_t P8:1;                      /**< bit:      8  Additional Interrupt Modes Disable       */
    uint32_t P9:1;                      /**< bit:      9  Additional Interrupt Modes Disable       */
    uint32_t P10:1;                     /**< bit:     10  Additional Interrupt Modes Disable       */
    uint32_t P11:1;                     /**< bit:     11  Additional Interrupt Modes Disable       */
    uint32_t P12:1;                     /**< bit:     12  Additional Interrupt Modes Disable       */
    uint32_t P13:1;                     /**< bit:     13  Additional Interrupt Modes Disable       */
    uint32_t P14:1;                     /**< bit:     14  Additional Interrupt Modes Disable       */
    uint32_t P15:1;                     /**< bit:     15  Additional Interrupt Modes Disable       */
    uint32_t P16:1;                     /**< bit:     16  Additional Interrupt Modes Disable       */
    uint32_t P17:1;                     /**< bit:     17  Additional Interrupt Modes Disable       */
    uint32_t P18:1;                     /**< bit:     18  Additional Interrupt Modes Disable       */
    uint32_t P19:1;                     /**< bit:     19  Additional Interrupt Modes Disable       */
    uint32_t P20:1;                     /**< bit:     20  Additional Interrupt Modes Disable       */
    uint32_t P21:1;                     /**< bit:     21  Additional Interrupt Modes Disable       */
    uint32_t P22:1;                     /**< bit:     22  Additional Interrupt Modes Disable       */
    uint32_t P23:1;                     /**< bit:     23  Additional Interrupt Modes Disable       */
    uint32_t P24:1;                     /**< bit:     24  Additional Interrupt Modes Disable       */
    uint32_t P25:1;                     /**< bit:     25  Additional Interrupt Modes Disable       */
    uint32_t P26:1;                     /**< bit:     26  Additional Interrupt Modes Disable       */
    uint32_t P27:1;                     /**< bit:     27  Additional Interrupt Modes Disable       */
    uint32_t P28:1;                     /**< bit:     28  Additional Interrupt Modes Disable       */
    uint32_t P29:1;                     /**< bit:     29  Additional Interrupt Modes Disable       */
    uint32_t P30:1;                     /**< bit:     30  Additional Interrupt Modes Disable       */
    uint32_t P31:1;                     /**< bit:     31  Additional Interrupt Modes Disable       */
  } bit;                                /**< Structure used for bit  access */
  struct {
    uint32_t P:32;                      /**< bit:  0..31  Additional Interrupt Modes Disable       */
  } vec;                                /**< Structure used for vec  access  */
  uint32_t reg;                         /**< Type used for register access */
} PIO_AIMDR_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define PIO_AIMDR_OFFSET                    (0xB4)                                        /**<  (PIO_AIMDR) Additional Interrupt Modes Disable Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define PIO_AIMDR_P0_Pos                    0                                              /**< (PIO_AIMDR) Additional Interrupt Modes Disable Position */
#define PIO_AIMDR_P0_Msk                    (0x1U << PIO_AIMDR_P0_Pos)                     /**< (PIO_AIMDR) Additional Interrupt Modes Disable Mask */
#define PIO_AIMDR_P0                        PIO_AIMDR_P0_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_AIMDR_P0_Msk instead */
#define PIO_AIMDR_P1_Pos                    1                                              /**< (PIO_AIMDR) Additional Interrupt Modes Disable Position */
#define PIO_AIMDR_P1_Msk                    (0x1U << PIO_AIMDR_P1_Pos)                     /**< (PIO_AIMDR) Additional Interrupt Modes Disable Mask */
#define PIO_AIMDR_P1                        PIO_AIMDR_P1_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_AIMDR_P1_Msk instead */
#define PIO_AIMDR_P2_Pos                    2                                              /**< (PIO_AIMDR) Additional Interrupt Modes Disable Position */
#define PIO_AIMDR_P2_Msk                    (0x1U << PIO_AIMDR_P2_Pos)                     /**< (PIO_AIMDR) Additional Interrupt Modes Disable Mask */
#define PIO_AIMDR_P2                        PIO_AIMDR_P2_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_AIMDR_P2_Msk instead */
#define PIO_AIMDR_P3_Pos                    3                                              /**< (PIO_AIMDR) Additional Interrupt Modes Disable Position */
#define PIO_AIMDR_P3_Msk                    (0x1U << PIO_AIMDR_P3_Pos)                     /**< (PIO_AIMDR) Additional Interrupt Modes Disable Mask */
#define PIO_AIMDR_P3                        PIO_AIMDR_P3_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_AIMDR_P3_Msk instead */
#define PIO_AIMDR_P4_Pos                    4                                              /**< (PIO_AIMDR) Additional Interrupt Modes Disable Position */
#define PIO_AIMDR_P4_Msk                    (0x1U << PIO_AIMDR_P4_Pos)                     /**< (PIO_AIMDR) Additional Interrupt Modes Disable Mask */
#define PIO_AIMDR_P4                        PIO_AIMDR_P4_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_AIMDR_P4_Msk instead */
#define PIO_AIMDR_P5_Pos                    5                                              /**< (PIO_AIMDR) Additional Interrupt Modes Disable Position */
#define PIO_AIMDR_P5_Msk                    (0x1U << PIO_AIMDR_P5_Pos)                     /**< (PIO_AIMDR) Additional Interrupt Modes Disable Mask */
#define PIO_AIMDR_P5                        PIO_AIMDR_P5_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_AIMDR_P5_Msk instead */
#define PIO_AIMDR_P6_Pos                    6                                              /**< (PIO_AIMDR) Additional Interrupt Modes Disable Position */
#define PIO_AIMDR_P6_Msk                    (0x1U << PIO_AIMDR_P6_Pos)                     /**< (PIO_AIMDR) Additional Interrupt Modes Disable Mask */
#define PIO_AIMDR_P6                        PIO_AIMDR_P6_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_AIMDR_P6_Msk instead */
#define PIO_AIMDR_P7_Pos                    7                                              /**< (PIO_AIMDR) Additional Interrupt Modes Disable Position */
#define PIO_AIMDR_P7_Msk                    (0x1U << PIO_AIMDR_P7_Pos)                     /**< (PIO_AIMDR) Additional Interrupt Modes Disable Mask */
#define PIO_AIMDR_P7                        PIO_AIMDR_P7_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_AIMDR_P7_Msk instead */
#define PIO_AIMDR_P8_Pos                    8                                              /**< (PIO_AIMDR) Additional Interrupt Modes Disable Position */
#define PIO_AIMDR_P8_Msk                    (0x1U << PIO_AIMDR_P8_Pos)                     /**< (PIO_AIMDR) Additional Interrupt Modes Disable Mask */
#define PIO_AIMDR_P8                        PIO_AIMDR_P8_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_AIMDR_P8_Msk instead */
#define PIO_AIMDR_P9_Pos                    9                                              /**< (PIO_AIMDR) Additional Interrupt Modes Disable Position */
#define PIO_AIMDR_P9_Msk                    (0x1U << PIO_AIMDR_P9_Pos)                     /**< (PIO_AIMDR) Additional Interrupt Modes Disable Mask */
#define PIO_AIMDR_P9                        PIO_AIMDR_P9_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_AIMDR_P9_Msk instead */
#define PIO_AIMDR_P10_Pos                   10                                             /**< (PIO_AIMDR) Additional Interrupt Modes Disable Position */
#define PIO_AIMDR_P10_Msk                   (0x1U << PIO_AIMDR_P10_Pos)                    /**< (PIO_AIMDR) Additional Interrupt Modes Disable Mask */
#define PIO_AIMDR_P10                       PIO_AIMDR_P10_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_AIMDR_P10_Msk instead */
#define PIO_AIMDR_P11_Pos                   11                                             /**< (PIO_AIMDR) Additional Interrupt Modes Disable Position */
#define PIO_AIMDR_P11_Msk                   (0x1U << PIO_AIMDR_P11_Pos)                    /**< (PIO_AIMDR) Additional Interrupt Modes Disable Mask */
#define PIO_AIMDR_P11                       PIO_AIMDR_P11_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_AIMDR_P11_Msk instead */
#define PIO_AIMDR_P12_Pos                   12                                             /**< (PIO_AIMDR) Additional Interrupt Modes Disable Position */
#define PIO_AIMDR_P12_Msk                   (0x1U << PIO_AIMDR_P12_Pos)                    /**< (PIO_AIMDR) Additional Interrupt Modes Disable Mask */
#define PIO_AIMDR_P12                       PIO_AIMDR_P12_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_AIMDR_P12_Msk instead */
#define PIO_AIMDR_P13_Pos                   13                                             /**< (PIO_AIMDR) Additional Interrupt Modes Disable Position */
#define PIO_AIMDR_P13_Msk                   (0x1U << PIO_AIMDR_P13_Pos)                    /**< (PIO_AIMDR) Additional Interrupt Modes Disable Mask */
#define PIO_AIMDR_P13                       PIO_AIMDR_P13_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_AIMDR_P13_Msk instead */
#define PIO_AIMDR_P14_Pos                   14                                             /**< (PIO_AIMDR) Additional Interrupt Modes Disable Position */
#define PIO_AIMDR_P14_Msk                   (0x1U << PIO_AIMDR_P14_Pos)                    /**< (PIO_AIMDR) Additional Interrupt Modes Disable Mask */
#define PIO_AIMDR_P14                       PIO_AIMDR_P14_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_AIMDR_P14_Msk instead */
#define PIO_AIMDR_P15_Pos                   15                                             /**< (PIO_AIMDR) Additional Interrupt Modes Disable Position */
#define PIO_AIMDR_P15_Msk                   (0x1U << PIO_AIMDR_P15_Pos)                    /**< (PIO_AIMDR) Additional Interrupt Modes Disable Mask */
#define PIO_AIMDR_P15                       PIO_AIMDR_P15_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_AIMDR_P15_Msk instead */
#define PIO_AIMDR_P16_Pos                   16                                             /**< (PIO_AIMDR) Additional Interrupt Modes Disable Position */
#define PIO_AIMDR_P16_Msk                   (0x1U << PIO_AIMDR_P16_Pos)                    /**< (PIO_AIMDR) Additional Interrupt Modes Disable Mask */
#define PIO_AIMDR_P16                       PIO_AIMDR_P16_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_AIMDR_P16_Msk instead */
#define PIO_AIMDR_P17_Pos                   17                                             /**< (PIO_AIMDR) Additional Interrupt Modes Disable Position */
#define PIO_AIMDR_P17_Msk                   (0x1U << PIO_AIMDR_P17_Pos)                    /**< (PIO_AIMDR) Additional Interrupt Modes Disable Mask */
#define PIO_AIMDR_P17                       PIO_AIMDR_P17_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_AIMDR_P17_Msk instead */
#define PIO_AIMDR_P18_Pos                   18                                             /**< (PIO_AIMDR) Additional Interrupt Modes Disable Position */
#define PIO_AIMDR_P18_Msk                   (0x1U << PIO_AIMDR_P18_Pos)                    /**< (PIO_AIMDR) Additional Interrupt Modes Disable Mask */
#define PIO_AIMDR_P18                       PIO_AIMDR_P18_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_AIMDR_P18_Msk instead */
#define PIO_AIMDR_P19_Pos                   19                                             /**< (PIO_AIMDR) Additional Interrupt Modes Disable Position */
#define PIO_AIMDR_P19_Msk                   (0x1U << PIO_AIMDR_P19_Pos)                    /**< (PIO_AIMDR) Additional Interrupt Modes Disable Mask */
#define PIO_AIMDR_P19                       PIO_AIMDR_P19_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_AIMDR_P19_Msk instead */
#define PIO_AIMDR_P20_Pos                   20                                             /**< (PIO_AIMDR) Additional Interrupt Modes Disable Position */
#define PIO_AIMDR_P20_Msk                   (0x1U << PIO_AIMDR_P20_Pos)                    /**< (PIO_AIMDR) Additional Interrupt Modes Disable Mask */
#define PIO_AIMDR_P20                       PIO_AIMDR_P20_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_AIMDR_P20_Msk instead */
#define PIO_AIMDR_P21_Pos                   21                                             /**< (PIO_AIMDR) Additional Interrupt Modes Disable Position */
#define PIO_AIMDR_P21_Msk                   (0x1U << PIO_AIMDR_P21_Pos)                    /**< (PIO_AIMDR) Additional Interrupt Modes Disable Mask */
#define PIO_AIMDR_P21                       PIO_AIMDR_P21_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_AIMDR_P21_Msk instead */
#define PIO_AIMDR_P22_Pos                   22                                             /**< (PIO_AIMDR) Additional Interrupt Modes Disable Position */
#define PIO_AIMDR_P22_Msk                   (0x1U << PIO_AIMDR_P22_Pos)                    /**< (PIO_AIMDR) Additional Interrupt Modes Disable Mask */
#define PIO_AIMDR_P22                       PIO_AIMDR_P22_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_AIMDR_P22_Msk instead */
#define PIO_AIMDR_P23_Pos                   23                                             /**< (PIO_AIMDR) Additional Interrupt Modes Disable Position */
#define PIO_AIMDR_P23_Msk                   (0x1U << PIO_AIMDR_P23_Pos)                    /**< (PIO_AIMDR) Additional Interrupt Modes Disable Mask */
#define PIO_AIMDR_P23                       PIO_AIMDR_P23_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_AIMDR_P23_Msk instead */
#define PIO_AIMDR_P24_Pos                   24                                             /**< (PIO_AIMDR) Additional Interrupt Modes Disable Position */
#define PIO_AIMDR_P24_Msk                   (0x1U << PIO_AIMDR_P24_Pos)                    /**< (PIO_AIMDR) Additional Interrupt Modes Disable Mask */
#define PIO_AIMDR_P24                       PIO_AIMDR_P24_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_AIMDR_P24_Msk instead */
#define PIO_AIMDR_P25_Pos                   25                                             /**< (PIO_AIMDR) Additional Interrupt Modes Disable Position */
#define PIO_AIMDR_P25_Msk                   (0x1U << PIO_AIMDR_P25_Pos)                    /**< (PIO_AIMDR) Additional Interrupt Modes Disable Mask */
#define PIO_AIMDR_P25                       PIO_AIMDR_P25_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_AIMDR_P25_Msk instead */
#define PIO_AIMDR_P26_Pos                   26                                             /**< (PIO_AIMDR) Additional Interrupt Modes Disable Position */
#define PIO_AIMDR_P26_Msk                   (0x1U << PIO_AIMDR_P26_Pos)                    /**< (PIO_AIMDR) Additional Interrupt Modes Disable Mask */
#define PIO_AIMDR_P26                       PIO_AIMDR_P26_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_AIMDR_P26_Msk instead */
#define PIO_AIMDR_P27_Pos                   27                                             /**< (PIO_AIMDR) Additional Interrupt Modes Disable Position */
#define PIO_AIMDR_P27_Msk                   (0x1U << PIO_AIMDR_P27_Pos)                    /**< (PIO_AIMDR) Additional Interrupt Modes Disable Mask */
#define PIO_AIMDR_P27                       PIO_AIMDR_P27_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_AIMDR_P27_Msk instead */
#define PIO_AIMDR_P28_Pos                   28                                             /**< (PIO_AIMDR) Additional Interrupt Modes Disable Position */
#define PIO_AIMDR_P28_Msk                   (0x1U << PIO_AIMDR_P28_Pos)                    /**< (PIO_AIMDR) Additional Interrupt Modes Disable Mask */
#define PIO_AIMDR_P28                       PIO_AIMDR_P28_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_AIMDR_P28_Msk instead */
#define PIO_AIMDR_P29_Pos                   29                                             /**< (PIO_AIMDR) Additional Interrupt Modes Disable Position */
#define PIO_AIMDR_P29_Msk                   (0x1U << PIO_AIMDR_P29_Pos)                    /**< (PIO_AIMDR) Additional Interrupt Modes Disable Mask */
#define PIO_AIMDR_P29                       PIO_AIMDR_P29_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_AIMDR_P29_Msk instead */
#define PIO_AIMDR_P30_Pos                   30                                             /**< (PIO_AIMDR) Additional Interrupt Modes Disable Position */
#define PIO_AIMDR_P30_Msk                   (0x1U << PIO_AIMDR_P30_Pos)                    /**< (PIO_AIMDR) Additional Interrupt Modes Disable Mask */
#define PIO_AIMDR_P30                       PIO_AIMDR_P30_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_AIMDR_P30_Msk instead */
#define PIO_AIMDR_P31_Pos                   31                                             /**< (PIO_AIMDR) Additional Interrupt Modes Disable Position */
#define PIO_AIMDR_P31_Msk                   (0x1U << PIO_AIMDR_P31_Pos)                    /**< (PIO_AIMDR) Additional Interrupt Modes Disable Mask */
#define PIO_AIMDR_P31                       PIO_AIMDR_P31_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_AIMDR_P31_Msk instead */
#define PIO_AIMDR_P_Pos                     0                                              /**< (PIO_AIMDR Position) Additional Interrupt Modes Disable */
#define PIO_AIMDR_P_Msk                     (0xFFFFFFFFU << PIO_AIMDR_P_Pos)               /**< (PIO_AIMDR Mask) P */
#define PIO_AIMDR_P(value)                  (PIO_AIMDR_P_Msk & ((value) << PIO_AIMDR_P_Pos))  
#define PIO_AIMDR_MASK                      (0xFFFFFFFFU)                                  /**< \deprecated (PIO_AIMDR) Register MASK  (Use PIO_AIMDR_Msk instead)  */
#define PIO_AIMDR_Msk                       (0xFFFFFFFFU)                                  /**< (PIO_AIMDR) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- PIO_AIMMR : (PIO Offset: 0xb8) (R/ 32) Additional Interrupt Modes Mask Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t P0:1;                      /**< bit:      0  IO Line Index                            */
    uint32_t P1:1;                      /**< bit:      1  IO Line Index                            */
    uint32_t P2:1;                      /**< bit:      2  IO Line Index                            */
    uint32_t P3:1;                      /**< bit:      3  IO Line Index                            */
    uint32_t P4:1;                      /**< bit:      4  IO Line Index                            */
    uint32_t P5:1;                      /**< bit:      5  IO Line Index                            */
    uint32_t P6:1;                      /**< bit:      6  IO Line Index                            */
    uint32_t P7:1;                      /**< bit:      7  IO Line Index                            */
    uint32_t P8:1;                      /**< bit:      8  IO Line Index                            */
    uint32_t P9:1;                      /**< bit:      9  IO Line Index                            */
    uint32_t P10:1;                     /**< bit:     10  IO Line Index                            */
    uint32_t P11:1;                     /**< bit:     11  IO Line Index                            */
    uint32_t P12:1;                     /**< bit:     12  IO Line Index                            */
    uint32_t P13:1;                     /**< bit:     13  IO Line Index                            */
    uint32_t P14:1;                     /**< bit:     14  IO Line Index                            */
    uint32_t P15:1;                     /**< bit:     15  IO Line Index                            */
    uint32_t P16:1;                     /**< bit:     16  IO Line Index                            */
    uint32_t P17:1;                     /**< bit:     17  IO Line Index                            */
    uint32_t P18:1;                     /**< bit:     18  IO Line Index                            */
    uint32_t P19:1;                     /**< bit:     19  IO Line Index                            */
    uint32_t P20:1;                     /**< bit:     20  IO Line Index                            */
    uint32_t P21:1;                     /**< bit:     21  IO Line Index                            */
    uint32_t P22:1;                     /**< bit:     22  IO Line Index                            */
    uint32_t P23:1;                     /**< bit:     23  IO Line Index                            */
    uint32_t P24:1;                     /**< bit:     24  IO Line Index                            */
    uint32_t P25:1;                     /**< bit:     25  IO Line Index                            */
    uint32_t P26:1;                     /**< bit:     26  IO Line Index                            */
    uint32_t P27:1;                     /**< bit:     27  IO Line Index                            */
    uint32_t P28:1;                     /**< bit:     28  IO Line Index                            */
    uint32_t P29:1;                     /**< bit:     29  IO Line Index                            */
    uint32_t P30:1;                     /**< bit:     30  IO Line Index                            */
    uint32_t P31:1;                     /**< bit:     31  IO Line Index                            */
  } bit;                                /**< Structure used for bit  access */
  struct {
    uint32_t P:32;                      /**< bit:  0..31  IO Line Index                            */
  } vec;                                /**< Structure used for vec  access  */
  uint32_t reg;                         /**< Type used for register access */
} PIO_AIMMR_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define PIO_AIMMR_OFFSET                    (0xB8)                                        /**<  (PIO_AIMMR) Additional Interrupt Modes Mask Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define PIO_AIMMR_P0_Pos                    0                                              /**< (PIO_AIMMR) IO Line Index Position */
#define PIO_AIMMR_P0_Msk                    (0x1U << PIO_AIMMR_P0_Pos)                     /**< (PIO_AIMMR) IO Line Index Mask */
#define PIO_AIMMR_P0                        PIO_AIMMR_P0_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_AIMMR_P0_Msk instead */
#define PIO_AIMMR_P1_Pos                    1                                              /**< (PIO_AIMMR) IO Line Index Position */
#define PIO_AIMMR_P1_Msk                    (0x1U << PIO_AIMMR_P1_Pos)                     /**< (PIO_AIMMR) IO Line Index Mask */
#define PIO_AIMMR_P1                        PIO_AIMMR_P1_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_AIMMR_P1_Msk instead */
#define PIO_AIMMR_P2_Pos                    2                                              /**< (PIO_AIMMR) IO Line Index Position */
#define PIO_AIMMR_P2_Msk                    (0x1U << PIO_AIMMR_P2_Pos)                     /**< (PIO_AIMMR) IO Line Index Mask */
#define PIO_AIMMR_P2                        PIO_AIMMR_P2_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_AIMMR_P2_Msk instead */
#define PIO_AIMMR_P3_Pos                    3                                              /**< (PIO_AIMMR) IO Line Index Position */
#define PIO_AIMMR_P3_Msk                    (0x1U << PIO_AIMMR_P3_Pos)                     /**< (PIO_AIMMR) IO Line Index Mask */
#define PIO_AIMMR_P3                        PIO_AIMMR_P3_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_AIMMR_P3_Msk instead */
#define PIO_AIMMR_P4_Pos                    4                                              /**< (PIO_AIMMR) IO Line Index Position */
#define PIO_AIMMR_P4_Msk                    (0x1U << PIO_AIMMR_P4_Pos)                     /**< (PIO_AIMMR) IO Line Index Mask */
#define PIO_AIMMR_P4                        PIO_AIMMR_P4_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_AIMMR_P4_Msk instead */
#define PIO_AIMMR_P5_Pos                    5                                              /**< (PIO_AIMMR) IO Line Index Position */
#define PIO_AIMMR_P5_Msk                    (0x1U << PIO_AIMMR_P5_Pos)                     /**< (PIO_AIMMR) IO Line Index Mask */
#define PIO_AIMMR_P5                        PIO_AIMMR_P5_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_AIMMR_P5_Msk instead */
#define PIO_AIMMR_P6_Pos                    6                                              /**< (PIO_AIMMR) IO Line Index Position */
#define PIO_AIMMR_P6_Msk                    (0x1U << PIO_AIMMR_P6_Pos)                     /**< (PIO_AIMMR) IO Line Index Mask */
#define PIO_AIMMR_P6                        PIO_AIMMR_P6_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_AIMMR_P6_Msk instead */
#define PIO_AIMMR_P7_Pos                    7                                              /**< (PIO_AIMMR) IO Line Index Position */
#define PIO_AIMMR_P7_Msk                    (0x1U << PIO_AIMMR_P7_Pos)                     /**< (PIO_AIMMR) IO Line Index Mask */
#define PIO_AIMMR_P7                        PIO_AIMMR_P7_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_AIMMR_P7_Msk instead */
#define PIO_AIMMR_P8_Pos                    8                                              /**< (PIO_AIMMR) IO Line Index Position */
#define PIO_AIMMR_P8_Msk                    (0x1U << PIO_AIMMR_P8_Pos)                     /**< (PIO_AIMMR) IO Line Index Mask */
#define PIO_AIMMR_P8                        PIO_AIMMR_P8_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_AIMMR_P8_Msk instead */
#define PIO_AIMMR_P9_Pos                    9                                              /**< (PIO_AIMMR) IO Line Index Position */
#define PIO_AIMMR_P9_Msk                    (0x1U << PIO_AIMMR_P9_Pos)                     /**< (PIO_AIMMR) IO Line Index Mask */
#define PIO_AIMMR_P9                        PIO_AIMMR_P9_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_AIMMR_P9_Msk instead */
#define PIO_AIMMR_P10_Pos                   10                                             /**< (PIO_AIMMR) IO Line Index Position */
#define PIO_AIMMR_P10_Msk                   (0x1U << PIO_AIMMR_P10_Pos)                    /**< (PIO_AIMMR) IO Line Index Mask */
#define PIO_AIMMR_P10                       PIO_AIMMR_P10_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_AIMMR_P10_Msk instead */
#define PIO_AIMMR_P11_Pos                   11                                             /**< (PIO_AIMMR) IO Line Index Position */
#define PIO_AIMMR_P11_Msk                   (0x1U << PIO_AIMMR_P11_Pos)                    /**< (PIO_AIMMR) IO Line Index Mask */
#define PIO_AIMMR_P11                       PIO_AIMMR_P11_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_AIMMR_P11_Msk instead */
#define PIO_AIMMR_P12_Pos                   12                                             /**< (PIO_AIMMR) IO Line Index Position */
#define PIO_AIMMR_P12_Msk                   (0x1U << PIO_AIMMR_P12_Pos)                    /**< (PIO_AIMMR) IO Line Index Mask */
#define PIO_AIMMR_P12                       PIO_AIMMR_P12_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_AIMMR_P12_Msk instead */
#define PIO_AIMMR_P13_Pos                   13                                             /**< (PIO_AIMMR) IO Line Index Position */
#define PIO_AIMMR_P13_Msk                   (0x1U << PIO_AIMMR_P13_Pos)                    /**< (PIO_AIMMR) IO Line Index Mask */
#define PIO_AIMMR_P13                       PIO_AIMMR_P13_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_AIMMR_P13_Msk instead */
#define PIO_AIMMR_P14_Pos                   14                                             /**< (PIO_AIMMR) IO Line Index Position */
#define PIO_AIMMR_P14_Msk                   (0x1U << PIO_AIMMR_P14_Pos)                    /**< (PIO_AIMMR) IO Line Index Mask */
#define PIO_AIMMR_P14                       PIO_AIMMR_P14_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_AIMMR_P14_Msk instead */
#define PIO_AIMMR_P15_Pos                   15                                             /**< (PIO_AIMMR) IO Line Index Position */
#define PIO_AIMMR_P15_Msk                   (0x1U << PIO_AIMMR_P15_Pos)                    /**< (PIO_AIMMR) IO Line Index Mask */
#define PIO_AIMMR_P15                       PIO_AIMMR_P15_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_AIMMR_P15_Msk instead */
#define PIO_AIMMR_P16_Pos                   16                                             /**< (PIO_AIMMR) IO Line Index Position */
#define PIO_AIMMR_P16_Msk                   (0x1U << PIO_AIMMR_P16_Pos)                    /**< (PIO_AIMMR) IO Line Index Mask */
#define PIO_AIMMR_P16                       PIO_AIMMR_P16_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_AIMMR_P16_Msk instead */
#define PIO_AIMMR_P17_Pos                   17                                             /**< (PIO_AIMMR) IO Line Index Position */
#define PIO_AIMMR_P17_Msk                   (0x1U << PIO_AIMMR_P17_Pos)                    /**< (PIO_AIMMR) IO Line Index Mask */
#define PIO_AIMMR_P17                       PIO_AIMMR_P17_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_AIMMR_P17_Msk instead */
#define PIO_AIMMR_P18_Pos                   18                                             /**< (PIO_AIMMR) IO Line Index Position */
#define PIO_AIMMR_P18_Msk                   (0x1U << PIO_AIMMR_P18_Pos)                    /**< (PIO_AIMMR) IO Line Index Mask */
#define PIO_AIMMR_P18                       PIO_AIMMR_P18_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_AIMMR_P18_Msk instead */
#define PIO_AIMMR_P19_Pos                   19                                             /**< (PIO_AIMMR) IO Line Index Position */
#define PIO_AIMMR_P19_Msk                   (0x1U << PIO_AIMMR_P19_Pos)                    /**< (PIO_AIMMR) IO Line Index Mask */
#define PIO_AIMMR_P19                       PIO_AIMMR_P19_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_AIMMR_P19_Msk instead */
#define PIO_AIMMR_P20_Pos                   20                                             /**< (PIO_AIMMR) IO Line Index Position */
#define PIO_AIMMR_P20_Msk                   (0x1U << PIO_AIMMR_P20_Pos)                    /**< (PIO_AIMMR) IO Line Index Mask */
#define PIO_AIMMR_P20                       PIO_AIMMR_P20_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_AIMMR_P20_Msk instead */
#define PIO_AIMMR_P21_Pos                   21                                             /**< (PIO_AIMMR) IO Line Index Position */
#define PIO_AIMMR_P21_Msk                   (0x1U << PIO_AIMMR_P21_Pos)                    /**< (PIO_AIMMR) IO Line Index Mask */
#define PIO_AIMMR_P21                       PIO_AIMMR_P21_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_AIMMR_P21_Msk instead */
#define PIO_AIMMR_P22_Pos                   22                                             /**< (PIO_AIMMR) IO Line Index Position */
#define PIO_AIMMR_P22_Msk                   (0x1U << PIO_AIMMR_P22_Pos)                    /**< (PIO_AIMMR) IO Line Index Mask */
#define PIO_AIMMR_P22                       PIO_AIMMR_P22_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_AIMMR_P22_Msk instead */
#define PIO_AIMMR_P23_Pos                   23                                             /**< (PIO_AIMMR) IO Line Index Position */
#define PIO_AIMMR_P23_Msk                   (0x1U << PIO_AIMMR_P23_Pos)                    /**< (PIO_AIMMR) IO Line Index Mask */
#define PIO_AIMMR_P23                       PIO_AIMMR_P23_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_AIMMR_P23_Msk instead */
#define PIO_AIMMR_P24_Pos                   24                                             /**< (PIO_AIMMR) IO Line Index Position */
#define PIO_AIMMR_P24_Msk                   (0x1U << PIO_AIMMR_P24_Pos)                    /**< (PIO_AIMMR) IO Line Index Mask */
#define PIO_AIMMR_P24                       PIO_AIMMR_P24_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_AIMMR_P24_Msk instead */
#define PIO_AIMMR_P25_Pos                   25                                             /**< (PIO_AIMMR) IO Line Index Position */
#define PIO_AIMMR_P25_Msk                   (0x1U << PIO_AIMMR_P25_Pos)                    /**< (PIO_AIMMR) IO Line Index Mask */
#define PIO_AIMMR_P25                       PIO_AIMMR_P25_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_AIMMR_P25_Msk instead */
#define PIO_AIMMR_P26_Pos                   26                                             /**< (PIO_AIMMR) IO Line Index Position */
#define PIO_AIMMR_P26_Msk                   (0x1U << PIO_AIMMR_P26_Pos)                    /**< (PIO_AIMMR) IO Line Index Mask */
#define PIO_AIMMR_P26                       PIO_AIMMR_P26_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_AIMMR_P26_Msk instead */
#define PIO_AIMMR_P27_Pos                   27                                             /**< (PIO_AIMMR) IO Line Index Position */
#define PIO_AIMMR_P27_Msk                   (0x1U << PIO_AIMMR_P27_Pos)                    /**< (PIO_AIMMR) IO Line Index Mask */
#define PIO_AIMMR_P27                       PIO_AIMMR_P27_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_AIMMR_P27_Msk instead */
#define PIO_AIMMR_P28_Pos                   28                                             /**< (PIO_AIMMR) IO Line Index Position */
#define PIO_AIMMR_P28_Msk                   (0x1U << PIO_AIMMR_P28_Pos)                    /**< (PIO_AIMMR) IO Line Index Mask */
#define PIO_AIMMR_P28                       PIO_AIMMR_P28_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_AIMMR_P28_Msk instead */
#define PIO_AIMMR_P29_Pos                   29                                             /**< (PIO_AIMMR) IO Line Index Position */
#define PIO_AIMMR_P29_Msk                   (0x1U << PIO_AIMMR_P29_Pos)                    /**< (PIO_AIMMR) IO Line Index Mask */
#define PIO_AIMMR_P29                       PIO_AIMMR_P29_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_AIMMR_P29_Msk instead */
#define PIO_AIMMR_P30_Pos                   30                                             /**< (PIO_AIMMR) IO Line Index Position */
#define PIO_AIMMR_P30_Msk                   (0x1U << PIO_AIMMR_P30_Pos)                    /**< (PIO_AIMMR) IO Line Index Mask */
#define PIO_AIMMR_P30                       PIO_AIMMR_P30_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_AIMMR_P30_Msk instead */
#define PIO_AIMMR_P31_Pos                   31                                             /**< (PIO_AIMMR) IO Line Index Position */
#define PIO_AIMMR_P31_Msk                   (0x1U << PIO_AIMMR_P31_Pos)                    /**< (PIO_AIMMR) IO Line Index Mask */
#define PIO_AIMMR_P31                       PIO_AIMMR_P31_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_AIMMR_P31_Msk instead */
#define PIO_AIMMR_P_Pos                     0                                              /**< (PIO_AIMMR Position) IO Line Index */
#define PIO_AIMMR_P_Msk                     (0xFFFFFFFFU << PIO_AIMMR_P_Pos)               /**< (PIO_AIMMR Mask) P */
#define PIO_AIMMR_P(value)                  (PIO_AIMMR_P_Msk & ((value) << PIO_AIMMR_P_Pos))  
#define PIO_AIMMR_MASK                      (0xFFFFFFFFU)                                  /**< \deprecated (PIO_AIMMR) Register MASK  (Use PIO_AIMMR_Msk instead)  */
#define PIO_AIMMR_Msk                       (0xFFFFFFFFU)                                  /**< (PIO_AIMMR) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- PIO_ESR : (PIO Offset: 0xc0) (/W 32) Edge Select Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t P0:1;                      /**< bit:      0  Edge Interrupt Selection                 */
    uint32_t P1:1;                      /**< bit:      1  Edge Interrupt Selection                 */
    uint32_t P2:1;                      /**< bit:      2  Edge Interrupt Selection                 */
    uint32_t P3:1;                      /**< bit:      3  Edge Interrupt Selection                 */
    uint32_t P4:1;                      /**< bit:      4  Edge Interrupt Selection                 */
    uint32_t P5:1;                      /**< bit:      5  Edge Interrupt Selection                 */
    uint32_t P6:1;                      /**< bit:      6  Edge Interrupt Selection                 */
    uint32_t P7:1;                      /**< bit:      7  Edge Interrupt Selection                 */
    uint32_t P8:1;                      /**< bit:      8  Edge Interrupt Selection                 */
    uint32_t P9:1;                      /**< bit:      9  Edge Interrupt Selection                 */
    uint32_t P10:1;                     /**< bit:     10  Edge Interrupt Selection                 */
    uint32_t P11:1;                     /**< bit:     11  Edge Interrupt Selection                 */
    uint32_t P12:1;                     /**< bit:     12  Edge Interrupt Selection                 */
    uint32_t P13:1;                     /**< bit:     13  Edge Interrupt Selection                 */
    uint32_t P14:1;                     /**< bit:     14  Edge Interrupt Selection                 */
    uint32_t P15:1;                     /**< bit:     15  Edge Interrupt Selection                 */
    uint32_t P16:1;                     /**< bit:     16  Edge Interrupt Selection                 */
    uint32_t P17:1;                     /**< bit:     17  Edge Interrupt Selection                 */
    uint32_t P18:1;                     /**< bit:     18  Edge Interrupt Selection                 */
    uint32_t P19:1;                     /**< bit:     19  Edge Interrupt Selection                 */
    uint32_t P20:1;                     /**< bit:     20  Edge Interrupt Selection                 */
    uint32_t P21:1;                     /**< bit:     21  Edge Interrupt Selection                 */
    uint32_t P22:1;                     /**< bit:     22  Edge Interrupt Selection                 */
    uint32_t P23:1;                     /**< bit:     23  Edge Interrupt Selection                 */
    uint32_t P24:1;                     /**< bit:     24  Edge Interrupt Selection                 */
    uint32_t P25:1;                     /**< bit:     25  Edge Interrupt Selection                 */
    uint32_t P26:1;                     /**< bit:     26  Edge Interrupt Selection                 */
    uint32_t P27:1;                     /**< bit:     27  Edge Interrupt Selection                 */
    uint32_t P28:1;                     /**< bit:     28  Edge Interrupt Selection                 */
    uint32_t P29:1;                     /**< bit:     29  Edge Interrupt Selection                 */
    uint32_t P30:1;                     /**< bit:     30  Edge Interrupt Selection                 */
    uint32_t P31:1;                     /**< bit:     31  Edge Interrupt Selection                 */
  } bit;                                /**< Structure used for bit  access */
  struct {
    uint32_t P:32;                      /**< bit:  0..31  Edge Interrupt Selection                 */
  } vec;                                /**< Structure used for vec  access  */
  uint32_t reg;                         /**< Type used for register access */
} PIO_ESR_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define PIO_ESR_OFFSET                      (0xC0)                                        /**<  (PIO_ESR) Edge Select Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define PIO_ESR_P0_Pos                      0                                              /**< (PIO_ESR) Edge Interrupt Selection Position */
#define PIO_ESR_P0_Msk                      (0x1U << PIO_ESR_P0_Pos)                       /**< (PIO_ESR) Edge Interrupt Selection Mask */
#define PIO_ESR_P0                          PIO_ESR_P0_Msk                                 /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_ESR_P0_Msk instead */
#define PIO_ESR_P1_Pos                      1                                              /**< (PIO_ESR) Edge Interrupt Selection Position */
#define PIO_ESR_P1_Msk                      (0x1U << PIO_ESR_P1_Pos)                       /**< (PIO_ESR) Edge Interrupt Selection Mask */
#define PIO_ESR_P1                          PIO_ESR_P1_Msk                                 /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_ESR_P1_Msk instead */
#define PIO_ESR_P2_Pos                      2                                              /**< (PIO_ESR) Edge Interrupt Selection Position */
#define PIO_ESR_P2_Msk                      (0x1U << PIO_ESR_P2_Pos)                       /**< (PIO_ESR) Edge Interrupt Selection Mask */
#define PIO_ESR_P2                          PIO_ESR_P2_Msk                                 /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_ESR_P2_Msk instead */
#define PIO_ESR_P3_Pos                      3                                              /**< (PIO_ESR) Edge Interrupt Selection Position */
#define PIO_ESR_P3_Msk                      (0x1U << PIO_ESR_P3_Pos)                       /**< (PIO_ESR) Edge Interrupt Selection Mask */
#define PIO_ESR_P3                          PIO_ESR_P3_Msk                                 /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_ESR_P3_Msk instead */
#define PIO_ESR_P4_Pos                      4                                              /**< (PIO_ESR) Edge Interrupt Selection Position */
#define PIO_ESR_P4_Msk                      (0x1U << PIO_ESR_P4_Pos)                       /**< (PIO_ESR) Edge Interrupt Selection Mask */
#define PIO_ESR_P4                          PIO_ESR_P4_Msk                                 /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_ESR_P4_Msk instead */
#define PIO_ESR_P5_Pos                      5                                              /**< (PIO_ESR) Edge Interrupt Selection Position */
#define PIO_ESR_P5_Msk                      (0x1U << PIO_ESR_P5_Pos)                       /**< (PIO_ESR) Edge Interrupt Selection Mask */
#define PIO_ESR_P5                          PIO_ESR_P5_Msk                                 /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_ESR_P5_Msk instead */
#define PIO_ESR_P6_Pos                      6                                              /**< (PIO_ESR) Edge Interrupt Selection Position */
#define PIO_ESR_P6_Msk                      (0x1U << PIO_ESR_P6_Pos)                       /**< (PIO_ESR) Edge Interrupt Selection Mask */
#define PIO_ESR_P6                          PIO_ESR_P6_Msk                                 /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_ESR_P6_Msk instead */
#define PIO_ESR_P7_Pos                      7                                              /**< (PIO_ESR) Edge Interrupt Selection Position */
#define PIO_ESR_P7_Msk                      (0x1U << PIO_ESR_P7_Pos)                       /**< (PIO_ESR) Edge Interrupt Selection Mask */
#define PIO_ESR_P7                          PIO_ESR_P7_Msk                                 /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_ESR_P7_Msk instead */
#define PIO_ESR_P8_Pos                      8                                              /**< (PIO_ESR) Edge Interrupt Selection Position */
#define PIO_ESR_P8_Msk                      (0x1U << PIO_ESR_P8_Pos)                       /**< (PIO_ESR) Edge Interrupt Selection Mask */
#define PIO_ESR_P8                          PIO_ESR_P8_Msk                                 /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_ESR_P8_Msk instead */
#define PIO_ESR_P9_Pos                      9                                              /**< (PIO_ESR) Edge Interrupt Selection Position */
#define PIO_ESR_P9_Msk                      (0x1U << PIO_ESR_P9_Pos)                       /**< (PIO_ESR) Edge Interrupt Selection Mask */
#define PIO_ESR_P9                          PIO_ESR_P9_Msk                                 /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_ESR_P9_Msk instead */
#define PIO_ESR_P10_Pos                     10                                             /**< (PIO_ESR) Edge Interrupt Selection Position */
#define PIO_ESR_P10_Msk                     (0x1U << PIO_ESR_P10_Pos)                      /**< (PIO_ESR) Edge Interrupt Selection Mask */
#define PIO_ESR_P10                         PIO_ESR_P10_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_ESR_P10_Msk instead */
#define PIO_ESR_P11_Pos                     11                                             /**< (PIO_ESR) Edge Interrupt Selection Position */
#define PIO_ESR_P11_Msk                     (0x1U << PIO_ESR_P11_Pos)                      /**< (PIO_ESR) Edge Interrupt Selection Mask */
#define PIO_ESR_P11                         PIO_ESR_P11_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_ESR_P11_Msk instead */
#define PIO_ESR_P12_Pos                     12                                             /**< (PIO_ESR) Edge Interrupt Selection Position */
#define PIO_ESR_P12_Msk                     (0x1U << PIO_ESR_P12_Pos)                      /**< (PIO_ESR) Edge Interrupt Selection Mask */
#define PIO_ESR_P12                         PIO_ESR_P12_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_ESR_P12_Msk instead */
#define PIO_ESR_P13_Pos                     13                                             /**< (PIO_ESR) Edge Interrupt Selection Position */
#define PIO_ESR_P13_Msk                     (0x1U << PIO_ESR_P13_Pos)                      /**< (PIO_ESR) Edge Interrupt Selection Mask */
#define PIO_ESR_P13                         PIO_ESR_P13_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_ESR_P13_Msk instead */
#define PIO_ESR_P14_Pos                     14                                             /**< (PIO_ESR) Edge Interrupt Selection Position */
#define PIO_ESR_P14_Msk                     (0x1U << PIO_ESR_P14_Pos)                      /**< (PIO_ESR) Edge Interrupt Selection Mask */
#define PIO_ESR_P14                         PIO_ESR_P14_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_ESR_P14_Msk instead */
#define PIO_ESR_P15_Pos                     15                                             /**< (PIO_ESR) Edge Interrupt Selection Position */
#define PIO_ESR_P15_Msk                     (0x1U << PIO_ESR_P15_Pos)                      /**< (PIO_ESR) Edge Interrupt Selection Mask */
#define PIO_ESR_P15                         PIO_ESR_P15_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_ESR_P15_Msk instead */
#define PIO_ESR_P16_Pos                     16                                             /**< (PIO_ESR) Edge Interrupt Selection Position */
#define PIO_ESR_P16_Msk                     (0x1U << PIO_ESR_P16_Pos)                      /**< (PIO_ESR) Edge Interrupt Selection Mask */
#define PIO_ESR_P16                         PIO_ESR_P16_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_ESR_P16_Msk instead */
#define PIO_ESR_P17_Pos                     17                                             /**< (PIO_ESR) Edge Interrupt Selection Position */
#define PIO_ESR_P17_Msk                     (0x1U << PIO_ESR_P17_Pos)                      /**< (PIO_ESR) Edge Interrupt Selection Mask */
#define PIO_ESR_P17                         PIO_ESR_P17_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_ESR_P17_Msk instead */
#define PIO_ESR_P18_Pos                     18                                             /**< (PIO_ESR) Edge Interrupt Selection Position */
#define PIO_ESR_P18_Msk                     (0x1U << PIO_ESR_P18_Pos)                      /**< (PIO_ESR) Edge Interrupt Selection Mask */
#define PIO_ESR_P18                         PIO_ESR_P18_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_ESR_P18_Msk instead */
#define PIO_ESR_P19_Pos                     19                                             /**< (PIO_ESR) Edge Interrupt Selection Position */
#define PIO_ESR_P19_Msk                     (0x1U << PIO_ESR_P19_Pos)                      /**< (PIO_ESR) Edge Interrupt Selection Mask */
#define PIO_ESR_P19                         PIO_ESR_P19_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_ESR_P19_Msk instead */
#define PIO_ESR_P20_Pos                     20                                             /**< (PIO_ESR) Edge Interrupt Selection Position */
#define PIO_ESR_P20_Msk                     (0x1U << PIO_ESR_P20_Pos)                      /**< (PIO_ESR) Edge Interrupt Selection Mask */
#define PIO_ESR_P20                         PIO_ESR_P20_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_ESR_P20_Msk instead */
#define PIO_ESR_P21_Pos                     21                                             /**< (PIO_ESR) Edge Interrupt Selection Position */
#define PIO_ESR_P21_Msk                     (0x1U << PIO_ESR_P21_Pos)                      /**< (PIO_ESR) Edge Interrupt Selection Mask */
#define PIO_ESR_P21                         PIO_ESR_P21_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_ESR_P21_Msk instead */
#define PIO_ESR_P22_Pos                     22                                             /**< (PIO_ESR) Edge Interrupt Selection Position */
#define PIO_ESR_P22_Msk                     (0x1U << PIO_ESR_P22_Pos)                      /**< (PIO_ESR) Edge Interrupt Selection Mask */
#define PIO_ESR_P22                         PIO_ESR_P22_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_ESR_P22_Msk instead */
#define PIO_ESR_P23_Pos                     23                                             /**< (PIO_ESR) Edge Interrupt Selection Position */
#define PIO_ESR_P23_Msk                     (0x1U << PIO_ESR_P23_Pos)                      /**< (PIO_ESR) Edge Interrupt Selection Mask */
#define PIO_ESR_P23                         PIO_ESR_P23_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_ESR_P23_Msk instead */
#define PIO_ESR_P24_Pos                     24                                             /**< (PIO_ESR) Edge Interrupt Selection Position */
#define PIO_ESR_P24_Msk                     (0x1U << PIO_ESR_P24_Pos)                      /**< (PIO_ESR) Edge Interrupt Selection Mask */
#define PIO_ESR_P24                         PIO_ESR_P24_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_ESR_P24_Msk instead */
#define PIO_ESR_P25_Pos                     25                                             /**< (PIO_ESR) Edge Interrupt Selection Position */
#define PIO_ESR_P25_Msk                     (0x1U << PIO_ESR_P25_Pos)                      /**< (PIO_ESR) Edge Interrupt Selection Mask */
#define PIO_ESR_P25                         PIO_ESR_P25_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_ESR_P25_Msk instead */
#define PIO_ESR_P26_Pos                     26                                             /**< (PIO_ESR) Edge Interrupt Selection Position */
#define PIO_ESR_P26_Msk                     (0x1U << PIO_ESR_P26_Pos)                      /**< (PIO_ESR) Edge Interrupt Selection Mask */
#define PIO_ESR_P26                         PIO_ESR_P26_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_ESR_P26_Msk instead */
#define PIO_ESR_P27_Pos                     27                                             /**< (PIO_ESR) Edge Interrupt Selection Position */
#define PIO_ESR_P27_Msk                     (0x1U << PIO_ESR_P27_Pos)                      /**< (PIO_ESR) Edge Interrupt Selection Mask */
#define PIO_ESR_P27                         PIO_ESR_P27_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_ESR_P27_Msk instead */
#define PIO_ESR_P28_Pos                     28                                             /**< (PIO_ESR) Edge Interrupt Selection Position */
#define PIO_ESR_P28_Msk                     (0x1U << PIO_ESR_P28_Pos)                      /**< (PIO_ESR) Edge Interrupt Selection Mask */
#define PIO_ESR_P28                         PIO_ESR_P28_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_ESR_P28_Msk instead */
#define PIO_ESR_P29_Pos                     29                                             /**< (PIO_ESR) Edge Interrupt Selection Position */
#define PIO_ESR_P29_Msk                     (0x1U << PIO_ESR_P29_Pos)                      /**< (PIO_ESR) Edge Interrupt Selection Mask */
#define PIO_ESR_P29                         PIO_ESR_P29_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_ESR_P29_Msk instead */
#define PIO_ESR_P30_Pos                     30                                             /**< (PIO_ESR) Edge Interrupt Selection Position */
#define PIO_ESR_P30_Msk                     (0x1U << PIO_ESR_P30_Pos)                      /**< (PIO_ESR) Edge Interrupt Selection Mask */
#define PIO_ESR_P30                         PIO_ESR_P30_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_ESR_P30_Msk instead */
#define PIO_ESR_P31_Pos                     31                                             /**< (PIO_ESR) Edge Interrupt Selection Position */
#define PIO_ESR_P31_Msk                     (0x1U << PIO_ESR_P31_Pos)                      /**< (PIO_ESR) Edge Interrupt Selection Mask */
#define PIO_ESR_P31                         PIO_ESR_P31_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_ESR_P31_Msk instead */
#define PIO_ESR_P_Pos                       0                                              /**< (PIO_ESR Position) Edge Interrupt Selection */
#define PIO_ESR_P_Msk                       (0xFFFFFFFFU << PIO_ESR_P_Pos)                 /**< (PIO_ESR Mask) P */
#define PIO_ESR_P(value)                    (PIO_ESR_P_Msk & ((value) << PIO_ESR_P_Pos))   
#define PIO_ESR_MASK                        (0xFFFFFFFFU)                                  /**< \deprecated (PIO_ESR) Register MASK  (Use PIO_ESR_Msk instead)  */
#define PIO_ESR_Msk                         (0xFFFFFFFFU)                                  /**< (PIO_ESR) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- PIO_LSR : (PIO Offset: 0xc4) (/W 32) Level Select Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t P0:1;                      /**< bit:      0  Level Interrupt Selection                */
    uint32_t P1:1;                      /**< bit:      1  Level Interrupt Selection                */
    uint32_t P2:1;                      /**< bit:      2  Level Interrupt Selection                */
    uint32_t P3:1;                      /**< bit:      3  Level Interrupt Selection                */
    uint32_t P4:1;                      /**< bit:      4  Level Interrupt Selection                */
    uint32_t P5:1;                      /**< bit:      5  Level Interrupt Selection                */
    uint32_t P6:1;                      /**< bit:      6  Level Interrupt Selection                */
    uint32_t P7:1;                      /**< bit:      7  Level Interrupt Selection                */
    uint32_t P8:1;                      /**< bit:      8  Level Interrupt Selection                */
    uint32_t P9:1;                      /**< bit:      9  Level Interrupt Selection                */
    uint32_t P10:1;                     /**< bit:     10  Level Interrupt Selection                */
    uint32_t P11:1;                     /**< bit:     11  Level Interrupt Selection                */
    uint32_t P12:1;                     /**< bit:     12  Level Interrupt Selection                */
    uint32_t P13:1;                     /**< bit:     13  Level Interrupt Selection                */
    uint32_t P14:1;                     /**< bit:     14  Level Interrupt Selection                */
    uint32_t P15:1;                     /**< bit:     15  Level Interrupt Selection                */
    uint32_t P16:1;                     /**< bit:     16  Level Interrupt Selection                */
    uint32_t P17:1;                     /**< bit:     17  Level Interrupt Selection                */
    uint32_t P18:1;                     /**< bit:     18  Level Interrupt Selection                */
    uint32_t P19:1;                     /**< bit:     19  Level Interrupt Selection                */
    uint32_t P20:1;                     /**< bit:     20  Level Interrupt Selection                */
    uint32_t P21:1;                     /**< bit:     21  Level Interrupt Selection                */
    uint32_t P22:1;                     /**< bit:     22  Level Interrupt Selection                */
    uint32_t P23:1;                     /**< bit:     23  Level Interrupt Selection                */
    uint32_t P24:1;                     /**< bit:     24  Level Interrupt Selection                */
    uint32_t P25:1;                     /**< bit:     25  Level Interrupt Selection                */
    uint32_t P26:1;                     /**< bit:     26  Level Interrupt Selection                */
    uint32_t P27:1;                     /**< bit:     27  Level Interrupt Selection                */
    uint32_t P28:1;                     /**< bit:     28  Level Interrupt Selection                */
    uint32_t P29:1;                     /**< bit:     29  Level Interrupt Selection                */
    uint32_t P30:1;                     /**< bit:     30  Level Interrupt Selection                */
    uint32_t P31:1;                     /**< bit:     31  Level Interrupt Selection                */
  } bit;                                /**< Structure used for bit  access */
  struct {
    uint32_t P:32;                      /**< bit:  0..31  Level Interrupt Selection                */
  } vec;                                /**< Structure used for vec  access  */
  uint32_t reg;                         /**< Type used for register access */
} PIO_LSR_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define PIO_LSR_OFFSET                      (0xC4)                                        /**<  (PIO_LSR) Level Select Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define PIO_LSR_P0_Pos                      0                                              /**< (PIO_LSR) Level Interrupt Selection Position */
#define PIO_LSR_P0_Msk                      (0x1U << PIO_LSR_P0_Pos)                       /**< (PIO_LSR) Level Interrupt Selection Mask */
#define PIO_LSR_P0                          PIO_LSR_P0_Msk                                 /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_LSR_P0_Msk instead */
#define PIO_LSR_P1_Pos                      1                                              /**< (PIO_LSR) Level Interrupt Selection Position */
#define PIO_LSR_P1_Msk                      (0x1U << PIO_LSR_P1_Pos)                       /**< (PIO_LSR) Level Interrupt Selection Mask */
#define PIO_LSR_P1                          PIO_LSR_P1_Msk                                 /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_LSR_P1_Msk instead */
#define PIO_LSR_P2_Pos                      2                                              /**< (PIO_LSR) Level Interrupt Selection Position */
#define PIO_LSR_P2_Msk                      (0x1U << PIO_LSR_P2_Pos)                       /**< (PIO_LSR) Level Interrupt Selection Mask */
#define PIO_LSR_P2                          PIO_LSR_P2_Msk                                 /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_LSR_P2_Msk instead */
#define PIO_LSR_P3_Pos                      3                                              /**< (PIO_LSR) Level Interrupt Selection Position */
#define PIO_LSR_P3_Msk                      (0x1U << PIO_LSR_P3_Pos)                       /**< (PIO_LSR) Level Interrupt Selection Mask */
#define PIO_LSR_P3                          PIO_LSR_P3_Msk                                 /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_LSR_P3_Msk instead */
#define PIO_LSR_P4_Pos                      4                                              /**< (PIO_LSR) Level Interrupt Selection Position */
#define PIO_LSR_P4_Msk                      (0x1U << PIO_LSR_P4_Pos)                       /**< (PIO_LSR) Level Interrupt Selection Mask */
#define PIO_LSR_P4                          PIO_LSR_P4_Msk                                 /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_LSR_P4_Msk instead */
#define PIO_LSR_P5_Pos                      5                                              /**< (PIO_LSR) Level Interrupt Selection Position */
#define PIO_LSR_P5_Msk                      (0x1U << PIO_LSR_P5_Pos)                       /**< (PIO_LSR) Level Interrupt Selection Mask */
#define PIO_LSR_P5                          PIO_LSR_P5_Msk                                 /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_LSR_P5_Msk instead */
#define PIO_LSR_P6_Pos                      6                                              /**< (PIO_LSR) Level Interrupt Selection Position */
#define PIO_LSR_P6_Msk                      (0x1U << PIO_LSR_P6_Pos)                       /**< (PIO_LSR) Level Interrupt Selection Mask */
#define PIO_LSR_P6                          PIO_LSR_P6_Msk                                 /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_LSR_P6_Msk instead */
#define PIO_LSR_P7_Pos                      7                                              /**< (PIO_LSR) Level Interrupt Selection Position */
#define PIO_LSR_P7_Msk                      (0x1U << PIO_LSR_P7_Pos)                       /**< (PIO_LSR) Level Interrupt Selection Mask */
#define PIO_LSR_P7                          PIO_LSR_P7_Msk                                 /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_LSR_P7_Msk instead */
#define PIO_LSR_P8_Pos                      8                                              /**< (PIO_LSR) Level Interrupt Selection Position */
#define PIO_LSR_P8_Msk                      (0x1U << PIO_LSR_P8_Pos)                       /**< (PIO_LSR) Level Interrupt Selection Mask */
#define PIO_LSR_P8                          PIO_LSR_P8_Msk                                 /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_LSR_P8_Msk instead */
#define PIO_LSR_P9_Pos                      9                                              /**< (PIO_LSR) Level Interrupt Selection Position */
#define PIO_LSR_P9_Msk                      (0x1U << PIO_LSR_P9_Pos)                       /**< (PIO_LSR) Level Interrupt Selection Mask */
#define PIO_LSR_P9                          PIO_LSR_P9_Msk                                 /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_LSR_P9_Msk instead */
#define PIO_LSR_P10_Pos                     10                                             /**< (PIO_LSR) Level Interrupt Selection Position */
#define PIO_LSR_P10_Msk                     (0x1U << PIO_LSR_P10_Pos)                      /**< (PIO_LSR) Level Interrupt Selection Mask */
#define PIO_LSR_P10                         PIO_LSR_P10_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_LSR_P10_Msk instead */
#define PIO_LSR_P11_Pos                     11                                             /**< (PIO_LSR) Level Interrupt Selection Position */
#define PIO_LSR_P11_Msk                     (0x1U << PIO_LSR_P11_Pos)                      /**< (PIO_LSR) Level Interrupt Selection Mask */
#define PIO_LSR_P11                         PIO_LSR_P11_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_LSR_P11_Msk instead */
#define PIO_LSR_P12_Pos                     12                                             /**< (PIO_LSR) Level Interrupt Selection Position */
#define PIO_LSR_P12_Msk                     (0x1U << PIO_LSR_P12_Pos)                      /**< (PIO_LSR) Level Interrupt Selection Mask */
#define PIO_LSR_P12                         PIO_LSR_P12_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_LSR_P12_Msk instead */
#define PIO_LSR_P13_Pos                     13                                             /**< (PIO_LSR) Level Interrupt Selection Position */
#define PIO_LSR_P13_Msk                     (0x1U << PIO_LSR_P13_Pos)                      /**< (PIO_LSR) Level Interrupt Selection Mask */
#define PIO_LSR_P13                         PIO_LSR_P13_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_LSR_P13_Msk instead */
#define PIO_LSR_P14_Pos                     14                                             /**< (PIO_LSR) Level Interrupt Selection Position */
#define PIO_LSR_P14_Msk                     (0x1U << PIO_LSR_P14_Pos)                      /**< (PIO_LSR) Level Interrupt Selection Mask */
#define PIO_LSR_P14                         PIO_LSR_P14_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_LSR_P14_Msk instead */
#define PIO_LSR_P15_Pos                     15                                             /**< (PIO_LSR) Level Interrupt Selection Position */
#define PIO_LSR_P15_Msk                     (0x1U << PIO_LSR_P15_Pos)                      /**< (PIO_LSR) Level Interrupt Selection Mask */
#define PIO_LSR_P15                         PIO_LSR_P15_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_LSR_P15_Msk instead */
#define PIO_LSR_P16_Pos                     16                                             /**< (PIO_LSR) Level Interrupt Selection Position */
#define PIO_LSR_P16_Msk                     (0x1U << PIO_LSR_P16_Pos)                      /**< (PIO_LSR) Level Interrupt Selection Mask */
#define PIO_LSR_P16                         PIO_LSR_P16_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_LSR_P16_Msk instead */
#define PIO_LSR_P17_Pos                     17                                             /**< (PIO_LSR) Level Interrupt Selection Position */
#define PIO_LSR_P17_Msk                     (0x1U << PIO_LSR_P17_Pos)                      /**< (PIO_LSR) Level Interrupt Selection Mask */
#define PIO_LSR_P17                         PIO_LSR_P17_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_LSR_P17_Msk instead */
#define PIO_LSR_P18_Pos                     18                                             /**< (PIO_LSR) Level Interrupt Selection Position */
#define PIO_LSR_P18_Msk                     (0x1U << PIO_LSR_P18_Pos)                      /**< (PIO_LSR) Level Interrupt Selection Mask */
#define PIO_LSR_P18                         PIO_LSR_P18_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_LSR_P18_Msk instead */
#define PIO_LSR_P19_Pos                     19                                             /**< (PIO_LSR) Level Interrupt Selection Position */
#define PIO_LSR_P19_Msk                     (0x1U << PIO_LSR_P19_Pos)                      /**< (PIO_LSR) Level Interrupt Selection Mask */
#define PIO_LSR_P19                         PIO_LSR_P19_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_LSR_P19_Msk instead */
#define PIO_LSR_P20_Pos                     20                                             /**< (PIO_LSR) Level Interrupt Selection Position */
#define PIO_LSR_P20_Msk                     (0x1U << PIO_LSR_P20_Pos)                      /**< (PIO_LSR) Level Interrupt Selection Mask */
#define PIO_LSR_P20                         PIO_LSR_P20_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_LSR_P20_Msk instead */
#define PIO_LSR_P21_Pos                     21                                             /**< (PIO_LSR) Level Interrupt Selection Position */
#define PIO_LSR_P21_Msk                     (0x1U << PIO_LSR_P21_Pos)                      /**< (PIO_LSR) Level Interrupt Selection Mask */
#define PIO_LSR_P21                         PIO_LSR_P21_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_LSR_P21_Msk instead */
#define PIO_LSR_P22_Pos                     22                                             /**< (PIO_LSR) Level Interrupt Selection Position */
#define PIO_LSR_P22_Msk                     (0x1U << PIO_LSR_P22_Pos)                      /**< (PIO_LSR) Level Interrupt Selection Mask */
#define PIO_LSR_P22                         PIO_LSR_P22_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_LSR_P22_Msk instead */
#define PIO_LSR_P23_Pos                     23                                             /**< (PIO_LSR) Level Interrupt Selection Position */
#define PIO_LSR_P23_Msk                     (0x1U << PIO_LSR_P23_Pos)                      /**< (PIO_LSR) Level Interrupt Selection Mask */
#define PIO_LSR_P23                         PIO_LSR_P23_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_LSR_P23_Msk instead */
#define PIO_LSR_P24_Pos                     24                                             /**< (PIO_LSR) Level Interrupt Selection Position */
#define PIO_LSR_P24_Msk                     (0x1U << PIO_LSR_P24_Pos)                      /**< (PIO_LSR) Level Interrupt Selection Mask */
#define PIO_LSR_P24                         PIO_LSR_P24_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_LSR_P24_Msk instead */
#define PIO_LSR_P25_Pos                     25                                             /**< (PIO_LSR) Level Interrupt Selection Position */
#define PIO_LSR_P25_Msk                     (0x1U << PIO_LSR_P25_Pos)                      /**< (PIO_LSR) Level Interrupt Selection Mask */
#define PIO_LSR_P25                         PIO_LSR_P25_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_LSR_P25_Msk instead */
#define PIO_LSR_P26_Pos                     26                                             /**< (PIO_LSR) Level Interrupt Selection Position */
#define PIO_LSR_P26_Msk                     (0x1U << PIO_LSR_P26_Pos)                      /**< (PIO_LSR) Level Interrupt Selection Mask */
#define PIO_LSR_P26                         PIO_LSR_P26_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_LSR_P26_Msk instead */
#define PIO_LSR_P27_Pos                     27                                             /**< (PIO_LSR) Level Interrupt Selection Position */
#define PIO_LSR_P27_Msk                     (0x1U << PIO_LSR_P27_Pos)                      /**< (PIO_LSR) Level Interrupt Selection Mask */
#define PIO_LSR_P27                         PIO_LSR_P27_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_LSR_P27_Msk instead */
#define PIO_LSR_P28_Pos                     28                                             /**< (PIO_LSR) Level Interrupt Selection Position */
#define PIO_LSR_P28_Msk                     (0x1U << PIO_LSR_P28_Pos)                      /**< (PIO_LSR) Level Interrupt Selection Mask */
#define PIO_LSR_P28                         PIO_LSR_P28_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_LSR_P28_Msk instead */
#define PIO_LSR_P29_Pos                     29                                             /**< (PIO_LSR) Level Interrupt Selection Position */
#define PIO_LSR_P29_Msk                     (0x1U << PIO_LSR_P29_Pos)                      /**< (PIO_LSR) Level Interrupt Selection Mask */
#define PIO_LSR_P29                         PIO_LSR_P29_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_LSR_P29_Msk instead */
#define PIO_LSR_P30_Pos                     30                                             /**< (PIO_LSR) Level Interrupt Selection Position */
#define PIO_LSR_P30_Msk                     (0x1U << PIO_LSR_P30_Pos)                      /**< (PIO_LSR) Level Interrupt Selection Mask */
#define PIO_LSR_P30                         PIO_LSR_P30_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_LSR_P30_Msk instead */
#define PIO_LSR_P31_Pos                     31                                             /**< (PIO_LSR) Level Interrupt Selection Position */
#define PIO_LSR_P31_Msk                     (0x1U << PIO_LSR_P31_Pos)                      /**< (PIO_LSR) Level Interrupt Selection Mask */
#define PIO_LSR_P31                         PIO_LSR_P31_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_LSR_P31_Msk instead */
#define PIO_LSR_P_Pos                       0                                              /**< (PIO_LSR Position) Level Interrupt Selection */
#define PIO_LSR_P_Msk                       (0xFFFFFFFFU << PIO_LSR_P_Pos)                 /**< (PIO_LSR Mask) P */
#define PIO_LSR_P(value)                    (PIO_LSR_P_Msk & ((value) << PIO_LSR_P_Pos))   
#define PIO_LSR_MASK                        (0xFFFFFFFFU)                                  /**< \deprecated (PIO_LSR) Register MASK  (Use PIO_LSR_Msk instead)  */
#define PIO_LSR_Msk                         (0xFFFFFFFFU)                                  /**< (PIO_LSR) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- PIO_ELSR : (PIO Offset: 0xc8) (R/ 32) Edge/Level Status Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t P0:1;                      /**< bit:      0  Edge/Level Interrupt Source Selection    */
    uint32_t P1:1;                      /**< bit:      1  Edge/Level Interrupt Source Selection    */
    uint32_t P2:1;                      /**< bit:      2  Edge/Level Interrupt Source Selection    */
    uint32_t P3:1;                      /**< bit:      3  Edge/Level Interrupt Source Selection    */
    uint32_t P4:1;                      /**< bit:      4  Edge/Level Interrupt Source Selection    */
    uint32_t P5:1;                      /**< bit:      5  Edge/Level Interrupt Source Selection    */
    uint32_t P6:1;                      /**< bit:      6  Edge/Level Interrupt Source Selection    */
    uint32_t P7:1;                      /**< bit:      7  Edge/Level Interrupt Source Selection    */
    uint32_t P8:1;                      /**< bit:      8  Edge/Level Interrupt Source Selection    */
    uint32_t P9:1;                      /**< bit:      9  Edge/Level Interrupt Source Selection    */
    uint32_t P10:1;                     /**< bit:     10  Edge/Level Interrupt Source Selection    */
    uint32_t P11:1;                     /**< bit:     11  Edge/Level Interrupt Source Selection    */
    uint32_t P12:1;                     /**< bit:     12  Edge/Level Interrupt Source Selection    */
    uint32_t P13:1;                     /**< bit:     13  Edge/Level Interrupt Source Selection    */
    uint32_t P14:1;                     /**< bit:     14  Edge/Level Interrupt Source Selection    */
    uint32_t P15:1;                     /**< bit:     15  Edge/Level Interrupt Source Selection    */
    uint32_t P16:1;                     /**< bit:     16  Edge/Level Interrupt Source Selection    */
    uint32_t P17:1;                     /**< bit:     17  Edge/Level Interrupt Source Selection    */
    uint32_t P18:1;                     /**< bit:     18  Edge/Level Interrupt Source Selection    */
    uint32_t P19:1;                     /**< bit:     19  Edge/Level Interrupt Source Selection    */
    uint32_t P20:1;                     /**< bit:     20  Edge/Level Interrupt Source Selection    */
    uint32_t P21:1;                     /**< bit:     21  Edge/Level Interrupt Source Selection    */
    uint32_t P22:1;                     /**< bit:     22  Edge/Level Interrupt Source Selection    */
    uint32_t P23:1;                     /**< bit:     23  Edge/Level Interrupt Source Selection    */
    uint32_t P24:1;                     /**< bit:     24  Edge/Level Interrupt Source Selection    */
    uint32_t P25:1;                     /**< bit:     25  Edge/Level Interrupt Source Selection    */
    uint32_t P26:1;                     /**< bit:     26  Edge/Level Interrupt Source Selection    */
    uint32_t P27:1;                     /**< bit:     27  Edge/Level Interrupt Source Selection    */
    uint32_t P28:1;                     /**< bit:     28  Edge/Level Interrupt Source Selection    */
    uint32_t P29:1;                     /**< bit:     29  Edge/Level Interrupt Source Selection    */
    uint32_t P30:1;                     /**< bit:     30  Edge/Level Interrupt Source Selection    */
    uint32_t P31:1;                     /**< bit:     31  Edge/Level Interrupt Source Selection    */
  } bit;                                /**< Structure used for bit  access */
  struct {
    uint32_t P:32;                      /**< bit:  0..31  Edge/Level Interrupt Source Selection    */
  } vec;                                /**< Structure used for vec  access  */
  uint32_t reg;                         /**< Type used for register access */
} PIO_ELSR_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define PIO_ELSR_OFFSET                     (0xC8)                                        /**<  (PIO_ELSR) Edge/Level Status Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define PIO_ELSR_P0_Pos                     0                                              /**< (PIO_ELSR) Edge/Level Interrupt Source Selection Position */
#define PIO_ELSR_P0_Msk                     (0x1U << PIO_ELSR_P0_Pos)                      /**< (PIO_ELSR) Edge/Level Interrupt Source Selection Mask */
#define PIO_ELSR_P0                         PIO_ELSR_P0_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_ELSR_P0_Msk instead */
#define PIO_ELSR_P1_Pos                     1                                              /**< (PIO_ELSR) Edge/Level Interrupt Source Selection Position */
#define PIO_ELSR_P1_Msk                     (0x1U << PIO_ELSR_P1_Pos)                      /**< (PIO_ELSR) Edge/Level Interrupt Source Selection Mask */
#define PIO_ELSR_P1                         PIO_ELSR_P1_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_ELSR_P1_Msk instead */
#define PIO_ELSR_P2_Pos                     2                                              /**< (PIO_ELSR) Edge/Level Interrupt Source Selection Position */
#define PIO_ELSR_P2_Msk                     (0x1U << PIO_ELSR_P2_Pos)                      /**< (PIO_ELSR) Edge/Level Interrupt Source Selection Mask */
#define PIO_ELSR_P2                         PIO_ELSR_P2_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_ELSR_P2_Msk instead */
#define PIO_ELSR_P3_Pos                     3                                              /**< (PIO_ELSR) Edge/Level Interrupt Source Selection Position */
#define PIO_ELSR_P3_Msk                     (0x1U << PIO_ELSR_P3_Pos)                      /**< (PIO_ELSR) Edge/Level Interrupt Source Selection Mask */
#define PIO_ELSR_P3                         PIO_ELSR_P3_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_ELSR_P3_Msk instead */
#define PIO_ELSR_P4_Pos                     4                                              /**< (PIO_ELSR) Edge/Level Interrupt Source Selection Position */
#define PIO_ELSR_P4_Msk                     (0x1U << PIO_ELSR_P4_Pos)                      /**< (PIO_ELSR) Edge/Level Interrupt Source Selection Mask */
#define PIO_ELSR_P4                         PIO_ELSR_P4_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_ELSR_P4_Msk instead */
#define PIO_ELSR_P5_Pos                     5                                              /**< (PIO_ELSR) Edge/Level Interrupt Source Selection Position */
#define PIO_ELSR_P5_Msk                     (0x1U << PIO_ELSR_P5_Pos)                      /**< (PIO_ELSR) Edge/Level Interrupt Source Selection Mask */
#define PIO_ELSR_P5                         PIO_ELSR_P5_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_ELSR_P5_Msk instead */
#define PIO_ELSR_P6_Pos                     6                                              /**< (PIO_ELSR) Edge/Level Interrupt Source Selection Position */
#define PIO_ELSR_P6_Msk                     (0x1U << PIO_ELSR_P6_Pos)                      /**< (PIO_ELSR) Edge/Level Interrupt Source Selection Mask */
#define PIO_ELSR_P6                         PIO_ELSR_P6_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_ELSR_P6_Msk instead */
#define PIO_ELSR_P7_Pos                     7                                              /**< (PIO_ELSR) Edge/Level Interrupt Source Selection Position */
#define PIO_ELSR_P7_Msk                     (0x1U << PIO_ELSR_P7_Pos)                      /**< (PIO_ELSR) Edge/Level Interrupt Source Selection Mask */
#define PIO_ELSR_P7                         PIO_ELSR_P7_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_ELSR_P7_Msk instead */
#define PIO_ELSR_P8_Pos                     8                                              /**< (PIO_ELSR) Edge/Level Interrupt Source Selection Position */
#define PIO_ELSR_P8_Msk                     (0x1U << PIO_ELSR_P8_Pos)                      /**< (PIO_ELSR) Edge/Level Interrupt Source Selection Mask */
#define PIO_ELSR_P8                         PIO_ELSR_P8_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_ELSR_P8_Msk instead */
#define PIO_ELSR_P9_Pos                     9                                              /**< (PIO_ELSR) Edge/Level Interrupt Source Selection Position */
#define PIO_ELSR_P9_Msk                     (0x1U << PIO_ELSR_P9_Pos)                      /**< (PIO_ELSR) Edge/Level Interrupt Source Selection Mask */
#define PIO_ELSR_P9                         PIO_ELSR_P9_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_ELSR_P9_Msk instead */
#define PIO_ELSR_P10_Pos                    10                                             /**< (PIO_ELSR) Edge/Level Interrupt Source Selection Position */
#define PIO_ELSR_P10_Msk                    (0x1U << PIO_ELSR_P10_Pos)                     /**< (PIO_ELSR) Edge/Level Interrupt Source Selection Mask */
#define PIO_ELSR_P10                        PIO_ELSR_P10_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_ELSR_P10_Msk instead */
#define PIO_ELSR_P11_Pos                    11                                             /**< (PIO_ELSR) Edge/Level Interrupt Source Selection Position */
#define PIO_ELSR_P11_Msk                    (0x1U << PIO_ELSR_P11_Pos)                     /**< (PIO_ELSR) Edge/Level Interrupt Source Selection Mask */
#define PIO_ELSR_P11                        PIO_ELSR_P11_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_ELSR_P11_Msk instead */
#define PIO_ELSR_P12_Pos                    12                                             /**< (PIO_ELSR) Edge/Level Interrupt Source Selection Position */
#define PIO_ELSR_P12_Msk                    (0x1U << PIO_ELSR_P12_Pos)                     /**< (PIO_ELSR) Edge/Level Interrupt Source Selection Mask */
#define PIO_ELSR_P12                        PIO_ELSR_P12_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_ELSR_P12_Msk instead */
#define PIO_ELSR_P13_Pos                    13                                             /**< (PIO_ELSR) Edge/Level Interrupt Source Selection Position */
#define PIO_ELSR_P13_Msk                    (0x1U << PIO_ELSR_P13_Pos)                     /**< (PIO_ELSR) Edge/Level Interrupt Source Selection Mask */
#define PIO_ELSR_P13                        PIO_ELSR_P13_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_ELSR_P13_Msk instead */
#define PIO_ELSR_P14_Pos                    14                                             /**< (PIO_ELSR) Edge/Level Interrupt Source Selection Position */
#define PIO_ELSR_P14_Msk                    (0x1U << PIO_ELSR_P14_Pos)                     /**< (PIO_ELSR) Edge/Level Interrupt Source Selection Mask */
#define PIO_ELSR_P14                        PIO_ELSR_P14_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_ELSR_P14_Msk instead */
#define PIO_ELSR_P15_Pos                    15                                             /**< (PIO_ELSR) Edge/Level Interrupt Source Selection Position */
#define PIO_ELSR_P15_Msk                    (0x1U << PIO_ELSR_P15_Pos)                     /**< (PIO_ELSR) Edge/Level Interrupt Source Selection Mask */
#define PIO_ELSR_P15                        PIO_ELSR_P15_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_ELSR_P15_Msk instead */
#define PIO_ELSR_P16_Pos                    16                                             /**< (PIO_ELSR) Edge/Level Interrupt Source Selection Position */
#define PIO_ELSR_P16_Msk                    (0x1U << PIO_ELSR_P16_Pos)                     /**< (PIO_ELSR) Edge/Level Interrupt Source Selection Mask */
#define PIO_ELSR_P16                        PIO_ELSR_P16_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_ELSR_P16_Msk instead */
#define PIO_ELSR_P17_Pos                    17                                             /**< (PIO_ELSR) Edge/Level Interrupt Source Selection Position */
#define PIO_ELSR_P17_Msk                    (0x1U << PIO_ELSR_P17_Pos)                     /**< (PIO_ELSR) Edge/Level Interrupt Source Selection Mask */
#define PIO_ELSR_P17                        PIO_ELSR_P17_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_ELSR_P17_Msk instead */
#define PIO_ELSR_P18_Pos                    18                                             /**< (PIO_ELSR) Edge/Level Interrupt Source Selection Position */
#define PIO_ELSR_P18_Msk                    (0x1U << PIO_ELSR_P18_Pos)                     /**< (PIO_ELSR) Edge/Level Interrupt Source Selection Mask */
#define PIO_ELSR_P18                        PIO_ELSR_P18_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_ELSR_P18_Msk instead */
#define PIO_ELSR_P19_Pos                    19                                             /**< (PIO_ELSR) Edge/Level Interrupt Source Selection Position */
#define PIO_ELSR_P19_Msk                    (0x1U << PIO_ELSR_P19_Pos)                     /**< (PIO_ELSR) Edge/Level Interrupt Source Selection Mask */
#define PIO_ELSR_P19                        PIO_ELSR_P19_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_ELSR_P19_Msk instead */
#define PIO_ELSR_P20_Pos                    20                                             /**< (PIO_ELSR) Edge/Level Interrupt Source Selection Position */
#define PIO_ELSR_P20_Msk                    (0x1U << PIO_ELSR_P20_Pos)                     /**< (PIO_ELSR) Edge/Level Interrupt Source Selection Mask */
#define PIO_ELSR_P20                        PIO_ELSR_P20_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_ELSR_P20_Msk instead */
#define PIO_ELSR_P21_Pos                    21                                             /**< (PIO_ELSR) Edge/Level Interrupt Source Selection Position */
#define PIO_ELSR_P21_Msk                    (0x1U << PIO_ELSR_P21_Pos)                     /**< (PIO_ELSR) Edge/Level Interrupt Source Selection Mask */
#define PIO_ELSR_P21                        PIO_ELSR_P21_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_ELSR_P21_Msk instead */
#define PIO_ELSR_P22_Pos                    22                                             /**< (PIO_ELSR) Edge/Level Interrupt Source Selection Position */
#define PIO_ELSR_P22_Msk                    (0x1U << PIO_ELSR_P22_Pos)                     /**< (PIO_ELSR) Edge/Level Interrupt Source Selection Mask */
#define PIO_ELSR_P22                        PIO_ELSR_P22_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_ELSR_P22_Msk instead */
#define PIO_ELSR_P23_Pos                    23                                             /**< (PIO_ELSR) Edge/Level Interrupt Source Selection Position */
#define PIO_ELSR_P23_Msk                    (0x1U << PIO_ELSR_P23_Pos)                     /**< (PIO_ELSR) Edge/Level Interrupt Source Selection Mask */
#define PIO_ELSR_P23                        PIO_ELSR_P23_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_ELSR_P23_Msk instead */
#define PIO_ELSR_P24_Pos                    24                                             /**< (PIO_ELSR) Edge/Level Interrupt Source Selection Position */
#define PIO_ELSR_P24_Msk                    (0x1U << PIO_ELSR_P24_Pos)                     /**< (PIO_ELSR) Edge/Level Interrupt Source Selection Mask */
#define PIO_ELSR_P24                        PIO_ELSR_P24_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_ELSR_P24_Msk instead */
#define PIO_ELSR_P25_Pos                    25                                             /**< (PIO_ELSR) Edge/Level Interrupt Source Selection Position */
#define PIO_ELSR_P25_Msk                    (0x1U << PIO_ELSR_P25_Pos)                     /**< (PIO_ELSR) Edge/Level Interrupt Source Selection Mask */
#define PIO_ELSR_P25                        PIO_ELSR_P25_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_ELSR_P25_Msk instead */
#define PIO_ELSR_P26_Pos                    26                                             /**< (PIO_ELSR) Edge/Level Interrupt Source Selection Position */
#define PIO_ELSR_P26_Msk                    (0x1U << PIO_ELSR_P26_Pos)                     /**< (PIO_ELSR) Edge/Level Interrupt Source Selection Mask */
#define PIO_ELSR_P26                        PIO_ELSR_P26_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_ELSR_P26_Msk instead */
#define PIO_ELSR_P27_Pos                    27                                             /**< (PIO_ELSR) Edge/Level Interrupt Source Selection Position */
#define PIO_ELSR_P27_Msk                    (0x1U << PIO_ELSR_P27_Pos)                     /**< (PIO_ELSR) Edge/Level Interrupt Source Selection Mask */
#define PIO_ELSR_P27                        PIO_ELSR_P27_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_ELSR_P27_Msk instead */
#define PIO_ELSR_P28_Pos                    28                                             /**< (PIO_ELSR) Edge/Level Interrupt Source Selection Position */
#define PIO_ELSR_P28_Msk                    (0x1U << PIO_ELSR_P28_Pos)                     /**< (PIO_ELSR) Edge/Level Interrupt Source Selection Mask */
#define PIO_ELSR_P28                        PIO_ELSR_P28_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_ELSR_P28_Msk instead */
#define PIO_ELSR_P29_Pos                    29                                             /**< (PIO_ELSR) Edge/Level Interrupt Source Selection Position */
#define PIO_ELSR_P29_Msk                    (0x1U << PIO_ELSR_P29_Pos)                     /**< (PIO_ELSR) Edge/Level Interrupt Source Selection Mask */
#define PIO_ELSR_P29                        PIO_ELSR_P29_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_ELSR_P29_Msk instead */
#define PIO_ELSR_P30_Pos                    30                                             /**< (PIO_ELSR) Edge/Level Interrupt Source Selection Position */
#define PIO_ELSR_P30_Msk                    (0x1U << PIO_ELSR_P30_Pos)                     /**< (PIO_ELSR) Edge/Level Interrupt Source Selection Mask */
#define PIO_ELSR_P30                        PIO_ELSR_P30_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_ELSR_P30_Msk instead */
#define PIO_ELSR_P31_Pos                    31                                             /**< (PIO_ELSR) Edge/Level Interrupt Source Selection Position */
#define PIO_ELSR_P31_Msk                    (0x1U << PIO_ELSR_P31_Pos)                     /**< (PIO_ELSR) Edge/Level Interrupt Source Selection Mask */
#define PIO_ELSR_P31                        PIO_ELSR_P31_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_ELSR_P31_Msk instead */
#define PIO_ELSR_P_Pos                      0                                              /**< (PIO_ELSR Position) Edge/Level Interrupt Source Selection */
#define PIO_ELSR_P_Msk                      (0xFFFFFFFFU << PIO_ELSR_P_Pos)                /**< (PIO_ELSR Mask) P */
#define PIO_ELSR_P(value)                   (PIO_ELSR_P_Msk & ((value) << PIO_ELSR_P_Pos))  
#define PIO_ELSR_MASK                       (0xFFFFFFFFU)                                  /**< \deprecated (PIO_ELSR) Register MASK  (Use PIO_ELSR_Msk instead)  */
#define PIO_ELSR_Msk                        (0xFFFFFFFFU)                                  /**< (PIO_ELSR) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- PIO_FELLSR : (PIO Offset: 0xd0) (/W 32) Falling Edge/Low-Level Select Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t P0:1;                      /**< bit:      0  Falling Edge/Low-Level Interrupt Selection */
    uint32_t P1:1;                      /**< bit:      1  Falling Edge/Low-Level Interrupt Selection */
    uint32_t P2:1;                      /**< bit:      2  Falling Edge/Low-Level Interrupt Selection */
    uint32_t P3:1;                      /**< bit:      3  Falling Edge/Low-Level Interrupt Selection */
    uint32_t P4:1;                      /**< bit:      4  Falling Edge/Low-Level Interrupt Selection */
    uint32_t P5:1;                      /**< bit:      5  Falling Edge/Low-Level Interrupt Selection */
    uint32_t P6:1;                      /**< bit:      6  Falling Edge/Low-Level Interrupt Selection */
    uint32_t P7:1;                      /**< bit:      7  Falling Edge/Low-Level Interrupt Selection */
    uint32_t P8:1;                      /**< bit:      8  Falling Edge/Low-Level Interrupt Selection */
    uint32_t P9:1;                      /**< bit:      9  Falling Edge/Low-Level Interrupt Selection */
    uint32_t P10:1;                     /**< bit:     10  Falling Edge/Low-Level Interrupt Selection */
    uint32_t P11:1;                     /**< bit:     11  Falling Edge/Low-Level Interrupt Selection */
    uint32_t P12:1;                     /**< bit:     12  Falling Edge/Low-Level Interrupt Selection */
    uint32_t P13:1;                     /**< bit:     13  Falling Edge/Low-Level Interrupt Selection */
    uint32_t P14:1;                     /**< bit:     14  Falling Edge/Low-Level Interrupt Selection */
    uint32_t P15:1;                     /**< bit:     15  Falling Edge/Low-Level Interrupt Selection */
    uint32_t P16:1;                     /**< bit:     16  Falling Edge/Low-Level Interrupt Selection */
    uint32_t P17:1;                     /**< bit:     17  Falling Edge/Low-Level Interrupt Selection */
    uint32_t P18:1;                     /**< bit:     18  Falling Edge/Low-Level Interrupt Selection */
    uint32_t P19:1;                     /**< bit:     19  Falling Edge/Low-Level Interrupt Selection */
    uint32_t P20:1;                     /**< bit:     20  Falling Edge/Low-Level Interrupt Selection */
    uint32_t P21:1;                     /**< bit:     21  Falling Edge/Low-Level Interrupt Selection */
    uint32_t P22:1;                     /**< bit:     22  Falling Edge/Low-Level Interrupt Selection */
    uint32_t P23:1;                     /**< bit:     23  Falling Edge/Low-Level Interrupt Selection */
    uint32_t P24:1;                     /**< bit:     24  Falling Edge/Low-Level Interrupt Selection */
    uint32_t P25:1;                     /**< bit:     25  Falling Edge/Low-Level Interrupt Selection */
    uint32_t P26:1;                     /**< bit:     26  Falling Edge/Low-Level Interrupt Selection */
    uint32_t P27:1;                     /**< bit:     27  Falling Edge/Low-Level Interrupt Selection */
    uint32_t P28:1;                     /**< bit:     28  Falling Edge/Low-Level Interrupt Selection */
    uint32_t P29:1;                     /**< bit:     29  Falling Edge/Low-Level Interrupt Selection */
    uint32_t P30:1;                     /**< bit:     30  Falling Edge/Low-Level Interrupt Selection */
    uint32_t P31:1;                     /**< bit:     31  Falling Edge/Low-Level Interrupt Selection */
  } bit;                                /**< Structure used for bit  access */
  struct {
    uint32_t P:32;                      /**< bit:  0..31  Falling Edge/Low-Level Interrupt Selection */
  } vec;                                /**< Structure used for vec  access  */
  uint32_t reg;                         /**< Type used for register access */
} PIO_FELLSR_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define PIO_FELLSR_OFFSET                   (0xD0)                                        /**<  (PIO_FELLSR) Falling Edge/Low-Level Select Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define PIO_FELLSR_P0_Pos                   0                                              /**< (PIO_FELLSR) Falling Edge/Low-Level Interrupt Selection Position */
#define PIO_FELLSR_P0_Msk                   (0x1U << PIO_FELLSR_P0_Pos)                    /**< (PIO_FELLSR) Falling Edge/Low-Level Interrupt Selection Mask */
#define PIO_FELLSR_P0                       PIO_FELLSR_P0_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_FELLSR_P0_Msk instead */
#define PIO_FELLSR_P1_Pos                   1                                              /**< (PIO_FELLSR) Falling Edge/Low-Level Interrupt Selection Position */
#define PIO_FELLSR_P1_Msk                   (0x1U << PIO_FELLSR_P1_Pos)                    /**< (PIO_FELLSR) Falling Edge/Low-Level Interrupt Selection Mask */
#define PIO_FELLSR_P1                       PIO_FELLSR_P1_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_FELLSR_P1_Msk instead */
#define PIO_FELLSR_P2_Pos                   2                                              /**< (PIO_FELLSR) Falling Edge/Low-Level Interrupt Selection Position */
#define PIO_FELLSR_P2_Msk                   (0x1U << PIO_FELLSR_P2_Pos)                    /**< (PIO_FELLSR) Falling Edge/Low-Level Interrupt Selection Mask */
#define PIO_FELLSR_P2                       PIO_FELLSR_P2_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_FELLSR_P2_Msk instead */
#define PIO_FELLSR_P3_Pos                   3                                              /**< (PIO_FELLSR) Falling Edge/Low-Level Interrupt Selection Position */
#define PIO_FELLSR_P3_Msk                   (0x1U << PIO_FELLSR_P3_Pos)                    /**< (PIO_FELLSR) Falling Edge/Low-Level Interrupt Selection Mask */
#define PIO_FELLSR_P3                       PIO_FELLSR_P3_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_FELLSR_P3_Msk instead */
#define PIO_FELLSR_P4_Pos                   4                                              /**< (PIO_FELLSR) Falling Edge/Low-Level Interrupt Selection Position */
#define PIO_FELLSR_P4_Msk                   (0x1U << PIO_FELLSR_P4_Pos)                    /**< (PIO_FELLSR) Falling Edge/Low-Level Interrupt Selection Mask */
#define PIO_FELLSR_P4                       PIO_FELLSR_P4_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_FELLSR_P4_Msk instead */
#define PIO_FELLSR_P5_Pos                   5                                              /**< (PIO_FELLSR) Falling Edge/Low-Level Interrupt Selection Position */
#define PIO_FELLSR_P5_Msk                   (0x1U << PIO_FELLSR_P5_Pos)                    /**< (PIO_FELLSR) Falling Edge/Low-Level Interrupt Selection Mask */
#define PIO_FELLSR_P5                       PIO_FELLSR_P5_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_FELLSR_P5_Msk instead */
#define PIO_FELLSR_P6_Pos                   6                                              /**< (PIO_FELLSR) Falling Edge/Low-Level Interrupt Selection Position */
#define PIO_FELLSR_P6_Msk                   (0x1U << PIO_FELLSR_P6_Pos)                    /**< (PIO_FELLSR) Falling Edge/Low-Level Interrupt Selection Mask */
#define PIO_FELLSR_P6                       PIO_FELLSR_P6_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_FELLSR_P6_Msk instead */
#define PIO_FELLSR_P7_Pos                   7                                              /**< (PIO_FELLSR) Falling Edge/Low-Level Interrupt Selection Position */
#define PIO_FELLSR_P7_Msk                   (0x1U << PIO_FELLSR_P7_Pos)                    /**< (PIO_FELLSR) Falling Edge/Low-Level Interrupt Selection Mask */
#define PIO_FELLSR_P7                       PIO_FELLSR_P7_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_FELLSR_P7_Msk instead */
#define PIO_FELLSR_P8_Pos                   8                                              /**< (PIO_FELLSR) Falling Edge/Low-Level Interrupt Selection Position */
#define PIO_FELLSR_P8_Msk                   (0x1U << PIO_FELLSR_P8_Pos)                    /**< (PIO_FELLSR) Falling Edge/Low-Level Interrupt Selection Mask */
#define PIO_FELLSR_P8                       PIO_FELLSR_P8_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_FELLSR_P8_Msk instead */
#define PIO_FELLSR_P9_Pos                   9                                              /**< (PIO_FELLSR) Falling Edge/Low-Level Interrupt Selection Position */
#define PIO_FELLSR_P9_Msk                   (0x1U << PIO_FELLSR_P9_Pos)                    /**< (PIO_FELLSR) Falling Edge/Low-Level Interrupt Selection Mask */
#define PIO_FELLSR_P9                       PIO_FELLSR_P9_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_FELLSR_P9_Msk instead */
#define PIO_FELLSR_P10_Pos                  10                                             /**< (PIO_FELLSR) Falling Edge/Low-Level Interrupt Selection Position */
#define PIO_FELLSR_P10_Msk                  (0x1U << PIO_FELLSR_P10_Pos)                   /**< (PIO_FELLSR) Falling Edge/Low-Level Interrupt Selection Mask */
#define PIO_FELLSR_P10                      PIO_FELLSR_P10_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_FELLSR_P10_Msk instead */
#define PIO_FELLSR_P11_Pos                  11                                             /**< (PIO_FELLSR) Falling Edge/Low-Level Interrupt Selection Position */
#define PIO_FELLSR_P11_Msk                  (0x1U << PIO_FELLSR_P11_Pos)                   /**< (PIO_FELLSR) Falling Edge/Low-Level Interrupt Selection Mask */
#define PIO_FELLSR_P11                      PIO_FELLSR_P11_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_FELLSR_P11_Msk instead */
#define PIO_FELLSR_P12_Pos                  12                                             /**< (PIO_FELLSR) Falling Edge/Low-Level Interrupt Selection Position */
#define PIO_FELLSR_P12_Msk                  (0x1U << PIO_FELLSR_P12_Pos)                   /**< (PIO_FELLSR) Falling Edge/Low-Level Interrupt Selection Mask */
#define PIO_FELLSR_P12                      PIO_FELLSR_P12_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_FELLSR_P12_Msk instead */
#define PIO_FELLSR_P13_Pos                  13                                             /**< (PIO_FELLSR) Falling Edge/Low-Level Interrupt Selection Position */
#define PIO_FELLSR_P13_Msk                  (0x1U << PIO_FELLSR_P13_Pos)                   /**< (PIO_FELLSR) Falling Edge/Low-Level Interrupt Selection Mask */
#define PIO_FELLSR_P13                      PIO_FELLSR_P13_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_FELLSR_P13_Msk instead */
#define PIO_FELLSR_P14_Pos                  14                                             /**< (PIO_FELLSR) Falling Edge/Low-Level Interrupt Selection Position */
#define PIO_FELLSR_P14_Msk                  (0x1U << PIO_FELLSR_P14_Pos)                   /**< (PIO_FELLSR) Falling Edge/Low-Level Interrupt Selection Mask */
#define PIO_FELLSR_P14                      PIO_FELLSR_P14_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_FELLSR_P14_Msk instead */
#define PIO_FELLSR_P15_Pos                  15                                             /**< (PIO_FELLSR) Falling Edge/Low-Level Interrupt Selection Position */
#define PIO_FELLSR_P15_Msk                  (0x1U << PIO_FELLSR_P15_Pos)                   /**< (PIO_FELLSR) Falling Edge/Low-Level Interrupt Selection Mask */
#define PIO_FELLSR_P15                      PIO_FELLSR_P15_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_FELLSR_P15_Msk instead */
#define PIO_FELLSR_P16_Pos                  16                                             /**< (PIO_FELLSR) Falling Edge/Low-Level Interrupt Selection Position */
#define PIO_FELLSR_P16_Msk                  (0x1U << PIO_FELLSR_P16_Pos)                   /**< (PIO_FELLSR) Falling Edge/Low-Level Interrupt Selection Mask */
#define PIO_FELLSR_P16                      PIO_FELLSR_P16_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_FELLSR_P16_Msk instead */
#define PIO_FELLSR_P17_Pos                  17                                             /**< (PIO_FELLSR) Falling Edge/Low-Level Interrupt Selection Position */
#define PIO_FELLSR_P17_Msk                  (0x1U << PIO_FELLSR_P17_Pos)                   /**< (PIO_FELLSR) Falling Edge/Low-Level Interrupt Selection Mask */
#define PIO_FELLSR_P17                      PIO_FELLSR_P17_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_FELLSR_P17_Msk instead */
#define PIO_FELLSR_P18_Pos                  18                                             /**< (PIO_FELLSR) Falling Edge/Low-Level Interrupt Selection Position */
#define PIO_FELLSR_P18_Msk                  (0x1U << PIO_FELLSR_P18_Pos)                   /**< (PIO_FELLSR) Falling Edge/Low-Level Interrupt Selection Mask */
#define PIO_FELLSR_P18                      PIO_FELLSR_P18_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_FELLSR_P18_Msk instead */
#define PIO_FELLSR_P19_Pos                  19                                             /**< (PIO_FELLSR) Falling Edge/Low-Level Interrupt Selection Position */
#define PIO_FELLSR_P19_Msk                  (0x1U << PIO_FELLSR_P19_Pos)                   /**< (PIO_FELLSR) Falling Edge/Low-Level Interrupt Selection Mask */
#define PIO_FELLSR_P19                      PIO_FELLSR_P19_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_FELLSR_P19_Msk instead */
#define PIO_FELLSR_P20_Pos                  20                                             /**< (PIO_FELLSR) Falling Edge/Low-Level Interrupt Selection Position */
#define PIO_FELLSR_P20_Msk                  (0x1U << PIO_FELLSR_P20_Pos)                   /**< (PIO_FELLSR) Falling Edge/Low-Level Interrupt Selection Mask */
#define PIO_FELLSR_P20                      PIO_FELLSR_P20_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_FELLSR_P20_Msk instead */
#define PIO_FELLSR_P21_Pos                  21                                             /**< (PIO_FELLSR) Falling Edge/Low-Level Interrupt Selection Position */
#define PIO_FELLSR_P21_Msk                  (0x1U << PIO_FELLSR_P21_Pos)                   /**< (PIO_FELLSR) Falling Edge/Low-Level Interrupt Selection Mask */
#define PIO_FELLSR_P21                      PIO_FELLSR_P21_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_FELLSR_P21_Msk instead */
#define PIO_FELLSR_P22_Pos                  22                                             /**< (PIO_FELLSR) Falling Edge/Low-Level Interrupt Selection Position */
#define PIO_FELLSR_P22_Msk                  (0x1U << PIO_FELLSR_P22_Pos)                   /**< (PIO_FELLSR) Falling Edge/Low-Level Interrupt Selection Mask */
#define PIO_FELLSR_P22                      PIO_FELLSR_P22_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_FELLSR_P22_Msk instead */
#define PIO_FELLSR_P23_Pos                  23                                             /**< (PIO_FELLSR) Falling Edge/Low-Level Interrupt Selection Position */
#define PIO_FELLSR_P23_Msk                  (0x1U << PIO_FELLSR_P23_Pos)                   /**< (PIO_FELLSR) Falling Edge/Low-Level Interrupt Selection Mask */
#define PIO_FELLSR_P23                      PIO_FELLSR_P23_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_FELLSR_P23_Msk instead */
#define PIO_FELLSR_P24_Pos                  24                                             /**< (PIO_FELLSR) Falling Edge/Low-Level Interrupt Selection Position */
#define PIO_FELLSR_P24_Msk                  (0x1U << PIO_FELLSR_P24_Pos)                   /**< (PIO_FELLSR) Falling Edge/Low-Level Interrupt Selection Mask */
#define PIO_FELLSR_P24                      PIO_FELLSR_P24_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_FELLSR_P24_Msk instead */
#define PIO_FELLSR_P25_Pos                  25                                             /**< (PIO_FELLSR) Falling Edge/Low-Level Interrupt Selection Position */
#define PIO_FELLSR_P25_Msk                  (0x1U << PIO_FELLSR_P25_Pos)                   /**< (PIO_FELLSR) Falling Edge/Low-Level Interrupt Selection Mask */
#define PIO_FELLSR_P25                      PIO_FELLSR_P25_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_FELLSR_P25_Msk instead */
#define PIO_FELLSR_P26_Pos                  26                                             /**< (PIO_FELLSR) Falling Edge/Low-Level Interrupt Selection Position */
#define PIO_FELLSR_P26_Msk                  (0x1U << PIO_FELLSR_P26_Pos)                   /**< (PIO_FELLSR) Falling Edge/Low-Level Interrupt Selection Mask */
#define PIO_FELLSR_P26                      PIO_FELLSR_P26_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_FELLSR_P26_Msk instead */
#define PIO_FELLSR_P27_Pos                  27                                             /**< (PIO_FELLSR) Falling Edge/Low-Level Interrupt Selection Position */
#define PIO_FELLSR_P27_Msk                  (0x1U << PIO_FELLSR_P27_Pos)                   /**< (PIO_FELLSR) Falling Edge/Low-Level Interrupt Selection Mask */
#define PIO_FELLSR_P27                      PIO_FELLSR_P27_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_FELLSR_P27_Msk instead */
#define PIO_FELLSR_P28_Pos                  28                                             /**< (PIO_FELLSR) Falling Edge/Low-Level Interrupt Selection Position */
#define PIO_FELLSR_P28_Msk                  (0x1U << PIO_FELLSR_P28_Pos)                   /**< (PIO_FELLSR) Falling Edge/Low-Level Interrupt Selection Mask */
#define PIO_FELLSR_P28                      PIO_FELLSR_P28_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_FELLSR_P28_Msk instead */
#define PIO_FELLSR_P29_Pos                  29                                             /**< (PIO_FELLSR) Falling Edge/Low-Level Interrupt Selection Position */
#define PIO_FELLSR_P29_Msk                  (0x1U << PIO_FELLSR_P29_Pos)                   /**< (PIO_FELLSR) Falling Edge/Low-Level Interrupt Selection Mask */
#define PIO_FELLSR_P29                      PIO_FELLSR_P29_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_FELLSR_P29_Msk instead */
#define PIO_FELLSR_P30_Pos                  30                                             /**< (PIO_FELLSR) Falling Edge/Low-Level Interrupt Selection Position */
#define PIO_FELLSR_P30_Msk                  (0x1U << PIO_FELLSR_P30_Pos)                   /**< (PIO_FELLSR) Falling Edge/Low-Level Interrupt Selection Mask */
#define PIO_FELLSR_P30                      PIO_FELLSR_P30_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_FELLSR_P30_Msk instead */
#define PIO_FELLSR_P31_Pos                  31                                             /**< (PIO_FELLSR) Falling Edge/Low-Level Interrupt Selection Position */
#define PIO_FELLSR_P31_Msk                  (0x1U << PIO_FELLSR_P31_Pos)                   /**< (PIO_FELLSR) Falling Edge/Low-Level Interrupt Selection Mask */
#define PIO_FELLSR_P31                      PIO_FELLSR_P31_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_FELLSR_P31_Msk instead */
#define PIO_FELLSR_P_Pos                    0                                              /**< (PIO_FELLSR Position) Falling Edge/Low-Level Interrupt Selection */
#define PIO_FELLSR_P_Msk                    (0xFFFFFFFFU << PIO_FELLSR_P_Pos)              /**< (PIO_FELLSR Mask) P */
#define PIO_FELLSR_P(value)                 (PIO_FELLSR_P_Msk & ((value) << PIO_FELLSR_P_Pos))  
#define PIO_FELLSR_MASK                     (0xFFFFFFFFU)                                  /**< \deprecated (PIO_FELLSR) Register MASK  (Use PIO_FELLSR_Msk instead)  */
#define PIO_FELLSR_Msk                      (0xFFFFFFFFU)                                  /**< (PIO_FELLSR) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- PIO_REHLSR : (PIO Offset: 0xd4) (/W 32) Rising Edge/High-Level Select Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t P0:1;                      /**< bit:      0  Rising Edge/High-Level Interrupt Selection */
    uint32_t P1:1;                      /**< bit:      1  Rising Edge/High-Level Interrupt Selection */
    uint32_t P2:1;                      /**< bit:      2  Rising Edge/High-Level Interrupt Selection */
    uint32_t P3:1;                      /**< bit:      3  Rising Edge/High-Level Interrupt Selection */
    uint32_t P4:1;                      /**< bit:      4  Rising Edge/High-Level Interrupt Selection */
    uint32_t P5:1;                      /**< bit:      5  Rising Edge/High-Level Interrupt Selection */
    uint32_t P6:1;                      /**< bit:      6  Rising Edge/High-Level Interrupt Selection */
    uint32_t P7:1;                      /**< bit:      7  Rising Edge/High-Level Interrupt Selection */
    uint32_t P8:1;                      /**< bit:      8  Rising Edge/High-Level Interrupt Selection */
    uint32_t P9:1;                      /**< bit:      9  Rising Edge/High-Level Interrupt Selection */
    uint32_t P10:1;                     /**< bit:     10  Rising Edge/High-Level Interrupt Selection */
    uint32_t P11:1;                     /**< bit:     11  Rising Edge/High-Level Interrupt Selection */
    uint32_t P12:1;                     /**< bit:     12  Rising Edge/High-Level Interrupt Selection */
    uint32_t P13:1;                     /**< bit:     13  Rising Edge/High-Level Interrupt Selection */
    uint32_t P14:1;                     /**< bit:     14  Rising Edge/High-Level Interrupt Selection */
    uint32_t P15:1;                     /**< bit:     15  Rising Edge/High-Level Interrupt Selection */
    uint32_t P16:1;                     /**< bit:     16  Rising Edge/High-Level Interrupt Selection */
    uint32_t P17:1;                     /**< bit:     17  Rising Edge/High-Level Interrupt Selection */
    uint32_t P18:1;                     /**< bit:     18  Rising Edge/High-Level Interrupt Selection */
    uint32_t P19:1;                     /**< bit:     19  Rising Edge/High-Level Interrupt Selection */
    uint32_t P20:1;                     /**< bit:     20  Rising Edge/High-Level Interrupt Selection */
    uint32_t P21:1;                     /**< bit:     21  Rising Edge/High-Level Interrupt Selection */
    uint32_t P22:1;                     /**< bit:     22  Rising Edge/High-Level Interrupt Selection */
    uint32_t P23:1;                     /**< bit:     23  Rising Edge/High-Level Interrupt Selection */
    uint32_t P24:1;                     /**< bit:     24  Rising Edge/High-Level Interrupt Selection */
    uint32_t P25:1;                     /**< bit:     25  Rising Edge/High-Level Interrupt Selection */
    uint32_t P26:1;                     /**< bit:     26  Rising Edge/High-Level Interrupt Selection */
    uint32_t P27:1;                     /**< bit:     27  Rising Edge/High-Level Interrupt Selection */
    uint32_t P28:1;                     /**< bit:     28  Rising Edge/High-Level Interrupt Selection */
    uint32_t P29:1;                     /**< bit:     29  Rising Edge/High-Level Interrupt Selection */
    uint32_t P30:1;                     /**< bit:     30  Rising Edge/High-Level Interrupt Selection */
    uint32_t P31:1;                     /**< bit:     31  Rising Edge/High-Level Interrupt Selection */
  } bit;                                /**< Structure used for bit  access */
  struct {
    uint32_t P:32;                      /**< bit:  0..31  Rising Edge/High-Level Interrupt Selection */
  } vec;                                /**< Structure used for vec  access  */
  uint32_t reg;                         /**< Type used for register access */
} PIO_REHLSR_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define PIO_REHLSR_OFFSET                   (0xD4)                                        /**<  (PIO_REHLSR) Rising Edge/High-Level Select Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define PIO_REHLSR_P0_Pos                   0                                              /**< (PIO_REHLSR) Rising Edge/High-Level Interrupt Selection Position */
#define PIO_REHLSR_P0_Msk                   (0x1U << PIO_REHLSR_P0_Pos)                    /**< (PIO_REHLSR) Rising Edge/High-Level Interrupt Selection Mask */
#define PIO_REHLSR_P0                       PIO_REHLSR_P0_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_REHLSR_P0_Msk instead */
#define PIO_REHLSR_P1_Pos                   1                                              /**< (PIO_REHLSR) Rising Edge/High-Level Interrupt Selection Position */
#define PIO_REHLSR_P1_Msk                   (0x1U << PIO_REHLSR_P1_Pos)                    /**< (PIO_REHLSR) Rising Edge/High-Level Interrupt Selection Mask */
#define PIO_REHLSR_P1                       PIO_REHLSR_P1_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_REHLSR_P1_Msk instead */
#define PIO_REHLSR_P2_Pos                   2                                              /**< (PIO_REHLSR) Rising Edge/High-Level Interrupt Selection Position */
#define PIO_REHLSR_P2_Msk                   (0x1U << PIO_REHLSR_P2_Pos)                    /**< (PIO_REHLSR) Rising Edge/High-Level Interrupt Selection Mask */
#define PIO_REHLSR_P2                       PIO_REHLSR_P2_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_REHLSR_P2_Msk instead */
#define PIO_REHLSR_P3_Pos                   3                                              /**< (PIO_REHLSR) Rising Edge/High-Level Interrupt Selection Position */
#define PIO_REHLSR_P3_Msk                   (0x1U << PIO_REHLSR_P3_Pos)                    /**< (PIO_REHLSR) Rising Edge/High-Level Interrupt Selection Mask */
#define PIO_REHLSR_P3                       PIO_REHLSR_P3_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_REHLSR_P3_Msk instead */
#define PIO_REHLSR_P4_Pos                   4                                              /**< (PIO_REHLSR) Rising Edge/High-Level Interrupt Selection Position */
#define PIO_REHLSR_P4_Msk                   (0x1U << PIO_REHLSR_P4_Pos)                    /**< (PIO_REHLSR) Rising Edge/High-Level Interrupt Selection Mask */
#define PIO_REHLSR_P4                       PIO_REHLSR_P4_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_REHLSR_P4_Msk instead */
#define PIO_REHLSR_P5_Pos                   5                                              /**< (PIO_REHLSR) Rising Edge/High-Level Interrupt Selection Position */
#define PIO_REHLSR_P5_Msk                   (0x1U << PIO_REHLSR_P5_Pos)                    /**< (PIO_REHLSR) Rising Edge/High-Level Interrupt Selection Mask */
#define PIO_REHLSR_P5                       PIO_REHLSR_P5_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_REHLSR_P5_Msk instead */
#define PIO_REHLSR_P6_Pos                   6                                              /**< (PIO_REHLSR) Rising Edge/High-Level Interrupt Selection Position */
#define PIO_REHLSR_P6_Msk                   (0x1U << PIO_REHLSR_P6_Pos)                    /**< (PIO_REHLSR) Rising Edge/High-Level Interrupt Selection Mask */
#define PIO_REHLSR_P6                       PIO_REHLSR_P6_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_REHLSR_P6_Msk instead */
#define PIO_REHLSR_P7_Pos                   7                                              /**< (PIO_REHLSR) Rising Edge/High-Level Interrupt Selection Position */
#define PIO_REHLSR_P7_Msk                   (0x1U << PIO_REHLSR_P7_Pos)                    /**< (PIO_REHLSR) Rising Edge/High-Level Interrupt Selection Mask */
#define PIO_REHLSR_P7                       PIO_REHLSR_P7_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_REHLSR_P7_Msk instead */
#define PIO_REHLSR_P8_Pos                   8                                              /**< (PIO_REHLSR) Rising Edge/High-Level Interrupt Selection Position */
#define PIO_REHLSR_P8_Msk                   (0x1U << PIO_REHLSR_P8_Pos)                    /**< (PIO_REHLSR) Rising Edge/High-Level Interrupt Selection Mask */
#define PIO_REHLSR_P8                       PIO_REHLSR_P8_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_REHLSR_P8_Msk instead */
#define PIO_REHLSR_P9_Pos                   9                                              /**< (PIO_REHLSR) Rising Edge/High-Level Interrupt Selection Position */
#define PIO_REHLSR_P9_Msk                   (0x1U << PIO_REHLSR_P9_Pos)                    /**< (PIO_REHLSR) Rising Edge/High-Level Interrupt Selection Mask */
#define PIO_REHLSR_P9                       PIO_REHLSR_P9_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_REHLSR_P9_Msk instead */
#define PIO_REHLSR_P10_Pos                  10                                             /**< (PIO_REHLSR) Rising Edge/High-Level Interrupt Selection Position */
#define PIO_REHLSR_P10_Msk                  (0x1U << PIO_REHLSR_P10_Pos)                   /**< (PIO_REHLSR) Rising Edge/High-Level Interrupt Selection Mask */
#define PIO_REHLSR_P10                      PIO_REHLSR_P10_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_REHLSR_P10_Msk instead */
#define PIO_REHLSR_P11_Pos                  11                                             /**< (PIO_REHLSR) Rising Edge/High-Level Interrupt Selection Position */
#define PIO_REHLSR_P11_Msk                  (0x1U << PIO_REHLSR_P11_Pos)                   /**< (PIO_REHLSR) Rising Edge/High-Level Interrupt Selection Mask */
#define PIO_REHLSR_P11                      PIO_REHLSR_P11_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_REHLSR_P11_Msk instead */
#define PIO_REHLSR_P12_Pos                  12                                             /**< (PIO_REHLSR) Rising Edge/High-Level Interrupt Selection Position */
#define PIO_REHLSR_P12_Msk                  (0x1U << PIO_REHLSR_P12_Pos)                   /**< (PIO_REHLSR) Rising Edge/High-Level Interrupt Selection Mask */
#define PIO_REHLSR_P12                      PIO_REHLSR_P12_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_REHLSR_P12_Msk instead */
#define PIO_REHLSR_P13_Pos                  13                                             /**< (PIO_REHLSR) Rising Edge/High-Level Interrupt Selection Position */
#define PIO_REHLSR_P13_Msk                  (0x1U << PIO_REHLSR_P13_Pos)                   /**< (PIO_REHLSR) Rising Edge/High-Level Interrupt Selection Mask */
#define PIO_REHLSR_P13                      PIO_REHLSR_P13_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_REHLSR_P13_Msk instead */
#define PIO_REHLSR_P14_Pos                  14                                             /**< (PIO_REHLSR) Rising Edge/High-Level Interrupt Selection Position */
#define PIO_REHLSR_P14_Msk                  (0x1U << PIO_REHLSR_P14_Pos)                   /**< (PIO_REHLSR) Rising Edge/High-Level Interrupt Selection Mask */
#define PIO_REHLSR_P14                      PIO_REHLSR_P14_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_REHLSR_P14_Msk instead */
#define PIO_REHLSR_P15_Pos                  15                                             /**< (PIO_REHLSR) Rising Edge/High-Level Interrupt Selection Position */
#define PIO_REHLSR_P15_Msk                  (0x1U << PIO_REHLSR_P15_Pos)                   /**< (PIO_REHLSR) Rising Edge/High-Level Interrupt Selection Mask */
#define PIO_REHLSR_P15                      PIO_REHLSR_P15_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_REHLSR_P15_Msk instead */
#define PIO_REHLSR_P16_Pos                  16                                             /**< (PIO_REHLSR) Rising Edge/High-Level Interrupt Selection Position */
#define PIO_REHLSR_P16_Msk                  (0x1U << PIO_REHLSR_P16_Pos)                   /**< (PIO_REHLSR) Rising Edge/High-Level Interrupt Selection Mask */
#define PIO_REHLSR_P16                      PIO_REHLSR_P16_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_REHLSR_P16_Msk instead */
#define PIO_REHLSR_P17_Pos                  17                                             /**< (PIO_REHLSR) Rising Edge/High-Level Interrupt Selection Position */
#define PIO_REHLSR_P17_Msk                  (0x1U << PIO_REHLSR_P17_Pos)                   /**< (PIO_REHLSR) Rising Edge/High-Level Interrupt Selection Mask */
#define PIO_REHLSR_P17                      PIO_REHLSR_P17_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_REHLSR_P17_Msk instead */
#define PIO_REHLSR_P18_Pos                  18                                             /**< (PIO_REHLSR) Rising Edge/High-Level Interrupt Selection Position */
#define PIO_REHLSR_P18_Msk                  (0x1U << PIO_REHLSR_P18_Pos)                   /**< (PIO_REHLSR) Rising Edge/High-Level Interrupt Selection Mask */
#define PIO_REHLSR_P18                      PIO_REHLSR_P18_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_REHLSR_P18_Msk instead */
#define PIO_REHLSR_P19_Pos                  19                                             /**< (PIO_REHLSR) Rising Edge/High-Level Interrupt Selection Position */
#define PIO_REHLSR_P19_Msk                  (0x1U << PIO_REHLSR_P19_Pos)                   /**< (PIO_REHLSR) Rising Edge/High-Level Interrupt Selection Mask */
#define PIO_REHLSR_P19                      PIO_REHLSR_P19_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_REHLSR_P19_Msk instead */
#define PIO_REHLSR_P20_Pos                  20                                             /**< (PIO_REHLSR) Rising Edge/High-Level Interrupt Selection Position */
#define PIO_REHLSR_P20_Msk                  (0x1U << PIO_REHLSR_P20_Pos)                   /**< (PIO_REHLSR) Rising Edge/High-Level Interrupt Selection Mask */
#define PIO_REHLSR_P20                      PIO_REHLSR_P20_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_REHLSR_P20_Msk instead */
#define PIO_REHLSR_P21_Pos                  21                                             /**< (PIO_REHLSR) Rising Edge/High-Level Interrupt Selection Position */
#define PIO_REHLSR_P21_Msk                  (0x1U << PIO_REHLSR_P21_Pos)                   /**< (PIO_REHLSR) Rising Edge/High-Level Interrupt Selection Mask */
#define PIO_REHLSR_P21                      PIO_REHLSR_P21_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_REHLSR_P21_Msk instead */
#define PIO_REHLSR_P22_Pos                  22                                             /**< (PIO_REHLSR) Rising Edge/High-Level Interrupt Selection Position */
#define PIO_REHLSR_P22_Msk                  (0x1U << PIO_REHLSR_P22_Pos)                   /**< (PIO_REHLSR) Rising Edge/High-Level Interrupt Selection Mask */
#define PIO_REHLSR_P22                      PIO_REHLSR_P22_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_REHLSR_P22_Msk instead */
#define PIO_REHLSR_P23_Pos                  23                                             /**< (PIO_REHLSR) Rising Edge/High-Level Interrupt Selection Position */
#define PIO_REHLSR_P23_Msk                  (0x1U << PIO_REHLSR_P23_Pos)                   /**< (PIO_REHLSR) Rising Edge/High-Level Interrupt Selection Mask */
#define PIO_REHLSR_P23                      PIO_REHLSR_P23_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_REHLSR_P23_Msk instead */
#define PIO_REHLSR_P24_Pos                  24                                             /**< (PIO_REHLSR) Rising Edge/High-Level Interrupt Selection Position */
#define PIO_REHLSR_P24_Msk                  (0x1U << PIO_REHLSR_P24_Pos)                   /**< (PIO_REHLSR) Rising Edge/High-Level Interrupt Selection Mask */
#define PIO_REHLSR_P24                      PIO_REHLSR_P24_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_REHLSR_P24_Msk instead */
#define PIO_REHLSR_P25_Pos                  25                                             /**< (PIO_REHLSR) Rising Edge/High-Level Interrupt Selection Position */
#define PIO_REHLSR_P25_Msk                  (0x1U << PIO_REHLSR_P25_Pos)                   /**< (PIO_REHLSR) Rising Edge/High-Level Interrupt Selection Mask */
#define PIO_REHLSR_P25                      PIO_REHLSR_P25_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_REHLSR_P25_Msk instead */
#define PIO_REHLSR_P26_Pos                  26                                             /**< (PIO_REHLSR) Rising Edge/High-Level Interrupt Selection Position */
#define PIO_REHLSR_P26_Msk                  (0x1U << PIO_REHLSR_P26_Pos)                   /**< (PIO_REHLSR) Rising Edge/High-Level Interrupt Selection Mask */
#define PIO_REHLSR_P26                      PIO_REHLSR_P26_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_REHLSR_P26_Msk instead */
#define PIO_REHLSR_P27_Pos                  27                                             /**< (PIO_REHLSR) Rising Edge/High-Level Interrupt Selection Position */
#define PIO_REHLSR_P27_Msk                  (0x1U << PIO_REHLSR_P27_Pos)                   /**< (PIO_REHLSR) Rising Edge/High-Level Interrupt Selection Mask */
#define PIO_REHLSR_P27                      PIO_REHLSR_P27_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_REHLSR_P27_Msk instead */
#define PIO_REHLSR_P28_Pos                  28                                             /**< (PIO_REHLSR) Rising Edge/High-Level Interrupt Selection Position */
#define PIO_REHLSR_P28_Msk                  (0x1U << PIO_REHLSR_P28_Pos)                   /**< (PIO_REHLSR) Rising Edge/High-Level Interrupt Selection Mask */
#define PIO_REHLSR_P28                      PIO_REHLSR_P28_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_REHLSR_P28_Msk instead */
#define PIO_REHLSR_P29_Pos                  29                                             /**< (PIO_REHLSR) Rising Edge/High-Level Interrupt Selection Position */
#define PIO_REHLSR_P29_Msk                  (0x1U << PIO_REHLSR_P29_Pos)                   /**< (PIO_REHLSR) Rising Edge/High-Level Interrupt Selection Mask */
#define PIO_REHLSR_P29                      PIO_REHLSR_P29_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_REHLSR_P29_Msk instead */
#define PIO_REHLSR_P30_Pos                  30                                             /**< (PIO_REHLSR) Rising Edge/High-Level Interrupt Selection Position */
#define PIO_REHLSR_P30_Msk                  (0x1U << PIO_REHLSR_P30_Pos)                   /**< (PIO_REHLSR) Rising Edge/High-Level Interrupt Selection Mask */
#define PIO_REHLSR_P30                      PIO_REHLSR_P30_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_REHLSR_P30_Msk instead */
#define PIO_REHLSR_P31_Pos                  31                                             /**< (PIO_REHLSR) Rising Edge/High-Level Interrupt Selection Position */
#define PIO_REHLSR_P31_Msk                  (0x1U << PIO_REHLSR_P31_Pos)                   /**< (PIO_REHLSR) Rising Edge/High-Level Interrupt Selection Mask */
#define PIO_REHLSR_P31                      PIO_REHLSR_P31_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_REHLSR_P31_Msk instead */
#define PIO_REHLSR_P_Pos                    0                                              /**< (PIO_REHLSR Position) Rising Edge/High-Level Interrupt Selection */
#define PIO_REHLSR_P_Msk                    (0xFFFFFFFFU << PIO_REHLSR_P_Pos)              /**< (PIO_REHLSR Mask) P */
#define PIO_REHLSR_P(value)                 (PIO_REHLSR_P_Msk & ((value) << PIO_REHLSR_P_Pos))  
#define PIO_REHLSR_MASK                     (0xFFFFFFFFU)                                  /**< \deprecated (PIO_REHLSR) Register MASK  (Use PIO_REHLSR_Msk instead)  */
#define PIO_REHLSR_Msk                      (0xFFFFFFFFU)                                  /**< (PIO_REHLSR) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- PIO_FRLHSR : (PIO Offset: 0xd8) (R/ 32) Fall/Rise - Low/High Status Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t P0:1;                      /**< bit:      0  Edge/Level Interrupt Source Selection    */
    uint32_t P1:1;                      /**< bit:      1  Edge/Level Interrupt Source Selection    */
    uint32_t P2:1;                      /**< bit:      2  Edge/Level Interrupt Source Selection    */
    uint32_t P3:1;                      /**< bit:      3  Edge/Level Interrupt Source Selection    */
    uint32_t P4:1;                      /**< bit:      4  Edge/Level Interrupt Source Selection    */
    uint32_t P5:1;                      /**< bit:      5  Edge/Level Interrupt Source Selection    */
    uint32_t P6:1;                      /**< bit:      6  Edge/Level Interrupt Source Selection    */
    uint32_t P7:1;                      /**< bit:      7  Edge/Level Interrupt Source Selection    */
    uint32_t P8:1;                      /**< bit:      8  Edge/Level Interrupt Source Selection    */
    uint32_t P9:1;                      /**< bit:      9  Edge/Level Interrupt Source Selection    */
    uint32_t P10:1;                     /**< bit:     10  Edge/Level Interrupt Source Selection    */
    uint32_t P11:1;                     /**< bit:     11  Edge/Level Interrupt Source Selection    */
    uint32_t P12:1;                     /**< bit:     12  Edge/Level Interrupt Source Selection    */
    uint32_t P13:1;                     /**< bit:     13  Edge/Level Interrupt Source Selection    */
    uint32_t P14:1;                     /**< bit:     14  Edge/Level Interrupt Source Selection    */
    uint32_t P15:1;                     /**< bit:     15  Edge/Level Interrupt Source Selection    */
    uint32_t P16:1;                     /**< bit:     16  Edge/Level Interrupt Source Selection    */
    uint32_t P17:1;                     /**< bit:     17  Edge/Level Interrupt Source Selection    */
    uint32_t P18:1;                     /**< bit:     18  Edge/Level Interrupt Source Selection    */
    uint32_t P19:1;                     /**< bit:     19  Edge/Level Interrupt Source Selection    */
    uint32_t P20:1;                     /**< bit:     20  Edge/Level Interrupt Source Selection    */
    uint32_t P21:1;                     /**< bit:     21  Edge/Level Interrupt Source Selection    */
    uint32_t P22:1;                     /**< bit:     22  Edge/Level Interrupt Source Selection    */
    uint32_t P23:1;                     /**< bit:     23  Edge/Level Interrupt Source Selection    */
    uint32_t P24:1;                     /**< bit:     24  Edge/Level Interrupt Source Selection    */
    uint32_t P25:1;                     /**< bit:     25  Edge/Level Interrupt Source Selection    */
    uint32_t P26:1;                     /**< bit:     26  Edge/Level Interrupt Source Selection    */
    uint32_t P27:1;                     /**< bit:     27  Edge/Level Interrupt Source Selection    */
    uint32_t P28:1;                     /**< bit:     28  Edge/Level Interrupt Source Selection    */
    uint32_t P29:1;                     /**< bit:     29  Edge/Level Interrupt Source Selection    */
    uint32_t P30:1;                     /**< bit:     30  Edge/Level Interrupt Source Selection    */
    uint32_t P31:1;                     /**< bit:     31  Edge/Level Interrupt Source Selection    */
  } bit;                                /**< Structure used for bit  access */
  struct {
    uint32_t P:32;                      /**< bit:  0..31  Edge/Level Interrupt Source Selection    */
  } vec;                                /**< Structure used for vec  access  */
  uint32_t reg;                         /**< Type used for register access */
} PIO_FRLHSR_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define PIO_FRLHSR_OFFSET                   (0xD8)                                        /**<  (PIO_FRLHSR) Fall/Rise - Low/High Status Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define PIO_FRLHSR_P0_Pos                   0                                              /**< (PIO_FRLHSR) Edge/Level Interrupt Source Selection Position */
#define PIO_FRLHSR_P0_Msk                   (0x1U << PIO_FRLHSR_P0_Pos)                    /**< (PIO_FRLHSR) Edge/Level Interrupt Source Selection Mask */
#define PIO_FRLHSR_P0                       PIO_FRLHSR_P0_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_FRLHSR_P0_Msk instead */
#define PIO_FRLHSR_P1_Pos                   1                                              /**< (PIO_FRLHSR) Edge/Level Interrupt Source Selection Position */
#define PIO_FRLHSR_P1_Msk                   (0x1U << PIO_FRLHSR_P1_Pos)                    /**< (PIO_FRLHSR) Edge/Level Interrupt Source Selection Mask */
#define PIO_FRLHSR_P1                       PIO_FRLHSR_P1_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_FRLHSR_P1_Msk instead */
#define PIO_FRLHSR_P2_Pos                   2                                              /**< (PIO_FRLHSR) Edge/Level Interrupt Source Selection Position */
#define PIO_FRLHSR_P2_Msk                   (0x1U << PIO_FRLHSR_P2_Pos)                    /**< (PIO_FRLHSR) Edge/Level Interrupt Source Selection Mask */
#define PIO_FRLHSR_P2                       PIO_FRLHSR_P2_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_FRLHSR_P2_Msk instead */
#define PIO_FRLHSR_P3_Pos                   3                                              /**< (PIO_FRLHSR) Edge/Level Interrupt Source Selection Position */
#define PIO_FRLHSR_P3_Msk                   (0x1U << PIO_FRLHSR_P3_Pos)                    /**< (PIO_FRLHSR) Edge/Level Interrupt Source Selection Mask */
#define PIO_FRLHSR_P3                       PIO_FRLHSR_P3_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_FRLHSR_P3_Msk instead */
#define PIO_FRLHSR_P4_Pos                   4                                              /**< (PIO_FRLHSR) Edge/Level Interrupt Source Selection Position */
#define PIO_FRLHSR_P4_Msk                   (0x1U << PIO_FRLHSR_P4_Pos)                    /**< (PIO_FRLHSR) Edge/Level Interrupt Source Selection Mask */
#define PIO_FRLHSR_P4                       PIO_FRLHSR_P4_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_FRLHSR_P4_Msk instead */
#define PIO_FRLHSR_P5_Pos                   5                                              /**< (PIO_FRLHSR) Edge/Level Interrupt Source Selection Position */
#define PIO_FRLHSR_P5_Msk                   (0x1U << PIO_FRLHSR_P5_Pos)                    /**< (PIO_FRLHSR) Edge/Level Interrupt Source Selection Mask */
#define PIO_FRLHSR_P5                       PIO_FRLHSR_P5_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_FRLHSR_P5_Msk instead */
#define PIO_FRLHSR_P6_Pos                   6                                              /**< (PIO_FRLHSR) Edge/Level Interrupt Source Selection Position */
#define PIO_FRLHSR_P6_Msk                   (0x1U << PIO_FRLHSR_P6_Pos)                    /**< (PIO_FRLHSR) Edge/Level Interrupt Source Selection Mask */
#define PIO_FRLHSR_P6                       PIO_FRLHSR_P6_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_FRLHSR_P6_Msk instead */
#define PIO_FRLHSR_P7_Pos                   7                                              /**< (PIO_FRLHSR) Edge/Level Interrupt Source Selection Position */
#define PIO_FRLHSR_P7_Msk                   (0x1U << PIO_FRLHSR_P7_Pos)                    /**< (PIO_FRLHSR) Edge/Level Interrupt Source Selection Mask */
#define PIO_FRLHSR_P7                       PIO_FRLHSR_P7_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_FRLHSR_P7_Msk instead */
#define PIO_FRLHSR_P8_Pos                   8                                              /**< (PIO_FRLHSR) Edge/Level Interrupt Source Selection Position */
#define PIO_FRLHSR_P8_Msk                   (0x1U << PIO_FRLHSR_P8_Pos)                    /**< (PIO_FRLHSR) Edge/Level Interrupt Source Selection Mask */
#define PIO_FRLHSR_P8                       PIO_FRLHSR_P8_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_FRLHSR_P8_Msk instead */
#define PIO_FRLHSR_P9_Pos                   9                                              /**< (PIO_FRLHSR) Edge/Level Interrupt Source Selection Position */
#define PIO_FRLHSR_P9_Msk                   (0x1U << PIO_FRLHSR_P9_Pos)                    /**< (PIO_FRLHSR) Edge/Level Interrupt Source Selection Mask */
#define PIO_FRLHSR_P9                       PIO_FRLHSR_P9_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_FRLHSR_P9_Msk instead */
#define PIO_FRLHSR_P10_Pos                  10                                             /**< (PIO_FRLHSR) Edge/Level Interrupt Source Selection Position */
#define PIO_FRLHSR_P10_Msk                  (0x1U << PIO_FRLHSR_P10_Pos)                   /**< (PIO_FRLHSR) Edge/Level Interrupt Source Selection Mask */
#define PIO_FRLHSR_P10                      PIO_FRLHSR_P10_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_FRLHSR_P10_Msk instead */
#define PIO_FRLHSR_P11_Pos                  11                                             /**< (PIO_FRLHSR) Edge/Level Interrupt Source Selection Position */
#define PIO_FRLHSR_P11_Msk                  (0x1U << PIO_FRLHSR_P11_Pos)                   /**< (PIO_FRLHSR) Edge/Level Interrupt Source Selection Mask */
#define PIO_FRLHSR_P11                      PIO_FRLHSR_P11_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_FRLHSR_P11_Msk instead */
#define PIO_FRLHSR_P12_Pos                  12                                             /**< (PIO_FRLHSR) Edge/Level Interrupt Source Selection Position */
#define PIO_FRLHSR_P12_Msk                  (0x1U << PIO_FRLHSR_P12_Pos)                   /**< (PIO_FRLHSR) Edge/Level Interrupt Source Selection Mask */
#define PIO_FRLHSR_P12                      PIO_FRLHSR_P12_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_FRLHSR_P12_Msk instead */
#define PIO_FRLHSR_P13_Pos                  13                                             /**< (PIO_FRLHSR) Edge/Level Interrupt Source Selection Position */
#define PIO_FRLHSR_P13_Msk                  (0x1U << PIO_FRLHSR_P13_Pos)                   /**< (PIO_FRLHSR) Edge/Level Interrupt Source Selection Mask */
#define PIO_FRLHSR_P13                      PIO_FRLHSR_P13_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_FRLHSR_P13_Msk instead */
#define PIO_FRLHSR_P14_Pos                  14                                             /**< (PIO_FRLHSR) Edge/Level Interrupt Source Selection Position */
#define PIO_FRLHSR_P14_Msk                  (0x1U << PIO_FRLHSR_P14_Pos)                   /**< (PIO_FRLHSR) Edge/Level Interrupt Source Selection Mask */
#define PIO_FRLHSR_P14                      PIO_FRLHSR_P14_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_FRLHSR_P14_Msk instead */
#define PIO_FRLHSR_P15_Pos                  15                                             /**< (PIO_FRLHSR) Edge/Level Interrupt Source Selection Position */
#define PIO_FRLHSR_P15_Msk                  (0x1U << PIO_FRLHSR_P15_Pos)                   /**< (PIO_FRLHSR) Edge/Level Interrupt Source Selection Mask */
#define PIO_FRLHSR_P15                      PIO_FRLHSR_P15_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_FRLHSR_P15_Msk instead */
#define PIO_FRLHSR_P16_Pos                  16                                             /**< (PIO_FRLHSR) Edge/Level Interrupt Source Selection Position */
#define PIO_FRLHSR_P16_Msk                  (0x1U << PIO_FRLHSR_P16_Pos)                   /**< (PIO_FRLHSR) Edge/Level Interrupt Source Selection Mask */
#define PIO_FRLHSR_P16                      PIO_FRLHSR_P16_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_FRLHSR_P16_Msk instead */
#define PIO_FRLHSR_P17_Pos                  17                                             /**< (PIO_FRLHSR) Edge/Level Interrupt Source Selection Position */
#define PIO_FRLHSR_P17_Msk                  (0x1U << PIO_FRLHSR_P17_Pos)                   /**< (PIO_FRLHSR) Edge/Level Interrupt Source Selection Mask */
#define PIO_FRLHSR_P17                      PIO_FRLHSR_P17_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_FRLHSR_P17_Msk instead */
#define PIO_FRLHSR_P18_Pos                  18                                             /**< (PIO_FRLHSR) Edge/Level Interrupt Source Selection Position */
#define PIO_FRLHSR_P18_Msk                  (0x1U << PIO_FRLHSR_P18_Pos)                   /**< (PIO_FRLHSR) Edge/Level Interrupt Source Selection Mask */
#define PIO_FRLHSR_P18                      PIO_FRLHSR_P18_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_FRLHSR_P18_Msk instead */
#define PIO_FRLHSR_P19_Pos                  19                                             /**< (PIO_FRLHSR) Edge/Level Interrupt Source Selection Position */
#define PIO_FRLHSR_P19_Msk                  (0x1U << PIO_FRLHSR_P19_Pos)                   /**< (PIO_FRLHSR) Edge/Level Interrupt Source Selection Mask */
#define PIO_FRLHSR_P19                      PIO_FRLHSR_P19_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_FRLHSR_P19_Msk instead */
#define PIO_FRLHSR_P20_Pos                  20                                             /**< (PIO_FRLHSR) Edge/Level Interrupt Source Selection Position */
#define PIO_FRLHSR_P20_Msk                  (0x1U << PIO_FRLHSR_P20_Pos)                   /**< (PIO_FRLHSR) Edge/Level Interrupt Source Selection Mask */
#define PIO_FRLHSR_P20                      PIO_FRLHSR_P20_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_FRLHSR_P20_Msk instead */
#define PIO_FRLHSR_P21_Pos                  21                                             /**< (PIO_FRLHSR) Edge/Level Interrupt Source Selection Position */
#define PIO_FRLHSR_P21_Msk                  (0x1U << PIO_FRLHSR_P21_Pos)                   /**< (PIO_FRLHSR) Edge/Level Interrupt Source Selection Mask */
#define PIO_FRLHSR_P21                      PIO_FRLHSR_P21_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_FRLHSR_P21_Msk instead */
#define PIO_FRLHSR_P22_Pos                  22                                             /**< (PIO_FRLHSR) Edge/Level Interrupt Source Selection Position */
#define PIO_FRLHSR_P22_Msk                  (0x1U << PIO_FRLHSR_P22_Pos)                   /**< (PIO_FRLHSR) Edge/Level Interrupt Source Selection Mask */
#define PIO_FRLHSR_P22                      PIO_FRLHSR_P22_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_FRLHSR_P22_Msk instead */
#define PIO_FRLHSR_P23_Pos                  23                                             /**< (PIO_FRLHSR) Edge/Level Interrupt Source Selection Position */
#define PIO_FRLHSR_P23_Msk                  (0x1U << PIO_FRLHSR_P23_Pos)                   /**< (PIO_FRLHSR) Edge/Level Interrupt Source Selection Mask */
#define PIO_FRLHSR_P23                      PIO_FRLHSR_P23_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_FRLHSR_P23_Msk instead */
#define PIO_FRLHSR_P24_Pos                  24                                             /**< (PIO_FRLHSR) Edge/Level Interrupt Source Selection Position */
#define PIO_FRLHSR_P24_Msk                  (0x1U << PIO_FRLHSR_P24_Pos)                   /**< (PIO_FRLHSR) Edge/Level Interrupt Source Selection Mask */
#define PIO_FRLHSR_P24                      PIO_FRLHSR_P24_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_FRLHSR_P24_Msk instead */
#define PIO_FRLHSR_P25_Pos                  25                                             /**< (PIO_FRLHSR) Edge/Level Interrupt Source Selection Position */
#define PIO_FRLHSR_P25_Msk                  (0x1U << PIO_FRLHSR_P25_Pos)                   /**< (PIO_FRLHSR) Edge/Level Interrupt Source Selection Mask */
#define PIO_FRLHSR_P25                      PIO_FRLHSR_P25_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_FRLHSR_P25_Msk instead */
#define PIO_FRLHSR_P26_Pos                  26                                             /**< (PIO_FRLHSR) Edge/Level Interrupt Source Selection Position */
#define PIO_FRLHSR_P26_Msk                  (0x1U << PIO_FRLHSR_P26_Pos)                   /**< (PIO_FRLHSR) Edge/Level Interrupt Source Selection Mask */
#define PIO_FRLHSR_P26                      PIO_FRLHSR_P26_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_FRLHSR_P26_Msk instead */
#define PIO_FRLHSR_P27_Pos                  27                                             /**< (PIO_FRLHSR) Edge/Level Interrupt Source Selection Position */
#define PIO_FRLHSR_P27_Msk                  (0x1U << PIO_FRLHSR_P27_Pos)                   /**< (PIO_FRLHSR) Edge/Level Interrupt Source Selection Mask */
#define PIO_FRLHSR_P27                      PIO_FRLHSR_P27_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_FRLHSR_P27_Msk instead */
#define PIO_FRLHSR_P28_Pos                  28                                             /**< (PIO_FRLHSR) Edge/Level Interrupt Source Selection Position */
#define PIO_FRLHSR_P28_Msk                  (0x1U << PIO_FRLHSR_P28_Pos)                   /**< (PIO_FRLHSR) Edge/Level Interrupt Source Selection Mask */
#define PIO_FRLHSR_P28                      PIO_FRLHSR_P28_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_FRLHSR_P28_Msk instead */
#define PIO_FRLHSR_P29_Pos                  29                                             /**< (PIO_FRLHSR) Edge/Level Interrupt Source Selection Position */
#define PIO_FRLHSR_P29_Msk                  (0x1U << PIO_FRLHSR_P29_Pos)                   /**< (PIO_FRLHSR) Edge/Level Interrupt Source Selection Mask */
#define PIO_FRLHSR_P29                      PIO_FRLHSR_P29_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_FRLHSR_P29_Msk instead */
#define PIO_FRLHSR_P30_Pos                  30                                             /**< (PIO_FRLHSR) Edge/Level Interrupt Source Selection Position */
#define PIO_FRLHSR_P30_Msk                  (0x1U << PIO_FRLHSR_P30_Pos)                   /**< (PIO_FRLHSR) Edge/Level Interrupt Source Selection Mask */
#define PIO_FRLHSR_P30                      PIO_FRLHSR_P30_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_FRLHSR_P30_Msk instead */
#define PIO_FRLHSR_P31_Pos                  31                                             /**< (PIO_FRLHSR) Edge/Level Interrupt Source Selection Position */
#define PIO_FRLHSR_P31_Msk                  (0x1U << PIO_FRLHSR_P31_Pos)                   /**< (PIO_FRLHSR) Edge/Level Interrupt Source Selection Mask */
#define PIO_FRLHSR_P31                      PIO_FRLHSR_P31_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_FRLHSR_P31_Msk instead */
#define PIO_FRLHSR_P_Pos                    0                                              /**< (PIO_FRLHSR Position) Edge/Level Interrupt Source Selection */
#define PIO_FRLHSR_P_Msk                    (0xFFFFFFFFU << PIO_FRLHSR_P_Pos)              /**< (PIO_FRLHSR Mask) P */
#define PIO_FRLHSR_P(value)                 (PIO_FRLHSR_P_Msk & ((value) << PIO_FRLHSR_P_Pos))  
#define PIO_FRLHSR_MASK                     (0xFFFFFFFFU)                                  /**< \deprecated (PIO_FRLHSR) Register MASK  (Use PIO_FRLHSR_Msk instead)  */
#define PIO_FRLHSR_Msk                      (0xFFFFFFFFU)                                  /**< (PIO_FRLHSR) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- PIO_LOCKSR : (PIO Offset: 0xe0) (R/ 32) Lock Status -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t P0:1;                      /**< bit:      0  Lock Status                              */
    uint32_t P1:1;                      /**< bit:      1  Lock Status                              */
    uint32_t P2:1;                      /**< bit:      2  Lock Status                              */
    uint32_t P3:1;                      /**< bit:      3  Lock Status                              */
    uint32_t P4:1;                      /**< bit:      4  Lock Status                              */
    uint32_t P5:1;                      /**< bit:      5  Lock Status                              */
    uint32_t P6:1;                      /**< bit:      6  Lock Status                              */
    uint32_t P7:1;                      /**< bit:      7  Lock Status                              */
    uint32_t P8:1;                      /**< bit:      8  Lock Status                              */
    uint32_t P9:1;                      /**< bit:      9  Lock Status                              */
    uint32_t P10:1;                     /**< bit:     10  Lock Status                              */
    uint32_t P11:1;                     /**< bit:     11  Lock Status                              */
    uint32_t P12:1;                     /**< bit:     12  Lock Status                              */
    uint32_t P13:1;                     /**< bit:     13  Lock Status                              */
    uint32_t P14:1;                     /**< bit:     14  Lock Status                              */
    uint32_t P15:1;                     /**< bit:     15  Lock Status                              */
    uint32_t P16:1;                     /**< bit:     16  Lock Status                              */
    uint32_t P17:1;                     /**< bit:     17  Lock Status                              */
    uint32_t P18:1;                     /**< bit:     18  Lock Status                              */
    uint32_t P19:1;                     /**< bit:     19  Lock Status                              */
    uint32_t P20:1;                     /**< bit:     20  Lock Status                              */
    uint32_t P21:1;                     /**< bit:     21  Lock Status                              */
    uint32_t P22:1;                     /**< bit:     22  Lock Status                              */
    uint32_t P23:1;                     /**< bit:     23  Lock Status                              */
    uint32_t P24:1;                     /**< bit:     24  Lock Status                              */
    uint32_t P25:1;                     /**< bit:     25  Lock Status                              */
    uint32_t P26:1;                     /**< bit:     26  Lock Status                              */
    uint32_t P27:1;                     /**< bit:     27  Lock Status                              */
    uint32_t P28:1;                     /**< bit:     28  Lock Status                              */
    uint32_t P29:1;                     /**< bit:     29  Lock Status                              */
    uint32_t P30:1;                     /**< bit:     30  Lock Status                              */
    uint32_t P31:1;                     /**< bit:     31  Lock Status                              */
  } bit;                                /**< Structure used for bit  access */
  struct {
    uint32_t P:32;                      /**< bit:  0..31  Lock Status                              */
  } vec;                                /**< Structure used for vec  access  */
  uint32_t reg;                         /**< Type used for register access */
} PIO_LOCKSR_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define PIO_LOCKSR_OFFSET                   (0xE0)                                        /**<  (PIO_LOCKSR) Lock Status  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define PIO_LOCKSR_P0_Pos                   0                                              /**< (PIO_LOCKSR) Lock Status Position */
#define PIO_LOCKSR_P0_Msk                   (0x1U << PIO_LOCKSR_P0_Pos)                    /**< (PIO_LOCKSR) Lock Status Mask */
#define PIO_LOCKSR_P0                       PIO_LOCKSR_P0_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_LOCKSR_P0_Msk instead */
#define PIO_LOCKSR_P1_Pos                   1                                              /**< (PIO_LOCKSR) Lock Status Position */
#define PIO_LOCKSR_P1_Msk                   (0x1U << PIO_LOCKSR_P1_Pos)                    /**< (PIO_LOCKSR) Lock Status Mask */
#define PIO_LOCKSR_P1                       PIO_LOCKSR_P1_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_LOCKSR_P1_Msk instead */
#define PIO_LOCKSR_P2_Pos                   2                                              /**< (PIO_LOCKSR) Lock Status Position */
#define PIO_LOCKSR_P2_Msk                   (0x1U << PIO_LOCKSR_P2_Pos)                    /**< (PIO_LOCKSR) Lock Status Mask */
#define PIO_LOCKSR_P2                       PIO_LOCKSR_P2_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_LOCKSR_P2_Msk instead */
#define PIO_LOCKSR_P3_Pos                   3                                              /**< (PIO_LOCKSR) Lock Status Position */
#define PIO_LOCKSR_P3_Msk                   (0x1U << PIO_LOCKSR_P3_Pos)                    /**< (PIO_LOCKSR) Lock Status Mask */
#define PIO_LOCKSR_P3                       PIO_LOCKSR_P3_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_LOCKSR_P3_Msk instead */
#define PIO_LOCKSR_P4_Pos                   4                                              /**< (PIO_LOCKSR) Lock Status Position */
#define PIO_LOCKSR_P4_Msk                   (0x1U << PIO_LOCKSR_P4_Pos)                    /**< (PIO_LOCKSR) Lock Status Mask */
#define PIO_LOCKSR_P4                       PIO_LOCKSR_P4_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_LOCKSR_P4_Msk instead */
#define PIO_LOCKSR_P5_Pos                   5                                              /**< (PIO_LOCKSR) Lock Status Position */
#define PIO_LOCKSR_P5_Msk                   (0x1U << PIO_LOCKSR_P5_Pos)                    /**< (PIO_LOCKSR) Lock Status Mask */
#define PIO_LOCKSR_P5                       PIO_LOCKSR_P5_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_LOCKSR_P5_Msk instead */
#define PIO_LOCKSR_P6_Pos                   6                                              /**< (PIO_LOCKSR) Lock Status Position */
#define PIO_LOCKSR_P6_Msk                   (0x1U << PIO_LOCKSR_P6_Pos)                    /**< (PIO_LOCKSR) Lock Status Mask */
#define PIO_LOCKSR_P6                       PIO_LOCKSR_P6_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_LOCKSR_P6_Msk instead */
#define PIO_LOCKSR_P7_Pos                   7                                              /**< (PIO_LOCKSR) Lock Status Position */
#define PIO_LOCKSR_P7_Msk                   (0x1U << PIO_LOCKSR_P7_Pos)                    /**< (PIO_LOCKSR) Lock Status Mask */
#define PIO_LOCKSR_P7                       PIO_LOCKSR_P7_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_LOCKSR_P7_Msk instead */
#define PIO_LOCKSR_P8_Pos                   8                                              /**< (PIO_LOCKSR) Lock Status Position */
#define PIO_LOCKSR_P8_Msk                   (0x1U << PIO_LOCKSR_P8_Pos)                    /**< (PIO_LOCKSR) Lock Status Mask */
#define PIO_LOCKSR_P8                       PIO_LOCKSR_P8_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_LOCKSR_P8_Msk instead */
#define PIO_LOCKSR_P9_Pos                   9                                              /**< (PIO_LOCKSR) Lock Status Position */
#define PIO_LOCKSR_P9_Msk                   (0x1U << PIO_LOCKSR_P9_Pos)                    /**< (PIO_LOCKSR) Lock Status Mask */
#define PIO_LOCKSR_P9                       PIO_LOCKSR_P9_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_LOCKSR_P9_Msk instead */
#define PIO_LOCKSR_P10_Pos                  10                                             /**< (PIO_LOCKSR) Lock Status Position */
#define PIO_LOCKSR_P10_Msk                  (0x1U << PIO_LOCKSR_P10_Pos)                   /**< (PIO_LOCKSR) Lock Status Mask */
#define PIO_LOCKSR_P10                      PIO_LOCKSR_P10_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_LOCKSR_P10_Msk instead */
#define PIO_LOCKSR_P11_Pos                  11                                             /**< (PIO_LOCKSR) Lock Status Position */
#define PIO_LOCKSR_P11_Msk                  (0x1U << PIO_LOCKSR_P11_Pos)                   /**< (PIO_LOCKSR) Lock Status Mask */
#define PIO_LOCKSR_P11                      PIO_LOCKSR_P11_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_LOCKSR_P11_Msk instead */
#define PIO_LOCKSR_P12_Pos                  12                                             /**< (PIO_LOCKSR) Lock Status Position */
#define PIO_LOCKSR_P12_Msk                  (0x1U << PIO_LOCKSR_P12_Pos)                   /**< (PIO_LOCKSR) Lock Status Mask */
#define PIO_LOCKSR_P12                      PIO_LOCKSR_P12_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_LOCKSR_P12_Msk instead */
#define PIO_LOCKSR_P13_Pos                  13                                             /**< (PIO_LOCKSR) Lock Status Position */
#define PIO_LOCKSR_P13_Msk                  (0x1U << PIO_LOCKSR_P13_Pos)                   /**< (PIO_LOCKSR) Lock Status Mask */
#define PIO_LOCKSR_P13                      PIO_LOCKSR_P13_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_LOCKSR_P13_Msk instead */
#define PIO_LOCKSR_P14_Pos                  14                                             /**< (PIO_LOCKSR) Lock Status Position */
#define PIO_LOCKSR_P14_Msk                  (0x1U << PIO_LOCKSR_P14_Pos)                   /**< (PIO_LOCKSR) Lock Status Mask */
#define PIO_LOCKSR_P14                      PIO_LOCKSR_P14_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_LOCKSR_P14_Msk instead */
#define PIO_LOCKSR_P15_Pos                  15                                             /**< (PIO_LOCKSR) Lock Status Position */
#define PIO_LOCKSR_P15_Msk                  (0x1U << PIO_LOCKSR_P15_Pos)                   /**< (PIO_LOCKSR) Lock Status Mask */
#define PIO_LOCKSR_P15                      PIO_LOCKSR_P15_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_LOCKSR_P15_Msk instead */
#define PIO_LOCKSR_P16_Pos                  16                                             /**< (PIO_LOCKSR) Lock Status Position */
#define PIO_LOCKSR_P16_Msk                  (0x1U << PIO_LOCKSR_P16_Pos)                   /**< (PIO_LOCKSR) Lock Status Mask */
#define PIO_LOCKSR_P16                      PIO_LOCKSR_P16_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_LOCKSR_P16_Msk instead */
#define PIO_LOCKSR_P17_Pos                  17                                             /**< (PIO_LOCKSR) Lock Status Position */
#define PIO_LOCKSR_P17_Msk                  (0x1U << PIO_LOCKSR_P17_Pos)                   /**< (PIO_LOCKSR) Lock Status Mask */
#define PIO_LOCKSR_P17                      PIO_LOCKSR_P17_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_LOCKSR_P17_Msk instead */
#define PIO_LOCKSR_P18_Pos                  18                                             /**< (PIO_LOCKSR) Lock Status Position */
#define PIO_LOCKSR_P18_Msk                  (0x1U << PIO_LOCKSR_P18_Pos)                   /**< (PIO_LOCKSR) Lock Status Mask */
#define PIO_LOCKSR_P18                      PIO_LOCKSR_P18_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_LOCKSR_P18_Msk instead */
#define PIO_LOCKSR_P19_Pos                  19                                             /**< (PIO_LOCKSR) Lock Status Position */
#define PIO_LOCKSR_P19_Msk                  (0x1U << PIO_LOCKSR_P19_Pos)                   /**< (PIO_LOCKSR) Lock Status Mask */
#define PIO_LOCKSR_P19                      PIO_LOCKSR_P19_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_LOCKSR_P19_Msk instead */
#define PIO_LOCKSR_P20_Pos                  20                                             /**< (PIO_LOCKSR) Lock Status Position */
#define PIO_LOCKSR_P20_Msk                  (0x1U << PIO_LOCKSR_P20_Pos)                   /**< (PIO_LOCKSR) Lock Status Mask */
#define PIO_LOCKSR_P20                      PIO_LOCKSR_P20_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_LOCKSR_P20_Msk instead */
#define PIO_LOCKSR_P21_Pos                  21                                             /**< (PIO_LOCKSR) Lock Status Position */
#define PIO_LOCKSR_P21_Msk                  (0x1U << PIO_LOCKSR_P21_Pos)                   /**< (PIO_LOCKSR) Lock Status Mask */
#define PIO_LOCKSR_P21                      PIO_LOCKSR_P21_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_LOCKSR_P21_Msk instead */
#define PIO_LOCKSR_P22_Pos                  22                                             /**< (PIO_LOCKSR) Lock Status Position */
#define PIO_LOCKSR_P22_Msk                  (0x1U << PIO_LOCKSR_P22_Pos)                   /**< (PIO_LOCKSR) Lock Status Mask */
#define PIO_LOCKSR_P22                      PIO_LOCKSR_P22_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_LOCKSR_P22_Msk instead */
#define PIO_LOCKSR_P23_Pos                  23                                             /**< (PIO_LOCKSR) Lock Status Position */
#define PIO_LOCKSR_P23_Msk                  (0x1U << PIO_LOCKSR_P23_Pos)                   /**< (PIO_LOCKSR) Lock Status Mask */
#define PIO_LOCKSR_P23                      PIO_LOCKSR_P23_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_LOCKSR_P23_Msk instead */
#define PIO_LOCKSR_P24_Pos                  24                                             /**< (PIO_LOCKSR) Lock Status Position */
#define PIO_LOCKSR_P24_Msk                  (0x1U << PIO_LOCKSR_P24_Pos)                   /**< (PIO_LOCKSR) Lock Status Mask */
#define PIO_LOCKSR_P24                      PIO_LOCKSR_P24_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_LOCKSR_P24_Msk instead */
#define PIO_LOCKSR_P25_Pos                  25                                             /**< (PIO_LOCKSR) Lock Status Position */
#define PIO_LOCKSR_P25_Msk                  (0x1U << PIO_LOCKSR_P25_Pos)                   /**< (PIO_LOCKSR) Lock Status Mask */
#define PIO_LOCKSR_P25                      PIO_LOCKSR_P25_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_LOCKSR_P25_Msk instead */
#define PIO_LOCKSR_P26_Pos                  26                                             /**< (PIO_LOCKSR) Lock Status Position */
#define PIO_LOCKSR_P26_Msk                  (0x1U << PIO_LOCKSR_P26_Pos)                   /**< (PIO_LOCKSR) Lock Status Mask */
#define PIO_LOCKSR_P26                      PIO_LOCKSR_P26_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_LOCKSR_P26_Msk instead */
#define PIO_LOCKSR_P27_Pos                  27                                             /**< (PIO_LOCKSR) Lock Status Position */
#define PIO_LOCKSR_P27_Msk                  (0x1U << PIO_LOCKSR_P27_Pos)                   /**< (PIO_LOCKSR) Lock Status Mask */
#define PIO_LOCKSR_P27                      PIO_LOCKSR_P27_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_LOCKSR_P27_Msk instead */
#define PIO_LOCKSR_P28_Pos                  28                                             /**< (PIO_LOCKSR) Lock Status Position */
#define PIO_LOCKSR_P28_Msk                  (0x1U << PIO_LOCKSR_P28_Pos)                   /**< (PIO_LOCKSR) Lock Status Mask */
#define PIO_LOCKSR_P28                      PIO_LOCKSR_P28_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_LOCKSR_P28_Msk instead */
#define PIO_LOCKSR_P29_Pos                  29                                             /**< (PIO_LOCKSR) Lock Status Position */
#define PIO_LOCKSR_P29_Msk                  (0x1U << PIO_LOCKSR_P29_Pos)                   /**< (PIO_LOCKSR) Lock Status Mask */
#define PIO_LOCKSR_P29                      PIO_LOCKSR_P29_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_LOCKSR_P29_Msk instead */
#define PIO_LOCKSR_P30_Pos                  30                                             /**< (PIO_LOCKSR) Lock Status Position */
#define PIO_LOCKSR_P30_Msk                  (0x1U << PIO_LOCKSR_P30_Pos)                   /**< (PIO_LOCKSR) Lock Status Mask */
#define PIO_LOCKSR_P30                      PIO_LOCKSR_P30_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_LOCKSR_P30_Msk instead */
#define PIO_LOCKSR_P31_Pos                  31                                             /**< (PIO_LOCKSR) Lock Status Position */
#define PIO_LOCKSR_P31_Msk                  (0x1U << PIO_LOCKSR_P31_Pos)                   /**< (PIO_LOCKSR) Lock Status Mask */
#define PIO_LOCKSR_P31                      PIO_LOCKSR_P31_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_LOCKSR_P31_Msk instead */
#define PIO_LOCKSR_P_Pos                    0                                              /**< (PIO_LOCKSR Position) Lock Status */
#define PIO_LOCKSR_P_Msk                    (0xFFFFFFFFU << PIO_LOCKSR_P_Pos)              /**< (PIO_LOCKSR Mask) P */
#define PIO_LOCKSR_P(value)                 (PIO_LOCKSR_P_Msk & ((value) << PIO_LOCKSR_P_Pos))  
#define PIO_LOCKSR_MASK                     (0xFFFFFFFFU)                                  /**< \deprecated (PIO_LOCKSR) Register MASK  (Use PIO_LOCKSR_Msk instead)  */
#define PIO_LOCKSR_Msk                      (0xFFFFFFFFU)                                  /**< (PIO_LOCKSR) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- PIO_WPMR : (PIO Offset: 0xe4) (R/W 32) Write Protection Mode Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t WPEN:1;                    /**< bit:      0  Write Protection Enable                  */
    uint32_t :7;                        /**< bit:   1..7  Reserved */
    uint32_t WPKEY:24;                  /**< bit:  8..31  Write Protection Key                     */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} PIO_WPMR_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define PIO_WPMR_OFFSET                     (0xE4)                                        /**<  (PIO_WPMR) Write Protection Mode Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define PIO_WPMR_WPEN_Pos                   0                                              /**< (PIO_WPMR) Write Protection Enable Position */
#define PIO_WPMR_WPEN_Msk                   (0x1U << PIO_WPMR_WPEN_Pos)                    /**< (PIO_WPMR) Write Protection Enable Mask */
#define PIO_WPMR_WPEN                       PIO_WPMR_WPEN_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_WPMR_WPEN_Msk instead */
#define PIO_WPMR_WPKEY_Pos                  8                                              /**< (PIO_WPMR) Write Protection Key Position */
#define PIO_WPMR_WPKEY_Msk                  (0xFFFFFFU << PIO_WPMR_WPKEY_Pos)              /**< (PIO_WPMR) Write Protection Key Mask */
#define PIO_WPMR_WPKEY(value)               (PIO_WPMR_WPKEY_Msk & ((value) << PIO_WPMR_WPKEY_Pos))
#define   PIO_WPMR_WPKEY_PASSWD_Val         (0x50494FU)                                    /**< (PIO_WPMR) Writing any other value in this field aborts the write operation of the WPEN bit. Always reads as 0.  */
#define PIO_WPMR_WPKEY_PASSWD               (PIO_WPMR_WPKEY_PASSWD_Val << PIO_WPMR_WPKEY_Pos)  /**< (PIO_WPMR) Writing any other value in this field aborts the write operation of the WPEN bit. Always reads as 0. Position  */
#define PIO_WPMR_MASK                       (0xFFFFFF01U)                                  /**< \deprecated (PIO_WPMR) Register MASK  (Use PIO_WPMR_Msk instead)  */
#define PIO_WPMR_Msk                        (0xFFFFFF01U)                                  /**< (PIO_WPMR) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- PIO_WPSR : (PIO Offset: 0xe8) (R/ 32) Write Protection Status Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t WPVS:1;                    /**< bit:      0  Write Protection Violation Status        */
    uint32_t :7;                        /**< bit:   1..7  Reserved */
    uint32_t WPVSRC:16;                 /**< bit:  8..23  Write Protection Violation Source        */
    uint32_t :8;                        /**< bit: 24..31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} PIO_WPSR_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define PIO_WPSR_OFFSET                     (0xE8)                                        /**<  (PIO_WPSR) Write Protection Status Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define PIO_WPSR_WPVS_Pos                   0                                              /**< (PIO_WPSR) Write Protection Violation Status Position */
#define PIO_WPSR_WPVS_Msk                   (0x1U << PIO_WPSR_WPVS_Pos)                    /**< (PIO_WPSR) Write Protection Violation Status Mask */
#define PIO_WPSR_WPVS                       PIO_WPSR_WPVS_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_WPSR_WPVS_Msk instead */
#define PIO_WPSR_WPVSRC_Pos                 8                                              /**< (PIO_WPSR) Write Protection Violation Source Position */
#define PIO_WPSR_WPVSRC_Msk                 (0xFFFFU << PIO_WPSR_WPVSRC_Pos)               /**< (PIO_WPSR) Write Protection Violation Source Mask */
#define PIO_WPSR_WPVSRC(value)              (PIO_WPSR_WPVSRC_Msk & ((value) << PIO_WPSR_WPVSRC_Pos))
#define PIO_WPSR_MASK                       (0xFFFF01U)                                    /**< \deprecated (PIO_WPSR) Register MASK  (Use PIO_WPSR_Msk instead)  */
#define PIO_WPSR_Msk                        (0xFFFF01U)                                    /**< (PIO_WPSR) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- PIO_SCHMITT : (PIO Offset: 0x100) (R/W 32) Schmitt Trigger Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t SCHMITT0:1;                /**< bit:      0  Schmitt Trigger Control                  */
    uint32_t SCHMITT1:1;                /**< bit:      1  Schmitt Trigger Control                  */
    uint32_t SCHMITT2:1;                /**< bit:      2  Schmitt Trigger Control                  */
    uint32_t SCHMITT3:1;                /**< bit:      3  Schmitt Trigger Control                  */
    uint32_t SCHMITT4:1;                /**< bit:      4  Schmitt Trigger Control                  */
    uint32_t SCHMITT5:1;                /**< bit:      5  Schmitt Trigger Control                  */
    uint32_t SCHMITT6:1;                /**< bit:      6  Schmitt Trigger Control                  */
    uint32_t SCHMITT7:1;                /**< bit:      7  Schmitt Trigger Control                  */
    uint32_t SCHMITT8:1;                /**< bit:      8  Schmitt Trigger Control                  */
    uint32_t SCHMITT9:1;                /**< bit:      9  Schmitt Trigger Control                  */
    uint32_t SCHMITT10:1;               /**< bit:     10  Schmitt Trigger Control                  */
    uint32_t SCHMITT11:1;               /**< bit:     11  Schmitt Trigger Control                  */
    uint32_t SCHMITT12:1;               /**< bit:     12  Schmitt Trigger Control                  */
    uint32_t SCHMITT13:1;               /**< bit:     13  Schmitt Trigger Control                  */
    uint32_t SCHMITT14:1;               /**< bit:     14  Schmitt Trigger Control                  */
    uint32_t SCHMITT15:1;               /**< bit:     15  Schmitt Trigger Control                  */
    uint32_t SCHMITT16:1;               /**< bit:     16  Schmitt Trigger Control                  */
    uint32_t SCHMITT17:1;               /**< bit:     17  Schmitt Trigger Control                  */
    uint32_t SCHMITT18:1;               /**< bit:     18  Schmitt Trigger Control                  */
    uint32_t SCHMITT19:1;               /**< bit:     19  Schmitt Trigger Control                  */
    uint32_t SCHMITT20:1;               /**< bit:     20  Schmitt Trigger Control                  */
    uint32_t SCHMITT21:1;               /**< bit:     21  Schmitt Trigger Control                  */
    uint32_t SCHMITT22:1;               /**< bit:     22  Schmitt Trigger Control                  */
    uint32_t SCHMITT23:1;               /**< bit:     23  Schmitt Trigger Control                  */
    uint32_t SCHMITT24:1;               /**< bit:     24  Schmitt Trigger Control                  */
    uint32_t SCHMITT25:1;               /**< bit:     25  Schmitt Trigger Control                  */
    uint32_t SCHMITT26:1;               /**< bit:     26  Schmitt Trigger Control                  */
    uint32_t SCHMITT27:1;               /**< bit:     27  Schmitt Trigger Control                  */
    uint32_t SCHMITT28:1;               /**< bit:     28  Schmitt Trigger Control                  */
    uint32_t SCHMITT29:1;               /**< bit:     29  Schmitt Trigger Control                  */
    uint32_t SCHMITT30:1;               /**< bit:     30  Schmitt Trigger Control                  */
    uint32_t SCHMITT31:1;               /**< bit:     31  Schmitt Trigger Control                  */
  } bit;                                /**< Structure used for bit  access */
  struct {
    uint32_t SCHMITT:32;                /**< bit:  0..31  Schmitt Trigger Control                  */
  } vec;                                /**< Structure used for vec  access  */
  uint32_t reg;                         /**< Type used for register access */
} PIO_SCHMITT_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define PIO_SCHMITT_OFFSET                  (0x100)                                       /**<  (PIO_SCHMITT) Schmitt Trigger Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define PIO_SCHMITT_SCHMITT0_Pos            0                                              /**< (PIO_SCHMITT) Schmitt Trigger Control Position */
#define PIO_SCHMITT_SCHMITT0_Msk            (0x1U << PIO_SCHMITT_SCHMITT0_Pos)             /**< (PIO_SCHMITT) Schmitt Trigger Control Mask */
#define PIO_SCHMITT_SCHMITT0                PIO_SCHMITT_SCHMITT0_Msk                       /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_SCHMITT_SCHMITT0_Msk instead */
#define PIO_SCHMITT_SCHMITT1_Pos            1                                              /**< (PIO_SCHMITT) Schmitt Trigger Control Position */
#define PIO_SCHMITT_SCHMITT1_Msk            (0x1U << PIO_SCHMITT_SCHMITT1_Pos)             /**< (PIO_SCHMITT) Schmitt Trigger Control Mask */
#define PIO_SCHMITT_SCHMITT1                PIO_SCHMITT_SCHMITT1_Msk                       /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_SCHMITT_SCHMITT1_Msk instead */
#define PIO_SCHMITT_SCHMITT2_Pos            2                                              /**< (PIO_SCHMITT) Schmitt Trigger Control Position */
#define PIO_SCHMITT_SCHMITT2_Msk            (0x1U << PIO_SCHMITT_SCHMITT2_Pos)             /**< (PIO_SCHMITT) Schmitt Trigger Control Mask */
#define PIO_SCHMITT_SCHMITT2                PIO_SCHMITT_SCHMITT2_Msk                       /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_SCHMITT_SCHMITT2_Msk instead */
#define PIO_SCHMITT_SCHMITT3_Pos            3                                              /**< (PIO_SCHMITT) Schmitt Trigger Control Position */
#define PIO_SCHMITT_SCHMITT3_Msk            (0x1U << PIO_SCHMITT_SCHMITT3_Pos)             /**< (PIO_SCHMITT) Schmitt Trigger Control Mask */
#define PIO_SCHMITT_SCHMITT3                PIO_SCHMITT_SCHMITT3_Msk                       /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_SCHMITT_SCHMITT3_Msk instead */
#define PIO_SCHMITT_SCHMITT4_Pos            4                                              /**< (PIO_SCHMITT) Schmitt Trigger Control Position */
#define PIO_SCHMITT_SCHMITT4_Msk            (0x1U << PIO_SCHMITT_SCHMITT4_Pos)             /**< (PIO_SCHMITT) Schmitt Trigger Control Mask */
#define PIO_SCHMITT_SCHMITT4                PIO_SCHMITT_SCHMITT4_Msk                       /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_SCHMITT_SCHMITT4_Msk instead */
#define PIO_SCHMITT_SCHMITT5_Pos            5                                              /**< (PIO_SCHMITT) Schmitt Trigger Control Position */
#define PIO_SCHMITT_SCHMITT5_Msk            (0x1U << PIO_SCHMITT_SCHMITT5_Pos)             /**< (PIO_SCHMITT) Schmitt Trigger Control Mask */
#define PIO_SCHMITT_SCHMITT5                PIO_SCHMITT_SCHMITT5_Msk                       /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_SCHMITT_SCHMITT5_Msk instead */
#define PIO_SCHMITT_SCHMITT6_Pos            6                                              /**< (PIO_SCHMITT) Schmitt Trigger Control Position */
#define PIO_SCHMITT_SCHMITT6_Msk            (0x1U << PIO_SCHMITT_SCHMITT6_Pos)             /**< (PIO_SCHMITT) Schmitt Trigger Control Mask */
#define PIO_SCHMITT_SCHMITT6                PIO_SCHMITT_SCHMITT6_Msk                       /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_SCHMITT_SCHMITT6_Msk instead */
#define PIO_SCHMITT_SCHMITT7_Pos            7                                              /**< (PIO_SCHMITT) Schmitt Trigger Control Position */
#define PIO_SCHMITT_SCHMITT7_Msk            (0x1U << PIO_SCHMITT_SCHMITT7_Pos)             /**< (PIO_SCHMITT) Schmitt Trigger Control Mask */
#define PIO_SCHMITT_SCHMITT7                PIO_SCHMITT_SCHMITT7_Msk                       /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_SCHMITT_SCHMITT7_Msk instead */
#define PIO_SCHMITT_SCHMITT8_Pos            8                                              /**< (PIO_SCHMITT) Schmitt Trigger Control Position */
#define PIO_SCHMITT_SCHMITT8_Msk            (0x1U << PIO_SCHMITT_SCHMITT8_Pos)             /**< (PIO_SCHMITT) Schmitt Trigger Control Mask */
#define PIO_SCHMITT_SCHMITT8                PIO_SCHMITT_SCHMITT8_Msk                       /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_SCHMITT_SCHMITT8_Msk instead */
#define PIO_SCHMITT_SCHMITT9_Pos            9                                              /**< (PIO_SCHMITT) Schmitt Trigger Control Position */
#define PIO_SCHMITT_SCHMITT9_Msk            (0x1U << PIO_SCHMITT_SCHMITT9_Pos)             /**< (PIO_SCHMITT) Schmitt Trigger Control Mask */
#define PIO_SCHMITT_SCHMITT9                PIO_SCHMITT_SCHMITT9_Msk                       /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_SCHMITT_SCHMITT9_Msk instead */
#define PIO_SCHMITT_SCHMITT10_Pos           10                                             /**< (PIO_SCHMITT) Schmitt Trigger Control Position */
#define PIO_SCHMITT_SCHMITT10_Msk           (0x1U << PIO_SCHMITT_SCHMITT10_Pos)            /**< (PIO_SCHMITT) Schmitt Trigger Control Mask */
#define PIO_SCHMITT_SCHMITT10               PIO_SCHMITT_SCHMITT10_Msk                      /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_SCHMITT_SCHMITT10_Msk instead */
#define PIO_SCHMITT_SCHMITT11_Pos           11                                             /**< (PIO_SCHMITT) Schmitt Trigger Control Position */
#define PIO_SCHMITT_SCHMITT11_Msk           (0x1U << PIO_SCHMITT_SCHMITT11_Pos)            /**< (PIO_SCHMITT) Schmitt Trigger Control Mask */
#define PIO_SCHMITT_SCHMITT11               PIO_SCHMITT_SCHMITT11_Msk                      /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_SCHMITT_SCHMITT11_Msk instead */
#define PIO_SCHMITT_SCHMITT12_Pos           12                                             /**< (PIO_SCHMITT) Schmitt Trigger Control Position */
#define PIO_SCHMITT_SCHMITT12_Msk           (0x1U << PIO_SCHMITT_SCHMITT12_Pos)            /**< (PIO_SCHMITT) Schmitt Trigger Control Mask */
#define PIO_SCHMITT_SCHMITT12               PIO_SCHMITT_SCHMITT12_Msk                      /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_SCHMITT_SCHMITT12_Msk instead */
#define PIO_SCHMITT_SCHMITT13_Pos           13                                             /**< (PIO_SCHMITT) Schmitt Trigger Control Position */
#define PIO_SCHMITT_SCHMITT13_Msk           (0x1U << PIO_SCHMITT_SCHMITT13_Pos)            /**< (PIO_SCHMITT) Schmitt Trigger Control Mask */
#define PIO_SCHMITT_SCHMITT13               PIO_SCHMITT_SCHMITT13_Msk                      /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_SCHMITT_SCHMITT13_Msk instead */
#define PIO_SCHMITT_SCHMITT14_Pos           14                                             /**< (PIO_SCHMITT) Schmitt Trigger Control Position */
#define PIO_SCHMITT_SCHMITT14_Msk           (0x1U << PIO_SCHMITT_SCHMITT14_Pos)            /**< (PIO_SCHMITT) Schmitt Trigger Control Mask */
#define PIO_SCHMITT_SCHMITT14               PIO_SCHMITT_SCHMITT14_Msk                      /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_SCHMITT_SCHMITT14_Msk instead */
#define PIO_SCHMITT_SCHMITT15_Pos           15                                             /**< (PIO_SCHMITT) Schmitt Trigger Control Position */
#define PIO_SCHMITT_SCHMITT15_Msk           (0x1U << PIO_SCHMITT_SCHMITT15_Pos)            /**< (PIO_SCHMITT) Schmitt Trigger Control Mask */
#define PIO_SCHMITT_SCHMITT15               PIO_SCHMITT_SCHMITT15_Msk                      /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_SCHMITT_SCHMITT15_Msk instead */
#define PIO_SCHMITT_SCHMITT16_Pos           16                                             /**< (PIO_SCHMITT) Schmitt Trigger Control Position */
#define PIO_SCHMITT_SCHMITT16_Msk           (0x1U << PIO_SCHMITT_SCHMITT16_Pos)            /**< (PIO_SCHMITT) Schmitt Trigger Control Mask */
#define PIO_SCHMITT_SCHMITT16               PIO_SCHMITT_SCHMITT16_Msk                      /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_SCHMITT_SCHMITT16_Msk instead */
#define PIO_SCHMITT_SCHMITT17_Pos           17                                             /**< (PIO_SCHMITT) Schmitt Trigger Control Position */
#define PIO_SCHMITT_SCHMITT17_Msk           (0x1U << PIO_SCHMITT_SCHMITT17_Pos)            /**< (PIO_SCHMITT) Schmitt Trigger Control Mask */
#define PIO_SCHMITT_SCHMITT17               PIO_SCHMITT_SCHMITT17_Msk                      /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_SCHMITT_SCHMITT17_Msk instead */
#define PIO_SCHMITT_SCHMITT18_Pos           18                                             /**< (PIO_SCHMITT) Schmitt Trigger Control Position */
#define PIO_SCHMITT_SCHMITT18_Msk           (0x1U << PIO_SCHMITT_SCHMITT18_Pos)            /**< (PIO_SCHMITT) Schmitt Trigger Control Mask */
#define PIO_SCHMITT_SCHMITT18               PIO_SCHMITT_SCHMITT18_Msk                      /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_SCHMITT_SCHMITT18_Msk instead */
#define PIO_SCHMITT_SCHMITT19_Pos           19                                             /**< (PIO_SCHMITT) Schmitt Trigger Control Position */
#define PIO_SCHMITT_SCHMITT19_Msk           (0x1U << PIO_SCHMITT_SCHMITT19_Pos)            /**< (PIO_SCHMITT) Schmitt Trigger Control Mask */
#define PIO_SCHMITT_SCHMITT19               PIO_SCHMITT_SCHMITT19_Msk                      /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_SCHMITT_SCHMITT19_Msk instead */
#define PIO_SCHMITT_SCHMITT20_Pos           20                                             /**< (PIO_SCHMITT) Schmitt Trigger Control Position */
#define PIO_SCHMITT_SCHMITT20_Msk           (0x1U << PIO_SCHMITT_SCHMITT20_Pos)            /**< (PIO_SCHMITT) Schmitt Trigger Control Mask */
#define PIO_SCHMITT_SCHMITT20               PIO_SCHMITT_SCHMITT20_Msk                      /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_SCHMITT_SCHMITT20_Msk instead */
#define PIO_SCHMITT_SCHMITT21_Pos           21                                             /**< (PIO_SCHMITT) Schmitt Trigger Control Position */
#define PIO_SCHMITT_SCHMITT21_Msk           (0x1U << PIO_SCHMITT_SCHMITT21_Pos)            /**< (PIO_SCHMITT) Schmitt Trigger Control Mask */
#define PIO_SCHMITT_SCHMITT21               PIO_SCHMITT_SCHMITT21_Msk                      /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_SCHMITT_SCHMITT21_Msk instead */
#define PIO_SCHMITT_SCHMITT22_Pos           22                                             /**< (PIO_SCHMITT) Schmitt Trigger Control Position */
#define PIO_SCHMITT_SCHMITT22_Msk           (0x1U << PIO_SCHMITT_SCHMITT22_Pos)            /**< (PIO_SCHMITT) Schmitt Trigger Control Mask */
#define PIO_SCHMITT_SCHMITT22               PIO_SCHMITT_SCHMITT22_Msk                      /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_SCHMITT_SCHMITT22_Msk instead */
#define PIO_SCHMITT_SCHMITT23_Pos           23                                             /**< (PIO_SCHMITT) Schmitt Trigger Control Position */
#define PIO_SCHMITT_SCHMITT23_Msk           (0x1U << PIO_SCHMITT_SCHMITT23_Pos)            /**< (PIO_SCHMITT) Schmitt Trigger Control Mask */
#define PIO_SCHMITT_SCHMITT23               PIO_SCHMITT_SCHMITT23_Msk                      /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_SCHMITT_SCHMITT23_Msk instead */
#define PIO_SCHMITT_SCHMITT24_Pos           24                                             /**< (PIO_SCHMITT) Schmitt Trigger Control Position */
#define PIO_SCHMITT_SCHMITT24_Msk           (0x1U << PIO_SCHMITT_SCHMITT24_Pos)            /**< (PIO_SCHMITT) Schmitt Trigger Control Mask */
#define PIO_SCHMITT_SCHMITT24               PIO_SCHMITT_SCHMITT24_Msk                      /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_SCHMITT_SCHMITT24_Msk instead */
#define PIO_SCHMITT_SCHMITT25_Pos           25                                             /**< (PIO_SCHMITT) Schmitt Trigger Control Position */
#define PIO_SCHMITT_SCHMITT25_Msk           (0x1U << PIO_SCHMITT_SCHMITT25_Pos)            /**< (PIO_SCHMITT) Schmitt Trigger Control Mask */
#define PIO_SCHMITT_SCHMITT25               PIO_SCHMITT_SCHMITT25_Msk                      /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_SCHMITT_SCHMITT25_Msk instead */
#define PIO_SCHMITT_SCHMITT26_Pos           26                                             /**< (PIO_SCHMITT) Schmitt Trigger Control Position */
#define PIO_SCHMITT_SCHMITT26_Msk           (0x1U << PIO_SCHMITT_SCHMITT26_Pos)            /**< (PIO_SCHMITT) Schmitt Trigger Control Mask */
#define PIO_SCHMITT_SCHMITT26               PIO_SCHMITT_SCHMITT26_Msk                      /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_SCHMITT_SCHMITT26_Msk instead */
#define PIO_SCHMITT_SCHMITT27_Pos           27                                             /**< (PIO_SCHMITT) Schmitt Trigger Control Position */
#define PIO_SCHMITT_SCHMITT27_Msk           (0x1U << PIO_SCHMITT_SCHMITT27_Pos)            /**< (PIO_SCHMITT) Schmitt Trigger Control Mask */
#define PIO_SCHMITT_SCHMITT27               PIO_SCHMITT_SCHMITT27_Msk                      /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_SCHMITT_SCHMITT27_Msk instead */
#define PIO_SCHMITT_SCHMITT28_Pos           28                                             /**< (PIO_SCHMITT) Schmitt Trigger Control Position */
#define PIO_SCHMITT_SCHMITT28_Msk           (0x1U << PIO_SCHMITT_SCHMITT28_Pos)            /**< (PIO_SCHMITT) Schmitt Trigger Control Mask */
#define PIO_SCHMITT_SCHMITT28               PIO_SCHMITT_SCHMITT28_Msk                      /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_SCHMITT_SCHMITT28_Msk instead */
#define PIO_SCHMITT_SCHMITT29_Pos           29                                             /**< (PIO_SCHMITT) Schmitt Trigger Control Position */
#define PIO_SCHMITT_SCHMITT29_Msk           (0x1U << PIO_SCHMITT_SCHMITT29_Pos)            /**< (PIO_SCHMITT) Schmitt Trigger Control Mask */
#define PIO_SCHMITT_SCHMITT29               PIO_SCHMITT_SCHMITT29_Msk                      /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_SCHMITT_SCHMITT29_Msk instead */
#define PIO_SCHMITT_SCHMITT30_Pos           30                                             /**< (PIO_SCHMITT) Schmitt Trigger Control Position */
#define PIO_SCHMITT_SCHMITT30_Msk           (0x1U << PIO_SCHMITT_SCHMITT30_Pos)            /**< (PIO_SCHMITT) Schmitt Trigger Control Mask */
#define PIO_SCHMITT_SCHMITT30               PIO_SCHMITT_SCHMITT30_Msk                      /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_SCHMITT_SCHMITT30_Msk instead */
#define PIO_SCHMITT_SCHMITT31_Pos           31                                             /**< (PIO_SCHMITT) Schmitt Trigger Control Position */
#define PIO_SCHMITT_SCHMITT31_Msk           (0x1U << PIO_SCHMITT_SCHMITT31_Pos)            /**< (PIO_SCHMITT) Schmitt Trigger Control Mask */
#define PIO_SCHMITT_SCHMITT31               PIO_SCHMITT_SCHMITT31_Msk                      /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_SCHMITT_SCHMITT31_Msk instead */
#define PIO_SCHMITT_SCHMITT_Pos             0                                              /**< (PIO_SCHMITT Position) Schmitt Trigger Control */
#define PIO_SCHMITT_SCHMITT_Msk             (0xFFFFFFFFU << PIO_SCHMITT_SCHMITT_Pos)       /**< (PIO_SCHMITT Mask) SCHMITT */
#define PIO_SCHMITT_SCHMITT(value)          (PIO_SCHMITT_SCHMITT_Msk & ((value) << PIO_SCHMITT_SCHMITT_Pos))  
#define PIO_SCHMITT_MASK                    (0xFFFFFFFFU)                                  /**< \deprecated (PIO_SCHMITT) Register MASK  (Use PIO_SCHMITT_Msk instead)  */
#define PIO_SCHMITT_Msk                     (0xFFFFFFFFU)                                  /**< (PIO_SCHMITT) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- PIO_DRIVER : (PIO Offset: 0x118) (R/W 32) I/O Drive Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t LINE0:1;                   /**< bit:      0  Drive of PIO Line 0                      */
    uint32_t LINE1:1;                   /**< bit:      1  Drive of PIO Line 1                      */
    uint32_t LINE2:1;                   /**< bit:      2  Drive of PIO Line 2                      */
    uint32_t LINE3:1;                   /**< bit:      3  Drive of PIO Line 3                      */
    uint32_t LINE4:1;                   /**< bit:      4  Drive of PIO Line 4                      */
    uint32_t LINE5:1;                   /**< bit:      5  Drive of PIO Line 5                      */
    uint32_t LINE6:1;                   /**< bit:      6  Drive of PIO Line 6                      */
    uint32_t LINE7:1;                   /**< bit:      7  Drive of PIO Line 7                      */
    uint32_t LINE8:1;                   /**< bit:      8  Drive of PIO Line 8                      */
    uint32_t LINE9:1;                   /**< bit:      9  Drive of PIO Line 9                      */
    uint32_t LINE10:1;                  /**< bit:     10  Drive of PIO Line 10                     */
    uint32_t LINE11:1;                  /**< bit:     11  Drive of PIO Line 11                     */
    uint32_t LINE12:1;                  /**< bit:     12  Drive of PIO Line 12                     */
    uint32_t LINE13:1;                  /**< bit:     13  Drive of PIO Line 13                     */
    uint32_t LINE14:1;                  /**< bit:     14  Drive of PIO Line 14                     */
    uint32_t LINE15:1;                  /**< bit:     15  Drive of PIO Line 15                     */
    uint32_t LINE16:1;                  /**< bit:     16  Drive of PIO Line 16                     */
    uint32_t LINE17:1;                  /**< bit:     17  Drive of PIO Line 17                     */
    uint32_t LINE18:1;                  /**< bit:     18  Drive of PIO Line 18                     */
    uint32_t LINE19:1;                  /**< bit:     19  Drive of PIO Line 19                     */
    uint32_t LINE20:1;                  /**< bit:     20  Drive of PIO Line 20                     */
    uint32_t LINE21:1;                  /**< bit:     21  Drive of PIO Line 21                     */
    uint32_t LINE22:1;                  /**< bit:     22  Drive of PIO Line 22                     */
    uint32_t LINE23:1;                  /**< bit:     23  Drive of PIO Line 23                     */
    uint32_t LINE24:1;                  /**< bit:     24  Drive of PIO Line 24                     */
    uint32_t LINE25:1;                  /**< bit:     25  Drive of PIO Line 25                     */
    uint32_t LINE26:1;                  /**< bit:     26  Drive of PIO Line 26                     */
    uint32_t LINE27:1;                  /**< bit:     27  Drive of PIO Line 27                     */
    uint32_t LINE28:1;                  /**< bit:     28  Drive of PIO Line 28                     */
    uint32_t LINE29:1;                  /**< bit:     29  Drive of PIO Line 29                     */
    uint32_t LINE30:1;                  /**< bit:     30  Drive of PIO Line 30                     */
    uint32_t LINE31:1;                  /**< bit:     31  Drive of PIO Line 31                     */
  } bit;                                /**< Structure used for bit  access */
  struct {
    uint32_t LINE:32;                   /**< bit:  0..31  Drive of PIO Line 3x                     */
  } vec;                                /**< Structure used for vec  access  */
  uint32_t reg;                         /**< Type used for register access */
} PIO_DRIVER_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define PIO_DRIVER_OFFSET                   (0x118)                                       /**<  (PIO_DRIVER) I/O Drive Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define PIO_DRIVER_LINE0_Pos                0                                              /**< (PIO_DRIVER) Drive of PIO Line 0 Position */
#define PIO_DRIVER_LINE0_Msk                (0x1U << PIO_DRIVER_LINE0_Pos)                 /**< (PIO_DRIVER) Drive of PIO Line 0 Mask */
#define PIO_DRIVER_LINE0                    PIO_DRIVER_LINE0_Msk                           /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_DRIVER_LINE0_Msk instead */
#define   PIO_DRIVER_LINE0_LOW_DRIVE_Val    (0x0U)                                         /**< (PIO_DRIVER) Lowest drive  */
#define   PIO_DRIVER_LINE0_HIGH_DRIVE_Val   (0x1U)                                         /**< (PIO_DRIVER) Highest drive  */
#define PIO_DRIVER_LINE0_LOW_DRIVE          (PIO_DRIVER_LINE0_LOW_DRIVE_Val << PIO_DRIVER_LINE0_Pos)  /**< (PIO_DRIVER) Lowest drive Position  */
#define PIO_DRIVER_LINE0_HIGH_DRIVE         (PIO_DRIVER_LINE0_HIGH_DRIVE_Val << PIO_DRIVER_LINE0_Pos)  /**< (PIO_DRIVER) Highest drive Position  */
#define PIO_DRIVER_LINE1_Pos                1                                              /**< (PIO_DRIVER) Drive of PIO Line 1 Position */
#define PIO_DRIVER_LINE1_Msk                (0x1U << PIO_DRIVER_LINE1_Pos)                 /**< (PIO_DRIVER) Drive of PIO Line 1 Mask */
#define PIO_DRIVER_LINE1                    PIO_DRIVER_LINE1_Msk                           /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_DRIVER_LINE1_Msk instead */
#define   PIO_DRIVER_LINE1_LOW_DRIVE_Val    (0x0U)                                         /**< (PIO_DRIVER) Lowest drive  */
#define   PIO_DRIVER_LINE1_HIGH_DRIVE_Val   (0x1U)                                         /**< (PIO_DRIVER) Highest drive  */
#define PIO_DRIVER_LINE1_LOW_DRIVE          (PIO_DRIVER_LINE1_LOW_DRIVE_Val << PIO_DRIVER_LINE1_Pos)  /**< (PIO_DRIVER) Lowest drive Position  */
#define PIO_DRIVER_LINE1_HIGH_DRIVE         (PIO_DRIVER_LINE1_HIGH_DRIVE_Val << PIO_DRIVER_LINE1_Pos)  /**< (PIO_DRIVER) Highest drive Position  */
#define PIO_DRIVER_LINE2_Pos                2                                              /**< (PIO_DRIVER) Drive of PIO Line 2 Position */
#define PIO_DRIVER_LINE2_Msk                (0x1U << PIO_DRIVER_LINE2_Pos)                 /**< (PIO_DRIVER) Drive of PIO Line 2 Mask */
#define PIO_DRIVER_LINE2                    PIO_DRIVER_LINE2_Msk                           /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_DRIVER_LINE2_Msk instead */
#define   PIO_DRIVER_LINE2_LOW_DRIVE_Val    (0x0U)                                         /**< (PIO_DRIVER) Lowest drive  */
#define   PIO_DRIVER_LINE2_HIGH_DRIVE_Val   (0x1U)                                         /**< (PIO_DRIVER) Highest drive  */
#define PIO_DRIVER_LINE2_LOW_DRIVE          (PIO_DRIVER_LINE2_LOW_DRIVE_Val << PIO_DRIVER_LINE2_Pos)  /**< (PIO_DRIVER) Lowest drive Position  */
#define PIO_DRIVER_LINE2_HIGH_DRIVE         (PIO_DRIVER_LINE2_HIGH_DRIVE_Val << PIO_DRIVER_LINE2_Pos)  /**< (PIO_DRIVER) Highest drive Position  */
#define PIO_DRIVER_LINE3_Pos                3                                              /**< (PIO_DRIVER) Drive of PIO Line 3 Position */
#define PIO_DRIVER_LINE3_Msk                (0x1U << PIO_DRIVER_LINE3_Pos)                 /**< (PIO_DRIVER) Drive of PIO Line 3 Mask */
#define PIO_DRIVER_LINE3                    PIO_DRIVER_LINE3_Msk                           /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_DRIVER_LINE3_Msk instead */
#define   PIO_DRIVER_LINE3_LOW_DRIVE_Val    (0x0U)                                         /**< (PIO_DRIVER) Lowest drive  */
#define   PIO_DRIVER_LINE3_HIGH_DRIVE_Val   (0x1U)                                         /**< (PIO_DRIVER) Highest drive  */
#define PIO_DRIVER_LINE3_LOW_DRIVE          (PIO_DRIVER_LINE3_LOW_DRIVE_Val << PIO_DRIVER_LINE3_Pos)  /**< (PIO_DRIVER) Lowest drive Position  */
#define PIO_DRIVER_LINE3_HIGH_DRIVE         (PIO_DRIVER_LINE3_HIGH_DRIVE_Val << PIO_DRIVER_LINE3_Pos)  /**< (PIO_DRIVER) Highest drive Position  */
#define PIO_DRIVER_LINE4_Pos                4                                              /**< (PIO_DRIVER) Drive of PIO Line 4 Position */
#define PIO_DRIVER_LINE4_Msk                (0x1U << PIO_DRIVER_LINE4_Pos)                 /**< (PIO_DRIVER) Drive of PIO Line 4 Mask */
#define PIO_DRIVER_LINE4                    PIO_DRIVER_LINE4_Msk                           /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_DRIVER_LINE4_Msk instead */
#define   PIO_DRIVER_LINE4_LOW_DRIVE_Val    (0x0U)                                         /**< (PIO_DRIVER) Lowest drive  */
#define   PIO_DRIVER_LINE4_HIGH_DRIVE_Val   (0x1U)                                         /**< (PIO_DRIVER) Highest drive  */
#define PIO_DRIVER_LINE4_LOW_DRIVE          (PIO_DRIVER_LINE4_LOW_DRIVE_Val << PIO_DRIVER_LINE4_Pos)  /**< (PIO_DRIVER) Lowest drive Position  */
#define PIO_DRIVER_LINE4_HIGH_DRIVE         (PIO_DRIVER_LINE4_HIGH_DRIVE_Val << PIO_DRIVER_LINE4_Pos)  /**< (PIO_DRIVER) Highest drive Position  */
#define PIO_DRIVER_LINE5_Pos                5                                              /**< (PIO_DRIVER) Drive of PIO Line 5 Position */
#define PIO_DRIVER_LINE5_Msk                (0x1U << PIO_DRIVER_LINE5_Pos)                 /**< (PIO_DRIVER) Drive of PIO Line 5 Mask */
#define PIO_DRIVER_LINE5                    PIO_DRIVER_LINE5_Msk                           /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_DRIVER_LINE5_Msk instead */
#define   PIO_DRIVER_LINE5_LOW_DRIVE_Val    (0x0U)                                         /**< (PIO_DRIVER) Lowest drive  */
#define   PIO_DRIVER_LINE5_HIGH_DRIVE_Val   (0x1U)                                         /**< (PIO_DRIVER) Highest drive  */
#define PIO_DRIVER_LINE5_LOW_DRIVE          (PIO_DRIVER_LINE5_LOW_DRIVE_Val << PIO_DRIVER_LINE5_Pos)  /**< (PIO_DRIVER) Lowest drive Position  */
#define PIO_DRIVER_LINE5_HIGH_DRIVE         (PIO_DRIVER_LINE5_HIGH_DRIVE_Val << PIO_DRIVER_LINE5_Pos)  /**< (PIO_DRIVER) Highest drive Position  */
#define PIO_DRIVER_LINE6_Pos                6                                              /**< (PIO_DRIVER) Drive of PIO Line 6 Position */
#define PIO_DRIVER_LINE6_Msk                (0x1U << PIO_DRIVER_LINE6_Pos)                 /**< (PIO_DRIVER) Drive of PIO Line 6 Mask */
#define PIO_DRIVER_LINE6                    PIO_DRIVER_LINE6_Msk                           /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_DRIVER_LINE6_Msk instead */
#define   PIO_DRIVER_LINE6_LOW_DRIVE_Val    (0x0U)                                         /**< (PIO_DRIVER) Lowest drive  */
#define   PIO_DRIVER_LINE6_HIGH_DRIVE_Val   (0x1U)                                         /**< (PIO_DRIVER) Highest drive  */
#define PIO_DRIVER_LINE6_LOW_DRIVE          (PIO_DRIVER_LINE6_LOW_DRIVE_Val << PIO_DRIVER_LINE6_Pos)  /**< (PIO_DRIVER) Lowest drive Position  */
#define PIO_DRIVER_LINE6_HIGH_DRIVE         (PIO_DRIVER_LINE6_HIGH_DRIVE_Val << PIO_DRIVER_LINE6_Pos)  /**< (PIO_DRIVER) Highest drive Position  */
#define PIO_DRIVER_LINE7_Pos                7                                              /**< (PIO_DRIVER) Drive of PIO Line 7 Position */
#define PIO_DRIVER_LINE7_Msk                (0x1U << PIO_DRIVER_LINE7_Pos)                 /**< (PIO_DRIVER) Drive of PIO Line 7 Mask */
#define PIO_DRIVER_LINE7                    PIO_DRIVER_LINE7_Msk                           /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_DRIVER_LINE7_Msk instead */
#define   PIO_DRIVER_LINE7_LOW_DRIVE_Val    (0x0U)                                         /**< (PIO_DRIVER) Lowest drive  */
#define   PIO_DRIVER_LINE7_HIGH_DRIVE_Val   (0x1U)                                         /**< (PIO_DRIVER) Highest drive  */
#define PIO_DRIVER_LINE7_LOW_DRIVE          (PIO_DRIVER_LINE7_LOW_DRIVE_Val << PIO_DRIVER_LINE7_Pos)  /**< (PIO_DRIVER) Lowest drive Position  */
#define PIO_DRIVER_LINE7_HIGH_DRIVE         (PIO_DRIVER_LINE7_HIGH_DRIVE_Val << PIO_DRIVER_LINE7_Pos)  /**< (PIO_DRIVER) Highest drive Position  */
#define PIO_DRIVER_LINE8_Pos                8                                              /**< (PIO_DRIVER) Drive of PIO Line 8 Position */
#define PIO_DRIVER_LINE8_Msk                (0x1U << PIO_DRIVER_LINE8_Pos)                 /**< (PIO_DRIVER) Drive of PIO Line 8 Mask */
#define PIO_DRIVER_LINE8                    PIO_DRIVER_LINE8_Msk                           /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_DRIVER_LINE8_Msk instead */
#define   PIO_DRIVER_LINE8_LOW_DRIVE_Val    (0x0U)                                         /**< (PIO_DRIVER) Lowest drive  */
#define   PIO_DRIVER_LINE8_HIGH_DRIVE_Val   (0x1U)                                         /**< (PIO_DRIVER) Highest drive  */
#define PIO_DRIVER_LINE8_LOW_DRIVE          (PIO_DRIVER_LINE8_LOW_DRIVE_Val << PIO_DRIVER_LINE8_Pos)  /**< (PIO_DRIVER) Lowest drive Position  */
#define PIO_DRIVER_LINE8_HIGH_DRIVE         (PIO_DRIVER_LINE8_HIGH_DRIVE_Val << PIO_DRIVER_LINE8_Pos)  /**< (PIO_DRIVER) Highest drive Position  */
#define PIO_DRIVER_LINE9_Pos                9                                              /**< (PIO_DRIVER) Drive of PIO Line 9 Position */
#define PIO_DRIVER_LINE9_Msk                (0x1U << PIO_DRIVER_LINE9_Pos)                 /**< (PIO_DRIVER) Drive of PIO Line 9 Mask */
#define PIO_DRIVER_LINE9                    PIO_DRIVER_LINE9_Msk                           /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_DRIVER_LINE9_Msk instead */
#define   PIO_DRIVER_LINE9_LOW_DRIVE_Val    (0x0U)                                         /**< (PIO_DRIVER) Lowest drive  */
#define   PIO_DRIVER_LINE9_HIGH_DRIVE_Val   (0x1U)                                         /**< (PIO_DRIVER) Highest drive  */
#define PIO_DRIVER_LINE9_LOW_DRIVE          (PIO_DRIVER_LINE9_LOW_DRIVE_Val << PIO_DRIVER_LINE9_Pos)  /**< (PIO_DRIVER) Lowest drive Position  */
#define PIO_DRIVER_LINE9_HIGH_DRIVE         (PIO_DRIVER_LINE9_HIGH_DRIVE_Val << PIO_DRIVER_LINE9_Pos)  /**< (PIO_DRIVER) Highest drive Position  */
#define PIO_DRIVER_LINE10_Pos               10                                             /**< (PIO_DRIVER) Drive of PIO Line 10 Position */
#define PIO_DRIVER_LINE10_Msk               (0x1U << PIO_DRIVER_LINE10_Pos)                /**< (PIO_DRIVER) Drive of PIO Line 10 Mask */
#define PIO_DRIVER_LINE10                   PIO_DRIVER_LINE10_Msk                          /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_DRIVER_LINE10_Msk instead */
#define   PIO_DRIVER_LINE10_LOW_DRIVE_Val   (0x0U)                                         /**< (PIO_DRIVER) Lowest drive  */
#define   PIO_DRIVER_LINE10_HIGH_DRIVE_Val  (0x1U)                                         /**< (PIO_DRIVER) Highest drive  */
#define PIO_DRIVER_LINE10_LOW_DRIVE         (PIO_DRIVER_LINE10_LOW_DRIVE_Val << PIO_DRIVER_LINE10_Pos)  /**< (PIO_DRIVER) Lowest drive Position  */
#define PIO_DRIVER_LINE10_HIGH_DRIVE        (PIO_DRIVER_LINE10_HIGH_DRIVE_Val << PIO_DRIVER_LINE10_Pos)  /**< (PIO_DRIVER) Highest drive Position  */
#define PIO_DRIVER_LINE11_Pos               11                                             /**< (PIO_DRIVER) Drive of PIO Line 11 Position */
#define PIO_DRIVER_LINE11_Msk               (0x1U << PIO_DRIVER_LINE11_Pos)                /**< (PIO_DRIVER) Drive of PIO Line 11 Mask */
#define PIO_DRIVER_LINE11                   PIO_DRIVER_LINE11_Msk                          /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_DRIVER_LINE11_Msk instead */
#define   PIO_DRIVER_LINE11_LOW_DRIVE_Val   (0x0U)                                         /**< (PIO_DRIVER) Lowest drive  */
#define   PIO_DRIVER_LINE11_HIGH_DRIVE_Val  (0x1U)                                         /**< (PIO_DRIVER) Highest drive  */
#define PIO_DRIVER_LINE11_LOW_DRIVE         (PIO_DRIVER_LINE11_LOW_DRIVE_Val << PIO_DRIVER_LINE11_Pos)  /**< (PIO_DRIVER) Lowest drive Position  */
#define PIO_DRIVER_LINE11_HIGH_DRIVE        (PIO_DRIVER_LINE11_HIGH_DRIVE_Val << PIO_DRIVER_LINE11_Pos)  /**< (PIO_DRIVER) Highest drive Position  */
#define PIO_DRIVER_LINE12_Pos               12                                             /**< (PIO_DRIVER) Drive of PIO Line 12 Position */
#define PIO_DRIVER_LINE12_Msk               (0x1U << PIO_DRIVER_LINE12_Pos)                /**< (PIO_DRIVER) Drive of PIO Line 12 Mask */
#define PIO_DRIVER_LINE12                   PIO_DRIVER_LINE12_Msk                          /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_DRIVER_LINE12_Msk instead */
#define   PIO_DRIVER_LINE12_LOW_DRIVE_Val   (0x0U)                                         /**< (PIO_DRIVER) Lowest drive  */
#define   PIO_DRIVER_LINE12_HIGH_DRIVE_Val  (0x1U)                                         /**< (PIO_DRIVER) Highest drive  */
#define PIO_DRIVER_LINE12_LOW_DRIVE         (PIO_DRIVER_LINE12_LOW_DRIVE_Val << PIO_DRIVER_LINE12_Pos)  /**< (PIO_DRIVER) Lowest drive Position  */
#define PIO_DRIVER_LINE12_HIGH_DRIVE        (PIO_DRIVER_LINE12_HIGH_DRIVE_Val << PIO_DRIVER_LINE12_Pos)  /**< (PIO_DRIVER) Highest drive Position  */
#define PIO_DRIVER_LINE13_Pos               13                                             /**< (PIO_DRIVER) Drive of PIO Line 13 Position */
#define PIO_DRIVER_LINE13_Msk               (0x1U << PIO_DRIVER_LINE13_Pos)                /**< (PIO_DRIVER) Drive of PIO Line 13 Mask */
#define PIO_DRIVER_LINE13                   PIO_DRIVER_LINE13_Msk                          /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_DRIVER_LINE13_Msk instead */
#define   PIO_DRIVER_LINE13_LOW_DRIVE_Val   (0x0U)                                         /**< (PIO_DRIVER) Lowest drive  */
#define   PIO_DRIVER_LINE13_HIGH_DRIVE_Val  (0x1U)                                         /**< (PIO_DRIVER) Highest drive  */
#define PIO_DRIVER_LINE13_LOW_DRIVE         (PIO_DRIVER_LINE13_LOW_DRIVE_Val << PIO_DRIVER_LINE13_Pos)  /**< (PIO_DRIVER) Lowest drive Position  */
#define PIO_DRIVER_LINE13_HIGH_DRIVE        (PIO_DRIVER_LINE13_HIGH_DRIVE_Val << PIO_DRIVER_LINE13_Pos)  /**< (PIO_DRIVER) Highest drive Position  */
#define PIO_DRIVER_LINE14_Pos               14                                             /**< (PIO_DRIVER) Drive of PIO Line 14 Position */
#define PIO_DRIVER_LINE14_Msk               (0x1U << PIO_DRIVER_LINE14_Pos)                /**< (PIO_DRIVER) Drive of PIO Line 14 Mask */
#define PIO_DRIVER_LINE14                   PIO_DRIVER_LINE14_Msk                          /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_DRIVER_LINE14_Msk instead */
#define   PIO_DRIVER_LINE14_LOW_DRIVE_Val   (0x0U)                                         /**< (PIO_DRIVER) Lowest drive  */
#define   PIO_DRIVER_LINE14_HIGH_DRIVE_Val  (0x1U)                                         /**< (PIO_DRIVER) Highest drive  */
#define PIO_DRIVER_LINE14_LOW_DRIVE         (PIO_DRIVER_LINE14_LOW_DRIVE_Val << PIO_DRIVER_LINE14_Pos)  /**< (PIO_DRIVER) Lowest drive Position  */
#define PIO_DRIVER_LINE14_HIGH_DRIVE        (PIO_DRIVER_LINE14_HIGH_DRIVE_Val << PIO_DRIVER_LINE14_Pos)  /**< (PIO_DRIVER) Highest drive Position  */
#define PIO_DRIVER_LINE15_Pos               15                                             /**< (PIO_DRIVER) Drive of PIO Line 15 Position */
#define PIO_DRIVER_LINE15_Msk               (0x1U << PIO_DRIVER_LINE15_Pos)                /**< (PIO_DRIVER) Drive of PIO Line 15 Mask */
#define PIO_DRIVER_LINE15                   PIO_DRIVER_LINE15_Msk                          /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_DRIVER_LINE15_Msk instead */
#define   PIO_DRIVER_LINE15_LOW_DRIVE_Val   (0x0U)                                         /**< (PIO_DRIVER) Lowest drive  */
#define   PIO_DRIVER_LINE15_HIGH_DRIVE_Val  (0x1U)                                         /**< (PIO_DRIVER) Highest drive  */
#define PIO_DRIVER_LINE15_LOW_DRIVE         (PIO_DRIVER_LINE15_LOW_DRIVE_Val << PIO_DRIVER_LINE15_Pos)  /**< (PIO_DRIVER) Lowest drive Position  */
#define PIO_DRIVER_LINE15_HIGH_DRIVE        (PIO_DRIVER_LINE15_HIGH_DRIVE_Val << PIO_DRIVER_LINE15_Pos)  /**< (PIO_DRIVER) Highest drive Position  */
#define PIO_DRIVER_LINE16_Pos               16                                             /**< (PIO_DRIVER) Drive of PIO Line 16 Position */
#define PIO_DRIVER_LINE16_Msk               (0x1U << PIO_DRIVER_LINE16_Pos)                /**< (PIO_DRIVER) Drive of PIO Line 16 Mask */
#define PIO_DRIVER_LINE16                   PIO_DRIVER_LINE16_Msk                          /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_DRIVER_LINE16_Msk instead */
#define   PIO_DRIVER_LINE16_LOW_DRIVE_Val   (0x0U)                                         /**< (PIO_DRIVER) Lowest drive  */
#define   PIO_DRIVER_LINE16_HIGH_DRIVE_Val  (0x1U)                                         /**< (PIO_DRIVER) Highest drive  */
#define PIO_DRIVER_LINE16_LOW_DRIVE         (PIO_DRIVER_LINE16_LOW_DRIVE_Val << PIO_DRIVER_LINE16_Pos)  /**< (PIO_DRIVER) Lowest drive Position  */
#define PIO_DRIVER_LINE16_HIGH_DRIVE        (PIO_DRIVER_LINE16_HIGH_DRIVE_Val << PIO_DRIVER_LINE16_Pos)  /**< (PIO_DRIVER) Highest drive Position  */
#define PIO_DRIVER_LINE17_Pos               17                                             /**< (PIO_DRIVER) Drive of PIO Line 17 Position */
#define PIO_DRIVER_LINE17_Msk               (0x1U << PIO_DRIVER_LINE17_Pos)                /**< (PIO_DRIVER) Drive of PIO Line 17 Mask */
#define PIO_DRIVER_LINE17                   PIO_DRIVER_LINE17_Msk                          /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_DRIVER_LINE17_Msk instead */
#define   PIO_DRIVER_LINE17_LOW_DRIVE_Val   (0x0U)                                         /**< (PIO_DRIVER) Lowest drive  */
#define   PIO_DRIVER_LINE17_HIGH_DRIVE_Val  (0x1U)                                         /**< (PIO_DRIVER) Highest drive  */
#define PIO_DRIVER_LINE17_LOW_DRIVE         (PIO_DRIVER_LINE17_LOW_DRIVE_Val << PIO_DRIVER_LINE17_Pos)  /**< (PIO_DRIVER) Lowest drive Position  */
#define PIO_DRIVER_LINE17_HIGH_DRIVE        (PIO_DRIVER_LINE17_HIGH_DRIVE_Val << PIO_DRIVER_LINE17_Pos)  /**< (PIO_DRIVER) Highest drive Position  */
#define PIO_DRIVER_LINE18_Pos               18                                             /**< (PIO_DRIVER) Drive of PIO Line 18 Position */
#define PIO_DRIVER_LINE18_Msk               (0x1U << PIO_DRIVER_LINE18_Pos)                /**< (PIO_DRIVER) Drive of PIO Line 18 Mask */
#define PIO_DRIVER_LINE18                   PIO_DRIVER_LINE18_Msk                          /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_DRIVER_LINE18_Msk instead */
#define   PIO_DRIVER_LINE18_LOW_DRIVE_Val   (0x0U)                                         /**< (PIO_DRIVER) Lowest drive  */
#define   PIO_DRIVER_LINE18_HIGH_DRIVE_Val  (0x1U)                                         /**< (PIO_DRIVER) Highest drive  */
#define PIO_DRIVER_LINE18_LOW_DRIVE         (PIO_DRIVER_LINE18_LOW_DRIVE_Val << PIO_DRIVER_LINE18_Pos)  /**< (PIO_DRIVER) Lowest drive Position  */
#define PIO_DRIVER_LINE18_HIGH_DRIVE        (PIO_DRIVER_LINE18_HIGH_DRIVE_Val << PIO_DRIVER_LINE18_Pos)  /**< (PIO_DRIVER) Highest drive Position  */
#define PIO_DRIVER_LINE19_Pos               19                                             /**< (PIO_DRIVER) Drive of PIO Line 19 Position */
#define PIO_DRIVER_LINE19_Msk               (0x1U << PIO_DRIVER_LINE19_Pos)                /**< (PIO_DRIVER) Drive of PIO Line 19 Mask */
#define PIO_DRIVER_LINE19                   PIO_DRIVER_LINE19_Msk                          /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_DRIVER_LINE19_Msk instead */
#define   PIO_DRIVER_LINE19_LOW_DRIVE_Val   (0x0U)                                         /**< (PIO_DRIVER) Lowest drive  */
#define   PIO_DRIVER_LINE19_HIGH_DRIVE_Val  (0x1U)                                         /**< (PIO_DRIVER) Highest drive  */
#define PIO_DRIVER_LINE19_LOW_DRIVE         (PIO_DRIVER_LINE19_LOW_DRIVE_Val << PIO_DRIVER_LINE19_Pos)  /**< (PIO_DRIVER) Lowest drive Position  */
#define PIO_DRIVER_LINE19_HIGH_DRIVE        (PIO_DRIVER_LINE19_HIGH_DRIVE_Val << PIO_DRIVER_LINE19_Pos)  /**< (PIO_DRIVER) Highest drive Position  */
#define PIO_DRIVER_LINE20_Pos               20                                             /**< (PIO_DRIVER) Drive of PIO Line 20 Position */
#define PIO_DRIVER_LINE20_Msk               (0x1U << PIO_DRIVER_LINE20_Pos)                /**< (PIO_DRIVER) Drive of PIO Line 20 Mask */
#define PIO_DRIVER_LINE20                   PIO_DRIVER_LINE20_Msk                          /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_DRIVER_LINE20_Msk instead */
#define   PIO_DRIVER_LINE20_LOW_DRIVE_Val   (0x0U)                                         /**< (PIO_DRIVER) Lowest drive  */
#define   PIO_DRIVER_LINE20_HIGH_DRIVE_Val  (0x1U)                                         /**< (PIO_DRIVER) Highest drive  */
#define PIO_DRIVER_LINE20_LOW_DRIVE         (PIO_DRIVER_LINE20_LOW_DRIVE_Val << PIO_DRIVER_LINE20_Pos)  /**< (PIO_DRIVER) Lowest drive Position  */
#define PIO_DRIVER_LINE20_HIGH_DRIVE        (PIO_DRIVER_LINE20_HIGH_DRIVE_Val << PIO_DRIVER_LINE20_Pos)  /**< (PIO_DRIVER) Highest drive Position  */
#define PIO_DRIVER_LINE21_Pos               21                                             /**< (PIO_DRIVER) Drive of PIO Line 21 Position */
#define PIO_DRIVER_LINE21_Msk               (0x1U << PIO_DRIVER_LINE21_Pos)                /**< (PIO_DRIVER) Drive of PIO Line 21 Mask */
#define PIO_DRIVER_LINE21                   PIO_DRIVER_LINE21_Msk                          /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_DRIVER_LINE21_Msk instead */
#define   PIO_DRIVER_LINE21_LOW_DRIVE_Val   (0x0U)                                         /**< (PIO_DRIVER) Lowest drive  */
#define   PIO_DRIVER_LINE21_HIGH_DRIVE_Val  (0x1U)                                         /**< (PIO_DRIVER) Highest drive  */
#define PIO_DRIVER_LINE21_LOW_DRIVE         (PIO_DRIVER_LINE21_LOW_DRIVE_Val << PIO_DRIVER_LINE21_Pos)  /**< (PIO_DRIVER) Lowest drive Position  */
#define PIO_DRIVER_LINE21_HIGH_DRIVE        (PIO_DRIVER_LINE21_HIGH_DRIVE_Val << PIO_DRIVER_LINE21_Pos)  /**< (PIO_DRIVER) Highest drive Position  */
#define PIO_DRIVER_LINE22_Pos               22                                             /**< (PIO_DRIVER) Drive of PIO Line 22 Position */
#define PIO_DRIVER_LINE22_Msk               (0x1U << PIO_DRIVER_LINE22_Pos)                /**< (PIO_DRIVER) Drive of PIO Line 22 Mask */
#define PIO_DRIVER_LINE22                   PIO_DRIVER_LINE22_Msk                          /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_DRIVER_LINE22_Msk instead */
#define   PIO_DRIVER_LINE22_LOW_DRIVE_Val   (0x0U)                                         /**< (PIO_DRIVER) Lowest drive  */
#define   PIO_DRIVER_LINE22_HIGH_DRIVE_Val  (0x1U)                                         /**< (PIO_DRIVER) Highest drive  */
#define PIO_DRIVER_LINE22_LOW_DRIVE         (PIO_DRIVER_LINE22_LOW_DRIVE_Val << PIO_DRIVER_LINE22_Pos)  /**< (PIO_DRIVER) Lowest drive Position  */
#define PIO_DRIVER_LINE22_HIGH_DRIVE        (PIO_DRIVER_LINE22_HIGH_DRIVE_Val << PIO_DRIVER_LINE22_Pos)  /**< (PIO_DRIVER) Highest drive Position  */
#define PIO_DRIVER_LINE23_Pos               23                                             /**< (PIO_DRIVER) Drive of PIO Line 23 Position */
#define PIO_DRIVER_LINE23_Msk               (0x1U << PIO_DRIVER_LINE23_Pos)                /**< (PIO_DRIVER) Drive of PIO Line 23 Mask */
#define PIO_DRIVER_LINE23                   PIO_DRIVER_LINE23_Msk                          /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_DRIVER_LINE23_Msk instead */
#define   PIO_DRIVER_LINE23_LOW_DRIVE_Val   (0x0U)                                         /**< (PIO_DRIVER) Lowest drive  */
#define   PIO_DRIVER_LINE23_HIGH_DRIVE_Val  (0x1U)                                         /**< (PIO_DRIVER) Highest drive  */
#define PIO_DRIVER_LINE23_LOW_DRIVE         (PIO_DRIVER_LINE23_LOW_DRIVE_Val << PIO_DRIVER_LINE23_Pos)  /**< (PIO_DRIVER) Lowest drive Position  */
#define PIO_DRIVER_LINE23_HIGH_DRIVE        (PIO_DRIVER_LINE23_HIGH_DRIVE_Val << PIO_DRIVER_LINE23_Pos)  /**< (PIO_DRIVER) Highest drive Position  */
#define PIO_DRIVER_LINE24_Pos               24                                             /**< (PIO_DRIVER) Drive of PIO Line 24 Position */
#define PIO_DRIVER_LINE24_Msk               (0x1U << PIO_DRIVER_LINE24_Pos)                /**< (PIO_DRIVER) Drive of PIO Line 24 Mask */
#define PIO_DRIVER_LINE24                   PIO_DRIVER_LINE24_Msk                          /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_DRIVER_LINE24_Msk instead */
#define   PIO_DRIVER_LINE24_LOW_DRIVE_Val   (0x0U)                                         /**< (PIO_DRIVER) Lowest drive  */
#define   PIO_DRIVER_LINE24_HIGH_DRIVE_Val  (0x1U)                                         /**< (PIO_DRIVER) Highest drive  */
#define PIO_DRIVER_LINE24_LOW_DRIVE         (PIO_DRIVER_LINE24_LOW_DRIVE_Val << PIO_DRIVER_LINE24_Pos)  /**< (PIO_DRIVER) Lowest drive Position  */
#define PIO_DRIVER_LINE24_HIGH_DRIVE        (PIO_DRIVER_LINE24_HIGH_DRIVE_Val << PIO_DRIVER_LINE24_Pos)  /**< (PIO_DRIVER) Highest drive Position  */
#define PIO_DRIVER_LINE25_Pos               25                                             /**< (PIO_DRIVER) Drive of PIO Line 25 Position */
#define PIO_DRIVER_LINE25_Msk               (0x1U << PIO_DRIVER_LINE25_Pos)                /**< (PIO_DRIVER) Drive of PIO Line 25 Mask */
#define PIO_DRIVER_LINE25                   PIO_DRIVER_LINE25_Msk                          /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_DRIVER_LINE25_Msk instead */
#define   PIO_DRIVER_LINE25_LOW_DRIVE_Val   (0x0U)                                         /**< (PIO_DRIVER) Lowest drive  */
#define   PIO_DRIVER_LINE25_HIGH_DRIVE_Val  (0x1U)                                         /**< (PIO_DRIVER) Highest drive  */
#define PIO_DRIVER_LINE25_LOW_DRIVE         (PIO_DRIVER_LINE25_LOW_DRIVE_Val << PIO_DRIVER_LINE25_Pos)  /**< (PIO_DRIVER) Lowest drive Position  */
#define PIO_DRIVER_LINE25_HIGH_DRIVE        (PIO_DRIVER_LINE25_HIGH_DRIVE_Val << PIO_DRIVER_LINE25_Pos)  /**< (PIO_DRIVER) Highest drive Position  */
#define PIO_DRIVER_LINE26_Pos               26                                             /**< (PIO_DRIVER) Drive of PIO Line 26 Position */
#define PIO_DRIVER_LINE26_Msk               (0x1U << PIO_DRIVER_LINE26_Pos)                /**< (PIO_DRIVER) Drive of PIO Line 26 Mask */
#define PIO_DRIVER_LINE26                   PIO_DRIVER_LINE26_Msk                          /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_DRIVER_LINE26_Msk instead */
#define   PIO_DRIVER_LINE26_LOW_DRIVE_Val   (0x0U)                                         /**< (PIO_DRIVER) Lowest drive  */
#define   PIO_DRIVER_LINE26_HIGH_DRIVE_Val  (0x1U)                                         /**< (PIO_DRIVER) Highest drive  */
#define PIO_DRIVER_LINE26_LOW_DRIVE         (PIO_DRIVER_LINE26_LOW_DRIVE_Val << PIO_DRIVER_LINE26_Pos)  /**< (PIO_DRIVER) Lowest drive Position  */
#define PIO_DRIVER_LINE26_HIGH_DRIVE        (PIO_DRIVER_LINE26_HIGH_DRIVE_Val << PIO_DRIVER_LINE26_Pos)  /**< (PIO_DRIVER) Highest drive Position  */
#define PIO_DRIVER_LINE27_Pos               27                                             /**< (PIO_DRIVER) Drive of PIO Line 27 Position */
#define PIO_DRIVER_LINE27_Msk               (0x1U << PIO_DRIVER_LINE27_Pos)                /**< (PIO_DRIVER) Drive of PIO Line 27 Mask */
#define PIO_DRIVER_LINE27                   PIO_DRIVER_LINE27_Msk                          /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_DRIVER_LINE27_Msk instead */
#define   PIO_DRIVER_LINE27_LOW_DRIVE_Val   (0x0U)                                         /**< (PIO_DRIVER) Lowest drive  */
#define   PIO_DRIVER_LINE27_HIGH_DRIVE_Val  (0x1U)                                         /**< (PIO_DRIVER) Highest drive  */
#define PIO_DRIVER_LINE27_LOW_DRIVE         (PIO_DRIVER_LINE27_LOW_DRIVE_Val << PIO_DRIVER_LINE27_Pos)  /**< (PIO_DRIVER) Lowest drive Position  */
#define PIO_DRIVER_LINE27_HIGH_DRIVE        (PIO_DRIVER_LINE27_HIGH_DRIVE_Val << PIO_DRIVER_LINE27_Pos)  /**< (PIO_DRIVER) Highest drive Position  */
#define PIO_DRIVER_LINE28_Pos               28                                             /**< (PIO_DRIVER) Drive of PIO Line 28 Position */
#define PIO_DRIVER_LINE28_Msk               (0x1U << PIO_DRIVER_LINE28_Pos)                /**< (PIO_DRIVER) Drive of PIO Line 28 Mask */
#define PIO_DRIVER_LINE28                   PIO_DRIVER_LINE28_Msk                          /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_DRIVER_LINE28_Msk instead */
#define   PIO_DRIVER_LINE28_LOW_DRIVE_Val   (0x0U)                                         /**< (PIO_DRIVER) Lowest drive  */
#define   PIO_DRIVER_LINE28_HIGH_DRIVE_Val  (0x1U)                                         /**< (PIO_DRIVER) Highest drive  */
#define PIO_DRIVER_LINE28_LOW_DRIVE         (PIO_DRIVER_LINE28_LOW_DRIVE_Val << PIO_DRIVER_LINE28_Pos)  /**< (PIO_DRIVER) Lowest drive Position  */
#define PIO_DRIVER_LINE28_HIGH_DRIVE        (PIO_DRIVER_LINE28_HIGH_DRIVE_Val << PIO_DRIVER_LINE28_Pos)  /**< (PIO_DRIVER) Highest drive Position  */
#define PIO_DRIVER_LINE29_Pos               29                                             /**< (PIO_DRIVER) Drive of PIO Line 29 Position */
#define PIO_DRIVER_LINE29_Msk               (0x1U << PIO_DRIVER_LINE29_Pos)                /**< (PIO_DRIVER) Drive of PIO Line 29 Mask */
#define PIO_DRIVER_LINE29                   PIO_DRIVER_LINE29_Msk                          /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_DRIVER_LINE29_Msk instead */
#define   PIO_DRIVER_LINE29_LOW_DRIVE_Val   (0x0U)                                         /**< (PIO_DRIVER) Lowest drive  */
#define   PIO_DRIVER_LINE29_HIGH_DRIVE_Val  (0x1U)                                         /**< (PIO_DRIVER) Highest drive  */
#define PIO_DRIVER_LINE29_LOW_DRIVE         (PIO_DRIVER_LINE29_LOW_DRIVE_Val << PIO_DRIVER_LINE29_Pos)  /**< (PIO_DRIVER) Lowest drive Position  */
#define PIO_DRIVER_LINE29_HIGH_DRIVE        (PIO_DRIVER_LINE29_HIGH_DRIVE_Val << PIO_DRIVER_LINE29_Pos)  /**< (PIO_DRIVER) Highest drive Position  */
#define PIO_DRIVER_LINE30_Pos               30                                             /**< (PIO_DRIVER) Drive of PIO Line 30 Position */
#define PIO_DRIVER_LINE30_Msk               (0x1U << PIO_DRIVER_LINE30_Pos)                /**< (PIO_DRIVER) Drive of PIO Line 30 Mask */
#define PIO_DRIVER_LINE30                   PIO_DRIVER_LINE30_Msk                          /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_DRIVER_LINE30_Msk instead */
#define   PIO_DRIVER_LINE30_LOW_DRIVE_Val   (0x0U)                                         /**< (PIO_DRIVER) Lowest drive  */
#define   PIO_DRIVER_LINE30_HIGH_DRIVE_Val  (0x1U)                                         /**< (PIO_DRIVER) Highest drive  */
#define PIO_DRIVER_LINE30_LOW_DRIVE         (PIO_DRIVER_LINE30_LOW_DRIVE_Val << PIO_DRIVER_LINE30_Pos)  /**< (PIO_DRIVER) Lowest drive Position  */
#define PIO_DRIVER_LINE30_HIGH_DRIVE        (PIO_DRIVER_LINE30_HIGH_DRIVE_Val << PIO_DRIVER_LINE30_Pos)  /**< (PIO_DRIVER) Highest drive Position  */
#define PIO_DRIVER_LINE31_Pos               31                                             /**< (PIO_DRIVER) Drive of PIO Line 31 Position */
#define PIO_DRIVER_LINE31_Msk               (0x1U << PIO_DRIVER_LINE31_Pos)                /**< (PIO_DRIVER) Drive of PIO Line 31 Mask */
#define PIO_DRIVER_LINE31                   PIO_DRIVER_LINE31_Msk                          /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_DRIVER_LINE31_Msk instead */
#define   PIO_DRIVER_LINE31_LOW_DRIVE_Val   (0x0U)                                         /**< (PIO_DRIVER) Lowest drive  */
#define   PIO_DRIVER_LINE31_HIGH_DRIVE_Val  (0x1U)                                         /**< (PIO_DRIVER) Highest drive  */
#define PIO_DRIVER_LINE31_LOW_DRIVE         (PIO_DRIVER_LINE31_LOW_DRIVE_Val << PIO_DRIVER_LINE31_Pos)  /**< (PIO_DRIVER) Lowest drive Position  */
#define PIO_DRIVER_LINE31_HIGH_DRIVE        (PIO_DRIVER_LINE31_HIGH_DRIVE_Val << PIO_DRIVER_LINE31_Pos)  /**< (PIO_DRIVER) Highest drive Position  */
#define PIO_DRIVER_LINE_Pos                 0                                              /**< (PIO_DRIVER Position) Drive of PIO Line 3x */
#define PIO_DRIVER_LINE_Msk                 (0xFFFFFFFFU << PIO_DRIVER_LINE_Pos)           /**< (PIO_DRIVER Mask) LINE */
#define PIO_DRIVER_LINE(value)              (PIO_DRIVER_LINE_Msk & ((value) << PIO_DRIVER_LINE_Pos))  
#define PIO_DRIVER_MASK                     (0xFFFFFFFFU)                                  /**< \deprecated (PIO_DRIVER) Register MASK  (Use PIO_DRIVER_Msk instead)  */
#define PIO_DRIVER_Msk                      (0xFFFFFFFFU)                                  /**< (PIO_DRIVER) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- PIO_PCMR : (PIO Offset: 0x150) (R/W 32) Parallel Capture Mode Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t PCEN:1;                    /**< bit:      0  Parallel Capture Mode Enable             */
    uint32_t :3;                        /**< bit:   1..3  Reserved */
    uint32_t DSIZE:2;                   /**< bit:   4..5  Parallel Capture Mode Data Size          */
    uint32_t :3;                        /**< bit:   6..8  Reserved */
    uint32_t ALWYS:1;                   /**< bit:      9  Parallel Capture Mode Always Sampling    */
    uint32_t HALFS:1;                   /**< bit:     10  Parallel Capture Mode Half Sampling      */
    uint32_t FRSTS:1;                   /**< bit:     11  Parallel Capture Mode First Sample       */
    uint32_t :20;                       /**< bit: 12..31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} PIO_PCMR_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define PIO_PCMR_OFFSET                     (0x150)                                       /**<  (PIO_PCMR) Parallel Capture Mode Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define PIO_PCMR_PCEN_Pos                   0                                              /**< (PIO_PCMR) Parallel Capture Mode Enable Position */
#define PIO_PCMR_PCEN_Msk                   (0x1U << PIO_PCMR_PCEN_Pos)                    /**< (PIO_PCMR) Parallel Capture Mode Enable Mask */
#define PIO_PCMR_PCEN                       PIO_PCMR_PCEN_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PCMR_PCEN_Msk instead */
#define PIO_PCMR_DSIZE_Pos                  4                                              /**< (PIO_PCMR) Parallel Capture Mode Data Size Position */
#define PIO_PCMR_DSIZE_Msk                  (0x3U << PIO_PCMR_DSIZE_Pos)                   /**< (PIO_PCMR) Parallel Capture Mode Data Size Mask */
#define PIO_PCMR_DSIZE(value)               (PIO_PCMR_DSIZE_Msk & ((value) << PIO_PCMR_DSIZE_Pos))
#define   PIO_PCMR_DSIZE_BYTE_Val           (0x0U)                                         /**< (PIO_PCMR) The reception data in the PIO_PCRHR is a byte (8-bit)  */
#define   PIO_PCMR_DSIZE_HALFWORD_Val       (0x1U)                                         /**< (PIO_PCMR) The reception data in the PIO_PCRHR is a half-word (16-bit)  */
#define   PIO_PCMR_DSIZE_WORD_Val           (0x2U)                                         /**< (PIO_PCMR) The reception data in the PIO_PCRHR is a word (32-bit)  */
#define PIO_PCMR_DSIZE_BYTE                 (PIO_PCMR_DSIZE_BYTE_Val << PIO_PCMR_DSIZE_Pos)  /**< (PIO_PCMR) The reception data in the PIO_PCRHR is a byte (8-bit) Position  */
#define PIO_PCMR_DSIZE_HALFWORD             (PIO_PCMR_DSIZE_HALFWORD_Val << PIO_PCMR_DSIZE_Pos)  /**< (PIO_PCMR) The reception data in the PIO_PCRHR is a half-word (16-bit) Position  */
#define PIO_PCMR_DSIZE_WORD                 (PIO_PCMR_DSIZE_WORD_Val << PIO_PCMR_DSIZE_Pos)  /**< (PIO_PCMR) The reception data in the PIO_PCRHR is a word (32-bit) Position  */
#define PIO_PCMR_ALWYS_Pos                  9                                              /**< (PIO_PCMR) Parallel Capture Mode Always Sampling Position */
#define PIO_PCMR_ALWYS_Msk                  (0x1U << PIO_PCMR_ALWYS_Pos)                   /**< (PIO_PCMR) Parallel Capture Mode Always Sampling Mask */
#define PIO_PCMR_ALWYS                      PIO_PCMR_ALWYS_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PCMR_ALWYS_Msk instead */
#define PIO_PCMR_HALFS_Pos                  10                                             /**< (PIO_PCMR) Parallel Capture Mode Half Sampling Position */
#define PIO_PCMR_HALFS_Msk                  (0x1U << PIO_PCMR_HALFS_Pos)                   /**< (PIO_PCMR) Parallel Capture Mode Half Sampling Mask */
#define PIO_PCMR_HALFS                      PIO_PCMR_HALFS_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PCMR_HALFS_Msk instead */
#define PIO_PCMR_FRSTS_Pos                  11                                             /**< (PIO_PCMR) Parallel Capture Mode First Sample Position */
#define PIO_PCMR_FRSTS_Msk                  (0x1U << PIO_PCMR_FRSTS_Pos)                   /**< (PIO_PCMR) Parallel Capture Mode First Sample Mask */
#define PIO_PCMR_FRSTS                      PIO_PCMR_FRSTS_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PCMR_FRSTS_Msk instead */
#define PIO_PCMR_MASK                       (0xE31U)                                       /**< \deprecated (PIO_PCMR) Register MASK  (Use PIO_PCMR_Msk instead)  */
#define PIO_PCMR_Msk                        (0xE31U)                                       /**< (PIO_PCMR) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- PIO_PCIER : (PIO Offset: 0x154) (/W 32) Parallel Capture Interrupt Enable Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t DRDY:1;                    /**< bit:      0  Parallel Capture Mode Data Ready Interrupt Enable */
    uint32_t OVRE:1;                    /**< bit:      1  Parallel Capture Mode Overrun Error Interrupt Enable */
    uint32_t ENDRX:1;                   /**< bit:      2  End of Reception Transfer Interrupt Enable */
    uint32_t RXBUFF:1;                  /**< bit:      3  Reception Buffer Full Interrupt Enable   */
    uint32_t :28;                       /**< bit:  4..31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} PIO_PCIER_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define PIO_PCIER_OFFSET                    (0x154)                                       /**<  (PIO_PCIER) Parallel Capture Interrupt Enable Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define PIO_PCIER_DRDY_Pos                  0                                              /**< (PIO_PCIER) Parallel Capture Mode Data Ready Interrupt Enable Position */
#define PIO_PCIER_DRDY_Msk                  (0x1U << PIO_PCIER_DRDY_Pos)                   /**< (PIO_PCIER) Parallel Capture Mode Data Ready Interrupt Enable Mask */
#define PIO_PCIER_DRDY                      PIO_PCIER_DRDY_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PCIER_DRDY_Msk instead */
#define PIO_PCIER_OVRE_Pos                  1                                              /**< (PIO_PCIER) Parallel Capture Mode Overrun Error Interrupt Enable Position */
#define PIO_PCIER_OVRE_Msk                  (0x1U << PIO_PCIER_OVRE_Pos)                   /**< (PIO_PCIER) Parallel Capture Mode Overrun Error Interrupt Enable Mask */
#define PIO_PCIER_OVRE                      PIO_PCIER_OVRE_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PCIER_OVRE_Msk instead */
#define PIO_PCIER_ENDRX_Pos                 2                                              /**< (PIO_PCIER) End of Reception Transfer Interrupt Enable Position */
#define PIO_PCIER_ENDRX_Msk                 (0x1U << PIO_PCIER_ENDRX_Pos)                  /**< (PIO_PCIER) End of Reception Transfer Interrupt Enable Mask */
#define PIO_PCIER_ENDRX                     PIO_PCIER_ENDRX_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PCIER_ENDRX_Msk instead */
#define PIO_PCIER_RXBUFF_Pos                3                                              /**< (PIO_PCIER) Reception Buffer Full Interrupt Enable Position */
#define PIO_PCIER_RXBUFF_Msk                (0x1U << PIO_PCIER_RXBUFF_Pos)                 /**< (PIO_PCIER) Reception Buffer Full Interrupt Enable Mask */
#define PIO_PCIER_RXBUFF                    PIO_PCIER_RXBUFF_Msk                           /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PCIER_RXBUFF_Msk instead */
#define PIO_PCIER_MASK                      (0x0FU)                                        /**< \deprecated (PIO_PCIER) Register MASK  (Use PIO_PCIER_Msk instead)  */
#define PIO_PCIER_Msk                       (0x0FU)                                        /**< (PIO_PCIER) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- PIO_PCIDR : (PIO Offset: 0x158) (/W 32) Parallel Capture Interrupt Disable Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t DRDY:1;                    /**< bit:      0  Parallel Capture Mode Data Ready Interrupt Disable */
    uint32_t OVRE:1;                    /**< bit:      1  Parallel Capture Mode Overrun Error Interrupt Disable */
    uint32_t ENDRX:1;                   /**< bit:      2  End of Reception Transfer Interrupt Disable */
    uint32_t RXBUFF:1;                  /**< bit:      3  Reception Buffer Full Interrupt Disable  */
    uint32_t :28;                       /**< bit:  4..31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} PIO_PCIDR_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define PIO_PCIDR_OFFSET                    (0x158)                                       /**<  (PIO_PCIDR) Parallel Capture Interrupt Disable Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define PIO_PCIDR_DRDY_Pos                  0                                              /**< (PIO_PCIDR) Parallel Capture Mode Data Ready Interrupt Disable Position */
#define PIO_PCIDR_DRDY_Msk                  (0x1U << PIO_PCIDR_DRDY_Pos)                   /**< (PIO_PCIDR) Parallel Capture Mode Data Ready Interrupt Disable Mask */
#define PIO_PCIDR_DRDY                      PIO_PCIDR_DRDY_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PCIDR_DRDY_Msk instead */
#define PIO_PCIDR_OVRE_Pos                  1                                              /**< (PIO_PCIDR) Parallel Capture Mode Overrun Error Interrupt Disable Position */
#define PIO_PCIDR_OVRE_Msk                  (0x1U << PIO_PCIDR_OVRE_Pos)                   /**< (PIO_PCIDR) Parallel Capture Mode Overrun Error Interrupt Disable Mask */
#define PIO_PCIDR_OVRE                      PIO_PCIDR_OVRE_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PCIDR_OVRE_Msk instead */
#define PIO_PCIDR_ENDRX_Pos                 2                                              /**< (PIO_PCIDR) End of Reception Transfer Interrupt Disable Position */
#define PIO_PCIDR_ENDRX_Msk                 (0x1U << PIO_PCIDR_ENDRX_Pos)                  /**< (PIO_PCIDR) End of Reception Transfer Interrupt Disable Mask */
#define PIO_PCIDR_ENDRX                     PIO_PCIDR_ENDRX_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PCIDR_ENDRX_Msk instead */
#define PIO_PCIDR_RXBUFF_Pos                3                                              /**< (PIO_PCIDR) Reception Buffer Full Interrupt Disable Position */
#define PIO_PCIDR_RXBUFF_Msk                (0x1U << PIO_PCIDR_RXBUFF_Pos)                 /**< (PIO_PCIDR) Reception Buffer Full Interrupt Disable Mask */
#define PIO_PCIDR_RXBUFF                    PIO_PCIDR_RXBUFF_Msk                           /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PCIDR_RXBUFF_Msk instead */
#define PIO_PCIDR_MASK                      (0x0FU)                                        /**< \deprecated (PIO_PCIDR) Register MASK  (Use PIO_PCIDR_Msk instead)  */
#define PIO_PCIDR_Msk                       (0x0FU)                                        /**< (PIO_PCIDR) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- PIO_PCIMR : (PIO Offset: 0x15c) (R/ 32) Parallel Capture Interrupt Mask Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t DRDY:1;                    /**< bit:      0  Parallel Capture Mode Data Ready Interrupt Mask */
    uint32_t OVRE:1;                    /**< bit:      1  Parallel Capture Mode Overrun Error Interrupt Mask */
    uint32_t ENDRX:1;                   /**< bit:      2  End of Reception Transfer Interrupt Mask */
    uint32_t RXBUFF:1;                  /**< bit:      3  Reception Buffer Full Interrupt Mask     */
    uint32_t :28;                       /**< bit:  4..31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} PIO_PCIMR_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define PIO_PCIMR_OFFSET                    (0x15C)                                       /**<  (PIO_PCIMR) Parallel Capture Interrupt Mask Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define PIO_PCIMR_DRDY_Pos                  0                                              /**< (PIO_PCIMR) Parallel Capture Mode Data Ready Interrupt Mask Position */
#define PIO_PCIMR_DRDY_Msk                  (0x1U << PIO_PCIMR_DRDY_Pos)                   /**< (PIO_PCIMR) Parallel Capture Mode Data Ready Interrupt Mask Mask */
#define PIO_PCIMR_DRDY                      PIO_PCIMR_DRDY_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PCIMR_DRDY_Msk instead */
#define PIO_PCIMR_OVRE_Pos                  1                                              /**< (PIO_PCIMR) Parallel Capture Mode Overrun Error Interrupt Mask Position */
#define PIO_PCIMR_OVRE_Msk                  (0x1U << PIO_PCIMR_OVRE_Pos)                   /**< (PIO_PCIMR) Parallel Capture Mode Overrun Error Interrupt Mask Mask */
#define PIO_PCIMR_OVRE                      PIO_PCIMR_OVRE_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PCIMR_OVRE_Msk instead */
#define PIO_PCIMR_ENDRX_Pos                 2                                              /**< (PIO_PCIMR) End of Reception Transfer Interrupt Mask Position */
#define PIO_PCIMR_ENDRX_Msk                 (0x1U << PIO_PCIMR_ENDRX_Pos)                  /**< (PIO_PCIMR) End of Reception Transfer Interrupt Mask Mask */
#define PIO_PCIMR_ENDRX                     PIO_PCIMR_ENDRX_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PCIMR_ENDRX_Msk instead */
#define PIO_PCIMR_RXBUFF_Pos                3                                              /**< (PIO_PCIMR) Reception Buffer Full Interrupt Mask Position */
#define PIO_PCIMR_RXBUFF_Msk                (0x1U << PIO_PCIMR_RXBUFF_Pos)                 /**< (PIO_PCIMR) Reception Buffer Full Interrupt Mask Mask */
#define PIO_PCIMR_RXBUFF                    PIO_PCIMR_RXBUFF_Msk                           /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PCIMR_RXBUFF_Msk instead */
#define PIO_PCIMR_MASK                      (0x0FU)                                        /**< \deprecated (PIO_PCIMR) Register MASK  (Use PIO_PCIMR_Msk instead)  */
#define PIO_PCIMR_Msk                       (0x0FU)                                        /**< (PIO_PCIMR) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- PIO_PCISR : (PIO Offset: 0x160) (R/ 32) Parallel Capture Interrupt Status Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t DRDY:1;                    /**< bit:      0  Parallel Capture Mode Data Ready         */
    uint32_t OVRE:1;                    /**< bit:      1  Parallel Capture Mode Overrun Error      */
    uint32_t :30;                       /**< bit:  2..31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} PIO_PCISR_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define PIO_PCISR_OFFSET                    (0x160)                                       /**<  (PIO_PCISR) Parallel Capture Interrupt Status Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define PIO_PCISR_DRDY_Pos                  0                                              /**< (PIO_PCISR) Parallel Capture Mode Data Ready Position */
#define PIO_PCISR_DRDY_Msk                  (0x1U << PIO_PCISR_DRDY_Pos)                   /**< (PIO_PCISR) Parallel Capture Mode Data Ready Mask */
#define PIO_PCISR_DRDY                      PIO_PCISR_DRDY_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PCISR_DRDY_Msk instead */
#define PIO_PCISR_OVRE_Pos                  1                                              /**< (PIO_PCISR) Parallel Capture Mode Overrun Error Position */
#define PIO_PCISR_OVRE_Msk                  (0x1U << PIO_PCISR_OVRE_Pos)                   /**< (PIO_PCISR) Parallel Capture Mode Overrun Error Mask */
#define PIO_PCISR_OVRE                      PIO_PCISR_OVRE_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use PIO_PCISR_OVRE_Msk instead */
#define PIO_PCISR_MASK                      (0x03U)                                        /**< \deprecated (PIO_PCISR) Register MASK  (Use PIO_PCISR_Msk instead)  */
#define PIO_PCISR_Msk                       (0x03U)                                        /**< (PIO_PCISR) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- PIO_PCRHR : (PIO Offset: 0x164) (R/ 32) Parallel Capture Reception Holding Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t RDATA:32;                  /**< bit:  0..31  Parallel Capture Mode Reception Data     */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} PIO_PCRHR_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define PIO_PCRHR_OFFSET                    (0x164)                                       /**<  (PIO_PCRHR) Parallel Capture Reception Holding Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define PIO_PCRHR_RDATA_Pos                 0                                              /**< (PIO_PCRHR) Parallel Capture Mode Reception Data Position */
#define PIO_PCRHR_RDATA_Msk                 (0xFFFFFFFFU << PIO_PCRHR_RDATA_Pos)           /**< (PIO_PCRHR) Parallel Capture Mode Reception Data Mask */
#define PIO_PCRHR_RDATA(value)              (PIO_PCRHR_RDATA_Msk & ((value) << PIO_PCRHR_RDATA_Pos))
#define PIO_PCRHR_MASK                      (0xFFFFFFFFU)                                  /**< \deprecated (PIO_PCRHR) Register MASK  (Use PIO_PCRHR_Msk instead)  */
#define PIO_PCRHR_Msk                       (0xFFFFFFFFU)                                  /**< (PIO_PCRHR) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
#if COMPONENT_TYPEDEF_STYLE == 'R'
/** \brief PIO hardware registers */
typedef struct {  
  __O  uint32_t PIO_PER;        /**< (PIO Offset: 0x00) PIO Enable Register */
  __O  uint32_t PIO_PDR;        /**< (PIO Offset: 0x04) PIO Disable Register */
  __I  uint32_t PIO_PSR;        /**< (PIO Offset: 0x08) PIO Status Register */
  __I  uint32_t Reserved1[1];
  __O  uint32_t PIO_OER;        /**< (PIO Offset: 0x10) Output Enable Register */
  __O  uint32_t PIO_ODR;        /**< (PIO Offset: 0x14) Output Disable Register */
  __I  uint32_t PIO_OSR;        /**< (PIO Offset: 0x18) Output Status Register */
  __I  uint32_t Reserved2[1];
  __O  uint32_t PIO_IFER;       /**< (PIO Offset: 0x20) Glitch Input Filter Enable Register */
  __O  uint32_t PIO_IFDR;       /**< (PIO Offset: 0x24) Glitch Input Filter Disable Register */
  __I  uint32_t PIO_IFSR;       /**< (PIO Offset: 0x28) Glitch Input Filter Status Register */
  __I  uint32_t Reserved3[1];
  __O  uint32_t PIO_SODR;       /**< (PIO Offset: 0x30) Set Output Data Register */
  __O  uint32_t PIO_CODR;       /**< (PIO Offset: 0x34) Clear Output Data Register */
  __IO uint32_t PIO_ODSR;       /**< (PIO Offset: 0x38) Output Data Status Register */
  __I  uint32_t PIO_PDSR;       /**< (PIO Offset: 0x3C) Pin Data Status Register */
  __O  uint32_t PIO_IER;        /**< (PIO Offset: 0x40) Interrupt Enable Register */
  __O  uint32_t PIO_IDR;        /**< (PIO Offset: 0x44) Interrupt Disable Register */
  __I  uint32_t PIO_IMR;        /**< (PIO Offset: 0x48) Interrupt Mask Register */
  __I  uint32_t PIO_ISR;        /**< (PIO Offset: 0x4C) Interrupt Status Register */
  __O  uint32_t PIO_MDER;       /**< (PIO Offset: 0x50) Multi-driver Enable Register */
  __O  uint32_t PIO_MDDR;       /**< (PIO Offset: 0x54) Multi-driver Disable Register */
  __I  uint32_t PIO_MDSR;       /**< (PIO Offset: 0x58) Multi-driver Status Register */
  __I  uint32_t Reserved4[1];
  __O  uint32_t PIO_PUDR;       /**< (PIO Offset: 0x60) Pull-up Disable Register */
  __O  uint32_t PIO_PUER;       /**< (PIO Offset: 0x64) Pull-up Enable Register */
  __I  uint32_t PIO_PUSR;       /**< (PIO Offset: 0x68) Pad Pull-up Status Register */
  __I  uint32_t Reserved5[1];
  __IO uint32_t PIO_ABCDSR[2];  /**< (PIO Offset: 0x70) Peripheral Select Register 0 */
  __I  uint32_t Reserved6[2];
  __O  uint32_t PIO_IFSCDR;     /**< (PIO Offset: 0x80) Input Filter Slow Clock Disable Register */
  __O  uint32_t PIO_IFSCER;     /**< (PIO Offset: 0x84) Input Filter Slow Clock Enable Register */
  __I  uint32_t PIO_IFSCSR;     /**< (PIO Offset: 0x88) Input Filter Slow Clock Status Register */
  __IO uint32_t PIO_SCDR;       /**< (PIO Offset: 0x8C) Slow Clock Divider Debouncing Register */
  __O  uint32_t PIO_PPDDR;      /**< (PIO Offset: 0x90) Pad Pull-down Disable Register */
  __O  uint32_t PIO_PPDER;      /**< (PIO Offset: 0x94) Pad Pull-down Enable Register */
  __I  uint32_t PIO_PPDSR;      /**< (PIO Offset: 0x98) Pad Pull-down Status Register */
  __I  uint32_t Reserved7[1];
  __O  uint32_t PIO_OWER;       /**< (PIO Offset: 0xA0) Output Write Enable */
  __O  uint32_t PIO_OWDR;       /**< (PIO Offset: 0xA4) Output Write Disable */
  __I  uint32_t PIO_OWSR;       /**< (PIO Offset: 0xA8) Output Write Status Register */
  __I  uint32_t Reserved8[1];
  __O  uint32_t PIO_AIMER;      /**< (PIO Offset: 0xB0) Additional Interrupt Modes Enable Register */
  __O  uint32_t PIO_AIMDR;      /**< (PIO Offset: 0xB4) Additional Interrupt Modes Disable Register */
  __I  uint32_t PIO_AIMMR;      /**< (PIO Offset: 0xB8) Additional Interrupt Modes Mask Register */
  __I  uint32_t Reserved9[1];
  __O  uint32_t PIO_ESR;        /**< (PIO Offset: 0xC0) Edge Select Register */
  __O  uint32_t PIO_LSR;        /**< (PIO Offset: 0xC4) Level Select Register */
  __I  uint32_t PIO_ELSR;       /**< (PIO Offset: 0xC8) Edge/Level Status Register */
  __I  uint32_t Reserved10[1];
  __O  uint32_t PIO_FELLSR;     /**< (PIO Offset: 0xD0) Falling Edge/Low-Level Select Register */
  __O  uint32_t PIO_REHLSR;     /**< (PIO Offset: 0xD4) Rising Edge/High-Level Select Register */
  __I  uint32_t PIO_FRLHSR;     /**< (PIO Offset: 0xD8) Fall/Rise - Low/High Status Register */
  __I  uint32_t Reserved11[1];
  __I  uint32_t PIO_LOCKSR;     /**< (PIO Offset: 0xE0) Lock Status */
  __IO uint32_t PIO_WPMR;       /**< (PIO Offset: 0xE4) Write Protection Mode Register */
  __I  uint32_t PIO_WPSR;       /**< (PIO Offset: 0xE8) Write Protection Status Register */
  __I  uint32_t Reserved12[5];
  __IO uint32_t PIO_SCHMITT;    /**< (PIO Offset: 0x100) Schmitt Trigger Register */
  __I  uint32_t Reserved13[5];
  __IO uint32_t PIO_DRIVER;     /**< (PIO Offset: 0x118) I/O Drive Register */
  __I  uint32_t Reserved14[13];
  __IO uint32_t PIO_PCMR;       /**< (PIO Offset: 0x150) Parallel Capture Mode Register */
  __O  uint32_t PIO_PCIER;      /**< (PIO Offset: 0x154) Parallel Capture Interrupt Enable Register */
  __O  uint32_t PIO_PCIDR;      /**< (PIO Offset: 0x158) Parallel Capture Interrupt Disable Register */
  __I  uint32_t PIO_PCIMR;      /**< (PIO Offset: 0x15C) Parallel Capture Interrupt Mask Register */
  __I  uint32_t PIO_PCISR;      /**< (PIO Offset: 0x160) Parallel Capture Interrupt Status Register */
  __I  uint32_t PIO_PCRHR;      /**< (PIO Offset: 0x164) Parallel Capture Reception Holding Register */
} Pio;

#elif COMPONENT_TYPEDEF_STYLE == 'N'
/** \brief PIO hardware registers */
typedef struct {  
  __O  PIO_PER_Type                   PIO_PER;        /**< Offset: 0x00 ( /W  32) PIO Enable Register */
  __O  PIO_PDR_Type                   PIO_PDR;        /**< Offset: 0x04 ( /W  32) PIO Disable Register */
  __I  PIO_PSR_Type                   PIO_PSR;        /**< Offset: 0x08 (R/   32) PIO Status Register */
       RoReg8                         Reserved1[0x4];
  __O  PIO_OER_Type                   PIO_OER;        /**< Offset: 0x10 ( /W  32) Output Enable Register */
  __O  PIO_ODR_Type                   PIO_ODR;        /**< Offset: 0x14 ( /W  32) Output Disable Register */
  __I  PIO_OSR_Type                   PIO_OSR;        /**< Offset: 0x18 (R/   32) Output Status Register */
       RoReg8                         Reserved2[0x4];
  __O  PIO_IFER_Type                  PIO_IFER;       /**< Offset: 0x20 ( /W  32) Glitch Input Filter Enable Register */
  __O  PIO_IFDR_Type                  PIO_IFDR;       /**< Offset: 0x24 ( /W  32) Glitch Input Filter Disable Register */
  __I  PIO_IFSR_Type                  PIO_IFSR;       /**< Offset: 0x28 (R/   32) Glitch Input Filter Status Register */
       RoReg8                         Reserved3[0x4];
  __O  PIO_SODR_Type                  PIO_SODR;       /**< Offset: 0x30 ( /W  32) Set Output Data Register */
  __O  PIO_CODR_Type                  PIO_CODR;       /**< Offset: 0x34 ( /W  32) Clear Output Data Register */
  __IO PIO_ODSR_Type                  PIO_ODSR;       /**< Offset: 0x38 (R/W  32) Output Data Status Register */
  __I  PIO_PDSR_Type                  PIO_PDSR;       /**< Offset: 0x3C (R/   32) Pin Data Status Register */
  __O  PIO_IER_Type                   PIO_IER;        /**< Offset: 0x40 ( /W  32) Interrupt Enable Register */
  __O  PIO_IDR_Type                   PIO_IDR;        /**< Offset: 0x44 ( /W  32) Interrupt Disable Register */
  __I  PIO_IMR_Type                   PIO_IMR;        /**< Offset: 0x48 (R/   32) Interrupt Mask Register */
  __I  PIO_ISR_Type                   PIO_ISR;        /**< Offset: 0x4C (R/   32) Interrupt Status Register */
  __O  PIO_MDER_Type                  PIO_MDER;       /**< Offset: 0x50 ( /W  32) Multi-driver Enable Register */
  __O  PIO_MDDR_Type                  PIO_MDDR;       /**< Offset: 0x54 ( /W  32) Multi-driver Disable Register */
  __I  PIO_MDSR_Type                  PIO_MDSR;       /**< Offset: 0x58 (R/   32) Multi-driver Status Register */
       RoReg8                         Reserved4[0x4];
  __O  PIO_PUDR_Type                  PIO_PUDR;       /**< Offset: 0x60 ( /W  32) Pull-up Disable Register */
  __O  PIO_PUER_Type                  PIO_PUER;       /**< Offset: 0x64 ( /W  32) Pull-up Enable Register */
  __I  PIO_PUSR_Type                  PIO_PUSR;       /**< Offset: 0x68 (R/   32) Pad Pull-up Status Register */
       RoReg8                         Reserved5[0x4];
  __IO PIO_ABCDSR_Type                PIO_ABCDSR[2];  /**< Offset: 0x70 (R/W  32) Peripheral Select Register 0 */
       RoReg8                         Reserved6[0x8];
  __O  PIO_IFSCDR_Type                PIO_IFSCDR;     /**< Offset: 0x80 ( /W  32) Input Filter Slow Clock Disable Register */
  __O  PIO_IFSCER_Type                PIO_IFSCER;     /**< Offset: 0x84 ( /W  32) Input Filter Slow Clock Enable Register */
  __I  PIO_IFSCSR_Type                PIO_IFSCSR;     /**< Offset: 0x88 (R/   32) Input Filter Slow Clock Status Register */
  __IO PIO_SCDR_Type                  PIO_SCDR;       /**< Offset: 0x8C (R/W  32) Slow Clock Divider Debouncing Register */
  __O  PIO_PPDDR_Type                 PIO_PPDDR;      /**< Offset: 0x90 ( /W  32) Pad Pull-down Disable Register */
  __O  PIO_PPDER_Type                 PIO_PPDER;      /**< Offset: 0x94 ( /W  32) Pad Pull-down Enable Register */
  __I  PIO_PPDSR_Type                 PIO_PPDSR;      /**< Offset: 0x98 (R/   32) Pad Pull-down Status Register */
       RoReg8                         Reserved7[0x4];
  __O  PIO_OWER_Type                  PIO_OWER;       /**< Offset: 0xA0 ( /W  32) Output Write Enable */
  __O  PIO_OWDR_Type                  PIO_OWDR;       /**< Offset: 0xA4 ( /W  32) Output Write Disable */
  __I  PIO_OWSR_Type                  PIO_OWSR;       /**< Offset: 0xA8 (R/   32) Output Write Status Register */
       RoReg8                         Reserved8[0x4];
  __O  PIO_AIMER_Type                 PIO_AIMER;      /**< Offset: 0xB0 ( /W  32) Additional Interrupt Modes Enable Register */
  __O  PIO_AIMDR_Type                 PIO_AIMDR;      /**< Offset: 0xB4 ( /W  32) Additional Interrupt Modes Disable Register */
  __I  PIO_AIMMR_Type                 PIO_AIMMR;      /**< Offset: 0xB8 (R/   32) Additional Interrupt Modes Mask Register */
       RoReg8                         Reserved9[0x4];
  __O  PIO_ESR_Type                   PIO_ESR;        /**< Offset: 0xC0 ( /W  32) Edge Select Register */
  __O  PIO_LSR_Type                   PIO_LSR;        /**< Offset: 0xC4 ( /W  32) Level Select Register */
  __I  PIO_ELSR_Type                  PIO_ELSR;       /**< Offset: 0xC8 (R/   32) Edge/Level Status Register */
       RoReg8                         Reserved10[0x4];
  __O  PIO_FELLSR_Type                PIO_FELLSR;     /**< Offset: 0xD0 ( /W  32) Falling Edge/Low-Level Select Register */
  __O  PIO_REHLSR_Type                PIO_REHLSR;     /**< Offset: 0xD4 ( /W  32) Rising Edge/High-Level Select Register */
  __I  PIO_FRLHSR_Type                PIO_FRLHSR;     /**< Offset: 0xD8 (R/   32) Fall/Rise - Low/High Status Register */
       RoReg8                         Reserved11[0x4];
  __I  PIO_LOCKSR_Type                PIO_LOCKSR;     /**< Offset: 0xE0 (R/   32) Lock Status */
  __IO PIO_WPMR_Type                  PIO_WPMR;       /**< Offset: 0xE4 (R/W  32) Write Protection Mode Register */
  __I  PIO_WPSR_Type                  PIO_WPSR;       /**< Offset: 0xE8 (R/   32) Write Protection Status Register */
       RoReg8                         Reserved12[0x14];
  __IO PIO_SCHMITT_Type               PIO_SCHMITT;    /**< Offset: 0x100 (R/W  32) Schmitt Trigger Register */
       RoReg8                         Reserved13[0x14];
  __IO PIO_DRIVER_Type                PIO_DRIVER;     /**< Offset: 0x118 (R/W  32) I/O Drive Register */
       RoReg8                         Reserved14[0x34];
  __IO PIO_PCMR_Type                  PIO_PCMR;       /**< Offset: 0x150 (R/W  32) Parallel Capture Mode Register */
  __O  PIO_PCIER_Type                 PIO_PCIER;      /**< Offset: 0x154 ( /W  32) Parallel Capture Interrupt Enable Register */
  __O  PIO_PCIDR_Type                 PIO_PCIDR;      /**< Offset: 0x158 ( /W  32) Parallel Capture Interrupt Disable Register */
  __I  PIO_PCIMR_Type                 PIO_PCIMR;      /**< Offset: 0x15C (R/   32) Parallel Capture Interrupt Mask Register */
  __I  PIO_PCISR_Type                 PIO_PCISR;      /**< Offset: 0x160 (R/   32) Parallel Capture Interrupt Status Register */
  __I  PIO_PCRHR_Type                 PIO_PCRHR;      /**< Offset: 0x164 (R/   32) Parallel Capture Reception Holding Register */
} Pio;

#else /* COMPONENT_TYPEDEF_STYLE */
#error Unknown component typedef style
#endif /* COMPONENT_TYPEDEF_STYLE */

#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/** @}  end of Parallel Input/Output Controller */

#endif /* _SAME70_PIO_COMPONENT_H_ */
