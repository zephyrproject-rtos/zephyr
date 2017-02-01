/**
 * \file
 *
 * \brief Component description for EFC
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

#ifndef _SAME70_EFC_COMPONENT_H_
#define _SAME70_EFC_COMPONENT_H_
#define _SAME70_EFC_COMPONENT_         /**< \deprecated  Backward compatibility for ASF */

/** \addtogroup SAME70_EFC Embedded Flash Controller
 *  @{
 */
/* ========================================================================== */
/**  SOFTWARE API DEFINITION FOR EFC */
/* ========================================================================== */
#ifndef COMPONENT_TYPEDEF_STYLE
  #define COMPONENT_TYPEDEF_STYLE 'R'  /**< Defines default style of typedefs for the component header files ('R' = RFO, 'N' = NTO)*/
#endif

#define EFC_6450                       /**< (EFC) Module ID */
#define REV_EFC X                      /**< (EFC) Module revision */

/* -------- EEFC_FMR : (EFC Offset: 0x00) (R/W 32) EEFC Flash Mode Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t FRDY:1;                    /**< bit:      0  Flash Ready Interrupt Enable             */
    uint32_t :7;                        /**< bit:   1..7  Reserved */
    uint32_t FWS:4;                     /**< bit:  8..11  Flash Wait State                         */
    uint32_t :4;                        /**< bit: 12..15  Reserved */
    uint32_t SCOD:1;                    /**< bit:     16  Sequential Code Optimization Disable     */
    uint32_t :9;                        /**< bit: 17..25  Reserved */
    uint32_t CLOE:1;                    /**< bit:     26  Code Loop Optimization Enable            */
    uint32_t :5;                        /**< bit: 27..31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} EEFC_FMR_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define EEFC_FMR_OFFSET                     (0x00)                                        /**<  (EEFC_FMR) EEFC Flash Mode Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define EEFC_FMR_FRDY_Pos                   0                                              /**< (EEFC_FMR) Flash Ready Interrupt Enable Position */
#define EEFC_FMR_FRDY_Msk                   (0x1U << EEFC_FMR_FRDY_Pos)                    /**< (EEFC_FMR) Flash Ready Interrupt Enable Mask */
#define EEFC_FMR_FRDY                       EEFC_FMR_FRDY_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use EEFC_FMR_FRDY_Msk instead */
#define EEFC_FMR_FWS_Pos                    8                                              /**< (EEFC_FMR) Flash Wait State Position */
#define EEFC_FMR_FWS_Msk                    (0xFU << EEFC_FMR_FWS_Pos)                     /**< (EEFC_FMR) Flash Wait State Mask */
#define EEFC_FMR_FWS(value)                 (EEFC_FMR_FWS_Msk & ((value) << EEFC_FMR_FWS_Pos))
#define EEFC_FMR_SCOD_Pos                   16                                             /**< (EEFC_FMR) Sequential Code Optimization Disable Position */
#define EEFC_FMR_SCOD_Msk                   (0x1U << EEFC_FMR_SCOD_Pos)                    /**< (EEFC_FMR) Sequential Code Optimization Disable Mask */
#define EEFC_FMR_SCOD                       EEFC_FMR_SCOD_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use EEFC_FMR_SCOD_Msk instead */
#define EEFC_FMR_CLOE_Pos                   26                                             /**< (EEFC_FMR) Code Loop Optimization Enable Position */
#define EEFC_FMR_CLOE_Msk                   (0x1U << EEFC_FMR_CLOE_Pos)                    /**< (EEFC_FMR) Code Loop Optimization Enable Mask */
#define EEFC_FMR_CLOE                       EEFC_FMR_CLOE_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use EEFC_FMR_CLOE_Msk instead */
#define EEFC_FMR_MASK                       (0x4010F01U)                                   /**< \deprecated (EEFC_FMR) Register MASK  (Use EEFC_FMR_Msk instead)  */
#define EEFC_FMR_Msk                        (0x4010F01U)                                   /**< (EEFC_FMR) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- EEFC_FCR : (EFC Offset: 0x04) (/W 32) EEFC Flash Command Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t FCMD:8;                    /**< bit:   0..7  Flash Command                            */
    uint32_t FARG:16;                   /**< bit:  8..23  Flash Command Argument                   */
    uint32_t FKEY:8;                    /**< bit: 24..31  Flash Writing Protection Key             */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} EEFC_FCR_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define EEFC_FCR_OFFSET                     (0x04)                                        /**<  (EEFC_FCR) EEFC Flash Command Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define EEFC_FCR_FCMD_Pos                   0                                              /**< (EEFC_FCR) Flash Command Position */
#define EEFC_FCR_FCMD_Msk                   (0xFFU << EEFC_FCR_FCMD_Pos)                   /**< (EEFC_FCR) Flash Command Mask */
#define EEFC_FCR_FCMD(value)                (EEFC_FCR_FCMD_Msk & ((value) << EEFC_FCR_FCMD_Pos))
#define   EEFC_FCR_FCMD_GETD_Val            (0x0U)                                         /**< (EEFC_FCR) Get Flash descriptor  */
#define   EEFC_FCR_FCMD_WP_Val              (0x1U)                                         /**< (EEFC_FCR) Write page  */
#define   EEFC_FCR_FCMD_WPL_Val             (0x2U)                                         /**< (EEFC_FCR) Write page and lock  */
#define   EEFC_FCR_FCMD_EWP_Val             (0x3U)                                         /**< (EEFC_FCR) Erase page and write page  */
#define   EEFC_FCR_FCMD_EWPL_Val            (0x4U)                                         /**< (EEFC_FCR) Erase page and write page then lock  */
#define   EEFC_FCR_FCMD_EA_Val              (0x5U)                                         /**< (EEFC_FCR) Erase all  */
#define   EEFC_FCR_FCMD_EPA_Val             (0x7U)                                         /**< (EEFC_FCR) Erase pages  */
#define   EEFC_FCR_FCMD_SLB_Val             (0x8U)                                         /**< (EEFC_FCR) Set lock bit  */
#define   EEFC_FCR_FCMD_CLB_Val             (0x9U)                                         /**< (EEFC_FCR) Clear lock bit  */
#define   EEFC_FCR_FCMD_GLB_Val             (0xAU)                                         /**< (EEFC_FCR) Get lock bit  */
#define   EEFC_FCR_FCMD_SGPB_Val            (0xBU)                                         /**< (EEFC_FCR) Set GPNVM bit  */
#define   EEFC_FCR_FCMD_CGPB_Val            (0xCU)                                         /**< (EEFC_FCR) Clear GPNVM bit  */
#define   EEFC_FCR_FCMD_GGPB_Val            (0xDU)                                         /**< (EEFC_FCR) Get GPNVM bit  */
#define   EEFC_FCR_FCMD_STUI_Val            (0xEU)                                         /**< (EEFC_FCR) Start read unique identifier  */
#define   EEFC_FCR_FCMD_SPUI_Val            (0xFU)                                         /**< (EEFC_FCR) Stop read unique identifier  */
#define   EEFC_FCR_FCMD_GCALB_Val           (0x10U)                                        /**< (EEFC_FCR) Get CALIB bit  */
#define   EEFC_FCR_FCMD_ES_Val              (0x11U)                                        /**< (EEFC_FCR) Erase sector  */
#define   EEFC_FCR_FCMD_WUS_Val             (0x12U)                                        /**< (EEFC_FCR) Write user signature  */
#define   EEFC_FCR_FCMD_EUS_Val             (0x13U)                                        /**< (EEFC_FCR) Erase user signature  */
#define   EEFC_FCR_FCMD_STUS_Val            (0x14U)                                        /**< (EEFC_FCR) Start read user signature  */
#define   EEFC_FCR_FCMD_SPUS_Val            (0x15U)                                        /**< (EEFC_FCR) Stop read user signature  */
#define EEFC_FCR_FCMD_GETD                  (EEFC_FCR_FCMD_GETD_Val << EEFC_FCR_FCMD_Pos)  /**< (EEFC_FCR) Get Flash descriptor Position  */
#define EEFC_FCR_FCMD_WP                    (EEFC_FCR_FCMD_WP_Val << EEFC_FCR_FCMD_Pos)    /**< (EEFC_FCR) Write page Position  */
#define EEFC_FCR_FCMD_WPL                   (EEFC_FCR_FCMD_WPL_Val << EEFC_FCR_FCMD_Pos)   /**< (EEFC_FCR) Write page and lock Position  */
#define EEFC_FCR_FCMD_EWP                   (EEFC_FCR_FCMD_EWP_Val << EEFC_FCR_FCMD_Pos)   /**< (EEFC_FCR) Erase page and write page Position  */
#define EEFC_FCR_FCMD_EWPL                  (EEFC_FCR_FCMD_EWPL_Val << EEFC_FCR_FCMD_Pos)  /**< (EEFC_FCR) Erase page and write page then lock Position  */
#define EEFC_FCR_FCMD_EA                    (EEFC_FCR_FCMD_EA_Val << EEFC_FCR_FCMD_Pos)    /**< (EEFC_FCR) Erase all Position  */
#define EEFC_FCR_FCMD_EPA                   (EEFC_FCR_FCMD_EPA_Val << EEFC_FCR_FCMD_Pos)   /**< (EEFC_FCR) Erase pages Position  */
#define EEFC_FCR_FCMD_SLB                   (EEFC_FCR_FCMD_SLB_Val << EEFC_FCR_FCMD_Pos)   /**< (EEFC_FCR) Set lock bit Position  */
#define EEFC_FCR_FCMD_CLB                   (EEFC_FCR_FCMD_CLB_Val << EEFC_FCR_FCMD_Pos)   /**< (EEFC_FCR) Clear lock bit Position  */
#define EEFC_FCR_FCMD_GLB                   (EEFC_FCR_FCMD_GLB_Val << EEFC_FCR_FCMD_Pos)   /**< (EEFC_FCR) Get lock bit Position  */
#define EEFC_FCR_FCMD_SGPB                  (EEFC_FCR_FCMD_SGPB_Val << EEFC_FCR_FCMD_Pos)  /**< (EEFC_FCR) Set GPNVM bit Position  */
#define EEFC_FCR_FCMD_CGPB                  (EEFC_FCR_FCMD_CGPB_Val << EEFC_FCR_FCMD_Pos)  /**< (EEFC_FCR) Clear GPNVM bit Position  */
#define EEFC_FCR_FCMD_GGPB                  (EEFC_FCR_FCMD_GGPB_Val << EEFC_FCR_FCMD_Pos)  /**< (EEFC_FCR) Get GPNVM bit Position  */
#define EEFC_FCR_FCMD_STUI                  (EEFC_FCR_FCMD_STUI_Val << EEFC_FCR_FCMD_Pos)  /**< (EEFC_FCR) Start read unique identifier Position  */
#define EEFC_FCR_FCMD_SPUI                  (EEFC_FCR_FCMD_SPUI_Val << EEFC_FCR_FCMD_Pos)  /**< (EEFC_FCR) Stop read unique identifier Position  */
#define EEFC_FCR_FCMD_GCALB                 (EEFC_FCR_FCMD_GCALB_Val << EEFC_FCR_FCMD_Pos)  /**< (EEFC_FCR) Get CALIB bit Position  */
#define EEFC_FCR_FCMD_ES                    (EEFC_FCR_FCMD_ES_Val << EEFC_FCR_FCMD_Pos)    /**< (EEFC_FCR) Erase sector Position  */
#define EEFC_FCR_FCMD_WUS                   (EEFC_FCR_FCMD_WUS_Val << EEFC_FCR_FCMD_Pos)   /**< (EEFC_FCR) Write user signature Position  */
#define EEFC_FCR_FCMD_EUS                   (EEFC_FCR_FCMD_EUS_Val << EEFC_FCR_FCMD_Pos)   /**< (EEFC_FCR) Erase user signature Position  */
#define EEFC_FCR_FCMD_STUS                  (EEFC_FCR_FCMD_STUS_Val << EEFC_FCR_FCMD_Pos)  /**< (EEFC_FCR) Start read user signature Position  */
#define EEFC_FCR_FCMD_SPUS                  (EEFC_FCR_FCMD_SPUS_Val << EEFC_FCR_FCMD_Pos)  /**< (EEFC_FCR) Stop read user signature Position  */
#define EEFC_FCR_FARG_Pos                   8                                              /**< (EEFC_FCR) Flash Command Argument Position */
#define EEFC_FCR_FARG_Msk                   (0xFFFFU << EEFC_FCR_FARG_Pos)                 /**< (EEFC_FCR) Flash Command Argument Mask */
#define EEFC_FCR_FARG(value)                (EEFC_FCR_FARG_Msk & ((value) << EEFC_FCR_FARG_Pos))
#define EEFC_FCR_FKEY_Pos                   24                                             /**< (EEFC_FCR) Flash Writing Protection Key Position */
#define EEFC_FCR_FKEY_Msk                   (0xFFU << EEFC_FCR_FKEY_Pos)                   /**< (EEFC_FCR) Flash Writing Protection Key Mask */
#define EEFC_FCR_FKEY(value)                (EEFC_FCR_FKEY_Msk & ((value) << EEFC_FCR_FKEY_Pos))
#define   EEFC_FCR_FKEY_PASSWD_Val          (0x5AU)                                        /**< (EEFC_FCR) The 0x5A value enables the command defined by the bits of the register. If the field is written with a different value, the write is not performed and no action is started.  */
#define EEFC_FCR_FKEY_PASSWD                (EEFC_FCR_FKEY_PASSWD_Val << EEFC_FCR_FKEY_Pos)  /**< (EEFC_FCR) The 0x5A value enables the command defined by the bits of the register. If the field is written with a different value, the write is not performed and no action is started. Position  */
#define EEFC_FCR_MASK                       (0xFFFFFFFFU)                                  /**< \deprecated (EEFC_FCR) Register MASK  (Use EEFC_FCR_Msk instead)  */
#define EEFC_FCR_Msk                        (0xFFFFFFFFU)                                  /**< (EEFC_FCR) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- EEFC_FSR : (EFC Offset: 0x08) (R/ 32) EEFC Flash Status Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t FRDY:1;                    /**< bit:      0  Flash Ready Status (cleared when Flash is busy) */
    uint32_t FCMDE:1;                   /**< bit:      1  Flash Command Error Status (cleared on read or by writing EEFC_FCR) */
    uint32_t FLOCKE:1;                  /**< bit:      2  Flash Lock Error Status (cleared on read) */
    uint32_t FLERR:1;                   /**< bit:      3  Flash Error Status (cleared when a programming operation starts) */
    uint32_t :12;                       /**< bit:  4..15  Reserved */
    uint32_t UECCELSB:1;                /**< bit:     16  Unique ECC Error on LSB Part of the Memory Flash Data Bus (cleared on read) */
    uint32_t MECCELSB:1;                /**< bit:     17  Multiple ECC Error on LSB Part of the Memory Flash Data Bus (cleared on read) */
    uint32_t UECCEMSB:1;                /**< bit:     18  Unique ECC Error on MSB Part of the Memory Flash Data Bus (cleared on read) */
    uint32_t MECCEMSB:1;                /**< bit:     19  Multiple ECC Error on MSB Part of the Memory Flash Data Bus (cleared on read) */
    uint32_t :12;                       /**< bit: 20..31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} EEFC_FSR_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define EEFC_FSR_OFFSET                     (0x08)                                        /**<  (EEFC_FSR) EEFC Flash Status Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define EEFC_FSR_FRDY_Pos                   0                                              /**< (EEFC_FSR) Flash Ready Status (cleared when Flash is busy) Position */
#define EEFC_FSR_FRDY_Msk                   (0x1U << EEFC_FSR_FRDY_Pos)                    /**< (EEFC_FSR) Flash Ready Status (cleared when Flash is busy) Mask */
#define EEFC_FSR_FRDY                       EEFC_FSR_FRDY_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use EEFC_FSR_FRDY_Msk instead */
#define EEFC_FSR_FCMDE_Pos                  1                                              /**< (EEFC_FSR) Flash Command Error Status (cleared on read or by writing EEFC_FCR) Position */
#define EEFC_FSR_FCMDE_Msk                  (0x1U << EEFC_FSR_FCMDE_Pos)                   /**< (EEFC_FSR) Flash Command Error Status (cleared on read or by writing EEFC_FCR) Mask */
#define EEFC_FSR_FCMDE                      EEFC_FSR_FCMDE_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use EEFC_FSR_FCMDE_Msk instead */
#define EEFC_FSR_FLOCKE_Pos                 2                                              /**< (EEFC_FSR) Flash Lock Error Status (cleared on read) Position */
#define EEFC_FSR_FLOCKE_Msk                 (0x1U << EEFC_FSR_FLOCKE_Pos)                  /**< (EEFC_FSR) Flash Lock Error Status (cleared on read) Mask */
#define EEFC_FSR_FLOCKE                     EEFC_FSR_FLOCKE_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use EEFC_FSR_FLOCKE_Msk instead */
#define EEFC_FSR_FLERR_Pos                  3                                              /**< (EEFC_FSR) Flash Error Status (cleared when a programming operation starts) Position */
#define EEFC_FSR_FLERR_Msk                  (0x1U << EEFC_FSR_FLERR_Pos)                   /**< (EEFC_FSR) Flash Error Status (cleared when a programming operation starts) Mask */
#define EEFC_FSR_FLERR                      EEFC_FSR_FLERR_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use EEFC_FSR_FLERR_Msk instead */
#define EEFC_FSR_UECCELSB_Pos               16                                             /**< (EEFC_FSR) Unique ECC Error on LSB Part of the Memory Flash Data Bus (cleared on read) Position */
#define EEFC_FSR_UECCELSB_Msk               (0x1U << EEFC_FSR_UECCELSB_Pos)                /**< (EEFC_FSR) Unique ECC Error on LSB Part of the Memory Flash Data Bus (cleared on read) Mask */
#define EEFC_FSR_UECCELSB                   EEFC_FSR_UECCELSB_Msk                          /**< \deprecated Old style mask definition for 1 bit bitfield. Use EEFC_FSR_UECCELSB_Msk instead */
#define EEFC_FSR_MECCELSB_Pos               17                                             /**< (EEFC_FSR) Multiple ECC Error on LSB Part of the Memory Flash Data Bus (cleared on read) Position */
#define EEFC_FSR_MECCELSB_Msk               (0x1U << EEFC_FSR_MECCELSB_Pos)                /**< (EEFC_FSR) Multiple ECC Error on LSB Part of the Memory Flash Data Bus (cleared on read) Mask */
#define EEFC_FSR_MECCELSB                   EEFC_FSR_MECCELSB_Msk                          /**< \deprecated Old style mask definition for 1 bit bitfield. Use EEFC_FSR_MECCELSB_Msk instead */
#define EEFC_FSR_UECCEMSB_Pos               18                                             /**< (EEFC_FSR) Unique ECC Error on MSB Part of the Memory Flash Data Bus (cleared on read) Position */
#define EEFC_FSR_UECCEMSB_Msk               (0x1U << EEFC_FSR_UECCEMSB_Pos)                /**< (EEFC_FSR) Unique ECC Error on MSB Part of the Memory Flash Data Bus (cleared on read) Mask */
#define EEFC_FSR_UECCEMSB                   EEFC_FSR_UECCEMSB_Msk                          /**< \deprecated Old style mask definition for 1 bit bitfield. Use EEFC_FSR_UECCEMSB_Msk instead */
#define EEFC_FSR_MECCEMSB_Pos               19                                             /**< (EEFC_FSR) Multiple ECC Error on MSB Part of the Memory Flash Data Bus (cleared on read) Position */
#define EEFC_FSR_MECCEMSB_Msk               (0x1U << EEFC_FSR_MECCEMSB_Pos)                /**< (EEFC_FSR) Multiple ECC Error on MSB Part of the Memory Flash Data Bus (cleared on read) Mask */
#define EEFC_FSR_MECCEMSB                   EEFC_FSR_MECCEMSB_Msk                          /**< \deprecated Old style mask definition for 1 bit bitfield. Use EEFC_FSR_MECCEMSB_Msk instead */
#define EEFC_FSR_MASK                       (0xF000FU)                                     /**< \deprecated (EEFC_FSR) Register MASK  (Use EEFC_FSR_Msk instead)  */
#define EEFC_FSR_Msk                        (0xF000FU)                                     /**< (EEFC_FSR) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- EEFC_FRR : (EFC Offset: 0x0c) (R/ 32) EEFC Flash Result Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t FVALUE:32;                 /**< bit:  0..31  Flash Result Value                       */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} EEFC_FRR_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define EEFC_FRR_OFFSET                     (0x0C)                                        /**<  (EEFC_FRR) EEFC Flash Result Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define EEFC_FRR_FVALUE_Pos                 0                                              /**< (EEFC_FRR) Flash Result Value Position */
#define EEFC_FRR_FVALUE_Msk                 (0xFFFFFFFFU << EEFC_FRR_FVALUE_Pos)           /**< (EEFC_FRR) Flash Result Value Mask */
#define EEFC_FRR_FVALUE(value)              (EEFC_FRR_FVALUE_Msk & ((value) << EEFC_FRR_FVALUE_Pos))
#define EEFC_FRR_MASK                       (0xFFFFFFFFU)                                  /**< \deprecated (EEFC_FRR) Register MASK  (Use EEFC_FRR_Msk instead)  */
#define EEFC_FRR_Msk                        (0xFFFFFFFFU)                                  /**< (EEFC_FRR) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- EEFC_WPMR : (EFC Offset: 0xe4) (R/W 32) Write Protection Mode Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t WPEN:1;                    /**< bit:      0  Write Protection Enable                  */
    uint32_t :7;                        /**< bit:   1..7  Reserved */
    uint32_t WPKEY:24;                  /**< bit:  8..31  Write Protection Key                     */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} EEFC_WPMR_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define EEFC_WPMR_OFFSET                    (0xE4)                                        /**<  (EEFC_WPMR) Write Protection Mode Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define EEFC_WPMR_WPEN_Pos                  0                                              /**< (EEFC_WPMR) Write Protection Enable Position */
#define EEFC_WPMR_WPEN_Msk                  (0x1U << EEFC_WPMR_WPEN_Pos)                   /**< (EEFC_WPMR) Write Protection Enable Mask */
#define EEFC_WPMR_WPEN                      EEFC_WPMR_WPEN_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use EEFC_WPMR_WPEN_Msk instead */
#define EEFC_WPMR_WPKEY_Pos                 8                                              /**< (EEFC_WPMR) Write Protection Key Position */
#define EEFC_WPMR_WPKEY_Msk                 (0xFFFFFFU << EEFC_WPMR_WPKEY_Pos)             /**< (EEFC_WPMR) Write Protection Key Mask */
#define EEFC_WPMR_WPKEY(value)              (EEFC_WPMR_WPKEY_Msk & ((value) << EEFC_WPMR_WPKEY_Pos))
#define   EEFC_WPMR_WPKEY_PASSWD_Val        (0x454643U)                                    /**< (EEFC_WPMR) Writing any other value in this field aborts the write operation.Always reads as 0.  */
#define EEFC_WPMR_WPKEY_PASSWD              (EEFC_WPMR_WPKEY_PASSWD_Val << EEFC_WPMR_WPKEY_Pos)  /**< (EEFC_WPMR) Writing any other value in this field aborts the write operation.Always reads as 0. Position  */
#define EEFC_WPMR_MASK                      (0xFFFFFF01U)                                  /**< \deprecated (EEFC_WPMR) Register MASK  (Use EEFC_WPMR_Msk instead)  */
#define EEFC_WPMR_Msk                       (0xFFFFFF01U)                                  /**< (EEFC_WPMR) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
#if COMPONENT_TYPEDEF_STYLE == 'R'
/** \brief EFC hardware registers */
typedef struct {  
  __IO uint32_t EEFC_FMR;       /**< (EFC Offset: 0x00) EEFC Flash Mode Register */
  __O  uint32_t EEFC_FCR;       /**< (EFC Offset: 0x04) EEFC Flash Command Register */
  __I  uint32_t EEFC_FSR;       /**< (EFC Offset: 0x08) EEFC Flash Status Register */
  __I  uint32_t EEFC_FRR;       /**< (EFC Offset: 0x0C) EEFC Flash Result Register */
  __I  uint32_t Reserved1[53];
  __IO uint32_t EEFC_WPMR;      /**< (EFC Offset: 0xE4) Write Protection Mode Register */
} Efc;

#elif COMPONENT_TYPEDEF_STYLE == 'N'
/** \brief EFC hardware registers */
typedef struct {  
  __IO EEFC_FMR_Type                  EEFC_FMR;       /**< Offset: 0x00 (R/W  32) EEFC Flash Mode Register */
  __O  EEFC_FCR_Type                  EEFC_FCR;       /**< Offset: 0x04 ( /W  32) EEFC Flash Command Register */
  __I  EEFC_FSR_Type                  EEFC_FSR;       /**< Offset: 0x08 (R/   32) EEFC Flash Status Register */
  __I  EEFC_FRR_Type                  EEFC_FRR;       /**< Offset: 0x0C (R/   32) EEFC Flash Result Register */
       RoReg8                         Reserved1[0xD4];
  __IO EEFC_WPMR_Type                 EEFC_WPMR;      /**< Offset: 0xE4 (R/W  32) Write Protection Mode Register */
} Efc;

#else /* COMPONENT_TYPEDEF_STYLE */
#error Unknown component typedef style
#endif /* COMPONENT_TYPEDEF_STYLE */

#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/** @}  end of Embedded Flash Controller */

#endif /* _SAME70_EFC_COMPONENT_H_ */
