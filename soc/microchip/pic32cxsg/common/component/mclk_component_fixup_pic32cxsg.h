/*
 * Copyright (c) 2024 Microchip
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _MICROCHIP_PIC32CXSG_MCLK_COMPONENT_FIXUP_H_
#define _MICROCHIP_PIC32CXSG_MCLK_COMPONENT_FIXUP_H_

/* -------- MCLK_INTENCLR : (MCLK Offset: 0x01) (R/W 8) Interrupt Enable Clear -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint8_t  CKRDY:1;          /*!< bit:      0  Clock Ready Interrupt Enable       */
    uint8_t  :7;               /*!< bit:  1.. 7  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint8_t reg;                 /*!< Type      used for register access              */
} MCLK_INTENCLR_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- MCLK_INTENSET : (MCLK Offset: 0x02) (R/W 8) Interrupt Enable Set -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint8_t  CKRDY:1;          /*!< bit:      0  Clock Ready Interrupt Enable       */
    uint8_t  :7;               /*!< bit:  1.. 7  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint8_t reg;                 /*!< Type      used for register access              */
} MCLK_INTENSET_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- MCLK_INTFLAG : (MCLK Offset: 0x03) (R/W 8) Interrupt Flag Status and Clear -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { // __I to avoid read-modify-write on write-to-clear register
  struct {
    __I uint8_t  CKRDY:1;          /*!< bit:      0  Clock Ready                        */
    __I uint8_t  :7;               /*!< bit:  1.. 7  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint8_t reg;                 /*!< Type      used for register access              */
} MCLK_INTFLAG_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- MCLK_HSDIV : (MCLK Offset: 0x04) ( R/ 8) HS Clock Division -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint8_t  DIV:8;            /*!< bit:  0.. 7  CPU Clock Division Factor          */
  } bit;                       /*!< Structure used for bit  access                  */
  uint8_t reg;                 /*!< Type      used for register access              */
} MCLK_HSDIV_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- MCLK_CPUDIV : (MCLK Offset: 0x05) (R/W 8) CPU Clock Division -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint8_t  DIV:8;            /*!< bit:  0.. 7  Low-Power Clock Division Factor    */
  } bit;                       /*!< Structure used for bit  access                  */
  uint8_t reg;                 /*!< Type      used for register access              */
} MCLK_CPUDIV_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- MCLK_AHBMASK : (MCLK Offset: 0x10) (R/W 32) AHB Mask -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t HPB0_:1;          /*!< bit:      0  HPB0 AHB Clock Mask                */
    uint32_t HPB1_:1;          /*!< bit:      1  HPB1 AHB Clock Mask                */
    uint32_t HPB2_:1;          /*!< bit:      2  HPB2 AHB Clock Mask                */
    uint32_t HPB3_:1;          /*!< bit:      3  HPB3 AHB Clock Mask                */
    uint32_t DSU_:1;           /*!< bit:      4  DSU AHB Clock Mask                 */
    uint32_t HMATRIX_:1;       /*!< bit:      5  HMATRIX AHB Clock Mask             */
    uint32_t NVMCTRL_:1;       /*!< bit:      6  NVMCTRL AHB Clock Mask             */
    uint32_t HSRAM_:1;         /*!< bit:      7  HSRAM AHB Clock Mask               */
    uint32_t CMCC_:1;          /*!< bit:      8  CMCC AHB Clock Mask                */
    uint32_t DMAC_:1;          /*!< bit:      9  DMAC AHB Clock Mask                */
    uint32_t USB_:1;           /*!< bit:     10  USB AHB Clock Mask                 */
    uint32_t BKUPRAM_:1;       /*!< bit:     11  BKUPRAM AHB Clock Mask             */
    uint32_t PAC_:1;           /*!< bit:     12  PAC AHB Clock Mask                 */
    uint32_t QSPI_:1;          /*!< bit:     13  QSPI AHB Clock Mask                */
    uint32_t GMAC_:1;          /*!< bit:     14  GMAC AHB Clock Mask                */
    uint32_t SDHC0_:1;         /*!< bit:     15  SDHC0 AHB Clock Mask               */
    uint32_t SDHC1_:1;         /*!< bit:     16  SDHC1 AHB Clock Mask               */
    uint32_t CAN0_:1;          /*!< bit:     17  CAN0 AHB Clock Mask                */
    uint32_t CAN1_:1;          /*!< bit:     18  CAN1 AHB Clock Mask                */
    uint32_t ICM_:1;           /*!< bit:     19  ICM AHB Clock Mask                 */
    uint32_t PUKCC_:1;         /*!< bit:     20  PUKCC AHB Clock Mask               */
    uint32_t QSPI_2X_:1;       /*!< bit:     21  QSPI_2X AHB Clock Mask             */
    uint32_t NVMCTRL_SMEEPROM_:1; /*!< bit:     22  NVMCTRL_SMEEPROM AHB Clock Mask    */
    uint32_t NVMCTRL_CACHE_:1; /*!< bit:     23  NVMCTRL_CACHE AHB Clock Mask       */
    uint32_t :8;               /*!< bit: 24..31  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} MCLK_AHBMASK_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- MCLK_APBAMASK : (MCLK Offset: 0x14) (R/W 32) APBA Mask -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t PAC_:1;           /*!< bit:      0  PAC APB Clock Enable               */
    uint32_t PM_:1;            /*!< bit:      1  PM APB Clock Enable                */
    uint32_t MCLK_:1;          /*!< bit:      2  MCLK APB Clock Enable              */
    uint32_t RSTC_:1;          /*!< bit:      3  RSTC APB Clock Enable              */
    uint32_t OSCCTRL_:1;       /*!< bit:      4  OSCCTRL APB Clock Enable           */
    uint32_t OSC32KCTRL_:1;    /*!< bit:      5  OSC32KCTRL APB Clock Enable        */
    uint32_t SUPC_:1;          /*!< bit:      6  SUPC APB Clock Enable              */
    uint32_t GCLK_:1;          /*!< bit:      7  GCLK APB Clock Enable              */
    uint32_t WDT_:1;           /*!< bit:      8  WDT APB Clock Enable               */
    uint32_t RTC_:1;           /*!< bit:      9  RTC APB Clock Enable               */
    uint32_t EIC_:1;           /*!< bit:     10  EIC APB Clock Enable               */
    uint32_t FREQM_:1;         /*!< bit:     11  FREQM APB Clock Enable             */
    uint32_t SERCOM0_:1;       /*!< bit:     12  SERCOM0 APB Clock Enable           */
    uint32_t SERCOM1_:1;       /*!< bit:     13  SERCOM1 APB Clock Enable           */
    uint32_t TC0_:1;           /*!< bit:     14  TC0 APB Clock Enable               */
    uint32_t TC1_:1;           /*!< bit:     15  TC1 APB Clock Enable               */
    uint32_t :16;              /*!< bit: 16..31  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} MCLK_APBAMASK_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

#define MCLK_APBAMASK_EIC_BIT_MASK (_UINT32_(0x1) << MCLK_APBAMASK_EIC_Pos) 

/* -------- MCLK_APBBMASK : (MCLK Offset: 0x18) (R/W 32) APBB Mask -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t USB_:1;           /*!< bit:      0  USB APB Clock Enable               */
    uint32_t DSU_:1;           /*!< bit:      1  DSU APB Clock Enable               */
    uint32_t NVMCTRL_:1;       /*!< bit:      2  NVMCTRL APB Clock Enable           */
    uint32_t :1;               /*!< bit:      3  Reserved                           */
    uint32_t PORT_:1;          /*!< bit:      4  PORT APB Clock Enable              */
    uint32_t :1;               /*!< bit:      5  Reserved                           */
    uint32_t HMATRIX_:1;       /*!< bit:      6  HMATRIX APB Clock Enable           */
    uint32_t EVSYS_:1;         /*!< bit:      7  EVSYS APB Clock Enable             */
    uint32_t :1;               /*!< bit:      8  Reserved                           */
    uint32_t SERCOM2_:1;       /*!< bit:      9  SERCOM2 APB Clock Enable           */
    uint32_t SERCOM3_:1;       /*!< bit:     10  SERCOM3 APB Clock Enable           */
    uint32_t TCC0_:1;          /*!< bit:     11  TCC0 APB Clock Enable              */
    uint32_t TCC1_:1;          /*!< bit:     12  TCC1 APB Clock Enable              */
    uint32_t TC2_:1;           /*!< bit:     13  TC2 APB Clock Enable               */
    uint32_t TC3_:1;           /*!< bit:     14  TC3 APB Clock Enable               */
    uint32_t :1;               /*!< bit:     15  Reserved                           */
    uint32_t RAMECC_:1;        /*!< bit:     16  RAMECC APB Clock Enable            */
    uint32_t :15;              /*!< bit: 17..31  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} MCLK_APBBMASK_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- MCLK_APBCMASK : (MCLK Offset: 0x1C) (R/W 32) APBC Mask -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t :2;               /*!< bit:  0.. 1  Reserved                           */
    uint32_t GMAC_:1;          /*!< bit:      2  GMAC APB Clock Enable              */
    uint32_t TCC2_:1;          /*!< bit:      3  TCC2 APB Clock Enable              */
    uint32_t TCC3_:1;          /*!< bit:      4  TCC3 APB Clock Enable              */
    uint32_t TC4_:1;           /*!< bit:      5  TC4 APB Clock Enable               */
    uint32_t TC5_:1;           /*!< bit:      6  TC5 APB Clock Enable               */
    uint32_t PDEC_:1;          /*!< bit:      7  PDEC APB Clock Enable              */
    uint32_t AC_:1;            /*!< bit:      8  AC APB Clock Enable                */
    uint32_t AES_:1;           /*!< bit:      9  AES APB Clock Enable               */
    uint32_t TRNG_:1;          /*!< bit:     10  TRNG APB Clock Enable              */
    uint32_t ICM_:1;           /*!< bit:     11  ICM APB Clock Enable               */
    uint32_t :1;               /*!< bit:     12  Reserved                           */
    uint32_t QSPI_:1;          /*!< bit:     13  QSPI APB Clock Enable              */
    uint32_t CCL_:1;           /*!< bit:     14  CCL APB Clock Enable               */
    uint32_t :17;              /*!< bit: 15..31  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} MCLK_APBCMASK_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- MCLK_APBDMASK : (MCLK Offset: 0x20) (R/W 32) APBD Mask -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t SERCOM4_:1;       /*!< bit:      0  SERCOM4 APB Clock Enable           */
    uint32_t SERCOM5_:1;       /*!< bit:      1  SERCOM5 APB Clock Enable           */
    uint32_t SERCOM6_:1;       /*!< bit:      2  SERCOM6 APB Clock Enable           */
    uint32_t SERCOM7_:1;       /*!< bit:      3  SERCOM7 APB Clock Enable           */
    uint32_t TCC4_:1;          /*!< bit:      4  TCC4 APB Clock Enable              */
    uint32_t TC6_:1;           /*!< bit:      5  TC6 APB Clock Enable               */
    uint32_t TC7_:1;           /*!< bit:      6  TC7 APB Clock Enable               */
    uint32_t ADC0_:1;          /*!< bit:      7  ADC0 APB Clock Enable              */
    uint32_t ADC1_:1;          /*!< bit:      8  ADC1 APB Clock Enable              */
    uint32_t DAC_:1;           /*!< bit:      9  DAC APB Clock Enable               */
    uint32_t I2S_:1;           /*!< bit:     10  I2S APB Clock Enable               */
    uint32_t PCC_:1;           /*!< bit:     11  PCC APB Clock Enable               */
    uint32_t :20;              /*!< bit: 12..31  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} MCLK_APBDMASK_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef struct {
       RoReg8                    Reserved1[0x1];
  __IO MCLK_INTENCLR_Type        INTENCLR;    /**< \brief Offset: 0x01 (R/W  8) Interrupt Enable Clear */
  __IO MCLK_INTENSET_Type        INTENSET;    /**< \brief Offset: 0x02 (R/W  8) Interrupt Enable Set */
  __IO MCLK_INTFLAG_Type         INTFLAG;     /**< \brief Offset: 0x03 (R/W  8) Interrupt Flag Status and Clear */
  __I  MCLK_HSDIV_Type           HSDIV;       /**< \brief Offset: 0x04 (R/   8) HS Clock Division */
  __IO MCLK_CPUDIV_Type          CPUDIV;      /**< \brief Offset: 0x05 (R/W  8) CPU Clock Division */
       RoReg8                    Reserved2[0xA];
  __IO MCLK_AHBMASK_Type         AHBMASK;     /**< \brief Offset: 0x10 (R/W 32) AHB Mask */
  __IO MCLK_APBAMASK_Type        APBAMASK;    /**< \brief Offset: 0x14 (R/W 32) APBA Mask */
  __IO MCLK_APBBMASK_Type        APBBMASK;    /**< \brief Offset: 0x18 (R/W 32) APBB Mask */
  __IO MCLK_APBCMASK_Type        APBCMASK;    /**< \brief Offset: 0x1C (R/W 32) APBC Mask */
  __IO MCLK_APBDMASK_Type        APBDMASK;    /**< \brief Offset: 0x20 (R/W 32) APBD Mask */
} Mclk;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

#endif /* _MICROCHIP_PIC32CXSG_MCLK_COMPONENT_FIXUP_H_ */
