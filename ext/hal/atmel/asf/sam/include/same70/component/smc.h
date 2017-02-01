/**
 * \file
 *
 * \brief Component description for SMC
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

#ifndef _SAME70_SMC_COMPONENT_H_
#define _SAME70_SMC_COMPONENT_H_
#define _SAME70_SMC_COMPONENT_         /**< \deprecated  Backward compatibility for ASF */

/** \addtogroup SAME70_SMC Static Memory Controller
 *  @{
 */
/* ========================================================================== */
/**  SOFTWARE API DEFINITION FOR SMC */
/* ========================================================================== */
#ifndef COMPONENT_TYPEDEF_STYLE
  #define COMPONENT_TYPEDEF_STYLE 'R'  /**< Defines default style of typedefs for the component header files ('R' = RFO, 'N' = NTO)*/
#endif

#define SMC_6498                       /**< (SMC) Module ID */
#define REV_SMC H                      /**< (SMC) Module revision */

/* -------- SMC_SETUP : (SMC Offset: 0x00) (R/W 32) SMC Setup Register (CS_number = 0) -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t NWE_SETUP:6;               /**< bit:   0..5  NWE Setup Length                         */
    uint32_t :2;                        /**< bit:   6..7  Reserved */
    uint32_t NCS_WR_SETUP:6;            /**< bit:  8..13  NCS Setup Length in WRITE Access         */
    uint32_t :2;                        /**< bit: 14..15  Reserved */
    uint32_t NRD_SETUP:6;               /**< bit: 16..21  NRD Setup Length                         */
    uint32_t :2;                        /**< bit: 22..23  Reserved */
    uint32_t NCS_RD_SETUP:6;            /**< bit: 24..29  NCS Setup Length in READ Access          */
    uint32_t :2;                        /**< bit: 30..31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} SMC_SETUP_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define SMC_SETUP_OFFSET                    (0x00)                                        /**<  (SMC_SETUP) SMC Setup Register (CS_number = 0)  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define SMC_SETUP_NWE_SETUP_Pos             0                                              /**< (SMC_SETUP) NWE Setup Length Position */
#define SMC_SETUP_NWE_SETUP_Msk             (0x3FU << SMC_SETUP_NWE_SETUP_Pos)             /**< (SMC_SETUP) NWE Setup Length Mask */
#define SMC_SETUP_NWE_SETUP(value)          (SMC_SETUP_NWE_SETUP_Msk & ((value) << SMC_SETUP_NWE_SETUP_Pos))
#define SMC_SETUP_NCS_WR_SETUP_Pos          8                                              /**< (SMC_SETUP) NCS Setup Length in WRITE Access Position */
#define SMC_SETUP_NCS_WR_SETUP_Msk          (0x3FU << SMC_SETUP_NCS_WR_SETUP_Pos)          /**< (SMC_SETUP) NCS Setup Length in WRITE Access Mask */
#define SMC_SETUP_NCS_WR_SETUP(value)       (SMC_SETUP_NCS_WR_SETUP_Msk & ((value) << SMC_SETUP_NCS_WR_SETUP_Pos))
#define SMC_SETUP_NRD_SETUP_Pos             16                                             /**< (SMC_SETUP) NRD Setup Length Position */
#define SMC_SETUP_NRD_SETUP_Msk             (0x3FU << SMC_SETUP_NRD_SETUP_Pos)             /**< (SMC_SETUP) NRD Setup Length Mask */
#define SMC_SETUP_NRD_SETUP(value)          (SMC_SETUP_NRD_SETUP_Msk & ((value) << SMC_SETUP_NRD_SETUP_Pos))
#define SMC_SETUP_NCS_RD_SETUP_Pos          24                                             /**< (SMC_SETUP) NCS Setup Length in READ Access Position */
#define SMC_SETUP_NCS_RD_SETUP_Msk          (0x3FU << SMC_SETUP_NCS_RD_SETUP_Pos)          /**< (SMC_SETUP) NCS Setup Length in READ Access Mask */
#define SMC_SETUP_NCS_RD_SETUP(value)       (SMC_SETUP_NCS_RD_SETUP_Msk & ((value) << SMC_SETUP_NCS_RD_SETUP_Pos))
#define SMC_SETUP_MASK                      (0x3F3F3F3FU)                                  /**< \deprecated (SMC_SETUP) Register MASK  (Use SMC_SETUP_Msk instead)  */
#define SMC_SETUP_Msk                       (0x3F3F3F3FU)                                  /**< (SMC_SETUP) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- SMC_PULSE : (SMC Offset: 0x04) (R/W 32) SMC Pulse Register (CS_number = 0) -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t NWE_PULSE:7;               /**< bit:   0..6  NWE Pulse Length                         */
    uint32_t :1;                        /**< bit:      7  Reserved */
    uint32_t NCS_WR_PULSE:7;            /**< bit:  8..14  NCS Pulse Length in WRITE Access         */
    uint32_t :1;                        /**< bit:     15  Reserved */
    uint32_t NRD_PULSE:7;               /**< bit: 16..22  NRD Pulse Length                         */
    uint32_t :1;                        /**< bit:     23  Reserved */
    uint32_t NCS_RD_PULSE:7;            /**< bit: 24..30  NCS Pulse Length in READ Access          */
    uint32_t :1;                        /**< bit:     31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} SMC_PULSE_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define SMC_PULSE_OFFSET                    (0x04)                                        /**<  (SMC_PULSE) SMC Pulse Register (CS_number = 0)  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define SMC_PULSE_NWE_PULSE_Pos             0                                              /**< (SMC_PULSE) NWE Pulse Length Position */
#define SMC_PULSE_NWE_PULSE_Msk             (0x7FU << SMC_PULSE_NWE_PULSE_Pos)             /**< (SMC_PULSE) NWE Pulse Length Mask */
#define SMC_PULSE_NWE_PULSE(value)          (SMC_PULSE_NWE_PULSE_Msk & ((value) << SMC_PULSE_NWE_PULSE_Pos))
#define SMC_PULSE_NCS_WR_PULSE_Pos          8                                              /**< (SMC_PULSE) NCS Pulse Length in WRITE Access Position */
#define SMC_PULSE_NCS_WR_PULSE_Msk          (0x7FU << SMC_PULSE_NCS_WR_PULSE_Pos)          /**< (SMC_PULSE) NCS Pulse Length in WRITE Access Mask */
#define SMC_PULSE_NCS_WR_PULSE(value)       (SMC_PULSE_NCS_WR_PULSE_Msk & ((value) << SMC_PULSE_NCS_WR_PULSE_Pos))
#define SMC_PULSE_NRD_PULSE_Pos             16                                             /**< (SMC_PULSE) NRD Pulse Length Position */
#define SMC_PULSE_NRD_PULSE_Msk             (0x7FU << SMC_PULSE_NRD_PULSE_Pos)             /**< (SMC_PULSE) NRD Pulse Length Mask */
#define SMC_PULSE_NRD_PULSE(value)          (SMC_PULSE_NRD_PULSE_Msk & ((value) << SMC_PULSE_NRD_PULSE_Pos))
#define SMC_PULSE_NCS_RD_PULSE_Pos          24                                             /**< (SMC_PULSE) NCS Pulse Length in READ Access Position */
#define SMC_PULSE_NCS_RD_PULSE_Msk          (0x7FU << SMC_PULSE_NCS_RD_PULSE_Pos)          /**< (SMC_PULSE) NCS Pulse Length in READ Access Mask */
#define SMC_PULSE_NCS_RD_PULSE(value)       (SMC_PULSE_NCS_RD_PULSE_Msk & ((value) << SMC_PULSE_NCS_RD_PULSE_Pos))
#define SMC_PULSE_MASK                      (0x7F7F7F7FU)                                  /**< \deprecated (SMC_PULSE) Register MASK  (Use SMC_PULSE_Msk instead)  */
#define SMC_PULSE_Msk                       (0x7F7F7F7FU)                                  /**< (SMC_PULSE) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- SMC_CYCLE : (SMC Offset: 0x08) (R/W 32) SMC Cycle Register (CS_number = 0) -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t NWE_CYCLE:9;               /**< bit:   0..8  Total Write Cycle Length                 */
    uint32_t :7;                        /**< bit:  9..15  Reserved */
    uint32_t NRD_CYCLE:9;               /**< bit: 16..24  Total Read Cycle Length                  */
    uint32_t :7;                        /**< bit: 25..31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} SMC_CYCLE_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define SMC_CYCLE_OFFSET                    (0x08)                                        /**<  (SMC_CYCLE) SMC Cycle Register (CS_number = 0)  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define SMC_CYCLE_NWE_CYCLE_Pos             0                                              /**< (SMC_CYCLE) Total Write Cycle Length Position */
#define SMC_CYCLE_NWE_CYCLE_Msk             (0x1FFU << SMC_CYCLE_NWE_CYCLE_Pos)            /**< (SMC_CYCLE) Total Write Cycle Length Mask */
#define SMC_CYCLE_NWE_CYCLE(value)          (SMC_CYCLE_NWE_CYCLE_Msk & ((value) << SMC_CYCLE_NWE_CYCLE_Pos))
#define SMC_CYCLE_NRD_CYCLE_Pos             16                                             /**< (SMC_CYCLE) Total Read Cycle Length Position */
#define SMC_CYCLE_NRD_CYCLE_Msk             (0x1FFU << SMC_CYCLE_NRD_CYCLE_Pos)            /**< (SMC_CYCLE) Total Read Cycle Length Mask */
#define SMC_CYCLE_NRD_CYCLE(value)          (SMC_CYCLE_NRD_CYCLE_Msk & ((value) << SMC_CYCLE_NRD_CYCLE_Pos))
#define SMC_CYCLE_MASK                      (0x1FF01FFU)                                   /**< \deprecated (SMC_CYCLE) Register MASK  (Use SMC_CYCLE_Msk instead)  */
#define SMC_CYCLE_Msk                       (0x1FF01FFU)                                   /**< (SMC_CYCLE) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- SMC_MODE : (SMC Offset: 0x0c) (R/W 32) SMC MODE Register (CS_number = 0) -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t READ_MODE:1;               /**< bit:      0  Read Mode                                */
    uint32_t WRITE_MODE:1;              /**< bit:      1  Write Mode                               */
    uint32_t :2;                        /**< bit:   2..3  Reserved */
    uint32_t EXNW_MODE:2;               /**< bit:   4..5  NWAIT Mode                               */
    uint32_t :2;                        /**< bit:   6..7  Reserved */
    uint32_t BAT:1;                     /**< bit:      8  Byte Access Type                         */
    uint32_t :3;                        /**< bit:  9..11  Reserved */
    uint32_t DBW:1;                     /**< bit:     12  Data Bus Width                           */
    uint32_t :3;                        /**< bit: 13..15  Reserved */
    uint32_t TDF_CYCLES:4;              /**< bit: 16..19  Data Float Time                          */
    uint32_t TDF_MODE:1;                /**< bit:     20  TDF Optimization                         */
    uint32_t :3;                        /**< bit: 21..23  Reserved */
    uint32_t PMEN:1;                    /**< bit:     24  Page Mode Enabled                        */
    uint32_t :3;                        /**< bit: 25..27  Reserved */
    uint32_t PS:2;                      /**< bit: 28..29  Page Size                                */
    uint32_t :2;                        /**< bit: 30..31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} SMC_MODE_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define SMC_MODE_OFFSET                     (0x0C)                                        /**<  (SMC_MODE) SMC MODE Register (CS_number = 0)  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define SMC_MODE_READ_MODE_Pos              0                                              /**< (SMC_MODE) Read Mode Position */
#define SMC_MODE_READ_MODE_Msk              (0x1U << SMC_MODE_READ_MODE_Pos)               /**< (SMC_MODE) Read Mode Mask */
#define SMC_MODE_READ_MODE                  SMC_MODE_READ_MODE_Msk                         /**< \deprecated Old style mask definition for 1 bit bitfield. Use SMC_MODE_READ_MODE_Msk instead */
#define SMC_MODE_WRITE_MODE_Pos             1                                              /**< (SMC_MODE) Write Mode Position */
#define SMC_MODE_WRITE_MODE_Msk             (0x1U << SMC_MODE_WRITE_MODE_Pos)              /**< (SMC_MODE) Write Mode Mask */
#define SMC_MODE_WRITE_MODE                 SMC_MODE_WRITE_MODE_Msk                        /**< \deprecated Old style mask definition for 1 bit bitfield. Use SMC_MODE_WRITE_MODE_Msk instead */
#define SMC_MODE_EXNW_MODE_Pos              4                                              /**< (SMC_MODE) NWAIT Mode Position */
#define SMC_MODE_EXNW_MODE_Msk              (0x3U << SMC_MODE_EXNW_MODE_Pos)               /**< (SMC_MODE) NWAIT Mode Mask */
#define SMC_MODE_EXNW_MODE(value)           (SMC_MODE_EXNW_MODE_Msk & ((value) << SMC_MODE_EXNW_MODE_Pos))
#define   SMC_MODE_EXNW_MODE_DISABLED_Val   (0x0U)                                         /**< (SMC_MODE) Disabled  */
#define   SMC_MODE_EXNW_MODE_FROZEN_Val     (0x2U)                                         /**< (SMC_MODE) Frozen Mode  */
#define   SMC_MODE_EXNW_MODE_READY_Val      (0x3U)                                         /**< (SMC_MODE) Ready Mode  */
#define SMC_MODE_EXNW_MODE_DISABLED         (SMC_MODE_EXNW_MODE_DISABLED_Val << SMC_MODE_EXNW_MODE_Pos)  /**< (SMC_MODE) Disabled Position  */
#define SMC_MODE_EXNW_MODE_FROZEN           (SMC_MODE_EXNW_MODE_FROZEN_Val << SMC_MODE_EXNW_MODE_Pos)  /**< (SMC_MODE) Frozen Mode Position  */
#define SMC_MODE_EXNW_MODE_READY            (SMC_MODE_EXNW_MODE_READY_Val << SMC_MODE_EXNW_MODE_Pos)  /**< (SMC_MODE) Ready Mode Position  */
#define SMC_MODE_BAT_Pos                    8                                              /**< (SMC_MODE) Byte Access Type Position */
#define SMC_MODE_BAT_Msk                    (0x1U << SMC_MODE_BAT_Pos)                     /**< (SMC_MODE) Byte Access Type Mask */
#define SMC_MODE_BAT                        SMC_MODE_BAT_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use SMC_MODE_BAT_Msk instead */
#define   SMC_MODE_BAT_BYTE_SELECT_Val      (0x0U)                                         /**< (SMC_MODE) Byte select access type:- Write operation is controlled using NCS, NWE, NBS0, NBS1.- Read operation is controlled using NCS, NRD, NBS0, NBS1.  */
#define   SMC_MODE_BAT_BYTE_WRITE_Val       (0x1U)                                         /**< (SMC_MODE) Byte write access type:- Write operation is controlled using NCS, NWR0, NWR1.- Read operation is controlled using NCS and NRD.  */
#define SMC_MODE_BAT_BYTE_SELECT            (SMC_MODE_BAT_BYTE_SELECT_Val << SMC_MODE_BAT_Pos)  /**< (SMC_MODE) Byte select access type:- Write operation is controlled using NCS, NWE, NBS0, NBS1.- Read operation is controlled using NCS, NRD, NBS0, NBS1. Position  */
#define SMC_MODE_BAT_BYTE_WRITE             (SMC_MODE_BAT_BYTE_WRITE_Val << SMC_MODE_BAT_Pos)  /**< (SMC_MODE) Byte write access type:- Write operation is controlled using NCS, NWR0, NWR1.- Read operation is controlled using NCS and NRD. Position  */
#define SMC_MODE_DBW_Pos                    12                                             /**< (SMC_MODE) Data Bus Width Position */
#define SMC_MODE_DBW_Msk                    (0x1U << SMC_MODE_DBW_Pos)                     /**< (SMC_MODE) Data Bus Width Mask */
#define SMC_MODE_DBW                        SMC_MODE_DBW_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use SMC_MODE_DBW_Msk instead */
#define   SMC_MODE_DBW_8_BIT_Val            (0x0U)                                         /**< (SMC_MODE) 8-bit Data Bus  */
#define   SMC_MODE_DBW_16_BIT_Val           (0x1U)                                         /**< (SMC_MODE) 16-bit Data Bus  */
#define SMC_MODE_DBW_8_BIT                  (SMC_MODE_DBW_8_BIT_Val << SMC_MODE_DBW_Pos)   /**< (SMC_MODE) 8-bit Data Bus Position  */
#define SMC_MODE_DBW_16_BIT                 (SMC_MODE_DBW_16_BIT_Val << SMC_MODE_DBW_Pos)  /**< (SMC_MODE) 16-bit Data Bus Position  */
#define SMC_MODE_TDF_CYCLES_Pos             16                                             /**< (SMC_MODE) Data Float Time Position */
#define SMC_MODE_TDF_CYCLES_Msk             (0xFU << SMC_MODE_TDF_CYCLES_Pos)              /**< (SMC_MODE) Data Float Time Mask */
#define SMC_MODE_TDF_CYCLES(value)          (SMC_MODE_TDF_CYCLES_Msk & ((value) << SMC_MODE_TDF_CYCLES_Pos))
#define SMC_MODE_TDF_MODE_Pos               20                                             /**< (SMC_MODE) TDF Optimization Position */
#define SMC_MODE_TDF_MODE_Msk               (0x1U << SMC_MODE_TDF_MODE_Pos)                /**< (SMC_MODE) TDF Optimization Mask */
#define SMC_MODE_TDF_MODE                   SMC_MODE_TDF_MODE_Msk                          /**< \deprecated Old style mask definition for 1 bit bitfield. Use SMC_MODE_TDF_MODE_Msk instead */
#define SMC_MODE_PMEN_Pos                   24                                             /**< (SMC_MODE) Page Mode Enabled Position */
#define SMC_MODE_PMEN_Msk                   (0x1U << SMC_MODE_PMEN_Pos)                    /**< (SMC_MODE) Page Mode Enabled Mask */
#define SMC_MODE_PMEN                       SMC_MODE_PMEN_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use SMC_MODE_PMEN_Msk instead */
#define SMC_MODE_PS_Pos                     28                                             /**< (SMC_MODE) Page Size Position */
#define SMC_MODE_PS_Msk                     (0x3U << SMC_MODE_PS_Pos)                      /**< (SMC_MODE) Page Size Mask */
#define SMC_MODE_PS(value)                  (SMC_MODE_PS_Msk & ((value) << SMC_MODE_PS_Pos))
#define   SMC_MODE_PS_4_BYTE_Val            (0x0U)                                         /**< (SMC_MODE) 4-byte page  */
#define   SMC_MODE_PS_8_BYTE_Val            (0x1U)                                         /**< (SMC_MODE) 8-byte page  */
#define   SMC_MODE_PS_16_BYTE_Val           (0x2U)                                         /**< (SMC_MODE) 16-byte page  */
#define   SMC_MODE_PS_32_BYTE_Val           (0x3U)                                         /**< (SMC_MODE) 32-byte page  */
#define SMC_MODE_PS_4_BYTE                  (SMC_MODE_PS_4_BYTE_Val << SMC_MODE_PS_Pos)    /**< (SMC_MODE) 4-byte page Position  */
#define SMC_MODE_PS_8_BYTE                  (SMC_MODE_PS_8_BYTE_Val << SMC_MODE_PS_Pos)    /**< (SMC_MODE) 8-byte page Position  */
#define SMC_MODE_PS_16_BYTE                 (SMC_MODE_PS_16_BYTE_Val << SMC_MODE_PS_Pos)   /**< (SMC_MODE) 16-byte page Position  */
#define SMC_MODE_PS_32_BYTE                 (SMC_MODE_PS_32_BYTE_Val << SMC_MODE_PS_Pos)   /**< (SMC_MODE) 32-byte page Position  */
#define SMC_MODE_MASK                       (0x311F1133U)                                  /**< \deprecated (SMC_MODE) Register MASK  (Use SMC_MODE_Msk instead)  */
#define SMC_MODE_Msk                        (0x311F1133U)                                  /**< (SMC_MODE) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- SMC_OCMS : (SMC Offset: 0x80) (R/W 32) SMC OCMS MODE Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t SMSE:1;                    /**< bit:      0  Static Memory Controller Scrambling Enable */
    uint32_t :7;                        /**< bit:   1..7  Reserved */
    uint32_t CS0SE:1;                   /**< bit:      8  Chip Select 0 Scrambling Enable          */
    uint32_t CS1SE:1;                   /**< bit:      9  Chip Select 1 Scrambling Enable          */
    uint32_t CS2SE:1;                   /**< bit:     10  Chip Select 2 Scrambling Enable          */
    uint32_t CS3SE:1;                   /**< bit:     11  Chip Select 3 Scrambling Enable          */
    uint32_t :20;                       /**< bit: 12..31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} SMC_OCMS_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define SMC_OCMS_OFFSET                     (0x80)                                        /**<  (SMC_OCMS) SMC OCMS MODE Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define SMC_OCMS_SMSE_Pos                   0                                              /**< (SMC_OCMS) Static Memory Controller Scrambling Enable Position */
#define SMC_OCMS_SMSE_Msk                   (0x1U << SMC_OCMS_SMSE_Pos)                    /**< (SMC_OCMS) Static Memory Controller Scrambling Enable Mask */
#define SMC_OCMS_SMSE                       SMC_OCMS_SMSE_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use SMC_OCMS_SMSE_Msk instead */
#define SMC_OCMS_CS0SE_Pos                  8                                              /**< (SMC_OCMS) Chip Select 0 Scrambling Enable Position */
#define SMC_OCMS_CS0SE_Msk                  (0x1U << SMC_OCMS_CS0SE_Pos)                   /**< (SMC_OCMS) Chip Select 0 Scrambling Enable Mask */
#define SMC_OCMS_CS0SE                      SMC_OCMS_CS0SE_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use SMC_OCMS_CS0SE_Msk instead */
#define SMC_OCMS_CS1SE_Pos                  9                                              /**< (SMC_OCMS) Chip Select 1 Scrambling Enable Position */
#define SMC_OCMS_CS1SE_Msk                  (0x1U << SMC_OCMS_CS1SE_Pos)                   /**< (SMC_OCMS) Chip Select 1 Scrambling Enable Mask */
#define SMC_OCMS_CS1SE                      SMC_OCMS_CS1SE_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use SMC_OCMS_CS1SE_Msk instead */
#define SMC_OCMS_CS2SE_Pos                  10                                             /**< (SMC_OCMS) Chip Select 2 Scrambling Enable Position */
#define SMC_OCMS_CS2SE_Msk                  (0x1U << SMC_OCMS_CS2SE_Pos)                   /**< (SMC_OCMS) Chip Select 2 Scrambling Enable Mask */
#define SMC_OCMS_CS2SE                      SMC_OCMS_CS2SE_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use SMC_OCMS_CS2SE_Msk instead */
#define SMC_OCMS_CS3SE_Pos                  11                                             /**< (SMC_OCMS) Chip Select 3 Scrambling Enable Position */
#define SMC_OCMS_CS3SE_Msk                  (0x1U << SMC_OCMS_CS3SE_Pos)                   /**< (SMC_OCMS) Chip Select 3 Scrambling Enable Mask */
#define SMC_OCMS_CS3SE                      SMC_OCMS_CS3SE_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use SMC_OCMS_CS3SE_Msk instead */
#define SMC_OCMS_MASK                       (0xF01U)                                       /**< \deprecated (SMC_OCMS) Register MASK  (Use SMC_OCMS_Msk instead)  */
#define SMC_OCMS_Msk                        (0xF01U)                                       /**< (SMC_OCMS) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- SMC_KEY1 : (SMC Offset: 0x84) (/W 32) SMC OCMS KEY1 Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t KEY1:32;                   /**< bit:  0..31  Off Chip Memory Scrambling (OCMS) Key Part 1 */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} SMC_KEY1_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define SMC_KEY1_OFFSET                     (0x84)                                        /**<  (SMC_KEY1) SMC OCMS KEY1 Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define SMC_KEY1_KEY1_Pos                   0                                              /**< (SMC_KEY1) Off Chip Memory Scrambling (OCMS) Key Part 1 Position */
#define SMC_KEY1_KEY1_Msk                   (0xFFFFFFFFU << SMC_KEY1_KEY1_Pos)             /**< (SMC_KEY1) Off Chip Memory Scrambling (OCMS) Key Part 1 Mask */
#define SMC_KEY1_KEY1(value)                (SMC_KEY1_KEY1_Msk & ((value) << SMC_KEY1_KEY1_Pos))
#define SMC_KEY1_MASK                       (0xFFFFFFFFU)                                  /**< \deprecated (SMC_KEY1) Register MASK  (Use SMC_KEY1_Msk instead)  */
#define SMC_KEY1_Msk                        (0xFFFFFFFFU)                                  /**< (SMC_KEY1) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- SMC_KEY2 : (SMC Offset: 0x88) (/W 32) SMC OCMS KEY2 Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t KEY2:32;                   /**< bit:  0..31  Off Chip Memory Scrambling (OCMS) Key Part 2 */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} SMC_KEY2_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define SMC_KEY2_OFFSET                     (0x88)                                        /**<  (SMC_KEY2) SMC OCMS KEY2 Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define SMC_KEY2_KEY2_Pos                   0                                              /**< (SMC_KEY2) Off Chip Memory Scrambling (OCMS) Key Part 2 Position */
#define SMC_KEY2_KEY2_Msk                   (0xFFFFFFFFU << SMC_KEY2_KEY2_Pos)             /**< (SMC_KEY2) Off Chip Memory Scrambling (OCMS) Key Part 2 Mask */
#define SMC_KEY2_KEY2(value)                (SMC_KEY2_KEY2_Msk & ((value) << SMC_KEY2_KEY2_Pos))
#define SMC_KEY2_MASK                       (0xFFFFFFFFU)                                  /**< \deprecated (SMC_KEY2) Register MASK  (Use SMC_KEY2_Msk instead)  */
#define SMC_KEY2_Msk                        (0xFFFFFFFFU)                                  /**< (SMC_KEY2) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- SMC_WPMR : (SMC Offset: 0xe4) (R/W 32) SMC Write Protection Mode Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t WPEN:1;                    /**< bit:      0  Write Protect Enable                     */
    uint32_t :7;                        /**< bit:   1..7  Reserved */
    uint32_t WPKEY:24;                  /**< bit:  8..31  Write Protection Key                     */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} SMC_WPMR_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define SMC_WPMR_OFFSET                     (0xE4)                                        /**<  (SMC_WPMR) SMC Write Protection Mode Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define SMC_WPMR_WPEN_Pos                   0                                              /**< (SMC_WPMR) Write Protect Enable Position */
#define SMC_WPMR_WPEN_Msk                   (0x1U << SMC_WPMR_WPEN_Pos)                    /**< (SMC_WPMR) Write Protect Enable Mask */
#define SMC_WPMR_WPEN                       SMC_WPMR_WPEN_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use SMC_WPMR_WPEN_Msk instead */
#define SMC_WPMR_WPKEY_Pos                  8                                              /**< (SMC_WPMR) Write Protection Key Position */
#define SMC_WPMR_WPKEY_Msk                  (0xFFFFFFU << SMC_WPMR_WPKEY_Pos)              /**< (SMC_WPMR) Write Protection Key Mask */
#define SMC_WPMR_WPKEY(value)               (SMC_WPMR_WPKEY_Msk & ((value) << SMC_WPMR_WPKEY_Pos))
#define   SMC_WPMR_WPKEY_PASSWD_Val         (0x534D43U)                                    /**< (SMC_WPMR) Writing any other value in this field aborts the write operation of the WPEN bit. Always reads as 0.  */
#define SMC_WPMR_WPKEY_PASSWD               (SMC_WPMR_WPKEY_PASSWD_Val << SMC_WPMR_WPKEY_Pos)  /**< (SMC_WPMR) Writing any other value in this field aborts the write operation of the WPEN bit. Always reads as 0. Position  */
#define SMC_WPMR_MASK                       (0xFFFFFF01U)                                  /**< \deprecated (SMC_WPMR) Register MASK  (Use SMC_WPMR_Msk instead)  */
#define SMC_WPMR_Msk                        (0xFFFFFF01U)                                  /**< (SMC_WPMR) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- SMC_WPSR : (SMC Offset: 0xe8) (R/ 32) SMC Write Protection Status Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t WPVS:1;                    /**< bit:      0  Write Protection Violation Status        */
    uint32_t :7;                        /**< bit:   1..7  Reserved */
    uint32_t WPVSRC:16;                 /**< bit:  8..23  Write Protection Violation Source        */
    uint32_t :8;                        /**< bit: 24..31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} SMC_WPSR_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define SMC_WPSR_OFFSET                     (0xE8)                                        /**<  (SMC_WPSR) SMC Write Protection Status Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define SMC_WPSR_WPVS_Pos                   0                                              /**< (SMC_WPSR) Write Protection Violation Status Position */
#define SMC_WPSR_WPVS_Msk                   (0x1U << SMC_WPSR_WPVS_Pos)                    /**< (SMC_WPSR) Write Protection Violation Status Mask */
#define SMC_WPSR_WPVS                       SMC_WPSR_WPVS_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use SMC_WPSR_WPVS_Msk instead */
#define SMC_WPSR_WPVSRC_Pos                 8                                              /**< (SMC_WPSR) Write Protection Violation Source Position */
#define SMC_WPSR_WPVSRC_Msk                 (0xFFFFU << SMC_WPSR_WPVSRC_Pos)               /**< (SMC_WPSR) Write Protection Violation Source Mask */
#define SMC_WPSR_WPVSRC(value)              (SMC_WPSR_WPVSRC_Msk & ((value) << SMC_WPSR_WPVSRC_Pos))
#define SMC_WPSR_MASK                       (0xFFFF01U)                                    /**< \deprecated (SMC_WPSR) Register MASK  (Use SMC_WPSR_Msk instead)  */
#define SMC_WPSR_Msk                        (0xFFFF01U)                                    /**< (SMC_WPSR) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
#if COMPONENT_TYPEDEF_STYLE == 'R'
#define SMCCSNUMBER_NUMBER 4
/** \brief SMC_CS_NUMBER hardware registers */
typedef struct {  
  __IO uint32_t SMC_SETUP;      /**< (SMC_CS_NUMBER Offset: 0x00) SMC Setup Register (CS_number = 0) */
  __IO uint32_t SMC_PULSE;      /**< (SMC_CS_NUMBER Offset: 0x04) SMC Pulse Register (CS_number = 0) */
  __IO uint32_t SMC_CYCLE;      /**< (SMC_CS_NUMBER Offset: 0x08) SMC Cycle Register (CS_number = 0) */
  __IO uint32_t SMC_MODE;       /**< (SMC_CS_NUMBER Offset: 0x0C) SMC MODE Register (CS_number = 0) */
} SmcCsNumber;

/** \brief SMC hardware registers */
typedef struct {  
       SmcCsNumber SMC_CS_NUMBER[SMCCSNUMBER_NUMBER]; /**< Offset: 0x00 SMC Setup Register (CS_number = 0) */
  __I  uint32_t Reserved1[16];
  __IO uint32_t SMC_OCMS;       /**< (SMC Offset: 0x80) SMC OCMS MODE Register */
  __O  uint32_t SMC_KEY1;       /**< (SMC Offset: 0x84) SMC OCMS KEY1 Register */
  __O  uint32_t SMC_KEY2;       /**< (SMC Offset: 0x88) SMC OCMS KEY2 Register */
  __I  uint32_t Reserved2[22];
  __IO uint32_t SMC_WPMR;       /**< (SMC Offset: 0xE4) SMC Write Protection Mode Register */
  __I  uint32_t SMC_WPSR;       /**< (SMC Offset: 0xE8) SMC Write Protection Status Register */
} Smc;

#elif COMPONENT_TYPEDEF_STYLE == 'N'
/** \brief SMC_CS_NUMBER hardware registers */
typedef struct {  
  __IO SMC_SETUP_Type                 SMC_SETUP;      /**< Offset: 0x00 (R/W  32) SMC Setup Register (CS_number = 0) */
  __IO SMC_PULSE_Type                 SMC_PULSE;      /**< Offset: 0x04 (R/W  32) SMC Pulse Register (CS_number = 0) */
  __IO SMC_CYCLE_Type                 SMC_CYCLE;      /**< Offset: 0x08 (R/W  32) SMC Cycle Register (CS_number = 0) */
  __IO SMC_MODE_Type                  SMC_MODE;       /**< Offset: 0x0C (R/W  32) SMC MODE Register (CS_number = 0) */
} SmcCsNumber;

/** \brief SMC hardware registers */
typedef struct {  
       SmcCsNumber                    SMC_CS_NUMBER[4]; /**< Offset: 0x00 SMC Setup Register (CS_number = 0) */
       RoReg8                         Reserved1[0x40];
  __IO SMC_OCMS_Type                  SMC_OCMS;       /**< Offset: 0x80 (R/W  32) SMC OCMS MODE Register */
  __O  SMC_KEY1_Type                  SMC_KEY1;       /**< Offset: 0x84 ( /W  32) SMC OCMS KEY1 Register */
  __O  SMC_KEY2_Type                  SMC_KEY2;       /**< Offset: 0x88 ( /W  32) SMC OCMS KEY2 Register */
       RoReg8                         Reserved2[0x58];
  __IO SMC_WPMR_Type                  SMC_WPMR;       /**< Offset: 0xE4 (R/W  32) SMC Write Protection Mode Register */
  __I  SMC_WPSR_Type                  SMC_WPSR;       /**< Offset: 0xE8 (R/   32) SMC Write Protection Status Register */
} Smc;

#else /* COMPONENT_TYPEDEF_STYLE */
#error Unknown component typedef style
#endif /* COMPONENT_TYPEDEF_STYLE */

#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/** @}  end of Static Memory Controller */

#endif /* _SAME70_SMC_COMPONENT_H_ */
