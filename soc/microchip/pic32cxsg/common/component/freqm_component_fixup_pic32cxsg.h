/*
 * Copyright (c) 2024 Microchip
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _MICROCHIP_PIC32CXSG_FREQM_COMPONENT_FIXUP_H_
#define _MICROCHIP_PIC32CXSG_FREQM_COMPONENT_FIXUP_H_

/* -------- FREQM_CTRLA : (FREQM Offset: 0x00) (R/W 8) Control A Register -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint8_t  SWRST:1;          /*!< bit:      0  Software Reset                     */
    uint8_t  ENABLE:1;         /*!< bit:      1  Enable                             */
    uint8_t  :6;               /*!< bit:  2.. 7  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint8_t reg;                 /*!< Type      used for register access              */
} FREQM_CTRLA_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- FREQM_CTRLB : (FREQM Offset: 0x01) ( /W 8) Control B Register -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint8_t  START:1;          /*!< bit:      0  Start Measurement                  */
    uint8_t  :7;               /*!< bit:  1.. 7  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint8_t reg;                 /*!< Type      used for register access              */
} FREQM_CTRLB_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- FREQM_CFGA : (FREQM Offset: 0x02) (R/W 16) Config A register -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint16_t REFNUM:8;         /*!< bit:  0.. 7  Number of Reference Clock Cycles   */
    uint16_t :8;               /*!< bit:  8..15  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint16_t reg;                /*!< Type      used for register access              */
} FREQM_CFGA_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- FREQM_INTENCLR : (FREQM Offset: 0x08) (R/W 8) Interrupt Enable Clear Register -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint8_t  DONE:1;           /*!< bit:      0  Measurement Done Interrupt Enable  */
    uint8_t  :7;               /*!< bit:  1.. 7  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint8_t reg;                 /*!< Type      used for register access              */
} FREQM_INTENCLR_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- FREQM_INTENSET : (FREQM Offset: 0x09) (R/W 8) Interrupt Enable Set Register -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint8_t  DONE:1;           /*!< bit:      0  Measurement Done Interrupt Enable  */
    uint8_t  :7;               /*!< bit:  1.. 7  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint8_t reg;                 /*!< Type      used for register access              */
} FREQM_INTENSET_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- FREQM_INTFLAG : (FREQM Offset: 0x0A) (R/W 8) Interrupt Flag Register -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { // __I to avoid read-modify-write on write-to-clear register
  struct {
    __I uint8_t  DONE:1;           /*!< bit:      0  Measurement Done                   */
    __I uint8_t  :7;               /*!< bit:  1.. 7  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint8_t reg;                 /*!< Type      used for register access              */
} FREQM_INTFLAG_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- FREQM_STATUS : (FREQM Offset: 0x0B) (R/W 8) Status Register -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint8_t  BUSY:1;           /*!< bit:      0  FREQM Status                       */
    uint8_t  OVF:1;            /*!< bit:      1  Sticky Count Value Overflow        */
    uint8_t  :6;               /*!< bit:  2.. 7  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint8_t reg;                 /*!< Type      used for register access              */
} FREQM_STATUS_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- FREQM_SYNCBUSY : (FREQM Offset: 0x0C) ( R/ 32) Synchronization Busy Register -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t SWRST:1;          /*!< bit:      0  Software Reset                     */
    uint32_t ENABLE:1;         /*!< bit:      1  Enable                             */
    uint32_t :30;              /*!< bit:  2..31  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} FREQM_SYNCBUSY_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- FREQM_VALUE : (FREQM Offset: 0x10) ( R/ 32) Count Value Register -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t VALUE:24;         /*!< bit:  0..23  Measurement Value                  */
    uint32_t :8;               /*!< bit: 24..31  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} FREQM_VALUE_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/** \brief FREQM hardware registers */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef struct {
  __IO FREQM_CTRLA_Type          CTRLA;       /**< \brief Offset: 0x00 (R/W  8) Control A Register */
  __O  FREQM_CTRLB_Type          CTRLB;       /**< \brief Offset: 0x01 ( /W  8) Control B Register */
  __IO FREQM_CFGA_Type           CFGA;        /**< \brief Offset: 0x02 (R/W 16) Config A register */
       RoReg8                    Reserved1[0x4];
  __IO FREQM_INTENCLR_Type       INTENCLR;    /**< \brief Offset: 0x08 (R/W  8) Interrupt Enable Clear Register */
  __IO FREQM_INTENSET_Type       INTENSET;    /**< \brief Offset: 0x09 (R/W  8) Interrupt Enable Set Register */
  __IO FREQM_INTFLAG_Type        INTFLAG;     /**< \brief Offset: 0x0A (R/W  8) Interrupt Flag Register */
  __IO FREQM_STATUS_Type         STATUS;      /**< \brief Offset: 0x0B (R/W  8) Status Register */
  __I  FREQM_SYNCBUSY_Type       SYNCBUSY;    /**< \brief Offset: 0x0C (R/  32) Synchronization Busy Register */
  __I  FREQM_VALUE_Type          VALUE;       /**< \brief Offset: 0x10 (R/  32) Count Value Register */
} Freqm;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

#endif /* _MICROCHIP_PIC32CXSG_FREQM_COMPONENT_FIXUP_H_ */
