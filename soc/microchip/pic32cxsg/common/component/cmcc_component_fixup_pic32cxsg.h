/*
 * Copyright (c) 2024 Microchip
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _MICROCHIP_PIC32CXSG_CMCC_COMPONENT_FIXUP_H_
#define _MICROCHIP_PIC32CXSG_CMCC_COMPONENT_FIXUP_H_

/* -------- CMCC_TYPE : (CMCC Offset: 0x00) ( R/ 32) Cache Type Register -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t :1;               /*!< bit:      0  Reserved                           */
    uint32_t DCGCLK:1;         /*!< bit:      1  dynamic Clock Gating supported     */ /* MDS */
    uint32_t :2;               /*!< bit:  2.. 3  Reserved                           */
    uint32_t RRP:1;            /*!< bit:      4  Round Robin Policy supported       */
    uint32_t WAYNUM:2;         /*!< bit:  5.. 6  Number of Way                      */
    uint32_t LCKDOWN:1;        /*!< bit:      7  Lock Down supported                */
    uint32_t CSIZE:3;          /*!< bit:  8..10  Cache Size                         */
    uint32_t CLSIZE:3;         /*!< bit: 11..13  Cache Line Size                    */
    uint32_t :18;              /*!< bit: 14..31  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} CMCC_TYPE_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- CMCC_CFG : (CMCC Offset: 0x04) (R/W 32) Cache Configuration Register -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t :1;               /*!< bit:      0  Reserved                           */
    uint32_t ICDIS:1;          /*!< bit:      1  Instruction Cache Disable          */
    uint32_t DCDIS:1;          /*!< bit:      2  Data Cache Disable                 */
    uint32_t :1;               /*!< bit:      3  Reserved                           */
    uint32_t CSIZESW:3;        /*!< bit:  4.. 6  Cache size configured by software  */
    uint32_t :25;              /*!< bit:  7..31  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} CMCC_CFG_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- CMCC_CTRL : (CMCC Offset: 0x08) ( /W 32) Cache Control Register -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t CEN:1;            /*!< bit:      0  Cache Controller Enable            */
    uint32_t :31;              /*!< bit:  1..31  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} CMCC_CTRL_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- CMCC_SR : (CMCC Offset: 0x0C) ( R/ 32) Cache Status Register -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t CSTS:1;           /*!< bit:      0  Cache Controller Status            */
    uint32_t :31;              /*!< bit:  1..31  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} CMCC_SR_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- CMCC_LCKWAY : (CMCC Offset: 0x10) (R/W 32) Cache Lock per Way Register -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t LCKWAY:4;         /*!< bit:  0.. 3  Lockdown way Register              */
    uint32_t :28;              /*!< bit:  4..31  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} CMCC_LCKWAY_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- CMCC_MAINT0 : (CMCC Offset: 0x20) ( /W 32) Cache Maintenance Register 0 -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t INVALL:1;         /*!< bit:      0  Cache Controller invalidate All    */
    uint32_t :31;              /*!< bit:  1..31  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} CMCC_MAINT0_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */
/* -------- CMCC_MAINT1 : (CMCC Offset: 0x24) ( /W 32) Cache Maintenance Register 1 -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t :4;               /*!< bit:  0.. 3  Reserved                           */
    uint32_t INDEX:8;          /*!< bit:  4..11  Invalidate Index                   */
    uint32_t :16;              /*!< bit: 12..27  Reserved                           */
    uint32_t WAY:4;            /*!< bit: 28..31  Invalidate Way                     */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} CMCC_MAINT1_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- CMCC_MCFG : (CMCC Offset: 0x28) (R/W 32) Cache Monitor Configuration Register -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t MODE:2;           /*!< bit:  0.. 1  Cache Controller Monitor Counter Mode */
    uint32_t :30;              /*!< bit:  2..31  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} CMCC_MCFG_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- CMCC_MEN : (CMCC Offset: 0x2C) (R/W 32) Cache Monitor Enable Register -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t MENABLE:1;        /*!< bit:      0  Cache Controller Monitor Enable    */
    uint32_t :31;              /*!< bit:  1..31  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} CMCC_MEN_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- CMCC_MCTRL : (CMCC Offset: 0x30) ( /W 32) Cache Monitor Control Register -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t SWRST:1;          /*!< bit:      0  Cache Controller Software Reset    */
    uint32_t :31;              /*!< bit:  1..31  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} CMCC_MCTRL_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- CMCC_MSR : (CMCC Offset: 0x34) ( R/ 32) Cache Monitor Status Register -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t EVENT_CNT:32;     /*!< bit:  0..31  Monitor Event Counter              */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} CMCC_MSR_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef struct {
  __I  CMCC_TYPE_Type            TYPE;        /**< \brief Offset: 0x00 (R/  32) Cache Type Register */
  __IO CMCC_CFG_Type             CFG;         /**< \brief Offset: 0x04 (R/W 32) Cache Configuration Register */
  __O  CMCC_CTRL_Type            CTRL;        /**< \brief Offset: 0x08 ( /W 32) Cache Control Register */
  __I  CMCC_SR_Type              SR;          /**< \brief Offset: 0x0C (R/  32) Cache Status Register */
  __IO CMCC_LCKWAY_Type          LCKWAY;      /**< \brief Offset: 0x10 (R/W 32) Cache Lock per Way Register */
       RoReg8                    Reserved1[0xC];
  __O  CMCC_MAINT0_Type          MAINT0;      /**< \brief Offset: 0x20 ( /W 32) Cache Maintenance Register 0 */
  __O  CMCC_MAINT1_Type          MAINT1;      /**< \brief Offset: 0x24 ( /W 32) Cache Maintenance Register 1 */
  __IO CMCC_MCFG_Type            MCFG;        /**< \brief Offset: 0x28 (R/W 32) Cache Monitor Configuration Register */
  __IO CMCC_MEN_Type             MEN;         /**< \brief Offset: 0x2C (R/W 32) Cache Monitor Enable Register */
  __O  CMCC_MCTRL_Type           MCTRL;       /**< \brief Offset: 0x30 ( /W 32) Cache Monitor Control Register */
  __I  CMCC_MSR_Type             MSR;         /**< \brief Offset: 0x34 (R/  32) Cache Monitor Status Register */
} Cmcc;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

#endif /* _MICROCHIP_PIC32CXSG_CMCC_COMPONENT_FIXUP_H_ */
