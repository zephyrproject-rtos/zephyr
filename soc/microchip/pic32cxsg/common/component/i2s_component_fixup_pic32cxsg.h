/*
 * Copyright (c) 2024 Microchip
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _MICROCHIP_PIC32CXSG_I2S_COMPONENT_FIXUP_H_
#define _MICROCHIP_PIC32CXSG_I2S_COMPONENT_FIXUP_H_

/* -------- I2S_CTRLA : (I2S Offset: 0x00) (R/W 8) Control A -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint8_t  SWRST:1;          /*!< bit:      0  Software Reset                     */
    uint8_t  ENABLE:1;         /*!< bit:      1  Enable                             */
    uint8_t  CKEN0:1;          /*!< bit:      2  Clock Unit 0 Enable                */
    uint8_t  CKEN1:1;          /*!< bit:      3  Clock Unit 1 Enable                */
    uint8_t  TXEN:1;           /*!< bit:      4  Tx Serializer Enable               */
    uint8_t  RXEN:1;           /*!< bit:      5  Rx Serializer Enable               */
    uint8_t  :2;               /*!< bit:  6.. 7  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  struct {
    uint8_t  :2;               /*!< bit:  0.. 1  Reserved                           */
    uint8_t  CKEN:2;           /*!< bit:  2.. 3  Clock Unit x Enable                */
    uint8_t  :4;               /*!< bit:  4.. 7  Reserved                           */
  } vec;                       /*!< Structure used for vec  access                  */
  uint8_t reg;                 /*!< Type      used for register access              */
} I2S_CTRLA_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- I2S_CLKCTRL : (I2S Offset: 0x04) (R/W 32) Clock Unit n Control -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t SLOTSIZE:2;       /*!< bit:  0.. 1  Slot Size                          */
    uint32_t NBSLOTS:3;        /*!< bit:  2.. 4  Number of Slots in Frame           */
    uint32_t FSWIDTH:2;        /*!< bit:  5.. 6  Frame Sync Width                   */
    uint32_t BITDELAY:1;       /*!< bit:      7  Data Delay from Frame Sync         */
    uint32_t FSSEL:1;          /*!< bit:      8  Frame Sync Select                  */
    uint32_t FSINV:1;          /*!< bit:      9  Frame Sync Invert                  */
    uint32_t FSOUTINV:1;       /*!< bit:     10  Frame Sync Output Invert           */
    uint32_t SCKSEL:1;         /*!< bit:     11  Serial Clock Select                */
    uint32_t SCKOUTINV:1;      /*!< bit:     12  Serial Clock Output Invert         */
    uint32_t MCKSEL:1;         /*!< bit:     13  Master Clock Select                */
    uint32_t MCKEN:1;          /*!< bit:     14  Master Clock Enable                */
    uint32_t MCKOUTINV:1;      /*!< bit:     15  Master Clock Output Invert         */
    uint32_t MCKDIV:6;         /*!< bit: 16..21  Master Clock Division Factor       */
    uint32_t :2;               /*!< bit: 22..23  Reserved                           */
    uint32_t MCKOUTDIV:6;      /*!< bit: 24..29  Master Clock Output Division Factor */
    uint32_t :2;               /*!< bit: 30..31  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} I2S_CLKCTRL_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- I2S_INTENCLR : (I2S Offset: 0x0C) (R/W 16) Interrupt Enable Clear -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint16_t RXRDY0:1;         /*!< bit:      0  Receive Ready 0 Interrupt Enable   */
    uint16_t RXRDY1:1;         /*!< bit:      1  Receive Ready 1 Interrupt Enable   */
    uint16_t :2;               /*!< bit:  2.. 3  Reserved                           */
    uint16_t RXOR0:1;          /*!< bit:      4  Receive Overrun 0 Interrupt Enable */
    uint16_t RXOR1:1;          /*!< bit:      5  Receive Overrun 1 Interrupt Enable */
    uint16_t :2;               /*!< bit:  6.. 7  Reserved                           */
    uint16_t TXRDY0:1;         /*!< bit:      8  Transmit Ready 0 Interrupt Enable  */
    uint16_t TXRDY1:1;         /*!< bit:      9  Transmit Ready 1 Interrupt Enable  */
    uint16_t :2;               /*!< bit: 10..11  Reserved                           */
    uint16_t TXUR0:1;          /*!< bit:     12  Transmit Underrun 0 Interrupt Enable */
    uint16_t TXUR1:1;          /*!< bit:     13  Transmit Underrun 1 Interrupt Enable */
    uint16_t :2;               /*!< bit: 14..15  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  struct {
    uint16_t RXRDY:2;          /*!< bit:  0.. 1  Receive Ready x Interrupt Enable   */
    uint16_t :2;               /*!< bit:  2.. 3  Reserved                           */
    uint16_t RXOR:2;           /*!< bit:  4.. 5  Receive Overrun x Interrupt Enable */
    uint16_t :2;               /*!< bit:  6.. 7  Reserved                           */
    uint16_t TXRDY:2;          /*!< bit:  8.. 9  Transmit Ready x Interrupt Enable  */
    uint16_t :2;               /*!< bit: 10..11  Reserved                           */
    uint16_t TXUR:2;           /*!< bit: 12..13  Transmit Underrun x Interrupt Enable */
    uint16_t :2;               /*!< bit: 14..15  Reserved                           */
  } vec;                       /*!< Structure used for vec  access                  */
  uint16_t reg;                /*!< Type      used for register access              */
} I2S_INTENCLR_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- I2S_INTENSET : (I2S Offset: 0x10) (R/W 16) Interrupt Enable Set -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint16_t RXRDY0:1;         /*!< bit:      0  Receive Ready 0 Interrupt Enable   */
    uint16_t RXRDY1:1;         /*!< bit:      1  Receive Ready 1 Interrupt Enable   */
    uint16_t :2;               /*!< bit:  2.. 3  Reserved                           */
    uint16_t RXOR0:1;          /*!< bit:      4  Receive Overrun 0 Interrupt Enable */
    uint16_t RXOR1:1;          /*!< bit:      5  Receive Overrun 1 Interrupt Enable */
    uint16_t :2;               /*!< bit:  6.. 7  Reserved                           */
    uint16_t TXRDY0:1;         /*!< bit:      8  Transmit Ready 0 Interrupt Enable  */
    uint16_t TXRDY1:1;         /*!< bit:      9  Transmit Ready 1 Interrupt Enable  */
    uint16_t :2;               /*!< bit: 10..11  Reserved                           */
    uint16_t TXUR0:1;          /*!< bit:     12  Transmit Underrun 0 Interrupt Enable */
    uint16_t TXUR1:1;          /*!< bit:     13  Transmit Underrun 1 Interrupt Enable */
    uint16_t :2;               /*!< bit: 14..15  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  struct {
    uint16_t RXRDY:2;          /*!< bit:  0.. 1  Receive Ready x Interrupt Enable   */
    uint16_t :2;               /*!< bit:  2.. 3  Reserved                           */
    uint16_t RXOR:2;           /*!< bit:  4.. 5  Receive Overrun x Interrupt Enable */
    uint16_t :2;               /*!< bit:  6.. 7  Reserved                           */
    uint16_t TXRDY:2;          /*!< bit:  8.. 9  Transmit Ready x Interrupt Enable  */
    uint16_t :2;               /*!< bit: 10..11  Reserved                           */
    uint16_t TXUR:2;           /*!< bit: 12..13  Transmit Underrun x Interrupt Enable */
    uint16_t :2;               /*!< bit: 14..15  Reserved                           */
  } vec;                       /*!< Structure used for vec  access                  */
  uint16_t reg;                /*!< Type      used for register access              */
} I2S_INTENSET_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- I2S_INTFLAG : (I2S Offset: 0x14) (R/W 16) Interrupt Flag Status and Clear -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { // __I to avoid read-modify-write on write-to-clear register
  struct {
    __I uint16_t RXRDY0:1;         /*!< bit:      0  Receive Ready 0                    */
    __I uint16_t RXRDY1:1;         /*!< bit:      1  Receive Ready 1                    */
    __I uint16_t :2;               /*!< bit:  2.. 3  Reserved                           */
    __I uint16_t RXOR0:1;          /*!< bit:      4  Receive Overrun 0                  */
    __I uint16_t RXOR1:1;          /*!< bit:      5  Receive Overrun 1                  */
    __I uint16_t :2;               /*!< bit:  6.. 7  Reserved                           */
    __I uint16_t TXRDY0:1;         /*!< bit:      8  Transmit Ready 0                   */
    __I uint16_t TXRDY1:1;         /*!< bit:      9  Transmit Ready 1                   */
    __I uint16_t :2;               /*!< bit: 10..11  Reserved                           */
    __I uint16_t TXUR0:1;          /*!< bit:     12  Transmit Underrun 0                */
    __I uint16_t TXUR1:1;          /*!< bit:     13  Transmit Underrun 1                */
    __I uint16_t :2;               /*!< bit: 14..15  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  struct {
    __I uint16_t RXRDY:2;          /*!< bit:  0.. 1  Receive Ready x                    */
    __I uint16_t :2;               /*!< bit:  2.. 3  Reserved                           */
    __I uint16_t RXOR:2;           /*!< bit:  4.. 5  Receive Overrun x                  */
    __I uint16_t :2;               /*!< bit:  6.. 7  Reserved                           */
    __I uint16_t TXRDY:2;          /*!< bit:  8.. 9  Transmit Ready x                   */
    __I uint16_t :2;               /*!< bit: 10..11  Reserved                           */
    __I uint16_t TXUR:2;           /*!< bit: 12..13  Transmit Underrun x                */
    __I uint16_t :2;               /*!< bit: 14..15  Reserved                           */
  } vec;                       /*!< Structure used for vec  access                  */
  uint16_t reg;                /*!< Type      used for register access              */
} I2S_INTFLAG_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- I2S_SYNCBUSY : (I2S Offset: 0x18) ( R/ 16) Synchronization Status -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint16_t SWRST:1;          /*!< bit:      0  Software Reset Synchronization Status */
    uint16_t ENABLE:1;         /*!< bit:      1  Enable Synchronization Status      */
    uint16_t CKEN0:1;          /*!< bit:      2  Clock Unit 0 Enable Synchronization Status */
    uint16_t CKEN1:1;          /*!< bit:      3  Clock Unit 1 Enable Synchronization Status */
    uint16_t TXEN:1;           /*!< bit:      4  Tx Serializer Enable Synchronization Status */
    uint16_t RXEN:1;           /*!< bit:      5  Rx Serializer Enable Synchronization Status */
    uint16_t :2;               /*!< bit:  6.. 7  Reserved                           */
    uint16_t TXDATA:1;         /*!< bit:      8  Tx Data Synchronization Status     */
    uint16_t RXDATA:1;         /*!< bit:      9  Rx Data Synchronization Status     */
    uint16_t :6;               /*!< bit: 10..15  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  struct {
    uint16_t :2;               /*!< bit:  0.. 1  Reserved                           */
    uint16_t CKEN:2;           /*!< bit:  2.. 3  Clock Unit x Enable Synchronization Status */
    uint16_t :12;              /*!< bit:  4..15  Reserved                           */
  } vec;                       /*!< Structure used for vec  access                  */
  uint16_t reg;                /*!< Type      used for register access              */
} I2S_SYNCBUSY_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- I2S_TXCTRL : (I2S Offset: 0x20) (R/W 32) Tx Serializer Control -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t :2;               /*!< bit:  0.. 1  Reserved                           */
    uint32_t TXDEFAULT:2;      /*!< bit:  2.. 3  Line Default Line when Slot Disabled */
    uint32_t TXSAME:1;         /*!< bit:      4  Transmit Data when Underrun        */
    uint32_t :2;               /*!< bit:  5.. 6  Reserved                           */
    uint32_t SLOTADJ:1;        /*!< bit:      7  Data Slot Formatting Adjust        */
    uint32_t DATASIZE:3;       /*!< bit:  8..10  Data Word Size                     */
    uint32_t :1;               /*!< bit:     11  Reserved                           */
    uint32_t WORDADJ:1;        /*!< bit:     12  Data Word Formatting Adjust        */
    uint32_t EXTEND:2;         /*!< bit: 13..14  Data Formatting Bit Extension      */
    uint32_t BITREV:1;         /*!< bit:     15  Data Formatting Bit Reverse        */
    uint32_t SLOTDIS0:1;       /*!< bit:     16  Slot 0 Disabled for this Serializer */
    uint32_t SLOTDIS1:1;       /*!< bit:     17  Slot 1 Disabled for this Serializer */
    uint32_t SLOTDIS2:1;       /*!< bit:     18  Slot 2 Disabled for this Serializer */
    uint32_t SLOTDIS3:1;       /*!< bit:     19  Slot 3 Disabled for this Serializer */
    uint32_t SLOTDIS4:1;       /*!< bit:     20  Slot 4 Disabled for this Serializer */
    uint32_t SLOTDIS5:1;       /*!< bit:     21  Slot 5 Disabled for this Serializer */
    uint32_t SLOTDIS6:1;       /*!< bit:     22  Slot 6 Disabled for this Serializer */
    uint32_t SLOTDIS7:1;       /*!< bit:     23  Slot 7 Disabled for this Serializer */
    uint32_t MONO:1;           /*!< bit:     24  Mono Mode                          */
    uint32_t DMA:1;            /*!< bit:     25  Single or Multiple DMA Channels    */
    uint32_t :6;               /*!< bit: 26..31  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  struct {
    uint32_t :16;              /*!< bit:  0..15  Reserved                           */
    uint32_t SLOTDIS:8;        /*!< bit: 16..23  Slot x Disabled for this Serializer */
    uint32_t :8;               /*!< bit: 24..31  Reserved                           */
  } vec;                       /*!< Structure used for vec  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} I2S_TXCTRL_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- I2S_RXCTRL : (I2S Offset: 0x24) (R/W 32) Rx Serializer Control -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t SERMODE:2;        /*!< bit:  0.. 1  Serializer Mode                    */
    uint32_t :3;               /*!< bit:  2.. 4  Reserved                           */
    uint32_t CLKSEL:1;         /*!< bit:      5  Clock Unit Selection               */
    uint32_t :1;               /*!< bit:      6  Reserved                           */
    uint32_t SLOTADJ:1;        /*!< bit:      7  Data Slot Formatting Adjust        */
    uint32_t DATASIZE:3;       /*!< bit:  8..10  Data Word Size                     */
    uint32_t :1;               /*!< bit:     11  Reserved                           */
    uint32_t WORDADJ:1;        /*!< bit:     12  Data Word Formatting Adjust        */
    uint32_t EXTEND:2;         /*!< bit: 13..14  Data Formatting Bit Extension      */
    uint32_t BITREV:1;         /*!< bit:     15  Data Formatting Bit Reverse        */
    uint32_t SLOTDIS0:1;       /*!< bit:     16  Slot 0 Disabled for this Serializer */
    uint32_t SLOTDIS1:1;       /*!< bit:     17  Slot 1 Disabled for this Serializer */
    uint32_t SLOTDIS2:1;       /*!< bit:     18  Slot 2 Disabled for this Serializer */
    uint32_t SLOTDIS3:1;       /*!< bit:     19  Slot 3 Disabled for this Serializer */
    uint32_t SLOTDIS4:1;       /*!< bit:     20  Slot 4 Disabled for this Serializer */
    uint32_t SLOTDIS5:1;       /*!< bit:     21  Slot 5 Disabled for this Serializer */
    uint32_t SLOTDIS6:1;       /*!< bit:     22  Slot 6 Disabled for this Serializer */
    uint32_t SLOTDIS7:1;       /*!< bit:     23  Slot 7 Disabled for this Serializer */
    uint32_t MONO:1;           /*!< bit:     24  Mono Mode                          */
    uint32_t DMA:1;            /*!< bit:     25  Single or Multiple DMA Channels    */
    uint32_t RXLOOP:1;         /*!< bit:     26  Loop-back Test Mode                */
    uint32_t :5;               /*!< bit: 27..31  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  struct {
    uint32_t :16;              /*!< bit:  0..15  Reserved                           */
    uint32_t SLOTDIS:8;        /*!< bit: 16..23  Slot x Disabled for this Serializer */
    uint32_t :8;               /*!< bit: 24..31  Reserved                           */
  } vec;                       /*!< Structure used for vec  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} I2S_RXCTRL_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- I2S_TXDATA : (I2S Offset: 0x30) ( /W 32) Tx Data -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t DATA:32;          /*!< bit:  0..31  Sample Data                        */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} I2S_TXDATA_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- I2S_RXDATA : (I2S Offset: 0x34) ( R/ 32) Rx Data -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t DATA:32;          /*!< bit:  0..31  Sample Data                        */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} I2S_RXDATA_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/** \brief I2S hardware registers */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef struct {
  __IO I2S_CTRLA_Type            CTRLA;       /**< \brief Offset: 0x00 (R/W  8) Control A */
       RoReg8                    Reserved1[0x3];
  __IO I2S_CLKCTRL_Type          CLKCTRL[2];  /**< \brief Offset: 0x04 (R/W 32) Clock Unit n Control */
  __IO I2S_INTENCLR_Type         INTENCLR;    /**< \brief Offset: 0x0C (R/W 16) Interrupt Enable Clear */
       RoReg8                    Reserved2[0x2];
  __IO I2S_INTENSET_Type         INTENSET;    /**< \brief Offset: 0x10 (R/W 16) Interrupt Enable Set */
       RoReg8                    Reserved3[0x2];
  __IO I2S_INTFLAG_Type          INTFLAG;     /**< \brief Offset: 0x14 (R/W 16) Interrupt Flag Status and Clear */
       RoReg8                    Reserved4[0x2];
  __I  I2S_SYNCBUSY_Type         SYNCBUSY;    /**< \brief Offset: 0x18 (R/  16) Synchronization Status */
       RoReg8                    Reserved5[0x6];
  __IO I2S_TXCTRL_Type           TXCTRL;      /**< \brief Offset: 0x20 (R/W 32) Tx Serializer Control */
  __IO I2S_RXCTRL_Type           RXCTRL;      /**< \brief Offset: 0x24 (R/W 32) Rx Serializer Control */
       RoReg8                    Reserved6[0x8];
  __O  I2S_TXDATA_Type           TXDATA;      /**< \brief Offset: 0x30 ( /W 32) Tx Data */
  __I  I2S_RXDATA_Type           RXDATA;      /**< \brief Offset: 0x34 (R/  32) Rx Data */
} I2s;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

#endif /* _MICROCHIP_PIC32CXSG_I2S_COMPONENT_FIXUP_H_ */
