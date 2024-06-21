/*
 * Copyright (c) 2024 Microchip
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _MICROCHIP_PIC32CXSG_OSCCTRL_COMPONENT_FIXUP_H_
#define _MICROCHIP_PIC32CXSG_OSCCTRL_COMPONENT_FIXUP_H_

/* -------- OSCCTRL_DPLLCTRLA : (OSCCTRL Offset: 0x30) (R/W 8) DPLL Control A -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint8_t  :1;               /*!< bit:      0  Reserved                           */
    uint8_t  ENABLE:1;         /*!< bit:      1  DPLL Enable                        */
    uint8_t  :4;               /*!< bit:  2.. 5  Reserved                           */
    uint8_t  RUNSTDBY:1;       /*!< bit:      6  Run in Standby                     */
    uint8_t  ONDEMAND:1;       /*!< bit:      7  On Demand Control                  */
  } bit;                       /*!< Structure used for bit  access                  */
  uint8_t reg;                 /*!< Type      used for register access              */
} OSCCTRL_DPLLCTRLA_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- OSCCTRL_DPLLRATIO : (OSCCTRL Offset: 0x34) (R/W 32) DPLL Ratio Control -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t LDR:13;           /*!< bit:  0..12  Loop Divider Ratio                 */
    uint32_t :3;               /*!< bit: 13..15  Reserved                           */
    uint32_t LDRFRAC:5;        /*!< bit: 16..20  Loop Divider Ratio Fractional Part */
    uint32_t :11;              /*!< bit: 21..31  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} OSCCTRL_DPLLRATIO_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- OSCCTRL_DPLLCTRLB : (OSCCTRL Offset: 0x38) (R/W 32) DPLL Control B -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t FILTER:4;         /*!< bit:  0.. 3  Proportional Integral Filter Selection */
    uint32_t WUF:1;            /*!< bit:      4  Wake Up Fast                       */
    uint32_t REFCLK:3;         /*!< bit:  5.. 7  Reference Clock Selection          */
    uint32_t LTIME:3;          /*!< bit:  8..10  Lock Time                          */
    uint32_t LBYPASS:1;        /*!< bit:     11  Lock Bypass                        */
    uint32_t DCOFILTER:3;      /*!< bit: 12..14  Sigma-Delta DCO Filter Selection   */
    uint32_t DCOEN:1;          /*!< bit:     15  DCO Filter Enable                  */
    uint32_t DIV:11;           /*!< bit: 16..26  Clock Divider                      */
    uint32_t :5;               /*!< bit: 27..31  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} OSCCTRL_DPLLCTRLB_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- OSCCTRL_DPLLSYNCBUSY : (OSCCTRL Offset: 0x3C) ( R/ 32) DPLL Synchronization Busy -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t :1;               /*!< bit:      0  Reserved                           */
    uint32_t ENABLE:1;         /*!< bit:      1  DPLL Enable Synchronization Status */
    uint32_t DPLLRATIO:1;      /*!< bit:      2  DPLL Loop Divider Ratio Synchronization Status */
    uint32_t :29;              /*!< bit:  3..31  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} OSCCTRL_DPLLSYNCBUSY_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- OSCCTRL_DPLLSTATUS : (OSCCTRL Offset: 0x40) ( R/ 32) DPLL Status -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t LOCK:1;           /*!< bit:      0  DPLL Lock Status                   */
    uint32_t CLKRDY:1;         /*!< bit:      1  DPLL Clock Ready                   */
    uint32_t :30;              /*!< bit:  2..31  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} OSCCTRL_DPLLSTATUS_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- OSCCTRL_EVCTRL : (OSCCTRL Offset: 0x00) (R/W 8) Event Control -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint8_t  CFDEO0:1;         /*!< bit:      0  Clock 0 Failure Detector Event Output Enable */
    uint8_t  CFDEO1:1;         /*!< bit:      1  Clock 1 Failure Detector Event Output Enable */
    uint8_t  :6;               /*!< bit:  2.. 7  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  struct {
    uint8_t  CFDEO:2;          /*!< bit:  0.. 1  Clock x Failure Detector Event Output Enable */
    uint8_t  :6;               /*!< bit:  2.. 7  Reserved                           */
  } vec;                       /*!< Structure used for vec  access                  */
  uint8_t reg;                 /*!< Type      used for register access              */
} OSCCTRL_EVCTRL_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- OSCCTRL_INTENCLR : (OSCCTRL Offset: 0x04) (R/W 32) Interrupt Enable Clear -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t XOSCRDY0:1;       /*!< bit:      0  XOSC 0 Ready Interrupt Enable      */
    uint32_t XOSCRDY1:1;       /*!< bit:      1  XOSC 1 Ready Interrupt Enable      */
    uint32_t XOSCFAIL0:1;      /*!< bit:      2  XOSC 0 Clock Failure Detector Interrupt Enable */
    uint32_t XOSCFAIL1:1;      /*!< bit:      3  XOSC 1 Clock Failure Detector Interrupt Enable */
    uint32_t :4;               /*!< bit:  4.. 7  Reserved                           */
    uint32_t DFLLRDY:1;        /*!< bit:      8  DFLL Ready Interrupt Enable        */
    uint32_t DFLLOOB:1;        /*!< bit:      9  DFLL Out Of Bounds Interrupt Enable */
    uint32_t DFLLLCKF:1;       /*!< bit:     10  DFLL Lock Fine Interrupt Enable    */
    uint32_t DFLLLCKC:1;       /*!< bit:     11  DFLL Lock Coarse Interrupt Enable  */
    uint32_t DFLLRCS:1;        /*!< bit:     12  DFLL Reference Clock Stopped Interrupt Enable */
    uint32_t :3;               /*!< bit: 13..15  Reserved                           */
    uint32_t DPLL0LCKR:1;      /*!< bit:     16  DPLL0 Lock Rise Interrupt Enable   */
    uint32_t DPLL0LCKF:1;      /*!< bit:     17  DPLL0 Lock Fall Interrupt Enable   */
    uint32_t DPLL0LTO:1;       /*!< bit:     18  DPLL0 Lock Timeout Interrupt Enable */
    uint32_t DPLL0LDRTO:1;     /*!< bit:     19  DPLL0 Loop Divider Ratio Update Complete Interrupt Enable */
    uint32_t :4;               /*!< bit: 20..23  Reserved                           */
    uint32_t DPLL1LCKR:1;      /*!< bit:     24  DPLL1 Lock Rise Interrupt Enable   */
    uint32_t DPLL1LCKF:1;      /*!< bit:     25  DPLL1 Lock Fall Interrupt Enable   */
    uint32_t DPLL1LTO:1;       /*!< bit:     26  DPLL1 Lock Timeout Interrupt Enable */
    uint32_t DPLL1LDRTO:1;     /*!< bit:     27  DPLL1 Loop Divider Ratio Update Complete Interrupt Enable */
    uint32_t :4;               /*!< bit: 28..31  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  struct {
    uint32_t XOSCRDY:2;        /*!< bit:  0.. 1  XOSC x Ready Interrupt Enable      */
    uint32_t XOSCFAIL:2;       /*!< bit:  2.. 3  XOSC x Clock Failure Detector Interrupt Enable */
    uint32_t :28;              /*!< bit:  4..31  Reserved                           */
  } vec;                       /*!< Structure used for vec  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} OSCCTRL_INTENCLR_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */
/* -------- OSCCTRL_INTENSET : (OSCCTRL Offset: 0x08) (R/W 32) Interrupt Enable Set -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t XOSCRDY0:1;       /*!< bit:      0  XOSC 0 Ready Interrupt Enable      */
    uint32_t XOSCRDY1:1;       /*!< bit:      1  XOSC 1 Ready Interrupt Enable      */
    uint32_t XOSCFAIL0:1;      /*!< bit:      2  XOSC 0 Clock Failure Detector Interrupt Enable */
    uint32_t XOSCFAIL1:1;      /*!< bit:      3  XOSC 1 Clock Failure Detector Interrupt Enable */
    uint32_t :4;               /*!< bit:  4.. 7  Reserved                           */
    uint32_t DFLLRDY:1;        /*!< bit:      8  DFLL Ready Interrupt Enable        */
    uint32_t DFLLOOB:1;        /*!< bit:      9  DFLL Out Of Bounds Interrupt Enable */
    uint32_t DFLLLCKF:1;       /*!< bit:     10  DFLL Lock Fine Interrupt Enable    */
    uint32_t DFLLLCKC:1;       /*!< bit:     11  DFLL Lock Coarse Interrupt Enable  */
    uint32_t DFLLRCS:1;        /*!< bit:     12  DFLL Reference Clock Stopped Interrupt Enable */
    uint32_t :3;               /*!< bit: 13..15  Reserved                           */
    uint32_t DPLL0LCKR:1;      /*!< bit:     16  DPLL0 Lock Rise Interrupt Enable   */
    uint32_t DPLL0LCKF:1;      /*!< bit:     17  DPLL0 Lock Fall Interrupt Enable   */
    uint32_t DPLL0LTO:1;       /*!< bit:     18  DPLL0 Lock Timeout Interrupt Enable */
    uint32_t DPLL0LDRTO:1;     /*!< bit:     19  DPLL0 Loop Divider Ratio Update Complete Interrupt Enable */
    uint32_t :4;               /*!< bit: 20..23  Reserved                           */
    uint32_t DPLL1LCKR:1;      /*!< bit:     24  DPLL1 Lock Rise Interrupt Enable   */
    uint32_t DPLL1LCKF:1;      /*!< bit:     25  DPLL1 Lock Fall Interrupt Enable   */
    uint32_t DPLL1LTO:1;       /*!< bit:     26  DPLL1 Lock Timeout Interrupt Enable */
    uint32_t DPLL1LDRTO:1;     /*!< bit:     27  DPLL1 Loop Divider Ratio Update Complete Interrupt Enable */
    uint32_t :4;               /*!< bit: 28..31  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  struct {
    uint32_t XOSCRDY:2;        /*!< bit:  0.. 1  XOSC x Ready Interrupt Enable      */
    uint32_t XOSCFAIL:2;       /*!< bit:  2.. 3  XOSC x Clock Failure Detector Interrupt Enable */
    uint32_t :28;              /*!< bit:  4..31  Reserved                           */
  } vec;                       /*!< Structure used for vec  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} OSCCTRL_INTENSET_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- OSCCTRL_INTFLAG : (OSCCTRL Offset: 0x0C) (R/W 32) Interrupt Flag Status and Clear -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { // __I to avoid read-modify-write on write-to-clear register
  struct {
    __I uint32_t XOSCRDY0:1;       /*!< bit:      0  XOSC 0 Ready                       */
    __I uint32_t XOSCRDY1:1;       /*!< bit:      1  XOSC 1 Ready                       */
    __I uint32_t XOSCFAIL0:1;      /*!< bit:      2  XOSC 0 Clock Failure Detector      */
    __I uint32_t XOSCFAIL1:1;      /*!< bit:      3  XOSC 1 Clock Failure Detector      */
    __I uint32_t :4;               /*!< bit:  4.. 7  Reserved                           */
    __I uint32_t DFLLRDY:1;        /*!< bit:      8  DFLL Ready                         */
    __I uint32_t DFLLOOB:1;        /*!< bit:      9  DFLL Out Of Bounds                 */
    __I uint32_t DFLLLCKF:1;       /*!< bit:     10  DFLL Lock Fine                     */
    __I uint32_t DFLLLCKC:1;       /*!< bit:     11  DFLL Lock Coarse                   */
    __I uint32_t DFLLRCS:1;        /*!< bit:     12  DFLL Reference Clock Stopped       */
    __I uint32_t :3;               /*!< bit: 13..15  Reserved                           */
    __I uint32_t DPLL0LCKR:1;      /*!< bit:     16  DPLL0 Lock Rise                    */
    __I uint32_t DPLL0LCKF:1;      /*!< bit:     17  DPLL0 Lock Fall                    */
    __I uint32_t DPLL0LTO:1;       /*!< bit:     18  DPLL0 Lock Timeout                 */
    __I uint32_t DPLL0LDRTO:1;     /*!< bit:     19  DPLL0 Loop Divider Ratio Update Complete */
    __I uint32_t :4;               /*!< bit: 20..23  Reserved                           */
    __I uint32_t DPLL1LCKR:1;      /*!< bit:     24  DPLL1 Lock Rise                    */
    __I uint32_t DPLL1LCKF:1;      /*!< bit:     25  DPLL1 Lock Fall                    */
    __I uint32_t DPLL1LTO:1;       /*!< bit:     26  DPLL1 Lock Timeout                 */
    __I uint32_t DPLL1LDRTO:1;     /*!< bit:     27  DPLL1 Loop Divider Ratio Update Complete */
    __I uint32_t :4;               /*!< bit: 28..31  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  struct {
    __I uint32_t XOSCRDY:2;        /*!< bit:  0.. 1  XOSC x Ready                       */
    __I uint32_t XOSCFAIL:2;       /*!< bit:  2.. 3  XOSC x Clock Failure Detector      */
    __I uint32_t :28;              /*!< bit:  4..31  Reserved                           */
  } vec;                       /*!< Structure used for vec  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} OSCCTRL_INTFLAG_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- OSCCTRL_STATUS : (OSCCTRL Offset: 0x10) ( R/ 32) Status -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t XOSCRDY0:1;       /*!< bit:      0  XOSC 0 Ready                       */
    uint32_t XOSCRDY1:1;       /*!< bit:      1  XOSC 1 Ready                       */
    uint32_t XOSCFAIL0:1;      /*!< bit:      2  XOSC 0 Clock Failure Detector      */
    uint32_t XOSCFAIL1:1;      /*!< bit:      3  XOSC 1 Clock Failure Detector      */
    uint32_t XOSCCKSW0:1;      /*!< bit:      4  XOSC 0 Clock Switch                */
    uint32_t XOSCCKSW1:1;      /*!< bit:      5  XOSC 1 Clock Switch                */
    uint32_t :2;               /*!< bit:  6.. 7  Reserved                           */
    uint32_t DFLLRDY:1;        /*!< bit:      8  DFLL Ready                         */
    uint32_t DFLLOOB:1;        /*!< bit:      9  DFLL Out Of Bounds                 */
    uint32_t DFLLLCKF:1;       /*!< bit:     10  DFLL Lock Fine                     */
    uint32_t DFLLLCKC:1;       /*!< bit:     11  DFLL Lock Coarse                   */
    uint32_t DFLLRCS:1;        /*!< bit:     12  DFLL Reference Clock Stopped       */
    uint32_t :3;               /*!< bit: 13..15  Reserved                           */
    uint32_t DPLL0LCKR:1;      /*!< bit:     16  DPLL0 Lock Rise                    */
    uint32_t DPLL0LCKF:1;      /*!< bit:     17  DPLL0 Lock Fall                    */
    uint32_t DPLL0TO:1;        /*!< bit:     18  DPLL0 Timeout                      */
    uint32_t DPLL0LDRTO:1;     /*!< bit:     19  DPLL0 Loop Divider Ratio Update Complete */
    uint32_t :4;               /*!< bit: 20..23  Reserved                           */
    uint32_t DPLL1LCKR:1;      /*!< bit:     24  DPLL1 Lock Rise                    */
    uint32_t DPLL1LCKF:1;      /*!< bit:     25  DPLL1 Lock Fall                    */
    uint32_t DPLL1TO:1;        /*!< bit:     26  DPLL1 Timeout                      */
    uint32_t DPLL1LDRTO:1;     /*!< bit:     27  DPLL1 Loop Divider Ratio Update Complete */
    uint32_t :4;               /*!< bit: 28..31  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  struct {
    uint32_t XOSCRDY:2;        /*!< bit:  0.. 1  XOSC x Ready                       */
    uint32_t XOSCFAIL:2;       /*!< bit:  2.. 3  XOSC x Clock Failure Detector      */
    uint32_t XOSCCKSW:2;       /*!< bit:  4.. 5  XOSC x Clock Switch                */
    uint32_t :26;              /*!< bit:  6..31  Reserved                           */
  } vec;                       /*!< Structure used for vec  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} OSCCTRL_STATUS_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- OSCCTRL_XOSCCTRL : (OSCCTRL Offset: 0x14) (R/W 32) External Multipurpose Crystal Oscillator Control -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t :1;               /*!< bit:      0  Reserved                           */
    uint32_t ENABLE:1;         /*!< bit:      1  Oscillator Enable                  */
    uint32_t XTALEN:1;         /*!< bit:      2  Crystal Oscillator Enable          */
    uint32_t :3;               /*!< bit:  3.. 5  Reserved                           */
    uint32_t RUNSTDBY:1;       /*!< bit:      6  Run in Standby                     */
    uint32_t ONDEMAND:1;       /*!< bit:      7  On Demand Control                  */
    uint32_t LOWBUFGAIN:1;     /*!< bit:      8  Low Buffer Gain Enable             */
    uint32_t IPTAT:2;          /*!< bit:  9..10  Oscillator Current Reference       */
    uint32_t IMULT:4;          /*!< bit: 11..14  Oscillator Current Multiplier      */
    uint32_t ENALC:1;          /*!< bit:     15  Automatic Loop Control Enable      */
    uint32_t CFDEN:1;          /*!< bit:     16  Clock Failure Detector Enable      */
    uint32_t SWBEN:1;          /*!< bit:     17  Xosc Clock Switch Enable           */
    uint32_t :2;               /*!< bit: 18..19  Reserved                           */
    uint32_t STARTUP:4;        /*!< bit: 20..23  Start-Up Time                      */
    uint32_t CFDPRESC:4;       /*!< bit: 24..27  Clock Failure Detector Prescaler   */
    uint32_t :4;               /*!< bit: 28..31  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} OSCCTRL_XOSCCTRL_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- OSCCTRL_DFLLCTRLA : (OSCCTRL Offset: 0x1C) (R/W 8) DFLL48M Control A -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint8_t  :1;               /*!< bit:      0  Reserved                           */
    uint8_t  ENABLE:1;         /*!< bit:      1  DFLL Enable                        */
    uint8_t  :4;               /*!< bit:  2.. 5  Reserved                           */
    uint8_t  RUNSTDBY:1;       /*!< bit:      6  Run in Standby                     */
    uint8_t  ONDEMAND:1;       /*!< bit:      7  On Demand Control                  */
  } bit;                       /*!< Structure used for bit  access                  */
  uint8_t reg;                 /*!< Type      used for register access              */
} OSCCTRL_DFLLCTRLA_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- OSCCTRL_DFLLCTRLB : (OSCCTRL Offset: 0x20) (R/W 8) DFLL48M Control B -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint8_t  MODE:1;           /*!< bit:      0  Operating Mode Selection           */
    uint8_t  STABLE:1;         /*!< bit:      1  Stable DFLL Frequency              */
    uint8_t  LLAW:1;           /*!< bit:      2  Lose Lock After Wake               */
    uint8_t  USBCRM:1;         /*!< bit:      3  USB Clock Recovery Mode            */
    uint8_t  CCDIS:1;          /*!< bit:      4  Chill Cycle Disable                */
    uint8_t  QLDIS:1;          /*!< bit:      5  Quick Lock Disable                 */
    uint8_t  BPLCKC:1;         /*!< bit:      6  Bypass Coarse Lock                 */
    uint8_t  WAITLOCK:1;       /*!< bit:      7  Wait Lock                          */
  } bit;                       /*!< Structure used for bit  access                  */
  uint8_t reg;                 /*!< Type      used for register access              */
} OSCCTRL_DFLLCTRLB_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- OSCCTRL_DFLLVAL : (OSCCTRL Offset: 0x24) (R/W 32) DFLL48M Value -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t FINE:8;           /*!< bit:  0.. 7  Fine Value                         */
    uint32_t :2;               /*!< bit:  8.. 9  Reserved                           */
    uint32_t COARSE:6;         /*!< bit: 10..15  Coarse Value                       */
    uint32_t DIFF:16;          /*!< bit: 16..31  Multiplication Ratio Difference    */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} OSCCTRL_DFLLVAL_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- OSCCTRL_DFLLMUL : (OSCCTRL Offset: 0x28) (R/W 32) DFLL48M Multiplier -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t MUL:16;           /*!< bit:  0..15  DFLL Multiply Factor               */
    uint32_t FSTEP:8;          /*!< bit: 16..23  Fine Maximum Step                  */
    uint32_t :2;               /*!< bit: 24..25  Reserved                           */
    uint32_t CSTEP:6;          /*!< bit: 26..31  Coarse Maximum Step                */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} OSCCTRL_DFLLMUL_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- OSCCTRL_DFLLSYNC : (OSCCTRL Offset: 0x2C) (R/W 8) DFLL48M Synchronization -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint8_t  :1;               /*!< bit:      0  Reserved                           */
    uint8_t  ENABLE:1;         /*!< bit:      1  ENABLE Synchronization Busy        */
    uint8_t  DFLLCTRLB:1;      /*!< bit:      2  DFLLCTRLB Synchronization Busy     */
    uint8_t  DFLLVAL:1;        /*!< bit:      3  DFLLVAL Synchronization Busy       */
    uint8_t  DFLLMUL:1;        /*!< bit:      4  DFLLMUL Synchronization Busy       */
    uint8_t  :3;               /*!< bit:  5.. 7  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint8_t reg;                 /*!< Type      used for register access              */
} OSCCTRL_DFLLSYNC_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/** \brief OscctrlDpll hardware registers */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef struct {
  __IO OSCCTRL_DPLLCTRLA_Type    DPLLCTRLA;   /**< \brief Offset: 0x00 (R/W  8) DPLL Control A */
       RoReg8                    Reserved1[0x3];
  __IO OSCCTRL_DPLLRATIO_Type    DPLLRATIO;   /**< \brief Offset: 0x04 (R/W 32) DPLL Ratio Control */
  __IO OSCCTRL_DPLLCTRLB_Type    DPLLCTRLB;   /**< \brief Offset: 0x08 (R/W 32) DPLL Control B */
  __I  OSCCTRL_DPLLSYNCBUSY_Type DPLLSYNCBUSY; /**< \brief Offset: 0x0C (R/  32) DPLL Synchronization Busy */
  __I  OSCCTRL_DPLLSTATUS_Type   DPLLSTATUS;  /**< \brief Offset: 0x10 (R/  32) DPLL Status */
} OscctrlDpll;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/** \brief OSCCTRL hardware registers */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef struct {
  __IO OSCCTRL_EVCTRL_Type       EVCTRL;      /**< \brief Offset: 0x00 (R/W  8) Event Control */
       RoReg8                    Reserved1[0x3];
  __IO OSCCTRL_INTENCLR_Type     INTENCLR;    /**< \brief Offset: 0x04 (R/W 32) Interrupt Enable Clear */
  __IO OSCCTRL_INTENSET_Type     INTENSET;    /**< \brief Offset: 0x08 (R/W 32) Interrupt Enable Set */
  __IO OSCCTRL_INTFLAG_Type      INTFLAG;     /**< \brief Offset: 0x0C (R/W 32) Interrupt Flag Status and Clear */
  __I  OSCCTRL_STATUS_Type       STATUS;      /**< \brief Offset: 0x10 (R/  32) Status */
  __IO OSCCTRL_XOSCCTRL_Type     XOSCCTRL[2]; /**< \brief Offset: 0x14 (R/W 32) External Multipurpose Crystal Oscillator Control */
  __IO OSCCTRL_DFLLCTRLA_Type    DFLLCTRLA;   /**< \brief Offset: 0x1C (R/W  8) DFLL48M Control A */
       RoReg8                    Reserved2[0x3];
  __IO OSCCTRL_DFLLCTRLB_Type    DFLLCTRLB;   /**< \brief Offset: 0x20 (R/W  8) DFLL48M Control B */
       RoReg8                    Reserved3[0x3];
  __IO OSCCTRL_DFLLVAL_Type      DFLLVAL;     /**< \brief Offset: 0x24 (R/W 32) DFLL48M Value */
  __IO OSCCTRL_DFLLMUL_Type      DFLLMUL;     /**< \brief Offset: 0x28 (R/W 32) DFLL48M Multiplier */
  __IO OSCCTRL_DFLLSYNC_Type     DFLLSYNC;    /**< \brief Offset: 0x2C (R/W  8) DFLL48M Synchronization */
       RoReg8                    Reserved4[0x3];
       OscctrlDpll               Dpll[OSCCTRL_DPLL_NUMBER];     /**< \brief Offset: 0x30 OscctrlDpll groups [DPLLS_NUM] */
} Oscctrl;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

#endif /* _MICROCHIP_PIC32CXSG_OSCCTRL_COMPONENT_FIXUP_H_ */
