/*
 * Copyright (c) 2024 Microchip
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _MICROCHIP_PIC32CXSG_EIC_COMPONENT_FIXUP_H_
#define _MICROCHIP_PIC32CXSG_EIC_COMPONENT_FIXUP_H_
/* -------- EIC_CTRLA : (EIC Offset: 0x00) (R/W 8) Control A -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint8_t  SWRST:1;          /*!< bit:      0  Software Reset                     */
    uint8_t  ENABLE:1;         /*!< bit:      1  Enable                             */
    uint8_t  :2;               /*!< bit:  2.. 3  Reserved                           */
    uint8_t  CKSEL:1;          /*!< bit:      4  Clock Selection                    */
    uint8_t  :3;               /*!< bit:  5.. 7  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint8_t reg;                 /*!< Type      used for register access              */
} EIC_CTRLA_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- EIC_NMICTRL : (EIC Offset: 0x01) (R/W 8) Non-Maskable Interrupt Control -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint8_t  NMISENSE:3;       /*!< bit:  0.. 2  Non-Maskable Interrupt Sense Configuration */
    uint8_t  NMIFILTEN:1;      /*!< bit:      3  Non-Maskable Interrupt Filter Enable */
    uint8_t  NMIASYNCH:1;      /*!< bit:      4  Asynchronous Edge Detection Mode   */
    uint8_t  :3;               /*!< bit:  5.. 7  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint8_t reg;                 /*!< Type      used for register access              */
} EIC_NMICTRL_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- EIC_NMIFLAG : (EIC Offset: 0x02) (R/W 16) Non-Maskable Interrupt Flag Status and Clear -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint16_t NMI:1;            /*!< bit:      0  Non-Maskable Interrupt             */
    uint16_t :15;              /*!< bit:  1..15  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint16_t reg;                /*!< Type      used for register access              */
} EIC_NMIFLAG_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- EIC_SYNCBUSY : (EIC Offset: 0x04) ( R/ 32) Synchronization Busy -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t SWRST:1;          /*!< bit:      0  Software Reset Synchronization Busy Status */
    uint32_t ENABLE:1;         /*!< bit:      1  Enable Synchronization Busy Status */
    uint32_t :30;              /*!< bit:  2..31  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} EIC_SYNCBUSY_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- EIC_EVCTRL : (EIC Offset: 0x08) (R/W 32) Event Control -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t EXTINTEO:16;      /*!< bit:  0..15  External Interrupt Event Output Enable */
    uint32_t :16;              /*!< bit: 16..31  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} EIC_EVCTRL_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- EIC_INTENCLR : (EIC Offset: 0x0C) (R/W 32) Interrupt Enable Clear -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t EXTINT:16;        /*!< bit:  0..15  External Interrupt Enable          */
    uint32_t :16;              /*!< bit: 16..31  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} EIC_INTENCLR_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- EIC_INTENSET : (EIC Offset: 0x10) (R/W 32) Interrupt Enable Set -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t EXTINT:16;        /*!< bit:  0..15  External Interrupt Enable          */
    uint32_t :16;              /*!< bit: 16..31  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} EIC_INTENSET_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- EIC_INTFLAG : (EIC Offset: 0x14) (R/W 32) Interrupt Flag Status and Clear -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { // __I to avoid read-modify-write on write-to-clear register
  struct {
    __I uint32_t EXTINT:16;        /*!< bit:  0..15  External Interrupt                 */
    __I uint32_t :16;              /*!< bit: 16..31  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} EIC_INTFLAG_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- EIC_ASYNCH : (EIC Offset: 0x18) (R/W 32) External Interrupt Asynchronous Mode -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t ASYNCH:16;        /*!< bit:  0..15  Asynchronous Edge Detection Mode   */
    uint32_t :16;              /*!< bit: 16..31  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} EIC_ASYNCH_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */
/* -------- EIC_CONFIG0 : (EIC Offset: 0x1C) (R/W 32) External Interrupt Sense Configuration -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t SENSE0:3;         /*!< bit:  0.. 2  Input Sense Configuration 0        */
    uint32_t FILTEN0:1;        /*!< bit:      3  Filter Enable 0                    */
    uint32_t SENSE1:3;         /*!< bit:  4.. 6  Input Sense Configuration 1        */
    uint32_t FILTEN1:1;        /*!< bit:      7  Filter Enable 1                    */
    uint32_t SENSE2:3;         /*!< bit:  8..10  Input Sense Configuration 2        */
    uint32_t FILTEN2:1;        /*!< bit:     11  Filter Enable 2                    */
    uint32_t SENSE3:3;         /*!< bit: 12..14  Input Sense Configuration 3        */
    uint32_t FILTEN3:1;        /*!< bit:     15  Filter Enable 3                    */
    uint32_t SENSE4:3;         /*!< bit: 16..18  Input Sense Configuration 4        */
    uint32_t FILTEN4:1;        /*!< bit:     19  Filter Enable 4                    */
    uint32_t SENSE5:3;         /*!< bit: 20..22  Input Sense Configuration 5        */
    uint32_t FILTEN5:1;        /*!< bit:     23  Filter Enable 5                    */
    uint32_t SENSE6:3;         /*!< bit: 24..26  Input Sense Configuration 6        */
    uint32_t FILTEN6:1;        /*!< bit:     27  Filter Enable 6                    */
    uint32_t SENSE7:3;         /*!< bit: 28..30  Input Sense Configuration 7        */
    uint32_t FILTEN7:1;        /*!< bit:     31  Filter Enable 7                    */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} EIC_CONFIG_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* For SAM0 compatibily start */
/* -------- EIC_CONFIG0 : (EIC Offset: 0x20) (R/W 32) External Interrupt Sense Configuration -------- */
#define EIC_CONFIG_SENSE0_NONE               (EIC_CONFIG0_SENSE0_NONE_Val << EIC_CONFIG0_SENSE0_Pos) /* (EIC_CONFIG0) No detection Position  */
#define EIC_CONFIG_SENSE0_RISE               (EIC_CONFIG0_SENSE0_RISE_Val << EIC_CONFIG0_SENSE0_Pos) /* (EIC_CONFIG0) Rising edge detection Position  */
#define EIC_CONFIG_SENSE0_FALL               (EIC_CONFIG0_SENSE0_FALL_Val << EIC_CONFIG0_SENSE0_Pos) /* (EIC_CONFIG0) Falling edge detection Position  */
#define EIC_CONFIG_SENSE0_BOTH               (EIC_CONFIG0_SENSE0_BOTH_Val << EIC_CONFIG0_SENSE0_Pos) /* (EIC_CONFIG0) Both edges detection Position  */
#define EIC_CONFIG_SENSE0_HIGH               (EIC_CONFIG0_SENSE0_HIGH_Val << EIC_CONFIG0_SENSE0_Pos) /* (EIC_CONFIG0) High level detection Position  */
#define EIC_CONFIG_SENSE0_LOW                (EIC_CONFIG0_SENSE0_LOW_Val << EIC_CONFIG0_SENSE0_Pos) /* (EIC_CONFIG0) Low level detection Position  */
#define EIC_CONFIG_FILTEN0                   (_UINT32_(0x1) << EIC_CONFIG0_FILTEN0_Pos)           /* (EIC_CONFIG0) Filter Enable 0 Mask */
/* For SAM0 compatibily end */

/* -------- EIC_DEBOUNCEN : (EIC Offset: 0x30) (R/W 32) Debouncer Enable -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t DEBOUNCEN:16;     /*!< bit:  0..15  Debouncer Enable                   */
    uint32_t :16;              /*!< bit: 16..31  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} EIC_DEBOUNCEN_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- EIC_DPRESCALER : (EIC Offset: 0x34) (R/W 32) Debouncer Prescaler -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t PRESCALER0:3;     /*!< bit:  0.. 2  Debouncer Prescaler                */
    uint32_t STATES0:1;        /*!< bit:      3  Debouncer number of states         */
    uint32_t PRESCALER1:3;     /*!< bit:  4.. 6  Debouncer Prescaler                */
    uint32_t STATES1:1;        /*!< bit:      7  Debouncer number of states         */
    uint32_t :8;               /*!< bit:  8..15  Reserved                           */
    uint32_t TICKON:1;         /*!< bit:     16  Pin Sampler frequency selection    */
    uint32_t :15;              /*!< bit: 17..31  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} EIC_DPRESCALER_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- EIC_PINSTATE : (EIC Offset: 0x38) ( R/ 32) Pin State -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t PINSTATE:16;      /*!< bit:  0..15  Pin State                          */
    uint32_t :16;              /*!< bit: 16..31  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} EIC_PINSTATE_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef struct {
  __IO EIC_CTRLA_Type            CTRLA;       /**< \brief Offset: 0x00 (R/W  8) Control A */
  __IO EIC_NMICTRL_Type          NMICTRL;     /**< \brief Offset: 0x01 (R/W  8) Non-Maskable Interrupt Control */
  __IO EIC_NMIFLAG_Type          NMIFLAG;     /**< \brief Offset: 0x02 (R/W 16) Non-Maskable Interrupt Flag Status and Clear */
  __I  EIC_SYNCBUSY_Type         SYNCBUSY;    /**< \brief Offset: 0x04 (R/  32) Synchronization Busy */
  __IO EIC_EVCTRL_Type           EVCTRL;      /**< \brief Offset: 0x08 (R/W 32) Event Control */
  __IO EIC_INTENCLR_Type         INTENCLR;    /**< \brief Offset: 0x0C (R/W 32) Interrupt Enable Clear */
  __IO EIC_INTENSET_Type         INTENSET;    /**< \brief Offset: 0x10 (R/W 32) Interrupt Enable Set */
  __IO EIC_INTFLAG_Type          INTFLAG;     /**< \brief Offset: 0x14 (R/W 32) Interrupt Flag Status and Clear */
  __IO EIC_ASYNCH_Type           ASYNCH;      /**< \brief Offset: 0x18 (R/W 32) External Interrupt Asynchronous Mode */
  __IO EIC_CONFIG_Type           CONFIG[2];   /**< \brief Offset: 0x1C (R/W 32) External Interrupt Sense Configuration */
       RoReg8                    Reserved1[0xC];
  __IO EIC_DEBOUNCEN_Type        DEBOUNCEN;   /**< \brief Offset: 0x30 (R/W 32) Debouncer Enable */
  __IO EIC_DPRESCALER_Type       DPRESCALER;  /**< \brief Offset: 0x34 (R/W 32) Debouncer Prescaler */
  __I  EIC_PINSTATE_Type         PINSTATE;    /**< \brief Offset: 0x38 (R/  32) Pin State */
} Eic;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */


#endif /* _MICROCHIP_PIC32CXSG_EIC_COMPONENT_FIXUP_H_ */
