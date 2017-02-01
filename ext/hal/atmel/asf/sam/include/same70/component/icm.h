/**
 * \file
 *
 * \brief Component description for ICM
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

#ifndef _SAME70_ICM_COMPONENT_H_
#define _SAME70_ICM_COMPONENT_H_
#define _SAME70_ICM_COMPONENT_         /**< \deprecated  Backward compatibility for ASF */

/** \addtogroup SAME70_ICM Integrity Check Monitor
 *  @{
 */
/* ========================================================================== */
/**  SOFTWARE API DEFINITION FOR ICM */
/* ========================================================================== */
#ifndef COMPONENT_TYPEDEF_STYLE
  #define COMPONENT_TYPEDEF_STYLE 'R'  /**< Defines default style of typedefs for the component header files ('R' = RFO, 'N' = NTO)*/
#endif

#define ICM_11105                      /**< (ICM) Module ID */
#define REV_ICM F                      /**< (ICM) Module revision */

/* -------- ICM_CFG : (ICM Offset: 0x00) (R/W 32) Configuration Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t WBDIS:1;                   /**< bit:      0  Write Back Disable                       */
    uint32_t EOMDIS:1;                  /**< bit:      1  End of Monitoring Disable                */
    uint32_t SLBDIS:1;                  /**< bit:      2  Secondary List Branching Disable         */
    uint32_t :1;                        /**< bit:      3  Reserved */
    uint32_t BBC:4;                     /**< bit:   4..7  Bus Burden Control                       */
    uint32_t ASCD:1;                    /**< bit:      8  Automatic Switch To Compare Digest       */
    uint32_t DUALBUFF:1;                /**< bit:      9  Dual Input Buffer                        */
    uint32_t :2;                        /**< bit: 10..11  Reserved */
    uint32_t UIHASH:1;                  /**< bit:     12  User Initial Hash Value                  */
    uint32_t UALGO:3;                   /**< bit: 13..15  User SHA Algorithm                       */
    uint32_t :16;                       /**< bit: 16..31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} ICM_CFG_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define ICM_CFG_OFFSET                      (0x00)                                        /**<  (ICM_CFG) Configuration Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define ICM_CFG_WBDIS_Pos                   0                                              /**< (ICM_CFG) Write Back Disable Position */
#define ICM_CFG_WBDIS_Msk                   (0x1U << ICM_CFG_WBDIS_Pos)                    /**< (ICM_CFG) Write Back Disable Mask */
#define ICM_CFG_WBDIS                       ICM_CFG_WBDIS_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use ICM_CFG_WBDIS_Msk instead */
#define ICM_CFG_EOMDIS_Pos                  1                                              /**< (ICM_CFG) End of Monitoring Disable Position */
#define ICM_CFG_EOMDIS_Msk                  (0x1U << ICM_CFG_EOMDIS_Pos)                   /**< (ICM_CFG) End of Monitoring Disable Mask */
#define ICM_CFG_EOMDIS                      ICM_CFG_EOMDIS_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use ICM_CFG_EOMDIS_Msk instead */
#define ICM_CFG_SLBDIS_Pos                  2                                              /**< (ICM_CFG) Secondary List Branching Disable Position */
#define ICM_CFG_SLBDIS_Msk                  (0x1U << ICM_CFG_SLBDIS_Pos)                   /**< (ICM_CFG) Secondary List Branching Disable Mask */
#define ICM_CFG_SLBDIS                      ICM_CFG_SLBDIS_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use ICM_CFG_SLBDIS_Msk instead */
#define ICM_CFG_BBC_Pos                     4                                              /**< (ICM_CFG) Bus Burden Control Position */
#define ICM_CFG_BBC_Msk                     (0xFU << ICM_CFG_BBC_Pos)                      /**< (ICM_CFG) Bus Burden Control Mask */
#define ICM_CFG_BBC(value)                  (ICM_CFG_BBC_Msk & ((value) << ICM_CFG_BBC_Pos))
#define ICM_CFG_ASCD_Pos                    8                                              /**< (ICM_CFG) Automatic Switch To Compare Digest Position */
#define ICM_CFG_ASCD_Msk                    (0x1U << ICM_CFG_ASCD_Pos)                     /**< (ICM_CFG) Automatic Switch To Compare Digest Mask */
#define ICM_CFG_ASCD                        ICM_CFG_ASCD_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use ICM_CFG_ASCD_Msk instead */
#define ICM_CFG_DUALBUFF_Pos                9                                              /**< (ICM_CFG) Dual Input Buffer Position */
#define ICM_CFG_DUALBUFF_Msk                (0x1U << ICM_CFG_DUALBUFF_Pos)                 /**< (ICM_CFG) Dual Input Buffer Mask */
#define ICM_CFG_DUALBUFF                    ICM_CFG_DUALBUFF_Msk                           /**< \deprecated Old style mask definition for 1 bit bitfield. Use ICM_CFG_DUALBUFF_Msk instead */
#define ICM_CFG_UIHASH_Pos                  12                                             /**< (ICM_CFG) User Initial Hash Value Position */
#define ICM_CFG_UIHASH_Msk                  (0x1U << ICM_CFG_UIHASH_Pos)                   /**< (ICM_CFG) User Initial Hash Value Mask */
#define ICM_CFG_UIHASH                      ICM_CFG_UIHASH_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use ICM_CFG_UIHASH_Msk instead */
#define ICM_CFG_UALGO_Pos                   13                                             /**< (ICM_CFG) User SHA Algorithm Position */
#define ICM_CFG_UALGO_Msk                   (0x7U << ICM_CFG_UALGO_Pos)                    /**< (ICM_CFG) User SHA Algorithm Mask */
#define ICM_CFG_UALGO(value)                (ICM_CFG_UALGO_Msk & ((value) << ICM_CFG_UALGO_Pos))
#define   ICM_CFG_UALGO_SHA1_Val            (0x0U)                                         /**< (ICM_CFG) SHA1 algorithm processed  */
#define   ICM_CFG_UALGO_SHA256_Val          (0x1U)                                         /**< (ICM_CFG) SHA256 algorithm processed  */
#define   ICM_CFG_UALGO_SHA224_Val          (0x4U)                                         /**< (ICM_CFG) SHA224 algorithm processed  */
#define ICM_CFG_UALGO_SHA1                  (ICM_CFG_UALGO_SHA1_Val << ICM_CFG_UALGO_Pos)  /**< (ICM_CFG) SHA1 algorithm processed Position  */
#define ICM_CFG_UALGO_SHA256                (ICM_CFG_UALGO_SHA256_Val << ICM_CFG_UALGO_Pos)  /**< (ICM_CFG) SHA256 algorithm processed Position  */
#define ICM_CFG_UALGO_SHA224                (ICM_CFG_UALGO_SHA224_Val << ICM_CFG_UALGO_Pos)  /**< (ICM_CFG) SHA224 algorithm processed Position  */
#define ICM_CFG_MASK                        (0xF3F7U)                                      /**< \deprecated (ICM_CFG) Register MASK  (Use ICM_CFG_Msk instead)  */
#define ICM_CFG_Msk                         (0xF3F7U)                                      /**< (ICM_CFG) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- ICM_CTRL : (ICM Offset: 0x04) (/W 32) Control Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t ENABLE:1;                  /**< bit:      0  ICM Enable                               */
    uint32_t DISABLE:1;                 /**< bit:      1  ICM Disable Register                     */
    uint32_t SWRST:1;                   /**< bit:      2  Software Reset                           */
    uint32_t :1;                        /**< bit:      3  Reserved */
    uint32_t REHASH:4;                  /**< bit:   4..7  Recompute Internal Hash                  */
    uint32_t RMDIS:4;                   /**< bit:  8..11  Region Monitoring Disable                */
    uint32_t RMEN:4;                    /**< bit: 12..15  Region Monitoring Enable                 */
    uint32_t :16;                       /**< bit: 16..31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} ICM_CTRL_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define ICM_CTRL_OFFSET                     (0x04)                                        /**<  (ICM_CTRL) Control Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define ICM_CTRL_ENABLE_Pos                 0                                              /**< (ICM_CTRL) ICM Enable Position */
#define ICM_CTRL_ENABLE_Msk                 (0x1U << ICM_CTRL_ENABLE_Pos)                  /**< (ICM_CTRL) ICM Enable Mask */
#define ICM_CTRL_ENABLE                     ICM_CTRL_ENABLE_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use ICM_CTRL_ENABLE_Msk instead */
#define ICM_CTRL_DISABLE_Pos                1                                              /**< (ICM_CTRL) ICM Disable Register Position */
#define ICM_CTRL_DISABLE_Msk                (0x1U << ICM_CTRL_DISABLE_Pos)                 /**< (ICM_CTRL) ICM Disable Register Mask */
#define ICM_CTRL_DISABLE                    ICM_CTRL_DISABLE_Msk                           /**< \deprecated Old style mask definition for 1 bit bitfield. Use ICM_CTRL_DISABLE_Msk instead */
#define ICM_CTRL_SWRST_Pos                  2                                              /**< (ICM_CTRL) Software Reset Position */
#define ICM_CTRL_SWRST_Msk                  (0x1U << ICM_CTRL_SWRST_Pos)                   /**< (ICM_CTRL) Software Reset Mask */
#define ICM_CTRL_SWRST                      ICM_CTRL_SWRST_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use ICM_CTRL_SWRST_Msk instead */
#define ICM_CTRL_REHASH_Pos                 4                                              /**< (ICM_CTRL) Recompute Internal Hash Position */
#define ICM_CTRL_REHASH_Msk                 (0xFU << ICM_CTRL_REHASH_Pos)                  /**< (ICM_CTRL) Recompute Internal Hash Mask */
#define ICM_CTRL_REHASH(value)              (ICM_CTRL_REHASH_Msk & ((value) << ICM_CTRL_REHASH_Pos))
#define ICM_CTRL_RMDIS_Pos                  8                                              /**< (ICM_CTRL) Region Monitoring Disable Position */
#define ICM_CTRL_RMDIS_Msk                  (0xFU << ICM_CTRL_RMDIS_Pos)                   /**< (ICM_CTRL) Region Monitoring Disable Mask */
#define ICM_CTRL_RMDIS(value)               (ICM_CTRL_RMDIS_Msk & ((value) << ICM_CTRL_RMDIS_Pos))
#define ICM_CTRL_RMEN_Pos                   12                                             /**< (ICM_CTRL) Region Monitoring Enable Position */
#define ICM_CTRL_RMEN_Msk                   (0xFU << ICM_CTRL_RMEN_Pos)                    /**< (ICM_CTRL) Region Monitoring Enable Mask */
#define ICM_CTRL_RMEN(value)                (ICM_CTRL_RMEN_Msk & ((value) << ICM_CTRL_RMEN_Pos))
#define ICM_CTRL_MASK                       (0xFFF7U)                                      /**< \deprecated (ICM_CTRL) Register MASK  (Use ICM_CTRL_Msk instead)  */
#define ICM_CTRL_Msk                        (0xFFF7U)                                      /**< (ICM_CTRL) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- ICM_SR : (ICM Offset: 0x08) (/W 32) Status Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t ENABLE:1;                  /**< bit:      0  ICM Controller Enable Register           */
    uint32_t :7;                        /**< bit:   1..7  Reserved */
    uint32_t RAWRMDIS:4;                /**< bit:  8..11  Region Monitoring Disabled Raw Status    */
    uint32_t RMDIS:4;                   /**< bit: 12..15  Region Monitoring Disabled Status        */
    uint32_t :16;                       /**< bit: 16..31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} ICM_SR_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define ICM_SR_OFFSET                       (0x08)                                        /**<  (ICM_SR) Status Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define ICM_SR_ENABLE_Pos                   0                                              /**< (ICM_SR) ICM Controller Enable Register Position */
#define ICM_SR_ENABLE_Msk                   (0x1U << ICM_SR_ENABLE_Pos)                    /**< (ICM_SR) ICM Controller Enable Register Mask */
#define ICM_SR_ENABLE                       ICM_SR_ENABLE_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use ICM_SR_ENABLE_Msk instead */
#define ICM_SR_RAWRMDIS_Pos                 8                                              /**< (ICM_SR) Region Monitoring Disabled Raw Status Position */
#define ICM_SR_RAWRMDIS_Msk                 (0xFU << ICM_SR_RAWRMDIS_Pos)                  /**< (ICM_SR) Region Monitoring Disabled Raw Status Mask */
#define ICM_SR_RAWRMDIS(value)              (ICM_SR_RAWRMDIS_Msk & ((value) << ICM_SR_RAWRMDIS_Pos))
#define ICM_SR_RMDIS_Pos                    12                                             /**< (ICM_SR) Region Monitoring Disabled Status Position */
#define ICM_SR_RMDIS_Msk                    (0xFU << ICM_SR_RMDIS_Pos)                     /**< (ICM_SR) Region Monitoring Disabled Status Mask */
#define ICM_SR_RMDIS(value)                 (ICM_SR_RMDIS_Msk & ((value) << ICM_SR_RMDIS_Pos))
#define ICM_SR_MASK                         (0xFF01U)                                      /**< \deprecated (ICM_SR) Register MASK  (Use ICM_SR_Msk instead)  */
#define ICM_SR_Msk                          (0xFF01U)                                      /**< (ICM_SR) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- ICM_IER : (ICM Offset: 0x10) (/W 32) Interrupt Enable Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t RHC:4;                     /**< bit:   0..3  Region Hash Completed Interrupt Enable   */
    uint32_t RDM:4;                     /**< bit:   4..7  Region Digest Mismatch Interrupt Enable  */
    uint32_t RBE:4;                     /**< bit:  8..11  Region Bus Error Interrupt Enable        */
    uint32_t RWC:4;                     /**< bit: 12..15  Region Wrap Condition detected Interrupt Enable */
    uint32_t REC:4;                     /**< bit: 16..19  Region End bit Condition Detected Interrupt Enable */
    uint32_t RSU:4;                     /**< bit: 20..23  Region Status Updated Interrupt Disable  */
    uint32_t URAD:1;                    /**< bit:     24  Undefined Register Access Detection Interrupt Enable */
    uint32_t :7;                        /**< bit: 25..31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} ICM_IER_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define ICM_IER_OFFSET                      (0x10)                                        /**<  (ICM_IER) Interrupt Enable Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define ICM_IER_RHC_Pos                     0                                              /**< (ICM_IER) Region Hash Completed Interrupt Enable Position */
#define ICM_IER_RHC_Msk                     (0xFU << ICM_IER_RHC_Pos)                      /**< (ICM_IER) Region Hash Completed Interrupt Enable Mask */
#define ICM_IER_RHC(value)                  (ICM_IER_RHC_Msk & ((value) << ICM_IER_RHC_Pos))
#define ICM_IER_RDM_Pos                     4                                              /**< (ICM_IER) Region Digest Mismatch Interrupt Enable Position */
#define ICM_IER_RDM_Msk                     (0xFU << ICM_IER_RDM_Pos)                      /**< (ICM_IER) Region Digest Mismatch Interrupt Enable Mask */
#define ICM_IER_RDM(value)                  (ICM_IER_RDM_Msk & ((value) << ICM_IER_RDM_Pos))
#define ICM_IER_RBE_Pos                     8                                              /**< (ICM_IER) Region Bus Error Interrupt Enable Position */
#define ICM_IER_RBE_Msk                     (0xFU << ICM_IER_RBE_Pos)                      /**< (ICM_IER) Region Bus Error Interrupt Enable Mask */
#define ICM_IER_RBE(value)                  (ICM_IER_RBE_Msk & ((value) << ICM_IER_RBE_Pos))
#define ICM_IER_RWC_Pos                     12                                             /**< (ICM_IER) Region Wrap Condition detected Interrupt Enable Position */
#define ICM_IER_RWC_Msk                     (0xFU << ICM_IER_RWC_Pos)                      /**< (ICM_IER) Region Wrap Condition detected Interrupt Enable Mask */
#define ICM_IER_RWC(value)                  (ICM_IER_RWC_Msk & ((value) << ICM_IER_RWC_Pos))
#define ICM_IER_REC_Pos                     16                                             /**< (ICM_IER) Region End bit Condition Detected Interrupt Enable Position */
#define ICM_IER_REC_Msk                     (0xFU << ICM_IER_REC_Pos)                      /**< (ICM_IER) Region End bit Condition Detected Interrupt Enable Mask */
#define ICM_IER_REC(value)                  (ICM_IER_REC_Msk & ((value) << ICM_IER_REC_Pos))
#define ICM_IER_RSU_Pos                     20                                             /**< (ICM_IER) Region Status Updated Interrupt Disable Position */
#define ICM_IER_RSU_Msk                     (0xFU << ICM_IER_RSU_Pos)                      /**< (ICM_IER) Region Status Updated Interrupt Disable Mask */
#define ICM_IER_RSU(value)                  (ICM_IER_RSU_Msk & ((value) << ICM_IER_RSU_Pos))
#define ICM_IER_URAD_Pos                    24                                             /**< (ICM_IER) Undefined Register Access Detection Interrupt Enable Position */
#define ICM_IER_URAD_Msk                    (0x1U << ICM_IER_URAD_Pos)                     /**< (ICM_IER) Undefined Register Access Detection Interrupt Enable Mask */
#define ICM_IER_URAD                        ICM_IER_URAD_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use ICM_IER_URAD_Msk instead */
#define ICM_IER_MASK                        (0x1FFFFFFU)                                   /**< \deprecated (ICM_IER) Register MASK  (Use ICM_IER_Msk instead)  */
#define ICM_IER_Msk                         (0x1FFFFFFU)                                   /**< (ICM_IER) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- ICM_IDR : (ICM Offset: 0x14) (/W 32) Interrupt Disable Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t RHC:4;                     /**< bit:   0..3  Region Hash Completed Interrupt Disable  */
    uint32_t RDM:4;                     /**< bit:   4..7  Region Digest Mismatch Interrupt Disable */
    uint32_t RBE:4;                     /**< bit:  8..11  Region Bus Error Interrupt Disable       */
    uint32_t RWC:4;                     /**< bit: 12..15  Region Wrap Condition Detected Interrupt Disable */
    uint32_t REC:4;                     /**< bit: 16..19  Region End bit Condition detected Interrupt Disable */
    uint32_t RSU:4;                     /**< bit: 20..23  Region Status Updated Interrupt Disable  */
    uint32_t URAD:1;                    /**< bit:     24  Undefined Register Access Detection Interrupt Disable */
    uint32_t :7;                        /**< bit: 25..31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} ICM_IDR_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define ICM_IDR_OFFSET                      (0x14)                                        /**<  (ICM_IDR) Interrupt Disable Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define ICM_IDR_RHC_Pos                     0                                              /**< (ICM_IDR) Region Hash Completed Interrupt Disable Position */
#define ICM_IDR_RHC_Msk                     (0xFU << ICM_IDR_RHC_Pos)                      /**< (ICM_IDR) Region Hash Completed Interrupt Disable Mask */
#define ICM_IDR_RHC(value)                  (ICM_IDR_RHC_Msk & ((value) << ICM_IDR_RHC_Pos))
#define ICM_IDR_RDM_Pos                     4                                              /**< (ICM_IDR) Region Digest Mismatch Interrupt Disable Position */
#define ICM_IDR_RDM_Msk                     (0xFU << ICM_IDR_RDM_Pos)                      /**< (ICM_IDR) Region Digest Mismatch Interrupt Disable Mask */
#define ICM_IDR_RDM(value)                  (ICM_IDR_RDM_Msk & ((value) << ICM_IDR_RDM_Pos))
#define ICM_IDR_RBE_Pos                     8                                              /**< (ICM_IDR) Region Bus Error Interrupt Disable Position */
#define ICM_IDR_RBE_Msk                     (0xFU << ICM_IDR_RBE_Pos)                      /**< (ICM_IDR) Region Bus Error Interrupt Disable Mask */
#define ICM_IDR_RBE(value)                  (ICM_IDR_RBE_Msk & ((value) << ICM_IDR_RBE_Pos))
#define ICM_IDR_RWC_Pos                     12                                             /**< (ICM_IDR) Region Wrap Condition Detected Interrupt Disable Position */
#define ICM_IDR_RWC_Msk                     (0xFU << ICM_IDR_RWC_Pos)                      /**< (ICM_IDR) Region Wrap Condition Detected Interrupt Disable Mask */
#define ICM_IDR_RWC(value)                  (ICM_IDR_RWC_Msk & ((value) << ICM_IDR_RWC_Pos))
#define ICM_IDR_REC_Pos                     16                                             /**< (ICM_IDR) Region End bit Condition detected Interrupt Disable Position */
#define ICM_IDR_REC_Msk                     (0xFU << ICM_IDR_REC_Pos)                      /**< (ICM_IDR) Region End bit Condition detected Interrupt Disable Mask */
#define ICM_IDR_REC(value)                  (ICM_IDR_REC_Msk & ((value) << ICM_IDR_REC_Pos))
#define ICM_IDR_RSU_Pos                     20                                             /**< (ICM_IDR) Region Status Updated Interrupt Disable Position */
#define ICM_IDR_RSU_Msk                     (0xFU << ICM_IDR_RSU_Pos)                      /**< (ICM_IDR) Region Status Updated Interrupt Disable Mask */
#define ICM_IDR_RSU(value)                  (ICM_IDR_RSU_Msk & ((value) << ICM_IDR_RSU_Pos))
#define ICM_IDR_URAD_Pos                    24                                             /**< (ICM_IDR) Undefined Register Access Detection Interrupt Disable Position */
#define ICM_IDR_URAD_Msk                    (0x1U << ICM_IDR_URAD_Pos)                     /**< (ICM_IDR) Undefined Register Access Detection Interrupt Disable Mask */
#define ICM_IDR_URAD                        ICM_IDR_URAD_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use ICM_IDR_URAD_Msk instead */
#define ICM_IDR_MASK                        (0x1FFFFFFU)                                   /**< \deprecated (ICM_IDR) Register MASK  (Use ICM_IDR_Msk instead)  */
#define ICM_IDR_Msk                         (0x1FFFFFFU)                                   /**< (ICM_IDR) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- ICM_IMR : (ICM Offset: 0x18) (R/ 32) Interrupt Mask Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t RHC:4;                     /**< bit:   0..3  Region Hash Completed Interrupt Mask     */
    uint32_t RDM:4;                     /**< bit:   4..7  Region Digest Mismatch Interrupt Mask    */
    uint32_t RBE:4;                     /**< bit:  8..11  Region Bus Error Interrupt Mask          */
    uint32_t RWC:4;                     /**< bit: 12..15  Region Wrap Condition Detected Interrupt Mask */
    uint32_t REC:4;                     /**< bit: 16..19  Region End bit Condition Detected Interrupt Mask */
    uint32_t RSU:4;                     /**< bit: 20..23  Region Status Updated Interrupt Mask     */
    uint32_t URAD:1;                    /**< bit:     24  Undefined Register Access Detection Interrupt Mask */
    uint32_t :7;                        /**< bit: 25..31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} ICM_IMR_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define ICM_IMR_OFFSET                      (0x18)                                        /**<  (ICM_IMR) Interrupt Mask Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define ICM_IMR_RHC_Pos                     0                                              /**< (ICM_IMR) Region Hash Completed Interrupt Mask Position */
#define ICM_IMR_RHC_Msk                     (0xFU << ICM_IMR_RHC_Pos)                      /**< (ICM_IMR) Region Hash Completed Interrupt Mask Mask */
#define ICM_IMR_RHC(value)                  (ICM_IMR_RHC_Msk & ((value) << ICM_IMR_RHC_Pos))
#define ICM_IMR_RDM_Pos                     4                                              /**< (ICM_IMR) Region Digest Mismatch Interrupt Mask Position */
#define ICM_IMR_RDM_Msk                     (0xFU << ICM_IMR_RDM_Pos)                      /**< (ICM_IMR) Region Digest Mismatch Interrupt Mask Mask */
#define ICM_IMR_RDM(value)                  (ICM_IMR_RDM_Msk & ((value) << ICM_IMR_RDM_Pos))
#define ICM_IMR_RBE_Pos                     8                                              /**< (ICM_IMR) Region Bus Error Interrupt Mask Position */
#define ICM_IMR_RBE_Msk                     (0xFU << ICM_IMR_RBE_Pos)                      /**< (ICM_IMR) Region Bus Error Interrupt Mask Mask */
#define ICM_IMR_RBE(value)                  (ICM_IMR_RBE_Msk & ((value) << ICM_IMR_RBE_Pos))
#define ICM_IMR_RWC_Pos                     12                                             /**< (ICM_IMR) Region Wrap Condition Detected Interrupt Mask Position */
#define ICM_IMR_RWC_Msk                     (0xFU << ICM_IMR_RWC_Pos)                      /**< (ICM_IMR) Region Wrap Condition Detected Interrupt Mask Mask */
#define ICM_IMR_RWC(value)                  (ICM_IMR_RWC_Msk & ((value) << ICM_IMR_RWC_Pos))
#define ICM_IMR_REC_Pos                     16                                             /**< (ICM_IMR) Region End bit Condition Detected Interrupt Mask Position */
#define ICM_IMR_REC_Msk                     (0xFU << ICM_IMR_REC_Pos)                      /**< (ICM_IMR) Region End bit Condition Detected Interrupt Mask Mask */
#define ICM_IMR_REC(value)                  (ICM_IMR_REC_Msk & ((value) << ICM_IMR_REC_Pos))
#define ICM_IMR_RSU_Pos                     20                                             /**< (ICM_IMR) Region Status Updated Interrupt Mask Position */
#define ICM_IMR_RSU_Msk                     (0xFU << ICM_IMR_RSU_Pos)                      /**< (ICM_IMR) Region Status Updated Interrupt Mask Mask */
#define ICM_IMR_RSU(value)                  (ICM_IMR_RSU_Msk & ((value) << ICM_IMR_RSU_Pos))
#define ICM_IMR_URAD_Pos                    24                                             /**< (ICM_IMR) Undefined Register Access Detection Interrupt Mask Position */
#define ICM_IMR_URAD_Msk                    (0x1U << ICM_IMR_URAD_Pos)                     /**< (ICM_IMR) Undefined Register Access Detection Interrupt Mask Mask */
#define ICM_IMR_URAD                        ICM_IMR_URAD_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use ICM_IMR_URAD_Msk instead */
#define ICM_IMR_MASK                        (0x1FFFFFFU)                                   /**< \deprecated (ICM_IMR) Register MASK  (Use ICM_IMR_Msk instead)  */
#define ICM_IMR_Msk                         (0x1FFFFFFU)                                   /**< (ICM_IMR) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- ICM_ISR : (ICM Offset: 0x1c) (R/ 32) Interrupt Status Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t RHC:4;                     /**< bit:   0..3  Region Hash Completed                    */
    uint32_t RDM:4;                     /**< bit:   4..7  Region Digest Mismatch                   */
    uint32_t RBE:4;                     /**< bit:  8..11  Region Bus Error                         */
    uint32_t RWC:4;                     /**< bit: 12..15  Region Wrap Condition Detected           */
    uint32_t REC:4;                     /**< bit: 16..19  Region End bit Condition Detected        */
    uint32_t RSU:4;                     /**< bit: 20..23  Region Status Updated Detected           */
    uint32_t URAD:1;                    /**< bit:     24  Undefined Register Access Detection Status */
    uint32_t :7;                        /**< bit: 25..31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} ICM_ISR_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define ICM_ISR_OFFSET                      (0x1C)                                        /**<  (ICM_ISR) Interrupt Status Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define ICM_ISR_RHC_Pos                     0                                              /**< (ICM_ISR) Region Hash Completed Position */
#define ICM_ISR_RHC_Msk                     (0xFU << ICM_ISR_RHC_Pos)                      /**< (ICM_ISR) Region Hash Completed Mask */
#define ICM_ISR_RHC(value)                  (ICM_ISR_RHC_Msk & ((value) << ICM_ISR_RHC_Pos))
#define ICM_ISR_RDM_Pos                     4                                              /**< (ICM_ISR) Region Digest Mismatch Position */
#define ICM_ISR_RDM_Msk                     (0xFU << ICM_ISR_RDM_Pos)                      /**< (ICM_ISR) Region Digest Mismatch Mask */
#define ICM_ISR_RDM(value)                  (ICM_ISR_RDM_Msk & ((value) << ICM_ISR_RDM_Pos))
#define ICM_ISR_RBE_Pos                     8                                              /**< (ICM_ISR) Region Bus Error Position */
#define ICM_ISR_RBE_Msk                     (0xFU << ICM_ISR_RBE_Pos)                      /**< (ICM_ISR) Region Bus Error Mask */
#define ICM_ISR_RBE(value)                  (ICM_ISR_RBE_Msk & ((value) << ICM_ISR_RBE_Pos))
#define ICM_ISR_RWC_Pos                     12                                             /**< (ICM_ISR) Region Wrap Condition Detected Position */
#define ICM_ISR_RWC_Msk                     (0xFU << ICM_ISR_RWC_Pos)                      /**< (ICM_ISR) Region Wrap Condition Detected Mask */
#define ICM_ISR_RWC(value)                  (ICM_ISR_RWC_Msk & ((value) << ICM_ISR_RWC_Pos))
#define ICM_ISR_REC_Pos                     16                                             /**< (ICM_ISR) Region End bit Condition Detected Position */
#define ICM_ISR_REC_Msk                     (0xFU << ICM_ISR_REC_Pos)                      /**< (ICM_ISR) Region End bit Condition Detected Mask */
#define ICM_ISR_REC(value)                  (ICM_ISR_REC_Msk & ((value) << ICM_ISR_REC_Pos))
#define ICM_ISR_RSU_Pos                     20                                             /**< (ICM_ISR) Region Status Updated Detected Position */
#define ICM_ISR_RSU_Msk                     (0xFU << ICM_ISR_RSU_Pos)                      /**< (ICM_ISR) Region Status Updated Detected Mask */
#define ICM_ISR_RSU(value)                  (ICM_ISR_RSU_Msk & ((value) << ICM_ISR_RSU_Pos))
#define ICM_ISR_URAD_Pos                    24                                             /**< (ICM_ISR) Undefined Register Access Detection Status Position */
#define ICM_ISR_URAD_Msk                    (0x1U << ICM_ISR_URAD_Pos)                     /**< (ICM_ISR) Undefined Register Access Detection Status Mask */
#define ICM_ISR_URAD                        ICM_ISR_URAD_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use ICM_ISR_URAD_Msk instead */
#define ICM_ISR_MASK                        (0x1FFFFFFU)                                   /**< \deprecated (ICM_ISR) Register MASK  (Use ICM_ISR_Msk instead)  */
#define ICM_ISR_Msk                         (0x1FFFFFFU)                                   /**< (ICM_ISR) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- ICM_UASR : (ICM Offset: 0x20) (R/ 32) Undefined Access Status Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t URAT:3;                    /**< bit:   0..2  Undefined Register Access Trace          */
    uint32_t :29;                       /**< bit:  3..31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} ICM_UASR_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define ICM_UASR_OFFSET                     (0x20)                                        /**<  (ICM_UASR) Undefined Access Status Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define ICM_UASR_URAT_Pos                   0                                              /**< (ICM_UASR) Undefined Register Access Trace Position */
#define ICM_UASR_URAT_Msk                   (0x7U << ICM_UASR_URAT_Pos)                    /**< (ICM_UASR) Undefined Register Access Trace Mask */
#define ICM_UASR_URAT(value)                (ICM_UASR_URAT_Msk & ((value) << ICM_UASR_URAT_Pos))
#define   ICM_UASR_URAT_UNSPEC_STRUCT_MEMBER_Val (0x0U)                                         /**< (ICM_UASR) Unspecified structure member set to one detected when the descriptor is loaded.  */
#define   ICM_UASR_URAT_ICM_CFG_MODIFIED_Val (0x1U)                                         /**< (ICM_UASR) ICM_CFG modified during active monitoring.  */
#define   ICM_UASR_URAT_ICM_DSCR_MODIFIED_Val (0x2U)                                         /**< (ICM_UASR) ICM_DSCR modified during active monitoring.  */
#define   ICM_UASR_URAT_ICM_HASH_MODIFIED_Val (0x3U)                                         /**< (ICM_UASR) ICM_HASH modified during active monitoring  */
#define   ICM_UASR_URAT_READ_ACCESS_Val     (0x4U)                                         /**< (ICM_UASR) Write-only register read access  */
#define ICM_UASR_URAT_UNSPEC_STRUCT_MEMBER  (ICM_UASR_URAT_UNSPEC_STRUCT_MEMBER_Val << ICM_UASR_URAT_Pos)  /**< (ICM_UASR) Unspecified structure member set to one detected when the descriptor is loaded. Position  */
#define ICM_UASR_URAT_ICM_CFG_MODIFIED      (ICM_UASR_URAT_ICM_CFG_MODIFIED_Val << ICM_UASR_URAT_Pos)  /**< (ICM_UASR) ICM_CFG modified during active monitoring. Position  */
#define ICM_UASR_URAT_ICM_DSCR_MODIFIED     (ICM_UASR_URAT_ICM_DSCR_MODIFIED_Val << ICM_UASR_URAT_Pos)  /**< (ICM_UASR) ICM_DSCR modified during active monitoring. Position  */
#define ICM_UASR_URAT_ICM_HASH_MODIFIED     (ICM_UASR_URAT_ICM_HASH_MODIFIED_Val << ICM_UASR_URAT_Pos)  /**< (ICM_UASR) ICM_HASH modified during active monitoring Position  */
#define ICM_UASR_URAT_READ_ACCESS           (ICM_UASR_URAT_READ_ACCESS_Val << ICM_UASR_URAT_Pos)  /**< (ICM_UASR) Write-only register read access Position  */
#define ICM_UASR_MASK                       (0x07U)                                        /**< \deprecated (ICM_UASR) Register MASK  (Use ICM_UASR_Msk instead)  */
#define ICM_UASR_Msk                        (0x07U)                                        /**< (ICM_UASR) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- ICM_DSCR : (ICM Offset: 0x30) (R/W 32) Region Descriptor Area Start Address Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t :6;                        /**< bit:   0..5  Reserved */
    uint32_t DASA:26;                   /**< bit:  6..31  Descriptor Area Start Address            */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} ICM_DSCR_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define ICM_DSCR_OFFSET                     (0x30)                                        /**<  (ICM_DSCR) Region Descriptor Area Start Address Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define ICM_DSCR_DASA_Pos                   6                                              /**< (ICM_DSCR) Descriptor Area Start Address Position */
#define ICM_DSCR_DASA_Msk                   (0x3FFFFFFU << ICM_DSCR_DASA_Pos)              /**< (ICM_DSCR) Descriptor Area Start Address Mask */
#define ICM_DSCR_DASA(value)                (ICM_DSCR_DASA_Msk & ((value) << ICM_DSCR_DASA_Pos))
#define ICM_DSCR_MASK                       (0xFFFFFFC0U)                                  /**< \deprecated (ICM_DSCR) Register MASK  (Use ICM_DSCR_Msk instead)  */
#define ICM_DSCR_Msk                        (0xFFFFFFC0U)                                  /**< (ICM_DSCR) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- ICM_HASH : (ICM Offset: 0x34) (R/W 32) Region Hash Area Start Address Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t :7;                        /**< bit:   0..6  Reserved */
    uint32_t HASA:25;                   /**< bit:  7..31  Hash Area Start Address                  */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} ICM_HASH_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define ICM_HASH_OFFSET                     (0x34)                                        /**<  (ICM_HASH) Region Hash Area Start Address Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define ICM_HASH_HASA_Pos                   7                                              /**< (ICM_HASH) Hash Area Start Address Position */
#define ICM_HASH_HASA_Msk                   (0x1FFFFFFU << ICM_HASH_HASA_Pos)              /**< (ICM_HASH) Hash Area Start Address Mask */
#define ICM_HASH_HASA(value)                (ICM_HASH_HASA_Msk & ((value) << ICM_HASH_HASA_Pos))
#define ICM_HASH_MASK                       (0xFFFFFF80U)                                  /**< \deprecated (ICM_HASH) Register MASK  (Use ICM_HASH_Msk instead)  */
#define ICM_HASH_Msk                        (0xFFFFFF80U)                                  /**< (ICM_HASH) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- ICM_UIHVAL : (ICM Offset: 0x38) (/W 32) User Initial Hash Value 0 Register 0 -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t VAL:32;                    /**< bit:  0..31  Initial Hash Value                       */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} ICM_UIHVAL_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define ICM_UIHVAL_OFFSET                   (0x38)                                        /**<  (ICM_UIHVAL) User Initial Hash Value 0 Register 0  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define ICM_UIHVAL_VAL_Pos                  0                                              /**< (ICM_UIHVAL) Initial Hash Value Position */
#define ICM_UIHVAL_VAL_Msk                  (0xFFFFFFFFU << ICM_UIHVAL_VAL_Pos)            /**< (ICM_UIHVAL) Initial Hash Value Mask */
#define ICM_UIHVAL_VAL(value)               (ICM_UIHVAL_VAL_Msk & ((value) << ICM_UIHVAL_VAL_Pos))
#define ICM_UIHVAL_MASK                     (0xFFFFFFFFU)                                  /**< \deprecated (ICM_UIHVAL) Register MASK  (Use ICM_UIHVAL_Msk instead)  */
#define ICM_UIHVAL_Msk                      (0xFFFFFFFFU)                                  /**< (ICM_UIHVAL) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
#if COMPONENT_TYPEDEF_STYLE == 'R'
/** \brief ICM hardware registers */
typedef struct {  
  __IO uint32_t ICM_CFG;        /**< (ICM Offset: 0x00) Configuration Register */
  __O  uint32_t ICM_CTRL;       /**< (ICM Offset: 0x04) Control Register */
  __O  uint32_t ICM_SR;         /**< (ICM Offset: 0x08) Status Register */
  __I  uint32_t Reserved1[1];
  __O  uint32_t ICM_IER;        /**< (ICM Offset: 0x10) Interrupt Enable Register */
  __O  uint32_t ICM_IDR;        /**< (ICM Offset: 0x14) Interrupt Disable Register */
  __I  uint32_t ICM_IMR;        /**< (ICM Offset: 0x18) Interrupt Mask Register */
  __I  uint32_t ICM_ISR;        /**< (ICM Offset: 0x1C) Interrupt Status Register */
  __I  uint32_t ICM_UASR;       /**< (ICM Offset: 0x20) Undefined Access Status Register */
  __I  uint32_t Reserved2[3];
  __IO uint32_t ICM_DSCR;       /**< (ICM Offset: 0x30) Region Descriptor Area Start Address Register */
  __IO uint32_t ICM_HASH;       /**< (ICM Offset: 0x34) Region Hash Area Start Address Register */
  __O  uint32_t ICM_UIHVAL[8];  /**< (ICM Offset: 0x38) User Initial Hash Value 0 Register 0 */
} Icm;

#elif COMPONENT_TYPEDEF_STYLE == 'N'
/** \brief ICM hardware registers */
typedef struct {  
  __IO ICM_CFG_Type                   ICM_CFG;        /**< Offset: 0x00 (R/W  32) Configuration Register */
  __O  ICM_CTRL_Type                  ICM_CTRL;       /**< Offset: 0x04 ( /W  32) Control Register */
  __O  ICM_SR_Type                    ICM_SR;         /**< Offset: 0x08 ( /W  32) Status Register */
       RoReg8                         Reserved1[0x4];
  __O  ICM_IER_Type                   ICM_IER;        /**< Offset: 0x10 ( /W  32) Interrupt Enable Register */
  __O  ICM_IDR_Type                   ICM_IDR;        /**< Offset: 0x14 ( /W  32) Interrupt Disable Register */
  __I  ICM_IMR_Type                   ICM_IMR;        /**< Offset: 0x18 (R/   32) Interrupt Mask Register */
  __I  ICM_ISR_Type                   ICM_ISR;        /**< Offset: 0x1C (R/   32) Interrupt Status Register */
  __I  ICM_UASR_Type                  ICM_UASR;       /**< Offset: 0x20 (R/   32) Undefined Access Status Register */
       RoReg8                         Reserved2[0xC];
  __IO ICM_DSCR_Type                  ICM_DSCR;       /**< Offset: 0x30 (R/W  32) Region Descriptor Area Start Address Register */
  __IO ICM_HASH_Type                  ICM_HASH;       /**< Offset: 0x34 (R/W  32) Region Hash Area Start Address Register */
  __O  ICM_UIHVAL_Type                ICM_UIHVAL[8];  /**< Offset: 0x38 ( /W  32) User Initial Hash Value 0 Register 0 */
} Icm;

#else /* COMPONENT_TYPEDEF_STYLE */
#error Unknown component typedef style
#endif /* COMPONENT_TYPEDEF_STYLE */

#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/** @}  end of Integrity Check Monitor */

#endif /* _SAME70_ICM_COMPONENT_H_ */
