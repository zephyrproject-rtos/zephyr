/*
 * Copyright (c) 2024 Microchip
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _MICROCHIP_PIC32CXSG_ICM_COMPONENT_FIXUP_H_
#define _MICROCHIP_PIC32CXSG_ICM_COMPONENT_FIXUP_H_

/* -------- ICM_RADDR : (ICM Offset: 0x00) (R/W 32) Region Start Address -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  uint32_t reg;                /*!< Type      used for register access              */
} ICM_RADDR_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- ICM_RCFG : (ICM Offset: 0x04) (R/W 32) Region Configuration -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t CDWBN:1;          /*!< bit:      0  Compare Digest Write Back          */
    uint32_t WRAP:1;           /*!< bit:      1  Region Wrap                        */
    uint32_t EOM:1;            /*!< bit:      2  End of Monitoring                  */
    uint32_t :1;               /*!< bit:      3  Reserved                           */
    uint32_t RHIEN:1;          /*!< bit:      4  Region Hash Interrupt Enable       */
    uint32_t DMIEN:1;          /*!< bit:      5  Region Digest Mismatch Interrupt Enable */
    uint32_t BEIEN:1;          /*!< bit:      6  Region Bus Error Interrupt Enable  */
    uint32_t WCIEN:1;          /*!< bit:      7  Region Wrap Condition Detected Interrupt Enable */
    uint32_t ECIEN:1;          /*!< bit:      8  Region End bit Condition detected Interrupt Enable */
    uint32_t SUIEN:1;          /*!< bit:      9  Region Status Updated Interrupt Enable */
    uint32_t PROCDLY:1;        /*!< bit:     10  SHA Processing Delay               */
    uint32_t :1;               /*!< bit:     11  Reserved                           */
    uint32_t ALGO:3;           /*!< bit: 12..14  SHA Algorithm                      */
    uint32_t :9;               /*!< bit: 15..23  Reserved                           */
    uint32_t MRPROT:6;         /*!< bit: 24..29  Memory Region AHB Protection       */
    uint32_t :2;               /*!< bit: 30..31  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} ICM_RCFG_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- ICM_RCTRL : (ICM Offset: 0x08) (R/W 32) Region Control -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t TRSIZE:16;        /*!< bit:  0..15  Transfer Size                      */
    uint32_t :16;              /*!< bit: 16..31  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} ICM_RCTRL_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- ICM_RNEXT : (ICM Offset: 0x0C) (R/W 32) Region Next Address -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  uint32_t reg;                /*!< Type      used for register access              */
} ICM_RNEXT_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- ICM_CFG : (ICM Offset: 0x00) (R/W 32) Configuration -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t WBDIS:1;          /*!< bit:      0  Write Back Disable                 */
    uint32_t EOMDIS:1;         /*!< bit:      1  End of Monitoring Disable          */
    uint32_t SLBDIS:1;         /*!< bit:      2  Secondary List Branching Disable   */
    uint32_t :1;               /*!< bit:      3  Reserved                           */
    uint32_t BBC:4;            /*!< bit:  4.. 7  Bus Burden Control                 */
    uint32_t ASCD:1;           /*!< bit:      8  Automatic Switch To Compare Digest */
    uint32_t DUALBUFF:1;       /*!< bit:      9  Dual Input Buffer                  */
    uint32_t :2;               /*!< bit: 10..11  Reserved                           */
    uint32_t UIHASH:1;         /*!< bit:     12  User Initial Hash Value            */
    uint32_t UALGO:3;          /*!< bit: 13..15  User SHA Algorithm                 */
    uint32_t HAPROT:6;         /*!< bit: 16..21  Region Hash Area Protection        */
    uint32_t :2;               /*!< bit: 22..23  Reserved                           */
    uint32_t DAPROT:6;         /*!< bit: 24..29  Region Descriptor Area Protection  */
    uint32_t :2;               /*!< bit: 30..31  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} ICM_CFG_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */
/* -------- ICM_CTRL : (ICM Offset: 0x04) ( /W 32) Control -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t ENABLE:1;         /*!< bit:      0  ICM Enable                         */
    uint32_t DISABLE:1;        /*!< bit:      1  ICM Disable Register               */
    uint32_t SWRST:1;          /*!< bit:      2  Software Reset                     */
    uint32_t :1;               /*!< bit:      3  Reserved                           */
    uint32_t REHASH:4;         /*!< bit:  4.. 7  Recompute Internal Hash            */
    uint32_t RMDIS:4;          /*!< bit:  8..11  Region Monitoring Disable          */
    uint32_t RMEN:4;           /*!< bit: 12..15  Region Monitoring Enable           */
    uint32_t :16;              /*!< bit: 16..31  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} ICM_CTRL_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- ICM_SR : (ICM Offset: 0x08) ( R/ 32) Status -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t ENABLE:1;         /*!< bit:      0  ICM Controller Enable Register     */
    uint32_t :7;               /*!< bit:  1.. 7  Reserved                           */
    uint32_t RAWRMDIS:4;       /*!< bit:  8..11  RAW Region Monitoring Disabled Status */
    uint32_t RMDIS:4;          /*!< bit: 12..15  Region Monitoring Disabled Status  */
    uint32_t :16;              /*!< bit: 16..31  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} ICM_SR_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- ICM_IER : (ICM Offset: 0x10) ( /W 32) Interrupt Enable -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t RHC:4;            /*!< bit:  0.. 3  Region Hash Completed Interrupt Enable */
    uint32_t RDM:4;            /*!< bit:  4.. 7  Region Digest Mismatch Interrupt Enable */
    uint32_t RBE:4;            /*!< bit:  8..11  Region Bus Error Interrupt Enable  */
    uint32_t RWC:4;            /*!< bit: 12..15  Region Wrap Condition detected Interrupt Enable */
    uint32_t REC:4;            /*!< bit: 16..19  Region End bit Condition Detected Interrupt Enable */
    uint32_t RSU:4;            /*!< bit: 20..23  Region Status Updated Interrupt Disable */
    uint32_t URAD:1;           /*!< bit:     24  Undefined Register Access Detection Interrupt Enable */
    uint32_t :7;               /*!< bit: 25..31  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} ICM_IER_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- ICM_IDR : (ICM Offset: 0x14) ( /W 32) Interrupt Disable -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t RHC:4;            /*!< bit:  0.. 3  Region Hash Completed Interrupt Disable */
    uint32_t RDM:4;            /*!< bit:  4.. 7  Region Digest Mismatch Interrupt Disable */
    uint32_t RBE:4;            /*!< bit:  8..11  Region Bus Error Interrupt Disable */
    uint32_t RWC:4;            /*!< bit: 12..15  Region Wrap Condition Detected Interrupt Disable */
    uint32_t REC:4;            /*!< bit: 16..19  Region End bit Condition detected Interrupt Disable */
    uint32_t RSU:4;            /*!< bit: 20..23  Region Status Updated Interrupt Disable */
    uint32_t URAD:1;           /*!< bit:     24  Undefined Register Access Detection Interrupt Disable */
    uint32_t :7;               /*!< bit: 25..31  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} ICM_IDR_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- ICM_IMR : (ICM Offset: 0x18) ( R/ 32) Interrupt Mask -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t RHC:4;            /*!< bit:  0.. 3  Region Hash Completed Interrupt Mask */
    uint32_t RDM:4;            /*!< bit:  4.. 7  Region Digest Mismatch Interrupt Mask */
    uint32_t RBE:4;            /*!< bit:  8..11  Region Bus Error Interrupt Mask    */
    uint32_t RWC:4;            /*!< bit: 12..15  Region Wrap Condition Detected Interrupt Mask */
    uint32_t REC:4;            /*!< bit: 16..19  Region End bit Condition Detected Interrupt Mask */
    uint32_t RSU:4;            /*!< bit: 20..23  Region Status Updated Interrupt Mask */
    uint32_t URAD:1;           /*!< bit:     24  Undefined Register Access Detection Interrupt Mask */
    uint32_t :7;               /*!< bit: 25..31  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} ICM_IMR_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- ICM_ISR : (ICM Offset: 0x1C) ( R/ 32) Interrupt Status -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t RHC:4;            /*!< bit:  0.. 3  Region Hash Completed              */
    uint32_t RDM:4;            /*!< bit:  4.. 7  Region Digest Mismatch             */
    uint32_t RBE:4;            /*!< bit:  8..11  Region Bus Error                   */
    uint32_t RWC:4;            /*!< bit: 12..15  Region Wrap Condition Detected     */
    uint32_t REC:4;            /*!< bit: 16..19  Region End bit Condition Detected  */
    uint32_t RSU:4;            /*!< bit: 20..23  Region Status Updated Detected     */
    uint32_t URAD:1;           /*!< bit:     24  Undefined Register Access Detection Status */
    uint32_t :7;               /*!< bit: 25..31  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} ICM_ISR_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- ICM_UASR : (ICM Offset: 0x20) ( R/ 32) Undefined Access Status -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t URAT:3;           /*!< bit:  0.. 2  Undefined Register Access Trace    */
    uint32_t :29;              /*!< bit:  3..31  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} ICM_UASR_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- ICM_DSCR : (ICM Offset: 0x30) (R/W 32) Region Descriptor Area Start Address -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t :6;               /*!< bit:  0.. 5  Reserved                           */
    uint32_t DASA:26;          /*!< bit:  6..31  Descriptor Area Start Address      */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} ICM_DSCR_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- ICM_HASH : (ICM Offset: 0x34) (R/W 32) Region Hash Area Start Address -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t :7;               /*!< bit:  0.. 6  Reserved                           */
    uint32_t HASA:25;          /*!< bit:  7..31  Hash Area Start Address            */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} ICM_HASH_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- ICM_UIHVAL : (ICM Offset: 0x38) ( /W 32) User Initial Hash Value n -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t VAL:32;           /*!< bit:  0..31  Initial Hash Value                 */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} ICM_UIHVAL_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/** \brief ICM Descriptor SRAM registers */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef struct {
  __IO ICM_RADDR_Type            RADDR;       /**< \brief Offset: 0x00 (R/W 32) Region Start Address */
  __IO ICM_RCFG_Type             RCFG;        /**< \brief Offset: 0x04 (R/W 32) Region Configuration */
  __IO ICM_RCTRL_Type            RCTRL;       /**< \brief Offset: 0x08 (R/W 32) Region Control */
  __IO ICM_RNEXT_Type            RNEXT;       /**< \brief Offset: 0x0C (R/W 32) Region Next Address */
} IcmDescriptor;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/** \brief ICM APB hardware registers */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef struct {
  __IO ICM_CFG_Type              CFG;         /**< \brief Offset: 0x00 (R/W 32) Configuration */
  __O  ICM_CTRL_Type             CTRL;        /**< \brief Offset: 0x04 ( /W 32) Control */
  __I  ICM_SR_Type               SR;          /**< \brief Offset: 0x08 (R/  32) Status */
       RoReg8                    Reserved1[0x4];
  __O  ICM_IER_Type              IER;         /**< \brief Offset: 0x10 ( /W 32) Interrupt Enable */
  __O  ICM_IDR_Type              IDR;         /**< \brief Offset: 0x14 ( /W 32) Interrupt Disable */
  __I  ICM_IMR_Type              IMR;         /**< \brief Offset: 0x18 (R/  32) Interrupt Mask */
  __I  ICM_ISR_Type              ISR;         /**< \brief Offset: 0x1C (R/  32) Interrupt Status */
  __I  ICM_UASR_Type             UASR;        /**< \brief Offset: 0x20 (R/  32) Undefined Access Status */
       RoReg8                    Reserved2[0xC];
  __IO ICM_DSCR_Type             DSCR;        /**< \brief Offset: 0x30 (R/W 32) Region Descriptor Area Start Address */
  __IO ICM_HASH_Type             HASH;        /**< \brief Offset: 0x34 (R/W 32) Region Hash Area Start Address */
  __O  ICM_UIHVAL_Type           UIHVAL[8];   /**< \brief Offset: 0x38 ( /W 32) User Initial Hash Value n */
} Icm;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

#endif /* _MICROCHIP_PIC32CXSG_ICM_COMPONENT_FIXUP_H_ */
