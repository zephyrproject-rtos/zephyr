/*
 * Copyright (c) 2024 Microchip
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _MICROCHIP_PIC32CXSG_SUPC_COMPONENT_FIXUP_H_
#define _MICROCHIP_PIC32CXSG_SUPC_COMPONENT_FIXUP_H_

/* -------- SUPC_INTENCLR : (SUPC Offset: 0x00) (R/W 32) Interrupt Enable Clear -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t BOD33RDY:1;       /*!< bit:      0  BOD33 Ready                        */
    uint32_t BOD33DET:1;       /*!< bit:      1  BOD33 Detection                    */
    uint32_t B33SRDY:1;        /*!< bit:      2  BOD33 Synchronization Ready        */
    uint32_t :5;               /*!< bit:  3.. 7  Reserved                           */
    uint32_t VREGRDY:1;        /*!< bit:      8  Voltage Regulator Ready            */
    uint32_t :1;               /*!< bit:      9  Reserved                           */
    uint32_t VCORERDY:1;       /*!< bit:     10  VDDCORE Ready                      */
    uint32_t :21;              /*!< bit: 11..31  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} SUPC_INTENCLR_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- SUPC_INTENSET : (SUPC Offset: 0x04) (R/W 32) Interrupt Enable Set -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t BOD33RDY:1;       /*!< bit:      0  BOD33 Ready                        */
    uint32_t BOD33DET:1;       /*!< bit:      1  BOD33 Detection                    */
    uint32_t B33SRDY:1;        /*!< bit:      2  BOD33 Synchronization Ready        */
    uint32_t :5;               /*!< bit:  3.. 7  Reserved                           */
    uint32_t VREGRDY:1;        /*!< bit:      8  Voltage Regulator Ready            */
    uint32_t :1;               /*!< bit:      9  Reserved                           */
    uint32_t VCORERDY:1;       /*!< bit:     10  VDDCORE Ready                      */
    uint32_t :21;              /*!< bit: 11..31  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} SUPC_INTENSET_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- SUPC_INTFLAG : (SUPC Offset: 0x08) (R/W 32) Interrupt Flag Status and Clear -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { // __I to avoid read-modify-write on write-to-clear register
  struct {
    __I uint32_t BOD33RDY:1;       /*!< bit:      0  BOD33 Ready                        */
    __I uint32_t BOD33DET:1;       /*!< bit:      1  BOD33 Detection                    */
    __I uint32_t B33SRDY:1;        /*!< bit:      2  BOD33 Synchronization Ready        */
    __I uint32_t :5;               /*!< bit:  3.. 7  Reserved                           */
    __I uint32_t VREGRDY:1;        /*!< bit:      8  Voltage Regulator Ready            */
    __I uint32_t :1;               /*!< bit:      9  Reserved                           */
    __I uint32_t VCORERDY:1;       /*!< bit:     10  VDDCORE Ready                      */
    __I uint32_t :21;              /*!< bit: 11..31  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} SUPC_INTFLAG_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- SUPC_STATUS : (SUPC Offset: 0x0C) ( R/ 32) Power and Clocks Status -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t BOD33RDY:1;       /*!< bit:      0  BOD33 Ready                        */
    uint32_t BOD33DET:1;       /*!< bit:      1  BOD33 Detection                    */
    uint32_t B33SRDY:1;        /*!< bit:      2  BOD33 Synchronization Ready        */
    uint32_t :5;               /*!< bit:  3.. 7  Reserved                           */
    uint32_t VREGRDY:1;        /*!< bit:      8  Voltage Regulator Ready            */
    uint32_t :1;               /*!< bit:      9  Reserved                           */
    uint32_t VCORERDY:1;       /*!< bit:     10  VDDCORE Ready                      */
    uint32_t :21;              /*!< bit: 11..31  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} SUPC_STATUS_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- SUPC_BOD33 : (SUPC Offset: 0x10) (R/W 32) BOD33 Control -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t :1;               /*!< bit:      0  Reserved                           */
    uint32_t ENABLE:1;         /*!< bit:      1  Enable                             */
    uint32_t ACTION:2;         /*!< bit:  2.. 3  Action when Threshold Crossed      */
    uint32_t STDBYCFG:1;       /*!< bit:      4  Configuration in Standby mode      */
    uint32_t RUNSTDBY:1;       /*!< bit:      5  Run in Standby mode                */
    uint32_t RUNHIB:1;         /*!< bit:      6  Run in Hibernate mode              */
    uint32_t RUNBKUP:1;        /*!< bit:      7  Run in Backup mode                 */
    uint32_t HYST:4;           /*!< bit:  8..11  Hysteresis value                   */
    uint32_t PSEL:3;           /*!< bit: 12..14  Prescaler Select                   */
    uint32_t :1;               /*!< bit:     15  Reserved                           */
    uint32_t LEVEL:8;          /*!< bit: 16..23  Threshold Level for VDD            */
    uint32_t VBATLEVEL:8;      /*!< bit: 24..31  Threshold Level in battery backup sleep mode for VBAT */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} SUPC_BOD33_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- SUPC_VREG : (SUPC Offset: 0x18) (R/W 32) VREG Control -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t :1;               /*!< bit:      0  Reserved                           */
    uint32_t ENABLE:1;         /*!< bit:      1  Enable                             */
    uint32_t SEL:1;            /*!< bit:      2  Voltage Regulator Selection        */
    uint32_t :4;               /*!< bit:  3.. 6  Reserved                           */
    uint32_t RUNBKUP:1;        /*!< bit:      7  Run in Backup mode                 */
    uint32_t :8;               /*!< bit:  8..15  Reserved                           */
    uint32_t VSEN:1;           /*!< bit:     16  Voltage Scaling Enable             */
    uint32_t :7;               /*!< bit: 17..23  Reserved                           */
    uint32_t VSPER:3;          /*!< bit: 24..26  Voltage Scaling Period             */
    uint32_t :5;               /*!< bit: 27..31  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} SUPC_VREG_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- SUPC_VREF : (SUPC Offset: 0x1C) (R/W 32) VREF Control -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t :1;               /*!< bit:      0  Reserved                           */
    uint32_t :1;               /*!< bit:      1  Reserved   						*/
    uint32_t VREFOE:1;         /*!< bit:      2  Voltage Reference Output Enable    */
    uint32_t :1;          	   /*!< bit:      3  Reserved   						*/
    uint32_t :2;               /*!< bit:  4.. 5  Reserved                           */
    uint32_t RUNSTDBY:1;       /*!< bit:      6  Run during Standby                 */
    uint32_t ONDEMAND:1;       /*!< bit:      7  On Demand Contrl                   */
    uint32_t :8;               /*!< bit:  8..15  Reserved                           */
    uint32_t SEL:4;            /*!< bit: 16..19  Voltage Reference Selection        */
    uint32_t :12;              /*!< bit: 20..31  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} SUPC_VREF_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- SUPC_BBPS : (SUPC Offset: 0x20) (R/W 32) Battery Backup Power Switch -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t CONF:1;           /*!< bit:      0  Battery Backup Configuration       */
    uint32_t :1;               /*!< bit:      1  Reserved                           */
    uint32_t WAKEEN:1;         /*!< bit:      2  Wake Enable                        */
    uint32_t :29;              /*!< bit:  3..31  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} SUPC_BBPS_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- SUPC_BKOUT : (SUPC Offset: 0x24) (R/W 32) Backup Output Control -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t EN:2;             /*!< bit:  0.. 1  Enable Output                      */
    uint32_t :6;               /*!< bit:  2.. 7  Reserved                           */
    uint32_t CLR:2;            /*!< bit:  8.. 9  Clear Output                       */
    uint32_t :6;               /*!< bit: 10..15  Reserved                           */
    uint32_t SET:2;            /*!< bit: 16..17  Set Output                         */
    uint32_t :6;               /*!< bit: 18..23  Reserved                           */
    uint32_t RTCTGL:2;         /*!< bit: 24..25  RTC Toggle Output                  */
    uint32_t :6;               /*!< bit: 26..31  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} SUPC_BKOUT_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- SUPC_BKIN : (SUPC Offset: 0x28) ( R/ 32) Backup Input Control -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t BKIN:8;           /*!< bit:  0.. 7  Backup Input Value                 */
    uint32_t :24;              /*!< bit:  8..31  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} SUPC_BKIN_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/** \brief SUPC hardware registers */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef struct {
  __IO SUPC_INTENCLR_Type        INTENCLR;    /**< \brief Offset: 0x00 (R/W 32) Interrupt Enable Clear */
  __IO SUPC_INTENSET_Type        INTENSET;    /**< \brief Offset: 0x04 (R/W 32) Interrupt Enable Set */
  __IO SUPC_INTFLAG_Type         INTFLAG;     /**< \brief Offset: 0x08 (R/W 32) Interrupt Flag Status and Clear */
  __I  SUPC_STATUS_Type          STATUS;      /**< \brief Offset: 0x0C (R/  32) Power and Clocks Status */
  __IO SUPC_BOD33_Type           BOD33;       /**< \brief Offset: 0x10 (R/W 32) BOD33 Control */
       RoReg8                    Reserved1[0x4];
  __IO SUPC_VREG_Type            VREG;        /**< \brief Offset: 0x18 (R/W 32) VREG Control */
  __IO SUPC_VREF_Type            VREF;        /**< \brief Offset: 0x1C (R/W 32) VREF Control */
  __IO SUPC_BBPS_Type            BBPS;        /**< \brief Offset: 0x20 (R/W 32) Battery Backup Power Switch */
  __IO SUPC_BKOUT_Type           BKOUT;       /**< \brief Offset: 0x24 (R/W 32) Backup Output Control */
  __I  SUPC_BKIN_Type            BKIN;        /**< \brief Offset: 0x28 (R/  32) Backup Input Control */
} Supc;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

#endif /* _MICROCHIP_PIC32CXSG_SUPC_COMPONENT_FIXUP_H_ */
