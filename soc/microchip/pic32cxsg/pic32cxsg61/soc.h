/*
 * Copyright (c) 2024 Microchip
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _SOC_MICROCHIP_PIC32CXSG61_SOC_H_
#define _SOC_MICROCHIP_PIC32CXSG61_SOC_H_

#ifndef _ASMLANGUAGE

#define DONT_USE_CMSIS_INIT

#include <zephyr/types.h>

#if defined(CONFIG_SOC_PIC32CX1025SG61100)
#include <pic32cx1025sg61100.h>
#elif defined(CONFIG_SOC_PIC32CX1025SG61128)
#include <pic32cx1025sg61128.h>

#else
#error Library does not support the specified device.
#endif

#endif /* _ASMLANGUAGE */

#if defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)
#define AC               (0x42002000) /**< \brief (AC) APB Base Address */
#define ADC0             (0x43001C00) /**< \brief (ADC0) APB Base Address */
#define ADC1             (0x43002000) /**< \brief (ADC1) APB Base Address */
#define AES              (0x42002400) /**< \brief (AES) APB Base Address */
#define CAN0             (0x42000000) /**< \brief (CAN0) APB Base Address */
#define CAN1             (0x42000400) /**< \brief (CAN1) APB Base Address */
#define CCL              (0x42003800) /**< \brief (CCL) APB Base Address */
#define CMCC             (0x41006000) /**< \brief (CMCC) APB Base Address */
#define CMCC_AHB         (0x03000000) /**< \brief (CMCC) AHB Base Address */
#define DAC              (0x43002400) /**< \brief (DAC) APB Base Address */
#define DMAC             (0x4100A000) /**< \brief (DMAC) APB Base Address */
#define DSU              (0x41002000) /**< \brief (DSU) APB Base Address */
#define EIC              (0x40002800) /**< \brief (EIC) APB Base Address */
#define EVSYS            (0x4100E000) /**< \brief (EVSYS) APB Base Address */
#define FREQM            (0x40002C00) /**< \brief (FREQM) APB Base Address */
#define GCLK             (0x40001C00) /**< \brief (GCLK) APB Base Address */
#define GMAC             (0x42000800) /**< \brief (GMAC) APB Base Address */
#define HMATRIX          (0x4100C000) /**< \brief (HMATRIX) APB Base Address */
#define ICM              (0x42002C00) /**< \brief (ICM) APB Base Address */
#define I2S              (0x43002800) /**< \brief (I2S) APB Base Address */
#define MCLK             (0x40000800) /**< \brief (MCLK) APB Base Address */
#define NVMCTRL          (0x41004000) /**< \brief (NVMCTRL) APB Base Address */
#define NVMCTRL_SW0      (0x00800080) /**< \brief (NVMCTRL) SW0 Base Address */
#define NVMCTRL_TEMP_LOG (0x00800100) /**< \brief (NVMCTRL) TEMP_LOG Base Address */
#define NVMCTRL_USER     (0x00804000) /**< \brief (NVMCTRL) USER Base Address */
#define OSCCTRL          (0x40001000) /**< \brief (OSCCTRL) APB Base Address */
#define OSC32KCTRL       (0x40001400) /**< \brief (OSC32KCTRL) APB Base Address */
#define PAC              (0x40000000) /**< \brief (PAC) APB Base Address */
#define PCC              (0x43002C00) /**< \brief (PCC) APB Base Address */
#define PDEC             (0x42001C00) /**< \brief (PDEC) APB Base Address */
#define PM               (0x40000400) /**< \brief (PM) APB Base Address */
#define PORT             (0x41008000) /**< \brief (PORT) APB Base Address */
#define PUKCC            (0x42003000) /**< \brief (PUKCC) APB Base Address */
#define PUKCC_AHB        (0x02000000) /**< \brief (PUKCC) AHB Base Address */
#define QSPI             (0x42003400) /**< \brief (QSPI) APB Base Address */
#define QSPI_AHB         (0x04000000) /**< \brief (QSPI) AHB Base Address */
#define RAMECC           (0x41020000) /**< \brief (RAMECC) APB Base Address */
#define RSTC             (0x40000C00) /**< \brief (RSTC) APB Base Address */
#define RTC              (0x40002400) /**< \brief (RTC) APB Base Address */
#define SDHC0            (0x45000000) /**< \brief (SDHC0) AHB Base Address */
#define SDHC1            (0x46000000) /**< \brief (SDHC1) AHB Base Address */
#define SERCOM0          (0x40003000) /**< \brief (SERCOM0) APB Base Address */
#define SERCOM1          (0x40003400) /**< \brief (SERCOM1) APB Base Address */
#define SERCOM2          (0x41012000) /**< \brief (SERCOM2) APB Base Address */
#define SERCOM3          (0x41014000) /**< \brief (SERCOM3) APB Base Address */
#define SERCOM4          (0x43000000) /**< \brief (SERCOM4) APB Base Address */
#define SERCOM5          (0x43000400) /**< \brief (SERCOM5) APB Base Address */
#define SERCOM6          (0x43000800) /**< \brief (SERCOM6) APB Base Address */
#define SERCOM7          (0x43000C00) /**< \brief (SERCOM7) APB Base Address */
#define SUPC             (0x40001800) /**< \brief (SUPC) APB Base Address */
#define TC0              (0x40003800) /**< \brief (TC0) APB Base Address */
#define TC1              (0x40003C00) /**< \brief (TC1) APB Base Address */
#define TC2              (0x4101A000) /**< \brief (TC2) APB Base Address */
#define TC3              (0x4101C000) /**< \brief (TC3) APB Base Address */
#define TC4              (0x42001400) /**< \brief (TC4) APB Base Address */
#define TC5              (0x42001800) /**< \brief (TC5) APB Base Address */
#define TC6              (0x43001400) /**< \brief (TC6) APB Base Address */
#define TC7              (0x43001800) /**< \brief (TC7) APB Base Address */
#define TCC0             (0x41016000) /**< \brief (TCC0) APB Base Address */
#define TCC1             (0x41018000) /**< \brief (TCC1) APB Base Address */
#define TCC2             (0x42000C00) /**< \brief (TCC2) APB Base Address */
#define TCC3             (0x42001000) /**< \brief (TCC3) APB Base Address */
#define TCC4             (0x43001000) /**< \brief (TCC4) APB Base Address */
#define TRNG             (0x42002800) /**< \brief (TRNG) APB Base Address */
#define USB              (0x41000000) /**< \brief (USB) APB Base Address */
#define WDT              (0x40002000) /**< \brief (WDT) APB Base Address */

#else /* #if defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__) */

#define AC          ((Ac *)0x42002000UL) /**< \brief (AC) APB Base Address */
#define AC_INST_NUM 1                    /**< \brief (AC) Number of instances */
#define AC_INSTS    {AC}                 /**< \brief (AC) Instances List */

#define ADC0         ((Adc *)0x43001C00UL) /**< \brief (ADC0) APB Base Address */
#define ADC1         ((Adc *)0x43002000UL) /**< \brief (ADC1) APB Base Address */
#define ADC_INST_NUM 2                     /**< \brief (ADC) Number of instances */
#define ADC_INSTS    {ADC0, ADC1}          /**< \brief (ADC) Instances List */

#define AES          ((Aes *)0x42002400UL) /**< \brief (AES) APB Base Address */
#define AES_INST_NUM 1                     /**< \brief (AES) Number of instances */
#define AES_INSTS    {AES}                 /**< \brief (AES) Instances List */

#define CAN0         ((Can *)0x42000000UL) /**< \brief (CAN0) APB Base Address */
#define CAN1         ((Can *)0x42000400UL) /**< \brief (CAN1) APB Base Address */
#define CAN_INST_NUM 2                     /**< \brief (CAN) Number of instances */
#define CAN_INSTS    {CAN0, CAN1}          /**< \brief (CAN) Instances List */

#define CCL          ((Ccl *)0x42003800UL) /**< \brief (CCL) APB Base Address */
#define CCL_INST_NUM 1                     /**< \brief (CCL) Number of instances */
#define CCL_INSTS    {CCL}                 /**< \brief (CCL) Instances List */

#define CMCC          ((Cmcc *)0x41006000UL) /**< \brief (CMCC) APB Base Address */
#define CMCC_AHB      (0x03000000UL)         /**< \brief (CMCC) AHB Base Address */
#define CMCC_INST_NUM 1                      /**< \brief (CMCC) Number of instances */
#define CMCC_INSTS    {CMCC}                 /**< \brief (CMCC) Instances List */

#define DAC          ((Dac *)0x43002400UL) /**< \brief (DAC) APB Base Address */
#define DAC_INST_NUM 1                     /**< \brief (DAC) Number of instances */
#define DAC_INSTS    {DAC}                 /**< \brief (DAC) Instances List */

#define DMAC          ((Dmac *)0x4100A000UL) /**< \brief (DMAC) APB Base Address */
#define DMAC_INST_NUM 1                      /**< \brief (DMAC) Number of instances */
#define DMAC_INSTS    {DMAC}                 /**< \brief (DMAC) Instances List */

#define DSU          ((Dsu *)0x41002000UL) /**< \brief (DSU) APB Base Address */
#define DSU_INST_NUM 1                     /**< \brief (DSU) Number of instances */
#define DSU_INSTS    {DSU}                 /**< \brief (DSU) Instances List */

#define EIC          ((Eic *)0x40002800UL) /**< \brief (EIC) APB Base Address */
#define EIC_INST_NUM 1                     /**< \brief (EIC) Number of instances */
#define EIC_INSTS    {EIC}                 /**< \brief (EIC) Instances List */

#define EVSYS          ((Evsys *)0x4100E000UL) /**< \brief (EVSYS) APB Base Address */
#define EVSYS_INST_NUM 1                       /**< \brief (EVSYS) Number of instances */
#define EVSYS_INSTS    {EVSYS}                 /**< \brief (EVSYS) Instances List */

#define FREQM          ((Freqm *)0x40002C00UL) /**< \brief (FREQM) APB Base Address */
#define FREQM_INST_NUM 1                       /**< \brief (FREQM) Number of instances */
#define FREQM_INSTS    {FREQM}                 /**< \brief (FREQM) Instances List */

#define GCLK          ((Gclk *)0x40001C00UL) /**< \brief (GCLK) APB Base Address */
#define GCLK_INST_NUM 1                      /**< \brief (GCLK) Number of instances */
#define GCLK_INSTS    {GCLK}                 /**< \brief (GCLK) Instances List */

#define GMAC          ((Gmac *)0x42000800UL) /**< \brief (GMAC) APB Base Address */
#define GMAC_INST_NUM 1                      /**< \brief (GMAC) Number of instances */
#define GMAC_INSTS    {GMAC}                 /**< \brief (GMAC) Instances List */

#define HMATRIX           ((Hmatrixb *)0x4100C000UL) /**< \brief (HMATRIX) APB Base Address */
#define HMATRIXB_INST_NUM 1                          /**< \brief (HMATRIXB) Number of instances */
#define HMATRIXB_INSTS    {HMATRIX}                  /**< \brief (HMATRIXB) Instances List */

#define ICM          ((Icm *)0x42002C00UL) /**< \brief (ICM) APB Base Address */
#define ICM_INST_NUM 1                     /**< \brief (ICM) Number of instances */
#define ICM_INSTS    {ICM}                 /**< \brief (ICM) Instances List */

#define I2S          ((I2s *)0x43002800UL) /**< \brief (I2S) APB Base Address */
#define I2S_INST_NUM 1                     /**< \brief (I2S) Number of instances */
#define I2S_INSTS    {I2S}                 /**< \brief (I2S) Instances List */

#define MCLK          ((Mclk *)0x40000800UL) /**< \brief (MCLK) APB Base Address */
#define MCLK_INST_NUM 1                      /**< \brief (MCLK) Number of instances */
#define MCLK_INSTS    {MCLK}                 /**< \brief (MCLK) Instances List */

#define NVMCTRL          ((Nvmctrl *)0x41004000UL) /**< \brief (NVMCTRL) APB Base Address */
#define NVMCTRL_SW0      (0x00800080UL)            /**< \brief (NVMCTRL) SW0 Base Address */
#define NVMCTRL_TEMP_LOG (0x00800100UL)            /**< \brief (NVMCTRL) TEMP_LOG Base Address */
#define NVMCTRL_USER     (0x00804000UL)            /**< \brief (NVMCTRL) USER Base Address */
#define NVMCTRL_INST_NUM 1                         /**< \brief (NVMCTRL) Number of instances */
#define NVMCTRL_INSTS    {NVMCTRL}                 /**< \brief (NVMCTRL) Instances List */

#define OSCCTRL          ((Oscctrl *)0x40001000UL) /**< \brief (OSCCTRL) APB Base Address */
#define OSCCTRL_INST_NUM 1                         /**< \brief (OSCCTRL) Number of instances */
#define OSCCTRL_INSTS    {OSCCTRL}                 /**< \brief (OSCCTRL) Instances List */

#define OSC32KCTRL          ((Osc32kctrl *)0x40001400UL) /**< \brief (OSC32KCTRL) APB Base */
#define OSC32KCTRL_INST_NUM 1            /**< \brief (OSC32KCTRL) Num of instances */
#define OSC32KCTRL_INSTS    {OSC32KCTRL} /**< \brief (OSC32KCTRL) Instances List */

#define PAC          ((Pac *)0x40000000UL) /**< \brief (PAC) APB Base Address */
#define PAC_INST_NUM 1                     /**< \brief (PAC) Number of instances */
#define PAC_INSTS    {PAC}                 /**< \brief (PAC) Instances List */

#define PCC          ((Pcc *)0x43002C00UL) /**< \brief (PCC) APB Base Address */
#define PCC_INST_NUM 1                     /**< \brief (PCC) Number of instances */
#define PCC_INSTS    {PCC}                 /**< \brief (PCC) Instances List */

#define PDEC          ((Pdec *)0x42001C00UL) /**< \brief (PDEC) APB Base Address */
#define PDEC_INST_NUM 1                      /**< \brief (PDEC) Number of instances */
#define PDEC_INSTS    {PDEC}                 /**< \brief (PDEC) Instances List */

#define PM          ((Pm *)0x40000400UL) /**< \brief (PM) APB Base Address */
#define PM_INST_NUM 1                    /**< \brief (PM) Number of instances */
#define PM_INSTS    {PM}                 /**< \brief (PM) Instances List */

#define PORT          ((Port *)0x41008000UL) /**< \brief (PORT) APB Base Address */
#define PORT_INST_NUM 1                      /**< \brief (PORT) Number of instances */
#define PORT_INSTS    {PORT}                 /**< \brief (PORT) Instances List */

#define PUKCC          ((void *)0x42003000UL) /**< \brief (PUKCC) APB Base Address */
#define PUKCC_AHB      ((void *)0x02000000UL) /**< \brief (PUKCC) AHB Base Address */
#define PUKCC_INST_NUM 1                      /**< \brief (PUKCC) Number of instances */
#define PUKCC_INSTS    {PUKCC}                /**< \brief (PUKCC) Instances List */

#define QSPI          ((Qspi *)0x42003400UL) /**< \brief (QSPI) APB Base Address */
#define QSPI_AHB      (0x04000000UL)         /**< \brief (QSPI) AHB Base Address */
#define QSPI_INST_NUM 1                      /**< \brief (QSPI) Number of instances */
#define QSPI_INSTS    {QSPI}                 /**< \brief (QSPI) Instances List */

#define RAMECC          ((Ramecc *)0x41020000UL) /**< \brief (RAMECC) APB Base Address */
#define RAMECC_INST_NUM 1                        /**< \brief (RAMECC) Number of instances */
#define RAMECC_INSTS    {RAMECC}                 /**< \brief (RAMECC) Instances List */

#define RSTC          ((Rstc *)0x40000C00UL) /**< \brief (RSTC) APB Base Address */
#define RSTC_INST_NUM 1                      /**< \brief (RSTC) Number of instances */
#define RSTC_INSTS    {RSTC}                 /**< \brief (RSTC) Instances List */

#define RTC          ((Rtc *)0x40002400UL) /**< \brief (RTC) APB Base Address */
#define RTC_INST_NUM 1                     /**< \brief (RTC) Number of instances */
#define RTC_INSTS    {RTC}                 /**< \brief (RTC) Instances List */

#define SDHC0         ((Sdhc *)0x45000000UL) /**< \brief (SDHC0) AHB Base Address */
#define SDHC1         ((Sdhc *)0x46000000UL) /**< \brief (SDHC1) AHB Base Address */
#define SDHC_INST_NUM 2                      /**< \brief (SDHC) Number of instances */
#define SDHC_INSTS    {SDHC0, SDHC1}         /**< \brief (SDHC) Instances List */

#define SERCOM0         ((Sercom *)0x40003000UL) /**< \brief (SERCOM0) APB Base Address */
#define SERCOM1         ((Sercom *)0x40003400UL) /**< \brief (SERCOM1) APB Base Address */
#define SERCOM2         ((Sercom *)0x41012000UL) /**< \brief (SERCOM2) APB Base Address */
#define SERCOM3         ((Sercom *)0x41014000UL) /**< \brief (SERCOM3) APB Base Address */
#define SERCOM4         ((Sercom *)0x43000000UL) /**< \brief (SERCOM4) APB Base Address */
#define SERCOM5         ((Sercom *)0x43000400UL) /**< \brief (SERCOM5) APB Base Address */
#define SERCOM6         ((Sercom *)0x43000800UL) /**< \brief (SERCOM6) APB Base Address */
#define SERCOM7         ((Sercom *)0x43000C00UL) /**< \brief (SERCOM7) APB Base Address */
#define SERCOM_INST_NUM 8                        /**< \brief (SERCOM) Number of instances */
/**< \brief (SERCOM) Instances List */
#define SERCOM_INSTS    {SERCOM0, SERCOM1, SERCOM2, SERCOM3, SERCOM4, SERCOM5, SERCOM6, SERCOM7}

#define SUPC          ((Supc *)0x40001800UL) /**< \brief (SUPC) APB Base Address */
#define SUPC_INST_NUM 1                      /**< \brief (SUPC) Number of instances */
#define SUPC_INSTS    {SUPC}                 /**< \brief (SUPC) Instances List */

#define TC0         ((Tc *)0x40003800UL) /**< \brief (TC0) APB Base Address */
#define TC1         ((Tc *)0x40003C00UL) /**< \brief (TC1) APB Base Address */
#define TC2         ((Tc *)0x4101A000UL) /**< \brief (TC2) APB Base Address */
#define TC3         ((Tc *)0x4101C000UL) /**< \brief (TC3) APB Base Address */
#define TC4         ((Tc *)0x42001400UL) /**< \brief (TC4) APB Base Address */
#define TC5         ((Tc *)0x42001800UL) /**< \brief (TC5) APB Base Address */
#define TC6         ((Tc *)0x43001400UL) /**< \brief (TC6) APB Base Address */
#define TC7         ((Tc *)0x43001800UL) /**< \brief (TC7) APB Base Address */
#define TC_INST_NUM 8                    /**< \brief (TC) Number of instances */
/**< \brief (TC) Instances List */
#define TC_INSTS    {TC0, TC1, TC2, TC3, TC4, TC5, TC6, TC7}

#define TCC0         ((Tcc *)0x41016000UL)          /**< \brief (TCC0) APB Base Address */
#define TCC1         ((Tcc *)0x41018000UL)          /**< \brief (TCC1) APB Base Address */
#define TCC2         ((Tcc *)0x42000C00UL)          /**< \brief (TCC2) APB Base Address */
#define TCC3         ((Tcc *)0x42001000UL)          /**< \brief (TCC3) APB Base Address */
#define TCC4         ((Tcc *)0x43001000UL)          /**< \brief (TCC4) APB Base Address */
#define TCC_INST_NUM 5                              /**< \brief (TCC) Number of instances */
#define TCC_INSTS    {TCC0, TCC1, TCC2, TCC3, TCC4} /**< \brief (TCC) Instances List */

#define TRNG          ((Trng *)0x42002800UL) /**< \brief (TRNG) APB Base Address */
#define TRNG_INST_NUM 1                      /**< \brief (TRNG) Number of instances */
#define TRNG_INSTS    {TRNG}                 /**< \brief (TRNG) Instances List */

#define USB          ((Usb *)0x41000000UL) /**< \brief (USB) APB Base Address */
#define USB_INST_NUM 1                     /**< \brief (USB) Number of instances */
#define USB_INSTS    {USB}                 /**< \brief (USB) Instances List */

#define WDT          ((Wdt *)0x40002000UL) /**< \brief (WDT) APB Base Address */
#define WDT_INST_NUM 1                     /**< \brief (WDT) Number of instances */
#define WDT_INSTS    {WDT}                 /**< \brief (WDT) Instances List */

#endif /* (defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */
/*@}*/

#include <pic32cxsg_fixups.h>

/* #include "../common/soc_port.h" */
#include "../common/microchip_pic32cxsg_dt.h"

#define SOC_MICROCHIP_PIC32CXSG_OSC32K_FREQ_HZ 32768
#define SOC_MICROCHIP_PIC32CXSG_DFLL48_FREQ_HZ 48000000

/** Processor Clock (HCLK) Frequency */
#define SOC_MICROCHIP_PIC32CXSG_HCLK_FREQ_HZ  CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC
/** Master Clock (MCK) Frequency */
#define SOC_MICROCHIP_PIC32CXSG_MCK_FREQ_HZ   SOC_MICROCHIP_PIC32CXSG_HCLK_FREQ_HZ
#define SOC_MICROCHIP_PIC32CXSG_GCLK0_FREQ_HZ SOC_MICROCHIP_PIC32CXSG_MCK_FREQ_HZ
#define SOC_MICROCHIP_PIC32CXSG_GCLK2_FREQ_HZ 48000000

#define SOC_ATMEL_SAM0_HCLK_FREQ_HZ  CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC
#define SOC_ATMEL_SAM0_MCK_FREQ_HZ   SOC_MICROCHIP_PIC32CXSG_HCLK_FREQ_HZ
#define SOC_ATMEL_SAM0_GCLK0_FREQ_HZ SOC_MICROCHIP_PIC32CXSG_MCK_FREQ_HZ
#define SOC_ATMEL_SAM0_GCLK2_FREQ_HZ 48000000

#ifdef MCLK_APBAMASK_EIC
#undef MCLK_APBAMASK_EIC
#endif
#define MCLK_APBAMASK_EIC ((uint32_t)(0x1) << MCLK_APBAMASK_EIC_Pos)

#ifdef GCLK_PCHCTRL_CHEN
#undef GCLK_PCHCTRL_CHEN
#endif
#define GCLK_PCHCTRL_CHEN ((uint32_t)(0x1) << GCLK_PCHCTRL_CHEN_Pos)

#ifdef MCLK_APBBMASK_NVMCTRL
#undef MCLK_APBBMASK_NVMCTRL
#endif
#define MCLK_APBBMASK_NVMCTRL ((uint32_t)(0x1) << MCLK_APBBMASK_NVMCTRL_Pos)

#ifdef USB_DEVICE_EPINTFLAG_RXSTP
#undef USB_DEVICE_EPINTFLAG_RXSTP
#endif
#define USB_DEVICE_EPINTFLAG_RXSTP ((uint8_t)(0x1) << USB_DEVICE_EPINTFLAG_RXSTP_Pos)

#ifdef ADC_REFCTRL_REFCOMP
#undef ADC_REFCTRL_REFCOMP
#endif
#define ADC_REFCTRL_REFCOMP ((uint8_t)(0x1) << ADC_REFCTRL_REFCOMP_Pos)

#ifdef ADC_INPUTCTRL_DIFFMODE
#undef ADC_INPUTCTRL_DIFFMODE
#endif
#define ADC_INPUTCTRL_DIFFMODE ((uint16_t)(0x1) << ADC_INPUTCTRL_DIFFMODE_Pos)

#ifdef ADC_INPUTCTRL_MUXNEG_GND
#undef ADC_INPUTCTRL_MUXNEG_GND
#endif
#define ADC_INPUTCTRL_MUXNEG_GND (ADC_INPUTCTRL_MUXNEG_AVSS_Val << ADC_INPUTCTRL_MUXNEG_Pos)

#ifdef ADC_SWTRIG_START
#undef ADC_SWTRIG_START
#endif
#define ADC_SWTRIG_START ((uint8_t)(0x1) << ADC_SWTRIG_START_Pos)

#ifdef ADC_INTFLAG_MASK
#undef ADC_INTFLAG_MASK
#endif
#define ADC_INTFLAG_MASK ((uint8_t)ADC_INTFLAG_Msk)

#ifdef ADC_INTFLAG_MASK
#undef ADC_INTFLAG_MASK
#endif
#define ADC_INTFLAG_MASK ((uint8_t)ADC_INTADC_INTENCLR_MASKFLAG_Msk)

#ifdef ADC_INTENCLR_MASK
#undef ADC_INTENCLR_MASK
#endif
#define ADC_INTENCLR_MASK ((uint8_t)ADC_INTENCLR_Msk)

#ifdef ADC_INTENSET_RESRDY
#undef ADC_INTENSET_RESRDY
#endif
#define ADC_INTENSET_RESRDY ((uint8_t)(0x1) << ADC_INTENSET_RESRDY_Pos)

#endif /* _SOC_MICROCHIP_PIC32CXSG41_SOC_H_ */
