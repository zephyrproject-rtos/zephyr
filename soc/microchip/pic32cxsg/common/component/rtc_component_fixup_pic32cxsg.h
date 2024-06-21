/*
 * Copyright (c) 2024 Microchip
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _MICROCHIP_PIC32CXSG_RTC_COMPONENT_FIXUP_H_
#define _MICROCHIP_PIC32CXSG_RTC_COMPONENT_FIXUP_H_

/* -------- RTC_MODE0_CTRLA : (RTC Offset: 0x00) (R/W 16) MODE0 Control A -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint16_t SWRST:1;          /*!< bit:      0  Software Reset                     */
    uint16_t ENABLE:1;         /*!< bit:      1  Enable                             */
    uint16_t MODE:2;           /*!< bit:  2.. 3  Operating Mode                     */
    uint16_t :3;               /*!< bit:  4.. 6  Reserved                           */
    uint16_t MATCHCLR:1;       /*!< bit:      7  Clear on Match                     */
    uint16_t PRESCALER:4;      /*!< bit:  8..11  Prescaler                          */
    uint16_t :1;               /*!< bit:     12  Reserved                           */
    uint16_t BKTRST:1;         /*!< bit:     13  BKUP Registers Reset On Tamper Enable */
    uint16_t GPTRST:1;         /*!< bit:     14  GP Registers Reset On Tamper Enable */
    uint16_t COUNTSYNC:1;      /*!< bit:     15  Count Read Synchronization Enable  */
  } bit;                       /*!< Structure used for bit  access                  */
  uint16_t reg;                /*!< Type      used for register access              */
} RTC_MODE0_CTRLA_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- RTC_MODE1_CTRLA : (RTC Offset: 0x00) (R/W 16) MODE1 Control A -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint16_t SWRST:1;          /*!< bit:      0  Software Reset                     */
    uint16_t ENABLE:1;         /*!< bit:      1  Enable                             */
    uint16_t MODE:2;           /*!< bit:  2.. 3  Operating Mode                     */
    uint16_t :4;               /*!< bit:  4.. 7  Reserved                           */
    uint16_t PRESCALER:4;      /*!< bit:  8..11  Prescaler                          */
    uint16_t :1;               /*!< bit:     12  Reserved                           */
    uint16_t BKTRST:1;         /*!< bit:     13  BKUP Registers Reset On Tamper Enable */
    uint16_t GPTRST:1;         /*!< bit:     14  GP Registers Reset On Tamper Enable */
    uint16_t COUNTSYNC:1;      /*!< bit:     15  Count Read Synchronization Enable  */
  } bit;                       /*!< Structure used for bit  access                  */
  uint16_t reg;                /*!< Type      used for register access              */
} RTC_MODE1_CTRLA_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- RTC_MODE2_CTRLA : (RTC Offset: 0x00) (R/W 16) MODE2 Control A -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint16_t SWRST:1;          /*!< bit:      0  Software Reset                     */
    uint16_t ENABLE:1;         /*!< bit:      1  Enable                             */
    uint16_t MODE:2;           /*!< bit:  2.. 3  Operating Mode                     */
    uint16_t :2;               /*!< bit:  4.. 5  Reserved                           */
    uint16_t CLKREP:1;         /*!< bit:      6  Clock Representation               */
    uint16_t MATCHCLR:1;       /*!< bit:      7  Clear on Match                     */
    uint16_t PRESCALER:4;      /*!< bit:  8..11  Prescaler                          */
    uint16_t :1;               /*!< bit:     12  Reserved                           */
    uint16_t BKTRST:1;         /*!< bit:     13  BKUP Registers Reset On Tamper Enable */
    uint16_t GPTRST:1;         /*!< bit:     14  GP Registers Reset On Tamper Enable */
    uint16_t CLOCKSYNC:1;      /*!< bit:     15  Clock Read Synchronization Enable  */
  } bit;                       /*!< Structure used for bit  access                  */
  uint16_t reg;                /*!< Type      used for register access              */
} RTC_MODE2_CTRLA_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- RTC_MODE0_CTRLB : (RTC Offset: 0x02) (R/W 16) MODE0 Control B -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint16_t GP0EN:1;          /*!< bit:      0  General Purpose 0 Enable           */
    uint16_t GP2EN:1;          /*!< bit:      1  General Purpose 2 Enable           */
    uint16_t :2;               /*!< bit:  2.. 3  Reserved                           */
    uint16_t DEBMAJ:1;         /*!< bit:      4  Debouncer Majority Enable          */
    uint16_t DEBASYNC:1;       /*!< bit:      5  Debouncer Asynchronous Enable      */
    uint16_t RTCOUT:1;         /*!< bit:      6  RTC Output Enable                  */
    uint16_t DMAEN:1;          /*!< bit:      7  DMA Enable                         */
    uint16_t DEBF:3;           /*!< bit:  8..10  Debounce Freqnuency                */
    uint16_t :1;               /*!< bit:     11  Reserved                           */
    uint16_t ACTF:3;           /*!< bit: 12..14  Active Layer Freqnuency            */
    uint16_t :1;               /*!< bit:     15  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint16_t reg;                /*!< Type      used for register access              */
} RTC_MODE0_CTRLB_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- RTC_MODE1_CTRLB : (RTC Offset: 0x02) (R/W 16) MODE1 Control B -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint16_t GP0EN:1;          /*!< bit:      0  General Purpose 0 Enable           */
    uint16_t GP2EN:1;          /*!< bit:      1  General Purpose 2 Enable           */
    uint16_t :2;               /*!< bit:  2.. 3  Reserved                           */
    uint16_t DEBMAJ:1;         /*!< bit:      4  Debouncer Majority Enable          */
    uint16_t DEBASYNC:1;       /*!< bit:      5  Debouncer Asynchronous Enable      */
    uint16_t RTCOUT:1;         /*!< bit:      6  RTC Output Enable                  */
    uint16_t DMAEN:1;          /*!< bit:      7  DMA Enable                         */
    uint16_t DEBF:3;           /*!< bit:  8..10  Debounce Freqnuency                */
    uint16_t :1;               /*!< bit:     11  Reserved                           */
    uint16_t ACTF:3;           /*!< bit: 12..14  Active Layer Freqnuency            */
    uint16_t :1;               /*!< bit:     15  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint16_t reg;                /*!< Type      used for register access              */
} RTC_MODE1_CTRLB_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- RTC_MODE2_CTRLB : (RTC Offset: 0x02) (R/W 16) MODE2 Control B -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint16_t GP0EN:1;          /*!< bit:      0  General Purpose 0 Enable           */
    uint16_t GP2EN:1;          /*!< bit:      1  General Purpose 2 Enable           */
    uint16_t :2;               /*!< bit:  2.. 3  Reserved                           */
    uint16_t DEBMAJ:1;         /*!< bit:      4  Debouncer Majority Enable          */
    uint16_t DEBASYNC:1;       /*!< bit:      5  Debouncer Asynchronous Enable      */
    uint16_t RTCOUT:1;         /*!< bit:      6  RTC Output Enable                  */
    uint16_t DMAEN:1;          /*!< bit:      7  DMA Enable                         */
    uint16_t DEBF:3;           /*!< bit:  8..10  Debounce Freqnuency                */
    uint16_t :1;               /*!< bit:     11  Reserved                           */
    uint16_t ACTF:3;           /*!< bit: 12..14  Active Layer Freqnuency            */
    uint16_t :1;               /*!< bit:     15  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint16_t reg;                /*!< Type      used for register access              */
} RTC_MODE2_CTRLB_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- RTC_MODE0_EVCTRL : (RTC Offset: 0x04) (R/W 32) MODE0 Event Control -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t PEREO0:1;         /*!< bit:      0  Periodic Interval 0 Event Output Enable */
    uint32_t PEREO1:1;         /*!< bit:      1  Periodic Interval 1 Event Output Enable */
    uint32_t PEREO2:1;         /*!< bit:      2  Periodic Interval 2 Event Output Enable */
    uint32_t PEREO3:1;         /*!< bit:      3  Periodic Interval 3 Event Output Enable */
    uint32_t PEREO4:1;         /*!< bit:      4  Periodic Interval 4 Event Output Enable */
    uint32_t PEREO5:1;         /*!< bit:      5  Periodic Interval 5 Event Output Enable */
    uint32_t PEREO6:1;         /*!< bit:      6  Periodic Interval 6 Event Output Enable */
    uint32_t PEREO7:1;         /*!< bit:      7  Periodic Interval 7 Event Output Enable */
    uint32_t CMPEO0:1;         /*!< bit:      8  Compare 0 Event Output Enable      */
    uint32_t CMPEO1:1;         /*!< bit:      9  Compare 1 Event Output Enable      */
    uint32_t :4;               /*!< bit: 10..13  Reserved                           */
    uint32_t TAMPEREO:1;       /*!< bit:     14  Tamper Event Output Enable         */
    uint32_t OVFEO:1;          /*!< bit:     15  Overflow Event Output Enable       */
    uint32_t TAMPEVEI:1;       /*!< bit:     16  Tamper Event Input Enable          */
    uint32_t :15;              /*!< bit: 17..31  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  struct {
    uint32_t PEREO:8;          /*!< bit:  0.. 7  Periodic Interval x Event Output Enable */
    uint32_t CMPEO:2;          /*!< bit:  8.. 9  Compare x Event Output Enable      */
    uint32_t :22;              /*!< bit: 10..31  Reserved                           */
  } vec;                       /*!< Structure used for vec  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} RTC_MODE0_EVCTRL_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- RTC_MODE1_EVCTRL : (RTC Offset: 0x04) (R/W 32) MODE1 Event Control -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t PEREO0:1;         /*!< bit:      0  Periodic Interval 0 Event Output Enable */
    uint32_t PEREO1:1;         /*!< bit:      1  Periodic Interval 1 Event Output Enable */
    uint32_t PEREO2:1;         /*!< bit:      2  Periodic Interval 2 Event Output Enable */
    uint32_t PEREO3:1;         /*!< bit:      3  Periodic Interval 3 Event Output Enable */
    uint32_t PEREO4:1;         /*!< bit:      4  Periodic Interval 4 Event Output Enable */
    uint32_t PEREO5:1;         /*!< bit:      5  Periodic Interval 5 Event Output Enable */
    uint32_t PEREO6:1;         /*!< bit:      6  Periodic Interval 6 Event Output Enable */
    uint32_t PEREO7:1;         /*!< bit:      7  Periodic Interval 7 Event Output Enable */
    uint32_t CMPEO0:1;         /*!< bit:      8  Compare 0 Event Output Enable      */
    uint32_t CMPEO1:1;         /*!< bit:      9  Compare 1 Event Output Enable      */
    uint32_t CMPEO2:1;         /*!< bit:     10  Compare 2 Event Output Enable      */
    uint32_t CMPEO3:1;         /*!< bit:     11  Compare 3 Event Output Enable      */
    uint32_t :2;               /*!< bit: 12..13  Reserved                           */
    uint32_t TAMPEREO:1;       /*!< bit:     14  Tamper Event Output Enable         */
    uint32_t OVFEO:1;          /*!< bit:     15  Overflow Event Output Enable       */
    uint32_t TAMPEVEI:1;       /*!< bit:     16  Tamper Event Input Enable          */
    uint32_t :15;              /*!< bit: 17..31  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  struct {
    uint32_t PEREO:8;          /*!< bit:  0.. 7  Periodic Interval x Event Output Enable */
    uint32_t CMPEO:4;          /*!< bit:  8..11  Compare x Event Output Enable      */
    uint32_t :20;              /*!< bit: 12..31  Reserved                           */
  } vec;                       /*!< Structure used for vec  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} RTC_MODE1_EVCTRL_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- RTC_MODE2_EVCTRL : (RTC Offset: 0x04) (R/W 32) MODE2 Event Control -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t PEREO0:1;         /*!< bit:      0  Periodic Interval 0 Event Output Enable */
    uint32_t PEREO1:1;         /*!< bit:      1  Periodic Interval 1 Event Output Enable */
    uint32_t PEREO2:1;         /*!< bit:      2  Periodic Interval 2 Event Output Enable */
    uint32_t PEREO3:1;         /*!< bit:      3  Periodic Interval 3 Event Output Enable */
    uint32_t PEREO4:1;         /*!< bit:      4  Periodic Interval 4 Event Output Enable */
    uint32_t PEREO5:1;         /*!< bit:      5  Periodic Interval 5 Event Output Enable */
    uint32_t PEREO6:1;         /*!< bit:      6  Periodic Interval 6 Event Output Enable */
    uint32_t PEREO7:1;         /*!< bit:      7  Periodic Interval 7 Event Output Enable */
    uint32_t ALARMEO0:1;       /*!< bit:      8  Alarm 0 Event Output Enable        */
    uint32_t ALARMEO1:1;       /*!< bit:      9  Alarm 1 Event Output Enable        */
    uint32_t :4;               /*!< bit: 10..13  Reserved                           */
    uint32_t TAMPEREO:1;       /*!< bit:     14  Tamper Event Output Enable         */
    uint32_t OVFEO:1;          /*!< bit:     15  Overflow Event Output Enable       */
    uint32_t TAMPEVEI:1;       /*!< bit:     16  Tamper Event Input Enable          */
    uint32_t :15;              /*!< bit: 17..31  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  struct {
    uint32_t PEREO:8;          /*!< bit:  0.. 7  Periodic Interval x Event Output Enable */
    uint32_t ALARMEO:2;        /*!< bit:  8.. 9  Alarm x Event Output Enable        */
    uint32_t :22;              /*!< bit: 10..31  Reserved                           */
  } vec;                       /*!< Structure used for vec  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} RTC_MODE2_EVCTRL_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- RTC_MODE0_INTENCLR : (RTC Offset: 0x08) (R/W 16) MODE0 Interrupt Enable Clear -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint16_t PER0:1;           /*!< bit:      0  Periodic Interval 0 Interrupt Enable */
    uint16_t PER1:1;           /*!< bit:      1  Periodic Interval 1 Interrupt Enable */
    uint16_t PER2:1;           /*!< bit:      2  Periodic Interval 2 Interrupt Enable */
    uint16_t PER3:1;           /*!< bit:      3  Periodic Interval 3 Interrupt Enable */
    uint16_t PER4:1;           /*!< bit:      4  Periodic Interval 4 Interrupt Enable */
    uint16_t PER5:1;           /*!< bit:      5  Periodic Interval 5 Interrupt Enable */
    uint16_t PER6:1;           /*!< bit:      6  Periodic Interval 6 Interrupt Enable */
    uint16_t PER7:1;           /*!< bit:      7  Periodic Interval 7 Interrupt Enable */
    uint16_t CMP0:1;           /*!< bit:      8  Compare 0 Interrupt Enable         */
    uint16_t CMP1:1;           /*!< bit:      9  Compare 1 Interrupt Enable         */
    uint16_t :4;               /*!< bit: 10..13  Reserved                           */
    uint16_t TAMPER:1;         /*!< bit:     14  Tamper Enable                      */
    uint16_t OVF:1;            /*!< bit:     15  Overflow Interrupt Enable          */
  } bit;                       /*!< Structure used for bit  access                  */
  struct {
    uint16_t PER:8;            /*!< bit:  0.. 7  Periodic Interval x Interrupt Enable */
    uint16_t CMP:2;            /*!< bit:  8.. 9  Compare x Interrupt Enable         */
    uint16_t :6;               /*!< bit: 10..15  Reserved                           */
  } vec;                       /*!< Structure used for vec  access                  */
  uint16_t reg;                /*!< Type      used for register access              */
} RTC_MODE0_INTENCLR_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- RTC_MODE1_INTENCLR : (RTC Offset: 0x08) (R/W 16) MODE1 Interrupt Enable Clear -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint16_t PER0:1;           /*!< bit:      0  Periodic Interval 0 Interrupt Enable */
    uint16_t PER1:1;           /*!< bit:      1  Periodic Interval 1 Interrupt Enable */
    uint16_t PER2:1;           /*!< bit:      2  Periodic Interval 2 Interrupt Enable */
    uint16_t PER3:1;           /*!< bit:      3  Periodic Interval 3 Interrupt Enable */
    uint16_t PER4:1;           /*!< bit:      4  Periodic Interval 4 Interrupt Enable */
    uint16_t PER5:1;           /*!< bit:      5  Periodic Interval 5 Interrupt Enable */
    uint16_t PER6:1;           /*!< bit:      6  Periodic Interval 6 Interrupt Enable */
    uint16_t PER7:1;           /*!< bit:      7  Periodic Interval 7 Interrupt Enable */
    uint16_t CMP0:1;           /*!< bit:      8  Compare 0 Interrupt Enable         */
    uint16_t CMP1:1;           /*!< bit:      9  Compare 1 Interrupt Enable         */
    uint16_t CMP2:1;           /*!< bit:     10  Compare 2 Interrupt Enable         */
    uint16_t CMP3:1;           /*!< bit:     11  Compare 3 Interrupt Enable         */
    uint16_t :2;               /*!< bit: 12..13  Reserved                           */
    uint16_t TAMPER:1;         /*!< bit:     14  Tamper Enable                      */
    uint16_t OVF:1;            /*!< bit:     15  Overflow Interrupt Enable          */
  } bit;                       /*!< Structure used for bit  access                  */
  struct {
    uint16_t PER:8;            /*!< bit:  0.. 7  Periodic Interval x Interrupt Enable */
    uint16_t CMP:4;            /*!< bit:  8..11  Compare x Interrupt Enable         */
    uint16_t :4;               /*!< bit: 12..15  Reserved                           */
  } vec;                       /*!< Structure used for vec  access                  */
  uint16_t reg;                /*!< Type      used for register access              */
} RTC_MODE1_INTENCLR_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- RTC_MODE2_INTENCLR : (RTC Offset: 0x08) (R/W 16) MODE2 Interrupt Enable Clear -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint16_t PER0:1;           /*!< bit:      0  Periodic Interval 0 Interrupt Enable */
    uint16_t PER1:1;           /*!< bit:      1  Periodic Interval 1 Interrupt Enable */
    uint16_t PER2:1;           /*!< bit:      2  Periodic Interval 2 Interrupt Enable */
    uint16_t PER3:1;           /*!< bit:      3  Periodic Interval 3 Interrupt Enable */
    uint16_t PER4:1;           /*!< bit:      4  Periodic Interval 4 Interrupt Enable */
    uint16_t PER5:1;           /*!< bit:      5  Periodic Interval 5 Interrupt Enable */
    uint16_t PER6:1;           /*!< bit:      6  Periodic Interval 6 Interrupt Enable */
    uint16_t PER7:1;           /*!< bit:      7  Periodic Interval 7 Interrupt Enable */
    uint16_t ALARM0:1;         /*!< bit:      8  Alarm 0 Interrupt Enable           */
    uint16_t ALARM1:1;         /*!< bit:      9  Alarm 1 Interrupt Enable           */
    uint16_t :4;               /*!< bit: 10..13  Reserved                           */
    uint16_t TAMPER:1;         /*!< bit:     14  Tamper Enable                      */
    uint16_t OVF:1;            /*!< bit:     15  Overflow Interrupt Enable          */
  } bit;                       /*!< Structure used for bit  access                  */
  struct {
    uint16_t PER:8;            /*!< bit:  0.. 7  Periodic Interval x Interrupt Enable */
    uint16_t ALARM:2;          /*!< bit:  8.. 9  Alarm x Interrupt Enable           */
    uint16_t :6;               /*!< bit: 10..15  Reserved                           */
  } vec;                       /*!< Structure used for vec  access                  */
  uint16_t reg;                /*!< Type      used for register access              */
} RTC_MODE2_INTENCLR_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- RTC_MODE0_INTENSET : (RTC Offset: 0x0A) (R/W 16) MODE0 Interrupt Enable Set -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint16_t PER0:1;           /*!< bit:      0  Periodic Interval 0 Interrupt Enable */
    uint16_t PER1:1;           /*!< bit:      1  Periodic Interval 1 Interrupt Enable */
    uint16_t PER2:1;           /*!< bit:      2  Periodic Interval 2 Interrupt Enable */
    uint16_t PER3:1;           /*!< bit:      3  Periodic Interval 3 Interrupt Enable */
    uint16_t PER4:1;           /*!< bit:      4  Periodic Interval 4 Interrupt Enable */
    uint16_t PER5:1;           /*!< bit:      5  Periodic Interval 5 Interrupt Enable */
    uint16_t PER6:1;           /*!< bit:      6  Periodic Interval 6 Interrupt Enable */
    uint16_t PER7:1;           /*!< bit:      7  Periodic Interval 7 Interrupt Enable */
    uint16_t CMP0:1;           /*!< bit:      8  Compare 0 Interrupt Enable         */
    uint16_t CMP1:1;           /*!< bit:      9  Compare 1 Interrupt Enable         */
    uint16_t :4;               /*!< bit: 10..13  Reserved                           */
    uint16_t TAMPER:1;         /*!< bit:     14  Tamper Enable                      */
    uint16_t OVF:1;            /*!< bit:     15  Overflow Interrupt Enable          */
  } bit;                       /*!< Structure used for bit  access                  */
  struct {
    uint16_t PER:8;            /*!< bit:  0.. 7  Periodic Interval x Interrupt Enable */
    uint16_t CMP:2;            /*!< bit:  8.. 9  Compare x Interrupt Enable         */
    uint16_t :6;               /*!< bit: 10..15  Reserved                           */
  } vec;                       /*!< Structure used for vec  access                  */
  uint16_t reg;                /*!< Type      used for register access              */
} RTC_MODE0_INTENSET_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- RTC_MODE1_INTENSET : (RTC Offset: 0x0A) (R/W 16) MODE1 Interrupt Enable Set -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint16_t PER0:1;           /*!< bit:      0  Periodic Interval 0 Interrupt Enable */
    uint16_t PER1:1;           /*!< bit:      1  Periodic Interval 1 Interrupt Enable */
    uint16_t PER2:1;           /*!< bit:      2  Periodic Interval 2 Interrupt Enable */
    uint16_t PER3:1;           /*!< bit:      3  Periodic Interval 3 Interrupt Enable */
    uint16_t PER4:1;           /*!< bit:      4  Periodic Interval 4 Interrupt Enable */
    uint16_t PER5:1;           /*!< bit:      5  Periodic Interval 5 Interrupt Enable */
    uint16_t PER6:1;           /*!< bit:      6  Periodic Interval 6 Interrupt Enable */
    uint16_t PER7:1;           /*!< bit:      7  Periodic Interval 7 Interrupt Enable */
    uint16_t CMP0:1;           /*!< bit:      8  Compare 0 Interrupt Enable         */
    uint16_t CMP1:1;           /*!< bit:      9  Compare 1 Interrupt Enable         */
    uint16_t CMP2:1;           /*!< bit:     10  Compare 2 Interrupt Enable         */
    uint16_t CMP3:1;           /*!< bit:     11  Compare 3 Interrupt Enable         */
    uint16_t :2;               /*!< bit: 12..13  Reserved                           */
    uint16_t TAMPER:1;         /*!< bit:     14  Tamper Enable                      */
    uint16_t OVF:1;            /*!< bit:     15  Overflow Interrupt Enable          */
  } bit;                       /*!< Structure used for bit  access                  */
  struct {
    uint16_t PER:8;            /*!< bit:  0.. 7  Periodic Interval x Interrupt Enable */
    uint16_t CMP:4;            /*!< bit:  8..11  Compare x Interrupt Enable         */
    uint16_t :4;               /*!< bit: 12..15  Reserved                           */
  } vec;                       /*!< Structure used for vec  access                  */
  uint16_t reg;                /*!< Type      used for register access              */
} RTC_MODE1_INTENSET_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- RTC_MODE2_INTENSET : (RTC Offset: 0x0A) (R/W 16) MODE2 Interrupt Enable Set -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint16_t PER0:1;           /*!< bit:      0  Periodic Interval 0 Enable         */
    uint16_t PER1:1;           /*!< bit:      1  Periodic Interval 1 Enable         */
    uint16_t PER2:1;           /*!< bit:      2  Periodic Interval 2 Enable         */
    uint16_t PER3:1;           /*!< bit:      3  Periodic Interval 3 Enable         */
    uint16_t PER4:1;           /*!< bit:      4  Periodic Interval 4 Enable         */
    uint16_t PER5:1;           /*!< bit:      5  Periodic Interval 5 Enable         */
    uint16_t PER6:1;           /*!< bit:      6  Periodic Interval 6 Enable         */
    uint16_t PER7:1;           /*!< bit:      7  Periodic Interval 7 Enable         */
    uint16_t ALARM0:1;         /*!< bit:      8  Alarm 0 Interrupt Enable           */
    uint16_t ALARM1:1;         /*!< bit:      9  Alarm 1 Interrupt Enable           */
    uint16_t :4;               /*!< bit: 10..13  Reserved                           */
    uint16_t TAMPER:1;         /*!< bit:     14  Tamper Enable                      */
    uint16_t OVF:1;            /*!< bit:     15  Overflow Interrupt Enable          */
  } bit;                       /*!< Structure used for bit  access                  */
  struct {
    uint16_t PER:8;            /*!< bit:  0.. 7  Periodic Interval x Enable         */
    uint16_t ALARM:2;          /*!< bit:  8.. 9  Alarm x Interrupt Enable           */
    uint16_t :6;               /*!< bit: 10..15  Reserved                           */
  } vec;                       /*!< Structure used for vec  access                  */
  uint16_t reg;                /*!< Type      used for register access              */
} RTC_MODE2_INTENSET_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- RTC_MODE0_INTFLAG : (RTC Offset: 0x0C) (R/W 16) MODE0 Interrupt Flag Status and Clear -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { // __I to avoid read-modify-write on write-to-clear register
  struct {
    __I uint16_t PER0:1;           /*!< bit:      0  Periodic Interval 0                */
    __I uint16_t PER1:1;           /*!< bit:      1  Periodic Interval 1                */
    __I uint16_t PER2:1;           /*!< bit:      2  Periodic Interval 2                */
    __I uint16_t PER3:1;           /*!< bit:      3  Periodic Interval 3                */
    __I uint16_t PER4:1;           /*!< bit:      4  Periodic Interval 4                */
    __I uint16_t PER5:1;           /*!< bit:      5  Periodic Interval 5                */
    __I uint16_t PER6:1;           /*!< bit:      6  Periodic Interval 6                */
    __I uint16_t PER7:1;           /*!< bit:      7  Periodic Interval 7                */
    __I uint16_t CMP0:1;           /*!< bit:      8  Compare 0                          */
    __I uint16_t CMP1:1;           /*!< bit:      9  Compare 1                          */
    __I uint16_t :4;               /*!< bit: 10..13  Reserved                           */
    __I uint16_t TAMPER:1;         /*!< bit:     14  Tamper                             */
    __I uint16_t OVF:1;            /*!< bit:     15  Overflow                           */
  } bit;                       /*!< Structure used for bit  access                  */
  struct {
    __I uint16_t PER:8;            /*!< bit:  0.. 7  Periodic Interval x                */
    __I uint16_t CMP:2;            /*!< bit:  8.. 9  Compare x                          */
    __I uint16_t :6;               /*!< bit: 10..15  Reserved                           */
  } vec;                       /*!< Structure used for vec  access                  */
  uint16_t reg;                /*!< Type      used for register access              */
} RTC_MODE0_INTFLAG_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- RTC_MODE1_INTFLAG : (RTC Offset: 0x0C) (R/W 16) MODE1 Interrupt Flag Status and Clear -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { // __I to avoid read-modify-write on write-to-clear register
  struct {
    __I uint16_t PER0:1;           /*!< bit:      0  Periodic Interval 0                */
    __I uint16_t PER1:1;           /*!< bit:      1  Periodic Interval 1                */
    __I uint16_t PER2:1;           /*!< bit:      2  Periodic Interval 2                */
    __I uint16_t PER3:1;           /*!< bit:      3  Periodic Interval 3                */
    __I uint16_t PER4:1;           /*!< bit:      4  Periodic Interval 4                */
    __I uint16_t PER5:1;           /*!< bit:      5  Periodic Interval 5                */
    __I uint16_t PER6:1;           /*!< bit:      6  Periodic Interval 6                */
    __I uint16_t PER7:1;           /*!< bit:      7  Periodic Interval 7                */
    __I uint16_t CMP0:1;           /*!< bit:      8  Compare 0                          */
    __I uint16_t CMP1:1;           /*!< bit:      9  Compare 1                          */
    __I uint16_t CMP2:1;           /*!< bit:     10  Compare 2                          */
    __I uint16_t CMP3:1;           /*!< bit:     11  Compare 3                          */
    __I uint16_t :2;               /*!< bit: 12..13  Reserved                           */
    __I uint16_t TAMPER:1;         /*!< bit:     14  Tamper                             */
    __I uint16_t OVF:1;            /*!< bit:     15  Overflow                           */
  } bit;                       /*!< Structure used for bit  access                  */
  struct {
    __I uint16_t PER:8;            /*!< bit:  0.. 7  Periodic Interval x                */
    __I uint16_t CMP:4;            /*!< bit:  8..11  Compare x                          */
    __I uint16_t :4;               /*!< bit: 12..15  Reserved                           */
  } vec;                       /*!< Structure used for vec  access                  */
  uint16_t reg;                /*!< Type      used for register access              */
} RTC_MODE1_INTFLAG_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- RTC_MODE2_INTFLAG : (RTC Offset: 0x0C) (R/W 16) MODE2 Interrupt Flag Status and Clear -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { // __I to avoid read-modify-write on write-to-clear register
  struct {
    __I uint16_t PER0:1;           /*!< bit:      0  Periodic Interval 0                */
    __I uint16_t PER1:1;           /*!< bit:      1  Periodic Interval 1                */
    __I uint16_t PER2:1;           /*!< bit:      2  Periodic Interval 2                */
    __I uint16_t PER3:1;           /*!< bit:      3  Periodic Interval 3                */
    __I uint16_t PER4:1;           /*!< bit:      4  Periodic Interval 4                */
    __I uint16_t PER5:1;           /*!< bit:      5  Periodic Interval 5                */
    __I uint16_t PER6:1;           /*!< bit:      6  Periodic Interval 6                */
    __I uint16_t PER7:1;           /*!< bit:      7  Periodic Interval 7                */
    __I uint16_t ALARM0:1;         /*!< bit:      8  Alarm 0                            */
    __I uint16_t ALARM1:1;         /*!< bit:      9  Alarm 1                            */
    __I uint16_t :4;               /*!< bit: 10..13  Reserved                           */
    __I uint16_t TAMPER:1;         /*!< bit:     14  Tamper                             */
    __I uint16_t OVF:1;            /*!< bit:     15  Overflow                           */
  } bit;                       /*!< Structure used for bit  access                  */
  struct {
    __I uint16_t PER:8;            /*!< bit:  0.. 7  Periodic Interval x                */
    __I uint16_t ALARM:2;          /*!< bit:  8.. 9  Alarm x                            */
    __I uint16_t :6;               /*!< bit: 10..15  Reserved                           */
  } vec;                       /*!< Structure used for vec  access                  */
  uint16_t reg;                /*!< Type      used for register access              */
} RTC_MODE2_INTFLAG_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- RTC_DBGCTRL : (RTC Offset: 0x0E) (R/W 8) Debug Control -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint8_t  DBGRUN:1;         /*!< bit:      0  Run During Debug                   */
    uint8_t  :7;               /*!< bit:  1.. 7  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint8_t reg;                 /*!< Type      used for register access              */
} RTC_DBGCTRL_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- RTC_MODE0_SYNCBUSY : (RTC Offset: 0x10) ( R/ 32) MODE0 Synchronization Busy Status -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t SWRST:1;          /*!< bit:      0  Software Reset Busy                */
    uint32_t ENABLE:1;         /*!< bit:      1  Enable Bit Busy                    */
    uint32_t FREQCORR:1;       /*!< bit:      2  FREQCORR Register Busy             */
    uint32_t COUNT:1;          /*!< bit:      3  COUNT Register Busy                */
    uint32_t :1;               /*!< bit:      4  Reserved                           */
    uint32_t COMP0:1;          /*!< bit:      5  COMP 0 Register Busy               */
    uint32_t COMP1:1;          /*!< bit:      6  COMP 1 Register Busy               */
    uint32_t :8;               /*!< bit:  7..14  Reserved                           */
    uint32_t COUNTSYNC:1;      /*!< bit:     15  Count Synchronization Enable Bit Busy */
    uint32_t GP0:1;            /*!< bit:     16  General Purpose 0 Register Busy    */
    uint32_t GP1:1;            /*!< bit:     17  General Purpose 1 Register Busy    */
    uint32_t GP2:1;            /*!< bit:     18  General Purpose 2 Register Busy    */
    uint32_t GP3:1;            /*!< bit:     19  General Purpose 3 Register Busy    */
    uint32_t :12;              /*!< bit: 20..31  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  struct {
    uint32_t :5;               /*!< bit:  0.. 4  Reserved                           */
    uint32_t COMP:2;           /*!< bit:  5.. 6  COMP x Register Busy               */
    uint32_t :9;               /*!< bit:  7..15  Reserved                           */
    uint32_t GP:4;             /*!< bit: 16..19  General Purpose x Register Busy    */
    uint32_t :12;              /*!< bit: 20..31  Reserved                           */
  } vec;                       /*!< Structure used for vec  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} RTC_MODE0_SYNCBUSY_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */
/* -------- RTC_MODE1_SYNCBUSY : (RTC Offset: 0x10) ( R/ 32) MODE1 Synchronization Busy Status -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t SWRST:1;          /*!< bit:      0  Software Reset Bit Busy            */
    uint32_t ENABLE:1;         /*!< bit:      1  Enable Bit Busy                    */
    uint32_t FREQCORR:1;       /*!< bit:      2  FREQCORR Register Busy             */
    uint32_t COUNT:1;          /*!< bit:      3  COUNT Register Busy                */
    uint32_t PER:1;            /*!< bit:      4  PER Register Busy                  */
    uint32_t COMP0:1;          /*!< bit:      5  COMP 0 Register Busy               */
    uint32_t COMP1:1;          /*!< bit:      6  COMP 1 Register Busy               */
    uint32_t COMP2:1;          /*!< bit:      7  COMP 2 Register Busy               */
    uint32_t COMP3:1;          /*!< bit:      8  COMP 3 Register Busy               */
    uint32_t :6;               /*!< bit:  9..14  Reserved                           */
    uint32_t COUNTSYNC:1;      /*!< bit:     15  Count Synchronization Enable Bit Busy */
    uint32_t GP0:1;            /*!< bit:     16  General Purpose 0 Register Busy    */
    uint32_t GP1:1;            /*!< bit:     17  General Purpose 1 Register Busy    */
    uint32_t GP2:1;            /*!< bit:     18  General Purpose 2 Register Busy    */
    uint32_t GP3:1;            /*!< bit:     19  General Purpose 3 Register Busy    */
    uint32_t :12;              /*!< bit: 20..31  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  struct {
    uint32_t :5;               /*!< bit:  0.. 4  Reserved                           */
    uint32_t COMP:4;           /*!< bit:  5.. 8  COMP x Register Busy               */
    uint32_t :7;               /*!< bit:  9..15  Reserved                           */
    uint32_t GP:4;             /*!< bit: 16..19  General Purpose x Register Busy    */
    uint32_t :12;              /*!< bit: 20..31  Reserved                           */
  } vec;                       /*!< Structure used for vec  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} RTC_MODE1_SYNCBUSY_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- RTC_MODE2_SYNCBUSY : (RTC Offset: 0x10) ( R/ 32) MODE2 Synchronization Busy Status -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t SWRST:1;          /*!< bit:      0  Software Reset Bit Busy            */
    uint32_t ENABLE:1;         /*!< bit:      1  Enable Bit Busy                    */
    uint32_t FREQCORR:1;       /*!< bit:      2  FREQCORR Register Busy             */
    uint32_t CLOCK:1;          /*!< bit:      3  CLOCK Register Busy                */
    uint32_t :1;               /*!< bit:      4  Reserved                           */
    uint32_t ALARM0:1;         /*!< bit:      5  ALARM 0 Register Busy              */
    uint32_t ALARM1:1;         /*!< bit:      6  ALARM 1 Register Busy              */
    uint32_t :4;               /*!< bit:  7..10  Reserved                           */
    uint32_t MASK0:1;          /*!< bit:     11  MASK 0 Register Busy               */
    uint32_t MASK1:1;          /*!< bit:     12  MASK 1 Register Busy               */
    uint32_t :2;               /*!< bit: 13..14  Reserved                           */
    uint32_t CLOCKSYNC:1;      /*!< bit:     15  Clock Synchronization Enable Bit Busy */
    uint32_t GP0:1;            /*!< bit:     16  General Purpose 0 Register Busy    */
    uint32_t GP1:1;            /*!< bit:     17  General Purpose 1 Register Busy    */
    uint32_t GP2:1;            /*!< bit:     18  General Purpose 2 Register Busy    */
    uint32_t GP3:1;            /*!< bit:     19  General Purpose 3 Register Busy    */
    uint32_t :12;              /*!< bit: 20..31  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  struct {
    uint32_t :5;               /*!< bit:  0.. 4  Reserved                           */
    uint32_t ALARM:2;          /*!< bit:  5.. 6  ALARM x Register Busy              */
    uint32_t :4;               /*!< bit:  7..10  Reserved                           */
    uint32_t MASK:2;           /*!< bit: 11..12  MASK x Register Busy               */
    uint32_t :3;               /*!< bit: 13..15  Reserved                           */
    uint32_t GP:4;             /*!< bit: 16..19  General Purpose x Register Busy    */
    uint32_t :12;              /*!< bit: 20..31  Reserved                           */
  } vec;                       /*!< Structure used for vec  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} RTC_MODE2_SYNCBUSY_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- RTC_FREQCORR : (RTC Offset: 0x14) (R/W 8) Frequency Correction -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint8_t  VALUE:7;          /*!< bit:  0.. 6  Correction Value                   */
    uint8_t  SIGN:1;           /*!< bit:      7  Correction Sign                    */
  } bit;                       /*!< Structure used for bit  access                  */
  uint8_t reg;                 /*!< Type      used for register access              */
} RTC_FREQCORR_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- RTC_MODE0_COUNT : (RTC Offset: 0x18) (R/W 32) MODE0 Counter Value -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t COUNT:32;         /*!< bit:  0..31  Counter Value                      */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} RTC_MODE0_COUNT_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- RTC_MODE1_COUNT : (RTC Offset: 0x18) (R/W 16) MODE1 Counter Value -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint16_t COUNT:16;         /*!< bit:  0..15  Counter Value                      */
  } bit;                       /*!< Structure used for bit  access                  */
  uint16_t reg;                /*!< Type      used for register access              */
} RTC_MODE1_COUNT_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- RTC_MODE2_CLOCK : (RTC Offset: 0x18) (R/W 32) MODE2 Clock Value -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t SECOND:6;         /*!< bit:  0.. 5  Second                             */
    uint32_t MINUTE:6;         /*!< bit:  6..11  Minute                             */
    uint32_t HOUR:5;           /*!< bit: 12..16  Hour                               */
    uint32_t DAY:5;            /*!< bit: 17..21  Day                                */
    uint32_t MONTH:4;          /*!< bit: 22..25  Month                              */
    uint32_t YEAR:6;           /*!< bit: 26..31  Year                               */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} RTC_MODE2_CLOCK_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- RTC_MODE1_PER : (RTC Offset: 0x1C) (R/W 16) MODE1 Counter Period -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint16_t PER:16;           /*!< bit:  0..15  Counter Period                     */
  } bit;                       /*!< Structure used for bit  access                  */
  uint16_t reg;                /*!< Type      used for register access              */
} RTC_MODE1_PER_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- RTC_MODE0_COMP : (RTC Offset: 0x20) (R/W 32) MODE0 Compare n Value -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t COMP:32;          /*!< bit:  0..31  Compare Value                      */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} RTC_MODE0_COMP_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- RTC_MODE1_COMP : (RTC Offset: 0x20) (R/W 16) MODE1 Compare n Value -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint16_t COMP:16;          /*!< bit:  0..15  Compare Value                      */
  } bit;                       /*!< Structure used for bit  access                  */
  uint16_t reg;                /*!< Type      used for register access              */
} RTC_MODE1_COMP_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- RTC_MODE2_ALARM : (RTC Offset: 0x20) (R/W 32) MODE2 MODE2_ALARM Alarm n Value -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t SECOND:6;         /*!< bit:  0.. 5  Second                             */
    uint32_t MINUTE:6;         /*!< bit:  6..11  Minute                             */
    uint32_t HOUR:5;           /*!< bit: 12..16  Hour                               */
    uint32_t DAY:5;            /*!< bit: 17..21  Day                                */
    uint32_t MONTH:4;          /*!< bit: 22..25  Month                              */
    uint32_t YEAR:6;           /*!< bit: 26..31  Year                               */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} RTC_MODE2_ALARM_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

#define RTC_MODE2_ALARM_OFFSET      0x20         /**< \brief (RTC_MODE2_ALARM offset) MODE2_ALARM Alarm n Value */
#define RTC_MODE2_ALARM_RESETVALUE  _U_(0x00000000) /**< \brief (RTC_MODE2_ALARM reset_value) MODE2_ALARM Alarm n Value */

#define RTC_MODE2_ALARM_SECOND_Pos  0            /**< \brief (RTC_MODE2_ALARM) Second */
#define RTC_MODE2_ALARM_SECOND_Msk  (_U_(0x3F) << RTC_MODE2_ALARM_SECOND_Pos)
#define RTC_MODE2_ALARM_SECOND(value) (RTC_MODE2_ALARM_SECOND_Msk & ((value) << RTC_MODE2_ALARM_SECOND_Pos))
#define RTC_MODE2_ALARM_MINUTE_Pos  6            /**< \brief (RTC_MODE2_ALARM) Minute */
#define RTC_MODE2_ALARM_MINUTE_Msk  (_U_(0x3F) << RTC_MODE2_ALARM_MINUTE_Pos)
#define RTC_MODE2_ALARM_MINUTE(value) (RTC_MODE2_ALARM_MINUTE_Msk & ((value) << RTC_MODE2_ALARM_MINUTE_Pos))
#define RTC_MODE2_ALARM_HOUR_Pos    12           /**< \brief (RTC_MODE2_ALARM) Hour */
#define RTC_MODE2_ALARM_HOUR_Msk    (_U_(0x1F) << RTC_MODE2_ALARM_HOUR_Pos)
#define RTC_MODE2_ALARM_HOUR(value) (RTC_MODE2_ALARM_HOUR_Msk & ((value) << RTC_MODE2_ALARM_HOUR_Pos))
#define   RTC_MODE2_ALARM_HOUR_AM_Val     _U_(0x0)   /**< \brief (RTC_MODE2_ALARM) Morning hour */
#define   RTC_MODE2_ALARM_HOUR_PM_Val     _U_(0x10)   /**< \brief (RTC_MODE2_ALARM) Afternoon hour */
#define RTC_MODE2_ALARM_HOUR_AM     (RTC_MODE2_ALARM_HOUR_AM_Val   << RTC_MODE2_ALARM_HOUR_Pos)
#define RTC_MODE2_ALARM_HOUR_PM     (RTC_MODE2_ALARM_HOUR_PM_Val   << RTC_MODE2_ALARM_HOUR_Pos)
#define RTC_MODE2_ALARM_DAY_Pos     17           /**< \brief (RTC_MODE2_ALARM) Day */
#define RTC_MODE2_ALARM_DAY_Msk     (_U_(0x1F) << RTC_MODE2_ALARM_DAY_Pos)
#define RTC_MODE2_ALARM_DAY(value)  (RTC_MODE2_ALARM_DAY_Msk & ((value) << RTC_MODE2_ALARM_DAY_Pos))
#define RTC_MODE2_ALARM_MONTH_Pos   22           /**< \brief (RTC_MODE2_ALARM) Month */
#define RTC_MODE2_ALARM_MONTH_Msk   (_U_(0xF) << RTC_MODE2_ALARM_MONTH_Pos)
#define RTC_MODE2_ALARM_MONTH(value) (RTC_MODE2_ALARM_MONTH_Msk & ((value) << RTC_MODE2_ALARM_MONTH_Pos))
#define RTC_MODE2_ALARM_YEAR_Pos    26           /**< \brief (RTC_MODE2_ALARM) Year */
#define RTC_MODE2_ALARM_YEAR_Msk    (_U_(0x3F) << RTC_MODE2_ALARM_YEAR_Pos)
#define RTC_MODE2_ALARM_YEAR(value) (RTC_MODE2_ALARM_YEAR_Msk & ((value) << RTC_MODE2_ALARM_YEAR_Pos))
#define RTC_MODE2_ALARM_MASK        _U_(0xFFFFFFFF) /**< \brief (RTC_MODE2_ALARM) MASK Register */

/* -------- RTC_MODE2_MASK : (RTC Offset: 0x24) (R/W  8) MODE2 MODE2_ALARM Alarm n Mask -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint8_t  SEL:3;            /*!< bit:  0.. 2  Alarm Mask Selection               */
    uint8_t  :5;               /*!< bit:  3.. 7  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint8_t reg;                 /*!< Type      used for register access              */
} RTC_MODE2_MASK_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

#define RTC_MODE2_MASK_OFFSET       0x24         /**< \brief (RTC_MODE2_MASK offset) MODE2_ALARM Alarm n Mask */
#define RTC_MODE2_MASK_RESETVALUE   _U_(0x00)    /**< \brief (RTC_MODE2_MASK reset_value) MODE2_ALARM Alarm n Mask */

#define RTC_MODE2_MASK_SEL_Pos      0            /**< \brief (RTC_MODE2_MASK) Alarm Mask Selection */
#define RTC_MODE2_MASK_SEL_Msk      (_U_(0x7) << RTC_MODE2_MASK_SEL_Pos)
#define RTC_MODE2_MASK_SEL(value)   (RTC_MODE2_MASK_SEL_Msk & ((value) << RTC_MODE2_MASK_SEL_Pos))
#define   RTC_MODE2_MASK_SEL_OFF_Val      _U_(0x0)   /**< \brief (RTC_MODE2_MASK) Alarm Disabled */
#define   RTC_MODE2_MASK_SEL_SS_Val       _U_(0x1)   /**< \brief (RTC_MODE2_MASK) Match seconds only */
#define   RTC_MODE2_MASK_SEL_MMSS_Val     _U_(0x2)   /**< \brief (RTC_MODE2_MASK) Match seconds and minutes only */
#define   RTC_MODE2_MASK_SEL_HHMMSS_Val   _U_(0x3)   /**< \brief (RTC_MODE2_MASK) Match seconds, minutes, and hours only */
#define   RTC_MODE2_MASK_SEL_DDHHMMSS_Val _U_(0x4)   /**< \brief (RTC_MODE2_MASK) Match seconds, minutes, hours, and days only */
#define   RTC_MODE2_MASK_SEL_MMDDHHMMSS_Val _U_(0x5)   /**< \brief (RTC_MODE2_MASK) Match seconds, minutes, hours, days, and months only */
#define   RTC_MODE2_MASK_SEL_YYMMDDHHMMSS_Val _U_(0x6)   /**< \brief (RTC_MODE2_MASK) Match seconds, minutes, hours, days, months, and years */
#define RTC_MODE2_MASK_SEL_OFF      (RTC_MODE2_MASK_SEL_OFF_Val    << RTC_MODE2_MASK_SEL_Pos)
#define RTC_MODE2_MASK_SEL_SS       (RTC_MODE2_MASK_SEL_SS_Val     << RTC_MODE2_MASK_SEL_Pos)
#define RTC_MODE2_MASK_SEL_MMSS     (RTC_MODE2_MASK_SEL_MMSS_Val   << RTC_MODE2_MASK_SEL_Pos)
#define RTC_MODE2_MASK_SEL_HHMMSS   (RTC_MODE2_MASK_SEL_HHMMSS_Val << RTC_MODE2_MASK_SEL_Pos)
#define RTC_MODE2_MASK_SEL_DDHHMMSS (RTC_MODE2_MASK_SEL_DDHHMMSS_Val << RTC_MODE2_MASK_SEL_Pos)
#define RTC_MODE2_MASK_SEL_MMDDHHMMSS (RTC_MODE2_MASK_SEL_MMDDHHMMSS_Val << RTC_MODE2_MASK_SEL_Pos)
#define RTC_MODE2_MASK_SEL_YYMMDDHHMMSS (RTC_MODE2_MASK_SEL_YYMMDDHHMMSS_Val << RTC_MODE2_MASK_SEL_Pos)
#define RTC_MODE2_MASK_MASK         _U_(0x07)    /**< \brief (RTC_MODE2_MASK) MASK Register */

/* -------- RTC_GP : (RTC Offset: 0x40) (R/W 32) General Purpose -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t GP:32;            /*!< bit:  0..31  General Purpose                    */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} RTC_GP_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- RTC_TAMPCTRL : (RTC Offset: 0x60) (R/W 32) Tamper Control -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t IN0ACT:2;         /*!< bit:  0.. 1  Tamper Input 0 Action              */
    uint32_t IN1ACT:2;         /*!< bit:  2.. 3  Tamper Input 1 Action              */
    uint32_t IN2ACT:2;         /*!< bit:  4.. 5  Tamper Input 2 Action              */
    uint32_t IN3ACT:2;         /*!< bit:  6.. 7  Tamper Input 3 Action              */
    uint32_t IN4ACT:2;         /*!< bit:  8.. 9  Tamper Input 4 Action              */
    uint32_t :6;               /*!< bit: 10..15  Reserved                           */
    uint32_t TAMLVL0:1;        /*!< bit:     16  Tamper Level Select 0              */
    uint32_t TAMLVL1:1;        /*!< bit:     17  Tamper Level Select 1              */
    uint32_t TAMLVL2:1;        /*!< bit:     18  Tamper Level Select 2              */
    uint32_t TAMLVL3:1;        /*!< bit:     19  Tamper Level Select 3              */
    uint32_t TAMLVL4:1;        /*!< bit:     20  Tamper Level Select 4              */
    uint32_t :3;               /*!< bit: 21..23  Reserved                           */
    uint32_t DEBNC0:1;         /*!< bit:     24  Debouncer Enable 0                 */
    uint32_t DEBNC1:1;         /*!< bit:     25  Debouncer Enable 1                 */
    uint32_t DEBNC2:1;         /*!< bit:     26  Debouncer Enable 2                 */
    uint32_t DEBNC3:1;         /*!< bit:     27  Debouncer Enable 3                 */
    uint32_t DEBNC4:1;         /*!< bit:     28  Debouncer Enable 4                 */
    uint32_t :3;               /*!< bit: 29..31  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  struct {
    uint32_t :16;              /*!< bit:  0..15  Reserved                           */
    uint32_t TAMLVL:5;         /*!< bit: 16..20  Tamper Level Select x              */
    uint32_t :3;               /*!< bit: 21..23  Reserved                           */
    uint32_t DEBNC:5;          /*!< bit: 24..28  Debouncer Enable x                 */
    uint32_t :3;               /*!< bit: 29..31  Reserved                           */
  } vec;                       /*!< Structure used for vec  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} RTC_TAMPCTRL_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- RTC_MODE0_TIMESTAMP : (RTC Offset: 0x64) ( R/ 32) MODE0 Timestamp -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t COUNT:32;         /*!< bit:  0..31  Count Timestamp Value              */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} RTC_MODE0_TIMESTAMP_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- RTC_MODE1_TIMESTAMP : (RTC Offset: 0x64) ( R/ 32) MODE1 Timestamp -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t COUNT:16;         /*!< bit:  0..15  Count Timestamp Value              */
    uint32_t :16;              /*!< bit: 16..31  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} RTC_MODE1_TIMESTAMP_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- RTC_MODE2_TIMESTAMP : (RTC Offset: 0x64) ( R/ 32) MODE2 Timestamp -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t SECOND:6;         /*!< bit:  0.. 5  Second Timestamp Value             */
    uint32_t MINUTE:6;         /*!< bit:  6..11  Minute Timestamp Value             */
    uint32_t HOUR:5;           /*!< bit: 12..16  Hour Timestamp Value               */
    uint32_t DAY:5;            /*!< bit: 17..21  Day Timestamp Value                */
    uint32_t MONTH:4;          /*!< bit: 22..25  Month Timestamp Value              */
    uint32_t YEAR:6;           /*!< bit: 26..31  Year Timestamp Value               */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} RTC_MODE2_TIMESTAMP_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- RTC_TAMPID : (RTC Offset: 0x68) (R/W 32) Tamper ID -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t TAMPID0:1;        /*!< bit:      0  Tamper Input 0 Detected            */
    uint32_t TAMPID1:1;        /*!< bit:      1  Tamper Input 1 Detected            */
    uint32_t TAMPID2:1;        /*!< bit:      2  Tamper Input 2 Detected            */
    uint32_t TAMPID3:1;        /*!< bit:      3  Tamper Input 3 Detected            */
    uint32_t TAMPID4:1;        /*!< bit:      4  Tamper Input 4 Detected            */
    uint32_t :26;              /*!< bit:  5..30  Reserved                           */
    uint32_t TAMPEVT:1;        /*!< bit:     31  Tamper Event Detected              */
  } bit;                       /*!< Structure used for bit  access                  */
  struct {
    uint32_t TAMPID:5;         /*!< bit:  0.. 4  Tamper Input x Detected            */
    uint32_t :27;              /*!< bit:  5..31  Reserved                           */
  } vec;                       /*!< Structure used for vec  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} RTC_TAMPID_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- RTC_BKUP : (RTC Offset: 0x80) (R/W 32) Backup -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t BKUP:32;          /*!< bit:  0..31  Backup                             */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} RTC_BKUP_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/** \brief RtcMode2Alarm hardware registers */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef struct {
  __IO RTC_MODE2_ALARM_Type      ALARM;       /**< \brief Offset: 0x00 (R/W 32) MODE2_ALARM Alarm n Value */
  __IO RTC_MODE2_MASK_Type       MASK;        /**< \brief Offset: 0x04 (R/W  8) MODE2_ALARM Alarm n Mask */
       RoReg8                    Reserved1[0x3];
} RtcMode2Alarm;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/** \brief RTC_MODE0 hardware registers */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef struct { /* 32-bit Counter with Single 32-bit Compare */
  __IO RTC_MODE0_CTRLA_Type      CTRLA;       /**< \brief Offset: 0x00 (R/W 16) MODE0 Control A */
  __IO RTC_MODE0_CTRLB_Type      CTRLB;       /**< \brief Offset: 0x02 (R/W 16) MODE0 Control B */
  __IO RTC_MODE0_EVCTRL_Type     EVCTRL;      /**< \brief Offset: 0x04 (R/W 32) MODE0 Event Control */
  __IO RTC_MODE0_INTENCLR_Type   INTENCLR;    /**< \brief Offset: 0x08 (R/W 16) MODE0 Interrupt Enable Clear */
  __IO RTC_MODE0_INTENSET_Type   INTENSET;    /**< \brief Offset: 0x0A (R/W 16) MODE0 Interrupt Enable Set */
  __IO RTC_MODE0_INTFLAG_Type    INTFLAG;     /**< \brief Offset: 0x0C (R/W 16) MODE0 Interrupt Flag Status and Clear */
  __IO RTC_DBGCTRL_Type          DBGCTRL;     /**< \brief Offset: 0x0E (R/W  8) Debug Control */
       RoReg8                    Reserved1[0x1];
  __I  RTC_MODE0_SYNCBUSY_Type   SYNCBUSY;    /**< \brief Offset: 0x10 (R/  32) MODE0 Synchronization Busy Status */
  __IO RTC_FREQCORR_Type         FREQCORR;    /**< \brief Offset: 0x14 (R/W  8) Frequency Correction */
       RoReg8                    Reserved2[0x3];
  __IO RTC_MODE0_COUNT_Type      COUNT;       /**< \brief Offset: 0x18 (R/W 32) MODE0 Counter Value */
       RoReg8                    Reserved3[0x4];
  __IO RTC_MODE0_COMP_Type       COMP[2];     /**< \brief Offset: 0x20 (R/W 32) MODE0 Compare n Value */
       RoReg8                    Reserved4[0x18];
  __IO RTC_GP_Type               GP[4];       /**< \brief Offset: 0x40 (R/W 32) General Purpose */
       RoReg8                    Reserved5[0x10];
  __IO RTC_TAMPCTRL_Type         TAMPCTRL;    /**< \brief Offset: 0x60 (R/W 32) Tamper Control */
  __I  RTC_MODE0_TIMESTAMP_Type  TIMESTAMP;   /**< \brief Offset: 0x64 (R/  32) MODE0 Timestamp */
  __IO RTC_TAMPID_Type           TAMPID;      /**< \brief Offset: 0x68 (R/W 32) Tamper ID */
       RoReg8                    Reserved6[0x14];
  __IO RTC_BKUP_Type             BKUP[8];     /**< \brief Offset: 0x80 (R/W 32) Backup */
} RtcMode0;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/** \brief RTC_MODE1 hardware registers */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef struct { /* 16-bit Counter with Two 16-bit Compares */
  __IO RTC_MODE1_CTRLA_Type      CTRLA;       /**< \brief Offset: 0x00 (R/W 16) MODE1 Control A */
  __IO RTC_MODE1_CTRLB_Type      CTRLB;       /**< \brief Offset: 0x02 (R/W 16) MODE1 Control B */
  __IO RTC_MODE1_EVCTRL_Type     EVCTRL;      /**< \brief Offset: 0x04 (R/W 32) MODE1 Event Control */
  __IO RTC_MODE1_INTENCLR_Type   INTENCLR;    /**< \brief Offset: 0x08 (R/W 16) MODE1 Interrupt Enable Clear */
  __IO RTC_MODE1_INTENSET_Type   INTENSET;    /**< \brief Offset: 0x0A (R/W 16) MODE1 Interrupt Enable Set */
  __IO RTC_MODE1_INTFLAG_Type    INTFLAG;     /**< \brief Offset: 0x0C (R/W 16) MODE1 Interrupt Flag Status and Clear */
  __IO RTC_DBGCTRL_Type          DBGCTRL;     /**< \brief Offset: 0x0E (R/W  8) Debug Control */
       RoReg8                    Reserved1[0x1];
  __I  RTC_MODE1_SYNCBUSY_Type   SYNCBUSY;    /**< \brief Offset: 0x10 (R/  32) MODE1 Synchronization Busy Status */
  __IO RTC_FREQCORR_Type         FREQCORR;    /**< \brief Offset: 0x14 (R/W  8) Frequency Correction */
       RoReg8                    Reserved2[0x3];
  __IO RTC_MODE1_COUNT_Type      COUNT;       /**< \brief Offset: 0x18 (R/W 16) MODE1 Counter Value */
       RoReg8                    Reserved3[0x2];
  __IO RTC_MODE1_PER_Type        PER;         /**< \brief Offset: 0x1C (R/W 16) MODE1 Counter Period */
       RoReg8                    Reserved4[0x2];
  __IO RTC_MODE1_COMP_Type       COMP[4];     /**< \brief Offset: 0x20 (R/W 16) MODE1 Compare n Value */
       RoReg8                    Reserved5[0x18];
  __IO RTC_GP_Type               GP[4];       /**< \brief Offset: 0x40 (R/W 32) General Purpose */
       RoReg8                    Reserved6[0x10];
  __IO RTC_TAMPCTRL_Type         TAMPCTRL;    /**< \brief Offset: 0x60 (R/W 32) Tamper Control */
  __I  RTC_MODE1_TIMESTAMP_Type  TIMESTAMP;   /**< \brief Offset: 0x64 (R/  32) MODE1 Timestamp */
  __IO RTC_TAMPID_Type           TAMPID;      /**< \brief Offset: 0x68 (R/W 32) Tamper ID */
       RoReg8                    Reserved7[0x14];
  __IO RTC_BKUP_Type             BKUP[8];     /**< \brief Offset: 0x80 (R/W 32) Backup */
} RtcMode1;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/** \brief RTC_MODE2 hardware registers */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef struct { /* Clock/Calendar with Alarm */
  __IO RTC_MODE2_CTRLA_Type      CTRLA;       /**< \brief Offset: 0x00 (R/W 16) MODE2 Control A */
  __IO RTC_MODE2_CTRLB_Type      CTRLB;       /**< \brief Offset: 0x02 (R/W 16) MODE2 Control B */
  __IO RTC_MODE2_EVCTRL_Type     EVCTRL;      /**< \brief Offset: 0x04 (R/W 32) MODE2 Event Control */
  __IO RTC_MODE2_INTENCLR_Type   INTENCLR;    /**< \brief Offset: 0x08 (R/W 16) MODE2 Interrupt Enable Clear */
  __IO RTC_MODE2_INTENSET_Type   INTENSET;    /**< \brief Offset: 0x0A (R/W 16) MODE2 Interrupt Enable Set */
  __IO RTC_MODE2_INTFLAG_Type    INTFLAG;     /**< \brief Offset: 0x0C (R/W 16) MODE2 Interrupt Flag Status and Clear */
  __IO RTC_DBGCTRL_Type          DBGCTRL;     /**< \brief Offset: 0x0E (R/W  8) Debug Control */
       RoReg8                    Reserved1[0x1];
  __I  RTC_MODE2_SYNCBUSY_Type   SYNCBUSY;    /**< \brief Offset: 0x10 (R/  32) MODE2 Synchronization Busy Status */
  __IO RTC_FREQCORR_Type         FREQCORR;    /**< \brief Offset: 0x14 (R/W  8) Frequency Correction */
       RoReg8                    Reserved2[0x3];
  __IO RTC_MODE2_CLOCK_Type      CLOCK;       /**< \brief Offset: 0x18 (R/W 32) MODE2 Clock Value */
       RoReg8                    Reserved3[0x4];
       RtcMode2Alarm             Mode2Alarm[2]; /**< \brief Offset: 0x20 RtcMode2Alarm groups [NUM_OF_ALARMS] */
       RoReg8                    Reserved4[0x10];
  __IO RTC_GP_Type               GP[4];       /**< \brief Offset: 0x40 (R/W 32) General Purpose */
       RoReg8                    Reserved5[0x10];
  __IO RTC_TAMPCTRL_Type         TAMPCTRL;    /**< \brief Offset: 0x60 (R/W 32) Tamper Control */
  __I  RTC_MODE2_TIMESTAMP_Type  TIMESTAMP;   /**< \brief Offset: 0x64 (R/  32) MODE2 Timestamp */
  __IO RTC_TAMPID_Type           TAMPID;      /**< \brief Offset: 0x68 (R/W 32) Tamper ID */
       RoReg8                    Reserved6[0x14];
  __IO RTC_BKUP_Type             BKUP[8];     /**< \brief Offset: 0x80 (R/W 32) Backup */
} RtcMode2;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
       RtcMode0                  MODE0;       /**< \brief Offset: 0x00 32-bit Counter with Single 32-bit Compare */
       RtcMode1                  MODE1;       /**< \brief Offset: 0x00 16-bit Counter with Two 16-bit Compares */
       RtcMode2                  MODE2;       /**< \brief Offset: 0x00 Clock/Calendar with Alarm */
} Rtc;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

#endif /* _MICROCHIP_PIC32CXSG_RTC_COMPONENT_FIXUP_H_ */
