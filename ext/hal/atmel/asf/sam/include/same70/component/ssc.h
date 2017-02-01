/**
 * \file
 *
 * \brief Component description for SSC
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

#ifndef _SAME70_SSC_COMPONENT_H_
#define _SAME70_SSC_COMPONENT_H_
#define _SAME70_SSC_COMPONENT_         /**< \deprecated  Backward compatibility for ASF */

/** \addtogroup SAME70_SSC Synchronous Serial Controller
 *  @{
 */
/* ========================================================================== */
/**  SOFTWARE API DEFINITION FOR SSC */
/* ========================================================================== */
#ifndef COMPONENT_TYPEDEF_STYLE
  #define COMPONENT_TYPEDEF_STYLE 'R'  /**< Defines default style of typedefs for the component header files ('R' = RFO, 'N' = NTO)*/
#endif

#define SSC_6078                       /**< (SSC) Module ID */
#define REV_SSC O                      /**< (SSC) Module revision */

/* -------- SSC_CR : (SSC Offset: 0x00) (/W 32) Control Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t RXEN:1;                    /**< bit:      0  Receive Enable                           */
    uint32_t RXDIS:1;                   /**< bit:      1  Receive Disable                          */
    uint32_t :6;                        /**< bit:   2..7  Reserved */
    uint32_t TXEN:1;                    /**< bit:      8  Transmit Enable                          */
    uint32_t TXDIS:1;                   /**< bit:      9  Transmit Disable                         */
    uint32_t :5;                        /**< bit: 10..14  Reserved */
    uint32_t SWRST:1;                   /**< bit:     15  Software Reset                           */
    uint32_t :16;                       /**< bit: 16..31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} SSC_CR_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define SSC_CR_OFFSET                       (0x00)                                        /**<  (SSC_CR) Control Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define SSC_CR_RXEN_Pos                     0                                              /**< (SSC_CR) Receive Enable Position */
#define SSC_CR_RXEN_Msk                     (0x1U << SSC_CR_RXEN_Pos)                      /**< (SSC_CR) Receive Enable Mask */
#define SSC_CR_RXEN                         SSC_CR_RXEN_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use SSC_CR_RXEN_Msk instead */
#define SSC_CR_RXDIS_Pos                    1                                              /**< (SSC_CR) Receive Disable Position */
#define SSC_CR_RXDIS_Msk                    (0x1U << SSC_CR_RXDIS_Pos)                     /**< (SSC_CR) Receive Disable Mask */
#define SSC_CR_RXDIS                        SSC_CR_RXDIS_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use SSC_CR_RXDIS_Msk instead */
#define SSC_CR_TXEN_Pos                     8                                              /**< (SSC_CR) Transmit Enable Position */
#define SSC_CR_TXEN_Msk                     (0x1U << SSC_CR_TXEN_Pos)                      /**< (SSC_CR) Transmit Enable Mask */
#define SSC_CR_TXEN                         SSC_CR_TXEN_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use SSC_CR_TXEN_Msk instead */
#define SSC_CR_TXDIS_Pos                    9                                              /**< (SSC_CR) Transmit Disable Position */
#define SSC_CR_TXDIS_Msk                    (0x1U << SSC_CR_TXDIS_Pos)                     /**< (SSC_CR) Transmit Disable Mask */
#define SSC_CR_TXDIS                        SSC_CR_TXDIS_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use SSC_CR_TXDIS_Msk instead */
#define SSC_CR_SWRST_Pos                    15                                             /**< (SSC_CR) Software Reset Position */
#define SSC_CR_SWRST_Msk                    (0x1U << SSC_CR_SWRST_Pos)                     /**< (SSC_CR) Software Reset Mask */
#define SSC_CR_SWRST                        SSC_CR_SWRST_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use SSC_CR_SWRST_Msk instead */
#define SSC_CR_MASK                         (0x8303U)                                      /**< \deprecated (SSC_CR) Register MASK  (Use SSC_CR_Msk instead)  */
#define SSC_CR_Msk                          (0x8303U)                                      /**< (SSC_CR) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- SSC_CMR : (SSC Offset: 0x04) (R/W 32) Clock Mode Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t DIV:12;                    /**< bit:  0..11  Clock Divider                            */
    uint32_t :20;                       /**< bit: 12..31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} SSC_CMR_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define SSC_CMR_OFFSET                      (0x04)                                        /**<  (SSC_CMR) Clock Mode Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define SSC_CMR_DIV_Pos                     0                                              /**< (SSC_CMR) Clock Divider Position */
#define SSC_CMR_DIV_Msk                     (0xFFFU << SSC_CMR_DIV_Pos)                    /**< (SSC_CMR) Clock Divider Mask */
#define SSC_CMR_DIV(value)                  (SSC_CMR_DIV_Msk & ((value) << SSC_CMR_DIV_Pos))
#define SSC_CMR_MASK                        (0xFFFU)                                       /**< \deprecated (SSC_CMR) Register MASK  (Use SSC_CMR_Msk instead)  */
#define SSC_CMR_Msk                         (0xFFFU)                                       /**< (SSC_CMR) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- SSC_RCMR : (SSC Offset: 0x10) (R/W 32) Receive Clock Mode Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t CKS:2;                     /**< bit:   0..1  Receive Clock Selection                  */
    uint32_t CKO:3;                     /**< bit:   2..4  Receive Clock Output Mode Selection      */
    uint32_t CKI:1;                     /**< bit:      5  Receive Clock Inversion                  */
    uint32_t CKG:2;                     /**< bit:   6..7  Receive Clock Gating Selection           */
    uint32_t START:4;                   /**< bit:  8..11  Receive Start Selection                  */
    uint32_t STOP:1;                    /**< bit:     12  Receive Stop Selection                   */
    uint32_t :3;                        /**< bit: 13..15  Reserved */
    uint32_t STTDLY:8;                  /**< bit: 16..23  Receive Start Delay                      */
    uint32_t PERIOD:8;                  /**< bit: 24..31  Receive Period Divider Selection         */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} SSC_RCMR_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define SSC_RCMR_OFFSET                     (0x10)                                        /**<  (SSC_RCMR) Receive Clock Mode Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define SSC_RCMR_CKS_Pos                    0                                              /**< (SSC_RCMR) Receive Clock Selection Position */
#define SSC_RCMR_CKS_Msk                    (0x3U << SSC_RCMR_CKS_Pos)                     /**< (SSC_RCMR) Receive Clock Selection Mask */
#define SSC_RCMR_CKS(value)                 (SSC_RCMR_CKS_Msk & ((value) << SSC_RCMR_CKS_Pos))
#define   SSC_RCMR_CKS_MCK_Val              (0x0U)                                         /**< (SSC_RCMR) Divided Clock  */
#define   SSC_RCMR_CKS_TK_Val               (0x1U)                                         /**< (SSC_RCMR) TK Clock signal  */
#define   SSC_RCMR_CKS_RK_Val               (0x2U)                                         /**< (SSC_RCMR) RK pin  */
#define SSC_RCMR_CKS_MCK                    (SSC_RCMR_CKS_MCK_Val << SSC_RCMR_CKS_Pos)     /**< (SSC_RCMR) Divided Clock Position  */
#define SSC_RCMR_CKS_TK                     (SSC_RCMR_CKS_TK_Val << SSC_RCMR_CKS_Pos)      /**< (SSC_RCMR) TK Clock signal Position  */
#define SSC_RCMR_CKS_RK                     (SSC_RCMR_CKS_RK_Val << SSC_RCMR_CKS_Pos)      /**< (SSC_RCMR) RK pin Position  */
#define SSC_RCMR_CKO_Pos                    2                                              /**< (SSC_RCMR) Receive Clock Output Mode Selection Position */
#define SSC_RCMR_CKO_Msk                    (0x7U << SSC_RCMR_CKO_Pos)                     /**< (SSC_RCMR) Receive Clock Output Mode Selection Mask */
#define SSC_RCMR_CKO(value)                 (SSC_RCMR_CKO_Msk & ((value) << SSC_RCMR_CKO_Pos))
#define   SSC_RCMR_CKO_NONE_Val             (0x0U)                                         /**< (SSC_RCMR) None, RK pin is an input  */
#define   SSC_RCMR_CKO_CONTINUOUS_Val       (0x1U)                                         /**< (SSC_RCMR) Continuous Receive Clock, RK pin is an output  */
#define   SSC_RCMR_CKO_TRANSFER_Val         (0x2U)                                         /**< (SSC_RCMR) Receive Clock only during data transfers, RK pin is an output  */
#define SSC_RCMR_CKO_NONE                   (SSC_RCMR_CKO_NONE_Val << SSC_RCMR_CKO_Pos)    /**< (SSC_RCMR) None, RK pin is an input Position  */
#define SSC_RCMR_CKO_CONTINUOUS             (SSC_RCMR_CKO_CONTINUOUS_Val << SSC_RCMR_CKO_Pos)  /**< (SSC_RCMR) Continuous Receive Clock, RK pin is an output Position  */
#define SSC_RCMR_CKO_TRANSFER               (SSC_RCMR_CKO_TRANSFER_Val << SSC_RCMR_CKO_Pos)  /**< (SSC_RCMR) Receive Clock only during data transfers, RK pin is an output Position  */
#define SSC_RCMR_CKI_Pos                    5                                              /**< (SSC_RCMR) Receive Clock Inversion Position */
#define SSC_RCMR_CKI_Msk                    (0x1U << SSC_RCMR_CKI_Pos)                     /**< (SSC_RCMR) Receive Clock Inversion Mask */
#define SSC_RCMR_CKI                        SSC_RCMR_CKI_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use SSC_RCMR_CKI_Msk instead */
#define SSC_RCMR_CKG_Pos                    6                                              /**< (SSC_RCMR) Receive Clock Gating Selection Position */
#define SSC_RCMR_CKG_Msk                    (0x3U << SSC_RCMR_CKG_Pos)                     /**< (SSC_RCMR) Receive Clock Gating Selection Mask */
#define SSC_RCMR_CKG(value)                 (SSC_RCMR_CKG_Msk & ((value) << SSC_RCMR_CKG_Pos))
#define   SSC_RCMR_CKG_CONTINUOUS_Val       (0x0U)                                         /**< (SSC_RCMR) None  */
#define   SSC_RCMR_CKG_EN_RF_LOW_Val        (0x1U)                                         /**< (SSC_RCMR) Receive Clock enabled only if RF Low  */
#define   SSC_RCMR_CKG_EN_RF_HIGH_Val       (0x2U)                                         /**< (SSC_RCMR) Receive Clock enabled only if RF High  */
#define SSC_RCMR_CKG_CONTINUOUS             (SSC_RCMR_CKG_CONTINUOUS_Val << SSC_RCMR_CKG_Pos)  /**< (SSC_RCMR) None Position  */
#define SSC_RCMR_CKG_EN_RF_LOW              (SSC_RCMR_CKG_EN_RF_LOW_Val << SSC_RCMR_CKG_Pos)  /**< (SSC_RCMR) Receive Clock enabled only if RF Low Position  */
#define SSC_RCMR_CKG_EN_RF_HIGH             (SSC_RCMR_CKG_EN_RF_HIGH_Val << SSC_RCMR_CKG_Pos)  /**< (SSC_RCMR) Receive Clock enabled only if RF High Position  */
#define SSC_RCMR_START_Pos                  8                                              /**< (SSC_RCMR) Receive Start Selection Position */
#define SSC_RCMR_START_Msk                  (0xFU << SSC_RCMR_START_Pos)                   /**< (SSC_RCMR) Receive Start Selection Mask */
#define SSC_RCMR_START(value)               (SSC_RCMR_START_Msk & ((value) << SSC_RCMR_START_Pos))
#define   SSC_RCMR_START_CONTINUOUS_Val     (0x0U)                                         /**< (SSC_RCMR) Continuous, as soon as the receiver is enabled, and immediately after the end of transfer of the previous data.  */
#define   SSC_RCMR_START_TRANSMIT_Val       (0x1U)                                         /**< (SSC_RCMR) Transmit start  */
#define   SSC_RCMR_START_RF_LOW_Val         (0x2U)                                         /**< (SSC_RCMR) Detection of a low level on RF signal  */
#define   SSC_RCMR_START_RF_HIGH_Val        (0x3U)                                         /**< (SSC_RCMR) Detection of a high level on RF signal  */
#define   SSC_RCMR_START_RF_FALLING_Val     (0x4U)                                         /**< (SSC_RCMR) Detection of a falling edge on RF signal  */
#define   SSC_RCMR_START_RF_RISING_Val      (0x5U)                                         /**< (SSC_RCMR) Detection of a rising edge on RF signal  */
#define   SSC_RCMR_START_RF_LEVEL_Val       (0x6U)                                         /**< (SSC_RCMR) Detection of any level change on RF signal  */
#define   SSC_RCMR_START_RF_EDGE_Val        (0x7U)                                         /**< (SSC_RCMR) Detection of any edge on RF signal  */
#define   SSC_RCMR_START_CMP_0_Val          (0x8U)                                         /**< (SSC_RCMR) Compare 0  */
#define SSC_RCMR_START_CONTINUOUS           (SSC_RCMR_START_CONTINUOUS_Val << SSC_RCMR_START_Pos)  /**< (SSC_RCMR) Continuous, as soon as the receiver is enabled, and immediately after the end of transfer of the previous data. Position  */
#define SSC_RCMR_START_TRANSMIT             (SSC_RCMR_START_TRANSMIT_Val << SSC_RCMR_START_Pos)  /**< (SSC_RCMR) Transmit start Position  */
#define SSC_RCMR_START_RF_LOW               (SSC_RCMR_START_RF_LOW_Val << SSC_RCMR_START_Pos)  /**< (SSC_RCMR) Detection of a low level on RF signal Position  */
#define SSC_RCMR_START_RF_HIGH              (SSC_RCMR_START_RF_HIGH_Val << SSC_RCMR_START_Pos)  /**< (SSC_RCMR) Detection of a high level on RF signal Position  */
#define SSC_RCMR_START_RF_FALLING           (SSC_RCMR_START_RF_FALLING_Val << SSC_RCMR_START_Pos)  /**< (SSC_RCMR) Detection of a falling edge on RF signal Position  */
#define SSC_RCMR_START_RF_RISING            (SSC_RCMR_START_RF_RISING_Val << SSC_RCMR_START_Pos)  /**< (SSC_RCMR) Detection of a rising edge on RF signal Position  */
#define SSC_RCMR_START_RF_LEVEL             (SSC_RCMR_START_RF_LEVEL_Val << SSC_RCMR_START_Pos)  /**< (SSC_RCMR) Detection of any level change on RF signal Position  */
#define SSC_RCMR_START_RF_EDGE              (SSC_RCMR_START_RF_EDGE_Val << SSC_RCMR_START_Pos)  /**< (SSC_RCMR) Detection of any edge on RF signal Position  */
#define SSC_RCMR_START_CMP_0                (SSC_RCMR_START_CMP_0_Val << SSC_RCMR_START_Pos)  /**< (SSC_RCMR) Compare 0 Position  */
#define SSC_RCMR_STOP_Pos                   12                                             /**< (SSC_RCMR) Receive Stop Selection Position */
#define SSC_RCMR_STOP_Msk                   (0x1U << SSC_RCMR_STOP_Pos)                    /**< (SSC_RCMR) Receive Stop Selection Mask */
#define SSC_RCMR_STOP                       SSC_RCMR_STOP_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use SSC_RCMR_STOP_Msk instead */
#define SSC_RCMR_STTDLY_Pos                 16                                             /**< (SSC_RCMR) Receive Start Delay Position */
#define SSC_RCMR_STTDLY_Msk                 (0xFFU << SSC_RCMR_STTDLY_Pos)                 /**< (SSC_RCMR) Receive Start Delay Mask */
#define SSC_RCMR_STTDLY(value)              (SSC_RCMR_STTDLY_Msk & ((value) << SSC_RCMR_STTDLY_Pos))
#define SSC_RCMR_PERIOD_Pos                 24                                             /**< (SSC_RCMR) Receive Period Divider Selection Position */
#define SSC_RCMR_PERIOD_Msk                 (0xFFU << SSC_RCMR_PERIOD_Pos)                 /**< (SSC_RCMR) Receive Period Divider Selection Mask */
#define SSC_RCMR_PERIOD(value)              (SSC_RCMR_PERIOD_Msk & ((value) << SSC_RCMR_PERIOD_Pos))
#define SSC_RCMR_MASK                       (0xFFFF1FFFU)                                  /**< \deprecated (SSC_RCMR) Register MASK  (Use SSC_RCMR_Msk instead)  */
#define SSC_RCMR_Msk                        (0xFFFF1FFFU)                                  /**< (SSC_RCMR) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- SSC_RFMR : (SSC Offset: 0x14) (R/W 32) Receive Frame Mode Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t DATLEN:5;                  /**< bit:   0..4  Data Length                              */
    uint32_t LOOP:1;                    /**< bit:      5  Loop Mode                                */
    uint32_t :1;                        /**< bit:      6  Reserved */
    uint32_t MSBF:1;                    /**< bit:      7  Most Significant Bit First               */
    uint32_t DATNB:4;                   /**< bit:  8..11  Data Number per Frame                    */
    uint32_t :4;                        /**< bit: 12..15  Reserved */
    uint32_t FSLEN:4;                   /**< bit: 16..19  Receive Frame Sync Length                */
    uint32_t FSOS:3;                    /**< bit: 20..22  Receive Frame Sync Output Selection      */
    uint32_t :1;                        /**< bit:     23  Reserved */
    uint32_t FSEDGE:1;                  /**< bit:     24  Frame Sync Edge Detection                */
    uint32_t :3;                        /**< bit: 25..27  Reserved */
    uint32_t FSLEN_EXT:4;               /**< bit: 28..31  FSLEN Field Extension                    */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} SSC_RFMR_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define SSC_RFMR_OFFSET                     (0x14)                                        /**<  (SSC_RFMR) Receive Frame Mode Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define SSC_RFMR_DATLEN_Pos                 0                                              /**< (SSC_RFMR) Data Length Position */
#define SSC_RFMR_DATLEN_Msk                 (0x1FU << SSC_RFMR_DATLEN_Pos)                 /**< (SSC_RFMR) Data Length Mask */
#define SSC_RFMR_DATLEN(value)              (SSC_RFMR_DATLEN_Msk & ((value) << SSC_RFMR_DATLEN_Pos))
#define SSC_RFMR_LOOP_Pos                   5                                              /**< (SSC_RFMR) Loop Mode Position */
#define SSC_RFMR_LOOP_Msk                   (0x1U << SSC_RFMR_LOOP_Pos)                    /**< (SSC_RFMR) Loop Mode Mask */
#define SSC_RFMR_LOOP                       SSC_RFMR_LOOP_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use SSC_RFMR_LOOP_Msk instead */
#define SSC_RFMR_MSBF_Pos                   7                                              /**< (SSC_RFMR) Most Significant Bit First Position */
#define SSC_RFMR_MSBF_Msk                   (0x1U << SSC_RFMR_MSBF_Pos)                    /**< (SSC_RFMR) Most Significant Bit First Mask */
#define SSC_RFMR_MSBF                       SSC_RFMR_MSBF_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use SSC_RFMR_MSBF_Msk instead */
#define SSC_RFMR_DATNB_Pos                  8                                              /**< (SSC_RFMR) Data Number per Frame Position */
#define SSC_RFMR_DATNB_Msk                  (0xFU << SSC_RFMR_DATNB_Pos)                   /**< (SSC_RFMR) Data Number per Frame Mask */
#define SSC_RFMR_DATNB(value)               (SSC_RFMR_DATNB_Msk & ((value) << SSC_RFMR_DATNB_Pos))
#define SSC_RFMR_FSLEN_Pos                  16                                             /**< (SSC_RFMR) Receive Frame Sync Length Position */
#define SSC_RFMR_FSLEN_Msk                  (0xFU << SSC_RFMR_FSLEN_Pos)                   /**< (SSC_RFMR) Receive Frame Sync Length Mask */
#define SSC_RFMR_FSLEN(value)               (SSC_RFMR_FSLEN_Msk & ((value) << SSC_RFMR_FSLEN_Pos))
#define SSC_RFMR_FSOS_Pos                   20                                             /**< (SSC_RFMR) Receive Frame Sync Output Selection Position */
#define SSC_RFMR_FSOS_Msk                   (0x7U << SSC_RFMR_FSOS_Pos)                    /**< (SSC_RFMR) Receive Frame Sync Output Selection Mask */
#define SSC_RFMR_FSOS(value)                (SSC_RFMR_FSOS_Msk & ((value) << SSC_RFMR_FSOS_Pos))
#define   SSC_RFMR_FSOS_NONE_Val            (0x0U)                                         /**< (SSC_RFMR) None, RF pin is an input  */
#define   SSC_RFMR_FSOS_NEGATIVE_Val        (0x1U)                                         /**< (SSC_RFMR) Negative Pulse, RF pin is an output  */
#define   SSC_RFMR_FSOS_POSITIVE_Val        (0x2U)                                         /**< (SSC_RFMR) Positive Pulse, RF pin is an output  */
#define   SSC_RFMR_FSOS_LOW_Val             (0x3U)                                         /**< (SSC_RFMR) Driven Low during data transfer, RF pin is an output  */
#define   SSC_RFMR_FSOS_HIGH_Val            (0x4U)                                         /**< (SSC_RFMR) Driven High during data transfer, RF pin is an output  */
#define   SSC_RFMR_FSOS_TOGGLING_Val        (0x5U)                                         /**< (SSC_RFMR) Toggling at each start of data transfer, RF pin is an output  */
#define SSC_RFMR_FSOS_NONE                  (SSC_RFMR_FSOS_NONE_Val << SSC_RFMR_FSOS_Pos)  /**< (SSC_RFMR) None, RF pin is an input Position  */
#define SSC_RFMR_FSOS_NEGATIVE              (SSC_RFMR_FSOS_NEGATIVE_Val << SSC_RFMR_FSOS_Pos)  /**< (SSC_RFMR) Negative Pulse, RF pin is an output Position  */
#define SSC_RFMR_FSOS_POSITIVE              (SSC_RFMR_FSOS_POSITIVE_Val << SSC_RFMR_FSOS_Pos)  /**< (SSC_RFMR) Positive Pulse, RF pin is an output Position  */
#define SSC_RFMR_FSOS_LOW                   (SSC_RFMR_FSOS_LOW_Val << SSC_RFMR_FSOS_Pos)   /**< (SSC_RFMR) Driven Low during data transfer, RF pin is an output Position  */
#define SSC_RFMR_FSOS_HIGH                  (SSC_RFMR_FSOS_HIGH_Val << SSC_RFMR_FSOS_Pos)  /**< (SSC_RFMR) Driven High during data transfer, RF pin is an output Position  */
#define SSC_RFMR_FSOS_TOGGLING              (SSC_RFMR_FSOS_TOGGLING_Val << SSC_RFMR_FSOS_Pos)  /**< (SSC_RFMR) Toggling at each start of data transfer, RF pin is an output Position  */
#define SSC_RFMR_FSEDGE_Pos                 24                                             /**< (SSC_RFMR) Frame Sync Edge Detection Position */
#define SSC_RFMR_FSEDGE_Msk                 (0x1U << SSC_RFMR_FSEDGE_Pos)                  /**< (SSC_RFMR) Frame Sync Edge Detection Mask */
#define SSC_RFMR_FSEDGE                     SSC_RFMR_FSEDGE_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use SSC_RFMR_FSEDGE_Msk instead */
#define   SSC_RFMR_FSEDGE_POSITIVE_Val      (0x0U)                                         /**< (SSC_RFMR) Positive Edge Detection  */
#define   SSC_RFMR_FSEDGE_NEGATIVE_Val      (0x1U)                                         /**< (SSC_RFMR) Negative Edge Detection  */
#define SSC_RFMR_FSEDGE_POSITIVE            (SSC_RFMR_FSEDGE_POSITIVE_Val << SSC_RFMR_FSEDGE_Pos)  /**< (SSC_RFMR) Positive Edge Detection Position  */
#define SSC_RFMR_FSEDGE_NEGATIVE            (SSC_RFMR_FSEDGE_NEGATIVE_Val << SSC_RFMR_FSEDGE_Pos)  /**< (SSC_RFMR) Negative Edge Detection Position  */
#define SSC_RFMR_FSLEN_EXT_Pos              28                                             /**< (SSC_RFMR) FSLEN Field Extension Position */
#define SSC_RFMR_FSLEN_EXT_Msk              (0xFU << SSC_RFMR_FSLEN_EXT_Pos)               /**< (SSC_RFMR) FSLEN Field Extension Mask */
#define SSC_RFMR_FSLEN_EXT(value)           (SSC_RFMR_FSLEN_EXT_Msk & ((value) << SSC_RFMR_FSLEN_EXT_Pos))
#define SSC_RFMR_MASK                       (0xF17F0FBFU)                                  /**< \deprecated (SSC_RFMR) Register MASK  (Use SSC_RFMR_Msk instead)  */
#define SSC_RFMR_Msk                        (0xF17F0FBFU)                                  /**< (SSC_RFMR) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- SSC_TCMR : (SSC Offset: 0x18) (R/W 32) Transmit Clock Mode Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t CKS:2;                     /**< bit:   0..1  Transmit Clock Selection                 */
    uint32_t CKO:3;                     /**< bit:   2..4  Transmit Clock Output Mode Selection     */
    uint32_t CKI:1;                     /**< bit:      5  Transmit Clock Inversion                 */
    uint32_t CKG:2;                     /**< bit:   6..7  Transmit Clock Gating Selection          */
    uint32_t START:4;                   /**< bit:  8..11  Transmit Start Selection                 */
    uint32_t :4;                        /**< bit: 12..15  Reserved */
    uint32_t STTDLY:8;                  /**< bit: 16..23  Transmit Start Delay                     */
    uint32_t PERIOD:8;                  /**< bit: 24..31  Transmit Period Divider Selection        */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} SSC_TCMR_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define SSC_TCMR_OFFSET                     (0x18)                                        /**<  (SSC_TCMR) Transmit Clock Mode Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define SSC_TCMR_CKS_Pos                    0                                              /**< (SSC_TCMR) Transmit Clock Selection Position */
#define SSC_TCMR_CKS_Msk                    (0x3U << SSC_TCMR_CKS_Pos)                     /**< (SSC_TCMR) Transmit Clock Selection Mask */
#define SSC_TCMR_CKS(value)                 (SSC_TCMR_CKS_Msk & ((value) << SSC_TCMR_CKS_Pos))
#define   SSC_TCMR_CKS_MCK_Val              (0x0U)                                         /**< (SSC_TCMR) Divided Clock  */
#define   SSC_TCMR_CKS_RK_Val               (0x1U)                                         /**< (SSC_TCMR) RK Clock signal  */
#define   SSC_TCMR_CKS_TK_Val               (0x2U)                                         /**< (SSC_TCMR) TK pin  */
#define SSC_TCMR_CKS_MCK                    (SSC_TCMR_CKS_MCK_Val << SSC_TCMR_CKS_Pos)     /**< (SSC_TCMR) Divided Clock Position  */
#define SSC_TCMR_CKS_RK                     (SSC_TCMR_CKS_RK_Val << SSC_TCMR_CKS_Pos)      /**< (SSC_TCMR) RK Clock signal Position  */
#define SSC_TCMR_CKS_TK                     (SSC_TCMR_CKS_TK_Val << SSC_TCMR_CKS_Pos)      /**< (SSC_TCMR) TK pin Position  */
#define SSC_TCMR_CKO_Pos                    2                                              /**< (SSC_TCMR) Transmit Clock Output Mode Selection Position */
#define SSC_TCMR_CKO_Msk                    (0x7U << SSC_TCMR_CKO_Pos)                     /**< (SSC_TCMR) Transmit Clock Output Mode Selection Mask */
#define SSC_TCMR_CKO(value)                 (SSC_TCMR_CKO_Msk & ((value) << SSC_TCMR_CKO_Pos))
#define   SSC_TCMR_CKO_NONE_Val             (0x0U)                                         /**< (SSC_TCMR) None, TK pin is an input  */
#define   SSC_TCMR_CKO_CONTINUOUS_Val       (0x1U)                                         /**< (SSC_TCMR) Continuous Transmit Clock, TK pin is an output  */
#define   SSC_TCMR_CKO_TRANSFER_Val         (0x2U)                                         /**< (SSC_TCMR) Transmit Clock only during data transfers, TK pin is an output  */
#define SSC_TCMR_CKO_NONE                   (SSC_TCMR_CKO_NONE_Val << SSC_TCMR_CKO_Pos)    /**< (SSC_TCMR) None, TK pin is an input Position  */
#define SSC_TCMR_CKO_CONTINUOUS             (SSC_TCMR_CKO_CONTINUOUS_Val << SSC_TCMR_CKO_Pos)  /**< (SSC_TCMR) Continuous Transmit Clock, TK pin is an output Position  */
#define SSC_TCMR_CKO_TRANSFER               (SSC_TCMR_CKO_TRANSFER_Val << SSC_TCMR_CKO_Pos)  /**< (SSC_TCMR) Transmit Clock only during data transfers, TK pin is an output Position  */
#define SSC_TCMR_CKI_Pos                    5                                              /**< (SSC_TCMR) Transmit Clock Inversion Position */
#define SSC_TCMR_CKI_Msk                    (0x1U << SSC_TCMR_CKI_Pos)                     /**< (SSC_TCMR) Transmit Clock Inversion Mask */
#define SSC_TCMR_CKI                        SSC_TCMR_CKI_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use SSC_TCMR_CKI_Msk instead */
#define SSC_TCMR_CKG_Pos                    6                                              /**< (SSC_TCMR) Transmit Clock Gating Selection Position */
#define SSC_TCMR_CKG_Msk                    (0x3U << SSC_TCMR_CKG_Pos)                     /**< (SSC_TCMR) Transmit Clock Gating Selection Mask */
#define SSC_TCMR_CKG(value)                 (SSC_TCMR_CKG_Msk & ((value) << SSC_TCMR_CKG_Pos))
#define   SSC_TCMR_CKG_CONTINUOUS_Val       (0x0U)                                         /**< (SSC_TCMR) None  */
#define   SSC_TCMR_CKG_EN_TF_LOW_Val        (0x1U)                                         /**< (SSC_TCMR) Transmit Clock enabled only if TF Low  */
#define   SSC_TCMR_CKG_EN_TF_HIGH_Val       (0x2U)                                         /**< (SSC_TCMR) Transmit Clock enabled only if TF High  */
#define SSC_TCMR_CKG_CONTINUOUS             (SSC_TCMR_CKG_CONTINUOUS_Val << SSC_TCMR_CKG_Pos)  /**< (SSC_TCMR) None Position  */
#define SSC_TCMR_CKG_EN_TF_LOW              (SSC_TCMR_CKG_EN_TF_LOW_Val << SSC_TCMR_CKG_Pos)  /**< (SSC_TCMR) Transmit Clock enabled only if TF Low Position  */
#define SSC_TCMR_CKG_EN_TF_HIGH             (SSC_TCMR_CKG_EN_TF_HIGH_Val << SSC_TCMR_CKG_Pos)  /**< (SSC_TCMR) Transmit Clock enabled only if TF High Position  */
#define SSC_TCMR_START_Pos                  8                                              /**< (SSC_TCMR) Transmit Start Selection Position */
#define SSC_TCMR_START_Msk                  (0xFU << SSC_TCMR_START_Pos)                   /**< (SSC_TCMR) Transmit Start Selection Mask */
#define SSC_TCMR_START(value)               (SSC_TCMR_START_Msk & ((value) << SSC_TCMR_START_Pos))
#define   SSC_TCMR_START_CONTINUOUS_Val     (0x0U)                                         /**< (SSC_TCMR) Continuous, as soon as a word is written in the SSC_THR (if Transmit is enabled), and immediately after the end of transfer of the previous data  */
#define   SSC_TCMR_START_RECEIVE_Val        (0x1U)                                         /**< (SSC_TCMR) Receive start  */
#define   SSC_TCMR_START_TF_LOW_Val         (0x2U)                                         /**< (SSC_TCMR) Detection of a low level on TF signal  */
#define   SSC_TCMR_START_TF_HIGH_Val        (0x3U)                                         /**< (SSC_TCMR) Detection of a high level on TF signal  */
#define   SSC_TCMR_START_TF_FALLING_Val     (0x4U)                                         /**< (SSC_TCMR) Detection of a falling edge on TF signal  */
#define   SSC_TCMR_START_TF_RISING_Val      (0x5U)                                         /**< (SSC_TCMR) Detection of a rising edge on TF signal  */
#define   SSC_TCMR_START_TF_LEVEL_Val       (0x6U)                                         /**< (SSC_TCMR) Detection of any level change on TF signal  */
#define   SSC_TCMR_START_TF_EDGE_Val        (0x7U)                                         /**< (SSC_TCMR) Detection of any edge on TF signal  */
#define SSC_TCMR_START_CONTINUOUS           (SSC_TCMR_START_CONTINUOUS_Val << SSC_TCMR_START_Pos)  /**< (SSC_TCMR) Continuous, as soon as a word is written in the SSC_THR (if Transmit is enabled), and immediately after the end of transfer of the previous data Position  */
#define SSC_TCMR_START_RECEIVE              (SSC_TCMR_START_RECEIVE_Val << SSC_TCMR_START_Pos)  /**< (SSC_TCMR) Receive start Position  */
#define SSC_TCMR_START_TF_LOW               (SSC_TCMR_START_TF_LOW_Val << SSC_TCMR_START_Pos)  /**< (SSC_TCMR) Detection of a low level on TF signal Position  */
#define SSC_TCMR_START_TF_HIGH              (SSC_TCMR_START_TF_HIGH_Val << SSC_TCMR_START_Pos)  /**< (SSC_TCMR) Detection of a high level on TF signal Position  */
#define SSC_TCMR_START_TF_FALLING           (SSC_TCMR_START_TF_FALLING_Val << SSC_TCMR_START_Pos)  /**< (SSC_TCMR) Detection of a falling edge on TF signal Position  */
#define SSC_TCMR_START_TF_RISING            (SSC_TCMR_START_TF_RISING_Val << SSC_TCMR_START_Pos)  /**< (SSC_TCMR) Detection of a rising edge on TF signal Position  */
#define SSC_TCMR_START_TF_LEVEL             (SSC_TCMR_START_TF_LEVEL_Val << SSC_TCMR_START_Pos)  /**< (SSC_TCMR) Detection of any level change on TF signal Position  */
#define SSC_TCMR_START_TF_EDGE              (SSC_TCMR_START_TF_EDGE_Val << SSC_TCMR_START_Pos)  /**< (SSC_TCMR) Detection of any edge on TF signal Position  */
#define SSC_TCMR_STTDLY_Pos                 16                                             /**< (SSC_TCMR) Transmit Start Delay Position */
#define SSC_TCMR_STTDLY_Msk                 (0xFFU << SSC_TCMR_STTDLY_Pos)                 /**< (SSC_TCMR) Transmit Start Delay Mask */
#define SSC_TCMR_STTDLY(value)              (SSC_TCMR_STTDLY_Msk & ((value) << SSC_TCMR_STTDLY_Pos))
#define SSC_TCMR_PERIOD_Pos                 24                                             /**< (SSC_TCMR) Transmit Period Divider Selection Position */
#define SSC_TCMR_PERIOD_Msk                 (0xFFU << SSC_TCMR_PERIOD_Pos)                 /**< (SSC_TCMR) Transmit Period Divider Selection Mask */
#define SSC_TCMR_PERIOD(value)              (SSC_TCMR_PERIOD_Msk & ((value) << SSC_TCMR_PERIOD_Pos))
#define SSC_TCMR_MASK                       (0xFFFF0FFFU)                                  /**< \deprecated (SSC_TCMR) Register MASK  (Use SSC_TCMR_Msk instead)  */
#define SSC_TCMR_Msk                        (0xFFFF0FFFU)                                  /**< (SSC_TCMR) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- SSC_TFMR : (SSC Offset: 0x1c) (R/W 32) Transmit Frame Mode Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t DATLEN:5;                  /**< bit:   0..4  Data Length                              */
    uint32_t DATDEF:1;                  /**< bit:      5  Data Default Value                       */
    uint32_t :1;                        /**< bit:      6  Reserved */
    uint32_t MSBF:1;                    /**< bit:      7  Most Significant Bit First               */
    uint32_t DATNB:4;                   /**< bit:  8..11  Data Number per Frame                    */
    uint32_t :4;                        /**< bit: 12..15  Reserved */
    uint32_t FSLEN:4;                   /**< bit: 16..19  Transmit Frame Sync Length               */
    uint32_t FSOS:3;                    /**< bit: 20..22  Transmit Frame Sync Output Selection     */
    uint32_t FSDEN:1;                   /**< bit:     23  Frame Sync Data Enable                   */
    uint32_t FSEDGE:1;                  /**< bit:     24  Frame Sync Edge Detection                */
    uint32_t :3;                        /**< bit: 25..27  Reserved */
    uint32_t FSLEN_EXT:4;               /**< bit: 28..31  FSLEN Field Extension                    */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} SSC_TFMR_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define SSC_TFMR_OFFSET                     (0x1C)                                        /**<  (SSC_TFMR) Transmit Frame Mode Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define SSC_TFMR_DATLEN_Pos                 0                                              /**< (SSC_TFMR) Data Length Position */
#define SSC_TFMR_DATLEN_Msk                 (0x1FU << SSC_TFMR_DATLEN_Pos)                 /**< (SSC_TFMR) Data Length Mask */
#define SSC_TFMR_DATLEN(value)              (SSC_TFMR_DATLEN_Msk & ((value) << SSC_TFMR_DATLEN_Pos))
#define SSC_TFMR_DATDEF_Pos                 5                                              /**< (SSC_TFMR) Data Default Value Position */
#define SSC_TFMR_DATDEF_Msk                 (0x1U << SSC_TFMR_DATDEF_Pos)                  /**< (SSC_TFMR) Data Default Value Mask */
#define SSC_TFMR_DATDEF                     SSC_TFMR_DATDEF_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use SSC_TFMR_DATDEF_Msk instead */
#define SSC_TFMR_MSBF_Pos                   7                                              /**< (SSC_TFMR) Most Significant Bit First Position */
#define SSC_TFMR_MSBF_Msk                   (0x1U << SSC_TFMR_MSBF_Pos)                    /**< (SSC_TFMR) Most Significant Bit First Mask */
#define SSC_TFMR_MSBF                       SSC_TFMR_MSBF_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use SSC_TFMR_MSBF_Msk instead */
#define SSC_TFMR_DATNB_Pos                  8                                              /**< (SSC_TFMR) Data Number per Frame Position */
#define SSC_TFMR_DATNB_Msk                  (0xFU << SSC_TFMR_DATNB_Pos)                   /**< (SSC_TFMR) Data Number per Frame Mask */
#define SSC_TFMR_DATNB(value)               (SSC_TFMR_DATNB_Msk & ((value) << SSC_TFMR_DATNB_Pos))
#define SSC_TFMR_FSLEN_Pos                  16                                             /**< (SSC_TFMR) Transmit Frame Sync Length Position */
#define SSC_TFMR_FSLEN_Msk                  (0xFU << SSC_TFMR_FSLEN_Pos)                   /**< (SSC_TFMR) Transmit Frame Sync Length Mask */
#define SSC_TFMR_FSLEN(value)               (SSC_TFMR_FSLEN_Msk & ((value) << SSC_TFMR_FSLEN_Pos))
#define SSC_TFMR_FSOS_Pos                   20                                             /**< (SSC_TFMR) Transmit Frame Sync Output Selection Position */
#define SSC_TFMR_FSOS_Msk                   (0x7U << SSC_TFMR_FSOS_Pos)                    /**< (SSC_TFMR) Transmit Frame Sync Output Selection Mask */
#define SSC_TFMR_FSOS(value)                (SSC_TFMR_FSOS_Msk & ((value) << SSC_TFMR_FSOS_Pos))
#define   SSC_TFMR_FSOS_NONE_Val            (0x0U)                                         /**< (SSC_TFMR) None, TF pin is an input  */
#define   SSC_TFMR_FSOS_NEGATIVE_Val        (0x1U)                                         /**< (SSC_TFMR) Negative Pulse, TF pin is an output  */
#define   SSC_TFMR_FSOS_POSITIVE_Val        (0x2U)                                         /**< (SSC_TFMR) Positive Pulse, TF pin is an output  */
#define   SSC_TFMR_FSOS_LOW_Val             (0x3U)                                         /**< (SSC_TFMR) Driven Low during data transfer  */
#define   SSC_TFMR_FSOS_HIGH_Val            (0x4U)                                         /**< (SSC_TFMR) Driven High during data transfer  */
#define   SSC_TFMR_FSOS_TOGGLING_Val        (0x5U)                                         /**< (SSC_TFMR) Toggling at each start of data transfer  */
#define SSC_TFMR_FSOS_NONE                  (SSC_TFMR_FSOS_NONE_Val << SSC_TFMR_FSOS_Pos)  /**< (SSC_TFMR) None, TF pin is an input Position  */
#define SSC_TFMR_FSOS_NEGATIVE              (SSC_TFMR_FSOS_NEGATIVE_Val << SSC_TFMR_FSOS_Pos)  /**< (SSC_TFMR) Negative Pulse, TF pin is an output Position  */
#define SSC_TFMR_FSOS_POSITIVE              (SSC_TFMR_FSOS_POSITIVE_Val << SSC_TFMR_FSOS_Pos)  /**< (SSC_TFMR) Positive Pulse, TF pin is an output Position  */
#define SSC_TFMR_FSOS_LOW                   (SSC_TFMR_FSOS_LOW_Val << SSC_TFMR_FSOS_Pos)   /**< (SSC_TFMR) Driven Low during data transfer Position  */
#define SSC_TFMR_FSOS_HIGH                  (SSC_TFMR_FSOS_HIGH_Val << SSC_TFMR_FSOS_Pos)  /**< (SSC_TFMR) Driven High during data transfer Position  */
#define SSC_TFMR_FSOS_TOGGLING              (SSC_TFMR_FSOS_TOGGLING_Val << SSC_TFMR_FSOS_Pos)  /**< (SSC_TFMR) Toggling at each start of data transfer Position  */
#define SSC_TFMR_FSDEN_Pos                  23                                             /**< (SSC_TFMR) Frame Sync Data Enable Position */
#define SSC_TFMR_FSDEN_Msk                  (0x1U << SSC_TFMR_FSDEN_Pos)                   /**< (SSC_TFMR) Frame Sync Data Enable Mask */
#define SSC_TFMR_FSDEN                      SSC_TFMR_FSDEN_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use SSC_TFMR_FSDEN_Msk instead */
#define SSC_TFMR_FSEDGE_Pos                 24                                             /**< (SSC_TFMR) Frame Sync Edge Detection Position */
#define SSC_TFMR_FSEDGE_Msk                 (0x1U << SSC_TFMR_FSEDGE_Pos)                  /**< (SSC_TFMR) Frame Sync Edge Detection Mask */
#define SSC_TFMR_FSEDGE                     SSC_TFMR_FSEDGE_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use SSC_TFMR_FSEDGE_Msk instead */
#define   SSC_TFMR_FSEDGE_POSITIVE_Val      (0x0U)                                         /**< (SSC_TFMR) Positive Edge Detection  */
#define   SSC_TFMR_FSEDGE_NEGATIVE_Val      (0x1U)                                         /**< (SSC_TFMR) Negative Edge Detection  */
#define SSC_TFMR_FSEDGE_POSITIVE            (SSC_TFMR_FSEDGE_POSITIVE_Val << SSC_TFMR_FSEDGE_Pos)  /**< (SSC_TFMR) Positive Edge Detection Position  */
#define SSC_TFMR_FSEDGE_NEGATIVE            (SSC_TFMR_FSEDGE_NEGATIVE_Val << SSC_TFMR_FSEDGE_Pos)  /**< (SSC_TFMR) Negative Edge Detection Position  */
#define SSC_TFMR_FSLEN_EXT_Pos              28                                             /**< (SSC_TFMR) FSLEN Field Extension Position */
#define SSC_TFMR_FSLEN_EXT_Msk              (0xFU << SSC_TFMR_FSLEN_EXT_Pos)               /**< (SSC_TFMR) FSLEN Field Extension Mask */
#define SSC_TFMR_FSLEN_EXT(value)           (SSC_TFMR_FSLEN_EXT_Msk & ((value) << SSC_TFMR_FSLEN_EXT_Pos))
#define SSC_TFMR_MASK                       (0xF1FF0FBFU)                                  /**< \deprecated (SSC_TFMR) Register MASK  (Use SSC_TFMR_Msk instead)  */
#define SSC_TFMR_Msk                        (0xF1FF0FBFU)                                  /**< (SSC_TFMR) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- SSC_RHR : (SSC Offset: 0x20) (R/ 32) Receive Holding Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t RDAT:32;                   /**< bit:  0..31  Receive Data                             */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} SSC_RHR_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define SSC_RHR_OFFSET                      (0x20)                                        /**<  (SSC_RHR) Receive Holding Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define SSC_RHR_RDAT_Pos                    0                                              /**< (SSC_RHR) Receive Data Position */
#define SSC_RHR_RDAT_Msk                    (0xFFFFFFFFU << SSC_RHR_RDAT_Pos)              /**< (SSC_RHR) Receive Data Mask */
#define SSC_RHR_RDAT(value)                 (SSC_RHR_RDAT_Msk & ((value) << SSC_RHR_RDAT_Pos))
#define SSC_RHR_MASK                        (0xFFFFFFFFU)                                  /**< \deprecated (SSC_RHR) Register MASK  (Use SSC_RHR_Msk instead)  */
#define SSC_RHR_Msk                         (0xFFFFFFFFU)                                  /**< (SSC_RHR) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- SSC_THR : (SSC Offset: 0x24) (/W 32) Transmit Holding Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t TDAT:32;                   /**< bit:  0..31  Transmit Data                            */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} SSC_THR_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define SSC_THR_OFFSET                      (0x24)                                        /**<  (SSC_THR) Transmit Holding Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define SSC_THR_TDAT_Pos                    0                                              /**< (SSC_THR) Transmit Data Position */
#define SSC_THR_TDAT_Msk                    (0xFFFFFFFFU << SSC_THR_TDAT_Pos)              /**< (SSC_THR) Transmit Data Mask */
#define SSC_THR_TDAT(value)                 (SSC_THR_TDAT_Msk & ((value) << SSC_THR_TDAT_Pos))
#define SSC_THR_MASK                        (0xFFFFFFFFU)                                  /**< \deprecated (SSC_THR) Register MASK  (Use SSC_THR_Msk instead)  */
#define SSC_THR_Msk                         (0xFFFFFFFFU)                                  /**< (SSC_THR) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- SSC_RSHR : (SSC Offset: 0x30) (R/ 32) Receive Sync. Holding Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t RSDAT:16;                  /**< bit:  0..15  Receive Synchronization Data             */
    uint32_t :16;                       /**< bit: 16..31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} SSC_RSHR_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define SSC_RSHR_OFFSET                     (0x30)                                        /**<  (SSC_RSHR) Receive Sync. Holding Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define SSC_RSHR_RSDAT_Pos                  0                                              /**< (SSC_RSHR) Receive Synchronization Data Position */
#define SSC_RSHR_RSDAT_Msk                  (0xFFFFU << SSC_RSHR_RSDAT_Pos)                /**< (SSC_RSHR) Receive Synchronization Data Mask */
#define SSC_RSHR_RSDAT(value)               (SSC_RSHR_RSDAT_Msk & ((value) << SSC_RSHR_RSDAT_Pos))
#define SSC_RSHR_MASK                       (0xFFFFU)                                      /**< \deprecated (SSC_RSHR) Register MASK  (Use SSC_RSHR_Msk instead)  */
#define SSC_RSHR_Msk                        (0xFFFFU)                                      /**< (SSC_RSHR) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- SSC_TSHR : (SSC Offset: 0x34) (R/W 32) Transmit Sync. Holding Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t TSDAT:16;                  /**< bit:  0..15  Transmit Synchronization Data            */
    uint32_t :16;                       /**< bit: 16..31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} SSC_TSHR_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define SSC_TSHR_OFFSET                     (0x34)                                        /**<  (SSC_TSHR) Transmit Sync. Holding Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define SSC_TSHR_TSDAT_Pos                  0                                              /**< (SSC_TSHR) Transmit Synchronization Data Position */
#define SSC_TSHR_TSDAT_Msk                  (0xFFFFU << SSC_TSHR_TSDAT_Pos)                /**< (SSC_TSHR) Transmit Synchronization Data Mask */
#define SSC_TSHR_TSDAT(value)               (SSC_TSHR_TSDAT_Msk & ((value) << SSC_TSHR_TSDAT_Pos))
#define SSC_TSHR_MASK                       (0xFFFFU)                                      /**< \deprecated (SSC_TSHR) Register MASK  (Use SSC_TSHR_Msk instead)  */
#define SSC_TSHR_Msk                        (0xFFFFU)                                      /**< (SSC_TSHR) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- SSC_RC0R : (SSC Offset: 0x38) (R/W 32) Receive Compare 0 Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t CP0:16;                    /**< bit:  0..15  Receive Compare Data 0                   */
    uint32_t :16;                       /**< bit: 16..31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} SSC_RC0R_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define SSC_RC0R_OFFSET                     (0x38)                                        /**<  (SSC_RC0R) Receive Compare 0 Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define SSC_RC0R_CP0_Pos                    0                                              /**< (SSC_RC0R) Receive Compare Data 0 Position */
#define SSC_RC0R_CP0_Msk                    (0xFFFFU << SSC_RC0R_CP0_Pos)                  /**< (SSC_RC0R) Receive Compare Data 0 Mask */
#define SSC_RC0R_CP0(value)                 (SSC_RC0R_CP0_Msk & ((value) << SSC_RC0R_CP0_Pos))
#define SSC_RC0R_MASK                       (0xFFFFU)                                      /**< \deprecated (SSC_RC0R) Register MASK  (Use SSC_RC0R_Msk instead)  */
#define SSC_RC0R_Msk                        (0xFFFFU)                                      /**< (SSC_RC0R) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- SSC_RC1R : (SSC Offset: 0x3c) (R/W 32) Receive Compare 1 Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t CP1:16;                    /**< bit:  0..15  Receive Compare Data 1                   */
    uint32_t :16;                       /**< bit: 16..31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} SSC_RC1R_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define SSC_RC1R_OFFSET                     (0x3C)                                        /**<  (SSC_RC1R) Receive Compare 1 Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define SSC_RC1R_CP1_Pos                    0                                              /**< (SSC_RC1R) Receive Compare Data 1 Position */
#define SSC_RC1R_CP1_Msk                    (0xFFFFU << SSC_RC1R_CP1_Pos)                  /**< (SSC_RC1R) Receive Compare Data 1 Mask */
#define SSC_RC1R_CP1(value)                 (SSC_RC1R_CP1_Msk & ((value) << SSC_RC1R_CP1_Pos))
#define SSC_RC1R_MASK                       (0xFFFFU)                                      /**< \deprecated (SSC_RC1R) Register MASK  (Use SSC_RC1R_Msk instead)  */
#define SSC_RC1R_Msk                        (0xFFFFU)                                      /**< (SSC_RC1R) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- SSC_SR : (SSC Offset: 0x40) (R/ 32) Status Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t TXRDY:1;                   /**< bit:      0  Transmit Ready                           */
    uint32_t TXEMPTY:1;                 /**< bit:      1  Transmit Empty                           */
    uint32_t :2;                        /**< bit:   2..3  Reserved */
    uint32_t RXRDY:1;                   /**< bit:      4  Receive Ready                            */
    uint32_t OVRUN:1;                   /**< bit:      5  Receive Overrun                          */
    uint32_t :2;                        /**< bit:   6..7  Reserved */
    uint32_t CP0:1;                     /**< bit:      8  Compare 0                                */
    uint32_t CP1:1;                     /**< bit:      9  Compare 1                                */
    uint32_t TXSYN:1;                   /**< bit:     10  Transmit Sync                            */
    uint32_t RXSYN:1;                   /**< bit:     11  Receive Sync                             */
    uint32_t :4;                        /**< bit: 12..15  Reserved */
    uint32_t TXEN:1;                    /**< bit:     16  Transmit Enable                          */
    uint32_t RXEN:1;                    /**< bit:     17  Receive Enable                           */
    uint32_t :14;                       /**< bit: 18..31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} SSC_SR_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define SSC_SR_OFFSET                       (0x40)                                        /**<  (SSC_SR) Status Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define SSC_SR_TXRDY_Pos                    0                                              /**< (SSC_SR) Transmit Ready Position */
#define SSC_SR_TXRDY_Msk                    (0x1U << SSC_SR_TXRDY_Pos)                     /**< (SSC_SR) Transmit Ready Mask */
#define SSC_SR_TXRDY                        SSC_SR_TXRDY_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use SSC_SR_TXRDY_Msk instead */
#define SSC_SR_TXEMPTY_Pos                  1                                              /**< (SSC_SR) Transmit Empty Position */
#define SSC_SR_TXEMPTY_Msk                  (0x1U << SSC_SR_TXEMPTY_Pos)                   /**< (SSC_SR) Transmit Empty Mask */
#define SSC_SR_TXEMPTY                      SSC_SR_TXEMPTY_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use SSC_SR_TXEMPTY_Msk instead */
#define SSC_SR_RXRDY_Pos                    4                                              /**< (SSC_SR) Receive Ready Position */
#define SSC_SR_RXRDY_Msk                    (0x1U << SSC_SR_RXRDY_Pos)                     /**< (SSC_SR) Receive Ready Mask */
#define SSC_SR_RXRDY                        SSC_SR_RXRDY_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use SSC_SR_RXRDY_Msk instead */
#define SSC_SR_OVRUN_Pos                    5                                              /**< (SSC_SR) Receive Overrun Position */
#define SSC_SR_OVRUN_Msk                    (0x1U << SSC_SR_OVRUN_Pos)                     /**< (SSC_SR) Receive Overrun Mask */
#define SSC_SR_OVRUN                        SSC_SR_OVRUN_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use SSC_SR_OVRUN_Msk instead */
#define SSC_SR_CP0_Pos                      8                                              /**< (SSC_SR) Compare 0 Position */
#define SSC_SR_CP0_Msk                      (0x1U << SSC_SR_CP0_Pos)                       /**< (SSC_SR) Compare 0 Mask */
#define SSC_SR_CP0                          SSC_SR_CP0_Msk                                 /**< \deprecated Old style mask definition for 1 bit bitfield. Use SSC_SR_CP0_Msk instead */
#define SSC_SR_CP1_Pos                      9                                              /**< (SSC_SR) Compare 1 Position */
#define SSC_SR_CP1_Msk                      (0x1U << SSC_SR_CP1_Pos)                       /**< (SSC_SR) Compare 1 Mask */
#define SSC_SR_CP1                          SSC_SR_CP1_Msk                                 /**< \deprecated Old style mask definition for 1 bit bitfield. Use SSC_SR_CP1_Msk instead */
#define SSC_SR_TXSYN_Pos                    10                                             /**< (SSC_SR) Transmit Sync Position */
#define SSC_SR_TXSYN_Msk                    (0x1U << SSC_SR_TXSYN_Pos)                     /**< (SSC_SR) Transmit Sync Mask */
#define SSC_SR_TXSYN                        SSC_SR_TXSYN_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use SSC_SR_TXSYN_Msk instead */
#define SSC_SR_RXSYN_Pos                    11                                             /**< (SSC_SR) Receive Sync Position */
#define SSC_SR_RXSYN_Msk                    (0x1U << SSC_SR_RXSYN_Pos)                     /**< (SSC_SR) Receive Sync Mask */
#define SSC_SR_RXSYN                        SSC_SR_RXSYN_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use SSC_SR_RXSYN_Msk instead */
#define SSC_SR_TXEN_Pos                     16                                             /**< (SSC_SR) Transmit Enable Position */
#define SSC_SR_TXEN_Msk                     (0x1U << SSC_SR_TXEN_Pos)                      /**< (SSC_SR) Transmit Enable Mask */
#define SSC_SR_TXEN                         SSC_SR_TXEN_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use SSC_SR_TXEN_Msk instead */
#define SSC_SR_RXEN_Pos                     17                                             /**< (SSC_SR) Receive Enable Position */
#define SSC_SR_RXEN_Msk                     (0x1U << SSC_SR_RXEN_Pos)                      /**< (SSC_SR) Receive Enable Mask */
#define SSC_SR_RXEN                         SSC_SR_RXEN_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use SSC_SR_RXEN_Msk instead */
#define SSC_SR_MASK                         (0x30F33U)                                     /**< \deprecated (SSC_SR) Register MASK  (Use SSC_SR_Msk instead)  */
#define SSC_SR_Msk                          (0x30F33U)                                     /**< (SSC_SR) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- SSC_IER : (SSC Offset: 0x44) (/W 32) Interrupt Enable Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t TXRDY:1;                   /**< bit:      0  Transmit Ready Interrupt Enable          */
    uint32_t TXEMPTY:1;                 /**< bit:      1  Transmit Empty Interrupt Enable          */
    uint32_t :2;                        /**< bit:   2..3  Reserved */
    uint32_t RXRDY:1;                   /**< bit:      4  Receive Ready Interrupt Enable           */
    uint32_t OVRUN:1;                   /**< bit:      5  Receive Overrun Interrupt Enable         */
    uint32_t :2;                        /**< bit:   6..7  Reserved */
    uint32_t CP0:1;                     /**< bit:      8  Compare 0 Interrupt Enable               */
    uint32_t CP1:1;                     /**< bit:      9  Compare 1 Interrupt Enable               */
    uint32_t TXSYN:1;                   /**< bit:     10  Tx Sync Interrupt Enable                 */
    uint32_t RXSYN:1;                   /**< bit:     11  Rx Sync Interrupt Enable                 */
    uint32_t :20;                       /**< bit: 12..31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} SSC_IER_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define SSC_IER_OFFSET                      (0x44)                                        /**<  (SSC_IER) Interrupt Enable Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define SSC_IER_TXRDY_Pos                   0                                              /**< (SSC_IER) Transmit Ready Interrupt Enable Position */
#define SSC_IER_TXRDY_Msk                   (0x1U << SSC_IER_TXRDY_Pos)                    /**< (SSC_IER) Transmit Ready Interrupt Enable Mask */
#define SSC_IER_TXRDY                       SSC_IER_TXRDY_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use SSC_IER_TXRDY_Msk instead */
#define SSC_IER_TXEMPTY_Pos                 1                                              /**< (SSC_IER) Transmit Empty Interrupt Enable Position */
#define SSC_IER_TXEMPTY_Msk                 (0x1U << SSC_IER_TXEMPTY_Pos)                  /**< (SSC_IER) Transmit Empty Interrupt Enable Mask */
#define SSC_IER_TXEMPTY                     SSC_IER_TXEMPTY_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use SSC_IER_TXEMPTY_Msk instead */
#define SSC_IER_RXRDY_Pos                   4                                              /**< (SSC_IER) Receive Ready Interrupt Enable Position */
#define SSC_IER_RXRDY_Msk                   (0x1U << SSC_IER_RXRDY_Pos)                    /**< (SSC_IER) Receive Ready Interrupt Enable Mask */
#define SSC_IER_RXRDY                       SSC_IER_RXRDY_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use SSC_IER_RXRDY_Msk instead */
#define SSC_IER_OVRUN_Pos                   5                                              /**< (SSC_IER) Receive Overrun Interrupt Enable Position */
#define SSC_IER_OVRUN_Msk                   (0x1U << SSC_IER_OVRUN_Pos)                    /**< (SSC_IER) Receive Overrun Interrupt Enable Mask */
#define SSC_IER_OVRUN                       SSC_IER_OVRUN_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use SSC_IER_OVRUN_Msk instead */
#define SSC_IER_CP0_Pos                     8                                              /**< (SSC_IER) Compare 0 Interrupt Enable Position */
#define SSC_IER_CP0_Msk                     (0x1U << SSC_IER_CP0_Pos)                      /**< (SSC_IER) Compare 0 Interrupt Enable Mask */
#define SSC_IER_CP0                         SSC_IER_CP0_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use SSC_IER_CP0_Msk instead */
#define SSC_IER_CP1_Pos                     9                                              /**< (SSC_IER) Compare 1 Interrupt Enable Position */
#define SSC_IER_CP1_Msk                     (0x1U << SSC_IER_CP1_Pos)                      /**< (SSC_IER) Compare 1 Interrupt Enable Mask */
#define SSC_IER_CP1                         SSC_IER_CP1_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use SSC_IER_CP1_Msk instead */
#define SSC_IER_TXSYN_Pos                   10                                             /**< (SSC_IER) Tx Sync Interrupt Enable Position */
#define SSC_IER_TXSYN_Msk                   (0x1U << SSC_IER_TXSYN_Pos)                    /**< (SSC_IER) Tx Sync Interrupt Enable Mask */
#define SSC_IER_TXSYN                       SSC_IER_TXSYN_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use SSC_IER_TXSYN_Msk instead */
#define SSC_IER_RXSYN_Pos                   11                                             /**< (SSC_IER) Rx Sync Interrupt Enable Position */
#define SSC_IER_RXSYN_Msk                   (0x1U << SSC_IER_RXSYN_Pos)                    /**< (SSC_IER) Rx Sync Interrupt Enable Mask */
#define SSC_IER_RXSYN                       SSC_IER_RXSYN_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use SSC_IER_RXSYN_Msk instead */
#define SSC_IER_MASK                        (0xF33U)                                       /**< \deprecated (SSC_IER) Register MASK  (Use SSC_IER_Msk instead)  */
#define SSC_IER_Msk                         (0xF33U)                                       /**< (SSC_IER) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- SSC_IDR : (SSC Offset: 0x48) (/W 32) Interrupt Disable Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t TXRDY:1;                   /**< bit:      0  Transmit Ready Interrupt Disable         */
    uint32_t TXEMPTY:1;                 /**< bit:      1  Transmit Empty Interrupt Disable         */
    uint32_t :2;                        /**< bit:   2..3  Reserved */
    uint32_t RXRDY:1;                   /**< bit:      4  Receive Ready Interrupt Disable          */
    uint32_t OVRUN:1;                   /**< bit:      5  Receive Overrun Interrupt Disable        */
    uint32_t :2;                        /**< bit:   6..7  Reserved */
    uint32_t CP0:1;                     /**< bit:      8  Compare 0 Interrupt Disable              */
    uint32_t CP1:1;                     /**< bit:      9  Compare 1 Interrupt Disable              */
    uint32_t TXSYN:1;                   /**< bit:     10  Tx Sync Interrupt Enable                 */
    uint32_t RXSYN:1;                   /**< bit:     11  Rx Sync Interrupt Enable                 */
    uint32_t :20;                       /**< bit: 12..31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} SSC_IDR_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define SSC_IDR_OFFSET                      (0x48)                                        /**<  (SSC_IDR) Interrupt Disable Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define SSC_IDR_TXRDY_Pos                   0                                              /**< (SSC_IDR) Transmit Ready Interrupt Disable Position */
#define SSC_IDR_TXRDY_Msk                   (0x1U << SSC_IDR_TXRDY_Pos)                    /**< (SSC_IDR) Transmit Ready Interrupt Disable Mask */
#define SSC_IDR_TXRDY                       SSC_IDR_TXRDY_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use SSC_IDR_TXRDY_Msk instead */
#define SSC_IDR_TXEMPTY_Pos                 1                                              /**< (SSC_IDR) Transmit Empty Interrupt Disable Position */
#define SSC_IDR_TXEMPTY_Msk                 (0x1U << SSC_IDR_TXEMPTY_Pos)                  /**< (SSC_IDR) Transmit Empty Interrupt Disable Mask */
#define SSC_IDR_TXEMPTY                     SSC_IDR_TXEMPTY_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use SSC_IDR_TXEMPTY_Msk instead */
#define SSC_IDR_RXRDY_Pos                   4                                              /**< (SSC_IDR) Receive Ready Interrupt Disable Position */
#define SSC_IDR_RXRDY_Msk                   (0x1U << SSC_IDR_RXRDY_Pos)                    /**< (SSC_IDR) Receive Ready Interrupt Disable Mask */
#define SSC_IDR_RXRDY                       SSC_IDR_RXRDY_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use SSC_IDR_RXRDY_Msk instead */
#define SSC_IDR_OVRUN_Pos                   5                                              /**< (SSC_IDR) Receive Overrun Interrupt Disable Position */
#define SSC_IDR_OVRUN_Msk                   (0x1U << SSC_IDR_OVRUN_Pos)                    /**< (SSC_IDR) Receive Overrun Interrupt Disable Mask */
#define SSC_IDR_OVRUN                       SSC_IDR_OVRUN_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use SSC_IDR_OVRUN_Msk instead */
#define SSC_IDR_CP0_Pos                     8                                              /**< (SSC_IDR) Compare 0 Interrupt Disable Position */
#define SSC_IDR_CP0_Msk                     (0x1U << SSC_IDR_CP0_Pos)                      /**< (SSC_IDR) Compare 0 Interrupt Disable Mask */
#define SSC_IDR_CP0                         SSC_IDR_CP0_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use SSC_IDR_CP0_Msk instead */
#define SSC_IDR_CP1_Pos                     9                                              /**< (SSC_IDR) Compare 1 Interrupt Disable Position */
#define SSC_IDR_CP1_Msk                     (0x1U << SSC_IDR_CP1_Pos)                      /**< (SSC_IDR) Compare 1 Interrupt Disable Mask */
#define SSC_IDR_CP1                         SSC_IDR_CP1_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use SSC_IDR_CP1_Msk instead */
#define SSC_IDR_TXSYN_Pos                   10                                             /**< (SSC_IDR) Tx Sync Interrupt Enable Position */
#define SSC_IDR_TXSYN_Msk                   (0x1U << SSC_IDR_TXSYN_Pos)                    /**< (SSC_IDR) Tx Sync Interrupt Enable Mask */
#define SSC_IDR_TXSYN                       SSC_IDR_TXSYN_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use SSC_IDR_TXSYN_Msk instead */
#define SSC_IDR_RXSYN_Pos                   11                                             /**< (SSC_IDR) Rx Sync Interrupt Enable Position */
#define SSC_IDR_RXSYN_Msk                   (0x1U << SSC_IDR_RXSYN_Pos)                    /**< (SSC_IDR) Rx Sync Interrupt Enable Mask */
#define SSC_IDR_RXSYN                       SSC_IDR_RXSYN_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use SSC_IDR_RXSYN_Msk instead */
#define SSC_IDR_MASK                        (0xF33U)                                       /**< \deprecated (SSC_IDR) Register MASK  (Use SSC_IDR_Msk instead)  */
#define SSC_IDR_Msk                         (0xF33U)                                       /**< (SSC_IDR) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- SSC_IMR : (SSC Offset: 0x4c) (R/ 32) Interrupt Mask Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t TXRDY:1;                   /**< bit:      0  Transmit Ready Interrupt Mask            */
    uint32_t TXEMPTY:1;                 /**< bit:      1  Transmit Empty Interrupt Mask            */
    uint32_t :2;                        /**< bit:   2..3  Reserved */
    uint32_t RXRDY:1;                   /**< bit:      4  Receive Ready Interrupt Mask             */
    uint32_t OVRUN:1;                   /**< bit:      5  Receive Overrun Interrupt Mask           */
    uint32_t :2;                        /**< bit:   6..7  Reserved */
    uint32_t CP0:1;                     /**< bit:      8  Compare 0 Interrupt Mask                 */
    uint32_t CP1:1;                     /**< bit:      9  Compare 1 Interrupt Mask                 */
    uint32_t TXSYN:1;                   /**< bit:     10  Tx Sync Interrupt Mask                   */
    uint32_t RXSYN:1;                   /**< bit:     11  Rx Sync Interrupt Mask                   */
    uint32_t :20;                       /**< bit: 12..31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} SSC_IMR_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define SSC_IMR_OFFSET                      (0x4C)                                        /**<  (SSC_IMR) Interrupt Mask Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define SSC_IMR_TXRDY_Pos                   0                                              /**< (SSC_IMR) Transmit Ready Interrupt Mask Position */
#define SSC_IMR_TXRDY_Msk                   (0x1U << SSC_IMR_TXRDY_Pos)                    /**< (SSC_IMR) Transmit Ready Interrupt Mask Mask */
#define SSC_IMR_TXRDY                       SSC_IMR_TXRDY_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use SSC_IMR_TXRDY_Msk instead */
#define SSC_IMR_TXEMPTY_Pos                 1                                              /**< (SSC_IMR) Transmit Empty Interrupt Mask Position */
#define SSC_IMR_TXEMPTY_Msk                 (0x1U << SSC_IMR_TXEMPTY_Pos)                  /**< (SSC_IMR) Transmit Empty Interrupt Mask Mask */
#define SSC_IMR_TXEMPTY                     SSC_IMR_TXEMPTY_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use SSC_IMR_TXEMPTY_Msk instead */
#define SSC_IMR_RXRDY_Pos                   4                                              /**< (SSC_IMR) Receive Ready Interrupt Mask Position */
#define SSC_IMR_RXRDY_Msk                   (0x1U << SSC_IMR_RXRDY_Pos)                    /**< (SSC_IMR) Receive Ready Interrupt Mask Mask */
#define SSC_IMR_RXRDY                       SSC_IMR_RXRDY_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use SSC_IMR_RXRDY_Msk instead */
#define SSC_IMR_OVRUN_Pos                   5                                              /**< (SSC_IMR) Receive Overrun Interrupt Mask Position */
#define SSC_IMR_OVRUN_Msk                   (0x1U << SSC_IMR_OVRUN_Pos)                    /**< (SSC_IMR) Receive Overrun Interrupt Mask Mask */
#define SSC_IMR_OVRUN                       SSC_IMR_OVRUN_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use SSC_IMR_OVRUN_Msk instead */
#define SSC_IMR_CP0_Pos                     8                                              /**< (SSC_IMR) Compare 0 Interrupt Mask Position */
#define SSC_IMR_CP0_Msk                     (0x1U << SSC_IMR_CP0_Pos)                      /**< (SSC_IMR) Compare 0 Interrupt Mask Mask */
#define SSC_IMR_CP0                         SSC_IMR_CP0_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use SSC_IMR_CP0_Msk instead */
#define SSC_IMR_CP1_Pos                     9                                              /**< (SSC_IMR) Compare 1 Interrupt Mask Position */
#define SSC_IMR_CP1_Msk                     (0x1U << SSC_IMR_CP1_Pos)                      /**< (SSC_IMR) Compare 1 Interrupt Mask Mask */
#define SSC_IMR_CP1                         SSC_IMR_CP1_Msk                                /**< \deprecated Old style mask definition for 1 bit bitfield. Use SSC_IMR_CP1_Msk instead */
#define SSC_IMR_TXSYN_Pos                   10                                             /**< (SSC_IMR) Tx Sync Interrupt Mask Position */
#define SSC_IMR_TXSYN_Msk                   (0x1U << SSC_IMR_TXSYN_Pos)                    /**< (SSC_IMR) Tx Sync Interrupt Mask Mask */
#define SSC_IMR_TXSYN                       SSC_IMR_TXSYN_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use SSC_IMR_TXSYN_Msk instead */
#define SSC_IMR_RXSYN_Pos                   11                                             /**< (SSC_IMR) Rx Sync Interrupt Mask Position */
#define SSC_IMR_RXSYN_Msk                   (0x1U << SSC_IMR_RXSYN_Pos)                    /**< (SSC_IMR) Rx Sync Interrupt Mask Mask */
#define SSC_IMR_RXSYN                       SSC_IMR_RXSYN_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use SSC_IMR_RXSYN_Msk instead */
#define SSC_IMR_MASK                        (0xF33U)                                       /**< \deprecated (SSC_IMR) Register MASK  (Use SSC_IMR_Msk instead)  */
#define SSC_IMR_Msk                         (0xF33U)                                       /**< (SSC_IMR) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- SSC_WPMR : (SSC Offset: 0xe4) (R/W 32) Write Protection Mode Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t WPEN:1;                    /**< bit:      0  Write Protection Enable                  */
    uint32_t :7;                        /**< bit:   1..7  Reserved */
    uint32_t WPKEY:24;                  /**< bit:  8..31  Write Protection Key                     */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} SSC_WPMR_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define SSC_WPMR_OFFSET                     (0xE4)                                        /**<  (SSC_WPMR) Write Protection Mode Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define SSC_WPMR_WPEN_Pos                   0                                              /**< (SSC_WPMR) Write Protection Enable Position */
#define SSC_WPMR_WPEN_Msk                   (0x1U << SSC_WPMR_WPEN_Pos)                    /**< (SSC_WPMR) Write Protection Enable Mask */
#define SSC_WPMR_WPEN                       SSC_WPMR_WPEN_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use SSC_WPMR_WPEN_Msk instead */
#define SSC_WPMR_WPKEY_Pos                  8                                              /**< (SSC_WPMR) Write Protection Key Position */
#define SSC_WPMR_WPKEY_Msk                  (0xFFFFFFU << SSC_WPMR_WPKEY_Pos)              /**< (SSC_WPMR) Write Protection Key Mask */
#define SSC_WPMR_WPKEY(value)               (SSC_WPMR_WPKEY_Msk & ((value) << SSC_WPMR_WPKEY_Pos))
#define   SSC_WPMR_WPKEY_PASSWD_Val         (0x535343U)                                    /**< (SSC_WPMR) Writing any other value in this field aborts the write operation of the WPEN bit.Always reads as 0.  */
#define SSC_WPMR_WPKEY_PASSWD               (SSC_WPMR_WPKEY_PASSWD_Val << SSC_WPMR_WPKEY_Pos)  /**< (SSC_WPMR) Writing any other value in this field aborts the write operation of the WPEN bit.Always reads as 0. Position  */
#define SSC_WPMR_MASK                       (0xFFFFFF01U)                                  /**< \deprecated (SSC_WPMR) Register MASK  (Use SSC_WPMR_Msk instead)  */
#define SSC_WPMR_Msk                        (0xFFFFFF01U)                                  /**< (SSC_WPMR) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- SSC_WPSR : (SSC Offset: 0xe8) (R/ 32) Write Protection Status Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t WPVS:1;                    /**< bit:      0  Write Protection Violation Status        */
    uint32_t :7;                        /**< bit:   1..7  Reserved */
    uint32_t WPVSRC:16;                 /**< bit:  8..23  Write Protect Violation Source           */
    uint32_t :8;                        /**< bit: 24..31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} SSC_WPSR_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define SSC_WPSR_OFFSET                     (0xE8)                                        /**<  (SSC_WPSR) Write Protection Status Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define SSC_WPSR_WPVS_Pos                   0                                              /**< (SSC_WPSR) Write Protection Violation Status Position */
#define SSC_WPSR_WPVS_Msk                   (0x1U << SSC_WPSR_WPVS_Pos)                    /**< (SSC_WPSR) Write Protection Violation Status Mask */
#define SSC_WPSR_WPVS                       SSC_WPSR_WPVS_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use SSC_WPSR_WPVS_Msk instead */
#define SSC_WPSR_WPVSRC_Pos                 8                                              /**< (SSC_WPSR) Write Protect Violation Source Position */
#define SSC_WPSR_WPVSRC_Msk                 (0xFFFFU << SSC_WPSR_WPVSRC_Pos)               /**< (SSC_WPSR) Write Protect Violation Source Mask */
#define SSC_WPSR_WPVSRC(value)              (SSC_WPSR_WPVSRC_Msk & ((value) << SSC_WPSR_WPVSRC_Pos))
#define SSC_WPSR_MASK                       (0xFFFF01U)                                    /**< \deprecated (SSC_WPSR) Register MASK  (Use SSC_WPSR_Msk instead)  */
#define SSC_WPSR_Msk                        (0xFFFF01U)                                    /**< (SSC_WPSR) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
#if COMPONENT_TYPEDEF_STYLE == 'R'
/** \brief SSC hardware registers */
typedef struct {  
  __O  uint32_t SSC_CR;         /**< (SSC Offset: 0x00) Control Register */
  __IO uint32_t SSC_CMR;        /**< (SSC Offset: 0x04) Clock Mode Register */
  __I  uint32_t Reserved1[2];
  __IO uint32_t SSC_RCMR;       /**< (SSC Offset: 0x10) Receive Clock Mode Register */
  __IO uint32_t SSC_RFMR;       /**< (SSC Offset: 0x14) Receive Frame Mode Register */
  __IO uint32_t SSC_TCMR;       /**< (SSC Offset: 0x18) Transmit Clock Mode Register */
  __IO uint32_t SSC_TFMR;       /**< (SSC Offset: 0x1C) Transmit Frame Mode Register */
  __I  uint32_t SSC_RHR;        /**< (SSC Offset: 0x20) Receive Holding Register */
  __O  uint32_t SSC_THR;        /**< (SSC Offset: 0x24) Transmit Holding Register */
  __I  uint32_t Reserved2[2];
  __I  uint32_t SSC_RSHR;       /**< (SSC Offset: 0x30) Receive Sync. Holding Register */
  __IO uint32_t SSC_TSHR;       /**< (SSC Offset: 0x34) Transmit Sync. Holding Register */
  __IO uint32_t SSC_RC0R;       /**< (SSC Offset: 0x38) Receive Compare 0 Register */
  __IO uint32_t SSC_RC1R;       /**< (SSC Offset: 0x3C) Receive Compare 1 Register */
  __I  uint32_t SSC_SR;         /**< (SSC Offset: 0x40) Status Register */
  __O  uint32_t SSC_IER;        /**< (SSC Offset: 0x44) Interrupt Enable Register */
  __O  uint32_t SSC_IDR;        /**< (SSC Offset: 0x48) Interrupt Disable Register */
  __I  uint32_t SSC_IMR;        /**< (SSC Offset: 0x4C) Interrupt Mask Register */
  __I  uint32_t Reserved3[37];
  __IO uint32_t SSC_WPMR;       /**< (SSC Offset: 0xE4) Write Protection Mode Register */
  __I  uint32_t SSC_WPSR;       /**< (SSC Offset: 0xE8) Write Protection Status Register */
} Ssc;

#elif COMPONENT_TYPEDEF_STYLE == 'N'
/** \brief SSC hardware registers */
typedef struct {  
  __O  SSC_CR_Type                    SSC_CR;         /**< Offset: 0x00 ( /W  32) Control Register */
  __IO SSC_CMR_Type                   SSC_CMR;        /**< Offset: 0x04 (R/W  32) Clock Mode Register */
       RoReg8                         Reserved1[0x8];
  __IO SSC_RCMR_Type                  SSC_RCMR;       /**< Offset: 0x10 (R/W  32) Receive Clock Mode Register */
  __IO SSC_RFMR_Type                  SSC_RFMR;       /**< Offset: 0x14 (R/W  32) Receive Frame Mode Register */
  __IO SSC_TCMR_Type                  SSC_TCMR;       /**< Offset: 0x18 (R/W  32) Transmit Clock Mode Register */
  __IO SSC_TFMR_Type                  SSC_TFMR;       /**< Offset: 0x1C (R/W  32) Transmit Frame Mode Register */
  __I  SSC_RHR_Type                   SSC_RHR;        /**< Offset: 0x20 (R/   32) Receive Holding Register */
  __O  SSC_THR_Type                   SSC_THR;        /**< Offset: 0x24 ( /W  32) Transmit Holding Register */
       RoReg8                         Reserved2[0x8];
  __I  SSC_RSHR_Type                  SSC_RSHR;       /**< Offset: 0x30 (R/   32) Receive Sync. Holding Register */
  __IO SSC_TSHR_Type                  SSC_TSHR;       /**< Offset: 0x34 (R/W  32) Transmit Sync. Holding Register */
  __IO SSC_RC0R_Type                  SSC_RC0R;       /**< Offset: 0x38 (R/W  32) Receive Compare 0 Register */
  __IO SSC_RC1R_Type                  SSC_RC1R;       /**< Offset: 0x3C (R/W  32) Receive Compare 1 Register */
  __I  SSC_SR_Type                    SSC_SR;         /**< Offset: 0x40 (R/   32) Status Register */
  __O  SSC_IER_Type                   SSC_IER;        /**< Offset: 0x44 ( /W  32) Interrupt Enable Register */
  __O  SSC_IDR_Type                   SSC_IDR;        /**< Offset: 0x48 ( /W  32) Interrupt Disable Register */
  __I  SSC_IMR_Type                   SSC_IMR;        /**< Offset: 0x4C (R/   32) Interrupt Mask Register */
       RoReg8                         Reserved3[0x94];
  __IO SSC_WPMR_Type                  SSC_WPMR;       /**< Offset: 0xE4 (R/W  32) Write Protection Mode Register */
  __I  SSC_WPSR_Type                  SSC_WPSR;       /**< Offset: 0xE8 (R/   32) Write Protection Status Register */
} Ssc;

#else /* COMPONENT_TYPEDEF_STYLE */
#error Unknown component typedef style
#endif /* COMPONENT_TYPEDEF_STYLE */

#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/** @}  end of Synchronous Serial Controller */

#endif /* _SAME70_SSC_COMPONENT_H_ */
