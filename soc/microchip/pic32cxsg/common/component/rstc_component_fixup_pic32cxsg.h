/*
 * Copyright (c) 2024 Microchip
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _MICROCHIP_PIC32CXSG_RSTC_COMPONENT_FIXUP_H_
#define _MICROCHIP_PIC32CXSG_RSTC_COMPONENT_FIXUP_H_

/* -------- RSTC_RCAUSE : (RSTC Offset: 0x00) ( R/ 8) Reset Cause -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint8_t  POR:1;            /*!< bit:      0  Power On Reset                     */
    uint8_t  BODCORE:1;        /*!< bit:      1  Brown Out CORE Detector Reset      */
    uint8_t  BODVDD:1;         /*!< bit:      2  Brown Out VDD Detector Reset       */
    uint8_t  NVM:1;            /*!< bit:      3  NVM Reset                          */
    uint8_t  EXT:1;            /*!< bit:      4  External Reset                     */
    uint8_t  WDT_RST:1;        /*!< bit:      5  Watchdog Reset                     */
    uint8_t  SYST:1;           /*!< bit:      6  System Reset Request               */
    uint8_t  BACKUP:1;         /*!< bit:      7  Backup Reset                       */
  } bit;                       /*!< Structure used for bit  access                  */
  uint8_t reg;                 /*!< Type      used for register access              */
} RSTC_RCAUSE_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- RSTC_BKUPEXIT : (RSTC Offset: 0x02) ( R/ 8) Backup Exit Source -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint8_t  :1;               /*!< bit:      0  Reserved                           */
    uint8_t  RTC_INT:1;        /*!< bit:      1  Real Timer Counter Interrupt       */
    uint8_t  BBPS:1;           /*!< bit:      2  Battery Backup Power Switch        */
    uint8_t  :4;               /*!< bit:  3.. 6  Reserved                           */
    uint8_t  HIB:1;            /*!< bit:      7  Hibernate                          */
  } bit;                       /*!< Structure used for bit  access                  */
  uint8_t reg;                 /*!< Type      used for register access              */
} RSTC_BKUPEXIT_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/** \brief RSTC hardware registers */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef struct {
  __I  RSTC_RCAUSE_Type          RCAUSE;      /**< \brief Offset: 0x00 (R/   8) Reset Cause */
       RoReg8                    Reserved1[0x1];
  __I  RSTC_BKUPEXIT_Type        BKUPEXIT;    /**< \brief Offset: 0x02 (R/   8) Backup Exit Source */
} Rstc;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

#endif /* _MICROCHIP_PIC32CXSG_RSTC_COMPONENT_FIXUP_H_ */
