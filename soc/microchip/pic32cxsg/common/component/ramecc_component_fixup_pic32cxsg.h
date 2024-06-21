/*
 * Copyright (c) 2024 Microchip
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _MICROCHIP_PIC32CXSG_RAMECC_COMPONENT_FIXUP_H_
#define _MICROCHIP_PIC32CXSG_RAMECC_COMPONENT_FIXUP_H_

/* -------- RAMECC_INTENCLR : (RAMECC Offset: 0x00) (R/W 8) Interrupt Enable Clear -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint8_t  SINGLEE:1;        /*!< bit:      0  Single Bit ECC Error Interrupt Enable Clear */
    uint8_t  DUALE:1;          /*!< bit:      1  Dual Bit ECC Error Interrupt Enable Clear */
    uint8_t  :6;               /*!< bit:  2.. 7  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint8_t reg;                 /*!< Type      used for register access              */
} RAMECC_INTENCLR_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- RAMECC_INTENSET : (RAMECC Offset: 0x01) (R/W 8) Interrupt Enable Set -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint8_t  SINGLEE:1;        /*!< bit:      0  Single Bit ECC Error Interrupt Enable Set */
    uint8_t  DUALE:1;          /*!< bit:      1  Dual Bit ECC Error Interrupt Enable Set */
    uint8_t  :6;               /*!< bit:  2.. 7  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint8_t reg;                 /*!< Type      used for register access              */
} RAMECC_INTENSET_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- RAMECC_INTFLAG : (RAMECC Offset: 0x02) (R/W 8) Interrupt Flag -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { // __I to avoid read-modify-write on write-to-clear register
  struct {
    __I uint8_t  SINGLEE:1;        /*!< bit:      0  Single Bit ECC Error Interrupt     */
    __I uint8_t  DUALE:1;          /*!< bit:      1  Dual Bit ECC Error Interrupt       */
    __I uint8_t  :6;               /*!< bit:  2.. 7  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint8_t reg;                 /*!< Type      used for register access              */
} RAMECC_INTFLAG_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- RAMECC_STATUS : (RAMECC Offset: 0x03) ( R/ 8) Status -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint8_t  ECCDIS:1;         /*!< bit:      0  ECC Disable                        */
    uint8_t  :7;               /*!< bit:  1.. 7  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint8_t reg;                 /*!< Type      used for register access              */
} RAMECC_STATUS_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- RAMECC_ERRADDR : (RAMECC Offset: 0x04) ( R/ 32) Error Address -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t ERRADDR:17;       /*!< bit:  0..16  Error Address                      */
    uint32_t :15;              /*!< bit: 17..31  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} RAMECC_ERRADDR_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- RAMECC_DBGCTRL : (RAMECC Offset: 0x0F) (R/W 8) Debug Control -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint8_t  ECCDIS:1;         /*!< bit:      0  ECC Disable                        */
    uint8_t  ECCELOG:1;        /*!< bit:      1  ECC Error Log                      */
    uint8_t  :6;               /*!< bit:  2.. 7  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint8_t reg;                 /*!< Type      used for register access              */
} RAMECC_DBGCTRL_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/** \brief RAMECC hardware registers */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef struct {
  __IO RAMECC_INTENCLR_Type      INTENCLR;    /**< \brief Offset: 0x0 (R/W  8) Interrupt Enable Clear */
  __IO RAMECC_INTENSET_Type      INTENSET;    /**< \brief Offset: 0x1 (R/W  8) Interrupt Enable Set */
  __IO RAMECC_INTFLAG_Type       INTFLAG;     /**< \brief Offset: 0x2 (R/W  8) Interrupt Flag */
  __I  RAMECC_STATUS_Type        STATUS;      /**< \brief Offset: 0x3 (R/   8) Status */
  __I  RAMECC_ERRADDR_Type       ERRADDR;     /**< \brief Offset: 0x4 (R/  32) Error Address */
       RoReg8                    Reserved1[0x7];
  __IO RAMECC_DBGCTRL_Type       DBGCTRL;     /**< \brief Offset: 0xF (R/W  8) Debug Control */
} Ramecc;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

#endif /* _MICROCHIP_PIC32CXSG_RAMECC_COMPONENT_FIXUP_H_ */
