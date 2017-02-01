/**
 * \file
 *
 * \brief Component description for TWIHS
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

#ifndef _SAME70_TWIHS_COMPONENT_H_
#define _SAME70_TWIHS_COMPONENT_H_
#define _SAME70_TWIHS_COMPONENT_         /**< \deprecated  Backward compatibility for ASF */

/** \addtogroup SAME70_TWIHS Two-wire Interface High Speed
 *  @{
 */
/* ========================================================================== */
/**  SOFTWARE API DEFINITION FOR TWIHS */
/* ========================================================================== */
#ifndef COMPONENT_TYPEDEF_STYLE
  #define COMPONENT_TYPEDEF_STYLE 'R'  /**< Defines default style of typedefs for the component header files ('R' = RFO, 'N' = NTO)*/
#endif

#define TWIHS_11210                      /**< (TWIHS) Module ID */
#define REV_TWIHS S                      /**< (TWIHS) Module revision */

/* -------- TWIHS_CR : (TWIHS Offset: 0x00) (/W 32) Control Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t START:1;                   /**< bit:      0  Send a START Condition                   */
    uint32_t STOP:1;                    /**< bit:      1  Send a STOP Condition                    */
    uint32_t MSEN:1;                    /**< bit:      2  TWIHS Master Mode Enabled                */
    uint32_t MSDIS:1;                   /**< bit:      3  TWIHS Master Mode Disabled               */
    uint32_t SVEN:1;                    /**< bit:      4  TWIHS Slave Mode Enabled                 */
    uint32_t SVDIS:1;                   /**< bit:      5  TWIHS Slave Mode Disabled                */
    uint32_t QUICK:1;                   /**< bit:      6  SMBus Quick Command                      */
    uint32_t SWRST:1;                   /**< bit:      7  Software Reset                           */
    uint32_t HSEN:1;                    /**< bit:      8  TWIHS High-Speed Mode Enabled            */
    uint32_t HSDIS:1;                   /**< bit:      9  TWIHS High-Speed Mode Disabled           */
    uint32_t SMBEN:1;                   /**< bit:     10  SMBus Mode Enabled                       */
    uint32_t SMBDIS:1;                  /**< bit:     11  SMBus Mode Disabled                      */
    uint32_t PECEN:1;                   /**< bit:     12  Packet Error Checking Enable             */
    uint32_t PECDIS:1;                  /**< bit:     13  Packet Error Checking Disable            */
    uint32_t PECRQ:1;                   /**< bit:     14  PEC Request                              */
    uint32_t CLEAR:1;                   /**< bit:     15  Bus CLEAR Command                        */
    uint32_t ACMEN:1;                   /**< bit:     16  Alternative Command Mode Enable          */
    uint32_t ACMDIS:1;                  /**< bit:     17  Alternative Command Mode Disable         */
    uint32_t :6;                        /**< bit: 18..23  Reserved */
    uint32_t THRCLR:1;                  /**< bit:     24  Transmit Holding Register Clear          */
    uint32_t :1;                        /**< bit:     25  Reserved */
    uint32_t LOCKCLR:1;                 /**< bit:     26  Lock Clear                               */
    uint32_t :1;                        /**< bit:     27  Reserved */
    uint32_t FIFOEN:1;                  /**< bit:     28  FIFO Enable                              */
    uint32_t FIFODIS:1;                 /**< bit:     29  FIFO Disable                             */
    uint32_t :2;                        /**< bit: 30..31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} TWIHS_CR_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define TWIHS_CR_OFFSET                     (0x00)                                        /**<  (TWIHS_CR) Control Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define TWIHS_CR_START_Pos                  0                                              /**< (TWIHS_CR) Send a START Condition Position */
#define TWIHS_CR_START_Msk                  (0x1U << TWIHS_CR_START_Pos)                   /**< (TWIHS_CR) Send a START Condition Mask */
#define TWIHS_CR_START                      TWIHS_CR_START_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use TWIHS_CR_START_Msk instead */
#define TWIHS_CR_STOP_Pos                   1                                              /**< (TWIHS_CR) Send a STOP Condition Position */
#define TWIHS_CR_STOP_Msk                   (0x1U << TWIHS_CR_STOP_Pos)                    /**< (TWIHS_CR) Send a STOP Condition Mask */
#define TWIHS_CR_STOP                       TWIHS_CR_STOP_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use TWIHS_CR_STOP_Msk instead */
#define TWIHS_CR_MSEN_Pos                   2                                              /**< (TWIHS_CR) TWIHS Master Mode Enabled Position */
#define TWIHS_CR_MSEN_Msk                   (0x1U << TWIHS_CR_MSEN_Pos)                    /**< (TWIHS_CR) TWIHS Master Mode Enabled Mask */
#define TWIHS_CR_MSEN                       TWIHS_CR_MSEN_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use TWIHS_CR_MSEN_Msk instead */
#define TWIHS_CR_MSDIS_Pos                  3                                              /**< (TWIHS_CR) TWIHS Master Mode Disabled Position */
#define TWIHS_CR_MSDIS_Msk                  (0x1U << TWIHS_CR_MSDIS_Pos)                   /**< (TWIHS_CR) TWIHS Master Mode Disabled Mask */
#define TWIHS_CR_MSDIS                      TWIHS_CR_MSDIS_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use TWIHS_CR_MSDIS_Msk instead */
#define TWIHS_CR_SVEN_Pos                   4                                              /**< (TWIHS_CR) TWIHS Slave Mode Enabled Position */
#define TWIHS_CR_SVEN_Msk                   (0x1U << TWIHS_CR_SVEN_Pos)                    /**< (TWIHS_CR) TWIHS Slave Mode Enabled Mask */
#define TWIHS_CR_SVEN                       TWIHS_CR_SVEN_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use TWIHS_CR_SVEN_Msk instead */
#define TWIHS_CR_SVDIS_Pos                  5                                              /**< (TWIHS_CR) TWIHS Slave Mode Disabled Position */
#define TWIHS_CR_SVDIS_Msk                  (0x1U << TWIHS_CR_SVDIS_Pos)                   /**< (TWIHS_CR) TWIHS Slave Mode Disabled Mask */
#define TWIHS_CR_SVDIS                      TWIHS_CR_SVDIS_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use TWIHS_CR_SVDIS_Msk instead */
#define TWIHS_CR_QUICK_Pos                  6                                              /**< (TWIHS_CR) SMBus Quick Command Position */
#define TWIHS_CR_QUICK_Msk                  (0x1U << TWIHS_CR_QUICK_Pos)                   /**< (TWIHS_CR) SMBus Quick Command Mask */
#define TWIHS_CR_QUICK                      TWIHS_CR_QUICK_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use TWIHS_CR_QUICK_Msk instead */
#define TWIHS_CR_SWRST_Pos                  7                                              /**< (TWIHS_CR) Software Reset Position */
#define TWIHS_CR_SWRST_Msk                  (0x1U << TWIHS_CR_SWRST_Pos)                   /**< (TWIHS_CR) Software Reset Mask */
#define TWIHS_CR_SWRST                      TWIHS_CR_SWRST_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use TWIHS_CR_SWRST_Msk instead */
#define TWIHS_CR_HSEN_Pos                   8                                              /**< (TWIHS_CR) TWIHS High-Speed Mode Enabled Position */
#define TWIHS_CR_HSEN_Msk                   (0x1U << TWIHS_CR_HSEN_Pos)                    /**< (TWIHS_CR) TWIHS High-Speed Mode Enabled Mask */
#define TWIHS_CR_HSEN                       TWIHS_CR_HSEN_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use TWIHS_CR_HSEN_Msk instead */
#define TWIHS_CR_HSDIS_Pos                  9                                              /**< (TWIHS_CR) TWIHS High-Speed Mode Disabled Position */
#define TWIHS_CR_HSDIS_Msk                  (0x1U << TWIHS_CR_HSDIS_Pos)                   /**< (TWIHS_CR) TWIHS High-Speed Mode Disabled Mask */
#define TWIHS_CR_HSDIS                      TWIHS_CR_HSDIS_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use TWIHS_CR_HSDIS_Msk instead */
#define TWIHS_CR_SMBEN_Pos                  10                                             /**< (TWIHS_CR) SMBus Mode Enabled Position */
#define TWIHS_CR_SMBEN_Msk                  (0x1U << TWIHS_CR_SMBEN_Pos)                   /**< (TWIHS_CR) SMBus Mode Enabled Mask */
#define TWIHS_CR_SMBEN                      TWIHS_CR_SMBEN_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use TWIHS_CR_SMBEN_Msk instead */
#define TWIHS_CR_SMBDIS_Pos                 11                                             /**< (TWIHS_CR) SMBus Mode Disabled Position */
#define TWIHS_CR_SMBDIS_Msk                 (0x1U << TWIHS_CR_SMBDIS_Pos)                  /**< (TWIHS_CR) SMBus Mode Disabled Mask */
#define TWIHS_CR_SMBDIS                     TWIHS_CR_SMBDIS_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use TWIHS_CR_SMBDIS_Msk instead */
#define TWIHS_CR_PECEN_Pos                  12                                             /**< (TWIHS_CR) Packet Error Checking Enable Position */
#define TWIHS_CR_PECEN_Msk                  (0x1U << TWIHS_CR_PECEN_Pos)                   /**< (TWIHS_CR) Packet Error Checking Enable Mask */
#define TWIHS_CR_PECEN                      TWIHS_CR_PECEN_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use TWIHS_CR_PECEN_Msk instead */
#define TWIHS_CR_PECDIS_Pos                 13                                             /**< (TWIHS_CR) Packet Error Checking Disable Position */
#define TWIHS_CR_PECDIS_Msk                 (0x1U << TWIHS_CR_PECDIS_Pos)                  /**< (TWIHS_CR) Packet Error Checking Disable Mask */
#define TWIHS_CR_PECDIS                     TWIHS_CR_PECDIS_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use TWIHS_CR_PECDIS_Msk instead */
#define TWIHS_CR_PECRQ_Pos                  14                                             /**< (TWIHS_CR) PEC Request Position */
#define TWIHS_CR_PECRQ_Msk                  (0x1U << TWIHS_CR_PECRQ_Pos)                   /**< (TWIHS_CR) PEC Request Mask */
#define TWIHS_CR_PECRQ                      TWIHS_CR_PECRQ_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use TWIHS_CR_PECRQ_Msk instead */
#define TWIHS_CR_CLEAR_Pos                  15                                             /**< (TWIHS_CR) Bus CLEAR Command Position */
#define TWIHS_CR_CLEAR_Msk                  (0x1U << TWIHS_CR_CLEAR_Pos)                   /**< (TWIHS_CR) Bus CLEAR Command Mask */
#define TWIHS_CR_CLEAR                      TWIHS_CR_CLEAR_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use TWIHS_CR_CLEAR_Msk instead */
#define TWIHS_CR_ACMEN_Pos                  16                                             /**< (TWIHS_CR) Alternative Command Mode Enable Position */
#define TWIHS_CR_ACMEN_Msk                  (0x1U << TWIHS_CR_ACMEN_Pos)                   /**< (TWIHS_CR) Alternative Command Mode Enable Mask */
#define TWIHS_CR_ACMEN                      TWIHS_CR_ACMEN_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use TWIHS_CR_ACMEN_Msk instead */
#define TWIHS_CR_ACMDIS_Pos                 17                                             /**< (TWIHS_CR) Alternative Command Mode Disable Position */
#define TWIHS_CR_ACMDIS_Msk                 (0x1U << TWIHS_CR_ACMDIS_Pos)                  /**< (TWIHS_CR) Alternative Command Mode Disable Mask */
#define TWIHS_CR_ACMDIS                     TWIHS_CR_ACMDIS_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use TWIHS_CR_ACMDIS_Msk instead */
#define TWIHS_CR_THRCLR_Pos                 24                                             /**< (TWIHS_CR) Transmit Holding Register Clear Position */
#define TWIHS_CR_THRCLR_Msk                 (0x1U << TWIHS_CR_THRCLR_Pos)                  /**< (TWIHS_CR) Transmit Holding Register Clear Mask */
#define TWIHS_CR_THRCLR                     TWIHS_CR_THRCLR_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use TWIHS_CR_THRCLR_Msk instead */
#define TWIHS_CR_LOCKCLR_Pos                26                                             /**< (TWIHS_CR) Lock Clear Position */
#define TWIHS_CR_LOCKCLR_Msk                (0x1U << TWIHS_CR_LOCKCLR_Pos)                 /**< (TWIHS_CR) Lock Clear Mask */
#define TWIHS_CR_LOCKCLR                    TWIHS_CR_LOCKCLR_Msk                           /**< \deprecated Old style mask definition for 1 bit bitfield. Use TWIHS_CR_LOCKCLR_Msk instead */
#define TWIHS_CR_FIFOEN_Pos                 28                                             /**< (TWIHS_CR) FIFO Enable Position */
#define TWIHS_CR_FIFOEN_Msk                 (0x1U << TWIHS_CR_FIFOEN_Pos)                  /**< (TWIHS_CR) FIFO Enable Mask */
#define TWIHS_CR_FIFOEN                     TWIHS_CR_FIFOEN_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use TWIHS_CR_FIFOEN_Msk instead */
#define TWIHS_CR_FIFODIS_Pos                29                                             /**< (TWIHS_CR) FIFO Disable Position */
#define TWIHS_CR_FIFODIS_Msk                (0x1U << TWIHS_CR_FIFODIS_Pos)                 /**< (TWIHS_CR) FIFO Disable Mask */
#define TWIHS_CR_FIFODIS                    TWIHS_CR_FIFODIS_Msk                           /**< \deprecated Old style mask definition for 1 bit bitfield. Use TWIHS_CR_FIFODIS_Msk instead */
#define TWIHS_CR_MASK                       (0x3503FFFFU)                                  /**< \deprecated (TWIHS_CR) Register MASK  (Use TWIHS_CR_Msk instead)  */
#define TWIHS_CR_Msk                        (0x3503FFFFU)                                  /**< (TWIHS_CR) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- TWIHS_MMR : (TWIHS Offset: 0x04) (R/W 32) Master Mode Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t :8;                        /**< bit:   0..7  Reserved */
    uint32_t IADRSZ:2;                  /**< bit:   8..9  Internal Device Address Size             */
    uint32_t :2;                        /**< bit: 10..11  Reserved */
    uint32_t MREAD:1;                   /**< bit:     12  Master Read Direction                    */
    uint32_t :3;                        /**< bit: 13..15  Reserved */
    uint32_t DADR:7;                    /**< bit: 16..22  Device Address                           */
    uint32_t :9;                        /**< bit: 23..31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} TWIHS_MMR_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define TWIHS_MMR_OFFSET                    (0x04)                                        /**<  (TWIHS_MMR) Master Mode Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define TWIHS_MMR_IADRSZ_Pos                8                                              /**< (TWIHS_MMR) Internal Device Address Size Position */
#define TWIHS_MMR_IADRSZ_Msk                (0x3U << TWIHS_MMR_IADRSZ_Pos)                 /**< (TWIHS_MMR) Internal Device Address Size Mask */
#define TWIHS_MMR_IADRSZ(value)             (TWIHS_MMR_IADRSZ_Msk & ((value) << TWIHS_MMR_IADRSZ_Pos))
#define   TWIHS_MMR_IADRSZ_NONE_Val         (0x0U)                                         /**< (TWIHS_MMR) No internal device address  */
#define   TWIHS_MMR_IADRSZ_1_BYTE_Val       (0x1U)                                         /**< (TWIHS_MMR) One-byte internal device address  */
#define   TWIHS_MMR_IADRSZ_2_BYTE_Val       (0x2U)                                         /**< (TWIHS_MMR) Two-byte internal device address  */
#define   TWIHS_MMR_IADRSZ_3_BYTE_Val       (0x3U)                                         /**< (TWIHS_MMR) Three-byte internal device address  */
#define TWIHS_MMR_IADRSZ_NONE               (TWIHS_MMR_IADRSZ_NONE_Val << TWIHS_MMR_IADRSZ_Pos)  /**< (TWIHS_MMR) No internal device address Position  */
#define TWIHS_MMR_IADRSZ_1_BYTE             (TWIHS_MMR_IADRSZ_1_BYTE_Val << TWIHS_MMR_IADRSZ_Pos)  /**< (TWIHS_MMR) One-byte internal device address Position  */
#define TWIHS_MMR_IADRSZ_2_BYTE             (TWIHS_MMR_IADRSZ_2_BYTE_Val << TWIHS_MMR_IADRSZ_Pos)  /**< (TWIHS_MMR) Two-byte internal device address Position  */
#define TWIHS_MMR_IADRSZ_3_BYTE             (TWIHS_MMR_IADRSZ_3_BYTE_Val << TWIHS_MMR_IADRSZ_Pos)  /**< (TWIHS_MMR) Three-byte internal device address Position  */
#define TWIHS_MMR_MREAD_Pos                 12                                             /**< (TWIHS_MMR) Master Read Direction Position */
#define TWIHS_MMR_MREAD_Msk                 (0x1U << TWIHS_MMR_MREAD_Pos)                  /**< (TWIHS_MMR) Master Read Direction Mask */
#define TWIHS_MMR_MREAD                     TWIHS_MMR_MREAD_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use TWIHS_MMR_MREAD_Msk instead */
#define TWIHS_MMR_DADR_Pos                  16                                             /**< (TWIHS_MMR) Device Address Position */
#define TWIHS_MMR_DADR_Msk                  (0x7FU << TWIHS_MMR_DADR_Pos)                  /**< (TWIHS_MMR) Device Address Mask */
#define TWIHS_MMR_DADR(value)               (TWIHS_MMR_DADR_Msk & ((value) << TWIHS_MMR_DADR_Pos))
#define TWIHS_MMR_MASK                      (0x7F1300U)                                    /**< \deprecated (TWIHS_MMR) Register MASK  (Use TWIHS_MMR_Msk instead)  */
#define TWIHS_MMR_Msk                       (0x7F1300U)                                    /**< (TWIHS_MMR) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- TWIHS_SMR : (TWIHS Offset: 0x08) (R/W 32) Slave Mode Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t NACKEN:1;                  /**< bit:      0  Slave Receiver Data Phase NACK enable    */
    uint32_t :1;                        /**< bit:      1  Reserved */
    uint32_t SMDA:1;                    /**< bit:      2  SMBus Default Address                    */
    uint32_t SMHH:1;                    /**< bit:      3  SMBus Host Header                        */
    uint32_t :2;                        /**< bit:   4..5  Reserved */
    uint32_t SCLWSDIS:1;                /**< bit:      6  Clock Wait State Disable                 */
    uint32_t :1;                        /**< bit:      7  Reserved */
    uint32_t MASK:7;                    /**< bit:  8..14  Slave Address Mask                       */
    uint32_t :1;                        /**< bit:     15  Reserved */
    uint32_t SADR:7;                    /**< bit: 16..22  Slave Address                            */
    uint32_t :5;                        /**< bit: 23..27  Reserved */
    uint32_t SADR1EN:1;                 /**< bit:     28  Slave Address 1 Enable                   */
    uint32_t SADR2EN:1;                 /**< bit:     29  Slave Address 2 Enable                   */
    uint32_t SADR3EN:1;                 /**< bit:     30  Slave Address 3 Enable                   */
    uint32_t DATAMEN:1;                 /**< bit:     31  Data Matching Enable                     */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} TWIHS_SMR_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define TWIHS_SMR_OFFSET                    (0x08)                                        /**<  (TWIHS_SMR) Slave Mode Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define TWIHS_SMR_NACKEN_Pos                0                                              /**< (TWIHS_SMR) Slave Receiver Data Phase NACK enable Position */
#define TWIHS_SMR_NACKEN_Msk                (0x1U << TWIHS_SMR_NACKEN_Pos)                 /**< (TWIHS_SMR) Slave Receiver Data Phase NACK enable Mask */
#define TWIHS_SMR_NACKEN                    TWIHS_SMR_NACKEN_Msk                           /**< \deprecated Old style mask definition for 1 bit bitfield. Use TWIHS_SMR_NACKEN_Msk instead */
#define TWIHS_SMR_SMDA_Pos                  2                                              /**< (TWIHS_SMR) SMBus Default Address Position */
#define TWIHS_SMR_SMDA_Msk                  (0x1U << TWIHS_SMR_SMDA_Pos)                   /**< (TWIHS_SMR) SMBus Default Address Mask */
#define TWIHS_SMR_SMDA                      TWIHS_SMR_SMDA_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use TWIHS_SMR_SMDA_Msk instead */
#define TWIHS_SMR_SMHH_Pos                  3                                              /**< (TWIHS_SMR) SMBus Host Header Position */
#define TWIHS_SMR_SMHH_Msk                  (0x1U << TWIHS_SMR_SMHH_Pos)                   /**< (TWIHS_SMR) SMBus Host Header Mask */
#define TWIHS_SMR_SMHH                      TWIHS_SMR_SMHH_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use TWIHS_SMR_SMHH_Msk instead */
#define TWIHS_SMR_SCLWSDIS_Pos              6                                              /**< (TWIHS_SMR) Clock Wait State Disable Position */
#define TWIHS_SMR_SCLWSDIS_Msk              (0x1U << TWIHS_SMR_SCLWSDIS_Pos)               /**< (TWIHS_SMR) Clock Wait State Disable Mask */
#define TWIHS_SMR_SCLWSDIS                  TWIHS_SMR_SCLWSDIS_Msk                         /**< \deprecated Old style mask definition for 1 bit bitfield. Use TWIHS_SMR_SCLWSDIS_Msk instead */
#define TWIHS_SMR_MASK_Pos                  8                                              /**< (TWIHS_SMR) Slave Address Mask Position */
#define TWIHS_SMR_MASK_Msk                  (0x7FU << TWIHS_SMR_MASK_Pos)                  /**< (TWIHS_SMR) Slave Address Mask Mask */
#define TWIHS_SMR_MASK(value)               (TWIHS_SMR_MASK_Msk & ((value) << TWIHS_SMR_MASK_Pos))
#define TWIHS_SMR_SADR_Pos                  16                                             /**< (TWIHS_SMR) Slave Address Position */
#define TWIHS_SMR_SADR_Msk                  (0x7FU << TWIHS_SMR_SADR_Pos)                  /**< (TWIHS_SMR) Slave Address Mask */
#define TWIHS_SMR_SADR(value)               (TWIHS_SMR_SADR_Msk & ((value) << TWIHS_SMR_SADR_Pos))
#define TWIHS_SMR_SADR1EN_Pos               28                                             /**< (TWIHS_SMR) Slave Address 1 Enable Position */
#define TWIHS_SMR_SADR1EN_Msk               (0x1U << TWIHS_SMR_SADR1EN_Pos)                /**< (TWIHS_SMR) Slave Address 1 Enable Mask */
#define TWIHS_SMR_SADR1EN                   TWIHS_SMR_SADR1EN_Msk                          /**< \deprecated Old style mask definition for 1 bit bitfield. Use TWIHS_SMR_SADR1EN_Msk instead */
#define TWIHS_SMR_SADR2EN_Pos               29                                             /**< (TWIHS_SMR) Slave Address 2 Enable Position */
#define TWIHS_SMR_SADR2EN_Msk               (0x1U << TWIHS_SMR_SADR2EN_Pos)                /**< (TWIHS_SMR) Slave Address 2 Enable Mask */
#define TWIHS_SMR_SADR2EN                   TWIHS_SMR_SADR2EN_Msk                          /**< \deprecated Old style mask definition for 1 bit bitfield. Use TWIHS_SMR_SADR2EN_Msk instead */
#define TWIHS_SMR_SADR3EN_Pos               30                                             /**< (TWIHS_SMR) Slave Address 3 Enable Position */
#define TWIHS_SMR_SADR3EN_Msk               (0x1U << TWIHS_SMR_SADR3EN_Pos)                /**< (TWIHS_SMR) Slave Address 3 Enable Mask */
#define TWIHS_SMR_SADR3EN                   TWIHS_SMR_SADR3EN_Msk                          /**< \deprecated Old style mask definition for 1 bit bitfield. Use TWIHS_SMR_SADR3EN_Msk instead */
#define TWIHS_SMR_DATAMEN_Pos               31                                             /**< (TWIHS_SMR) Data Matching Enable Position */
#define TWIHS_SMR_DATAMEN_Msk               (0x1U << TWIHS_SMR_DATAMEN_Pos)                /**< (TWIHS_SMR) Data Matching Enable Mask */
#define TWIHS_SMR_DATAMEN                   TWIHS_SMR_DATAMEN_Msk                          /**< \deprecated Old style mask definition for 1 bit bitfield. Use TWIHS_SMR_DATAMEN_Msk instead */
#define TWIHS_SMR_Msk                       (0xF07F7F4DU)                                  /**< (TWIHS_SMR) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- TWIHS_IADR : (TWIHS Offset: 0x0c) (R/W 32) Internal Address Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t IADR:24;                   /**< bit:  0..23  Internal Address                         */
    uint32_t :8;                        /**< bit: 24..31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} TWIHS_IADR_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define TWIHS_IADR_OFFSET                   (0x0C)                                        /**<  (TWIHS_IADR) Internal Address Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define TWIHS_IADR_IADR_Pos                 0                                              /**< (TWIHS_IADR) Internal Address Position */
#define TWIHS_IADR_IADR_Msk                 (0xFFFFFFU << TWIHS_IADR_IADR_Pos)             /**< (TWIHS_IADR) Internal Address Mask */
#define TWIHS_IADR_IADR(value)              (TWIHS_IADR_IADR_Msk & ((value) << TWIHS_IADR_IADR_Pos))
#define TWIHS_IADR_MASK                     (0xFFFFFFU)                                    /**< \deprecated (TWIHS_IADR) Register MASK  (Use TWIHS_IADR_Msk instead)  */
#define TWIHS_IADR_Msk                      (0xFFFFFFU)                                    /**< (TWIHS_IADR) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- TWIHS_CWGR : (TWIHS Offset: 0x10) (R/W 32) Clock Waveform Generator Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t CLDIV:8;                   /**< bit:   0..7  Clock Low Divider                        */
    uint32_t CHDIV:8;                   /**< bit:  8..15  Clock High Divider                       */
    uint32_t CKDIV:3;                   /**< bit: 16..18  Clock Divider                            */
    uint32_t :1;                        /**< bit:     19  Reserved */
    uint32_t CKSRC:1;                   /**< bit:     20  Transfer Rate Clock Source               */
    uint32_t :3;                        /**< bit: 21..23  Reserved */
    uint32_t HOLD:5;                    /**< bit: 24..28  TWD Hold Time Versus TWCK Falling        */
    uint32_t :3;                        /**< bit: 29..31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} TWIHS_CWGR_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define TWIHS_CWGR_OFFSET                   (0x10)                                        /**<  (TWIHS_CWGR) Clock Waveform Generator Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define TWIHS_CWGR_CLDIV_Pos                0                                              /**< (TWIHS_CWGR) Clock Low Divider Position */
#define TWIHS_CWGR_CLDIV_Msk                (0xFFU << TWIHS_CWGR_CLDIV_Pos)                /**< (TWIHS_CWGR) Clock Low Divider Mask */
#define TWIHS_CWGR_CLDIV(value)             (TWIHS_CWGR_CLDIV_Msk & ((value) << TWIHS_CWGR_CLDIV_Pos))
#define TWIHS_CWGR_CHDIV_Pos                8                                              /**< (TWIHS_CWGR) Clock High Divider Position */
#define TWIHS_CWGR_CHDIV_Msk                (0xFFU << TWIHS_CWGR_CHDIV_Pos)                /**< (TWIHS_CWGR) Clock High Divider Mask */
#define TWIHS_CWGR_CHDIV(value)             (TWIHS_CWGR_CHDIV_Msk & ((value) << TWIHS_CWGR_CHDIV_Pos))
#define TWIHS_CWGR_CKDIV_Pos                16                                             /**< (TWIHS_CWGR) Clock Divider Position */
#define TWIHS_CWGR_CKDIV_Msk                (0x7U << TWIHS_CWGR_CKDIV_Pos)                 /**< (TWIHS_CWGR) Clock Divider Mask */
#define TWIHS_CWGR_CKDIV(value)             (TWIHS_CWGR_CKDIV_Msk & ((value) << TWIHS_CWGR_CKDIV_Pos))
#define TWIHS_CWGR_CKSRC_Pos                20                                             /**< (TWIHS_CWGR) Transfer Rate Clock Source Position */
#define TWIHS_CWGR_CKSRC_Msk                (0x1U << TWIHS_CWGR_CKSRC_Pos)                 /**< (TWIHS_CWGR) Transfer Rate Clock Source Mask */
#define TWIHS_CWGR_CKSRC                    TWIHS_CWGR_CKSRC_Msk                           /**< \deprecated Old style mask definition for 1 bit bitfield. Use TWIHS_CWGR_CKSRC_Msk instead */
#define TWIHS_CWGR_HOLD_Pos                 24                                             /**< (TWIHS_CWGR) TWD Hold Time Versus TWCK Falling Position */
#define TWIHS_CWGR_HOLD_Msk                 (0x1FU << TWIHS_CWGR_HOLD_Pos)                 /**< (TWIHS_CWGR) TWD Hold Time Versus TWCK Falling Mask */
#define TWIHS_CWGR_HOLD(value)              (TWIHS_CWGR_HOLD_Msk & ((value) << TWIHS_CWGR_HOLD_Pos))
#define TWIHS_CWGR_MASK                     (0x1F17FFFFU)                                  /**< \deprecated (TWIHS_CWGR) Register MASK  (Use TWIHS_CWGR_Msk instead)  */
#define TWIHS_CWGR_Msk                      (0x1F17FFFFU)                                  /**< (TWIHS_CWGR) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- TWIHS_SR : (TWIHS Offset: 0x20) (R/ 32) Status Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t TXCOMP:1;                  /**< bit:      0  Transmission Completed (cleared by writing TWIHS_THR) */
    uint32_t RXRDY:1;                   /**< bit:      1  Receive Holding Register Ready (cleared by reading TWIHS_RHR) */
    uint32_t TXRDY:1;                   /**< bit:      2  Transmit Holding Register Ready (cleared by writing TWIHS_THR) */
    uint32_t SVREAD:1;                  /**< bit:      3  Slave Read                               */
    uint32_t SVACC:1;                   /**< bit:      4  Slave Access                             */
    uint32_t GACC:1;                    /**< bit:      5  General Call Access (cleared on read)    */
    uint32_t OVRE:1;                    /**< bit:      6  Overrun Error (cleared on read)          */
    uint32_t UNRE:1;                    /**< bit:      7  Underrun Error (cleared on read)         */
    uint32_t NACK:1;                    /**< bit:      8  Not Acknowledged (cleared on read)       */
    uint32_t ARBLST:1;                  /**< bit:      9  Arbitration Lost (cleared on read)       */
    uint32_t SCLWS:1;                   /**< bit:     10  Clock Wait State                         */
    uint32_t EOSACC:1;                  /**< bit:     11  End Of Slave Access (cleared on read)    */
    uint32_t :4;                        /**< bit: 12..15  Reserved */
    uint32_t MCACK:1;                   /**< bit:     16  Master Code Acknowledge (cleared on read) */
    uint32_t :1;                        /**< bit:     17  Reserved */
    uint32_t TOUT:1;                    /**< bit:     18  Timeout Error (cleared on read)          */
    uint32_t PECERR:1;                  /**< bit:     19  PEC Error (cleared on read)              */
    uint32_t SMBDAM:1;                  /**< bit:     20  SMBus Default Address Match (cleared on read) */
    uint32_t SMBHHM:1;                  /**< bit:     21  SMBus Host Header Address Match (cleared on read) */
    uint32_t :2;                        /**< bit: 22..23  Reserved */
    uint32_t SCL:1;                     /**< bit:     24  SCL Line Value                           */
    uint32_t SDA:1;                     /**< bit:     25  SDA Line Value                           */
    uint32_t :6;                        /**< bit: 26..31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} TWIHS_SR_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define TWIHS_SR_OFFSET                     (0x20)                                        /**<  (TWIHS_SR) Status Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define TWIHS_SR_TXCOMP_Pos                 0                                              /**< (TWIHS_SR) Transmission Completed (cleared by writing TWIHS_THR) Position */
#define TWIHS_SR_TXCOMP_Msk                 (0x1U << TWIHS_SR_TXCOMP_Pos)                  /**< (TWIHS_SR) Transmission Completed (cleared by writing TWIHS_THR) Mask */
#define TWIHS_SR_TXCOMP                     TWIHS_SR_TXCOMP_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use TWIHS_SR_TXCOMP_Msk instead */
#define TWIHS_SR_RXRDY_Pos                  1                                              /**< (TWIHS_SR) Receive Holding Register Ready (cleared by reading TWIHS_RHR) Position */
#define TWIHS_SR_RXRDY_Msk                  (0x1U << TWIHS_SR_RXRDY_Pos)                   /**< (TWIHS_SR) Receive Holding Register Ready (cleared by reading TWIHS_RHR) Mask */
#define TWIHS_SR_RXRDY                      TWIHS_SR_RXRDY_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use TWIHS_SR_RXRDY_Msk instead */
#define TWIHS_SR_TXRDY_Pos                  2                                              /**< (TWIHS_SR) Transmit Holding Register Ready (cleared by writing TWIHS_THR) Position */
#define TWIHS_SR_TXRDY_Msk                  (0x1U << TWIHS_SR_TXRDY_Pos)                   /**< (TWIHS_SR) Transmit Holding Register Ready (cleared by writing TWIHS_THR) Mask */
#define TWIHS_SR_TXRDY                      TWIHS_SR_TXRDY_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use TWIHS_SR_TXRDY_Msk instead */
#define TWIHS_SR_SVREAD_Pos                 3                                              /**< (TWIHS_SR) Slave Read Position */
#define TWIHS_SR_SVREAD_Msk                 (0x1U << TWIHS_SR_SVREAD_Pos)                  /**< (TWIHS_SR) Slave Read Mask */
#define TWIHS_SR_SVREAD                     TWIHS_SR_SVREAD_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use TWIHS_SR_SVREAD_Msk instead */
#define TWIHS_SR_SVACC_Pos                  4                                              /**< (TWIHS_SR) Slave Access Position */
#define TWIHS_SR_SVACC_Msk                  (0x1U << TWIHS_SR_SVACC_Pos)                   /**< (TWIHS_SR) Slave Access Mask */
#define TWIHS_SR_SVACC                      TWIHS_SR_SVACC_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use TWIHS_SR_SVACC_Msk instead */
#define TWIHS_SR_GACC_Pos                   5                                              /**< (TWIHS_SR) General Call Access (cleared on read) Position */
#define TWIHS_SR_GACC_Msk                   (0x1U << TWIHS_SR_GACC_Pos)                    /**< (TWIHS_SR) General Call Access (cleared on read) Mask */
#define TWIHS_SR_GACC                       TWIHS_SR_GACC_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use TWIHS_SR_GACC_Msk instead */
#define TWIHS_SR_OVRE_Pos                   6                                              /**< (TWIHS_SR) Overrun Error (cleared on read) Position */
#define TWIHS_SR_OVRE_Msk                   (0x1U << TWIHS_SR_OVRE_Pos)                    /**< (TWIHS_SR) Overrun Error (cleared on read) Mask */
#define TWIHS_SR_OVRE                       TWIHS_SR_OVRE_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use TWIHS_SR_OVRE_Msk instead */
#define TWIHS_SR_UNRE_Pos                   7                                              /**< (TWIHS_SR) Underrun Error (cleared on read) Position */
#define TWIHS_SR_UNRE_Msk                   (0x1U << TWIHS_SR_UNRE_Pos)                    /**< (TWIHS_SR) Underrun Error (cleared on read) Mask */
#define TWIHS_SR_UNRE                       TWIHS_SR_UNRE_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use TWIHS_SR_UNRE_Msk instead */
#define TWIHS_SR_NACK_Pos                   8                                              /**< (TWIHS_SR) Not Acknowledged (cleared on read) Position */
#define TWIHS_SR_NACK_Msk                   (0x1U << TWIHS_SR_NACK_Pos)                    /**< (TWIHS_SR) Not Acknowledged (cleared on read) Mask */
#define TWIHS_SR_NACK                       TWIHS_SR_NACK_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use TWIHS_SR_NACK_Msk instead */
#define TWIHS_SR_ARBLST_Pos                 9                                              /**< (TWIHS_SR) Arbitration Lost (cleared on read) Position */
#define TWIHS_SR_ARBLST_Msk                 (0x1U << TWIHS_SR_ARBLST_Pos)                  /**< (TWIHS_SR) Arbitration Lost (cleared on read) Mask */
#define TWIHS_SR_ARBLST                     TWIHS_SR_ARBLST_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use TWIHS_SR_ARBLST_Msk instead */
#define TWIHS_SR_SCLWS_Pos                  10                                             /**< (TWIHS_SR) Clock Wait State Position */
#define TWIHS_SR_SCLWS_Msk                  (0x1U << TWIHS_SR_SCLWS_Pos)                   /**< (TWIHS_SR) Clock Wait State Mask */
#define TWIHS_SR_SCLWS                      TWIHS_SR_SCLWS_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use TWIHS_SR_SCLWS_Msk instead */
#define TWIHS_SR_EOSACC_Pos                 11                                             /**< (TWIHS_SR) End Of Slave Access (cleared on read) Position */
#define TWIHS_SR_EOSACC_Msk                 (0x1U << TWIHS_SR_EOSACC_Pos)                  /**< (TWIHS_SR) End Of Slave Access (cleared on read) Mask */
#define TWIHS_SR_EOSACC                     TWIHS_SR_EOSACC_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use TWIHS_SR_EOSACC_Msk instead */
#define TWIHS_SR_MCACK_Pos                  16                                             /**< (TWIHS_SR) Master Code Acknowledge (cleared on read) Position */
#define TWIHS_SR_MCACK_Msk                  (0x1U << TWIHS_SR_MCACK_Pos)                   /**< (TWIHS_SR) Master Code Acknowledge (cleared on read) Mask */
#define TWIHS_SR_MCACK                      TWIHS_SR_MCACK_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use TWIHS_SR_MCACK_Msk instead */
#define TWIHS_SR_TOUT_Pos                   18                                             /**< (TWIHS_SR) Timeout Error (cleared on read) Position */
#define TWIHS_SR_TOUT_Msk                   (0x1U << TWIHS_SR_TOUT_Pos)                    /**< (TWIHS_SR) Timeout Error (cleared on read) Mask */
#define TWIHS_SR_TOUT                       TWIHS_SR_TOUT_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use TWIHS_SR_TOUT_Msk instead */
#define TWIHS_SR_PECERR_Pos                 19                                             /**< (TWIHS_SR) PEC Error (cleared on read) Position */
#define TWIHS_SR_PECERR_Msk                 (0x1U << TWIHS_SR_PECERR_Pos)                  /**< (TWIHS_SR) PEC Error (cleared on read) Mask */
#define TWIHS_SR_PECERR                     TWIHS_SR_PECERR_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use TWIHS_SR_PECERR_Msk instead */
#define TWIHS_SR_SMBDAM_Pos                 20                                             /**< (TWIHS_SR) SMBus Default Address Match (cleared on read) Position */
#define TWIHS_SR_SMBDAM_Msk                 (0x1U << TWIHS_SR_SMBDAM_Pos)                  /**< (TWIHS_SR) SMBus Default Address Match (cleared on read) Mask */
#define TWIHS_SR_SMBDAM                     TWIHS_SR_SMBDAM_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use TWIHS_SR_SMBDAM_Msk instead */
#define TWIHS_SR_SMBHHM_Pos                 21                                             /**< (TWIHS_SR) SMBus Host Header Address Match (cleared on read) Position */
#define TWIHS_SR_SMBHHM_Msk                 (0x1U << TWIHS_SR_SMBHHM_Pos)                  /**< (TWIHS_SR) SMBus Host Header Address Match (cleared on read) Mask */
#define TWIHS_SR_SMBHHM                     TWIHS_SR_SMBHHM_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use TWIHS_SR_SMBHHM_Msk instead */
#define TWIHS_SR_SCL_Pos                    24                                             /**< (TWIHS_SR) SCL Line Value Position */
#define TWIHS_SR_SCL_Msk                    (0x1U << TWIHS_SR_SCL_Pos)                     /**< (TWIHS_SR) SCL Line Value Mask */
#define TWIHS_SR_SCL                        TWIHS_SR_SCL_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use TWIHS_SR_SCL_Msk instead */
#define TWIHS_SR_SDA_Pos                    25                                             /**< (TWIHS_SR) SDA Line Value Position */
#define TWIHS_SR_SDA_Msk                    (0x1U << TWIHS_SR_SDA_Pos)                     /**< (TWIHS_SR) SDA Line Value Mask */
#define TWIHS_SR_SDA                        TWIHS_SR_SDA_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use TWIHS_SR_SDA_Msk instead */
#define TWIHS_SR_MASK                       (0x33D0FFFU)                                   /**< \deprecated (TWIHS_SR) Register MASK  (Use TWIHS_SR_Msk instead)  */
#define TWIHS_SR_Msk                        (0x33D0FFFU)                                   /**< (TWIHS_SR) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- TWIHS_IER : (TWIHS Offset: 0x24) (/W 32) Interrupt Enable Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t TXCOMP:1;                  /**< bit:      0  Transmission Completed Interrupt Enable  */
    uint32_t RXRDY:1;                   /**< bit:      1  Receive Holding Register Ready Interrupt Enable */
    uint32_t TXRDY:1;                   /**< bit:      2  Transmit Holding Register Ready Interrupt Enable */
    uint32_t :1;                        /**< bit:      3  Reserved */
    uint32_t SVACC:1;                   /**< bit:      4  Slave Access Interrupt Enable            */
    uint32_t GACC:1;                    /**< bit:      5  General Call Access Interrupt Enable     */
    uint32_t OVRE:1;                    /**< bit:      6  Overrun Error Interrupt Enable           */
    uint32_t UNRE:1;                    /**< bit:      7  Underrun Error Interrupt Enable          */
    uint32_t NACK:1;                    /**< bit:      8  Not Acknowledge Interrupt Enable         */
    uint32_t ARBLST:1;                  /**< bit:      9  Arbitration Lost Interrupt Enable        */
    uint32_t SCL_WS:1;                  /**< bit:     10  Clock Wait State Interrupt Enable        */
    uint32_t EOSACC:1;                  /**< bit:     11  End Of Slave Access Interrupt Enable     */
    uint32_t :4;                        /**< bit: 12..15  Reserved */
    uint32_t MCACK:1;                   /**< bit:     16  Master Code Acknowledge Interrupt Enable */
    uint32_t :1;                        /**< bit:     17  Reserved */
    uint32_t TOUT:1;                    /**< bit:     18  Timeout Error Interrupt Enable           */
    uint32_t PECERR:1;                  /**< bit:     19  PEC Error Interrupt Enable               */
    uint32_t SMBDAM:1;                  /**< bit:     20  SMBus Default Address Match Interrupt Enable */
    uint32_t SMBHHM:1;                  /**< bit:     21  SMBus Host Header Address Match Interrupt Enable */
    uint32_t :10;                       /**< bit: 22..31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} TWIHS_IER_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define TWIHS_IER_OFFSET                    (0x24)                                        /**<  (TWIHS_IER) Interrupt Enable Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define TWIHS_IER_TXCOMP_Pos                0                                              /**< (TWIHS_IER) Transmission Completed Interrupt Enable Position */
#define TWIHS_IER_TXCOMP_Msk                (0x1U << TWIHS_IER_TXCOMP_Pos)                 /**< (TWIHS_IER) Transmission Completed Interrupt Enable Mask */
#define TWIHS_IER_TXCOMP                    TWIHS_IER_TXCOMP_Msk                           /**< \deprecated Old style mask definition for 1 bit bitfield. Use TWIHS_IER_TXCOMP_Msk instead */
#define TWIHS_IER_RXRDY_Pos                 1                                              /**< (TWIHS_IER) Receive Holding Register Ready Interrupt Enable Position */
#define TWIHS_IER_RXRDY_Msk                 (0x1U << TWIHS_IER_RXRDY_Pos)                  /**< (TWIHS_IER) Receive Holding Register Ready Interrupt Enable Mask */
#define TWIHS_IER_RXRDY                     TWIHS_IER_RXRDY_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use TWIHS_IER_RXRDY_Msk instead */
#define TWIHS_IER_TXRDY_Pos                 2                                              /**< (TWIHS_IER) Transmit Holding Register Ready Interrupt Enable Position */
#define TWIHS_IER_TXRDY_Msk                 (0x1U << TWIHS_IER_TXRDY_Pos)                  /**< (TWIHS_IER) Transmit Holding Register Ready Interrupt Enable Mask */
#define TWIHS_IER_TXRDY                     TWIHS_IER_TXRDY_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use TWIHS_IER_TXRDY_Msk instead */
#define TWIHS_IER_SVACC_Pos                 4                                              /**< (TWIHS_IER) Slave Access Interrupt Enable Position */
#define TWIHS_IER_SVACC_Msk                 (0x1U << TWIHS_IER_SVACC_Pos)                  /**< (TWIHS_IER) Slave Access Interrupt Enable Mask */
#define TWIHS_IER_SVACC                     TWIHS_IER_SVACC_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use TWIHS_IER_SVACC_Msk instead */
#define TWIHS_IER_GACC_Pos                  5                                              /**< (TWIHS_IER) General Call Access Interrupt Enable Position */
#define TWIHS_IER_GACC_Msk                  (0x1U << TWIHS_IER_GACC_Pos)                   /**< (TWIHS_IER) General Call Access Interrupt Enable Mask */
#define TWIHS_IER_GACC                      TWIHS_IER_GACC_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use TWIHS_IER_GACC_Msk instead */
#define TWIHS_IER_OVRE_Pos                  6                                              /**< (TWIHS_IER) Overrun Error Interrupt Enable Position */
#define TWIHS_IER_OVRE_Msk                  (0x1U << TWIHS_IER_OVRE_Pos)                   /**< (TWIHS_IER) Overrun Error Interrupt Enable Mask */
#define TWIHS_IER_OVRE                      TWIHS_IER_OVRE_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use TWIHS_IER_OVRE_Msk instead */
#define TWIHS_IER_UNRE_Pos                  7                                              /**< (TWIHS_IER) Underrun Error Interrupt Enable Position */
#define TWIHS_IER_UNRE_Msk                  (0x1U << TWIHS_IER_UNRE_Pos)                   /**< (TWIHS_IER) Underrun Error Interrupt Enable Mask */
#define TWIHS_IER_UNRE                      TWIHS_IER_UNRE_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use TWIHS_IER_UNRE_Msk instead */
#define TWIHS_IER_NACK_Pos                  8                                              /**< (TWIHS_IER) Not Acknowledge Interrupt Enable Position */
#define TWIHS_IER_NACK_Msk                  (0x1U << TWIHS_IER_NACK_Pos)                   /**< (TWIHS_IER) Not Acknowledge Interrupt Enable Mask */
#define TWIHS_IER_NACK                      TWIHS_IER_NACK_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use TWIHS_IER_NACK_Msk instead */
#define TWIHS_IER_ARBLST_Pos                9                                              /**< (TWIHS_IER) Arbitration Lost Interrupt Enable Position */
#define TWIHS_IER_ARBLST_Msk                (0x1U << TWIHS_IER_ARBLST_Pos)                 /**< (TWIHS_IER) Arbitration Lost Interrupt Enable Mask */
#define TWIHS_IER_ARBLST                    TWIHS_IER_ARBLST_Msk                           /**< \deprecated Old style mask definition for 1 bit bitfield. Use TWIHS_IER_ARBLST_Msk instead */
#define TWIHS_IER_SCL_WS_Pos                10                                             /**< (TWIHS_IER) Clock Wait State Interrupt Enable Position */
#define TWIHS_IER_SCL_WS_Msk                (0x1U << TWIHS_IER_SCL_WS_Pos)                 /**< (TWIHS_IER) Clock Wait State Interrupt Enable Mask */
#define TWIHS_IER_SCL_WS                    TWIHS_IER_SCL_WS_Msk                           /**< \deprecated Old style mask definition for 1 bit bitfield. Use TWIHS_IER_SCL_WS_Msk instead */
#define TWIHS_IER_EOSACC_Pos                11                                             /**< (TWIHS_IER) End Of Slave Access Interrupt Enable Position */
#define TWIHS_IER_EOSACC_Msk                (0x1U << TWIHS_IER_EOSACC_Pos)                 /**< (TWIHS_IER) End Of Slave Access Interrupt Enable Mask */
#define TWIHS_IER_EOSACC                    TWIHS_IER_EOSACC_Msk                           /**< \deprecated Old style mask definition for 1 bit bitfield. Use TWIHS_IER_EOSACC_Msk instead */
#define TWIHS_IER_MCACK_Pos                 16                                             /**< (TWIHS_IER) Master Code Acknowledge Interrupt Enable Position */
#define TWIHS_IER_MCACK_Msk                 (0x1U << TWIHS_IER_MCACK_Pos)                  /**< (TWIHS_IER) Master Code Acknowledge Interrupt Enable Mask */
#define TWIHS_IER_MCACK                     TWIHS_IER_MCACK_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use TWIHS_IER_MCACK_Msk instead */
#define TWIHS_IER_TOUT_Pos                  18                                             /**< (TWIHS_IER) Timeout Error Interrupt Enable Position */
#define TWIHS_IER_TOUT_Msk                  (0x1U << TWIHS_IER_TOUT_Pos)                   /**< (TWIHS_IER) Timeout Error Interrupt Enable Mask */
#define TWIHS_IER_TOUT                      TWIHS_IER_TOUT_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use TWIHS_IER_TOUT_Msk instead */
#define TWIHS_IER_PECERR_Pos                19                                             /**< (TWIHS_IER) PEC Error Interrupt Enable Position */
#define TWIHS_IER_PECERR_Msk                (0x1U << TWIHS_IER_PECERR_Pos)                 /**< (TWIHS_IER) PEC Error Interrupt Enable Mask */
#define TWIHS_IER_PECERR                    TWIHS_IER_PECERR_Msk                           /**< \deprecated Old style mask definition for 1 bit bitfield. Use TWIHS_IER_PECERR_Msk instead */
#define TWIHS_IER_SMBDAM_Pos                20                                             /**< (TWIHS_IER) SMBus Default Address Match Interrupt Enable Position */
#define TWIHS_IER_SMBDAM_Msk                (0x1U << TWIHS_IER_SMBDAM_Pos)                 /**< (TWIHS_IER) SMBus Default Address Match Interrupt Enable Mask */
#define TWIHS_IER_SMBDAM                    TWIHS_IER_SMBDAM_Msk                           /**< \deprecated Old style mask definition for 1 bit bitfield. Use TWIHS_IER_SMBDAM_Msk instead */
#define TWIHS_IER_SMBHHM_Pos                21                                             /**< (TWIHS_IER) SMBus Host Header Address Match Interrupt Enable Position */
#define TWIHS_IER_SMBHHM_Msk                (0x1U << TWIHS_IER_SMBHHM_Pos)                 /**< (TWIHS_IER) SMBus Host Header Address Match Interrupt Enable Mask */
#define TWIHS_IER_SMBHHM                    TWIHS_IER_SMBHHM_Msk                           /**< \deprecated Old style mask definition for 1 bit bitfield. Use TWIHS_IER_SMBHHM_Msk instead */
#define TWIHS_IER_MASK                      (0x3D0FF7U)                                    /**< \deprecated (TWIHS_IER) Register MASK  (Use TWIHS_IER_Msk instead)  */
#define TWIHS_IER_Msk                       (0x3D0FF7U)                                    /**< (TWIHS_IER) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- TWIHS_IDR : (TWIHS Offset: 0x28) (/W 32) Interrupt Disable Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t TXCOMP:1;                  /**< bit:      0  Transmission Completed Interrupt Disable */
    uint32_t RXRDY:1;                   /**< bit:      1  Receive Holding Register Ready Interrupt Disable */
    uint32_t TXRDY:1;                   /**< bit:      2  Transmit Holding Register Ready Interrupt Disable */
    uint32_t :1;                        /**< bit:      3  Reserved */
    uint32_t SVACC:1;                   /**< bit:      4  Slave Access Interrupt Disable           */
    uint32_t GACC:1;                    /**< bit:      5  General Call Access Interrupt Disable    */
    uint32_t OVRE:1;                    /**< bit:      6  Overrun Error Interrupt Disable          */
    uint32_t UNRE:1;                    /**< bit:      7  Underrun Error Interrupt Disable         */
    uint32_t NACK:1;                    /**< bit:      8  Not Acknowledge Interrupt Disable        */
    uint32_t ARBLST:1;                  /**< bit:      9  Arbitration Lost Interrupt Disable       */
    uint32_t SCL_WS:1;                  /**< bit:     10  Clock Wait State Interrupt Disable       */
    uint32_t EOSACC:1;                  /**< bit:     11  End Of Slave Access Interrupt Disable    */
    uint32_t :4;                        /**< bit: 12..15  Reserved */
    uint32_t MCACK:1;                   /**< bit:     16  Master Code Acknowledge Interrupt Disable */
    uint32_t :1;                        /**< bit:     17  Reserved */
    uint32_t TOUT:1;                    /**< bit:     18  Timeout Error Interrupt Disable          */
    uint32_t PECERR:1;                  /**< bit:     19  PEC Error Interrupt Disable              */
    uint32_t SMBDAM:1;                  /**< bit:     20  SMBus Default Address Match Interrupt Disable */
    uint32_t SMBHHM:1;                  /**< bit:     21  SMBus Host Header Address Match Interrupt Disable */
    uint32_t :10;                       /**< bit: 22..31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} TWIHS_IDR_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define TWIHS_IDR_OFFSET                    (0x28)                                        /**<  (TWIHS_IDR) Interrupt Disable Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define TWIHS_IDR_TXCOMP_Pos                0                                              /**< (TWIHS_IDR) Transmission Completed Interrupt Disable Position */
#define TWIHS_IDR_TXCOMP_Msk                (0x1U << TWIHS_IDR_TXCOMP_Pos)                 /**< (TWIHS_IDR) Transmission Completed Interrupt Disable Mask */
#define TWIHS_IDR_TXCOMP                    TWIHS_IDR_TXCOMP_Msk                           /**< \deprecated Old style mask definition for 1 bit bitfield. Use TWIHS_IDR_TXCOMP_Msk instead */
#define TWIHS_IDR_RXRDY_Pos                 1                                              /**< (TWIHS_IDR) Receive Holding Register Ready Interrupt Disable Position */
#define TWIHS_IDR_RXRDY_Msk                 (0x1U << TWIHS_IDR_RXRDY_Pos)                  /**< (TWIHS_IDR) Receive Holding Register Ready Interrupt Disable Mask */
#define TWIHS_IDR_RXRDY                     TWIHS_IDR_RXRDY_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use TWIHS_IDR_RXRDY_Msk instead */
#define TWIHS_IDR_TXRDY_Pos                 2                                              /**< (TWIHS_IDR) Transmit Holding Register Ready Interrupt Disable Position */
#define TWIHS_IDR_TXRDY_Msk                 (0x1U << TWIHS_IDR_TXRDY_Pos)                  /**< (TWIHS_IDR) Transmit Holding Register Ready Interrupt Disable Mask */
#define TWIHS_IDR_TXRDY                     TWIHS_IDR_TXRDY_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use TWIHS_IDR_TXRDY_Msk instead */
#define TWIHS_IDR_SVACC_Pos                 4                                              /**< (TWIHS_IDR) Slave Access Interrupt Disable Position */
#define TWIHS_IDR_SVACC_Msk                 (0x1U << TWIHS_IDR_SVACC_Pos)                  /**< (TWIHS_IDR) Slave Access Interrupt Disable Mask */
#define TWIHS_IDR_SVACC                     TWIHS_IDR_SVACC_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use TWIHS_IDR_SVACC_Msk instead */
#define TWIHS_IDR_GACC_Pos                  5                                              /**< (TWIHS_IDR) General Call Access Interrupt Disable Position */
#define TWIHS_IDR_GACC_Msk                  (0x1U << TWIHS_IDR_GACC_Pos)                   /**< (TWIHS_IDR) General Call Access Interrupt Disable Mask */
#define TWIHS_IDR_GACC                      TWIHS_IDR_GACC_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use TWIHS_IDR_GACC_Msk instead */
#define TWIHS_IDR_OVRE_Pos                  6                                              /**< (TWIHS_IDR) Overrun Error Interrupt Disable Position */
#define TWIHS_IDR_OVRE_Msk                  (0x1U << TWIHS_IDR_OVRE_Pos)                   /**< (TWIHS_IDR) Overrun Error Interrupt Disable Mask */
#define TWIHS_IDR_OVRE                      TWIHS_IDR_OVRE_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use TWIHS_IDR_OVRE_Msk instead */
#define TWIHS_IDR_UNRE_Pos                  7                                              /**< (TWIHS_IDR) Underrun Error Interrupt Disable Position */
#define TWIHS_IDR_UNRE_Msk                  (0x1U << TWIHS_IDR_UNRE_Pos)                   /**< (TWIHS_IDR) Underrun Error Interrupt Disable Mask */
#define TWIHS_IDR_UNRE                      TWIHS_IDR_UNRE_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use TWIHS_IDR_UNRE_Msk instead */
#define TWIHS_IDR_NACK_Pos                  8                                              /**< (TWIHS_IDR) Not Acknowledge Interrupt Disable Position */
#define TWIHS_IDR_NACK_Msk                  (0x1U << TWIHS_IDR_NACK_Pos)                   /**< (TWIHS_IDR) Not Acknowledge Interrupt Disable Mask */
#define TWIHS_IDR_NACK                      TWIHS_IDR_NACK_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use TWIHS_IDR_NACK_Msk instead */
#define TWIHS_IDR_ARBLST_Pos                9                                              /**< (TWIHS_IDR) Arbitration Lost Interrupt Disable Position */
#define TWIHS_IDR_ARBLST_Msk                (0x1U << TWIHS_IDR_ARBLST_Pos)                 /**< (TWIHS_IDR) Arbitration Lost Interrupt Disable Mask */
#define TWIHS_IDR_ARBLST                    TWIHS_IDR_ARBLST_Msk                           /**< \deprecated Old style mask definition for 1 bit bitfield. Use TWIHS_IDR_ARBLST_Msk instead */
#define TWIHS_IDR_SCL_WS_Pos                10                                             /**< (TWIHS_IDR) Clock Wait State Interrupt Disable Position */
#define TWIHS_IDR_SCL_WS_Msk                (0x1U << TWIHS_IDR_SCL_WS_Pos)                 /**< (TWIHS_IDR) Clock Wait State Interrupt Disable Mask */
#define TWIHS_IDR_SCL_WS                    TWIHS_IDR_SCL_WS_Msk                           /**< \deprecated Old style mask definition for 1 bit bitfield. Use TWIHS_IDR_SCL_WS_Msk instead */
#define TWIHS_IDR_EOSACC_Pos                11                                             /**< (TWIHS_IDR) End Of Slave Access Interrupt Disable Position */
#define TWIHS_IDR_EOSACC_Msk                (0x1U << TWIHS_IDR_EOSACC_Pos)                 /**< (TWIHS_IDR) End Of Slave Access Interrupt Disable Mask */
#define TWIHS_IDR_EOSACC                    TWIHS_IDR_EOSACC_Msk                           /**< \deprecated Old style mask definition for 1 bit bitfield. Use TWIHS_IDR_EOSACC_Msk instead */
#define TWIHS_IDR_MCACK_Pos                 16                                             /**< (TWIHS_IDR) Master Code Acknowledge Interrupt Disable Position */
#define TWIHS_IDR_MCACK_Msk                 (0x1U << TWIHS_IDR_MCACK_Pos)                  /**< (TWIHS_IDR) Master Code Acknowledge Interrupt Disable Mask */
#define TWIHS_IDR_MCACK                     TWIHS_IDR_MCACK_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use TWIHS_IDR_MCACK_Msk instead */
#define TWIHS_IDR_TOUT_Pos                  18                                             /**< (TWIHS_IDR) Timeout Error Interrupt Disable Position */
#define TWIHS_IDR_TOUT_Msk                  (0x1U << TWIHS_IDR_TOUT_Pos)                   /**< (TWIHS_IDR) Timeout Error Interrupt Disable Mask */
#define TWIHS_IDR_TOUT                      TWIHS_IDR_TOUT_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use TWIHS_IDR_TOUT_Msk instead */
#define TWIHS_IDR_PECERR_Pos                19                                             /**< (TWIHS_IDR) PEC Error Interrupt Disable Position */
#define TWIHS_IDR_PECERR_Msk                (0x1U << TWIHS_IDR_PECERR_Pos)                 /**< (TWIHS_IDR) PEC Error Interrupt Disable Mask */
#define TWIHS_IDR_PECERR                    TWIHS_IDR_PECERR_Msk                           /**< \deprecated Old style mask definition for 1 bit bitfield. Use TWIHS_IDR_PECERR_Msk instead */
#define TWIHS_IDR_SMBDAM_Pos                20                                             /**< (TWIHS_IDR) SMBus Default Address Match Interrupt Disable Position */
#define TWIHS_IDR_SMBDAM_Msk                (0x1U << TWIHS_IDR_SMBDAM_Pos)                 /**< (TWIHS_IDR) SMBus Default Address Match Interrupt Disable Mask */
#define TWIHS_IDR_SMBDAM                    TWIHS_IDR_SMBDAM_Msk                           /**< \deprecated Old style mask definition for 1 bit bitfield. Use TWIHS_IDR_SMBDAM_Msk instead */
#define TWIHS_IDR_SMBHHM_Pos                21                                             /**< (TWIHS_IDR) SMBus Host Header Address Match Interrupt Disable Position */
#define TWIHS_IDR_SMBHHM_Msk                (0x1U << TWIHS_IDR_SMBHHM_Pos)                 /**< (TWIHS_IDR) SMBus Host Header Address Match Interrupt Disable Mask */
#define TWIHS_IDR_SMBHHM                    TWIHS_IDR_SMBHHM_Msk                           /**< \deprecated Old style mask definition for 1 bit bitfield. Use TWIHS_IDR_SMBHHM_Msk instead */
#define TWIHS_IDR_MASK                      (0x3D0FF7U)                                    /**< \deprecated (TWIHS_IDR) Register MASK  (Use TWIHS_IDR_Msk instead)  */
#define TWIHS_IDR_Msk                       (0x3D0FF7U)                                    /**< (TWIHS_IDR) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- TWIHS_IMR : (TWIHS Offset: 0x2c) (R/ 32) Interrupt Mask Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t TXCOMP:1;                  /**< bit:      0  Transmission Completed Interrupt Mask    */
    uint32_t RXRDY:1;                   /**< bit:      1  Receive Holding Register Ready Interrupt Mask */
    uint32_t TXRDY:1;                   /**< bit:      2  Transmit Holding Register Ready Interrupt Mask */
    uint32_t :1;                        /**< bit:      3  Reserved */
    uint32_t SVACC:1;                   /**< bit:      4  Slave Access Interrupt Mask              */
    uint32_t GACC:1;                    /**< bit:      5  General Call Access Interrupt Mask       */
    uint32_t OVRE:1;                    /**< bit:      6  Overrun Error Interrupt Mask             */
    uint32_t UNRE:1;                    /**< bit:      7  Underrun Error Interrupt Mask            */
    uint32_t NACK:1;                    /**< bit:      8  Not Acknowledge Interrupt Mask           */
    uint32_t ARBLST:1;                  /**< bit:      9  Arbitration Lost Interrupt Mask          */
    uint32_t SCL_WS:1;                  /**< bit:     10  Clock Wait State Interrupt Mask          */
    uint32_t EOSACC:1;                  /**< bit:     11  End Of Slave Access Interrupt Mask       */
    uint32_t :4;                        /**< bit: 12..15  Reserved */
    uint32_t MCACK:1;                   /**< bit:     16  Master Code Acknowledge Interrupt Mask   */
    uint32_t :1;                        /**< bit:     17  Reserved */
    uint32_t TOUT:1;                    /**< bit:     18  Timeout Error Interrupt Mask             */
    uint32_t PECERR:1;                  /**< bit:     19  PEC Error Interrupt Mask                 */
    uint32_t SMBDAM:1;                  /**< bit:     20  SMBus Default Address Match Interrupt Mask */
    uint32_t SMBHHM:1;                  /**< bit:     21  SMBus Host Header Address Match Interrupt Mask */
    uint32_t :10;                       /**< bit: 22..31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} TWIHS_IMR_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define TWIHS_IMR_OFFSET                    (0x2C)                                        /**<  (TWIHS_IMR) Interrupt Mask Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define TWIHS_IMR_TXCOMP_Pos                0                                              /**< (TWIHS_IMR) Transmission Completed Interrupt Mask Position */
#define TWIHS_IMR_TXCOMP_Msk                (0x1U << TWIHS_IMR_TXCOMP_Pos)                 /**< (TWIHS_IMR) Transmission Completed Interrupt Mask Mask */
#define TWIHS_IMR_TXCOMP                    TWIHS_IMR_TXCOMP_Msk                           /**< \deprecated Old style mask definition for 1 bit bitfield. Use TWIHS_IMR_TXCOMP_Msk instead */
#define TWIHS_IMR_RXRDY_Pos                 1                                              /**< (TWIHS_IMR) Receive Holding Register Ready Interrupt Mask Position */
#define TWIHS_IMR_RXRDY_Msk                 (0x1U << TWIHS_IMR_RXRDY_Pos)                  /**< (TWIHS_IMR) Receive Holding Register Ready Interrupt Mask Mask */
#define TWIHS_IMR_RXRDY                     TWIHS_IMR_RXRDY_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use TWIHS_IMR_RXRDY_Msk instead */
#define TWIHS_IMR_TXRDY_Pos                 2                                              /**< (TWIHS_IMR) Transmit Holding Register Ready Interrupt Mask Position */
#define TWIHS_IMR_TXRDY_Msk                 (0x1U << TWIHS_IMR_TXRDY_Pos)                  /**< (TWIHS_IMR) Transmit Holding Register Ready Interrupt Mask Mask */
#define TWIHS_IMR_TXRDY                     TWIHS_IMR_TXRDY_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use TWIHS_IMR_TXRDY_Msk instead */
#define TWIHS_IMR_SVACC_Pos                 4                                              /**< (TWIHS_IMR) Slave Access Interrupt Mask Position */
#define TWIHS_IMR_SVACC_Msk                 (0x1U << TWIHS_IMR_SVACC_Pos)                  /**< (TWIHS_IMR) Slave Access Interrupt Mask Mask */
#define TWIHS_IMR_SVACC                     TWIHS_IMR_SVACC_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use TWIHS_IMR_SVACC_Msk instead */
#define TWIHS_IMR_GACC_Pos                  5                                              /**< (TWIHS_IMR) General Call Access Interrupt Mask Position */
#define TWIHS_IMR_GACC_Msk                  (0x1U << TWIHS_IMR_GACC_Pos)                   /**< (TWIHS_IMR) General Call Access Interrupt Mask Mask */
#define TWIHS_IMR_GACC                      TWIHS_IMR_GACC_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use TWIHS_IMR_GACC_Msk instead */
#define TWIHS_IMR_OVRE_Pos                  6                                              /**< (TWIHS_IMR) Overrun Error Interrupt Mask Position */
#define TWIHS_IMR_OVRE_Msk                  (0x1U << TWIHS_IMR_OVRE_Pos)                   /**< (TWIHS_IMR) Overrun Error Interrupt Mask Mask */
#define TWIHS_IMR_OVRE                      TWIHS_IMR_OVRE_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use TWIHS_IMR_OVRE_Msk instead */
#define TWIHS_IMR_UNRE_Pos                  7                                              /**< (TWIHS_IMR) Underrun Error Interrupt Mask Position */
#define TWIHS_IMR_UNRE_Msk                  (0x1U << TWIHS_IMR_UNRE_Pos)                   /**< (TWIHS_IMR) Underrun Error Interrupt Mask Mask */
#define TWIHS_IMR_UNRE                      TWIHS_IMR_UNRE_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use TWIHS_IMR_UNRE_Msk instead */
#define TWIHS_IMR_NACK_Pos                  8                                              /**< (TWIHS_IMR) Not Acknowledge Interrupt Mask Position */
#define TWIHS_IMR_NACK_Msk                  (0x1U << TWIHS_IMR_NACK_Pos)                   /**< (TWIHS_IMR) Not Acknowledge Interrupt Mask Mask */
#define TWIHS_IMR_NACK                      TWIHS_IMR_NACK_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use TWIHS_IMR_NACK_Msk instead */
#define TWIHS_IMR_ARBLST_Pos                9                                              /**< (TWIHS_IMR) Arbitration Lost Interrupt Mask Position */
#define TWIHS_IMR_ARBLST_Msk                (0x1U << TWIHS_IMR_ARBLST_Pos)                 /**< (TWIHS_IMR) Arbitration Lost Interrupt Mask Mask */
#define TWIHS_IMR_ARBLST                    TWIHS_IMR_ARBLST_Msk                           /**< \deprecated Old style mask definition for 1 bit bitfield. Use TWIHS_IMR_ARBLST_Msk instead */
#define TWIHS_IMR_SCL_WS_Pos                10                                             /**< (TWIHS_IMR) Clock Wait State Interrupt Mask Position */
#define TWIHS_IMR_SCL_WS_Msk                (0x1U << TWIHS_IMR_SCL_WS_Pos)                 /**< (TWIHS_IMR) Clock Wait State Interrupt Mask Mask */
#define TWIHS_IMR_SCL_WS                    TWIHS_IMR_SCL_WS_Msk                           /**< \deprecated Old style mask definition for 1 bit bitfield. Use TWIHS_IMR_SCL_WS_Msk instead */
#define TWIHS_IMR_EOSACC_Pos                11                                             /**< (TWIHS_IMR) End Of Slave Access Interrupt Mask Position */
#define TWIHS_IMR_EOSACC_Msk                (0x1U << TWIHS_IMR_EOSACC_Pos)                 /**< (TWIHS_IMR) End Of Slave Access Interrupt Mask Mask */
#define TWIHS_IMR_EOSACC                    TWIHS_IMR_EOSACC_Msk                           /**< \deprecated Old style mask definition for 1 bit bitfield. Use TWIHS_IMR_EOSACC_Msk instead */
#define TWIHS_IMR_MCACK_Pos                 16                                             /**< (TWIHS_IMR) Master Code Acknowledge Interrupt Mask Position */
#define TWIHS_IMR_MCACK_Msk                 (0x1U << TWIHS_IMR_MCACK_Pos)                  /**< (TWIHS_IMR) Master Code Acknowledge Interrupt Mask Mask */
#define TWIHS_IMR_MCACK                     TWIHS_IMR_MCACK_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use TWIHS_IMR_MCACK_Msk instead */
#define TWIHS_IMR_TOUT_Pos                  18                                             /**< (TWIHS_IMR) Timeout Error Interrupt Mask Position */
#define TWIHS_IMR_TOUT_Msk                  (0x1U << TWIHS_IMR_TOUT_Pos)                   /**< (TWIHS_IMR) Timeout Error Interrupt Mask Mask */
#define TWIHS_IMR_TOUT                      TWIHS_IMR_TOUT_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use TWIHS_IMR_TOUT_Msk instead */
#define TWIHS_IMR_PECERR_Pos                19                                             /**< (TWIHS_IMR) PEC Error Interrupt Mask Position */
#define TWIHS_IMR_PECERR_Msk                (0x1U << TWIHS_IMR_PECERR_Pos)                 /**< (TWIHS_IMR) PEC Error Interrupt Mask Mask */
#define TWIHS_IMR_PECERR                    TWIHS_IMR_PECERR_Msk                           /**< \deprecated Old style mask definition for 1 bit bitfield. Use TWIHS_IMR_PECERR_Msk instead */
#define TWIHS_IMR_SMBDAM_Pos                20                                             /**< (TWIHS_IMR) SMBus Default Address Match Interrupt Mask Position */
#define TWIHS_IMR_SMBDAM_Msk                (0x1U << TWIHS_IMR_SMBDAM_Pos)                 /**< (TWIHS_IMR) SMBus Default Address Match Interrupt Mask Mask */
#define TWIHS_IMR_SMBDAM                    TWIHS_IMR_SMBDAM_Msk                           /**< \deprecated Old style mask definition for 1 bit bitfield. Use TWIHS_IMR_SMBDAM_Msk instead */
#define TWIHS_IMR_SMBHHM_Pos                21                                             /**< (TWIHS_IMR) SMBus Host Header Address Match Interrupt Mask Position */
#define TWIHS_IMR_SMBHHM_Msk                (0x1U << TWIHS_IMR_SMBHHM_Pos)                 /**< (TWIHS_IMR) SMBus Host Header Address Match Interrupt Mask Mask */
#define TWIHS_IMR_SMBHHM                    TWIHS_IMR_SMBHHM_Msk                           /**< \deprecated Old style mask definition for 1 bit bitfield. Use TWIHS_IMR_SMBHHM_Msk instead */
#define TWIHS_IMR_MASK                      (0x3D0FF7U)                                    /**< \deprecated (TWIHS_IMR) Register MASK  (Use TWIHS_IMR_Msk instead)  */
#define TWIHS_IMR_Msk                       (0x3D0FF7U)                                    /**< (TWIHS_IMR) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- TWIHS_RHR : (TWIHS Offset: 0x30) (R/ 32) Receive Holding Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t RXDATA:8;                  /**< bit:   0..7  Master or Slave Receive Holding Data     */
    uint32_t :24;                       /**< bit:  8..31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} TWIHS_RHR_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define TWIHS_RHR_OFFSET                    (0x30)                                        /**<  (TWIHS_RHR) Receive Holding Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define TWIHS_RHR_RXDATA_Pos                0                                              /**< (TWIHS_RHR) Master or Slave Receive Holding Data Position */
#define TWIHS_RHR_RXDATA_Msk                (0xFFU << TWIHS_RHR_RXDATA_Pos)                /**< (TWIHS_RHR) Master or Slave Receive Holding Data Mask */
#define TWIHS_RHR_RXDATA(value)             (TWIHS_RHR_RXDATA_Msk & ((value) << TWIHS_RHR_RXDATA_Pos))
#define TWIHS_RHR_MASK                      (0xFFU)                                        /**< \deprecated (TWIHS_RHR) Register MASK  (Use TWIHS_RHR_Msk instead)  */
#define TWIHS_RHR_Msk                       (0xFFU)                                        /**< (TWIHS_RHR) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- TWIHS_THR : (TWIHS Offset: 0x34) (/W 32) Transmit Holding Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t TXDATA:8;                  /**< bit:   0..7  Master or Slave Transmit Holding Data    */
    uint32_t :24;                       /**< bit:  8..31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} TWIHS_THR_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define TWIHS_THR_OFFSET                    (0x34)                                        /**<  (TWIHS_THR) Transmit Holding Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define TWIHS_THR_TXDATA_Pos                0                                              /**< (TWIHS_THR) Master or Slave Transmit Holding Data Position */
#define TWIHS_THR_TXDATA_Msk                (0xFFU << TWIHS_THR_TXDATA_Pos)                /**< (TWIHS_THR) Master or Slave Transmit Holding Data Mask */
#define TWIHS_THR_TXDATA(value)             (TWIHS_THR_TXDATA_Msk & ((value) << TWIHS_THR_TXDATA_Pos))
#define TWIHS_THR_MASK                      (0xFFU)                                        /**< \deprecated (TWIHS_THR) Register MASK  (Use TWIHS_THR_Msk instead)  */
#define TWIHS_THR_Msk                       (0xFFU)                                        /**< (TWIHS_THR) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- TWIHS_SMBTR : (TWIHS Offset: 0x38) (R/W 32) SMBus Timing Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t PRESC:4;                   /**< bit:   0..3  SMBus Clock Prescaler                    */
    uint32_t :4;                        /**< bit:   4..7  Reserved */
    uint32_t TLOWS:8;                   /**< bit:  8..15  Slave Clock Stretch Maximum Cycles       */
    uint32_t TLOWM:8;                   /**< bit: 16..23  Master Clock Stretch Maximum Cycles      */
    uint32_t THMAX:8;                   /**< bit: 24..31  Clock High Maximum Cycles                */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} TWIHS_SMBTR_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define TWIHS_SMBTR_OFFSET                  (0x38)                                        /**<  (TWIHS_SMBTR) SMBus Timing Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define TWIHS_SMBTR_PRESC_Pos               0                                              /**< (TWIHS_SMBTR) SMBus Clock Prescaler Position */
#define TWIHS_SMBTR_PRESC_Msk               (0xFU << TWIHS_SMBTR_PRESC_Pos)                /**< (TWIHS_SMBTR) SMBus Clock Prescaler Mask */
#define TWIHS_SMBTR_PRESC(value)            (TWIHS_SMBTR_PRESC_Msk & ((value) << TWIHS_SMBTR_PRESC_Pos))
#define TWIHS_SMBTR_TLOWS_Pos               8                                              /**< (TWIHS_SMBTR) Slave Clock Stretch Maximum Cycles Position */
#define TWIHS_SMBTR_TLOWS_Msk               (0xFFU << TWIHS_SMBTR_TLOWS_Pos)               /**< (TWIHS_SMBTR) Slave Clock Stretch Maximum Cycles Mask */
#define TWIHS_SMBTR_TLOWS(value)            (TWIHS_SMBTR_TLOWS_Msk & ((value) << TWIHS_SMBTR_TLOWS_Pos))
#define TWIHS_SMBTR_TLOWM_Pos               16                                             /**< (TWIHS_SMBTR) Master Clock Stretch Maximum Cycles Position */
#define TWIHS_SMBTR_TLOWM_Msk               (0xFFU << TWIHS_SMBTR_TLOWM_Pos)               /**< (TWIHS_SMBTR) Master Clock Stretch Maximum Cycles Mask */
#define TWIHS_SMBTR_TLOWM(value)            (TWIHS_SMBTR_TLOWM_Msk & ((value) << TWIHS_SMBTR_TLOWM_Pos))
#define TWIHS_SMBTR_THMAX_Pos               24                                             /**< (TWIHS_SMBTR) Clock High Maximum Cycles Position */
#define TWIHS_SMBTR_THMAX_Msk               (0xFFU << TWIHS_SMBTR_THMAX_Pos)               /**< (TWIHS_SMBTR) Clock High Maximum Cycles Mask */
#define TWIHS_SMBTR_THMAX(value)            (TWIHS_SMBTR_THMAX_Msk & ((value) << TWIHS_SMBTR_THMAX_Pos))
#define TWIHS_SMBTR_MASK                    (0xFFFFFF0FU)                                  /**< \deprecated (TWIHS_SMBTR) Register MASK  (Use TWIHS_SMBTR_Msk instead)  */
#define TWIHS_SMBTR_Msk                     (0xFFFFFF0FU)                                  /**< (TWIHS_SMBTR) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- TWIHS_FILTR : (TWIHS Offset: 0x44) (R/W 32) Filter Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t FILT:1;                    /**< bit:      0  RX Digital Filter                        */
    uint32_t PADFEN:1;                  /**< bit:      1  PAD Filter Enable                        */
    uint32_t PADFCFG:1;                 /**< bit:      2  PAD Filter Config                        */
    uint32_t :5;                        /**< bit:   3..7  Reserved */
    uint32_t THRES:3;                   /**< bit:  8..10  Digital Filter Threshold                 */
    uint32_t :21;                       /**< bit: 11..31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} TWIHS_FILTR_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define TWIHS_FILTR_OFFSET                  (0x44)                                        /**<  (TWIHS_FILTR) Filter Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define TWIHS_FILTR_FILT_Pos                0                                              /**< (TWIHS_FILTR) RX Digital Filter Position */
#define TWIHS_FILTR_FILT_Msk                (0x1U << TWIHS_FILTR_FILT_Pos)                 /**< (TWIHS_FILTR) RX Digital Filter Mask */
#define TWIHS_FILTR_FILT                    TWIHS_FILTR_FILT_Msk                           /**< \deprecated Old style mask definition for 1 bit bitfield. Use TWIHS_FILTR_FILT_Msk instead */
#define TWIHS_FILTR_PADFEN_Pos              1                                              /**< (TWIHS_FILTR) PAD Filter Enable Position */
#define TWIHS_FILTR_PADFEN_Msk              (0x1U << TWIHS_FILTR_PADFEN_Pos)               /**< (TWIHS_FILTR) PAD Filter Enable Mask */
#define TWIHS_FILTR_PADFEN                  TWIHS_FILTR_PADFEN_Msk                         /**< \deprecated Old style mask definition for 1 bit bitfield. Use TWIHS_FILTR_PADFEN_Msk instead */
#define TWIHS_FILTR_PADFCFG_Pos             2                                              /**< (TWIHS_FILTR) PAD Filter Config Position */
#define TWIHS_FILTR_PADFCFG_Msk             (0x1U << TWIHS_FILTR_PADFCFG_Pos)              /**< (TWIHS_FILTR) PAD Filter Config Mask */
#define TWIHS_FILTR_PADFCFG                 TWIHS_FILTR_PADFCFG_Msk                        /**< \deprecated Old style mask definition for 1 bit bitfield. Use TWIHS_FILTR_PADFCFG_Msk instead */
#define TWIHS_FILTR_THRES_Pos               8                                              /**< (TWIHS_FILTR) Digital Filter Threshold Position */
#define TWIHS_FILTR_THRES_Msk               (0x7U << TWIHS_FILTR_THRES_Pos)                /**< (TWIHS_FILTR) Digital Filter Threshold Mask */
#define TWIHS_FILTR_THRES(value)            (TWIHS_FILTR_THRES_Msk & ((value) << TWIHS_FILTR_THRES_Pos))
#define TWIHS_FILTR_MASK                    (0x707U)                                       /**< \deprecated (TWIHS_FILTR) Register MASK  (Use TWIHS_FILTR_Msk instead)  */
#define TWIHS_FILTR_Msk                     (0x707U)                                       /**< (TWIHS_FILTR) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- TWIHS_SWMR : (TWIHS Offset: 0x4c) (R/W 32) SleepWalking Matching Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t SADR1:7;                   /**< bit:   0..6  Slave Address 1                          */
    uint32_t :1;                        /**< bit:      7  Reserved */
    uint32_t SADR2:7;                   /**< bit:  8..14  Slave Address 2                          */
    uint32_t :1;                        /**< bit:     15  Reserved */
    uint32_t SADR3:7;                   /**< bit: 16..22  Slave Address 3                          */
    uint32_t :1;                        /**< bit:     23  Reserved */
    uint32_t DATAM:8;                   /**< bit: 24..31  Data Match                               */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} TWIHS_SWMR_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define TWIHS_SWMR_OFFSET                   (0x4C)                                        /**<  (TWIHS_SWMR) SleepWalking Matching Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define TWIHS_SWMR_SADR1_Pos                0                                              /**< (TWIHS_SWMR) Slave Address 1 Position */
#define TWIHS_SWMR_SADR1_Msk                (0x7FU << TWIHS_SWMR_SADR1_Pos)                /**< (TWIHS_SWMR) Slave Address 1 Mask */
#define TWIHS_SWMR_SADR1(value)             (TWIHS_SWMR_SADR1_Msk & ((value) << TWIHS_SWMR_SADR1_Pos))
#define TWIHS_SWMR_SADR2_Pos                8                                              /**< (TWIHS_SWMR) Slave Address 2 Position */
#define TWIHS_SWMR_SADR2_Msk                (0x7FU << TWIHS_SWMR_SADR2_Pos)                /**< (TWIHS_SWMR) Slave Address 2 Mask */
#define TWIHS_SWMR_SADR2(value)             (TWIHS_SWMR_SADR2_Msk & ((value) << TWIHS_SWMR_SADR2_Pos))
#define TWIHS_SWMR_SADR3_Pos                16                                             /**< (TWIHS_SWMR) Slave Address 3 Position */
#define TWIHS_SWMR_SADR3_Msk                (0x7FU << TWIHS_SWMR_SADR3_Pos)                /**< (TWIHS_SWMR) Slave Address 3 Mask */
#define TWIHS_SWMR_SADR3(value)             (TWIHS_SWMR_SADR3_Msk & ((value) << TWIHS_SWMR_SADR3_Pos))
#define TWIHS_SWMR_DATAM_Pos                24                                             /**< (TWIHS_SWMR) Data Match Position */
#define TWIHS_SWMR_DATAM_Msk                (0xFFU << TWIHS_SWMR_DATAM_Pos)                /**< (TWIHS_SWMR) Data Match Mask */
#define TWIHS_SWMR_DATAM(value)             (TWIHS_SWMR_DATAM_Msk & ((value) << TWIHS_SWMR_DATAM_Pos))
#define TWIHS_SWMR_MASK                     (0xFF7F7F7FU)                                  /**< \deprecated (TWIHS_SWMR) Register MASK  (Use TWIHS_SWMR_Msk instead)  */
#define TWIHS_SWMR_Msk                      (0xFF7F7F7FU)                                  /**< (TWIHS_SWMR) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- TWIHS_WPMR : (TWIHS Offset: 0xe4) (R/W 32) Write Protection Mode Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t WPEN:1;                    /**< bit:      0  Write Protection Enable                  */
    uint32_t :7;                        /**< bit:   1..7  Reserved */
    uint32_t WPKEY:24;                  /**< bit:  8..31  Write Protection Key                     */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} TWIHS_WPMR_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define TWIHS_WPMR_OFFSET                   (0xE4)                                        /**<  (TWIHS_WPMR) Write Protection Mode Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define TWIHS_WPMR_WPEN_Pos                 0                                              /**< (TWIHS_WPMR) Write Protection Enable Position */
#define TWIHS_WPMR_WPEN_Msk                 (0x1U << TWIHS_WPMR_WPEN_Pos)                  /**< (TWIHS_WPMR) Write Protection Enable Mask */
#define TWIHS_WPMR_WPEN                     TWIHS_WPMR_WPEN_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use TWIHS_WPMR_WPEN_Msk instead */
#define TWIHS_WPMR_WPKEY_Pos                8                                              /**< (TWIHS_WPMR) Write Protection Key Position */
#define TWIHS_WPMR_WPKEY_Msk                (0xFFFFFFU << TWIHS_WPMR_WPKEY_Pos)            /**< (TWIHS_WPMR) Write Protection Key Mask */
#define TWIHS_WPMR_WPKEY(value)             (TWIHS_WPMR_WPKEY_Msk & ((value) << TWIHS_WPMR_WPKEY_Pos))
#define   TWIHS_WPMR_WPKEY_PASSWD_Val       (0x545749U)                                    /**< (TWIHS_WPMR) Writing any other value in this field aborts the write operation of the WPEN bit.Always reads as 0  */
#define TWIHS_WPMR_WPKEY_PASSWD             (TWIHS_WPMR_WPKEY_PASSWD_Val << TWIHS_WPMR_WPKEY_Pos)  /**< (TWIHS_WPMR) Writing any other value in this field aborts the write operation of the WPEN bit.Always reads as 0 Position  */
#define TWIHS_WPMR_MASK                     (0xFFFFFF01U)                                  /**< \deprecated (TWIHS_WPMR) Register MASK  (Use TWIHS_WPMR_Msk instead)  */
#define TWIHS_WPMR_Msk                      (0xFFFFFF01U)                                  /**< (TWIHS_WPMR) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- TWIHS_WPSR : (TWIHS Offset: 0xe8) (R/ 32) Write Protection Status Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t WPVS:1;                    /**< bit:      0  Write Protection Violation Status        */
    uint32_t :7;                        /**< bit:   1..7  Reserved */
    uint32_t WPVSRC:24;                 /**< bit:  8..31  Write Protection Violation Source        */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} TWIHS_WPSR_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define TWIHS_WPSR_OFFSET                   (0xE8)                                        /**<  (TWIHS_WPSR) Write Protection Status Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define TWIHS_WPSR_WPVS_Pos                 0                                              /**< (TWIHS_WPSR) Write Protection Violation Status Position */
#define TWIHS_WPSR_WPVS_Msk                 (0x1U << TWIHS_WPSR_WPVS_Pos)                  /**< (TWIHS_WPSR) Write Protection Violation Status Mask */
#define TWIHS_WPSR_WPVS                     TWIHS_WPSR_WPVS_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use TWIHS_WPSR_WPVS_Msk instead */
#define TWIHS_WPSR_WPVSRC_Pos               8                                              /**< (TWIHS_WPSR) Write Protection Violation Source Position */
#define TWIHS_WPSR_WPVSRC_Msk               (0xFFFFFFU << TWIHS_WPSR_WPVSRC_Pos)           /**< (TWIHS_WPSR) Write Protection Violation Source Mask */
#define TWIHS_WPSR_WPVSRC(value)            (TWIHS_WPSR_WPVSRC_Msk & ((value) << TWIHS_WPSR_WPVSRC_Pos))
#define TWIHS_WPSR_MASK                     (0xFFFFFF01U)                                  /**< \deprecated (TWIHS_WPSR) Register MASK  (Use TWIHS_WPSR_Msk instead)  */
#define TWIHS_WPSR_Msk                      (0xFFFFFF01U)                                  /**< (TWIHS_WPSR) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
#if COMPONENT_TYPEDEF_STYLE == 'R'
/** \brief TWIHS hardware registers */
typedef struct {  
  __O  uint32_t TWIHS_CR;       /**< (TWIHS Offset: 0x00) Control Register */
  __IO uint32_t TWIHS_MMR;      /**< (TWIHS Offset: 0x04) Master Mode Register */
  __IO uint32_t TWIHS_SMR;      /**< (TWIHS Offset: 0x08) Slave Mode Register */
  __IO uint32_t TWIHS_IADR;     /**< (TWIHS Offset: 0x0C) Internal Address Register */
  __IO uint32_t TWIHS_CWGR;     /**< (TWIHS Offset: 0x10) Clock Waveform Generator Register */
  __I  uint32_t Reserved1[3];
  __I  uint32_t TWIHS_SR;       /**< (TWIHS Offset: 0x20) Status Register */
  __O  uint32_t TWIHS_IER;      /**< (TWIHS Offset: 0x24) Interrupt Enable Register */
  __O  uint32_t TWIHS_IDR;      /**< (TWIHS Offset: 0x28) Interrupt Disable Register */
  __I  uint32_t TWIHS_IMR;      /**< (TWIHS Offset: 0x2C) Interrupt Mask Register */
  __I  uint32_t TWIHS_RHR;      /**< (TWIHS Offset: 0x30) Receive Holding Register */
  __O  uint32_t TWIHS_THR;      /**< (TWIHS Offset: 0x34) Transmit Holding Register */
  __IO uint32_t TWIHS_SMBTR;    /**< (TWIHS Offset: 0x38) SMBus Timing Register */
  __I  uint32_t Reserved2[2];
  __IO uint32_t TWIHS_FILTR;    /**< (TWIHS Offset: 0x44) Filter Register */
  __I  uint32_t Reserved3[1];
  __IO uint32_t TWIHS_SWMR;     /**< (TWIHS Offset: 0x4C) SleepWalking Matching Register */
  __I  uint32_t Reserved4[37];
  __IO uint32_t TWIHS_WPMR;     /**< (TWIHS Offset: 0xE4) Write Protection Mode Register */
  __I  uint32_t TWIHS_WPSR;     /**< (TWIHS Offset: 0xE8) Write Protection Status Register */
} Twihs;

#elif COMPONENT_TYPEDEF_STYLE == 'N'
/** \brief TWIHS hardware registers */
typedef struct {  
  __O  TWIHS_CR_Type                  TWIHS_CR;       /**< Offset: 0x00 ( /W  32) Control Register */
  __IO TWIHS_MMR_Type                 TWIHS_MMR;      /**< Offset: 0x04 (R/W  32) Master Mode Register */
  __IO TWIHS_SMR_Type                 TWIHS_SMR;      /**< Offset: 0x08 (R/W  32) Slave Mode Register */
  __IO TWIHS_IADR_Type                TWIHS_IADR;     /**< Offset: 0x0C (R/W  32) Internal Address Register */
  __IO TWIHS_CWGR_Type                TWIHS_CWGR;     /**< Offset: 0x10 (R/W  32) Clock Waveform Generator Register */
       RoReg8                         Reserved1[0xC];
  __I  TWIHS_SR_Type                  TWIHS_SR;       /**< Offset: 0x20 (R/   32) Status Register */
  __O  TWIHS_IER_Type                 TWIHS_IER;      /**< Offset: 0x24 ( /W  32) Interrupt Enable Register */
  __O  TWIHS_IDR_Type                 TWIHS_IDR;      /**< Offset: 0x28 ( /W  32) Interrupt Disable Register */
  __I  TWIHS_IMR_Type                 TWIHS_IMR;      /**< Offset: 0x2C (R/   32) Interrupt Mask Register */
  __I  TWIHS_RHR_Type                 TWIHS_RHR;      /**< Offset: 0x30 (R/   32) Receive Holding Register */
  __O  TWIHS_THR_Type                 TWIHS_THR;      /**< Offset: 0x34 ( /W  32) Transmit Holding Register */
  __IO TWIHS_SMBTR_Type               TWIHS_SMBTR;    /**< Offset: 0x38 (R/W  32) SMBus Timing Register */
       RoReg8                         Reserved2[0x8];
  __IO TWIHS_FILTR_Type               TWIHS_FILTR;    /**< Offset: 0x44 (R/W  32) Filter Register */
       RoReg8                         Reserved3[0x4];
  __IO TWIHS_SWMR_Type                TWIHS_SWMR;     /**< Offset: 0x4C (R/W  32) SleepWalking Matching Register */
       RoReg8                         Reserved4[0x94];
  __IO TWIHS_WPMR_Type                TWIHS_WPMR;     /**< Offset: 0xE4 (R/W  32) Write Protection Mode Register */
  __I  TWIHS_WPSR_Type                TWIHS_WPSR;     /**< Offset: 0xE8 (R/   32) Write Protection Status Register */
} Twihs;

#else /* COMPONENT_TYPEDEF_STYLE */
#error Unknown component typedef style
#endif /* COMPONENT_TYPEDEF_STYLE */

#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/** @}  end of Two-wire Interface High Speed */

#endif /* _SAME70_TWIHS_COMPONENT_H_ */
