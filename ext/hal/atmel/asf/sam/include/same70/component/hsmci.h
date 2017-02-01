/**
 * \file
 *
 * \brief Component description for HSMCI
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

#ifndef _SAME70_HSMCI_COMPONENT_H_
#define _SAME70_HSMCI_COMPONENT_H_
#define _SAME70_HSMCI_COMPONENT_         /**< \deprecated  Backward compatibility for ASF */

/** \addtogroup SAME70_HSMCI High Speed MultiMedia Card Interface
 *  @{
 */
/* ========================================================================== */
/**  SOFTWARE API DEFINITION FOR HSMCI */
/* ========================================================================== */
#ifndef COMPONENT_TYPEDEF_STYLE
  #define COMPONENT_TYPEDEF_STYLE 'R'  /**< Defines default style of typedefs for the component header files ('R' = RFO, 'N' = NTO)*/
#endif

#define HSMCI_6449                       /**< (HSMCI) Module ID */
#define REV_HSMCI Q                      /**< (HSMCI) Module revision */

/* -------- HSMCI_CR : (HSMCI Offset: 0x00) (/W 32) Control Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t MCIEN:1;                   /**< bit:      0  Multi-Media Interface Enable             */
    uint32_t MCIDIS:1;                  /**< bit:      1  Multi-Media Interface Disable            */
    uint32_t PWSEN:1;                   /**< bit:      2  Power Save Mode Enable                   */
    uint32_t PWSDIS:1;                  /**< bit:      3  Power Save Mode Disable                  */
    uint32_t :3;                        /**< bit:   4..6  Reserved */
    uint32_t SWRST:1;                   /**< bit:      7  Software Reset                           */
    uint32_t :24;                       /**< bit:  8..31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} HSMCI_CR_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define HSMCI_CR_OFFSET                     (0x00)                                        /**<  (HSMCI_CR) Control Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define HSMCI_CR_MCIEN_Pos                  0                                              /**< (HSMCI_CR) Multi-Media Interface Enable Position */
#define HSMCI_CR_MCIEN_Msk                  (0x1U << HSMCI_CR_MCIEN_Pos)                   /**< (HSMCI_CR) Multi-Media Interface Enable Mask */
#define HSMCI_CR_MCIEN                      HSMCI_CR_MCIEN_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use HSMCI_CR_MCIEN_Msk instead */
#define HSMCI_CR_MCIDIS_Pos                 1                                              /**< (HSMCI_CR) Multi-Media Interface Disable Position */
#define HSMCI_CR_MCIDIS_Msk                 (0x1U << HSMCI_CR_MCIDIS_Pos)                  /**< (HSMCI_CR) Multi-Media Interface Disable Mask */
#define HSMCI_CR_MCIDIS                     HSMCI_CR_MCIDIS_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use HSMCI_CR_MCIDIS_Msk instead */
#define HSMCI_CR_PWSEN_Pos                  2                                              /**< (HSMCI_CR) Power Save Mode Enable Position */
#define HSMCI_CR_PWSEN_Msk                  (0x1U << HSMCI_CR_PWSEN_Pos)                   /**< (HSMCI_CR) Power Save Mode Enable Mask */
#define HSMCI_CR_PWSEN                      HSMCI_CR_PWSEN_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use HSMCI_CR_PWSEN_Msk instead */
#define HSMCI_CR_PWSDIS_Pos                 3                                              /**< (HSMCI_CR) Power Save Mode Disable Position */
#define HSMCI_CR_PWSDIS_Msk                 (0x1U << HSMCI_CR_PWSDIS_Pos)                  /**< (HSMCI_CR) Power Save Mode Disable Mask */
#define HSMCI_CR_PWSDIS                     HSMCI_CR_PWSDIS_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use HSMCI_CR_PWSDIS_Msk instead */
#define HSMCI_CR_SWRST_Pos                  7                                              /**< (HSMCI_CR) Software Reset Position */
#define HSMCI_CR_SWRST_Msk                  (0x1U << HSMCI_CR_SWRST_Pos)                   /**< (HSMCI_CR) Software Reset Mask */
#define HSMCI_CR_SWRST                      HSMCI_CR_SWRST_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use HSMCI_CR_SWRST_Msk instead */
#define HSMCI_CR_MASK                       (0x8FU)                                        /**< \deprecated (HSMCI_CR) Register MASK  (Use HSMCI_CR_Msk instead)  */
#define HSMCI_CR_Msk                        (0x8FU)                                        /**< (HSMCI_CR) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- HSMCI_MR : (HSMCI Offset: 0x04) (R/W 32) Mode Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t CLKDIV:8;                  /**< bit:   0..7  Clock Divider                            */
    uint32_t PWSDIV:3;                  /**< bit:  8..10  Power Saving Divider                     */
    uint32_t RDPROOF:1;                 /**< bit:     11  Read Proof Enable                        */
    uint32_t WRPROOF:1;                 /**< bit:     12  Write Proof Enable                       */
    uint32_t FBYTE:1;                   /**< bit:     13  Force Byte Transfer                      */
    uint32_t PADV:1;                    /**< bit:     14  Padding Value                            */
    uint32_t :1;                        /**< bit:     15  Reserved */
    uint32_t CLKODD:1;                  /**< bit:     16  Clock divider is odd                     */
    uint32_t :15;                       /**< bit: 17..31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} HSMCI_MR_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define HSMCI_MR_OFFSET                     (0x04)                                        /**<  (HSMCI_MR) Mode Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define HSMCI_MR_CLKDIV_Pos                 0                                              /**< (HSMCI_MR) Clock Divider Position */
#define HSMCI_MR_CLKDIV_Msk                 (0xFFU << HSMCI_MR_CLKDIV_Pos)                 /**< (HSMCI_MR) Clock Divider Mask */
#define HSMCI_MR_CLKDIV(value)              (HSMCI_MR_CLKDIV_Msk & ((value) << HSMCI_MR_CLKDIV_Pos))
#define HSMCI_MR_PWSDIV_Pos                 8                                              /**< (HSMCI_MR) Power Saving Divider Position */
#define HSMCI_MR_PWSDIV_Msk                 (0x7U << HSMCI_MR_PWSDIV_Pos)                  /**< (HSMCI_MR) Power Saving Divider Mask */
#define HSMCI_MR_PWSDIV(value)              (HSMCI_MR_PWSDIV_Msk & ((value) << HSMCI_MR_PWSDIV_Pos))
#define HSMCI_MR_RDPROOF_Pos                11                                             /**< (HSMCI_MR) Read Proof Enable Position */
#define HSMCI_MR_RDPROOF_Msk                (0x1U << HSMCI_MR_RDPROOF_Pos)                 /**< (HSMCI_MR) Read Proof Enable Mask */
#define HSMCI_MR_RDPROOF                    HSMCI_MR_RDPROOF_Msk                           /**< \deprecated Old style mask definition for 1 bit bitfield. Use HSMCI_MR_RDPROOF_Msk instead */
#define HSMCI_MR_WRPROOF_Pos                12                                             /**< (HSMCI_MR) Write Proof Enable Position */
#define HSMCI_MR_WRPROOF_Msk                (0x1U << HSMCI_MR_WRPROOF_Pos)                 /**< (HSMCI_MR) Write Proof Enable Mask */
#define HSMCI_MR_WRPROOF                    HSMCI_MR_WRPROOF_Msk                           /**< \deprecated Old style mask definition for 1 bit bitfield. Use HSMCI_MR_WRPROOF_Msk instead */
#define HSMCI_MR_FBYTE_Pos                  13                                             /**< (HSMCI_MR) Force Byte Transfer Position */
#define HSMCI_MR_FBYTE_Msk                  (0x1U << HSMCI_MR_FBYTE_Pos)                   /**< (HSMCI_MR) Force Byte Transfer Mask */
#define HSMCI_MR_FBYTE                      HSMCI_MR_FBYTE_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use HSMCI_MR_FBYTE_Msk instead */
#define HSMCI_MR_PADV_Pos                   14                                             /**< (HSMCI_MR) Padding Value Position */
#define HSMCI_MR_PADV_Msk                   (0x1U << HSMCI_MR_PADV_Pos)                    /**< (HSMCI_MR) Padding Value Mask */
#define HSMCI_MR_PADV                       HSMCI_MR_PADV_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use HSMCI_MR_PADV_Msk instead */
#define HSMCI_MR_CLKODD_Pos                 16                                             /**< (HSMCI_MR) Clock divider is odd Position */
#define HSMCI_MR_CLKODD_Msk                 (0x1U << HSMCI_MR_CLKODD_Pos)                  /**< (HSMCI_MR) Clock divider is odd Mask */
#define HSMCI_MR_CLKODD                     HSMCI_MR_CLKODD_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use HSMCI_MR_CLKODD_Msk instead */
#define HSMCI_MR_MASK                       (0x17FFFU)                                     /**< \deprecated (HSMCI_MR) Register MASK  (Use HSMCI_MR_Msk instead)  */
#define HSMCI_MR_Msk                        (0x17FFFU)                                     /**< (HSMCI_MR) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- HSMCI_DTOR : (HSMCI Offset: 0x08) (R/W 32) Data Timeout Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t DTOCYC:4;                  /**< bit:   0..3  Data Timeout Cycle Number                */
    uint32_t DTOMUL:3;                  /**< bit:   4..6  Data Timeout Multiplier                  */
    uint32_t :25;                       /**< bit:  7..31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} HSMCI_DTOR_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define HSMCI_DTOR_OFFSET                   (0x08)                                        /**<  (HSMCI_DTOR) Data Timeout Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define HSMCI_DTOR_DTOCYC_Pos               0                                              /**< (HSMCI_DTOR) Data Timeout Cycle Number Position */
#define HSMCI_DTOR_DTOCYC_Msk               (0xFU << HSMCI_DTOR_DTOCYC_Pos)                /**< (HSMCI_DTOR) Data Timeout Cycle Number Mask */
#define HSMCI_DTOR_DTOCYC(value)            (HSMCI_DTOR_DTOCYC_Msk & ((value) << HSMCI_DTOR_DTOCYC_Pos))
#define HSMCI_DTOR_DTOMUL_Pos               4                                              /**< (HSMCI_DTOR) Data Timeout Multiplier Position */
#define HSMCI_DTOR_DTOMUL_Msk               (0x7U << HSMCI_DTOR_DTOMUL_Pos)                /**< (HSMCI_DTOR) Data Timeout Multiplier Mask */
#define HSMCI_DTOR_DTOMUL(value)            (HSMCI_DTOR_DTOMUL_Msk & ((value) << HSMCI_DTOR_DTOMUL_Pos))
#define   HSMCI_DTOR_DTOMUL_1_Val           (0x0U)                                         /**< (HSMCI_DTOR) DTOCYC  */
#define   HSMCI_DTOR_DTOMUL_16_Val          (0x1U)                                         /**< (HSMCI_DTOR) DTOCYC x 16  */
#define   HSMCI_DTOR_DTOMUL_128_Val         (0x2U)                                         /**< (HSMCI_DTOR) DTOCYC x 128  */
#define   HSMCI_DTOR_DTOMUL_256_Val         (0x3U)                                         /**< (HSMCI_DTOR) DTOCYC x 256  */
#define   HSMCI_DTOR_DTOMUL_1024_Val        (0x4U)                                         /**< (HSMCI_DTOR) DTOCYC x 1024  */
#define   HSMCI_DTOR_DTOMUL_4096_Val        (0x5U)                                         /**< (HSMCI_DTOR) DTOCYC x 4096  */
#define   HSMCI_DTOR_DTOMUL_65536_Val       (0x6U)                                         /**< (HSMCI_DTOR) DTOCYC x 65536  */
#define   HSMCI_DTOR_DTOMUL_1048576_Val     (0x7U)                                         /**< (HSMCI_DTOR) DTOCYC x 1048576  */
#define HSMCI_DTOR_DTOMUL_1                 (HSMCI_DTOR_DTOMUL_1_Val << HSMCI_DTOR_DTOMUL_Pos)  /**< (HSMCI_DTOR) DTOCYC Position  */
#define HSMCI_DTOR_DTOMUL_16                (HSMCI_DTOR_DTOMUL_16_Val << HSMCI_DTOR_DTOMUL_Pos)  /**< (HSMCI_DTOR) DTOCYC x 16 Position  */
#define HSMCI_DTOR_DTOMUL_128               (HSMCI_DTOR_DTOMUL_128_Val << HSMCI_DTOR_DTOMUL_Pos)  /**< (HSMCI_DTOR) DTOCYC x 128 Position  */
#define HSMCI_DTOR_DTOMUL_256               (HSMCI_DTOR_DTOMUL_256_Val << HSMCI_DTOR_DTOMUL_Pos)  /**< (HSMCI_DTOR) DTOCYC x 256 Position  */
#define HSMCI_DTOR_DTOMUL_1024              (HSMCI_DTOR_DTOMUL_1024_Val << HSMCI_DTOR_DTOMUL_Pos)  /**< (HSMCI_DTOR) DTOCYC x 1024 Position  */
#define HSMCI_DTOR_DTOMUL_4096              (HSMCI_DTOR_DTOMUL_4096_Val << HSMCI_DTOR_DTOMUL_Pos)  /**< (HSMCI_DTOR) DTOCYC x 4096 Position  */
#define HSMCI_DTOR_DTOMUL_65536             (HSMCI_DTOR_DTOMUL_65536_Val << HSMCI_DTOR_DTOMUL_Pos)  /**< (HSMCI_DTOR) DTOCYC x 65536 Position  */
#define HSMCI_DTOR_DTOMUL_1048576           (HSMCI_DTOR_DTOMUL_1048576_Val << HSMCI_DTOR_DTOMUL_Pos)  /**< (HSMCI_DTOR) DTOCYC x 1048576 Position  */
#define HSMCI_DTOR_MASK                     (0x7FU)                                        /**< \deprecated (HSMCI_DTOR) Register MASK  (Use HSMCI_DTOR_Msk instead)  */
#define HSMCI_DTOR_Msk                      (0x7FU)                                        /**< (HSMCI_DTOR) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- HSMCI_SDCR : (HSMCI Offset: 0x0c) (R/W 32) SD/SDIO Card Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t SDCSEL:2;                  /**< bit:   0..1  SDCard/SDIO Slot                         */
    uint32_t :4;                        /**< bit:   2..5  Reserved */
    uint32_t SDCBUS:2;                  /**< bit:   6..7  SDCard/SDIO Bus Width                    */
    uint32_t :24;                       /**< bit:  8..31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} HSMCI_SDCR_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define HSMCI_SDCR_OFFSET                   (0x0C)                                        /**<  (HSMCI_SDCR) SD/SDIO Card Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define HSMCI_SDCR_SDCSEL_Pos               0                                              /**< (HSMCI_SDCR) SDCard/SDIO Slot Position */
#define HSMCI_SDCR_SDCSEL_Msk               (0x3U << HSMCI_SDCR_SDCSEL_Pos)                /**< (HSMCI_SDCR) SDCard/SDIO Slot Mask */
#define HSMCI_SDCR_SDCSEL(value)            (HSMCI_SDCR_SDCSEL_Msk & ((value) << HSMCI_SDCR_SDCSEL_Pos))
#define   HSMCI_SDCR_SDCSEL_SLOTA_Val       (0x0U)                                         /**< (HSMCI_SDCR) Slot A is selected.  */
#define HSMCI_SDCR_SDCSEL_SLOTA             (HSMCI_SDCR_SDCSEL_SLOTA_Val << HSMCI_SDCR_SDCSEL_Pos)  /**< (HSMCI_SDCR) Slot A is selected. Position  */
#define HSMCI_SDCR_SDCBUS_Pos               6                                              /**< (HSMCI_SDCR) SDCard/SDIO Bus Width Position */
#define HSMCI_SDCR_SDCBUS_Msk               (0x3U << HSMCI_SDCR_SDCBUS_Pos)                /**< (HSMCI_SDCR) SDCard/SDIO Bus Width Mask */
#define HSMCI_SDCR_SDCBUS(value)            (HSMCI_SDCR_SDCBUS_Msk & ((value) << HSMCI_SDCR_SDCBUS_Pos))
#define   HSMCI_SDCR_SDCBUS_1_Val           (0x0U)                                         /**< (HSMCI_SDCR) 1 bit  */
#define   HSMCI_SDCR_SDCBUS_4_Val           (0x2U)                                         /**< (HSMCI_SDCR) 4 bits  */
#define   HSMCI_SDCR_SDCBUS_8_Val           (0x3U)                                         /**< (HSMCI_SDCR) 8 bits  */
#define HSMCI_SDCR_SDCBUS_1                 (HSMCI_SDCR_SDCBUS_1_Val << HSMCI_SDCR_SDCBUS_Pos)  /**< (HSMCI_SDCR) 1 bit Position  */
#define HSMCI_SDCR_SDCBUS_4                 (HSMCI_SDCR_SDCBUS_4_Val << HSMCI_SDCR_SDCBUS_Pos)  /**< (HSMCI_SDCR) 4 bits Position  */
#define HSMCI_SDCR_SDCBUS_8                 (HSMCI_SDCR_SDCBUS_8_Val << HSMCI_SDCR_SDCBUS_Pos)  /**< (HSMCI_SDCR) 8 bits Position  */
#define HSMCI_SDCR_MASK                     (0xC3U)                                        /**< \deprecated (HSMCI_SDCR) Register MASK  (Use HSMCI_SDCR_Msk instead)  */
#define HSMCI_SDCR_Msk                      (0xC3U)                                        /**< (HSMCI_SDCR) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- HSMCI_ARGR : (HSMCI Offset: 0x10) (R/W 32) Argument Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t ARG:32;                    /**< bit:  0..31  Command Argument                         */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} HSMCI_ARGR_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define HSMCI_ARGR_OFFSET                   (0x10)                                        /**<  (HSMCI_ARGR) Argument Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define HSMCI_ARGR_ARG_Pos                  0                                              /**< (HSMCI_ARGR) Command Argument Position */
#define HSMCI_ARGR_ARG_Msk                  (0xFFFFFFFFU << HSMCI_ARGR_ARG_Pos)            /**< (HSMCI_ARGR) Command Argument Mask */
#define HSMCI_ARGR_ARG(value)               (HSMCI_ARGR_ARG_Msk & ((value) << HSMCI_ARGR_ARG_Pos))
#define HSMCI_ARGR_MASK                     (0xFFFFFFFFU)                                  /**< \deprecated (HSMCI_ARGR) Register MASK  (Use HSMCI_ARGR_Msk instead)  */
#define HSMCI_ARGR_Msk                      (0xFFFFFFFFU)                                  /**< (HSMCI_ARGR) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- HSMCI_CMDR : (HSMCI Offset: 0x14) (/W 32) Command Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t CMDNB:6;                   /**< bit:   0..5  Command Number                           */
    uint32_t RSPTYP:2;                  /**< bit:   6..7  Response Type                            */
    uint32_t SPCMD:3;                   /**< bit:  8..10  Special Command                          */
    uint32_t OPDCMD:1;                  /**< bit:     11  Open Drain Command                       */
    uint32_t MAXLAT:1;                  /**< bit:     12  Max Latency for Command to Response      */
    uint32_t :3;                        /**< bit: 13..15  Reserved */
    uint32_t TRCMD:2;                   /**< bit: 16..17  Transfer Command                         */
    uint32_t TRDIR:1;                   /**< bit:     18  Transfer Direction                       */
    uint32_t TRTYP:3;                   /**< bit: 19..21  Transfer Type                            */
    uint32_t :2;                        /**< bit: 22..23  Reserved */
    uint32_t IOSPCMD:2;                 /**< bit: 24..25  SDIO Special Command                     */
    uint32_t ATACS:1;                   /**< bit:     26  ATA with Command Completion Signal       */
    uint32_t BOOT_ACK:1;                /**< bit:     27  Boot Operation Acknowledge               */
    uint32_t :4;                        /**< bit: 28..31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} HSMCI_CMDR_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define HSMCI_CMDR_OFFSET                   (0x14)                                        /**<  (HSMCI_CMDR) Command Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define HSMCI_CMDR_CMDNB_Pos                0                                              /**< (HSMCI_CMDR) Command Number Position */
#define HSMCI_CMDR_CMDNB_Msk                (0x3FU << HSMCI_CMDR_CMDNB_Pos)                /**< (HSMCI_CMDR) Command Number Mask */
#define HSMCI_CMDR_CMDNB(value)             (HSMCI_CMDR_CMDNB_Msk & ((value) << HSMCI_CMDR_CMDNB_Pos))
#define HSMCI_CMDR_RSPTYP_Pos               6                                              /**< (HSMCI_CMDR) Response Type Position */
#define HSMCI_CMDR_RSPTYP_Msk               (0x3U << HSMCI_CMDR_RSPTYP_Pos)                /**< (HSMCI_CMDR) Response Type Mask */
#define HSMCI_CMDR_RSPTYP(value)            (HSMCI_CMDR_RSPTYP_Msk & ((value) << HSMCI_CMDR_RSPTYP_Pos))
#define   HSMCI_CMDR_RSPTYP_NORESP_Val      (0x0U)                                         /**< (HSMCI_CMDR) No response  */
#define   HSMCI_CMDR_RSPTYP_48_BIT_Val      (0x1U)                                         /**< (HSMCI_CMDR) 48-bit response  */
#define   HSMCI_CMDR_RSPTYP_136_BIT_Val     (0x2U)                                         /**< (HSMCI_CMDR) 136-bit response  */
#define   HSMCI_CMDR_RSPTYP_R1B_Val         (0x3U)                                         /**< (HSMCI_CMDR) R1b response type  */
#define HSMCI_CMDR_RSPTYP_NORESP            (HSMCI_CMDR_RSPTYP_NORESP_Val << HSMCI_CMDR_RSPTYP_Pos)  /**< (HSMCI_CMDR) No response Position  */
#define HSMCI_CMDR_RSPTYP_48_BIT            (HSMCI_CMDR_RSPTYP_48_BIT_Val << HSMCI_CMDR_RSPTYP_Pos)  /**< (HSMCI_CMDR) 48-bit response Position  */
#define HSMCI_CMDR_RSPTYP_136_BIT           (HSMCI_CMDR_RSPTYP_136_BIT_Val << HSMCI_CMDR_RSPTYP_Pos)  /**< (HSMCI_CMDR) 136-bit response Position  */
#define HSMCI_CMDR_RSPTYP_R1B               (HSMCI_CMDR_RSPTYP_R1B_Val << HSMCI_CMDR_RSPTYP_Pos)  /**< (HSMCI_CMDR) R1b response type Position  */
#define HSMCI_CMDR_SPCMD_Pos                8                                              /**< (HSMCI_CMDR) Special Command Position */
#define HSMCI_CMDR_SPCMD_Msk                (0x7U << HSMCI_CMDR_SPCMD_Pos)                 /**< (HSMCI_CMDR) Special Command Mask */
#define HSMCI_CMDR_SPCMD(value)             (HSMCI_CMDR_SPCMD_Msk & ((value) << HSMCI_CMDR_SPCMD_Pos))
#define   HSMCI_CMDR_SPCMD_STD_Val          (0x0U)                                         /**< (HSMCI_CMDR) Not a special CMD.  */
#define   HSMCI_CMDR_SPCMD_INIT_Val         (0x1U)                                         /**< (HSMCI_CMDR) Initialization CMD: 74 clock cycles for initialization sequence.  */
#define   HSMCI_CMDR_SPCMD_SYNC_Val         (0x2U)                                         /**< (HSMCI_CMDR) Synchronized CMD: Wait for the end of the current data block transfer before sending the pending command.  */
#define   HSMCI_CMDR_SPCMD_CE_ATA_Val       (0x3U)                                         /**< (HSMCI_CMDR) CE-ATA Completion Signal disable Command. The host cancels the ability for the device to return a command completion signal on the command line.  */
#define   HSMCI_CMDR_SPCMD_IT_CMD_Val       (0x4U)                                         /**< (HSMCI_CMDR) Interrupt command: Corresponds to the Interrupt Mode (CMD40).  */
#define   HSMCI_CMDR_SPCMD_IT_RESP_Val      (0x5U)                                         /**< (HSMCI_CMDR) Interrupt response: Corresponds to the Interrupt Mode (CMD40).  */
#define   HSMCI_CMDR_SPCMD_BOR_Val          (0x6U)                                         /**< (HSMCI_CMDR) Boot Operation Request. Start a boot operation mode, the host processor can read boot data from the MMC device directly.  */
#define   HSMCI_CMDR_SPCMD_EBO_Val          (0x7U)                                         /**< (HSMCI_CMDR) End Boot Operation. This command allows the host processor to terminate the boot operation mode.  */
#define HSMCI_CMDR_SPCMD_STD                (HSMCI_CMDR_SPCMD_STD_Val << HSMCI_CMDR_SPCMD_Pos)  /**< (HSMCI_CMDR) Not a special CMD. Position  */
#define HSMCI_CMDR_SPCMD_INIT               (HSMCI_CMDR_SPCMD_INIT_Val << HSMCI_CMDR_SPCMD_Pos)  /**< (HSMCI_CMDR) Initialization CMD: 74 clock cycles for initialization sequence. Position  */
#define HSMCI_CMDR_SPCMD_SYNC               (HSMCI_CMDR_SPCMD_SYNC_Val << HSMCI_CMDR_SPCMD_Pos)  /**< (HSMCI_CMDR) Synchronized CMD: Wait for the end of the current data block transfer before sending the pending command. Position  */
#define HSMCI_CMDR_SPCMD_CE_ATA             (HSMCI_CMDR_SPCMD_CE_ATA_Val << HSMCI_CMDR_SPCMD_Pos)  /**< (HSMCI_CMDR) CE-ATA Completion Signal disable Command. The host cancels the ability for the device to return a command completion signal on the command line. Position  */
#define HSMCI_CMDR_SPCMD_IT_CMD             (HSMCI_CMDR_SPCMD_IT_CMD_Val << HSMCI_CMDR_SPCMD_Pos)  /**< (HSMCI_CMDR) Interrupt command: Corresponds to the Interrupt Mode (CMD40). Position  */
#define HSMCI_CMDR_SPCMD_IT_RESP            (HSMCI_CMDR_SPCMD_IT_RESP_Val << HSMCI_CMDR_SPCMD_Pos)  /**< (HSMCI_CMDR) Interrupt response: Corresponds to the Interrupt Mode (CMD40). Position  */
#define HSMCI_CMDR_SPCMD_BOR                (HSMCI_CMDR_SPCMD_BOR_Val << HSMCI_CMDR_SPCMD_Pos)  /**< (HSMCI_CMDR) Boot Operation Request. Start a boot operation mode, the host processor can read boot data from the MMC device directly. Position  */
#define HSMCI_CMDR_SPCMD_EBO                (HSMCI_CMDR_SPCMD_EBO_Val << HSMCI_CMDR_SPCMD_Pos)  /**< (HSMCI_CMDR) End Boot Operation. This command allows the host processor to terminate the boot operation mode. Position  */
#define HSMCI_CMDR_OPDCMD_Pos               11                                             /**< (HSMCI_CMDR) Open Drain Command Position */
#define HSMCI_CMDR_OPDCMD_Msk               (0x1U << HSMCI_CMDR_OPDCMD_Pos)                /**< (HSMCI_CMDR) Open Drain Command Mask */
#define HSMCI_CMDR_OPDCMD                   HSMCI_CMDR_OPDCMD_Msk                          /**< \deprecated Old style mask definition for 1 bit bitfield. Use HSMCI_CMDR_OPDCMD_Msk instead */
#define   HSMCI_CMDR_OPDCMD_PUSHPULL_Val    (0x0U)                                         /**< (HSMCI_CMDR) Push pull command.  */
#define   HSMCI_CMDR_OPDCMD_OPENDRAIN_Val   (0x1U)                                         /**< (HSMCI_CMDR) Open drain command.  */
#define HSMCI_CMDR_OPDCMD_PUSHPULL          (HSMCI_CMDR_OPDCMD_PUSHPULL_Val << HSMCI_CMDR_OPDCMD_Pos)  /**< (HSMCI_CMDR) Push pull command. Position  */
#define HSMCI_CMDR_OPDCMD_OPENDRAIN         (HSMCI_CMDR_OPDCMD_OPENDRAIN_Val << HSMCI_CMDR_OPDCMD_Pos)  /**< (HSMCI_CMDR) Open drain command. Position  */
#define HSMCI_CMDR_MAXLAT_Pos               12                                             /**< (HSMCI_CMDR) Max Latency for Command to Response Position */
#define HSMCI_CMDR_MAXLAT_Msk               (0x1U << HSMCI_CMDR_MAXLAT_Pos)                /**< (HSMCI_CMDR) Max Latency for Command to Response Mask */
#define HSMCI_CMDR_MAXLAT                   HSMCI_CMDR_MAXLAT_Msk                          /**< \deprecated Old style mask definition for 1 bit bitfield. Use HSMCI_CMDR_MAXLAT_Msk instead */
#define   HSMCI_CMDR_MAXLAT_5_Val           (0x0U)                                         /**< (HSMCI_CMDR) 5-cycle max latency.  */
#define   HSMCI_CMDR_MAXLAT_64_Val          (0x1U)                                         /**< (HSMCI_CMDR) 64-cycle max latency.  */
#define HSMCI_CMDR_MAXLAT_5                 (HSMCI_CMDR_MAXLAT_5_Val << HSMCI_CMDR_MAXLAT_Pos)  /**< (HSMCI_CMDR) 5-cycle max latency. Position  */
#define HSMCI_CMDR_MAXLAT_64                (HSMCI_CMDR_MAXLAT_64_Val << HSMCI_CMDR_MAXLAT_Pos)  /**< (HSMCI_CMDR) 64-cycle max latency. Position  */
#define HSMCI_CMDR_TRCMD_Pos                16                                             /**< (HSMCI_CMDR) Transfer Command Position */
#define HSMCI_CMDR_TRCMD_Msk                (0x3U << HSMCI_CMDR_TRCMD_Pos)                 /**< (HSMCI_CMDR) Transfer Command Mask */
#define HSMCI_CMDR_TRCMD(value)             (HSMCI_CMDR_TRCMD_Msk & ((value) << HSMCI_CMDR_TRCMD_Pos))
#define   HSMCI_CMDR_TRCMD_NO_DATA_Val      (0x0U)                                         /**< (HSMCI_CMDR) No data transfer  */
#define   HSMCI_CMDR_TRCMD_START_DATA_Val   (0x1U)                                         /**< (HSMCI_CMDR) Start data transfer  */
#define   HSMCI_CMDR_TRCMD_STOP_DATA_Val    (0x2U)                                         /**< (HSMCI_CMDR) Stop data transfer  */
#define HSMCI_CMDR_TRCMD_NO_DATA            (HSMCI_CMDR_TRCMD_NO_DATA_Val << HSMCI_CMDR_TRCMD_Pos)  /**< (HSMCI_CMDR) No data transfer Position  */
#define HSMCI_CMDR_TRCMD_START_DATA         (HSMCI_CMDR_TRCMD_START_DATA_Val << HSMCI_CMDR_TRCMD_Pos)  /**< (HSMCI_CMDR) Start data transfer Position  */
#define HSMCI_CMDR_TRCMD_STOP_DATA          (HSMCI_CMDR_TRCMD_STOP_DATA_Val << HSMCI_CMDR_TRCMD_Pos)  /**< (HSMCI_CMDR) Stop data transfer Position  */
#define HSMCI_CMDR_TRDIR_Pos                18                                             /**< (HSMCI_CMDR) Transfer Direction Position */
#define HSMCI_CMDR_TRDIR_Msk                (0x1U << HSMCI_CMDR_TRDIR_Pos)                 /**< (HSMCI_CMDR) Transfer Direction Mask */
#define HSMCI_CMDR_TRDIR                    HSMCI_CMDR_TRDIR_Msk                           /**< \deprecated Old style mask definition for 1 bit bitfield. Use HSMCI_CMDR_TRDIR_Msk instead */
#define   HSMCI_CMDR_TRDIR_WRITE_Val        (0x0U)                                         /**< (HSMCI_CMDR) Write.  */
#define   HSMCI_CMDR_TRDIR_READ_Val         (0x1U)                                         /**< (HSMCI_CMDR) Read.  */
#define HSMCI_CMDR_TRDIR_WRITE              (HSMCI_CMDR_TRDIR_WRITE_Val << HSMCI_CMDR_TRDIR_Pos)  /**< (HSMCI_CMDR) Write. Position  */
#define HSMCI_CMDR_TRDIR_READ               (HSMCI_CMDR_TRDIR_READ_Val << HSMCI_CMDR_TRDIR_Pos)  /**< (HSMCI_CMDR) Read. Position  */
#define HSMCI_CMDR_TRTYP_Pos                19                                             /**< (HSMCI_CMDR) Transfer Type Position */
#define HSMCI_CMDR_TRTYP_Msk                (0x7U << HSMCI_CMDR_TRTYP_Pos)                 /**< (HSMCI_CMDR) Transfer Type Mask */
#define HSMCI_CMDR_TRTYP(value)             (HSMCI_CMDR_TRTYP_Msk & ((value) << HSMCI_CMDR_TRTYP_Pos))
#define   HSMCI_CMDR_TRTYP_SINGLE_Val       (0x0U)                                         /**< (HSMCI_CMDR) MMC/SD Card Single Block  */
#define   HSMCI_CMDR_TRTYP_MULTIPLE_Val     (0x1U)                                         /**< (HSMCI_CMDR) MMC/SD Card Multiple Block  */
#define   HSMCI_CMDR_TRTYP_STREAM_Val       (0x2U)                                         /**< (HSMCI_CMDR) MMC Stream  */
#define   HSMCI_CMDR_TRTYP_BYTE_Val         (0x4U)                                         /**< (HSMCI_CMDR) SDIO Byte  */
#define   HSMCI_CMDR_TRTYP_BLOCK_Val        (0x5U)                                         /**< (HSMCI_CMDR) SDIO Block  */
#define HSMCI_CMDR_TRTYP_SINGLE             (HSMCI_CMDR_TRTYP_SINGLE_Val << HSMCI_CMDR_TRTYP_Pos)  /**< (HSMCI_CMDR) MMC/SD Card Single Block Position  */
#define HSMCI_CMDR_TRTYP_MULTIPLE           (HSMCI_CMDR_TRTYP_MULTIPLE_Val << HSMCI_CMDR_TRTYP_Pos)  /**< (HSMCI_CMDR) MMC/SD Card Multiple Block Position  */
#define HSMCI_CMDR_TRTYP_STREAM             (HSMCI_CMDR_TRTYP_STREAM_Val << HSMCI_CMDR_TRTYP_Pos)  /**< (HSMCI_CMDR) MMC Stream Position  */
#define HSMCI_CMDR_TRTYP_BYTE               (HSMCI_CMDR_TRTYP_BYTE_Val << HSMCI_CMDR_TRTYP_Pos)  /**< (HSMCI_CMDR) SDIO Byte Position  */
#define HSMCI_CMDR_TRTYP_BLOCK              (HSMCI_CMDR_TRTYP_BLOCK_Val << HSMCI_CMDR_TRTYP_Pos)  /**< (HSMCI_CMDR) SDIO Block Position  */
#define HSMCI_CMDR_IOSPCMD_Pos              24                                             /**< (HSMCI_CMDR) SDIO Special Command Position */
#define HSMCI_CMDR_IOSPCMD_Msk              (0x3U << HSMCI_CMDR_IOSPCMD_Pos)               /**< (HSMCI_CMDR) SDIO Special Command Mask */
#define HSMCI_CMDR_IOSPCMD(value)           (HSMCI_CMDR_IOSPCMD_Msk & ((value) << HSMCI_CMDR_IOSPCMD_Pos))
#define   HSMCI_CMDR_IOSPCMD_STD_Val        (0x0U)                                         /**< (HSMCI_CMDR) Not an SDIO Special Command  */
#define   HSMCI_CMDR_IOSPCMD_SUSPEND_Val    (0x1U)                                         /**< (HSMCI_CMDR) SDIO Suspend Command  */
#define   HSMCI_CMDR_IOSPCMD_RESUME_Val     (0x2U)                                         /**< (HSMCI_CMDR) SDIO Resume Command  */
#define HSMCI_CMDR_IOSPCMD_STD              (HSMCI_CMDR_IOSPCMD_STD_Val << HSMCI_CMDR_IOSPCMD_Pos)  /**< (HSMCI_CMDR) Not an SDIO Special Command Position  */
#define HSMCI_CMDR_IOSPCMD_SUSPEND          (HSMCI_CMDR_IOSPCMD_SUSPEND_Val << HSMCI_CMDR_IOSPCMD_Pos)  /**< (HSMCI_CMDR) SDIO Suspend Command Position  */
#define HSMCI_CMDR_IOSPCMD_RESUME           (HSMCI_CMDR_IOSPCMD_RESUME_Val << HSMCI_CMDR_IOSPCMD_Pos)  /**< (HSMCI_CMDR) SDIO Resume Command Position  */
#define HSMCI_CMDR_ATACS_Pos                26                                             /**< (HSMCI_CMDR) ATA with Command Completion Signal Position */
#define HSMCI_CMDR_ATACS_Msk                (0x1U << HSMCI_CMDR_ATACS_Pos)                 /**< (HSMCI_CMDR) ATA with Command Completion Signal Mask */
#define HSMCI_CMDR_ATACS                    HSMCI_CMDR_ATACS_Msk                           /**< \deprecated Old style mask definition for 1 bit bitfield. Use HSMCI_CMDR_ATACS_Msk instead */
#define   HSMCI_CMDR_ATACS_NORMAL_Val       (0x0U)                                         /**< (HSMCI_CMDR) Normal operation mode.  */
#define   HSMCI_CMDR_ATACS_COMPLETION_Val   (0x1U)                                         /**< (HSMCI_CMDR) This bit indicates that a completion signal is expected within a programmed amount of time (HSMCI_CSTOR).  */
#define HSMCI_CMDR_ATACS_NORMAL             (HSMCI_CMDR_ATACS_NORMAL_Val << HSMCI_CMDR_ATACS_Pos)  /**< (HSMCI_CMDR) Normal operation mode. Position  */
#define HSMCI_CMDR_ATACS_COMPLETION         (HSMCI_CMDR_ATACS_COMPLETION_Val << HSMCI_CMDR_ATACS_Pos)  /**< (HSMCI_CMDR) This bit indicates that a completion signal is expected within a programmed amount of time (HSMCI_CSTOR). Position  */
#define HSMCI_CMDR_BOOT_ACK_Pos             27                                             /**< (HSMCI_CMDR) Boot Operation Acknowledge Position */
#define HSMCI_CMDR_BOOT_ACK_Msk             (0x1U << HSMCI_CMDR_BOOT_ACK_Pos)              /**< (HSMCI_CMDR) Boot Operation Acknowledge Mask */
#define HSMCI_CMDR_BOOT_ACK                 HSMCI_CMDR_BOOT_ACK_Msk                        /**< \deprecated Old style mask definition for 1 bit bitfield. Use HSMCI_CMDR_BOOT_ACK_Msk instead */
#define HSMCI_CMDR_MASK                     (0xF3F1FFFU)                                   /**< \deprecated (HSMCI_CMDR) Register MASK  (Use HSMCI_CMDR_Msk instead)  */
#define HSMCI_CMDR_Msk                      (0xF3F1FFFU)                                   /**< (HSMCI_CMDR) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- HSMCI_BLKR : (HSMCI Offset: 0x18) (R/W 32) Block Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t BCNT:16;                   /**< bit:  0..15  MMC/SDIO Block Count - SDIO Byte Count   */
    uint32_t BLKLEN:16;                 /**< bit: 16..31  Data Block Length                        */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} HSMCI_BLKR_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define HSMCI_BLKR_OFFSET                   (0x18)                                        /**<  (HSMCI_BLKR) Block Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define HSMCI_BLKR_BCNT_Pos                 0                                              /**< (HSMCI_BLKR) MMC/SDIO Block Count - SDIO Byte Count Position */
#define HSMCI_BLKR_BCNT_Msk                 (0xFFFFU << HSMCI_BLKR_BCNT_Pos)               /**< (HSMCI_BLKR) MMC/SDIO Block Count - SDIO Byte Count Mask */
#define HSMCI_BLKR_BCNT(value)              (HSMCI_BLKR_BCNT_Msk & ((value) << HSMCI_BLKR_BCNT_Pos))
#define HSMCI_BLKR_BLKLEN_Pos               16                                             /**< (HSMCI_BLKR) Data Block Length Position */
#define HSMCI_BLKR_BLKLEN_Msk               (0xFFFFU << HSMCI_BLKR_BLKLEN_Pos)             /**< (HSMCI_BLKR) Data Block Length Mask */
#define HSMCI_BLKR_BLKLEN(value)            (HSMCI_BLKR_BLKLEN_Msk & ((value) << HSMCI_BLKR_BLKLEN_Pos))
#define HSMCI_BLKR_MASK                     (0xFFFFFFFFU)                                  /**< \deprecated (HSMCI_BLKR) Register MASK  (Use HSMCI_BLKR_Msk instead)  */
#define HSMCI_BLKR_Msk                      (0xFFFFFFFFU)                                  /**< (HSMCI_BLKR) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- HSMCI_CSTOR : (HSMCI Offset: 0x1c) (R/W 32) Completion Signal Timeout Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t CSTOCYC:4;                 /**< bit:   0..3  Completion Signal Timeout Cycle Number   */
    uint32_t CSTOMUL:3;                 /**< bit:   4..6  Completion Signal Timeout Multiplier     */
    uint32_t :25;                       /**< bit:  7..31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} HSMCI_CSTOR_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define HSMCI_CSTOR_OFFSET                  (0x1C)                                        /**<  (HSMCI_CSTOR) Completion Signal Timeout Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define HSMCI_CSTOR_CSTOCYC_Pos             0                                              /**< (HSMCI_CSTOR) Completion Signal Timeout Cycle Number Position */
#define HSMCI_CSTOR_CSTOCYC_Msk             (0xFU << HSMCI_CSTOR_CSTOCYC_Pos)              /**< (HSMCI_CSTOR) Completion Signal Timeout Cycle Number Mask */
#define HSMCI_CSTOR_CSTOCYC(value)          (HSMCI_CSTOR_CSTOCYC_Msk & ((value) << HSMCI_CSTOR_CSTOCYC_Pos))
#define HSMCI_CSTOR_CSTOMUL_Pos             4                                              /**< (HSMCI_CSTOR) Completion Signal Timeout Multiplier Position */
#define HSMCI_CSTOR_CSTOMUL_Msk             (0x7U << HSMCI_CSTOR_CSTOMUL_Pos)              /**< (HSMCI_CSTOR) Completion Signal Timeout Multiplier Mask */
#define HSMCI_CSTOR_CSTOMUL(value)          (HSMCI_CSTOR_CSTOMUL_Msk & ((value) << HSMCI_CSTOR_CSTOMUL_Pos))
#define   HSMCI_CSTOR_CSTOMUL_1_Val         (0x0U)                                         /**< (HSMCI_CSTOR) CSTOCYC x 1  */
#define   HSMCI_CSTOR_CSTOMUL_16_Val        (0x1U)                                         /**< (HSMCI_CSTOR) CSTOCYC x 16  */
#define   HSMCI_CSTOR_CSTOMUL_128_Val       (0x2U)                                         /**< (HSMCI_CSTOR) CSTOCYC x 128  */
#define   HSMCI_CSTOR_CSTOMUL_256_Val       (0x3U)                                         /**< (HSMCI_CSTOR) CSTOCYC x 256  */
#define   HSMCI_CSTOR_CSTOMUL_1024_Val      (0x4U)                                         /**< (HSMCI_CSTOR) CSTOCYC x 1024  */
#define   HSMCI_CSTOR_CSTOMUL_4096_Val      (0x5U)                                         /**< (HSMCI_CSTOR) CSTOCYC x 4096  */
#define   HSMCI_CSTOR_CSTOMUL_65536_Val     (0x6U)                                         /**< (HSMCI_CSTOR) CSTOCYC x 65536  */
#define   HSMCI_CSTOR_CSTOMUL_1048576_Val   (0x7U)                                         /**< (HSMCI_CSTOR) CSTOCYC x 1048576  */
#define HSMCI_CSTOR_CSTOMUL_1               (HSMCI_CSTOR_CSTOMUL_1_Val << HSMCI_CSTOR_CSTOMUL_Pos)  /**< (HSMCI_CSTOR) CSTOCYC x 1 Position  */
#define HSMCI_CSTOR_CSTOMUL_16              (HSMCI_CSTOR_CSTOMUL_16_Val << HSMCI_CSTOR_CSTOMUL_Pos)  /**< (HSMCI_CSTOR) CSTOCYC x 16 Position  */
#define HSMCI_CSTOR_CSTOMUL_128             (HSMCI_CSTOR_CSTOMUL_128_Val << HSMCI_CSTOR_CSTOMUL_Pos)  /**< (HSMCI_CSTOR) CSTOCYC x 128 Position  */
#define HSMCI_CSTOR_CSTOMUL_256             (HSMCI_CSTOR_CSTOMUL_256_Val << HSMCI_CSTOR_CSTOMUL_Pos)  /**< (HSMCI_CSTOR) CSTOCYC x 256 Position  */
#define HSMCI_CSTOR_CSTOMUL_1024            (HSMCI_CSTOR_CSTOMUL_1024_Val << HSMCI_CSTOR_CSTOMUL_Pos)  /**< (HSMCI_CSTOR) CSTOCYC x 1024 Position  */
#define HSMCI_CSTOR_CSTOMUL_4096            (HSMCI_CSTOR_CSTOMUL_4096_Val << HSMCI_CSTOR_CSTOMUL_Pos)  /**< (HSMCI_CSTOR) CSTOCYC x 4096 Position  */
#define HSMCI_CSTOR_CSTOMUL_65536           (HSMCI_CSTOR_CSTOMUL_65536_Val << HSMCI_CSTOR_CSTOMUL_Pos)  /**< (HSMCI_CSTOR) CSTOCYC x 65536 Position  */
#define HSMCI_CSTOR_CSTOMUL_1048576         (HSMCI_CSTOR_CSTOMUL_1048576_Val << HSMCI_CSTOR_CSTOMUL_Pos)  /**< (HSMCI_CSTOR) CSTOCYC x 1048576 Position  */
#define HSMCI_CSTOR_MASK                    (0x7FU)                                        /**< \deprecated (HSMCI_CSTOR) Register MASK  (Use HSMCI_CSTOR_Msk instead)  */
#define HSMCI_CSTOR_Msk                     (0x7FU)                                        /**< (HSMCI_CSTOR) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- HSMCI_RSPR : (HSMCI Offset: 0x20) (R/ 32) Response Register 0 -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t RSP:32;                    /**< bit:  0..31  Response                                 */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} HSMCI_RSPR_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define HSMCI_RSPR_OFFSET                   (0x20)                                        /**<  (HSMCI_RSPR) Response Register 0  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define HSMCI_RSPR_RSP_Pos                  0                                              /**< (HSMCI_RSPR) Response Position */
#define HSMCI_RSPR_RSP_Msk                  (0xFFFFFFFFU << HSMCI_RSPR_RSP_Pos)            /**< (HSMCI_RSPR) Response Mask */
#define HSMCI_RSPR_RSP(value)               (HSMCI_RSPR_RSP_Msk & ((value) << HSMCI_RSPR_RSP_Pos))
#define HSMCI_RSPR_MASK                     (0xFFFFFFFFU)                                  /**< \deprecated (HSMCI_RSPR) Register MASK  (Use HSMCI_RSPR_Msk instead)  */
#define HSMCI_RSPR_Msk                      (0xFFFFFFFFU)                                  /**< (HSMCI_RSPR) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- HSMCI_RDR : (HSMCI Offset: 0x30) (R/ 32) Receive Data Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t DATA:32;                   /**< bit:  0..31  Data to Read                             */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} HSMCI_RDR_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define HSMCI_RDR_OFFSET                    (0x30)                                        /**<  (HSMCI_RDR) Receive Data Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define HSMCI_RDR_DATA_Pos                  0                                              /**< (HSMCI_RDR) Data to Read Position */
#define HSMCI_RDR_DATA_Msk                  (0xFFFFFFFFU << HSMCI_RDR_DATA_Pos)            /**< (HSMCI_RDR) Data to Read Mask */
#define HSMCI_RDR_DATA(value)               (HSMCI_RDR_DATA_Msk & ((value) << HSMCI_RDR_DATA_Pos))
#define HSMCI_RDR_MASK                      (0xFFFFFFFFU)                                  /**< \deprecated (HSMCI_RDR) Register MASK  (Use HSMCI_RDR_Msk instead)  */
#define HSMCI_RDR_Msk                       (0xFFFFFFFFU)                                  /**< (HSMCI_RDR) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- HSMCI_TDR : (HSMCI Offset: 0x34) (/W 32) Transmit Data Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t DATA:32;                   /**< bit:  0..31  Data to Write                            */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} HSMCI_TDR_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define HSMCI_TDR_OFFSET                    (0x34)                                        /**<  (HSMCI_TDR) Transmit Data Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define HSMCI_TDR_DATA_Pos                  0                                              /**< (HSMCI_TDR) Data to Write Position */
#define HSMCI_TDR_DATA_Msk                  (0xFFFFFFFFU << HSMCI_TDR_DATA_Pos)            /**< (HSMCI_TDR) Data to Write Mask */
#define HSMCI_TDR_DATA(value)               (HSMCI_TDR_DATA_Msk & ((value) << HSMCI_TDR_DATA_Pos))
#define HSMCI_TDR_MASK                      (0xFFFFFFFFU)                                  /**< \deprecated (HSMCI_TDR) Register MASK  (Use HSMCI_TDR_Msk instead)  */
#define HSMCI_TDR_Msk                       (0xFFFFFFFFU)                                  /**< (HSMCI_TDR) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- HSMCI_SR : (HSMCI Offset: 0x40) (R/ 32) Status Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t CMDRDY:1;                  /**< bit:      0  Command Ready (cleared by writing in HSMCI_CMDR) */
    uint32_t RXRDY:1;                   /**< bit:      1  Receiver Ready (cleared by reading HSMCI_RDR) */
    uint32_t TXRDY:1;                   /**< bit:      2  Transmit Ready (cleared by writing in HSMCI_TDR) */
    uint32_t BLKE:1;                    /**< bit:      3  Data Block Ended (cleared on read)       */
    uint32_t DTIP:1;                    /**< bit:      4  Data Transfer in Progress (cleared at the end of CRC16 calculation) */
    uint32_t NOTBUSY:1;                 /**< bit:      5  HSMCI Not Busy                           */
    uint32_t :2;                        /**< bit:   6..7  Reserved */
    uint32_t SDIOIRQA:1;                /**< bit:      8  SDIO Interrupt for Slot A (cleared on read) */
    uint32_t :3;                        /**< bit:  9..11  Reserved */
    uint32_t SDIOWAIT:1;                /**< bit:     12  SDIO Read Wait Operation Status          */
    uint32_t CSRCV:1;                   /**< bit:     13  CE-ATA Completion Signal Received (cleared on read) */
    uint32_t :2;                        /**< bit: 14..15  Reserved */
    uint32_t RINDE:1;                   /**< bit:     16  Response Index Error (cleared by writing in HSMCI_CMDR) */
    uint32_t RDIRE:1;                   /**< bit:     17  Response Direction Error (cleared by writing in HSMCI_CMDR) */
    uint32_t RCRCE:1;                   /**< bit:     18  Response CRC Error (cleared by writing in HSMCI_CMDR) */
    uint32_t RENDE:1;                   /**< bit:     19  Response End Bit Error (cleared by writing in HSMCI_CMDR) */
    uint32_t RTOE:1;                    /**< bit:     20  Response Time-out Error (cleared by writing in HSMCI_CMDR) */
    uint32_t DCRCE:1;                   /**< bit:     21  Data CRC Error (cleared on read)         */
    uint32_t DTOE:1;                    /**< bit:     22  Data Time-out Error (cleared on read)    */
    uint32_t CSTOE:1;                   /**< bit:     23  Completion Signal Time-out Error (cleared on read) */
    uint32_t BLKOVRE:1;                 /**< bit:     24  DMA Block Overrun Error (cleared on read) */
    uint32_t :1;                        /**< bit:     25  Reserved */
    uint32_t FIFOEMPTY:1;               /**< bit:     26  FIFO empty flag                          */
    uint32_t XFRDONE:1;                 /**< bit:     27  Transfer Done flag                       */
    uint32_t ACKRCV:1;                  /**< bit:     28  Boot Operation Acknowledge Received  (cleared on read) */
    uint32_t ACKRCVE:1;                 /**< bit:     29  Boot Operation Acknowledge Error (cleared on read) */
    uint32_t OVRE:1;                    /**< bit:     30  Overrun (if FERRCTRL = 1, cleared by writing in HSMCI_CMDR or cleared on read if FERRCTRL = 0) */
    uint32_t UNRE:1;                    /**< bit:     31  Underrun (if FERRCTRL = 1, cleared by writing in HSMCI_CMDR or cleared on read if FERRCTRL = 0) */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} HSMCI_SR_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define HSMCI_SR_OFFSET                     (0x40)                                        /**<  (HSMCI_SR) Status Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define HSMCI_SR_CMDRDY_Pos                 0                                              /**< (HSMCI_SR) Command Ready (cleared by writing in HSMCI_CMDR) Position */
#define HSMCI_SR_CMDRDY_Msk                 (0x1U << HSMCI_SR_CMDRDY_Pos)                  /**< (HSMCI_SR) Command Ready (cleared by writing in HSMCI_CMDR) Mask */
#define HSMCI_SR_CMDRDY                     HSMCI_SR_CMDRDY_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use HSMCI_SR_CMDRDY_Msk instead */
#define HSMCI_SR_RXRDY_Pos                  1                                              /**< (HSMCI_SR) Receiver Ready (cleared by reading HSMCI_RDR) Position */
#define HSMCI_SR_RXRDY_Msk                  (0x1U << HSMCI_SR_RXRDY_Pos)                   /**< (HSMCI_SR) Receiver Ready (cleared by reading HSMCI_RDR) Mask */
#define HSMCI_SR_RXRDY                      HSMCI_SR_RXRDY_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use HSMCI_SR_RXRDY_Msk instead */
#define HSMCI_SR_TXRDY_Pos                  2                                              /**< (HSMCI_SR) Transmit Ready (cleared by writing in HSMCI_TDR) Position */
#define HSMCI_SR_TXRDY_Msk                  (0x1U << HSMCI_SR_TXRDY_Pos)                   /**< (HSMCI_SR) Transmit Ready (cleared by writing in HSMCI_TDR) Mask */
#define HSMCI_SR_TXRDY                      HSMCI_SR_TXRDY_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use HSMCI_SR_TXRDY_Msk instead */
#define HSMCI_SR_BLKE_Pos                   3                                              /**< (HSMCI_SR) Data Block Ended (cleared on read) Position */
#define HSMCI_SR_BLKE_Msk                   (0x1U << HSMCI_SR_BLKE_Pos)                    /**< (HSMCI_SR) Data Block Ended (cleared on read) Mask */
#define HSMCI_SR_BLKE                       HSMCI_SR_BLKE_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use HSMCI_SR_BLKE_Msk instead */
#define HSMCI_SR_DTIP_Pos                   4                                              /**< (HSMCI_SR) Data Transfer in Progress (cleared at the end of CRC16 calculation) Position */
#define HSMCI_SR_DTIP_Msk                   (0x1U << HSMCI_SR_DTIP_Pos)                    /**< (HSMCI_SR) Data Transfer in Progress (cleared at the end of CRC16 calculation) Mask */
#define HSMCI_SR_DTIP                       HSMCI_SR_DTIP_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use HSMCI_SR_DTIP_Msk instead */
#define HSMCI_SR_NOTBUSY_Pos                5                                              /**< (HSMCI_SR) HSMCI Not Busy Position */
#define HSMCI_SR_NOTBUSY_Msk                (0x1U << HSMCI_SR_NOTBUSY_Pos)                 /**< (HSMCI_SR) HSMCI Not Busy Mask */
#define HSMCI_SR_NOTBUSY                    HSMCI_SR_NOTBUSY_Msk                           /**< \deprecated Old style mask definition for 1 bit bitfield. Use HSMCI_SR_NOTBUSY_Msk instead */
#define HSMCI_SR_SDIOIRQA_Pos               8                                              /**< (HSMCI_SR) SDIO Interrupt for Slot A (cleared on read) Position */
#define HSMCI_SR_SDIOIRQA_Msk               (0x1U << HSMCI_SR_SDIOIRQA_Pos)                /**< (HSMCI_SR) SDIO Interrupt for Slot A (cleared on read) Mask */
#define HSMCI_SR_SDIOIRQA                   HSMCI_SR_SDIOIRQA_Msk                          /**< \deprecated Old style mask definition for 1 bit bitfield. Use HSMCI_SR_SDIOIRQA_Msk instead */
#define HSMCI_SR_SDIOWAIT_Pos               12                                             /**< (HSMCI_SR) SDIO Read Wait Operation Status Position */
#define HSMCI_SR_SDIOWAIT_Msk               (0x1U << HSMCI_SR_SDIOWAIT_Pos)                /**< (HSMCI_SR) SDIO Read Wait Operation Status Mask */
#define HSMCI_SR_SDIOWAIT                   HSMCI_SR_SDIOWAIT_Msk                          /**< \deprecated Old style mask definition for 1 bit bitfield. Use HSMCI_SR_SDIOWAIT_Msk instead */
#define HSMCI_SR_CSRCV_Pos                  13                                             /**< (HSMCI_SR) CE-ATA Completion Signal Received (cleared on read) Position */
#define HSMCI_SR_CSRCV_Msk                  (0x1U << HSMCI_SR_CSRCV_Pos)                   /**< (HSMCI_SR) CE-ATA Completion Signal Received (cleared on read) Mask */
#define HSMCI_SR_CSRCV                      HSMCI_SR_CSRCV_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use HSMCI_SR_CSRCV_Msk instead */
#define HSMCI_SR_RINDE_Pos                  16                                             /**< (HSMCI_SR) Response Index Error (cleared by writing in HSMCI_CMDR) Position */
#define HSMCI_SR_RINDE_Msk                  (0x1U << HSMCI_SR_RINDE_Pos)                   /**< (HSMCI_SR) Response Index Error (cleared by writing in HSMCI_CMDR) Mask */
#define HSMCI_SR_RINDE                      HSMCI_SR_RINDE_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use HSMCI_SR_RINDE_Msk instead */
#define HSMCI_SR_RDIRE_Pos                  17                                             /**< (HSMCI_SR) Response Direction Error (cleared by writing in HSMCI_CMDR) Position */
#define HSMCI_SR_RDIRE_Msk                  (0x1U << HSMCI_SR_RDIRE_Pos)                   /**< (HSMCI_SR) Response Direction Error (cleared by writing in HSMCI_CMDR) Mask */
#define HSMCI_SR_RDIRE                      HSMCI_SR_RDIRE_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use HSMCI_SR_RDIRE_Msk instead */
#define HSMCI_SR_RCRCE_Pos                  18                                             /**< (HSMCI_SR) Response CRC Error (cleared by writing in HSMCI_CMDR) Position */
#define HSMCI_SR_RCRCE_Msk                  (0x1U << HSMCI_SR_RCRCE_Pos)                   /**< (HSMCI_SR) Response CRC Error (cleared by writing in HSMCI_CMDR) Mask */
#define HSMCI_SR_RCRCE                      HSMCI_SR_RCRCE_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use HSMCI_SR_RCRCE_Msk instead */
#define HSMCI_SR_RENDE_Pos                  19                                             /**< (HSMCI_SR) Response End Bit Error (cleared by writing in HSMCI_CMDR) Position */
#define HSMCI_SR_RENDE_Msk                  (0x1U << HSMCI_SR_RENDE_Pos)                   /**< (HSMCI_SR) Response End Bit Error (cleared by writing in HSMCI_CMDR) Mask */
#define HSMCI_SR_RENDE                      HSMCI_SR_RENDE_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use HSMCI_SR_RENDE_Msk instead */
#define HSMCI_SR_RTOE_Pos                   20                                             /**< (HSMCI_SR) Response Time-out Error (cleared by writing in HSMCI_CMDR) Position */
#define HSMCI_SR_RTOE_Msk                   (0x1U << HSMCI_SR_RTOE_Pos)                    /**< (HSMCI_SR) Response Time-out Error (cleared by writing in HSMCI_CMDR) Mask */
#define HSMCI_SR_RTOE                       HSMCI_SR_RTOE_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use HSMCI_SR_RTOE_Msk instead */
#define HSMCI_SR_DCRCE_Pos                  21                                             /**< (HSMCI_SR) Data CRC Error (cleared on read) Position */
#define HSMCI_SR_DCRCE_Msk                  (0x1U << HSMCI_SR_DCRCE_Pos)                   /**< (HSMCI_SR) Data CRC Error (cleared on read) Mask */
#define HSMCI_SR_DCRCE                      HSMCI_SR_DCRCE_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use HSMCI_SR_DCRCE_Msk instead */
#define HSMCI_SR_DTOE_Pos                   22                                             /**< (HSMCI_SR) Data Time-out Error (cleared on read) Position */
#define HSMCI_SR_DTOE_Msk                   (0x1U << HSMCI_SR_DTOE_Pos)                    /**< (HSMCI_SR) Data Time-out Error (cleared on read) Mask */
#define HSMCI_SR_DTOE                       HSMCI_SR_DTOE_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use HSMCI_SR_DTOE_Msk instead */
#define HSMCI_SR_CSTOE_Pos                  23                                             /**< (HSMCI_SR) Completion Signal Time-out Error (cleared on read) Position */
#define HSMCI_SR_CSTOE_Msk                  (0x1U << HSMCI_SR_CSTOE_Pos)                   /**< (HSMCI_SR) Completion Signal Time-out Error (cleared on read) Mask */
#define HSMCI_SR_CSTOE                      HSMCI_SR_CSTOE_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use HSMCI_SR_CSTOE_Msk instead */
#define HSMCI_SR_BLKOVRE_Pos                24                                             /**< (HSMCI_SR) DMA Block Overrun Error (cleared on read) Position */
#define HSMCI_SR_BLKOVRE_Msk                (0x1U << HSMCI_SR_BLKOVRE_Pos)                 /**< (HSMCI_SR) DMA Block Overrun Error (cleared on read) Mask */
#define HSMCI_SR_BLKOVRE                    HSMCI_SR_BLKOVRE_Msk                           /**< \deprecated Old style mask definition for 1 bit bitfield. Use HSMCI_SR_BLKOVRE_Msk instead */
#define HSMCI_SR_FIFOEMPTY_Pos              26                                             /**< (HSMCI_SR) FIFO empty flag Position */
#define HSMCI_SR_FIFOEMPTY_Msk              (0x1U << HSMCI_SR_FIFOEMPTY_Pos)               /**< (HSMCI_SR) FIFO empty flag Mask */
#define HSMCI_SR_FIFOEMPTY                  HSMCI_SR_FIFOEMPTY_Msk                         /**< \deprecated Old style mask definition for 1 bit bitfield. Use HSMCI_SR_FIFOEMPTY_Msk instead */
#define HSMCI_SR_XFRDONE_Pos                27                                             /**< (HSMCI_SR) Transfer Done flag Position */
#define HSMCI_SR_XFRDONE_Msk                (0x1U << HSMCI_SR_XFRDONE_Pos)                 /**< (HSMCI_SR) Transfer Done flag Mask */
#define HSMCI_SR_XFRDONE                    HSMCI_SR_XFRDONE_Msk                           /**< \deprecated Old style mask definition for 1 bit bitfield. Use HSMCI_SR_XFRDONE_Msk instead */
#define HSMCI_SR_ACKRCV_Pos                 28                                             /**< (HSMCI_SR) Boot Operation Acknowledge Received  (cleared on read) Position */
#define HSMCI_SR_ACKRCV_Msk                 (0x1U << HSMCI_SR_ACKRCV_Pos)                  /**< (HSMCI_SR) Boot Operation Acknowledge Received  (cleared on read) Mask */
#define HSMCI_SR_ACKRCV                     HSMCI_SR_ACKRCV_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use HSMCI_SR_ACKRCV_Msk instead */
#define HSMCI_SR_ACKRCVE_Pos                29                                             /**< (HSMCI_SR) Boot Operation Acknowledge Error (cleared on read) Position */
#define HSMCI_SR_ACKRCVE_Msk                (0x1U << HSMCI_SR_ACKRCVE_Pos)                 /**< (HSMCI_SR) Boot Operation Acknowledge Error (cleared on read) Mask */
#define HSMCI_SR_ACKRCVE                    HSMCI_SR_ACKRCVE_Msk                           /**< \deprecated Old style mask definition for 1 bit bitfield. Use HSMCI_SR_ACKRCVE_Msk instead */
#define HSMCI_SR_OVRE_Pos                   30                                             /**< (HSMCI_SR) Overrun (if FERRCTRL = 1, cleared by writing in HSMCI_CMDR or cleared on read if FERRCTRL = 0) Position */
#define HSMCI_SR_OVRE_Msk                   (0x1U << HSMCI_SR_OVRE_Pos)                    /**< (HSMCI_SR) Overrun (if FERRCTRL = 1, cleared by writing in HSMCI_CMDR or cleared on read if FERRCTRL = 0) Mask */
#define HSMCI_SR_OVRE                       HSMCI_SR_OVRE_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use HSMCI_SR_OVRE_Msk instead */
#define HSMCI_SR_UNRE_Pos                   31                                             /**< (HSMCI_SR) Underrun (if FERRCTRL = 1, cleared by writing in HSMCI_CMDR or cleared on read if FERRCTRL = 0) Position */
#define HSMCI_SR_UNRE_Msk                   (0x1U << HSMCI_SR_UNRE_Pos)                    /**< (HSMCI_SR) Underrun (if FERRCTRL = 1, cleared by writing in HSMCI_CMDR or cleared on read if FERRCTRL = 0) Mask */
#define HSMCI_SR_UNRE                       HSMCI_SR_UNRE_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use HSMCI_SR_UNRE_Msk instead */
#define HSMCI_SR_MASK                       (0xFDFF313FU)                                  /**< \deprecated (HSMCI_SR) Register MASK  (Use HSMCI_SR_Msk instead)  */
#define HSMCI_SR_Msk                        (0xFDFF313FU)                                  /**< (HSMCI_SR) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- HSMCI_IER : (HSMCI Offset: 0x44) (/W 32) Interrupt Enable Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t CMDRDY:1;                  /**< bit:      0  Command Ready Interrupt Enable           */
    uint32_t RXRDY:1;                   /**< bit:      1  Receiver Ready Interrupt Enable          */
    uint32_t TXRDY:1;                   /**< bit:      2  Transmit Ready Interrupt Enable          */
    uint32_t BLKE:1;                    /**< bit:      3  Data Block Ended Interrupt Enable        */
    uint32_t DTIP:1;                    /**< bit:      4  Data Transfer in Progress Interrupt Enable */
    uint32_t NOTBUSY:1;                 /**< bit:      5  Data Not Busy Interrupt Enable           */
    uint32_t :2;                        /**< bit:   6..7  Reserved */
    uint32_t SDIOIRQA:1;                /**< bit:      8  SDIO Interrupt for Slot A Interrupt Enable */
    uint32_t :3;                        /**< bit:  9..11  Reserved */
    uint32_t SDIOWAIT:1;                /**< bit:     12  SDIO Read Wait Operation Status Interrupt Enable */
    uint32_t CSRCV:1;                   /**< bit:     13  Completion Signal Received Interrupt Enable */
    uint32_t :2;                        /**< bit: 14..15  Reserved */
    uint32_t RINDE:1;                   /**< bit:     16  Response Index Error Interrupt Enable    */
    uint32_t RDIRE:1;                   /**< bit:     17  Response Direction Error Interrupt Enable */
    uint32_t RCRCE:1;                   /**< bit:     18  Response CRC Error Interrupt Enable      */
    uint32_t RENDE:1;                   /**< bit:     19  Response End Bit Error Interrupt Enable  */
    uint32_t RTOE:1;                    /**< bit:     20  Response Time-out Error Interrupt Enable */
    uint32_t DCRCE:1;                   /**< bit:     21  Data CRC Error Interrupt Enable          */
    uint32_t DTOE:1;                    /**< bit:     22  Data Time-out Error Interrupt Enable     */
    uint32_t CSTOE:1;                   /**< bit:     23  Completion Signal Timeout Error Interrupt Enable */
    uint32_t BLKOVRE:1;                 /**< bit:     24  DMA Block Overrun Error Interrupt Enable */
    uint32_t :1;                        /**< bit:     25  Reserved */
    uint32_t FIFOEMPTY:1;               /**< bit:     26  FIFO empty Interrupt enable              */
    uint32_t XFRDONE:1;                 /**< bit:     27  Transfer Done Interrupt enable           */
    uint32_t ACKRCV:1;                  /**< bit:     28  Boot Acknowledge Interrupt Enable        */
    uint32_t ACKRCVE:1;                 /**< bit:     29  Boot Acknowledge Error Interrupt Enable  */
    uint32_t OVRE:1;                    /**< bit:     30  Overrun Interrupt Enable                 */
    uint32_t UNRE:1;                    /**< bit:     31  Underrun Interrupt Enable                */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} HSMCI_IER_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define HSMCI_IER_OFFSET                    (0x44)                                        /**<  (HSMCI_IER) Interrupt Enable Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define HSMCI_IER_CMDRDY_Pos                0                                              /**< (HSMCI_IER) Command Ready Interrupt Enable Position */
#define HSMCI_IER_CMDRDY_Msk                (0x1U << HSMCI_IER_CMDRDY_Pos)                 /**< (HSMCI_IER) Command Ready Interrupt Enable Mask */
#define HSMCI_IER_CMDRDY                    HSMCI_IER_CMDRDY_Msk                           /**< \deprecated Old style mask definition for 1 bit bitfield. Use HSMCI_IER_CMDRDY_Msk instead */
#define HSMCI_IER_RXRDY_Pos                 1                                              /**< (HSMCI_IER) Receiver Ready Interrupt Enable Position */
#define HSMCI_IER_RXRDY_Msk                 (0x1U << HSMCI_IER_RXRDY_Pos)                  /**< (HSMCI_IER) Receiver Ready Interrupt Enable Mask */
#define HSMCI_IER_RXRDY                     HSMCI_IER_RXRDY_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use HSMCI_IER_RXRDY_Msk instead */
#define HSMCI_IER_TXRDY_Pos                 2                                              /**< (HSMCI_IER) Transmit Ready Interrupt Enable Position */
#define HSMCI_IER_TXRDY_Msk                 (0x1U << HSMCI_IER_TXRDY_Pos)                  /**< (HSMCI_IER) Transmit Ready Interrupt Enable Mask */
#define HSMCI_IER_TXRDY                     HSMCI_IER_TXRDY_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use HSMCI_IER_TXRDY_Msk instead */
#define HSMCI_IER_BLKE_Pos                  3                                              /**< (HSMCI_IER) Data Block Ended Interrupt Enable Position */
#define HSMCI_IER_BLKE_Msk                  (0x1U << HSMCI_IER_BLKE_Pos)                   /**< (HSMCI_IER) Data Block Ended Interrupt Enable Mask */
#define HSMCI_IER_BLKE                      HSMCI_IER_BLKE_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use HSMCI_IER_BLKE_Msk instead */
#define HSMCI_IER_DTIP_Pos                  4                                              /**< (HSMCI_IER) Data Transfer in Progress Interrupt Enable Position */
#define HSMCI_IER_DTIP_Msk                  (0x1U << HSMCI_IER_DTIP_Pos)                   /**< (HSMCI_IER) Data Transfer in Progress Interrupt Enable Mask */
#define HSMCI_IER_DTIP                      HSMCI_IER_DTIP_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use HSMCI_IER_DTIP_Msk instead */
#define HSMCI_IER_NOTBUSY_Pos               5                                              /**< (HSMCI_IER) Data Not Busy Interrupt Enable Position */
#define HSMCI_IER_NOTBUSY_Msk               (0x1U << HSMCI_IER_NOTBUSY_Pos)                /**< (HSMCI_IER) Data Not Busy Interrupt Enable Mask */
#define HSMCI_IER_NOTBUSY                   HSMCI_IER_NOTBUSY_Msk                          /**< \deprecated Old style mask definition for 1 bit bitfield. Use HSMCI_IER_NOTBUSY_Msk instead */
#define HSMCI_IER_SDIOIRQA_Pos              8                                              /**< (HSMCI_IER) SDIO Interrupt for Slot A Interrupt Enable Position */
#define HSMCI_IER_SDIOIRQA_Msk              (0x1U << HSMCI_IER_SDIOIRQA_Pos)               /**< (HSMCI_IER) SDIO Interrupt for Slot A Interrupt Enable Mask */
#define HSMCI_IER_SDIOIRQA                  HSMCI_IER_SDIOIRQA_Msk                         /**< \deprecated Old style mask definition for 1 bit bitfield. Use HSMCI_IER_SDIOIRQA_Msk instead */
#define HSMCI_IER_SDIOWAIT_Pos              12                                             /**< (HSMCI_IER) SDIO Read Wait Operation Status Interrupt Enable Position */
#define HSMCI_IER_SDIOWAIT_Msk              (0x1U << HSMCI_IER_SDIOWAIT_Pos)               /**< (HSMCI_IER) SDIO Read Wait Operation Status Interrupt Enable Mask */
#define HSMCI_IER_SDIOWAIT                  HSMCI_IER_SDIOWAIT_Msk                         /**< \deprecated Old style mask definition for 1 bit bitfield. Use HSMCI_IER_SDIOWAIT_Msk instead */
#define HSMCI_IER_CSRCV_Pos                 13                                             /**< (HSMCI_IER) Completion Signal Received Interrupt Enable Position */
#define HSMCI_IER_CSRCV_Msk                 (0x1U << HSMCI_IER_CSRCV_Pos)                  /**< (HSMCI_IER) Completion Signal Received Interrupt Enable Mask */
#define HSMCI_IER_CSRCV                     HSMCI_IER_CSRCV_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use HSMCI_IER_CSRCV_Msk instead */
#define HSMCI_IER_RINDE_Pos                 16                                             /**< (HSMCI_IER) Response Index Error Interrupt Enable Position */
#define HSMCI_IER_RINDE_Msk                 (0x1U << HSMCI_IER_RINDE_Pos)                  /**< (HSMCI_IER) Response Index Error Interrupt Enable Mask */
#define HSMCI_IER_RINDE                     HSMCI_IER_RINDE_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use HSMCI_IER_RINDE_Msk instead */
#define HSMCI_IER_RDIRE_Pos                 17                                             /**< (HSMCI_IER) Response Direction Error Interrupt Enable Position */
#define HSMCI_IER_RDIRE_Msk                 (0x1U << HSMCI_IER_RDIRE_Pos)                  /**< (HSMCI_IER) Response Direction Error Interrupt Enable Mask */
#define HSMCI_IER_RDIRE                     HSMCI_IER_RDIRE_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use HSMCI_IER_RDIRE_Msk instead */
#define HSMCI_IER_RCRCE_Pos                 18                                             /**< (HSMCI_IER) Response CRC Error Interrupt Enable Position */
#define HSMCI_IER_RCRCE_Msk                 (0x1U << HSMCI_IER_RCRCE_Pos)                  /**< (HSMCI_IER) Response CRC Error Interrupt Enable Mask */
#define HSMCI_IER_RCRCE                     HSMCI_IER_RCRCE_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use HSMCI_IER_RCRCE_Msk instead */
#define HSMCI_IER_RENDE_Pos                 19                                             /**< (HSMCI_IER) Response End Bit Error Interrupt Enable Position */
#define HSMCI_IER_RENDE_Msk                 (0x1U << HSMCI_IER_RENDE_Pos)                  /**< (HSMCI_IER) Response End Bit Error Interrupt Enable Mask */
#define HSMCI_IER_RENDE                     HSMCI_IER_RENDE_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use HSMCI_IER_RENDE_Msk instead */
#define HSMCI_IER_RTOE_Pos                  20                                             /**< (HSMCI_IER) Response Time-out Error Interrupt Enable Position */
#define HSMCI_IER_RTOE_Msk                  (0x1U << HSMCI_IER_RTOE_Pos)                   /**< (HSMCI_IER) Response Time-out Error Interrupt Enable Mask */
#define HSMCI_IER_RTOE                      HSMCI_IER_RTOE_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use HSMCI_IER_RTOE_Msk instead */
#define HSMCI_IER_DCRCE_Pos                 21                                             /**< (HSMCI_IER) Data CRC Error Interrupt Enable Position */
#define HSMCI_IER_DCRCE_Msk                 (0x1U << HSMCI_IER_DCRCE_Pos)                  /**< (HSMCI_IER) Data CRC Error Interrupt Enable Mask */
#define HSMCI_IER_DCRCE                     HSMCI_IER_DCRCE_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use HSMCI_IER_DCRCE_Msk instead */
#define HSMCI_IER_DTOE_Pos                  22                                             /**< (HSMCI_IER) Data Time-out Error Interrupt Enable Position */
#define HSMCI_IER_DTOE_Msk                  (0x1U << HSMCI_IER_DTOE_Pos)                   /**< (HSMCI_IER) Data Time-out Error Interrupt Enable Mask */
#define HSMCI_IER_DTOE                      HSMCI_IER_DTOE_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use HSMCI_IER_DTOE_Msk instead */
#define HSMCI_IER_CSTOE_Pos                 23                                             /**< (HSMCI_IER) Completion Signal Timeout Error Interrupt Enable Position */
#define HSMCI_IER_CSTOE_Msk                 (0x1U << HSMCI_IER_CSTOE_Pos)                  /**< (HSMCI_IER) Completion Signal Timeout Error Interrupt Enable Mask */
#define HSMCI_IER_CSTOE                     HSMCI_IER_CSTOE_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use HSMCI_IER_CSTOE_Msk instead */
#define HSMCI_IER_BLKOVRE_Pos               24                                             /**< (HSMCI_IER) DMA Block Overrun Error Interrupt Enable Position */
#define HSMCI_IER_BLKOVRE_Msk               (0x1U << HSMCI_IER_BLKOVRE_Pos)                /**< (HSMCI_IER) DMA Block Overrun Error Interrupt Enable Mask */
#define HSMCI_IER_BLKOVRE                   HSMCI_IER_BLKOVRE_Msk                          /**< \deprecated Old style mask definition for 1 bit bitfield. Use HSMCI_IER_BLKOVRE_Msk instead */
#define HSMCI_IER_FIFOEMPTY_Pos             26                                             /**< (HSMCI_IER) FIFO empty Interrupt enable Position */
#define HSMCI_IER_FIFOEMPTY_Msk             (0x1U << HSMCI_IER_FIFOEMPTY_Pos)              /**< (HSMCI_IER) FIFO empty Interrupt enable Mask */
#define HSMCI_IER_FIFOEMPTY                 HSMCI_IER_FIFOEMPTY_Msk                        /**< \deprecated Old style mask definition for 1 bit bitfield. Use HSMCI_IER_FIFOEMPTY_Msk instead */
#define HSMCI_IER_XFRDONE_Pos               27                                             /**< (HSMCI_IER) Transfer Done Interrupt enable Position */
#define HSMCI_IER_XFRDONE_Msk               (0x1U << HSMCI_IER_XFRDONE_Pos)                /**< (HSMCI_IER) Transfer Done Interrupt enable Mask */
#define HSMCI_IER_XFRDONE                   HSMCI_IER_XFRDONE_Msk                          /**< \deprecated Old style mask definition for 1 bit bitfield. Use HSMCI_IER_XFRDONE_Msk instead */
#define HSMCI_IER_ACKRCV_Pos                28                                             /**< (HSMCI_IER) Boot Acknowledge Interrupt Enable Position */
#define HSMCI_IER_ACKRCV_Msk                (0x1U << HSMCI_IER_ACKRCV_Pos)                 /**< (HSMCI_IER) Boot Acknowledge Interrupt Enable Mask */
#define HSMCI_IER_ACKRCV                    HSMCI_IER_ACKRCV_Msk                           /**< \deprecated Old style mask definition for 1 bit bitfield. Use HSMCI_IER_ACKRCV_Msk instead */
#define HSMCI_IER_ACKRCVE_Pos               29                                             /**< (HSMCI_IER) Boot Acknowledge Error Interrupt Enable Position */
#define HSMCI_IER_ACKRCVE_Msk               (0x1U << HSMCI_IER_ACKRCVE_Pos)                /**< (HSMCI_IER) Boot Acknowledge Error Interrupt Enable Mask */
#define HSMCI_IER_ACKRCVE                   HSMCI_IER_ACKRCVE_Msk                          /**< \deprecated Old style mask definition for 1 bit bitfield. Use HSMCI_IER_ACKRCVE_Msk instead */
#define HSMCI_IER_OVRE_Pos                  30                                             /**< (HSMCI_IER) Overrun Interrupt Enable Position */
#define HSMCI_IER_OVRE_Msk                  (0x1U << HSMCI_IER_OVRE_Pos)                   /**< (HSMCI_IER) Overrun Interrupt Enable Mask */
#define HSMCI_IER_OVRE                      HSMCI_IER_OVRE_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use HSMCI_IER_OVRE_Msk instead */
#define HSMCI_IER_UNRE_Pos                  31                                             /**< (HSMCI_IER) Underrun Interrupt Enable Position */
#define HSMCI_IER_UNRE_Msk                  (0x1U << HSMCI_IER_UNRE_Pos)                   /**< (HSMCI_IER) Underrun Interrupt Enable Mask */
#define HSMCI_IER_UNRE                      HSMCI_IER_UNRE_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use HSMCI_IER_UNRE_Msk instead */
#define HSMCI_IER_MASK                      (0xFDFF313FU)                                  /**< \deprecated (HSMCI_IER) Register MASK  (Use HSMCI_IER_Msk instead)  */
#define HSMCI_IER_Msk                       (0xFDFF313FU)                                  /**< (HSMCI_IER) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- HSMCI_IDR : (HSMCI Offset: 0x48) (/W 32) Interrupt Disable Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t CMDRDY:1;                  /**< bit:      0  Command Ready Interrupt Disable          */
    uint32_t RXRDY:1;                   /**< bit:      1  Receiver Ready Interrupt Disable         */
    uint32_t TXRDY:1;                   /**< bit:      2  Transmit Ready Interrupt Disable         */
    uint32_t BLKE:1;                    /**< bit:      3  Data Block Ended Interrupt Disable       */
    uint32_t DTIP:1;                    /**< bit:      4  Data Transfer in Progress Interrupt Disable */
    uint32_t NOTBUSY:1;                 /**< bit:      5  Data Not Busy Interrupt Disable          */
    uint32_t :2;                        /**< bit:   6..7  Reserved */
    uint32_t SDIOIRQA:1;                /**< bit:      8  SDIO Interrupt for Slot A Interrupt Disable */
    uint32_t :3;                        /**< bit:  9..11  Reserved */
    uint32_t SDIOWAIT:1;                /**< bit:     12  SDIO Read Wait Operation Status Interrupt Disable */
    uint32_t CSRCV:1;                   /**< bit:     13  Completion Signal received interrupt Disable */
    uint32_t :2;                        /**< bit: 14..15  Reserved */
    uint32_t RINDE:1;                   /**< bit:     16  Response Index Error Interrupt Disable   */
    uint32_t RDIRE:1;                   /**< bit:     17  Response Direction Error Interrupt Disable */
    uint32_t RCRCE:1;                   /**< bit:     18  Response CRC Error Interrupt Disable     */
    uint32_t RENDE:1;                   /**< bit:     19  Response End Bit Error Interrupt Disable */
    uint32_t RTOE:1;                    /**< bit:     20  Response Time-out Error Interrupt Disable */
    uint32_t DCRCE:1;                   /**< bit:     21  Data CRC Error Interrupt Disable         */
    uint32_t DTOE:1;                    /**< bit:     22  Data Time-out Error Interrupt Disable    */
    uint32_t CSTOE:1;                   /**< bit:     23  Completion Signal Time out Error Interrupt Disable */
    uint32_t BLKOVRE:1;                 /**< bit:     24  DMA Block Overrun Error Interrupt Disable */
    uint32_t :1;                        /**< bit:     25  Reserved */
    uint32_t FIFOEMPTY:1;               /**< bit:     26  FIFO empty Interrupt Disable             */
    uint32_t XFRDONE:1;                 /**< bit:     27  Transfer Done Interrupt Disable          */
    uint32_t ACKRCV:1;                  /**< bit:     28  Boot Acknowledge Interrupt Disable       */
    uint32_t ACKRCVE:1;                 /**< bit:     29  Boot Acknowledge Error Interrupt Disable */
    uint32_t OVRE:1;                    /**< bit:     30  Overrun Interrupt Disable                */
    uint32_t UNRE:1;                    /**< bit:     31  Underrun Interrupt Disable               */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} HSMCI_IDR_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define HSMCI_IDR_OFFSET                    (0x48)                                        /**<  (HSMCI_IDR) Interrupt Disable Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define HSMCI_IDR_CMDRDY_Pos                0                                              /**< (HSMCI_IDR) Command Ready Interrupt Disable Position */
#define HSMCI_IDR_CMDRDY_Msk                (0x1U << HSMCI_IDR_CMDRDY_Pos)                 /**< (HSMCI_IDR) Command Ready Interrupt Disable Mask */
#define HSMCI_IDR_CMDRDY                    HSMCI_IDR_CMDRDY_Msk                           /**< \deprecated Old style mask definition for 1 bit bitfield. Use HSMCI_IDR_CMDRDY_Msk instead */
#define HSMCI_IDR_RXRDY_Pos                 1                                              /**< (HSMCI_IDR) Receiver Ready Interrupt Disable Position */
#define HSMCI_IDR_RXRDY_Msk                 (0x1U << HSMCI_IDR_RXRDY_Pos)                  /**< (HSMCI_IDR) Receiver Ready Interrupt Disable Mask */
#define HSMCI_IDR_RXRDY                     HSMCI_IDR_RXRDY_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use HSMCI_IDR_RXRDY_Msk instead */
#define HSMCI_IDR_TXRDY_Pos                 2                                              /**< (HSMCI_IDR) Transmit Ready Interrupt Disable Position */
#define HSMCI_IDR_TXRDY_Msk                 (0x1U << HSMCI_IDR_TXRDY_Pos)                  /**< (HSMCI_IDR) Transmit Ready Interrupt Disable Mask */
#define HSMCI_IDR_TXRDY                     HSMCI_IDR_TXRDY_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use HSMCI_IDR_TXRDY_Msk instead */
#define HSMCI_IDR_BLKE_Pos                  3                                              /**< (HSMCI_IDR) Data Block Ended Interrupt Disable Position */
#define HSMCI_IDR_BLKE_Msk                  (0x1U << HSMCI_IDR_BLKE_Pos)                   /**< (HSMCI_IDR) Data Block Ended Interrupt Disable Mask */
#define HSMCI_IDR_BLKE                      HSMCI_IDR_BLKE_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use HSMCI_IDR_BLKE_Msk instead */
#define HSMCI_IDR_DTIP_Pos                  4                                              /**< (HSMCI_IDR) Data Transfer in Progress Interrupt Disable Position */
#define HSMCI_IDR_DTIP_Msk                  (0x1U << HSMCI_IDR_DTIP_Pos)                   /**< (HSMCI_IDR) Data Transfer in Progress Interrupt Disable Mask */
#define HSMCI_IDR_DTIP                      HSMCI_IDR_DTIP_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use HSMCI_IDR_DTIP_Msk instead */
#define HSMCI_IDR_NOTBUSY_Pos               5                                              /**< (HSMCI_IDR) Data Not Busy Interrupt Disable Position */
#define HSMCI_IDR_NOTBUSY_Msk               (0x1U << HSMCI_IDR_NOTBUSY_Pos)                /**< (HSMCI_IDR) Data Not Busy Interrupt Disable Mask */
#define HSMCI_IDR_NOTBUSY                   HSMCI_IDR_NOTBUSY_Msk                          /**< \deprecated Old style mask definition for 1 bit bitfield. Use HSMCI_IDR_NOTBUSY_Msk instead */
#define HSMCI_IDR_SDIOIRQA_Pos              8                                              /**< (HSMCI_IDR) SDIO Interrupt for Slot A Interrupt Disable Position */
#define HSMCI_IDR_SDIOIRQA_Msk              (0x1U << HSMCI_IDR_SDIOIRQA_Pos)               /**< (HSMCI_IDR) SDIO Interrupt for Slot A Interrupt Disable Mask */
#define HSMCI_IDR_SDIOIRQA                  HSMCI_IDR_SDIOIRQA_Msk                         /**< \deprecated Old style mask definition for 1 bit bitfield. Use HSMCI_IDR_SDIOIRQA_Msk instead */
#define HSMCI_IDR_SDIOWAIT_Pos              12                                             /**< (HSMCI_IDR) SDIO Read Wait Operation Status Interrupt Disable Position */
#define HSMCI_IDR_SDIOWAIT_Msk              (0x1U << HSMCI_IDR_SDIOWAIT_Pos)               /**< (HSMCI_IDR) SDIO Read Wait Operation Status Interrupt Disable Mask */
#define HSMCI_IDR_SDIOWAIT                  HSMCI_IDR_SDIOWAIT_Msk                         /**< \deprecated Old style mask definition for 1 bit bitfield. Use HSMCI_IDR_SDIOWAIT_Msk instead */
#define HSMCI_IDR_CSRCV_Pos                 13                                             /**< (HSMCI_IDR) Completion Signal received interrupt Disable Position */
#define HSMCI_IDR_CSRCV_Msk                 (0x1U << HSMCI_IDR_CSRCV_Pos)                  /**< (HSMCI_IDR) Completion Signal received interrupt Disable Mask */
#define HSMCI_IDR_CSRCV                     HSMCI_IDR_CSRCV_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use HSMCI_IDR_CSRCV_Msk instead */
#define HSMCI_IDR_RINDE_Pos                 16                                             /**< (HSMCI_IDR) Response Index Error Interrupt Disable Position */
#define HSMCI_IDR_RINDE_Msk                 (0x1U << HSMCI_IDR_RINDE_Pos)                  /**< (HSMCI_IDR) Response Index Error Interrupt Disable Mask */
#define HSMCI_IDR_RINDE                     HSMCI_IDR_RINDE_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use HSMCI_IDR_RINDE_Msk instead */
#define HSMCI_IDR_RDIRE_Pos                 17                                             /**< (HSMCI_IDR) Response Direction Error Interrupt Disable Position */
#define HSMCI_IDR_RDIRE_Msk                 (0x1U << HSMCI_IDR_RDIRE_Pos)                  /**< (HSMCI_IDR) Response Direction Error Interrupt Disable Mask */
#define HSMCI_IDR_RDIRE                     HSMCI_IDR_RDIRE_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use HSMCI_IDR_RDIRE_Msk instead */
#define HSMCI_IDR_RCRCE_Pos                 18                                             /**< (HSMCI_IDR) Response CRC Error Interrupt Disable Position */
#define HSMCI_IDR_RCRCE_Msk                 (0x1U << HSMCI_IDR_RCRCE_Pos)                  /**< (HSMCI_IDR) Response CRC Error Interrupt Disable Mask */
#define HSMCI_IDR_RCRCE                     HSMCI_IDR_RCRCE_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use HSMCI_IDR_RCRCE_Msk instead */
#define HSMCI_IDR_RENDE_Pos                 19                                             /**< (HSMCI_IDR) Response End Bit Error Interrupt Disable Position */
#define HSMCI_IDR_RENDE_Msk                 (0x1U << HSMCI_IDR_RENDE_Pos)                  /**< (HSMCI_IDR) Response End Bit Error Interrupt Disable Mask */
#define HSMCI_IDR_RENDE                     HSMCI_IDR_RENDE_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use HSMCI_IDR_RENDE_Msk instead */
#define HSMCI_IDR_RTOE_Pos                  20                                             /**< (HSMCI_IDR) Response Time-out Error Interrupt Disable Position */
#define HSMCI_IDR_RTOE_Msk                  (0x1U << HSMCI_IDR_RTOE_Pos)                   /**< (HSMCI_IDR) Response Time-out Error Interrupt Disable Mask */
#define HSMCI_IDR_RTOE                      HSMCI_IDR_RTOE_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use HSMCI_IDR_RTOE_Msk instead */
#define HSMCI_IDR_DCRCE_Pos                 21                                             /**< (HSMCI_IDR) Data CRC Error Interrupt Disable Position */
#define HSMCI_IDR_DCRCE_Msk                 (0x1U << HSMCI_IDR_DCRCE_Pos)                  /**< (HSMCI_IDR) Data CRC Error Interrupt Disable Mask */
#define HSMCI_IDR_DCRCE                     HSMCI_IDR_DCRCE_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use HSMCI_IDR_DCRCE_Msk instead */
#define HSMCI_IDR_DTOE_Pos                  22                                             /**< (HSMCI_IDR) Data Time-out Error Interrupt Disable Position */
#define HSMCI_IDR_DTOE_Msk                  (0x1U << HSMCI_IDR_DTOE_Pos)                   /**< (HSMCI_IDR) Data Time-out Error Interrupt Disable Mask */
#define HSMCI_IDR_DTOE                      HSMCI_IDR_DTOE_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use HSMCI_IDR_DTOE_Msk instead */
#define HSMCI_IDR_CSTOE_Pos                 23                                             /**< (HSMCI_IDR) Completion Signal Time out Error Interrupt Disable Position */
#define HSMCI_IDR_CSTOE_Msk                 (0x1U << HSMCI_IDR_CSTOE_Pos)                  /**< (HSMCI_IDR) Completion Signal Time out Error Interrupt Disable Mask */
#define HSMCI_IDR_CSTOE                     HSMCI_IDR_CSTOE_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use HSMCI_IDR_CSTOE_Msk instead */
#define HSMCI_IDR_BLKOVRE_Pos               24                                             /**< (HSMCI_IDR) DMA Block Overrun Error Interrupt Disable Position */
#define HSMCI_IDR_BLKOVRE_Msk               (0x1U << HSMCI_IDR_BLKOVRE_Pos)                /**< (HSMCI_IDR) DMA Block Overrun Error Interrupt Disable Mask */
#define HSMCI_IDR_BLKOVRE                   HSMCI_IDR_BLKOVRE_Msk                          /**< \deprecated Old style mask definition for 1 bit bitfield. Use HSMCI_IDR_BLKOVRE_Msk instead */
#define HSMCI_IDR_FIFOEMPTY_Pos             26                                             /**< (HSMCI_IDR) FIFO empty Interrupt Disable Position */
#define HSMCI_IDR_FIFOEMPTY_Msk             (0x1U << HSMCI_IDR_FIFOEMPTY_Pos)              /**< (HSMCI_IDR) FIFO empty Interrupt Disable Mask */
#define HSMCI_IDR_FIFOEMPTY                 HSMCI_IDR_FIFOEMPTY_Msk                        /**< \deprecated Old style mask definition for 1 bit bitfield. Use HSMCI_IDR_FIFOEMPTY_Msk instead */
#define HSMCI_IDR_XFRDONE_Pos               27                                             /**< (HSMCI_IDR) Transfer Done Interrupt Disable Position */
#define HSMCI_IDR_XFRDONE_Msk               (0x1U << HSMCI_IDR_XFRDONE_Pos)                /**< (HSMCI_IDR) Transfer Done Interrupt Disable Mask */
#define HSMCI_IDR_XFRDONE                   HSMCI_IDR_XFRDONE_Msk                          /**< \deprecated Old style mask definition for 1 bit bitfield. Use HSMCI_IDR_XFRDONE_Msk instead */
#define HSMCI_IDR_ACKRCV_Pos                28                                             /**< (HSMCI_IDR) Boot Acknowledge Interrupt Disable Position */
#define HSMCI_IDR_ACKRCV_Msk                (0x1U << HSMCI_IDR_ACKRCV_Pos)                 /**< (HSMCI_IDR) Boot Acknowledge Interrupt Disable Mask */
#define HSMCI_IDR_ACKRCV                    HSMCI_IDR_ACKRCV_Msk                           /**< \deprecated Old style mask definition for 1 bit bitfield. Use HSMCI_IDR_ACKRCV_Msk instead */
#define HSMCI_IDR_ACKRCVE_Pos               29                                             /**< (HSMCI_IDR) Boot Acknowledge Error Interrupt Disable Position */
#define HSMCI_IDR_ACKRCVE_Msk               (0x1U << HSMCI_IDR_ACKRCVE_Pos)                /**< (HSMCI_IDR) Boot Acknowledge Error Interrupt Disable Mask */
#define HSMCI_IDR_ACKRCVE                   HSMCI_IDR_ACKRCVE_Msk                          /**< \deprecated Old style mask definition for 1 bit bitfield. Use HSMCI_IDR_ACKRCVE_Msk instead */
#define HSMCI_IDR_OVRE_Pos                  30                                             /**< (HSMCI_IDR) Overrun Interrupt Disable Position */
#define HSMCI_IDR_OVRE_Msk                  (0x1U << HSMCI_IDR_OVRE_Pos)                   /**< (HSMCI_IDR) Overrun Interrupt Disable Mask */
#define HSMCI_IDR_OVRE                      HSMCI_IDR_OVRE_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use HSMCI_IDR_OVRE_Msk instead */
#define HSMCI_IDR_UNRE_Pos                  31                                             /**< (HSMCI_IDR) Underrun Interrupt Disable Position */
#define HSMCI_IDR_UNRE_Msk                  (0x1U << HSMCI_IDR_UNRE_Pos)                   /**< (HSMCI_IDR) Underrun Interrupt Disable Mask */
#define HSMCI_IDR_UNRE                      HSMCI_IDR_UNRE_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use HSMCI_IDR_UNRE_Msk instead */
#define HSMCI_IDR_MASK                      (0xFDFF313FU)                                  /**< \deprecated (HSMCI_IDR) Register MASK  (Use HSMCI_IDR_Msk instead)  */
#define HSMCI_IDR_Msk                       (0xFDFF313FU)                                  /**< (HSMCI_IDR) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- HSMCI_IMR : (HSMCI Offset: 0x4c) (R/ 32) Interrupt Mask Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t CMDRDY:1;                  /**< bit:      0  Command Ready Interrupt Mask             */
    uint32_t RXRDY:1;                   /**< bit:      1  Receiver Ready Interrupt Mask            */
    uint32_t TXRDY:1;                   /**< bit:      2  Transmit Ready Interrupt Mask            */
    uint32_t BLKE:1;                    /**< bit:      3  Data Block Ended Interrupt Mask          */
    uint32_t DTIP:1;                    /**< bit:      4  Data Transfer in Progress Interrupt Mask */
    uint32_t NOTBUSY:1;                 /**< bit:      5  Data Not Busy Interrupt Mask             */
    uint32_t :2;                        /**< bit:   6..7  Reserved */
    uint32_t SDIOIRQA:1;                /**< bit:      8  SDIO Interrupt for Slot A Interrupt Mask */
    uint32_t :3;                        /**< bit:  9..11  Reserved */
    uint32_t SDIOWAIT:1;                /**< bit:     12  SDIO Read Wait Operation Status Interrupt Mask */
    uint32_t CSRCV:1;                   /**< bit:     13  Completion Signal Received Interrupt Mask */
    uint32_t :2;                        /**< bit: 14..15  Reserved */
    uint32_t RINDE:1;                   /**< bit:     16  Response Index Error Interrupt Mask      */
    uint32_t RDIRE:1;                   /**< bit:     17  Response Direction Error Interrupt Mask  */
    uint32_t RCRCE:1;                   /**< bit:     18  Response CRC Error Interrupt Mask        */
    uint32_t RENDE:1;                   /**< bit:     19  Response End Bit Error Interrupt Mask    */
    uint32_t RTOE:1;                    /**< bit:     20  Response Time-out Error Interrupt Mask   */
    uint32_t DCRCE:1;                   /**< bit:     21  Data CRC Error Interrupt Mask            */
    uint32_t DTOE:1;                    /**< bit:     22  Data Time-out Error Interrupt Mask       */
    uint32_t CSTOE:1;                   /**< bit:     23  Completion Signal Time-out Error Interrupt Mask */
    uint32_t BLKOVRE:1;                 /**< bit:     24  DMA Block Overrun Error Interrupt Mask   */
    uint32_t :1;                        /**< bit:     25  Reserved */
    uint32_t FIFOEMPTY:1;               /**< bit:     26  FIFO Empty Interrupt Mask                */
    uint32_t XFRDONE:1;                 /**< bit:     27  Transfer Done Interrupt Mask             */
    uint32_t ACKRCV:1;                  /**< bit:     28  Boot Operation Acknowledge Received Interrupt Mask */
    uint32_t ACKRCVE:1;                 /**< bit:     29  Boot Operation Acknowledge Error Interrupt Mask */
    uint32_t OVRE:1;                    /**< bit:     30  Overrun Interrupt Mask                   */
    uint32_t UNRE:1;                    /**< bit:     31  Underrun Interrupt Mask                  */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} HSMCI_IMR_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define HSMCI_IMR_OFFSET                    (0x4C)                                        /**<  (HSMCI_IMR) Interrupt Mask Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define HSMCI_IMR_CMDRDY_Pos                0                                              /**< (HSMCI_IMR) Command Ready Interrupt Mask Position */
#define HSMCI_IMR_CMDRDY_Msk                (0x1U << HSMCI_IMR_CMDRDY_Pos)                 /**< (HSMCI_IMR) Command Ready Interrupt Mask Mask */
#define HSMCI_IMR_CMDRDY                    HSMCI_IMR_CMDRDY_Msk                           /**< \deprecated Old style mask definition for 1 bit bitfield. Use HSMCI_IMR_CMDRDY_Msk instead */
#define HSMCI_IMR_RXRDY_Pos                 1                                              /**< (HSMCI_IMR) Receiver Ready Interrupt Mask Position */
#define HSMCI_IMR_RXRDY_Msk                 (0x1U << HSMCI_IMR_RXRDY_Pos)                  /**< (HSMCI_IMR) Receiver Ready Interrupt Mask Mask */
#define HSMCI_IMR_RXRDY                     HSMCI_IMR_RXRDY_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use HSMCI_IMR_RXRDY_Msk instead */
#define HSMCI_IMR_TXRDY_Pos                 2                                              /**< (HSMCI_IMR) Transmit Ready Interrupt Mask Position */
#define HSMCI_IMR_TXRDY_Msk                 (0x1U << HSMCI_IMR_TXRDY_Pos)                  /**< (HSMCI_IMR) Transmit Ready Interrupt Mask Mask */
#define HSMCI_IMR_TXRDY                     HSMCI_IMR_TXRDY_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use HSMCI_IMR_TXRDY_Msk instead */
#define HSMCI_IMR_BLKE_Pos                  3                                              /**< (HSMCI_IMR) Data Block Ended Interrupt Mask Position */
#define HSMCI_IMR_BLKE_Msk                  (0x1U << HSMCI_IMR_BLKE_Pos)                   /**< (HSMCI_IMR) Data Block Ended Interrupt Mask Mask */
#define HSMCI_IMR_BLKE                      HSMCI_IMR_BLKE_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use HSMCI_IMR_BLKE_Msk instead */
#define HSMCI_IMR_DTIP_Pos                  4                                              /**< (HSMCI_IMR) Data Transfer in Progress Interrupt Mask Position */
#define HSMCI_IMR_DTIP_Msk                  (0x1U << HSMCI_IMR_DTIP_Pos)                   /**< (HSMCI_IMR) Data Transfer in Progress Interrupt Mask Mask */
#define HSMCI_IMR_DTIP                      HSMCI_IMR_DTIP_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use HSMCI_IMR_DTIP_Msk instead */
#define HSMCI_IMR_NOTBUSY_Pos               5                                              /**< (HSMCI_IMR) Data Not Busy Interrupt Mask Position */
#define HSMCI_IMR_NOTBUSY_Msk               (0x1U << HSMCI_IMR_NOTBUSY_Pos)                /**< (HSMCI_IMR) Data Not Busy Interrupt Mask Mask */
#define HSMCI_IMR_NOTBUSY                   HSMCI_IMR_NOTBUSY_Msk                          /**< \deprecated Old style mask definition for 1 bit bitfield. Use HSMCI_IMR_NOTBUSY_Msk instead */
#define HSMCI_IMR_SDIOIRQA_Pos              8                                              /**< (HSMCI_IMR) SDIO Interrupt for Slot A Interrupt Mask Position */
#define HSMCI_IMR_SDIOIRQA_Msk              (0x1U << HSMCI_IMR_SDIOIRQA_Pos)               /**< (HSMCI_IMR) SDIO Interrupt for Slot A Interrupt Mask Mask */
#define HSMCI_IMR_SDIOIRQA                  HSMCI_IMR_SDIOIRQA_Msk                         /**< \deprecated Old style mask definition for 1 bit bitfield. Use HSMCI_IMR_SDIOIRQA_Msk instead */
#define HSMCI_IMR_SDIOWAIT_Pos              12                                             /**< (HSMCI_IMR) SDIO Read Wait Operation Status Interrupt Mask Position */
#define HSMCI_IMR_SDIOWAIT_Msk              (0x1U << HSMCI_IMR_SDIOWAIT_Pos)               /**< (HSMCI_IMR) SDIO Read Wait Operation Status Interrupt Mask Mask */
#define HSMCI_IMR_SDIOWAIT                  HSMCI_IMR_SDIOWAIT_Msk                         /**< \deprecated Old style mask definition for 1 bit bitfield. Use HSMCI_IMR_SDIOWAIT_Msk instead */
#define HSMCI_IMR_CSRCV_Pos                 13                                             /**< (HSMCI_IMR) Completion Signal Received Interrupt Mask Position */
#define HSMCI_IMR_CSRCV_Msk                 (0x1U << HSMCI_IMR_CSRCV_Pos)                  /**< (HSMCI_IMR) Completion Signal Received Interrupt Mask Mask */
#define HSMCI_IMR_CSRCV                     HSMCI_IMR_CSRCV_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use HSMCI_IMR_CSRCV_Msk instead */
#define HSMCI_IMR_RINDE_Pos                 16                                             /**< (HSMCI_IMR) Response Index Error Interrupt Mask Position */
#define HSMCI_IMR_RINDE_Msk                 (0x1U << HSMCI_IMR_RINDE_Pos)                  /**< (HSMCI_IMR) Response Index Error Interrupt Mask Mask */
#define HSMCI_IMR_RINDE                     HSMCI_IMR_RINDE_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use HSMCI_IMR_RINDE_Msk instead */
#define HSMCI_IMR_RDIRE_Pos                 17                                             /**< (HSMCI_IMR) Response Direction Error Interrupt Mask Position */
#define HSMCI_IMR_RDIRE_Msk                 (0x1U << HSMCI_IMR_RDIRE_Pos)                  /**< (HSMCI_IMR) Response Direction Error Interrupt Mask Mask */
#define HSMCI_IMR_RDIRE                     HSMCI_IMR_RDIRE_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use HSMCI_IMR_RDIRE_Msk instead */
#define HSMCI_IMR_RCRCE_Pos                 18                                             /**< (HSMCI_IMR) Response CRC Error Interrupt Mask Position */
#define HSMCI_IMR_RCRCE_Msk                 (0x1U << HSMCI_IMR_RCRCE_Pos)                  /**< (HSMCI_IMR) Response CRC Error Interrupt Mask Mask */
#define HSMCI_IMR_RCRCE                     HSMCI_IMR_RCRCE_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use HSMCI_IMR_RCRCE_Msk instead */
#define HSMCI_IMR_RENDE_Pos                 19                                             /**< (HSMCI_IMR) Response End Bit Error Interrupt Mask Position */
#define HSMCI_IMR_RENDE_Msk                 (0x1U << HSMCI_IMR_RENDE_Pos)                  /**< (HSMCI_IMR) Response End Bit Error Interrupt Mask Mask */
#define HSMCI_IMR_RENDE                     HSMCI_IMR_RENDE_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use HSMCI_IMR_RENDE_Msk instead */
#define HSMCI_IMR_RTOE_Pos                  20                                             /**< (HSMCI_IMR) Response Time-out Error Interrupt Mask Position */
#define HSMCI_IMR_RTOE_Msk                  (0x1U << HSMCI_IMR_RTOE_Pos)                   /**< (HSMCI_IMR) Response Time-out Error Interrupt Mask Mask */
#define HSMCI_IMR_RTOE                      HSMCI_IMR_RTOE_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use HSMCI_IMR_RTOE_Msk instead */
#define HSMCI_IMR_DCRCE_Pos                 21                                             /**< (HSMCI_IMR) Data CRC Error Interrupt Mask Position */
#define HSMCI_IMR_DCRCE_Msk                 (0x1U << HSMCI_IMR_DCRCE_Pos)                  /**< (HSMCI_IMR) Data CRC Error Interrupt Mask Mask */
#define HSMCI_IMR_DCRCE                     HSMCI_IMR_DCRCE_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use HSMCI_IMR_DCRCE_Msk instead */
#define HSMCI_IMR_DTOE_Pos                  22                                             /**< (HSMCI_IMR) Data Time-out Error Interrupt Mask Position */
#define HSMCI_IMR_DTOE_Msk                  (0x1U << HSMCI_IMR_DTOE_Pos)                   /**< (HSMCI_IMR) Data Time-out Error Interrupt Mask Mask */
#define HSMCI_IMR_DTOE                      HSMCI_IMR_DTOE_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use HSMCI_IMR_DTOE_Msk instead */
#define HSMCI_IMR_CSTOE_Pos                 23                                             /**< (HSMCI_IMR) Completion Signal Time-out Error Interrupt Mask Position */
#define HSMCI_IMR_CSTOE_Msk                 (0x1U << HSMCI_IMR_CSTOE_Pos)                  /**< (HSMCI_IMR) Completion Signal Time-out Error Interrupt Mask Mask */
#define HSMCI_IMR_CSTOE                     HSMCI_IMR_CSTOE_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use HSMCI_IMR_CSTOE_Msk instead */
#define HSMCI_IMR_BLKOVRE_Pos               24                                             /**< (HSMCI_IMR) DMA Block Overrun Error Interrupt Mask Position */
#define HSMCI_IMR_BLKOVRE_Msk               (0x1U << HSMCI_IMR_BLKOVRE_Pos)                /**< (HSMCI_IMR) DMA Block Overrun Error Interrupt Mask Mask */
#define HSMCI_IMR_BLKOVRE                   HSMCI_IMR_BLKOVRE_Msk                          /**< \deprecated Old style mask definition for 1 bit bitfield. Use HSMCI_IMR_BLKOVRE_Msk instead */
#define HSMCI_IMR_FIFOEMPTY_Pos             26                                             /**< (HSMCI_IMR) FIFO Empty Interrupt Mask Position */
#define HSMCI_IMR_FIFOEMPTY_Msk             (0x1U << HSMCI_IMR_FIFOEMPTY_Pos)              /**< (HSMCI_IMR) FIFO Empty Interrupt Mask Mask */
#define HSMCI_IMR_FIFOEMPTY                 HSMCI_IMR_FIFOEMPTY_Msk                        /**< \deprecated Old style mask definition for 1 bit bitfield. Use HSMCI_IMR_FIFOEMPTY_Msk instead */
#define HSMCI_IMR_XFRDONE_Pos               27                                             /**< (HSMCI_IMR) Transfer Done Interrupt Mask Position */
#define HSMCI_IMR_XFRDONE_Msk               (0x1U << HSMCI_IMR_XFRDONE_Pos)                /**< (HSMCI_IMR) Transfer Done Interrupt Mask Mask */
#define HSMCI_IMR_XFRDONE                   HSMCI_IMR_XFRDONE_Msk                          /**< \deprecated Old style mask definition for 1 bit bitfield. Use HSMCI_IMR_XFRDONE_Msk instead */
#define HSMCI_IMR_ACKRCV_Pos                28                                             /**< (HSMCI_IMR) Boot Operation Acknowledge Received Interrupt Mask Position */
#define HSMCI_IMR_ACKRCV_Msk                (0x1U << HSMCI_IMR_ACKRCV_Pos)                 /**< (HSMCI_IMR) Boot Operation Acknowledge Received Interrupt Mask Mask */
#define HSMCI_IMR_ACKRCV                    HSMCI_IMR_ACKRCV_Msk                           /**< \deprecated Old style mask definition for 1 bit bitfield. Use HSMCI_IMR_ACKRCV_Msk instead */
#define HSMCI_IMR_ACKRCVE_Pos               29                                             /**< (HSMCI_IMR) Boot Operation Acknowledge Error Interrupt Mask Position */
#define HSMCI_IMR_ACKRCVE_Msk               (0x1U << HSMCI_IMR_ACKRCVE_Pos)                /**< (HSMCI_IMR) Boot Operation Acknowledge Error Interrupt Mask Mask */
#define HSMCI_IMR_ACKRCVE                   HSMCI_IMR_ACKRCVE_Msk                          /**< \deprecated Old style mask definition for 1 bit bitfield. Use HSMCI_IMR_ACKRCVE_Msk instead */
#define HSMCI_IMR_OVRE_Pos                  30                                             /**< (HSMCI_IMR) Overrun Interrupt Mask Position */
#define HSMCI_IMR_OVRE_Msk                  (0x1U << HSMCI_IMR_OVRE_Pos)                   /**< (HSMCI_IMR) Overrun Interrupt Mask Mask */
#define HSMCI_IMR_OVRE                      HSMCI_IMR_OVRE_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use HSMCI_IMR_OVRE_Msk instead */
#define HSMCI_IMR_UNRE_Pos                  31                                             /**< (HSMCI_IMR) Underrun Interrupt Mask Position */
#define HSMCI_IMR_UNRE_Msk                  (0x1U << HSMCI_IMR_UNRE_Pos)                   /**< (HSMCI_IMR) Underrun Interrupt Mask Mask */
#define HSMCI_IMR_UNRE                      HSMCI_IMR_UNRE_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use HSMCI_IMR_UNRE_Msk instead */
#define HSMCI_IMR_MASK                      (0xFDFF313FU)                                  /**< \deprecated (HSMCI_IMR) Register MASK  (Use HSMCI_IMR_Msk instead)  */
#define HSMCI_IMR_Msk                       (0xFDFF313FU)                                  /**< (HSMCI_IMR) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- HSMCI_DMA : (HSMCI Offset: 0x50) (R/W 32) DMA Configuration Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t :4;                        /**< bit:   0..3  Reserved */
    uint32_t CHKSIZE:3;                 /**< bit:   4..6  DMA Channel Read and Write Chunk Size    */
    uint32_t :1;                        /**< bit:      7  Reserved */
    uint32_t DMAEN:1;                   /**< bit:      8  DMA Hardware Handshaking Enable          */
    uint32_t :23;                       /**< bit:  9..31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} HSMCI_DMA_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define HSMCI_DMA_OFFSET                    (0x50)                                        /**<  (HSMCI_DMA) DMA Configuration Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define HSMCI_DMA_CHKSIZE_Pos               4                                              /**< (HSMCI_DMA) DMA Channel Read and Write Chunk Size Position */
#define HSMCI_DMA_CHKSIZE_Msk               (0x7U << HSMCI_DMA_CHKSIZE_Pos)                /**< (HSMCI_DMA) DMA Channel Read and Write Chunk Size Mask */
#define HSMCI_DMA_CHKSIZE(value)            (HSMCI_DMA_CHKSIZE_Msk & ((value) << HSMCI_DMA_CHKSIZE_Pos))
#define   HSMCI_DMA_CHKSIZE_1_Val           (0x0U)                                         /**< (HSMCI_DMA) 1 data available  */
#define   HSMCI_DMA_CHKSIZE_2_Val           (0x1U)                                         /**< (HSMCI_DMA) 2 data available  */
#define   HSMCI_DMA_CHKSIZE_4_Val           (0x2U)                                         /**< (HSMCI_DMA) 4 data available  */
#define   HSMCI_DMA_CHKSIZE_8_Val           (0x3U)                                         /**< (HSMCI_DMA) 8 data available  */
#define   HSMCI_DMA_CHKSIZE_16_Val          (0x4U)                                         /**< (HSMCI_DMA) 16 data available  */
#define HSMCI_DMA_CHKSIZE_1                 (HSMCI_DMA_CHKSIZE_1_Val << HSMCI_DMA_CHKSIZE_Pos)  /**< (HSMCI_DMA) 1 data available Position  */
#define HSMCI_DMA_CHKSIZE_2                 (HSMCI_DMA_CHKSIZE_2_Val << HSMCI_DMA_CHKSIZE_Pos)  /**< (HSMCI_DMA) 2 data available Position  */
#define HSMCI_DMA_CHKSIZE_4                 (HSMCI_DMA_CHKSIZE_4_Val << HSMCI_DMA_CHKSIZE_Pos)  /**< (HSMCI_DMA) 4 data available Position  */
#define HSMCI_DMA_CHKSIZE_8                 (HSMCI_DMA_CHKSIZE_8_Val << HSMCI_DMA_CHKSIZE_Pos)  /**< (HSMCI_DMA) 8 data available Position  */
#define HSMCI_DMA_CHKSIZE_16                (HSMCI_DMA_CHKSIZE_16_Val << HSMCI_DMA_CHKSIZE_Pos)  /**< (HSMCI_DMA) 16 data available Position  */
#define HSMCI_DMA_DMAEN_Pos                 8                                              /**< (HSMCI_DMA) DMA Hardware Handshaking Enable Position */
#define HSMCI_DMA_DMAEN_Msk                 (0x1U << HSMCI_DMA_DMAEN_Pos)                  /**< (HSMCI_DMA) DMA Hardware Handshaking Enable Mask */
#define HSMCI_DMA_DMAEN                     HSMCI_DMA_DMAEN_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use HSMCI_DMA_DMAEN_Msk instead */
#define HSMCI_DMA_MASK                      (0x170U)                                       /**< \deprecated (HSMCI_DMA) Register MASK  (Use HSMCI_DMA_Msk instead)  */
#define HSMCI_DMA_Msk                       (0x170U)                                       /**< (HSMCI_DMA) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- HSMCI_CFG : (HSMCI Offset: 0x54) (R/W 32) Configuration Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t FIFOMODE:1;                /**< bit:      0  HSMCI Internal FIFO control mode         */
    uint32_t :3;                        /**< bit:   1..3  Reserved */
    uint32_t FERRCTRL:1;                /**< bit:      4  Flow Error flag reset control mode       */
    uint32_t :3;                        /**< bit:   5..7  Reserved */
    uint32_t HSMODE:1;                  /**< bit:      8  High Speed Mode                          */
    uint32_t :3;                        /**< bit:  9..11  Reserved */
    uint32_t LSYNC:1;                   /**< bit:     12  Synchronize on the last block            */
    uint32_t :19;                       /**< bit: 13..31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} HSMCI_CFG_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define HSMCI_CFG_OFFSET                    (0x54)                                        /**<  (HSMCI_CFG) Configuration Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define HSMCI_CFG_FIFOMODE_Pos              0                                              /**< (HSMCI_CFG) HSMCI Internal FIFO control mode Position */
#define HSMCI_CFG_FIFOMODE_Msk              (0x1U << HSMCI_CFG_FIFOMODE_Pos)               /**< (HSMCI_CFG) HSMCI Internal FIFO control mode Mask */
#define HSMCI_CFG_FIFOMODE                  HSMCI_CFG_FIFOMODE_Msk                         /**< \deprecated Old style mask definition for 1 bit bitfield. Use HSMCI_CFG_FIFOMODE_Msk instead */
#define HSMCI_CFG_FERRCTRL_Pos              4                                              /**< (HSMCI_CFG) Flow Error flag reset control mode Position */
#define HSMCI_CFG_FERRCTRL_Msk              (0x1U << HSMCI_CFG_FERRCTRL_Pos)               /**< (HSMCI_CFG) Flow Error flag reset control mode Mask */
#define HSMCI_CFG_FERRCTRL                  HSMCI_CFG_FERRCTRL_Msk                         /**< \deprecated Old style mask definition for 1 bit bitfield. Use HSMCI_CFG_FERRCTRL_Msk instead */
#define HSMCI_CFG_HSMODE_Pos                8                                              /**< (HSMCI_CFG) High Speed Mode Position */
#define HSMCI_CFG_HSMODE_Msk                (0x1U << HSMCI_CFG_HSMODE_Pos)                 /**< (HSMCI_CFG) High Speed Mode Mask */
#define HSMCI_CFG_HSMODE                    HSMCI_CFG_HSMODE_Msk                           /**< \deprecated Old style mask definition for 1 bit bitfield. Use HSMCI_CFG_HSMODE_Msk instead */
#define HSMCI_CFG_LSYNC_Pos                 12                                             /**< (HSMCI_CFG) Synchronize on the last block Position */
#define HSMCI_CFG_LSYNC_Msk                 (0x1U << HSMCI_CFG_LSYNC_Pos)                  /**< (HSMCI_CFG) Synchronize on the last block Mask */
#define HSMCI_CFG_LSYNC                     HSMCI_CFG_LSYNC_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use HSMCI_CFG_LSYNC_Msk instead */
#define HSMCI_CFG_MASK                      (0x1111U)                                      /**< \deprecated (HSMCI_CFG) Register MASK  (Use HSMCI_CFG_Msk instead)  */
#define HSMCI_CFG_Msk                       (0x1111U)                                      /**< (HSMCI_CFG) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- HSMCI_WPMR : (HSMCI Offset: 0xe4) (R/W 32) Write Protection Mode Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t WPEN:1;                    /**< bit:      0  Write Protect Enable                     */
    uint32_t :7;                        /**< bit:   1..7  Reserved */
    uint32_t WPKEY:24;                  /**< bit:  8..31  Write Protect Key                        */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} HSMCI_WPMR_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define HSMCI_WPMR_OFFSET                   (0xE4)                                        /**<  (HSMCI_WPMR) Write Protection Mode Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define HSMCI_WPMR_WPEN_Pos                 0                                              /**< (HSMCI_WPMR) Write Protect Enable Position */
#define HSMCI_WPMR_WPEN_Msk                 (0x1U << HSMCI_WPMR_WPEN_Pos)                  /**< (HSMCI_WPMR) Write Protect Enable Mask */
#define HSMCI_WPMR_WPEN                     HSMCI_WPMR_WPEN_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use HSMCI_WPMR_WPEN_Msk instead */
#define HSMCI_WPMR_WPKEY_Pos                8                                              /**< (HSMCI_WPMR) Write Protect Key Position */
#define HSMCI_WPMR_WPKEY_Msk                (0xFFFFFFU << HSMCI_WPMR_WPKEY_Pos)            /**< (HSMCI_WPMR) Write Protect Key Mask */
#define HSMCI_WPMR_WPKEY(value)             (HSMCI_WPMR_WPKEY_Msk & ((value) << HSMCI_WPMR_WPKEY_Pos))
#define   HSMCI_WPMR_WPKEY_PASSWD_Val       (0x4D4349U)                                    /**< (HSMCI_WPMR) Writing any other value in this field aborts the write operation of the WPEN bit.Always reads as 0.  */
#define HSMCI_WPMR_WPKEY_PASSWD             (HSMCI_WPMR_WPKEY_PASSWD_Val << HSMCI_WPMR_WPKEY_Pos)  /**< (HSMCI_WPMR) Writing any other value in this field aborts the write operation of the WPEN bit.Always reads as 0. Position  */
#define HSMCI_WPMR_MASK                     (0xFFFFFF01U)                                  /**< \deprecated (HSMCI_WPMR) Register MASK  (Use HSMCI_WPMR_Msk instead)  */
#define HSMCI_WPMR_Msk                      (0xFFFFFF01U)                                  /**< (HSMCI_WPMR) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- HSMCI_WPSR : (HSMCI Offset: 0xe8) (R/ 32) Write Protection Status Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t WPVS:1;                    /**< bit:      0  Write Protection Violation Status        */
    uint32_t :7;                        /**< bit:   1..7  Reserved */
    uint32_t WPVSRC:16;                 /**< bit:  8..23  Write Protection Violation Source        */
    uint32_t :8;                        /**< bit: 24..31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} HSMCI_WPSR_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define HSMCI_WPSR_OFFSET                   (0xE8)                                        /**<  (HSMCI_WPSR) Write Protection Status Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define HSMCI_WPSR_WPVS_Pos                 0                                              /**< (HSMCI_WPSR) Write Protection Violation Status Position */
#define HSMCI_WPSR_WPVS_Msk                 (0x1U << HSMCI_WPSR_WPVS_Pos)                  /**< (HSMCI_WPSR) Write Protection Violation Status Mask */
#define HSMCI_WPSR_WPVS                     HSMCI_WPSR_WPVS_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use HSMCI_WPSR_WPVS_Msk instead */
#define HSMCI_WPSR_WPVSRC_Pos               8                                              /**< (HSMCI_WPSR) Write Protection Violation Source Position */
#define HSMCI_WPSR_WPVSRC_Msk               (0xFFFFU << HSMCI_WPSR_WPVSRC_Pos)             /**< (HSMCI_WPSR) Write Protection Violation Source Mask */
#define HSMCI_WPSR_WPVSRC(value)            (HSMCI_WPSR_WPVSRC_Msk & ((value) << HSMCI_WPSR_WPVSRC_Pos))
#define HSMCI_WPSR_MASK                     (0xFFFF01U)                                    /**< \deprecated (HSMCI_WPSR) Register MASK  (Use HSMCI_WPSR_Msk instead)  */
#define HSMCI_WPSR_Msk                      (0xFFFF01U)                                    /**< (HSMCI_WPSR) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- HSMCI_FIFO : (HSMCI Offset: 0x200) (R/W 32) FIFO Memory Aperture0 0 -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t DATA:32;                   /**< bit:  0..31  Data to Read or Data to Write            */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} HSMCI_FIFO_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define HSMCI_FIFO_OFFSET                   (0x200)                                       /**<  (HSMCI_FIFO) FIFO Memory Aperture0 0  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define HSMCI_FIFO_DATA_Pos                 0                                              /**< (HSMCI_FIFO) Data to Read or Data to Write Position */
#define HSMCI_FIFO_DATA_Msk                 (0xFFFFFFFFU << HSMCI_FIFO_DATA_Pos)           /**< (HSMCI_FIFO) Data to Read or Data to Write Mask */
#define HSMCI_FIFO_DATA(value)              (HSMCI_FIFO_DATA_Msk & ((value) << HSMCI_FIFO_DATA_Pos))
#define HSMCI_FIFO_MASK                     (0xFFFFFFFFU)                                  /**< \deprecated (HSMCI_FIFO) Register MASK  (Use HSMCI_FIFO_Msk instead)  */
#define HSMCI_FIFO_Msk                      (0xFFFFFFFFU)                                  /**< (HSMCI_FIFO) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
#if COMPONENT_TYPEDEF_STYLE == 'R'
/** \brief HSMCI hardware registers */
typedef struct {  
  __O  uint32_t HSMCI_CR;       /**< (HSMCI Offset: 0x00) Control Register */
  __IO uint32_t HSMCI_MR;       /**< (HSMCI Offset: 0x04) Mode Register */
  __IO uint32_t HSMCI_DTOR;     /**< (HSMCI Offset: 0x08) Data Timeout Register */
  __IO uint32_t HSMCI_SDCR;     /**< (HSMCI Offset: 0x0C) SD/SDIO Card Register */
  __IO uint32_t HSMCI_ARGR;     /**< (HSMCI Offset: 0x10) Argument Register */
  __O  uint32_t HSMCI_CMDR;     /**< (HSMCI Offset: 0x14) Command Register */
  __IO uint32_t HSMCI_BLKR;     /**< (HSMCI Offset: 0x18) Block Register */
  __IO uint32_t HSMCI_CSTOR;    /**< (HSMCI Offset: 0x1C) Completion Signal Timeout Register */
  __I  uint32_t HSMCI_RSPR[4];  /**< (HSMCI Offset: 0x20) Response Register 0 */
  __I  uint32_t HSMCI_RDR;      /**< (HSMCI Offset: 0x30) Receive Data Register */
  __O  uint32_t HSMCI_TDR;      /**< (HSMCI Offset: 0x34) Transmit Data Register */
  __I  uint32_t Reserved1[2];
  __I  uint32_t HSMCI_SR;       /**< (HSMCI Offset: 0x40) Status Register */
  __O  uint32_t HSMCI_IER;      /**< (HSMCI Offset: 0x44) Interrupt Enable Register */
  __O  uint32_t HSMCI_IDR;      /**< (HSMCI Offset: 0x48) Interrupt Disable Register */
  __I  uint32_t HSMCI_IMR;      /**< (HSMCI Offset: 0x4C) Interrupt Mask Register */
  __IO uint32_t HSMCI_DMA;      /**< (HSMCI Offset: 0x50) DMA Configuration Register */
  __IO uint32_t HSMCI_CFG;      /**< (HSMCI Offset: 0x54) Configuration Register */
  __I  uint32_t Reserved2[35];
  __IO uint32_t HSMCI_WPMR;     /**< (HSMCI Offset: 0xE4) Write Protection Mode Register */
  __I  uint32_t HSMCI_WPSR;     /**< (HSMCI Offset: 0xE8) Write Protection Status Register */
  __I  uint32_t Reserved3[69];
  __IO uint32_t HSMCI_FIFO[256]; /**< (HSMCI Offset: 0x200) FIFO Memory Aperture0 0 */
} Hsmci;

#elif COMPONENT_TYPEDEF_STYLE == 'N'
/** \brief HSMCI hardware registers */
typedef struct {  
  __O  HSMCI_CR_Type                  HSMCI_CR;       /**< Offset: 0x00 ( /W  32) Control Register */
  __IO HSMCI_MR_Type                  HSMCI_MR;       /**< Offset: 0x04 (R/W  32) Mode Register */
  __IO HSMCI_DTOR_Type                HSMCI_DTOR;     /**< Offset: 0x08 (R/W  32) Data Timeout Register */
  __IO HSMCI_SDCR_Type                HSMCI_SDCR;     /**< Offset: 0x0C (R/W  32) SD/SDIO Card Register */
  __IO HSMCI_ARGR_Type                HSMCI_ARGR;     /**< Offset: 0x10 (R/W  32) Argument Register */
  __O  HSMCI_CMDR_Type                HSMCI_CMDR;     /**< Offset: 0x14 ( /W  32) Command Register */
  __IO HSMCI_BLKR_Type                HSMCI_BLKR;     /**< Offset: 0x18 (R/W  32) Block Register */
  __IO HSMCI_CSTOR_Type               HSMCI_CSTOR;    /**< Offset: 0x1C (R/W  32) Completion Signal Timeout Register */
  __I  HSMCI_RSPR_Type                HSMCI_RSPR[4];  /**< Offset: 0x20 (R/   32) Response Register 0 */
  __I  HSMCI_RDR_Type                 HSMCI_RDR;      /**< Offset: 0x30 (R/   32) Receive Data Register */
  __O  HSMCI_TDR_Type                 HSMCI_TDR;      /**< Offset: 0x34 ( /W  32) Transmit Data Register */
       RoReg8                         Reserved1[0x8];
  __I  HSMCI_SR_Type                  HSMCI_SR;       /**< Offset: 0x40 (R/   32) Status Register */
  __O  HSMCI_IER_Type                 HSMCI_IER;      /**< Offset: 0x44 ( /W  32) Interrupt Enable Register */
  __O  HSMCI_IDR_Type                 HSMCI_IDR;      /**< Offset: 0x48 ( /W  32) Interrupt Disable Register */
  __I  HSMCI_IMR_Type                 HSMCI_IMR;      /**< Offset: 0x4C (R/   32) Interrupt Mask Register */
  __IO HSMCI_DMA_Type                 HSMCI_DMA;      /**< Offset: 0x50 (R/W  32) DMA Configuration Register */
  __IO HSMCI_CFG_Type                 HSMCI_CFG;      /**< Offset: 0x54 (R/W  32) Configuration Register */
       RoReg8                         Reserved2[0x8C];
  __IO HSMCI_WPMR_Type                HSMCI_WPMR;     /**< Offset: 0xE4 (R/W  32) Write Protection Mode Register */
  __I  HSMCI_WPSR_Type                HSMCI_WPSR;     /**< Offset: 0xE8 (R/   32) Write Protection Status Register */
       RoReg8                         Reserved3[0x114];
  __IO HSMCI_FIFO_Type                HSMCI_FIFO[256]; /**< Offset: 0x200 (R/W  32) FIFO Memory Aperture0 0 */
} Hsmci;

#else /* COMPONENT_TYPEDEF_STYLE */
#error Unknown component typedef style
#endif /* COMPONENT_TYPEDEF_STYLE */

#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/** @}  end of High Speed MultiMedia Card Interface */

#endif /* _SAME70_HSMCI_COMPONENT_H_ */
