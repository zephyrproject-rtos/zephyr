/*
 * Copyright (c) 2024 Microchip
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _MICROCHIP_PIC32CXSG_DAC_COMPONENT_FIXUP_H_
#define _MICROCHIP_PIC32CXSG_DAC_COMPONENT_FIXUP_H_

/* -------- DAC_CTRLA : (DAC Offset: 0x00) (R/W 8) Control A -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint8_t  SWRST:1;          /*!< bit:      0  Software Reset                     */
    uint8_t  ENABLE:1;         /*!< bit:      1  Enable DAC Controller              */
    uint8_t  :6;               /*!< bit:  2.. 7  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint8_t reg;                 /*!< Type      used for register access              */
} DAC_CTRLA_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- DAC_CTRLB : (DAC Offset: 0x01) (R/W 8) Control B -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint8_t  DIFF:1;           /*!< bit:      0  Differential mode enable           */
    uint8_t  REFSEL:2;         /*!< bit:  1.. 2  Reference Selection for DAC0/1     */
    uint8_t  :5;               /*!< bit:  3.. 7  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint8_t reg;                 /*!< Type      used for register access              */
} DAC_CTRLB_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- DAC_EVCTRL : (DAC Offset: 0x02) (R/W 8) Event Control -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint8_t  STARTEI0:1;       /*!< bit:      0  Start Conversion Event Input DAC 0 */
    uint8_t  STARTEI1:1;       /*!< bit:      1  Start Conversion Event Input DAC 1 */
    uint8_t  EMPTYEO0:1;       /*!< bit:      2  Data Buffer Empty Event Output DAC 0 */
    uint8_t  EMPTYEO1:1;       /*!< bit:      3  Data Buffer Empty Event Output DAC 1 */
    uint8_t  INVEI0:1;         /*!< bit:      4  Enable Invertion of DAC 0 input event */
    uint8_t  INVEI1:1;         /*!< bit:      5  Enable Invertion of DAC 1 input event */
    uint8_t  :1;    		   /*!< bit:      6  Deprecated        */
    uint8_t  :1;    		   /*!< bit:      7  Deprecated        */
  } bit;                       /*!< Structure used for bit  access                  */
  struct {
    uint8_t  STARTEI:2;        /*!< bit:  0.. 1  Start Conversion Event Input DAC x */
    uint8_t  EMPTYEO:2;        /*!< bit:  2.. 3  Data Buffer Empty Event Output DAC x */
    uint8_t  INVEI:2;          /*!< bit:  4.. 5  Enable Invertion of DAC x input event */
    uint8_t  :2;    		   /*!< bit:  6.. 7  Deprecated        */
  } vec;                       /*!< Structure used for vec  access                  */
  uint8_t reg;                 /*!< Type      used for register access              */
} DAC_EVCTRL_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- DAC_INTENCLR : (DAC Offset: 0x04) (R/W 8) Interrupt Enable Clear -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint8_t  UNDERRUN0:1;      /*!< bit:      0  Underrun 0 Interrupt Enable        */
    uint8_t  UNDERRUN1:1;      /*!< bit:      1  Underrun 1 Interrupt Enable        */
    uint8_t  EMPTY0:1;         /*!< bit:      2  Data Buffer 0 Empty Interrupt Enable */
    uint8_t  EMPTY1:1;         /*!< bit:      3  Data Buffer 1 Empty Interrupt Enable */
    uint8_t  :1;    		   /*!< bit:      4  Deprecated    */
    uint8_t  :1;    		   /*!< bit:      5  Deprecated    */
    uint8_t  :1;  			   /*!< bit:      6  Deprecated         */
    uint8_t  :1;  			   /*!< bit:      7  Deprecated         */
  } bit;                       /*!< Structure used for bit  access                  */
  struct {
    uint8_t  UNDERRUN:2;       /*!< bit:  0.. 1  Underrun x Interrupt Enable        */
    uint8_t  EMPTY:2;          /*!< bit:  2.. 3  Data Buffer x Empty Interrupt Enable */
    uint8_t  :2;    		   /*!< bit:  4.. 5  Deprecated    */
    uint8_t  :2;    		   /*!< bit:  6.. 7  Deprecated        */
  } vec;                       /*!< Structure used for vec  access                  */
  uint8_t reg;                 /*!< Type      used for register access              */
} DAC_INTENCLR_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- DAC_INTENSET : (DAC Offset: 0x05) (R/W 8) Interrupt Enable Set -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint8_t  UNDERRUN0:1;      /*!< bit:      0  Underrun 0 Interrupt Enable        */
    uint8_t  UNDERRUN1:1;      /*!< bit:      1  Underrun 1 Interrupt Enable        */
    uint8_t  EMPTY0:1;         /*!< bit:      2  Data Buffer 0 Empty Interrupt Enable */
    uint8_t  EMPTY1:1;         /*!< bit:      3  Data Buffer 1 Empty Interrupt Enable */
    uint8_t  :1;    		   /*!< bit:      4  Deprecated    */
    uint8_t  :1;    		   /*!< bit:      5  Deprecated    */
    uint8_t  :1;    		   /*!< bit:      6  Deprecated         */
    uint8_t  :1;    		   /*!< bit:      7  Deprecated         */
  } bit;                       /*!< Structure used for bit  access                  */
  struct {
    uint8_t  UNDERRUN:2;       /*!< bit:  0.. 1  Underrun x Interrupt Enable        */
    uint8_t  EMPTY:2;          /*!< bit:  2.. 3  Data Buffer x Empty Interrupt Enable */
    uint8_t  :2;    		   /*!< bit:  4.. 5  Deprecated    */
    uint8_t  :2;   			   /*!< bit:  6.. 7  Deprecated         */
  } vec;                       /*!< Structure used for vec  access                  */
  uint8_t reg;                 /*!< Type      used for register access              */
} DAC_INTENSET_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- DAC_INTFLAG : (DAC Offset: 0x06) (R/W 8) Interrupt Flag Status and Clear -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { // __I to avoid read-modify-write on write-to-clear register
  struct {
    __I uint8_t  UNDERRUN0:1;      /*!< bit:      0  Result 0 Underrun                  */
    __I uint8_t  UNDERRUN1:1;      /*!< bit:      1  Result 1 Underrun                  */
    __I uint8_t  EMPTY0:1;         /*!< bit:      2  Data Buffer 0 Empty                */
    __I uint8_t  EMPTY1:1;         /*!< bit:      3  Data Buffer 1 Empty                */
    __I uint8_t  :1;    		   /*!< bit:      4  Deprecated                    */
    __I uint8_t  :1;    		   /*!< bit:      5  Deprecated                     */
    __I uint8_t  :1;   			   /*!< bit:      6  Deprecated                   */
    __I uint8_t  :1;   			   /*!< bit:      7  Deprecated                   */
  } bit;                       /*!< Structure used for bit  access                  */
  struct {
    __I uint8_t  UNDERRUN:2;       /*!< bit:  0.. 1  Result x Underrun                  */
    __I uint8_t  EMPTY:2;          /*!< bit:  2.. 3  Data Buffer x Empty                */
    __I uint8_t  :2;    		   /*!< bit:  4.. 5  Deprecated                     */
    __I uint8_t  :2;    		   /*!< bit:  6.. 7  Deprecated                   */
  } vec;                       /*!< Structure used for vec  access                  */
  uint8_t reg;                 /*!< Type      used for register access              */
} DAC_INTFLAG_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- DAC_STATUS : (DAC Offset: 0x07) ( R/ 8) Status -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint8_t  READY0:1;         /*!< bit:      0  DAC 0 Startup Ready                */
    uint8_t  READY1:1;         /*!< bit:      1  DAC 1 Startup Ready                */
    uint8_t  EOC0:1;           /*!< bit:      2  DAC 0 End of Conversion            */
    uint8_t  EOC1:1;           /*!< bit:      3  DAC 1 End of Conversion            */
    uint8_t  :4;               /*!< bit:  4.. 7  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  struct {
    uint8_t  READY:2;          /*!< bit:  0.. 1  DAC x Startup Ready                */
    uint8_t  EOC:2;            /*!< bit:  2.. 3  DAC x End of Conversion            */
    uint8_t  :4;               /*!< bit:  4.. 7  Reserved                           */
  } vec;                       /*!< Structure used for vec  access                  */
  uint8_t reg;                 /*!< Type      used for register access              */
} DAC_STATUS_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- DAC_SYNCBUSY : (DAC Offset: 0x08) ( R/ 32) Synchronization Busy -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t SWRST:1;          /*!< bit:      0  Software Reset                     */
    uint32_t ENABLE:1;         /*!< bit:      1  DAC Enable Status                  */
    uint32_t DATA0:1;          /*!< bit:      2  Data DAC 0                         */
    uint32_t DATA1:1;          /*!< bit:      3  Data DAC 1                         */
    uint32_t DATABUF0:1;       /*!< bit:      4  Data Buffer DAC 0                  */
    uint32_t DATABUF1:1;       /*!< bit:      5  Data Buffer DAC 1                  */
    uint32_t :26;              /*!< bit:  6..31  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  struct {
    uint32_t :2;               /*!< bit:  0.. 1  Reserved                           */
    uint32_t DATA:2;           /*!< bit:  2.. 3  Data DAC x                         */
    uint32_t DATABUF:2;        /*!< bit:  4.. 5  Data Buffer DAC x                  */
    uint32_t :26;              /*!< bit:  6..31  Reserved                           */
  } vec;                       /*!< Structure used for vec  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} DAC_SYNCBUSY_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- DAC_DACCTRL : (DAC Offset: 0x0C) (R/W 16) DAC n Control -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint16_t LEFTADJ:1;        /*!< bit:      0  Left Adjusted Data                 */
    uint16_t ENABLE:1;         /*!< bit:      1  Enable DAC0                        */
    uint16_t CCTRL:2;          /*!< bit:  2.. 3  Current Control                    */
    uint16_t :1;               /*!< bit:      4  Reserved                           */
    uint16_t FEXT:1;           /*!< bit:      5  Standalone Filter                  */
    uint16_t RUNSTDBY:1;       /*!< bit:      6  Run in Standby                     */
    uint16_t DITHER:1;         /*!< bit:      7  Dithering Mode                     */
    uint16_t REFRESH:4;        /*!< bit:  8..11  Refresh period                     */
    uint16_t :1;               /*!< bit:     12  Reserved                           */
    uint16_t OSR:3;            /*!< bit: 13..15  Sampling Rate                      */
  } bit;                       /*!< Structure used for bit  access                  */
  uint16_t reg;                /*!< Type      used for register access              */
} DAC_DACCTRL_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- DAC_DATA : (DAC Offset: 0x10) ( /W 16) DAC n Data -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint16_t DATA:16;          /*!< bit:  0..15  DAC0 Data                          */
  } bit;                       /*!< Structure used for bit  access                  */
  uint16_t reg;                /*!< Type      used for register access              */
} DAC_DATA_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- DAC_DATABUF : (DAC Offset: 0x14) ( /W 16) DAC n Data Buffer -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint16_t DATABUF:16;       /*!< bit:  0..15  DAC0 Data Buffer                   */
  } bit;                       /*!< Structure used for bit  access                  */
  uint16_t reg;                /*!< Type      used for register access              */
} DAC_DATABUF_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- DAC_DBGCTRL : (DAC Offset: 0x18) (R/W 8) Debug Control -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint8_t  DBGRUN:1;         /*!< bit:      0  Debug Run                          */
    uint8_t  :7;               /*!< bit:  1.. 7  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint8_t reg;                 /*!< Type      used for register access              */
} DAC_DBGCTRL_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/** \brief DAC hardware registers */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef struct {
  __IO DAC_CTRLA_Type            CTRLA;       /**< \brief Offset: 0x00 (R/W  8) Control A */
  __IO DAC_CTRLB_Type            CTRLB;       /**< \brief Offset: 0x01 (R/W  8) Control B */
  __IO DAC_EVCTRL_Type           EVCTRL;      /**< \brief Offset: 0x02 (R/W  8) Event Control */
       RoReg8                    Reserved1[0x1];
  __IO DAC_INTENCLR_Type         INTENCLR;    /**< \brief Offset: 0x04 (R/W  8) Interrupt Enable Clear */
  __IO DAC_INTENSET_Type         INTENSET;    /**< \brief Offset: 0x05 (R/W  8) Interrupt Enable Set */
  __IO DAC_INTFLAG_Type          INTFLAG;     /**< \brief Offset: 0x06 (R/W  8) Interrupt Flag Status and Clear */
  __I  DAC_STATUS_Type           STATUS;      /**< \brief Offset: 0x07 (R/   8) Status */
  __I  DAC_SYNCBUSY_Type         SYNCBUSY;    /**< \brief Offset: 0x08 (R/  32) Synchronization Busy */
  __IO DAC_DACCTRL_Type          DACCTRL[2];  /**< \brief Offset: 0x0C (R/W 16) DAC n Control */
  __O  DAC_DATA_Type             DATA[2];     /**< \brief Offset: 0x10 ( /W 16) DAC n Data */
  __O  DAC_DATABUF_Type          DATABUF[2];  /**< \brief Offset: 0x14 ( /W 16) DAC n Data Buffer */
  __IO DAC_DBGCTRL_Type          DBGCTRL;     /**< \brief Offset: 0x18 (R/W  8) Debug Control */
  /*     RoReg8                    Reserved2[0x3]; - Deprecated */
  /*__I  DAC_RESULT_Type           RESULT[2]; - Deprecated */  /**< \brief Offset: 0x1C (R/  16) Filter Result */
} Dac;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

#endif /* _MICROCHIP_PIC32CXSG_DAC_COMPONENT_FIXUP_H_ */
