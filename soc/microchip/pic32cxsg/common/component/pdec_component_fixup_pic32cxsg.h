/*
 * Copyright (c) 2024 Microchip
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _MICROCHIP_PIC32CXSG_PDEC_COMPONENT_FIXUP_H_
#define _MICROCHIP_PIC32CXSG_PDEC_COMPONENT_FIXUP_H_

/* -------- PDEC_CTRLA : (PDEC Offset: 0x00) (R/W 32) Control A -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t SWRST:1;          /*!< bit:      0  Software Reset                     */
    uint32_t ENABLE:1;         /*!< bit:      1  Enable                             */
    uint32_t MODE:2;           /*!< bit:  2.. 3  Operation Mode                     */
    uint32_t :2;               /*!< bit:  4.. 5  Reserved                           */
    uint32_t RUNSTDBY:1;       /*!< bit:      6  Run in Standby                     */
    uint32_t :1;               /*!< bit:      7  Reserved                           */
    uint32_t CONF:3;           /*!< bit:  8..10  PDEC Configuration                 */
    uint32_t ALOCK:1;          /*!< bit:     11  Auto Lock                          */
    uint32_t :2;               /*!< bit: 12..13  Reserved                           */
    uint32_t SWAP:1;           /*!< bit:     14  PDEC Phase A and B Swap            */
    uint32_t PEREN:1;          /*!< bit:     15  Period Enable                      */
    uint32_t PINEN0:1;         /*!< bit:     16  PDEC Input From Pin 0 Enable       */
    uint32_t PINEN1:1;         /*!< bit:     17  PDEC Input From Pin 1 Enable       */
    uint32_t PINEN2:1;         /*!< bit:     18  PDEC Input From Pin 2 Enable       */
    uint32_t :1;               /*!< bit:     19  Reserved                           */
    uint32_t PINVEN0:1;        /*!< bit:     20  IO Pin 0 Invert Enable             */
    uint32_t PINVEN1:1;        /*!< bit:     21  IO Pin 1 Invert Enable             */
    uint32_t PINVEN2:1;        /*!< bit:     22  IO Pin 2 Invert Enable             */
    uint32_t :1;               /*!< bit:     23  Reserved                           */
    uint32_t ANGULAR:3;        /*!< bit: 24..26  Angular Counter Length             */
    uint32_t :1;               /*!< bit:     27  Reserved                           */
    uint32_t MAXCMP:4;         /*!< bit: 28..31  Maximum Consecutive Missing Pulses */
  } bit;                       /*!< Structure used for bit  access                  */
  struct {
    uint32_t :16;              /*!< bit:  0..15  Reserved                           */
    uint32_t PINEN:3;          /*!< bit: 16..18  PDEC Input From Pin x Enable       */
    uint32_t :1;               /*!< bit:     19  Reserved                           */
    uint32_t PINVEN:3;         /*!< bit: 20..22  IO Pin x Invert Enable             */
    uint32_t :9;               /*!< bit: 23..31  Reserved                           */
  } vec;                       /*!< Structure used for vec  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} PDEC_CTRLA_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- PDEC_CTRLBCLR : (PDEC Offset: 0x04) (R/W 8) Control B Clear -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint8_t  :1;               /*!< bit:      0  Reserved                           */
    uint8_t  LUPD:1;           /*!< bit:      1  Lock Update                        */
    uint8_t  :3;               /*!< bit:  2.. 4  Reserved                           */
    uint8_t  CMD:3;            /*!< bit:  5.. 7  Command                            */
  } bit;                       /*!< Structure used for bit  access                  */
  uint8_t reg;                 /*!< Type      used for register access              */
} PDEC_CTRLBCLR_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- PDEC_CTRLBSET : (PDEC Offset: 0x05) (R/W 8) Control B Set -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint8_t  :1;               /*!< bit:      0  Reserved                           */
    uint8_t  LUPD:1;           /*!< bit:      1  Lock Update                        */
    uint8_t  :3;               /*!< bit:  2.. 4  Reserved                           */
    uint8_t  CMD:3;            /*!< bit:  5.. 7  Command                            */
  } bit;                       /*!< Structure used for bit  access                  */
  uint8_t reg;                 /*!< Type      used for register access              */
} PDEC_CTRLBSET_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- PDEC_EVCTRL : (PDEC Offset: 0x06) (R/W 16) Event Control -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint16_t EVACT:2;          /*!< bit:  0.. 1  Event Action                       */
    uint16_t EVINV:3;          /*!< bit:  2.. 4  Inverted Event Input Enable        */
    uint16_t EVEI:3;           /*!< bit:  5.. 7  Event Input Enable                 */
    uint16_t OVFEO:1;          /*!< bit:      8  Overflow/Underflow Output Event Enable */
    uint16_t ERREO:1;          /*!< bit:      9  Error  Output Event Enable         */
    uint16_t DIREO:1;          /*!< bit:     10  Direction Output Event Enable      */
    uint16_t VLCEO:1;          /*!< bit:     11  Velocity Output Event Enable       */
    uint16_t MCEO0:1;          /*!< bit:     12  Match Channel 0 Event Output Enable */
    uint16_t MCEO1:1;          /*!< bit:     13  Match Channel 1 Event Output Enable */
    uint16_t :2;               /*!< bit: 14..15  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  struct {
    uint16_t :12;              /*!< bit:  0..11  Reserved                           */
    uint16_t MCEO:2;           /*!< bit: 12..13  Match Channel x Event Output Enable */
    uint16_t :2;               /*!< bit: 14..15  Reserved                           */
  } vec;                       /*!< Structure used for vec  access                  */
  uint16_t reg;                /*!< Type      used for register access              */
} PDEC_EVCTRL_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- PDEC_INTENCLR : (PDEC Offset: 0x08) (R/W 8) Interrupt Enable Clear -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint8_t  OVF:1;            /*!< bit:      0  Overflow/Underflow Interrupt Disable */
    uint8_t  ERR:1;            /*!< bit:      1  Error Interrupt Disable            */
    uint8_t  DIR:1;            /*!< bit:      2  Direction Interrupt Disable        */
    uint8_t  VLC:1;            /*!< bit:      3  Velocity Interrupt Disable         */
    uint8_t  MC0:1;            /*!< bit:      4  Channel 0 Compare Match Disable    */
    uint8_t  MC1:1;            /*!< bit:      5  Channel 1 Compare Match Disable    */
    uint8_t  :2;               /*!< bit:  6.. 7  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  struct {
    uint8_t  :4;               /*!< bit:  0.. 3  Reserved                           */
    uint8_t  MC:2;             /*!< bit:  4.. 5  Channel x Compare Match Disable    */
    uint8_t  :2;               /*!< bit:  6.. 7  Reserved                           */
  } vec;                       /*!< Structure used for vec  access                  */
  uint8_t reg;                 /*!< Type      used for register access              */
} PDEC_INTENCLR_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- PDEC_INTENSET : (PDEC Offset: 0x09) (R/W 8) Interrupt Enable Set -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint8_t  OVF:1;            /*!< bit:      0  Overflow/Underflow Interrupt Enable */
    uint8_t  ERR:1;            /*!< bit:      1  Error Interrupt Enable             */
    uint8_t  DIR:1;            /*!< bit:      2  Direction Interrupt Enable         */
    uint8_t  VLC:1;            /*!< bit:      3  Velocity Interrupt Enable          */
    uint8_t  MC0:1;            /*!< bit:      4  Channel 0 Compare Match Enable     */
    uint8_t  MC1:1;            /*!< bit:      5  Channel 1 Compare Match Enable     */
    uint8_t  :2;               /*!< bit:  6.. 7  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  struct {
    uint8_t  :4;               /*!< bit:  0.. 3  Reserved                           */
    uint8_t  MC:2;             /*!< bit:  4.. 5  Channel x Compare Match Enable     */
    uint8_t  :2;               /*!< bit:  6.. 7  Reserved                           */
  } vec;                       /*!< Structure used for vec  access                  */
  uint8_t reg;                 /*!< Type      used for register access              */
} PDEC_INTENSET_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- PDEC_INTFLAG : (PDEC Offset: 0x0A) (R/W 8) Interrupt Flag Status and Clear -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { // __I to avoid read-modify-write on write-to-clear register
  struct {
    __I uint8_t  OVF:1;            /*!< bit:      0  Overflow/Underflow                 */
    __I uint8_t  ERR:1;            /*!< bit:      1  Error                              */
    __I uint8_t  DIR:1;            /*!< bit:      2  Direction Change                   */
    __I uint8_t  VLC:1;            /*!< bit:      3  Velocity                           */
    __I uint8_t  MC0:1;            /*!< bit:      4  Channel 0 Compare Match            */
    __I uint8_t  MC1:1;            /*!< bit:      5  Channel 1 Compare Match            */
    __I uint8_t  :2;               /*!< bit:  6.. 7  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  struct {
    __I uint8_t  :4;               /*!< bit:  0.. 3  Reserved                           */
    __I uint8_t  MC:2;             /*!< bit:  4.. 5  Channel x Compare Match            */
    __I uint8_t  :2;               /*!< bit:  6.. 7  Reserved                           */
  } vec;                       /*!< Structure used for vec  access                  */
  uint8_t reg;                 /*!< Type      used for register access              */
} PDEC_INTFLAG_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- PDEC_STATUS : (PDEC Offset: 0x0C) (R/W 16) Status -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint16_t QERR:1;           /*!< bit:      0  Quadrature Error Flag              */
    uint16_t IDXERR:1;         /*!< bit:      1  Index Error Flag                   */
    uint16_t MPERR:1;          /*!< bit:      2  Missing Pulse Error flag           */
    uint16_t :1;               /*!< bit:      3  Reserved                           */
    uint16_t WINERR:1;         /*!< bit:      4  Window Error Flag                  */
    uint16_t HERR:1;           /*!< bit:      5  Hall Error Flag                    */
    uint16_t STOP:1;           /*!< bit:      6  Stop                               */
    uint16_t DIR:1;            /*!< bit:      7  Direction Status Flag              */
    uint16_t PRESCBUFV:1;      /*!< bit:      8  Prescaler Buffer Valid             */
    uint16_t FILTERBUFV:1;     /*!< bit:      9  Filter Buffer Valid                */
    uint16_t :2;               /*!< bit: 10..11  Reserved                           */
    uint16_t CCBUFV0:1;        /*!< bit:     12  Compare Channel 0 Buffer Valid     */
    uint16_t CCBUFV1:1;        /*!< bit:     13  Compare Channel 1 Buffer Valid     */
    uint16_t :2;               /*!< bit: 14..15  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  struct {
    uint16_t :12;              /*!< bit:  0..11  Reserved                           */
    uint16_t CCBUFV:2;         /*!< bit: 12..13  Compare Channel x Buffer Valid     */
    uint16_t :2;               /*!< bit: 14..15  Reserved                           */
  } vec;                       /*!< Structure used for vec  access                  */
  uint16_t reg;                /*!< Type      used for register access              */
} PDEC_STATUS_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- PDEC_DBGCTRL : (PDEC Offset: 0x0F) (R/W 8) Debug Control -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint8_t  DBGRUN:1;         /*!< bit:      0  Debug Run Mode                     */
    uint8_t  :7;               /*!< bit:  1.. 7  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint8_t reg;                 /*!< Type      used for register access              */
} PDEC_DBGCTRL_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- PDEC_SYNCBUSY : (PDEC Offset: 0x10) ( R/ 32) Synchronization Status -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t SWRST:1;          /*!< bit:      0  Software Reset Synchronization Busy */
    uint32_t ENABLE:1;         /*!< bit:      1  Enable Synchronization Busy        */
    uint32_t CTRLB:1;          /*!< bit:      2  Control B Synchronization Busy     */
    uint32_t STATUS:1;         /*!< bit:      3  Status Synchronization Busy        */
    uint32_t PRESC:1;          /*!< bit:      4  Prescaler Synchronization Busy     */
    uint32_t FILTER:1;         /*!< bit:      5  Filter Synchronization Busy        */
    uint32_t COUNT:1;          /*!< bit:      6  Count Synchronization Busy         */
    uint32_t CC0:1;            /*!< bit:      7  Compare Channel 0 Synchronization Busy */
    uint32_t CC1:1;            /*!< bit:      8  Compare Channel 1 Synchronization Busy */
    uint32_t :23;              /*!< bit:  9..31  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  struct {
    uint32_t :7;               /*!< bit:  0.. 6  Reserved                           */
    uint32_t CC:2;             /*!< bit:  7.. 8  Compare Channel x Synchronization Busy */
    uint32_t :23;              /*!< bit:  9..31  Reserved                           */
  } vec;                       /*!< Structure used for vec  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} PDEC_SYNCBUSY_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */
/* -------- PDEC_PRESC : (PDEC Offset: 0x14) (R/W 8) Prescaler Value -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint8_t  PRESC:4;          /*!< bit:  0.. 3  Prescaler Value                    */
    uint8_t  :4;               /*!< bit:  4.. 7  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint8_t reg;                 /*!< Type      used for register access              */
} PDEC_PRESC_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- PDEC_FILTER : (PDEC Offset: 0x15) (R/W 8) Filter Value -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint8_t  FILTER:8;         /*!< bit:  0.. 7  Filter Value                       */
  } bit;                       /*!< Structure used for bit  access                  */
  uint8_t reg;                 /*!< Type      used for register access              */
} PDEC_FILTER_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- PDEC_PRESCBUF : (PDEC Offset: 0x18) (R/W 8) Prescaler Buffer Value -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint8_t  PRESCBUF:4;       /*!< bit:  0.. 3  Prescaler Buffer Value             */
    uint8_t  :4;               /*!< bit:  4.. 7  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint8_t reg;                 /*!< Type      used for register access              */
} PDEC_PRESCBUF_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- PDEC_FILTERBUF : (PDEC Offset: 0x19) (R/W 8) Filter Buffer Value -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint8_t  FILTERBUF:8;      /*!< bit:  0.. 7  Filter Buffer Value                */
  } bit;                       /*!< Structure used for bit  access                  */
  uint8_t reg;                 /*!< Type      used for register access              */
} PDEC_FILTERBUF_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- PDEC_COUNT : (PDEC Offset: 0x1C) (R/W 32) Counter Value -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t COUNT:16;         /*!< bit:  0..15  Counter Value                      */
    uint32_t :16;              /*!< bit: 16..31  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} PDEC_COUNT_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- PDEC_CC : (PDEC Offset: 0x20) (R/W 32) Channel n Compare Value -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t CC:16;            /*!< bit:  0..15  Channel Compare Value              */
    uint32_t :16;              /*!< bit: 16..31  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} PDEC_CC_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- PDEC_CCBUF : (PDEC Offset: 0x30) (R/W 32) Channel Compare Buffer Value -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t CCBUF:16;         /*!< bit:  0..15  Channel Compare Buffer Value       */
    uint32_t :16;              /*!< bit: 16..31  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} PDEC_CCBUF_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/** \brief PDEC hardware registers */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef struct {
  __IO PDEC_CTRLA_Type           CTRLA;       /**< \brief Offset: 0x00 (R/W 32) Control A */
  __IO PDEC_CTRLBCLR_Type        CTRLBCLR;    /**< \brief Offset: 0x04 (R/W  8) Control B Clear */
  __IO PDEC_CTRLBSET_Type        CTRLBSET;    /**< \brief Offset: 0x05 (R/W  8) Control B Set */
  __IO PDEC_EVCTRL_Type          EVCTRL;      /**< \brief Offset: 0x06 (R/W 16) Event Control */
  __IO PDEC_INTENCLR_Type        INTENCLR;    /**< \brief Offset: 0x08 (R/W  8) Interrupt Enable Clear */
  __IO PDEC_INTENSET_Type        INTENSET;    /**< \brief Offset: 0x09 (R/W  8) Interrupt Enable Set */
  __IO PDEC_INTFLAG_Type         INTFLAG;     /**< \brief Offset: 0x0A (R/W  8) Interrupt Flag Status and Clear */
       RoReg8                    Reserved1[0x1];
  __IO PDEC_STATUS_Type          STATUS;      /**< \brief Offset: 0x0C (R/W 16) Status */
       RoReg8                    Reserved2[0x1];
  __IO PDEC_DBGCTRL_Type         DBGCTRL;     /**< \brief Offset: 0x0F (R/W  8) Debug Control */
  __I  PDEC_SYNCBUSY_Type        SYNCBUSY;    /**< \brief Offset: 0x10 (R/  32) Synchronization Status */
  __IO PDEC_PRESC_Type           PRESC;       /**< \brief Offset: 0x14 (R/W  8) Prescaler Value */
  __IO PDEC_FILTER_Type          FILTER;      /**< \brief Offset: 0x15 (R/W  8) Filter Value */
       RoReg8                    Reserved3[0x2];
  __IO PDEC_PRESCBUF_Type        PRESCBUF;    /**< \brief Offset: 0x18 (R/W  8) Prescaler Buffer Value */
  __IO PDEC_FILTERBUF_Type       FILTERBUF;   /**< \brief Offset: 0x19 (R/W  8) Filter Buffer Value */
       RoReg8                    Reserved4[0x2];
  __IO PDEC_COUNT_Type           COUNT;       /**< \brief Offset: 0x1C (R/W 32) Counter Value */
  __IO PDEC_CC_Type              CC[2];       /**< \brief Offset: 0x20 (R/W 32) Channel n Compare Value */
       RoReg8                    Reserved5[0x8];
  __IO PDEC_CCBUF_Type           CCBUF[2];    /**< \brief Offset: 0x30 (R/W 32) Channel Compare Buffer Value */
} Pdec;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

#endif /* _MICROCHIP_PIC32CXSG_PDEC_COMPONENT_FIXUP_H_ */
