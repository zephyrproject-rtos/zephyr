/*
 * Copyright (c) 2024 Microchip
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _MICROCHIP_PIC32CXSG_EVSYS_COMPONENT_FIXUP_H_
#define _MICROCHIP_PIC32CXSG_EVSYS_COMPONENT_FIXUP_H_

/* -------- EVSYS_CHANNEL : (EVSYS Offset: 0x00) (R/W 32) Channel n Control -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t EVGEN:7;          /*!< bit:  0.. 6  Event Generator Selection          */
    uint32_t :1;               /*!< bit:      7  Reserved                           */
    uint32_t PATH:2;           /*!< bit:  8.. 9  Path Selection                     */
    uint32_t EDGSEL:2;         /*!< bit: 10..11  Edge Detection Selection           */
    uint32_t :2;               /*!< bit: 12..13  Reserved                           */
    uint32_t RUNSTDBY:1;       /*!< bit:     14  Run in standby                     */
    uint32_t ONDEMAND:1;       /*!< bit:     15  Generic Clock On Demand            */
    uint32_t :16;              /*!< bit: 16..31  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} EVSYS_CHANNEL_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- EVSYS_CHINTENCLR : (EVSYS Offset: 0x04) (R/W 8) Channel n Interrupt Enable Clear -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint8_t  OVR:1;            /*!< bit:      0  Channel Overrun Interrupt Disable  */
    uint8_t  EVD:1;            /*!< bit:      1  Channel Event Detected Interrupt Disable */
    uint8_t  :6;               /*!< bit:  2.. 7  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint8_t reg;                 /*!< Type      used for register access              */
} EVSYS_CHINTENCLR_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- EVSYS_CHINTENSET : (EVSYS Offset: 0x05) (R/W 8) Channel n Interrupt Enable Set -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint8_t  OVR:1;            /*!< bit:      0  Channel Overrun Interrupt Enable   */
    uint8_t  EVD:1;            /*!< bit:      1  Channel Event Detected Interrupt Enable */
    uint8_t  :6;               /*!< bit:  2.. 7  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint8_t reg;                 /*!< Type      used for register access              */
} EVSYS_CHINTENSET_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- EVSYS_CHINTFLAG : (EVSYS Offset: 0x06) (R/W 8) Channel n Interrupt Flag Status and Clear -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { // __I to avoid read-modify-write on write-to-clear register
  struct {
    __I uint8_t  OVR:1;            /*!< bit:      0  Channel Overrun                    */
    __I uint8_t  EVD:1;            /*!< bit:      1  Channel Event Detected             */
    __I uint8_t  :6;               /*!< bit:  2.. 7  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint8_t reg;                 /*!< Type      used for register access              */
} EVSYS_CHINTFLAG_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- EVSYS_CHSTATUS : (EVSYS Offset: 0x07) ( R/ 8) Channel n Status -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint8_t  RDYUSR:1;         /*!< bit:      0  Ready User                         */
    uint8_t  BUSYCH:1;         /*!< bit:      1  Busy Channel                       */
    uint8_t  :6;               /*!< bit:  2.. 7  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint8_t reg;                 /*!< Type      used for register access              */
} EVSYS_CHSTATUS_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- EVSYS_CTRLA : (EVSYS Offset: 0x00) (R/W 8) Control -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint8_t  SWRST:1;          /*!< bit:      0  Software Reset                     */
    uint8_t  :7;               /*!< bit:  1.. 7  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint8_t reg;                 /*!< Type      used for register access              */
} EVSYS_CTRLA_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- EVSYS_SWEVT : (EVSYS Offset: 0x04) ( /W 32) Software Event -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t CHANNEL0:1;       /*!< bit:      0  Channel 0 Software Selection       */
    uint32_t CHANNEL1:1;       /*!< bit:      1  Channel 1 Software Selection       */
    uint32_t CHANNEL2:1;       /*!< bit:      2  Channel 2 Software Selection       */
    uint32_t CHANNEL3:1;       /*!< bit:      3  Channel 3 Software Selection       */
    uint32_t CHANNEL4:1;       /*!< bit:      4  Channel 4 Software Selection       */
    uint32_t CHANNEL5:1;       /*!< bit:      5  Channel 5 Software Selection       */
    uint32_t CHANNEL6:1;       /*!< bit:      6  Channel 6 Software Selection       */
    uint32_t CHANNEL7:1;       /*!< bit:      7  Channel 7 Software Selection       */
    uint32_t CHANNEL8:1;       /*!< bit:      8  Channel 8 Software Selection       */
    uint32_t CHANNEL9:1;       /*!< bit:      9  Channel 9 Software Selection       */
    uint32_t CHANNEL10:1;      /*!< bit:     10  Channel 10 Software Selection      */
    uint32_t CHANNEL11:1;      /*!< bit:     11  Channel 11 Software Selection      */
    uint32_t CHANNEL12:1;      /*!< bit:     12  Channel 12 Software Selection      */
    uint32_t CHANNEL13:1;      /*!< bit:     13  Channel 13 Software Selection      */
    uint32_t CHANNEL14:1;      /*!< bit:     14  Channel 14 Software Selection      */
    uint32_t CHANNEL15:1;      /*!< bit:     15  Channel 15 Software Selection      */
    uint32_t CHANNEL16:1;      /*!< bit:     16  Channel 16 Software Selection      */
    uint32_t CHANNEL17:1;      /*!< bit:     17  Channel 17 Software Selection      */
    uint32_t CHANNEL18:1;      /*!< bit:     18  Channel 18 Software Selection      */
    uint32_t CHANNEL19:1;      /*!< bit:     19  Channel 19 Software Selection      */
    uint32_t CHANNEL20:1;      /*!< bit:     20  Channel 20 Software Selection      */
    uint32_t CHANNEL21:1;      /*!< bit:     21  Channel 21 Software Selection      */
    uint32_t CHANNEL22:1;      /*!< bit:     22  Channel 22 Software Selection      */
    uint32_t CHANNEL23:1;      /*!< bit:     23  Channel 23 Software Selection      */
    uint32_t CHANNEL24:1;      /*!< bit:     24  Channel 24 Software Selection      */
    uint32_t CHANNEL25:1;      /*!< bit:     25  Channel 25 Software Selection      */
    uint32_t CHANNEL26:1;      /*!< bit:     26  Channel 26 Software Selection      */
    uint32_t CHANNEL27:1;      /*!< bit:     27  Channel 27 Software Selection      */
    uint32_t CHANNEL28:1;      /*!< bit:     28  Channel 28 Software Selection      */
    uint32_t CHANNEL29:1;      /*!< bit:     29  Channel 29 Software Selection      */
    uint32_t CHANNEL30:1;      /*!< bit:     30  Channel 30 Software Selection      */
    uint32_t CHANNEL31:1;      /*!< bit:     31  Channel 31 Software Selection      */
  } bit;                       /*!< Structure used for bit  access                  */
  struct {
    uint32_t CHANNEL:32;       /*!< bit:  0..31  Channel x Software Selection       */
  } vec;                       /*!< Structure used for vec  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} EVSYS_SWEVT_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- EVSYS_PRICTRL : (EVSYS Offset: 0x08) (R/W 8) Priority Control -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint8_t  PRI:4;            /*!< bit:  0.. 3  Channel Priority Number            */
    uint8_t  :3;               /*!< bit:  4.. 6  Reserved                           */
    uint8_t  RREN:1;           /*!< bit:      7  Round-Robin Scheduling Enable      */
  } bit;                       /*!< Structure used for bit  access                  */
  uint8_t reg;                 /*!< Type      used for register access              */
} EVSYS_PRICTRL_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- EVSYS_INTPEND : (EVSYS Offset: 0x10) (R/W 16) Channel Pending Interrupt -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint16_t ID:4;             /*!< bit:  0.. 3  Channel ID                         */
    uint16_t :4;               /*!< bit:  4.. 7  Reserved                           */
    uint16_t OVR:1;            /*!< bit:      8  Channel Overrun                    */
    uint16_t EVD:1;            /*!< bit:      9  Channel Event Detected             */
    uint16_t :4;               /*!< bit: 10..13  Reserved                           */
    uint16_t READY:1;          /*!< bit:     14  Ready                              */
    uint16_t BUSY:1;           /*!< bit:     15  Busy                               */
  } bit;                       /*!< Structure used for bit  access                  */
  uint16_t reg;                /*!< Type      used for register access              */
} EVSYS_INTPEND_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- EVSYS_INTSTATUS : (EVSYS Offset: 0x14) ( R/ 32) Interrupt Status -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t CHINT0:1;         /*!< bit:      0  Channel 0 Pending Interrupt        */
    uint32_t CHINT1:1;         /*!< bit:      1  Channel 1 Pending Interrupt        */
    uint32_t CHINT2:1;         /*!< bit:      2  Channel 2 Pending Interrupt        */
    uint32_t CHINT3:1;         /*!< bit:      3  Channel 3 Pending Interrupt        */
    uint32_t CHINT4:1;         /*!< bit:      4  Channel 4 Pending Interrupt        */
    uint32_t CHINT5:1;         /*!< bit:      5  Channel 5 Pending Interrupt        */
    uint32_t CHINT6:1;         /*!< bit:      6  Channel 6 Pending Interrupt        */
    uint32_t CHINT7:1;         /*!< bit:      7  Channel 7 Pending Interrupt        */
    uint32_t CHINT8:1;         /*!< bit:      8  Channel 8 Pending Interrupt        */
    uint32_t CHINT9:1;         /*!< bit:      9  Channel 9 Pending Interrupt        */
    uint32_t CHINT10:1;        /*!< bit:     10  Channel 10 Pending Interrupt       */
    uint32_t CHINT11:1;        /*!< bit:     11  Channel 11 Pending Interrupt       */
    uint32_t :20;              /*!< bit: 12..31  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  struct {
    uint32_t CHINT:12;         /*!< bit:  0..11  Channel x Pending Interrupt        */
    uint32_t :20;              /*!< bit: 12..31  Reserved                           */
  } vec;                       /*!< Structure used for vec  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} EVSYS_INTSTATUS_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- EVSYS_BUSYCH : (EVSYS Offset: 0x18) ( R/ 32) Busy Channels -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t BUSYCH0:1;        /*!< bit:      0  Busy Channel 0                     */
    uint32_t BUSYCH1:1;        /*!< bit:      1  Busy Channel 1                     */
    uint32_t BUSYCH2:1;        /*!< bit:      2  Busy Channel 2                     */
    uint32_t BUSYCH3:1;        /*!< bit:      3  Busy Channel 3                     */
    uint32_t BUSYCH4:1;        /*!< bit:      4  Busy Channel 4                     */
    uint32_t BUSYCH5:1;        /*!< bit:      5  Busy Channel 5                     */
    uint32_t BUSYCH6:1;        /*!< bit:      6  Busy Channel 6                     */
    uint32_t BUSYCH7:1;        /*!< bit:      7  Busy Channel 7                     */
    uint32_t BUSYCH8:1;        /*!< bit:      8  Busy Channel 8                     */
    uint32_t BUSYCH9:1;        /*!< bit:      9  Busy Channel 9                     */
    uint32_t BUSYCH10:1;       /*!< bit:     10  Busy Channel 10                    */
    uint32_t BUSYCH11:1;       /*!< bit:     11  Busy Channel 11                    */
    uint32_t :20;              /*!< bit: 12..31  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  struct {
    uint32_t BUSYCH:12;        /*!< bit:  0..11  Busy Channel x                     */
    uint32_t :20;              /*!< bit: 12..31  Reserved                           */
  } vec;                       /*!< Structure used for vec  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} EVSYS_BUSYCH_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- EVSYS_READYUSR : (EVSYS Offset: 0x1C) ( R/ 32) Ready Users -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t READYUSR0:1;      /*!< bit:      0  Ready User for Channel 0           */
    uint32_t READYUSR1:1;      /*!< bit:      1  Ready User for Channel 1           */
    uint32_t READYUSR2:1;      /*!< bit:      2  Ready User for Channel 2           */
    uint32_t READYUSR3:1;      /*!< bit:      3  Ready User for Channel 3           */
    uint32_t READYUSR4:1;      /*!< bit:      4  Ready User for Channel 4           */
    uint32_t READYUSR5:1;      /*!< bit:      5  Ready User for Channel 5           */
    uint32_t READYUSR6:1;      /*!< bit:      6  Ready User for Channel 6           */
    uint32_t READYUSR7:1;      /*!< bit:      7  Ready User for Channel 7           */
    uint32_t READYUSR8:1;      /*!< bit:      8  Ready User for Channel 8           */
    uint32_t READYUSR9:1;      /*!< bit:      9  Ready User for Channel 9           */
    uint32_t READYUSR10:1;     /*!< bit:     10  Ready User for Channel 10          */
    uint32_t READYUSR11:1;     /*!< bit:     11  Ready User for Channel 11          */
    uint32_t :20;              /*!< bit: 12..31  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  struct {
    uint32_t READYUSR:12;      /*!< bit:  0..11  Ready User for Channel x           */
    uint32_t :20;              /*!< bit: 12..31  Reserved                           */
  } vec;                       /*!< Structure used for vec  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} EVSYS_READYUSR_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- EVSYS_USER : (EVSYS Offset: 0x120) (R/W 32) User Multiplexer n -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t CHANNEL:6;        /*!< bit:  0.. 5  Channel Event Selection            */
    uint32_t :26;              /*!< bit:  6..31  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} EVSYS_USER_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/** \brief EvsysChannel hardware registers */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef struct {
  __IO EVSYS_CHANNEL_Type        CHANNEL;     /**< \brief Offset: 0x000 (R/W 32) Channel n Control */
  __IO EVSYS_CHINTENCLR_Type     CHINTENCLR;  /**< \brief Offset: 0x004 (R/W  8) Channel n Interrupt Enable Clear */
  __IO EVSYS_CHINTENSET_Type     CHINTENSET;  /**< \brief Offset: 0x005 (R/W  8) Channel n Interrupt Enable Set */
  __IO EVSYS_CHINTFLAG_Type      CHINTFLAG;   /**< \brief Offset: 0x006 (R/W  8) Channel n Interrupt Flag Status and Clear */
  __I  EVSYS_CHSTATUS_Type       CHSTATUS;    /**< \brief Offset: 0x007 (R/   8) Channel n Status */
} EvsysChannel;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/** \brief EVSYS hardware registers */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef struct {
  __IO EVSYS_CTRLA_Type          CTRLA;       /**< \brief Offset: 0x000 (R/W  8) Control */
       RoReg8                    Reserved1[0x3];
  __O  EVSYS_SWEVT_Type          SWEVT;       /**< \brief Offset: 0x004 ( /W 32) Software Event */
  __IO EVSYS_PRICTRL_Type        PRICTRL;     /**< \brief Offset: 0x008 (R/W  8) Priority Control */
       RoReg8                    Reserved2[0x7];
  __IO EVSYS_INTPEND_Type        INTPEND;     /**< \brief Offset: 0x010 (R/W 16) Channel Pending Interrupt */
       RoReg8                    Reserved3[0x2];
  __I  EVSYS_INTSTATUS_Type      INTSTATUS;   /**< \brief Offset: 0x014 (R/  32) Interrupt Status */
  __I  EVSYS_BUSYCH_Type         BUSYCH;      /**< \brief Offset: 0x018 (R/  32) Busy Channels */
  __I  EVSYS_READYUSR_Type       READYUSR;    /**< \brief Offset: 0x01C (R/  32) Ready Users */
       EvsysChannel              Channel[EVSYS_CHANNEL_NUMBER]; /**< \brief Offset: 0x020 EvsysChannel groups [CHANNELS] */
  __IO EVSYS_USER_Type           USER[67];    /**< \brief Offset: 0x120 (R/W 32) User Multiplexer n */
} Evsys;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

#endif /* _MICROCHIP_PIC32CXSG_EVSYS_COMPONENT_FIXUP_H_ */
