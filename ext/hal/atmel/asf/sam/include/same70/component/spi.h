/**
 * \file
 *
 * \brief Component description for SPI
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

#ifndef _SAME70_SPI_COMPONENT_H_
#define _SAME70_SPI_COMPONENT_H_
#define _SAME70_SPI_COMPONENT_         /**< \deprecated  Backward compatibility for ASF */

/** \addtogroup SAME70_SPI Serial Peripheral Interface
 *  @{
 */
/* ========================================================================== */
/**  SOFTWARE API DEFINITION FOR SPI */
/* ========================================================================== */
#ifndef COMPONENT_TYPEDEF_STYLE
  #define COMPONENT_TYPEDEF_STYLE 'R'  /**< Defines default style of typedefs for the component header files ('R' = RFO, 'N' = NTO)*/
#endif

#define SPI_6088                       /**< (SPI) Module ID */
#define REV_SPI ZF                     /**< (SPI) Module revision */

/* -------- SPI_CR : (SPI Offset: 0x00) (/W 32) Control Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t SPIEN:1;                   /**< bit:      0  SPI Enable                               */
    uint32_t SPIDIS:1;                  /**< bit:      1  SPI Disable                              */
    uint32_t :5;                        /**< bit:   2..6  Reserved */
    uint32_t SWRST:1;                   /**< bit:      7  SPI Software Reset                       */
    uint32_t :4;                        /**< bit:  8..11  Reserved */
    uint32_t REQCLR:1;                  /**< bit:     12  Request to Clear the Comparison Trigger  */
    uint32_t :3;                        /**< bit: 13..15  Reserved */
    uint32_t TXFCLR:1;                  /**< bit:     16  Transmit FIFO Clear                      */
    uint32_t RXFCLR:1;                  /**< bit:     17  Receive FIFO Clear                       */
    uint32_t :6;                        /**< bit: 18..23  Reserved */
    uint32_t LASTXFER:1;                /**< bit:     24  Last Transfer                            */
    uint32_t :5;                        /**< bit: 25..29  Reserved */
    uint32_t FIFOEN:1;                  /**< bit:     30  FIFO Enable                              */
    uint32_t FIFODIS:1;                 /**< bit:     31  FIFO Disable                             */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} SPI_CR_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define SPI_CR_OFFSET                       (0x00)                                        /**<  (SPI_CR) Control Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define SPI_CR_SPIEN_Pos                    0                                              /**< (SPI_CR) SPI Enable Position */
#define SPI_CR_SPIEN_Msk                    (0x1U << SPI_CR_SPIEN_Pos)                     /**< (SPI_CR) SPI Enable Mask */
#define SPI_CR_SPIEN                        SPI_CR_SPIEN_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use SPI_CR_SPIEN_Msk instead */
#define SPI_CR_SPIDIS_Pos                   1                                              /**< (SPI_CR) SPI Disable Position */
#define SPI_CR_SPIDIS_Msk                   (0x1U << SPI_CR_SPIDIS_Pos)                    /**< (SPI_CR) SPI Disable Mask */
#define SPI_CR_SPIDIS                       SPI_CR_SPIDIS_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use SPI_CR_SPIDIS_Msk instead */
#define SPI_CR_SWRST_Pos                    7                                              /**< (SPI_CR) SPI Software Reset Position */
#define SPI_CR_SWRST_Msk                    (0x1U << SPI_CR_SWRST_Pos)                     /**< (SPI_CR) SPI Software Reset Mask */
#define SPI_CR_SWRST                        SPI_CR_SWRST_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use SPI_CR_SWRST_Msk instead */
#define SPI_CR_REQCLR_Pos                   12                                             /**< (SPI_CR) Request to Clear the Comparison Trigger Position */
#define SPI_CR_REQCLR_Msk                   (0x1U << SPI_CR_REQCLR_Pos)                    /**< (SPI_CR) Request to Clear the Comparison Trigger Mask */
#define SPI_CR_REQCLR                       SPI_CR_REQCLR_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use SPI_CR_REQCLR_Msk instead */
#define SPI_CR_TXFCLR_Pos                   16                                             /**< (SPI_CR) Transmit FIFO Clear Position */
#define SPI_CR_TXFCLR_Msk                   (0x1U << SPI_CR_TXFCLR_Pos)                    /**< (SPI_CR) Transmit FIFO Clear Mask */
#define SPI_CR_TXFCLR                       SPI_CR_TXFCLR_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use SPI_CR_TXFCLR_Msk instead */
#define SPI_CR_RXFCLR_Pos                   17                                             /**< (SPI_CR) Receive FIFO Clear Position */
#define SPI_CR_RXFCLR_Msk                   (0x1U << SPI_CR_RXFCLR_Pos)                    /**< (SPI_CR) Receive FIFO Clear Mask */
#define SPI_CR_RXFCLR                       SPI_CR_RXFCLR_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use SPI_CR_RXFCLR_Msk instead */
#define SPI_CR_LASTXFER_Pos                 24                                             /**< (SPI_CR) Last Transfer Position */
#define SPI_CR_LASTXFER_Msk                 (0x1U << SPI_CR_LASTXFER_Pos)                  /**< (SPI_CR) Last Transfer Mask */
#define SPI_CR_LASTXFER                     SPI_CR_LASTXFER_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use SPI_CR_LASTXFER_Msk instead */
#define SPI_CR_FIFOEN_Pos                   30                                             /**< (SPI_CR) FIFO Enable Position */
#define SPI_CR_FIFOEN_Msk                   (0x1U << SPI_CR_FIFOEN_Pos)                    /**< (SPI_CR) FIFO Enable Mask */
#define SPI_CR_FIFOEN                       SPI_CR_FIFOEN_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use SPI_CR_FIFOEN_Msk instead */
#define SPI_CR_FIFODIS_Pos                  31                                             /**< (SPI_CR) FIFO Disable Position */
#define SPI_CR_FIFODIS_Msk                  (0x1U << SPI_CR_FIFODIS_Pos)                   /**< (SPI_CR) FIFO Disable Mask */
#define SPI_CR_FIFODIS                      SPI_CR_FIFODIS_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use SPI_CR_FIFODIS_Msk instead */
#define SPI_CR_MASK                         (0xC1031083U)                                  /**< \deprecated (SPI_CR) Register MASK  (Use SPI_CR_Msk instead)  */
#define SPI_CR_Msk                          (0xC1031083U)                                  /**< (SPI_CR) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- SPI_MR : (SPI Offset: 0x04) (R/W 32) Mode Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t MSTR:1;                    /**< bit:      0  Master/Slave Mode                        */
    uint32_t PS:1;                      /**< bit:      1  Peripheral Select                        */
    uint32_t PCSDEC:1;                  /**< bit:      2  Chip Select Decode                       */
    uint32_t :1;                        /**< bit:      3  Reserved */
    uint32_t MODFDIS:1;                 /**< bit:      4  Mode Fault Detection                     */
    uint32_t WDRBT:1;                   /**< bit:      5  Wait Data Read Before Transfer           */
    uint32_t :1;                        /**< bit:      6  Reserved */
    uint32_t LLB:1;                     /**< bit:      7  Local Loopback Enable                    */
    uint32_t :8;                        /**< bit:  8..15  Reserved */
    uint32_t PCS:4;                     /**< bit: 16..19  Peripheral Chip Select                   */
    uint32_t :4;                        /**< bit: 20..23  Reserved */
    uint32_t DLYBCS:8;                  /**< bit: 24..31  Delay Between Chip Selects               */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} SPI_MR_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define SPI_MR_OFFSET                       (0x04)                                        /**<  (SPI_MR) Mode Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define SPI_MR_MSTR_Pos                     0                                              /**< (SPI_MR) Master/Slave Mode Position */
#define SPI_MR_MSTR_Msk                     (0x1U << SPI_MR_MSTR_Pos)                      /**< (SPI_MR) Master/Slave Mode Mask */
#define SPI_MR_MSTR                         SPI_MR_MSTR_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use SPI_MR_MSTR_Msk instead */
#define SPI_MR_PS_Pos                       1                                              /**< (SPI_MR) Peripheral Select Position */
#define SPI_MR_PS_Msk                       (0x1U << SPI_MR_PS_Pos)                        /**< (SPI_MR) Peripheral Select Mask */
#define SPI_MR_PS                           SPI_MR_PS_Msk                                  /**< \deprecated Old style mask definition for 1 bit bitfield. Use SPI_MR_PS_Msk instead */
#define SPI_MR_PCSDEC_Pos                   2                                              /**< (SPI_MR) Chip Select Decode Position */
#define SPI_MR_PCSDEC_Msk                   (0x1U << SPI_MR_PCSDEC_Pos)                    /**< (SPI_MR) Chip Select Decode Mask */
#define SPI_MR_PCSDEC                       SPI_MR_PCSDEC_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use SPI_MR_PCSDEC_Msk instead */
#define SPI_MR_MODFDIS_Pos                  4                                              /**< (SPI_MR) Mode Fault Detection Position */
#define SPI_MR_MODFDIS_Msk                  (0x1U << SPI_MR_MODFDIS_Pos)                   /**< (SPI_MR) Mode Fault Detection Mask */
#define SPI_MR_MODFDIS                      SPI_MR_MODFDIS_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use SPI_MR_MODFDIS_Msk instead */
#define SPI_MR_WDRBT_Pos                    5                                              /**< (SPI_MR) Wait Data Read Before Transfer Position */
#define SPI_MR_WDRBT_Msk                    (0x1U << SPI_MR_WDRBT_Pos)                     /**< (SPI_MR) Wait Data Read Before Transfer Mask */
#define SPI_MR_WDRBT                        SPI_MR_WDRBT_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use SPI_MR_WDRBT_Msk instead */
#define SPI_MR_LLB_Pos                      7                                              /**< (SPI_MR) Local Loopback Enable Position */
#define SPI_MR_LLB_Msk                      (0x1U << SPI_MR_LLB_Pos)                       /**< (SPI_MR) Local Loopback Enable Mask */
#define SPI_MR_LLB                          SPI_MR_LLB_Msk                                 /**< \deprecated Old style mask definition for 1 bit bitfield. Use SPI_MR_LLB_Msk instead */
#define SPI_MR_PCS_Pos                      16                                             /**< (SPI_MR) Peripheral Chip Select Position */
#define SPI_MR_PCS_Msk                      (0xFU << SPI_MR_PCS_Pos)                       /**< (SPI_MR) Peripheral Chip Select Mask */
#define SPI_MR_PCS(value)                   (SPI_MR_PCS_Msk & ((value) << SPI_MR_PCS_Pos))
#define SPI_MR_DLYBCS_Pos                   24                                             /**< (SPI_MR) Delay Between Chip Selects Position */
#define SPI_MR_DLYBCS_Msk                   (0xFFU << SPI_MR_DLYBCS_Pos)                   /**< (SPI_MR) Delay Between Chip Selects Mask */
#define SPI_MR_DLYBCS(value)                (SPI_MR_DLYBCS_Msk & ((value) << SPI_MR_DLYBCS_Pos))
#define SPI_MR_MASK                         (0xFF0F00B7U)                                  /**< \deprecated (SPI_MR) Register MASK  (Use SPI_MR_Msk instead)  */
#define SPI_MR_Msk                          (0xFF0F00B7U)                                  /**< (SPI_MR) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- SPI_RDR : (SPI Offset: 0x08) (R/ 32) Receive Data Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t RD:16;                     /**< bit:  0..15  Receive Data                             */
    uint32_t PCS:4;                     /**< bit: 16..19  Peripheral Chip Select                   */
    uint32_t :12;                       /**< bit: 20..31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} SPI_RDR_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define SPI_RDR_OFFSET                      (0x08)                                        /**<  (SPI_RDR) Receive Data Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define SPI_RDR_RD_Pos                      0                                              /**< (SPI_RDR) Receive Data Position */
#define SPI_RDR_RD_Msk                      (0xFFFFU << SPI_RDR_RD_Pos)                    /**< (SPI_RDR) Receive Data Mask */
#define SPI_RDR_RD(value)                   (SPI_RDR_RD_Msk & ((value) << SPI_RDR_RD_Pos))
#define SPI_RDR_PCS_Pos                     16                                             /**< (SPI_RDR) Peripheral Chip Select Position */
#define SPI_RDR_PCS_Msk                     (0xFU << SPI_RDR_PCS_Pos)                      /**< (SPI_RDR) Peripheral Chip Select Mask */
#define SPI_RDR_PCS(value)                  (SPI_RDR_PCS_Msk & ((value) << SPI_RDR_PCS_Pos))
#define SPI_RDR_MASK                        (0xFFFFFU)                                     /**< \deprecated (SPI_RDR) Register MASK  (Use SPI_RDR_Msk instead)  */
#define SPI_RDR_Msk                         (0xFFFFFU)                                     /**< (SPI_RDR) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- SPI_TDR : (SPI Offset: 0x0c) (/W 32) Transmit Data Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t TD:16;                     /**< bit:  0..15  Transmit Data                            */
    uint32_t PCS:4;                     /**< bit: 16..19  Peripheral Chip Select                   */
    uint32_t :4;                        /**< bit: 20..23  Reserved */
    uint32_t LASTXFER:1;                /**< bit:     24  Last Transfer                            */
    uint32_t :7;                        /**< bit: 25..31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} SPI_TDR_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define SPI_TDR_OFFSET                      (0x0C)                                        /**<  (SPI_TDR) Transmit Data Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define SPI_TDR_TD_Pos                      0                                              /**< (SPI_TDR) Transmit Data Position */
#define SPI_TDR_TD_Msk                      (0xFFFFU << SPI_TDR_TD_Pos)                    /**< (SPI_TDR) Transmit Data Mask */
#define SPI_TDR_TD(value)                   (SPI_TDR_TD_Msk & ((value) << SPI_TDR_TD_Pos))
#define SPI_TDR_PCS_Pos                     16                                             /**< (SPI_TDR) Peripheral Chip Select Position */
#define SPI_TDR_PCS_Msk                     (0xFU << SPI_TDR_PCS_Pos)                      /**< (SPI_TDR) Peripheral Chip Select Mask */
#define SPI_TDR_PCS(value)                  (SPI_TDR_PCS_Msk & ((value) << SPI_TDR_PCS_Pos))
#define SPI_TDR_LASTXFER_Pos                24                                             /**< (SPI_TDR) Last Transfer Position */
#define SPI_TDR_LASTXFER_Msk                (0x1U << SPI_TDR_LASTXFER_Pos)                 /**< (SPI_TDR) Last Transfer Mask */
#define SPI_TDR_LASTXFER                    SPI_TDR_LASTXFER_Msk                           /**< \deprecated Old style mask definition for 1 bit bitfield. Use SPI_TDR_LASTXFER_Msk instead */
#define SPI_TDR_MASK                        (0x10FFFFFU)                                   /**< \deprecated (SPI_TDR) Register MASK  (Use SPI_TDR_Msk instead)  */
#define SPI_TDR_Msk                         (0x10FFFFFU)                                   /**< (SPI_TDR) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- SPI_SR : (SPI Offset: 0x10) (R/ 32) Status Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t RDRF:1;                    /**< bit:      0  Receive Data Register Full (cleared by reading SPI_RDR) */
    uint32_t TDRE:1;                    /**< bit:      1  Transmit Data Register Empty (cleared by writing SPI_TDR) */
    uint32_t MODF:1;                    /**< bit:      2  Mode Fault Error (cleared on read)       */
    uint32_t OVRES:1;                   /**< bit:      3  Overrun Error Status (cleared on read)   */
    uint32_t :4;                        /**< bit:   4..7  Reserved */
    uint32_t NSSR:1;                    /**< bit:      8  NSS Rising (cleared on read)             */
    uint32_t TXEMPTY:1;                 /**< bit:      9  Transmission Registers Empty (cleared by writing SPI_TDR) */
    uint32_t UNDES:1;                   /**< bit:     10  Underrun Error Status (Slave mode only) (cleared on read) */
    uint32_t :5;                        /**< bit: 11..15  Reserved */
    uint32_t SPIENS:1;                  /**< bit:     16  SPI Enable Status                        */
    uint32_t :15;                       /**< bit: 17..31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} SPI_SR_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define SPI_SR_OFFSET                       (0x10)                                        /**<  (SPI_SR) Status Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define SPI_SR_RDRF_Pos                     0                                              /**< (SPI_SR) Receive Data Register Full (cleared by reading SPI_RDR) Position */
#define SPI_SR_RDRF_Msk                     (0x1U << SPI_SR_RDRF_Pos)                      /**< (SPI_SR) Receive Data Register Full (cleared by reading SPI_RDR) Mask */
#define SPI_SR_RDRF                         SPI_SR_RDRF_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use SPI_SR_RDRF_Msk instead */
#define SPI_SR_TDRE_Pos                     1                                              /**< (SPI_SR) Transmit Data Register Empty (cleared by writing SPI_TDR) Position */
#define SPI_SR_TDRE_Msk                     (0x1U << SPI_SR_TDRE_Pos)                      /**< (SPI_SR) Transmit Data Register Empty (cleared by writing SPI_TDR) Mask */
#define SPI_SR_TDRE                         SPI_SR_TDRE_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use SPI_SR_TDRE_Msk instead */
#define SPI_SR_MODF_Pos                     2                                              /**< (SPI_SR) Mode Fault Error (cleared on read) Position */
#define SPI_SR_MODF_Msk                     (0x1U << SPI_SR_MODF_Pos)                      /**< (SPI_SR) Mode Fault Error (cleared on read) Mask */
#define SPI_SR_MODF                         SPI_SR_MODF_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use SPI_SR_MODF_Msk instead */
#define SPI_SR_OVRES_Pos                    3                                              /**< (SPI_SR) Overrun Error Status (cleared on read) Position */
#define SPI_SR_OVRES_Msk                    (0x1U << SPI_SR_OVRES_Pos)                     /**< (SPI_SR) Overrun Error Status (cleared on read) Mask */
#define SPI_SR_OVRES                        SPI_SR_OVRES_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use SPI_SR_OVRES_Msk instead */
#define SPI_SR_NSSR_Pos                     8                                              /**< (SPI_SR) NSS Rising (cleared on read) Position */
#define SPI_SR_NSSR_Msk                     (0x1U << SPI_SR_NSSR_Pos)                      /**< (SPI_SR) NSS Rising (cleared on read) Mask */
#define SPI_SR_NSSR                         SPI_SR_NSSR_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use SPI_SR_NSSR_Msk instead */
#define SPI_SR_TXEMPTY_Pos                  9                                              /**< (SPI_SR) Transmission Registers Empty (cleared by writing SPI_TDR) Position */
#define SPI_SR_TXEMPTY_Msk                  (0x1U << SPI_SR_TXEMPTY_Pos)                   /**< (SPI_SR) Transmission Registers Empty (cleared by writing SPI_TDR) Mask */
#define SPI_SR_TXEMPTY                      SPI_SR_TXEMPTY_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use SPI_SR_TXEMPTY_Msk instead */
#define SPI_SR_UNDES_Pos                    10                                             /**< (SPI_SR) Underrun Error Status (Slave mode only) (cleared on read) Position */
#define SPI_SR_UNDES_Msk                    (0x1U << SPI_SR_UNDES_Pos)                     /**< (SPI_SR) Underrun Error Status (Slave mode only) (cleared on read) Mask */
#define SPI_SR_UNDES                        SPI_SR_UNDES_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use SPI_SR_UNDES_Msk instead */
#define SPI_SR_SPIENS_Pos                   16                                             /**< (SPI_SR) SPI Enable Status Position */
#define SPI_SR_SPIENS_Msk                   (0x1U << SPI_SR_SPIENS_Pos)                    /**< (SPI_SR) SPI Enable Status Mask */
#define SPI_SR_SPIENS                       SPI_SR_SPIENS_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use SPI_SR_SPIENS_Msk instead */
#define SPI_SR_MASK                         (0x1070FU)                                     /**< \deprecated (SPI_SR) Register MASK  (Use SPI_SR_Msk instead)  */
#define SPI_SR_Msk                          (0x1070FU)                                     /**< (SPI_SR) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- SPI_IER : (SPI Offset: 0x14) (/W 32) Interrupt Enable Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t RDRF:1;                    /**< bit:      0  Receive Data Register Full Interrupt Enable */
    uint32_t TDRE:1;                    /**< bit:      1  SPI Transmit Data Register Empty Interrupt Enable */
    uint32_t MODF:1;                    /**< bit:      2  Mode Fault Error Interrupt Enable        */
    uint32_t OVRES:1;                   /**< bit:      3  Overrun Error Interrupt Enable           */
    uint32_t :4;                        /**< bit:   4..7  Reserved */
    uint32_t NSSR:1;                    /**< bit:      8  NSS Rising Interrupt Enable              */
    uint32_t TXEMPTY:1;                 /**< bit:      9  Transmission Registers Empty Enable      */
    uint32_t UNDES:1;                   /**< bit:     10  Underrun Error Interrupt Enable          */
    uint32_t :21;                       /**< bit: 11..31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} SPI_IER_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define SPI_IER_OFFSET                      (0x14)                                        /**<  (SPI_IER) Interrupt Enable Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define SPI_IER_RDRF_Pos                    0                                              /**< (SPI_IER) Receive Data Register Full Interrupt Enable Position */
#define SPI_IER_RDRF_Msk                    (0x1U << SPI_IER_RDRF_Pos)                     /**< (SPI_IER) Receive Data Register Full Interrupt Enable Mask */
#define SPI_IER_RDRF                        SPI_IER_RDRF_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use SPI_IER_RDRF_Msk instead */
#define SPI_IER_TDRE_Pos                    1                                              /**< (SPI_IER) SPI Transmit Data Register Empty Interrupt Enable Position */
#define SPI_IER_TDRE_Msk                    (0x1U << SPI_IER_TDRE_Pos)                     /**< (SPI_IER) SPI Transmit Data Register Empty Interrupt Enable Mask */
#define SPI_IER_TDRE                        SPI_IER_TDRE_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use SPI_IER_TDRE_Msk instead */
#define SPI_IER_MODF_Pos                    2                                              /**< (SPI_IER) Mode Fault Error Interrupt Enable Position */
#define SPI_IER_MODF_Msk                    (0x1U << SPI_IER_MODF_Pos)                     /**< (SPI_IER) Mode Fault Error Interrupt Enable Mask */
#define SPI_IER_MODF                        SPI_IER_MODF_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use SPI_IER_MODF_Msk instead */
#define SPI_IER_OVRES_Pos                   3                                              /**< (SPI_IER) Overrun Error Interrupt Enable Position */
#define SPI_IER_OVRES_Msk                   (0x1U << SPI_IER_OVRES_Pos)                    /**< (SPI_IER) Overrun Error Interrupt Enable Mask */
#define SPI_IER_OVRES                       SPI_IER_OVRES_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use SPI_IER_OVRES_Msk instead */
#define SPI_IER_NSSR_Pos                    8                                              /**< (SPI_IER) NSS Rising Interrupt Enable Position */
#define SPI_IER_NSSR_Msk                    (0x1U << SPI_IER_NSSR_Pos)                     /**< (SPI_IER) NSS Rising Interrupt Enable Mask */
#define SPI_IER_NSSR                        SPI_IER_NSSR_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use SPI_IER_NSSR_Msk instead */
#define SPI_IER_TXEMPTY_Pos                 9                                              /**< (SPI_IER) Transmission Registers Empty Enable Position */
#define SPI_IER_TXEMPTY_Msk                 (0x1U << SPI_IER_TXEMPTY_Pos)                  /**< (SPI_IER) Transmission Registers Empty Enable Mask */
#define SPI_IER_TXEMPTY                     SPI_IER_TXEMPTY_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use SPI_IER_TXEMPTY_Msk instead */
#define SPI_IER_UNDES_Pos                   10                                             /**< (SPI_IER) Underrun Error Interrupt Enable Position */
#define SPI_IER_UNDES_Msk                   (0x1U << SPI_IER_UNDES_Pos)                    /**< (SPI_IER) Underrun Error Interrupt Enable Mask */
#define SPI_IER_UNDES                       SPI_IER_UNDES_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use SPI_IER_UNDES_Msk instead */
#define SPI_IER_MASK                        (0x70FU)                                       /**< \deprecated (SPI_IER) Register MASK  (Use SPI_IER_Msk instead)  */
#define SPI_IER_Msk                         (0x70FU)                                       /**< (SPI_IER) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- SPI_IDR : (SPI Offset: 0x18) (/W 32) Interrupt Disable Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t RDRF:1;                    /**< bit:      0  Receive Data Register Full Interrupt Disable */
    uint32_t TDRE:1;                    /**< bit:      1  SPI Transmit Data Register Empty Interrupt Disable */
    uint32_t MODF:1;                    /**< bit:      2  Mode Fault Error Interrupt Disable       */
    uint32_t OVRES:1;                   /**< bit:      3  Overrun Error Interrupt Disable          */
    uint32_t :4;                        /**< bit:   4..7  Reserved */
    uint32_t NSSR:1;                    /**< bit:      8  NSS Rising Interrupt Disable             */
    uint32_t TXEMPTY:1;                 /**< bit:      9  Transmission Registers Empty Disable     */
    uint32_t UNDES:1;                   /**< bit:     10  Underrun Error Interrupt Disable         */
    uint32_t :21;                       /**< bit: 11..31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} SPI_IDR_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define SPI_IDR_OFFSET                      (0x18)                                        /**<  (SPI_IDR) Interrupt Disable Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define SPI_IDR_RDRF_Pos                    0                                              /**< (SPI_IDR) Receive Data Register Full Interrupt Disable Position */
#define SPI_IDR_RDRF_Msk                    (0x1U << SPI_IDR_RDRF_Pos)                     /**< (SPI_IDR) Receive Data Register Full Interrupt Disable Mask */
#define SPI_IDR_RDRF                        SPI_IDR_RDRF_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use SPI_IDR_RDRF_Msk instead */
#define SPI_IDR_TDRE_Pos                    1                                              /**< (SPI_IDR) SPI Transmit Data Register Empty Interrupt Disable Position */
#define SPI_IDR_TDRE_Msk                    (0x1U << SPI_IDR_TDRE_Pos)                     /**< (SPI_IDR) SPI Transmit Data Register Empty Interrupt Disable Mask */
#define SPI_IDR_TDRE                        SPI_IDR_TDRE_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use SPI_IDR_TDRE_Msk instead */
#define SPI_IDR_MODF_Pos                    2                                              /**< (SPI_IDR) Mode Fault Error Interrupt Disable Position */
#define SPI_IDR_MODF_Msk                    (0x1U << SPI_IDR_MODF_Pos)                     /**< (SPI_IDR) Mode Fault Error Interrupt Disable Mask */
#define SPI_IDR_MODF                        SPI_IDR_MODF_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use SPI_IDR_MODF_Msk instead */
#define SPI_IDR_OVRES_Pos                   3                                              /**< (SPI_IDR) Overrun Error Interrupt Disable Position */
#define SPI_IDR_OVRES_Msk                   (0x1U << SPI_IDR_OVRES_Pos)                    /**< (SPI_IDR) Overrun Error Interrupt Disable Mask */
#define SPI_IDR_OVRES                       SPI_IDR_OVRES_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use SPI_IDR_OVRES_Msk instead */
#define SPI_IDR_NSSR_Pos                    8                                              /**< (SPI_IDR) NSS Rising Interrupt Disable Position */
#define SPI_IDR_NSSR_Msk                    (0x1U << SPI_IDR_NSSR_Pos)                     /**< (SPI_IDR) NSS Rising Interrupt Disable Mask */
#define SPI_IDR_NSSR                        SPI_IDR_NSSR_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use SPI_IDR_NSSR_Msk instead */
#define SPI_IDR_TXEMPTY_Pos                 9                                              /**< (SPI_IDR) Transmission Registers Empty Disable Position */
#define SPI_IDR_TXEMPTY_Msk                 (0x1U << SPI_IDR_TXEMPTY_Pos)                  /**< (SPI_IDR) Transmission Registers Empty Disable Mask */
#define SPI_IDR_TXEMPTY                     SPI_IDR_TXEMPTY_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use SPI_IDR_TXEMPTY_Msk instead */
#define SPI_IDR_UNDES_Pos                   10                                             /**< (SPI_IDR) Underrun Error Interrupt Disable Position */
#define SPI_IDR_UNDES_Msk                   (0x1U << SPI_IDR_UNDES_Pos)                    /**< (SPI_IDR) Underrun Error Interrupt Disable Mask */
#define SPI_IDR_UNDES                       SPI_IDR_UNDES_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use SPI_IDR_UNDES_Msk instead */
#define SPI_IDR_MASK                        (0x70FU)                                       /**< \deprecated (SPI_IDR) Register MASK  (Use SPI_IDR_Msk instead)  */
#define SPI_IDR_Msk                         (0x70FU)                                       /**< (SPI_IDR) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- SPI_IMR : (SPI Offset: 0x1c) (R/ 32) Interrupt Mask Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t RDRF:1;                    /**< bit:      0  Receive Data Register Full Interrupt Mask */
    uint32_t TDRE:1;                    /**< bit:      1  SPI Transmit Data Register Empty Interrupt Mask */
    uint32_t MODF:1;                    /**< bit:      2  Mode Fault Error Interrupt Mask          */
    uint32_t OVRES:1;                   /**< bit:      3  Overrun Error Interrupt Mask             */
    uint32_t :4;                        /**< bit:   4..7  Reserved */
    uint32_t NSSR:1;                    /**< bit:      8  NSS Rising Interrupt Mask                */
    uint32_t TXEMPTY:1;                 /**< bit:      9  Transmission Registers Empty Mask        */
    uint32_t UNDES:1;                   /**< bit:     10  Underrun Error Interrupt Mask            */
    uint32_t :21;                       /**< bit: 11..31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} SPI_IMR_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define SPI_IMR_OFFSET                      (0x1C)                                        /**<  (SPI_IMR) Interrupt Mask Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define SPI_IMR_RDRF_Pos                    0                                              /**< (SPI_IMR) Receive Data Register Full Interrupt Mask Position */
#define SPI_IMR_RDRF_Msk                    (0x1U << SPI_IMR_RDRF_Pos)                     /**< (SPI_IMR) Receive Data Register Full Interrupt Mask Mask */
#define SPI_IMR_RDRF                        SPI_IMR_RDRF_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use SPI_IMR_RDRF_Msk instead */
#define SPI_IMR_TDRE_Pos                    1                                              /**< (SPI_IMR) SPI Transmit Data Register Empty Interrupt Mask Position */
#define SPI_IMR_TDRE_Msk                    (0x1U << SPI_IMR_TDRE_Pos)                     /**< (SPI_IMR) SPI Transmit Data Register Empty Interrupt Mask Mask */
#define SPI_IMR_TDRE                        SPI_IMR_TDRE_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use SPI_IMR_TDRE_Msk instead */
#define SPI_IMR_MODF_Pos                    2                                              /**< (SPI_IMR) Mode Fault Error Interrupt Mask Position */
#define SPI_IMR_MODF_Msk                    (0x1U << SPI_IMR_MODF_Pos)                     /**< (SPI_IMR) Mode Fault Error Interrupt Mask Mask */
#define SPI_IMR_MODF                        SPI_IMR_MODF_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use SPI_IMR_MODF_Msk instead */
#define SPI_IMR_OVRES_Pos                   3                                              /**< (SPI_IMR) Overrun Error Interrupt Mask Position */
#define SPI_IMR_OVRES_Msk                   (0x1U << SPI_IMR_OVRES_Pos)                    /**< (SPI_IMR) Overrun Error Interrupt Mask Mask */
#define SPI_IMR_OVRES                       SPI_IMR_OVRES_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use SPI_IMR_OVRES_Msk instead */
#define SPI_IMR_NSSR_Pos                    8                                              /**< (SPI_IMR) NSS Rising Interrupt Mask Position */
#define SPI_IMR_NSSR_Msk                    (0x1U << SPI_IMR_NSSR_Pos)                     /**< (SPI_IMR) NSS Rising Interrupt Mask Mask */
#define SPI_IMR_NSSR                        SPI_IMR_NSSR_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use SPI_IMR_NSSR_Msk instead */
#define SPI_IMR_TXEMPTY_Pos                 9                                              /**< (SPI_IMR) Transmission Registers Empty Mask Position */
#define SPI_IMR_TXEMPTY_Msk                 (0x1U << SPI_IMR_TXEMPTY_Pos)                  /**< (SPI_IMR) Transmission Registers Empty Mask Mask */
#define SPI_IMR_TXEMPTY                     SPI_IMR_TXEMPTY_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use SPI_IMR_TXEMPTY_Msk instead */
#define SPI_IMR_UNDES_Pos                   10                                             /**< (SPI_IMR) Underrun Error Interrupt Mask Position */
#define SPI_IMR_UNDES_Msk                   (0x1U << SPI_IMR_UNDES_Pos)                    /**< (SPI_IMR) Underrun Error Interrupt Mask Mask */
#define SPI_IMR_UNDES                       SPI_IMR_UNDES_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use SPI_IMR_UNDES_Msk instead */
#define SPI_IMR_MASK                        (0x70FU)                                       /**< \deprecated (SPI_IMR) Register MASK  (Use SPI_IMR_Msk instead)  */
#define SPI_IMR_Msk                         (0x70FU)                                       /**< (SPI_IMR) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- SPI_CSR : (SPI Offset: 0x30) (R/W 32) Chip Select Register 0 -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t CPOL:1;                    /**< bit:      0  Clock Polarity                           */
    uint32_t NCPHA:1;                   /**< bit:      1  Clock Phase                              */
    uint32_t CSNAAT:1;                  /**< bit:      2  Chip Select Not Active After Transfer (Ignored if CSAAT = 1) */
    uint32_t CSAAT:1;                   /**< bit:      3  Chip Select Active After Transfer        */
    uint32_t BITS:4;                    /**< bit:   4..7  Bits Per Transfer                        */
    uint32_t SCBR:8;                    /**< bit:  8..15  Serial Clock Bit Rate                    */
    uint32_t DLYBS:8;                   /**< bit: 16..23  Delay Before SPCK                        */
    uint32_t DLYBCT:8;                  /**< bit: 24..31  Delay Between Consecutive Transfers      */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} SPI_CSR_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define SPI_CSR_OFFSET                      (0x30)                                        /**<  (SPI_CSR) Chip Select Register 0  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define SPI_CSR_CPOL_Pos                    0                                              /**< (SPI_CSR) Clock Polarity Position */
#define SPI_CSR_CPOL_Msk                    (0x1U << SPI_CSR_CPOL_Pos)                     /**< (SPI_CSR) Clock Polarity Mask */
#define SPI_CSR_CPOL                        SPI_CSR_CPOL_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use SPI_CSR_CPOL_Msk instead */
#define SPI_CSR_NCPHA_Pos                   1                                              /**< (SPI_CSR) Clock Phase Position */
#define SPI_CSR_NCPHA_Msk                   (0x1U << SPI_CSR_NCPHA_Pos)                    /**< (SPI_CSR) Clock Phase Mask */
#define SPI_CSR_NCPHA                       SPI_CSR_NCPHA_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use SPI_CSR_NCPHA_Msk instead */
#define SPI_CSR_CSNAAT_Pos                  2                                              /**< (SPI_CSR) Chip Select Not Active After Transfer (Ignored if CSAAT = 1) Position */
#define SPI_CSR_CSNAAT_Msk                  (0x1U << SPI_CSR_CSNAAT_Pos)                   /**< (SPI_CSR) Chip Select Not Active After Transfer (Ignored if CSAAT = 1) Mask */
#define SPI_CSR_CSNAAT                      SPI_CSR_CSNAAT_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use SPI_CSR_CSNAAT_Msk instead */
#define SPI_CSR_CSAAT_Pos                   3                                              /**< (SPI_CSR) Chip Select Active After Transfer Position */
#define SPI_CSR_CSAAT_Msk                   (0x1U << SPI_CSR_CSAAT_Pos)                    /**< (SPI_CSR) Chip Select Active After Transfer Mask */
#define SPI_CSR_CSAAT                       SPI_CSR_CSAAT_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use SPI_CSR_CSAAT_Msk instead */
#define SPI_CSR_BITS_Pos                    4                                              /**< (SPI_CSR) Bits Per Transfer Position */
#define SPI_CSR_BITS_Msk                    (0xFU << SPI_CSR_BITS_Pos)                     /**< (SPI_CSR) Bits Per Transfer Mask */
#define SPI_CSR_BITS(value)                 (SPI_CSR_BITS_Msk & ((value) << SPI_CSR_BITS_Pos))
#define   SPI_CSR_BITS_8_BIT_Val            (0x0U)                                         /**< (SPI_CSR) 8 bits for transfer  */
#define   SPI_CSR_BITS_9_BIT_Val            (0x1U)                                         /**< (SPI_CSR) 9 bits for transfer  */
#define   SPI_CSR_BITS_10_BIT_Val           (0x2U)                                         /**< (SPI_CSR) 10 bits for transfer  */
#define   SPI_CSR_BITS_11_BIT_Val           (0x3U)                                         /**< (SPI_CSR) 11 bits for transfer  */
#define   SPI_CSR_BITS_12_BIT_Val           (0x4U)                                         /**< (SPI_CSR) 12 bits for transfer  */
#define   SPI_CSR_BITS_13_BIT_Val           (0x5U)                                         /**< (SPI_CSR) 13 bits for transfer  */
#define   SPI_CSR_BITS_14_BIT_Val           (0x6U)                                         /**< (SPI_CSR) 14 bits for transfer  */
#define   SPI_CSR_BITS_15_BIT_Val           (0x7U)                                         /**< (SPI_CSR) 15 bits for transfer  */
#define   SPI_CSR_BITS_16_BIT_Val           (0x8U)                                         /**< (SPI_CSR) 16 bits for transfer  */
#define SPI_CSR_BITS_8_BIT                  (SPI_CSR_BITS_8_BIT_Val << SPI_CSR_BITS_Pos)   /**< (SPI_CSR) 8 bits for transfer Position  */
#define SPI_CSR_BITS_9_BIT                  (SPI_CSR_BITS_9_BIT_Val << SPI_CSR_BITS_Pos)   /**< (SPI_CSR) 9 bits for transfer Position  */
#define SPI_CSR_BITS_10_BIT                 (SPI_CSR_BITS_10_BIT_Val << SPI_CSR_BITS_Pos)  /**< (SPI_CSR) 10 bits for transfer Position  */
#define SPI_CSR_BITS_11_BIT                 (SPI_CSR_BITS_11_BIT_Val << SPI_CSR_BITS_Pos)  /**< (SPI_CSR) 11 bits for transfer Position  */
#define SPI_CSR_BITS_12_BIT                 (SPI_CSR_BITS_12_BIT_Val << SPI_CSR_BITS_Pos)  /**< (SPI_CSR) 12 bits for transfer Position  */
#define SPI_CSR_BITS_13_BIT                 (SPI_CSR_BITS_13_BIT_Val << SPI_CSR_BITS_Pos)  /**< (SPI_CSR) 13 bits for transfer Position  */
#define SPI_CSR_BITS_14_BIT                 (SPI_CSR_BITS_14_BIT_Val << SPI_CSR_BITS_Pos)  /**< (SPI_CSR) 14 bits for transfer Position  */
#define SPI_CSR_BITS_15_BIT                 (SPI_CSR_BITS_15_BIT_Val << SPI_CSR_BITS_Pos)  /**< (SPI_CSR) 15 bits for transfer Position  */
#define SPI_CSR_BITS_16_BIT                 (SPI_CSR_BITS_16_BIT_Val << SPI_CSR_BITS_Pos)  /**< (SPI_CSR) 16 bits for transfer Position  */
#define SPI_CSR_SCBR_Pos                    8                                              /**< (SPI_CSR) Serial Clock Bit Rate Position */
#define SPI_CSR_SCBR_Msk                    (0xFFU << SPI_CSR_SCBR_Pos)                    /**< (SPI_CSR) Serial Clock Bit Rate Mask */
#define SPI_CSR_SCBR(value)                 (SPI_CSR_SCBR_Msk & ((value) << SPI_CSR_SCBR_Pos))
#define SPI_CSR_DLYBS_Pos                   16                                             /**< (SPI_CSR) Delay Before SPCK Position */
#define SPI_CSR_DLYBS_Msk                   (0xFFU << SPI_CSR_DLYBS_Pos)                   /**< (SPI_CSR) Delay Before SPCK Mask */
#define SPI_CSR_DLYBS(value)                (SPI_CSR_DLYBS_Msk & ((value) << SPI_CSR_DLYBS_Pos))
#define SPI_CSR_DLYBCT_Pos                  24                                             /**< (SPI_CSR) Delay Between Consecutive Transfers Position */
#define SPI_CSR_DLYBCT_Msk                  (0xFFU << SPI_CSR_DLYBCT_Pos)                  /**< (SPI_CSR) Delay Between Consecutive Transfers Mask */
#define SPI_CSR_DLYBCT(value)               (SPI_CSR_DLYBCT_Msk & ((value) << SPI_CSR_DLYBCT_Pos))
#define SPI_CSR_MASK                        (0xFFFFFFFFU)                                  /**< \deprecated (SPI_CSR) Register MASK  (Use SPI_CSR_Msk instead)  */
#define SPI_CSR_Msk                         (0xFFFFFFFFU)                                  /**< (SPI_CSR) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- SPI_WPMR : (SPI Offset: 0xe4) (R/W 32) Write Protection Mode Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t WPEN:1;                    /**< bit:      0  Write Protection Enable                  */
    uint32_t :7;                        /**< bit:   1..7  Reserved */
    uint32_t WPKEY:24;                  /**< bit:  8..31  Write Protection Key                     */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} SPI_WPMR_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define SPI_WPMR_OFFSET                     (0xE4)                                        /**<  (SPI_WPMR) Write Protection Mode Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define SPI_WPMR_WPEN_Pos                   0                                              /**< (SPI_WPMR) Write Protection Enable Position */
#define SPI_WPMR_WPEN_Msk                   (0x1U << SPI_WPMR_WPEN_Pos)                    /**< (SPI_WPMR) Write Protection Enable Mask */
#define SPI_WPMR_WPEN                       SPI_WPMR_WPEN_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use SPI_WPMR_WPEN_Msk instead */
#define SPI_WPMR_WPKEY_Pos                  8                                              /**< (SPI_WPMR) Write Protection Key Position */
#define SPI_WPMR_WPKEY_Msk                  (0xFFFFFFU << SPI_WPMR_WPKEY_Pos)              /**< (SPI_WPMR) Write Protection Key Mask */
#define SPI_WPMR_WPKEY(value)               (SPI_WPMR_WPKEY_Msk & ((value) << SPI_WPMR_WPKEY_Pos))
#define   SPI_WPMR_WPKEY_PASSWD_Val         (0x535049U)                                    /**< (SPI_WPMR) Writing any other value in this field aborts the write operation of the WPEN bit.Always reads as 0.  */
#define SPI_WPMR_WPKEY_PASSWD               (SPI_WPMR_WPKEY_PASSWD_Val << SPI_WPMR_WPKEY_Pos)  /**< (SPI_WPMR) Writing any other value in this field aborts the write operation of the WPEN bit.Always reads as 0. Position  */
#define SPI_WPMR_MASK                       (0xFFFFFF01U)                                  /**< \deprecated (SPI_WPMR) Register MASK  (Use SPI_WPMR_Msk instead)  */
#define SPI_WPMR_Msk                        (0xFFFFFF01U)                                  /**< (SPI_WPMR) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- SPI_WPSR : (SPI Offset: 0xe8) (R/ 32) Write Protection Status Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t WPVS:1;                    /**< bit:      0  Write Protection Violation Status        */
    uint32_t :7;                        /**< bit:   1..7  Reserved */
    uint32_t WPVSRC:8;                  /**< bit:  8..15  Write Protection Violation Source        */
    uint32_t :16;                       /**< bit: 16..31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} SPI_WPSR_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define SPI_WPSR_OFFSET                     (0xE8)                                        /**<  (SPI_WPSR) Write Protection Status Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define SPI_WPSR_WPVS_Pos                   0                                              /**< (SPI_WPSR) Write Protection Violation Status Position */
#define SPI_WPSR_WPVS_Msk                   (0x1U << SPI_WPSR_WPVS_Pos)                    /**< (SPI_WPSR) Write Protection Violation Status Mask */
#define SPI_WPSR_WPVS                       SPI_WPSR_WPVS_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use SPI_WPSR_WPVS_Msk instead */
#define SPI_WPSR_WPVSRC_Pos                 8                                              /**< (SPI_WPSR) Write Protection Violation Source Position */
#define SPI_WPSR_WPVSRC_Msk                 (0xFFU << SPI_WPSR_WPVSRC_Pos)                 /**< (SPI_WPSR) Write Protection Violation Source Mask */
#define SPI_WPSR_WPVSRC(value)              (SPI_WPSR_WPVSRC_Msk & ((value) << SPI_WPSR_WPVSRC_Pos))
#define SPI_WPSR_MASK                       (0xFF01U)                                      /**< \deprecated (SPI_WPSR) Register MASK  (Use SPI_WPSR_Msk instead)  */
#define SPI_WPSR_Msk                        (0xFF01U)                                      /**< (SPI_WPSR) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
#if COMPONENT_TYPEDEF_STYLE == 'R'
/** \brief SPI hardware registers */
typedef struct {  
  __O  uint32_t SPI_CR;         /**< (SPI Offset: 0x00) Control Register */
  __IO uint32_t SPI_MR;         /**< (SPI Offset: 0x04) Mode Register */
  __I  uint32_t SPI_RDR;        /**< (SPI Offset: 0x08) Receive Data Register */
  __O  uint32_t SPI_TDR;        /**< (SPI Offset: 0x0C) Transmit Data Register */
  __I  uint32_t SPI_SR;         /**< (SPI Offset: 0x10) Status Register */
  __O  uint32_t SPI_IER;        /**< (SPI Offset: 0x14) Interrupt Enable Register */
  __O  uint32_t SPI_IDR;        /**< (SPI Offset: 0x18) Interrupt Disable Register */
  __I  uint32_t SPI_IMR;        /**< (SPI Offset: 0x1C) Interrupt Mask Register */
  __I  uint32_t Reserved1[4];
  __IO uint32_t SPI_CSR[4];     /**< (SPI Offset: 0x30) Chip Select Register 0 */
  __I  uint32_t Reserved2[41];
  __IO uint32_t SPI_WPMR;       /**< (SPI Offset: 0xE4) Write Protection Mode Register */
  __I  uint32_t SPI_WPSR;       /**< (SPI Offset: 0xE8) Write Protection Status Register */
} Spi;

#elif COMPONENT_TYPEDEF_STYLE == 'N'
/** \brief SPI hardware registers */
typedef struct {  
  __O  SPI_CR_Type                    SPI_CR;         /**< Offset: 0x00 ( /W  32) Control Register */
  __IO SPI_MR_Type                    SPI_MR;         /**< Offset: 0x04 (R/W  32) Mode Register */
  __I  SPI_RDR_Type                   SPI_RDR;        /**< Offset: 0x08 (R/   32) Receive Data Register */
  __O  SPI_TDR_Type                   SPI_TDR;        /**< Offset: 0x0C ( /W  32) Transmit Data Register */
  __I  SPI_SR_Type                    SPI_SR;         /**< Offset: 0x10 (R/   32) Status Register */
  __O  SPI_IER_Type                   SPI_IER;        /**< Offset: 0x14 ( /W  32) Interrupt Enable Register */
  __O  SPI_IDR_Type                   SPI_IDR;        /**< Offset: 0x18 ( /W  32) Interrupt Disable Register */
  __I  SPI_IMR_Type                   SPI_IMR;        /**< Offset: 0x1C (R/   32) Interrupt Mask Register */
       RoReg8                         Reserved1[0x10];
  __IO SPI_CSR_Type                   SPI_CSR[4];     /**< Offset: 0x30 (R/W  32) Chip Select Register 0 */
       RoReg8                         Reserved2[0xA4];
  __IO SPI_WPMR_Type                  SPI_WPMR;       /**< Offset: 0xE4 (R/W  32) Write Protection Mode Register */
  __I  SPI_WPSR_Type                  SPI_WPSR;       /**< Offset: 0xE8 (R/   32) Write Protection Status Register */
} Spi;

#else /* COMPONENT_TYPEDEF_STYLE */
#error Unknown component typedef style
#endif /* COMPONENT_TYPEDEF_STYLE */

#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/** @}  end of Serial Peripheral Interface */

#endif /* _SAME70_SPI_COMPONENT_H_ */
