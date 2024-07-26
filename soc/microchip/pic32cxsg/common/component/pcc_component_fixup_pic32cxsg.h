/*
 * Copyright (c) 2024 Microchip
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _MICROCHIP_PIC32CXSG_PCC_COMPONENT_FIXUP_H_
#define _MICROCHIP_PIC32CXSG_PCC_COMPONENT_FIXUP_H_

/* -------- PCC_MR : (PCC Offset: 0x00) (R/W 32) Mode Register -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t PCEN:1;           /*!< bit:      0  Parallel Capture Enable            */
    uint32_t :3;               /*!< bit:  1.. 3  Reserved                           */
    uint32_t DSIZE:2;          /*!< bit:  4.. 5  Data size                          */
    uint32_t :2;               /*!< bit:  6.. 7  Reserved                           */
    uint32_t SCALE:1;          /*!< bit:      8  Scale data                         */
    uint32_t ALWYS:1;          /*!< bit:      9  Always Sampling                    */
    uint32_t HALFS:1;          /*!< bit:     10  Half Sampling                      */
    uint32_t FRSTS:1;          /*!< bit:     11  First sample                       */
    uint32_t :4;               /*!< bit: 12..15  Reserved                           */
    uint32_t ISIZE:3;          /*!< bit: 16..18  Input Data Size                    */
    uint32_t :11;              /*!< bit: 19..29  Reserved                           */
    uint32_t CID:2;            /*!< bit: 30..31  Clear If Disabled                  */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} PCC_MR_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- PCC_IER : (PCC Offset: 0x04) ( /W 32) Interrupt Enable Register -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t DRDY:1;           /*!< bit:      0  Data Ready Interrupt Enable        */
    uint32_t OVRE:1;           /*!< bit:      1  Overrun Error Interrupt Enable     */
    uint32_t :30;              /*!< bit:  2..31  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} PCC_IER_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */
/* -------- PCC_IDR : (PCC Offset: 0x08) ( /W 32) Interrupt Disable Register -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t DRDY:1;           /*!< bit:      0  Data Ready Interrupt Disable       */
    uint32_t OVRE:1;           /*!< bit:      1  Overrun Error Interrupt Disable    */
    uint32_t :30;              /*!< bit:  2..31  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} PCC_IDR_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- PCC_IMR : (PCC Offset: 0x0C) ( R/ 32) Interrupt Mask Register -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t DRDY:1;           /*!< bit:      0  Data Ready Interrupt Mask          */
    uint32_t OVRE:1;           /*!< bit:      1  Overrun Error Interrupt Mask       */
    uint32_t :30;              /*!< bit:  2..31  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} PCC_IMR_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- PCC_ISR : (PCC Offset: 0x10) ( R/ 32) Interrupt Status Register -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t DRDY:1;           /*!< bit:      0  Data Ready Interrupt Status        */
    uint32_t OVRE:1;           /*!< bit:      1  Overrun Error Interrupt Status     */
    uint32_t :30;              /*!< bit:  2..31  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} PCC_ISR_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- PCC_RHR : (PCC Offset: 0x14) ( R/ 32) Reception Holding Register -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t RDATA:32;         /*!< bit:  0..31  Reception Data                     */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} PCC_RHR_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- PCC_WPMR : (PCC Offset: 0xE0) (R/W 32) Write Protection Mode Register -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t WPEN:1;           /*!< bit:      0  Write Protection Enable            */
    uint32_t :7;               /*!< bit:  1.. 7  Reserved                           */
    uint32_t WPKEY:24;         /*!< bit:  8..31  Write Protection Key               */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} PCC_WPMR_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- PCC_WPSR : (PCC Offset: 0xE4) ( R/ 32) Write Protection Status Register -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t WPVS:1;           /*!< bit:      0  Write Protection Violation Source  */
    uint32_t :7;               /*!< bit:  1.. 7  Reserved                           */
    uint32_t WPVSRC:16;        /*!< bit:  8..23  Write Protection Violation Status  */
    uint32_t :8;               /*!< bit: 24..31  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} PCC_WPSR_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/** \brief PCC hardware registers */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef struct {
  __IO PCC_MR_Type               MR;          /**< \brief Offset: 0x00 (R/W 32) Mode Register */
  __O  PCC_IER_Type              IER;         /**< \brief Offset: 0x04 ( /W 32) Interrupt Enable Register */
  __O  PCC_IDR_Type              IDR;         /**< \brief Offset: 0x08 ( /W 32) Interrupt Disable Register */
  __I  PCC_IMR_Type              IMR;         /**< \brief Offset: 0x0C (R/  32) Interrupt Mask Register */
  __I  PCC_ISR_Type              ISR;         /**< \brief Offset: 0x10 (R/  32) Interrupt Status Register */
  __I  PCC_RHR_Type              RHR;         /**< \brief Offset: 0x14 (R/  32) Reception Holding Register */
       RoReg8                    Reserved1[0xC8];
  __IO PCC_WPMR_Type             WPMR;        /**< \brief Offset: 0xE0 (R/W 32) Write Protection Mode Register */
  __I  PCC_WPSR_Type             WPSR;        /**< \brief Offset: 0xE4 (R/  32) Write Protection Status Register */
} Pcc;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

#endif /* _MICROCHIP_PIC32CXSG_PCC_COMPONENT_FIXUP_H_ */
