/*
 * Copyright (c) 2024 Microchip
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _MICROCHIP_PIC32CXSG_TC_COMPONENT_FIXUP_H_
#define _MICROCHIP_PIC32CXSG_TC_COMPONENT_FIXUP_H_

/* -------- TC_CTRLA : (TC Offset: 0x00) (R/W 32) Control A -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t SWRST:1;          /*!< bit:      0  Software Reset                     */
    uint32_t ENABLE:1;         /*!< bit:      1  Enable                             */
    uint32_t MODE:2;           /*!< bit:  2.. 3  Timer Counter Mode                 */
    uint32_t PRESCSYNC:2;      /*!< bit:  4.. 5  Prescaler and Counter Synchronization */
    uint32_t RUNSTDBY:1;       /*!< bit:      6  Run during Standby                 */
    uint32_t ONDEMAND:1;       /*!< bit:      7  Clock On Demand                    */
    uint32_t PRESCALER:3;      /*!< bit:  8..10  Prescaler                          */
    uint32_t ALOCK:1;          /*!< bit:     11  Auto Lock                          */
    uint32_t :4;               /*!< bit: 12..15  Reserved                           */
    uint32_t CAPTEN0:1;        /*!< bit:     16  Capture Channel 0 Enable           */
    uint32_t CAPTEN1:1;        /*!< bit:     17  Capture Channel 1 Enable           */
    uint32_t :2;               /*!< bit: 18..19  Reserved                           */
    uint32_t COPEN0:1;         /*!< bit:     20  Capture On Pin 0 Enable            */
    uint32_t COPEN1:1;         /*!< bit:     21  Capture On Pin 1 Enable            */
    uint32_t :2;               /*!< bit: 22..23  Reserved                           */
    uint32_t CAPTMODE0:2;      /*!< bit: 24..25  Capture Mode Channel 0             */
    uint32_t :1;               /*!< bit:     26  Reserved                           */
    uint32_t CAPTMODE1:2;      /*!< bit: 27..28  Capture mode Channel 1             */
    uint32_t :3;               /*!< bit: 29..31  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  struct {
    uint32_t :16;              /*!< bit:  0..15  Reserved                           */
    uint32_t CAPTEN:2;         /*!< bit: 16..17  Capture Channel x Enable           */
    uint32_t :2;               /*!< bit: 18..19  Reserved                           */
    uint32_t COPEN:2;          /*!< bit: 20..21  Capture On Pin x Enable            */
    uint32_t :10;              /*!< bit: 22..31  Reserved                           */
  } vec;                       /*!< Structure used for vec  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} TC_CTRLA_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- TC_CTRLBCLR : (TC Offset: 0x04) (R/W 8) Control B Clear -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint8_t  DIR:1;            /*!< bit:      0  Counter Direction                  */
    uint8_t  LUPD:1;           /*!< bit:      1  Lock Update                        */
    uint8_t  ONESHOT:1;        /*!< bit:      2  One-Shot on Counter                */
    uint8_t  :2;               /*!< bit:  3.. 4  Reserved                           */
    uint8_t  CMD:3;            /*!< bit:  5.. 7  Command                            */
  } bit;                       /*!< Structure used for bit  access                  */
  uint8_t reg;                 /*!< Type      used for register access              */
} TC_CTRLBCLR_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- TC_CTRLBSET : (TC Offset: 0x05) (R/W 8) Control B Set -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint8_t  DIR:1;            /*!< bit:      0  Counter Direction                  */
    uint8_t  LUPD:1;           /*!< bit:      1  Lock Update                        */
    uint8_t  ONESHOT:1;        /*!< bit:      2  One-Shot on Counter                */
    uint8_t  :2;               /*!< bit:  3.. 4  Reserved                           */
    uint8_t  CMD:3;            /*!< bit:  5.. 7  Command                            */
  } bit;                       /*!< Structure used for bit  access                  */
  uint8_t reg;                 /*!< Type      used for register access              */
} TC_CTRLBSET_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- TC_EVCTRL : (TC Offset: 0x06) (R/W 16) Event Control -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint16_t EVACT:3;          /*!< bit:  0.. 2  Event Action                       */
    uint16_t :1;               /*!< bit:      3  Reserved                           */
    uint16_t TCINV:1;          /*!< bit:      4  TC Event Input Polarity            */
    uint16_t TCEI:1;           /*!< bit:      5  TC Event Enable                    */
    uint16_t :2;               /*!< bit:  6.. 7  Reserved                           */
    uint16_t OVFEO:1;          /*!< bit:      8  Event Output Enable                */
    uint16_t :3;               /*!< bit:  9..11  Reserved                           */
    uint16_t MCEO0:1;          /*!< bit:     12  MC Event Output Enable 0           */
    uint16_t MCEO1:1;          /*!< bit:     13  MC Event Output Enable 1           */
    uint16_t :2;               /*!< bit: 14..15  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  struct {
    uint16_t :12;              /*!< bit:  0..11  Reserved                           */
    uint16_t MCEO:2;           /*!< bit: 12..13  MC Event Output Enable x           */
    uint16_t :2;               /*!< bit: 14..15  Reserved                           */
  } vec;                       /*!< Structure used for vec  access                  */
  uint16_t reg;                /*!< Type      used for register access              */
} TC_EVCTRL_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- TC_INTENCLR : (TC Offset: 0x08) (R/W 8) Interrupt Enable Clear -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint8_t  OVF:1;            /*!< bit:      0  OVF Interrupt Disable              */
    uint8_t  ERR:1;            /*!< bit:      1  ERR Interrupt Disable              */
    uint8_t  :2;               /*!< bit:  2.. 3  Reserved                           */
    uint8_t  MC0:1;            /*!< bit:      4  MC Interrupt Disable 0             */
    uint8_t  MC1:1;            /*!< bit:      5  MC Interrupt Disable 1             */
    uint8_t  :2;               /*!< bit:  6.. 7  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  struct {
    uint8_t  :4;               /*!< bit:  0.. 3  Reserved                           */
    uint8_t  MC:2;             /*!< bit:  4.. 5  MC Interrupt Disable x             */
    uint8_t  :2;               /*!< bit:  6.. 7  Reserved                           */
  } vec;                       /*!< Structure used for vec  access                  */
  uint8_t reg;                 /*!< Type      used for register access              */
} TC_INTENCLR_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- TC_INTENSET : (TC Offset: 0x09) (R/W 8) Interrupt Enable Set -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint8_t  OVF:1;            /*!< bit:      0  OVF Interrupt Enable               */
    uint8_t  ERR:1;            /*!< bit:      1  ERR Interrupt Enable               */
    uint8_t  :2;               /*!< bit:  2.. 3  Reserved                           */
    uint8_t  MC0:1;            /*!< bit:      4  MC Interrupt Enable 0              */
    uint8_t  MC1:1;            /*!< bit:      5  MC Interrupt Enable 1              */
    uint8_t  :2;               /*!< bit:  6.. 7  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  struct {
    uint8_t  :4;               /*!< bit:  0.. 3  Reserved                           */
    uint8_t  MC:2;             /*!< bit:  4.. 5  MC Interrupt Enable x              */
    uint8_t  :2;               /*!< bit:  6.. 7  Reserved                           */
  } vec;                       /*!< Structure used for vec  access                  */
  uint8_t reg;                 /*!< Type      used for register access              */
} TC_INTENSET_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- TC_INTFLAG : (TC Offset: 0x0A) (R/W 8) Interrupt Flag Status and Clear -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { // __I to avoid read-modify-write on write-to-clear register
  struct {
    __I uint8_t  OVF:1;            /*!< bit:      0  OVF Interrupt Flag                 */
    __I uint8_t  ERR:1;            /*!< bit:      1  ERR Interrupt Flag                 */
    __I uint8_t  :2;               /*!< bit:  2.. 3  Reserved                           */
    __I uint8_t  MC0:1;            /*!< bit:      4  MC Interrupt Flag 0                */
    __I uint8_t  MC1:1;            /*!< bit:      5  MC Interrupt Flag 1                */
    __I uint8_t  :2;               /*!< bit:  6.. 7  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  struct {
    __I uint8_t  :4;               /*!< bit:  0.. 3  Reserved                           */
    __I uint8_t  MC:2;             /*!< bit:  4.. 5  MC Interrupt Flag x                */
    __I uint8_t  :2;               /*!< bit:  6.. 7  Reserved                           */
  } vec;                       /*!< Structure used for vec  access                  */
  uint8_t reg;                 /*!< Type      used for register access              */
} TC_INTFLAG_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- TC_STATUS : (TC Offset: 0x0B) (R/W 8) Status -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint8_t  STOP:1;           /*!< bit:      0  Stop Status Flag                   */
    uint8_t  SLAVE:1;          /*!< bit:      1  Slave Status Flag                  */
    uint8_t  :1;               /*!< bit:      2  Reserved                           */
    uint8_t  PERBUFV:1;        /*!< bit:      3  Synchronization Busy Status        */
    uint8_t  CCBUFV0:1;        /*!< bit:      4  Compare channel buffer 0 valid     */
    uint8_t  CCBUFV1:1;        /*!< bit:      5  Compare channel buffer 1 valid     */
    uint8_t  :2;               /*!< bit:  6.. 7  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  struct {
    uint8_t  :4;               /*!< bit:  0.. 3  Reserved                           */
    uint8_t  CCBUFV:2;         /*!< bit:  4.. 5  Compare channel buffer x valid     */
    uint8_t  :2;               /*!< bit:  6.. 7  Reserved                           */
  } vec;                       /*!< Structure used for vec  access                  */
  uint8_t reg;                 /*!< Type      used for register access              */
} TC_STATUS_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- TC_WAVE : (TC Offset: 0x0C) (R/W 8) Waveform Generation Control -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint8_t  WAVEGEN:2;        /*!< bit:  0.. 1  Waveform Generation Mode           */
    uint8_t  :6;               /*!< bit:  2.. 7  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint8_t reg;                 /*!< Type      used for register access              */
} TC_WAVE_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- TC_DRVCTRL : (TC Offset: 0x0D) (R/W 8) Control C -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint8_t  INVEN0:1;         /*!< bit:      0  Output Waveform Invert Enable 0    */
    uint8_t  INVEN1:1;         /*!< bit:      1  Output Waveform Invert Enable 1    */
    uint8_t  :6;               /*!< bit:  2.. 7  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  struct {
    uint8_t  INVEN:2;          /*!< bit:  0.. 1  Output Waveform Invert Enable x    */
    uint8_t  :6;               /*!< bit:  2.. 7  Reserved                           */
  } vec;                       /*!< Structure used for vec  access                  */
  uint8_t reg;                 /*!< Type      used for register access              */
} TC_DRVCTRL_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- TC_DBGCTRL : (TC Offset: 0x0F) (R/W 8) Debug Control -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint8_t  DBGRUN:1;         /*!< bit:      0  Run During Debug                   */
    uint8_t  :7;               /*!< bit:  1.. 7  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint8_t reg;                 /*!< Type      used for register access              */
} TC_DBGCTRL_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- TC_SYNCBUSY : (TC Offset: 0x10) ( R/ 32) Synchronization Status -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t SWRST:1;          /*!< bit:      0  swrst                              */
    uint32_t ENABLE:1;         /*!< bit:      1  enable                             */
    uint32_t CTRLB:1;          /*!< bit:      2  CTRLB                              */
    uint32_t STATUS:1;         /*!< bit:      3  STATUS                             */
    uint32_t COUNT:1;          /*!< bit:      4  Counter                            */
    uint32_t PER:1;            /*!< bit:      5  Period                             */
    uint32_t CC0:1;            /*!< bit:      6  Compare Channel 0                  */
    uint32_t CC1:1;            /*!< bit:      7  Compare Channel 1                  */
    uint32_t :24;              /*!< bit:  8..31  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  struct {
    uint32_t :6;               /*!< bit:  0.. 5  Reserved                           */
    uint32_t CC:2;             /*!< bit:  6.. 7  Compare Channel x                  */
    uint32_t :24;              /*!< bit:  8..31  Reserved                           */
  } vec;                       /*!< Structure used for vec  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} TC_SYNCBUSY_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- TC_COUNT8_COUNT : (TC Offset: 0x14) (R/W 8) COUNT8 Count -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint8_t  COUNT:8;          /*!< bit:  0.. 7  Counter Value                      */
  } bit;                       /*!< Structure used for bit  access                  */
  uint8_t reg;                 /*!< Type      used for register access              */
} TC_COUNT8_COUNT_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- TC_COUNT16_COUNT : (TC Offset: 0x14) (R/W 16) COUNT16 Count -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint16_t COUNT:16;         /*!< bit:  0..15  Counter Value                      */
  } bit;                       /*!< Structure used for bit  access                  */
  uint16_t reg;                /*!< Type      used for register access              */
} TC_COUNT16_COUNT_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- TC_COUNT32_COUNT : (TC Offset: 0x14) (R/W 32) COUNT32 Count -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t COUNT:32;         /*!< bit:  0..31  Counter Value                      */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} TC_COUNT32_COUNT_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- TC_COUNT8_PER : (TC Offset: 0x1B) (R/W 8) COUNT8 Period -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint8_t  PER:8;            /*!< bit:  0.. 7  Period Value                       */
  } bit;                       /*!< Structure used for bit  access                  */
  uint8_t reg;                 /*!< Type      used for register access              */
} TC_COUNT8_PER_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- TC_COUNT8_CC : (TC Offset: 0x1C) (R/W 8) COUNT8 Compare and Capture -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint8_t  CC:8;             /*!< bit:  0.. 7  Counter/Compare Value              */
  } bit;                       /*!< Structure used for bit  access                  */
  uint8_t reg;                 /*!< Type      used for register access              */
} TC_COUNT8_CC_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- TC_COUNT16_CC : (TC Offset: 0x1C) (R/W 16) COUNT16 Compare and Capture -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint16_t CC:16;            /*!< bit:  0..15  Counter/Compare Value              */
  } bit;                       /*!< Structure used for bit  access                  */
  uint16_t reg;                /*!< Type      used for register access              */
} TC_COUNT16_CC_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- TC_COUNT32_CC : (TC Offset: 0x1C) (R/W 32) COUNT32 Compare and Capture -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t CC:32;            /*!< bit:  0..31  Counter/Compare Value              */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} TC_COUNT32_CC_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- TC_COUNT8_PERBUF : (TC Offset: 0x2F) (R/W 8) COUNT8 Period Buffer -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint8_t  PERBUF:8;         /*!< bit:  0.. 7  Period Buffer Value                */
  } bit;                       /*!< Structure used for bit  access                  */
  uint8_t reg;                 /*!< Type      used for register access              */
} TC_COUNT8_PERBUF_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- TC_COUNT8_CCBUF : (TC Offset: 0x30) (R/W 8) COUNT8 Compare and Capture Buffer -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint8_t  CCBUF:8;          /*!< bit:  0.. 7  Counter/Compare Buffer Value       */
  } bit;                       /*!< Structure used for bit  access                  */
  uint8_t reg;                 /*!< Type      used for register access              */
} TC_COUNT8_CCBUF_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- TC_COUNT16_CCBUF : (TC Offset: 0x30) (R/W 16) COUNT16 Compare and Capture Buffer -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint16_t CCBUF:16;         /*!< bit:  0..15  Counter/Compare Buffer Value       */
  } bit;                       /*!< Structure used for bit  access                  */
  uint16_t reg;                /*!< Type      used for register access              */
} TC_COUNT16_CCBUF_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- TC_COUNT32_CCBUF : (TC Offset: 0x30) (R/W 32) COUNT32 Compare and Capture Buffer -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t CCBUF:32;         /*!< bit:  0..31  Counter/Compare Buffer Value       */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} TC_COUNT32_CCBUF_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/** \brief TC_COUNT8 hardware registers */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef struct { /* 8-bit Counter Mode */
  __IO TC_CTRLA_Type             CTRLA;       /**< \brief Offset: 0x00 (R/W 32) Control A */
  __IO TC_CTRLBCLR_Type          CTRLBCLR;    /**< \brief Offset: 0x04 (R/W  8) Control B Clear */
  __IO TC_CTRLBSET_Type          CTRLBSET;    /**< \brief Offset: 0x05 (R/W  8) Control B Set */
  __IO TC_EVCTRL_Type            EVCTRL;      /**< \brief Offset: 0x06 (R/W 16) Event Control */
  __IO TC_INTENCLR_Type          INTENCLR;    /**< \brief Offset: 0x08 (R/W  8) Interrupt Enable Clear */
  __IO TC_INTENSET_Type          INTENSET;    /**< \brief Offset: 0x09 (R/W  8) Interrupt Enable Set */
  __IO TC_INTFLAG_Type           INTFLAG;     /**< \brief Offset: 0x0A (R/W  8) Interrupt Flag Status and Clear */
  __IO TC_STATUS_Type            STATUS;      /**< \brief Offset: 0x0B (R/W  8) Status */
  __IO TC_WAVE_Type              WAVE;        /**< \brief Offset: 0x0C (R/W  8) Waveform Generation Control */
  __IO TC_DRVCTRL_Type           DRVCTRL;     /**< \brief Offset: 0x0D (R/W  8) Control C */
       RoReg8                    Reserved1[0x1];
  __IO TC_DBGCTRL_Type           DBGCTRL;     /**< \brief Offset: 0x0F (R/W  8) Debug Control */
  __I  TC_SYNCBUSY_Type          SYNCBUSY;    /**< \brief Offset: 0x10 (R/  32) Synchronization Status */
  __IO TC_COUNT8_COUNT_Type      COUNT;       /**< \brief Offset: 0x14 (R/W  8) COUNT8 Count */
       RoReg8                    Reserved2[0x6];
  __IO TC_COUNT8_PER_Type        PER;         /**< \brief Offset: 0x1B (R/W  8) COUNT8 Period */
  __IO TC_COUNT8_CC_Type         CC[2];       /**< \brief Offset: 0x1C (R/W  8) COUNT8 Compare and Capture */
       RoReg8                    Reserved3[0x11];
  __IO TC_COUNT8_PERBUF_Type     PERBUF;      /**< \brief Offset: 0x2F (R/W  8) COUNT8 Period Buffer */
  __IO TC_COUNT8_CCBUF_Type      CCBUF[2];    /**< \brief Offset: 0x30 (R/W  8) COUNT8 Compare and Capture Buffer */
} TcCount8;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/** \brief TC_COUNT16 hardware registers */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef struct { /* 16-bit Counter Mode */
  __IO TC_CTRLA_Type             CTRLA;       /**< \brief Offset: 0x00 (R/W 32) Control A */
  __IO TC_CTRLBCLR_Type          CTRLBCLR;    /**< \brief Offset: 0x04 (R/W  8) Control B Clear */
  __IO TC_CTRLBSET_Type          CTRLBSET;    /**< \brief Offset: 0x05 (R/W  8) Control B Set */
  __IO TC_EVCTRL_Type            EVCTRL;      /**< \brief Offset: 0x06 (R/W 16) Event Control */
  __IO TC_INTENCLR_Type          INTENCLR;    /**< \brief Offset: 0x08 (R/W  8) Interrupt Enable Clear */
  __IO TC_INTENSET_Type          INTENSET;    /**< \brief Offset: 0x09 (R/W  8) Interrupt Enable Set */
  __IO TC_INTFLAG_Type           INTFLAG;     /**< \brief Offset: 0x0A (R/W  8) Interrupt Flag Status and Clear */
  __IO TC_STATUS_Type            STATUS;      /**< \brief Offset: 0x0B (R/W  8) Status */
  __IO TC_WAVE_Type              WAVE;        /**< \brief Offset: 0x0C (R/W  8) Waveform Generation Control */
  __IO TC_DRVCTRL_Type           DRVCTRL;     /**< \brief Offset: 0x0D (R/W  8) Control C */
       RoReg8                    Reserved1[0x1];
  __IO TC_DBGCTRL_Type           DBGCTRL;     /**< \brief Offset: 0x0F (R/W  8) Debug Control */
  __I  TC_SYNCBUSY_Type          SYNCBUSY;    /**< \brief Offset: 0x10 (R/  32) Synchronization Status */
  __IO TC_COUNT16_COUNT_Type     COUNT;       /**< \brief Offset: 0x14 (R/W 16) COUNT16 Count */
       RoReg8                    Reserved2[0x6];
  __IO TC_COUNT16_CC_Type        CC[2];       /**< \brief Offset: 0x1C (R/W 16) COUNT16 Compare and Capture */
       RoReg8                    Reserved3[0x10];
  __IO TC_COUNT16_CCBUF_Type     CCBUF[2];    /**< \brief Offset: 0x30 (R/W 16) COUNT16 Compare and Capture Buffer */
} TcCount16;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/** \brief TC_COUNT32 hardware registers */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef struct { /* 32-bit Counter Mode */
  __IO TC_CTRLA_Type             CTRLA;       /**< \brief Offset: 0x00 (R/W 32) Control A */
  __IO TC_CTRLBCLR_Type          CTRLBCLR;    /**< \brief Offset: 0x04 (R/W  8) Control B Clear */
  __IO TC_CTRLBSET_Type          CTRLBSET;    /**< \brief Offset: 0x05 (R/W  8) Control B Set */
  __IO TC_EVCTRL_Type            EVCTRL;      /**< \brief Offset: 0x06 (R/W 16) Event Control */
  __IO TC_INTENCLR_Type          INTENCLR;    /**< \brief Offset: 0x08 (R/W  8) Interrupt Enable Clear */
  __IO TC_INTENSET_Type          INTENSET;    /**< \brief Offset: 0x09 (R/W  8) Interrupt Enable Set */
  __IO TC_INTFLAG_Type           INTFLAG;     /**< \brief Offset: 0x0A (R/W  8) Interrupt Flag Status and Clear */
  __IO TC_STATUS_Type            STATUS;      /**< \brief Offset: 0x0B (R/W  8) Status */
  __IO TC_WAVE_Type              WAVE;        /**< \brief Offset: 0x0C (R/W  8) Waveform Generation Control */
  __IO TC_DRVCTRL_Type           DRVCTRL;     /**< \brief Offset: 0x0D (R/W  8) Control C */
       RoReg8                    Reserved1[0x1];
  __IO TC_DBGCTRL_Type           DBGCTRL;     /**< \brief Offset: 0x0F (R/W  8) Debug Control */
  __I  TC_SYNCBUSY_Type          SYNCBUSY;    /**< \brief Offset: 0x10 (R/  32) Synchronization Status */
  __IO TC_COUNT32_COUNT_Type     COUNT;       /**< \brief Offset: 0x14 (R/W 32) COUNT32 Count */
       RoReg8                    Reserved2[0x4];
  __IO TC_COUNT32_CC_Type        CC[2];       /**< \brief Offset: 0x1C (R/W 32) COUNT32 Compare and Capture */
       RoReg8                    Reserved3[0xC];
  __IO TC_COUNT32_CCBUF_Type     CCBUF[2];    /**< \brief Offset: 0x30 (R/W 32) COUNT32 Compare and Capture Buffer */
} TcCount32;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
       TcCount8                  COUNT8;      /**< \brief Offset: 0x00 8-bit Counter Mode */
       TcCount16                 COUNT16;     /**< \brief Offset: 0x00 16-bit Counter Mode */
       TcCount32                 COUNT32;     /**< \brief Offset: 0x00 32-bit Counter Mode */
} Tc;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */






#endif /* _MICROCHIP_PIC32CXSG_TC_COMPONENT_FIXUP_H_ */
