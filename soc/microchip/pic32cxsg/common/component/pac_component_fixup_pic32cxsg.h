/*
 * Copyright (c) 2024 Microchip
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _MICROCHIP_PIC32CXSG_PAC_COMPONENT_FIXUP_H_
#define _MICROCHIP_PIC32CXSG_PAC_COMPONENT_FIXUP_H_

/* -------- PAC_WRCTRL : (PAC Offset: 0x00) (R/W 32) Write control -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t PERID:16;         /*!< bit:  0..15  Peripheral identifier              */
    uint32_t KEY:8;            /*!< bit: 16..23  Peripheral access control key      */
    uint32_t :8;               /*!< bit: 24..31  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} PAC_WRCTRL_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- PAC_EVCTRL : (PAC Offset: 0x04) (R/W 8) Event control -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint8_t  ERREO:1;          /*!< bit:      0  Peripheral acess error event output */
    uint8_t  :7;               /*!< bit:  1.. 7  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint8_t reg;                 /*!< Type      used for register access              */
} PAC_EVCTRL_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- PAC_INTENCLR : (PAC Offset: 0x08) (R/W 8) Interrupt enable clear -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint8_t  ERR:1;            /*!< bit:      0  Peripheral access error interrupt disable */
    uint8_t  :7;               /*!< bit:  1.. 7  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint8_t reg;                 /*!< Type      used for register access              */
} PAC_INTENCLR_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- PAC_INTENSET : (PAC Offset: 0x09) (R/W 8) Interrupt enable set -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint8_t  ERR:1;            /*!< bit:      0  Peripheral access error interrupt enable */
    uint8_t  :7;               /*!< bit:  1.. 7  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint8_t reg;                 /*!< Type      used for register access              */
} PAC_INTENSET_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- PAC_INTFLAGAHB : (PAC Offset: 0x10) (R/W 32) Bridge interrupt flag status -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { // __I to avoid read-modify-write on write-to-clear register
  struct {
    __I uint32_t FLASH_:1;         /*!< bit:      0  FLASH                              */
    __I uint32_t FLASH_ALT_:1;     /*!< bit:      1  FLASH_ALT                          */
    __I uint32_t SEEPROM_:1;       /*!< bit:      2  SEEPROM                            */
    __I uint32_t RAMCM4S_:1;       /*!< bit:      3  RAMCM4S                            */
    __I uint32_t RAMPPPDSU_:1;     /*!< bit:      4  RAMPPPDSU                          */
    __I uint32_t RAMDMAWR_:1;      /*!< bit:      5  RAMDMAWR                           */
    __I uint32_t RAMDMACICM_:1;    /*!< bit:      6  RAMDMACICM                         */
    __I uint32_t HPB0_:1;          /*!< bit:      7  HPB0                               */
    __I uint32_t HPB1_:1;          /*!< bit:      8  HPB1                               */
    __I uint32_t HPB2_:1;          /*!< bit:      9  HPB2                               */
    __I uint32_t HPB3_:1;          /*!< bit:     10  HPB3                               */
    __I uint32_t PUKCC_:1;         /*!< bit:     11  PUKCC                              */
    __I uint32_t SDHC0_:1;         /*!< bit:     12  SDHC0                              */
    __I uint32_t SDHC1_:1;         /*!< bit:     13  SDHC1                              */
    __I uint32_t QSPI_:1;          /*!< bit:     14  QSPI                               */
    __I uint32_t BKUPRAM_:1;       /*!< bit:     15  BKUPRAM                            */
    __I uint32_t :16;              /*!< bit: 16..31  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} PAC_INTFLAGAHB_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- PAC_INTFLAGA : (PAC Offset: 0x14) (R/W 32) Peripheral interrupt flag status - Bridge A -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { // __I to avoid read-modify-write on write-to-clear register
  struct {
    __I uint32_t PAC_:1;           /*!< bit:      0  PAC                                */
    __I uint32_t PM_:1;            /*!< bit:      1  PM                                 */
    __I uint32_t MCLK_:1;          /*!< bit:      2  MCLK                               */
    __I uint32_t RSTC_:1;          /*!< bit:      3  RSTC                               */
    __I uint32_t OSCCTRL_:1;       /*!< bit:      4  OSCCTRL                            */
    __I uint32_t OSC32KCTRL_:1;    /*!< bit:      5  OSC32KCTRL                         */
    __I uint32_t SUPC_:1;          /*!< bit:      6  SUPC                               */
    __I uint32_t GCLK_:1;          /*!< bit:      7  GCLK                               */
    __I uint32_t WDT_:1;           /*!< bit:      8  WDT                                */
    __I uint32_t RTC_:1;           /*!< bit:      9  RTC                                */
    __I uint32_t EIC_:1;           /*!< bit:     10  EIC                                */
    __I uint32_t FREQM_:1;         /*!< bit:     11  FREQM                              */
    __I uint32_t SERCOM0_:1;       /*!< bit:     12  SERCOM0                            */
    __I uint32_t SERCOM1_:1;       /*!< bit:     13  SERCOM1                            */
    __I uint32_t TC0_:1;           /*!< bit:     14  TC0                                */
    __I uint32_t TC1_:1;           /*!< bit:     15  TC1                                */
    __I uint32_t :16;              /*!< bit: 16..31  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} PAC_INTFLAGA_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- PAC_INTFLAGB : (PAC Offset: 0x18) (R/W 32) Peripheral interrupt flag status - Bridge B -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { // __I to avoid read-modify-write on write-to-clear register
  struct {
    __I uint32_t USB_:1;           /*!< bit:      0  USB                                */
    __I uint32_t DSU_:1;           /*!< bit:      1  DSU                                */
    __I uint32_t NVMCTRL_:1;       /*!< bit:      2  NVMCTRL                            */
    __I uint32_t CMCC_:1;          /*!< bit:      3  CMCC                               */
    __I uint32_t PORT_:1;          /*!< bit:      4  PORT                               */
    __I uint32_t DMAC_:1;          /*!< bit:      5  DMAC                               */
    __I uint32_t HMATRIX_:1;       /*!< bit:      6  HMATRIX                            */
    __I uint32_t EVSYS_:1;         /*!< bit:      7  EVSYS                              */
    __I uint32_t :1;               /*!< bit:      8  Reserved                           */
    __I uint32_t SERCOM2_:1;       /*!< bit:      9  SERCOM2                            */
    __I uint32_t SERCOM3_:1;       /*!< bit:     10  SERCOM3                            */
    __I uint32_t TCC0_:1;          /*!< bit:     11  TCC0                               */
    __I uint32_t TCC1_:1;          /*!< bit:     12  TCC1                               */
    __I uint32_t TC2_:1;           /*!< bit:     13  TC2                                */
    __I uint32_t TC3_:1;           /*!< bit:     14  TC3                                */
    __I uint32_t :1;               /*!< bit:     15  Reserved                           */
    __I uint32_t RAMECC_:1;        /*!< bit:     16  RAMECC                             */
    __I uint32_t :15;              /*!< bit: 17..31  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} PAC_INTFLAGB_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- PAC_INTFLAGC : (PAC Offset: 0x1C) (R/W 32) Peripheral interrupt flag status - Bridge C -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { // __I to avoid read-modify-write on write-to-clear register
  struct {
    __I uint32_t CAN0_:1;          /*!< bit:      0  CAN0                               */
    __I uint32_t CAN1_:1;          /*!< bit:      1  CAN1                               */
    __I uint32_t GMAC_:1;          /*!< bit:      2  GMAC                               */
    __I uint32_t TCC2_:1;          /*!< bit:      3  TCC2                               */
    __I uint32_t TCC3_:1;          /*!< bit:      4  TCC3                               */
    __I uint32_t TC4_:1;           /*!< bit:      5  TC4                                */
    __I uint32_t TC5_:1;           /*!< bit:      6  TC5                                */
    __I uint32_t PDEC_:1;          /*!< bit:      7  PDEC                               */
    __I uint32_t AC_:1;            /*!< bit:      8  AC                                 */
    __I uint32_t AES_:1;           /*!< bit:      9  AES                                */
    __I uint32_t TRNG_:1;          /*!< bit:     10  TRNG                               */
    __I uint32_t ICM_:1;           /*!< bit:     11  ICM                                */
    __I uint32_t PUKCC_:1;         /*!< bit:     12  PUKCC                              */
    __I uint32_t QSPI_:1;          /*!< bit:     13  QSPI                               */
    __I uint32_t CCL_:1;           /*!< bit:     14  CCL                                */
    __I uint32_t :17;              /*!< bit: 15..31  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} PAC_INTFLAGC_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- PAC_INTFLAGD : (PAC Offset: 0x20) (R/W 32) Peripheral interrupt flag status - Bridge D -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { // __I to avoid read-modify-write on write-to-clear register
  struct {
    __I uint32_t SERCOM4_:1;       /*!< bit:      0  SERCOM4                            */
    __I uint32_t SERCOM5_:1;       /*!< bit:      1  SERCOM5                            */
    __I uint32_t SERCOM6_:1;       /*!< bit:      2  SERCOM6                            */
    __I uint32_t SERCOM7_:1;       /*!< bit:      3  SERCOM7                            */
    __I uint32_t TCC4_:1;          /*!< bit:      4  TCC4                               */
    __I uint32_t TC6_:1;           /*!< bit:      5  TC6                                */
    __I uint32_t TC7_:1;           /*!< bit:      6  TC7                                */
    __I uint32_t ADC0_:1;          /*!< bit:      7  ADC0                               */
    __I uint32_t ADC1_:1;          /*!< bit:      8  ADC1                               */
    __I uint32_t DAC_:1;           /*!< bit:      9  DAC                                */
    __I uint32_t I2S_:1;           /*!< bit:     10  I2S                                */
    __I uint32_t PCC_:1;           /*!< bit:     11  PCC                                */
    __I uint32_t :20;              /*!< bit: 12..31  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} PAC_INTFLAGD_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- PAC_STATUSA : (PAC Offset: 0x34) ( R/ 32) Peripheral write protection status - Bridge A -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t PAC_:1;           /*!< bit:      0  PAC APB Protect Enable             */
    uint32_t PM_:1;            /*!< bit:      1  PM APB Protect Enable              */
    uint32_t MCLK_:1;          /*!< bit:      2  MCLK APB Protect Enable            */
    uint32_t RSTC_:1;          /*!< bit:      3  RSTC APB Protect Enable            */
    uint32_t OSCCTRL_:1;       /*!< bit:      4  OSCCTRL APB Protect Enable         */
    uint32_t OSC32KCTRL_:1;    /*!< bit:      5  OSC32KCTRL APB Protect Enable      */
    uint32_t SUPC_:1;          /*!< bit:      6  SUPC APB Protect Enable            */
    uint32_t GCLK_:1;          /*!< bit:      7  GCLK APB Protect Enable            */
    uint32_t WDT_:1;           /*!< bit:      8  WDT APB Protect Enable             */
    uint32_t RTC_:1;           /*!< bit:      9  RTC APB Protect Enable             */
    uint32_t EIC_:1;           /*!< bit:     10  EIC APB Protect Enable             */
    uint32_t FREQM_:1;         /*!< bit:     11  FREQM APB Protect Enable           */
    uint32_t SERCOM0_:1;       /*!< bit:     12  SERCOM0 APB Protect Enable         */
    uint32_t SERCOM1_:1;       /*!< bit:     13  SERCOM1 APB Protect Enable         */
    uint32_t TC0_:1;           /*!< bit:     14  TC0 APB Protect Enable             */
    uint32_t TC1_:1;           /*!< bit:     15  TC1 APB Protect Enable             */
    uint32_t :16;              /*!< bit: 16..31  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} PAC_STATUSA_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- PAC_STATUSB : (PAC Offset: 0x38) ( R/ 32) Peripheral write protection status - Bridge B -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t USB_:1;           /*!< bit:      0  USB APB Protect Enable             */
    uint32_t DSU_:1;           /*!< bit:      1  DSU APB Protect Enable             */
    uint32_t NVMCTRL_:1;       /*!< bit:      2  NVMCTRL APB Protect Enable         */
    uint32_t CMCC_:1;          /*!< bit:      3  CMCC APB Protect Enable            */
    uint32_t PORT_:1;          /*!< bit:      4  PORT APB Protect Enable            */
    uint32_t DMAC_:1;          /*!< bit:      5  DMAC APB Protect Enable            */
    uint32_t HMATRIX_:1;       /*!< bit:      6  HMATRIX APB Protect Enable         */
    uint32_t EVSYS_:1;         /*!< bit:      7  EVSYS APB Protect Enable           */
    uint32_t :1;               /*!< bit:      8  Reserved                           */
    uint32_t SERCOM2_:1;       /*!< bit:      9  SERCOM2 APB Protect Enable         */
    uint32_t SERCOM3_:1;       /*!< bit:     10  SERCOM3 APB Protect Enable         */
    uint32_t TCC0_:1;          /*!< bit:     11  TCC0 APB Protect Enable            */
    uint32_t TCC1_:1;          /*!< bit:     12  TCC1 APB Protect Enable            */
    uint32_t TC2_:1;           /*!< bit:     13  TC2 APB Protect Enable             */
    uint32_t TC3_:1;           /*!< bit:     14  TC3 APB Protect Enable             */
    uint32_t :1;               /*!< bit:     15  Reserved                           */
    uint32_t RAMECC_:1;        /*!< bit:     16  RAMECC APB Protect Enable          */
    uint32_t :15;              /*!< bit: 17..31  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} PAC_STATUSB_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- PAC_STATUSC : (PAC Offset: 0x3C) ( R/ 32) Peripheral write protection status - Bridge C -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t CAN0_:1;          /*!< bit:      0  CAN0 APB Protect Enable            */
    uint32_t CAN1_:1;          /*!< bit:      1  CAN1 APB Protect Enable            */
    uint32_t GMAC_:1;          /*!< bit:      2  GMAC APB Protect Enable            */
    uint32_t TCC2_:1;          /*!< bit:      3  TCC2 APB Protect Enable            */
    uint32_t TCC3_:1;          /*!< bit:      4  TCC3 APB Protect Enable            */
    uint32_t TC4_:1;           /*!< bit:      5  TC4 APB Protect Enable             */
    uint32_t TC5_:1;           /*!< bit:      6  TC5 APB Protect Enable             */
    uint32_t PDEC_:1;          /*!< bit:      7  PDEC APB Protect Enable            */
    uint32_t AC_:1;            /*!< bit:      8  AC APB Protect Enable              */
    uint32_t AES_:1;           /*!< bit:      9  AES APB Protect Enable             */
    uint32_t TRNG_:1;          /*!< bit:     10  TRNG APB Protect Enable            */
    uint32_t ICM_:1;           /*!< bit:     11  ICM APB Protect Enable             */
    uint32_t PUKCC_:1;         /*!< bit:     12  PUKCC APB Protect Enable           */
    uint32_t QSPI_:1;          /*!< bit:     13  QSPI APB Protect Enable            */
    uint32_t CCL_:1;           /*!< bit:     14  CCL APB Protect Enable             */
    uint32_t :17;              /*!< bit: 15..31  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} PAC_STATUSC_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- PAC_STATUSD : (PAC Offset: 0x40) ( R/ 32) Peripheral write protection status - Bridge D -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t SERCOM4_:1;       /*!< bit:      0  SERCOM4 APB Protect Enable         */
    uint32_t SERCOM5_:1;       /*!< bit:      1  SERCOM5 APB Protect Enable         */
    uint32_t SERCOM6_:1;       /*!< bit:      2  SERCOM6 APB Protect Enable         */
    uint32_t SERCOM7_:1;       /*!< bit:      3  SERCOM7 APB Protect Enable         */
    uint32_t TCC4_:1;          /*!< bit:      4  TCC4 APB Protect Enable            */
    uint32_t TC6_:1;           /*!< bit:      5  TC6 APB Protect Enable             */
    uint32_t TC7_:1;           /*!< bit:      6  TC7 APB Protect Enable             */
    uint32_t ADC0_:1;          /*!< bit:      7  ADC0 APB Protect Enable            */
    uint32_t ADC1_:1;          /*!< bit:      8  ADC1 APB Protect Enable            */
    uint32_t DAC_:1;           /*!< bit:      9  DAC APB Protect Enable             */
    uint32_t I2S_:1;           /*!< bit:     10  I2S APB Protect Enable             */
    uint32_t PCC_:1;           /*!< bit:     11  PCC APB Protect Enable             */
    uint32_t :20;              /*!< bit: 12..31  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} PAC_STATUSD_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/** \brief PAC hardware registers */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef struct {
  __IO PAC_WRCTRL_Type           WRCTRL;      /**< \brief Offset: 0x00 (R/W 32) Write control */
  __IO PAC_EVCTRL_Type           EVCTRL;      /**< \brief Offset: 0x04 (R/W  8) Event control */
       RoReg8                    Reserved1[0x3];
  __IO PAC_INTENCLR_Type         INTENCLR;    /**< \brief Offset: 0x08 (R/W  8) Interrupt enable clear */
  __IO PAC_INTENSET_Type         INTENSET;    /**< \brief Offset: 0x09 (R/W  8) Interrupt enable set */
       RoReg8                    Reserved2[0x6];
  __IO PAC_INTFLAGAHB_Type       INTFLAGAHB;  /**< \brief Offset: 0x10 (R/W 32) Bridge interrupt flag status */
  __IO PAC_INTFLAGA_Type         INTFLAGA;    /**< \brief Offset: 0x14 (R/W 32) Peripheral interrupt flag status - Bridge A */
  __IO PAC_INTFLAGB_Type         INTFLAGB;    /**< \brief Offset: 0x18 (R/W 32) Peripheral interrupt flag status - Bridge B */
  __IO PAC_INTFLAGC_Type         INTFLAGC;    /**< \brief Offset: 0x1C (R/W 32) Peripheral interrupt flag status - Bridge C */
  __IO PAC_INTFLAGD_Type         INTFLAGD;    /**< \brief Offset: 0x20 (R/W 32) Peripheral interrupt flag status - Bridge D */
       RoReg8                    Reserved3[0x10];
  __I  PAC_STATUSA_Type          STATUSA;     /**< \brief Offset: 0x34 (R/  32) Peripheral write protection status - Bridge A */
  __I  PAC_STATUSB_Type          STATUSB;     /**< \brief Offset: 0x38 (R/  32) Peripheral write protection status - Bridge B */
  __I  PAC_STATUSC_Type          STATUSC;     /**< \brief Offset: 0x3C (R/  32) Peripheral write protection status - Bridge C */
  __I  PAC_STATUSD_Type          STATUSD;     /**< \brief Offset: 0x40 (R/  32) Peripheral write protection status - Bridge D */
} Pac;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

#endif /* _MICROCHIP_PIC32CXSG_PAC_COMPONENT_FIXUP_H_ */
