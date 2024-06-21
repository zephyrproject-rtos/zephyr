/*
 * Copyright (c) 2024 Microchip
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _MICROCHIP_PIC32CXSG_OSC32KCTRL_COMPONENT_FIXUP_H_
#define _MICROCHIP_PIC32CXSG_OSC32KCTRL_COMPONENT_FIXUP_H_

/* -------- OSC32KCTRL_INTENCLR : (OSC32KCTRL Offset: 0x00) (R/W 32) Interrupt Enable Clear -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t XOSC32KRDY:1;     /*!< bit:      0  XOSC32K Ready Interrupt Enable     */
    uint32_t :1;               /*!< bit:      1  Reserved                           */
    uint32_t XOSC32KFAIL:1;    /*!< bit:      2  XOSC32K Clock Failure Detector Interrupt Enable */
    uint32_t :29;              /*!< bit:  3..31  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} OSC32KCTRL_INTENCLR_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- OSC32KCTRL_INTENSET : (OSC32KCTRL Offset: 0x04) (R/W 32) Interrupt Enable Set -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t XOSC32KRDY:1;     /*!< bit:      0  XOSC32K Ready Interrupt Enable     */
    uint32_t :1;               /*!< bit:      1  Reserved                           */
    uint32_t XOSC32KFAIL:1;    /*!< bit:      2  XOSC32K Clock Failure Detector Interrupt Enable */
    uint32_t :29;              /*!< bit:  3..31  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} OSC32KCTRL_INTENSET_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- OSC32KCTRL_INTFLAG : (OSC32KCTRL Offset: 0x08) (R/W 32) Interrupt Flag Status and Clear -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { // __I to avoid read-modify-write on write-to-clear register
  struct {
    __I uint32_t XOSC32KRDY:1;     /*!< bit:      0  XOSC32K Ready                      */
    __I uint32_t :1;               /*!< bit:      1  Reserved                           */
    __I uint32_t XOSC32KFAIL:1;    /*!< bit:      2  XOSC32K Clock Failure Detector     */
    __I uint32_t :29;              /*!< bit:  3..31  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} OSC32KCTRL_INTFLAG_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- OSC32KCTRL_STATUS : (OSC32KCTRL Offset: 0x0C) ( R/ 32) Power and Clocks Status -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t XOSC32KRDY:1;     /*!< bit:      0  XOSC32K Ready                      */
    uint32_t :1;               /*!< bit:      1  Reserved                           */
    uint32_t XOSC32KFAIL:1;    /*!< bit:      2  XOSC32K Clock Failure Detector     */
    uint32_t XOSC32KSW:1;      /*!< bit:      3  XOSC32K Clock switch               */
    uint32_t :28;              /*!< bit:  4..31  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} OSC32KCTRL_STATUS_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- OSC32KCTRL_RTCCTRL : (OSC32KCTRL Offset: 0x10) (R/W 8) RTC Clock Selection -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint8_t  RTCSEL:3;         /*!< bit:  0.. 2  RTC Clock Selection                */
    uint8_t  :5;               /*!< bit:  3.. 7  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint8_t reg;                 /*!< Type      used for register access              */
} OSC32KCTRL_RTCCTRL_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- OSC32KCTRL_XOSC32K : (OSC32KCTRL Offset: 0x14) (R/W 16) 32kHz External Crystal Oscillator (XOSC32K) Control -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint16_t :1;               /*!< bit:      0  Reserved                           */
    uint16_t ENABLE:1;         /*!< bit:      1  Oscillator Enable                  */
    uint16_t XTALEN:1;         /*!< bit:      2  Crystal Oscillator Enable          */
    uint16_t EN32K:1;          /*!< bit:      3  32kHz Output Enable                */
    uint16_t EN1K:1;           /*!< bit:      4  1kHz Output Enable                 */
    uint16_t :1;               /*!< bit:      5  Reserved                           */
    uint16_t RUNSTDBY:1;       /*!< bit:      6  Run in Standby                     */
    uint16_t ONDEMAND:1;       /*!< bit:      7  On Demand Control                  */
    uint16_t STARTUP:3;        /*!< bit:  8..10  Oscillator Start-Up Time           */
    uint16_t :1;               /*!< bit:     11  Reserved                           */
    uint16_t WRTLOCK:1;        /*!< bit:     12  Write Lock                         */
    uint16_t CGM:2;            /*!< bit: 13..14  Control Gain Mode                  */
    uint16_t :1;               /*!< bit:     15  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint16_t reg;                /*!< Type      used for register access              */
} OSC32KCTRL_XOSC32K_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- OSC32KCTRL_CFDCTRL : (OSC32KCTRL Offset: 0x16) (R/W 8) Clock Failure Detector Control -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint8_t  CFDEN:1;          /*!< bit:      0  Clock Failure Detector Enable      */
    uint8_t  SWBACK:1;         /*!< bit:      1  Clock Switch Back                  */
    uint8_t  CFDPRESC:1;       /*!< bit:      2  Clock Failure Detector Prescaler   */
    uint8_t  :5;               /*!< bit:  3.. 7  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint8_t reg;                 /*!< Type      used for register access              */
} OSC32KCTRL_CFDCTRL_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- OSC32KCTRL_EVCTRL : (OSC32KCTRL Offset: 0x17) (R/W 8) Event Control -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint8_t  CFDEO:1;          /*!< bit:      0  Clock Failure Detector Event Output Enable */
    uint8_t  :7;               /*!< bit:  1.. 7  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint8_t reg;                 /*!< Type      used for register access              */
} OSC32KCTRL_EVCTRL_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- OSC32KCTRL_OSCULP32K : (OSC32KCTRL Offset: 0x1C) (R/W 32) 32kHz Ultra Low Power Internal Oscillator (OSCULP32K) Control -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t :1;               /*!< bit:      0  Reserved                           */
    uint32_t EN32K:1;          /*!< bit:      1  Enable Out 32k                     */
    uint32_t EN1K:1;           /*!< bit:      2  Enable Out 1k                      */
    uint32_t :5;               /*!< bit:  3.. 7  Reserved                           */
    uint32_t CALIB:6;          /*!< bit:  8..13  Oscillator Calibration             */
    uint32_t :1;               /*!< bit:     14  Reserved                           */
    uint32_t WRTLOCK:1;        /*!< bit:     15  Write Lock                         */
    uint32_t :16;              /*!< bit: 16..31  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} OSC32KCTRL_OSCULP32K_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef struct {
  __IO OSC32KCTRL_INTENCLR_Type  INTENCLR;    /**< \brief Offset: 0x00 (R/W 32) Interrupt Enable Clear */
  __IO OSC32KCTRL_INTENSET_Type  INTENSET;    /**< \brief Offset: 0x04 (R/W 32) Interrupt Enable Set */
  __IO OSC32KCTRL_INTFLAG_Type   INTFLAG;     /**< \brief Offset: 0x08 (R/W 32) Interrupt Flag Status and Clear */
  __I  OSC32KCTRL_STATUS_Type    STATUS;      /**< \brief Offset: 0x0C (R/  32) Power and Clocks Status */
  __IO OSC32KCTRL_RTCCTRL_Type   RTCCTRL;     /**< \brief Offset: 0x10 (R/W  8) RTC Clock Selection */
       RoReg8                    Reserved1[0x3];
  __IO OSC32KCTRL_XOSC32K_Type   XOSC32K;     /**< \brief Offset: 0x14 (R/W 16) 32kHz External Crystal Oscillator (XOSC32K) Control */
  __IO OSC32KCTRL_CFDCTRL_Type   CFDCTRL;     /**< \brief Offset: 0x16 (R/W  8) Clock Failure Detector Control */
  __IO OSC32KCTRL_EVCTRL_Type    EVCTRL;      /**< \brief Offset: 0x17 (R/W  8) Event Control */
       RoReg8                    Reserved2[0x4];
  __IO OSC32KCTRL_OSCULP32K_Type OSCULP32K;   /**< \brief Offset: 0x1C (R/W 32) 32kHz Ultra Low Power Internal Oscillator (OSCULP32K) Control */
} Osc32kctrl;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */



#endif /* _MICROCHIP_PIC32CXSG_OSC32KCTRL_COMPONENT_FIXUP_H_ */
