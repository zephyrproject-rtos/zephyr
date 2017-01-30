/*
** ###################################################################
**     Processors:          MKW41Z256VHT4
**                          MKW41Z512VHT4
**
**     Compilers:           Keil ARM C/C++ Compiler
**                          GNU C Compiler
**                          IAR ANSI C/C++ Compiler for ARM
**
**     Reference manual:    MKW41Z512RM Rev. 0.1, 04/2016
**     Version:             rev. 1.0, 2015-09-23
**     Build:               b160720
**
**     Abstract:
**         CMSIS Peripheral Access Layer for MKW41Z4
**
**     Copyright (c) 1997 - 2016 Freescale Semiconductor, Inc.
**     All rights reserved.
**
**     Redistribution and use in source and binary forms, with or without modification,
**     are permitted provided that the following conditions are met:
**
**     o Redistributions of source code must retain the above copyright notice, this list
**       of conditions and the following disclaimer.
**
**     o Redistributions in binary form must reproduce the above copyright notice, this
**       list of conditions and the following disclaimer in the documentation and/or
**       other materials provided with the distribution.
**
**     o Neither the name of Freescale Semiconductor, Inc. nor the names of its
**       contributors may be used to endorse or promote products derived from this
**       software without specific prior written permission.
**
**     THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
**     ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
**     WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
**     DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
**     ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
**     (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
**     LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
**     ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
**     (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
**     SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
**
**     http:                 www.freescale.com
**     mail:                 support@freescale.com
**
**     Revisions:
**     - rev. 1.0 (2015-09-23)
**         Initial version.
**
** ###################################################################
*/

/*!
 * @file MKW41Z4.h
 * @version 1.0
 * @date 2015-09-23
 * @brief CMSIS Peripheral Access Layer for MKW41Z4
 *
 * CMSIS Peripheral Access Layer for MKW41Z4
 */

#ifndef _MKW41Z4_H_
#define _MKW41Z4_H_                              /**< Symbol preventing repeated inclusion */

/** Memory map major version (memory maps with equal major version number are
 * compatible) */
#define MCU_MEM_MAP_VERSION 0x0100U
/** Memory map minor version */
#define MCU_MEM_MAP_VERSION_MINOR 0x0000U


/* ----------------------------------------------------------------------------
   -- Interrupt vector numbers
   ---------------------------------------------------------------------------- */

/*!
 * @addtogroup Interrupt_vector_numbers Interrupt vector numbers
 * @{
 */

/** Interrupt Number Definitions */
#define NUMBER_OF_INT_VECTORS 48                 /**< Number of interrupts in the Vector table */

typedef enum IRQn {
  /* Auxiliary constants */
  NotAvail_IRQn                = -128,             /**< Not available device specific interrupt */

  /* Core interrupts */
  NonMaskableInt_IRQn          = -14,              /**< Non Maskable Interrupt */
  HardFault_IRQn               = -13,              /**< Cortex-M0 SV Hard Fault Interrupt */
  SVCall_IRQn                  = -5,               /**< Cortex-M0 SV Call Interrupt */
  PendSV_IRQn                  = -2,               /**< Cortex-M0 Pend SV Interrupt */
  SysTick_IRQn                 = -1,               /**< Cortex-M0 System Tick Interrupt */

  /* Device specific interrupts */
  DMA0_IRQn                    = 0,                /**< DMA channel 0 transfer complete */
  DMA1_IRQn                    = 1,                /**< DMA channel 1 transfer complete */
  DMA2_IRQn                    = 2,                /**< DMA channel 2 transfer complete */
  DMA3_IRQn                    = 3,                /**< DMA channel 3 transfer complete */
  Reserved20_IRQn              = 4,                /**< Reserved interrupt */
  FTFA_IRQn                    = 5,                /**< Command complete and read collision */
  LVD_LVW_DCDC_IRQn            = 6,                /**< Low-voltage detect, low-voltage warning, DCDC */
  LLWU_IRQn                    = 7,                /**< Low leakage wakeup Unit */
  I2C0_IRQn                    = 8,                /**< I2C0 interrupt */
  I2C1_IRQn                    = 9,                /**< I2C1 interrupt */
  SPI0_IRQn                    = 10,               /**< SPI0 single interrupt vector for all sources */
  TSI0_IRQn                    = 11,               /**< TSI0 single interrupt vector for all sources */
  LPUART0_IRQn                 = 12,               /**< LPUART0 status and error */
  TRNG0_IRQn                   = 13,               /**< TRNG0 interrupt */
  CMT_IRQn                     = 14,               /**< CMT interrupt */
  ADC0_IRQn                    = 15,               /**< ADC0 interrupt */
  CMP0_IRQn                    = 16,               /**< CMP0 interrupt */
  TPM0_IRQn                    = 17,               /**< TPM0 single interrupt vector for all sources */
  TPM1_IRQn                    = 18,               /**< TPM1 single interrupt vector for all sources */
  TPM2_IRQn                    = 19,               /**< TPM2 single interrupt vector for all sources */
  RTC_IRQn                     = 20,               /**< RTC alarm */
  RTC_Seconds_IRQn             = 21,               /**< RTC seconds */
  PIT_IRQn                     = 22,               /**< PIT interrupt */
  LTC0_IRQn                    = 23,               /**< LTC0 interrupt */
  Radio_0_IRQn                 = 24,               /**< BTLE, ZIGBEE, ANT, GENFSK interrupt 0 */
  DAC0_IRQn                    = 25,               /**< DAC0 interrupt */
  Radio_1_IRQn                 = 26,               /**< BTLE, ZIGBEE, ANT, GENFSK interrupt 1 */
  MCG_IRQn                     = 27,               /**< MCG interrupt */
  LPTMR0_IRQn                  = 28,               /**< LPTMR0 interrupt */
  SPI1_IRQn                    = 29,               /**< SPI1 single interrupt vector for all sources */
  PORTA_IRQn                   = 30,               /**< PORTA Pin detect */
  PORTB_PORTC_IRQn             = 31                /**< PORTB and PORTC Pin detect */
} IRQn_Type;

/*!
 * @}
 */ /* end of group Interrupt_vector_numbers */


/* ----------------------------------------------------------------------------
   -- Cortex M0 Core Configuration
   ---------------------------------------------------------------------------- */

/*!
 * @addtogroup Cortex_Core_Configuration Cortex M0 Core Configuration
 * @{
 */

#define __CM0PLUS_REV                  0x0000    /**< Core revision r0p0 */
#define __MPU_PRESENT                  0         /**< Defines if an MPU is present or not */
#define __VTOR_PRESENT                 1         /**< Defines if VTOR is present or not */
#define __NVIC_PRIO_BITS               2         /**< Number of priority bits implemented in the NVIC */
#define __Vendor_SysTickConfig         0         /**< Vendor specific implementation of SysTickConfig is defined */

#include "core_cm0plus.h"              /* Core Peripheral Access Layer */
#include "system_MKW41Z4.h"            /* Device specific configuration file */

/*!
 * @}
 */ /* end of group Cortex_Core_Configuration */


/* ----------------------------------------------------------------------------
   -- Mapping Information
   ---------------------------------------------------------------------------- */

/*!
 * @addtogroup Mapping_Information Mapping Information
 * @{
 */

/** Mapping Information */
/*!
 * @addtogroup edma_request
 * @{
 */

/*******************************************************************************
 * Definitions
 ******************************************************************************/

/*!
 * @brief Structure for the DMA hardware request
 *
 * Defines the structure for the DMA hardware request collections. The user can configure the
 * hardware request into DMAMUX to trigger the DMA transfer accordingly. The index
 * of the hardware request varies according  to the to SoC.
 */
typedef enum _dma_request_source
{
    kDmaRequestMux0Disable          = 0|0x100U,    /**< DMAMUX TriggerDisabled. */
    kDmaRequestMux0Reserved1        = 1|0x100U,    /**< Reserved1 */
    kDmaRequestMux0LPUART0Rx        = 2|0x100U,    /**< LPUART0 Receive. */
    kDmaRequestMux0LPUART0Tx        = 3|0x100U,    /**< LPUART0 Transmit. */
    kDmaRequestMux0Reserved4        = 4|0x100U,    /**< Reserved4 */
    kDmaRequestMux0Reserved5        = 5|0x100U,    /**< Reserved5 */
    kDmaRequestMux0Reserved6        = 6|0x100U,    /**< Reserved6 */
    kDmaRequestMux0Reserved7        = 7|0x100U,    /**< Reserved7 */
    kDmaRequestMux0Reserved8        = 8|0x100U,    /**< Reserved8 */
    kDmaRequestMux0Reserved9        = 9|0x100U,    /**< Reserved9 */
    kDmaRequestMux0Reserved10       = 10|0x100U,   /**< Reserved10 */
    kDmaRequestMux0Reserved11       = 11|0x100U,   /**< Reserved11 */
    kDmaRequestMux0Reserved12       = 12|0x100U,   /**< Reserved12 */
    kDmaRequestMux0Reserved13       = 13|0x100U,   /**< Reserved13 */
    kDmaRequestMux0Reserved14       = 14|0x100U,   /**< Reserved14 */
    kDmaRequestMux0Reserved15       = 15|0x100U,   /**< Reserved15 */
    kDmaRequestMux0SPI0Rx           = 16|0x100U,   /**< SPI0 Receive. */
    kDmaRequestMux0SPI0Tx           = 17|0x100U,   /**< SPI0 Transmit. */
    kDmaRequestMux0SPI1Rx           = 18|0x100U,   /**< SPI1 Receive. */
    kDmaRequestMux0SPI1Tx           = 19|0x100U,   /**< SPI1 Transmit. */
    kDmaRequestMux0LTC0InputFIFO    = 20|0x100U,   /**< LTC0 Input FIFO. */
    kDmaRequestMux0LTC0OutputFIFO   = 21|0x100U,   /**< LTC0 Output FIFO. */
    kDmaRequestMux0I2C0             = 22|0x100U,   /**< I2C0. */
    kDmaRequestMux0I2C1             = 23|0x100U,   /**< I2C1. */
    kDmaRequestMux0TPM0Channel0     = 24|0x100U,   /**< TPM0 C0V. */
    kDmaRequestMux0TPM0Channel1     = 25|0x100U,   /**< TPM0 C1V. */
    kDmaRequestMux0TPM0Channel2     = 26|0x100U,   /**< TPM0 C2V. */
    kDmaRequestMux0TPM0Channel3     = 27|0x100U,   /**< TPM0 C3V. */
    kDmaRequestMux0Reserved28       = 28|0x100U,   /**< Reserved28 */
    kDmaRequestMux0Reserved29       = 29|0x100U,   /**< Reserved29 */
    kDmaRequestMux0Reserved30       = 30|0x100U,   /**< Reserved30 */
    kDmaRequestMux0Reserved31       = 31|0x100U,   /**< Reserved31 */
    kDmaRequestMux0TPM1Channel0     = 32|0x100U,   /**< TPM1 C0V. */
    kDmaRequestMux0TPM1Channel1     = 33|0x100U,   /**< TPM1 C1V. */
    kDmaRequestMux0TPM2Channel0     = 34|0x100U,   /**< TPM2 C0V. */
    kDmaRequestMux0TPM2Channel1     = 35|0x100U,   /**< TPM2 C1V. */
    kDmaRequestMux0Reserved36       = 36|0x100U,   /**< Reserved36 */
    kDmaRequestMux0Reserved37       = 37|0x100U,   /**< Reserved37 */
    kDmaRequestMux0Reserved38       = 38|0x100U,   /**< Reserved38 */
    kDmaRequestMux0Reserved39       = 39|0x100U,   /**< Reserved39 */
    kDmaRequestMux0ADC0             = 40|0x100U,   /**< ADC0. */
    kDmaRequestMux0Reserved41       = 41|0x100U,   /**< Reserved41 */
    kDmaRequestMux0CMP0             = 42|0x100U,   /**< CMP0. */
    kDmaRequestMux0Reserved43       = 43|0x100U,   /**< Reserved43 */
    kDmaRequestMux0Reserved44       = 44|0x100U,   /**< Reserved44 */
    kDmaRequestMux0DAC0             = 45|0x100U,   /**< DAC0. */
    kDmaRequestMux0Reserved46       = 46|0x100U,   /**< Reserved46 */
    kDmaRequestMux0CMT              = 47|0x100U,   /**< CMT. */
    kDmaRequestMux0Reserved48       = 48|0x100U,   /**< Reserved48 */
    kDmaRequestMux0PortA            = 49|0x100U,   /**< PTA. */
    kDmaRequestMux0PortB            = 50|0x100U,   /**< PTB. */
    kDmaRequestMux0PortC            = 51|0x100U,   /**< PTC. */
    kDmaRequestMux0Reserved52       = 52|0x100U,   /**< Reserved52 */
    kDmaRequestMux0Reserved53       = 53|0x100U,   /**< Reserved53 */
    kDmaRequestMux0TPM0Overflow     = 54|0x100U,   /**< TPM0. */
    kDmaRequestMux0TPM1Overflow     = 55|0x100U,   /**< TPM1. */
    kDmaRequestMux0TPM2Overflow     = 56|0x100U,   /**< TPM2. */
    kDmaRequestMux0TSI0             = 57|0x100U,   /**< TSI0. */
    kDmaRequestMux0Reserved58       = 58|0x100U,   /**< Reserved58 */
    kDmaRequestMux0Reserved59       = 59|0x100U,   /**< Reserved59 */
    kDmaRequestMux0AlwaysOn60       = 60|0x100U,   /**< DMAMUX Always Enabled slot. */
    kDmaRequestMux0AlwaysOn61       = 61|0x100U,   /**< DMAMUX Always Enabled slot. */
    kDmaRequestMux0AlwaysOn62       = 62|0x100U,   /**< DMAMUX Always Enabled slot. */
    kDmaRequestMux0AlwaysOn63       = 63|0x100U,   /**< DMAMUX Always Enabled slot. */
} dma_request_source_t;

/* @} */


/*!
 * @}
 */ /* end of group Mapping_Information */


/* ----------------------------------------------------------------------------
   -- Device Peripheral Access Layer
   ---------------------------------------------------------------------------- */

/*!
 * @addtogroup Peripheral_access_layer Device Peripheral Access Layer
 * @{
 */


/*
** Start of section using anonymous unions
*/

#if defined(__ARMCC_VERSION)
  #pragma push
  #pragma anon_unions
#elif defined(__GNUC__)
  /* anonymous unions are enabled by default */
#elif defined(__IAR_SYSTEMS_ICC__)
  #pragma language=extended
#else
  #error Not supported compiler type
#endif

/* ----------------------------------------------------------------------------
   -- ADC Peripheral Access Layer
   ---------------------------------------------------------------------------- */

/*!
 * @addtogroup ADC_Peripheral_Access_Layer ADC Peripheral Access Layer
 * @{
 */

/** ADC - Register Layout Typedef */
typedef struct {
  __IO uint32_t SC1[2];                            /**< ADC Status and Control Registers 1, array offset: 0x0, array step: 0x4 */
  __IO uint32_t CFG1;                              /**< ADC Configuration Register 1, offset: 0x8 */
  __IO uint32_t CFG2;                              /**< ADC Configuration Register 2, offset: 0xC */
  __I  uint32_t R[2];                              /**< ADC Data Result Register, array offset: 0x10, array step: 0x4 */
  __IO uint32_t CV1;                               /**< Compare Value Registers, offset: 0x18 */
  __IO uint32_t CV2;                               /**< Compare Value Registers, offset: 0x1C */
  __IO uint32_t SC2;                               /**< Status and Control Register 2, offset: 0x20 */
  __IO uint32_t SC3;                               /**< Status and Control Register 3, offset: 0x24 */
  __IO uint32_t OFS;                               /**< ADC Offset Correction Register, offset: 0x28 */
  __IO uint32_t PG;                                /**< ADC Plus-Side Gain Register, offset: 0x2C */
  __IO uint32_t MG;                                /**< ADC Minus-Side Gain Register, offset: 0x30 */
  __IO uint32_t CLPD;                              /**< ADC Plus-Side General Calibration Value Register, offset: 0x34 */
  __IO uint32_t CLPS;                              /**< ADC Plus-Side General Calibration Value Register, offset: 0x38 */
  __IO uint32_t CLP4;                              /**< ADC Plus-Side General Calibration Value Register, offset: 0x3C */
  __IO uint32_t CLP3;                              /**< ADC Plus-Side General Calibration Value Register, offset: 0x40 */
  __IO uint32_t CLP2;                              /**< ADC Plus-Side General Calibration Value Register, offset: 0x44 */
  __IO uint32_t CLP1;                              /**< ADC Plus-Side General Calibration Value Register, offset: 0x48 */
  __IO uint32_t CLP0;                              /**< ADC Plus-Side General Calibration Value Register, offset: 0x4C */
       uint8_t RESERVED_0[4];
  __IO uint32_t CLMD;                              /**< ADC Minus-Side General Calibration Value Register, offset: 0x54 */
  __IO uint32_t CLMS;                              /**< ADC Minus-Side General Calibration Value Register, offset: 0x58 */
  __IO uint32_t CLM4;                              /**< ADC Minus-Side General Calibration Value Register, offset: 0x5C */
  __IO uint32_t CLM3;                              /**< ADC Minus-Side General Calibration Value Register, offset: 0x60 */
  __IO uint32_t CLM2;                              /**< ADC Minus-Side General Calibration Value Register, offset: 0x64 */
  __IO uint32_t CLM1;                              /**< ADC Minus-Side General Calibration Value Register, offset: 0x68 */
  __IO uint32_t CLM0;                              /**< ADC Minus-Side General Calibration Value Register, offset: 0x6C */
} ADC_Type;

/* ----------------------------------------------------------------------------
   -- ADC Register Masks
   ---------------------------------------------------------------------------- */

/*!
 * @addtogroup ADC_Register_Masks ADC Register Masks
 * @{
 */

/*! @name SC1 - ADC Status and Control Registers 1 */
#define ADC_SC1_ADCH_MASK                        (0x1FU)
#define ADC_SC1_ADCH_SHIFT                       (0U)
#define ADC_SC1_ADCH(x)                          (((uint32_t)(((uint32_t)(x)) << ADC_SC1_ADCH_SHIFT)) & ADC_SC1_ADCH_MASK)
#define ADC_SC1_DIFF_MASK                        (0x20U)
#define ADC_SC1_DIFF_SHIFT                       (5U)
#define ADC_SC1_DIFF(x)                          (((uint32_t)(((uint32_t)(x)) << ADC_SC1_DIFF_SHIFT)) & ADC_SC1_DIFF_MASK)
#define ADC_SC1_AIEN_MASK                        (0x40U)
#define ADC_SC1_AIEN_SHIFT                       (6U)
#define ADC_SC1_AIEN(x)                          (((uint32_t)(((uint32_t)(x)) << ADC_SC1_AIEN_SHIFT)) & ADC_SC1_AIEN_MASK)
#define ADC_SC1_COCO_MASK                        (0x80U)
#define ADC_SC1_COCO_SHIFT                       (7U)
#define ADC_SC1_COCO(x)                          (((uint32_t)(((uint32_t)(x)) << ADC_SC1_COCO_SHIFT)) & ADC_SC1_COCO_MASK)

/* The count of ADC_SC1 */
#define ADC_SC1_COUNT                            (2U)

/*! @name CFG1 - ADC Configuration Register 1 */
#define ADC_CFG1_ADICLK_MASK                     (0x3U)
#define ADC_CFG1_ADICLK_SHIFT                    (0U)
#define ADC_CFG1_ADICLK(x)                       (((uint32_t)(((uint32_t)(x)) << ADC_CFG1_ADICLK_SHIFT)) & ADC_CFG1_ADICLK_MASK)
#define ADC_CFG1_MODE_MASK                       (0xCU)
#define ADC_CFG1_MODE_SHIFT                      (2U)
#define ADC_CFG1_MODE(x)                         (((uint32_t)(((uint32_t)(x)) << ADC_CFG1_MODE_SHIFT)) & ADC_CFG1_MODE_MASK)
#define ADC_CFG1_ADLSMP_MASK                     (0x10U)
#define ADC_CFG1_ADLSMP_SHIFT                    (4U)
#define ADC_CFG1_ADLSMP(x)                       (((uint32_t)(((uint32_t)(x)) << ADC_CFG1_ADLSMP_SHIFT)) & ADC_CFG1_ADLSMP_MASK)
#define ADC_CFG1_ADIV_MASK                       (0x60U)
#define ADC_CFG1_ADIV_SHIFT                      (5U)
#define ADC_CFG1_ADIV(x)                         (((uint32_t)(((uint32_t)(x)) << ADC_CFG1_ADIV_SHIFT)) & ADC_CFG1_ADIV_MASK)
#define ADC_CFG1_ADLPC_MASK                      (0x80U)
#define ADC_CFG1_ADLPC_SHIFT                     (7U)
#define ADC_CFG1_ADLPC(x)                        (((uint32_t)(((uint32_t)(x)) << ADC_CFG1_ADLPC_SHIFT)) & ADC_CFG1_ADLPC_MASK)

/*! @name CFG2 - ADC Configuration Register 2 */
#define ADC_CFG2_ADLSTS_MASK                     (0x3U)
#define ADC_CFG2_ADLSTS_SHIFT                    (0U)
#define ADC_CFG2_ADLSTS(x)                       (((uint32_t)(((uint32_t)(x)) << ADC_CFG2_ADLSTS_SHIFT)) & ADC_CFG2_ADLSTS_MASK)
#define ADC_CFG2_ADHSC_MASK                      (0x4U)
#define ADC_CFG2_ADHSC_SHIFT                     (2U)
#define ADC_CFG2_ADHSC(x)                        (((uint32_t)(((uint32_t)(x)) << ADC_CFG2_ADHSC_SHIFT)) & ADC_CFG2_ADHSC_MASK)
#define ADC_CFG2_ADACKEN_MASK                    (0x8U)
#define ADC_CFG2_ADACKEN_SHIFT                   (3U)
#define ADC_CFG2_ADACKEN(x)                      (((uint32_t)(((uint32_t)(x)) << ADC_CFG2_ADACKEN_SHIFT)) & ADC_CFG2_ADACKEN_MASK)
#define ADC_CFG2_MUXSEL_MASK                     (0x10U)
#define ADC_CFG2_MUXSEL_SHIFT                    (4U)
#define ADC_CFG2_MUXSEL(x)                       (((uint32_t)(((uint32_t)(x)) << ADC_CFG2_MUXSEL_SHIFT)) & ADC_CFG2_MUXSEL_MASK)

/*! @name R - ADC Data Result Register */
#define ADC_R_D_MASK                             (0xFFFFU)
#define ADC_R_D_SHIFT                            (0U)
#define ADC_R_D(x)                               (((uint32_t)(((uint32_t)(x)) << ADC_R_D_SHIFT)) & ADC_R_D_MASK)

/* The count of ADC_R */
#define ADC_R_COUNT                              (2U)

/*! @name CV1 - Compare Value Registers */
#define ADC_CV1_CV_MASK                          (0xFFFFU)
#define ADC_CV1_CV_SHIFT                         (0U)
#define ADC_CV1_CV(x)                            (((uint32_t)(((uint32_t)(x)) << ADC_CV1_CV_SHIFT)) & ADC_CV1_CV_MASK)

/*! @name CV2 - Compare Value Registers */
#define ADC_CV2_CV_MASK                          (0xFFFFU)
#define ADC_CV2_CV_SHIFT                         (0U)
#define ADC_CV2_CV(x)                            (((uint32_t)(((uint32_t)(x)) << ADC_CV2_CV_SHIFT)) & ADC_CV2_CV_MASK)

/*! @name SC2 - Status and Control Register 2 */
#define ADC_SC2_REFSEL_MASK                      (0x3U)
#define ADC_SC2_REFSEL_SHIFT                     (0U)
#define ADC_SC2_REFSEL(x)                        (((uint32_t)(((uint32_t)(x)) << ADC_SC2_REFSEL_SHIFT)) & ADC_SC2_REFSEL_MASK)
#define ADC_SC2_DMAEN_MASK                       (0x4U)
#define ADC_SC2_DMAEN_SHIFT                      (2U)
#define ADC_SC2_DMAEN(x)                         (((uint32_t)(((uint32_t)(x)) << ADC_SC2_DMAEN_SHIFT)) & ADC_SC2_DMAEN_MASK)
#define ADC_SC2_ACREN_MASK                       (0x8U)
#define ADC_SC2_ACREN_SHIFT                      (3U)
#define ADC_SC2_ACREN(x)                         (((uint32_t)(((uint32_t)(x)) << ADC_SC2_ACREN_SHIFT)) & ADC_SC2_ACREN_MASK)
#define ADC_SC2_ACFGT_MASK                       (0x10U)
#define ADC_SC2_ACFGT_SHIFT                      (4U)
#define ADC_SC2_ACFGT(x)                         (((uint32_t)(((uint32_t)(x)) << ADC_SC2_ACFGT_SHIFT)) & ADC_SC2_ACFGT_MASK)
#define ADC_SC2_ACFE_MASK                        (0x20U)
#define ADC_SC2_ACFE_SHIFT                       (5U)
#define ADC_SC2_ACFE(x)                          (((uint32_t)(((uint32_t)(x)) << ADC_SC2_ACFE_SHIFT)) & ADC_SC2_ACFE_MASK)
#define ADC_SC2_ADTRG_MASK                       (0x40U)
#define ADC_SC2_ADTRG_SHIFT                      (6U)
#define ADC_SC2_ADTRG(x)                         (((uint32_t)(((uint32_t)(x)) << ADC_SC2_ADTRG_SHIFT)) & ADC_SC2_ADTRG_MASK)
#define ADC_SC2_ADACT_MASK                       (0x80U)
#define ADC_SC2_ADACT_SHIFT                      (7U)
#define ADC_SC2_ADACT(x)                         (((uint32_t)(((uint32_t)(x)) << ADC_SC2_ADACT_SHIFT)) & ADC_SC2_ADACT_MASK)

/*! @name SC3 - Status and Control Register 3 */
#define ADC_SC3_AVGS_MASK                        (0x3U)
#define ADC_SC3_AVGS_SHIFT                       (0U)
#define ADC_SC3_AVGS(x)                          (((uint32_t)(((uint32_t)(x)) << ADC_SC3_AVGS_SHIFT)) & ADC_SC3_AVGS_MASK)
#define ADC_SC3_AVGE_MASK                        (0x4U)
#define ADC_SC3_AVGE_SHIFT                       (2U)
#define ADC_SC3_AVGE(x)                          (((uint32_t)(((uint32_t)(x)) << ADC_SC3_AVGE_SHIFT)) & ADC_SC3_AVGE_MASK)
#define ADC_SC3_ADCO_MASK                        (0x8U)
#define ADC_SC3_ADCO_SHIFT                       (3U)
#define ADC_SC3_ADCO(x)                          (((uint32_t)(((uint32_t)(x)) << ADC_SC3_ADCO_SHIFT)) & ADC_SC3_ADCO_MASK)
#define ADC_SC3_CALF_MASK                        (0x40U)
#define ADC_SC3_CALF_SHIFT                       (6U)
#define ADC_SC3_CALF(x)                          (((uint32_t)(((uint32_t)(x)) << ADC_SC3_CALF_SHIFT)) & ADC_SC3_CALF_MASK)
#define ADC_SC3_CAL_MASK                         (0x80U)
#define ADC_SC3_CAL_SHIFT                        (7U)
#define ADC_SC3_CAL(x)                           (((uint32_t)(((uint32_t)(x)) << ADC_SC3_CAL_SHIFT)) & ADC_SC3_CAL_MASK)

/*! @name OFS - ADC Offset Correction Register */
#define ADC_OFS_OFS_MASK                         (0xFFFFU)
#define ADC_OFS_OFS_SHIFT                        (0U)
#define ADC_OFS_OFS(x)                           (((uint32_t)(((uint32_t)(x)) << ADC_OFS_OFS_SHIFT)) & ADC_OFS_OFS_MASK)

/*! @name PG - ADC Plus-Side Gain Register */
#define ADC_PG_PG_MASK                           (0xFFFFU)
#define ADC_PG_PG_SHIFT                          (0U)
#define ADC_PG_PG(x)                             (((uint32_t)(((uint32_t)(x)) << ADC_PG_PG_SHIFT)) & ADC_PG_PG_MASK)

/*! @name MG - ADC Minus-Side Gain Register */
#define ADC_MG_MG_MASK                           (0xFFFFU)
#define ADC_MG_MG_SHIFT                          (0U)
#define ADC_MG_MG(x)                             (((uint32_t)(((uint32_t)(x)) << ADC_MG_MG_SHIFT)) & ADC_MG_MG_MASK)

/*! @name CLPD - ADC Plus-Side General Calibration Value Register */
#define ADC_CLPD_CLPD_MASK                       (0x3FU)
#define ADC_CLPD_CLPD_SHIFT                      (0U)
#define ADC_CLPD_CLPD(x)                         (((uint32_t)(((uint32_t)(x)) << ADC_CLPD_CLPD_SHIFT)) & ADC_CLPD_CLPD_MASK)

/*! @name CLPS - ADC Plus-Side General Calibration Value Register */
#define ADC_CLPS_CLPS_MASK                       (0x3FU)
#define ADC_CLPS_CLPS_SHIFT                      (0U)
#define ADC_CLPS_CLPS(x)                         (((uint32_t)(((uint32_t)(x)) << ADC_CLPS_CLPS_SHIFT)) & ADC_CLPS_CLPS_MASK)

/*! @name CLP4 - ADC Plus-Side General Calibration Value Register */
#define ADC_CLP4_CLP4_MASK                       (0x3FFU)
#define ADC_CLP4_CLP4_SHIFT                      (0U)
#define ADC_CLP4_CLP4(x)                         (((uint32_t)(((uint32_t)(x)) << ADC_CLP4_CLP4_SHIFT)) & ADC_CLP4_CLP4_MASK)

/*! @name CLP3 - ADC Plus-Side General Calibration Value Register */
#define ADC_CLP3_CLP3_MASK                       (0x1FFU)
#define ADC_CLP3_CLP3_SHIFT                      (0U)
#define ADC_CLP3_CLP3(x)                         (((uint32_t)(((uint32_t)(x)) << ADC_CLP3_CLP3_SHIFT)) & ADC_CLP3_CLP3_MASK)

/*! @name CLP2 - ADC Plus-Side General Calibration Value Register */
#define ADC_CLP2_CLP2_MASK                       (0xFFU)
#define ADC_CLP2_CLP2_SHIFT                      (0U)
#define ADC_CLP2_CLP2(x)                         (((uint32_t)(((uint32_t)(x)) << ADC_CLP2_CLP2_SHIFT)) & ADC_CLP2_CLP2_MASK)

/*! @name CLP1 - ADC Plus-Side General Calibration Value Register */
#define ADC_CLP1_CLP1_MASK                       (0x7FU)
#define ADC_CLP1_CLP1_SHIFT                      (0U)
#define ADC_CLP1_CLP1(x)                         (((uint32_t)(((uint32_t)(x)) << ADC_CLP1_CLP1_SHIFT)) & ADC_CLP1_CLP1_MASK)

/*! @name CLP0 - ADC Plus-Side General Calibration Value Register */
#define ADC_CLP0_CLP0_MASK                       (0x3FU)
#define ADC_CLP0_CLP0_SHIFT                      (0U)
#define ADC_CLP0_CLP0(x)                         (((uint32_t)(((uint32_t)(x)) << ADC_CLP0_CLP0_SHIFT)) & ADC_CLP0_CLP0_MASK)

/*! @name CLMD - ADC Minus-Side General Calibration Value Register */
#define ADC_CLMD_CLMD_MASK                       (0x3FU)
#define ADC_CLMD_CLMD_SHIFT                      (0U)
#define ADC_CLMD_CLMD(x)                         (((uint32_t)(((uint32_t)(x)) << ADC_CLMD_CLMD_SHIFT)) & ADC_CLMD_CLMD_MASK)

/*! @name CLMS - ADC Minus-Side General Calibration Value Register */
#define ADC_CLMS_CLMS_MASK                       (0x3FU)
#define ADC_CLMS_CLMS_SHIFT                      (0U)
#define ADC_CLMS_CLMS(x)                         (((uint32_t)(((uint32_t)(x)) << ADC_CLMS_CLMS_SHIFT)) & ADC_CLMS_CLMS_MASK)

/*! @name CLM4 - ADC Minus-Side General Calibration Value Register */
#define ADC_CLM4_CLM4_MASK                       (0x3FFU)
#define ADC_CLM4_CLM4_SHIFT                      (0U)
#define ADC_CLM4_CLM4(x)                         (((uint32_t)(((uint32_t)(x)) << ADC_CLM4_CLM4_SHIFT)) & ADC_CLM4_CLM4_MASK)

/*! @name CLM3 - ADC Minus-Side General Calibration Value Register */
#define ADC_CLM3_CLM3_MASK                       (0x1FFU)
#define ADC_CLM3_CLM3_SHIFT                      (0U)
#define ADC_CLM3_CLM3(x)                         (((uint32_t)(((uint32_t)(x)) << ADC_CLM3_CLM3_SHIFT)) & ADC_CLM3_CLM3_MASK)

/*! @name CLM2 - ADC Minus-Side General Calibration Value Register */
#define ADC_CLM2_CLM2_MASK                       (0xFFU)
#define ADC_CLM2_CLM2_SHIFT                      (0U)
#define ADC_CLM2_CLM2(x)                         (((uint32_t)(((uint32_t)(x)) << ADC_CLM2_CLM2_SHIFT)) & ADC_CLM2_CLM2_MASK)

/*! @name CLM1 - ADC Minus-Side General Calibration Value Register */
#define ADC_CLM1_CLM1_MASK                       (0x7FU)
#define ADC_CLM1_CLM1_SHIFT                      (0U)
#define ADC_CLM1_CLM1(x)                         (((uint32_t)(((uint32_t)(x)) << ADC_CLM1_CLM1_SHIFT)) & ADC_CLM1_CLM1_MASK)

/*! @name CLM0 - ADC Minus-Side General Calibration Value Register */
#define ADC_CLM0_CLM0_MASK                       (0x3FU)
#define ADC_CLM0_CLM0_SHIFT                      (0U)
#define ADC_CLM0_CLM0(x)                         (((uint32_t)(((uint32_t)(x)) << ADC_CLM0_CLM0_SHIFT)) & ADC_CLM0_CLM0_MASK)


/*!
 * @}
 */ /* end of group ADC_Register_Masks */


/* ADC - Peripheral instance base addresses */
/** Peripheral ADC0 base address */
#define ADC0_BASE                                (0x4003B000u)
/** Peripheral ADC0 base pointer */
#define ADC0                                     ((ADC_Type *)ADC0_BASE)
/** Array initializer of ADC peripheral base addresses */
#define ADC_BASE_ADDRS                           { ADC0_BASE }
/** Array initializer of ADC peripheral base pointers */
#define ADC_BASE_PTRS                            { ADC0 }
/** Interrupt vectors for the ADC peripheral type */
#define ADC_IRQS                                 { ADC0_IRQn }

/*!
 * @}
 */ /* end of group ADC_Peripheral_Access_Layer */


/* ----------------------------------------------------------------------------
   -- ANT Peripheral Access Layer
   ---------------------------------------------------------------------------- */

/*!
 * @addtogroup ANT_Peripheral_Access_Layer ANT Peripheral Access Layer
 * @{
 */

/** ANT - Register Layout Typedef */
typedef struct {
  __IO uint32_t IRQ_CTRL;                          /**< IRQ CONTROL, offset: 0x0 */
  __IO uint32_t EVENT_TMR;                         /**< EVENT TIMER, offset: 0x4 */
  __IO uint32_t T1_CMP;                            /**< T1 COMPARE, offset: 0x8 */
  __IO uint32_t T2_CMP;                            /**< T2 COMPARE, offset: 0xC */
  __I  uint32_t TIMESTAMP;                         /**< TIMESTAMP, offset: 0x10 */
  __IO uint32_t XCVR_CTRL;                         /**< TRANSCEIVER CONTROL, offset: 0x14 */
  __I  uint32_t XCVR_STS;                          /**< TRANSCEIVER STATUS, offset: 0x18 */
  __IO uint32_t XCVR_CFG;                          /**< TRANSCEIVER CONFIGURATION, offset: 0x1C */
  __IO uint32_t CHANNEL_NUM;                       /**< CHANNEL NUMBER, offset: 0x20 */
  __IO uint32_t TX_POWER;                          /**< TRANSMIT POWER, offset: 0x24 */
  __IO uint32_t NTW_ADR_CTRL;                      /**< NETWORK ADDRESS CONTROL, offset: 0x28 */
  __IO uint32_t NTW_ADR_0;                         /**< NETWORK ADDRESS 0, offset: 0x2C */
  __IO uint32_t NTW_ADR_1;                         /**< NETWORK ADDRESS 1, offset: 0x30 */
  __IO uint32_t NTW_ADR_2;                         /**< NETWORK ADDRESS 2, offset: 0x34 */
  __IO uint32_t NTW_ADR_3;                         /**< NETWORK ADDRESS 3, offset: 0x38 */
  __IO uint32_t RX_WATERMARK;                      /**< RX WATERMARK, offset: 0x3C */
  __IO uint32_t DSM_CTRL;                          /**< DSM CONTROL, offset: 0x40 */
  __I  uint32_t PART_ID;                           /**< PART ID, offset: 0x44 */
       uint8_t RESERVED_0[184];
  __IO uint16_t PACKET_BUFFER[64];                 /**< PACKET BUFFER, array offset: 0x100, array step: 0x2 */
} ANT_Type;

/* ----------------------------------------------------------------------------
   -- ANT Register Masks
   ---------------------------------------------------------------------------- */

/*!
 * @addtogroup ANT_Register_Masks ANT Register Masks
 * @{
 */

/*! @name IRQ_CTRL - IRQ CONTROL */
#define ANT_IRQ_CTRL_SEQ_END_IRQ_MASK            (0x1U)
#define ANT_IRQ_CTRL_SEQ_END_IRQ_SHIFT           (0U)
#define ANT_IRQ_CTRL_SEQ_END_IRQ(x)              (((uint32_t)(((uint32_t)(x)) << ANT_IRQ_CTRL_SEQ_END_IRQ_SHIFT)) & ANT_IRQ_CTRL_SEQ_END_IRQ_MASK)
#define ANT_IRQ_CTRL_TX_IRQ_MASK                 (0x2U)
#define ANT_IRQ_CTRL_TX_IRQ_SHIFT                (1U)
#define ANT_IRQ_CTRL_TX_IRQ(x)                   (((uint32_t)(((uint32_t)(x)) << ANT_IRQ_CTRL_TX_IRQ_SHIFT)) & ANT_IRQ_CTRL_TX_IRQ_MASK)
#define ANT_IRQ_CTRL_RX_IRQ_MASK                 (0x4U)
#define ANT_IRQ_CTRL_RX_IRQ_SHIFT                (2U)
#define ANT_IRQ_CTRL_RX_IRQ(x)                   (((uint32_t)(((uint32_t)(x)) << ANT_IRQ_CTRL_RX_IRQ_SHIFT)) & ANT_IRQ_CTRL_RX_IRQ_MASK)
#define ANT_IRQ_CTRL_NTW_ADR_IRQ_MASK            (0x8U)
#define ANT_IRQ_CTRL_NTW_ADR_IRQ_SHIFT           (3U)
#define ANT_IRQ_CTRL_NTW_ADR_IRQ(x)              (((uint32_t)(((uint32_t)(x)) << ANT_IRQ_CTRL_NTW_ADR_IRQ_SHIFT)) & ANT_IRQ_CTRL_NTW_ADR_IRQ_MASK)
#define ANT_IRQ_CTRL_T1_IRQ_MASK                 (0x10U)
#define ANT_IRQ_CTRL_T1_IRQ_SHIFT                (4U)
#define ANT_IRQ_CTRL_T1_IRQ(x)                   (((uint32_t)(((uint32_t)(x)) << ANT_IRQ_CTRL_T1_IRQ_SHIFT)) & ANT_IRQ_CTRL_T1_IRQ_MASK)
#define ANT_IRQ_CTRL_T2_IRQ_MASK                 (0x20U)
#define ANT_IRQ_CTRL_T2_IRQ_SHIFT                (5U)
#define ANT_IRQ_CTRL_T2_IRQ(x)                   (((uint32_t)(((uint32_t)(x)) << ANT_IRQ_CTRL_T2_IRQ_SHIFT)) & ANT_IRQ_CTRL_T2_IRQ_MASK)
#define ANT_IRQ_CTRL_PLL_UNLOCK_IRQ_MASK         (0x40U)
#define ANT_IRQ_CTRL_PLL_UNLOCK_IRQ_SHIFT        (6U)
#define ANT_IRQ_CTRL_PLL_UNLOCK_IRQ(x)           (((uint32_t)(((uint32_t)(x)) << ANT_IRQ_CTRL_PLL_UNLOCK_IRQ_SHIFT)) & ANT_IRQ_CTRL_PLL_UNLOCK_IRQ_MASK)
#define ANT_IRQ_CTRL_WAKE_IRQ_MASK               (0x80U)
#define ANT_IRQ_CTRL_WAKE_IRQ_SHIFT              (7U)
#define ANT_IRQ_CTRL_WAKE_IRQ(x)                 (((uint32_t)(((uint32_t)(x)) << ANT_IRQ_CTRL_WAKE_IRQ_SHIFT)) & ANT_IRQ_CTRL_WAKE_IRQ_MASK)
#define ANT_IRQ_CTRL_RX_WATERMARK_IRQ_MASK       (0x100U)
#define ANT_IRQ_CTRL_RX_WATERMARK_IRQ_SHIFT      (8U)
#define ANT_IRQ_CTRL_RX_WATERMARK_IRQ(x)         (((uint32_t)(((uint32_t)(x)) << ANT_IRQ_CTRL_RX_WATERMARK_IRQ_SHIFT)) & ANT_IRQ_CTRL_RX_WATERMARK_IRQ_MASK)
#define ANT_IRQ_CTRL_TSM_IRQ_MASK                (0x200U)
#define ANT_IRQ_CTRL_TSM_IRQ_SHIFT               (9U)
#define ANT_IRQ_CTRL_TSM_IRQ(x)                  (((uint32_t)(((uint32_t)(x)) << ANT_IRQ_CTRL_TSM_IRQ_SHIFT)) & ANT_IRQ_CTRL_TSM_IRQ_MASK)
#define ANT_IRQ_CTRL_SEQ_END_IRQ_EN_MASK         (0x10000U)
#define ANT_IRQ_CTRL_SEQ_END_IRQ_EN_SHIFT        (16U)
#define ANT_IRQ_CTRL_SEQ_END_IRQ_EN(x)           (((uint32_t)(((uint32_t)(x)) << ANT_IRQ_CTRL_SEQ_END_IRQ_EN_SHIFT)) & ANT_IRQ_CTRL_SEQ_END_IRQ_EN_MASK)
#define ANT_IRQ_CTRL_TX_IRQ_EN_MASK              (0x20000U)
#define ANT_IRQ_CTRL_TX_IRQ_EN_SHIFT             (17U)
#define ANT_IRQ_CTRL_TX_IRQ_EN(x)                (((uint32_t)(((uint32_t)(x)) << ANT_IRQ_CTRL_TX_IRQ_EN_SHIFT)) & ANT_IRQ_CTRL_TX_IRQ_EN_MASK)
#define ANT_IRQ_CTRL_RX_IRQ_EN_MASK              (0x40000U)
#define ANT_IRQ_CTRL_RX_IRQ_EN_SHIFT             (18U)
#define ANT_IRQ_CTRL_RX_IRQ_EN(x)                (((uint32_t)(((uint32_t)(x)) << ANT_IRQ_CTRL_RX_IRQ_EN_SHIFT)) & ANT_IRQ_CTRL_RX_IRQ_EN_MASK)
#define ANT_IRQ_CTRL_NTW_ADR_IRQ_EN_MASK         (0x80000U)
#define ANT_IRQ_CTRL_NTW_ADR_IRQ_EN_SHIFT        (19U)
#define ANT_IRQ_CTRL_NTW_ADR_IRQ_EN(x)           (((uint32_t)(((uint32_t)(x)) << ANT_IRQ_CTRL_NTW_ADR_IRQ_EN_SHIFT)) & ANT_IRQ_CTRL_NTW_ADR_IRQ_EN_MASK)
#define ANT_IRQ_CTRL_T1_IRQ_EN_MASK              (0x100000U)
#define ANT_IRQ_CTRL_T1_IRQ_EN_SHIFT             (20U)
#define ANT_IRQ_CTRL_T1_IRQ_EN(x)                (((uint32_t)(((uint32_t)(x)) << ANT_IRQ_CTRL_T1_IRQ_EN_SHIFT)) & ANT_IRQ_CTRL_T1_IRQ_EN_MASK)
#define ANT_IRQ_CTRL_T2_IRQ_EN_MASK              (0x200000U)
#define ANT_IRQ_CTRL_T2_IRQ_EN_SHIFT             (21U)
#define ANT_IRQ_CTRL_T2_IRQ_EN(x)                (((uint32_t)(((uint32_t)(x)) << ANT_IRQ_CTRL_T2_IRQ_EN_SHIFT)) & ANT_IRQ_CTRL_T2_IRQ_EN_MASK)
#define ANT_IRQ_CTRL_PLL_UNLOCK_IRQ_EN_MASK      (0x400000U)
#define ANT_IRQ_CTRL_PLL_UNLOCK_IRQ_EN_SHIFT     (22U)
#define ANT_IRQ_CTRL_PLL_UNLOCK_IRQ_EN(x)        (((uint32_t)(((uint32_t)(x)) << ANT_IRQ_CTRL_PLL_UNLOCK_IRQ_EN_SHIFT)) & ANT_IRQ_CTRL_PLL_UNLOCK_IRQ_EN_MASK)
#define ANT_IRQ_CTRL_WAKE_IRQ_EN_MASK            (0x800000U)
#define ANT_IRQ_CTRL_WAKE_IRQ_EN_SHIFT           (23U)
#define ANT_IRQ_CTRL_WAKE_IRQ_EN(x)              (((uint32_t)(((uint32_t)(x)) << ANT_IRQ_CTRL_WAKE_IRQ_EN_SHIFT)) & ANT_IRQ_CTRL_WAKE_IRQ_EN_MASK)
#define ANT_IRQ_CTRL_RX_WATERMARK_IRQ_EN_MASK    (0x1000000U)
#define ANT_IRQ_CTRL_RX_WATERMARK_IRQ_EN_SHIFT   (24U)
#define ANT_IRQ_CTRL_RX_WATERMARK_IRQ_EN(x)      (((uint32_t)(((uint32_t)(x)) << ANT_IRQ_CTRL_RX_WATERMARK_IRQ_EN_SHIFT)) & ANT_IRQ_CTRL_RX_WATERMARK_IRQ_EN_MASK)
#define ANT_IRQ_CTRL_TSM_IRQ_EN_MASK             (0x2000000U)
#define ANT_IRQ_CTRL_TSM_IRQ_EN_SHIFT            (25U)
#define ANT_IRQ_CTRL_TSM_IRQ_EN(x)               (((uint32_t)(((uint32_t)(x)) << ANT_IRQ_CTRL_TSM_IRQ_EN_SHIFT)) & ANT_IRQ_CTRL_TSM_IRQ_EN_MASK)
#define ANT_IRQ_CTRL_ANT_IRQ_EN_MASK             (0x4000000U)
#define ANT_IRQ_CTRL_ANT_IRQ_EN_SHIFT            (26U)
#define ANT_IRQ_CTRL_ANT_IRQ_EN(x)               (((uint32_t)(((uint32_t)(x)) << ANT_IRQ_CTRL_ANT_IRQ_EN_SHIFT)) & ANT_IRQ_CTRL_ANT_IRQ_EN_MASK)
#define ANT_IRQ_CTRL_CRC_IGNORE_MASK             (0x8000000U)
#define ANT_IRQ_CTRL_CRC_IGNORE_SHIFT            (27U)
#define ANT_IRQ_CTRL_CRC_IGNORE(x)               (((uint32_t)(((uint32_t)(x)) << ANT_IRQ_CTRL_CRC_IGNORE_SHIFT)) & ANT_IRQ_CTRL_CRC_IGNORE_MASK)
#define ANT_IRQ_CTRL_CRC_VALID_MASK              (0x80000000U)
#define ANT_IRQ_CTRL_CRC_VALID_SHIFT             (31U)
#define ANT_IRQ_CTRL_CRC_VALID(x)                (((uint32_t)(((uint32_t)(x)) << ANT_IRQ_CTRL_CRC_VALID_SHIFT)) & ANT_IRQ_CTRL_CRC_VALID_MASK)

/*! @name EVENT_TMR - EVENT TIMER */
#define ANT_EVENT_TMR_EVENT_TMR_MASK             (0xFFFFFFU)
#define ANT_EVENT_TMR_EVENT_TMR_SHIFT            (0U)
#define ANT_EVENT_TMR_EVENT_TMR(x)               (((uint32_t)(((uint32_t)(x)) << ANT_EVENT_TMR_EVENT_TMR_SHIFT)) & ANT_EVENT_TMR_EVENT_TMR_MASK)
#define ANT_EVENT_TMR_EVENT_TMR_LD_MASK          (0x1000000U)
#define ANT_EVENT_TMR_EVENT_TMR_LD_SHIFT         (24U)
#define ANT_EVENT_TMR_EVENT_TMR_LD(x)            (((uint32_t)(((uint32_t)(x)) << ANT_EVENT_TMR_EVENT_TMR_LD_SHIFT)) & ANT_EVENT_TMR_EVENT_TMR_LD_MASK)
#define ANT_EVENT_TMR_EVENT_TMR_ADD_MASK         (0x2000000U)
#define ANT_EVENT_TMR_EVENT_TMR_ADD_SHIFT        (25U)
#define ANT_EVENT_TMR_EVENT_TMR_ADD(x)           (((uint32_t)(((uint32_t)(x)) << ANT_EVENT_TMR_EVENT_TMR_ADD_SHIFT)) & ANT_EVENT_TMR_EVENT_TMR_ADD_MASK)

/*! @name T1_CMP - T1 COMPARE */
#define ANT_T1_CMP_T1_CMP_MASK                   (0xFFFFFFU)
#define ANT_T1_CMP_T1_CMP_SHIFT                  (0U)
#define ANT_T1_CMP_T1_CMP(x)                     (((uint32_t)(((uint32_t)(x)) << ANT_T1_CMP_T1_CMP_SHIFT)) & ANT_T1_CMP_T1_CMP_MASK)
#define ANT_T1_CMP_T1_CMP_EN_MASK                (0x1000000U)
#define ANT_T1_CMP_T1_CMP_EN_SHIFT               (24U)
#define ANT_T1_CMP_T1_CMP_EN(x)                  (((uint32_t)(((uint32_t)(x)) << ANT_T1_CMP_T1_CMP_EN_SHIFT)) & ANT_T1_CMP_T1_CMP_EN_MASK)

/*! @name T2_CMP - T2 COMPARE */
#define ANT_T2_CMP_T2_CMP_MASK                   (0xFFFFFFU)
#define ANT_T2_CMP_T2_CMP_SHIFT                  (0U)
#define ANT_T2_CMP_T2_CMP(x)                     (((uint32_t)(((uint32_t)(x)) << ANT_T2_CMP_T2_CMP_SHIFT)) & ANT_T2_CMP_T2_CMP_MASK)
#define ANT_T2_CMP_T2_CMP_EN_MASK                (0x1000000U)
#define ANT_T2_CMP_T2_CMP_EN_SHIFT               (24U)
#define ANT_T2_CMP_T2_CMP_EN(x)                  (((uint32_t)(((uint32_t)(x)) << ANT_T2_CMP_T2_CMP_EN_SHIFT)) & ANT_T2_CMP_T2_CMP_EN_MASK)

/*! @name TIMESTAMP - TIMESTAMP */
#define ANT_TIMESTAMP_TIMESTAMP_MASK             (0xFFFFFFU)
#define ANT_TIMESTAMP_TIMESTAMP_SHIFT            (0U)
#define ANT_TIMESTAMP_TIMESTAMP(x)               (((uint32_t)(((uint32_t)(x)) << ANT_TIMESTAMP_TIMESTAMP_SHIFT)) & ANT_TIMESTAMP_TIMESTAMP_MASK)

/*! @name XCVR_CTRL - TRANSCEIVER CONTROL */
#define ANT_XCVR_CTRL_SEQCMD_MASK                (0xFU)
#define ANT_XCVR_CTRL_SEQCMD_SHIFT               (0U)
#define ANT_XCVR_CTRL_SEQCMD(x)                  (((uint32_t)(((uint32_t)(x)) << ANT_XCVR_CTRL_SEQCMD_SHIFT)) & ANT_XCVR_CTRL_SEQCMD_MASK)
#define ANT_XCVR_CTRL_TX_PKT_LENGTH_MASK         (0x3F00U)
#define ANT_XCVR_CTRL_TX_PKT_LENGTH_SHIFT        (8U)
#define ANT_XCVR_CTRL_TX_PKT_LENGTH(x)           (((uint32_t)(((uint32_t)(x)) << ANT_XCVR_CTRL_TX_PKT_LENGTH_SHIFT)) & ANT_XCVR_CTRL_TX_PKT_LENGTH_MASK)
#define ANT_XCVR_CTRL_RX_PKT_LENGTH_MASK         (0x3F0000U)
#define ANT_XCVR_CTRL_RX_PKT_LENGTH_SHIFT        (16U)
#define ANT_XCVR_CTRL_RX_PKT_LENGTH(x)           (((uint32_t)(((uint32_t)(x)) << ANT_XCVR_CTRL_RX_PKT_LENGTH_SHIFT)) & ANT_XCVR_CTRL_RX_PKT_LENGTH_MASK)
#define ANT_XCVR_CTRL_CMDDEC_CS_MASK             (0x7000000U)
#define ANT_XCVR_CTRL_CMDDEC_CS_SHIFT            (24U)
#define ANT_XCVR_CTRL_CMDDEC_CS(x)               (((uint32_t)(((uint32_t)(x)) << ANT_XCVR_CTRL_CMDDEC_CS_SHIFT)) & ANT_XCVR_CTRL_CMDDEC_CS_MASK)
#define ANT_XCVR_CTRL_XCVR_BUSY_MASK             (0x80000000U)
#define ANT_XCVR_CTRL_XCVR_BUSY_SHIFT            (31U)
#define ANT_XCVR_CTRL_XCVR_BUSY(x)               (((uint32_t)(((uint32_t)(x)) << ANT_XCVR_CTRL_XCVR_BUSY_SHIFT)) & ANT_XCVR_CTRL_XCVR_BUSY_MASK)

/*! @name XCVR_STS - TRANSCEIVER STATUS */
#define ANT_XCVR_STS_TX_START_T1_PEND_MASK       (0x1U)
#define ANT_XCVR_STS_TX_START_T1_PEND_SHIFT      (0U)
#define ANT_XCVR_STS_TX_START_T1_PEND(x)         (((uint32_t)(((uint32_t)(x)) << ANT_XCVR_STS_TX_START_T1_PEND_SHIFT)) & ANT_XCVR_STS_TX_START_T1_PEND_MASK)
#define ANT_XCVR_STS_TX_START_T2_PEND_MASK       (0x2U)
#define ANT_XCVR_STS_TX_START_T2_PEND_SHIFT      (1U)
#define ANT_XCVR_STS_TX_START_T2_PEND(x)         (((uint32_t)(((uint32_t)(x)) << ANT_XCVR_STS_TX_START_T2_PEND_SHIFT)) & ANT_XCVR_STS_TX_START_T2_PEND_MASK)
#define ANT_XCVR_STS_TX_IN_WARMUP_MASK           (0x4U)
#define ANT_XCVR_STS_TX_IN_WARMUP_SHIFT          (2U)
#define ANT_XCVR_STS_TX_IN_WARMUP(x)             (((uint32_t)(((uint32_t)(x)) << ANT_XCVR_STS_TX_IN_WARMUP_SHIFT)) & ANT_XCVR_STS_TX_IN_WARMUP_MASK)
#define ANT_XCVR_STS_TX_IN_PROGRESS_MASK         (0x8U)
#define ANT_XCVR_STS_TX_IN_PROGRESS_SHIFT        (3U)
#define ANT_XCVR_STS_TX_IN_PROGRESS(x)           (((uint32_t)(((uint32_t)(x)) << ANT_XCVR_STS_TX_IN_PROGRESS_SHIFT)) & ANT_XCVR_STS_TX_IN_PROGRESS_MASK)
#define ANT_XCVR_STS_TX_IN_WARMDN_MASK           (0x10U)
#define ANT_XCVR_STS_TX_IN_WARMDN_SHIFT          (4U)
#define ANT_XCVR_STS_TX_IN_WARMDN(x)             (((uint32_t)(((uint32_t)(x)) << ANT_XCVR_STS_TX_IN_WARMDN_SHIFT)) & ANT_XCVR_STS_TX_IN_WARMDN_MASK)
#define ANT_XCVR_STS_RX_START_T1_PEND_MASK       (0x20U)
#define ANT_XCVR_STS_RX_START_T1_PEND_SHIFT      (5U)
#define ANT_XCVR_STS_RX_START_T1_PEND(x)         (((uint32_t)(((uint32_t)(x)) << ANT_XCVR_STS_RX_START_T1_PEND_SHIFT)) & ANT_XCVR_STS_RX_START_T1_PEND_MASK)
#define ANT_XCVR_STS_RX_START_T2_PEND_MASK       (0x40U)
#define ANT_XCVR_STS_RX_START_T2_PEND_SHIFT      (6U)
#define ANT_XCVR_STS_RX_START_T2_PEND(x)         (((uint32_t)(((uint32_t)(x)) << ANT_XCVR_STS_RX_START_T2_PEND_SHIFT)) & ANT_XCVR_STS_RX_START_T2_PEND_MASK)
#define ANT_XCVR_STS_RX_STOP_T1_PEND_MASK        (0x80U)
#define ANT_XCVR_STS_RX_STOP_T1_PEND_SHIFT       (7U)
#define ANT_XCVR_STS_RX_STOP_T1_PEND(x)          (((uint32_t)(((uint32_t)(x)) << ANT_XCVR_STS_RX_STOP_T1_PEND_SHIFT)) & ANT_XCVR_STS_RX_STOP_T1_PEND_MASK)
#define ANT_XCVR_STS_RX_STOP_T2_PEND_MASK        (0x100U)
#define ANT_XCVR_STS_RX_STOP_T2_PEND_SHIFT       (8U)
#define ANT_XCVR_STS_RX_STOP_T2_PEND(x)          (((uint32_t)(((uint32_t)(x)) << ANT_XCVR_STS_RX_STOP_T2_PEND_SHIFT)) & ANT_XCVR_STS_RX_STOP_T2_PEND_MASK)
#define ANT_XCVR_STS_RX_IN_WARMUP_MASK           (0x200U)
#define ANT_XCVR_STS_RX_IN_WARMUP_SHIFT          (9U)
#define ANT_XCVR_STS_RX_IN_WARMUP(x)             (((uint32_t)(((uint32_t)(x)) << ANT_XCVR_STS_RX_IN_WARMUP_SHIFT)) & ANT_XCVR_STS_RX_IN_WARMUP_MASK)
#define ANT_XCVR_STS_RX_IN_SEARCH_MASK           (0x400U)
#define ANT_XCVR_STS_RX_IN_SEARCH_SHIFT          (10U)
#define ANT_XCVR_STS_RX_IN_SEARCH(x)             (((uint32_t)(((uint32_t)(x)) << ANT_XCVR_STS_RX_IN_SEARCH_SHIFT)) & ANT_XCVR_STS_RX_IN_SEARCH_MASK)
#define ANT_XCVR_STS_RX_IN_PROGRESS_MASK         (0x800U)
#define ANT_XCVR_STS_RX_IN_PROGRESS_SHIFT        (11U)
#define ANT_XCVR_STS_RX_IN_PROGRESS(x)           (((uint32_t)(((uint32_t)(x)) << ANT_XCVR_STS_RX_IN_PROGRESS_SHIFT)) & ANT_XCVR_STS_RX_IN_PROGRESS_MASK)
#define ANT_XCVR_STS_RX_IN_WARMDN_MASK           (0x1000U)
#define ANT_XCVR_STS_RX_IN_WARMDN_SHIFT          (12U)
#define ANT_XCVR_STS_RX_IN_WARMDN(x)             (((uint32_t)(((uint32_t)(x)) << ANT_XCVR_STS_RX_IN_WARMDN_SHIFT)) & ANT_XCVR_STS_RX_IN_WARMDN_MASK)
#define ANT_XCVR_STS_CRC_VALID_MASK              (0x8000U)
#define ANT_XCVR_STS_CRC_VALID_SHIFT             (15U)
#define ANT_XCVR_STS_CRC_VALID(x)                (((uint32_t)(((uint32_t)(x)) << ANT_XCVR_STS_CRC_VALID_SHIFT)) & ANT_XCVR_STS_CRC_VALID_MASK)
#define ANT_XCVR_STS_RSSI_MASK                   (0xFF0000U)
#define ANT_XCVR_STS_RSSI_SHIFT                  (16U)
#define ANT_XCVR_STS_RSSI(x)                     (((uint32_t)(((uint32_t)(x)) << ANT_XCVR_STS_RSSI_SHIFT)) & ANT_XCVR_STS_RSSI_MASK)

/*! @name XCVR_CFG - TRANSCEIVER CONFIGURATION */
#define ANT_XCVR_CFG_TX_WHITEN_DIS_MASK          (0x1U)
#define ANT_XCVR_CFG_TX_WHITEN_DIS_SHIFT         (0U)
#define ANT_XCVR_CFG_TX_WHITEN_DIS(x)            (((uint32_t)(((uint32_t)(x)) << ANT_XCVR_CFG_TX_WHITEN_DIS_SHIFT)) & ANT_XCVR_CFG_TX_WHITEN_DIS_MASK)
#define ANT_XCVR_CFG_RX_DEWHITEN_DIS_MASK        (0x2U)
#define ANT_XCVR_CFG_RX_DEWHITEN_DIS_SHIFT       (1U)
#define ANT_XCVR_CFG_RX_DEWHITEN_DIS(x)          (((uint32_t)(((uint32_t)(x)) << ANT_XCVR_CFG_RX_DEWHITEN_DIS_SHIFT)) & ANT_XCVR_CFG_RX_DEWHITEN_DIS_MASK)
#define ANT_XCVR_CFG_SW_CRC_EN_MASK              (0x4U)
#define ANT_XCVR_CFG_SW_CRC_EN_SHIFT             (2U)
#define ANT_XCVR_CFG_SW_CRC_EN(x)                (((uint32_t)(((uint32_t)(x)) << ANT_XCVR_CFG_SW_CRC_EN_SHIFT)) & ANT_XCVR_CFG_SW_CRC_EN_MASK)
#define ANT_XCVR_CFG_PREAMBLE_SZ_MASK            (0x30U)
#define ANT_XCVR_CFG_PREAMBLE_SZ_SHIFT           (4U)
#define ANT_XCVR_CFG_PREAMBLE_SZ(x)              (((uint32_t)(((uint32_t)(x)) << ANT_XCVR_CFG_PREAMBLE_SZ_SHIFT)) & ANT_XCVR_CFG_PREAMBLE_SZ_MASK)
#define ANT_XCVR_CFG_TX_WARMUP_MASK              (0xFF00U)
#define ANT_XCVR_CFG_TX_WARMUP_SHIFT             (8U)
#define ANT_XCVR_CFG_TX_WARMUP(x)                (((uint32_t)(((uint32_t)(x)) << ANT_XCVR_CFG_TX_WARMUP_SHIFT)) & ANT_XCVR_CFG_TX_WARMUP_MASK)
#define ANT_XCVR_CFG_RX_WARMUP_MASK              (0xFF0000U)
#define ANT_XCVR_CFG_RX_WARMUP_SHIFT             (16U)
#define ANT_XCVR_CFG_RX_WARMUP(x)                (((uint32_t)(((uint32_t)(x)) << ANT_XCVR_CFG_RX_WARMUP_SHIFT)) & ANT_XCVR_CFG_RX_WARMUP_MASK)

/*! @name CHANNEL_NUM - CHANNEL NUMBER */
#define ANT_CHANNEL_NUM_CHANNEL_NUM_MASK         (0x7FU)
#define ANT_CHANNEL_NUM_CHANNEL_NUM_SHIFT        (0U)
#define ANT_CHANNEL_NUM_CHANNEL_NUM(x)           (((uint32_t)(((uint32_t)(x)) << ANT_CHANNEL_NUM_CHANNEL_NUM_SHIFT)) & ANT_CHANNEL_NUM_CHANNEL_NUM_MASK)

/*! @name TX_POWER - TRANSMIT POWER */
#define ANT_TX_POWER_TX_POWER_MASK               (0x3FU)
#define ANT_TX_POWER_TX_POWER_SHIFT              (0U)
#define ANT_TX_POWER_TX_POWER(x)                 (((uint32_t)(((uint32_t)(x)) << ANT_TX_POWER_TX_POWER_SHIFT)) & ANT_TX_POWER_TX_POWER_MASK)

/*! @name NTW_ADR_CTRL - NETWORK ADDRESS CONTROL */
#define ANT_NTW_ADR_CTRL_NTW_ADR_EN_MASK         (0xFU)
#define ANT_NTW_ADR_CTRL_NTW_ADR_EN_SHIFT        (0U)
#define ANT_NTW_ADR_CTRL_NTW_ADR_EN(x)           (((uint32_t)(((uint32_t)(x)) << ANT_NTW_ADR_CTRL_NTW_ADR_EN_SHIFT)) & ANT_NTW_ADR_CTRL_NTW_ADR_EN_MASK)
#define ANT_NTW_ADR_CTRL_NTW_ADR_MCH_MASK        (0xF0U)
#define ANT_NTW_ADR_CTRL_NTW_ADR_MCH_SHIFT       (4U)
#define ANT_NTW_ADR_CTRL_NTW_ADR_MCH(x)          (((uint32_t)(((uint32_t)(x)) << ANT_NTW_ADR_CTRL_NTW_ADR_MCH_SHIFT)) & ANT_NTW_ADR_CTRL_NTW_ADR_MCH_MASK)
#define ANT_NTW_ADR_CTRL_NTW_ADR0_SZ_MASK        (0x300U)
#define ANT_NTW_ADR_CTRL_NTW_ADR0_SZ_SHIFT       (8U)
#define ANT_NTW_ADR_CTRL_NTW_ADR0_SZ(x)          (((uint32_t)(((uint32_t)(x)) << ANT_NTW_ADR_CTRL_NTW_ADR0_SZ_SHIFT)) & ANT_NTW_ADR_CTRL_NTW_ADR0_SZ_MASK)
#define ANT_NTW_ADR_CTRL_NTW_ADR1_SZ_MASK        (0xC00U)
#define ANT_NTW_ADR_CTRL_NTW_ADR1_SZ_SHIFT       (10U)
#define ANT_NTW_ADR_CTRL_NTW_ADR1_SZ(x)          (((uint32_t)(((uint32_t)(x)) << ANT_NTW_ADR_CTRL_NTW_ADR1_SZ_SHIFT)) & ANT_NTW_ADR_CTRL_NTW_ADR1_SZ_MASK)
#define ANT_NTW_ADR_CTRL_NTW_ADR2_SZ_MASK        (0x3000U)
#define ANT_NTW_ADR_CTRL_NTW_ADR2_SZ_SHIFT       (12U)
#define ANT_NTW_ADR_CTRL_NTW_ADR2_SZ(x)          (((uint32_t)(((uint32_t)(x)) << ANT_NTW_ADR_CTRL_NTW_ADR2_SZ_SHIFT)) & ANT_NTW_ADR_CTRL_NTW_ADR2_SZ_MASK)
#define ANT_NTW_ADR_CTRL_NTW_ADR3_SZ_MASK        (0xC000U)
#define ANT_NTW_ADR_CTRL_NTW_ADR3_SZ_SHIFT       (14U)
#define ANT_NTW_ADR_CTRL_NTW_ADR3_SZ(x)          (((uint32_t)(((uint32_t)(x)) << ANT_NTW_ADR_CTRL_NTW_ADR3_SZ_SHIFT)) & ANT_NTW_ADR_CTRL_NTW_ADR3_SZ_MASK)
#define ANT_NTW_ADR_CTRL_NTW_ADR_THR0_MASK       (0x70000U)
#define ANT_NTW_ADR_CTRL_NTW_ADR_THR0_SHIFT      (16U)
#define ANT_NTW_ADR_CTRL_NTW_ADR_THR0(x)         (((uint32_t)(((uint32_t)(x)) << ANT_NTW_ADR_CTRL_NTW_ADR_THR0_SHIFT)) & ANT_NTW_ADR_CTRL_NTW_ADR_THR0_MASK)
#define ANT_NTW_ADR_CTRL_NTW_ADR_THR1_MASK       (0x700000U)
#define ANT_NTW_ADR_CTRL_NTW_ADR_THR1_SHIFT      (20U)
#define ANT_NTW_ADR_CTRL_NTW_ADR_THR1(x)         (((uint32_t)(((uint32_t)(x)) << ANT_NTW_ADR_CTRL_NTW_ADR_THR1_SHIFT)) & ANT_NTW_ADR_CTRL_NTW_ADR_THR1_MASK)
#define ANT_NTW_ADR_CTRL_NTW_ADR_THR2_MASK       (0x7000000U)
#define ANT_NTW_ADR_CTRL_NTW_ADR_THR2_SHIFT      (24U)
#define ANT_NTW_ADR_CTRL_NTW_ADR_THR2(x)         (((uint32_t)(((uint32_t)(x)) << ANT_NTW_ADR_CTRL_NTW_ADR_THR2_SHIFT)) & ANT_NTW_ADR_CTRL_NTW_ADR_THR2_MASK)
#define ANT_NTW_ADR_CTRL_NTW_ADR_THR3_MASK       (0x70000000U)
#define ANT_NTW_ADR_CTRL_NTW_ADR_THR3_SHIFT      (28U)
#define ANT_NTW_ADR_CTRL_NTW_ADR_THR3(x)         (((uint32_t)(((uint32_t)(x)) << ANT_NTW_ADR_CTRL_NTW_ADR_THR3_SHIFT)) & ANT_NTW_ADR_CTRL_NTW_ADR_THR3_MASK)

/*! @name NTW_ADR_0 - NETWORK ADDRESS 0 */
#define ANT_NTW_ADR_0_NTW_ADR_0_MASK             (0xFFFFFFFFU)
#define ANT_NTW_ADR_0_NTW_ADR_0_SHIFT            (0U)
#define ANT_NTW_ADR_0_NTW_ADR_0(x)               (((uint32_t)(((uint32_t)(x)) << ANT_NTW_ADR_0_NTW_ADR_0_SHIFT)) & ANT_NTW_ADR_0_NTW_ADR_0_MASK)

/*! @name NTW_ADR_1 - NETWORK ADDRESS 1 */
#define ANT_NTW_ADR_1_NTW_ADR_1_MASK             (0xFFFFFFFFU)
#define ANT_NTW_ADR_1_NTW_ADR_1_SHIFT            (0U)
#define ANT_NTW_ADR_1_NTW_ADR_1(x)               (((uint32_t)(((uint32_t)(x)) << ANT_NTW_ADR_1_NTW_ADR_1_SHIFT)) & ANT_NTW_ADR_1_NTW_ADR_1_MASK)

/*! @name NTW_ADR_2 - NETWORK ADDRESS 2 */
#define ANT_NTW_ADR_2_NTW_ADR_2_MASK             (0xFFFFFFFFU)
#define ANT_NTW_ADR_2_NTW_ADR_2_SHIFT            (0U)
#define ANT_NTW_ADR_2_NTW_ADR_2(x)               (((uint32_t)(((uint32_t)(x)) << ANT_NTW_ADR_2_NTW_ADR_2_SHIFT)) & ANT_NTW_ADR_2_NTW_ADR_2_MASK)

/*! @name NTW_ADR_3 - NETWORK ADDRESS 3 */
#define ANT_NTW_ADR_3_NTW_ADR_3_MASK             (0xFFFFFFFFU)
#define ANT_NTW_ADR_3_NTW_ADR_3_SHIFT            (0U)
#define ANT_NTW_ADR_3_NTW_ADR_3(x)               (((uint32_t)(((uint32_t)(x)) << ANT_NTW_ADR_3_NTW_ADR_3_SHIFT)) & ANT_NTW_ADR_3_NTW_ADR_3_MASK)

/*! @name RX_WATERMARK - RX WATERMARK */
#define ANT_RX_WATERMARK_RX_WATERMARK_MASK       (0x7FU)
#define ANT_RX_WATERMARK_RX_WATERMARK_SHIFT      (0U)
#define ANT_RX_WATERMARK_RX_WATERMARK(x)         (((uint32_t)(((uint32_t)(x)) << ANT_RX_WATERMARK_RX_WATERMARK_SHIFT)) & ANT_RX_WATERMARK_RX_WATERMARK_MASK)
#define ANT_RX_WATERMARK_BYTE_COUNTER_MASK       (0x7F0000U)
#define ANT_RX_WATERMARK_BYTE_COUNTER_SHIFT      (16U)
#define ANT_RX_WATERMARK_BYTE_COUNTER(x)         (((uint32_t)(((uint32_t)(x)) << ANT_RX_WATERMARK_BYTE_COUNTER_SHIFT)) & ANT_RX_WATERMARK_BYTE_COUNTER_MASK)

/*! @name DSM_CTRL - DSM CONTROL */
#define ANT_DSM_CTRL_ANT_SLEEP_EN_MASK           (0x1U)
#define ANT_DSM_CTRL_ANT_SLEEP_EN_SHIFT          (0U)
#define ANT_DSM_CTRL_ANT_SLEEP_EN(x)             (((uint32_t)(((uint32_t)(x)) << ANT_DSM_CTRL_ANT_SLEEP_EN_SHIFT)) & ANT_DSM_CTRL_ANT_SLEEP_EN_MASK)

/*! @name PART_ID - PART ID */
#define ANT_PART_ID_PART_ID_MASK                 (0xFFU)
#define ANT_PART_ID_PART_ID_SHIFT                (0U)
#define ANT_PART_ID_PART_ID(x)                   (((uint32_t)(((uint32_t)(x)) << ANT_PART_ID_PART_ID_SHIFT)) & ANT_PART_ID_PART_ID_MASK)

/*! @name PACKET_BUFFER - PACKET BUFFER */
#define ANT_PACKET_BUFFER_PACKET_BUFFER_MASK     (0xFFFFU)
#define ANT_PACKET_BUFFER_PACKET_BUFFER_SHIFT    (0U)
#define ANT_PACKET_BUFFER_PACKET_BUFFER(x)       (((uint16_t)(((uint16_t)(x)) << ANT_PACKET_BUFFER_PACKET_BUFFER_SHIFT)) & ANT_PACKET_BUFFER_PACKET_BUFFER_MASK)

/* The count of ANT_PACKET_BUFFER */
#define ANT_PACKET_BUFFER_COUNT                  (64U)


/*!
 * @}
 */ /* end of group ANT_Register_Masks */


/* ANT - Peripheral instance base addresses */
/** Peripheral ANT base address */
#define ANT_BASE                                 (0x4005E000u)
/** Peripheral ANT base pointer */
#define ANT                                      ((ANT_Type *)ANT_BASE)
/** Array initializer of ANT peripheral base addresses */
#define ANT_BASE_ADDRS                           { ANT_BASE }
/** Array initializer of ANT peripheral base pointers */
#define ANT_BASE_PTRS                            { ANT }

/*!
 * @}
 */ /* end of group ANT_Peripheral_Access_Layer */


/* ----------------------------------------------------------------------------
   -- BTLE_RF Peripheral Access Layer
   ---------------------------------------------------------------------------- */

/*!
 * @addtogroup BTLE_RF_Peripheral_Access_Layer BTLE_RF Peripheral Access Layer
 * @{
 */

/** BTLE_RF - Register Layout Typedef */
typedef struct {
       uint8_t RESERVED_0[1536];
  __I  uint16_t BLE_PART_ID;                       /**< BLUETOOTH LOW ENERGY PART ID, offset: 0x600 */
       uint8_t RESERVED_1[2];
  __I  uint16_t DSM_STATUS;                        /**< BLE DSM STATUS, offset: 0x604 */
       uint8_t RESERVED_2[2];
  __IO uint16_t MISC_CTRL;                         /**< BLUETOOTH LOW ENERGY MISCELLANEOUS CONTROL, offset: 0x608 */
} BTLE_RF_Type;

/* ----------------------------------------------------------------------------
   -- BTLE_RF Register Masks
   ---------------------------------------------------------------------------- */

/*!
 * @addtogroup BTLE_RF_Register_Masks BTLE_RF Register Masks
 * @{
 */

/*! @name BLE_PART_ID - BLUETOOTH LOW ENERGY PART ID */
#define BTLE_RF_BLE_PART_ID_BLE_PART_ID_MASK     (0xFFFFU)
#define BTLE_RF_BLE_PART_ID_BLE_PART_ID_SHIFT    (0U)
#define BTLE_RF_BLE_PART_ID_BLE_PART_ID(x)       (((uint16_t)(((uint16_t)(x)) << BTLE_RF_BLE_PART_ID_BLE_PART_ID_SHIFT)) & BTLE_RF_BLE_PART_ID_BLE_PART_ID_MASK)

/*! @name DSM_STATUS - BLE DSM STATUS */
#define BTLE_RF_DSM_STATUS_ORF_SYSCLK_REQ_MASK   (0x1U)
#define BTLE_RF_DSM_STATUS_ORF_SYSCLK_REQ_SHIFT  (0U)
#define BTLE_RF_DSM_STATUS_ORF_SYSCLK_REQ(x)     (((uint16_t)(((uint16_t)(x)) << BTLE_RF_DSM_STATUS_ORF_SYSCLK_REQ_SHIFT)) & BTLE_RF_DSM_STATUS_ORF_SYSCLK_REQ_MASK)
#define BTLE_RF_DSM_STATUS_RIF_LL_ACTIVE_MASK    (0x2U)
#define BTLE_RF_DSM_STATUS_RIF_LL_ACTIVE_SHIFT   (1U)
#define BTLE_RF_DSM_STATUS_RIF_LL_ACTIVE(x)      (((uint16_t)(((uint16_t)(x)) << BTLE_RF_DSM_STATUS_RIF_LL_ACTIVE_SHIFT)) & BTLE_RF_DSM_STATUS_RIF_LL_ACTIVE_MASK)
#define BTLE_RF_DSM_STATUS_XCVR_BUSY_MASK        (0x4U)
#define BTLE_RF_DSM_STATUS_XCVR_BUSY_SHIFT       (2U)
#define BTLE_RF_DSM_STATUS_XCVR_BUSY(x)          (((uint16_t)(((uint16_t)(x)) << BTLE_RF_DSM_STATUS_XCVR_BUSY_SHIFT)) & BTLE_RF_DSM_STATUS_XCVR_BUSY_MASK)

/*! @name MISC_CTRL - BLUETOOTH LOW ENERGY MISCELLANEOUS CONTROL */
#define BTLE_RF_MISC_CTRL_TSM_INTR_EN_MASK       (0x2U)
#define BTLE_RF_MISC_CTRL_TSM_INTR_EN_SHIFT      (1U)
#define BTLE_RF_MISC_CTRL_TSM_INTR_EN(x)         (((uint16_t)(((uint16_t)(x)) << BTLE_RF_MISC_CTRL_TSM_INTR_EN_SHIFT)) & BTLE_RF_MISC_CTRL_TSM_INTR_EN_MASK)


/*!
 * @}
 */ /* end of group BTLE_RF_Register_Masks */


/* BTLE_RF - Peripheral instance base addresses */
/** Peripheral BTLE_RF base address */
#define BTLE_RF_BASE                             (0x4005B000u)
/** Peripheral BTLE_RF base pointer */
#define BTLE_RF                                  ((BTLE_RF_Type *)BTLE_RF_BASE)
/** Array initializer of BTLE_RF peripheral base addresses */
#define BTLE_RF_BASE_ADDRS                       { BTLE_RF_BASE }
/** Array initializer of BTLE_RF peripheral base pointers */
#define BTLE_RF_BASE_PTRS                        { BTLE_RF }

/*!
 * @}
 */ /* end of group BTLE_RF_Peripheral_Access_Layer */


/* ----------------------------------------------------------------------------
   -- CMP Peripheral Access Layer
   ---------------------------------------------------------------------------- */

/*!
 * @addtogroup CMP_Peripheral_Access_Layer CMP Peripheral Access Layer
 * @{
 */

/** CMP - Register Layout Typedef */
typedef struct {
  __IO uint8_t CR0;                                /**< CMP Control Register 0, offset: 0x0 */
  __IO uint8_t CR1;                                /**< CMP Control Register 1, offset: 0x1 */
  __IO uint8_t FPR;                                /**< CMP Filter Period Register, offset: 0x2 */
  __IO uint8_t SCR;                                /**< CMP Status and Control Register, offset: 0x3 */
  __IO uint8_t DACCR;                              /**< DAC Control Register, offset: 0x4 */
  __IO uint8_t MUXCR;                              /**< MUX Control Register, offset: 0x5 */
} CMP_Type;

/* ----------------------------------------------------------------------------
   -- CMP Register Masks
   ---------------------------------------------------------------------------- */

/*!
 * @addtogroup CMP_Register_Masks CMP Register Masks
 * @{
 */

/*! @name CR0 - CMP Control Register 0 */
#define CMP_CR0_HYSTCTR_MASK                     (0x3U)
#define CMP_CR0_HYSTCTR_SHIFT                    (0U)
#define CMP_CR0_HYSTCTR(x)                       (((uint8_t)(((uint8_t)(x)) << CMP_CR0_HYSTCTR_SHIFT)) & CMP_CR0_HYSTCTR_MASK)
#define CMP_CR0_FILTER_CNT_MASK                  (0x70U)
#define CMP_CR0_FILTER_CNT_SHIFT                 (4U)
#define CMP_CR0_FILTER_CNT(x)                    (((uint8_t)(((uint8_t)(x)) << CMP_CR0_FILTER_CNT_SHIFT)) & CMP_CR0_FILTER_CNT_MASK)

/*! @name CR1 - CMP Control Register 1 */
#define CMP_CR1_EN_MASK                          (0x1U)
#define CMP_CR1_EN_SHIFT                         (0U)
#define CMP_CR1_EN(x)                            (((uint8_t)(((uint8_t)(x)) << CMP_CR1_EN_SHIFT)) & CMP_CR1_EN_MASK)
#define CMP_CR1_OPE_MASK                         (0x2U)
#define CMP_CR1_OPE_SHIFT                        (1U)
#define CMP_CR1_OPE(x)                           (((uint8_t)(((uint8_t)(x)) << CMP_CR1_OPE_SHIFT)) & CMP_CR1_OPE_MASK)
#define CMP_CR1_COS_MASK                         (0x4U)
#define CMP_CR1_COS_SHIFT                        (2U)
#define CMP_CR1_COS(x)                           (((uint8_t)(((uint8_t)(x)) << CMP_CR1_COS_SHIFT)) & CMP_CR1_COS_MASK)
#define CMP_CR1_INV_MASK                         (0x8U)
#define CMP_CR1_INV_SHIFT                        (3U)
#define CMP_CR1_INV(x)                           (((uint8_t)(((uint8_t)(x)) << CMP_CR1_INV_SHIFT)) & CMP_CR1_INV_MASK)
#define CMP_CR1_PMODE_MASK                       (0x10U)
#define CMP_CR1_PMODE_SHIFT                      (4U)
#define CMP_CR1_PMODE(x)                         (((uint8_t)(((uint8_t)(x)) << CMP_CR1_PMODE_SHIFT)) & CMP_CR1_PMODE_MASK)
#define CMP_CR1_TRIGM_MASK                       (0x20U)
#define CMP_CR1_TRIGM_SHIFT                      (5U)
#define CMP_CR1_TRIGM(x)                         (((uint8_t)(((uint8_t)(x)) << CMP_CR1_TRIGM_SHIFT)) & CMP_CR1_TRIGM_MASK)
#define CMP_CR1_WE_MASK                          (0x40U)
#define CMP_CR1_WE_SHIFT                         (6U)
#define CMP_CR1_WE(x)                            (((uint8_t)(((uint8_t)(x)) << CMP_CR1_WE_SHIFT)) & CMP_CR1_WE_MASK)
#define CMP_CR1_SE_MASK                          (0x80U)
#define CMP_CR1_SE_SHIFT                         (7U)
#define CMP_CR1_SE(x)                            (((uint8_t)(((uint8_t)(x)) << CMP_CR1_SE_SHIFT)) & CMP_CR1_SE_MASK)

/*! @name FPR - CMP Filter Period Register */
#define CMP_FPR_FILT_PER_MASK                    (0xFFU)
#define CMP_FPR_FILT_PER_SHIFT                   (0U)
#define CMP_FPR_FILT_PER(x)                      (((uint8_t)(((uint8_t)(x)) << CMP_FPR_FILT_PER_SHIFT)) & CMP_FPR_FILT_PER_MASK)

/*! @name SCR - CMP Status and Control Register */
#define CMP_SCR_COUT_MASK                        (0x1U)
#define CMP_SCR_COUT_SHIFT                       (0U)
#define CMP_SCR_COUT(x)                          (((uint8_t)(((uint8_t)(x)) << CMP_SCR_COUT_SHIFT)) & CMP_SCR_COUT_MASK)
#define CMP_SCR_CFF_MASK                         (0x2U)
#define CMP_SCR_CFF_SHIFT                        (1U)
#define CMP_SCR_CFF(x)                           (((uint8_t)(((uint8_t)(x)) << CMP_SCR_CFF_SHIFT)) & CMP_SCR_CFF_MASK)
#define CMP_SCR_CFR_MASK                         (0x4U)
#define CMP_SCR_CFR_SHIFT                        (2U)
#define CMP_SCR_CFR(x)                           (((uint8_t)(((uint8_t)(x)) << CMP_SCR_CFR_SHIFT)) & CMP_SCR_CFR_MASK)
#define CMP_SCR_IEF_MASK                         (0x8U)
#define CMP_SCR_IEF_SHIFT                        (3U)
#define CMP_SCR_IEF(x)                           (((uint8_t)(((uint8_t)(x)) << CMP_SCR_IEF_SHIFT)) & CMP_SCR_IEF_MASK)
#define CMP_SCR_IER_MASK                         (0x10U)
#define CMP_SCR_IER_SHIFT                        (4U)
#define CMP_SCR_IER(x)                           (((uint8_t)(((uint8_t)(x)) << CMP_SCR_IER_SHIFT)) & CMP_SCR_IER_MASK)
#define CMP_SCR_DMAEN_MASK                       (0x40U)
#define CMP_SCR_DMAEN_SHIFT                      (6U)
#define CMP_SCR_DMAEN(x)                         (((uint8_t)(((uint8_t)(x)) << CMP_SCR_DMAEN_SHIFT)) & CMP_SCR_DMAEN_MASK)

/*! @name DACCR - DAC Control Register */
#define CMP_DACCR_VOSEL_MASK                     (0x3FU)
#define CMP_DACCR_VOSEL_SHIFT                    (0U)
#define CMP_DACCR_VOSEL(x)                       (((uint8_t)(((uint8_t)(x)) << CMP_DACCR_VOSEL_SHIFT)) & CMP_DACCR_VOSEL_MASK)
#define CMP_DACCR_VRSEL_MASK                     (0x40U)
#define CMP_DACCR_VRSEL_SHIFT                    (6U)
#define CMP_DACCR_VRSEL(x)                       (((uint8_t)(((uint8_t)(x)) << CMP_DACCR_VRSEL_SHIFT)) & CMP_DACCR_VRSEL_MASK)
#define CMP_DACCR_DACEN_MASK                     (0x80U)
#define CMP_DACCR_DACEN_SHIFT                    (7U)
#define CMP_DACCR_DACEN(x)                       (((uint8_t)(((uint8_t)(x)) << CMP_DACCR_DACEN_SHIFT)) & CMP_DACCR_DACEN_MASK)

/*! @name MUXCR - MUX Control Register */
#define CMP_MUXCR_MSEL_MASK                      (0x7U)
#define CMP_MUXCR_MSEL_SHIFT                     (0U)
#define CMP_MUXCR_MSEL(x)                        (((uint8_t)(((uint8_t)(x)) << CMP_MUXCR_MSEL_SHIFT)) & CMP_MUXCR_MSEL_MASK)
#define CMP_MUXCR_PSEL_MASK                      (0x38U)
#define CMP_MUXCR_PSEL_SHIFT                     (3U)
#define CMP_MUXCR_PSEL(x)                        (((uint8_t)(((uint8_t)(x)) << CMP_MUXCR_PSEL_SHIFT)) & CMP_MUXCR_PSEL_MASK)
#define CMP_MUXCR_PSTM_MASK                      (0x80U)
#define CMP_MUXCR_PSTM_SHIFT                     (7U)
#define CMP_MUXCR_PSTM(x)                        (((uint8_t)(((uint8_t)(x)) << CMP_MUXCR_PSTM_SHIFT)) & CMP_MUXCR_PSTM_MASK)


/*!
 * @}
 */ /* end of group CMP_Register_Masks */


/* CMP - Peripheral instance base addresses */
/** Peripheral CMP0 base address */
#define CMP0_BASE                                (0x40073000u)
/** Peripheral CMP0 base pointer */
#define CMP0                                     ((CMP_Type *)CMP0_BASE)
/** Array initializer of CMP peripheral base addresses */
#define CMP_BASE_ADDRS                           { CMP0_BASE }
/** Array initializer of CMP peripheral base pointers */
#define CMP_BASE_PTRS                            { CMP0 }
/** Interrupt vectors for the CMP peripheral type */
#define CMP_IRQS                                 { CMP0_IRQn }

/*!
 * @}
 */ /* end of group CMP_Peripheral_Access_Layer */


/* ----------------------------------------------------------------------------
   -- CMT Peripheral Access Layer
   ---------------------------------------------------------------------------- */

/*!
 * @addtogroup CMT_Peripheral_Access_Layer CMT Peripheral Access Layer
 * @{
 */

/** CMT - Register Layout Typedef */
typedef struct {
  __IO uint8_t CGH1;                               /**< CMT Carrier Generator High Data Register 1, offset: 0x0 */
  __IO uint8_t CGL1;                               /**< CMT Carrier Generator Low Data Register 1, offset: 0x1 */
  __IO uint8_t CGH2;                               /**< CMT Carrier Generator High Data Register 2, offset: 0x2 */
  __IO uint8_t CGL2;                               /**< CMT Carrier Generator Low Data Register 2, offset: 0x3 */
  __IO uint8_t OC;                                 /**< CMT Output Control Register, offset: 0x4 */
  __IO uint8_t MSC;                                /**< CMT Modulator Status and Control Register, offset: 0x5 */
  __IO uint8_t CMD1;                               /**< CMT Modulator Data Register Mark High, offset: 0x6 */
  __IO uint8_t CMD2;                               /**< CMT Modulator Data Register Mark Low, offset: 0x7 */
  __IO uint8_t CMD3;                               /**< CMT Modulator Data Register Space High, offset: 0x8 */
  __IO uint8_t CMD4;                               /**< CMT Modulator Data Register Space Low, offset: 0x9 */
  __IO uint8_t PPS;                                /**< CMT Primary Prescaler Register, offset: 0xA */
  __IO uint8_t DMA;                                /**< CMT Direct Memory Access Register, offset: 0xB */
} CMT_Type;

/* ----------------------------------------------------------------------------
   -- CMT Register Masks
   ---------------------------------------------------------------------------- */

/*!
 * @addtogroup CMT_Register_Masks CMT Register Masks
 * @{
 */

/*! @name CGH1 - CMT Carrier Generator High Data Register 1 */
#define CMT_CGH1_PH_MASK                         (0xFFU)
#define CMT_CGH1_PH_SHIFT                        (0U)
#define CMT_CGH1_PH(x)                           (((uint8_t)(((uint8_t)(x)) << CMT_CGH1_PH_SHIFT)) & CMT_CGH1_PH_MASK)

/*! @name CGL1 - CMT Carrier Generator Low Data Register 1 */
#define CMT_CGL1_PL_MASK                         (0xFFU)
#define CMT_CGL1_PL_SHIFT                        (0U)
#define CMT_CGL1_PL(x)                           (((uint8_t)(((uint8_t)(x)) << CMT_CGL1_PL_SHIFT)) & CMT_CGL1_PL_MASK)

/*! @name CGH2 - CMT Carrier Generator High Data Register 2 */
#define CMT_CGH2_SH_MASK                         (0xFFU)
#define CMT_CGH2_SH_SHIFT                        (0U)
#define CMT_CGH2_SH(x)                           (((uint8_t)(((uint8_t)(x)) << CMT_CGH2_SH_SHIFT)) & CMT_CGH2_SH_MASK)

/*! @name CGL2 - CMT Carrier Generator Low Data Register 2 */
#define CMT_CGL2_SL_MASK                         (0xFFU)
#define CMT_CGL2_SL_SHIFT                        (0U)
#define CMT_CGL2_SL(x)                           (((uint8_t)(((uint8_t)(x)) << CMT_CGL2_SL_SHIFT)) & CMT_CGL2_SL_MASK)

/*! @name OC - CMT Output Control Register */
#define CMT_OC_IROPEN_MASK                       (0x20U)
#define CMT_OC_IROPEN_SHIFT                      (5U)
#define CMT_OC_IROPEN(x)                         (((uint8_t)(((uint8_t)(x)) << CMT_OC_IROPEN_SHIFT)) & CMT_OC_IROPEN_MASK)
#define CMT_OC_CMTPOL_MASK                       (0x40U)
#define CMT_OC_CMTPOL_SHIFT                      (6U)
#define CMT_OC_CMTPOL(x)                         (((uint8_t)(((uint8_t)(x)) << CMT_OC_CMTPOL_SHIFT)) & CMT_OC_CMTPOL_MASK)
#define CMT_OC_IROL_MASK                         (0x80U)
#define CMT_OC_IROL_SHIFT                        (7U)
#define CMT_OC_IROL(x)                           (((uint8_t)(((uint8_t)(x)) << CMT_OC_IROL_SHIFT)) & CMT_OC_IROL_MASK)

/*! @name MSC - CMT Modulator Status and Control Register */
#define CMT_MSC_MCGEN_MASK                       (0x1U)
#define CMT_MSC_MCGEN_SHIFT                      (0U)
#define CMT_MSC_MCGEN(x)                         (((uint8_t)(((uint8_t)(x)) << CMT_MSC_MCGEN_SHIFT)) & CMT_MSC_MCGEN_MASK)
#define CMT_MSC_EOCIE_MASK                       (0x2U)
#define CMT_MSC_EOCIE_SHIFT                      (1U)
#define CMT_MSC_EOCIE(x)                         (((uint8_t)(((uint8_t)(x)) << CMT_MSC_EOCIE_SHIFT)) & CMT_MSC_EOCIE_MASK)
#define CMT_MSC_FSK_MASK                         (0x4U)
#define CMT_MSC_FSK_SHIFT                        (2U)
#define CMT_MSC_FSK(x)                           (((uint8_t)(((uint8_t)(x)) << CMT_MSC_FSK_SHIFT)) & CMT_MSC_FSK_MASK)
#define CMT_MSC_BASE_MASK                        (0x8U)
#define CMT_MSC_BASE_SHIFT                       (3U)
#define CMT_MSC_BASE(x)                          (((uint8_t)(((uint8_t)(x)) << CMT_MSC_BASE_SHIFT)) & CMT_MSC_BASE_MASK)
#define CMT_MSC_EXSPC_MASK                       (0x10U)
#define CMT_MSC_EXSPC_SHIFT                      (4U)
#define CMT_MSC_EXSPC(x)                         (((uint8_t)(((uint8_t)(x)) << CMT_MSC_EXSPC_SHIFT)) & CMT_MSC_EXSPC_MASK)
#define CMT_MSC_CMTDIV_MASK                      (0x60U)
#define CMT_MSC_CMTDIV_SHIFT                     (5U)
#define CMT_MSC_CMTDIV(x)                        (((uint8_t)(((uint8_t)(x)) << CMT_MSC_CMTDIV_SHIFT)) & CMT_MSC_CMTDIV_MASK)
#define CMT_MSC_EOCF_MASK                        (0x80U)
#define CMT_MSC_EOCF_SHIFT                       (7U)
#define CMT_MSC_EOCF(x)                          (((uint8_t)(((uint8_t)(x)) << CMT_MSC_EOCF_SHIFT)) & CMT_MSC_EOCF_MASK)

/*! @name CMD1 - CMT Modulator Data Register Mark High */
#define CMT_CMD1_MB_MASK                         (0xFFU)
#define CMT_CMD1_MB_SHIFT                        (0U)
#define CMT_CMD1_MB(x)                           (((uint8_t)(((uint8_t)(x)) << CMT_CMD1_MB_SHIFT)) & CMT_CMD1_MB_MASK)

/*! @name CMD2 - CMT Modulator Data Register Mark Low */
#define CMT_CMD2_MB_MASK                         (0xFFU)
#define CMT_CMD2_MB_SHIFT                        (0U)
#define CMT_CMD2_MB(x)                           (((uint8_t)(((uint8_t)(x)) << CMT_CMD2_MB_SHIFT)) & CMT_CMD2_MB_MASK)

/*! @name CMD3 - CMT Modulator Data Register Space High */
#define CMT_CMD3_SB_MASK                         (0xFFU)
#define CMT_CMD3_SB_SHIFT                        (0U)
#define CMT_CMD3_SB(x)                           (((uint8_t)(((uint8_t)(x)) << CMT_CMD3_SB_SHIFT)) & CMT_CMD3_SB_MASK)

/*! @name CMD4 - CMT Modulator Data Register Space Low */
#define CMT_CMD4_SB_MASK                         (0xFFU)
#define CMT_CMD4_SB_SHIFT                        (0U)
#define CMT_CMD4_SB(x)                           (((uint8_t)(((uint8_t)(x)) << CMT_CMD4_SB_SHIFT)) & CMT_CMD4_SB_MASK)

/*! @name PPS - CMT Primary Prescaler Register */
#define CMT_PPS_PPSDIV_MASK                      (0xFU)
#define CMT_PPS_PPSDIV_SHIFT                     (0U)
#define CMT_PPS_PPSDIV(x)                        (((uint8_t)(((uint8_t)(x)) << CMT_PPS_PPSDIV_SHIFT)) & CMT_PPS_PPSDIV_MASK)

/*! @name DMA - CMT Direct Memory Access Register */
#define CMT_DMA_DMA_MASK                         (0x1U)
#define CMT_DMA_DMA_SHIFT                        (0U)
#define CMT_DMA_DMA(x)                           (((uint8_t)(((uint8_t)(x)) << CMT_DMA_DMA_SHIFT)) & CMT_DMA_DMA_MASK)


/*!
 * @}
 */ /* end of group CMT_Register_Masks */


/* CMT - Peripheral instance base addresses */
/** Peripheral CMT base address */
#define CMT_BASE                                 (0x40062000u)
/** Peripheral CMT base pointer */
#define CMT                                      ((CMT_Type *)CMT_BASE)
/** Array initializer of CMT peripheral base addresses */
#define CMT_BASE_ADDRS                           { CMT_BASE }
/** Array initializer of CMT peripheral base pointers */
#define CMT_BASE_PTRS                            { CMT }
/** Interrupt vectors for the CMT peripheral type */
#define CMT_IRQS                                 { CMT_IRQn }

/*!
 * @}
 */ /* end of group CMT_Peripheral_Access_Layer */


/* ----------------------------------------------------------------------------
   -- DAC Peripheral Access Layer
   ---------------------------------------------------------------------------- */

/*!
 * @addtogroup DAC_Peripheral_Access_Layer DAC Peripheral Access Layer
 * @{
 */

/** DAC - Register Layout Typedef */
typedef struct {
  struct {                                         /* offset: 0x0, array step: 0x2 */
    __IO uint8_t DATL;                               /**< DAC Data Low Register, array offset: 0x0, array step: 0x2 */
    __IO uint8_t DATH;                               /**< DAC Data High Register, array offset: 0x1, array step: 0x2 */
  } DAT[2];
       uint8_t RESERVED_0[28];
  __IO uint8_t SR;                                 /**< DAC Status Register, offset: 0x20 */
  __IO uint8_t C0;                                 /**< DAC Control Register, offset: 0x21 */
  __IO uint8_t C1;                                 /**< DAC Control Register 1, offset: 0x22 */
  __IO uint8_t C2;                                 /**< DAC Control Register 2, offset: 0x23 */
} DAC_Type;

/* ----------------------------------------------------------------------------
   -- DAC Register Masks
   ---------------------------------------------------------------------------- */

/*!
 * @addtogroup DAC_Register_Masks DAC Register Masks
 * @{
 */

/*! @name DATL - DAC Data Low Register */
#define DAC_DATL_DATA0_MASK                      (0xFFU)
#define DAC_DATL_DATA0_SHIFT                     (0U)
#define DAC_DATL_DATA0(x)                        (((uint8_t)(((uint8_t)(x)) << DAC_DATL_DATA0_SHIFT)) & DAC_DATL_DATA0_MASK)

/* The count of DAC_DATL */
#define DAC_DATL_COUNT                           (2U)

/*! @name DATH - DAC Data High Register */
#define DAC_DATH_DATA1_MASK                      (0xFU)
#define DAC_DATH_DATA1_SHIFT                     (0U)
#define DAC_DATH_DATA1(x)                        (((uint8_t)(((uint8_t)(x)) << DAC_DATH_DATA1_SHIFT)) & DAC_DATH_DATA1_MASK)

/* The count of DAC_DATH */
#define DAC_DATH_COUNT                           (2U)

/*! @name SR - DAC Status Register */
#define DAC_SR_DACBFRPBF_MASK                    (0x1U)
#define DAC_SR_DACBFRPBF_SHIFT                   (0U)
#define DAC_SR_DACBFRPBF(x)                      (((uint8_t)(((uint8_t)(x)) << DAC_SR_DACBFRPBF_SHIFT)) & DAC_SR_DACBFRPBF_MASK)
#define DAC_SR_DACBFRPTF_MASK                    (0x2U)
#define DAC_SR_DACBFRPTF_SHIFT                   (1U)
#define DAC_SR_DACBFRPTF(x)                      (((uint8_t)(((uint8_t)(x)) << DAC_SR_DACBFRPTF_SHIFT)) & DAC_SR_DACBFRPTF_MASK)
#define DAC_SR_DACBFWMF_MASK                     (0x4U)
#define DAC_SR_DACBFWMF_SHIFT                    (2U)
#define DAC_SR_DACBFWMF(x)                       (((uint8_t)(((uint8_t)(x)) << DAC_SR_DACBFWMF_SHIFT)) & DAC_SR_DACBFWMF_MASK)

/*! @name C0 - DAC Control Register */
#define DAC_C0_DACBBIEN_MASK                     (0x1U)
#define DAC_C0_DACBBIEN_SHIFT                    (0U)
#define DAC_C0_DACBBIEN(x)                       (((uint8_t)(((uint8_t)(x)) << DAC_C0_DACBBIEN_SHIFT)) & DAC_C0_DACBBIEN_MASK)
#define DAC_C0_DACBTIEN_MASK                     (0x2U)
#define DAC_C0_DACBTIEN_SHIFT                    (1U)
#define DAC_C0_DACBTIEN(x)                       (((uint8_t)(((uint8_t)(x)) << DAC_C0_DACBTIEN_SHIFT)) & DAC_C0_DACBTIEN_MASK)
#define DAC_C0_DACBWIEN_MASK                     (0x4U)
#define DAC_C0_DACBWIEN_SHIFT                    (2U)
#define DAC_C0_DACBWIEN(x)                       (((uint8_t)(((uint8_t)(x)) << DAC_C0_DACBWIEN_SHIFT)) & DAC_C0_DACBWIEN_MASK)
#define DAC_C0_LPEN_MASK                         (0x8U)
#define DAC_C0_LPEN_SHIFT                        (3U)
#define DAC_C0_LPEN(x)                           (((uint8_t)(((uint8_t)(x)) << DAC_C0_LPEN_SHIFT)) & DAC_C0_LPEN_MASK)
#define DAC_C0_DACSWTRG_MASK                     (0x10U)
#define DAC_C0_DACSWTRG_SHIFT                    (4U)
#define DAC_C0_DACSWTRG(x)                       (((uint8_t)(((uint8_t)(x)) << DAC_C0_DACSWTRG_SHIFT)) & DAC_C0_DACSWTRG_MASK)
#define DAC_C0_DACTRGSEL_MASK                    (0x20U)
#define DAC_C0_DACTRGSEL_SHIFT                   (5U)
#define DAC_C0_DACTRGSEL(x)                      (((uint8_t)(((uint8_t)(x)) << DAC_C0_DACTRGSEL_SHIFT)) & DAC_C0_DACTRGSEL_MASK)
#define DAC_C0_DACRFS_MASK                       (0x40U)
#define DAC_C0_DACRFS_SHIFT                      (6U)
#define DAC_C0_DACRFS(x)                         (((uint8_t)(((uint8_t)(x)) << DAC_C0_DACRFS_SHIFT)) & DAC_C0_DACRFS_MASK)
#define DAC_C0_DACEN_MASK                        (0x80U)
#define DAC_C0_DACEN_SHIFT                       (7U)
#define DAC_C0_DACEN(x)                          (((uint8_t)(((uint8_t)(x)) << DAC_C0_DACEN_SHIFT)) & DAC_C0_DACEN_MASK)

/*! @name C1 - DAC Control Register 1 */
#define DAC_C1_DACBFEN_MASK                      (0x1U)
#define DAC_C1_DACBFEN_SHIFT                     (0U)
#define DAC_C1_DACBFEN(x)                        (((uint8_t)(((uint8_t)(x)) << DAC_C1_DACBFEN_SHIFT)) & DAC_C1_DACBFEN_MASK)
#define DAC_C1_DACBFMD_MASK                      (0x4U)
#define DAC_C1_DACBFMD_SHIFT                     (2U)
#define DAC_C1_DACBFMD(x)                        (((uint8_t)(((uint8_t)(x)) << DAC_C1_DACBFMD_SHIFT)) & DAC_C1_DACBFMD_MASK)
#define DAC_C1_DACBFWM_MASK                      (0x18U)
#define DAC_C1_DACBFWM_SHIFT                     (3U)
#define DAC_C1_DACBFWM(x)                        (((uint8_t)(((uint8_t)(x)) << DAC_C1_DACBFWM_SHIFT)) & DAC_C1_DACBFWM_MASK)
#define DAC_C1_DMAEN_MASK                        (0x80U)
#define DAC_C1_DMAEN_SHIFT                       (7U)
#define DAC_C1_DMAEN(x)                          (((uint8_t)(((uint8_t)(x)) << DAC_C1_DMAEN_SHIFT)) & DAC_C1_DMAEN_MASK)

/*! @name C2 - DAC Control Register 2 */
#define DAC_C2_DACBFUP_MASK                      (0x1U)
#define DAC_C2_DACBFUP_SHIFT                     (0U)
#define DAC_C2_DACBFUP(x)                        (((uint8_t)(((uint8_t)(x)) << DAC_C2_DACBFUP_SHIFT)) & DAC_C2_DACBFUP_MASK)
#define DAC_C2_DACBFRP_MASK                      (0x10U)
#define DAC_C2_DACBFRP_SHIFT                     (4U)
#define DAC_C2_DACBFRP(x)                        (((uint8_t)(((uint8_t)(x)) << DAC_C2_DACBFRP_SHIFT)) & DAC_C2_DACBFRP_MASK)


/*!
 * @}
 */ /* end of group DAC_Register_Masks */


/* DAC - Peripheral instance base addresses */
/** Peripheral DAC0 base address */
#define DAC0_BASE                                (0x4003F000u)
/** Peripheral DAC0 base pointer */
#define DAC0                                     ((DAC_Type *)DAC0_BASE)
/** Array initializer of DAC peripheral base addresses */
#define DAC_BASE_ADDRS                           { DAC0_BASE }
/** Array initializer of DAC peripheral base pointers */
#define DAC_BASE_PTRS                            { DAC0 }
/** Interrupt vectors for the DAC peripheral type */
#define DAC_IRQS                                 { DAC0_IRQn }

/*!
 * @}
 */ /* end of group DAC_Peripheral_Access_Layer */


/* ----------------------------------------------------------------------------
   -- DCDC Peripheral Access Layer
   ---------------------------------------------------------------------------- */

/*!
 * @addtogroup DCDC_Peripheral_Access_Layer DCDC Peripheral Access Layer
 * @{
 */

/** DCDC - Register Layout Typedef */
typedef struct {
  __IO uint32_t REG0;                              /**< DCDC REGISTER 0, offset: 0x0 */
  __IO uint32_t REG1;                              /**< DCDC REGISTER 1, offset: 0x4 */
  __IO uint32_t REG2;                              /**< DCDC REGISTER 2, offset: 0x8 */
  __IO uint32_t REG3;                              /**< DCDC REGISTER 3, offset: 0xC */
  __IO uint32_t REG4;                              /**< DCDC REGISTER 4, offset: 0x10 */
       uint8_t RESERVED_0[4];
  __IO uint32_t REG6;                              /**< DCDC REGISTER 6, offset: 0x18 */
  __IO uint32_t REG7;                              /**< DCDC REGISTER 7, offset: 0x1C */
} DCDC_Type;

/* ----------------------------------------------------------------------------
   -- DCDC Register Masks
   ---------------------------------------------------------------------------- */

/*!
 * @addtogroup DCDC_Register_Masks DCDC Register Masks
 * @{
 */

/*! @name REG0 - DCDC REGISTER 0 */
#define DCDC_REG0_DCDC_DISABLE_AUTO_CLK_SWITCH_MASK (0x2U)
#define DCDC_REG0_DCDC_DISABLE_AUTO_CLK_SWITCH_SHIFT (1U)
#define DCDC_REG0_DCDC_DISABLE_AUTO_CLK_SWITCH(x) (((uint32_t)(((uint32_t)(x)) << DCDC_REG0_DCDC_DISABLE_AUTO_CLK_SWITCH_SHIFT)) & DCDC_REG0_DCDC_DISABLE_AUTO_CLK_SWITCH_MASK)
#define DCDC_REG0_DCDC_SEL_CLK_MASK              (0x4U)
#define DCDC_REG0_DCDC_SEL_CLK_SHIFT             (2U)
#define DCDC_REG0_DCDC_SEL_CLK(x)                (((uint32_t)(((uint32_t)(x)) << DCDC_REG0_DCDC_SEL_CLK_SHIFT)) & DCDC_REG0_DCDC_SEL_CLK_MASK)
#define DCDC_REG0_DCDC_PWD_OSC_INT_MASK          (0x8U)
#define DCDC_REG0_DCDC_PWD_OSC_INT_SHIFT         (3U)
#define DCDC_REG0_DCDC_PWD_OSC_INT(x)            (((uint32_t)(((uint32_t)(x)) << DCDC_REG0_DCDC_PWD_OSC_INT_SHIFT)) & DCDC_REG0_DCDC_PWD_OSC_INT_MASK)
#define DCDC_REG0_DCDC_LP_DF_CMP_ENABLE_MASK     (0x200U)
#define DCDC_REG0_DCDC_LP_DF_CMP_ENABLE_SHIFT    (9U)
#define DCDC_REG0_DCDC_LP_DF_CMP_ENABLE(x)       (((uint32_t)(((uint32_t)(x)) << DCDC_REG0_DCDC_LP_DF_CMP_ENABLE_SHIFT)) & DCDC_REG0_DCDC_LP_DF_CMP_ENABLE_MASK)
#define DCDC_REG0_DCDC_VBAT_DIV_CTRL_MASK        (0xC00U)
#define DCDC_REG0_DCDC_VBAT_DIV_CTRL_SHIFT       (10U)
#define DCDC_REG0_DCDC_VBAT_DIV_CTRL(x)          (((uint32_t)(((uint32_t)(x)) << DCDC_REG0_DCDC_VBAT_DIV_CTRL_SHIFT)) & DCDC_REG0_DCDC_VBAT_DIV_CTRL_MASK)
#define DCDC_REG0_DCDC_LP_STATE_HYS_L_MASK       (0x60000U)
#define DCDC_REG0_DCDC_LP_STATE_HYS_L_SHIFT      (17U)
#define DCDC_REG0_DCDC_LP_STATE_HYS_L(x)         (((uint32_t)(((uint32_t)(x)) << DCDC_REG0_DCDC_LP_STATE_HYS_L_SHIFT)) & DCDC_REG0_DCDC_LP_STATE_HYS_L_MASK)
#define DCDC_REG0_DCDC_LP_STATE_HYS_H_MASK       (0x180000U)
#define DCDC_REG0_DCDC_LP_STATE_HYS_H_SHIFT      (19U)
#define DCDC_REG0_DCDC_LP_STATE_HYS_H(x)         (((uint32_t)(((uint32_t)(x)) << DCDC_REG0_DCDC_LP_STATE_HYS_H_SHIFT)) & DCDC_REG0_DCDC_LP_STATE_HYS_H_MASK)
#define DCDC_REG0_HYST_LP_COMP_ADJ_MASK          (0x200000U)
#define DCDC_REG0_HYST_LP_COMP_ADJ_SHIFT         (21U)
#define DCDC_REG0_HYST_LP_COMP_ADJ(x)            (((uint32_t)(((uint32_t)(x)) << DCDC_REG0_HYST_LP_COMP_ADJ_SHIFT)) & DCDC_REG0_HYST_LP_COMP_ADJ_MASK)
#define DCDC_REG0_HYST_LP_CMP_DISABLE_MASK       (0x400000U)
#define DCDC_REG0_HYST_LP_CMP_DISABLE_SHIFT      (22U)
#define DCDC_REG0_HYST_LP_CMP_DISABLE(x)         (((uint32_t)(((uint32_t)(x)) << DCDC_REG0_HYST_LP_CMP_DISABLE_SHIFT)) & DCDC_REG0_HYST_LP_CMP_DISABLE_MASK)
#define DCDC_REG0_OFFSET_RSNS_LP_ADJ_MASK        (0x800000U)
#define DCDC_REG0_OFFSET_RSNS_LP_ADJ_SHIFT       (23U)
#define DCDC_REG0_OFFSET_RSNS_LP_ADJ(x)          (((uint32_t)(((uint32_t)(x)) << DCDC_REG0_OFFSET_RSNS_LP_ADJ_SHIFT)) & DCDC_REG0_OFFSET_RSNS_LP_ADJ_MASK)
#define DCDC_REG0_OFFSET_RSNS_LP_DISABLE_MASK    (0x1000000U)
#define DCDC_REG0_OFFSET_RSNS_LP_DISABLE_SHIFT   (24U)
#define DCDC_REG0_OFFSET_RSNS_LP_DISABLE(x)      (((uint32_t)(((uint32_t)(x)) << DCDC_REG0_OFFSET_RSNS_LP_DISABLE_SHIFT)) & DCDC_REG0_OFFSET_RSNS_LP_DISABLE_MASK)
#define DCDC_REG0_DCDC_LESS_I_MASK               (0x2000000U)
#define DCDC_REG0_DCDC_LESS_I_SHIFT              (25U)
#define DCDC_REG0_DCDC_LESS_I(x)                 (((uint32_t)(((uint32_t)(x)) << DCDC_REG0_DCDC_LESS_I_SHIFT)) & DCDC_REG0_DCDC_LESS_I_MASK)
#define DCDC_REG0_PWD_CMP_OFFSET_MASK            (0x4000000U)
#define DCDC_REG0_PWD_CMP_OFFSET_SHIFT           (26U)
#define DCDC_REG0_PWD_CMP_OFFSET(x)              (((uint32_t)(((uint32_t)(x)) << DCDC_REG0_PWD_CMP_OFFSET_SHIFT)) & DCDC_REG0_PWD_CMP_OFFSET_MASK)
#define DCDC_REG0_DCDC_XTALOK_DISABLE_MASK       (0x8000000U)
#define DCDC_REG0_DCDC_XTALOK_DISABLE_SHIFT      (27U)
#define DCDC_REG0_DCDC_XTALOK_DISABLE(x)         (((uint32_t)(((uint32_t)(x)) << DCDC_REG0_DCDC_XTALOK_DISABLE_SHIFT)) & DCDC_REG0_DCDC_XTALOK_DISABLE_MASK)
#define DCDC_REG0_PSWITCH_STATUS_MASK            (0x10000000U)
#define DCDC_REG0_PSWITCH_STATUS_SHIFT           (28U)
#define DCDC_REG0_PSWITCH_STATUS(x)              (((uint32_t)(((uint32_t)(x)) << DCDC_REG0_PSWITCH_STATUS_SHIFT)) & DCDC_REG0_PSWITCH_STATUS_MASK)
#define DCDC_REG0_VLPS_CONFIG_DCDC_HP_MASK       (0x20000000U)
#define DCDC_REG0_VLPS_CONFIG_DCDC_HP_SHIFT      (29U)
#define DCDC_REG0_VLPS_CONFIG_DCDC_HP(x)         (((uint32_t)(((uint32_t)(x)) << DCDC_REG0_VLPS_CONFIG_DCDC_HP_SHIFT)) & DCDC_REG0_VLPS_CONFIG_DCDC_HP_MASK)
#define DCDC_REG0_VLPR_VLPW_CONFIG_DCDC_HP_MASK  (0x40000000U)
#define DCDC_REG0_VLPR_VLPW_CONFIG_DCDC_HP_SHIFT (30U)
#define DCDC_REG0_VLPR_VLPW_CONFIG_DCDC_HP(x)    (((uint32_t)(((uint32_t)(x)) << DCDC_REG0_VLPR_VLPW_CONFIG_DCDC_HP_SHIFT)) & DCDC_REG0_VLPR_VLPW_CONFIG_DCDC_HP_MASK)
#define DCDC_REG0_DCDC_STS_DC_OK_MASK            (0x80000000U)
#define DCDC_REG0_DCDC_STS_DC_OK_SHIFT           (31U)
#define DCDC_REG0_DCDC_STS_DC_OK(x)              (((uint32_t)(((uint32_t)(x)) << DCDC_REG0_DCDC_STS_DC_OK_SHIFT)) & DCDC_REG0_DCDC_STS_DC_OK_MASK)

/*! @name REG1 - DCDC REGISTER 1 */
#define DCDC_REG1_POSLIMIT_BUCK_IN_MASK          (0x7FU)
#define DCDC_REG1_POSLIMIT_BUCK_IN_SHIFT         (0U)
#define DCDC_REG1_POSLIMIT_BUCK_IN(x)            (((uint32_t)(((uint32_t)(x)) << DCDC_REG1_POSLIMIT_BUCK_IN_SHIFT)) & DCDC_REG1_POSLIMIT_BUCK_IN_MASK)
#define DCDC_REG1_POSLIMIT_BOOST_IN_MASK         (0x3F80U)
#define DCDC_REG1_POSLIMIT_BOOST_IN_SHIFT        (7U)
#define DCDC_REG1_POSLIMIT_BOOST_IN(x)           (((uint32_t)(((uint32_t)(x)) << DCDC_REG1_POSLIMIT_BOOST_IN_SHIFT)) & DCDC_REG1_POSLIMIT_BOOST_IN_MASK)
#define DCDC_REG1_DCDC_LOOPCTRL_CM_HST_THRESH_MASK (0x200000U)
#define DCDC_REG1_DCDC_LOOPCTRL_CM_HST_THRESH_SHIFT (21U)
#define DCDC_REG1_DCDC_LOOPCTRL_CM_HST_THRESH(x) (((uint32_t)(((uint32_t)(x)) << DCDC_REG1_DCDC_LOOPCTRL_CM_HST_THRESH_SHIFT)) & DCDC_REG1_DCDC_LOOPCTRL_CM_HST_THRESH_MASK)
#define DCDC_REG1_DCDC_LOOPCTRL_DF_HST_THRESH_MASK (0x400000U)
#define DCDC_REG1_DCDC_LOOPCTRL_DF_HST_THRESH_SHIFT (22U)
#define DCDC_REG1_DCDC_LOOPCTRL_DF_HST_THRESH(x) (((uint32_t)(((uint32_t)(x)) << DCDC_REG1_DCDC_LOOPCTRL_DF_HST_THRESH_SHIFT)) & DCDC_REG1_DCDC_LOOPCTRL_DF_HST_THRESH_MASK)
#define DCDC_REG1_DCDC_LOOPCTRL_EN_CM_HYST_MASK  (0x800000U)
#define DCDC_REG1_DCDC_LOOPCTRL_EN_CM_HYST_SHIFT (23U)
#define DCDC_REG1_DCDC_LOOPCTRL_EN_CM_HYST(x)    (((uint32_t)(((uint32_t)(x)) << DCDC_REG1_DCDC_LOOPCTRL_EN_CM_HYST_SHIFT)) & DCDC_REG1_DCDC_LOOPCTRL_EN_CM_HYST_MASK)
#define DCDC_REG1_DCDC_LOOPCTRL_EN_DF_HYST_MASK  (0x1000000U)
#define DCDC_REG1_DCDC_LOOPCTRL_EN_DF_HYST_SHIFT (24U)
#define DCDC_REG1_DCDC_LOOPCTRL_EN_DF_HYST(x)    (((uint32_t)(((uint32_t)(x)) << DCDC_REG1_DCDC_LOOPCTRL_EN_DF_HYST_SHIFT)) & DCDC_REG1_DCDC_LOOPCTRL_EN_DF_HYST_MASK)

/*! @name REG2 - DCDC REGISTER 2 */
#define DCDC_REG2_DCDC_LOOPCTRL_HYST_SIGN_MASK   (0x2000U)
#define DCDC_REG2_DCDC_LOOPCTRL_HYST_SIGN_SHIFT  (13U)
#define DCDC_REG2_DCDC_LOOPCTRL_HYST_SIGN(x)     (((uint32_t)(((uint32_t)(x)) << DCDC_REG2_DCDC_LOOPCTRL_HYST_SIGN_SHIFT)) & DCDC_REG2_DCDC_LOOPCTRL_HYST_SIGN_MASK)
#define DCDC_REG2_DCDC_BATTMONITOR_EN_BATADJ_MASK (0x8000U)
#define DCDC_REG2_DCDC_BATTMONITOR_EN_BATADJ_SHIFT (15U)
#define DCDC_REG2_DCDC_BATTMONITOR_EN_BATADJ(x)  (((uint32_t)(((uint32_t)(x)) << DCDC_REG2_DCDC_BATTMONITOR_EN_BATADJ_SHIFT)) & DCDC_REG2_DCDC_BATTMONITOR_EN_BATADJ_MASK)
#define DCDC_REG2_DCDC_BATTMONITOR_BATT_VAL_MASK (0x3FF0000U)
#define DCDC_REG2_DCDC_BATTMONITOR_BATT_VAL_SHIFT (16U)
#define DCDC_REG2_DCDC_BATTMONITOR_BATT_VAL(x)   (((uint32_t)(((uint32_t)(x)) << DCDC_REG2_DCDC_BATTMONITOR_BATT_VAL_SHIFT)) & DCDC_REG2_DCDC_BATTMONITOR_BATT_VAL_MASK)

/*! @name REG3 - DCDC REGISTER 3 */
#define DCDC_REG3_DCDC_VDD1P8CTRL_TRG_MASK       (0x3FU)
#define DCDC_REG3_DCDC_VDD1P8CTRL_TRG_SHIFT      (0U)
#define DCDC_REG3_DCDC_VDD1P8CTRL_TRG(x)         (((uint32_t)(((uint32_t)(x)) << DCDC_REG3_DCDC_VDD1P8CTRL_TRG_SHIFT)) & DCDC_REG3_DCDC_VDD1P8CTRL_TRG_MASK)
#define DCDC_REG3_DCDC_VDD1P5CTRL_TRG_BUCK_MASK  (0x7C0U)
#define DCDC_REG3_DCDC_VDD1P5CTRL_TRG_BUCK_SHIFT (6U)
#define DCDC_REG3_DCDC_VDD1P5CTRL_TRG_BUCK(x)    (((uint32_t)(((uint32_t)(x)) << DCDC_REG3_DCDC_VDD1P5CTRL_TRG_BUCK_SHIFT)) & DCDC_REG3_DCDC_VDD1P5CTRL_TRG_BUCK_MASK)
#define DCDC_REG3_DCDC_VDD1P5CTRL_TRG_BOOST_MASK (0xF800U)
#define DCDC_REG3_DCDC_VDD1P5CTRL_TRG_BOOST_SHIFT (11U)
#define DCDC_REG3_DCDC_VDD1P5CTRL_TRG_BOOST(x)   (((uint32_t)(((uint32_t)(x)) << DCDC_REG3_DCDC_VDD1P5CTRL_TRG_BOOST_SHIFT)) & DCDC_REG3_DCDC_VDD1P5CTRL_TRG_BOOST_MASK)
#define DCDC_REG3_DCDC_VDD1P5CTRL_ADJTN_MASK     (0x1E0000U)
#define DCDC_REG3_DCDC_VDD1P5CTRL_ADJTN_SHIFT    (17U)
#define DCDC_REG3_DCDC_VDD1P5CTRL_ADJTN(x)       (((uint32_t)(((uint32_t)(x)) << DCDC_REG3_DCDC_VDD1P5CTRL_ADJTN_SHIFT)) & DCDC_REG3_DCDC_VDD1P5CTRL_ADJTN_MASK)
#define DCDC_REG3_DCDC_MINPWR_DC_HALFCLK_PULSED_MASK (0x200000U)
#define DCDC_REG3_DCDC_MINPWR_DC_HALFCLK_PULSED_SHIFT (21U)
#define DCDC_REG3_DCDC_MINPWR_DC_HALFCLK_PULSED(x) (((uint32_t)(((uint32_t)(x)) << DCDC_REG3_DCDC_MINPWR_DC_HALFCLK_PULSED_SHIFT)) & DCDC_REG3_DCDC_MINPWR_DC_HALFCLK_PULSED_MASK)
#define DCDC_REG3_DCDC_MINPWR_DOUBLE_FETS_PULSED_MASK (0x400000U)
#define DCDC_REG3_DCDC_MINPWR_DOUBLE_FETS_PULSED_SHIFT (22U)
#define DCDC_REG3_DCDC_MINPWR_DOUBLE_FETS_PULSED(x) (((uint32_t)(((uint32_t)(x)) << DCDC_REG3_DCDC_MINPWR_DOUBLE_FETS_PULSED_SHIFT)) & DCDC_REG3_DCDC_MINPWR_DOUBLE_FETS_PULSED_MASK)
#define DCDC_REG3_DCDC_MINPWR_HALF_FETS_PULSED_MASK (0x800000U)
#define DCDC_REG3_DCDC_MINPWR_HALF_FETS_PULSED_SHIFT (23U)
#define DCDC_REG3_DCDC_MINPWR_HALF_FETS_PULSED(x) (((uint32_t)(((uint32_t)(x)) << DCDC_REG3_DCDC_MINPWR_HALF_FETS_PULSED_SHIFT)) & DCDC_REG3_DCDC_MINPWR_HALF_FETS_PULSED_MASK)
#define DCDC_REG3_DCDC_MINPWR_DC_HALFCLK_MASK    (0x1000000U)
#define DCDC_REG3_DCDC_MINPWR_DC_HALFCLK_SHIFT   (24U)
#define DCDC_REG3_DCDC_MINPWR_DC_HALFCLK(x)      (((uint32_t)(((uint32_t)(x)) << DCDC_REG3_DCDC_MINPWR_DC_HALFCLK_SHIFT)) & DCDC_REG3_DCDC_MINPWR_DC_HALFCLK_MASK)
#define DCDC_REG3_DCDC_MINPWR_DOUBLE_FETS_MASK   (0x2000000U)
#define DCDC_REG3_DCDC_MINPWR_DOUBLE_FETS_SHIFT  (25U)
#define DCDC_REG3_DCDC_MINPWR_DOUBLE_FETS(x)     (((uint32_t)(((uint32_t)(x)) << DCDC_REG3_DCDC_MINPWR_DOUBLE_FETS_SHIFT)) & DCDC_REG3_DCDC_MINPWR_DOUBLE_FETS_MASK)
#define DCDC_REG3_DCDC_MINPWR_HALF_FETS_MASK     (0x4000000U)
#define DCDC_REG3_DCDC_MINPWR_HALF_FETS_SHIFT    (26U)
#define DCDC_REG3_DCDC_MINPWR_HALF_FETS(x)       (((uint32_t)(((uint32_t)(x)) << DCDC_REG3_DCDC_MINPWR_HALF_FETS_SHIFT)) & DCDC_REG3_DCDC_MINPWR_HALF_FETS_MASK)
#define DCDC_REG3_DCDC_VDD1P5CTRL_DISABLE_STEP_MASK (0x20000000U)
#define DCDC_REG3_DCDC_VDD1P5CTRL_DISABLE_STEP_SHIFT (29U)
#define DCDC_REG3_DCDC_VDD1P5CTRL_DISABLE_STEP(x) (((uint32_t)(((uint32_t)(x)) << DCDC_REG3_DCDC_VDD1P5CTRL_DISABLE_STEP_SHIFT)) & DCDC_REG3_DCDC_VDD1P5CTRL_DISABLE_STEP_MASK)
#define DCDC_REG3_DCDC_VDD1P8CTRL_DISABLE_STEP_MASK (0x40000000U)
#define DCDC_REG3_DCDC_VDD1P8CTRL_DISABLE_STEP_SHIFT (30U)
#define DCDC_REG3_DCDC_VDD1P8CTRL_DISABLE_STEP(x) (((uint32_t)(((uint32_t)(x)) << DCDC_REG3_DCDC_VDD1P8CTRL_DISABLE_STEP_SHIFT)) & DCDC_REG3_DCDC_VDD1P8CTRL_DISABLE_STEP_MASK)

/*! @name REG4 - DCDC REGISTER 4 */
#define DCDC_REG4_DCDC_SW_SHUTDOWN_MASK          (0x1U)
#define DCDC_REG4_DCDC_SW_SHUTDOWN_SHIFT         (0U)
#define DCDC_REG4_DCDC_SW_SHUTDOWN(x)            (((uint32_t)(((uint32_t)(x)) << DCDC_REG4_DCDC_SW_SHUTDOWN_SHIFT)) & DCDC_REG4_DCDC_SW_SHUTDOWN_MASK)
#define DCDC_REG4_UNLOCK_MASK                    (0xFFFF0000U)
#define DCDC_REG4_UNLOCK_SHIFT                   (16U)
#define DCDC_REG4_UNLOCK(x)                      (((uint32_t)(((uint32_t)(x)) << DCDC_REG4_UNLOCK_SHIFT)) & DCDC_REG4_UNLOCK_MASK)

/*! @name REG6 - DCDC REGISTER 6 */
#define DCDC_REG6_PSWITCH_INT_RISE_EN_MASK       (0x1U)
#define DCDC_REG6_PSWITCH_INT_RISE_EN_SHIFT      (0U)
#define DCDC_REG6_PSWITCH_INT_RISE_EN(x)         (((uint32_t)(((uint32_t)(x)) << DCDC_REG6_PSWITCH_INT_RISE_EN_SHIFT)) & DCDC_REG6_PSWITCH_INT_RISE_EN_MASK)
#define DCDC_REG6_PSWITCH_INT_FALL_EN_MASK       (0x2U)
#define DCDC_REG6_PSWITCH_INT_FALL_EN_SHIFT      (1U)
#define DCDC_REG6_PSWITCH_INT_FALL_EN(x)         (((uint32_t)(((uint32_t)(x)) << DCDC_REG6_PSWITCH_INT_FALL_EN_SHIFT)) & DCDC_REG6_PSWITCH_INT_FALL_EN_MASK)
#define DCDC_REG6_PSWITCH_INT_CLEAR_MASK         (0x4U)
#define DCDC_REG6_PSWITCH_INT_CLEAR_SHIFT        (2U)
#define DCDC_REG6_PSWITCH_INT_CLEAR(x)           (((uint32_t)(((uint32_t)(x)) << DCDC_REG6_PSWITCH_INT_CLEAR_SHIFT)) & DCDC_REG6_PSWITCH_INT_CLEAR_MASK)
#define DCDC_REG6_PSWITCH_INT_MUTE_MASK          (0x8U)
#define DCDC_REG6_PSWITCH_INT_MUTE_SHIFT         (3U)
#define DCDC_REG6_PSWITCH_INT_MUTE(x)            (((uint32_t)(((uint32_t)(x)) << DCDC_REG6_PSWITCH_INT_MUTE_SHIFT)) & DCDC_REG6_PSWITCH_INT_MUTE_MASK)
#define DCDC_REG6_PSWITCH_INT_STS_MASK           (0x80000000U)
#define DCDC_REG6_PSWITCH_INT_STS_SHIFT          (31U)
#define DCDC_REG6_PSWITCH_INT_STS(x)             (((uint32_t)(((uint32_t)(x)) << DCDC_REG6_PSWITCH_INT_STS_SHIFT)) & DCDC_REG6_PSWITCH_INT_STS_MASK)

/*! @name REG7 - DCDC REGISTER 7 */
#define DCDC_REG7_INTEGRATOR_VALUE_MASK          (0x7FFFFU)
#define DCDC_REG7_INTEGRATOR_VALUE_SHIFT         (0U)
#define DCDC_REG7_INTEGRATOR_VALUE(x)            (((uint32_t)(((uint32_t)(x)) << DCDC_REG7_INTEGRATOR_VALUE_SHIFT)) & DCDC_REG7_INTEGRATOR_VALUE_MASK)
#define DCDC_REG7_INTEGRATOR_VALUE_SEL_MASK      (0x80000U)
#define DCDC_REG7_INTEGRATOR_VALUE_SEL_SHIFT     (19U)
#define DCDC_REG7_INTEGRATOR_VALUE_SEL(x)        (((uint32_t)(((uint32_t)(x)) << DCDC_REG7_INTEGRATOR_VALUE_SEL_SHIFT)) & DCDC_REG7_INTEGRATOR_VALUE_SEL_MASK)
#define DCDC_REG7_PULSE_RUN_SPEEDUP_MASK         (0x100000U)
#define DCDC_REG7_PULSE_RUN_SPEEDUP_SHIFT        (20U)
#define DCDC_REG7_PULSE_RUN_SPEEDUP(x)           (((uint32_t)(((uint32_t)(x)) << DCDC_REG7_PULSE_RUN_SPEEDUP_SHIFT)) & DCDC_REG7_PULSE_RUN_SPEEDUP_MASK)


/*!
 * @}
 */ /* end of group DCDC_Register_Masks */


/* DCDC - Peripheral instance base addresses */
/** Peripheral DCDC base address */
#define DCDC_BASE                                (0x4005A000u)
/** Peripheral DCDC base pointer */
#define DCDC                                     ((DCDC_Type *)DCDC_BASE)
/** Array initializer of DCDC peripheral base addresses */
#define DCDC_BASE_ADDRS                          { DCDC_BASE }
/** Array initializer of DCDC peripheral base pointers */
#define DCDC_BASE_PTRS                           { DCDC }

/*!
 * @}
 */ /* end of group DCDC_Peripheral_Access_Layer */


/* ----------------------------------------------------------------------------
   -- DMA Peripheral Access Layer
   ---------------------------------------------------------------------------- */

/*!
 * @addtogroup DMA_Peripheral_Access_Layer DMA Peripheral Access Layer
 * @{
 */

/** DMA - Register Layout Typedef */
typedef struct {
  __IO uint32_t CR;                                /**< Control Register, offset: 0x0 */
  __I  uint32_t ES;                                /**< Error Status Register, offset: 0x4 */
       uint8_t RESERVED_0[4];
  __IO uint32_t ERQ;                               /**< Enable Request Register, offset: 0xC */
       uint8_t RESERVED_1[4];
  __IO uint32_t EEI;                               /**< Enable Error Interrupt Register, offset: 0x14 */
  __O  uint8_t CEEI;                               /**< Clear Enable Error Interrupt Register, offset: 0x18 */
  __O  uint8_t SEEI;                               /**< Set Enable Error Interrupt Register, offset: 0x19 */
  __O  uint8_t CERQ;                               /**< Clear Enable Request Register, offset: 0x1A */
  __O  uint8_t SERQ;                               /**< Set Enable Request Register, offset: 0x1B */
  __O  uint8_t CDNE;                               /**< Clear DONE Status Bit Register, offset: 0x1C */
  __O  uint8_t SSRT;                               /**< Set START Bit Register, offset: 0x1D */
  __O  uint8_t CERR;                               /**< Clear Error Register, offset: 0x1E */
  __O  uint8_t CINT;                               /**< Clear Interrupt Request Register, offset: 0x1F */
       uint8_t RESERVED_2[4];
  __IO uint32_t INT;                               /**< Interrupt Request Register, offset: 0x24 */
       uint8_t RESERVED_3[4];
  __IO uint32_t ERR;                               /**< Error Register, offset: 0x2C */
       uint8_t RESERVED_4[4];
  __I  uint32_t HRS;                               /**< Hardware Request Status Register, offset: 0x34 */
       uint8_t RESERVED_5[12];
  __IO uint32_t EARS;                              /**< Enable Asynchronous Request in Stop Register, offset: 0x44 */
       uint8_t RESERVED_6[184];
  __IO uint8_t DCHPRI3;                            /**< Channel n Priority Register, offset: 0x100 */
  __IO uint8_t DCHPRI2;                            /**< Channel n Priority Register, offset: 0x101 */
  __IO uint8_t DCHPRI1;                            /**< Channel n Priority Register, offset: 0x102 */
  __IO uint8_t DCHPRI0;                            /**< Channel n Priority Register, offset: 0x103 */
       uint8_t RESERVED_7[3836];
  struct {                                         /* offset: 0x1000, array step: 0x20 */
    __IO uint32_t SADDR;                             /**< TCD Source Address, array offset: 0x1000, array step: 0x20 */
    __IO uint16_t SOFF;                              /**< TCD Signed Source Address Offset, array offset: 0x1004, array step: 0x20 */
    __IO uint16_t ATTR;                              /**< TCD Transfer Attributes, array offset: 0x1006, array step: 0x20 */
    union {                                          /* offset: 0x1008, array step: 0x20 */
      __IO uint32_t NBYTES_MLNO;                       /**< TCD Minor Byte Count (Minor Loop Mapping Disabled), array offset: 0x1008, array step: 0x20 */
      __IO uint32_t NBYTES_MLOFFNO;                    /**< TCD Signed Minor Loop Offset (Minor Loop Mapping Enabled and Offset Disabled), array offset: 0x1008, array step: 0x20 */
      __IO uint32_t NBYTES_MLOFFYES;                   /**< TCD Signed Minor Loop Offset (Minor Loop Mapping and Offset Enabled), array offset: 0x1008, array step: 0x20 */
    };
    __IO uint32_t SLAST;                             /**< TCD Last Source Address Adjustment, array offset: 0x100C, array step: 0x20 */
    __IO uint32_t DADDR;                             /**< TCD Destination Address, array offset: 0x1010, array step: 0x20 */
    __IO uint16_t DOFF;                              /**< TCD Signed Destination Address Offset, array offset: 0x1014, array step: 0x20 */
    union {                                          /* offset: 0x1016, array step: 0x20 */
      __IO uint16_t CITER_ELINKNO;                     /**< TCD Current Minor Loop Link, Major Loop Count (Channel Linking Disabled), array offset: 0x1016, array step: 0x20 */
      __IO uint16_t CITER_ELINKYES;                    /**< TCD Current Minor Loop Link, Major Loop Count (Channel Linking Enabled), array offset: 0x1016, array step: 0x20 */
    };
    __IO uint32_t DLAST_SGA;                         /**< TCD Last Destination Address Adjustment/Scatter Gather Address, array offset: 0x1018, array step: 0x20 */
    __IO uint16_t CSR;                               /**< TCD Control and Status, array offset: 0x101C, array step: 0x20 */
    union {                                          /* offset: 0x101E, array step: 0x20 */
      __IO uint16_t BITER_ELINKNO;                     /**< TCD Beginning Minor Loop Link, Major Loop Count (Channel Linking Disabled), array offset: 0x101E, array step: 0x20 */
      __IO uint16_t BITER_ELINKYES;                    /**< TCD Beginning Minor Loop Link, Major Loop Count (Channel Linking Enabled), array offset: 0x101E, array step: 0x20 */
    };
  } TCD[4];
} DMA_Type;

/* ----------------------------------------------------------------------------
   -- DMA Register Masks
   ---------------------------------------------------------------------------- */

/*!
 * @addtogroup DMA_Register_Masks DMA Register Masks
 * @{
 */

/*! @name CR - Control Register */
#define DMA_CR_EDBG_MASK                         (0x2U)
#define DMA_CR_EDBG_SHIFT                        (1U)
#define DMA_CR_EDBG(x)                           (((uint32_t)(((uint32_t)(x)) << DMA_CR_EDBG_SHIFT)) & DMA_CR_EDBG_MASK)
#define DMA_CR_ERCA_MASK                         (0x4U)
#define DMA_CR_ERCA_SHIFT                        (2U)
#define DMA_CR_ERCA(x)                           (((uint32_t)(((uint32_t)(x)) << DMA_CR_ERCA_SHIFT)) & DMA_CR_ERCA_MASK)
#define DMA_CR_HOE_MASK                          (0x10U)
#define DMA_CR_HOE_SHIFT                         (4U)
#define DMA_CR_HOE(x)                            (((uint32_t)(((uint32_t)(x)) << DMA_CR_HOE_SHIFT)) & DMA_CR_HOE_MASK)
#define DMA_CR_HALT_MASK                         (0x20U)
#define DMA_CR_HALT_SHIFT                        (5U)
#define DMA_CR_HALT(x)                           (((uint32_t)(((uint32_t)(x)) << DMA_CR_HALT_SHIFT)) & DMA_CR_HALT_MASK)
#define DMA_CR_CLM_MASK                          (0x40U)
#define DMA_CR_CLM_SHIFT                         (6U)
#define DMA_CR_CLM(x)                            (((uint32_t)(((uint32_t)(x)) << DMA_CR_CLM_SHIFT)) & DMA_CR_CLM_MASK)
#define DMA_CR_EMLM_MASK                         (0x80U)
#define DMA_CR_EMLM_SHIFT                        (7U)
#define DMA_CR_EMLM(x)                           (((uint32_t)(((uint32_t)(x)) << DMA_CR_EMLM_SHIFT)) & DMA_CR_EMLM_MASK)
#define DMA_CR_ECX_MASK                          (0x10000U)
#define DMA_CR_ECX_SHIFT                         (16U)
#define DMA_CR_ECX(x)                            (((uint32_t)(((uint32_t)(x)) << DMA_CR_ECX_SHIFT)) & DMA_CR_ECX_MASK)
#define DMA_CR_CX_MASK                           (0x20000U)
#define DMA_CR_CX_SHIFT                          (17U)
#define DMA_CR_CX(x)                             (((uint32_t)(((uint32_t)(x)) << DMA_CR_CX_SHIFT)) & DMA_CR_CX_MASK)
#define DMA_CR_ACTIVE_MASK                       (0x80000000U)
#define DMA_CR_ACTIVE_SHIFT                      (31U)
#define DMA_CR_ACTIVE(x)                         (((uint32_t)(((uint32_t)(x)) << DMA_CR_ACTIVE_SHIFT)) & DMA_CR_ACTIVE_MASK)

/*! @name ES - Error Status Register */
#define DMA_ES_DBE_MASK                          (0x1U)
#define DMA_ES_DBE_SHIFT                         (0U)
#define DMA_ES_DBE(x)                            (((uint32_t)(((uint32_t)(x)) << DMA_ES_DBE_SHIFT)) & DMA_ES_DBE_MASK)
#define DMA_ES_SBE_MASK                          (0x2U)
#define DMA_ES_SBE_SHIFT                         (1U)
#define DMA_ES_SBE(x)                            (((uint32_t)(((uint32_t)(x)) << DMA_ES_SBE_SHIFT)) & DMA_ES_SBE_MASK)
#define DMA_ES_SGE_MASK                          (0x4U)
#define DMA_ES_SGE_SHIFT                         (2U)
#define DMA_ES_SGE(x)                            (((uint32_t)(((uint32_t)(x)) << DMA_ES_SGE_SHIFT)) & DMA_ES_SGE_MASK)
#define DMA_ES_NCE_MASK                          (0x8U)
#define DMA_ES_NCE_SHIFT                         (3U)
#define DMA_ES_NCE(x)                            (((uint32_t)(((uint32_t)(x)) << DMA_ES_NCE_SHIFT)) & DMA_ES_NCE_MASK)
#define DMA_ES_DOE_MASK                          (0x10U)
#define DMA_ES_DOE_SHIFT                         (4U)
#define DMA_ES_DOE(x)                            (((uint32_t)(((uint32_t)(x)) << DMA_ES_DOE_SHIFT)) & DMA_ES_DOE_MASK)
#define DMA_ES_DAE_MASK                          (0x20U)
#define DMA_ES_DAE_SHIFT                         (5U)
#define DMA_ES_DAE(x)                            (((uint32_t)(((uint32_t)(x)) << DMA_ES_DAE_SHIFT)) & DMA_ES_DAE_MASK)
#define DMA_ES_SOE_MASK                          (0x40U)
#define DMA_ES_SOE_SHIFT                         (6U)
#define DMA_ES_SOE(x)                            (((uint32_t)(((uint32_t)(x)) << DMA_ES_SOE_SHIFT)) & DMA_ES_SOE_MASK)
#define DMA_ES_SAE_MASK                          (0x80U)
#define DMA_ES_SAE_SHIFT                         (7U)
#define DMA_ES_SAE(x)                            (((uint32_t)(((uint32_t)(x)) << DMA_ES_SAE_SHIFT)) & DMA_ES_SAE_MASK)
#define DMA_ES_ERRCHN_MASK                       (0x300U)
#define DMA_ES_ERRCHN_SHIFT                      (8U)
#define DMA_ES_ERRCHN(x)                         (((uint32_t)(((uint32_t)(x)) << DMA_ES_ERRCHN_SHIFT)) & DMA_ES_ERRCHN_MASK)
#define DMA_ES_CPE_MASK                          (0x4000U)
#define DMA_ES_CPE_SHIFT                         (14U)
#define DMA_ES_CPE(x)                            (((uint32_t)(((uint32_t)(x)) << DMA_ES_CPE_SHIFT)) & DMA_ES_CPE_MASK)
#define DMA_ES_ECX_MASK                          (0x10000U)
#define DMA_ES_ECX_SHIFT                         (16U)
#define DMA_ES_ECX(x)                            (((uint32_t)(((uint32_t)(x)) << DMA_ES_ECX_SHIFT)) & DMA_ES_ECX_MASK)
#define DMA_ES_VLD_MASK                          (0x80000000U)
#define DMA_ES_VLD_SHIFT                         (31U)
#define DMA_ES_VLD(x)                            (((uint32_t)(((uint32_t)(x)) << DMA_ES_VLD_SHIFT)) & DMA_ES_VLD_MASK)

/*! @name ERQ - Enable Request Register */
#define DMA_ERQ_ERQ0_MASK                        (0x1U)
#define DMA_ERQ_ERQ0_SHIFT                       (0U)
#define DMA_ERQ_ERQ0(x)                          (((uint32_t)(((uint32_t)(x)) << DMA_ERQ_ERQ0_SHIFT)) & DMA_ERQ_ERQ0_MASK)
#define DMA_ERQ_ERQ1_MASK                        (0x2U)
#define DMA_ERQ_ERQ1_SHIFT                       (1U)
#define DMA_ERQ_ERQ1(x)                          (((uint32_t)(((uint32_t)(x)) << DMA_ERQ_ERQ1_SHIFT)) & DMA_ERQ_ERQ1_MASK)
#define DMA_ERQ_ERQ2_MASK                        (0x4U)
#define DMA_ERQ_ERQ2_SHIFT                       (2U)
#define DMA_ERQ_ERQ2(x)                          (((uint32_t)(((uint32_t)(x)) << DMA_ERQ_ERQ2_SHIFT)) & DMA_ERQ_ERQ2_MASK)
#define DMA_ERQ_ERQ3_MASK                        (0x8U)
#define DMA_ERQ_ERQ3_SHIFT                       (3U)
#define DMA_ERQ_ERQ3(x)                          (((uint32_t)(((uint32_t)(x)) << DMA_ERQ_ERQ3_SHIFT)) & DMA_ERQ_ERQ3_MASK)

/*! @name EEI - Enable Error Interrupt Register */
#define DMA_EEI_EEI0_MASK                        (0x1U)
#define DMA_EEI_EEI0_SHIFT                       (0U)
#define DMA_EEI_EEI0(x)                          (((uint32_t)(((uint32_t)(x)) << DMA_EEI_EEI0_SHIFT)) & DMA_EEI_EEI0_MASK)
#define DMA_EEI_EEI1_MASK                        (0x2U)
#define DMA_EEI_EEI1_SHIFT                       (1U)
#define DMA_EEI_EEI1(x)                          (((uint32_t)(((uint32_t)(x)) << DMA_EEI_EEI1_SHIFT)) & DMA_EEI_EEI1_MASK)
#define DMA_EEI_EEI2_MASK                        (0x4U)
#define DMA_EEI_EEI2_SHIFT                       (2U)
#define DMA_EEI_EEI2(x)                          (((uint32_t)(((uint32_t)(x)) << DMA_EEI_EEI2_SHIFT)) & DMA_EEI_EEI2_MASK)
#define DMA_EEI_EEI3_MASK                        (0x8U)
#define DMA_EEI_EEI3_SHIFT                       (3U)
#define DMA_EEI_EEI3(x)                          (((uint32_t)(((uint32_t)(x)) << DMA_EEI_EEI3_SHIFT)) & DMA_EEI_EEI3_MASK)

/*! @name CEEI - Clear Enable Error Interrupt Register */
#define DMA_CEEI_CEEI_MASK                       (0x3U)
#define DMA_CEEI_CEEI_SHIFT                      (0U)
#define DMA_CEEI_CEEI(x)                         (((uint8_t)(((uint8_t)(x)) << DMA_CEEI_CEEI_SHIFT)) & DMA_CEEI_CEEI_MASK)
#define DMA_CEEI_CAEE_MASK                       (0x40U)
#define DMA_CEEI_CAEE_SHIFT                      (6U)
#define DMA_CEEI_CAEE(x)                         (((uint8_t)(((uint8_t)(x)) << DMA_CEEI_CAEE_SHIFT)) & DMA_CEEI_CAEE_MASK)
#define DMA_CEEI_NOP_MASK                        (0x80U)
#define DMA_CEEI_NOP_SHIFT                       (7U)
#define DMA_CEEI_NOP(x)                          (((uint8_t)(((uint8_t)(x)) << DMA_CEEI_NOP_SHIFT)) & DMA_CEEI_NOP_MASK)

/*! @name SEEI - Set Enable Error Interrupt Register */
#define DMA_SEEI_SEEI_MASK                       (0x3U)
#define DMA_SEEI_SEEI_SHIFT                      (0U)
#define DMA_SEEI_SEEI(x)                         (((uint8_t)(((uint8_t)(x)) << DMA_SEEI_SEEI_SHIFT)) & DMA_SEEI_SEEI_MASK)
#define DMA_SEEI_SAEE_MASK                       (0x40U)
#define DMA_SEEI_SAEE_SHIFT                      (6U)
#define DMA_SEEI_SAEE(x)                         (((uint8_t)(((uint8_t)(x)) << DMA_SEEI_SAEE_SHIFT)) & DMA_SEEI_SAEE_MASK)
#define DMA_SEEI_NOP_MASK                        (0x80U)
#define DMA_SEEI_NOP_SHIFT                       (7U)
#define DMA_SEEI_NOP(x)                          (((uint8_t)(((uint8_t)(x)) << DMA_SEEI_NOP_SHIFT)) & DMA_SEEI_NOP_MASK)

/*! @name CERQ - Clear Enable Request Register */
#define DMA_CERQ_CERQ_MASK                       (0x3U)
#define DMA_CERQ_CERQ_SHIFT                      (0U)
#define DMA_CERQ_CERQ(x)                         (((uint8_t)(((uint8_t)(x)) << DMA_CERQ_CERQ_SHIFT)) & DMA_CERQ_CERQ_MASK)
#define DMA_CERQ_CAER_MASK                       (0x40U)
#define DMA_CERQ_CAER_SHIFT                      (6U)
#define DMA_CERQ_CAER(x)                         (((uint8_t)(((uint8_t)(x)) << DMA_CERQ_CAER_SHIFT)) & DMA_CERQ_CAER_MASK)
#define DMA_CERQ_NOP_MASK                        (0x80U)
#define DMA_CERQ_NOP_SHIFT                       (7U)
#define DMA_CERQ_NOP(x)                          (((uint8_t)(((uint8_t)(x)) << DMA_CERQ_NOP_SHIFT)) & DMA_CERQ_NOP_MASK)

/*! @name SERQ - Set Enable Request Register */
#define DMA_SERQ_SERQ_MASK                       (0x3U)
#define DMA_SERQ_SERQ_SHIFT                      (0U)
#define DMA_SERQ_SERQ(x)                         (((uint8_t)(((uint8_t)(x)) << DMA_SERQ_SERQ_SHIFT)) & DMA_SERQ_SERQ_MASK)
#define DMA_SERQ_SAER_MASK                       (0x40U)
#define DMA_SERQ_SAER_SHIFT                      (6U)
#define DMA_SERQ_SAER(x)                         (((uint8_t)(((uint8_t)(x)) << DMA_SERQ_SAER_SHIFT)) & DMA_SERQ_SAER_MASK)
#define DMA_SERQ_NOP_MASK                        (0x80U)
#define DMA_SERQ_NOP_SHIFT                       (7U)
#define DMA_SERQ_NOP(x)                          (((uint8_t)(((uint8_t)(x)) << DMA_SERQ_NOP_SHIFT)) & DMA_SERQ_NOP_MASK)

/*! @name CDNE - Clear DONE Status Bit Register */
#define DMA_CDNE_CDNE_MASK                       (0x3U)
#define DMA_CDNE_CDNE_SHIFT                      (0U)
#define DMA_CDNE_CDNE(x)                         (((uint8_t)(((uint8_t)(x)) << DMA_CDNE_CDNE_SHIFT)) & DMA_CDNE_CDNE_MASK)
#define DMA_CDNE_CADN_MASK                       (0x40U)
#define DMA_CDNE_CADN_SHIFT                      (6U)
#define DMA_CDNE_CADN(x)                         (((uint8_t)(((uint8_t)(x)) << DMA_CDNE_CADN_SHIFT)) & DMA_CDNE_CADN_MASK)
#define DMA_CDNE_NOP_MASK                        (0x80U)
#define DMA_CDNE_NOP_SHIFT                       (7U)
#define DMA_CDNE_NOP(x)                          (((uint8_t)(((uint8_t)(x)) << DMA_CDNE_NOP_SHIFT)) & DMA_CDNE_NOP_MASK)

/*! @name SSRT - Set START Bit Register */
#define DMA_SSRT_SSRT_MASK                       (0x3U)
#define DMA_SSRT_SSRT_SHIFT                      (0U)
#define DMA_SSRT_SSRT(x)                         (((uint8_t)(((uint8_t)(x)) << DMA_SSRT_SSRT_SHIFT)) & DMA_SSRT_SSRT_MASK)
#define DMA_SSRT_SAST_MASK                       (0x40U)
#define DMA_SSRT_SAST_SHIFT                      (6U)
#define DMA_SSRT_SAST(x)                         (((uint8_t)(((uint8_t)(x)) << DMA_SSRT_SAST_SHIFT)) & DMA_SSRT_SAST_MASK)
#define DMA_SSRT_NOP_MASK                        (0x80U)
#define DMA_SSRT_NOP_SHIFT                       (7U)
#define DMA_SSRT_NOP(x)                          (((uint8_t)(((uint8_t)(x)) << DMA_SSRT_NOP_SHIFT)) & DMA_SSRT_NOP_MASK)

/*! @name CERR - Clear Error Register */
#define DMA_CERR_CERR_MASK                       (0x3U)
#define DMA_CERR_CERR_SHIFT                      (0U)
#define DMA_CERR_CERR(x)                         (((uint8_t)(((uint8_t)(x)) << DMA_CERR_CERR_SHIFT)) & DMA_CERR_CERR_MASK)
#define DMA_CERR_CAEI_MASK                       (0x40U)
#define DMA_CERR_CAEI_SHIFT                      (6U)
#define DMA_CERR_CAEI(x)                         (((uint8_t)(((uint8_t)(x)) << DMA_CERR_CAEI_SHIFT)) & DMA_CERR_CAEI_MASK)
#define DMA_CERR_NOP_MASK                        (0x80U)
#define DMA_CERR_NOP_SHIFT                       (7U)
#define DMA_CERR_NOP(x)                          (((uint8_t)(((uint8_t)(x)) << DMA_CERR_NOP_SHIFT)) & DMA_CERR_NOP_MASK)

/*! @name CINT - Clear Interrupt Request Register */
#define DMA_CINT_CINT_MASK                       (0x3U)
#define DMA_CINT_CINT_SHIFT                      (0U)
#define DMA_CINT_CINT(x)                         (((uint8_t)(((uint8_t)(x)) << DMA_CINT_CINT_SHIFT)) & DMA_CINT_CINT_MASK)
#define DMA_CINT_CAIR_MASK                       (0x40U)
#define DMA_CINT_CAIR_SHIFT                      (6U)
#define DMA_CINT_CAIR(x)                         (((uint8_t)(((uint8_t)(x)) << DMA_CINT_CAIR_SHIFT)) & DMA_CINT_CAIR_MASK)
#define DMA_CINT_NOP_MASK                        (0x80U)
#define DMA_CINT_NOP_SHIFT                       (7U)
#define DMA_CINT_NOP(x)                          (((uint8_t)(((uint8_t)(x)) << DMA_CINT_NOP_SHIFT)) & DMA_CINT_NOP_MASK)

/*! @name INT - Interrupt Request Register */
#define DMA_INT_INT0_MASK                        (0x1U)
#define DMA_INT_INT0_SHIFT                       (0U)
#define DMA_INT_INT0(x)                          (((uint32_t)(((uint32_t)(x)) << DMA_INT_INT0_SHIFT)) & DMA_INT_INT0_MASK)
#define DMA_INT_INT1_MASK                        (0x2U)
#define DMA_INT_INT1_SHIFT                       (1U)
#define DMA_INT_INT1(x)                          (((uint32_t)(((uint32_t)(x)) << DMA_INT_INT1_SHIFT)) & DMA_INT_INT1_MASK)
#define DMA_INT_INT2_MASK                        (0x4U)
#define DMA_INT_INT2_SHIFT                       (2U)
#define DMA_INT_INT2(x)                          (((uint32_t)(((uint32_t)(x)) << DMA_INT_INT2_SHIFT)) & DMA_INT_INT2_MASK)
#define DMA_INT_INT3_MASK                        (0x8U)
#define DMA_INT_INT3_SHIFT                       (3U)
#define DMA_INT_INT3(x)                          (((uint32_t)(((uint32_t)(x)) << DMA_INT_INT3_SHIFT)) & DMA_INT_INT3_MASK)

/*! @name ERR - Error Register */
#define DMA_ERR_ERR0_MASK                        (0x1U)
#define DMA_ERR_ERR0_SHIFT                       (0U)
#define DMA_ERR_ERR0(x)                          (((uint32_t)(((uint32_t)(x)) << DMA_ERR_ERR0_SHIFT)) & DMA_ERR_ERR0_MASK)
#define DMA_ERR_ERR1_MASK                        (0x2U)
#define DMA_ERR_ERR1_SHIFT                       (1U)
#define DMA_ERR_ERR1(x)                          (((uint32_t)(((uint32_t)(x)) << DMA_ERR_ERR1_SHIFT)) & DMA_ERR_ERR1_MASK)
#define DMA_ERR_ERR2_MASK                        (0x4U)
#define DMA_ERR_ERR2_SHIFT                       (2U)
#define DMA_ERR_ERR2(x)                          (((uint32_t)(((uint32_t)(x)) << DMA_ERR_ERR2_SHIFT)) & DMA_ERR_ERR2_MASK)
#define DMA_ERR_ERR3_MASK                        (0x8U)
#define DMA_ERR_ERR3_SHIFT                       (3U)
#define DMA_ERR_ERR3(x)                          (((uint32_t)(((uint32_t)(x)) << DMA_ERR_ERR3_SHIFT)) & DMA_ERR_ERR3_MASK)

/*! @name HRS - Hardware Request Status Register */
#define DMA_HRS_HRS0_MASK                        (0x1U)
#define DMA_HRS_HRS0_SHIFT                       (0U)
#define DMA_HRS_HRS0(x)                          (((uint32_t)(((uint32_t)(x)) << DMA_HRS_HRS0_SHIFT)) & DMA_HRS_HRS0_MASK)
#define DMA_HRS_HRS1_MASK                        (0x2U)
#define DMA_HRS_HRS1_SHIFT                       (1U)
#define DMA_HRS_HRS1(x)                          (((uint32_t)(((uint32_t)(x)) << DMA_HRS_HRS1_SHIFT)) & DMA_HRS_HRS1_MASK)
#define DMA_HRS_HRS2_MASK                        (0x4U)
#define DMA_HRS_HRS2_SHIFT                       (2U)
#define DMA_HRS_HRS2(x)                          (((uint32_t)(((uint32_t)(x)) << DMA_HRS_HRS2_SHIFT)) & DMA_HRS_HRS2_MASK)
#define DMA_HRS_HRS3_MASK                        (0x8U)
#define DMA_HRS_HRS3_SHIFT                       (3U)
#define DMA_HRS_HRS3(x)                          (((uint32_t)(((uint32_t)(x)) << DMA_HRS_HRS3_SHIFT)) & DMA_HRS_HRS3_MASK)

/*! @name EARS - Enable Asynchronous Request in Stop Register */
#define DMA_EARS_EDREQ_0_MASK                    (0x1U)
#define DMA_EARS_EDREQ_0_SHIFT                   (0U)
#define DMA_EARS_EDREQ_0(x)                      (((uint32_t)(((uint32_t)(x)) << DMA_EARS_EDREQ_0_SHIFT)) & DMA_EARS_EDREQ_0_MASK)
#define DMA_EARS_EDREQ_1_MASK                    (0x2U)
#define DMA_EARS_EDREQ_1_SHIFT                   (1U)
#define DMA_EARS_EDREQ_1(x)                      (((uint32_t)(((uint32_t)(x)) << DMA_EARS_EDREQ_1_SHIFT)) & DMA_EARS_EDREQ_1_MASK)
#define DMA_EARS_EDREQ_2_MASK                    (0x4U)
#define DMA_EARS_EDREQ_2_SHIFT                   (2U)
#define DMA_EARS_EDREQ_2(x)                      (((uint32_t)(((uint32_t)(x)) << DMA_EARS_EDREQ_2_SHIFT)) & DMA_EARS_EDREQ_2_MASK)
#define DMA_EARS_EDREQ_3_MASK                    (0x8U)
#define DMA_EARS_EDREQ_3_SHIFT                   (3U)
#define DMA_EARS_EDREQ_3(x)                      (((uint32_t)(((uint32_t)(x)) << DMA_EARS_EDREQ_3_SHIFT)) & DMA_EARS_EDREQ_3_MASK)

/*! @name DCHPRI3 - Channel n Priority Register */
#define DMA_DCHPRI3_CHPRI_MASK                   (0x3U)
#define DMA_DCHPRI3_CHPRI_SHIFT                  (0U)
#define DMA_DCHPRI3_CHPRI(x)                     (((uint8_t)(((uint8_t)(x)) << DMA_DCHPRI3_CHPRI_SHIFT)) & DMA_DCHPRI3_CHPRI_MASK)
#define DMA_DCHPRI3_DPA_MASK                     (0x40U)
#define DMA_DCHPRI3_DPA_SHIFT                    (6U)
#define DMA_DCHPRI3_DPA(x)                       (((uint8_t)(((uint8_t)(x)) << DMA_DCHPRI3_DPA_SHIFT)) & DMA_DCHPRI3_DPA_MASK)
#define DMA_DCHPRI3_ECP_MASK                     (0x80U)
#define DMA_DCHPRI3_ECP_SHIFT                    (7U)
#define DMA_DCHPRI3_ECP(x)                       (((uint8_t)(((uint8_t)(x)) << DMA_DCHPRI3_ECP_SHIFT)) & DMA_DCHPRI3_ECP_MASK)

/*! @name DCHPRI2 - Channel n Priority Register */
#define DMA_DCHPRI2_CHPRI_MASK                   (0x3U)
#define DMA_DCHPRI2_CHPRI_SHIFT                  (0U)
#define DMA_DCHPRI2_CHPRI(x)                     (((uint8_t)(((uint8_t)(x)) << DMA_DCHPRI2_CHPRI_SHIFT)) & DMA_DCHPRI2_CHPRI_MASK)
#define DMA_DCHPRI2_DPA_MASK                     (0x40U)
#define DMA_DCHPRI2_DPA_SHIFT                    (6U)
#define DMA_DCHPRI2_DPA(x)                       (((uint8_t)(((uint8_t)(x)) << DMA_DCHPRI2_DPA_SHIFT)) & DMA_DCHPRI2_DPA_MASK)
#define DMA_DCHPRI2_ECP_MASK                     (0x80U)
#define DMA_DCHPRI2_ECP_SHIFT                    (7U)
#define DMA_DCHPRI2_ECP(x)                       (((uint8_t)(((uint8_t)(x)) << DMA_DCHPRI2_ECP_SHIFT)) & DMA_DCHPRI2_ECP_MASK)

/*! @name DCHPRI1 - Channel n Priority Register */
#define DMA_DCHPRI1_CHPRI_MASK                   (0x3U)
#define DMA_DCHPRI1_CHPRI_SHIFT                  (0U)
#define DMA_DCHPRI1_CHPRI(x)                     (((uint8_t)(((uint8_t)(x)) << DMA_DCHPRI1_CHPRI_SHIFT)) & DMA_DCHPRI1_CHPRI_MASK)
#define DMA_DCHPRI1_DPA_MASK                     (0x40U)
#define DMA_DCHPRI1_DPA_SHIFT                    (6U)
#define DMA_DCHPRI1_DPA(x)                       (((uint8_t)(((uint8_t)(x)) << DMA_DCHPRI1_DPA_SHIFT)) & DMA_DCHPRI1_DPA_MASK)
#define DMA_DCHPRI1_ECP_MASK                     (0x80U)
#define DMA_DCHPRI1_ECP_SHIFT                    (7U)
#define DMA_DCHPRI1_ECP(x)                       (((uint8_t)(((uint8_t)(x)) << DMA_DCHPRI1_ECP_SHIFT)) & DMA_DCHPRI1_ECP_MASK)

/*! @name DCHPRI0 - Channel n Priority Register */
#define DMA_DCHPRI0_CHPRI_MASK                   (0x3U)
#define DMA_DCHPRI0_CHPRI_SHIFT                  (0U)
#define DMA_DCHPRI0_CHPRI(x)                     (((uint8_t)(((uint8_t)(x)) << DMA_DCHPRI0_CHPRI_SHIFT)) & DMA_DCHPRI0_CHPRI_MASK)
#define DMA_DCHPRI0_DPA_MASK                     (0x40U)
#define DMA_DCHPRI0_DPA_SHIFT                    (6U)
#define DMA_DCHPRI0_DPA(x)                       (((uint8_t)(((uint8_t)(x)) << DMA_DCHPRI0_DPA_SHIFT)) & DMA_DCHPRI0_DPA_MASK)
#define DMA_DCHPRI0_ECP_MASK                     (0x80U)
#define DMA_DCHPRI0_ECP_SHIFT                    (7U)
#define DMA_DCHPRI0_ECP(x)                       (((uint8_t)(((uint8_t)(x)) << DMA_DCHPRI0_ECP_SHIFT)) & DMA_DCHPRI0_ECP_MASK)

/*! @name SADDR - TCD Source Address */
#define DMA_SADDR_SADDR_MASK                     (0xFFFFFFFFU)
#define DMA_SADDR_SADDR_SHIFT                    (0U)
#define DMA_SADDR_SADDR(x)                       (((uint32_t)(((uint32_t)(x)) << DMA_SADDR_SADDR_SHIFT)) & DMA_SADDR_SADDR_MASK)

/* The count of DMA_SADDR */
#define DMA_SADDR_COUNT                          (4U)

/*! @name SOFF - TCD Signed Source Address Offset */
#define DMA_SOFF_SOFF_MASK                       (0xFFFFU)
#define DMA_SOFF_SOFF_SHIFT                      (0U)
#define DMA_SOFF_SOFF(x)                         (((uint16_t)(((uint16_t)(x)) << DMA_SOFF_SOFF_SHIFT)) & DMA_SOFF_SOFF_MASK)

/* The count of DMA_SOFF */
#define DMA_SOFF_COUNT                           (4U)

/*! @name ATTR - TCD Transfer Attributes */
#define DMA_ATTR_DSIZE_MASK                      (0x7U)
#define DMA_ATTR_DSIZE_SHIFT                     (0U)
#define DMA_ATTR_DSIZE(x)                        (((uint16_t)(((uint16_t)(x)) << DMA_ATTR_DSIZE_SHIFT)) & DMA_ATTR_DSIZE_MASK)
#define DMA_ATTR_DMOD_MASK                       (0xF8U)
#define DMA_ATTR_DMOD_SHIFT                      (3U)
#define DMA_ATTR_DMOD(x)                         (((uint16_t)(((uint16_t)(x)) << DMA_ATTR_DMOD_SHIFT)) & DMA_ATTR_DMOD_MASK)
#define DMA_ATTR_SSIZE_MASK                      (0x700U)
#define DMA_ATTR_SSIZE_SHIFT                     (8U)
#define DMA_ATTR_SSIZE(x)                        (((uint16_t)(((uint16_t)(x)) << DMA_ATTR_SSIZE_SHIFT)) & DMA_ATTR_SSIZE_MASK)
#define DMA_ATTR_SMOD_MASK                       (0xF800U)
#define DMA_ATTR_SMOD_SHIFT                      (11U)
#define DMA_ATTR_SMOD(x)                         (((uint16_t)(((uint16_t)(x)) << DMA_ATTR_SMOD_SHIFT)) & DMA_ATTR_SMOD_MASK)

/* The count of DMA_ATTR */
#define DMA_ATTR_COUNT                           (4U)

/*! @name NBYTES_MLNO - TCD Minor Byte Count (Minor Loop Mapping Disabled) */
#define DMA_NBYTES_MLNO_NBYTES_MASK              (0xFFFFFFFFU)
#define DMA_NBYTES_MLNO_NBYTES_SHIFT             (0U)
#define DMA_NBYTES_MLNO_NBYTES(x)                (((uint32_t)(((uint32_t)(x)) << DMA_NBYTES_MLNO_NBYTES_SHIFT)) & DMA_NBYTES_MLNO_NBYTES_MASK)

/* The count of DMA_NBYTES_MLNO */
#define DMA_NBYTES_MLNO_COUNT                    (4U)

/*! @name NBYTES_MLOFFNO - TCD Signed Minor Loop Offset (Minor Loop Mapping Enabled and Offset Disabled) */
#define DMA_NBYTES_MLOFFNO_NBYTES_MASK           (0x3FFFFFFFU)
#define DMA_NBYTES_MLOFFNO_NBYTES_SHIFT          (0U)
#define DMA_NBYTES_MLOFFNO_NBYTES(x)             (((uint32_t)(((uint32_t)(x)) << DMA_NBYTES_MLOFFNO_NBYTES_SHIFT)) & DMA_NBYTES_MLOFFNO_NBYTES_MASK)
#define DMA_NBYTES_MLOFFNO_DMLOE_MASK            (0x40000000U)
#define DMA_NBYTES_MLOFFNO_DMLOE_SHIFT           (30U)
#define DMA_NBYTES_MLOFFNO_DMLOE(x)              (((uint32_t)(((uint32_t)(x)) << DMA_NBYTES_MLOFFNO_DMLOE_SHIFT)) & DMA_NBYTES_MLOFFNO_DMLOE_MASK)
#define DMA_NBYTES_MLOFFNO_SMLOE_MASK            (0x80000000U)
#define DMA_NBYTES_MLOFFNO_SMLOE_SHIFT           (31U)
#define DMA_NBYTES_MLOFFNO_SMLOE(x)              (((uint32_t)(((uint32_t)(x)) << DMA_NBYTES_MLOFFNO_SMLOE_SHIFT)) & DMA_NBYTES_MLOFFNO_SMLOE_MASK)

/* The count of DMA_NBYTES_MLOFFNO */
#define DMA_NBYTES_MLOFFNO_COUNT                 (4U)

/*! @name NBYTES_MLOFFYES - TCD Signed Minor Loop Offset (Minor Loop Mapping and Offset Enabled) */
#define DMA_NBYTES_MLOFFYES_NBYTES_MASK          (0x3FFU)
#define DMA_NBYTES_MLOFFYES_NBYTES_SHIFT         (0U)
#define DMA_NBYTES_MLOFFYES_NBYTES(x)            (((uint32_t)(((uint32_t)(x)) << DMA_NBYTES_MLOFFYES_NBYTES_SHIFT)) & DMA_NBYTES_MLOFFYES_NBYTES_MASK)
#define DMA_NBYTES_MLOFFYES_MLOFF_MASK           (0x3FFFFC00U)
#define DMA_NBYTES_MLOFFYES_MLOFF_SHIFT          (10U)
#define DMA_NBYTES_MLOFFYES_MLOFF(x)             (((uint32_t)(((uint32_t)(x)) << DMA_NBYTES_MLOFFYES_MLOFF_SHIFT)) & DMA_NBYTES_MLOFFYES_MLOFF_MASK)
#define DMA_NBYTES_MLOFFYES_DMLOE_MASK           (0x40000000U)
#define DMA_NBYTES_MLOFFYES_DMLOE_SHIFT          (30U)
#define DMA_NBYTES_MLOFFYES_DMLOE(x)             (((uint32_t)(((uint32_t)(x)) << DMA_NBYTES_MLOFFYES_DMLOE_SHIFT)) & DMA_NBYTES_MLOFFYES_DMLOE_MASK)
#define DMA_NBYTES_MLOFFYES_SMLOE_MASK           (0x80000000U)
#define DMA_NBYTES_MLOFFYES_SMLOE_SHIFT          (31U)
#define DMA_NBYTES_MLOFFYES_SMLOE(x)             (((uint32_t)(((uint32_t)(x)) << DMA_NBYTES_MLOFFYES_SMLOE_SHIFT)) & DMA_NBYTES_MLOFFYES_SMLOE_MASK)

/* The count of DMA_NBYTES_MLOFFYES */
#define DMA_NBYTES_MLOFFYES_COUNT                (4U)

/*! @name SLAST - TCD Last Source Address Adjustment */
#define DMA_SLAST_SLAST_MASK                     (0xFFFFFFFFU)
#define DMA_SLAST_SLAST_SHIFT                    (0U)
#define DMA_SLAST_SLAST(x)                       (((uint32_t)(((uint32_t)(x)) << DMA_SLAST_SLAST_SHIFT)) & DMA_SLAST_SLAST_MASK)

/* The count of DMA_SLAST */
#define DMA_SLAST_COUNT                          (4U)

/*! @name DADDR - TCD Destination Address */
#define DMA_DADDR_DADDR_MASK                     (0xFFFFFFFFU)
#define DMA_DADDR_DADDR_SHIFT                    (0U)
#define DMA_DADDR_DADDR(x)                       (((uint32_t)(((uint32_t)(x)) << DMA_DADDR_DADDR_SHIFT)) & DMA_DADDR_DADDR_MASK)

/* The count of DMA_DADDR */
#define DMA_DADDR_COUNT                          (4U)

/*! @name DOFF - TCD Signed Destination Address Offset */
#define DMA_DOFF_DOFF_MASK                       (0xFFFFU)
#define DMA_DOFF_DOFF_SHIFT                      (0U)
#define DMA_DOFF_DOFF(x)                         (((uint16_t)(((uint16_t)(x)) << DMA_DOFF_DOFF_SHIFT)) & DMA_DOFF_DOFF_MASK)

/* The count of DMA_DOFF */
#define DMA_DOFF_COUNT                           (4U)

/*! @name CITER_ELINKNO - TCD Current Minor Loop Link, Major Loop Count (Channel Linking Disabled) */
#define DMA_CITER_ELINKNO_CITER_MASK             (0x7FFFU)
#define DMA_CITER_ELINKNO_CITER_SHIFT            (0U)
#define DMA_CITER_ELINKNO_CITER(x)               (((uint16_t)(((uint16_t)(x)) << DMA_CITER_ELINKNO_CITER_SHIFT)) & DMA_CITER_ELINKNO_CITER_MASK)
#define DMA_CITER_ELINKNO_ELINK_MASK             (0x8000U)
#define DMA_CITER_ELINKNO_ELINK_SHIFT            (15U)
#define DMA_CITER_ELINKNO_ELINK(x)               (((uint16_t)(((uint16_t)(x)) << DMA_CITER_ELINKNO_ELINK_SHIFT)) & DMA_CITER_ELINKNO_ELINK_MASK)

/* The count of DMA_CITER_ELINKNO */
#define DMA_CITER_ELINKNO_COUNT                  (4U)

/*! @name CITER_ELINKYES - TCD Current Minor Loop Link, Major Loop Count (Channel Linking Enabled) */
#define DMA_CITER_ELINKYES_CITER_MASK            (0x1FFU)
#define DMA_CITER_ELINKYES_CITER_SHIFT           (0U)
#define DMA_CITER_ELINKYES_CITER(x)              (((uint16_t)(((uint16_t)(x)) << DMA_CITER_ELINKYES_CITER_SHIFT)) & DMA_CITER_ELINKYES_CITER_MASK)
#define DMA_CITER_ELINKYES_LINKCH_MASK           (0x600U)
#define DMA_CITER_ELINKYES_LINKCH_SHIFT          (9U)
#define DMA_CITER_ELINKYES_LINKCH(x)             (((uint16_t)(((uint16_t)(x)) << DMA_CITER_ELINKYES_LINKCH_SHIFT)) & DMA_CITER_ELINKYES_LINKCH_MASK)
#define DMA_CITER_ELINKYES_ELINK_MASK            (0x8000U)
#define DMA_CITER_ELINKYES_ELINK_SHIFT           (15U)
#define DMA_CITER_ELINKYES_ELINK(x)              (((uint16_t)(((uint16_t)(x)) << DMA_CITER_ELINKYES_ELINK_SHIFT)) & DMA_CITER_ELINKYES_ELINK_MASK)

/* The count of DMA_CITER_ELINKYES */
#define DMA_CITER_ELINKYES_COUNT                 (4U)

/*! @name DLAST_SGA - TCD Last Destination Address Adjustment/Scatter Gather Address */
#define DMA_DLAST_SGA_DLASTSGA_MASK              (0xFFFFFFFFU)
#define DMA_DLAST_SGA_DLASTSGA_SHIFT             (0U)
#define DMA_DLAST_SGA_DLASTSGA(x)                (((uint32_t)(((uint32_t)(x)) << DMA_DLAST_SGA_DLASTSGA_SHIFT)) & DMA_DLAST_SGA_DLASTSGA_MASK)

/* The count of DMA_DLAST_SGA */
#define DMA_DLAST_SGA_COUNT                      (4U)

/*! @name CSR - TCD Control and Status */
#define DMA_CSR_START_MASK                       (0x1U)
#define DMA_CSR_START_SHIFT                      (0U)
#define DMA_CSR_START(x)                         (((uint16_t)(((uint16_t)(x)) << DMA_CSR_START_SHIFT)) & DMA_CSR_START_MASK)
#define DMA_CSR_INTMAJOR_MASK                    (0x2U)
#define DMA_CSR_INTMAJOR_SHIFT                   (1U)
#define DMA_CSR_INTMAJOR(x)                      (((uint16_t)(((uint16_t)(x)) << DMA_CSR_INTMAJOR_SHIFT)) & DMA_CSR_INTMAJOR_MASK)
#define DMA_CSR_INTHALF_MASK                     (0x4U)
#define DMA_CSR_INTHALF_SHIFT                    (2U)
#define DMA_CSR_INTHALF(x)                       (((uint16_t)(((uint16_t)(x)) << DMA_CSR_INTHALF_SHIFT)) & DMA_CSR_INTHALF_MASK)
#define DMA_CSR_DREQ_MASK                        (0x8U)
#define DMA_CSR_DREQ_SHIFT                       (3U)
#define DMA_CSR_DREQ(x)                          (((uint16_t)(((uint16_t)(x)) << DMA_CSR_DREQ_SHIFT)) & DMA_CSR_DREQ_MASK)
#define DMA_CSR_ESG_MASK                         (0x10U)
#define DMA_CSR_ESG_SHIFT                        (4U)
#define DMA_CSR_ESG(x)                           (((uint16_t)(((uint16_t)(x)) << DMA_CSR_ESG_SHIFT)) & DMA_CSR_ESG_MASK)
#define DMA_CSR_MAJORELINK_MASK                  (0x20U)
#define DMA_CSR_MAJORELINK_SHIFT                 (5U)
#define DMA_CSR_MAJORELINK(x)                    (((uint16_t)(((uint16_t)(x)) << DMA_CSR_MAJORELINK_SHIFT)) & DMA_CSR_MAJORELINK_MASK)
#define DMA_CSR_ACTIVE_MASK                      (0x40U)
#define DMA_CSR_ACTIVE_SHIFT                     (6U)
#define DMA_CSR_ACTIVE(x)                        (((uint16_t)(((uint16_t)(x)) << DMA_CSR_ACTIVE_SHIFT)) & DMA_CSR_ACTIVE_MASK)
#define DMA_CSR_DONE_MASK                        (0x80U)
#define DMA_CSR_DONE_SHIFT                       (7U)
#define DMA_CSR_DONE(x)                          (((uint16_t)(((uint16_t)(x)) << DMA_CSR_DONE_SHIFT)) & DMA_CSR_DONE_MASK)
#define DMA_CSR_MAJORLINKCH_MASK                 (0x300U)
#define DMA_CSR_MAJORLINKCH_SHIFT                (8U)
#define DMA_CSR_MAJORLINKCH(x)                   (((uint16_t)(((uint16_t)(x)) << DMA_CSR_MAJORLINKCH_SHIFT)) & DMA_CSR_MAJORLINKCH_MASK)
#define DMA_CSR_BWC_MASK                         (0xC000U)
#define DMA_CSR_BWC_SHIFT                        (14U)
#define DMA_CSR_BWC(x)                           (((uint16_t)(((uint16_t)(x)) << DMA_CSR_BWC_SHIFT)) & DMA_CSR_BWC_MASK)

/* The count of DMA_CSR */
#define DMA_CSR_COUNT                            (4U)

/*! @name BITER_ELINKNO - TCD Beginning Minor Loop Link, Major Loop Count (Channel Linking Disabled) */
#define DMA_BITER_ELINKNO_BITER_MASK             (0x7FFFU)
#define DMA_BITER_ELINKNO_BITER_SHIFT            (0U)
#define DMA_BITER_ELINKNO_BITER(x)               (((uint16_t)(((uint16_t)(x)) << DMA_BITER_ELINKNO_BITER_SHIFT)) & DMA_BITER_ELINKNO_BITER_MASK)
#define DMA_BITER_ELINKNO_ELINK_MASK             (0x8000U)
#define DMA_BITER_ELINKNO_ELINK_SHIFT            (15U)
#define DMA_BITER_ELINKNO_ELINK(x)               (((uint16_t)(((uint16_t)(x)) << DMA_BITER_ELINKNO_ELINK_SHIFT)) & DMA_BITER_ELINKNO_ELINK_MASK)

/* The count of DMA_BITER_ELINKNO */
#define DMA_BITER_ELINKNO_COUNT                  (4U)

/*! @name BITER_ELINKYES - TCD Beginning Minor Loop Link, Major Loop Count (Channel Linking Enabled) */
#define DMA_BITER_ELINKYES_BITER_MASK            (0x1FFU)
#define DMA_BITER_ELINKYES_BITER_SHIFT           (0U)
#define DMA_BITER_ELINKYES_BITER(x)              (((uint16_t)(((uint16_t)(x)) << DMA_BITER_ELINKYES_BITER_SHIFT)) & DMA_BITER_ELINKYES_BITER_MASK)
#define DMA_BITER_ELINKYES_LINKCH_MASK           (0x600U)
#define DMA_BITER_ELINKYES_LINKCH_SHIFT          (9U)
#define DMA_BITER_ELINKYES_LINKCH(x)             (((uint16_t)(((uint16_t)(x)) << DMA_BITER_ELINKYES_LINKCH_SHIFT)) & DMA_BITER_ELINKYES_LINKCH_MASK)
#define DMA_BITER_ELINKYES_ELINK_MASK            (0x8000U)
#define DMA_BITER_ELINKYES_ELINK_SHIFT           (15U)
#define DMA_BITER_ELINKYES_ELINK(x)              (((uint16_t)(((uint16_t)(x)) << DMA_BITER_ELINKYES_ELINK_SHIFT)) & DMA_BITER_ELINKYES_ELINK_MASK)

/* The count of DMA_BITER_ELINKYES */
#define DMA_BITER_ELINKYES_COUNT                 (4U)


/*!
 * @}
 */ /* end of group DMA_Register_Masks */


/* DMA - Peripheral instance base addresses */
/** Peripheral DMA base address */
#define DMA_BASE                                 (0x40008000u)
/** Peripheral DMA base pointer */
#define DMA0                                     ((DMA_Type *)DMA_BASE)
/** Array initializer of DMA peripheral base addresses */
#define DMA_BASE_ADDRS                           { DMA_BASE }
/** Array initializer of DMA peripheral base pointers */
#define DMA_BASE_PTRS                            { DMA0 }
/** Interrupt vectors for the DMA peripheral type */
#define DMA_CHN_IRQS                             { DMA0_IRQn, DMA1_IRQn, DMA2_IRQn, DMA3_IRQn }

/*!
 * @}
 */ /* end of group DMA_Peripheral_Access_Layer */


/* ----------------------------------------------------------------------------
   -- DMAMUX Peripheral Access Layer
   ---------------------------------------------------------------------------- */

/*!
 * @addtogroup DMAMUX_Peripheral_Access_Layer DMAMUX Peripheral Access Layer
 * @{
 */

/** DMAMUX - Register Layout Typedef */
typedef struct {
  __IO uint8_t CHCFG[4];                           /**< Channel Configuration register, array offset: 0x0, array step: 0x1 */
} DMAMUX_Type;

/* ----------------------------------------------------------------------------
   -- DMAMUX Register Masks
   ---------------------------------------------------------------------------- */

/*!
 * @addtogroup DMAMUX_Register_Masks DMAMUX Register Masks
 * @{
 */

/*! @name CHCFG - Channel Configuration register */
#define DMAMUX_CHCFG_SOURCE_MASK                 (0x3FU)
#define DMAMUX_CHCFG_SOURCE_SHIFT                (0U)
#define DMAMUX_CHCFG_SOURCE(x)                   (((uint8_t)(((uint8_t)(x)) << DMAMUX_CHCFG_SOURCE_SHIFT)) & DMAMUX_CHCFG_SOURCE_MASK)
#define DMAMUX_CHCFG_TRIG_MASK                   (0x40U)
#define DMAMUX_CHCFG_TRIG_SHIFT                  (6U)
#define DMAMUX_CHCFG_TRIG(x)                     (((uint8_t)(((uint8_t)(x)) << DMAMUX_CHCFG_TRIG_SHIFT)) & DMAMUX_CHCFG_TRIG_MASK)
#define DMAMUX_CHCFG_ENBL_MASK                   (0x80U)
#define DMAMUX_CHCFG_ENBL_SHIFT                  (7U)
#define DMAMUX_CHCFG_ENBL(x)                     (((uint8_t)(((uint8_t)(x)) << DMAMUX_CHCFG_ENBL_SHIFT)) & DMAMUX_CHCFG_ENBL_MASK)

/* The count of DMAMUX_CHCFG */
#define DMAMUX_CHCFG_COUNT                       (4U)


/*!
 * @}
 */ /* end of group DMAMUX_Register_Masks */


/* DMAMUX - Peripheral instance base addresses */
/** Peripheral DMAMUX0 base address */
#define DMAMUX0_BASE                             (0x40021000u)
/** Peripheral DMAMUX0 base pointer */
#define DMAMUX0                                  ((DMAMUX_Type *)DMAMUX0_BASE)
/** Array initializer of DMAMUX peripheral base addresses */
#define DMAMUX_BASE_ADDRS                        { DMAMUX0_BASE }
/** Array initializer of DMAMUX peripheral base pointers */
#define DMAMUX_BASE_PTRS                         { DMAMUX0 }

/*!
 * @}
 */ /* end of group DMAMUX_Peripheral_Access_Layer */


/* ----------------------------------------------------------------------------
   -- FGPIO Peripheral Access Layer
   ---------------------------------------------------------------------------- */

/*!
 * @addtogroup FGPIO_Peripheral_Access_Layer FGPIO Peripheral Access Layer
 * @{
 */

/** FGPIO - Register Layout Typedef */
typedef struct {
  __IO uint32_t PDOR;                              /**< Port Data Output Register, offset: 0x0 */
  __O  uint32_t PSOR;                              /**< Port Set Output Register, offset: 0x4 */
  __O  uint32_t PCOR;                              /**< Port Clear Output Register, offset: 0x8 */
  __O  uint32_t PTOR;                              /**< Port Toggle Output Register, offset: 0xC */
  __I  uint32_t PDIR;                              /**< Port Data Input Register, offset: 0x10 */
  __IO uint32_t PDDR;                              /**< Port Data Direction Register, offset: 0x14 */
} FGPIO_Type;

/* ----------------------------------------------------------------------------
   -- FGPIO Register Masks
   ---------------------------------------------------------------------------- */

/*!
 * @addtogroup FGPIO_Register_Masks FGPIO Register Masks
 * @{
 */

/*! @name PDOR - Port Data Output Register */
#define FGPIO_PDOR_PDO_MASK                      (0xFFFFFFFFU)
#define FGPIO_PDOR_PDO_SHIFT                     (0U)
#define FGPIO_PDOR_PDO(x)                        (((uint32_t)(((uint32_t)(x)) << FGPIO_PDOR_PDO_SHIFT)) & FGPIO_PDOR_PDO_MASK)

/*! @name PSOR - Port Set Output Register */
#define FGPIO_PSOR_PTSO_MASK                     (0xFFFFFFFFU)
#define FGPIO_PSOR_PTSO_SHIFT                    (0U)
#define FGPIO_PSOR_PTSO(x)                       (((uint32_t)(((uint32_t)(x)) << FGPIO_PSOR_PTSO_SHIFT)) & FGPIO_PSOR_PTSO_MASK)

/*! @name PCOR - Port Clear Output Register */
#define FGPIO_PCOR_PTCO_MASK                     (0xFFFFFFFFU)
#define FGPIO_PCOR_PTCO_SHIFT                    (0U)
#define FGPIO_PCOR_PTCO(x)                       (((uint32_t)(((uint32_t)(x)) << FGPIO_PCOR_PTCO_SHIFT)) & FGPIO_PCOR_PTCO_MASK)

/*! @name PTOR - Port Toggle Output Register */
#define FGPIO_PTOR_PTTO_MASK                     (0xFFFFFFFFU)
#define FGPIO_PTOR_PTTO_SHIFT                    (0U)
#define FGPIO_PTOR_PTTO(x)                       (((uint32_t)(((uint32_t)(x)) << FGPIO_PTOR_PTTO_SHIFT)) & FGPIO_PTOR_PTTO_MASK)

/*! @name PDIR - Port Data Input Register */
#define FGPIO_PDIR_PDI_MASK                      (0xFFFFFFFFU)
#define FGPIO_PDIR_PDI_SHIFT                     (0U)
#define FGPIO_PDIR_PDI(x)                        (((uint32_t)(((uint32_t)(x)) << FGPIO_PDIR_PDI_SHIFT)) & FGPIO_PDIR_PDI_MASK)

/*! @name PDDR - Port Data Direction Register */
#define FGPIO_PDDR_PDD_MASK                      (0xFFFFFFFFU)
#define FGPIO_PDDR_PDD_SHIFT                     (0U)
#define FGPIO_PDDR_PDD(x)                        (((uint32_t)(((uint32_t)(x)) << FGPIO_PDDR_PDD_SHIFT)) & FGPIO_PDDR_PDD_MASK)


/*!
 * @}
 */ /* end of group FGPIO_Register_Masks */


/* FGPIO - Peripheral instance base addresses */
/** Peripheral FGPIOA base address */
#define FGPIOA_BASE                              (0xF8000000u)
/** Peripheral FGPIOA base pointer */
#define FGPIOA                                   ((FGPIO_Type *)FGPIOA_BASE)
/** Peripheral FGPIOB base address */
#define FGPIOB_BASE                              (0xF8000040u)
/** Peripheral FGPIOB base pointer */
#define FGPIOB                                   ((FGPIO_Type *)FGPIOB_BASE)
/** Peripheral FGPIOC base address */
#define FGPIOC_BASE                              (0xF8000080u)
/** Peripheral FGPIOC base pointer */
#define FGPIOC                                   ((FGPIO_Type *)FGPIOC_BASE)
/** Array initializer of FGPIO peripheral base addresses */
#define FGPIO_BASE_ADDRS                         { FGPIOA_BASE, FGPIOB_BASE, FGPIOC_BASE }
/** Array initializer of FGPIO peripheral base pointers */
#define FGPIO_BASE_PTRS                          { FGPIOA, FGPIOB, FGPIOC }

/*!
 * @}
 */ /* end of group FGPIO_Peripheral_Access_Layer */


/* ----------------------------------------------------------------------------
   -- FTFA Peripheral Access Layer
   ---------------------------------------------------------------------------- */

/*!
 * @addtogroup FTFA_Peripheral_Access_Layer FTFA Peripheral Access Layer
 * @{
 */

/** FTFA - Register Layout Typedef */
typedef struct {
  __IO uint8_t FSTAT;                              /**< Flash Status Register, offset: 0x0 */
  __IO uint8_t FCNFG;                              /**< Flash Configuration Register, offset: 0x1 */
  __I  uint8_t FSEC;                               /**< Flash Security Register, offset: 0x2 */
  __I  uint8_t FOPT;                               /**< Flash Option Register, offset: 0x3 */
  __IO uint8_t FCCOB3;                             /**< Flash Common Command Object Registers, offset: 0x4 */
  __IO uint8_t FCCOB2;                             /**< Flash Common Command Object Registers, offset: 0x5 */
  __IO uint8_t FCCOB1;                             /**< Flash Common Command Object Registers, offset: 0x6 */
  __IO uint8_t FCCOB0;                             /**< Flash Common Command Object Registers, offset: 0x7 */
  __IO uint8_t FCCOB7;                             /**< Flash Common Command Object Registers, offset: 0x8 */
  __IO uint8_t FCCOB6;                             /**< Flash Common Command Object Registers, offset: 0x9 */
  __IO uint8_t FCCOB5;                             /**< Flash Common Command Object Registers, offset: 0xA */
  __IO uint8_t FCCOB4;                             /**< Flash Common Command Object Registers, offset: 0xB */
  __IO uint8_t FCCOBB;                             /**< Flash Common Command Object Registers, offset: 0xC */
  __IO uint8_t FCCOBA;                             /**< Flash Common Command Object Registers, offset: 0xD */
  __IO uint8_t FCCOB9;                             /**< Flash Common Command Object Registers, offset: 0xE */
  __IO uint8_t FCCOB8;                             /**< Flash Common Command Object Registers, offset: 0xF */
  __IO uint8_t FPROT3;                             /**< Program Flash Protection Registers, offset: 0x10 */
  __IO uint8_t FPROT2;                             /**< Program Flash Protection Registers, offset: 0x11 */
  __IO uint8_t FPROT1;                             /**< Program Flash Protection Registers, offset: 0x12 */
  __IO uint8_t FPROT0;                             /**< Program Flash Protection Registers, offset: 0x13 */
       uint8_t RESERVED_0[4];
  __I  uint8_t XACCH3;                             /**< Execute-only Access Registers, offset: 0x18 */
  __I  uint8_t XACCH2;                             /**< Execute-only Access Registers, offset: 0x19 */
  __I  uint8_t XACCH1;                             /**< Execute-only Access Registers, offset: 0x1A */
  __I  uint8_t XACCH0;                             /**< Execute-only Access Registers, offset: 0x1B */
  __I  uint8_t XACCL3;                             /**< Execute-only Access Registers, offset: 0x1C */
  __I  uint8_t XACCL2;                             /**< Execute-only Access Registers, offset: 0x1D */
  __I  uint8_t XACCL1;                             /**< Execute-only Access Registers, offset: 0x1E */
  __I  uint8_t XACCL0;                             /**< Execute-only Access Registers, offset: 0x1F */
  __I  uint8_t SACCH3;                             /**< Supervisor-only Access Registers, offset: 0x20 */
  __I  uint8_t SACCH2;                             /**< Supervisor-only Access Registers, offset: 0x21 */
  __I  uint8_t SACCH1;                             /**< Supervisor-only Access Registers, offset: 0x22 */
  __I  uint8_t SACCH0;                             /**< Supervisor-only Access Registers, offset: 0x23 */
  __I  uint8_t SACCL3;                             /**< Supervisor-only Access Registers, offset: 0x24 */
  __I  uint8_t SACCL2;                             /**< Supervisor-only Access Registers, offset: 0x25 */
  __I  uint8_t SACCL1;                             /**< Supervisor-only Access Registers, offset: 0x26 */
  __I  uint8_t SACCL0;                             /**< Supervisor-only Access Registers, offset: 0x27 */
  __I  uint8_t FACSS;                              /**< Flash Access Segment Size Register, offset: 0x28 */
       uint8_t RESERVED_1[2];
  __I  uint8_t FACSN;                              /**< Flash Access Segment Number Register, offset: 0x2B */
} FTFA_Type;

/* ----------------------------------------------------------------------------
   -- FTFA Register Masks
   ---------------------------------------------------------------------------- */

/*!
 * @addtogroup FTFA_Register_Masks FTFA Register Masks
 * @{
 */

/*! @name FSTAT - Flash Status Register */
#define FTFA_FSTAT_MGSTAT0_MASK                  (0x1U)
#define FTFA_FSTAT_MGSTAT0_SHIFT                 (0U)
#define FTFA_FSTAT_MGSTAT0(x)                    (((uint8_t)(((uint8_t)(x)) << FTFA_FSTAT_MGSTAT0_SHIFT)) & FTFA_FSTAT_MGSTAT0_MASK)
#define FTFA_FSTAT_FPVIOL_MASK                   (0x10U)
#define FTFA_FSTAT_FPVIOL_SHIFT                  (4U)
#define FTFA_FSTAT_FPVIOL(x)                     (((uint8_t)(((uint8_t)(x)) << FTFA_FSTAT_FPVIOL_SHIFT)) & FTFA_FSTAT_FPVIOL_MASK)
#define FTFA_FSTAT_ACCERR_MASK                   (0x20U)
#define FTFA_FSTAT_ACCERR_SHIFT                  (5U)
#define FTFA_FSTAT_ACCERR(x)                     (((uint8_t)(((uint8_t)(x)) << FTFA_FSTAT_ACCERR_SHIFT)) & FTFA_FSTAT_ACCERR_MASK)
#define FTFA_FSTAT_RDCOLERR_MASK                 (0x40U)
#define FTFA_FSTAT_RDCOLERR_SHIFT                (6U)
#define FTFA_FSTAT_RDCOLERR(x)                   (((uint8_t)(((uint8_t)(x)) << FTFA_FSTAT_RDCOLERR_SHIFT)) & FTFA_FSTAT_RDCOLERR_MASK)
#define FTFA_FSTAT_CCIF_MASK                     (0x80U)
#define FTFA_FSTAT_CCIF_SHIFT                    (7U)
#define FTFA_FSTAT_CCIF(x)                       (((uint8_t)(((uint8_t)(x)) << FTFA_FSTAT_CCIF_SHIFT)) & FTFA_FSTAT_CCIF_MASK)

/*! @name FCNFG - Flash Configuration Register */
#define FTFA_FCNFG_ERSSUSP_MASK                  (0x10U)
#define FTFA_FCNFG_ERSSUSP_SHIFT                 (4U)
#define FTFA_FCNFG_ERSSUSP(x)                    (((uint8_t)(((uint8_t)(x)) << FTFA_FCNFG_ERSSUSP_SHIFT)) & FTFA_FCNFG_ERSSUSP_MASK)
#define FTFA_FCNFG_ERSAREQ_MASK                  (0x20U)
#define FTFA_FCNFG_ERSAREQ_SHIFT                 (5U)
#define FTFA_FCNFG_ERSAREQ(x)                    (((uint8_t)(((uint8_t)(x)) << FTFA_FCNFG_ERSAREQ_SHIFT)) & FTFA_FCNFG_ERSAREQ_MASK)
#define FTFA_FCNFG_RDCOLLIE_MASK                 (0x40U)
#define FTFA_FCNFG_RDCOLLIE_SHIFT                (6U)
#define FTFA_FCNFG_RDCOLLIE(x)                   (((uint8_t)(((uint8_t)(x)) << FTFA_FCNFG_RDCOLLIE_SHIFT)) & FTFA_FCNFG_RDCOLLIE_MASK)
#define FTFA_FCNFG_CCIE_MASK                     (0x80U)
#define FTFA_FCNFG_CCIE_SHIFT                    (7U)
#define FTFA_FCNFG_CCIE(x)                       (((uint8_t)(((uint8_t)(x)) << FTFA_FCNFG_CCIE_SHIFT)) & FTFA_FCNFG_CCIE_MASK)

/*! @name FSEC - Flash Security Register */
#define FTFA_FSEC_SEC_MASK                       (0x3U)
#define FTFA_FSEC_SEC_SHIFT                      (0U)
#define FTFA_FSEC_SEC(x)                         (((uint8_t)(((uint8_t)(x)) << FTFA_FSEC_SEC_SHIFT)) & FTFA_FSEC_SEC_MASK)
#define FTFA_FSEC_FSLACC_MASK                    (0xCU)
#define FTFA_FSEC_FSLACC_SHIFT                   (2U)
#define FTFA_FSEC_FSLACC(x)                      (((uint8_t)(((uint8_t)(x)) << FTFA_FSEC_FSLACC_SHIFT)) & FTFA_FSEC_FSLACC_MASK)
#define FTFA_FSEC_MEEN_MASK                      (0x30U)
#define FTFA_FSEC_MEEN_SHIFT                     (4U)
#define FTFA_FSEC_MEEN(x)                        (((uint8_t)(((uint8_t)(x)) << FTFA_FSEC_MEEN_SHIFT)) & FTFA_FSEC_MEEN_MASK)
#define FTFA_FSEC_KEYEN_MASK                     (0xC0U)
#define FTFA_FSEC_KEYEN_SHIFT                    (6U)
#define FTFA_FSEC_KEYEN(x)                       (((uint8_t)(((uint8_t)(x)) << FTFA_FSEC_KEYEN_SHIFT)) & FTFA_FSEC_KEYEN_MASK)

/*! @name FOPT - Flash Option Register */
#define FTFA_FOPT_OPT_MASK                       (0xFFU)
#define FTFA_FOPT_OPT_SHIFT                      (0U)
#define FTFA_FOPT_OPT(x)                         (((uint8_t)(((uint8_t)(x)) << FTFA_FOPT_OPT_SHIFT)) & FTFA_FOPT_OPT_MASK)

/*! @name FCCOB3 - Flash Common Command Object Registers */
#define FTFA_FCCOB3_CCOBn_MASK                   (0xFFU)
#define FTFA_FCCOB3_CCOBn_SHIFT                  (0U)
#define FTFA_FCCOB3_CCOBn(x)                     (((uint8_t)(((uint8_t)(x)) << FTFA_FCCOB3_CCOBn_SHIFT)) & FTFA_FCCOB3_CCOBn_MASK)

/*! @name FCCOB2 - Flash Common Command Object Registers */
#define FTFA_FCCOB2_CCOBn_MASK                   (0xFFU)
#define FTFA_FCCOB2_CCOBn_SHIFT                  (0U)
#define FTFA_FCCOB2_CCOBn(x)                     (((uint8_t)(((uint8_t)(x)) << FTFA_FCCOB2_CCOBn_SHIFT)) & FTFA_FCCOB2_CCOBn_MASK)

/*! @name FCCOB1 - Flash Common Command Object Registers */
#define FTFA_FCCOB1_CCOBn_MASK                   (0xFFU)
#define FTFA_FCCOB1_CCOBn_SHIFT                  (0U)
#define FTFA_FCCOB1_CCOBn(x)                     (((uint8_t)(((uint8_t)(x)) << FTFA_FCCOB1_CCOBn_SHIFT)) & FTFA_FCCOB1_CCOBn_MASK)

/*! @name FCCOB0 - Flash Common Command Object Registers */
#define FTFA_FCCOB0_CCOBn_MASK                   (0xFFU)
#define FTFA_FCCOB0_CCOBn_SHIFT                  (0U)
#define FTFA_FCCOB0_CCOBn(x)                     (((uint8_t)(((uint8_t)(x)) << FTFA_FCCOB0_CCOBn_SHIFT)) & FTFA_FCCOB0_CCOBn_MASK)

/*! @name FCCOB7 - Flash Common Command Object Registers */
#define FTFA_FCCOB7_CCOBn_MASK                   (0xFFU)
#define FTFA_FCCOB7_CCOBn_SHIFT                  (0U)
#define FTFA_FCCOB7_CCOBn(x)                     (((uint8_t)(((uint8_t)(x)) << FTFA_FCCOB7_CCOBn_SHIFT)) & FTFA_FCCOB7_CCOBn_MASK)

/*! @name FCCOB6 - Flash Common Command Object Registers */
#define FTFA_FCCOB6_CCOBn_MASK                   (0xFFU)
#define FTFA_FCCOB6_CCOBn_SHIFT                  (0U)
#define FTFA_FCCOB6_CCOBn(x)                     (((uint8_t)(((uint8_t)(x)) << FTFA_FCCOB6_CCOBn_SHIFT)) & FTFA_FCCOB6_CCOBn_MASK)

/*! @name FCCOB5 - Flash Common Command Object Registers */
#define FTFA_FCCOB5_CCOBn_MASK                   (0xFFU)
#define FTFA_FCCOB5_CCOBn_SHIFT                  (0U)
#define FTFA_FCCOB5_CCOBn(x)                     (((uint8_t)(((uint8_t)(x)) << FTFA_FCCOB5_CCOBn_SHIFT)) & FTFA_FCCOB5_CCOBn_MASK)

/*! @name FCCOB4 - Flash Common Command Object Registers */
#define FTFA_FCCOB4_CCOBn_MASK                   (0xFFU)
#define FTFA_FCCOB4_CCOBn_SHIFT                  (0U)
#define FTFA_FCCOB4_CCOBn(x)                     (((uint8_t)(((uint8_t)(x)) << FTFA_FCCOB4_CCOBn_SHIFT)) & FTFA_FCCOB4_CCOBn_MASK)

/*! @name FCCOBB - Flash Common Command Object Registers */
#define FTFA_FCCOBB_CCOBn_MASK                   (0xFFU)
#define FTFA_FCCOBB_CCOBn_SHIFT                  (0U)
#define FTFA_FCCOBB_CCOBn(x)                     (((uint8_t)(((uint8_t)(x)) << FTFA_FCCOBB_CCOBn_SHIFT)) & FTFA_FCCOBB_CCOBn_MASK)

/*! @name FCCOBA - Flash Common Command Object Registers */
#define FTFA_FCCOBA_CCOBn_MASK                   (0xFFU)
#define FTFA_FCCOBA_CCOBn_SHIFT                  (0U)
#define FTFA_FCCOBA_CCOBn(x)                     (((uint8_t)(((uint8_t)(x)) << FTFA_FCCOBA_CCOBn_SHIFT)) & FTFA_FCCOBA_CCOBn_MASK)

/*! @name FCCOB9 - Flash Common Command Object Registers */
#define FTFA_FCCOB9_CCOBn_MASK                   (0xFFU)
#define FTFA_FCCOB9_CCOBn_SHIFT                  (0U)
#define FTFA_FCCOB9_CCOBn(x)                     (((uint8_t)(((uint8_t)(x)) << FTFA_FCCOB9_CCOBn_SHIFT)) & FTFA_FCCOB9_CCOBn_MASK)

/*! @name FCCOB8 - Flash Common Command Object Registers */
#define FTFA_FCCOB8_CCOBn_MASK                   (0xFFU)
#define FTFA_FCCOB8_CCOBn_SHIFT                  (0U)
#define FTFA_FCCOB8_CCOBn(x)                     (((uint8_t)(((uint8_t)(x)) << FTFA_FCCOB8_CCOBn_SHIFT)) & FTFA_FCCOB8_CCOBn_MASK)

/*! @name FPROT3 - Program Flash Protection Registers */
#define FTFA_FPROT3_PROT_MASK                    (0xFFU)
#define FTFA_FPROT3_PROT_SHIFT                   (0U)
#define FTFA_FPROT3_PROT(x)                      (((uint8_t)(((uint8_t)(x)) << FTFA_FPROT3_PROT_SHIFT)) & FTFA_FPROT3_PROT_MASK)

/*! @name FPROT2 - Program Flash Protection Registers */
#define FTFA_FPROT2_PROT_MASK                    (0xFFU)
#define FTFA_FPROT2_PROT_SHIFT                   (0U)
#define FTFA_FPROT2_PROT(x)                      (((uint8_t)(((uint8_t)(x)) << FTFA_FPROT2_PROT_SHIFT)) & FTFA_FPROT2_PROT_MASK)

/*! @name FPROT1 - Program Flash Protection Registers */
#define FTFA_FPROT1_PROT_MASK                    (0xFFU)
#define FTFA_FPROT1_PROT_SHIFT                   (0U)
#define FTFA_FPROT1_PROT(x)                      (((uint8_t)(((uint8_t)(x)) << FTFA_FPROT1_PROT_SHIFT)) & FTFA_FPROT1_PROT_MASK)

/*! @name FPROT0 - Program Flash Protection Registers */
#define FTFA_FPROT0_PROT_MASK                    (0xFFU)
#define FTFA_FPROT0_PROT_SHIFT                   (0U)
#define FTFA_FPROT0_PROT(x)                      (((uint8_t)(((uint8_t)(x)) << FTFA_FPROT0_PROT_SHIFT)) & FTFA_FPROT0_PROT_MASK)

/*! @name XACCH3 - Execute-only Access Registers */
#define FTFA_XACCH3_XA_MASK                      (0xFFU)
#define FTFA_XACCH3_XA_SHIFT                     (0U)
#define FTFA_XACCH3_XA(x)                        (((uint8_t)(((uint8_t)(x)) << FTFA_XACCH3_XA_SHIFT)) & FTFA_XACCH3_XA_MASK)

/*! @name XACCH2 - Execute-only Access Registers */
#define FTFA_XACCH2_XA_MASK                      (0xFFU)
#define FTFA_XACCH2_XA_SHIFT                     (0U)
#define FTFA_XACCH2_XA(x)                        (((uint8_t)(((uint8_t)(x)) << FTFA_XACCH2_XA_SHIFT)) & FTFA_XACCH2_XA_MASK)

/*! @name XACCH1 - Execute-only Access Registers */
#define FTFA_XACCH1_XA_MASK                      (0xFFU)
#define FTFA_XACCH1_XA_SHIFT                     (0U)
#define FTFA_XACCH1_XA(x)                        (((uint8_t)(((uint8_t)(x)) << FTFA_XACCH1_XA_SHIFT)) & FTFA_XACCH1_XA_MASK)

/*! @name XACCH0 - Execute-only Access Registers */
#define FTFA_XACCH0_XA_MASK                      (0xFFU)
#define FTFA_XACCH0_XA_SHIFT                     (0U)
#define FTFA_XACCH0_XA(x)                        (((uint8_t)(((uint8_t)(x)) << FTFA_XACCH0_XA_SHIFT)) & FTFA_XACCH0_XA_MASK)

/*! @name XACCL3 - Execute-only Access Registers */
#define FTFA_XACCL3_XA_MASK                      (0xFFU)
#define FTFA_XACCL3_XA_SHIFT                     (0U)
#define FTFA_XACCL3_XA(x)                        (((uint8_t)(((uint8_t)(x)) << FTFA_XACCL3_XA_SHIFT)) & FTFA_XACCL3_XA_MASK)

/*! @name XACCL2 - Execute-only Access Registers */
#define FTFA_XACCL2_XA_MASK                      (0xFFU)
#define FTFA_XACCL2_XA_SHIFT                     (0U)
#define FTFA_XACCL2_XA(x)                        (((uint8_t)(((uint8_t)(x)) << FTFA_XACCL2_XA_SHIFT)) & FTFA_XACCL2_XA_MASK)

/*! @name XACCL1 - Execute-only Access Registers */
#define FTFA_XACCL1_XA_MASK                      (0xFFU)
#define FTFA_XACCL1_XA_SHIFT                     (0U)
#define FTFA_XACCL1_XA(x)                        (((uint8_t)(((uint8_t)(x)) << FTFA_XACCL1_XA_SHIFT)) & FTFA_XACCL1_XA_MASK)

/*! @name XACCL0 - Execute-only Access Registers */
#define FTFA_XACCL0_XA_MASK                      (0xFFU)
#define FTFA_XACCL0_XA_SHIFT                     (0U)
#define FTFA_XACCL0_XA(x)                        (((uint8_t)(((uint8_t)(x)) << FTFA_XACCL0_XA_SHIFT)) & FTFA_XACCL0_XA_MASK)

/*! @name SACCH3 - Supervisor-only Access Registers */
#define FTFA_SACCH3_SA_MASK                      (0xFFU)
#define FTFA_SACCH3_SA_SHIFT                     (0U)
#define FTFA_SACCH3_SA(x)                        (((uint8_t)(((uint8_t)(x)) << FTFA_SACCH3_SA_SHIFT)) & FTFA_SACCH3_SA_MASK)

/*! @name SACCH2 - Supervisor-only Access Registers */
#define FTFA_SACCH2_SA_MASK                      (0xFFU)
#define FTFA_SACCH2_SA_SHIFT                     (0U)
#define FTFA_SACCH2_SA(x)                        (((uint8_t)(((uint8_t)(x)) << FTFA_SACCH2_SA_SHIFT)) & FTFA_SACCH2_SA_MASK)

/*! @name SACCH1 - Supervisor-only Access Registers */
#define FTFA_SACCH1_SA_MASK                      (0xFFU)
#define FTFA_SACCH1_SA_SHIFT                     (0U)
#define FTFA_SACCH1_SA(x)                        (((uint8_t)(((uint8_t)(x)) << FTFA_SACCH1_SA_SHIFT)) & FTFA_SACCH1_SA_MASK)

/*! @name SACCH0 - Supervisor-only Access Registers */
#define FTFA_SACCH0_SA_MASK                      (0xFFU)
#define FTFA_SACCH0_SA_SHIFT                     (0U)
#define FTFA_SACCH0_SA(x)                        (((uint8_t)(((uint8_t)(x)) << FTFA_SACCH0_SA_SHIFT)) & FTFA_SACCH0_SA_MASK)

/*! @name SACCL3 - Supervisor-only Access Registers */
#define FTFA_SACCL3_SA_MASK                      (0xFFU)
#define FTFA_SACCL3_SA_SHIFT                     (0U)
#define FTFA_SACCL3_SA(x)                        (((uint8_t)(((uint8_t)(x)) << FTFA_SACCL3_SA_SHIFT)) & FTFA_SACCL3_SA_MASK)

/*! @name SACCL2 - Supervisor-only Access Registers */
#define FTFA_SACCL2_SA_MASK                      (0xFFU)
#define FTFA_SACCL2_SA_SHIFT                     (0U)
#define FTFA_SACCL2_SA(x)                        (((uint8_t)(((uint8_t)(x)) << FTFA_SACCL2_SA_SHIFT)) & FTFA_SACCL2_SA_MASK)

/*! @name SACCL1 - Supervisor-only Access Registers */
#define FTFA_SACCL1_SA_MASK                      (0xFFU)
#define FTFA_SACCL1_SA_SHIFT                     (0U)
#define FTFA_SACCL1_SA(x)                        (((uint8_t)(((uint8_t)(x)) << FTFA_SACCL1_SA_SHIFT)) & FTFA_SACCL1_SA_MASK)

/*! @name SACCL0 - Supervisor-only Access Registers */
#define FTFA_SACCL0_SA_MASK                      (0xFFU)
#define FTFA_SACCL0_SA_SHIFT                     (0U)
#define FTFA_SACCL0_SA(x)                        (((uint8_t)(((uint8_t)(x)) << FTFA_SACCL0_SA_SHIFT)) & FTFA_SACCL0_SA_MASK)

/*! @name FACSS - Flash Access Segment Size Register */
#define FTFA_FACSS_SGSIZE_MASK                   (0xFFU)
#define FTFA_FACSS_SGSIZE_SHIFT                  (0U)
#define FTFA_FACSS_SGSIZE(x)                     (((uint8_t)(((uint8_t)(x)) << FTFA_FACSS_SGSIZE_SHIFT)) & FTFA_FACSS_SGSIZE_MASK)

/*! @name FACSN - Flash Access Segment Number Register */
#define FTFA_FACSN_NUMSG_MASK                    (0xFFU)
#define FTFA_FACSN_NUMSG_SHIFT                   (0U)
#define FTFA_FACSN_NUMSG(x)                      (((uint8_t)(((uint8_t)(x)) << FTFA_FACSN_NUMSG_SHIFT)) & FTFA_FACSN_NUMSG_MASK)


/*!
 * @}
 */ /* end of group FTFA_Register_Masks */


/* FTFA - Peripheral instance base addresses */
/** Peripheral FTFA base address */
#define FTFA_BASE                                (0x40020000u)
/** Peripheral FTFA base pointer */
#define FTFA                                     ((FTFA_Type *)FTFA_BASE)
/** Array initializer of FTFA peripheral base addresses */
#define FTFA_BASE_ADDRS                          { FTFA_BASE }
/** Array initializer of FTFA peripheral base pointers */
#define FTFA_BASE_PTRS                           { FTFA }
/** Interrupt vectors for the FTFA peripheral type */
#define FTFA_COMMAND_COMPLETE_IRQS               { FTFA_IRQn }

/*!
 * @}
 */ /* end of group FTFA_Peripheral_Access_Layer */


/* ----------------------------------------------------------------------------
   -- GENFSK Peripheral Access Layer
   ---------------------------------------------------------------------------- */

/*!
 * @addtogroup GENFSK_Peripheral_Access_Layer GENFSK Peripheral Access Layer
 * @{
 */

/** GENFSK - Register Layout Typedef */
typedef struct {
  __IO uint32_t IRQ_CTRL;                          /**< IRQ CONTROL, offset: 0x0 */
  __IO uint32_t EVENT_TMR;                         /**< EVENT TIMER, offset: 0x4 */
  __IO uint32_t T1_CMP;                            /**< T1 COMPARE, offset: 0x8 */
  __IO uint32_t T2_CMP;                            /**< T2 COMPARE, offset: 0xC */
  __I  uint32_t TIMESTAMP;                         /**< TIMESTAMP, offset: 0x10 */
  __IO uint32_t XCVR_CTRL;                         /**< TRANSCEIVER CONTROL, offset: 0x14 */
  __I  uint32_t XCVR_STS;                          /**< TRANSCEIVER STATUS, offset: 0x18 */
  __IO uint32_t XCVR_CFG;                          /**< TRANSCEIVER CONFIGURATION, offset: 0x1C */
  __IO uint32_t CHANNEL_NUM;                       /**< CHANNEL NUMBER, offset: 0x20 */
  __IO uint32_t TX_POWER;                          /**< TRANSMIT POWER, offset: 0x24 */
  __IO uint32_t NTW_ADR_CTRL;                      /**< NETWORK ADDRESS CONTROL, offset: 0x28 */
  __IO uint32_t NTW_ADR_0;                         /**< NETWORK ADDRESS 0, offset: 0x2C */
  __IO uint32_t NTW_ADR_1;                         /**< NETWORK ADDRESS 1, offset: 0x30 */
  __IO uint32_t NTW_ADR_2;                         /**< NETWORK ADDRESS 2, offset: 0x34 */
  __IO uint32_t NTW_ADR_3;                         /**< NETWORK ADDRESS 3, offset: 0x38 */
  __IO uint32_t RX_WATERMARK;                      /**< RECEIVE WATERMARK, offset: 0x3C */
  __IO uint32_t DSM_CTRL;                          /**< DSM CONTROL, offset: 0x40 */
  __I  uint32_t PART_ID;                           /**< PART ID, offset: 0x44 */
       uint8_t RESERVED_0[24];
  __IO uint32_t PACKET_CFG;                        /**< PACKET CONFIGURATION, offset: 0x60 */
  __IO uint32_t H0_CFG;                            /**< H0 CONFIGURATION, offset: 0x64 */
  __IO uint32_t H1_CFG;                            /**< H1 CONFIGURATION, offset: 0x68 */
  __IO uint32_t CRC_CFG;                           /**< CRC CONFIGURATION, offset: 0x6C */
  __IO uint32_t CRC_INIT;                          /**< CRC INITIALIZATION, offset: 0x70 */
  __IO uint32_t CRC_POLY;                          /**< CRC POLYNOMIAL, offset: 0x74 */
  __IO uint32_t CRC_XOR_OUT;                       /**< CRC XOR OUT, offset: 0x78 */
  __IO uint32_t WHITEN_CFG;                        /**< WHITENER CONFIGURATION, offset: 0x7C */
  __IO uint32_t WHITEN_POLY;                       /**< WHITENER POLYNOMIAL, offset: 0x80 */
  __IO uint32_t WHITEN_SZ_THR;                     /**< WHITENER SIZE THRESHOLD, offset: 0x84 */
  __IO uint32_t BITRATE;                           /**< BIT RATE, offset: 0x88 */
  __IO uint32_t PB_PARTITION;                      /**< PACKET BUFFER PARTITION POINT, offset: 0x8C */
} GENFSK_Type;

/* ----------------------------------------------------------------------------
   -- GENFSK Register Masks
   ---------------------------------------------------------------------------- */

/*!
 * @addtogroup GENFSK_Register_Masks GENFSK Register Masks
 * @{
 */

/*! @name IRQ_CTRL - IRQ CONTROL */
#define GENFSK_IRQ_CTRL_SEQ_END_IRQ_MASK         (0x1U)
#define GENFSK_IRQ_CTRL_SEQ_END_IRQ_SHIFT        (0U)
#define GENFSK_IRQ_CTRL_SEQ_END_IRQ(x)           (((uint32_t)(((uint32_t)(x)) << GENFSK_IRQ_CTRL_SEQ_END_IRQ_SHIFT)) & GENFSK_IRQ_CTRL_SEQ_END_IRQ_MASK)
#define GENFSK_IRQ_CTRL_TX_IRQ_MASK              (0x2U)
#define GENFSK_IRQ_CTRL_TX_IRQ_SHIFT             (1U)
#define GENFSK_IRQ_CTRL_TX_IRQ(x)                (((uint32_t)(((uint32_t)(x)) << GENFSK_IRQ_CTRL_TX_IRQ_SHIFT)) & GENFSK_IRQ_CTRL_TX_IRQ_MASK)
#define GENFSK_IRQ_CTRL_RX_IRQ_MASK              (0x4U)
#define GENFSK_IRQ_CTRL_RX_IRQ_SHIFT             (2U)
#define GENFSK_IRQ_CTRL_RX_IRQ(x)                (((uint32_t)(((uint32_t)(x)) << GENFSK_IRQ_CTRL_RX_IRQ_SHIFT)) & GENFSK_IRQ_CTRL_RX_IRQ_MASK)
#define GENFSK_IRQ_CTRL_NTW_ADR_IRQ_MASK         (0x8U)
#define GENFSK_IRQ_CTRL_NTW_ADR_IRQ_SHIFT        (3U)
#define GENFSK_IRQ_CTRL_NTW_ADR_IRQ(x)           (((uint32_t)(((uint32_t)(x)) << GENFSK_IRQ_CTRL_NTW_ADR_IRQ_SHIFT)) & GENFSK_IRQ_CTRL_NTW_ADR_IRQ_MASK)
#define GENFSK_IRQ_CTRL_T1_IRQ_MASK              (0x10U)
#define GENFSK_IRQ_CTRL_T1_IRQ_SHIFT             (4U)
#define GENFSK_IRQ_CTRL_T1_IRQ(x)                (((uint32_t)(((uint32_t)(x)) << GENFSK_IRQ_CTRL_T1_IRQ_SHIFT)) & GENFSK_IRQ_CTRL_T1_IRQ_MASK)
#define GENFSK_IRQ_CTRL_T2_IRQ_MASK              (0x20U)
#define GENFSK_IRQ_CTRL_T2_IRQ_SHIFT             (5U)
#define GENFSK_IRQ_CTRL_T2_IRQ(x)                (((uint32_t)(((uint32_t)(x)) << GENFSK_IRQ_CTRL_T2_IRQ_SHIFT)) & GENFSK_IRQ_CTRL_T2_IRQ_MASK)
#define GENFSK_IRQ_CTRL_PLL_UNLOCK_IRQ_MASK      (0x40U)
#define GENFSK_IRQ_CTRL_PLL_UNLOCK_IRQ_SHIFT     (6U)
#define GENFSK_IRQ_CTRL_PLL_UNLOCK_IRQ(x)        (((uint32_t)(((uint32_t)(x)) << GENFSK_IRQ_CTRL_PLL_UNLOCK_IRQ_SHIFT)) & GENFSK_IRQ_CTRL_PLL_UNLOCK_IRQ_MASK)
#define GENFSK_IRQ_CTRL_WAKE_IRQ_MASK            (0x80U)
#define GENFSK_IRQ_CTRL_WAKE_IRQ_SHIFT           (7U)
#define GENFSK_IRQ_CTRL_WAKE_IRQ(x)              (((uint32_t)(((uint32_t)(x)) << GENFSK_IRQ_CTRL_WAKE_IRQ_SHIFT)) & GENFSK_IRQ_CTRL_WAKE_IRQ_MASK)
#define GENFSK_IRQ_CTRL_RX_WATERMARK_IRQ_MASK    (0x100U)
#define GENFSK_IRQ_CTRL_RX_WATERMARK_IRQ_SHIFT   (8U)
#define GENFSK_IRQ_CTRL_RX_WATERMARK_IRQ(x)      (((uint32_t)(((uint32_t)(x)) << GENFSK_IRQ_CTRL_RX_WATERMARK_IRQ_SHIFT)) & GENFSK_IRQ_CTRL_RX_WATERMARK_IRQ_MASK)
#define GENFSK_IRQ_CTRL_TSM_IRQ_MASK             (0x200U)
#define GENFSK_IRQ_CTRL_TSM_IRQ_SHIFT            (9U)
#define GENFSK_IRQ_CTRL_TSM_IRQ(x)               (((uint32_t)(((uint32_t)(x)) << GENFSK_IRQ_CTRL_TSM_IRQ_SHIFT)) & GENFSK_IRQ_CTRL_TSM_IRQ_MASK)
#define GENFSK_IRQ_CTRL_SEQ_END_IRQ_EN_MASK      (0x10000U)
#define GENFSK_IRQ_CTRL_SEQ_END_IRQ_EN_SHIFT     (16U)
#define GENFSK_IRQ_CTRL_SEQ_END_IRQ_EN(x)        (((uint32_t)(((uint32_t)(x)) << GENFSK_IRQ_CTRL_SEQ_END_IRQ_EN_SHIFT)) & GENFSK_IRQ_CTRL_SEQ_END_IRQ_EN_MASK)
#define GENFSK_IRQ_CTRL_TX_IRQ_EN_MASK           (0x20000U)
#define GENFSK_IRQ_CTRL_TX_IRQ_EN_SHIFT          (17U)
#define GENFSK_IRQ_CTRL_TX_IRQ_EN(x)             (((uint32_t)(((uint32_t)(x)) << GENFSK_IRQ_CTRL_TX_IRQ_EN_SHIFT)) & GENFSK_IRQ_CTRL_TX_IRQ_EN_MASK)
#define GENFSK_IRQ_CTRL_RX_IRQ_EN_MASK           (0x40000U)
#define GENFSK_IRQ_CTRL_RX_IRQ_EN_SHIFT          (18U)
#define GENFSK_IRQ_CTRL_RX_IRQ_EN(x)             (((uint32_t)(((uint32_t)(x)) << GENFSK_IRQ_CTRL_RX_IRQ_EN_SHIFT)) & GENFSK_IRQ_CTRL_RX_IRQ_EN_MASK)
#define GENFSK_IRQ_CTRL_NTW_ADR_IRQ_EN_MASK      (0x80000U)
#define GENFSK_IRQ_CTRL_NTW_ADR_IRQ_EN_SHIFT     (19U)
#define GENFSK_IRQ_CTRL_NTW_ADR_IRQ_EN(x)        (((uint32_t)(((uint32_t)(x)) << GENFSK_IRQ_CTRL_NTW_ADR_IRQ_EN_SHIFT)) & GENFSK_IRQ_CTRL_NTW_ADR_IRQ_EN_MASK)
#define GENFSK_IRQ_CTRL_T1_IRQ_EN_MASK           (0x100000U)
#define GENFSK_IRQ_CTRL_T1_IRQ_EN_SHIFT          (20U)
#define GENFSK_IRQ_CTRL_T1_IRQ_EN(x)             (((uint32_t)(((uint32_t)(x)) << GENFSK_IRQ_CTRL_T1_IRQ_EN_SHIFT)) & GENFSK_IRQ_CTRL_T1_IRQ_EN_MASK)
#define GENFSK_IRQ_CTRL_T2_IRQ_EN_MASK           (0x200000U)
#define GENFSK_IRQ_CTRL_T2_IRQ_EN_SHIFT          (21U)
#define GENFSK_IRQ_CTRL_T2_IRQ_EN(x)             (((uint32_t)(((uint32_t)(x)) << GENFSK_IRQ_CTRL_T2_IRQ_EN_SHIFT)) & GENFSK_IRQ_CTRL_T2_IRQ_EN_MASK)
#define GENFSK_IRQ_CTRL_PLL_UNLOCK_IRQ_EN_MASK   (0x400000U)
#define GENFSK_IRQ_CTRL_PLL_UNLOCK_IRQ_EN_SHIFT  (22U)
#define GENFSK_IRQ_CTRL_PLL_UNLOCK_IRQ_EN(x)     (((uint32_t)(((uint32_t)(x)) << GENFSK_IRQ_CTRL_PLL_UNLOCK_IRQ_EN_SHIFT)) & GENFSK_IRQ_CTRL_PLL_UNLOCK_IRQ_EN_MASK)
#define GENFSK_IRQ_CTRL_WAKE_IRQ_EN_MASK         (0x800000U)
#define GENFSK_IRQ_CTRL_WAKE_IRQ_EN_SHIFT        (23U)
#define GENFSK_IRQ_CTRL_WAKE_IRQ_EN(x)           (((uint32_t)(((uint32_t)(x)) << GENFSK_IRQ_CTRL_WAKE_IRQ_EN_SHIFT)) & GENFSK_IRQ_CTRL_WAKE_IRQ_EN_MASK)
#define GENFSK_IRQ_CTRL_RX_WATERMARK_IRQ_EN_MASK (0x1000000U)
#define GENFSK_IRQ_CTRL_RX_WATERMARK_IRQ_EN_SHIFT (24U)
#define GENFSK_IRQ_CTRL_RX_WATERMARK_IRQ_EN(x)   (((uint32_t)(((uint32_t)(x)) << GENFSK_IRQ_CTRL_RX_WATERMARK_IRQ_EN_SHIFT)) & GENFSK_IRQ_CTRL_RX_WATERMARK_IRQ_EN_MASK)
#define GENFSK_IRQ_CTRL_TSM_IRQ_EN_MASK          (0x2000000U)
#define GENFSK_IRQ_CTRL_TSM_IRQ_EN_SHIFT         (25U)
#define GENFSK_IRQ_CTRL_TSM_IRQ_EN(x)            (((uint32_t)(((uint32_t)(x)) << GENFSK_IRQ_CTRL_TSM_IRQ_EN_SHIFT)) & GENFSK_IRQ_CTRL_TSM_IRQ_EN_MASK)
#define GENFSK_IRQ_CTRL_GENERIC_FSK_IRQ_EN_MASK  (0x4000000U)
#define GENFSK_IRQ_CTRL_GENERIC_FSK_IRQ_EN_SHIFT (26U)
#define GENFSK_IRQ_CTRL_GENERIC_FSK_IRQ_EN(x)    (((uint32_t)(((uint32_t)(x)) << GENFSK_IRQ_CTRL_GENERIC_FSK_IRQ_EN_SHIFT)) & GENFSK_IRQ_CTRL_GENERIC_FSK_IRQ_EN_MASK)
#define GENFSK_IRQ_CTRL_CRC_IGNORE_MASK          (0x8000000U)
#define GENFSK_IRQ_CTRL_CRC_IGNORE_SHIFT         (27U)
#define GENFSK_IRQ_CTRL_CRC_IGNORE(x)            (((uint32_t)(((uint32_t)(x)) << GENFSK_IRQ_CTRL_CRC_IGNORE_SHIFT)) & GENFSK_IRQ_CTRL_CRC_IGNORE_MASK)
#define GENFSK_IRQ_CTRL_CRC_VALID_MASK           (0x80000000U)
#define GENFSK_IRQ_CTRL_CRC_VALID_SHIFT          (31U)
#define GENFSK_IRQ_CTRL_CRC_VALID(x)             (((uint32_t)(((uint32_t)(x)) << GENFSK_IRQ_CTRL_CRC_VALID_SHIFT)) & GENFSK_IRQ_CTRL_CRC_VALID_MASK)

/*! @name EVENT_TMR - EVENT TIMER */
#define GENFSK_EVENT_TMR_EVENT_TMR_MASK          (0xFFFFFFU)
#define GENFSK_EVENT_TMR_EVENT_TMR_SHIFT         (0U)
#define GENFSK_EVENT_TMR_EVENT_TMR(x)            (((uint32_t)(((uint32_t)(x)) << GENFSK_EVENT_TMR_EVENT_TMR_SHIFT)) & GENFSK_EVENT_TMR_EVENT_TMR_MASK)
#define GENFSK_EVENT_TMR_EVENT_TMR_LD_MASK       (0x1000000U)
#define GENFSK_EVENT_TMR_EVENT_TMR_LD_SHIFT      (24U)
#define GENFSK_EVENT_TMR_EVENT_TMR_LD(x)         (((uint32_t)(((uint32_t)(x)) << GENFSK_EVENT_TMR_EVENT_TMR_LD_SHIFT)) & GENFSK_EVENT_TMR_EVENT_TMR_LD_MASK)
#define GENFSK_EVENT_TMR_EVENT_TMR_ADD_MASK      (0x2000000U)
#define GENFSK_EVENT_TMR_EVENT_TMR_ADD_SHIFT     (25U)
#define GENFSK_EVENT_TMR_EVENT_TMR_ADD(x)        (((uint32_t)(((uint32_t)(x)) << GENFSK_EVENT_TMR_EVENT_TMR_ADD_SHIFT)) & GENFSK_EVENT_TMR_EVENT_TMR_ADD_MASK)

/*! @name T1_CMP - T1 COMPARE */
#define GENFSK_T1_CMP_T1_CMP_MASK                (0xFFFFFFU)
#define GENFSK_T1_CMP_T1_CMP_SHIFT               (0U)
#define GENFSK_T1_CMP_T1_CMP(x)                  (((uint32_t)(((uint32_t)(x)) << GENFSK_T1_CMP_T1_CMP_SHIFT)) & GENFSK_T1_CMP_T1_CMP_MASK)
#define GENFSK_T1_CMP_T1_CMP_EN_MASK             (0x1000000U)
#define GENFSK_T1_CMP_T1_CMP_EN_SHIFT            (24U)
#define GENFSK_T1_CMP_T1_CMP_EN(x)               (((uint32_t)(((uint32_t)(x)) << GENFSK_T1_CMP_T1_CMP_EN_SHIFT)) & GENFSK_T1_CMP_T1_CMP_EN_MASK)

/*! @name T2_CMP - T2 COMPARE */
#define GENFSK_T2_CMP_T2_CMP_MASK                (0xFFFFFFU)
#define GENFSK_T2_CMP_T2_CMP_SHIFT               (0U)
#define GENFSK_T2_CMP_T2_CMP(x)                  (((uint32_t)(((uint32_t)(x)) << GENFSK_T2_CMP_T2_CMP_SHIFT)) & GENFSK_T2_CMP_T2_CMP_MASK)
#define GENFSK_T2_CMP_T2_CMP_EN_MASK             (0x1000000U)
#define GENFSK_T2_CMP_T2_CMP_EN_SHIFT            (24U)
#define GENFSK_T2_CMP_T2_CMP_EN(x)               (((uint32_t)(((uint32_t)(x)) << GENFSK_T2_CMP_T2_CMP_EN_SHIFT)) & GENFSK_T2_CMP_T2_CMP_EN_MASK)

/*! @name TIMESTAMP - TIMESTAMP */
#define GENFSK_TIMESTAMP_TIMESTAMP_MASK          (0xFFFFFFU)
#define GENFSK_TIMESTAMP_TIMESTAMP_SHIFT         (0U)
#define GENFSK_TIMESTAMP_TIMESTAMP(x)            (((uint32_t)(((uint32_t)(x)) << GENFSK_TIMESTAMP_TIMESTAMP_SHIFT)) & GENFSK_TIMESTAMP_TIMESTAMP_MASK)

/*! @name XCVR_CTRL - TRANSCEIVER CONTROL */
#define GENFSK_XCVR_CTRL_SEQCMD_MASK             (0xFU)
#define GENFSK_XCVR_CTRL_SEQCMD_SHIFT            (0U)
#define GENFSK_XCVR_CTRL_SEQCMD(x)               (((uint32_t)(((uint32_t)(x)) << GENFSK_XCVR_CTRL_SEQCMD_SHIFT)) & GENFSK_XCVR_CTRL_SEQCMD_MASK)
#define GENFSK_XCVR_CTRL_CMDDEC_CS_MASK          (0x7000000U)
#define GENFSK_XCVR_CTRL_CMDDEC_CS_SHIFT         (24U)
#define GENFSK_XCVR_CTRL_CMDDEC_CS(x)            (((uint32_t)(((uint32_t)(x)) << GENFSK_XCVR_CTRL_CMDDEC_CS_SHIFT)) & GENFSK_XCVR_CTRL_CMDDEC_CS_MASK)
#define GENFSK_XCVR_CTRL_XCVR_BUSY_MASK          (0x80000000U)
#define GENFSK_XCVR_CTRL_XCVR_BUSY_SHIFT         (31U)
#define GENFSK_XCVR_CTRL_XCVR_BUSY(x)            (((uint32_t)(((uint32_t)(x)) << GENFSK_XCVR_CTRL_XCVR_BUSY_SHIFT)) & GENFSK_XCVR_CTRL_XCVR_BUSY_MASK)

/*! @name XCVR_STS - TRANSCEIVER STATUS */
#define GENFSK_XCVR_STS_TX_START_T1_PEND_MASK    (0x1U)
#define GENFSK_XCVR_STS_TX_START_T1_PEND_SHIFT   (0U)
#define GENFSK_XCVR_STS_TX_START_T1_PEND(x)      (((uint32_t)(((uint32_t)(x)) << GENFSK_XCVR_STS_TX_START_T1_PEND_SHIFT)) & GENFSK_XCVR_STS_TX_START_T1_PEND_MASK)
#define GENFSK_XCVR_STS_TX_START_T2_PEND_MASK    (0x2U)
#define GENFSK_XCVR_STS_TX_START_T2_PEND_SHIFT   (1U)
#define GENFSK_XCVR_STS_TX_START_T2_PEND(x)      (((uint32_t)(((uint32_t)(x)) << GENFSK_XCVR_STS_TX_START_T2_PEND_SHIFT)) & GENFSK_XCVR_STS_TX_START_T2_PEND_MASK)
#define GENFSK_XCVR_STS_TX_IN_WARMUP_MASK        (0x4U)
#define GENFSK_XCVR_STS_TX_IN_WARMUP_SHIFT       (2U)
#define GENFSK_XCVR_STS_TX_IN_WARMUP(x)          (((uint32_t)(((uint32_t)(x)) << GENFSK_XCVR_STS_TX_IN_WARMUP_SHIFT)) & GENFSK_XCVR_STS_TX_IN_WARMUP_MASK)
#define GENFSK_XCVR_STS_TX_IN_PROGRESS_MASK      (0x8U)
#define GENFSK_XCVR_STS_TX_IN_PROGRESS_SHIFT     (3U)
#define GENFSK_XCVR_STS_TX_IN_PROGRESS(x)        (((uint32_t)(((uint32_t)(x)) << GENFSK_XCVR_STS_TX_IN_PROGRESS_SHIFT)) & GENFSK_XCVR_STS_TX_IN_PROGRESS_MASK)
#define GENFSK_XCVR_STS_TX_IN_WARMDN_MASK        (0x10U)
#define GENFSK_XCVR_STS_TX_IN_WARMDN_SHIFT       (4U)
#define GENFSK_XCVR_STS_TX_IN_WARMDN(x)          (((uint32_t)(((uint32_t)(x)) << GENFSK_XCVR_STS_TX_IN_WARMDN_SHIFT)) & GENFSK_XCVR_STS_TX_IN_WARMDN_MASK)
#define GENFSK_XCVR_STS_RX_START_T1_PEND_MASK    (0x20U)
#define GENFSK_XCVR_STS_RX_START_T1_PEND_SHIFT   (5U)
#define GENFSK_XCVR_STS_RX_START_T1_PEND(x)      (((uint32_t)(((uint32_t)(x)) << GENFSK_XCVR_STS_RX_START_T1_PEND_SHIFT)) & GENFSK_XCVR_STS_RX_START_T1_PEND_MASK)
#define GENFSK_XCVR_STS_RX_START_T2_PEND_MASK    (0x40U)
#define GENFSK_XCVR_STS_RX_START_T2_PEND_SHIFT   (6U)
#define GENFSK_XCVR_STS_RX_START_T2_PEND(x)      (((uint32_t)(((uint32_t)(x)) << GENFSK_XCVR_STS_RX_START_T2_PEND_SHIFT)) & GENFSK_XCVR_STS_RX_START_T2_PEND_MASK)
#define GENFSK_XCVR_STS_RX_STOP_T1_PEND_MASK     (0x80U)
#define GENFSK_XCVR_STS_RX_STOP_T1_PEND_SHIFT    (7U)
#define GENFSK_XCVR_STS_RX_STOP_T1_PEND(x)       (((uint32_t)(((uint32_t)(x)) << GENFSK_XCVR_STS_RX_STOP_T1_PEND_SHIFT)) & GENFSK_XCVR_STS_RX_STOP_T1_PEND_MASK)
#define GENFSK_XCVR_STS_RX_STOP_T2_PEND_MASK     (0x100U)
#define GENFSK_XCVR_STS_RX_STOP_T2_PEND_SHIFT    (8U)
#define GENFSK_XCVR_STS_RX_STOP_T2_PEND(x)       (((uint32_t)(((uint32_t)(x)) << GENFSK_XCVR_STS_RX_STOP_T2_PEND_SHIFT)) & GENFSK_XCVR_STS_RX_STOP_T2_PEND_MASK)
#define GENFSK_XCVR_STS_RX_IN_WARMUP_MASK        (0x200U)
#define GENFSK_XCVR_STS_RX_IN_WARMUP_SHIFT       (9U)
#define GENFSK_XCVR_STS_RX_IN_WARMUP(x)          (((uint32_t)(((uint32_t)(x)) << GENFSK_XCVR_STS_RX_IN_WARMUP_SHIFT)) & GENFSK_XCVR_STS_RX_IN_WARMUP_MASK)
#define GENFSK_XCVR_STS_RX_IN_SEARCH_MASK        (0x400U)
#define GENFSK_XCVR_STS_RX_IN_SEARCH_SHIFT       (10U)
#define GENFSK_XCVR_STS_RX_IN_SEARCH(x)          (((uint32_t)(((uint32_t)(x)) << GENFSK_XCVR_STS_RX_IN_SEARCH_SHIFT)) & GENFSK_XCVR_STS_RX_IN_SEARCH_MASK)
#define GENFSK_XCVR_STS_RX_IN_PROGRESS_MASK      (0x800U)
#define GENFSK_XCVR_STS_RX_IN_PROGRESS_SHIFT     (11U)
#define GENFSK_XCVR_STS_RX_IN_PROGRESS(x)        (((uint32_t)(((uint32_t)(x)) << GENFSK_XCVR_STS_RX_IN_PROGRESS_SHIFT)) & GENFSK_XCVR_STS_RX_IN_PROGRESS_MASK)
#define GENFSK_XCVR_STS_RX_IN_WARMDN_MASK        (0x1000U)
#define GENFSK_XCVR_STS_RX_IN_WARMDN_SHIFT       (12U)
#define GENFSK_XCVR_STS_RX_IN_WARMDN(x)          (((uint32_t)(((uint32_t)(x)) << GENFSK_XCVR_STS_RX_IN_WARMDN_SHIFT)) & GENFSK_XCVR_STS_RX_IN_WARMDN_MASK)
#define GENFSK_XCVR_STS_LQI_VALID_MASK           (0x4000U)
#define GENFSK_XCVR_STS_LQI_VALID_SHIFT          (14U)
#define GENFSK_XCVR_STS_LQI_VALID(x)             (((uint32_t)(((uint32_t)(x)) << GENFSK_XCVR_STS_LQI_VALID_SHIFT)) & GENFSK_XCVR_STS_LQI_VALID_MASK)
#define GENFSK_XCVR_STS_CRC_VALID_MASK           (0x8000U)
#define GENFSK_XCVR_STS_CRC_VALID_SHIFT          (15U)
#define GENFSK_XCVR_STS_CRC_VALID(x)             (((uint32_t)(((uint32_t)(x)) << GENFSK_XCVR_STS_CRC_VALID_SHIFT)) & GENFSK_XCVR_STS_CRC_VALID_MASK)
#define GENFSK_XCVR_STS_RSSI_MASK                (0xFF0000U)
#define GENFSK_XCVR_STS_RSSI_SHIFT               (16U)
#define GENFSK_XCVR_STS_RSSI(x)                  (((uint32_t)(((uint32_t)(x)) << GENFSK_XCVR_STS_RSSI_SHIFT)) & GENFSK_XCVR_STS_RSSI_MASK)
#define GENFSK_XCVR_STS_LQI_MASK                 (0xFF000000U)
#define GENFSK_XCVR_STS_LQI_SHIFT                (24U)
#define GENFSK_XCVR_STS_LQI(x)                   (((uint32_t)(((uint32_t)(x)) << GENFSK_XCVR_STS_LQI_SHIFT)) & GENFSK_XCVR_STS_LQI_MASK)

/*! @name XCVR_CFG - TRANSCEIVER CONFIGURATION */
#define GENFSK_XCVR_CFG_TX_WHITEN_DIS_MASK       (0x1U)
#define GENFSK_XCVR_CFG_TX_WHITEN_DIS_SHIFT      (0U)
#define GENFSK_XCVR_CFG_TX_WHITEN_DIS(x)         (((uint32_t)(((uint32_t)(x)) << GENFSK_XCVR_CFG_TX_WHITEN_DIS_SHIFT)) & GENFSK_XCVR_CFG_TX_WHITEN_DIS_MASK)
#define GENFSK_XCVR_CFG_RX_DEWHITEN_DIS_MASK     (0x2U)
#define GENFSK_XCVR_CFG_RX_DEWHITEN_DIS_SHIFT    (1U)
#define GENFSK_XCVR_CFG_RX_DEWHITEN_DIS(x)       (((uint32_t)(((uint32_t)(x)) << GENFSK_XCVR_CFG_RX_DEWHITEN_DIS_SHIFT)) & GENFSK_XCVR_CFG_RX_DEWHITEN_DIS_MASK)
#define GENFSK_XCVR_CFG_SW_CRC_EN_MASK           (0x4U)
#define GENFSK_XCVR_CFG_SW_CRC_EN_SHIFT          (2U)
#define GENFSK_XCVR_CFG_SW_CRC_EN(x)             (((uint32_t)(((uint32_t)(x)) << GENFSK_XCVR_CFG_SW_CRC_EN_SHIFT)) & GENFSK_XCVR_CFG_SW_CRC_EN_MASK)
#define GENFSK_XCVR_CFG_PREAMBLE_SZ_MASK         (0x70U)
#define GENFSK_XCVR_CFG_PREAMBLE_SZ_SHIFT        (4U)
#define GENFSK_XCVR_CFG_PREAMBLE_SZ(x)           (((uint32_t)(((uint32_t)(x)) << GENFSK_XCVR_CFG_PREAMBLE_SZ_SHIFT)) & GENFSK_XCVR_CFG_PREAMBLE_SZ_MASK)
#define GENFSK_XCVR_CFG_TX_WARMUP_MASK           (0xFF00U)
#define GENFSK_XCVR_CFG_TX_WARMUP_SHIFT          (8U)
#define GENFSK_XCVR_CFG_TX_WARMUP(x)             (((uint32_t)(((uint32_t)(x)) << GENFSK_XCVR_CFG_TX_WARMUP_SHIFT)) & GENFSK_XCVR_CFG_TX_WARMUP_MASK)
#define GENFSK_XCVR_CFG_RX_WARMUP_MASK           (0xFF0000U)
#define GENFSK_XCVR_CFG_RX_WARMUP_SHIFT          (16U)
#define GENFSK_XCVR_CFG_RX_WARMUP(x)             (((uint32_t)(((uint32_t)(x)) << GENFSK_XCVR_CFG_RX_WARMUP_SHIFT)) & GENFSK_XCVR_CFG_RX_WARMUP_MASK)

/*! @name CHANNEL_NUM - CHANNEL NUMBER */
#define GENFSK_CHANNEL_NUM_CHANNEL_NUM_MASK      (0x7FU)
#define GENFSK_CHANNEL_NUM_CHANNEL_NUM_SHIFT     (0U)
#define GENFSK_CHANNEL_NUM_CHANNEL_NUM(x)        (((uint32_t)(((uint32_t)(x)) << GENFSK_CHANNEL_NUM_CHANNEL_NUM_SHIFT)) & GENFSK_CHANNEL_NUM_CHANNEL_NUM_MASK)

/*! @name TX_POWER - TRANSMIT POWER */
#define GENFSK_TX_POWER_TX_POWER_MASK            (0x3FU)
#define GENFSK_TX_POWER_TX_POWER_SHIFT           (0U)
#define GENFSK_TX_POWER_TX_POWER(x)              (((uint32_t)(((uint32_t)(x)) << GENFSK_TX_POWER_TX_POWER_SHIFT)) & GENFSK_TX_POWER_TX_POWER_MASK)

/*! @name NTW_ADR_CTRL - NETWORK ADDRESS CONTROL */
#define GENFSK_NTW_ADR_CTRL_NTW_ADR_EN_MASK      (0xFU)
#define GENFSK_NTW_ADR_CTRL_NTW_ADR_EN_SHIFT     (0U)
#define GENFSK_NTW_ADR_CTRL_NTW_ADR_EN(x)        (((uint32_t)(((uint32_t)(x)) << GENFSK_NTW_ADR_CTRL_NTW_ADR_EN_SHIFT)) & GENFSK_NTW_ADR_CTRL_NTW_ADR_EN_MASK)
#define GENFSK_NTW_ADR_CTRL_NTW_ADR_MCH_MASK     (0xF0U)
#define GENFSK_NTW_ADR_CTRL_NTW_ADR_MCH_SHIFT    (4U)
#define GENFSK_NTW_ADR_CTRL_NTW_ADR_MCH(x)       (((uint32_t)(((uint32_t)(x)) << GENFSK_NTW_ADR_CTRL_NTW_ADR_MCH_SHIFT)) & GENFSK_NTW_ADR_CTRL_NTW_ADR_MCH_MASK)
#define GENFSK_NTW_ADR_CTRL_NTW_ADR0_SZ_MASK     (0x300U)
#define GENFSK_NTW_ADR_CTRL_NTW_ADR0_SZ_SHIFT    (8U)
#define GENFSK_NTW_ADR_CTRL_NTW_ADR0_SZ(x)       (((uint32_t)(((uint32_t)(x)) << GENFSK_NTW_ADR_CTRL_NTW_ADR0_SZ_SHIFT)) & GENFSK_NTW_ADR_CTRL_NTW_ADR0_SZ_MASK)
#define GENFSK_NTW_ADR_CTRL_NTW_ADR1_SZ_MASK     (0xC00U)
#define GENFSK_NTW_ADR_CTRL_NTW_ADR1_SZ_SHIFT    (10U)
#define GENFSK_NTW_ADR_CTRL_NTW_ADR1_SZ(x)       (((uint32_t)(((uint32_t)(x)) << GENFSK_NTW_ADR_CTRL_NTW_ADR1_SZ_SHIFT)) & GENFSK_NTW_ADR_CTRL_NTW_ADR1_SZ_MASK)
#define GENFSK_NTW_ADR_CTRL_NTW_ADR2_SZ_MASK     (0x3000U)
#define GENFSK_NTW_ADR_CTRL_NTW_ADR2_SZ_SHIFT    (12U)
#define GENFSK_NTW_ADR_CTRL_NTW_ADR2_SZ(x)       (((uint32_t)(((uint32_t)(x)) << GENFSK_NTW_ADR_CTRL_NTW_ADR2_SZ_SHIFT)) & GENFSK_NTW_ADR_CTRL_NTW_ADR2_SZ_MASK)
#define GENFSK_NTW_ADR_CTRL_NTW_ADR3_SZ_MASK     (0xC000U)
#define GENFSK_NTW_ADR_CTRL_NTW_ADR3_SZ_SHIFT    (14U)
#define GENFSK_NTW_ADR_CTRL_NTW_ADR3_SZ(x)       (((uint32_t)(((uint32_t)(x)) << GENFSK_NTW_ADR_CTRL_NTW_ADR3_SZ_SHIFT)) & GENFSK_NTW_ADR_CTRL_NTW_ADR3_SZ_MASK)
#define GENFSK_NTW_ADR_CTRL_NTW_ADR_THR0_MASK    (0x70000U)
#define GENFSK_NTW_ADR_CTRL_NTW_ADR_THR0_SHIFT   (16U)
#define GENFSK_NTW_ADR_CTRL_NTW_ADR_THR0(x)      (((uint32_t)(((uint32_t)(x)) << GENFSK_NTW_ADR_CTRL_NTW_ADR_THR0_SHIFT)) & GENFSK_NTW_ADR_CTRL_NTW_ADR_THR0_MASK)
#define GENFSK_NTW_ADR_CTRL_NTW_ADR_THR1_MASK    (0x700000U)
#define GENFSK_NTW_ADR_CTRL_NTW_ADR_THR1_SHIFT   (20U)
#define GENFSK_NTW_ADR_CTRL_NTW_ADR_THR1(x)      (((uint32_t)(((uint32_t)(x)) << GENFSK_NTW_ADR_CTRL_NTW_ADR_THR1_SHIFT)) & GENFSK_NTW_ADR_CTRL_NTW_ADR_THR1_MASK)
#define GENFSK_NTW_ADR_CTRL_NTW_ADR_THR2_MASK    (0x7000000U)
#define GENFSK_NTW_ADR_CTRL_NTW_ADR_THR2_SHIFT   (24U)
#define GENFSK_NTW_ADR_CTRL_NTW_ADR_THR2(x)      (((uint32_t)(((uint32_t)(x)) << GENFSK_NTW_ADR_CTRL_NTW_ADR_THR2_SHIFT)) & GENFSK_NTW_ADR_CTRL_NTW_ADR_THR2_MASK)
#define GENFSK_NTW_ADR_CTRL_NTW_ADR_THR3_MASK    (0x70000000U)
#define GENFSK_NTW_ADR_CTRL_NTW_ADR_THR3_SHIFT   (28U)
#define GENFSK_NTW_ADR_CTRL_NTW_ADR_THR3(x)      (((uint32_t)(((uint32_t)(x)) << GENFSK_NTW_ADR_CTRL_NTW_ADR_THR3_SHIFT)) & GENFSK_NTW_ADR_CTRL_NTW_ADR_THR3_MASK)

/*! @name NTW_ADR_0 - NETWORK ADDRESS 0 */
#define GENFSK_NTW_ADR_0_NTW_ADR_0_MASK          (0xFFFFFFFFU)
#define GENFSK_NTW_ADR_0_NTW_ADR_0_SHIFT         (0U)
#define GENFSK_NTW_ADR_0_NTW_ADR_0(x)            (((uint32_t)(((uint32_t)(x)) << GENFSK_NTW_ADR_0_NTW_ADR_0_SHIFT)) & GENFSK_NTW_ADR_0_NTW_ADR_0_MASK)

/*! @name NTW_ADR_1 - NETWORK ADDRESS 1 */
#define GENFSK_NTW_ADR_1_NTW_ADR_1_MASK          (0xFFFFFFFFU)
#define GENFSK_NTW_ADR_1_NTW_ADR_1_SHIFT         (0U)
#define GENFSK_NTW_ADR_1_NTW_ADR_1(x)            (((uint32_t)(((uint32_t)(x)) << GENFSK_NTW_ADR_1_NTW_ADR_1_SHIFT)) & GENFSK_NTW_ADR_1_NTW_ADR_1_MASK)

/*! @name NTW_ADR_2 - NETWORK ADDRESS 2 */
#define GENFSK_NTW_ADR_2_NTW_ADR_2_MASK          (0xFFFFFFFFU)
#define GENFSK_NTW_ADR_2_NTW_ADR_2_SHIFT         (0U)
#define GENFSK_NTW_ADR_2_NTW_ADR_2(x)            (((uint32_t)(((uint32_t)(x)) << GENFSK_NTW_ADR_2_NTW_ADR_2_SHIFT)) & GENFSK_NTW_ADR_2_NTW_ADR_2_MASK)

/*! @name NTW_ADR_3 - NETWORK ADDRESS 3 */
#define GENFSK_NTW_ADR_3_NTW_ADR_3_MASK          (0xFFFFFFFFU)
#define GENFSK_NTW_ADR_3_NTW_ADR_3_SHIFT         (0U)
#define GENFSK_NTW_ADR_3_NTW_ADR_3(x)            (((uint32_t)(((uint32_t)(x)) << GENFSK_NTW_ADR_3_NTW_ADR_3_SHIFT)) & GENFSK_NTW_ADR_3_NTW_ADR_3_MASK)

/*! @name RX_WATERMARK - RECEIVE WATERMARK */
#define GENFSK_RX_WATERMARK_RX_WATERMARK_MASK    (0x1FFFU)
#define GENFSK_RX_WATERMARK_RX_WATERMARK_SHIFT   (0U)
#define GENFSK_RX_WATERMARK_RX_WATERMARK(x)      (((uint32_t)(((uint32_t)(x)) << GENFSK_RX_WATERMARK_RX_WATERMARK_SHIFT)) & GENFSK_RX_WATERMARK_RX_WATERMARK_MASK)
#define GENFSK_RX_WATERMARK_BYTE_COUNTER_MASK    (0x1FFF0000U)
#define GENFSK_RX_WATERMARK_BYTE_COUNTER_SHIFT   (16U)
#define GENFSK_RX_WATERMARK_BYTE_COUNTER(x)      (((uint32_t)(((uint32_t)(x)) << GENFSK_RX_WATERMARK_BYTE_COUNTER_SHIFT)) & GENFSK_RX_WATERMARK_BYTE_COUNTER_MASK)

/*! @name DSM_CTRL - DSM CONTROL */
#define GENFSK_DSM_CTRL_GENERIC_FSK_SLEEP_EN_MASK (0x1U)
#define GENFSK_DSM_CTRL_GENERIC_FSK_SLEEP_EN_SHIFT (0U)
#define GENFSK_DSM_CTRL_GENERIC_FSK_SLEEP_EN(x)  (((uint32_t)(((uint32_t)(x)) << GENFSK_DSM_CTRL_GENERIC_FSK_SLEEP_EN_SHIFT)) & GENFSK_DSM_CTRL_GENERIC_FSK_SLEEP_EN_MASK)

/*! @name PART_ID - PART ID */
#define GENFSK_PART_ID_PART_ID_MASK              (0xFFU)
#define GENFSK_PART_ID_PART_ID_SHIFT             (0U)
#define GENFSK_PART_ID_PART_ID(x)                (((uint32_t)(((uint32_t)(x)) << GENFSK_PART_ID_PART_ID_SHIFT)) & GENFSK_PART_ID_PART_ID_MASK)

/*! @name PACKET_CFG - PACKET CONFIGURATION */
#define GENFSK_PACKET_CFG_LENGTH_SZ_MASK         (0x1FU)
#define GENFSK_PACKET_CFG_LENGTH_SZ_SHIFT        (0U)
#define GENFSK_PACKET_CFG_LENGTH_SZ(x)           (((uint32_t)(((uint32_t)(x)) << GENFSK_PACKET_CFG_LENGTH_SZ_SHIFT)) & GENFSK_PACKET_CFG_LENGTH_SZ_MASK)
#define GENFSK_PACKET_CFG_LENGTH_BIT_ORD_MASK    (0x20U)
#define GENFSK_PACKET_CFG_LENGTH_BIT_ORD_SHIFT   (5U)
#define GENFSK_PACKET_CFG_LENGTH_BIT_ORD(x)      (((uint32_t)(((uint32_t)(x)) << GENFSK_PACKET_CFG_LENGTH_BIT_ORD_SHIFT)) & GENFSK_PACKET_CFG_LENGTH_BIT_ORD_MASK)
#define GENFSK_PACKET_CFG_SYNC_ADDR_SZ_MASK      (0xC0U)
#define GENFSK_PACKET_CFG_SYNC_ADDR_SZ_SHIFT     (6U)
#define GENFSK_PACKET_CFG_SYNC_ADDR_SZ(x)        (((uint32_t)(((uint32_t)(x)) << GENFSK_PACKET_CFG_SYNC_ADDR_SZ_SHIFT)) & GENFSK_PACKET_CFG_SYNC_ADDR_SZ_MASK)
#define GENFSK_PACKET_CFG_LENGTH_ADJ_MASK        (0x3F00U)
#define GENFSK_PACKET_CFG_LENGTH_ADJ_SHIFT       (8U)
#define GENFSK_PACKET_CFG_LENGTH_ADJ(x)          (((uint32_t)(((uint32_t)(x)) << GENFSK_PACKET_CFG_LENGTH_ADJ_SHIFT)) & GENFSK_PACKET_CFG_LENGTH_ADJ_MASK)
#define GENFSK_PACKET_CFG_LENGTH_FAIL_MASK       (0x8000U)
#define GENFSK_PACKET_CFG_LENGTH_FAIL_SHIFT      (15U)
#define GENFSK_PACKET_CFG_LENGTH_FAIL(x)         (((uint32_t)(((uint32_t)(x)) << GENFSK_PACKET_CFG_LENGTH_FAIL_SHIFT)) & GENFSK_PACKET_CFG_LENGTH_FAIL_MASK)
#define GENFSK_PACKET_CFG_H0_SZ_MASK             (0x1F0000U)
#define GENFSK_PACKET_CFG_H0_SZ_SHIFT            (16U)
#define GENFSK_PACKET_CFG_H0_SZ(x)               (((uint32_t)(((uint32_t)(x)) << GENFSK_PACKET_CFG_H0_SZ_SHIFT)) & GENFSK_PACKET_CFG_H0_SZ_MASK)
#define GENFSK_PACKET_CFG_H0_FAIL_MASK           (0x800000U)
#define GENFSK_PACKET_CFG_H0_FAIL_SHIFT          (23U)
#define GENFSK_PACKET_CFG_H0_FAIL(x)             (((uint32_t)(((uint32_t)(x)) << GENFSK_PACKET_CFG_H0_FAIL_SHIFT)) & GENFSK_PACKET_CFG_H0_FAIL_MASK)
#define GENFSK_PACKET_CFG_H1_SZ_MASK             (0x1F000000U)
#define GENFSK_PACKET_CFG_H1_SZ_SHIFT            (24U)
#define GENFSK_PACKET_CFG_H1_SZ(x)               (((uint32_t)(((uint32_t)(x)) << GENFSK_PACKET_CFG_H1_SZ_SHIFT)) & GENFSK_PACKET_CFG_H1_SZ_MASK)
#define GENFSK_PACKET_CFG_H1_FAIL_MASK           (0x80000000U)
#define GENFSK_PACKET_CFG_H1_FAIL_SHIFT          (31U)
#define GENFSK_PACKET_CFG_H1_FAIL(x)             (((uint32_t)(((uint32_t)(x)) << GENFSK_PACKET_CFG_H1_FAIL_SHIFT)) & GENFSK_PACKET_CFG_H1_FAIL_MASK)

/*! @name H0_CFG - H0 CONFIGURATION */
#define GENFSK_H0_CFG_H0_MATCH_MASK              (0xFFFFU)
#define GENFSK_H0_CFG_H0_MATCH_SHIFT             (0U)
#define GENFSK_H0_CFG_H0_MATCH(x)                (((uint32_t)(((uint32_t)(x)) << GENFSK_H0_CFG_H0_MATCH_SHIFT)) & GENFSK_H0_CFG_H0_MATCH_MASK)
#define GENFSK_H0_CFG_H0_MASK_MASK               (0xFFFF0000U)
#define GENFSK_H0_CFG_H0_MASK_SHIFT              (16U)
#define GENFSK_H0_CFG_H0_MASK(x)                 (((uint32_t)(((uint32_t)(x)) << GENFSK_H0_CFG_H0_MASK_SHIFT)) & GENFSK_H0_CFG_H0_MASK_MASK)

/*! @name H1_CFG - H1 CONFIGURATION */
#define GENFSK_H1_CFG_H1_MATCH_MASK              (0xFFFFU)
#define GENFSK_H1_CFG_H1_MATCH_SHIFT             (0U)
#define GENFSK_H1_CFG_H1_MATCH(x)                (((uint32_t)(((uint32_t)(x)) << GENFSK_H1_CFG_H1_MATCH_SHIFT)) & GENFSK_H1_CFG_H1_MATCH_MASK)
#define GENFSK_H1_CFG_H1_MASK_MASK               (0xFFFF0000U)
#define GENFSK_H1_CFG_H1_MASK_SHIFT              (16U)
#define GENFSK_H1_CFG_H1_MASK(x)                 (((uint32_t)(((uint32_t)(x)) << GENFSK_H1_CFG_H1_MASK_SHIFT)) & GENFSK_H1_CFG_H1_MASK_MASK)

/*! @name CRC_CFG - CRC CONFIGURATION */
#define GENFSK_CRC_CFG_CRC_SZ_MASK               (0x7U)
#define GENFSK_CRC_CFG_CRC_SZ_SHIFT              (0U)
#define GENFSK_CRC_CFG_CRC_SZ(x)                 (((uint32_t)(((uint32_t)(x)) << GENFSK_CRC_CFG_CRC_SZ_SHIFT)) & GENFSK_CRC_CFG_CRC_SZ_MASK)
#define GENFSK_CRC_CFG_CRC_START_BYTE_MASK       (0xF00U)
#define GENFSK_CRC_CFG_CRC_START_BYTE_SHIFT      (8U)
#define GENFSK_CRC_CFG_CRC_START_BYTE(x)         (((uint32_t)(((uint32_t)(x)) << GENFSK_CRC_CFG_CRC_START_BYTE_SHIFT)) & GENFSK_CRC_CFG_CRC_START_BYTE_MASK)
#define GENFSK_CRC_CFG_CRC_REF_IN_MASK           (0x10000U)
#define GENFSK_CRC_CFG_CRC_REF_IN_SHIFT          (16U)
#define GENFSK_CRC_CFG_CRC_REF_IN(x)             (((uint32_t)(((uint32_t)(x)) << GENFSK_CRC_CFG_CRC_REF_IN_SHIFT)) & GENFSK_CRC_CFG_CRC_REF_IN_MASK)
#define GENFSK_CRC_CFG_CRC_REF_OUT_MASK          (0x20000U)
#define GENFSK_CRC_CFG_CRC_REF_OUT_SHIFT         (17U)
#define GENFSK_CRC_CFG_CRC_REF_OUT(x)            (((uint32_t)(((uint32_t)(x)) << GENFSK_CRC_CFG_CRC_REF_OUT_SHIFT)) & GENFSK_CRC_CFG_CRC_REF_OUT_MASK)
#define GENFSK_CRC_CFG_CRC_BYTE_ORD_MASK         (0x40000U)
#define GENFSK_CRC_CFG_CRC_BYTE_ORD_SHIFT        (18U)
#define GENFSK_CRC_CFG_CRC_BYTE_ORD(x)           (((uint32_t)(((uint32_t)(x)) << GENFSK_CRC_CFG_CRC_BYTE_ORD_SHIFT)) & GENFSK_CRC_CFG_CRC_BYTE_ORD_MASK)

/*! @name CRC_INIT - CRC INITIALIZATION */
#define GENFSK_CRC_INIT_CRC_SEED_MASK            (0xFFFFFFFFU)
#define GENFSK_CRC_INIT_CRC_SEED_SHIFT           (0U)
#define GENFSK_CRC_INIT_CRC_SEED(x)              (((uint32_t)(((uint32_t)(x)) << GENFSK_CRC_INIT_CRC_SEED_SHIFT)) & GENFSK_CRC_INIT_CRC_SEED_MASK)

/*! @name CRC_POLY - CRC POLYNOMIAL */
#define GENFSK_CRC_POLY_CRC_POLY_MASK            (0xFFFFFFFFU)
#define GENFSK_CRC_POLY_CRC_POLY_SHIFT           (0U)
#define GENFSK_CRC_POLY_CRC_POLY(x)              (((uint32_t)(((uint32_t)(x)) << GENFSK_CRC_POLY_CRC_POLY_SHIFT)) & GENFSK_CRC_POLY_CRC_POLY_MASK)

/*! @name CRC_XOR_OUT - CRC XOR OUT */
#define GENFSK_CRC_XOR_OUT_CRC_XOR_OUT_MASK      (0xFFFFFFFFU)
#define GENFSK_CRC_XOR_OUT_CRC_XOR_OUT_SHIFT     (0U)
#define GENFSK_CRC_XOR_OUT_CRC_XOR_OUT(x)        (((uint32_t)(((uint32_t)(x)) << GENFSK_CRC_XOR_OUT_CRC_XOR_OUT_SHIFT)) & GENFSK_CRC_XOR_OUT_CRC_XOR_OUT_MASK)

/*! @name WHITEN_CFG - WHITENER CONFIGURATION */
#define GENFSK_WHITEN_CFG_WHITEN_START_MASK      (0x3U)
#define GENFSK_WHITEN_CFG_WHITEN_START_SHIFT     (0U)
#define GENFSK_WHITEN_CFG_WHITEN_START(x)        (((uint32_t)(((uint32_t)(x)) << GENFSK_WHITEN_CFG_WHITEN_START_SHIFT)) & GENFSK_WHITEN_CFG_WHITEN_START_MASK)
#define GENFSK_WHITEN_CFG_WHITEN_END_MASK        (0x4U)
#define GENFSK_WHITEN_CFG_WHITEN_END_SHIFT       (2U)
#define GENFSK_WHITEN_CFG_WHITEN_END(x)          (((uint32_t)(((uint32_t)(x)) << GENFSK_WHITEN_CFG_WHITEN_END_SHIFT)) & GENFSK_WHITEN_CFG_WHITEN_END_MASK)
#define GENFSK_WHITEN_CFG_WHITEN_B4_CRC_MASK     (0x8U)
#define GENFSK_WHITEN_CFG_WHITEN_B4_CRC_SHIFT    (3U)
#define GENFSK_WHITEN_CFG_WHITEN_B4_CRC(x)       (((uint32_t)(((uint32_t)(x)) << GENFSK_WHITEN_CFG_WHITEN_B4_CRC_SHIFT)) & GENFSK_WHITEN_CFG_WHITEN_B4_CRC_MASK)
#define GENFSK_WHITEN_CFG_WHITEN_POLY_TYPE_MASK  (0x10U)
#define GENFSK_WHITEN_CFG_WHITEN_POLY_TYPE_SHIFT (4U)
#define GENFSK_WHITEN_CFG_WHITEN_POLY_TYPE(x)    (((uint32_t)(((uint32_t)(x)) << GENFSK_WHITEN_CFG_WHITEN_POLY_TYPE_SHIFT)) & GENFSK_WHITEN_CFG_WHITEN_POLY_TYPE_MASK)
#define GENFSK_WHITEN_CFG_WHITEN_REF_IN_MASK     (0x20U)
#define GENFSK_WHITEN_CFG_WHITEN_REF_IN_SHIFT    (5U)
#define GENFSK_WHITEN_CFG_WHITEN_REF_IN(x)       (((uint32_t)(((uint32_t)(x)) << GENFSK_WHITEN_CFG_WHITEN_REF_IN_SHIFT)) & GENFSK_WHITEN_CFG_WHITEN_REF_IN_MASK)
#define GENFSK_WHITEN_CFG_WHITEN_PAYLOAD_REINIT_MASK (0x40U)
#define GENFSK_WHITEN_CFG_WHITEN_PAYLOAD_REINIT_SHIFT (6U)
#define GENFSK_WHITEN_CFG_WHITEN_PAYLOAD_REINIT(x) (((uint32_t)(((uint32_t)(x)) << GENFSK_WHITEN_CFG_WHITEN_PAYLOAD_REINIT_SHIFT)) & GENFSK_WHITEN_CFG_WHITEN_PAYLOAD_REINIT_MASK)
#define GENFSK_WHITEN_CFG_WHITEN_SIZE_MASK       (0xF00U)
#define GENFSK_WHITEN_CFG_WHITEN_SIZE_SHIFT      (8U)
#define GENFSK_WHITEN_CFG_WHITEN_SIZE(x)         (((uint32_t)(((uint32_t)(x)) << GENFSK_WHITEN_CFG_WHITEN_SIZE_SHIFT)) & GENFSK_WHITEN_CFG_WHITEN_SIZE_MASK)
#define GENFSK_WHITEN_CFG_MANCHESTER_EN_MASK     (0x1000U)
#define GENFSK_WHITEN_CFG_MANCHESTER_EN_SHIFT    (12U)
#define GENFSK_WHITEN_CFG_MANCHESTER_EN(x)       (((uint32_t)(((uint32_t)(x)) << GENFSK_WHITEN_CFG_MANCHESTER_EN_SHIFT)) & GENFSK_WHITEN_CFG_MANCHESTER_EN_MASK)
#define GENFSK_WHITEN_CFG_MANCHESTER_INV_MASK    (0x2000U)
#define GENFSK_WHITEN_CFG_MANCHESTER_INV_SHIFT   (13U)
#define GENFSK_WHITEN_CFG_MANCHESTER_INV(x)      (((uint32_t)(((uint32_t)(x)) << GENFSK_WHITEN_CFG_MANCHESTER_INV_SHIFT)) & GENFSK_WHITEN_CFG_MANCHESTER_INV_MASK)
#define GENFSK_WHITEN_CFG_MANCHESTER_START_MASK  (0x4000U)
#define GENFSK_WHITEN_CFG_MANCHESTER_START_SHIFT (14U)
#define GENFSK_WHITEN_CFG_MANCHESTER_START(x)    (((uint32_t)(((uint32_t)(x)) << GENFSK_WHITEN_CFG_MANCHESTER_START_SHIFT)) & GENFSK_WHITEN_CFG_MANCHESTER_START_MASK)
#define GENFSK_WHITEN_CFG_WHITEN_INIT_MASK       (0x1FF0000U)
#define GENFSK_WHITEN_CFG_WHITEN_INIT_SHIFT      (16U)
#define GENFSK_WHITEN_CFG_WHITEN_INIT(x)         (((uint32_t)(((uint32_t)(x)) << GENFSK_WHITEN_CFG_WHITEN_INIT_SHIFT)) & GENFSK_WHITEN_CFG_WHITEN_INIT_MASK)

/*! @name WHITEN_POLY - WHITENER POLYNOMIAL */
#define GENFSK_WHITEN_POLY_WHITEN_POLY_MASK      (0x1FFU)
#define GENFSK_WHITEN_POLY_WHITEN_POLY_SHIFT     (0U)
#define GENFSK_WHITEN_POLY_WHITEN_POLY(x)        (((uint32_t)(((uint32_t)(x)) << GENFSK_WHITEN_POLY_WHITEN_POLY_SHIFT)) & GENFSK_WHITEN_POLY_WHITEN_POLY_MASK)

/*! @name WHITEN_SZ_THR - WHITENER SIZE THRESHOLD */
#define GENFSK_WHITEN_SZ_THR_WHITEN_SZ_THR_MASK  (0xFFFU)
#define GENFSK_WHITEN_SZ_THR_WHITEN_SZ_THR_SHIFT (0U)
#define GENFSK_WHITEN_SZ_THR_WHITEN_SZ_THR(x)    (((uint32_t)(((uint32_t)(x)) << GENFSK_WHITEN_SZ_THR_WHITEN_SZ_THR_SHIFT)) & GENFSK_WHITEN_SZ_THR_WHITEN_SZ_THR_MASK)
#define GENFSK_WHITEN_SZ_THR_LENGTH_MAX_MASK     (0x7F0000U)
#define GENFSK_WHITEN_SZ_THR_LENGTH_MAX_SHIFT    (16U)
#define GENFSK_WHITEN_SZ_THR_LENGTH_MAX(x)       (((uint32_t)(((uint32_t)(x)) << GENFSK_WHITEN_SZ_THR_LENGTH_MAX_SHIFT)) & GENFSK_WHITEN_SZ_THR_LENGTH_MAX_MASK)
#define GENFSK_WHITEN_SZ_THR_REC_BAD_PKT_MASK    (0x800000U)
#define GENFSK_WHITEN_SZ_THR_REC_BAD_PKT_SHIFT   (23U)
#define GENFSK_WHITEN_SZ_THR_REC_BAD_PKT(x)      (((uint32_t)(((uint32_t)(x)) << GENFSK_WHITEN_SZ_THR_REC_BAD_PKT_SHIFT)) & GENFSK_WHITEN_SZ_THR_REC_BAD_PKT_MASK)

/*! @name BITRATE - BIT RATE */
#define GENFSK_BITRATE_BITRATE_MASK              (0x3U)
#define GENFSK_BITRATE_BITRATE_SHIFT             (0U)
#define GENFSK_BITRATE_BITRATE(x)                (((uint32_t)(((uint32_t)(x)) << GENFSK_BITRATE_BITRATE_SHIFT)) & GENFSK_BITRATE_BITRATE_MASK)

/*! @name PB_PARTITION - PACKET BUFFER PARTITION POINT */
#define GENFSK_PB_PARTITION_PB_PARTITION_MASK    (0x7FFU)
#define GENFSK_PB_PARTITION_PB_PARTITION_SHIFT   (0U)
#define GENFSK_PB_PARTITION_PB_PARTITION(x)      (((uint32_t)(((uint32_t)(x)) << GENFSK_PB_PARTITION_PB_PARTITION_SHIFT)) & GENFSK_PB_PARTITION_PB_PARTITION_MASK)


/*!
 * @}
 */ /* end of group GENFSK_Register_Masks */


/* GENFSK - Peripheral instance base addresses */
/** Peripheral GENFSK base address */
#define GENFSK_BASE                              (0x4005F000u)
/** Peripheral GENFSK base pointer */
#define GENFSK                                   ((GENFSK_Type *)GENFSK_BASE)
/** Array initializer of GENFSK peripheral base addresses */
#define GENFSK_BASE_ADDRS                        { GENFSK_BASE }
/** Array initializer of GENFSK peripheral base pointers */
#define GENFSK_BASE_PTRS                         { GENFSK }

/*!
 * @}
 */ /* end of group GENFSK_Peripheral_Access_Layer */


/* ----------------------------------------------------------------------------
   -- GPIO Peripheral Access Layer
   ---------------------------------------------------------------------------- */

/*!
 * @addtogroup GPIO_Peripheral_Access_Layer GPIO Peripheral Access Layer
 * @{
 */

/** GPIO - Register Layout Typedef */
typedef struct {
  __IO uint32_t PDOR;                              /**< Port Data Output Register, offset: 0x0 */
  __O  uint32_t PSOR;                              /**< Port Set Output Register, offset: 0x4 */
  __O  uint32_t PCOR;                              /**< Port Clear Output Register, offset: 0x8 */
  __O  uint32_t PTOR;                              /**< Port Toggle Output Register, offset: 0xC */
  __I  uint32_t PDIR;                              /**< Port Data Input Register, offset: 0x10 */
  __IO uint32_t PDDR;                              /**< Port Data Direction Register, offset: 0x14 */
} GPIO_Type;

/* ----------------------------------------------------------------------------
   -- GPIO Register Masks
   ---------------------------------------------------------------------------- */

/*!
 * @addtogroup GPIO_Register_Masks GPIO Register Masks
 * @{
 */

/*! @name PDOR - Port Data Output Register */
#define GPIO_PDOR_PDO_MASK                       (0xFFFFFFFFU)
#define GPIO_PDOR_PDO_SHIFT                      (0U)
#define GPIO_PDOR_PDO(x)                         (((uint32_t)(((uint32_t)(x)) << GPIO_PDOR_PDO_SHIFT)) & GPIO_PDOR_PDO_MASK)

/*! @name PSOR - Port Set Output Register */
#define GPIO_PSOR_PTSO_MASK                      (0xFFFFFFFFU)
#define GPIO_PSOR_PTSO_SHIFT                     (0U)
#define GPIO_PSOR_PTSO(x)                        (((uint32_t)(((uint32_t)(x)) << GPIO_PSOR_PTSO_SHIFT)) & GPIO_PSOR_PTSO_MASK)

/*! @name PCOR - Port Clear Output Register */
#define GPIO_PCOR_PTCO_MASK                      (0xFFFFFFFFU)
#define GPIO_PCOR_PTCO_SHIFT                     (0U)
#define GPIO_PCOR_PTCO(x)                        (((uint32_t)(((uint32_t)(x)) << GPIO_PCOR_PTCO_SHIFT)) & GPIO_PCOR_PTCO_MASK)

/*! @name PTOR - Port Toggle Output Register */
#define GPIO_PTOR_PTTO_MASK                      (0xFFFFFFFFU)
#define GPIO_PTOR_PTTO_SHIFT                     (0U)
#define GPIO_PTOR_PTTO(x)                        (((uint32_t)(((uint32_t)(x)) << GPIO_PTOR_PTTO_SHIFT)) & GPIO_PTOR_PTTO_MASK)

/*! @name PDIR - Port Data Input Register */
#define GPIO_PDIR_PDI_MASK                       (0xFFFFFFFFU)
#define GPIO_PDIR_PDI_SHIFT                      (0U)
#define GPIO_PDIR_PDI(x)                         (((uint32_t)(((uint32_t)(x)) << GPIO_PDIR_PDI_SHIFT)) & GPIO_PDIR_PDI_MASK)

/*! @name PDDR - Port Data Direction Register */
#define GPIO_PDDR_PDD_MASK                       (0xFFFFFFFFU)
#define GPIO_PDDR_PDD_SHIFT                      (0U)
#define GPIO_PDDR_PDD(x)                         (((uint32_t)(((uint32_t)(x)) << GPIO_PDDR_PDD_SHIFT)) & GPIO_PDDR_PDD_MASK)


/*!
 * @}
 */ /* end of group GPIO_Register_Masks */


/* GPIO - Peripheral instance base addresses */
/** Peripheral GPIOA base address */
#define GPIOA_BASE                               (0x400FF000u)
/** Peripheral GPIOA base pointer */
#define GPIOA                                    ((GPIO_Type *)GPIOA_BASE)
/** Peripheral GPIOB base address */
#define GPIOB_BASE                               (0x400FF040u)
/** Peripheral GPIOB base pointer */
#define GPIOB                                    ((GPIO_Type *)GPIOB_BASE)
/** Peripheral GPIOC base address */
#define GPIOC_BASE                               (0x400FF080u)
/** Peripheral GPIOC base pointer */
#define GPIOC                                    ((GPIO_Type *)GPIOC_BASE)
/** Array initializer of GPIO peripheral base addresses */
#define GPIO_BASE_ADDRS                          { GPIOA_BASE, GPIOB_BASE, GPIOC_BASE }
/** Array initializer of GPIO peripheral base pointers */
#define GPIO_BASE_PTRS                           { GPIOA, GPIOB, GPIOC }

/*!
 * @}
 */ /* end of group GPIO_Peripheral_Access_Layer */


/* ----------------------------------------------------------------------------
   -- I2C Peripheral Access Layer
   ---------------------------------------------------------------------------- */

/*!
 * @addtogroup I2C_Peripheral_Access_Layer I2C Peripheral Access Layer
 * @{
 */

/** I2C - Register Layout Typedef */
typedef struct {
  __IO uint8_t A1;                                 /**< I2C Address Register 1, offset: 0x0 */
  __IO uint8_t F;                                  /**< I2C Frequency Divider register, offset: 0x1 */
  __IO uint8_t C1;                                 /**< I2C Control Register 1, offset: 0x2 */
  __IO uint8_t S;                                  /**< I2C Status register, offset: 0x3 */
  __IO uint8_t D;                                  /**< I2C Data I/O register, offset: 0x4 */
  __IO uint8_t C2;                                 /**< I2C Control Register 2, offset: 0x5 */
  __IO uint8_t FLT;                                /**< I2C Programmable Input Glitch Filter Register, offset: 0x6 */
  __IO uint8_t RA;                                 /**< I2C Range Address register, offset: 0x7 */
  __IO uint8_t SMB;                                /**< I2C SMBus Control and Status register, offset: 0x8 */
  __IO uint8_t A2;                                 /**< I2C Address Register 2, offset: 0x9 */
  __IO uint8_t SLTH;                               /**< I2C SCL Low Timeout Register High, offset: 0xA */
  __IO uint8_t SLTL;                               /**< I2C SCL Low Timeout Register Low, offset: 0xB */
  __IO uint8_t S2;                                 /**< I2C Status register 2, offset: 0xC */
} I2C_Type;

/* ----------------------------------------------------------------------------
   -- I2C Register Masks
   ---------------------------------------------------------------------------- */

/*!
 * @addtogroup I2C_Register_Masks I2C Register Masks
 * @{
 */

/*! @name A1 - I2C Address Register 1 */
#define I2C_A1_AD_MASK                           (0xFEU)
#define I2C_A1_AD_SHIFT                          (1U)
#define I2C_A1_AD(x)                             (((uint8_t)(((uint8_t)(x)) << I2C_A1_AD_SHIFT)) & I2C_A1_AD_MASK)

/*! @name F - I2C Frequency Divider register */
#define I2C_F_ICR_MASK                           (0x3FU)
#define I2C_F_ICR_SHIFT                          (0U)
#define I2C_F_ICR(x)                             (((uint8_t)(((uint8_t)(x)) << I2C_F_ICR_SHIFT)) & I2C_F_ICR_MASK)
#define I2C_F_MULT_MASK                          (0xC0U)
#define I2C_F_MULT_SHIFT                         (6U)
#define I2C_F_MULT(x)                            (((uint8_t)(((uint8_t)(x)) << I2C_F_MULT_SHIFT)) & I2C_F_MULT_MASK)

/*! @name C1 - I2C Control Register 1 */
#define I2C_C1_DMAEN_MASK                        (0x1U)
#define I2C_C1_DMAEN_SHIFT                       (0U)
#define I2C_C1_DMAEN(x)                          (((uint8_t)(((uint8_t)(x)) << I2C_C1_DMAEN_SHIFT)) & I2C_C1_DMAEN_MASK)
#define I2C_C1_WUEN_MASK                         (0x2U)
#define I2C_C1_WUEN_SHIFT                        (1U)
#define I2C_C1_WUEN(x)                           (((uint8_t)(((uint8_t)(x)) << I2C_C1_WUEN_SHIFT)) & I2C_C1_WUEN_MASK)
#define I2C_C1_RSTA_MASK                         (0x4U)
#define I2C_C1_RSTA_SHIFT                        (2U)
#define I2C_C1_RSTA(x)                           (((uint8_t)(((uint8_t)(x)) << I2C_C1_RSTA_SHIFT)) & I2C_C1_RSTA_MASK)
#define I2C_C1_TXAK_MASK                         (0x8U)
#define I2C_C1_TXAK_SHIFT                        (3U)
#define I2C_C1_TXAK(x)                           (((uint8_t)(((uint8_t)(x)) << I2C_C1_TXAK_SHIFT)) & I2C_C1_TXAK_MASK)
#define I2C_C1_TX_MASK                           (0x10U)
#define I2C_C1_TX_SHIFT                          (4U)
#define I2C_C1_TX(x)                             (((uint8_t)(((uint8_t)(x)) << I2C_C1_TX_SHIFT)) & I2C_C1_TX_MASK)
#define I2C_C1_MST_MASK                          (0x20U)
#define I2C_C1_MST_SHIFT                         (5U)
#define I2C_C1_MST(x)                            (((uint8_t)(((uint8_t)(x)) << I2C_C1_MST_SHIFT)) & I2C_C1_MST_MASK)
#define I2C_C1_IICIE_MASK                        (0x40U)
#define I2C_C1_IICIE_SHIFT                       (6U)
#define I2C_C1_IICIE(x)                          (((uint8_t)(((uint8_t)(x)) << I2C_C1_IICIE_SHIFT)) & I2C_C1_IICIE_MASK)
#define I2C_C1_IICEN_MASK                        (0x80U)
#define I2C_C1_IICEN_SHIFT                       (7U)
#define I2C_C1_IICEN(x)                          (((uint8_t)(((uint8_t)(x)) << I2C_C1_IICEN_SHIFT)) & I2C_C1_IICEN_MASK)

/*! @name S - I2C Status register */
#define I2C_S_RXAK_MASK                          (0x1U)
#define I2C_S_RXAK_SHIFT                         (0U)
#define I2C_S_RXAK(x)                            (((uint8_t)(((uint8_t)(x)) << I2C_S_RXAK_SHIFT)) & I2C_S_RXAK_MASK)
#define I2C_S_IICIF_MASK                         (0x2U)
#define I2C_S_IICIF_SHIFT                        (1U)
#define I2C_S_IICIF(x)                           (((uint8_t)(((uint8_t)(x)) << I2C_S_IICIF_SHIFT)) & I2C_S_IICIF_MASK)
#define I2C_S_SRW_MASK                           (0x4U)
#define I2C_S_SRW_SHIFT                          (2U)
#define I2C_S_SRW(x)                             (((uint8_t)(((uint8_t)(x)) << I2C_S_SRW_SHIFT)) & I2C_S_SRW_MASK)
#define I2C_S_RAM_MASK                           (0x8U)
#define I2C_S_RAM_SHIFT                          (3U)
#define I2C_S_RAM(x)                             (((uint8_t)(((uint8_t)(x)) << I2C_S_RAM_SHIFT)) & I2C_S_RAM_MASK)
#define I2C_S_ARBL_MASK                          (0x10U)
#define I2C_S_ARBL_SHIFT                         (4U)
#define I2C_S_ARBL(x)                            (((uint8_t)(((uint8_t)(x)) << I2C_S_ARBL_SHIFT)) & I2C_S_ARBL_MASK)
#define I2C_S_BUSY_MASK                          (0x20U)
#define I2C_S_BUSY_SHIFT                         (5U)
#define I2C_S_BUSY(x)                            (((uint8_t)(((uint8_t)(x)) << I2C_S_BUSY_SHIFT)) & I2C_S_BUSY_MASK)
#define I2C_S_IAAS_MASK                          (0x40U)
#define I2C_S_IAAS_SHIFT                         (6U)
#define I2C_S_IAAS(x)                            (((uint8_t)(((uint8_t)(x)) << I2C_S_IAAS_SHIFT)) & I2C_S_IAAS_MASK)
#define I2C_S_TCF_MASK                           (0x80U)
#define I2C_S_TCF_SHIFT                          (7U)
#define I2C_S_TCF(x)                             (((uint8_t)(((uint8_t)(x)) << I2C_S_TCF_SHIFT)) & I2C_S_TCF_MASK)

/*! @name D - I2C Data I/O register */
#define I2C_D_DATA_MASK                          (0xFFU)
#define I2C_D_DATA_SHIFT                         (0U)
#define I2C_D_DATA(x)                            (((uint8_t)(((uint8_t)(x)) << I2C_D_DATA_SHIFT)) & I2C_D_DATA_MASK)

/*! @name C2 - I2C Control Register 2 */
#define I2C_C2_AD_MASK                           (0x7U)
#define I2C_C2_AD_SHIFT                          (0U)
#define I2C_C2_AD(x)                             (((uint8_t)(((uint8_t)(x)) << I2C_C2_AD_SHIFT)) & I2C_C2_AD_MASK)
#define I2C_C2_RMEN_MASK                         (0x8U)
#define I2C_C2_RMEN_SHIFT                        (3U)
#define I2C_C2_RMEN(x)                           (((uint8_t)(((uint8_t)(x)) << I2C_C2_RMEN_SHIFT)) & I2C_C2_RMEN_MASK)
#define I2C_C2_SBRC_MASK                         (0x10U)
#define I2C_C2_SBRC_SHIFT                        (4U)
#define I2C_C2_SBRC(x)                           (((uint8_t)(((uint8_t)(x)) << I2C_C2_SBRC_SHIFT)) & I2C_C2_SBRC_MASK)
#define I2C_C2_HDRS_MASK                         (0x20U)
#define I2C_C2_HDRS_SHIFT                        (5U)
#define I2C_C2_HDRS(x)                           (((uint8_t)(((uint8_t)(x)) << I2C_C2_HDRS_SHIFT)) & I2C_C2_HDRS_MASK)
#define I2C_C2_ADEXT_MASK                        (0x40U)
#define I2C_C2_ADEXT_SHIFT                       (6U)
#define I2C_C2_ADEXT(x)                          (((uint8_t)(((uint8_t)(x)) << I2C_C2_ADEXT_SHIFT)) & I2C_C2_ADEXT_MASK)
#define I2C_C2_GCAEN_MASK                        (0x80U)
#define I2C_C2_GCAEN_SHIFT                       (7U)
#define I2C_C2_GCAEN(x)                          (((uint8_t)(((uint8_t)(x)) << I2C_C2_GCAEN_SHIFT)) & I2C_C2_GCAEN_MASK)

/*! @name FLT - I2C Programmable Input Glitch Filter Register */
#define I2C_FLT_FLT_MASK                         (0xFU)
#define I2C_FLT_FLT_SHIFT                        (0U)
#define I2C_FLT_FLT(x)                           (((uint8_t)(((uint8_t)(x)) << I2C_FLT_FLT_SHIFT)) & I2C_FLT_FLT_MASK)
#define I2C_FLT_STARTF_MASK                      (0x10U)
#define I2C_FLT_STARTF_SHIFT                     (4U)
#define I2C_FLT_STARTF(x)                        (((uint8_t)(((uint8_t)(x)) << I2C_FLT_STARTF_SHIFT)) & I2C_FLT_STARTF_MASK)
#define I2C_FLT_SSIE_MASK                        (0x20U)
#define I2C_FLT_SSIE_SHIFT                       (5U)
#define I2C_FLT_SSIE(x)                          (((uint8_t)(((uint8_t)(x)) << I2C_FLT_SSIE_SHIFT)) & I2C_FLT_SSIE_MASK)
#define I2C_FLT_STOPF_MASK                       (0x40U)
#define I2C_FLT_STOPF_SHIFT                      (6U)
#define I2C_FLT_STOPF(x)                         (((uint8_t)(((uint8_t)(x)) << I2C_FLT_STOPF_SHIFT)) & I2C_FLT_STOPF_MASK)
#define I2C_FLT_SHEN_MASK                        (0x80U)
#define I2C_FLT_SHEN_SHIFT                       (7U)
#define I2C_FLT_SHEN(x)                          (((uint8_t)(((uint8_t)(x)) << I2C_FLT_SHEN_SHIFT)) & I2C_FLT_SHEN_MASK)

/*! @name RA - I2C Range Address register */
#define I2C_RA_RAD_MASK                          (0xFEU)
#define I2C_RA_RAD_SHIFT                         (1U)
#define I2C_RA_RAD(x)                            (((uint8_t)(((uint8_t)(x)) << I2C_RA_RAD_SHIFT)) & I2C_RA_RAD_MASK)

/*! @name SMB - I2C SMBus Control and Status register */
#define I2C_SMB_SHTF2IE_MASK                     (0x1U)
#define I2C_SMB_SHTF2IE_SHIFT                    (0U)
#define I2C_SMB_SHTF2IE(x)                       (((uint8_t)(((uint8_t)(x)) << I2C_SMB_SHTF2IE_SHIFT)) & I2C_SMB_SHTF2IE_MASK)
#define I2C_SMB_SHTF2_MASK                       (0x2U)
#define I2C_SMB_SHTF2_SHIFT                      (1U)
#define I2C_SMB_SHTF2(x)                         (((uint8_t)(((uint8_t)(x)) << I2C_SMB_SHTF2_SHIFT)) & I2C_SMB_SHTF2_MASK)
#define I2C_SMB_SHTF1_MASK                       (0x4U)
#define I2C_SMB_SHTF1_SHIFT                      (2U)
#define I2C_SMB_SHTF1(x)                         (((uint8_t)(((uint8_t)(x)) << I2C_SMB_SHTF1_SHIFT)) & I2C_SMB_SHTF1_MASK)
#define I2C_SMB_SLTF_MASK                        (0x8U)
#define I2C_SMB_SLTF_SHIFT                       (3U)
#define I2C_SMB_SLTF(x)                          (((uint8_t)(((uint8_t)(x)) << I2C_SMB_SLTF_SHIFT)) & I2C_SMB_SLTF_MASK)
#define I2C_SMB_TCKSEL_MASK                      (0x10U)
#define I2C_SMB_TCKSEL_SHIFT                     (4U)
#define I2C_SMB_TCKSEL(x)                        (((uint8_t)(((uint8_t)(x)) << I2C_SMB_TCKSEL_SHIFT)) & I2C_SMB_TCKSEL_MASK)
#define I2C_SMB_SIICAEN_MASK                     (0x20U)
#define I2C_SMB_SIICAEN_SHIFT                    (5U)
#define I2C_SMB_SIICAEN(x)                       (((uint8_t)(((uint8_t)(x)) << I2C_SMB_SIICAEN_SHIFT)) & I2C_SMB_SIICAEN_MASK)
#define I2C_SMB_ALERTEN_MASK                     (0x40U)
#define I2C_SMB_ALERTEN_SHIFT                    (6U)
#define I2C_SMB_ALERTEN(x)                       (((uint8_t)(((uint8_t)(x)) << I2C_SMB_ALERTEN_SHIFT)) & I2C_SMB_ALERTEN_MASK)
#define I2C_SMB_FACK_MASK                        (0x80U)
#define I2C_SMB_FACK_SHIFT                       (7U)
#define I2C_SMB_FACK(x)                          (((uint8_t)(((uint8_t)(x)) << I2C_SMB_FACK_SHIFT)) & I2C_SMB_FACK_MASK)

/*! @name A2 - I2C Address Register 2 */
#define I2C_A2_SAD_MASK                          (0xFEU)
#define I2C_A2_SAD_SHIFT                         (1U)
#define I2C_A2_SAD(x)                            (((uint8_t)(((uint8_t)(x)) << I2C_A2_SAD_SHIFT)) & I2C_A2_SAD_MASK)

/*! @name SLTH - I2C SCL Low Timeout Register High */
#define I2C_SLTH_SSLT_MASK                       (0xFFU)
#define I2C_SLTH_SSLT_SHIFT                      (0U)
#define I2C_SLTH_SSLT(x)                         (((uint8_t)(((uint8_t)(x)) << I2C_SLTH_SSLT_SHIFT)) & I2C_SLTH_SSLT_MASK)

/*! @name SLTL - I2C SCL Low Timeout Register Low */
#define I2C_SLTL_SSLT_MASK                       (0xFFU)
#define I2C_SLTL_SSLT_SHIFT                      (0U)
#define I2C_SLTL_SSLT(x)                         (((uint8_t)(((uint8_t)(x)) << I2C_SLTL_SSLT_SHIFT)) & I2C_SLTL_SSLT_MASK)

/*! @name S2 - I2C Status register 2 */
#define I2C_S2_EMPTY_MASK                        (0x1U)
#define I2C_S2_EMPTY_SHIFT                       (0U)
#define I2C_S2_EMPTY(x)                          (((uint8_t)(((uint8_t)(x)) << I2C_S2_EMPTY_SHIFT)) & I2C_S2_EMPTY_MASK)
#define I2C_S2_ERROR_MASK                        (0x2U)
#define I2C_S2_ERROR_SHIFT                       (1U)
#define I2C_S2_ERROR(x)                          (((uint8_t)(((uint8_t)(x)) << I2C_S2_ERROR_SHIFT)) & I2C_S2_ERROR_MASK)
#define I2C_S2_DFEN_MASK                         (0x4U)
#define I2C_S2_DFEN_SHIFT                        (2U)
#define I2C_S2_DFEN(x)                           (((uint8_t)(((uint8_t)(x)) << I2C_S2_DFEN_SHIFT)) & I2C_S2_DFEN_MASK)


/*!
 * @}
 */ /* end of group I2C_Register_Masks */


/* I2C - Peripheral instance base addresses */
/** Peripheral I2C0 base address */
#define I2C0_BASE                                (0x40066000u)
/** Peripheral I2C0 base pointer */
#define I2C0                                     ((I2C_Type *)I2C0_BASE)
/** Peripheral I2C1 base address */
#define I2C1_BASE                                (0x40067000u)
/** Peripheral I2C1 base pointer */
#define I2C1                                     ((I2C_Type *)I2C1_BASE)
/** Array initializer of I2C peripheral base addresses */
#define I2C_BASE_ADDRS                           { I2C0_BASE, I2C1_BASE }
/** Array initializer of I2C peripheral base pointers */
#define I2C_BASE_PTRS                            { I2C0, I2C1 }
/** Interrupt vectors for the I2C peripheral type */
#define I2C_IRQS                                 { I2C0_IRQn, I2C1_IRQn }

/*!
 * @}
 */ /* end of group I2C_Peripheral_Access_Layer */


/* ----------------------------------------------------------------------------
   -- LLWU Peripheral Access Layer
   ---------------------------------------------------------------------------- */

/*!
 * @addtogroup LLWU_Peripheral_Access_Layer LLWU Peripheral Access Layer
 * @{
 */

/** LLWU - Register Layout Typedef */
typedef struct {
  __IO uint8_t PE1;                                /**< LLWU Pin Enable 1 register, offset: 0x0 */
  __IO uint8_t PE2;                                /**< LLWU Pin Enable 2 register, offset: 0x1 */
  __IO uint8_t PE3;                                /**< LLWU Pin Enable 3 register, offset: 0x2 */
  __IO uint8_t PE4;                                /**< LLWU Pin Enable 4 register, offset: 0x3 */
  __IO uint8_t ME;                                 /**< LLWU Module Enable register, offset: 0x4 */
  __IO uint8_t F1;                                 /**< LLWU Flag 1 register, offset: 0x5 */
  __IO uint8_t F2;                                 /**< LLWU Flag 2 register, offset: 0x6 */
  __I  uint8_t F3;                                 /**< LLWU Flag 3 register, offset: 0x7 */
  __IO uint8_t FILT1;                              /**< LLWU Pin Filter 1 register, offset: 0x8 */
  __IO uint8_t FILT2;                              /**< LLWU Pin Filter 2 register, offset: 0x9 */
} LLWU_Type;

/* ----------------------------------------------------------------------------
   -- LLWU Register Masks
   ---------------------------------------------------------------------------- */

/*!
 * @addtogroup LLWU_Register_Masks LLWU Register Masks
 * @{
 */

/*! @name PE1 - LLWU Pin Enable 1 register */
#define LLWU_PE1_WUPE0_MASK                      (0x3U)
#define LLWU_PE1_WUPE0_SHIFT                     (0U)
#define LLWU_PE1_WUPE0(x)                        (((uint8_t)(((uint8_t)(x)) << LLWU_PE1_WUPE0_SHIFT)) & LLWU_PE1_WUPE0_MASK)
#define LLWU_PE1_WUPE1_MASK                      (0xCU)
#define LLWU_PE1_WUPE1_SHIFT                     (2U)
#define LLWU_PE1_WUPE1(x)                        (((uint8_t)(((uint8_t)(x)) << LLWU_PE1_WUPE1_SHIFT)) & LLWU_PE1_WUPE1_MASK)
#define LLWU_PE1_WUPE2_MASK                      (0x30U)
#define LLWU_PE1_WUPE2_SHIFT                     (4U)
#define LLWU_PE1_WUPE2(x)                        (((uint8_t)(((uint8_t)(x)) << LLWU_PE1_WUPE2_SHIFT)) & LLWU_PE1_WUPE2_MASK)
#define LLWU_PE1_WUPE3_MASK                      (0xC0U)
#define LLWU_PE1_WUPE3_SHIFT                     (6U)
#define LLWU_PE1_WUPE3(x)                        (((uint8_t)(((uint8_t)(x)) << LLWU_PE1_WUPE3_SHIFT)) & LLWU_PE1_WUPE3_MASK)

/*! @name PE2 - LLWU Pin Enable 2 register */
#define LLWU_PE2_WUPE4_MASK                      (0x3U)
#define LLWU_PE2_WUPE4_SHIFT                     (0U)
#define LLWU_PE2_WUPE4(x)                        (((uint8_t)(((uint8_t)(x)) << LLWU_PE2_WUPE4_SHIFT)) & LLWU_PE2_WUPE4_MASK)
#define LLWU_PE2_WUPE5_MASK                      (0xCU)
#define LLWU_PE2_WUPE5_SHIFT                     (2U)
#define LLWU_PE2_WUPE5(x)                        (((uint8_t)(((uint8_t)(x)) << LLWU_PE2_WUPE5_SHIFT)) & LLWU_PE2_WUPE5_MASK)
#define LLWU_PE2_WUPE6_MASK                      (0x30U)
#define LLWU_PE2_WUPE6_SHIFT                     (4U)
#define LLWU_PE2_WUPE6(x)                        (((uint8_t)(((uint8_t)(x)) << LLWU_PE2_WUPE6_SHIFT)) & LLWU_PE2_WUPE6_MASK)
#define LLWU_PE2_WUPE7_MASK                      (0xC0U)
#define LLWU_PE2_WUPE7_SHIFT                     (6U)
#define LLWU_PE2_WUPE7(x)                        (((uint8_t)(((uint8_t)(x)) << LLWU_PE2_WUPE7_SHIFT)) & LLWU_PE2_WUPE7_MASK)

/*! @name PE3 - LLWU Pin Enable 3 register */
#define LLWU_PE3_WUPE8_MASK                      (0x3U)
#define LLWU_PE3_WUPE8_SHIFT                     (0U)
#define LLWU_PE3_WUPE8(x)                        (((uint8_t)(((uint8_t)(x)) << LLWU_PE3_WUPE8_SHIFT)) & LLWU_PE3_WUPE8_MASK)
#define LLWU_PE3_WUPE9_MASK                      (0xCU)
#define LLWU_PE3_WUPE9_SHIFT                     (2U)
#define LLWU_PE3_WUPE9(x)                        (((uint8_t)(((uint8_t)(x)) << LLWU_PE3_WUPE9_SHIFT)) & LLWU_PE3_WUPE9_MASK)
#define LLWU_PE3_WUPE10_MASK                     (0x30U)
#define LLWU_PE3_WUPE10_SHIFT                    (4U)
#define LLWU_PE3_WUPE10(x)                       (((uint8_t)(((uint8_t)(x)) << LLWU_PE3_WUPE10_SHIFT)) & LLWU_PE3_WUPE10_MASK)
#define LLWU_PE3_WUPE11_MASK                     (0xC0U)
#define LLWU_PE3_WUPE11_SHIFT                    (6U)
#define LLWU_PE3_WUPE11(x)                       (((uint8_t)(((uint8_t)(x)) << LLWU_PE3_WUPE11_SHIFT)) & LLWU_PE3_WUPE11_MASK)

/*! @name PE4 - LLWU Pin Enable 4 register */
#define LLWU_PE4_WUPE12_MASK                     (0x3U)
#define LLWU_PE4_WUPE12_SHIFT                    (0U)
#define LLWU_PE4_WUPE12(x)                       (((uint8_t)(((uint8_t)(x)) << LLWU_PE4_WUPE12_SHIFT)) & LLWU_PE4_WUPE12_MASK)
#define LLWU_PE4_WUPE13_MASK                     (0xCU)
#define LLWU_PE4_WUPE13_SHIFT                    (2U)
#define LLWU_PE4_WUPE13(x)                       (((uint8_t)(((uint8_t)(x)) << LLWU_PE4_WUPE13_SHIFT)) & LLWU_PE4_WUPE13_MASK)
#define LLWU_PE4_WUPE14_MASK                     (0x30U)
#define LLWU_PE4_WUPE14_SHIFT                    (4U)
#define LLWU_PE4_WUPE14(x)                       (((uint8_t)(((uint8_t)(x)) << LLWU_PE4_WUPE14_SHIFT)) & LLWU_PE4_WUPE14_MASK)
#define LLWU_PE4_WUPE15_MASK                     (0xC0U)
#define LLWU_PE4_WUPE15_SHIFT                    (6U)
#define LLWU_PE4_WUPE15(x)                       (((uint8_t)(((uint8_t)(x)) << LLWU_PE4_WUPE15_SHIFT)) & LLWU_PE4_WUPE15_MASK)

/*! @name ME - LLWU Module Enable register */
#define LLWU_ME_WUME0_MASK                       (0x1U)
#define LLWU_ME_WUME0_SHIFT                      (0U)
#define LLWU_ME_WUME0(x)                         (((uint8_t)(((uint8_t)(x)) << LLWU_ME_WUME0_SHIFT)) & LLWU_ME_WUME0_MASK)
#define LLWU_ME_WUME1_MASK                       (0x2U)
#define LLWU_ME_WUME1_SHIFT                      (1U)
#define LLWU_ME_WUME1(x)                         (((uint8_t)(((uint8_t)(x)) << LLWU_ME_WUME1_SHIFT)) & LLWU_ME_WUME1_MASK)
#define LLWU_ME_WUME2_MASK                       (0x4U)
#define LLWU_ME_WUME2_SHIFT                      (2U)
#define LLWU_ME_WUME2(x)                         (((uint8_t)(((uint8_t)(x)) << LLWU_ME_WUME2_SHIFT)) & LLWU_ME_WUME2_MASK)
#define LLWU_ME_WUME3_MASK                       (0x8U)
#define LLWU_ME_WUME3_SHIFT                      (3U)
#define LLWU_ME_WUME3(x)                         (((uint8_t)(((uint8_t)(x)) << LLWU_ME_WUME3_SHIFT)) & LLWU_ME_WUME3_MASK)
#define LLWU_ME_WUME4_MASK                       (0x10U)
#define LLWU_ME_WUME4_SHIFT                      (4U)
#define LLWU_ME_WUME4(x)                         (((uint8_t)(((uint8_t)(x)) << LLWU_ME_WUME4_SHIFT)) & LLWU_ME_WUME4_MASK)
#define LLWU_ME_WUME5_MASK                       (0x20U)
#define LLWU_ME_WUME5_SHIFT                      (5U)
#define LLWU_ME_WUME5(x)                         (((uint8_t)(((uint8_t)(x)) << LLWU_ME_WUME5_SHIFT)) & LLWU_ME_WUME5_MASK)
#define LLWU_ME_WUME6_MASK                       (0x40U)
#define LLWU_ME_WUME6_SHIFT                      (6U)
#define LLWU_ME_WUME6(x)                         (((uint8_t)(((uint8_t)(x)) << LLWU_ME_WUME6_SHIFT)) & LLWU_ME_WUME6_MASK)
#define LLWU_ME_WUME7_MASK                       (0x80U)
#define LLWU_ME_WUME7_SHIFT                      (7U)
#define LLWU_ME_WUME7(x)                         (((uint8_t)(((uint8_t)(x)) << LLWU_ME_WUME7_SHIFT)) & LLWU_ME_WUME7_MASK)

/*! @name F1 - LLWU Flag 1 register */
#define LLWU_F1_WUF0_MASK                        (0x1U)
#define LLWU_F1_WUF0_SHIFT                       (0U)
#define LLWU_F1_WUF0(x)                          (((uint8_t)(((uint8_t)(x)) << LLWU_F1_WUF0_SHIFT)) & LLWU_F1_WUF0_MASK)
#define LLWU_F1_WUF1_MASK                        (0x2U)
#define LLWU_F1_WUF1_SHIFT                       (1U)
#define LLWU_F1_WUF1(x)                          (((uint8_t)(((uint8_t)(x)) << LLWU_F1_WUF1_SHIFT)) & LLWU_F1_WUF1_MASK)
#define LLWU_F1_WUF2_MASK                        (0x4U)
#define LLWU_F1_WUF2_SHIFT                       (2U)
#define LLWU_F1_WUF2(x)                          (((uint8_t)(((uint8_t)(x)) << LLWU_F1_WUF2_SHIFT)) & LLWU_F1_WUF2_MASK)
#define LLWU_F1_WUF3_MASK                        (0x8U)
#define LLWU_F1_WUF3_SHIFT                       (3U)
#define LLWU_F1_WUF3(x)                          (((uint8_t)(((uint8_t)(x)) << LLWU_F1_WUF3_SHIFT)) & LLWU_F1_WUF3_MASK)
#define LLWU_F1_WUF4_MASK                        (0x10U)
#define LLWU_F1_WUF4_SHIFT                       (4U)
#define LLWU_F1_WUF4(x)                          (((uint8_t)(((uint8_t)(x)) << LLWU_F1_WUF4_SHIFT)) & LLWU_F1_WUF4_MASK)
#define LLWU_F1_WUF5_MASK                        (0x20U)
#define LLWU_F1_WUF5_SHIFT                       (5U)
#define LLWU_F1_WUF5(x)                          (((uint8_t)(((uint8_t)(x)) << LLWU_F1_WUF5_SHIFT)) & LLWU_F1_WUF5_MASK)
#define LLWU_F1_WUF6_MASK                        (0x40U)
#define LLWU_F1_WUF6_SHIFT                       (6U)
#define LLWU_F1_WUF6(x)                          (((uint8_t)(((uint8_t)(x)) << LLWU_F1_WUF6_SHIFT)) & LLWU_F1_WUF6_MASK)
#define LLWU_F1_WUF7_MASK                        (0x80U)
#define LLWU_F1_WUF7_SHIFT                       (7U)
#define LLWU_F1_WUF7(x)                          (((uint8_t)(((uint8_t)(x)) << LLWU_F1_WUF7_SHIFT)) & LLWU_F1_WUF7_MASK)

/*! @name F2 - LLWU Flag 2 register */
#define LLWU_F2_WUF8_MASK                        (0x1U)
#define LLWU_F2_WUF8_SHIFT                       (0U)
#define LLWU_F2_WUF8(x)                          (((uint8_t)(((uint8_t)(x)) << LLWU_F2_WUF8_SHIFT)) & LLWU_F2_WUF8_MASK)
#define LLWU_F2_WUF9_MASK                        (0x2U)
#define LLWU_F2_WUF9_SHIFT                       (1U)
#define LLWU_F2_WUF9(x)                          (((uint8_t)(((uint8_t)(x)) << LLWU_F2_WUF9_SHIFT)) & LLWU_F2_WUF9_MASK)
#define LLWU_F2_WUF10_MASK                       (0x4U)
#define LLWU_F2_WUF10_SHIFT                      (2U)
#define LLWU_F2_WUF10(x)                         (((uint8_t)(((uint8_t)(x)) << LLWU_F2_WUF10_SHIFT)) & LLWU_F2_WUF10_MASK)
#define LLWU_F2_WUF11_MASK                       (0x8U)
#define LLWU_F2_WUF11_SHIFT                      (3U)
#define LLWU_F2_WUF11(x)                         (((uint8_t)(((uint8_t)(x)) << LLWU_F2_WUF11_SHIFT)) & LLWU_F2_WUF11_MASK)
#define LLWU_F2_WUF12_MASK                       (0x10U)
#define LLWU_F2_WUF12_SHIFT                      (4U)
#define LLWU_F2_WUF12(x)                         (((uint8_t)(((uint8_t)(x)) << LLWU_F2_WUF12_SHIFT)) & LLWU_F2_WUF12_MASK)
#define LLWU_F2_WUF13_MASK                       (0x20U)
#define LLWU_F2_WUF13_SHIFT                      (5U)
#define LLWU_F2_WUF13(x)                         (((uint8_t)(((uint8_t)(x)) << LLWU_F2_WUF13_SHIFT)) & LLWU_F2_WUF13_MASK)
#define LLWU_F2_WUF14_MASK                       (0x40U)
#define LLWU_F2_WUF14_SHIFT                      (6U)
#define LLWU_F2_WUF14(x)                         (((uint8_t)(((uint8_t)(x)) << LLWU_F2_WUF14_SHIFT)) & LLWU_F2_WUF14_MASK)
#define LLWU_F2_WUF15_MASK                       (0x80U)
#define LLWU_F2_WUF15_SHIFT                      (7U)
#define LLWU_F2_WUF15(x)                         (((uint8_t)(((uint8_t)(x)) << LLWU_F2_WUF15_SHIFT)) & LLWU_F2_WUF15_MASK)

/*! @name F3 - LLWU Flag 3 register */
#define LLWU_F3_MWUF0_MASK                       (0x1U)
#define LLWU_F3_MWUF0_SHIFT                      (0U)
#define LLWU_F3_MWUF0(x)                         (((uint8_t)(((uint8_t)(x)) << LLWU_F3_MWUF0_SHIFT)) & LLWU_F3_MWUF0_MASK)
#define LLWU_F3_MWUF1_MASK                       (0x2U)
#define LLWU_F3_MWUF1_SHIFT                      (1U)
#define LLWU_F3_MWUF1(x)                         (((uint8_t)(((uint8_t)(x)) << LLWU_F3_MWUF1_SHIFT)) & LLWU_F3_MWUF1_MASK)
#define LLWU_F3_MWUF2_MASK                       (0x4U)
#define LLWU_F3_MWUF2_SHIFT                      (2U)
#define LLWU_F3_MWUF2(x)                         (((uint8_t)(((uint8_t)(x)) << LLWU_F3_MWUF2_SHIFT)) & LLWU_F3_MWUF2_MASK)
#define LLWU_F3_MWUF3_MASK                       (0x8U)
#define LLWU_F3_MWUF3_SHIFT                      (3U)
#define LLWU_F3_MWUF3(x)                         (((uint8_t)(((uint8_t)(x)) << LLWU_F3_MWUF3_SHIFT)) & LLWU_F3_MWUF3_MASK)
#define LLWU_F3_MWUF4_MASK                       (0x10U)
#define LLWU_F3_MWUF4_SHIFT                      (4U)
#define LLWU_F3_MWUF4(x)                         (((uint8_t)(((uint8_t)(x)) << LLWU_F3_MWUF4_SHIFT)) & LLWU_F3_MWUF4_MASK)
#define LLWU_F3_MWUF5_MASK                       (0x20U)
#define LLWU_F3_MWUF5_SHIFT                      (5U)
#define LLWU_F3_MWUF5(x)                         (((uint8_t)(((uint8_t)(x)) << LLWU_F3_MWUF5_SHIFT)) & LLWU_F3_MWUF5_MASK)
#define LLWU_F3_MWUF6_MASK                       (0x40U)
#define LLWU_F3_MWUF6_SHIFT                      (6U)
#define LLWU_F3_MWUF6(x)                         (((uint8_t)(((uint8_t)(x)) << LLWU_F3_MWUF6_SHIFT)) & LLWU_F3_MWUF6_MASK)
#define LLWU_F3_MWUF7_MASK                       (0x80U)
#define LLWU_F3_MWUF7_SHIFT                      (7U)
#define LLWU_F3_MWUF7(x)                         (((uint8_t)(((uint8_t)(x)) << LLWU_F3_MWUF7_SHIFT)) & LLWU_F3_MWUF7_MASK)

/*! @name FILT1 - LLWU Pin Filter 1 register */
#define LLWU_FILT1_FILTSEL_MASK                  (0xFU)
#define LLWU_FILT1_FILTSEL_SHIFT                 (0U)
#define LLWU_FILT1_FILTSEL(x)                    (((uint8_t)(((uint8_t)(x)) << LLWU_FILT1_FILTSEL_SHIFT)) & LLWU_FILT1_FILTSEL_MASK)
#define LLWU_FILT1_FILTE_MASK                    (0x60U)
#define LLWU_FILT1_FILTE_SHIFT                   (5U)
#define LLWU_FILT1_FILTE(x)                      (((uint8_t)(((uint8_t)(x)) << LLWU_FILT1_FILTE_SHIFT)) & LLWU_FILT1_FILTE_MASK)
#define LLWU_FILT1_FILTF_MASK                    (0x80U)
#define LLWU_FILT1_FILTF_SHIFT                   (7U)
#define LLWU_FILT1_FILTF(x)                      (((uint8_t)(((uint8_t)(x)) << LLWU_FILT1_FILTF_SHIFT)) & LLWU_FILT1_FILTF_MASK)

/*! @name FILT2 - LLWU Pin Filter 2 register */
#define LLWU_FILT2_FILTSEL_MASK                  (0xFU)
#define LLWU_FILT2_FILTSEL_SHIFT                 (0U)
#define LLWU_FILT2_FILTSEL(x)                    (((uint8_t)(((uint8_t)(x)) << LLWU_FILT2_FILTSEL_SHIFT)) & LLWU_FILT2_FILTSEL_MASK)
#define LLWU_FILT2_FILTE_MASK                    (0x60U)
#define LLWU_FILT2_FILTE_SHIFT                   (5U)
#define LLWU_FILT2_FILTE(x)                      (((uint8_t)(((uint8_t)(x)) << LLWU_FILT2_FILTE_SHIFT)) & LLWU_FILT2_FILTE_MASK)
#define LLWU_FILT2_FILTF_MASK                    (0x80U)
#define LLWU_FILT2_FILTF_SHIFT                   (7U)
#define LLWU_FILT2_FILTF(x)                      (((uint8_t)(((uint8_t)(x)) << LLWU_FILT2_FILTF_SHIFT)) & LLWU_FILT2_FILTF_MASK)


/*!
 * @}
 */ /* end of group LLWU_Register_Masks */


/* LLWU - Peripheral instance base addresses */
/** Peripheral LLWU base address */
#define LLWU_BASE                                (0x4007C000u)
/** Peripheral LLWU base pointer */
#define LLWU                                     ((LLWU_Type *)LLWU_BASE)
/** Array initializer of LLWU peripheral base addresses */
#define LLWU_BASE_ADDRS                          { LLWU_BASE }
/** Array initializer of LLWU peripheral base pointers */
#define LLWU_BASE_PTRS                           { LLWU }
/** Interrupt vectors for the LLWU peripheral type */
#define LLWU_IRQS                                { LLWU_IRQn }

/*!
 * @}
 */ /* end of group LLWU_Peripheral_Access_Layer */


/* ----------------------------------------------------------------------------
   -- LPTMR Peripheral Access Layer
   ---------------------------------------------------------------------------- */

/*!
 * @addtogroup LPTMR_Peripheral_Access_Layer LPTMR Peripheral Access Layer
 * @{
 */

/** LPTMR - Register Layout Typedef */
typedef struct {
  __IO uint32_t CSR;                               /**< Low Power Timer Control Status Register, offset: 0x0 */
  __IO uint32_t PSR;                               /**< Low Power Timer Prescale Register, offset: 0x4 */
  __IO uint32_t CMR;                               /**< Low Power Timer Compare Register, offset: 0x8 */
  __IO uint32_t CNR;                               /**< Low Power Timer Counter Register, offset: 0xC */
} LPTMR_Type;

/* ----------------------------------------------------------------------------
   -- LPTMR Register Masks
   ---------------------------------------------------------------------------- */

/*!
 * @addtogroup LPTMR_Register_Masks LPTMR Register Masks
 * @{
 */

/*! @name CSR - Low Power Timer Control Status Register */
#define LPTMR_CSR_TEN_MASK                       (0x1U)
#define LPTMR_CSR_TEN_SHIFT                      (0U)
#define LPTMR_CSR_TEN(x)                         (((uint32_t)(((uint32_t)(x)) << LPTMR_CSR_TEN_SHIFT)) & LPTMR_CSR_TEN_MASK)
#define LPTMR_CSR_TMS_MASK                       (0x2U)
#define LPTMR_CSR_TMS_SHIFT                      (1U)
#define LPTMR_CSR_TMS(x)                         (((uint32_t)(((uint32_t)(x)) << LPTMR_CSR_TMS_SHIFT)) & LPTMR_CSR_TMS_MASK)
#define LPTMR_CSR_TFC_MASK                       (0x4U)
#define LPTMR_CSR_TFC_SHIFT                      (2U)
#define LPTMR_CSR_TFC(x)                         (((uint32_t)(((uint32_t)(x)) << LPTMR_CSR_TFC_SHIFT)) & LPTMR_CSR_TFC_MASK)
#define LPTMR_CSR_TPP_MASK                       (0x8U)
#define LPTMR_CSR_TPP_SHIFT                      (3U)
#define LPTMR_CSR_TPP(x)                         (((uint32_t)(((uint32_t)(x)) << LPTMR_CSR_TPP_SHIFT)) & LPTMR_CSR_TPP_MASK)
#define LPTMR_CSR_TPS_MASK                       (0x30U)
#define LPTMR_CSR_TPS_SHIFT                      (4U)
#define LPTMR_CSR_TPS(x)                         (((uint32_t)(((uint32_t)(x)) << LPTMR_CSR_TPS_SHIFT)) & LPTMR_CSR_TPS_MASK)
#define LPTMR_CSR_TIE_MASK                       (0x40U)
#define LPTMR_CSR_TIE_SHIFT                      (6U)
#define LPTMR_CSR_TIE(x)                         (((uint32_t)(((uint32_t)(x)) << LPTMR_CSR_TIE_SHIFT)) & LPTMR_CSR_TIE_MASK)
#define LPTMR_CSR_TCF_MASK                       (0x80U)
#define LPTMR_CSR_TCF_SHIFT                      (7U)
#define LPTMR_CSR_TCF(x)                         (((uint32_t)(((uint32_t)(x)) << LPTMR_CSR_TCF_SHIFT)) & LPTMR_CSR_TCF_MASK)

/*! @name PSR - Low Power Timer Prescale Register */
#define LPTMR_PSR_PCS_MASK                       (0x3U)
#define LPTMR_PSR_PCS_SHIFT                      (0U)
#define LPTMR_PSR_PCS(x)                         (((uint32_t)(((uint32_t)(x)) << LPTMR_PSR_PCS_SHIFT)) & LPTMR_PSR_PCS_MASK)
#define LPTMR_PSR_PBYP_MASK                      (0x4U)
#define LPTMR_PSR_PBYP_SHIFT                     (2U)
#define LPTMR_PSR_PBYP(x)                        (((uint32_t)(((uint32_t)(x)) << LPTMR_PSR_PBYP_SHIFT)) & LPTMR_PSR_PBYP_MASK)
#define LPTMR_PSR_PRESCALE_MASK                  (0x78U)
#define LPTMR_PSR_PRESCALE_SHIFT                 (3U)
#define LPTMR_PSR_PRESCALE(x)                    (((uint32_t)(((uint32_t)(x)) << LPTMR_PSR_PRESCALE_SHIFT)) & LPTMR_PSR_PRESCALE_MASK)

/*! @name CMR - Low Power Timer Compare Register */
#define LPTMR_CMR_COMPARE_MASK                   (0xFFFFU)
#define LPTMR_CMR_COMPARE_SHIFT                  (0U)
#define LPTMR_CMR_COMPARE(x)                     (((uint32_t)(((uint32_t)(x)) << LPTMR_CMR_COMPARE_SHIFT)) & LPTMR_CMR_COMPARE_MASK)

/*! @name CNR - Low Power Timer Counter Register */
#define LPTMR_CNR_COUNTER_MASK                   (0xFFFFU)
#define LPTMR_CNR_COUNTER_SHIFT                  (0U)
#define LPTMR_CNR_COUNTER(x)                     (((uint32_t)(((uint32_t)(x)) << LPTMR_CNR_COUNTER_SHIFT)) & LPTMR_CNR_COUNTER_MASK)


/*!
 * @}
 */ /* end of group LPTMR_Register_Masks */


/* LPTMR - Peripheral instance base addresses */
/** Peripheral LPTMR0 base address */
#define LPTMR0_BASE                              (0x40040000u)
/** Peripheral LPTMR0 base pointer */
#define LPTMR0                                   ((LPTMR_Type *)LPTMR0_BASE)
/** Array initializer of LPTMR peripheral base addresses */
#define LPTMR_BASE_ADDRS                         { LPTMR0_BASE }
/** Array initializer of LPTMR peripheral base pointers */
#define LPTMR_BASE_PTRS                          { LPTMR0 }
/** Interrupt vectors for the LPTMR peripheral type */
#define LPTMR_IRQS                               { LPTMR0_IRQn }

/*!
 * @}
 */ /* end of group LPTMR_Peripheral_Access_Layer */


/* ----------------------------------------------------------------------------
   -- LPUART Peripheral Access Layer
   ---------------------------------------------------------------------------- */

/*!
 * @addtogroup LPUART_Peripheral_Access_Layer LPUART Peripheral Access Layer
 * @{
 */

/** LPUART - Register Layout Typedef */
typedef struct {
  __IO uint32_t BAUD;                              /**< LPUART Baud Rate Register, offset: 0x0 */
  __IO uint32_t STAT;                              /**< LPUART Status Register, offset: 0x4 */
  __IO uint32_t CTRL;                              /**< LPUART Control Register, offset: 0x8 */
  __IO uint32_t DATA;                              /**< LPUART Data Register, offset: 0xC */
  __IO uint32_t MATCH;                             /**< LPUART Match Address Register, offset: 0x10 */
  __IO uint32_t MODIR;                             /**< LPUART Modem IrDA Register, offset: 0x14 */
} LPUART_Type;

/* ----------------------------------------------------------------------------
   -- LPUART Register Masks
   ---------------------------------------------------------------------------- */

/*!
 * @addtogroup LPUART_Register_Masks LPUART Register Masks
 * @{
 */

/*! @name BAUD - LPUART Baud Rate Register */
#define LPUART_BAUD_SBR_MASK                     (0x1FFFU)
#define LPUART_BAUD_SBR_SHIFT                    (0U)
#define LPUART_BAUD_SBR(x)                       (((uint32_t)(((uint32_t)(x)) << LPUART_BAUD_SBR_SHIFT)) & LPUART_BAUD_SBR_MASK)
#define LPUART_BAUD_SBNS_MASK                    (0x2000U)
#define LPUART_BAUD_SBNS_SHIFT                   (13U)
#define LPUART_BAUD_SBNS(x)                      (((uint32_t)(((uint32_t)(x)) << LPUART_BAUD_SBNS_SHIFT)) & LPUART_BAUD_SBNS_MASK)
#define LPUART_BAUD_RXEDGIE_MASK                 (0x4000U)
#define LPUART_BAUD_RXEDGIE_SHIFT                (14U)
#define LPUART_BAUD_RXEDGIE(x)                   (((uint32_t)(((uint32_t)(x)) << LPUART_BAUD_RXEDGIE_SHIFT)) & LPUART_BAUD_RXEDGIE_MASK)
#define LPUART_BAUD_LBKDIE_MASK                  (0x8000U)
#define LPUART_BAUD_LBKDIE_SHIFT                 (15U)
#define LPUART_BAUD_LBKDIE(x)                    (((uint32_t)(((uint32_t)(x)) << LPUART_BAUD_LBKDIE_SHIFT)) & LPUART_BAUD_LBKDIE_MASK)
#define LPUART_BAUD_RESYNCDIS_MASK               (0x10000U)
#define LPUART_BAUD_RESYNCDIS_SHIFT              (16U)
#define LPUART_BAUD_RESYNCDIS(x)                 (((uint32_t)(((uint32_t)(x)) << LPUART_BAUD_RESYNCDIS_SHIFT)) & LPUART_BAUD_RESYNCDIS_MASK)
#define LPUART_BAUD_BOTHEDGE_MASK                (0x20000U)
#define LPUART_BAUD_BOTHEDGE_SHIFT               (17U)
#define LPUART_BAUD_BOTHEDGE(x)                  (((uint32_t)(((uint32_t)(x)) << LPUART_BAUD_BOTHEDGE_SHIFT)) & LPUART_BAUD_BOTHEDGE_MASK)
#define LPUART_BAUD_MATCFG_MASK                  (0xC0000U)
#define LPUART_BAUD_MATCFG_SHIFT                 (18U)
#define LPUART_BAUD_MATCFG(x)                    (((uint32_t)(((uint32_t)(x)) << LPUART_BAUD_MATCFG_SHIFT)) & LPUART_BAUD_MATCFG_MASK)
#define LPUART_BAUD_RDMAE_MASK                   (0x200000U)
#define LPUART_BAUD_RDMAE_SHIFT                  (21U)
#define LPUART_BAUD_RDMAE(x)                     (((uint32_t)(((uint32_t)(x)) << LPUART_BAUD_RDMAE_SHIFT)) & LPUART_BAUD_RDMAE_MASK)
#define LPUART_BAUD_TDMAE_MASK                   (0x800000U)
#define LPUART_BAUD_TDMAE_SHIFT                  (23U)
#define LPUART_BAUD_TDMAE(x)                     (((uint32_t)(((uint32_t)(x)) << LPUART_BAUD_TDMAE_SHIFT)) & LPUART_BAUD_TDMAE_MASK)
#define LPUART_BAUD_OSR_MASK                     (0x1F000000U)
#define LPUART_BAUD_OSR_SHIFT                    (24U)
#define LPUART_BAUD_OSR(x)                       (((uint32_t)(((uint32_t)(x)) << LPUART_BAUD_OSR_SHIFT)) & LPUART_BAUD_OSR_MASK)
#define LPUART_BAUD_M10_MASK                     (0x20000000U)
#define LPUART_BAUD_M10_SHIFT                    (29U)
#define LPUART_BAUD_M10(x)                       (((uint32_t)(((uint32_t)(x)) << LPUART_BAUD_M10_SHIFT)) & LPUART_BAUD_M10_MASK)
#define LPUART_BAUD_MAEN2_MASK                   (0x40000000U)
#define LPUART_BAUD_MAEN2_SHIFT                  (30U)
#define LPUART_BAUD_MAEN2(x)                     (((uint32_t)(((uint32_t)(x)) << LPUART_BAUD_MAEN2_SHIFT)) & LPUART_BAUD_MAEN2_MASK)
#define LPUART_BAUD_MAEN1_MASK                   (0x80000000U)
#define LPUART_BAUD_MAEN1_SHIFT                  (31U)
#define LPUART_BAUD_MAEN1(x)                     (((uint32_t)(((uint32_t)(x)) << LPUART_BAUD_MAEN1_SHIFT)) & LPUART_BAUD_MAEN1_MASK)

/*! @name STAT - LPUART Status Register */
#define LPUART_STAT_MA2F_MASK                    (0x4000U)
#define LPUART_STAT_MA2F_SHIFT                   (14U)
#define LPUART_STAT_MA2F(x)                      (((uint32_t)(((uint32_t)(x)) << LPUART_STAT_MA2F_SHIFT)) & LPUART_STAT_MA2F_MASK)
#define LPUART_STAT_MA1F_MASK                    (0x8000U)
#define LPUART_STAT_MA1F_SHIFT                   (15U)
#define LPUART_STAT_MA1F(x)                      (((uint32_t)(((uint32_t)(x)) << LPUART_STAT_MA1F_SHIFT)) & LPUART_STAT_MA1F_MASK)
#define LPUART_STAT_PF_MASK                      (0x10000U)
#define LPUART_STAT_PF_SHIFT                     (16U)
#define LPUART_STAT_PF(x)                        (((uint32_t)(((uint32_t)(x)) << LPUART_STAT_PF_SHIFT)) & LPUART_STAT_PF_MASK)
#define LPUART_STAT_FE_MASK                      (0x20000U)
#define LPUART_STAT_FE_SHIFT                     (17U)
#define LPUART_STAT_FE(x)                        (((uint32_t)(((uint32_t)(x)) << LPUART_STAT_FE_SHIFT)) & LPUART_STAT_FE_MASK)
#define LPUART_STAT_NF_MASK                      (0x40000U)
#define LPUART_STAT_NF_SHIFT                     (18U)
#define LPUART_STAT_NF(x)                        (((uint32_t)(((uint32_t)(x)) << LPUART_STAT_NF_SHIFT)) & LPUART_STAT_NF_MASK)
#define LPUART_STAT_OR_MASK                      (0x80000U)
#define LPUART_STAT_OR_SHIFT                     (19U)
#define LPUART_STAT_OR(x)                        (((uint32_t)(((uint32_t)(x)) << LPUART_STAT_OR_SHIFT)) & LPUART_STAT_OR_MASK)
#define LPUART_STAT_IDLE_MASK                    (0x100000U)
#define LPUART_STAT_IDLE_SHIFT                   (20U)
#define LPUART_STAT_IDLE(x)                      (((uint32_t)(((uint32_t)(x)) << LPUART_STAT_IDLE_SHIFT)) & LPUART_STAT_IDLE_MASK)
#define LPUART_STAT_RDRF_MASK                    (0x200000U)
#define LPUART_STAT_RDRF_SHIFT                   (21U)
#define LPUART_STAT_RDRF(x)                      (((uint32_t)(((uint32_t)(x)) << LPUART_STAT_RDRF_SHIFT)) & LPUART_STAT_RDRF_MASK)
#define LPUART_STAT_TC_MASK                      (0x400000U)
#define LPUART_STAT_TC_SHIFT                     (22U)
#define LPUART_STAT_TC(x)                        (((uint32_t)(((uint32_t)(x)) << LPUART_STAT_TC_SHIFT)) & LPUART_STAT_TC_MASK)
#define LPUART_STAT_TDRE_MASK                    (0x800000U)
#define LPUART_STAT_TDRE_SHIFT                   (23U)
#define LPUART_STAT_TDRE(x)                      (((uint32_t)(((uint32_t)(x)) << LPUART_STAT_TDRE_SHIFT)) & LPUART_STAT_TDRE_MASK)
#define LPUART_STAT_RAF_MASK                     (0x1000000U)
#define LPUART_STAT_RAF_SHIFT                    (24U)
#define LPUART_STAT_RAF(x)                       (((uint32_t)(((uint32_t)(x)) << LPUART_STAT_RAF_SHIFT)) & LPUART_STAT_RAF_MASK)
#define LPUART_STAT_LBKDE_MASK                   (0x2000000U)
#define LPUART_STAT_LBKDE_SHIFT                  (25U)
#define LPUART_STAT_LBKDE(x)                     (((uint32_t)(((uint32_t)(x)) << LPUART_STAT_LBKDE_SHIFT)) & LPUART_STAT_LBKDE_MASK)
#define LPUART_STAT_BRK13_MASK                   (0x4000000U)
#define LPUART_STAT_BRK13_SHIFT                  (26U)
#define LPUART_STAT_BRK13(x)                     (((uint32_t)(((uint32_t)(x)) << LPUART_STAT_BRK13_SHIFT)) & LPUART_STAT_BRK13_MASK)
#define LPUART_STAT_RWUID_MASK                   (0x8000000U)
#define LPUART_STAT_RWUID_SHIFT                  (27U)
#define LPUART_STAT_RWUID(x)                     (((uint32_t)(((uint32_t)(x)) << LPUART_STAT_RWUID_SHIFT)) & LPUART_STAT_RWUID_MASK)
#define LPUART_STAT_RXINV_MASK                   (0x10000000U)
#define LPUART_STAT_RXINV_SHIFT                  (28U)
#define LPUART_STAT_RXINV(x)                     (((uint32_t)(((uint32_t)(x)) << LPUART_STAT_RXINV_SHIFT)) & LPUART_STAT_RXINV_MASK)
#define LPUART_STAT_MSBF_MASK                    (0x20000000U)
#define LPUART_STAT_MSBF_SHIFT                   (29U)
#define LPUART_STAT_MSBF(x)                      (((uint32_t)(((uint32_t)(x)) << LPUART_STAT_MSBF_SHIFT)) & LPUART_STAT_MSBF_MASK)
#define LPUART_STAT_RXEDGIF_MASK                 (0x40000000U)
#define LPUART_STAT_RXEDGIF_SHIFT                (30U)
#define LPUART_STAT_RXEDGIF(x)                   (((uint32_t)(((uint32_t)(x)) << LPUART_STAT_RXEDGIF_SHIFT)) & LPUART_STAT_RXEDGIF_MASK)
#define LPUART_STAT_LBKDIF_MASK                  (0x80000000U)
#define LPUART_STAT_LBKDIF_SHIFT                 (31U)
#define LPUART_STAT_LBKDIF(x)                    (((uint32_t)(((uint32_t)(x)) << LPUART_STAT_LBKDIF_SHIFT)) & LPUART_STAT_LBKDIF_MASK)

/*! @name CTRL - LPUART Control Register */
#define LPUART_CTRL_PT_MASK                      (0x1U)
#define LPUART_CTRL_PT_SHIFT                     (0U)
#define LPUART_CTRL_PT(x)                        (((uint32_t)(((uint32_t)(x)) << LPUART_CTRL_PT_SHIFT)) & LPUART_CTRL_PT_MASK)
#define LPUART_CTRL_PE_MASK                      (0x2U)
#define LPUART_CTRL_PE_SHIFT                     (1U)
#define LPUART_CTRL_PE(x)                        (((uint32_t)(((uint32_t)(x)) << LPUART_CTRL_PE_SHIFT)) & LPUART_CTRL_PE_MASK)
#define LPUART_CTRL_ILT_MASK                     (0x4U)
#define LPUART_CTRL_ILT_SHIFT                    (2U)
#define LPUART_CTRL_ILT(x)                       (((uint32_t)(((uint32_t)(x)) << LPUART_CTRL_ILT_SHIFT)) & LPUART_CTRL_ILT_MASK)
#define LPUART_CTRL_WAKE_MASK                    (0x8U)
#define LPUART_CTRL_WAKE_SHIFT                   (3U)
#define LPUART_CTRL_WAKE(x)                      (((uint32_t)(((uint32_t)(x)) << LPUART_CTRL_WAKE_SHIFT)) & LPUART_CTRL_WAKE_MASK)
#define LPUART_CTRL_M_MASK                       (0x10U)
#define LPUART_CTRL_M_SHIFT                      (4U)
#define LPUART_CTRL_M(x)                         (((uint32_t)(((uint32_t)(x)) << LPUART_CTRL_M_SHIFT)) & LPUART_CTRL_M_MASK)
#define LPUART_CTRL_RSRC_MASK                    (0x20U)
#define LPUART_CTRL_RSRC_SHIFT                   (5U)
#define LPUART_CTRL_RSRC(x)                      (((uint32_t)(((uint32_t)(x)) << LPUART_CTRL_RSRC_SHIFT)) & LPUART_CTRL_RSRC_MASK)
#define LPUART_CTRL_DOZEEN_MASK                  (0x40U)
#define LPUART_CTRL_DOZEEN_SHIFT                 (6U)
#define LPUART_CTRL_DOZEEN(x)                    (((uint32_t)(((uint32_t)(x)) << LPUART_CTRL_DOZEEN_SHIFT)) & LPUART_CTRL_DOZEEN_MASK)
#define LPUART_CTRL_LOOPS_MASK                   (0x80U)
#define LPUART_CTRL_LOOPS_SHIFT                  (7U)
#define LPUART_CTRL_LOOPS(x)                     (((uint32_t)(((uint32_t)(x)) << LPUART_CTRL_LOOPS_SHIFT)) & LPUART_CTRL_LOOPS_MASK)
#define LPUART_CTRL_IDLECFG_MASK                 (0x700U)
#define LPUART_CTRL_IDLECFG_SHIFT                (8U)
#define LPUART_CTRL_IDLECFG(x)                   (((uint32_t)(((uint32_t)(x)) << LPUART_CTRL_IDLECFG_SHIFT)) & LPUART_CTRL_IDLECFG_MASK)
#define LPUART_CTRL_MA2IE_MASK                   (0x4000U)
#define LPUART_CTRL_MA2IE_SHIFT                  (14U)
#define LPUART_CTRL_MA2IE(x)                     (((uint32_t)(((uint32_t)(x)) << LPUART_CTRL_MA2IE_SHIFT)) & LPUART_CTRL_MA2IE_MASK)
#define LPUART_CTRL_MA1IE_MASK                   (0x8000U)
#define LPUART_CTRL_MA1IE_SHIFT                  (15U)
#define LPUART_CTRL_MA1IE(x)                     (((uint32_t)(((uint32_t)(x)) << LPUART_CTRL_MA1IE_SHIFT)) & LPUART_CTRL_MA1IE_MASK)
#define LPUART_CTRL_SBK_MASK                     (0x10000U)
#define LPUART_CTRL_SBK_SHIFT                    (16U)
#define LPUART_CTRL_SBK(x)                       (((uint32_t)(((uint32_t)(x)) << LPUART_CTRL_SBK_SHIFT)) & LPUART_CTRL_SBK_MASK)
#define LPUART_CTRL_RWU_MASK                     (0x20000U)
#define LPUART_CTRL_RWU_SHIFT                    (17U)
#define LPUART_CTRL_RWU(x)                       (((uint32_t)(((uint32_t)(x)) << LPUART_CTRL_RWU_SHIFT)) & LPUART_CTRL_RWU_MASK)
#define LPUART_CTRL_RE_MASK                      (0x40000U)
#define LPUART_CTRL_RE_SHIFT                     (18U)
#define LPUART_CTRL_RE(x)                        (((uint32_t)(((uint32_t)(x)) << LPUART_CTRL_RE_SHIFT)) & LPUART_CTRL_RE_MASK)
#define LPUART_CTRL_TE_MASK                      (0x80000U)
#define LPUART_CTRL_TE_SHIFT                     (19U)
#define LPUART_CTRL_TE(x)                        (((uint32_t)(((uint32_t)(x)) << LPUART_CTRL_TE_SHIFT)) & LPUART_CTRL_TE_MASK)
#define LPUART_CTRL_ILIE_MASK                    (0x100000U)
#define LPUART_CTRL_ILIE_SHIFT                   (20U)
#define LPUART_CTRL_ILIE(x)                      (((uint32_t)(((uint32_t)(x)) << LPUART_CTRL_ILIE_SHIFT)) & LPUART_CTRL_ILIE_MASK)
#define LPUART_CTRL_RIE_MASK                     (0x200000U)
#define LPUART_CTRL_RIE_SHIFT                    (21U)
#define LPUART_CTRL_RIE(x)                       (((uint32_t)(((uint32_t)(x)) << LPUART_CTRL_RIE_SHIFT)) & LPUART_CTRL_RIE_MASK)
#define LPUART_CTRL_TCIE_MASK                    (0x400000U)
#define LPUART_CTRL_TCIE_SHIFT                   (22U)
#define LPUART_CTRL_TCIE(x)                      (((uint32_t)(((uint32_t)(x)) << LPUART_CTRL_TCIE_SHIFT)) & LPUART_CTRL_TCIE_MASK)
#define LPUART_CTRL_TIE_MASK                     (0x800000U)
#define LPUART_CTRL_TIE_SHIFT                    (23U)
#define LPUART_CTRL_TIE(x)                       (((uint32_t)(((uint32_t)(x)) << LPUART_CTRL_TIE_SHIFT)) & LPUART_CTRL_TIE_MASK)
#define LPUART_CTRL_PEIE_MASK                    (0x1000000U)
#define LPUART_CTRL_PEIE_SHIFT                   (24U)
#define LPUART_CTRL_PEIE(x)                      (((uint32_t)(((uint32_t)(x)) << LPUART_CTRL_PEIE_SHIFT)) & LPUART_CTRL_PEIE_MASK)
#define LPUART_CTRL_FEIE_MASK                    (0x2000000U)
#define LPUART_CTRL_FEIE_SHIFT                   (25U)
#define LPUART_CTRL_FEIE(x)                      (((uint32_t)(((uint32_t)(x)) << LPUART_CTRL_FEIE_SHIFT)) & LPUART_CTRL_FEIE_MASK)
#define LPUART_CTRL_NEIE_MASK                    (0x4000000U)
#define LPUART_CTRL_NEIE_SHIFT                   (26U)
#define LPUART_CTRL_NEIE(x)                      (((uint32_t)(((uint32_t)(x)) << LPUART_CTRL_NEIE_SHIFT)) & LPUART_CTRL_NEIE_MASK)
#define LPUART_CTRL_ORIE_MASK                    (0x8000000U)
#define LPUART_CTRL_ORIE_SHIFT                   (27U)
#define LPUART_CTRL_ORIE(x)                      (((uint32_t)(((uint32_t)(x)) << LPUART_CTRL_ORIE_SHIFT)) & LPUART_CTRL_ORIE_MASK)
#define LPUART_CTRL_TXINV_MASK                   (0x10000000U)
#define LPUART_CTRL_TXINV_SHIFT                  (28U)
#define LPUART_CTRL_TXINV(x)                     (((uint32_t)(((uint32_t)(x)) << LPUART_CTRL_TXINV_SHIFT)) & LPUART_CTRL_TXINV_MASK)
#define LPUART_CTRL_TXDIR_MASK                   (0x20000000U)
#define LPUART_CTRL_TXDIR_SHIFT                  (29U)
#define LPUART_CTRL_TXDIR(x)                     (((uint32_t)(((uint32_t)(x)) << LPUART_CTRL_TXDIR_SHIFT)) & LPUART_CTRL_TXDIR_MASK)
#define LPUART_CTRL_R9T8_MASK                    (0x40000000U)
#define LPUART_CTRL_R9T8_SHIFT                   (30U)
#define LPUART_CTRL_R9T8(x)                      (((uint32_t)(((uint32_t)(x)) << LPUART_CTRL_R9T8_SHIFT)) & LPUART_CTRL_R9T8_MASK)
#define LPUART_CTRL_R8T9_MASK                    (0x80000000U)
#define LPUART_CTRL_R8T9_SHIFT                   (31U)
#define LPUART_CTRL_R8T9(x)                      (((uint32_t)(((uint32_t)(x)) << LPUART_CTRL_R8T9_SHIFT)) & LPUART_CTRL_R8T9_MASK)

/*! @name DATA - LPUART Data Register */
#define LPUART_DATA_R0T0_MASK                    (0x1U)
#define LPUART_DATA_R0T0_SHIFT                   (0U)
#define LPUART_DATA_R0T0(x)                      (((uint32_t)(((uint32_t)(x)) << LPUART_DATA_R0T0_SHIFT)) & LPUART_DATA_R0T0_MASK)
#define LPUART_DATA_R1T1_MASK                    (0x2U)
#define LPUART_DATA_R1T1_SHIFT                   (1U)
#define LPUART_DATA_R1T1(x)                      (((uint32_t)(((uint32_t)(x)) << LPUART_DATA_R1T1_SHIFT)) & LPUART_DATA_R1T1_MASK)
#define LPUART_DATA_R2T2_MASK                    (0x4U)
#define LPUART_DATA_R2T2_SHIFT                   (2U)
#define LPUART_DATA_R2T2(x)                      (((uint32_t)(((uint32_t)(x)) << LPUART_DATA_R2T2_SHIFT)) & LPUART_DATA_R2T2_MASK)
#define LPUART_DATA_R3T3_MASK                    (0x8U)
#define LPUART_DATA_R3T3_SHIFT                   (3U)
#define LPUART_DATA_R3T3(x)                      (((uint32_t)(((uint32_t)(x)) << LPUART_DATA_R3T3_SHIFT)) & LPUART_DATA_R3T3_MASK)
#define LPUART_DATA_R4T4_MASK                    (0x10U)
#define LPUART_DATA_R4T4_SHIFT                   (4U)
#define LPUART_DATA_R4T4(x)                      (((uint32_t)(((uint32_t)(x)) << LPUART_DATA_R4T4_SHIFT)) & LPUART_DATA_R4T4_MASK)
#define LPUART_DATA_R5T5_MASK                    (0x20U)
#define LPUART_DATA_R5T5_SHIFT                   (5U)
#define LPUART_DATA_R5T5(x)                      (((uint32_t)(((uint32_t)(x)) << LPUART_DATA_R5T5_SHIFT)) & LPUART_DATA_R5T5_MASK)
#define LPUART_DATA_R6T6_MASK                    (0x40U)
#define LPUART_DATA_R6T6_SHIFT                   (6U)
#define LPUART_DATA_R6T6(x)                      (((uint32_t)(((uint32_t)(x)) << LPUART_DATA_R6T6_SHIFT)) & LPUART_DATA_R6T6_MASK)
#define LPUART_DATA_R7T7_MASK                    (0x80U)
#define LPUART_DATA_R7T7_SHIFT                   (7U)
#define LPUART_DATA_R7T7(x)                      (((uint32_t)(((uint32_t)(x)) << LPUART_DATA_R7T7_SHIFT)) & LPUART_DATA_R7T7_MASK)
#define LPUART_DATA_R8T8_MASK                    (0x100U)
#define LPUART_DATA_R8T8_SHIFT                   (8U)
#define LPUART_DATA_R8T8(x)                      (((uint32_t)(((uint32_t)(x)) << LPUART_DATA_R8T8_SHIFT)) & LPUART_DATA_R8T8_MASK)
#define LPUART_DATA_R9T9_MASK                    (0x200U)
#define LPUART_DATA_R9T9_SHIFT                   (9U)
#define LPUART_DATA_R9T9(x)                      (((uint32_t)(((uint32_t)(x)) << LPUART_DATA_R9T9_SHIFT)) & LPUART_DATA_R9T9_MASK)
#define LPUART_DATA_IDLINE_MASK                  (0x800U)
#define LPUART_DATA_IDLINE_SHIFT                 (11U)
#define LPUART_DATA_IDLINE(x)                    (((uint32_t)(((uint32_t)(x)) << LPUART_DATA_IDLINE_SHIFT)) & LPUART_DATA_IDLINE_MASK)
#define LPUART_DATA_RXEMPT_MASK                  (0x1000U)
#define LPUART_DATA_RXEMPT_SHIFT                 (12U)
#define LPUART_DATA_RXEMPT(x)                    (((uint32_t)(((uint32_t)(x)) << LPUART_DATA_RXEMPT_SHIFT)) & LPUART_DATA_RXEMPT_MASK)
#define LPUART_DATA_FRETSC_MASK                  (0x2000U)
#define LPUART_DATA_FRETSC_SHIFT                 (13U)
#define LPUART_DATA_FRETSC(x)                    (((uint32_t)(((uint32_t)(x)) << LPUART_DATA_FRETSC_SHIFT)) & LPUART_DATA_FRETSC_MASK)
#define LPUART_DATA_PARITYE_MASK                 (0x4000U)
#define LPUART_DATA_PARITYE_SHIFT                (14U)
#define LPUART_DATA_PARITYE(x)                   (((uint32_t)(((uint32_t)(x)) << LPUART_DATA_PARITYE_SHIFT)) & LPUART_DATA_PARITYE_MASK)
#define LPUART_DATA_NOISY_MASK                   (0x8000U)
#define LPUART_DATA_NOISY_SHIFT                  (15U)
#define LPUART_DATA_NOISY(x)                     (((uint32_t)(((uint32_t)(x)) << LPUART_DATA_NOISY_SHIFT)) & LPUART_DATA_NOISY_MASK)

/*! @name MATCH - LPUART Match Address Register */
#define LPUART_MATCH_MA1_MASK                    (0x3FFU)
#define LPUART_MATCH_MA1_SHIFT                   (0U)
#define LPUART_MATCH_MA1(x)                      (((uint32_t)(((uint32_t)(x)) << LPUART_MATCH_MA1_SHIFT)) & LPUART_MATCH_MA1_MASK)
#define LPUART_MATCH_MA2_MASK                    (0x3FF0000U)
#define LPUART_MATCH_MA2_SHIFT                   (16U)
#define LPUART_MATCH_MA2(x)                      (((uint32_t)(((uint32_t)(x)) << LPUART_MATCH_MA2_SHIFT)) & LPUART_MATCH_MA2_MASK)

/*! @name MODIR - LPUART Modem IrDA Register */
#define LPUART_MODIR_TXCTSE_MASK                 (0x1U)
#define LPUART_MODIR_TXCTSE_SHIFT                (0U)
#define LPUART_MODIR_TXCTSE(x)                   (((uint32_t)(((uint32_t)(x)) << LPUART_MODIR_TXCTSE_SHIFT)) & LPUART_MODIR_TXCTSE_MASK)
#define LPUART_MODIR_TXRTSE_MASK                 (0x2U)
#define LPUART_MODIR_TXRTSE_SHIFT                (1U)
#define LPUART_MODIR_TXRTSE(x)                   (((uint32_t)(((uint32_t)(x)) << LPUART_MODIR_TXRTSE_SHIFT)) & LPUART_MODIR_TXRTSE_MASK)
#define LPUART_MODIR_TXRTSPOL_MASK               (0x4U)
#define LPUART_MODIR_TXRTSPOL_SHIFT              (2U)
#define LPUART_MODIR_TXRTSPOL(x)                 (((uint32_t)(((uint32_t)(x)) << LPUART_MODIR_TXRTSPOL_SHIFT)) & LPUART_MODIR_TXRTSPOL_MASK)
#define LPUART_MODIR_RXRTSE_MASK                 (0x8U)
#define LPUART_MODIR_RXRTSE_SHIFT                (3U)
#define LPUART_MODIR_RXRTSE(x)                   (((uint32_t)(((uint32_t)(x)) << LPUART_MODIR_RXRTSE_SHIFT)) & LPUART_MODIR_RXRTSE_MASK)
#define LPUART_MODIR_TXCTSC_MASK                 (0x10U)
#define LPUART_MODIR_TXCTSC_SHIFT                (4U)
#define LPUART_MODIR_TXCTSC(x)                   (((uint32_t)(((uint32_t)(x)) << LPUART_MODIR_TXCTSC_SHIFT)) & LPUART_MODIR_TXCTSC_MASK)
#define LPUART_MODIR_TXCTSSRC_MASK               (0x20U)
#define LPUART_MODIR_TXCTSSRC_SHIFT              (5U)
#define LPUART_MODIR_TXCTSSRC(x)                 (((uint32_t)(((uint32_t)(x)) << LPUART_MODIR_TXCTSSRC_SHIFT)) & LPUART_MODIR_TXCTSSRC_MASK)
#define LPUART_MODIR_TNP_MASK                    (0x30000U)
#define LPUART_MODIR_TNP_SHIFT                   (16U)
#define LPUART_MODIR_TNP(x)                      (((uint32_t)(((uint32_t)(x)) << LPUART_MODIR_TNP_SHIFT)) & LPUART_MODIR_TNP_MASK)
#define LPUART_MODIR_IREN_MASK                   (0x40000U)
#define LPUART_MODIR_IREN_SHIFT                  (18U)
#define LPUART_MODIR_IREN(x)                     (((uint32_t)(((uint32_t)(x)) << LPUART_MODIR_IREN_SHIFT)) & LPUART_MODIR_IREN_MASK)


/*!
 * @}
 */ /* end of group LPUART_Register_Masks */


/* LPUART - Peripheral instance base addresses */
/** Peripheral LPUART0 base address */
#define LPUART0_BASE                             (0x40054000u)
/** Peripheral LPUART0 base pointer */
#define LPUART0                                  ((LPUART_Type *)LPUART0_BASE)
/** Array initializer of LPUART peripheral base addresses */
#define LPUART_BASE_ADDRS                        { LPUART0_BASE }
/** Array initializer of LPUART peripheral base pointers */
#define LPUART_BASE_PTRS                         { LPUART0 }
/** Interrupt vectors for the LPUART peripheral type */
#define LPUART_RX_TX_IRQS                        { LPUART0_IRQn }
#define LPUART_ERR_IRQS                          { LPUART0_IRQn }

/*!
 * @}
 */ /* end of group LPUART_Peripheral_Access_Layer */


/* ----------------------------------------------------------------------------
   -- LTC Peripheral Access Layer
   ---------------------------------------------------------------------------- */

/*!
 * @addtogroup LTC_Peripheral_Access_Layer LTC Peripheral Access Layer
 * @{
 */

/** LTC - Register Layout Typedef */
typedef struct {
  __IO uint32_t MD;                                /**< Mode Register, offset: 0x0 */
       uint8_t RESERVED_0[4];
  __IO uint32_t KS;                                /**< Key Size Register, offset: 0x8 */
       uint8_t RESERVED_1[4];
  __IO uint32_t DS;                                /**< Data Size Register, offset: 0x10 */
       uint8_t RESERVED_2[4];
  __IO uint32_t ICVS;                              /**< ICV Size Register, offset: 0x18 */
       uint8_t RESERVED_3[20];
  __IO uint32_t COM;                               /**< Command Register, offset: 0x30 */
  __IO uint32_t CTL;                               /**< Control Register, offset: 0x34 */
       uint8_t RESERVED_4[8];
  __IO uint32_t CW;                                /**< Clear Written Register, offset: 0x40 */
       uint8_t RESERVED_5[4];
  __IO uint32_t STA;                               /**< Status Register, offset: 0x48 */
  __I  uint32_t ESTA;                              /**< Error Status Register, offset: 0x4C */
       uint8_t RESERVED_6[8];
  __IO uint32_t AADSZ;                             /**< AAD Size Register, offset: 0x58 */
       uint8_t RESERVED_7[164];
  __IO uint32_t CTX[14];                           /**< Context Register, array offset: 0x100, array step: 0x4 */
       uint8_t RESERVED_8[200];
  __IO uint32_t KEY[4];                            /**< Key Registers, array offset: 0x200, array step: 0x4 */
       uint8_t RESERVED_9[736];
  __I  uint32_t VID1;                              /**< Version ID Register, offset: 0x4F0 */
  __I  uint32_t VID2;                              /**< Version ID 2 Register, offset: 0x4F4 */
  __I  uint32_t CHAVID;                            /**< CHA Version ID Register, offset: 0x4F8 */
       uint8_t RESERVED_10[708];
  __I  uint32_t FIFOSTA;                           /**< FIFO Status Register, offset: 0x7C0 */
       uint8_t RESERVED_11[28];
  __O  uint32_t IFIFO;                             /**< Input Data FIFO, offset: 0x7E0 */
       uint8_t RESERVED_12[12];
  __I  uint32_t OFIFO;                             /**< Output Data FIFO, offset: 0x7F0 */
} LTC_Type;

/* ----------------------------------------------------------------------------
   -- LTC Register Masks
   ---------------------------------------------------------------------------- */

/*!
 * @addtogroup LTC_Register_Masks LTC Register Masks
 * @{
 */

/*! @name MD - Mode Register */
#define LTC_MD_ENC_MASK                          (0x1U)
#define LTC_MD_ENC_SHIFT                         (0U)
#define LTC_MD_ENC(x)                            (((uint32_t)(((uint32_t)(x)) << LTC_MD_ENC_SHIFT)) & LTC_MD_ENC_MASK)
#define LTC_MD_ICV_TEST_MASK                     (0x2U)
#define LTC_MD_ICV_TEST_SHIFT                    (1U)
#define LTC_MD_ICV_TEST(x)                       (((uint32_t)(((uint32_t)(x)) << LTC_MD_ICV_TEST_SHIFT)) & LTC_MD_ICV_TEST_MASK)
#define LTC_MD_AS_MASK                           (0xCU)
#define LTC_MD_AS_SHIFT                          (2U)
#define LTC_MD_AS(x)                             (((uint32_t)(((uint32_t)(x)) << LTC_MD_AS_SHIFT)) & LTC_MD_AS_MASK)
#define LTC_MD_AAI_MASK                          (0x1FF0U)
#define LTC_MD_AAI_SHIFT                         (4U)
#define LTC_MD_AAI(x)                            (((uint32_t)(((uint32_t)(x)) << LTC_MD_AAI_SHIFT)) & LTC_MD_AAI_MASK)
#define LTC_MD_ALG_MASK                          (0xFF0000U)
#define LTC_MD_ALG_SHIFT                         (16U)
#define LTC_MD_ALG(x)                            (((uint32_t)(((uint32_t)(x)) << LTC_MD_ALG_SHIFT)) & LTC_MD_ALG_MASK)

/*! @name KS - Key Size Register */
#define LTC_KS_KS_MASK                           (0x1FU)
#define LTC_KS_KS_SHIFT                          (0U)
#define LTC_KS_KS(x)                             (((uint32_t)(((uint32_t)(x)) << LTC_KS_KS_SHIFT)) & LTC_KS_KS_MASK)

/*! @name DS - Data Size Register */
#define LTC_DS_DS_MASK                           (0xFFFU)
#define LTC_DS_DS_SHIFT                          (0U)
#define LTC_DS_DS(x)                             (((uint32_t)(((uint32_t)(x)) << LTC_DS_DS_SHIFT)) & LTC_DS_DS_MASK)

/*! @name ICVS - ICV Size Register */
#define LTC_ICVS_ICVS_MASK                       (0x1FU)
#define LTC_ICVS_ICVS_SHIFT                      (0U)
#define LTC_ICVS_ICVS(x)                         (((uint32_t)(((uint32_t)(x)) << LTC_ICVS_ICVS_SHIFT)) & LTC_ICVS_ICVS_MASK)

/*! @name COM - Command Register */
#define LTC_COM_ALL_MASK                         (0x1U)
#define LTC_COM_ALL_SHIFT                        (0U)
#define LTC_COM_ALL(x)                           (((uint32_t)(((uint32_t)(x)) << LTC_COM_ALL_SHIFT)) & LTC_COM_ALL_MASK)
#define LTC_COM_AES_MASK                         (0x2U)
#define LTC_COM_AES_SHIFT                        (1U)
#define LTC_COM_AES(x)                           (((uint32_t)(((uint32_t)(x)) << LTC_COM_AES_SHIFT)) & LTC_COM_AES_MASK)

/*! @name CTL - Control Register */
#define LTC_CTL_IM_MASK                          (0x1U)
#define LTC_CTL_IM_SHIFT                         (0U)
#define LTC_CTL_IM(x)                            (((uint32_t)(((uint32_t)(x)) << LTC_CTL_IM_SHIFT)) & LTC_CTL_IM_MASK)
#define LTC_CTL_IFE_MASK                         (0x100U)
#define LTC_CTL_IFE_SHIFT                        (8U)
#define LTC_CTL_IFE(x)                           (((uint32_t)(((uint32_t)(x)) << LTC_CTL_IFE_SHIFT)) & LTC_CTL_IFE_MASK)
#define LTC_CTL_IFR_MASK                         (0x200U)
#define LTC_CTL_IFR_SHIFT                        (9U)
#define LTC_CTL_IFR(x)                           (((uint32_t)(((uint32_t)(x)) << LTC_CTL_IFR_SHIFT)) & LTC_CTL_IFR_MASK)
#define LTC_CTL_OFE_MASK                         (0x1000U)
#define LTC_CTL_OFE_SHIFT                        (12U)
#define LTC_CTL_OFE(x)                           (((uint32_t)(((uint32_t)(x)) << LTC_CTL_OFE_SHIFT)) & LTC_CTL_OFE_MASK)
#define LTC_CTL_OFR_MASK                         (0x2000U)
#define LTC_CTL_OFR_SHIFT                        (13U)
#define LTC_CTL_OFR(x)                           (((uint32_t)(((uint32_t)(x)) << LTC_CTL_OFR_SHIFT)) & LTC_CTL_OFR_MASK)
#define LTC_CTL_IFS_MASK                         (0x10000U)
#define LTC_CTL_IFS_SHIFT                        (16U)
#define LTC_CTL_IFS(x)                           (((uint32_t)(((uint32_t)(x)) << LTC_CTL_IFS_SHIFT)) & LTC_CTL_IFS_MASK)
#define LTC_CTL_OFS_MASK                         (0x20000U)
#define LTC_CTL_OFS_SHIFT                        (17U)
#define LTC_CTL_OFS(x)                           (((uint32_t)(((uint32_t)(x)) << LTC_CTL_OFS_SHIFT)) & LTC_CTL_OFS_MASK)
#define LTC_CTL_KIS_MASK                         (0x100000U)
#define LTC_CTL_KIS_SHIFT                        (20U)
#define LTC_CTL_KIS(x)                           (((uint32_t)(((uint32_t)(x)) << LTC_CTL_KIS_SHIFT)) & LTC_CTL_KIS_MASK)
#define LTC_CTL_KOS_MASK                         (0x200000U)
#define LTC_CTL_KOS_SHIFT                        (21U)
#define LTC_CTL_KOS(x)                           (((uint32_t)(((uint32_t)(x)) << LTC_CTL_KOS_SHIFT)) & LTC_CTL_KOS_MASK)
#define LTC_CTL_CIS_MASK                         (0x400000U)
#define LTC_CTL_CIS_SHIFT                        (22U)
#define LTC_CTL_CIS(x)                           (((uint32_t)(((uint32_t)(x)) << LTC_CTL_CIS_SHIFT)) & LTC_CTL_CIS_MASK)
#define LTC_CTL_COS_MASK                         (0x800000U)
#define LTC_CTL_COS_SHIFT                        (23U)
#define LTC_CTL_COS(x)                           (((uint32_t)(((uint32_t)(x)) << LTC_CTL_COS_SHIFT)) & LTC_CTL_COS_MASK)
#define LTC_CTL_KAL_MASK                         (0x80000000U)
#define LTC_CTL_KAL_SHIFT                        (31U)
#define LTC_CTL_KAL(x)                           (((uint32_t)(((uint32_t)(x)) << LTC_CTL_KAL_SHIFT)) & LTC_CTL_KAL_MASK)

/*! @name CW - Clear Written Register */
#define LTC_CW_CM_MASK                           (0x1U)
#define LTC_CW_CM_SHIFT                          (0U)
#define LTC_CW_CM(x)                             (((uint32_t)(((uint32_t)(x)) << LTC_CW_CM_SHIFT)) & LTC_CW_CM_MASK)
#define LTC_CW_CDS_MASK                          (0x4U)
#define LTC_CW_CDS_SHIFT                         (2U)
#define LTC_CW_CDS(x)                            (((uint32_t)(((uint32_t)(x)) << LTC_CW_CDS_SHIFT)) & LTC_CW_CDS_MASK)
#define LTC_CW_CICV_MASK                         (0x8U)
#define LTC_CW_CICV_SHIFT                        (3U)
#define LTC_CW_CICV(x)                           (((uint32_t)(((uint32_t)(x)) << LTC_CW_CICV_SHIFT)) & LTC_CW_CICV_MASK)
#define LTC_CW_CCR_MASK                          (0x20U)
#define LTC_CW_CCR_SHIFT                         (5U)
#define LTC_CW_CCR(x)                            (((uint32_t)(((uint32_t)(x)) << LTC_CW_CCR_SHIFT)) & LTC_CW_CCR_MASK)
#define LTC_CW_CKR_MASK                          (0x40U)
#define LTC_CW_CKR_SHIFT                         (6U)
#define LTC_CW_CKR(x)                            (((uint32_t)(((uint32_t)(x)) << LTC_CW_CKR_SHIFT)) & LTC_CW_CKR_MASK)
#define LTC_CW_COF_MASK                          (0x40000000U)
#define LTC_CW_COF_SHIFT                         (30U)
#define LTC_CW_COF(x)                            (((uint32_t)(((uint32_t)(x)) << LTC_CW_COF_SHIFT)) & LTC_CW_COF_MASK)
#define LTC_CW_CIF_MASK                          (0x80000000U)
#define LTC_CW_CIF_SHIFT                         (31U)
#define LTC_CW_CIF(x)                            (((uint32_t)(((uint32_t)(x)) << LTC_CW_CIF_SHIFT)) & LTC_CW_CIF_MASK)

/*! @name STA - Status Register */
#define LTC_STA_AB_MASK                          (0x2U)
#define LTC_STA_AB_SHIFT                         (1U)
#define LTC_STA_AB(x)                            (((uint32_t)(((uint32_t)(x)) << LTC_STA_AB_SHIFT)) & LTC_STA_AB_MASK)
#define LTC_STA_DI_MASK                          (0x10000U)
#define LTC_STA_DI_SHIFT                         (16U)
#define LTC_STA_DI(x)                            (((uint32_t)(((uint32_t)(x)) << LTC_STA_DI_SHIFT)) & LTC_STA_DI_MASK)
#define LTC_STA_EI_MASK                          (0x100000U)
#define LTC_STA_EI_SHIFT                         (20U)
#define LTC_STA_EI(x)                            (((uint32_t)(((uint32_t)(x)) << LTC_STA_EI_SHIFT)) & LTC_STA_EI_MASK)

/*! @name ESTA - Error Status Register */
#define LTC_ESTA_ERRID1_MASK                     (0xFU)
#define LTC_ESTA_ERRID1_SHIFT                    (0U)
#define LTC_ESTA_ERRID1(x)                       (((uint32_t)(((uint32_t)(x)) << LTC_ESTA_ERRID1_SHIFT)) & LTC_ESTA_ERRID1_MASK)
#define LTC_ESTA_CL1_MASK                        (0xF00U)
#define LTC_ESTA_CL1_SHIFT                       (8U)
#define LTC_ESTA_CL1(x)                          (((uint32_t)(((uint32_t)(x)) << LTC_ESTA_CL1_SHIFT)) & LTC_ESTA_CL1_MASK)

/*! @name AADSZ - AAD Size Register */
#define LTC_AADSZ_AADSZ_MASK                     (0xFU)
#define LTC_AADSZ_AADSZ_SHIFT                    (0U)
#define LTC_AADSZ_AADSZ(x)                       (((uint32_t)(((uint32_t)(x)) << LTC_AADSZ_AADSZ_SHIFT)) & LTC_AADSZ_AADSZ_MASK)
#define LTC_AADSZ_AL_MASK                        (0x80000000U)
#define LTC_AADSZ_AL_SHIFT                       (31U)
#define LTC_AADSZ_AL(x)                          (((uint32_t)(((uint32_t)(x)) << LTC_AADSZ_AL_SHIFT)) & LTC_AADSZ_AL_MASK)

/*! @name CTX - Context Register */
#define LTC_CTX_CTX_MASK                         (0xFFFFFFFFU)
#define LTC_CTX_CTX_SHIFT                        (0U)
#define LTC_CTX_CTX(x)                           (((uint32_t)(((uint32_t)(x)) << LTC_CTX_CTX_SHIFT)) & LTC_CTX_CTX_MASK)

/* The count of LTC_CTX */
#define LTC_CTX_COUNT                            (14U)

/*! @name KEY - Key Registers */
#define LTC_KEY_KEY_MASK                         (0xFFFFFFFFU)
#define LTC_KEY_KEY_SHIFT                        (0U)
#define LTC_KEY_KEY(x)                           (((uint32_t)(((uint32_t)(x)) << LTC_KEY_KEY_SHIFT)) & LTC_KEY_KEY_MASK)

/* The count of LTC_KEY */
#define LTC_KEY_COUNT                            (4U)

/*! @name VID1 - Version ID Register */
#define LTC_VID1_MIN_REV_MASK                    (0xFFU)
#define LTC_VID1_MIN_REV_SHIFT                   (0U)
#define LTC_VID1_MIN_REV(x)                      (((uint32_t)(((uint32_t)(x)) << LTC_VID1_MIN_REV_SHIFT)) & LTC_VID1_MIN_REV_MASK)
#define LTC_VID1_MAJ_REV_MASK                    (0xFF00U)
#define LTC_VID1_MAJ_REV_SHIFT                   (8U)
#define LTC_VID1_MAJ_REV(x)                      (((uint32_t)(((uint32_t)(x)) << LTC_VID1_MAJ_REV_SHIFT)) & LTC_VID1_MAJ_REV_MASK)
#define LTC_VID1_IP_ID_MASK                      (0xFFFF0000U)
#define LTC_VID1_IP_ID_SHIFT                     (16U)
#define LTC_VID1_IP_ID(x)                        (((uint32_t)(((uint32_t)(x)) << LTC_VID1_IP_ID_SHIFT)) & LTC_VID1_IP_ID_MASK)

/*! @name VID2 - Version ID 2 Register */
#define LTC_VID2_ECO_REV_MASK                    (0xFFU)
#define LTC_VID2_ECO_REV_SHIFT                   (0U)
#define LTC_VID2_ECO_REV(x)                      (((uint32_t)(((uint32_t)(x)) << LTC_VID2_ECO_REV_SHIFT)) & LTC_VID2_ECO_REV_MASK)
#define LTC_VID2_ARCH_ERA_MASK                   (0xFF00U)
#define LTC_VID2_ARCH_ERA_SHIFT                  (8U)
#define LTC_VID2_ARCH_ERA(x)                     (((uint32_t)(((uint32_t)(x)) << LTC_VID2_ARCH_ERA_SHIFT)) & LTC_VID2_ARCH_ERA_MASK)

/*! @name CHAVID - CHA Version ID Register */
#define LTC_CHAVID_AESREV_MASK                   (0xFU)
#define LTC_CHAVID_AESREV_SHIFT                  (0U)
#define LTC_CHAVID_AESREV(x)                     (((uint32_t)(((uint32_t)(x)) << LTC_CHAVID_AESREV_SHIFT)) & LTC_CHAVID_AESREV_MASK)
#define LTC_CHAVID_AESVID_MASK                   (0xF0U)
#define LTC_CHAVID_AESVID_SHIFT                  (4U)
#define LTC_CHAVID_AESVID(x)                     (((uint32_t)(((uint32_t)(x)) << LTC_CHAVID_AESVID_SHIFT)) & LTC_CHAVID_AESVID_MASK)

/*! @name FIFOSTA - FIFO Status Register */
#define LTC_FIFOSTA_IFL_MASK                     (0x7FU)
#define LTC_FIFOSTA_IFL_SHIFT                    (0U)
#define LTC_FIFOSTA_IFL(x)                       (((uint32_t)(((uint32_t)(x)) << LTC_FIFOSTA_IFL_SHIFT)) & LTC_FIFOSTA_IFL_MASK)
#define LTC_FIFOSTA_IFF_MASK                     (0x8000U)
#define LTC_FIFOSTA_IFF_SHIFT                    (15U)
#define LTC_FIFOSTA_IFF(x)                       (((uint32_t)(((uint32_t)(x)) << LTC_FIFOSTA_IFF_SHIFT)) & LTC_FIFOSTA_IFF_MASK)
#define LTC_FIFOSTA_OFL_MASK                     (0x7F0000U)
#define LTC_FIFOSTA_OFL_SHIFT                    (16U)
#define LTC_FIFOSTA_OFL(x)                       (((uint32_t)(((uint32_t)(x)) << LTC_FIFOSTA_OFL_SHIFT)) & LTC_FIFOSTA_OFL_MASK)
#define LTC_FIFOSTA_OFF_MASK                     (0x80000000U)
#define LTC_FIFOSTA_OFF_SHIFT                    (31U)
#define LTC_FIFOSTA_OFF(x)                       (((uint32_t)(((uint32_t)(x)) << LTC_FIFOSTA_OFF_SHIFT)) & LTC_FIFOSTA_OFF_MASK)

/*! @name IFIFO - Input Data FIFO */
#define LTC_IFIFO_IFIFO_MASK                     (0xFFFFFFFFU)
#define LTC_IFIFO_IFIFO_SHIFT                    (0U)
#define LTC_IFIFO_IFIFO(x)                       (((uint32_t)(((uint32_t)(x)) << LTC_IFIFO_IFIFO_SHIFT)) & LTC_IFIFO_IFIFO_MASK)

/*! @name OFIFO - Output Data FIFO */
#define LTC_OFIFO_OFIFO_MASK                     (0xFFFFFFFFU)
#define LTC_OFIFO_OFIFO_SHIFT                    (0U)
#define LTC_OFIFO_OFIFO(x)                       (((uint32_t)(((uint32_t)(x)) << LTC_OFIFO_OFIFO_SHIFT)) & LTC_OFIFO_OFIFO_MASK)


/*!
 * @}
 */ /* end of group LTC_Register_Masks */


/* LTC - Peripheral instance base addresses */
/** Peripheral LTC0 base address */
#define LTC0_BASE                                (0x40058000u)
/** Peripheral LTC0 base pointer */
#define LTC0                                     ((LTC_Type *)LTC0_BASE)
/** Array initializer of LTC peripheral base addresses */
#define LTC_BASE_ADDRS                           { LTC0_BASE }
/** Array initializer of LTC peripheral base pointers */
#define LTC_BASE_PTRS                            { LTC0 }
/** Interrupt vectors for the LTC peripheral type */
#define LTC_IRQS                                 { LTC0_IRQn }

/*!
 * @}
 */ /* end of group LTC_Peripheral_Access_Layer */


/* ----------------------------------------------------------------------------
   -- MCG Peripheral Access Layer
   ---------------------------------------------------------------------------- */

/*!
 * @addtogroup MCG_Peripheral_Access_Layer MCG Peripheral Access Layer
 * @{
 */

/** MCG - Register Layout Typedef */
typedef struct {
  __IO uint8_t C1;                                 /**< MCG Control 1 Register, offset: 0x0 */
  __IO uint8_t C2;                                 /**< MCG Control 2 Register, offset: 0x1 */
  __IO uint8_t C3;                                 /**< MCG Control 3 Register, offset: 0x2 */
  __IO uint8_t C4;                                 /**< MCG Control 4 Register, offset: 0x3 */
  __I  uint8_t C5;                                 /**< MCG Control 5 Register, offset: 0x4 */
  __IO uint8_t C6;                                 /**< MCG Control 6 Register, offset: 0x5 */
  __I  uint8_t S;                                  /**< MCG Status Register, offset: 0x6 */
       uint8_t RESERVED_0[1];
  __IO uint8_t SC;                                 /**< MCG Status and Control Register, offset: 0x8 */
       uint8_t RESERVED_1[1];
  __IO uint8_t ATCVH;                              /**< MCG Auto Trim Compare Value High Register, offset: 0xA */
  __IO uint8_t ATCVL;                              /**< MCG Auto Trim Compare Value Low Register, offset: 0xB */
  __IO uint8_t C7;                                 /**< MCG Control 7 Register, offset: 0xC */
  __IO uint8_t C8;                                 /**< MCG Control 8 Register, offset: 0xD */
} MCG_Type;

/* ----------------------------------------------------------------------------
   -- MCG Register Masks
   ---------------------------------------------------------------------------- */

/*!
 * @addtogroup MCG_Register_Masks MCG Register Masks
 * @{
 */

/*! @name C1 - MCG Control 1 Register */
#define MCG_C1_IREFSTEN_MASK                     (0x1U)
#define MCG_C1_IREFSTEN_SHIFT                    (0U)
#define MCG_C1_IREFSTEN(x)                       (((uint8_t)(((uint8_t)(x)) << MCG_C1_IREFSTEN_SHIFT)) & MCG_C1_IREFSTEN_MASK)
#define MCG_C1_IRCLKEN_MASK                      (0x2U)
#define MCG_C1_IRCLKEN_SHIFT                     (1U)
#define MCG_C1_IRCLKEN(x)                        (((uint8_t)(((uint8_t)(x)) << MCG_C1_IRCLKEN_SHIFT)) & MCG_C1_IRCLKEN_MASK)
#define MCG_C1_IREFS_MASK                        (0x4U)
#define MCG_C1_IREFS_SHIFT                       (2U)
#define MCG_C1_IREFS(x)                          (((uint8_t)(((uint8_t)(x)) << MCG_C1_IREFS_SHIFT)) & MCG_C1_IREFS_MASK)
#define MCG_C1_FRDIV_MASK                        (0x38U)
#define MCG_C1_FRDIV_SHIFT                       (3U)
#define MCG_C1_FRDIV(x)                          (((uint8_t)(((uint8_t)(x)) << MCG_C1_FRDIV_SHIFT)) & MCG_C1_FRDIV_MASK)
#define MCG_C1_CLKS_MASK                         (0xC0U)
#define MCG_C1_CLKS_SHIFT                        (6U)
#define MCG_C1_CLKS(x)                           (((uint8_t)(((uint8_t)(x)) << MCG_C1_CLKS_SHIFT)) & MCG_C1_CLKS_MASK)

/*! @name C2 - MCG Control 2 Register */
#define MCG_C2_IRCS_MASK                         (0x1U)
#define MCG_C2_IRCS_SHIFT                        (0U)
#define MCG_C2_IRCS(x)                           (((uint8_t)(((uint8_t)(x)) << MCG_C2_IRCS_SHIFT)) & MCG_C2_IRCS_MASK)
#define MCG_C2_LP_MASK                           (0x2U)
#define MCG_C2_LP_SHIFT                          (1U)
#define MCG_C2_LP(x)                             (((uint8_t)(((uint8_t)(x)) << MCG_C2_LP_SHIFT)) & MCG_C2_LP_MASK)
#define MCG_C2_EREFS_MASK                        (0x4U)
#define MCG_C2_EREFS_SHIFT                       (2U)
#define MCG_C2_EREFS(x)                          (((uint8_t)(((uint8_t)(x)) << MCG_C2_EREFS_SHIFT)) & MCG_C2_EREFS_MASK)
#define MCG_C2_HGO_MASK                          (0x8U)
#define MCG_C2_HGO_SHIFT                         (3U)
#define MCG_C2_HGO(x)                            (((uint8_t)(((uint8_t)(x)) << MCG_C2_HGO_SHIFT)) & MCG_C2_HGO_MASK)
#define MCG_C2_RANGE_MASK                        (0x30U)
#define MCG_C2_RANGE_SHIFT                       (4U)
#define MCG_C2_RANGE(x)                          (((uint8_t)(((uint8_t)(x)) << MCG_C2_RANGE_SHIFT)) & MCG_C2_RANGE_MASK)
#define MCG_C2_FCFTRIM_MASK                      (0x40U)
#define MCG_C2_FCFTRIM_SHIFT                     (6U)
#define MCG_C2_FCFTRIM(x)                        (((uint8_t)(((uint8_t)(x)) << MCG_C2_FCFTRIM_SHIFT)) & MCG_C2_FCFTRIM_MASK)
#define MCG_C2_LOCRE0_MASK                       (0x80U)
#define MCG_C2_LOCRE0_SHIFT                      (7U)
#define MCG_C2_LOCRE0(x)                         (((uint8_t)(((uint8_t)(x)) << MCG_C2_LOCRE0_SHIFT)) & MCG_C2_LOCRE0_MASK)

/*! @name C3 - MCG Control 3 Register */
#define MCG_C3_SCTRIM_MASK                       (0xFFU)
#define MCG_C3_SCTRIM_SHIFT                      (0U)
#define MCG_C3_SCTRIM(x)                         (((uint8_t)(((uint8_t)(x)) << MCG_C3_SCTRIM_SHIFT)) & MCG_C3_SCTRIM_MASK)

/*! @name C4 - MCG Control 4 Register */
#define MCG_C4_SCFTRIM_MASK                      (0x1U)
#define MCG_C4_SCFTRIM_SHIFT                     (0U)
#define MCG_C4_SCFTRIM(x)                        (((uint8_t)(((uint8_t)(x)) << MCG_C4_SCFTRIM_SHIFT)) & MCG_C4_SCFTRIM_MASK)
#define MCG_C4_FCTRIM_MASK                       (0x1EU)
#define MCG_C4_FCTRIM_SHIFT                      (1U)
#define MCG_C4_FCTRIM(x)                         (((uint8_t)(((uint8_t)(x)) << MCG_C4_FCTRIM_SHIFT)) & MCG_C4_FCTRIM_MASK)
#define MCG_C4_DRST_DRS_MASK                     (0x60U)
#define MCG_C4_DRST_DRS_SHIFT                    (5U)
#define MCG_C4_DRST_DRS(x)                       (((uint8_t)(((uint8_t)(x)) << MCG_C4_DRST_DRS_SHIFT)) & MCG_C4_DRST_DRS_MASK)
#define MCG_C4_DMX32_MASK                        (0x80U)
#define MCG_C4_DMX32_SHIFT                       (7U)
#define MCG_C4_DMX32(x)                          (((uint8_t)(((uint8_t)(x)) << MCG_C4_DMX32_SHIFT)) & MCG_C4_DMX32_MASK)

/*! @name C6 - MCG Control 6 Register */
#define MCG_C6_CME0_MASK                         (0x20U)
#define MCG_C6_CME0_SHIFT                        (5U)
#define MCG_C6_CME0(x)                           (((uint8_t)(((uint8_t)(x)) << MCG_C6_CME0_SHIFT)) & MCG_C6_CME0_MASK)

/*! @name S - MCG Status Register */
#define MCG_S_IRCST_MASK                         (0x1U)
#define MCG_S_IRCST_SHIFT                        (0U)
#define MCG_S_IRCST(x)                           (((uint8_t)(((uint8_t)(x)) << MCG_S_IRCST_SHIFT)) & MCG_S_IRCST_MASK)
#define MCG_S_OSCINIT0_MASK                      (0x2U)
#define MCG_S_OSCINIT0_SHIFT                     (1U)
#define MCG_S_OSCINIT0(x)                        (((uint8_t)(((uint8_t)(x)) << MCG_S_OSCINIT0_SHIFT)) & MCG_S_OSCINIT0_MASK)
#define MCG_S_CLKST_MASK                         (0xCU)
#define MCG_S_CLKST_SHIFT                        (2U)
#define MCG_S_CLKST(x)                           (((uint8_t)(((uint8_t)(x)) << MCG_S_CLKST_SHIFT)) & MCG_S_CLKST_MASK)
#define MCG_S_IREFST_MASK                        (0x10U)
#define MCG_S_IREFST_SHIFT                       (4U)
#define MCG_S_IREFST(x)                          (((uint8_t)(((uint8_t)(x)) << MCG_S_IREFST_SHIFT)) & MCG_S_IREFST_MASK)

/*! @name SC - MCG Status and Control Register */
#define MCG_SC_LOCS0_MASK                        (0x1U)
#define MCG_SC_LOCS0_SHIFT                       (0U)
#define MCG_SC_LOCS0(x)                          (((uint8_t)(((uint8_t)(x)) << MCG_SC_LOCS0_SHIFT)) & MCG_SC_LOCS0_MASK)
#define MCG_SC_FCRDIV_MASK                       (0xEU)
#define MCG_SC_FCRDIV_SHIFT                      (1U)
#define MCG_SC_FCRDIV(x)                         (((uint8_t)(((uint8_t)(x)) << MCG_SC_FCRDIV_SHIFT)) & MCG_SC_FCRDIV_MASK)
#define MCG_SC_FLTPRSRV_MASK                     (0x10U)
#define MCG_SC_FLTPRSRV_SHIFT                    (4U)
#define MCG_SC_FLTPRSRV(x)                       (((uint8_t)(((uint8_t)(x)) << MCG_SC_FLTPRSRV_SHIFT)) & MCG_SC_FLTPRSRV_MASK)
#define MCG_SC_ATMF_MASK                         (0x20U)
#define MCG_SC_ATMF_SHIFT                        (5U)
#define MCG_SC_ATMF(x)                           (((uint8_t)(((uint8_t)(x)) << MCG_SC_ATMF_SHIFT)) & MCG_SC_ATMF_MASK)
#define MCG_SC_ATMS_MASK                         (0x40U)
#define MCG_SC_ATMS_SHIFT                        (6U)
#define MCG_SC_ATMS(x)                           (((uint8_t)(((uint8_t)(x)) << MCG_SC_ATMS_SHIFT)) & MCG_SC_ATMS_MASK)
#define MCG_SC_ATME_MASK                         (0x80U)
#define MCG_SC_ATME_SHIFT                        (7U)
#define MCG_SC_ATME(x)                           (((uint8_t)(((uint8_t)(x)) << MCG_SC_ATME_SHIFT)) & MCG_SC_ATME_MASK)

/*! @name ATCVH - MCG Auto Trim Compare Value High Register */
#define MCG_ATCVH_ATCVH_MASK                     (0xFFU)
#define MCG_ATCVH_ATCVH_SHIFT                    (0U)
#define MCG_ATCVH_ATCVH(x)                       (((uint8_t)(((uint8_t)(x)) << MCG_ATCVH_ATCVH_SHIFT)) & MCG_ATCVH_ATCVH_MASK)

/*! @name ATCVL - MCG Auto Trim Compare Value Low Register */
#define MCG_ATCVL_ATCVL_MASK                     (0xFFU)
#define MCG_ATCVL_ATCVL_SHIFT                    (0U)
#define MCG_ATCVL_ATCVL(x)                       (((uint8_t)(((uint8_t)(x)) << MCG_ATCVL_ATCVL_SHIFT)) & MCG_ATCVL_ATCVL_MASK)

/*! @name C7 - MCG Control 7 Register */
#define MCG_C7_OSCSEL_MASK                       (0x1U)
#define MCG_C7_OSCSEL_SHIFT                      (0U)
#define MCG_C7_OSCSEL(x)                         (((uint8_t)(((uint8_t)(x)) << MCG_C7_OSCSEL_SHIFT)) & MCG_C7_OSCSEL_MASK)

/*! @name C8 - MCG Control 8 Register */
#define MCG_C8_LOCS1_MASK                        (0x1U)
#define MCG_C8_LOCS1_SHIFT                       (0U)
#define MCG_C8_LOCS1(x)                          (((uint8_t)(((uint8_t)(x)) << MCG_C8_LOCS1_SHIFT)) & MCG_C8_LOCS1_MASK)
#define MCG_C8_CME1_MASK                         (0x20U)
#define MCG_C8_CME1_SHIFT                        (5U)
#define MCG_C8_CME1(x)                           (((uint8_t)(((uint8_t)(x)) << MCG_C8_CME1_SHIFT)) & MCG_C8_CME1_MASK)
#define MCG_C8_LOCRE1_MASK                       (0x80U)
#define MCG_C8_LOCRE1_SHIFT                      (7U)
#define MCG_C8_LOCRE1(x)                         (((uint8_t)(((uint8_t)(x)) << MCG_C8_LOCRE1_SHIFT)) & MCG_C8_LOCRE1_MASK)


/*!
 * @}
 */ /* end of group MCG_Register_Masks */


/* MCG - Peripheral instance base addresses */
/** Peripheral MCG base address */
#define MCG_BASE                                 (0x40064000u)
/** Peripheral MCG base pointer */
#define MCG                                      ((MCG_Type *)MCG_BASE)
/** Array initializer of MCG peripheral base addresses */
#define MCG_BASE_ADDRS                           { MCG_BASE }
/** Array initializer of MCG peripheral base pointers */
#define MCG_BASE_PTRS                            { MCG }
/** Interrupt vectors for the MCG peripheral type */
#define MCG_IRQS                                 { MCG_IRQn }

/*!
 * @}
 */ /* end of group MCG_Peripheral_Access_Layer */


/* ----------------------------------------------------------------------------
   -- MCM Peripheral Access Layer
   ---------------------------------------------------------------------------- */

/*!
 * @addtogroup MCM_Peripheral_Access_Layer MCM Peripheral Access Layer
 * @{
 */

/** MCM - Register Layout Typedef */
typedef struct {
       uint8_t RESERVED_0[8];
  __I  uint16_t PLASC;                             /**< Crossbar Switch (AXBS) Slave Configuration, offset: 0x8 */
  __I  uint16_t PLAMC;                             /**< Crossbar Switch (AXBS) Master Configuration, offset: 0xA */
  __IO uint32_t PLACR;                             /**< Platform Control Register, offset: 0xC */
       uint8_t RESERVED_1[48];
  __IO uint32_t CPO;                               /**< Compute Operation Control Register, offset: 0x40 */
} MCM_Type;

/* ----------------------------------------------------------------------------
   -- MCM Register Masks
   ---------------------------------------------------------------------------- */

/*!
 * @addtogroup MCM_Register_Masks MCM Register Masks
 * @{
 */

/*! @name PLASC - Crossbar Switch (AXBS) Slave Configuration */
#define MCM_PLASC_ASC_MASK                       (0xFFU)
#define MCM_PLASC_ASC_SHIFT                      (0U)
#define MCM_PLASC_ASC(x)                         (((uint16_t)(((uint16_t)(x)) << MCM_PLASC_ASC_SHIFT)) & MCM_PLASC_ASC_MASK)

/*! @name PLAMC - Crossbar Switch (AXBS) Master Configuration */
#define MCM_PLAMC_AMC_MASK                       (0xFFU)
#define MCM_PLAMC_AMC_SHIFT                      (0U)
#define MCM_PLAMC_AMC(x)                         (((uint16_t)(((uint16_t)(x)) << MCM_PLAMC_AMC_SHIFT)) & MCM_PLAMC_AMC_MASK)

/*! @name PLACR - Platform Control Register */
#define MCM_PLACR_ARB_MASK                       (0x200U)
#define MCM_PLACR_ARB_SHIFT                      (9U)
#define MCM_PLACR_ARB(x)                         (((uint32_t)(((uint32_t)(x)) << MCM_PLACR_ARB_SHIFT)) & MCM_PLACR_ARB_MASK)
#define MCM_PLACR_CFCC_MASK                      (0x400U)
#define MCM_PLACR_CFCC_SHIFT                     (10U)
#define MCM_PLACR_CFCC(x)                        (((uint32_t)(((uint32_t)(x)) << MCM_PLACR_CFCC_SHIFT)) & MCM_PLACR_CFCC_MASK)
#define MCM_PLACR_DFCDA_MASK                     (0x800U)
#define MCM_PLACR_DFCDA_SHIFT                    (11U)
#define MCM_PLACR_DFCDA(x)                       (((uint32_t)(((uint32_t)(x)) << MCM_PLACR_DFCDA_SHIFT)) & MCM_PLACR_DFCDA_MASK)
#define MCM_PLACR_DFCIC_MASK                     (0x1000U)
#define MCM_PLACR_DFCIC_SHIFT                    (12U)
#define MCM_PLACR_DFCIC(x)                       (((uint32_t)(((uint32_t)(x)) << MCM_PLACR_DFCIC_SHIFT)) & MCM_PLACR_DFCIC_MASK)
#define MCM_PLACR_DFCC_MASK                      (0x2000U)
#define MCM_PLACR_DFCC_SHIFT                     (13U)
#define MCM_PLACR_DFCC(x)                        (((uint32_t)(((uint32_t)(x)) << MCM_PLACR_DFCC_SHIFT)) & MCM_PLACR_DFCC_MASK)
#define MCM_PLACR_EFDS_MASK                      (0x4000U)
#define MCM_PLACR_EFDS_SHIFT                     (14U)
#define MCM_PLACR_EFDS(x)                        (((uint32_t)(((uint32_t)(x)) << MCM_PLACR_EFDS_SHIFT)) & MCM_PLACR_EFDS_MASK)
#define MCM_PLACR_DFCS_MASK                      (0x8000U)
#define MCM_PLACR_DFCS_SHIFT                     (15U)
#define MCM_PLACR_DFCS(x)                        (((uint32_t)(((uint32_t)(x)) << MCM_PLACR_DFCS_SHIFT)) & MCM_PLACR_DFCS_MASK)
#define MCM_PLACR_ESFC_MASK                      (0x10000U)
#define MCM_PLACR_ESFC_SHIFT                     (16U)
#define MCM_PLACR_ESFC(x)                        (((uint32_t)(((uint32_t)(x)) << MCM_PLACR_ESFC_SHIFT)) & MCM_PLACR_ESFC_MASK)

/*! @name CPO - Compute Operation Control Register */
#define MCM_CPO_CPOREQ_MASK                      (0x1U)
#define MCM_CPO_CPOREQ_SHIFT                     (0U)
#define MCM_CPO_CPOREQ(x)                        (((uint32_t)(((uint32_t)(x)) << MCM_CPO_CPOREQ_SHIFT)) & MCM_CPO_CPOREQ_MASK)
#define MCM_CPO_CPOACK_MASK                      (0x2U)
#define MCM_CPO_CPOACK_SHIFT                     (1U)
#define MCM_CPO_CPOACK(x)                        (((uint32_t)(((uint32_t)(x)) << MCM_CPO_CPOACK_SHIFT)) & MCM_CPO_CPOACK_MASK)
#define MCM_CPO_CPOWOI_MASK                      (0x4U)
#define MCM_CPO_CPOWOI_SHIFT                     (2U)
#define MCM_CPO_CPOWOI(x)                        (((uint32_t)(((uint32_t)(x)) << MCM_CPO_CPOWOI_SHIFT)) & MCM_CPO_CPOWOI_MASK)


/*!
 * @}
 */ /* end of group MCM_Register_Masks */


/* MCM - Peripheral instance base addresses */
/** Peripheral MCM base address */
#define MCM_BASE                                 (0xF0003000u)
/** Peripheral MCM base pointer */
#define MCM                                      ((MCM_Type *)MCM_BASE)
/** Array initializer of MCM peripheral base addresses */
#define MCM_BASE_ADDRS                           { MCM_BASE }
/** Array initializer of MCM peripheral base pointers */
#define MCM_BASE_PTRS                            { MCM }

/*!
 * @}
 */ /* end of group MCM_Peripheral_Access_Layer */


/* ----------------------------------------------------------------------------
   -- MTB Peripheral Access Layer
   ---------------------------------------------------------------------------- */

/*!
 * @addtogroup MTB_Peripheral_Access_Layer MTB Peripheral Access Layer
 * @{
 */

/** MTB - Register Layout Typedef */
typedef struct {
  __IO uint32_t POSITION;                          /**< MTB Position Register, offset: 0x0 */
  __IO uint32_t MASTER;                            /**< MTB Master Register, offset: 0x4 */
  __IO uint32_t FLOW;                              /**< MTB Flow Register, offset: 0x8 */
  __I  uint32_t BASE;                              /**< MTB Base Register, offset: 0xC */
       uint8_t RESERVED_0[3824];
  __I  uint32_t MODECTRL;                          /**< Integration Mode Control Register, offset: 0xF00 */
       uint8_t RESERVED_1[156];
  __I  uint32_t TAGSET;                            /**< Claim TAG Set Register, offset: 0xFA0 */
  __I  uint32_t TAGCLEAR;                          /**< Claim TAG Clear Register, offset: 0xFA4 */
       uint8_t RESERVED_2[8];
  __I  uint32_t LOCKACCESS;                        /**< Lock Access Register, offset: 0xFB0 */
  __I  uint32_t LOCKSTAT;                          /**< Lock Status Register, offset: 0xFB4 */
  __I  uint32_t AUTHSTAT;                          /**< Authentication Status Register, offset: 0xFB8 */
  __I  uint32_t DEVICEARCH;                        /**< Device Architecture Register, offset: 0xFBC */
       uint8_t RESERVED_3[8];
  __I  uint32_t DEVICECFG;                         /**< Device Configuration Register, offset: 0xFC8 */
  __I  uint32_t DEVICETYPID;                       /**< Device Type Identifier Register, offset: 0xFCC */
  __I  uint32_t PERIPHID4;                         /**< Peripheral ID Register, offset: 0xFD0 */
  __I  uint32_t PERIPHID5;                         /**< Peripheral ID Register, offset: 0xFD4 */
  __I  uint32_t PERIPHID6;                         /**< Peripheral ID Register, offset: 0xFD8 */
  __I  uint32_t PERIPHID7;                         /**< Peripheral ID Register, offset: 0xFDC */
  __I  uint32_t PERIPHID0;                         /**< Peripheral ID Register, offset: 0xFE0 */
  __I  uint32_t PERIPHID1;                         /**< Peripheral ID Register, offset: 0xFE4 */
  __I  uint32_t PERIPHID2;                         /**< Peripheral ID Register, offset: 0xFE8 */
  __I  uint32_t PERIPHID3;                         /**< Peripheral ID Register, offset: 0xFEC */
  __I  uint32_t COMPID[4];                         /**< Component ID Register, array offset: 0xFF0, array step: 0x4 */
} MTB_Type;

/* ----------------------------------------------------------------------------
   -- MTB Register Masks
   ---------------------------------------------------------------------------- */

/*!
 * @addtogroup MTB_Register_Masks MTB Register Masks
 * @{
 */

/*! @name POSITION - MTB Position Register */
#define MTB_POSITION_WRAP_MASK                   (0x4U)
#define MTB_POSITION_WRAP_SHIFT                  (2U)
#define MTB_POSITION_WRAP(x)                     (((uint32_t)(((uint32_t)(x)) << MTB_POSITION_WRAP_SHIFT)) & MTB_POSITION_WRAP_MASK)
#define MTB_POSITION_POINTER_MASK                (0xFFFFFFF8U)
#define MTB_POSITION_POINTER_SHIFT               (3U)
#define MTB_POSITION_POINTER(x)                  (((uint32_t)(((uint32_t)(x)) << MTB_POSITION_POINTER_SHIFT)) & MTB_POSITION_POINTER_MASK)

/*! @name MASTER - MTB Master Register */
#define MTB_MASTER_MASK_MASK                     (0x1FU)
#define MTB_MASTER_MASK_SHIFT                    (0U)
#define MTB_MASTER_MASK(x)                       (((uint32_t)(((uint32_t)(x)) << MTB_MASTER_MASK_SHIFT)) & MTB_MASTER_MASK_MASK)
#define MTB_MASTER_TSTARTEN_MASK                 (0x20U)
#define MTB_MASTER_TSTARTEN_SHIFT                (5U)
#define MTB_MASTER_TSTARTEN(x)                   (((uint32_t)(((uint32_t)(x)) << MTB_MASTER_TSTARTEN_SHIFT)) & MTB_MASTER_TSTARTEN_MASK)
#define MTB_MASTER_TSTOPEN_MASK                  (0x40U)
#define MTB_MASTER_TSTOPEN_SHIFT                 (6U)
#define MTB_MASTER_TSTOPEN(x)                    (((uint32_t)(((uint32_t)(x)) << MTB_MASTER_TSTOPEN_SHIFT)) & MTB_MASTER_TSTOPEN_MASK)
#define MTB_MASTER_SFRWPRIV_MASK                 (0x80U)
#define MTB_MASTER_SFRWPRIV_SHIFT                (7U)
#define MTB_MASTER_SFRWPRIV(x)                   (((uint32_t)(((uint32_t)(x)) << MTB_MASTER_SFRWPRIV_SHIFT)) & MTB_MASTER_SFRWPRIV_MASK)
#define MTB_MASTER_RAMPRIV_MASK                  (0x100U)
#define MTB_MASTER_RAMPRIV_SHIFT                 (8U)
#define MTB_MASTER_RAMPRIV(x)                    (((uint32_t)(((uint32_t)(x)) << MTB_MASTER_RAMPRIV_SHIFT)) & MTB_MASTER_RAMPRIV_MASK)
#define MTB_MASTER_HALTREQ_MASK                  (0x200U)
#define MTB_MASTER_HALTREQ_SHIFT                 (9U)
#define MTB_MASTER_HALTREQ(x)                    (((uint32_t)(((uint32_t)(x)) << MTB_MASTER_HALTREQ_SHIFT)) & MTB_MASTER_HALTREQ_MASK)
#define MTB_MASTER_EN_MASK                       (0x80000000U)
#define MTB_MASTER_EN_SHIFT                      (31U)
#define MTB_MASTER_EN(x)                         (((uint32_t)(((uint32_t)(x)) << MTB_MASTER_EN_SHIFT)) & MTB_MASTER_EN_MASK)

/*! @name FLOW - MTB Flow Register */
#define MTB_FLOW_AUTOSTOP_MASK                   (0x1U)
#define MTB_FLOW_AUTOSTOP_SHIFT                  (0U)
#define MTB_FLOW_AUTOSTOP(x)                     (((uint32_t)(((uint32_t)(x)) << MTB_FLOW_AUTOSTOP_SHIFT)) & MTB_FLOW_AUTOSTOP_MASK)
#define MTB_FLOW_AUTOHALT_MASK                   (0x2U)
#define MTB_FLOW_AUTOHALT_SHIFT                  (1U)
#define MTB_FLOW_AUTOHALT(x)                     (((uint32_t)(((uint32_t)(x)) << MTB_FLOW_AUTOHALT_SHIFT)) & MTB_FLOW_AUTOHALT_MASK)
#define MTB_FLOW_WATERMARK_MASK                  (0xFFFFFFF8U)
#define MTB_FLOW_WATERMARK_SHIFT                 (3U)
#define MTB_FLOW_WATERMARK(x)                    (((uint32_t)(((uint32_t)(x)) << MTB_FLOW_WATERMARK_SHIFT)) & MTB_FLOW_WATERMARK_MASK)

/*! @name BASE - MTB Base Register */
#define MTB_BASE_BASEADDR_MASK                   (0xFFFFFFFFU)
#define MTB_BASE_BASEADDR_SHIFT                  (0U)
#define MTB_BASE_BASEADDR(x)                     (((uint32_t)(((uint32_t)(x)) << MTB_BASE_BASEADDR_SHIFT)) & MTB_BASE_BASEADDR_MASK)

/*! @name MODECTRL - Integration Mode Control Register */
#define MTB_MODECTRL_MODECTRL_MASK               (0xFFFFFFFFU)
#define MTB_MODECTRL_MODECTRL_SHIFT              (0U)
#define MTB_MODECTRL_MODECTRL(x)                 (((uint32_t)(((uint32_t)(x)) << MTB_MODECTRL_MODECTRL_SHIFT)) & MTB_MODECTRL_MODECTRL_MASK)

/*! @name TAGSET - Claim TAG Set Register */
#define MTB_TAGSET_TAGSET_MASK                   (0xFFFFFFFFU)
#define MTB_TAGSET_TAGSET_SHIFT                  (0U)
#define MTB_TAGSET_TAGSET(x)                     (((uint32_t)(((uint32_t)(x)) << MTB_TAGSET_TAGSET_SHIFT)) & MTB_TAGSET_TAGSET_MASK)

/*! @name TAGCLEAR - Claim TAG Clear Register */
#define MTB_TAGCLEAR_TAGCLEAR_MASK               (0xFFFFFFFFU)
#define MTB_TAGCLEAR_TAGCLEAR_SHIFT              (0U)
#define MTB_TAGCLEAR_TAGCLEAR(x)                 (((uint32_t)(((uint32_t)(x)) << MTB_TAGCLEAR_TAGCLEAR_SHIFT)) & MTB_TAGCLEAR_TAGCLEAR_MASK)

/*! @name LOCKACCESS - Lock Access Register */
#define MTB_LOCKACCESS_LOCKACCESS_MASK           (0xFFFFFFFFU)
#define MTB_LOCKACCESS_LOCKACCESS_SHIFT          (0U)
#define MTB_LOCKACCESS_LOCKACCESS(x)             (((uint32_t)(((uint32_t)(x)) << MTB_LOCKACCESS_LOCKACCESS_SHIFT)) & MTB_LOCKACCESS_LOCKACCESS_MASK)

/*! @name LOCKSTAT - Lock Status Register */
#define MTB_LOCKSTAT_LOCKSTAT_MASK               (0xFFFFFFFFU)
#define MTB_LOCKSTAT_LOCKSTAT_SHIFT              (0U)
#define MTB_LOCKSTAT_LOCKSTAT(x)                 (((uint32_t)(((uint32_t)(x)) << MTB_LOCKSTAT_LOCKSTAT_SHIFT)) & MTB_LOCKSTAT_LOCKSTAT_MASK)

/*! @name AUTHSTAT - Authentication Status Register */
#define MTB_AUTHSTAT_BIT0_MASK                   (0x1U)
#define MTB_AUTHSTAT_BIT0_SHIFT                  (0U)
#define MTB_AUTHSTAT_BIT0(x)                     (((uint32_t)(((uint32_t)(x)) << MTB_AUTHSTAT_BIT0_SHIFT)) & MTB_AUTHSTAT_BIT0_MASK)
#define MTB_AUTHSTAT_BIT1_MASK                   (0x2U)
#define MTB_AUTHSTAT_BIT1_SHIFT                  (1U)
#define MTB_AUTHSTAT_BIT1(x)                     (((uint32_t)(((uint32_t)(x)) << MTB_AUTHSTAT_BIT1_SHIFT)) & MTB_AUTHSTAT_BIT1_MASK)
#define MTB_AUTHSTAT_BIT2_MASK                   (0x4U)
#define MTB_AUTHSTAT_BIT2_SHIFT                  (2U)
#define MTB_AUTHSTAT_BIT2(x)                     (((uint32_t)(((uint32_t)(x)) << MTB_AUTHSTAT_BIT2_SHIFT)) & MTB_AUTHSTAT_BIT2_MASK)
#define MTB_AUTHSTAT_BIT3_MASK                   (0x8U)
#define MTB_AUTHSTAT_BIT3_SHIFT                  (3U)
#define MTB_AUTHSTAT_BIT3(x)                     (((uint32_t)(((uint32_t)(x)) << MTB_AUTHSTAT_BIT3_SHIFT)) & MTB_AUTHSTAT_BIT3_MASK)

/*! @name DEVICEARCH - Device Architecture Register */
#define MTB_DEVICEARCH_DEVICEARCH_MASK           (0xFFFFFFFFU)
#define MTB_DEVICEARCH_DEVICEARCH_SHIFT          (0U)
#define MTB_DEVICEARCH_DEVICEARCH(x)             (((uint32_t)(((uint32_t)(x)) << MTB_DEVICEARCH_DEVICEARCH_SHIFT)) & MTB_DEVICEARCH_DEVICEARCH_MASK)

/*! @name DEVICECFG - Device Configuration Register */
#define MTB_DEVICECFG_DEVICECFG_MASK             (0xFFFFFFFFU)
#define MTB_DEVICECFG_DEVICECFG_SHIFT            (0U)
#define MTB_DEVICECFG_DEVICECFG(x)               (((uint32_t)(((uint32_t)(x)) << MTB_DEVICECFG_DEVICECFG_SHIFT)) & MTB_DEVICECFG_DEVICECFG_MASK)

/*! @name DEVICETYPID - Device Type Identifier Register */
#define MTB_DEVICETYPID_DEVICETYPID_MASK         (0xFFFFFFFFU)
#define MTB_DEVICETYPID_DEVICETYPID_SHIFT        (0U)
#define MTB_DEVICETYPID_DEVICETYPID(x)           (((uint32_t)(((uint32_t)(x)) << MTB_DEVICETYPID_DEVICETYPID_SHIFT)) & MTB_DEVICETYPID_DEVICETYPID_MASK)

/*! @name PERIPHID4 - Peripheral ID Register */
#define MTB_PERIPHID4_PERIPHID_MASK              (0xFFFFFFFFU)
#define MTB_PERIPHID4_PERIPHID_SHIFT             (0U)
#define MTB_PERIPHID4_PERIPHID(x)                (((uint32_t)(((uint32_t)(x)) << MTB_PERIPHID4_PERIPHID_SHIFT)) & MTB_PERIPHID4_PERIPHID_MASK)

/*! @name PERIPHID5 - Peripheral ID Register */
#define MTB_PERIPHID5_PERIPHID_MASK              (0xFFFFFFFFU)
#define MTB_PERIPHID5_PERIPHID_SHIFT             (0U)
#define MTB_PERIPHID5_PERIPHID(x)                (((uint32_t)(((uint32_t)(x)) << MTB_PERIPHID5_PERIPHID_SHIFT)) & MTB_PERIPHID5_PERIPHID_MASK)

/*! @name PERIPHID6 - Peripheral ID Register */
#define MTB_PERIPHID6_PERIPHID_MASK              (0xFFFFFFFFU)
#define MTB_PERIPHID6_PERIPHID_SHIFT             (0U)
#define MTB_PERIPHID6_PERIPHID(x)                (((uint32_t)(((uint32_t)(x)) << MTB_PERIPHID6_PERIPHID_SHIFT)) & MTB_PERIPHID6_PERIPHID_MASK)

/*! @name PERIPHID7 - Peripheral ID Register */
#define MTB_PERIPHID7_PERIPHID_MASK              (0xFFFFFFFFU)
#define MTB_PERIPHID7_PERIPHID_SHIFT             (0U)
#define MTB_PERIPHID7_PERIPHID(x)                (((uint32_t)(((uint32_t)(x)) << MTB_PERIPHID7_PERIPHID_SHIFT)) & MTB_PERIPHID7_PERIPHID_MASK)

/*! @name PERIPHID0 - Peripheral ID Register */
#define MTB_PERIPHID0_PERIPHID_MASK              (0xFFFFFFFFU)
#define MTB_PERIPHID0_PERIPHID_SHIFT             (0U)
#define MTB_PERIPHID0_PERIPHID(x)                (((uint32_t)(((uint32_t)(x)) << MTB_PERIPHID0_PERIPHID_SHIFT)) & MTB_PERIPHID0_PERIPHID_MASK)

/*! @name PERIPHID1 - Peripheral ID Register */
#define MTB_PERIPHID1_PERIPHID_MASK              (0xFFFFFFFFU)
#define MTB_PERIPHID1_PERIPHID_SHIFT             (0U)
#define MTB_PERIPHID1_PERIPHID(x)                (((uint32_t)(((uint32_t)(x)) << MTB_PERIPHID1_PERIPHID_SHIFT)) & MTB_PERIPHID1_PERIPHID_MASK)

/*! @name PERIPHID2 - Peripheral ID Register */
#define MTB_PERIPHID2_PERIPHID_MASK              (0xFFFFFFFFU)
#define MTB_PERIPHID2_PERIPHID_SHIFT             (0U)
#define MTB_PERIPHID2_PERIPHID(x)                (((uint32_t)(((uint32_t)(x)) << MTB_PERIPHID2_PERIPHID_SHIFT)) & MTB_PERIPHID2_PERIPHID_MASK)

/*! @name PERIPHID3 - Peripheral ID Register */
#define MTB_PERIPHID3_PERIPHID_MASK              (0xFFFFFFFFU)
#define MTB_PERIPHID3_PERIPHID_SHIFT             (0U)
#define MTB_PERIPHID3_PERIPHID(x)                (((uint32_t)(((uint32_t)(x)) << MTB_PERIPHID3_PERIPHID_SHIFT)) & MTB_PERIPHID3_PERIPHID_MASK)

/*! @name COMPID - Component ID Register */
#define MTB_COMPID_COMPID_MASK                   (0xFFFFFFFFU)
#define MTB_COMPID_COMPID_SHIFT                  (0U)
#define MTB_COMPID_COMPID(x)                     (((uint32_t)(((uint32_t)(x)) << MTB_COMPID_COMPID_SHIFT)) & MTB_COMPID_COMPID_MASK)

/* The count of MTB_COMPID */
#define MTB_COMPID_COUNT                         (4U)


/*!
 * @}
 */ /* end of group MTB_Register_Masks */


/* MTB - Peripheral instance base addresses */
/** Peripheral MTB base address */
#define MTB_BASE                                 (0xF0000000u)
/** Peripheral MTB base pointer */
#define MTB                                      ((MTB_Type *)MTB_BASE)
/** Array initializer of MTB peripheral base addresses */
#define MTB_BASE_ADDRS                           { MTB_BASE }
/** Array initializer of MTB peripheral base pointers */
#define MTB_BASE_PTRS                            { MTB }

/*!
 * @}
 */ /* end of group MTB_Peripheral_Access_Layer */


/* ----------------------------------------------------------------------------
   -- MTBDWT Peripheral Access Layer
   ---------------------------------------------------------------------------- */

/*!
 * @addtogroup MTBDWT_Peripheral_Access_Layer MTBDWT Peripheral Access Layer
 * @{
 */

/** MTBDWT - Register Layout Typedef */
typedef struct {
  __I  uint32_t CTRL;                              /**< MTB DWT Control Register, offset: 0x0 */
       uint8_t RESERVED_0[28];
  struct {                                         /* offset: 0x20, array step: 0x10 */
    __IO uint32_t COMP;                              /**< MTB_DWT Comparator Register, array offset: 0x20, array step: 0x10 */
    __IO uint32_t MASK;                              /**< MTB_DWT Comparator Mask Register, array offset: 0x24, array step: 0x10 */
    __IO uint32_t FCT;                               /**< MTB_DWT Comparator Function Register 0..MTB_DWT Comparator Function Register 1, array offset: 0x28, array step: 0x10 */
         uint8_t RESERVED_0[4];
  } COMPARATOR[2];
       uint8_t RESERVED_1[448];
  __IO uint32_t TBCTRL;                            /**< MTB_DWT Trace Buffer Control Register, offset: 0x200 */
       uint8_t RESERVED_2[3524];
  __I  uint32_t DEVICECFG;                         /**< Device Configuration Register, offset: 0xFC8 */
  __I  uint32_t DEVICETYPID;                       /**< Device Type Identifier Register, offset: 0xFCC */
  __I  uint32_t PERIPHID4;                         /**< Peripheral ID Register, offset: 0xFD0 */
  __I  uint32_t PERIPHID5;                         /**< Peripheral ID Register, offset: 0xFD4 */
  __I  uint32_t PERIPHID6;                         /**< Peripheral ID Register, offset: 0xFD8 */
  __I  uint32_t PERIPHID7;                         /**< Peripheral ID Register, offset: 0xFDC */
  __I  uint32_t PERIPHID0;                         /**< Peripheral ID Register, offset: 0xFE0 */
  __I  uint32_t PERIPHID1;                         /**< Peripheral ID Register, offset: 0xFE4 */
  __I  uint32_t PERIPHID2;                         /**< Peripheral ID Register, offset: 0xFE8 */
  __I  uint32_t PERIPHID3;                         /**< Peripheral ID Register, offset: 0xFEC */
  __I  uint32_t COMPID[4];                         /**< Component ID Register, array offset: 0xFF0, array step: 0x4 */
} MTBDWT_Type;

/* ----------------------------------------------------------------------------
   -- MTBDWT Register Masks
   ---------------------------------------------------------------------------- */

/*!
 * @addtogroup MTBDWT_Register_Masks MTBDWT Register Masks
 * @{
 */

/*! @name CTRL - MTB DWT Control Register */
#define MTBDWT_CTRL_DWTCFGCTRL_MASK              (0xFFFFFFFU)
#define MTBDWT_CTRL_DWTCFGCTRL_SHIFT             (0U)
#define MTBDWT_CTRL_DWTCFGCTRL(x)                (((uint32_t)(((uint32_t)(x)) << MTBDWT_CTRL_DWTCFGCTRL_SHIFT)) & MTBDWT_CTRL_DWTCFGCTRL_MASK)
#define MTBDWT_CTRL_NUMCMP_MASK                  (0xF0000000U)
#define MTBDWT_CTRL_NUMCMP_SHIFT                 (28U)
#define MTBDWT_CTRL_NUMCMP(x)                    (((uint32_t)(((uint32_t)(x)) << MTBDWT_CTRL_NUMCMP_SHIFT)) & MTBDWT_CTRL_NUMCMP_MASK)

/*! @name COMP - MTB_DWT Comparator Register */
#define MTBDWT_COMP_COMP_MASK                    (0xFFFFFFFFU)
#define MTBDWT_COMP_COMP_SHIFT                   (0U)
#define MTBDWT_COMP_COMP(x)                      (((uint32_t)(((uint32_t)(x)) << MTBDWT_COMP_COMP_SHIFT)) & MTBDWT_COMP_COMP_MASK)

/* The count of MTBDWT_COMP */
#define MTBDWT_COMP_COUNT                        (2U)

/*! @name MASK - MTB_DWT Comparator Mask Register */
#define MTBDWT_MASK_MASK_MASK                    (0x1FU)
#define MTBDWT_MASK_MASK_SHIFT                   (0U)
#define MTBDWT_MASK_MASK(x)                      (((uint32_t)(((uint32_t)(x)) << MTBDWT_MASK_MASK_SHIFT)) & MTBDWT_MASK_MASK_MASK)

/* The count of MTBDWT_MASK */
#define MTBDWT_MASK_COUNT                        (2U)

/*! @name FCT - MTB_DWT Comparator Function Register 0..MTB_DWT Comparator Function Register 1 */
#define MTBDWT_FCT_FUNCTION_MASK                 (0xFU)
#define MTBDWT_FCT_FUNCTION_SHIFT                (0U)
#define MTBDWT_FCT_FUNCTION(x)                   (((uint32_t)(((uint32_t)(x)) << MTBDWT_FCT_FUNCTION_SHIFT)) & MTBDWT_FCT_FUNCTION_MASK)
#define MTBDWT_FCT_DATAVMATCH_MASK               (0x100U)
#define MTBDWT_FCT_DATAVMATCH_SHIFT              (8U)
#define MTBDWT_FCT_DATAVMATCH(x)                 (((uint32_t)(((uint32_t)(x)) << MTBDWT_FCT_DATAVMATCH_SHIFT)) & MTBDWT_FCT_DATAVMATCH_MASK)
#define MTBDWT_FCT_DATAVSIZE_MASK                (0xC00U)
#define MTBDWT_FCT_DATAVSIZE_SHIFT               (10U)
#define MTBDWT_FCT_DATAVSIZE(x)                  (((uint32_t)(((uint32_t)(x)) << MTBDWT_FCT_DATAVSIZE_SHIFT)) & MTBDWT_FCT_DATAVSIZE_MASK)
#define MTBDWT_FCT_DATAVADDR0_MASK               (0xF000U)
#define MTBDWT_FCT_DATAVADDR0_SHIFT              (12U)
#define MTBDWT_FCT_DATAVADDR0(x)                 (((uint32_t)(((uint32_t)(x)) << MTBDWT_FCT_DATAVADDR0_SHIFT)) & MTBDWT_FCT_DATAVADDR0_MASK)
#define MTBDWT_FCT_MATCHED_MASK                  (0x1000000U)
#define MTBDWT_FCT_MATCHED_SHIFT                 (24U)
#define MTBDWT_FCT_MATCHED(x)                    (((uint32_t)(((uint32_t)(x)) << MTBDWT_FCT_MATCHED_SHIFT)) & MTBDWT_FCT_MATCHED_MASK)

/* The count of MTBDWT_FCT */
#define MTBDWT_FCT_COUNT                         (2U)

/*! @name TBCTRL - MTB_DWT Trace Buffer Control Register */
#define MTBDWT_TBCTRL_ACOMP0_MASK                (0x1U)
#define MTBDWT_TBCTRL_ACOMP0_SHIFT               (0U)
#define MTBDWT_TBCTRL_ACOMP0(x)                  (((uint32_t)(((uint32_t)(x)) << MTBDWT_TBCTRL_ACOMP0_SHIFT)) & MTBDWT_TBCTRL_ACOMP0_MASK)
#define MTBDWT_TBCTRL_ACOMP1_MASK                (0x2U)
#define MTBDWT_TBCTRL_ACOMP1_SHIFT               (1U)
#define MTBDWT_TBCTRL_ACOMP1(x)                  (((uint32_t)(((uint32_t)(x)) << MTBDWT_TBCTRL_ACOMP1_SHIFT)) & MTBDWT_TBCTRL_ACOMP1_MASK)
#define MTBDWT_TBCTRL_NUMCOMP_MASK               (0xF0000000U)
#define MTBDWT_TBCTRL_NUMCOMP_SHIFT              (28U)
#define MTBDWT_TBCTRL_NUMCOMP(x)                 (((uint32_t)(((uint32_t)(x)) << MTBDWT_TBCTRL_NUMCOMP_SHIFT)) & MTBDWT_TBCTRL_NUMCOMP_MASK)

/*! @name DEVICECFG - Device Configuration Register */
#define MTBDWT_DEVICECFG_DEVICECFG_MASK          (0xFFFFFFFFU)
#define MTBDWT_DEVICECFG_DEVICECFG_SHIFT         (0U)
#define MTBDWT_DEVICECFG_DEVICECFG(x)            (((uint32_t)(((uint32_t)(x)) << MTBDWT_DEVICECFG_DEVICECFG_SHIFT)) & MTBDWT_DEVICECFG_DEVICECFG_MASK)

/*! @name DEVICETYPID - Device Type Identifier Register */
#define MTBDWT_DEVICETYPID_DEVICETYPID_MASK      (0xFFFFFFFFU)
#define MTBDWT_DEVICETYPID_DEVICETYPID_SHIFT     (0U)
#define MTBDWT_DEVICETYPID_DEVICETYPID(x)        (((uint32_t)(((uint32_t)(x)) << MTBDWT_DEVICETYPID_DEVICETYPID_SHIFT)) & MTBDWT_DEVICETYPID_DEVICETYPID_MASK)

/*! @name PERIPHID4 - Peripheral ID Register */
#define MTBDWT_PERIPHID4_PERIPHID_MASK           (0xFFFFFFFFU)
#define MTBDWT_PERIPHID4_PERIPHID_SHIFT          (0U)
#define MTBDWT_PERIPHID4_PERIPHID(x)             (((uint32_t)(((uint32_t)(x)) << MTBDWT_PERIPHID4_PERIPHID_SHIFT)) & MTBDWT_PERIPHID4_PERIPHID_MASK)

/*! @name PERIPHID5 - Peripheral ID Register */
#define MTBDWT_PERIPHID5_PERIPHID_MASK           (0xFFFFFFFFU)
#define MTBDWT_PERIPHID5_PERIPHID_SHIFT          (0U)
#define MTBDWT_PERIPHID5_PERIPHID(x)             (((uint32_t)(((uint32_t)(x)) << MTBDWT_PERIPHID5_PERIPHID_SHIFT)) & MTBDWT_PERIPHID5_PERIPHID_MASK)

/*! @name PERIPHID6 - Peripheral ID Register */
#define MTBDWT_PERIPHID6_PERIPHID_MASK           (0xFFFFFFFFU)
#define MTBDWT_PERIPHID6_PERIPHID_SHIFT          (0U)
#define MTBDWT_PERIPHID6_PERIPHID(x)             (((uint32_t)(((uint32_t)(x)) << MTBDWT_PERIPHID6_PERIPHID_SHIFT)) & MTBDWT_PERIPHID6_PERIPHID_MASK)

/*! @name PERIPHID7 - Peripheral ID Register */
#define MTBDWT_PERIPHID7_PERIPHID_MASK           (0xFFFFFFFFU)
#define MTBDWT_PERIPHID7_PERIPHID_SHIFT          (0U)
#define MTBDWT_PERIPHID7_PERIPHID(x)             (((uint32_t)(((uint32_t)(x)) << MTBDWT_PERIPHID7_PERIPHID_SHIFT)) & MTBDWT_PERIPHID7_PERIPHID_MASK)

/*! @name PERIPHID0 - Peripheral ID Register */
#define MTBDWT_PERIPHID0_PERIPHID_MASK           (0xFFFFFFFFU)
#define MTBDWT_PERIPHID0_PERIPHID_SHIFT          (0U)
#define MTBDWT_PERIPHID0_PERIPHID(x)             (((uint32_t)(((uint32_t)(x)) << MTBDWT_PERIPHID0_PERIPHID_SHIFT)) & MTBDWT_PERIPHID0_PERIPHID_MASK)

/*! @name PERIPHID1 - Peripheral ID Register */
#define MTBDWT_PERIPHID1_PERIPHID_MASK           (0xFFFFFFFFU)
#define MTBDWT_PERIPHID1_PERIPHID_SHIFT          (0U)
#define MTBDWT_PERIPHID1_PERIPHID(x)             (((uint32_t)(((uint32_t)(x)) << MTBDWT_PERIPHID1_PERIPHID_SHIFT)) & MTBDWT_PERIPHID1_PERIPHID_MASK)

/*! @name PERIPHID2 - Peripheral ID Register */
#define MTBDWT_PERIPHID2_PERIPHID_MASK           (0xFFFFFFFFU)
#define MTBDWT_PERIPHID2_PERIPHID_SHIFT          (0U)
#define MTBDWT_PERIPHID2_PERIPHID(x)             (((uint32_t)(((uint32_t)(x)) << MTBDWT_PERIPHID2_PERIPHID_SHIFT)) & MTBDWT_PERIPHID2_PERIPHID_MASK)

/*! @name PERIPHID3 - Peripheral ID Register */
#define MTBDWT_PERIPHID3_PERIPHID_MASK           (0xFFFFFFFFU)
#define MTBDWT_PERIPHID3_PERIPHID_SHIFT          (0U)
#define MTBDWT_PERIPHID3_PERIPHID(x)             (((uint32_t)(((uint32_t)(x)) << MTBDWT_PERIPHID3_PERIPHID_SHIFT)) & MTBDWT_PERIPHID3_PERIPHID_MASK)

/*! @name COMPID - Component ID Register */
#define MTBDWT_COMPID_COMPID_MASK                (0xFFFFFFFFU)
#define MTBDWT_COMPID_COMPID_SHIFT               (0U)
#define MTBDWT_COMPID_COMPID(x)                  (((uint32_t)(((uint32_t)(x)) << MTBDWT_COMPID_COMPID_SHIFT)) & MTBDWT_COMPID_COMPID_MASK)

/* The count of MTBDWT_COMPID */
#define MTBDWT_COMPID_COUNT                      (4U)


/*!
 * @}
 */ /* end of group MTBDWT_Register_Masks */


/* MTBDWT - Peripheral instance base addresses */
/** Peripheral MTBDWT base address */
#define MTBDWT_BASE                              (0xF0001000u)
/** Peripheral MTBDWT base pointer */
#define MTBDWT                                   ((MTBDWT_Type *)MTBDWT_BASE)
/** Array initializer of MTBDWT peripheral base addresses */
#define MTBDWT_BASE_ADDRS                        { MTBDWT_BASE }
/** Array initializer of MTBDWT peripheral base pointers */
#define MTBDWT_BASE_PTRS                         { MTBDWT }

/*!
 * @}
 */ /* end of group MTBDWT_Peripheral_Access_Layer */


/* ----------------------------------------------------------------------------
   -- NV Peripheral Access Layer
   ---------------------------------------------------------------------------- */

/*!
 * @addtogroup NV_Peripheral_Access_Layer NV Peripheral Access Layer
 * @{
 */

/** NV - Register Layout Typedef */
typedef struct {
  __I  uint8_t BACKKEY3;                           /**< Backdoor Comparison Key 3., offset: 0x0 */
  __I  uint8_t BACKKEY2;                           /**< Backdoor Comparison Key 2., offset: 0x1 */
  __I  uint8_t BACKKEY1;                           /**< Backdoor Comparison Key 1., offset: 0x2 */
  __I  uint8_t BACKKEY0;                           /**< Backdoor Comparison Key 0., offset: 0x3 */
  __I  uint8_t BACKKEY7;                           /**< Backdoor Comparison Key 7., offset: 0x4 */
  __I  uint8_t BACKKEY6;                           /**< Backdoor Comparison Key 6., offset: 0x5 */
  __I  uint8_t BACKKEY5;                           /**< Backdoor Comparison Key 5., offset: 0x6 */
  __I  uint8_t BACKKEY4;                           /**< Backdoor Comparison Key 4., offset: 0x7 */
  __I  uint8_t FPROT3;                             /**< Non-volatile P-Flash Protection 1 - Low Register, offset: 0x8 */
  __I  uint8_t FPROT2;                             /**< Non-volatile P-Flash Protection 1 - High Register, offset: 0x9 */
  __I  uint8_t FPROT1;                             /**< Non-volatile P-Flash Protection 0 - Low Register, offset: 0xA */
  __I  uint8_t FPROT0;                             /**< Non-volatile P-Flash Protection 0 - High Register, offset: 0xB */
  __I  uint8_t FSEC;                               /**< Non-volatile Flash Security Register, offset: 0xC */
  __I  uint8_t FOPT;                               /**< Non-volatile Flash Option Register, offset: 0xD */
} NV_Type;

/* ----------------------------------------------------------------------------
   -- NV Register Masks
   ---------------------------------------------------------------------------- */

/*!
 * @addtogroup NV_Register_Masks NV Register Masks
 * @{
 */

/*! @name BACKKEY3 - Backdoor Comparison Key 3. */
#define NV_BACKKEY3_KEY_MASK                     (0xFFU)
#define NV_BACKKEY3_KEY_SHIFT                    (0U)
#define NV_BACKKEY3_KEY(x)                       (((uint8_t)(((uint8_t)(x)) << NV_BACKKEY3_KEY_SHIFT)) & NV_BACKKEY3_KEY_MASK)

/*! @name BACKKEY2 - Backdoor Comparison Key 2. */
#define NV_BACKKEY2_KEY_MASK                     (0xFFU)
#define NV_BACKKEY2_KEY_SHIFT                    (0U)
#define NV_BACKKEY2_KEY(x)                       (((uint8_t)(((uint8_t)(x)) << NV_BACKKEY2_KEY_SHIFT)) & NV_BACKKEY2_KEY_MASK)

/*! @name BACKKEY1 - Backdoor Comparison Key 1. */
#define NV_BACKKEY1_KEY_MASK                     (0xFFU)
#define NV_BACKKEY1_KEY_SHIFT                    (0U)
#define NV_BACKKEY1_KEY(x)                       (((uint8_t)(((uint8_t)(x)) << NV_BACKKEY1_KEY_SHIFT)) & NV_BACKKEY1_KEY_MASK)

/*! @name BACKKEY0 - Backdoor Comparison Key 0. */
#define NV_BACKKEY0_KEY_MASK                     (0xFFU)
#define NV_BACKKEY0_KEY_SHIFT                    (0U)
#define NV_BACKKEY0_KEY(x)                       (((uint8_t)(((uint8_t)(x)) << NV_BACKKEY0_KEY_SHIFT)) & NV_BACKKEY0_KEY_MASK)

/*! @name BACKKEY7 - Backdoor Comparison Key 7. */
#define NV_BACKKEY7_KEY_MASK                     (0xFFU)
#define NV_BACKKEY7_KEY_SHIFT                    (0U)
#define NV_BACKKEY7_KEY(x)                       (((uint8_t)(((uint8_t)(x)) << NV_BACKKEY7_KEY_SHIFT)) & NV_BACKKEY7_KEY_MASK)

/*! @name BACKKEY6 - Backdoor Comparison Key 6. */
#define NV_BACKKEY6_KEY_MASK                     (0xFFU)
#define NV_BACKKEY6_KEY_SHIFT                    (0U)
#define NV_BACKKEY6_KEY(x)                       (((uint8_t)(((uint8_t)(x)) << NV_BACKKEY6_KEY_SHIFT)) & NV_BACKKEY6_KEY_MASK)

/*! @name BACKKEY5 - Backdoor Comparison Key 5. */
#define NV_BACKKEY5_KEY_MASK                     (0xFFU)
#define NV_BACKKEY5_KEY_SHIFT                    (0U)
#define NV_BACKKEY5_KEY(x)                       (((uint8_t)(((uint8_t)(x)) << NV_BACKKEY5_KEY_SHIFT)) & NV_BACKKEY5_KEY_MASK)

/*! @name BACKKEY4 - Backdoor Comparison Key 4. */
#define NV_BACKKEY4_KEY_MASK                     (0xFFU)
#define NV_BACKKEY4_KEY_SHIFT                    (0U)
#define NV_BACKKEY4_KEY(x)                       (((uint8_t)(((uint8_t)(x)) << NV_BACKKEY4_KEY_SHIFT)) & NV_BACKKEY4_KEY_MASK)

/*! @name FPROT3 - Non-volatile P-Flash Protection 1 - Low Register */
#define NV_FPROT3_PROT_MASK                      (0xFFU)
#define NV_FPROT3_PROT_SHIFT                     (0U)
#define NV_FPROT3_PROT(x)                        (((uint8_t)(((uint8_t)(x)) << NV_FPROT3_PROT_SHIFT)) & NV_FPROT3_PROT_MASK)

/*! @name FPROT2 - Non-volatile P-Flash Protection 1 - High Register */
#define NV_FPROT2_PROT_MASK                      (0xFFU)
#define NV_FPROT2_PROT_SHIFT                     (0U)
#define NV_FPROT2_PROT(x)                        (((uint8_t)(((uint8_t)(x)) << NV_FPROT2_PROT_SHIFT)) & NV_FPROT2_PROT_MASK)

/*! @name FPROT1 - Non-volatile P-Flash Protection 0 - Low Register */
#define NV_FPROT1_PROT_MASK                      (0xFFU)
#define NV_FPROT1_PROT_SHIFT                     (0U)
#define NV_FPROT1_PROT(x)                        (((uint8_t)(((uint8_t)(x)) << NV_FPROT1_PROT_SHIFT)) & NV_FPROT1_PROT_MASK)

/*! @name FPROT0 - Non-volatile P-Flash Protection 0 - High Register */
#define NV_FPROT0_PROT_MASK                      (0xFFU)
#define NV_FPROT0_PROT_SHIFT                     (0U)
#define NV_FPROT0_PROT(x)                        (((uint8_t)(((uint8_t)(x)) << NV_FPROT0_PROT_SHIFT)) & NV_FPROT0_PROT_MASK)

/*! @name FSEC - Non-volatile Flash Security Register */
#define NV_FSEC_SEC_MASK                         (0x3U)
#define NV_FSEC_SEC_SHIFT                        (0U)
#define NV_FSEC_SEC(x)                           (((uint8_t)(((uint8_t)(x)) << NV_FSEC_SEC_SHIFT)) & NV_FSEC_SEC_MASK)
#define NV_FSEC_FSLACC_MASK                      (0xCU)
#define NV_FSEC_FSLACC_SHIFT                     (2U)
#define NV_FSEC_FSLACC(x)                        (((uint8_t)(((uint8_t)(x)) << NV_FSEC_FSLACC_SHIFT)) & NV_FSEC_FSLACC_MASK)
#define NV_FSEC_MEEN_MASK                        (0x30U)
#define NV_FSEC_MEEN_SHIFT                       (4U)
#define NV_FSEC_MEEN(x)                          (((uint8_t)(((uint8_t)(x)) << NV_FSEC_MEEN_SHIFT)) & NV_FSEC_MEEN_MASK)
#define NV_FSEC_KEYEN_MASK                       (0xC0U)
#define NV_FSEC_KEYEN_SHIFT                      (6U)
#define NV_FSEC_KEYEN(x)                         (((uint8_t)(((uint8_t)(x)) << NV_FSEC_KEYEN_SHIFT)) & NV_FSEC_KEYEN_MASK)

/*! @name FOPT - Non-volatile Flash Option Register */
#define NV_FOPT_LPBOOT0_MASK                     (0x1U)
#define NV_FOPT_LPBOOT0_SHIFT                    (0U)
#define NV_FOPT_LPBOOT0(x)                       (((uint8_t)(((uint8_t)(x)) << NV_FOPT_LPBOOT0_SHIFT)) & NV_FOPT_LPBOOT0_MASK)
#define NV_FOPT_NMI_DIS_MASK                     (0x4U)
#define NV_FOPT_NMI_DIS_SHIFT                    (2U)
#define NV_FOPT_NMI_DIS(x)                       (((uint8_t)(((uint8_t)(x)) << NV_FOPT_NMI_DIS_SHIFT)) & NV_FOPT_NMI_DIS_MASK)
#define NV_FOPT_RESET_PIN_CFG_MASK               (0x8U)
#define NV_FOPT_RESET_PIN_CFG_SHIFT              (3U)
#define NV_FOPT_RESET_PIN_CFG(x)                 (((uint8_t)(((uint8_t)(x)) << NV_FOPT_RESET_PIN_CFG_SHIFT)) & NV_FOPT_RESET_PIN_CFG_MASK)
#define NV_FOPT_LPBOOT1_MASK                     (0x10U)
#define NV_FOPT_LPBOOT1_SHIFT                    (4U)
#define NV_FOPT_LPBOOT1(x)                       (((uint8_t)(((uint8_t)(x)) << NV_FOPT_LPBOOT1_SHIFT)) & NV_FOPT_LPBOOT1_MASK)
#define NV_FOPT_FAST_INIT_MASK                   (0x20U)
#define NV_FOPT_FAST_INIT_SHIFT                  (5U)
#define NV_FOPT_FAST_INIT(x)                     (((uint8_t)(((uint8_t)(x)) << NV_FOPT_FAST_INIT_SHIFT)) & NV_FOPT_FAST_INIT_MASK)


/*!
 * @}
 */ /* end of group NV_Register_Masks */


/* NV - Peripheral instance base addresses */
/** Peripheral FTFA_FlashConfig base address */
#define FTFA_FlashConfig_BASE                    (0x400u)
/** Peripheral FTFA_FlashConfig base pointer */
#define FTFA_FlashConfig                         ((NV_Type *)FTFA_FlashConfig_BASE)
/** Array initializer of NV peripheral base addresses */
#define NV_BASE_ADDRS                            { FTFA_FlashConfig_BASE }
/** Array initializer of NV peripheral base pointers */
#define NV_BASE_PTRS                             { FTFA_FlashConfig }

/*!
 * @}
 */ /* end of group NV_Peripheral_Access_Layer */


/* ----------------------------------------------------------------------------
   -- PIT Peripheral Access Layer
   ---------------------------------------------------------------------------- */

/*!
 * @addtogroup PIT_Peripheral_Access_Layer PIT Peripheral Access Layer
 * @{
 */

/** PIT - Register Layout Typedef */
typedef struct {
  __IO uint32_t MCR;                               /**< PIT Module Control Register, offset: 0x0 */
       uint8_t RESERVED_0[220];
  __I  uint32_t LTMR64H;                           /**< PIT Upper Lifetime Timer Register, offset: 0xE0 */
  __I  uint32_t LTMR64L;                           /**< PIT Lower Lifetime Timer Register, offset: 0xE4 */
       uint8_t RESERVED_1[24];
  struct {                                         /* offset: 0x100, array step: 0x10 */
    __IO uint32_t LDVAL;                             /**< Timer Load Value Register, array offset: 0x100, array step: 0x10 */
    __I  uint32_t CVAL;                              /**< Current Timer Value Register, array offset: 0x104, array step: 0x10 */
    __IO uint32_t TCTRL;                             /**< Timer Control Register, array offset: 0x108, array step: 0x10 */
    __IO uint32_t TFLG;                              /**< Timer Flag Register, array offset: 0x10C, array step: 0x10 */
  } CHANNEL[2];
} PIT_Type;

/* ----------------------------------------------------------------------------
   -- PIT Register Masks
   ---------------------------------------------------------------------------- */

/*!
 * @addtogroup PIT_Register_Masks PIT Register Masks
 * @{
 */

/*! @name MCR - PIT Module Control Register */
#define PIT_MCR_FRZ_MASK                         (0x1U)
#define PIT_MCR_FRZ_SHIFT                        (0U)
#define PIT_MCR_FRZ(x)                           (((uint32_t)(((uint32_t)(x)) << PIT_MCR_FRZ_SHIFT)) & PIT_MCR_FRZ_MASK)
#define PIT_MCR_MDIS_MASK                        (0x2U)
#define PIT_MCR_MDIS_SHIFT                       (1U)
#define PIT_MCR_MDIS(x)                          (((uint32_t)(((uint32_t)(x)) << PIT_MCR_MDIS_SHIFT)) & PIT_MCR_MDIS_MASK)

/*! @name LTMR64H - PIT Upper Lifetime Timer Register */
#define PIT_LTMR64H_LTH_MASK                     (0xFFFFFFFFU)
#define PIT_LTMR64H_LTH_SHIFT                    (0U)
#define PIT_LTMR64H_LTH(x)                       (((uint32_t)(((uint32_t)(x)) << PIT_LTMR64H_LTH_SHIFT)) & PIT_LTMR64H_LTH_MASK)

/*! @name LTMR64L - PIT Lower Lifetime Timer Register */
#define PIT_LTMR64L_LTL_MASK                     (0xFFFFFFFFU)
#define PIT_LTMR64L_LTL_SHIFT                    (0U)
#define PIT_LTMR64L_LTL(x)                       (((uint32_t)(((uint32_t)(x)) << PIT_LTMR64L_LTL_SHIFT)) & PIT_LTMR64L_LTL_MASK)

/*! @name LDVAL - Timer Load Value Register */
#define PIT_LDVAL_TSV_MASK                       (0xFFFFFFFFU)
#define PIT_LDVAL_TSV_SHIFT                      (0U)
#define PIT_LDVAL_TSV(x)                         (((uint32_t)(((uint32_t)(x)) << PIT_LDVAL_TSV_SHIFT)) & PIT_LDVAL_TSV_MASK)

/* The count of PIT_LDVAL */
#define PIT_LDVAL_COUNT                          (2U)

/*! @name CVAL - Current Timer Value Register */
#define PIT_CVAL_TVL_MASK                        (0xFFFFFFFFU)
#define PIT_CVAL_TVL_SHIFT                       (0U)
#define PIT_CVAL_TVL(x)                          (((uint32_t)(((uint32_t)(x)) << PIT_CVAL_TVL_SHIFT)) & PIT_CVAL_TVL_MASK)

/* The count of PIT_CVAL */
#define PIT_CVAL_COUNT                           (2U)

/*! @name TCTRL - Timer Control Register */
#define PIT_TCTRL_TEN_MASK                       (0x1U)
#define PIT_TCTRL_TEN_SHIFT                      (0U)
#define PIT_TCTRL_TEN(x)                         (((uint32_t)(((uint32_t)(x)) << PIT_TCTRL_TEN_SHIFT)) & PIT_TCTRL_TEN_MASK)
#define PIT_TCTRL_TIE_MASK                       (0x2U)
#define PIT_TCTRL_TIE_SHIFT                      (1U)
#define PIT_TCTRL_TIE(x)                         (((uint32_t)(((uint32_t)(x)) << PIT_TCTRL_TIE_SHIFT)) & PIT_TCTRL_TIE_MASK)
#define PIT_TCTRL_CHN_MASK                       (0x4U)
#define PIT_TCTRL_CHN_SHIFT                      (2U)
#define PIT_TCTRL_CHN(x)                         (((uint32_t)(((uint32_t)(x)) << PIT_TCTRL_CHN_SHIFT)) & PIT_TCTRL_CHN_MASK)

/* The count of PIT_TCTRL */
#define PIT_TCTRL_COUNT                          (2U)

/*! @name TFLG - Timer Flag Register */
#define PIT_TFLG_TIF_MASK                        (0x1U)
#define PIT_TFLG_TIF_SHIFT                       (0U)
#define PIT_TFLG_TIF(x)                          (((uint32_t)(((uint32_t)(x)) << PIT_TFLG_TIF_SHIFT)) & PIT_TFLG_TIF_MASK)

/* The count of PIT_TFLG */
#define PIT_TFLG_COUNT                           (2U)


/*!
 * @}
 */ /* end of group PIT_Register_Masks */


/* PIT - Peripheral instance base addresses */
/** Peripheral PIT base address */
#define PIT_BASE                                 (0x40037000u)
/** Peripheral PIT base pointer */
#define PIT                                      ((PIT_Type *)PIT_BASE)
/** Array initializer of PIT peripheral base addresses */
#define PIT_BASE_ADDRS                           { PIT_BASE }
/** Array initializer of PIT peripheral base pointers */
#define PIT_BASE_PTRS                            { PIT }
/** Interrupt vectors for the PIT peripheral type */
#define PIT_IRQS                                 { PIT_IRQn, PIT_IRQn }

/*!
 * @}
 */ /* end of group PIT_Peripheral_Access_Layer */


/* ----------------------------------------------------------------------------
   -- PMC Peripheral Access Layer
   ---------------------------------------------------------------------------- */

/*!
 * @addtogroup PMC_Peripheral_Access_Layer PMC Peripheral Access Layer
 * @{
 */

/** PMC - Register Layout Typedef */
typedef struct {
  __IO uint8_t LVDSC1;                             /**< Low Voltage Detect Status And Control 1 register, offset: 0x0 */
  __IO uint8_t LVDSC2;                             /**< Low Voltage Detect Status And Control 2 register, offset: 0x1 */
  __IO uint8_t REGSC;                              /**< Regulator Status And Control register, offset: 0x2 */
} PMC_Type;

/* ----------------------------------------------------------------------------
   -- PMC Register Masks
   ---------------------------------------------------------------------------- */

/*!
 * @addtogroup PMC_Register_Masks PMC Register Masks
 * @{
 */

/*! @name LVDSC1 - Low Voltage Detect Status And Control 1 register */
#define PMC_LVDSC1_LVDV_MASK                     (0x3U)
#define PMC_LVDSC1_LVDV_SHIFT                    (0U)
#define PMC_LVDSC1_LVDV(x)                       (((uint8_t)(((uint8_t)(x)) << PMC_LVDSC1_LVDV_SHIFT)) & PMC_LVDSC1_LVDV_MASK)
#define PMC_LVDSC1_LVDRE_MASK                    (0x10U)
#define PMC_LVDSC1_LVDRE_SHIFT                   (4U)
#define PMC_LVDSC1_LVDRE(x)                      (((uint8_t)(((uint8_t)(x)) << PMC_LVDSC1_LVDRE_SHIFT)) & PMC_LVDSC1_LVDRE_MASK)
#define PMC_LVDSC1_LVDIE_MASK                    (0x20U)
#define PMC_LVDSC1_LVDIE_SHIFT                   (5U)
#define PMC_LVDSC1_LVDIE(x)                      (((uint8_t)(((uint8_t)(x)) << PMC_LVDSC1_LVDIE_SHIFT)) & PMC_LVDSC1_LVDIE_MASK)
#define PMC_LVDSC1_LVDACK_MASK                   (0x40U)
#define PMC_LVDSC1_LVDACK_SHIFT                  (6U)
#define PMC_LVDSC1_LVDACK(x)                     (((uint8_t)(((uint8_t)(x)) << PMC_LVDSC1_LVDACK_SHIFT)) & PMC_LVDSC1_LVDACK_MASK)
#define PMC_LVDSC1_LVDF_MASK                     (0x80U)
#define PMC_LVDSC1_LVDF_SHIFT                    (7U)
#define PMC_LVDSC1_LVDF(x)                       (((uint8_t)(((uint8_t)(x)) << PMC_LVDSC1_LVDF_SHIFT)) & PMC_LVDSC1_LVDF_MASK)

/*! @name LVDSC2 - Low Voltage Detect Status And Control 2 register */
#define PMC_LVDSC2_LVWV_MASK                     (0x3U)
#define PMC_LVDSC2_LVWV_SHIFT                    (0U)
#define PMC_LVDSC2_LVWV(x)                       (((uint8_t)(((uint8_t)(x)) << PMC_LVDSC2_LVWV_SHIFT)) & PMC_LVDSC2_LVWV_MASK)
#define PMC_LVDSC2_LVWIE_MASK                    (0x20U)
#define PMC_LVDSC2_LVWIE_SHIFT                   (5U)
#define PMC_LVDSC2_LVWIE(x)                      (((uint8_t)(((uint8_t)(x)) << PMC_LVDSC2_LVWIE_SHIFT)) & PMC_LVDSC2_LVWIE_MASK)
#define PMC_LVDSC2_LVWACK_MASK                   (0x40U)
#define PMC_LVDSC2_LVWACK_SHIFT                  (6U)
#define PMC_LVDSC2_LVWACK(x)                     (((uint8_t)(((uint8_t)(x)) << PMC_LVDSC2_LVWACK_SHIFT)) & PMC_LVDSC2_LVWACK_MASK)
#define PMC_LVDSC2_LVWF_MASK                     (0x80U)
#define PMC_LVDSC2_LVWF_SHIFT                    (7U)
#define PMC_LVDSC2_LVWF(x)                       (((uint8_t)(((uint8_t)(x)) << PMC_LVDSC2_LVWF_SHIFT)) & PMC_LVDSC2_LVWF_MASK)

/*! @name REGSC - Regulator Status And Control register */
#define PMC_REGSC_BGBE_MASK                      (0x1U)
#define PMC_REGSC_BGBE_SHIFT                     (0U)
#define PMC_REGSC_BGBE(x)                        (((uint8_t)(((uint8_t)(x)) << PMC_REGSC_BGBE_SHIFT)) & PMC_REGSC_BGBE_MASK)
#define PMC_REGSC_REGONS_MASK                    (0x4U)
#define PMC_REGSC_REGONS_SHIFT                   (2U)
#define PMC_REGSC_REGONS(x)                      (((uint8_t)(((uint8_t)(x)) << PMC_REGSC_REGONS_SHIFT)) & PMC_REGSC_REGONS_MASK)
#define PMC_REGSC_ACKISO_MASK                    (0x8U)
#define PMC_REGSC_ACKISO_SHIFT                   (3U)
#define PMC_REGSC_ACKISO(x)                      (((uint8_t)(((uint8_t)(x)) << PMC_REGSC_ACKISO_SHIFT)) & PMC_REGSC_ACKISO_MASK)
#define PMC_REGSC_VLPO_MASK                      (0x40U)
#define PMC_REGSC_VLPO_SHIFT                     (6U)
#define PMC_REGSC_VLPO(x)                        (((uint8_t)(((uint8_t)(x)) << PMC_REGSC_VLPO_SHIFT)) & PMC_REGSC_VLPO_MASK)


/*!
 * @}
 */ /* end of group PMC_Register_Masks */


/* PMC - Peripheral instance base addresses */
/** Peripheral PMC base address */
#define PMC_BASE                                 (0x4007D000u)
/** Peripheral PMC base pointer */
#define PMC                                      ((PMC_Type *)PMC_BASE)
/** Array initializer of PMC peripheral base addresses */
#define PMC_BASE_ADDRS                           { PMC_BASE }
/** Array initializer of PMC peripheral base pointers */
#define PMC_BASE_PTRS                            { PMC }
/** Interrupt vectors for the PMC peripheral type */
#define PMC_IRQS                                 { LVD_LVW_DCDC_IRQn }

/*!
 * @}
 */ /* end of group PMC_Peripheral_Access_Layer */


/* ----------------------------------------------------------------------------
   -- PORT Peripheral Access Layer
   ---------------------------------------------------------------------------- */

/*!
 * @addtogroup PORT_Peripheral_Access_Layer PORT Peripheral Access Layer
 * @{
 */

/** PORT - Register Layout Typedef */
typedef struct {
  __IO uint32_t PCR[32];                           /**< Pin Control Register n, array offset: 0x0, array step: 0x4 */
  __O  uint32_t GPCLR;                             /**< Global Pin Control Low Register, offset: 0x80 */
  __O  uint32_t GPCHR;                             /**< Global Pin Control High Register, offset: 0x84 */
       uint8_t RESERVED_0[24];
  __IO uint32_t ISFR;                              /**< Interrupt Status Flag Register, offset: 0xA0 */
} PORT_Type;

/* ----------------------------------------------------------------------------
   -- PORT Register Masks
   ---------------------------------------------------------------------------- */

/*!
 * @addtogroup PORT_Register_Masks PORT Register Masks
 * @{
 */

/*! @name PCR - Pin Control Register n */
#define PORT_PCR_PS_MASK                         (0x1U)
#define PORT_PCR_PS_SHIFT                        (0U)
#define PORT_PCR_PS(x)                           (((uint32_t)(((uint32_t)(x)) << PORT_PCR_PS_SHIFT)) & PORT_PCR_PS_MASK)
#define PORT_PCR_PE_MASK                         (0x2U)
#define PORT_PCR_PE_SHIFT                        (1U)
#define PORT_PCR_PE(x)                           (((uint32_t)(((uint32_t)(x)) << PORT_PCR_PE_SHIFT)) & PORT_PCR_PE_MASK)
#define PORT_PCR_SRE_MASK                        (0x4U)
#define PORT_PCR_SRE_SHIFT                       (2U)
#define PORT_PCR_SRE(x)                          (((uint32_t)(((uint32_t)(x)) << PORT_PCR_SRE_SHIFT)) & PORT_PCR_SRE_MASK)
#define PORT_PCR_PFE_MASK                        (0x10U)
#define PORT_PCR_PFE_SHIFT                       (4U)
#define PORT_PCR_PFE(x)                          (((uint32_t)(((uint32_t)(x)) << PORT_PCR_PFE_SHIFT)) & PORT_PCR_PFE_MASK)
#define PORT_PCR_DSE_MASK                        (0x40U)
#define PORT_PCR_DSE_SHIFT                       (6U)
#define PORT_PCR_DSE(x)                          (((uint32_t)(((uint32_t)(x)) << PORT_PCR_DSE_SHIFT)) & PORT_PCR_DSE_MASK)
#define PORT_PCR_MUX_MASK                        (0x700U)
#define PORT_PCR_MUX_SHIFT                       (8U)
#define PORT_PCR_MUX(x)                          (((uint32_t)(((uint32_t)(x)) << PORT_PCR_MUX_SHIFT)) & PORT_PCR_MUX_MASK)
#define PORT_PCR_IRQC_MASK                       (0xF0000U)
#define PORT_PCR_IRQC_SHIFT                      (16U)
#define PORT_PCR_IRQC(x)                         (((uint32_t)(((uint32_t)(x)) << PORT_PCR_IRQC_SHIFT)) & PORT_PCR_IRQC_MASK)
#define PORT_PCR_ISF_MASK                        (0x1000000U)
#define PORT_PCR_ISF_SHIFT                       (24U)
#define PORT_PCR_ISF(x)                          (((uint32_t)(((uint32_t)(x)) << PORT_PCR_ISF_SHIFT)) & PORT_PCR_ISF_MASK)

/* The count of PORT_PCR */
#define PORT_PCR_COUNT                           (32U)

/*! @name GPCLR - Global Pin Control Low Register */
#define PORT_GPCLR_GPWD_MASK                     (0xFFFFU)
#define PORT_GPCLR_GPWD_SHIFT                    (0U)
#define PORT_GPCLR_GPWD(x)                       (((uint32_t)(((uint32_t)(x)) << PORT_GPCLR_GPWD_SHIFT)) & PORT_GPCLR_GPWD_MASK)
#define PORT_GPCLR_GPWE_MASK                     (0xFFFF0000U)
#define PORT_GPCLR_GPWE_SHIFT                    (16U)
#define PORT_GPCLR_GPWE(x)                       (((uint32_t)(((uint32_t)(x)) << PORT_GPCLR_GPWE_SHIFT)) & PORT_GPCLR_GPWE_MASK)

/*! @name GPCHR - Global Pin Control High Register */
#define PORT_GPCHR_GPWD_MASK                     (0xFFFFU)
#define PORT_GPCHR_GPWD_SHIFT                    (0U)
#define PORT_GPCHR_GPWD(x)                       (((uint32_t)(((uint32_t)(x)) << PORT_GPCHR_GPWD_SHIFT)) & PORT_GPCHR_GPWD_MASK)
#define PORT_GPCHR_GPWE_MASK                     (0xFFFF0000U)
#define PORT_GPCHR_GPWE_SHIFT                    (16U)
#define PORT_GPCHR_GPWE(x)                       (((uint32_t)(((uint32_t)(x)) << PORT_GPCHR_GPWE_SHIFT)) & PORT_GPCHR_GPWE_MASK)

/*! @name ISFR - Interrupt Status Flag Register */
#define PORT_ISFR_ISF_MASK                       (0xFFFFFFFFU)
#define PORT_ISFR_ISF_SHIFT                      (0U)
#define PORT_ISFR_ISF(x)                         (((uint32_t)(((uint32_t)(x)) << PORT_ISFR_ISF_SHIFT)) & PORT_ISFR_ISF_MASK)


/*!
 * @}
 */ /* end of group PORT_Register_Masks */


/* PORT - Peripheral instance base addresses */
/** Peripheral PORTA base address */
#define PORTA_BASE                               (0x40049000u)
/** Peripheral PORTA base pointer */
#define PORTA                                    ((PORT_Type *)PORTA_BASE)
/** Peripheral PORTB base address */
#define PORTB_BASE                               (0x4004A000u)
/** Peripheral PORTB base pointer */
#define PORTB                                    ((PORT_Type *)PORTB_BASE)
/** Peripheral PORTC base address */
#define PORTC_BASE                               (0x4004B000u)
/** Peripheral PORTC base pointer */
#define PORTC                                    ((PORT_Type *)PORTC_BASE)
/** Array initializer of PORT peripheral base addresses */
#define PORT_BASE_ADDRS                          { PORTA_BASE, PORTB_BASE, PORTC_BASE }
/** Array initializer of PORT peripheral base pointers */
#define PORT_BASE_PTRS                           { PORTA, PORTB, PORTC }
/** Interrupt vectors for the PORT peripheral type */
#define PORT_IRQS                                { PORTA_IRQn, PORTB_PORTC_IRQn, PORTB_PORTC_IRQn }

/*!
 * @}
 */ /* end of group PORT_Peripheral_Access_Layer */


/* ----------------------------------------------------------------------------
   -- RCM Peripheral Access Layer
   ---------------------------------------------------------------------------- */

/*!
 * @addtogroup RCM_Peripheral_Access_Layer RCM Peripheral Access Layer
 * @{
 */

/** RCM - Register Layout Typedef */
typedef struct {
  __I  uint8_t SRS0;                               /**< System Reset Status Register 0, offset: 0x0 */
  __I  uint8_t SRS1;                               /**< System Reset Status Register 1, offset: 0x1 */
       uint8_t RESERVED_0[2];
  __IO uint8_t RPFC;                               /**< Reset Pin Filter Control register, offset: 0x4 */
  __IO uint8_t RPFW;                               /**< Reset Pin Filter Width register, offset: 0x5 */
} RCM_Type;

/* ----------------------------------------------------------------------------
   -- RCM Register Masks
   ---------------------------------------------------------------------------- */

/*!
 * @addtogroup RCM_Register_Masks RCM Register Masks
 * @{
 */

/*! @name SRS0 - System Reset Status Register 0 */
#define RCM_SRS0_WAKEUP_MASK                     (0x1U)
#define RCM_SRS0_WAKEUP_SHIFT                    (0U)
#define RCM_SRS0_WAKEUP(x)                       (((uint8_t)(((uint8_t)(x)) << RCM_SRS0_WAKEUP_SHIFT)) & RCM_SRS0_WAKEUP_MASK)
#define RCM_SRS0_LVD_MASK                        (0x2U)
#define RCM_SRS0_LVD_SHIFT                       (1U)
#define RCM_SRS0_LVD(x)                          (((uint8_t)(((uint8_t)(x)) << RCM_SRS0_LVD_SHIFT)) & RCM_SRS0_LVD_MASK)
#define RCM_SRS0_LOC_MASK                        (0x4U)
#define RCM_SRS0_LOC_SHIFT                       (2U)
#define RCM_SRS0_LOC(x)                          (((uint8_t)(((uint8_t)(x)) << RCM_SRS0_LOC_SHIFT)) & RCM_SRS0_LOC_MASK)
#define RCM_SRS0_WDOG_MASK                       (0x20U)
#define RCM_SRS0_WDOG_SHIFT                      (5U)
#define RCM_SRS0_WDOG(x)                         (((uint8_t)(((uint8_t)(x)) << RCM_SRS0_WDOG_SHIFT)) & RCM_SRS0_WDOG_MASK)
#define RCM_SRS0_PIN_MASK                        (0x40U)
#define RCM_SRS0_PIN_SHIFT                       (6U)
#define RCM_SRS0_PIN(x)                          (((uint8_t)(((uint8_t)(x)) << RCM_SRS0_PIN_SHIFT)) & RCM_SRS0_PIN_MASK)
#define RCM_SRS0_POR_MASK                        (0x80U)
#define RCM_SRS0_POR_SHIFT                       (7U)
#define RCM_SRS0_POR(x)                          (((uint8_t)(((uint8_t)(x)) << RCM_SRS0_POR_SHIFT)) & RCM_SRS0_POR_MASK)

/*! @name SRS1 - System Reset Status Register 1 */
#define RCM_SRS1_LOCKUP_MASK                     (0x2U)
#define RCM_SRS1_LOCKUP_SHIFT                    (1U)
#define RCM_SRS1_LOCKUP(x)                       (((uint8_t)(((uint8_t)(x)) << RCM_SRS1_LOCKUP_SHIFT)) & RCM_SRS1_LOCKUP_MASK)
#define RCM_SRS1_SW_MASK                         (0x4U)
#define RCM_SRS1_SW_SHIFT                        (2U)
#define RCM_SRS1_SW(x)                           (((uint8_t)(((uint8_t)(x)) << RCM_SRS1_SW_SHIFT)) & RCM_SRS1_SW_MASK)
#define RCM_SRS1_MDM_AP_MASK                     (0x8U)
#define RCM_SRS1_MDM_AP_SHIFT                    (3U)
#define RCM_SRS1_MDM_AP(x)                       (((uint8_t)(((uint8_t)(x)) << RCM_SRS1_MDM_AP_SHIFT)) & RCM_SRS1_MDM_AP_MASK)
#define RCM_SRS1_SACKERR_MASK                    (0x20U)
#define RCM_SRS1_SACKERR_SHIFT                   (5U)
#define RCM_SRS1_SACKERR(x)                      (((uint8_t)(((uint8_t)(x)) << RCM_SRS1_SACKERR_SHIFT)) & RCM_SRS1_SACKERR_MASK)

/*! @name RPFC - Reset Pin Filter Control register */
#define RCM_RPFC_RSTFLTSRW_MASK                  (0x3U)
#define RCM_RPFC_RSTFLTSRW_SHIFT                 (0U)
#define RCM_RPFC_RSTFLTSRW(x)                    (((uint8_t)(((uint8_t)(x)) << RCM_RPFC_RSTFLTSRW_SHIFT)) & RCM_RPFC_RSTFLTSRW_MASK)
#define RCM_RPFC_RSTFLTSS_MASK                   (0x4U)
#define RCM_RPFC_RSTFLTSS_SHIFT                  (2U)
#define RCM_RPFC_RSTFLTSS(x)                     (((uint8_t)(((uint8_t)(x)) << RCM_RPFC_RSTFLTSS_SHIFT)) & RCM_RPFC_RSTFLTSS_MASK)

/*! @name RPFW - Reset Pin Filter Width register */
#define RCM_RPFW_RSTFLTSEL_MASK                  (0x1FU)
#define RCM_RPFW_RSTFLTSEL_SHIFT                 (0U)
#define RCM_RPFW_RSTFLTSEL(x)                    (((uint8_t)(((uint8_t)(x)) << RCM_RPFW_RSTFLTSEL_SHIFT)) & RCM_RPFW_RSTFLTSEL_MASK)


/*!
 * @}
 */ /* end of group RCM_Register_Masks */


/* RCM - Peripheral instance base addresses */
/** Peripheral RCM base address */
#define RCM_BASE                                 (0x4007F000u)
/** Peripheral RCM base pointer */
#define RCM                                      ((RCM_Type *)RCM_BASE)
/** Array initializer of RCM peripheral base addresses */
#define RCM_BASE_ADDRS                           { RCM_BASE }
/** Array initializer of RCM peripheral base pointers */
#define RCM_BASE_PTRS                            { RCM }

/*!
 * @}
 */ /* end of group RCM_Peripheral_Access_Layer */


/* ----------------------------------------------------------------------------
   -- RFSYS Peripheral Access Layer
   ---------------------------------------------------------------------------- */

/*!
 * @addtogroup RFSYS_Peripheral_Access_Layer RFSYS Peripheral Access Layer
 * @{
 */

/** RFSYS - Register Layout Typedef */
typedef struct {
  __IO uint32_t REG[8];                            /**< Register file register, array offset: 0x0, array step: 0x4 */
} RFSYS_Type;

/* ----------------------------------------------------------------------------
   -- RFSYS Register Masks
   ---------------------------------------------------------------------------- */

/*!
 * @addtogroup RFSYS_Register_Masks RFSYS Register Masks
 * @{
 */

/*! @name REG - Register file register */
#define RFSYS_REG_LL_MASK                        (0xFFU)
#define RFSYS_REG_LL_SHIFT                       (0U)
#define RFSYS_REG_LL(x)                          (((uint32_t)(((uint32_t)(x)) << RFSYS_REG_LL_SHIFT)) & RFSYS_REG_LL_MASK)
#define RFSYS_REG_LH_MASK                        (0xFF00U)
#define RFSYS_REG_LH_SHIFT                       (8U)
#define RFSYS_REG_LH(x)                          (((uint32_t)(((uint32_t)(x)) << RFSYS_REG_LH_SHIFT)) & RFSYS_REG_LH_MASK)
#define RFSYS_REG_HL_MASK                        (0xFF0000U)
#define RFSYS_REG_HL_SHIFT                       (16U)
#define RFSYS_REG_HL(x)                          (((uint32_t)(((uint32_t)(x)) << RFSYS_REG_HL_SHIFT)) & RFSYS_REG_HL_MASK)
#define RFSYS_REG_HH_MASK                        (0xFF000000U)
#define RFSYS_REG_HH_SHIFT                       (24U)
#define RFSYS_REG_HH(x)                          (((uint32_t)(((uint32_t)(x)) << RFSYS_REG_HH_SHIFT)) & RFSYS_REG_HH_MASK)

/* The count of RFSYS_REG */
#define RFSYS_REG_COUNT                          (8U)


/*!
 * @}
 */ /* end of group RFSYS_Register_Masks */


/* RFSYS - Peripheral instance base addresses */
/** Peripheral RFSYS base address */
#define RFSYS_BASE                               (0x40041000u)
/** Peripheral RFSYS base pointer */
#define RFSYS                                    ((RFSYS_Type *)RFSYS_BASE)
/** Array initializer of RFSYS peripheral base addresses */
#define RFSYS_BASE_ADDRS                         { RFSYS_BASE }
/** Array initializer of RFSYS peripheral base pointers */
#define RFSYS_BASE_PTRS                          { RFSYS }

/*!
 * @}
 */ /* end of group RFSYS_Peripheral_Access_Layer */


/* ----------------------------------------------------------------------------
   -- ROM Peripheral Access Layer
   ---------------------------------------------------------------------------- */

/*!
 * @addtogroup ROM_Peripheral_Access_Layer ROM Peripheral Access Layer
 * @{
 */

/** ROM - Register Layout Typedef */
typedef struct {
  __I  uint32_t ENTRY[3];                          /**< Entry, array offset: 0x0, array step: 0x4 */
  __I  uint32_t TABLEMARK;                         /**< End of Table Marker Register, offset: 0xC */
       uint8_t RESERVED_0[4028];
  __I  uint32_t SYSACCESS;                         /**< System Access Register, offset: 0xFCC */
  __I  uint32_t PERIPHID4;                         /**< Peripheral ID Register, offset: 0xFD0 */
  __I  uint32_t PERIPHID5;                         /**< Peripheral ID Register, offset: 0xFD4 */
  __I  uint32_t PERIPHID6;                         /**< Peripheral ID Register, offset: 0xFD8 */
  __I  uint32_t PERIPHID7;                         /**< Peripheral ID Register, offset: 0xFDC */
  __I  uint32_t PERIPHID0;                         /**< Peripheral ID Register, offset: 0xFE0 */
  __I  uint32_t PERIPHID1;                         /**< Peripheral ID Register, offset: 0xFE4 */
  __I  uint32_t PERIPHID2;                         /**< Peripheral ID Register, offset: 0xFE8 */
  __I  uint32_t PERIPHID3;                         /**< Peripheral ID Register, offset: 0xFEC */
  __I  uint32_t COMPID[4];                         /**< Component ID Register, array offset: 0xFF0, array step: 0x4 */
} ROM_Type;

/* ----------------------------------------------------------------------------
   -- ROM Register Masks
   ---------------------------------------------------------------------------- */

/*!
 * @addtogroup ROM_Register_Masks ROM Register Masks
 * @{
 */

/*! @name ENTRY - Entry */
#define ROM_ENTRY_ENTRY_MASK                     (0xFFFFFFFFU)
#define ROM_ENTRY_ENTRY_SHIFT                    (0U)
#define ROM_ENTRY_ENTRY(x)                       (((uint32_t)(((uint32_t)(x)) << ROM_ENTRY_ENTRY_SHIFT)) & ROM_ENTRY_ENTRY_MASK)

/* The count of ROM_ENTRY */
#define ROM_ENTRY_COUNT                          (3U)

/*! @name TABLEMARK - End of Table Marker Register */
#define ROM_TABLEMARK_MARK_MASK                  (0xFFFFFFFFU)
#define ROM_TABLEMARK_MARK_SHIFT                 (0U)
#define ROM_TABLEMARK_MARK(x)                    (((uint32_t)(((uint32_t)(x)) << ROM_TABLEMARK_MARK_SHIFT)) & ROM_TABLEMARK_MARK_MASK)

/*! @name SYSACCESS - System Access Register */
#define ROM_SYSACCESS_SYSACCESS_MASK             (0xFFFFFFFFU)
#define ROM_SYSACCESS_SYSACCESS_SHIFT            (0U)
#define ROM_SYSACCESS_SYSACCESS(x)               (((uint32_t)(((uint32_t)(x)) << ROM_SYSACCESS_SYSACCESS_SHIFT)) & ROM_SYSACCESS_SYSACCESS_MASK)

/*! @name PERIPHID4 - Peripheral ID Register */
#define ROM_PERIPHID4_PERIPHID_MASK              (0xFFFFFFFFU)
#define ROM_PERIPHID4_PERIPHID_SHIFT             (0U)
#define ROM_PERIPHID4_PERIPHID(x)                (((uint32_t)(((uint32_t)(x)) << ROM_PERIPHID4_PERIPHID_SHIFT)) & ROM_PERIPHID4_PERIPHID_MASK)

/*! @name PERIPHID5 - Peripheral ID Register */
#define ROM_PERIPHID5_PERIPHID_MASK              (0xFFFFFFFFU)
#define ROM_PERIPHID5_PERIPHID_SHIFT             (0U)
#define ROM_PERIPHID5_PERIPHID(x)                (((uint32_t)(((uint32_t)(x)) << ROM_PERIPHID5_PERIPHID_SHIFT)) & ROM_PERIPHID5_PERIPHID_MASK)

/*! @name PERIPHID6 - Peripheral ID Register */
#define ROM_PERIPHID6_PERIPHID_MASK              (0xFFFFFFFFU)
#define ROM_PERIPHID6_PERIPHID_SHIFT             (0U)
#define ROM_PERIPHID6_PERIPHID(x)                (((uint32_t)(((uint32_t)(x)) << ROM_PERIPHID6_PERIPHID_SHIFT)) & ROM_PERIPHID6_PERIPHID_MASK)

/*! @name PERIPHID7 - Peripheral ID Register */
#define ROM_PERIPHID7_PERIPHID_MASK              (0xFFFFFFFFU)
#define ROM_PERIPHID7_PERIPHID_SHIFT             (0U)
#define ROM_PERIPHID7_PERIPHID(x)                (((uint32_t)(((uint32_t)(x)) << ROM_PERIPHID7_PERIPHID_SHIFT)) & ROM_PERIPHID7_PERIPHID_MASK)

/*! @name PERIPHID0 - Peripheral ID Register */
#define ROM_PERIPHID0_PERIPHID_MASK              (0xFFFFFFFFU)
#define ROM_PERIPHID0_PERIPHID_SHIFT             (0U)
#define ROM_PERIPHID0_PERIPHID(x)                (((uint32_t)(((uint32_t)(x)) << ROM_PERIPHID0_PERIPHID_SHIFT)) & ROM_PERIPHID0_PERIPHID_MASK)

/*! @name PERIPHID1 - Peripheral ID Register */
#define ROM_PERIPHID1_PERIPHID_MASK              (0xFFFFFFFFU)
#define ROM_PERIPHID1_PERIPHID_SHIFT             (0U)
#define ROM_PERIPHID1_PERIPHID(x)                (((uint32_t)(((uint32_t)(x)) << ROM_PERIPHID1_PERIPHID_SHIFT)) & ROM_PERIPHID1_PERIPHID_MASK)

/*! @name PERIPHID2 - Peripheral ID Register */
#define ROM_PERIPHID2_PERIPHID_MASK              (0xFFFFFFFFU)
#define ROM_PERIPHID2_PERIPHID_SHIFT             (0U)
#define ROM_PERIPHID2_PERIPHID(x)                (((uint32_t)(((uint32_t)(x)) << ROM_PERIPHID2_PERIPHID_SHIFT)) & ROM_PERIPHID2_PERIPHID_MASK)

/*! @name PERIPHID3 - Peripheral ID Register */
#define ROM_PERIPHID3_PERIPHID_MASK              (0xFFFFFFFFU)
#define ROM_PERIPHID3_PERIPHID_SHIFT             (0U)
#define ROM_PERIPHID3_PERIPHID(x)                (((uint32_t)(((uint32_t)(x)) << ROM_PERIPHID3_PERIPHID_SHIFT)) & ROM_PERIPHID3_PERIPHID_MASK)

/*! @name COMPID - Component ID Register */
#define ROM_COMPID_COMPID_MASK                   (0xFFFFFFFFU)
#define ROM_COMPID_COMPID_SHIFT                  (0U)
#define ROM_COMPID_COMPID(x)                     (((uint32_t)(((uint32_t)(x)) << ROM_COMPID_COMPID_SHIFT)) & ROM_COMPID_COMPID_MASK)

/* The count of ROM_COMPID */
#define ROM_COMPID_COUNT                         (4U)


/*!
 * @}
 */ /* end of group ROM_Register_Masks */


/* ROM - Peripheral instance base addresses */
/** Peripheral ROM base address */
#define ROM_BASE                                 (0xF0002000u)
/** Peripheral ROM base pointer */
#define ROM                                      ((ROM_Type *)ROM_BASE)
/** Array initializer of ROM peripheral base addresses */
#define ROM_BASE_ADDRS                           { ROM_BASE }
/** Array initializer of ROM peripheral base pointers */
#define ROM_BASE_PTRS                            { ROM }

/*!
 * @}
 */ /* end of group ROM_Peripheral_Access_Layer */


/* ----------------------------------------------------------------------------
   -- RSIM Peripheral Access Layer
   ---------------------------------------------------------------------------- */

/*!
 * @addtogroup RSIM_Peripheral_Access_Layer RSIM Peripheral Access Layer
 * @{
 */

/** RSIM - Register Layout Typedef */
typedef struct {
  __IO uint32_t CONTROL;                           /**< Radio System Control, offset: 0x0 */
  __IO uint32_t ACTIVE_DELAY;                      /**< Radio Active Early Warning, offset: 0x4 */
  __I  uint32_t MAC_MSB;                           /**< Radio MAC Address, offset: 0x8 */
  __I  uint32_t MAC_LSB;                           /**< Radio MAC Address, offset: 0xC */
  __IO uint32_t MISC;                              /**< Radio Miscellaneous, offset: 0x10 */
       uint8_t RESERVED_0[236];
  __I  uint32_t DSM_TIMER;                         /**< Deep Sleep Timer, offset: 0x100 */
  __IO uint32_t DSM_CONTROL;                       /**< Deep Sleep Timer Control, offset: 0x104 */
  __IO uint32_t DSM_OSC_OFFSET;                    /**< Deep Sleep Wakeup Time Offset, offset: 0x108 */
  __IO uint32_t ANT_SLEEP;                         /**< ANT Link Layer Sleep Time, offset: 0x10C */
  __IO uint32_t ANT_WAKE;                          /**< ANT Link Layer Wake Time, offset: 0x110 */
  __IO uint32_t ZIG_SLEEP;                         /**< 802.15.4 Link Layer Sleep Time, offset: 0x114 */
  __IO uint32_t ZIG_WAKE;                          /**< 802.15.4 Link Layer Wake Time, offset: 0x118 */
  __IO uint32_t GEN_SLEEP;                         /**< Generic FSK Link Layer Sleep Time, offset: 0x11C */
  __IO uint32_t GEN_WAKE;                          /**< Generic FSK Link Layer Wake Time, offset: 0x120 */
  __IO uint32_t RF_OSC_CTRL;                       /**< Radio Oscillator Control, offset: 0x124 */
  __IO uint32_t ANA_TEST;                          /**< Radio Analog Test Registers, offset: 0x128 */
  __IO uint32_t ANA_TRIM;                          /**< Radio Analog Trim Registers, offset: 0x12C */
} RSIM_Type;

/* ----------------------------------------------------------------------------
   -- RSIM Register Masks
   ---------------------------------------------------------------------------- */

/*!
 * @addtogroup RSIM_Register_Masks RSIM Register Masks
 * @{
 */

/*! @name CONTROL - Radio System Control */
#define RSIM_CONTROL_BLE_RF_OSC_REQ_EN_MASK      (0x1U)
#define RSIM_CONTROL_BLE_RF_OSC_REQ_EN_SHIFT     (0U)
#define RSIM_CONTROL_BLE_RF_OSC_REQ_EN(x)        (((uint32_t)(((uint32_t)(x)) << RSIM_CONTROL_BLE_RF_OSC_REQ_EN_SHIFT)) & RSIM_CONTROL_BLE_RF_OSC_REQ_EN_MASK)
#define RSIM_CONTROL_BLE_RF_OSC_REQ_STAT_MASK    (0x2U)
#define RSIM_CONTROL_BLE_RF_OSC_REQ_STAT_SHIFT   (1U)
#define RSIM_CONTROL_BLE_RF_OSC_REQ_STAT(x)      (((uint32_t)(((uint32_t)(x)) << RSIM_CONTROL_BLE_RF_OSC_REQ_STAT_SHIFT)) & RSIM_CONTROL_BLE_RF_OSC_REQ_STAT_MASK)
#define RSIM_CONTROL_BLE_RF_OSC_REQ_INT_EN_MASK  (0x10U)
#define RSIM_CONTROL_BLE_RF_OSC_REQ_INT_EN_SHIFT (4U)
#define RSIM_CONTROL_BLE_RF_OSC_REQ_INT_EN(x)    (((uint32_t)(((uint32_t)(x)) << RSIM_CONTROL_BLE_RF_OSC_REQ_INT_EN_SHIFT)) & RSIM_CONTROL_BLE_RF_OSC_REQ_INT_EN_MASK)
#define RSIM_CONTROL_BLE_RF_OSC_REQ_INT_MASK     (0x20U)
#define RSIM_CONTROL_BLE_RF_OSC_REQ_INT_SHIFT    (5U)
#define RSIM_CONTROL_BLE_RF_OSC_REQ_INT(x)       (((uint32_t)(((uint32_t)(x)) << RSIM_CONTROL_BLE_RF_OSC_REQ_INT_SHIFT)) & RSIM_CONTROL_BLE_RF_OSC_REQ_INT_MASK)
#define RSIM_CONTROL_RF_OSC_EN_MASK              (0xF00U)
#define RSIM_CONTROL_RF_OSC_EN_SHIFT             (8U)
#define RSIM_CONTROL_RF_OSC_EN(x)                (((uint32_t)(((uint32_t)(x)) << RSIM_CONTROL_RF_OSC_EN_SHIFT)) & RSIM_CONTROL_RF_OSC_EN_MASK)
#define RSIM_CONTROL_RADIO_GASKET_BYPASS_OVRD_EN_MASK (0x1000U)
#define RSIM_CONTROL_RADIO_GASKET_BYPASS_OVRD_EN_SHIFT (12U)
#define RSIM_CONTROL_RADIO_GASKET_BYPASS_OVRD_EN(x) (((uint32_t)(((uint32_t)(x)) << RSIM_CONTROL_RADIO_GASKET_BYPASS_OVRD_EN_SHIFT)) & RSIM_CONTROL_RADIO_GASKET_BYPASS_OVRD_EN_MASK)
#define RSIM_CONTROL_RADIO_GASKET_BYPASS_OVRD_MASK (0x2000U)
#define RSIM_CONTROL_RADIO_GASKET_BYPASS_OVRD_SHIFT (13U)
#define RSIM_CONTROL_RADIO_GASKET_BYPASS_OVRD(x) (((uint32_t)(((uint32_t)(x)) << RSIM_CONTROL_RADIO_GASKET_BYPASS_OVRD_SHIFT)) & RSIM_CONTROL_RADIO_GASKET_BYPASS_OVRD_MASK)
#define RSIM_CONTROL_IPP_OBE_3V_BLE_ACTIVE_1_MASK (0x10000U)
#define RSIM_CONTROL_IPP_OBE_3V_BLE_ACTIVE_1_SHIFT (16U)
#define RSIM_CONTROL_IPP_OBE_3V_BLE_ACTIVE_1(x)  (((uint32_t)(((uint32_t)(x)) << RSIM_CONTROL_IPP_OBE_3V_BLE_ACTIVE_1_SHIFT)) & RSIM_CONTROL_IPP_OBE_3V_BLE_ACTIVE_1_MASK)
#define RSIM_CONTROL_IPP_OBE_3V_BLE_ACTIVE_2_MASK (0x20000U)
#define RSIM_CONTROL_IPP_OBE_3V_BLE_ACTIVE_2_SHIFT (17U)
#define RSIM_CONTROL_IPP_OBE_3V_BLE_ACTIVE_2(x)  (((uint32_t)(((uint32_t)(x)) << RSIM_CONTROL_IPP_OBE_3V_BLE_ACTIVE_2_SHIFT)) & RSIM_CONTROL_IPP_OBE_3V_BLE_ACTIVE_2_MASK)
#define RSIM_CONTROL_RADIO_RAM_ACCESS_OVRD_EN_MASK (0x40000U)
#define RSIM_CONTROL_RADIO_RAM_ACCESS_OVRD_EN_SHIFT (18U)
#define RSIM_CONTROL_RADIO_RAM_ACCESS_OVRD_EN(x) (((uint32_t)(((uint32_t)(x)) << RSIM_CONTROL_RADIO_RAM_ACCESS_OVRD_EN_SHIFT)) & RSIM_CONTROL_RADIO_RAM_ACCESS_OVRD_EN_MASK)
#define RSIM_CONTROL_RADIO_RAM_ACCESS_OVRD_MASK  (0x80000U)
#define RSIM_CONTROL_RADIO_RAM_ACCESS_OVRD_SHIFT (19U)
#define RSIM_CONTROL_RADIO_RAM_ACCESS_OVRD(x)    (((uint32_t)(((uint32_t)(x)) << RSIM_CONTROL_RADIO_RAM_ACCESS_OVRD_SHIFT)) & RSIM_CONTROL_RADIO_RAM_ACCESS_OVRD_MASK)
#define RSIM_CONTROL_RSIM_DSM_EXIT_MASK          (0x100000U)
#define RSIM_CONTROL_RSIM_DSM_EXIT_SHIFT         (20U)
#define RSIM_CONTROL_RSIM_DSM_EXIT(x)            (((uint32_t)(((uint32_t)(x)) << RSIM_CONTROL_RSIM_DSM_EXIT_SHIFT)) & RSIM_CONTROL_RSIM_DSM_EXIT_MASK)
#define RSIM_CONTROL_RSIM_STOP_ACK_OVRD_EN_MASK  (0x400000U)
#define RSIM_CONTROL_RSIM_STOP_ACK_OVRD_EN_SHIFT (22U)
#define RSIM_CONTROL_RSIM_STOP_ACK_OVRD_EN(x)    (((uint32_t)(((uint32_t)(x)) << RSIM_CONTROL_RSIM_STOP_ACK_OVRD_EN_SHIFT)) & RSIM_CONTROL_RSIM_STOP_ACK_OVRD_EN_MASK)
#define RSIM_CONTROL_RSIM_STOP_ACK_OVRD_MASK     (0x800000U)
#define RSIM_CONTROL_RSIM_STOP_ACK_OVRD_SHIFT    (23U)
#define RSIM_CONTROL_RSIM_STOP_ACK_OVRD(x)       (((uint32_t)(((uint32_t)(x)) << RSIM_CONTROL_RSIM_STOP_ACK_OVRD_SHIFT)) & RSIM_CONTROL_RSIM_STOP_ACK_OVRD_MASK)
#define RSIM_CONTROL_RF_OSC_READY_MASK           (0x1000000U)
#define RSIM_CONTROL_RF_OSC_READY_SHIFT          (24U)
#define RSIM_CONTROL_RF_OSC_READY(x)             (((uint32_t)(((uint32_t)(x)) << RSIM_CONTROL_RF_OSC_READY_SHIFT)) & RSIM_CONTROL_RF_OSC_READY_MASK)
#define RSIM_CONTROL_RF_OSC_READY_OVRD_EN_MASK   (0x2000000U)
#define RSIM_CONTROL_RF_OSC_READY_OVRD_EN_SHIFT  (25U)
#define RSIM_CONTROL_RF_OSC_READY_OVRD_EN(x)     (((uint32_t)(((uint32_t)(x)) << RSIM_CONTROL_RF_OSC_READY_OVRD_EN_SHIFT)) & RSIM_CONTROL_RF_OSC_READY_OVRD_EN_MASK)
#define RSIM_CONTROL_RF_OSC_READY_OVRD_MASK      (0x4000000U)
#define RSIM_CONTROL_RF_OSC_READY_OVRD_SHIFT     (26U)
#define RSIM_CONTROL_RF_OSC_READY_OVRD(x)        (((uint32_t)(((uint32_t)(x)) << RSIM_CONTROL_RF_OSC_READY_OVRD_SHIFT)) & RSIM_CONTROL_RF_OSC_READY_OVRD_MASK)
#define RSIM_CONTROL_BLOCK_SOC_RESETS_MASK       (0x10000000U)
#define RSIM_CONTROL_BLOCK_SOC_RESETS_SHIFT      (28U)
#define RSIM_CONTROL_BLOCK_SOC_RESETS(x)         (((uint32_t)(((uint32_t)(x)) << RSIM_CONTROL_BLOCK_SOC_RESETS_SHIFT)) & RSIM_CONTROL_BLOCK_SOC_RESETS_MASK)
#define RSIM_CONTROL_BLOCK_RADIO_OUTPUTS_MASK    (0x20000000U)
#define RSIM_CONTROL_BLOCK_RADIO_OUTPUTS_SHIFT   (29U)
#define RSIM_CONTROL_BLOCK_RADIO_OUTPUTS(x)      (((uint32_t)(((uint32_t)(x)) << RSIM_CONTROL_BLOCK_RADIO_OUTPUTS_SHIFT)) & RSIM_CONTROL_BLOCK_RADIO_OUTPUTS_MASK)
#define RSIM_CONTROL_ALLOW_DFT_RESETS_MASK       (0x40000000U)
#define RSIM_CONTROL_ALLOW_DFT_RESETS_SHIFT      (30U)
#define RSIM_CONTROL_ALLOW_DFT_RESETS(x)         (((uint32_t)(((uint32_t)(x)) << RSIM_CONTROL_ALLOW_DFT_RESETS_SHIFT)) & RSIM_CONTROL_ALLOW_DFT_RESETS_MASK)
#define RSIM_CONTROL_RADIO_RESET_BIT_MASK        (0x80000000U)
#define RSIM_CONTROL_RADIO_RESET_BIT_SHIFT       (31U)
#define RSIM_CONTROL_RADIO_RESET_BIT(x)          (((uint32_t)(((uint32_t)(x)) << RSIM_CONTROL_RADIO_RESET_BIT_SHIFT)) & RSIM_CONTROL_RADIO_RESET_BIT_MASK)

/*! @name ACTIVE_DELAY - Radio Active Early Warning */
#define RSIM_ACTIVE_DELAY_BLE_FINE_DELAY_MASK    (0x3FU)
#define RSIM_ACTIVE_DELAY_BLE_FINE_DELAY_SHIFT   (0U)
#define RSIM_ACTIVE_DELAY_BLE_FINE_DELAY(x)      (((uint32_t)(((uint32_t)(x)) << RSIM_ACTIVE_DELAY_BLE_FINE_DELAY_SHIFT)) & RSIM_ACTIVE_DELAY_BLE_FINE_DELAY_MASK)
#define RSIM_ACTIVE_DELAY_BLE_COARSE_DELAY_MASK  (0xF0000U)
#define RSIM_ACTIVE_DELAY_BLE_COARSE_DELAY_SHIFT (16U)
#define RSIM_ACTIVE_DELAY_BLE_COARSE_DELAY(x)    (((uint32_t)(((uint32_t)(x)) << RSIM_ACTIVE_DELAY_BLE_COARSE_DELAY_SHIFT)) & RSIM_ACTIVE_DELAY_BLE_COARSE_DELAY_MASK)

/*! @name MAC_MSB - Radio MAC Address */
#define RSIM_MAC_MSB_MAC_ADDR_MSB_MASK           (0xFFU)
#define RSIM_MAC_MSB_MAC_ADDR_MSB_SHIFT          (0U)
#define RSIM_MAC_MSB_MAC_ADDR_MSB(x)             (((uint32_t)(((uint32_t)(x)) << RSIM_MAC_MSB_MAC_ADDR_MSB_SHIFT)) & RSIM_MAC_MSB_MAC_ADDR_MSB_MASK)

/*! @name MAC_LSB - Radio MAC Address */
#define RSIM_MAC_LSB_MAC_ADDR_LSB_MASK           (0xFFFFFFFFU)
#define RSIM_MAC_LSB_MAC_ADDR_LSB_SHIFT          (0U)
#define RSIM_MAC_LSB_MAC_ADDR_LSB(x)             (((uint32_t)(((uint32_t)(x)) << RSIM_MAC_LSB_MAC_ADDR_LSB_SHIFT)) & RSIM_MAC_LSB_MAC_ADDR_LSB_MASK)

/*! @name MISC - Radio Miscellaneous */
#define RSIM_MISC_ANALOG_TEST_EN_MASK            (0x1FU)
#define RSIM_MISC_ANALOG_TEST_EN_SHIFT           (0U)
#define RSIM_MISC_ANALOG_TEST_EN(x)              (((uint32_t)(((uint32_t)(x)) << RSIM_MISC_ANALOG_TEST_EN_SHIFT)) & RSIM_MISC_ANALOG_TEST_EN_MASK)
#define RSIM_MISC_RADIO_VERSION_MASK             (0xFF000000U)
#define RSIM_MISC_RADIO_VERSION_SHIFT            (24U)
#define RSIM_MISC_RADIO_VERSION(x)               (((uint32_t)(((uint32_t)(x)) << RSIM_MISC_RADIO_VERSION_SHIFT)) & RSIM_MISC_RADIO_VERSION_MASK)

/*! @name DSM_TIMER - Deep Sleep Timer */
#define RSIM_DSM_TIMER_DSM_TIMER_MASK            (0xFFFFFFU)
#define RSIM_DSM_TIMER_DSM_TIMER_SHIFT           (0U)
#define RSIM_DSM_TIMER_DSM_TIMER(x)              (((uint32_t)(((uint32_t)(x)) << RSIM_DSM_TIMER_DSM_TIMER_SHIFT)) & RSIM_DSM_TIMER_DSM_TIMER_MASK)

/*! @name DSM_CONTROL - Deep Sleep Timer Control */
#define RSIM_DSM_CONTROL_DSM_ANT_READY_MASK      (0x1U)
#define RSIM_DSM_CONTROL_DSM_ANT_READY_SHIFT     (0U)
#define RSIM_DSM_CONTROL_DSM_ANT_READY(x)        (((uint32_t)(((uint32_t)(x)) << RSIM_DSM_CONTROL_DSM_ANT_READY_SHIFT)) & RSIM_DSM_CONTROL_DSM_ANT_READY_MASK)
#define RSIM_DSM_CONTROL_ANT_DEEP_SLEEP_STATUS_MASK (0x2U)
#define RSIM_DSM_CONTROL_ANT_DEEP_SLEEP_STATUS_SHIFT (1U)
#define RSIM_DSM_CONTROL_ANT_DEEP_SLEEP_STATUS(x) (((uint32_t)(((uint32_t)(x)) << RSIM_DSM_CONTROL_ANT_DEEP_SLEEP_STATUS_SHIFT)) & RSIM_DSM_CONTROL_ANT_DEEP_SLEEP_STATUS_MASK)
#define RSIM_DSM_CONTROL_DSM_ANT_FINISHED_MASK   (0x4U)
#define RSIM_DSM_CONTROL_DSM_ANT_FINISHED_SHIFT  (2U)
#define RSIM_DSM_CONTROL_DSM_ANT_FINISHED(x)     (((uint32_t)(((uint32_t)(x)) << RSIM_DSM_CONTROL_DSM_ANT_FINISHED_SHIFT)) & RSIM_DSM_CONTROL_DSM_ANT_FINISHED_MASK)
#define RSIM_DSM_CONTROL_ANT_SYSCLK_REQUEST_EN_MASK (0x8U)
#define RSIM_DSM_CONTROL_ANT_SYSCLK_REQUEST_EN_SHIFT (3U)
#define RSIM_DSM_CONTROL_ANT_SYSCLK_REQUEST_EN(x) (((uint32_t)(((uint32_t)(x)) << RSIM_DSM_CONTROL_ANT_SYSCLK_REQUEST_EN_SHIFT)) & RSIM_DSM_CONTROL_ANT_SYSCLK_REQUEST_EN_MASK)
#define RSIM_DSM_CONTROL_ANT_SLEEP_REQUEST_MASK  (0x10U)
#define RSIM_DSM_CONTROL_ANT_SLEEP_REQUEST_SHIFT (4U)
#define RSIM_DSM_CONTROL_ANT_SLEEP_REQUEST(x)    (((uint32_t)(((uint32_t)(x)) << RSIM_DSM_CONTROL_ANT_SLEEP_REQUEST_SHIFT)) & RSIM_DSM_CONTROL_ANT_SLEEP_REQUEST_MASK)
#define RSIM_DSM_CONTROL_ANT_SYSCLK_REQ_MASK     (0x20U)
#define RSIM_DSM_CONTROL_ANT_SYSCLK_REQ_SHIFT    (5U)
#define RSIM_DSM_CONTROL_ANT_SYSCLK_REQ(x)       (((uint32_t)(((uint32_t)(x)) << RSIM_DSM_CONTROL_ANT_SYSCLK_REQ_SHIFT)) & RSIM_DSM_CONTROL_ANT_SYSCLK_REQ_MASK)
#define RSIM_DSM_CONTROL_ANT_SYSCLK_INTERRUPT_EN_MASK (0x40U)
#define RSIM_DSM_CONTROL_ANT_SYSCLK_INTERRUPT_EN_SHIFT (6U)
#define RSIM_DSM_CONTROL_ANT_SYSCLK_INTERRUPT_EN(x) (((uint32_t)(((uint32_t)(x)) << RSIM_DSM_CONTROL_ANT_SYSCLK_INTERRUPT_EN_SHIFT)) & RSIM_DSM_CONTROL_ANT_SYSCLK_INTERRUPT_EN_MASK)
#define RSIM_DSM_CONTROL_ANT_SYSCLK_REQ_INT_MASK (0x80U)
#define RSIM_DSM_CONTROL_ANT_SYSCLK_REQ_INT_SHIFT (7U)
#define RSIM_DSM_CONTROL_ANT_SYSCLK_REQ_INT(x)   (((uint32_t)(((uint32_t)(x)) << RSIM_DSM_CONTROL_ANT_SYSCLK_REQ_INT_SHIFT)) & RSIM_DSM_CONTROL_ANT_SYSCLK_REQ_INT_MASK)
#define RSIM_DSM_CONTROL_DSM_GEN_READY_MASK      (0x100U)
#define RSIM_DSM_CONTROL_DSM_GEN_READY_SHIFT     (8U)
#define RSIM_DSM_CONTROL_DSM_GEN_READY(x)        (((uint32_t)(((uint32_t)(x)) << RSIM_DSM_CONTROL_DSM_GEN_READY_SHIFT)) & RSIM_DSM_CONTROL_DSM_GEN_READY_MASK)
#define RSIM_DSM_CONTROL_GEN_DEEP_SLEEP_STATUS_MASK (0x200U)
#define RSIM_DSM_CONTROL_GEN_DEEP_SLEEP_STATUS_SHIFT (9U)
#define RSIM_DSM_CONTROL_GEN_DEEP_SLEEP_STATUS(x) (((uint32_t)(((uint32_t)(x)) << RSIM_DSM_CONTROL_GEN_DEEP_SLEEP_STATUS_SHIFT)) & RSIM_DSM_CONTROL_GEN_DEEP_SLEEP_STATUS_MASK)
#define RSIM_DSM_CONTROL_DSM_GEN_FINISHED_MASK   (0x400U)
#define RSIM_DSM_CONTROL_DSM_GEN_FINISHED_SHIFT  (10U)
#define RSIM_DSM_CONTROL_DSM_GEN_FINISHED(x)     (((uint32_t)(((uint32_t)(x)) << RSIM_DSM_CONTROL_DSM_GEN_FINISHED_SHIFT)) & RSIM_DSM_CONTROL_DSM_GEN_FINISHED_MASK)
#define RSIM_DSM_CONTROL_GEN_SYSCLK_REQUEST_EN_MASK (0x800U)
#define RSIM_DSM_CONTROL_GEN_SYSCLK_REQUEST_EN_SHIFT (11U)
#define RSIM_DSM_CONTROL_GEN_SYSCLK_REQUEST_EN(x) (((uint32_t)(((uint32_t)(x)) << RSIM_DSM_CONTROL_GEN_SYSCLK_REQUEST_EN_SHIFT)) & RSIM_DSM_CONTROL_GEN_SYSCLK_REQUEST_EN_MASK)
#define RSIM_DSM_CONTROL_GEN_SLEEP_REQUEST_MASK  (0x1000U)
#define RSIM_DSM_CONTROL_GEN_SLEEP_REQUEST_SHIFT (12U)
#define RSIM_DSM_CONTROL_GEN_SLEEP_REQUEST(x)    (((uint32_t)(((uint32_t)(x)) << RSIM_DSM_CONTROL_GEN_SLEEP_REQUEST_SHIFT)) & RSIM_DSM_CONTROL_GEN_SLEEP_REQUEST_MASK)
#define RSIM_DSM_CONTROL_GEN_SYSCLK_REQ_MASK     (0x2000U)
#define RSIM_DSM_CONTROL_GEN_SYSCLK_REQ_SHIFT    (13U)
#define RSIM_DSM_CONTROL_GEN_SYSCLK_REQ(x)       (((uint32_t)(((uint32_t)(x)) << RSIM_DSM_CONTROL_GEN_SYSCLK_REQ_SHIFT)) & RSIM_DSM_CONTROL_GEN_SYSCLK_REQ_MASK)
#define RSIM_DSM_CONTROL_GEN_SYSCLK_INTERRUPT_EN_MASK (0x4000U)
#define RSIM_DSM_CONTROL_GEN_SYSCLK_INTERRUPT_EN_SHIFT (14U)
#define RSIM_DSM_CONTROL_GEN_SYSCLK_INTERRUPT_EN(x) (((uint32_t)(((uint32_t)(x)) << RSIM_DSM_CONTROL_GEN_SYSCLK_INTERRUPT_EN_SHIFT)) & RSIM_DSM_CONTROL_GEN_SYSCLK_INTERRUPT_EN_MASK)
#define RSIM_DSM_CONTROL_GEN_SYSCLK_REQ_INT_MASK (0x8000U)
#define RSIM_DSM_CONTROL_GEN_SYSCLK_REQ_INT_SHIFT (15U)
#define RSIM_DSM_CONTROL_GEN_SYSCLK_REQ_INT(x)   (((uint32_t)(((uint32_t)(x)) << RSIM_DSM_CONTROL_GEN_SYSCLK_REQ_INT_SHIFT)) & RSIM_DSM_CONTROL_GEN_SYSCLK_REQ_INT_MASK)
#define RSIM_DSM_CONTROL_DSM_ZIG_READY_MASK      (0x10000U)
#define RSIM_DSM_CONTROL_DSM_ZIG_READY_SHIFT     (16U)
#define RSIM_DSM_CONTROL_DSM_ZIG_READY(x)        (((uint32_t)(((uint32_t)(x)) << RSIM_DSM_CONTROL_DSM_ZIG_READY_SHIFT)) & RSIM_DSM_CONTROL_DSM_ZIG_READY_MASK)
#define RSIM_DSM_CONTROL_ZIG_DEEP_SLEEP_STATUS_MASK (0x20000U)
#define RSIM_DSM_CONTROL_ZIG_DEEP_SLEEP_STATUS_SHIFT (17U)
#define RSIM_DSM_CONTROL_ZIG_DEEP_SLEEP_STATUS(x) (((uint32_t)(((uint32_t)(x)) << RSIM_DSM_CONTROL_ZIG_DEEP_SLEEP_STATUS_SHIFT)) & RSIM_DSM_CONTROL_ZIG_DEEP_SLEEP_STATUS_MASK)
#define RSIM_DSM_CONTROL_DSM_ZIG_FINISHED_MASK   (0x40000U)
#define RSIM_DSM_CONTROL_DSM_ZIG_FINISHED_SHIFT  (18U)
#define RSIM_DSM_CONTROL_DSM_ZIG_FINISHED(x)     (((uint32_t)(((uint32_t)(x)) << RSIM_DSM_CONTROL_DSM_ZIG_FINISHED_SHIFT)) & RSIM_DSM_CONTROL_DSM_ZIG_FINISHED_MASK)
#define RSIM_DSM_CONTROL_ZIG_SYSCLK_REQUEST_EN_MASK (0x80000U)
#define RSIM_DSM_CONTROL_ZIG_SYSCLK_REQUEST_EN_SHIFT (19U)
#define RSIM_DSM_CONTROL_ZIG_SYSCLK_REQUEST_EN(x) (((uint32_t)(((uint32_t)(x)) << RSIM_DSM_CONTROL_ZIG_SYSCLK_REQUEST_EN_SHIFT)) & RSIM_DSM_CONTROL_ZIG_SYSCLK_REQUEST_EN_MASK)
#define RSIM_DSM_CONTROL_ZIG_SLEEP_REQUEST_MASK  (0x100000U)
#define RSIM_DSM_CONTROL_ZIG_SLEEP_REQUEST_SHIFT (20U)
#define RSIM_DSM_CONTROL_ZIG_SLEEP_REQUEST(x)    (((uint32_t)(((uint32_t)(x)) << RSIM_DSM_CONTROL_ZIG_SLEEP_REQUEST_SHIFT)) & RSIM_DSM_CONTROL_ZIG_SLEEP_REQUEST_MASK)
#define RSIM_DSM_CONTROL_ZIG_SYSCLK_REQ_MASK     (0x200000U)
#define RSIM_DSM_CONTROL_ZIG_SYSCLK_REQ_SHIFT    (21U)
#define RSIM_DSM_CONTROL_ZIG_SYSCLK_REQ(x)       (((uint32_t)(((uint32_t)(x)) << RSIM_DSM_CONTROL_ZIG_SYSCLK_REQ_SHIFT)) & RSIM_DSM_CONTROL_ZIG_SYSCLK_REQ_MASK)
#define RSIM_DSM_CONTROL_ZIG_SYSCLK_INTERRUPT_EN_MASK (0x400000U)
#define RSIM_DSM_CONTROL_ZIG_SYSCLK_INTERRUPT_EN_SHIFT (22U)
#define RSIM_DSM_CONTROL_ZIG_SYSCLK_INTERRUPT_EN(x) (((uint32_t)(((uint32_t)(x)) << RSIM_DSM_CONTROL_ZIG_SYSCLK_INTERRUPT_EN_SHIFT)) & RSIM_DSM_CONTROL_ZIG_SYSCLK_INTERRUPT_EN_MASK)
#define RSIM_DSM_CONTROL_ZIG_SYSCLK_REQ_INT_MASK (0x800000U)
#define RSIM_DSM_CONTROL_ZIG_SYSCLK_REQ_INT_SHIFT (23U)
#define RSIM_DSM_CONTROL_ZIG_SYSCLK_REQ_INT(x)   (((uint32_t)(((uint32_t)(x)) << RSIM_DSM_CONTROL_ZIG_SYSCLK_REQ_INT_SHIFT)) & RSIM_DSM_CONTROL_ZIG_SYSCLK_REQ_INT_MASK)
#define RSIM_DSM_CONTROL_DSM_TIMER_CLR_MASK      (0x8000000U)
#define RSIM_DSM_CONTROL_DSM_TIMER_CLR_SHIFT     (27U)
#define RSIM_DSM_CONTROL_DSM_TIMER_CLR(x)        (((uint32_t)(((uint32_t)(x)) << RSIM_DSM_CONTROL_DSM_TIMER_CLR_SHIFT)) & RSIM_DSM_CONTROL_DSM_TIMER_CLR_MASK)
#define RSIM_DSM_CONTROL_DSM_TIMER_EN_MASK       (0x80000000U)
#define RSIM_DSM_CONTROL_DSM_TIMER_EN_SHIFT      (31U)
#define RSIM_DSM_CONTROL_DSM_TIMER_EN(x)         (((uint32_t)(((uint32_t)(x)) << RSIM_DSM_CONTROL_DSM_TIMER_EN_SHIFT)) & RSIM_DSM_CONTROL_DSM_TIMER_EN_MASK)

/*! @name DSM_OSC_OFFSET - Deep Sleep Wakeup Time Offset */
#define RSIM_DSM_OSC_OFFSET_DSM_OSC_STABILIZE_TIME_MASK (0x3FFU)
#define RSIM_DSM_OSC_OFFSET_DSM_OSC_STABILIZE_TIME_SHIFT (0U)
#define RSIM_DSM_OSC_OFFSET_DSM_OSC_STABILIZE_TIME(x) (((uint32_t)(((uint32_t)(x)) << RSIM_DSM_OSC_OFFSET_DSM_OSC_STABILIZE_TIME_SHIFT)) & RSIM_DSM_OSC_OFFSET_DSM_OSC_STABILIZE_TIME_MASK)

/*! @name ANT_SLEEP - ANT Link Layer Sleep Time */
#define RSIM_ANT_SLEEP_ANT_SLEEP_TIME_MASK       (0xFFFFFFU)
#define RSIM_ANT_SLEEP_ANT_SLEEP_TIME_SHIFT      (0U)
#define RSIM_ANT_SLEEP_ANT_SLEEP_TIME(x)         (((uint32_t)(((uint32_t)(x)) << RSIM_ANT_SLEEP_ANT_SLEEP_TIME_SHIFT)) & RSIM_ANT_SLEEP_ANT_SLEEP_TIME_MASK)

/*! @name ANT_WAKE - ANT Link Layer Wake Time */
#define RSIM_ANT_WAKE_ANT_WAKE_TIME_MASK         (0xFFFFFFU)
#define RSIM_ANT_WAKE_ANT_WAKE_TIME_SHIFT        (0U)
#define RSIM_ANT_WAKE_ANT_WAKE_TIME(x)           (((uint32_t)(((uint32_t)(x)) << RSIM_ANT_WAKE_ANT_WAKE_TIME_SHIFT)) & RSIM_ANT_WAKE_ANT_WAKE_TIME_MASK)

/*! @name ZIG_SLEEP - 802.15.4 Link Layer Sleep Time */
#define RSIM_ZIG_SLEEP_ZIG_SLEEP_TIME_MASK       (0xFFFFFFU)
#define RSIM_ZIG_SLEEP_ZIG_SLEEP_TIME_SHIFT      (0U)
#define RSIM_ZIG_SLEEP_ZIG_SLEEP_TIME(x)         (((uint32_t)(((uint32_t)(x)) << RSIM_ZIG_SLEEP_ZIG_SLEEP_TIME_SHIFT)) & RSIM_ZIG_SLEEP_ZIG_SLEEP_TIME_MASK)

/*! @name ZIG_WAKE - 802.15.4 Link Layer Wake Time */
#define RSIM_ZIG_WAKE_ZIG_WAKE_TIME_MASK         (0xFFFFFFU)
#define RSIM_ZIG_WAKE_ZIG_WAKE_TIME_SHIFT        (0U)
#define RSIM_ZIG_WAKE_ZIG_WAKE_TIME(x)           (((uint32_t)(((uint32_t)(x)) << RSIM_ZIG_WAKE_ZIG_WAKE_TIME_SHIFT)) & RSIM_ZIG_WAKE_ZIG_WAKE_TIME_MASK)

/*! @name GEN_SLEEP - Generic FSK Link Layer Sleep Time */
#define RSIM_GEN_SLEEP_GEN_SLEEP_TIME_MASK       (0xFFFFFFU)
#define RSIM_GEN_SLEEP_GEN_SLEEP_TIME_SHIFT      (0U)
#define RSIM_GEN_SLEEP_GEN_SLEEP_TIME(x)         (((uint32_t)(((uint32_t)(x)) << RSIM_GEN_SLEEP_GEN_SLEEP_TIME_SHIFT)) & RSIM_GEN_SLEEP_GEN_SLEEP_TIME_MASK)

/*! @name GEN_WAKE - Generic FSK Link Layer Wake Time */
#define RSIM_GEN_WAKE_GEN_WAKE_TIME_MASK         (0xFFFFFFU)
#define RSIM_GEN_WAKE_GEN_WAKE_TIME_SHIFT        (0U)
#define RSIM_GEN_WAKE_GEN_WAKE_TIME(x)           (((uint32_t)(((uint32_t)(x)) << RSIM_GEN_WAKE_GEN_WAKE_TIME_SHIFT)) & RSIM_GEN_WAKE_GEN_WAKE_TIME_MASK)

/*! @name RF_OSC_CTRL - Radio Oscillator Control */
#define RSIM_RF_OSC_CTRL_BB_XTAL_ALC_COUNT_SEL_MASK (0x3U)
#define RSIM_RF_OSC_CTRL_BB_XTAL_ALC_COUNT_SEL_SHIFT (0U)
#define RSIM_RF_OSC_CTRL_BB_XTAL_ALC_COUNT_SEL(x) (((uint32_t)(((uint32_t)(x)) << RSIM_RF_OSC_CTRL_BB_XTAL_ALC_COUNT_SEL_SHIFT)) & RSIM_RF_OSC_CTRL_BB_XTAL_ALC_COUNT_SEL_MASK)
#define RSIM_RF_OSC_CTRL_BB_XTAL_ALC_ON_MASK     (0x4U)
#define RSIM_RF_OSC_CTRL_BB_XTAL_ALC_ON_SHIFT    (2U)
#define RSIM_RF_OSC_CTRL_BB_XTAL_ALC_ON(x)       (((uint32_t)(((uint32_t)(x)) << RSIM_RF_OSC_CTRL_BB_XTAL_ALC_ON_SHIFT)) & RSIM_RF_OSC_CTRL_BB_XTAL_ALC_ON_MASK)
#define RSIM_RF_OSC_CTRL_RF_OSC_BYPASS_EN_MASK   (0x8U)
#define RSIM_RF_OSC_CTRL_RF_OSC_BYPASS_EN_SHIFT  (3U)
#define RSIM_RF_OSC_CTRL_RF_OSC_BYPASS_EN(x)     (((uint32_t)(((uint32_t)(x)) << RSIM_RF_OSC_CTRL_RF_OSC_BYPASS_EN_SHIFT)) & RSIM_RF_OSC_CTRL_RF_OSC_BYPASS_EN_MASK)
#define RSIM_RF_OSC_CTRL_BB_XTAL_COMP_BIAS_MASK  (0x1F0U)
#define RSIM_RF_OSC_CTRL_BB_XTAL_COMP_BIAS_SHIFT (4U)
#define RSIM_RF_OSC_CTRL_BB_XTAL_COMP_BIAS(x)    (((uint32_t)(((uint32_t)(x)) << RSIM_RF_OSC_CTRL_BB_XTAL_COMP_BIAS_SHIFT)) & RSIM_RF_OSC_CTRL_BB_XTAL_COMP_BIAS_MASK)
#define RSIM_RF_OSC_CTRL_BB_XTAL_DC_COUP_MODE_EN_MASK (0x200U)
#define RSIM_RF_OSC_CTRL_BB_XTAL_DC_COUP_MODE_EN_SHIFT (9U)
#define RSIM_RF_OSC_CTRL_BB_XTAL_DC_COUP_MODE_EN(x) (((uint32_t)(((uint32_t)(x)) << RSIM_RF_OSC_CTRL_BB_XTAL_DC_COUP_MODE_EN_SHIFT)) & RSIM_RF_OSC_CTRL_BB_XTAL_DC_COUP_MODE_EN_MASK)
#define RSIM_RF_OSC_CTRL_BB_XTAL_DIAGSEL_MASK    (0x400U)
#define RSIM_RF_OSC_CTRL_BB_XTAL_DIAGSEL_SHIFT   (10U)
#define RSIM_RF_OSC_CTRL_BB_XTAL_DIAGSEL(x)      (((uint32_t)(((uint32_t)(x)) << RSIM_RF_OSC_CTRL_BB_XTAL_DIAGSEL_SHIFT)) & RSIM_RF_OSC_CTRL_BB_XTAL_DIAGSEL_MASK)
#define RSIM_RF_OSC_CTRL_BB_XTAL_DIG_CLK_ON_MASK (0x800U)
#define RSIM_RF_OSC_CTRL_BB_XTAL_DIG_CLK_ON_SHIFT (11U)
#define RSIM_RF_OSC_CTRL_BB_XTAL_DIG_CLK_ON(x)   (((uint32_t)(((uint32_t)(x)) << RSIM_RF_OSC_CTRL_BB_XTAL_DIG_CLK_ON_SHIFT)) & RSIM_RF_OSC_CTRL_BB_XTAL_DIG_CLK_ON_MASK)
#define RSIM_RF_OSC_CTRL_BB_XTAL_GM_MASK         (0x1F000U)
#define RSIM_RF_OSC_CTRL_BB_XTAL_GM_SHIFT        (12U)
#define RSIM_RF_OSC_CTRL_BB_XTAL_GM(x)           (((uint32_t)(((uint32_t)(x)) << RSIM_RF_OSC_CTRL_BB_XTAL_GM_SHIFT)) & RSIM_RF_OSC_CTRL_BB_XTAL_GM_MASK)
#define RSIM_RF_OSC_CTRL_BB_XTAL_ON_OVRD_MASK    (0x20000U)
#define RSIM_RF_OSC_CTRL_BB_XTAL_ON_OVRD_SHIFT   (17U)
#define RSIM_RF_OSC_CTRL_BB_XTAL_ON_OVRD(x)      (((uint32_t)(((uint32_t)(x)) << RSIM_RF_OSC_CTRL_BB_XTAL_ON_OVRD_SHIFT)) & RSIM_RF_OSC_CTRL_BB_XTAL_ON_OVRD_MASK)
#define RSIM_RF_OSC_CTRL_BB_XTAL_ON_OVRD_ON_MASK (0x40000U)
#define RSIM_RF_OSC_CTRL_BB_XTAL_ON_OVRD_ON_SHIFT (18U)
#define RSIM_RF_OSC_CTRL_BB_XTAL_ON_OVRD_ON(x)   (((uint32_t)(((uint32_t)(x)) << RSIM_RF_OSC_CTRL_BB_XTAL_ON_OVRD_ON_SHIFT)) & RSIM_RF_OSC_CTRL_BB_XTAL_ON_OVRD_ON_MASK)
#define RSIM_RF_OSC_CTRL_BB_XTAL_READY_COUNT_SEL_MASK (0x300000U)
#define RSIM_RF_OSC_CTRL_BB_XTAL_READY_COUNT_SEL_SHIFT (20U)
#define RSIM_RF_OSC_CTRL_BB_XTAL_READY_COUNT_SEL(x) (((uint32_t)(((uint32_t)(x)) << RSIM_RF_OSC_CTRL_BB_XTAL_READY_COUNT_SEL_SHIFT)) & RSIM_RF_OSC_CTRL_BB_XTAL_READY_COUNT_SEL_MASK)
#define RSIM_RF_OSC_CTRL_RADIO_EXT_OSC_RF_EN_SEL_MASK (0x8000000U)
#define RSIM_RF_OSC_CTRL_RADIO_EXT_OSC_RF_EN_SEL_SHIFT (27U)
#define RSIM_RF_OSC_CTRL_RADIO_EXT_OSC_RF_EN_SEL(x) (((uint32_t)(((uint32_t)(x)) << RSIM_RF_OSC_CTRL_RADIO_EXT_OSC_RF_EN_SEL_SHIFT)) & RSIM_RF_OSC_CTRL_RADIO_EXT_OSC_RF_EN_SEL_MASK)
#define RSIM_RF_OSC_CTRL_RADIO_EXT_OSC_OVRD_MASK (0x10000000U)
#define RSIM_RF_OSC_CTRL_RADIO_EXT_OSC_OVRD_SHIFT (28U)
#define RSIM_RF_OSC_CTRL_RADIO_EXT_OSC_OVRD(x)   (((uint32_t)(((uint32_t)(x)) << RSIM_RF_OSC_CTRL_RADIO_EXT_OSC_OVRD_SHIFT)) & RSIM_RF_OSC_CTRL_RADIO_EXT_OSC_OVRD_MASK)
#define RSIM_RF_OSC_CTRL_RADIO_EXT_OSC_OVRD_EN_MASK (0x20000000U)
#define RSIM_RF_OSC_CTRL_RADIO_EXT_OSC_OVRD_EN_SHIFT (29U)
#define RSIM_RF_OSC_CTRL_RADIO_EXT_OSC_OVRD_EN(x) (((uint32_t)(((uint32_t)(x)) << RSIM_RF_OSC_CTRL_RADIO_EXT_OSC_OVRD_EN_SHIFT)) & RSIM_RF_OSC_CTRL_RADIO_EXT_OSC_OVRD_EN_MASK)
#define RSIM_RF_OSC_CTRL_RADIO_RF_ABORT_OVRD_MASK (0x40000000U)
#define RSIM_RF_OSC_CTRL_RADIO_RF_ABORT_OVRD_SHIFT (30U)
#define RSIM_RF_OSC_CTRL_RADIO_RF_ABORT_OVRD(x)  (((uint32_t)(((uint32_t)(x)) << RSIM_RF_OSC_CTRL_RADIO_RF_ABORT_OVRD_SHIFT)) & RSIM_RF_OSC_CTRL_RADIO_RF_ABORT_OVRD_MASK)
#define RSIM_RF_OSC_CTRL_RADIO_RF_ABORT_OVRD_EN_MASK (0x80000000U)
#define RSIM_RF_OSC_CTRL_RADIO_RF_ABORT_OVRD_EN_SHIFT (31U)
#define RSIM_RF_OSC_CTRL_RADIO_RF_ABORT_OVRD_EN(x) (((uint32_t)(((uint32_t)(x)) << RSIM_RF_OSC_CTRL_RADIO_RF_ABORT_OVRD_EN_SHIFT)) & RSIM_RF_OSC_CTRL_RADIO_RF_ABORT_OVRD_EN_MASK)

/*! @name ANA_TEST - Radio Analog Test Registers */
#define RSIM_ANA_TEST_BB_LDO_LS_BYP_MASK         (0x1U)
#define RSIM_ANA_TEST_BB_LDO_LS_BYP_SHIFT        (0U)
#define RSIM_ANA_TEST_BB_LDO_LS_BYP(x)           (((uint32_t)(((uint32_t)(x)) << RSIM_ANA_TEST_BB_LDO_LS_BYP_SHIFT)) & RSIM_ANA_TEST_BB_LDO_LS_BYP_MASK)
#define RSIM_ANA_TEST_BB_LDO_LS_DIAGSEL_MASK     (0x2U)
#define RSIM_ANA_TEST_BB_LDO_LS_DIAGSEL_SHIFT    (1U)
#define RSIM_ANA_TEST_BB_LDO_LS_DIAGSEL(x)       (((uint32_t)(((uint32_t)(x)) << RSIM_ANA_TEST_BB_LDO_LS_DIAGSEL_SHIFT)) & RSIM_ANA_TEST_BB_LDO_LS_DIAGSEL_MASK)
#define RSIM_ANA_TEST_BB_LDO_XO_BYP_ON_MASK      (0x4U)
#define RSIM_ANA_TEST_BB_LDO_XO_BYP_ON_SHIFT     (2U)
#define RSIM_ANA_TEST_BB_LDO_XO_BYP_ON(x)        (((uint32_t)(((uint32_t)(x)) << RSIM_ANA_TEST_BB_LDO_XO_BYP_ON_SHIFT)) & RSIM_ANA_TEST_BB_LDO_XO_BYP_ON_MASK)
#define RSIM_ANA_TEST_BB_LDO_XO_DIAGSEL_MASK     (0x8U)
#define RSIM_ANA_TEST_BB_LDO_XO_DIAGSEL_SHIFT    (3U)
#define RSIM_ANA_TEST_BB_LDO_XO_DIAGSEL(x)       (((uint32_t)(((uint32_t)(x)) << RSIM_ANA_TEST_BB_LDO_XO_DIAGSEL_SHIFT)) & RSIM_ANA_TEST_BB_LDO_XO_DIAGSEL_MASK)
#define RSIM_ANA_TEST_BB_XTAL_TEST_MASK          (0x10U)
#define RSIM_ANA_TEST_BB_XTAL_TEST_SHIFT         (4U)
#define RSIM_ANA_TEST_BB_XTAL_TEST(x)            (((uint32_t)(((uint32_t)(x)) << RSIM_ANA_TEST_BB_XTAL_TEST_SHIFT)) & RSIM_ANA_TEST_BB_XTAL_TEST_MASK)
#define RSIM_ANA_TEST_BG_DIAGBUF_MASK            (0x20U)
#define RSIM_ANA_TEST_BG_DIAGBUF_SHIFT           (5U)
#define RSIM_ANA_TEST_BG_DIAGBUF(x)              (((uint32_t)(((uint32_t)(x)) << RSIM_ANA_TEST_BG_DIAGBUF_SHIFT)) & RSIM_ANA_TEST_BG_DIAGBUF_MASK)
#define RSIM_ANA_TEST_BG_DIAGSEL_MASK            (0x40U)
#define RSIM_ANA_TEST_BG_DIAGSEL_SHIFT           (6U)
#define RSIM_ANA_TEST_BG_DIAGSEL(x)              (((uint32_t)(((uint32_t)(x)) << RSIM_ANA_TEST_BG_DIAGSEL_SHIFT)) & RSIM_ANA_TEST_BG_DIAGSEL_MASK)
#define RSIM_ANA_TEST_BG_STARTUPFORCE_MASK       (0x80U)
#define RSIM_ANA_TEST_BG_STARTUPFORCE_SHIFT      (7U)
#define RSIM_ANA_TEST_BG_STARTUPFORCE(x)         (((uint32_t)(((uint32_t)(x)) << RSIM_ANA_TEST_BG_STARTUPFORCE_SHIFT)) & RSIM_ANA_TEST_BG_STARTUPFORCE_MASK)
#define RSIM_ANA_TEST_DIAG_1234_ON_MASK          (0x100U)
#define RSIM_ANA_TEST_DIAG_1234_ON_SHIFT         (8U)
#define RSIM_ANA_TEST_DIAG_1234_ON(x)            (((uint32_t)(((uint32_t)(x)) << RSIM_ANA_TEST_DIAG_1234_ON_SHIFT)) & RSIM_ANA_TEST_DIAG_1234_ON_MASK)
#define RSIM_ANA_TEST_DIAG2SOCADC_DEC_MASK       (0x600U)
#define RSIM_ANA_TEST_DIAG2SOCADC_DEC_SHIFT      (9U)
#define RSIM_ANA_TEST_DIAG2SOCADC_DEC(x)         (((uint32_t)(((uint32_t)(x)) << RSIM_ANA_TEST_DIAG2SOCADC_DEC_SHIFT)) & RSIM_ANA_TEST_DIAG2SOCADC_DEC_MASK)
#define RSIM_ANA_TEST_DIAG2SOCADC_DEC_ON_MASK    (0x800U)
#define RSIM_ANA_TEST_DIAG2SOCADC_DEC_ON_SHIFT   (11U)
#define RSIM_ANA_TEST_DIAG2SOCADC_DEC_ON(x)      (((uint32_t)(((uint32_t)(x)) << RSIM_ANA_TEST_DIAG2SOCADC_DEC_ON_SHIFT)) & RSIM_ANA_TEST_DIAG2SOCADC_DEC_ON_MASK)
#define RSIM_ANA_TEST_DIAGCODE_MASK              (0x7000U)
#define RSIM_ANA_TEST_DIAGCODE_SHIFT             (12U)
#define RSIM_ANA_TEST_DIAGCODE(x)                (((uint32_t)(((uint32_t)(x)) << RSIM_ANA_TEST_DIAGCODE_SHIFT)) & RSIM_ANA_TEST_DIAGCODE_MASK)

/*! @name ANA_TRIM - Radio Analog Trim Registers */
#define RSIM_ANA_TRIM_BB_LDO_LS_SPARE_MASK       (0x3U)
#define RSIM_ANA_TRIM_BB_LDO_LS_SPARE_SHIFT      (0U)
#define RSIM_ANA_TRIM_BB_LDO_LS_SPARE(x)         (((uint32_t)(((uint32_t)(x)) << RSIM_ANA_TRIM_BB_LDO_LS_SPARE_SHIFT)) & RSIM_ANA_TRIM_BB_LDO_LS_SPARE_MASK)
#define RSIM_ANA_TRIM_BB_LDO_LS_TRIM_MASK        (0x38U)
#define RSIM_ANA_TRIM_BB_LDO_LS_TRIM_SHIFT       (3U)
#define RSIM_ANA_TRIM_BB_LDO_LS_TRIM(x)          (((uint32_t)(((uint32_t)(x)) << RSIM_ANA_TRIM_BB_LDO_LS_TRIM_SHIFT)) & RSIM_ANA_TRIM_BB_LDO_LS_TRIM_MASK)
#define RSIM_ANA_TRIM_BB_LDO_XO_SPARE_MASK       (0xC0U)
#define RSIM_ANA_TRIM_BB_LDO_XO_SPARE_SHIFT      (6U)
#define RSIM_ANA_TRIM_BB_LDO_XO_SPARE(x)         (((uint32_t)(((uint32_t)(x)) << RSIM_ANA_TRIM_BB_LDO_XO_SPARE_SHIFT)) & RSIM_ANA_TRIM_BB_LDO_XO_SPARE_MASK)
#define RSIM_ANA_TRIM_BB_LDO_XO_TRIM_MASK        (0x700U)
#define RSIM_ANA_TRIM_BB_LDO_XO_TRIM_SHIFT       (8U)
#define RSIM_ANA_TRIM_BB_LDO_XO_TRIM(x)          (((uint32_t)(((uint32_t)(x)) << RSIM_ANA_TRIM_BB_LDO_XO_TRIM_SHIFT)) & RSIM_ANA_TRIM_BB_LDO_XO_TRIM_MASK)
#define RSIM_ANA_TRIM_BB_XTAL_SPARE_MASK         (0xF800U)
#define RSIM_ANA_TRIM_BB_XTAL_SPARE_SHIFT        (11U)
#define RSIM_ANA_TRIM_BB_XTAL_SPARE(x)           (((uint32_t)(((uint32_t)(x)) << RSIM_ANA_TRIM_BB_XTAL_SPARE_SHIFT)) & RSIM_ANA_TRIM_BB_XTAL_SPARE_MASK)
#define RSIM_ANA_TRIM_BB_XTAL_TRIM_MASK          (0xFF0000U)
#define RSIM_ANA_TRIM_BB_XTAL_TRIM_SHIFT         (16U)
#define RSIM_ANA_TRIM_BB_XTAL_TRIM(x)            (((uint32_t)(((uint32_t)(x)) << RSIM_ANA_TRIM_BB_XTAL_TRIM_SHIFT)) & RSIM_ANA_TRIM_BB_XTAL_TRIM_MASK)
#define RSIM_ANA_TRIM_BG_1V_TRIM_MASK            (0xF000000U)
#define RSIM_ANA_TRIM_BG_1V_TRIM_SHIFT           (24U)
#define RSIM_ANA_TRIM_BG_1V_TRIM(x)              (((uint32_t)(((uint32_t)(x)) << RSIM_ANA_TRIM_BG_1V_TRIM_SHIFT)) & RSIM_ANA_TRIM_BG_1V_TRIM_MASK)
#define RSIM_ANA_TRIM_BG_IBIAS_5U_TRIM_MASK      (0xF0000000U)
#define RSIM_ANA_TRIM_BG_IBIAS_5U_TRIM_SHIFT     (28U)
#define RSIM_ANA_TRIM_BG_IBIAS_5U_TRIM(x)        (((uint32_t)(((uint32_t)(x)) << RSIM_ANA_TRIM_BG_IBIAS_5U_TRIM_SHIFT)) & RSIM_ANA_TRIM_BG_IBIAS_5U_TRIM_MASK)


/*!
 * @}
 */ /* end of group RSIM_Register_Masks */


/* RSIM - Peripheral instance base addresses */
/** Peripheral RSIM base address */
#define RSIM_BASE                                (0x40059000u)
/** Peripheral RSIM base pointer */
#define RSIM                                     ((RSIM_Type *)RSIM_BASE)
/** Array initializer of RSIM peripheral base addresses */
#define RSIM_BASE_ADDRS                          { RSIM_BASE }
/** Array initializer of RSIM peripheral base pointers */
#define RSIM_BASE_PTRS                           { RSIM }

/*!
 * @}
 */ /* end of group RSIM_Peripheral_Access_Layer */


/* ----------------------------------------------------------------------------
   -- RTC Peripheral Access Layer
   ---------------------------------------------------------------------------- */

/*!
 * @addtogroup RTC_Peripheral_Access_Layer RTC Peripheral Access Layer
 * @{
 */

/** RTC - Register Layout Typedef */
typedef struct {
  __IO uint32_t TSR;                               /**< RTC Time Seconds Register, offset: 0x0 */
  __IO uint32_t TPR;                               /**< RTC Time Prescaler Register, offset: 0x4 */
  __IO uint32_t TAR;                               /**< RTC Time Alarm Register, offset: 0x8 */
  __IO uint32_t TCR;                               /**< RTC Time Compensation Register, offset: 0xC */
  __IO uint32_t CR;                                /**< RTC Control Register, offset: 0x10 */
  __IO uint32_t SR;                                /**< RTC Status Register, offset: 0x14 */
  __IO uint32_t LR;                                /**< RTC Lock Register, offset: 0x18 */
  __IO uint32_t IER;                               /**< RTC Interrupt Enable Register, offset: 0x1C */
} RTC_Type;

/* ----------------------------------------------------------------------------
   -- RTC Register Masks
   ---------------------------------------------------------------------------- */

/*!
 * @addtogroup RTC_Register_Masks RTC Register Masks
 * @{
 */

/*! @name TSR - RTC Time Seconds Register */
#define RTC_TSR_TSR_MASK                         (0xFFFFFFFFU)
#define RTC_TSR_TSR_SHIFT                        (0U)
#define RTC_TSR_TSR(x)                           (((uint32_t)(((uint32_t)(x)) << RTC_TSR_TSR_SHIFT)) & RTC_TSR_TSR_MASK)

/*! @name TPR - RTC Time Prescaler Register */
#define RTC_TPR_TPR_MASK                         (0xFFFFU)
#define RTC_TPR_TPR_SHIFT                        (0U)
#define RTC_TPR_TPR(x)                           (((uint32_t)(((uint32_t)(x)) << RTC_TPR_TPR_SHIFT)) & RTC_TPR_TPR_MASK)

/*! @name TAR - RTC Time Alarm Register */
#define RTC_TAR_TAR_MASK                         (0xFFFFFFFFU)
#define RTC_TAR_TAR_SHIFT                        (0U)
#define RTC_TAR_TAR(x)                           (((uint32_t)(((uint32_t)(x)) << RTC_TAR_TAR_SHIFT)) & RTC_TAR_TAR_MASK)

/*! @name TCR - RTC Time Compensation Register */
#define RTC_TCR_TCR_MASK                         (0xFFU)
#define RTC_TCR_TCR_SHIFT                        (0U)
#define RTC_TCR_TCR(x)                           (((uint32_t)(((uint32_t)(x)) << RTC_TCR_TCR_SHIFT)) & RTC_TCR_TCR_MASK)
#define RTC_TCR_CIR_MASK                         (0xFF00U)
#define RTC_TCR_CIR_SHIFT                        (8U)
#define RTC_TCR_CIR(x)                           (((uint32_t)(((uint32_t)(x)) << RTC_TCR_CIR_SHIFT)) & RTC_TCR_CIR_MASK)
#define RTC_TCR_TCV_MASK                         (0xFF0000U)
#define RTC_TCR_TCV_SHIFT                        (16U)
#define RTC_TCR_TCV(x)                           (((uint32_t)(((uint32_t)(x)) << RTC_TCR_TCV_SHIFT)) & RTC_TCR_TCV_MASK)
#define RTC_TCR_CIC_MASK                         (0xFF000000U)
#define RTC_TCR_CIC_SHIFT                        (24U)
#define RTC_TCR_CIC(x)                           (((uint32_t)(((uint32_t)(x)) << RTC_TCR_CIC_SHIFT)) & RTC_TCR_CIC_MASK)

/*! @name CR - RTC Control Register */
#define RTC_CR_SWR_MASK                          (0x1U)
#define RTC_CR_SWR_SHIFT                         (0U)
#define RTC_CR_SWR(x)                            (((uint32_t)(((uint32_t)(x)) << RTC_CR_SWR_SHIFT)) & RTC_CR_SWR_MASK)
#define RTC_CR_WPE_MASK                          (0x2U)
#define RTC_CR_WPE_SHIFT                         (1U)
#define RTC_CR_WPE(x)                            (((uint32_t)(((uint32_t)(x)) << RTC_CR_WPE_SHIFT)) & RTC_CR_WPE_MASK)
#define RTC_CR_SUP_MASK                          (0x4U)
#define RTC_CR_SUP_SHIFT                         (2U)
#define RTC_CR_SUP(x)                            (((uint32_t)(((uint32_t)(x)) << RTC_CR_SUP_SHIFT)) & RTC_CR_SUP_MASK)
#define RTC_CR_UM_MASK                           (0x8U)
#define RTC_CR_UM_SHIFT                          (3U)
#define RTC_CR_UM(x)                             (((uint32_t)(((uint32_t)(x)) << RTC_CR_UM_SHIFT)) & RTC_CR_UM_MASK)
#define RTC_CR_WPS_MASK                          (0x10U)
#define RTC_CR_WPS_SHIFT                         (4U)
#define RTC_CR_WPS(x)                            (((uint32_t)(((uint32_t)(x)) << RTC_CR_WPS_SHIFT)) & RTC_CR_WPS_MASK)
#define RTC_CR_OSCE_MASK                         (0x100U)
#define RTC_CR_OSCE_SHIFT                        (8U)
#define RTC_CR_OSCE(x)                           (((uint32_t)(((uint32_t)(x)) << RTC_CR_OSCE_SHIFT)) & RTC_CR_OSCE_MASK)
#define RTC_CR_CLKO_MASK                         (0x200U)
#define RTC_CR_CLKO_SHIFT                        (9U)
#define RTC_CR_CLKO(x)                           (((uint32_t)(((uint32_t)(x)) << RTC_CR_CLKO_SHIFT)) & RTC_CR_CLKO_MASK)
#define RTC_CR_SC16P_MASK                        (0x400U)
#define RTC_CR_SC16P_SHIFT                       (10U)
#define RTC_CR_SC16P(x)                          (((uint32_t)(((uint32_t)(x)) << RTC_CR_SC16P_SHIFT)) & RTC_CR_SC16P_MASK)
#define RTC_CR_SC8P_MASK                         (0x800U)
#define RTC_CR_SC8P_SHIFT                        (11U)
#define RTC_CR_SC8P(x)                           (((uint32_t)(((uint32_t)(x)) << RTC_CR_SC8P_SHIFT)) & RTC_CR_SC8P_MASK)
#define RTC_CR_SC4P_MASK                         (0x1000U)
#define RTC_CR_SC4P_SHIFT                        (12U)
#define RTC_CR_SC4P(x)                           (((uint32_t)(((uint32_t)(x)) << RTC_CR_SC4P_SHIFT)) & RTC_CR_SC4P_MASK)
#define RTC_CR_SC2P_MASK                         (0x2000U)
#define RTC_CR_SC2P_SHIFT                        (13U)
#define RTC_CR_SC2P(x)                           (((uint32_t)(((uint32_t)(x)) << RTC_CR_SC2P_SHIFT)) & RTC_CR_SC2P_MASK)

/*! @name SR - RTC Status Register */
#define RTC_SR_TIF_MASK                          (0x1U)
#define RTC_SR_TIF_SHIFT                         (0U)
#define RTC_SR_TIF(x)                            (((uint32_t)(((uint32_t)(x)) << RTC_SR_TIF_SHIFT)) & RTC_SR_TIF_MASK)
#define RTC_SR_TOF_MASK                          (0x2U)
#define RTC_SR_TOF_SHIFT                         (1U)
#define RTC_SR_TOF(x)                            (((uint32_t)(((uint32_t)(x)) << RTC_SR_TOF_SHIFT)) & RTC_SR_TOF_MASK)
#define RTC_SR_TAF_MASK                          (0x4U)
#define RTC_SR_TAF_SHIFT                         (2U)
#define RTC_SR_TAF(x)                            (((uint32_t)(((uint32_t)(x)) << RTC_SR_TAF_SHIFT)) & RTC_SR_TAF_MASK)
#define RTC_SR_TCE_MASK                          (0x10U)
#define RTC_SR_TCE_SHIFT                         (4U)
#define RTC_SR_TCE(x)                            (((uint32_t)(((uint32_t)(x)) << RTC_SR_TCE_SHIFT)) & RTC_SR_TCE_MASK)

/*! @name LR - RTC Lock Register */
#define RTC_LR_TCL_MASK                          (0x8U)
#define RTC_LR_TCL_SHIFT                         (3U)
#define RTC_LR_TCL(x)                            (((uint32_t)(((uint32_t)(x)) << RTC_LR_TCL_SHIFT)) & RTC_LR_TCL_MASK)
#define RTC_LR_CRL_MASK                          (0x10U)
#define RTC_LR_CRL_SHIFT                         (4U)
#define RTC_LR_CRL(x)                            (((uint32_t)(((uint32_t)(x)) << RTC_LR_CRL_SHIFT)) & RTC_LR_CRL_MASK)
#define RTC_LR_SRL_MASK                          (0x20U)
#define RTC_LR_SRL_SHIFT                         (5U)
#define RTC_LR_SRL(x)                            (((uint32_t)(((uint32_t)(x)) << RTC_LR_SRL_SHIFT)) & RTC_LR_SRL_MASK)
#define RTC_LR_LRL_MASK                          (0x40U)
#define RTC_LR_LRL_SHIFT                         (6U)
#define RTC_LR_LRL(x)                            (((uint32_t)(((uint32_t)(x)) << RTC_LR_LRL_SHIFT)) & RTC_LR_LRL_MASK)

/*! @name IER - RTC Interrupt Enable Register */
#define RTC_IER_TIIE_MASK                        (0x1U)
#define RTC_IER_TIIE_SHIFT                       (0U)
#define RTC_IER_TIIE(x)                          (((uint32_t)(((uint32_t)(x)) << RTC_IER_TIIE_SHIFT)) & RTC_IER_TIIE_MASK)
#define RTC_IER_TOIE_MASK                        (0x2U)
#define RTC_IER_TOIE_SHIFT                       (1U)
#define RTC_IER_TOIE(x)                          (((uint32_t)(((uint32_t)(x)) << RTC_IER_TOIE_SHIFT)) & RTC_IER_TOIE_MASK)
#define RTC_IER_TAIE_MASK                        (0x4U)
#define RTC_IER_TAIE_SHIFT                       (2U)
#define RTC_IER_TAIE(x)                          (((uint32_t)(((uint32_t)(x)) << RTC_IER_TAIE_SHIFT)) & RTC_IER_TAIE_MASK)
#define RTC_IER_TSIE_MASK                        (0x10U)
#define RTC_IER_TSIE_SHIFT                       (4U)
#define RTC_IER_TSIE(x)                          (((uint32_t)(((uint32_t)(x)) << RTC_IER_TSIE_SHIFT)) & RTC_IER_TSIE_MASK)
#define RTC_IER_WPON_MASK                        (0x80U)
#define RTC_IER_WPON_SHIFT                       (7U)
#define RTC_IER_WPON(x)                          (((uint32_t)(((uint32_t)(x)) << RTC_IER_WPON_SHIFT)) & RTC_IER_WPON_MASK)


/*!
 * @}
 */ /* end of group RTC_Register_Masks */


/* RTC - Peripheral instance base addresses */
/** Peripheral RTC base address */
#define RTC_BASE                                 (0x4003D000u)
/** Peripheral RTC base pointer */
#define RTC                                      ((RTC_Type *)RTC_BASE)
/** Array initializer of RTC peripheral base addresses */
#define RTC_BASE_ADDRS                           { RTC_BASE }
/** Array initializer of RTC peripheral base pointers */
#define RTC_BASE_PTRS                            { RTC }
/** Interrupt vectors for the RTC peripheral type */
#define RTC_IRQS                                 { RTC_IRQn }
#define RTC_SECONDS_IRQS                         { RTC_Seconds_IRQn }

/*!
 * @}
 */ /* end of group RTC_Peripheral_Access_Layer */


/* ----------------------------------------------------------------------------
   -- SIM Peripheral Access Layer
   ---------------------------------------------------------------------------- */

/*!
 * @addtogroup SIM_Peripheral_Access_Layer SIM Peripheral Access Layer
 * @{
 */

/** SIM - Register Layout Typedef */
typedef struct {
  __IO uint32_t SOPT1;                             /**< System Options Register 1, offset: 0x0 */
       uint8_t RESERVED_0[4096];
  __IO uint32_t SOPT2;                             /**< System Options Register 2, offset: 0x1004 */
       uint8_t RESERVED_1[4];
  __IO uint32_t SOPT4;                             /**< System Options Register 4, offset: 0x100C */
  __IO uint32_t SOPT5;                             /**< System Options Register 5, offset: 0x1010 */
       uint8_t RESERVED_2[4];
  __IO uint32_t SOPT7;                             /**< System Options Register 7, offset: 0x1018 */
       uint8_t RESERVED_3[8];
  __I  uint32_t SDID;                              /**< System Device Identification Register, offset: 0x1024 */
       uint8_t RESERVED_4[12];
  __IO uint32_t SCGC4;                             /**< System Clock Gating Control Register 4, offset: 0x1034 */
  __IO uint32_t SCGC5;                             /**< System Clock Gating Control Register 5, offset: 0x1038 */
  __IO uint32_t SCGC6;                             /**< System Clock Gating Control Register 6, offset: 0x103C */
  __IO uint32_t SCGC7;                             /**< System Clock Gating Control Register 7, offset: 0x1040 */
  __IO uint32_t CLKDIV1;                           /**< System Clock Divider Register 1, offset: 0x1044 */
       uint8_t RESERVED_5[4];
  __IO uint32_t FCFG1;                             /**< Flash Configuration Register 1, offset: 0x104C */
  __I  uint32_t FCFG2;                             /**< Flash Configuration Register 2, offset: 0x1050 */
       uint8_t RESERVED_6[4];
  __I  uint32_t UIDMH;                             /**< Unique Identification Register Mid-High, offset: 0x1058 */
  __I  uint32_t UIDML;                             /**< Unique Identification Register Mid Low, offset: 0x105C */
  __I  uint32_t UIDL;                              /**< Unique Identification Register Low, offset: 0x1060 */
       uint8_t RESERVED_7[156];
  __IO uint32_t COPC;                              /**< COP Control Register, offset: 0x1100 */
  __O  uint32_t SRVCOP;                            /**< Service COP, offset: 0x1104 */
} SIM_Type;

/* ----------------------------------------------------------------------------
   -- SIM Register Masks
   ---------------------------------------------------------------------------- */

/*!
 * @addtogroup SIM_Register_Masks SIM Register Masks
 * @{
 */

/*! @name SOPT1 - System Options Register 1 */
#define SIM_SOPT1_OSC32KOUT_MASK                 (0x30000U)
#define SIM_SOPT1_OSC32KOUT_SHIFT                (16U)
#define SIM_SOPT1_OSC32KOUT(x)                   (((uint32_t)(((uint32_t)(x)) << SIM_SOPT1_OSC32KOUT_SHIFT)) & SIM_SOPT1_OSC32KOUT_MASK)
#define SIM_SOPT1_OSC32KSEL_MASK                 (0xC0000U)
#define SIM_SOPT1_OSC32KSEL_SHIFT                (18U)
#define SIM_SOPT1_OSC32KSEL(x)                   (((uint32_t)(((uint32_t)(x)) << SIM_SOPT1_OSC32KSEL_SHIFT)) & SIM_SOPT1_OSC32KSEL_MASK)

/*! @name SOPT2 - System Options Register 2 */
#define SIM_SOPT2_CLKOUTSEL_MASK                 (0xE0U)
#define SIM_SOPT2_CLKOUTSEL_SHIFT                (5U)
#define SIM_SOPT2_CLKOUTSEL(x)                   (((uint32_t)(((uint32_t)(x)) << SIM_SOPT2_CLKOUTSEL_SHIFT)) & SIM_SOPT2_CLKOUTSEL_MASK)
#define SIM_SOPT2_TPMSRC_MASK                    (0x3000000U)
#define SIM_SOPT2_TPMSRC_SHIFT                   (24U)
#define SIM_SOPT2_TPMSRC(x)                      (((uint32_t)(((uint32_t)(x)) << SIM_SOPT2_TPMSRC_SHIFT)) & SIM_SOPT2_TPMSRC_MASK)
#define SIM_SOPT2_LPUART0SRC_MASK                (0xC000000U)
#define SIM_SOPT2_LPUART0SRC_SHIFT               (26U)
#define SIM_SOPT2_LPUART0SRC(x)                  (((uint32_t)(((uint32_t)(x)) << SIM_SOPT2_LPUART0SRC_SHIFT)) & SIM_SOPT2_LPUART0SRC_MASK)

/*! @name SOPT4 - System Options Register 4 */
#define SIM_SOPT4_TPM1CH0SRC_MASK                (0x40000U)
#define SIM_SOPT4_TPM1CH0SRC_SHIFT               (18U)
#define SIM_SOPT4_TPM1CH0SRC(x)                  (((uint32_t)(((uint32_t)(x)) << SIM_SOPT4_TPM1CH0SRC_SHIFT)) & SIM_SOPT4_TPM1CH0SRC_MASK)
#define SIM_SOPT4_TPM2CH0SRC_MASK                (0x100000U)
#define SIM_SOPT4_TPM2CH0SRC_SHIFT               (20U)
#define SIM_SOPT4_TPM2CH0SRC(x)                  (((uint32_t)(((uint32_t)(x)) << SIM_SOPT4_TPM2CH0SRC_SHIFT)) & SIM_SOPT4_TPM2CH0SRC_MASK)
#define SIM_SOPT4_TPM0CLKSEL_MASK                (0x1000000U)
#define SIM_SOPT4_TPM0CLKSEL_SHIFT               (24U)
#define SIM_SOPT4_TPM0CLKSEL(x)                  (((uint32_t)(((uint32_t)(x)) << SIM_SOPT4_TPM0CLKSEL_SHIFT)) & SIM_SOPT4_TPM0CLKSEL_MASK)
#define SIM_SOPT4_TPM1CLKSEL_MASK                (0x2000000U)
#define SIM_SOPT4_TPM1CLKSEL_SHIFT               (25U)
#define SIM_SOPT4_TPM1CLKSEL(x)                  (((uint32_t)(((uint32_t)(x)) << SIM_SOPT4_TPM1CLKSEL_SHIFT)) & SIM_SOPT4_TPM1CLKSEL_MASK)
#define SIM_SOPT4_TPM2CLKSEL_MASK                (0x4000000U)
#define SIM_SOPT4_TPM2CLKSEL_SHIFT               (26U)
#define SIM_SOPT4_TPM2CLKSEL(x)                  (((uint32_t)(((uint32_t)(x)) << SIM_SOPT4_TPM2CLKSEL_SHIFT)) & SIM_SOPT4_TPM2CLKSEL_MASK)

/*! @name SOPT5 - System Options Register 5 */
#define SIM_SOPT5_LPUART0TXSRC_MASK              (0x3U)
#define SIM_SOPT5_LPUART0TXSRC_SHIFT             (0U)
#define SIM_SOPT5_LPUART0TXSRC(x)                (((uint32_t)(((uint32_t)(x)) << SIM_SOPT5_LPUART0TXSRC_SHIFT)) & SIM_SOPT5_LPUART0TXSRC_MASK)
#define SIM_SOPT5_LPUART0RXSRC_MASK              (0x4U)
#define SIM_SOPT5_LPUART0RXSRC_SHIFT             (2U)
#define SIM_SOPT5_LPUART0RXSRC(x)                (((uint32_t)(((uint32_t)(x)) << SIM_SOPT5_LPUART0RXSRC_SHIFT)) & SIM_SOPT5_LPUART0RXSRC_MASK)
#define SIM_SOPT5_LPUART0ODE_MASK                (0x10000U)
#define SIM_SOPT5_LPUART0ODE_SHIFT               (16U)
#define SIM_SOPT5_LPUART0ODE(x)                  (((uint32_t)(((uint32_t)(x)) << SIM_SOPT5_LPUART0ODE_SHIFT)) & SIM_SOPT5_LPUART0ODE_MASK)

/*! @name SOPT7 - System Options Register 7 */
#define SIM_SOPT7_ADC0TRGSEL_MASK                (0xFU)
#define SIM_SOPT7_ADC0TRGSEL_SHIFT               (0U)
#define SIM_SOPT7_ADC0TRGSEL(x)                  (((uint32_t)(((uint32_t)(x)) << SIM_SOPT7_ADC0TRGSEL_SHIFT)) & SIM_SOPT7_ADC0TRGSEL_MASK)
#define SIM_SOPT7_ADC0PRETRGSEL_MASK             (0x10U)
#define SIM_SOPT7_ADC0PRETRGSEL_SHIFT            (4U)
#define SIM_SOPT7_ADC0PRETRGSEL(x)               (((uint32_t)(((uint32_t)(x)) << SIM_SOPT7_ADC0PRETRGSEL_SHIFT)) & SIM_SOPT7_ADC0PRETRGSEL_MASK)
#define SIM_SOPT7_ADC0ALTTRGEN_MASK              (0x80U)
#define SIM_SOPT7_ADC0ALTTRGEN_SHIFT             (7U)
#define SIM_SOPT7_ADC0ALTTRGEN(x)                (((uint32_t)(((uint32_t)(x)) << SIM_SOPT7_ADC0ALTTRGEN_SHIFT)) & SIM_SOPT7_ADC0ALTTRGEN_MASK)

/*! @name SDID - System Device Identification Register */
#define SIM_SDID_PINID_MASK                      (0xFU)
#define SIM_SDID_PINID_SHIFT                     (0U)
#define SIM_SDID_PINID(x)                        (((uint32_t)(((uint32_t)(x)) << SIM_SDID_PINID_SHIFT)) & SIM_SDID_PINID_MASK)
#define SIM_SDID_DIEID_MASK                      (0xF80U)
#define SIM_SDID_DIEID_SHIFT                     (7U)
#define SIM_SDID_DIEID(x)                        (((uint32_t)(((uint32_t)(x)) << SIM_SDID_DIEID_SHIFT)) & SIM_SDID_DIEID_MASK)
#define SIM_SDID_REVID_MASK                      (0xF000U)
#define SIM_SDID_REVID_SHIFT                     (12U)
#define SIM_SDID_REVID(x)                        (((uint32_t)(((uint32_t)(x)) << SIM_SDID_REVID_SHIFT)) & SIM_SDID_REVID_MASK)
#define SIM_SDID_SRAMSIZE_MASK                   (0xF0000U)
#define SIM_SDID_SRAMSIZE_SHIFT                  (16U)
#define SIM_SDID_SRAMSIZE(x)                     (((uint32_t)(((uint32_t)(x)) << SIM_SDID_SRAMSIZE_SHIFT)) & SIM_SDID_SRAMSIZE_MASK)
#define SIM_SDID_SERIESID_MASK                   (0xF00000U)
#define SIM_SDID_SERIESID_SHIFT                  (20U)
#define SIM_SDID_SERIESID(x)                     (((uint32_t)(((uint32_t)(x)) << SIM_SDID_SERIESID_SHIFT)) & SIM_SDID_SERIESID_MASK)
#define SIM_SDID_SUBFAMID_MASK                   (0x3000000U)
#define SIM_SDID_SUBFAMID_SHIFT                  (24U)
#define SIM_SDID_SUBFAMID(x)                     (((uint32_t)(((uint32_t)(x)) << SIM_SDID_SUBFAMID_SHIFT)) & SIM_SDID_SUBFAMID_MASK)
#define SIM_SDID_FAMID_MASK                      (0xF0000000U)
#define SIM_SDID_FAMID_SHIFT                     (28U)
#define SIM_SDID_FAMID(x)                        (((uint32_t)(((uint32_t)(x)) << SIM_SDID_FAMID_SHIFT)) & SIM_SDID_FAMID_MASK)

/*! @name SCGC4 - System Clock Gating Control Register 4 */
#define SIM_SCGC4_CMT_MASK                       (0x4U)
#define SIM_SCGC4_CMT_SHIFT                      (2U)
#define SIM_SCGC4_CMT(x)                         (((uint32_t)(((uint32_t)(x)) << SIM_SCGC4_CMT_SHIFT)) & SIM_SCGC4_CMT_MASK)
#define SIM_SCGC4_I2C0_MASK                      (0x40U)
#define SIM_SCGC4_I2C0_SHIFT                     (6U)
#define SIM_SCGC4_I2C0(x)                        (((uint32_t)(((uint32_t)(x)) << SIM_SCGC4_I2C0_SHIFT)) & SIM_SCGC4_I2C0_MASK)
#define SIM_SCGC4_I2C1_MASK                      (0x80U)
#define SIM_SCGC4_I2C1_SHIFT                     (7U)
#define SIM_SCGC4_I2C1(x)                        (((uint32_t)(((uint32_t)(x)) << SIM_SCGC4_I2C1_SHIFT)) & SIM_SCGC4_I2C1_MASK)
#define SIM_SCGC4_CMP_MASK                       (0x80000U)
#define SIM_SCGC4_CMP_SHIFT                      (19U)
#define SIM_SCGC4_CMP(x)                         (((uint32_t)(((uint32_t)(x)) << SIM_SCGC4_CMP_SHIFT)) & SIM_SCGC4_CMP_MASK)
#define SIM_SCGC4_VREF_MASK                      (0x100000U)
#define SIM_SCGC4_VREF_SHIFT                     (20U)
#define SIM_SCGC4_VREF(x)                        (((uint32_t)(((uint32_t)(x)) << SIM_SCGC4_VREF_SHIFT)) & SIM_SCGC4_VREF_MASK)

/*! @name SCGC5 - System Clock Gating Control Register 5 */
#define SIM_SCGC5_LPTMR_MASK                     (0x1U)
#define SIM_SCGC5_LPTMR_SHIFT                    (0U)
#define SIM_SCGC5_LPTMR(x)                       (((uint32_t)(((uint32_t)(x)) << SIM_SCGC5_LPTMR_SHIFT)) & SIM_SCGC5_LPTMR_MASK)
#define SIM_SCGC5_TSI_MASK                       (0x20U)
#define SIM_SCGC5_TSI_SHIFT                      (5U)
#define SIM_SCGC5_TSI(x)                         (((uint32_t)(((uint32_t)(x)) << SIM_SCGC5_TSI_SHIFT)) & SIM_SCGC5_TSI_MASK)
#define SIM_SCGC5_PORTA_MASK                     (0x200U)
#define SIM_SCGC5_PORTA_SHIFT                    (9U)
#define SIM_SCGC5_PORTA(x)                       (((uint32_t)(((uint32_t)(x)) << SIM_SCGC5_PORTA_SHIFT)) & SIM_SCGC5_PORTA_MASK)
#define SIM_SCGC5_PORTB_MASK                     (0x400U)
#define SIM_SCGC5_PORTB_SHIFT                    (10U)
#define SIM_SCGC5_PORTB(x)                       (((uint32_t)(((uint32_t)(x)) << SIM_SCGC5_PORTB_SHIFT)) & SIM_SCGC5_PORTB_MASK)
#define SIM_SCGC5_PORTC_MASK                     (0x800U)
#define SIM_SCGC5_PORTC_SHIFT                    (11U)
#define SIM_SCGC5_PORTC(x)                       (((uint32_t)(((uint32_t)(x)) << SIM_SCGC5_PORTC_SHIFT)) & SIM_SCGC5_PORTC_MASK)
#define SIM_SCGC5_LPUART0_MASK                   (0x100000U)
#define SIM_SCGC5_LPUART0_SHIFT                  (20U)
#define SIM_SCGC5_LPUART0(x)                     (((uint32_t)(((uint32_t)(x)) << SIM_SCGC5_LPUART0_SHIFT)) & SIM_SCGC5_LPUART0_MASK)
#define SIM_SCGC5_LTC_MASK                       (0x1000000U)
#define SIM_SCGC5_LTC_SHIFT                      (24U)
#define SIM_SCGC5_LTC(x)                         (((uint32_t)(((uint32_t)(x)) << SIM_SCGC5_LTC_SHIFT)) & SIM_SCGC5_LTC_MASK)
#define SIM_SCGC5_RSIM_MASK                      (0x2000000U)
#define SIM_SCGC5_RSIM_SHIFT                     (25U)
#define SIM_SCGC5_RSIM(x)                        (((uint32_t)(((uint32_t)(x)) << SIM_SCGC5_RSIM_SHIFT)) & SIM_SCGC5_RSIM_MASK)
#define SIM_SCGC5_DCDC_MASK                      (0x4000000U)
#define SIM_SCGC5_DCDC_SHIFT                     (26U)
#define SIM_SCGC5_DCDC(x)                        (((uint32_t)(((uint32_t)(x)) << SIM_SCGC5_DCDC_SHIFT)) & SIM_SCGC5_DCDC_MASK)
#define SIM_SCGC5_BTLL_MASK                      (0x8000000U)
#define SIM_SCGC5_BTLL_SHIFT                     (27U)
#define SIM_SCGC5_BTLL(x)                        (((uint32_t)(((uint32_t)(x)) << SIM_SCGC5_BTLL_SHIFT)) & SIM_SCGC5_BTLL_MASK)
#define SIM_SCGC5_PHYDIG_MASK                    (0x10000000U)
#define SIM_SCGC5_PHYDIG_SHIFT                   (28U)
#define SIM_SCGC5_PHYDIG(x)                      (((uint32_t)(((uint32_t)(x)) << SIM_SCGC5_PHYDIG_SHIFT)) & SIM_SCGC5_PHYDIG_MASK)
#define SIM_SCGC5_ZigBee_MASK                    (0x20000000U)
#define SIM_SCGC5_ZigBee_SHIFT                   (29U)
#define SIM_SCGC5_ZigBee(x)                      (((uint32_t)(((uint32_t)(x)) << SIM_SCGC5_ZigBee_SHIFT)) & SIM_SCGC5_ZigBee_MASK)
#define SIM_SCGC5_ANT_MASK                       (0x40000000U)
#define SIM_SCGC5_ANT_SHIFT                      (30U)
#define SIM_SCGC5_ANT(x)                         (((uint32_t)(((uint32_t)(x)) << SIM_SCGC5_ANT_SHIFT)) & SIM_SCGC5_ANT_MASK)
#define SIM_SCGC5_GEN_FSK_MASK                   (0x80000000U)
#define SIM_SCGC5_GEN_FSK_SHIFT                  (31U)
#define SIM_SCGC5_GEN_FSK(x)                     (((uint32_t)(((uint32_t)(x)) << SIM_SCGC5_GEN_FSK_SHIFT)) & SIM_SCGC5_GEN_FSK_MASK)

/*! @name SCGC6 - System Clock Gating Control Register 6 */
#define SIM_SCGC6_FTF_MASK                       (0x1U)
#define SIM_SCGC6_FTF_SHIFT                      (0U)
#define SIM_SCGC6_FTF(x)                         (((uint32_t)(((uint32_t)(x)) << SIM_SCGC6_FTF_SHIFT)) & SIM_SCGC6_FTF_MASK)
#define SIM_SCGC6_DMAMUX_MASK                    (0x2U)
#define SIM_SCGC6_DMAMUX_SHIFT                   (1U)
#define SIM_SCGC6_DMAMUX(x)                      (((uint32_t)(((uint32_t)(x)) << SIM_SCGC6_DMAMUX_SHIFT)) & SIM_SCGC6_DMAMUX_MASK)
#define SIM_SCGC6_TRNG_MASK                      (0x200U)
#define SIM_SCGC6_TRNG_SHIFT                     (9U)
#define SIM_SCGC6_TRNG(x)                        (((uint32_t)(((uint32_t)(x)) << SIM_SCGC6_TRNG_SHIFT)) & SIM_SCGC6_TRNG_MASK)
#define SIM_SCGC6_SPI0_MASK                      (0x1000U)
#define SIM_SCGC6_SPI0_SHIFT                     (12U)
#define SIM_SCGC6_SPI0(x)                        (((uint32_t)(((uint32_t)(x)) << SIM_SCGC6_SPI0_SHIFT)) & SIM_SCGC6_SPI0_MASK)
#define SIM_SCGC6_SPI1_MASK                      (0x2000U)
#define SIM_SCGC6_SPI1_SHIFT                     (13U)
#define SIM_SCGC6_SPI1(x)                        (((uint32_t)(((uint32_t)(x)) << SIM_SCGC6_SPI1_SHIFT)) & SIM_SCGC6_SPI1_MASK)
#define SIM_SCGC6_PIT_MASK                       (0x800000U)
#define SIM_SCGC6_PIT_SHIFT                      (23U)
#define SIM_SCGC6_PIT(x)                         (((uint32_t)(((uint32_t)(x)) << SIM_SCGC6_PIT_SHIFT)) & SIM_SCGC6_PIT_MASK)
#define SIM_SCGC6_TPM0_MASK                      (0x1000000U)
#define SIM_SCGC6_TPM0_SHIFT                     (24U)
#define SIM_SCGC6_TPM0(x)                        (((uint32_t)(((uint32_t)(x)) << SIM_SCGC6_TPM0_SHIFT)) & SIM_SCGC6_TPM0_MASK)
#define SIM_SCGC6_TPM1_MASK                      (0x2000000U)
#define SIM_SCGC6_TPM1_SHIFT                     (25U)
#define SIM_SCGC6_TPM1(x)                        (((uint32_t)(((uint32_t)(x)) << SIM_SCGC6_TPM1_SHIFT)) & SIM_SCGC6_TPM1_MASK)
#define SIM_SCGC6_TPM2_MASK                      (0x4000000U)
#define SIM_SCGC6_TPM2_SHIFT                     (26U)
#define SIM_SCGC6_TPM2(x)                        (((uint32_t)(((uint32_t)(x)) << SIM_SCGC6_TPM2_SHIFT)) & SIM_SCGC6_TPM2_MASK)
#define SIM_SCGC6_ADC0_MASK                      (0x8000000U)
#define SIM_SCGC6_ADC0_SHIFT                     (27U)
#define SIM_SCGC6_ADC0(x)                        (((uint32_t)(((uint32_t)(x)) << SIM_SCGC6_ADC0_SHIFT)) & SIM_SCGC6_ADC0_MASK)
#define SIM_SCGC6_RTC_MASK                       (0x20000000U)
#define SIM_SCGC6_RTC_SHIFT                      (29U)
#define SIM_SCGC6_RTC(x)                         (((uint32_t)(((uint32_t)(x)) << SIM_SCGC6_RTC_SHIFT)) & SIM_SCGC6_RTC_MASK)
#define SIM_SCGC6_DAC0_MASK                      (0x80000000U)
#define SIM_SCGC6_DAC0_SHIFT                     (31U)
#define SIM_SCGC6_DAC0(x)                        (((uint32_t)(((uint32_t)(x)) << SIM_SCGC6_DAC0_SHIFT)) & SIM_SCGC6_DAC0_MASK)

/*! @name SCGC7 - System Clock Gating Control Register 7 */
#define SIM_SCGC7_DMA_MASK                       (0x100U)
#define SIM_SCGC7_DMA_SHIFT                      (8U)
#define SIM_SCGC7_DMA(x)                         (((uint32_t)(((uint32_t)(x)) << SIM_SCGC7_DMA_SHIFT)) & SIM_SCGC7_DMA_MASK)

/*! @name CLKDIV1 - System Clock Divider Register 1 */
#define SIM_CLKDIV1_OUTDIV4_MASK                 (0x70000U)
#define SIM_CLKDIV1_OUTDIV4_SHIFT                (16U)
#define SIM_CLKDIV1_OUTDIV4(x)                   (((uint32_t)(((uint32_t)(x)) << SIM_CLKDIV1_OUTDIV4_SHIFT)) & SIM_CLKDIV1_OUTDIV4_MASK)
#define SIM_CLKDIV1_OUTDIV1_MASK                 (0xF0000000U)
#define SIM_CLKDIV1_OUTDIV1_SHIFT                (28U)
#define SIM_CLKDIV1_OUTDIV1(x)                   (((uint32_t)(((uint32_t)(x)) << SIM_CLKDIV1_OUTDIV1_SHIFT)) & SIM_CLKDIV1_OUTDIV1_MASK)

/*! @name FCFG1 - Flash Configuration Register 1 */
#define SIM_FCFG1_FLASHDIS_MASK                  (0x1U)
#define SIM_FCFG1_FLASHDIS_SHIFT                 (0U)
#define SIM_FCFG1_FLASHDIS(x)                    (((uint32_t)(((uint32_t)(x)) << SIM_FCFG1_FLASHDIS_SHIFT)) & SIM_FCFG1_FLASHDIS_MASK)
#define SIM_FCFG1_FLASHDOZE_MASK                 (0x2U)
#define SIM_FCFG1_FLASHDOZE_SHIFT                (1U)
#define SIM_FCFG1_FLASHDOZE(x)                   (((uint32_t)(((uint32_t)(x)) << SIM_FCFG1_FLASHDOZE_SHIFT)) & SIM_FCFG1_FLASHDOZE_MASK)
#define SIM_FCFG1_PFSIZE_MASK                    (0xF000000U)
#define SIM_FCFG1_PFSIZE_SHIFT                   (24U)
#define SIM_FCFG1_PFSIZE(x)                      (((uint32_t)(((uint32_t)(x)) << SIM_FCFG1_PFSIZE_SHIFT)) & SIM_FCFG1_PFSIZE_MASK)

/*! @name FCFG2 - Flash Configuration Register 2 */
#define SIM_FCFG2_MAXADDR1_MASK                  (0x7F0000U)
#define SIM_FCFG2_MAXADDR1_SHIFT                 (16U)
#define SIM_FCFG2_MAXADDR1(x)                    (((uint32_t)(((uint32_t)(x)) << SIM_FCFG2_MAXADDR1_SHIFT)) & SIM_FCFG2_MAXADDR1_MASK)
#define SIM_FCFG2_MAXADDR0_MASK                  (0x7F000000U)
#define SIM_FCFG2_MAXADDR0_SHIFT                 (24U)
#define SIM_FCFG2_MAXADDR0(x)                    (((uint32_t)(((uint32_t)(x)) << SIM_FCFG2_MAXADDR0_SHIFT)) & SIM_FCFG2_MAXADDR0_MASK)

/*! @name UIDMH - Unique Identification Register Mid-High */
#define SIM_UIDMH_UID_MASK                       (0xFFFFU)
#define SIM_UIDMH_UID_SHIFT                      (0U)
#define SIM_UIDMH_UID(x)                         (((uint32_t)(((uint32_t)(x)) << SIM_UIDMH_UID_SHIFT)) & SIM_UIDMH_UID_MASK)

/*! @name UIDML - Unique Identification Register Mid Low */
#define SIM_UIDML_UID_MASK                       (0xFFFFFFFFU)
#define SIM_UIDML_UID_SHIFT                      (0U)
#define SIM_UIDML_UID(x)                         (((uint32_t)(((uint32_t)(x)) << SIM_UIDML_UID_SHIFT)) & SIM_UIDML_UID_MASK)

/*! @name UIDL - Unique Identification Register Low */
#define SIM_UIDL_UID_MASK                        (0xFFFFFFFFU)
#define SIM_UIDL_UID_SHIFT                       (0U)
#define SIM_UIDL_UID(x)                          (((uint32_t)(((uint32_t)(x)) << SIM_UIDL_UID_SHIFT)) & SIM_UIDL_UID_MASK)

/*! @name COPC - COP Control Register */
#define SIM_COPC_COPW_MASK                       (0x1U)
#define SIM_COPC_COPW_SHIFT                      (0U)
#define SIM_COPC_COPW(x)                         (((uint32_t)(((uint32_t)(x)) << SIM_COPC_COPW_SHIFT)) & SIM_COPC_COPW_MASK)
#define SIM_COPC_COPCLKS_MASK                    (0x2U)
#define SIM_COPC_COPCLKS_SHIFT                   (1U)
#define SIM_COPC_COPCLKS(x)                      (((uint32_t)(((uint32_t)(x)) << SIM_COPC_COPCLKS_SHIFT)) & SIM_COPC_COPCLKS_MASK)
#define SIM_COPC_COPT_MASK                       (0xCU)
#define SIM_COPC_COPT_SHIFT                      (2U)
#define SIM_COPC_COPT(x)                         (((uint32_t)(((uint32_t)(x)) << SIM_COPC_COPT_SHIFT)) & SIM_COPC_COPT_MASK)
#define SIM_COPC_COPSTPEN_MASK                   (0x10U)
#define SIM_COPC_COPSTPEN_SHIFT                  (4U)
#define SIM_COPC_COPSTPEN(x)                     (((uint32_t)(((uint32_t)(x)) << SIM_COPC_COPSTPEN_SHIFT)) & SIM_COPC_COPSTPEN_MASK)
#define SIM_COPC_COPDBGEN_MASK                   (0x20U)
#define SIM_COPC_COPDBGEN_SHIFT                  (5U)
#define SIM_COPC_COPDBGEN(x)                     (((uint32_t)(((uint32_t)(x)) << SIM_COPC_COPDBGEN_SHIFT)) & SIM_COPC_COPDBGEN_MASK)
#define SIM_COPC_COPCLKSEL_MASK                  (0xC0U)
#define SIM_COPC_COPCLKSEL_SHIFT                 (6U)
#define SIM_COPC_COPCLKSEL(x)                    (((uint32_t)(((uint32_t)(x)) << SIM_COPC_COPCLKSEL_SHIFT)) & SIM_COPC_COPCLKSEL_MASK)

/*! @name SRVCOP - Service COP */
#define SIM_SRVCOP_SRVCOP_MASK                   (0xFFU)
#define SIM_SRVCOP_SRVCOP_SHIFT                  (0U)
#define SIM_SRVCOP_SRVCOP(x)                     (((uint32_t)(((uint32_t)(x)) << SIM_SRVCOP_SRVCOP_SHIFT)) & SIM_SRVCOP_SRVCOP_MASK)


/*!
 * @}
 */ /* end of group SIM_Register_Masks */


/* SIM - Peripheral instance base addresses */
/** Peripheral SIM base address */
#define SIM_BASE                                 (0x40047000u)
/** Peripheral SIM base pointer */
#define SIM                                      ((SIM_Type *)SIM_BASE)
/** Array initializer of SIM peripheral base addresses */
#define SIM_BASE_ADDRS                           { SIM_BASE }
/** Array initializer of SIM peripheral base pointers */
#define SIM_BASE_PTRS                            { SIM }

/*!
 * @}
 */ /* end of group SIM_Peripheral_Access_Layer */


/* ----------------------------------------------------------------------------
   -- SMC Peripheral Access Layer
   ---------------------------------------------------------------------------- */

/*!
 * @addtogroup SMC_Peripheral_Access_Layer SMC Peripheral Access Layer
 * @{
 */

/** SMC - Register Layout Typedef */
typedef struct {
  __IO uint8_t PMPROT;                             /**< Power Mode Protection register, offset: 0x0 */
  __IO uint8_t PMCTRL;                             /**< Power Mode Control register, offset: 0x1 */
  __IO uint8_t STOPCTRL;                           /**< Stop Control Register, offset: 0x2 */
  __I  uint8_t PMSTAT;                             /**< Power Mode Status register, offset: 0x3 */
} SMC_Type;

/* ----------------------------------------------------------------------------
   -- SMC Register Masks
   ---------------------------------------------------------------------------- */

/*!
 * @addtogroup SMC_Register_Masks SMC Register Masks
 * @{
 */

/*! @name PMPROT - Power Mode Protection register */
#define SMC_PMPROT_AVLLS_MASK                    (0x2U)
#define SMC_PMPROT_AVLLS_SHIFT                   (1U)
#define SMC_PMPROT_AVLLS(x)                      (((uint8_t)(((uint8_t)(x)) << SMC_PMPROT_AVLLS_SHIFT)) & SMC_PMPROT_AVLLS_MASK)
#define SMC_PMPROT_ALLS_MASK                     (0x8U)
#define SMC_PMPROT_ALLS_SHIFT                    (3U)
#define SMC_PMPROT_ALLS(x)                       (((uint8_t)(((uint8_t)(x)) << SMC_PMPROT_ALLS_SHIFT)) & SMC_PMPROT_ALLS_MASK)
#define SMC_PMPROT_AVLP_MASK                     (0x20U)
#define SMC_PMPROT_AVLP_SHIFT                    (5U)
#define SMC_PMPROT_AVLP(x)                       (((uint8_t)(((uint8_t)(x)) << SMC_PMPROT_AVLP_SHIFT)) & SMC_PMPROT_AVLP_MASK)

/*! @name PMCTRL - Power Mode Control register */
#define SMC_PMCTRL_STOPM_MASK                    (0x7U)
#define SMC_PMCTRL_STOPM_SHIFT                   (0U)
#define SMC_PMCTRL_STOPM(x)                      (((uint8_t)(((uint8_t)(x)) << SMC_PMCTRL_STOPM_SHIFT)) & SMC_PMCTRL_STOPM_MASK)
#define SMC_PMCTRL_STOPA_MASK                    (0x8U)
#define SMC_PMCTRL_STOPA_SHIFT                   (3U)
#define SMC_PMCTRL_STOPA(x)                      (((uint8_t)(((uint8_t)(x)) << SMC_PMCTRL_STOPA_SHIFT)) & SMC_PMCTRL_STOPA_MASK)
#define SMC_PMCTRL_RUNM_MASK                     (0x60U)
#define SMC_PMCTRL_RUNM_SHIFT                    (5U)
#define SMC_PMCTRL_RUNM(x)                       (((uint8_t)(((uint8_t)(x)) << SMC_PMCTRL_RUNM_SHIFT)) & SMC_PMCTRL_RUNM_MASK)

/*! @name STOPCTRL - Stop Control Register */
#define SMC_STOPCTRL_LLSM_MASK                   (0x7U)
#define SMC_STOPCTRL_LLSM_SHIFT                  (0U)
#define SMC_STOPCTRL_LLSM(x)                     (((uint8_t)(((uint8_t)(x)) << SMC_STOPCTRL_LLSM_SHIFT)) & SMC_STOPCTRL_LLSM_MASK)
#define SMC_STOPCTRL_RAM2PO_MASK                 (0x10U)
#define SMC_STOPCTRL_RAM2PO_SHIFT                (4U)
#define SMC_STOPCTRL_RAM2PO(x)                   (((uint8_t)(((uint8_t)(x)) << SMC_STOPCTRL_RAM2PO_SHIFT)) & SMC_STOPCTRL_RAM2PO_MASK)
#define SMC_STOPCTRL_PORPO_MASK                  (0x20U)
#define SMC_STOPCTRL_PORPO_SHIFT                 (5U)
#define SMC_STOPCTRL_PORPO(x)                    (((uint8_t)(((uint8_t)(x)) << SMC_STOPCTRL_PORPO_SHIFT)) & SMC_STOPCTRL_PORPO_MASK)
#define SMC_STOPCTRL_PSTOPO_MASK                 (0xC0U)
#define SMC_STOPCTRL_PSTOPO_SHIFT                (6U)
#define SMC_STOPCTRL_PSTOPO(x)                   (((uint8_t)(((uint8_t)(x)) << SMC_STOPCTRL_PSTOPO_SHIFT)) & SMC_STOPCTRL_PSTOPO_MASK)

/*! @name PMSTAT - Power Mode Status register */
#define SMC_PMSTAT_PMSTAT_MASK                   (0xFFU)
#define SMC_PMSTAT_PMSTAT_SHIFT                  (0U)
#define SMC_PMSTAT_PMSTAT(x)                     (((uint8_t)(((uint8_t)(x)) << SMC_PMSTAT_PMSTAT_SHIFT)) & SMC_PMSTAT_PMSTAT_MASK)


/*!
 * @}
 */ /* end of group SMC_Register_Masks */


/* SMC - Peripheral instance base addresses */
/** Peripheral SMC base address */
#define SMC_BASE                                 (0x4007E000u)
/** Peripheral SMC base pointer */
#define SMC                                      ((SMC_Type *)SMC_BASE)
/** Array initializer of SMC peripheral base addresses */
#define SMC_BASE_ADDRS                           { SMC_BASE }
/** Array initializer of SMC peripheral base pointers */
#define SMC_BASE_PTRS                            { SMC }

/*!
 * @}
 */ /* end of group SMC_Peripheral_Access_Layer */


/* ----------------------------------------------------------------------------
   -- SPI Peripheral Access Layer
   ---------------------------------------------------------------------------- */

/*!
 * @addtogroup SPI_Peripheral_Access_Layer SPI Peripheral Access Layer
 * @{
 */

/** SPI - Register Layout Typedef */
typedef struct {
  __IO uint32_t MCR;                               /**< Module Configuration Register, offset: 0x0 */
       uint8_t RESERVED_0[4];
  __IO uint32_t TCR;                               /**< Transfer Count Register, offset: 0x8 */
  union {                                          /* offset: 0xC */
    __IO uint32_t CTAR[2];                           /**< Clock and Transfer Attributes Register (In Master Mode), array offset: 0xC, array step: 0x4 */
    __IO uint32_t CTAR_SLAVE[1];                     /**< Clock and Transfer Attributes Register (In Slave Mode), array offset: 0xC, array step: 0x4 */
  };
       uint8_t RESERVED_1[24];
  __IO uint32_t SR;                                /**< Status Register, offset: 0x2C */
  __IO uint32_t RSER;                              /**< DMA/Interrupt Request Select and Enable Register, offset: 0x30 */
  union {                                          /* offset: 0x34 */
    __IO uint32_t PUSHR;                             /**< PUSH TX FIFO Register In Master Mode, offset: 0x34 */
    __IO uint32_t PUSHR_SLAVE;                       /**< PUSH TX FIFO Register In Slave Mode, offset: 0x34 */
  };
  __I  uint32_t POPR;                              /**< POP RX FIFO Register, offset: 0x38 */
  __I  uint32_t TXFR0;                             /**< Transmit FIFO Registers, offset: 0x3C */
  __I  uint32_t TXFR1;                             /**< Transmit FIFO Registers, offset: 0x40 */
  __I  uint32_t TXFR2;                             /**< Transmit FIFO Registers, offset: 0x44 */
  __I  uint32_t TXFR3;                             /**< Transmit FIFO Registers, offset: 0x48 */
       uint8_t RESERVED_2[48];
  __I  uint32_t RXFR0;                             /**< Receive FIFO Registers, offset: 0x7C */
  __I  uint32_t RXFR1;                             /**< Receive FIFO Registers, offset: 0x80 */
  __I  uint32_t RXFR2;                             /**< Receive FIFO Registers, offset: 0x84 */
  __I  uint32_t RXFR3;                             /**< Receive FIFO Registers, offset: 0x88 */
} SPI_Type;

/* ----------------------------------------------------------------------------
   -- SPI Register Masks
   ---------------------------------------------------------------------------- */

/*!
 * @addtogroup SPI_Register_Masks SPI Register Masks
 * @{
 */

/*! @name MCR - Module Configuration Register */
#define SPI_MCR_HALT_MASK                        (0x1U)
#define SPI_MCR_HALT_SHIFT                       (0U)
#define SPI_MCR_HALT(x)                          (((uint32_t)(((uint32_t)(x)) << SPI_MCR_HALT_SHIFT)) & SPI_MCR_HALT_MASK)
#define SPI_MCR_SMPL_PT_MASK                     (0x300U)
#define SPI_MCR_SMPL_PT_SHIFT                    (8U)
#define SPI_MCR_SMPL_PT(x)                       (((uint32_t)(((uint32_t)(x)) << SPI_MCR_SMPL_PT_SHIFT)) & SPI_MCR_SMPL_PT_MASK)
#define SPI_MCR_CLR_RXF_MASK                     (0x400U)
#define SPI_MCR_CLR_RXF_SHIFT                    (10U)
#define SPI_MCR_CLR_RXF(x)                       (((uint32_t)(((uint32_t)(x)) << SPI_MCR_CLR_RXF_SHIFT)) & SPI_MCR_CLR_RXF_MASK)
#define SPI_MCR_CLR_TXF_MASK                     (0x800U)
#define SPI_MCR_CLR_TXF_SHIFT                    (11U)
#define SPI_MCR_CLR_TXF(x)                       (((uint32_t)(((uint32_t)(x)) << SPI_MCR_CLR_TXF_SHIFT)) & SPI_MCR_CLR_TXF_MASK)
#define SPI_MCR_DIS_RXF_MASK                     (0x1000U)
#define SPI_MCR_DIS_RXF_SHIFT                    (12U)
#define SPI_MCR_DIS_RXF(x)                       (((uint32_t)(((uint32_t)(x)) << SPI_MCR_DIS_RXF_SHIFT)) & SPI_MCR_DIS_RXF_MASK)
#define SPI_MCR_DIS_TXF_MASK                     (0x2000U)
#define SPI_MCR_DIS_TXF_SHIFT                    (13U)
#define SPI_MCR_DIS_TXF(x)                       (((uint32_t)(((uint32_t)(x)) << SPI_MCR_DIS_TXF_SHIFT)) & SPI_MCR_DIS_TXF_MASK)
#define SPI_MCR_MDIS_MASK                        (0x4000U)
#define SPI_MCR_MDIS_SHIFT                       (14U)
#define SPI_MCR_MDIS(x)                          (((uint32_t)(((uint32_t)(x)) << SPI_MCR_MDIS_SHIFT)) & SPI_MCR_MDIS_MASK)
#define SPI_MCR_DOZE_MASK                        (0x8000U)
#define SPI_MCR_DOZE_SHIFT                       (15U)
#define SPI_MCR_DOZE(x)                          (((uint32_t)(((uint32_t)(x)) << SPI_MCR_DOZE_SHIFT)) & SPI_MCR_DOZE_MASK)
#define SPI_MCR_PCSIS_MASK                       (0xF0000U)
#define SPI_MCR_PCSIS_SHIFT                      (16U)
#define SPI_MCR_PCSIS(x)                         (((uint32_t)(((uint32_t)(x)) << SPI_MCR_PCSIS_SHIFT)) & SPI_MCR_PCSIS_MASK)
#define SPI_MCR_ROOE_MASK                        (0x1000000U)
#define SPI_MCR_ROOE_SHIFT                       (24U)
#define SPI_MCR_ROOE(x)                          (((uint32_t)(((uint32_t)(x)) << SPI_MCR_ROOE_SHIFT)) & SPI_MCR_ROOE_MASK)
#define SPI_MCR_MTFE_MASK                        (0x4000000U)
#define SPI_MCR_MTFE_SHIFT                       (26U)
#define SPI_MCR_MTFE(x)                          (((uint32_t)(((uint32_t)(x)) << SPI_MCR_MTFE_SHIFT)) & SPI_MCR_MTFE_MASK)
#define SPI_MCR_FRZ_MASK                         (0x8000000U)
#define SPI_MCR_FRZ_SHIFT                        (27U)
#define SPI_MCR_FRZ(x)                           (((uint32_t)(((uint32_t)(x)) << SPI_MCR_FRZ_SHIFT)) & SPI_MCR_FRZ_MASK)
#define SPI_MCR_DCONF_MASK                       (0x30000000U)
#define SPI_MCR_DCONF_SHIFT                      (28U)
#define SPI_MCR_DCONF(x)                         (((uint32_t)(((uint32_t)(x)) << SPI_MCR_DCONF_SHIFT)) & SPI_MCR_DCONF_MASK)
#define SPI_MCR_CONT_SCKE_MASK                   (0x40000000U)
#define SPI_MCR_CONT_SCKE_SHIFT                  (30U)
#define SPI_MCR_CONT_SCKE(x)                     (((uint32_t)(((uint32_t)(x)) << SPI_MCR_CONT_SCKE_SHIFT)) & SPI_MCR_CONT_SCKE_MASK)
#define SPI_MCR_MSTR_MASK                        (0x80000000U)
#define SPI_MCR_MSTR_SHIFT                       (31U)
#define SPI_MCR_MSTR(x)                          (((uint32_t)(((uint32_t)(x)) << SPI_MCR_MSTR_SHIFT)) & SPI_MCR_MSTR_MASK)

/*! @name TCR - Transfer Count Register */
#define SPI_TCR_SPI_TCNT_MASK                    (0xFFFF0000U)
#define SPI_TCR_SPI_TCNT_SHIFT                   (16U)
#define SPI_TCR_SPI_TCNT(x)                      (((uint32_t)(((uint32_t)(x)) << SPI_TCR_SPI_TCNT_SHIFT)) & SPI_TCR_SPI_TCNT_MASK)

/*! @name CTAR - Clock and Transfer Attributes Register (In Master Mode) */
#define SPI_CTAR_BR_MASK                         (0xFU)
#define SPI_CTAR_BR_SHIFT                        (0U)
#define SPI_CTAR_BR(x)                           (((uint32_t)(((uint32_t)(x)) << SPI_CTAR_BR_SHIFT)) & SPI_CTAR_BR_MASK)
#define SPI_CTAR_DT_MASK                         (0xF0U)
#define SPI_CTAR_DT_SHIFT                        (4U)
#define SPI_CTAR_DT(x)                           (((uint32_t)(((uint32_t)(x)) << SPI_CTAR_DT_SHIFT)) & SPI_CTAR_DT_MASK)
#define SPI_CTAR_ASC_MASK                        (0xF00U)
#define SPI_CTAR_ASC_SHIFT                       (8U)
#define SPI_CTAR_ASC(x)                          (((uint32_t)(((uint32_t)(x)) << SPI_CTAR_ASC_SHIFT)) & SPI_CTAR_ASC_MASK)
#define SPI_CTAR_CSSCK_MASK                      (0xF000U)
#define SPI_CTAR_CSSCK_SHIFT                     (12U)
#define SPI_CTAR_CSSCK(x)                        (((uint32_t)(((uint32_t)(x)) << SPI_CTAR_CSSCK_SHIFT)) & SPI_CTAR_CSSCK_MASK)
#define SPI_CTAR_PBR_MASK                        (0x30000U)
#define SPI_CTAR_PBR_SHIFT                       (16U)
#define SPI_CTAR_PBR(x)                          (((uint32_t)(((uint32_t)(x)) << SPI_CTAR_PBR_SHIFT)) & SPI_CTAR_PBR_MASK)
#define SPI_CTAR_PDT_MASK                        (0xC0000U)
#define SPI_CTAR_PDT_SHIFT                       (18U)
#define SPI_CTAR_PDT(x)                          (((uint32_t)(((uint32_t)(x)) << SPI_CTAR_PDT_SHIFT)) & SPI_CTAR_PDT_MASK)
#define SPI_CTAR_PASC_MASK                       (0x300000U)
#define SPI_CTAR_PASC_SHIFT                      (20U)
#define SPI_CTAR_PASC(x)                         (((uint32_t)(((uint32_t)(x)) << SPI_CTAR_PASC_SHIFT)) & SPI_CTAR_PASC_MASK)
#define SPI_CTAR_PCSSCK_MASK                     (0xC00000U)
#define SPI_CTAR_PCSSCK_SHIFT                    (22U)
#define SPI_CTAR_PCSSCK(x)                       (((uint32_t)(((uint32_t)(x)) << SPI_CTAR_PCSSCK_SHIFT)) & SPI_CTAR_PCSSCK_MASK)
#define SPI_CTAR_LSBFE_MASK                      (0x1000000U)
#define SPI_CTAR_LSBFE_SHIFT                     (24U)
#define SPI_CTAR_LSBFE(x)                        (((uint32_t)(((uint32_t)(x)) << SPI_CTAR_LSBFE_SHIFT)) & SPI_CTAR_LSBFE_MASK)
#define SPI_CTAR_CPHA_MASK                       (0x2000000U)
#define SPI_CTAR_CPHA_SHIFT                      (25U)
#define SPI_CTAR_CPHA(x)                         (((uint32_t)(((uint32_t)(x)) << SPI_CTAR_CPHA_SHIFT)) & SPI_CTAR_CPHA_MASK)
#define SPI_CTAR_CPOL_MASK                       (0x4000000U)
#define SPI_CTAR_CPOL_SHIFT                      (26U)
#define SPI_CTAR_CPOL(x)                         (((uint32_t)(((uint32_t)(x)) << SPI_CTAR_CPOL_SHIFT)) & SPI_CTAR_CPOL_MASK)
#define SPI_CTAR_FMSZ_MASK                       (0x78000000U)
#define SPI_CTAR_FMSZ_SHIFT                      (27U)
#define SPI_CTAR_FMSZ(x)                         (((uint32_t)(((uint32_t)(x)) << SPI_CTAR_FMSZ_SHIFT)) & SPI_CTAR_FMSZ_MASK)
#define SPI_CTAR_DBR_MASK                        (0x80000000U)
#define SPI_CTAR_DBR_SHIFT                       (31U)
#define SPI_CTAR_DBR(x)                          (((uint32_t)(((uint32_t)(x)) << SPI_CTAR_DBR_SHIFT)) & SPI_CTAR_DBR_MASK)

/* The count of SPI_CTAR */
#define SPI_CTAR_COUNT                           (2U)

/*! @name CTAR_SLAVE - Clock and Transfer Attributes Register (In Slave Mode) */
#define SPI_CTAR_SLAVE_CPHA_MASK                 (0x2000000U)
#define SPI_CTAR_SLAVE_CPHA_SHIFT                (25U)
#define SPI_CTAR_SLAVE_CPHA(x)                   (((uint32_t)(((uint32_t)(x)) << SPI_CTAR_SLAVE_CPHA_SHIFT)) & SPI_CTAR_SLAVE_CPHA_MASK)
#define SPI_CTAR_SLAVE_CPOL_MASK                 (0x4000000U)
#define SPI_CTAR_SLAVE_CPOL_SHIFT                (26U)
#define SPI_CTAR_SLAVE_CPOL(x)                   (((uint32_t)(((uint32_t)(x)) << SPI_CTAR_SLAVE_CPOL_SHIFT)) & SPI_CTAR_SLAVE_CPOL_MASK)
#define SPI_CTAR_SLAVE_FMSZ_MASK                 (0x78000000U)
#define SPI_CTAR_SLAVE_FMSZ_SHIFT                (27U)
#define SPI_CTAR_SLAVE_FMSZ(x)                   (((uint32_t)(((uint32_t)(x)) << SPI_CTAR_SLAVE_FMSZ_SHIFT)) & SPI_CTAR_SLAVE_FMSZ_MASK)

/* The count of SPI_CTAR_SLAVE */
#define SPI_CTAR_SLAVE_COUNT                     (1U)

/*! @name SR - Status Register */
#define SPI_SR_POPNXTPTR_MASK                    (0xFU)
#define SPI_SR_POPNXTPTR_SHIFT                   (0U)
#define SPI_SR_POPNXTPTR(x)                      (((uint32_t)(((uint32_t)(x)) << SPI_SR_POPNXTPTR_SHIFT)) & SPI_SR_POPNXTPTR_MASK)
#define SPI_SR_RXCTR_MASK                        (0xF0U)
#define SPI_SR_RXCTR_SHIFT                       (4U)
#define SPI_SR_RXCTR(x)                          (((uint32_t)(((uint32_t)(x)) << SPI_SR_RXCTR_SHIFT)) & SPI_SR_RXCTR_MASK)
#define SPI_SR_TXNXTPTR_MASK                     (0xF00U)
#define SPI_SR_TXNXTPTR_SHIFT                    (8U)
#define SPI_SR_TXNXTPTR(x)                       (((uint32_t)(((uint32_t)(x)) << SPI_SR_TXNXTPTR_SHIFT)) & SPI_SR_TXNXTPTR_MASK)
#define SPI_SR_TXCTR_MASK                        (0xF000U)
#define SPI_SR_TXCTR_SHIFT                       (12U)
#define SPI_SR_TXCTR(x)                          (((uint32_t)(((uint32_t)(x)) << SPI_SR_TXCTR_SHIFT)) & SPI_SR_TXCTR_MASK)
#define SPI_SR_RFDF_MASK                         (0x20000U)
#define SPI_SR_RFDF_SHIFT                        (17U)
#define SPI_SR_RFDF(x)                           (((uint32_t)(((uint32_t)(x)) << SPI_SR_RFDF_SHIFT)) & SPI_SR_RFDF_MASK)
#define SPI_SR_RFOF_MASK                         (0x80000U)
#define SPI_SR_RFOF_SHIFT                        (19U)
#define SPI_SR_RFOF(x)                           (((uint32_t)(((uint32_t)(x)) << SPI_SR_RFOF_SHIFT)) & SPI_SR_RFOF_MASK)
#define SPI_SR_TFFF_MASK                         (0x2000000U)
#define SPI_SR_TFFF_SHIFT                        (25U)
#define SPI_SR_TFFF(x)                           (((uint32_t)(((uint32_t)(x)) << SPI_SR_TFFF_SHIFT)) & SPI_SR_TFFF_MASK)
#define SPI_SR_TFUF_MASK                         (0x8000000U)
#define SPI_SR_TFUF_SHIFT                        (27U)
#define SPI_SR_TFUF(x)                           (((uint32_t)(((uint32_t)(x)) << SPI_SR_TFUF_SHIFT)) & SPI_SR_TFUF_MASK)
#define SPI_SR_EOQF_MASK                         (0x10000000U)
#define SPI_SR_EOQF_SHIFT                        (28U)
#define SPI_SR_EOQF(x)                           (((uint32_t)(((uint32_t)(x)) << SPI_SR_EOQF_SHIFT)) & SPI_SR_EOQF_MASK)
#define SPI_SR_TXRXS_MASK                        (0x40000000U)
#define SPI_SR_TXRXS_SHIFT                       (30U)
#define SPI_SR_TXRXS(x)                          (((uint32_t)(((uint32_t)(x)) << SPI_SR_TXRXS_SHIFT)) & SPI_SR_TXRXS_MASK)
#define SPI_SR_TCF_MASK                          (0x80000000U)
#define SPI_SR_TCF_SHIFT                         (31U)
#define SPI_SR_TCF(x)                            (((uint32_t)(((uint32_t)(x)) << SPI_SR_TCF_SHIFT)) & SPI_SR_TCF_MASK)

/*! @name RSER - DMA/Interrupt Request Select and Enable Register */
#define SPI_RSER_RFDF_DIRS_MASK                  (0x10000U)
#define SPI_RSER_RFDF_DIRS_SHIFT                 (16U)
#define SPI_RSER_RFDF_DIRS(x)                    (((uint32_t)(((uint32_t)(x)) << SPI_RSER_RFDF_DIRS_SHIFT)) & SPI_RSER_RFDF_DIRS_MASK)
#define SPI_RSER_RFDF_RE_MASK                    (0x20000U)
#define SPI_RSER_RFDF_RE_SHIFT                   (17U)
#define SPI_RSER_RFDF_RE(x)                      (((uint32_t)(((uint32_t)(x)) << SPI_RSER_RFDF_RE_SHIFT)) & SPI_RSER_RFDF_RE_MASK)
#define SPI_RSER_RFOF_RE_MASK                    (0x80000U)
#define SPI_RSER_RFOF_RE_SHIFT                   (19U)
#define SPI_RSER_RFOF_RE(x)                      (((uint32_t)(((uint32_t)(x)) << SPI_RSER_RFOF_RE_SHIFT)) & SPI_RSER_RFOF_RE_MASK)
#define SPI_RSER_TFFF_DIRS_MASK                  (0x1000000U)
#define SPI_RSER_TFFF_DIRS_SHIFT                 (24U)
#define SPI_RSER_TFFF_DIRS(x)                    (((uint32_t)(((uint32_t)(x)) << SPI_RSER_TFFF_DIRS_SHIFT)) & SPI_RSER_TFFF_DIRS_MASK)
#define SPI_RSER_TFFF_RE_MASK                    (0x2000000U)
#define SPI_RSER_TFFF_RE_SHIFT                   (25U)
#define SPI_RSER_TFFF_RE(x)                      (((uint32_t)(((uint32_t)(x)) << SPI_RSER_TFFF_RE_SHIFT)) & SPI_RSER_TFFF_RE_MASK)
#define SPI_RSER_TFUF_RE_MASK                    (0x8000000U)
#define SPI_RSER_TFUF_RE_SHIFT                   (27U)
#define SPI_RSER_TFUF_RE(x)                      (((uint32_t)(((uint32_t)(x)) << SPI_RSER_TFUF_RE_SHIFT)) & SPI_RSER_TFUF_RE_MASK)
#define SPI_RSER_EOQF_RE_MASK                    (0x10000000U)
#define SPI_RSER_EOQF_RE_SHIFT                   (28U)
#define SPI_RSER_EOQF_RE(x)                      (((uint32_t)(((uint32_t)(x)) << SPI_RSER_EOQF_RE_SHIFT)) & SPI_RSER_EOQF_RE_MASK)
#define SPI_RSER_TCF_RE_MASK                     (0x80000000U)
#define SPI_RSER_TCF_RE_SHIFT                    (31U)
#define SPI_RSER_TCF_RE(x)                       (((uint32_t)(((uint32_t)(x)) << SPI_RSER_TCF_RE_SHIFT)) & SPI_RSER_TCF_RE_MASK)

/*! @name PUSHR - PUSH TX FIFO Register In Master Mode */
#define SPI_PUSHR_TXDATA_MASK                    (0xFFFFU)
#define SPI_PUSHR_TXDATA_SHIFT                   (0U)
#define SPI_PUSHR_TXDATA(x)                      (((uint32_t)(((uint32_t)(x)) << SPI_PUSHR_TXDATA_SHIFT)) & SPI_PUSHR_TXDATA_MASK)
#define SPI_PUSHR_PCS_MASK                       (0xF0000U)
#define SPI_PUSHR_PCS_SHIFT                      (16U)
#define SPI_PUSHR_PCS(x)                         (((uint32_t)(((uint32_t)(x)) << SPI_PUSHR_PCS_SHIFT)) & SPI_PUSHR_PCS_MASK)
#define SPI_PUSHR_CTCNT_MASK                     (0x4000000U)
#define SPI_PUSHR_CTCNT_SHIFT                    (26U)
#define SPI_PUSHR_CTCNT(x)                       (((uint32_t)(((uint32_t)(x)) << SPI_PUSHR_CTCNT_SHIFT)) & SPI_PUSHR_CTCNT_MASK)
#define SPI_PUSHR_EOQ_MASK                       (0x8000000U)
#define SPI_PUSHR_EOQ_SHIFT                      (27U)
#define SPI_PUSHR_EOQ(x)                         (((uint32_t)(((uint32_t)(x)) << SPI_PUSHR_EOQ_SHIFT)) & SPI_PUSHR_EOQ_MASK)
#define SPI_PUSHR_CTAS_MASK                      (0x70000000U)
#define SPI_PUSHR_CTAS_SHIFT                     (28U)
#define SPI_PUSHR_CTAS(x)                        (((uint32_t)(((uint32_t)(x)) << SPI_PUSHR_CTAS_SHIFT)) & SPI_PUSHR_CTAS_MASK)
#define SPI_PUSHR_CONT_MASK                      (0x80000000U)
#define SPI_PUSHR_CONT_SHIFT                     (31U)
#define SPI_PUSHR_CONT(x)                        (((uint32_t)(((uint32_t)(x)) << SPI_PUSHR_CONT_SHIFT)) & SPI_PUSHR_CONT_MASK)

/*! @name PUSHR_SLAVE - PUSH TX FIFO Register In Slave Mode */
#define SPI_PUSHR_SLAVE_TXDATA_MASK              (0xFFFFU)
#define SPI_PUSHR_SLAVE_TXDATA_SHIFT             (0U)
#define SPI_PUSHR_SLAVE_TXDATA(x)                (((uint32_t)(((uint32_t)(x)) << SPI_PUSHR_SLAVE_TXDATA_SHIFT)) & SPI_PUSHR_SLAVE_TXDATA_MASK)

/*! @name POPR - POP RX FIFO Register */
#define SPI_POPR_RXDATA_MASK                     (0xFFFFFFFFU)
#define SPI_POPR_RXDATA_SHIFT                    (0U)
#define SPI_POPR_RXDATA(x)                       (((uint32_t)(((uint32_t)(x)) << SPI_POPR_RXDATA_SHIFT)) & SPI_POPR_RXDATA_MASK)

/*! @name TXFR0 - Transmit FIFO Registers */
#define SPI_TXFR0_TXDATA_MASK                    (0xFFFFU)
#define SPI_TXFR0_TXDATA_SHIFT                   (0U)
#define SPI_TXFR0_TXDATA(x)                      (((uint32_t)(((uint32_t)(x)) << SPI_TXFR0_TXDATA_SHIFT)) & SPI_TXFR0_TXDATA_MASK)
#define SPI_TXFR0_TXCMD_TXDATA_MASK              (0xFFFF0000U)
#define SPI_TXFR0_TXCMD_TXDATA_SHIFT             (16U)
#define SPI_TXFR0_TXCMD_TXDATA(x)                (((uint32_t)(((uint32_t)(x)) << SPI_TXFR0_TXCMD_TXDATA_SHIFT)) & SPI_TXFR0_TXCMD_TXDATA_MASK)

/*! @name TXFR1 - Transmit FIFO Registers */
#define SPI_TXFR1_TXDATA_MASK                    (0xFFFFU)
#define SPI_TXFR1_TXDATA_SHIFT                   (0U)
#define SPI_TXFR1_TXDATA(x)                      (((uint32_t)(((uint32_t)(x)) << SPI_TXFR1_TXDATA_SHIFT)) & SPI_TXFR1_TXDATA_MASK)
#define SPI_TXFR1_TXCMD_TXDATA_MASK              (0xFFFF0000U)
#define SPI_TXFR1_TXCMD_TXDATA_SHIFT             (16U)
#define SPI_TXFR1_TXCMD_TXDATA(x)                (((uint32_t)(((uint32_t)(x)) << SPI_TXFR1_TXCMD_TXDATA_SHIFT)) & SPI_TXFR1_TXCMD_TXDATA_MASK)

/*! @name TXFR2 - Transmit FIFO Registers */
#define SPI_TXFR2_TXDATA_MASK                    (0xFFFFU)
#define SPI_TXFR2_TXDATA_SHIFT                   (0U)
#define SPI_TXFR2_TXDATA(x)                      (((uint32_t)(((uint32_t)(x)) << SPI_TXFR2_TXDATA_SHIFT)) & SPI_TXFR2_TXDATA_MASK)
#define SPI_TXFR2_TXCMD_TXDATA_MASK              (0xFFFF0000U)
#define SPI_TXFR2_TXCMD_TXDATA_SHIFT             (16U)
#define SPI_TXFR2_TXCMD_TXDATA(x)                (((uint32_t)(((uint32_t)(x)) << SPI_TXFR2_TXCMD_TXDATA_SHIFT)) & SPI_TXFR2_TXCMD_TXDATA_MASK)

/*! @name TXFR3 - Transmit FIFO Registers */
#define SPI_TXFR3_TXDATA_MASK                    (0xFFFFU)
#define SPI_TXFR3_TXDATA_SHIFT                   (0U)
#define SPI_TXFR3_TXDATA(x)                      (((uint32_t)(((uint32_t)(x)) << SPI_TXFR3_TXDATA_SHIFT)) & SPI_TXFR3_TXDATA_MASK)
#define SPI_TXFR3_TXCMD_TXDATA_MASK              (0xFFFF0000U)
#define SPI_TXFR3_TXCMD_TXDATA_SHIFT             (16U)
#define SPI_TXFR3_TXCMD_TXDATA(x)                (((uint32_t)(((uint32_t)(x)) << SPI_TXFR3_TXCMD_TXDATA_SHIFT)) & SPI_TXFR3_TXCMD_TXDATA_MASK)

/*! @name RXFR0 - Receive FIFO Registers */
#define SPI_RXFR0_RXDATA_MASK                    (0xFFFFFFFFU)
#define SPI_RXFR0_RXDATA_SHIFT                   (0U)
#define SPI_RXFR0_RXDATA(x)                      (((uint32_t)(((uint32_t)(x)) << SPI_RXFR0_RXDATA_SHIFT)) & SPI_RXFR0_RXDATA_MASK)

/*! @name RXFR1 - Receive FIFO Registers */
#define SPI_RXFR1_RXDATA_MASK                    (0xFFFFFFFFU)
#define SPI_RXFR1_RXDATA_SHIFT                   (0U)
#define SPI_RXFR1_RXDATA(x)                      (((uint32_t)(((uint32_t)(x)) << SPI_RXFR1_RXDATA_SHIFT)) & SPI_RXFR1_RXDATA_MASK)

/*! @name RXFR2 - Receive FIFO Registers */
#define SPI_RXFR2_RXDATA_MASK                    (0xFFFFFFFFU)
#define SPI_RXFR2_RXDATA_SHIFT                   (0U)
#define SPI_RXFR2_RXDATA(x)                      (((uint32_t)(((uint32_t)(x)) << SPI_RXFR2_RXDATA_SHIFT)) & SPI_RXFR2_RXDATA_MASK)

/*! @name RXFR3 - Receive FIFO Registers */
#define SPI_RXFR3_RXDATA_MASK                    (0xFFFFFFFFU)
#define SPI_RXFR3_RXDATA_SHIFT                   (0U)
#define SPI_RXFR3_RXDATA(x)                      (((uint32_t)(((uint32_t)(x)) << SPI_RXFR3_RXDATA_SHIFT)) & SPI_RXFR3_RXDATA_MASK)


/*!
 * @}
 */ /* end of group SPI_Register_Masks */


/* SPI - Peripheral instance base addresses */
/** Peripheral SPI0 base address */
#define SPI0_BASE                                (0x4002C000u)
/** Peripheral SPI0 base pointer */
#define SPI0                                     ((SPI_Type *)SPI0_BASE)
/** Peripheral SPI1 base address */
#define SPI1_BASE                                (0x4002D000u)
/** Peripheral SPI1 base pointer */
#define SPI1                                     ((SPI_Type *)SPI1_BASE)
/** Array initializer of SPI peripheral base addresses */
#define SPI_BASE_ADDRS                           { SPI0_BASE, SPI1_BASE }
/** Array initializer of SPI peripheral base pointers */
#define SPI_BASE_PTRS                            { SPI0, SPI1 }
/** Interrupt vectors for the SPI peripheral type */
#define SPI_IRQS                                 { SPI0_IRQn, SPI1_IRQn }

/*!
 * @}
 */ /* end of group SPI_Peripheral_Access_Layer */


/* ----------------------------------------------------------------------------
   -- TPM Peripheral Access Layer
   ---------------------------------------------------------------------------- */

/*!
 * @addtogroup TPM_Peripheral_Access_Layer TPM Peripheral Access Layer
 * @{
 */

/** TPM - Register Layout Typedef */
typedef struct {
  __IO uint32_t SC;                                /**< Status and Control, offset: 0x0 */
  __IO uint32_t CNT;                               /**< Counter, offset: 0x4 */
  __IO uint32_t MOD;                               /**< Modulo, offset: 0x8 */
  struct {                                         /* offset: 0xC, array step: 0x8 */
    __IO uint32_t CnSC;                              /**< Channel (n) Status and Control, array offset: 0xC, array step: 0x8 */
    __IO uint32_t CnV;                               /**< Channel (n) Value, array offset: 0x10, array step: 0x8 */
  } CONTROLS[4];
       uint8_t RESERVED_0[36];
  __IO uint32_t STATUS;                            /**< Capture and Compare Status, offset: 0x50 */
       uint8_t RESERVED_1[16];
  __IO uint32_t COMBINE;                           /**< Combine Channel Register, offset: 0x64 */
       uint8_t RESERVED_2[8];
  __IO uint32_t POL;                               /**< Channel Polarity, offset: 0x70 */
       uint8_t RESERVED_3[4];
  __IO uint32_t FILTER;                            /**< Filter Control, offset: 0x78 */
       uint8_t RESERVED_4[4];
  __IO uint32_t QDCTRL;                            /**< Quadrature Decoder Control and Status, offset: 0x80 */
  __IO uint32_t CONF;                              /**< Configuration, offset: 0x84 */
} TPM_Type;

/* ----------------------------------------------------------------------------
   -- TPM Register Masks
   ---------------------------------------------------------------------------- */

/*!
 * @addtogroup TPM_Register_Masks TPM Register Masks
 * @{
 */

/*! @name SC - Status and Control */
#define TPM_SC_PS_MASK                           (0x7U)
#define TPM_SC_PS_SHIFT                          (0U)
#define TPM_SC_PS(x)                             (((uint32_t)(((uint32_t)(x)) << TPM_SC_PS_SHIFT)) & TPM_SC_PS_MASK)
#define TPM_SC_CMOD_MASK                         (0x18U)
#define TPM_SC_CMOD_SHIFT                        (3U)
#define TPM_SC_CMOD(x)                           (((uint32_t)(((uint32_t)(x)) << TPM_SC_CMOD_SHIFT)) & TPM_SC_CMOD_MASK)
#define TPM_SC_CPWMS_MASK                        (0x20U)
#define TPM_SC_CPWMS_SHIFT                       (5U)
#define TPM_SC_CPWMS(x)                          (((uint32_t)(((uint32_t)(x)) << TPM_SC_CPWMS_SHIFT)) & TPM_SC_CPWMS_MASK)
#define TPM_SC_TOIE_MASK                         (0x40U)
#define TPM_SC_TOIE_SHIFT                        (6U)
#define TPM_SC_TOIE(x)                           (((uint32_t)(((uint32_t)(x)) << TPM_SC_TOIE_SHIFT)) & TPM_SC_TOIE_MASK)
#define TPM_SC_TOF_MASK                          (0x80U)
#define TPM_SC_TOF_SHIFT                         (7U)
#define TPM_SC_TOF(x)                            (((uint32_t)(((uint32_t)(x)) << TPM_SC_TOF_SHIFT)) & TPM_SC_TOF_MASK)
#define TPM_SC_DMA_MASK                          (0x100U)
#define TPM_SC_DMA_SHIFT                         (8U)
#define TPM_SC_DMA(x)                            (((uint32_t)(((uint32_t)(x)) << TPM_SC_DMA_SHIFT)) & TPM_SC_DMA_MASK)

/*! @name CNT - Counter */
#define TPM_CNT_COUNT_MASK                       (0xFFFFU)
#define TPM_CNT_COUNT_SHIFT                      (0U)
#define TPM_CNT_COUNT(x)                         (((uint32_t)(((uint32_t)(x)) << TPM_CNT_COUNT_SHIFT)) & TPM_CNT_COUNT_MASK)

/*! @name MOD - Modulo */
#define TPM_MOD_MOD_MASK                         (0xFFFFU)
#define TPM_MOD_MOD_SHIFT                        (0U)
#define TPM_MOD_MOD(x)                           (((uint32_t)(((uint32_t)(x)) << TPM_MOD_MOD_SHIFT)) & TPM_MOD_MOD_MASK)

/*! @name CnSC - Channel (n) Status and Control */
#define TPM_CnSC_DMA_MASK                        (0x1U)
#define TPM_CnSC_DMA_SHIFT                       (0U)
#define TPM_CnSC_DMA(x)                          (((uint32_t)(((uint32_t)(x)) << TPM_CnSC_DMA_SHIFT)) & TPM_CnSC_DMA_MASK)
#define TPM_CnSC_ELSA_MASK                       (0x4U)
#define TPM_CnSC_ELSA_SHIFT                      (2U)
#define TPM_CnSC_ELSA(x)                         (((uint32_t)(((uint32_t)(x)) << TPM_CnSC_ELSA_SHIFT)) & TPM_CnSC_ELSA_MASK)
#define TPM_CnSC_ELSB_MASK                       (0x8U)
#define TPM_CnSC_ELSB_SHIFT                      (3U)
#define TPM_CnSC_ELSB(x)                         (((uint32_t)(((uint32_t)(x)) << TPM_CnSC_ELSB_SHIFT)) & TPM_CnSC_ELSB_MASK)
#define TPM_CnSC_MSA_MASK                        (0x10U)
#define TPM_CnSC_MSA_SHIFT                       (4U)
#define TPM_CnSC_MSA(x)                          (((uint32_t)(((uint32_t)(x)) << TPM_CnSC_MSA_SHIFT)) & TPM_CnSC_MSA_MASK)
#define TPM_CnSC_MSB_MASK                        (0x20U)
#define TPM_CnSC_MSB_SHIFT                       (5U)
#define TPM_CnSC_MSB(x)                          (((uint32_t)(((uint32_t)(x)) << TPM_CnSC_MSB_SHIFT)) & TPM_CnSC_MSB_MASK)
#define TPM_CnSC_CHIE_MASK                       (0x40U)
#define TPM_CnSC_CHIE_SHIFT                      (6U)
#define TPM_CnSC_CHIE(x)                         (((uint32_t)(((uint32_t)(x)) << TPM_CnSC_CHIE_SHIFT)) & TPM_CnSC_CHIE_MASK)
#define TPM_CnSC_CHF_MASK                        (0x80U)
#define TPM_CnSC_CHF_SHIFT                       (7U)
#define TPM_CnSC_CHF(x)                          (((uint32_t)(((uint32_t)(x)) << TPM_CnSC_CHF_SHIFT)) & TPM_CnSC_CHF_MASK)

/* The count of TPM_CnSC */
#define TPM_CnSC_COUNT                           (4U)

/*! @name CnV - Channel (n) Value */
#define TPM_CnV_VAL_MASK                         (0xFFFFU)
#define TPM_CnV_VAL_SHIFT                        (0U)
#define TPM_CnV_VAL(x)                           (((uint32_t)(((uint32_t)(x)) << TPM_CnV_VAL_SHIFT)) & TPM_CnV_VAL_MASK)

/* The count of TPM_CnV */
#define TPM_CnV_COUNT                            (4U)

/*! @name STATUS - Capture and Compare Status */
#define TPM_STATUS_CH0F_MASK                     (0x1U)
#define TPM_STATUS_CH0F_SHIFT                    (0U)
#define TPM_STATUS_CH0F(x)                       (((uint32_t)(((uint32_t)(x)) << TPM_STATUS_CH0F_SHIFT)) & TPM_STATUS_CH0F_MASK)
#define TPM_STATUS_CH1F_MASK                     (0x2U)
#define TPM_STATUS_CH1F_SHIFT                    (1U)
#define TPM_STATUS_CH1F(x)                       (((uint32_t)(((uint32_t)(x)) << TPM_STATUS_CH1F_SHIFT)) & TPM_STATUS_CH1F_MASK)
#define TPM_STATUS_CH2F_MASK                     (0x4U)
#define TPM_STATUS_CH2F_SHIFT                    (2U)
#define TPM_STATUS_CH2F(x)                       (((uint32_t)(((uint32_t)(x)) << TPM_STATUS_CH2F_SHIFT)) & TPM_STATUS_CH2F_MASK)
#define TPM_STATUS_CH3F_MASK                     (0x8U)
#define TPM_STATUS_CH3F_SHIFT                    (3U)
#define TPM_STATUS_CH3F(x)                       (((uint32_t)(((uint32_t)(x)) << TPM_STATUS_CH3F_SHIFT)) & TPM_STATUS_CH3F_MASK)
#define TPM_STATUS_TOF_MASK                      (0x100U)
#define TPM_STATUS_TOF_SHIFT                     (8U)
#define TPM_STATUS_TOF(x)                        (((uint32_t)(((uint32_t)(x)) << TPM_STATUS_TOF_SHIFT)) & TPM_STATUS_TOF_MASK)

/*! @name COMBINE - Combine Channel Register */
#define TPM_COMBINE_COMBINE0_MASK                (0x1U)
#define TPM_COMBINE_COMBINE0_SHIFT               (0U)
#define TPM_COMBINE_COMBINE0(x)                  (((uint32_t)(((uint32_t)(x)) << TPM_COMBINE_COMBINE0_SHIFT)) & TPM_COMBINE_COMBINE0_MASK)
#define TPM_COMBINE_COMSWAP0_MASK                (0x2U)
#define TPM_COMBINE_COMSWAP0_SHIFT               (1U)
#define TPM_COMBINE_COMSWAP0(x)                  (((uint32_t)(((uint32_t)(x)) << TPM_COMBINE_COMSWAP0_SHIFT)) & TPM_COMBINE_COMSWAP0_MASK)
#define TPM_COMBINE_COMBINE1_MASK                (0x100U)
#define TPM_COMBINE_COMBINE1_SHIFT               (8U)
#define TPM_COMBINE_COMBINE1(x)                  (((uint32_t)(((uint32_t)(x)) << TPM_COMBINE_COMBINE1_SHIFT)) & TPM_COMBINE_COMBINE1_MASK)
#define TPM_COMBINE_COMSWAP1_MASK                (0x200U)
#define TPM_COMBINE_COMSWAP1_SHIFT               (9U)
#define TPM_COMBINE_COMSWAP1(x)                  (((uint32_t)(((uint32_t)(x)) << TPM_COMBINE_COMSWAP1_SHIFT)) & TPM_COMBINE_COMSWAP1_MASK)

/*! @name POL - Channel Polarity */
#define TPM_POL_POL0_MASK                        (0x1U)
#define TPM_POL_POL0_SHIFT                       (0U)
#define TPM_POL_POL0(x)                          (((uint32_t)(((uint32_t)(x)) << TPM_POL_POL0_SHIFT)) & TPM_POL_POL0_MASK)
#define TPM_POL_POL1_MASK                        (0x2U)
#define TPM_POL_POL1_SHIFT                       (1U)
#define TPM_POL_POL1(x)                          (((uint32_t)(((uint32_t)(x)) << TPM_POL_POL1_SHIFT)) & TPM_POL_POL1_MASK)
#define TPM_POL_POL2_MASK                        (0x4U)
#define TPM_POL_POL2_SHIFT                       (2U)
#define TPM_POL_POL2(x)                          (((uint32_t)(((uint32_t)(x)) << TPM_POL_POL2_SHIFT)) & TPM_POL_POL2_MASK)
#define TPM_POL_POL3_MASK                        (0x8U)
#define TPM_POL_POL3_SHIFT                       (3U)
#define TPM_POL_POL3(x)                          (((uint32_t)(((uint32_t)(x)) << TPM_POL_POL3_SHIFT)) & TPM_POL_POL3_MASK)

/*! @name FILTER - Filter Control */
#define TPM_FILTER_CH0FVAL_MASK                  (0xFU)
#define TPM_FILTER_CH0FVAL_SHIFT                 (0U)
#define TPM_FILTER_CH0FVAL(x)                    (((uint32_t)(((uint32_t)(x)) << TPM_FILTER_CH0FVAL_SHIFT)) & TPM_FILTER_CH0FVAL_MASK)
#define TPM_FILTER_CH1FVAL_MASK                  (0xF0U)
#define TPM_FILTER_CH1FVAL_SHIFT                 (4U)
#define TPM_FILTER_CH1FVAL(x)                    (((uint32_t)(((uint32_t)(x)) << TPM_FILTER_CH1FVAL_SHIFT)) & TPM_FILTER_CH1FVAL_MASK)
#define TPM_FILTER_CH2FVAL_MASK                  (0xF00U)
#define TPM_FILTER_CH2FVAL_SHIFT                 (8U)
#define TPM_FILTER_CH2FVAL(x)                    (((uint32_t)(((uint32_t)(x)) << TPM_FILTER_CH2FVAL_SHIFT)) & TPM_FILTER_CH2FVAL_MASK)
#define TPM_FILTER_CH3FVAL_MASK                  (0xF000U)
#define TPM_FILTER_CH3FVAL_SHIFT                 (12U)
#define TPM_FILTER_CH3FVAL(x)                    (((uint32_t)(((uint32_t)(x)) << TPM_FILTER_CH3FVAL_SHIFT)) & TPM_FILTER_CH3FVAL_MASK)

/*! @name QDCTRL - Quadrature Decoder Control and Status */
#define TPM_QDCTRL_QUADEN_MASK                   (0x1U)
#define TPM_QDCTRL_QUADEN_SHIFT                  (0U)
#define TPM_QDCTRL_QUADEN(x)                     (((uint32_t)(((uint32_t)(x)) << TPM_QDCTRL_QUADEN_SHIFT)) & TPM_QDCTRL_QUADEN_MASK)
#define TPM_QDCTRL_TOFDIR_MASK                   (0x2U)
#define TPM_QDCTRL_TOFDIR_SHIFT                  (1U)
#define TPM_QDCTRL_TOFDIR(x)                     (((uint32_t)(((uint32_t)(x)) << TPM_QDCTRL_TOFDIR_SHIFT)) & TPM_QDCTRL_TOFDIR_MASK)
#define TPM_QDCTRL_QUADIR_MASK                   (0x4U)
#define TPM_QDCTRL_QUADIR_SHIFT                  (2U)
#define TPM_QDCTRL_QUADIR(x)                     (((uint32_t)(((uint32_t)(x)) << TPM_QDCTRL_QUADIR_SHIFT)) & TPM_QDCTRL_QUADIR_MASK)
#define TPM_QDCTRL_QUADMODE_MASK                 (0x8U)
#define TPM_QDCTRL_QUADMODE_SHIFT                (3U)
#define TPM_QDCTRL_QUADMODE(x)                   (((uint32_t)(((uint32_t)(x)) << TPM_QDCTRL_QUADMODE_SHIFT)) & TPM_QDCTRL_QUADMODE_MASK)

/*! @name CONF - Configuration */
#define TPM_CONF_DOZEEN_MASK                     (0x20U)
#define TPM_CONF_DOZEEN_SHIFT                    (5U)
#define TPM_CONF_DOZEEN(x)                       (((uint32_t)(((uint32_t)(x)) << TPM_CONF_DOZEEN_SHIFT)) & TPM_CONF_DOZEEN_MASK)
#define TPM_CONF_DBGMODE_MASK                    (0xC0U)
#define TPM_CONF_DBGMODE_SHIFT                   (6U)
#define TPM_CONF_DBGMODE(x)                      (((uint32_t)(((uint32_t)(x)) << TPM_CONF_DBGMODE_SHIFT)) & TPM_CONF_DBGMODE_MASK)
#define TPM_CONF_GTBSYNC_MASK                    (0x100U)
#define TPM_CONF_GTBSYNC_SHIFT                   (8U)
#define TPM_CONF_GTBSYNC(x)                      (((uint32_t)(((uint32_t)(x)) << TPM_CONF_GTBSYNC_SHIFT)) & TPM_CONF_GTBSYNC_MASK)
#define TPM_CONF_GTBEEN_MASK                     (0x200U)
#define TPM_CONF_GTBEEN_SHIFT                    (9U)
#define TPM_CONF_GTBEEN(x)                       (((uint32_t)(((uint32_t)(x)) << TPM_CONF_GTBEEN_SHIFT)) & TPM_CONF_GTBEEN_MASK)
#define TPM_CONF_CSOT_MASK                       (0x10000U)
#define TPM_CONF_CSOT_SHIFT                      (16U)
#define TPM_CONF_CSOT(x)                         (((uint32_t)(((uint32_t)(x)) << TPM_CONF_CSOT_SHIFT)) & TPM_CONF_CSOT_MASK)
#define TPM_CONF_CSOO_MASK                       (0x20000U)
#define TPM_CONF_CSOO_SHIFT                      (17U)
#define TPM_CONF_CSOO(x)                         (((uint32_t)(((uint32_t)(x)) << TPM_CONF_CSOO_SHIFT)) & TPM_CONF_CSOO_MASK)
#define TPM_CONF_CROT_MASK                       (0x40000U)
#define TPM_CONF_CROT_SHIFT                      (18U)
#define TPM_CONF_CROT(x)                         (((uint32_t)(((uint32_t)(x)) << TPM_CONF_CROT_SHIFT)) & TPM_CONF_CROT_MASK)
#define TPM_CONF_CPOT_MASK                       (0x80000U)
#define TPM_CONF_CPOT_SHIFT                      (19U)
#define TPM_CONF_CPOT(x)                         (((uint32_t)(((uint32_t)(x)) << TPM_CONF_CPOT_SHIFT)) & TPM_CONF_CPOT_MASK)
#define TPM_CONF_TRGPOL_MASK                     (0x400000U)
#define TPM_CONF_TRGPOL_SHIFT                    (22U)
#define TPM_CONF_TRGPOL(x)                       (((uint32_t)(((uint32_t)(x)) << TPM_CONF_TRGPOL_SHIFT)) & TPM_CONF_TRGPOL_MASK)
#define TPM_CONF_TRGSRC_MASK                     (0x800000U)
#define TPM_CONF_TRGSRC_SHIFT                    (23U)
#define TPM_CONF_TRGSRC(x)                       (((uint32_t)(((uint32_t)(x)) << TPM_CONF_TRGSRC_SHIFT)) & TPM_CONF_TRGSRC_MASK)
#define TPM_CONF_TRGSEL_MASK                     (0xF000000U)
#define TPM_CONF_TRGSEL_SHIFT                    (24U)
#define TPM_CONF_TRGSEL(x)                       (((uint32_t)(((uint32_t)(x)) << TPM_CONF_TRGSEL_SHIFT)) & TPM_CONF_TRGSEL_MASK)


/*!
 * @}
 */ /* end of group TPM_Register_Masks */


/* TPM - Peripheral instance base addresses */
/** Peripheral TPM0 base address */
#define TPM0_BASE                                (0x40038000u)
/** Peripheral TPM0 base pointer */
#define TPM0                                     ((TPM_Type *)TPM0_BASE)
/** Peripheral TPM1 base address */
#define TPM1_BASE                                (0x40039000u)
/** Peripheral TPM1 base pointer */
#define TPM1                                     ((TPM_Type *)TPM1_BASE)
/** Peripheral TPM2 base address */
#define TPM2_BASE                                (0x4003A000u)
/** Peripheral TPM2 base pointer */
#define TPM2                                     ((TPM_Type *)TPM2_BASE)
/** Array initializer of TPM peripheral base addresses */
#define TPM_BASE_ADDRS                           { TPM0_BASE, TPM1_BASE, TPM2_BASE }
/** Array initializer of TPM peripheral base pointers */
#define TPM_BASE_PTRS                            { TPM0, TPM1, TPM2 }
/** Interrupt vectors for the TPM peripheral type */
#define TPM_IRQS                                 { TPM0_IRQn, TPM1_IRQn, TPM2_IRQn }

/*!
 * @}
 */ /* end of group TPM_Peripheral_Access_Layer */


/* ----------------------------------------------------------------------------
   -- TRNG Peripheral Access Layer
   ---------------------------------------------------------------------------- */

/*!
 * @addtogroup TRNG_Peripheral_Access_Layer TRNG Peripheral Access Layer
 * @{
 */

/** TRNG - Register Layout Typedef */
typedef struct {
  __IO uint32_t MCTL;                              /**< Miscellaneous Control Register, offset: 0x0 */
  __IO uint32_t SCMISC;                            /**< Statistical Check Miscellaneous Register, offset: 0x4 */
  __IO uint32_t PKRRNG;                            /**< Poker Range Register, offset: 0x8 */
  union {                                          /* offset: 0xC */
    __IO uint32_t PKRMAX;                            /**< Poker Maximum Limit Register, offset: 0xC */
    __I  uint32_t PKRSQ;                             /**< Poker Square Calculation Result Register, offset: 0xC */
  };
  __IO uint32_t SDCTL;                             /**< Seed Control Register, offset: 0x10 */
  union {                                          /* offset: 0x14 */
    __IO uint32_t SBLIM;                             /**< Sparse Bit Limit Register, offset: 0x14 */
    __I  uint32_t TOTSAM;                            /**< Total Samples Register, offset: 0x14 */
  };
  __IO uint32_t FRQMIN;                            /**< Frequency Count Minimum Limit Register, offset: 0x18 */
  union {                                          /* offset: 0x1C */
    __I  uint32_t FRQCNT;                            /**< Frequency Count Register, offset: 0x1C */
    __IO uint32_t FRQMAX;                            /**< Frequency Count Maximum Limit Register, offset: 0x1C */
  };
  union {                                          /* offset: 0x20 */
    __I  uint32_t SCMC;                              /**< Statistical Check Monobit Count Register, offset: 0x20 */
    __IO uint32_t SCML;                              /**< Statistical Check Monobit Limit Register, offset: 0x20 */
  };
  union {                                          /* offset: 0x24 */
    __I  uint32_t SCR1C;                             /**< Statistical Check Run Length 1 Count Register, offset: 0x24 */
    __IO uint32_t SCR1L;                             /**< Statistical Check Run Length 1 Limit Register, offset: 0x24 */
  };
  union {                                          /* offset: 0x28 */
    __I  uint32_t SCR2C;                             /**< Statistical Check Run Length 2 Count Register, offset: 0x28 */
    __IO uint32_t SCR2L;                             /**< Statistical Check Run Length 2 Limit Register, offset: 0x28 */
  };
  union {                                          /* offset: 0x2C */
    __I  uint32_t SCR3C;                             /**< Statistical Check Run Length 3 Count Register, offset: 0x2C */
    __IO uint32_t SCR3L;                             /**< Statistical Check Run Length 3 Limit Register, offset: 0x2C */
  };
  union {                                          /* offset: 0x30 */
    __I  uint32_t SCR4C;                             /**< Statistical Check Run Length 4 Count Register, offset: 0x30 */
    __IO uint32_t SCR4L;                             /**< Statistical Check Run Length 4 Limit Register, offset: 0x30 */
  };
  union {                                          /* offset: 0x34 */
    __I  uint32_t SCR5C;                             /**< Statistical Check Run Length 5 Count Register, offset: 0x34 */
    __IO uint32_t SCR5L;                             /**< Statistical Check Run Length 5 Limit Register, offset: 0x34 */
  };
  union {                                          /* offset: 0x38 */
    __I  uint32_t SCR6PC;                            /**< Statistical Check Run Length 6+ Count Register, offset: 0x38 */
    __IO uint32_t SCR6PL;                            /**< Statistical Check Run Length 6+ Limit Register, offset: 0x38 */
  };
  __I  uint32_t STATUS;                            /**< Status Register, offset: 0x3C */
  __I  uint32_t ENT[16];                           /**< Entropy Read Register, array offset: 0x40, array step: 0x4 */
  __I  uint32_t PKRCNT10;                          /**< Statistical Check Poker Count 1 and 0 Register, offset: 0x80 */
  __I  uint32_t PKRCNT32;                          /**< Statistical Check Poker Count 3 and 2 Register, offset: 0x84 */
  __I  uint32_t PKRCNT54;                          /**< Statistical Check Poker Count 5 and 4 Register, offset: 0x88 */
  __I  uint32_t PKRCNT76;                          /**< Statistical Check Poker Count 7 and 6 Register, offset: 0x8C */
  __I  uint32_t PKRCNT98;                          /**< Statistical Check Poker Count 9 and 8 Register, offset: 0x90 */
  __I  uint32_t PKRCNTBA;                          /**< Statistical Check Poker Count B and A Register, offset: 0x94 */
  __I  uint32_t PKRCNTDC;                          /**< Statistical Check Poker Count D and C Register, offset: 0x98 */
  __I  uint32_t PKRCNTFE;                          /**< Statistical Check Poker Count F and E Register, offset: 0x9C */
       uint8_t RESERVED_0[16];
  __IO uint32_t SEC_CFG;                           /**< Security Configuration Register, offset: 0xB0 */
  __IO uint32_t INT_CTRL;                          /**< Interrupt Control Register, offset: 0xB4 */
  __IO uint32_t INT_MASK;                          /**< Mask Register, offset: 0xB8 */
  __IO uint32_t INT_STATUS;                        /**< Interrupt Status Register, offset: 0xBC */
       uint8_t RESERVED_1[48];
  __I  uint32_t VID1;                              /**< Version ID Register (MS), offset: 0xF0 */
  __I  uint32_t VID2;                              /**< Version ID Register (LS), offset: 0xF4 */
} TRNG_Type;

/* ----------------------------------------------------------------------------
   -- TRNG Register Masks
   ---------------------------------------------------------------------------- */

/*!
 * @addtogroup TRNG_Register_Masks TRNG Register Masks
 * @{
 */

/*! @name MCTL - Miscellaneous Control Register */
#define TRNG_MCTL_SAMP_MODE_MASK                 (0x3U)
#define TRNG_MCTL_SAMP_MODE_SHIFT                (0U)
#define TRNG_MCTL_SAMP_MODE(x)                   (((uint32_t)(((uint32_t)(x)) << TRNG_MCTL_SAMP_MODE_SHIFT)) & TRNG_MCTL_SAMP_MODE_MASK)
#define TRNG_MCTL_OSC_DIV_MASK                   (0xCU)
#define TRNG_MCTL_OSC_DIV_SHIFT                  (2U)
#define TRNG_MCTL_OSC_DIV(x)                     (((uint32_t)(((uint32_t)(x)) << TRNG_MCTL_OSC_DIV_SHIFT)) & TRNG_MCTL_OSC_DIV_MASK)
#define TRNG_MCTL_UNUSED_MASK                    (0x10U)
#define TRNG_MCTL_UNUSED_SHIFT                   (4U)
#define TRNG_MCTL_UNUSED(x)                      (((uint32_t)(((uint32_t)(x)) << TRNG_MCTL_UNUSED_SHIFT)) & TRNG_MCTL_UNUSED_MASK)
#define TRNG_MCTL_TRNG_ACC_MASK                  (0x20U)
#define TRNG_MCTL_TRNG_ACC_SHIFT                 (5U)
#define TRNG_MCTL_TRNG_ACC(x)                    (((uint32_t)(((uint32_t)(x)) << TRNG_MCTL_TRNG_ACC_SHIFT)) & TRNG_MCTL_TRNG_ACC_MASK)
#define TRNG_MCTL_RST_DEF_MASK                   (0x40U)
#define TRNG_MCTL_RST_DEF_SHIFT                  (6U)
#define TRNG_MCTL_RST_DEF(x)                     (((uint32_t)(((uint32_t)(x)) << TRNG_MCTL_RST_DEF_SHIFT)) & TRNG_MCTL_RST_DEF_MASK)
#define TRNG_MCTL_FOR_SCLK_MASK                  (0x80U)
#define TRNG_MCTL_FOR_SCLK_SHIFT                 (7U)
#define TRNG_MCTL_FOR_SCLK(x)                    (((uint32_t)(((uint32_t)(x)) << TRNG_MCTL_FOR_SCLK_SHIFT)) & TRNG_MCTL_FOR_SCLK_MASK)
#define TRNG_MCTL_FCT_FAIL_MASK                  (0x100U)
#define TRNG_MCTL_FCT_FAIL_SHIFT                 (8U)
#define TRNG_MCTL_FCT_FAIL(x)                    (((uint32_t)(((uint32_t)(x)) << TRNG_MCTL_FCT_FAIL_SHIFT)) & TRNG_MCTL_FCT_FAIL_MASK)
#define TRNG_MCTL_FCT_VAL_MASK                   (0x200U)
#define TRNG_MCTL_FCT_VAL_SHIFT                  (9U)
#define TRNG_MCTL_FCT_VAL(x)                     (((uint32_t)(((uint32_t)(x)) << TRNG_MCTL_FCT_VAL_SHIFT)) & TRNG_MCTL_FCT_VAL_MASK)
#define TRNG_MCTL_ENT_VAL_MASK                   (0x400U)
#define TRNG_MCTL_ENT_VAL_SHIFT                  (10U)
#define TRNG_MCTL_ENT_VAL(x)                     (((uint32_t)(((uint32_t)(x)) << TRNG_MCTL_ENT_VAL_SHIFT)) & TRNG_MCTL_ENT_VAL_MASK)
#define TRNG_MCTL_TST_OUT_MASK                   (0x800U)
#define TRNG_MCTL_TST_OUT_SHIFT                  (11U)
#define TRNG_MCTL_TST_OUT(x)                     (((uint32_t)(((uint32_t)(x)) << TRNG_MCTL_TST_OUT_SHIFT)) & TRNG_MCTL_TST_OUT_MASK)
#define TRNG_MCTL_ERR_MASK                       (0x1000U)
#define TRNG_MCTL_ERR_SHIFT                      (12U)
#define TRNG_MCTL_ERR(x)                         (((uint32_t)(((uint32_t)(x)) << TRNG_MCTL_ERR_SHIFT)) & TRNG_MCTL_ERR_MASK)
#define TRNG_MCTL_TSTOP_OK_MASK                  (0x2000U)
#define TRNG_MCTL_TSTOP_OK_SHIFT                 (13U)
#define TRNG_MCTL_TSTOP_OK(x)                    (((uint32_t)(((uint32_t)(x)) << TRNG_MCTL_TSTOP_OK_SHIFT)) & TRNG_MCTL_TSTOP_OK_MASK)
#define TRNG_MCTL_PRGM_MASK                      (0x10000U)
#define TRNG_MCTL_PRGM_SHIFT                     (16U)
#define TRNG_MCTL_PRGM(x)                        (((uint32_t)(((uint32_t)(x)) << TRNG_MCTL_PRGM_SHIFT)) & TRNG_MCTL_PRGM_MASK)

/*! @name SCMISC - Statistical Check Miscellaneous Register */
#define TRNG_SCMISC_LRUN_MAX_MASK                (0xFFU)
#define TRNG_SCMISC_LRUN_MAX_SHIFT               (0U)
#define TRNG_SCMISC_LRUN_MAX(x)                  (((uint32_t)(((uint32_t)(x)) << TRNG_SCMISC_LRUN_MAX_SHIFT)) & TRNG_SCMISC_LRUN_MAX_MASK)
#define TRNG_SCMISC_RTY_CT_MASK                  (0xF0000U)
#define TRNG_SCMISC_RTY_CT_SHIFT                 (16U)
#define TRNG_SCMISC_RTY_CT(x)                    (((uint32_t)(((uint32_t)(x)) << TRNG_SCMISC_RTY_CT_SHIFT)) & TRNG_SCMISC_RTY_CT_MASK)

/*! @name PKRRNG - Poker Range Register */
#define TRNG_PKRRNG_PKR_RNG_MASK                 (0xFFFFU)
#define TRNG_PKRRNG_PKR_RNG_SHIFT                (0U)
#define TRNG_PKRRNG_PKR_RNG(x)                   (((uint32_t)(((uint32_t)(x)) << TRNG_PKRRNG_PKR_RNG_SHIFT)) & TRNG_PKRRNG_PKR_RNG_MASK)

/*! @name PKRMAX - Poker Maximum Limit Register */
#define TRNG_PKRMAX_PKR_MAX_MASK                 (0xFFFFFFU)
#define TRNG_PKRMAX_PKR_MAX_SHIFT                (0U)
#define TRNG_PKRMAX_PKR_MAX(x)                   (((uint32_t)(((uint32_t)(x)) << TRNG_PKRMAX_PKR_MAX_SHIFT)) & TRNG_PKRMAX_PKR_MAX_MASK)

/*! @name PKRSQ - Poker Square Calculation Result Register */
#define TRNG_PKRSQ_PKR_SQ_MASK                   (0xFFFFFFU)
#define TRNG_PKRSQ_PKR_SQ_SHIFT                  (0U)
#define TRNG_PKRSQ_PKR_SQ(x)                     (((uint32_t)(((uint32_t)(x)) << TRNG_PKRSQ_PKR_SQ_SHIFT)) & TRNG_PKRSQ_PKR_SQ_MASK)

/*! @name SDCTL - Seed Control Register */
#define TRNG_SDCTL_SAMP_SIZE_MASK                (0xFFFFU)
#define TRNG_SDCTL_SAMP_SIZE_SHIFT               (0U)
#define TRNG_SDCTL_SAMP_SIZE(x)                  (((uint32_t)(((uint32_t)(x)) << TRNG_SDCTL_SAMP_SIZE_SHIFT)) & TRNG_SDCTL_SAMP_SIZE_MASK)
#define TRNG_SDCTL_ENT_DLY_MASK                  (0xFFFF0000U)
#define TRNG_SDCTL_ENT_DLY_SHIFT                 (16U)
#define TRNG_SDCTL_ENT_DLY(x)                    (((uint32_t)(((uint32_t)(x)) << TRNG_SDCTL_ENT_DLY_SHIFT)) & TRNG_SDCTL_ENT_DLY_MASK)

/*! @name SBLIM - Sparse Bit Limit Register */
#define TRNG_SBLIM_SB_LIM_MASK                   (0x3FFU)
#define TRNG_SBLIM_SB_LIM_SHIFT                  (0U)
#define TRNG_SBLIM_SB_LIM(x)                     (((uint32_t)(((uint32_t)(x)) << TRNG_SBLIM_SB_LIM_SHIFT)) & TRNG_SBLIM_SB_LIM_MASK)

/*! @name TOTSAM - Total Samples Register */
#define TRNG_TOTSAM_TOT_SAM_MASK                 (0xFFFFFU)
#define TRNG_TOTSAM_TOT_SAM_SHIFT                (0U)
#define TRNG_TOTSAM_TOT_SAM(x)                   (((uint32_t)(((uint32_t)(x)) << TRNG_TOTSAM_TOT_SAM_SHIFT)) & TRNG_TOTSAM_TOT_SAM_MASK)

/*! @name FRQMIN - Frequency Count Minimum Limit Register */
#define TRNG_FRQMIN_FRQ_MIN_MASK                 (0x3FFFFFU)
#define TRNG_FRQMIN_FRQ_MIN_SHIFT                (0U)
#define TRNG_FRQMIN_FRQ_MIN(x)                   (((uint32_t)(((uint32_t)(x)) << TRNG_FRQMIN_FRQ_MIN_SHIFT)) & TRNG_FRQMIN_FRQ_MIN_MASK)

/*! @name FRQCNT - Frequency Count Register */
#define TRNG_FRQCNT_FRQ_CT_MASK                  (0x3FFFFFU)
#define TRNG_FRQCNT_FRQ_CT_SHIFT                 (0U)
#define TRNG_FRQCNT_FRQ_CT(x)                    (((uint32_t)(((uint32_t)(x)) << TRNG_FRQCNT_FRQ_CT_SHIFT)) & TRNG_FRQCNT_FRQ_CT_MASK)

/*! @name FRQMAX - Frequency Count Maximum Limit Register */
#define TRNG_FRQMAX_FRQ_MAX_MASK                 (0x3FFFFFU)
#define TRNG_FRQMAX_FRQ_MAX_SHIFT                (0U)
#define TRNG_FRQMAX_FRQ_MAX(x)                   (((uint32_t)(((uint32_t)(x)) << TRNG_FRQMAX_FRQ_MAX_SHIFT)) & TRNG_FRQMAX_FRQ_MAX_MASK)

/*! @name SCMC - Statistical Check Monobit Count Register */
#define TRNG_SCMC_MONO_CT_MASK                   (0xFFFFU)
#define TRNG_SCMC_MONO_CT_SHIFT                  (0U)
#define TRNG_SCMC_MONO_CT(x)                     (((uint32_t)(((uint32_t)(x)) << TRNG_SCMC_MONO_CT_SHIFT)) & TRNG_SCMC_MONO_CT_MASK)

/*! @name SCML - Statistical Check Monobit Limit Register */
#define TRNG_SCML_MONO_MAX_MASK                  (0xFFFFU)
#define TRNG_SCML_MONO_MAX_SHIFT                 (0U)
#define TRNG_SCML_MONO_MAX(x)                    (((uint32_t)(((uint32_t)(x)) << TRNG_SCML_MONO_MAX_SHIFT)) & TRNG_SCML_MONO_MAX_MASK)
#define TRNG_SCML_MONO_RNG_MASK                  (0xFFFF0000U)
#define TRNG_SCML_MONO_RNG_SHIFT                 (16U)
#define TRNG_SCML_MONO_RNG(x)                    (((uint32_t)(((uint32_t)(x)) << TRNG_SCML_MONO_RNG_SHIFT)) & TRNG_SCML_MONO_RNG_MASK)

/*! @name SCR1C - Statistical Check Run Length 1 Count Register */
#define TRNG_SCR1C_R1_0_CT_MASK                  (0x7FFFU)
#define TRNG_SCR1C_R1_0_CT_SHIFT                 (0U)
#define TRNG_SCR1C_R1_0_CT(x)                    (((uint32_t)(((uint32_t)(x)) << TRNG_SCR1C_R1_0_CT_SHIFT)) & TRNG_SCR1C_R1_0_CT_MASK)
#define TRNG_SCR1C_R1_1_CT_MASK                  (0x7FFF0000U)
#define TRNG_SCR1C_R1_1_CT_SHIFT                 (16U)
#define TRNG_SCR1C_R1_1_CT(x)                    (((uint32_t)(((uint32_t)(x)) << TRNG_SCR1C_R1_1_CT_SHIFT)) & TRNG_SCR1C_R1_1_CT_MASK)

/*! @name SCR1L - Statistical Check Run Length 1 Limit Register */
#define TRNG_SCR1L_RUN1_MAX_MASK                 (0x7FFFU)
#define TRNG_SCR1L_RUN1_MAX_SHIFT                (0U)
#define TRNG_SCR1L_RUN1_MAX(x)                   (((uint32_t)(((uint32_t)(x)) << TRNG_SCR1L_RUN1_MAX_SHIFT)) & TRNG_SCR1L_RUN1_MAX_MASK)
#define TRNG_SCR1L_RUN1_RNG_MASK                 (0x7FFF0000U)
#define TRNG_SCR1L_RUN1_RNG_SHIFT                (16U)
#define TRNG_SCR1L_RUN1_RNG(x)                   (((uint32_t)(((uint32_t)(x)) << TRNG_SCR1L_RUN1_RNG_SHIFT)) & TRNG_SCR1L_RUN1_RNG_MASK)

/*! @name SCR2C - Statistical Check Run Length 2 Count Register */
#define TRNG_SCR2C_R2_0_CT_MASK                  (0x3FFFU)
#define TRNG_SCR2C_R2_0_CT_SHIFT                 (0U)
#define TRNG_SCR2C_R2_0_CT(x)                    (((uint32_t)(((uint32_t)(x)) << TRNG_SCR2C_R2_0_CT_SHIFT)) & TRNG_SCR2C_R2_0_CT_MASK)
#define TRNG_SCR2C_R2_1_CT_MASK                  (0x3FFF0000U)
#define TRNG_SCR2C_R2_1_CT_SHIFT                 (16U)
#define TRNG_SCR2C_R2_1_CT(x)                    (((uint32_t)(((uint32_t)(x)) << TRNG_SCR2C_R2_1_CT_SHIFT)) & TRNG_SCR2C_R2_1_CT_MASK)

/*! @name SCR2L - Statistical Check Run Length 2 Limit Register */
#define TRNG_SCR2L_RUN2_MAX_MASK                 (0x3FFFU)
#define TRNG_SCR2L_RUN2_MAX_SHIFT                (0U)
#define TRNG_SCR2L_RUN2_MAX(x)                   (((uint32_t)(((uint32_t)(x)) << TRNG_SCR2L_RUN2_MAX_SHIFT)) & TRNG_SCR2L_RUN2_MAX_MASK)
#define TRNG_SCR2L_RUN2_RNG_MASK                 (0x3FFF0000U)
#define TRNG_SCR2L_RUN2_RNG_SHIFT                (16U)
#define TRNG_SCR2L_RUN2_RNG(x)                   (((uint32_t)(((uint32_t)(x)) << TRNG_SCR2L_RUN2_RNG_SHIFT)) & TRNG_SCR2L_RUN2_RNG_MASK)

/*! @name SCR3C - Statistical Check Run Length 3 Count Register */
#define TRNG_SCR3C_R3_0_CT_MASK                  (0x1FFFU)
#define TRNG_SCR3C_R3_0_CT_SHIFT                 (0U)
#define TRNG_SCR3C_R3_0_CT(x)                    (((uint32_t)(((uint32_t)(x)) << TRNG_SCR3C_R3_0_CT_SHIFT)) & TRNG_SCR3C_R3_0_CT_MASK)
#define TRNG_SCR3C_R3_1_CT_MASK                  (0x1FFF0000U)
#define TRNG_SCR3C_R3_1_CT_SHIFT                 (16U)
#define TRNG_SCR3C_R3_1_CT(x)                    (((uint32_t)(((uint32_t)(x)) << TRNG_SCR3C_R3_1_CT_SHIFT)) & TRNG_SCR3C_R3_1_CT_MASK)

/*! @name SCR3L - Statistical Check Run Length 3 Limit Register */
#define TRNG_SCR3L_RUN3_MAX_MASK                 (0x1FFFU)
#define TRNG_SCR3L_RUN3_MAX_SHIFT                (0U)
#define TRNG_SCR3L_RUN3_MAX(x)                   (((uint32_t)(((uint32_t)(x)) << TRNG_SCR3L_RUN3_MAX_SHIFT)) & TRNG_SCR3L_RUN3_MAX_MASK)
#define TRNG_SCR3L_RUN3_RNG_MASK                 (0x1FFF0000U)
#define TRNG_SCR3L_RUN3_RNG_SHIFT                (16U)
#define TRNG_SCR3L_RUN3_RNG(x)                   (((uint32_t)(((uint32_t)(x)) << TRNG_SCR3L_RUN3_RNG_SHIFT)) & TRNG_SCR3L_RUN3_RNG_MASK)

/*! @name SCR4C - Statistical Check Run Length 4 Count Register */
#define TRNG_SCR4C_R4_0_CT_MASK                  (0xFFFU)
#define TRNG_SCR4C_R4_0_CT_SHIFT                 (0U)
#define TRNG_SCR4C_R4_0_CT(x)                    (((uint32_t)(((uint32_t)(x)) << TRNG_SCR4C_R4_0_CT_SHIFT)) & TRNG_SCR4C_R4_0_CT_MASK)
#define TRNG_SCR4C_R4_1_CT_MASK                  (0xFFF0000U)
#define TRNG_SCR4C_R4_1_CT_SHIFT                 (16U)
#define TRNG_SCR4C_R4_1_CT(x)                    (((uint32_t)(((uint32_t)(x)) << TRNG_SCR4C_R4_1_CT_SHIFT)) & TRNG_SCR4C_R4_1_CT_MASK)

/*! @name SCR4L - Statistical Check Run Length 4 Limit Register */
#define TRNG_SCR4L_RUN4_MAX_MASK                 (0xFFFU)
#define TRNG_SCR4L_RUN4_MAX_SHIFT                (0U)
#define TRNG_SCR4L_RUN4_MAX(x)                   (((uint32_t)(((uint32_t)(x)) << TRNG_SCR4L_RUN4_MAX_SHIFT)) & TRNG_SCR4L_RUN4_MAX_MASK)
#define TRNG_SCR4L_RUN4_RNG_MASK                 (0xFFF0000U)
#define TRNG_SCR4L_RUN4_RNG_SHIFT                (16U)
#define TRNG_SCR4L_RUN4_RNG(x)                   (((uint32_t)(((uint32_t)(x)) << TRNG_SCR4L_RUN4_RNG_SHIFT)) & TRNG_SCR4L_RUN4_RNG_MASK)

/*! @name SCR5C - Statistical Check Run Length 5 Count Register */
#define TRNG_SCR5C_R5_0_CT_MASK                  (0x7FFU)
#define TRNG_SCR5C_R5_0_CT_SHIFT                 (0U)
#define TRNG_SCR5C_R5_0_CT(x)                    (((uint32_t)(((uint32_t)(x)) << TRNG_SCR5C_R5_0_CT_SHIFT)) & TRNG_SCR5C_R5_0_CT_MASK)
#define TRNG_SCR5C_R5_1_CT_MASK                  (0x7FF0000U)
#define TRNG_SCR5C_R5_1_CT_SHIFT                 (16U)
#define TRNG_SCR5C_R5_1_CT(x)                    (((uint32_t)(((uint32_t)(x)) << TRNG_SCR5C_R5_1_CT_SHIFT)) & TRNG_SCR5C_R5_1_CT_MASK)

/*! @name SCR5L - Statistical Check Run Length 5 Limit Register */
#define TRNG_SCR5L_RUN5_MAX_MASK                 (0x7FFU)
#define TRNG_SCR5L_RUN5_MAX_SHIFT                (0U)
#define TRNG_SCR5L_RUN5_MAX(x)                   (((uint32_t)(((uint32_t)(x)) << TRNG_SCR5L_RUN5_MAX_SHIFT)) & TRNG_SCR5L_RUN5_MAX_MASK)
#define TRNG_SCR5L_RUN5_RNG_MASK                 (0x7FF0000U)
#define TRNG_SCR5L_RUN5_RNG_SHIFT                (16U)
#define TRNG_SCR5L_RUN5_RNG(x)                   (((uint32_t)(((uint32_t)(x)) << TRNG_SCR5L_RUN5_RNG_SHIFT)) & TRNG_SCR5L_RUN5_RNG_MASK)

/*! @name SCR6PC - Statistical Check Run Length 6+ Count Register */
#define TRNG_SCR6PC_R6P_0_CT_MASK                (0x7FFU)
#define TRNG_SCR6PC_R6P_0_CT_SHIFT               (0U)
#define TRNG_SCR6PC_R6P_0_CT(x)                  (((uint32_t)(((uint32_t)(x)) << TRNG_SCR6PC_R6P_0_CT_SHIFT)) & TRNG_SCR6PC_R6P_0_CT_MASK)
#define TRNG_SCR6PC_R6P_1_CT_MASK                (0x7FF0000U)
#define TRNG_SCR6PC_R6P_1_CT_SHIFT               (16U)
#define TRNG_SCR6PC_R6P_1_CT(x)                  (((uint32_t)(((uint32_t)(x)) << TRNG_SCR6PC_R6P_1_CT_SHIFT)) & TRNG_SCR6PC_R6P_1_CT_MASK)

/*! @name SCR6PL - Statistical Check Run Length 6+ Limit Register */
#define TRNG_SCR6PL_RUN6P_MAX_MASK               (0x7FFU)
#define TRNG_SCR6PL_RUN6P_MAX_SHIFT              (0U)
#define TRNG_SCR6PL_RUN6P_MAX(x)                 (((uint32_t)(((uint32_t)(x)) << TRNG_SCR6PL_RUN6P_MAX_SHIFT)) & TRNG_SCR6PL_RUN6P_MAX_MASK)
#define TRNG_SCR6PL_RUN6P_RNG_MASK               (0x7FF0000U)
#define TRNG_SCR6PL_RUN6P_RNG_SHIFT              (16U)
#define TRNG_SCR6PL_RUN6P_RNG(x)                 (((uint32_t)(((uint32_t)(x)) << TRNG_SCR6PL_RUN6P_RNG_SHIFT)) & TRNG_SCR6PL_RUN6P_RNG_MASK)

/*! @name STATUS - Status Register */
#define TRNG_STATUS_TF1BR0_MASK                  (0x1U)
#define TRNG_STATUS_TF1BR0_SHIFT                 (0U)
#define TRNG_STATUS_TF1BR0(x)                    (((uint32_t)(((uint32_t)(x)) << TRNG_STATUS_TF1BR0_SHIFT)) & TRNG_STATUS_TF1BR0_MASK)
#define TRNG_STATUS_TF1BR1_MASK                  (0x2U)
#define TRNG_STATUS_TF1BR1_SHIFT                 (1U)
#define TRNG_STATUS_TF1BR1(x)                    (((uint32_t)(((uint32_t)(x)) << TRNG_STATUS_TF1BR1_SHIFT)) & TRNG_STATUS_TF1BR1_MASK)
#define TRNG_STATUS_TF2BR0_MASK                  (0x4U)
#define TRNG_STATUS_TF2BR0_SHIFT                 (2U)
#define TRNG_STATUS_TF2BR0(x)                    (((uint32_t)(((uint32_t)(x)) << TRNG_STATUS_TF2BR0_SHIFT)) & TRNG_STATUS_TF2BR0_MASK)
#define TRNG_STATUS_TF2BR1_MASK                  (0x8U)
#define TRNG_STATUS_TF2BR1_SHIFT                 (3U)
#define TRNG_STATUS_TF2BR1(x)                    (((uint32_t)(((uint32_t)(x)) << TRNG_STATUS_TF2BR1_SHIFT)) & TRNG_STATUS_TF2BR1_MASK)
#define TRNG_STATUS_TF3BR0_MASK                  (0x10U)
#define TRNG_STATUS_TF3BR0_SHIFT                 (4U)
#define TRNG_STATUS_TF3BR0(x)                    (((uint32_t)(((uint32_t)(x)) << TRNG_STATUS_TF3BR0_SHIFT)) & TRNG_STATUS_TF3BR0_MASK)
#define TRNG_STATUS_TF3BR1_MASK                  (0x20U)
#define TRNG_STATUS_TF3BR1_SHIFT                 (5U)
#define TRNG_STATUS_TF3BR1(x)                    (((uint32_t)(((uint32_t)(x)) << TRNG_STATUS_TF3BR1_SHIFT)) & TRNG_STATUS_TF3BR1_MASK)
#define TRNG_STATUS_TF4BR0_MASK                  (0x40U)
#define TRNG_STATUS_TF4BR0_SHIFT                 (6U)
#define TRNG_STATUS_TF4BR0(x)                    (((uint32_t)(((uint32_t)(x)) << TRNG_STATUS_TF4BR0_SHIFT)) & TRNG_STATUS_TF4BR0_MASK)
#define TRNG_STATUS_TF4BR1_MASK                  (0x80U)
#define TRNG_STATUS_TF4BR1_SHIFT                 (7U)
#define TRNG_STATUS_TF4BR1(x)                    (((uint32_t)(((uint32_t)(x)) << TRNG_STATUS_TF4BR1_SHIFT)) & TRNG_STATUS_TF4BR1_MASK)
#define TRNG_STATUS_TF5BR0_MASK                  (0x100U)
#define TRNG_STATUS_TF5BR0_SHIFT                 (8U)
#define TRNG_STATUS_TF5BR0(x)                    (((uint32_t)(((uint32_t)(x)) << TRNG_STATUS_TF5BR0_SHIFT)) & TRNG_STATUS_TF5BR0_MASK)
#define TRNG_STATUS_TF5BR1_MASK                  (0x200U)
#define TRNG_STATUS_TF5BR1_SHIFT                 (9U)
#define TRNG_STATUS_TF5BR1(x)                    (((uint32_t)(((uint32_t)(x)) << TRNG_STATUS_TF5BR1_SHIFT)) & TRNG_STATUS_TF5BR1_MASK)
#define TRNG_STATUS_TF6PBR0_MASK                 (0x400U)
#define TRNG_STATUS_TF6PBR0_SHIFT                (10U)
#define TRNG_STATUS_TF6PBR0(x)                   (((uint32_t)(((uint32_t)(x)) << TRNG_STATUS_TF6PBR0_SHIFT)) & TRNG_STATUS_TF6PBR0_MASK)
#define TRNG_STATUS_TF6PBR1_MASK                 (0x800U)
#define TRNG_STATUS_TF6PBR1_SHIFT                (11U)
#define TRNG_STATUS_TF6PBR1(x)                   (((uint32_t)(((uint32_t)(x)) << TRNG_STATUS_TF6PBR1_SHIFT)) & TRNG_STATUS_TF6PBR1_MASK)
#define TRNG_STATUS_TFSB_MASK                    (0x1000U)
#define TRNG_STATUS_TFSB_SHIFT                   (12U)
#define TRNG_STATUS_TFSB(x)                      (((uint32_t)(((uint32_t)(x)) << TRNG_STATUS_TFSB_SHIFT)) & TRNG_STATUS_TFSB_MASK)
#define TRNG_STATUS_TFLR_MASK                    (0x2000U)
#define TRNG_STATUS_TFLR_SHIFT                   (13U)
#define TRNG_STATUS_TFLR(x)                      (((uint32_t)(((uint32_t)(x)) << TRNG_STATUS_TFLR_SHIFT)) & TRNG_STATUS_TFLR_MASK)
#define TRNG_STATUS_TFP_MASK                     (0x4000U)
#define TRNG_STATUS_TFP_SHIFT                    (14U)
#define TRNG_STATUS_TFP(x)                       (((uint32_t)(((uint32_t)(x)) << TRNG_STATUS_TFP_SHIFT)) & TRNG_STATUS_TFP_MASK)
#define TRNG_STATUS_TFMB_MASK                    (0x8000U)
#define TRNG_STATUS_TFMB_SHIFT                   (15U)
#define TRNG_STATUS_TFMB(x)                      (((uint32_t)(((uint32_t)(x)) << TRNG_STATUS_TFMB_SHIFT)) & TRNG_STATUS_TFMB_MASK)
#define TRNG_STATUS_RETRY_CT_MASK                (0xF0000U)
#define TRNG_STATUS_RETRY_CT_SHIFT               (16U)
#define TRNG_STATUS_RETRY_CT(x)                  (((uint32_t)(((uint32_t)(x)) << TRNG_STATUS_RETRY_CT_SHIFT)) & TRNG_STATUS_RETRY_CT_MASK)

/*! @name ENT - Entropy Read Register */
#define TRNG_ENT_ENT_MASK                        (0xFFFFFFFFU)
#define TRNG_ENT_ENT_SHIFT                       (0U)
#define TRNG_ENT_ENT(x)                          (((uint32_t)(((uint32_t)(x)) << TRNG_ENT_ENT_SHIFT)) & TRNG_ENT_ENT_MASK)

/* The count of TRNG_ENT */
#define TRNG_ENT_COUNT                           (16U)

/*! @name PKRCNT10 - Statistical Check Poker Count 1 and 0 Register */
#define TRNG_PKRCNT10_PKR_0_CT_MASK              (0xFFFFU)
#define TRNG_PKRCNT10_PKR_0_CT_SHIFT             (0U)
#define TRNG_PKRCNT10_PKR_0_CT(x)                (((uint32_t)(((uint32_t)(x)) << TRNG_PKRCNT10_PKR_0_CT_SHIFT)) & TRNG_PKRCNT10_PKR_0_CT_MASK)
#define TRNG_PKRCNT10_PKR_1_CT_MASK              (0xFFFF0000U)
#define TRNG_PKRCNT10_PKR_1_CT_SHIFT             (16U)
#define TRNG_PKRCNT10_PKR_1_CT(x)                (((uint32_t)(((uint32_t)(x)) << TRNG_PKRCNT10_PKR_1_CT_SHIFT)) & TRNG_PKRCNT10_PKR_1_CT_MASK)

/*! @name PKRCNT32 - Statistical Check Poker Count 3 and 2 Register */
#define TRNG_PKRCNT32_PKR_2_CT_MASK              (0xFFFFU)
#define TRNG_PKRCNT32_PKR_2_CT_SHIFT             (0U)
#define TRNG_PKRCNT32_PKR_2_CT(x)                (((uint32_t)(((uint32_t)(x)) << TRNG_PKRCNT32_PKR_2_CT_SHIFT)) & TRNG_PKRCNT32_PKR_2_CT_MASK)
#define TRNG_PKRCNT32_PKR_3_CT_MASK              (0xFFFF0000U)
#define TRNG_PKRCNT32_PKR_3_CT_SHIFT             (16U)
#define TRNG_PKRCNT32_PKR_3_CT(x)                (((uint32_t)(((uint32_t)(x)) << TRNG_PKRCNT32_PKR_3_CT_SHIFT)) & TRNG_PKRCNT32_PKR_3_CT_MASK)

/*! @name PKRCNT54 - Statistical Check Poker Count 5 and 4 Register */
#define TRNG_PKRCNT54_PKR_4_CT_MASK              (0xFFFFU)
#define TRNG_PKRCNT54_PKR_4_CT_SHIFT             (0U)
#define TRNG_PKRCNT54_PKR_4_CT(x)                (((uint32_t)(((uint32_t)(x)) << TRNG_PKRCNT54_PKR_4_CT_SHIFT)) & TRNG_PKRCNT54_PKR_4_CT_MASK)
#define TRNG_PKRCNT54_PKR_5_CT_MASK              (0xFFFF0000U)
#define TRNG_PKRCNT54_PKR_5_CT_SHIFT             (16U)
#define TRNG_PKRCNT54_PKR_5_CT(x)                (((uint32_t)(((uint32_t)(x)) << TRNG_PKRCNT54_PKR_5_CT_SHIFT)) & TRNG_PKRCNT54_PKR_5_CT_MASK)

/*! @name PKRCNT76 - Statistical Check Poker Count 7 and 6 Register */
#define TRNG_PKRCNT76_PKR_6_CT_MASK              (0xFFFFU)
#define TRNG_PKRCNT76_PKR_6_CT_SHIFT             (0U)
#define TRNG_PKRCNT76_PKR_6_CT(x)                (((uint32_t)(((uint32_t)(x)) << TRNG_PKRCNT76_PKR_6_CT_SHIFT)) & TRNG_PKRCNT76_PKR_6_CT_MASK)
#define TRNG_PKRCNT76_PKR_7_CT_MASK              (0xFFFF0000U)
#define TRNG_PKRCNT76_PKR_7_CT_SHIFT             (16U)
#define TRNG_PKRCNT76_PKR_7_CT(x)                (((uint32_t)(((uint32_t)(x)) << TRNG_PKRCNT76_PKR_7_CT_SHIFT)) & TRNG_PKRCNT76_PKR_7_CT_MASK)

/*! @name PKRCNT98 - Statistical Check Poker Count 9 and 8 Register */
#define TRNG_PKRCNT98_PKR_8_CT_MASK              (0xFFFFU)
#define TRNG_PKRCNT98_PKR_8_CT_SHIFT             (0U)
#define TRNG_PKRCNT98_PKR_8_CT(x)                (((uint32_t)(((uint32_t)(x)) << TRNG_PKRCNT98_PKR_8_CT_SHIFT)) & TRNG_PKRCNT98_PKR_8_CT_MASK)
#define TRNG_PKRCNT98_PKR_9_CT_MASK              (0xFFFF0000U)
#define TRNG_PKRCNT98_PKR_9_CT_SHIFT             (16U)
#define TRNG_PKRCNT98_PKR_9_CT(x)                (((uint32_t)(((uint32_t)(x)) << TRNG_PKRCNT98_PKR_9_CT_SHIFT)) & TRNG_PKRCNT98_PKR_9_CT_MASK)

/*! @name PKRCNTBA - Statistical Check Poker Count B and A Register */
#define TRNG_PKRCNTBA_PKR_A_CT_MASK              (0xFFFFU)
#define TRNG_PKRCNTBA_PKR_A_CT_SHIFT             (0U)
#define TRNG_PKRCNTBA_PKR_A_CT(x)                (((uint32_t)(((uint32_t)(x)) << TRNG_PKRCNTBA_PKR_A_CT_SHIFT)) & TRNG_PKRCNTBA_PKR_A_CT_MASK)
#define TRNG_PKRCNTBA_PKR_B_CT_MASK              (0xFFFF0000U)
#define TRNG_PKRCNTBA_PKR_B_CT_SHIFT             (16U)
#define TRNG_PKRCNTBA_PKR_B_CT(x)                (((uint32_t)(((uint32_t)(x)) << TRNG_PKRCNTBA_PKR_B_CT_SHIFT)) & TRNG_PKRCNTBA_PKR_B_CT_MASK)

/*! @name PKRCNTDC - Statistical Check Poker Count D and C Register */
#define TRNG_PKRCNTDC_PKR_C_CT_MASK              (0xFFFFU)
#define TRNG_PKRCNTDC_PKR_C_CT_SHIFT             (0U)
#define TRNG_PKRCNTDC_PKR_C_CT(x)                (((uint32_t)(((uint32_t)(x)) << TRNG_PKRCNTDC_PKR_C_CT_SHIFT)) & TRNG_PKRCNTDC_PKR_C_CT_MASK)
#define TRNG_PKRCNTDC_PKR_D_CT_MASK              (0xFFFF0000U)
#define TRNG_PKRCNTDC_PKR_D_CT_SHIFT             (16U)
#define TRNG_PKRCNTDC_PKR_D_CT(x)                (((uint32_t)(((uint32_t)(x)) << TRNG_PKRCNTDC_PKR_D_CT_SHIFT)) & TRNG_PKRCNTDC_PKR_D_CT_MASK)

/*! @name PKRCNTFE - Statistical Check Poker Count F and E Register */
#define TRNG_PKRCNTFE_PKR_E_CT_MASK              (0xFFFFU)
#define TRNG_PKRCNTFE_PKR_E_CT_SHIFT             (0U)
#define TRNG_PKRCNTFE_PKR_E_CT(x)                (((uint32_t)(((uint32_t)(x)) << TRNG_PKRCNTFE_PKR_E_CT_SHIFT)) & TRNG_PKRCNTFE_PKR_E_CT_MASK)
#define TRNG_PKRCNTFE_PKR_F_CT_MASK              (0xFFFF0000U)
#define TRNG_PKRCNTFE_PKR_F_CT_SHIFT             (16U)
#define TRNG_PKRCNTFE_PKR_F_CT(x)                (((uint32_t)(((uint32_t)(x)) << TRNG_PKRCNTFE_PKR_F_CT_SHIFT)) & TRNG_PKRCNTFE_PKR_F_CT_MASK)

/*! @name SEC_CFG - Security Configuration Register */
#define TRNG_SEC_CFG_SH0_MASK                    (0x1U)
#define TRNG_SEC_CFG_SH0_SHIFT                   (0U)
#define TRNG_SEC_CFG_SH0(x)                      (((uint32_t)(((uint32_t)(x)) << TRNG_SEC_CFG_SH0_SHIFT)) & TRNG_SEC_CFG_SH0_MASK)
#define TRNG_SEC_CFG_NO_PRGM_MASK                (0x2U)
#define TRNG_SEC_CFG_NO_PRGM_SHIFT               (1U)
#define TRNG_SEC_CFG_NO_PRGM(x)                  (((uint32_t)(((uint32_t)(x)) << TRNG_SEC_CFG_NO_PRGM_SHIFT)) & TRNG_SEC_CFG_NO_PRGM_MASK)
#define TRNG_SEC_CFG_SK_VAL_MASK                 (0x4U)
#define TRNG_SEC_CFG_SK_VAL_SHIFT                (2U)
#define TRNG_SEC_CFG_SK_VAL(x)                   (((uint32_t)(((uint32_t)(x)) << TRNG_SEC_CFG_SK_VAL_SHIFT)) & TRNG_SEC_CFG_SK_VAL_MASK)

/*! @name INT_CTRL - Interrupt Control Register */
#define TRNG_INT_CTRL_HW_ERR_MASK                (0x1U)
#define TRNG_INT_CTRL_HW_ERR_SHIFT               (0U)
#define TRNG_INT_CTRL_HW_ERR(x)                  (((uint32_t)(((uint32_t)(x)) << TRNG_INT_CTRL_HW_ERR_SHIFT)) & TRNG_INT_CTRL_HW_ERR_MASK)
#define TRNG_INT_CTRL_ENT_VAL_MASK               (0x2U)
#define TRNG_INT_CTRL_ENT_VAL_SHIFT              (1U)
#define TRNG_INT_CTRL_ENT_VAL(x)                 (((uint32_t)(((uint32_t)(x)) << TRNG_INT_CTRL_ENT_VAL_SHIFT)) & TRNG_INT_CTRL_ENT_VAL_MASK)
#define TRNG_INT_CTRL_FRQ_CT_FAIL_MASK           (0x4U)
#define TRNG_INT_CTRL_FRQ_CT_FAIL_SHIFT          (2U)
#define TRNG_INT_CTRL_FRQ_CT_FAIL(x)             (((uint32_t)(((uint32_t)(x)) << TRNG_INT_CTRL_FRQ_CT_FAIL_SHIFT)) & TRNG_INT_CTRL_FRQ_CT_FAIL_MASK)
#define TRNG_INT_CTRL_UNUSED_MASK                (0xFFFFFFF8U)
#define TRNG_INT_CTRL_UNUSED_SHIFT               (3U)
#define TRNG_INT_CTRL_UNUSED(x)                  (((uint32_t)(((uint32_t)(x)) << TRNG_INT_CTRL_UNUSED_SHIFT)) & TRNG_INT_CTRL_UNUSED_MASK)

/*! @name INT_MASK - Mask Register */
#define TRNG_INT_MASK_HW_ERR_MASK                (0x1U)
#define TRNG_INT_MASK_HW_ERR_SHIFT               (0U)
#define TRNG_INT_MASK_HW_ERR(x)                  (((uint32_t)(((uint32_t)(x)) << TRNG_INT_MASK_HW_ERR_SHIFT)) & TRNG_INT_MASK_HW_ERR_MASK)
#define TRNG_INT_MASK_ENT_VAL_MASK               (0x2U)
#define TRNG_INT_MASK_ENT_VAL_SHIFT              (1U)
#define TRNG_INT_MASK_ENT_VAL(x)                 (((uint32_t)(((uint32_t)(x)) << TRNG_INT_MASK_ENT_VAL_SHIFT)) & TRNG_INT_MASK_ENT_VAL_MASK)
#define TRNG_INT_MASK_FRQ_CT_FAIL_MASK           (0x4U)
#define TRNG_INT_MASK_FRQ_CT_FAIL_SHIFT          (2U)
#define TRNG_INT_MASK_FRQ_CT_FAIL(x)             (((uint32_t)(((uint32_t)(x)) << TRNG_INT_MASK_FRQ_CT_FAIL_SHIFT)) & TRNG_INT_MASK_FRQ_CT_FAIL_MASK)

/*! @name INT_STATUS - Interrupt Status Register */
#define TRNG_INT_STATUS_HW_ERR_MASK              (0x1U)
#define TRNG_INT_STATUS_HW_ERR_SHIFT             (0U)
#define TRNG_INT_STATUS_HW_ERR(x)                (((uint32_t)(((uint32_t)(x)) << TRNG_INT_STATUS_HW_ERR_SHIFT)) & TRNG_INT_STATUS_HW_ERR_MASK)
#define TRNG_INT_STATUS_ENT_VAL_MASK             (0x2U)
#define TRNG_INT_STATUS_ENT_VAL_SHIFT            (1U)
#define TRNG_INT_STATUS_ENT_VAL(x)               (((uint32_t)(((uint32_t)(x)) << TRNG_INT_STATUS_ENT_VAL_SHIFT)) & TRNG_INT_STATUS_ENT_VAL_MASK)
#define TRNG_INT_STATUS_FRQ_CT_FAIL_MASK         (0x4U)
#define TRNG_INT_STATUS_FRQ_CT_FAIL_SHIFT        (2U)
#define TRNG_INT_STATUS_FRQ_CT_FAIL(x)           (((uint32_t)(((uint32_t)(x)) << TRNG_INT_STATUS_FRQ_CT_FAIL_SHIFT)) & TRNG_INT_STATUS_FRQ_CT_FAIL_MASK)

/*! @name VID1 - Version ID Register (MS) */
#define TRNG_VID1_MIN_REV_MASK                   (0xFFU)
#define TRNG_VID1_MIN_REV_SHIFT                  (0U)
#define TRNG_VID1_MIN_REV(x)                     (((uint32_t)(((uint32_t)(x)) << TRNG_VID1_MIN_REV_SHIFT)) & TRNG_VID1_MIN_REV_MASK)
#define TRNG_VID1_MAJ_REV_MASK                   (0xFF00U)
#define TRNG_VID1_MAJ_REV_SHIFT                  (8U)
#define TRNG_VID1_MAJ_REV(x)                     (((uint32_t)(((uint32_t)(x)) << TRNG_VID1_MAJ_REV_SHIFT)) & TRNG_VID1_MAJ_REV_MASK)
#define TRNG_VID1_IP_ID_MASK                     (0xFFFF0000U)
#define TRNG_VID1_IP_ID_SHIFT                    (16U)
#define TRNG_VID1_IP_ID(x)                       (((uint32_t)(((uint32_t)(x)) << TRNG_VID1_IP_ID_SHIFT)) & TRNG_VID1_IP_ID_MASK)

/*! @name VID2 - Version ID Register (LS) */
#define TRNG_VID2_CONFIG_OPT_MASK                (0xFFU)
#define TRNG_VID2_CONFIG_OPT_SHIFT               (0U)
#define TRNG_VID2_CONFIG_OPT(x)                  (((uint32_t)(((uint32_t)(x)) << TRNG_VID2_CONFIG_OPT_SHIFT)) & TRNG_VID2_CONFIG_OPT_MASK)
#define TRNG_VID2_ECO_REV_MASK                   (0xFF00U)
#define TRNG_VID2_ECO_REV_SHIFT                  (8U)
#define TRNG_VID2_ECO_REV(x)                     (((uint32_t)(((uint32_t)(x)) << TRNG_VID2_ECO_REV_SHIFT)) & TRNG_VID2_ECO_REV_MASK)
#define TRNG_VID2_INTG_OPT_MASK                  (0xFF0000U)
#define TRNG_VID2_INTG_OPT_SHIFT                 (16U)
#define TRNG_VID2_INTG_OPT(x)                    (((uint32_t)(((uint32_t)(x)) << TRNG_VID2_INTG_OPT_SHIFT)) & TRNG_VID2_INTG_OPT_MASK)
#define TRNG_VID2_ERA_MASK                       (0xFF000000U)
#define TRNG_VID2_ERA_SHIFT                      (24U)
#define TRNG_VID2_ERA(x)                         (((uint32_t)(((uint32_t)(x)) << TRNG_VID2_ERA_SHIFT)) & TRNG_VID2_ERA_MASK)


/*!
 * @}
 */ /* end of group TRNG_Register_Masks */


/* TRNG - Peripheral instance base addresses */
/** Peripheral TRNG0 base address */
#define TRNG0_BASE                               (0x40029000u)
/** Peripheral TRNG0 base pointer */
#define TRNG0                                    ((TRNG_Type *)TRNG0_BASE)
/** Array initializer of TRNG peripheral base addresses */
#define TRNG_BASE_ADDRS                          { TRNG0_BASE }
/** Array initializer of TRNG peripheral base pointers */
#define TRNG_BASE_PTRS                           { TRNG0 }
/** Interrupt vectors for the TRNG peripheral type */
#define TRNG_IRQS                                { TRNG0_IRQn }

/*!
 * @}
 */ /* end of group TRNG_Peripheral_Access_Layer */


/* ----------------------------------------------------------------------------
   -- TSI Peripheral Access Layer
   ---------------------------------------------------------------------------- */

/*!
 * @addtogroup TSI_Peripheral_Access_Layer TSI Peripheral Access Layer
 * @{
 */

/** TSI - Register Layout Typedef */
typedef struct {
  __IO uint32_t GENCS;                             /**< TSI General Control and Status Register, offset: 0x0 */
  __IO uint32_t DATA;                              /**< TSI DATA Register, offset: 0x4 */
  __IO uint32_t TSHD;                              /**< TSI Threshold Register, offset: 0x8 */
} TSI_Type;

/* ----------------------------------------------------------------------------
   -- TSI Register Masks
   ---------------------------------------------------------------------------- */

/*!
 * @addtogroup TSI_Register_Masks TSI Register Masks
 * @{
 */

/*! @name GENCS - TSI General Control and Status Register */
#define TSI_GENCS_CURSW_MASK                     (0x2U)
#define TSI_GENCS_CURSW_SHIFT                    (1U)
#define TSI_GENCS_CURSW(x)                       (((uint32_t)(((uint32_t)(x)) << TSI_GENCS_CURSW_SHIFT)) & TSI_GENCS_CURSW_MASK)
#define TSI_GENCS_EOSF_MASK                      (0x4U)
#define TSI_GENCS_EOSF_SHIFT                     (2U)
#define TSI_GENCS_EOSF(x)                        (((uint32_t)(((uint32_t)(x)) << TSI_GENCS_EOSF_SHIFT)) & TSI_GENCS_EOSF_MASK)
#define TSI_GENCS_SCNIP_MASK                     (0x8U)
#define TSI_GENCS_SCNIP_SHIFT                    (3U)
#define TSI_GENCS_SCNIP(x)                       (((uint32_t)(((uint32_t)(x)) << TSI_GENCS_SCNIP_SHIFT)) & TSI_GENCS_SCNIP_MASK)
#define TSI_GENCS_STM_MASK                       (0x10U)
#define TSI_GENCS_STM_SHIFT                      (4U)
#define TSI_GENCS_STM(x)                         (((uint32_t)(((uint32_t)(x)) << TSI_GENCS_STM_SHIFT)) & TSI_GENCS_STM_MASK)
#define TSI_GENCS_STPE_MASK                      (0x20U)
#define TSI_GENCS_STPE_SHIFT                     (5U)
#define TSI_GENCS_STPE(x)                        (((uint32_t)(((uint32_t)(x)) << TSI_GENCS_STPE_SHIFT)) & TSI_GENCS_STPE_MASK)
#define TSI_GENCS_TSIIEN_MASK                    (0x40U)
#define TSI_GENCS_TSIIEN_SHIFT                   (6U)
#define TSI_GENCS_TSIIEN(x)                      (((uint32_t)(((uint32_t)(x)) << TSI_GENCS_TSIIEN_SHIFT)) & TSI_GENCS_TSIIEN_MASK)
#define TSI_GENCS_TSIEN_MASK                     (0x80U)
#define TSI_GENCS_TSIEN_SHIFT                    (7U)
#define TSI_GENCS_TSIEN(x)                       (((uint32_t)(((uint32_t)(x)) << TSI_GENCS_TSIEN_SHIFT)) & TSI_GENCS_TSIEN_MASK)
#define TSI_GENCS_NSCN_MASK                      (0x1F00U)
#define TSI_GENCS_NSCN_SHIFT                     (8U)
#define TSI_GENCS_NSCN(x)                        (((uint32_t)(((uint32_t)(x)) << TSI_GENCS_NSCN_SHIFT)) & TSI_GENCS_NSCN_MASK)
#define TSI_GENCS_PS_MASK                        (0xE000U)
#define TSI_GENCS_PS_SHIFT                       (13U)
#define TSI_GENCS_PS(x)                          (((uint32_t)(((uint32_t)(x)) << TSI_GENCS_PS_SHIFT)) & TSI_GENCS_PS_MASK)
#define TSI_GENCS_EXTCHRG_MASK                   (0x70000U)
#define TSI_GENCS_EXTCHRG_SHIFT                  (16U)
#define TSI_GENCS_EXTCHRG(x)                     (((uint32_t)(((uint32_t)(x)) << TSI_GENCS_EXTCHRG_SHIFT)) & TSI_GENCS_EXTCHRG_MASK)
#define TSI_GENCS_DVOLT_MASK                     (0x180000U)
#define TSI_GENCS_DVOLT_SHIFT                    (19U)
#define TSI_GENCS_DVOLT(x)                       (((uint32_t)(((uint32_t)(x)) << TSI_GENCS_DVOLT_SHIFT)) & TSI_GENCS_DVOLT_MASK)
#define TSI_GENCS_REFCHRG_MASK                   (0xE00000U)
#define TSI_GENCS_REFCHRG_SHIFT                  (21U)
#define TSI_GENCS_REFCHRG(x)                     (((uint32_t)(((uint32_t)(x)) << TSI_GENCS_REFCHRG_SHIFT)) & TSI_GENCS_REFCHRG_MASK)
#define TSI_GENCS_MODE_MASK                      (0xF000000U)
#define TSI_GENCS_MODE_SHIFT                     (24U)
#define TSI_GENCS_MODE(x)                        (((uint32_t)(((uint32_t)(x)) << TSI_GENCS_MODE_SHIFT)) & TSI_GENCS_MODE_MASK)
#define TSI_GENCS_ESOR_MASK                      (0x10000000U)
#define TSI_GENCS_ESOR_SHIFT                     (28U)
#define TSI_GENCS_ESOR(x)                        (((uint32_t)(((uint32_t)(x)) << TSI_GENCS_ESOR_SHIFT)) & TSI_GENCS_ESOR_MASK)
#define TSI_GENCS_OUTRGF_MASK                    (0x80000000U)
#define TSI_GENCS_OUTRGF_SHIFT                   (31U)
#define TSI_GENCS_OUTRGF(x)                      (((uint32_t)(((uint32_t)(x)) << TSI_GENCS_OUTRGF_SHIFT)) & TSI_GENCS_OUTRGF_MASK)

/*! @name DATA - TSI DATA Register */
#define TSI_DATA_TSICNT_MASK                     (0xFFFFU)
#define TSI_DATA_TSICNT_SHIFT                    (0U)
#define TSI_DATA_TSICNT(x)                       (((uint32_t)(((uint32_t)(x)) << TSI_DATA_TSICNT_SHIFT)) & TSI_DATA_TSICNT_MASK)
#define TSI_DATA_SWTS_MASK                       (0x400000U)
#define TSI_DATA_SWTS_SHIFT                      (22U)
#define TSI_DATA_SWTS(x)                         (((uint32_t)(((uint32_t)(x)) << TSI_DATA_SWTS_SHIFT)) & TSI_DATA_SWTS_MASK)
#define TSI_DATA_DMAEN_MASK                      (0x800000U)
#define TSI_DATA_DMAEN_SHIFT                     (23U)
#define TSI_DATA_DMAEN(x)                        (((uint32_t)(((uint32_t)(x)) << TSI_DATA_DMAEN_SHIFT)) & TSI_DATA_DMAEN_MASK)
#define TSI_DATA_TSICH_MASK                      (0xF0000000U)
#define TSI_DATA_TSICH_SHIFT                     (28U)
#define TSI_DATA_TSICH(x)                        (((uint32_t)(((uint32_t)(x)) << TSI_DATA_TSICH_SHIFT)) & TSI_DATA_TSICH_MASK)

/*! @name TSHD - TSI Threshold Register */
#define TSI_TSHD_THRESL_MASK                     (0xFFFFU)
#define TSI_TSHD_THRESL_SHIFT                    (0U)
#define TSI_TSHD_THRESL(x)                       (((uint32_t)(((uint32_t)(x)) << TSI_TSHD_THRESL_SHIFT)) & TSI_TSHD_THRESL_MASK)
#define TSI_TSHD_THRESH_MASK                     (0xFFFF0000U)
#define TSI_TSHD_THRESH_SHIFT                    (16U)
#define TSI_TSHD_THRESH(x)                       (((uint32_t)(((uint32_t)(x)) << TSI_TSHD_THRESH_SHIFT)) & TSI_TSHD_THRESH_MASK)


/*!
 * @}
 */ /* end of group TSI_Register_Masks */


/* TSI - Peripheral instance base addresses */
/** Peripheral TSI0 base address */
#define TSI0_BASE                                (0x40045000u)
/** Peripheral TSI0 base pointer */
#define TSI0                                     ((TSI_Type *)TSI0_BASE)
/** Array initializer of TSI peripheral base addresses */
#define TSI_BASE_ADDRS                           { TSI0_BASE }
/** Array initializer of TSI peripheral base pointers */
#define TSI_BASE_PTRS                            { TSI0 }
/** Interrupt vectors for the TSI peripheral type */
#define TSI_IRQS                                 { TSI0_IRQn }

/*!
 * @}
 */ /* end of group TSI_Peripheral_Access_Layer */


/* ----------------------------------------------------------------------------
   -- VREF Peripheral Access Layer
   ---------------------------------------------------------------------------- */

/*!
 * @addtogroup VREF_Peripheral_Access_Layer VREF Peripheral Access Layer
 * @{
 */

/** VREF - Register Layout Typedef */
typedef struct {
  __IO uint8_t TRM;                                /**< VREF Trim Register, offset: 0x0 */
  __IO uint8_t SC;                                 /**< VREF Status and Control Register, offset: 0x1 */
} VREF_Type;

/* ----------------------------------------------------------------------------
   -- VREF Register Masks
   ---------------------------------------------------------------------------- */

/*!
 * @addtogroup VREF_Register_Masks VREF Register Masks
 * @{
 */

/*! @name TRM - VREF Trim Register */
#define VREF_TRM_TRIM_MASK                       (0x3FU)
#define VREF_TRM_TRIM_SHIFT                      (0U)
#define VREF_TRM_TRIM(x)                         (((uint8_t)(((uint8_t)(x)) << VREF_TRM_TRIM_SHIFT)) & VREF_TRM_TRIM_MASK)
#define VREF_TRM_CHOPEN_MASK                     (0x40U)
#define VREF_TRM_CHOPEN_SHIFT                    (6U)
#define VREF_TRM_CHOPEN(x)                       (((uint8_t)(((uint8_t)(x)) << VREF_TRM_CHOPEN_SHIFT)) & VREF_TRM_CHOPEN_MASK)

/*! @name SC - VREF Status and Control Register */
#define VREF_SC_MODE_LV_MASK                     (0x3U)
#define VREF_SC_MODE_LV_SHIFT                    (0U)
#define VREF_SC_MODE_LV(x)                       (((uint8_t)(((uint8_t)(x)) << VREF_SC_MODE_LV_SHIFT)) & VREF_SC_MODE_LV_MASK)
#define VREF_SC_VREFST_MASK                      (0x4U)
#define VREF_SC_VREFST_SHIFT                     (2U)
#define VREF_SC_VREFST(x)                        (((uint8_t)(((uint8_t)(x)) << VREF_SC_VREFST_SHIFT)) & VREF_SC_VREFST_MASK)
#define VREF_SC_ICOMPEN_MASK                     (0x20U)
#define VREF_SC_ICOMPEN_SHIFT                    (5U)
#define VREF_SC_ICOMPEN(x)                       (((uint8_t)(((uint8_t)(x)) << VREF_SC_ICOMPEN_SHIFT)) & VREF_SC_ICOMPEN_MASK)
#define VREF_SC_REGEN_MASK                       (0x40U)
#define VREF_SC_REGEN_SHIFT                      (6U)
#define VREF_SC_REGEN(x)                         (((uint8_t)(((uint8_t)(x)) << VREF_SC_REGEN_SHIFT)) & VREF_SC_REGEN_MASK)
#define VREF_SC_VREFEN_MASK                      (0x80U)
#define VREF_SC_VREFEN_SHIFT                     (7U)
#define VREF_SC_VREFEN(x)                        (((uint8_t)(((uint8_t)(x)) << VREF_SC_VREFEN_SHIFT)) & VREF_SC_VREFEN_MASK)


/*!
 * @}
 */ /* end of group VREF_Register_Masks */


/* VREF - Peripheral instance base addresses */
/** Peripheral VREF base address */
#define VREF_BASE                                (0x40074000u)
/** Peripheral VREF base pointer */
#define VREF                                     ((VREF_Type *)VREF_BASE)
/** Array initializer of VREF peripheral base addresses */
#define VREF_BASE_ADDRS                          { VREF_BASE }
/** Array initializer of VREF peripheral base pointers */
#define VREF_BASE_PTRS                           { VREF }

/*!
 * @}
 */ /* end of group VREF_Peripheral_Access_Layer */


/* ----------------------------------------------------------------------------
   -- XCVR_ANALOG Peripheral Access Layer
   ---------------------------------------------------------------------------- */

/*!
 * @addtogroup XCVR_ANALOG_Peripheral_Access_Layer XCVR_ANALOG Peripheral Access Layer
 * @{
 */

/** XCVR_ANALOG - Register Layout Typedef */
typedef struct {
  __IO uint32_t BB_LDO_1;                          /**< RF Analog Baseband LDO Control 1, offset: 0x0 */
  __IO uint32_t BB_LDO_2;                          /**< RF Analog Baseband LDO Control 2, offset: 0x4 */
  __IO uint32_t RX_ADC;                            /**< RF Analog ADC Control, offset: 0x8 */
  __IO uint32_t RX_BBA;                            /**< RF Analog BBA Control, offset: 0xC */
  __IO uint32_t RX_LNA;                            /**< RF Analog LNA Control, offset: 0x10 */
  __IO uint32_t RX_TZA;                            /**< RF Analog TZA Control, offset: 0x14 */
  __IO uint32_t RX_AUXPLL;                         /**< RF Analog Aux PLL Control, offset: 0x18 */
  __IO uint32_t SY_CTRL_1;                         /**< RF Analog Synthesizer Control 1, offset: 0x1C */
  __IO uint32_t SY_CTRL_2;                         /**< RF Analog Synthesizer Control 2, offset: 0x20 */
  __IO uint32_t TX_DAC_PA;                         /**< RF Analog TX HPM DAC and PA Control, offset: 0x24 */
  __IO uint32_t BALUN_TX;                          /**< RF Analog Balun TX Mode Control, offset: 0x28 */
  __IO uint32_t BALUN_RX;                          /**< RF Analog Balun RX Mode Control, offset: 0x2C */
  __I  uint32_t DFT_OBSV_1;                        /**< RF Analog DFT Observation Register 1, offset: 0x30 */
  __IO uint32_t DFT_OBSV_2;                        /**< RF Analog DFT Observation Register 2, offset: 0x34 */
} XCVR_ANALOG_Type;

/* ----------------------------------------------------------------------------
   -- XCVR_ANALOG Register Masks
   ---------------------------------------------------------------------------- */

/*!
 * @addtogroup XCVR_ANALOG_Register_Masks XCVR_ANALOG Register Masks
 * @{
 */

/*! @name BB_LDO_1 - RF Analog Baseband LDO Control 1 */
#define XCVR_ANALOG_BB_LDO_1_BB_LDO_ADCDAC_BYP_MASK (0x1U)
#define XCVR_ANALOG_BB_LDO_1_BB_LDO_ADCDAC_BYP_SHIFT (0U)
#define XCVR_ANALOG_BB_LDO_1_BB_LDO_ADCDAC_BYP(x) (((uint32_t)(((uint32_t)(x)) << XCVR_ANALOG_BB_LDO_1_BB_LDO_ADCDAC_BYP_SHIFT)) & XCVR_ANALOG_BB_LDO_1_BB_LDO_ADCDAC_BYP_MASK)
#define XCVR_ANALOG_BB_LDO_1_BB_LDO_ADCDAC_DIAGSEL_MASK (0x2U)
#define XCVR_ANALOG_BB_LDO_1_BB_LDO_ADCDAC_DIAGSEL_SHIFT (1U)
#define XCVR_ANALOG_BB_LDO_1_BB_LDO_ADCDAC_DIAGSEL(x) (((uint32_t)(((uint32_t)(x)) << XCVR_ANALOG_BB_LDO_1_BB_LDO_ADCDAC_DIAGSEL_SHIFT)) & XCVR_ANALOG_BB_LDO_1_BB_LDO_ADCDAC_DIAGSEL_MASK)
#define XCVR_ANALOG_BB_LDO_1_BB_LDO_ADCDAC_SPARE_MASK (0xCU)
#define XCVR_ANALOG_BB_LDO_1_BB_LDO_ADCDAC_SPARE_SHIFT (2U)
#define XCVR_ANALOG_BB_LDO_1_BB_LDO_ADCDAC_SPARE(x) (((uint32_t)(((uint32_t)(x)) << XCVR_ANALOG_BB_LDO_1_BB_LDO_ADCDAC_SPARE_SHIFT)) & XCVR_ANALOG_BB_LDO_1_BB_LDO_ADCDAC_SPARE_MASK)
#define XCVR_ANALOG_BB_LDO_1_BB_LDO_ADCDAC_TRIM_MASK (0x70U)
#define XCVR_ANALOG_BB_LDO_1_BB_LDO_ADCDAC_TRIM_SHIFT (4U)
#define XCVR_ANALOG_BB_LDO_1_BB_LDO_ADCDAC_TRIM(x) (((uint32_t)(((uint32_t)(x)) << XCVR_ANALOG_BB_LDO_1_BB_LDO_ADCDAC_TRIM_SHIFT)) & XCVR_ANALOG_BB_LDO_1_BB_LDO_ADCDAC_TRIM_MASK)
#define XCVR_ANALOG_BB_LDO_1_BB_LDO_BBA_BYP_MASK (0x100U)
#define XCVR_ANALOG_BB_LDO_1_BB_LDO_BBA_BYP_SHIFT (8U)
#define XCVR_ANALOG_BB_LDO_1_BB_LDO_BBA_BYP(x)   (((uint32_t)(((uint32_t)(x)) << XCVR_ANALOG_BB_LDO_1_BB_LDO_BBA_BYP_SHIFT)) & XCVR_ANALOG_BB_LDO_1_BB_LDO_BBA_BYP_MASK)
#define XCVR_ANALOG_BB_LDO_1_BB_LDO_BBA_DIAGSEL_MASK (0x200U)
#define XCVR_ANALOG_BB_LDO_1_BB_LDO_BBA_DIAGSEL_SHIFT (9U)
#define XCVR_ANALOG_BB_LDO_1_BB_LDO_BBA_DIAGSEL(x) (((uint32_t)(((uint32_t)(x)) << XCVR_ANALOG_BB_LDO_1_BB_LDO_BBA_DIAGSEL_SHIFT)) & XCVR_ANALOG_BB_LDO_1_BB_LDO_BBA_DIAGSEL_MASK)
#define XCVR_ANALOG_BB_LDO_1_BB_LDO_BBA_SPARE_MASK (0xC00U)
#define XCVR_ANALOG_BB_LDO_1_BB_LDO_BBA_SPARE_SHIFT (10U)
#define XCVR_ANALOG_BB_LDO_1_BB_LDO_BBA_SPARE(x) (((uint32_t)(((uint32_t)(x)) << XCVR_ANALOG_BB_LDO_1_BB_LDO_BBA_SPARE_SHIFT)) & XCVR_ANALOG_BB_LDO_1_BB_LDO_BBA_SPARE_MASK)
#define XCVR_ANALOG_BB_LDO_1_BB_LDO_BBA_TRIM_MASK (0x7000U)
#define XCVR_ANALOG_BB_LDO_1_BB_LDO_BBA_TRIM_SHIFT (12U)
#define XCVR_ANALOG_BB_LDO_1_BB_LDO_BBA_TRIM(x)  (((uint32_t)(((uint32_t)(x)) << XCVR_ANALOG_BB_LDO_1_BB_LDO_BBA_TRIM_SHIFT)) & XCVR_ANALOG_BB_LDO_1_BB_LDO_BBA_TRIM_MASK)
#define XCVR_ANALOG_BB_LDO_1_BB_LDO_FDBK_BYP_MASK (0x10000U)
#define XCVR_ANALOG_BB_LDO_1_BB_LDO_FDBK_BYP_SHIFT (16U)
#define XCVR_ANALOG_BB_LDO_1_BB_LDO_FDBK_BYP(x)  (((uint32_t)(((uint32_t)(x)) << XCVR_ANALOG_BB_LDO_1_BB_LDO_FDBK_BYP_SHIFT)) & XCVR_ANALOG_BB_LDO_1_BB_LDO_FDBK_BYP_MASK)
#define XCVR_ANALOG_BB_LDO_1_BB_LDO_FDBK_DIAGSEL_MASK (0x20000U)
#define XCVR_ANALOG_BB_LDO_1_BB_LDO_FDBK_DIAGSEL_SHIFT (17U)
#define XCVR_ANALOG_BB_LDO_1_BB_LDO_FDBK_DIAGSEL(x) (((uint32_t)(((uint32_t)(x)) << XCVR_ANALOG_BB_LDO_1_BB_LDO_FDBK_DIAGSEL_SHIFT)) & XCVR_ANALOG_BB_LDO_1_BB_LDO_FDBK_DIAGSEL_MASK)
#define XCVR_ANALOG_BB_LDO_1_BB_LDO_FDBK_SPARE_MASK (0xC0000U)
#define XCVR_ANALOG_BB_LDO_1_BB_LDO_FDBK_SPARE_SHIFT (18U)
#define XCVR_ANALOG_BB_LDO_1_BB_LDO_FDBK_SPARE(x) (((uint32_t)(((uint32_t)(x)) << XCVR_ANALOG_BB_LDO_1_BB_LDO_FDBK_SPARE_SHIFT)) & XCVR_ANALOG_BB_LDO_1_BB_LDO_FDBK_SPARE_MASK)
#define XCVR_ANALOG_BB_LDO_1_BB_LDO_FDBK_TRIM_MASK (0x700000U)
#define XCVR_ANALOG_BB_LDO_1_BB_LDO_FDBK_TRIM_SHIFT (20U)
#define XCVR_ANALOG_BB_LDO_1_BB_LDO_FDBK_TRIM(x) (((uint32_t)(((uint32_t)(x)) << XCVR_ANALOG_BB_LDO_1_BB_LDO_FDBK_TRIM_SHIFT)) & XCVR_ANALOG_BB_LDO_1_BB_LDO_FDBK_TRIM_MASK)
#define XCVR_ANALOG_BB_LDO_1_BB_LDO_HF_BYP_MASK  (0x1000000U)
#define XCVR_ANALOG_BB_LDO_1_BB_LDO_HF_BYP_SHIFT (24U)
#define XCVR_ANALOG_BB_LDO_1_BB_LDO_HF_BYP(x)    (((uint32_t)(((uint32_t)(x)) << XCVR_ANALOG_BB_LDO_1_BB_LDO_HF_BYP_SHIFT)) & XCVR_ANALOG_BB_LDO_1_BB_LDO_HF_BYP_MASK)
#define XCVR_ANALOG_BB_LDO_1_BB_LDO_HF_DIAGSEL_MASK (0x2000000U)
#define XCVR_ANALOG_BB_LDO_1_BB_LDO_HF_DIAGSEL_SHIFT (25U)
#define XCVR_ANALOG_BB_LDO_1_BB_LDO_HF_DIAGSEL(x) (((uint32_t)(((uint32_t)(x)) << XCVR_ANALOG_BB_LDO_1_BB_LDO_HF_DIAGSEL_SHIFT)) & XCVR_ANALOG_BB_LDO_1_BB_LDO_HF_DIAGSEL_MASK)
#define XCVR_ANALOG_BB_LDO_1_BB_LDO_HF_SPARE_MASK (0xC000000U)
#define XCVR_ANALOG_BB_LDO_1_BB_LDO_HF_SPARE_SHIFT (26U)
#define XCVR_ANALOG_BB_LDO_1_BB_LDO_HF_SPARE(x)  (((uint32_t)(((uint32_t)(x)) << XCVR_ANALOG_BB_LDO_1_BB_LDO_HF_SPARE_SHIFT)) & XCVR_ANALOG_BB_LDO_1_BB_LDO_HF_SPARE_MASK)
#define XCVR_ANALOG_BB_LDO_1_BB_LDO_HF_TRIM_MASK (0x70000000U)
#define XCVR_ANALOG_BB_LDO_1_BB_LDO_HF_TRIM_SHIFT (28U)
#define XCVR_ANALOG_BB_LDO_1_BB_LDO_HF_TRIM(x)   (((uint32_t)(((uint32_t)(x)) << XCVR_ANALOG_BB_LDO_1_BB_LDO_HF_TRIM_SHIFT)) & XCVR_ANALOG_BB_LDO_1_BB_LDO_HF_TRIM_MASK)

/*! @name BB_LDO_2 - RF Analog Baseband LDO Control 2 */
#define XCVR_ANALOG_BB_LDO_2_BB_LDO_PD_BYP_MASK  (0x1U)
#define XCVR_ANALOG_BB_LDO_2_BB_LDO_PD_BYP_SHIFT (0U)
#define XCVR_ANALOG_BB_LDO_2_BB_LDO_PD_BYP(x)    (((uint32_t)(((uint32_t)(x)) << XCVR_ANALOG_BB_LDO_2_BB_LDO_PD_BYP_SHIFT)) & XCVR_ANALOG_BB_LDO_2_BB_LDO_PD_BYP_MASK)
#define XCVR_ANALOG_BB_LDO_2_BB_LDO_PD_DIAGSEL_MASK (0x2U)
#define XCVR_ANALOG_BB_LDO_2_BB_LDO_PD_DIAGSEL_SHIFT (1U)
#define XCVR_ANALOG_BB_LDO_2_BB_LDO_PD_DIAGSEL(x) (((uint32_t)(((uint32_t)(x)) << XCVR_ANALOG_BB_LDO_2_BB_LDO_PD_DIAGSEL_SHIFT)) & XCVR_ANALOG_BB_LDO_2_BB_LDO_PD_DIAGSEL_MASK)
#define XCVR_ANALOG_BB_LDO_2_BB_LDO_PD_SPARE_MASK (0xCU)
#define XCVR_ANALOG_BB_LDO_2_BB_LDO_PD_SPARE_SHIFT (2U)
#define XCVR_ANALOG_BB_LDO_2_BB_LDO_PD_SPARE(x)  (((uint32_t)(((uint32_t)(x)) << XCVR_ANALOG_BB_LDO_2_BB_LDO_PD_SPARE_SHIFT)) & XCVR_ANALOG_BB_LDO_2_BB_LDO_PD_SPARE_MASK)
#define XCVR_ANALOG_BB_LDO_2_BB_LDO_PD_TRIM_MASK (0x70U)
#define XCVR_ANALOG_BB_LDO_2_BB_LDO_PD_TRIM_SHIFT (4U)
#define XCVR_ANALOG_BB_LDO_2_BB_LDO_PD_TRIM(x)   (((uint32_t)(((uint32_t)(x)) << XCVR_ANALOG_BB_LDO_2_BB_LDO_PD_TRIM_SHIFT)) & XCVR_ANALOG_BB_LDO_2_BB_LDO_PD_TRIM_MASK)
#define XCVR_ANALOG_BB_LDO_2_BB_LDO_VCO_SPARE_MASK (0x300U)
#define XCVR_ANALOG_BB_LDO_2_BB_LDO_VCO_SPARE_SHIFT (8U)
#define XCVR_ANALOG_BB_LDO_2_BB_LDO_VCO_SPARE(x) (((uint32_t)(((uint32_t)(x)) << XCVR_ANALOG_BB_LDO_2_BB_LDO_VCO_SPARE_SHIFT)) & XCVR_ANALOG_BB_LDO_2_BB_LDO_VCO_SPARE_MASK)
#define XCVR_ANALOG_BB_LDO_2_BB_LDO_VCOLO_BYP_MASK (0x400U)
#define XCVR_ANALOG_BB_LDO_2_BB_LDO_VCOLO_BYP_SHIFT (10U)
#define XCVR_ANALOG_BB_LDO_2_BB_LDO_VCOLO_BYP(x) (((uint32_t)(((uint32_t)(x)) << XCVR_ANALOG_BB_LDO_2_BB_LDO_VCOLO_BYP_SHIFT)) & XCVR_ANALOG_BB_LDO_2_BB_LDO_VCOLO_BYP_MASK)
#define XCVR_ANALOG_BB_LDO_2_BB_LDO_VCOLO_DIAGSEL_MASK (0x800U)
#define XCVR_ANALOG_BB_LDO_2_BB_LDO_VCOLO_DIAGSEL_SHIFT (11U)
#define XCVR_ANALOG_BB_LDO_2_BB_LDO_VCOLO_DIAGSEL(x) (((uint32_t)(((uint32_t)(x)) << XCVR_ANALOG_BB_LDO_2_BB_LDO_VCOLO_DIAGSEL_SHIFT)) & XCVR_ANALOG_BB_LDO_2_BB_LDO_VCOLO_DIAGSEL_MASK)
#define XCVR_ANALOG_BB_LDO_2_BB_LDO_VCOLO_TRIM_MASK (0x7000U)
#define XCVR_ANALOG_BB_LDO_2_BB_LDO_VCOLO_TRIM_SHIFT (12U)
#define XCVR_ANALOG_BB_LDO_2_BB_LDO_VCOLO_TRIM(x) (((uint32_t)(((uint32_t)(x)) << XCVR_ANALOG_BB_LDO_2_BB_LDO_VCOLO_TRIM_SHIFT)) & XCVR_ANALOG_BB_LDO_2_BB_LDO_VCOLO_TRIM_MASK)
#define XCVR_ANALOG_BB_LDO_2_BB_LDO_VTREF_DIAGSEL_MASK (0x10000U)
#define XCVR_ANALOG_BB_LDO_2_BB_LDO_VTREF_DIAGSEL_SHIFT (16U)
#define XCVR_ANALOG_BB_LDO_2_BB_LDO_VTREF_DIAGSEL(x) (((uint32_t)(((uint32_t)(x)) << XCVR_ANALOG_BB_LDO_2_BB_LDO_VTREF_DIAGSEL_SHIFT)) & XCVR_ANALOG_BB_LDO_2_BB_LDO_VTREF_DIAGSEL_MASK)
#define XCVR_ANALOG_BB_LDO_2_BB_LDO_VTREF_TC_MASK (0x60000U)
#define XCVR_ANALOG_BB_LDO_2_BB_LDO_VTREF_TC_SHIFT (17U)
#define XCVR_ANALOG_BB_LDO_2_BB_LDO_VTREF_TC(x)  (((uint32_t)(((uint32_t)(x)) << XCVR_ANALOG_BB_LDO_2_BB_LDO_VTREF_TC_SHIFT)) & XCVR_ANALOG_BB_LDO_2_BB_LDO_VTREF_TC_MASK)

/*! @name RX_ADC - RF Analog ADC Control */
#define XCVR_ANALOG_RX_ADC_RX_ADC_BUMP_MASK      (0xFFU)
#define XCVR_ANALOG_RX_ADC_RX_ADC_BUMP_SHIFT     (0U)
#define XCVR_ANALOG_RX_ADC_RX_ADC_BUMP(x)        (((uint32_t)(((uint32_t)(x)) << XCVR_ANALOG_RX_ADC_RX_ADC_BUMP_SHIFT)) & XCVR_ANALOG_RX_ADC_RX_ADC_BUMP_MASK)
#define XCVR_ANALOG_RX_ADC_RX_ADC_FS_SEL_MASK    (0x300U)
#define XCVR_ANALOG_RX_ADC_RX_ADC_FS_SEL_SHIFT   (8U)
#define XCVR_ANALOG_RX_ADC_RX_ADC_FS_SEL(x)      (((uint32_t)(((uint32_t)(x)) << XCVR_ANALOG_RX_ADC_RX_ADC_FS_SEL_SHIFT)) & XCVR_ANALOG_RX_ADC_RX_ADC_FS_SEL_MASK)
#define XCVR_ANALOG_RX_ADC_RX_ADC_I_DIAGSEL_MASK (0x400U)
#define XCVR_ANALOG_RX_ADC_RX_ADC_I_DIAGSEL_SHIFT (10U)
#define XCVR_ANALOG_RX_ADC_RX_ADC_I_DIAGSEL(x)   (((uint32_t)(((uint32_t)(x)) << XCVR_ANALOG_RX_ADC_RX_ADC_I_DIAGSEL_SHIFT)) & XCVR_ANALOG_RX_ADC_RX_ADC_I_DIAGSEL_MASK)
#define XCVR_ANALOG_RX_ADC_RX_ADC_Q_DIAGSEL_MASK (0x800U)
#define XCVR_ANALOG_RX_ADC_RX_ADC_Q_DIAGSEL_SHIFT (11U)
#define XCVR_ANALOG_RX_ADC_RX_ADC_Q_DIAGSEL(x)   (((uint32_t)(((uint32_t)(x)) << XCVR_ANALOG_RX_ADC_RX_ADC_Q_DIAGSEL_SHIFT)) & XCVR_ANALOG_RX_ADC_RX_ADC_Q_DIAGSEL_MASK)
#define XCVR_ANALOG_RX_ADC_RX_ADC_SPARE_MASK     (0xF000U)
#define XCVR_ANALOG_RX_ADC_RX_ADC_SPARE_SHIFT    (12U)
#define XCVR_ANALOG_RX_ADC_RX_ADC_SPARE(x)       (((uint32_t)(((uint32_t)(x)) << XCVR_ANALOG_RX_ADC_RX_ADC_SPARE_SHIFT)) & XCVR_ANALOG_RX_ADC_RX_ADC_SPARE_MASK)

/*! @name RX_BBA - RF Analog BBA Control */
#define XCVR_ANALOG_RX_BBA_RX_BBA_BW_SEL_MASK    (0x7U)
#define XCVR_ANALOG_RX_BBA_RX_BBA_BW_SEL_SHIFT   (0U)
#define XCVR_ANALOG_RX_BBA_RX_BBA_BW_SEL(x)      (((uint32_t)(((uint32_t)(x)) << XCVR_ANALOG_RX_BBA_RX_BBA_BW_SEL_SHIFT)) & XCVR_ANALOG_RX_BBA_RX_BBA_BW_SEL_MASK)
#define XCVR_ANALOG_RX_BBA_RX_BBA_CUR_BUMP_MASK  (0x8U)
#define XCVR_ANALOG_RX_BBA_RX_BBA_CUR_BUMP_SHIFT (3U)
#define XCVR_ANALOG_RX_BBA_RX_BBA_CUR_BUMP(x)    (((uint32_t)(((uint32_t)(x)) << XCVR_ANALOG_RX_BBA_RX_BBA_CUR_BUMP_SHIFT)) & XCVR_ANALOG_RX_BBA_RX_BBA_CUR_BUMP_MASK)
#define XCVR_ANALOG_RX_BBA_RX_BBA_DIAGSEL1_MASK  (0x10U)
#define XCVR_ANALOG_RX_BBA_RX_BBA_DIAGSEL1_SHIFT (4U)
#define XCVR_ANALOG_RX_BBA_RX_BBA_DIAGSEL1(x)    (((uint32_t)(((uint32_t)(x)) << XCVR_ANALOG_RX_BBA_RX_BBA_DIAGSEL1_SHIFT)) & XCVR_ANALOG_RX_BBA_RX_BBA_DIAGSEL1_MASK)
#define XCVR_ANALOG_RX_BBA_RX_BBA_DIAGSEL2_MASK  (0x20U)
#define XCVR_ANALOG_RX_BBA_RX_BBA_DIAGSEL2_SHIFT (5U)
#define XCVR_ANALOG_RX_BBA_RX_BBA_DIAGSEL2(x)    (((uint32_t)(((uint32_t)(x)) << XCVR_ANALOG_RX_BBA_RX_BBA_DIAGSEL2_SHIFT)) & XCVR_ANALOG_RX_BBA_RX_BBA_DIAGSEL2_MASK)
#define XCVR_ANALOG_RX_BBA_RX_BBA_DIAGSEL3_MASK  (0x40U)
#define XCVR_ANALOG_RX_BBA_RX_BBA_DIAGSEL3_SHIFT (6U)
#define XCVR_ANALOG_RX_BBA_RX_BBA_DIAGSEL3(x)    (((uint32_t)(((uint32_t)(x)) << XCVR_ANALOG_RX_BBA_RX_BBA_DIAGSEL3_SHIFT)) & XCVR_ANALOG_RX_BBA_RX_BBA_DIAGSEL3_MASK)
#define XCVR_ANALOG_RX_BBA_RX_BBA_DIAGSEL4_MASK  (0x80U)
#define XCVR_ANALOG_RX_BBA_RX_BBA_DIAGSEL4_SHIFT (7U)
#define XCVR_ANALOG_RX_BBA_RX_BBA_DIAGSEL4(x)    (((uint32_t)(((uint32_t)(x)) << XCVR_ANALOG_RX_BBA_RX_BBA_DIAGSEL4_SHIFT)) & XCVR_ANALOG_RX_BBA_RX_BBA_DIAGSEL4_MASK)
#define XCVR_ANALOG_RX_BBA_RX_BBA_SPARE_MASK     (0x3F0000U)
#define XCVR_ANALOG_RX_BBA_RX_BBA_SPARE_SHIFT    (16U)
#define XCVR_ANALOG_RX_BBA_RX_BBA_SPARE(x)       (((uint32_t)(((uint32_t)(x)) << XCVR_ANALOG_RX_BBA_RX_BBA_SPARE_SHIFT)) & XCVR_ANALOG_RX_BBA_RX_BBA_SPARE_MASK)
#define XCVR_ANALOG_RX_BBA_RX_BBA2_BW_SEL_MASK   (0x7000000U)
#define XCVR_ANALOG_RX_BBA_RX_BBA2_BW_SEL_SHIFT  (24U)
#define XCVR_ANALOG_RX_BBA_RX_BBA2_BW_SEL(x)     (((uint32_t)(((uint32_t)(x)) << XCVR_ANALOG_RX_BBA_RX_BBA2_BW_SEL_SHIFT)) & XCVR_ANALOG_RX_BBA_RX_BBA2_BW_SEL_MASK)
#define XCVR_ANALOG_RX_BBA_RX_BBA2_SPARE_MASK    (0x70000000U)
#define XCVR_ANALOG_RX_BBA_RX_BBA2_SPARE_SHIFT   (28U)
#define XCVR_ANALOG_RX_BBA_RX_BBA2_SPARE(x)      (((uint32_t)(((uint32_t)(x)) << XCVR_ANALOG_RX_BBA_RX_BBA2_SPARE_SHIFT)) & XCVR_ANALOG_RX_BBA_RX_BBA2_SPARE_MASK)

/*! @name RX_LNA - RF Analog LNA Control */
#define XCVR_ANALOG_RX_LNA_RX_LNA_BUMP_MASK      (0xFU)
#define XCVR_ANALOG_RX_LNA_RX_LNA_BUMP_SHIFT     (0U)
#define XCVR_ANALOG_RX_LNA_RX_LNA_BUMP(x)        (((uint32_t)(((uint32_t)(x)) << XCVR_ANALOG_RX_LNA_RX_LNA_BUMP_SHIFT)) & XCVR_ANALOG_RX_LNA_RX_LNA_BUMP_MASK)
#define XCVR_ANALOG_RX_LNA_RX_LNA_HG_DIAGSEL_MASK (0x10U)
#define XCVR_ANALOG_RX_LNA_RX_LNA_HG_DIAGSEL_SHIFT (4U)
#define XCVR_ANALOG_RX_LNA_RX_LNA_HG_DIAGSEL(x)  (((uint32_t)(((uint32_t)(x)) << XCVR_ANALOG_RX_LNA_RX_LNA_HG_DIAGSEL_SHIFT)) & XCVR_ANALOG_RX_LNA_RX_LNA_HG_DIAGSEL_MASK)
#define XCVR_ANALOG_RX_LNA_RX_LNA_HIZ_ENABLE_MASK (0x20U)
#define XCVR_ANALOG_RX_LNA_RX_LNA_HIZ_ENABLE_SHIFT (5U)
#define XCVR_ANALOG_RX_LNA_RX_LNA_HIZ_ENABLE(x)  (((uint32_t)(((uint32_t)(x)) << XCVR_ANALOG_RX_LNA_RX_LNA_HIZ_ENABLE_SHIFT)) & XCVR_ANALOG_RX_LNA_RX_LNA_HIZ_ENABLE_MASK)
#define XCVR_ANALOG_RX_LNA_RX_LNA_LG_DIAGSEL_MASK (0x40U)
#define XCVR_ANALOG_RX_LNA_RX_LNA_LG_DIAGSEL_SHIFT (6U)
#define XCVR_ANALOG_RX_LNA_RX_LNA_LG_DIAGSEL(x)  (((uint32_t)(((uint32_t)(x)) << XCVR_ANALOG_RX_LNA_RX_LNA_LG_DIAGSEL_SHIFT)) & XCVR_ANALOG_RX_LNA_RX_LNA_LG_DIAGSEL_MASK)
#define XCVR_ANALOG_RX_LNA_RX_LNA_SPARE_MASK     (0x300U)
#define XCVR_ANALOG_RX_LNA_RX_LNA_SPARE_SHIFT    (8U)
#define XCVR_ANALOG_RX_LNA_RX_LNA_SPARE(x)       (((uint32_t)(((uint32_t)(x)) << XCVR_ANALOG_RX_LNA_RX_LNA_SPARE_SHIFT)) & XCVR_ANALOG_RX_LNA_RX_LNA_SPARE_MASK)
#define XCVR_ANALOG_RX_LNA_RX_MIXER_BUMP_MASK    (0xF0000U)
#define XCVR_ANALOG_RX_LNA_RX_MIXER_BUMP_SHIFT   (16U)
#define XCVR_ANALOG_RX_LNA_RX_MIXER_BUMP(x)      (((uint32_t)(((uint32_t)(x)) << XCVR_ANALOG_RX_LNA_RX_MIXER_BUMP_SHIFT)) & XCVR_ANALOG_RX_LNA_RX_MIXER_BUMP_MASK)
#define XCVR_ANALOG_RX_LNA_RX_MIXER_SPARE_MASK   (0x100000U)
#define XCVR_ANALOG_RX_LNA_RX_MIXER_SPARE_SHIFT  (20U)
#define XCVR_ANALOG_RX_LNA_RX_MIXER_SPARE(x)     (((uint32_t)(((uint32_t)(x)) << XCVR_ANALOG_RX_LNA_RX_MIXER_SPARE_SHIFT)) & XCVR_ANALOG_RX_LNA_RX_MIXER_SPARE_MASK)

/*! @name RX_TZA - RF Analog TZA Control */
#define XCVR_ANALOG_RX_TZA_RX_TZA_BW_SEL_MASK    (0x7U)
#define XCVR_ANALOG_RX_TZA_RX_TZA_BW_SEL_SHIFT   (0U)
#define XCVR_ANALOG_RX_TZA_RX_TZA_BW_SEL(x)      (((uint32_t)(((uint32_t)(x)) << XCVR_ANALOG_RX_TZA_RX_TZA_BW_SEL_SHIFT)) & XCVR_ANALOG_RX_TZA_RX_TZA_BW_SEL_MASK)
#define XCVR_ANALOG_RX_TZA_RX_TZA_CUR_BUMP_MASK  (0x8U)
#define XCVR_ANALOG_RX_TZA_RX_TZA_CUR_BUMP_SHIFT (3U)
#define XCVR_ANALOG_RX_TZA_RX_TZA_CUR_BUMP(x)    (((uint32_t)(((uint32_t)(x)) << XCVR_ANALOG_RX_TZA_RX_TZA_CUR_BUMP_SHIFT)) & XCVR_ANALOG_RX_TZA_RX_TZA_CUR_BUMP_MASK)
#define XCVR_ANALOG_RX_TZA_RX_TZA_GAIN_BUMP_MASK (0x10U)
#define XCVR_ANALOG_RX_TZA_RX_TZA_GAIN_BUMP_SHIFT (4U)
#define XCVR_ANALOG_RX_TZA_RX_TZA_GAIN_BUMP(x)   (((uint32_t)(((uint32_t)(x)) << XCVR_ANALOG_RX_TZA_RX_TZA_GAIN_BUMP_SHIFT)) & XCVR_ANALOG_RX_TZA_RX_TZA_GAIN_BUMP_MASK)
#define XCVR_ANALOG_RX_TZA_RX_TZA_SPARE_MASK     (0x3F0000U)
#define XCVR_ANALOG_RX_TZA_RX_TZA_SPARE_SHIFT    (16U)
#define XCVR_ANALOG_RX_TZA_RX_TZA_SPARE(x)       (((uint32_t)(((uint32_t)(x)) << XCVR_ANALOG_RX_TZA_RX_TZA_SPARE_SHIFT)) & XCVR_ANALOG_RX_TZA_RX_TZA_SPARE_MASK)
#define XCVR_ANALOG_RX_TZA_RX_TZA1_DIAGSEL_MASK  (0x1000000U)
#define XCVR_ANALOG_RX_TZA_RX_TZA1_DIAGSEL_SHIFT (24U)
#define XCVR_ANALOG_RX_TZA_RX_TZA1_DIAGSEL(x)    (((uint32_t)(((uint32_t)(x)) << XCVR_ANALOG_RX_TZA_RX_TZA1_DIAGSEL_SHIFT)) & XCVR_ANALOG_RX_TZA_RX_TZA1_DIAGSEL_MASK)
#define XCVR_ANALOG_RX_TZA_RX_TZA2_DIAGSEL_MASK  (0x2000000U)
#define XCVR_ANALOG_RX_TZA_RX_TZA2_DIAGSEL_SHIFT (25U)
#define XCVR_ANALOG_RX_TZA_RX_TZA2_DIAGSEL(x)    (((uint32_t)(((uint32_t)(x)) << XCVR_ANALOG_RX_TZA_RX_TZA2_DIAGSEL_SHIFT)) & XCVR_ANALOG_RX_TZA_RX_TZA2_DIAGSEL_MASK)
#define XCVR_ANALOG_RX_TZA_RX_TZA3_DIAGSEL_MASK  (0x4000000U)
#define XCVR_ANALOG_RX_TZA_RX_TZA3_DIAGSEL_SHIFT (26U)
#define XCVR_ANALOG_RX_TZA_RX_TZA3_DIAGSEL(x)    (((uint32_t)(((uint32_t)(x)) << XCVR_ANALOG_RX_TZA_RX_TZA3_DIAGSEL_SHIFT)) & XCVR_ANALOG_RX_TZA_RX_TZA3_DIAGSEL_MASK)
#define XCVR_ANALOG_RX_TZA_RX_TZA4_DIAGSEL_MASK  (0x8000000U)
#define XCVR_ANALOG_RX_TZA_RX_TZA4_DIAGSEL_SHIFT (27U)
#define XCVR_ANALOG_RX_TZA_RX_TZA4_DIAGSEL(x)    (((uint32_t)(((uint32_t)(x)) << XCVR_ANALOG_RX_TZA_RX_TZA4_DIAGSEL_SHIFT)) & XCVR_ANALOG_RX_TZA_RX_TZA4_DIAGSEL_MASK)

/*! @name RX_AUXPLL - RF Analog Aux PLL Control */
#define XCVR_ANALOG_RX_AUXPLL_BIAS_TRIM_MASK     (0x7U)
#define XCVR_ANALOG_RX_AUXPLL_BIAS_TRIM_SHIFT    (0U)
#define XCVR_ANALOG_RX_AUXPLL_BIAS_TRIM(x)       (((uint32_t)(((uint32_t)(x)) << XCVR_ANALOG_RX_AUXPLL_BIAS_TRIM_SHIFT)) & XCVR_ANALOG_RX_AUXPLL_BIAS_TRIM_MASK)
#define XCVR_ANALOG_RX_AUXPLL_DIAGSEL1_MASK      (0x8U)
#define XCVR_ANALOG_RX_AUXPLL_DIAGSEL1_SHIFT     (3U)
#define XCVR_ANALOG_RX_AUXPLL_DIAGSEL1(x)        (((uint32_t)(((uint32_t)(x)) << XCVR_ANALOG_RX_AUXPLL_DIAGSEL1_SHIFT)) & XCVR_ANALOG_RX_AUXPLL_DIAGSEL1_MASK)
#define XCVR_ANALOG_RX_AUXPLL_DIAGSEL2_MASK      (0x10U)
#define XCVR_ANALOG_RX_AUXPLL_DIAGSEL2_SHIFT     (4U)
#define XCVR_ANALOG_RX_AUXPLL_DIAGSEL2(x)        (((uint32_t)(((uint32_t)(x)) << XCVR_ANALOG_RX_AUXPLL_DIAGSEL2_SHIFT)) & XCVR_ANALOG_RX_AUXPLL_DIAGSEL2_MASK)
#define XCVR_ANALOG_RX_AUXPLL_LF_CNTL_MASK       (0xE0U)
#define XCVR_ANALOG_RX_AUXPLL_LF_CNTL_SHIFT      (5U)
#define XCVR_ANALOG_RX_AUXPLL_LF_CNTL(x)         (((uint32_t)(((uint32_t)(x)) << XCVR_ANALOG_RX_AUXPLL_LF_CNTL_SHIFT)) & XCVR_ANALOG_RX_AUXPLL_LF_CNTL_MASK)
#define XCVR_ANALOG_RX_AUXPLL_SPARE_MASK         (0xF00U)
#define XCVR_ANALOG_RX_AUXPLL_SPARE_SHIFT        (8U)
#define XCVR_ANALOG_RX_AUXPLL_SPARE(x)           (((uint32_t)(((uint32_t)(x)) << XCVR_ANALOG_RX_AUXPLL_SPARE_SHIFT)) & XCVR_ANALOG_RX_AUXPLL_SPARE_MASK)
#define XCVR_ANALOG_RX_AUXPLL_VCO_DAC_REF_ADJUST_MASK (0xF000U)
#define XCVR_ANALOG_RX_AUXPLL_VCO_DAC_REF_ADJUST_SHIFT (12U)
#define XCVR_ANALOG_RX_AUXPLL_VCO_DAC_REF_ADJUST(x) (((uint32_t)(((uint32_t)(x)) << XCVR_ANALOG_RX_AUXPLL_VCO_DAC_REF_ADJUST_SHIFT)) & XCVR_ANALOG_RX_AUXPLL_VCO_DAC_REF_ADJUST_MASK)
#define XCVR_ANALOG_RX_AUXPLL_VTUNE_TESTMODE_MASK (0x10000U)
#define XCVR_ANALOG_RX_AUXPLL_VTUNE_TESTMODE_SHIFT (16U)
#define XCVR_ANALOG_RX_AUXPLL_VTUNE_TESTMODE(x)  (((uint32_t)(((uint32_t)(x)) << XCVR_ANALOG_RX_AUXPLL_VTUNE_TESTMODE_SHIFT)) & XCVR_ANALOG_RX_AUXPLL_VTUNE_TESTMODE_MASK)
#define XCVR_ANALOG_RX_AUXPLL_RXTX_BAL_BIAST_MASK (0x300000U)
#define XCVR_ANALOG_RX_AUXPLL_RXTX_BAL_BIAST_SHIFT (20U)
#define XCVR_ANALOG_RX_AUXPLL_RXTX_BAL_BIAST(x)  (((uint32_t)(((uint32_t)(x)) << XCVR_ANALOG_RX_AUXPLL_RXTX_BAL_BIAST_SHIFT)) & XCVR_ANALOG_RX_AUXPLL_RXTX_BAL_BIAST_MASK)
#define XCVR_ANALOG_RX_AUXPLL_RXTX_BAL_SPARE_MASK (0x7000000U)
#define XCVR_ANALOG_RX_AUXPLL_RXTX_BAL_SPARE_SHIFT (24U)
#define XCVR_ANALOG_RX_AUXPLL_RXTX_BAL_SPARE(x)  (((uint32_t)(((uint32_t)(x)) << XCVR_ANALOG_RX_AUXPLL_RXTX_BAL_SPARE_SHIFT)) & XCVR_ANALOG_RX_AUXPLL_RXTX_BAL_SPARE_MASK)
#define XCVR_ANALOG_RX_AUXPLL_RXTX_RCCAL_DIAGSEL_MASK (0x10000000U)
#define XCVR_ANALOG_RX_AUXPLL_RXTX_RCCAL_DIAGSEL_SHIFT (28U)
#define XCVR_ANALOG_RX_AUXPLL_RXTX_RCCAL_DIAGSEL(x) (((uint32_t)(((uint32_t)(x)) << XCVR_ANALOG_RX_AUXPLL_RXTX_RCCAL_DIAGSEL_SHIFT)) & XCVR_ANALOG_RX_AUXPLL_RXTX_RCCAL_DIAGSEL_MASK)

/*! @name SY_CTRL_1 - RF Analog Synthesizer Control 1 */
#define XCVR_ANALOG_SY_CTRL_1_SY_DIVN_SPARE_MASK (0x1U)
#define XCVR_ANALOG_SY_CTRL_1_SY_DIVN_SPARE_SHIFT (0U)
#define XCVR_ANALOG_SY_CTRL_1_SY_DIVN_SPARE(x)   (((uint32_t)(((uint32_t)(x)) << XCVR_ANALOG_SY_CTRL_1_SY_DIVN_SPARE_SHIFT)) & XCVR_ANALOG_SY_CTRL_1_SY_DIVN_SPARE_MASK)
#define XCVR_ANALOG_SY_CTRL_1_SY_FCAL_SPARE_MASK (0x2U)
#define XCVR_ANALOG_SY_CTRL_1_SY_FCAL_SPARE_SHIFT (1U)
#define XCVR_ANALOG_SY_CTRL_1_SY_FCAL_SPARE(x)   (((uint32_t)(((uint32_t)(x)) << XCVR_ANALOG_SY_CTRL_1_SY_FCAL_SPARE_SHIFT)) & XCVR_ANALOG_SY_CTRL_1_SY_FCAL_SPARE_MASK)
#define XCVR_ANALOG_SY_CTRL_1_SY_LO_BUMP_RTLO_FDBK_MASK (0x30U)
#define XCVR_ANALOG_SY_CTRL_1_SY_LO_BUMP_RTLO_FDBK_SHIFT (4U)
#define XCVR_ANALOG_SY_CTRL_1_SY_LO_BUMP_RTLO_FDBK(x) (((uint32_t)(((uint32_t)(x)) << XCVR_ANALOG_SY_CTRL_1_SY_LO_BUMP_RTLO_FDBK_SHIFT)) & XCVR_ANALOG_SY_CTRL_1_SY_LO_BUMP_RTLO_FDBK_MASK)
#define XCVR_ANALOG_SY_CTRL_1_SY_LO_BUMP_RTLO_RX_MASK (0xC0U)
#define XCVR_ANALOG_SY_CTRL_1_SY_LO_BUMP_RTLO_RX_SHIFT (6U)
#define XCVR_ANALOG_SY_CTRL_1_SY_LO_BUMP_RTLO_RX(x) (((uint32_t)(((uint32_t)(x)) << XCVR_ANALOG_SY_CTRL_1_SY_LO_BUMP_RTLO_RX_SHIFT)) & XCVR_ANALOG_SY_CTRL_1_SY_LO_BUMP_RTLO_RX_MASK)
#define XCVR_ANALOG_SY_CTRL_1_SY_LO_BUMP_RTLO_TX_MASK (0x300U)
#define XCVR_ANALOG_SY_CTRL_1_SY_LO_BUMP_RTLO_TX_SHIFT (8U)
#define XCVR_ANALOG_SY_CTRL_1_SY_LO_BUMP_RTLO_TX(x) (((uint32_t)(((uint32_t)(x)) << XCVR_ANALOG_SY_CTRL_1_SY_LO_BUMP_RTLO_TX_SHIFT)) & XCVR_ANALOG_SY_CTRL_1_SY_LO_BUMP_RTLO_TX_MASK)
#define XCVR_ANALOG_SY_CTRL_1_SY_LO_DIAGSEL_MASK (0x400U)
#define XCVR_ANALOG_SY_CTRL_1_SY_LO_DIAGSEL_SHIFT (10U)
#define XCVR_ANALOG_SY_CTRL_1_SY_LO_DIAGSEL(x)   (((uint32_t)(((uint32_t)(x)) << XCVR_ANALOG_SY_CTRL_1_SY_LO_DIAGSEL_SHIFT)) & XCVR_ANALOG_SY_CTRL_1_SY_LO_DIAGSEL_MASK)
#define XCVR_ANALOG_SY_CTRL_1_SY_LO_SPARE_MASK   (0x7000U)
#define XCVR_ANALOG_SY_CTRL_1_SY_LO_SPARE_SHIFT  (12U)
#define XCVR_ANALOG_SY_CTRL_1_SY_LO_SPARE(x)     (((uint32_t)(((uint32_t)(x)) << XCVR_ANALOG_SY_CTRL_1_SY_LO_SPARE_SHIFT)) & XCVR_ANALOG_SY_CTRL_1_SY_LO_SPARE_MASK)
#define XCVR_ANALOG_SY_CTRL_1_SY_LPF_FILT_CTRL_MASK (0x70000U)
#define XCVR_ANALOG_SY_CTRL_1_SY_LPF_FILT_CTRL_SHIFT (16U)
#define XCVR_ANALOG_SY_CTRL_1_SY_LPF_FILT_CTRL(x) (((uint32_t)(((uint32_t)(x)) << XCVR_ANALOG_SY_CTRL_1_SY_LPF_FILT_CTRL_SHIFT)) & XCVR_ANALOG_SY_CTRL_1_SY_LPF_FILT_CTRL_MASK)
#define XCVR_ANALOG_SY_CTRL_1_SY_LPF_SPARE_MASK  (0x80000U)
#define XCVR_ANALOG_SY_CTRL_1_SY_LPF_SPARE_SHIFT (19U)
#define XCVR_ANALOG_SY_CTRL_1_SY_LPF_SPARE(x)    (((uint32_t)(((uint32_t)(x)) << XCVR_ANALOG_SY_CTRL_1_SY_LPF_SPARE_SHIFT)) & XCVR_ANALOG_SY_CTRL_1_SY_LPF_SPARE_MASK)
#define XCVR_ANALOG_SY_CTRL_1_SY_PD_DIAGSEL_MASK (0x100000U)
#define XCVR_ANALOG_SY_CTRL_1_SY_PD_DIAGSEL_SHIFT (20U)
#define XCVR_ANALOG_SY_CTRL_1_SY_PD_DIAGSEL(x)   (((uint32_t)(((uint32_t)(x)) << XCVR_ANALOG_SY_CTRL_1_SY_PD_DIAGSEL_SHIFT)) & XCVR_ANALOG_SY_CTRL_1_SY_PD_DIAGSEL_MASK)
#define XCVR_ANALOG_SY_CTRL_1_SY_PD_PCH_TUNE_MASK (0x600000U)
#define XCVR_ANALOG_SY_CTRL_1_SY_PD_PCH_TUNE_SHIFT (21U)
#define XCVR_ANALOG_SY_CTRL_1_SY_PD_PCH_TUNE(x)  (((uint32_t)(((uint32_t)(x)) << XCVR_ANALOG_SY_CTRL_1_SY_PD_PCH_TUNE_SHIFT)) & XCVR_ANALOG_SY_CTRL_1_SY_PD_PCH_TUNE_MASK)
#define XCVR_ANALOG_SY_CTRL_1_SY_PD_PCH_SEL_MASK (0x800000U)
#define XCVR_ANALOG_SY_CTRL_1_SY_PD_PCH_SEL_SHIFT (23U)
#define XCVR_ANALOG_SY_CTRL_1_SY_PD_PCH_SEL(x)   (((uint32_t)(((uint32_t)(x)) << XCVR_ANALOG_SY_CTRL_1_SY_PD_PCH_SEL_SHIFT)) & XCVR_ANALOG_SY_CTRL_1_SY_PD_PCH_SEL_MASK)
#define XCVR_ANALOG_SY_CTRL_1_SY_PD_SPARE_MASK   (0x3000000U)
#define XCVR_ANALOG_SY_CTRL_1_SY_PD_SPARE_SHIFT  (24U)
#define XCVR_ANALOG_SY_CTRL_1_SY_PD_SPARE(x)     (((uint32_t)(((uint32_t)(x)) << XCVR_ANALOG_SY_CTRL_1_SY_PD_SPARE_SHIFT)) & XCVR_ANALOG_SY_CTRL_1_SY_PD_SPARE_MASK)
#define XCVR_ANALOG_SY_CTRL_1_SY_PD_VTUNE_OVERRIDE_TEST_MODE_MASK (0x10000000U)
#define XCVR_ANALOG_SY_CTRL_1_SY_PD_VTUNE_OVERRIDE_TEST_MODE_SHIFT (28U)
#define XCVR_ANALOG_SY_CTRL_1_SY_PD_VTUNE_OVERRIDE_TEST_MODE(x) (((uint32_t)(((uint32_t)(x)) << XCVR_ANALOG_SY_CTRL_1_SY_PD_VTUNE_OVERRIDE_TEST_MODE_SHIFT)) & XCVR_ANALOG_SY_CTRL_1_SY_PD_VTUNE_OVERRIDE_TEST_MODE_MASK)

/*! @name SY_CTRL_2 - RF Analog Synthesizer Control 2 */
#define XCVR_ANALOG_SY_CTRL_2_SY_VCO_BIAS_MASK   (0x7U)
#define XCVR_ANALOG_SY_CTRL_2_SY_VCO_BIAS_SHIFT  (0U)
#define XCVR_ANALOG_SY_CTRL_2_SY_VCO_BIAS(x)     (((uint32_t)(((uint32_t)(x)) << XCVR_ANALOG_SY_CTRL_2_SY_VCO_BIAS_SHIFT)) & XCVR_ANALOG_SY_CTRL_2_SY_VCO_BIAS_MASK)
#define XCVR_ANALOG_SY_CTRL_2_SY_VCO_DIAGSEL_MASK (0x8U)
#define XCVR_ANALOG_SY_CTRL_2_SY_VCO_DIAGSEL_SHIFT (3U)
#define XCVR_ANALOG_SY_CTRL_2_SY_VCO_DIAGSEL(x)  (((uint32_t)(((uint32_t)(x)) << XCVR_ANALOG_SY_CTRL_2_SY_VCO_DIAGSEL_SHIFT)) & XCVR_ANALOG_SY_CTRL_2_SY_VCO_DIAGSEL_MASK)
#define XCVR_ANALOG_SY_CTRL_2_SY_VCO_KV_MASK     (0x70U)
#define XCVR_ANALOG_SY_CTRL_2_SY_VCO_KV_SHIFT    (4U)
#define XCVR_ANALOG_SY_CTRL_2_SY_VCO_KV(x)       (((uint32_t)(((uint32_t)(x)) << XCVR_ANALOG_SY_CTRL_2_SY_VCO_KV_SHIFT)) & XCVR_ANALOG_SY_CTRL_2_SY_VCO_KV_MASK)
#define XCVR_ANALOG_SY_CTRL_2_SY_VCO_KVM_MASK    (0x700U)
#define XCVR_ANALOG_SY_CTRL_2_SY_VCO_KVM_SHIFT   (8U)
#define XCVR_ANALOG_SY_CTRL_2_SY_VCO_KVM(x)      (((uint32_t)(((uint32_t)(x)) << XCVR_ANALOG_SY_CTRL_2_SY_VCO_KVM_SHIFT)) & XCVR_ANALOG_SY_CTRL_2_SY_VCO_KVM_MASK)
#define XCVR_ANALOG_SY_CTRL_2_SY_VCO_PK_DET_ON_MASK (0x1000U)
#define XCVR_ANALOG_SY_CTRL_2_SY_VCO_PK_DET_ON_SHIFT (12U)
#define XCVR_ANALOG_SY_CTRL_2_SY_VCO_PK_DET_ON(x) (((uint32_t)(((uint32_t)(x)) << XCVR_ANALOG_SY_CTRL_2_SY_VCO_PK_DET_ON_SHIFT)) & XCVR_ANALOG_SY_CTRL_2_SY_VCO_PK_DET_ON_MASK)
#define XCVR_ANALOG_SY_CTRL_2_SY_VCO_SPARE_MASK  (0x1C000U)
#define XCVR_ANALOG_SY_CTRL_2_SY_VCO_SPARE_SHIFT (14U)
#define XCVR_ANALOG_SY_CTRL_2_SY_VCO_SPARE(x)    (((uint32_t)(((uint32_t)(x)) << XCVR_ANALOG_SY_CTRL_2_SY_VCO_SPARE_SHIFT)) & XCVR_ANALOG_SY_CTRL_2_SY_VCO_SPARE_MASK)

/*! @name TX_DAC_PA - RF Analog TX HPM DAC and PA Control */
#define XCVR_ANALOG_TX_DAC_PA_TX_DAC_BUMP_CAP_MASK (0x3U)
#define XCVR_ANALOG_TX_DAC_PA_TX_DAC_BUMP_CAP_SHIFT (0U)
#define XCVR_ANALOG_TX_DAC_PA_TX_DAC_BUMP_CAP(x) (((uint32_t)(((uint32_t)(x)) << XCVR_ANALOG_TX_DAC_PA_TX_DAC_BUMP_CAP_SHIFT)) & XCVR_ANALOG_TX_DAC_PA_TX_DAC_BUMP_CAP_MASK)
#define XCVR_ANALOG_TX_DAC_PA_TX_DAC_BUMP_IDAC_MASK (0x18U)
#define XCVR_ANALOG_TX_DAC_PA_TX_DAC_BUMP_IDAC_SHIFT (3U)
#define XCVR_ANALOG_TX_DAC_PA_TX_DAC_BUMP_IDAC(x) (((uint32_t)(((uint32_t)(x)) << XCVR_ANALOG_TX_DAC_PA_TX_DAC_BUMP_IDAC_SHIFT)) & XCVR_ANALOG_TX_DAC_PA_TX_DAC_BUMP_IDAC_MASK)
#define XCVR_ANALOG_TX_DAC_PA_TX_DAC_BUMP_RLOAD_MASK (0xC0U)
#define XCVR_ANALOG_TX_DAC_PA_TX_DAC_BUMP_RLOAD_SHIFT (6U)
#define XCVR_ANALOG_TX_DAC_PA_TX_DAC_BUMP_RLOAD(x) (((uint32_t)(((uint32_t)(x)) << XCVR_ANALOG_TX_DAC_PA_TX_DAC_BUMP_RLOAD_SHIFT)) & XCVR_ANALOG_TX_DAC_PA_TX_DAC_BUMP_RLOAD_MASK)
#define XCVR_ANALOG_TX_DAC_PA_TX_DAC_DIAGSEL_MASK (0x200U)
#define XCVR_ANALOG_TX_DAC_PA_TX_DAC_DIAGSEL_SHIFT (9U)
#define XCVR_ANALOG_TX_DAC_PA_TX_DAC_DIAGSEL(x)  (((uint32_t)(((uint32_t)(x)) << XCVR_ANALOG_TX_DAC_PA_TX_DAC_DIAGSEL_SHIFT)) & XCVR_ANALOG_TX_DAC_PA_TX_DAC_DIAGSEL_MASK)
#define XCVR_ANALOG_TX_DAC_PA_TX_DAC_INVERT_CLK_MASK (0x400U)
#define XCVR_ANALOG_TX_DAC_PA_TX_DAC_INVERT_CLK_SHIFT (10U)
#define XCVR_ANALOG_TX_DAC_PA_TX_DAC_INVERT_CLK(x) (((uint32_t)(((uint32_t)(x)) << XCVR_ANALOG_TX_DAC_PA_TX_DAC_INVERT_CLK_SHIFT)) & XCVR_ANALOG_TX_DAC_PA_TX_DAC_INVERT_CLK_MASK)
#define XCVR_ANALOG_TX_DAC_PA_TX_DAC_OPAMP_DIAGSEL_MASK (0x800U)
#define XCVR_ANALOG_TX_DAC_PA_TX_DAC_OPAMP_DIAGSEL_SHIFT (11U)
#define XCVR_ANALOG_TX_DAC_PA_TX_DAC_OPAMP_DIAGSEL(x) (((uint32_t)(((uint32_t)(x)) << XCVR_ANALOG_TX_DAC_PA_TX_DAC_OPAMP_DIAGSEL_SHIFT)) & XCVR_ANALOG_TX_DAC_PA_TX_DAC_OPAMP_DIAGSEL_MASK)
#define XCVR_ANALOG_TX_DAC_PA_TX_DAC_SPARE_MASK  (0xE000U)
#define XCVR_ANALOG_TX_DAC_PA_TX_DAC_SPARE_SHIFT (13U)
#define XCVR_ANALOG_TX_DAC_PA_TX_DAC_SPARE(x)    (((uint32_t)(((uint32_t)(x)) << XCVR_ANALOG_TX_DAC_PA_TX_DAC_SPARE_SHIFT)) & XCVR_ANALOG_TX_DAC_PA_TX_DAC_SPARE_MASK)
#define XCVR_ANALOG_TX_DAC_PA_TX_PA_BUMP_VBIAS_MASK (0xE0000U)
#define XCVR_ANALOG_TX_DAC_PA_TX_PA_BUMP_VBIAS_SHIFT (17U)
#define XCVR_ANALOG_TX_DAC_PA_TX_PA_BUMP_VBIAS(x) (((uint32_t)(((uint32_t)(x)) << XCVR_ANALOG_TX_DAC_PA_TX_PA_BUMP_VBIAS_SHIFT)) & XCVR_ANALOG_TX_DAC_PA_TX_PA_BUMP_VBIAS_MASK)
#define XCVR_ANALOG_TX_DAC_PA_TX_PA_DIAGSEL_MASK (0x200000U)
#define XCVR_ANALOG_TX_DAC_PA_TX_PA_DIAGSEL_SHIFT (21U)
#define XCVR_ANALOG_TX_DAC_PA_TX_PA_DIAGSEL(x)   (((uint32_t)(((uint32_t)(x)) << XCVR_ANALOG_TX_DAC_PA_TX_PA_DIAGSEL_SHIFT)) & XCVR_ANALOG_TX_DAC_PA_TX_PA_DIAGSEL_MASK)
#define XCVR_ANALOG_TX_DAC_PA_TX_PA_SPARE_MASK   (0x3800000U)
#define XCVR_ANALOG_TX_DAC_PA_TX_PA_SPARE_SHIFT  (23U)
#define XCVR_ANALOG_TX_DAC_PA_TX_PA_SPARE(x)     (((uint32_t)(((uint32_t)(x)) << XCVR_ANALOG_TX_DAC_PA_TX_PA_SPARE_SHIFT)) & XCVR_ANALOG_TX_DAC_PA_TX_PA_SPARE_MASK)

/*! @name BALUN_TX - RF Analog Balun TX Mode Control */
#define XCVR_ANALOG_BALUN_TX_RXTX_BAL_TX_CODE_MASK (0xFFFFFFU)
#define XCVR_ANALOG_BALUN_TX_RXTX_BAL_TX_CODE_SHIFT (0U)
#define XCVR_ANALOG_BALUN_TX_RXTX_BAL_TX_CODE(x) (((uint32_t)(((uint32_t)(x)) << XCVR_ANALOG_BALUN_TX_RXTX_BAL_TX_CODE_SHIFT)) & XCVR_ANALOG_BALUN_TX_RXTX_BAL_TX_CODE_MASK)

/*! @name BALUN_RX - RF Analog Balun RX Mode Control */
#define XCVR_ANALOG_BALUN_RX_RXTX_BAL_RX_CODE_MASK (0xFFFFFFU)
#define XCVR_ANALOG_BALUN_RX_RXTX_BAL_RX_CODE_SHIFT (0U)
#define XCVR_ANALOG_BALUN_RX_RXTX_BAL_RX_CODE(x) (((uint32_t)(((uint32_t)(x)) << XCVR_ANALOG_BALUN_RX_RXTX_BAL_RX_CODE_SHIFT)) & XCVR_ANALOG_BALUN_RX_RXTX_BAL_RX_CODE_MASK)

/*! @name DFT_OBSV_1 - RF Analog DFT Observation Register 1 */
#define XCVR_ANALOG_DFT_OBSV_1_DFT_FREQ_COUNTER_MASK (0x7FFFFU)
#define XCVR_ANALOG_DFT_OBSV_1_DFT_FREQ_COUNTER_SHIFT (0U)
#define XCVR_ANALOG_DFT_OBSV_1_DFT_FREQ_COUNTER(x) (((uint32_t)(((uint32_t)(x)) << XCVR_ANALOG_DFT_OBSV_1_DFT_FREQ_COUNTER_SHIFT)) & XCVR_ANALOG_DFT_OBSV_1_DFT_FREQ_COUNTER_MASK)
#define XCVR_ANALOG_DFT_OBSV_1_CTUNE_MAX_DIFF_MASK (0xFF00000U)
#define XCVR_ANALOG_DFT_OBSV_1_CTUNE_MAX_DIFF_SHIFT (20U)
#define XCVR_ANALOG_DFT_OBSV_1_CTUNE_MAX_DIFF(x) (((uint32_t)(((uint32_t)(x)) << XCVR_ANALOG_DFT_OBSV_1_CTUNE_MAX_DIFF_SHIFT)) & XCVR_ANALOG_DFT_OBSV_1_CTUNE_MAX_DIFF_MASK)

/*! @name DFT_OBSV_2 - RF Analog DFT Observation Register 2 */
#define XCVR_ANALOG_DFT_OBSV_2_SYN_BIST_MAX_DIFF_MASK (0x1FFFFU)
#define XCVR_ANALOG_DFT_OBSV_2_SYN_BIST_MAX_DIFF_SHIFT (0U)
#define XCVR_ANALOG_DFT_OBSV_2_SYN_BIST_MAX_DIFF(x) (((uint32_t)(((uint32_t)(x)) << XCVR_ANALOG_DFT_OBSV_2_SYN_BIST_MAX_DIFF_SHIFT)) & XCVR_ANALOG_DFT_OBSV_2_SYN_BIST_MAX_DIFF_MASK)
#define XCVR_ANALOG_DFT_OBSV_2_SYN_BIST_MAX_DIFF_CH_MASK (0x7F000000U)
#define XCVR_ANALOG_DFT_OBSV_2_SYN_BIST_MAX_DIFF_CH_SHIFT (24U)
#define XCVR_ANALOG_DFT_OBSV_2_SYN_BIST_MAX_DIFF_CH(x) (((uint32_t)(((uint32_t)(x)) << XCVR_ANALOG_DFT_OBSV_2_SYN_BIST_MAX_DIFF_CH_SHIFT)) & XCVR_ANALOG_DFT_OBSV_2_SYN_BIST_MAX_DIFF_CH_MASK)
#define XCVR_ANALOG_DFT_OBSV_2_SYN_BIST_IGNORE_FAILS_MASK (0x80000000U)
#define XCVR_ANALOG_DFT_OBSV_2_SYN_BIST_IGNORE_FAILS_SHIFT (31U)
#define XCVR_ANALOG_DFT_OBSV_2_SYN_BIST_IGNORE_FAILS(x) (((uint32_t)(((uint32_t)(x)) << XCVR_ANALOG_DFT_OBSV_2_SYN_BIST_IGNORE_FAILS_SHIFT)) & XCVR_ANALOG_DFT_OBSV_2_SYN_BIST_IGNORE_FAILS_MASK)


/*!
 * @}
 */ /* end of group XCVR_ANALOG_Register_Masks */


/* XCVR_ANALOG - Peripheral instance base addresses */
/** Peripheral XCVR_ANA base address */
#define XCVR_ANA_BASE                            (0x4005C500u)
/** Peripheral XCVR_ANA base pointer */
#define XCVR_ANA                                 ((XCVR_ANALOG_Type *)XCVR_ANA_BASE)
/** Array initializer of XCVR_ANALOG peripheral base addresses */
#define XCVR_ANALOG_BASE_ADDRS                   { XCVR_ANA_BASE }
/** Array initializer of XCVR_ANALOG peripheral base pointers */
#define XCVR_ANALOG_BASE_PTRS                    { XCVR_ANA }

/*!
 * @}
 */ /* end of group XCVR_ANALOG_Peripheral_Access_Layer */


/* ----------------------------------------------------------------------------
   -- XCVR_CTRL Peripheral Access Layer
   ---------------------------------------------------------------------------- */

/*!
 * @addtogroup XCVR_CTRL_Peripheral_Access_Layer XCVR_CTRL Peripheral Access Layer
 * @{
 */

/** XCVR_CTRL - Register Layout Typedef */
typedef struct {
  __IO uint32_t XCVR_CTRL;                         /**< TRANSCEIVER CONTROL, offset: 0x0 */
  __IO uint32_t XCVR_STATUS;                       /**< TRANSCEIVER STATUS, offset: 0x4 */
  __IO uint32_t BLE_ARB_CTRL;                      /**< BLE ARBITRATION CONTROL, offset: 0x8 */
       uint8_t RESERVED_0[4];
  __IO uint32_t OVERWRITE_VER;                     /**< OVERWRITE VERSION, offset: 0x10 */
  __IO uint32_t DMA_CTRL;                          /**< TRANSCEIVER DMA CONTROL, offset: 0x14 */
  __I  uint32_t DMA_DATA;                          /**< TRANSCEIVER DMA DATA, offset: 0x18 */
  __IO uint32_t DTEST_CTRL;                        /**< DIGITAL TEST MUX CONTROL, offset: 0x1C */
  __IO uint32_t PACKET_RAM_CTRL;                   /**< PACKET RAM CONTROL, offset: 0x20 */
  __IO uint32_t FAD_CTRL;                          /**< FAD CONTROL, offset: 0x24 */
  __IO uint32_t LPPS_CTRL;                         /**< LOW POWER PREAMBLE SEARCH CONTROL, offset: 0x28 */
  __IO uint32_t RF_NOT_ALLOWED_CTRL;               /**< WIFI COEXISTENCE CONTROL, offset: 0x2C */
  __IO uint32_t CRCW_CFG;                          /**< CRC/WHITENER CONTROL, offset: 0x30 */
  __I  uint32_t CRC_EC_MASK;                       /**< CRC ERROR CORRECTION MASK, offset: 0x34 */
  __I  uint32_t CRC_RES_OUT;                       /**< CRC RESULT, offset: 0x38 */
} XCVR_CTRL_Type;

/* ----------------------------------------------------------------------------
   -- XCVR_CTRL Register Masks
   ---------------------------------------------------------------------------- */

/*!
 * @addtogroup XCVR_CTRL_Register_Masks XCVR_CTRL Register Masks
 * @{
 */

/*! @name XCVR_CTRL - TRANSCEIVER CONTROL */
#define XCVR_CTRL_XCVR_CTRL_PROTOCOL_MASK        (0xFU)
#define XCVR_CTRL_XCVR_CTRL_PROTOCOL_SHIFT       (0U)
#define XCVR_CTRL_XCVR_CTRL_PROTOCOL(x)          (((uint32_t)(((uint32_t)(x)) << XCVR_CTRL_XCVR_CTRL_PROTOCOL_SHIFT)) & XCVR_CTRL_XCVR_CTRL_PROTOCOL_MASK)
#define XCVR_CTRL_XCVR_CTRL_TGT_PWR_SRC_MASK     (0x70U)
#define XCVR_CTRL_XCVR_CTRL_TGT_PWR_SRC_SHIFT    (4U)
#define XCVR_CTRL_XCVR_CTRL_TGT_PWR_SRC(x)       (((uint32_t)(((uint32_t)(x)) << XCVR_CTRL_XCVR_CTRL_TGT_PWR_SRC_SHIFT)) & XCVR_CTRL_XCVR_CTRL_TGT_PWR_SRC_MASK)
#define XCVR_CTRL_XCVR_CTRL_REF_CLK_FREQ_MASK    (0x300U)
#define XCVR_CTRL_XCVR_CTRL_REF_CLK_FREQ_SHIFT   (8U)
#define XCVR_CTRL_XCVR_CTRL_REF_CLK_FREQ(x)      (((uint32_t)(((uint32_t)(x)) << XCVR_CTRL_XCVR_CTRL_REF_CLK_FREQ_SHIFT)) & XCVR_CTRL_XCVR_CTRL_REF_CLK_FREQ_MASK)
#define XCVR_CTRL_XCVR_CTRL_SOC_RF_OSC_CLK_GATE_EN_MASK (0x800U)
#define XCVR_CTRL_XCVR_CTRL_SOC_RF_OSC_CLK_GATE_EN_SHIFT (11U)
#define XCVR_CTRL_XCVR_CTRL_SOC_RF_OSC_CLK_GATE_EN(x) (((uint32_t)(((uint32_t)(x)) << XCVR_CTRL_XCVR_CTRL_SOC_RF_OSC_CLK_GATE_EN_SHIFT)) & XCVR_CTRL_XCVR_CTRL_SOC_RF_OSC_CLK_GATE_EN_MASK)
#define XCVR_CTRL_XCVR_CTRL_DEMOD_SEL_MASK       (0x3000U)
#define XCVR_CTRL_XCVR_CTRL_DEMOD_SEL_SHIFT      (12U)
#define XCVR_CTRL_XCVR_CTRL_DEMOD_SEL(x)         (((uint32_t)(((uint32_t)(x)) << XCVR_CTRL_XCVR_CTRL_DEMOD_SEL_SHIFT)) & XCVR_CTRL_XCVR_CTRL_DEMOD_SEL_MASK)
#define XCVR_CTRL_XCVR_CTRL_RADIO0_IRQ_SEL_MASK  (0x70000U)
#define XCVR_CTRL_XCVR_CTRL_RADIO0_IRQ_SEL_SHIFT (16U)
#define XCVR_CTRL_XCVR_CTRL_RADIO0_IRQ_SEL(x)    (((uint32_t)(((uint32_t)(x)) << XCVR_CTRL_XCVR_CTRL_RADIO0_IRQ_SEL_SHIFT)) & XCVR_CTRL_XCVR_CTRL_RADIO0_IRQ_SEL_MASK)
#define XCVR_CTRL_XCVR_CTRL_RADIO1_IRQ_SEL_MASK  (0x700000U)
#define XCVR_CTRL_XCVR_CTRL_RADIO1_IRQ_SEL_SHIFT (20U)
#define XCVR_CTRL_XCVR_CTRL_RADIO1_IRQ_SEL(x)    (((uint32_t)(((uint32_t)(x)) << XCVR_CTRL_XCVR_CTRL_RADIO1_IRQ_SEL_SHIFT)) & XCVR_CTRL_XCVR_CTRL_RADIO1_IRQ_SEL_MASK)

/*! @name XCVR_STATUS - TRANSCEIVER STATUS */
#define XCVR_CTRL_XCVR_STATUS_TSM_COUNT_MASK     (0xFFU)
#define XCVR_CTRL_XCVR_STATUS_TSM_COUNT_SHIFT    (0U)
#define XCVR_CTRL_XCVR_STATUS_TSM_COUNT(x)       (((uint32_t)(((uint32_t)(x)) << XCVR_CTRL_XCVR_STATUS_TSM_COUNT_SHIFT)) & XCVR_CTRL_XCVR_STATUS_TSM_COUNT_MASK)
#define XCVR_CTRL_XCVR_STATUS_PLL_SEQ_STATE_MASK (0xF00U)
#define XCVR_CTRL_XCVR_STATUS_PLL_SEQ_STATE_SHIFT (8U)
#define XCVR_CTRL_XCVR_STATUS_PLL_SEQ_STATE(x)   (((uint32_t)(((uint32_t)(x)) << XCVR_CTRL_XCVR_STATUS_PLL_SEQ_STATE_SHIFT)) & XCVR_CTRL_XCVR_STATUS_PLL_SEQ_STATE_MASK)
#define XCVR_CTRL_XCVR_STATUS_RX_MODE_MASK       (0x1000U)
#define XCVR_CTRL_XCVR_STATUS_RX_MODE_SHIFT      (12U)
#define XCVR_CTRL_XCVR_STATUS_RX_MODE(x)         (((uint32_t)(((uint32_t)(x)) << XCVR_CTRL_XCVR_STATUS_RX_MODE_SHIFT)) & XCVR_CTRL_XCVR_STATUS_RX_MODE_MASK)
#define XCVR_CTRL_XCVR_STATUS_TX_MODE_MASK       (0x2000U)
#define XCVR_CTRL_XCVR_STATUS_TX_MODE_SHIFT      (13U)
#define XCVR_CTRL_XCVR_STATUS_TX_MODE(x)         (((uint32_t)(((uint32_t)(x)) << XCVR_CTRL_XCVR_STATUS_TX_MODE_SHIFT)) & XCVR_CTRL_XCVR_STATUS_TX_MODE_MASK)
#define XCVR_CTRL_XCVR_STATUS_BTLE_SYSCLK_REQ_MASK (0x10000U)
#define XCVR_CTRL_XCVR_STATUS_BTLE_SYSCLK_REQ_SHIFT (16U)
#define XCVR_CTRL_XCVR_STATUS_BTLE_SYSCLK_REQ(x) (((uint32_t)(((uint32_t)(x)) << XCVR_CTRL_XCVR_STATUS_BTLE_SYSCLK_REQ_SHIFT)) & XCVR_CTRL_XCVR_STATUS_BTLE_SYSCLK_REQ_MASK)
#define XCVR_CTRL_XCVR_STATUS_RIF_LL_ACTIVE_MASK (0x20000U)
#define XCVR_CTRL_XCVR_STATUS_RIF_LL_ACTIVE_SHIFT (17U)
#define XCVR_CTRL_XCVR_STATUS_RIF_LL_ACTIVE(x)   (((uint32_t)(((uint32_t)(x)) << XCVR_CTRL_XCVR_STATUS_RIF_LL_ACTIVE_SHIFT)) & XCVR_CTRL_XCVR_STATUS_RIF_LL_ACTIVE_MASK)
#define XCVR_CTRL_XCVR_STATUS_XTAL_READY_MASK    (0x40000U)
#define XCVR_CTRL_XCVR_STATUS_XTAL_READY_SHIFT   (18U)
#define XCVR_CTRL_XCVR_STATUS_XTAL_READY(x)      (((uint32_t)(((uint32_t)(x)) << XCVR_CTRL_XCVR_STATUS_XTAL_READY_SHIFT)) & XCVR_CTRL_XCVR_STATUS_XTAL_READY_MASK)
#define XCVR_CTRL_XCVR_STATUS_SOC_USING_RF_OSC_CLK_MASK (0x80000U)
#define XCVR_CTRL_XCVR_STATUS_SOC_USING_RF_OSC_CLK_SHIFT (19U)
#define XCVR_CTRL_XCVR_STATUS_SOC_USING_RF_OSC_CLK(x) (((uint32_t)(((uint32_t)(x)) << XCVR_CTRL_XCVR_STATUS_SOC_USING_RF_OSC_CLK_SHIFT)) & XCVR_CTRL_XCVR_STATUS_SOC_USING_RF_OSC_CLK_MASK)
#define XCVR_CTRL_XCVR_STATUS_TSM_IRQ0_MASK      (0x1000000U)
#define XCVR_CTRL_XCVR_STATUS_TSM_IRQ0_SHIFT     (24U)
#define XCVR_CTRL_XCVR_STATUS_TSM_IRQ0(x)        (((uint32_t)(((uint32_t)(x)) << XCVR_CTRL_XCVR_STATUS_TSM_IRQ0_SHIFT)) & XCVR_CTRL_XCVR_STATUS_TSM_IRQ0_MASK)
#define XCVR_CTRL_XCVR_STATUS_TSM_IRQ1_MASK      (0x2000000U)
#define XCVR_CTRL_XCVR_STATUS_TSM_IRQ1_SHIFT     (25U)
#define XCVR_CTRL_XCVR_STATUS_TSM_IRQ1(x)        (((uint32_t)(((uint32_t)(x)) << XCVR_CTRL_XCVR_STATUS_TSM_IRQ1_SHIFT)) & XCVR_CTRL_XCVR_STATUS_TSM_IRQ1_MASK)

/*! @name BLE_ARB_CTRL - BLE ARBITRATION CONTROL */
#define XCVR_CTRL_BLE_ARB_CTRL_BLE_RELINQUISH_MASK (0x1U)
#define XCVR_CTRL_BLE_ARB_CTRL_BLE_RELINQUISH_SHIFT (0U)
#define XCVR_CTRL_BLE_ARB_CTRL_BLE_RELINQUISH(x) (((uint32_t)(((uint32_t)(x)) << XCVR_CTRL_BLE_ARB_CTRL_BLE_RELINQUISH_SHIFT)) & XCVR_CTRL_BLE_ARB_CTRL_BLE_RELINQUISH_MASK)
#define XCVR_CTRL_BLE_ARB_CTRL_XCVR_BUSY_MASK    (0x2U)
#define XCVR_CTRL_BLE_ARB_CTRL_XCVR_BUSY_SHIFT   (1U)
#define XCVR_CTRL_BLE_ARB_CTRL_XCVR_BUSY(x)      (((uint32_t)(((uint32_t)(x)) << XCVR_CTRL_BLE_ARB_CTRL_XCVR_BUSY_SHIFT)) & XCVR_CTRL_BLE_ARB_CTRL_XCVR_BUSY_MASK)

/*! @name OVERWRITE_VER - OVERWRITE VERSION */
#define XCVR_CTRL_OVERWRITE_VER_OVERWRITE_VER_MASK (0xFFU)
#define XCVR_CTRL_OVERWRITE_VER_OVERWRITE_VER_SHIFT (0U)
#define XCVR_CTRL_OVERWRITE_VER_OVERWRITE_VER(x) (((uint32_t)(((uint32_t)(x)) << XCVR_CTRL_OVERWRITE_VER_OVERWRITE_VER_SHIFT)) & XCVR_CTRL_OVERWRITE_VER_OVERWRITE_VER_MASK)

/*! @name DMA_CTRL - TRANSCEIVER DMA CONTROL */
#define XCVR_CTRL_DMA_CTRL_DMA_PAGE_MASK         (0xFU)
#define XCVR_CTRL_DMA_CTRL_DMA_PAGE_SHIFT        (0U)
#define XCVR_CTRL_DMA_CTRL_DMA_PAGE(x)           (((uint32_t)(((uint32_t)(x)) << XCVR_CTRL_DMA_CTRL_DMA_PAGE_SHIFT)) & XCVR_CTRL_DMA_CTRL_DMA_PAGE_MASK)
#define XCVR_CTRL_DMA_CTRL_SINGLE_REQ_MODE_MASK  (0x10U)
#define XCVR_CTRL_DMA_CTRL_SINGLE_REQ_MODE_SHIFT (4U)
#define XCVR_CTRL_DMA_CTRL_SINGLE_REQ_MODE(x)    (((uint32_t)(((uint32_t)(x)) << XCVR_CTRL_DMA_CTRL_SINGLE_REQ_MODE_SHIFT)) & XCVR_CTRL_DMA_CTRL_SINGLE_REQ_MODE_MASK)
#define XCVR_CTRL_DMA_CTRL_BYPASS_DMA_SYNC_MASK  (0x20U)
#define XCVR_CTRL_DMA_CTRL_BYPASS_DMA_SYNC_SHIFT (5U)
#define XCVR_CTRL_DMA_CTRL_BYPASS_DMA_SYNC(x)    (((uint32_t)(((uint32_t)(x)) << XCVR_CTRL_DMA_CTRL_BYPASS_DMA_SYNC_SHIFT)) & XCVR_CTRL_DMA_CTRL_BYPASS_DMA_SYNC_MASK)
#define XCVR_CTRL_DMA_CTRL_DMA_TRIGGERRED_MASK   (0x40U)
#define XCVR_CTRL_DMA_CTRL_DMA_TRIGGERRED_SHIFT  (6U)
#define XCVR_CTRL_DMA_CTRL_DMA_TRIGGERRED(x)     (((uint32_t)(((uint32_t)(x)) << XCVR_CTRL_DMA_CTRL_DMA_TRIGGERRED_SHIFT)) & XCVR_CTRL_DMA_CTRL_DMA_TRIGGERRED_MASK)
#define XCVR_CTRL_DMA_CTRL_DMA_TIMED_OUT_MASK    (0x80U)
#define XCVR_CTRL_DMA_CTRL_DMA_TIMED_OUT_SHIFT   (7U)
#define XCVR_CTRL_DMA_CTRL_DMA_TIMED_OUT(x)      (((uint32_t)(((uint32_t)(x)) << XCVR_CTRL_DMA_CTRL_DMA_TIMED_OUT_SHIFT)) & XCVR_CTRL_DMA_CTRL_DMA_TIMED_OUT_MASK)
#define XCVR_CTRL_DMA_CTRL_DMA_TIMEOUT_MASK      (0xF00U)
#define XCVR_CTRL_DMA_CTRL_DMA_TIMEOUT_SHIFT     (8U)
#define XCVR_CTRL_DMA_CTRL_DMA_TIMEOUT(x)        (((uint32_t)(((uint32_t)(x)) << XCVR_CTRL_DMA_CTRL_DMA_TIMEOUT_SHIFT)) & XCVR_CTRL_DMA_CTRL_DMA_TIMEOUT_MASK)

/*! @name DMA_DATA - TRANSCEIVER DMA DATA */
#define XCVR_CTRL_DMA_DATA_DMA_DATA_MASK         (0xFFFFFFFFU)
#define XCVR_CTRL_DMA_DATA_DMA_DATA_SHIFT        (0U)
#define XCVR_CTRL_DMA_DATA_DMA_DATA(x)           (((uint32_t)(((uint32_t)(x)) << XCVR_CTRL_DMA_DATA_DMA_DATA_SHIFT)) & XCVR_CTRL_DMA_DATA_DMA_DATA_MASK)

/*! @name DTEST_CTRL - DIGITAL TEST MUX CONTROL */
#define XCVR_CTRL_DTEST_CTRL_DTEST_PAGE_MASK     (0x3FU)
#define XCVR_CTRL_DTEST_CTRL_DTEST_PAGE_SHIFT    (0U)
#define XCVR_CTRL_DTEST_CTRL_DTEST_PAGE(x)       (((uint32_t)(((uint32_t)(x)) << XCVR_CTRL_DTEST_CTRL_DTEST_PAGE_SHIFT)) & XCVR_CTRL_DTEST_CTRL_DTEST_PAGE_MASK)
#define XCVR_CTRL_DTEST_CTRL_DTEST_EN_MASK       (0x80U)
#define XCVR_CTRL_DTEST_CTRL_DTEST_EN_SHIFT      (7U)
#define XCVR_CTRL_DTEST_CTRL_DTEST_EN(x)         (((uint32_t)(((uint32_t)(x)) << XCVR_CTRL_DTEST_CTRL_DTEST_EN_SHIFT)) & XCVR_CTRL_DTEST_CTRL_DTEST_EN_MASK)
#define XCVR_CTRL_DTEST_CTRL_GPIO0_OVLAY_PIN_MASK (0xF00U)
#define XCVR_CTRL_DTEST_CTRL_GPIO0_OVLAY_PIN_SHIFT (8U)
#define XCVR_CTRL_DTEST_CTRL_GPIO0_OVLAY_PIN(x)  (((uint32_t)(((uint32_t)(x)) << XCVR_CTRL_DTEST_CTRL_GPIO0_OVLAY_PIN_SHIFT)) & XCVR_CTRL_DTEST_CTRL_GPIO0_OVLAY_PIN_MASK)
#define XCVR_CTRL_DTEST_CTRL_GPIO1_OVLAY_PIN_MASK (0xF000U)
#define XCVR_CTRL_DTEST_CTRL_GPIO1_OVLAY_PIN_SHIFT (12U)
#define XCVR_CTRL_DTEST_CTRL_GPIO1_OVLAY_PIN(x)  (((uint32_t)(((uint32_t)(x)) << XCVR_CTRL_DTEST_CTRL_GPIO1_OVLAY_PIN_SHIFT)) & XCVR_CTRL_DTEST_CTRL_GPIO1_OVLAY_PIN_MASK)
#define XCVR_CTRL_DTEST_CTRL_TSM_GPIO_OVLAY_MASK (0x30000U)
#define XCVR_CTRL_DTEST_CTRL_TSM_GPIO_OVLAY_SHIFT (16U)
#define XCVR_CTRL_DTEST_CTRL_TSM_GPIO_OVLAY(x)   (((uint32_t)(((uint32_t)(x)) << XCVR_CTRL_DTEST_CTRL_TSM_GPIO_OVLAY_SHIFT)) & XCVR_CTRL_DTEST_CTRL_TSM_GPIO_OVLAY_MASK)
#define XCVR_CTRL_DTEST_CTRL_DTEST_SHFT_MASK     (0x7000000U)
#define XCVR_CTRL_DTEST_CTRL_DTEST_SHFT_SHIFT    (24U)
#define XCVR_CTRL_DTEST_CTRL_DTEST_SHFT(x)       (((uint32_t)(((uint32_t)(x)) << XCVR_CTRL_DTEST_CTRL_DTEST_SHFT_SHIFT)) & XCVR_CTRL_DTEST_CTRL_DTEST_SHFT_MASK)
#define XCVR_CTRL_DTEST_CTRL_RAW_MODE_I_MASK     (0x10000000U)
#define XCVR_CTRL_DTEST_CTRL_RAW_MODE_I_SHIFT    (28U)
#define XCVR_CTRL_DTEST_CTRL_RAW_MODE_I(x)       (((uint32_t)(((uint32_t)(x)) << XCVR_CTRL_DTEST_CTRL_RAW_MODE_I_SHIFT)) & XCVR_CTRL_DTEST_CTRL_RAW_MODE_I_MASK)
#define XCVR_CTRL_DTEST_CTRL_RAW_MODE_Q_MASK     (0x20000000U)
#define XCVR_CTRL_DTEST_CTRL_RAW_MODE_Q_SHIFT    (29U)
#define XCVR_CTRL_DTEST_CTRL_RAW_MODE_Q(x)       (((uint32_t)(((uint32_t)(x)) << XCVR_CTRL_DTEST_CTRL_RAW_MODE_Q_SHIFT)) & XCVR_CTRL_DTEST_CTRL_RAW_MODE_Q_MASK)

/*! @name PACKET_RAM_CTRL - PACKET RAM CONTROL */
#define XCVR_CTRL_PACKET_RAM_CTRL_DBG_PAGE_MASK  (0xFU)
#define XCVR_CTRL_PACKET_RAM_CTRL_DBG_PAGE_SHIFT (0U)
#define XCVR_CTRL_PACKET_RAM_CTRL_DBG_PAGE(x)    (((uint32_t)(((uint32_t)(x)) << XCVR_CTRL_PACKET_RAM_CTRL_DBG_PAGE_SHIFT)) & XCVR_CTRL_PACKET_RAM_CTRL_DBG_PAGE_MASK)
#define XCVR_CTRL_PACKET_RAM_CTRL_PB_PROTECT_MASK (0x10U)
#define XCVR_CTRL_PACKET_RAM_CTRL_PB_PROTECT_SHIFT (4U)
#define XCVR_CTRL_PACKET_RAM_CTRL_PB_PROTECT(x)  (((uint32_t)(((uint32_t)(x)) << XCVR_CTRL_PACKET_RAM_CTRL_PB_PROTECT_SHIFT)) & XCVR_CTRL_PACKET_RAM_CTRL_PB_PROTECT_MASK)
#define XCVR_CTRL_PACKET_RAM_CTRL_XCVR_RAM_ALLOW_MASK (0x20U)
#define XCVR_CTRL_PACKET_RAM_CTRL_XCVR_RAM_ALLOW_SHIFT (5U)
#define XCVR_CTRL_PACKET_RAM_CTRL_XCVR_RAM_ALLOW(x) (((uint32_t)(((uint32_t)(x)) << XCVR_CTRL_PACKET_RAM_CTRL_XCVR_RAM_ALLOW_SHIFT)) & XCVR_CTRL_PACKET_RAM_CTRL_XCVR_RAM_ALLOW_MASK)
#define XCVR_CTRL_PACKET_RAM_CTRL_ALL_PROTOCOLS_ALLOW_MASK (0x40U)
#define XCVR_CTRL_PACKET_RAM_CTRL_ALL_PROTOCOLS_ALLOW_SHIFT (6U)
#define XCVR_CTRL_PACKET_RAM_CTRL_ALL_PROTOCOLS_ALLOW(x) (((uint32_t)(((uint32_t)(x)) << XCVR_CTRL_PACKET_RAM_CTRL_ALL_PROTOCOLS_ALLOW_SHIFT)) & XCVR_CTRL_PACKET_RAM_CTRL_ALL_PROTOCOLS_ALLOW_MASK)
#define XCVR_CTRL_PACKET_RAM_CTRL_DBG_TRIGGERRED_MASK (0x80U)
#define XCVR_CTRL_PACKET_RAM_CTRL_DBG_TRIGGERRED_SHIFT (7U)
#define XCVR_CTRL_PACKET_RAM_CTRL_DBG_TRIGGERRED(x) (((uint32_t)(((uint32_t)(x)) << XCVR_CTRL_PACKET_RAM_CTRL_DBG_TRIGGERRED_SHIFT)) & XCVR_CTRL_PACKET_RAM_CTRL_DBG_TRIGGERRED_MASK)
#define XCVR_CTRL_PACKET_RAM_CTRL_DBG_RAM_FULL_MASK (0x300U)
#define XCVR_CTRL_PACKET_RAM_CTRL_DBG_RAM_FULL_SHIFT (8U)
#define XCVR_CTRL_PACKET_RAM_CTRL_DBG_RAM_FULL(x) (((uint32_t)(((uint32_t)(x)) << XCVR_CTRL_PACKET_RAM_CTRL_DBG_RAM_FULL_SHIFT)) & XCVR_CTRL_PACKET_RAM_CTRL_DBG_RAM_FULL_MASK)
#define XCVR_CTRL_PACKET_RAM_CTRL_RAM0_CLK_ON_OVRD_EN_MASK (0x400U)
#define XCVR_CTRL_PACKET_RAM_CTRL_RAM0_CLK_ON_OVRD_EN_SHIFT (10U)
#define XCVR_CTRL_PACKET_RAM_CTRL_RAM0_CLK_ON_OVRD_EN(x) (((uint32_t)(((uint32_t)(x)) << XCVR_CTRL_PACKET_RAM_CTRL_RAM0_CLK_ON_OVRD_EN_SHIFT)) & XCVR_CTRL_PACKET_RAM_CTRL_RAM0_CLK_ON_OVRD_EN_MASK)
#define XCVR_CTRL_PACKET_RAM_CTRL_RAM0_CLK_ON_OVRD_MASK (0x800U)
#define XCVR_CTRL_PACKET_RAM_CTRL_RAM0_CLK_ON_OVRD_SHIFT (11U)
#define XCVR_CTRL_PACKET_RAM_CTRL_RAM0_CLK_ON_OVRD(x) (((uint32_t)(((uint32_t)(x)) << XCVR_CTRL_PACKET_RAM_CTRL_RAM0_CLK_ON_OVRD_SHIFT)) & XCVR_CTRL_PACKET_RAM_CTRL_RAM0_CLK_ON_OVRD_MASK)
#define XCVR_CTRL_PACKET_RAM_CTRL_RAM1_CLK_ON_OVRD_EN_MASK (0x1000U)
#define XCVR_CTRL_PACKET_RAM_CTRL_RAM1_CLK_ON_OVRD_EN_SHIFT (12U)
#define XCVR_CTRL_PACKET_RAM_CTRL_RAM1_CLK_ON_OVRD_EN(x) (((uint32_t)(((uint32_t)(x)) << XCVR_CTRL_PACKET_RAM_CTRL_RAM1_CLK_ON_OVRD_EN_SHIFT)) & XCVR_CTRL_PACKET_RAM_CTRL_RAM1_CLK_ON_OVRD_EN_MASK)
#define XCVR_CTRL_PACKET_RAM_CTRL_RAM1_CLK_ON_OVRD_MASK (0x2000U)
#define XCVR_CTRL_PACKET_RAM_CTRL_RAM1_CLK_ON_OVRD_SHIFT (13U)
#define XCVR_CTRL_PACKET_RAM_CTRL_RAM1_CLK_ON_OVRD(x) (((uint32_t)(((uint32_t)(x)) << XCVR_CTRL_PACKET_RAM_CTRL_RAM1_CLK_ON_OVRD_SHIFT)) & XCVR_CTRL_PACKET_RAM_CTRL_RAM1_CLK_ON_OVRD_MASK)
#define XCVR_CTRL_PACKET_RAM_CTRL_RAM0_CE_ON_OVRD_EN_MASK (0x4000U)
#define XCVR_CTRL_PACKET_RAM_CTRL_RAM0_CE_ON_OVRD_EN_SHIFT (14U)
#define XCVR_CTRL_PACKET_RAM_CTRL_RAM0_CE_ON_OVRD_EN(x) (((uint32_t)(((uint32_t)(x)) << XCVR_CTRL_PACKET_RAM_CTRL_RAM0_CE_ON_OVRD_EN_SHIFT)) & XCVR_CTRL_PACKET_RAM_CTRL_RAM0_CE_ON_OVRD_EN_MASK)
#define XCVR_CTRL_PACKET_RAM_CTRL_RAM0_CE_ON_OVRD_MASK (0x8000U)
#define XCVR_CTRL_PACKET_RAM_CTRL_RAM0_CE_ON_OVRD_SHIFT (15U)
#define XCVR_CTRL_PACKET_RAM_CTRL_RAM0_CE_ON_OVRD(x) (((uint32_t)(((uint32_t)(x)) << XCVR_CTRL_PACKET_RAM_CTRL_RAM0_CE_ON_OVRD_SHIFT)) & XCVR_CTRL_PACKET_RAM_CTRL_RAM0_CE_ON_OVRD_MASK)
#define XCVR_CTRL_PACKET_RAM_CTRL_RAM1_CE_ON_OVRD_EN_MASK (0x10000U)
#define XCVR_CTRL_PACKET_RAM_CTRL_RAM1_CE_ON_OVRD_EN_SHIFT (16U)
#define XCVR_CTRL_PACKET_RAM_CTRL_RAM1_CE_ON_OVRD_EN(x) (((uint32_t)(((uint32_t)(x)) << XCVR_CTRL_PACKET_RAM_CTRL_RAM1_CE_ON_OVRD_EN_SHIFT)) & XCVR_CTRL_PACKET_RAM_CTRL_RAM1_CE_ON_OVRD_EN_MASK)
#define XCVR_CTRL_PACKET_RAM_CTRL_RAM1_CE_ON_OVRD_MASK (0x20000U)
#define XCVR_CTRL_PACKET_RAM_CTRL_RAM1_CE_ON_OVRD_SHIFT (17U)
#define XCVR_CTRL_PACKET_RAM_CTRL_RAM1_CE_ON_OVRD(x) (((uint32_t)(((uint32_t)(x)) << XCVR_CTRL_PACKET_RAM_CTRL_RAM1_CE_ON_OVRD_SHIFT)) & XCVR_CTRL_PACKET_RAM_CTRL_RAM1_CE_ON_OVRD_MASK)

/*! @name FAD_CTRL - FAD CONTROL */
#define XCVR_CTRL_FAD_CTRL_FAD_EN_MASK           (0x1U)
#define XCVR_CTRL_FAD_CTRL_FAD_EN_SHIFT          (0U)
#define XCVR_CTRL_FAD_CTRL_FAD_EN(x)             (((uint32_t)(((uint32_t)(x)) << XCVR_CTRL_FAD_CTRL_FAD_EN_SHIFT)) & XCVR_CTRL_FAD_CTRL_FAD_EN_MASK)
#define XCVR_CTRL_FAD_CTRL_ANTX_MASK             (0x2U)
#define XCVR_CTRL_FAD_CTRL_ANTX_SHIFT            (1U)
#define XCVR_CTRL_FAD_CTRL_ANTX(x)               (((uint32_t)(((uint32_t)(x)) << XCVR_CTRL_FAD_CTRL_ANTX_SHIFT)) & XCVR_CTRL_FAD_CTRL_ANTX_MASK)
#define XCVR_CTRL_FAD_CTRL_ANTX_EN_MASK          (0x30U)
#define XCVR_CTRL_FAD_CTRL_ANTX_EN_SHIFT         (4U)
#define XCVR_CTRL_FAD_CTRL_ANTX_EN(x)            (((uint32_t)(((uint32_t)(x)) << XCVR_CTRL_FAD_CTRL_ANTX_EN_SHIFT)) & XCVR_CTRL_FAD_CTRL_ANTX_EN_MASK)
#define XCVR_CTRL_FAD_CTRL_ANTX_HZ_MASK          (0x40U)
#define XCVR_CTRL_FAD_CTRL_ANTX_HZ_SHIFT         (6U)
#define XCVR_CTRL_FAD_CTRL_ANTX_HZ(x)            (((uint32_t)(((uint32_t)(x)) << XCVR_CTRL_FAD_CTRL_ANTX_HZ_SHIFT)) & XCVR_CTRL_FAD_CTRL_ANTX_HZ_MASK)
#define XCVR_CTRL_FAD_CTRL_ANTX_CTRLMODE_MASK    (0x80U)
#define XCVR_CTRL_FAD_CTRL_ANTX_CTRLMODE_SHIFT   (7U)
#define XCVR_CTRL_FAD_CTRL_ANTX_CTRLMODE(x)      (((uint32_t)(((uint32_t)(x)) << XCVR_CTRL_FAD_CTRL_ANTX_CTRLMODE_SHIFT)) & XCVR_CTRL_FAD_CTRL_ANTX_CTRLMODE_MASK)
#define XCVR_CTRL_FAD_CTRL_ANTX_POL_MASK         (0xF00U)
#define XCVR_CTRL_FAD_CTRL_ANTX_POL_SHIFT        (8U)
#define XCVR_CTRL_FAD_CTRL_ANTX_POL(x)           (((uint32_t)(((uint32_t)(x)) << XCVR_CTRL_FAD_CTRL_ANTX_POL_SHIFT)) & XCVR_CTRL_FAD_CTRL_ANTX_POL_MASK)
#define XCVR_CTRL_FAD_CTRL_FAD_NOT_GPIO_MASK     (0xF000U)
#define XCVR_CTRL_FAD_CTRL_FAD_NOT_GPIO_SHIFT    (12U)
#define XCVR_CTRL_FAD_CTRL_FAD_NOT_GPIO(x)       (((uint32_t)(((uint32_t)(x)) << XCVR_CTRL_FAD_CTRL_FAD_NOT_GPIO_SHIFT)) & XCVR_CTRL_FAD_CTRL_FAD_NOT_GPIO_MASK)

/*! @name LPPS_CTRL - LOW POWER PREAMBLE SEARCH CONTROL */
#define XCVR_CTRL_LPPS_CTRL_LPPS_ENABLE_MASK     (0x1U)
#define XCVR_CTRL_LPPS_CTRL_LPPS_ENABLE_SHIFT    (0U)
#define XCVR_CTRL_LPPS_CTRL_LPPS_ENABLE(x)       (((uint32_t)(((uint32_t)(x)) << XCVR_CTRL_LPPS_CTRL_LPPS_ENABLE_SHIFT)) & XCVR_CTRL_LPPS_CTRL_LPPS_ENABLE_MASK)
#define XCVR_CTRL_LPPS_CTRL_LPPS_TZA_ALLOW_MASK  (0x2U)
#define XCVR_CTRL_LPPS_CTRL_LPPS_TZA_ALLOW_SHIFT (1U)
#define XCVR_CTRL_LPPS_CTRL_LPPS_TZA_ALLOW(x)    (((uint32_t)(((uint32_t)(x)) << XCVR_CTRL_LPPS_CTRL_LPPS_TZA_ALLOW_SHIFT)) & XCVR_CTRL_LPPS_CTRL_LPPS_TZA_ALLOW_MASK)
#define XCVR_CTRL_LPPS_CTRL_LPPS_BBA_ALLOW_MASK  (0x4U)
#define XCVR_CTRL_LPPS_CTRL_LPPS_BBA_ALLOW_SHIFT (2U)
#define XCVR_CTRL_LPPS_CTRL_LPPS_BBA_ALLOW(x)    (((uint32_t)(((uint32_t)(x)) << XCVR_CTRL_LPPS_CTRL_LPPS_BBA_ALLOW_SHIFT)) & XCVR_CTRL_LPPS_CTRL_LPPS_BBA_ALLOW_MASK)
#define XCVR_CTRL_LPPS_CTRL_LPPS_ADC_ALLOW_MASK  (0x8U)
#define XCVR_CTRL_LPPS_CTRL_LPPS_ADC_ALLOW_SHIFT (3U)
#define XCVR_CTRL_LPPS_CTRL_LPPS_ADC_ALLOW(x)    (((uint32_t)(((uint32_t)(x)) << XCVR_CTRL_LPPS_CTRL_LPPS_ADC_ALLOW_SHIFT)) & XCVR_CTRL_LPPS_CTRL_LPPS_ADC_ALLOW_MASK)
#define XCVR_CTRL_LPPS_CTRL_LPPS_DCOC_ALLOW_MASK (0x10U)
#define XCVR_CTRL_LPPS_CTRL_LPPS_DCOC_ALLOW_SHIFT (4U)
#define XCVR_CTRL_LPPS_CTRL_LPPS_DCOC_ALLOW(x)   (((uint32_t)(((uint32_t)(x)) << XCVR_CTRL_LPPS_CTRL_LPPS_DCOC_ALLOW_SHIFT)) & XCVR_CTRL_LPPS_CTRL_LPPS_DCOC_ALLOW_MASK)
#define XCVR_CTRL_LPPS_CTRL_LPPS_PDET_ALLOW_MASK (0x20U)
#define XCVR_CTRL_LPPS_CTRL_LPPS_PDET_ALLOW_SHIFT (5U)
#define XCVR_CTRL_LPPS_CTRL_LPPS_PDET_ALLOW(x)   (((uint32_t)(((uint32_t)(x)) << XCVR_CTRL_LPPS_CTRL_LPPS_PDET_ALLOW_SHIFT)) & XCVR_CTRL_LPPS_CTRL_LPPS_PDET_ALLOW_MASK)
#define XCVR_CTRL_LPPS_CTRL_LPPS_SY_LO_ALLOW_MASK (0x40U)
#define XCVR_CTRL_LPPS_CTRL_LPPS_SY_LO_ALLOW_SHIFT (6U)
#define XCVR_CTRL_LPPS_CTRL_LPPS_SY_LO_ALLOW(x)  (((uint32_t)(((uint32_t)(x)) << XCVR_CTRL_LPPS_CTRL_LPPS_SY_LO_ALLOW_SHIFT)) & XCVR_CTRL_LPPS_CTRL_LPPS_SY_LO_ALLOW_MASK)
#define XCVR_CTRL_LPPS_CTRL_LPPS_SY_LO_BUF_ALLOW_MASK (0x80U)
#define XCVR_CTRL_LPPS_CTRL_LPPS_SY_LO_BUF_ALLOW_SHIFT (7U)
#define XCVR_CTRL_LPPS_CTRL_LPPS_SY_LO_BUF_ALLOW(x) (((uint32_t)(((uint32_t)(x)) << XCVR_CTRL_LPPS_CTRL_LPPS_SY_LO_BUF_ALLOW_SHIFT)) & XCVR_CTRL_LPPS_CTRL_LPPS_SY_LO_BUF_ALLOW_MASK)
#define XCVR_CTRL_LPPS_CTRL_LPPS_RX_DIG_ALLOW_MASK (0x100U)
#define XCVR_CTRL_LPPS_CTRL_LPPS_RX_DIG_ALLOW_SHIFT (8U)
#define XCVR_CTRL_LPPS_CTRL_LPPS_RX_DIG_ALLOW(x) (((uint32_t)(((uint32_t)(x)) << XCVR_CTRL_LPPS_CTRL_LPPS_RX_DIG_ALLOW_SHIFT)) & XCVR_CTRL_LPPS_CTRL_LPPS_RX_DIG_ALLOW_MASK)
#define XCVR_CTRL_LPPS_CTRL_LPPS_DCOC_DIG_ALLOW_MASK (0x200U)
#define XCVR_CTRL_LPPS_CTRL_LPPS_DCOC_DIG_ALLOW_SHIFT (9U)
#define XCVR_CTRL_LPPS_CTRL_LPPS_DCOC_DIG_ALLOW(x) (((uint32_t)(((uint32_t)(x)) << XCVR_CTRL_LPPS_CTRL_LPPS_DCOC_DIG_ALLOW_SHIFT)) & XCVR_CTRL_LPPS_CTRL_LPPS_DCOC_DIG_ALLOW_MASK)
#define XCVR_CTRL_LPPS_CTRL_LPPS_START_RX_MASK   (0xFF0000U)
#define XCVR_CTRL_LPPS_CTRL_LPPS_START_RX_SHIFT  (16U)
#define XCVR_CTRL_LPPS_CTRL_LPPS_START_RX(x)     (((uint32_t)(((uint32_t)(x)) << XCVR_CTRL_LPPS_CTRL_LPPS_START_RX_SHIFT)) & XCVR_CTRL_LPPS_CTRL_LPPS_START_RX_MASK)
#define XCVR_CTRL_LPPS_CTRL_LPPS_DEST_RX_MASK    (0xFF000000U)
#define XCVR_CTRL_LPPS_CTRL_LPPS_DEST_RX_SHIFT   (24U)
#define XCVR_CTRL_LPPS_CTRL_LPPS_DEST_RX(x)      (((uint32_t)(((uint32_t)(x)) << XCVR_CTRL_LPPS_CTRL_LPPS_DEST_RX_SHIFT)) & XCVR_CTRL_LPPS_CTRL_LPPS_DEST_RX_MASK)

/*! @name RF_NOT_ALLOWED_CTRL - WIFI COEXISTENCE CONTROL */
#define XCVR_CTRL_RF_NOT_ALLOWED_CTRL_RF_NOT_ALLOWED_NO_TX_MASK (0x1U)
#define XCVR_CTRL_RF_NOT_ALLOWED_CTRL_RF_NOT_ALLOWED_NO_TX_SHIFT (0U)
#define XCVR_CTRL_RF_NOT_ALLOWED_CTRL_RF_NOT_ALLOWED_NO_TX(x) (((uint32_t)(((uint32_t)(x)) << XCVR_CTRL_RF_NOT_ALLOWED_CTRL_RF_NOT_ALLOWED_NO_TX_SHIFT)) & XCVR_CTRL_RF_NOT_ALLOWED_CTRL_RF_NOT_ALLOWED_NO_TX_MASK)
#define XCVR_CTRL_RF_NOT_ALLOWED_CTRL_RF_NOT_ALLOWED_NO_RX_MASK (0x2U)
#define XCVR_CTRL_RF_NOT_ALLOWED_CTRL_RF_NOT_ALLOWED_NO_RX_SHIFT (1U)
#define XCVR_CTRL_RF_NOT_ALLOWED_CTRL_RF_NOT_ALLOWED_NO_RX(x) (((uint32_t)(((uint32_t)(x)) << XCVR_CTRL_RF_NOT_ALLOWED_CTRL_RF_NOT_ALLOWED_NO_RX_SHIFT)) & XCVR_CTRL_RF_NOT_ALLOWED_CTRL_RF_NOT_ALLOWED_NO_RX_MASK)
#define XCVR_CTRL_RF_NOT_ALLOWED_CTRL_RF_NOT_ALLOWED_ASSERTED_MASK (0x4U)
#define XCVR_CTRL_RF_NOT_ALLOWED_CTRL_RF_NOT_ALLOWED_ASSERTED_SHIFT (2U)
#define XCVR_CTRL_RF_NOT_ALLOWED_CTRL_RF_NOT_ALLOWED_ASSERTED(x) (((uint32_t)(((uint32_t)(x)) << XCVR_CTRL_RF_NOT_ALLOWED_CTRL_RF_NOT_ALLOWED_ASSERTED_SHIFT)) & XCVR_CTRL_RF_NOT_ALLOWED_CTRL_RF_NOT_ALLOWED_ASSERTED_MASK)
#define XCVR_CTRL_RF_NOT_ALLOWED_CTRL_RF_NOT_ALLOWED_TX_ABORT_MASK (0x8U)
#define XCVR_CTRL_RF_NOT_ALLOWED_CTRL_RF_NOT_ALLOWED_TX_ABORT_SHIFT (3U)
#define XCVR_CTRL_RF_NOT_ALLOWED_CTRL_RF_NOT_ALLOWED_TX_ABORT(x) (((uint32_t)(((uint32_t)(x)) << XCVR_CTRL_RF_NOT_ALLOWED_CTRL_RF_NOT_ALLOWED_TX_ABORT_SHIFT)) & XCVR_CTRL_RF_NOT_ALLOWED_CTRL_RF_NOT_ALLOWED_TX_ABORT_MASK)
#define XCVR_CTRL_RF_NOT_ALLOWED_CTRL_RF_NOT_ALLOWED_RX_ABORT_MASK (0x10U)
#define XCVR_CTRL_RF_NOT_ALLOWED_CTRL_RF_NOT_ALLOWED_RX_ABORT_SHIFT (4U)
#define XCVR_CTRL_RF_NOT_ALLOWED_CTRL_RF_NOT_ALLOWED_RX_ABORT(x) (((uint32_t)(((uint32_t)(x)) << XCVR_CTRL_RF_NOT_ALLOWED_CTRL_RF_NOT_ALLOWED_RX_ABORT_SHIFT)) & XCVR_CTRL_RF_NOT_ALLOWED_CTRL_RF_NOT_ALLOWED_RX_ABORT_MASK)
#define XCVR_CTRL_RF_NOT_ALLOWED_CTRL_RF_NOT_ALLOWED_MASK (0x20U)
#define XCVR_CTRL_RF_NOT_ALLOWED_CTRL_RF_NOT_ALLOWED_SHIFT (5U)
#define XCVR_CTRL_RF_NOT_ALLOWED_CTRL_RF_NOT_ALLOWED(x) (((uint32_t)(((uint32_t)(x)) << XCVR_CTRL_RF_NOT_ALLOWED_CTRL_RF_NOT_ALLOWED_SHIFT)) & XCVR_CTRL_RF_NOT_ALLOWED_CTRL_RF_NOT_ALLOWED_MASK)

/*! @name CRCW_CFG - CRC/WHITENER CONTROL */
#define XCVR_CTRL_CRCW_CFG_CRCW_EN_MASK          (0x1U)
#define XCVR_CTRL_CRCW_CFG_CRCW_EN_SHIFT         (0U)
#define XCVR_CTRL_CRCW_CFG_CRCW_EN(x)            (((uint32_t)(((uint32_t)(x)) << XCVR_CTRL_CRCW_CFG_CRCW_EN_SHIFT)) & XCVR_CTRL_CRCW_CFG_CRCW_EN_MASK)
#define XCVR_CTRL_CRCW_CFG_CRC_ZERO_MASK         (0x2U)
#define XCVR_CTRL_CRCW_CFG_CRC_ZERO_SHIFT        (1U)
#define XCVR_CTRL_CRCW_CFG_CRC_ZERO(x)           (((uint32_t)(((uint32_t)(x)) << XCVR_CTRL_CRCW_CFG_CRC_ZERO_SHIFT)) & XCVR_CTRL_CRCW_CFG_CRC_ZERO_MASK)
#define XCVR_CTRL_CRCW_CFG_CRC_EARLY_FAIL_MASK   (0x4U)
#define XCVR_CTRL_CRCW_CFG_CRC_EARLY_FAIL_SHIFT  (2U)
#define XCVR_CTRL_CRCW_CFG_CRC_EARLY_FAIL(x)     (((uint32_t)(((uint32_t)(x)) << XCVR_CTRL_CRCW_CFG_CRC_EARLY_FAIL_SHIFT)) & XCVR_CTRL_CRCW_CFG_CRC_EARLY_FAIL_MASK)
#define XCVR_CTRL_CRCW_CFG_CRC_RES_OUT_VLD_MASK  (0x8U)
#define XCVR_CTRL_CRCW_CFG_CRC_RES_OUT_VLD_SHIFT (3U)
#define XCVR_CTRL_CRCW_CFG_CRC_RES_OUT_VLD(x)    (((uint32_t)(((uint32_t)(x)) << XCVR_CTRL_CRCW_CFG_CRC_RES_OUT_VLD_SHIFT)) & XCVR_CTRL_CRCW_CFG_CRC_RES_OUT_VLD_MASK)
#define XCVR_CTRL_CRCW_CFG_CRC_EC_OFFSET_MASK    (0x7FF0000U)
#define XCVR_CTRL_CRCW_CFG_CRC_EC_OFFSET_SHIFT   (16U)
#define XCVR_CTRL_CRCW_CFG_CRC_EC_OFFSET(x)      (((uint32_t)(((uint32_t)(x)) << XCVR_CTRL_CRCW_CFG_CRC_EC_OFFSET_SHIFT)) & XCVR_CTRL_CRCW_CFG_CRC_EC_OFFSET_MASK)
#define XCVR_CTRL_CRCW_CFG_CRC_EC_DONE_MASK      (0x10000000U)
#define XCVR_CTRL_CRCW_CFG_CRC_EC_DONE_SHIFT     (28U)
#define XCVR_CTRL_CRCW_CFG_CRC_EC_DONE(x)        (((uint32_t)(((uint32_t)(x)) << XCVR_CTRL_CRCW_CFG_CRC_EC_DONE_SHIFT)) & XCVR_CTRL_CRCW_CFG_CRC_EC_DONE_MASK)
#define XCVR_CTRL_CRCW_CFG_CRC_EC_FAIL_MASK      (0x20000000U)
#define XCVR_CTRL_CRCW_CFG_CRC_EC_FAIL_SHIFT     (29U)
#define XCVR_CTRL_CRCW_CFG_CRC_EC_FAIL(x)        (((uint32_t)(((uint32_t)(x)) << XCVR_CTRL_CRCW_CFG_CRC_EC_FAIL_SHIFT)) & XCVR_CTRL_CRCW_CFG_CRC_EC_FAIL_MASK)

/*! @name CRC_EC_MASK - CRC ERROR CORRECTION MASK */
#define XCVR_CTRL_CRC_EC_MASK_CRC_EC_MASK_MASK   (0xFFFFFFFFU)
#define XCVR_CTRL_CRC_EC_MASK_CRC_EC_MASK_SHIFT  (0U)
#define XCVR_CTRL_CRC_EC_MASK_CRC_EC_MASK(x)     (((uint32_t)(((uint32_t)(x)) << XCVR_CTRL_CRC_EC_MASK_CRC_EC_MASK_SHIFT)) & XCVR_CTRL_CRC_EC_MASK_CRC_EC_MASK_MASK)

/*! @name CRC_RES_OUT - CRC RESULT */
#define XCVR_CTRL_CRC_RES_OUT_CRC_RES_OUT_MASK   (0xFFFFFFFFU)
#define XCVR_CTRL_CRC_RES_OUT_CRC_RES_OUT_SHIFT  (0U)
#define XCVR_CTRL_CRC_RES_OUT_CRC_RES_OUT(x)     (((uint32_t)(((uint32_t)(x)) << XCVR_CTRL_CRC_RES_OUT_CRC_RES_OUT_SHIFT)) & XCVR_CTRL_CRC_RES_OUT_CRC_RES_OUT_MASK)


/*!
 * @}
 */ /* end of group XCVR_CTRL_Register_Masks */


/* XCVR_CTRL - Peripheral instance base addresses */
/** Peripheral XCVR_MISC base address */
#define XCVR_MISC_BASE                           (0x4005C280u)
/** Peripheral XCVR_MISC base pointer */
#define XCVR_MISC                                ((XCVR_CTRL_Type *)XCVR_MISC_BASE)
/** Array initializer of XCVR_CTRL peripheral base addresses */
#define XCVR_CTRL_BASE_ADDRS                     { XCVR_MISC_BASE }
/** Array initializer of XCVR_CTRL peripheral base pointers */
#define XCVR_CTRL_BASE_PTRS                      { XCVR_MISC }

/*!
 * @}
 */ /* end of group XCVR_CTRL_Peripheral_Access_Layer */


/* ----------------------------------------------------------------------------
   -- XCVR_PHY Peripheral Access Layer
   ---------------------------------------------------------------------------- */

/*!
 * @addtogroup XCVR_PHY_Peripheral_Access_Layer XCVR_PHY Peripheral Access Layer
 * @{
 */

/** XCVR_PHY - Register Layout Typedef */
typedef struct {
  __IO uint32_t PHY_PRE_REF0;                      /**< PREAMBLE REFERENCE WAVEFORM 0, offset: 0x0 */
  __IO uint32_t PRE_REF1;                          /**< PREAMBLE REFERENCE WAVEFORM 1, offset: 0x4 */
  __IO uint32_t PRE_REF2;                          /**< PREAMBLE REFERENCE WAVEFORM 2, offset: 0x8 */
       uint8_t RESERVED_0[20];
  __IO uint32_t CFG1;                              /**< PHY CONFIGURATION REGISTER 1, offset: 0x20 */
  __IO uint32_t CFG2;                              /**< PHY CONFIGURATION REGISTER 2, offset: 0x24 */
  __IO uint32_t EL_CFG;                            /**< PHY EARLY/LATE CONFIGURATION REGISTER, offset: 0x28 */
  __IO uint32_t NTW_ADR_BSM;                       /**< PHY NETWORK ADDRESS FOR BSM, offset: 0x2C */
  __I  uint32_t STATUS;                            /**< PHY STATUS REGISTER, offset: 0x30 */
} XCVR_PHY_Type;

/* ----------------------------------------------------------------------------
   -- XCVR_PHY Register Masks
   ---------------------------------------------------------------------------- */

/*!
 * @addtogroup XCVR_PHY_Register_Masks XCVR_PHY Register Masks
 * @{
 */

/*! @name PHY_PRE_REF0 - PREAMBLE REFERENCE WAVEFORM 0 */
#define XCVR_PHY_PHY_PRE_REF0_FSK_PREAMBLE_REF0_MASK (0xFFFFFFFFU)
#define XCVR_PHY_PHY_PRE_REF0_FSK_PREAMBLE_REF0_SHIFT (0U)
#define XCVR_PHY_PHY_PRE_REF0_FSK_PREAMBLE_REF0(x) (((uint32_t)(((uint32_t)(x)) << XCVR_PHY_PHY_PRE_REF0_FSK_PREAMBLE_REF0_SHIFT)) & XCVR_PHY_PHY_PRE_REF0_FSK_PREAMBLE_REF0_MASK)

/*! @name PRE_REF1 - PREAMBLE REFERENCE WAVEFORM 1 */
#define XCVR_PHY_PRE_REF1_FSK_PREAMBLE_REF1_MASK (0xFFFFFFFFU)
#define XCVR_PHY_PRE_REF1_FSK_PREAMBLE_REF1_SHIFT (0U)
#define XCVR_PHY_PRE_REF1_FSK_PREAMBLE_REF1(x)   (((uint32_t)(((uint32_t)(x)) << XCVR_PHY_PRE_REF1_FSK_PREAMBLE_REF1_SHIFT)) & XCVR_PHY_PRE_REF1_FSK_PREAMBLE_REF1_MASK)

/*! @name PRE_REF2 - PREAMBLE REFERENCE WAVEFORM 2 */
#define XCVR_PHY_PRE_REF2_FSK_PREAMBLE_REF2_MASK (0xFFFFU)
#define XCVR_PHY_PRE_REF2_FSK_PREAMBLE_REF2_SHIFT (0U)
#define XCVR_PHY_PRE_REF2_FSK_PREAMBLE_REF2(x)   (((uint32_t)(((uint32_t)(x)) << XCVR_PHY_PRE_REF2_FSK_PREAMBLE_REF2_SHIFT)) & XCVR_PHY_PRE_REF2_FSK_PREAMBLE_REF2_MASK)

/*! @name CFG1 - PHY CONFIGURATION REGISTER 1 */
#define XCVR_PHY_CFG1_AA_PLAYBACK_MASK           (0x2U)
#define XCVR_PHY_CFG1_AA_PLAYBACK_SHIFT          (1U)
#define XCVR_PHY_CFG1_AA_PLAYBACK(x)             (((uint32_t)(((uint32_t)(x)) << XCVR_PHY_CFG1_AA_PLAYBACK_SHIFT)) & XCVR_PHY_CFG1_AA_PLAYBACK_MASK)
#define XCVR_PHY_CFG1_AA_OUTPUT_SEL_MASK         (0x4U)
#define XCVR_PHY_CFG1_AA_OUTPUT_SEL_SHIFT        (2U)
#define XCVR_PHY_CFG1_AA_OUTPUT_SEL(x)           (((uint32_t)(((uint32_t)(x)) << XCVR_PHY_CFG1_AA_OUTPUT_SEL_SHIFT)) & XCVR_PHY_CFG1_AA_OUTPUT_SEL_MASK)
#define XCVR_PHY_CFG1_FSK_BIT_INVERT_MASK        (0x8U)
#define XCVR_PHY_CFG1_FSK_BIT_INVERT_SHIFT       (3U)
#define XCVR_PHY_CFG1_FSK_BIT_INVERT(x)          (((uint32_t)(((uint32_t)(x)) << XCVR_PHY_CFG1_FSK_BIT_INVERT_SHIFT)) & XCVR_PHY_CFG1_FSK_BIT_INVERT_MASK)
#define XCVR_PHY_CFG1_RFU00_MASK                 (0x10U)
#define XCVR_PHY_CFG1_RFU00_SHIFT                (4U)
#define XCVR_PHY_CFG1_RFU00(x)                   (((uint32_t)(((uint32_t)(x)) << XCVR_PHY_CFG1_RFU00_SHIFT)) & XCVR_PHY_CFG1_RFU00_MASK)
#define XCVR_PHY_CFG1_BSM_EN_BLE_MASK            (0x20U)
#define XCVR_PHY_CFG1_BSM_EN_BLE_SHIFT           (5U)
#define XCVR_PHY_CFG1_BSM_EN_BLE(x)              (((uint32_t)(((uint32_t)(x)) << XCVR_PHY_CFG1_BSM_EN_BLE_SHIFT)) & XCVR_PHY_CFG1_BSM_EN_BLE_MASK)
#define XCVR_PHY_CFG1_DEMOD_CLK_MODE_MASK        (0xC0U)
#define XCVR_PHY_CFG1_DEMOD_CLK_MODE_SHIFT       (6U)
#define XCVR_PHY_CFG1_DEMOD_CLK_MODE(x)          (((uint32_t)(((uint32_t)(x)) << XCVR_PHY_CFG1_DEMOD_CLK_MODE_SHIFT)) & XCVR_PHY_CFG1_DEMOD_CLK_MODE_MASK)
#define XCVR_PHY_CFG1_CTS_THRESH_MASK            (0xFF00U)
#define XCVR_PHY_CFG1_CTS_THRESH_SHIFT           (8U)
#define XCVR_PHY_CFG1_CTS_THRESH(x)              (((uint32_t)(((uint32_t)(x)) << XCVR_PHY_CFG1_CTS_THRESH_SHIFT)) & XCVR_PHY_CFG1_CTS_THRESH_MASK)
#define XCVR_PHY_CFG1_FSK_FTS_TIMEOUT_MASK       (0x700000U)
#define XCVR_PHY_CFG1_FSK_FTS_TIMEOUT_SHIFT      (20U)
#define XCVR_PHY_CFG1_FSK_FTS_TIMEOUT(x)         (((uint32_t)(((uint32_t)(x)) << XCVR_PHY_CFG1_FSK_FTS_TIMEOUT_SHIFT)) & XCVR_PHY_CFG1_FSK_FTS_TIMEOUT_MASK)
#define XCVR_PHY_CFG1_RFU01_MASK                 (0x1000000U)
#define XCVR_PHY_CFG1_RFU01_SHIFT                (24U)
#define XCVR_PHY_CFG1_RFU01(x)                   (((uint32_t)(((uint32_t)(x)) << XCVR_PHY_CFG1_RFU01_SHIFT)) & XCVR_PHY_CFG1_RFU01_MASK)
#define XCVR_PHY_CFG1_RFU02_MASK                 (0x2000000U)
#define XCVR_PHY_CFG1_RFU02_SHIFT                (25U)
#define XCVR_PHY_CFG1_RFU02(x)                   (((uint32_t)(((uint32_t)(x)) << XCVR_PHY_CFG1_RFU02_SHIFT)) & XCVR_PHY_CFG1_RFU02_MASK)
#define XCVR_PHY_CFG1_BLE_NTW_ADR_THR_MASK       (0x70000000U)
#define XCVR_PHY_CFG1_BLE_NTW_ADR_THR_SHIFT      (28U)
#define XCVR_PHY_CFG1_BLE_NTW_ADR_THR(x)         (((uint32_t)(((uint32_t)(x)) << XCVR_PHY_CFG1_BLE_NTW_ADR_THR_SHIFT)) & XCVR_PHY_CFG1_BLE_NTW_ADR_THR_MASK)

/*! @name CFG2 - PHY CONFIGURATION REGISTER 2 */
#define XCVR_PHY_CFG2_PHY_FIFO_PRECHG_MASK       (0xFU)
#define XCVR_PHY_CFG2_PHY_FIFO_PRECHG_SHIFT      (0U)
#define XCVR_PHY_CFG2_PHY_FIFO_PRECHG(x)         (((uint32_t)(((uint32_t)(x)) << XCVR_PHY_CFG2_PHY_FIFO_PRECHG_SHIFT)) & XCVR_PHY_CFG2_PHY_FIFO_PRECHG_MASK)
#define XCVR_PHY_CFG2_RFU03_MASK                 (0x10U)
#define XCVR_PHY_CFG2_RFU03_SHIFT                (4U)
#define XCVR_PHY_CFG2_RFU03(x)                   (((uint32_t)(((uint32_t)(x)) << XCVR_PHY_CFG2_RFU03_SHIFT)) & XCVR_PHY_CFG2_RFU03_MASK)
#define XCVR_PHY_CFG2_RFU04_MASK                 (0x20U)
#define XCVR_PHY_CFG2_RFU04_SHIFT                (5U)
#define XCVR_PHY_CFG2_RFU04(x)                   (((uint32_t)(((uint32_t)(x)) << XCVR_PHY_CFG2_RFU04_SHIFT)) & XCVR_PHY_CFG2_RFU04_MASK)
#define XCVR_PHY_CFG2_RFU05_MASK                 (0x40U)
#define XCVR_PHY_CFG2_RFU05_SHIFT                (6U)
#define XCVR_PHY_CFG2_RFU05(x)                   (((uint32_t)(((uint32_t)(x)) << XCVR_PHY_CFG2_RFU05_SHIFT)) & XCVR_PHY_CFG2_RFU05_MASK)
#define XCVR_PHY_CFG2_RFU06_MASK                 (0x80U)
#define XCVR_PHY_CFG2_RFU06_SHIFT                (7U)
#define XCVR_PHY_CFG2_RFU06(x)                   (((uint32_t)(((uint32_t)(x)) << XCVR_PHY_CFG2_RFU06_SHIFT)) & XCVR_PHY_CFG2_RFU06_MASK)
#define XCVR_PHY_CFG2_X2_DEMOD_GAIN_MASK         (0xF00U)
#define XCVR_PHY_CFG2_X2_DEMOD_GAIN_SHIFT        (8U)
#define XCVR_PHY_CFG2_X2_DEMOD_GAIN(x)           (((uint32_t)(((uint32_t)(x)) << XCVR_PHY_CFG2_X2_DEMOD_GAIN_SHIFT)) & XCVR_PHY_CFG2_X2_DEMOD_GAIN_MASK)
#define XCVR_PHY_CFG2_RFU07_MASK                 (0x10000U)
#define XCVR_PHY_CFG2_RFU07_SHIFT                (16U)
#define XCVR_PHY_CFG2_RFU07(x)                   (((uint32_t)(((uint32_t)(x)) << XCVR_PHY_CFG2_RFU07_SHIFT)) & XCVR_PHY_CFG2_RFU07_MASK)
#define XCVR_PHY_CFG2_RFU08_MASK                 (0x20000U)
#define XCVR_PHY_CFG2_RFU08_SHIFT                (17U)
#define XCVR_PHY_CFG2_RFU08(x)                   (((uint32_t)(((uint32_t)(x)) << XCVR_PHY_CFG2_RFU08_SHIFT)) & XCVR_PHY_CFG2_RFU08_MASK)
#define XCVR_PHY_CFG2_RFU09_MASK                 (0x40000U)
#define XCVR_PHY_CFG2_RFU09_SHIFT                (18U)
#define XCVR_PHY_CFG2_RFU09(x)                   (((uint32_t)(((uint32_t)(x)) << XCVR_PHY_CFG2_RFU09_SHIFT)) & XCVR_PHY_CFG2_RFU09_MASK)
#define XCVR_PHY_CFG2_RFU10_MASK                 (0x80000U)
#define XCVR_PHY_CFG2_RFU10_SHIFT                (19U)
#define XCVR_PHY_CFG2_RFU10(x)                   (((uint32_t)(((uint32_t)(x)) << XCVR_PHY_CFG2_RFU10_SHIFT)) & XCVR_PHY_CFG2_RFU10_MASK)
#define XCVR_PHY_CFG2_RFU11_MASK                 (0x100000U)
#define XCVR_PHY_CFG2_RFU11_SHIFT                (20U)
#define XCVR_PHY_CFG2_RFU11(x)                   (((uint32_t)(((uint32_t)(x)) << XCVR_PHY_CFG2_RFU11_SHIFT)) & XCVR_PHY_CFG2_RFU11_MASK)
#define XCVR_PHY_CFG2_RFU12_MASK                 (0x200000U)
#define XCVR_PHY_CFG2_RFU12_SHIFT                (21U)
#define XCVR_PHY_CFG2_RFU12(x)                   (((uint32_t)(((uint32_t)(x)) << XCVR_PHY_CFG2_RFU12_SHIFT)) & XCVR_PHY_CFG2_RFU12_MASK)
#define XCVR_PHY_CFG2_RFU13_MASK                 (0x400000U)
#define XCVR_PHY_CFG2_RFU13_SHIFT                (22U)
#define XCVR_PHY_CFG2_RFU13(x)                   (((uint32_t)(((uint32_t)(x)) << XCVR_PHY_CFG2_RFU13_SHIFT)) & XCVR_PHY_CFG2_RFU13_MASK)
#define XCVR_PHY_CFG2_RFU14_MASK                 (0x800000U)
#define XCVR_PHY_CFG2_RFU14_SHIFT                (23U)
#define XCVR_PHY_CFG2_RFU14(x)                   (((uint32_t)(((uint32_t)(x)) << XCVR_PHY_CFG2_RFU14_SHIFT)) & XCVR_PHY_CFG2_RFU14_MASK)
#define XCVR_PHY_CFG2_RFU15_MASK                 (0x1000000U)
#define XCVR_PHY_CFG2_RFU15_SHIFT                (24U)
#define XCVR_PHY_CFG2_RFU15(x)                   (((uint32_t)(((uint32_t)(x)) << XCVR_PHY_CFG2_RFU15_SHIFT)) & XCVR_PHY_CFG2_RFU15_MASK)
#define XCVR_PHY_CFG2_RFU16_MASK                 (0x2000000U)
#define XCVR_PHY_CFG2_RFU16_SHIFT                (25U)
#define XCVR_PHY_CFG2_RFU16(x)                   (((uint32_t)(((uint32_t)(x)) << XCVR_PHY_CFG2_RFU16_SHIFT)) & XCVR_PHY_CFG2_RFU16_MASK)
#define XCVR_PHY_CFG2_PHY_CLK_ON_MASK            (0x80000000U)
#define XCVR_PHY_CFG2_PHY_CLK_ON_SHIFT           (31U)
#define XCVR_PHY_CFG2_PHY_CLK_ON(x)              (((uint32_t)(((uint32_t)(x)) << XCVR_PHY_CFG2_PHY_CLK_ON_SHIFT)) & XCVR_PHY_CFG2_PHY_CLK_ON_MASK)

/*! @name EL_CFG - PHY EARLY/LATE CONFIGURATION REGISTER */
#define XCVR_PHY_EL_CFG_EL_ENABLE_MASK           (0x1U)
#define XCVR_PHY_EL_CFG_EL_ENABLE_SHIFT          (0U)
#define XCVR_PHY_EL_CFG_EL_ENABLE(x)             (((uint32_t)(((uint32_t)(x)) << XCVR_PHY_EL_CFG_EL_ENABLE_SHIFT)) & XCVR_PHY_EL_CFG_EL_ENABLE_MASK)
#define XCVR_PHY_EL_CFG_EL_ZB_ENABLE_MASK        (0x2U)
#define XCVR_PHY_EL_CFG_EL_ZB_ENABLE_SHIFT       (1U)
#define XCVR_PHY_EL_CFG_EL_ZB_ENABLE(x)          (((uint32_t)(((uint32_t)(x)) << XCVR_PHY_EL_CFG_EL_ZB_ENABLE_SHIFT)) & XCVR_PHY_EL_CFG_EL_ZB_ENABLE_MASK)
#define XCVR_PHY_EL_CFG_EL_ZB_WIN_SIZE_MASK      (0x4U)
#define XCVR_PHY_EL_CFG_EL_ZB_WIN_SIZE_SHIFT     (2U)
#define XCVR_PHY_EL_CFG_EL_ZB_WIN_SIZE(x)        (((uint32_t)(((uint32_t)(x)) << XCVR_PHY_EL_CFG_EL_ZB_WIN_SIZE_SHIFT)) & XCVR_PHY_EL_CFG_EL_ZB_WIN_SIZE_MASK)
#define XCVR_PHY_EL_CFG_EL_WIN_SIZE_MASK         (0xF00U)
#define XCVR_PHY_EL_CFG_EL_WIN_SIZE_SHIFT        (8U)
#define XCVR_PHY_EL_CFG_EL_WIN_SIZE(x)           (((uint32_t)(((uint32_t)(x)) << XCVR_PHY_EL_CFG_EL_WIN_SIZE_SHIFT)) & XCVR_PHY_EL_CFG_EL_WIN_SIZE_MASK)
#define XCVR_PHY_EL_CFG_EL_INTERVAL_MASK         (0x3F0000U)
#define XCVR_PHY_EL_CFG_EL_INTERVAL_SHIFT        (16U)
#define XCVR_PHY_EL_CFG_EL_INTERVAL(x)           (((uint32_t)(((uint32_t)(x)) << XCVR_PHY_EL_CFG_EL_INTERVAL_SHIFT)) & XCVR_PHY_EL_CFG_EL_INTERVAL_MASK)

/*! @name NTW_ADR_BSM - PHY NETWORK ADDRESS FOR BSM */
#define XCVR_PHY_NTW_ADR_BSM_NTW_ADR_BSM_MASK    (0xFFFFFFFFU)
#define XCVR_PHY_NTW_ADR_BSM_NTW_ADR_BSM_SHIFT   (0U)
#define XCVR_PHY_NTW_ADR_BSM_NTW_ADR_BSM(x)      (((uint32_t)(((uint32_t)(x)) << XCVR_PHY_NTW_ADR_BSM_NTW_ADR_BSM_SHIFT)) & XCVR_PHY_NTW_ADR_BSM_NTW_ADR_BSM_MASK)

/*! @name STATUS - PHY STATUS REGISTER */
#define XCVR_PHY_STATUS_PREAMBLE_FOUND_MASK      (0x1U)
#define XCVR_PHY_STATUS_PREAMBLE_FOUND_SHIFT     (0U)
#define XCVR_PHY_STATUS_PREAMBLE_FOUND(x)        (((uint32_t)(((uint32_t)(x)) << XCVR_PHY_STATUS_PREAMBLE_FOUND_SHIFT)) & XCVR_PHY_STATUS_PREAMBLE_FOUND_MASK)
#define XCVR_PHY_STATUS_AA_SFD_MATCHED_MASK      (0x2U)
#define XCVR_PHY_STATUS_AA_SFD_MATCHED_SHIFT     (1U)
#define XCVR_PHY_STATUS_AA_SFD_MATCHED(x)        (((uint32_t)(((uint32_t)(x)) << XCVR_PHY_STATUS_AA_SFD_MATCHED_SHIFT)) & XCVR_PHY_STATUS_AA_SFD_MATCHED_MASK)
#define XCVR_PHY_STATUS_AA_MATCHED_MASK          (0xF0U)
#define XCVR_PHY_STATUS_AA_MATCHED_SHIFT         (4U)
#define XCVR_PHY_STATUS_AA_MATCHED(x)            (((uint32_t)(((uint32_t)(x)) << XCVR_PHY_STATUS_AA_MATCHED_SHIFT)) & XCVR_PHY_STATUS_AA_MATCHED_MASK)
#define XCVR_PHY_STATUS_HAMMING_DISTANCE_MASK    (0x700U)
#define XCVR_PHY_STATUS_HAMMING_DISTANCE_SHIFT   (8U)
#define XCVR_PHY_STATUS_HAMMING_DISTANCE(x)      (((uint32_t)(((uint32_t)(x)) << XCVR_PHY_STATUS_HAMMING_DISTANCE_SHIFT)) & XCVR_PHY_STATUS_HAMMING_DISTANCE_MASK)
#define XCVR_PHY_STATUS_DATA_FIFO_DEPTH_MASK     (0xF000U)
#define XCVR_PHY_STATUS_DATA_FIFO_DEPTH_SHIFT    (12U)
#define XCVR_PHY_STATUS_DATA_FIFO_DEPTH(x)       (((uint32_t)(((uint32_t)(x)) << XCVR_PHY_STATUS_DATA_FIFO_DEPTH_SHIFT)) & XCVR_PHY_STATUS_DATA_FIFO_DEPTH_MASK)
#define XCVR_PHY_STATUS_CFO_ESTIMATE_MASK        (0xFF0000U)
#define XCVR_PHY_STATUS_CFO_ESTIMATE_SHIFT       (16U)
#define XCVR_PHY_STATUS_CFO_ESTIMATE(x)          (((uint32_t)(((uint32_t)(x)) << XCVR_PHY_STATUS_CFO_ESTIMATE_SHIFT)) & XCVR_PHY_STATUS_CFO_ESTIMATE_MASK)


/*!
 * @}
 */ /* end of group XCVR_PHY_Register_Masks */


/* XCVR_PHY - Peripheral instance base addresses */
/** Peripheral XCVR_PHY base address */
#define XCVR_PHY_BASE                            (0x4005C400u)
/** Peripheral XCVR_PHY base pointer */
#define XCVR_PHY                                 ((XCVR_PHY_Type *)XCVR_PHY_BASE)
/** Array initializer of XCVR_PHY peripheral base addresses */
#define XCVR_PHY_BASE_ADDRS                      { XCVR_PHY_BASE }
/** Array initializer of XCVR_PHY peripheral base pointers */
#define XCVR_PHY_BASE_PTRS                       { XCVR_PHY }

/*!
 * @}
 */ /* end of group XCVR_PHY_Peripheral_Access_Layer */


/* ----------------------------------------------------------------------------
   -- XCVR_PKT_RAM Peripheral Access Layer
   ---------------------------------------------------------------------------- */

/*!
 * @addtogroup XCVR_PKT_RAM_Peripheral_Access_Layer XCVR_PKT_RAM Peripheral Access Layer
 * @{
 */

/** XCVR_PKT_RAM - Register Layout Typedef */
typedef struct {
  __IO uint16_t PACKET_RAM_0[544];                 /**< Shared Packet RAM for multiple Link Layer usage., array offset: 0x0, array step: 0x2 */
  __IO uint16_t PACKET_RAM_1[544];                 /**< Shared Packet RAM for multiple Link Layer usage., array offset: 0x440, array step: 0x2 */
} XCVR_PKT_RAM_Type;

/* ----------------------------------------------------------------------------
   -- XCVR_PKT_RAM Register Masks
   ---------------------------------------------------------------------------- */

/*!
 * @addtogroup XCVR_PKT_RAM_Register_Masks XCVR_PKT_RAM Register Masks
 * @{
 */

/*! @name PACKET_RAM_0 - Shared Packet RAM for multiple Link Layer usage. */
#define XCVR_PKT_RAM_PACKET_RAM_0_LSBYTE_MASK    (0xFFU)
#define XCVR_PKT_RAM_PACKET_RAM_0_LSBYTE_SHIFT   (0U)
#define XCVR_PKT_RAM_PACKET_RAM_0_LSBYTE(x)      (((uint16_t)(((uint16_t)(x)) << XCVR_PKT_RAM_PACKET_RAM_0_LSBYTE_SHIFT)) & XCVR_PKT_RAM_PACKET_RAM_0_LSBYTE_MASK)
#define XCVR_PKT_RAM_PACKET_RAM_0_MSBYTE_MASK    (0xFF00U)
#define XCVR_PKT_RAM_PACKET_RAM_0_MSBYTE_SHIFT   (8U)
#define XCVR_PKT_RAM_PACKET_RAM_0_MSBYTE(x)      (((uint16_t)(((uint16_t)(x)) << XCVR_PKT_RAM_PACKET_RAM_0_MSBYTE_SHIFT)) & XCVR_PKT_RAM_PACKET_RAM_0_MSBYTE_MASK)

/* The count of XCVR_PKT_RAM_PACKET_RAM_0 */
#define XCVR_PKT_RAM_PACKET_RAM_0_COUNT          (544U)

/*! @name PACKET_RAM_1 - Shared Packet RAM for multiple Link Layer usage. */
#define XCVR_PKT_RAM_PACKET_RAM_1_LSBYTE_MASK    (0xFFU)
#define XCVR_PKT_RAM_PACKET_RAM_1_LSBYTE_SHIFT   (0U)
#define XCVR_PKT_RAM_PACKET_RAM_1_LSBYTE(x)      (((uint16_t)(((uint16_t)(x)) << XCVR_PKT_RAM_PACKET_RAM_1_LSBYTE_SHIFT)) & XCVR_PKT_RAM_PACKET_RAM_1_LSBYTE_MASK)
#define XCVR_PKT_RAM_PACKET_RAM_1_MSBYTE_MASK    (0xFF00U)
#define XCVR_PKT_RAM_PACKET_RAM_1_MSBYTE_SHIFT   (8U)
#define XCVR_PKT_RAM_PACKET_RAM_1_MSBYTE(x)      (((uint16_t)(((uint16_t)(x)) << XCVR_PKT_RAM_PACKET_RAM_1_MSBYTE_SHIFT)) & XCVR_PKT_RAM_PACKET_RAM_1_MSBYTE_MASK)

/* The count of XCVR_PKT_RAM_PACKET_RAM_1 */
#define XCVR_PKT_RAM_PACKET_RAM_1_COUNT          (544U)


/*!
 * @}
 */ /* end of group XCVR_PKT_RAM_Register_Masks */


/* XCVR_PKT_RAM - Peripheral instance base addresses */
/** Peripheral XCVR_PKT_RAM base address */
#define XCVR_PKT_RAM_BASE                        (0x4005C700u)
/** Peripheral XCVR_PKT_RAM base pointer */
#define XCVR_PKT_RAM                             ((XCVR_PKT_RAM_Type *)XCVR_PKT_RAM_BASE)
/** Array initializer of XCVR_PKT_RAM peripheral base addresses */
#define XCVR_PKT_RAM_BASE_ADDRS                  { XCVR_PKT_RAM_BASE }
/** Array initializer of XCVR_PKT_RAM peripheral base pointers */
#define XCVR_PKT_RAM_BASE_PTRS                   { XCVR_PKT_RAM }

/*!
 * @}
 */ /* end of group XCVR_PKT_RAM_Peripheral_Access_Layer */


/* ----------------------------------------------------------------------------
   -- XCVR_PLL_DIG Peripheral Access Layer
   ---------------------------------------------------------------------------- */

/*!
 * @addtogroup XCVR_PLL_DIG_Peripheral_Access_Layer XCVR_PLL_DIG Peripheral Access Layer
 * @{
 */

/** XCVR_PLL_DIG - Register Layout Typedef */
typedef struct {
  __IO uint32_t HPM_BUMP;                          /**< PLL HPM Analog Bump Control, offset: 0x0 */
  __IO uint32_t MOD_CTRL;                          /**< PLL Modulation Control, offset: 0x4 */
  __IO uint32_t CHAN_MAP;                          /**< PLL Channel Mapping, offset: 0x8 */
  __IO uint32_t LOCK_DETECT;                       /**< PLL Lock Detect Control, offset: 0xC */
  __IO uint32_t HPM_CTRL;                          /**< PLL High Port Modulator Control, offset: 0x10 */
  __IO uint32_t HPMCAL_CTRL;                       /**< PLL High Port Calibration Control, offset: 0x14 */
  __IO uint32_t HPM_CAL1;                          /**< PLL High Port Calibration Result 1, offset: 0x18 */
  __IO uint32_t HPM_CAL2;                          /**< PLL High Port Calibration Result 2, offset: 0x1C */
  __IO uint32_t HPM_SDM_RES;                       /**< PLL High Port Sigma Delta Results, offset: 0x20 */
  __IO uint32_t LPM_CTRL;                          /**< PLL Low Port Modulator Control, offset: 0x24 */
  __IO uint32_t LPM_SDM_CTRL1;                     /**< PLL Low Port Sigma Delta Control 1, offset: 0x28 */
  __IO uint32_t LPM_SDM_CTRL2;                     /**< PLL Low Port Sigma Delta Control 2, offset: 0x2C */
  __IO uint32_t LPM_SDM_CTRL3;                     /**< PLL Low Port Sigma Delta Control 3, offset: 0x30 */
  __I  uint32_t LPM_SDM_RES1;                      /**< PLL Low Port Sigma Delta Result 1, offset: 0x34 */
  __I  uint32_t LPM_SDM_RES2;                      /**< PLL Low Port Sigma Delta Result 2, offset: 0x38 */
  __IO uint32_t DELAY_MATCH;                       /**< PLL Delay Matching, offset: 0x3C */
  __IO uint32_t CTUNE_CTRL;                        /**< PLL Coarse Tune Control, offset: 0x40 */
  __I  uint32_t CTUNE_CNT6;                        /**< PLL Coarse Tune Count 6, offset: 0x44 */
  __I  uint32_t CTUNE_CNT5_4;                      /**< PLL Coarse Tune Counts 5 and 4, offset: 0x48 */
  __I  uint32_t CTUNE_CNT3_2;                      /**< PLL Coarse Tune Counts 3 and 2, offset: 0x4C */
  __I  uint32_t CTUNE_CNT1_0;                      /**< PLL Coarse Tune Counts 1 and 0, offset: 0x50 */
  __I  uint32_t CTUNE_RES;                         /**< PLL Coarse Tune Results, offset: 0x54 */
} XCVR_PLL_DIG_Type;

/* ----------------------------------------------------------------------------
   -- XCVR_PLL_DIG Register Masks
   ---------------------------------------------------------------------------- */

/*!
 * @addtogroup XCVR_PLL_DIG_Register_Masks XCVR_PLL_DIG Register Masks
 * @{
 */

/*! @name HPM_BUMP - PLL HPM Analog Bump Control */
#define XCVR_PLL_DIG_HPM_BUMP_HPM_VCM_TX_MASK    (0x7U)
#define XCVR_PLL_DIG_HPM_BUMP_HPM_VCM_TX_SHIFT   (0U)
#define XCVR_PLL_DIG_HPM_BUMP_HPM_VCM_TX(x)      (((uint32_t)(((uint32_t)(x)) << XCVR_PLL_DIG_HPM_BUMP_HPM_VCM_TX_SHIFT)) & XCVR_PLL_DIG_HPM_BUMP_HPM_VCM_TX_MASK)
#define XCVR_PLL_DIG_HPM_BUMP_HPM_VCM_CAL_MASK   (0x70U)
#define XCVR_PLL_DIG_HPM_BUMP_HPM_VCM_CAL_SHIFT  (4U)
#define XCVR_PLL_DIG_HPM_BUMP_HPM_VCM_CAL(x)     (((uint32_t)(((uint32_t)(x)) << XCVR_PLL_DIG_HPM_BUMP_HPM_VCM_CAL_SHIFT)) & XCVR_PLL_DIG_HPM_BUMP_HPM_VCM_CAL_MASK)
#define XCVR_PLL_DIG_HPM_BUMP_HPM_FDB_RES_TX_MASK (0x300U)
#define XCVR_PLL_DIG_HPM_BUMP_HPM_FDB_RES_TX_SHIFT (8U)
#define XCVR_PLL_DIG_HPM_BUMP_HPM_FDB_RES_TX(x)  (((uint32_t)(((uint32_t)(x)) << XCVR_PLL_DIG_HPM_BUMP_HPM_FDB_RES_TX_SHIFT)) & XCVR_PLL_DIG_HPM_BUMP_HPM_FDB_RES_TX_MASK)
#define XCVR_PLL_DIG_HPM_BUMP_HPM_FDB_RES_CAL_MASK (0x3000U)
#define XCVR_PLL_DIG_HPM_BUMP_HPM_FDB_RES_CAL_SHIFT (12U)
#define XCVR_PLL_DIG_HPM_BUMP_HPM_FDB_RES_CAL(x) (((uint32_t)(((uint32_t)(x)) << XCVR_PLL_DIG_HPM_BUMP_HPM_FDB_RES_CAL_SHIFT)) & XCVR_PLL_DIG_HPM_BUMP_HPM_FDB_RES_CAL_MASK)

/*! @name MOD_CTRL - PLL Modulation Control */
#define XCVR_PLL_DIG_MOD_CTRL_MODULATION_WORD_MANUAL_MASK (0x1FFFU)
#define XCVR_PLL_DIG_MOD_CTRL_MODULATION_WORD_MANUAL_SHIFT (0U)
#define XCVR_PLL_DIG_MOD_CTRL_MODULATION_WORD_MANUAL(x) (((uint32_t)(((uint32_t)(x)) << XCVR_PLL_DIG_MOD_CTRL_MODULATION_WORD_MANUAL_SHIFT)) & XCVR_PLL_DIG_MOD_CTRL_MODULATION_WORD_MANUAL_MASK)
#define XCVR_PLL_DIG_MOD_CTRL_MOD_DISABLE_MASK   (0x8000U)
#define XCVR_PLL_DIG_MOD_CTRL_MOD_DISABLE_SHIFT  (15U)
#define XCVR_PLL_DIG_MOD_CTRL_MOD_DISABLE(x)     (((uint32_t)(((uint32_t)(x)) << XCVR_PLL_DIG_MOD_CTRL_MOD_DISABLE_SHIFT)) & XCVR_PLL_DIG_MOD_CTRL_MOD_DISABLE_MASK)
#define XCVR_PLL_DIG_MOD_CTRL_HPM_MOD_MANUAL_MASK (0xFF0000U)
#define XCVR_PLL_DIG_MOD_CTRL_HPM_MOD_MANUAL_SHIFT (16U)
#define XCVR_PLL_DIG_MOD_CTRL_HPM_MOD_MANUAL(x)  (((uint32_t)(((uint32_t)(x)) << XCVR_PLL_DIG_MOD_CTRL_HPM_MOD_MANUAL_SHIFT)) & XCVR_PLL_DIG_MOD_CTRL_HPM_MOD_MANUAL_MASK)
#define XCVR_PLL_DIG_MOD_CTRL_HPM_MOD_DISABLE_MASK (0x8000000U)
#define XCVR_PLL_DIG_MOD_CTRL_HPM_MOD_DISABLE_SHIFT (27U)
#define XCVR_PLL_DIG_MOD_CTRL_HPM_MOD_DISABLE(x) (((uint32_t)(((uint32_t)(x)) << XCVR_PLL_DIG_MOD_CTRL_HPM_MOD_DISABLE_SHIFT)) & XCVR_PLL_DIG_MOD_CTRL_HPM_MOD_DISABLE_MASK)
#define XCVR_PLL_DIG_MOD_CTRL_HPM_SDM_OUT_MANUAL_MASK (0x30000000U)
#define XCVR_PLL_DIG_MOD_CTRL_HPM_SDM_OUT_MANUAL_SHIFT (28U)
#define XCVR_PLL_DIG_MOD_CTRL_HPM_SDM_OUT_MANUAL(x) (((uint32_t)(((uint32_t)(x)) << XCVR_PLL_DIG_MOD_CTRL_HPM_SDM_OUT_MANUAL_SHIFT)) & XCVR_PLL_DIG_MOD_CTRL_HPM_SDM_OUT_MANUAL_MASK)
#define XCVR_PLL_DIG_MOD_CTRL_HPM_SDM_OUT_DISABLE_MASK (0x80000000U)
#define XCVR_PLL_DIG_MOD_CTRL_HPM_SDM_OUT_DISABLE_SHIFT (31U)
#define XCVR_PLL_DIG_MOD_CTRL_HPM_SDM_OUT_DISABLE(x) (((uint32_t)(((uint32_t)(x)) << XCVR_PLL_DIG_MOD_CTRL_HPM_SDM_OUT_DISABLE_SHIFT)) & XCVR_PLL_DIG_MOD_CTRL_HPM_SDM_OUT_DISABLE_MASK)

/*! @name CHAN_MAP - PLL Channel Mapping */
#define XCVR_PLL_DIG_CHAN_MAP_CHANNEL_NUM_MASK   (0x7FU)
#define XCVR_PLL_DIG_CHAN_MAP_CHANNEL_NUM_SHIFT  (0U)
#define XCVR_PLL_DIG_CHAN_MAP_CHANNEL_NUM(x)     (((uint32_t)(((uint32_t)(x)) << XCVR_PLL_DIG_CHAN_MAP_CHANNEL_NUM_SHIFT)) & XCVR_PLL_DIG_CHAN_MAP_CHANNEL_NUM_MASK)
#define XCVR_PLL_DIG_CHAN_MAP_BOC_MASK           (0x100U)
#define XCVR_PLL_DIG_CHAN_MAP_BOC_SHIFT          (8U)
#define XCVR_PLL_DIG_CHAN_MAP_BOC(x)             (((uint32_t)(((uint32_t)(x)) << XCVR_PLL_DIG_CHAN_MAP_BOC_SHIFT)) & XCVR_PLL_DIG_CHAN_MAP_BOC_MASK)
#define XCVR_PLL_DIG_CHAN_MAP_BMR_MASK           (0x200U)
#define XCVR_PLL_DIG_CHAN_MAP_BMR_SHIFT          (9U)
#define XCVR_PLL_DIG_CHAN_MAP_BMR(x)             (((uint32_t)(((uint32_t)(x)) << XCVR_PLL_DIG_CHAN_MAP_BMR_SHIFT)) & XCVR_PLL_DIG_CHAN_MAP_BMR_MASK)
#define XCVR_PLL_DIG_CHAN_MAP_ZOC_MASK           (0x400U)
#define XCVR_PLL_DIG_CHAN_MAP_ZOC_SHIFT          (10U)
#define XCVR_PLL_DIG_CHAN_MAP_ZOC(x)             (((uint32_t)(((uint32_t)(x)) << XCVR_PLL_DIG_CHAN_MAP_ZOC_SHIFT)) & XCVR_PLL_DIG_CHAN_MAP_ZOC_MASK)

/*! @name LOCK_DETECT - PLL Lock Detect Control */
#define XCVR_PLL_DIG_LOCK_DETECT_CT_FAIL_MASK    (0x1U)
#define XCVR_PLL_DIG_LOCK_DETECT_CT_FAIL_SHIFT   (0U)
#define XCVR_PLL_DIG_LOCK_DETECT_CT_FAIL(x)      (((uint32_t)(((uint32_t)(x)) << XCVR_PLL_DIG_LOCK_DETECT_CT_FAIL_SHIFT)) & XCVR_PLL_DIG_LOCK_DETECT_CT_FAIL_MASK)
#define XCVR_PLL_DIG_LOCK_DETECT_CTFF_MASK       (0x2U)
#define XCVR_PLL_DIG_LOCK_DETECT_CTFF_SHIFT      (1U)
#define XCVR_PLL_DIG_LOCK_DETECT_CTFF(x)         (((uint32_t)(((uint32_t)(x)) << XCVR_PLL_DIG_LOCK_DETECT_CTFF_SHIFT)) & XCVR_PLL_DIG_LOCK_DETECT_CTFF_MASK)
#define XCVR_PLL_DIG_LOCK_DETECT_CS_FAIL_MASK    (0x4U)
#define XCVR_PLL_DIG_LOCK_DETECT_CS_FAIL_SHIFT   (2U)
#define XCVR_PLL_DIG_LOCK_DETECT_CS_FAIL(x)      (((uint32_t)(((uint32_t)(x)) << XCVR_PLL_DIG_LOCK_DETECT_CS_FAIL_SHIFT)) & XCVR_PLL_DIG_LOCK_DETECT_CS_FAIL_MASK)
#define XCVR_PLL_DIG_LOCK_DETECT_CSFF_MASK       (0x8U)
#define XCVR_PLL_DIG_LOCK_DETECT_CSFF_SHIFT      (3U)
#define XCVR_PLL_DIG_LOCK_DETECT_CSFF(x)         (((uint32_t)(((uint32_t)(x)) << XCVR_PLL_DIG_LOCK_DETECT_CSFF_SHIFT)) & XCVR_PLL_DIG_LOCK_DETECT_CSFF_MASK)
#define XCVR_PLL_DIG_LOCK_DETECT_FT_FAIL_MASK    (0x10U)
#define XCVR_PLL_DIG_LOCK_DETECT_FT_FAIL_SHIFT   (4U)
#define XCVR_PLL_DIG_LOCK_DETECT_FT_FAIL(x)      (((uint32_t)(((uint32_t)(x)) << XCVR_PLL_DIG_LOCK_DETECT_FT_FAIL_SHIFT)) & XCVR_PLL_DIG_LOCK_DETECT_FT_FAIL_MASK)
#define XCVR_PLL_DIG_LOCK_DETECT_FTFF_MASK       (0x20U)
#define XCVR_PLL_DIG_LOCK_DETECT_FTFF_SHIFT      (5U)
#define XCVR_PLL_DIG_LOCK_DETECT_FTFF(x)         (((uint32_t)(((uint32_t)(x)) << XCVR_PLL_DIG_LOCK_DETECT_FTFF_SHIFT)) & XCVR_PLL_DIG_LOCK_DETECT_FTFF_MASK)
#define XCVR_PLL_DIG_LOCK_DETECT_TAFF_MASK       (0x80U)
#define XCVR_PLL_DIG_LOCK_DETECT_TAFF_SHIFT      (7U)
#define XCVR_PLL_DIG_LOCK_DETECT_TAFF(x)         (((uint32_t)(((uint32_t)(x)) << XCVR_PLL_DIG_LOCK_DETECT_TAFF_SHIFT)) & XCVR_PLL_DIG_LOCK_DETECT_TAFF_MASK)
#define XCVR_PLL_DIG_LOCK_DETECT_CTUNE_LDF_LEV_MASK (0xF00U)
#define XCVR_PLL_DIG_LOCK_DETECT_CTUNE_LDF_LEV_SHIFT (8U)
#define XCVR_PLL_DIG_LOCK_DETECT_CTUNE_LDF_LEV(x) (((uint32_t)(((uint32_t)(x)) << XCVR_PLL_DIG_LOCK_DETECT_CTUNE_LDF_LEV_SHIFT)) & XCVR_PLL_DIG_LOCK_DETECT_CTUNE_LDF_LEV_MASK)
#define XCVR_PLL_DIG_LOCK_DETECT_FTF_RX_THRSH_MASK (0x3F000U)
#define XCVR_PLL_DIG_LOCK_DETECT_FTF_RX_THRSH_SHIFT (12U)
#define XCVR_PLL_DIG_LOCK_DETECT_FTF_RX_THRSH(x) (((uint32_t)(((uint32_t)(x)) << XCVR_PLL_DIG_LOCK_DETECT_FTF_RX_THRSH_SHIFT)) & XCVR_PLL_DIG_LOCK_DETECT_FTF_RX_THRSH_MASK)
#define XCVR_PLL_DIG_LOCK_DETECT_FTW_RX_MASK     (0x80000U)
#define XCVR_PLL_DIG_LOCK_DETECT_FTW_RX_SHIFT    (19U)
#define XCVR_PLL_DIG_LOCK_DETECT_FTW_RX(x)       (((uint32_t)(((uint32_t)(x)) << XCVR_PLL_DIG_LOCK_DETECT_FTW_RX_SHIFT)) & XCVR_PLL_DIG_LOCK_DETECT_FTW_RX_MASK)
#define XCVR_PLL_DIG_LOCK_DETECT_FTF_TX_THRSH_MASK (0x3F00000U)
#define XCVR_PLL_DIG_LOCK_DETECT_FTF_TX_THRSH_SHIFT (20U)
#define XCVR_PLL_DIG_LOCK_DETECT_FTF_TX_THRSH(x) (((uint32_t)(((uint32_t)(x)) << XCVR_PLL_DIG_LOCK_DETECT_FTF_TX_THRSH_SHIFT)) & XCVR_PLL_DIG_LOCK_DETECT_FTF_TX_THRSH_MASK)
#define XCVR_PLL_DIG_LOCK_DETECT_FTW_TX_MASK     (0x8000000U)
#define XCVR_PLL_DIG_LOCK_DETECT_FTW_TX_SHIFT    (27U)
#define XCVR_PLL_DIG_LOCK_DETECT_FTW_TX(x)       (((uint32_t)(((uint32_t)(x)) << XCVR_PLL_DIG_LOCK_DETECT_FTW_TX_SHIFT)) & XCVR_PLL_DIG_LOCK_DETECT_FTW_TX_MASK)
#define XCVR_PLL_DIG_LOCK_DETECT_FREQ_COUNT_GO_MASK (0x10000000U)
#define XCVR_PLL_DIG_LOCK_DETECT_FREQ_COUNT_GO_SHIFT (28U)
#define XCVR_PLL_DIG_LOCK_DETECT_FREQ_COUNT_GO(x) (((uint32_t)(((uint32_t)(x)) << XCVR_PLL_DIG_LOCK_DETECT_FREQ_COUNT_GO_SHIFT)) & XCVR_PLL_DIG_LOCK_DETECT_FREQ_COUNT_GO_MASK)
#define XCVR_PLL_DIG_LOCK_DETECT_FREQ_COUNT_FINISHED_MASK (0x20000000U)
#define XCVR_PLL_DIG_LOCK_DETECT_FREQ_COUNT_FINISHED_SHIFT (29U)
#define XCVR_PLL_DIG_LOCK_DETECT_FREQ_COUNT_FINISHED(x) (((uint32_t)(((uint32_t)(x)) << XCVR_PLL_DIG_LOCK_DETECT_FREQ_COUNT_FINISHED_SHIFT)) & XCVR_PLL_DIG_LOCK_DETECT_FREQ_COUNT_FINISHED_MASK)
#define XCVR_PLL_DIG_LOCK_DETECT_FREQ_COUNT_TIME_MASK (0xC0000000U)
#define XCVR_PLL_DIG_LOCK_DETECT_FREQ_COUNT_TIME_SHIFT (30U)
#define XCVR_PLL_DIG_LOCK_DETECT_FREQ_COUNT_TIME(x) (((uint32_t)(((uint32_t)(x)) << XCVR_PLL_DIG_LOCK_DETECT_FREQ_COUNT_TIME_SHIFT)) & XCVR_PLL_DIG_LOCK_DETECT_FREQ_COUNT_TIME_MASK)

/*! @name HPM_CTRL - PLL High Port Modulator Control */
#define XCVR_PLL_DIG_HPM_CTRL_HPM_SDM_IN_MANUAL_MASK (0x3FFU)
#define XCVR_PLL_DIG_HPM_CTRL_HPM_SDM_IN_MANUAL_SHIFT (0U)
#define XCVR_PLL_DIG_HPM_CTRL_HPM_SDM_IN_MANUAL(x) (((uint32_t)(((uint32_t)(x)) << XCVR_PLL_DIG_HPM_CTRL_HPM_SDM_IN_MANUAL_SHIFT)) & XCVR_PLL_DIG_HPM_CTRL_HPM_SDM_IN_MANUAL_MASK)
#define XCVR_PLL_DIG_HPM_CTRL_HPFF_MASK          (0x2000U)
#define XCVR_PLL_DIG_HPM_CTRL_HPFF_SHIFT         (13U)
#define XCVR_PLL_DIG_HPM_CTRL_HPFF(x)            (((uint32_t)(((uint32_t)(x)) << XCVR_PLL_DIG_HPM_CTRL_HPFF_SHIFT)) & XCVR_PLL_DIG_HPM_CTRL_HPFF_MASK)
#define XCVR_PLL_DIG_HPM_CTRL_HPM_SDM_OUT_INVERT_MASK (0x4000U)
#define XCVR_PLL_DIG_HPM_CTRL_HPM_SDM_OUT_INVERT_SHIFT (14U)
#define XCVR_PLL_DIG_HPM_CTRL_HPM_SDM_OUT_INVERT(x) (((uint32_t)(((uint32_t)(x)) << XCVR_PLL_DIG_HPM_CTRL_HPM_SDM_OUT_INVERT_SHIFT)) & XCVR_PLL_DIG_HPM_CTRL_HPM_SDM_OUT_INVERT_MASK)
#define XCVR_PLL_DIG_HPM_CTRL_HPM_SDM_IN_DISABLE_MASK (0x8000U)
#define XCVR_PLL_DIG_HPM_CTRL_HPM_SDM_IN_DISABLE_SHIFT (15U)
#define XCVR_PLL_DIG_HPM_CTRL_HPM_SDM_IN_DISABLE(x) (((uint32_t)(((uint32_t)(x)) << XCVR_PLL_DIG_HPM_CTRL_HPM_SDM_IN_DISABLE_SHIFT)) & XCVR_PLL_DIG_HPM_CTRL_HPM_SDM_IN_DISABLE_MASK)
#define XCVR_PLL_DIG_HPM_CTRL_HPM_LFSR_SIZE_MASK (0x70000U)
#define XCVR_PLL_DIG_HPM_CTRL_HPM_LFSR_SIZE_SHIFT (16U)
#define XCVR_PLL_DIG_HPM_CTRL_HPM_LFSR_SIZE(x)   (((uint32_t)(((uint32_t)(x)) << XCVR_PLL_DIG_HPM_CTRL_HPM_LFSR_SIZE_SHIFT)) & XCVR_PLL_DIG_HPM_CTRL_HPM_LFSR_SIZE_MASK)
#define XCVR_PLL_DIG_HPM_CTRL_HPM_DTH_SCL_MASK   (0x100000U)
#define XCVR_PLL_DIG_HPM_CTRL_HPM_DTH_SCL_SHIFT  (20U)
#define XCVR_PLL_DIG_HPM_CTRL_HPM_DTH_SCL(x)     (((uint32_t)(((uint32_t)(x)) << XCVR_PLL_DIG_HPM_CTRL_HPM_DTH_SCL_SHIFT)) & XCVR_PLL_DIG_HPM_CTRL_HPM_DTH_SCL_MASK)
#define XCVR_PLL_DIG_HPM_CTRL_HPM_DTH_EN_MASK    (0x800000U)
#define XCVR_PLL_DIG_HPM_CTRL_HPM_DTH_EN_SHIFT   (23U)
#define XCVR_PLL_DIG_HPM_CTRL_HPM_DTH_EN(x)      (((uint32_t)(((uint32_t)(x)) << XCVR_PLL_DIG_HPM_CTRL_HPM_DTH_EN_SHIFT)) & XCVR_PLL_DIG_HPM_CTRL_HPM_DTH_EN_MASK)
#define XCVR_PLL_DIG_HPM_CTRL_HPM_INTEGER_SCALE_MASK (0x3000000U)
#define XCVR_PLL_DIG_HPM_CTRL_HPM_INTEGER_SCALE_SHIFT (24U)
#define XCVR_PLL_DIG_HPM_CTRL_HPM_INTEGER_SCALE(x) (((uint32_t)(((uint32_t)(x)) << XCVR_PLL_DIG_HPM_CTRL_HPM_INTEGER_SCALE_SHIFT)) & XCVR_PLL_DIG_HPM_CTRL_HPM_INTEGER_SCALE_MASK)
#define XCVR_PLL_DIG_HPM_CTRL_HPM_INTEGER_INVERT_MASK (0x8000000U)
#define XCVR_PLL_DIG_HPM_CTRL_HPM_INTEGER_INVERT_SHIFT (27U)
#define XCVR_PLL_DIG_HPM_CTRL_HPM_INTEGER_INVERT(x) (((uint32_t)(((uint32_t)(x)) << XCVR_PLL_DIG_HPM_CTRL_HPM_INTEGER_INVERT_SHIFT)) & XCVR_PLL_DIG_HPM_CTRL_HPM_INTEGER_INVERT_MASK)
#define XCVR_PLL_DIG_HPM_CTRL_HPM_CAL_INVERT_MASK (0x10000000U)
#define XCVR_PLL_DIG_HPM_CTRL_HPM_CAL_INVERT_SHIFT (28U)
#define XCVR_PLL_DIG_HPM_CTRL_HPM_CAL_INVERT(x)  (((uint32_t)(((uint32_t)(x)) << XCVR_PLL_DIG_HPM_CTRL_HPM_CAL_INVERT_SHIFT)) & XCVR_PLL_DIG_HPM_CTRL_HPM_CAL_INVERT_MASK)
#define XCVR_PLL_DIG_HPM_CTRL_HPM_MOD_IN_INVERT_MASK (0x80000000U)
#define XCVR_PLL_DIG_HPM_CTRL_HPM_MOD_IN_INVERT_SHIFT (31U)
#define XCVR_PLL_DIG_HPM_CTRL_HPM_MOD_IN_INVERT(x) (((uint32_t)(((uint32_t)(x)) << XCVR_PLL_DIG_HPM_CTRL_HPM_MOD_IN_INVERT_SHIFT)) & XCVR_PLL_DIG_HPM_CTRL_HPM_MOD_IN_INVERT_MASK)

/*! @name HPMCAL_CTRL - PLL High Port Calibration Control */
#define XCVR_PLL_DIG_HPMCAL_CTRL_HPM_CAL_FACTOR_MASK (0x1FFFU)
#define XCVR_PLL_DIG_HPMCAL_CTRL_HPM_CAL_FACTOR_SHIFT (0U)
#define XCVR_PLL_DIG_HPMCAL_CTRL_HPM_CAL_FACTOR(x) (((uint32_t)(((uint32_t)(x)) << XCVR_PLL_DIG_HPMCAL_CTRL_HPM_CAL_FACTOR_SHIFT)) & XCVR_PLL_DIG_HPMCAL_CTRL_HPM_CAL_FACTOR_MASK)
#define XCVR_PLL_DIG_HPMCAL_CTRL_HPM_CAL_NOT_BUMPED_MASK (0x2000U)
#define XCVR_PLL_DIG_HPMCAL_CTRL_HPM_CAL_NOT_BUMPED_SHIFT (13U)
#define XCVR_PLL_DIG_HPMCAL_CTRL_HPM_CAL_NOT_BUMPED(x) (((uint32_t)(((uint32_t)(x)) << XCVR_PLL_DIG_HPMCAL_CTRL_HPM_CAL_NOT_BUMPED_SHIFT)) & XCVR_PLL_DIG_HPMCAL_CTRL_HPM_CAL_NOT_BUMPED_MASK)
#define XCVR_PLL_DIG_HPMCAL_CTRL_HPM_CAL_COUNT_SCALE_MASK (0x4000U)
#define XCVR_PLL_DIG_HPMCAL_CTRL_HPM_CAL_COUNT_SCALE_SHIFT (14U)
#define XCVR_PLL_DIG_HPMCAL_CTRL_HPM_CAL_COUNT_SCALE(x) (((uint32_t)(((uint32_t)(x)) << XCVR_PLL_DIG_HPMCAL_CTRL_HPM_CAL_COUNT_SCALE_SHIFT)) & XCVR_PLL_DIG_HPMCAL_CTRL_HPM_CAL_COUNT_SCALE_MASK)
#define XCVR_PLL_DIG_HPMCAL_CTRL_HP_CAL_DISABLE_MASK (0x8000U)
#define XCVR_PLL_DIG_HPMCAL_CTRL_HP_CAL_DISABLE_SHIFT (15U)
#define XCVR_PLL_DIG_HPMCAL_CTRL_HP_CAL_DISABLE(x) (((uint32_t)(((uint32_t)(x)) << XCVR_PLL_DIG_HPMCAL_CTRL_HP_CAL_DISABLE_SHIFT)) & XCVR_PLL_DIG_HPMCAL_CTRL_HP_CAL_DISABLE_MASK)
#define XCVR_PLL_DIG_HPMCAL_CTRL_HPM_CAL_FACTOR_MANUAL_MASK (0x1FFF0000U)
#define XCVR_PLL_DIG_HPMCAL_CTRL_HPM_CAL_FACTOR_MANUAL_SHIFT (16U)
#define XCVR_PLL_DIG_HPMCAL_CTRL_HPM_CAL_FACTOR_MANUAL(x) (((uint32_t)(((uint32_t)(x)) << XCVR_PLL_DIG_HPMCAL_CTRL_HPM_CAL_FACTOR_MANUAL_SHIFT)) & XCVR_PLL_DIG_HPMCAL_CTRL_HPM_CAL_FACTOR_MANUAL_MASK)
#define XCVR_PLL_DIG_HPMCAL_CTRL_HPM_CAL_ARRAY_SIZE_MASK (0x40000000U)
#define XCVR_PLL_DIG_HPMCAL_CTRL_HPM_CAL_ARRAY_SIZE_SHIFT (30U)
#define XCVR_PLL_DIG_HPMCAL_CTRL_HPM_CAL_ARRAY_SIZE(x) (((uint32_t)(((uint32_t)(x)) << XCVR_PLL_DIG_HPMCAL_CTRL_HPM_CAL_ARRAY_SIZE_SHIFT)) & XCVR_PLL_DIG_HPMCAL_CTRL_HPM_CAL_ARRAY_SIZE_MASK)
#define XCVR_PLL_DIG_HPMCAL_CTRL_HPM_CAL_TIME_MASK (0x80000000U)
#define XCVR_PLL_DIG_HPMCAL_CTRL_HPM_CAL_TIME_SHIFT (31U)
#define XCVR_PLL_DIG_HPMCAL_CTRL_HPM_CAL_TIME(x) (((uint32_t)(((uint32_t)(x)) << XCVR_PLL_DIG_HPMCAL_CTRL_HPM_CAL_TIME_SHIFT)) & XCVR_PLL_DIG_HPMCAL_CTRL_HPM_CAL_TIME_MASK)

/*! @name HPM_CAL1 - PLL High Port Calibration Result 1 */
#define XCVR_PLL_DIG_HPM_CAL1_HPM_COUNT_1_MASK   (0x7FFFFU)
#define XCVR_PLL_DIG_HPM_CAL1_HPM_COUNT_1_SHIFT  (0U)
#define XCVR_PLL_DIG_HPM_CAL1_HPM_COUNT_1(x)     (((uint32_t)(((uint32_t)(x)) << XCVR_PLL_DIG_HPM_CAL1_HPM_COUNT_1_SHIFT)) & XCVR_PLL_DIG_HPM_CAL1_HPM_COUNT_1_MASK)
#define XCVR_PLL_DIG_HPM_CAL1_CS_WT_MASK         (0x700000U)
#define XCVR_PLL_DIG_HPM_CAL1_CS_WT_SHIFT        (20U)
#define XCVR_PLL_DIG_HPM_CAL1_CS_WT(x)           (((uint32_t)(((uint32_t)(x)) << XCVR_PLL_DIG_HPM_CAL1_CS_WT_SHIFT)) & XCVR_PLL_DIG_HPM_CAL1_CS_WT_MASK)
#define XCVR_PLL_DIG_HPM_CAL1_CS_FW_MASK         (0x7000000U)
#define XCVR_PLL_DIG_HPM_CAL1_CS_FW_SHIFT        (24U)
#define XCVR_PLL_DIG_HPM_CAL1_CS_FW(x)           (((uint32_t)(((uint32_t)(x)) << XCVR_PLL_DIG_HPM_CAL1_CS_FW_SHIFT)) & XCVR_PLL_DIG_HPM_CAL1_CS_FW_MASK)
#define XCVR_PLL_DIG_HPM_CAL1_CS_FCNT_MASK       (0xF0000000U)
#define XCVR_PLL_DIG_HPM_CAL1_CS_FCNT_SHIFT      (28U)
#define XCVR_PLL_DIG_HPM_CAL1_CS_FCNT(x)         (((uint32_t)(((uint32_t)(x)) << XCVR_PLL_DIG_HPM_CAL1_CS_FCNT_SHIFT)) & XCVR_PLL_DIG_HPM_CAL1_CS_FCNT_MASK)

/*! @name HPM_CAL2 - PLL High Port Calibration Result 2 */
#define XCVR_PLL_DIG_HPM_CAL2_HPM_COUNT_2_MASK   (0x7FFFFU)
#define XCVR_PLL_DIG_HPM_CAL2_HPM_COUNT_2_SHIFT  (0U)
#define XCVR_PLL_DIG_HPM_CAL2_HPM_COUNT_2(x)     (((uint32_t)(((uint32_t)(x)) << XCVR_PLL_DIG_HPM_CAL2_HPM_COUNT_2_SHIFT)) & XCVR_PLL_DIG_HPM_CAL2_HPM_COUNT_2_MASK)
#define XCVR_PLL_DIG_HPM_CAL2_CS_RC_MASK         (0x100000U)
#define XCVR_PLL_DIG_HPM_CAL2_CS_RC_SHIFT        (20U)
#define XCVR_PLL_DIG_HPM_CAL2_CS_RC(x)           (((uint32_t)(((uint32_t)(x)) << XCVR_PLL_DIG_HPM_CAL2_CS_RC_SHIFT)) & XCVR_PLL_DIG_HPM_CAL2_CS_RC_MASK)
#define XCVR_PLL_DIG_HPM_CAL2_CS_FT_MASK         (0x1F000000U)
#define XCVR_PLL_DIG_HPM_CAL2_CS_FT_SHIFT        (24U)
#define XCVR_PLL_DIG_HPM_CAL2_CS_FT(x)           (((uint32_t)(((uint32_t)(x)) << XCVR_PLL_DIG_HPM_CAL2_CS_FT_SHIFT)) & XCVR_PLL_DIG_HPM_CAL2_CS_FT_MASK)

/*! @name HPM_SDM_RES - PLL High Port Sigma Delta Results */
#define XCVR_PLL_DIG_HPM_SDM_RES_HPM_NUM_SELECTED_MASK (0x3FFU)
#define XCVR_PLL_DIG_HPM_SDM_RES_HPM_NUM_SELECTED_SHIFT (0U)
#define XCVR_PLL_DIG_HPM_SDM_RES_HPM_NUM_SELECTED(x) (((uint32_t)(((uint32_t)(x)) << XCVR_PLL_DIG_HPM_SDM_RES_HPM_NUM_SELECTED_SHIFT)) & XCVR_PLL_DIG_HPM_SDM_RES_HPM_NUM_SELECTED_MASK)
#define XCVR_PLL_DIG_HPM_SDM_RES_HPM_DENOM_MASK  (0x3FF0000U)
#define XCVR_PLL_DIG_HPM_SDM_RES_HPM_DENOM_SHIFT (16U)
#define XCVR_PLL_DIG_HPM_SDM_RES_HPM_DENOM(x)    (((uint32_t)(((uint32_t)(x)) << XCVR_PLL_DIG_HPM_SDM_RES_HPM_DENOM_SHIFT)) & XCVR_PLL_DIG_HPM_SDM_RES_HPM_DENOM_MASK)
#define XCVR_PLL_DIG_HPM_SDM_RES_HPM_COUNT_ADJUST_MASK (0xF0000000U)
#define XCVR_PLL_DIG_HPM_SDM_RES_HPM_COUNT_ADJUST_SHIFT (28U)
#define XCVR_PLL_DIG_HPM_SDM_RES_HPM_COUNT_ADJUST(x) (((uint32_t)(((uint32_t)(x)) << XCVR_PLL_DIG_HPM_SDM_RES_HPM_COUNT_ADJUST_SHIFT)) & XCVR_PLL_DIG_HPM_SDM_RES_HPM_COUNT_ADJUST_MASK)

/*! @name LPM_CTRL - PLL Low Port Modulator Control */
#define XCVR_PLL_DIG_LPM_CTRL_PLL_LD_MANUAL_MASK (0x3FU)
#define XCVR_PLL_DIG_LPM_CTRL_PLL_LD_MANUAL_SHIFT (0U)
#define XCVR_PLL_DIG_LPM_CTRL_PLL_LD_MANUAL(x)   (((uint32_t)(((uint32_t)(x)) << XCVR_PLL_DIG_LPM_CTRL_PLL_LD_MANUAL_SHIFT)) & XCVR_PLL_DIG_LPM_CTRL_PLL_LD_MANUAL_MASK)
#define XCVR_PLL_DIG_LPM_CTRL_PLL_LD_DISABLE_MASK (0x800U)
#define XCVR_PLL_DIG_LPM_CTRL_PLL_LD_DISABLE_SHIFT (11U)
#define XCVR_PLL_DIG_LPM_CTRL_PLL_LD_DISABLE(x)  (((uint32_t)(((uint32_t)(x)) << XCVR_PLL_DIG_LPM_CTRL_PLL_LD_DISABLE_SHIFT)) & XCVR_PLL_DIG_LPM_CTRL_PLL_LD_DISABLE_MASK)
#define XCVR_PLL_DIG_LPM_CTRL_LPFF_MASK          (0x2000U)
#define XCVR_PLL_DIG_LPM_CTRL_LPFF_SHIFT         (13U)
#define XCVR_PLL_DIG_LPM_CTRL_LPFF(x)            (((uint32_t)(((uint32_t)(x)) << XCVR_PLL_DIG_LPM_CTRL_LPFF_SHIFT)) & XCVR_PLL_DIG_LPM_CTRL_LPFF_MASK)
#define XCVR_PLL_DIG_LPM_CTRL_LPM_SDM_INV_MASK   (0x4000U)
#define XCVR_PLL_DIG_LPM_CTRL_LPM_SDM_INV_SHIFT  (14U)
#define XCVR_PLL_DIG_LPM_CTRL_LPM_SDM_INV(x)     (((uint32_t)(((uint32_t)(x)) << XCVR_PLL_DIG_LPM_CTRL_LPM_SDM_INV_SHIFT)) & XCVR_PLL_DIG_LPM_CTRL_LPM_SDM_INV_MASK)
#define XCVR_PLL_DIG_LPM_CTRL_LPM_DISABLE_MASK   (0x8000U)
#define XCVR_PLL_DIG_LPM_CTRL_LPM_DISABLE_SHIFT  (15U)
#define XCVR_PLL_DIG_LPM_CTRL_LPM_DISABLE(x)     (((uint32_t)(((uint32_t)(x)) << XCVR_PLL_DIG_LPM_CTRL_LPM_DISABLE_SHIFT)) & XCVR_PLL_DIG_LPM_CTRL_LPM_DISABLE_MASK)
#define XCVR_PLL_DIG_LPM_CTRL_LPM_DTH_SCL_MASK   (0xF0000U)
#define XCVR_PLL_DIG_LPM_CTRL_LPM_DTH_SCL_SHIFT  (16U)
#define XCVR_PLL_DIG_LPM_CTRL_LPM_DTH_SCL(x)     (((uint32_t)(((uint32_t)(x)) << XCVR_PLL_DIG_LPM_CTRL_LPM_DTH_SCL_SHIFT)) & XCVR_PLL_DIG_LPM_CTRL_LPM_DTH_SCL_MASK)
#define XCVR_PLL_DIG_LPM_CTRL_LPM_D_CTRL_MASK    (0x400000U)
#define XCVR_PLL_DIG_LPM_CTRL_LPM_D_CTRL_SHIFT   (22U)
#define XCVR_PLL_DIG_LPM_CTRL_LPM_D_CTRL(x)      (((uint32_t)(((uint32_t)(x)) << XCVR_PLL_DIG_LPM_CTRL_LPM_D_CTRL_SHIFT)) & XCVR_PLL_DIG_LPM_CTRL_LPM_D_CTRL_MASK)
#define XCVR_PLL_DIG_LPM_CTRL_LPM_D_OVRD_MASK    (0x800000U)
#define XCVR_PLL_DIG_LPM_CTRL_LPM_D_OVRD_SHIFT   (23U)
#define XCVR_PLL_DIG_LPM_CTRL_LPM_D_OVRD(x)      (((uint32_t)(((uint32_t)(x)) << XCVR_PLL_DIG_LPM_CTRL_LPM_D_OVRD_SHIFT)) & XCVR_PLL_DIG_LPM_CTRL_LPM_D_OVRD_MASK)
#define XCVR_PLL_DIG_LPM_CTRL_LPM_SCALE_MASK     (0xF000000U)
#define XCVR_PLL_DIG_LPM_CTRL_LPM_SCALE_SHIFT    (24U)
#define XCVR_PLL_DIG_LPM_CTRL_LPM_SCALE(x)       (((uint32_t)(((uint32_t)(x)) << XCVR_PLL_DIG_LPM_CTRL_LPM_SCALE_SHIFT)) & XCVR_PLL_DIG_LPM_CTRL_LPM_SCALE_MASK)
#define XCVR_PLL_DIG_LPM_CTRL_LPM_SDM_USE_NEG_MASK (0x80000000U)
#define XCVR_PLL_DIG_LPM_CTRL_LPM_SDM_USE_NEG_SHIFT (31U)
#define XCVR_PLL_DIG_LPM_CTRL_LPM_SDM_USE_NEG(x) (((uint32_t)(((uint32_t)(x)) << XCVR_PLL_DIG_LPM_CTRL_LPM_SDM_USE_NEG_SHIFT)) & XCVR_PLL_DIG_LPM_CTRL_LPM_SDM_USE_NEG_MASK)

/*! @name LPM_SDM_CTRL1 - PLL Low Port Sigma Delta Control 1 */
#define XCVR_PLL_DIG_LPM_SDM_CTRL1_LPM_INTG_SELECTED_MASK (0x7FU)
#define XCVR_PLL_DIG_LPM_SDM_CTRL1_LPM_INTG_SELECTED_SHIFT (0U)
#define XCVR_PLL_DIG_LPM_SDM_CTRL1_LPM_INTG_SELECTED(x) (((uint32_t)(((uint32_t)(x)) << XCVR_PLL_DIG_LPM_SDM_CTRL1_LPM_INTG_SELECTED_SHIFT)) & XCVR_PLL_DIG_LPM_SDM_CTRL1_LPM_INTG_SELECTED_MASK)
#define XCVR_PLL_DIG_LPM_SDM_CTRL1_HPM_ARRAY_BIAS_MASK (0x7F00U)
#define XCVR_PLL_DIG_LPM_SDM_CTRL1_HPM_ARRAY_BIAS_SHIFT (8U)
#define XCVR_PLL_DIG_LPM_SDM_CTRL1_HPM_ARRAY_BIAS(x) (((uint32_t)(((uint32_t)(x)) << XCVR_PLL_DIG_LPM_SDM_CTRL1_HPM_ARRAY_BIAS_SHIFT)) & XCVR_PLL_DIG_LPM_SDM_CTRL1_HPM_ARRAY_BIAS_MASK)
#define XCVR_PLL_DIG_LPM_SDM_CTRL1_LPM_INTG_MASK (0x7F0000U)
#define XCVR_PLL_DIG_LPM_SDM_CTRL1_LPM_INTG_SHIFT (16U)
#define XCVR_PLL_DIG_LPM_SDM_CTRL1_LPM_INTG(x)   (((uint32_t)(((uint32_t)(x)) << XCVR_PLL_DIG_LPM_SDM_CTRL1_LPM_INTG_SHIFT)) & XCVR_PLL_DIG_LPM_SDM_CTRL1_LPM_INTG_MASK)
#define XCVR_PLL_DIG_LPM_SDM_CTRL1_SDM_MAP_DISABLE_MASK (0x80000000U)
#define XCVR_PLL_DIG_LPM_SDM_CTRL1_SDM_MAP_DISABLE_SHIFT (31U)
#define XCVR_PLL_DIG_LPM_SDM_CTRL1_SDM_MAP_DISABLE(x) (((uint32_t)(((uint32_t)(x)) << XCVR_PLL_DIG_LPM_SDM_CTRL1_SDM_MAP_DISABLE_SHIFT)) & XCVR_PLL_DIG_LPM_SDM_CTRL1_SDM_MAP_DISABLE_MASK)

/*! @name LPM_SDM_CTRL2 - PLL Low Port Sigma Delta Control 2 */
#define XCVR_PLL_DIG_LPM_SDM_CTRL2_LPM_NUM_MASK  (0xFFFFFFFU)
#define XCVR_PLL_DIG_LPM_SDM_CTRL2_LPM_NUM_SHIFT (0U)
#define XCVR_PLL_DIG_LPM_SDM_CTRL2_LPM_NUM(x)    (((uint32_t)(((uint32_t)(x)) << XCVR_PLL_DIG_LPM_SDM_CTRL2_LPM_NUM_SHIFT)) & XCVR_PLL_DIG_LPM_SDM_CTRL2_LPM_NUM_MASK)

/*! @name LPM_SDM_CTRL3 - PLL Low Port Sigma Delta Control 3 */
#define XCVR_PLL_DIG_LPM_SDM_CTRL3_LPM_DENOM_MASK (0xFFFFFFFU)
#define XCVR_PLL_DIG_LPM_SDM_CTRL3_LPM_DENOM_SHIFT (0U)
#define XCVR_PLL_DIG_LPM_SDM_CTRL3_LPM_DENOM(x)  (((uint32_t)(((uint32_t)(x)) << XCVR_PLL_DIG_LPM_SDM_CTRL3_LPM_DENOM_SHIFT)) & XCVR_PLL_DIG_LPM_SDM_CTRL3_LPM_DENOM_MASK)

/*! @name LPM_SDM_RES1 - PLL Low Port Sigma Delta Result 1 */
#define XCVR_PLL_DIG_LPM_SDM_RES1_LPM_NUM_SELECTED_MASK (0xFFFFFFFU)
#define XCVR_PLL_DIG_LPM_SDM_RES1_LPM_NUM_SELECTED_SHIFT (0U)
#define XCVR_PLL_DIG_LPM_SDM_RES1_LPM_NUM_SELECTED(x) (((uint32_t)(((uint32_t)(x)) << XCVR_PLL_DIG_LPM_SDM_RES1_LPM_NUM_SELECTED_SHIFT)) & XCVR_PLL_DIG_LPM_SDM_RES1_LPM_NUM_SELECTED_MASK)

/*! @name LPM_SDM_RES2 - PLL Low Port Sigma Delta Result 2 */
#define XCVR_PLL_DIG_LPM_SDM_RES2_LPM_DENOM_SELECTED_MASK (0xFFFFFFFU)
#define XCVR_PLL_DIG_LPM_SDM_RES2_LPM_DENOM_SELECTED_SHIFT (0U)
#define XCVR_PLL_DIG_LPM_SDM_RES2_LPM_DENOM_SELECTED(x) (((uint32_t)(((uint32_t)(x)) << XCVR_PLL_DIG_LPM_SDM_RES2_LPM_DENOM_SELECTED_SHIFT)) & XCVR_PLL_DIG_LPM_SDM_RES2_LPM_DENOM_SELECTED_MASK)

/*! @name DELAY_MATCH - PLL Delay Matching */
#define XCVR_PLL_DIG_DELAY_MATCH_LPM_SDM_DELAY_MASK (0xFU)
#define XCVR_PLL_DIG_DELAY_MATCH_LPM_SDM_DELAY_SHIFT (0U)
#define XCVR_PLL_DIG_DELAY_MATCH_LPM_SDM_DELAY(x) (((uint32_t)(((uint32_t)(x)) << XCVR_PLL_DIG_DELAY_MATCH_LPM_SDM_DELAY_SHIFT)) & XCVR_PLL_DIG_DELAY_MATCH_LPM_SDM_DELAY_MASK)
#define XCVR_PLL_DIG_DELAY_MATCH_HPM_SDM_DELAY_MASK (0xF00U)
#define XCVR_PLL_DIG_DELAY_MATCH_HPM_SDM_DELAY_SHIFT (8U)
#define XCVR_PLL_DIG_DELAY_MATCH_HPM_SDM_DELAY(x) (((uint32_t)(((uint32_t)(x)) << XCVR_PLL_DIG_DELAY_MATCH_HPM_SDM_DELAY_SHIFT)) & XCVR_PLL_DIG_DELAY_MATCH_HPM_SDM_DELAY_MASK)
#define XCVR_PLL_DIG_DELAY_MATCH_HPM_INTEGER_DELAY_MASK (0xF0000U)
#define XCVR_PLL_DIG_DELAY_MATCH_HPM_INTEGER_DELAY_SHIFT (16U)
#define XCVR_PLL_DIG_DELAY_MATCH_HPM_INTEGER_DELAY(x) (((uint32_t)(((uint32_t)(x)) << XCVR_PLL_DIG_DELAY_MATCH_HPM_INTEGER_DELAY_SHIFT)) & XCVR_PLL_DIG_DELAY_MATCH_HPM_INTEGER_DELAY_MASK)

/*! @name CTUNE_CTRL - PLL Coarse Tune Control */
#define XCVR_PLL_DIG_CTUNE_CTRL_CTUNE_TARGET_MANUAL_MASK (0xFFFU)
#define XCVR_PLL_DIG_CTUNE_CTRL_CTUNE_TARGET_MANUAL_SHIFT (0U)
#define XCVR_PLL_DIG_CTUNE_CTRL_CTUNE_TARGET_MANUAL(x) (((uint32_t)(((uint32_t)(x)) << XCVR_PLL_DIG_CTUNE_CTRL_CTUNE_TARGET_MANUAL_SHIFT)) & XCVR_PLL_DIG_CTUNE_CTRL_CTUNE_TARGET_MANUAL_MASK)
#define XCVR_PLL_DIG_CTUNE_CTRL_CTUNE_TARGET_DISABLE_MASK (0x8000U)
#define XCVR_PLL_DIG_CTUNE_CTRL_CTUNE_TARGET_DISABLE_SHIFT (15U)
#define XCVR_PLL_DIG_CTUNE_CTRL_CTUNE_TARGET_DISABLE(x) (((uint32_t)(((uint32_t)(x)) << XCVR_PLL_DIG_CTUNE_CTRL_CTUNE_TARGET_DISABLE_SHIFT)) & XCVR_PLL_DIG_CTUNE_CTRL_CTUNE_TARGET_DISABLE_MASK)
#define XCVR_PLL_DIG_CTUNE_CTRL_CTUNE_ADJUST_MASK (0xF0000U)
#define XCVR_PLL_DIG_CTUNE_CTRL_CTUNE_ADJUST_SHIFT (16U)
#define XCVR_PLL_DIG_CTUNE_CTRL_CTUNE_ADJUST(x)  (((uint32_t)(((uint32_t)(x)) << XCVR_PLL_DIG_CTUNE_CTRL_CTUNE_ADJUST_SHIFT)) & XCVR_PLL_DIG_CTUNE_CTRL_CTUNE_ADJUST_MASK)
#define XCVR_PLL_DIG_CTUNE_CTRL_CTUNE_MANUAL_MASK (0x7F000000U)
#define XCVR_PLL_DIG_CTUNE_CTRL_CTUNE_MANUAL_SHIFT (24U)
#define XCVR_PLL_DIG_CTUNE_CTRL_CTUNE_MANUAL(x)  (((uint32_t)(((uint32_t)(x)) << XCVR_PLL_DIG_CTUNE_CTRL_CTUNE_MANUAL_SHIFT)) & XCVR_PLL_DIG_CTUNE_CTRL_CTUNE_MANUAL_MASK)
#define XCVR_PLL_DIG_CTUNE_CTRL_CTUNE_DISABLE_MASK (0x80000000U)
#define XCVR_PLL_DIG_CTUNE_CTRL_CTUNE_DISABLE_SHIFT (31U)
#define XCVR_PLL_DIG_CTUNE_CTRL_CTUNE_DISABLE(x) (((uint32_t)(((uint32_t)(x)) << XCVR_PLL_DIG_CTUNE_CTRL_CTUNE_DISABLE_SHIFT)) & XCVR_PLL_DIG_CTUNE_CTRL_CTUNE_DISABLE_MASK)

/*! @name CTUNE_CNT6 - PLL Coarse Tune Count 6 */
#define XCVR_PLL_DIG_CTUNE_CNT6_CTUNE_COUNT_6_MASK (0x1FFFU)
#define XCVR_PLL_DIG_CTUNE_CNT6_CTUNE_COUNT_6_SHIFT (0U)
#define XCVR_PLL_DIG_CTUNE_CNT6_CTUNE_COUNT_6(x) (((uint32_t)(((uint32_t)(x)) << XCVR_PLL_DIG_CTUNE_CNT6_CTUNE_COUNT_6_SHIFT)) & XCVR_PLL_DIG_CTUNE_CNT6_CTUNE_COUNT_6_MASK)

/*! @name CTUNE_CNT5_4 - PLL Coarse Tune Counts 5 and 4 */
#define XCVR_PLL_DIG_CTUNE_CNT5_4_CTUNE_COUNT_4_MASK (0x1FFFU)
#define XCVR_PLL_DIG_CTUNE_CNT5_4_CTUNE_COUNT_4_SHIFT (0U)
#define XCVR_PLL_DIG_CTUNE_CNT5_4_CTUNE_COUNT_4(x) (((uint32_t)(((uint32_t)(x)) << XCVR_PLL_DIG_CTUNE_CNT5_4_CTUNE_COUNT_4_SHIFT)) & XCVR_PLL_DIG_CTUNE_CNT5_4_CTUNE_COUNT_4_MASK)
#define XCVR_PLL_DIG_CTUNE_CNT5_4_CTUNE_COUNT_5_MASK (0x1FFF0000U)
#define XCVR_PLL_DIG_CTUNE_CNT5_4_CTUNE_COUNT_5_SHIFT (16U)
#define XCVR_PLL_DIG_CTUNE_CNT5_4_CTUNE_COUNT_5(x) (((uint32_t)(((uint32_t)(x)) << XCVR_PLL_DIG_CTUNE_CNT5_4_CTUNE_COUNT_5_SHIFT)) & XCVR_PLL_DIG_CTUNE_CNT5_4_CTUNE_COUNT_5_MASK)

/*! @name CTUNE_CNT3_2 - PLL Coarse Tune Counts 3 and 2 */
#define XCVR_PLL_DIG_CTUNE_CNT3_2_CTUNE_COUNT_2_MASK (0x1FFFU)
#define XCVR_PLL_DIG_CTUNE_CNT3_2_CTUNE_COUNT_2_SHIFT (0U)
#define XCVR_PLL_DIG_CTUNE_CNT3_2_CTUNE_COUNT_2(x) (((uint32_t)(((uint32_t)(x)) << XCVR_PLL_DIG_CTUNE_CNT3_2_CTUNE_COUNT_2_SHIFT)) & XCVR_PLL_DIG_CTUNE_CNT3_2_CTUNE_COUNT_2_MASK)
#define XCVR_PLL_DIG_CTUNE_CNT3_2_CTUNE_COUNT_3_MASK (0x1FFF0000U)
#define XCVR_PLL_DIG_CTUNE_CNT3_2_CTUNE_COUNT_3_SHIFT (16U)
#define XCVR_PLL_DIG_CTUNE_CNT3_2_CTUNE_COUNT_3(x) (((uint32_t)(((uint32_t)(x)) << XCVR_PLL_DIG_CTUNE_CNT3_2_CTUNE_COUNT_3_SHIFT)) & XCVR_PLL_DIG_CTUNE_CNT3_2_CTUNE_COUNT_3_MASK)

/*! @name CTUNE_CNT1_0 - PLL Coarse Tune Counts 1 and 0 */
#define XCVR_PLL_DIG_CTUNE_CNT1_0_CTUNE_COUNT_0_MASK (0x1FFFU)
#define XCVR_PLL_DIG_CTUNE_CNT1_0_CTUNE_COUNT_0_SHIFT (0U)
#define XCVR_PLL_DIG_CTUNE_CNT1_0_CTUNE_COUNT_0(x) (((uint32_t)(((uint32_t)(x)) << XCVR_PLL_DIG_CTUNE_CNT1_0_CTUNE_COUNT_0_SHIFT)) & XCVR_PLL_DIG_CTUNE_CNT1_0_CTUNE_COUNT_0_MASK)
#define XCVR_PLL_DIG_CTUNE_CNT1_0_CTUNE_COUNT_1_MASK (0x1FFF0000U)
#define XCVR_PLL_DIG_CTUNE_CNT1_0_CTUNE_COUNT_1_SHIFT (16U)
#define XCVR_PLL_DIG_CTUNE_CNT1_0_CTUNE_COUNT_1(x) (((uint32_t)(((uint32_t)(x)) << XCVR_PLL_DIG_CTUNE_CNT1_0_CTUNE_COUNT_1_SHIFT)) & XCVR_PLL_DIG_CTUNE_CNT1_0_CTUNE_COUNT_1_MASK)

/*! @name CTUNE_RES - PLL Coarse Tune Results */
#define XCVR_PLL_DIG_CTUNE_RES_CTUNE_SELECTED_MASK (0x7FU)
#define XCVR_PLL_DIG_CTUNE_RES_CTUNE_SELECTED_SHIFT (0U)
#define XCVR_PLL_DIG_CTUNE_RES_CTUNE_SELECTED(x) (((uint32_t)(((uint32_t)(x)) << XCVR_PLL_DIG_CTUNE_RES_CTUNE_SELECTED_SHIFT)) & XCVR_PLL_DIG_CTUNE_RES_CTUNE_SELECTED_MASK)
#define XCVR_PLL_DIG_CTUNE_RES_CTUNE_BEST_DIFF_MASK (0xFF00U)
#define XCVR_PLL_DIG_CTUNE_RES_CTUNE_BEST_DIFF_SHIFT (8U)
#define XCVR_PLL_DIG_CTUNE_RES_CTUNE_BEST_DIFF(x) (((uint32_t)(((uint32_t)(x)) << XCVR_PLL_DIG_CTUNE_RES_CTUNE_BEST_DIFF_SHIFT)) & XCVR_PLL_DIG_CTUNE_RES_CTUNE_BEST_DIFF_MASK)
#define XCVR_PLL_DIG_CTUNE_RES_CTUNE_FREQ_SELECTED_MASK (0xFFF0000U)
#define XCVR_PLL_DIG_CTUNE_RES_CTUNE_FREQ_SELECTED_SHIFT (16U)
#define XCVR_PLL_DIG_CTUNE_RES_CTUNE_FREQ_SELECTED(x) (((uint32_t)(((uint32_t)(x)) << XCVR_PLL_DIG_CTUNE_RES_CTUNE_FREQ_SELECTED_SHIFT)) & XCVR_PLL_DIG_CTUNE_RES_CTUNE_FREQ_SELECTED_MASK)


/*!
 * @}
 */ /* end of group XCVR_PLL_DIG_Register_Masks */


/* XCVR_PLL_DIG - Peripheral instance base addresses */
/** Peripheral XCVR_PLL_DIG base address */
#define XCVR_PLL_DIG_BASE                        (0x4005C224u)
/** Peripheral XCVR_PLL_DIG base pointer */
#define XCVR_PLL_DIG                             ((XCVR_PLL_DIG_Type *)XCVR_PLL_DIG_BASE)
/** Array initializer of XCVR_PLL_DIG peripheral base addresses */
#define XCVR_PLL_DIG_BASE_ADDRS                  { XCVR_PLL_DIG_BASE }
/** Array initializer of XCVR_PLL_DIG peripheral base pointers */
#define XCVR_PLL_DIG_BASE_PTRS                   { XCVR_PLL_DIG }

/*!
 * @}
 */ /* end of group XCVR_PLL_DIG_Peripheral_Access_Layer */


/* ----------------------------------------------------------------------------
   -- XCVR_RX_DIG Peripheral Access Layer
   ---------------------------------------------------------------------------- */

/*!
 * @addtogroup XCVR_RX_DIG_Peripheral_Access_Layer XCVR_RX_DIG Peripheral Access Layer
 * @{
 */

/** XCVR_RX_DIG - Register Layout Typedef */
typedef struct {
  __IO uint32_t RX_DIG_CTRL;                       /**< RX Digital Control, offset: 0x0 */
  __IO uint32_t AGC_CTRL_0;                        /**< AGC Control 0, offset: 0x4 */
  __IO uint32_t AGC_CTRL_1;                        /**< AGC Control 1, offset: 0x8 */
  __IO uint32_t AGC_CTRL_2;                        /**< AGC Control 2, offset: 0xC */
  __IO uint32_t AGC_CTRL_3;                        /**< AGC Control 3, offset: 0x10 */
  __I  uint32_t AGC_STAT;                          /**< AGC Status, offset: 0x14 */
  __IO uint32_t RSSI_CTRL_0;                       /**< RSSI Control 0, offset: 0x18 */
  __I  uint32_t RSSI_CTRL_1;                       /**< RSSI Control 1, offset: 0x1C */
  __I  uint32_t RSSI_DFT;                          /**< RSSI DFT, offset: 0x20 */
  __IO uint32_t DCOC_CTRL_0;                       /**< DCOC Control 0, offset: 0x24 */
  __IO uint32_t DCOC_CTRL_1;                       /**< DCOC Control 1, offset: 0x28 */
  __IO uint32_t DCOC_DAC_INIT;                     /**< DCOC DAC Initialization, offset: 0x2C */
  __IO uint32_t DCOC_DIG_MAN;                      /**< DCOC Digital Correction Manual Override, offset: 0x30 */
  __IO uint32_t DCOC_CAL_GAIN;                     /**< DCOC Calibration Gain, offset: 0x34 */
  __I  uint32_t DCOC_STAT;                         /**< DCOC Status, offset: 0x38 */
  __I  uint32_t DCOC_DC_EST;                       /**< DCOC DC Estimate, offset: 0x3C */
  __IO uint32_t DCOC_CAL_RCP;                      /**< DCOC Calibration Reciprocals, offset: 0x40 */
       uint8_t RESERVED_0[4];
  __IO uint32_t IQMC_CTRL;                         /**< IQMC Control, offset: 0x48 */
  __IO uint32_t IQMC_CAL;                          /**< IQMC Calibration, offset: 0x4C */
  __IO uint32_t LNA_GAIN_VAL_3_0;                  /**< LNA_GAIN Step Values 3..0, offset: 0x50 */
  __IO uint32_t LNA_GAIN_VAL_7_4;                  /**< LNA_GAIN Step Values 7..4, offset: 0x54 */
  __IO uint32_t LNA_GAIN_VAL_8;                    /**< LNA_GAIN Step Values 8, offset: 0x58 */
  __IO uint32_t BBA_RES_TUNE_VAL_7_0;              /**< BBA Resistor Tune Values 7..0, offset: 0x5C */
  __IO uint32_t BBA_RES_TUNE_VAL_10_8;             /**< BBA Resistor Tune Values 10..8, offset: 0x60 */
  __IO uint32_t LNA_GAIN_LIN_VAL_2_0;              /**< LNA Linear Gain Values 2..0, offset: 0x64 */
  __IO uint32_t LNA_GAIN_LIN_VAL_5_3;              /**< LNA Linear Gain Values 5..3, offset: 0x68 */
  __IO uint32_t LNA_GAIN_LIN_VAL_8_6;              /**< LNA Linear Gain Values 8..6, offset: 0x6C */
  __IO uint32_t LNA_GAIN_LIN_VAL_9;                /**< LNA Linear Gain Values 9, offset: 0x70 */
  __IO uint32_t BBA_RES_TUNE_LIN_VAL_3_0;          /**< BBA Resistor Tune Values 3..0, offset: 0x74 */
  __IO uint32_t BBA_RES_TUNE_LIN_VAL_7_4;          /**< BBA Resistor Tune Values 7..4, offset: 0x78 */
  __IO uint32_t BBA_RES_TUNE_LIN_VAL_10_8;         /**< BBA Resistor Tune Values 10..8, offset: 0x7C */
  __IO uint32_t AGC_GAIN_TBL_03_00;                /**< AGC Gain Tables Step 03..00, offset: 0x80 */
  __IO uint32_t AGC_GAIN_TBL_07_04;                /**< AGC Gain Tables Step 07..04, offset: 0x84 */
  __IO uint32_t AGC_GAIN_TBL_11_08;                /**< AGC Gain Tables Step 11..08, offset: 0x88 */
  __IO uint32_t AGC_GAIN_TBL_15_12;                /**< AGC Gain Tables Step 15..12, offset: 0x8C */
  __IO uint32_t AGC_GAIN_TBL_19_16;                /**< AGC Gain Tables Step 19..16, offset: 0x90 */
  __IO uint32_t AGC_GAIN_TBL_23_20;                /**< AGC Gain Tables Step 23..20, offset: 0x94 */
  __IO uint32_t AGC_GAIN_TBL_26_24;                /**< AGC Gain Tables Step 26..24, offset: 0x98 */
       uint8_t RESERVED_1[4];
  __IO uint32_t DCOC_OFFSET[27];                   /**< DCOC Offset, array offset: 0xA0, array step: 0x4 */
  __IO uint32_t DCOC_BBA_STEP;                     /**< DCOC BBA DAC Step, offset: 0x10C */
  __IO uint32_t DCOC_TZA_STEP_0;                   /**< DCOC TZA DAC Step 0, offset: 0x110 */
  __IO uint32_t DCOC_TZA_STEP_1;                   /**< DCOC TZA DAC Step 1, offset: 0x114 */
  __IO uint32_t DCOC_TZA_STEP_2;                   /**< DCOC TZA DAC Step 2, offset: 0x118 */
  __IO uint32_t DCOC_TZA_STEP_3;                   /**< DCOC TZA DAC Step 3, offset: 0x11C */
  __IO uint32_t DCOC_TZA_STEP_4;                   /**< DCOC TZA DAC Step 4, offset: 0x120 */
  __IO uint32_t DCOC_TZA_STEP_5;                   /**< DCOC TZA DAC Step 5, offset: 0x124 */
  __IO uint32_t DCOC_TZA_STEP_6;                   /**< DCOC TZA DAC Step 6, offset: 0x128 */
  __IO uint32_t DCOC_TZA_STEP_7;                   /**< DCOC TZA DAC Step 7, offset: 0x12C */
  __IO uint32_t DCOC_TZA_STEP_8;                   /**< DCOC TZA DAC Step 5, offset: 0x130 */
  __IO uint32_t DCOC_TZA_STEP_9;                   /**< DCOC TZA DAC Step 9, offset: 0x134 */
  __IO uint32_t DCOC_TZA_STEP_10;                  /**< DCOC TZA DAC Step 10, offset: 0x138 */
       uint8_t RESERVED_2[44];
  __I  uint32_t DCOC_CAL_ALPHA;                    /**< DCOC Calibration Alpha, offset: 0x168 */
  __I  uint32_t DCOC_CAL_BETA_Q;                   /**< DCOC Calibration Beta Q, offset: 0x16C */
  __I  uint32_t DCOC_CAL_BETA_I;                   /**< DCOC Calibration Beta I, offset: 0x170 */
  __I  uint32_t DCOC_CAL_GAMMA;                    /**< DCOC Calibration Gamma, offset: 0x174 */
  __IO uint32_t DCOC_CAL_IIR;                      /**< DCOC Calibration IIR, offset: 0x178 */
       uint8_t RESERVED_3[4];
  __I  uint32_t DCOC_CAL[3];                       /**< DCOC Calibration Result, array offset: 0x180, array step: 0x4 */
       uint8_t RESERVED_4[4];
  __IO uint32_t CCA_ED_LQI_CTRL_0;                 /**< RX_DIG CCA ED LQI Control Register 0, offset: 0x190 */
  __IO uint32_t CCA_ED_LQI_CTRL_1;                 /**< RX_DIG CCA ED LQI Control Register 1, offset: 0x194 */
  __I  uint32_t CCA_ED_LQI_STAT_0;                 /**< RX_DIG CCA ED LQI Status Register 0, offset: 0x198 */
       uint8_t RESERVED_5[4];
  __IO uint32_t RX_CHF_COEF_0;                     /**< Receive Channel Filter Coefficient 0, offset: 0x1A0 */
  __IO uint32_t RX_CHF_COEF_1;                     /**< Receive Channel Filter Coefficient 1, offset: 0x1A4 */
  __IO uint32_t RX_CHF_COEF_2;                     /**< Receive Channel Filter Coefficient 2, offset: 0x1A8 */
  __IO uint32_t RX_CHF_COEF_3;                     /**< Receive Channel Filter Coefficient 3, offset: 0x1AC */
  __IO uint32_t RX_CHF_COEF_4;                     /**< Receive Channel Filter Coefficient 4, offset: 0x1B0 */
  __IO uint32_t RX_CHF_COEF_5;                     /**< Receive Channel Filter Coefficient 5, offset: 0x1B4 */
  __IO uint32_t RX_CHF_COEF_6;                     /**< Receive Channel Filter Coefficient 6, offset: 0x1B8 */
  __IO uint32_t RX_CHF_COEF_7;                     /**< Receive Channel Filter Coefficient 7, offset: 0x1BC */
  __IO uint32_t RX_CHF_COEF_8;                     /**< Receive Channel Filter Coefficient 8, offset: 0x1C0 */
  __IO uint32_t RX_CHF_COEF_9;                     /**< Receive Channel Filter Coefficient 9, offset: 0x1C4 */
  __IO uint32_t RX_CHF_COEF_10;                    /**< Receive Channel Filter Coefficient 10, offset: 0x1C8 */
  __IO uint32_t RX_CHF_COEF_11;                    /**< Receive Channel Filter Coefficient 11, offset: 0x1CC */
  __IO uint32_t AGC_MAN_AGC_IDX;                   /**< AGC Manual AGC Index, offset: 0x1D0 */
  __IO uint32_t DC_RESID_CTRL;                     /**< DC Residual Control, offset: 0x1D4 */
  __I  uint32_t DC_RESID_EST;                      /**< DC Residual Estimate, offset: 0x1D8 */
  __IO uint32_t RX_RCCAL_CTRL0;                    /**< RX RC Calibration Control0, offset: 0x1DC */
  __IO uint32_t RX_RCCAL_CTRL1;                    /**< RX RC Calibration Control1, offset: 0x1E0 */
  __I  uint32_t RX_RCCAL_STAT;                     /**< RX RC Calibration Status, offset: 0x1E4 */
  __IO uint32_t AUXPLL_FCAL_CTRL;                  /**< Aux PLL Frequency Calibration Control, offset: 0x1E8 */
  __I  uint32_t AUXPLL_FCAL_CNT6;                  /**< Aux PLL Frequency Calibration Count 6, offset: 0x1EC */
  __I  uint32_t AUXPLL_FCAL_CNT5_4;                /**< Aux PLL Frequency Calibration Count 5 and 4, offset: 0x1F0 */
  __I  uint32_t AUXPLL_FCAL_CNT3_2;                /**< Aux PLL Frequency Calibration Count 3 and 2, offset: 0x1F4 */
  __I  uint32_t AUXPLL_FCAL_CNT1_0;                /**< Aux PLL Frequency Calibration Count 1 and 0, offset: 0x1F8 */
  __IO uint32_t RXDIG_DFT;                         /**< RXDIG DFT, offset: 0x1FC */
} XCVR_RX_DIG_Type;

/* ----------------------------------------------------------------------------
   -- XCVR_RX_DIG Register Masks
   ---------------------------------------------------------------------------- */

/*!
 * @addtogroup XCVR_RX_DIG_Register_Masks XCVR_RX_DIG Register Masks
 * @{
 */

/*! @name RX_DIG_CTRL - RX Digital Control */
#define XCVR_RX_DIG_RX_DIG_CTRL_RX_ADC_NEGEDGE_MASK (0x1U)
#define XCVR_RX_DIG_RX_DIG_CTRL_RX_ADC_NEGEDGE_SHIFT (0U)
#define XCVR_RX_DIG_RX_DIG_CTRL_RX_ADC_NEGEDGE(x) (((uint32_t)(((uint32_t)(x)) << XCVR_RX_DIG_RX_DIG_CTRL_RX_ADC_NEGEDGE_SHIFT)) & XCVR_RX_DIG_RX_DIG_CTRL_RX_ADC_NEGEDGE_MASK)
#define XCVR_RX_DIG_RX_DIG_CTRL_RX_CH_FILT_BYPASS_MASK (0x2U)
#define XCVR_RX_DIG_RX_DIG_CTRL_RX_CH_FILT_BYPASS_SHIFT (1U)
#define XCVR_RX_DIG_RX_DIG_CTRL_RX_CH_FILT_BYPASS(x) (((uint32_t)(((uint32_t)(x)) << XCVR_RX_DIG_RX_DIG_CTRL_RX_CH_FILT_BYPASS_SHIFT)) & XCVR_RX_DIG_RX_DIG_CTRL_RX_CH_FILT_BYPASS_MASK)
#define XCVR_RX_DIG_RX_DIG_CTRL_RX_ADC_RAW_EN_MASK (0x4U)
#define XCVR_RX_DIG_RX_DIG_CTRL_RX_ADC_RAW_EN_SHIFT (2U)
#define XCVR_RX_DIG_RX_DIG_CTRL_RX_ADC_RAW_EN(x) (((uint32_t)(((uint32_t)(x)) << XCVR_RX_DIG_RX_DIG_CTRL_RX_ADC_RAW_EN_SHIFT)) & XCVR_RX_DIG_RX_DIG_CTRL_RX_ADC_RAW_EN_MASK)
#define XCVR_RX_DIG_RX_DIG_CTRL_RX_ADC_POL_MASK  (0x8U)
#define XCVR_RX_DIG_RX_DIG_CTRL_RX_ADC_POL_SHIFT (3U)
#define XCVR_RX_DIG_RX_DIG_CTRL_RX_ADC_POL(x)    (((uint32_t)(((uint32_t)(x)) << XCVR_RX_DIG_RX_DIG_CTRL_RX_ADC_POL_SHIFT)) & XCVR_RX_DIG_RX_DIG_CTRL_RX_ADC_POL_MASK)
#define XCVR_RX_DIG_RX_DIG_CTRL_RX_DEC_FILT_OSR_MASK (0x70U)
#define XCVR_RX_DIG_RX_DIG_CTRL_RX_DEC_FILT_OSR_SHIFT (4U)
#define XCVR_RX_DIG_RX_DIG_CTRL_RX_DEC_FILT_OSR(x) (((uint32_t)(((uint32_t)(x)) << XCVR_RX_DIG_RX_DIG_CTRL_RX_DEC_FILT_OSR_SHIFT)) & XCVR_RX_DIG_RX_DIG_CTRL_RX_DEC_FILT_OSR_MASK)
#define XCVR_RX_DIG_RX_DIG_CTRL_RX_FSK_ZB_SEL_MASK (0x100U)
#define XCVR_RX_DIG_RX_DIG_CTRL_RX_FSK_ZB_SEL_SHIFT (8U)
#define XCVR_RX_DIG_RX_DIG_CTRL_RX_FSK_ZB_SEL(x) (((uint32_t)(((uint32_t)(x)) << XCVR_RX_DIG_RX_DIG_CTRL_RX_FSK_ZB_SEL_SHIFT)) & XCVR_RX_DIG_RX_DIG_CTRL_RX_FSK_ZB_SEL_MASK)
#define XCVR_RX_DIG_RX_DIG_CTRL_RX_NORM_EN_MASK  (0x200U)
#define XCVR_RX_DIG_RX_DIG_CTRL_RX_NORM_EN_SHIFT (9U)
#define XCVR_RX_DIG_RX_DIG_CTRL_RX_NORM_EN(x)    (((uint32_t)(((uint32_t)(x)) << XCVR_RX_DIG_RX_DIG_CTRL_RX_NORM_EN_SHIFT)) & XCVR_RX_DIG_RX_DIG_CTRL_RX_NORM_EN_MASK)
#define XCVR_RX_DIG_RX_DIG_CTRL_RX_RSSI_EN_MASK  (0x400U)
#define XCVR_RX_DIG_RX_DIG_CTRL_RX_RSSI_EN_SHIFT (10U)
#define XCVR_RX_DIG_RX_DIG_CTRL_RX_RSSI_EN(x)    (((uint32_t)(((uint32_t)(x)) << XCVR_RX_DIG_RX_DIG_CTRL_RX_RSSI_EN_SHIFT)) & XCVR_RX_DIG_RX_DIG_CTRL_RX_RSSI_EN_MASK)
#define XCVR_RX_DIG_RX_DIG_CTRL_RX_AGC_EN_MASK   (0x800U)
#define XCVR_RX_DIG_RX_DIG_CTRL_RX_AGC_EN_SHIFT  (11U)
#define XCVR_RX_DIG_RX_DIG_CTRL_RX_AGC_EN(x)     (((uint32_t)(((uint32_t)(x)) << XCVR_RX_DIG_RX_DIG_CTRL_RX_AGC_EN_SHIFT)) & XCVR_RX_DIG_RX_DIG_CTRL_RX_AGC_EN_MASK)
#define XCVR_RX_DIG_RX_DIG_CTRL_RX_DCOC_EN_MASK  (0x1000U)
#define XCVR_RX_DIG_RX_DIG_CTRL_RX_DCOC_EN_SHIFT (12U)
#define XCVR_RX_DIG_RX_DIG_CTRL_RX_DCOC_EN(x)    (((uint32_t)(((uint32_t)(x)) << XCVR_RX_DIG_RX_DIG_CTRL_RX_DCOC_EN_SHIFT)) & XCVR_RX_DIG_RX_DIG_CTRL_RX_DCOC_EN_MASK)
#define XCVR_RX_DIG_RX_DIG_CTRL_RX_DCOC_CAL_EN_MASK (0x2000U)
#define XCVR_RX_DIG_RX_DIG_CTRL_RX_DCOC_CAL_EN_SHIFT (13U)
#define XCVR_RX_DIG_RX_DIG_CTRL_RX_DCOC_CAL_EN(x) (((uint32_t)(((uint32_t)(x)) << XCVR_RX_DIG_RX_DIG_CTRL_RX_DCOC_CAL_EN_SHIFT)) & XCVR_RX_DIG_RX_DIG_CTRL_RX_DCOC_CAL_EN_MASK)
#define XCVR_RX_DIG_RX_DIG_CTRL_RX_IQ_SWAP_MASK  (0x4000U)
#define XCVR_RX_DIG_RX_DIG_CTRL_RX_IQ_SWAP_SHIFT (14U)
#define XCVR_RX_DIG_RX_DIG_CTRL_RX_IQ_SWAP(x)    (((uint32_t)(((uint32_t)(x)) << XCVR_RX_DIG_RX_DIG_CTRL_RX_IQ_SWAP_SHIFT)) & XCVR_RX_DIG_RX_DIG_CTRL_RX_IQ_SWAP_MASK)
#define XCVR_RX_DIG_RX_DIG_CTRL_RX_DC_RESID_EN_MASK (0x8000U)
#define XCVR_RX_DIG_RX_DIG_CTRL_RX_DC_RESID_EN_SHIFT (15U)
#define XCVR_RX_DIG_RX_DIG_CTRL_RX_DC_RESID_EN(x) (((uint32_t)(((uint32_t)(x)) << XCVR_RX_DIG_RX_DIG_CTRL_RX_DC_RESID_EN_SHIFT)) & XCVR_RX_DIG_RX_DIG_CTRL_RX_DC_RESID_EN_MASK)
#define XCVR_RX_DIG_RX_DIG_CTRL_RX_SRC_EN_MASK   (0x10000U)
#define XCVR_RX_DIG_RX_DIG_CTRL_RX_SRC_EN_SHIFT  (16U)
#define XCVR_RX_DIG_RX_DIG_CTRL_RX_SRC_EN(x)     (((uint32_t)(((uint32_t)(x)) << XCVR_RX_DIG_RX_DIG_CTRL_RX_SRC_EN_SHIFT)) & XCVR_RX_DIG_RX_DIG_CTRL_RX_SRC_EN_MASK)
#define XCVR_RX_DIG_RX_DIG_CTRL_RX_SRC_RATE_MASK (0x20000U)
#define XCVR_RX_DIG_RX_DIG_CTRL_RX_SRC_RATE_SHIFT (17U)
#define XCVR_RX_DIG_RX_DIG_CTRL_RX_SRC_RATE(x)   (((uint32_t)(((uint32_t)(x)) << XCVR_RX_DIG_RX_DIG_CTRL_RX_SRC_RATE_SHIFT)) & XCVR_RX_DIG_RX_DIG_CTRL_RX_SRC_RATE_MASK)
#define XCVR_RX_DIG_RX_DIG_CTRL_RX_DMA_DTEST_EN_MASK (0x40000U)
#define XCVR_RX_DIG_RX_DIG_CTRL_RX_DMA_DTEST_EN_SHIFT (18U)
#define XCVR_RX_DIG_RX_DIG_CTRL_RX_DMA_DTEST_EN(x) (((uint32_t)(((uint32_t)(x)) << XCVR_RX_DIG_RX_DIG_CTRL_RX_DMA_DTEST_EN_SHIFT)) & XCVR_RX_DIG_RX_DIG_CTRL_RX_DMA_DTEST_EN_MASK)
#define XCVR_RX_DIG_RX_DIG_CTRL_RX_DEC_FILT_GAIN_MASK (0x1F00000U)
#define XCVR_RX_DIG_RX_DIG_CTRL_RX_DEC_FILT_GAIN_SHIFT (20U)
#define XCVR_RX_DIG_RX_DIG_CTRL_RX_DEC_FILT_GAIN(x) (((uint32_t)(((uint32_t)(x)) << XCVR_RX_DIG_RX_DIG_CTRL_RX_DEC_FILT_GAIN_SHIFT)) & XCVR_RX_DIG_RX_DIG_CTRL_RX_DEC_FILT_GAIN_MASK)
#define XCVR_RX_DIG_RX_DIG_CTRL_RX_DEC_FILT_HZD_CORR_DIS_MASK (0x2000000U)
#define XCVR_RX_DIG_RX_DIG_CTRL_RX_DEC_FILT_HZD_CORR_DIS_SHIFT (25U)
#define XCVR_RX_DIG_RX_DIG_CTRL_RX_DEC_FILT_HZD_CORR_DIS(x) (((uint32_t)(((uint32_t)(x)) << XCVR_RX_DIG_RX_DIG_CTRL_RX_DEC_FILT_HZD_CORR_DIS_SHIFT)) & XCVR_RX_DIG_RX_DIG_CTRL_RX_DEC_FILT_HZD_CORR_DIS_MASK)
#define XCVR_RX_DIG_RX_DIG_CTRL_RX_DEC_FILT_HAZARD_MASK (0x10000000U)
#define XCVR_RX_DIG_RX_DIG_CTRL_RX_DEC_FILT_HAZARD_SHIFT (28U)
#define XCVR_RX_DIG_RX_DIG_CTRL_RX_DEC_FILT_HAZARD(x) (((uint32_t)(((uint32_t)(x)) << XCVR_RX_DIG_RX_DIG_CTRL_RX_DEC_FILT_HAZARD_SHIFT)) & XCVR_RX_DIG_RX_DIG_CTRL_RX_DEC_FILT_HAZARD_MASK)
#define XCVR_RX_DIG_RX_DIG_CTRL_RX_RSSI_FILT_HAZARD_MASK (0x20000000U)
#define XCVR_RX_DIG_RX_DIG_CTRL_RX_RSSI_FILT_HAZARD_SHIFT (29U)
#define XCVR_RX_DIG_RX_DIG_CTRL_RX_RSSI_FILT_HAZARD(x) (((uint32_t)(((uint32_t)(x)) << XCVR_RX_DIG_RX_DIG_CTRL_RX_RSSI_FILT_HAZARD_SHIFT)) & XCVR_RX_DIG_RX_DIG_CTRL_RX_RSSI_FILT_HAZARD_MASK)
#define XCVR_RX_DIG_RX_DIG_CTRL_RX_DEC_FILT_SAT_I_MASK (0x40000000U)
#define XCVR_RX_DIG_RX_DIG_CTRL_RX_DEC_FILT_SAT_I_SHIFT (30U)
#define XCVR_RX_DIG_RX_DIG_CTRL_RX_DEC_FILT_SAT_I(x) (((uint32_t)(((uint32_t)(x)) << XCVR_RX_DIG_RX_DIG_CTRL_RX_DEC_FILT_SAT_I_SHIFT)) & XCVR_RX_DIG_RX_DIG_CTRL_RX_DEC_FILT_SAT_I_MASK)
#define XCVR_RX_DIG_RX_DIG_CTRL_RX_DEC_FILT_SAT_Q_MASK (0x80000000U)
#define XCVR_RX_DIG_RX_DIG_CTRL_RX_DEC_FILT_SAT_Q_SHIFT (31U)
#define XCVR_RX_DIG_RX_DIG_CTRL_RX_DEC_FILT_SAT_Q(x) (((uint32_t)(((uint32_t)(x)) << XCVR_RX_DIG_RX_DIG_CTRL_RX_DEC_FILT_SAT_Q_SHIFT)) & XCVR_RX_DIG_RX_DIG_CTRL_RX_DEC_FILT_SAT_Q_MASK)

/*! @name AGC_CTRL_0 - AGC Control 0 */
#define XCVR_RX_DIG_AGC_CTRL_0_SLOW_AGC_EN_MASK  (0x1U)
#define XCVR_RX_DIG_AGC_CTRL_0_SLOW_AGC_EN_SHIFT (0U)
#define XCVR_RX_DIG_AGC_CTRL_0_SLOW_AGC_EN(x)    (((uint32_t)(((uint32_t)(x)) << XCVR_RX_DIG_AGC_CTRL_0_SLOW_AGC_EN_SHIFT)) & XCVR_RX_DIG_AGC_CTRL_0_SLOW_AGC_EN_MASK)
#define XCVR_RX_DIG_AGC_CTRL_0_SLOW_AGC_SRC_MASK (0x6U)
#define XCVR_RX_DIG_AGC_CTRL_0_SLOW_AGC_SRC_SHIFT (1U)
#define XCVR_RX_DIG_AGC_CTRL_0_SLOW_AGC_SRC(x)   (((uint32_t)(((uint32_t)(x)) << XCVR_RX_DIG_AGC_CTRL_0_SLOW_AGC_SRC_SHIFT)) & XCVR_RX_DIG_AGC_CTRL_0_SLOW_AGC_SRC_MASK)
#define XCVR_RX_DIG_AGC_CTRL_0_AGC_FREEZE_EN_MASK (0x8U)
#define XCVR_RX_DIG_AGC_CTRL_0_AGC_FREEZE_EN_SHIFT (3U)
#define XCVR_RX_DIG_AGC_CTRL_0_AGC_FREEZE_EN(x)  (((uint32_t)(((uint32_t)(x)) << XCVR_RX_DIG_AGC_CTRL_0_AGC_FREEZE_EN_SHIFT)) & XCVR_RX_DIG_AGC_CTRL_0_AGC_FREEZE_EN_MASK)
#define XCVR_RX_DIG_AGC_CTRL_0_AGC_FREEZE_PRE_OR_AA_MASK (0x10U)
#define XCVR_RX_DIG_AGC_CTRL_0_AGC_FREEZE_PRE_OR_AA_SHIFT (4U)
#define XCVR_RX_DIG_AGC_CTRL_0_AGC_FREEZE_PRE_OR_AA(x) (((uint32_t)(((uint32_t)(x)) << XCVR_RX_DIG_AGC_CTRL_0_AGC_FREEZE_PRE_OR_AA_SHIFT)) & XCVR_RX_DIG_AGC_CTRL_0_AGC_FREEZE_PRE_OR_AA_MASK)
#define XCVR_RX_DIG_AGC_CTRL_0_AGC_UP_EN_MASK    (0x40U)
#define XCVR_RX_DIG_AGC_CTRL_0_AGC_UP_EN_SHIFT   (6U)
#define XCVR_RX_DIG_AGC_CTRL_0_AGC_UP_EN(x)      (((uint32_t)(((uint32_t)(x)) << XCVR_RX_DIG_AGC_CTRL_0_AGC_UP_EN_SHIFT)) & XCVR_RX_DIG_AGC_CTRL_0_AGC_UP_EN_MASK)
#define XCVR_RX_DIG_AGC_CTRL_0_AGC_UP_SRC_MASK   (0x80U)
#define XCVR_RX_DIG_AGC_CTRL_0_AGC_UP_SRC_SHIFT  (7U)
#define XCVR_RX_DIG_AGC_CTRL_0_AGC_UP_SRC(x)     (((uint32_t)(((uint32_t)(x)) << XCVR_RX_DIG_AGC_CTRL_0_AGC_UP_SRC_SHIFT)) & XCVR_RX_DIG_AGC_CTRL_0_AGC_UP_SRC_MASK)
#define XCVR_RX_DIG_AGC_CTRL_0_AGC_DOWN_BBA_STEP_SZ_MASK (0xF00U)
#define XCVR_RX_DIG_AGC_CTRL_0_AGC_DOWN_BBA_STEP_SZ_SHIFT (8U)
#define XCVR_RX_DIG_AGC_CTRL_0_AGC_DOWN_BBA_STEP_SZ(x) (((uint32_t)(((uint32_t)(x)) << XCVR_RX_DIG_AGC_CTRL_0_AGC_DOWN_BBA_STEP_SZ_SHIFT)) & XCVR_RX_DIG_AGC_CTRL_0_AGC_DOWN_BBA_STEP_SZ_MASK)
#define XCVR_RX_DIG_AGC_CTRL_0_AGC_DOWN_LNA_STEP_SZ_MASK (0xF000U)
#define XCVR_RX_DIG_AGC_CTRL_0_AGC_DOWN_LNA_STEP_SZ_SHIFT (12U)
#define XCVR_RX_DIG_AGC_CTRL_0_AGC_DOWN_LNA_STEP_SZ(x) (((uint32_t)(((uint32_t)(x)) << XCVR_RX_DIG_AGC_CTRL_0_AGC_DOWN_LNA_STEP_SZ_SHIFT)) & XCVR_RX_DIG_AGC_CTRL_0_AGC_DOWN_LNA_STEP_SZ_MASK)
#define XCVR_RX_DIG_AGC_CTRL_0_AGC_UP_RSSI_THRESH_MASK (0xFF0000U)
#define XCVR_RX_DIG_AGC_CTRL_0_AGC_UP_RSSI_THRESH_SHIFT (16U)
#define XCVR_RX_DIG_AGC_CTRL_0_AGC_UP_RSSI_THRESH(x) (((uint32_t)(((uint32_t)(x)) << XCVR_RX_DIG_AGC_CTRL_0_AGC_UP_RSSI_THRESH_SHIFT)) & XCVR_RX_DIG_AGC_CTRL_0_AGC_UP_RSSI_THRESH_MASK)
#define XCVR_RX_DIG_AGC_CTRL_0_AGC_DOWN_RSSI_THRESH_MASK (0xFF000000U)
#define XCVR_RX_DIG_AGC_CTRL_0_AGC_DOWN_RSSI_THRESH_SHIFT (24U)
#define XCVR_RX_DIG_AGC_CTRL_0_AGC_DOWN_RSSI_THRESH(x) (((uint32_t)(((uint32_t)(x)) << XCVR_RX_DIG_AGC_CTRL_0_AGC_DOWN_RSSI_THRESH_SHIFT)) & XCVR_RX_DIG_AGC_CTRL_0_AGC_DOWN_RSSI_THRESH_MASK)

/*! @name AGC_CTRL_1 - AGC Control 1 */
#define XCVR_RX_DIG_AGC_CTRL_1_BBA_ALT_CODE_MASK (0xFU)
#define XCVR_RX_DIG_AGC_CTRL_1_BBA_ALT_CODE_SHIFT (0U)
#define XCVR_RX_DIG_AGC_CTRL_1_BBA_ALT_CODE(x)   (((uint32_t)(((uint32_t)(x)) << XCVR_RX_DIG_AGC_CTRL_1_BBA_ALT_CODE_SHIFT)) & XCVR_RX_DIG_AGC_CTRL_1_BBA_ALT_CODE_MASK)
#define XCVR_RX_DIG_AGC_CTRL_1_LNA_ALT_CODE_MASK (0xFF0U)
#define XCVR_RX_DIG_AGC_CTRL_1_LNA_ALT_CODE_SHIFT (4U)
#define XCVR_RX_DIG_AGC_CTRL_1_LNA_ALT_CODE(x)   (((uint32_t)(((uint32_t)(x)) << XCVR_RX_DIG_AGC_CTRL_1_LNA_ALT_CODE_SHIFT)) & XCVR_RX_DIG_AGC_CTRL_1_LNA_ALT_CODE_MASK)
#define XCVR_RX_DIG_AGC_CTRL_1_LNA_USER_GAIN_MASK (0xF000U)
#define XCVR_RX_DIG_AGC_CTRL_1_LNA_USER_GAIN_SHIFT (12U)
#define XCVR_RX_DIG_AGC_CTRL_1_LNA_USER_GAIN(x)  (((uint32_t)(((uint32_t)(x)) << XCVR_RX_DIG_AGC_CTRL_1_LNA_USER_GAIN_SHIFT)) & XCVR_RX_DIG_AGC_CTRL_1_LNA_USER_GAIN_MASK)
#define XCVR_RX_DIG_AGC_CTRL_1_BBA_USER_GAIN_MASK (0xF0000U)
#define XCVR_RX_DIG_AGC_CTRL_1_BBA_USER_GAIN_SHIFT (16U)
#define XCVR_RX_DIG_AGC_CTRL_1_BBA_USER_GAIN(x)  (((uint32_t)(((uint32_t)(x)) << XCVR_RX_DIG_AGC_CTRL_1_BBA_USER_GAIN_SHIFT)) & XCVR_RX_DIG_AGC_CTRL_1_BBA_USER_GAIN_MASK)
#define XCVR_RX_DIG_AGC_CTRL_1_USER_LNA_GAIN_EN_MASK (0x100000U)
#define XCVR_RX_DIG_AGC_CTRL_1_USER_LNA_GAIN_EN_SHIFT (20U)
#define XCVR_RX_DIG_AGC_CTRL_1_USER_LNA_GAIN_EN(x) (((uint32_t)(((uint32_t)(x)) << XCVR_RX_DIG_AGC_CTRL_1_USER_LNA_GAIN_EN_SHIFT)) & XCVR_RX_DIG_AGC_CTRL_1_USER_LNA_GAIN_EN_MASK)
#define XCVR_RX_DIG_AGC_CTRL_1_USER_BBA_GAIN_EN_MASK (0x200000U)
#define XCVR_RX_DIG_AGC_CTRL_1_USER_BBA_GAIN_EN_SHIFT (21U)
#define XCVR_RX_DIG_AGC_CTRL_1_USER_BBA_GAIN_EN(x) (((uint32_t)(((uint32_t)(x)) << XCVR_RX_DIG_AGC_CTRL_1_USER_BBA_GAIN_EN_SHIFT)) & XCVR_RX_DIG_AGC_CTRL_1_USER_BBA_GAIN_EN_MASK)
#define XCVR_RX_DIG_AGC_CTRL_1_PRESLOW_EN_MASK   (0x400000U)
#define XCVR_RX_DIG_AGC_CTRL_1_PRESLOW_EN_SHIFT  (22U)
#define XCVR_RX_DIG_AGC_CTRL_1_PRESLOW_EN(x)     (((uint32_t)(((uint32_t)(x)) << XCVR_RX_DIG_AGC_CTRL_1_PRESLOW_EN_SHIFT)) & XCVR_RX_DIG_AGC_CTRL_1_PRESLOW_EN_MASK)
#define XCVR_RX_DIG_AGC_CTRL_1_LNA_GAIN_SETTLE_TIME_MASK (0xFF000000U)
#define XCVR_RX_DIG_AGC_CTRL_1_LNA_GAIN_SETTLE_TIME_SHIFT (24U)
#define XCVR_RX_DIG_AGC_CTRL_1_LNA_GAIN_SETTLE_TIME(x) (((uint32_t)(((uint32_t)(x)) << XCVR_RX_DIG_AGC_CTRL_1_LNA_GAIN_SETTLE_TIME_SHIFT)) & XCVR_RX_DIG_AGC_CTRL_1_LNA_GAIN_SETTLE_TIME_MASK)

/*! @name AGC_CTRL_2 - AGC Control 2 */
#define XCVR_RX_DIG_AGC_CTRL_2_BBA_PDET_RST_MASK (0x1U)
#define XCVR_RX_DIG_AGC_CTRL_2_BBA_PDET_RST_SHIFT (0U)
#define XCVR_RX_DIG_AGC_CTRL_2_BBA_PDET_RST(x)   (((uint32_t)(((uint32_t)(x)) << XCVR_RX_DIG_AGC_CTRL_2_BBA_PDET_RST_SHIFT)) & XCVR_RX_DIG_AGC_CTRL_2_BBA_PDET_RST_MASK)
#define XCVR_RX_DIG_AGC_CTRL_2_TZA_PDET_RST_MASK (0x2U)
#define XCVR_RX_DIG_AGC_CTRL_2_TZA_PDET_RST_SHIFT (1U)
#define XCVR_RX_DIG_AGC_CTRL_2_TZA_PDET_RST(x)   (((uint32_t)(((uint32_t)(x)) << XCVR_RX_DIG_AGC_CTRL_2_TZA_PDET_RST_SHIFT)) & XCVR_RX_DIG_AGC_CTRL_2_TZA_PDET_RST_MASK)
#define XCVR_RX_DIG_AGC_CTRL_2_MAN_PDET_RST_MASK (0x4U)
#define XCVR_RX_DIG_AGC_CTRL_2_MAN_PDET_RST_SHIFT (2U)
#define XCVR_RX_DIG_AGC_CTRL_2_MAN_PDET_RST(x)   (((uint32_t)(((uint32_t)(x)) << XCVR_RX_DIG_AGC_CTRL_2_MAN_PDET_RST_SHIFT)) & XCVR_RX_DIG_AGC_CTRL_2_MAN_PDET_RST_MASK)
#define XCVR_RX_DIG_AGC_CTRL_2_BBA_GAIN_SETTLE_TIME_MASK (0xFF0U)
#define XCVR_RX_DIG_AGC_CTRL_2_BBA_GAIN_SETTLE_TIME_SHIFT (4U)
#define XCVR_RX_DIG_AGC_CTRL_2_BBA_GAIN_SETTLE_TIME(x) (((uint32_t)(((uint32_t)(x)) << XCVR_RX_DIG_AGC_CTRL_2_BBA_GAIN_SETTLE_TIME_SHIFT)) & XCVR_RX_DIG_AGC_CTRL_2_BBA_GAIN_SETTLE_TIME_MASK)
#define XCVR_RX_DIG_AGC_CTRL_2_BBA_PDET_SEL_LO_MASK (0x7000U)
#define XCVR_RX_DIG_AGC_CTRL_2_BBA_PDET_SEL_LO_SHIFT (12U)
#define XCVR_RX_DIG_AGC_CTRL_2_BBA_PDET_SEL_LO(x) (((uint32_t)(((uint32_t)(x)) << XCVR_RX_DIG_AGC_CTRL_2_BBA_PDET_SEL_LO_SHIFT)) & XCVR_RX_DIG_AGC_CTRL_2_BBA_PDET_SEL_LO_MASK)
#define XCVR_RX_DIG_AGC_CTRL_2_BBA_PDET_SEL_HI_MASK (0x38000U)
#define XCVR_RX_DIG_AGC_CTRL_2_BBA_PDET_SEL_HI_SHIFT (15U)
#define XCVR_RX_DIG_AGC_CTRL_2_BBA_PDET_SEL_HI(x) (((uint32_t)(((uint32_t)(x)) << XCVR_RX_DIG_AGC_CTRL_2_BBA_PDET_SEL_HI_SHIFT)) & XCVR_RX_DIG_AGC_CTRL_2_BBA_PDET_SEL_HI_MASK)
#define XCVR_RX_DIG_AGC_CTRL_2_TZA_PDET_SEL_LO_MASK (0x1C0000U)
#define XCVR_RX_DIG_AGC_CTRL_2_TZA_PDET_SEL_LO_SHIFT (18U)
#define XCVR_RX_DIG_AGC_CTRL_2_TZA_PDET_SEL_LO(x) (((uint32_t)(((uint32_t)(x)) << XCVR_RX_DIG_AGC_CTRL_2_TZA_PDET_SEL_LO_SHIFT)) & XCVR_RX_DIG_AGC_CTRL_2_TZA_PDET_SEL_LO_MASK)
#define XCVR_RX_DIG_AGC_CTRL_2_TZA_PDET_SEL_HI_MASK (0xE00000U)
#define XCVR_RX_DIG_AGC_CTRL_2_TZA_PDET_SEL_HI_SHIFT (21U)
#define XCVR_RX_DIG_AGC_CTRL_2_TZA_PDET_SEL_HI(x) (((uint32_t)(((uint32_t)(x)) << XCVR_RX_DIG_AGC_CTRL_2_TZA_PDET_SEL_HI_SHIFT)) & XCVR_RX_DIG_AGC_CTRL_2_TZA_PDET_SEL_HI_MASK)
#define XCVR_RX_DIG_AGC_CTRL_2_AGC_FAST_EXPIRE_MASK (0x3F000000U)
#define XCVR_RX_DIG_AGC_CTRL_2_AGC_FAST_EXPIRE_SHIFT (24U)
#define XCVR_RX_DIG_AGC_CTRL_2_AGC_FAST_EXPIRE(x) (((uint32_t)(((uint32_t)(x)) << XCVR_RX_DIG_AGC_CTRL_2_AGC_FAST_EXPIRE_SHIFT)) & XCVR_RX_DIG_AGC_CTRL_2_AGC_FAST_EXPIRE_MASK)
#define XCVR_RX_DIG_AGC_CTRL_2_LNA_LG_ON_OVR_MASK (0x40000000U)
#define XCVR_RX_DIG_AGC_CTRL_2_LNA_LG_ON_OVR_SHIFT (30U)
#define XCVR_RX_DIG_AGC_CTRL_2_LNA_LG_ON_OVR(x)  (((uint32_t)(((uint32_t)(x)) << XCVR_RX_DIG_AGC_CTRL_2_LNA_LG_ON_OVR_SHIFT)) & XCVR_RX_DIG_AGC_CTRL_2_LNA_LG_ON_OVR_MASK)
#define XCVR_RX_DIG_AGC_CTRL_2_LNA_HG_ON_OVR_MASK (0x80000000U)
#define XCVR_RX_DIG_AGC_CTRL_2_LNA_HG_ON_OVR_SHIFT (31U)
#define XCVR_RX_DIG_AGC_CTRL_2_LNA_HG_ON_OVR(x)  (((uint32_t)(((uint32_t)(x)) << XCVR_RX_DIG_AGC_CTRL_2_LNA_HG_ON_OVR_SHIFT)) & XCVR_RX_DIG_AGC_CTRL_2_LNA_HG_ON_OVR_MASK)

/*! @name AGC_CTRL_3 - AGC Control 3 */
#define XCVR_RX_DIG_AGC_CTRL_3_AGC_UNFREEZE_TIME_MASK (0x1FFFU)
#define XCVR_RX_DIG_AGC_CTRL_3_AGC_UNFREEZE_TIME_SHIFT (0U)
#define XCVR_RX_DIG_AGC_CTRL_3_AGC_UNFREEZE_TIME(x) (((uint32_t)(((uint32_t)(x)) << XCVR_RX_DIG_AGC_CTRL_3_AGC_UNFREEZE_TIME_SHIFT)) & XCVR_RX_DIG_AGC_CTRL_3_AGC_UNFREEZE_TIME_MASK)
#define XCVR_RX_DIG_AGC_CTRL_3_AGC_PDET_LO_DLY_MASK (0xE000U)
#define XCVR_RX_DIG_AGC_CTRL_3_AGC_PDET_LO_DLY_SHIFT (13U)
#define XCVR_RX_DIG_AGC_CTRL_3_AGC_PDET_LO_DLY(x) (((uint32_t)(((uint32_t)(x)) << XCVR_RX_DIG_AGC_CTRL_3_AGC_PDET_LO_DLY_SHIFT)) & XCVR_RX_DIG_AGC_CTRL_3_AGC_PDET_LO_DLY_MASK)
#define XCVR_RX_DIG_AGC_CTRL_3_AGC_RSSI_DELT_H2S_MASK (0x7F0000U)
#define XCVR_RX_DIG_AGC_CTRL_3_AGC_RSSI_DELT_H2S_SHIFT (16U)
#define XCVR_RX_DIG_AGC_CTRL_3_AGC_RSSI_DELT_H2S(x) (((uint32_t)(((uint32_t)(x)) << XCVR_RX_DIG_AGC_CTRL_3_AGC_RSSI_DELT_H2S_SHIFT)) & XCVR_RX_DIG_AGC_CTRL_3_AGC_RSSI_DELT_H2S_MASK)
#define XCVR_RX_DIG_AGC_CTRL_3_AGC_H2S_STEP_SZ_MASK (0xF800000U)
#define XCVR_RX_DIG_AGC_CTRL_3_AGC_H2S_STEP_SZ_SHIFT (23U)
#define XCVR_RX_DIG_AGC_CTRL_3_AGC_H2S_STEP_SZ(x) (((uint32_t)(((uint32_t)(x)) << XCVR_RX_DIG_AGC_CTRL_3_AGC_H2S_STEP_SZ_SHIFT)) & XCVR_RX_DIG_AGC_CTRL_3_AGC_H2S_STEP_SZ_MASK)
#define XCVR_RX_DIG_AGC_CTRL_3_AGC_UP_STEP_SZ_MASK (0xF0000000U)
#define XCVR_RX_DIG_AGC_CTRL_3_AGC_UP_STEP_SZ_SHIFT (28U)
#define XCVR_RX_DIG_AGC_CTRL_3_AGC_UP_STEP_SZ(x) (((uint32_t)(((uint32_t)(x)) << XCVR_RX_DIG_AGC_CTRL_3_AGC_UP_STEP_SZ_SHIFT)) & XCVR_RX_DIG_AGC_CTRL_3_AGC_UP_STEP_SZ_MASK)

/*! @name AGC_STAT - AGC Status */
#define XCVR_RX_DIG_AGC_STAT_BBA_PDET_LO_STAT_MASK (0x1U)
#define XCVR_RX_DIG_AGC_STAT_BBA_PDET_LO_STAT_SHIFT (0U)
#define XCVR_RX_DIG_AGC_STAT_BBA_PDET_LO_STAT(x) (((uint32_t)(((uint32_t)(x)) << XCVR_RX_DIG_AGC_STAT_BBA_PDET_LO_STAT_SHIFT)) & XCVR_RX_DIG_AGC_STAT_BBA_PDET_LO_STAT_MASK)
#define XCVR_RX_DIG_AGC_STAT_BBA_PDET_HI_STAT_MASK (0x2U)
#define XCVR_RX_DIG_AGC_STAT_BBA_PDET_HI_STAT_SHIFT (1U)
#define XCVR_RX_DIG_AGC_STAT_BBA_PDET_HI_STAT(x) (((uint32_t)(((uint32_t)(x)) << XCVR_RX_DIG_AGC_STAT_BBA_PDET_HI_STAT_SHIFT)) & XCVR_RX_DIG_AGC_STAT_BBA_PDET_HI_STAT_MASK)
#define XCVR_RX_DIG_AGC_STAT_TZA_PDET_LO_STAT_MASK (0x4U)
#define XCVR_RX_DIG_AGC_STAT_TZA_PDET_LO_STAT_SHIFT (2U)
#define XCVR_RX_DIG_AGC_STAT_TZA_PDET_LO_STAT(x) (((uint32_t)(((uint32_t)(x)) << XCVR_RX_DIG_AGC_STAT_TZA_PDET_LO_STAT_SHIFT)) & XCVR_RX_DIG_AGC_STAT_TZA_PDET_LO_STAT_MASK)
#define XCVR_RX_DIG_AGC_STAT_TZA_PDET_HI_STAT_MASK (0x8U)
#define XCVR_RX_DIG_AGC_STAT_TZA_PDET_HI_STAT_SHIFT (3U)
#define XCVR_RX_DIG_AGC_STAT_TZA_PDET_HI_STAT(x) (((uint32_t)(((uint32_t)(x)) << XCVR_RX_DIG_AGC_STAT_TZA_PDET_HI_STAT_SHIFT)) & XCVR_RX_DIG_AGC_STAT_TZA_PDET_HI_STAT_MASK)
#define XCVR_RX_DIG_AGC_STAT_CURR_AGC_IDX_MASK   (0x1F0U)
#define XCVR_RX_DIG_AGC_STAT_CURR_AGC_IDX_SHIFT  (4U)
#define XCVR_RX_DIG_AGC_STAT_CURR_AGC_IDX(x)     (((uint32_t)(((uint32_t)(x)) << XCVR_RX_DIG_AGC_STAT_CURR_AGC_IDX_SHIFT)) & XCVR_RX_DIG_AGC_STAT_CURR_AGC_IDX_MASK)
#define XCVR_RX_DIG_AGC_STAT_AGC_FROZEN_MASK     (0x200U)
#define XCVR_RX_DIG_AGC_STAT_AGC_FROZEN_SHIFT    (9U)
#define XCVR_RX_DIG_AGC_STAT_AGC_FROZEN(x)       (((uint32_t)(((uint32_t)(x)) << XCVR_RX_DIG_AGC_STAT_AGC_FROZEN_SHIFT)) & XCVR_RX_DIG_AGC_STAT_AGC_FROZEN_MASK)
#define XCVR_RX_DIG_AGC_STAT_RSSI_ADC_RAW_MASK   (0xFF0000U)
#define XCVR_RX_DIG_AGC_STAT_RSSI_ADC_RAW_SHIFT  (16U)
#define XCVR_RX_DIG_AGC_STAT_RSSI_ADC_RAW(x)     (((uint32_t)(((uint32_t)(x)) << XCVR_RX_DIG_AGC_STAT_RSSI_ADC_RAW_SHIFT)) & XCVR_RX_DIG_AGC_STAT_RSSI_ADC_RAW_MASK)

/*! @name RSSI_CTRL_0 - RSSI Control 0 */
#define XCVR_RX_DIG_RSSI_CTRL_0_RSSI_USE_VALS_MASK (0x1U)
#define XCVR_RX_DIG_RSSI_CTRL_0_RSSI_USE_VALS_SHIFT (0U)
#define XCVR_RX_DIG_RSSI_CTRL_0_RSSI_USE_VALS(x) (((uint32_t)(((uint32_t)(x)) << XCVR_RX_DIG_RSSI_CTRL_0_RSSI_USE_VALS_SHIFT)) & XCVR_RX_DIG_RSSI_CTRL_0_RSSI_USE_VALS_MASK)
#define XCVR_RX_DIG_RSSI_CTRL_0_RSSI_HOLD_SRC_MASK (0x6U)
#define XCVR_RX_DIG_RSSI_CTRL_0_RSSI_HOLD_SRC_SHIFT (1U)
#define XCVR_RX_DIG_RSSI_CTRL_0_RSSI_HOLD_SRC(x) (((uint32_t)(((uint32_t)(x)) << XCVR_RX_DIG_RSSI_CTRL_0_RSSI_HOLD_SRC_SHIFT)) & XCVR_RX_DIG_RSSI_CTRL_0_RSSI_HOLD_SRC_MASK)
#define XCVR_RX_DIG_RSSI_CTRL_0_RSSI_HOLD_EN_MASK (0x8U)
#define XCVR_RX_DIG_RSSI_CTRL_0_RSSI_HOLD_EN_SHIFT (3U)
#define XCVR_RX_DIG_RSSI_CTRL_0_RSSI_HOLD_EN(x)  (((uint32_t)(((uint32_t)(x)) << XCVR_RX_DIG_RSSI_CTRL_0_RSSI_HOLD_EN_SHIFT)) & XCVR_RX_DIG_RSSI_CTRL_0_RSSI_HOLD_EN_MASK)
#define XCVR_RX_DIG_RSSI_CTRL_0_RSSI_IIR_CW_WEIGHT_MASK (0x60U)
#define XCVR_RX_DIG_RSSI_CTRL_0_RSSI_IIR_CW_WEIGHT_SHIFT (5U)
#define XCVR_RX_DIG_RSSI_CTRL_0_RSSI_IIR_CW_WEIGHT(x) (((uint32_t)(((uint32_t)(x)) << XCVR_RX_DIG_RSSI_CTRL_0_RSSI_IIR_CW_WEIGHT_SHIFT)) & XCVR_RX_DIG_RSSI_CTRL_0_RSSI_IIR_CW_WEIGHT_MASK)
#define XCVR_RX_DIG_RSSI_CTRL_0_RSSI_N_WINDOW_AVG_MASK (0x300U)
#define XCVR_RX_DIG_RSSI_CTRL_0_RSSI_N_WINDOW_AVG_SHIFT (8U)
#define XCVR_RX_DIG_RSSI_CTRL_0_RSSI_N_WINDOW_AVG(x) (((uint32_t)(((uint32_t)(x)) << XCVR_RX_DIG_RSSI_CTRL_0_RSSI_N_WINDOW_AVG_SHIFT)) & XCVR_RX_DIG_RSSI_CTRL_0_RSSI_N_WINDOW_AVG_MASK)
#define XCVR_RX_DIG_RSSI_CTRL_0_RSSI_HOLD_DELAY_MASK (0xFC00U)
#define XCVR_RX_DIG_RSSI_CTRL_0_RSSI_HOLD_DELAY_SHIFT (10U)
#define XCVR_RX_DIG_RSSI_CTRL_0_RSSI_HOLD_DELAY(x) (((uint32_t)(((uint32_t)(x)) << XCVR_RX_DIG_RSSI_CTRL_0_RSSI_HOLD_DELAY_SHIFT)) & XCVR_RX_DIG_RSSI_CTRL_0_RSSI_HOLD_DELAY_MASK)
#define XCVR_RX_DIG_RSSI_CTRL_0_RSSI_IIR_WEIGHT_MASK (0xF0000U)
#define XCVR_RX_DIG_RSSI_CTRL_0_RSSI_IIR_WEIGHT_SHIFT (16U)
#define XCVR_RX_DIG_RSSI_CTRL_0_RSSI_IIR_WEIGHT(x) (((uint32_t)(((uint32_t)(x)) << XCVR_RX_DIG_RSSI_CTRL_0_RSSI_IIR_WEIGHT_SHIFT)) & XCVR_RX_DIG_RSSI_CTRL_0_RSSI_IIR_WEIGHT_MASK)
#define XCVR_RX_DIG_RSSI_CTRL_0_RSSI_VLD_SETTLE_MASK (0x700000U)
#define XCVR_RX_DIG_RSSI_CTRL_0_RSSI_VLD_SETTLE_SHIFT (20U)
#define XCVR_RX_DIG_RSSI_CTRL_0_RSSI_VLD_SETTLE(x) (((uint32_t)(((uint32_t)(x)) << XCVR_RX_DIG_RSSI_CTRL_0_RSSI_VLD_SETTLE_SHIFT)) & XCVR_RX_DIG_RSSI_CTRL_0_RSSI_VLD_SETTLE_MASK)
#define XCVR_RX_DIG_RSSI_CTRL_0_RSSI_ADJ_MASK    (0xFF000000U)
#define XCVR_RX_DIG_RSSI_CTRL_0_RSSI_ADJ_SHIFT   (24U)
#define XCVR_RX_DIG_RSSI_CTRL_0_RSSI_ADJ(x)      (((uint32_t)(((uint32_t)(x)) << XCVR_RX_DIG_RSSI_CTRL_0_RSSI_ADJ_SHIFT)) & XCVR_RX_DIG_RSSI_CTRL_0_RSSI_ADJ_MASK)

/*! @name RSSI_CTRL_1 - RSSI Control 1 */
#define XCVR_RX_DIG_RSSI_CTRL_1_RSSI_OUT_MASK    (0xFF000000U)
#define XCVR_RX_DIG_RSSI_CTRL_1_RSSI_OUT_SHIFT   (24U)
#define XCVR_RX_DIG_RSSI_CTRL_1_RSSI_OUT(x)      (((uint32_t)(((uint32_t)(x)) << XCVR_RX_DIG_RSSI_CTRL_1_RSSI_OUT_SHIFT)) & XCVR_RX_DIG_RSSI_CTRL_1_RSSI_OUT_MASK)

/*! @name RSSI_DFT - RSSI DFT */
#define XCVR_RX_DIG_RSSI_DFT_DFT_MAG_MASK        (0x1FFFU)
#define XCVR_RX_DIG_RSSI_DFT_DFT_MAG_SHIFT       (0U)
#define XCVR_RX_DIG_RSSI_DFT_DFT_MAG(x)          (((uint32_t)(((uint32_t)(x)) << XCVR_RX_DIG_RSSI_DFT_DFT_MAG_SHIFT)) & XCVR_RX_DIG_RSSI_DFT_DFT_MAG_MASK)
#define XCVR_RX_DIG_RSSI_DFT_DFT_NOISE_MASK      (0x1FFF0000U)
#define XCVR_RX_DIG_RSSI_DFT_DFT_NOISE_SHIFT     (16U)
#define XCVR_RX_DIG_RSSI_DFT_DFT_NOISE(x)        (((uint32_t)(((uint32_t)(x)) << XCVR_RX_DIG_RSSI_DFT_DFT_NOISE_SHIFT)) & XCVR_RX_DIG_RSSI_DFT_DFT_NOISE_MASK)

/*! @name DCOC_CTRL_0 - DCOC Control 0 */
#define XCVR_RX_DIG_DCOC_CTRL_0_DCOC_MIDPWR_TRK_DIS_MASK (0x1U)
#define XCVR_RX_DIG_DCOC_CTRL_0_DCOC_MIDPWR_TRK_DIS_SHIFT (0U)
#define XCVR_RX_DIG_DCOC_CTRL_0_DCOC_MIDPWR_TRK_DIS(x) (((uint32_t)(((uint32_t)(x)) << XCVR_RX_DIG_DCOC_CTRL_0_DCOC_MIDPWR_TRK_DIS_SHIFT)) & XCVR_RX_DIG_DCOC_CTRL_0_DCOC_MIDPWR_TRK_DIS_MASK)
#define XCVR_RX_DIG_DCOC_CTRL_0_DCOC_MAN_MASK    (0x2U)
#define XCVR_RX_DIG_DCOC_CTRL_0_DCOC_MAN_SHIFT   (1U)
#define XCVR_RX_DIG_DCOC_CTRL_0_DCOC_MAN(x)      (((uint32_t)(((uint32_t)(x)) << XCVR_RX_DIG_DCOC_CTRL_0_DCOC_MAN_SHIFT)) & XCVR_RX_DIG_DCOC_CTRL_0_DCOC_MAN_MASK)
#define XCVR_RX_DIG_DCOC_CTRL_0_DCOC_TRK_EST_OVR_MASK (0x4U)
#define XCVR_RX_DIG_DCOC_CTRL_0_DCOC_TRK_EST_OVR_SHIFT (2U)
#define XCVR_RX_DIG_DCOC_CTRL_0_DCOC_TRK_EST_OVR(x) (((uint32_t)(((uint32_t)(x)) << XCVR_RX_DIG_DCOC_CTRL_0_DCOC_TRK_EST_OVR_SHIFT)) & XCVR_RX_DIG_DCOC_CTRL_0_DCOC_TRK_EST_OVR_MASK)
#define XCVR_RX_DIG_DCOC_CTRL_0_DCOC_CORRECT_SRC_MASK (0x8U)
#define XCVR_RX_DIG_DCOC_CTRL_0_DCOC_CORRECT_SRC_SHIFT (3U)
#define XCVR_RX_DIG_DCOC_CTRL_0_DCOC_CORRECT_SRC(x) (((uint32_t)(((uint32_t)(x)) << XCVR_RX_DIG_DCOC_CTRL_0_DCOC_CORRECT_SRC_SHIFT)) & XCVR_RX_DIG_DCOC_CTRL_0_DCOC_CORRECT_SRC_MASK)
#define XCVR_RX_DIG_DCOC_CTRL_0_DCOC_CORRECT_EN_MASK (0x10U)
#define XCVR_RX_DIG_DCOC_CTRL_0_DCOC_CORRECT_EN_SHIFT (4U)
#define XCVR_RX_DIG_DCOC_CTRL_0_DCOC_CORRECT_EN(x) (((uint32_t)(((uint32_t)(x)) << XCVR_RX_DIG_DCOC_CTRL_0_DCOC_CORRECT_EN_SHIFT)) & XCVR_RX_DIG_DCOC_CTRL_0_DCOC_CORRECT_EN_MASK)
#define XCVR_RX_DIG_DCOC_CTRL_0_TRACK_FROM_ZERO_MASK (0x20U)
#define XCVR_RX_DIG_DCOC_CTRL_0_TRACK_FROM_ZERO_SHIFT (5U)
#define XCVR_RX_DIG_DCOC_CTRL_0_TRACK_FROM_ZERO(x) (((uint32_t)(((uint32_t)(x)) << XCVR_RX_DIG_DCOC_CTRL_0_TRACK_FROM_ZERO_SHIFT)) & XCVR_RX_DIG_DCOC_CTRL_0_TRACK_FROM_ZERO_MASK)
#define XCVR_RX_DIG_DCOC_CTRL_0_BBA_CORR_POL_MASK (0x40U)
#define XCVR_RX_DIG_DCOC_CTRL_0_BBA_CORR_POL_SHIFT (6U)
#define XCVR_RX_DIG_DCOC_CTRL_0_BBA_CORR_POL(x)  (((uint32_t)(((uint32_t)(x)) << XCVR_RX_DIG_DCOC_CTRL_0_BBA_CORR_POL_SHIFT)) & XCVR_RX_DIG_DCOC_CTRL_0_BBA_CORR_POL_MASK)
#define XCVR_RX_DIG_DCOC_CTRL_0_TZA_CORR_POL_MASK (0x80U)
#define XCVR_RX_DIG_DCOC_CTRL_0_TZA_CORR_POL_SHIFT (7U)
#define XCVR_RX_DIG_DCOC_CTRL_0_TZA_CORR_POL(x)  (((uint32_t)(((uint32_t)(x)) << XCVR_RX_DIG_DCOC_CTRL_0_TZA_CORR_POL_SHIFT)) & XCVR_RX_DIG_DCOC_CTRL_0_TZA_CORR_POL_MASK)
#define XCVR_RX_DIG_DCOC_CTRL_0_DCOC_CAL_DURATION_MASK (0x1F00U)
#define XCVR_RX_DIG_DCOC_CTRL_0_DCOC_CAL_DURATION_SHIFT (8U)
#define XCVR_RX_DIG_DCOC_CTRL_0_DCOC_CAL_DURATION(x) (((uint32_t)(((uint32_t)(x)) << XCVR_RX_DIG_DCOC_CTRL_0_DCOC_CAL_DURATION_SHIFT)) & XCVR_RX_DIG_DCOC_CTRL_0_DCOC_CAL_DURATION_MASK)
#define XCVR_RX_DIG_DCOC_CTRL_0_DCOC_CORR_DLY_MASK (0x1F0000U)
#define XCVR_RX_DIG_DCOC_CTRL_0_DCOC_CORR_DLY_SHIFT (16U)
#define XCVR_RX_DIG_DCOC_CTRL_0_DCOC_CORR_DLY(x) (((uint32_t)(((uint32_t)(x)) << XCVR_RX_DIG_DCOC_CTRL_0_DCOC_CORR_DLY_SHIFT)) & XCVR_RX_DIG_DCOC_CTRL_0_DCOC_CORR_DLY_MASK)
#define XCVR_RX_DIG_DCOC_CTRL_0_DCOC_CORR_HOLD_TIME_MASK (0x7F000000U)
#define XCVR_RX_DIG_DCOC_CTRL_0_DCOC_CORR_HOLD_TIME_SHIFT (24U)
#define XCVR_RX_DIG_DCOC_CTRL_0_DCOC_CORR_HOLD_TIME(x) (((uint32_t)(((uint32_t)(x)) << XCVR_RX_DIG_DCOC_CTRL_0_DCOC_CORR_HOLD_TIME_SHIFT)) & XCVR_RX_DIG_DCOC_CTRL_0_DCOC_CORR_HOLD_TIME_MASK)

/*! @name DCOC_CTRL_1 - DCOC Control 1 */
#define XCVR_RX_DIG_DCOC_CTRL_1_DCOC_SIGN_SCALE_IDX_MASK (0x3U)
#define XCVR_RX_DIG_DCOC_CTRL_1_DCOC_SIGN_SCALE_IDX_SHIFT (0U)
#define XCVR_RX_DIG_DCOC_CTRL_1_DCOC_SIGN_SCALE_IDX(x) (((uint32_t)(((uint32_t)(x)) << XCVR_RX_DIG_DCOC_CTRL_1_DCOC_SIGN_SCALE_IDX_SHIFT)) & XCVR_RX_DIG_DCOC_CTRL_1_DCOC_SIGN_SCALE_IDX_MASK)
#define XCVR_RX_DIG_DCOC_CTRL_1_DCOC_ALPHAC_SCALE_IDX_MASK (0x1CU)
#define XCVR_RX_DIG_DCOC_CTRL_1_DCOC_ALPHAC_SCALE_IDX_SHIFT (2U)
#define XCVR_RX_DIG_DCOC_CTRL_1_DCOC_ALPHAC_SCALE_IDX(x) (((uint32_t)(((uint32_t)(x)) << XCVR_RX_DIG_DCOC_CTRL_1_DCOC_ALPHAC_SCALE_IDX_SHIFT)) & XCVR_RX_DIG_DCOC_CTRL_1_DCOC_ALPHAC_SCALE_IDX_MASK)
#define XCVR_RX_DIG_DCOC_CTRL_1_DCOC_ALPHA_RADIUS_IDX_MASK (0xE0U)
#define XCVR_RX_DIG_DCOC_CTRL_1_DCOC_ALPHA_RADIUS_IDX_SHIFT (5U)
#define XCVR_RX_DIG_DCOC_CTRL_1_DCOC_ALPHA_RADIUS_IDX(x) (((uint32_t)(((uint32_t)(x)) << XCVR_RX_DIG_DCOC_CTRL_1_DCOC_ALPHA_RADIUS_IDX_SHIFT)) & XCVR_RX_DIG_DCOC_CTRL_1_DCOC_ALPHA_RADIUS_IDX_MASK)
#define XCVR_RX_DIG_DCOC_CTRL_1_DCOC_TRK_EST_GS_CNT_MASK (0x7000U)
#define XCVR_RX_DIG_DCOC_CTRL_1_DCOC_TRK_EST_GS_CNT_SHIFT (12U)
#define XCVR_RX_DIG_DCOC_CTRL_1_DCOC_TRK_EST_GS_CNT(x) (((uint32_t)(((uint32_t)(x)) << XCVR_RX_DIG_DCOC_CTRL_1_DCOC_TRK_EST_GS_CNT_SHIFT)) & XCVR_RX_DIG_DCOC_CTRL_1_DCOC_TRK_EST_GS_CNT_MASK)
#define XCVR_RX_DIG_DCOC_CTRL_1_DCOC_SIGN_SCALE_GS_IDX_MASK (0x30000U)
#define XCVR_RX_DIG_DCOC_CTRL_1_DCOC_SIGN_SCALE_GS_IDX_SHIFT (16U)
#define XCVR_RX_DIG_DCOC_CTRL_1_DCOC_SIGN_SCALE_GS_IDX(x) (((uint32_t)(((uint32_t)(x)) << XCVR_RX_DIG_DCOC_CTRL_1_DCOC_SIGN_SCALE_GS_IDX_SHIFT)) & XCVR_RX_DIG_DCOC_CTRL_1_DCOC_SIGN_SCALE_GS_IDX_MASK)
#define XCVR_RX_DIG_DCOC_CTRL_1_DCOC_ALPHAC_SCALE_GS_IDX_MASK (0x1C0000U)
#define XCVR_RX_DIG_DCOC_CTRL_1_DCOC_ALPHAC_SCALE_GS_IDX_SHIFT (18U)
#define XCVR_RX_DIG_DCOC_CTRL_1_DCOC_ALPHAC_SCALE_GS_IDX(x) (((uint32_t)(((uint32_t)(x)) << XCVR_RX_DIG_DCOC_CTRL_1_DCOC_ALPHAC_SCALE_GS_IDX_SHIFT)) & XCVR_RX_DIG_DCOC_CTRL_1_DCOC_ALPHAC_SCALE_GS_IDX_MASK)
#define XCVR_RX_DIG_DCOC_CTRL_1_DCOC_ALPHA_RADIUS_GS_IDX_MASK (0xE00000U)
#define XCVR_RX_DIG_DCOC_CTRL_1_DCOC_ALPHA_RADIUS_GS_IDX_SHIFT (21U)
#define XCVR_RX_DIG_DCOC_CTRL_1_DCOC_ALPHA_RADIUS_GS_IDX(x) (((uint32_t)(((uint32_t)(x)) << XCVR_RX_DIG_DCOC_CTRL_1_DCOC_ALPHA_RADIUS_GS_IDX_SHIFT)) & XCVR_RX_DIG_DCOC_CTRL_1_DCOC_ALPHA_RADIUS_GS_IDX_MASK)
#define XCVR_RX_DIG_DCOC_CTRL_1_DCOC_TRK_MIN_AGC_IDX_MASK (0x1F000000U)
#define XCVR_RX_DIG_DCOC_CTRL_1_DCOC_TRK_MIN_AGC_IDX_SHIFT (24U)
#define XCVR_RX_DIG_DCOC_CTRL_1_DCOC_TRK_MIN_AGC_IDX(x) (((uint32_t)(((uint32_t)(x)) << XCVR_RX_DIG_DCOC_CTRL_1_DCOC_TRK_MIN_AGC_IDX_SHIFT)) & XCVR_RX_DIG_DCOC_CTRL_1_DCOC_TRK_MIN_AGC_IDX_MASK)

/*! @name DCOC_DAC_INIT - DCOC DAC Initialization */
#define XCVR_RX_DIG_DCOC_DAC_INIT_BBA_DCOC_INIT_I_MASK (0x3FU)
#define XCVR_RX_DIG_DCOC_DAC_INIT_BBA_DCOC_INIT_I_SHIFT (0U)
#define XCVR_RX_DIG_DCOC_DAC_INIT_BBA_DCOC_INIT_I(x) (((uint32_t)(((uint32_t)(x)) << XCVR_RX_DIG_DCOC_DAC_INIT_BBA_DCOC_INIT_I_SHIFT)) & XCVR_RX_DIG_DCOC_DAC_INIT_BBA_DCOC_INIT_I_MASK)
#define XCVR_RX_DIG_DCOC_DAC_INIT_BBA_DCOC_INIT_Q_MASK (0x3F00U)
#define XCVR_RX_DIG_DCOC_DAC_INIT_BBA_DCOC_INIT_Q_SHIFT (8U)
#define XCVR_RX_DIG_DCOC_DAC_INIT_BBA_DCOC_INIT_Q(x) (((uint32_t)(((uint32_t)(x)) << XCVR_RX_DIG_DCOC_DAC_INIT_BBA_DCOC_INIT_Q_SHIFT)) & XCVR_RX_DIG_DCOC_DAC_INIT_BBA_DCOC_INIT_Q_MASK)
#define XCVR_RX_DIG_DCOC_DAC_INIT_TZA_DCOC_INIT_I_MASK (0xFF0000U)
#define XCVR_RX_DIG_DCOC_DAC_INIT_TZA_DCOC_INIT_I_SHIFT (16U)
#define XCVR_RX_DIG_DCOC_DAC_INIT_TZA_DCOC_INIT_I(x) (((uint32_t)(((uint32_t)(x)) << XCVR_RX_DIG_DCOC_DAC_INIT_TZA_DCOC_INIT_I_SHIFT)) & XCVR_RX_DIG_DCOC_DAC_INIT_TZA_DCOC_INIT_I_MASK)
#define XCVR_RX_DIG_DCOC_DAC_INIT_TZA_DCOC_INIT_Q_MASK (0xFF000000U)
#define XCVR_RX_DIG_DCOC_DAC_INIT_TZA_DCOC_INIT_Q_SHIFT (24U)
#define XCVR_RX_DIG_DCOC_DAC_INIT_TZA_DCOC_INIT_Q(x) (((uint32_t)(((uint32_t)(x)) << XCVR_RX_DIG_DCOC_DAC_INIT_TZA_DCOC_INIT_Q_SHIFT)) & XCVR_RX_DIG_DCOC_DAC_INIT_TZA_DCOC_INIT_Q_MASK)

/*! @name DCOC_DIG_MAN - DCOC Digital Correction Manual Override */
#define XCVR_RX_DIG_DCOC_DIG_MAN_DIG_DCOC_INIT_I_MASK (0xFFFU)
#define XCVR_RX_DIG_DCOC_DIG_MAN_DIG_DCOC_INIT_I_SHIFT (0U)
#define XCVR_RX_DIG_DCOC_DIG_MAN_DIG_DCOC_INIT_I(x) (((uint32_t)(((uint32_t)(x)) << XCVR_RX_DIG_DCOC_DIG_MAN_DIG_DCOC_INIT_I_SHIFT)) & XCVR_RX_DIG_DCOC_DIG_MAN_DIG_DCOC_INIT_I_MASK)
#define XCVR_RX_DIG_DCOC_DIG_MAN_DIG_DCOC_INIT_Q_MASK (0xFFF0000U)
#define XCVR_RX_DIG_DCOC_DIG_MAN_DIG_DCOC_INIT_Q_SHIFT (16U)
#define XCVR_RX_DIG_DCOC_DIG_MAN_DIG_DCOC_INIT_Q(x) (((uint32_t)(((uint32_t)(x)) << XCVR_RX_DIG_DCOC_DIG_MAN_DIG_DCOC_INIT_Q_SHIFT)) & XCVR_RX_DIG_DCOC_DIG_MAN_DIG_DCOC_INIT_Q_MASK)

/*! @name DCOC_CAL_GAIN - DCOC Calibration Gain */
#define XCVR_RX_DIG_DCOC_CAL_GAIN_DCOC_BBA_CAL_GAIN1_MASK (0xF00U)
#define XCVR_RX_DIG_DCOC_CAL_GAIN_DCOC_BBA_CAL_GAIN1_SHIFT (8U)
#define XCVR_RX_DIG_DCOC_CAL_GAIN_DCOC_BBA_CAL_GAIN1(x) (((uint32_t)(((uint32_t)(x)) << XCVR_RX_DIG_DCOC_CAL_GAIN_DCOC_BBA_CAL_GAIN1_SHIFT)) & XCVR_RX_DIG_DCOC_CAL_GAIN_DCOC_BBA_CAL_GAIN1_MASK)
#define XCVR_RX_DIG_DCOC_CAL_GAIN_DCOC_LNA_CAL_GAIN1_MASK (0xF000U)
#define XCVR_RX_DIG_DCOC_CAL_GAIN_DCOC_LNA_CAL_GAIN1_SHIFT (12U)
#define XCVR_RX_DIG_DCOC_CAL_GAIN_DCOC_LNA_CAL_GAIN1(x) (((uint32_t)(((uint32_t)(x)) << XCVR_RX_DIG_DCOC_CAL_GAIN_DCOC_LNA_CAL_GAIN1_SHIFT)) & XCVR_RX_DIG_DCOC_CAL_GAIN_DCOC_LNA_CAL_GAIN1_MASK)
#define XCVR_RX_DIG_DCOC_CAL_GAIN_DCOC_BBA_CAL_GAIN2_MASK (0xF0000U)
#define XCVR_RX_DIG_DCOC_CAL_GAIN_DCOC_BBA_CAL_GAIN2_SHIFT (16U)
#define XCVR_RX_DIG_DCOC_CAL_GAIN_DCOC_BBA_CAL_GAIN2(x) (((uint32_t)(((uint32_t)(x)) << XCVR_RX_DIG_DCOC_CAL_GAIN_DCOC_BBA_CAL_GAIN2_SHIFT)) & XCVR_RX_DIG_DCOC_CAL_GAIN_DCOC_BBA_CAL_GAIN2_MASK)
#define XCVR_RX_DIG_DCOC_CAL_GAIN_DCOC_LNA_CAL_GAIN2_MASK (0xF00000U)
#define XCVR_RX_DIG_DCOC_CAL_GAIN_DCOC_LNA_CAL_GAIN2_SHIFT (20U)
#define XCVR_RX_DIG_DCOC_CAL_GAIN_DCOC_LNA_CAL_GAIN2(x) (((uint32_t)(((uint32_t)(x)) << XCVR_RX_DIG_DCOC_CAL_GAIN_DCOC_LNA_CAL_GAIN2_SHIFT)) & XCVR_RX_DIG_DCOC_CAL_GAIN_DCOC_LNA_CAL_GAIN2_MASK)
#define XCVR_RX_DIG_DCOC_CAL_GAIN_DCOC_BBA_CAL_GAIN3_MASK (0xF000000U)
#define XCVR_RX_DIG_DCOC_CAL_GAIN_DCOC_BBA_CAL_GAIN3_SHIFT (24U)
#define XCVR_RX_DIG_DCOC_CAL_GAIN_DCOC_BBA_CAL_GAIN3(x) (((uint32_t)(((uint32_t)(x)) << XCVR_RX_DIG_DCOC_CAL_GAIN_DCOC_BBA_CAL_GAIN3_SHIFT)) & XCVR_RX_DIG_DCOC_CAL_GAIN_DCOC_BBA_CAL_GAIN3_MASK)
#define XCVR_RX_DIG_DCOC_CAL_GAIN_DCOC_LNA_CAL_GAIN3_MASK (0xF0000000U)
#define XCVR_RX_DIG_DCOC_CAL_GAIN_DCOC_LNA_CAL_GAIN3_SHIFT (28U)
#define XCVR_RX_DIG_DCOC_CAL_GAIN_DCOC_LNA_CAL_GAIN3(x) (((uint32_t)(((uint32_t)(x)) << XCVR_RX_DIG_DCOC_CAL_GAIN_DCOC_LNA_CAL_GAIN3_SHIFT)) & XCVR_RX_DIG_DCOC_CAL_GAIN_DCOC_LNA_CAL_GAIN3_MASK)

/*! @name DCOC_STAT - DCOC Status */
#define XCVR_RX_DIG_DCOC_STAT_BBA_DCOC_I_MASK    (0x3FU)
#define XCVR_RX_DIG_DCOC_STAT_BBA_DCOC_I_SHIFT   (0U)
#define XCVR_RX_DIG_DCOC_STAT_BBA_DCOC_I(x)      (((uint32_t)(((uint32_t)(x)) << XCVR_RX_DIG_DCOC_STAT_BBA_DCOC_I_SHIFT)) & XCVR_RX_DIG_DCOC_STAT_BBA_DCOC_I_MASK)
#define XCVR_RX_DIG_DCOC_STAT_BBA_DCOC_Q_MASK    (0x3F00U)
#define XCVR_RX_DIG_DCOC_STAT_BBA_DCOC_Q_SHIFT   (8U)
#define XCVR_RX_DIG_DCOC_STAT_BBA_DCOC_Q(x)      (((uint32_t)(((uint32_t)(x)) << XCVR_RX_DIG_DCOC_STAT_BBA_DCOC_Q_SHIFT)) & XCVR_RX_DIG_DCOC_STAT_BBA_DCOC_Q_MASK)
#define XCVR_RX_DIG_DCOC_STAT_TZA_DCOC_I_MASK    (0xFF0000U)
#define XCVR_RX_DIG_DCOC_STAT_TZA_DCOC_I_SHIFT   (16U)
#define XCVR_RX_DIG_DCOC_STAT_TZA_DCOC_I(x)      (((uint32_t)(((uint32_t)(x)) << XCVR_RX_DIG_DCOC_STAT_TZA_DCOC_I_SHIFT)) & XCVR_RX_DIG_DCOC_STAT_TZA_DCOC_I_MASK)
#define XCVR_RX_DIG_DCOC_STAT_TZA_DCOC_Q_MASK    (0xFF000000U)
#define XCVR_RX_DIG_DCOC_STAT_TZA_DCOC_Q_SHIFT   (24U)
#define XCVR_RX_DIG_DCOC_STAT_TZA_DCOC_Q(x)      (((uint32_t)(((uint32_t)(x)) << XCVR_RX_DIG_DCOC_STAT_TZA_DCOC_Q_SHIFT)) & XCVR_RX_DIG_DCOC_STAT_TZA_DCOC_Q_MASK)

/*! @name DCOC_DC_EST - DCOC DC Estimate */
#define XCVR_RX_DIG_DCOC_DC_EST_DC_EST_I_MASK    (0xFFFU)
#define XCVR_RX_DIG_DCOC_DC_EST_DC_EST_I_SHIFT   (0U)
#define XCVR_RX_DIG_DCOC_DC_EST_DC_EST_I(x)      (((uint32_t)(((uint32_t)(x)) << XCVR_RX_DIG_DCOC_DC_EST_DC_EST_I_SHIFT)) & XCVR_RX_DIG_DCOC_DC_EST_DC_EST_I_MASK)
#define XCVR_RX_DIG_DCOC_DC_EST_DC_EST_Q_MASK    (0xFFF0000U)
#define XCVR_RX_DIG_DCOC_DC_EST_DC_EST_Q_SHIFT   (16U)
#define XCVR_RX_DIG_DCOC_DC_EST_DC_EST_Q(x)      (((uint32_t)(((uint32_t)(x)) << XCVR_RX_DIG_DCOC_DC_EST_DC_EST_Q_SHIFT)) & XCVR_RX_DIG_DCOC_DC_EST_DC_EST_Q_MASK)

/*! @name DCOC_CAL_RCP - DCOC Calibration Reciprocals */
#define XCVR_RX_DIG_DCOC_CAL_RCP_DCOC_TMP_CALC_RECIP_MASK (0x7FFU)
#define XCVR_RX_DIG_DCOC_CAL_RCP_DCOC_TMP_CALC_RECIP_SHIFT (0U)
#define XCVR_RX_DIG_DCOC_CAL_RCP_DCOC_TMP_CALC_RECIP(x) (((uint32_t)(((uint32_t)(x)) << XCVR_RX_DIG_DCOC_CAL_RCP_DCOC_TMP_CALC_RECIP_SHIFT)) & XCVR_RX_DIG_DCOC_CAL_RCP_DCOC_TMP_CALC_RECIP_MASK)
#define XCVR_RX_DIG_DCOC_CAL_RCP_ALPHA_CALC_RECIP_MASK (0x7FF0000U)
#define XCVR_RX_DIG_DCOC_CAL_RCP_ALPHA_CALC_RECIP_SHIFT (16U)
#define XCVR_RX_DIG_DCOC_CAL_RCP_ALPHA_CALC_RECIP(x) (((uint32_t)(((uint32_t)(x)) << XCVR_RX_DIG_DCOC_CAL_RCP_ALPHA_CALC_RECIP_SHIFT)) & XCVR_RX_DIG_DCOC_CAL_RCP_ALPHA_CALC_RECIP_MASK)

/*! @name IQMC_CTRL - IQMC Control */
#define XCVR_RX_DIG_IQMC_CTRL_IQMC_CAL_EN_MASK   (0x1U)
#define XCVR_RX_DIG_IQMC_CTRL_IQMC_CAL_EN_SHIFT  (0U)
#define XCVR_RX_DIG_IQMC_CTRL_IQMC_CAL_EN(x)     (((uint32_t)(((uint32_t)(x)) << XCVR_RX_DIG_IQMC_CTRL_IQMC_CAL_EN_SHIFT)) & XCVR_RX_DIG_IQMC_CTRL_IQMC_CAL_EN_MASK)
#define XCVR_RX_DIG_IQMC_CTRL_IQMC_NUM_ITER_MASK (0xFF00U)
#define XCVR_RX_DIG_IQMC_CTRL_IQMC_NUM_ITER_SHIFT (8U)
#define XCVR_RX_DIG_IQMC_CTRL_IQMC_NUM_ITER(x)   (((uint32_t)(((uint32_t)(x)) << XCVR_RX_DIG_IQMC_CTRL_IQMC_NUM_ITER_SHIFT)) & XCVR_RX_DIG_IQMC_CTRL_IQMC_NUM_ITER_MASK)
#define XCVR_RX_DIG_IQMC_CTRL_IQMC_DC_GAIN_ADJ_MASK (0x7FF0000U)
#define XCVR_RX_DIG_IQMC_CTRL_IQMC_DC_GAIN_ADJ_SHIFT (16U)
#define XCVR_RX_DIG_IQMC_CTRL_IQMC_DC_GAIN_ADJ(x) (((uint32_t)(((uint32_t)(x)) << XCVR_RX_DIG_IQMC_CTRL_IQMC_DC_GAIN_ADJ_SHIFT)) & XCVR_RX_DIG_IQMC_CTRL_IQMC_DC_GAIN_ADJ_MASK)

/*! @name IQMC_CAL - IQMC Calibration */
#define XCVR_RX_DIG_IQMC_CAL_IQMC_GAIN_ADJ_MASK  (0x7FFU)
#define XCVR_RX_DIG_IQMC_CAL_IQMC_GAIN_ADJ_SHIFT (0U)
#define XCVR_RX_DIG_IQMC_CAL_IQMC_GAIN_ADJ(x)    (((uint32_t)(((uint32_t)(x)) << XCVR_RX_DIG_IQMC_CAL_IQMC_GAIN_ADJ_SHIFT)) & XCVR_RX_DIG_IQMC_CAL_IQMC_GAIN_ADJ_MASK)
#define XCVR_RX_DIG_IQMC_CAL_IQMC_PHASE_ADJ_MASK (0xFFF0000U)
#define XCVR_RX_DIG_IQMC_CAL_IQMC_PHASE_ADJ_SHIFT (16U)
#define XCVR_RX_DIG_IQMC_CAL_IQMC_PHASE_ADJ(x)   (((uint32_t)(((uint32_t)(x)) << XCVR_RX_DIG_IQMC_CAL_IQMC_PHASE_ADJ_SHIFT)) & XCVR_RX_DIG_IQMC_CAL_IQMC_PHASE_ADJ_MASK)

/*! @name LNA_GAIN_VAL_3_0 - LNA_GAIN Step Values 3..0 */
#define XCVR_RX_DIG_LNA_GAIN_VAL_3_0_LNA_GAIN_VAL_0_MASK (0xFFU)
#define XCVR_RX_DIG_LNA_GAIN_VAL_3_0_LNA_GAIN_VAL_0_SHIFT (0U)
#define XCVR_RX_DIG_LNA_GAIN_VAL_3_0_LNA_GAIN_VAL_0(x) (((uint32_t)(((uint32_t)(x)) << XCVR_RX_DIG_LNA_GAIN_VAL_3_0_LNA_GAIN_VAL_0_SHIFT)) & XCVR_RX_DIG_LNA_GAIN_VAL_3_0_LNA_GAIN_VAL_0_MASK)
#define XCVR_RX_DIG_LNA_GAIN_VAL_3_0_LNA_GAIN_VAL_1_MASK (0xFF00U)
#define XCVR_RX_DIG_LNA_GAIN_VAL_3_0_LNA_GAIN_VAL_1_SHIFT (8U)
#define XCVR_RX_DIG_LNA_GAIN_VAL_3_0_LNA_GAIN_VAL_1(x) (((uint32_t)(((uint32_t)(x)) << XCVR_RX_DIG_LNA_GAIN_VAL_3_0_LNA_GAIN_VAL_1_SHIFT)) & XCVR_RX_DIG_LNA_GAIN_VAL_3_0_LNA_GAIN_VAL_1_MASK)
#define XCVR_RX_DIG_LNA_GAIN_VAL_3_0_LNA_GAIN_VAL_2_MASK (0xFF0000U)
#define XCVR_RX_DIG_LNA_GAIN_VAL_3_0_LNA_GAIN_VAL_2_SHIFT (16U)
#define XCVR_RX_DIG_LNA_GAIN_VAL_3_0_LNA_GAIN_VAL_2(x) (((uint32_t)(((uint32_t)(x)) << XCVR_RX_DIG_LNA_GAIN_VAL_3_0_LNA_GAIN_VAL_2_SHIFT)) & XCVR_RX_DIG_LNA_GAIN_VAL_3_0_LNA_GAIN_VAL_2_MASK)
#define XCVR_RX_DIG_LNA_GAIN_VAL_3_0_LNA_GAIN_VAL_3_MASK (0xFF000000U)
#define XCVR_RX_DIG_LNA_GAIN_VAL_3_0_LNA_GAIN_VAL_3_SHIFT (24U)
#define XCVR_RX_DIG_LNA_GAIN_VAL_3_0_LNA_GAIN_VAL_3(x) (((uint32_t)(((uint32_t)(x)) << XCVR_RX_DIG_LNA_GAIN_VAL_3_0_LNA_GAIN_VAL_3_SHIFT)) & XCVR_RX_DIG_LNA_GAIN_VAL_3_0_LNA_GAIN_VAL_3_MASK)

/*! @name LNA_GAIN_VAL_7_4 - LNA_GAIN Step Values 7..4 */
#define XCVR_RX_DIG_LNA_GAIN_VAL_7_4_LNA_GAIN_VAL_4_MASK (0xFFU)
#define XCVR_RX_DIG_LNA_GAIN_VAL_7_4_LNA_GAIN_VAL_4_SHIFT (0U)
#define XCVR_RX_DIG_LNA_GAIN_VAL_7_4_LNA_GAIN_VAL_4(x) (((uint32_t)(((uint32_t)(x)) << XCVR_RX_DIG_LNA_GAIN_VAL_7_4_LNA_GAIN_VAL_4_SHIFT)) & XCVR_RX_DIG_LNA_GAIN_VAL_7_4_LNA_GAIN_VAL_4_MASK)
#define XCVR_RX_DIG_LNA_GAIN_VAL_7_4_LNA_GAIN_VAL_5_MASK (0xFF00U)
#define XCVR_RX_DIG_LNA_GAIN_VAL_7_4_LNA_GAIN_VAL_5_SHIFT (8U)
#define XCVR_RX_DIG_LNA_GAIN_VAL_7_4_LNA_GAIN_VAL_5(x) (((uint32_t)(((uint32_t)(x)) << XCVR_RX_DIG_LNA_GAIN_VAL_7_4_LNA_GAIN_VAL_5_SHIFT)) & XCVR_RX_DIG_LNA_GAIN_VAL_7_4_LNA_GAIN_VAL_5_MASK)
#define XCVR_RX_DIG_LNA_GAIN_VAL_7_4_LNA_GAIN_VAL_6_MASK (0xFF0000U)
#define XCVR_RX_DIG_LNA_GAIN_VAL_7_4_LNA_GAIN_VAL_6_SHIFT (16U)
#define XCVR_RX_DIG_LNA_GAIN_VAL_7_4_LNA_GAIN_VAL_6(x) (((uint32_t)(((uint32_t)(x)) << XCVR_RX_DIG_LNA_GAIN_VAL_7_4_LNA_GAIN_VAL_6_SHIFT)) & XCVR_RX_DIG_LNA_GAIN_VAL_7_4_LNA_GAIN_VAL_6_MASK)
#define XCVR_RX_DIG_LNA_GAIN_VAL_7_4_LNA_GAIN_VAL_7_MASK (0xFF000000U)
#define XCVR_RX_DIG_LNA_GAIN_VAL_7_4_LNA_GAIN_VAL_7_SHIFT (24U)
#define XCVR_RX_DIG_LNA_GAIN_VAL_7_4_LNA_GAIN_VAL_7(x) (((uint32_t)(((uint32_t)(x)) << XCVR_RX_DIG_LNA_GAIN_VAL_7_4_LNA_GAIN_VAL_7_SHIFT)) & XCVR_RX_DIG_LNA_GAIN_VAL_7_4_LNA_GAIN_VAL_7_MASK)

/*! @name LNA_GAIN_VAL_8 - LNA_GAIN Step Values 8 */
#define XCVR_RX_DIG_LNA_GAIN_VAL_8_LNA_GAIN_VAL_8_MASK (0xFFU)
#define XCVR_RX_DIG_LNA_GAIN_VAL_8_LNA_GAIN_VAL_8_SHIFT (0U)
#define XCVR_RX_DIG_LNA_GAIN_VAL_8_LNA_GAIN_VAL_8(x) (((uint32_t)(((uint32_t)(x)) << XCVR_RX_DIG_LNA_GAIN_VAL_8_LNA_GAIN_VAL_8_SHIFT)) & XCVR_RX_DIG_LNA_GAIN_VAL_8_LNA_GAIN_VAL_8_MASK)
#define XCVR_RX_DIG_LNA_GAIN_VAL_8_LNA_GAIN_VAL_9_MASK (0xFF00U)
#define XCVR_RX_DIG_LNA_GAIN_VAL_8_LNA_GAIN_VAL_9_SHIFT (8U)
#define XCVR_RX_DIG_LNA_GAIN_VAL_8_LNA_GAIN_VAL_9(x) (((uint32_t)(((uint32_t)(x)) << XCVR_RX_DIG_LNA_GAIN_VAL_8_LNA_GAIN_VAL_9_SHIFT)) & XCVR_RX_DIG_LNA_GAIN_VAL_8_LNA_GAIN_VAL_9_MASK)

/*! @name BBA_RES_TUNE_VAL_7_0 - BBA Resistor Tune Values 7..0 */
#define XCVR_RX_DIG_BBA_RES_TUNE_VAL_7_0_BBA_RES_TUNE_VAL_0_MASK (0xFU)
#define XCVR_RX_DIG_BBA_RES_TUNE_VAL_7_0_BBA_RES_TUNE_VAL_0_SHIFT (0U)
#define XCVR_RX_DIG_BBA_RES_TUNE_VAL_7_0_BBA_RES_TUNE_VAL_0(x) (((uint32_t)(((uint32_t)(x)) << XCVR_RX_DIG_BBA_RES_TUNE_VAL_7_0_BBA_RES_TUNE_VAL_0_SHIFT)) & XCVR_RX_DIG_BBA_RES_TUNE_VAL_7_0_BBA_RES_TUNE_VAL_0_MASK)
#define XCVR_RX_DIG_BBA_RES_TUNE_VAL_7_0_BBA_RES_TUNE_VAL_1_MASK (0xF0U)
#define XCVR_RX_DIG_BBA_RES_TUNE_VAL_7_0_BBA_RES_TUNE_VAL_1_SHIFT (4U)
#define XCVR_RX_DIG_BBA_RES_TUNE_VAL_7_0_BBA_RES_TUNE_VAL_1(x) (((uint32_t)(((uint32_t)(x)) << XCVR_RX_DIG_BBA_RES_TUNE_VAL_7_0_BBA_RES_TUNE_VAL_1_SHIFT)) & XCVR_RX_DIG_BBA_RES_TUNE_VAL_7_0_BBA_RES_TUNE_VAL_1_MASK)
#define XCVR_RX_DIG_BBA_RES_TUNE_VAL_7_0_BBA_RES_TUNE_VAL_2_MASK (0xF00U)
#define XCVR_RX_DIG_BBA_RES_TUNE_VAL_7_0_BBA_RES_TUNE_VAL_2_SHIFT (8U)
#define XCVR_RX_DIG_BBA_RES_TUNE_VAL_7_0_BBA_RES_TUNE_VAL_2(x) (((uint32_t)(((uint32_t)(x)) << XCVR_RX_DIG_BBA_RES_TUNE_VAL_7_0_BBA_RES_TUNE_VAL_2_SHIFT)) & XCVR_RX_DIG_BBA_RES_TUNE_VAL_7_0_BBA_RES_TUNE_VAL_2_MASK)
#define XCVR_RX_DIG_BBA_RES_TUNE_VAL_7_0_BBA_RES_TUNE_VAL_3_MASK (0xF000U)
#define XCVR_RX_DIG_BBA_RES_TUNE_VAL_7_0_BBA_RES_TUNE_VAL_3_SHIFT (12U)
#define XCVR_RX_DIG_BBA_RES_TUNE_VAL_7_0_BBA_RES_TUNE_VAL_3(x) (((uint32_t)(((uint32_t)(x)) << XCVR_RX_DIG_BBA_RES_TUNE_VAL_7_0_BBA_RES_TUNE_VAL_3_SHIFT)) & XCVR_RX_DIG_BBA_RES_TUNE_VAL_7_0_BBA_RES_TUNE_VAL_3_MASK)
#define XCVR_RX_DIG_BBA_RES_TUNE_VAL_7_0_BBA_RES_TUNE_VAL_4_MASK (0xF0000U)
#define XCVR_RX_DIG_BBA_RES_TUNE_VAL_7_0_BBA_RES_TUNE_VAL_4_SHIFT (16U)
#define XCVR_RX_DIG_BBA_RES_TUNE_VAL_7_0_BBA_RES_TUNE_VAL_4(x) (((uint32_t)(((uint32_t)(x)) << XCVR_RX_DIG_BBA_RES_TUNE_VAL_7_0_BBA_RES_TUNE_VAL_4_SHIFT)) & XCVR_RX_DIG_BBA_RES_TUNE_VAL_7_0_BBA_RES_TUNE_VAL_4_MASK)
#define XCVR_RX_DIG_BBA_RES_TUNE_VAL_7_0_BBA_RES_TUNE_VAL_5_MASK (0xF00000U)
#define XCVR_RX_DIG_BBA_RES_TUNE_VAL_7_0_BBA_RES_TUNE_VAL_5_SHIFT (20U)
#define XCVR_RX_DIG_BBA_RES_TUNE_VAL_7_0_BBA_RES_TUNE_VAL_5(x) (((uint32_t)(((uint32_t)(x)) << XCVR_RX_DIG_BBA_RES_TUNE_VAL_7_0_BBA_RES_TUNE_VAL_5_SHIFT)) & XCVR_RX_DIG_BBA_RES_TUNE_VAL_7_0_BBA_RES_TUNE_VAL_5_MASK)
#define XCVR_RX_DIG_BBA_RES_TUNE_VAL_7_0_BBA_RES_TUNE_VAL_6_MASK (0xF000000U)
#define XCVR_RX_DIG_BBA_RES_TUNE_VAL_7_0_BBA_RES_TUNE_VAL_6_SHIFT (24U)
#define XCVR_RX_DIG_BBA_RES_TUNE_VAL_7_0_BBA_RES_TUNE_VAL_6(x) (((uint32_t)(((uint32_t)(x)) << XCVR_RX_DIG_BBA_RES_TUNE_VAL_7_0_BBA_RES_TUNE_VAL_6_SHIFT)) & XCVR_RX_DIG_BBA_RES_TUNE_VAL_7_0_BBA_RES_TUNE_VAL_6_MASK)
#define XCVR_RX_DIG_BBA_RES_TUNE_VAL_7_0_BBA_RES_TUNE_VAL_7_MASK (0xF0000000U)
#define XCVR_RX_DIG_BBA_RES_TUNE_VAL_7_0_BBA_RES_TUNE_VAL_7_SHIFT (28U)
#define XCVR_RX_DIG_BBA_RES_TUNE_VAL_7_0_BBA_RES_TUNE_VAL_7(x) (((uint32_t)(((uint32_t)(x)) << XCVR_RX_DIG_BBA_RES_TUNE_VAL_7_0_BBA_RES_TUNE_VAL_7_SHIFT)) & XCVR_RX_DIG_BBA_RES_TUNE_VAL_7_0_BBA_RES_TUNE_VAL_7_MASK)

/*! @name BBA_RES_TUNE_VAL_10_8 - BBA Resistor Tune Values 10..8 */
#define XCVR_RX_DIG_BBA_RES_TUNE_VAL_10_8_BBA_RES_TUNE_VAL_8_MASK (0xFU)
#define XCVR_RX_DIG_BBA_RES_TUNE_VAL_10_8_BBA_RES_TUNE_VAL_8_SHIFT (0U)
#define XCVR_RX_DIG_BBA_RES_TUNE_VAL_10_8_BBA_RES_TUNE_VAL_8(x) (((uint32_t)(((uint32_t)(x)) << XCVR_RX_DIG_BBA_RES_TUNE_VAL_10_8_BBA_RES_TUNE_VAL_8_SHIFT)) & XCVR_RX_DIG_BBA_RES_TUNE_VAL_10_8_BBA_RES_TUNE_VAL_8_MASK)
#define XCVR_RX_DIG_BBA_RES_TUNE_VAL_10_8_BBA_RES_TUNE_VAL_9_MASK (0xF0U)
#define XCVR_RX_DIG_BBA_RES_TUNE_VAL_10_8_BBA_RES_TUNE_VAL_9_SHIFT (4U)
#define XCVR_RX_DIG_BBA_RES_TUNE_VAL_10_8_BBA_RES_TUNE_VAL_9(x) (((uint32_t)(((uint32_t)(x)) << XCVR_RX_DIG_BBA_RES_TUNE_VAL_10_8_BBA_RES_TUNE_VAL_9_SHIFT)) & XCVR_RX_DIG_BBA_RES_TUNE_VAL_10_8_BBA_RES_TUNE_VAL_9_MASK)
#define XCVR_RX_DIG_BBA_RES_TUNE_VAL_10_8_BBA_RES_TUNE_VAL_10_MASK (0xF00U)
#define XCVR_RX_DIG_BBA_RES_TUNE_VAL_10_8_BBA_RES_TUNE_VAL_10_SHIFT (8U)
#define XCVR_RX_DIG_BBA_RES_TUNE_VAL_10_8_BBA_RES_TUNE_VAL_10(x) (((uint32_t)(((uint32_t)(x)) << XCVR_RX_DIG_BBA_RES_TUNE_VAL_10_8_BBA_RES_TUNE_VAL_10_SHIFT)) & XCVR_RX_DIG_BBA_RES_TUNE_VAL_10_8_BBA_RES_TUNE_VAL_10_MASK)

/*! @name LNA_GAIN_LIN_VAL_2_0 - LNA Linear Gain Values 2..0 */
#define XCVR_RX_DIG_LNA_GAIN_LIN_VAL_2_0_LNA_GAIN_LIN_VAL_0_MASK (0x3FFU)
#define XCVR_RX_DIG_LNA_GAIN_LIN_VAL_2_0_LNA_GAIN_LIN_VAL_0_SHIFT (0U)
#define XCVR_RX_DIG_LNA_GAIN_LIN_VAL_2_0_LNA_GAIN_LIN_VAL_0(x) (((uint32_t)(((uint32_t)(x)) << XCVR_RX_DIG_LNA_GAIN_LIN_VAL_2_0_LNA_GAIN_LIN_VAL_0_SHIFT)) & XCVR_RX_DIG_LNA_GAIN_LIN_VAL_2_0_LNA_GAIN_LIN_VAL_0_MASK)
#define XCVR_RX_DIG_LNA_GAIN_LIN_VAL_2_0_LNA_GAIN_LIN_VAL_1_MASK (0xFFC00U)
#define XCVR_RX_DIG_LNA_GAIN_LIN_VAL_2_0_LNA_GAIN_LIN_VAL_1_SHIFT (10U)
#define XCVR_RX_DIG_LNA_GAIN_LIN_VAL_2_0_LNA_GAIN_LIN_VAL_1(x) (((uint32_t)(((uint32_t)(x)) << XCVR_RX_DIG_LNA_GAIN_LIN_VAL_2_0_LNA_GAIN_LIN_VAL_1_SHIFT)) & XCVR_RX_DIG_LNA_GAIN_LIN_VAL_2_0_LNA_GAIN_LIN_VAL_1_MASK)
#define XCVR_RX_DIG_LNA_GAIN_LIN_VAL_2_0_LNA_GAIN_LIN_VAL_2_MASK (0x3FF00000U)
#define XCVR_RX_DIG_LNA_GAIN_LIN_VAL_2_0_LNA_GAIN_LIN_VAL_2_SHIFT (20U)
#define XCVR_RX_DIG_LNA_GAIN_LIN_VAL_2_0_LNA_GAIN_LIN_VAL_2(x) (((uint32_t)(((uint32_t)(x)) << XCVR_RX_DIG_LNA_GAIN_LIN_VAL_2_0_LNA_GAIN_LIN_VAL_2_SHIFT)) & XCVR_RX_DIG_LNA_GAIN_LIN_VAL_2_0_LNA_GAIN_LIN_VAL_2_MASK)

/*! @name LNA_GAIN_LIN_VAL_5_3 - LNA Linear Gain Values 5..3 */
#define XCVR_RX_DIG_LNA_GAIN_LIN_VAL_5_3_LNA_GAIN_LIN_VAL_3_MASK (0x3FFU)
#define XCVR_RX_DIG_LNA_GAIN_LIN_VAL_5_3_LNA_GAIN_LIN_VAL_3_SHIFT (0U)
#define XCVR_RX_DIG_LNA_GAIN_LIN_VAL_5_3_LNA_GAIN_LIN_VAL_3(x) (((uint32_t)(((uint32_t)(x)) << XCVR_RX_DIG_LNA_GAIN_LIN_VAL_5_3_LNA_GAIN_LIN_VAL_3_SHIFT)) & XCVR_RX_DIG_LNA_GAIN_LIN_VAL_5_3_LNA_GAIN_LIN_VAL_3_MASK)
#define XCVR_RX_DIG_LNA_GAIN_LIN_VAL_5_3_LNA_GAIN_LIN_VAL_4_MASK (0xFFC00U)
#define XCVR_RX_DIG_LNA_GAIN_LIN_VAL_5_3_LNA_GAIN_LIN_VAL_4_SHIFT (10U)
#define XCVR_RX_DIG_LNA_GAIN_LIN_VAL_5_3_LNA_GAIN_LIN_VAL_4(x) (((uint32_t)(((uint32_t)(x)) << XCVR_RX_DIG_LNA_GAIN_LIN_VAL_5_3_LNA_GAIN_LIN_VAL_4_SHIFT)) & XCVR_RX_DIG_LNA_GAIN_LIN_VAL_5_3_LNA_GAIN_LIN_VAL_4_MASK)
#define XCVR_RX_DIG_LNA_GAIN_LIN_VAL_5_3_LNA_GAIN_LIN_VAL_5_MASK (0x3FF00000U)
#define XCVR_RX_DIG_LNA_GAIN_LIN_VAL_5_3_LNA_GAIN_LIN_VAL_5_SHIFT (20U)
#define XCVR_RX_DIG_LNA_GAIN_LIN_VAL_5_3_LNA_GAIN_LIN_VAL_5(x) (((uint32_t)(((uint32_t)(x)) << XCVR_RX_DIG_LNA_GAIN_LIN_VAL_5_3_LNA_GAIN_LIN_VAL_5_SHIFT)) & XCVR_RX_DIG_LNA_GAIN_LIN_VAL_5_3_LNA_GAIN_LIN_VAL_5_MASK)

/*! @name LNA_GAIN_LIN_VAL_8_6 - LNA Linear Gain Values 8..6 */
#define XCVR_RX_DIG_LNA_GAIN_LIN_VAL_8_6_LNA_GAIN_LIN_VAL_6_MASK (0x3FFU)
#define XCVR_RX_DIG_LNA_GAIN_LIN_VAL_8_6_LNA_GAIN_LIN_VAL_6_SHIFT (0U)
#define XCVR_RX_DIG_LNA_GAIN_LIN_VAL_8_6_LNA_GAIN_LIN_VAL_6(x) (((uint32_t)(((uint32_t)(x)) << XCVR_RX_DIG_LNA_GAIN_LIN_VAL_8_6_LNA_GAIN_LIN_VAL_6_SHIFT)) & XCVR_RX_DIG_LNA_GAIN_LIN_VAL_8_6_LNA_GAIN_LIN_VAL_6_MASK)
#define XCVR_RX_DIG_LNA_GAIN_LIN_VAL_8_6_LNA_GAIN_LIN_VAL_7_MASK (0xFFC00U)
#define XCVR_RX_DIG_LNA_GAIN_LIN_VAL_8_6_LNA_GAIN_LIN_VAL_7_SHIFT (10U)
#define XCVR_RX_DIG_LNA_GAIN_LIN_VAL_8_6_LNA_GAIN_LIN_VAL_7(x) (((uint32_t)(((uint32_t)(x)) << XCVR_RX_DIG_LNA_GAIN_LIN_VAL_8_6_LNA_GAIN_LIN_VAL_7_SHIFT)) & XCVR_RX_DIG_LNA_GAIN_LIN_VAL_8_6_LNA_GAIN_LIN_VAL_7_MASK)
#define XCVR_RX_DIG_LNA_GAIN_LIN_VAL_8_6_LNA_GAIN_LIN_VAL_8_MASK (0x3FF00000U)
#define XCVR_RX_DIG_LNA_GAIN_LIN_VAL_8_6_LNA_GAIN_LIN_VAL_8_SHIFT (20U)
#define XCVR_RX_DIG_LNA_GAIN_LIN_VAL_8_6_LNA_GAIN_LIN_VAL_8(x) (((uint32_t)(((uint32_t)(x)) << XCVR_RX_DIG_LNA_GAIN_LIN_VAL_8_6_LNA_GAIN_LIN_VAL_8_SHIFT)) & XCVR_RX_DIG_LNA_GAIN_LIN_VAL_8_6_LNA_GAIN_LIN_VAL_8_MASK)

/*! @name LNA_GAIN_LIN_VAL_9 - LNA Linear Gain Values 9 */
#define XCVR_RX_DIG_LNA_GAIN_LIN_VAL_9_LNA_GAIN_LIN_VAL_9_MASK (0x3FFU)
#define XCVR_RX_DIG_LNA_GAIN_LIN_VAL_9_LNA_GAIN_LIN_VAL_9_SHIFT (0U)
#define XCVR_RX_DIG_LNA_GAIN_LIN_VAL_9_LNA_GAIN_LIN_VAL_9(x) (((uint32_t)(((uint32_t)(x)) << XCVR_RX_DIG_LNA_GAIN_LIN_VAL_9_LNA_GAIN_LIN_VAL_9_SHIFT)) & XCVR_RX_DIG_LNA_GAIN_LIN_VAL_9_LNA_GAIN_LIN_VAL_9_MASK)

/*! @name BBA_RES_TUNE_LIN_VAL_3_0 - BBA Resistor Tune Values 3..0 */
#define XCVR_RX_DIG_BBA_RES_TUNE_LIN_VAL_3_0_BBA_RES_TUNE_LIN_VAL_0_MASK (0xFFU)
#define XCVR_RX_DIG_BBA_RES_TUNE_LIN_VAL_3_0_BBA_RES_TUNE_LIN_VAL_0_SHIFT (0U)
#define XCVR_RX_DIG_BBA_RES_TUNE_LIN_VAL_3_0_BBA_RES_TUNE_LIN_VAL_0(x) (((uint32_t)(((uint32_t)(x)) << XCVR_RX_DIG_BBA_RES_TUNE_LIN_VAL_3_0_BBA_RES_TUNE_LIN_VAL_0_SHIFT)) & XCVR_RX_DIG_BBA_RES_TUNE_LIN_VAL_3_0_BBA_RES_TUNE_LIN_VAL_0_MASK)
#define XCVR_RX_DIG_BBA_RES_TUNE_LIN_VAL_3_0_BBA_RES_TUNE_LIN_VAL_1_MASK (0xFF00U)
#define XCVR_RX_DIG_BBA_RES_TUNE_LIN_VAL_3_0_BBA_RES_TUNE_LIN_VAL_1_SHIFT (8U)
#define XCVR_RX_DIG_BBA_RES_TUNE_LIN_VAL_3_0_BBA_RES_TUNE_LIN_VAL_1(x) (((uint32_t)(((uint32_t)(x)) << XCVR_RX_DIG_BBA_RES_TUNE_LIN_VAL_3_0_BBA_RES_TUNE_LIN_VAL_1_SHIFT)) & XCVR_RX_DIG_BBA_RES_TUNE_LIN_VAL_3_0_BBA_RES_TUNE_LIN_VAL_1_MASK)
#define XCVR_RX_DIG_BBA_RES_TUNE_LIN_VAL_3_0_BBA_RES_TUNE_LIN_VAL_2_MASK (0xFF0000U)
#define XCVR_RX_DIG_BBA_RES_TUNE_LIN_VAL_3_0_BBA_RES_TUNE_LIN_VAL_2_SHIFT (16U)
#define XCVR_RX_DIG_BBA_RES_TUNE_LIN_VAL_3_0_BBA_RES_TUNE_LIN_VAL_2(x) (((uint32_t)(((uint32_t)(x)) << XCVR_RX_DIG_BBA_RES_TUNE_LIN_VAL_3_0_BBA_RES_TUNE_LIN_VAL_2_SHIFT)) & XCVR_RX_DIG_BBA_RES_TUNE_LIN_VAL_3_0_BBA_RES_TUNE_LIN_VAL_2_MASK)
#define XCVR_RX_DIG_BBA_RES_TUNE_LIN_VAL_3_0_BBA_RES_TUNE_LIN_VAL_3_MASK (0xFF000000U)
#define XCVR_RX_DIG_BBA_RES_TUNE_LIN_VAL_3_0_BBA_RES_TUNE_LIN_VAL_3_SHIFT (24U)
#define XCVR_RX_DIG_BBA_RES_TUNE_LIN_VAL_3_0_BBA_RES_TUNE_LIN_VAL_3(x) (((uint32_t)(((uint32_t)(x)) << XCVR_RX_DIG_BBA_RES_TUNE_LIN_VAL_3_0_BBA_RES_TUNE_LIN_VAL_3_SHIFT)) & XCVR_RX_DIG_BBA_RES_TUNE_LIN_VAL_3_0_BBA_RES_TUNE_LIN_VAL_3_MASK)

/*! @name BBA_RES_TUNE_LIN_VAL_7_4 - BBA Resistor Tune Values 7..4 */
#define XCVR_RX_DIG_BBA_RES_TUNE_LIN_VAL_7_4_BBA_RES_TUNE_LIN_VAL_4_MASK (0xFFU)
#define XCVR_RX_DIG_BBA_RES_TUNE_LIN_VAL_7_4_BBA_RES_TUNE_LIN_VAL_4_SHIFT (0U)
#define XCVR_RX_DIG_BBA_RES_TUNE_LIN_VAL_7_4_BBA_RES_TUNE_LIN_VAL_4(x) (((uint32_t)(((uint32_t)(x)) << XCVR_RX_DIG_BBA_RES_TUNE_LIN_VAL_7_4_BBA_RES_TUNE_LIN_VAL_4_SHIFT)) & XCVR_RX_DIG_BBA_RES_TUNE_LIN_VAL_7_4_BBA_RES_TUNE_LIN_VAL_4_MASK)
#define XCVR_RX_DIG_BBA_RES_TUNE_LIN_VAL_7_4_BBA_RES_TUNE_LIN_VAL_5_MASK (0xFF00U)
#define XCVR_RX_DIG_BBA_RES_TUNE_LIN_VAL_7_4_BBA_RES_TUNE_LIN_VAL_5_SHIFT (8U)
#define XCVR_RX_DIG_BBA_RES_TUNE_LIN_VAL_7_4_BBA_RES_TUNE_LIN_VAL_5(x) (((uint32_t)(((uint32_t)(x)) << XCVR_RX_DIG_BBA_RES_TUNE_LIN_VAL_7_4_BBA_RES_TUNE_LIN_VAL_5_SHIFT)) & XCVR_RX_DIG_BBA_RES_TUNE_LIN_VAL_7_4_BBA_RES_TUNE_LIN_VAL_5_MASK)
#define XCVR_RX_DIG_BBA_RES_TUNE_LIN_VAL_7_4_BBA_RES_TUNE_LIN_VAL_6_MASK (0xFF0000U)
#define XCVR_RX_DIG_BBA_RES_TUNE_LIN_VAL_7_4_BBA_RES_TUNE_LIN_VAL_6_SHIFT (16U)
#define XCVR_RX_DIG_BBA_RES_TUNE_LIN_VAL_7_4_BBA_RES_TUNE_LIN_VAL_6(x) (((uint32_t)(((uint32_t)(x)) << XCVR_RX_DIG_BBA_RES_TUNE_LIN_VAL_7_4_BBA_RES_TUNE_LIN_VAL_6_SHIFT)) & XCVR_RX_DIG_BBA_RES_TUNE_LIN_VAL_7_4_BBA_RES_TUNE_LIN_VAL_6_MASK)
#define XCVR_RX_DIG_BBA_RES_TUNE_LIN_VAL_7_4_BBA_RES_TUNE_LIN_VAL_7_MASK (0xFF000000U)
#define XCVR_RX_DIG_BBA_RES_TUNE_LIN_VAL_7_4_BBA_RES_TUNE_LIN_VAL_7_SHIFT (24U)
#define XCVR_RX_DIG_BBA_RES_TUNE_LIN_VAL_7_4_BBA_RES_TUNE_LIN_VAL_7(x) (((uint32_t)(((uint32_t)(x)) << XCVR_RX_DIG_BBA_RES_TUNE_LIN_VAL_7_4_BBA_RES_TUNE_LIN_VAL_7_SHIFT)) & XCVR_RX_DIG_BBA_RES_TUNE_LIN_VAL_7_4_BBA_RES_TUNE_LIN_VAL_7_MASK)

/*! @name BBA_RES_TUNE_LIN_VAL_10_8 - BBA Resistor Tune Values 10..8 */
#define XCVR_RX_DIG_BBA_RES_TUNE_LIN_VAL_10_8_BBA_RES_TUNE_LIN_VAL_8_MASK (0x3FFU)
#define XCVR_RX_DIG_BBA_RES_TUNE_LIN_VAL_10_8_BBA_RES_TUNE_LIN_VAL_8_SHIFT (0U)
#define XCVR_RX_DIG_BBA_RES_TUNE_LIN_VAL_10_8_BBA_RES_TUNE_LIN_VAL_8(x) (((uint32_t)(((uint32_t)(x)) << XCVR_RX_DIG_BBA_RES_TUNE_LIN_VAL_10_8_BBA_RES_TUNE_LIN_VAL_8_SHIFT)) & XCVR_RX_DIG_BBA_RES_TUNE_LIN_VAL_10_8_BBA_RES_TUNE_LIN_VAL_8_MASK)
#define XCVR_RX_DIG_BBA_RES_TUNE_LIN_VAL_10_8_BBA_RES_TUNE_LIN_VAL_9_MASK (0xFFC00U)
#define XCVR_RX_DIG_BBA_RES_TUNE_LIN_VAL_10_8_BBA_RES_TUNE_LIN_VAL_9_SHIFT (10U)
#define XCVR_RX_DIG_BBA_RES_TUNE_LIN_VAL_10_8_BBA_RES_TUNE_LIN_VAL_9(x) (((uint32_t)(((uint32_t)(x)) << XCVR_RX_DIG_BBA_RES_TUNE_LIN_VAL_10_8_BBA_RES_TUNE_LIN_VAL_9_SHIFT)) & XCVR_RX_DIG_BBA_RES_TUNE_LIN_VAL_10_8_BBA_RES_TUNE_LIN_VAL_9_MASK)
#define XCVR_RX_DIG_BBA_RES_TUNE_LIN_VAL_10_8_BBA_RES_TUNE_LIN_VAL_10_MASK (0x3FF00000U)
#define XCVR_RX_DIG_BBA_RES_TUNE_LIN_VAL_10_8_BBA_RES_TUNE_LIN_VAL_10_SHIFT (20U)
#define XCVR_RX_DIG_BBA_RES_TUNE_LIN_VAL_10_8_BBA_RES_TUNE_LIN_VAL_10(x) (((uint32_t)(((uint32_t)(x)) << XCVR_RX_DIG_BBA_RES_TUNE_LIN_VAL_10_8_BBA_RES_TUNE_LIN_VAL_10_SHIFT)) & XCVR_RX_DIG_BBA_RES_TUNE_LIN_VAL_10_8_BBA_RES_TUNE_LIN_VAL_10_MASK)

/*! @name AGC_GAIN_TBL_03_00 - AGC Gain Tables Step 03..00 */
#define XCVR_RX_DIG_AGC_GAIN_TBL_03_00_BBA_GAIN_00_MASK (0xFU)
#define XCVR_RX_DIG_AGC_GAIN_TBL_03_00_BBA_GAIN_00_SHIFT (0U)
#define XCVR_RX_DIG_AGC_GAIN_TBL_03_00_BBA_GAIN_00(x) (((uint32_t)(((uint32_t)(x)) << XCVR_RX_DIG_AGC_GAIN_TBL_03_00_BBA_GAIN_00_SHIFT)) & XCVR_RX_DIG_AGC_GAIN_TBL_03_00_BBA_GAIN_00_MASK)
#define XCVR_RX_DIG_AGC_GAIN_TBL_03_00_LNA_GAIN_00_MASK (0xF0U)
#define XCVR_RX_DIG_AGC_GAIN_TBL_03_00_LNA_GAIN_00_SHIFT (4U)
#define XCVR_RX_DIG_AGC_GAIN_TBL_03_00_LNA_GAIN_00(x) (((uint32_t)(((uint32_t)(x)) << XCVR_RX_DIG_AGC_GAIN_TBL_03_00_LNA_GAIN_00_SHIFT)) & XCVR_RX_DIG_AGC_GAIN_TBL_03_00_LNA_GAIN_00_MASK)
#define XCVR_RX_DIG_AGC_GAIN_TBL_03_00_BBA_GAIN_01_MASK (0xF00U)
#define XCVR_RX_DIG_AGC_GAIN_TBL_03_00_BBA_GAIN_01_SHIFT (8U)
#define XCVR_RX_DIG_AGC_GAIN_TBL_03_00_BBA_GAIN_01(x) (((uint32_t)(((uint32_t)(x)) << XCVR_RX_DIG_AGC_GAIN_TBL_03_00_BBA_GAIN_01_SHIFT)) & XCVR_RX_DIG_AGC_GAIN_TBL_03_00_BBA_GAIN_01_MASK)
#define XCVR_RX_DIG_AGC_GAIN_TBL_03_00_LNA_GAIN_01_MASK (0xF000U)
#define XCVR_RX_DIG_AGC_GAIN_TBL_03_00_LNA_GAIN_01_SHIFT (12U)
#define XCVR_RX_DIG_AGC_GAIN_TBL_03_00_LNA_GAIN_01(x) (((uint32_t)(((uint32_t)(x)) << XCVR_RX_DIG_AGC_GAIN_TBL_03_00_LNA_GAIN_01_SHIFT)) & XCVR_RX_DIG_AGC_GAIN_TBL_03_00_LNA_GAIN_01_MASK)
#define XCVR_RX_DIG_AGC_GAIN_TBL_03_00_BBA_GAIN_02_MASK (0xF0000U)
#define XCVR_RX_DIG_AGC_GAIN_TBL_03_00_BBA_GAIN_02_SHIFT (16U)
#define XCVR_RX_DIG_AGC_GAIN_TBL_03_00_BBA_GAIN_02(x) (((uint32_t)(((uint32_t)(x)) << XCVR_RX_DIG_AGC_GAIN_TBL_03_00_BBA_GAIN_02_SHIFT)) & XCVR_RX_DIG_AGC_GAIN_TBL_03_00_BBA_GAIN_02_MASK)
#define XCVR_RX_DIG_AGC_GAIN_TBL_03_00_LNA_GAIN_02_MASK (0xF00000U)
#define XCVR_RX_DIG_AGC_GAIN_TBL_03_00_LNA_GAIN_02_SHIFT (20U)
#define XCVR_RX_DIG_AGC_GAIN_TBL_03_00_LNA_GAIN_02(x) (((uint32_t)(((uint32_t)(x)) << XCVR_RX_DIG_AGC_GAIN_TBL_03_00_LNA_GAIN_02_SHIFT)) & XCVR_RX_DIG_AGC_GAIN_TBL_03_00_LNA_GAIN_02_MASK)
#define XCVR_RX_DIG_AGC_GAIN_TBL_03_00_BBA_GAIN_03_MASK (0xF000000U)
#define XCVR_RX_DIG_AGC_GAIN_TBL_03_00_BBA_GAIN_03_SHIFT (24U)
#define XCVR_RX_DIG_AGC_GAIN_TBL_03_00_BBA_GAIN_03(x) (((uint32_t)(((uint32_t)(x)) << XCVR_RX_DIG_AGC_GAIN_TBL_03_00_BBA_GAIN_03_SHIFT)) & XCVR_RX_DIG_AGC_GAIN_TBL_03_00_BBA_GAIN_03_MASK)
#define XCVR_RX_DIG_AGC_GAIN_TBL_03_00_LNA_GAIN_03_MASK (0xF0000000U)
#define XCVR_RX_DIG_AGC_GAIN_TBL_03_00_LNA_GAIN_03_SHIFT (28U)
#define XCVR_RX_DIG_AGC_GAIN_TBL_03_00_LNA_GAIN_03(x) (((uint32_t)(((uint32_t)(x)) << XCVR_RX_DIG_AGC_GAIN_TBL_03_00_LNA_GAIN_03_SHIFT)) & XCVR_RX_DIG_AGC_GAIN_TBL_03_00_LNA_GAIN_03_MASK)

/*! @name AGC_GAIN_TBL_07_04 - AGC Gain Tables Step 07..04 */
#define XCVR_RX_DIG_AGC_GAIN_TBL_07_04_BBA_GAIN_04_MASK (0xFU)
#define XCVR_RX_DIG_AGC_GAIN_TBL_07_04_BBA_GAIN_04_SHIFT (0U)
#define XCVR_RX_DIG_AGC_GAIN_TBL_07_04_BBA_GAIN_04(x) (((uint32_t)(((uint32_t)(x)) << XCVR_RX_DIG_AGC_GAIN_TBL_07_04_BBA_GAIN_04_SHIFT)) & XCVR_RX_DIG_AGC_GAIN_TBL_07_04_BBA_GAIN_04_MASK)
#define XCVR_RX_DIG_AGC_GAIN_TBL_07_04_LNA_GAIN_04_MASK (0xF0U)
#define XCVR_RX_DIG_AGC_GAIN_TBL_07_04_LNA_GAIN_04_SHIFT (4U)
#define XCVR_RX_DIG_AGC_GAIN_TBL_07_04_LNA_GAIN_04(x) (((uint32_t)(((uint32_t)(x)) << XCVR_RX_DIG_AGC_GAIN_TBL_07_04_LNA_GAIN_04_SHIFT)) & XCVR_RX_DIG_AGC_GAIN_TBL_07_04_LNA_GAIN_04_MASK)
#define XCVR_RX_DIG_AGC_GAIN_TBL_07_04_BBA_GAIN_05_MASK (0xF00U)
#define XCVR_RX_DIG_AGC_GAIN_TBL_07_04_BBA_GAIN_05_SHIFT (8U)
#define XCVR_RX_DIG_AGC_GAIN_TBL_07_04_BBA_GAIN_05(x) (((uint32_t)(((uint32_t)(x)) << XCVR_RX_DIG_AGC_GAIN_TBL_07_04_BBA_GAIN_05_SHIFT)) & XCVR_RX_DIG_AGC_GAIN_TBL_07_04_BBA_GAIN_05_MASK)
#define XCVR_RX_DIG_AGC_GAIN_TBL_07_04_LNA_GAIN_05_MASK (0xF000U)
#define XCVR_RX_DIG_AGC_GAIN_TBL_07_04_LNA_GAIN_05_SHIFT (12U)
#define XCVR_RX_DIG_AGC_GAIN_TBL_07_04_LNA_GAIN_05(x) (((uint32_t)(((uint32_t)(x)) << XCVR_RX_DIG_AGC_GAIN_TBL_07_04_LNA_GAIN_05_SHIFT)) & XCVR_RX_DIG_AGC_GAIN_TBL_07_04_LNA_GAIN_05_MASK)
#define XCVR_RX_DIG_AGC_GAIN_TBL_07_04_BBA_GAIN_06_MASK (0xF0000U)
#define XCVR_RX_DIG_AGC_GAIN_TBL_07_04_BBA_GAIN_06_SHIFT (16U)
#define XCVR_RX_DIG_AGC_GAIN_TBL_07_04_BBA_GAIN_06(x) (((uint32_t)(((uint32_t)(x)) << XCVR_RX_DIG_AGC_GAIN_TBL_07_04_BBA_GAIN_06_SHIFT)) & XCVR_RX_DIG_AGC_GAIN_TBL_07_04_BBA_GAIN_06_MASK)
#define XCVR_RX_DIG_AGC_GAIN_TBL_07_04_LNA_GAIN_06_MASK (0xF00000U)
#define XCVR_RX_DIG_AGC_GAIN_TBL_07_04_LNA_GAIN_06_SHIFT (20U)
#define XCVR_RX_DIG_AGC_GAIN_TBL_07_04_LNA_GAIN_06(x) (((uint32_t)(((uint32_t)(x)) << XCVR_RX_DIG_AGC_GAIN_TBL_07_04_LNA_GAIN_06_SHIFT)) & XCVR_RX_DIG_AGC_GAIN_TBL_07_04_LNA_GAIN_06_MASK)
#define XCVR_RX_DIG_AGC_GAIN_TBL_07_04_BBA_GAIN_07_MASK (0xF000000U)
#define XCVR_RX_DIG_AGC_GAIN_TBL_07_04_BBA_GAIN_07_SHIFT (24U)
#define XCVR_RX_DIG_AGC_GAIN_TBL_07_04_BBA_GAIN_07(x) (((uint32_t)(((uint32_t)(x)) << XCVR_RX_DIG_AGC_GAIN_TBL_07_04_BBA_GAIN_07_SHIFT)) & XCVR_RX_DIG_AGC_GAIN_TBL_07_04_BBA_GAIN_07_MASK)
#define XCVR_RX_DIG_AGC_GAIN_TBL_07_04_LNA_GAIN_07_MASK (0xF0000000U)
#define XCVR_RX_DIG_AGC_GAIN_TBL_07_04_LNA_GAIN_07_SHIFT (28U)
#define XCVR_RX_DIG_AGC_GAIN_TBL_07_04_LNA_GAIN_07(x) (((uint32_t)(((uint32_t)(x)) << XCVR_RX_DIG_AGC_GAIN_TBL_07_04_LNA_GAIN_07_SHIFT)) & XCVR_RX_DIG_AGC_GAIN_TBL_07_04_LNA_GAIN_07_MASK)

/*! @name AGC_GAIN_TBL_11_08 - AGC Gain Tables Step 11..08 */
#define XCVR_RX_DIG_AGC_GAIN_TBL_11_08_BBA_GAIN_08_MASK (0xFU)
#define XCVR_RX_DIG_AGC_GAIN_TBL_11_08_BBA_GAIN_08_SHIFT (0U)
#define XCVR_RX_DIG_AGC_GAIN_TBL_11_08_BBA_GAIN_08(x) (((uint32_t)(((uint32_t)(x)) << XCVR_RX_DIG_AGC_GAIN_TBL_11_08_BBA_GAIN_08_SHIFT)) & XCVR_RX_DIG_AGC_GAIN_TBL_11_08_BBA_GAIN_08_MASK)
#define XCVR_RX_DIG_AGC_GAIN_TBL_11_08_LNA_GAIN_08_MASK (0xF0U)
#define XCVR_RX_DIG_AGC_GAIN_TBL_11_08_LNA_GAIN_08_SHIFT (4U)
#define XCVR_RX_DIG_AGC_GAIN_TBL_11_08_LNA_GAIN_08(x) (((uint32_t)(((uint32_t)(x)) << XCVR_RX_DIG_AGC_GAIN_TBL_11_08_LNA_GAIN_08_SHIFT)) & XCVR_RX_DIG_AGC_GAIN_TBL_11_08_LNA_GAIN_08_MASK)
#define XCVR_RX_DIG_AGC_GAIN_TBL_11_08_BBA_GAIN_09_MASK (0xF00U)
#define XCVR_RX_DIG_AGC_GAIN_TBL_11_08_BBA_GAIN_09_SHIFT (8U)
#define XCVR_RX_DIG_AGC_GAIN_TBL_11_08_BBA_GAIN_09(x) (((uint32_t)(((uint32_t)(x)) << XCVR_RX_DIG_AGC_GAIN_TBL_11_08_BBA_GAIN_09_SHIFT)) & XCVR_RX_DIG_AGC_GAIN_TBL_11_08_BBA_GAIN_09_MASK)
#define XCVR_RX_DIG_AGC_GAIN_TBL_11_08_LNA_GAIN_09_MASK (0xF000U)
#define XCVR_RX_DIG_AGC_GAIN_TBL_11_08_LNA_GAIN_09_SHIFT (12U)
#define XCVR_RX_DIG_AGC_GAIN_TBL_11_08_LNA_GAIN_09(x) (((uint32_t)(((uint32_t)(x)) << XCVR_RX_DIG_AGC_GAIN_TBL_11_08_LNA_GAIN_09_SHIFT)) & XCVR_RX_DIG_AGC_GAIN_TBL_11_08_LNA_GAIN_09_MASK)
#define XCVR_RX_DIG_AGC_GAIN_TBL_11_08_BBA_GAIN_10_MASK (0xF0000U)
#define XCVR_RX_DIG_AGC_GAIN_TBL_11_08_BBA_GAIN_10_SHIFT (16U)
#define XCVR_RX_DIG_AGC_GAIN_TBL_11_08_BBA_GAIN_10(x) (((uint32_t)(((uint32_t)(x)) << XCVR_RX_DIG_AGC_GAIN_TBL_11_08_BBA_GAIN_10_SHIFT)) & XCVR_RX_DIG_AGC_GAIN_TBL_11_08_BBA_GAIN_10_MASK)
#define XCVR_RX_DIG_AGC_GAIN_TBL_11_08_LNA_GAIN_10_MASK (0xF00000U)
#define XCVR_RX_DIG_AGC_GAIN_TBL_11_08_LNA_GAIN_10_SHIFT (20U)
#define XCVR_RX_DIG_AGC_GAIN_TBL_11_08_LNA_GAIN_10(x) (((uint32_t)(((uint32_t)(x)) << XCVR_RX_DIG_AGC_GAIN_TBL_11_08_LNA_GAIN_10_SHIFT)) & XCVR_RX_DIG_AGC_GAIN_TBL_11_08_LNA_GAIN_10_MASK)
#define XCVR_RX_DIG_AGC_GAIN_TBL_11_08_BBA_GAIN_11_MASK (0xF000000U)
#define XCVR_RX_DIG_AGC_GAIN_TBL_11_08_BBA_GAIN_11_SHIFT (24U)
#define XCVR_RX_DIG_AGC_GAIN_TBL_11_08_BBA_GAIN_11(x) (((uint32_t)(((uint32_t)(x)) << XCVR_RX_DIG_AGC_GAIN_TBL_11_08_BBA_GAIN_11_SHIFT)) & XCVR_RX_DIG_AGC_GAIN_TBL_11_08_BBA_GAIN_11_MASK)
#define XCVR_RX_DIG_AGC_GAIN_TBL_11_08_LNA_GAIN_11_MASK (0xF0000000U)
#define XCVR_RX_DIG_AGC_GAIN_TBL_11_08_LNA_GAIN_11_SHIFT (28U)
#define XCVR_RX_DIG_AGC_GAIN_TBL_11_08_LNA_GAIN_11(x) (((uint32_t)(((uint32_t)(x)) << XCVR_RX_DIG_AGC_GAIN_TBL_11_08_LNA_GAIN_11_SHIFT)) & XCVR_RX_DIG_AGC_GAIN_TBL_11_08_LNA_GAIN_11_MASK)

/*! @name AGC_GAIN_TBL_15_12 - AGC Gain Tables Step 15..12 */
#define XCVR_RX_DIG_AGC_GAIN_TBL_15_12_BBA_GAIN_12_MASK (0xFU)
#define XCVR_RX_DIG_AGC_GAIN_TBL_15_12_BBA_GAIN_12_SHIFT (0U)
#define XCVR_RX_DIG_AGC_GAIN_TBL_15_12_BBA_GAIN_12(x) (((uint32_t)(((uint32_t)(x)) << XCVR_RX_DIG_AGC_GAIN_TBL_15_12_BBA_GAIN_12_SHIFT)) & XCVR_RX_DIG_AGC_GAIN_TBL_15_12_BBA_GAIN_12_MASK)
#define XCVR_RX_DIG_AGC_GAIN_TBL_15_12_LNA_GAIN_12_MASK (0xF0U)
#define XCVR_RX_DIG_AGC_GAIN_TBL_15_12_LNA_GAIN_12_SHIFT (4U)
#define XCVR_RX_DIG_AGC_GAIN_TBL_15_12_LNA_GAIN_12(x) (((uint32_t)(((uint32_t)(x)) << XCVR_RX_DIG_AGC_GAIN_TBL_15_12_LNA_GAIN_12_SHIFT)) & XCVR_RX_DIG_AGC_GAIN_TBL_15_12_LNA_GAIN_12_MASK)
#define XCVR_RX_DIG_AGC_GAIN_TBL_15_12_BBA_GAIN_13_MASK (0xF00U)
#define XCVR_RX_DIG_AGC_GAIN_TBL_15_12_BBA_GAIN_13_SHIFT (8U)
#define XCVR_RX_DIG_AGC_GAIN_TBL_15_12_BBA_GAIN_13(x) (((uint32_t)(((uint32_t)(x)) << XCVR_RX_DIG_AGC_GAIN_TBL_15_12_BBA_GAIN_13_SHIFT)) & XCVR_RX_DIG_AGC_GAIN_TBL_15_12_BBA_GAIN_13_MASK)
#define XCVR_RX_DIG_AGC_GAIN_TBL_15_12_LNA_GAIN_13_MASK (0xF000U)
#define XCVR_RX_DIG_AGC_GAIN_TBL_15_12_LNA_GAIN_13_SHIFT (12U)
#define XCVR_RX_DIG_AGC_GAIN_TBL_15_12_LNA_GAIN_13(x) (((uint32_t)(((uint32_t)(x)) << XCVR_RX_DIG_AGC_GAIN_TBL_15_12_LNA_GAIN_13_SHIFT)) & XCVR_RX_DIG_AGC_GAIN_TBL_15_12_LNA_GAIN_13_MASK)
#define XCVR_RX_DIG_AGC_GAIN_TBL_15_12_BBA_GAIN_14_MASK (0xF0000U)
#define XCVR_RX_DIG_AGC_GAIN_TBL_15_12_BBA_GAIN_14_SHIFT (16U)
#define XCVR_RX_DIG_AGC_GAIN_TBL_15_12_BBA_GAIN_14(x) (((uint32_t)(((uint32_t)(x)) << XCVR_RX_DIG_AGC_GAIN_TBL_15_12_BBA_GAIN_14_SHIFT)) & XCVR_RX_DIG_AGC_GAIN_TBL_15_12_BBA_GAIN_14_MASK)
#define XCVR_RX_DIG_AGC_GAIN_TBL_15_12_LNA_GAIN_14_MASK (0xF00000U)
#define XCVR_RX_DIG_AGC_GAIN_TBL_15_12_LNA_GAIN_14_SHIFT (20U)
#define XCVR_RX_DIG_AGC_GAIN_TBL_15_12_LNA_GAIN_14(x) (((uint32_t)(((uint32_t)(x)) << XCVR_RX_DIG_AGC_GAIN_TBL_15_12_LNA_GAIN_14_SHIFT)) & XCVR_RX_DIG_AGC_GAIN_TBL_15_12_LNA_GAIN_14_MASK)
#define XCVR_RX_DIG_AGC_GAIN_TBL_15_12_BBA_GAIN_15_MASK (0xF000000U)
#define XCVR_RX_DIG_AGC_GAIN_TBL_15_12_BBA_GAIN_15_SHIFT (24U)
#define XCVR_RX_DIG_AGC_GAIN_TBL_15_12_BBA_GAIN_15(x) (((uint32_t)(((uint32_t)(x)) << XCVR_RX_DIG_AGC_GAIN_TBL_15_12_BBA_GAIN_15_SHIFT)) & XCVR_RX_DIG_AGC_GAIN_TBL_15_12_BBA_GAIN_15_MASK)
#define XCVR_RX_DIG_AGC_GAIN_TBL_15_12_LNA_GAIN_15_MASK (0xF0000000U)
#define XCVR_RX_DIG_AGC_GAIN_TBL_15_12_LNA_GAIN_15_SHIFT (28U)
#define XCVR_RX_DIG_AGC_GAIN_TBL_15_12_LNA_GAIN_15(x) (((uint32_t)(((uint32_t)(x)) << XCVR_RX_DIG_AGC_GAIN_TBL_15_12_LNA_GAIN_15_SHIFT)) & XCVR_RX_DIG_AGC_GAIN_TBL_15_12_LNA_GAIN_15_MASK)

/*! @name AGC_GAIN_TBL_19_16 - AGC Gain Tables Step 19..16 */
#define XCVR_RX_DIG_AGC_GAIN_TBL_19_16_BBA_GAIN_16_MASK (0xFU)
#define XCVR_RX_DIG_AGC_GAIN_TBL_19_16_BBA_GAIN_16_SHIFT (0U)
#define XCVR_RX_DIG_AGC_GAIN_TBL_19_16_BBA_GAIN_16(x) (((uint32_t)(((uint32_t)(x)) << XCVR_RX_DIG_AGC_GAIN_TBL_19_16_BBA_GAIN_16_SHIFT)) & XCVR_RX_DIG_AGC_GAIN_TBL_19_16_BBA_GAIN_16_MASK)
#define XCVR_RX_DIG_AGC_GAIN_TBL_19_16_LNA_GAIN_16_MASK (0xF0U)
#define XCVR_RX_DIG_AGC_GAIN_TBL_19_16_LNA_GAIN_16_SHIFT (4U)
#define XCVR_RX_DIG_AGC_GAIN_TBL_19_16_LNA_GAIN_16(x) (((uint32_t)(((uint32_t)(x)) << XCVR_RX_DIG_AGC_GAIN_TBL_19_16_LNA_GAIN_16_SHIFT)) & XCVR_RX_DIG_AGC_GAIN_TBL_19_16_LNA_GAIN_16_MASK)
#define XCVR_RX_DIG_AGC_GAIN_TBL_19_16_BBA_GAIN_17_MASK (0xF00U)
#define XCVR_RX_DIG_AGC_GAIN_TBL_19_16_BBA_GAIN_17_SHIFT (8U)
#define XCVR_RX_DIG_AGC_GAIN_TBL_19_16_BBA_GAIN_17(x) (((uint32_t)(((uint32_t)(x)) << XCVR_RX_DIG_AGC_GAIN_TBL_19_16_BBA_GAIN_17_SHIFT)) & XCVR_RX_DIG_AGC_GAIN_TBL_19_16_BBA_GAIN_17_MASK)
#define XCVR_RX_DIG_AGC_GAIN_TBL_19_16_LNA_GAIN_17_MASK (0xF000U)
#define XCVR_RX_DIG_AGC_GAIN_TBL_19_16_LNA_GAIN_17_SHIFT (12U)
#define XCVR_RX_DIG_AGC_GAIN_TBL_19_16_LNA_GAIN_17(x) (((uint32_t)(((uint32_t)(x)) << XCVR_RX_DIG_AGC_GAIN_TBL_19_16_LNA_GAIN_17_SHIFT)) & XCVR_RX_DIG_AGC_GAIN_TBL_19_16_LNA_GAIN_17_MASK)
#define XCVR_RX_DIG_AGC_GAIN_TBL_19_16_BBA_GAIN_18_MASK (0xF0000U)
#define XCVR_RX_DIG_AGC_GAIN_TBL_19_16_BBA_GAIN_18_SHIFT (16U)
#define XCVR_RX_DIG_AGC_GAIN_TBL_19_16_BBA_GAIN_18(x) (((uint32_t)(((uint32_t)(x)) << XCVR_RX_DIG_AGC_GAIN_TBL_19_16_BBA_GAIN_18_SHIFT)) & XCVR_RX_DIG_AGC_GAIN_TBL_19_16_BBA_GAIN_18_MASK)
#define XCVR_RX_DIG_AGC_GAIN_TBL_19_16_LNA_GAIN_18_MASK (0xF00000U)
#define XCVR_RX_DIG_AGC_GAIN_TBL_19_16_LNA_GAIN_18_SHIFT (20U)
#define XCVR_RX_DIG_AGC_GAIN_TBL_19_16_LNA_GAIN_18(x) (((uint32_t)(((uint32_t)(x)) << XCVR_RX_DIG_AGC_GAIN_TBL_19_16_LNA_GAIN_18_SHIFT)) & XCVR_RX_DIG_AGC_GAIN_TBL_19_16_LNA_GAIN_18_MASK)
#define XCVR_RX_DIG_AGC_GAIN_TBL_19_16_BBA_GAIN_19_MASK (0xF000000U)
#define XCVR_RX_DIG_AGC_GAIN_TBL_19_16_BBA_GAIN_19_SHIFT (24U)
#define XCVR_RX_DIG_AGC_GAIN_TBL_19_16_BBA_GAIN_19(x) (((uint32_t)(((uint32_t)(x)) << XCVR_RX_DIG_AGC_GAIN_TBL_19_16_BBA_GAIN_19_SHIFT)) & XCVR_RX_DIG_AGC_GAIN_TBL_19_16_BBA_GAIN_19_MASK)
#define XCVR_RX_DIG_AGC_GAIN_TBL_19_16_LNA_GAIN_19_MASK (0xF0000000U)
#define XCVR_RX_DIG_AGC_GAIN_TBL_19_16_LNA_GAIN_19_SHIFT (28U)
#define XCVR_RX_DIG_AGC_GAIN_TBL_19_16_LNA_GAIN_19(x) (((uint32_t)(((uint32_t)(x)) << XCVR_RX_DIG_AGC_GAIN_TBL_19_16_LNA_GAIN_19_SHIFT)) & XCVR_RX_DIG_AGC_GAIN_TBL_19_16_LNA_GAIN_19_MASK)

/*! @name AGC_GAIN_TBL_23_20 - AGC Gain Tables Step 23..20 */
#define XCVR_RX_DIG_AGC_GAIN_TBL_23_20_BBA_GAIN_20_MASK (0xFU)
#define XCVR_RX_DIG_AGC_GAIN_TBL_23_20_BBA_GAIN_20_SHIFT (0U)
#define XCVR_RX_DIG_AGC_GAIN_TBL_23_20_BBA_GAIN_20(x) (((uint32_t)(((uint32_t)(x)) << XCVR_RX_DIG_AGC_GAIN_TBL_23_20_BBA_GAIN_20_SHIFT)) & XCVR_RX_DIG_AGC_GAIN_TBL_23_20_BBA_GAIN_20_MASK)
#define XCVR_RX_DIG_AGC_GAIN_TBL_23_20_LNA_GAIN_20_MASK (0xF0U)
#define XCVR_RX_DIG_AGC_GAIN_TBL_23_20_LNA_GAIN_20_SHIFT (4U)
#define XCVR_RX_DIG_AGC_GAIN_TBL_23_20_LNA_GAIN_20(x) (((uint32_t)(((uint32_t)(x)) << XCVR_RX_DIG_AGC_GAIN_TBL_23_20_LNA_GAIN_20_SHIFT)) & XCVR_RX_DIG_AGC_GAIN_TBL_23_20_LNA_GAIN_20_MASK)
#define XCVR_RX_DIG_AGC_GAIN_TBL_23_20_BBA_GAIN_21_MASK (0xF00U)
#define XCVR_RX_DIG_AGC_GAIN_TBL_23_20_BBA_GAIN_21_SHIFT (8U)
#define XCVR_RX_DIG_AGC_GAIN_TBL_23_20_BBA_GAIN_21(x) (((uint32_t)(((uint32_t)(x)) << XCVR_RX_DIG_AGC_GAIN_TBL_23_20_BBA_GAIN_21_SHIFT)) & XCVR_RX_DIG_AGC_GAIN_TBL_23_20_BBA_GAIN_21_MASK)
#define XCVR_RX_DIG_AGC_GAIN_TBL_23_20_LNA_GAIN_21_MASK (0xF000U)
#define XCVR_RX_DIG_AGC_GAIN_TBL_23_20_LNA_GAIN_21_SHIFT (12U)
#define XCVR_RX_DIG_AGC_GAIN_TBL_23_20_LNA_GAIN_21(x) (((uint32_t)(((uint32_t)(x)) << XCVR_RX_DIG_AGC_GAIN_TBL_23_20_LNA_GAIN_21_SHIFT)) & XCVR_RX_DIG_AGC_GAIN_TBL_23_20_LNA_GAIN_21_MASK)
#define XCVR_RX_DIG_AGC_GAIN_TBL_23_20_BBA_GAIN_22_MASK (0xF0000U)
#define XCVR_RX_DIG_AGC_GAIN_TBL_23_20_BBA_GAIN_22_SHIFT (16U)
#define XCVR_RX_DIG_AGC_GAIN_TBL_23_20_BBA_GAIN_22(x) (((uint32_t)(((uint32_t)(x)) << XCVR_RX_DIG_AGC_GAIN_TBL_23_20_BBA_GAIN_22_SHIFT)) & XCVR_RX_DIG_AGC_GAIN_TBL_23_20_BBA_GAIN_22_MASK)
#define XCVR_RX_DIG_AGC_GAIN_TBL_23_20_LNA_GAIN_22_MASK (0xF00000U)
#define XCVR_RX_DIG_AGC_GAIN_TBL_23_20_LNA_GAIN_22_SHIFT (20U)
#define XCVR_RX_DIG_AGC_GAIN_TBL_23_20_LNA_GAIN_22(x) (((uint32_t)(((uint32_t)(x)) << XCVR_RX_DIG_AGC_GAIN_TBL_23_20_LNA_GAIN_22_SHIFT)) & XCVR_RX_DIG_AGC_GAIN_TBL_23_20_LNA_GAIN_22_MASK)
#define XCVR_RX_DIG_AGC_GAIN_TBL_23_20_BBA_GAIN_23_MASK (0xF000000U)
#define XCVR_RX_DIG_AGC_GAIN_TBL_23_20_BBA_GAIN_23_SHIFT (24U)
#define XCVR_RX_DIG_AGC_GAIN_TBL_23_20_BBA_GAIN_23(x) (((uint32_t)(((uint32_t)(x)) << XCVR_RX_DIG_AGC_GAIN_TBL_23_20_BBA_GAIN_23_SHIFT)) & XCVR_RX_DIG_AGC_GAIN_TBL_23_20_BBA_GAIN_23_MASK)
#define XCVR_RX_DIG_AGC_GAIN_TBL_23_20_LNA_GAIN_23_MASK (0xF0000000U)
#define XCVR_RX_DIG_AGC_GAIN_TBL_23_20_LNA_GAIN_23_SHIFT (28U)
#define XCVR_RX_DIG_AGC_GAIN_TBL_23_20_LNA_GAIN_23(x) (((uint32_t)(((uint32_t)(x)) << XCVR_RX_DIG_AGC_GAIN_TBL_23_20_LNA_GAIN_23_SHIFT)) & XCVR_RX_DIG_AGC_GAIN_TBL_23_20_LNA_GAIN_23_MASK)

/*! @name AGC_GAIN_TBL_26_24 - AGC Gain Tables Step 26..24 */
#define XCVR_RX_DIG_AGC_GAIN_TBL_26_24_BBA_GAIN_24_MASK (0xFU)
#define XCVR_RX_DIG_AGC_GAIN_TBL_26_24_BBA_GAIN_24_SHIFT (0U)
#define XCVR_RX_DIG_AGC_GAIN_TBL_26_24_BBA_GAIN_24(x) (((uint32_t)(((uint32_t)(x)) << XCVR_RX_DIG_AGC_GAIN_TBL_26_24_BBA_GAIN_24_SHIFT)) & XCVR_RX_DIG_AGC_GAIN_TBL_26_24_BBA_GAIN_24_MASK)
#define XCVR_RX_DIG_AGC_GAIN_TBL_26_24_LNA_GAIN_24_MASK (0xF0U)
#define XCVR_RX_DIG_AGC_GAIN_TBL_26_24_LNA_GAIN_24_SHIFT (4U)
#define XCVR_RX_DIG_AGC_GAIN_TBL_26_24_LNA_GAIN_24(x) (((uint32_t)(((uint32_t)(x)) << XCVR_RX_DIG_AGC_GAIN_TBL_26_24_LNA_GAIN_24_SHIFT)) & XCVR_RX_DIG_AGC_GAIN_TBL_26_24_LNA_GAIN_24_MASK)
#define XCVR_RX_DIG_AGC_GAIN_TBL_26_24_BBA_GAIN_25_MASK (0xF00U)
#define XCVR_RX_DIG_AGC_GAIN_TBL_26_24_BBA_GAIN_25_SHIFT (8U)
#define XCVR_RX_DIG_AGC_GAIN_TBL_26_24_BBA_GAIN_25(x) (((uint32_t)(((uint32_t)(x)) << XCVR_RX_DIG_AGC_GAIN_TBL_26_24_BBA_GAIN_25_SHIFT)) & XCVR_RX_DIG_AGC_GAIN_TBL_26_24_BBA_GAIN_25_MASK)
#define XCVR_RX_DIG_AGC_GAIN_TBL_26_24_LNA_GAIN_25_MASK (0xF000U)
#define XCVR_RX_DIG_AGC_GAIN_TBL_26_24_LNA_GAIN_25_SHIFT (12U)
#define XCVR_RX_DIG_AGC_GAIN_TBL_26_24_LNA_GAIN_25(x) (((uint32_t)(((uint32_t)(x)) << XCVR_RX_DIG_AGC_GAIN_TBL_26_24_LNA_GAIN_25_SHIFT)) & XCVR_RX_DIG_AGC_GAIN_TBL_26_24_LNA_GAIN_25_MASK)
#define XCVR_RX_DIG_AGC_GAIN_TBL_26_24_BBA_GAIN_26_MASK (0xF0000U)
#define XCVR_RX_DIG_AGC_GAIN_TBL_26_24_BBA_GAIN_26_SHIFT (16U)
#define XCVR_RX_DIG_AGC_GAIN_TBL_26_24_BBA_GAIN_26(x) (((uint32_t)(((uint32_t)(x)) << XCVR_RX_DIG_AGC_GAIN_TBL_26_24_BBA_GAIN_26_SHIFT)) & XCVR_RX_DIG_AGC_GAIN_TBL_26_24_BBA_GAIN_26_MASK)
#define XCVR_RX_DIG_AGC_GAIN_TBL_26_24_LNA_GAIN_26_MASK (0xF00000U)
#define XCVR_RX_DIG_AGC_GAIN_TBL_26_24_LNA_GAIN_26_SHIFT (20U)
#define XCVR_RX_DIG_AGC_GAIN_TBL_26_24_LNA_GAIN_26(x) (((uint32_t)(((uint32_t)(x)) << XCVR_RX_DIG_AGC_GAIN_TBL_26_24_LNA_GAIN_26_SHIFT)) & XCVR_RX_DIG_AGC_GAIN_TBL_26_24_LNA_GAIN_26_MASK)

/*! @name DCOC_OFFSET - DCOC Offset */
#define XCVR_RX_DIG_DCOC_OFFSET_DCOC_BBA_OFFSET_I_MASK (0x3FU)
#define XCVR_RX_DIG_DCOC_OFFSET_DCOC_BBA_OFFSET_I_SHIFT (0U)
#define XCVR_RX_DIG_DCOC_OFFSET_DCOC_BBA_OFFSET_I(x) (((uint32_t)(((uint32_t)(x)) << XCVR_RX_DIG_DCOC_OFFSET_DCOC_BBA_OFFSET_I_SHIFT)) & XCVR_RX_DIG_DCOC_OFFSET_DCOC_BBA_OFFSET_I_MASK)
#define XCVR_RX_DIG_DCOC_OFFSET_DCOC_BBA_OFFSET_Q_MASK (0x3F00U)
#define XCVR_RX_DIG_DCOC_OFFSET_DCOC_BBA_OFFSET_Q_SHIFT (8U)
#define XCVR_RX_DIG_DCOC_OFFSET_DCOC_BBA_OFFSET_Q(x) (((uint32_t)(((uint32_t)(x)) << XCVR_RX_DIG_DCOC_OFFSET_DCOC_BBA_OFFSET_Q_SHIFT)) & XCVR_RX_DIG_DCOC_OFFSET_DCOC_BBA_OFFSET_Q_MASK)
#define XCVR_RX_DIG_DCOC_OFFSET_DCOC_TZA_OFFSET_I_MASK (0xFF0000U)
#define XCVR_RX_DIG_DCOC_OFFSET_DCOC_TZA_OFFSET_I_SHIFT (16U)
#define XCVR_RX_DIG_DCOC_OFFSET_DCOC_TZA_OFFSET_I(x) (((uint32_t)(((uint32_t)(x)) << XCVR_RX_DIG_DCOC_OFFSET_DCOC_TZA_OFFSET_I_SHIFT)) & XCVR_RX_DIG_DCOC_OFFSET_DCOC_TZA_OFFSET_I_MASK)
#define XCVR_RX_DIG_DCOC_OFFSET_DCOC_TZA_OFFSET_Q_MASK (0xFF000000U)
#define XCVR_RX_DIG_DCOC_OFFSET_DCOC_TZA_OFFSET_Q_SHIFT (24U)
#define XCVR_RX_DIG_DCOC_OFFSET_DCOC_TZA_OFFSET_Q(x) (((uint32_t)(((uint32_t)(x)) << XCVR_RX_DIG_DCOC_OFFSET_DCOC_TZA_OFFSET_Q_SHIFT)) & XCVR_RX_DIG_DCOC_OFFSET_DCOC_TZA_OFFSET_Q_MASK)

/* The count of XCVR_RX_DIG_DCOC_OFFSET */
#define XCVR_RX_DIG_DCOC_OFFSET_COUNT            (27U)

/*! @name DCOC_BBA_STEP - DCOC BBA DAC Step */
#define XCVR_RX_DIG_DCOC_BBA_STEP_BBA_DCOC_STEP_RECIP_MASK (0x1FFFU)
#define XCVR_RX_DIG_DCOC_BBA_STEP_BBA_DCOC_STEP_RECIP_SHIFT (0U)
#define XCVR_RX_DIG_DCOC_BBA_STEP_BBA_DCOC_STEP_RECIP(x) (((uint32_t)(((uint32_t)(x)) << XCVR_RX_DIG_DCOC_BBA_STEP_BBA_DCOC_STEP_RECIP_SHIFT)) & XCVR_RX_DIG_DCOC_BBA_STEP_BBA_DCOC_STEP_RECIP_MASK)
#define XCVR_RX_DIG_DCOC_BBA_STEP_BBA_DCOC_STEP_MASK (0x1FF0000U)
#define XCVR_RX_DIG_DCOC_BBA_STEP_BBA_DCOC_STEP_SHIFT (16U)
#define XCVR_RX_DIG_DCOC_BBA_STEP_BBA_DCOC_STEP(x) (((uint32_t)(((uint32_t)(x)) << XCVR_RX_DIG_DCOC_BBA_STEP_BBA_DCOC_STEP_SHIFT)) & XCVR_RX_DIG_DCOC_BBA_STEP_BBA_DCOC_STEP_MASK)

/*! @name DCOC_TZA_STEP_0 - DCOC TZA DAC Step 0 */
#define XCVR_RX_DIG_DCOC_TZA_STEP_0_DCOC_TZA_STEP_RCP_0_MASK (0x1FFFU)
#define XCVR_RX_DIG_DCOC_TZA_STEP_0_DCOC_TZA_STEP_RCP_0_SHIFT (0U)
#define XCVR_RX_DIG_DCOC_TZA_STEP_0_DCOC_TZA_STEP_RCP_0(x) (((uint32_t)(((uint32_t)(x)) << XCVR_RX_DIG_DCOC_TZA_STEP_0_DCOC_TZA_STEP_RCP_0_SHIFT)) & XCVR_RX_DIG_DCOC_TZA_STEP_0_DCOC_TZA_STEP_RCP_0_MASK)
#define XCVR_RX_DIG_DCOC_TZA_STEP_0_DCOC_TZA_STEP_GAIN_0_MASK (0xFFF0000U)
#define XCVR_RX_DIG_DCOC_TZA_STEP_0_DCOC_TZA_STEP_GAIN_0_SHIFT (16U)
#define XCVR_RX_DIG_DCOC_TZA_STEP_0_DCOC_TZA_STEP_GAIN_0(x) (((uint32_t)(((uint32_t)(x)) << XCVR_RX_DIG_DCOC_TZA_STEP_0_DCOC_TZA_STEP_GAIN_0_SHIFT)) & XCVR_RX_DIG_DCOC_TZA_STEP_0_DCOC_TZA_STEP_GAIN_0_MASK)

/*! @name DCOC_TZA_STEP_1 - DCOC TZA DAC Step 1 */
#define XCVR_RX_DIG_DCOC_TZA_STEP_1_DCOC_TZA_STEP_RCP_1_MASK (0x1FFFU)
#define XCVR_RX_DIG_DCOC_TZA_STEP_1_DCOC_TZA_STEP_RCP_1_SHIFT (0U)
#define XCVR_RX_DIG_DCOC_TZA_STEP_1_DCOC_TZA_STEP_RCP_1(x) (((uint32_t)(((uint32_t)(x)) << XCVR_RX_DIG_DCOC_TZA_STEP_1_DCOC_TZA_STEP_RCP_1_SHIFT)) & XCVR_RX_DIG_DCOC_TZA_STEP_1_DCOC_TZA_STEP_RCP_1_MASK)
#define XCVR_RX_DIG_DCOC_TZA_STEP_1_DCOC_TZA_STEP_GAIN_1_MASK (0xFFF0000U)
#define XCVR_RX_DIG_DCOC_TZA_STEP_1_DCOC_TZA_STEP_GAIN_1_SHIFT (16U)
#define XCVR_RX_DIG_DCOC_TZA_STEP_1_DCOC_TZA_STEP_GAIN_1(x) (((uint32_t)(((uint32_t)(x)) << XCVR_RX_DIG_DCOC_TZA_STEP_1_DCOC_TZA_STEP_GAIN_1_SHIFT)) & XCVR_RX_DIG_DCOC_TZA_STEP_1_DCOC_TZA_STEP_GAIN_1_MASK)

/*! @name DCOC_TZA_STEP_2 - DCOC TZA DAC Step 2 */
#define XCVR_RX_DIG_DCOC_TZA_STEP_2_DCOC_TZA_STEP_RCP_2_MASK (0x1FFFU)
#define XCVR_RX_DIG_DCOC_TZA_STEP_2_DCOC_TZA_STEP_RCP_2_SHIFT (0U)
#define XCVR_RX_DIG_DCOC_TZA_STEP_2_DCOC_TZA_STEP_RCP_2(x) (((uint32_t)(((uint32_t)(x)) << XCVR_RX_DIG_DCOC_TZA_STEP_2_DCOC_TZA_STEP_RCP_2_SHIFT)) & XCVR_RX_DIG_DCOC_TZA_STEP_2_DCOC_TZA_STEP_RCP_2_MASK)
#define XCVR_RX_DIG_DCOC_TZA_STEP_2_DCOC_TZA_STEP_GAIN_2_MASK (0xFFF0000U)
#define XCVR_RX_DIG_DCOC_TZA_STEP_2_DCOC_TZA_STEP_GAIN_2_SHIFT (16U)
#define XCVR_RX_DIG_DCOC_TZA_STEP_2_DCOC_TZA_STEP_GAIN_2(x) (((uint32_t)(((uint32_t)(x)) << XCVR_RX_DIG_DCOC_TZA_STEP_2_DCOC_TZA_STEP_GAIN_2_SHIFT)) & XCVR_RX_DIG_DCOC_TZA_STEP_2_DCOC_TZA_STEP_GAIN_2_MASK)

/*! @name DCOC_TZA_STEP_3 - DCOC TZA DAC Step 3 */
#define XCVR_RX_DIG_DCOC_TZA_STEP_3_DCOC_TZA_STEP_RCP_3_MASK (0x1FFFU)
#define XCVR_RX_DIG_DCOC_TZA_STEP_3_DCOC_TZA_STEP_RCP_3_SHIFT (0U)
#define XCVR_RX_DIG_DCOC_TZA_STEP_3_DCOC_TZA_STEP_RCP_3(x) (((uint32_t)(((uint32_t)(x)) << XCVR_RX_DIG_DCOC_TZA_STEP_3_DCOC_TZA_STEP_RCP_3_SHIFT)) & XCVR_RX_DIG_DCOC_TZA_STEP_3_DCOC_TZA_STEP_RCP_3_MASK)
#define XCVR_RX_DIG_DCOC_TZA_STEP_3_DCOC_TZA_STEP_GAIN_3_MASK (0xFFF0000U)
#define XCVR_RX_DIG_DCOC_TZA_STEP_3_DCOC_TZA_STEP_GAIN_3_SHIFT (16U)
#define XCVR_RX_DIG_DCOC_TZA_STEP_3_DCOC_TZA_STEP_GAIN_3(x) (((uint32_t)(((uint32_t)(x)) << XCVR_RX_DIG_DCOC_TZA_STEP_3_DCOC_TZA_STEP_GAIN_3_SHIFT)) & XCVR_RX_DIG_DCOC_TZA_STEP_3_DCOC_TZA_STEP_GAIN_3_MASK)

/*! @name DCOC_TZA_STEP_4 - DCOC TZA DAC Step 4 */
#define XCVR_RX_DIG_DCOC_TZA_STEP_4_DCOC_TZA_STEP_RCP_4_MASK (0x1FFFU)
#define XCVR_RX_DIG_DCOC_TZA_STEP_4_DCOC_TZA_STEP_RCP_4_SHIFT (0U)
#define XCVR_RX_DIG_DCOC_TZA_STEP_4_DCOC_TZA_STEP_RCP_4(x) (((uint32_t)(((uint32_t)(x)) << XCVR_RX_DIG_DCOC_TZA_STEP_4_DCOC_TZA_STEP_RCP_4_SHIFT)) & XCVR_RX_DIG_DCOC_TZA_STEP_4_DCOC_TZA_STEP_RCP_4_MASK)
#define XCVR_RX_DIG_DCOC_TZA_STEP_4_DCOC_TZA_STEP_GAIN_4_MASK (0xFFF0000U)
#define XCVR_RX_DIG_DCOC_TZA_STEP_4_DCOC_TZA_STEP_GAIN_4_SHIFT (16U)
#define XCVR_RX_DIG_DCOC_TZA_STEP_4_DCOC_TZA_STEP_GAIN_4(x) (((uint32_t)(((uint32_t)(x)) << XCVR_RX_DIG_DCOC_TZA_STEP_4_DCOC_TZA_STEP_GAIN_4_SHIFT)) & XCVR_RX_DIG_DCOC_TZA_STEP_4_DCOC_TZA_STEP_GAIN_4_MASK)

/*! @name DCOC_TZA_STEP_5 - DCOC TZA DAC Step 5 */
#define XCVR_RX_DIG_DCOC_TZA_STEP_5_DCOC_TZA_STEP_RCP_5_MASK (0x1FFFU)
#define XCVR_RX_DIG_DCOC_TZA_STEP_5_DCOC_TZA_STEP_RCP_5_SHIFT (0U)
#define XCVR_RX_DIG_DCOC_TZA_STEP_5_DCOC_TZA_STEP_RCP_5(x) (((uint32_t)(((uint32_t)(x)) << XCVR_RX_DIG_DCOC_TZA_STEP_5_DCOC_TZA_STEP_RCP_5_SHIFT)) & XCVR_RX_DIG_DCOC_TZA_STEP_5_DCOC_TZA_STEP_RCP_5_MASK)
#define XCVR_RX_DIG_DCOC_TZA_STEP_5_DCOC_TZA_STEP_GAIN_5_MASK (0xFFF0000U)
#define XCVR_RX_DIG_DCOC_TZA_STEP_5_DCOC_TZA_STEP_GAIN_5_SHIFT (16U)
#define XCVR_RX_DIG_DCOC_TZA_STEP_5_DCOC_TZA_STEP_GAIN_5(x) (((uint32_t)(((uint32_t)(x)) << XCVR_RX_DIG_DCOC_TZA_STEP_5_DCOC_TZA_STEP_GAIN_5_SHIFT)) & XCVR_RX_DIG_DCOC_TZA_STEP_5_DCOC_TZA_STEP_GAIN_5_MASK)

/*! @name DCOC_TZA_STEP_6 - DCOC TZA DAC Step 6 */
#define XCVR_RX_DIG_DCOC_TZA_STEP_6_DCOC_TZA_STEP_RCP_6_MASK (0x1FFFU)
#define XCVR_RX_DIG_DCOC_TZA_STEP_6_DCOC_TZA_STEP_RCP_6_SHIFT (0U)
#define XCVR_RX_DIG_DCOC_TZA_STEP_6_DCOC_TZA_STEP_RCP_6(x) (((uint32_t)(((uint32_t)(x)) << XCVR_RX_DIG_DCOC_TZA_STEP_6_DCOC_TZA_STEP_RCP_6_SHIFT)) & XCVR_RX_DIG_DCOC_TZA_STEP_6_DCOC_TZA_STEP_RCP_6_MASK)
#define XCVR_RX_DIG_DCOC_TZA_STEP_6_DCOC_TZA_STEP_GAIN_6_MASK (0xFFF0000U)
#define XCVR_RX_DIG_DCOC_TZA_STEP_6_DCOC_TZA_STEP_GAIN_6_SHIFT (16U)
#define XCVR_RX_DIG_DCOC_TZA_STEP_6_DCOC_TZA_STEP_GAIN_6(x) (((uint32_t)(((uint32_t)(x)) << XCVR_RX_DIG_DCOC_TZA_STEP_6_DCOC_TZA_STEP_GAIN_6_SHIFT)) & XCVR_RX_DIG_DCOC_TZA_STEP_6_DCOC_TZA_STEP_GAIN_6_MASK)

/*! @name DCOC_TZA_STEP_7 - DCOC TZA DAC Step 7 */
#define XCVR_RX_DIG_DCOC_TZA_STEP_7_DCOC_TZA_STEP_RCP_7_MASK (0x1FFFU)
#define XCVR_RX_DIG_DCOC_TZA_STEP_7_DCOC_TZA_STEP_RCP_7_SHIFT (0U)
#define XCVR_RX_DIG_DCOC_TZA_STEP_7_DCOC_TZA_STEP_RCP_7(x) (((uint32_t)(((uint32_t)(x)) << XCVR_RX_DIG_DCOC_TZA_STEP_7_DCOC_TZA_STEP_RCP_7_SHIFT)) & XCVR_RX_DIG_DCOC_TZA_STEP_7_DCOC_TZA_STEP_RCP_7_MASK)
#define XCVR_RX_DIG_DCOC_TZA_STEP_7_DCOC_TZA_STEP_GAIN_7_MASK (0x1FFF0000U)
#define XCVR_RX_DIG_DCOC_TZA_STEP_7_DCOC_TZA_STEP_GAIN_7_SHIFT (16U)
#define XCVR_RX_DIG_DCOC_TZA_STEP_7_DCOC_TZA_STEP_GAIN_7(x) (((uint32_t)(((uint32_t)(x)) << XCVR_RX_DIG_DCOC_TZA_STEP_7_DCOC_TZA_STEP_GAIN_7_SHIFT)) & XCVR_RX_DIG_DCOC_TZA_STEP_7_DCOC_TZA_STEP_GAIN_7_MASK)

/*! @name DCOC_TZA_STEP_8 - DCOC TZA DAC Step 5 */
#define XCVR_RX_DIG_DCOC_TZA_STEP_8_DCOC_TZA_STEP_RCP_8_MASK (0x1FFFU)
#define XCVR_RX_DIG_DCOC_TZA_STEP_8_DCOC_TZA_STEP_RCP_8_SHIFT (0U)
#define XCVR_RX_DIG_DCOC_TZA_STEP_8_DCOC_TZA_STEP_RCP_8(x) (((uint32_t)(((uint32_t)(x)) << XCVR_RX_DIG_DCOC_TZA_STEP_8_DCOC_TZA_STEP_RCP_8_SHIFT)) & XCVR_RX_DIG_DCOC_TZA_STEP_8_DCOC_TZA_STEP_RCP_8_MASK)
#define XCVR_RX_DIG_DCOC_TZA_STEP_8_DCOC_TZA_STEP_GAIN_8_MASK (0x1FFF0000U)
#define XCVR_RX_DIG_DCOC_TZA_STEP_8_DCOC_TZA_STEP_GAIN_8_SHIFT (16U)
#define XCVR_RX_DIG_DCOC_TZA_STEP_8_DCOC_TZA_STEP_GAIN_8(x) (((uint32_t)(((uint32_t)(x)) << XCVR_RX_DIG_DCOC_TZA_STEP_8_DCOC_TZA_STEP_GAIN_8_SHIFT)) & XCVR_RX_DIG_DCOC_TZA_STEP_8_DCOC_TZA_STEP_GAIN_8_MASK)

/*! @name DCOC_TZA_STEP_9 - DCOC TZA DAC Step 9 */
#define XCVR_RX_DIG_DCOC_TZA_STEP_9_DCOC_TZA_STEP_RCP_9_MASK (0x1FFFU)
#define XCVR_RX_DIG_DCOC_TZA_STEP_9_DCOC_TZA_STEP_RCP_9_SHIFT (0U)
#define XCVR_RX_DIG_DCOC_TZA_STEP_9_DCOC_TZA_STEP_RCP_9(x) (((uint32_t)(((uint32_t)(x)) << XCVR_RX_DIG_DCOC_TZA_STEP_9_DCOC_TZA_STEP_RCP_9_SHIFT)) & XCVR_RX_DIG_DCOC_TZA_STEP_9_DCOC_TZA_STEP_RCP_9_MASK)
#define XCVR_RX_DIG_DCOC_TZA_STEP_9_DCOC_TZA_STEP_GAIN_9_MASK (0x3FFF0000U)
#define XCVR_RX_DIG_DCOC_TZA_STEP_9_DCOC_TZA_STEP_GAIN_9_SHIFT (16U)
#define XCVR_RX_DIG_DCOC_TZA_STEP_9_DCOC_TZA_STEP_GAIN_9(x) (((uint32_t)(((uint32_t)(x)) << XCVR_RX_DIG_DCOC_TZA_STEP_9_DCOC_TZA_STEP_GAIN_9_SHIFT)) & XCVR_RX_DIG_DCOC_TZA_STEP_9_DCOC_TZA_STEP_GAIN_9_MASK)

/*! @name DCOC_TZA_STEP_10 - DCOC TZA DAC Step 10 */
#define XCVR_RX_DIG_DCOC_TZA_STEP_10_DCOC_TZA_STEP_RCP_10_MASK (0x1FFFU)
#define XCVR_RX_DIG_DCOC_TZA_STEP_10_DCOC_TZA_STEP_RCP_10_SHIFT (0U)
#define XCVR_RX_DIG_DCOC_TZA_STEP_10_DCOC_TZA_STEP_RCP_10(x) (((uint32_t)(((uint32_t)(x)) << XCVR_RX_DIG_DCOC_TZA_STEP_10_DCOC_TZA_STEP_RCP_10_SHIFT)) & XCVR_RX_DIG_DCOC_TZA_STEP_10_DCOC_TZA_STEP_RCP_10_MASK)
#define XCVR_RX_DIG_DCOC_TZA_STEP_10_DCOC_TZA_STEP_GAIN_10_MASK (0x3FFF0000U)
#define XCVR_RX_DIG_DCOC_TZA_STEP_10_DCOC_TZA_STEP_GAIN_10_SHIFT (16U)
#define XCVR_RX_DIG_DCOC_TZA_STEP_10_DCOC_TZA_STEP_GAIN_10(x) (((uint32_t)(((uint32_t)(x)) << XCVR_RX_DIG_DCOC_TZA_STEP_10_DCOC_TZA_STEP_GAIN_10_SHIFT)) & XCVR_RX_DIG_DCOC_TZA_STEP_10_DCOC_TZA_STEP_GAIN_10_MASK)

/*! @name DCOC_CAL_ALPHA - DCOC Calibration Alpha */
#define XCVR_RX_DIG_DCOC_CAL_ALPHA_DCOC_CAL_ALPHA_I_MASK (0x7FFU)
#define XCVR_RX_DIG_DCOC_CAL_ALPHA_DCOC_CAL_ALPHA_I_SHIFT (0U)
#define XCVR_RX_DIG_DCOC_CAL_ALPHA_DCOC_CAL_ALPHA_I(x) (((uint32_t)(((uint32_t)(x)) << XCVR_RX_DIG_DCOC_CAL_ALPHA_DCOC_CAL_ALPHA_I_SHIFT)) & XCVR_RX_DIG_DCOC_CAL_ALPHA_DCOC_CAL_ALPHA_I_MASK)
#define XCVR_RX_DIG_DCOC_CAL_ALPHA_DCOC_CAL_ALPHA_Q_MASK (0x7FF0000U)
#define XCVR_RX_DIG_DCOC_CAL_ALPHA_DCOC_CAL_ALPHA_Q_SHIFT (16U)
#define XCVR_RX_DIG_DCOC_CAL_ALPHA_DCOC_CAL_ALPHA_Q(x) (((uint32_t)(((uint32_t)(x)) << XCVR_RX_DIG_DCOC_CAL_ALPHA_DCOC_CAL_ALPHA_Q_SHIFT)) & XCVR_RX_DIG_DCOC_CAL_ALPHA_DCOC_CAL_ALPHA_Q_MASK)

/*! @name DCOC_CAL_BETA_Q - DCOC Calibration Beta Q */
#define XCVR_RX_DIG_DCOC_CAL_BETA_Q_DCOC_CAL_BETA_Q_MASK (0x1FFFFU)
#define XCVR_RX_DIG_DCOC_CAL_BETA_Q_DCOC_CAL_BETA_Q_SHIFT (0U)
#define XCVR_RX_DIG_DCOC_CAL_BETA_Q_DCOC_CAL_BETA_Q(x) (((uint32_t)(((uint32_t)(x)) << XCVR_RX_DIG_DCOC_CAL_BETA_Q_DCOC_CAL_BETA_Q_SHIFT)) & XCVR_RX_DIG_DCOC_CAL_BETA_Q_DCOC_CAL_BETA_Q_MASK)

/*! @name DCOC_CAL_BETA_I - DCOC Calibration Beta I */
#define XCVR_RX_DIG_DCOC_CAL_BETA_I_DCOC_CAL_BETA_I_MASK (0x1FFFFU)
#define XCVR_RX_DIG_DCOC_CAL_BETA_I_DCOC_CAL_BETA_I_SHIFT (0U)
#define XCVR_RX_DIG_DCOC_CAL_BETA_I_DCOC_CAL_BETA_I(x) (((uint32_t)(((uint32_t)(x)) << XCVR_RX_DIG_DCOC_CAL_BETA_I_DCOC_CAL_BETA_I_SHIFT)) & XCVR_RX_DIG_DCOC_CAL_BETA_I_DCOC_CAL_BETA_I_MASK)

/*! @name DCOC_CAL_GAMMA - DCOC Calibration Gamma */
#define XCVR_RX_DIG_DCOC_CAL_GAMMA_DCOC_CAL_GAMMA_I_MASK (0xFFFFU)
#define XCVR_RX_DIG_DCOC_CAL_GAMMA_DCOC_CAL_GAMMA_I_SHIFT (0U)
#define XCVR_RX_DIG_DCOC_CAL_GAMMA_DCOC_CAL_GAMMA_I(x) (((uint32_t)(((uint32_t)(x)) << XCVR_RX_DIG_DCOC_CAL_GAMMA_DCOC_CAL_GAMMA_I_SHIFT)) & XCVR_RX_DIG_DCOC_CAL_GAMMA_DCOC_CAL_GAMMA_I_MASK)
#define XCVR_RX_DIG_DCOC_CAL_GAMMA_DCOC_CAL_GAMMA_Q_MASK (0xFFFF0000U)
#define XCVR_RX_DIG_DCOC_CAL_GAMMA_DCOC_CAL_GAMMA_Q_SHIFT (16U)
#define XCVR_RX_DIG_DCOC_CAL_GAMMA_DCOC_CAL_GAMMA_Q(x) (((uint32_t)(((uint32_t)(x)) << XCVR_RX_DIG_DCOC_CAL_GAMMA_DCOC_CAL_GAMMA_Q_SHIFT)) & XCVR_RX_DIG_DCOC_CAL_GAMMA_DCOC_CAL_GAMMA_Q_MASK)

/*! @name DCOC_CAL_IIR - DCOC Calibration IIR */
#define XCVR_RX_DIG_DCOC_CAL_IIR_DCOC_CAL_IIR1A_IDX_MASK (0x3U)
#define XCVR_RX_DIG_DCOC_CAL_IIR_DCOC_CAL_IIR1A_IDX_SHIFT (0U)
#define XCVR_RX_DIG_DCOC_CAL_IIR_DCOC_CAL_IIR1A_IDX(x) (((uint32_t)(((uint32_t)(x)) << XCVR_RX_DIG_DCOC_CAL_IIR_DCOC_CAL_IIR1A_IDX_SHIFT)) & XCVR_RX_DIG_DCOC_CAL_IIR_DCOC_CAL_IIR1A_IDX_MASK)
#define XCVR_RX_DIG_DCOC_CAL_IIR_DCOC_CAL_IIR2A_IDX_MASK (0xCU)
#define XCVR_RX_DIG_DCOC_CAL_IIR_DCOC_CAL_IIR2A_IDX_SHIFT (2U)
#define XCVR_RX_DIG_DCOC_CAL_IIR_DCOC_CAL_IIR2A_IDX(x) (((uint32_t)(((uint32_t)(x)) << XCVR_RX_DIG_DCOC_CAL_IIR_DCOC_CAL_IIR2A_IDX_SHIFT)) & XCVR_RX_DIG_DCOC_CAL_IIR_DCOC_CAL_IIR2A_IDX_MASK)
#define XCVR_RX_DIG_DCOC_CAL_IIR_DCOC_CAL_IIR3A_IDX_MASK (0x30U)
#define XCVR_RX_DIG_DCOC_CAL_IIR_DCOC_CAL_IIR3A_IDX_SHIFT (4U)
#define XCVR_RX_DIG_DCOC_CAL_IIR_DCOC_CAL_IIR3A_IDX(x) (((uint32_t)(((uint32_t)(x)) << XCVR_RX_DIG_DCOC_CAL_IIR_DCOC_CAL_IIR3A_IDX_SHIFT)) & XCVR_RX_DIG_DCOC_CAL_IIR_DCOC_CAL_IIR3A_IDX_MASK)

/*! @name DCOC_CAL - DCOC Calibration Result */
#define XCVR_RX_DIG_DCOC_CAL_DCOC_CAL_RES_I_MASK (0xFFFU)
#define XCVR_RX_DIG_DCOC_CAL_DCOC_CAL_RES_I_SHIFT (0U)
#define XCVR_RX_DIG_DCOC_CAL_DCOC_CAL_RES_I(x)   (((uint32_t)(((uint32_t)(x)) << XCVR_RX_DIG_DCOC_CAL_DCOC_CAL_RES_I_SHIFT)) & XCVR_RX_DIG_DCOC_CAL_DCOC_CAL_RES_I_MASK)
#define XCVR_RX_DIG_DCOC_CAL_DCOC_CAL_RES_Q_MASK (0xFFF0000U)
#define XCVR_RX_DIG_DCOC_CAL_DCOC_CAL_RES_Q_SHIFT (16U)
#define XCVR_RX_DIG_DCOC_CAL_DCOC_CAL_RES_Q(x)   (((uint32_t)(((uint32_t)(x)) << XCVR_RX_DIG_DCOC_CAL_DCOC_CAL_RES_Q_SHIFT)) & XCVR_RX_DIG_DCOC_CAL_DCOC_CAL_RES_Q_MASK)

/* The count of XCVR_RX_DIG_DCOC_CAL */
#define XCVR_RX_DIG_DCOC_CAL_COUNT               (3U)

/*! @name CCA_ED_LQI_CTRL_0 - RX_DIG CCA ED LQI Control Register 0 */
#define XCVR_RX_DIG_CCA_ED_LQI_CTRL_0_LQI_CORR_THRESH_MASK (0xFFU)
#define XCVR_RX_DIG_CCA_ED_LQI_CTRL_0_LQI_CORR_THRESH_SHIFT (0U)
#define XCVR_RX_DIG_CCA_ED_LQI_CTRL_0_LQI_CORR_THRESH(x) (((uint32_t)(((uint32_t)(x)) << XCVR_RX_DIG_CCA_ED_LQI_CTRL_0_LQI_CORR_THRESH_SHIFT)) & XCVR_RX_DIG_CCA_ED_LQI_CTRL_0_LQI_CORR_THRESH_MASK)
#define XCVR_RX_DIG_CCA_ED_LQI_CTRL_0_CORR_CNTR_THRESH_MASK (0xFF00U)
#define XCVR_RX_DIG_CCA_ED_LQI_CTRL_0_CORR_CNTR_THRESH_SHIFT (8U)
#define XCVR_RX_DIG_CCA_ED_LQI_CTRL_0_CORR_CNTR_THRESH(x) (((uint32_t)(((uint32_t)(x)) << XCVR_RX_DIG_CCA_ED_LQI_CTRL_0_CORR_CNTR_THRESH_SHIFT)) & XCVR_RX_DIG_CCA_ED_LQI_CTRL_0_CORR_CNTR_THRESH_MASK)
#define XCVR_RX_DIG_CCA_ED_LQI_CTRL_0_LQI_CNTR_MASK (0xFF0000U)
#define XCVR_RX_DIG_CCA_ED_LQI_CTRL_0_LQI_CNTR_SHIFT (16U)
#define XCVR_RX_DIG_CCA_ED_LQI_CTRL_0_LQI_CNTR(x) (((uint32_t)(((uint32_t)(x)) << XCVR_RX_DIG_CCA_ED_LQI_CTRL_0_LQI_CNTR_SHIFT)) & XCVR_RX_DIG_CCA_ED_LQI_CTRL_0_LQI_CNTR_MASK)
#define XCVR_RX_DIG_CCA_ED_LQI_CTRL_0_SNR_ADJ_MASK (0x3F000000U)
#define XCVR_RX_DIG_CCA_ED_LQI_CTRL_0_SNR_ADJ_SHIFT (24U)
#define XCVR_RX_DIG_CCA_ED_LQI_CTRL_0_SNR_ADJ(x) (((uint32_t)(((uint32_t)(x)) << XCVR_RX_DIG_CCA_ED_LQI_CTRL_0_SNR_ADJ_SHIFT)) & XCVR_RX_DIG_CCA_ED_LQI_CTRL_0_SNR_ADJ_MASK)

/*! @name CCA_ED_LQI_CTRL_1 - RX_DIG CCA ED LQI Control Register 1 */
#define XCVR_RX_DIG_CCA_ED_LQI_CTRL_1_RSSI_NOISE_AVG_DELAY_MASK (0x3FU)
#define XCVR_RX_DIG_CCA_ED_LQI_CTRL_1_RSSI_NOISE_AVG_DELAY_SHIFT (0U)
#define XCVR_RX_DIG_CCA_ED_LQI_CTRL_1_RSSI_NOISE_AVG_DELAY(x) (((uint32_t)(((uint32_t)(x)) << XCVR_RX_DIG_CCA_ED_LQI_CTRL_1_RSSI_NOISE_AVG_DELAY_SHIFT)) & XCVR_RX_DIG_CCA_ED_LQI_CTRL_1_RSSI_NOISE_AVG_DELAY_MASK)
#define XCVR_RX_DIG_CCA_ED_LQI_CTRL_1_RSSI_NOISE_AVG_FACTOR_MASK (0x1C0U)
#define XCVR_RX_DIG_CCA_ED_LQI_CTRL_1_RSSI_NOISE_AVG_FACTOR_SHIFT (6U)
#define XCVR_RX_DIG_CCA_ED_LQI_CTRL_1_RSSI_NOISE_AVG_FACTOR(x) (((uint32_t)(((uint32_t)(x)) << XCVR_RX_DIG_CCA_ED_LQI_CTRL_1_RSSI_NOISE_AVG_FACTOR_SHIFT)) & XCVR_RX_DIG_CCA_ED_LQI_CTRL_1_RSSI_NOISE_AVG_FACTOR_MASK)
#define XCVR_RX_DIG_CCA_ED_LQI_CTRL_1_LQI_RSSI_WEIGHT_MASK (0xE00U)
#define XCVR_RX_DIG_CCA_ED_LQI_CTRL_1_LQI_RSSI_WEIGHT_SHIFT (9U)
#define XCVR_RX_DIG_CCA_ED_LQI_CTRL_1_LQI_RSSI_WEIGHT(x) (((uint32_t)(((uint32_t)(x)) << XCVR_RX_DIG_CCA_ED_LQI_CTRL_1_LQI_RSSI_WEIGHT_SHIFT)) & XCVR_RX_DIG_CCA_ED_LQI_CTRL_1_LQI_RSSI_WEIGHT_MASK)
#define XCVR_RX_DIG_CCA_ED_LQI_CTRL_1_LQI_RSSI_SENS_MASK (0xF000U)
#define XCVR_RX_DIG_CCA_ED_LQI_CTRL_1_LQI_RSSI_SENS_SHIFT (12U)
#define XCVR_RX_DIG_CCA_ED_LQI_CTRL_1_LQI_RSSI_SENS(x) (((uint32_t)(((uint32_t)(x)) << XCVR_RX_DIG_CCA_ED_LQI_CTRL_1_LQI_RSSI_SENS_SHIFT)) & XCVR_RX_DIG_CCA_ED_LQI_CTRL_1_LQI_RSSI_SENS_MASK)
#define XCVR_RX_DIG_CCA_ED_LQI_CTRL_1_SNR_LQI_DIS_MASK (0x10000U)
#define XCVR_RX_DIG_CCA_ED_LQI_CTRL_1_SNR_LQI_DIS_SHIFT (16U)
#define XCVR_RX_DIG_CCA_ED_LQI_CTRL_1_SNR_LQI_DIS(x) (((uint32_t)(((uint32_t)(x)) << XCVR_RX_DIG_CCA_ED_LQI_CTRL_1_SNR_LQI_DIS_SHIFT)) & XCVR_RX_DIG_CCA_ED_LQI_CTRL_1_SNR_LQI_DIS_MASK)
#define XCVR_RX_DIG_CCA_ED_LQI_CTRL_1_SEL_SNR_MODE_MASK (0x20000U)
#define XCVR_RX_DIG_CCA_ED_LQI_CTRL_1_SEL_SNR_MODE_SHIFT (17U)
#define XCVR_RX_DIG_CCA_ED_LQI_CTRL_1_SEL_SNR_MODE(x) (((uint32_t)(((uint32_t)(x)) << XCVR_RX_DIG_CCA_ED_LQI_CTRL_1_SEL_SNR_MODE_SHIFT)) & XCVR_RX_DIG_CCA_ED_LQI_CTRL_1_SEL_SNR_MODE_MASK)
#define XCVR_RX_DIG_CCA_ED_LQI_CTRL_1_MEAS_TRANS_TO_IDLE_MASK (0x40000U)
#define XCVR_RX_DIG_CCA_ED_LQI_CTRL_1_MEAS_TRANS_TO_IDLE_SHIFT (18U)
#define XCVR_RX_DIG_CCA_ED_LQI_CTRL_1_MEAS_TRANS_TO_IDLE(x) (((uint32_t)(((uint32_t)(x)) << XCVR_RX_DIG_CCA_ED_LQI_CTRL_1_MEAS_TRANS_TO_IDLE_SHIFT)) & XCVR_RX_DIG_CCA_ED_LQI_CTRL_1_MEAS_TRANS_TO_IDLE_MASK)
#define XCVR_RX_DIG_CCA_ED_LQI_CTRL_1_CCA1_ED_EN_DIS_MASK (0x80000U)
#define XCVR_RX_DIG_CCA_ED_LQI_CTRL_1_CCA1_ED_EN_DIS_SHIFT (19U)
#define XCVR_RX_DIG_CCA_ED_LQI_CTRL_1_CCA1_ED_EN_DIS(x) (((uint32_t)(((uint32_t)(x)) << XCVR_RX_DIG_CCA_ED_LQI_CTRL_1_CCA1_ED_EN_DIS_SHIFT)) & XCVR_RX_DIG_CCA_ED_LQI_CTRL_1_CCA1_ED_EN_DIS_MASK)
#define XCVR_RX_DIG_CCA_ED_LQI_CTRL_1_MAN_MEAS_COMPLETE_MASK (0x100000U)
#define XCVR_RX_DIG_CCA_ED_LQI_CTRL_1_MAN_MEAS_COMPLETE_SHIFT (20U)
#define XCVR_RX_DIG_CCA_ED_LQI_CTRL_1_MAN_MEAS_COMPLETE(x) (((uint32_t)(((uint32_t)(x)) << XCVR_RX_DIG_CCA_ED_LQI_CTRL_1_MAN_MEAS_COMPLETE_SHIFT)) & XCVR_RX_DIG_CCA_ED_LQI_CTRL_1_MAN_MEAS_COMPLETE_MASK)
#define XCVR_RX_DIG_CCA_ED_LQI_CTRL_1_MAN_AA_MATCH_MASK (0x200000U)
#define XCVR_RX_DIG_CCA_ED_LQI_CTRL_1_MAN_AA_MATCH_SHIFT (21U)
#define XCVR_RX_DIG_CCA_ED_LQI_CTRL_1_MAN_AA_MATCH(x) (((uint32_t)(((uint32_t)(x)) << XCVR_RX_DIG_CCA_ED_LQI_CTRL_1_MAN_AA_MATCH_SHIFT)) & XCVR_RX_DIG_CCA_ED_LQI_CTRL_1_MAN_AA_MATCH_MASK)
#define XCVR_RX_DIG_CCA_ED_LQI_CTRL_1_SNR_LQI_WEIGHT_MASK (0xF000000U)
#define XCVR_RX_DIG_CCA_ED_LQI_CTRL_1_SNR_LQI_WEIGHT_SHIFT (24U)
#define XCVR_RX_DIG_CCA_ED_LQI_CTRL_1_SNR_LQI_WEIGHT(x) (((uint32_t)(((uint32_t)(x)) << XCVR_RX_DIG_CCA_ED_LQI_CTRL_1_SNR_LQI_WEIGHT_SHIFT)) & XCVR_RX_DIG_CCA_ED_LQI_CTRL_1_SNR_LQI_WEIGHT_MASK)
#define XCVR_RX_DIG_CCA_ED_LQI_CTRL_1_LQI_BIAS_MASK (0xF0000000U)
#define XCVR_RX_DIG_CCA_ED_LQI_CTRL_1_LQI_BIAS_SHIFT (28U)
#define XCVR_RX_DIG_CCA_ED_LQI_CTRL_1_LQI_BIAS(x) (((uint32_t)(((uint32_t)(x)) << XCVR_RX_DIG_CCA_ED_LQI_CTRL_1_LQI_BIAS_SHIFT)) & XCVR_RX_DIG_CCA_ED_LQI_CTRL_1_LQI_BIAS_MASK)

/*! @name CCA_ED_LQI_STAT_0 - RX_DIG CCA ED LQI Status Register 0 */
#define XCVR_RX_DIG_CCA_ED_LQI_STAT_0_LQI_OUT_MASK (0xFFU)
#define XCVR_RX_DIG_CCA_ED_LQI_STAT_0_LQI_OUT_SHIFT (0U)
#define XCVR_RX_DIG_CCA_ED_LQI_STAT_0_LQI_OUT(x) (((uint32_t)(((uint32_t)(x)) << XCVR_RX_DIG_CCA_ED_LQI_STAT_0_LQI_OUT_SHIFT)) & XCVR_RX_DIG_CCA_ED_LQI_STAT_0_LQI_OUT_MASK)
#define XCVR_RX_DIG_CCA_ED_LQI_STAT_0_ED_OUT_MASK (0xFF00U)
#define XCVR_RX_DIG_CCA_ED_LQI_STAT_0_ED_OUT_SHIFT (8U)
#define XCVR_RX_DIG_CCA_ED_LQI_STAT_0_ED_OUT(x)  (((uint32_t)(((uint32_t)(x)) << XCVR_RX_DIG_CCA_ED_LQI_STAT_0_ED_OUT_SHIFT)) & XCVR_RX_DIG_CCA_ED_LQI_STAT_0_ED_OUT_MASK)
#define XCVR_RX_DIG_CCA_ED_LQI_STAT_0_SNR_OUT_MASK (0xFF0000U)
#define XCVR_RX_DIG_CCA_ED_LQI_STAT_0_SNR_OUT_SHIFT (16U)
#define XCVR_RX_DIG_CCA_ED_LQI_STAT_0_SNR_OUT(x) (((uint32_t)(((uint32_t)(x)) << XCVR_RX_DIG_CCA_ED_LQI_STAT_0_SNR_OUT_SHIFT)) & XCVR_RX_DIG_CCA_ED_LQI_STAT_0_SNR_OUT_MASK)
#define XCVR_RX_DIG_CCA_ED_LQI_STAT_0_CCA1_STATE_MASK (0x1000000U)
#define XCVR_RX_DIG_CCA_ED_LQI_STAT_0_CCA1_STATE_SHIFT (24U)
#define XCVR_RX_DIG_CCA_ED_LQI_STAT_0_CCA1_STATE(x) (((uint32_t)(((uint32_t)(x)) << XCVR_RX_DIG_CCA_ED_LQI_STAT_0_CCA1_STATE_SHIFT)) & XCVR_RX_DIG_CCA_ED_LQI_STAT_0_CCA1_STATE_MASK)
#define XCVR_RX_DIG_CCA_ED_LQI_STAT_0_MEAS_COMPLETE_MASK (0x2000000U)
#define XCVR_RX_DIG_CCA_ED_LQI_STAT_0_MEAS_COMPLETE_SHIFT (25U)
#define XCVR_RX_DIG_CCA_ED_LQI_STAT_0_MEAS_COMPLETE(x) (((uint32_t)(((uint32_t)(x)) << XCVR_RX_DIG_CCA_ED_LQI_STAT_0_MEAS_COMPLETE_SHIFT)) & XCVR_RX_DIG_CCA_ED_LQI_STAT_0_MEAS_COMPLETE_MASK)

/*! @name RX_CHF_COEF_0 - Receive Channel Filter Coefficient 0 */
#define XCVR_RX_DIG_RX_CHF_COEF_0_RX_CH_FILT_H0_MASK (0x3FU)
#define XCVR_RX_DIG_RX_CHF_COEF_0_RX_CH_FILT_H0_SHIFT (0U)
#define XCVR_RX_DIG_RX_CHF_COEF_0_RX_CH_FILT_H0(x) (((uint32_t)(((uint32_t)(x)) << XCVR_RX_DIG_RX_CHF_COEF_0_RX_CH_FILT_H0_SHIFT)) & XCVR_RX_DIG_RX_CHF_COEF_0_RX_CH_FILT_H0_MASK)

/*! @name RX_CHF_COEF_1 - Receive Channel Filter Coefficient 1 */
#define XCVR_RX_DIG_RX_CHF_COEF_1_RX_CH_FILT_H1_MASK (0x3FU)
#define XCVR_RX_DIG_RX_CHF_COEF_1_RX_CH_FILT_H1_SHIFT (0U)
#define XCVR_RX_DIG_RX_CHF_COEF_1_RX_CH_FILT_H1(x) (((uint32_t)(((uint32_t)(x)) << XCVR_RX_DIG_RX_CHF_COEF_1_RX_CH_FILT_H1_SHIFT)) & XCVR_RX_DIG_RX_CHF_COEF_1_RX_CH_FILT_H1_MASK)

/*! @name RX_CHF_COEF_2 - Receive Channel Filter Coefficient 2 */
#define XCVR_RX_DIG_RX_CHF_COEF_2_RX_CH_FILT_H2_MASK (0x7FU)
#define XCVR_RX_DIG_RX_CHF_COEF_2_RX_CH_FILT_H2_SHIFT (0U)
#define XCVR_RX_DIG_RX_CHF_COEF_2_RX_CH_FILT_H2(x) (((uint32_t)(((uint32_t)(x)) << XCVR_RX_DIG_RX_CHF_COEF_2_RX_CH_FILT_H2_SHIFT)) & XCVR_RX_DIG_RX_CHF_COEF_2_RX_CH_FILT_H2_MASK)

/*! @name RX_CHF_COEF_3 - Receive Channel Filter Coefficient 3 */
#define XCVR_RX_DIG_RX_CHF_COEF_3_RX_CH_FILT_H3_MASK (0x7FU)
#define XCVR_RX_DIG_RX_CHF_COEF_3_RX_CH_FILT_H3_SHIFT (0U)
#define XCVR_RX_DIG_RX_CHF_COEF_3_RX_CH_FILT_H3(x) (((uint32_t)(((uint32_t)(x)) << XCVR_RX_DIG_RX_CHF_COEF_3_RX_CH_FILT_H3_SHIFT)) & XCVR_RX_DIG_RX_CHF_COEF_3_RX_CH_FILT_H3_MASK)

/*! @name RX_CHF_COEF_4 - Receive Channel Filter Coefficient 4 */
#define XCVR_RX_DIG_RX_CHF_COEF_4_RX_CH_FILT_H4_MASK (0x7FU)
#define XCVR_RX_DIG_RX_CHF_COEF_4_RX_CH_FILT_H4_SHIFT (0U)
#define XCVR_RX_DIG_RX_CHF_COEF_4_RX_CH_FILT_H4(x) (((uint32_t)(((uint32_t)(x)) << XCVR_RX_DIG_RX_CHF_COEF_4_RX_CH_FILT_H4_SHIFT)) & XCVR_RX_DIG_RX_CHF_COEF_4_RX_CH_FILT_H4_MASK)

/*! @name RX_CHF_COEF_5 - Receive Channel Filter Coefficient 5 */
#define XCVR_RX_DIG_RX_CHF_COEF_5_RX_CH_FILT_H5_MASK (0x7FU)
#define XCVR_RX_DIG_RX_CHF_COEF_5_RX_CH_FILT_H5_SHIFT (0U)
#define XCVR_RX_DIG_RX_CHF_COEF_5_RX_CH_FILT_H5(x) (((uint32_t)(((uint32_t)(x)) << XCVR_RX_DIG_RX_CHF_COEF_5_RX_CH_FILT_H5_SHIFT)) & XCVR_RX_DIG_RX_CHF_COEF_5_RX_CH_FILT_H5_MASK)

/*! @name RX_CHF_COEF_6 - Receive Channel Filter Coefficient 6 */
#define XCVR_RX_DIG_RX_CHF_COEF_6_RX_CH_FILT_H6_MASK (0xFFU)
#define XCVR_RX_DIG_RX_CHF_COEF_6_RX_CH_FILT_H6_SHIFT (0U)
#define XCVR_RX_DIG_RX_CHF_COEF_6_RX_CH_FILT_H6(x) (((uint32_t)(((uint32_t)(x)) << XCVR_RX_DIG_RX_CHF_COEF_6_RX_CH_FILT_H6_SHIFT)) & XCVR_RX_DIG_RX_CHF_COEF_6_RX_CH_FILT_H6_MASK)

/*! @name RX_CHF_COEF_7 - Receive Channel Filter Coefficient 7 */
#define XCVR_RX_DIG_RX_CHF_COEF_7_RX_CH_FILT_H7_MASK (0xFFU)
#define XCVR_RX_DIG_RX_CHF_COEF_7_RX_CH_FILT_H7_SHIFT (0U)
#define XCVR_RX_DIG_RX_CHF_COEF_7_RX_CH_FILT_H7(x) (((uint32_t)(((uint32_t)(x)) << XCVR_RX_DIG_RX_CHF_COEF_7_RX_CH_FILT_H7_SHIFT)) & XCVR_RX_DIG_RX_CHF_COEF_7_RX_CH_FILT_H7_MASK)

/*! @name RX_CHF_COEF_8 - Receive Channel Filter Coefficient 8 */
#define XCVR_RX_DIG_RX_CHF_COEF_8_RX_CH_FILT_H8_MASK (0x1FFU)
#define XCVR_RX_DIG_RX_CHF_COEF_8_RX_CH_FILT_H8_SHIFT (0U)
#define XCVR_RX_DIG_RX_CHF_COEF_8_RX_CH_FILT_H8(x) (((uint32_t)(((uint32_t)(x)) << XCVR_RX_DIG_RX_CHF_COEF_8_RX_CH_FILT_H8_SHIFT)) & XCVR_RX_DIG_RX_CHF_COEF_8_RX_CH_FILT_H8_MASK)

/*! @name RX_CHF_COEF_9 - Receive Channel Filter Coefficient 9 */
#define XCVR_RX_DIG_RX_CHF_COEF_9_RX_CH_FILT_H9_MASK (0x1FFU)
#define XCVR_RX_DIG_RX_CHF_COEF_9_RX_CH_FILT_H9_SHIFT (0U)
#define XCVR_RX_DIG_RX_CHF_COEF_9_RX_CH_FILT_H9(x) (((uint32_t)(((uint32_t)(x)) << XCVR_RX_DIG_RX_CHF_COEF_9_RX_CH_FILT_H9_SHIFT)) & XCVR_RX_DIG_RX_CHF_COEF_9_RX_CH_FILT_H9_MASK)

/*! @name RX_CHF_COEF_10 - Receive Channel Filter Coefficient 10 */
#define XCVR_RX_DIG_RX_CHF_COEF_10_RX_CH_FILT_H10_MASK (0x3FFU)
#define XCVR_RX_DIG_RX_CHF_COEF_10_RX_CH_FILT_H10_SHIFT (0U)
#define XCVR_RX_DIG_RX_CHF_COEF_10_RX_CH_FILT_H10(x) (((uint32_t)(((uint32_t)(x)) << XCVR_RX_DIG_RX_CHF_COEF_10_RX_CH_FILT_H10_SHIFT)) & XCVR_RX_DIG_RX_CHF_COEF_10_RX_CH_FILT_H10_MASK)

/*! @name RX_CHF_COEF_11 - Receive Channel Filter Coefficient 11 */
#define XCVR_RX_DIG_RX_CHF_COEF_11_RX_CH_FILT_H11_MASK (0x3FFU)
#define XCVR_RX_DIG_RX_CHF_COEF_11_RX_CH_FILT_H11_SHIFT (0U)
#define XCVR_RX_DIG_RX_CHF_COEF_11_RX_CH_FILT_H11(x) (((uint32_t)(((uint32_t)(x)) << XCVR_RX_DIG_RX_CHF_COEF_11_RX_CH_FILT_H11_SHIFT)) & XCVR_RX_DIG_RX_CHF_COEF_11_RX_CH_FILT_H11_MASK)

/*! @name AGC_MAN_AGC_IDX - AGC Manual AGC Index */
#define XCVR_RX_DIG_AGC_MAN_AGC_IDX_AGC_MAN_IDX_MASK (0x1F0000U)
#define XCVR_RX_DIG_AGC_MAN_AGC_IDX_AGC_MAN_IDX_SHIFT (16U)
#define XCVR_RX_DIG_AGC_MAN_AGC_IDX_AGC_MAN_IDX(x) (((uint32_t)(((uint32_t)(x)) << XCVR_RX_DIG_AGC_MAN_AGC_IDX_AGC_MAN_IDX_SHIFT)) & XCVR_RX_DIG_AGC_MAN_AGC_IDX_AGC_MAN_IDX_MASK)
#define XCVR_RX_DIG_AGC_MAN_AGC_IDX_AGC_MAN_IDX_EN_MASK (0x1000000U)
#define XCVR_RX_DIG_AGC_MAN_AGC_IDX_AGC_MAN_IDX_EN_SHIFT (24U)
#define XCVR_RX_DIG_AGC_MAN_AGC_IDX_AGC_MAN_IDX_EN(x) (((uint32_t)(((uint32_t)(x)) << XCVR_RX_DIG_AGC_MAN_AGC_IDX_AGC_MAN_IDX_EN_SHIFT)) & XCVR_RX_DIG_AGC_MAN_AGC_IDX_AGC_MAN_IDX_EN_MASK)
#define XCVR_RX_DIG_AGC_MAN_AGC_IDX_AGC_DCOC_START_PT_MASK (0x2000000U)
#define XCVR_RX_DIG_AGC_MAN_AGC_IDX_AGC_DCOC_START_PT_SHIFT (25U)
#define XCVR_RX_DIG_AGC_MAN_AGC_IDX_AGC_DCOC_START_PT(x) (((uint32_t)(((uint32_t)(x)) << XCVR_RX_DIG_AGC_MAN_AGC_IDX_AGC_DCOC_START_PT_SHIFT)) & XCVR_RX_DIG_AGC_MAN_AGC_IDX_AGC_DCOC_START_PT_MASK)

/*! @name DC_RESID_CTRL - DC Residual Control */
#define XCVR_RX_DIG_DC_RESID_CTRL_DC_RESID_NWIN_MASK (0x7FU)
#define XCVR_RX_DIG_DC_RESID_CTRL_DC_RESID_NWIN_SHIFT (0U)
#define XCVR_RX_DIG_DC_RESID_CTRL_DC_RESID_NWIN(x) (((uint32_t)(((uint32_t)(x)) << XCVR_RX_DIG_DC_RESID_CTRL_DC_RESID_NWIN_SHIFT)) & XCVR_RX_DIG_DC_RESID_CTRL_DC_RESID_NWIN_MASK)
#define XCVR_RX_DIG_DC_RESID_CTRL_DC_RESID_ITER_FREEZE_MASK (0xF00U)
#define XCVR_RX_DIG_DC_RESID_CTRL_DC_RESID_ITER_FREEZE_SHIFT (8U)
#define XCVR_RX_DIG_DC_RESID_CTRL_DC_RESID_ITER_FREEZE(x) (((uint32_t)(((uint32_t)(x)) << XCVR_RX_DIG_DC_RESID_CTRL_DC_RESID_ITER_FREEZE_SHIFT)) & XCVR_RX_DIG_DC_RESID_CTRL_DC_RESID_ITER_FREEZE_MASK)
#define XCVR_RX_DIG_DC_RESID_CTRL_DC_RESID_ALPHA_MASK (0x7000U)
#define XCVR_RX_DIG_DC_RESID_CTRL_DC_RESID_ALPHA_SHIFT (12U)
#define XCVR_RX_DIG_DC_RESID_CTRL_DC_RESID_ALPHA(x) (((uint32_t)(((uint32_t)(x)) << XCVR_RX_DIG_DC_RESID_CTRL_DC_RESID_ALPHA_SHIFT)) & XCVR_RX_DIG_DC_RESID_CTRL_DC_RESID_ALPHA_MASK)
#define XCVR_RX_DIG_DC_RESID_CTRL_DC_RESID_DLY_MASK (0x70000U)
#define XCVR_RX_DIG_DC_RESID_CTRL_DC_RESID_DLY_SHIFT (16U)
#define XCVR_RX_DIG_DC_RESID_CTRL_DC_RESID_DLY(x) (((uint32_t)(((uint32_t)(x)) << XCVR_RX_DIG_DC_RESID_CTRL_DC_RESID_DLY_SHIFT)) & XCVR_RX_DIG_DC_RESID_CTRL_DC_RESID_DLY_MASK)
#define XCVR_RX_DIG_DC_RESID_CTRL_DC_RESID_EXT_DC_EN_MASK (0x100000U)
#define XCVR_RX_DIG_DC_RESID_CTRL_DC_RESID_EXT_DC_EN_SHIFT (20U)
#define XCVR_RX_DIG_DC_RESID_CTRL_DC_RESID_EXT_DC_EN(x) (((uint32_t)(((uint32_t)(x)) << XCVR_RX_DIG_DC_RESID_CTRL_DC_RESID_EXT_DC_EN_SHIFT)) & XCVR_RX_DIG_DC_RESID_CTRL_DC_RESID_EXT_DC_EN_MASK)
#define XCVR_RX_DIG_DC_RESID_CTRL_DC_RESID_MIN_AGC_IDX_MASK (0x1F000000U)
#define XCVR_RX_DIG_DC_RESID_CTRL_DC_RESID_MIN_AGC_IDX_SHIFT (24U)
#define XCVR_RX_DIG_DC_RESID_CTRL_DC_RESID_MIN_AGC_IDX(x) (((uint32_t)(((uint32_t)(x)) << XCVR_RX_DIG_DC_RESID_CTRL_DC_RESID_MIN_AGC_IDX_SHIFT)) & XCVR_RX_DIG_DC_RESID_CTRL_DC_RESID_MIN_AGC_IDX_MASK)

/*! @name DC_RESID_EST - DC Residual Estimate */
#define XCVR_RX_DIG_DC_RESID_EST_DC_RESID_OFFSET_I_MASK (0x1FFFU)
#define XCVR_RX_DIG_DC_RESID_EST_DC_RESID_OFFSET_I_SHIFT (0U)
#define XCVR_RX_DIG_DC_RESID_EST_DC_RESID_OFFSET_I(x) (((uint32_t)(((uint32_t)(x)) << XCVR_RX_DIG_DC_RESID_EST_DC_RESID_OFFSET_I_SHIFT)) & XCVR_RX_DIG_DC_RESID_EST_DC_RESID_OFFSET_I_MASK)
#define XCVR_RX_DIG_DC_RESID_EST_DC_RESID_OFFSET_Q_MASK (0x1FFF0000U)
#define XCVR_RX_DIG_DC_RESID_EST_DC_RESID_OFFSET_Q_SHIFT (16U)
#define XCVR_RX_DIG_DC_RESID_EST_DC_RESID_OFFSET_Q(x) (((uint32_t)(((uint32_t)(x)) << XCVR_RX_DIG_DC_RESID_EST_DC_RESID_OFFSET_Q_SHIFT)) & XCVR_RX_DIG_DC_RESID_EST_DC_RESID_OFFSET_Q_MASK)

/*! @name RX_RCCAL_CTRL0 - RX RC Calibration Control0 */
#define XCVR_RX_DIG_RX_RCCAL_CTRL0_BBA_RCCAL_OFFSET_MASK (0xFU)
#define XCVR_RX_DIG_RX_RCCAL_CTRL0_BBA_RCCAL_OFFSET_SHIFT (0U)
#define XCVR_RX_DIG_RX_RCCAL_CTRL0_BBA_RCCAL_OFFSET(x) (((uint32_t)(((uint32_t)(x)) << XCVR_RX_DIG_RX_RCCAL_CTRL0_BBA_RCCAL_OFFSET_SHIFT)) & XCVR_RX_DIG_RX_RCCAL_CTRL0_BBA_RCCAL_OFFSET_MASK)
#define XCVR_RX_DIG_RX_RCCAL_CTRL0_BBA_RCCAL_MANUAL_MASK (0x1F0U)
#define XCVR_RX_DIG_RX_RCCAL_CTRL0_BBA_RCCAL_MANUAL_SHIFT (4U)
#define XCVR_RX_DIG_RX_RCCAL_CTRL0_BBA_RCCAL_MANUAL(x) (((uint32_t)(((uint32_t)(x)) << XCVR_RX_DIG_RX_RCCAL_CTRL0_BBA_RCCAL_MANUAL_SHIFT)) & XCVR_RX_DIG_RX_RCCAL_CTRL0_BBA_RCCAL_MANUAL_MASK)
#define XCVR_RX_DIG_RX_RCCAL_CTRL0_BBA_RCCAL_DIS_MASK (0x200U)
#define XCVR_RX_DIG_RX_RCCAL_CTRL0_BBA_RCCAL_DIS_SHIFT (9U)
#define XCVR_RX_DIG_RX_RCCAL_CTRL0_BBA_RCCAL_DIS(x) (((uint32_t)(((uint32_t)(x)) << XCVR_RX_DIG_RX_RCCAL_CTRL0_BBA_RCCAL_DIS_SHIFT)) & XCVR_RX_DIG_RX_RCCAL_CTRL0_BBA_RCCAL_DIS_MASK)
#define XCVR_RX_DIG_RX_RCCAL_CTRL0_RCCAL_SMP_DLY_MASK (0x3000U)
#define XCVR_RX_DIG_RX_RCCAL_CTRL0_RCCAL_SMP_DLY_SHIFT (12U)
#define XCVR_RX_DIG_RX_RCCAL_CTRL0_RCCAL_SMP_DLY(x) (((uint32_t)(((uint32_t)(x)) << XCVR_RX_DIG_RX_RCCAL_CTRL0_RCCAL_SMP_DLY_SHIFT)) & XCVR_RX_DIG_RX_RCCAL_CTRL0_RCCAL_SMP_DLY_MASK)
#define XCVR_RX_DIG_RX_RCCAL_CTRL0_RCCAL_COMP_INV_MASK (0x8000U)
#define XCVR_RX_DIG_RX_RCCAL_CTRL0_RCCAL_COMP_INV_SHIFT (15U)
#define XCVR_RX_DIG_RX_RCCAL_CTRL0_RCCAL_COMP_INV(x) (((uint32_t)(((uint32_t)(x)) << XCVR_RX_DIG_RX_RCCAL_CTRL0_RCCAL_COMP_INV_SHIFT)) & XCVR_RX_DIG_RX_RCCAL_CTRL0_RCCAL_COMP_INV_MASK)
#define XCVR_RX_DIG_RX_RCCAL_CTRL0_TZA_RCCAL_OFFSET_MASK (0xF0000U)
#define XCVR_RX_DIG_RX_RCCAL_CTRL0_TZA_RCCAL_OFFSET_SHIFT (16U)
#define XCVR_RX_DIG_RX_RCCAL_CTRL0_TZA_RCCAL_OFFSET(x) (((uint32_t)(((uint32_t)(x)) << XCVR_RX_DIG_RX_RCCAL_CTRL0_TZA_RCCAL_OFFSET_SHIFT)) & XCVR_RX_DIG_RX_RCCAL_CTRL0_TZA_RCCAL_OFFSET_MASK)
#define XCVR_RX_DIG_RX_RCCAL_CTRL0_TZA_RCCAL_MANUAL_MASK (0x1F00000U)
#define XCVR_RX_DIG_RX_RCCAL_CTRL0_TZA_RCCAL_MANUAL_SHIFT (20U)
#define XCVR_RX_DIG_RX_RCCAL_CTRL0_TZA_RCCAL_MANUAL(x) (((uint32_t)(((uint32_t)(x)) << XCVR_RX_DIG_RX_RCCAL_CTRL0_TZA_RCCAL_MANUAL_SHIFT)) & XCVR_RX_DIG_RX_RCCAL_CTRL0_TZA_RCCAL_MANUAL_MASK)
#define XCVR_RX_DIG_RX_RCCAL_CTRL0_TZA_RCCAL_DIS_MASK (0x2000000U)
#define XCVR_RX_DIG_RX_RCCAL_CTRL0_TZA_RCCAL_DIS_SHIFT (25U)
#define XCVR_RX_DIG_RX_RCCAL_CTRL0_TZA_RCCAL_DIS(x) (((uint32_t)(((uint32_t)(x)) << XCVR_RX_DIG_RX_RCCAL_CTRL0_TZA_RCCAL_DIS_SHIFT)) & XCVR_RX_DIG_RX_RCCAL_CTRL0_TZA_RCCAL_DIS_MASK)

/*! @name RX_RCCAL_CTRL1 - RX RC Calibration Control1 */
#define XCVR_RX_DIG_RX_RCCAL_CTRL1_ADC_RCCAL_OFFSET_MASK (0xFU)
#define XCVR_RX_DIG_RX_RCCAL_CTRL1_ADC_RCCAL_OFFSET_SHIFT (0U)
#define XCVR_RX_DIG_RX_RCCAL_CTRL1_ADC_RCCAL_OFFSET(x) (((uint32_t)(((uint32_t)(x)) << XCVR_RX_DIG_RX_RCCAL_CTRL1_ADC_RCCAL_OFFSET_SHIFT)) & XCVR_RX_DIG_RX_RCCAL_CTRL1_ADC_RCCAL_OFFSET_MASK)
#define XCVR_RX_DIG_RX_RCCAL_CTRL1_ADC_RCCAL_MANUAL_MASK (0x1F0U)
#define XCVR_RX_DIG_RX_RCCAL_CTRL1_ADC_RCCAL_MANUAL_SHIFT (4U)
#define XCVR_RX_DIG_RX_RCCAL_CTRL1_ADC_RCCAL_MANUAL(x) (((uint32_t)(((uint32_t)(x)) << XCVR_RX_DIG_RX_RCCAL_CTRL1_ADC_RCCAL_MANUAL_SHIFT)) & XCVR_RX_DIG_RX_RCCAL_CTRL1_ADC_RCCAL_MANUAL_MASK)
#define XCVR_RX_DIG_RX_RCCAL_CTRL1_ADC_RCCAL_DIS_MASK (0x200U)
#define XCVR_RX_DIG_RX_RCCAL_CTRL1_ADC_RCCAL_DIS_SHIFT (9U)
#define XCVR_RX_DIG_RX_RCCAL_CTRL1_ADC_RCCAL_DIS(x) (((uint32_t)(((uint32_t)(x)) << XCVR_RX_DIG_RX_RCCAL_CTRL1_ADC_RCCAL_DIS_SHIFT)) & XCVR_RX_DIG_RX_RCCAL_CTRL1_ADC_RCCAL_DIS_MASK)
#define XCVR_RX_DIG_RX_RCCAL_CTRL1_BBA2_RCCAL_OFFSET_MASK (0xF0000U)
#define XCVR_RX_DIG_RX_RCCAL_CTRL1_BBA2_RCCAL_OFFSET_SHIFT (16U)
#define XCVR_RX_DIG_RX_RCCAL_CTRL1_BBA2_RCCAL_OFFSET(x) (((uint32_t)(((uint32_t)(x)) << XCVR_RX_DIG_RX_RCCAL_CTRL1_BBA2_RCCAL_OFFSET_SHIFT)) & XCVR_RX_DIG_RX_RCCAL_CTRL1_BBA2_RCCAL_OFFSET_MASK)
#define XCVR_RX_DIG_RX_RCCAL_CTRL1_BBA2_RCCAL_MANUAL_MASK (0x1F00000U)
#define XCVR_RX_DIG_RX_RCCAL_CTRL1_BBA2_RCCAL_MANUAL_SHIFT (20U)
#define XCVR_RX_DIG_RX_RCCAL_CTRL1_BBA2_RCCAL_MANUAL(x) (((uint32_t)(((uint32_t)(x)) << XCVR_RX_DIG_RX_RCCAL_CTRL1_BBA2_RCCAL_MANUAL_SHIFT)) & XCVR_RX_DIG_RX_RCCAL_CTRL1_BBA2_RCCAL_MANUAL_MASK)
#define XCVR_RX_DIG_RX_RCCAL_CTRL1_BBA2_RCCAL_DIS_MASK (0x2000000U)
#define XCVR_RX_DIG_RX_RCCAL_CTRL1_BBA2_RCCAL_DIS_SHIFT (25U)
#define XCVR_RX_DIG_RX_RCCAL_CTRL1_BBA2_RCCAL_DIS(x) (((uint32_t)(((uint32_t)(x)) << XCVR_RX_DIG_RX_RCCAL_CTRL1_BBA2_RCCAL_DIS_SHIFT)) & XCVR_RX_DIG_RX_RCCAL_CTRL1_BBA2_RCCAL_DIS_MASK)

/*! @name RX_RCCAL_STAT - RX RC Calibration Status */
#define XCVR_RX_DIG_RX_RCCAL_STAT_RCCAL_CODE_MASK (0x1FU)
#define XCVR_RX_DIG_RX_RCCAL_STAT_RCCAL_CODE_SHIFT (0U)
#define XCVR_RX_DIG_RX_RCCAL_STAT_RCCAL_CODE(x)  (((uint32_t)(((uint32_t)(x)) << XCVR_RX_DIG_RX_RCCAL_STAT_RCCAL_CODE_SHIFT)) & XCVR_RX_DIG_RX_RCCAL_STAT_RCCAL_CODE_MASK)
#define XCVR_RX_DIG_RX_RCCAL_STAT_ADC_RCCAL_MASK (0x3E0U)
#define XCVR_RX_DIG_RX_RCCAL_STAT_ADC_RCCAL_SHIFT (5U)
#define XCVR_RX_DIG_RX_RCCAL_STAT_ADC_RCCAL(x)   (((uint32_t)(((uint32_t)(x)) << XCVR_RX_DIG_RX_RCCAL_STAT_ADC_RCCAL_SHIFT)) & XCVR_RX_DIG_RX_RCCAL_STAT_ADC_RCCAL_MASK)
#define XCVR_RX_DIG_RX_RCCAL_STAT_BBA2_RCCAL_MASK (0x7C00U)
#define XCVR_RX_DIG_RX_RCCAL_STAT_BBA2_RCCAL_SHIFT (10U)
#define XCVR_RX_DIG_RX_RCCAL_STAT_BBA2_RCCAL(x)  (((uint32_t)(((uint32_t)(x)) << XCVR_RX_DIG_RX_RCCAL_STAT_BBA2_RCCAL_SHIFT)) & XCVR_RX_DIG_RX_RCCAL_STAT_BBA2_RCCAL_MASK)
#define XCVR_RX_DIG_RX_RCCAL_STAT_BBA_RCCAL_MASK (0x1F0000U)
#define XCVR_RX_DIG_RX_RCCAL_STAT_BBA_RCCAL_SHIFT (16U)
#define XCVR_RX_DIG_RX_RCCAL_STAT_BBA_RCCAL(x)   (((uint32_t)(((uint32_t)(x)) << XCVR_RX_DIG_RX_RCCAL_STAT_BBA_RCCAL_SHIFT)) & XCVR_RX_DIG_RX_RCCAL_STAT_BBA_RCCAL_MASK)
#define XCVR_RX_DIG_RX_RCCAL_STAT_TZA_RCCAL_MASK (0x3E00000U)
#define XCVR_RX_DIG_RX_RCCAL_STAT_TZA_RCCAL_SHIFT (21U)
#define XCVR_RX_DIG_RX_RCCAL_STAT_TZA_RCCAL(x)   (((uint32_t)(((uint32_t)(x)) << XCVR_RX_DIG_RX_RCCAL_STAT_TZA_RCCAL_SHIFT)) & XCVR_RX_DIG_RX_RCCAL_STAT_TZA_RCCAL_MASK)

/*! @name AUXPLL_FCAL_CTRL - Aux PLL Frequency Calibration Control */
#define XCVR_RX_DIG_AUXPLL_FCAL_CTRL_DAC_CAL_ADJUST_MANUAL_MASK (0x7FU)
#define XCVR_RX_DIG_AUXPLL_FCAL_CTRL_DAC_CAL_ADJUST_MANUAL_SHIFT (0U)
#define XCVR_RX_DIG_AUXPLL_FCAL_CTRL_DAC_CAL_ADJUST_MANUAL(x) (((uint32_t)(((uint32_t)(x)) << XCVR_RX_DIG_AUXPLL_FCAL_CTRL_DAC_CAL_ADJUST_MANUAL_SHIFT)) & XCVR_RX_DIG_AUXPLL_FCAL_CTRL_DAC_CAL_ADJUST_MANUAL_MASK)
#define XCVR_RX_DIG_AUXPLL_FCAL_CTRL_AUXPLL_DAC_CAL_ADJUST_DIS_MASK (0x80U)
#define XCVR_RX_DIG_AUXPLL_FCAL_CTRL_AUXPLL_DAC_CAL_ADJUST_DIS_SHIFT (7U)
#define XCVR_RX_DIG_AUXPLL_FCAL_CTRL_AUXPLL_DAC_CAL_ADJUST_DIS(x) (((uint32_t)(((uint32_t)(x)) << XCVR_RX_DIG_AUXPLL_FCAL_CTRL_AUXPLL_DAC_CAL_ADJUST_DIS_SHIFT)) & XCVR_RX_DIG_AUXPLL_FCAL_CTRL_AUXPLL_DAC_CAL_ADJUST_DIS_MASK)
#define XCVR_RX_DIG_AUXPLL_FCAL_CTRL_FCAL_RUN_CNT_MASK (0x100U)
#define XCVR_RX_DIG_AUXPLL_FCAL_CTRL_FCAL_RUN_CNT_SHIFT (8U)
#define XCVR_RX_DIG_AUXPLL_FCAL_CTRL_FCAL_RUN_CNT(x) (((uint32_t)(((uint32_t)(x)) << XCVR_RX_DIG_AUXPLL_FCAL_CTRL_FCAL_RUN_CNT_SHIFT)) & XCVR_RX_DIG_AUXPLL_FCAL_CTRL_FCAL_RUN_CNT_MASK)
#define XCVR_RX_DIG_AUXPLL_FCAL_CTRL_FCAL_COMP_INV_MASK (0x200U)
#define XCVR_RX_DIG_AUXPLL_FCAL_CTRL_FCAL_COMP_INV_SHIFT (9U)
#define XCVR_RX_DIG_AUXPLL_FCAL_CTRL_FCAL_COMP_INV(x) (((uint32_t)(((uint32_t)(x)) << XCVR_RX_DIG_AUXPLL_FCAL_CTRL_FCAL_COMP_INV_SHIFT)) & XCVR_RX_DIG_AUXPLL_FCAL_CTRL_FCAL_COMP_INV_MASK)
#define XCVR_RX_DIG_AUXPLL_FCAL_CTRL_FCAL_SMP_DLY_MASK (0xC00U)
#define XCVR_RX_DIG_AUXPLL_FCAL_CTRL_FCAL_SMP_DLY_SHIFT (10U)
#define XCVR_RX_DIG_AUXPLL_FCAL_CTRL_FCAL_SMP_DLY(x) (((uint32_t)(((uint32_t)(x)) << XCVR_RX_DIG_AUXPLL_FCAL_CTRL_FCAL_SMP_DLY_SHIFT)) & XCVR_RX_DIG_AUXPLL_FCAL_CTRL_FCAL_SMP_DLY_MASK)
#define XCVR_RX_DIG_AUXPLL_FCAL_CTRL_DAC_CAL_ADJUST_MASK (0x7F0000U)
#define XCVR_RX_DIG_AUXPLL_FCAL_CTRL_DAC_CAL_ADJUST_SHIFT (16U)
#define XCVR_RX_DIG_AUXPLL_FCAL_CTRL_DAC_CAL_ADJUST(x) (((uint32_t)(((uint32_t)(x)) << XCVR_RX_DIG_AUXPLL_FCAL_CTRL_DAC_CAL_ADJUST_SHIFT)) & XCVR_RX_DIG_AUXPLL_FCAL_CTRL_DAC_CAL_ADJUST_MASK)

/*! @name AUXPLL_FCAL_CNT6 - Aux PLL Frequency Calibration Count 6 */
#define XCVR_RX_DIG_AUXPLL_FCAL_CNT6_FCAL_COUNT_6_MASK (0x3FFU)
#define XCVR_RX_DIG_AUXPLL_FCAL_CNT6_FCAL_COUNT_6_SHIFT (0U)
#define XCVR_RX_DIG_AUXPLL_FCAL_CNT6_FCAL_COUNT_6(x) (((uint32_t)(((uint32_t)(x)) << XCVR_RX_DIG_AUXPLL_FCAL_CNT6_FCAL_COUNT_6_SHIFT)) & XCVR_RX_DIG_AUXPLL_FCAL_CNT6_FCAL_COUNT_6_MASK)
#define XCVR_RX_DIG_AUXPLL_FCAL_CNT6_FCAL_BESTDIFF_MASK (0x3FF0000U)
#define XCVR_RX_DIG_AUXPLL_FCAL_CNT6_FCAL_BESTDIFF_SHIFT (16U)
#define XCVR_RX_DIG_AUXPLL_FCAL_CNT6_FCAL_BESTDIFF(x) (((uint32_t)(((uint32_t)(x)) << XCVR_RX_DIG_AUXPLL_FCAL_CNT6_FCAL_BESTDIFF_SHIFT)) & XCVR_RX_DIG_AUXPLL_FCAL_CNT6_FCAL_BESTDIFF_MASK)

/*! @name AUXPLL_FCAL_CNT5_4 - Aux PLL Frequency Calibration Count 5 and 4 */
#define XCVR_RX_DIG_AUXPLL_FCAL_CNT5_4_FCAL_COUNT_4_MASK (0x3FFU)
#define XCVR_RX_DIG_AUXPLL_FCAL_CNT5_4_FCAL_COUNT_4_SHIFT (0U)
#define XCVR_RX_DIG_AUXPLL_FCAL_CNT5_4_FCAL_COUNT_4(x) (((uint32_t)(((uint32_t)(x)) << XCVR_RX_DIG_AUXPLL_FCAL_CNT5_4_FCAL_COUNT_4_SHIFT)) & XCVR_RX_DIG_AUXPLL_FCAL_CNT5_4_FCAL_COUNT_4_MASK)
#define XCVR_RX_DIG_AUXPLL_FCAL_CNT5_4_FCAL_COUNT_5_MASK (0x3FF0000U)
#define XCVR_RX_DIG_AUXPLL_FCAL_CNT5_4_FCAL_COUNT_5_SHIFT (16U)
#define XCVR_RX_DIG_AUXPLL_FCAL_CNT5_4_FCAL_COUNT_5(x) (((uint32_t)(((uint32_t)(x)) << XCVR_RX_DIG_AUXPLL_FCAL_CNT5_4_FCAL_COUNT_5_SHIFT)) & XCVR_RX_DIG_AUXPLL_FCAL_CNT5_4_FCAL_COUNT_5_MASK)

/*! @name AUXPLL_FCAL_CNT3_2 - Aux PLL Frequency Calibration Count 3 and 2 */
#define XCVR_RX_DIG_AUXPLL_FCAL_CNT3_2_FCAL_COUNT_2_MASK (0x3FFU)
#define XCVR_RX_DIG_AUXPLL_FCAL_CNT3_2_FCAL_COUNT_2_SHIFT (0U)
#define XCVR_RX_DIG_AUXPLL_FCAL_CNT3_2_FCAL_COUNT_2(x) (((uint32_t)(((uint32_t)(x)) << XCVR_RX_DIG_AUXPLL_FCAL_CNT3_2_FCAL_COUNT_2_SHIFT)) & XCVR_RX_DIG_AUXPLL_FCAL_CNT3_2_FCAL_COUNT_2_MASK)
#define XCVR_RX_DIG_AUXPLL_FCAL_CNT3_2_FCAL_COUNT_3_MASK (0x3FF0000U)
#define XCVR_RX_DIG_AUXPLL_FCAL_CNT3_2_FCAL_COUNT_3_SHIFT (16U)
#define XCVR_RX_DIG_AUXPLL_FCAL_CNT3_2_FCAL_COUNT_3(x) (((uint32_t)(((uint32_t)(x)) << XCVR_RX_DIG_AUXPLL_FCAL_CNT3_2_FCAL_COUNT_3_SHIFT)) & XCVR_RX_DIG_AUXPLL_FCAL_CNT3_2_FCAL_COUNT_3_MASK)

/*! @name AUXPLL_FCAL_CNT1_0 - Aux PLL Frequency Calibration Count 1 and 0 */
#define XCVR_RX_DIG_AUXPLL_FCAL_CNT1_0_FCAL_COUNT_0_MASK (0x3FFU)
#define XCVR_RX_DIG_AUXPLL_FCAL_CNT1_0_FCAL_COUNT_0_SHIFT (0U)
#define XCVR_RX_DIG_AUXPLL_FCAL_CNT1_0_FCAL_COUNT_0(x) (((uint32_t)(((uint32_t)(x)) << XCVR_RX_DIG_AUXPLL_FCAL_CNT1_0_FCAL_COUNT_0_SHIFT)) & XCVR_RX_DIG_AUXPLL_FCAL_CNT1_0_FCAL_COUNT_0_MASK)
#define XCVR_RX_DIG_AUXPLL_FCAL_CNT1_0_FCAL_COUNT_1_MASK (0x3FF0000U)
#define XCVR_RX_DIG_AUXPLL_FCAL_CNT1_0_FCAL_COUNT_1_SHIFT (16U)
#define XCVR_RX_DIG_AUXPLL_FCAL_CNT1_0_FCAL_COUNT_1(x) (((uint32_t)(((uint32_t)(x)) << XCVR_RX_DIG_AUXPLL_FCAL_CNT1_0_FCAL_COUNT_1_SHIFT)) & XCVR_RX_DIG_AUXPLL_FCAL_CNT1_0_FCAL_COUNT_1_MASK)

/*! @name RXDIG_DFT - RXDIG DFT */
#define XCVR_RX_DIG_RXDIG_DFT_DFT_TONE_FREQ_MASK (0x7U)
#define XCVR_RX_DIG_RXDIG_DFT_DFT_TONE_FREQ_SHIFT (0U)
#define XCVR_RX_DIG_RXDIG_DFT_DFT_TONE_FREQ(x)   (((uint32_t)(((uint32_t)(x)) << XCVR_RX_DIG_RXDIG_DFT_DFT_TONE_FREQ_SHIFT)) & XCVR_RX_DIG_RXDIG_DFT_DFT_TONE_FREQ_MASK)
#define XCVR_RX_DIG_RXDIG_DFT_DFT_TONE_SCALE_MASK (0x8U)
#define XCVR_RX_DIG_RXDIG_DFT_DFT_TONE_SCALE_SHIFT (3U)
#define XCVR_RX_DIG_RXDIG_DFT_DFT_TONE_SCALE(x)  (((uint32_t)(((uint32_t)(x)) << XCVR_RX_DIG_RXDIG_DFT_DFT_TONE_SCALE_SHIFT)) & XCVR_RX_DIG_RXDIG_DFT_DFT_TONE_SCALE_MASK)
#define XCVR_RX_DIG_RXDIG_DFT_DFT_TONE_TZA_EN_MASK (0x10U)
#define XCVR_RX_DIG_RXDIG_DFT_DFT_TONE_TZA_EN_SHIFT (4U)
#define XCVR_RX_DIG_RXDIG_DFT_DFT_TONE_TZA_EN(x) (((uint32_t)(((uint32_t)(x)) << XCVR_RX_DIG_RXDIG_DFT_DFT_TONE_TZA_EN_SHIFT)) & XCVR_RX_DIG_RXDIG_DFT_DFT_TONE_TZA_EN_MASK)
#define XCVR_RX_DIG_RXDIG_DFT_DFT_TONE_BBA_EN_MASK (0x20U)
#define XCVR_RX_DIG_RXDIG_DFT_DFT_TONE_BBA_EN_SHIFT (5U)
#define XCVR_RX_DIG_RXDIG_DFT_DFT_TONE_BBA_EN(x) (((uint32_t)(((uint32_t)(x)) << XCVR_RX_DIG_RXDIG_DFT_DFT_TONE_BBA_EN_SHIFT)) & XCVR_RX_DIG_RXDIG_DFT_DFT_TONE_BBA_EN_MASK)


/*!
 * @}
 */ /* end of group XCVR_RX_DIG_Register_Masks */


/* XCVR_RX_DIG - Peripheral instance base addresses */
/** Peripheral XCVR_RX_DIG base address */
#define XCVR_RX_DIG_BASE                         (0x4005C000u)
/** Peripheral XCVR_RX_DIG base pointer */
#define XCVR_RX_DIG                              ((XCVR_RX_DIG_Type *)XCVR_RX_DIG_BASE)
/** Array initializer of XCVR_RX_DIG peripheral base addresses */
#define XCVR_RX_DIG_BASE_ADDRS                   { XCVR_RX_DIG_BASE }
/** Array initializer of XCVR_RX_DIG peripheral base pointers */
#define XCVR_RX_DIG_BASE_PTRS                    { XCVR_RX_DIG }

/*!
 * @}
 */ /* end of group XCVR_RX_DIG_Peripheral_Access_Layer */


/* ----------------------------------------------------------------------------
   -- XCVR_TSM Peripheral Access Layer
   ---------------------------------------------------------------------------- */

/*!
 * @addtogroup XCVR_TSM_Peripheral_Access_Layer XCVR_TSM Peripheral Access Layer
 * @{
 */

/** XCVR_TSM - Register Layout Typedef */
typedef struct {
  __IO uint32_t CTRL;                              /**< TRANSCEIVER SEQUENCE MANAGER CONTROL, offset: 0x0 */
  __IO uint32_t END_OF_SEQ;                        /**< TSM END OF SEQUENCE, offset: 0x4 */
  __IO uint32_t OVRD0;                             /**< TSM OVERRIDE REGISTER 0, offset: 0x8 */
  __IO uint32_t OVRD1;                             /**< TSM OVERRIDE REGISTER 1, offset: 0xC */
  __IO uint32_t OVRD2;                             /**< TSM OVERRIDE REGISTER 2, offset: 0x10 */
  __IO uint32_t OVRD3;                             /**< TSM OVERRIDE REGISTER 3, offset: 0x14 */
  __IO uint32_t PA_POWER;                          /**< PA POWER, offset: 0x18 */
  __IO uint32_t PA_RAMP_TBL0;                      /**< PA RAMP TABLE 0, offset: 0x1C */
  __IO uint32_t PA_RAMP_TBL1;                      /**< PA RAMP TABLE 1, offset: 0x20 */
  __IO uint32_t RECYCLE_COUNT;                     /**< TSM RECYCLE COUNT, offset: 0x24 */
  __IO uint32_t FAST_CTRL1;                        /**< TSM FAST WARMUP CONTROL REGISTER 1, offset: 0x28 */
  __IO uint32_t FAST_CTRL2;                        /**< TSM FAST WARMUP CONTROL REGISTER 2, offset: 0x2C */
  __IO uint32_t TIMING00;                          /**< TSM_TIMING00, offset: 0x30 */
  __IO uint32_t TIMING01;                          /**< TSM_TIMING01, offset: 0x34 */
  __IO uint32_t TIMING02;                          /**< TSM_TIMING02, offset: 0x38 */
  __IO uint32_t TIMING03;                          /**< TSM_TIMING03, offset: 0x3C */
  __IO uint32_t TIMING04;                          /**< TSM_TIMING04, offset: 0x40 */
  __IO uint32_t TIMING05;                          /**< TSM_TIMING05, offset: 0x44 */
  __IO uint32_t TIMING06;                          /**< TSM_TIMING06, offset: 0x48 */
  __IO uint32_t TIMING07;                          /**< TSM_TIMING07, offset: 0x4C */
  __IO uint32_t TIMING08;                          /**< TSM_TIMING08, offset: 0x50 */
  __IO uint32_t TIMING09;                          /**< TSM_TIMING09, offset: 0x54 */
  __IO uint32_t TIMING10;                          /**< TSM_TIMING10, offset: 0x58 */
  __IO uint32_t TIMING11;                          /**< TSM_TIMING11, offset: 0x5C */
  __IO uint32_t TIMING12;                          /**< TSM_TIMING12, offset: 0x60 */
  __IO uint32_t TIMING13;                          /**< TSM_TIMING13, offset: 0x64 */
  __IO uint32_t TIMING14;                          /**< TSM_TIMING14, offset: 0x68 */
  __IO uint32_t TIMING15;                          /**< TSM_TIMING15, offset: 0x6C */
  __IO uint32_t TIMING16;                          /**< TSM_TIMING16, offset: 0x70 */
  __IO uint32_t TIMING17;                          /**< TSM_TIMING17, offset: 0x74 */
  __IO uint32_t TIMING18;                          /**< TSM_TIMING18, offset: 0x78 */
  __IO uint32_t TIMING19;                          /**< TSM_TIMING19, offset: 0x7C */
  __IO uint32_t TIMING20;                          /**< TSM_TIMING20, offset: 0x80 */
  __IO uint32_t TIMING21;                          /**< TSM_TIMING21, offset: 0x84 */
  __IO uint32_t TIMING22;                          /**< TSM_TIMING22, offset: 0x88 */
  __IO uint32_t TIMING23;                          /**< TSM_TIMING23, offset: 0x8C */
  __IO uint32_t TIMING24;                          /**< TSM_TIMING24, offset: 0x90 */
  __IO uint32_t TIMING25;                          /**< TSM_TIMING25, offset: 0x94 */
  __IO uint32_t TIMING26;                          /**< TSM_TIMING26, offset: 0x98 */
  __IO uint32_t TIMING27;                          /**< TSM_TIMING27, offset: 0x9C */
  __IO uint32_t TIMING28;                          /**< TSM_TIMING28, offset: 0xA0 */
  __IO uint32_t TIMING29;                          /**< TSM_TIMING29, offset: 0xA4 */
  __IO uint32_t TIMING30;                          /**< TSM_TIMING30, offset: 0xA8 */
  __IO uint32_t TIMING31;                          /**< TSM_TIMING31, offset: 0xAC */
  __IO uint32_t TIMING32;                          /**< TSM_TIMING32, offset: 0xB0 */
  __IO uint32_t TIMING33;                          /**< TSM_TIMING33, offset: 0xB4 */
  __IO uint32_t TIMING34;                          /**< TSM_TIMING34, offset: 0xB8 */
  __IO uint32_t TIMING35;                          /**< TSM_TIMING35, offset: 0xBC */
  __IO uint32_t TIMING36;                          /**< TSM_TIMING36, offset: 0xC0 */
  __IO uint32_t TIMING37;                          /**< TSM_TIMING37, offset: 0xC4 */
  __IO uint32_t TIMING38;                          /**< TSM_TIMING38, offset: 0xC8 */
  __IO uint32_t TIMING39;                          /**< TSM_TIMING39, offset: 0xCC */
  __IO uint32_t TIMING40;                          /**< TSM_TIMING40, offset: 0xD0 */
  __IO uint32_t TIMING41;                          /**< TSM_TIMING41, offset: 0xD4 */
  __IO uint32_t TIMING42;                          /**< TSM_TIMING42, offset: 0xD8 */
  __IO uint32_t TIMING43;                          /**< TSM_TIMING43, offset: 0xDC */
  __IO uint32_t TIMING44;                          /**< TSM_TIMING44, offset: 0xE0 */
  __IO uint32_t TIMING45;                          /**< TSM_TIMING45, offset: 0xE4 */
  __IO uint32_t TIMING46;                          /**< TSM_TIMING46, offset: 0xE8 */
  __IO uint32_t TIMING47;                          /**< TSM_TIMING47, offset: 0xEC */
  __IO uint32_t TIMING48;                          /**< TSM_TIMING48, offset: 0xF0 */
  __IO uint32_t TIMING49;                          /**< TSM_TIMING49, offset: 0xF4 */
  __IO uint32_t TIMING50;                          /**< TSM_TIMING50, offset: 0xF8 */
  __IO uint32_t TIMING51;                          /**< TSM_TIMING51, offset: 0xFC */
  __IO uint32_t TIMING52;                          /**< TSM_TIMING52, offset: 0x100 */
  __IO uint32_t TIMING53;                          /**< TSM_TIMING53, offset: 0x104 */
  __IO uint32_t TIMING54;                          /**< TSM_TIMING54, offset: 0x108 */
  __IO uint32_t TIMING55;                          /**< TSM_TIMING55, offset: 0x10C */
  __IO uint32_t TIMING56;                          /**< TSM_TIMING56, offset: 0x110 */
  __IO uint32_t TIMING57;                          /**< TSM_TIMING57, offset: 0x114 */
  __IO uint32_t TIMING58;                          /**< TSM_TIMING58, offset: 0x118 */
} XCVR_TSM_Type;

/* ----------------------------------------------------------------------------
   -- XCVR_TSM Register Masks
   ---------------------------------------------------------------------------- */

/*!
 * @addtogroup XCVR_TSM_Register_Masks XCVR_TSM Register Masks
 * @{
 */

/*! @name CTRL - TRANSCEIVER SEQUENCE MANAGER CONTROL */
#define XCVR_TSM_CTRL_FORCE_TX_EN_MASK           (0x4U)
#define XCVR_TSM_CTRL_FORCE_TX_EN_SHIFT          (2U)
#define XCVR_TSM_CTRL_FORCE_TX_EN(x)             (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_CTRL_FORCE_TX_EN_SHIFT)) & XCVR_TSM_CTRL_FORCE_TX_EN_MASK)
#define XCVR_TSM_CTRL_FORCE_RX_EN_MASK           (0x8U)
#define XCVR_TSM_CTRL_FORCE_RX_EN_SHIFT          (3U)
#define XCVR_TSM_CTRL_FORCE_RX_EN(x)             (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_CTRL_FORCE_RX_EN_SHIFT)) & XCVR_TSM_CTRL_FORCE_RX_EN_MASK)
#define XCVR_TSM_CTRL_PA_RAMP_SEL_MASK           (0x30U)
#define XCVR_TSM_CTRL_PA_RAMP_SEL_SHIFT          (4U)
#define XCVR_TSM_CTRL_PA_RAMP_SEL(x)             (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_CTRL_PA_RAMP_SEL_SHIFT)) & XCVR_TSM_CTRL_PA_RAMP_SEL_MASK)
#define XCVR_TSM_CTRL_DATA_PADDING_EN_MASK       (0xC0U)
#define XCVR_TSM_CTRL_DATA_PADDING_EN_SHIFT      (6U)
#define XCVR_TSM_CTRL_DATA_PADDING_EN(x)         (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_CTRL_DATA_PADDING_EN_SHIFT)) & XCVR_TSM_CTRL_DATA_PADDING_EN_MASK)
#define XCVR_TSM_CTRL_TSM_IRQ0_EN_MASK           (0x100U)
#define XCVR_TSM_CTRL_TSM_IRQ0_EN_SHIFT          (8U)
#define XCVR_TSM_CTRL_TSM_IRQ0_EN(x)             (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_CTRL_TSM_IRQ0_EN_SHIFT)) & XCVR_TSM_CTRL_TSM_IRQ0_EN_MASK)
#define XCVR_TSM_CTRL_TSM_IRQ1_EN_MASK           (0x200U)
#define XCVR_TSM_CTRL_TSM_IRQ1_EN_SHIFT          (9U)
#define XCVR_TSM_CTRL_TSM_IRQ1_EN(x)             (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_CTRL_TSM_IRQ1_EN_SHIFT)) & XCVR_TSM_CTRL_TSM_IRQ1_EN_MASK)
#define XCVR_TSM_CTRL_RAMP_DN_DELAY_MASK         (0xF000U)
#define XCVR_TSM_CTRL_RAMP_DN_DELAY_SHIFT        (12U)
#define XCVR_TSM_CTRL_RAMP_DN_DELAY(x)           (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_CTRL_RAMP_DN_DELAY_SHIFT)) & XCVR_TSM_CTRL_RAMP_DN_DELAY_MASK)
#define XCVR_TSM_CTRL_TX_ABORT_DIS_MASK          (0x10000U)
#define XCVR_TSM_CTRL_TX_ABORT_DIS_SHIFT         (16U)
#define XCVR_TSM_CTRL_TX_ABORT_DIS(x)            (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_CTRL_TX_ABORT_DIS_SHIFT)) & XCVR_TSM_CTRL_TX_ABORT_DIS_MASK)
#define XCVR_TSM_CTRL_RX_ABORT_DIS_MASK          (0x20000U)
#define XCVR_TSM_CTRL_RX_ABORT_DIS_SHIFT         (17U)
#define XCVR_TSM_CTRL_RX_ABORT_DIS(x)            (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_CTRL_RX_ABORT_DIS_SHIFT)) & XCVR_TSM_CTRL_RX_ABORT_DIS_MASK)
#define XCVR_TSM_CTRL_ABORT_ON_CTUNE_MASK        (0x40000U)
#define XCVR_TSM_CTRL_ABORT_ON_CTUNE_SHIFT       (18U)
#define XCVR_TSM_CTRL_ABORT_ON_CTUNE(x)          (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_CTRL_ABORT_ON_CTUNE_SHIFT)) & XCVR_TSM_CTRL_ABORT_ON_CTUNE_MASK)
#define XCVR_TSM_CTRL_ABORT_ON_CYCLE_SLIP_MASK   (0x80000U)
#define XCVR_TSM_CTRL_ABORT_ON_CYCLE_SLIP_SHIFT  (19U)
#define XCVR_TSM_CTRL_ABORT_ON_CYCLE_SLIP(x)     (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_CTRL_ABORT_ON_CYCLE_SLIP_SHIFT)) & XCVR_TSM_CTRL_ABORT_ON_CYCLE_SLIP_MASK)
#define XCVR_TSM_CTRL_ABORT_ON_FREQ_TARG_MASK    (0x100000U)
#define XCVR_TSM_CTRL_ABORT_ON_FREQ_TARG_SHIFT   (20U)
#define XCVR_TSM_CTRL_ABORT_ON_FREQ_TARG(x)      (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_CTRL_ABORT_ON_FREQ_TARG_SHIFT)) & XCVR_TSM_CTRL_ABORT_ON_FREQ_TARG_MASK)
#define XCVR_TSM_CTRL_BKPT_MASK                  (0xFF000000U)
#define XCVR_TSM_CTRL_BKPT_SHIFT                 (24U)
#define XCVR_TSM_CTRL_BKPT(x)                    (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_CTRL_BKPT_SHIFT)) & XCVR_TSM_CTRL_BKPT_MASK)

/*! @name END_OF_SEQ - TSM END OF SEQUENCE */
#define XCVR_TSM_END_OF_SEQ_END_OF_TX_WU_MASK    (0xFFU)
#define XCVR_TSM_END_OF_SEQ_END_OF_TX_WU_SHIFT   (0U)
#define XCVR_TSM_END_OF_SEQ_END_OF_TX_WU(x)      (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_END_OF_SEQ_END_OF_TX_WU_SHIFT)) & XCVR_TSM_END_OF_SEQ_END_OF_TX_WU_MASK)
#define XCVR_TSM_END_OF_SEQ_END_OF_TX_WD_MASK    (0xFF00U)
#define XCVR_TSM_END_OF_SEQ_END_OF_TX_WD_SHIFT   (8U)
#define XCVR_TSM_END_OF_SEQ_END_OF_TX_WD(x)      (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_END_OF_SEQ_END_OF_TX_WD_SHIFT)) & XCVR_TSM_END_OF_SEQ_END_OF_TX_WD_MASK)
#define XCVR_TSM_END_OF_SEQ_END_OF_RX_WU_MASK    (0xFF0000U)
#define XCVR_TSM_END_OF_SEQ_END_OF_RX_WU_SHIFT   (16U)
#define XCVR_TSM_END_OF_SEQ_END_OF_RX_WU(x)      (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_END_OF_SEQ_END_OF_RX_WU_SHIFT)) & XCVR_TSM_END_OF_SEQ_END_OF_RX_WU_MASK)
#define XCVR_TSM_END_OF_SEQ_END_OF_RX_WD_MASK    (0xFF000000U)
#define XCVR_TSM_END_OF_SEQ_END_OF_RX_WD_SHIFT   (24U)
#define XCVR_TSM_END_OF_SEQ_END_OF_RX_WD(x)      (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_END_OF_SEQ_END_OF_RX_WD_SHIFT)) & XCVR_TSM_END_OF_SEQ_END_OF_RX_WD_MASK)

/*! @name OVRD0 - TSM OVERRIDE REGISTER 0 */
#define XCVR_TSM_OVRD0_BB_LDO_HF_EN_OVRD_EN_MASK (0x1U)
#define XCVR_TSM_OVRD0_BB_LDO_HF_EN_OVRD_EN_SHIFT (0U)
#define XCVR_TSM_OVRD0_BB_LDO_HF_EN_OVRD_EN(x)   (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_OVRD0_BB_LDO_HF_EN_OVRD_EN_SHIFT)) & XCVR_TSM_OVRD0_BB_LDO_HF_EN_OVRD_EN_MASK)
#define XCVR_TSM_OVRD0_BB_LDO_HF_EN_OVRD_MASK    (0x2U)
#define XCVR_TSM_OVRD0_BB_LDO_HF_EN_OVRD_SHIFT   (1U)
#define XCVR_TSM_OVRD0_BB_LDO_HF_EN_OVRD(x)      (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_OVRD0_BB_LDO_HF_EN_OVRD_SHIFT)) & XCVR_TSM_OVRD0_BB_LDO_HF_EN_OVRD_MASK)
#define XCVR_TSM_OVRD0_BB_LDO_ADCDAC_EN_OVRD_EN_MASK (0x4U)
#define XCVR_TSM_OVRD0_BB_LDO_ADCDAC_EN_OVRD_EN_SHIFT (2U)
#define XCVR_TSM_OVRD0_BB_LDO_ADCDAC_EN_OVRD_EN(x) (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_OVRD0_BB_LDO_ADCDAC_EN_OVRD_EN_SHIFT)) & XCVR_TSM_OVRD0_BB_LDO_ADCDAC_EN_OVRD_EN_MASK)
#define XCVR_TSM_OVRD0_BB_LDO_ADCDAC_EN_OVRD_MASK (0x8U)
#define XCVR_TSM_OVRD0_BB_LDO_ADCDAC_EN_OVRD_SHIFT (3U)
#define XCVR_TSM_OVRD0_BB_LDO_ADCDAC_EN_OVRD(x)  (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_OVRD0_BB_LDO_ADCDAC_EN_OVRD_SHIFT)) & XCVR_TSM_OVRD0_BB_LDO_ADCDAC_EN_OVRD_MASK)
#define XCVR_TSM_OVRD0_BB_LDO_BBA_EN_OVRD_EN_MASK (0x10U)
#define XCVR_TSM_OVRD0_BB_LDO_BBA_EN_OVRD_EN_SHIFT (4U)
#define XCVR_TSM_OVRD0_BB_LDO_BBA_EN_OVRD_EN(x)  (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_OVRD0_BB_LDO_BBA_EN_OVRD_EN_SHIFT)) & XCVR_TSM_OVRD0_BB_LDO_BBA_EN_OVRD_EN_MASK)
#define XCVR_TSM_OVRD0_BB_LDO_BBA_EN_OVRD_MASK   (0x20U)
#define XCVR_TSM_OVRD0_BB_LDO_BBA_EN_OVRD_SHIFT  (5U)
#define XCVR_TSM_OVRD0_BB_LDO_BBA_EN_OVRD(x)     (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_OVRD0_BB_LDO_BBA_EN_OVRD_SHIFT)) & XCVR_TSM_OVRD0_BB_LDO_BBA_EN_OVRD_MASK)
#define XCVR_TSM_OVRD0_BB_LDO_PD_EN_OVRD_EN_MASK (0x40U)
#define XCVR_TSM_OVRD0_BB_LDO_PD_EN_OVRD_EN_SHIFT (6U)
#define XCVR_TSM_OVRD0_BB_LDO_PD_EN_OVRD_EN(x)   (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_OVRD0_BB_LDO_PD_EN_OVRD_EN_SHIFT)) & XCVR_TSM_OVRD0_BB_LDO_PD_EN_OVRD_EN_MASK)
#define XCVR_TSM_OVRD0_BB_LDO_PD_EN_OVRD_MASK    (0x80U)
#define XCVR_TSM_OVRD0_BB_LDO_PD_EN_OVRD_SHIFT   (7U)
#define XCVR_TSM_OVRD0_BB_LDO_PD_EN_OVRD(x)      (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_OVRD0_BB_LDO_PD_EN_OVRD_SHIFT)) & XCVR_TSM_OVRD0_BB_LDO_PD_EN_OVRD_MASK)
#define XCVR_TSM_OVRD0_BB_LDO_FDBK_EN_OVRD_EN_MASK (0x100U)
#define XCVR_TSM_OVRD0_BB_LDO_FDBK_EN_OVRD_EN_SHIFT (8U)
#define XCVR_TSM_OVRD0_BB_LDO_FDBK_EN_OVRD_EN(x) (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_OVRD0_BB_LDO_FDBK_EN_OVRD_EN_SHIFT)) & XCVR_TSM_OVRD0_BB_LDO_FDBK_EN_OVRD_EN_MASK)
#define XCVR_TSM_OVRD0_BB_LDO_FDBK_EN_OVRD_MASK  (0x200U)
#define XCVR_TSM_OVRD0_BB_LDO_FDBK_EN_OVRD_SHIFT (9U)
#define XCVR_TSM_OVRD0_BB_LDO_FDBK_EN_OVRD(x)    (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_OVRD0_BB_LDO_FDBK_EN_OVRD_SHIFT)) & XCVR_TSM_OVRD0_BB_LDO_FDBK_EN_OVRD_MASK)
#define XCVR_TSM_OVRD0_BB_LDO_VCOLO_EN_OVRD_EN_MASK (0x400U)
#define XCVR_TSM_OVRD0_BB_LDO_VCOLO_EN_OVRD_EN_SHIFT (10U)
#define XCVR_TSM_OVRD0_BB_LDO_VCOLO_EN_OVRD_EN(x) (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_OVRD0_BB_LDO_VCOLO_EN_OVRD_EN_SHIFT)) & XCVR_TSM_OVRD0_BB_LDO_VCOLO_EN_OVRD_EN_MASK)
#define XCVR_TSM_OVRD0_BB_LDO_VCOLO_EN_OVRD_MASK (0x800U)
#define XCVR_TSM_OVRD0_BB_LDO_VCOLO_EN_OVRD_SHIFT (11U)
#define XCVR_TSM_OVRD0_BB_LDO_VCOLO_EN_OVRD(x)   (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_OVRD0_BB_LDO_VCOLO_EN_OVRD_SHIFT)) & XCVR_TSM_OVRD0_BB_LDO_VCOLO_EN_OVRD_MASK)
#define XCVR_TSM_OVRD0_BB_LDO_VTREF_EN_OVRD_EN_MASK (0x1000U)
#define XCVR_TSM_OVRD0_BB_LDO_VTREF_EN_OVRD_EN_SHIFT (12U)
#define XCVR_TSM_OVRD0_BB_LDO_VTREF_EN_OVRD_EN(x) (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_OVRD0_BB_LDO_VTREF_EN_OVRD_EN_SHIFT)) & XCVR_TSM_OVRD0_BB_LDO_VTREF_EN_OVRD_EN_MASK)
#define XCVR_TSM_OVRD0_BB_LDO_VTREF_EN_OVRD_MASK (0x2000U)
#define XCVR_TSM_OVRD0_BB_LDO_VTREF_EN_OVRD_SHIFT (13U)
#define XCVR_TSM_OVRD0_BB_LDO_VTREF_EN_OVRD(x)   (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_OVRD0_BB_LDO_VTREF_EN_OVRD_SHIFT)) & XCVR_TSM_OVRD0_BB_LDO_VTREF_EN_OVRD_MASK)
#define XCVR_TSM_OVRD0_BB_LDO_FDBK_BLEED_EN_OVRD_EN_MASK (0x4000U)
#define XCVR_TSM_OVRD0_BB_LDO_FDBK_BLEED_EN_OVRD_EN_SHIFT (14U)
#define XCVR_TSM_OVRD0_BB_LDO_FDBK_BLEED_EN_OVRD_EN(x) (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_OVRD0_BB_LDO_FDBK_BLEED_EN_OVRD_EN_SHIFT)) & XCVR_TSM_OVRD0_BB_LDO_FDBK_BLEED_EN_OVRD_EN_MASK)
#define XCVR_TSM_OVRD0_BB_LDO_FDBK_BLEED_EN_OVRD_MASK (0x8000U)
#define XCVR_TSM_OVRD0_BB_LDO_FDBK_BLEED_EN_OVRD_SHIFT (15U)
#define XCVR_TSM_OVRD0_BB_LDO_FDBK_BLEED_EN_OVRD(x) (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_OVRD0_BB_LDO_FDBK_BLEED_EN_OVRD_SHIFT)) & XCVR_TSM_OVRD0_BB_LDO_FDBK_BLEED_EN_OVRD_MASK)
#define XCVR_TSM_OVRD0_BB_LDO_VCOLO_BLEED_EN_OVRD_EN_MASK (0x10000U)
#define XCVR_TSM_OVRD0_BB_LDO_VCOLO_BLEED_EN_OVRD_EN_SHIFT (16U)
#define XCVR_TSM_OVRD0_BB_LDO_VCOLO_BLEED_EN_OVRD_EN(x) (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_OVRD0_BB_LDO_VCOLO_BLEED_EN_OVRD_EN_SHIFT)) & XCVR_TSM_OVRD0_BB_LDO_VCOLO_BLEED_EN_OVRD_EN_MASK)
#define XCVR_TSM_OVRD0_BB_LDO_VCOLO_BLEED_EN_OVRD_MASK (0x20000U)
#define XCVR_TSM_OVRD0_BB_LDO_VCOLO_BLEED_EN_OVRD_SHIFT (17U)
#define XCVR_TSM_OVRD0_BB_LDO_VCOLO_BLEED_EN_OVRD(x) (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_OVRD0_BB_LDO_VCOLO_BLEED_EN_OVRD_SHIFT)) & XCVR_TSM_OVRD0_BB_LDO_VCOLO_BLEED_EN_OVRD_MASK)
#define XCVR_TSM_OVRD0_BB_LDO_VCOLO_FASTCHARGE_EN_OVRD_EN_MASK (0x40000U)
#define XCVR_TSM_OVRD0_BB_LDO_VCOLO_FASTCHARGE_EN_OVRD_EN_SHIFT (18U)
#define XCVR_TSM_OVRD0_BB_LDO_VCOLO_FASTCHARGE_EN_OVRD_EN(x) (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_OVRD0_BB_LDO_VCOLO_FASTCHARGE_EN_OVRD_EN_SHIFT)) & XCVR_TSM_OVRD0_BB_LDO_VCOLO_FASTCHARGE_EN_OVRD_EN_MASK)
#define XCVR_TSM_OVRD0_BB_LDO_VCOLO_FASTCHARGE_EN_OVRD_MASK (0x80000U)
#define XCVR_TSM_OVRD0_BB_LDO_VCOLO_FASTCHARGE_EN_OVRD_SHIFT (19U)
#define XCVR_TSM_OVRD0_BB_LDO_VCOLO_FASTCHARGE_EN_OVRD(x) (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_OVRD0_BB_LDO_VCOLO_FASTCHARGE_EN_OVRD_SHIFT)) & XCVR_TSM_OVRD0_BB_LDO_VCOLO_FASTCHARGE_EN_OVRD_MASK)
#define XCVR_TSM_OVRD0_BB_XTAL_PLL_REF_CLK_EN_OVRD_EN_MASK (0x100000U)
#define XCVR_TSM_OVRD0_BB_XTAL_PLL_REF_CLK_EN_OVRD_EN_SHIFT (20U)
#define XCVR_TSM_OVRD0_BB_XTAL_PLL_REF_CLK_EN_OVRD_EN(x) (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_OVRD0_BB_XTAL_PLL_REF_CLK_EN_OVRD_EN_SHIFT)) & XCVR_TSM_OVRD0_BB_XTAL_PLL_REF_CLK_EN_OVRD_EN_MASK)
#define XCVR_TSM_OVRD0_BB_XTAL_PLL_REF_CLK_EN_OVRD_MASK (0x200000U)
#define XCVR_TSM_OVRD0_BB_XTAL_PLL_REF_CLK_EN_OVRD_SHIFT (21U)
#define XCVR_TSM_OVRD0_BB_XTAL_PLL_REF_CLK_EN_OVRD(x) (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_OVRD0_BB_XTAL_PLL_REF_CLK_EN_OVRD_SHIFT)) & XCVR_TSM_OVRD0_BB_XTAL_PLL_REF_CLK_EN_OVRD_MASK)
#define XCVR_TSM_OVRD0_BB_XTAL_DAC_REF_CLK_EN_OVRD_EN_MASK (0x400000U)
#define XCVR_TSM_OVRD0_BB_XTAL_DAC_REF_CLK_EN_OVRD_EN_SHIFT (22U)
#define XCVR_TSM_OVRD0_BB_XTAL_DAC_REF_CLK_EN_OVRD_EN(x) (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_OVRD0_BB_XTAL_DAC_REF_CLK_EN_OVRD_EN_SHIFT)) & XCVR_TSM_OVRD0_BB_XTAL_DAC_REF_CLK_EN_OVRD_EN_MASK)
#define XCVR_TSM_OVRD0_BB_XTAL_DAC_REF_CLK_EN_OVRD_MASK (0x800000U)
#define XCVR_TSM_OVRD0_BB_XTAL_DAC_REF_CLK_EN_OVRD_SHIFT (23U)
#define XCVR_TSM_OVRD0_BB_XTAL_DAC_REF_CLK_EN_OVRD(x) (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_OVRD0_BB_XTAL_DAC_REF_CLK_EN_OVRD_SHIFT)) & XCVR_TSM_OVRD0_BB_XTAL_DAC_REF_CLK_EN_OVRD_MASK)
#define XCVR_TSM_OVRD0_BB_XTAL_AUXPLL_REF_CLK_EN_OVRD_EN_MASK (0x1000000U)
#define XCVR_TSM_OVRD0_BB_XTAL_AUXPLL_REF_CLK_EN_OVRD_EN_SHIFT (24U)
#define XCVR_TSM_OVRD0_BB_XTAL_AUXPLL_REF_CLK_EN_OVRD_EN(x) (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_OVRD0_BB_XTAL_AUXPLL_REF_CLK_EN_OVRD_EN_SHIFT)) & XCVR_TSM_OVRD0_BB_XTAL_AUXPLL_REF_CLK_EN_OVRD_EN_MASK)
#define XCVR_TSM_OVRD0_BB_XTAL_AUXPLL_REF_CLK_EN_OVRD_MASK (0x2000000U)
#define XCVR_TSM_OVRD0_BB_XTAL_AUXPLL_REF_CLK_EN_OVRD_SHIFT (25U)
#define XCVR_TSM_OVRD0_BB_XTAL_AUXPLL_REF_CLK_EN_OVRD(x) (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_OVRD0_BB_XTAL_AUXPLL_REF_CLK_EN_OVRD_SHIFT)) & XCVR_TSM_OVRD0_BB_XTAL_AUXPLL_REF_CLK_EN_OVRD_MASK)
#define XCVR_TSM_OVRD0_SY_VCO_AUTOTUNE_EN_OVRD_EN_MASK (0x4000000U)
#define XCVR_TSM_OVRD0_SY_VCO_AUTOTUNE_EN_OVRD_EN_SHIFT (26U)
#define XCVR_TSM_OVRD0_SY_VCO_AUTOTUNE_EN_OVRD_EN(x) (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_OVRD0_SY_VCO_AUTOTUNE_EN_OVRD_EN_SHIFT)) & XCVR_TSM_OVRD0_SY_VCO_AUTOTUNE_EN_OVRD_EN_MASK)
#define XCVR_TSM_OVRD0_SY_VCO_AUTOTUNE_EN_OVRD_MASK (0x8000000U)
#define XCVR_TSM_OVRD0_SY_VCO_AUTOTUNE_EN_OVRD_SHIFT (27U)
#define XCVR_TSM_OVRD0_SY_VCO_AUTOTUNE_EN_OVRD(x) (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_OVRD0_SY_VCO_AUTOTUNE_EN_OVRD_SHIFT)) & XCVR_TSM_OVRD0_SY_VCO_AUTOTUNE_EN_OVRD_MASK)
#define XCVR_TSM_OVRD0_SY_PD_CYCLE_SLIP_LD_EN_OVRD_EN_MASK (0x10000000U)
#define XCVR_TSM_OVRD0_SY_PD_CYCLE_SLIP_LD_EN_OVRD_EN_SHIFT (28U)
#define XCVR_TSM_OVRD0_SY_PD_CYCLE_SLIP_LD_EN_OVRD_EN(x) (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_OVRD0_SY_PD_CYCLE_SLIP_LD_EN_OVRD_EN_SHIFT)) & XCVR_TSM_OVRD0_SY_PD_CYCLE_SLIP_LD_EN_OVRD_EN_MASK)
#define XCVR_TSM_OVRD0_SY_PD_CYCLE_SLIP_LD_EN_OVRD_MASK (0x20000000U)
#define XCVR_TSM_OVRD0_SY_PD_CYCLE_SLIP_LD_EN_OVRD_SHIFT (29U)
#define XCVR_TSM_OVRD0_SY_PD_CYCLE_SLIP_LD_EN_OVRD(x) (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_OVRD0_SY_PD_CYCLE_SLIP_LD_EN_OVRD_SHIFT)) & XCVR_TSM_OVRD0_SY_PD_CYCLE_SLIP_LD_EN_OVRD_MASK)
#define XCVR_TSM_OVRD0_SY_VCO_EN_OVRD_EN_MASK    (0x40000000U)
#define XCVR_TSM_OVRD0_SY_VCO_EN_OVRD_EN_SHIFT   (30U)
#define XCVR_TSM_OVRD0_SY_VCO_EN_OVRD_EN(x)      (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_OVRD0_SY_VCO_EN_OVRD_EN_SHIFT)) & XCVR_TSM_OVRD0_SY_VCO_EN_OVRD_EN_MASK)
#define XCVR_TSM_OVRD0_SY_VCO_EN_OVRD_MASK       (0x80000000U)
#define XCVR_TSM_OVRD0_SY_VCO_EN_OVRD_SHIFT      (31U)
#define XCVR_TSM_OVRD0_SY_VCO_EN_OVRD(x)         (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_OVRD0_SY_VCO_EN_OVRD_SHIFT)) & XCVR_TSM_OVRD0_SY_VCO_EN_OVRD_MASK)

/*! @name OVRD1 - TSM OVERRIDE REGISTER 1 */
#define XCVR_TSM_OVRD1_SY_LO_RX_BUF_EN_OVRD_EN_MASK (0x1U)
#define XCVR_TSM_OVRD1_SY_LO_RX_BUF_EN_OVRD_EN_SHIFT (0U)
#define XCVR_TSM_OVRD1_SY_LO_RX_BUF_EN_OVRD_EN(x) (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_OVRD1_SY_LO_RX_BUF_EN_OVRD_EN_SHIFT)) & XCVR_TSM_OVRD1_SY_LO_RX_BUF_EN_OVRD_EN_MASK)
#define XCVR_TSM_OVRD1_SY_LO_RX_BUF_EN_OVRD_MASK (0x2U)
#define XCVR_TSM_OVRD1_SY_LO_RX_BUF_EN_OVRD_SHIFT (1U)
#define XCVR_TSM_OVRD1_SY_LO_RX_BUF_EN_OVRD(x)   (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_OVRD1_SY_LO_RX_BUF_EN_OVRD_SHIFT)) & XCVR_TSM_OVRD1_SY_LO_RX_BUF_EN_OVRD_MASK)
#define XCVR_TSM_OVRD1_SY_LO_TX_BUF_EN_OVRD_EN_MASK (0x4U)
#define XCVR_TSM_OVRD1_SY_LO_TX_BUF_EN_OVRD_EN_SHIFT (2U)
#define XCVR_TSM_OVRD1_SY_LO_TX_BUF_EN_OVRD_EN(x) (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_OVRD1_SY_LO_TX_BUF_EN_OVRD_EN_SHIFT)) & XCVR_TSM_OVRD1_SY_LO_TX_BUF_EN_OVRD_EN_MASK)
#define XCVR_TSM_OVRD1_SY_LO_TX_BUF_EN_OVRD_MASK (0x8U)
#define XCVR_TSM_OVRD1_SY_LO_TX_BUF_EN_OVRD_SHIFT (3U)
#define XCVR_TSM_OVRD1_SY_LO_TX_BUF_EN_OVRD(x)   (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_OVRD1_SY_LO_TX_BUF_EN_OVRD_SHIFT)) & XCVR_TSM_OVRD1_SY_LO_TX_BUF_EN_OVRD_MASK)
#define XCVR_TSM_OVRD1_SY_DIVN_EN_OVRD_EN_MASK   (0x10U)
#define XCVR_TSM_OVRD1_SY_DIVN_EN_OVRD_EN_SHIFT  (4U)
#define XCVR_TSM_OVRD1_SY_DIVN_EN_OVRD_EN(x)     (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_OVRD1_SY_DIVN_EN_OVRD_EN_SHIFT)) & XCVR_TSM_OVRD1_SY_DIVN_EN_OVRD_EN_MASK)
#define XCVR_TSM_OVRD1_SY_DIVN_EN_OVRD_MASK      (0x20U)
#define XCVR_TSM_OVRD1_SY_DIVN_EN_OVRD_SHIFT     (5U)
#define XCVR_TSM_OVRD1_SY_DIVN_EN_OVRD(x)        (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_OVRD1_SY_DIVN_EN_OVRD_SHIFT)) & XCVR_TSM_OVRD1_SY_DIVN_EN_OVRD_MASK)
#define XCVR_TSM_OVRD1_SY_PD_FILTER_CHARGE_EN_OVRD_EN_MASK (0x40U)
#define XCVR_TSM_OVRD1_SY_PD_FILTER_CHARGE_EN_OVRD_EN_SHIFT (6U)
#define XCVR_TSM_OVRD1_SY_PD_FILTER_CHARGE_EN_OVRD_EN(x) (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_OVRD1_SY_PD_FILTER_CHARGE_EN_OVRD_EN_SHIFT)) & XCVR_TSM_OVRD1_SY_PD_FILTER_CHARGE_EN_OVRD_EN_MASK)
#define XCVR_TSM_OVRD1_SY_PD_FILTER_CHARGE_EN_OVRD_MASK (0x80U)
#define XCVR_TSM_OVRD1_SY_PD_FILTER_CHARGE_EN_OVRD_SHIFT (7U)
#define XCVR_TSM_OVRD1_SY_PD_FILTER_CHARGE_EN_OVRD(x) (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_OVRD1_SY_PD_FILTER_CHARGE_EN_OVRD_SHIFT)) & XCVR_TSM_OVRD1_SY_PD_FILTER_CHARGE_EN_OVRD_MASK)
#define XCVR_TSM_OVRD1_SY_PD_EN_OVRD_EN_MASK     (0x100U)
#define XCVR_TSM_OVRD1_SY_PD_EN_OVRD_EN_SHIFT    (8U)
#define XCVR_TSM_OVRD1_SY_PD_EN_OVRD_EN(x)       (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_OVRD1_SY_PD_EN_OVRD_EN_SHIFT)) & XCVR_TSM_OVRD1_SY_PD_EN_OVRD_EN_MASK)
#define XCVR_TSM_OVRD1_SY_PD_EN_OVRD_MASK        (0x200U)
#define XCVR_TSM_OVRD1_SY_PD_EN_OVRD_SHIFT       (9U)
#define XCVR_TSM_OVRD1_SY_PD_EN_OVRD(x)          (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_OVRD1_SY_PD_EN_OVRD_SHIFT)) & XCVR_TSM_OVRD1_SY_PD_EN_OVRD_MASK)
#define XCVR_TSM_OVRD1_SY_LO_DIVN_EN_OVRD_EN_MASK (0x400U)
#define XCVR_TSM_OVRD1_SY_LO_DIVN_EN_OVRD_EN_SHIFT (10U)
#define XCVR_TSM_OVRD1_SY_LO_DIVN_EN_OVRD_EN(x)  (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_OVRD1_SY_LO_DIVN_EN_OVRD_EN_SHIFT)) & XCVR_TSM_OVRD1_SY_LO_DIVN_EN_OVRD_EN_MASK)
#define XCVR_TSM_OVRD1_SY_LO_DIVN_EN_OVRD_MASK   (0x800U)
#define XCVR_TSM_OVRD1_SY_LO_DIVN_EN_OVRD_SHIFT  (11U)
#define XCVR_TSM_OVRD1_SY_LO_DIVN_EN_OVRD(x)     (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_OVRD1_SY_LO_DIVN_EN_OVRD_SHIFT)) & XCVR_TSM_OVRD1_SY_LO_DIVN_EN_OVRD_MASK)
#define XCVR_TSM_OVRD1_SY_LO_RX_EN_OVRD_EN_MASK  (0x1000U)
#define XCVR_TSM_OVRD1_SY_LO_RX_EN_OVRD_EN_SHIFT (12U)
#define XCVR_TSM_OVRD1_SY_LO_RX_EN_OVRD_EN(x)    (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_OVRD1_SY_LO_RX_EN_OVRD_EN_SHIFT)) & XCVR_TSM_OVRD1_SY_LO_RX_EN_OVRD_EN_MASK)
#define XCVR_TSM_OVRD1_SY_LO_RX_EN_OVRD_MASK     (0x2000U)
#define XCVR_TSM_OVRD1_SY_LO_RX_EN_OVRD_SHIFT    (13U)
#define XCVR_TSM_OVRD1_SY_LO_RX_EN_OVRD(x)       (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_OVRD1_SY_LO_RX_EN_OVRD_SHIFT)) & XCVR_TSM_OVRD1_SY_LO_RX_EN_OVRD_MASK)
#define XCVR_TSM_OVRD1_SY_LO_TX_EN_OVRD_EN_MASK  (0x4000U)
#define XCVR_TSM_OVRD1_SY_LO_TX_EN_OVRD_EN_SHIFT (14U)
#define XCVR_TSM_OVRD1_SY_LO_TX_EN_OVRD_EN(x)    (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_OVRD1_SY_LO_TX_EN_OVRD_EN_SHIFT)) & XCVR_TSM_OVRD1_SY_LO_TX_EN_OVRD_EN_MASK)
#define XCVR_TSM_OVRD1_SY_LO_TX_EN_OVRD_MASK     (0x8000U)
#define XCVR_TSM_OVRD1_SY_LO_TX_EN_OVRD_SHIFT    (15U)
#define XCVR_TSM_OVRD1_SY_LO_TX_EN_OVRD(x)       (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_OVRD1_SY_LO_TX_EN_OVRD_SHIFT)) & XCVR_TSM_OVRD1_SY_LO_TX_EN_OVRD_MASK)
#define XCVR_TSM_OVRD1_SY_DIVN_CAL_EN_OVRD_EN_MASK (0x10000U)
#define XCVR_TSM_OVRD1_SY_DIVN_CAL_EN_OVRD_EN_SHIFT (16U)
#define XCVR_TSM_OVRD1_SY_DIVN_CAL_EN_OVRD_EN(x) (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_OVRD1_SY_DIVN_CAL_EN_OVRD_EN_SHIFT)) & XCVR_TSM_OVRD1_SY_DIVN_CAL_EN_OVRD_EN_MASK)
#define XCVR_TSM_OVRD1_SY_DIVN_CAL_EN_OVRD_MASK  (0x20000U)
#define XCVR_TSM_OVRD1_SY_DIVN_CAL_EN_OVRD_SHIFT (17U)
#define XCVR_TSM_OVRD1_SY_DIVN_CAL_EN_OVRD(x)    (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_OVRD1_SY_DIVN_CAL_EN_OVRD_SHIFT)) & XCVR_TSM_OVRD1_SY_DIVN_CAL_EN_OVRD_MASK)
#define XCVR_TSM_OVRD1_RX_MIXER_EN_OVRD_EN_MASK  (0x40000U)
#define XCVR_TSM_OVRD1_RX_MIXER_EN_OVRD_EN_SHIFT (18U)
#define XCVR_TSM_OVRD1_RX_MIXER_EN_OVRD_EN(x)    (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_OVRD1_RX_MIXER_EN_OVRD_EN_SHIFT)) & XCVR_TSM_OVRD1_RX_MIXER_EN_OVRD_EN_MASK)
#define XCVR_TSM_OVRD1_RX_MIXER_EN_OVRD_MASK     (0x80000U)
#define XCVR_TSM_OVRD1_RX_MIXER_EN_OVRD_SHIFT    (19U)
#define XCVR_TSM_OVRD1_RX_MIXER_EN_OVRD(x)       (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_OVRD1_RX_MIXER_EN_OVRD_SHIFT)) & XCVR_TSM_OVRD1_RX_MIXER_EN_OVRD_MASK)
#define XCVR_TSM_OVRD1_TX_PA_EN_OVRD_EN_MASK     (0x100000U)
#define XCVR_TSM_OVRD1_TX_PA_EN_OVRD_EN_SHIFT    (20U)
#define XCVR_TSM_OVRD1_TX_PA_EN_OVRD_EN(x)       (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_OVRD1_TX_PA_EN_OVRD_EN_SHIFT)) & XCVR_TSM_OVRD1_TX_PA_EN_OVRD_EN_MASK)
#define XCVR_TSM_OVRD1_TX_PA_EN_OVRD_MASK        (0x200000U)
#define XCVR_TSM_OVRD1_TX_PA_EN_OVRD_SHIFT       (21U)
#define XCVR_TSM_OVRD1_TX_PA_EN_OVRD(x)          (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_OVRD1_TX_PA_EN_OVRD_SHIFT)) & XCVR_TSM_OVRD1_TX_PA_EN_OVRD_MASK)
#define XCVR_TSM_OVRD1_RX_ADC_I_EN_OVRD_EN_MASK  (0x400000U)
#define XCVR_TSM_OVRD1_RX_ADC_I_EN_OVRD_EN_SHIFT (22U)
#define XCVR_TSM_OVRD1_RX_ADC_I_EN_OVRD_EN(x)    (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_OVRD1_RX_ADC_I_EN_OVRD_EN_SHIFT)) & XCVR_TSM_OVRD1_RX_ADC_I_EN_OVRD_EN_MASK)
#define XCVR_TSM_OVRD1_RX_ADC_I_EN_OVRD_MASK     (0x800000U)
#define XCVR_TSM_OVRD1_RX_ADC_I_EN_OVRD_SHIFT    (23U)
#define XCVR_TSM_OVRD1_RX_ADC_I_EN_OVRD(x)       (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_OVRD1_RX_ADC_I_EN_OVRD_SHIFT)) & XCVR_TSM_OVRD1_RX_ADC_I_EN_OVRD_MASK)
#define XCVR_TSM_OVRD1_RX_ADC_Q_EN_OVRD_EN_MASK  (0x1000000U)
#define XCVR_TSM_OVRD1_RX_ADC_Q_EN_OVRD_EN_SHIFT (24U)
#define XCVR_TSM_OVRD1_RX_ADC_Q_EN_OVRD_EN(x)    (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_OVRD1_RX_ADC_Q_EN_OVRD_EN_SHIFT)) & XCVR_TSM_OVRD1_RX_ADC_Q_EN_OVRD_EN_MASK)
#define XCVR_TSM_OVRD1_RX_ADC_Q_EN_OVRD_MASK     (0x2000000U)
#define XCVR_TSM_OVRD1_RX_ADC_Q_EN_OVRD_SHIFT    (25U)
#define XCVR_TSM_OVRD1_RX_ADC_Q_EN_OVRD(x)       (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_OVRD1_RX_ADC_Q_EN_OVRD_SHIFT)) & XCVR_TSM_OVRD1_RX_ADC_Q_EN_OVRD_MASK)
#define XCVR_TSM_OVRD1_RX_ADC_RESET_EN_OVRD_EN_MASK (0x4000000U)
#define XCVR_TSM_OVRD1_RX_ADC_RESET_EN_OVRD_EN_SHIFT (26U)
#define XCVR_TSM_OVRD1_RX_ADC_RESET_EN_OVRD_EN(x) (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_OVRD1_RX_ADC_RESET_EN_OVRD_EN_SHIFT)) & XCVR_TSM_OVRD1_RX_ADC_RESET_EN_OVRD_EN_MASK)
#define XCVR_TSM_OVRD1_RX_ADC_RESET_EN_OVRD_MASK (0x8000000U)
#define XCVR_TSM_OVRD1_RX_ADC_RESET_EN_OVRD_SHIFT (27U)
#define XCVR_TSM_OVRD1_RX_ADC_RESET_EN_OVRD(x)   (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_OVRD1_RX_ADC_RESET_EN_OVRD_SHIFT)) & XCVR_TSM_OVRD1_RX_ADC_RESET_EN_OVRD_MASK)
#define XCVR_TSM_OVRD1_RX_BBA_I_EN_OVRD_EN_MASK  (0x10000000U)
#define XCVR_TSM_OVRD1_RX_BBA_I_EN_OVRD_EN_SHIFT (28U)
#define XCVR_TSM_OVRD1_RX_BBA_I_EN_OVRD_EN(x)    (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_OVRD1_RX_BBA_I_EN_OVRD_EN_SHIFT)) & XCVR_TSM_OVRD1_RX_BBA_I_EN_OVRD_EN_MASK)
#define XCVR_TSM_OVRD1_RX_BBA_I_EN_OVRD_MASK     (0x20000000U)
#define XCVR_TSM_OVRD1_RX_BBA_I_EN_OVRD_SHIFT    (29U)
#define XCVR_TSM_OVRD1_RX_BBA_I_EN_OVRD(x)       (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_OVRD1_RX_BBA_I_EN_OVRD_SHIFT)) & XCVR_TSM_OVRD1_RX_BBA_I_EN_OVRD_MASK)
#define XCVR_TSM_OVRD1_RX_BBA_Q_EN_OVRD_EN_MASK  (0x40000000U)
#define XCVR_TSM_OVRD1_RX_BBA_Q_EN_OVRD_EN_SHIFT (30U)
#define XCVR_TSM_OVRD1_RX_BBA_Q_EN_OVRD_EN(x)    (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_OVRD1_RX_BBA_Q_EN_OVRD_EN_SHIFT)) & XCVR_TSM_OVRD1_RX_BBA_Q_EN_OVRD_EN_MASK)
#define XCVR_TSM_OVRD1_RX_BBA_Q_EN_OVRD_MASK     (0x80000000U)
#define XCVR_TSM_OVRD1_RX_BBA_Q_EN_OVRD_SHIFT    (31U)
#define XCVR_TSM_OVRD1_RX_BBA_Q_EN_OVRD(x)       (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_OVRD1_RX_BBA_Q_EN_OVRD_SHIFT)) & XCVR_TSM_OVRD1_RX_BBA_Q_EN_OVRD_MASK)

/*! @name OVRD2 - TSM OVERRIDE REGISTER 2 */
#define XCVR_TSM_OVRD2_RX_BBA_PDET_EN_OVRD_EN_MASK (0x1U)
#define XCVR_TSM_OVRD2_RX_BBA_PDET_EN_OVRD_EN_SHIFT (0U)
#define XCVR_TSM_OVRD2_RX_BBA_PDET_EN_OVRD_EN(x) (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_OVRD2_RX_BBA_PDET_EN_OVRD_EN_SHIFT)) & XCVR_TSM_OVRD2_RX_BBA_PDET_EN_OVRD_EN_MASK)
#define XCVR_TSM_OVRD2_RX_BBA_PDET_EN_OVRD_MASK  (0x2U)
#define XCVR_TSM_OVRD2_RX_BBA_PDET_EN_OVRD_SHIFT (1U)
#define XCVR_TSM_OVRD2_RX_BBA_PDET_EN_OVRD(x)    (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_OVRD2_RX_BBA_PDET_EN_OVRD_SHIFT)) & XCVR_TSM_OVRD2_RX_BBA_PDET_EN_OVRD_MASK)
#define XCVR_TSM_OVRD2_RX_BBA_DCOC_EN_OVRD_EN_MASK (0x4U)
#define XCVR_TSM_OVRD2_RX_BBA_DCOC_EN_OVRD_EN_SHIFT (2U)
#define XCVR_TSM_OVRD2_RX_BBA_DCOC_EN_OVRD_EN(x) (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_OVRD2_RX_BBA_DCOC_EN_OVRD_EN_SHIFT)) & XCVR_TSM_OVRD2_RX_BBA_DCOC_EN_OVRD_EN_MASK)
#define XCVR_TSM_OVRD2_RX_BBA_DCOC_EN_OVRD_MASK  (0x8U)
#define XCVR_TSM_OVRD2_RX_BBA_DCOC_EN_OVRD_SHIFT (3U)
#define XCVR_TSM_OVRD2_RX_BBA_DCOC_EN_OVRD(x)    (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_OVRD2_RX_BBA_DCOC_EN_OVRD_SHIFT)) & XCVR_TSM_OVRD2_RX_BBA_DCOC_EN_OVRD_MASK)
#define XCVR_TSM_OVRD2_RX_LNA_EN_OVRD_EN_MASK    (0x10U)
#define XCVR_TSM_OVRD2_RX_LNA_EN_OVRD_EN_SHIFT   (4U)
#define XCVR_TSM_OVRD2_RX_LNA_EN_OVRD_EN(x)      (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_OVRD2_RX_LNA_EN_OVRD_EN_SHIFT)) & XCVR_TSM_OVRD2_RX_LNA_EN_OVRD_EN_MASK)
#define XCVR_TSM_OVRD2_RX_LNA_EN_OVRD_MASK       (0x20U)
#define XCVR_TSM_OVRD2_RX_LNA_EN_OVRD_SHIFT      (5U)
#define XCVR_TSM_OVRD2_RX_LNA_EN_OVRD(x)         (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_OVRD2_RX_LNA_EN_OVRD_SHIFT)) & XCVR_TSM_OVRD2_RX_LNA_EN_OVRD_MASK)
#define XCVR_TSM_OVRD2_RX_TZA_I_EN_OVRD_EN_MASK  (0x40U)
#define XCVR_TSM_OVRD2_RX_TZA_I_EN_OVRD_EN_SHIFT (6U)
#define XCVR_TSM_OVRD2_RX_TZA_I_EN_OVRD_EN(x)    (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_OVRD2_RX_TZA_I_EN_OVRD_EN_SHIFT)) & XCVR_TSM_OVRD2_RX_TZA_I_EN_OVRD_EN_MASK)
#define XCVR_TSM_OVRD2_RX_TZA_I_EN_OVRD_MASK     (0x80U)
#define XCVR_TSM_OVRD2_RX_TZA_I_EN_OVRD_SHIFT    (7U)
#define XCVR_TSM_OVRD2_RX_TZA_I_EN_OVRD(x)       (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_OVRD2_RX_TZA_I_EN_OVRD_SHIFT)) & XCVR_TSM_OVRD2_RX_TZA_I_EN_OVRD_MASK)
#define XCVR_TSM_OVRD2_RX_TZA_Q_EN_OVRD_EN_MASK  (0x100U)
#define XCVR_TSM_OVRD2_RX_TZA_Q_EN_OVRD_EN_SHIFT (8U)
#define XCVR_TSM_OVRD2_RX_TZA_Q_EN_OVRD_EN(x)    (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_OVRD2_RX_TZA_Q_EN_OVRD_EN_SHIFT)) & XCVR_TSM_OVRD2_RX_TZA_Q_EN_OVRD_EN_MASK)
#define XCVR_TSM_OVRD2_RX_TZA_Q_EN_OVRD_MASK     (0x200U)
#define XCVR_TSM_OVRD2_RX_TZA_Q_EN_OVRD_SHIFT    (9U)
#define XCVR_TSM_OVRD2_RX_TZA_Q_EN_OVRD(x)       (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_OVRD2_RX_TZA_Q_EN_OVRD_SHIFT)) & XCVR_TSM_OVRD2_RX_TZA_Q_EN_OVRD_MASK)
#define XCVR_TSM_OVRD2_RX_TZA_PDET_EN_OVRD_EN_MASK (0x400U)
#define XCVR_TSM_OVRD2_RX_TZA_PDET_EN_OVRD_EN_SHIFT (10U)
#define XCVR_TSM_OVRD2_RX_TZA_PDET_EN_OVRD_EN(x) (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_OVRD2_RX_TZA_PDET_EN_OVRD_EN_SHIFT)) & XCVR_TSM_OVRD2_RX_TZA_PDET_EN_OVRD_EN_MASK)
#define XCVR_TSM_OVRD2_RX_TZA_PDET_EN_OVRD_MASK  (0x800U)
#define XCVR_TSM_OVRD2_RX_TZA_PDET_EN_OVRD_SHIFT (11U)
#define XCVR_TSM_OVRD2_RX_TZA_PDET_EN_OVRD(x)    (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_OVRD2_RX_TZA_PDET_EN_OVRD_SHIFT)) & XCVR_TSM_OVRD2_RX_TZA_PDET_EN_OVRD_MASK)
#define XCVR_TSM_OVRD2_RX_TZA_DCOC_EN_OVRD_EN_MASK (0x1000U)
#define XCVR_TSM_OVRD2_RX_TZA_DCOC_EN_OVRD_EN_SHIFT (12U)
#define XCVR_TSM_OVRD2_RX_TZA_DCOC_EN_OVRD_EN(x) (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_OVRD2_RX_TZA_DCOC_EN_OVRD_EN_SHIFT)) & XCVR_TSM_OVRD2_RX_TZA_DCOC_EN_OVRD_EN_MASK)
#define XCVR_TSM_OVRD2_RX_TZA_DCOC_EN_OVRD_MASK  (0x2000U)
#define XCVR_TSM_OVRD2_RX_TZA_DCOC_EN_OVRD_SHIFT (13U)
#define XCVR_TSM_OVRD2_RX_TZA_DCOC_EN_OVRD(x)    (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_OVRD2_RX_TZA_DCOC_EN_OVRD_SHIFT)) & XCVR_TSM_OVRD2_RX_TZA_DCOC_EN_OVRD_MASK)
#define XCVR_TSM_OVRD2_PLL_DIG_EN_OVRD_EN_MASK   (0x4000U)
#define XCVR_TSM_OVRD2_PLL_DIG_EN_OVRD_EN_SHIFT  (14U)
#define XCVR_TSM_OVRD2_PLL_DIG_EN_OVRD_EN(x)     (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_OVRD2_PLL_DIG_EN_OVRD_EN_SHIFT)) & XCVR_TSM_OVRD2_PLL_DIG_EN_OVRD_EN_MASK)
#define XCVR_TSM_OVRD2_PLL_DIG_EN_OVRD_MASK      (0x8000U)
#define XCVR_TSM_OVRD2_PLL_DIG_EN_OVRD_SHIFT     (15U)
#define XCVR_TSM_OVRD2_PLL_DIG_EN_OVRD(x)        (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_OVRD2_PLL_DIG_EN_OVRD_SHIFT)) & XCVR_TSM_OVRD2_PLL_DIG_EN_OVRD_MASK)
#define XCVR_TSM_OVRD2_TX_DIG_EN_OVRD_EN_MASK    (0x10000U)
#define XCVR_TSM_OVRD2_TX_DIG_EN_OVRD_EN_SHIFT   (16U)
#define XCVR_TSM_OVRD2_TX_DIG_EN_OVRD_EN(x)      (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_OVRD2_TX_DIG_EN_OVRD_EN_SHIFT)) & XCVR_TSM_OVRD2_TX_DIG_EN_OVRD_EN_MASK)
#define XCVR_TSM_OVRD2_TX_DIG_EN_OVRD_MASK       (0x20000U)
#define XCVR_TSM_OVRD2_TX_DIG_EN_OVRD_SHIFT      (17U)
#define XCVR_TSM_OVRD2_TX_DIG_EN_OVRD(x)         (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_OVRD2_TX_DIG_EN_OVRD_SHIFT)) & XCVR_TSM_OVRD2_TX_DIG_EN_OVRD_MASK)
#define XCVR_TSM_OVRD2_RX_DIG_EN_OVRD_EN_MASK    (0x40000U)
#define XCVR_TSM_OVRD2_RX_DIG_EN_OVRD_EN_SHIFT   (18U)
#define XCVR_TSM_OVRD2_RX_DIG_EN_OVRD_EN(x)      (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_OVRD2_RX_DIG_EN_OVRD_EN_SHIFT)) & XCVR_TSM_OVRD2_RX_DIG_EN_OVRD_EN_MASK)
#define XCVR_TSM_OVRD2_RX_DIG_EN_OVRD_MASK       (0x80000U)
#define XCVR_TSM_OVRD2_RX_DIG_EN_OVRD_SHIFT      (19U)
#define XCVR_TSM_OVRD2_RX_DIG_EN_OVRD(x)         (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_OVRD2_RX_DIG_EN_OVRD_SHIFT)) & XCVR_TSM_OVRD2_RX_DIG_EN_OVRD_MASK)
#define XCVR_TSM_OVRD2_RX_INIT_OVRD_EN_MASK      (0x100000U)
#define XCVR_TSM_OVRD2_RX_INIT_OVRD_EN_SHIFT     (20U)
#define XCVR_TSM_OVRD2_RX_INIT_OVRD_EN(x)        (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_OVRD2_RX_INIT_OVRD_EN_SHIFT)) & XCVR_TSM_OVRD2_RX_INIT_OVRD_EN_MASK)
#define XCVR_TSM_OVRD2_RX_INIT_OVRD_MASK         (0x200000U)
#define XCVR_TSM_OVRD2_RX_INIT_OVRD_SHIFT        (21U)
#define XCVR_TSM_OVRD2_RX_INIT_OVRD(x)           (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_OVRD2_RX_INIT_OVRD_SHIFT)) & XCVR_TSM_OVRD2_RX_INIT_OVRD_MASK)
#define XCVR_TSM_OVRD2_SIGMA_DELTA_EN_OVRD_EN_MASK (0x400000U)
#define XCVR_TSM_OVRD2_SIGMA_DELTA_EN_OVRD_EN_SHIFT (22U)
#define XCVR_TSM_OVRD2_SIGMA_DELTA_EN_OVRD_EN(x) (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_OVRD2_SIGMA_DELTA_EN_OVRD_EN_SHIFT)) & XCVR_TSM_OVRD2_SIGMA_DELTA_EN_OVRD_EN_MASK)
#define XCVR_TSM_OVRD2_SIGMA_DELTA_EN_OVRD_MASK  (0x800000U)
#define XCVR_TSM_OVRD2_SIGMA_DELTA_EN_OVRD_SHIFT (23U)
#define XCVR_TSM_OVRD2_SIGMA_DELTA_EN_OVRD(x)    (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_OVRD2_SIGMA_DELTA_EN_OVRD_SHIFT)) & XCVR_TSM_OVRD2_SIGMA_DELTA_EN_OVRD_MASK)
#define XCVR_TSM_OVRD2_RX_PHY_EN_OVRD_EN_MASK    (0x1000000U)
#define XCVR_TSM_OVRD2_RX_PHY_EN_OVRD_EN_SHIFT   (24U)
#define XCVR_TSM_OVRD2_RX_PHY_EN_OVRD_EN(x)      (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_OVRD2_RX_PHY_EN_OVRD_EN_SHIFT)) & XCVR_TSM_OVRD2_RX_PHY_EN_OVRD_EN_MASK)
#define XCVR_TSM_OVRD2_RX_PHY_EN_OVRD_MASK       (0x2000000U)
#define XCVR_TSM_OVRD2_RX_PHY_EN_OVRD_SHIFT      (25U)
#define XCVR_TSM_OVRD2_RX_PHY_EN_OVRD(x)         (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_OVRD2_RX_PHY_EN_OVRD_SHIFT)) & XCVR_TSM_OVRD2_RX_PHY_EN_OVRD_MASK)
#define XCVR_TSM_OVRD2_DCOC_EN_OVRD_EN_MASK      (0x4000000U)
#define XCVR_TSM_OVRD2_DCOC_EN_OVRD_EN_SHIFT     (26U)
#define XCVR_TSM_OVRD2_DCOC_EN_OVRD_EN(x)        (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_OVRD2_DCOC_EN_OVRD_EN_SHIFT)) & XCVR_TSM_OVRD2_DCOC_EN_OVRD_EN_MASK)
#define XCVR_TSM_OVRD2_DCOC_EN_OVRD_MASK         (0x8000000U)
#define XCVR_TSM_OVRD2_DCOC_EN_OVRD_SHIFT        (27U)
#define XCVR_TSM_OVRD2_DCOC_EN_OVRD(x)           (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_OVRD2_DCOC_EN_OVRD_SHIFT)) & XCVR_TSM_OVRD2_DCOC_EN_OVRD_MASK)
#define XCVR_TSM_OVRD2_DCOC_INIT_OVRD_EN_MASK    (0x10000000U)
#define XCVR_TSM_OVRD2_DCOC_INIT_OVRD_EN_SHIFT   (28U)
#define XCVR_TSM_OVRD2_DCOC_INIT_OVRD_EN(x)      (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_OVRD2_DCOC_INIT_OVRD_EN_SHIFT)) & XCVR_TSM_OVRD2_DCOC_INIT_OVRD_EN_MASK)
#define XCVR_TSM_OVRD2_DCOC_INIT_OVRD_MASK       (0x20000000U)
#define XCVR_TSM_OVRD2_DCOC_INIT_OVRD_SHIFT      (29U)
#define XCVR_TSM_OVRD2_DCOC_INIT_OVRD(x)         (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_OVRD2_DCOC_INIT_OVRD_SHIFT)) & XCVR_TSM_OVRD2_DCOC_INIT_OVRD_MASK)
#define XCVR_TSM_OVRD2_FREQ_TARG_LD_EN_OVRD_EN_MASK (0x40000000U)
#define XCVR_TSM_OVRD2_FREQ_TARG_LD_EN_OVRD_EN_SHIFT (30U)
#define XCVR_TSM_OVRD2_FREQ_TARG_LD_EN_OVRD_EN(x) (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_OVRD2_FREQ_TARG_LD_EN_OVRD_EN_SHIFT)) & XCVR_TSM_OVRD2_FREQ_TARG_LD_EN_OVRD_EN_MASK)
#define XCVR_TSM_OVRD2_FREQ_TARG_LD_EN_OVRD_MASK (0x80000000U)
#define XCVR_TSM_OVRD2_FREQ_TARG_LD_EN_OVRD_SHIFT (31U)
#define XCVR_TSM_OVRD2_FREQ_TARG_LD_EN_OVRD(x)   (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_OVRD2_FREQ_TARG_LD_EN_OVRD_SHIFT)) & XCVR_TSM_OVRD2_FREQ_TARG_LD_EN_OVRD_MASK)

/*! @name OVRD3 - TSM OVERRIDE REGISTER 3 */
#define XCVR_TSM_OVRD3_TSM_SPARE0_EN_OVRD_EN_MASK (0x1U)
#define XCVR_TSM_OVRD3_TSM_SPARE0_EN_OVRD_EN_SHIFT (0U)
#define XCVR_TSM_OVRD3_TSM_SPARE0_EN_OVRD_EN(x)  (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_OVRD3_TSM_SPARE0_EN_OVRD_EN_SHIFT)) & XCVR_TSM_OVRD3_TSM_SPARE0_EN_OVRD_EN_MASK)
#define XCVR_TSM_OVRD3_TSM_SPARE0_EN_OVRD_MASK   (0x2U)
#define XCVR_TSM_OVRD3_TSM_SPARE0_EN_OVRD_SHIFT  (1U)
#define XCVR_TSM_OVRD3_TSM_SPARE0_EN_OVRD(x)     (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_OVRD3_TSM_SPARE0_EN_OVRD_SHIFT)) & XCVR_TSM_OVRD3_TSM_SPARE0_EN_OVRD_MASK)
#define XCVR_TSM_OVRD3_TSM_SPARE1_EN_OVRD_EN_MASK (0x4U)
#define XCVR_TSM_OVRD3_TSM_SPARE1_EN_OVRD_EN_SHIFT (2U)
#define XCVR_TSM_OVRD3_TSM_SPARE1_EN_OVRD_EN(x)  (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_OVRD3_TSM_SPARE1_EN_OVRD_EN_SHIFT)) & XCVR_TSM_OVRD3_TSM_SPARE1_EN_OVRD_EN_MASK)
#define XCVR_TSM_OVRD3_TSM_SPARE1_EN_OVRD_MASK   (0x8U)
#define XCVR_TSM_OVRD3_TSM_SPARE1_EN_OVRD_SHIFT  (3U)
#define XCVR_TSM_OVRD3_TSM_SPARE1_EN_OVRD(x)     (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_OVRD3_TSM_SPARE1_EN_OVRD_SHIFT)) & XCVR_TSM_OVRD3_TSM_SPARE1_EN_OVRD_MASK)
#define XCVR_TSM_OVRD3_TSM_SPARE2_EN_OVRD_EN_MASK (0x10U)
#define XCVR_TSM_OVRD3_TSM_SPARE2_EN_OVRD_EN_SHIFT (4U)
#define XCVR_TSM_OVRD3_TSM_SPARE2_EN_OVRD_EN(x)  (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_OVRD3_TSM_SPARE2_EN_OVRD_EN_SHIFT)) & XCVR_TSM_OVRD3_TSM_SPARE2_EN_OVRD_EN_MASK)
#define XCVR_TSM_OVRD3_TSM_SPARE2_EN_OVRD_MASK   (0x20U)
#define XCVR_TSM_OVRD3_TSM_SPARE2_EN_OVRD_SHIFT  (5U)
#define XCVR_TSM_OVRD3_TSM_SPARE2_EN_OVRD(x)     (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_OVRD3_TSM_SPARE2_EN_OVRD_SHIFT)) & XCVR_TSM_OVRD3_TSM_SPARE2_EN_OVRD_MASK)
#define XCVR_TSM_OVRD3_TSM_SPARE3_EN_OVRD_EN_MASK (0x40U)
#define XCVR_TSM_OVRD3_TSM_SPARE3_EN_OVRD_EN_SHIFT (6U)
#define XCVR_TSM_OVRD3_TSM_SPARE3_EN_OVRD_EN(x)  (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_OVRD3_TSM_SPARE3_EN_OVRD_EN_SHIFT)) & XCVR_TSM_OVRD3_TSM_SPARE3_EN_OVRD_EN_MASK)
#define XCVR_TSM_OVRD3_TSM_SPARE3_EN_OVRD_MASK   (0x80U)
#define XCVR_TSM_OVRD3_TSM_SPARE3_EN_OVRD_SHIFT  (7U)
#define XCVR_TSM_OVRD3_TSM_SPARE3_EN_OVRD(x)     (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_OVRD3_TSM_SPARE3_EN_OVRD_SHIFT)) & XCVR_TSM_OVRD3_TSM_SPARE3_EN_OVRD_MASK)
#define XCVR_TSM_OVRD3_RXTX_AUXPLL_BIAS_EN_OVRD_EN_MASK (0x100U)
#define XCVR_TSM_OVRD3_RXTX_AUXPLL_BIAS_EN_OVRD_EN_SHIFT (8U)
#define XCVR_TSM_OVRD3_RXTX_AUXPLL_BIAS_EN_OVRD_EN(x) (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_OVRD3_RXTX_AUXPLL_BIAS_EN_OVRD_EN_SHIFT)) & XCVR_TSM_OVRD3_RXTX_AUXPLL_BIAS_EN_OVRD_EN_MASK)
#define XCVR_TSM_OVRD3_RXTX_AUXPLL_BIAS_EN_OVRD_MASK (0x200U)
#define XCVR_TSM_OVRD3_RXTX_AUXPLL_BIAS_EN_OVRD_SHIFT (9U)
#define XCVR_TSM_OVRD3_RXTX_AUXPLL_BIAS_EN_OVRD(x) (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_OVRD3_RXTX_AUXPLL_BIAS_EN_OVRD_SHIFT)) & XCVR_TSM_OVRD3_RXTX_AUXPLL_BIAS_EN_OVRD_MASK)
#define XCVR_TSM_OVRD3_RXTX_AUXPLL_VCO_EN_OVRD_EN_MASK (0x400U)
#define XCVR_TSM_OVRD3_RXTX_AUXPLL_VCO_EN_OVRD_EN_SHIFT (10U)
#define XCVR_TSM_OVRD3_RXTX_AUXPLL_VCO_EN_OVRD_EN(x) (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_OVRD3_RXTX_AUXPLL_VCO_EN_OVRD_EN_SHIFT)) & XCVR_TSM_OVRD3_RXTX_AUXPLL_VCO_EN_OVRD_EN_MASK)
#define XCVR_TSM_OVRD3_RXTX_AUXPLL_VCO_EN_OVRD_MASK (0x800U)
#define XCVR_TSM_OVRD3_RXTX_AUXPLL_VCO_EN_OVRD_SHIFT (11U)
#define XCVR_TSM_OVRD3_RXTX_AUXPLL_VCO_EN_OVRD(x) (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_OVRD3_RXTX_AUXPLL_VCO_EN_OVRD_SHIFT)) & XCVR_TSM_OVRD3_RXTX_AUXPLL_VCO_EN_OVRD_MASK)
#define XCVR_TSM_OVRD3_RXTX_AUXPLL_FCAL_EN_OVRD_EN_MASK (0x1000U)
#define XCVR_TSM_OVRD3_RXTX_AUXPLL_FCAL_EN_OVRD_EN_SHIFT (12U)
#define XCVR_TSM_OVRD3_RXTX_AUXPLL_FCAL_EN_OVRD_EN(x) (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_OVRD3_RXTX_AUXPLL_FCAL_EN_OVRD_EN_SHIFT)) & XCVR_TSM_OVRD3_RXTX_AUXPLL_FCAL_EN_OVRD_EN_MASK)
#define XCVR_TSM_OVRD3_RXTX_AUXPLL_FCAL_EN_OVRD_MASK (0x2000U)
#define XCVR_TSM_OVRD3_RXTX_AUXPLL_FCAL_EN_OVRD_SHIFT (13U)
#define XCVR_TSM_OVRD3_RXTX_AUXPLL_FCAL_EN_OVRD(x) (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_OVRD3_RXTX_AUXPLL_FCAL_EN_OVRD_SHIFT)) & XCVR_TSM_OVRD3_RXTX_AUXPLL_FCAL_EN_OVRD_MASK)
#define XCVR_TSM_OVRD3_RXTX_AUXPLL_LF_EN_OVRD_EN_MASK (0x4000U)
#define XCVR_TSM_OVRD3_RXTX_AUXPLL_LF_EN_OVRD_EN_SHIFT (14U)
#define XCVR_TSM_OVRD3_RXTX_AUXPLL_LF_EN_OVRD_EN(x) (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_OVRD3_RXTX_AUXPLL_LF_EN_OVRD_EN_SHIFT)) & XCVR_TSM_OVRD3_RXTX_AUXPLL_LF_EN_OVRD_EN_MASK)
#define XCVR_TSM_OVRD3_RXTX_AUXPLL_LF_EN_OVRD_MASK (0x8000U)
#define XCVR_TSM_OVRD3_RXTX_AUXPLL_LF_EN_OVRD_SHIFT (15U)
#define XCVR_TSM_OVRD3_RXTX_AUXPLL_LF_EN_OVRD(x) (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_OVRD3_RXTX_AUXPLL_LF_EN_OVRD_SHIFT)) & XCVR_TSM_OVRD3_RXTX_AUXPLL_LF_EN_OVRD_MASK)
#define XCVR_TSM_OVRD3_RXTX_AUXPLL_PD_EN_OVRD_EN_MASK (0x10000U)
#define XCVR_TSM_OVRD3_RXTX_AUXPLL_PD_EN_OVRD_EN_SHIFT (16U)
#define XCVR_TSM_OVRD3_RXTX_AUXPLL_PD_EN_OVRD_EN(x) (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_OVRD3_RXTX_AUXPLL_PD_EN_OVRD_EN_SHIFT)) & XCVR_TSM_OVRD3_RXTX_AUXPLL_PD_EN_OVRD_EN_MASK)
#define XCVR_TSM_OVRD3_RXTX_AUXPLL_PD_EN_OVRD_MASK (0x20000U)
#define XCVR_TSM_OVRD3_RXTX_AUXPLL_PD_EN_OVRD_SHIFT (17U)
#define XCVR_TSM_OVRD3_RXTX_AUXPLL_PD_EN_OVRD(x) (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_OVRD3_RXTX_AUXPLL_PD_EN_OVRD_SHIFT)) & XCVR_TSM_OVRD3_RXTX_AUXPLL_PD_EN_OVRD_MASK)
#define XCVR_TSM_OVRD3_RXTX_AUXPLL_PD_LF_FILTER_CHARGE_EN_OVRD_EN_MASK (0x40000U)
#define XCVR_TSM_OVRD3_RXTX_AUXPLL_PD_LF_FILTER_CHARGE_EN_OVRD_EN_SHIFT (18U)
#define XCVR_TSM_OVRD3_RXTX_AUXPLL_PD_LF_FILTER_CHARGE_EN_OVRD_EN(x) (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_OVRD3_RXTX_AUXPLL_PD_LF_FILTER_CHARGE_EN_OVRD_EN_SHIFT)) & XCVR_TSM_OVRD3_RXTX_AUXPLL_PD_LF_FILTER_CHARGE_EN_OVRD_EN_MASK)
#define XCVR_TSM_OVRD3_RXTX_AUXPLL_PD_LF_FILTER_CHARGE_EN_OVRD_MASK (0x80000U)
#define XCVR_TSM_OVRD3_RXTX_AUXPLL_PD_LF_FILTER_CHARGE_EN_OVRD_SHIFT (19U)
#define XCVR_TSM_OVRD3_RXTX_AUXPLL_PD_LF_FILTER_CHARGE_EN_OVRD(x) (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_OVRD3_RXTX_AUXPLL_PD_LF_FILTER_CHARGE_EN_OVRD_SHIFT)) & XCVR_TSM_OVRD3_RXTX_AUXPLL_PD_LF_FILTER_CHARGE_EN_OVRD_MASK)
#define XCVR_TSM_OVRD3_RXTX_AUXPLL_ADC_BUF_EN_OVRD_EN_MASK (0x100000U)
#define XCVR_TSM_OVRD3_RXTX_AUXPLL_ADC_BUF_EN_OVRD_EN_SHIFT (20U)
#define XCVR_TSM_OVRD3_RXTX_AUXPLL_ADC_BUF_EN_OVRD_EN(x) (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_OVRD3_RXTX_AUXPLL_ADC_BUF_EN_OVRD_EN_SHIFT)) & XCVR_TSM_OVRD3_RXTX_AUXPLL_ADC_BUF_EN_OVRD_EN_MASK)
#define XCVR_TSM_OVRD3_RXTX_AUXPLL_ADC_BUF_EN_OVRD_MASK (0x200000U)
#define XCVR_TSM_OVRD3_RXTX_AUXPLL_ADC_BUF_EN_OVRD_SHIFT (21U)
#define XCVR_TSM_OVRD3_RXTX_AUXPLL_ADC_BUF_EN_OVRD(x) (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_OVRD3_RXTX_AUXPLL_ADC_BUF_EN_OVRD_SHIFT)) & XCVR_TSM_OVRD3_RXTX_AUXPLL_ADC_BUF_EN_OVRD_MASK)
#define XCVR_TSM_OVRD3_RXTX_AUXPLL_DIG_BUF_EN_OVRD_EN_MASK (0x400000U)
#define XCVR_TSM_OVRD3_RXTX_AUXPLL_DIG_BUF_EN_OVRD_EN_SHIFT (22U)
#define XCVR_TSM_OVRD3_RXTX_AUXPLL_DIG_BUF_EN_OVRD_EN(x) (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_OVRD3_RXTX_AUXPLL_DIG_BUF_EN_OVRD_EN_SHIFT)) & XCVR_TSM_OVRD3_RXTX_AUXPLL_DIG_BUF_EN_OVRD_EN_MASK)
#define XCVR_TSM_OVRD3_RXTX_AUXPLL_DIG_BUF_EN_OVRD_MASK (0x800000U)
#define XCVR_TSM_OVRD3_RXTX_AUXPLL_DIG_BUF_EN_OVRD_SHIFT (23U)
#define XCVR_TSM_OVRD3_RXTX_AUXPLL_DIG_BUF_EN_OVRD(x) (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_OVRD3_RXTX_AUXPLL_DIG_BUF_EN_OVRD_SHIFT)) & XCVR_TSM_OVRD3_RXTX_AUXPLL_DIG_BUF_EN_OVRD_MASK)
#define XCVR_TSM_OVRD3_RXTX_RCCAL_EN_OVRD_EN_MASK (0x1000000U)
#define XCVR_TSM_OVRD3_RXTX_RCCAL_EN_OVRD_EN_SHIFT (24U)
#define XCVR_TSM_OVRD3_RXTX_RCCAL_EN_OVRD_EN(x)  (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_OVRD3_RXTX_RCCAL_EN_OVRD_EN_SHIFT)) & XCVR_TSM_OVRD3_RXTX_RCCAL_EN_OVRD_EN_MASK)
#define XCVR_TSM_OVRD3_RXTX_RCCAL_EN_OVRD_MASK   (0x2000000U)
#define XCVR_TSM_OVRD3_RXTX_RCCAL_EN_OVRD_SHIFT  (25U)
#define XCVR_TSM_OVRD3_RXTX_RCCAL_EN_OVRD(x)     (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_OVRD3_RXTX_RCCAL_EN_OVRD_SHIFT)) & XCVR_TSM_OVRD3_RXTX_RCCAL_EN_OVRD_MASK)
#define XCVR_TSM_OVRD3_TX_HPM_DAC_EN_OVRD_EN_MASK (0x4000000U)
#define XCVR_TSM_OVRD3_TX_HPM_DAC_EN_OVRD_EN_SHIFT (26U)
#define XCVR_TSM_OVRD3_TX_HPM_DAC_EN_OVRD_EN(x)  (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_OVRD3_TX_HPM_DAC_EN_OVRD_EN_SHIFT)) & XCVR_TSM_OVRD3_TX_HPM_DAC_EN_OVRD_EN_MASK)
#define XCVR_TSM_OVRD3_TX_HPM_DAC_EN_OVRD_MASK   (0x8000000U)
#define XCVR_TSM_OVRD3_TX_HPM_DAC_EN_OVRD_SHIFT  (27U)
#define XCVR_TSM_OVRD3_TX_HPM_DAC_EN_OVRD(x)     (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_OVRD3_TX_HPM_DAC_EN_OVRD_SHIFT)) & XCVR_TSM_OVRD3_TX_HPM_DAC_EN_OVRD_MASK)
#define XCVR_TSM_OVRD3_TX_MODE_OVRD_EN_MASK      (0x10000000U)
#define XCVR_TSM_OVRD3_TX_MODE_OVRD_EN_SHIFT     (28U)
#define XCVR_TSM_OVRD3_TX_MODE_OVRD_EN(x)        (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_OVRD3_TX_MODE_OVRD_EN_SHIFT)) & XCVR_TSM_OVRD3_TX_MODE_OVRD_EN_MASK)
#define XCVR_TSM_OVRD3_TX_MODE_OVRD_MASK         (0x20000000U)
#define XCVR_TSM_OVRD3_TX_MODE_OVRD_SHIFT        (29U)
#define XCVR_TSM_OVRD3_TX_MODE_OVRD(x)           (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_OVRD3_TX_MODE_OVRD_SHIFT)) & XCVR_TSM_OVRD3_TX_MODE_OVRD_MASK)
#define XCVR_TSM_OVRD3_RX_MODE_OVRD_EN_MASK      (0x40000000U)
#define XCVR_TSM_OVRD3_RX_MODE_OVRD_EN_SHIFT     (30U)
#define XCVR_TSM_OVRD3_RX_MODE_OVRD_EN(x)        (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_OVRD3_RX_MODE_OVRD_EN_SHIFT)) & XCVR_TSM_OVRD3_RX_MODE_OVRD_EN_MASK)
#define XCVR_TSM_OVRD3_RX_MODE_OVRD_MASK         (0x80000000U)
#define XCVR_TSM_OVRD3_RX_MODE_OVRD_SHIFT        (31U)
#define XCVR_TSM_OVRD3_RX_MODE_OVRD(x)           (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_OVRD3_RX_MODE_OVRD_SHIFT)) & XCVR_TSM_OVRD3_RX_MODE_OVRD_MASK)

/*! @name PA_POWER - PA POWER */
#define XCVR_TSM_PA_POWER_PA_POWER_MASK          (0x3FU)
#define XCVR_TSM_PA_POWER_PA_POWER_SHIFT         (0U)
#define XCVR_TSM_PA_POWER_PA_POWER(x)            (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_PA_POWER_PA_POWER_SHIFT)) & XCVR_TSM_PA_POWER_PA_POWER_MASK)

/*! @name PA_RAMP_TBL0 - PA RAMP TABLE 0 */
#define XCVR_TSM_PA_RAMP_TBL0_PA_RAMP0_MASK      (0x3FU)
#define XCVR_TSM_PA_RAMP_TBL0_PA_RAMP0_SHIFT     (0U)
#define XCVR_TSM_PA_RAMP_TBL0_PA_RAMP0(x)        (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_PA_RAMP_TBL0_PA_RAMP0_SHIFT)) & XCVR_TSM_PA_RAMP_TBL0_PA_RAMP0_MASK)
#define XCVR_TSM_PA_RAMP_TBL0_PA_RAMP1_MASK      (0x3F00U)
#define XCVR_TSM_PA_RAMP_TBL0_PA_RAMP1_SHIFT     (8U)
#define XCVR_TSM_PA_RAMP_TBL0_PA_RAMP1(x)        (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_PA_RAMP_TBL0_PA_RAMP1_SHIFT)) & XCVR_TSM_PA_RAMP_TBL0_PA_RAMP1_MASK)
#define XCVR_TSM_PA_RAMP_TBL0_PA_RAMP2_MASK      (0x3F0000U)
#define XCVR_TSM_PA_RAMP_TBL0_PA_RAMP2_SHIFT     (16U)
#define XCVR_TSM_PA_RAMP_TBL0_PA_RAMP2(x)        (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_PA_RAMP_TBL0_PA_RAMP2_SHIFT)) & XCVR_TSM_PA_RAMP_TBL0_PA_RAMP2_MASK)
#define XCVR_TSM_PA_RAMP_TBL0_PA_RAMP3_MASK      (0x3F000000U)
#define XCVR_TSM_PA_RAMP_TBL0_PA_RAMP3_SHIFT     (24U)
#define XCVR_TSM_PA_RAMP_TBL0_PA_RAMP3(x)        (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_PA_RAMP_TBL0_PA_RAMP3_SHIFT)) & XCVR_TSM_PA_RAMP_TBL0_PA_RAMP3_MASK)

/*! @name PA_RAMP_TBL1 - PA RAMP TABLE 1 */
#define XCVR_TSM_PA_RAMP_TBL1_PA_RAMP4_MASK      (0x3FU)
#define XCVR_TSM_PA_RAMP_TBL1_PA_RAMP4_SHIFT     (0U)
#define XCVR_TSM_PA_RAMP_TBL1_PA_RAMP4(x)        (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_PA_RAMP_TBL1_PA_RAMP4_SHIFT)) & XCVR_TSM_PA_RAMP_TBL1_PA_RAMP4_MASK)
#define XCVR_TSM_PA_RAMP_TBL1_PA_RAMP5_MASK      (0x3F00U)
#define XCVR_TSM_PA_RAMP_TBL1_PA_RAMP5_SHIFT     (8U)
#define XCVR_TSM_PA_RAMP_TBL1_PA_RAMP5(x)        (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_PA_RAMP_TBL1_PA_RAMP5_SHIFT)) & XCVR_TSM_PA_RAMP_TBL1_PA_RAMP5_MASK)
#define XCVR_TSM_PA_RAMP_TBL1_PA_RAMP6_MASK      (0x3F0000U)
#define XCVR_TSM_PA_RAMP_TBL1_PA_RAMP6_SHIFT     (16U)
#define XCVR_TSM_PA_RAMP_TBL1_PA_RAMP6(x)        (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_PA_RAMP_TBL1_PA_RAMP6_SHIFT)) & XCVR_TSM_PA_RAMP_TBL1_PA_RAMP6_MASK)
#define XCVR_TSM_PA_RAMP_TBL1_PA_RAMP7_MASK      (0x3F000000U)
#define XCVR_TSM_PA_RAMP_TBL1_PA_RAMP7_SHIFT     (24U)
#define XCVR_TSM_PA_RAMP_TBL1_PA_RAMP7(x)        (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_PA_RAMP_TBL1_PA_RAMP7_SHIFT)) & XCVR_TSM_PA_RAMP_TBL1_PA_RAMP7_MASK)

/*! @name RECYCLE_COUNT - TSM RECYCLE COUNT */
#define XCVR_TSM_RECYCLE_COUNT_RECYCLE_COUNT0_MASK (0xFFU)
#define XCVR_TSM_RECYCLE_COUNT_RECYCLE_COUNT0_SHIFT (0U)
#define XCVR_TSM_RECYCLE_COUNT_RECYCLE_COUNT0(x) (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_RECYCLE_COUNT_RECYCLE_COUNT0_SHIFT)) & XCVR_TSM_RECYCLE_COUNT_RECYCLE_COUNT0_MASK)
#define XCVR_TSM_RECYCLE_COUNT_RECYCLE_COUNT1_MASK (0xFF00U)
#define XCVR_TSM_RECYCLE_COUNT_RECYCLE_COUNT1_SHIFT (8U)
#define XCVR_TSM_RECYCLE_COUNT_RECYCLE_COUNT1(x) (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_RECYCLE_COUNT_RECYCLE_COUNT1_SHIFT)) & XCVR_TSM_RECYCLE_COUNT_RECYCLE_COUNT1_MASK)
#define XCVR_TSM_RECYCLE_COUNT_RECYCLE_COUNT2_MASK (0xFF0000U)
#define XCVR_TSM_RECYCLE_COUNT_RECYCLE_COUNT2_SHIFT (16U)
#define XCVR_TSM_RECYCLE_COUNT_RECYCLE_COUNT2(x) (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_RECYCLE_COUNT_RECYCLE_COUNT2_SHIFT)) & XCVR_TSM_RECYCLE_COUNT_RECYCLE_COUNT2_MASK)

/*! @name FAST_CTRL1 - TSM FAST WARMUP CONTROL REGISTER 1 */
#define XCVR_TSM_FAST_CTRL1_FAST_TX_WU_EN_MASK   (0x1U)
#define XCVR_TSM_FAST_CTRL1_FAST_TX_WU_EN_SHIFT  (0U)
#define XCVR_TSM_FAST_CTRL1_FAST_TX_WU_EN(x)     (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_FAST_CTRL1_FAST_TX_WU_EN_SHIFT)) & XCVR_TSM_FAST_CTRL1_FAST_TX_WU_EN_MASK)
#define XCVR_TSM_FAST_CTRL1_FAST_RX_WU_EN_MASK   (0x2U)
#define XCVR_TSM_FAST_CTRL1_FAST_RX_WU_EN_SHIFT  (1U)
#define XCVR_TSM_FAST_CTRL1_FAST_RX_WU_EN(x)     (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_FAST_CTRL1_FAST_RX_WU_EN_SHIFT)) & XCVR_TSM_FAST_CTRL1_FAST_RX_WU_EN_MASK)
#define XCVR_TSM_FAST_CTRL1_FAST_RX2TX_EN_MASK   (0x4U)
#define XCVR_TSM_FAST_CTRL1_FAST_RX2TX_EN_SHIFT  (2U)
#define XCVR_TSM_FAST_CTRL1_FAST_RX2TX_EN(x)     (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_FAST_CTRL1_FAST_RX2TX_EN_SHIFT)) & XCVR_TSM_FAST_CTRL1_FAST_RX2TX_EN_MASK)
#define XCVR_TSM_FAST_CTRL1_FAST_WU_CLEAR_MASK   (0x8U)
#define XCVR_TSM_FAST_CTRL1_FAST_WU_CLEAR_SHIFT  (3U)
#define XCVR_TSM_FAST_CTRL1_FAST_WU_CLEAR(x)     (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_FAST_CTRL1_FAST_WU_CLEAR_SHIFT)) & XCVR_TSM_FAST_CTRL1_FAST_WU_CLEAR_MASK)
#define XCVR_TSM_FAST_CTRL1_FAST_RX2TX_START_MASK (0xFF00U)
#define XCVR_TSM_FAST_CTRL1_FAST_RX2TX_START_SHIFT (8U)
#define XCVR_TSM_FAST_CTRL1_FAST_RX2TX_START(x)  (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_FAST_CTRL1_FAST_RX2TX_START_SHIFT)) & XCVR_TSM_FAST_CTRL1_FAST_RX2TX_START_MASK)

/*! @name FAST_CTRL2 - TSM FAST WARMUP CONTROL REGISTER 2 */
#define XCVR_TSM_FAST_CTRL2_FAST_START_TX_MASK   (0xFFU)
#define XCVR_TSM_FAST_CTRL2_FAST_START_TX_SHIFT  (0U)
#define XCVR_TSM_FAST_CTRL2_FAST_START_TX(x)     (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_FAST_CTRL2_FAST_START_TX_SHIFT)) & XCVR_TSM_FAST_CTRL2_FAST_START_TX_MASK)
#define XCVR_TSM_FAST_CTRL2_FAST_DEST_TX_MASK    (0xFF00U)
#define XCVR_TSM_FAST_CTRL2_FAST_DEST_TX_SHIFT   (8U)
#define XCVR_TSM_FAST_CTRL2_FAST_DEST_TX(x)      (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_FAST_CTRL2_FAST_DEST_TX_SHIFT)) & XCVR_TSM_FAST_CTRL2_FAST_DEST_TX_MASK)
#define XCVR_TSM_FAST_CTRL2_FAST_START_RX_MASK   (0xFF0000U)
#define XCVR_TSM_FAST_CTRL2_FAST_START_RX_SHIFT  (16U)
#define XCVR_TSM_FAST_CTRL2_FAST_START_RX(x)     (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_FAST_CTRL2_FAST_START_RX_SHIFT)) & XCVR_TSM_FAST_CTRL2_FAST_START_RX_MASK)
#define XCVR_TSM_FAST_CTRL2_FAST_DEST_RX_MASK    (0xFF000000U)
#define XCVR_TSM_FAST_CTRL2_FAST_DEST_RX_SHIFT   (24U)
#define XCVR_TSM_FAST_CTRL2_FAST_DEST_RX(x)      (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_FAST_CTRL2_FAST_DEST_RX_SHIFT)) & XCVR_TSM_FAST_CTRL2_FAST_DEST_RX_MASK)

/*! @name TIMING00 - TSM_TIMING00 */
#define XCVR_TSM_TIMING00_BB_LDO_HF_EN_TX_HI_MASK (0xFFU)
#define XCVR_TSM_TIMING00_BB_LDO_HF_EN_TX_HI_SHIFT (0U)
#define XCVR_TSM_TIMING00_BB_LDO_HF_EN_TX_HI(x)  (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_TIMING00_BB_LDO_HF_EN_TX_HI_SHIFT)) & XCVR_TSM_TIMING00_BB_LDO_HF_EN_TX_HI_MASK)
#define XCVR_TSM_TIMING00_BB_LDO_HF_EN_TX_LO_MASK (0xFF00U)
#define XCVR_TSM_TIMING00_BB_LDO_HF_EN_TX_LO_SHIFT (8U)
#define XCVR_TSM_TIMING00_BB_LDO_HF_EN_TX_LO(x)  (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_TIMING00_BB_LDO_HF_EN_TX_LO_SHIFT)) & XCVR_TSM_TIMING00_BB_LDO_HF_EN_TX_LO_MASK)
#define XCVR_TSM_TIMING00_BB_LDO_HF_EN_RX_HI_MASK (0xFF0000U)
#define XCVR_TSM_TIMING00_BB_LDO_HF_EN_RX_HI_SHIFT (16U)
#define XCVR_TSM_TIMING00_BB_LDO_HF_EN_RX_HI(x)  (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_TIMING00_BB_LDO_HF_EN_RX_HI_SHIFT)) & XCVR_TSM_TIMING00_BB_LDO_HF_EN_RX_HI_MASK)
#define XCVR_TSM_TIMING00_BB_LDO_HF_EN_RX_LO_MASK (0xFF000000U)
#define XCVR_TSM_TIMING00_BB_LDO_HF_EN_RX_LO_SHIFT (24U)
#define XCVR_TSM_TIMING00_BB_LDO_HF_EN_RX_LO(x)  (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_TIMING00_BB_LDO_HF_EN_RX_LO_SHIFT)) & XCVR_TSM_TIMING00_BB_LDO_HF_EN_RX_LO_MASK)

/*! @name TIMING01 - TSM_TIMING01 */
#define XCVR_TSM_TIMING01_BB_LDO_ADCDAC_EN_TX_HI_MASK (0xFFU)
#define XCVR_TSM_TIMING01_BB_LDO_ADCDAC_EN_TX_HI_SHIFT (0U)
#define XCVR_TSM_TIMING01_BB_LDO_ADCDAC_EN_TX_HI(x) (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_TIMING01_BB_LDO_ADCDAC_EN_TX_HI_SHIFT)) & XCVR_TSM_TIMING01_BB_LDO_ADCDAC_EN_TX_HI_MASK)
#define XCVR_TSM_TIMING01_BB_LDO_ADCDAC_EN_TX_LO_MASK (0xFF00U)
#define XCVR_TSM_TIMING01_BB_LDO_ADCDAC_EN_TX_LO_SHIFT (8U)
#define XCVR_TSM_TIMING01_BB_LDO_ADCDAC_EN_TX_LO(x) (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_TIMING01_BB_LDO_ADCDAC_EN_TX_LO_SHIFT)) & XCVR_TSM_TIMING01_BB_LDO_ADCDAC_EN_TX_LO_MASK)
#define XCVR_TSM_TIMING01_BB_LDO_ADCDAC_EN_RX_HI_MASK (0xFF0000U)
#define XCVR_TSM_TIMING01_BB_LDO_ADCDAC_EN_RX_HI_SHIFT (16U)
#define XCVR_TSM_TIMING01_BB_LDO_ADCDAC_EN_RX_HI(x) (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_TIMING01_BB_LDO_ADCDAC_EN_RX_HI_SHIFT)) & XCVR_TSM_TIMING01_BB_LDO_ADCDAC_EN_RX_HI_MASK)
#define XCVR_TSM_TIMING01_BB_LDO_ADCDAC_EN_RX_LO_MASK (0xFF000000U)
#define XCVR_TSM_TIMING01_BB_LDO_ADCDAC_EN_RX_LO_SHIFT (24U)
#define XCVR_TSM_TIMING01_BB_LDO_ADCDAC_EN_RX_LO(x) (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_TIMING01_BB_LDO_ADCDAC_EN_RX_LO_SHIFT)) & XCVR_TSM_TIMING01_BB_LDO_ADCDAC_EN_RX_LO_MASK)

/*! @name TIMING02 - TSM_TIMING02 */
#define XCVR_TSM_TIMING02_BB_LDO_BBA_EN_RX_HI_MASK (0xFF0000U)
#define XCVR_TSM_TIMING02_BB_LDO_BBA_EN_RX_HI_SHIFT (16U)
#define XCVR_TSM_TIMING02_BB_LDO_BBA_EN_RX_HI(x) (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_TIMING02_BB_LDO_BBA_EN_RX_HI_SHIFT)) & XCVR_TSM_TIMING02_BB_LDO_BBA_EN_RX_HI_MASK)
#define XCVR_TSM_TIMING02_BB_LDO_BBA_EN_RX_LO_MASK (0xFF000000U)
#define XCVR_TSM_TIMING02_BB_LDO_BBA_EN_RX_LO_SHIFT (24U)
#define XCVR_TSM_TIMING02_BB_LDO_BBA_EN_RX_LO(x) (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_TIMING02_BB_LDO_BBA_EN_RX_LO_SHIFT)) & XCVR_TSM_TIMING02_BB_LDO_BBA_EN_RX_LO_MASK)

/*! @name TIMING03 - TSM_TIMING03 */
#define XCVR_TSM_TIMING03_BB_LDO_PD_EN_TX_HI_MASK (0xFFU)
#define XCVR_TSM_TIMING03_BB_LDO_PD_EN_TX_HI_SHIFT (0U)
#define XCVR_TSM_TIMING03_BB_LDO_PD_EN_TX_HI(x)  (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_TIMING03_BB_LDO_PD_EN_TX_HI_SHIFT)) & XCVR_TSM_TIMING03_BB_LDO_PD_EN_TX_HI_MASK)
#define XCVR_TSM_TIMING03_BB_LDO_PD_EN_TX_LO_MASK (0xFF00U)
#define XCVR_TSM_TIMING03_BB_LDO_PD_EN_TX_LO_SHIFT (8U)
#define XCVR_TSM_TIMING03_BB_LDO_PD_EN_TX_LO(x)  (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_TIMING03_BB_LDO_PD_EN_TX_LO_SHIFT)) & XCVR_TSM_TIMING03_BB_LDO_PD_EN_TX_LO_MASK)
#define XCVR_TSM_TIMING03_BB_LDO_PD_EN_RX_HI_MASK (0xFF0000U)
#define XCVR_TSM_TIMING03_BB_LDO_PD_EN_RX_HI_SHIFT (16U)
#define XCVR_TSM_TIMING03_BB_LDO_PD_EN_RX_HI(x)  (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_TIMING03_BB_LDO_PD_EN_RX_HI_SHIFT)) & XCVR_TSM_TIMING03_BB_LDO_PD_EN_RX_HI_MASK)
#define XCVR_TSM_TIMING03_BB_LDO_PD_EN_RX_LO_MASK (0xFF000000U)
#define XCVR_TSM_TIMING03_BB_LDO_PD_EN_RX_LO_SHIFT (24U)
#define XCVR_TSM_TIMING03_BB_LDO_PD_EN_RX_LO(x)  (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_TIMING03_BB_LDO_PD_EN_RX_LO_SHIFT)) & XCVR_TSM_TIMING03_BB_LDO_PD_EN_RX_LO_MASK)

/*! @name TIMING04 - TSM_TIMING04 */
#define XCVR_TSM_TIMING04_BB_LDO_FDBK_EN_TX_HI_MASK (0xFFU)
#define XCVR_TSM_TIMING04_BB_LDO_FDBK_EN_TX_HI_SHIFT (0U)
#define XCVR_TSM_TIMING04_BB_LDO_FDBK_EN_TX_HI(x) (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_TIMING04_BB_LDO_FDBK_EN_TX_HI_SHIFT)) & XCVR_TSM_TIMING04_BB_LDO_FDBK_EN_TX_HI_MASK)
#define XCVR_TSM_TIMING04_BB_LDO_FDBK_EN_TX_LO_MASK (0xFF00U)
#define XCVR_TSM_TIMING04_BB_LDO_FDBK_EN_TX_LO_SHIFT (8U)
#define XCVR_TSM_TIMING04_BB_LDO_FDBK_EN_TX_LO(x) (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_TIMING04_BB_LDO_FDBK_EN_TX_LO_SHIFT)) & XCVR_TSM_TIMING04_BB_LDO_FDBK_EN_TX_LO_MASK)
#define XCVR_TSM_TIMING04_BB_LDO_FDBK_EN_RX_HI_MASK (0xFF0000U)
#define XCVR_TSM_TIMING04_BB_LDO_FDBK_EN_RX_HI_SHIFT (16U)
#define XCVR_TSM_TIMING04_BB_LDO_FDBK_EN_RX_HI(x) (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_TIMING04_BB_LDO_FDBK_EN_RX_HI_SHIFT)) & XCVR_TSM_TIMING04_BB_LDO_FDBK_EN_RX_HI_MASK)
#define XCVR_TSM_TIMING04_BB_LDO_FDBK_EN_RX_LO_MASK (0xFF000000U)
#define XCVR_TSM_TIMING04_BB_LDO_FDBK_EN_RX_LO_SHIFT (24U)
#define XCVR_TSM_TIMING04_BB_LDO_FDBK_EN_RX_LO(x) (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_TIMING04_BB_LDO_FDBK_EN_RX_LO_SHIFT)) & XCVR_TSM_TIMING04_BB_LDO_FDBK_EN_RX_LO_MASK)

/*! @name TIMING05 - TSM_TIMING05 */
#define XCVR_TSM_TIMING05_BB_LDO_VCOLO_EN_TX_HI_MASK (0xFFU)
#define XCVR_TSM_TIMING05_BB_LDO_VCOLO_EN_TX_HI_SHIFT (0U)
#define XCVR_TSM_TIMING05_BB_LDO_VCOLO_EN_TX_HI(x) (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_TIMING05_BB_LDO_VCOLO_EN_TX_HI_SHIFT)) & XCVR_TSM_TIMING05_BB_LDO_VCOLO_EN_TX_HI_MASK)
#define XCVR_TSM_TIMING05_BB_LDO_VCOLO_EN_TX_LO_MASK (0xFF00U)
#define XCVR_TSM_TIMING05_BB_LDO_VCOLO_EN_TX_LO_SHIFT (8U)
#define XCVR_TSM_TIMING05_BB_LDO_VCOLO_EN_TX_LO(x) (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_TIMING05_BB_LDO_VCOLO_EN_TX_LO_SHIFT)) & XCVR_TSM_TIMING05_BB_LDO_VCOLO_EN_TX_LO_MASK)
#define XCVR_TSM_TIMING05_BB_LDO_VCOLO_EN_RX_HI_MASK (0xFF0000U)
#define XCVR_TSM_TIMING05_BB_LDO_VCOLO_EN_RX_HI_SHIFT (16U)
#define XCVR_TSM_TIMING05_BB_LDO_VCOLO_EN_RX_HI(x) (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_TIMING05_BB_LDO_VCOLO_EN_RX_HI_SHIFT)) & XCVR_TSM_TIMING05_BB_LDO_VCOLO_EN_RX_HI_MASK)
#define XCVR_TSM_TIMING05_BB_LDO_VCOLO_EN_RX_LO_MASK (0xFF000000U)
#define XCVR_TSM_TIMING05_BB_LDO_VCOLO_EN_RX_LO_SHIFT (24U)
#define XCVR_TSM_TIMING05_BB_LDO_VCOLO_EN_RX_LO(x) (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_TIMING05_BB_LDO_VCOLO_EN_RX_LO_SHIFT)) & XCVR_TSM_TIMING05_BB_LDO_VCOLO_EN_RX_LO_MASK)

/*! @name TIMING06 - TSM_TIMING06 */
#define XCVR_TSM_TIMING06_BB_LDO_VTREF_EN_TX_HI_MASK (0xFFU)
#define XCVR_TSM_TIMING06_BB_LDO_VTREF_EN_TX_HI_SHIFT (0U)
#define XCVR_TSM_TIMING06_BB_LDO_VTREF_EN_TX_HI(x) (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_TIMING06_BB_LDO_VTREF_EN_TX_HI_SHIFT)) & XCVR_TSM_TIMING06_BB_LDO_VTREF_EN_TX_HI_MASK)
#define XCVR_TSM_TIMING06_BB_LDO_VTREF_EN_TX_LO_MASK (0xFF00U)
#define XCVR_TSM_TIMING06_BB_LDO_VTREF_EN_TX_LO_SHIFT (8U)
#define XCVR_TSM_TIMING06_BB_LDO_VTREF_EN_TX_LO(x) (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_TIMING06_BB_LDO_VTREF_EN_TX_LO_SHIFT)) & XCVR_TSM_TIMING06_BB_LDO_VTREF_EN_TX_LO_MASK)
#define XCVR_TSM_TIMING06_BB_LDO_VTREF_EN_RX_HI_MASK (0xFF0000U)
#define XCVR_TSM_TIMING06_BB_LDO_VTREF_EN_RX_HI_SHIFT (16U)
#define XCVR_TSM_TIMING06_BB_LDO_VTREF_EN_RX_HI(x) (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_TIMING06_BB_LDO_VTREF_EN_RX_HI_SHIFT)) & XCVR_TSM_TIMING06_BB_LDO_VTREF_EN_RX_HI_MASK)
#define XCVR_TSM_TIMING06_BB_LDO_VTREF_EN_RX_LO_MASK (0xFF000000U)
#define XCVR_TSM_TIMING06_BB_LDO_VTREF_EN_RX_LO_SHIFT (24U)
#define XCVR_TSM_TIMING06_BB_LDO_VTREF_EN_RX_LO(x) (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_TIMING06_BB_LDO_VTREF_EN_RX_LO_SHIFT)) & XCVR_TSM_TIMING06_BB_LDO_VTREF_EN_RX_LO_MASK)

/*! @name TIMING07 - TSM_TIMING07 */
#define XCVR_TSM_TIMING07_BB_LDO_FDBK_BLEED_EN_TX_HI_MASK (0xFFU)
#define XCVR_TSM_TIMING07_BB_LDO_FDBK_BLEED_EN_TX_HI_SHIFT (0U)
#define XCVR_TSM_TIMING07_BB_LDO_FDBK_BLEED_EN_TX_HI(x) (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_TIMING07_BB_LDO_FDBK_BLEED_EN_TX_HI_SHIFT)) & XCVR_TSM_TIMING07_BB_LDO_FDBK_BLEED_EN_TX_HI_MASK)
#define XCVR_TSM_TIMING07_BB_LDO_FDBK_BLEED_EN_TX_LO_MASK (0xFF00U)
#define XCVR_TSM_TIMING07_BB_LDO_FDBK_BLEED_EN_TX_LO_SHIFT (8U)
#define XCVR_TSM_TIMING07_BB_LDO_FDBK_BLEED_EN_TX_LO(x) (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_TIMING07_BB_LDO_FDBK_BLEED_EN_TX_LO_SHIFT)) & XCVR_TSM_TIMING07_BB_LDO_FDBK_BLEED_EN_TX_LO_MASK)
#define XCVR_TSM_TIMING07_BB_LDO_FDBK_BLEED_EN_RX_HI_MASK (0xFF0000U)
#define XCVR_TSM_TIMING07_BB_LDO_FDBK_BLEED_EN_RX_HI_SHIFT (16U)
#define XCVR_TSM_TIMING07_BB_LDO_FDBK_BLEED_EN_RX_HI(x) (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_TIMING07_BB_LDO_FDBK_BLEED_EN_RX_HI_SHIFT)) & XCVR_TSM_TIMING07_BB_LDO_FDBK_BLEED_EN_RX_HI_MASK)
#define XCVR_TSM_TIMING07_BB_LDO_FDBK_BLEED_EN_RX_LO_MASK (0xFF000000U)
#define XCVR_TSM_TIMING07_BB_LDO_FDBK_BLEED_EN_RX_LO_SHIFT (24U)
#define XCVR_TSM_TIMING07_BB_LDO_FDBK_BLEED_EN_RX_LO(x) (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_TIMING07_BB_LDO_FDBK_BLEED_EN_RX_LO_SHIFT)) & XCVR_TSM_TIMING07_BB_LDO_FDBK_BLEED_EN_RX_LO_MASK)

/*! @name TIMING08 - TSM_TIMING08 */
#define XCVR_TSM_TIMING08_BB_LDO_VCOLO_BLEED_EN_TX_HI_MASK (0xFFU)
#define XCVR_TSM_TIMING08_BB_LDO_VCOLO_BLEED_EN_TX_HI_SHIFT (0U)
#define XCVR_TSM_TIMING08_BB_LDO_VCOLO_BLEED_EN_TX_HI(x) (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_TIMING08_BB_LDO_VCOLO_BLEED_EN_TX_HI_SHIFT)) & XCVR_TSM_TIMING08_BB_LDO_VCOLO_BLEED_EN_TX_HI_MASK)
#define XCVR_TSM_TIMING08_BB_LDO_VCOLO_BLEED_EN_TX_LO_MASK (0xFF00U)
#define XCVR_TSM_TIMING08_BB_LDO_VCOLO_BLEED_EN_TX_LO_SHIFT (8U)
#define XCVR_TSM_TIMING08_BB_LDO_VCOLO_BLEED_EN_TX_LO(x) (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_TIMING08_BB_LDO_VCOLO_BLEED_EN_TX_LO_SHIFT)) & XCVR_TSM_TIMING08_BB_LDO_VCOLO_BLEED_EN_TX_LO_MASK)
#define XCVR_TSM_TIMING08_BB_LDO_VCOLO_BLEED_EN_RX_HI_MASK (0xFF0000U)
#define XCVR_TSM_TIMING08_BB_LDO_VCOLO_BLEED_EN_RX_HI_SHIFT (16U)
#define XCVR_TSM_TIMING08_BB_LDO_VCOLO_BLEED_EN_RX_HI(x) (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_TIMING08_BB_LDO_VCOLO_BLEED_EN_RX_HI_SHIFT)) & XCVR_TSM_TIMING08_BB_LDO_VCOLO_BLEED_EN_RX_HI_MASK)
#define XCVR_TSM_TIMING08_BB_LDO_VCOLO_BLEED_EN_RX_LO_MASK (0xFF000000U)
#define XCVR_TSM_TIMING08_BB_LDO_VCOLO_BLEED_EN_RX_LO_SHIFT (24U)
#define XCVR_TSM_TIMING08_BB_LDO_VCOLO_BLEED_EN_RX_LO(x) (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_TIMING08_BB_LDO_VCOLO_BLEED_EN_RX_LO_SHIFT)) & XCVR_TSM_TIMING08_BB_LDO_VCOLO_BLEED_EN_RX_LO_MASK)

/*! @name TIMING09 - TSM_TIMING09 */
#define XCVR_TSM_TIMING09_BB_LDO_VCOLO_FASTCHARGE_EN_TX_HI_MASK (0xFFU)
#define XCVR_TSM_TIMING09_BB_LDO_VCOLO_FASTCHARGE_EN_TX_HI_SHIFT (0U)
#define XCVR_TSM_TIMING09_BB_LDO_VCOLO_FASTCHARGE_EN_TX_HI(x) (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_TIMING09_BB_LDO_VCOLO_FASTCHARGE_EN_TX_HI_SHIFT)) & XCVR_TSM_TIMING09_BB_LDO_VCOLO_FASTCHARGE_EN_TX_HI_MASK)
#define XCVR_TSM_TIMING09_BB_LDO_VCOLO_FASTCHARGE_EN_TX_LO_MASK (0xFF00U)
#define XCVR_TSM_TIMING09_BB_LDO_VCOLO_FASTCHARGE_EN_TX_LO_SHIFT (8U)
#define XCVR_TSM_TIMING09_BB_LDO_VCOLO_FASTCHARGE_EN_TX_LO(x) (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_TIMING09_BB_LDO_VCOLO_FASTCHARGE_EN_TX_LO_SHIFT)) & XCVR_TSM_TIMING09_BB_LDO_VCOLO_FASTCHARGE_EN_TX_LO_MASK)
#define XCVR_TSM_TIMING09_BB_LDO_VCOLO_FASTCHARGE_EN_RX_HI_MASK (0xFF0000U)
#define XCVR_TSM_TIMING09_BB_LDO_VCOLO_FASTCHARGE_EN_RX_HI_SHIFT (16U)
#define XCVR_TSM_TIMING09_BB_LDO_VCOLO_FASTCHARGE_EN_RX_HI(x) (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_TIMING09_BB_LDO_VCOLO_FASTCHARGE_EN_RX_HI_SHIFT)) & XCVR_TSM_TIMING09_BB_LDO_VCOLO_FASTCHARGE_EN_RX_HI_MASK)
#define XCVR_TSM_TIMING09_BB_LDO_VCOLO_FASTCHARGE_EN_RX_LO_MASK (0xFF000000U)
#define XCVR_TSM_TIMING09_BB_LDO_VCOLO_FASTCHARGE_EN_RX_LO_SHIFT (24U)
#define XCVR_TSM_TIMING09_BB_LDO_VCOLO_FASTCHARGE_EN_RX_LO(x) (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_TIMING09_BB_LDO_VCOLO_FASTCHARGE_EN_RX_LO_SHIFT)) & XCVR_TSM_TIMING09_BB_LDO_VCOLO_FASTCHARGE_EN_RX_LO_MASK)

/*! @name TIMING10 - TSM_TIMING10 */
#define XCVR_TSM_TIMING10_BB_XTAL_PLL_REF_CLK_EN_TX_HI_MASK (0xFFU)
#define XCVR_TSM_TIMING10_BB_XTAL_PLL_REF_CLK_EN_TX_HI_SHIFT (0U)
#define XCVR_TSM_TIMING10_BB_XTAL_PLL_REF_CLK_EN_TX_HI(x) (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_TIMING10_BB_XTAL_PLL_REF_CLK_EN_TX_HI_SHIFT)) & XCVR_TSM_TIMING10_BB_XTAL_PLL_REF_CLK_EN_TX_HI_MASK)
#define XCVR_TSM_TIMING10_BB_XTAL_PLL_REF_CLK_EN_TX_LO_MASK (0xFF00U)
#define XCVR_TSM_TIMING10_BB_XTAL_PLL_REF_CLK_EN_TX_LO_SHIFT (8U)
#define XCVR_TSM_TIMING10_BB_XTAL_PLL_REF_CLK_EN_TX_LO(x) (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_TIMING10_BB_XTAL_PLL_REF_CLK_EN_TX_LO_SHIFT)) & XCVR_TSM_TIMING10_BB_XTAL_PLL_REF_CLK_EN_TX_LO_MASK)
#define XCVR_TSM_TIMING10_BB_XTAL_PLL_REF_CLK_EN_RX_HI_MASK (0xFF0000U)
#define XCVR_TSM_TIMING10_BB_XTAL_PLL_REF_CLK_EN_RX_HI_SHIFT (16U)
#define XCVR_TSM_TIMING10_BB_XTAL_PLL_REF_CLK_EN_RX_HI(x) (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_TIMING10_BB_XTAL_PLL_REF_CLK_EN_RX_HI_SHIFT)) & XCVR_TSM_TIMING10_BB_XTAL_PLL_REF_CLK_EN_RX_HI_MASK)
#define XCVR_TSM_TIMING10_BB_XTAL_PLL_REF_CLK_EN_RX_LO_MASK (0xFF000000U)
#define XCVR_TSM_TIMING10_BB_XTAL_PLL_REF_CLK_EN_RX_LO_SHIFT (24U)
#define XCVR_TSM_TIMING10_BB_XTAL_PLL_REF_CLK_EN_RX_LO(x) (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_TIMING10_BB_XTAL_PLL_REF_CLK_EN_RX_LO_SHIFT)) & XCVR_TSM_TIMING10_BB_XTAL_PLL_REF_CLK_EN_RX_LO_MASK)

/*! @name TIMING11 - TSM_TIMING11 */
#define XCVR_TSM_TIMING11_BB_XTAL_DAC_REF_CLK_EN_TX_HI_MASK (0xFFU)
#define XCVR_TSM_TIMING11_BB_XTAL_DAC_REF_CLK_EN_TX_HI_SHIFT (0U)
#define XCVR_TSM_TIMING11_BB_XTAL_DAC_REF_CLK_EN_TX_HI(x) (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_TIMING11_BB_XTAL_DAC_REF_CLK_EN_TX_HI_SHIFT)) & XCVR_TSM_TIMING11_BB_XTAL_DAC_REF_CLK_EN_TX_HI_MASK)
#define XCVR_TSM_TIMING11_BB_XTAL_DAC_REF_CLK_EN_TX_LO_MASK (0xFF00U)
#define XCVR_TSM_TIMING11_BB_XTAL_DAC_REF_CLK_EN_TX_LO_SHIFT (8U)
#define XCVR_TSM_TIMING11_BB_XTAL_DAC_REF_CLK_EN_TX_LO(x) (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_TIMING11_BB_XTAL_DAC_REF_CLK_EN_TX_LO_SHIFT)) & XCVR_TSM_TIMING11_BB_XTAL_DAC_REF_CLK_EN_TX_LO_MASK)

/*! @name TIMING12 - TSM_TIMING12 */
#define XCVR_TSM_TIMING12_RXTX_AUXPLL_VCO_REF_CLK_EN_RX_HI_MASK (0xFF0000U)
#define XCVR_TSM_TIMING12_RXTX_AUXPLL_VCO_REF_CLK_EN_RX_HI_SHIFT (16U)
#define XCVR_TSM_TIMING12_RXTX_AUXPLL_VCO_REF_CLK_EN_RX_HI(x) (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_TIMING12_RXTX_AUXPLL_VCO_REF_CLK_EN_RX_HI_SHIFT)) & XCVR_TSM_TIMING12_RXTX_AUXPLL_VCO_REF_CLK_EN_RX_HI_MASK)
#define XCVR_TSM_TIMING12_RXTX_AUXPLL_VCO_REF_CLK_EN_RX_LO_MASK (0xFF000000U)
#define XCVR_TSM_TIMING12_RXTX_AUXPLL_VCO_REF_CLK_EN_RX_LO_SHIFT (24U)
#define XCVR_TSM_TIMING12_RXTX_AUXPLL_VCO_REF_CLK_EN_RX_LO(x) (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_TIMING12_RXTX_AUXPLL_VCO_REF_CLK_EN_RX_LO_SHIFT)) & XCVR_TSM_TIMING12_RXTX_AUXPLL_VCO_REF_CLK_EN_RX_LO_MASK)

/*! @name TIMING13 - TSM_TIMING13 */
#define XCVR_TSM_TIMING13_SY_VCO_AUTOTUNE_EN_TX_HI_MASK (0xFFU)
#define XCVR_TSM_TIMING13_SY_VCO_AUTOTUNE_EN_TX_HI_SHIFT (0U)
#define XCVR_TSM_TIMING13_SY_VCO_AUTOTUNE_EN_TX_HI(x) (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_TIMING13_SY_VCO_AUTOTUNE_EN_TX_HI_SHIFT)) & XCVR_TSM_TIMING13_SY_VCO_AUTOTUNE_EN_TX_HI_MASK)
#define XCVR_TSM_TIMING13_SY_VCO_AUTOTUNE_EN_TX_LO_MASK (0xFF00U)
#define XCVR_TSM_TIMING13_SY_VCO_AUTOTUNE_EN_TX_LO_SHIFT (8U)
#define XCVR_TSM_TIMING13_SY_VCO_AUTOTUNE_EN_TX_LO(x) (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_TIMING13_SY_VCO_AUTOTUNE_EN_TX_LO_SHIFT)) & XCVR_TSM_TIMING13_SY_VCO_AUTOTUNE_EN_TX_LO_MASK)
#define XCVR_TSM_TIMING13_SY_VCO_AUTOTUNE_EN_RX_HI_MASK (0xFF0000U)
#define XCVR_TSM_TIMING13_SY_VCO_AUTOTUNE_EN_RX_HI_SHIFT (16U)
#define XCVR_TSM_TIMING13_SY_VCO_AUTOTUNE_EN_RX_HI(x) (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_TIMING13_SY_VCO_AUTOTUNE_EN_RX_HI_SHIFT)) & XCVR_TSM_TIMING13_SY_VCO_AUTOTUNE_EN_RX_HI_MASK)
#define XCVR_TSM_TIMING13_SY_VCO_AUTOTUNE_EN_RX_LO_MASK (0xFF000000U)
#define XCVR_TSM_TIMING13_SY_VCO_AUTOTUNE_EN_RX_LO_SHIFT (24U)
#define XCVR_TSM_TIMING13_SY_VCO_AUTOTUNE_EN_RX_LO(x) (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_TIMING13_SY_VCO_AUTOTUNE_EN_RX_LO_SHIFT)) & XCVR_TSM_TIMING13_SY_VCO_AUTOTUNE_EN_RX_LO_MASK)

/*! @name TIMING14 - TSM_TIMING14 */
#define XCVR_TSM_TIMING14_SY_PD_CYCLE_SLIP_LD_FT_EN_TX_HI_MASK (0xFFU)
#define XCVR_TSM_TIMING14_SY_PD_CYCLE_SLIP_LD_FT_EN_TX_HI_SHIFT (0U)
#define XCVR_TSM_TIMING14_SY_PD_CYCLE_SLIP_LD_FT_EN_TX_HI(x) (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_TIMING14_SY_PD_CYCLE_SLIP_LD_FT_EN_TX_HI_SHIFT)) & XCVR_TSM_TIMING14_SY_PD_CYCLE_SLIP_LD_FT_EN_TX_HI_MASK)
#define XCVR_TSM_TIMING14_SY_PD_CYCLE_SLIP_LD_FT_EN_TX_LO_MASK (0xFF00U)
#define XCVR_TSM_TIMING14_SY_PD_CYCLE_SLIP_LD_FT_EN_TX_LO_SHIFT (8U)
#define XCVR_TSM_TIMING14_SY_PD_CYCLE_SLIP_LD_FT_EN_TX_LO(x) (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_TIMING14_SY_PD_CYCLE_SLIP_LD_FT_EN_TX_LO_SHIFT)) & XCVR_TSM_TIMING14_SY_PD_CYCLE_SLIP_LD_FT_EN_TX_LO_MASK)
#define XCVR_TSM_TIMING14_SY_PD_CYCLE_SLIP_LD_FT_EN_RX_HI_MASK (0xFF0000U)
#define XCVR_TSM_TIMING14_SY_PD_CYCLE_SLIP_LD_FT_EN_RX_HI_SHIFT (16U)
#define XCVR_TSM_TIMING14_SY_PD_CYCLE_SLIP_LD_FT_EN_RX_HI(x) (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_TIMING14_SY_PD_CYCLE_SLIP_LD_FT_EN_RX_HI_SHIFT)) & XCVR_TSM_TIMING14_SY_PD_CYCLE_SLIP_LD_FT_EN_RX_HI_MASK)
#define XCVR_TSM_TIMING14_SY_PD_CYCLE_SLIP_LD_FT_EN_RX_LO_MASK (0xFF000000U)
#define XCVR_TSM_TIMING14_SY_PD_CYCLE_SLIP_LD_FT_EN_RX_LO_SHIFT (24U)
#define XCVR_TSM_TIMING14_SY_PD_CYCLE_SLIP_LD_FT_EN_RX_LO(x) (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_TIMING14_SY_PD_CYCLE_SLIP_LD_FT_EN_RX_LO_SHIFT)) & XCVR_TSM_TIMING14_SY_PD_CYCLE_SLIP_LD_FT_EN_RX_LO_MASK)

/*! @name TIMING15 - TSM_TIMING15 */
#define XCVR_TSM_TIMING15_SY_VCO_EN_TX_HI_MASK   (0xFFU)
#define XCVR_TSM_TIMING15_SY_VCO_EN_TX_HI_SHIFT  (0U)
#define XCVR_TSM_TIMING15_SY_VCO_EN_TX_HI(x)     (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_TIMING15_SY_VCO_EN_TX_HI_SHIFT)) & XCVR_TSM_TIMING15_SY_VCO_EN_TX_HI_MASK)
#define XCVR_TSM_TIMING15_SY_VCO_EN_TX_LO_MASK   (0xFF00U)
#define XCVR_TSM_TIMING15_SY_VCO_EN_TX_LO_SHIFT  (8U)
#define XCVR_TSM_TIMING15_SY_VCO_EN_TX_LO(x)     (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_TIMING15_SY_VCO_EN_TX_LO_SHIFT)) & XCVR_TSM_TIMING15_SY_VCO_EN_TX_LO_MASK)
#define XCVR_TSM_TIMING15_SY_VCO_EN_RX_HI_MASK   (0xFF0000U)
#define XCVR_TSM_TIMING15_SY_VCO_EN_RX_HI_SHIFT  (16U)
#define XCVR_TSM_TIMING15_SY_VCO_EN_RX_HI(x)     (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_TIMING15_SY_VCO_EN_RX_HI_SHIFT)) & XCVR_TSM_TIMING15_SY_VCO_EN_RX_HI_MASK)
#define XCVR_TSM_TIMING15_SY_VCO_EN_RX_LO_MASK   (0xFF000000U)
#define XCVR_TSM_TIMING15_SY_VCO_EN_RX_LO_SHIFT  (24U)
#define XCVR_TSM_TIMING15_SY_VCO_EN_RX_LO(x)     (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_TIMING15_SY_VCO_EN_RX_LO_SHIFT)) & XCVR_TSM_TIMING15_SY_VCO_EN_RX_LO_MASK)

/*! @name TIMING16 - TSM_TIMING16 */
#define XCVR_TSM_TIMING16_SY_LO_RX_BUF_EN_RX_HI_MASK (0xFF0000U)
#define XCVR_TSM_TIMING16_SY_LO_RX_BUF_EN_RX_HI_SHIFT (16U)
#define XCVR_TSM_TIMING16_SY_LO_RX_BUF_EN_RX_HI(x) (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_TIMING16_SY_LO_RX_BUF_EN_RX_HI_SHIFT)) & XCVR_TSM_TIMING16_SY_LO_RX_BUF_EN_RX_HI_MASK)
#define XCVR_TSM_TIMING16_SY_LO_RX_BUF_EN_RX_LO_MASK (0xFF000000U)
#define XCVR_TSM_TIMING16_SY_LO_RX_BUF_EN_RX_LO_SHIFT (24U)
#define XCVR_TSM_TIMING16_SY_LO_RX_BUF_EN_RX_LO(x) (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_TIMING16_SY_LO_RX_BUF_EN_RX_LO_SHIFT)) & XCVR_TSM_TIMING16_SY_LO_RX_BUF_EN_RX_LO_MASK)

/*! @name TIMING17 - TSM_TIMING17 */
#define XCVR_TSM_TIMING17_SY_LO_TX_BUF_EN_TX_HI_MASK (0xFFU)
#define XCVR_TSM_TIMING17_SY_LO_TX_BUF_EN_TX_HI_SHIFT (0U)
#define XCVR_TSM_TIMING17_SY_LO_TX_BUF_EN_TX_HI(x) (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_TIMING17_SY_LO_TX_BUF_EN_TX_HI_SHIFT)) & XCVR_TSM_TIMING17_SY_LO_TX_BUF_EN_TX_HI_MASK)
#define XCVR_TSM_TIMING17_SY_LO_TX_BUF_EN_TX_LO_MASK (0xFF00U)
#define XCVR_TSM_TIMING17_SY_LO_TX_BUF_EN_TX_LO_SHIFT (8U)
#define XCVR_TSM_TIMING17_SY_LO_TX_BUF_EN_TX_LO(x) (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_TIMING17_SY_LO_TX_BUF_EN_TX_LO_SHIFT)) & XCVR_TSM_TIMING17_SY_LO_TX_BUF_EN_TX_LO_MASK)

/*! @name TIMING18 - TSM_TIMING18 */
#define XCVR_TSM_TIMING18_SY_DIVN_EN_TX_HI_MASK  (0xFFU)
#define XCVR_TSM_TIMING18_SY_DIVN_EN_TX_HI_SHIFT (0U)
#define XCVR_TSM_TIMING18_SY_DIVN_EN_TX_HI(x)    (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_TIMING18_SY_DIVN_EN_TX_HI_SHIFT)) & XCVR_TSM_TIMING18_SY_DIVN_EN_TX_HI_MASK)
#define XCVR_TSM_TIMING18_SY_DIVN_EN_TX_LO_MASK  (0xFF00U)
#define XCVR_TSM_TIMING18_SY_DIVN_EN_TX_LO_SHIFT (8U)
#define XCVR_TSM_TIMING18_SY_DIVN_EN_TX_LO(x)    (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_TIMING18_SY_DIVN_EN_TX_LO_SHIFT)) & XCVR_TSM_TIMING18_SY_DIVN_EN_TX_LO_MASK)
#define XCVR_TSM_TIMING18_SY_DIVN_EN_RX_HI_MASK  (0xFF0000U)
#define XCVR_TSM_TIMING18_SY_DIVN_EN_RX_HI_SHIFT (16U)
#define XCVR_TSM_TIMING18_SY_DIVN_EN_RX_HI(x)    (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_TIMING18_SY_DIVN_EN_RX_HI_SHIFT)) & XCVR_TSM_TIMING18_SY_DIVN_EN_RX_HI_MASK)
#define XCVR_TSM_TIMING18_SY_DIVN_EN_RX_LO_MASK  (0xFF000000U)
#define XCVR_TSM_TIMING18_SY_DIVN_EN_RX_LO_SHIFT (24U)
#define XCVR_TSM_TIMING18_SY_DIVN_EN_RX_LO(x)    (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_TIMING18_SY_DIVN_EN_RX_LO_SHIFT)) & XCVR_TSM_TIMING18_SY_DIVN_EN_RX_LO_MASK)

/*! @name TIMING19 - TSM_TIMING19 */
#define XCVR_TSM_TIMING19_SY_PD_FILTER_CHARGE_EN_TX_HI_MASK (0xFFU)
#define XCVR_TSM_TIMING19_SY_PD_FILTER_CHARGE_EN_TX_HI_SHIFT (0U)
#define XCVR_TSM_TIMING19_SY_PD_FILTER_CHARGE_EN_TX_HI(x) (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_TIMING19_SY_PD_FILTER_CHARGE_EN_TX_HI_SHIFT)) & XCVR_TSM_TIMING19_SY_PD_FILTER_CHARGE_EN_TX_HI_MASK)
#define XCVR_TSM_TIMING19_SY_PD_FILTER_CHARGE_EN_TX_LO_MASK (0xFF00U)
#define XCVR_TSM_TIMING19_SY_PD_FILTER_CHARGE_EN_TX_LO_SHIFT (8U)
#define XCVR_TSM_TIMING19_SY_PD_FILTER_CHARGE_EN_TX_LO(x) (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_TIMING19_SY_PD_FILTER_CHARGE_EN_TX_LO_SHIFT)) & XCVR_TSM_TIMING19_SY_PD_FILTER_CHARGE_EN_TX_LO_MASK)
#define XCVR_TSM_TIMING19_SY_PD_FILTER_CHARGE_EN_RX_HI_MASK (0xFF0000U)
#define XCVR_TSM_TIMING19_SY_PD_FILTER_CHARGE_EN_RX_HI_SHIFT (16U)
#define XCVR_TSM_TIMING19_SY_PD_FILTER_CHARGE_EN_RX_HI(x) (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_TIMING19_SY_PD_FILTER_CHARGE_EN_RX_HI_SHIFT)) & XCVR_TSM_TIMING19_SY_PD_FILTER_CHARGE_EN_RX_HI_MASK)
#define XCVR_TSM_TIMING19_SY_PD_FILTER_CHARGE_EN_RX_LO_MASK (0xFF000000U)
#define XCVR_TSM_TIMING19_SY_PD_FILTER_CHARGE_EN_RX_LO_SHIFT (24U)
#define XCVR_TSM_TIMING19_SY_PD_FILTER_CHARGE_EN_RX_LO(x) (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_TIMING19_SY_PD_FILTER_CHARGE_EN_RX_LO_SHIFT)) & XCVR_TSM_TIMING19_SY_PD_FILTER_CHARGE_EN_RX_LO_MASK)

/*! @name TIMING20 - TSM_TIMING20 */
#define XCVR_TSM_TIMING20_SY_PD_EN_TX_HI_MASK    (0xFFU)
#define XCVR_TSM_TIMING20_SY_PD_EN_TX_HI_SHIFT   (0U)
#define XCVR_TSM_TIMING20_SY_PD_EN_TX_HI(x)      (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_TIMING20_SY_PD_EN_TX_HI_SHIFT)) & XCVR_TSM_TIMING20_SY_PD_EN_TX_HI_MASK)
#define XCVR_TSM_TIMING20_SY_PD_EN_TX_LO_MASK    (0xFF00U)
#define XCVR_TSM_TIMING20_SY_PD_EN_TX_LO_SHIFT   (8U)
#define XCVR_TSM_TIMING20_SY_PD_EN_TX_LO(x)      (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_TIMING20_SY_PD_EN_TX_LO_SHIFT)) & XCVR_TSM_TIMING20_SY_PD_EN_TX_LO_MASK)
#define XCVR_TSM_TIMING20_SY_PD_EN_RX_HI_MASK    (0xFF0000U)
#define XCVR_TSM_TIMING20_SY_PD_EN_RX_HI_SHIFT   (16U)
#define XCVR_TSM_TIMING20_SY_PD_EN_RX_HI(x)      (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_TIMING20_SY_PD_EN_RX_HI_SHIFT)) & XCVR_TSM_TIMING20_SY_PD_EN_RX_HI_MASK)
#define XCVR_TSM_TIMING20_SY_PD_EN_RX_LO_MASK    (0xFF000000U)
#define XCVR_TSM_TIMING20_SY_PD_EN_RX_LO_SHIFT   (24U)
#define XCVR_TSM_TIMING20_SY_PD_EN_RX_LO(x)      (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_TIMING20_SY_PD_EN_RX_LO_SHIFT)) & XCVR_TSM_TIMING20_SY_PD_EN_RX_LO_MASK)

/*! @name TIMING21 - TSM_TIMING21 */
#define XCVR_TSM_TIMING21_SY_LO_DIVN_EN_TX_HI_MASK (0xFFU)
#define XCVR_TSM_TIMING21_SY_LO_DIVN_EN_TX_HI_SHIFT (0U)
#define XCVR_TSM_TIMING21_SY_LO_DIVN_EN_TX_HI(x) (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_TIMING21_SY_LO_DIVN_EN_TX_HI_SHIFT)) & XCVR_TSM_TIMING21_SY_LO_DIVN_EN_TX_HI_MASK)
#define XCVR_TSM_TIMING21_SY_LO_DIVN_EN_TX_LO_MASK (0xFF00U)
#define XCVR_TSM_TIMING21_SY_LO_DIVN_EN_TX_LO_SHIFT (8U)
#define XCVR_TSM_TIMING21_SY_LO_DIVN_EN_TX_LO(x) (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_TIMING21_SY_LO_DIVN_EN_TX_LO_SHIFT)) & XCVR_TSM_TIMING21_SY_LO_DIVN_EN_TX_LO_MASK)
#define XCVR_TSM_TIMING21_SY_LO_DIVN_EN_RX_HI_MASK (0xFF0000U)
#define XCVR_TSM_TIMING21_SY_LO_DIVN_EN_RX_HI_SHIFT (16U)
#define XCVR_TSM_TIMING21_SY_LO_DIVN_EN_RX_HI(x) (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_TIMING21_SY_LO_DIVN_EN_RX_HI_SHIFT)) & XCVR_TSM_TIMING21_SY_LO_DIVN_EN_RX_HI_MASK)
#define XCVR_TSM_TIMING21_SY_LO_DIVN_EN_RX_LO_MASK (0xFF000000U)
#define XCVR_TSM_TIMING21_SY_LO_DIVN_EN_RX_LO_SHIFT (24U)
#define XCVR_TSM_TIMING21_SY_LO_DIVN_EN_RX_LO(x) (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_TIMING21_SY_LO_DIVN_EN_RX_LO_SHIFT)) & XCVR_TSM_TIMING21_SY_LO_DIVN_EN_RX_LO_MASK)

/*! @name TIMING22 - TSM_TIMING22 */
#define XCVR_TSM_TIMING22_SY_LO_RX_EN_RX_HI_MASK (0xFF0000U)
#define XCVR_TSM_TIMING22_SY_LO_RX_EN_RX_HI_SHIFT (16U)
#define XCVR_TSM_TIMING22_SY_LO_RX_EN_RX_HI(x)   (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_TIMING22_SY_LO_RX_EN_RX_HI_SHIFT)) & XCVR_TSM_TIMING22_SY_LO_RX_EN_RX_HI_MASK)
#define XCVR_TSM_TIMING22_SY_LO_RX_EN_RX_LO_MASK (0xFF000000U)
#define XCVR_TSM_TIMING22_SY_LO_RX_EN_RX_LO_SHIFT (24U)
#define XCVR_TSM_TIMING22_SY_LO_RX_EN_RX_LO(x)   (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_TIMING22_SY_LO_RX_EN_RX_LO_SHIFT)) & XCVR_TSM_TIMING22_SY_LO_RX_EN_RX_LO_MASK)

/*! @name TIMING23 - TSM_TIMING23 */
#define XCVR_TSM_TIMING23_SY_LO_TX_EN_TX_HI_MASK (0xFFU)
#define XCVR_TSM_TIMING23_SY_LO_TX_EN_TX_HI_SHIFT (0U)
#define XCVR_TSM_TIMING23_SY_LO_TX_EN_TX_HI(x)   (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_TIMING23_SY_LO_TX_EN_TX_HI_SHIFT)) & XCVR_TSM_TIMING23_SY_LO_TX_EN_TX_HI_MASK)
#define XCVR_TSM_TIMING23_SY_LO_TX_EN_TX_LO_MASK (0xFF00U)
#define XCVR_TSM_TIMING23_SY_LO_TX_EN_TX_LO_SHIFT (8U)
#define XCVR_TSM_TIMING23_SY_LO_TX_EN_TX_LO(x)   (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_TIMING23_SY_LO_TX_EN_TX_LO_SHIFT)) & XCVR_TSM_TIMING23_SY_LO_TX_EN_TX_LO_MASK)

/*! @name TIMING24 - TSM_TIMING24 */
#define XCVR_TSM_TIMING24_SY_DIVN_CAL_EN_TX_HI_MASK (0xFFU)
#define XCVR_TSM_TIMING24_SY_DIVN_CAL_EN_TX_HI_SHIFT (0U)
#define XCVR_TSM_TIMING24_SY_DIVN_CAL_EN_TX_HI(x) (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_TIMING24_SY_DIVN_CAL_EN_TX_HI_SHIFT)) & XCVR_TSM_TIMING24_SY_DIVN_CAL_EN_TX_HI_MASK)
#define XCVR_TSM_TIMING24_SY_DIVN_CAL_EN_TX_LO_MASK (0xFF00U)
#define XCVR_TSM_TIMING24_SY_DIVN_CAL_EN_TX_LO_SHIFT (8U)
#define XCVR_TSM_TIMING24_SY_DIVN_CAL_EN_TX_LO(x) (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_TIMING24_SY_DIVN_CAL_EN_TX_LO_SHIFT)) & XCVR_TSM_TIMING24_SY_DIVN_CAL_EN_TX_LO_MASK)
#define XCVR_TSM_TIMING24_SY_DIVN_CAL_EN_RX_HI_MASK (0xFF0000U)
#define XCVR_TSM_TIMING24_SY_DIVN_CAL_EN_RX_HI_SHIFT (16U)
#define XCVR_TSM_TIMING24_SY_DIVN_CAL_EN_RX_HI(x) (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_TIMING24_SY_DIVN_CAL_EN_RX_HI_SHIFT)) & XCVR_TSM_TIMING24_SY_DIVN_CAL_EN_RX_HI_MASK)
#define XCVR_TSM_TIMING24_SY_DIVN_CAL_EN_RX_LO_MASK (0xFF000000U)
#define XCVR_TSM_TIMING24_SY_DIVN_CAL_EN_RX_LO_SHIFT (24U)
#define XCVR_TSM_TIMING24_SY_DIVN_CAL_EN_RX_LO(x) (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_TIMING24_SY_DIVN_CAL_EN_RX_LO_SHIFT)) & XCVR_TSM_TIMING24_SY_DIVN_CAL_EN_RX_LO_MASK)

/*! @name TIMING25 - TSM_TIMING25 */
#define XCVR_TSM_TIMING25_RX_LNA_MIXER_EN_RX_HI_MASK (0xFF0000U)
#define XCVR_TSM_TIMING25_RX_LNA_MIXER_EN_RX_HI_SHIFT (16U)
#define XCVR_TSM_TIMING25_RX_LNA_MIXER_EN_RX_HI(x) (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_TIMING25_RX_LNA_MIXER_EN_RX_HI_SHIFT)) & XCVR_TSM_TIMING25_RX_LNA_MIXER_EN_RX_HI_MASK)
#define XCVR_TSM_TIMING25_RX_LNA_MIXER_EN_RX_LO_MASK (0xFF000000U)
#define XCVR_TSM_TIMING25_RX_LNA_MIXER_EN_RX_LO_SHIFT (24U)
#define XCVR_TSM_TIMING25_RX_LNA_MIXER_EN_RX_LO(x) (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_TIMING25_RX_LNA_MIXER_EN_RX_LO_SHIFT)) & XCVR_TSM_TIMING25_RX_LNA_MIXER_EN_RX_LO_MASK)

/*! @name TIMING26 - TSM_TIMING26 */
#define XCVR_TSM_TIMING26_TX_PA_EN_TX_HI_MASK    (0xFFU)
#define XCVR_TSM_TIMING26_TX_PA_EN_TX_HI_SHIFT   (0U)
#define XCVR_TSM_TIMING26_TX_PA_EN_TX_HI(x)      (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_TIMING26_TX_PA_EN_TX_HI_SHIFT)) & XCVR_TSM_TIMING26_TX_PA_EN_TX_HI_MASK)
#define XCVR_TSM_TIMING26_TX_PA_EN_TX_LO_MASK    (0xFF00U)
#define XCVR_TSM_TIMING26_TX_PA_EN_TX_LO_SHIFT   (8U)
#define XCVR_TSM_TIMING26_TX_PA_EN_TX_LO(x)      (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_TIMING26_TX_PA_EN_TX_LO_SHIFT)) & XCVR_TSM_TIMING26_TX_PA_EN_TX_LO_MASK)

/*! @name TIMING27 - TSM_TIMING27 */
#define XCVR_TSM_TIMING27_RX_ADC_I_Q_EN_RX_HI_MASK (0xFF0000U)
#define XCVR_TSM_TIMING27_RX_ADC_I_Q_EN_RX_HI_SHIFT (16U)
#define XCVR_TSM_TIMING27_RX_ADC_I_Q_EN_RX_HI(x) (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_TIMING27_RX_ADC_I_Q_EN_RX_HI_SHIFT)) & XCVR_TSM_TIMING27_RX_ADC_I_Q_EN_RX_HI_MASK)
#define XCVR_TSM_TIMING27_RX_ADC_I_Q_EN_RX_LO_MASK (0xFF000000U)
#define XCVR_TSM_TIMING27_RX_ADC_I_Q_EN_RX_LO_SHIFT (24U)
#define XCVR_TSM_TIMING27_RX_ADC_I_Q_EN_RX_LO(x) (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_TIMING27_RX_ADC_I_Q_EN_RX_LO_SHIFT)) & XCVR_TSM_TIMING27_RX_ADC_I_Q_EN_RX_LO_MASK)

/*! @name TIMING28 - TSM_TIMING28 */
#define XCVR_TSM_TIMING28_RX_ADC_RESET_EN_RX_HI_MASK (0xFF0000U)
#define XCVR_TSM_TIMING28_RX_ADC_RESET_EN_RX_HI_SHIFT (16U)
#define XCVR_TSM_TIMING28_RX_ADC_RESET_EN_RX_HI(x) (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_TIMING28_RX_ADC_RESET_EN_RX_HI_SHIFT)) & XCVR_TSM_TIMING28_RX_ADC_RESET_EN_RX_HI_MASK)
#define XCVR_TSM_TIMING28_RX_ADC_RESET_EN_RX_LO_MASK (0xFF000000U)
#define XCVR_TSM_TIMING28_RX_ADC_RESET_EN_RX_LO_SHIFT (24U)
#define XCVR_TSM_TIMING28_RX_ADC_RESET_EN_RX_LO(x) (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_TIMING28_RX_ADC_RESET_EN_RX_LO_SHIFT)) & XCVR_TSM_TIMING28_RX_ADC_RESET_EN_RX_LO_MASK)

/*! @name TIMING29 - TSM_TIMING29 */
#define XCVR_TSM_TIMING29_RX_BBA_I_Q_EN_RX_HI_MASK (0xFF0000U)
#define XCVR_TSM_TIMING29_RX_BBA_I_Q_EN_RX_HI_SHIFT (16U)
#define XCVR_TSM_TIMING29_RX_BBA_I_Q_EN_RX_HI(x) (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_TIMING29_RX_BBA_I_Q_EN_RX_HI_SHIFT)) & XCVR_TSM_TIMING29_RX_BBA_I_Q_EN_RX_HI_MASK)
#define XCVR_TSM_TIMING29_RX_BBA_I_Q_EN_RX_LO_MASK (0xFF000000U)
#define XCVR_TSM_TIMING29_RX_BBA_I_Q_EN_RX_LO_SHIFT (24U)
#define XCVR_TSM_TIMING29_RX_BBA_I_Q_EN_RX_LO(x) (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_TIMING29_RX_BBA_I_Q_EN_RX_LO_SHIFT)) & XCVR_TSM_TIMING29_RX_BBA_I_Q_EN_RX_LO_MASK)

/*! @name TIMING30 - TSM_TIMING30 */
#define XCVR_TSM_TIMING30_RX_BBA_PDET_EN_RX_HI_MASK (0xFF0000U)
#define XCVR_TSM_TIMING30_RX_BBA_PDET_EN_RX_HI_SHIFT (16U)
#define XCVR_TSM_TIMING30_RX_BBA_PDET_EN_RX_HI(x) (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_TIMING30_RX_BBA_PDET_EN_RX_HI_SHIFT)) & XCVR_TSM_TIMING30_RX_BBA_PDET_EN_RX_HI_MASK)
#define XCVR_TSM_TIMING30_RX_BBA_PDET_EN_RX_LO_MASK (0xFF000000U)
#define XCVR_TSM_TIMING30_RX_BBA_PDET_EN_RX_LO_SHIFT (24U)
#define XCVR_TSM_TIMING30_RX_BBA_PDET_EN_RX_LO(x) (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_TIMING30_RX_BBA_PDET_EN_RX_LO_SHIFT)) & XCVR_TSM_TIMING30_RX_BBA_PDET_EN_RX_LO_MASK)

/*! @name TIMING31 - TSM_TIMING31 */
#define XCVR_TSM_TIMING31_RX_BBA_TZA_DCOC_EN_RX_HI_MASK (0xFF0000U)
#define XCVR_TSM_TIMING31_RX_BBA_TZA_DCOC_EN_RX_HI_SHIFT (16U)
#define XCVR_TSM_TIMING31_RX_BBA_TZA_DCOC_EN_RX_HI(x) (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_TIMING31_RX_BBA_TZA_DCOC_EN_RX_HI_SHIFT)) & XCVR_TSM_TIMING31_RX_BBA_TZA_DCOC_EN_RX_HI_MASK)
#define XCVR_TSM_TIMING31_RX_BBA_TZA_DCOC_EN_RX_LO_MASK (0xFF000000U)
#define XCVR_TSM_TIMING31_RX_BBA_TZA_DCOC_EN_RX_LO_SHIFT (24U)
#define XCVR_TSM_TIMING31_RX_BBA_TZA_DCOC_EN_RX_LO(x) (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_TIMING31_RX_BBA_TZA_DCOC_EN_RX_LO_SHIFT)) & XCVR_TSM_TIMING31_RX_BBA_TZA_DCOC_EN_RX_LO_MASK)

/*! @name TIMING32 - TSM_TIMING32 */
#define XCVR_TSM_TIMING32_RX_TZA_I_Q_EN_RX_HI_MASK (0xFF0000U)
#define XCVR_TSM_TIMING32_RX_TZA_I_Q_EN_RX_HI_SHIFT (16U)
#define XCVR_TSM_TIMING32_RX_TZA_I_Q_EN_RX_HI(x) (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_TIMING32_RX_TZA_I_Q_EN_RX_HI_SHIFT)) & XCVR_TSM_TIMING32_RX_TZA_I_Q_EN_RX_HI_MASK)
#define XCVR_TSM_TIMING32_RX_TZA_I_Q_EN_RX_LO_MASK (0xFF000000U)
#define XCVR_TSM_TIMING32_RX_TZA_I_Q_EN_RX_LO_SHIFT (24U)
#define XCVR_TSM_TIMING32_RX_TZA_I_Q_EN_RX_LO(x) (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_TIMING32_RX_TZA_I_Q_EN_RX_LO_SHIFT)) & XCVR_TSM_TIMING32_RX_TZA_I_Q_EN_RX_LO_MASK)

/*! @name TIMING33 - TSM_TIMING33 */
#define XCVR_TSM_TIMING33_RX_TZA_PDET_EN_RX_HI_MASK (0xFF0000U)
#define XCVR_TSM_TIMING33_RX_TZA_PDET_EN_RX_HI_SHIFT (16U)
#define XCVR_TSM_TIMING33_RX_TZA_PDET_EN_RX_HI(x) (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_TIMING33_RX_TZA_PDET_EN_RX_HI_SHIFT)) & XCVR_TSM_TIMING33_RX_TZA_PDET_EN_RX_HI_MASK)
#define XCVR_TSM_TIMING33_RX_TZA_PDET_EN_RX_LO_MASK (0xFF000000U)
#define XCVR_TSM_TIMING33_RX_TZA_PDET_EN_RX_LO_SHIFT (24U)
#define XCVR_TSM_TIMING33_RX_TZA_PDET_EN_RX_LO(x) (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_TIMING33_RX_TZA_PDET_EN_RX_LO_SHIFT)) & XCVR_TSM_TIMING33_RX_TZA_PDET_EN_RX_LO_MASK)

/*! @name TIMING34 - TSM_TIMING34 */
#define XCVR_TSM_TIMING34_PLL_DIG_EN_TX_HI_MASK  (0xFFU)
#define XCVR_TSM_TIMING34_PLL_DIG_EN_TX_HI_SHIFT (0U)
#define XCVR_TSM_TIMING34_PLL_DIG_EN_TX_HI(x)    (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_TIMING34_PLL_DIG_EN_TX_HI_SHIFT)) & XCVR_TSM_TIMING34_PLL_DIG_EN_TX_HI_MASK)
#define XCVR_TSM_TIMING34_PLL_DIG_EN_TX_LO_MASK  (0xFF00U)
#define XCVR_TSM_TIMING34_PLL_DIG_EN_TX_LO_SHIFT (8U)
#define XCVR_TSM_TIMING34_PLL_DIG_EN_TX_LO(x)    (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_TIMING34_PLL_DIG_EN_TX_LO_SHIFT)) & XCVR_TSM_TIMING34_PLL_DIG_EN_TX_LO_MASK)
#define XCVR_TSM_TIMING34_PLL_DIG_EN_RX_HI_MASK  (0xFF0000U)
#define XCVR_TSM_TIMING34_PLL_DIG_EN_RX_HI_SHIFT (16U)
#define XCVR_TSM_TIMING34_PLL_DIG_EN_RX_HI(x)    (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_TIMING34_PLL_DIG_EN_RX_HI_SHIFT)) & XCVR_TSM_TIMING34_PLL_DIG_EN_RX_HI_MASK)
#define XCVR_TSM_TIMING34_PLL_DIG_EN_RX_LO_MASK  (0xFF000000U)
#define XCVR_TSM_TIMING34_PLL_DIG_EN_RX_LO_SHIFT (24U)
#define XCVR_TSM_TIMING34_PLL_DIG_EN_RX_LO(x)    (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_TIMING34_PLL_DIG_EN_RX_LO_SHIFT)) & XCVR_TSM_TIMING34_PLL_DIG_EN_RX_LO_MASK)

/*! @name TIMING35 - TSM_TIMING35 */
#define XCVR_TSM_TIMING35_TX_DIG_EN_TX_HI_MASK   (0xFFU)
#define XCVR_TSM_TIMING35_TX_DIG_EN_TX_HI_SHIFT  (0U)
#define XCVR_TSM_TIMING35_TX_DIG_EN_TX_HI(x)     (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_TIMING35_TX_DIG_EN_TX_HI_SHIFT)) & XCVR_TSM_TIMING35_TX_DIG_EN_TX_HI_MASK)
#define XCVR_TSM_TIMING35_TX_DIG_EN_TX_LO_MASK   (0xFF00U)
#define XCVR_TSM_TIMING35_TX_DIG_EN_TX_LO_SHIFT  (8U)
#define XCVR_TSM_TIMING35_TX_DIG_EN_TX_LO(x)     (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_TIMING35_TX_DIG_EN_TX_LO_SHIFT)) & XCVR_TSM_TIMING35_TX_DIG_EN_TX_LO_MASK)

/*! @name TIMING36 - TSM_TIMING36 */
#define XCVR_TSM_TIMING36_RX_DIG_EN_RX_HI_MASK   (0xFF0000U)
#define XCVR_TSM_TIMING36_RX_DIG_EN_RX_HI_SHIFT  (16U)
#define XCVR_TSM_TIMING36_RX_DIG_EN_RX_HI(x)     (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_TIMING36_RX_DIG_EN_RX_HI_SHIFT)) & XCVR_TSM_TIMING36_RX_DIG_EN_RX_HI_MASK)
#define XCVR_TSM_TIMING36_RX_DIG_EN_RX_LO_MASK   (0xFF000000U)
#define XCVR_TSM_TIMING36_RX_DIG_EN_RX_LO_SHIFT  (24U)
#define XCVR_TSM_TIMING36_RX_DIG_EN_RX_LO(x)     (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_TIMING36_RX_DIG_EN_RX_LO_SHIFT)) & XCVR_TSM_TIMING36_RX_DIG_EN_RX_LO_MASK)

/*! @name TIMING37 - TSM_TIMING37 */
#define XCVR_TSM_TIMING37_RX_INIT_RX_HI_MASK     (0xFF0000U)
#define XCVR_TSM_TIMING37_RX_INIT_RX_HI_SHIFT    (16U)
#define XCVR_TSM_TIMING37_RX_INIT_RX_HI(x)       (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_TIMING37_RX_INIT_RX_HI_SHIFT)) & XCVR_TSM_TIMING37_RX_INIT_RX_HI_MASK)
#define XCVR_TSM_TIMING37_RX_INIT_RX_LO_MASK     (0xFF000000U)
#define XCVR_TSM_TIMING37_RX_INIT_RX_LO_SHIFT    (24U)
#define XCVR_TSM_TIMING37_RX_INIT_RX_LO(x)       (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_TIMING37_RX_INIT_RX_LO_SHIFT)) & XCVR_TSM_TIMING37_RX_INIT_RX_LO_MASK)

/*! @name TIMING38 - TSM_TIMING38 */
#define XCVR_TSM_TIMING38_SIGMA_DELTA_EN_TX_HI_MASK (0xFFU)
#define XCVR_TSM_TIMING38_SIGMA_DELTA_EN_TX_HI_SHIFT (0U)
#define XCVR_TSM_TIMING38_SIGMA_DELTA_EN_TX_HI(x) (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_TIMING38_SIGMA_DELTA_EN_TX_HI_SHIFT)) & XCVR_TSM_TIMING38_SIGMA_DELTA_EN_TX_HI_MASK)
#define XCVR_TSM_TIMING38_SIGMA_DELTA_EN_TX_LO_MASK (0xFF00U)
#define XCVR_TSM_TIMING38_SIGMA_DELTA_EN_TX_LO_SHIFT (8U)
#define XCVR_TSM_TIMING38_SIGMA_DELTA_EN_TX_LO(x) (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_TIMING38_SIGMA_DELTA_EN_TX_LO_SHIFT)) & XCVR_TSM_TIMING38_SIGMA_DELTA_EN_TX_LO_MASK)
#define XCVR_TSM_TIMING38_SIGMA_DELTA_EN_RX_HI_MASK (0xFF0000U)
#define XCVR_TSM_TIMING38_SIGMA_DELTA_EN_RX_HI_SHIFT (16U)
#define XCVR_TSM_TIMING38_SIGMA_DELTA_EN_RX_HI(x) (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_TIMING38_SIGMA_DELTA_EN_RX_HI_SHIFT)) & XCVR_TSM_TIMING38_SIGMA_DELTA_EN_RX_HI_MASK)
#define XCVR_TSM_TIMING38_SIGMA_DELTA_EN_RX_LO_MASK (0xFF000000U)
#define XCVR_TSM_TIMING38_SIGMA_DELTA_EN_RX_LO_SHIFT (24U)
#define XCVR_TSM_TIMING38_SIGMA_DELTA_EN_RX_LO(x) (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_TIMING38_SIGMA_DELTA_EN_RX_LO_SHIFT)) & XCVR_TSM_TIMING38_SIGMA_DELTA_EN_RX_LO_MASK)

/*! @name TIMING39 - TSM_TIMING39 */
#define XCVR_TSM_TIMING39_RX_PHY_EN_RX_HI_MASK   (0xFF0000U)
#define XCVR_TSM_TIMING39_RX_PHY_EN_RX_HI_SHIFT  (16U)
#define XCVR_TSM_TIMING39_RX_PHY_EN_RX_HI(x)     (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_TIMING39_RX_PHY_EN_RX_HI_SHIFT)) & XCVR_TSM_TIMING39_RX_PHY_EN_RX_HI_MASK)
#define XCVR_TSM_TIMING39_RX_PHY_EN_RX_LO_MASK   (0xFF000000U)
#define XCVR_TSM_TIMING39_RX_PHY_EN_RX_LO_SHIFT  (24U)
#define XCVR_TSM_TIMING39_RX_PHY_EN_RX_LO(x)     (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_TIMING39_RX_PHY_EN_RX_LO_SHIFT)) & XCVR_TSM_TIMING39_RX_PHY_EN_RX_LO_MASK)

/*! @name TIMING40 - TSM_TIMING40 */
#define XCVR_TSM_TIMING40_DCOC_EN_RX_HI_MASK     (0xFF0000U)
#define XCVR_TSM_TIMING40_DCOC_EN_RX_HI_SHIFT    (16U)
#define XCVR_TSM_TIMING40_DCOC_EN_RX_HI(x)       (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_TIMING40_DCOC_EN_RX_HI_SHIFT)) & XCVR_TSM_TIMING40_DCOC_EN_RX_HI_MASK)
#define XCVR_TSM_TIMING40_DCOC_EN_RX_LO_MASK     (0xFF000000U)
#define XCVR_TSM_TIMING40_DCOC_EN_RX_LO_SHIFT    (24U)
#define XCVR_TSM_TIMING40_DCOC_EN_RX_LO(x)       (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_TIMING40_DCOC_EN_RX_LO_SHIFT)) & XCVR_TSM_TIMING40_DCOC_EN_RX_LO_MASK)

/*! @name TIMING41 - TSM_TIMING41 */
#define XCVR_TSM_TIMING41_DCOC_INIT_RX_HI_MASK   (0xFF0000U)
#define XCVR_TSM_TIMING41_DCOC_INIT_RX_HI_SHIFT  (16U)
#define XCVR_TSM_TIMING41_DCOC_INIT_RX_HI(x)     (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_TIMING41_DCOC_INIT_RX_HI_SHIFT)) & XCVR_TSM_TIMING41_DCOC_INIT_RX_HI_MASK)
#define XCVR_TSM_TIMING41_DCOC_INIT_RX_LO_MASK   (0xFF000000U)
#define XCVR_TSM_TIMING41_DCOC_INIT_RX_LO_SHIFT  (24U)
#define XCVR_TSM_TIMING41_DCOC_INIT_RX_LO(x)     (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_TIMING41_DCOC_INIT_RX_LO_SHIFT)) & XCVR_TSM_TIMING41_DCOC_INIT_RX_LO_MASK)

/*! @name TIMING42 - TSM_TIMING42 */
#define XCVR_TSM_TIMING42_SAR_ADC_TRIG_EN_TX_HI_MASK (0xFFU)
#define XCVR_TSM_TIMING42_SAR_ADC_TRIG_EN_TX_HI_SHIFT (0U)
#define XCVR_TSM_TIMING42_SAR_ADC_TRIG_EN_TX_HI(x) (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_TIMING42_SAR_ADC_TRIG_EN_TX_HI_SHIFT)) & XCVR_TSM_TIMING42_SAR_ADC_TRIG_EN_TX_HI_MASK)
#define XCVR_TSM_TIMING42_SAR_ADC_TRIG_EN_TX_LO_MASK (0xFF00U)
#define XCVR_TSM_TIMING42_SAR_ADC_TRIG_EN_TX_LO_SHIFT (8U)
#define XCVR_TSM_TIMING42_SAR_ADC_TRIG_EN_TX_LO(x) (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_TIMING42_SAR_ADC_TRIG_EN_TX_LO_SHIFT)) & XCVR_TSM_TIMING42_SAR_ADC_TRIG_EN_TX_LO_MASK)
#define XCVR_TSM_TIMING42_SAR_ADC_TRIG_EN_RX_HI_MASK (0xFF0000U)
#define XCVR_TSM_TIMING42_SAR_ADC_TRIG_EN_RX_HI_SHIFT (16U)
#define XCVR_TSM_TIMING42_SAR_ADC_TRIG_EN_RX_HI(x) (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_TIMING42_SAR_ADC_TRIG_EN_RX_HI_SHIFT)) & XCVR_TSM_TIMING42_SAR_ADC_TRIG_EN_RX_HI_MASK)
#define XCVR_TSM_TIMING42_SAR_ADC_TRIG_EN_RX_LO_MASK (0xFF000000U)
#define XCVR_TSM_TIMING42_SAR_ADC_TRIG_EN_RX_LO_SHIFT (24U)
#define XCVR_TSM_TIMING42_SAR_ADC_TRIG_EN_RX_LO(x) (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_TIMING42_SAR_ADC_TRIG_EN_RX_LO_SHIFT)) & XCVR_TSM_TIMING42_SAR_ADC_TRIG_EN_RX_LO_MASK)

/*! @name TIMING43 - TSM_TIMING43 */
#define XCVR_TSM_TIMING43_TSM_SPARE0_EN_TX_HI_MASK (0xFFU)
#define XCVR_TSM_TIMING43_TSM_SPARE0_EN_TX_HI_SHIFT (0U)
#define XCVR_TSM_TIMING43_TSM_SPARE0_EN_TX_HI(x) (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_TIMING43_TSM_SPARE0_EN_TX_HI_SHIFT)) & XCVR_TSM_TIMING43_TSM_SPARE0_EN_TX_HI_MASK)
#define XCVR_TSM_TIMING43_TSM_SPARE0_EN_TX_LO_MASK (0xFF00U)
#define XCVR_TSM_TIMING43_TSM_SPARE0_EN_TX_LO_SHIFT (8U)
#define XCVR_TSM_TIMING43_TSM_SPARE0_EN_TX_LO(x) (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_TIMING43_TSM_SPARE0_EN_TX_LO_SHIFT)) & XCVR_TSM_TIMING43_TSM_SPARE0_EN_TX_LO_MASK)
#define XCVR_TSM_TIMING43_TSM_SPARE0_EN_RX_HI_MASK (0xFF0000U)
#define XCVR_TSM_TIMING43_TSM_SPARE0_EN_RX_HI_SHIFT (16U)
#define XCVR_TSM_TIMING43_TSM_SPARE0_EN_RX_HI(x) (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_TIMING43_TSM_SPARE0_EN_RX_HI_SHIFT)) & XCVR_TSM_TIMING43_TSM_SPARE0_EN_RX_HI_MASK)
#define XCVR_TSM_TIMING43_TSM_SPARE0_EN_RX_LO_MASK (0xFF000000U)
#define XCVR_TSM_TIMING43_TSM_SPARE0_EN_RX_LO_SHIFT (24U)
#define XCVR_TSM_TIMING43_TSM_SPARE0_EN_RX_LO(x) (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_TIMING43_TSM_SPARE0_EN_RX_LO_SHIFT)) & XCVR_TSM_TIMING43_TSM_SPARE0_EN_RX_LO_MASK)

/*! @name TIMING44 - TSM_TIMING44 */
#define XCVR_TSM_TIMING44_TSM_SPARE1_EN_TX_HI_MASK (0xFFU)
#define XCVR_TSM_TIMING44_TSM_SPARE1_EN_TX_HI_SHIFT (0U)
#define XCVR_TSM_TIMING44_TSM_SPARE1_EN_TX_HI(x) (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_TIMING44_TSM_SPARE1_EN_TX_HI_SHIFT)) & XCVR_TSM_TIMING44_TSM_SPARE1_EN_TX_HI_MASK)
#define XCVR_TSM_TIMING44_TSM_SPARE1_EN_TX_LO_MASK (0xFF00U)
#define XCVR_TSM_TIMING44_TSM_SPARE1_EN_TX_LO_SHIFT (8U)
#define XCVR_TSM_TIMING44_TSM_SPARE1_EN_TX_LO(x) (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_TIMING44_TSM_SPARE1_EN_TX_LO_SHIFT)) & XCVR_TSM_TIMING44_TSM_SPARE1_EN_TX_LO_MASK)
#define XCVR_TSM_TIMING44_TSM_SPARE1_EN_RX_HI_MASK (0xFF0000U)
#define XCVR_TSM_TIMING44_TSM_SPARE1_EN_RX_HI_SHIFT (16U)
#define XCVR_TSM_TIMING44_TSM_SPARE1_EN_RX_HI(x) (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_TIMING44_TSM_SPARE1_EN_RX_HI_SHIFT)) & XCVR_TSM_TIMING44_TSM_SPARE1_EN_RX_HI_MASK)
#define XCVR_TSM_TIMING44_TSM_SPARE1_EN_RX_LO_MASK (0xFF000000U)
#define XCVR_TSM_TIMING44_TSM_SPARE1_EN_RX_LO_SHIFT (24U)
#define XCVR_TSM_TIMING44_TSM_SPARE1_EN_RX_LO(x) (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_TIMING44_TSM_SPARE1_EN_RX_LO_SHIFT)) & XCVR_TSM_TIMING44_TSM_SPARE1_EN_RX_LO_MASK)

/*! @name TIMING45 - TSM_TIMING45 */
#define XCVR_TSM_TIMING45_TSM_SPARE2_EN_TX_HI_MASK (0xFFU)
#define XCVR_TSM_TIMING45_TSM_SPARE2_EN_TX_HI_SHIFT (0U)
#define XCVR_TSM_TIMING45_TSM_SPARE2_EN_TX_HI(x) (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_TIMING45_TSM_SPARE2_EN_TX_HI_SHIFT)) & XCVR_TSM_TIMING45_TSM_SPARE2_EN_TX_HI_MASK)
#define XCVR_TSM_TIMING45_TSM_SPARE2_EN_TX_LO_MASK (0xFF00U)
#define XCVR_TSM_TIMING45_TSM_SPARE2_EN_TX_LO_SHIFT (8U)
#define XCVR_TSM_TIMING45_TSM_SPARE2_EN_TX_LO(x) (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_TIMING45_TSM_SPARE2_EN_TX_LO_SHIFT)) & XCVR_TSM_TIMING45_TSM_SPARE2_EN_TX_LO_MASK)
#define XCVR_TSM_TIMING45_TSM_SPARE2_EN_RX_HI_MASK (0xFF0000U)
#define XCVR_TSM_TIMING45_TSM_SPARE2_EN_RX_HI_SHIFT (16U)
#define XCVR_TSM_TIMING45_TSM_SPARE2_EN_RX_HI(x) (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_TIMING45_TSM_SPARE2_EN_RX_HI_SHIFT)) & XCVR_TSM_TIMING45_TSM_SPARE2_EN_RX_HI_MASK)
#define XCVR_TSM_TIMING45_TSM_SPARE2_EN_RX_LO_MASK (0xFF000000U)
#define XCVR_TSM_TIMING45_TSM_SPARE2_EN_RX_LO_SHIFT (24U)
#define XCVR_TSM_TIMING45_TSM_SPARE2_EN_RX_LO(x) (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_TIMING45_TSM_SPARE2_EN_RX_LO_SHIFT)) & XCVR_TSM_TIMING45_TSM_SPARE2_EN_RX_LO_MASK)

/*! @name TIMING46 - TSM_TIMING46 */
#define XCVR_TSM_TIMING46_TSM_SPARE3_EN_TX_HI_MASK (0xFFU)
#define XCVR_TSM_TIMING46_TSM_SPARE3_EN_TX_HI_SHIFT (0U)
#define XCVR_TSM_TIMING46_TSM_SPARE3_EN_TX_HI(x) (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_TIMING46_TSM_SPARE3_EN_TX_HI_SHIFT)) & XCVR_TSM_TIMING46_TSM_SPARE3_EN_TX_HI_MASK)
#define XCVR_TSM_TIMING46_TSM_SPARE3_EN_TX_LO_MASK (0xFF00U)
#define XCVR_TSM_TIMING46_TSM_SPARE3_EN_TX_LO_SHIFT (8U)
#define XCVR_TSM_TIMING46_TSM_SPARE3_EN_TX_LO(x) (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_TIMING46_TSM_SPARE3_EN_TX_LO_SHIFT)) & XCVR_TSM_TIMING46_TSM_SPARE3_EN_TX_LO_MASK)
#define XCVR_TSM_TIMING46_TSM_SPARE3_EN_RX_HI_MASK (0xFF0000U)
#define XCVR_TSM_TIMING46_TSM_SPARE3_EN_RX_HI_SHIFT (16U)
#define XCVR_TSM_TIMING46_TSM_SPARE3_EN_RX_HI(x) (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_TIMING46_TSM_SPARE3_EN_RX_HI_SHIFT)) & XCVR_TSM_TIMING46_TSM_SPARE3_EN_RX_HI_MASK)
#define XCVR_TSM_TIMING46_TSM_SPARE3_EN_RX_LO_MASK (0xFF000000U)
#define XCVR_TSM_TIMING46_TSM_SPARE3_EN_RX_LO_SHIFT (24U)
#define XCVR_TSM_TIMING46_TSM_SPARE3_EN_RX_LO(x) (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_TIMING46_TSM_SPARE3_EN_RX_LO_SHIFT)) & XCVR_TSM_TIMING46_TSM_SPARE3_EN_RX_LO_MASK)

/*! @name TIMING47 - TSM_TIMING47 */
#define XCVR_TSM_TIMING47_GPIO0_TRIG_EN_TX_HI_MASK (0xFFU)
#define XCVR_TSM_TIMING47_GPIO0_TRIG_EN_TX_HI_SHIFT (0U)
#define XCVR_TSM_TIMING47_GPIO0_TRIG_EN_TX_HI(x) (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_TIMING47_GPIO0_TRIG_EN_TX_HI_SHIFT)) & XCVR_TSM_TIMING47_GPIO0_TRIG_EN_TX_HI_MASK)
#define XCVR_TSM_TIMING47_GPIO0_TRIG_EN_TX_LO_MASK (0xFF00U)
#define XCVR_TSM_TIMING47_GPIO0_TRIG_EN_TX_LO_SHIFT (8U)
#define XCVR_TSM_TIMING47_GPIO0_TRIG_EN_TX_LO(x) (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_TIMING47_GPIO0_TRIG_EN_TX_LO_SHIFT)) & XCVR_TSM_TIMING47_GPIO0_TRIG_EN_TX_LO_MASK)
#define XCVR_TSM_TIMING47_GPIO0_TRIG_EN_RX_HI_MASK (0xFF0000U)
#define XCVR_TSM_TIMING47_GPIO0_TRIG_EN_RX_HI_SHIFT (16U)
#define XCVR_TSM_TIMING47_GPIO0_TRIG_EN_RX_HI(x) (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_TIMING47_GPIO0_TRIG_EN_RX_HI_SHIFT)) & XCVR_TSM_TIMING47_GPIO0_TRIG_EN_RX_HI_MASK)
#define XCVR_TSM_TIMING47_GPIO0_TRIG_EN_RX_LO_MASK (0xFF000000U)
#define XCVR_TSM_TIMING47_GPIO0_TRIG_EN_RX_LO_SHIFT (24U)
#define XCVR_TSM_TIMING47_GPIO0_TRIG_EN_RX_LO(x) (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_TIMING47_GPIO0_TRIG_EN_RX_LO_SHIFT)) & XCVR_TSM_TIMING47_GPIO0_TRIG_EN_RX_LO_MASK)

/*! @name TIMING48 - TSM_TIMING48 */
#define XCVR_TSM_TIMING48_GPIO1_TRIG_EN_TX_HI_MASK (0xFFU)
#define XCVR_TSM_TIMING48_GPIO1_TRIG_EN_TX_HI_SHIFT (0U)
#define XCVR_TSM_TIMING48_GPIO1_TRIG_EN_TX_HI(x) (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_TIMING48_GPIO1_TRIG_EN_TX_HI_SHIFT)) & XCVR_TSM_TIMING48_GPIO1_TRIG_EN_TX_HI_MASK)
#define XCVR_TSM_TIMING48_GPIO1_TRIG_EN_TX_LO_MASK (0xFF00U)
#define XCVR_TSM_TIMING48_GPIO1_TRIG_EN_TX_LO_SHIFT (8U)
#define XCVR_TSM_TIMING48_GPIO1_TRIG_EN_TX_LO(x) (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_TIMING48_GPIO1_TRIG_EN_TX_LO_SHIFT)) & XCVR_TSM_TIMING48_GPIO1_TRIG_EN_TX_LO_MASK)
#define XCVR_TSM_TIMING48_GPIO1_TRIG_EN_RX_HI_MASK (0xFF0000U)
#define XCVR_TSM_TIMING48_GPIO1_TRIG_EN_RX_HI_SHIFT (16U)
#define XCVR_TSM_TIMING48_GPIO1_TRIG_EN_RX_HI(x) (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_TIMING48_GPIO1_TRIG_EN_RX_HI_SHIFT)) & XCVR_TSM_TIMING48_GPIO1_TRIG_EN_RX_HI_MASK)
#define XCVR_TSM_TIMING48_GPIO1_TRIG_EN_RX_LO_MASK (0xFF000000U)
#define XCVR_TSM_TIMING48_GPIO1_TRIG_EN_RX_LO_SHIFT (24U)
#define XCVR_TSM_TIMING48_GPIO1_TRIG_EN_RX_LO(x) (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_TIMING48_GPIO1_TRIG_EN_RX_LO_SHIFT)) & XCVR_TSM_TIMING48_GPIO1_TRIG_EN_RX_LO_MASK)

/*! @name TIMING49 - TSM_TIMING49 */
#define XCVR_TSM_TIMING49_GPIO2_TRIG_EN_TX_HI_MASK (0xFFU)
#define XCVR_TSM_TIMING49_GPIO2_TRIG_EN_TX_HI_SHIFT (0U)
#define XCVR_TSM_TIMING49_GPIO2_TRIG_EN_TX_HI(x) (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_TIMING49_GPIO2_TRIG_EN_TX_HI_SHIFT)) & XCVR_TSM_TIMING49_GPIO2_TRIG_EN_TX_HI_MASK)
#define XCVR_TSM_TIMING49_GPIO2_TRIG_EN_TX_LO_MASK (0xFF00U)
#define XCVR_TSM_TIMING49_GPIO2_TRIG_EN_TX_LO_SHIFT (8U)
#define XCVR_TSM_TIMING49_GPIO2_TRIG_EN_TX_LO(x) (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_TIMING49_GPIO2_TRIG_EN_TX_LO_SHIFT)) & XCVR_TSM_TIMING49_GPIO2_TRIG_EN_TX_LO_MASK)
#define XCVR_TSM_TIMING49_GPIO2_TRIG_EN_RX_HI_MASK (0xFF0000U)
#define XCVR_TSM_TIMING49_GPIO2_TRIG_EN_RX_HI_SHIFT (16U)
#define XCVR_TSM_TIMING49_GPIO2_TRIG_EN_RX_HI(x) (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_TIMING49_GPIO2_TRIG_EN_RX_HI_SHIFT)) & XCVR_TSM_TIMING49_GPIO2_TRIG_EN_RX_HI_MASK)
#define XCVR_TSM_TIMING49_GPIO2_TRIG_EN_RX_LO_MASK (0xFF000000U)
#define XCVR_TSM_TIMING49_GPIO2_TRIG_EN_RX_LO_SHIFT (24U)
#define XCVR_TSM_TIMING49_GPIO2_TRIG_EN_RX_LO(x) (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_TIMING49_GPIO2_TRIG_EN_RX_LO_SHIFT)) & XCVR_TSM_TIMING49_GPIO2_TRIG_EN_RX_LO_MASK)

/*! @name TIMING50 - TSM_TIMING50 */
#define XCVR_TSM_TIMING50_GPIO3_TRIG_EN_TX_HI_MASK (0xFFU)
#define XCVR_TSM_TIMING50_GPIO3_TRIG_EN_TX_HI_SHIFT (0U)
#define XCVR_TSM_TIMING50_GPIO3_TRIG_EN_TX_HI(x) (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_TIMING50_GPIO3_TRIG_EN_TX_HI_SHIFT)) & XCVR_TSM_TIMING50_GPIO3_TRIG_EN_TX_HI_MASK)
#define XCVR_TSM_TIMING50_GPIO3_TRIG_EN_TX_LO_MASK (0xFF00U)
#define XCVR_TSM_TIMING50_GPIO3_TRIG_EN_TX_LO_SHIFT (8U)
#define XCVR_TSM_TIMING50_GPIO3_TRIG_EN_TX_LO(x) (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_TIMING50_GPIO3_TRIG_EN_TX_LO_SHIFT)) & XCVR_TSM_TIMING50_GPIO3_TRIG_EN_TX_LO_MASK)
#define XCVR_TSM_TIMING50_GPIO3_TRIG_EN_RX_HI_MASK (0xFF0000U)
#define XCVR_TSM_TIMING50_GPIO3_TRIG_EN_RX_HI_SHIFT (16U)
#define XCVR_TSM_TIMING50_GPIO3_TRIG_EN_RX_HI(x) (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_TIMING50_GPIO3_TRIG_EN_RX_HI_SHIFT)) & XCVR_TSM_TIMING50_GPIO3_TRIG_EN_RX_HI_MASK)
#define XCVR_TSM_TIMING50_GPIO3_TRIG_EN_RX_LO_MASK (0xFF000000U)
#define XCVR_TSM_TIMING50_GPIO3_TRIG_EN_RX_LO_SHIFT (24U)
#define XCVR_TSM_TIMING50_GPIO3_TRIG_EN_RX_LO(x) (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_TIMING50_GPIO3_TRIG_EN_RX_LO_SHIFT)) & XCVR_TSM_TIMING50_GPIO3_TRIG_EN_RX_LO_MASK)

/*! @name TIMING51 - TSM_TIMING51 */
#define XCVR_TSM_TIMING51_RXTX_AUXPLL_BIAS_EN_RX_HI_MASK (0xFF0000U)
#define XCVR_TSM_TIMING51_RXTX_AUXPLL_BIAS_EN_RX_HI_SHIFT (16U)
#define XCVR_TSM_TIMING51_RXTX_AUXPLL_BIAS_EN_RX_HI(x) (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_TIMING51_RXTX_AUXPLL_BIAS_EN_RX_HI_SHIFT)) & XCVR_TSM_TIMING51_RXTX_AUXPLL_BIAS_EN_RX_HI_MASK)
#define XCVR_TSM_TIMING51_RXTX_AUXPLL_BIAS_EN_RX_LO_MASK (0xFF000000U)
#define XCVR_TSM_TIMING51_RXTX_AUXPLL_BIAS_EN_RX_LO_SHIFT (24U)
#define XCVR_TSM_TIMING51_RXTX_AUXPLL_BIAS_EN_RX_LO(x) (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_TIMING51_RXTX_AUXPLL_BIAS_EN_RX_LO_SHIFT)) & XCVR_TSM_TIMING51_RXTX_AUXPLL_BIAS_EN_RX_LO_MASK)

/*! @name TIMING52 - TSM_TIMING52 */
#define XCVR_TSM_TIMING52_RXTX_AUXPLL_FCAL_EN_RX_HI_MASK (0xFF0000U)
#define XCVR_TSM_TIMING52_RXTX_AUXPLL_FCAL_EN_RX_HI_SHIFT (16U)
#define XCVR_TSM_TIMING52_RXTX_AUXPLL_FCAL_EN_RX_HI(x) (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_TIMING52_RXTX_AUXPLL_FCAL_EN_RX_HI_SHIFT)) & XCVR_TSM_TIMING52_RXTX_AUXPLL_FCAL_EN_RX_HI_MASK)
#define XCVR_TSM_TIMING52_RXTX_AUXPLL_FCAL_EN_RX_LO_MASK (0xFF000000U)
#define XCVR_TSM_TIMING52_RXTX_AUXPLL_FCAL_EN_RX_LO_SHIFT (24U)
#define XCVR_TSM_TIMING52_RXTX_AUXPLL_FCAL_EN_RX_LO(x) (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_TIMING52_RXTX_AUXPLL_FCAL_EN_RX_LO_SHIFT)) & XCVR_TSM_TIMING52_RXTX_AUXPLL_FCAL_EN_RX_LO_MASK)

/*! @name TIMING53 - TSM_TIMING53 */
#define XCVR_TSM_TIMING53_RXTX_AUXPLL_LF_PD_EN_RX_HI_MASK (0xFF0000U)
#define XCVR_TSM_TIMING53_RXTX_AUXPLL_LF_PD_EN_RX_HI_SHIFT (16U)
#define XCVR_TSM_TIMING53_RXTX_AUXPLL_LF_PD_EN_RX_HI(x) (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_TIMING53_RXTX_AUXPLL_LF_PD_EN_RX_HI_SHIFT)) & XCVR_TSM_TIMING53_RXTX_AUXPLL_LF_PD_EN_RX_HI_MASK)
#define XCVR_TSM_TIMING53_RXTX_AUXPLL_LF_PD_EN_RX_LO_MASK (0xFF000000U)
#define XCVR_TSM_TIMING53_RXTX_AUXPLL_LF_PD_EN_RX_LO_SHIFT (24U)
#define XCVR_TSM_TIMING53_RXTX_AUXPLL_LF_PD_EN_RX_LO(x) (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_TIMING53_RXTX_AUXPLL_LF_PD_EN_RX_LO_SHIFT)) & XCVR_TSM_TIMING53_RXTX_AUXPLL_LF_PD_EN_RX_LO_MASK)

/*! @name TIMING54 - TSM_TIMING54 */
#define XCVR_TSM_TIMING54_RXTX_AUXPLL_PD_LF_FILTER_CHARGE_EN_RX_HI_MASK (0xFF0000U)
#define XCVR_TSM_TIMING54_RXTX_AUXPLL_PD_LF_FILTER_CHARGE_EN_RX_HI_SHIFT (16U)
#define XCVR_TSM_TIMING54_RXTX_AUXPLL_PD_LF_FILTER_CHARGE_EN_RX_HI(x) (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_TIMING54_RXTX_AUXPLL_PD_LF_FILTER_CHARGE_EN_RX_HI_SHIFT)) & XCVR_TSM_TIMING54_RXTX_AUXPLL_PD_LF_FILTER_CHARGE_EN_RX_HI_MASK)
#define XCVR_TSM_TIMING54_RXTX_AUXPLL_PD_LF_FILTER_CHARGE_EN_RX_LO_MASK (0xFF000000U)
#define XCVR_TSM_TIMING54_RXTX_AUXPLL_PD_LF_FILTER_CHARGE_EN_RX_LO_SHIFT (24U)
#define XCVR_TSM_TIMING54_RXTX_AUXPLL_PD_LF_FILTER_CHARGE_EN_RX_LO(x) (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_TIMING54_RXTX_AUXPLL_PD_LF_FILTER_CHARGE_EN_RX_LO_SHIFT)) & XCVR_TSM_TIMING54_RXTX_AUXPLL_PD_LF_FILTER_CHARGE_EN_RX_LO_MASK)

/*! @name TIMING55 - TSM_TIMING55 */
#define XCVR_TSM_TIMING55_RXTX_AUXPLL_ADC_BUF_EN_RX_HI_MASK (0xFF0000U)
#define XCVR_TSM_TIMING55_RXTX_AUXPLL_ADC_BUF_EN_RX_HI_SHIFT (16U)
#define XCVR_TSM_TIMING55_RXTX_AUXPLL_ADC_BUF_EN_RX_HI(x) (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_TIMING55_RXTX_AUXPLL_ADC_BUF_EN_RX_HI_SHIFT)) & XCVR_TSM_TIMING55_RXTX_AUXPLL_ADC_BUF_EN_RX_HI_MASK)
#define XCVR_TSM_TIMING55_RXTX_AUXPLL_ADC_BUF_EN_RX_LO_MASK (0xFF000000U)
#define XCVR_TSM_TIMING55_RXTX_AUXPLL_ADC_BUF_EN_RX_LO_SHIFT (24U)
#define XCVR_TSM_TIMING55_RXTX_AUXPLL_ADC_BUF_EN_RX_LO(x) (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_TIMING55_RXTX_AUXPLL_ADC_BUF_EN_RX_LO_SHIFT)) & XCVR_TSM_TIMING55_RXTX_AUXPLL_ADC_BUF_EN_RX_LO_MASK)

/*! @name TIMING56 - TSM_TIMING56 */
#define XCVR_TSM_TIMING56_RXTX_AUXPLL_DIG_BUF_EN_RX_HI_MASK (0xFF0000U)
#define XCVR_TSM_TIMING56_RXTX_AUXPLL_DIG_BUF_EN_RX_HI_SHIFT (16U)
#define XCVR_TSM_TIMING56_RXTX_AUXPLL_DIG_BUF_EN_RX_HI(x) (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_TIMING56_RXTX_AUXPLL_DIG_BUF_EN_RX_HI_SHIFT)) & XCVR_TSM_TIMING56_RXTX_AUXPLL_DIG_BUF_EN_RX_HI_MASK)
#define XCVR_TSM_TIMING56_RXTX_AUXPLL_DIG_BUF_EN_RX_LO_MASK (0xFF000000U)
#define XCVR_TSM_TIMING56_RXTX_AUXPLL_DIG_BUF_EN_RX_LO_SHIFT (24U)
#define XCVR_TSM_TIMING56_RXTX_AUXPLL_DIG_BUF_EN_RX_LO(x) (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_TIMING56_RXTX_AUXPLL_DIG_BUF_EN_RX_LO_SHIFT)) & XCVR_TSM_TIMING56_RXTX_AUXPLL_DIG_BUF_EN_RX_LO_MASK)

/*! @name TIMING57 - TSM_TIMING57 */
#define XCVR_TSM_TIMING57_RXTX_RCCAL_EN_RX_HI_MASK (0xFF0000U)
#define XCVR_TSM_TIMING57_RXTX_RCCAL_EN_RX_HI_SHIFT (16U)
#define XCVR_TSM_TIMING57_RXTX_RCCAL_EN_RX_HI(x) (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_TIMING57_RXTX_RCCAL_EN_RX_HI_SHIFT)) & XCVR_TSM_TIMING57_RXTX_RCCAL_EN_RX_HI_MASK)
#define XCVR_TSM_TIMING57_RXTX_RCCAL_EN_RX_LO_MASK (0xFF000000U)
#define XCVR_TSM_TIMING57_RXTX_RCCAL_EN_RX_LO_SHIFT (24U)
#define XCVR_TSM_TIMING57_RXTX_RCCAL_EN_RX_LO(x) (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_TIMING57_RXTX_RCCAL_EN_RX_LO_SHIFT)) & XCVR_TSM_TIMING57_RXTX_RCCAL_EN_RX_LO_MASK)

/*! @name TIMING58 - TSM_TIMING58 */
#define XCVR_TSM_TIMING58_TX_HPM_DAC_EN_TX_HI_MASK (0xFFU)
#define XCVR_TSM_TIMING58_TX_HPM_DAC_EN_TX_HI_SHIFT (0U)
#define XCVR_TSM_TIMING58_TX_HPM_DAC_EN_TX_HI(x) (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_TIMING58_TX_HPM_DAC_EN_TX_HI_SHIFT)) & XCVR_TSM_TIMING58_TX_HPM_DAC_EN_TX_HI_MASK)
#define XCVR_TSM_TIMING58_TX_HPM_DAC_EN_TX_LO_MASK (0xFF00U)
#define XCVR_TSM_TIMING58_TX_HPM_DAC_EN_TX_LO_SHIFT (8U)
#define XCVR_TSM_TIMING58_TX_HPM_DAC_EN_TX_LO(x) (((uint32_t)(((uint32_t)(x)) << XCVR_TSM_TIMING58_TX_HPM_DAC_EN_TX_LO_SHIFT)) & XCVR_TSM_TIMING58_TX_HPM_DAC_EN_TX_LO_MASK)


/*!
 * @}
 */ /* end of group XCVR_TSM_Register_Masks */


/* XCVR_TSM - Peripheral instance base addresses */
/** Peripheral XCVR_TSM base address */
#define XCVR_TSM_BASE                            (0x4005C2C0u)
/** Peripheral XCVR_TSM base pointer */
#define XCVR_TSM                                 ((XCVR_TSM_Type *)XCVR_TSM_BASE)
/** Array initializer of XCVR_TSM peripheral base addresses */
#define XCVR_TSM_BASE_ADDRS                      { XCVR_TSM_BASE }
/** Array initializer of XCVR_TSM peripheral base pointers */
#define XCVR_TSM_BASE_PTRS                       { XCVR_TSM }

/*!
 * @}
 */ /* end of group XCVR_TSM_Peripheral_Access_Layer */


/* ----------------------------------------------------------------------------
   -- XCVR_TX_DIG Peripheral Access Layer
   ---------------------------------------------------------------------------- */

/*!
 * @addtogroup XCVR_TX_DIG_Peripheral_Access_Layer XCVR_TX_DIG Peripheral Access Layer
 * @{
 */

/** XCVR_TX_DIG - Register Layout Typedef */
typedef struct {
  __IO uint32_t CTRL;                              /**< TX Digital Control, offset: 0x0 */
  __IO uint32_t DATA_PADDING;                      /**< TX Data Padding, offset: 0x4 */
  __IO uint32_t GFSK_CTRL;                         /**< TX GFSK Modulator Control, offset: 0x8 */
  __IO uint32_t GFSK_COEFF2;                       /**< TX GFSK Filter Coefficients 2, offset: 0xC */
  __IO uint32_t GFSK_COEFF1;                       /**< TX GFSK Filter Coefficients 1, offset: 0x10 */
  __IO uint32_t FSK_SCALE;                         /**< TX FSK Modulation Levels, offset: 0x14 */
  __IO uint32_t DFT_PATTERN;                       /**< TX DFT Modulation Pattern, offset: 0x18 */
  __IO uint32_t RF_DFT_BIST_1;                     /**< TX DFT Control 1, offset: 0x1C */
  __IO uint32_t RF_DFT_BIST_2;                     /**< TX DFT Control 2, offset: 0x20 */
} XCVR_TX_DIG_Type;

/* ----------------------------------------------------------------------------
   -- XCVR_TX_DIG Register Masks
   ---------------------------------------------------------------------------- */

/*!
 * @addtogroup XCVR_TX_DIG_Register_Masks XCVR_TX_DIG Register Masks
 * @{
 */

/*! @name CTRL - TX Digital Control */
#define XCVR_TX_DIG_CTRL_RADIO_DFT_MODE_MASK     (0xFU)
#define XCVR_TX_DIG_CTRL_RADIO_DFT_MODE_SHIFT    (0U)
#define XCVR_TX_DIG_CTRL_RADIO_DFT_MODE(x)       (((uint32_t)(((uint32_t)(x)) << XCVR_TX_DIG_CTRL_RADIO_DFT_MODE_SHIFT)) & XCVR_TX_DIG_CTRL_RADIO_DFT_MODE_MASK)
#define XCVR_TX_DIG_CTRL_LFSR_LENGTH_MASK        (0x70U)
#define XCVR_TX_DIG_CTRL_LFSR_LENGTH_SHIFT       (4U)
#define XCVR_TX_DIG_CTRL_LFSR_LENGTH(x)          (((uint32_t)(((uint32_t)(x)) << XCVR_TX_DIG_CTRL_LFSR_LENGTH_SHIFT)) & XCVR_TX_DIG_CTRL_LFSR_LENGTH_MASK)
#define XCVR_TX_DIG_CTRL_LFSR_EN_MASK            (0x80U)
#define XCVR_TX_DIG_CTRL_LFSR_EN_SHIFT           (7U)
#define XCVR_TX_DIG_CTRL_LFSR_EN(x)              (((uint32_t)(((uint32_t)(x)) << XCVR_TX_DIG_CTRL_LFSR_EN_SHIFT)) & XCVR_TX_DIG_CTRL_LFSR_EN_MASK)
#define XCVR_TX_DIG_CTRL_DFT_CLK_SEL_MASK        (0x700U)
#define XCVR_TX_DIG_CTRL_DFT_CLK_SEL_SHIFT       (8U)
#define XCVR_TX_DIG_CTRL_DFT_CLK_SEL(x)          (((uint32_t)(((uint32_t)(x)) << XCVR_TX_DIG_CTRL_DFT_CLK_SEL_SHIFT)) & XCVR_TX_DIG_CTRL_DFT_CLK_SEL_MASK)
#define XCVR_TX_DIG_CTRL_TX_DFT_EN_MASK          (0x800U)
#define XCVR_TX_DIG_CTRL_TX_DFT_EN_SHIFT         (11U)
#define XCVR_TX_DIG_CTRL_TX_DFT_EN(x)            (((uint32_t)(((uint32_t)(x)) << XCVR_TX_DIG_CTRL_TX_DFT_EN_SHIFT)) & XCVR_TX_DIG_CTRL_TX_DFT_EN_MASK)
#define XCVR_TX_DIG_CTRL_SOC_TEST_SEL_MASK       (0x3000U)
#define XCVR_TX_DIG_CTRL_SOC_TEST_SEL_SHIFT      (12U)
#define XCVR_TX_DIG_CTRL_SOC_TEST_SEL(x)         (((uint32_t)(((uint32_t)(x)) << XCVR_TX_DIG_CTRL_SOC_TEST_SEL_SHIFT)) & XCVR_TX_DIG_CTRL_SOC_TEST_SEL_MASK)
#define XCVR_TX_DIG_CTRL_TX_CAPTURE_POL_MASK     (0x10000U)
#define XCVR_TX_DIG_CTRL_TX_CAPTURE_POL_SHIFT    (16U)
#define XCVR_TX_DIG_CTRL_TX_CAPTURE_POL(x)       (((uint32_t)(((uint32_t)(x)) << XCVR_TX_DIG_CTRL_TX_CAPTURE_POL_SHIFT)) & XCVR_TX_DIG_CTRL_TX_CAPTURE_POL_MASK)
#define XCVR_TX_DIG_CTRL_FREQ_WORD_ADJ_MASK      (0xFFC00000U)
#define XCVR_TX_DIG_CTRL_FREQ_WORD_ADJ_SHIFT     (22U)
#define XCVR_TX_DIG_CTRL_FREQ_WORD_ADJ(x)        (((uint32_t)(((uint32_t)(x)) << XCVR_TX_DIG_CTRL_FREQ_WORD_ADJ_SHIFT)) & XCVR_TX_DIG_CTRL_FREQ_WORD_ADJ_MASK)

/*! @name DATA_PADDING - TX Data Padding */
#define XCVR_TX_DIG_DATA_PADDING_DATA_PADDING_PAT_0_MASK (0xFFU)
#define XCVR_TX_DIG_DATA_PADDING_DATA_PADDING_PAT_0_SHIFT (0U)
#define XCVR_TX_DIG_DATA_PADDING_DATA_PADDING_PAT_0(x) (((uint32_t)(((uint32_t)(x)) << XCVR_TX_DIG_DATA_PADDING_DATA_PADDING_PAT_0_SHIFT)) & XCVR_TX_DIG_DATA_PADDING_DATA_PADDING_PAT_0_MASK)
#define XCVR_TX_DIG_DATA_PADDING_DATA_PADDING_PAT_1_MASK (0xFF00U)
#define XCVR_TX_DIG_DATA_PADDING_DATA_PADDING_PAT_1_SHIFT (8U)
#define XCVR_TX_DIG_DATA_PADDING_DATA_PADDING_PAT_1(x) (((uint32_t)(((uint32_t)(x)) << XCVR_TX_DIG_DATA_PADDING_DATA_PADDING_PAT_1_SHIFT)) & XCVR_TX_DIG_DATA_PADDING_DATA_PADDING_PAT_1_MASK)
#define XCVR_TX_DIG_DATA_PADDING_DFT_LFSR_OUT_MASK (0x7FFF0000U)
#define XCVR_TX_DIG_DATA_PADDING_DFT_LFSR_OUT_SHIFT (16U)
#define XCVR_TX_DIG_DATA_PADDING_DFT_LFSR_OUT(x) (((uint32_t)(((uint32_t)(x)) << XCVR_TX_DIG_DATA_PADDING_DFT_LFSR_OUT_SHIFT)) & XCVR_TX_DIG_DATA_PADDING_DFT_LFSR_OUT_MASK)
#define XCVR_TX_DIG_DATA_PADDING_LRM_MASK        (0x80000000U)
#define XCVR_TX_DIG_DATA_PADDING_LRM_SHIFT       (31U)
#define XCVR_TX_DIG_DATA_PADDING_LRM(x)          (((uint32_t)(((uint32_t)(x)) << XCVR_TX_DIG_DATA_PADDING_LRM_SHIFT)) & XCVR_TX_DIG_DATA_PADDING_LRM_MASK)

/*! @name GFSK_CTRL - TX GFSK Modulator Control */
#define XCVR_TX_DIG_GFSK_CTRL_GFSK_MULTIPLY_TABLE_MANUAL_MASK (0xFFFFU)
#define XCVR_TX_DIG_GFSK_CTRL_GFSK_MULTIPLY_TABLE_MANUAL_SHIFT (0U)
#define XCVR_TX_DIG_GFSK_CTRL_GFSK_MULTIPLY_TABLE_MANUAL(x) (((uint32_t)(((uint32_t)(x)) << XCVR_TX_DIG_GFSK_CTRL_GFSK_MULTIPLY_TABLE_MANUAL_SHIFT)) & XCVR_TX_DIG_GFSK_CTRL_GFSK_MULTIPLY_TABLE_MANUAL_MASK)
#define XCVR_TX_DIG_GFSK_CTRL_GFSK_MI_MASK       (0x30000U)
#define XCVR_TX_DIG_GFSK_CTRL_GFSK_MI_SHIFT      (16U)
#define XCVR_TX_DIG_GFSK_CTRL_GFSK_MI(x)         (((uint32_t)(((uint32_t)(x)) << XCVR_TX_DIG_GFSK_CTRL_GFSK_MI_SHIFT)) & XCVR_TX_DIG_GFSK_CTRL_GFSK_MI_MASK)
#define XCVR_TX_DIG_GFSK_CTRL_GFSK_MLD_MASK      (0x100000U)
#define XCVR_TX_DIG_GFSK_CTRL_GFSK_MLD_SHIFT     (20U)
#define XCVR_TX_DIG_GFSK_CTRL_GFSK_MLD(x)        (((uint32_t)(((uint32_t)(x)) << XCVR_TX_DIG_GFSK_CTRL_GFSK_MLD_SHIFT)) & XCVR_TX_DIG_GFSK_CTRL_GFSK_MLD_MASK)
#define XCVR_TX_DIG_GFSK_CTRL_GFSK_FLD_MASK      (0x200000U)
#define XCVR_TX_DIG_GFSK_CTRL_GFSK_FLD_SHIFT     (21U)
#define XCVR_TX_DIG_GFSK_CTRL_GFSK_FLD(x)        (((uint32_t)(((uint32_t)(x)) << XCVR_TX_DIG_GFSK_CTRL_GFSK_FLD_SHIFT)) & XCVR_TX_DIG_GFSK_CTRL_GFSK_FLD_MASK)
#define XCVR_TX_DIG_GFSK_CTRL_GFSK_MOD_INDEX_SCALING_MASK (0x7000000U)
#define XCVR_TX_DIG_GFSK_CTRL_GFSK_MOD_INDEX_SCALING_SHIFT (24U)
#define XCVR_TX_DIG_GFSK_CTRL_GFSK_MOD_INDEX_SCALING(x) (((uint32_t)(((uint32_t)(x)) << XCVR_TX_DIG_GFSK_CTRL_GFSK_MOD_INDEX_SCALING_SHIFT)) & XCVR_TX_DIG_GFSK_CTRL_GFSK_MOD_INDEX_SCALING_MASK)
#define XCVR_TX_DIG_GFSK_CTRL_TX_IMAGE_FILTER_OVRD_EN_MASK (0x10000000U)
#define XCVR_TX_DIG_GFSK_CTRL_TX_IMAGE_FILTER_OVRD_EN_SHIFT (28U)
#define XCVR_TX_DIG_GFSK_CTRL_TX_IMAGE_FILTER_OVRD_EN(x) (((uint32_t)(((uint32_t)(x)) << XCVR_TX_DIG_GFSK_CTRL_TX_IMAGE_FILTER_OVRD_EN_SHIFT)) & XCVR_TX_DIG_GFSK_CTRL_TX_IMAGE_FILTER_OVRD_EN_MASK)
#define XCVR_TX_DIG_GFSK_CTRL_TX_IMAGE_FILTER_0_OVRD_MASK (0x20000000U)
#define XCVR_TX_DIG_GFSK_CTRL_TX_IMAGE_FILTER_0_OVRD_SHIFT (29U)
#define XCVR_TX_DIG_GFSK_CTRL_TX_IMAGE_FILTER_0_OVRD(x) (((uint32_t)(((uint32_t)(x)) << XCVR_TX_DIG_GFSK_CTRL_TX_IMAGE_FILTER_0_OVRD_SHIFT)) & XCVR_TX_DIG_GFSK_CTRL_TX_IMAGE_FILTER_0_OVRD_MASK)
#define XCVR_TX_DIG_GFSK_CTRL_TX_IMAGE_FILTER_1_OVRD_MASK (0x40000000U)
#define XCVR_TX_DIG_GFSK_CTRL_TX_IMAGE_FILTER_1_OVRD_SHIFT (30U)
#define XCVR_TX_DIG_GFSK_CTRL_TX_IMAGE_FILTER_1_OVRD(x) (((uint32_t)(((uint32_t)(x)) << XCVR_TX_DIG_GFSK_CTRL_TX_IMAGE_FILTER_1_OVRD_SHIFT)) & XCVR_TX_DIG_GFSK_CTRL_TX_IMAGE_FILTER_1_OVRD_MASK)
#define XCVR_TX_DIG_GFSK_CTRL_TX_IMAGE_FILTER_2_OVRD_MASK (0x80000000U)
#define XCVR_TX_DIG_GFSK_CTRL_TX_IMAGE_FILTER_2_OVRD_SHIFT (31U)
#define XCVR_TX_DIG_GFSK_CTRL_TX_IMAGE_FILTER_2_OVRD(x) (((uint32_t)(((uint32_t)(x)) << XCVR_TX_DIG_GFSK_CTRL_TX_IMAGE_FILTER_2_OVRD_SHIFT)) & XCVR_TX_DIG_GFSK_CTRL_TX_IMAGE_FILTER_2_OVRD_MASK)

/*! @name GFSK_COEFF2 - TX GFSK Filter Coefficients 2 */
#define XCVR_TX_DIG_GFSK_COEFF2_GFSK_FILTER_COEFF_MANUAL2_MASK (0xFFFFFFFFU)
#define XCVR_TX_DIG_GFSK_COEFF2_GFSK_FILTER_COEFF_MANUAL2_SHIFT (0U)
#define XCVR_TX_DIG_GFSK_COEFF2_GFSK_FILTER_COEFF_MANUAL2(x) (((uint32_t)(((uint32_t)(x)) << XCVR_TX_DIG_GFSK_COEFF2_GFSK_FILTER_COEFF_MANUAL2_SHIFT)) & XCVR_TX_DIG_GFSK_COEFF2_GFSK_FILTER_COEFF_MANUAL2_MASK)

/*! @name GFSK_COEFF1 - TX GFSK Filter Coefficients 1 */
#define XCVR_TX_DIG_GFSK_COEFF1_GFSK_FILTER_COEFF_MANUAL1_MASK (0xFFFFFFFFU)
#define XCVR_TX_DIG_GFSK_COEFF1_GFSK_FILTER_COEFF_MANUAL1_SHIFT (0U)
#define XCVR_TX_DIG_GFSK_COEFF1_GFSK_FILTER_COEFF_MANUAL1(x) (((uint32_t)(((uint32_t)(x)) << XCVR_TX_DIG_GFSK_COEFF1_GFSK_FILTER_COEFF_MANUAL1_SHIFT)) & XCVR_TX_DIG_GFSK_COEFF1_GFSK_FILTER_COEFF_MANUAL1_MASK)

/*! @name FSK_SCALE - TX FSK Modulation Levels */
#define XCVR_TX_DIG_FSK_SCALE_FSK_MODULATION_SCALE_0_MASK (0x1FFFU)
#define XCVR_TX_DIG_FSK_SCALE_FSK_MODULATION_SCALE_0_SHIFT (0U)
#define XCVR_TX_DIG_FSK_SCALE_FSK_MODULATION_SCALE_0(x) (((uint32_t)(((uint32_t)(x)) << XCVR_TX_DIG_FSK_SCALE_FSK_MODULATION_SCALE_0_SHIFT)) & XCVR_TX_DIG_FSK_SCALE_FSK_MODULATION_SCALE_0_MASK)
#define XCVR_TX_DIG_FSK_SCALE_FSK_MODULATION_SCALE_1_MASK (0x1FFF0000U)
#define XCVR_TX_DIG_FSK_SCALE_FSK_MODULATION_SCALE_1_SHIFT (16U)
#define XCVR_TX_DIG_FSK_SCALE_FSK_MODULATION_SCALE_1(x) (((uint32_t)(((uint32_t)(x)) << XCVR_TX_DIG_FSK_SCALE_FSK_MODULATION_SCALE_1_SHIFT)) & XCVR_TX_DIG_FSK_SCALE_FSK_MODULATION_SCALE_1_MASK)

/*! @name DFT_PATTERN - TX DFT Modulation Pattern */
#define XCVR_TX_DIG_DFT_PATTERN_DFT_MOD_PATTERN_MASK (0xFFFFFFFFU)
#define XCVR_TX_DIG_DFT_PATTERN_DFT_MOD_PATTERN_SHIFT (0U)
#define XCVR_TX_DIG_DFT_PATTERN_DFT_MOD_PATTERN(x) (((uint32_t)(((uint32_t)(x)) << XCVR_TX_DIG_DFT_PATTERN_DFT_MOD_PATTERN_SHIFT)) & XCVR_TX_DIG_DFT_PATTERN_DFT_MOD_PATTERN_MASK)

/*! @name RF_DFT_BIST_1 - TX DFT Control 1 */
#define XCVR_TX_DIG_RF_DFT_BIST_1_CTUNE_BIST_GO_MASK (0x1U)
#define XCVR_TX_DIG_RF_DFT_BIST_1_CTUNE_BIST_GO_SHIFT (0U)
#define XCVR_TX_DIG_RF_DFT_BIST_1_CTUNE_BIST_GO(x) (((uint32_t)(((uint32_t)(x)) << XCVR_TX_DIG_RF_DFT_BIST_1_CTUNE_BIST_GO_SHIFT)) & XCVR_TX_DIG_RF_DFT_BIST_1_CTUNE_BIST_GO_MASK)
#define XCVR_TX_DIG_RF_DFT_BIST_1_CTUNE_BIST_FINISHED_MASK (0x2U)
#define XCVR_TX_DIG_RF_DFT_BIST_1_CTUNE_BIST_FINISHED_SHIFT (1U)
#define XCVR_TX_DIG_RF_DFT_BIST_1_CTUNE_BIST_FINISHED(x) (((uint32_t)(((uint32_t)(x)) << XCVR_TX_DIG_RF_DFT_BIST_1_CTUNE_BIST_FINISHED_SHIFT)) & XCVR_TX_DIG_RF_DFT_BIST_1_CTUNE_BIST_FINISHED_MASK)
#define XCVR_TX_DIG_RF_DFT_BIST_1_CTUNE_BIST_RESULT_MASK (0x4U)
#define XCVR_TX_DIG_RF_DFT_BIST_1_CTUNE_BIST_RESULT_SHIFT (2U)
#define XCVR_TX_DIG_RF_DFT_BIST_1_CTUNE_BIST_RESULT(x) (((uint32_t)(((uint32_t)(x)) << XCVR_TX_DIG_RF_DFT_BIST_1_CTUNE_BIST_RESULT_SHIFT)) & XCVR_TX_DIG_RF_DFT_BIST_1_CTUNE_BIST_RESULT_MASK)
#define XCVR_TX_DIG_RF_DFT_BIST_1_CTUNE_BIST_THRSHLD_MASK (0xF0U)
#define XCVR_TX_DIG_RF_DFT_BIST_1_CTUNE_BIST_THRSHLD_SHIFT (4U)
#define XCVR_TX_DIG_RF_DFT_BIST_1_CTUNE_BIST_THRSHLD(x) (((uint32_t)(((uint32_t)(x)) << XCVR_TX_DIG_RF_DFT_BIST_1_CTUNE_BIST_THRSHLD_SHIFT)) & XCVR_TX_DIG_RF_DFT_BIST_1_CTUNE_BIST_THRSHLD_MASK)
#define XCVR_TX_DIG_RF_DFT_BIST_1_CTUNE_MAX_DIFF_MASK (0xFF00U)
#define XCVR_TX_DIG_RF_DFT_BIST_1_CTUNE_MAX_DIFF_SHIFT (8U)
#define XCVR_TX_DIG_RF_DFT_BIST_1_CTUNE_MAX_DIFF(x) (((uint32_t)(((uint32_t)(x)) << XCVR_TX_DIG_RF_DFT_BIST_1_CTUNE_MAX_DIFF_SHIFT)) & XCVR_TX_DIG_RF_DFT_BIST_1_CTUNE_MAX_DIFF_MASK)
#define XCVR_TX_DIG_RF_DFT_BIST_1_CTUNE_MAX_DIFF_CH_MASK (0x7F0000U)
#define XCVR_TX_DIG_RF_DFT_BIST_1_CTUNE_MAX_DIFF_CH_SHIFT (16U)
#define XCVR_TX_DIG_RF_DFT_BIST_1_CTUNE_MAX_DIFF_CH(x) (((uint32_t)(((uint32_t)(x)) << XCVR_TX_DIG_RF_DFT_BIST_1_CTUNE_MAX_DIFF_CH_SHIFT)) & XCVR_TX_DIG_RF_DFT_BIST_1_CTUNE_MAX_DIFF_CH_MASK)
#define XCVR_TX_DIG_RF_DFT_BIST_1_PA_AM_MOD_FREQ_MASK (0x7000000U)
#define XCVR_TX_DIG_RF_DFT_BIST_1_PA_AM_MOD_FREQ_SHIFT (24U)
#define XCVR_TX_DIG_RF_DFT_BIST_1_PA_AM_MOD_FREQ(x) (((uint32_t)(((uint32_t)(x)) << XCVR_TX_DIG_RF_DFT_BIST_1_PA_AM_MOD_FREQ_SHIFT)) & XCVR_TX_DIG_RF_DFT_BIST_1_PA_AM_MOD_FREQ_MASK)
#define XCVR_TX_DIG_RF_DFT_BIST_1_PA_AM_MOD_ENTRIES_MASK (0x70000000U)
#define XCVR_TX_DIG_RF_DFT_BIST_1_PA_AM_MOD_ENTRIES_SHIFT (28U)
#define XCVR_TX_DIG_RF_DFT_BIST_1_PA_AM_MOD_ENTRIES(x) (((uint32_t)(((uint32_t)(x)) << XCVR_TX_DIG_RF_DFT_BIST_1_PA_AM_MOD_ENTRIES_SHIFT)) & XCVR_TX_DIG_RF_DFT_BIST_1_PA_AM_MOD_ENTRIES_MASK)
#define XCVR_TX_DIG_RF_DFT_BIST_1_PA_AM_MOD_EN_MASK (0x80000000U)
#define XCVR_TX_DIG_RF_DFT_BIST_1_PA_AM_MOD_EN_SHIFT (31U)
#define XCVR_TX_DIG_RF_DFT_BIST_1_PA_AM_MOD_EN(x) (((uint32_t)(((uint32_t)(x)) << XCVR_TX_DIG_RF_DFT_BIST_1_PA_AM_MOD_EN_SHIFT)) & XCVR_TX_DIG_RF_DFT_BIST_1_PA_AM_MOD_EN_MASK)

/*! @name RF_DFT_BIST_2 - TX DFT Control 2 */
#define XCVR_TX_DIG_RF_DFT_BIST_2_SYN_BIST_GO_MASK (0x1U)
#define XCVR_TX_DIG_RF_DFT_BIST_2_SYN_BIST_GO_SHIFT (0U)
#define XCVR_TX_DIG_RF_DFT_BIST_2_SYN_BIST_GO(x) (((uint32_t)(((uint32_t)(x)) << XCVR_TX_DIG_RF_DFT_BIST_2_SYN_BIST_GO_SHIFT)) & XCVR_TX_DIG_RF_DFT_BIST_2_SYN_BIST_GO_MASK)
#define XCVR_TX_DIG_RF_DFT_BIST_2_SYN_BIST_FINISHED_MASK (0x2U)
#define XCVR_TX_DIG_RF_DFT_BIST_2_SYN_BIST_FINISHED_SHIFT (1U)
#define XCVR_TX_DIG_RF_DFT_BIST_2_SYN_BIST_FINISHED(x) (((uint32_t)(((uint32_t)(x)) << XCVR_TX_DIG_RF_DFT_BIST_2_SYN_BIST_FINISHED_SHIFT)) & XCVR_TX_DIG_RF_DFT_BIST_2_SYN_BIST_FINISHED_MASK)
#define XCVR_TX_DIG_RF_DFT_BIST_2_SYN_BIST_RESULT_MASK (0x4U)
#define XCVR_TX_DIG_RF_DFT_BIST_2_SYN_BIST_RESULT_SHIFT (2U)
#define XCVR_TX_DIG_RF_DFT_BIST_2_SYN_BIST_RESULT(x) (((uint32_t)(((uint32_t)(x)) << XCVR_TX_DIG_RF_DFT_BIST_2_SYN_BIST_RESULT_SHIFT)) & XCVR_TX_DIG_RF_DFT_BIST_2_SYN_BIST_RESULT_MASK)
#define XCVR_TX_DIG_RF_DFT_BIST_2_SYN_BIST_ALL_CHANNELS_MASK (0x8U)
#define XCVR_TX_DIG_RF_DFT_BIST_2_SYN_BIST_ALL_CHANNELS_SHIFT (3U)
#define XCVR_TX_DIG_RF_DFT_BIST_2_SYN_BIST_ALL_CHANNELS(x) (((uint32_t)(((uint32_t)(x)) << XCVR_TX_DIG_RF_DFT_BIST_2_SYN_BIST_ALL_CHANNELS_SHIFT)) & XCVR_TX_DIG_RF_DFT_BIST_2_SYN_BIST_ALL_CHANNELS_MASK)
#define XCVR_TX_DIG_RF_DFT_BIST_2_FREQ_COUNT_THRESHOLD_MASK (0xFF0U)
#define XCVR_TX_DIG_RF_DFT_BIST_2_FREQ_COUNT_THRESHOLD_SHIFT (4U)
#define XCVR_TX_DIG_RF_DFT_BIST_2_FREQ_COUNT_THRESHOLD(x) (((uint32_t)(((uint32_t)(x)) << XCVR_TX_DIG_RF_DFT_BIST_2_FREQ_COUNT_THRESHOLD_SHIFT)) & XCVR_TX_DIG_RF_DFT_BIST_2_FREQ_COUNT_THRESHOLD_MASK)
#define XCVR_TX_DIG_RF_DFT_BIST_2_HPM_INL_BIST_GO_MASK (0x1000U)
#define XCVR_TX_DIG_RF_DFT_BIST_2_HPM_INL_BIST_GO_SHIFT (12U)
#define XCVR_TX_DIG_RF_DFT_BIST_2_HPM_INL_BIST_GO(x) (((uint32_t)(((uint32_t)(x)) << XCVR_TX_DIG_RF_DFT_BIST_2_HPM_INL_BIST_GO_SHIFT)) & XCVR_TX_DIG_RF_DFT_BIST_2_HPM_INL_BIST_GO_MASK)
#define XCVR_TX_DIG_RF_DFT_BIST_2_HPM_INL_BIST_FINISHED_MASK (0x2000U)
#define XCVR_TX_DIG_RF_DFT_BIST_2_HPM_INL_BIST_FINISHED_SHIFT (13U)
#define XCVR_TX_DIG_RF_DFT_BIST_2_HPM_INL_BIST_FINISHED(x) (((uint32_t)(((uint32_t)(x)) << XCVR_TX_DIG_RF_DFT_BIST_2_HPM_INL_BIST_FINISHED_SHIFT)) & XCVR_TX_DIG_RF_DFT_BIST_2_HPM_INL_BIST_FINISHED_MASK)
#define XCVR_TX_DIG_RF_DFT_BIST_2_HPM_INL_BIST_RESULT_MASK (0x4000U)
#define XCVR_TX_DIG_RF_DFT_BIST_2_HPM_INL_BIST_RESULT_SHIFT (14U)
#define XCVR_TX_DIG_RF_DFT_BIST_2_HPM_INL_BIST_RESULT(x) (((uint32_t)(((uint32_t)(x)) << XCVR_TX_DIG_RF_DFT_BIST_2_HPM_INL_BIST_RESULT_SHIFT)) & XCVR_TX_DIG_RF_DFT_BIST_2_HPM_INL_BIST_RESULT_MASK)
#define XCVR_TX_DIG_RF_DFT_BIST_2_HPM_DNL_BIST_GO_MASK (0x10000U)
#define XCVR_TX_DIG_RF_DFT_BIST_2_HPM_DNL_BIST_GO_SHIFT (16U)
#define XCVR_TX_DIG_RF_DFT_BIST_2_HPM_DNL_BIST_GO(x) (((uint32_t)(((uint32_t)(x)) << XCVR_TX_DIG_RF_DFT_BIST_2_HPM_DNL_BIST_GO_SHIFT)) & XCVR_TX_DIG_RF_DFT_BIST_2_HPM_DNL_BIST_GO_MASK)
#define XCVR_TX_DIG_RF_DFT_BIST_2_HPM_DNL_BIST_FINISHED_MASK (0x20000U)
#define XCVR_TX_DIG_RF_DFT_BIST_2_HPM_DNL_BIST_FINISHED_SHIFT (17U)
#define XCVR_TX_DIG_RF_DFT_BIST_2_HPM_DNL_BIST_FINISHED(x) (((uint32_t)(((uint32_t)(x)) << XCVR_TX_DIG_RF_DFT_BIST_2_HPM_DNL_BIST_FINISHED_SHIFT)) & XCVR_TX_DIG_RF_DFT_BIST_2_HPM_DNL_BIST_FINISHED_MASK)
#define XCVR_TX_DIG_RF_DFT_BIST_2_HPM_DNL_BIST_RESULT_MASK (0x40000U)
#define XCVR_TX_DIG_RF_DFT_BIST_2_HPM_DNL_BIST_RESULT_SHIFT (18U)
#define XCVR_TX_DIG_RF_DFT_BIST_2_HPM_DNL_BIST_RESULT(x) (((uint32_t)(((uint32_t)(x)) << XCVR_TX_DIG_RF_DFT_BIST_2_HPM_DNL_BIST_RESULT_SHIFT)) & XCVR_TX_DIG_RF_DFT_BIST_2_HPM_DNL_BIST_RESULT_MASK)
#define XCVR_TX_DIG_RF_DFT_BIST_2_DFT_MAX_RAM_SIZE_MASK (0x1FF00000U)
#define XCVR_TX_DIG_RF_DFT_BIST_2_DFT_MAX_RAM_SIZE_SHIFT (20U)
#define XCVR_TX_DIG_RF_DFT_BIST_2_DFT_MAX_RAM_SIZE(x) (((uint32_t)(((uint32_t)(x)) << XCVR_TX_DIG_RF_DFT_BIST_2_DFT_MAX_RAM_SIZE_SHIFT)) & XCVR_TX_DIG_RF_DFT_BIST_2_DFT_MAX_RAM_SIZE_MASK)


/*!
 * @}
 */ /* end of group XCVR_TX_DIG_Register_Masks */


/* XCVR_TX_DIG - Peripheral instance base addresses */
/** Peripheral XCVR_TX_DIG base address */
#define XCVR_TX_DIG_BASE                         (0x4005C200u)
/** Peripheral XCVR_TX_DIG base pointer */
#define XCVR_TX_DIG                              ((XCVR_TX_DIG_Type *)XCVR_TX_DIG_BASE)
/** Array initializer of XCVR_TX_DIG peripheral base addresses */
#define XCVR_TX_DIG_BASE_ADDRS                   { XCVR_TX_DIG_BASE }
/** Array initializer of XCVR_TX_DIG peripheral base pointers */
#define XCVR_TX_DIG_BASE_PTRS                    { XCVR_TX_DIG }

/*!
 * @}
 */ /* end of group XCVR_TX_DIG_Peripheral_Access_Layer */


/* ----------------------------------------------------------------------------
   -- XCVR_ZBDEM Peripheral Access Layer
   ---------------------------------------------------------------------------- */

/*!
 * @addtogroup XCVR_ZBDEM_Peripheral_Access_Layer XCVR_ZBDEM Peripheral Access Layer
 * @{
 */

/** XCVR_ZBDEM - Register Layout Typedef */
typedef struct {
  __IO uint32_t CORR_CTRL;                         /**< 802.15.4 DEMOD CORRELLATOR CONTROL, offset: 0x0 */
  __IO uint32_t PN_TYPE;                           /**< 802.15.4 DEMOD PN TYPE, offset: 0x4 */
  __IO uint32_t PN_CODE;                           /**< 802.15.4 DEMOD PN CODE, offset: 0x8 */
  __IO uint32_t SYNC_CTRL;                         /**< 802.15.4 DEMOD SYMBOL SYNC CONTROL, offset: 0xC */
  __IO uint32_t CCA_LQI_SRC;                       /**< 802.15.4 CCA/LQI SOURCE, offset: 0x10 */
  __IO uint32_t FAD_THR;                           /**< FAD CORRELATOR THRESHOLD, offset: 0x14 */
  __IO uint32_t ZBDEM_AFC;                         /**< 802.15.4 AFC STATUS, offset: 0x18 */
} XCVR_ZBDEM_Type;

/* ----------------------------------------------------------------------------
   -- XCVR_ZBDEM Register Masks
   ---------------------------------------------------------------------------- */

/*!
 * @addtogroup XCVR_ZBDEM_Register_Masks XCVR_ZBDEM Register Masks
 * @{
 */

/*! @name CORR_CTRL - 802.15.4 DEMOD CORRELLATOR CONTROL */
#define XCVR_ZBDEM_CORR_CTRL_CORR_VT_MASK        (0xFFU)
#define XCVR_ZBDEM_CORR_CTRL_CORR_VT_SHIFT       (0U)
#define XCVR_ZBDEM_CORR_CTRL_CORR_VT(x)          (((uint32_t)(((uint32_t)(x)) << XCVR_ZBDEM_CORR_CTRL_CORR_VT_SHIFT)) & XCVR_ZBDEM_CORR_CTRL_CORR_VT_MASK)
#define XCVR_ZBDEM_CORR_CTRL_CORR_NVAL_MASK      (0x700U)
#define XCVR_ZBDEM_CORR_CTRL_CORR_NVAL_SHIFT     (8U)
#define XCVR_ZBDEM_CORR_CTRL_CORR_NVAL(x)        (((uint32_t)(((uint32_t)(x)) << XCVR_ZBDEM_CORR_CTRL_CORR_NVAL_SHIFT)) & XCVR_ZBDEM_CORR_CTRL_CORR_NVAL_MASK)
#define XCVR_ZBDEM_CORR_CTRL_MAX_CORR_EN_MASK    (0x800U)
#define XCVR_ZBDEM_CORR_CTRL_MAX_CORR_EN_SHIFT   (11U)
#define XCVR_ZBDEM_CORR_CTRL_MAX_CORR_EN(x)      (((uint32_t)(((uint32_t)(x)) << XCVR_ZBDEM_CORR_CTRL_MAX_CORR_EN_SHIFT)) & XCVR_ZBDEM_CORR_CTRL_MAX_CORR_EN_MASK)
#define XCVR_ZBDEM_CORR_CTRL_ZBDEM_CLK_ON_MASK   (0x8000U)
#define XCVR_ZBDEM_CORR_CTRL_ZBDEM_CLK_ON_SHIFT  (15U)
#define XCVR_ZBDEM_CORR_CTRL_ZBDEM_CLK_ON(x)     (((uint32_t)(((uint32_t)(x)) << XCVR_ZBDEM_CORR_CTRL_ZBDEM_CLK_ON_SHIFT)) & XCVR_ZBDEM_CORR_CTRL_ZBDEM_CLK_ON_MASK)
#define XCVR_ZBDEM_CORR_CTRL_RX_MAX_CORR_MASK    (0xFF0000U)
#define XCVR_ZBDEM_CORR_CTRL_RX_MAX_CORR_SHIFT   (16U)
#define XCVR_ZBDEM_CORR_CTRL_RX_MAX_CORR(x)      (((uint32_t)(((uint32_t)(x)) << XCVR_ZBDEM_CORR_CTRL_RX_MAX_CORR_SHIFT)) & XCVR_ZBDEM_CORR_CTRL_RX_MAX_CORR_MASK)
#define XCVR_ZBDEM_CORR_CTRL_RX_MAX_PREAMBLE_MASK (0xFF000000U)
#define XCVR_ZBDEM_CORR_CTRL_RX_MAX_PREAMBLE_SHIFT (24U)
#define XCVR_ZBDEM_CORR_CTRL_RX_MAX_PREAMBLE(x)  (((uint32_t)(((uint32_t)(x)) << XCVR_ZBDEM_CORR_CTRL_RX_MAX_PREAMBLE_SHIFT)) & XCVR_ZBDEM_CORR_CTRL_RX_MAX_PREAMBLE_MASK)

/*! @name PN_TYPE - 802.15.4 DEMOD PN TYPE */
#define XCVR_ZBDEM_PN_TYPE_PN_TYPE_MASK          (0x1U)
#define XCVR_ZBDEM_PN_TYPE_PN_TYPE_SHIFT         (0U)
#define XCVR_ZBDEM_PN_TYPE_PN_TYPE(x)            (((uint32_t)(((uint32_t)(x)) << XCVR_ZBDEM_PN_TYPE_PN_TYPE_SHIFT)) & XCVR_ZBDEM_PN_TYPE_PN_TYPE_MASK)
#define XCVR_ZBDEM_PN_TYPE_TX_INV_MASK           (0x2U)
#define XCVR_ZBDEM_PN_TYPE_TX_INV_SHIFT          (1U)
#define XCVR_ZBDEM_PN_TYPE_TX_INV(x)             (((uint32_t)(((uint32_t)(x)) << XCVR_ZBDEM_PN_TYPE_TX_INV_SHIFT)) & XCVR_ZBDEM_PN_TYPE_TX_INV_MASK)

/*! @name PN_CODE - 802.15.4 DEMOD PN CODE */
#define XCVR_ZBDEM_PN_CODE_PN_LSB_MASK           (0xFFFFU)
#define XCVR_ZBDEM_PN_CODE_PN_LSB_SHIFT          (0U)
#define XCVR_ZBDEM_PN_CODE_PN_LSB(x)             (((uint32_t)(((uint32_t)(x)) << XCVR_ZBDEM_PN_CODE_PN_LSB_SHIFT)) & XCVR_ZBDEM_PN_CODE_PN_LSB_MASK)
#define XCVR_ZBDEM_PN_CODE_PN_MSB_MASK           (0xFFFF0000U)
#define XCVR_ZBDEM_PN_CODE_PN_MSB_SHIFT          (16U)
#define XCVR_ZBDEM_PN_CODE_PN_MSB(x)             (((uint32_t)(((uint32_t)(x)) << XCVR_ZBDEM_PN_CODE_PN_MSB_SHIFT)) & XCVR_ZBDEM_PN_CODE_PN_MSB_MASK)

/*! @name SYNC_CTRL - 802.15.4 DEMOD SYMBOL SYNC CONTROL */
#define XCVR_ZBDEM_SYNC_CTRL_SYNC_PER_MASK       (0x7U)
#define XCVR_ZBDEM_SYNC_CTRL_SYNC_PER_SHIFT      (0U)
#define XCVR_ZBDEM_SYNC_CTRL_SYNC_PER(x)         (((uint32_t)(((uint32_t)(x)) << XCVR_ZBDEM_SYNC_CTRL_SYNC_PER_SHIFT)) & XCVR_ZBDEM_SYNC_CTRL_SYNC_PER_MASK)
#define XCVR_ZBDEM_SYNC_CTRL_TRACK_ENABLE_MASK   (0x8U)
#define XCVR_ZBDEM_SYNC_CTRL_TRACK_ENABLE_SHIFT  (3U)
#define XCVR_ZBDEM_SYNC_CTRL_TRACK_ENABLE(x)     (((uint32_t)(((uint32_t)(x)) << XCVR_ZBDEM_SYNC_CTRL_TRACK_ENABLE_SHIFT)) & XCVR_ZBDEM_SYNC_CTRL_TRACK_ENABLE_MASK)

/*! @name CCA_LQI_SRC - 802.15.4 CCA/LQI SOURCE */
#define XCVR_ZBDEM_CCA_LQI_SRC_CCA1_FROM_RX_DIG_MASK (0x1U)
#define XCVR_ZBDEM_CCA_LQI_SRC_CCA1_FROM_RX_DIG_SHIFT (0U)
#define XCVR_ZBDEM_CCA_LQI_SRC_CCA1_FROM_RX_DIG(x) (((uint32_t)(((uint32_t)(x)) << XCVR_ZBDEM_CCA_LQI_SRC_CCA1_FROM_RX_DIG_SHIFT)) & XCVR_ZBDEM_CCA_LQI_SRC_CCA1_FROM_RX_DIG_MASK)
#define XCVR_ZBDEM_CCA_LQI_SRC_LQI_FROM_RX_DIG_MASK (0x2U)
#define XCVR_ZBDEM_CCA_LQI_SRC_LQI_FROM_RX_DIG_SHIFT (1U)
#define XCVR_ZBDEM_CCA_LQI_SRC_LQI_FROM_RX_DIG(x) (((uint32_t)(((uint32_t)(x)) << XCVR_ZBDEM_CCA_LQI_SRC_LQI_FROM_RX_DIG_SHIFT)) & XCVR_ZBDEM_CCA_LQI_SRC_LQI_FROM_RX_DIG_MASK)
#define XCVR_ZBDEM_CCA_LQI_SRC_LQI_START_AT_SFD_MASK (0x4U)
#define XCVR_ZBDEM_CCA_LQI_SRC_LQI_START_AT_SFD_SHIFT (2U)
#define XCVR_ZBDEM_CCA_LQI_SRC_LQI_START_AT_SFD(x) (((uint32_t)(((uint32_t)(x)) << XCVR_ZBDEM_CCA_LQI_SRC_LQI_START_AT_SFD_SHIFT)) & XCVR_ZBDEM_CCA_LQI_SRC_LQI_START_AT_SFD_MASK)

/*! @name FAD_THR - FAD CORRELATOR THRESHOLD */
#define XCVR_ZBDEM_FAD_THR_FAD_THR_MASK          (0xFFU)
#define XCVR_ZBDEM_FAD_THR_FAD_THR_SHIFT         (0U)
#define XCVR_ZBDEM_FAD_THR_FAD_THR(x)            (((uint32_t)(((uint32_t)(x)) << XCVR_ZBDEM_FAD_THR_FAD_THR_SHIFT)) & XCVR_ZBDEM_FAD_THR_FAD_THR_MASK)

/*! @name ZBDEM_AFC - 802.15.4 AFC STATUS */
#define XCVR_ZBDEM_ZBDEM_AFC_AFC_EN_MASK         (0x1U)
#define XCVR_ZBDEM_ZBDEM_AFC_AFC_EN_SHIFT        (0U)
#define XCVR_ZBDEM_ZBDEM_AFC_AFC_EN(x)           (((uint32_t)(((uint32_t)(x)) << XCVR_ZBDEM_ZBDEM_AFC_AFC_EN_SHIFT)) & XCVR_ZBDEM_ZBDEM_AFC_AFC_EN_MASK)
#define XCVR_ZBDEM_ZBDEM_AFC_DCD_EN_MASK         (0x2U)
#define XCVR_ZBDEM_ZBDEM_AFC_DCD_EN_SHIFT        (1U)
#define XCVR_ZBDEM_ZBDEM_AFC_DCD_EN(x)           (((uint32_t)(((uint32_t)(x)) << XCVR_ZBDEM_ZBDEM_AFC_DCD_EN_SHIFT)) & XCVR_ZBDEM_ZBDEM_AFC_DCD_EN_MASK)
#define XCVR_ZBDEM_ZBDEM_AFC_AFC_OUT_MASK        (0x1F00U)
#define XCVR_ZBDEM_ZBDEM_AFC_AFC_OUT_SHIFT       (8U)
#define XCVR_ZBDEM_ZBDEM_AFC_AFC_OUT(x)          (((uint32_t)(((uint32_t)(x)) << XCVR_ZBDEM_ZBDEM_AFC_AFC_OUT_SHIFT)) & XCVR_ZBDEM_ZBDEM_AFC_AFC_OUT_MASK)


/*!
 * @}
 */ /* end of group XCVR_ZBDEM_Register_Masks */


/* XCVR_ZBDEM - Peripheral instance base addresses */
/** Peripheral XCVR_ZBDEM base address */
#define XCVR_ZBDEM_BASE                          (0x4005C480u)
/** Peripheral XCVR_ZBDEM base pointer */
#define XCVR_ZBDEM                               ((XCVR_ZBDEM_Type *)XCVR_ZBDEM_BASE)
/** Array initializer of XCVR_ZBDEM peripheral base addresses */
#define XCVR_ZBDEM_BASE_ADDRS                    { XCVR_ZBDEM_BASE }
/** Array initializer of XCVR_ZBDEM peripheral base pointers */
#define XCVR_ZBDEM_BASE_PTRS                     { XCVR_ZBDEM }

/*!
 * @}
 */ /* end of group XCVR_ZBDEM_Peripheral_Access_Layer */


/* ----------------------------------------------------------------------------
   -- ZLL Peripheral Access Layer
   ---------------------------------------------------------------------------- */

/*!
 * @addtogroup ZLL_Peripheral_Access_Layer ZLL Peripheral Access Layer
 * @{
 */

/** ZLL - Register Layout Typedef */
typedef struct {
  __IO uint32_t IRQSTS;                            /**< INTERRUPT REQUEST STATUS, offset: 0x0 */
  __IO uint32_t PHY_CTRL;                          /**< PHY CONTROL, offset: 0x4 */
  __IO uint32_t EVENT_TMR;                         /**< EVENT TIMER, offset: 0x8 */
  __I  uint32_t TIMESTAMP;                         /**< TIMESTAMP, offset: 0xC */
  __IO uint32_t T1CMP;                             /**< T1 COMPARE, offset: 0x10 */
  __IO uint32_t T2CMP;                             /**< T2 COMPARE, offset: 0x14 */
  __IO uint32_t T2PRIMECMP;                        /**< T2 PRIME COMPARE, offset: 0x18 */
  __IO uint32_t T3CMP;                             /**< T3 COMPARE, offset: 0x1C */
  __IO uint32_t T4CMP;                             /**< T4 COMPARE, offset: 0x20 */
  __IO uint32_t PA_PWR;                            /**< PA POWER, offset: 0x24 */
  __IO uint32_t CHANNEL_NUM0;                      /**< CHANNEL NUMBER 0, offset: 0x28 */
  __I  uint32_t LQI_AND_RSSI;                      /**< LQI AND RSSI, offset: 0x2C */
  __IO uint32_t MACSHORTADDRS0;                    /**< MAC SHORT ADDRESS 0, offset: 0x30 */
  __IO uint32_t MACLONGADDRS0_LSB;                 /**< MAC LONG ADDRESS 0 LSB, offset: 0x34 */
  __IO uint32_t MACLONGADDRS0_MSB;                 /**< MAC LONG ADDRESS 0 MSB, offset: 0x38 */
  __IO uint32_t RX_FRAME_FILTER;                   /**< RECEIVE FRAME FILTER, offset: 0x3C */
  __IO uint32_t CCA_LQI_CTRL;                      /**< CCA AND LQI CONTROL, offset: 0x40 */
  __IO uint32_t CCA2_CTRL;                         /**< CCA2 CONTROL, offset: 0x44 */
       uint8_t RESERVED_0[4];
  __IO uint32_t DSM_CTRL;                          /**< DSM CONTROL, offset: 0x4C */
  __IO uint32_t BSM_CTRL;                          /**< BSM CONTROL, offset: 0x50 */
  __IO uint32_t MACSHORTADDRS1;                    /**< MAC SHORT ADDRESS FOR PAN1, offset: 0x54 */
  __IO uint32_t MACLONGADDRS1_LSB;                 /**< MAC LONG ADDRESS 1 LSB, offset: 0x58 */
  __IO uint32_t MACLONGADDRS1_MSB;                 /**< MAC LONG ADDRESS 1 MSB, offset: 0x5C */
  __IO uint32_t DUAL_PAN_CTRL;                     /**< DUAL PAN CONTROL, offset: 0x60 */
  __IO uint32_t CHANNEL_NUM1;                      /**< CHANNEL NUMBER 1, offset: 0x64 */
  __IO uint32_t SAM_CTRL;                          /**< SAM CONTROL, offset: 0x68 */
  __IO uint32_t SAM_TABLE;                         /**< SOURCE ADDRESS MANAGEMENT TABLE, offset: 0x6C */
  __I  uint32_t SAM_MATCH;                         /**< SOURCE ADDRESS MANAGEMENT MATCH, offset: 0x70 */
  __I  uint32_t SAM_FREE_IDX;                      /**< SAM FREE INDEX, offset: 0x74 */
  __IO uint32_t SEQ_CTRL_STS;                      /**< SEQUENCE CONTROL AND STATUS, offset: 0x78 */
  __IO uint32_t ACKDELAY;                          /**< ACK DELAY, offset: 0x7C */
  __IO uint32_t FILTERFAIL_CODE;                   /**< FILTER FAIL CODE, offset: 0x80 */
  __IO uint32_t RX_WTR_MARK;                       /**< RECEIVE WATER MARK, offset: 0x84 */
       uint8_t RESERVED_1[4];
  __IO uint32_t SLOT_PRELOAD;                      /**< SLOT PRELOAD, offset: 0x8C */
  __I  uint32_t SEQ_STATE;                         /**< 802.15.4 SEQUENCE STATE, offset: 0x90 */
  __IO uint32_t TMR_PRESCALE;                      /**< TIMER PRESCALER, offset: 0x94 */
  __IO uint32_t LENIENCY_LSB;                      /**< LENIENCY LSB, offset: 0x98 */
  __IO uint32_t LENIENCY_MSB;                      /**< LENIENCY MSB, offset: 0x9C */
  __I  uint32_t PART_ID;                           /**< PART ID, offset: 0xA0 */
       uint8_t RESERVED_2[92];
  __IO uint16_t PKT_BUFFER_TX[64];                 /**< Packet Buffer TX, array offset: 0x100, array step: 0x2 */
  __IO uint16_t PKT_BUFFER_RX[64];                 /**< Packet Buffer RX, array offset: 0x180, array step: 0x2 */
} ZLL_Type;

/* ----------------------------------------------------------------------------
   -- ZLL Register Masks
   ---------------------------------------------------------------------------- */

/*!
 * @addtogroup ZLL_Register_Masks ZLL Register Masks
 * @{
 */

/*! @name IRQSTS - INTERRUPT REQUEST STATUS */
#define ZLL_IRQSTS_SEQIRQ_MASK                   (0x1U)
#define ZLL_IRQSTS_SEQIRQ_SHIFT                  (0U)
#define ZLL_IRQSTS_SEQIRQ(x)                     (((uint32_t)(((uint32_t)(x)) << ZLL_IRQSTS_SEQIRQ_SHIFT)) & ZLL_IRQSTS_SEQIRQ_MASK)
#define ZLL_IRQSTS_TXIRQ_MASK                    (0x2U)
#define ZLL_IRQSTS_TXIRQ_SHIFT                   (1U)
#define ZLL_IRQSTS_TXIRQ(x)                      (((uint32_t)(((uint32_t)(x)) << ZLL_IRQSTS_TXIRQ_SHIFT)) & ZLL_IRQSTS_TXIRQ_MASK)
#define ZLL_IRQSTS_RXIRQ_MASK                    (0x4U)
#define ZLL_IRQSTS_RXIRQ_SHIFT                   (2U)
#define ZLL_IRQSTS_RXIRQ(x)                      (((uint32_t)(((uint32_t)(x)) << ZLL_IRQSTS_RXIRQ_SHIFT)) & ZLL_IRQSTS_RXIRQ_MASK)
#define ZLL_IRQSTS_CCAIRQ_MASK                   (0x8U)
#define ZLL_IRQSTS_CCAIRQ_SHIFT                  (3U)
#define ZLL_IRQSTS_CCAIRQ(x)                     (((uint32_t)(((uint32_t)(x)) << ZLL_IRQSTS_CCAIRQ_SHIFT)) & ZLL_IRQSTS_CCAIRQ_MASK)
#define ZLL_IRQSTS_RXWTRMRKIRQ_MASK              (0x10U)
#define ZLL_IRQSTS_RXWTRMRKIRQ_SHIFT             (4U)
#define ZLL_IRQSTS_RXWTRMRKIRQ(x)                (((uint32_t)(((uint32_t)(x)) << ZLL_IRQSTS_RXWTRMRKIRQ_SHIFT)) & ZLL_IRQSTS_RXWTRMRKIRQ_MASK)
#define ZLL_IRQSTS_FILTERFAIL_IRQ_MASK           (0x20U)
#define ZLL_IRQSTS_FILTERFAIL_IRQ_SHIFT          (5U)
#define ZLL_IRQSTS_FILTERFAIL_IRQ(x)             (((uint32_t)(((uint32_t)(x)) << ZLL_IRQSTS_FILTERFAIL_IRQ_SHIFT)) & ZLL_IRQSTS_FILTERFAIL_IRQ_MASK)
#define ZLL_IRQSTS_PLL_UNLOCK_IRQ_MASK           (0x40U)
#define ZLL_IRQSTS_PLL_UNLOCK_IRQ_SHIFT          (6U)
#define ZLL_IRQSTS_PLL_UNLOCK_IRQ(x)             (((uint32_t)(((uint32_t)(x)) << ZLL_IRQSTS_PLL_UNLOCK_IRQ_SHIFT)) & ZLL_IRQSTS_PLL_UNLOCK_IRQ_MASK)
#define ZLL_IRQSTS_RX_FRM_PEND_MASK              (0x80U)
#define ZLL_IRQSTS_RX_FRM_PEND_SHIFT             (7U)
#define ZLL_IRQSTS_RX_FRM_PEND(x)                (((uint32_t)(((uint32_t)(x)) << ZLL_IRQSTS_RX_FRM_PEND_SHIFT)) & ZLL_IRQSTS_RX_FRM_PEND_MASK)
#define ZLL_IRQSTS_WAKE_IRQ_MASK                 (0x100U)
#define ZLL_IRQSTS_WAKE_IRQ_SHIFT                (8U)
#define ZLL_IRQSTS_WAKE_IRQ(x)                   (((uint32_t)(((uint32_t)(x)) << ZLL_IRQSTS_WAKE_IRQ_SHIFT)) & ZLL_IRQSTS_WAKE_IRQ_MASK)
#define ZLL_IRQSTS_TSM_IRQ_MASK                  (0x400U)
#define ZLL_IRQSTS_TSM_IRQ_SHIFT                 (10U)
#define ZLL_IRQSTS_TSM_IRQ(x)                    (((uint32_t)(((uint32_t)(x)) << ZLL_IRQSTS_TSM_IRQ_SHIFT)) & ZLL_IRQSTS_TSM_IRQ_MASK)
#define ZLL_IRQSTS_ENH_PKT_STATUS_MASK           (0x800U)
#define ZLL_IRQSTS_ENH_PKT_STATUS_SHIFT          (11U)
#define ZLL_IRQSTS_ENH_PKT_STATUS(x)             (((uint32_t)(((uint32_t)(x)) << ZLL_IRQSTS_ENH_PKT_STATUS_SHIFT)) & ZLL_IRQSTS_ENH_PKT_STATUS_MASK)
#define ZLL_IRQSTS_PI_MASK                       (0x1000U)
#define ZLL_IRQSTS_PI_SHIFT                      (12U)
#define ZLL_IRQSTS_PI(x)                         (((uint32_t)(((uint32_t)(x)) << ZLL_IRQSTS_PI_SHIFT)) & ZLL_IRQSTS_PI_MASK)
#define ZLL_IRQSTS_SRCADDR_MASK                  (0x2000U)
#define ZLL_IRQSTS_SRCADDR_SHIFT                 (13U)
#define ZLL_IRQSTS_SRCADDR(x)                    (((uint32_t)(((uint32_t)(x)) << ZLL_IRQSTS_SRCADDR_SHIFT)) & ZLL_IRQSTS_SRCADDR_MASK)
#define ZLL_IRQSTS_CCA_MASK                      (0x4000U)
#define ZLL_IRQSTS_CCA_SHIFT                     (14U)
#define ZLL_IRQSTS_CCA(x)                        (((uint32_t)(((uint32_t)(x)) << ZLL_IRQSTS_CCA_SHIFT)) & ZLL_IRQSTS_CCA_MASK)
#define ZLL_IRQSTS_CRCVALID_MASK                 (0x8000U)
#define ZLL_IRQSTS_CRCVALID_SHIFT                (15U)
#define ZLL_IRQSTS_CRCVALID(x)                   (((uint32_t)(((uint32_t)(x)) << ZLL_IRQSTS_CRCVALID_SHIFT)) & ZLL_IRQSTS_CRCVALID_MASK)
#define ZLL_IRQSTS_TMR1IRQ_MASK                  (0x10000U)
#define ZLL_IRQSTS_TMR1IRQ_SHIFT                 (16U)
#define ZLL_IRQSTS_TMR1IRQ(x)                    (((uint32_t)(((uint32_t)(x)) << ZLL_IRQSTS_TMR1IRQ_SHIFT)) & ZLL_IRQSTS_TMR1IRQ_MASK)
#define ZLL_IRQSTS_TMR2IRQ_MASK                  (0x20000U)
#define ZLL_IRQSTS_TMR2IRQ_SHIFT                 (17U)
#define ZLL_IRQSTS_TMR2IRQ(x)                    (((uint32_t)(((uint32_t)(x)) << ZLL_IRQSTS_TMR2IRQ_SHIFT)) & ZLL_IRQSTS_TMR2IRQ_MASK)
#define ZLL_IRQSTS_TMR3IRQ_MASK                  (0x40000U)
#define ZLL_IRQSTS_TMR3IRQ_SHIFT                 (18U)
#define ZLL_IRQSTS_TMR3IRQ(x)                    (((uint32_t)(((uint32_t)(x)) << ZLL_IRQSTS_TMR3IRQ_SHIFT)) & ZLL_IRQSTS_TMR3IRQ_MASK)
#define ZLL_IRQSTS_TMR4IRQ_MASK                  (0x80000U)
#define ZLL_IRQSTS_TMR4IRQ_SHIFT                 (19U)
#define ZLL_IRQSTS_TMR4IRQ(x)                    (((uint32_t)(((uint32_t)(x)) << ZLL_IRQSTS_TMR4IRQ_SHIFT)) & ZLL_IRQSTS_TMR4IRQ_MASK)
#define ZLL_IRQSTS_TMR1MSK_MASK                  (0x100000U)
#define ZLL_IRQSTS_TMR1MSK_SHIFT                 (20U)
#define ZLL_IRQSTS_TMR1MSK(x)                    (((uint32_t)(((uint32_t)(x)) << ZLL_IRQSTS_TMR1MSK_SHIFT)) & ZLL_IRQSTS_TMR1MSK_MASK)
#define ZLL_IRQSTS_TMR2MSK_MASK                  (0x200000U)
#define ZLL_IRQSTS_TMR2MSK_SHIFT                 (21U)
#define ZLL_IRQSTS_TMR2MSK(x)                    (((uint32_t)(((uint32_t)(x)) << ZLL_IRQSTS_TMR2MSK_SHIFT)) & ZLL_IRQSTS_TMR2MSK_MASK)
#define ZLL_IRQSTS_TMR3MSK_MASK                  (0x400000U)
#define ZLL_IRQSTS_TMR3MSK_SHIFT                 (22U)
#define ZLL_IRQSTS_TMR3MSK(x)                    (((uint32_t)(((uint32_t)(x)) << ZLL_IRQSTS_TMR3MSK_SHIFT)) & ZLL_IRQSTS_TMR3MSK_MASK)
#define ZLL_IRQSTS_TMR4MSK_MASK                  (0x800000U)
#define ZLL_IRQSTS_TMR4MSK_SHIFT                 (23U)
#define ZLL_IRQSTS_TMR4MSK(x)                    (((uint32_t)(((uint32_t)(x)) << ZLL_IRQSTS_TMR4MSK_SHIFT)) & ZLL_IRQSTS_TMR4MSK_MASK)
#define ZLL_IRQSTS_RX_FRAME_LENGTH_MASK          (0x7F000000U)
#define ZLL_IRQSTS_RX_FRAME_LENGTH_SHIFT         (24U)
#define ZLL_IRQSTS_RX_FRAME_LENGTH(x)            (((uint32_t)(((uint32_t)(x)) << ZLL_IRQSTS_RX_FRAME_LENGTH_SHIFT)) & ZLL_IRQSTS_RX_FRAME_LENGTH_MASK)

/*! @name PHY_CTRL - PHY CONTROL */
#define ZLL_PHY_CTRL_XCVSEQ_MASK                 (0x7U)
#define ZLL_PHY_CTRL_XCVSEQ_SHIFT                (0U)
#define ZLL_PHY_CTRL_XCVSEQ(x)                   (((uint32_t)(((uint32_t)(x)) << ZLL_PHY_CTRL_XCVSEQ_SHIFT)) & ZLL_PHY_CTRL_XCVSEQ_MASK)
#define ZLL_PHY_CTRL_AUTOACK_MASK                (0x8U)
#define ZLL_PHY_CTRL_AUTOACK_SHIFT               (3U)
#define ZLL_PHY_CTRL_AUTOACK(x)                  (((uint32_t)(((uint32_t)(x)) << ZLL_PHY_CTRL_AUTOACK_SHIFT)) & ZLL_PHY_CTRL_AUTOACK_MASK)
#define ZLL_PHY_CTRL_RXACKRQD_MASK               (0x10U)
#define ZLL_PHY_CTRL_RXACKRQD_SHIFT              (4U)
#define ZLL_PHY_CTRL_RXACKRQD(x)                 (((uint32_t)(((uint32_t)(x)) << ZLL_PHY_CTRL_RXACKRQD_SHIFT)) & ZLL_PHY_CTRL_RXACKRQD_MASK)
#define ZLL_PHY_CTRL_CCABFRTX_MASK               (0x20U)
#define ZLL_PHY_CTRL_CCABFRTX_SHIFT              (5U)
#define ZLL_PHY_CTRL_CCABFRTX(x)                 (((uint32_t)(((uint32_t)(x)) << ZLL_PHY_CTRL_CCABFRTX_SHIFT)) & ZLL_PHY_CTRL_CCABFRTX_MASK)
#define ZLL_PHY_CTRL_SLOTTED_MASK                (0x40U)
#define ZLL_PHY_CTRL_SLOTTED_SHIFT               (6U)
#define ZLL_PHY_CTRL_SLOTTED(x)                  (((uint32_t)(((uint32_t)(x)) << ZLL_PHY_CTRL_SLOTTED_SHIFT)) & ZLL_PHY_CTRL_SLOTTED_MASK)
#define ZLL_PHY_CTRL_TMRTRIGEN_MASK              (0x80U)
#define ZLL_PHY_CTRL_TMRTRIGEN_SHIFT             (7U)
#define ZLL_PHY_CTRL_TMRTRIGEN(x)                (((uint32_t)(((uint32_t)(x)) << ZLL_PHY_CTRL_TMRTRIGEN_SHIFT)) & ZLL_PHY_CTRL_TMRTRIGEN_MASK)
#define ZLL_PHY_CTRL_SEQMSK_MASK                 (0x100U)
#define ZLL_PHY_CTRL_SEQMSK_SHIFT                (8U)
#define ZLL_PHY_CTRL_SEQMSK(x)                   (((uint32_t)(((uint32_t)(x)) << ZLL_PHY_CTRL_SEQMSK_SHIFT)) & ZLL_PHY_CTRL_SEQMSK_MASK)
#define ZLL_PHY_CTRL_TXMSK_MASK                  (0x200U)
#define ZLL_PHY_CTRL_TXMSK_SHIFT                 (9U)
#define ZLL_PHY_CTRL_TXMSK(x)                    (((uint32_t)(((uint32_t)(x)) << ZLL_PHY_CTRL_TXMSK_SHIFT)) & ZLL_PHY_CTRL_TXMSK_MASK)
#define ZLL_PHY_CTRL_RXMSK_MASK                  (0x400U)
#define ZLL_PHY_CTRL_RXMSK_SHIFT                 (10U)
#define ZLL_PHY_CTRL_RXMSK(x)                    (((uint32_t)(((uint32_t)(x)) << ZLL_PHY_CTRL_RXMSK_SHIFT)) & ZLL_PHY_CTRL_RXMSK_MASK)
#define ZLL_PHY_CTRL_CCAMSK_MASK                 (0x800U)
#define ZLL_PHY_CTRL_CCAMSK_SHIFT                (11U)
#define ZLL_PHY_CTRL_CCAMSK(x)                   (((uint32_t)(((uint32_t)(x)) << ZLL_PHY_CTRL_CCAMSK_SHIFT)) & ZLL_PHY_CTRL_CCAMSK_MASK)
#define ZLL_PHY_CTRL_RX_WMRK_MSK_MASK            (0x1000U)
#define ZLL_PHY_CTRL_RX_WMRK_MSK_SHIFT           (12U)
#define ZLL_PHY_CTRL_RX_WMRK_MSK(x)              (((uint32_t)(((uint32_t)(x)) << ZLL_PHY_CTRL_RX_WMRK_MSK_SHIFT)) & ZLL_PHY_CTRL_RX_WMRK_MSK_MASK)
#define ZLL_PHY_CTRL_FILTERFAIL_MSK_MASK         (0x2000U)
#define ZLL_PHY_CTRL_FILTERFAIL_MSK_SHIFT        (13U)
#define ZLL_PHY_CTRL_FILTERFAIL_MSK(x)           (((uint32_t)(((uint32_t)(x)) << ZLL_PHY_CTRL_FILTERFAIL_MSK_SHIFT)) & ZLL_PHY_CTRL_FILTERFAIL_MSK_MASK)
#define ZLL_PHY_CTRL_PLL_UNLOCK_MSK_MASK         (0x4000U)
#define ZLL_PHY_CTRL_PLL_UNLOCK_MSK_SHIFT        (14U)
#define ZLL_PHY_CTRL_PLL_UNLOCK_MSK(x)           (((uint32_t)(((uint32_t)(x)) << ZLL_PHY_CTRL_PLL_UNLOCK_MSK_SHIFT)) & ZLL_PHY_CTRL_PLL_UNLOCK_MSK_MASK)
#define ZLL_PHY_CTRL_CRC_MSK_MASK                (0x8000U)
#define ZLL_PHY_CTRL_CRC_MSK_SHIFT               (15U)
#define ZLL_PHY_CTRL_CRC_MSK(x)                  (((uint32_t)(((uint32_t)(x)) << ZLL_PHY_CTRL_CRC_MSK_SHIFT)) & ZLL_PHY_CTRL_CRC_MSK_MASK)
#define ZLL_PHY_CTRL_WAKE_MSK_MASK               (0x10000U)
#define ZLL_PHY_CTRL_WAKE_MSK_SHIFT              (16U)
#define ZLL_PHY_CTRL_WAKE_MSK(x)                 (((uint32_t)(((uint32_t)(x)) << ZLL_PHY_CTRL_WAKE_MSK_SHIFT)) & ZLL_PHY_CTRL_WAKE_MSK_MASK)
#define ZLL_PHY_CTRL_TSM_MSK_MASK                (0x40000U)
#define ZLL_PHY_CTRL_TSM_MSK_SHIFT               (18U)
#define ZLL_PHY_CTRL_TSM_MSK(x)                  (((uint32_t)(((uint32_t)(x)) << ZLL_PHY_CTRL_TSM_MSK_SHIFT)) & ZLL_PHY_CTRL_TSM_MSK_MASK)
#define ZLL_PHY_CTRL_TMR1CMP_EN_MASK             (0x100000U)
#define ZLL_PHY_CTRL_TMR1CMP_EN_SHIFT            (20U)
#define ZLL_PHY_CTRL_TMR1CMP_EN(x)               (((uint32_t)(((uint32_t)(x)) << ZLL_PHY_CTRL_TMR1CMP_EN_SHIFT)) & ZLL_PHY_CTRL_TMR1CMP_EN_MASK)
#define ZLL_PHY_CTRL_TMR2CMP_EN_MASK             (0x200000U)
#define ZLL_PHY_CTRL_TMR2CMP_EN_SHIFT            (21U)
#define ZLL_PHY_CTRL_TMR2CMP_EN(x)               (((uint32_t)(((uint32_t)(x)) << ZLL_PHY_CTRL_TMR2CMP_EN_SHIFT)) & ZLL_PHY_CTRL_TMR2CMP_EN_MASK)
#define ZLL_PHY_CTRL_TMR3CMP_EN_MASK             (0x400000U)
#define ZLL_PHY_CTRL_TMR3CMP_EN_SHIFT            (22U)
#define ZLL_PHY_CTRL_TMR3CMP_EN(x)               (((uint32_t)(((uint32_t)(x)) << ZLL_PHY_CTRL_TMR3CMP_EN_SHIFT)) & ZLL_PHY_CTRL_TMR3CMP_EN_MASK)
#define ZLL_PHY_CTRL_TMR4CMP_EN_MASK             (0x800000U)
#define ZLL_PHY_CTRL_TMR4CMP_EN_SHIFT            (23U)
#define ZLL_PHY_CTRL_TMR4CMP_EN(x)               (((uint32_t)(((uint32_t)(x)) << ZLL_PHY_CTRL_TMR4CMP_EN_SHIFT)) & ZLL_PHY_CTRL_TMR4CMP_EN_MASK)
#define ZLL_PHY_CTRL_TC2PRIME_EN_MASK            (0x1000000U)
#define ZLL_PHY_CTRL_TC2PRIME_EN_SHIFT           (24U)
#define ZLL_PHY_CTRL_TC2PRIME_EN(x)              (((uint32_t)(((uint32_t)(x)) << ZLL_PHY_CTRL_TC2PRIME_EN_SHIFT)) & ZLL_PHY_CTRL_TC2PRIME_EN_MASK)
#define ZLL_PHY_CTRL_PROMISCUOUS_MASK            (0x2000000U)
#define ZLL_PHY_CTRL_PROMISCUOUS_SHIFT           (25U)
#define ZLL_PHY_CTRL_PROMISCUOUS(x)              (((uint32_t)(((uint32_t)(x)) << ZLL_PHY_CTRL_PROMISCUOUS_SHIFT)) & ZLL_PHY_CTRL_PROMISCUOUS_MASK)
#define ZLL_PHY_CTRL_CCATYPE_MASK                (0x18000000U)
#define ZLL_PHY_CTRL_CCATYPE_SHIFT               (27U)
#define ZLL_PHY_CTRL_CCATYPE(x)                  (((uint32_t)(((uint32_t)(x)) << ZLL_PHY_CTRL_CCATYPE_SHIFT)) & ZLL_PHY_CTRL_CCATYPE_MASK)
#define ZLL_PHY_CTRL_PANCORDNTR0_MASK            (0x20000000U)
#define ZLL_PHY_CTRL_PANCORDNTR0_SHIFT           (29U)
#define ZLL_PHY_CTRL_PANCORDNTR0(x)              (((uint32_t)(((uint32_t)(x)) << ZLL_PHY_CTRL_PANCORDNTR0_SHIFT)) & ZLL_PHY_CTRL_PANCORDNTR0_MASK)
#define ZLL_PHY_CTRL_TC3TMOUT_MASK               (0x40000000U)
#define ZLL_PHY_CTRL_TC3TMOUT_SHIFT              (30U)
#define ZLL_PHY_CTRL_TC3TMOUT(x)                 (((uint32_t)(((uint32_t)(x)) << ZLL_PHY_CTRL_TC3TMOUT_SHIFT)) & ZLL_PHY_CTRL_TC3TMOUT_MASK)
#define ZLL_PHY_CTRL_TRCV_MSK_MASK               (0x80000000U)
#define ZLL_PHY_CTRL_TRCV_MSK_SHIFT              (31U)
#define ZLL_PHY_CTRL_TRCV_MSK(x)                 (((uint32_t)(((uint32_t)(x)) << ZLL_PHY_CTRL_TRCV_MSK_SHIFT)) & ZLL_PHY_CTRL_TRCV_MSK_MASK)

/*! @name EVENT_TMR - EVENT TIMER */
#define ZLL_EVENT_TMR_EVENT_TMR_LD_MASK          (0x1U)
#define ZLL_EVENT_TMR_EVENT_TMR_LD_SHIFT         (0U)
#define ZLL_EVENT_TMR_EVENT_TMR_LD(x)            (((uint32_t)(((uint32_t)(x)) << ZLL_EVENT_TMR_EVENT_TMR_LD_SHIFT)) & ZLL_EVENT_TMR_EVENT_TMR_LD_MASK)
#define ZLL_EVENT_TMR_EVENT_TMR_ADD_MASK         (0x2U)
#define ZLL_EVENT_TMR_EVENT_TMR_ADD_SHIFT        (1U)
#define ZLL_EVENT_TMR_EVENT_TMR_ADD(x)           (((uint32_t)(((uint32_t)(x)) << ZLL_EVENT_TMR_EVENT_TMR_ADD_SHIFT)) & ZLL_EVENT_TMR_EVENT_TMR_ADD_MASK)
#define ZLL_EVENT_TMR_EVENT_TMR_FRAC_MASK        (0xF0U)
#define ZLL_EVENT_TMR_EVENT_TMR_FRAC_SHIFT       (4U)
#define ZLL_EVENT_TMR_EVENT_TMR_FRAC(x)          (((uint32_t)(((uint32_t)(x)) << ZLL_EVENT_TMR_EVENT_TMR_FRAC_SHIFT)) & ZLL_EVENT_TMR_EVENT_TMR_FRAC_MASK)
#define ZLL_EVENT_TMR_EVENT_TMR_MASK             (0xFFFFFF00U)
#define ZLL_EVENT_TMR_EVENT_TMR_SHIFT            (8U)
#define ZLL_EVENT_TMR_EVENT_TMR(x)               (((uint32_t)(((uint32_t)(x)) << ZLL_EVENT_TMR_EVENT_TMR_SHIFT)) & ZLL_EVENT_TMR_EVENT_TMR_MASK)

/*! @name TIMESTAMP - TIMESTAMP */
#define ZLL_TIMESTAMP_TIMESTAMP_MASK             (0xFFFFFFU)
#define ZLL_TIMESTAMP_TIMESTAMP_SHIFT            (0U)
#define ZLL_TIMESTAMP_TIMESTAMP(x)               (((uint32_t)(((uint32_t)(x)) << ZLL_TIMESTAMP_TIMESTAMP_SHIFT)) & ZLL_TIMESTAMP_TIMESTAMP_MASK)

/*! @name T1CMP - T1 COMPARE */
#define ZLL_T1CMP_T1CMP_MASK                     (0xFFFFFFU)
#define ZLL_T1CMP_T1CMP_SHIFT                    (0U)
#define ZLL_T1CMP_T1CMP(x)                       (((uint32_t)(((uint32_t)(x)) << ZLL_T1CMP_T1CMP_SHIFT)) & ZLL_T1CMP_T1CMP_MASK)

/*! @name T2CMP - T2 COMPARE */
#define ZLL_T2CMP_T2CMP_MASK                     (0xFFFFFFU)
#define ZLL_T2CMP_T2CMP_SHIFT                    (0U)
#define ZLL_T2CMP_T2CMP(x)                       (((uint32_t)(((uint32_t)(x)) << ZLL_T2CMP_T2CMP_SHIFT)) & ZLL_T2CMP_T2CMP_MASK)

/*! @name T2PRIMECMP - T2 PRIME COMPARE */
#define ZLL_T2PRIMECMP_T2PRIMECMP_MASK           (0xFFFFU)
#define ZLL_T2PRIMECMP_T2PRIMECMP_SHIFT          (0U)
#define ZLL_T2PRIMECMP_T2PRIMECMP(x)             (((uint32_t)(((uint32_t)(x)) << ZLL_T2PRIMECMP_T2PRIMECMP_SHIFT)) & ZLL_T2PRIMECMP_T2PRIMECMP_MASK)

/*! @name T3CMP - T3 COMPARE */
#define ZLL_T3CMP_T3CMP_MASK                     (0xFFFFFFU)
#define ZLL_T3CMP_T3CMP_SHIFT                    (0U)
#define ZLL_T3CMP_T3CMP(x)                       (((uint32_t)(((uint32_t)(x)) << ZLL_T3CMP_T3CMP_SHIFT)) & ZLL_T3CMP_T3CMP_MASK)

/*! @name T4CMP - T4 COMPARE */
#define ZLL_T4CMP_T4CMP_MASK                     (0xFFFFFFU)
#define ZLL_T4CMP_T4CMP_SHIFT                    (0U)
#define ZLL_T4CMP_T4CMP(x)                       (((uint32_t)(((uint32_t)(x)) << ZLL_T4CMP_T4CMP_SHIFT)) & ZLL_T4CMP_T4CMP_MASK)

/*! @name PA_PWR - PA POWER */
#define ZLL_PA_PWR_PA_PWR_MASK                   (0x3FU)
#define ZLL_PA_PWR_PA_PWR_SHIFT                  (0U)
#define ZLL_PA_PWR_PA_PWR(x)                     (((uint32_t)(((uint32_t)(x)) << ZLL_PA_PWR_PA_PWR_SHIFT)) & ZLL_PA_PWR_PA_PWR_MASK)

/*! @name CHANNEL_NUM0 - CHANNEL NUMBER 0 */
#define ZLL_CHANNEL_NUM0_CHANNEL_NUM0_MASK       (0x7FU)
#define ZLL_CHANNEL_NUM0_CHANNEL_NUM0_SHIFT      (0U)
#define ZLL_CHANNEL_NUM0_CHANNEL_NUM0(x)         (((uint32_t)(((uint32_t)(x)) << ZLL_CHANNEL_NUM0_CHANNEL_NUM0_SHIFT)) & ZLL_CHANNEL_NUM0_CHANNEL_NUM0_MASK)

/*! @name LQI_AND_RSSI - LQI AND RSSI */
#define ZLL_LQI_AND_RSSI_LQI_VALUE_MASK          (0xFFU)
#define ZLL_LQI_AND_RSSI_LQI_VALUE_SHIFT         (0U)
#define ZLL_LQI_AND_RSSI_LQI_VALUE(x)            (((uint32_t)(((uint32_t)(x)) << ZLL_LQI_AND_RSSI_LQI_VALUE_SHIFT)) & ZLL_LQI_AND_RSSI_LQI_VALUE_MASK)
#define ZLL_LQI_AND_RSSI_RSSI_MASK               (0xFF00U)
#define ZLL_LQI_AND_RSSI_RSSI_SHIFT              (8U)
#define ZLL_LQI_AND_RSSI_RSSI(x)                 (((uint32_t)(((uint32_t)(x)) << ZLL_LQI_AND_RSSI_RSSI_SHIFT)) & ZLL_LQI_AND_RSSI_RSSI_MASK)
#define ZLL_LQI_AND_RSSI_CCA1_ED_FNL_MASK        (0xFF0000U)
#define ZLL_LQI_AND_RSSI_CCA1_ED_FNL_SHIFT       (16U)
#define ZLL_LQI_AND_RSSI_CCA1_ED_FNL(x)          (((uint32_t)(((uint32_t)(x)) << ZLL_LQI_AND_RSSI_CCA1_ED_FNL_SHIFT)) & ZLL_LQI_AND_RSSI_CCA1_ED_FNL_MASK)

/*! @name MACSHORTADDRS0 - MAC SHORT ADDRESS 0 */
#define ZLL_MACSHORTADDRS0_MACPANID0_MASK        (0xFFFFU)
#define ZLL_MACSHORTADDRS0_MACPANID0_SHIFT       (0U)
#define ZLL_MACSHORTADDRS0_MACPANID0(x)          (((uint32_t)(((uint32_t)(x)) << ZLL_MACSHORTADDRS0_MACPANID0_SHIFT)) & ZLL_MACSHORTADDRS0_MACPANID0_MASK)
#define ZLL_MACSHORTADDRS0_MACSHORTADDRS0_MASK   (0xFFFF0000U)
#define ZLL_MACSHORTADDRS0_MACSHORTADDRS0_SHIFT  (16U)
#define ZLL_MACSHORTADDRS0_MACSHORTADDRS0(x)     (((uint32_t)(((uint32_t)(x)) << ZLL_MACSHORTADDRS0_MACSHORTADDRS0_SHIFT)) & ZLL_MACSHORTADDRS0_MACSHORTADDRS0_MASK)

/*! @name MACLONGADDRS0_LSB - MAC LONG ADDRESS 0 LSB */
#define ZLL_MACLONGADDRS0_LSB_MACLONGADDRS0_LSB_MASK (0xFFFFFFFFU)
#define ZLL_MACLONGADDRS0_LSB_MACLONGADDRS0_LSB_SHIFT (0U)
#define ZLL_MACLONGADDRS0_LSB_MACLONGADDRS0_LSB(x) (((uint32_t)(((uint32_t)(x)) << ZLL_MACLONGADDRS0_LSB_MACLONGADDRS0_LSB_SHIFT)) & ZLL_MACLONGADDRS0_LSB_MACLONGADDRS0_LSB_MASK)

/*! @name MACLONGADDRS0_MSB - MAC LONG ADDRESS 0 MSB */
#define ZLL_MACLONGADDRS0_MSB_MACLONGADDRS0_MSB_MASK (0xFFFFFFFFU)
#define ZLL_MACLONGADDRS0_MSB_MACLONGADDRS0_MSB_SHIFT (0U)
#define ZLL_MACLONGADDRS0_MSB_MACLONGADDRS0_MSB(x) (((uint32_t)(((uint32_t)(x)) << ZLL_MACLONGADDRS0_MSB_MACLONGADDRS0_MSB_SHIFT)) & ZLL_MACLONGADDRS0_MSB_MACLONGADDRS0_MSB_MASK)

/*! @name RX_FRAME_FILTER - RECEIVE FRAME FILTER */
#define ZLL_RX_FRAME_FILTER_BEACON_FT_MASK       (0x1U)
#define ZLL_RX_FRAME_FILTER_BEACON_FT_SHIFT      (0U)
#define ZLL_RX_FRAME_FILTER_BEACON_FT(x)         (((uint32_t)(((uint32_t)(x)) << ZLL_RX_FRAME_FILTER_BEACON_FT_SHIFT)) & ZLL_RX_FRAME_FILTER_BEACON_FT_MASK)
#define ZLL_RX_FRAME_FILTER_DATA_FT_MASK         (0x2U)
#define ZLL_RX_FRAME_FILTER_DATA_FT_SHIFT        (1U)
#define ZLL_RX_FRAME_FILTER_DATA_FT(x)           (((uint32_t)(((uint32_t)(x)) << ZLL_RX_FRAME_FILTER_DATA_FT_SHIFT)) & ZLL_RX_FRAME_FILTER_DATA_FT_MASK)
#define ZLL_RX_FRAME_FILTER_ACK_FT_MASK          (0x4U)
#define ZLL_RX_FRAME_FILTER_ACK_FT_SHIFT         (2U)
#define ZLL_RX_FRAME_FILTER_ACK_FT(x)            (((uint32_t)(((uint32_t)(x)) << ZLL_RX_FRAME_FILTER_ACK_FT_SHIFT)) & ZLL_RX_FRAME_FILTER_ACK_FT_MASK)
#define ZLL_RX_FRAME_FILTER_CMD_FT_MASK          (0x8U)
#define ZLL_RX_FRAME_FILTER_CMD_FT_SHIFT         (3U)
#define ZLL_RX_FRAME_FILTER_CMD_FT(x)            (((uint32_t)(((uint32_t)(x)) << ZLL_RX_FRAME_FILTER_CMD_FT_SHIFT)) & ZLL_RX_FRAME_FILTER_CMD_FT_MASK)
#define ZLL_RX_FRAME_FILTER_LLDN_FT_MASK         (0x10U)
#define ZLL_RX_FRAME_FILTER_LLDN_FT_SHIFT        (4U)
#define ZLL_RX_FRAME_FILTER_LLDN_FT(x)           (((uint32_t)(((uint32_t)(x)) << ZLL_RX_FRAME_FILTER_LLDN_FT_SHIFT)) & ZLL_RX_FRAME_FILTER_LLDN_FT_MASK)
#define ZLL_RX_FRAME_FILTER_MULTIPURPOSE_FT_MASK (0x20U)
#define ZLL_RX_FRAME_FILTER_MULTIPURPOSE_FT_SHIFT (5U)
#define ZLL_RX_FRAME_FILTER_MULTIPURPOSE_FT(x)   (((uint32_t)(((uint32_t)(x)) << ZLL_RX_FRAME_FILTER_MULTIPURPOSE_FT_SHIFT)) & ZLL_RX_FRAME_FILTER_MULTIPURPOSE_FT_MASK)
#define ZLL_RX_FRAME_FILTER_NS_FT_MASK           (0x40U)
#define ZLL_RX_FRAME_FILTER_NS_FT_SHIFT          (6U)
#define ZLL_RX_FRAME_FILTER_NS_FT(x)             (((uint32_t)(((uint32_t)(x)) << ZLL_RX_FRAME_FILTER_NS_FT_SHIFT)) & ZLL_RX_FRAME_FILTER_NS_FT_MASK)
#define ZLL_RX_FRAME_FILTER_EXTENDED_FT_MASK     (0x80U)
#define ZLL_RX_FRAME_FILTER_EXTENDED_FT_SHIFT    (7U)
#define ZLL_RX_FRAME_FILTER_EXTENDED_FT(x)       (((uint32_t)(((uint32_t)(x)) << ZLL_RX_FRAME_FILTER_EXTENDED_FT_SHIFT)) & ZLL_RX_FRAME_FILTER_EXTENDED_FT_MASK)
#define ZLL_RX_FRAME_FILTER_FRM_VER_FILTER_MASK  (0xF00U)
#define ZLL_RX_FRAME_FILTER_FRM_VER_FILTER_SHIFT (8U)
#define ZLL_RX_FRAME_FILTER_FRM_VER_FILTER(x)    (((uint32_t)(((uint32_t)(x)) << ZLL_RX_FRAME_FILTER_FRM_VER_FILTER_SHIFT)) & ZLL_RX_FRAME_FILTER_FRM_VER_FILTER_MASK)
#define ZLL_RX_FRAME_FILTER_ACTIVE_PROMISCUOUS_MASK (0x4000U)
#define ZLL_RX_FRAME_FILTER_ACTIVE_PROMISCUOUS_SHIFT (14U)
#define ZLL_RX_FRAME_FILTER_ACTIVE_PROMISCUOUS(x) (((uint32_t)(((uint32_t)(x)) << ZLL_RX_FRAME_FILTER_ACTIVE_PROMISCUOUS_SHIFT)) & ZLL_RX_FRAME_FILTER_ACTIVE_PROMISCUOUS_MASK)
#define ZLL_RX_FRAME_FILTER_EXTENDED_FCS_CHK_MASK (0x8000U)
#define ZLL_RX_FRAME_FILTER_EXTENDED_FCS_CHK_SHIFT (15U)
#define ZLL_RX_FRAME_FILTER_EXTENDED_FCS_CHK(x)  (((uint32_t)(((uint32_t)(x)) << ZLL_RX_FRAME_FILTER_EXTENDED_FCS_CHK_SHIFT)) & ZLL_RX_FRAME_FILTER_EXTENDED_FCS_CHK_MASK)
#define ZLL_RX_FRAME_FILTER_FV2_BEACON_RECD_MASK (0x10000U)
#define ZLL_RX_FRAME_FILTER_FV2_BEACON_RECD_SHIFT (16U)
#define ZLL_RX_FRAME_FILTER_FV2_BEACON_RECD(x)   (((uint32_t)(((uint32_t)(x)) << ZLL_RX_FRAME_FILTER_FV2_BEACON_RECD_SHIFT)) & ZLL_RX_FRAME_FILTER_FV2_BEACON_RECD_MASK)
#define ZLL_RX_FRAME_FILTER_FV2_DATA_RECD_MASK   (0x20000U)
#define ZLL_RX_FRAME_FILTER_FV2_DATA_RECD_SHIFT  (17U)
#define ZLL_RX_FRAME_FILTER_FV2_DATA_RECD(x)     (((uint32_t)(((uint32_t)(x)) << ZLL_RX_FRAME_FILTER_FV2_DATA_RECD_SHIFT)) & ZLL_RX_FRAME_FILTER_FV2_DATA_RECD_MASK)
#define ZLL_RX_FRAME_FILTER_FV2_ACK_RECD_MASK    (0x40000U)
#define ZLL_RX_FRAME_FILTER_FV2_ACK_RECD_SHIFT   (18U)
#define ZLL_RX_FRAME_FILTER_FV2_ACK_RECD(x)      (((uint32_t)(((uint32_t)(x)) << ZLL_RX_FRAME_FILTER_FV2_ACK_RECD_SHIFT)) & ZLL_RX_FRAME_FILTER_FV2_ACK_RECD_MASK)
#define ZLL_RX_FRAME_FILTER_FV2_CMD_RECD_MASK    (0x80000U)
#define ZLL_RX_FRAME_FILTER_FV2_CMD_RECD_SHIFT   (19U)
#define ZLL_RX_FRAME_FILTER_FV2_CMD_RECD(x)      (((uint32_t)(((uint32_t)(x)) << ZLL_RX_FRAME_FILTER_FV2_CMD_RECD_SHIFT)) & ZLL_RX_FRAME_FILTER_FV2_CMD_RECD_MASK)
#define ZLL_RX_FRAME_FILTER_LLDN_RECD_MASK       (0x100000U)
#define ZLL_RX_FRAME_FILTER_LLDN_RECD_SHIFT      (20U)
#define ZLL_RX_FRAME_FILTER_LLDN_RECD(x)         (((uint32_t)(((uint32_t)(x)) << ZLL_RX_FRAME_FILTER_LLDN_RECD_SHIFT)) & ZLL_RX_FRAME_FILTER_LLDN_RECD_MASK)
#define ZLL_RX_FRAME_FILTER_MULTIPURPOSE_RECD_MASK (0x200000U)
#define ZLL_RX_FRAME_FILTER_MULTIPURPOSE_RECD_SHIFT (21U)
#define ZLL_RX_FRAME_FILTER_MULTIPURPOSE_RECD(x) (((uint32_t)(((uint32_t)(x)) << ZLL_RX_FRAME_FILTER_MULTIPURPOSE_RECD_SHIFT)) & ZLL_RX_FRAME_FILTER_MULTIPURPOSE_RECD_MASK)
#define ZLL_RX_FRAME_FILTER_EXTENDED_RECD_MASK   (0x800000U)
#define ZLL_RX_FRAME_FILTER_EXTENDED_RECD_SHIFT  (23U)
#define ZLL_RX_FRAME_FILTER_EXTENDED_RECD(x)     (((uint32_t)(((uint32_t)(x)) << ZLL_RX_FRAME_FILTER_EXTENDED_RECD_SHIFT)) & ZLL_RX_FRAME_FILTER_EXTENDED_RECD_MASK)

/*! @name CCA_LQI_CTRL - CCA AND LQI CONTROL */
#define ZLL_CCA_LQI_CTRL_CCA1_THRESH_MASK        (0xFFU)
#define ZLL_CCA_LQI_CTRL_CCA1_THRESH_SHIFT       (0U)
#define ZLL_CCA_LQI_CTRL_CCA1_THRESH(x)          (((uint32_t)(((uint32_t)(x)) << ZLL_CCA_LQI_CTRL_CCA1_THRESH_SHIFT)) & ZLL_CCA_LQI_CTRL_CCA1_THRESH_MASK)
#define ZLL_CCA_LQI_CTRL_LQI_OFFSET_COMP_MASK    (0xFF0000U)
#define ZLL_CCA_LQI_CTRL_LQI_OFFSET_COMP_SHIFT   (16U)
#define ZLL_CCA_LQI_CTRL_LQI_OFFSET_COMP(x)      (((uint32_t)(((uint32_t)(x)) << ZLL_CCA_LQI_CTRL_LQI_OFFSET_COMP_SHIFT)) & ZLL_CCA_LQI_CTRL_LQI_OFFSET_COMP_MASK)
#define ZLL_CCA_LQI_CTRL_CCA3_AND_NOT_OR_MASK    (0x8000000U)
#define ZLL_CCA_LQI_CTRL_CCA3_AND_NOT_OR_SHIFT   (27U)
#define ZLL_CCA_LQI_CTRL_CCA3_AND_NOT_OR(x)      (((uint32_t)(((uint32_t)(x)) << ZLL_CCA_LQI_CTRL_CCA3_AND_NOT_OR_SHIFT)) & ZLL_CCA_LQI_CTRL_CCA3_AND_NOT_OR_MASK)

/*! @name CCA2_CTRL - CCA2 CONTROL */
#define ZLL_CCA2_CTRL_CCA2_NUM_CORR_PEAKS_MASK   (0xFU)
#define ZLL_CCA2_CTRL_CCA2_NUM_CORR_PEAKS_SHIFT  (0U)
#define ZLL_CCA2_CTRL_CCA2_NUM_CORR_PEAKS(x)     (((uint32_t)(((uint32_t)(x)) << ZLL_CCA2_CTRL_CCA2_NUM_CORR_PEAKS_SHIFT)) & ZLL_CCA2_CTRL_CCA2_NUM_CORR_PEAKS_MASK)
#define ZLL_CCA2_CTRL_CCA2_MIN_NUM_CORR_TH_MASK  (0x70U)
#define ZLL_CCA2_CTRL_CCA2_MIN_NUM_CORR_TH_SHIFT (4U)
#define ZLL_CCA2_CTRL_CCA2_MIN_NUM_CORR_TH(x)    (((uint32_t)(((uint32_t)(x)) << ZLL_CCA2_CTRL_CCA2_MIN_NUM_CORR_TH_SHIFT)) & ZLL_CCA2_CTRL_CCA2_MIN_NUM_CORR_TH_MASK)
#define ZLL_CCA2_CTRL_CCA2_CORR_THRESH_MASK      (0xFF00U)
#define ZLL_CCA2_CTRL_CCA2_CORR_THRESH_SHIFT     (8U)
#define ZLL_CCA2_CTRL_CCA2_CORR_THRESH(x)        (((uint32_t)(((uint32_t)(x)) << ZLL_CCA2_CTRL_CCA2_CORR_THRESH_SHIFT)) & ZLL_CCA2_CTRL_CCA2_CORR_THRESH_MASK)

/*! @name DSM_CTRL - DSM CONTROL */
#define ZLL_DSM_CTRL_ZIGBEE_SLEEP_EN_MASK        (0x1U)
#define ZLL_DSM_CTRL_ZIGBEE_SLEEP_EN_SHIFT       (0U)
#define ZLL_DSM_CTRL_ZIGBEE_SLEEP_EN(x)          (((uint32_t)(((uint32_t)(x)) << ZLL_DSM_CTRL_ZIGBEE_SLEEP_EN_SHIFT)) & ZLL_DSM_CTRL_ZIGBEE_SLEEP_EN_MASK)

/*! @name BSM_CTRL - BSM CONTROL */
#define ZLL_BSM_CTRL_BSM_EN_MASK                 (0x1U)
#define ZLL_BSM_CTRL_BSM_EN_SHIFT                (0U)
#define ZLL_BSM_CTRL_BSM_EN(x)                   (((uint32_t)(((uint32_t)(x)) << ZLL_BSM_CTRL_BSM_EN_SHIFT)) & ZLL_BSM_CTRL_BSM_EN_MASK)

/*! @name MACSHORTADDRS1 - MAC SHORT ADDRESS FOR PAN1 */
#define ZLL_MACSHORTADDRS1_MACPANID1_MASK        (0xFFFFU)
#define ZLL_MACSHORTADDRS1_MACPANID1_SHIFT       (0U)
#define ZLL_MACSHORTADDRS1_MACPANID1(x)          (((uint32_t)(((uint32_t)(x)) << ZLL_MACSHORTADDRS1_MACPANID1_SHIFT)) & ZLL_MACSHORTADDRS1_MACPANID1_MASK)
#define ZLL_MACSHORTADDRS1_MACSHORTADDRS1_MASK   (0xFFFF0000U)
#define ZLL_MACSHORTADDRS1_MACSHORTADDRS1_SHIFT  (16U)
#define ZLL_MACSHORTADDRS1_MACSHORTADDRS1(x)     (((uint32_t)(((uint32_t)(x)) << ZLL_MACSHORTADDRS1_MACSHORTADDRS1_SHIFT)) & ZLL_MACSHORTADDRS1_MACSHORTADDRS1_MASK)

/*! @name MACLONGADDRS1_LSB - MAC LONG ADDRESS 1 LSB */
#define ZLL_MACLONGADDRS1_LSB_MACLONGADDRS1_LSB_MASK (0xFFFFFFFFU)
#define ZLL_MACLONGADDRS1_LSB_MACLONGADDRS1_LSB_SHIFT (0U)
#define ZLL_MACLONGADDRS1_LSB_MACLONGADDRS1_LSB(x) (((uint32_t)(((uint32_t)(x)) << ZLL_MACLONGADDRS1_LSB_MACLONGADDRS1_LSB_SHIFT)) & ZLL_MACLONGADDRS1_LSB_MACLONGADDRS1_LSB_MASK)

/*! @name MACLONGADDRS1_MSB - MAC LONG ADDRESS 1 MSB */
#define ZLL_MACLONGADDRS1_MSB_MACLONGADDRS1_MSB_MASK (0xFFFFFFFFU)
#define ZLL_MACLONGADDRS1_MSB_MACLONGADDRS1_MSB_SHIFT (0U)
#define ZLL_MACLONGADDRS1_MSB_MACLONGADDRS1_MSB(x) (((uint32_t)(((uint32_t)(x)) << ZLL_MACLONGADDRS1_MSB_MACLONGADDRS1_MSB_SHIFT)) & ZLL_MACLONGADDRS1_MSB_MACLONGADDRS1_MSB_MASK)

/*! @name DUAL_PAN_CTRL - DUAL PAN CONTROL */
#define ZLL_DUAL_PAN_CTRL_ACTIVE_NETWORK_MASK    (0x1U)
#define ZLL_DUAL_PAN_CTRL_ACTIVE_NETWORK_SHIFT   (0U)
#define ZLL_DUAL_PAN_CTRL_ACTIVE_NETWORK(x)      (((uint32_t)(((uint32_t)(x)) << ZLL_DUAL_PAN_CTRL_ACTIVE_NETWORK_SHIFT)) & ZLL_DUAL_PAN_CTRL_ACTIVE_NETWORK_MASK)
#define ZLL_DUAL_PAN_CTRL_DUAL_PAN_AUTO_MASK     (0x2U)
#define ZLL_DUAL_PAN_CTRL_DUAL_PAN_AUTO_SHIFT    (1U)
#define ZLL_DUAL_PAN_CTRL_DUAL_PAN_AUTO(x)       (((uint32_t)(((uint32_t)(x)) << ZLL_DUAL_PAN_CTRL_DUAL_PAN_AUTO_SHIFT)) & ZLL_DUAL_PAN_CTRL_DUAL_PAN_AUTO_MASK)
#define ZLL_DUAL_PAN_CTRL_PANCORDNTR1_MASK       (0x4U)
#define ZLL_DUAL_PAN_CTRL_PANCORDNTR1_SHIFT      (2U)
#define ZLL_DUAL_PAN_CTRL_PANCORDNTR1(x)         (((uint32_t)(((uint32_t)(x)) << ZLL_DUAL_PAN_CTRL_PANCORDNTR1_SHIFT)) & ZLL_DUAL_PAN_CTRL_PANCORDNTR1_MASK)
#define ZLL_DUAL_PAN_CTRL_CURRENT_NETWORK_MASK   (0x8U)
#define ZLL_DUAL_PAN_CTRL_CURRENT_NETWORK_SHIFT  (3U)
#define ZLL_DUAL_PAN_CTRL_CURRENT_NETWORK(x)     (((uint32_t)(((uint32_t)(x)) << ZLL_DUAL_PAN_CTRL_CURRENT_NETWORK_SHIFT)) & ZLL_DUAL_PAN_CTRL_CURRENT_NETWORK_MASK)
#define ZLL_DUAL_PAN_CTRL_ZB_DP_CHAN_OVRD_EN_MASK (0x10U)
#define ZLL_DUAL_PAN_CTRL_ZB_DP_CHAN_OVRD_EN_SHIFT (4U)
#define ZLL_DUAL_PAN_CTRL_ZB_DP_CHAN_OVRD_EN(x)  (((uint32_t)(((uint32_t)(x)) << ZLL_DUAL_PAN_CTRL_ZB_DP_CHAN_OVRD_EN_SHIFT)) & ZLL_DUAL_PAN_CTRL_ZB_DP_CHAN_OVRD_EN_MASK)
#define ZLL_DUAL_PAN_CTRL_ZB_DP_CHAN_OVRD_SEL_MASK (0x20U)
#define ZLL_DUAL_PAN_CTRL_ZB_DP_CHAN_OVRD_SEL_SHIFT (5U)
#define ZLL_DUAL_PAN_CTRL_ZB_DP_CHAN_OVRD_SEL(x) (((uint32_t)(((uint32_t)(x)) << ZLL_DUAL_PAN_CTRL_ZB_DP_CHAN_OVRD_SEL_SHIFT)) & ZLL_DUAL_PAN_CTRL_ZB_DP_CHAN_OVRD_SEL_MASK)
#define ZLL_DUAL_PAN_CTRL_DUAL_PAN_DWELL_MASK    (0xFF00U)
#define ZLL_DUAL_PAN_CTRL_DUAL_PAN_DWELL_SHIFT   (8U)
#define ZLL_DUAL_PAN_CTRL_DUAL_PAN_DWELL(x)      (((uint32_t)(((uint32_t)(x)) << ZLL_DUAL_PAN_CTRL_DUAL_PAN_DWELL_SHIFT)) & ZLL_DUAL_PAN_CTRL_DUAL_PAN_DWELL_MASK)
#define ZLL_DUAL_PAN_CTRL_DUAL_PAN_REMAIN_MASK   (0x3F0000U)
#define ZLL_DUAL_PAN_CTRL_DUAL_PAN_REMAIN_SHIFT  (16U)
#define ZLL_DUAL_PAN_CTRL_DUAL_PAN_REMAIN(x)     (((uint32_t)(((uint32_t)(x)) << ZLL_DUAL_PAN_CTRL_DUAL_PAN_REMAIN_SHIFT)) & ZLL_DUAL_PAN_CTRL_DUAL_PAN_REMAIN_MASK)
#define ZLL_DUAL_PAN_CTRL_RECD_ON_PAN0_MASK      (0x400000U)
#define ZLL_DUAL_PAN_CTRL_RECD_ON_PAN0_SHIFT     (22U)
#define ZLL_DUAL_PAN_CTRL_RECD_ON_PAN0(x)        (((uint32_t)(((uint32_t)(x)) << ZLL_DUAL_PAN_CTRL_RECD_ON_PAN0_SHIFT)) & ZLL_DUAL_PAN_CTRL_RECD_ON_PAN0_MASK)
#define ZLL_DUAL_PAN_CTRL_RECD_ON_PAN1_MASK      (0x800000U)
#define ZLL_DUAL_PAN_CTRL_RECD_ON_PAN1_SHIFT     (23U)
#define ZLL_DUAL_PAN_CTRL_RECD_ON_PAN1(x)        (((uint32_t)(((uint32_t)(x)) << ZLL_DUAL_PAN_CTRL_RECD_ON_PAN1_SHIFT)) & ZLL_DUAL_PAN_CTRL_RECD_ON_PAN1_MASK)

/*! @name CHANNEL_NUM1 - CHANNEL NUMBER 1 */
#define ZLL_CHANNEL_NUM1_CHANNEL_NUM1_MASK       (0x7FU)
#define ZLL_CHANNEL_NUM1_CHANNEL_NUM1_SHIFT      (0U)
#define ZLL_CHANNEL_NUM1_CHANNEL_NUM1(x)         (((uint32_t)(((uint32_t)(x)) << ZLL_CHANNEL_NUM1_CHANNEL_NUM1_SHIFT)) & ZLL_CHANNEL_NUM1_CHANNEL_NUM1_MASK)

/*! @name SAM_CTRL - SAM CONTROL */
#define ZLL_SAM_CTRL_SAP0_EN_MASK                (0x1U)
#define ZLL_SAM_CTRL_SAP0_EN_SHIFT               (0U)
#define ZLL_SAM_CTRL_SAP0_EN(x)                  (((uint32_t)(((uint32_t)(x)) << ZLL_SAM_CTRL_SAP0_EN_SHIFT)) & ZLL_SAM_CTRL_SAP0_EN_MASK)
#define ZLL_SAM_CTRL_SAA0_EN_MASK                (0x2U)
#define ZLL_SAM_CTRL_SAA0_EN_SHIFT               (1U)
#define ZLL_SAM_CTRL_SAA0_EN(x)                  (((uint32_t)(((uint32_t)(x)) << ZLL_SAM_CTRL_SAA0_EN_SHIFT)) & ZLL_SAM_CTRL_SAA0_EN_MASK)
#define ZLL_SAM_CTRL_SAP1_EN_MASK                (0x4U)
#define ZLL_SAM_CTRL_SAP1_EN_SHIFT               (2U)
#define ZLL_SAM_CTRL_SAP1_EN(x)                  (((uint32_t)(((uint32_t)(x)) << ZLL_SAM_CTRL_SAP1_EN_SHIFT)) & ZLL_SAM_CTRL_SAP1_EN_MASK)
#define ZLL_SAM_CTRL_SAA1_EN_MASK                (0x8U)
#define ZLL_SAM_CTRL_SAA1_EN_SHIFT               (3U)
#define ZLL_SAM_CTRL_SAA1_EN(x)                  (((uint32_t)(((uint32_t)(x)) << ZLL_SAM_CTRL_SAA1_EN_SHIFT)) & ZLL_SAM_CTRL_SAA1_EN_MASK)
#define ZLL_SAM_CTRL_SAA0_START_MASK             (0xFF00U)
#define ZLL_SAM_CTRL_SAA0_START_SHIFT            (8U)
#define ZLL_SAM_CTRL_SAA0_START(x)               (((uint32_t)(((uint32_t)(x)) << ZLL_SAM_CTRL_SAA0_START_SHIFT)) & ZLL_SAM_CTRL_SAA0_START_MASK)
#define ZLL_SAM_CTRL_SAP1_START_MASK             (0xFF0000U)
#define ZLL_SAM_CTRL_SAP1_START_SHIFT            (16U)
#define ZLL_SAM_CTRL_SAP1_START(x)               (((uint32_t)(((uint32_t)(x)) << ZLL_SAM_CTRL_SAP1_START_SHIFT)) & ZLL_SAM_CTRL_SAP1_START_MASK)
#define ZLL_SAM_CTRL_SAA1_START_MASK             (0xFF000000U)
#define ZLL_SAM_CTRL_SAA1_START_SHIFT            (24U)
#define ZLL_SAM_CTRL_SAA1_START(x)               (((uint32_t)(((uint32_t)(x)) << ZLL_SAM_CTRL_SAA1_START_SHIFT)) & ZLL_SAM_CTRL_SAA1_START_MASK)

/*! @name SAM_TABLE - SOURCE ADDRESS MANAGEMENT TABLE */
#define ZLL_SAM_TABLE_SAM_INDEX_MASK             (0x7FU)
#define ZLL_SAM_TABLE_SAM_INDEX_SHIFT            (0U)
#define ZLL_SAM_TABLE_SAM_INDEX(x)               (((uint32_t)(((uint32_t)(x)) << ZLL_SAM_TABLE_SAM_INDEX_SHIFT)) & ZLL_SAM_TABLE_SAM_INDEX_MASK)
#define ZLL_SAM_TABLE_SAM_INDEX_WR_MASK          (0x80U)
#define ZLL_SAM_TABLE_SAM_INDEX_WR_SHIFT         (7U)
#define ZLL_SAM_TABLE_SAM_INDEX_WR(x)            (((uint32_t)(((uint32_t)(x)) << ZLL_SAM_TABLE_SAM_INDEX_WR_SHIFT)) & ZLL_SAM_TABLE_SAM_INDEX_WR_MASK)
#define ZLL_SAM_TABLE_SAM_CHECKSUM_MASK          (0xFFFF00U)
#define ZLL_SAM_TABLE_SAM_CHECKSUM_SHIFT         (8U)
#define ZLL_SAM_TABLE_SAM_CHECKSUM(x)            (((uint32_t)(((uint32_t)(x)) << ZLL_SAM_TABLE_SAM_CHECKSUM_SHIFT)) & ZLL_SAM_TABLE_SAM_CHECKSUM_MASK)
#define ZLL_SAM_TABLE_SAM_INDEX_INV_MASK         (0x1000000U)
#define ZLL_SAM_TABLE_SAM_INDEX_INV_SHIFT        (24U)
#define ZLL_SAM_TABLE_SAM_INDEX_INV(x)           (((uint32_t)(((uint32_t)(x)) << ZLL_SAM_TABLE_SAM_INDEX_INV_SHIFT)) & ZLL_SAM_TABLE_SAM_INDEX_INV_MASK)
#define ZLL_SAM_TABLE_SAM_INDEX_EN_MASK          (0x2000000U)
#define ZLL_SAM_TABLE_SAM_INDEX_EN_SHIFT         (25U)
#define ZLL_SAM_TABLE_SAM_INDEX_EN(x)            (((uint32_t)(((uint32_t)(x)) << ZLL_SAM_TABLE_SAM_INDEX_EN_SHIFT)) & ZLL_SAM_TABLE_SAM_INDEX_EN_MASK)
#define ZLL_SAM_TABLE_ACK_FRM_PND_MASK           (0x4000000U)
#define ZLL_SAM_TABLE_ACK_FRM_PND_SHIFT          (26U)
#define ZLL_SAM_TABLE_ACK_FRM_PND(x)             (((uint32_t)(((uint32_t)(x)) << ZLL_SAM_TABLE_ACK_FRM_PND_SHIFT)) & ZLL_SAM_TABLE_ACK_FRM_PND_MASK)
#define ZLL_SAM_TABLE_ACK_FRM_PND_CTRL_MASK      (0x8000000U)
#define ZLL_SAM_TABLE_ACK_FRM_PND_CTRL_SHIFT     (27U)
#define ZLL_SAM_TABLE_ACK_FRM_PND_CTRL(x)        (((uint32_t)(((uint32_t)(x)) << ZLL_SAM_TABLE_ACK_FRM_PND_CTRL_SHIFT)) & ZLL_SAM_TABLE_ACK_FRM_PND_CTRL_MASK)
#define ZLL_SAM_TABLE_FIND_FREE_IDX_MASK         (0x10000000U)
#define ZLL_SAM_TABLE_FIND_FREE_IDX_SHIFT        (28U)
#define ZLL_SAM_TABLE_FIND_FREE_IDX(x)           (((uint32_t)(((uint32_t)(x)) << ZLL_SAM_TABLE_FIND_FREE_IDX_SHIFT)) & ZLL_SAM_TABLE_FIND_FREE_IDX_MASK)
#define ZLL_SAM_TABLE_INVALIDATE_ALL_MASK        (0x20000000U)
#define ZLL_SAM_TABLE_INVALIDATE_ALL_SHIFT       (29U)
#define ZLL_SAM_TABLE_INVALIDATE_ALL(x)          (((uint32_t)(((uint32_t)(x)) << ZLL_SAM_TABLE_INVALIDATE_ALL_SHIFT)) & ZLL_SAM_TABLE_INVALIDATE_ALL_MASK)
#define ZLL_SAM_TABLE_SAM_BUSY_MASK              (0x80000000U)
#define ZLL_SAM_TABLE_SAM_BUSY_SHIFT             (31U)
#define ZLL_SAM_TABLE_SAM_BUSY(x)                (((uint32_t)(((uint32_t)(x)) << ZLL_SAM_TABLE_SAM_BUSY_SHIFT)) & ZLL_SAM_TABLE_SAM_BUSY_MASK)

/*! @name SAM_MATCH - SOURCE ADDRESS MANAGEMENT MATCH */
#define ZLL_SAM_MATCH_SAP0_MATCH_MASK            (0x7FU)
#define ZLL_SAM_MATCH_SAP0_MATCH_SHIFT           (0U)
#define ZLL_SAM_MATCH_SAP0_MATCH(x)              (((uint32_t)(((uint32_t)(x)) << ZLL_SAM_MATCH_SAP0_MATCH_SHIFT)) & ZLL_SAM_MATCH_SAP0_MATCH_MASK)
#define ZLL_SAM_MATCH_SAP0_ADDR_PRESENT_MASK     (0x80U)
#define ZLL_SAM_MATCH_SAP0_ADDR_PRESENT_SHIFT    (7U)
#define ZLL_SAM_MATCH_SAP0_ADDR_PRESENT(x)       (((uint32_t)(((uint32_t)(x)) << ZLL_SAM_MATCH_SAP0_ADDR_PRESENT_SHIFT)) & ZLL_SAM_MATCH_SAP0_ADDR_PRESENT_MASK)
#define ZLL_SAM_MATCH_SAA0_MATCH_MASK            (0x7F00U)
#define ZLL_SAM_MATCH_SAA0_MATCH_SHIFT           (8U)
#define ZLL_SAM_MATCH_SAA0_MATCH(x)              (((uint32_t)(((uint32_t)(x)) << ZLL_SAM_MATCH_SAA0_MATCH_SHIFT)) & ZLL_SAM_MATCH_SAA0_MATCH_MASK)
#define ZLL_SAM_MATCH_SAA0_ADDR_ABSENT_MASK      (0x8000U)
#define ZLL_SAM_MATCH_SAA0_ADDR_ABSENT_SHIFT     (15U)
#define ZLL_SAM_MATCH_SAA0_ADDR_ABSENT(x)        (((uint32_t)(((uint32_t)(x)) << ZLL_SAM_MATCH_SAA0_ADDR_ABSENT_SHIFT)) & ZLL_SAM_MATCH_SAA0_ADDR_ABSENT_MASK)
#define ZLL_SAM_MATCH_SAP1_MATCH_MASK            (0x7F0000U)
#define ZLL_SAM_MATCH_SAP1_MATCH_SHIFT           (16U)
#define ZLL_SAM_MATCH_SAP1_MATCH(x)              (((uint32_t)(((uint32_t)(x)) << ZLL_SAM_MATCH_SAP1_MATCH_SHIFT)) & ZLL_SAM_MATCH_SAP1_MATCH_MASK)
#define ZLL_SAM_MATCH_SAP1_ADDR_PRESENT_MASK     (0x800000U)
#define ZLL_SAM_MATCH_SAP1_ADDR_PRESENT_SHIFT    (23U)
#define ZLL_SAM_MATCH_SAP1_ADDR_PRESENT(x)       (((uint32_t)(((uint32_t)(x)) << ZLL_SAM_MATCH_SAP1_ADDR_PRESENT_SHIFT)) & ZLL_SAM_MATCH_SAP1_ADDR_PRESENT_MASK)
#define ZLL_SAM_MATCH_SAA1_MATCH_MASK            (0x7F000000U)
#define ZLL_SAM_MATCH_SAA1_MATCH_SHIFT           (24U)
#define ZLL_SAM_MATCH_SAA1_MATCH(x)              (((uint32_t)(((uint32_t)(x)) << ZLL_SAM_MATCH_SAA1_MATCH_SHIFT)) & ZLL_SAM_MATCH_SAA1_MATCH_MASK)
#define ZLL_SAM_MATCH_SAA1_ADDR_ABSENT_MASK      (0x80000000U)
#define ZLL_SAM_MATCH_SAA1_ADDR_ABSENT_SHIFT     (31U)
#define ZLL_SAM_MATCH_SAA1_ADDR_ABSENT(x)        (((uint32_t)(((uint32_t)(x)) << ZLL_SAM_MATCH_SAA1_ADDR_ABSENT_SHIFT)) & ZLL_SAM_MATCH_SAA1_ADDR_ABSENT_MASK)

/*! @name SAM_FREE_IDX - SAM FREE INDEX */
#define ZLL_SAM_FREE_IDX_SAP0_1ST_FREE_IDX_MASK  (0xFFU)
#define ZLL_SAM_FREE_IDX_SAP0_1ST_FREE_IDX_SHIFT (0U)
#define ZLL_SAM_FREE_IDX_SAP0_1ST_FREE_IDX(x)    (((uint32_t)(((uint32_t)(x)) << ZLL_SAM_FREE_IDX_SAP0_1ST_FREE_IDX_SHIFT)) & ZLL_SAM_FREE_IDX_SAP0_1ST_FREE_IDX_MASK)
#define ZLL_SAM_FREE_IDX_SAA0_1ST_FREE_IDX_MASK  (0xFF00U)
#define ZLL_SAM_FREE_IDX_SAA0_1ST_FREE_IDX_SHIFT (8U)
#define ZLL_SAM_FREE_IDX_SAA0_1ST_FREE_IDX(x)    (((uint32_t)(((uint32_t)(x)) << ZLL_SAM_FREE_IDX_SAA0_1ST_FREE_IDX_SHIFT)) & ZLL_SAM_FREE_IDX_SAA0_1ST_FREE_IDX_MASK)
#define ZLL_SAM_FREE_IDX_SAP1_1ST_FREE_IDX_MASK  (0xFF0000U)
#define ZLL_SAM_FREE_IDX_SAP1_1ST_FREE_IDX_SHIFT (16U)
#define ZLL_SAM_FREE_IDX_SAP1_1ST_FREE_IDX(x)    (((uint32_t)(((uint32_t)(x)) << ZLL_SAM_FREE_IDX_SAP1_1ST_FREE_IDX_SHIFT)) & ZLL_SAM_FREE_IDX_SAP1_1ST_FREE_IDX_MASK)
#define ZLL_SAM_FREE_IDX_SAA1_1ST_FREE_IDX_MASK  (0xFF000000U)
#define ZLL_SAM_FREE_IDX_SAA1_1ST_FREE_IDX_SHIFT (24U)
#define ZLL_SAM_FREE_IDX_SAA1_1ST_FREE_IDX(x)    (((uint32_t)(((uint32_t)(x)) << ZLL_SAM_FREE_IDX_SAA1_1ST_FREE_IDX_SHIFT)) & ZLL_SAM_FREE_IDX_SAA1_1ST_FREE_IDX_MASK)

/*! @name SEQ_CTRL_STS - SEQUENCE CONTROL AND STATUS */
#define ZLL_SEQ_CTRL_STS_CLR_NEW_SEQ_INHIBIT_MASK (0x4U)
#define ZLL_SEQ_CTRL_STS_CLR_NEW_SEQ_INHIBIT_SHIFT (2U)
#define ZLL_SEQ_CTRL_STS_CLR_NEW_SEQ_INHIBIT(x)  (((uint32_t)(((uint32_t)(x)) << ZLL_SEQ_CTRL_STS_CLR_NEW_SEQ_INHIBIT_SHIFT)) & ZLL_SEQ_CTRL_STS_CLR_NEW_SEQ_INHIBIT_MASK)
#define ZLL_SEQ_CTRL_STS_EVENT_TMR_DO_NOT_LATCH_MASK (0x8U)
#define ZLL_SEQ_CTRL_STS_EVENT_TMR_DO_NOT_LATCH_SHIFT (3U)
#define ZLL_SEQ_CTRL_STS_EVENT_TMR_DO_NOT_LATCH(x) (((uint32_t)(((uint32_t)(x)) << ZLL_SEQ_CTRL_STS_EVENT_TMR_DO_NOT_LATCH_SHIFT)) & ZLL_SEQ_CTRL_STS_EVENT_TMR_DO_NOT_LATCH_MASK)
#define ZLL_SEQ_CTRL_STS_LATCH_PREAMBLE_MASK     (0x10U)
#define ZLL_SEQ_CTRL_STS_LATCH_PREAMBLE_SHIFT    (4U)
#define ZLL_SEQ_CTRL_STS_LATCH_PREAMBLE(x)       (((uint32_t)(((uint32_t)(x)) << ZLL_SEQ_CTRL_STS_LATCH_PREAMBLE_SHIFT)) & ZLL_SEQ_CTRL_STS_LATCH_PREAMBLE_MASK)
#define ZLL_SEQ_CTRL_STS_NO_RX_RECYCLE_MASK      (0x20U)
#define ZLL_SEQ_CTRL_STS_NO_RX_RECYCLE_SHIFT     (5U)
#define ZLL_SEQ_CTRL_STS_NO_RX_RECYCLE(x)        (((uint32_t)(((uint32_t)(x)) << ZLL_SEQ_CTRL_STS_NO_RX_RECYCLE_SHIFT)) & ZLL_SEQ_CTRL_STS_NO_RX_RECYCLE_MASK)
#define ZLL_SEQ_CTRL_STS_FORCE_CRC_ERROR_MASK    (0x40U)
#define ZLL_SEQ_CTRL_STS_FORCE_CRC_ERROR_SHIFT   (6U)
#define ZLL_SEQ_CTRL_STS_FORCE_CRC_ERROR(x)      (((uint32_t)(((uint32_t)(x)) << ZLL_SEQ_CTRL_STS_FORCE_CRC_ERROR_SHIFT)) & ZLL_SEQ_CTRL_STS_FORCE_CRC_ERROR_MASK)
#define ZLL_SEQ_CTRL_STS_CONTINUOUS_EN_MASK      (0x80U)
#define ZLL_SEQ_CTRL_STS_CONTINUOUS_EN_SHIFT     (7U)
#define ZLL_SEQ_CTRL_STS_CONTINUOUS_EN(x)        (((uint32_t)(((uint32_t)(x)) << ZLL_SEQ_CTRL_STS_CONTINUOUS_EN_SHIFT)) & ZLL_SEQ_CTRL_STS_CONTINUOUS_EN_MASK)
#define ZLL_SEQ_CTRL_STS_XCVSEQ_ACTUAL_MASK      (0x700U)
#define ZLL_SEQ_CTRL_STS_XCVSEQ_ACTUAL_SHIFT     (8U)
#define ZLL_SEQ_CTRL_STS_XCVSEQ_ACTUAL(x)        (((uint32_t)(((uint32_t)(x)) << ZLL_SEQ_CTRL_STS_XCVSEQ_ACTUAL_SHIFT)) & ZLL_SEQ_CTRL_STS_XCVSEQ_ACTUAL_MASK)
#define ZLL_SEQ_CTRL_STS_SEQ_IDLE_MASK           (0x800U)
#define ZLL_SEQ_CTRL_STS_SEQ_IDLE_SHIFT          (11U)
#define ZLL_SEQ_CTRL_STS_SEQ_IDLE(x)             (((uint32_t)(((uint32_t)(x)) << ZLL_SEQ_CTRL_STS_SEQ_IDLE_SHIFT)) & ZLL_SEQ_CTRL_STS_SEQ_IDLE_MASK)
#define ZLL_SEQ_CTRL_STS_NEW_SEQ_INHIBIT_MASK    (0x1000U)
#define ZLL_SEQ_CTRL_STS_NEW_SEQ_INHIBIT_SHIFT   (12U)
#define ZLL_SEQ_CTRL_STS_NEW_SEQ_INHIBIT(x)      (((uint32_t)(((uint32_t)(x)) << ZLL_SEQ_CTRL_STS_NEW_SEQ_INHIBIT_SHIFT)) & ZLL_SEQ_CTRL_STS_NEW_SEQ_INHIBIT_MASK)
#define ZLL_SEQ_CTRL_STS_RX_TIMEOUT_PENDING_MASK (0x2000U)
#define ZLL_SEQ_CTRL_STS_RX_TIMEOUT_PENDING_SHIFT (13U)
#define ZLL_SEQ_CTRL_STS_RX_TIMEOUT_PENDING(x)   (((uint32_t)(((uint32_t)(x)) << ZLL_SEQ_CTRL_STS_RX_TIMEOUT_PENDING_SHIFT)) & ZLL_SEQ_CTRL_STS_RX_TIMEOUT_PENDING_MASK)
#define ZLL_SEQ_CTRL_STS_RX_MODE_MASK            (0x4000U)
#define ZLL_SEQ_CTRL_STS_RX_MODE_SHIFT           (14U)
#define ZLL_SEQ_CTRL_STS_RX_MODE(x)              (((uint32_t)(((uint32_t)(x)) << ZLL_SEQ_CTRL_STS_RX_MODE_SHIFT)) & ZLL_SEQ_CTRL_STS_RX_MODE_MASK)
#define ZLL_SEQ_CTRL_STS_TMR2_SEQ_TRIG_ARMED_MASK (0x8000U)
#define ZLL_SEQ_CTRL_STS_TMR2_SEQ_TRIG_ARMED_SHIFT (15U)
#define ZLL_SEQ_CTRL_STS_TMR2_SEQ_TRIG_ARMED(x)  (((uint32_t)(((uint32_t)(x)) << ZLL_SEQ_CTRL_STS_TMR2_SEQ_TRIG_ARMED_SHIFT)) & ZLL_SEQ_CTRL_STS_TMR2_SEQ_TRIG_ARMED_MASK)
#define ZLL_SEQ_CTRL_STS_SEQ_T_STATUS_MASK       (0x3F0000U)
#define ZLL_SEQ_CTRL_STS_SEQ_T_STATUS_SHIFT      (16U)
#define ZLL_SEQ_CTRL_STS_SEQ_T_STATUS(x)         (((uint32_t)(((uint32_t)(x)) << ZLL_SEQ_CTRL_STS_SEQ_T_STATUS_SHIFT)) & ZLL_SEQ_CTRL_STS_SEQ_T_STATUS_MASK)
#define ZLL_SEQ_CTRL_STS_SW_ABORTED_MASK         (0x1000000U)
#define ZLL_SEQ_CTRL_STS_SW_ABORTED_SHIFT        (24U)
#define ZLL_SEQ_CTRL_STS_SW_ABORTED(x)           (((uint32_t)(((uint32_t)(x)) << ZLL_SEQ_CTRL_STS_SW_ABORTED_SHIFT)) & ZLL_SEQ_CTRL_STS_SW_ABORTED_MASK)
#define ZLL_SEQ_CTRL_STS_TC3_ABORTED_MASK        (0x2000000U)
#define ZLL_SEQ_CTRL_STS_TC3_ABORTED_SHIFT       (25U)
#define ZLL_SEQ_CTRL_STS_TC3_ABORTED(x)          (((uint32_t)(((uint32_t)(x)) << ZLL_SEQ_CTRL_STS_TC3_ABORTED_SHIFT)) & ZLL_SEQ_CTRL_STS_TC3_ABORTED_MASK)
#define ZLL_SEQ_CTRL_STS_PLL_ABORTED_MASK        (0x4000000U)
#define ZLL_SEQ_CTRL_STS_PLL_ABORTED_SHIFT       (26U)
#define ZLL_SEQ_CTRL_STS_PLL_ABORTED(x)          (((uint32_t)(((uint32_t)(x)) << ZLL_SEQ_CTRL_STS_PLL_ABORTED_SHIFT)) & ZLL_SEQ_CTRL_STS_PLL_ABORTED_MASK)

/*! @name ACKDELAY - ACK DELAY */
#define ZLL_ACKDELAY_ACKDELAY_MASK               (0x3FU)
#define ZLL_ACKDELAY_ACKDELAY_SHIFT              (0U)
#define ZLL_ACKDELAY_ACKDELAY(x)                 (((uint32_t)(((uint32_t)(x)) << ZLL_ACKDELAY_ACKDELAY_SHIFT)) & ZLL_ACKDELAY_ACKDELAY_MASK)
#define ZLL_ACKDELAY_TXDELAY_MASK                (0x3F00U)
#define ZLL_ACKDELAY_TXDELAY_SHIFT               (8U)
#define ZLL_ACKDELAY_TXDELAY(x)                  (((uint32_t)(((uint32_t)(x)) << ZLL_ACKDELAY_TXDELAY_SHIFT)) & ZLL_ACKDELAY_TXDELAY_MASK)

/*! @name FILTERFAIL_CODE - FILTER FAIL CODE */
#define ZLL_FILTERFAIL_CODE_FILTERFAIL_CODE_MASK (0x3FFU)
#define ZLL_FILTERFAIL_CODE_FILTERFAIL_CODE_SHIFT (0U)
#define ZLL_FILTERFAIL_CODE_FILTERFAIL_CODE(x)   (((uint32_t)(((uint32_t)(x)) << ZLL_FILTERFAIL_CODE_FILTERFAIL_CODE_SHIFT)) & ZLL_FILTERFAIL_CODE_FILTERFAIL_CODE_MASK)
#define ZLL_FILTERFAIL_CODE_FILTERFAIL_PAN_SEL_MASK (0x8000U)
#define ZLL_FILTERFAIL_CODE_FILTERFAIL_PAN_SEL_SHIFT (15U)
#define ZLL_FILTERFAIL_CODE_FILTERFAIL_PAN_SEL(x) (((uint32_t)(((uint32_t)(x)) << ZLL_FILTERFAIL_CODE_FILTERFAIL_PAN_SEL_SHIFT)) & ZLL_FILTERFAIL_CODE_FILTERFAIL_PAN_SEL_MASK)

/*! @name RX_WTR_MARK - RECEIVE WATER MARK */
#define ZLL_RX_WTR_MARK_RX_WTR_MARK_MASK         (0xFFU)
#define ZLL_RX_WTR_MARK_RX_WTR_MARK_SHIFT        (0U)
#define ZLL_RX_WTR_MARK_RX_WTR_MARK(x)           (((uint32_t)(((uint32_t)(x)) << ZLL_RX_WTR_MARK_RX_WTR_MARK_SHIFT)) & ZLL_RX_WTR_MARK_RX_WTR_MARK_MASK)

/*! @name SLOT_PRELOAD - SLOT PRELOAD */
#define ZLL_SLOT_PRELOAD_SLOT_PRELOAD_MASK       (0xFFU)
#define ZLL_SLOT_PRELOAD_SLOT_PRELOAD_SHIFT      (0U)
#define ZLL_SLOT_PRELOAD_SLOT_PRELOAD(x)         (((uint32_t)(((uint32_t)(x)) << ZLL_SLOT_PRELOAD_SLOT_PRELOAD_SHIFT)) & ZLL_SLOT_PRELOAD_SLOT_PRELOAD_MASK)

/*! @name SEQ_STATE - 802.15.4 SEQUENCE STATE */
#define ZLL_SEQ_STATE_SEQ_STATE_MASK             (0x1FU)
#define ZLL_SEQ_STATE_SEQ_STATE_SHIFT            (0U)
#define ZLL_SEQ_STATE_SEQ_STATE(x)               (((uint32_t)(((uint32_t)(x)) << ZLL_SEQ_STATE_SEQ_STATE_SHIFT)) & ZLL_SEQ_STATE_SEQ_STATE_MASK)
#define ZLL_SEQ_STATE_PREAMBLE_DET_MASK          (0x100U)
#define ZLL_SEQ_STATE_PREAMBLE_DET_SHIFT         (8U)
#define ZLL_SEQ_STATE_PREAMBLE_DET(x)            (((uint32_t)(((uint32_t)(x)) << ZLL_SEQ_STATE_PREAMBLE_DET_SHIFT)) & ZLL_SEQ_STATE_PREAMBLE_DET_MASK)
#define ZLL_SEQ_STATE_SFD_DET_MASK               (0x200U)
#define ZLL_SEQ_STATE_SFD_DET_SHIFT              (9U)
#define ZLL_SEQ_STATE_SFD_DET(x)                 (((uint32_t)(((uint32_t)(x)) << ZLL_SEQ_STATE_SFD_DET_SHIFT)) & ZLL_SEQ_STATE_SFD_DET_MASK)
#define ZLL_SEQ_STATE_FILTERFAIL_FLAG_SEL_MASK   (0x400U)
#define ZLL_SEQ_STATE_FILTERFAIL_FLAG_SEL_SHIFT  (10U)
#define ZLL_SEQ_STATE_FILTERFAIL_FLAG_SEL(x)     (((uint32_t)(((uint32_t)(x)) << ZLL_SEQ_STATE_FILTERFAIL_FLAG_SEL_SHIFT)) & ZLL_SEQ_STATE_FILTERFAIL_FLAG_SEL_MASK)
#define ZLL_SEQ_STATE_CRCVALID_MASK              (0x800U)
#define ZLL_SEQ_STATE_CRCVALID_SHIFT             (11U)
#define ZLL_SEQ_STATE_CRCVALID(x)                (((uint32_t)(((uint32_t)(x)) << ZLL_SEQ_STATE_CRCVALID_SHIFT)) & ZLL_SEQ_STATE_CRCVALID_MASK)
#define ZLL_SEQ_STATE_PLL_ABORT_MASK             (0x1000U)
#define ZLL_SEQ_STATE_PLL_ABORT_SHIFT            (12U)
#define ZLL_SEQ_STATE_PLL_ABORT(x)               (((uint32_t)(((uint32_t)(x)) << ZLL_SEQ_STATE_PLL_ABORT_SHIFT)) & ZLL_SEQ_STATE_PLL_ABORT_MASK)
#define ZLL_SEQ_STATE_PLL_ABORTED_MASK           (0x2000U)
#define ZLL_SEQ_STATE_PLL_ABORTED_SHIFT          (13U)
#define ZLL_SEQ_STATE_PLL_ABORTED(x)             (((uint32_t)(((uint32_t)(x)) << ZLL_SEQ_STATE_PLL_ABORTED_SHIFT)) & ZLL_SEQ_STATE_PLL_ABORTED_MASK)
#define ZLL_SEQ_STATE_RX_BYTE_COUNT_MASK         (0xFF0000U)
#define ZLL_SEQ_STATE_RX_BYTE_COUNT_SHIFT        (16U)
#define ZLL_SEQ_STATE_RX_BYTE_COUNT(x)           (((uint32_t)(((uint32_t)(x)) << ZLL_SEQ_STATE_RX_BYTE_COUNT_SHIFT)) & ZLL_SEQ_STATE_RX_BYTE_COUNT_MASK)
#define ZLL_SEQ_STATE_CCCA_BUSY_CNT_MASK         (0x3F000000U)
#define ZLL_SEQ_STATE_CCCA_BUSY_CNT_SHIFT        (24U)
#define ZLL_SEQ_STATE_CCCA_BUSY_CNT(x)           (((uint32_t)(((uint32_t)(x)) << ZLL_SEQ_STATE_CCCA_BUSY_CNT_SHIFT)) & ZLL_SEQ_STATE_CCCA_BUSY_CNT_MASK)

/*! @name TMR_PRESCALE - TIMER PRESCALER */
#define ZLL_TMR_PRESCALE_TMR_PRESCALE_MASK       (0x7U)
#define ZLL_TMR_PRESCALE_TMR_PRESCALE_SHIFT      (0U)
#define ZLL_TMR_PRESCALE_TMR_PRESCALE(x)         (((uint32_t)(((uint32_t)(x)) << ZLL_TMR_PRESCALE_TMR_PRESCALE_SHIFT)) & ZLL_TMR_PRESCALE_TMR_PRESCALE_MASK)

/*! @name LENIENCY_LSB - LENIENCY LSB */
#define ZLL_LENIENCY_LSB_LENIENCY_LSB_MASK       (0xFFFFFFFFU)
#define ZLL_LENIENCY_LSB_LENIENCY_LSB_SHIFT      (0U)
#define ZLL_LENIENCY_LSB_LENIENCY_LSB(x)         (((uint32_t)(((uint32_t)(x)) << ZLL_LENIENCY_LSB_LENIENCY_LSB_SHIFT)) & ZLL_LENIENCY_LSB_LENIENCY_LSB_MASK)

/*! @name LENIENCY_MSB - LENIENCY MSB */
#define ZLL_LENIENCY_MSB_LENIENCY_MSB_MASK       (0xFFU)
#define ZLL_LENIENCY_MSB_LENIENCY_MSB_SHIFT      (0U)
#define ZLL_LENIENCY_MSB_LENIENCY_MSB(x)         (((uint32_t)(((uint32_t)(x)) << ZLL_LENIENCY_MSB_LENIENCY_MSB_SHIFT)) & ZLL_LENIENCY_MSB_LENIENCY_MSB_MASK)

/*! @name PART_ID - PART ID */
#define ZLL_PART_ID_PART_ID_MASK                 (0xFFU)
#define ZLL_PART_ID_PART_ID_SHIFT                (0U)
#define ZLL_PART_ID_PART_ID(x)                   (((uint32_t)(((uint32_t)(x)) << ZLL_PART_ID_PART_ID_SHIFT)) & ZLL_PART_ID_PART_ID_MASK)

/*! @name PKT_BUFFER_TX - Packet Buffer TX */
#define ZLL_PKT_BUFFER_TX_PKT_BUFFER_TX_MASK     (0xFFFFU)
#define ZLL_PKT_BUFFER_TX_PKT_BUFFER_TX_SHIFT    (0U)
#define ZLL_PKT_BUFFER_TX_PKT_BUFFER_TX(x)       (((uint16_t)(((uint16_t)(x)) << ZLL_PKT_BUFFER_TX_PKT_BUFFER_TX_SHIFT)) & ZLL_PKT_BUFFER_TX_PKT_BUFFER_TX_MASK)

/* The count of ZLL_PKT_BUFFER_TX */
#define ZLL_PKT_BUFFER_TX_COUNT                  (64U)

/*! @name PKT_BUFFER_RX - Packet Buffer RX */
#define ZLL_PKT_BUFFER_RX_PKT_BUFFER_RX_MASK     (0xFFFFU)
#define ZLL_PKT_BUFFER_RX_PKT_BUFFER_RX_SHIFT    (0U)
#define ZLL_PKT_BUFFER_RX_PKT_BUFFER_RX(x)       (((uint16_t)(((uint16_t)(x)) << ZLL_PKT_BUFFER_RX_PKT_BUFFER_RX_SHIFT)) & ZLL_PKT_BUFFER_RX_PKT_BUFFER_RX_MASK)

/* The count of ZLL_PKT_BUFFER_RX */
#define ZLL_PKT_BUFFER_RX_COUNT                  (64U)


/*!
 * @}
 */ /* end of group ZLL_Register_Masks */


/* ZLL - Peripheral instance base addresses */
/** Peripheral ZLL base address */
#define ZLL_BASE                                 (0x4005D000u)
/** Peripheral ZLL base pointer */
#define ZLL                                      ((ZLL_Type *)ZLL_BASE)
/** Array initializer of ZLL peripheral base addresses */
#define ZLL_BASE_ADDRS                           { ZLL_BASE }
/** Array initializer of ZLL peripheral base pointers */
#define ZLL_BASE_PTRS                            { ZLL }

/*!
 * @}
 */ /* end of group ZLL_Peripheral_Access_Layer */


/*
** End of section using anonymous unions
*/

#if defined(__ARMCC_VERSION)
  #pragma pop
#elif defined(__GNUC__)
  /* leave anonymous unions enabled */
#elif defined(__IAR_SYSTEMS_ICC__)
  #pragma language=default
#else
  #error Not supported compiler type
#endif

/*!
 * @}
 */ /* end of group Peripheral_access_layer */


/* ----------------------------------------------------------------------------
   -- SDK Compatibility
   ---------------------------------------------------------------------------- */

/*!
 * @addtogroup SDK_Compatibility_Symbols SDK Compatibility
 * @{
 */

#define DSPI0                     SPI0
#define DSPI1                     SPI1

/*!
 * @}
 */ /* end of group SDK_Compatibility_Symbols */


#endif  /* _MKW41Z4_H_ */

