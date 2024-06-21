/*
 * Copyright (c) 2024 Microchip
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _MICROCHIP_PIC32CXSG_PM_COMPONENT_FIXUP_H_
#define _MICROCHIP_PIC32CXSG_PM_COMPONENT_FIXUP_H_

/* -------- PM_CTRLA : (PM Offset: 0x00) (R/W 8) Control A -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint8_t  :2;               /*!< bit:  0.. 1  Reserved                           */
    uint8_t  IORET:1;          /*!< bit:      2  I/O Retention                      */
    uint8_t  :5;               /*!< bit:  3.. 7  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint8_t reg;                 /*!< Type      used for register access              */
} PM_CTRLA_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- PM_SLEEPCFG : (PM Offset: 0x01) (R/W 8) Sleep Configuration -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint8_t  SLEEPMODE:3;      /*!< bit:  0.. 2  Sleep Mode                         */
    uint8_t  :5;               /*!< bit:  3.. 7  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint8_t reg;                 /*!< Type      used for register access              */
} PM_SLEEPCFG_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- PM_INTENCLR : (PM Offset: 0x04) (R/W 8) Interrupt Enable Clear -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint8_t  SLEEPRDY:1;       /*!< bit:      0  Sleep Mode Entry Ready Enable      */
    uint8_t  :7;               /*!< bit:  1.. 7  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint8_t reg;                 /*!< Type      used for register access              */
} PM_INTENCLR_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- PM_INTENSET : (PM Offset: 0x05) (R/W 8) Interrupt Enable Set -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint8_t  SLEEPRDY:1;       /*!< bit:      0  Sleep Mode Entry Ready Enable      */
    uint8_t  :7;               /*!< bit:  1.. 7  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint8_t reg;                 /*!< Type      used for register access              */
} PM_INTENSET_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- PM_INTFLAG : (PM Offset: 0x06) (R/W 8) Interrupt Flag Status and Clear -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { // __I to avoid read-modify-write on write-to-clear register
  struct {
    __I uint8_t  SLEEPRDY:1;       /*!< bit:      0  Sleep Mode Entry Ready             */
    __I uint8_t  :7;               /*!< bit:  1.. 7  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint8_t reg;                 /*!< Type      used for register access              */
} PM_INTFLAG_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- PM_STDBYCFG : (PM Offset: 0x08) (R/W 8) Standby Configuration -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint8_t  RAMCFG:2;         /*!< bit:  0.. 1  Ram Configuration                  */
    uint8_t  :2;               /*!< bit:  2.. 3  Reserved                           */
    uint8_t  FASTWKUP:2;       /*!< bit:  4.. 5  Fast Wakeup                        */
    uint8_t  :2;               /*!< bit:  6.. 7  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint8_t reg;                 /*!< Type      used for register access              */
} PM_STDBYCFG_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- PM_HIBCFG : (PM Offset: 0x09) (R/W 8) Hibernate Configuration -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint8_t  RAMCFG:2;         /*!< bit:  0.. 1  Ram Configuration                  */
    uint8_t  BRAMCFG:2;        /*!< bit:  2.. 3  Backup Ram Configuration           */
    uint8_t  :4;               /*!< bit:  4.. 7  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint8_t reg;                 /*!< Type      used for register access              */
} PM_HIBCFG_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- PM_BKUPCFG : (PM Offset: 0x0A) (R/W 8) Backup Configuration -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint8_t  BRAMCFG:2;        /*!< bit:  0.. 1  Ram Configuration                  */
    uint8_t  :6;               /*!< bit:  2.. 7  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint8_t reg;                 /*!< Type      used for register access              */
} PM_BKUPCFG_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- PM_PWSAKDLY : (PM Offset: 0x12) (R/W 8) Power Switch Acknowledge Delay -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint8_t  DLYVAL:7;         /*!< bit:  0.. 6  Delay Value                        */
    uint8_t  IGNACK:1;         /*!< bit:      7  Ignore Acknowledge                 */
  } bit;                       /*!< Structure used for bit  access                  */
  uint8_t reg;                 /*!< Type      used for register access              */
} PM_PWSAKDLY_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/** \brief PM hardware registers */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef struct {
  __IO PM_CTRLA_Type             CTRLA;       /**< \brief Offset: 0x00 (R/W  8) Control A */
  __IO PM_SLEEPCFG_Type          SLEEPCFG;    /**< \brief Offset: 0x01 (R/W  8) Sleep Configuration */
       RoReg8                    Reserved1[0x2];
  __IO PM_INTENCLR_Type          INTENCLR;    /**< \brief Offset: 0x04 (R/W  8) Interrupt Enable Clear */
  __IO PM_INTENSET_Type          INTENSET;    /**< \brief Offset: 0x05 (R/W  8) Interrupt Enable Set */
  __IO PM_INTFLAG_Type           INTFLAG;     /**< \brief Offset: 0x06 (R/W  8) Interrupt Flag Status and Clear */
       RoReg8                    Reserved2[0x1];
  __IO PM_STDBYCFG_Type          STDBYCFG;    /**< \brief Offset: 0x08 (R/W  8) Standby Configuration */
  __IO PM_HIBCFG_Type            HIBCFG;      /**< \brief Offset: 0x09 (R/W  8) Hibernate Configuration */
  __IO PM_BKUPCFG_Type           BKUPCFG;     /**< \brief Offset: 0x0A (R/W  8) Backup Configuration */
       RoReg8                    Reserved3[0x7];
  __IO PM_PWSAKDLY_Type          PWSAKDLY;    /**< \brief Offset: 0x12 (R/W  8) Power Switch Acknowledge Delay */
} Pm;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */




#endif /* _MICROCHIP_PIC32CXSG_PM_COMPONENT_FIXUP_H_ */
