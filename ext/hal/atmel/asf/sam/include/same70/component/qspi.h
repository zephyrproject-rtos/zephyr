/**
 * \file
 *
 * \brief Component description for QSPI
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

#ifndef _SAME70_QSPI_COMPONENT_H_
#define _SAME70_QSPI_COMPONENT_H_
#define _SAME70_QSPI_COMPONENT_         /**< \deprecated  Backward compatibility for ASF */

/** \addtogroup SAME70_QSPI Quad Serial Peripheral Interface
 *  @{
 */
/* ========================================================================== */
/**  SOFTWARE API DEFINITION FOR QSPI */
/* ========================================================================== */
#ifndef COMPONENT_TYPEDEF_STYLE
  #define COMPONENT_TYPEDEF_STYLE 'R'  /**< Defines default style of typedefs for the component header files ('R' = RFO, 'N' = NTO)*/
#endif

#define QSPI_11171                      /**< (QSPI) Module ID */
#define REV_QSPI G                      /**< (QSPI) Module revision */

/* -------- QSPI_CR : (QSPI Offset: 0x00) (/W 32) Control Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t QSPIEN:1;                  /**< bit:      0  QSPI Enable                              */
    uint32_t QSPIDIS:1;                 /**< bit:      1  QSPI Disable                             */
    uint32_t :5;                        /**< bit:   2..6  Reserved */
    uint32_t SWRST:1;                   /**< bit:      7  QSPI Software Reset                      */
    uint32_t :16;                       /**< bit:  8..23  Reserved */
    uint32_t LASTXFER:1;                /**< bit:     24  Last Transfer                            */
    uint32_t :7;                        /**< bit: 25..31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} QSPI_CR_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define QSPI_CR_OFFSET                      (0x00)                                        /**<  (QSPI_CR) Control Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define QSPI_CR_QSPIEN_Pos                  0                                              /**< (QSPI_CR) QSPI Enable Position */
#define QSPI_CR_QSPIEN_Msk                  (0x1U << QSPI_CR_QSPIEN_Pos)                   /**< (QSPI_CR) QSPI Enable Mask */
#define QSPI_CR_QSPIEN                      QSPI_CR_QSPIEN_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use QSPI_CR_QSPIEN_Msk instead */
#define QSPI_CR_QSPIDIS_Pos                 1                                              /**< (QSPI_CR) QSPI Disable Position */
#define QSPI_CR_QSPIDIS_Msk                 (0x1U << QSPI_CR_QSPIDIS_Pos)                  /**< (QSPI_CR) QSPI Disable Mask */
#define QSPI_CR_QSPIDIS                     QSPI_CR_QSPIDIS_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use QSPI_CR_QSPIDIS_Msk instead */
#define QSPI_CR_SWRST_Pos                   7                                              /**< (QSPI_CR) QSPI Software Reset Position */
#define QSPI_CR_SWRST_Msk                   (0x1U << QSPI_CR_SWRST_Pos)                    /**< (QSPI_CR) QSPI Software Reset Mask */
#define QSPI_CR_SWRST                       QSPI_CR_SWRST_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use QSPI_CR_SWRST_Msk instead */
#define QSPI_CR_LASTXFER_Pos                24                                             /**< (QSPI_CR) Last Transfer Position */
#define QSPI_CR_LASTXFER_Msk                (0x1U << QSPI_CR_LASTXFER_Pos)                 /**< (QSPI_CR) Last Transfer Mask */
#define QSPI_CR_LASTXFER                    QSPI_CR_LASTXFER_Msk                           /**< \deprecated Old style mask definition for 1 bit bitfield. Use QSPI_CR_LASTXFER_Msk instead */
#define QSPI_CR_MASK                        (0x1000083U)                                   /**< \deprecated (QSPI_CR) Register MASK  (Use QSPI_CR_Msk instead)  */
#define QSPI_CR_Msk                         (0x1000083U)                                   /**< (QSPI_CR) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- QSPI_MR : (QSPI Offset: 0x04) (R/W 32) Mode Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t SMM:1;                     /**< bit:      0  Serial Memory Mode                       */
    uint32_t LLB:1;                     /**< bit:      1  Local Loopback Enable                    */
    uint32_t WDRBT:1;                   /**< bit:      2  Wait Data Read Before Transfer           */
    uint32_t :1;                        /**< bit:      3  Reserved */
    uint32_t CSMODE:2;                  /**< bit:   4..5  Chip Select Mode                         */
    uint32_t :2;                        /**< bit:   6..7  Reserved */
    uint32_t NBBITS:4;                  /**< bit:  8..11  Number Of Bits Per Transfer              */
    uint32_t :4;                        /**< bit: 12..15  Reserved */
    uint32_t DLYBCT:8;                  /**< bit: 16..23  Delay Between Consecutive Transfers      */
    uint32_t DLYCS:8;                   /**< bit: 24..31  Minimum Inactive QCS Delay               */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} QSPI_MR_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define QSPI_MR_OFFSET                      (0x04)                                        /**<  (QSPI_MR) Mode Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define QSPI_MR_SMM_Pos                     0                                              /**< (QSPI_MR) Serial Memory Mode Position */
#define QSPI_MR_SMM_Msk                     (0x1U << QSPI_MR_SMM_Pos)                      /**< (QSPI_MR) Serial Memory Mode Mask */
#define QSPI_MR_SMM                         QSPI_MR_SMM_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use QSPI_MR_SMM_Msk instead */
#define   QSPI_MR_SMM_SPI_Val               (0x0U)                                         /**< (QSPI_MR) The QSPI is in SPI mode.  */
#define   QSPI_MR_SMM_MEMORY_Val            (0x1U)                                         /**< (QSPI_MR) The QSPI is in Serial Memory mode.  */
#define QSPI_MR_SMM_SPI                     (QSPI_MR_SMM_SPI_Val << QSPI_MR_SMM_Pos)       /**< (QSPI_MR) The QSPI is in SPI mode. Position  */
#define QSPI_MR_SMM_MEMORY                  (QSPI_MR_SMM_MEMORY_Val << QSPI_MR_SMM_Pos)    /**< (QSPI_MR) The QSPI is in Serial Memory mode. Position  */
#define QSPI_MR_LLB_Pos                     1                                              /**< (QSPI_MR) Local Loopback Enable Position */
#define QSPI_MR_LLB_Msk                     (0x1U << QSPI_MR_LLB_Pos)                      /**< (QSPI_MR) Local Loopback Enable Mask */
#define QSPI_MR_LLB                         QSPI_MR_LLB_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use QSPI_MR_LLB_Msk instead */
#define   QSPI_MR_LLB_DISABLED_Val          (0x0U)                                         /**< (QSPI_MR) Local loopback path disabled.  */
#define   QSPI_MR_LLB_ENABLED_Val           (0x1U)                                         /**< (QSPI_MR) Local loopback path enabled.  */
#define QSPI_MR_LLB_DISABLED                (QSPI_MR_LLB_DISABLED_Val << QSPI_MR_LLB_Pos)  /**< (QSPI_MR) Local loopback path disabled. Position  */
#define QSPI_MR_LLB_ENABLED                 (QSPI_MR_LLB_ENABLED_Val << QSPI_MR_LLB_Pos)   /**< (QSPI_MR) Local loopback path enabled. Position  */
#define QSPI_MR_WDRBT_Pos                   2                                              /**< (QSPI_MR) Wait Data Read Before Transfer Position */
#define QSPI_MR_WDRBT_Msk                   (0x1U << QSPI_MR_WDRBT_Pos)                    /**< (QSPI_MR) Wait Data Read Before Transfer Mask */
#define QSPI_MR_WDRBT                       QSPI_MR_WDRBT_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use QSPI_MR_WDRBT_Msk instead */
#define   QSPI_MR_WDRBT_DISABLED_Val        (0x0U)                                         /**< (QSPI_MR) No effect. In SPI mode, a transfer can be initiated whatever the state of the QSPI_RDR is.  */
#define   QSPI_MR_WDRBT_ENABLED_Val         (0x1U)                                         /**< (QSPI_MR) In SPI mode, a transfer can start only if the QSPI_RDR is empty, i.e., does not contain any unread data. This mode prevents overrun error in reception.  */
#define QSPI_MR_WDRBT_DISABLED              (QSPI_MR_WDRBT_DISABLED_Val << QSPI_MR_WDRBT_Pos)  /**< (QSPI_MR) No effect. In SPI mode, a transfer can be initiated whatever the state of the QSPI_RDR is. Position  */
#define QSPI_MR_WDRBT_ENABLED               (QSPI_MR_WDRBT_ENABLED_Val << QSPI_MR_WDRBT_Pos)  /**< (QSPI_MR) In SPI mode, a transfer can start only if the QSPI_RDR is empty, i.e., does not contain any unread data. This mode prevents overrun error in reception. Position  */
#define QSPI_MR_CSMODE_Pos                  4                                              /**< (QSPI_MR) Chip Select Mode Position */
#define QSPI_MR_CSMODE_Msk                  (0x3U << QSPI_MR_CSMODE_Pos)                   /**< (QSPI_MR) Chip Select Mode Mask */
#define QSPI_MR_CSMODE(value)               (QSPI_MR_CSMODE_Msk & ((value) << QSPI_MR_CSMODE_Pos))
#define   QSPI_MR_CSMODE_NOT_RELOADED_Val   (0x0U)                                         /**< (QSPI_MR) The chip select is deasserted if QSPI_TDR.TD has not been reloaded before the end of the current transfer.  */
#define   QSPI_MR_CSMODE_LASTXFER_Val       (0x1U)                                         /**< (QSPI_MR) The chip select is deasserted when the bit LASTXFER is written at 1 and the character written in QSPI_TDR.TD has been transferred.  */
#define   QSPI_MR_CSMODE_SYSTEMATICALLY_Val (0x2U)                                         /**< (QSPI_MR) The chip select is deasserted systematically after each transfer.  */
#define QSPI_MR_CSMODE_NOT_RELOADED         (QSPI_MR_CSMODE_NOT_RELOADED_Val << QSPI_MR_CSMODE_Pos)  /**< (QSPI_MR) The chip select is deasserted if QSPI_TDR.TD has not been reloaded before the end of the current transfer. Position  */
#define QSPI_MR_CSMODE_LASTXFER             (QSPI_MR_CSMODE_LASTXFER_Val << QSPI_MR_CSMODE_Pos)  /**< (QSPI_MR) The chip select is deasserted when the bit LASTXFER is written at 1 and the character written in QSPI_TDR.TD has been transferred. Position  */
#define QSPI_MR_CSMODE_SYSTEMATICALLY       (QSPI_MR_CSMODE_SYSTEMATICALLY_Val << QSPI_MR_CSMODE_Pos)  /**< (QSPI_MR) The chip select is deasserted systematically after each transfer. Position  */
#define QSPI_MR_NBBITS_Pos                  8                                              /**< (QSPI_MR) Number Of Bits Per Transfer Position */
#define QSPI_MR_NBBITS_Msk                  (0xFU << QSPI_MR_NBBITS_Pos)                   /**< (QSPI_MR) Number Of Bits Per Transfer Mask */
#define QSPI_MR_NBBITS(value)               (QSPI_MR_NBBITS_Msk & ((value) << QSPI_MR_NBBITS_Pos))
#define   QSPI_MR_NBBITS_8_BIT_Val          (0x0U)                                         /**< (QSPI_MR) 8 bits for transfer  */
#define   QSPI_MR_NBBITS_16_BIT_Val         (0x8U)                                         /**< (QSPI_MR) 16 bits for transfer  */
#define QSPI_MR_NBBITS_8_BIT                (QSPI_MR_NBBITS_8_BIT_Val << QSPI_MR_NBBITS_Pos)  /**< (QSPI_MR) 8 bits for transfer Position  */
#define QSPI_MR_NBBITS_16_BIT               (QSPI_MR_NBBITS_16_BIT_Val << QSPI_MR_NBBITS_Pos)  /**< (QSPI_MR) 16 bits for transfer Position  */
#define QSPI_MR_DLYBCT_Pos                  16                                             /**< (QSPI_MR) Delay Between Consecutive Transfers Position */
#define QSPI_MR_DLYBCT_Msk                  (0xFFU << QSPI_MR_DLYBCT_Pos)                  /**< (QSPI_MR) Delay Between Consecutive Transfers Mask */
#define QSPI_MR_DLYBCT(value)               (QSPI_MR_DLYBCT_Msk & ((value) << QSPI_MR_DLYBCT_Pos))
#define QSPI_MR_DLYCS_Pos                   24                                             /**< (QSPI_MR) Minimum Inactive QCS Delay Position */
#define QSPI_MR_DLYCS_Msk                   (0xFFU << QSPI_MR_DLYCS_Pos)                   /**< (QSPI_MR) Minimum Inactive QCS Delay Mask */
#define QSPI_MR_DLYCS(value)                (QSPI_MR_DLYCS_Msk & ((value) << QSPI_MR_DLYCS_Pos))
#define QSPI_MR_MASK                        (0xFFFF0F37U)                                  /**< \deprecated (QSPI_MR) Register MASK  (Use QSPI_MR_Msk instead)  */
#define QSPI_MR_Msk                         (0xFFFF0F37U)                                  /**< (QSPI_MR) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- QSPI_RDR : (QSPI Offset: 0x08) (R/ 32) Receive Data Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t RD:16;                     /**< bit:  0..15  Receive Data                             */
    uint32_t :16;                       /**< bit: 16..31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} QSPI_RDR_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define QSPI_RDR_OFFSET                     (0x08)                                        /**<  (QSPI_RDR) Receive Data Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define QSPI_RDR_RD_Pos                     0                                              /**< (QSPI_RDR) Receive Data Position */
#define QSPI_RDR_RD_Msk                     (0xFFFFU << QSPI_RDR_RD_Pos)                   /**< (QSPI_RDR) Receive Data Mask */
#define QSPI_RDR_RD(value)                  (QSPI_RDR_RD_Msk & ((value) << QSPI_RDR_RD_Pos))
#define QSPI_RDR_MASK                       (0xFFFFU)                                      /**< \deprecated (QSPI_RDR) Register MASK  (Use QSPI_RDR_Msk instead)  */
#define QSPI_RDR_Msk                        (0xFFFFU)                                      /**< (QSPI_RDR) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- QSPI_TDR : (QSPI Offset: 0x0c) (/W 32) Transmit Data Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t TD:16;                     /**< bit:  0..15  Transmit Data                            */
    uint32_t :16;                       /**< bit: 16..31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} QSPI_TDR_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define QSPI_TDR_OFFSET                     (0x0C)                                        /**<  (QSPI_TDR) Transmit Data Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define QSPI_TDR_TD_Pos                     0                                              /**< (QSPI_TDR) Transmit Data Position */
#define QSPI_TDR_TD_Msk                     (0xFFFFU << QSPI_TDR_TD_Pos)                   /**< (QSPI_TDR) Transmit Data Mask */
#define QSPI_TDR_TD(value)                  (QSPI_TDR_TD_Msk & ((value) << QSPI_TDR_TD_Pos))
#define QSPI_TDR_MASK                       (0xFFFFU)                                      /**< \deprecated (QSPI_TDR) Register MASK  (Use QSPI_TDR_Msk instead)  */
#define QSPI_TDR_Msk                        (0xFFFFU)                                      /**< (QSPI_TDR) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- QSPI_SR : (QSPI Offset: 0x10) (R/ 32) Status Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t RDRF:1;                    /**< bit:      0  Receive Data Register Full               */
    uint32_t TDRE:1;                    /**< bit:      1  Transmit Data Register Empty             */
    uint32_t TXEMPTY:1;                 /**< bit:      2  Transmission Registers Empty             */
    uint32_t OVRES:1;                   /**< bit:      3  Overrun Error Status                     */
    uint32_t :4;                        /**< bit:   4..7  Reserved */
    uint32_t CSR:1;                     /**< bit:      8  Chip Select Rise                         */
    uint32_t CSS:1;                     /**< bit:      9  Chip Select Status                       */
    uint32_t INSTRE:1;                  /**< bit:     10  Instruction End Status                   */
    uint32_t :13;                       /**< bit: 11..23  Reserved */
    uint32_t QSPIENS:1;                 /**< bit:     24  QSPI Enable Status                       */
    uint32_t :7;                        /**< bit: 25..31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} QSPI_SR_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define QSPI_SR_OFFSET                      (0x10)                                        /**<  (QSPI_SR) Status Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define QSPI_SR_RDRF_Pos                    0                                              /**< (QSPI_SR) Receive Data Register Full Position */
#define QSPI_SR_RDRF_Msk                    (0x1U << QSPI_SR_RDRF_Pos)                     /**< (QSPI_SR) Receive Data Register Full Mask */
#define QSPI_SR_RDRF                        QSPI_SR_RDRF_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use QSPI_SR_RDRF_Msk instead */
#define QSPI_SR_TDRE_Pos                    1                                              /**< (QSPI_SR) Transmit Data Register Empty Position */
#define QSPI_SR_TDRE_Msk                    (0x1U << QSPI_SR_TDRE_Pos)                     /**< (QSPI_SR) Transmit Data Register Empty Mask */
#define QSPI_SR_TDRE                        QSPI_SR_TDRE_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use QSPI_SR_TDRE_Msk instead */
#define QSPI_SR_TXEMPTY_Pos                 2                                              /**< (QSPI_SR) Transmission Registers Empty Position */
#define QSPI_SR_TXEMPTY_Msk                 (0x1U << QSPI_SR_TXEMPTY_Pos)                  /**< (QSPI_SR) Transmission Registers Empty Mask */
#define QSPI_SR_TXEMPTY                     QSPI_SR_TXEMPTY_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use QSPI_SR_TXEMPTY_Msk instead */
#define QSPI_SR_OVRES_Pos                   3                                              /**< (QSPI_SR) Overrun Error Status Position */
#define QSPI_SR_OVRES_Msk                   (0x1U << QSPI_SR_OVRES_Pos)                    /**< (QSPI_SR) Overrun Error Status Mask */
#define QSPI_SR_OVRES                       QSPI_SR_OVRES_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use QSPI_SR_OVRES_Msk instead */
#define QSPI_SR_CSR_Pos                     8                                              /**< (QSPI_SR) Chip Select Rise Position */
#define QSPI_SR_CSR_Msk                     (0x1U << QSPI_SR_CSR_Pos)                      /**< (QSPI_SR) Chip Select Rise Mask */
#define QSPI_SR_CSR                         QSPI_SR_CSR_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use QSPI_SR_CSR_Msk instead */
#define QSPI_SR_CSS_Pos                     9                                              /**< (QSPI_SR) Chip Select Status Position */
#define QSPI_SR_CSS_Msk                     (0x1U << QSPI_SR_CSS_Pos)                      /**< (QSPI_SR) Chip Select Status Mask */
#define QSPI_SR_CSS                         QSPI_SR_CSS_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use QSPI_SR_CSS_Msk instead */
#define QSPI_SR_INSTRE_Pos                  10                                             /**< (QSPI_SR) Instruction End Status Position */
#define QSPI_SR_INSTRE_Msk                  (0x1U << QSPI_SR_INSTRE_Pos)                   /**< (QSPI_SR) Instruction End Status Mask */
#define QSPI_SR_INSTRE                      QSPI_SR_INSTRE_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use QSPI_SR_INSTRE_Msk instead */
#define QSPI_SR_QSPIENS_Pos                 24                                             /**< (QSPI_SR) QSPI Enable Status Position */
#define QSPI_SR_QSPIENS_Msk                 (0x1U << QSPI_SR_QSPIENS_Pos)                  /**< (QSPI_SR) QSPI Enable Status Mask */
#define QSPI_SR_QSPIENS                     QSPI_SR_QSPIENS_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use QSPI_SR_QSPIENS_Msk instead */
#define QSPI_SR_MASK                        (0x100070FU)                                   /**< \deprecated (QSPI_SR) Register MASK  (Use QSPI_SR_Msk instead)  */
#define QSPI_SR_Msk                         (0x100070FU)                                   /**< (QSPI_SR) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- QSPI_IER : (QSPI Offset: 0x14) (/W 32) Interrupt Enable Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t RDRF:1;                    /**< bit:      0  Receive Data Register Full Interrupt Enable */
    uint32_t TDRE:1;                    /**< bit:      1  Transmit Data Register Empty Interrupt Enable */
    uint32_t TXEMPTY:1;                 /**< bit:      2  Transmission Registers Empty Enable      */
    uint32_t OVRES:1;                   /**< bit:      3  Overrun Error Interrupt Enable           */
    uint32_t :4;                        /**< bit:   4..7  Reserved */
    uint32_t CSR:1;                     /**< bit:      8  Chip Select Rise Interrupt Enable        */
    uint32_t CSS:1;                     /**< bit:      9  Chip Select Status Interrupt Enable      */
    uint32_t INSTRE:1;                  /**< bit:     10  Instruction End Interrupt Enable         */
    uint32_t :21;                       /**< bit: 11..31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} QSPI_IER_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define QSPI_IER_OFFSET                     (0x14)                                        /**<  (QSPI_IER) Interrupt Enable Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define QSPI_IER_RDRF_Pos                   0                                              /**< (QSPI_IER) Receive Data Register Full Interrupt Enable Position */
#define QSPI_IER_RDRF_Msk                   (0x1U << QSPI_IER_RDRF_Pos)                    /**< (QSPI_IER) Receive Data Register Full Interrupt Enable Mask */
#define QSPI_IER_RDRF                       QSPI_IER_RDRF_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use QSPI_IER_RDRF_Msk instead */
#define QSPI_IER_TDRE_Pos                   1                                              /**< (QSPI_IER) Transmit Data Register Empty Interrupt Enable Position */
#define QSPI_IER_TDRE_Msk                   (0x1U << QSPI_IER_TDRE_Pos)                    /**< (QSPI_IER) Transmit Data Register Empty Interrupt Enable Mask */
#define QSPI_IER_TDRE                       QSPI_IER_TDRE_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use QSPI_IER_TDRE_Msk instead */
#define QSPI_IER_TXEMPTY_Pos                2                                              /**< (QSPI_IER) Transmission Registers Empty Enable Position */
#define QSPI_IER_TXEMPTY_Msk                (0x1U << QSPI_IER_TXEMPTY_Pos)                 /**< (QSPI_IER) Transmission Registers Empty Enable Mask */
#define QSPI_IER_TXEMPTY                    QSPI_IER_TXEMPTY_Msk                           /**< \deprecated Old style mask definition for 1 bit bitfield. Use QSPI_IER_TXEMPTY_Msk instead */
#define QSPI_IER_OVRES_Pos                  3                                              /**< (QSPI_IER) Overrun Error Interrupt Enable Position */
#define QSPI_IER_OVRES_Msk                  (0x1U << QSPI_IER_OVRES_Pos)                   /**< (QSPI_IER) Overrun Error Interrupt Enable Mask */
#define QSPI_IER_OVRES                      QSPI_IER_OVRES_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use QSPI_IER_OVRES_Msk instead */
#define QSPI_IER_CSR_Pos                    8                                              /**< (QSPI_IER) Chip Select Rise Interrupt Enable Position */
#define QSPI_IER_CSR_Msk                    (0x1U << QSPI_IER_CSR_Pos)                     /**< (QSPI_IER) Chip Select Rise Interrupt Enable Mask */
#define QSPI_IER_CSR                        QSPI_IER_CSR_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use QSPI_IER_CSR_Msk instead */
#define QSPI_IER_CSS_Pos                    9                                              /**< (QSPI_IER) Chip Select Status Interrupt Enable Position */
#define QSPI_IER_CSS_Msk                    (0x1U << QSPI_IER_CSS_Pos)                     /**< (QSPI_IER) Chip Select Status Interrupt Enable Mask */
#define QSPI_IER_CSS                        QSPI_IER_CSS_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use QSPI_IER_CSS_Msk instead */
#define QSPI_IER_INSTRE_Pos                 10                                             /**< (QSPI_IER) Instruction End Interrupt Enable Position */
#define QSPI_IER_INSTRE_Msk                 (0x1U << QSPI_IER_INSTRE_Pos)                  /**< (QSPI_IER) Instruction End Interrupt Enable Mask */
#define QSPI_IER_INSTRE                     QSPI_IER_INSTRE_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use QSPI_IER_INSTRE_Msk instead */
#define QSPI_IER_MASK                       (0x70FU)                                       /**< \deprecated (QSPI_IER) Register MASK  (Use QSPI_IER_Msk instead)  */
#define QSPI_IER_Msk                        (0x70FU)                                       /**< (QSPI_IER) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- QSPI_IDR : (QSPI Offset: 0x18) (/W 32) Interrupt Disable Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t RDRF:1;                    /**< bit:      0  Receive Data Register Full Interrupt Disable */
    uint32_t TDRE:1;                    /**< bit:      1  Transmit Data Register Empty Interrupt Disable */
    uint32_t TXEMPTY:1;                 /**< bit:      2  Transmission Registers Empty Disable     */
    uint32_t OVRES:1;                   /**< bit:      3  Overrun Error Interrupt Disable          */
    uint32_t :4;                        /**< bit:   4..7  Reserved */
    uint32_t CSR:1;                     /**< bit:      8  Chip Select Rise Interrupt Disable       */
    uint32_t CSS:1;                     /**< bit:      9  Chip Select Status Interrupt Disable     */
    uint32_t INSTRE:1;                  /**< bit:     10  Instruction End Interrupt Disable        */
    uint32_t :21;                       /**< bit: 11..31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} QSPI_IDR_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define QSPI_IDR_OFFSET                     (0x18)                                        /**<  (QSPI_IDR) Interrupt Disable Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define QSPI_IDR_RDRF_Pos                   0                                              /**< (QSPI_IDR) Receive Data Register Full Interrupt Disable Position */
#define QSPI_IDR_RDRF_Msk                   (0x1U << QSPI_IDR_RDRF_Pos)                    /**< (QSPI_IDR) Receive Data Register Full Interrupt Disable Mask */
#define QSPI_IDR_RDRF                       QSPI_IDR_RDRF_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use QSPI_IDR_RDRF_Msk instead */
#define QSPI_IDR_TDRE_Pos                   1                                              /**< (QSPI_IDR) Transmit Data Register Empty Interrupt Disable Position */
#define QSPI_IDR_TDRE_Msk                   (0x1U << QSPI_IDR_TDRE_Pos)                    /**< (QSPI_IDR) Transmit Data Register Empty Interrupt Disable Mask */
#define QSPI_IDR_TDRE                       QSPI_IDR_TDRE_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use QSPI_IDR_TDRE_Msk instead */
#define QSPI_IDR_TXEMPTY_Pos                2                                              /**< (QSPI_IDR) Transmission Registers Empty Disable Position */
#define QSPI_IDR_TXEMPTY_Msk                (0x1U << QSPI_IDR_TXEMPTY_Pos)                 /**< (QSPI_IDR) Transmission Registers Empty Disable Mask */
#define QSPI_IDR_TXEMPTY                    QSPI_IDR_TXEMPTY_Msk                           /**< \deprecated Old style mask definition for 1 bit bitfield. Use QSPI_IDR_TXEMPTY_Msk instead */
#define QSPI_IDR_OVRES_Pos                  3                                              /**< (QSPI_IDR) Overrun Error Interrupt Disable Position */
#define QSPI_IDR_OVRES_Msk                  (0x1U << QSPI_IDR_OVRES_Pos)                   /**< (QSPI_IDR) Overrun Error Interrupt Disable Mask */
#define QSPI_IDR_OVRES                      QSPI_IDR_OVRES_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use QSPI_IDR_OVRES_Msk instead */
#define QSPI_IDR_CSR_Pos                    8                                              /**< (QSPI_IDR) Chip Select Rise Interrupt Disable Position */
#define QSPI_IDR_CSR_Msk                    (0x1U << QSPI_IDR_CSR_Pos)                     /**< (QSPI_IDR) Chip Select Rise Interrupt Disable Mask */
#define QSPI_IDR_CSR                        QSPI_IDR_CSR_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use QSPI_IDR_CSR_Msk instead */
#define QSPI_IDR_CSS_Pos                    9                                              /**< (QSPI_IDR) Chip Select Status Interrupt Disable Position */
#define QSPI_IDR_CSS_Msk                    (0x1U << QSPI_IDR_CSS_Pos)                     /**< (QSPI_IDR) Chip Select Status Interrupt Disable Mask */
#define QSPI_IDR_CSS                        QSPI_IDR_CSS_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use QSPI_IDR_CSS_Msk instead */
#define QSPI_IDR_INSTRE_Pos                 10                                             /**< (QSPI_IDR) Instruction End Interrupt Disable Position */
#define QSPI_IDR_INSTRE_Msk                 (0x1U << QSPI_IDR_INSTRE_Pos)                  /**< (QSPI_IDR) Instruction End Interrupt Disable Mask */
#define QSPI_IDR_INSTRE                     QSPI_IDR_INSTRE_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use QSPI_IDR_INSTRE_Msk instead */
#define QSPI_IDR_MASK                       (0x70FU)                                       /**< \deprecated (QSPI_IDR) Register MASK  (Use QSPI_IDR_Msk instead)  */
#define QSPI_IDR_Msk                        (0x70FU)                                       /**< (QSPI_IDR) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- QSPI_IMR : (QSPI Offset: 0x1c) (R/ 32) Interrupt Mask Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t RDRF:1;                    /**< bit:      0  Receive Data Register Full Interrupt Mask */
    uint32_t TDRE:1;                    /**< bit:      1  Transmit Data Register Empty Interrupt Mask */
    uint32_t TXEMPTY:1;                 /**< bit:      2  Transmission Registers Empty Mask        */
    uint32_t OVRES:1;                   /**< bit:      3  Overrun Error Interrupt Mask             */
    uint32_t :4;                        /**< bit:   4..7  Reserved */
    uint32_t CSR:1;                     /**< bit:      8  Chip Select Rise Interrupt Mask          */
    uint32_t CSS:1;                     /**< bit:      9  Chip Select Status Interrupt Mask        */
    uint32_t INSTRE:1;                  /**< bit:     10  Instruction End Interrupt Mask           */
    uint32_t :21;                       /**< bit: 11..31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} QSPI_IMR_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define QSPI_IMR_OFFSET                     (0x1C)                                        /**<  (QSPI_IMR) Interrupt Mask Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define QSPI_IMR_RDRF_Pos                   0                                              /**< (QSPI_IMR) Receive Data Register Full Interrupt Mask Position */
#define QSPI_IMR_RDRF_Msk                   (0x1U << QSPI_IMR_RDRF_Pos)                    /**< (QSPI_IMR) Receive Data Register Full Interrupt Mask Mask */
#define QSPI_IMR_RDRF                       QSPI_IMR_RDRF_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use QSPI_IMR_RDRF_Msk instead */
#define QSPI_IMR_TDRE_Pos                   1                                              /**< (QSPI_IMR) Transmit Data Register Empty Interrupt Mask Position */
#define QSPI_IMR_TDRE_Msk                   (0x1U << QSPI_IMR_TDRE_Pos)                    /**< (QSPI_IMR) Transmit Data Register Empty Interrupt Mask Mask */
#define QSPI_IMR_TDRE                       QSPI_IMR_TDRE_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use QSPI_IMR_TDRE_Msk instead */
#define QSPI_IMR_TXEMPTY_Pos                2                                              /**< (QSPI_IMR) Transmission Registers Empty Mask Position */
#define QSPI_IMR_TXEMPTY_Msk                (0x1U << QSPI_IMR_TXEMPTY_Pos)                 /**< (QSPI_IMR) Transmission Registers Empty Mask Mask */
#define QSPI_IMR_TXEMPTY                    QSPI_IMR_TXEMPTY_Msk                           /**< \deprecated Old style mask definition for 1 bit bitfield. Use QSPI_IMR_TXEMPTY_Msk instead */
#define QSPI_IMR_OVRES_Pos                  3                                              /**< (QSPI_IMR) Overrun Error Interrupt Mask Position */
#define QSPI_IMR_OVRES_Msk                  (0x1U << QSPI_IMR_OVRES_Pos)                   /**< (QSPI_IMR) Overrun Error Interrupt Mask Mask */
#define QSPI_IMR_OVRES                      QSPI_IMR_OVRES_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use QSPI_IMR_OVRES_Msk instead */
#define QSPI_IMR_CSR_Pos                    8                                              /**< (QSPI_IMR) Chip Select Rise Interrupt Mask Position */
#define QSPI_IMR_CSR_Msk                    (0x1U << QSPI_IMR_CSR_Pos)                     /**< (QSPI_IMR) Chip Select Rise Interrupt Mask Mask */
#define QSPI_IMR_CSR                        QSPI_IMR_CSR_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use QSPI_IMR_CSR_Msk instead */
#define QSPI_IMR_CSS_Pos                    9                                              /**< (QSPI_IMR) Chip Select Status Interrupt Mask Position */
#define QSPI_IMR_CSS_Msk                    (0x1U << QSPI_IMR_CSS_Pos)                     /**< (QSPI_IMR) Chip Select Status Interrupt Mask Mask */
#define QSPI_IMR_CSS                        QSPI_IMR_CSS_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use QSPI_IMR_CSS_Msk instead */
#define QSPI_IMR_INSTRE_Pos                 10                                             /**< (QSPI_IMR) Instruction End Interrupt Mask Position */
#define QSPI_IMR_INSTRE_Msk                 (0x1U << QSPI_IMR_INSTRE_Pos)                  /**< (QSPI_IMR) Instruction End Interrupt Mask Mask */
#define QSPI_IMR_INSTRE                     QSPI_IMR_INSTRE_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use QSPI_IMR_INSTRE_Msk instead */
#define QSPI_IMR_MASK                       (0x70FU)                                       /**< \deprecated (QSPI_IMR) Register MASK  (Use QSPI_IMR_Msk instead)  */
#define QSPI_IMR_Msk                        (0x70FU)                                       /**< (QSPI_IMR) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- QSPI_SCR : (QSPI Offset: 0x20) (R/W 32) Serial Clock Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t CPOL:1;                    /**< bit:      0  Clock Polarity                           */
    uint32_t CPHA:1;                    /**< bit:      1  Clock Phase                              */
    uint32_t :6;                        /**< bit:   2..7  Reserved */
    uint32_t SCBR:8;                    /**< bit:  8..15  Serial Clock Baud Rate                   */
    uint32_t DLYBS:8;                   /**< bit: 16..23  Delay Before QSCK                        */
    uint32_t :8;                        /**< bit: 24..31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} QSPI_SCR_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define QSPI_SCR_OFFSET                     (0x20)                                        /**<  (QSPI_SCR) Serial Clock Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define QSPI_SCR_CPOL_Pos                   0                                              /**< (QSPI_SCR) Clock Polarity Position */
#define QSPI_SCR_CPOL_Msk                   (0x1U << QSPI_SCR_CPOL_Pos)                    /**< (QSPI_SCR) Clock Polarity Mask */
#define QSPI_SCR_CPOL                       QSPI_SCR_CPOL_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use QSPI_SCR_CPOL_Msk instead */
#define QSPI_SCR_CPHA_Pos                   1                                              /**< (QSPI_SCR) Clock Phase Position */
#define QSPI_SCR_CPHA_Msk                   (0x1U << QSPI_SCR_CPHA_Pos)                    /**< (QSPI_SCR) Clock Phase Mask */
#define QSPI_SCR_CPHA                       QSPI_SCR_CPHA_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use QSPI_SCR_CPHA_Msk instead */
#define QSPI_SCR_SCBR_Pos                   8                                              /**< (QSPI_SCR) Serial Clock Baud Rate Position */
#define QSPI_SCR_SCBR_Msk                   (0xFFU << QSPI_SCR_SCBR_Pos)                   /**< (QSPI_SCR) Serial Clock Baud Rate Mask */
#define QSPI_SCR_SCBR(value)                (QSPI_SCR_SCBR_Msk & ((value) << QSPI_SCR_SCBR_Pos))
#define QSPI_SCR_DLYBS_Pos                  16                                             /**< (QSPI_SCR) Delay Before QSCK Position */
#define QSPI_SCR_DLYBS_Msk                  (0xFFU << QSPI_SCR_DLYBS_Pos)                  /**< (QSPI_SCR) Delay Before QSCK Mask */
#define QSPI_SCR_DLYBS(value)               (QSPI_SCR_DLYBS_Msk & ((value) << QSPI_SCR_DLYBS_Pos))
#define QSPI_SCR_MASK                       (0xFFFF03U)                                    /**< \deprecated (QSPI_SCR) Register MASK  (Use QSPI_SCR_Msk instead)  */
#define QSPI_SCR_Msk                        (0xFFFF03U)                                    /**< (QSPI_SCR) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- QSPI_IAR : (QSPI Offset: 0x30) (R/W 32) Instruction Address Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t ADDR:32;                   /**< bit:  0..31  Address                                  */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} QSPI_IAR_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define QSPI_IAR_OFFSET                     (0x30)                                        /**<  (QSPI_IAR) Instruction Address Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define QSPI_IAR_ADDR_Pos                   0                                              /**< (QSPI_IAR) Address Position */
#define QSPI_IAR_ADDR_Msk                   (0xFFFFFFFFU << QSPI_IAR_ADDR_Pos)             /**< (QSPI_IAR) Address Mask */
#define QSPI_IAR_ADDR(value)                (QSPI_IAR_ADDR_Msk & ((value) << QSPI_IAR_ADDR_Pos))
#define QSPI_IAR_MASK                       (0xFFFFFFFFU)                                  /**< \deprecated (QSPI_IAR) Register MASK  (Use QSPI_IAR_Msk instead)  */
#define QSPI_IAR_Msk                        (0xFFFFFFFFU)                                  /**< (QSPI_IAR) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- QSPI_ICR : (QSPI Offset: 0x34) (R/W 32) Instruction Code Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t INST:8;                    /**< bit:   0..7  Instruction Code                         */
    uint32_t :8;                        /**< bit:  8..15  Reserved */
    uint32_t OPT:8;                     /**< bit: 16..23  Option Code                              */
    uint32_t :8;                        /**< bit: 24..31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} QSPI_ICR_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define QSPI_ICR_OFFSET                     (0x34)                                        /**<  (QSPI_ICR) Instruction Code Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define QSPI_ICR_INST_Pos                   0                                              /**< (QSPI_ICR) Instruction Code Position */
#define QSPI_ICR_INST_Msk                   (0xFFU << QSPI_ICR_INST_Pos)                   /**< (QSPI_ICR) Instruction Code Mask */
#define QSPI_ICR_INST(value)                (QSPI_ICR_INST_Msk & ((value) << QSPI_ICR_INST_Pos))
#define QSPI_ICR_OPT_Pos                    16                                             /**< (QSPI_ICR) Option Code Position */
#define QSPI_ICR_OPT_Msk                    (0xFFU << QSPI_ICR_OPT_Pos)                    /**< (QSPI_ICR) Option Code Mask */
#define QSPI_ICR_OPT(value)                 (QSPI_ICR_OPT_Msk & ((value) << QSPI_ICR_OPT_Pos))
#define QSPI_ICR_MASK                       (0xFF00FFU)                                    /**< \deprecated (QSPI_ICR) Register MASK  (Use QSPI_ICR_Msk instead)  */
#define QSPI_ICR_Msk                        (0xFF00FFU)                                    /**< (QSPI_ICR) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- QSPI_IFR : (QSPI Offset: 0x38) (R/W 32) Instruction Frame Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t WIDTH:3;                   /**< bit:   0..2  Width of Instruction Code, Address, Option Code and Data */
    uint32_t :1;                        /**< bit:      3  Reserved */
    uint32_t INSTEN:1;                  /**< bit:      4  Instruction Enable                       */
    uint32_t ADDREN:1;                  /**< bit:      5  Address Enable                           */
    uint32_t OPTEN:1;                   /**< bit:      6  Option Enable                            */
    uint32_t DATAEN:1;                  /**< bit:      7  Data Enable                              */
    uint32_t OPTL:2;                    /**< bit:   8..9  Option Code Length                       */
    uint32_t ADDRL:1;                   /**< bit:     10  Address Length                           */
    uint32_t :1;                        /**< bit:     11  Reserved */
    uint32_t TFRTYP:2;                  /**< bit: 12..13  Data Transfer Type                       */
    uint32_t CRM:1;                     /**< bit:     14  Continuous Read Mode                     */
    uint32_t :1;                        /**< bit:     15  Reserved */
    uint32_t NBDUM:5;                   /**< bit: 16..20  Number Of Dummy Cycles                   */
    uint32_t :11;                       /**< bit: 21..31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} QSPI_IFR_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define QSPI_IFR_OFFSET                     (0x38)                                        /**<  (QSPI_IFR) Instruction Frame Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define QSPI_IFR_WIDTH_Pos                  0                                              /**< (QSPI_IFR) Width of Instruction Code, Address, Option Code and Data Position */
#define QSPI_IFR_WIDTH_Msk                  (0x7U << QSPI_IFR_WIDTH_Pos)                   /**< (QSPI_IFR) Width of Instruction Code, Address, Option Code and Data Mask */
#define QSPI_IFR_WIDTH(value)               (QSPI_IFR_WIDTH_Msk & ((value) << QSPI_IFR_WIDTH_Pos))
#define   QSPI_IFR_WIDTH_SINGLE_BIT_SPI_Val (0x0U)                                         /**< (QSPI_IFR) Instruction: Single-bit SPI / Address-Option: Single-bit SPI / Data: Single-bit SPI  */
#define   QSPI_IFR_WIDTH_DUAL_OUTPUT_Val    (0x1U)                                         /**< (QSPI_IFR) Instruction: Single-bit SPI / Address-Option: Single-bit SPI / Data: Dual SPI  */
#define   QSPI_IFR_WIDTH_QUAD_OUTPUT_Val    (0x2U)                                         /**< (QSPI_IFR) Instruction: Single-bit SPI / Address-Option: Single-bit SPI / Data: Quad SPI  */
#define   QSPI_IFR_WIDTH_DUAL_IO_Val        (0x3U)                                         /**< (QSPI_IFR) Instruction: Single-bit SPI / Address-Option: Dual SPI / Data: Dual SPI  */
#define   QSPI_IFR_WIDTH_QUAD_IO_Val        (0x4U)                                         /**< (QSPI_IFR) Instruction: Single-bit SPI / Address-Option: Quad SPI / Data: Quad SPI  */
#define   QSPI_IFR_WIDTH_DUAL_CMD_Val       (0x5U)                                         /**< (QSPI_IFR) Instruction: Dual SPI / Address-Option: Dual SPI / Data: Dual SPI  */
#define   QSPI_IFR_WIDTH_QUAD_CMD_Val       (0x6U)                                         /**< (QSPI_IFR) Instruction: Quad SPI / Address-Option: Quad SPI / Data: Quad SPI  */
#define QSPI_IFR_WIDTH_SINGLE_BIT_SPI       (QSPI_IFR_WIDTH_SINGLE_BIT_SPI_Val << QSPI_IFR_WIDTH_Pos)  /**< (QSPI_IFR) Instruction: Single-bit SPI / Address-Option: Single-bit SPI / Data: Single-bit SPI Position  */
#define QSPI_IFR_WIDTH_DUAL_OUTPUT          (QSPI_IFR_WIDTH_DUAL_OUTPUT_Val << QSPI_IFR_WIDTH_Pos)  /**< (QSPI_IFR) Instruction: Single-bit SPI / Address-Option: Single-bit SPI / Data: Dual SPI Position  */
#define QSPI_IFR_WIDTH_QUAD_OUTPUT          (QSPI_IFR_WIDTH_QUAD_OUTPUT_Val << QSPI_IFR_WIDTH_Pos)  /**< (QSPI_IFR) Instruction: Single-bit SPI / Address-Option: Single-bit SPI / Data: Quad SPI Position  */
#define QSPI_IFR_WIDTH_DUAL_IO              (QSPI_IFR_WIDTH_DUAL_IO_Val << QSPI_IFR_WIDTH_Pos)  /**< (QSPI_IFR) Instruction: Single-bit SPI / Address-Option: Dual SPI / Data: Dual SPI Position  */
#define QSPI_IFR_WIDTH_QUAD_IO              (QSPI_IFR_WIDTH_QUAD_IO_Val << QSPI_IFR_WIDTH_Pos)  /**< (QSPI_IFR) Instruction: Single-bit SPI / Address-Option: Quad SPI / Data: Quad SPI Position  */
#define QSPI_IFR_WIDTH_DUAL_CMD             (QSPI_IFR_WIDTH_DUAL_CMD_Val << QSPI_IFR_WIDTH_Pos)  /**< (QSPI_IFR) Instruction: Dual SPI / Address-Option: Dual SPI / Data: Dual SPI Position  */
#define QSPI_IFR_WIDTH_QUAD_CMD             (QSPI_IFR_WIDTH_QUAD_CMD_Val << QSPI_IFR_WIDTH_Pos)  /**< (QSPI_IFR) Instruction: Quad SPI / Address-Option: Quad SPI / Data: Quad SPI Position  */
#define QSPI_IFR_INSTEN_Pos                 4                                              /**< (QSPI_IFR) Instruction Enable Position */
#define QSPI_IFR_INSTEN_Msk                 (0x1U << QSPI_IFR_INSTEN_Pos)                  /**< (QSPI_IFR) Instruction Enable Mask */
#define QSPI_IFR_INSTEN                     QSPI_IFR_INSTEN_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use QSPI_IFR_INSTEN_Msk instead */
#define QSPI_IFR_ADDREN_Pos                 5                                              /**< (QSPI_IFR) Address Enable Position */
#define QSPI_IFR_ADDREN_Msk                 (0x1U << QSPI_IFR_ADDREN_Pos)                  /**< (QSPI_IFR) Address Enable Mask */
#define QSPI_IFR_ADDREN                     QSPI_IFR_ADDREN_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use QSPI_IFR_ADDREN_Msk instead */
#define QSPI_IFR_OPTEN_Pos                  6                                              /**< (QSPI_IFR) Option Enable Position */
#define QSPI_IFR_OPTEN_Msk                  (0x1U << QSPI_IFR_OPTEN_Pos)                   /**< (QSPI_IFR) Option Enable Mask */
#define QSPI_IFR_OPTEN                      QSPI_IFR_OPTEN_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use QSPI_IFR_OPTEN_Msk instead */
#define QSPI_IFR_DATAEN_Pos                 7                                              /**< (QSPI_IFR) Data Enable Position */
#define QSPI_IFR_DATAEN_Msk                 (0x1U << QSPI_IFR_DATAEN_Pos)                  /**< (QSPI_IFR) Data Enable Mask */
#define QSPI_IFR_DATAEN                     QSPI_IFR_DATAEN_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use QSPI_IFR_DATAEN_Msk instead */
#define QSPI_IFR_OPTL_Pos                   8                                              /**< (QSPI_IFR) Option Code Length Position */
#define QSPI_IFR_OPTL_Msk                   (0x3U << QSPI_IFR_OPTL_Pos)                    /**< (QSPI_IFR) Option Code Length Mask */
#define QSPI_IFR_OPTL(value)                (QSPI_IFR_OPTL_Msk & ((value) << QSPI_IFR_OPTL_Pos))
#define   QSPI_IFR_OPTL_OPTION_1BIT_Val     (0x0U)                                         /**< (QSPI_IFR) The option code is 1 bit long.  */
#define   QSPI_IFR_OPTL_OPTION_2BIT_Val     (0x1U)                                         /**< (QSPI_IFR) The option code is 2 bits long.  */
#define   QSPI_IFR_OPTL_OPTION_4BIT_Val     (0x2U)                                         /**< (QSPI_IFR) The option code is 4 bits long.  */
#define   QSPI_IFR_OPTL_OPTION_8BIT_Val     (0x3U)                                         /**< (QSPI_IFR) The option code is 8 bits long.  */
#define QSPI_IFR_OPTL_OPTION_1BIT           (QSPI_IFR_OPTL_OPTION_1BIT_Val << QSPI_IFR_OPTL_Pos)  /**< (QSPI_IFR) The option code is 1 bit long. Position  */
#define QSPI_IFR_OPTL_OPTION_2BIT           (QSPI_IFR_OPTL_OPTION_2BIT_Val << QSPI_IFR_OPTL_Pos)  /**< (QSPI_IFR) The option code is 2 bits long. Position  */
#define QSPI_IFR_OPTL_OPTION_4BIT           (QSPI_IFR_OPTL_OPTION_4BIT_Val << QSPI_IFR_OPTL_Pos)  /**< (QSPI_IFR) The option code is 4 bits long. Position  */
#define QSPI_IFR_OPTL_OPTION_8BIT           (QSPI_IFR_OPTL_OPTION_8BIT_Val << QSPI_IFR_OPTL_Pos)  /**< (QSPI_IFR) The option code is 8 bits long. Position  */
#define QSPI_IFR_ADDRL_Pos                  10                                             /**< (QSPI_IFR) Address Length Position */
#define QSPI_IFR_ADDRL_Msk                  (0x1U << QSPI_IFR_ADDRL_Pos)                   /**< (QSPI_IFR) Address Length Mask */
#define QSPI_IFR_ADDRL                      QSPI_IFR_ADDRL_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use QSPI_IFR_ADDRL_Msk instead */
#define   QSPI_IFR_ADDRL_24_BIT_Val         (0x0U)                                         /**< (QSPI_IFR) The address is 24 bits long.  */
#define   QSPI_IFR_ADDRL_32_BIT_Val         (0x1U)                                         /**< (QSPI_IFR) The address is 32 bits long.  */
#define QSPI_IFR_ADDRL_24_BIT               (QSPI_IFR_ADDRL_24_BIT_Val << QSPI_IFR_ADDRL_Pos)  /**< (QSPI_IFR) The address is 24 bits long. Position  */
#define QSPI_IFR_ADDRL_32_BIT               (QSPI_IFR_ADDRL_32_BIT_Val << QSPI_IFR_ADDRL_Pos)  /**< (QSPI_IFR) The address is 32 bits long. Position  */
#define QSPI_IFR_TFRTYP_Pos                 12                                             /**< (QSPI_IFR) Data Transfer Type Position */
#define QSPI_IFR_TFRTYP_Msk                 (0x3U << QSPI_IFR_TFRTYP_Pos)                  /**< (QSPI_IFR) Data Transfer Type Mask */
#define QSPI_IFR_TFRTYP(value)              (QSPI_IFR_TFRTYP_Msk & ((value) << QSPI_IFR_TFRTYP_Pos))
#define   QSPI_IFR_TFRTYP_TRSFR_READ_Val    (0x0U)                                         /**< (QSPI_IFR) Read transfer from the serial memory.Scrambling is not performed.Read at random location (fetch) in the serial Flash memory is not possible.  */
#define   QSPI_IFR_TFRTYP_TRSFR_READ_MEMORY_Val (0x1U)                                         /**< (QSPI_IFR) Read data transfer from the serial memory.If enabled, scrambling is performed.Read at random location (fetch) in the serial Flash memory is possible.  */
#define   QSPI_IFR_TFRTYP_TRSFR_WRITE_Val   (0x2U)                                         /**< (QSPI_IFR) Write transfer into the serial memory.Scrambling is not performed.  */
#define   QSPI_IFR_TFRTYP_TRSFR_WRITE_MEMORY_Val (0x3U)                                         /**< (QSPI_IFR) Write data transfer into the serial memory.If enabled, scrambling is performed.  */
#define QSPI_IFR_TFRTYP_TRSFR_READ          (QSPI_IFR_TFRTYP_TRSFR_READ_Val << QSPI_IFR_TFRTYP_Pos)  /**< (QSPI_IFR) Read transfer from the serial memory.Scrambling is not performed.Read at random location (fetch) in the serial Flash memory is not possible. Position  */
#define QSPI_IFR_TFRTYP_TRSFR_READ_MEMORY   (QSPI_IFR_TFRTYP_TRSFR_READ_MEMORY_Val << QSPI_IFR_TFRTYP_Pos)  /**< (QSPI_IFR) Read data transfer from the serial memory.If enabled, scrambling is performed.Read at random location (fetch) in the serial Flash memory is possible. Position  */
#define QSPI_IFR_TFRTYP_TRSFR_WRITE         (QSPI_IFR_TFRTYP_TRSFR_WRITE_Val << QSPI_IFR_TFRTYP_Pos)  /**< (QSPI_IFR) Write transfer into the serial memory.Scrambling is not performed. Position  */
#define QSPI_IFR_TFRTYP_TRSFR_WRITE_MEMORY  (QSPI_IFR_TFRTYP_TRSFR_WRITE_MEMORY_Val << QSPI_IFR_TFRTYP_Pos)  /**< (QSPI_IFR) Write data transfer into the serial memory.If enabled, scrambling is performed. Position  */
#define QSPI_IFR_CRM_Pos                    14                                             /**< (QSPI_IFR) Continuous Read Mode Position */
#define QSPI_IFR_CRM_Msk                    (0x1U << QSPI_IFR_CRM_Pos)                     /**< (QSPI_IFR) Continuous Read Mode Mask */
#define QSPI_IFR_CRM                        QSPI_IFR_CRM_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use QSPI_IFR_CRM_Msk instead */
#define   QSPI_IFR_CRM_DISABLED_Val         (0x0U)                                         /**< (QSPI_IFR) The Continuous Read mode is disabled.  */
#define   QSPI_IFR_CRM_ENABLED_Val          (0x1U)                                         /**< (QSPI_IFR) The Continuous Read mode is enabled.  */
#define QSPI_IFR_CRM_DISABLED               (QSPI_IFR_CRM_DISABLED_Val << QSPI_IFR_CRM_Pos)  /**< (QSPI_IFR) The Continuous Read mode is disabled. Position  */
#define QSPI_IFR_CRM_ENABLED                (QSPI_IFR_CRM_ENABLED_Val << QSPI_IFR_CRM_Pos)  /**< (QSPI_IFR) The Continuous Read mode is enabled. Position  */
#define QSPI_IFR_NBDUM_Pos                  16                                             /**< (QSPI_IFR) Number Of Dummy Cycles Position */
#define QSPI_IFR_NBDUM_Msk                  (0x1FU << QSPI_IFR_NBDUM_Pos)                  /**< (QSPI_IFR) Number Of Dummy Cycles Mask */
#define QSPI_IFR_NBDUM(value)               (QSPI_IFR_NBDUM_Msk & ((value) << QSPI_IFR_NBDUM_Pos))
#define QSPI_IFR_MASK                       (0x1F77F7U)                                    /**< \deprecated (QSPI_IFR) Register MASK  (Use QSPI_IFR_Msk instead)  */
#define QSPI_IFR_Msk                        (0x1F77F7U)                                    /**< (QSPI_IFR) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- QSPI_SMR : (QSPI Offset: 0x40) (R/W 32) Scrambling Mode Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t SCREN:1;                   /**< bit:      0  Scrambling/Unscrambling Enable           */
    uint32_t RVDIS:1;                   /**< bit:      1  Scrambling/Unscrambling Random Value Disable */
    uint32_t :30;                       /**< bit:  2..31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} QSPI_SMR_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define QSPI_SMR_OFFSET                     (0x40)                                        /**<  (QSPI_SMR) Scrambling Mode Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define QSPI_SMR_SCREN_Pos                  0                                              /**< (QSPI_SMR) Scrambling/Unscrambling Enable Position */
#define QSPI_SMR_SCREN_Msk                  (0x1U << QSPI_SMR_SCREN_Pos)                   /**< (QSPI_SMR) Scrambling/Unscrambling Enable Mask */
#define QSPI_SMR_SCREN                      QSPI_SMR_SCREN_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use QSPI_SMR_SCREN_Msk instead */
#define   QSPI_SMR_SCREN_DISABLED_Val       (0x0U)                                         /**< (QSPI_SMR) The scrambling/unscrambling is disabled.  */
#define   QSPI_SMR_SCREN_ENABLED_Val        (0x1U)                                         /**< (QSPI_SMR) The scrambling/unscrambling is enabled.  */
#define QSPI_SMR_SCREN_DISABLED             (QSPI_SMR_SCREN_DISABLED_Val << QSPI_SMR_SCREN_Pos)  /**< (QSPI_SMR) The scrambling/unscrambling is disabled. Position  */
#define QSPI_SMR_SCREN_ENABLED              (QSPI_SMR_SCREN_ENABLED_Val << QSPI_SMR_SCREN_Pos)  /**< (QSPI_SMR) The scrambling/unscrambling is enabled. Position  */
#define QSPI_SMR_RVDIS_Pos                  1                                              /**< (QSPI_SMR) Scrambling/Unscrambling Random Value Disable Position */
#define QSPI_SMR_RVDIS_Msk                  (0x1U << QSPI_SMR_RVDIS_Pos)                   /**< (QSPI_SMR) Scrambling/Unscrambling Random Value Disable Mask */
#define QSPI_SMR_RVDIS                      QSPI_SMR_RVDIS_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use QSPI_SMR_RVDIS_Msk instead */
#define QSPI_SMR_MASK                       (0x03U)                                        /**< \deprecated (QSPI_SMR) Register MASK  (Use QSPI_SMR_Msk instead)  */
#define QSPI_SMR_Msk                        (0x03U)                                        /**< (QSPI_SMR) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- QSPI_SKR : (QSPI Offset: 0x44) (/W 32) Scrambling Key Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t USRK:32;                   /**< bit:  0..31  Scrambling User Key                      */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} QSPI_SKR_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define QSPI_SKR_OFFSET                     (0x44)                                        /**<  (QSPI_SKR) Scrambling Key Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define QSPI_SKR_USRK_Pos                   0                                              /**< (QSPI_SKR) Scrambling User Key Position */
#define QSPI_SKR_USRK_Msk                   (0xFFFFFFFFU << QSPI_SKR_USRK_Pos)             /**< (QSPI_SKR) Scrambling User Key Mask */
#define QSPI_SKR_USRK(value)                (QSPI_SKR_USRK_Msk & ((value) << QSPI_SKR_USRK_Pos))
#define QSPI_SKR_MASK                       (0xFFFFFFFFU)                                  /**< \deprecated (QSPI_SKR) Register MASK  (Use QSPI_SKR_Msk instead)  */
#define QSPI_SKR_Msk                        (0xFFFFFFFFU)                                  /**< (QSPI_SKR) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- QSPI_WPMR : (QSPI Offset: 0xe4) (R/W 32) Write Protection Mode Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t WPEN:1;                    /**< bit:      0  Write Protection Enable                  */
    uint32_t :7;                        /**< bit:   1..7  Reserved */
    uint32_t WPKEY:24;                  /**< bit:  8..31  Write Protection Key                     */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} QSPI_WPMR_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define QSPI_WPMR_OFFSET                    (0xE4)                                        /**<  (QSPI_WPMR) Write Protection Mode Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define QSPI_WPMR_WPEN_Pos                  0                                              /**< (QSPI_WPMR) Write Protection Enable Position */
#define QSPI_WPMR_WPEN_Msk                  (0x1U << QSPI_WPMR_WPEN_Pos)                   /**< (QSPI_WPMR) Write Protection Enable Mask */
#define QSPI_WPMR_WPEN                      QSPI_WPMR_WPEN_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use QSPI_WPMR_WPEN_Msk instead */
#define QSPI_WPMR_WPKEY_Pos                 8                                              /**< (QSPI_WPMR) Write Protection Key Position */
#define QSPI_WPMR_WPKEY_Msk                 (0xFFFFFFU << QSPI_WPMR_WPKEY_Pos)             /**< (QSPI_WPMR) Write Protection Key Mask */
#define QSPI_WPMR_WPKEY(value)              (QSPI_WPMR_WPKEY_Msk & ((value) << QSPI_WPMR_WPKEY_Pos))
#define   QSPI_WPMR_WPKEY_PASSWD_Val        (0x515350U)                                    /**< (QSPI_WPMR) Writing any other value in this field aborts the write operation of the WPEN bit. Always reads as 0.  */
#define QSPI_WPMR_WPKEY_PASSWD              (QSPI_WPMR_WPKEY_PASSWD_Val << QSPI_WPMR_WPKEY_Pos)  /**< (QSPI_WPMR) Writing any other value in this field aborts the write operation of the WPEN bit. Always reads as 0. Position  */
#define QSPI_WPMR_MASK                      (0xFFFFFF01U)                                  /**< \deprecated (QSPI_WPMR) Register MASK  (Use QSPI_WPMR_Msk instead)  */
#define QSPI_WPMR_Msk                       (0xFFFFFF01U)                                  /**< (QSPI_WPMR) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- QSPI_WPSR : (QSPI Offset: 0xe8) (R/ 32) Write Protection Status Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t WPVS:1;                    /**< bit:      0  Write Protection Violation Status        */
    uint32_t :7;                        /**< bit:   1..7  Reserved */
    uint32_t WPVSRC:8;                  /**< bit:  8..15  Write Protection Violation Source        */
    uint32_t :16;                       /**< bit: 16..31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} QSPI_WPSR_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define QSPI_WPSR_OFFSET                    (0xE8)                                        /**<  (QSPI_WPSR) Write Protection Status Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define QSPI_WPSR_WPVS_Pos                  0                                              /**< (QSPI_WPSR) Write Protection Violation Status Position */
#define QSPI_WPSR_WPVS_Msk                  (0x1U << QSPI_WPSR_WPVS_Pos)                   /**< (QSPI_WPSR) Write Protection Violation Status Mask */
#define QSPI_WPSR_WPVS                      QSPI_WPSR_WPVS_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use QSPI_WPSR_WPVS_Msk instead */
#define QSPI_WPSR_WPVSRC_Pos                8                                              /**< (QSPI_WPSR) Write Protection Violation Source Position */
#define QSPI_WPSR_WPVSRC_Msk                (0xFFU << QSPI_WPSR_WPVSRC_Pos)                /**< (QSPI_WPSR) Write Protection Violation Source Mask */
#define QSPI_WPSR_WPVSRC(value)             (QSPI_WPSR_WPVSRC_Msk & ((value) << QSPI_WPSR_WPVSRC_Pos))
#define QSPI_WPSR_MASK                      (0xFF01U)                                      /**< \deprecated (QSPI_WPSR) Register MASK  (Use QSPI_WPSR_Msk instead)  */
#define QSPI_WPSR_Msk                       (0xFF01U)                                      /**< (QSPI_WPSR) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
#if COMPONENT_TYPEDEF_STYLE == 'R'
/** \brief QSPI hardware registers */
typedef struct {  
  __O  uint32_t QSPI_CR;        /**< (QSPI Offset: 0x00) Control Register */
  __IO uint32_t QSPI_MR;        /**< (QSPI Offset: 0x04) Mode Register */
  __I  uint32_t QSPI_RDR;       /**< (QSPI Offset: 0x08) Receive Data Register */
  __O  uint32_t QSPI_TDR;       /**< (QSPI Offset: 0x0C) Transmit Data Register */
  __I  uint32_t QSPI_SR;        /**< (QSPI Offset: 0x10) Status Register */
  __O  uint32_t QSPI_IER;       /**< (QSPI Offset: 0x14) Interrupt Enable Register */
  __O  uint32_t QSPI_IDR;       /**< (QSPI Offset: 0x18) Interrupt Disable Register */
  __I  uint32_t QSPI_IMR;       /**< (QSPI Offset: 0x1C) Interrupt Mask Register */
  __IO uint32_t QSPI_SCR;       /**< (QSPI Offset: 0x20) Serial Clock Register */
  __I  uint32_t Reserved1[3];
  __IO uint32_t QSPI_IAR;       /**< (QSPI Offset: 0x30) Instruction Address Register */
  __IO uint32_t QSPI_ICR;       /**< (QSPI Offset: 0x34) Instruction Code Register */
  __IO uint32_t QSPI_IFR;       /**< (QSPI Offset: 0x38) Instruction Frame Register */
  __I  uint32_t Reserved2[1];
  __IO uint32_t QSPI_SMR;       /**< (QSPI Offset: 0x40) Scrambling Mode Register */
  __O  uint32_t QSPI_SKR;       /**< (QSPI Offset: 0x44) Scrambling Key Register */
  __I  uint32_t Reserved3[39];
  __IO uint32_t QSPI_WPMR;      /**< (QSPI Offset: 0xE4) Write Protection Mode Register */
  __I  uint32_t QSPI_WPSR;      /**< (QSPI Offset: 0xE8) Write Protection Status Register */
} Qspi;

#elif COMPONENT_TYPEDEF_STYLE == 'N'
/** \brief QSPI hardware registers */
typedef struct {  
  __O  QSPI_CR_Type                   QSPI_CR;        /**< Offset: 0x00 ( /W  32) Control Register */
  __IO QSPI_MR_Type                   QSPI_MR;        /**< Offset: 0x04 (R/W  32) Mode Register */
  __I  QSPI_RDR_Type                  QSPI_RDR;       /**< Offset: 0x08 (R/   32) Receive Data Register */
  __O  QSPI_TDR_Type                  QSPI_TDR;       /**< Offset: 0x0C ( /W  32) Transmit Data Register */
  __I  QSPI_SR_Type                   QSPI_SR;        /**< Offset: 0x10 (R/   32) Status Register */
  __O  QSPI_IER_Type                  QSPI_IER;       /**< Offset: 0x14 ( /W  32) Interrupt Enable Register */
  __O  QSPI_IDR_Type                  QSPI_IDR;       /**< Offset: 0x18 ( /W  32) Interrupt Disable Register */
  __I  QSPI_IMR_Type                  QSPI_IMR;       /**< Offset: 0x1C (R/   32) Interrupt Mask Register */
  __IO QSPI_SCR_Type                  QSPI_SCR;       /**< Offset: 0x20 (R/W  32) Serial Clock Register */
       RoReg8                         Reserved1[0xC];
  __IO QSPI_IAR_Type                  QSPI_IAR;       /**< Offset: 0x30 (R/W  32) Instruction Address Register */
  __IO QSPI_ICR_Type                  QSPI_ICR;       /**< Offset: 0x34 (R/W  32) Instruction Code Register */
  __IO QSPI_IFR_Type                  QSPI_IFR;       /**< Offset: 0x38 (R/W  32) Instruction Frame Register */
       RoReg8                         Reserved2[0x4];
  __IO QSPI_SMR_Type                  QSPI_SMR;       /**< Offset: 0x40 (R/W  32) Scrambling Mode Register */
  __O  QSPI_SKR_Type                  QSPI_SKR;       /**< Offset: 0x44 ( /W  32) Scrambling Key Register */
       RoReg8                         Reserved3[0x9C];
  __IO QSPI_WPMR_Type                 QSPI_WPMR;      /**< Offset: 0xE4 (R/W  32) Write Protection Mode Register */
  __I  QSPI_WPSR_Type                 QSPI_WPSR;      /**< Offset: 0xE8 (R/   32) Write Protection Status Register */
} Qspi;

#else /* COMPONENT_TYPEDEF_STYLE */
#error Unknown component typedef style
#endif /* COMPONENT_TYPEDEF_STYLE */

#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/** @}  end of Quad Serial Peripheral Interface */

#endif /* _SAME70_QSPI_COMPONENT_H_ */
