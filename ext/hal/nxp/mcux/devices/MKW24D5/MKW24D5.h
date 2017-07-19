/*
** ###################################################################
**     Processor:           MKW24D512VHA5
**     Compilers:           Keil ARM C/C++ Compiler
**                          Freescale C/C++ for Embedded ARM
**                          GNU C Compiler
**                          IAR ANSI C/C++ Compiler for ARM
**                          MCUXpresso Compiler
**
**     Reference manual:    MKW2xDRM Rev.2  July 2014
**     Version:             rev. 2.0, 2014-11-26
**     Build:               b170112
**
**     Abstract:
**         CMSIS Peripheral Access Layer for MKW24D5
**
**     Copyright (c) 1997 - 2016 Freescale Semiconductor, Inc.
**     Copyright 2016 - 2017 NXP
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
**     o Neither the name of the copyright holder nor the names of its
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
**     http:                 www.nxp.com
**     mail:                 support@nxp.com
**
**     Revisions:
**     - rev. 1.0 (2013-11-22)
**         Initial version.
**     - rev. 2.0 (2014-11-26)
**         update of SystemInit() imlementation
**         Module access macro module_BASES replaced by module_BASE_PTRS.
**         Register accessor macros added to the memory map.
**         MCG - bit LOLS in MCG_S register renamed to LOLS0.
**         DAC0 registers removed.
**
** ###################################################################
*/

/*!
 * @file MKW24D5.h
 * @version 2.0
 * @date 2014-11-26
 * @brief CMSIS Peripheral Access Layer for MKW24D5
 *
 * CMSIS Peripheral Access Layer for MKW24D5
 */

#ifndef _MKW24D5_H_
#define _MKW24D5_H_                              /**< Symbol preventing repeated inclusion */

/** Memory map major version (memory maps with equal major version number are
 * compatible) */
#define MCU_MEM_MAP_VERSION 0x0200U
/** Memory map minor version */
#define MCU_MEM_MAP_VERSION_MINOR 0x0000U

/**
 * @brief Macro to calculate address of an aliased word in the peripheral
 *        bitband area for a peripheral register and bit (bit band region 0x40000000 to
 *        0x400FFFFF).
 * @param Reg Register to access.
 * @param Bit Bit number to access.
 * @return  Address of the aliased word in the peripheral bitband area.
 */
#define BITBAND_REGADDR(Reg,Bit) (0x42000000u + (32u*((uint32_t)&(Reg) - (uint32_t)0x40000000u)) + (4u*((uint32_t)(Bit))))
/**
 * @brief Macro to access a single bit of a peripheral register (bit band region
 *        0x40000000 to 0x400FFFFF) using the bit-band alias region access. Can
 *        be used for peripherals with 32bit access allowed.
 * @param Reg Register to access.
 * @param Bit Bit number to access.
 * @return Value of the targeted bit in the bit band region.
 */
#define BITBAND_REG32(Reg,Bit) (*((uint32_t volatile*)(BITBAND_REGADDR((Reg),(Bit)))))
#define BITBAND_REG(Reg,Bit) (BITBAND_REG32((Reg),(Bit)))
/**
 * @brief Macro to access a single bit of a peripheral register (bit band region
 *        0x40000000 to 0x400FFFFF) using the bit-band alias region access. Can
 *        be used for peripherals with 16bit access allowed.
 * @param Reg Register to access.
 * @param Bit Bit number to access.
 * @return Value of the targeted bit in the bit band region.
 */
#define BITBAND_REG16(Reg,Bit) (*((uint16_t volatile*)(BITBAND_REGADDR((Reg),(Bit)))))
/**
 * @brief Macro to access a single bit of a peripheral register (bit band region
 *        0x40000000 to 0x400FFFFF) using the bit-band alias region access. Can
 *        be used for peripherals with 8bit access allowed.
 * @param Reg Register to access.
 * @param Bit Bit number to access.
 * @return Value of the targeted bit in the bit band region.
 */
#define BITBAND_REG8(Reg,Bit) (*((uint8_t volatile*)(BITBAND_REGADDR((Reg),(Bit)))))

/* ----------------------------------------------------------------------------
   -- Interrupt vector numbers
   ---------------------------------------------------------------------------- */

/*!
 * @addtogroup Interrupt_vector_numbers Interrupt vector numbers
 * @{
 */

/** Interrupt Number Definitions */
#define NUMBER_OF_INT_VECTORS 81                 /**< Number of interrupts in the Vector table */

typedef enum IRQn {
  /* Auxiliary constants */
  NotAvail_IRQn                = -128,             /**< Not available device specific interrupt */

  /* Core interrupts */
  NonMaskableInt_IRQn          = -14,              /**< Non Maskable Interrupt */
  HardFault_IRQn               = -13,              /**< Cortex-M4 SV Hard Fault Interrupt */
  MemoryManagement_IRQn        = -12,              /**< Cortex-M4 Memory Management Interrupt */
  BusFault_IRQn                = -11,              /**< Cortex-M4 Bus Fault Interrupt */
  UsageFault_IRQn              = -10,              /**< Cortex-M4 Usage Fault Interrupt */
  SVCall_IRQn                  = -5,               /**< Cortex-M4 SV Call Interrupt */
  DebugMonitor_IRQn            = -4,               /**< Cortex-M4 Debug Monitor Interrupt */
  PendSV_IRQn                  = -2,               /**< Cortex-M4 Pend SV Interrupt */
  SysTick_IRQn                 = -1,               /**< Cortex-M4 System Tick Interrupt */

  /* Device specific interrupts */
  DMA0_IRQn                    = 0,                /**< DMA channel 0 transfer complete */
  DMA1_IRQn                    = 1,                /**< DMA channel 1 transfer complete */
  DMA2_IRQn                    = 2,                /**< DMA channel 2 transfer complete */
  DMA3_IRQn                    = 3,                /**< DMA channel 3 transfer complete */
  DMA4_IRQn                    = 4,                /**< DMA channel 4 transfer complete */
  DMA5_IRQn                    = 5,                /**< DMA channel 5 transfer complete */
  DMA6_IRQn                    = 6,                /**< DMA channel 6 transfer complete */
  DMA7_IRQn                    = 7,                /**< DMA channel 7 transfer complete */
  DMA8_IRQn                    = 8,                /**< DMA channel 8 transfer complete */
  DMA9_IRQn                    = 9,                /**< DMA channel 9 transfer complete */
  DMA10_IRQn                   = 10,               /**< DMA channel 10 transfer complete */
  DMA11_IRQn                   = 11,               /**< DMA channel 11 transfer complete */
  DMA12_IRQn                   = 12,               /**< DMA channel 12 transfer complete */
  DMA13_IRQn                   = 13,               /**< DMA channel 13 transfer complete */
  DMA14_IRQn                   = 14,               /**< DMA channel 14 transfer complete */
  DMA15_IRQn                   = 15,               /**< DMA channel 15 transfer complete */
  DMA_Error_IRQn               = 16,               /**< DMA channel 0 - 15 error */
  MCM_IRQn                     = 17,               /**< MCM normal interrupt */
  FTFL_IRQn                    = 18,               /**< FTFL command complete */
  FTFL_Collision_IRQn          = 19,               /**< FTFL read collision */
  PMC_IRQn                     = 20,               /**< PMC controller low-voltage detect, low-voltage warning */
  LLWU_IRQn                    = 21,               /**< Low leakage wakeup */
  WDOG_EWM_IRQn                = 22,               /**< Single interrupt vector for  WDOG and EWM */
  RNG_IRQn                     = 23,               /**< Randon number generator */
  I2C0_IRQn                    = 24,               /**< Inter-integrated circuit 0 */
  I2C1_IRQn                    = 25,               /**< Inter-integrated circuit 1 */
  SPI0_IRQn                    = 26,               /**< Serial peripheral Interface 0 */
  SPI1_IRQn                    = 27,               /**< Serial peripheral Interface 1 */
  I2S0_Tx_IRQn                 = 28,               /**< Integrated interchip sound 0 transmit interrupt */
  I2S0_Rx_IRQn                 = 29,               /**< Integrated interchip sound 0 receive interrupt */
  Reserved46_IRQn              = 30,               /**< Reserved interrupt */
  UART0_RX_TX_IRQn             = 31,               /**< UART0 receive/transmit interrupt */
  UART0_ERR_IRQn               = 32,               /**< UART0 error interrupt */
  UART1_RX_TX_IRQn             = 33,               /**< UART1 receive/transmit interrupt */
  UART1_ERR_IRQn               = 34,               /**< UART1 error interrupt */
  UART2_RX_TX_IRQn             = 35,               /**< UART2 receive/transmit interrupt */
  UART2_ERR_IRQn               = 36,               /**< UART2 error interrupt */
  Reserved53_IRQn              = 37,               /**< Reserved interrupt */
  Reserved54_IRQn              = 38,               /**< Reserved interrupt */
  ADC0_IRQn                    = 39,               /**< Analog-to-digital converter 0 */
  CMP0_IRQn                    = 40,               /**< Comparator 0 */
  CMP1_IRQn                    = 41,               /**< Comparator 1 */
  FTM0_IRQn                    = 42,               /**< FlexTimer module 0 fault, overflow and channels interrupt */
  FTM1_IRQn                    = 43,               /**< FlexTimer module 1 fault, overflow and channels interrupt */
  FTM2_IRQn                    = 44,               /**< FlexTimer module 2 fault, overflow and channels interrupt */
  CMT_IRQn                     = 45,               /**< Carrier modulator transmitter */
  RTC_IRQn                     = 46,               /**< Real time clock */
  RTC_Seconds_IRQn             = 47,               /**< Real time clock seconds */
  PIT0_IRQn                    = 48,               /**< Periodic interrupt timer channel 0 */
  PIT1_IRQn                    = 49,               /**< Periodic interrupt timer channel 1 */
  PIT2_IRQn                    = 50,               /**< Periodic interrupt timer channel 2 */
  PIT3_IRQn                    = 51,               /**< Periodic interrupt timer channel 3 */
  PDB0_IRQn                    = 52,               /**< Programmable delay block */
  USB0_IRQn                    = 53,               /**< USB OTG interrupt */
  USBDCD_IRQn                  = 54,               /**< USB charger detect */
  Reserved71_IRQn              = 55,               /**< Reserved interrupt */
  Reserved72_IRQn              = 56,               /**< Reserved interrupt */
  MCG_IRQn                     = 57,               /**< Multipurpose clock generator */
  LPTMR0_IRQn                  = 58,               /**< Low power timer interrupt */
  PORTA_IRQn                   = 59,               /**< Port A pin detect interrupt */
  PORTB_IRQn                   = 60,               /**< Port B pin detect interrupt */
  PORTC_IRQn                   = 61,               /**< Port C pin detect interrupt */
  PORTD_IRQn                   = 62,               /**< Port D pin detect interrupt */
  PORTE_IRQn                   = 63,               /**< Port E pin detect interrupt */
  SWI_IRQn                     = 64                /**< Software interrupt */
} IRQn_Type;

/*!
 * @}
 */ /* end of group Interrupt_vector_numbers */


/* ----------------------------------------------------------------------------
   -- Cortex M4 Core Configuration
   ---------------------------------------------------------------------------- */

/*!
 * @addtogroup Cortex_Core_Configuration Cortex M4 Core Configuration
 * @{
 */

#define __MPU_PRESENT                  0         /**< Defines if an MPU is present or not */
#define __NVIC_PRIO_BITS               4         /**< Number of priority bits implemented in the NVIC */
#define __Vendor_SysTickConfig         0         /**< Vendor specific implementation of SysTickConfig is defined */
#define __FPU_PRESENT                  0         /**< Defines if an FPU is present or not */

#include "core_cm4.h"                  /* Core Peripheral Access Layer */
#include "system_MKW24D5.h"            /* Device specific configuration file */

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
    kDmaRequestMux0UART0Rx          = 2|0x100U,    /**< UART0 Receive. */
    kDmaRequestMux0UART0Tx          = 3|0x100U,    /**< UART0 Transmit. */
    kDmaRequestMux0UART1Rx          = 4|0x100U,    /**< UART1 Receive. */
    kDmaRequestMux0UART1Tx          = 5|0x100U,    /**< UART1 Transmit. */
    kDmaRequestMux0UART2Rx          = 6|0x100U,    /**< UART2 Receive. */
    kDmaRequestMux0UART2Tx          = 7|0x100U,    /**< UART2 Transmit. */
    kDmaRequestMux0Reserved8        = 8|0x100U,    /**< Reserved8 */
    kDmaRequestMux0Reserved9        = 9|0x100U,    /**< Reserved9 */
    kDmaRequestMux0Reserved10       = 10|0x100U,   /**< Reserved10 */
    kDmaRequestMux0Reserved11       = 11|0x100U,   /**< Reserved11 */
    kDmaRequestMux0Reserved12       = 12|0x100U,   /**< Reserved12 */
    kDmaRequestMux0Reserved13       = 13|0x100U,   /**< Reserved13 */
    kDmaRequestMux0I2S0Rx           = 14|0x100U,   /**< I2S0 Receive. */
    kDmaRequestMux0I2S0Tx           = 15|0x100U,   /**< I2S0 Transmit. */
    kDmaRequestMux0SPI0Rx           = 16|0x100U,   /**< SPI0 Receive. */
    kDmaRequestMux0SPI0Tx           = 17|0x100U,   /**< SPI0 Transmit. */
    kDmaRequestMux0SPI1Rx           = 18|0x100U,   /**< SPI1 Receive. */
    kDmaRequestMux0SPI1Tx           = 19|0x100U,   /**< SPI1 Transmit. */
    kDmaRequestMux0Reserved20       = 20|0x100U,   /**< Reserved20 */
    kDmaRequestMux0Reserved21       = 21|0x100U,   /**< Reserved21 */
    kDmaRequestMux0I2C0             = 22|0x100U,   /**< I2C0. */
    kDmaRequestMux0I2C1             = 23|0x100U,   /**< I2C1. */
    kDmaRequestMux0FTM0Channel0     = 24|0x100U,   /**< FTM0 C0V. */
    kDmaRequestMux0FTM0Channel1     = 25|0x100U,   /**< FTM0 C1V. */
    kDmaRequestMux0FTM0Channel2     = 26|0x100U,   /**< FTM0 C2V. */
    kDmaRequestMux0FTM0Channel3     = 27|0x100U,   /**< FTM0 C3V. */
    kDmaRequestMux0FTM0Channel4     = 28|0x100U,   /**< FTM0 C4V. */
    kDmaRequestMux0FTM0Channel5     = 29|0x100U,   /**< FTM0 C5V. */
    kDmaRequestMux0FTM0Channel6     = 30|0x100U,   /**< FTM0 C6V. */
    kDmaRequestMux0FTM0Channel7     = 31|0x100U,   /**< FTM0 C7V. */
    kDmaRequestMux0FTM1Channel0     = 32|0x100U,   /**< FTM1 C0V. */
    kDmaRequestMux0FTM1Channel1     = 33|0x100U,   /**< FTM1 C1V. */
    kDmaRequestMux0FTM2Channel0     = 34|0x100U,   /**< FTM2 C0V. */
    kDmaRequestMux0FTM2Channel1     = 35|0x100U,   /**< FTM2 C1V. */
    kDmaRequestMux0Reserved36       = 36|0x100U,   /**< Reserved36 */
    kDmaRequestMux0Reserved37       = 37|0x100U,   /**< Reserved37 */
    kDmaRequestMux0Reserved38       = 38|0x100U,   /**< Reserved38 */
    kDmaRequestMux0Reserved39       = 39|0x100U,   /**< Reserved39 */
    kDmaRequestMux0ADC0             = 40|0x100U,   /**< ADC0. */
    kDmaRequestMux0Reserved41       = 41|0x100U,   /**< Reserved41 */
    kDmaRequestMux0CMP0             = 42|0x100U,   /**< CMP0. */
    kDmaRequestMux0CMP1             = 43|0x100U,   /**< CMP1. */
    kDmaRequestMux0Reserved44       = 44|0x100U,   /**< Reserved44 */
    kDmaRequestMux0Reserved45       = 45|0x100U,   /**< Reserved45 */
    kDmaRequestMux0Reserved46       = 46|0x100U,   /**< Reserved46 */
    kDmaRequestMux0CMT              = 47|0x100U,   /**< CMT. */
    kDmaRequestMux0PDB              = 48|0x100U,   /**< PDB0. */
    kDmaRequestMux0PortA            = 49|0x100U,   /**< PTA. */
    kDmaRequestMux0PortB            = 50|0x100U,   /**< PTB. */
    kDmaRequestMux0PortC            = 51|0x100U,   /**< PTC. */
    kDmaRequestMux0PortD            = 52|0x100U,   /**< PTD. */
    kDmaRequestMux0PortE            = 53|0x100U,   /**< PTE. */
    kDmaRequestMux0AlwaysOn54       = 54|0x100U,   /**< DMAMUX Always Enabled slot. */
    kDmaRequestMux0AlwaysOn55       = 55|0x100U,   /**< DMAMUX Always Enabled slot. */
    kDmaRequestMux0AlwaysOn56       = 56|0x100U,   /**< DMAMUX Always Enabled slot. */
    kDmaRequestMux0AlwaysOn57       = 57|0x100U,   /**< DMAMUX Always Enabled slot. */
    kDmaRequestMux0AlwaysOn58       = 58|0x100U,   /**< DMAMUX Always Enabled slot. */
    kDmaRequestMux0AlwaysOn59       = 59|0x100U,   /**< DMAMUX Always Enabled slot. */
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
#elif defined(__CWCC__)
  #pragma push
  #pragma cpp_extensions on
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
   -- CAU Peripheral Access Layer
   ---------------------------------------------------------------------------- */

/*!
 * @addtogroup CAU_Peripheral_Access_Layer CAU Peripheral Access Layer
 * @{
 */

/** CAU - Register Layout Typedef */
typedef struct {
  __O  uint32_t DIRECT[16];                        /**< Direct access register 0..Direct access register 15, array offset: 0x0, array step: 0x4 */
       uint8_t RESERVED_0[2048];
  __O  uint32_t LDR_CASR;                          /**< Status register - Load Register command, offset: 0x840 */
  __O  uint32_t LDR_CAA;                           /**< Accumulator register - Load Register command, offset: 0x844 */
  __O  uint32_t LDR_CA[9];                         /**< General Purpose Register 0 - Load Register command..General Purpose Register 8 - Load Register command, array offset: 0x848, array step: 0x4 */
       uint8_t RESERVED_1[20];
  __I  uint32_t STR_CASR;                          /**< Status register - Store Register command, offset: 0x880 */
  __I  uint32_t STR_CAA;                           /**< Accumulator register - Store Register command, offset: 0x884 */
  __I  uint32_t STR_CA[9];                         /**< General Purpose Register 0 - Store Register command..General Purpose Register 8 - Store Register command, array offset: 0x888, array step: 0x4 */
       uint8_t RESERVED_2[20];
  __O  uint32_t ADR_CASR;                          /**< Status register - Add Register command, offset: 0x8C0 */
  __O  uint32_t ADR_CAA;                           /**< Accumulator register - Add to register command, offset: 0x8C4 */
  __O  uint32_t ADR_CA[9];                         /**< General Purpose Register 0 - Add to register command..General Purpose Register 8 - Add to register command, array offset: 0x8C8, array step: 0x4 */
       uint8_t RESERVED_3[20];
  __O  uint32_t RADR_CASR;                         /**< Status register - Reverse and Add to Register command, offset: 0x900 */
  __O  uint32_t RADR_CAA;                          /**< Accumulator register - Reverse and Add to Register command, offset: 0x904 */
  __O  uint32_t RADR_CA[9];                        /**< General Purpose Register 0 - Reverse and Add to Register command..General Purpose Register 8 - Reverse and Add to Register command, array offset: 0x908, array step: 0x4 */
       uint8_t RESERVED_4[84];
  __O  uint32_t XOR_CASR;                          /**< Status register - Exclusive Or command, offset: 0x980 */
  __O  uint32_t XOR_CAA;                           /**< Accumulator register - Exclusive Or command, offset: 0x984 */
  __O  uint32_t XOR_CA[9];                         /**< General Purpose Register 0 - Exclusive Or command..General Purpose Register 8 - Exclusive Or command, array offset: 0x988, array step: 0x4 */
       uint8_t RESERVED_5[20];
  __O  uint32_t ROTL_CASR;                         /**< Status register - Rotate Left command, offset: 0x9C0 */
  __O  uint32_t ROTL_CAA;                          /**< Accumulator register - Rotate Left command, offset: 0x9C4 */
  __O  uint32_t ROTL_CA[9];                        /**< General Purpose Register 0 - Rotate Left command..General Purpose Register 8 - Rotate Left command, array offset: 0x9C8, array step: 0x4 */
       uint8_t RESERVED_6[276];
  __O  uint32_t AESC_CASR;                         /**< Status register - AES Column Operation command, offset: 0xB00 */
  __O  uint32_t AESC_CAA;                          /**< Accumulator register - AES Column Operation command, offset: 0xB04 */
  __O  uint32_t AESC_CA[9];                        /**< General Purpose Register 0 - AES Column Operation command..General Purpose Register 8 - AES Column Operation command, array offset: 0xB08, array step: 0x4 */
       uint8_t RESERVED_7[20];
  __O  uint32_t AESIC_CASR;                        /**< Status register - AES Inverse Column Operation command, offset: 0xB40 */
  __O  uint32_t AESIC_CAA;                         /**< Accumulator register - AES Inverse Column Operation command, offset: 0xB44 */
  __O  uint32_t AESIC_CA[9];                       /**< General Purpose Register 0 - AES Inverse Column Operation command..General Purpose Register 8 - AES Inverse Column Operation command, array offset: 0xB48, array step: 0x4 */
} CAU_Type;

/* ----------------------------------------------------------------------------
   -- CAU Register Masks
   ---------------------------------------------------------------------------- */

/*!
 * @addtogroup CAU_Register_Masks CAU Register Masks
 * @{
 */

/*! @name DIRECT - Direct access register 0..Direct access register 15 */
#define CAU_DIRECT_CAU_DIRECT0_MASK              (0xFFFFFFFFU)
#define CAU_DIRECT_CAU_DIRECT0_SHIFT             (0U)
#define CAU_DIRECT_CAU_DIRECT0(x)                (((uint32_t)(((uint32_t)(x)) << CAU_DIRECT_CAU_DIRECT0_SHIFT)) & CAU_DIRECT_CAU_DIRECT0_MASK)
#define CAU_DIRECT_CAU_DIRECT1_MASK              (0xFFFFFFFFU)
#define CAU_DIRECT_CAU_DIRECT1_SHIFT             (0U)
#define CAU_DIRECT_CAU_DIRECT1(x)                (((uint32_t)(((uint32_t)(x)) << CAU_DIRECT_CAU_DIRECT1_SHIFT)) & CAU_DIRECT_CAU_DIRECT1_MASK)
#define CAU_DIRECT_CAU_DIRECT2_MASK              (0xFFFFFFFFU)
#define CAU_DIRECT_CAU_DIRECT2_SHIFT             (0U)
#define CAU_DIRECT_CAU_DIRECT2(x)                (((uint32_t)(((uint32_t)(x)) << CAU_DIRECT_CAU_DIRECT2_SHIFT)) & CAU_DIRECT_CAU_DIRECT2_MASK)
#define CAU_DIRECT_CAU_DIRECT3_MASK              (0xFFFFFFFFU)
#define CAU_DIRECT_CAU_DIRECT3_SHIFT             (0U)
#define CAU_DIRECT_CAU_DIRECT3(x)                (((uint32_t)(((uint32_t)(x)) << CAU_DIRECT_CAU_DIRECT3_SHIFT)) & CAU_DIRECT_CAU_DIRECT3_MASK)
#define CAU_DIRECT_CAU_DIRECT4_MASK              (0xFFFFFFFFU)
#define CAU_DIRECT_CAU_DIRECT4_SHIFT             (0U)
#define CAU_DIRECT_CAU_DIRECT4(x)                (((uint32_t)(((uint32_t)(x)) << CAU_DIRECT_CAU_DIRECT4_SHIFT)) & CAU_DIRECT_CAU_DIRECT4_MASK)
#define CAU_DIRECT_CAU_DIRECT5_MASK              (0xFFFFFFFFU)
#define CAU_DIRECT_CAU_DIRECT5_SHIFT             (0U)
#define CAU_DIRECT_CAU_DIRECT5(x)                (((uint32_t)(((uint32_t)(x)) << CAU_DIRECT_CAU_DIRECT5_SHIFT)) & CAU_DIRECT_CAU_DIRECT5_MASK)
#define CAU_DIRECT_CAU_DIRECT6_MASK              (0xFFFFFFFFU)
#define CAU_DIRECT_CAU_DIRECT6_SHIFT             (0U)
#define CAU_DIRECT_CAU_DIRECT6(x)                (((uint32_t)(((uint32_t)(x)) << CAU_DIRECT_CAU_DIRECT6_SHIFT)) & CAU_DIRECT_CAU_DIRECT6_MASK)
#define CAU_DIRECT_CAU_DIRECT7_MASK              (0xFFFFFFFFU)
#define CAU_DIRECT_CAU_DIRECT7_SHIFT             (0U)
#define CAU_DIRECT_CAU_DIRECT7(x)                (((uint32_t)(((uint32_t)(x)) << CAU_DIRECT_CAU_DIRECT7_SHIFT)) & CAU_DIRECT_CAU_DIRECT7_MASK)
#define CAU_DIRECT_CAU_DIRECT8_MASK              (0xFFFFFFFFU)
#define CAU_DIRECT_CAU_DIRECT8_SHIFT             (0U)
#define CAU_DIRECT_CAU_DIRECT8(x)                (((uint32_t)(((uint32_t)(x)) << CAU_DIRECT_CAU_DIRECT8_SHIFT)) & CAU_DIRECT_CAU_DIRECT8_MASK)
#define CAU_DIRECT_CAU_DIRECT9_MASK              (0xFFFFFFFFU)
#define CAU_DIRECT_CAU_DIRECT9_SHIFT             (0U)
#define CAU_DIRECT_CAU_DIRECT9(x)                (((uint32_t)(((uint32_t)(x)) << CAU_DIRECT_CAU_DIRECT9_SHIFT)) & CAU_DIRECT_CAU_DIRECT9_MASK)
#define CAU_DIRECT_CAU_DIRECT10_MASK             (0xFFFFFFFFU)
#define CAU_DIRECT_CAU_DIRECT10_SHIFT            (0U)
#define CAU_DIRECT_CAU_DIRECT10(x)               (((uint32_t)(((uint32_t)(x)) << CAU_DIRECT_CAU_DIRECT10_SHIFT)) & CAU_DIRECT_CAU_DIRECT10_MASK)
#define CAU_DIRECT_CAU_DIRECT11_MASK             (0xFFFFFFFFU)
#define CAU_DIRECT_CAU_DIRECT11_SHIFT            (0U)
#define CAU_DIRECT_CAU_DIRECT11(x)               (((uint32_t)(((uint32_t)(x)) << CAU_DIRECT_CAU_DIRECT11_SHIFT)) & CAU_DIRECT_CAU_DIRECT11_MASK)
#define CAU_DIRECT_CAU_DIRECT12_MASK             (0xFFFFFFFFU)
#define CAU_DIRECT_CAU_DIRECT12_SHIFT            (0U)
#define CAU_DIRECT_CAU_DIRECT12(x)               (((uint32_t)(((uint32_t)(x)) << CAU_DIRECT_CAU_DIRECT12_SHIFT)) & CAU_DIRECT_CAU_DIRECT12_MASK)
#define CAU_DIRECT_CAU_DIRECT13_MASK             (0xFFFFFFFFU)
#define CAU_DIRECT_CAU_DIRECT13_SHIFT            (0U)
#define CAU_DIRECT_CAU_DIRECT13(x)               (((uint32_t)(((uint32_t)(x)) << CAU_DIRECT_CAU_DIRECT13_SHIFT)) & CAU_DIRECT_CAU_DIRECT13_MASK)
#define CAU_DIRECT_CAU_DIRECT14_MASK             (0xFFFFFFFFU)
#define CAU_DIRECT_CAU_DIRECT14_SHIFT            (0U)
#define CAU_DIRECT_CAU_DIRECT14(x)               (((uint32_t)(((uint32_t)(x)) << CAU_DIRECT_CAU_DIRECT14_SHIFT)) & CAU_DIRECT_CAU_DIRECT14_MASK)
#define CAU_DIRECT_CAU_DIRECT15_MASK             (0xFFFFFFFFU)
#define CAU_DIRECT_CAU_DIRECT15_SHIFT            (0U)
#define CAU_DIRECT_CAU_DIRECT15(x)               (((uint32_t)(((uint32_t)(x)) << CAU_DIRECT_CAU_DIRECT15_SHIFT)) & CAU_DIRECT_CAU_DIRECT15_MASK)

/* The count of CAU_DIRECT */
#define CAU_DIRECT_COUNT                         (16U)

/*! @name LDR_CASR - Status register - Load Register command */
#define CAU_LDR_CASR_IC_MASK                     (0x1U)
#define CAU_LDR_CASR_IC_SHIFT                    (0U)
#define CAU_LDR_CASR_IC(x)                       (((uint32_t)(((uint32_t)(x)) << CAU_LDR_CASR_IC_SHIFT)) & CAU_LDR_CASR_IC_MASK)
#define CAU_LDR_CASR_DPE_MASK                    (0x2U)
#define CAU_LDR_CASR_DPE_SHIFT                   (1U)
#define CAU_LDR_CASR_DPE(x)                      (((uint32_t)(((uint32_t)(x)) << CAU_LDR_CASR_DPE_SHIFT)) & CAU_LDR_CASR_DPE_MASK)
#define CAU_LDR_CASR_VER_MASK                    (0xF0000000U)
#define CAU_LDR_CASR_VER_SHIFT                   (28U)
#define CAU_LDR_CASR_VER(x)                      (((uint32_t)(((uint32_t)(x)) << CAU_LDR_CASR_VER_SHIFT)) & CAU_LDR_CASR_VER_MASK)

/*! @name LDR_CAA - Accumulator register - Load Register command */
#define CAU_LDR_CAA_ACC_MASK                     (0xFFFFFFFFU)
#define CAU_LDR_CAA_ACC_SHIFT                    (0U)
#define CAU_LDR_CAA_ACC(x)                       (((uint32_t)(((uint32_t)(x)) << CAU_LDR_CAA_ACC_SHIFT)) & CAU_LDR_CAA_ACC_MASK)

/*! @name LDR_CA - General Purpose Register 0 - Load Register command..General Purpose Register 8 - Load Register command */
#define CAU_LDR_CA_CA0_MASK                      (0xFFFFFFFFU)
#define CAU_LDR_CA_CA0_SHIFT                     (0U)
#define CAU_LDR_CA_CA0(x)                        (((uint32_t)(((uint32_t)(x)) << CAU_LDR_CA_CA0_SHIFT)) & CAU_LDR_CA_CA0_MASK)
#define CAU_LDR_CA_CA1_MASK                      (0xFFFFFFFFU)
#define CAU_LDR_CA_CA1_SHIFT                     (0U)
#define CAU_LDR_CA_CA1(x)                        (((uint32_t)(((uint32_t)(x)) << CAU_LDR_CA_CA1_SHIFT)) & CAU_LDR_CA_CA1_MASK)
#define CAU_LDR_CA_CA2_MASK                      (0xFFFFFFFFU)
#define CAU_LDR_CA_CA2_SHIFT                     (0U)
#define CAU_LDR_CA_CA2(x)                        (((uint32_t)(((uint32_t)(x)) << CAU_LDR_CA_CA2_SHIFT)) & CAU_LDR_CA_CA2_MASK)
#define CAU_LDR_CA_CA3_MASK                      (0xFFFFFFFFU)
#define CAU_LDR_CA_CA3_SHIFT                     (0U)
#define CAU_LDR_CA_CA3(x)                        (((uint32_t)(((uint32_t)(x)) << CAU_LDR_CA_CA3_SHIFT)) & CAU_LDR_CA_CA3_MASK)
#define CAU_LDR_CA_CA4_MASK                      (0xFFFFFFFFU)
#define CAU_LDR_CA_CA4_SHIFT                     (0U)
#define CAU_LDR_CA_CA4(x)                        (((uint32_t)(((uint32_t)(x)) << CAU_LDR_CA_CA4_SHIFT)) & CAU_LDR_CA_CA4_MASK)
#define CAU_LDR_CA_CA5_MASK                      (0xFFFFFFFFU)
#define CAU_LDR_CA_CA5_SHIFT                     (0U)
#define CAU_LDR_CA_CA5(x)                        (((uint32_t)(((uint32_t)(x)) << CAU_LDR_CA_CA5_SHIFT)) & CAU_LDR_CA_CA5_MASK)
#define CAU_LDR_CA_CA6_MASK                      (0xFFFFFFFFU)
#define CAU_LDR_CA_CA6_SHIFT                     (0U)
#define CAU_LDR_CA_CA6(x)                        (((uint32_t)(((uint32_t)(x)) << CAU_LDR_CA_CA6_SHIFT)) & CAU_LDR_CA_CA6_MASK)
#define CAU_LDR_CA_CA7_MASK                      (0xFFFFFFFFU)
#define CAU_LDR_CA_CA7_SHIFT                     (0U)
#define CAU_LDR_CA_CA7(x)                        (((uint32_t)(((uint32_t)(x)) << CAU_LDR_CA_CA7_SHIFT)) & CAU_LDR_CA_CA7_MASK)
#define CAU_LDR_CA_CA8_MASK                      (0xFFFFFFFFU)
#define CAU_LDR_CA_CA8_SHIFT                     (0U)
#define CAU_LDR_CA_CA8(x)                        (((uint32_t)(((uint32_t)(x)) << CAU_LDR_CA_CA8_SHIFT)) & CAU_LDR_CA_CA8_MASK)

/* The count of CAU_LDR_CA */
#define CAU_LDR_CA_COUNT                         (9U)

/*! @name STR_CASR - Status register - Store Register command */
#define CAU_STR_CASR_IC_MASK                     (0x1U)
#define CAU_STR_CASR_IC_SHIFT                    (0U)
#define CAU_STR_CASR_IC(x)                       (((uint32_t)(((uint32_t)(x)) << CAU_STR_CASR_IC_SHIFT)) & CAU_STR_CASR_IC_MASK)
#define CAU_STR_CASR_DPE_MASK                    (0x2U)
#define CAU_STR_CASR_DPE_SHIFT                   (1U)
#define CAU_STR_CASR_DPE(x)                      (((uint32_t)(((uint32_t)(x)) << CAU_STR_CASR_DPE_SHIFT)) & CAU_STR_CASR_DPE_MASK)
#define CAU_STR_CASR_VER_MASK                    (0xF0000000U)
#define CAU_STR_CASR_VER_SHIFT                   (28U)
#define CAU_STR_CASR_VER(x)                      (((uint32_t)(((uint32_t)(x)) << CAU_STR_CASR_VER_SHIFT)) & CAU_STR_CASR_VER_MASK)

/*! @name STR_CAA - Accumulator register - Store Register command */
#define CAU_STR_CAA_ACC_MASK                     (0xFFFFFFFFU)
#define CAU_STR_CAA_ACC_SHIFT                    (0U)
#define CAU_STR_CAA_ACC(x)                       (((uint32_t)(((uint32_t)(x)) << CAU_STR_CAA_ACC_SHIFT)) & CAU_STR_CAA_ACC_MASK)

/*! @name STR_CA - General Purpose Register 0 - Store Register command..General Purpose Register 8 - Store Register command */
#define CAU_STR_CA_CA0_MASK                      (0xFFFFFFFFU)
#define CAU_STR_CA_CA0_SHIFT                     (0U)
#define CAU_STR_CA_CA0(x)                        (((uint32_t)(((uint32_t)(x)) << CAU_STR_CA_CA0_SHIFT)) & CAU_STR_CA_CA0_MASK)
#define CAU_STR_CA_CA1_MASK                      (0xFFFFFFFFU)
#define CAU_STR_CA_CA1_SHIFT                     (0U)
#define CAU_STR_CA_CA1(x)                        (((uint32_t)(((uint32_t)(x)) << CAU_STR_CA_CA1_SHIFT)) & CAU_STR_CA_CA1_MASK)
#define CAU_STR_CA_CA2_MASK                      (0xFFFFFFFFU)
#define CAU_STR_CA_CA2_SHIFT                     (0U)
#define CAU_STR_CA_CA2(x)                        (((uint32_t)(((uint32_t)(x)) << CAU_STR_CA_CA2_SHIFT)) & CAU_STR_CA_CA2_MASK)
#define CAU_STR_CA_CA3_MASK                      (0xFFFFFFFFU)
#define CAU_STR_CA_CA3_SHIFT                     (0U)
#define CAU_STR_CA_CA3(x)                        (((uint32_t)(((uint32_t)(x)) << CAU_STR_CA_CA3_SHIFT)) & CAU_STR_CA_CA3_MASK)
#define CAU_STR_CA_CA4_MASK                      (0xFFFFFFFFU)
#define CAU_STR_CA_CA4_SHIFT                     (0U)
#define CAU_STR_CA_CA4(x)                        (((uint32_t)(((uint32_t)(x)) << CAU_STR_CA_CA4_SHIFT)) & CAU_STR_CA_CA4_MASK)
#define CAU_STR_CA_CA5_MASK                      (0xFFFFFFFFU)
#define CAU_STR_CA_CA5_SHIFT                     (0U)
#define CAU_STR_CA_CA5(x)                        (((uint32_t)(((uint32_t)(x)) << CAU_STR_CA_CA5_SHIFT)) & CAU_STR_CA_CA5_MASK)
#define CAU_STR_CA_CA6_MASK                      (0xFFFFFFFFU)
#define CAU_STR_CA_CA6_SHIFT                     (0U)
#define CAU_STR_CA_CA6(x)                        (((uint32_t)(((uint32_t)(x)) << CAU_STR_CA_CA6_SHIFT)) & CAU_STR_CA_CA6_MASK)
#define CAU_STR_CA_CA7_MASK                      (0xFFFFFFFFU)
#define CAU_STR_CA_CA7_SHIFT                     (0U)
#define CAU_STR_CA_CA7(x)                        (((uint32_t)(((uint32_t)(x)) << CAU_STR_CA_CA7_SHIFT)) & CAU_STR_CA_CA7_MASK)
#define CAU_STR_CA_CA8_MASK                      (0xFFFFFFFFU)
#define CAU_STR_CA_CA8_SHIFT                     (0U)
#define CAU_STR_CA_CA8(x)                        (((uint32_t)(((uint32_t)(x)) << CAU_STR_CA_CA8_SHIFT)) & CAU_STR_CA_CA8_MASK)

/* The count of CAU_STR_CA */
#define CAU_STR_CA_COUNT                         (9U)

/*! @name ADR_CASR - Status register - Add Register command */
#define CAU_ADR_CASR_IC_MASK                     (0x1U)
#define CAU_ADR_CASR_IC_SHIFT                    (0U)
#define CAU_ADR_CASR_IC(x)                       (((uint32_t)(((uint32_t)(x)) << CAU_ADR_CASR_IC_SHIFT)) & CAU_ADR_CASR_IC_MASK)
#define CAU_ADR_CASR_DPE_MASK                    (0x2U)
#define CAU_ADR_CASR_DPE_SHIFT                   (1U)
#define CAU_ADR_CASR_DPE(x)                      (((uint32_t)(((uint32_t)(x)) << CAU_ADR_CASR_DPE_SHIFT)) & CAU_ADR_CASR_DPE_MASK)
#define CAU_ADR_CASR_VER_MASK                    (0xF0000000U)
#define CAU_ADR_CASR_VER_SHIFT                   (28U)
#define CAU_ADR_CASR_VER(x)                      (((uint32_t)(((uint32_t)(x)) << CAU_ADR_CASR_VER_SHIFT)) & CAU_ADR_CASR_VER_MASK)

/*! @name ADR_CAA - Accumulator register - Add to register command */
#define CAU_ADR_CAA_ACC_MASK                     (0xFFFFFFFFU)
#define CAU_ADR_CAA_ACC_SHIFT                    (0U)
#define CAU_ADR_CAA_ACC(x)                       (((uint32_t)(((uint32_t)(x)) << CAU_ADR_CAA_ACC_SHIFT)) & CAU_ADR_CAA_ACC_MASK)

/*! @name ADR_CA - General Purpose Register 0 - Add to register command..General Purpose Register 8 - Add to register command */
#define CAU_ADR_CA_CA0_MASK                      (0xFFFFFFFFU)
#define CAU_ADR_CA_CA0_SHIFT                     (0U)
#define CAU_ADR_CA_CA0(x)                        (((uint32_t)(((uint32_t)(x)) << CAU_ADR_CA_CA0_SHIFT)) & CAU_ADR_CA_CA0_MASK)
#define CAU_ADR_CA_CA1_MASK                      (0xFFFFFFFFU)
#define CAU_ADR_CA_CA1_SHIFT                     (0U)
#define CAU_ADR_CA_CA1(x)                        (((uint32_t)(((uint32_t)(x)) << CAU_ADR_CA_CA1_SHIFT)) & CAU_ADR_CA_CA1_MASK)
#define CAU_ADR_CA_CA2_MASK                      (0xFFFFFFFFU)
#define CAU_ADR_CA_CA2_SHIFT                     (0U)
#define CAU_ADR_CA_CA2(x)                        (((uint32_t)(((uint32_t)(x)) << CAU_ADR_CA_CA2_SHIFT)) & CAU_ADR_CA_CA2_MASK)
#define CAU_ADR_CA_CA3_MASK                      (0xFFFFFFFFU)
#define CAU_ADR_CA_CA3_SHIFT                     (0U)
#define CAU_ADR_CA_CA3(x)                        (((uint32_t)(((uint32_t)(x)) << CAU_ADR_CA_CA3_SHIFT)) & CAU_ADR_CA_CA3_MASK)
#define CAU_ADR_CA_CA4_MASK                      (0xFFFFFFFFU)
#define CAU_ADR_CA_CA4_SHIFT                     (0U)
#define CAU_ADR_CA_CA4(x)                        (((uint32_t)(((uint32_t)(x)) << CAU_ADR_CA_CA4_SHIFT)) & CAU_ADR_CA_CA4_MASK)
#define CAU_ADR_CA_CA5_MASK                      (0xFFFFFFFFU)
#define CAU_ADR_CA_CA5_SHIFT                     (0U)
#define CAU_ADR_CA_CA5(x)                        (((uint32_t)(((uint32_t)(x)) << CAU_ADR_CA_CA5_SHIFT)) & CAU_ADR_CA_CA5_MASK)
#define CAU_ADR_CA_CA6_MASK                      (0xFFFFFFFFU)
#define CAU_ADR_CA_CA6_SHIFT                     (0U)
#define CAU_ADR_CA_CA6(x)                        (((uint32_t)(((uint32_t)(x)) << CAU_ADR_CA_CA6_SHIFT)) & CAU_ADR_CA_CA6_MASK)
#define CAU_ADR_CA_CA7_MASK                      (0xFFFFFFFFU)
#define CAU_ADR_CA_CA7_SHIFT                     (0U)
#define CAU_ADR_CA_CA7(x)                        (((uint32_t)(((uint32_t)(x)) << CAU_ADR_CA_CA7_SHIFT)) & CAU_ADR_CA_CA7_MASK)
#define CAU_ADR_CA_CA8_MASK                      (0xFFFFFFFFU)
#define CAU_ADR_CA_CA8_SHIFT                     (0U)
#define CAU_ADR_CA_CA8(x)                        (((uint32_t)(((uint32_t)(x)) << CAU_ADR_CA_CA8_SHIFT)) & CAU_ADR_CA_CA8_MASK)

/* The count of CAU_ADR_CA */
#define CAU_ADR_CA_COUNT                         (9U)

/*! @name RADR_CASR - Status register - Reverse and Add to Register command */
#define CAU_RADR_CASR_IC_MASK                    (0x1U)
#define CAU_RADR_CASR_IC_SHIFT                   (0U)
#define CAU_RADR_CASR_IC(x)                      (((uint32_t)(((uint32_t)(x)) << CAU_RADR_CASR_IC_SHIFT)) & CAU_RADR_CASR_IC_MASK)
#define CAU_RADR_CASR_DPE_MASK                   (0x2U)
#define CAU_RADR_CASR_DPE_SHIFT                  (1U)
#define CAU_RADR_CASR_DPE(x)                     (((uint32_t)(((uint32_t)(x)) << CAU_RADR_CASR_DPE_SHIFT)) & CAU_RADR_CASR_DPE_MASK)
#define CAU_RADR_CASR_VER_MASK                   (0xF0000000U)
#define CAU_RADR_CASR_VER_SHIFT                  (28U)
#define CAU_RADR_CASR_VER(x)                     (((uint32_t)(((uint32_t)(x)) << CAU_RADR_CASR_VER_SHIFT)) & CAU_RADR_CASR_VER_MASK)

/*! @name RADR_CAA - Accumulator register - Reverse and Add to Register command */
#define CAU_RADR_CAA_ACC_MASK                    (0xFFFFFFFFU)
#define CAU_RADR_CAA_ACC_SHIFT                   (0U)
#define CAU_RADR_CAA_ACC(x)                      (((uint32_t)(((uint32_t)(x)) << CAU_RADR_CAA_ACC_SHIFT)) & CAU_RADR_CAA_ACC_MASK)

/*! @name RADR_CA - General Purpose Register 0 - Reverse and Add to Register command..General Purpose Register 8 - Reverse and Add to Register command */
#define CAU_RADR_CA_CA0_MASK                     (0xFFFFFFFFU)
#define CAU_RADR_CA_CA0_SHIFT                    (0U)
#define CAU_RADR_CA_CA0(x)                       (((uint32_t)(((uint32_t)(x)) << CAU_RADR_CA_CA0_SHIFT)) & CAU_RADR_CA_CA0_MASK)
#define CAU_RADR_CA_CA1_MASK                     (0xFFFFFFFFU)
#define CAU_RADR_CA_CA1_SHIFT                    (0U)
#define CAU_RADR_CA_CA1(x)                       (((uint32_t)(((uint32_t)(x)) << CAU_RADR_CA_CA1_SHIFT)) & CAU_RADR_CA_CA1_MASK)
#define CAU_RADR_CA_CA2_MASK                     (0xFFFFFFFFU)
#define CAU_RADR_CA_CA2_SHIFT                    (0U)
#define CAU_RADR_CA_CA2(x)                       (((uint32_t)(((uint32_t)(x)) << CAU_RADR_CA_CA2_SHIFT)) & CAU_RADR_CA_CA2_MASK)
#define CAU_RADR_CA_CA3_MASK                     (0xFFFFFFFFU)
#define CAU_RADR_CA_CA3_SHIFT                    (0U)
#define CAU_RADR_CA_CA3(x)                       (((uint32_t)(((uint32_t)(x)) << CAU_RADR_CA_CA3_SHIFT)) & CAU_RADR_CA_CA3_MASK)
#define CAU_RADR_CA_CA4_MASK                     (0xFFFFFFFFU)
#define CAU_RADR_CA_CA4_SHIFT                    (0U)
#define CAU_RADR_CA_CA4(x)                       (((uint32_t)(((uint32_t)(x)) << CAU_RADR_CA_CA4_SHIFT)) & CAU_RADR_CA_CA4_MASK)
#define CAU_RADR_CA_CA5_MASK                     (0xFFFFFFFFU)
#define CAU_RADR_CA_CA5_SHIFT                    (0U)
#define CAU_RADR_CA_CA5(x)                       (((uint32_t)(((uint32_t)(x)) << CAU_RADR_CA_CA5_SHIFT)) & CAU_RADR_CA_CA5_MASK)
#define CAU_RADR_CA_CA6_MASK                     (0xFFFFFFFFU)
#define CAU_RADR_CA_CA6_SHIFT                    (0U)
#define CAU_RADR_CA_CA6(x)                       (((uint32_t)(((uint32_t)(x)) << CAU_RADR_CA_CA6_SHIFT)) & CAU_RADR_CA_CA6_MASK)
#define CAU_RADR_CA_CA7_MASK                     (0xFFFFFFFFU)
#define CAU_RADR_CA_CA7_SHIFT                    (0U)
#define CAU_RADR_CA_CA7(x)                       (((uint32_t)(((uint32_t)(x)) << CAU_RADR_CA_CA7_SHIFT)) & CAU_RADR_CA_CA7_MASK)
#define CAU_RADR_CA_CA8_MASK                     (0xFFFFFFFFU)
#define CAU_RADR_CA_CA8_SHIFT                    (0U)
#define CAU_RADR_CA_CA8(x)                       (((uint32_t)(((uint32_t)(x)) << CAU_RADR_CA_CA8_SHIFT)) & CAU_RADR_CA_CA8_MASK)

/* The count of CAU_RADR_CA */
#define CAU_RADR_CA_COUNT                        (9U)

/*! @name XOR_CASR - Status register - Exclusive Or command */
#define CAU_XOR_CASR_IC_MASK                     (0x1U)
#define CAU_XOR_CASR_IC_SHIFT                    (0U)
#define CAU_XOR_CASR_IC(x)                       (((uint32_t)(((uint32_t)(x)) << CAU_XOR_CASR_IC_SHIFT)) & CAU_XOR_CASR_IC_MASK)
#define CAU_XOR_CASR_DPE_MASK                    (0x2U)
#define CAU_XOR_CASR_DPE_SHIFT                   (1U)
#define CAU_XOR_CASR_DPE(x)                      (((uint32_t)(((uint32_t)(x)) << CAU_XOR_CASR_DPE_SHIFT)) & CAU_XOR_CASR_DPE_MASK)
#define CAU_XOR_CASR_VER_MASK                    (0xF0000000U)
#define CAU_XOR_CASR_VER_SHIFT                   (28U)
#define CAU_XOR_CASR_VER(x)                      (((uint32_t)(((uint32_t)(x)) << CAU_XOR_CASR_VER_SHIFT)) & CAU_XOR_CASR_VER_MASK)

/*! @name XOR_CAA - Accumulator register - Exclusive Or command */
#define CAU_XOR_CAA_ACC_MASK                     (0xFFFFFFFFU)
#define CAU_XOR_CAA_ACC_SHIFT                    (0U)
#define CAU_XOR_CAA_ACC(x)                       (((uint32_t)(((uint32_t)(x)) << CAU_XOR_CAA_ACC_SHIFT)) & CAU_XOR_CAA_ACC_MASK)

/*! @name XOR_CA - General Purpose Register 0 - Exclusive Or command..General Purpose Register 8 - Exclusive Or command */
#define CAU_XOR_CA_CA0_MASK                      (0xFFFFFFFFU)
#define CAU_XOR_CA_CA0_SHIFT                     (0U)
#define CAU_XOR_CA_CA0(x)                        (((uint32_t)(((uint32_t)(x)) << CAU_XOR_CA_CA0_SHIFT)) & CAU_XOR_CA_CA0_MASK)
#define CAU_XOR_CA_CA1_MASK                      (0xFFFFFFFFU)
#define CAU_XOR_CA_CA1_SHIFT                     (0U)
#define CAU_XOR_CA_CA1(x)                        (((uint32_t)(((uint32_t)(x)) << CAU_XOR_CA_CA1_SHIFT)) & CAU_XOR_CA_CA1_MASK)
#define CAU_XOR_CA_CA2_MASK                      (0xFFFFFFFFU)
#define CAU_XOR_CA_CA2_SHIFT                     (0U)
#define CAU_XOR_CA_CA2(x)                        (((uint32_t)(((uint32_t)(x)) << CAU_XOR_CA_CA2_SHIFT)) & CAU_XOR_CA_CA2_MASK)
#define CAU_XOR_CA_CA3_MASK                      (0xFFFFFFFFU)
#define CAU_XOR_CA_CA3_SHIFT                     (0U)
#define CAU_XOR_CA_CA3(x)                        (((uint32_t)(((uint32_t)(x)) << CAU_XOR_CA_CA3_SHIFT)) & CAU_XOR_CA_CA3_MASK)
#define CAU_XOR_CA_CA4_MASK                      (0xFFFFFFFFU)
#define CAU_XOR_CA_CA4_SHIFT                     (0U)
#define CAU_XOR_CA_CA4(x)                        (((uint32_t)(((uint32_t)(x)) << CAU_XOR_CA_CA4_SHIFT)) & CAU_XOR_CA_CA4_MASK)
#define CAU_XOR_CA_CA5_MASK                      (0xFFFFFFFFU)
#define CAU_XOR_CA_CA5_SHIFT                     (0U)
#define CAU_XOR_CA_CA5(x)                        (((uint32_t)(((uint32_t)(x)) << CAU_XOR_CA_CA5_SHIFT)) & CAU_XOR_CA_CA5_MASK)
#define CAU_XOR_CA_CA6_MASK                      (0xFFFFFFFFU)
#define CAU_XOR_CA_CA6_SHIFT                     (0U)
#define CAU_XOR_CA_CA6(x)                        (((uint32_t)(((uint32_t)(x)) << CAU_XOR_CA_CA6_SHIFT)) & CAU_XOR_CA_CA6_MASK)
#define CAU_XOR_CA_CA7_MASK                      (0xFFFFFFFFU)
#define CAU_XOR_CA_CA7_SHIFT                     (0U)
#define CAU_XOR_CA_CA7(x)                        (((uint32_t)(((uint32_t)(x)) << CAU_XOR_CA_CA7_SHIFT)) & CAU_XOR_CA_CA7_MASK)
#define CAU_XOR_CA_CA8_MASK                      (0xFFFFFFFFU)
#define CAU_XOR_CA_CA8_SHIFT                     (0U)
#define CAU_XOR_CA_CA8(x)                        (((uint32_t)(((uint32_t)(x)) << CAU_XOR_CA_CA8_SHIFT)) & CAU_XOR_CA_CA8_MASK)

/* The count of CAU_XOR_CA */
#define CAU_XOR_CA_COUNT                         (9U)

/*! @name ROTL_CASR - Status register - Rotate Left command */
#define CAU_ROTL_CASR_IC_MASK                    (0x1U)
#define CAU_ROTL_CASR_IC_SHIFT                   (0U)
#define CAU_ROTL_CASR_IC(x)                      (((uint32_t)(((uint32_t)(x)) << CAU_ROTL_CASR_IC_SHIFT)) & CAU_ROTL_CASR_IC_MASK)
#define CAU_ROTL_CASR_DPE_MASK                   (0x2U)
#define CAU_ROTL_CASR_DPE_SHIFT                  (1U)
#define CAU_ROTL_CASR_DPE(x)                     (((uint32_t)(((uint32_t)(x)) << CAU_ROTL_CASR_DPE_SHIFT)) & CAU_ROTL_CASR_DPE_MASK)
#define CAU_ROTL_CASR_VER_MASK                   (0xF0000000U)
#define CAU_ROTL_CASR_VER_SHIFT                  (28U)
#define CAU_ROTL_CASR_VER(x)                     (((uint32_t)(((uint32_t)(x)) << CAU_ROTL_CASR_VER_SHIFT)) & CAU_ROTL_CASR_VER_MASK)

/*! @name ROTL_CAA - Accumulator register - Rotate Left command */
#define CAU_ROTL_CAA_ACC_MASK                    (0xFFFFFFFFU)
#define CAU_ROTL_CAA_ACC_SHIFT                   (0U)
#define CAU_ROTL_CAA_ACC(x)                      (((uint32_t)(((uint32_t)(x)) << CAU_ROTL_CAA_ACC_SHIFT)) & CAU_ROTL_CAA_ACC_MASK)

/*! @name ROTL_CA - General Purpose Register 0 - Rotate Left command..General Purpose Register 8 - Rotate Left command */
#define CAU_ROTL_CA_CA0_MASK                     (0xFFFFFFFFU)
#define CAU_ROTL_CA_CA0_SHIFT                    (0U)
#define CAU_ROTL_CA_CA0(x)                       (((uint32_t)(((uint32_t)(x)) << CAU_ROTL_CA_CA0_SHIFT)) & CAU_ROTL_CA_CA0_MASK)
#define CAU_ROTL_CA_CA1_MASK                     (0xFFFFFFFFU)
#define CAU_ROTL_CA_CA1_SHIFT                    (0U)
#define CAU_ROTL_CA_CA1(x)                       (((uint32_t)(((uint32_t)(x)) << CAU_ROTL_CA_CA1_SHIFT)) & CAU_ROTL_CA_CA1_MASK)
#define CAU_ROTL_CA_CA2_MASK                     (0xFFFFFFFFU)
#define CAU_ROTL_CA_CA2_SHIFT                    (0U)
#define CAU_ROTL_CA_CA2(x)                       (((uint32_t)(((uint32_t)(x)) << CAU_ROTL_CA_CA2_SHIFT)) & CAU_ROTL_CA_CA2_MASK)
#define CAU_ROTL_CA_CA3_MASK                     (0xFFFFFFFFU)
#define CAU_ROTL_CA_CA3_SHIFT                    (0U)
#define CAU_ROTL_CA_CA3(x)                       (((uint32_t)(((uint32_t)(x)) << CAU_ROTL_CA_CA3_SHIFT)) & CAU_ROTL_CA_CA3_MASK)
#define CAU_ROTL_CA_CA4_MASK                     (0xFFFFFFFFU)
#define CAU_ROTL_CA_CA4_SHIFT                    (0U)
#define CAU_ROTL_CA_CA4(x)                       (((uint32_t)(((uint32_t)(x)) << CAU_ROTL_CA_CA4_SHIFT)) & CAU_ROTL_CA_CA4_MASK)
#define CAU_ROTL_CA_CA5_MASK                     (0xFFFFFFFFU)
#define CAU_ROTL_CA_CA5_SHIFT                    (0U)
#define CAU_ROTL_CA_CA5(x)                       (((uint32_t)(((uint32_t)(x)) << CAU_ROTL_CA_CA5_SHIFT)) & CAU_ROTL_CA_CA5_MASK)
#define CAU_ROTL_CA_CA6_MASK                     (0xFFFFFFFFU)
#define CAU_ROTL_CA_CA6_SHIFT                    (0U)
#define CAU_ROTL_CA_CA6(x)                       (((uint32_t)(((uint32_t)(x)) << CAU_ROTL_CA_CA6_SHIFT)) & CAU_ROTL_CA_CA6_MASK)
#define CAU_ROTL_CA_CA7_MASK                     (0xFFFFFFFFU)
#define CAU_ROTL_CA_CA7_SHIFT                    (0U)
#define CAU_ROTL_CA_CA7(x)                       (((uint32_t)(((uint32_t)(x)) << CAU_ROTL_CA_CA7_SHIFT)) & CAU_ROTL_CA_CA7_MASK)
#define CAU_ROTL_CA_CA8_MASK                     (0xFFFFFFFFU)
#define CAU_ROTL_CA_CA8_SHIFT                    (0U)
#define CAU_ROTL_CA_CA8(x)                       (((uint32_t)(((uint32_t)(x)) << CAU_ROTL_CA_CA8_SHIFT)) & CAU_ROTL_CA_CA8_MASK)

/* The count of CAU_ROTL_CA */
#define CAU_ROTL_CA_COUNT                        (9U)

/*! @name AESC_CASR - Status register - AES Column Operation command */
#define CAU_AESC_CASR_IC_MASK                    (0x1U)
#define CAU_AESC_CASR_IC_SHIFT                   (0U)
#define CAU_AESC_CASR_IC(x)                      (((uint32_t)(((uint32_t)(x)) << CAU_AESC_CASR_IC_SHIFT)) & CAU_AESC_CASR_IC_MASK)
#define CAU_AESC_CASR_DPE_MASK                   (0x2U)
#define CAU_AESC_CASR_DPE_SHIFT                  (1U)
#define CAU_AESC_CASR_DPE(x)                     (((uint32_t)(((uint32_t)(x)) << CAU_AESC_CASR_DPE_SHIFT)) & CAU_AESC_CASR_DPE_MASK)
#define CAU_AESC_CASR_VER_MASK                   (0xF0000000U)
#define CAU_AESC_CASR_VER_SHIFT                  (28U)
#define CAU_AESC_CASR_VER(x)                     (((uint32_t)(((uint32_t)(x)) << CAU_AESC_CASR_VER_SHIFT)) & CAU_AESC_CASR_VER_MASK)

/*! @name AESC_CAA - Accumulator register - AES Column Operation command */
#define CAU_AESC_CAA_ACC_MASK                    (0xFFFFFFFFU)
#define CAU_AESC_CAA_ACC_SHIFT                   (0U)
#define CAU_AESC_CAA_ACC(x)                      (((uint32_t)(((uint32_t)(x)) << CAU_AESC_CAA_ACC_SHIFT)) & CAU_AESC_CAA_ACC_MASK)

/*! @name AESC_CA - General Purpose Register 0 - AES Column Operation command..General Purpose Register 8 - AES Column Operation command */
#define CAU_AESC_CA_CA0_MASK                     (0xFFFFFFFFU)
#define CAU_AESC_CA_CA0_SHIFT                    (0U)
#define CAU_AESC_CA_CA0(x)                       (((uint32_t)(((uint32_t)(x)) << CAU_AESC_CA_CA0_SHIFT)) & CAU_AESC_CA_CA0_MASK)
#define CAU_AESC_CA_CA1_MASK                     (0xFFFFFFFFU)
#define CAU_AESC_CA_CA1_SHIFT                    (0U)
#define CAU_AESC_CA_CA1(x)                       (((uint32_t)(((uint32_t)(x)) << CAU_AESC_CA_CA1_SHIFT)) & CAU_AESC_CA_CA1_MASK)
#define CAU_AESC_CA_CA2_MASK                     (0xFFFFFFFFU)
#define CAU_AESC_CA_CA2_SHIFT                    (0U)
#define CAU_AESC_CA_CA2(x)                       (((uint32_t)(((uint32_t)(x)) << CAU_AESC_CA_CA2_SHIFT)) & CAU_AESC_CA_CA2_MASK)
#define CAU_AESC_CA_CA3_MASK                     (0xFFFFFFFFU)
#define CAU_AESC_CA_CA3_SHIFT                    (0U)
#define CAU_AESC_CA_CA3(x)                       (((uint32_t)(((uint32_t)(x)) << CAU_AESC_CA_CA3_SHIFT)) & CAU_AESC_CA_CA3_MASK)
#define CAU_AESC_CA_CA4_MASK                     (0xFFFFFFFFU)
#define CAU_AESC_CA_CA4_SHIFT                    (0U)
#define CAU_AESC_CA_CA4(x)                       (((uint32_t)(((uint32_t)(x)) << CAU_AESC_CA_CA4_SHIFT)) & CAU_AESC_CA_CA4_MASK)
#define CAU_AESC_CA_CA5_MASK                     (0xFFFFFFFFU)
#define CAU_AESC_CA_CA5_SHIFT                    (0U)
#define CAU_AESC_CA_CA5(x)                       (((uint32_t)(((uint32_t)(x)) << CAU_AESC_CA_CA5_SHIFT)) & CAU_AESC_CA_CA5_MASK)
#define CAU_AESC_CA_CA6_MASK                     (0xFFFFFFFFU)
#define CAU_AESC_CA_CA6_SHIFT                    (0U)
#define CAU_AESC_CA_CA6(x)                       (((uint32_t)(((uint32_t)(x)) << CAU_AESC_CA_CA6_SHIFT)) & CAU_AESC_CA_CA6_MASK)
#define CAU_AESC_CA_CA7_MASK                     (0xFFFFFFFFU)
#define CAU_AESC_CA_CA7_SHIFT                    (0U)
#define CAU_AESC_CA_CA7(x)                       (((uint32_t)(((uint32_t)(x)) << CAU_AESC_CA_CA7_SHIFT)) & CAU_AESC_CA_CA7_MASK)
#define CAU_AESC_CA_CA8_MASK                     (0xFFFFFFFFU)
#define CAU_AESC_CA_CA8_SHIFT                    (0U)
#define CAU_AESC_CA_CA8(x)                       (((uint32_t)(((uint32_t)(x)) << CAU_AESC_CA_CA8_SHIFT)) & CAU_AESC_CA_CA8_MASK)

/* The count of CAU_AESC_CA */
#define CAU_AESC_CA_COUNT                        (9U)

/*! @name AESIC_CASR - Status register - AES Inverse Column Operation command */
#define CAU_AESIC_CASR_IC_MASK                   (0x1U)
#define CAU_AESIC_CASR_IC_SHIFT                  (0U)
#define CAU_AESIC_CASR_IC(x)                     (((uint32_t)(((uint32_t)(x)) << CAU_AESIC_CASR_IC_SHIFT)) & CAU_AESIC_CASR_IC_MASK)
#define CAU_AESIC_CASR_DPE_MASK                  (0x2U)
#define CAU_AESIC_CASR_DPE_SHIFT                 (1U)
#define CAU_AESIC_CASR_DPE(x)                    (((uint32_t)(((uint32_t)(x)) << CAU_AESIC_CASR_DPE_SHIFT)) & CAU_AESIC_CASR_DPE_MASK)
#define CAU_AESIC_CASR_VER_MASK                  (0xF0000000U)
#define CAU_AESIC_CASR_VER_SHIFT                 (28U)
#define CAU_AESIC_CASR_VER(x)                    (((uint32_t)(((uint32_t)(x)) << CAU_AESIC_CASR_VER_SHIFT)) & CAU_AESIC_CASR_VER_MASK)

/*! @name AESIC_CAA - Accumulator register - AES Inverse Column Operation command */
#define CAU_AESIC_CAA_ACC_MASK                   (0xFFFFFFFFU)
#define CAU_AESIC_CAA_ACC_SHIFT                  (0U)
#define CAU_AESIC_CAA_ACC(x)                     (((uint32_t)(((uint32_t)(x)) << CAU_AESIC_CAA_ACC_SHIFT)) & CAU_AESIC_CAA_ACC_MASK)

/*! @name AESIC_CA - General Purpose Register 0 - AES Inverse Column Operation command..General Purpose Register 8 - AES Inverse Column Operation command */
#define CAU_AESIC_CA_CA0_MASK                    (0xFFFFFFFFU)
#define CAU_AESIC_CA_CA0_SHIFT                   (0U)
#define CAU_AESIC_CA_CA0(x)                      (((uint32_t)(((uint32_t)(x)) << CAU_AESIC_CA_CA0_SHIFT)) & CAU_AESIC_CA_CA0_MASK)
#define CAU_AESIC_CA_CA1_MASK                    (0xFFFFFFFFU)
#define CAU_AESIC_CA_CA1_SHIFT                   (0U)
#define CAU_AESIC_CA_CA1(x)                      (((uint32_t)(((uint32_t)(x)) << CAU_AESIC_CA_CA1_SHIFT)) & CAU_AESIC_CA_CA1_MASK)
#define CAU_AESIC_CA_CA2_MASK                    (0xFFFFFFFFU)
#define CAU_AESIC_CA_CA2_SHIFT                   (0U)
#define CAU_AESIC_CA_CA2(x)                      (((uint32_t)(((uint32_t)(x)) << CAU_AESIC_CA_CA2_SHIFT)) & CAU_AESIC_CA_CA2_MASK)
#define CAU_AESIC_CA_CA3_MASK                    (0xFFFFFFFFU)
#define CAU_AESIC_CA_CA3_SHIFT                   (0U)
#define CAU_AESIC_CA_CA3(x)                      (((uint32_t)(((uint32_t)(x)) << CAU_AESIC_CA_CA3_SHIFT)) & CAU_AESIC_CA_CA3_MASK)
#define CAU_AESIC_CA_CA4_MASK                    (0xFFFFFFFFU)
#define CAU_AESIC_CA_CA4_SHIFT                   (0U)
#define CAU_AESIC_CA_CA4(x)                      (((uint32_t)(((uint32_t)(x)) << CAU_AESIC_CA_CA4_SHIFT)) & CAU_AESIC_CA_CA4_MASK)
#define CAU_AESIC_CA_CA5_MASK                    (0xFFFFFFFFU)
#define CAU_AESIC_CA_CA5_SHIFT                   (0U)
#define CAU_AESIC_CA_CA5(x)                      (((uint32_t)(((uint32_t)(x)) << CAU_AESIC_CA_CA5_SHIFT)) & CAU_AESIC_CA_CA5_MASK)
#define CAU_AESIC_CA_CA6_MASK                    (0xFFFFFFFFU)
#define CAU_AESIC_CA_CA6_SHIFT                   (0U)
#define CAU_AESIC_CA_CA6(x)                      (((uint32_t)(((uint32_t)(x)) << CAU_AESIC_CA_CA6_SHIFT)) & CAU_AESIC_CA_CA6_MASK)
#define CAU_AESIC_CA_CA7_MASK                    (0xFFFFFFFFU)
#define CAU_AESIC_CA_CA7_SHIFT                   (0U)
#define CAU_AESIC_CA_CA7(x)                      (((uint32_t)(((uint32_t)(x)) << CAU_AESIC_CA_CA7_SHIFT)) & CAU_AESIC_CA_CA7_MASK)
#define CAU_AESIC_CA_CA8_MASK                    (0xFFFFFFFFU)
#define CAU_AESIC_CA_CA8_SHIFT                   (0U)
#define CAU_AESIC_CA_CA8(x)                      (((uint32_t)(((uint32_t)(x)) << CAU_AESIC_CA_CA8_SHIFT)) & CAU_AESIC_CA_CA8_MASK)

/* The count of CAU_AESIC_CA */
#define CAU_AESIC_CA_COUNT                       (9U)


/*!
 * @}
 */ /* end of group CAU_Register_Masks */


/* CAU - Peripheral instance base addresses */
/** Peripheral CAU base address */
#define CAU_BASE                                 (0xE0081000u)
/** Peripheral CAU base pointer */
#define CAU                                      ((CAU_Type *)CAU_BASE)
/** Array initializer of CAU peripheral base addresses */
#define CAU_BASE_ADDRS                           { CAU_BASE }
/** Array initializer of CAU peripheral base pointers */
#define CAU_BASE_PTRS                            { CAU }

/*!
 * @}
 */ /* end of group CAU_Peripheral_Access_Layer */


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
/** Peripheral CMP1 base address */
#define CMP1_BASE                                (0x40073008u)
/** Peripheral CMP1 base pointer */
#define CMP1                                     ((CMP_Type *)CMP1_BASE)
/** Array initializer of CMP peripheral base addresses */
#define CMP_BASE_ADDRS                           { CMP0_BASE, CMP1_BASE }
/** Array initializer of CMP peripheral base pointers */
#define CMP_BASE_PTRS                            { CMP0, CMP1 }
/** Interrupt vectors for the CMP peripheral type */
#define CMP_IRQS                                 { CMP0_IRQn, CMP1_IRQn }

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
   -- CRC Peripheral Access Layer
   ---------------------------------------------------------------------------- */

/*!
 * @addtogroup CRC_Peripheral_Access_Layer CRC Peripheral Access Layer
 * @{
 */

/** CRC - Register Layout Typedef */
typedef struct {
  union {                                          /* offset: 0x0 */
    struct {                                         /* offset: 0x0 */
      __IO uint16_t DATAL;                             /**< CRC_DATAL register., offset: 0x0 */
      __IO uint16_t DATAH;                             /**< CRC_DATAH register., offset: 0x2 */
    } ACCESS16BIT;
    __IO uint32_t DATA;                              /**< CRC Data register, offset: 0x0 */
    struct {                                         /* offset: 0x0 */
      __IO uint8_t DATALL;                             /**< CRC_DATALL register., offset: 0x0 */
      __IO uint8_t DATALU;                             /**< CRC_DATALU register., offset: 0x1 */
      __IO uint8_t DATAHL;                             /**< CRC_DATAHL register., offset: 0x2 */
      __IO uint8_t DATAHU;                             /**< CRC_DATAHU register., offset: 0x3 */
    } ACCESS8BIT;
  };
  union {                                          /* offset: 0x4 */
    struct {                                         /* offset: 0x4 */
      __IO uint16_t GPOLYL;                            /**< CRC_GPOLYL register., offset: 0x4 */
      __IO uint16_t GPOLYH;                            /**< CRC_GPOLYH register., offset: 0x6 */
    } GPOLY_ACCESS16BIT;
    __IO uint32_t GPOLY;                             /**< CRC Polynomial register, offset: 0x4 */
    struct {                                         /* offset: 0x4 */
      __IO uint8_t GPOLYLL;                            /**< CRC_GPOLYLL register., offset: 0x4 */
      __IO uint8_t GPOLYLU;                            /**< CRC_GPOLYLU register., offset: 0x5 */
      __IO uint8_t GPOLYHL;                            /**< CRC_GPOLYHL register., offset: 0x6 */
      __IO uint8_t GPOLYHU;                            /**< CRC_GPOLYHU register., offset: 0x7 */
    } GPOLY_ACCESS8BIT;
  };
  union {                                          /* offset: 0x8 */
    __IO uint32_t CTRL;                              /**< CRC Control register, offset: 0x8 */
    struct {                                         /* offset: 0x8 */
           uint8_t RESERVED_0[3];
      __IO uint8_t CTRLHU;                             /**< CRC_CTRLHU register., offset: 0xB */
    } CTRL_ACCESS8BIT;
  };
} CRC_Type;

/* ----------------------------------------------------------------------------
   -- CRC Register Masks
   ---------------------------------------------------------------------------- */

/*!
 * @addtogroup CRC_Register_Masks CRC Register Masks
 * @{
 */

/*! @name DATAL - CRC_DATAL register. */
#define CRC_DATAL_DATAL_MASK                     (0xFFFFU)
#define CRC_DATAL_DATAL_SHIFT                    (0U)
#define CRC_DATAL_DATAL(x)                       (((uint16_t)(((uint16_t)(x)) << CRC_DATAL_DATAL_SHIFT)) & CRC_DATAL_DATAL_MASK)

/*! @name DATAH - CRC_DATAH register. */
#define CRC_DATAH_DATAH_MASK                     (0xFFFFU)
#define CRC_DATAH_DATAH_SHIFT                    (0U)
#define CRC_DATAH_DATAH(x)                       (((uint16_t)(((uint16_t)(x)) << CRC_DATAH_DATAH_SHIFT)) & CRC_DATAH_DATAH_MASK)

/*! @name DATA - CRC Data register */
#define CRC_DATA_LL_MASK                         (0xFFU)
#define CRC_DATA_LL_SHIFT                        (0U)
#define CRC_DATA_LL(x)                           (((uint32_t)(((uint32_t)(x)) << CRC_DATA_LL_SHIFT)) & CRC_DATA_LL_MASK)
#define CRC_DATA_LU_MASK                         (0xFF00U)
#define CRC_DATA_LU_SHIFT                        (8U)
#define CRC_DATA_LU(x)                           (((uint32_t)(((uint32_t)(x)) << CRC_DATA_LU_SHIFT)) & CRC_DATA_LU_MASK)
#define CRC_DATA_HL_MASK                         (0xFF0000U)
#define CRC_DATA_HL_SHIFT                        (16U)
#define CRC_DATA_HL(x)                           (((uint32_t)(((uint32_t)(x)) << CRC_DATA_HL_SHIFT)) & CRC_DATA_HL_MASK)
#define CRC_DATA_HU_MASK                         (0xFF000000U)
#define CRC_DATA_HU_SHIFT                        (24U)
#define CRC_DATA_HU(x)                           (((uint32_t)(((uint32_t)(x)) << CRC_DATA_HU_SHIFT)) & CRC_DATA_HU_MASK)

/*! @name DATALL - CRC_DATALL register. */
#define CRC_DATALL_DATALL_MASK                   (0xFFU)
#define CRC_DATALL_DATALL_SHIFT                  (0U)
#define CRC_DATALL_DATALL(x)                     (((uint8_t)(((uint8_t)(x)) << CRC_DATALL_DATALL_SHIFT)) & CRC_DATALL_DATALL_MASK)

/*! @name DATALU - CRC_DATALU register. */
#define CRC_DATALU_DATALU_MASK                   (0xFFU)
#define CRC_DATALU_DATALU_SHIFT                  (0U)
#define CRC_DATALU_DATALU(x)                     (((uint8_t)(((uint8_t)(x)) << CRC_DATALU_DATALU_SHIFT)) & CRC_DATALU_DATALU_MASK)

/*! @name DATAHL - CRC_DATAHL register. */
#define CRC_DATAHL_DATAHL_MASK                   (0xFFU)
#define CRC_DATAHL_DATAHL_SHIFT                  (0U)
#define CRC_DATAHL_DATAHL(x)                     (((uint8_t)(((uint8_t)(x)) << CRC_DATAHL_DATAHL_SHIFT)) & CRC_DATAHL_DATAHL_MASK)

/*! @name DATAHU - CRC_DATAHU register. */
#define CRC_DATAHU_DATAHU_MASK                   (0xFFU)
#define CRC_DATAHU_DATAHU_SHIFT                  (0U)
#define CRC_DATAHU_DATAHU(x)                     (((uint8_t)(((uint8_t)(x)) << CRC_DATAHU_DATAHU_SHIFT)) & CRC_DATAHU_DATAHU_MASK)

/*! @name GPOLYL - CRC_GPOLYL register. */
#define CRC_GPOLYL_GPOLYL_MASK                   (0xFFFFU)
#define CRC_GPOLYL_GPOLYL_SHIFT                  (0U)
#define CRC_GPOLYL_GPOLYL(x)                     (((uint16_t)(((uint16_t)(x)) << CRC_GPOLYL_GPOLYL_SHIFT)) & CRC_GPOLYL_GPOLYL_MASK)

/*! @name GPOLYH - CRC_GPOLYH register. */
#define CRC_GPOLYH_GPOLYH_MASK                   (0xFFFFU)
#define CRC_GPOLYH_GPOLYH_SHIFT                  (0U)
#define CRC_GPOLYH_GPOLYH(x)                     (((uint16_t)(((uint16_t)(x)) << CRC_GPOLYH_GPOLYH_SHIFT)) & CRC_GPOLYH_GPOLYH_MASK)

/*! @name GPOLY - CRC Polynomial register */
#define CRC_GPOLY_LOW_MASK                       (0xFFFFU)
#define CRC_GPOLY_LOW_SHIFT                      (0U)
#define CRC_GPOLY_LOW(x)                         (((uint32_t)(((uint32_t)(x)) << CRC_GPOLY_LOW_SHIFT)) & CRC_GPOLY_LOW_MASK)
#define CRC_GPOLY_HIGH_MASK                      (0xFFFF0000U)
#define CRC_GPOLY_HIGH_SHIFT                     (16U)
#define CRC_GPOLY_HIGH(x)                        (((uint32_t)(((uint32_t)(x)) << CRC_GPOLY_HIGH_SHIFT)) & CRC_GPOLY_HIGH_MASK)

/*! @name GPOLYLL - CRC_GPOLYLL register. */
#define CRC_GPOLYLL_GPOLYLL_MASK                 (0xFFU)
#define CRC_GPOLYLL_GPOLYLL_SHIFT                (0U)
#define CRC_GPOLYLL_GPOLYLL(x)                   (((uint8_t)(((uint8_t)(x)) << CRC_GPOLYLL_GPOLYLL_SHIFT)) & CRC_GPOLYLL_GPOLYLL_MASK)

/*! @name GPOLYLU - CRC_GPOLYLU register. */
#define CRC_GPOLYLU_GPOLYLU_MASK                 (0xFFU)
#define CRC_GPOLYLU_GPOLYLU_SHIFT                (0U)
#define CRC_GPOLYLU_GPOLYLU(x)                   (((uint8_t)(((uint8_t)(x)) << CRC_GPOLYLU_GPOLYLU_SHIFT)) & CRC_GPOLYLU_GPOLYLU_MASK)

/*! @name GPOLYHL - CRC_GPOLYHL register. */
#define CRC_GPOLYHL_GPOLYHL_MASK                 (0xFFU)
#define CRC_GPOLYHL_GPOLYHL_SHIFT                (0U)
#define CRC_GPOLYHL_GPOLYHL(x)                   (((uint8_t)(((uint8_t)(x)) << CRC_GPOLYHL_GPOLYHL_SHIFT)) & CRC_GPOLYHL_GPOLYHL_MASK)

/*! @name GPOLYHU - CRC_GPOLYHU register. */
#define CRC_GPOLYHU_GPOLYHU_MASK                 (0xFFU)
#define CRC_GPOLYHU_GPOLYHU_SHIFT                (0U)
#define CRC_GPOLYHU_GPOLYHU(x)                   (((uint8_t)(((uint8_t)(x)) << CRC_GPOLYHU_GPOLYHU_SHIFT)) & CRC_GPOLYHU_GPOLYHU_MASK)

/*! @name CTRL - CRC Control register */
#define CRC_CTRL_TCRC_MASK                       (0x1000000U)
#define CRC_CTRL_TCRC_SHIFT                      (24U)
#define CRC_CTRL_TCRC(x)                         (((uint32_t)(((uint32_t)(x)) << CRC_CTRL_TCRC_SHIFT)) & CRC_CTRL_TCRC_MASK)
#define CRC_CTRL_WAS_MASK                        (0x2000000U)
#define CRC_CTRL_WAS_SHIFT                       (25U)
#define CRC_CTRL_WAS(x)                          (((uint32_t)(((uint32_t)(x)) << CRC_CTRL_WAS_SHIFT)) & CRC_CTRL_WAS_MASK)
#define CRC_CTRL_FXOR_MASK                       (0x4000000U)
#define CRC_CTRL_FXOR_SHIFT                      (26U)
#define CRC_CTRL_FXOR(x)                         (((uint32_t)(((uint32_t)(x)) << CRC_CTRL_FXOR_SHIFT)) & CRC_CTRL_FXOR_MASK)
#define CRC_CTRL_TOTR_MASK                       (0x30000000U)
#define CRC_CTRL_TOTR_SHIFT                      (28U)
#define CRC_CTRL_TOTR(x)                         (((uint32_t)(((uint32_t)(x)) << CRC_CTRL_TOTR_SHIFT)) & CRC_CTRL_TOTR_MASK)
#define CRC_CTRL_TOT_MASK                        (0xC0000000U)
#define CRC_CTRL_TOT_SHIFT                       (30U)
#define CRC_CTRL_TOT(x)                          (((uint32_t)(((uint32_t)(x)) << CRC_CTRL_TOT_SHIFT)) & CRC_CTRL_TOT_MASK)

/*! @name CTRLHU - CRC_CTRLHU register. */
#define CRC_CTRLHU_TCRC_MASK                     (0x1U)
#define CRC_CTRLHU_TCRC_SHIFT                    (0U)
#define CRC_CTRLHU_TCRC(x)                       (((uint8_t)(((uint8_t)(x)) << CRC_CTRLHU_TCRC_SHIFT)) & CRC_CTRLHU_TCRC_MASK)
#define CRC_CTRLHU_WAS_MASK                      (0x2U)
#define CRC_CTRLHU_WAS_SHIFT                     (1U)
#define CRC_CTRLHU_WAS(x)                        (((uint8_t)(((uint8_t)(x)) << CRC_CTRLHU_WAS_SHIFT)) & CRC_CTRLHU_WAS_MASK)
#define CRC_CTRLHU_FXOR_MASK                     (0x4U)
#define CRC_CTRLHU_FXOR_SHIFT                    (2U)
#define CRC_CTRLHU_FXOR(x)                       (((uint8_t)(((uint8_t)(x)) << CRC_CTRLHU_FXOR_SHIFT)) & CRC_CTRLHU_FXOR_MASK)
#define CRC_CTRLHU_TOTR_MASK                     (0x30U)
#define CRC_CTRLHU_TOTR_SHIFT                    (4U)
#define CRC_CTRLHU_TOTR(x)                       (((uint8_t)(((uint8_t)(x)) << CRC_CTRLHU_TOTR_SHIFT)) & CRC_CTRLHU_TOTR_MASK)
#define CRC_CTRLHU_TOT_MASK                      (0xC0U)
#define CRC_CTRLHU_TOT_SHIFT                     (6U)
#define CRC_CTRLHU_TOT(x)                        (((uint8_t)(((uint8_t)(x)) << CRC_CTRLHU_TOT_SHIFT)) & CRC_CTRLHU_TOT_MASK)


/*!
 * @}
 */ /* end of group CRC_Register_Masks */


/* CRC - Peripheral instance base addresses */
/** Peripheral CRC base address */
#define CRC_BASE                                 (0x40032000u)
/** Peripheral CRC base pointer */
#define CRC0                                     ((CRC_Type *)CRC_BASE)
/** Array initializer of CRC peripheral base addresses */
#define CRC_BASE_ADDRS                           { CRC_BASE }
/** Array initializer of CRC peripheral base pointers */
#define CRC_BASE_PTRS                            { CRC0 }

/*!
 * @}
 */ /* end of group CRC_Peripheral_Access_Layer */


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
  __IO uint32_t HRS;                               /**< Hardware Request Status Register, offset: 0x34 */
       uint8_t RESERVED_5[200];
  __IO uint8_t DCHPRI3;                            /**< Channel n Priority Register, offset: 0x100 */
  __IO uint8_t DCHPRI2;                            /**< Channel n Priority Register, offset: 0x101 */
  __IO uint8_t DCHPRI1;                            /**< Channel n Priority Register, offset: 0x102 */
  __IO uint8_t DCHPRI0;                            /**< Channel n Priority Register, offset: 0x103 */
  __IO uint8_t DCHPRI7;                            /**< Channel n Priority Register, offset: 0x104 */
  __IO uint8_t DCHPRI6;                            /**< Channel n Priority Register, offset: 0x105 */
  __IO uint8_t DCHPRI5;                            /**< Channel n Priority Register, offset: 0x106 */
  __IO uint8_t DCHPRI4;                            /**< Channel n Priority Register, offset: 0x107 */
  __IO uint8_t DCHPRI11;                           /**< Channel n Priority Register, offset: 0x108 */
  __IO uint8_t DCHPRI10;                           /**< Channel n Priority Register, offset: 0x109 */
  __IO uint8_t DCHPRI9;                            /**< Channel n Priority Register, offset: 0x10A */
  __IO uint8_t DCHPRI8;                            /**< Channel n Priority Register, offset: 0x10B */
  __IO uint8_t DCHPRI15;                           /**< Channel n Priority Register, offset: 0x10C */
  __IO uint8_t DCHPRI14;                           /**< Channel n Priority Register, offset: 0x10D */
  __IO uint8_t DCHPRI13;                           /**< Channel n Priority Register, offset: 0x10E */
  __IO uint8_t DCHPRI12;                           /**< Channel n Priority Register, offset: 0x10F */
       uint8_t RESERVED_6[3824];
  struct {                                         /* offset: 0x1000, array step: 0x20 */
    __IO uint32_t SADDR;                             /**< TCD Source Address, array offset: 0x1000, array step: 0x20 */
    __IO uint16_t SOFF;                              /**< TCD Signed Source Address Offset, array offset: 0x1004, array step: 0x20 */
    __IO uint16_t ATTR;                              /**< TCD Transfer Attributes, array offset: 0x1006, array step: 0x20 */
    union {                                          /* offset: 0x1008, array step: 0x20 */
      __IO uint32_t NBYTES_MLNO;                       /**< TCD Minor Byte Count (Minor Loop Disabled), array offset: 0x1008, array step: 0x20 */
      __IO uint32_t NBYTES_MLOFFNO;                    /**< TCD Signed Minor Loop Offset (Minor Loop Enabled and Offset Disabled), array offset: 0x1008, array step: 0x20 */
      __IO uint32_t NBYTES_MLOFFYES;                   /**< TCD Signed Minor Loop Offset (Minor Loop and Offset Enabled), array offset: 0x1008, array step: 0x20 */
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
  } TCD[16];
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
#define DMA_ES_ERRCHN_MASK                       (0xF00U)
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
#define DMA_ERQ_ERQ4_MASK                        (0x10U)
#define DMA_ERQ_ERQ4_SHIFT                       (4U)
#define DMA_ERQ_ERQ4(x)                          (((uint32_t)(((uint32_t)(x)) << DMA_ERQ_ERQ4_SHIFT)) & DMA_ERQ_ERQ4_MASK)
#define DMA_ERQ_ERQ5_MASK                        (0x20U)
#define DMA_ERQ_ERQ5_SHIFT                       (5U)
#define DMA_ERQ_ERQ5(x)                          (((uint32_t)(((uint32_t)(x)) << DMA_ERQ_ERQ5_SHIFT)) & DMA_ERQ_ERQ5_MASK)
#define DMA_ERQ_ERQ6_MASK                        (0x40U)
#define DMA_ERQ_ERQ6_SHIFT                       (6U)
#define DMA_ERQ_ERQ6(x)                          (((uint32_t)(((uint32_t)(x)) << DMA_ERQ_ERQ6_SHIFT)) & DMA_ERQ_ERQ6_MASK)
#define DMA_ERQ_ERQ7_MASK                        (0x80U)
#define DMA_ERQ_ERQ7_SHIFT                       (7U)
#define DMA_ERQ_ERQ7(x)                          (((uint32_t)(((uint32_t)(x)) << DMA_ERQ_ERQ7_SHIFT)) & DMA_ERQ_ERQ7_MASK)
#define DMA_ERQ_ERQ8_MASK                        (0x100U)
#define DMA_ERQ_ERQ8_SHIFT                       (8U)
#define DMA_ERQ_ERQ8(x)                          (((uint32_t)(((uint32_t)(x)) << DMA_ERQ_ERQ8_SHIFT)) & DMA_ERQ_ERQ8_MASK)
#define DMA_ERQ_ERQ9_MASK                        (0x200U)
#define DMA_ERQ_ERQ9_SHIFT                       (9U)
#define DMA_ERQ_ERQ9(x)                          (((uint32_t)(((uint32_t)(x)) << DMA_ERQ_ERQ9_SHIFT)) & DMA_ERQ_ERQ9_MASK)
#define DMA_ERQ_ERQ10_MASK                       (0x400U)
#define DMA_ERQ_ERQ10_SHIFT                      (10U)
#define DMA_ERQ_ERQ10(x)                         (((uint32_t)(((uint32_t)(x)) << DMA_ERQ_ERQ10_SHIFT)) & DMA_ERQ_ERQ10_MASK)
#define DMA_ERQ_ERQ11_MASK                       (0x800U)
#define DMA_ERQ_ERQ11_SHIFT                      (11U)
#define DMA_ERQ_ERQ11(x)                         (((uint32_t)(((uint32_t)(x)) << DMA_ERQ_ERQ11_SHIFT)) & DMA_ERQ_ERQ11_MASK)
#define DMA_ERQ_ERQ12_MASK                       (0x1000U)
#define DMA_ERQ_ERQ12_SHIFT                      (12U)
#define DMA_ERQ_ERQ12(x)                         (((uint32_t)(((uint32_t)(x)) << DMA_ERQ_ERQ12_SHIFT)) & DMA_ERQ_ERQ12_MASK)
#define DMA_ERQ_ERQ13_MASK                       (0x2000U)
#define DMA_ERQ_ERQ13_SHIFT                      (13U)
#define DMA_ERQ_ERQ13(x)                         (((uint32_t)(((uint32_t)(x)) << DMA_ERQ_ERQ13_SHIFT)) & DMA_ERQ_ERQ13_MASK)
#define DMA_ERQ_ERQ14_MASK                       (0x4000U)
#define DMA_ERQ_ERQ14_SHIFT                      (14U)
#define DMA_ERQ_ERQ14(x)                         (((uint32_t)(((uint32_t)(x)) << DMA_ERQ_ERQ14_SHIFT)) & DMA_ERQ_ERQ14_MASK)
#define DMA_ERQ_ERQ15_MASK                       (0x8000U)
#define DMA_ERQ_ERQ15_SHIFT                      (15U)
#define DMA_ERQ_ERQ15(x)                         (((uint32_t)(((uint32_t)(x)) << DMA_ERQ_ERQ15_SHIFT)) & DMA_ERQ_ERQ15_MASK)

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
#define DMA_EEI_EEI4_MASK                        (0x10U)
#define DMA_EEI_EEI4_SHIFT                       (4U)
#define DMA_EEI_EEI4(x)                          (((uint32_t)(((uint32_t)(x)) << DMA_EEI_EEI4_SHIFT)) & DMA_EEI_EEI4_MASK)
#define DMA_EEI_EEI5_MASK                        (0x20U)
#define DMA_EEI_EEI5_SHIFT                       (5U)
#define DMA_EEI_EEI5(x)                          (((uint32_t)(((uint32_t)(x)) << DMA_EEI_EEI5_SHIFT)) & DMA_EEI_EEI5_MASK)
#define DMA_EEI_EEI6_MASK                        (0x40U)
#define DMA_EEI_EEI6_SHIFT                       (6U)
#define DMA_EEI_EEI6(x)                          (((uint32_t)(((uint32_t)(x)) << DMA_EEI_EEI6_SHIFT)) & DMA_EEI_EEI6_MASK)
#define DMA_EEI_EEI7_MASK                        (0x80U)
#define DMA_EEI_EEI7_SHIFT                       (7U)
#define DMA_EEI_EEI7(x)                          (((uint32_t)(((uint32_t)(x)) << DMA_EEI_EEI7_SHIFT)) & DMA_EEI_EEI7_MASK)
#define DMA_EEI_EEI8_MASK                        (0x100U)
#define DMA_EEI_EEI8_SHIFT                       (8U)
#define DMA_EEI_EEI8(x)                          (((uint32_t)(((uint32_t)(x)) << DMA_EEI_EEI8_SHIFT)) & DMA_EEI_EEI8_MASK)
#define DMA_EEI_EEI9_MASK                        (0x200U)
#define DMA_EEI_EEI9_SHIFT                       (9U)
#define DMA_EEI_EEI9(x)                          (((uint32_t)(((uint32_t)(x)) << DMA_EEI_EEI9_SHIFT)) & DMA_EEI_EEI9_MASK)
#define DMA_EEI_EEI10_MASK                       (0x400U)
#define DMA_EEI_EEI10_SHIFT                      (10U)
#define DMA_EEI_EEI10(x)                         (((uint32_t)(((uint32_t)(x)) << DMA_EEI_EEI10_SHIFT)) & DMA_EEI_EEI10_MASK)
#define DMA_EEI_EEI11_MASK                       (0x800U)
#define DMA_EEI_EEI11_SHIFT                      (11U)
#define DMA_EEI_EEI11(x)                         (((uint32_t)(((uint32_t)(x)) << DMA_EEI_EEI11_SHIFT)) & DMA_EEI_EEI11_MASK)
#define DMA_EEI_EEI12_MASK                       (0x1000U)
#define DMA_EEI_EEI12_SHIFT                      (12U)
#define DMA_EEI_EEI12(x)                         (((uint32_t)(((uint32_t)(x)) << DMA_EEI_EEI12_SHIFT)) & DMA_EEI_EEI12_MASK)
#define DMA_EEI_EEI13_MASK                       (0x2000U)
#define DMA_EEI_EEI13_SHIFT                      (13U)
#define DMA_EEI_EEI13(x)                         (((uint32_t)(((uint32_t)(x)) << DMA_EEI_EEI13_SHIFT)) & DMA_EEI_EEI13_MASK)
#define DMA_EEI_EEI14_MASK                       (0x4000U)
#define DMA_EEI_EEI14_SHIFT                      (14U)
#define DMA_EEI_EEI14(x)                         (((uint32_t)(((uint32_t)(x)) << DMA_EEI_EEI14_SHIFT)) & DMA_EEI_EEI14_MASK)
#define DMA_EEI_EEI15_MASK                       (0x8000U)
#define DMA_EEI_EEI15_SHIFT                      (15U)
#define DMA_EEI_EEI15(x)                         (((uint32_t)(((uint32_t)(x)) << DMA_EEI_EEI15_SHIFT)) & DMA_EEI_EEI15_MASK)

/*! @name CEEI - Clear Enable Error Interrupt Register */
#define DMA_CEEI_CEEI_MASK                       (0xFU)
#define DMA_CEEI_CEEI_SHIFT                      (0U)
#define DMA_CEEI_CEEI(x)                         (((uint8_t)(((uint8_t)(x)) << DMA_CEEI_CEEI_SHIFT)) & DMA_CEEI_CEEI_MASK)
#define DMA_CEEI_CAEE_MASK                       (0x40U)
#define DMA_CEEI_CAEE_SHIFT                      (6U)
#define DMA_CEEI_CAEE(x)                         (((uint8_t)(((uint8_t)(x)) << DMA_CEEI_CAEE_SHIFT)) & DMA_CEEI_CAEE_MASK)
#define DMA_CEEI_NOP_MASK                        (0x80U)
#define DMA_CEEI_NOP_SHIFT                       (7U)
#define DMA_CEEI_NOP(x)                          (((uint8_t)(((uint8_t)(x)) << DMA_CEEI_NOP_SHIFT)) & DMA_CEEI_NOP_MASK)

/*! @name SEEI - Set Enable Error Interrupt Register */
#define DMA_SEEI_SEEI_MASK                       (0xFU)
#define DMA_SEEI_SEEI_SHIFT                      (0U)
#define DMA_SEEI_SEEI(x)                         (((uint8_t)(((uint8_t)(x)) << DMA_SEEI_SEEI_SHIFT)) & DMA_SEEI_SEEI_MASK)
#define DMA_SEEI_SAEE_MASK                       (0x40U)
#define DMA_SEEI_SAEE_SHIFT                      (6U)
#define DMA_SEEI_SAEE(x)                         (((uint8_t)(((uint8_t)(x)) << DMA_SEEI_SAEE_SHIFT)) & DMA_SEEI_SAEE_MASK)
#define DMA_SEEI_NOP_MASK                        (0x80U)
#define DMA_SEEI_NOP_SHIFT                       (7U)
#define DMA_SEEI_NOP(x)                          (((uint8_t)(((uint8_t)(x)) << DMA_SEEI_NOP_SHIFT)) & DMA_SEEI_NOP_MASK)

/*! @name CERQ - Clear Enable Request Register */
#define DMA_CERQ_CERQ_MASK                       (0xFU)
#define DMA_CERQ_CERQ_SHIFT                      (0U)
#define DMA_CERQ_CERQ(x)                         (((uint8_t)(((uint8_t)(x)) << DMA_CERQ_CERQ_SHIFT)) & DMA_CERQ_CERQ_MASK)
#define DMA_CERQ_CAER_MASK                       (0x40U)
#define DMA_CERQ_CAER_SHIFT                      (6U)
#define DMA_CERQ_CAER(x)                         (((uint8_t)(((uint8_t)(x)) << DMA_CERQ_CAER_SHIFT)) & DMA_CERQ_CAER_MASK)
#define DMA_CERQ_NOP_MASK                        (0x80U)
#define DMA_CERQ_NOP_SHIFT                       (7U)
#define DMA_CERQ_NOP(x)                          (((uint8_t)(((uint8_t)(x)) << DMA_CERQ_NOP_SHIFT)) & DMA_CERQ_NOP_MASK)

/*! @name SERQ - Set Enable Request Register */
#define DMA_SERQ_SERQ_MASK                       (0xFU)
#define DMA_SERQ_SERQ_SHIFT                      (0U)
#define DMA_SERQ_SERQ(x)                         (((uint8_t)(((uint8_t)(x)) << DMA_SERQ_SERQ_SHIFT)) & DMA_SERQ_SERQ_MASK)
#define DMA_SERQ_SAER_MASK                       (0x40U)
#define DMA_SERQ_SAER_SHIFT                      (6U)
#define DMA_SERQ_SAER(x)                         (((uint8_t)(((uint8_t)(x)) << DMA_SERQ_SAER_SHIFT)) & DMA_SERQ_SAER_MASK)
#define DMA_SERQ_NOP_MASK                        (0x80U)
#define DMA_SERQ_NOP_SHIFT                       (7U)
#define DMA_SERQ_NOP(x)                          (((uint8_t)(((uint8_t)(x)) << DMA_SERQ_NOP_SHIFT)) & DMA_SERQ_NOP_MASK)

/*! @name CDNE - Clear DONE Status Bit Register */
#define DMA_CDNE_CDNE_MASK                       (0xFU)
#define DMA_CDNE_CDNE_SHIFT                      (0U)
#define DMA_CDNE_CDNE(x)                         (((uint8_t)(((uint8_t)(x)) << DMA_CDNE_CDNE_SHIFT)) & DMA_CDNE_CDNE_MASK)
#define DMA_CDNE_CADN_MASK                       (0x40U)
#define DMA_CDNE_CADN_SHIFT                      (6U)
#define DMA_CDNE_CADN(x)                         (((uint8_t)(((uint8_t)(x)) << DMA_CDNE_CADN_SHIFT)) & DMA_CDNE_CADN_MASK)
#define DMA_CDNE_NOP_MASK                        (0x80U)
#define DMA_CDNE_NOP_SHIFT                       (7U)
#define DMA_CDNE_NOP(x)                          (((uint8_t)(((uint8_t)(x)) << DMA_CDNE_NOP_SHIFT)) & DMA_CDNE_NOP_MASK)

/*! @name SSRT - Set START Bit Register */
#define DMA_SSRT_SSRT_MASK                       (0xFU)
#define DMA_SSRT_SSRT_SHIFT                      (0U)
#define DMA_SSRT_SSRT(x)                         (((uint8_t)(((uint8_t)(x)) << DMA_SSRT_SSRT_SHIFT)) & DMA_SSRT_SSRT_MASK)
#define DMA_SSRT_SAST_MASK                       (0x40U)
#define DMA_SSRT_SAST_SHIFT                      (6U)
#define DMA_SSRT_SAST(x)                         (((uint8_t)(((uint8_t)(x)) << DMA_SSRT_SAST_SHIFT)) & DMA_SSRT_SAST_MASK)
#define DMA_SSRT_NOP_MASK                        (0x80U)
#define DMA_SSRT_NOP_SHIFT                       (7U)
#define DMA_SSRT_NOP(x)                          (((uint8_t)(((uint8_t)(x)) << DMA_SSRT_NOP_SHIFT)) & DMA_SSRT_NOP_MASK)

/*! @name CERR - Clear Error Register */
#define DMA_CERR_CERR_MASK                       (0xFU)
#define DMA_CERR_CERR_SHIFT                      (0U)
#define DMA_CERR_CERR(x)                         (((uint8_t)(((uint8_t)(x)) << DMA_CERR_CERR_SHIFT)) & DMA_CERR_CERR_MASK)
#define DMA_CERR_CAEI_MASK                       (0x40U)
#define DMA_CERR_CAEI_SHIFT                      (6U)
#define DMA_CERR_CAEI(x)                         (((uint8_t)(((uint8_t)(x)) << DMA_CERR_CAEI_SHIFT)) & DMA_CERR_CAEI_MASK)
#define DMA_CERR_NOP_MASK                        (0x80U)
#define DMA_CERR_NOP_SHIFT                       (7U)
#define DMA_CERR_NOP(x)                          (((uint8_t)(((uint8_t)(x)) << DMA_CERR_NOP_SHIFT)) & DMA_CERR_NOP_MASK)

/*! @name CINT - Clear Interrupt Request Register */
#define DMA_CINT_CINT_MASK                       (0xFU)
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
#define DMA_INT_INT4_MASK                        (0x10U)
#define DMA_INT_INT4_SHIFT                       (4U)
#define DMA_INT_INT4(x)                          (((uint32_t)(((uint32_t)(x)) << DMA_INT_INT4_SHIFT)) & DMA_INT_INT4_MASK)
#define DMA_INT_INT5_MASK                        (0x20U)
#define DMA_INT_INT5_SHIFT                       (5U)
#define DMA_INT_INT5(x)                          (((uint32_t)(((uint32_t)(x)) << DMA_INT_INT5_SHIFT)) & DMA_INT_INT5_MASK)
#define DMA_INT_INT6_MASK                        (0x40U)
#define DMA_INT_INT6_SHIFT                       (6U)
#define DMA_INT_INT6(x)                          (((uint32_t)(((uint32_t)(x)) << DMA_INT_INT6_SHIFT)) & DMA_INT_INT6_MASK)
#define DMA_INT_INT7_MASK                        (0x80U)
#define DMA_INT_INT7_SHIFT                       (7U)
#define DMA_INT_INT7(x)                          (((uint32_t)(((uint32_t)(x)) << DMA_INT_INT7_SHIFT)) & DMA_INT_INT7_MASK)
#define DMA_INT_INT8_MASK                        (0x100U)
#define DMA_INT_INT8_SHIFT                       (8U)
#define DMA_INT_INT8(x)                          (((uint32_t)(((uint32_t)(x)) << DMA_INT_INT8_SHIFT)) & DMA_INT_INT8_MASK)
#define DMA_INT_INT9_MASK                        (0x200U)
#define DMA_INT_INT9_SHIFT                       (9U)
#define DMA_INT_INT9(x)                          (((uint32_t)(((uint32_t)(x)) << DMA_INT_INT9_SHIFT)) & DMA_INT_INT9_MASK)
#define DMA_INT_INT10_MASK                       (0x400U)
#define DMA_INT_INT10_SHIFT                      (10U)
#define DMA_INT_INT10(x)                         (((uint32_t)(((uint32_t)(x)) << DMA_INT_INT10_SHIFT)) & DMA_INT_INT10_MASK)
#define DMA_INT_INT11_MASK                       (0x800U)
#define DMA_INT_INT11_SHIFT                      (11U)
#define DMA_INT_INT11(x)                         (((uint32_t)(((uint32_t)(x)) << DMA_INT_INT11_SHIFT)) & DMA_INT_INT11_MASK)
#define DMA_INT_INT12_MASK                       (0x1000U)
#define DMA_INT_INT12_SHIFT                      (12U)
#define DMA_INT_INT12(x)                         (((uint32_t)(((uint32_t)(x)) << DMA_INT_INT12_SHIFT)) & DMA_INT_INT12_MASK)
#define DMA_INT_INT13_MASK                       (0x2000U)
#define DMA_INT_INT13_SHIFT                      (13U)
#define DMA_INT_INT13(x)                         (((uint32_t)(((uint32_t)(x)) << DMA_INT_INT13_SHIFT)) & DMA_INT_INT13_MASK)
#define DMA_INT_INT14_MASK                       (0x4000U)
#define DMA_INT_INT14_SHIFT                      (14U)
#define DMA_INT_INT14(x)                         (((uint32_t)(((uint32_t)(x)) << DMA_INT_INT14_SHIFT)) & DMA_INT_INT14_MASK)
#define DMA_INT_INT15_MASK                       (0x8000U)
#define DMA_INT_INT15_SHIFT                      (15U)
#define DMA_INT_INT15(x)                         (((uint32_t)(((uint32_t)(x)) << DMA_INT_INT15_SHIFT)) & DMA_INT_INT15_MASK)

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
#define DMA_ERR_ERR4_MASK                        (0x10U)
#define DMA_ERR_ERR4_SHIFT                       (4U)
#define DMA_ERR_ERR4(x)                          (((uint32_t)(((uint32_t)(x)) << DMA_ERR_ERR4_SHIFT)) & DMA_ERR_ERR4_MASK)
#define DMA_ERR_ERR5_MASK                        (0x20U)
#define DMA_ERR_ERR5_SHIFT                       (5U)
#define DMA_ERR_ERR5(x)                          (((uint32_t)(((uint32_t)(x)) << DMA_ERR_ERR5_SHIFT)) & DMA_ERR_ERR5_MASK)
#define DMA_ERR_ERR6_MASK                        (0x40U)
#define DMA_ERR_ERR6_SHIFT                       (6U)
#define DMA_ERR_ERR6(x)                          (((uint32_t)(((uint32_t)(x)) << DMA_ERR_ERR6_SHIFT)) & DMA_ERR_ERR6_MASK)
#define DMA_ERR_ERR7_MASK                        (0x80U)
#define DMA_ERR_ERR7_SHIFT                       (7U)
#define DMA_ERR_ERR7(x)                          (((uint32_t)(((uint32_t)(x)) << DMA_ERR_ERR7_SHIFT)) & DMA_ERR_ERR7_MASK)
#define DMA_ERR_ERR8_MASK                        (0x100U)
#define DMA_ERR_ERR8_SHIFT                       (8U)
#define DMA_ERR_ERR8(x)                          (((uint32_t)(((uint32_t)(x)) << DMA_ERR_ERR8_SHIFT)) & DMA_ERR_ERR8_MASK)
#define DMA_ERR_ERR9_MASK                        (0x200U)
#define DMA_ERR_ERR9_SHIFT                       (9U)
#define DMA_ERR_ERR9(x)                          (((uint32_t)(((uint32_t)(x)) << DMA_ERR_ERR9_SHIFT)) & DMA_ERR_ERR9_MASK)
#define DMA_ERR_ERR10_MASK                       (0x400U)
#define DMA_ERR_ERR10_SHIFT                      (10U)
#define DMA_ERR_ERR10(x)                         (((uint32_t)(((uint32_t)(x)) << DMA_ERR_ERR10_SHIFT)) & DMA_ERR_ERR10_MASK)
#define DMA_ERR_ERR11_MASK                       (0x800U)
#define DMA_ERR_ERR11_SHIFT                      (11U)
#define DMA_ERR_ERR11(x)                         (((uint32_t)(((uint32_t)(x)) << DMA_ERR_ERR11_SHIFT)) & DMA_ERR_ERR11_MASK)
#define DMA_ERR_ERR12_MASK                       (0x1000U)
#define DMA_ERR_ERR12_SHIFT                      (12U)
#define DMA_ERR_ERR12(x)                         (((uint32_t)(((uint32_t)(x)) << DMA_ERR_ERR12_SHIFT)) & DMA_ERR_ERR12_MASK)
#define DMA_ERR_ERR13_MASK                       (0x2000U)
#define DMA_ERR_ERR13_SHIFT                      (13U)
#define DMA_ERR_ERR13(x)                         (((uint32_t)(((uint32_t)(x)) << DMA_ERR_ERR13_SHIFT)) & DMA_ERR_ERR13_MASK)
#define DMA_ERR_ERR14_MASK                       (0x4000U)
#define DMA_ERR_ERR14_SHIFT                      (14U)
#define DMA_ERR_ERR14(x)                         (((uint32_t)(((uint32_t)(x)) << DMA_ERR_ERR14_SHIFT)) & DMA_ERR_ERR14_MASK)
#define DMA_ERR_ERR15_MASK                       (0x8000U)
#define DMA_ERR_ERR15_SHIFT                      (15U)
#define DMA_ERR_ERR15(x)                         (((uint32_t)(((uint32_t)(x)) << DMA_ERR_ERR15_SHIFT)) & DMA_ERR_ERR15_MASK)

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
#define DMA_HRS_HRS4_MASK                        (0x10U)
#define DMA_HRS_HRS4_SHIFT                       (4U)
#define DMA_HRS_HRS4(x)                          (((uint32_t)(((uint32_t)(x)) << DMA_HRS_HRS4_SHIFT)) & DMA_HRS_HRS4_MASK)
#define DMA_HRS_HRS5_MASK                        (0x20U)
#define DMA_HRS_HRS5_SHIFT                       (5U)
#define DMA_HRS_HRS5(x)                          (((uint32_t)(((uint32_t)(x)) << DMA_HRS_HRS5_SHIFT)) & DMA_HRS_HRS5_MASK)
#define DMA_HRS_HRS6_MASK                        (0x40U)
#define DMA_HRS_HRS6_SHIFT                       (6U)
#define DMA_HRS_HRS6(x)                          (((uint32_t)(((uint32_t)(x)) << DMA_HRS_HRS6_SHIFT)) & DMA_HRS_HRS6_MASK)
#define DMA_HRS_HRS7_MASK                        (0x80U)
#define DMA_HRS_HRS7_SHIFT                       (7U)
#define DMA_HRS_HRS7(x)                          (((uint32_t)(((uint32_t)(x)) << DMA_HRS_HRS7_SHIFT)) & DMA_HRS_HRS7_MASK)
#define DMA_HRS_HRS8_MASK                        (0x100U)
#define DMA_HRS_HRS8_SHIFT                       (8U)
#define DMA_HRS_HRS8(x)                          (((uint32_t)(((uint32_t)(x)) << DMA_HRS_HRS8_SHIFT)) & DMA_HRS_HRS8_MASK)
#define DMA_HRS_HRS9_MASK                        (0x200U)
#define DMA_HRS_HRS9_SHIFT                       (9U)
#define DMA_HRS_HRS9(x)                          (((uint32_t)(((uint32_t)(x)) << DMA_HRS_HRS9_SHIFT)) & DMA_HRS_HRS9_MASK)
#define DMA_HRS_HRS10_MASK                       (0x400U)
#define DMA_HRS_HRS10_SHIFT                      (10U)
#define DMA_HRS_HRS10(x)                         (((uint32_t)(((uint32_t)(x)) << DMA_HRS_HRS10_SHIFT)) & DMA_HRS_HRS10_MASK)
#define DMA_HRS_HRS11_MASK                       (0x800U)
#define DMA_HRS_HRS11_SHIFT                      (11U)
#define DMA_HRS_HRS11(x)                         (((uint32_t)(((uint32_t)(x)) << DMA_HRS_HRS11_SHIFT)) & DMA_HRS_HRS11_MASK)
#define DMA_HRS_HRS12_MASK                       (0x1000U)
#define DMA_HRS_HRS12_SHIFT                      (12U)
#define DMA_HRS_HRS12(x)                         (((uint32_t)(((uint32_t)(x)) << DMA_HRS_HRS12_SHIFT)) & DMA_HRS_HRS12_MASK)
#define DMA_HRS_HRS13_MASK                       (0x2000U)
#define DMA_HRS_HRS13_SHIFT                      (13U)
#define DMA_HRS_HRS13(x)                         (((uint32_t)(((uint32_t)(x)) << DMA_HRS_HRS13_SHIFT)) & DMA_HRS_HRS13_MASK)
#define DMA_HRS_HRS14_MASK                       (0x4000U)
#define DMA_HRS_HRS14_SHIFT                      (14U)
#define DMA_HRS_HRS14(x)                         (((uint32_t)(((uint32_t)(x)) << DMA_HRS_HRS14_SHIFT)) & DMA_HRS_HRS14_MASK)
#define DMA_HRS_HRS15_MASK                       (0x8000U)
#define DMA_HRS_HRS15_SHIFT                      (15U)
#define DMA_HRS_HRS15(x)                         (((uint32_t)(((uint32_t)(x)) << DMA_HRS_HRS15_SHIFT)) & DMA_HRS_HRS15_MASK)

/*! @name DCHPRI3 - Channel n Priority Register */
#define DMA_DCHPRI3_CHPRI_MASK                   (0xFU)
#define DMA_DCHPRI3_CHPRI_SHIFT                  (0U)
#define DMA_DCHPRI3_CHPRI(x)                     (((uint8_t)(((uint8_t)(x)) << DMA_DCHPRI3_CHPRI_SHIFT)) & DMA_DCHPRI3_CHPRI_MASK)
#define DMA_DCHPRI3_DPA_MASK                     (0x40U)
#define DMA_DCHPRI3_DPA_SHIFT                    (6U)
#define DMA_DCHPRI3_DPA(x)                       (((uint8_t)(((uint8_t)(x)) << DMA_DCHPRI3_DPA_SHIFT)) & DMA_DCHPRI3_DPA_MASK)
#define DMA_DCHPRI3_ECP_MASK                     (0x80U)
#define DMA_DCHPRI3_ECP_SHIFT                    (7U)
#define DMA_DCHPRI3_ECP(x)                       (((uint8_t)(((uint8_t)(x)) << DMA_DCHPRI3_ECP_SHIFT)) & DMA_DCHPRI3_ECP_MASK)

/*! @name DCHPRI2 - Channel n Priority Register */
#define DMA_DCHPRI2_CHPRI_MASK                   (0xFU)
#define DMA_DCHPRI2_CHPRI_SHIFT                  (0U)
#define DMA_DCHPRI2_CHPRI(x)                     (((uint8_t)(((uint8_t)(x)) << DMA_DCHPRI2_CHPRI_SHIFT)) & DMA_DCHPRI2_CHPRI_MASK)
#define DMA_DCHPRI2_DPA_MASK                     (0x40U)
#define DMA_DCHPRI2_DPA_SHIFT                    (6U)
#define DMA_DCHPRI2_DPA(x)                       (((uint8_t)(((uint8_t)(x)) << DMA_DCHPRI2_DPA_SHIFT)) & DMA_DCHPRI2_DPA_MASK)
#define DMA_DCHPRI2_ECP_MASK                     (0x80U)
#define DMA_DCHPRI2_ECP_SHIFT                    (7U)
#define DMA_DCHPRI2_ECP(x)                       (((uint8_t)(((uint8_t)(x)) << DMA_DCHPRI2_ECP_SHIFT)) & DMA_DCHPRI2_ECP_MASK)

/*! @name DCHPRI1 - Channel n Priority Register */
#define DMA_DCHPRI1_CHPRI_MASK                   (0xFU)
#define DMA_DCHPRI1_CHPRI_SHIFT                  (0U)
#define DMA_DCHPRI1_CHPRI(x)                     (((uint8_t)(((uint8_t)(x)) << DMA_DCHPRI1_CHPRI_SHIFT)) & DMA_DCHPRI1_CHPRI_MASK)
#define DMA_DCHPRI1_DPA_MASK                     (0x40U)
#define DMA_DCHPRI1_DPA_SHIFT                    (6U)
#define DMA_DCHPRI1_DPA(x)                       (((uint8_t)(((uint8_t)(x)) << DMA_DCHPRI1_DPA_SHIFT)) & DMA_DCHPRI1_DPA_MASK)
#define DMA_DCHPRI1_ECP_MASK                     (0x80U)
#define DMA_DCHPRI1_ECP_SHIFT                    (7U)
#define DMA_DCHPRI1_ECP(x)                       (((uint8_t)(((uint8_t)(x)) << DMA_DCHPRI1_ECP_SHIFT)) & DMA_DCHPRI1_ECP_MASK)

/*! @name DCHPRI0 - Channel n Priority Register */
#define DMA_DCHPRI0_CHPRI_MASK                   (0xFU)
#define DMA_DCHPRI0_CHPRI_SHIFT                  (0U)
#define DMA_DCHPRI0_CHPRI(x)                     (((uint8_t)(((uint8_t)(x)) << DMA_DCHPRI0_CHPRI_SHIFT)) & DMA_DCHPRI0_CHPRI_MASK)
#define DMA_DCHPRI0_DPA_MASK                     (0x40U)
#define DMA_DCHPRI0_DPA_SHIFT                    (6U)
#define DMA_DCHPRI0_DPA(x)                       (((uint8_t)(((uint8_t)(x)) << DMA_DCHPRI0_DPA_SHIFT)) & DMA_DCHPRI0_DPA_MASK)
#define DMA_DCHPRI0_ECP_MASK                     (0x80U)
#define DMA_DCHPRI0_ECP_SHIFT                    (7U)
#define DMA_DCHPRI0_ECP(x)                       (((uint8_t)(((uint8_t)(x)) << DMA_DCHPRI0_ECP_SHIFT)) & DMA_DCHPRI0_ECP_MASK)

/*! @name DCHPRI7 - Channel n Priority Register */
#define DMA_DCHPRI7_CHPRI_MASK                   (0xFU)
#define DMA_DCHPRI7_CHPRI_SHIFT                  (0U)
#define DMA_DCHPRI7_CHPRI(x)                     (((uint8_t)(((uint8_t)(x)) << DMA_DCHPRI7_CHPRI_SHIFT)) & DMA_DCHPRI7_CHPRI_MASK)
#define DMA_DCHPRI7_DPA_MASK                     (0x40U)
#define DMA_DCHPRI7_DPA_SHIFT                    (6U)
#define DMA_DCHPRI7_DPA(x)                       (((uint8_t)(((uint8_t)(x)) << DMA_DCHPRI7_DPA_SHIFT)) & DMA_DCHPRI7_DPA_MASK)
#define DMA_DCHPRI7_ECP_MASK                     (0x80U)
#define DMA_DCHPRI7_ECP_SHIFT                    (7U)
#define DMA_DCHPRI7_ECP(x)                       (((uint8_t)(((uint8_t)(x)) << DMA_DCHPRI7_ECP_SHIFT)) & DMA_DCHPRI7_ECP_MASK)

/*! @name DCHPRI6 - Channel n Priority Register */
#define DMA_DCHPRI6_CHPRI_MASK                   (0xFU)
#define DMA_DCHPRI6_CHPRI_SHIFT                  (0U)
#define DMA_DCHPRI6_CHPRI(x)                     (((uint8_t)(((uint8_t)(x)) << DMA_DCHPRI6_CHPRI_SHIFT)) & DMA_DCHPRI6_CHPRI_MASK)
#define DMA_DCHPRI6_DPA_MASK                     (0x40U)
#define DMA_DCHPRI6_DPA_SHIFT                    (6U)
#define DMA_DCHPRI6_DPA(x)                       (((uint8_t)(((uint8_t)(x)) << DMA_DCHPRI6_DPA_SHIFT)) & DMA_DCHPRI6_DPA_MASK)
#define DMA_DCHPRI6_ECP_MASK                     (0x80U)
#define DMA_DCHPRI6_ECP_SHIFT                    (7U)
#define DMA_DCHPRI6_ECP(x)                       (((uint8_t)(((uint8_t)(x)) << DMA_DCHPRI6_ECP_SHIFT)) & DMA_DCHPRI6_ECP_MASK)

/*! @name DCHPRI5 - Channel n Priority Register */
#define DMA_DCHPRI5_CHPRI_MASK                   (0xFU)
#define DMA_DCHPRI5_CHPRI_SHIFT                  (0U)
#define DMA_DCHPRI5_CHPRI(x)                     (((uint8_t)(((uint8_t)(x)) << DMA_DCHPRI5_CHPRI_SHIFT)) & DMA_DCHPRI5_CHPRI_MASK)
#define DMA_DCHPRI5_DPA_MASK                     (0x40U)
#define DMA_DCHPRI5_DPA_SHIFT                    (6U)
#define DMA_DCHPRI5_DPA(x)                       (((uint8_t)(((uint8_t)(x)) << DMA_DCHPRI5_DPA_SHIFT)) & DMA_DCHPRI5_DPA_MASK)
#define DMA_DCHPRI5_ECP_MASK                     (0x80U)
#define DMA_DCHPRI5_ECP_SHIFT                    (7U)
#define DMA_DCHPRI5_ECP(x)                       (((uint8_t)(((uint8_t)(x)) << DMA_DCHPRI5_ECP_SHIFT)) & DMA_DCHPRI5_ECP_MASK)

/*! @name DCHPRI4 - Channel n Priority Register */
#define DMA_DCHPRI4_CHPRI_MASK                   (0xFU)
#define DMA_DCHPRI4_CHPRI_SHIFT                  (0U)
#define DMA_DCHPRI4_CHPRI(x)                     (((uint8_t)(((uint8_t)(x)) << DMA_DCHPRI4_CHPRI_SHIFT)) & DMA_DCHPRI4_CHPRI_MASK)
#define DMA_DCHPRI4_DPA_MASK                     (0x40U)
#define DMA_DCHPRI4_DPA_SHIFT                    (6U)
#define DMA_DCHPRI4_DPA(x)                       (((uint8_t)(((uint8_t)(x)) << DMA_DCHPRI4_DPA_SHIFT)) & DMA_DCHPRI4_DPA_MASK)
#define DMA_DCHPRI4_ECP_MASK                     (0x80U)
#define DMA_DCHPRI4_ECP_SHIFT                    (7U)
#define DMA_DCHPRI4_ECP(x)                       (((uint8_t)(((uint8_t)(x)) << DMA_DCHPRI4_ECP_SHIFT)) & DMA_DCHPRI4_ECP_MASK)

/*! @name DCHPRI11 - Channel n Priority Register */
#define DMA_DCHPRI11_CHPRI_MASK                  (0xFU)
#define DMA_DCHPRI11_CHPRI_SHIFT                 (0U)
#define DMA_DCHPRI11_CHPRI(x)                    (((uint8_t)(((uint8_t)(x)) << DMA_DCHPRI11_CHPRI_SHIFT)) & DMA_DCHPRI11_CHPRI_MASK)
#define DMA_DCHPRI11_DPA_MASK                    (0x40U)
#define DMA_DCHPRI11_DPA_SHIFT                   (6U)
#define DMA_DCHPRI11_DPA(x)                      (((uint8_t)(((uint8_t)(x)) << DMA_DCHPRI11_DPA_SHIFT)) & DMA_DCHPRI11_DPA_MASK)
#define DMA_DCHPRI11_ECP_MASK                    (0x80U)
#define DMA_DCHPRI11_ECP_SHIFT                   (7U)
#define DMA_DCHPRI11_ECP(x)                      (((uint8_t)(((uint8_t)(x)) << DMA_DCHPRI11_ECP_SHIFT)) & DMA_DCHPRI11_ECP_MASK)

/*! @name DCHPRI10 - Channel n Priority Register */
#define DMA_DCHPRI10_CHPRI_MASK                  (0xFU)
#define DMA_DCHPRI10_CHPRI_SHIFT                 (0U)
#define DMA_DCHPRI10_CHPRI(x)                    (((uint8_t)(((uint8_t)(x)) << DMA_DCHPRI10_CHPRI_SHIFT)) & DMA_DCHPRI10_CHPRI_MASK)
#define DMA_DCHPRI10_DPA_MASK                    (0x40U)
#define DMA_DCHPRI10_DPA_SHIFT                   (6U)
#define DMA_DCHPRI10_DPA(x)                      (((uint8_t)(((uint8_t)(x)) << DMA_DCHPRI10_DPA_SHIFT)) & DMA_DCHPRI10_DPA_MASK)
#define DMA_DCHPRI10_ECP_MASK                    (0x80U)
#define DMA_DCHPRI10_ECP_SHIFT                   (7U)
#define DMA_DCHPRI10_ECP(x)                      (((uint8_t)(((uint8_t)(x)) << DMA_DCHPRI10_ECP_SHIFT)) & DMA_DCHPRI10_ECP_MASK)

/*! @name DCHPRI9 - Channel n Priority Register */
#define DMA_DCHPRI9_CHPRI_MASK                   (0xFU)
#define DMA_DCHPRI9_CHPRI_SHIFT                  (0U)
#define DMA_DCHPRI9_CHPRI(x)                     (((uint8_t)(((uint8_t)(x)) << DMA_DCHPRI9_CHPRI_SHIFT)) & DMA_DCHPRI9_CHPRI_MASK)
#define DMA_DCHPRI9_DPA_MASK                     (0x40U)
#define DMA_DCHPRI9_DPA_SHIFT                    (6U)
#define DMA_DCHPRI9_DPA(x)                       (((uint8_t)(((uint8_t)(x)) << DMA_DCHPRI9_DPA_SHIFT)) & DMA_DCHPRI9_DPA_MASK)
#define DMA_DCHPRI9_ECP_MASK                     (0x80U)
#define DMA_DCHPRI9_ECP_SHIFT                    (7U)
#define DMA_DCHPRI9_ECP(x)                       (((uint8_t)(((uint8_t)(x)) << DMA_DCHPRI9_ECP_SHIFT)) & DMA_DCHPRI9_ECP_MASK)

/*! @name DCHPRI8 - Channel n Priority Register */
#define DMA_DCHPRI8_CHPRI_MASK                   (0xFU)
#define DMA_DCHPRI8_CHPRI_SHIFT                  (0U)
#define DMA_DCHPRI8_CHPRI(x)                     (((uint8_t)(((uint8_t)(x)) << DMA_DCHPRI8_CHPRI_SHIFT)) & DMA_DCHPRI8_CHPRI_MASK)
#define DMA_DCHPRI8_DPA_MASK                     (0x40U)
#define DMA_DCHPRI8_DPA_SHIFT                    (6U)
#define DMA_DCHPRI8_DPA(x)                       (((uint8_t)(((uint8_t)(x)) << DMA_DCHPRI8_DPA_SHIFT)) & DMA_DCHPRI8_DPA_MASK)
#define DMA_DCHPRI8_ECP_MASK                     (0x80U)
#define DMA_DCHPRI8_ECP_SHIFT                    (7U)
#define DMA_DCHPRI8_ECP(x)                       (((uint8_t)(((uint8_t)(x)) << DMA_DCHPRI8_ECP_SHIFT)) & DMA_DCHPRI8_ECP_MASK)

/*! @name DCHPRI15 - Channel n Priority Register */
#define DMA_DCHPRI15_CHPRI_MASK                  (0xFU)
#define DMA_DCHPRI15_CHPRI_SHIFT                 (0U)
#define DMA_DCHPRI15_CHPRI(x)                    (((uint8_t)(((uint8_t)(x)) << DMA_DCHPRI15_CHPRI_SHIFT)) & DMA_DCHPRI15_CHPRI_MASK)
#define DMA_DCHPRI15_DPA_MASK                    (0x40U)
#define DMA_DCHPRI15_DPA_SHIFT                   (6U)
#define DMA_DCHPRI15_DPA(x)                      (((uint8_t)(((uint8_t)(x)) << DMA_DCHPRI15_DPA_SHIFT)) & DMA_DCHPRI15_DPA_MASK)
#define DMA_DCHPRI15_ECP_MASK                    (0x80U)
#define DMA_DCHPRI15_ECP_SHIFT                   (7U)
#define DMA_DCHPRI15_ECP(x)                      (((uint8_t)(((uint8_t)(x)) << DMA_DCHPRI15_ECP_SHIFT)) & DMA_DCHPRI15_ECP_MASK)

/*! @name DCHPRI14 - Channel n Priority Register */
#define DMA_DCHPRI14_CHPRI_MASK                  (0xFU)
#define DMA_DCHPRI14_CHPRI_SHIFT                 (0U)
#define DMA_DCHPRI14_CHPRI(x)                    (((uint8_t)(((uint8_t)(x)) << DMA_DCHPRI14_CHPRI_SHIFT)) & DMA_DCHPRI14_CHPRI_MASK)
#define DMA_DCHPRI14_DPA_MASK                    (0x40U)
#define DMA_DCHPRI14_DPA_SHIFT                   (6U)
#define DMA_DCHPRI14_DPA(x)                      (((uint8_t)(((uint8_t)(x)) << DMA_DCHPRI14_DPA_SHIFT)) & DMA_DCHPRI14_DPA_MASK)
#define DMA_DCHPRI14_ECP_MASK                    (0x80U)
#define DMA_DCHPRI14_ECP_SHIFT                   (7U)
#define DMA_DCHPRI14_ECP(x)                      (((uint8_t)(((uint8_t)(x)) << DMA_DCHPRI14_ECP_SHIFT)) & DMA_DCHPRI14_ECP_MASK)

/*! @name DCHPRI13 - Channel n Priority Register */
#define DMA_DCHPRI13_CHPRI_MASK                  (0xFU)
#define DMA_DCHPRI13_CHPRI_SHIFT                 (0U)
#define DMA_DCHPRI13_CHPRI(x)                    (((uint8_t)(((uint8_t)(x)) << DMA_DCHPRI13_CHPRI_SHIFT)) & DMA_DCHPRI13_CHPRI_MASK)
#define DMA_DCHPRI13_DPA_MASK                    (0x40U)
#define DMA_DCHPRI13_DPA_SHIFT                   (6U)
#define DMA_DCHPRI13_DPA(x)                      (((uint8_t)(((uint8_t)(x)) << DMA_DCHPRI13_DPA_SHIFT)) & DMA_DCHPRI13_DPA_MASK)
#define DMA_DCHPRI13_ECP_MASK                    (0x80U)
#define DMA_DCHPRI13_ECP_SHIFT                   (7U)
#define DMA_DCHPRI13_ECP(x)                      (((uint8_t)(((uint8_t)(x)) << DMA_DCHPRI13_ECP_SHIFT)) & DMA_DCHPRI13_ECP_MASK)

/*! @name DCHPRI12 - Channel n Priority Register */
#define DMA_DCHPRI12_CHPRI_MASK                  (0xFU)
#define DMA_DCHPRI12_CHPRI_SHIFT                 (0U)
#define DMA_DCHPRI12_CHPRI(x)                    (((uint8_t)(((uint8_t)(x)) << DMA_DCHPRI12_CHPRI_SHIFT)) & DMA_DCHPRI12_CHPRI_MASK)
#define DMA_DCHPRI12_DPA_MASK                    (0x40U)
#define DMA_DCHPRI12_DPA_SHIFT                   (6U)
#define DMA_DCHPRI12_DPA(x)                      (((uint8_t)(((uint8_t)(x)) << DMA_DCHPRI12_DPA_SHIFT)) & DMA_DCHPRI12_DPA_MASK)
#define DMA_DCHPRI12_ECP_MASK                    (0x80U)
#define DMA_DCHPRI12_ECP_SHIFT                   (7U)
#define DMA_DCHPRI12_ECP(x)                      (((uint8_t)(((uint8_t)(x)) << DMA_DCHPRI12_ECP_SHIFT)) & DMA_DCHPRI12_ECP_MASK)

/*! @name SADDR - TCD Source Address */
#define DMA_SADDR_SADDR_MASK                     (0xFFFFFFFFU)
#define DMA_SADDR_SADDR_SHIFT                    (0U)
#define DMA_SADDR_SADDR(x)                       (((uint32_t)(((uint32_t)(x)) << DMA_SADDR_SADDR_SHIFT)) & DMA_SADDR_SADDR_MASK)

/* The count of DMA_SADDR */
#define DMA_SADDR_COUNT                          (16U)

/*! @name SOFF - TCD Signed Source Address Offset */
#define DMA_SOFF_SOFF_MASK                       (0xFFFFU)
#define DMA_SOFF_SOFF_SHIFT                      (0U)
#define DMA_SOFF_SOFF(x)                         (((uint16_t)(((uint16_t)(x)) << DMA_SOFF_SOFF_SHIFT)) & DMA_SOFF_SOFF_MASK)

/* The count of DMA_SOFF */
#define DMA_SOFF_COUNT                           (16U)

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
#define DMA_ATTR_COUNT                           (16U)

/*! @name NBYTES_MLNO - TCD Minor Byte Count (Minor Loop Disabled) */
#define DMA_NBYTES_MLNO_NBYTES_MASK              (0xFFFFFFFFU)
#define DMA_NBYTES_MLNO_NBYTES_SHIFT             (0U)
#define DMA_NBYTES_MLNO_NBYTES(x)                (((uint32_t)(((uint32_t)(x)) << DMA_NBYTES_MLNO_NBYTES_SHIFT)) & DMA_NBYTES_MLNO_NBYTES_MASK)

/* The count of DMA_NBYTES_MLNO */
#define DMA_NBYTES_MLNO_COUNT                    (16U)

/*! @name NBYTES_MLOFFNO - TCD Signed Minor Loop Offset (Minor Loop Enabled and Offset Disabled) */
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
#define DMA_NBYTES_MLOFFNO_COUNT                 (16U)

/*! @name NBYTES_MLOFFYES - TCD Signed Minor Loop Offset (Minor Loop and Offset Enabled) */
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
#define DMA_NBYTES_MLOFFYES_COUNT                (16U)

/*! @name SLAST - TCD Last Source Address Adjustment */
#define DMA_SLAST_SLAST_MASK                     (0xFFFFFFFFU)
#define DMA_SLAST_SLAST_SHIFT                    (0U)
#define DMA_SLAST_SLAST(x)                       (((uint32_t)(((uint32_t)(x)) << DMA_SLAST_SLAST_SHIFT)) & DMA_SLAST_SLAST_MASK)

/* The count of DMA_SLAST */
#define DMA_SLAST_COUNT                          (16U)

/*! @name DADDR - TCD Destination Address */
#define DMA_DADDR_DADDR_MASK                     (0xFFFFFFFFU)
#define DMA_DADDR_DADDR_SHIFT                    (0U)
#define DMA_DADDR_DADDR(x)                       (((uint32_t)(((uint32_t)(x)) << DMA_DADDR_DADDR_SHIFT)) & DMA_DADDR_DADDR_MASK)

/* The count of DMA_DADDR */
#define DMA_DADDR_COUNT                          (16U)

/*! @name DOFF - TCD Signed Destination Address Offset */
#define DMA_DOFF_DOFF_MASK                       (0xFFFFU)
#define DMA_DOFF_DOFF_SHIFT                      (0U)
#define DMA_DOFF_DOFF(x)                         (((uint16_t)(((uint16_t)(x)) << DMA_DOFF_DOFF_SHIFT)) & DMA_DOFF_DOFF_MASK)

/* The count of DMA_DOFF */
#define DMA_DOFF_COUNT                           (16U)

/*! @name CITER_ELINKNO - TCD Current Minor Loop Link, Major Loop Count (Channel Linking Disabled) */
#define DMA_CITER_ELINKNO_CITER_MASK             (0x7FFFU)
#define DMA_CITER_ELINKNO_CITER_SHIFT            (0U)
#define DMA_CITER_ELINKNO_CITER(x)               (((uint16_t)(((uint16_t)(x)) << DMA_CITER_ELINKNO_CITER_SHIFT)) & DMA_CITER_ELINKNO_CITER_MASK)
#define DMA_CITER_ELINKNO_ELINK_MASK             (0x8000U)
#define DMA_CITER_ELINKNO_ELINK_SHIFT            (15U)
#define DMA_CITER_ELINKNO_ELINK(x)               (((uint16_t)(((uint16_t)(x)) << DMA_CITER_ELINKNO_ELINK_SHIFT)) & DMA_CITER_ELINKNO_ELINK_MASK)

/* The count of DMA_CITER_ELINKNO */
#define DMA_CITER_ELINKNO_COUNT                  (16U)

/*! @name CITER_ELINKYES - TCD Current Minor Loop Link, Major Loop Count (Channel Linking Enabled) */
#define DMA_CITER_ELINKYES_CITER_MASK            (0x1FFU)
#define DMA_CITER_ELINKYES_CITER_SHIFT           (0U)
#define DMA_CITER_ELINKYES_CITER(x)              (((uint16_t)(((uint16_t)(x)) << DMA_CITER_ELINKYES_CITER_SHIFT)) & DMA_CITER_ELINKYES_CITER_MASK)
#define DMA_CITER_ELINKYES_LINKCH_MASK           (0x1E00U)
#define DMA_CITER_ELINKYES_LINKCH_SHIFT          (9U)
#define DMA_CITER_ELINKYES_LINKCH(x)             (((uint16_t)(((uint16_t)(x)) << DMA_CITER_ELINKYES_LINKCH_SHIFT)) & DMA_CITER_ELINKYES_LINKCH_MASK)
#define DMA_CITER_ELINKYES_ELINK_MASK            (0x8000U)
#define DMA_CITER_ELINKYES_ELINK_SHIFT           (15U)
#define DMA_CITER_ELINKYES_ELINK(x)              (((uint16_t)(((uint16_t)(x)) << DMA_CITER_ELINKYES_ELINK_SHIFT)) & DMA_CITER_ELINKYES_ELINK_MASK)

/* The count of DMA_CITER_ELINKYES */
#define DMA_CITER_ELINKYES_COUNT                 (16U)

/*! @name DLAST_SGA - TCD Last Destination Address Adjustment/Scatter Gather Address */
#define DMA_DLAST_SGA_DLASTSGA_MASK              (0xFFFFFFFFU)
#define DMA_DLAST_SGA_DLASTSGA_SHIFT             (0U)
#define DMA_DLAST_SGA_DLASTSGA(x)                (((uint32_t)(((uint32_t)(x)) << DMA_DLAST_SGA_DLASTSGA_SHIFT)) & DMA_DLAST_SGA_DLASTSGA_MASK)

/* The count of DMA_DLAST_SGA */
#define DMA_DLAST_SGA_COUNT                      (16U)

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
#define DMA_CSR_MAJORLINKCH_MASK                 (0xF00U)
#define DMA_CSR_MAJORLINKCH_SHIFT                (8U)
#define DMA_CSR_MAJORLINKCH(x)                   (((uint16_t)(((uint16_t)(x)) << DMA_CSR_MAJORLINKCH_SHIFT)) & DMA_CSR_MAJORLINKCH_MASK)
#define DMA_CSR_BWC_MASK                         (0xC000U)
#define DMA_CSR_BWC_SHIFT                        (14U)
#define DMA_CSR_BWC(x)                           (((uint16_t)(((uint16_t)(x)) << DMA_CSR_BWC_SHIFT)) & DMA_CSR_BWC_MASK)

/* The count of DMA_CSR */
#define DMA_CSR_COUNT                            (16U)

/*! @name BITER_ELINKNO - TCD Beginning Minor Loop Link, Major Loop Count (Channel Linking Disabled) */
#define DMA_BITER_ELINKNO_BITER_MASK             (0x7FFFU)
#define DMA_BITER_ELINKNO_BITER_SHIFT            (0U)
#define DMA_BITER_ELINKNO_BITER(x)               (((uint16_t)(((uint16_t)(x)) << DMA_BITER_ELINKNO_BITER_SHIFT)) & DMA_BITER_ELINKNO_BITER_MASK)
#define DMA_BITER_ELINKNO_ELINK_MASK             (0x8000U)
#define DMA_BITER_ELINKNO_ELINK_SHIFT            (15U)
#define DMA_BITER_ELINKNO_ELINK(x)               (((uint16_t)(((uint16_t)(x)) << DMA_BITER_ELINKNO_ELINK_SHIFT)) & DMA_BITER_ELINKNO_ELINK_MASK)

/* The count of DMA_BITER_ELINKNO */
#define DMA_BITER_ELINKNO_COUNT                  (16U)

/*! @name BITER_ELINKYES - TCD Beginning Minor Loop Link, Major Loop Count (Channel Linking Enabled) */
#define DMA_BITER_ELINKYES_BITER_MASK            (0x1FFU)
#define DMA_BITER_ELINKYES_BITER_SHIFT           (0U)
#define DMA_BITER_ELINKYES_BITER(x)              (((uint16_t)(((uint16_t)(x)) << DMA_BITER_ELINKYES_BITER_SHIFT)) & DMA_BITER_ELINKYES_BITER_MASK)
#define DMA_BITER_ELINKYES_LINKCH_MASK           (0x1E00U)
#define DMA_BITER_ELINKYES_LINKCH_SHIFT          (9U)
#define DMA_BITER_ELINKYES_LINKCH(x)             (((uint16_t)(((uint16_t)(x)) << DMA_BITER_ELINKYES_LINKCH_SHIFT)) & DMA_BITER_ELINKYES_LINKCH_MASK)
#define DMA_BITER_ELINKYES_ELINK_MASK            (0x8000U)
#define DMA_BITER_ELINKYES_ELINK_SHIFT           (15U)
#define DMA_BITER_ELINKYES_ELINK(x)              (((uint16_t)(((uint16_t)(x)) << DMA_BITER_ELINKYES_ELINK_SHIFT)) & DMA_BITER_ELINKYES_ELINK_MASK)

/* The count of DMA_BITER_ELINKYES */
#define DMA_BITER_ELINKYES_COUNT                 (16U)


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
#define DMA_CHN_IRQS                             { { DMA0_IRQn, DMA1_IRQn, DMA2_IRQn, DMA3_IRQn, DMA4_IRQn, DMA5_IRQn, DMA6_IRQn, DMA7_IRQn, DMA8_IRQn, DMA9_IRQn, DMA10_IRQn, DMA11_IRQn, DMA12_IRQn, DMA13_IRQn, DMA14_IRQn, DMA15_IRQn } }
#define DMA_ERROR_IRQS                           { DMA_Error_IRQn }

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
  __IO uint8_t CHCFG[16];                          /**< Channel Configuration register, array offset: 0x0, array step: 0x1 */
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
#define DMAMUX_CHCFG_COUNT                       (16U)


/*!
 * @}
 */ /* end of group DMAMUX_Register_Masks */


/* DMAMUX - Peripheral instance base addresses */
/** Peripheral DMAMUX base address */
#define DMAMUX_BASE                              (0x40021000u)
/** Peripheral DMAMUX base pointer */
#define DMAMUX                                   ((DMAMUX_Type *)DMAMUX_BASE)
/** Array initializer of DMAMUX peripheral base addresses */
#define DMAMUX_BASE_ADDRS                        { DMAMUX_BASE }
/** Array initializer of DMAMUX peripheral base pointers */
#define DMAMUX_BASE_PTRS                         { DMAMUX }

/*!
 * @}
 */ /* end of group DMAMUX_Peripheral_Access_Layer */


/* ----------------------------------------------------------------------------
   -- EWM Peripheral Access Layer
   ---------------------------------------------------------------------------- */

/*!
 * @addtogroup EWM_Peripheral_Access_Layer EWM Peripheral Access Layer
 * @{
 */

/** EWM - Register Layout Typedef */
typedef struct {
  __IO uint8_t CTRL;                               /**< Control Register, offset: 0x0 */
  __O  uint8_t SERV;                               /**< Service Register, offset: 0x1 */
  __IO uint8_t CMPL;                               /**< Compare Low Register, offset: 0x2 */
  __IO uint8_t CMPH;                               /**< Compare High Register, offset: 0x3 */
} EWM_Type;

/* ----------------------------------------------------------------------------
   -- EWM Register Masks
   ---------------------------------------------------------------------------- */

/*!
 * @addtogroup EWM_Register_Masks EWM Register Masks
 * @{
 */

/*! @name CTRL - Control Register */
#define EWM_CTRL_EWMEN_MASK                      (0x1U)
#define EWM_CTRL_EWMEN_SHIFT                     (0U)
#define EWM_CTRL_EWMEN(x)                        (((uint8_t)(((uint8_t)(x)) << EWM_CTRL_EWMEN_SHIFT)) & EWM_CTRL_EWMEN_MASK)
#define EWM_CTRL_ASSIN_MASK                      (0x2U)
#define EWM_CTRL_ASSIN_SHIFT                     (1U)
#define EWM_CTRL_ASSIN(x)                        (((uint8_t)(((uint8_t)(x)) << EWM_CTRL_ASSIN_SHIFT)) & EWM_CTRL_ASSIN_MASK)
#define EWM_CTRL_INEN_MASK                       (0x4U)
#define EWM_CTRL_INEN_SHIFT                      (2U)
#define EWM_CTRL_INEN(x)                         (((uint8_t)(((uint8_t)(x)) << EWM_CTRL_INEN_SHIFT)) & EWM_CTRL_INEN_MASK)
#define EWM_CTRL_INTEN_MASK                      (0x8U)
#define EWM_CTRL_INTEN_SHIFT                     (3U)
#define EWM_CTRL_INTEN(x)                        (((uint8_t)(((uint8_t)(x)) << EWM_CTRL_INTEN_SHIFT)) & EWM_CTRL_INTEN_MASK)

/*! @name SERV - Service Register */
#define EWM_SERV_SERVICE_MASK                    (0xFFU)
#define EWM_SERV_SERVICE_SHIFT                   (0U)
#define EWM_SERV_SERVICE(x)                      (((uint8_t)(((uint8_t)(x)) << EWM_SERV_SERVICE_SHIFT)) & EWM_SERV_SERVICE_MASK)

/*! @name CMPL - Compare Low Register */
#define EWM_CMPL_COMPAREL_MASK                   (0xFFU)
#define EWM_CMPL_COMPAREL_SHIFT                  (0U)
#define EWM_CMPL_COMPAREL(x)                     (((uint8_t)(((uint8_t)(x)) << EWM_CMPL_COMPAREL_SHIFT)) & EWM_CMPL_COMPAREL_MASK)

/*! @name CMPH - Compare High Register */
#define EWM_CMPH_COMPAREH_MASK                   (0xFFU)
#define EWM_CMPH_COMPAREH_SHIFT                  (0U)
#define EWM_CMPH_COMPAREH(x)                     (((uint8_t)(((uint8_t)(x)) << EWM_CMPH_COMPAREH_SHIFT)) & EWM_CMPH_COMPAREH_MASK)


/*!
 * @}
 */ /* end of group EWM_Register_Masks */


/* EWM - Peripheral instance base addresses */
/** Peripheral EWM base address */
#define EWM_BASE                                 (0x40061000u)
/** Peripheral EWM base pointer */
#define EWM                                      ((EWM_Type *)EWM_BASE)
/** Array initializer of EWM peripheral base addresses */
#define EWM_BASE_ADDRS                           { EWM_BASE }
/** Array initializer of EWM peripheral base pointers */
#define EWM_BASE_PTRS                            { EWM }
/** Interrupt vectors for the EWM peripheral type */
#define EWM_IRQS                                 { WDOG_EWM_IRQn }

/*!
 * @}
 */ /* end of group EWM_Peripheral_Access_Layer */


/* ----------------------------------------------------------------------------
   -- FMC Peripheral Access Layer
   ---------------------------------------------------------------------------- */

/*!
 * @addtogroup FMC_Peripheral_Access_Layer FMC Peripheral Access Layer
 * @{
 */

/** FMC - Register Layout Typedef */
typedef struct {
  __IO uint32_t PFAPR;                             /**< Flash Access Protection Register, offset: 0x0 */
  __IO uint32_t PFB0CR;                            /**< Flash Bank 0 Control Register, offset: 0x4 */
  __IO uint32_t PFB1CR;                            /**< Flash Bank 1 Control Register, offset: 0x8 */
       uint8_t RESERVED_0[244];
  __IO uint32_t TAGVD[4][2];                       /**< Cache Tag Storage, array offset: 0x100, array step: index*0x8, index2*0x4 */
       uint8_t RESERVED_1[224];
  struct {                                         /* offset: 0x200, array step: index*0x10, index2*0x8 */
    __IO uint32_t DATA_U;                            /**< Cache Data Storage (upper word), array offset: 0x200, array step: index*0x10, index2*0x8 */
    __IO uint32_t DATA_L;                            /**< Cache Data Storage (lower word), array offset: 0x204, array step: index*0x10, index2*0x8 */
  } SET[4][2];
} FMC_Type;

/* ----------------------------------------------------------------------------
   -- FMC Register Masks
   ---------------------------------------------------------------------------- */

/*!
 * @addtogroup FMC_Register_Masks FMC Register Masks
 * @{
 */

/*! @name PFAPR - Flash Access Protection Register */
#define FMC_PFAPR_M0AP_MASK                      (0x3U)
#define FMC_PFAPR_M0AP_SHIFT                     (0U)
#define FMC_PFAPR_M0AP(x)                        (((uint32_t)(((uint32_t)(x)) << FMC_PFAPR_M0AP_SHIFT)) & FMC_PFAPR_M0AP_MASK)
#define FMC_PFAPR_M1AP_MASK                      (0xCU)
#define FMC_PFAPR_M1AP_SHIFT                     (2U)
#define FMC_PFAPR_M1AP(x)                        (((uint32_t)(((uint32_t)(x)) << FMC_PFAPR_M1AP_SHIFT)) & FMC_PFAPR_M1AP_MASK)
#define FMC_PFAPR_M2AP_MASK                      (0x30U)
#define FMC_PFAPR_M2AP_SHIFT                     (4U)
#define FMC_PFAPR_M2AP(x)                        (((uint32_t)(((uint32_t)(x)) << FMC_PFAPR_M2AP_SHIFT)) & FMC_PFAPR_M2AP_MASK)
#define FMC_PFAPR_M3AP_MASK                      (0xC0U)
#define FMC_PFAPR_M3AP_SHIFT                     (6U)
#define FMC_PFAPR_M3AP(x)                        (((uint32_t)(((uint32_t)(x)) << FMC_PFAPR_M3AP_SHIFT)) & FMC_PFAPR_M3AP_MASK)
#define FMC_PFAPR_M4AP_MASK                      (0x300U)
#define FMC_PFAPR_M4AP_SHIFT                     (8U)
#define FMC_PFAPR_M4AP(x)                        (((uint32_t)(((uint32_t)(x)) << FMC_PFAPR_M4AP_SHIFT)) & FMC_PFAPR_M4AP_MASK)
#define FMC_PFAPR_M5AP_MASK                      (0xC00U)
#define FMC_PFAPR_M5AP_SHIFT                     (10U)
#define FMC_PFAPR_M5AP(x)                        (((uint32_t)(((uint32_t)(x)) << FMC_PFAPR_M5AP_SHIFT)) & FMC_PFAPR_M5AP_MASK)
#define FMC_PFAPR_M6AP_MASK                      (0x3000U)
#define FMC_PFAPR_M6AP_SHIFT                     (12U)
#define FMC_PFAPR_M6AP(x)                        (((uint32_t)(((uint32_t)(x)) << FMC_PFAPR_M6AP_SHIFT)) & FMC_PFAPR_M6AP_MASK)
#define FMC_PFAPR_M7AP_MASK                      (0xC000U)
#define FMC_PFAPR_M7AP_SHIFT                     (14U)
#define FMC_PFAPR_M7AP(x)                        (((uint32_t)(((uint32_t)(x)) << FMC_PFAPR_M7AP_SHIFT)) & FMC_PFAPR_M7AP_MASK)
#define FMC_PFAPR_M0PFD_MASK                     (0x10000U)
#define FMC_PFAPR_M0PFD_SHIFT                    (16U)
#define FMC_PFAPR_M0PFD(x)                       (((uint32_t)(((uint32_t)(x)) << FMC_PFAPR_M0PFD_SHIFT)) & FMC_PFAPR_M0PFD_MASK)
#define FMC_PFAPR_M1PFD_MASK                     (0x20000U)
#define FMC_PFAPR_M1PFD_SHIFT                    (17U)
#define FMC_PFAPR_M1PFD(x)                       (((uint32_t)(((uint32_t)(x)) << FMC_PFAPR_M1PFD_SHIFT)) & FMC_PFAPR_M1PFD_MASK)
#define FMC_PFAPR_M2PFD_MASK                     (0x40000U)
#define FMC_PFAPR_M2PFD_SHIFT                    (18U)
#define FMC_PFAPR_M2PFD(x)                       (((uint32_t)(((uint32_t)(x)) << FMC_PFAPR_M2PFD_SHIFT)) & FMC_PFAPR_M2PFD_MASK)
#define FMC_PFAPR_M3PFD_MASK                     (0x80000U)
#define FMC_PFAPR_M3PFD_SHIFT                    (19U)
#define FMC_PFAPR_M3PFD(x)                       (((uint32_t)(((uint32_t)(x)) << FMC_PFAPR_M3PFD_SHIFT)) & FMC_PFAPR_M3PFD_MASK)
#define FMC_PFAPR_M4PFD_MASK                     (0x100000U)
#define FMC_PFAPR_M4PFD_SHIFT                    (20U)
#define FMC_PFAPR_M4PFD(x)                       (((uint32_t)(((uint32_t)(x)) << FMC_PFAPR_M4PFD_SHIFT)) & FMC_PFAPR_M4PFD_MASK)
#define FMC_PFAPR_M5PFD_MASK                     (0x200000U)
#define FMC_PFAPR_M5PFD_SHIFT                    (21U)
#define FMC_PFAPR_M5PFD(x)                       (((uint32_t)(((uint32_t)(x)) << FMC_PFAPR_M5PFD_SHIFT)) & FMC_PFAPR_M5PFD_MASK)
#define FMC_PFAPR_M6PFD_MASK                     (0x400000U)
#define FMC_PFAPR_M6PFD_SHIFT                    (22U)
#define FMC_PFAPR_M6PFD(x)                       (((uint32_t)(((uint32_t)(x)) << FMC_PFAPR_M6PFD_SHIFT)) & FMC_PFAPR_M6PFD_MASK)
#define FMC_PFAPR_M7PFD_MASK                     (0x800000U)
#define FMC_PFAPR_M7PFD_SHIFT                    (23U)
#define FMC_PFAPR_M7PFD(x)                       (((uint32_t)(((uint32_t)(x)) << FMC_PFAPR_M7PFD_SHIFT)) & FMC_PFAPR_M7PFD_MASK)

/*! @name PFB0CR - Flash Bank 0 Control Register */
#define FMC_PFB0CR_B0SEBE_MASK                   (0x1U)
#define FMC_PFB0CR_B0SEBE_SHIFT                  (0U)
#define FMC_PFB0CR_B0SEBE(x)                     (((uint32_t)(((uint32_t)(x)) << FMC_PFB0CR_B0SEBE_SHIFT)) & FMC_PFB0CR_B0SEBE_MASK)
#define FMC_PFB0CR_B0IPE_MASK                    (0x2U)
#define FMC_PFB0CR_B0IPE_SHIFT                   (1U)
#define FMC_PFB0CR_B0IPE(x)                      (((uint32_t)(((uint32_t)(x)) << FMC_PFB0CR_B0IPE_SHIFT)) & FMC_PFB0CR_B0IPE_MASK)
#define FMC_PFB0CR_B0DPE_MASK                    (0x4U)
#define FMC_PFB0CR_B0DPE_SHIFT                   (2U)
#define FMC_PFB0CR_B0DPE(x)                      (((uint32_t)(((uint32_t)(x)) << FMC_PFB0CR_B0DPE_SHIFT)) & FMC_PFB0CR_B0DPE_MASK)
#define FMC_PFB0CR_B0ICE_MASK                    (0x8U)
#define FMC_PFB0CR_B0ICE_SHIFT                   (3U)
#define FMC_PFB0CR_B0ICE(x)                      (((uint32_t)(((uint32_t)(x)) << FMC_PFB0CR_B0ICE_SHIFT)) & FMC_PFB0CR_B0ICE_MASK)
#define FMC_PFB0CR_B0DCE_MASK                    (0x10U)
#define FMC_PFB0CR_B0DCE_SHIFT                   (4U)
#define FMC_PFB0CR_B0DCE(x)                      (((uint32_t)(((uint32_t)(x)) << FMC_PFB0CR_B0DCE_SHIFT)) & FMC_PFB0CR_B0DCE_MASK)
#define FMC_PFB0CR_CRC_MASK                      (0xE0U)
#define FMC_PFB0CR_CRC_SHIFT                     (5U)
#define FMC_PFB0CR_CRC(x)                        (((uint32_t)(((uint32_t)(x)) << FMC_PFB0CR_CRC_SHIFT)) & FMC_PFB0CR_CRC_MASK)
#define FMC_PFB0CR_B0MW_MASK                     (0x60000U)
#define FMC_PFB0CR_B0MW_SHIFT                    (17U)
#define FMC_PFB0CR_B0MW(x)                       (((uint32_t)(((uint32_t)(x)) << FMC_PFB0CR_B0MW_SHIFT)) & FMC_PFB0CR_B0MW_MASK)
#define FMC_PFB0CR_S_B_INV_MASK                  (0x80000U)
#define FMC_PFB0CR_S_B_INV_SHIFT                 (19U)
#define FMC_PFB0CR_S_B_INV(x)                    (((uint32_t)(((uint32_t)(x)) << FMC_PFB0CR_S_B_INV_SHIFT)) & FMC_PFB0CR_S_B_INV_MASK)
#define FMC_PFB0CR_CINV_WAY_MASK                 (0xF00000U)
#define FMC_PFB0CR_CINV_WAY_SHIFT                (20U)
#define FMC_PFB0CR_CINV_WAY(x)                   (((uint32_t)(((uint32_t)(x)) << FMC_PFB0CR_CINV_WAY_SHIFT)) & FMC_PFB0CR_CINV_WAY_MASK)
#define FMC_PFB0CR_CLCK_WAY_MASK                 (0xF000000U)
#define FMC_PFB0CR_CLCK_WAY_SHIFT                (24U)
#define FMC_PFB0CR_CLCK_WAY(x)                   (((uint32_t)(((uint32_t)(x)) << FMC_PFB0CR_CLCK_WAY_SHIFT)) & FMC_PFB0CR_CLCK_WAY_MASK)
#define FMC_PFB0CR_B0RWSC_MASK                   (0xF0000000U)
#define FMC_PFB0CR_B0RWSC_SHIFT                  (28U)
#define FMC_PFB0CR_B0RWSC(x)                     (((uint32_t)(((uint32_t)(x)) << FMC_PFB0CR_B0RWSC_SHIFT)) & FMC_PFB0CR_B0RWSC_MASK)

/*! @name PFB1CR - Flash Bank 1 Control Register */
#define FMC_PFB1CR_B1SEBE_MASK                   (0x1U)
#define FMC_PFB1CR_B1SEBE_SHIFT                  (0U)
#define FMC_PFB1CR_B1SEBE(x)                     (((uint32_t)(((uint32_t)(x)) << FMC_PFB1CR_B1SEBE_SHIFT)) & FMC_PFB1CR_B1SEBE_MASK)
#define FMC_PFB1CR_B1IPE_MASK                    (0x2U)
#define FMC_PFB1CR_B1IPE_SHIFT                   (1U)
#define FMC_PFB1CR_B1IPE(x)                      (((uint32_t)(((uint32_t)(x)) << FMC_PFB1CR_B1IPE_SHIFT)) & FMC_PFB1CR_B1IPE_MASK)
#define FMC_PFB1CR_B1DPE_MASK                    (0x4U)
#define FMC_PFB1CR_B1DPE_SHIFT                   (2U)
#define FMC_PFB1CR_B1DPE(x)                      (((uint32_t)(((uint32_t)(x)) << FMC_PFB1CR_B1DPE_SHIFT)) & FMC_PFB1CR_B1DPE_MASK)
#define FMC_PFB1CR_B1ICE_MASK                    (0x8U)
#define FMC_PFB1CR_B1ICE_SHIFT                   (3U)
#define FMC_PFB1CR_B1ICE(x)                      (((uint32_t)(((uint32_t)(x)) << FMC_PFB1CR_B1ICE_SHIFT)) & FMC_PFB1CR_B1ICE_MASK)
#define FMC_PFB1CR_B1DCE_MASK                    (0x10U)
#define FMC_PFB1CR_B1DCE_SHIFT                   (4U)
#define FMC_PFB1CR_B1DCE(x)                      (((uint32_t)(((uint32_t)(x)) << FMC_PFB1CR_B1DCE_SHIFT)) & FMC_PFB1CR_B1DCE_MASK)
#define FMC_PFB1CR_B1MW_MASK                     (0x60000U)
#define FMC_PFB1CR_B1MW_SHIFT                    (17U)
#define FMC_PFB1CR_B1MW(x)                       (((uint32_t)(((uint32_t)(x)) << FMC_PFB1CR_B1MW_SHIFT)) & FMC_PFB1CR_B1MW_MASK)
#define FMC_PFB1CR_B1RWSC_MASK                   (0xF0000000U)
#define FMC_PFB1CR_B1RWSC_SHIFT                  (28U)
#define FMC_PFB1CR_B1RWSC(x)                     (((uint32_t)(((uint32_t)(x)) << FMC_PFB1CR_B1RWSC_SHIFT)) & FMC_PFB1CR_B1RWSC_MASK)

/*! @name TAGVD - Cache Tag Storage */
#define FMC_TAGVD_valid_MASK                     (0x1U)
#define FMC_TAGVD_valid_SHIFT                    (0U)
#define FMC_TAGVD_valid(x)                       (((uint32_t)(((uint32_t)(x)) << FMC_TAGVD_valid_SHIFT)) & FMC_TAGVD_valid_MASK)
#define FMC_TAGVD_tag_MASK                       (0x7FFF0U)
#define FMC_TAGVD_tag_SHIFT                      (4U)
#define FMC_TAGVD_tag(x)                         (((uint32_t)(((uint32_t)(x)) << FMC_TAGVD_tag_SHIFT)) & FMC_TAGVD_tag_MASK)

/* The count of FMC_TAGVD */
#define FMC_TAGVD_COUNT                          (4U)

/* The count of FMC_TAGVD */
#define FMC_TAGVD_COUNT2                         (2U)

/*! @name DATA_U - Cache Data Storage (upper word) */
#define FMC_DATA_U_data_MASK                     (0xFFFFFFFFU)
#define FMC_DATA_U_data_SHIFT                    (0U)
#define FMC_DATA_U_data(x)                       (((uint32_t)(((uint32_t)(x)) << FMC_DATA_U_data_SHIFT)) & FMC_DATA_U_data_MASK)

/* The count of FMC_DATA_U */
#define FMC_DATA_U_COUNT                         (4U)

/* The count of FMC_DATA_U */
#define FMC_DATA_U_COUNT2                        (2U)

/*! @name DATA_L - Cache Data Storage (lower word) */
#define FMC_DATA_L_data_MASK                     (0xFFFFFFFFU)
#define FMC_DATA_L_data_SHIFT                    (0U)
#define FMC_DATA_L_data(x)                       (((uint32_t)(((uint32_t)(x)) << FMC_DATA_L_data_SHIFT)) & FMC_DATA_L_data_MASK)

/* The count of FMC_DATA_L */
#define FMC_DATA_L_COUNT                         (4U)

/* The count of FMC_DATA_L */
#define FMC_DATA_L_COUNT2                        (2U)


/*!
 * @}
 */ /* end of group FMC_Register_Masks */


/* FMC - Peripheral instance base addresses */
/** Peripheral FMC base address */
#define FMC_BASE                                 (0x4001F000u)
/** Peripheral FMC base pointer */
#define FMC                                      ((FMC_Type *)FMC_BASE)
/** Array initializer of FMC peripheral base addresses */
#define FMC_BASE_ADDRS                           { FMC_BASE }
/** Array initializer of FMC peripheral base pointers */
#define FMC_BASE_PTRS                            { FMC }

/*!
 * @}
 */ /* end of group FMC_Peripheral_Access_Layer */


/* ----------------------------------------------------------------------------
   -- FTFL Peripheral Access Layer
   ---------------------------------------------------------------------------- */

/*!
 * @addtogroup FTFL_Peripheral_Access_Layer FTFL Peripheral Access Layer
 * @{
 */

/** FTFL - Register Layout Typedef */
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
       uint8_t RESERVED_0[2];
  __IO uint8_t FEPROT;                             /**< EEPROM Protection Register, offset: 0x16 */
  __IO uint8_t FDPROT;                             /**< Data Flash Protection Register, offset: 0x17 */
} FTFL_Type;

/* ----------------------------------------------------------------------------
   -- FTFL Register Masks
   ---------------------------------------------------------------------------- */

/*!
 * @addtogroup FTFL_Register_Masks FTFL Register Masks
 * @{
 */

/*! @name FSTAT - Flash Status Register */
#define FTFL_FSTAT_MGSTAT0_MASK                  (0x1U)
#define FTFL_FSTAT_MGSTAT0_SHIFT                 (0U)
#define FTFL_FSTAT_MGSTAT0(x)                    (((uint8_t)(((uint8_t)(x)) << FTFL_FSTAT_MGSTAT0_SHIFT)) & FTFL_FSTAT_MGSTAT0_MASK)
#define FTFL_FSTAT_FPVIOL_MASK                   (0x10U)
#define FTFL_FSTAT_FPVIOL_SHIFT                  (4U)
#define FTFL_FSTAT_FPVIOL(x)                     (((uint8_t)(((uint8_t)(x)) << FTFL_FSTAT_FPVIOL_SHIFT)) & FTFL_FSTAT_FPVIOL_MASK)
#define FTFL_FSTAT_ACCERR_MASK                   (0x20U)
#define FTFL_FSTAT_ACCERR_SHIFT                  (5U)
#define FTFL_FSTAT_ACCERR(x)                     (((uint8_t)(((uint8_t)(x)) << FTFL_FSTAT_ACCERR_SHIFT)) & FTFL_FSTAT_ACCERR_MASK)
#define FTFL_FSTAT_RDCOLERR_MASK                 (0x40U)
#define FTFL_FSTAT_RDCOLERR_SHIFT                (6U)
#define FTFL_FSTAT_RDCOLERR(x)                   (((uint8_t)(((uint8_t)(x)) << FTFL_FSTAT_RDCOLERR_SHIFT)) & FTFL_FSTAT_RDCOLERR_MASK)
#define FTFL_FSTAT_CCIF_MASK                     (0x80U)
#define FTFL_FSTAT_CCIF_SHIFT                    (7U)
#define FTFL_FSTAT_CCIF(x)                       (((uint8_t)(((uint8_t)(x)) << FTFL_FSTAT_CCIF_SHIFT)) & FTFL_FSTAT_CCIF_MASK)

/*! @name FCNFG - Flash Configuration Register */
#define FTFL_FCNFG_EEERDY_MASK                   (0x1U)
#define FTFL_FCNFG_EEERDY_SHIFT                  (0U)
#define FTFL_FCNFG_EEERDY(x)                     (((uint8_t)(((uint8_t)(x)) << FTFL_FCNFG_EEERDY_SHIFT)) & FTFL_FCNFG_EEERDY_MASK)
#define FTFL_FCNFG_RAMRDY_MASK                   (0x2U)
#define FTFL_FCNFG_RAMRDY_SHIFT                  (1U)
#define FTFL_FCNFG_RAMRDY(x)                     (((uint8_t)(((uint8_t)(x)) << FTFL_FCNFG_RAMRDY_SHIFT)) & FTFL_FCNFG_RAMRDY_MASK)
#define FTFL_FCNFG_PFLSH_MASK                    (0x4U)
#define FTFL_FCNFG_PFLSH_SHIFT                   (2U)
#define FTFL_FCNFG_PFLSH(x)                      (((uint8_t)(((uint8_t)(x)) << FTFL_FCNFG_PFLSH_SHIFT)) & FTFL_FCNFG_PFLSH_MASK)
#define FTFL_FCNFG_SWAP_MASK                     (0x8U)
#define FTFL_FCNFG_SWAP_SHIFT                    (3U)
#define FTFL_FCNFG_SWAP(x)                       (((uint8_t)(((uint8_t)(x)) << FTFL_FCNFG_SWAP_SHIFT)) & FTFL_FCNFG_SWAP_MASK)
#define FTFL_FCNFG_ERSSUSP_MASK                  (0x10U)
#define FTFL_FCNFG_ERSSUSP_SHIFT                 (4U)
#define FTFL_FCNFG_ERSSUSP(x)                    (((uint8_t)(((uint8_t)(x)) << FTFL_FCNFG_ERSSUSP_SHIFT)) & FTFL_FCNFG_ERSSUSP_MASK)
#define FTFL_FCNFG_ERSAREQ_MASK                  (0x20U)
#define FTFL_FCNFG_ERSAREQ_SHIFT                 (5U)
#define FTFL_FCNFG_ERSAREQ(x)                    (((uint8_t)(((uint8_t)(x)) << FTFL_FCNFG_ERSAREQ_SHIFT)) & FTFL_FCNFG_ERSAREQ_MASK)
#define FTFL_FCNFG_RDCOLLIE_MASK                 (0x40U)
#define FTFL_FCNFG_RDCOLLIE_SHIFT                (6U)
#define FTFL_FCNFG_RDCOLLIE(x)                   (((uint8_t)(((uint8_t)(x)) << FTFL_FCNFG_RDCOLLIE_SHIFT)) & FTFL_FCNFG_RDCOLLIE_MASK)
#define FTFL_FCNFG_CCIE_MASK                     (0x80U)
#define FTFL_FCNFG_CCIE_SHIFT                    (7U)
#define FTFL_FCNFG_CCIE(x)                       (((uint8_t)(((uint8_t)(x)) << FTFL_FCNFG_CCIE_SHIFT)) & FTFL_FCNFG_CCIE_MASK)

/*! @name FSEC - Flash Security Register */
#define FTFL_FSEC_SEC_MASK                       (0x3U)
#define FTFL_FSEC_SEC_SHIFT                      (0U)
#define FTFL_FSEC_SEC(x)                         (((uint8_t)(((uint8_t)(x)) << FTFL_FSEC_SEC_SHIFT)) & FTFL_FSEC_SEC_MASK)
#define FTFL_FSEC_FSLACC_MASK                    (0xCU)
#define FTFL_FSEC_FSLACC_SHIFT                   (2U)
#define FTFL_FSEC_FSLACC(x)                      (((uint8_t)(((uint8_t)(x)) << FTFL_FSEC_FSLACC_SHIFT)) & FTFL_FSEC_FSLACC_MASK)
#define FTFL_FSEC_MEEN_MASK                      (0x30U)
#define FTFL_FSEC_MEEN_SHIFT                     (4U)
#define FTFL_FSEC_MEEN(x)                        (((uint8_t)(((uint8_t)(x)) << FTFL_FSEC_MEEN_SHIFT)) & FTFL_FSEC_MEEN_MASK)
#define FTFL_FSEC_KEYEN_MASK                     (0xC0U)
#define FTFL_FSEC_KEYEN_SHIFT                    (6U)
#define FTFL_FSEC_KEYEN(x)                       (((uint8_t)(((uint8_t)(x)) << FTFL_FSEC_KEYEN_SHIFT)) & FTFL_FSEC_KEYEN_MASK)

/*! @name FOPT - Flash Option Register */
#define FTFL_FOPT_OPT_MASK                       (0xFFU)
#define FTFL_FOPT_OPT_SHIFT                      (0U)
#define FTFL_FOPT_OPT(x)                         (((uint8_t)(((uint8_t)(x)) << FTFL_FOPT_OPT_SHIFT)) & FTFL_FOPT_OPT_MASK)

/*! @name FCCOB3 - Flash Common Command Object Registers */
#define FTFL_FCCOB3_CCOBn_MASK                   (0xFFU)
#define FTFL_FCCOB3_CCOBn_SHIFT                  (0U)
#define FTFL_FCCOB3_CCOBn(x)                     (((uint8_t)(((uint8_t)(x)) << FTFL_FCCOB3_CCOBn_SHIFT)) & FTFL_FCCOB3_CCOBn_MASK)

/*! @name FCCOB2 - Flash Common Command Object Registers */
#define FTFL_FCCOB2_CCOBn_MASK                   (0xFFU)
#define FTFL_FCCOB2_CCOBn_SHIFT                  (0U)
#define FTFL_FCCOB2_CCOBn(x)                     (((uint8_t)(((uint8_t)(x)) << FTFL_FCCOB2_CCOBn_SHIFT)) & FTFL_FCCOB2_CCOBn_MASK)

/*! @name FCCOB1 - Flash Common Command Object Registers */
#define FTFL_FCCOB1_CCOBn_MASK                   (0xFFU)
#define FTFL_FCCOB1_CCOBn_SHIFT                  (0U)
#define FTFL_FCCOB1_CCOBn(x)                     (((uint8_t)(((uint8_t)(x)) << FTFL_FCCOB1_CCOBn_SHIFT)) & FTFL_FCCOB1_CCOBn_MASK)

/*! @name FCCOB0 - Flash Common Command Object Registers */
#define FTFL_FCCOB0_CCOBn_MASK                   (0xFFU)
#define FTFL_FCCOB0_CCOBn_SHIFT                  (0U)
#define FTFL_FCCOB0_CCOBn(x)                     (((uint8_t)(((uint8_t)(x)) << FTFL_FCCOB0_CCOBn_SHIFT)) & FTFL_FCCOB0_CCOBn_MASK)

/*! @name FCCOB7 - Flash Common Command Object Registers */
#define FTFL_FCCOB7_CCOBn_MASK                   (0xFFU)
#define FTFL_FCCOB7_CCOBn_SHIFT                  (0U)
#define FTFL_FCCOB7_CCOBn(x)                     (((uint8_t)(((uint8_t)(x)) << FTFL_FCCOB7_CCOBn_SHIFT)) & FTFL_FCCOB7_CCOBn_MASK)

/*! @name FCCOB6 - Flash Common Command Object Registers */
#define FTFL_FCCOB6_CCOBn_MASK                   (0xFFU)
#define FTFL_FCCOB6_CCOBn_SHIFT                  (0U)
#define FTFL_FCCOB6_CCOBn(x)                     (((uint8_t)(((uint8_t)(x)) << FTFL_FCCOB6_CCOBn_SHIFT)) & FTFL_FCCOB6_CCOBn_MASK)

/*! @name FCCOB5 - Flash Common Command Object Registers */
#define FTFL_FCCOB5_CCOBn_MASK                   (0xFFU)
#define FTFL_FCCOB5_CCOBn_SHIFT                  (0U)
#define FTFL_FCCOB5_CCOBn(x)                     (((uint8_t)(((uint8_t)(x)) << FTFL_FCCOB5_CCOBn_SHIFT)) & FTFL_FCCOB5_CCOBn_MASK)

/*! @name FCCOB4 - Flash Common Command Object Registers */
#define FTFL_FCCOB4_CCOBn_MASK                   (0xFFU)
#define FTFL_FCCOB4_CCOBn_SHIFT                  (0U)
#define FTFL_FCCOB4_CCOBn(x)                     (((uint8_t)(((uint8_t)(x)) << FTFL_FCCOB4_CCOBn_SHIFT)) & FTFL_FCCOB4_CCOBn_MASK)

/*! @name FCCOBB - Flash Common Command Object Registers */
#define FTFL_FCCOBB_CCOBn_MASK                   (0xFFU)
#define FTFL_FCCOBB_CCOBn_SHIFT                  (0U)
#define FTFL_FCCOBB_CCOBn(x)                     (((uint8_t)(((uint8_t)(x)) << FTFL_FCCOBB_CCOBn_SHIFT)) & FTFL_FCCOBB_CCOBn_MASK)

/*! @name FCCOBA - Flash Common Command Object Registers */
#define FTFL_FCCOBA_CCOBn_MASK                   (0xFFU)
#define FTFL_FCCOBA_CCOBn_SHIFT                  (0U)
#define FTFL_FCCOBA_CCOBn(x)                     (((uint8_t)(((uint8_t)(x)) << FTFL_FCCOBA_CCOBn_SHIFT)) & FTFL_FCCOBA_CCOBn_MASK)

/*! @name FCCOB9 - Flash Common Command Object Registers */
#define FTFL_FCCOB9_CCOBn_MASK                   (0xFFU)
#define FTFL_FCCOB9_CCOBn_SHIFT                  (0U)
#define FTFL_FCCOB9_CCOBn(x)                     (((uint8_t)(((uint8_t)(x)) << FTFL_FCCOB9_CCOBn_SHIFT)) & FTFL_FCCOB9_CCOBn_MASK)

/*! @name FCCOB8 - Flash Common Command Object Registers */
#define FTFL_FCCOB8_CCOBn_MASK                   (0xFFU)
#define FTFL_FCCOB8_CCOBn_SHIFT                  (0U)
#define FTFL_FCCOB8_CCOBn(x)                     (((uint8_t)(((uint8_t)(x)) << FTFL_FCCOB8_CCOBn_SHIFT)) & FTFL_FCCOB8_CCOBn_MASK)

/*! @name FPROT3 - Program Flash Protection Registers */
#define FTFL_FPROT3_PROT_MASK                    (0xFFU)
#define FTFL_FPROT3_PROT_SHIFT                   (0U)
#define FTFL_FPROT3_PROT(x)                      (((uint8_t)(((uint8_t)(x)) << FTFL_FPROT3_PROT_SHIFT)) & FTFL_FPROT3_PROT_MASK)

/*! @name FPROT2 - Program Flash Protection Registers */
#define FTFL_FPROT2_PROT_MASK                    (0xFFU)
#define FTFL_FPROT2_PROT_SHIFT                   (0U)
#define FTFL_FPROT2_PROT(x)                      (((uint8_t)(((uint8_t)(x)) << FTFL_FPROT2_PROT_SHIFT)) & FTFL_FPROT2_PROT_MASK)

/*! @name FPROT1 - Program Flash Protection Registers */
#define FTFL_FPROT1_PROT_MASK                    (0xFFU)
#define FTFL_FPROT1_PROT_SHIFT                   (0U)
#define FTFL_FPROT1_PROT(x)                      (((uint8_t)(((uint8_t)(x)) << FTFL_FPROT1_PROT_SHIFT)) & FTFL_FPROT1_PROT_MASK)

/*! @name FPROT0 - Program Flash Protection Registers */
#define FTFL_FPROT0_PROT_MASK                    (0xFFU)
#define FTFL_FPROT0_PROT_SHIFT                   (0U)
#define FTFL_FPROT0_PROT(x)                      (((uint8_t)(((uint8_t)(x)) << FTFL_FPROT0_PROT_SHIFT)) & FTFL_FPROT0_PROT_MASK)

/*! @name FEPROT - EEPROM Protection Register */
#define FTFL_FEPROT_EPROT_MASK                   (0xFFU)
#define FTFL_FEPROT_EPROT_SHIFT                  (0U)
#define FTFL_FEPROT_EPROT(x)                     (((uint8_t)(((uint8_t)(x)) << FTFL_FEPROT_EPROT_SHIFT)) & FTFL_FEPROT_EPROT_MASK)

/*! @name FDPROT - Data Flash Protection Register */
#define FTFL_FDPROT_DPROT_MASK                   (0xFFU)
#define FTFL_FDPROT_DPROT_SHIFT                  (0U)
#define FTFL_FDPROT_DPROT(x)                     (((uint8_t)(((uint8_t)(x)) << FTFL_FDPROT_DPROT_SHIFT)) & FTFL_FDPROT_DPROT_MASK)


/*!
 * @}
 */ /* end of group FTFL_Register_Masks */


/* FTFL - Peripheral instance base addresses */
/** Peripheral FTFL base address */
#define FTFL_BASE                                (0x40020000u)
/** Peripheral FTFL base pointer */
#define FTFL                                     ((FTFL_Type *)FTFL_BASE)
/** Array initializer of FTFL peripheral base addresses */
#define FTFL_BASE_ADDRS                          { FTFL_BASE }
/** Array initializer of FTFL peripheral base pointers */
#define FTFL_BASE_PTRS                           { FTFL }
/** Interrupt vectors for the FTFL peripheral type */
#define FTFL_COMMAND_COMPLETE_IRQS               { FTFL_IRQn }
#define FTFL_READ_COLLISION_IRQS                 { FTFL_Collision_IRQn }

/*!
 * @}
 */ /* end of group FTFL_Peripheral_Access_Layer */


/* ----------------------------------------------------------------------------
   -- FTM Peripheral Access Layer
   ---------------------------------------------------------------------------- */

/*!
 * @addtogroup FTM_Peripheral_Access_Layer FTM Peripheral Access Layer
 * @{
 */

/** FTM - Register Layout Typedef */
typedef struct {
  __IO uint32_t SC;                                /**< Status And Control, offset: 0x0 */
  __IO uint32_t CNT;                               /**< Counter, offset: 0x4 */
  __IO uint32_t MOD;                               /**< Modulo, offset: 0x8 */
  struct {                                         /* offset: 0xC, array step: 0x8 */
    __IO uint32_t CnSC;                              /**< Channel (n) Status And Control, array offset: 0xC, array step: 0x8 */
    __IO uint32_t CnV;                               /**< Channel (n) Value, array offset: 0x10, array step: 0x8 */
  } CONTROLS[8];
  __IO uint32_t CNTIN;                             /**< Counter Initial Value, offset: 0x4C */
  __IO uint32_t STATUS;                            /**< Capture And Compare Status, offset: 0x50 */
  __IO uint32_t MODE;                              /**< Features Mode Selection, offset: 0x54 */
  __IO uint32_t SYNC;                              /**< Synchronization, offset: 0x58 */
  __IO uint32_t OUTINIT;                           /**< Initial State For Channels Output, offset: 0x5C */
  __IO uint32_t OUTMASK;                           /**< Output Mask, offset: 0x60 */
  __IO uint32_t COMBINE;                           /**< Function For Linked Channels, offset: 0x64 */
  __IO uint32_t DEADTIME;                          /**< Deadtime Insertion Control, offset: 0x68 */
  __IO uint32_t EXTTRIG;                           /**< FTM External Trigger, offset: 0x6C */
  __IO uint32_t POL;                               /**< Channels Polarity, offset: 0x70 */
  __IO uint32_t FMS;                               /**< Fault Mode Status, offset: 0x74 */
  __IO uint32_t FILTER;                            /**< Input Capture Filter Control, offset: 0x78 */
  __IO uint32_t FLTCTRL;                           /**< Fault Control, offset: 0x7C */
  __IO uint32_t QDCTRL;                            /**< Quadrature Decoder Control And Status, offset: 0x80 */
  __IO uint32_t CONF;                              /**< Configuration, offset: 0x84 */
  __IO uint32_t FLTPOL;                            /**< FTM Fault Input Polarity, offset: 0x88 */
  __IO uint32_t SYNCONF;                           /**< Synchronization Configuration, offset: 0x8C */
  __IO uint32_t INVCTRL;                           /**< FTM Inverting Control, offset: 0x90 */
  __IO uint32_t SWOCTRL;                           /**< FTM Software Output Control, offset: 0x94 */
  __IO uint32_t PWMLOAD;                           /**< FTM PWM Load, offset: 0x98 */
} FTM_Type;

/* ----------------------------------------------------------------------------
   -- FTM Register Masks
   ---------------------------------------------------------------------------- */

/*!
 * @addtogroup FTM_Register_Masks FTM Register Masks
 * @{
 */

/*! @name SC - Status And Control */
#define FTM_SC_PS_MASK                           (0x7U)
#define FTM_SC_PS_SHIFT                          (0U)
#define FTM_SC_PS(x)                             (((uint32_t)(((uint32_t)(x)) << FTM_SC_PS_SHIFT)) & FTM_SC_PS_MASK)
#define FTM_SC_CLKS_MASK                         (0x18U)
#define FTM_SC_CLKS_SHIFT                        (3U)
#define FTM_SC_CLKS(x)                           (((uint32_t)(((uint32_t)(x)) << FTM_SC_CLKS_SHIFT)) & FTM_SC_CLKS_MASK)
#define FTM_SC_CPWMS_MASK                        (0x20U)
#define FTM_SC_CPWMS_SHIFT                       (5U)
#define FTM_SC_CPWMS(x)                          (((uint32_t)(((uint32_t)(x)) << FTM_SC_CPWMS_SHIFT)) & FTM_SC_CPWMS_MASK)
#define FTM_SC_TOIE_MASK                         (0x40U)
#define FTM_SC_TOIE_SHIFT                        (6U)
#define FTM_SC_TOIE(x)                           (((uint32_t)(((uint32_t)(x)) << FTM_SC_TOIE_SHIFT)) & FTM_SC_TOIE_MASK)
#define FTM_SC_TOF_MASK                          (0x80U)
#define FTM_SC_TOF_SHIFT                         (7U)
#define FTM_SC_TOF(x)                            (((uint32_t)(((uint32_t)(x)) << FTM_SC_TOF_SHIFT)) & FTM_SC_TOF_MASK)

/*! @name CNT - Counter */
#define FTM_CNT_COUNT_MASK                       (0xFFFFU)
#define FTM_CNT_COUNT_SHIFT                      (0U)
#define FTM_CNT_COUNT(x)                         (((uint32_t)(((uint32_t)(x)) << FTM_CNT_COUNT_SHIFT)) & FTM_CNT_COUNT_MASK)

/*! @name MOD - Modulo */
#define FTM_MOD_MOD_MASK                         (0xFFFFU)
#define FTM_MOD_MOD_SHIFT                        (0U)
#define FTM_MOD_MOD(x)                           (((uint32_t)(((uint32_t)(x)) << FTM_MOD_MOD_SHIFT)) & FTM_MOD_MOD_MASK)

/*! @name CnSC - Channel (n) Status And Control */
#define FTM_CnSC_DMA_MASK                        (0x1U)
#define FTM_CnSC_DMA_SHIFT                       (0U)
#define FTM_CnSC_DMA(x)                          (((uint32_t)(((uint32_t)(x)) << FTM_CnSC_DMA_SHIFT)) & FTM_CnSC_DMA_MASK)
#define FTM_CnSC_ELSA_MASK                       (0x4U)
#define FTM_CnSC_ELSA_SHIFT                      (2U)
#define FTM_CnSC_ELSA(x)                         (((uint32_t)(((uint32_t)(x)) << FTM_CnSC_ELSA_SHIFT)) & FTM_CnSC_ELSA_MASK)
#define FTM_CnSC_ELSB_MASK                       (0x8U)
#define FTM_CnSC_ELSB_SHIFT                      (3U)
#define FTM_CnSC_ELSB(x)                         (((uint32_t)(((uint32_t)(x)) << FTM_CnSC_ELSB_SHIFT)) & FTM_CnSC_ELSB_MASK)
#define FTM_CnSC_MSA_MASK                        (0x10U)
#define FTM_CnSC_MSA_SHIFT                       (4U)
#define FTM_CnSC_MSA(x)                          (((uint32_t)(((uint32_t)(x)) << FTM_CnSC_MSA_SHIFT)) & FTM_CnSC_MSA_MASK)
#define FTM_CnSC_MSB_MASK                        (0x20U)
#define FTM_CnSC_MSB_SHIFT                       (5U)
#define FTM_CnSC_MSB(x)                          (((uint32_t)(((uint32_t)(x)) << FTM_CnSC_MSB_SHIFT)) & FTM_CnSC_MSB_MASK)
#define FTM_CnSC_CHIE_MASK                       (0x40U)
#define FTM_CnSC_CHIE_SHIFT                      (6U)
#define FTM_CnSC_CHIE(x)                         (((uint32_t)(((uint32_t)(x)) << FTM_CnSC_CHIE_SHIFT)) & FTM_CnSC_CHIE_MASK)
#define FTM_CnSC_CHF_MASK                        (0x80U)
#define FTM_CnSC_CHF_SHIFT                       (7U)
#define FTM_CnSC_CHF(x)                          (((uint32_t)(((uint32_t)(x)) << FTM_CnSC_CHF_SHIFT)) & FTM_CnSC_CHF_MASK)

/* The count of FTM_CnSC */
#define FTM_CnSC_COUNT                           (8U)

/*! @name CnV - Channel (n) Value */
#define FTM_CnV_VAL_MASK                         (0xFFFFU)
#define FTM_CnV_VAL_SHIFT                        (0U)
#define FTM_CnV_VAL(x)                           (((uint32_t)(((uint32_t)(x)) << FTM_CnV_VAL_SHIFT)) & FTM_CnV_VAL_MASK)

/* The count of FTM_CnV */
#define FTM_CnV_COUNT                            (8U)

/*! @name CNTIN - Counter Initial Value */
#define FTM_CNTIN_INIT_MASK                      (0xFFFFU)
#define FTM_CNTIN_INIT_SHIFT                     (0U)
#define FTM_CNTIN_INIT(x)                        (((uint32_t)(((uint32_t)(x)) << FTM_CNTIN_INIT_SHIFT)) & FTM_CNTIN_INIT_MASK)

/*! @name STATUS - Capture And Compare Status */
#define FTM_STATUS_CH0F_MASK                     (0x1U)
#define FTM_STATUS_CH0F_SHIFT                    (0U)
#define FTM_STATUS_CH0F(x)                       (((uint32_t)(((uint32_t)(x)) << FTM_STATUS_CH0F_SHIFT)) & FTM_STATUS_CH0F_MASK)
#define FTM_STATUS_CH1F_MASK                     (0x2U)
#define FTM_STATUS_CH1F_SHIFT                    (1U)
#define FTM_STATUS_CH1F(x)                       (((uint32_t)(((uint32_t)(x)) << FTM_STATUS_CH1F_SHIFT)) & FTM_STATUS_CH1F_MASK)
#define FTM_STATUS_CH2F_MASK                     (0x4U)
#define FTM_STATUS_CH2F_SHIFT                    (2U)
#define FTM_STATUS_CH2F(x)                       (((uint32_t)(((uint32_t)(x)) << FTM_STATUS_CH2F_SHIFT)) & FTM_STATUS_CH2F_MASK)
#define FTM_STATUS_CH3F_MASK                     (0x8U)
#define FTM_STATUS_CH3F_SHIFT                    (3U)
#define FTM_STATUS_CH3F(x)                       (((uint32_t)(((uint32_t)(x)) << FTM_STATUS_CH3F_SHIFT)) & FTM_STATUS_CH3F_MASK)
#define FTM_STATUS_CH4F_MASK                     (0x10U)
#define FTM_STATUS_CH4F_SHIFT                    (4U)
#define FTM_STATUS_CH4F(x)                       (((uint32_t)(((uint32_t)(x)) << FTM_STATUS_CH4F_SHIFT)) & FTM_STATUS_CH4F_MASK)
#define FTM_STATUS_CH5F_MASK                     (0x20U)
#define FTM_STATUS_CH5F_SHIFT                    (5U)
#define FTM_STATUS_CH5F(x)                       (((uint32_t)(((uint32_t)(x)) << FTM_STATUS_CH5F_SHIFT)) & FTM_STATUS_CH5F_MASK)
#define FTM_STATUS_CH6F_MASK                     (0x40U)
#define FTM_STATUS_CH6F_SHIFT                    (6U)
#define FTM_STATUS_CH6F(x)                       (((uint32_t)(((uint32_t)(x)) << FTM_STATUS_CH6F_SHIFT)) & FTM_STATUS_CH6F_MASK)
#define FTM_STATUS_CH7F_MASK                     (0x80U)
#define FTM_STATUS_CH7F_SHIFT                    (7U)
#define FTM_STATUS_CH7F(x)                       (((uint32_t)(((uint32_t)(x)) << FTM_STATUS_CH7F_SHIFT)) & FTM_STATUS_CH7F_MASK)

/*! @name MODE - Features Mode Selection */
#define FTM_MODE_FTMEN_MASK                      (0x1U)
#define FTM_MODE_FTMEN_SHIFT                     (0U)
#define FTM_MODE_FTMEN(x)                        (((uint32_t)(((uint32_t)(x)) << FTM_MODE_FTMEN_SHIFT)) & FTM_MODE_FTMEN_MASK)
#define FTM_MODE_INIT_MASK                       (0x2U)
#define FTM_MODE_INIT_SHIFT                      (1U)
#define FTM_MODE_INIT(x)                         (((uint32_t)(((uint32_t)(x)) << FTM_MODE_INIT_SHIFT)) & FTM_MODE_INIT_MASK)
#define FTM_MODE_WPDIS_MASK                      (0x4U)
#define FTM_MODE_WPDIS_SHIFT                     (2U)
#define FTM_MODE_WPDIS(x)                        (((uint32_t)(((uint32_t)(x)) << FTM_MODE_WPDIS_SHIFT)) & FTM_MODE_WPDIS_MASK)
#define FTM_MODE_PWMSYNC_MASK                    (0x8U)
#define FTM_MODE_PWMSYNC_SHIFT                   (3U)
#define FTM_MODE_PWMSYNC(x)                      (((uint32_t)(((uint32_t)(x)) << FTM_MODE_PWMSYNC_SHIFT)) & FTM_MODE_PWMSYNC_MASK)
#define FTM_MODE_CAPTEST_MASK                    (0x10U)
#define FTM_MODE_CAPTEST_SHIFT                   (4U)
#define FTM_MODE_CAPTEST(x)                      (((uint32_t)(((uint32_t)(x)) << FTM_MODE_CAPTEST_SHIFT)) & FTM_MODE_CAPTEST_MASK)
#define FTM_MODE_FAULTM_MASK                     (0x60U)
#define FTM_MODE_FAULTM_SHIFT                    (5U)
#define FTM_MODE_FAULTM(x)                       (((uint32_t)(((uint32_t)(x)) << FTM_MODE_FAULTM_SHIFT)) & FTM_MODE_FAULTM_MASK)
#define FTM_MODE_FAULTIE_MASK                    (0x80U)
#define FTM_MODE_FAULTIE_SHIFT                   (7U)
#define FTM_MODE_FAULTIE(x)                      (((uint32_t)(((uint32_t)(x)) << FTM_MODE_FAULTIE_SHIFT)) & FTM_MODE_FAULTIE_MASK)

/*! @name SYNC - Synchronization */
#define FTM_SYNC_CNTMIN_MASK                     (0x1U)
#define FTM_SYNC_CNTMIN_SHIFT                    (0U)
#define FTM_SYNC_CNTMIN(x)                       (((uint32_t)(((uint32_t)(x)) << FTM_SYNC_CNTMIN_SHIFT)) & FTM_SYNC_CNTMIN_MASK)
#define FTM_SYNC_CNTMAX_MASK                     (0x2U)
#define FTM_SYNC_CNTMAX_SHIFT                    (1U)
#define FTM_SYNC_CNTMAX(x)                       (((uint32_t)(((uint32_t)(x)) << FTM_SYNC_CNTMAX_SHIFT)) & FTM_SYNC_CNTMAX_MASK)
#define FTM_SYNC_REINIT_MASK                     (0x4U)
#define FTM_SYNC_REINIT_SHIFT                    (2U)
#define FTM_SYNC_REINIT(x)                       (((uint32_t)(((uint32_t)(x)) << FTM_SYNC_REINIT_SHIFT)) & FTM_SYNC_REINIT_MASK)
#define FTM_SYNC_SYNCHOM_MASK                    (0x8U)
#define FTM_SYNC_SYNCHOM_SHIFT                   (3U)
#define FTM_SYNC_SYNCHOM(x)                      (((uint32_t)(((uint32_t)(x)) << FTM_SYNC_SYNCHOM_SHIFT)) & FTM_SYNC_SYNCHOM_MASK)
#define FTM_SYNC_TRIG0_MASK                      (0x10U)
#define FTM_SYNC_TRIG0_SHIFT                     (4U)
#define FTM_SYNC_TRIG0(x)                        (((uint32_t)(((uint32_t)(x)) << FTM_SYNC_TRIG0_SHIFT)) & FTM_SYNC_TRIG0_MASK)
#define FTM_SYNC_TRIG1_MASK                      (0x20U)
#define FTM_SYNC_TRIG1_SHIFT                     (5U)
#define FTM_SYNC_TRIG1(x)                        (((uint32_t)(((uint32_t)(x)) << FTM_SYNC_TRIG1_SHIFT)) & FTM_SYNC_TRIG1_MASK)
#define FTM_SYNC_TRIG2_MASK                      (0x40U)
#define FTM_SYNC_TRIG2_SHIFT                     (6U)
#define FTM_SYNC_TRIG2(x)                        (((uint32_t)(((uint32_t)(x)) << FTM_SYNC_TRIG2_SHIFT)) & FTM_SYNC_TRIG2_MASK)
#define FTM_SYNC_SWSYNC_MASK                     (0x80U)
#define FTM_SYNC_SWSYNC_SHIFT                    (7U)
#define FTM_SYNC_SWSYNC(x)                       (((uint32_t)(((uint32_t)(x)) << FTM_SYNC_SWSYNC_SHIFT)) & FTM_SYNC_SWSYNC_MASK)

/*! @name OUTINIT - Initial State For Channels Output */
#define FTM_OUTINIT_CH0OI_MASK                   (0x1U)
#define FTM_OUTINIT_CH0OI_SHIFT                  (0U)
#define FTM_OUTINIT_CH0OI(x)                     (((uint32_t)(((uint32_t)(x)) << FTM_OUTINIT_CH0OI_SHIFT)) & FTM_OUTINIT_CH0OI_MASK)
#define FTM_OUTINIT_CH1OI_MASK                   (0x2U)
#define FTM_OUTINIT_CH1OI_SHIFT                  (1U)
#define FTM_OUTINIT_CH1OI(x)                     (((uint32_t)(((uint32_t)(x)) << FTM_OUTINIT_CH1OI_SHIFT)) & FTM_OUTINIT_CH1OI_MASK)
#define FTM_OUTINIT_CH2OI_MASK                   (0x4U)
#define FTM_OUTINIT_CH2OI_SHIFT                  (2U)
#define FTM_OUTINIT_CH2OI(x)                     (((uint32_t)(((uint32_t)(x)) << FTM_OUTINIT_CH2OI_SHIFT)) & FTM_OUTINIT_CH2OI_MASK)
#define FTM_OUTINIT_CH3OI_MASK                   (0x8U)
#define FTM_OUTINIT_CH3OI_SHIFT                  (3U)
#define FTM_OUTINIT_CH3OI(x)                     (((uint32_t)(((uint32_t)(x)) << FTM_OUTINIT_CH3OI_SHIFT)) & FTM_OUTINIT_CH3OI_MASK)
#define FTM_OUTINIT_CH4OI_MASK                   (0x10U)
#define FTM_OUTINIT_CH4OI_SHIFT                  (4U)
#define FTM_OUTINIT_CH4OI(x)                     (((uint32_t)(((uint32_t)(x)) << FTM_OUTINIT_CH4OI_SHIFT)) & FTM_OUTINIT_CH4OI_MASK)
#define FTM_OUTINIT_CH5OI_MASK                   (0x20U)
#define FTM_OUTINIT_CH5OI_SHIFT                  (5U)
#define FTM_OUTINIT_CH5OI(x)                     (((uint32_t)(((uint32_t)(x)) << FTM_OUTINIT_CH5OI_SHIFT)) & FTM_OUTINIT_CH5OI_MASK)
#define FTM_OUTINIT_CH6OI_MASK                   (0x40U)
#define FTM_OUTINIT_CH6OI_SHIFT                  (6U)
#define FTM_OUTINIT_CH6OI(x)                     (((uint32_t)(((uint32_t)(x)) << FTM_OUTINIT_CH6OI_SHIFT)) & FTM_OUTINIT_CH6OI_MASK)
#define FTM_OUTINIT_CH7OI_MASK                   (0x80U)
#define FTM_OUTINIT_CH7OI_SHIFT                  (7U)
#define FTM_OUTINIT_CH7OI(x)                     (((uint32_t)(((uint32_t)(x)) << FTM_OUTINIT_CH7OI_SHIFT)) & FTM_OUTINIT_CH7OI_MASK)

/*! @name OUTMASK - Output Mask */
#define FTM_OUTMASK_CH0OM_MASK                   (0x1U)
#define FTM_OUTMASK_CH0OM_SHIFT                  (0U)
#define FTM_OUTMASK_CH0OM(x)                     (((uint32_t)(((uint32_t)(x)) << FTM_OUTMASK_CH0OM_SHIFT)) & FTM_OUTMASK_CH0OM_MASK)
#define FTM_OUTMASK_CH1OM_MASK                   (0x2U)
#define FTM_OUTMASK_CH1OM_SHIFT                  (1U)
#define FTM_OUTMASK_CH1OM(x)                     (((uint32_t)(((uint32_t)(x)) << FTM_OUTMASK_CH1OM_SHIFT)) & FTM_OUTMASK_CH1OM_MASK)
#define FTM_OUTMASK_CH2OM_MASK                   (0x4U)
#define FTM_OUTMASK_CH2OM_SHIFT                  (2U)
#define FTM_OUTMASK_CH2OM(x)                     (((uint32_t)(((uint32_t)(x)) << FTM_OUTMASK_CH2OM_SHIFT)) & FTM_OUTMASK_CH2OM_MASK)
#define FTM_OUTMASK_CH3OM_MASK                   (0x8U)
#define FTM_OUTMASK_CH3OM_SHIFT                  (3U)
#define FTM_OUTMASK_CH3OM(x)                     (((uint32_t)(((uint32_t)(x)) << FTM_OUTMASK_CH3OM_SHIFT)) & FTM_OUTMASK_CH3OM_MASK)
#define FTM_OUTMASK_CH4OM_MASK                   (0x10U)
#define FTM_OUTMASK_CH4OM_SHIFT                  (4U)
#define FTM_OUTMASK_CH4OM(x)                     (((uint32_t)(((uint32_t)(x)) << FTM_OUTMASK_CH4OM_SHIFT)) & FTM_OUTMASK_CH4OM_MASK)
#define FTM_OUTMASK_CH5OM_MASK                   (0x20U)
#define FTM_OUTMASK_CH5OM_SHIFT                  (5U)
#define FTM_OUTMASK_CH5OM(x)                     (((uint32_t)(((uint32_t)(x)) << FTM_OUTMASK_CH5OM_SHIFT)) & FTM_OUTMASK_CH5OM_MASK)
#define FTM_OUTMASK_CH6OM_MASK                   (0x40U)
#define FTM_OUTMASK_CH6OM_SHIFT                  (6U)
#define FTM_OUTMASK_CH6OM(x)                     (((uint32_t)(((uint32_t)(x)) << FTM_OUTMASK_CH6OM_SHIFT)) & FTM_OUTMASK_CH6OM_MASK)
#define FTM_OUTMASK_CH7OM_MASK                   (0x80U)
#define FTM_OUTMASK_CH7OM_SHIFT                  (7U)
#define FTM_OUTMASK_CH7OM(x)                     (((uint32_t)(((uint32_t)(x)) << FTM_OUTMASK_CH7OM_SHIFT)) & FTM_OUTMASK_CH7OM_MASK)

/*! @name COMBINE - Function For Linked Channels */
#define FTM_COMBINE_COMBINE0_MASK                (0x1U)
#define FTM_COMBINE_COMBINE0_SHIFT               (0U)
#define FTM_COMBINE_COMBINE0(x)                  (((uint32_t)(((uint32_t)(x)) << FTM_COMBINE_COMBINE0_SHIFT)) & FTM_COMBINE_COMBINE0_MASK)
#define FTM_COMBINE_COMP0_MASK                   (0x2U)
#define FTM_COMBINE_COMP0_SHIFT                  (1U)
#define FTM_COMBINE_COMP0(x)                     (((uint32_t)(((uint32_t)(x)) << FTM_COMBINE_COMP0_SHIFT)) & FTM_COMBINE_COMP0_MASK)
#define FTM_COMBINE_DECAPEN0_MASK                (0x4U)
#define FTM_COMBINE_DECAPEN0_SHIFT               (2U)
#define FTM_COMBINE_DECAPEN0(x)                  (((uint32_t)(((uint32_t)(x)) << FTM_COMBINE_DECAPEN0_SHIFT)) & FTM_COMBINE_DECAPEN0_MASK)
#define FTM_COMBINE_DECAP0_MASK                  (0x8U)
#define FTM_COMBINE_DECAP0_SHIFT                 (3U)
#define FTM_COMBINE_DECAP0(x)                    (((uint32_t)(((uint32_t)(x)) << FTM_COMBINE_DECAP0_SHIFT)) & FTM_COMBINE_DECAP0_MASK)
#define FTM_COMBINE_DTEN0_MASK                   (0x10U)
#define FTM_COMBINE_DTEN0_SHIFT                  (4U)
#define FTM_COMBINE_DTEN0(x)                     (((uint32_t)(((uint32_t)(x)) << FTM_COMBINE_DTEN0_SHIFT)) & FTM_COMBINE_DTEN0_MASK)
#define FTM_COMBINE_SYNCEN0_MASK                 (0x20U)
#define FTM_COMBINE_SYNCEN0_SHIFT                (5U)
#define FTM_COMBINE_SYNCEN0(x)                   (((uint32_t)(((uint32_t)(x)) << FTM_COMBINE_SYNCEN0_SHIFT)) & FTM_COMBINE_SYNCEN0_MASK)
#define FTM_COMBINE_FAULTEN0_MASK                (0x40U)
#define FTM_COMBINE_FAULTEN0_SHIFT               (6U)
#define FTM_COMBINE_FAULTEN0(x)                  (((uint32_t)(((uint32_t)(x)) << FTM_COMBINE_FAULTEN0_SHIFT)) & FTM_COMBINE_FAULTEN0_MASK)
#define FTM_COMBINE_COMBINE1_MASK                (0x100U)
#define FTM_COMBINE_COMBINE1_SHIFT               (8U)
#define FTM_COMBINE_COMBINE1(x)                  (((uint32_t)(((uint32_t)(x)) << FTM_COMBINE_COMBINE1_SHIFT)) & FTM_COMBINE_COMBINE1_MASK)
#define FTM_COMBINE_COMP1_MASK                   (0x200U)
#define FTM_COMBINE_COMP1_SHIFT                  (9U)
#define FTM_COMBINE_COMP1(x)                     (((uint32_t)(((uint32_t)(x)) << FTM_COMBINE_COMP1_SHIFT)) & FTM_COMBINE_COMP1_MASK)
#define FTM_COMBINE_DECAPEN1_MASK                (0x400U)
#define FTM_COMBINE_DECAPEN1_SHIFT               (10U)
#define FTM_COMBINE_DECAPEN1(x)                  (((uint32_t)(((uint32_t)(x)) << FTM_COMBINE_DECAPEN1_SHIFT)) & FTM_COMBINE_DECAPEN1_MASK)
#define FTM_COMBINE_DECAP1_MASK                  (0x800U)
#define FTM_COMBINE_DECAP1_SHIFT                 (11U)
#define FTM_COMBINE_DECAP1(x)                    (((uint32_t)(((uint32_t)(x)) << FTM_COMBINE_DECAP1_SHIFT)) & FTM_COMBINE_DECAP1_MASK)
#define FTM_COMBINE_DTEN1_MASK                   (0x1000U)
#define FTM_COMBINE_DTEN1_SHIFT                  (12U)
#define FTM_COMBINE_DTEN1(x)                     (((uint32_t)(((uint32_t)(x)) << FTM_COMBINE_DTEN1_SHIFT)) & FTM_COMBINE_DTEN1_MASK)
#define FTM_COMBINE_SYNCEN1_MASK                 (0x2000U)
#define FTM_COMBINE_SYNCEN1_SHIFT                (13U)
#define FTM_COMBINE_SYNCEN1(x)                   (((uint32_t)(((uint32_t)(x)) << FTM_COMBINE_SYNCEN1_SHIFT)) & FTM_COMBINE_SYNCEN1_MASK)
#define FTM_COMBINE_FAULTEN1_MASK                (0x4000U)
#define FTM_COMBINE_FAULTEN1_SHIFT               (14U)
#define FTM_COMBINE_FAULTEN1(x)                  (((uint32_t)(((uint32_t)(x)) << FTM_COMBINE_FAULTEN1_SHIFT)) & FTM_COMBINE_FAULTEN1_MASK)
#define FTM_COMBINE_COMBINE2_MASK                (0x10000U)
#define FTM_COMBINE_COMBINE2_SHIFT               (16U)
#define FTM_COMBINE_COMBINE2(x)                  (((uint32_t)(((uint32_t)(x)) << FTM_COMBINE_COMBINE2_SHIFT)) & FTM_COMBINE_COMBINE2_MASK)
#define FTM_COMBINE_COMP2_MASK                   (0x20000U)
#define FTM_COMBINE_COMP2_SHIFT                  (17U)
#define FTM_COMBINE_COMP2(x)                     (((uint32_t)(((uint32_t)(x)) << FTM_COMBINE_COMP2_SHIFT)) & FTM_COMBINE_COMP2_MASK)
#define FTM_COMBINE_DECAPEN2_MASK                (0x40000U)
#define FTM_COMBINE_DECAPEN2_SHIFT               (18U)
#define FTM_COMBINE_DECAPEN2(x)                  (((uint32_t)(((uint32_t)(x)) << FTM_COMBINE_DECAPEN2_SHIFT)) & FTM_COMBINE_DECAPEN2_MASK)
#define FTM_COMBINE_DECAP2_MASK                  (0x80000U)
#define FTM_COMBINE_DECAP2_SHIFT                 (19U)
#define FTM_COMBINE_DECAP2(x)                    (((uint32_t)(((uint32_t)(x)) << FTM_COMBINE_DECAP2_SHIFT)) & FTM_COMBINE_DECAP2_MASK)
#define FTM_COMBINE_DTEN2_MASK                   (0x100000U)
#define FTM_COMBINE_DTEN2_SHIFT                  (20U)
#define FTM_COMBINE_DTEN2(x)                     (((uint32_t)(((uint32_t)(x)) << FTM_COMBINE_DTEN2_SHIFT)) & FTM_COMBINE_DTEN2_MASK)
#define FTM_COMBINE_SYNCEN2_MASK                 (0x200000U)
#define FTM_COMBINE_SYNCEN2_SHIFT                (21U)
#define FTM_COMBINE_SYNCEN2(x)                   (((uint32_t)(((uint32_t)(x)) << FTM_COMBINE_SYNCEN2_SHIFT)) & FTM_COMBINE_SYNCEN2_MASK)
#define FTM_COMBINE_FAULTEN2_MASK                (0x400000U)
#define FTM_COMBINE_FAULTEN2_SHIFT               (22U)
#define FTM_COMBINE_FAULTEN2(x)                  (((uint32_t)(((uint32_t)(x)) << FTM_COMBINE_FAULTEN2_SHIFT)) & FTM_COMBINE_FAULTEN2_MASK)
#define FTM_COMBINE_COMBINE3_MASK                (0x1000000U)
#define FTM_COMBINE_COMBINE3_SHIFT               (24U)
#define FTM_COMBINE_COMBINE3(x)                  (((uint32_t)(((uint32_t)(x)) << FTM_COMBINE_COMBINE3_SHIFT)) & FTM_COMBINE_COMBINE3_MASK)
#define FTM_COMBINE_COMP3_MASK                   (0x2000000U)
#define FTM_COMBINE_COMP3_SHIFT                  (25U)
#define FTM_COMBINE_COMP3(x)                     (((uint32_t)(((uint32_t)(x)) << FTM_COMBINE_COMP3_SHIFT)) & FTM_COMBINE_COMP3_MASK)
#define FTM_COMBINE_DECAPEN3_MASK                (0x4000000U)
#define FTM_COMBINE_DECAPEN3_SHIFT               (26U)
#define FTM_COMBINE_DECAPEN3(x)                  (((uint32_t)(((uint32_t)(x)) << FTM_COMBINE_DECAPEN3_SHIFT)) & FTM_COMBINE_DECAPEN3_MASK)
#define FTM_COMBINE_DECAP3_MASK                  (0x8000000U)
#define FTM_COMBINE_DECAP3_SHIFT                 (27U)
#define FTM_COMBINE_DECAP3(x)                    (((uint32_t)(((uint32_t)(x)) << FTM_COMBINE_DECAP3_SHIFT)) & FTM_COMBINE_DECAP3_MASK)
#define FTM_COMBINE_DTEN3_MASK                   (0x10000000U)
#define FTM_COMBINE_DTEN3_SHIFT                  (28U)
#define FTM_COMBINE_DTEN3(x)                     (((uint32_t)(((uint32_t)(x)) << FTM_COMBINE_DTEN3_SHIFT)) & FTM_COMBINE_DTEN3_MASK)
#define FTM_COMBINE_SYNCEN3_MASK                 (0x20000000U)
#define FTM_COMBINE_SYNCEN3_SHIFT                (29U)
#define FTM_COMBINE_SYNCEN3(x)                   (((uint32_t)(((uint32_t)(x)) << FTM_COMBINE_SYNCEN3_SHIFT)) & FTM_COMBINE_SYNCEN3_MASK)
#define FTM_COMBINE_FAULTEN3_MASK                (0x40000000U)
#define FTM_COMBINE_FAULTEN3_SHIFT               (30U)
#define FTM_COMBINE_FAULTEN3(x)                  (((uint32_t)(((uint32_t)(x)) << FTM_COMBINE_FAULTEN3_SHIFT)) & FTM_COMBINE_FAULTEN3_MASK)

/*! @name DEADTIME - Deadtime Insertion Control */
#define FTM_DEADTIME_DTVAL_MASK                  (0x3FU)
#define FTM_DEADTIME_DTVAL_SHIFT                 (0U)
#define FTM_DEADTIME_DTVAL(x)                    (((uint32_t)(((uint32_t)(x)) << FTM_DEADTIME_DTVAL_SHIFT)) & FTM_DEADTIME_DTVAL_MASK)
#define FTM_DEADTIME_DTPS_MASK                   (0xC0U)
#define FTM_DEADTIME_DTPS_SHIFT                  (6U)
#define FTM_DEADTIME_DTPS(x)                     (((uint32_t)(((uint32_t)(x)) << FTM_DEADTIME_DTPS_SHIFT)) & FTM_DEADTIME_DTPS_MASK)

/*! @name EXTTRIG - FTM External Trigger */
#define FTM_EXTTRIG_CH2TRIG_MASK                 (0x1U)
#define FTM_EXTTRIG_CH2TRIG_SHIFT                (0U)
#define FTM_EXTTRIG_CH2TRIG(x)                   (((uint32_t)(((uint32_t)(x)) << FTM_EXTTRIG_CH2TRIG_SHIFT)) & FTM_EXTTRIG_CH2TRIG_MASK)
#define FTM_EXTTRIG_CH3TRIG_MASK                 (0x2U)
#define FTM_EXTTRIG_CH3TRIG_SHIFT                (1U)
#define FTM_EXTTRIG_CH3TRIG(x)                   (((uint32_t)(((uint32_t)(x)) << FTM_EXTTRIG_CH3TRIG_SHIFT)) & FTM_EXTTRIG_CH3TRIG_MASK)
#define FTM_EXTTRIG_CH4TRIG_MASK                 (0x4U)
#define FTM_EXTTRIG_CH4TRIG_SHIFT                (2U)
#define FTM_EXTTRIG_CH4TRIG(x)                   (((uint32_t)(((uint32_t)(x)) << FTM_EXTTRIG_CH4TRIG_SHIFT)) & FTM_EXTTRIG_CH4TRIG_MASK)
#define FTM_EXTTRIG_CH5TRIG_MASK                 (0x8U)
#define FTM_EXTTRIG_CH5TRIG_SHIFT                (3U)
#define FTM_EXTTRIG_CH5TRIG(x)                   (((uint32_t)(((uint32_t)(x)) << FTM_EXTTRIG_CH5TRIG_SHIFT)) & FTM_EXTTRIG_CH5TRIG_MASK)
#define FTM_EXTTRIG_CH0TRIG_MASK                 (0x10U)
#define FTM_EXTTRIG_CH0TRIG_SHIFT                (4U)
#define FTM_EXTTRIG_CH0TRIG(x)                   (((uint32_t)(((uint32_t)(x)) << FTM_EXTTRIG_CH0TRIG_SHIFT)) & FTM_EXTTRIG_CH0TRIG_MASK)
#define FTM_EXTTRIG_CH1TRIG_MASK                 (0x20U)
#define FTM_EXTTRIG_CH1TRIG_SHIFT                (5U)
#define FTM_EXTTRIG_CH1TRIG(x)                   (((uint32_t)(((uint32_t)(x)) << FTM_EXTTRIG_CH1TRIG_SHIFT)) & FTM_EXTTRIG_CH1TRIG_MASK)
#define FTM_EXTTRIG_INITTRIGEN_MASK              (0x40U)
#define FTM_EXTTRIG_INITTRIGEN_SHIFT             (6U)
#define FTM_EXTTRIG_INITTRIGEN(x)                (((uint32_t)(((uint32_t)(x)) << FTM_EXTTRIG_INITTRIGEN_SHIFT)) & FTM_EXTTRIG_INITTRIGEN_MASK)
#define FTM_EXTTRIG_TRIGF_MASK                   (0x80U)
#define FTM_EXTTRIG_TRIGF_SHIFT                  (7U)
#define FTM_EXTTRIG_TRIGF(x)                     (((uint32_t)(((uint32_t)(x)) << FTM_EXTTRIG_TRIGF_SHIFT)) & FTM_EXTTRIG_TRIGF_MASK)

/*! @name POL - Channels Polarity */
#define FTM_POL_POL0_MASK                        (0x1U)
#define FTM_POL_POL0_SHIFT                       (0U)
#define FTM_POL_POL0(x)                          (((uint32_t)(((uint32_t)(x)) << FTM_POL_POL0_SHIFT)) & FTM_POL_POL0_MASK)
#define FTM_POL_POL1_MASK                        (0x2U)
#define FTM_POL_POL1_SHIFT                       (1U)
#define FTM_POL_POL1(x)                          (((uint32_t)(((uint32_t)(x)) << FTM_POL_POL1_SHIFT)) & FTM_POL_POL1_MASK)
#define FTM_POL_POL2_MASK                        (0x4U)
#define FTM_POL_POL2_SHIFT                       (2U)
#define FTM_POL_POL2(x)                          (((uint32_t)(((uint32_t)(x)) << FTM_POL_POL2_SHIFT)) & FTM_POL_POL2_MASK)
#define FTM_POL_POL3_MASK                        (0x8U)
#define FTM_POL_POL3_SHIFT                       (3U)
#define FTM_POL_POL3(x)                          (((uint32_t)(((uint32_t)(x)) << FTM_POL_POL3_SHIFT)) & FTM_POL_POL3_MASK)
#define FTM_POL_POL4_MASK                        (0x10U)
#define FTM_POL_POL4_SHIFT                       (4U)
#define FTM_POL_POL4(x)                          (((uint32_t)(((uint32_t)(x)) << FTM_POL_POL4_SHIFT)) & FTM_POL_POL4_MASK)
#define FTM_POL_POL5_MASK                        (0x20U)
#define FTM_POL_POL5_SHIFT                       (5U)
#define FTM_POL_POL5(x)                          (((uint32_t)(((uint32_t)(x)) << FTM_POL_POL5_SHIFT)) & FTM_POL_POL5_MASK)
#define FTM_POL_POL6_MASK                        (0x40U)
#define FTM_POL_POL6_SHIFT                       (6U)
#define FTM_POL_POL6(x)                          (((uint32_t)(((uint32_t)(x)) << FTM_POL_POL6_SHIFT)) & FTM_POL_POL6_MASK)
#define FTM_POL_POL7_MASK                        (0x80U)
#define FTM_POL_POL7_SHIFT                       (7U)
#define FTM_POL_POL7(x)                          (((uint32_t)(((uint32_t)(x)) << FTM_POL_POL7_SHIFT)) & FTM_POL_POL7_MASK)

/*! @name FMS - Fault Mode Status */
#define FTM_FMS_FAULTF0_MASK                     (0x1U)
#define FTM_FMS_FAULTF0_SHIFT                    (0U)
#define FTM_FMS_FAULTF0(x)                       (((uint32_t)(((uint32_t)(x)) << FTM_FMS_FAULTF0_SHIFT)) & FTM_FMS_FAULTF0_MASK)
#define FTM_FMS_FAULTF1_MASK                     (0x2U)
#define FTM_FMS_FAULTF1_SHIFT                    (1U)
#define FTM_FMS_FAULTF1(x)                       (((uint32_t)(((uint32_t)(x)) << FTM_FMS_FAULTF1_SHIFT)) & FTM_FMS_FAULTF1_MASK)
#define FTM_FMS_FAULTF2_MASK                     (0x4U)
#define FTM_FMS_FAULTF2_SHIFT                    (2U)
#define FTM_FMS_FAULTF2(x)                       (((uint32_t)(((uint32_t)(x)) << FTM_FMS_FAULTF2_SHIFT)) & FTM_FMS_FAULTF2_MASK)
#define FTM_FMS_FAULTF3_MASK                     (0x8U)
#define FTM_FMS_FAULTF3_SHIFT                    (3U)
#define FTM_FMS_FAULTF3(x)                       (((uint32_t)(((uint32_t)(x)) << FTM_FMS_FAULTF3_SHIFT)) & FTM_FMS_FAULTF3_MASK)
#define FTM_FMS_FAULTIN_MASK                     (0x20U)
#define FTM_FMS_FAULTIN_SHIFT                    (5U)
#define FTM_FMS_FAULTIN(x)                       (((uint32_t)(((uint32_t)(x)) << FTM_FMS_FAULTIN_SHIFT)) & FTM_FMS_FAULTIN_MASK)
#define FTM_FMS_WPEN_MASK                        (0x40U)
#define FTM_FMS_WPEN_SHIFT                       (6U)
#define FTM_FMS_WPEN(x)                          (((uint32_t)(((uint32_t)(x)) << FTM_FMS_WPEN_SHIFT)) & FTM_FMS_WPEN_MASK)
#define FTM_FMS_FAULTF_MASK                      (0x80U)
#define FTM_FMS_FAULTF_SHIFT                     (7U)
#define FTM_FMS_FAULTF(x)                        (((uint32_t)(((uint32_t)(x)) << FTM_FMS_FAULTF_SHIFT)) & FTM_FMS_FAULTF_MASK)

/*! @name FILTER - Input Capture Filter Control */
#define FTM_FILTER_CH0FVAL_MASK                  (0xFU)
#define FTM_FILTER_CH0FVAL_SHIFT                 (0U)
#define FTM_FILTER_CH0FVAL(x)                    (((uint32_t)(((uint32_t)(x)) << FTM_FILTER_CH0FVAL_SHIFT)) & FTM_FILTER_CH0FVAL_MASK)
#define FTM_FILTER_CH1FVAL_MASK                  (0xF0U)
#define FTM_FILTER_CH1FVAL_SHIFT                 (4U)
#define FTM_FILTER_CH1FVAL(x)                    (((uint32_t)(((uint32_t)(x)) << FTM_FILTER_CH1FVAL_SHIFT)) & FTM_FILTER_CH1FVAL_MASK)
#define FTM_FILTER_CH2FVAL_MASK                  (0xF00U)
#define FTM_FILTER_CH2FVAL_SHIFT                 (8U)
#define FTM_FILTER_CH2FVAL(x)                    (((uint32_t)(((uint32_t)(x)) << FTM_FILTER_CH2FVAL_SHIFT)) & FTM_FILTER_CH2FVAL_MASK)
#define FTM_FILTER_CH3FVAL_MASK                  (0xF000U)
#define FTM_FILTER_CH3FVAL_SHIFT                 (12U)
#define FTM_FILTER_CH3FVAL(x)                    (((uint32_t)(((uint32_t)(x)) << FTM_FILTER_CH3FVAL_SHIFT)) & FTM_FILTER_CH3FVAL_MASK)

/*! @name FLTCTRL - Fault Control */
#define FTM_FLTCTRL_FAULT0EN_MASK                (0x1U)
#define FTM_FLTCTRL_FAULT0EN_SHIFT               (0U)
#define FTM_FLTCTRL_FAULT0EN(x)                  (((uint32_t)(((uint32_t)(x)) << FTM_FLTCTRL_FAULT0EN_SHIFT)) & FTM_FLTCTRL_FAULT0EN_MASK)
#define FTM_FLTCTRL_FAULT1EN_MASK                (0x2U)
#define FTM_FLTCTRL_FAULT1EN_SHIFT               (1U)
#define FTM_FLTCTRL_FAULT1EN(x)                  (((uint32_t)(((uint32_t)(x)) << FTM_FLTCTRL_FAULT1EN_SHIFT)) & FTM_FLTCTRL_FAULT1EN_MASK)
#define FTM_FLTCTRL_FAULT2EN_MASK                (0x4U)
#define FTM_FLTCTRL_FAULT2EN_SHIFT               (2U)
#define FTM_FLTCTRL_FAULT2EN(x)                  (((uint32_t)(((uint32_t)(x)) << FTM_FLTCTRL_FAULT2EN_SHIFT)) & FTM_FLTCTRL_FAULT2EN_MASK)
#define FTM_FLTCTRL_FAULT3EN_MASK                (0x8U)
#define FTM_FLTCTRL_FAULT3EN_SHIFT               (3U)
#define FTM_FLTCTRL_FAULT3EN(x)                  (((uint32_t)(((uint32_t)(x)) << FTM_FLTCTRL_FAULT3EN_SHIFT)) & FTM_FLTCTRL_FAULT3EN_MASK)
#define FTM_FLTCTRL_FFLTR0EN_MASK                (0x10U)
#define FTM_FLTCTRL_FFLTR0EN_SHIFT               (4U)
#define FTM_FLTCTRL_FFLTR0EN(x)                  (((uint32_t)(((uint32_t)(x)) << FTM_FLTCTRL_FFLTR0EN_SHIFT)) & FTM_FLTCTRL_FFLTR0EN_MASK)
#define FTM_FLTCTRL_FFLTR1EN_MASK                (0x20U)
#define FTM_FLTCTRL_FFLTR1EN_SHIFT               (5U)
#define FTM_FLTCTRL_FFLTR1EN(x)                  (((uint32_t)(((uint32_t)(x)) << FTM_FLTCTRL_FFLTR1EN_SHIFT)) & FTM_FLTCTRL_FFLTR1EN_MASK)
#define FTM_FLTCTRL_FFLTR2EN_MASK                (0x40U)
#define FTM_FLTCTRL_FFLTR2EN_SHIFT               (6U)
#define FTM_FLTCTRL_FFLTR2EN(x)                  (((uint32_t)(((uint32_t)(x)) << FTM_FLTCTRL_FFLTR2EN_SHIFT)) & FTM_FLTCTRL_FFLTR2EN_MASK)
#define FTM_FLTCTRL_FFLTR3EN_MASK                (0x80U)
#define FTM_FLTCTRL_FFLTR3EN_SHIFT               (7U)
#define FTM_FLTCTRL_FFLTR3EN(x)                  (((uint32_t)(((uint32_t)(x)) << FTM_FLTCTRL_FFLTR3EN_SHIFT)) & FTM_FLTCTRL_FFLTR3EN_MASK)
#define FTM_FLTCTRL_FFVAL_MASK                   (0xF00U)
#define FTM_FLTCTRL_FFVAL_SHIFT                  (8U)
#define FTM_FLTCTRL_FFVAL(x)                     (((uint32_t)(((uint32_t)(x)) << FTM_FLTCTRL_FFVAL_SHIFT)) & FTM_FLTCTRL_FFVAL_MASK)

/*! @name QDCTRL - Quadrature Decoder Control And Status */
#define FTM_QDCTRL_QUADEN_MASK                   (0x1U)
#define FTM_QDCTRL_QUADEN_SHIFT                  (0U)
#define FTM_QDCTRL_QUADEN(x)                     (((uint32_t)(((uint32_t)(x)) << FTM_QDCTRL_QUADEN_SHIFT)) & FTM_QDCTRL_QUADEN_MASK)
#define FTM_QDCTRL_TOFDIR_MASK                   (0x2U)
#define FTM_QDCTRL_TOFDIR_SHIFT                  (1U)
#define FTM_QDCTRL_TOFDIR(x)                     (((uint32_t)(((uint32_t)(x)) << FTM_QDCTRL_TOFDIR_SHIFT)) & FTM_QDCTRL_TOFDIR_MASK)
#define FTM_QDCTRL_QUADIR_MASK                   (0x4U)
#define FTM_QDCTRL_QUADIR_SHIFT                  (2U)
#define FTM_QDCTRL_QUADIR(x)                     (((uint32_t)(((uint32_t)(x)) << FTM_QDCTRL_QUADIR_SHIFT)) & FTM_QDCTRL_QUADIR_MASK)
#define FTM_QDCTRL_QUADMODE_MASK                 (0x8U)
#define FTM_QDCTRL_QUADMODE_SHIFT                (3U)
#define FTM_QDCTRL_QUADMODE(x)                   (((uint32_t)(((uint32_t)(x)) << FTM_QDCTRL_QUADMODE_SHIFT)) & FTM_QDCTRL_QUADMODE_MASK)
#define FTM_QDCTRL_PHBPOL_MASK                   (0x10U)
#define FTM_QDCTRL_PHBPOL_SHIFT                  (4U)
#define FTM_QDCTRL_PHBPOL(x)                     (((uint32_t)(((uint32_t)(x)) << FTM_QDCTRL_PHBPOL_SHIFT)) & FTM_QDCTRL_PHBPOL_MASK)
#define FTM_QDCTRL_PHAPOL_MASK                   (0x20U)
#define FTM_QDCTRL_PHAPOL_SHIFT                  (5U)
#define FTM_QDCTRL_PHAPOL(x)                     (((uint32_t)(((uint32_t)(x)) << FTM_QDCTRL_PHAPOL_SHIFT)) & FTM_QDCTRL_PHAPOL_MASK)
#define FTM_QDCTRL_PHBFLTREN_MASK                (0x40U)
#define FTM_QDCTRL_PHBFLTREN_SHIFT               (6U)
#define FTM_QDCTRL_PHBFLTREN(x)                  (((uint32_t)(((uint32_t)(x)) << FTM_QDCTRL_PHBFLTREN_SHIFT)) & FTM_QDCTRL_PHBFLTREN_MASK)
#define FTM_QDCTRL_PHAFLTREN_MASK                (0x80U)
#define FTM_QDCTRL_PHAFLTREN_SHIFT               (7U)
#define FTM_QDCTRL_PHAFLTREN(x)                  (((uint32_t)(((uint32_t)(x)) << FTM_QDCTRL_PHAFLTREN_SHIFT)) & FTM_QDCTRL_PHAFLTREN_MASK)

/*! @name CONF - Configuration */
#define FTM_CONF_NUMTOF_MASK                     (0x1FU)
#define FTM_CONF_NUMTOF_SHIFT                    (0U)
#define FTM_CONF_NUMTOF(x)                       (((uint32_t)(((uint32_t)(x)) << FTM_CONF_NUMTOF_SHIFT)) & FTM_CONF_NUMTOF_MASK)
#define FTM_CONF_BDMMODE_MASK                    (0xC0U)
#define FTM_CONF_BDMMODE_SHIFT                   (6U)
#define FTM_CONF_BDMMODE(x)                      (((uint32_t)(((uint32_t)(x)) << FTM_CONF_BDMMODE_SHIFT)) & FTM_CONF_BDMMODE_MASK)
#define FTM_CONF_GTBEEN_MASK                     (0x200U)
#define FTM_CONF_GTBEEN_SHIFT                    (9U)
#define FTM_CONF_GTBEEN(x)                       (((uint32_t)(((uint32_t)(x)) << FTM_CONF_GTBEEN_SHIFT)) & FTM_CONF_GTBEEN_MASK)
#define FTM_CONF_GTBEOUT_MASK                    (0x400U)
#define FTM_CONF_GTBEOUT_SHIFT                   (10U)
#define FTM_CONF_GTBEOUT(x)                      (((uint32_t)(((uint32_t)(x)) << FTM_CONF_GTBEOUT_SHIFT)) & FTM_CONF_GTBEOUT_MASK)

/*! @name FLTPOL - FTM Fault Input Polarity */
#define FTM_FLTPOL_FLT0POL_MASK                  (0x1U)
#define FTM_FLTPOL_FLT0POL_SHIFT                 (0U)
#define FTM_FLTPOL_FLT0POL(x)                    (((uint32_t)(((uint32_t)(x)) << FTM_FLTPOL_FLT0POL_SHIFT)) & FTM_FLTPOL_FLT0POL_MASK)
#define FTM_FLTPOL_FLT1POL_MASK                  (0x2U)
#define FTM_FLTPOL_FLT1POL_SHIFT                 (1U)
#define FTM_FLTPOL_FLT1POL(x)                    (((uint32_t)(((uint32_t)(x)) << FTM_FLTPOL_FLT1POL_SHIFT)) & FTM_FLTPOL_FLT1POL_MASK)
#define FTM_FLTPOL_FLT2POL_MASK                  (0x4U)
#define FTM_FLTPOL_FLT2POL_SHIFT                 (2U)
#define FTM_FLTPOL_FLT2POL(x)                    (((uint32_t)(((uint32_t)(x)) << FTM_FLTPOL_FLT2POL_SHIFT)) & FTM_FLTPOL_FLT2POL_MASK)
#define FTM_FLTPOL_FLT3POL_MASK                  (0x8U)
#define FTM_FLTPOL_FLT3POL_SHIFT                 (3U)
#define FTM_FLTPOL_FLT3POL(x)                    (((uint32_t)(((uint32_t)(x)) << FTM_FLTPOL_FLT3POL_SHIFT)) & FTM_FLTPOL_FLT3POL_MASK)

/*! @name SYNCONF - Synchronization Configuration */
#define FTM_SYNCONF_HWTRIGMODE_MASK              (0x1U)
#define FTM_SYNCONF_HWTRIGMODE_SHIFT             (0U)
#define FTM_SYNCONF_HWTRIGMODE(x)                (((uint32_t)(((uint32_t)(x)) << FTM_SYNCONF_HWTRIGMODE_SHIFT)) & FTM_SYNCONF_HWTRIGMODE_MASK)
#define FTM_SYNCONF_CNTINC_MASK                  (0x4U)
#define FTM_SYNCONF_CNTINC_SHIFT                 (2U)
#define FTM_SYNCONF_CNTINC(x)                    (((uint32_t)(((uint32_t)(x)) << FTM_SYNCONF_CNTINC_SHIFT)) & FTM_SYNCONF_CNTINC_MASK)
#define FTM_SYNCONF_INVC_MASK                    (0x10U)
#define FTM_SYNCONF_INVC_SHIFT                   (4U)
#define FTM_SYNCONF_INVC(x)                      (((uint32_t)(((uint32_t)(x)) << FTM_SYNCONF_INVC_SHIFT)) & FTM_SYNCONF_INVC_MASK)
#define FTM_SYNCONF_SWOC_MASK                    (0x20U)
#define FTM_SYNCONF_SWOC_SHIFT                   (5U)
#define FTM_SYNCONF_SWOC(x)                      (((uint32_t)(((uint32_t)(x)) << FTM_SYNCONF_SWOC_SHIFT)) & FTM_SYNCONF_SWOC_MASK)
#define FTM_SYNCONF_SYNCMODE_MASK                (0x80U)
#define FTM_SYNCONF_SYNCMODE_SHIFT               (7U)
#define FTM_SYNCONF_SYNCMODE(x)                  (((uint32_t)(((uint32_t)(x)) << FTM_SYNCONF_SYNCMODE_SHIFT)) & FTM_SYNCONF_SYNCMODE_MASK)
#define FTM_SYNCONF_SWRSTCNT_MASK                (0x100U)
#define FTM_SYNCONF_SWRSTCNT_SHIFT               (8U)
#define FTM_SYNCONF_SWRSTCNT(x)                  (((uint32_t)(((uint32_t)(x)) << FTM_SYNCONF_SWRSTCNT_SHIFT)) & FTM_SYNCONF_SWRSTCNT_MASK)
#define FTM_SYNCONF_SWWRBUF_MASK                 (0x200U)
#define FTM_SYNCONF_SWWRBUF_SHIFT                (9U)
#define FTM_SYNCONF_SWWRBUF(x)                   (((uint32_t)(((uint32_t)(x)) << FTM_SYNCONF_SWWRBUF_SHIFT)) & FTM_SYNCONF_SWWRBUF_MASK)
#define FTM_SYNCONF_SWOM_MASK                    (0x400U)
#define FTM_SYNCONF_SWOM_SHIFT                   (10U)
#define FTM_SYNCONF_SWOM(x)                      (((uint32_t)(((uint32_t)(x)) << FTM_SYNCONF_SWOM_SHIFT)) & FTM_SYNCONF_SWOM_MASK)
#define FTM_SYNCONF_SWINVC_MASK                  (0x800U)
#define FTM_SYNCONF_SWINVC_SHIFT                 (11U)
#define FTM_SYNCONF_SWINVC(x)                    (((uint32_t)(((uint32_t)(x)) << FTM_SYNCONF_SWINVC_SHIFT)) & FTM_SYNCONF_SWINVC_MASK)
#define FTM_SYNCONF_SWSOC_MASK                   (0x1000U)
#define FTM_SYNCONF_SWSOC_SHIFT                  (12U)
#define FTM_SYNCONF_SWSOC(x)                     (((uint32_t)(((uint32_t)(x)) << FTM_SYNCONF_SWSOC_SHIFT)) & FTM_SYNCONF_SWSOC_MASK)
#define FTM_SYNCONF_HWRSTCNT_MASK                (0x10000U)
#define FTM_SYNCONF_HWRSTCNT_SHIFT               (16U)
#define FTM_SYNCONF_HWRSTCNT(x)                  (((uint32_t)(((uint32_t)(x)) << FTM_SYNCONF_HWRSTCNT_SHIFT)) & FTM_SYNCONF_HWRSTCNT_MASK)
#define FTM_SYNCONF_HWWRBUF_MASK                 (0x20000U)
#define FTM_SYNCONF_HWWRBUF_SHIFT                (17U)
#define FTM_SYNCONF_HWWRBUF(x)                   (((uint32_t)(((uint32_t)(x)) << FTM_SYNCONF_HWWRBUF_SHIFT)) & FTM_SYNCONF_HWWRBUF_MASK)
#define FTM_SYNCONF_HWOM_MASK                    (0x40000U)
#define FTM_SYNCONF_HWOM_SHIFT                   (18U)
#define FTM_SYNCONF_HWOM(x)                      (((uint32_t)(((uint32_t)(x)) << FTM_SYNCONF_HWOM_SHIFT)) & FTM_SYNCONF_HWOM_MASK)
#define FTM_SYNCONF_HWINVC_MASK                  (0x80000U)
#define FTM_SYNCONF_HWINVC_SHIFT                 (19U)
#define FTM_SYNCONF_HWINVC(x)                    (((uint32_t)(((uint32_t)(x)) << FTM_SYNCONF_HWINVC_SHIFT)) & FTM_SYNCONF_HWINVC_MASK)
#define FTM_SYNCONF_HWSOC_MASK                   (0x100000U)
#define FTM_SYNCONF_HWSOC_SHIFT                  (20U)
#define FTM_SYNCONF_HWSOC(x)                     (((uint32_t)(((uint32_t)(x)) << FTM_SYNCONF_HWSOC_SHIFT)) & FTM_SYNCONF_HWSOC_MASK)

/*! @name INVCTRL - FTM Inverting Control */
#define FTM_INVCTRL_INV0EN_MASK                  (0x1U)
#define FTM_INVCTRL_INV0EN_SHIFT                 (0U)
#define FTM_INVCTRL_INV0EN(x)                    (((uint32_t)(((uint32_t)(x)) << FTM_INVCTRL_INV0EN_SHIFT)) & FTM_INVCTRL_INV0EN_MASK)
#define FTM_INVCTRL_INV1EN_MASK                  (0x2U)
#define FTM_INVCTRL_INV1EN_SHIFT                 (1U)
#define FTM_INVCTRL_INV1EN(x)                    (((uint32_t)(((uint32_t)(x)) << FTM_INVCTRL_INV1EN_SHIFT)) & FTM_INVCTRL_INV1EN_MASK)
#define FTM_INVCTRL_INV2EN_MASK                  (0x4U)
#define FTM_INVCTRL_INV2EN_SHIFT                 (2U)
#define FTM_INVCTRL_INV2EN(x)                    (((uint32_t)(((uint32_t)(x)) << FTM_INVCTRL_INV2EN_SHIFT)) & FTM_INVCTRL_INV2EN_MASK)
#define FTM_INVCTRL_INV3EN_MASK                  (0x8U)
#define FTM_INVCTRL_INV3EN_SHIFT                 (3U)
#define FTM_INVCTRL_INV3EN(x)                    (((uint32_t)(((uint32_t)(x)) << FTM_INVCTRL_INV3EN_SHIFT)) & FTM_INVCTRL_INV3EN_MASK)

/*! @name SWOCTRL - FTM Software Output Control */
#define FTM_SWOCTRL_CH0OC_MASK                   (0x1U)
#define FTM_SWOCTRL_CH0OC_SHIFT                  (0U)
#define FTM_SWOCTRL_CH0OC(x)                     (((uint32_t)(((uint32_t)(x)) << FTM_SWOCTRL_CH0OC_SHIFT)) & FTM_SWOCTRL_CH0OC_MASK)
#define FTM_SWOCTRL_CH1OC_MASK                   (0x2U)
#define FTM_SWOCTRL_CH1OC_SHIFT                  (1U)
#define FTM_SWOCTRL_CH1OC(x)                     (((uint32_t)(((uint32_t)(x)) << FTM_SWOCTRL_CH1OC_SHIFT)) & FTM_SWOCTRL_CH1OC_MASK)
#define FTM_SWOCTRL_CH2OC_MASK                   (0x4U)
#define FTM_SWOCTRL_CH2OC_SHIFT                  (2U)
#define FTM_SWOCTRL_CH2OC(x)                     (((uint32_t)(((uint32_t)(x)) << FTM_SWOCTRL_CH2OC_SHIFT)) & FTM_SWOCTRL_CH2OC_MASK)
#define FTM_SWOCTRL_CH3OC_MASK                   (0x8U)
#define FTM_SWOCTRL_CH3OC_SHIFT                  (3U)
#define FTM_SWOCTRL_CH3OC(x)                     (((uint32_t)(((uint32_t)(x)) << FTM_SWOCTRL_CH3OC_SHIFT)) & FTM_SWOCTRL_CH3OC_MASK)
#define FTM_SWOCTRL_CH4OC_MASK                   (0x10U)
#define FTM_SWOCTRL_CH4OC_SHIFT                  (4U)
#define FTM_SWOCTRL_CH4OC(x)                     (((uint32_t)(((uint32_t)(x)) << FTM_SWOCTRL_CH4OC_SHIFT)) & FTM_SWOCTRL_CH4OC_MASK)
#define FTM_SWOCTRL_CH5OC_MASK                   (0x20U)
#define FTM_SWOCTRL_CH5OC_SHIFT                  (5U)
#define FTM_SWOCTRL_CH5OC(x)                     (((uint32_t)(((uint32_t)(x)) << FTM_SWOCTRL_CH5OC_SHIFT)) & FTM_SWOCTRL_CH5OC_MASK)
#define FTM_SWOCTRL_CH6OC_MASK                   (0x40U)
#define FTM_SWOCTRL_CH6OC_SHIFT                  (6U)
#define FTM_SWOCTRL_CH6OC(x)                     (((uint32_t)(((uint32_t)(x)) << FTM_SWOCTRL_CH6OC_SHIFT)) & FTM_SWOCTRL_CH6OC_MASK)
#define FTM_SWOCTRL_CH7OC_MASK                   (0x80U)
#define FTM_SWOCTRL_CH7OC_SHIFT                  (7U)
#define FTM_SWOCTRL_CH7OC(x)                     (((uint32_t)(((uint32_t)(x)) << FTM_SWOCTRL_CH7OC_SHIFT)) & FTM_SWOCTRL_CH7OC_MASK)
#define FTM_SWOCTRL_CH0OCV_MASK                  (0x100U)
#define FTM_SWOCTRL_CH0OCV_SHIFT                 (8U)
#define FTM_SWOCTRL_CH0OCV(x)                    (((uint32_t)(((uint32_t)(x)) << FTM_SWOCTRL_CH0OCV_SHIFT)) & FTM_SWOCTRL_CH0OCV_MASK)
#define FTM_SWOCTRL_CH1OCV_MASK                  (0x200U)
#define FTM_SWOCTRL_CH1OCV_SHIFT                 (9U)
#define FTM_SWOCTRL_CH1OCV(x)                    (((uint32_t)(((uint32_t)(x)) << FTM_SWOCTRL_CH1OCV_SHIFT)) & FTM_SWOCTRL_CH1OCV_MASK)
#define FTM_SWOCTRL_CH2OCV_MASK                  (0x400U)
#define FTM_SWOCTRL_CH2OCV_SHIFT                 (10U)
#define FTM_SWOCTRL_CH2OCV(x)                    (((uint32_t)(((uint32_t)(x)) << FTM_SWOCTRL_CH2OCV_SHIFT)) & FTM_SWOCTRL_CH2OCV_MASK)
#define FTM_SWOCTRL_CH3OCV_MASK                  (0x800U)
#define FTM_SWOCTRL_CH3OCV_SHIFT                 (11U)
#define FTM_SWOCTRL_CH3OCV(x)                    (((uint32_t)(((uint32_t)(x)) << FTM_SWOCTRL_CH3OCV_SHIFT)) & FTM_SWOCTRL_CH3OCV_MASK)
#define FTM_SWOCTRL_CH4OCV_MASK                  (0x1000U)
#define FTM_SWOCTRL_CH4OCV_SHIFT                 (12U)
#define FTM_SWOCTRL_CH4OCV(x)                    (((uint32_t)(((uint32_t)(x)) << FTM_SWOCTRL_CH4OCV_SHIFT)) & FTM_SWOCTRL_CH4OCV_MASK)
#define FTM_SWOCTRL_CH5OCV_MASK                  (0x2000U)
#define FTM_SWOCTRL_CH5OCV_SHIFT                 (13U)
#define FTM_SWOCTRL_CH5OCV(x)                    (((uint32_t)(((uint32_t)(x)) << FTM_SWOCTRL_CH5OCV_SHIFT)) & FTM_SWOCTRL_CH5OCV_MASK)
#define FTM_SWOCTRL_CH6OCV_MASK                  (0x4000U)
#define FTM_SWOCTRL_CH6OCV_SHIFT                 (14U)
#define FTM_SWOCTRL_CH6OCV(x)                    (((uint32_t)(((uint32_t)(x)) << FTM_SWOCTRL_CH6OCV_SHIFT)) & FTM_SWOCTRL_CH6OCV_MASK)
#define FTM_SWOCTRL_CH7OCV_MASK                  (0x8000U)
#define FTM_SWOCTRL_CH7OCV_SHIFT                 (15U)
#define FTM_SWOCTRL_CH7OCV(x)                    (((uint32_t)(((uint32_t)(x)) << FTM_SWOCTRL_CH7OCV_SHIFT)) & FTM_SWOCTRL_CH7OCV_MASK)

/*! @name PWMLOAD - FTM PWM Load */
#define FTM_PWMLOAD_CH0SEL_MASK                  (0x1U)
#define FTM_PWMLOAD_CH0SEL_SHIFT                 (0U)
#define FTM_PWMLOAD_CH0SEL(x)                    (((uint32_t)(((uint32_t)(x)) << FTM_PWMLOAD_CH0SEL_SHIFT)) & FTM_PWMLOAD_CH0SEL_MASK)
#define FTM_PWMLOAD_CH1SEL_MASK                  (0x2U)
#define FTM_PWMLOAD_CH1SEL_SHIFT                 (1U)
#define FTM_PWMLOAD_CH1SEL(x)                    (((uint32_t)(((uint32_t)(x)) << FTM_PWMLOAD_CH1SEL_SHIFT)) & FTM_PWMLOAD_CH1SEL_MASK)
#define FTM_PWMLOAD_CH2SEL_MASK                  (0x4U)
#define FTM_PWMLOAD_CH2SEL_SHIFT                 (2U)
#define FTM_PWMLOAD_CH2SEL(x)                    (((uint32_t)(((uint32_t)(x)) << FTM_PWMLOAD_CH2SEL_SHIFT)) & FTM_PWMLOAD_CH2SEL_MASK)
#define FTM_PWMLOAD_CH3SEL_MASK                  (0x8U)
#define FTM_PWMLOAD_CH3SEL_SHIFT                 (3U)
#define FTM_PWMLOAD_CH3SEL(x)                    (((uint32_t)(((uint32_t)(x)) << FTM_PWMLOAD_CH3SEL_SHIFT)) & FTM_PWMLOAD_CH3SEL_MASK)
#define FTM_PWMLOAD_CH4SEL_MASK                  (0x10U)
#define FTM_PWMLOAD_CH4SEL_SHIFT                 (4U)
#define FTM_PWMLOAD_CH4SEL(x)                    (((uint32_t)(((uint32_t)(x)) << FTM_PWMLOAD_CH4SEL_SHIFT)) & FTM_PWMLOAD_CH4SEL_MASK)
#define FTM_PWMLOAD_CH5SEL_MASK                  (0x20U)
#define FTM_PWMLOAD_CH5SEL_SHIFT                 (5U)
#define FTM_PWMLOAD_CH5SEL(x)                    (((uint32_t)(((uint32_t)(x)) << FTM_PWMLOAD_CH5SEL_SHIFT)) & FTM_PWMLOAD_CH5SEL_MASK)
#define FTM_PWMLOAD_CH6SEL_MASK                  (0x40U)
#define FTM_PWMLOAD_CH6SEL_SHIFT                 (6U)
#define FTM_PWMLOAD_CH6SEL(x)                    (((uint32_t)(((uint32_t)(x)) << FTM_PWMLOAD_CH6SEL_SHIFT)) & FTM_PWMLOAD_CH6SEL_MASK)
#define FTM_PWMLOAD_CH7SEL_MASK                  (0x80U)
#define FTM_PWMLOAD_CH7SEL_SHIFT                 (7U)
#define FTM_PWMLOAD_CH7SEL(x)                    (((uint32_t)(((uint32_t)(x)) << FTM_PWMLOAD_CH7SEL_SHIFT)) & FTM_PWMLOAD_CH7SEL_MASK)
#define FTM_PWMLOAD_LDOK_MASK                    (0x200U)
#define FTM_PWMLOAD_LDOK_SHIFT                   (9U)
#define FTM_PWMLOAD_LDOK(x)                      (((uint32_t)(((uint32_t)(x)) << FTM_PWMLOAD_LDOK_SHIFT)) & FTM_PWMLOAD_LDOK_MASK)


/*!
 * @}
 */ /* end of group FTM_Register_Masks */


/* FTM - Peripheral instance base addresses */
/** Peripheral FTM0 base address */
#define FTM0_BASE                                (0x40038000u)
/** Peripheral FTM0 base pointer */
#define FTM0                                     ((FTM_Type *)FTM0_BASE)
/** Peripheral FTM1 base address */
#define FTM1_BASE                                (0x40039000u)
/** Peripheral FTM1 base pointer */
#define FTM1                                     ((FTM_Type *)FTM1_BASE)
/** Peripheral FTM2 base address */
#define FTM2_BASE                                (0x4003A000u)
/** Peripheral FTM2 base pointer */
#define FTM2                                     ((FTM_Type *)FTM2_BASE)
/** Array initializer of FTM peripheral base addresses */
#define FTM_BASE_ADDRS                           { FTM0_BASE, FTM1_BASE, FTM2_BASE }
/** Array initializer of FTM peripheral base pointers */
#define FTM_BASE_PTRS                            { FTM0, FTM1, FTM2 }
/** Interrupt vectors for the FTM peripheral type */
#define FTM_IRQS                                 { FTM0_IRQn, FTM1_IRQn, FTM2_IRQn }

/*!
 * @}
 */ /* end of group FTM_Peripheral_Access_Layer */


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
/** Peripheral GPIOD base address */
#define GPIOD_BASE                               (0x400FF0C0u)
/** Peripheral GPIOD base pointer */
#define GPIOD                                    ((GPIO_Type *)GPIOD_BASE)
/** Peripheral GPIOE base address */
#define GPIOE_BASE                               (0x400FF100u)
/** Peripheral GPIOE base pointer */
#define GPIOE                                    ((GPIO_Type *)GPIOE_BASE)
/** Array initializer of GPIO peripheral base addresses */
#define GPIO_BASE_ADDRS                          { GPIOA_BASE, GPIOB_BASE, GPIOC_BASE, GPIOD_BASE, GPIOE_BASE }
/** Array initializer of GPIO peripheral base pointers */
#define GPIO_BASE_PTRS                           { GPIOA, GPIOB, GPIOC, GPIOD, GPIOE }

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
  __IO uint8_t FLT;                                /**< I2C Programmable Input Glitch Filter register, offset: 0x6 */
  __IO uint8_t RA;                                 /**< I2C Range Address register, offset: 0x7 */
  __IO uint8_t SMB;                                /**< I2C SMBus Control and Status register, offset: 0x8 */
  __IO uint8_t A2;                                 /**< I2C Address Register 2, offset: 0x9 */
  __IO uint8_t SLTH;                               /**< I2C SCL Low Timeout Register High, offset: 0xA */
  __IO uint8_t SLTL;                               /**< I2C SCL Low Timeout Register Low, offset: 0xB */
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

/*! @name FLT - I2C Programmable Input Glitch Filter register */
#define I2C_FLT_FLT_MASK                         (0x1FU)
#define I2C_FLT_FLT_SHIFT                        (0U)
#define I2C_FLT_FLT(x)                           (((uint8_t)(((uint8_t)(x)) << I2C_FLT_FLT_SHIFT)) & I2C_FLT_FLT_MASK)

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
   -- I2S Peripheral Access Layer
   ---------------------------------------------------------------------------- */

/*!
 * @addtogroup I2S_Peripheral_Access_Layer I2S Peripheral Access Layer
 * @{
 */

/** I2S - Register Layout Typedef */
typedef struct {
  __IO uint32_t TCSR;                              /**< SAI Transmit Control Register, offset: 0x0 */
  __IO uint32_t TCR1;                              /**< SAI Transmit Configuration 1 Register, offset: 0x4 */
  __IO uint32_t TCR2;                              /**< SAI Transmit Configuration 2 Register, offset: 0x8 */
  __IO uint32_t TCR3;                              /**< SAI Transmit Configuration 3 Register, offset: 0xC */
  __IO uint32_t TCR4;                              /**< SAI Transmit Configuration 4 Register, offset: 0x10 */
  __IO uint32_t TCR5;                              /**< SAI Transmit Configuration 5 Register, offset: 0x14 */
       uint8_t RESERVED_0[8];
  __O  uint32_t TDR[1];                            /**< SAI Transmit Data Register, array offset: 0x20, array step: 0x4 */
       uint8_t RESERVED_1[28];
  __I  uint32_t TFR[1];                            /**< SAI Transmit FIFO Register, array offset: 0x40, array step: 0x4 */
       uint8_t RESERVED_2[28];
  __IO uint32_t TMR;                               /**< SAI Transmit Mask Register, offset: 0x60 */
       uint8_t RESERVED_3[28];
  __IO uint32_t RCSR;                              /**< SAI Receive Control Register, offset: 0x80 */
  __IO uint32_t RCR1;                              /**< SAI Receive Configuration 1 Register, offset: 0x84 */
  __IO uint32_t RCR2;                              /**< SAI Receive Configuration 2 Register, offset: 0x88 */
  __IO uint32_t RCR3;                              /**< SAI Receive Configuration 3 Register, offset: 0x8C */
  __IO uint32_t RCR4;                              /**< SAI Receive Configuration 4 Register, offset: 0x90 */
  __IO uint32_t RCR5;                              /**< SAI Receive Configuration 5 Register, offset: 0x94 */
       uint8_t RESERVED_4[8];
  __I  uint32_t RDR[1];                            /**< SAI Receive Data Register, array offset: 0xA0, array step: 0x4 */
       uint8_t RESERVED_5[28];
  __I  uint32_t RFR[1];                            /**< SAI Receive FIFO Register, array offset: 0xC0, array step: 0x4 */
       uint8_t RESERVED_6[28];
  __IO uint32_t RMR;                               /**< SAI Receive Mask Register, offset: 0xE0 */
       uint8_t RESERVED_7[28];
  __IO uint32_t MCR;                               /**< SAI MCLK Control Register, offset: 0x100 */
  __IO uint32_t MDR;                               /**< SAI MCLK Divide Register, offset: 0x104 */
} I2S_Type;

/* ----------------------------------------------------------------------------
   -- I2S Register Masks
   ---------------------------------------------------------------------------- */

/*!
 * @addtogroup I2S_Register_Masks I2S Register Masks
 * @{
 */

/*! @name TCSR - SAI Transmit Control Register */
#define I2S_TCSR_FRDE_MASK                       (0x1U)
#define I2S_TCSR_FRDE_SHIFT                      (0U)
#define I2S_TCSR_FRDE(x)                         (((uint32_t)(((uint32_t)(x)) << I2S_TCSR_FRDE_SHIFT)) & I2S_TCSR_FRDE_MASK)
#define I2S_TCSR_FWDE_MASK                       (0x2U)
#define I2S_TCSR_FWDE_SHIFT                      (1U)
#define I2S_TCSR_FWDE(x)                         (((uint32_t)(((uint32_t)(x)) << I2S_TCSR_FWDE_SHIFT)) & I2S_TCSR_FWDE_MASK)
#define I2S_TCSR_FRIE_MASK                       (0x100U)
#define I2S_TCSR_FRIE_SHIFT                      (8U)
#define I2S_TCSR_FRIE(x)                         (((uint32_t)(((uint32_t)(x)) << I2S_TCSR_FRIE_SHIFT)) & I2S_TCSR_FRIE_MASK)
#define I2S_TCSR_FWIE_MASK                       (0x200U)
#define I2S_TCSR_FWIE_SHIFT                      (9U)
#define I2S_TCSR_FWIE(x)                         (((uint32_t)(((uint32_t)(x)) << I2S_TCSR_FWIE_SHIFT)) & I2S_TCSR_FWIE_MASK)
#define I2S_TCSR_FEIE_MASK                       (0x400U)
#define I2S_TCSR_FEIE_SHIFT                      (10U)
#define I2S_TCSR_FEIE(x)                         (((uint32_t)(((uint32_t)(x)) << I2S_TCSR_FEIE_SHIFT)) & I2S_TCSR_FEIE_MASK)
#define I2S_TCSR_SEIE_MASK                       (0x800U)
#define I2S_TCSR_SEIE_SHIFT                      (11U)
#define I2S_TCSR_SEIE(x)                         (((uint32_t)(((uint32_t)(x)) << I2S_TCSR_SEIE_SHIFT)) & I2S_TCSR_SEIE_MASK)
#define I2S_TCSR_WSIE_MASK                       (0x1000U)
#define I2S_TCSR_WSIE_SHIFT                      (12U)
#define I2S_TCSR_WSIE(x)                         (((uint32_t)(((uint32_t)(x)) << I2S_TCSR_WSIE_SHIFT)) & I2S_TCSR_WSIE_MASK)
#define I2S_TCSR_FRF_MASK                        (0x10000U)
#define I2S_TCSR_FRF_SHIFT                       (16U)
#define I2S_TCSR_FRF(x)                          (((uint32_t)(((uint32_t)(x)) << I2S_TCSR_FRF_SHIFT)) & I2S_TCSR_FRF_MASK)
#define I2S_TCSR_FWF_MASK                        (0x20000U)
#define I2S_TCSR_FWF_SHIFT                       (17U)
#define I2S_TCSR_FWF(x)                          (((uint32_t)(((uint32_t)(x)) << I2S_TCSR_FWF_SHIFT)) & I2S_TCSR_FWF_MASK)
#define I2S_TCSR_FEF_MASK                        (0x40000U)
#define I2S_TCSR_FEF_SHIFT                       (18U)
#define I2S_TCSR_FEF(x)                          (((uint32_t)(((uint32_t)(x)) << I2S_TCSR_FEF_SHIFT)) & I2S_TCSR_FEF_MASK)
#define I2S_TCSR_SEF_MASK                        (0x80000U)
#define I2S_TCSR_SEF_SHIFT                       (19U)
#define I2S_TCSR_SEF(x)                          (((uint32_t)(((uint32_t)(x)) << I2S_TCSR_SEF_SHIFT)) & I2S_TCSR_SEF_MASK)
#define I2S_TCSR_WSF_MASK                        (0x100000U)
#define I2S_TCSR_WSF_SHIFT                       (20U)
#define I2S_TCSR_WSF(x)                          (((uint32_t)(((uint32_t)(x)) << I2S_TCSR_WSF_SHIFT)) & I2S_TCSR_WSF_MASK)
#define I2S_TCSR_SR_MASK                         (0x1000000U)
#define I2S_TCSR_SR_SHIFT                        (24U)
#define I2S_TCSR_SR(x)                           (((uint32_t)(((uint32_t)(x)) << I2S_TCSR_SR_SHIFT)) & I2S_TCSR_SR_MASK)
#define I2S_TCSR_FR_MASK                         (0x2000000U)
#define I2S_TCSR_FR_SHIFT                        (25U)
#define I2S_TCSR_FR(x)                           (((uint32_t)(((uint32_t)(x)) << I2S_TCSR_FR_SHIFT)) & I2S_TCSR_FR_MASK)
#define I2S_TCSR_BCE_MASK                        (0x10000000U)
#define I2S_TCSR_BCE_SHIFT                       (28U)
#define I2S_TCSR_BCE(x)                          (((uint32_t)(((uint32_t)(x)) << I2S_TCSR_BCE_SHIFT)) & I2S_TCSR_BCE_MASK)
#define I2S_TCSR_DBGE_MASK                       (0x20000000U)
#define I2S_TCSR_DBGE_SHIFT                      (29U)
#define I2S_TCSR_DBGE(x)                         (((uint32_t)(((uint32_t)(x)) << I2S_TCSR_DBGE_SHIFT)) & I2S_TCSR_DBGE_MASK)
#define I2S_TCSR_STOPE_MASK                      (0x40000000U)
#define I2S_TCSR_STOPE_SHIFT                     (30U)
#define I2S_TCSR_STOPE(x)                        (((uint32_t)(((uint32_t)(x)) << I2S_TCSR_STOPE_SHIFT)) & I2S_TCSR_STOPE_MASK)
#define I2S_TCSR_TE_MASK                         (0x80000000U)
#define I2S_TCSR_TE_SHIFT                        (31U)
#define I2S_TCSR_TE(x)                           (((uint32_t)(((uint32_t)(x)) << I2S_TCSR_TE_SHIFT)) & I2S_TCSR_TE_MASK)

/*! @name TCR1 - SAI Transmit Configuration 1 Register */
#define I2S_TCR1_TFW_MASK                        (0x7U)
#define I2S_TCR1_TFW_SHIFT                       (0U)
#define I2S_TCR1_TFW(x)                          (((uint32_t)(((uint32_t)(x)) << I2S_TCR1_TFW_SHIFT)) & I2S_TCR1_TFW_MASK)

/*! @name TCR2 - SAI Transmit Configuration 2 Register */
#define I2S_TCR2_DIV_MASK                        (0xFFU)
#define I2S_TCR2_DIV_SHIFT                       (0U)
#define I2S_TCR2_DIV(x)                          (((uint32_t)(((uint32_t)(x)) << I2S_TCR2_DIV_SHIFT)) & I2S_TCR2_DIV_MASK)
#define I2S_TCR2_BCD_MASK                        (0x1000000U)
#define I2S_TCR2_BCD_SHIFT                       (24U)
#define I2S_TCR2_BCD(x)                          (((uint32_t)(((uint32_t)(x)) << I2S_TCR2_BCD_SHIFT)) & I2S_TCR2_BCD_MASK)
#define I2S_TCR2_BCP_MASK                        (0x2000000U)
#define I2S_TCR2_BCP_SHIFT                       (25U)
#define I2S_TCR2_BCP(x)                          (((uint32_t)(((uint32_t)(x)) << I2S_TCR2_BCP_SHIFT)) & I2S_TCR2_BCP_MASK)
#define I2S_TCR2_MSEL_MASK                       (0xC000000U)
#define I2S_TCR2_MSEL_SHIFT                      (26U)
#define I2S_TCR2_MSEL(x)                         (((uint32_t)(((uint32_t)(x)) << I2S_TCR2_MSEL_SHIFT)) & I2S_TCR2_MSEL_MASK)
#define I2S_TCR2_BCI_MASK                        (0x10000000U)
#define I2S_TCR2_BCI_SHIFT                       (28U)
#define I2S_TCR2_BCI(x)                          (((uint32_t)(((uint32_t)(x)) << I2S_TCR2_BCI_SHIFT)) & I2S_TCR2_BCI_MASK)
#define I2S_TCR2_BCS_MASK                        (0x20000000U)
#define I2S_TCR2_BCS_SHIFT                       (29U)
#define I2S_TCR2_BCS(x)                          (((uint32_t)(((uint32_t)(x)) << I2S_TCR2_BCS_SHIFT)) & I2S_TCR2_BCS_MASK)
#define I2S_TCR2_SYNC_MASK                       (0xC0000000U)
#define I2S_TCR2_SYNC_SHIFT                      (30U)
#define I2S_TCR2_SYNC(x)                         (((uint32_t)(((uint32_t)(x)) << I2S_TCR2_SYNC_SHIFT)) & I2S_TCR2_SYNC_MASK)

/*! @name TCR3 - SAI Transmit Configuration 3 Register */
#define I2S_TCR3_WDFL_MASK                       (0xFU)
#define I2S_TCR3_WDFL_SHIFT                      (0U)
#define I2S_TCR3_WDFL(x)                         (((uint32_t)(((uint32_t)(x)) << I2S_TCR3_WDFL_SHIFT)) & I2S_TCR3_WDFL_MASK)
#define I2S_TCR3_TCE_MASK                        (0x10000U)
#define I2S_TCR3_TCE_SHIFT                       (16U)
#define I2S_TCR3_TCE(x)                          (((uint32_t)(((uint32_t)(x)) << I2S_TCR3_TCE_SHIFT)) & I2S_TCR3_TCE_MASK)

/*! @name TCR4 - SAI Transmit Configuration 4 Register */
#define I2S_TCR4_FSD_MASK                        (0x1U)
#define I2S_TCR4_FSD_SHIFT                       (0U)
#define I2S_TCR4_FSD(x)                          (((uint32_t)(((uint32_t)(x)) << I2S_TCR4_FSD_SHIFT)) & I2S_TCR4_FSD_MASK)
#define I2S_TCR4_FSP_MASK                        (0x2U)
#define I2S_TCR4_FSP_SHIFT                       (1U)
#define I2S_TCR4_FSP(x)                          (((uint32_t)(((uint32_t)(x)) << I2S_TCR4_FSP_SHIFT)) & I2S_TCR4_FSP_MASK)
#define I2S_TCR4_FSE_MASK                        (0x8U)
#define I2S_TCR4_FSE_SHIFT                       (3U)
#define I2S_TCR4_FSE(x)                          (((uint32_t)(((uint32_t)(x)) << I2S_TCR4_FSE_SHIFT)) & I2S_TCR4_FSE_MASK)
#define I2S_TCR4_MF_MASK                         (0x10U)
#define I2S_TCR4_MF_SHIFT                        (4U)
#define I2S_TCR4_MF(x)                           (((uint32_t)(((uint32_t)(x)) << I2S_TCR4_MF_SHIFT)) & I2S_TCR4_MF_MASK)
#define I2S_TCR4_SYWD_MASK                       (0x1F00U)
#define I2S_TCR4_SYWD_SHIFT                      (8U)
#define I2S_TCR4_SYWD(x)                         (((uint32_t)(((uint32_t)(x)) << I2S_TCR4_SYWD_SHIFT)) & I2S_TCR4_SYWD_MASK)
#define I2S_TCR4_FRSZ_MASK                       (0xF0000U)
#define I2S_TCR4_FRSZ_SHIFT                      (16U)
#define I2S_TCR4_FRSZ(x)                         (((uint32_t)(((uint32_t)(x)) << I2S_TCR4_FRSZ_SHIFT)) & I2S_TCR4_FRSZ_MASK)

/*! @name TCR5 - SAI Transmit Configuration 5 Register */
#define I2S_TCR5_FBT_MASK                        (0x1F00U)
#define I2S_TCR5_FBT_SHIFT                       (8U)
#define I2S_TCR5_FBT(x)                          (((uint32_t)(((uint32_t)(x)) << I2S_TCR5_FBT_SHIFT)) & I2S_TCR5_FBT_MASK)
#define I2S_TCR5_W0W_MASK                        (0x1F0000U)
#define I2S_TCR5_W0W_SHIFT                       (16U)
#define I2S_TCR5_W0W(x)                          (((uint32_t)(((uint32_t)(x)) << I2S_TCR5_W0W_SHIFT)) & I2S_TCR5_W0W_MASK)
#define I2S_TCR5_WNW_MASK                        (0x1F000000U)
#define I2S_TCR5_WNW_SHIFT                       (24U)
#define I2S_TCR5_WNW(x)                          (((uint32_t)(((uint32_t)(x)) << I2S_TCR5_WNW_SHIFT)) & I2S_TCR5_WNW_MASK)

/*! @name TDR - SAI Transmit Data Register */
#define I2S_TDR_TDR_MASK                         (0xFFFFFFFFU)
#define I2S_TDR_TDR_SHIFT                        (0U)
#define I2S_TDR_TDR(x)                           (((uint32_t)(((uint32_t)(x)) << I2S_TDR_TDR_SHIFT)) & I2S_TDR_TDR_MASK)

/* The count of I2S_TDR */
#define I2S_TDR_COUNT                            (1U)

/*! @name TFR - SAI Transmit FIFO Register */
#define I2S_TFR_RFP_MASK                         (0xFU)
#define I2S_TFR_RFP_SHIFT                        (0U)
#define I2S_TFR_RFP(x)                           (((uint32_t)(((uint32_t)(x)) << I2S_TFR_RFP_SHIFT)) & I2S_TFR_RFP_MASK)
#define I2S_TFR_WFP_MASK                         (0xF0000U)
#define I2S_TFR_WFP_SHIFT                        (16U)
#define I2S_TFR_WFP(x)                           (((uint32_t)(((uint32_t)(x)) << I2S_TFR_WFP_SHIFT)) & I2S_TFR_WFP_MASK)

/* The count of I2S_TFR */
#define I2S_TFR_COUNT                            (1U)

/*! @name TMR - SAI Transmit Mask Register */
#define I2S_TMR_TWM_MASK                         (0xFFFFU)
#define I2S_TMR_TWM_SHIFT                        (0U)
#define I2S_TMR_TWM(x)                           (((uint32_t)(((uint32_t)(x)) << I2S_TMR_TWM_SHIFT)) & I2S_TMR_TWM_MASK)

/*! @name RCSR - SAI Receive Control Register */
#define I2S_RCSR_FRDE_MASK                       (0x1U)
#define I2S_RCSR_FRDE_SHIFT                      (0U)
#define I2S_RCSR_FRDE(x)                         (((uint32_t)(((uint32_t)(x)) << I2S_RCSR_FRDE_SHIFT)) & I2S_RCSR_FRDE_MASK)
#define I2S_RCSR_FWDE_MASK                       (0x2U)
#define I2S_RCSR_FWDE_SHIFT                      (1U)
#define I2S_RCSR_FWDE(x)                         (((uint32_t)(((uint32_t)(x)) << I2S_RCSR_FWDE_SHIFT)) & I2S_RCSR_FWDE_MASK)
#define I2S_RCSR_FRIE_MASK                       (0x100U)
#define I2S_RCSR_FRIE_SHIFT                      (8U)
#define I2S_RCSR_FRIE(x)                         (((uint32_t)(((uint32_t)(x)) << I2S_RCSR_FRIE_SHIFT)) & I2S_RCSR_FRIE_MASK)
#define I2S_RCSR_FWIE_MASK                       (0x200U)
#define I2S_RCSR_FWIE_SHIFT                      (9U)
#define I2S_RCSR_FWIE(x)                         (((uint32_t)(((uint32_t)(x)) << I2S_RCSR_FWIE_SHIFT)) & I2S_RCSR_FWIE_MASK)
#define I2S_RCSR_FEIE_MASK                       (0x400U)
#define I2S_RCSR_FEIE_SHIFT                      (10U)
#define I2S_RCSR_FEIE(x)                         (((uint32_t)(((uint32_t)(x)) << I2S_RCSR_FEIE_SHIFT)) & I2S_RCSR_FEIE_MASK)
#define I2S_RCSR_SEIE_MASK                       (0x800U)
#define I2S_RCSR_SEIE_SHIFT                      (11U)
#define I2S_RCSR_SEIE(x)                         (((uint32_t)(((uint32_t)(x)) << I2S_RCSR_SEIE_SHIFT)) & I2S_RCSR_SEIE_MASK)
#define I2S_RCSR_WSIE_MASK                       (0x1000U)
#define I2S_RCSR_WSIE_SHIFT                      (12U)
#define I2S_RCSR_WSIE(x)                         (((uint32_t)(((uint32_t)(x)) << I2S_RCSR_WSIE_SHIFT)) & I2S_RCSR_WSIE_MASK)
#define I2S_RCSR_FRF_MASK                        (0x10000U)
#define I2S_RCSR_FRF_SHIFT                       (16U)
#define I2S_RCSR_FRF(x)                          (((uint32_t)(((uint32_t)(x)) << I2S_RCSR_FRF_SHIFT)) & I2S_RCSR_FRF_MASK)
#define I2S_RCSR_FWF_MASK                        (0x20000U)
#define I2S_RCSR_FWF_SHIFT                       (17U)
#define I2S_RCSR_FWF(x)                          (((uint32_t)(((uint32_t)(x)) << I2S_RCSR_FWF_SHIFT)) & I2S_RCSR_FWF_MASK)
#define I2S_RCSR_FEF_MASK                        (0x40000U)
#define I2S_RCSR_FEF_SHIFT                       (18U)
#define I2S_RCSR_FEF(x)                          (((uint32_t)(((uint32_t)(x)) << I2S_RCSR_FEF_SHIFT)) & I2S_RCSR_FEF_MASK)
#define I2S_RCSR_SEF_MASK                        (0x80000U)
#define I2S_RCSR_SEF_SHIFT                       (19U)
#define I2S_RCSR_SEF(x)                          (((uint32_t)(((uint32_t)(x)) << I2S_RCSR_SEF_SHIFT)) & I2S_RCSR_SEF_MASK)
#define I2S_RCSR_WSF_MASK                        (0x100000U)
#define I2S_RCSR_WSF_SHIFT                       (20U)
#define I2S_RCSR_WSF(x)                          (((uint32_t)(((uint32_t)(x)) << I2S_RCSR_WSF_SHIFT)) & I2S_RCSR_WSF_MASK)
#define I2S_RCSR_SR_MASK                         (0x1000000U)
#define I2S_RCSR_SR_SHIFT                        (24U)
#define I2S_RCSR_SR(x)                           (((uint32_t)(((uint32_t)(x)) << I2S_RCSR_SR_SHIFT)) & I2S_RCSR_SR_MASK)
#define I2S_RCSR_FR_MASK                         (0x2000000U)
#define I2S_RCSR_FR_SHIFT                        (25U)
#define I2S_RCSR_FR(x)                           (((uint32_t)(((uint32_t)(x)) << I2S_RCSR_FR_SHIFT)) & I2S_RCSR_FR_MASK)
#define I2S_RCSR_BCE_MASK                        (0x10000000U)
#define I2S_RCSR_BCE_SHIFT                       (28U)
#define I2S_RCSR_BCE(x)                          (((uint32_t)(((uint32_t)(x)) << I2S_RCSR_BCE_SHIFT)) & I2S_RCSR_BCE_MASK)
#define I2S_RCSR_DBGE_MASK                       (0x20000000U)
#define I2S_RCSR_DBGE_SHIFT                      (29U)
#define I2S_RCSR_DBGE(x)                         (((uint32_t)(((uint32_t)(x)) << I2S_RCSR_DBGE_SHIFT)) & I2S_RCSR_DBGE_MASK)
#define I2S_RCSR_STOPE_MASK                      (0x40000000U)
#define I2S_RCSR_STOPE_SHIFT                     (30U)
#define I2S_RCSR_STOPE(x)                        (((uint32_t)(((uint32_t)(x)) << I2S_RCSR_STOPE_SHIFT)) & I2S_RCSR_STOPE_MASK)
#define I2S_RCSR_RE_MASK                         (0x80000000U)
#define I2S_RCSR_RE_SHIFT                        (31U)
#define I2S_RCSR_RE(x)                           (((uint32_t)(((uint32_t)(x)) << I2S_RCSR_RE_SHIFT)) & I2S_RCSR_RE_MASK)

/*! @name RCR1 - SAI Receive Configuration 1 Register */
#define I2S_RCR1_RFW_MASK                        (0x7U)
#define I2S_RCR1_RFW_SHIFT                       (0U)
#define I2S_RCR1_RFW(x)                          (((uint32_t)(((uint32_t)(x)) << I2S_RCR1_RFW_SHIFT)) & I2S_RCR1_RFW_MASK)

/*! @name RCR2 - SAI Receive Configuration 2 Register */
#define I2S_RCR2_DIV_MASK                        (0xFFU)
#define I2S_RCR2_DIV_SHIFT                       (0U)
#define I2S_RCR2_DIV(x)                          (((uint32_t)(((uint32_t)(x)) << I2S_RCR2_DIV_SHIFT)) & I2S_RCR2_DIV_MASK)
#define I2S_RCR2_BCD_MASK                        (0x1000000U)
#define I2S_RCR2_BCD_SHIFT                       (24U)
#define I2S_RCR2_BCD(x)                          (((uint32_t)(((uint32_t)(x)) << I2S_RCR2_BCD_SHIFT)) & I2S_RCR2_BCD_MASK)
#define I2S_RCR2_BCP_MASK                        (0x2000000U)
#define I2S_RCR2_BCP_SHIFT                       (25U)
#define I2S_RCR2_BCP(x)                          (((uint32_t)(((uint32_t)(x)) << I2S_RCR2_BCP_SHIFT)) & I2S_RCR2_BCP_MASK)
#define I2S_RCR2_MSEL_MASK                       (0xC000000U)
#define I2S_RCR2_MSEL_SHIFT                      (26U)
#define I2S_RCR2_MSEL(x)                         (((uint32_t)(((uint32_t)(x)) << I2S_RCR2_MSEL_SHIFT)) & I2S_RCR2_MSEL_MASK)
#define I2S_RCR2_BCI_MASK                        (0x10000000U)
#define I2S_RCR2_BCI_SHIFT                       (28U)
#define I2S_RCR2_BCI(x)                          (((uint32_t)(((uint32_t)(x)) << I2S_RCR2_BCI_SHIFT)) & I2S_RCR2_BCI_MASK)
#define I2S_RCR2_BCS_MASK                        (0x20000000U)
#define I2S_RCR2_BCS_SHIFT                       (29U)
#define I2S_RCR2_BCS(x)                          (((uint32_t)(((uint32_t)(x)) << I2S_RCR2_BCS_SHIFT)) & I2S_RCR2_BCS_MASK)
#define I2S_RCR2_SYNC_MASK                       (0xC0000000U)
#define I2S_RCR2_SYNC_SHIFT                      (30U)
#define I2S_RCR2_SYNC(x)                         (((uint32_t)(((uint32_t)(x)) << I2S_RCR2_SYNC_SHIFT)) & I2S_RCR2_SYNC_MASK)

/*! @name RCR3 - SAI Receive Configuration 3 Register */
#define I2S_RCR3_WDFL_MASK                       (0xFU)
#define I2S_RCR3_WDFL_SHIFT                      (0U)
#define I2S_RCR3_WDFL(x)                         (((uint32_t)(((uint32_t)(x)) << I2S_RCR3_WDFL_SHIFT)) & I2S_RCR3_WDFL_MASK)
#define I2S_RCR3_RCE_MASK                        (0x10000U)
#define I2S_RCR3_RCE_SHIFT                       (16U)
#define I2S_RCR3_RCE(x)                          (((uint32_t)(((uint32_t)(x)) << I2S_RCR3_RCE_SHIFT)) & I2S_RCR3_RCE_MASK)

/*! @name RCR4 - SAI Receive Configuration 4 Register */
#define I2S_RCR4_FSD_MASK                        (0x1U)
#define I2S_RCR4_FSD_SHIFT                       (0U)
#define I2S_RCR4_FSD(x)                          (((uint32_t)(((uint32_t)(x)) << I2S_RCR4_FSD_SHIFT)) & I2S_RCR4_FSD_MASK)
#define I2S_RCR4_FSP_MASK                        (0x2U)
#define I2S_RCR4_FSP_SHIFT                       (1U)
#define I2S_RCR4_FSP(x)                          (((uint32_t)(((uint32_t)(x)) << I2S_RCR4_FSP_SHIFT)) & I2S_RCR4_FSP_MASK)
#define I2S_RCR4_FSE_MASK                        (0x8U)
#define I2S_RCR4_FSE_SHIFT                       (3U)
#define I2S_RCR4_FSE(x)                          (((uint32_t)(((uint32_t)(x)) << I2S_RCR4_FSE_SHIFT)) & I2S_RCR4_FSE_MASK)
#define I2S_RCR4_MF_MASK                         (0x10U)
#define I2S_RCR4_MF_SHIFT                        (4U)
#define I2S_RCR4_MF(x)                           (((uint32_t)(((uint32_t)(x)) << I2S_RCR4_MF_SHIFT)) & I2S_RCR4_MF_MASK)
#define I2S_RCR4_SYWD_MASK                       (0x1F00U)
#define I2S_RCR4_SYWD_SHIFT                      (8U)
#define I2S_RCR4_SYWD(x)                         (((uint32_t)(((uint32_t)(x)) << I2S_RCR4_SYWD_SHIFT)) & I2S_RCR4_SYWD_MASK)
#define I2S_RCR4_FRSZ_MASK                       (0xF0000U)
#define I2S_RCR4_FRSZ_SHIFT                      (16U)
#define I2S_RCR4_FRSZ(x)                         (((uint32_t)(((uint32_t)(x)) << I2S_RCR4_FRSZ_SHIFT)) & I2S_RCR4_FRSZ_MASK)

/*! @name RCR5 - SAI Receive Configuration 5 Register */
#define I2S_RCR5_FBT_MASK                        (0x1F00U)
#define I2S_RCR5_FBT_SHIFT                       (8U)
#define I2S_RCR5_FBT(x)                          (((uint32_t)(((uint32_t)(x)) << I2S_RCR5_FBT_SHIFT)) & I2S_RCR5_FBT_MASK)
#define I2S_RCR5_W0W_MASK                        (0x1F0000U)
#define I2S_RCR5_W0W_SHIFT                       (16U)
#define I2S_RCR5_W0W(x)                          (((uint32_t)(((uint32_t)(x)) << I2S_RCR5_W0W_SHIFT)) & I2S_RCR5_W0W_MASK)
#define I2S_RCR5_WNW_MASK                        (0x1F000000U)
#define I2S_RCR5_WNW_SHIFT                       (24U)
#define I2S_RCR5_WNW(x)                          (((uint32_t)(((uint32_t)(x)) << I2S_RCR5_WNW_SHIFT)) & I2S_RCR5_WNW_MASK)

/*! @name RDR - SAI Receive Data Register */
#define I2S_RDR_RDR_MASK                         (0xFFFFFFFFU)
#define I2S_RDR_RDR_SHIFT                        (0U)
#define I2S_RDR_RDR(x)                           (((uint32_t)(((uint32_t)(x)) << I2S_RDR_RDR_SHIFT)) & I2S_RDR_RDR_MASK)

/* The count of I2S_RDR */
#define I2S_RDR_COUNT                            (1U)

/*! @name RFR - SAI Receive FIFO Register */
#define I2S_RFR_RFP_MASK                         (0xFU)
#define I2S_RFR_RFP_SHIFT                        (0U)
#define I2S_RFR_RFP(x)                           (((uint32_t)(((uint32_t)(x)) << I2S_RFR_RFP_SHIFT)) & I2S_RFR_RFP_MASK)
#define I2S_RFR_WFP_MASK                         (0xF0000U)
#define I2S_RFR_WFP_SHIFT                        (16U)
#define I2S_RFR_WFP(x)                           (((uint32_t)(((uint32_t)(x)) << I2S_RFR_WFP_SHIFT)) & I2S_RFR_WFP_MASK)

/* The count of I2S_RFR */
#define I2S_RFR_COUNT                            (1U)

/*! @name RMR - SAI Receive Mask Register */
#define I2S_RMR_RWM_MASK                         (0xFFFFU)
#define I2S_RMR_RWM_SHIFT                        (0U)
#define I2S_RMR_RWM(x)                           (((uint32_t)(((uint32_t)(x)) << I2S_RMR_RWM_SHIFT)) & I2S_RMR_RWM_MASK)

/*! @name MCR - SAI MCLK Control Register */
#define I2S_MCR_MICS_MASK                        (0x3000000U)
#define I2S_MCR_MICS_SHIFT                       (24U)
#define I2S_MCR_MICS(x)                          (((uint32_t)(((uint32_t)(x)) << I2S_MCR_MICS_SHIFT)) & I2S_MCR_MICS_MASK)
#define I2S_MCR_MOE_MASK                         (0x40000000U)
#define I2S_MCR_MOE_SHIFT                        (30U)
#define I2S_MCR_MOE(x)                           (((uint32_t)(((uint32_t)(x)) << I2S_MCR_MOE_SHIFT)) & I2S_MCR_MOE_MASK)
#define I2S_MCR_DUF_MASK                         (0x80000000U)
#define I2S_MCR_DUF_SHIFT                        (31U)
#define I2S_MCR_DUF(x)                           (((uint32_t)(((uint32_t)(x)) << I2S_MCR_DUF_SHIFT)) & I2S_MCR_DUF_MASK)

/*! @name MDR - SAI MCLK Divide Register */
#define I2S_MDR_DIVIDE_MASK                      (0xFFFU)
#define I2S_MDR_DIVIDE_SHIFT                     (0U)
#define I2S_MDR_DIVIDE(x)                        (((uint32_t)(((uint32_t)(x)) << I2S_MDR_DIVIDE_SHIFT)) & I2S_MDR_DIVIDE_MASK)
#define I2S_MDR_FRACT_MASK                       (0xFF000U)
#define I2S_MDR_FRACT_SHIFT                      (12U)
#define I2S_MDR_FRACT(x)                         (((uint32_t)(((uint32_t)(x)) << I2S_MDR_FRACT_SHIFT)) & I2S_MDR_FRACT_MASK)


/*!
 * @}
 */ /* end of group I2S_Register_Masks */


/* I2S - Peripheral instance base addresses */
/** Peripheral I2S0 base address */
#define I2S0_BASE                                (0x4002F000u)
/** Peripheral I2S0 base pointer */
#define I2S0                                     ((I2S_Type *)I2S0_BASE)
/** Array initializer of I2S peripheral base addresses */
#define I2S_BASE_ADDRS                           { I2S0_BASE }
/** Array initializer of I2S peripheral base pointers */
#define I2S_BASE_PTRS                            { I2S0 }
/** Interrupt vectors for the I2S peripheral type */
#define I2S_RX_IRQS                              { I2S0_Rx_IRQn }
#define I2S_TX_IRQS                              { I2S0_Tx_IRQn }

/*!
 * @}
 */ /* end of group I2S_Peripheral_Access_Layer */


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
  __IO uint8_t RST;                                /**< LLWU Reset Enable register, offset: 0xA */
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

/*! @name RST - LLWU Reset Enable register */
#define LLWU_RST_RSTFILT_MASK                    (0x1U)
#define LLWU_RST_RSTFILT_SHIFT                   (0U)
#define LLWU_RST_RSTFILT(x)                      (((uint8_t)(((uint8_t)(x)) << LLWU_RST_RSTFILT_SHIFT)) & LLWU_RST_RSTFILT_MASK)
#define LLWU_RST_LLRSTE_MASK                     (0x2U)
#define LLWU_RST_LLRSTE_SHIFT                    (1U)
#define LLWU_RST_LLRSTE(x)                       (((uint8_t)(((uint8_t)(x)) << LLWU_RST_LLRSTE_SHIFT)) & LLWU_RST_LLRSTE_MASK)


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
  __IO uint8_t C5;                                 /**< MCG Control 5 Register, offset: 0x4 */
  __IO uint8_t C6;                                 /**< MCG Control 6 Register, offset: 0x5 */
  __IO uint8_t S;                                  /**< MCG Status Register, offset: 0x6 */
       uint8_t RESERVED_0[1];
  __IO uint8_t SC;                                 /**< MCG Status and Control Register, offset: 0x8 */
       uint8_t RESERVED_1[1];
  __IO uint8_t ATCVH;                              /**< MCG Auto Trim Compare Value High Register, offset: 0xA */
  __IO uint8_t ATCVL;                              /**< MCG Auto Trim Compare Value Low Register, offset: 0xB */
  __IO uint8_t C7;                                 /**< MCG Control 7 Register, offset: 0xC */
  __IO uint8_t C8;                                 /**< MCG Control 8 Register, offset: 0xD */
  __I  uint8_t C9;                                 /**< MCG Control 9 Register, offset: 0xE */
  __I  uint8_t C10;                                /**< MCG Control 10 Register, offset: 0xF */
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
#define MCG_C2_EREFS0_MASK                       (0x4U)
#define MCG_C2_EREFS0_SHIFT                      (2U)
#define MCG_C2_EREFS0(x)                         (((uint8_t)(((uint8_t)(x)) << MCG_C2_EREFS0_SHIFT)) & MCG_C2_EREFS0_MASK)
#define MCG_C2_HGO0_MASK                         (0x8U)
#define MCG_C2_HGO0_SHIFT                        (3U)
#define MCG_C2_HGO0(x)                           (((uint8_t)(((uint8_t)(x)) << MCG_C2_HGO0_SHIFT)) & MCG_C2_HGO0_MASK)
#define MCG_C2_RANGE0_MASK                       (0x30U)
#define MCG_C2_RANGE0_SHIFT                      (4U)
#define MCG_C2_RANGE0(x)                         (((uint8_t)(((uint8_t)(x)) << MCG_C2_RANGE0_SHIFT)) & MCG_C2_RANGE0_MASK)
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

/*! @name C5 - MCG Control 5 Register */
#define MCG_C5_PRDIV0_MASK                       (0x1FU)
#define MCG_C5_PRDIV0_SHIFT                      (0U)
#define MCG_C5_PRDIV0(x)                         (((uint8_t)(((uint8_t)(x)) << MCG_C5_PRDIV0_SHIFT)) & MCG_C5_PRDIV0_MASK)
#define MCG_C5_PLLSTEN0_MASK                     (0x20U)
#define MCG_C5_PLLSTEN0_SHIFT                    (5U)
#define MCG_C5_PLLSTEN0(x)                       (((uint8_t)(((uint8_t)(x)) << MCG_C5_PLLSTEN0_SHIFT)) & MCG_C5_PLLSTEN0_MASK)
#define MCG_C5_PLLCLKEN0_MASK                    (0x40U)
#define MCG_C5_PLLCLKEN0_SHIFT                   (6U)
#define MCG_C5_PLLCLKEN0(x)                      (((uint8_t)(((uint8_t)(x)) << MCG_C5_PLLCLKEN0_SHIFT)) & MCG_C5_PLLCLKEN0_MASK)

/*! @name C6 - MCG Control 6 Register */
#define MCG_C6_VDIV0_MASK                        (0x1FU)
#define MCG_C6_VDIV0_SHIFT                       (0U)
#define MCG_C6_VDIV0(x)                          (((uint8_t)(((uint8_t)(x)) << MCG_C6_VDIV0_SHIFT)) & MCG_C6_VDIV0_MASK)
#define MCG_C6_CME0_MASK                         (0x20U)
#define MCG_C6_CME0_SHIFT                        (5U)
#define MCG_C6_CME0(x)                           (((uint8_t)(((uint8_t)(x)) << MCG_C6_CME0_SHIFT)) & MCG_C6_CME0_MASK)
#define MCG_C6_PLLS_MASK                         (0x40U)
#define MCG_C6_PLLS_SHIFT                        (6U)
#define MCG_C6_PLLS(x)                           (((uint8_t)(((uint8_t)(x)) << MCG_C6_PLLS_SHIFT)) & MCG_C6_PLLS_MASK)
#define MCG_C6_LOLIE0_MASK                       (0x80U)
#define MCG_C6_LOLIE0_SHIFT                      (7U)
#define MCG_C6_LOLIE0(x)                         (((uint8_t)(((uint8_t)(x)) << MCG_C6_LOLIE0_SHIFT)) & MCG_C6_LOLIE0_MASK)

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
#define MCG_S_PLLST_MASK                         (0x20U)
#define MCG_S_PLLST_SHIFT                        (5U)
#define MCG_S_PLLST(x)                           (((uint8_t)(((uint8_t)(x)) << MCG_S_PLLST_SHIFT)) & MCG_S_PLLST_MASK)
#define MCG_S_LOCK0_MASK                         (0x40U)
#define MCG_S_LOCK0_SHIFT                        (6U)
#define MCG_S_LOCK0(x)                           (((uint8_t)(((uint8_t)(x)) << MCG_S_LOCK0_SHIFT)) & MCG_S_LOCK0_MASK)
#define MCG_S_LOLS0_MASK                         (0x80U)
#define MCG_S_LOLS0_SHIFT                        (7U)
#define MCG_S_LOLS0(x)                           (((uint8_t)(((uint8_t)(x)) << MCG_S_LOLS0_SHIFT)) & MCG_S_LOLS0_MASK)

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
#define MCG_C8_LOLRE_MASK                        (0x40U)
#define MCG_C8_LOLRE_SHIFT                       (6U)
#define MCG_C8_LOLRE(x)                          (((uint8_t)(((uint8_t)(x)) << MCG_C8_LOLRE_SHIFT)) & MCG_C8_LOLRE_MASK)
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
/* MCG C2[EREFS] backward compatibility */
#define MCG_C2_EREFS_MASK         (MCG_C2_EREFS0_MASK)
#define MCG_C2_EREFS_SHIFT        (MCG_C2_EREFS0_SHIFT)
#define MCG_C2_EREFS_WIDTH        (MCG_C2_EREFS0_WIDTH)
#define MCG_C2_EREFS(x)           (MCG_C2_EREFS0(x))

/* MCG C2[HGO] backward compatibility */
#define MCG_C2_HGO_MASK         (MCG_C2_HGO0_MASK)
#define MCG_C2_HGO_SHIFT        (MCG_C2_HGO0_SHIFT)
#define MCG_C2_HGO_WIDTH        (MCG_C2_HGO0_WIDTH)
#define MCG_C2_HGO(x)           (MCG_C2_HGO0(x))

/* MCG C2[RANGE] backward compatibility */
#define MCG_C2_RANGE_MASK         (MCG_C2_RANGE0_MASK)
#define MCG_C2_RANGE_SHIFT        (MCG_C2_RANGE0_SHIFT)
#define MCG_C2_RANGE_WIDTH        (MCG_C2_RANGE0_WIDTH)
#define MCG_C2_RANGE(x)           (MCG_C2_RANGE0(x))


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
  __IO uint32_t PLACR;                             /**< Crossbar Switch (AXBS) Control Register, offset: 0xC */
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

/*! @name PLACR - Crossbar Switch (AXBS) Control Register */
#define MCM_PLACR_ARB_MASK                       (0x200U)
#define MCM_PLACR_ARB_SHIFT                      (9U)
#define MCM_PLACR_ARB(x)                         (((uint32_t)(((uint32_t)(x)) << MCM_PLACR_ARB_SHIFT)) & MCM_PLACR_ARB_MASK)


/*!
 * @}
 */ /* end of group MCM_Register_Masks */


/* MCM - Peripheral instance base addresses */
/** Peripheral MCM base address */
#define MCM_BASE                                 (0xE0080000u)
/** Peripheral MCM base pointer */
#define MCM                                      ((MCM_Type *)MCM_BASE)
/** Array initializer of MCM peripheral base addresses */
#define MCM_BASE_ADDRS                           { MCM_BASE }
/** Array initializer of MCM peripheral base pointers */
#define MCM_BASE_PTRS                            { MCM }
/** Interrupt vectors for the MCM peripheral type */
#define MCM_IRQS                                 { MCM_IRQn }

/*!
 * @}
 */ /* end of group MCM_Peripheral_Access_Layer */


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
  __I  uint8_t FEPROT;                             /**< Non-volatile EERAM Protection Register, offset: 0xE */
  __I  uint8_t FDPROT;                             /**< Non-volatile D-Flash Protection Register, offset: 0xF */
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
#define NV_FOPT_LPBOOT_MASK                      (0x1U)
#define NV_FOPT_LPBOOT_SHIFT                     (0U)
#define NV_FOPT_LPBOOT(x)                        (((uint8_t)(((uint8_t)(x)) << NV_FOPT_LPBOOT_SHIFT)) & NV_FOPT_LPBOOT_MASK)
#define NV_FOPT_EZPORT_DIS_MASK                  (0x2U)
#define NV_FOPT_EZPORT_DIS_SHIFT                 (1U)
#define NV_FOPT_EZPORT_DIS(x)                    (((uint8_t)(((uint8_t)(x)) << NV_FOPT_EZPORT_DIS_SHIFT)) & NV_FOPT_EZPORT_DIS_MASK)
#define NV_FOPT_NMI_DIS_MASK                     (0x4U)
#define NV_FOPT_NMI_DIS_SHIFT                    (2U)
#define NV_FOPT_NMI_DIS(x)                       (((uint8_t)(((uint8_t)(x)) << NV_FOPT_NMI_DIS_SHIFT)) & NV_FOPT_NMI_DIS_MASK)

/*! @name FEPROT - Non-volatile EERAM Protection Register */
#define NV_FEPROT_EPROT_MASK                     (0xFFU)
#define NV_FEPROT_EPROT_SHIFT                    (0U)
#define NV_FEPROT_EPROT(x)                       (((uint8_t)(((uint8_t)(x)) << NV_FEPROT_EPROT_SHIFT)) & NV_FEPROT_EPROT_MASK)

/*! @name FDPROT - Non-volatile D-Flash Protection Register */
#define NV_FDPROT_DPROT_MASK                     (0xFFU)
#define NV_FDPROT_DPROT_SHIFT                    (0U)
#define NV_FDPROT_DPROT(x)                       (((uint8_t)(((uint8_t)(x)) << NV_FDPROT_DPROT_SHIFT)) & NV_FDPROT_DPROT_MASK)


/*!
 * @}
 */ /* end of group NV_Register_Masks */


/* NV - Peripheral instance base addresses */
/** Peripheral FTFL_FlashConfig base address */
#define FTFL_FlashConfig_BASE                    (0x400u)
/** Peripheral FTFL_FlashConfig base pointer */
#define FTFL_FlashConfig                         ((NV_Type *)FTFL_FlashConfig_BASE)
/** Array initializer of NV peripheral base addresses */
#define NV_BASE_ADDRS                            { FTFL_FlashConfig_BASE }
/** Array initializer of NV peripheral base pointers */
#define NV_BASE_PTRS                             { FTFL_FlashConfig }

/*!
 * @}
 */ /* end of group NV_Peripheral_Access_Layer */


/* ----------------------------------------------------------------------------
   -- OSC Peripheral Access Layer
   ---------------------------------------------------------------------------- */

/*!
 * @addtogroup OSC_Peripheral_Access_Layer OSC Peripheral Access Layer
 * @{
 */

/** OSC - Register Layout Typedef */
typedef struct {
  __IO uint8_t CR;                                 /**< OSC Control Register, offset: 0x0 */
} OSC_Type;

/* ----------------------------------------------------------------------------
   -- OSC Register Masks
   ---------------------------------------------------------------------------- */

/*!
 * @addtogroup OSC_Register_Masks OSC Register Masks
 * @{
 */

/*! @name CR - OSC Control Register */
#define OSC_CR_SC16P_MASK                        (0x1U)
#define OSC_CR_SC16P_SHIFT                       (0U)
#define OSC_CR_SC16P(x)                          (((uint8_t)(((uint8_t)(x)) << OSC_CR_SC16P_SHIFT)) & OSC_CR_SC16P_MASK)
#define OSC_CR_SC8P_MASK                         (0x2U)
#define OSC_CR_SC8P_SHIFT                        (1U)
#define OSC_CR_SC8P(x)                           (((uint8_t)(((uint8_t)(x)) << OSC_CR_SC8P_SHIFT)) & OSC_CR_SC8P_MASK)
#define OSC_CR_SC4P_MASK                         (0x4U)
#define OSC_CR_SC4P_SHIFT                        (2U)
#define OSC_CR_SC4P(x)                           (((uint8_t)(((uint8_t)(x)) << OSC_CR_SC4P_SHIFT)) & OSC_CR_SC4P_MASK)
#define OSC_CR_SC2P_MASK                         (0x8U)
#define OSC_CR_SC2P_SHIFT                        (3U)
#define OSC_CR_SC2P(x)                           (((uint8_t)(((uint8_t)(x)) << OSC_CR_SC2P_SHIFT)) & OSC_CR_SC2P_MASK)
#define OSC_CR_EREFSTEN_MASK                     (0x20U)
#define OSC_CR_EREFSTEN_SHIFT                    (5U)
#define OSC_CR_EREFSTEN(x)                       (((uint8_t)(((uint8_t)(x)) << OSC_CR_EREFSTEN_SHIFT)) & OSC_CR_EREFSTEN_MASK)
#define OSC_CR_ERCLKEN_MASK                      (0x80U)
#define OSC_CR_ERCLKEN_SHIFT                     (7U)
#define OSC_CR_ERCLKEN(x)                        (((uint8_t)(((uint8_t)(x)) << OSC_CR_ERCLKEN_SHIFT)) & OSC_CR_ERCLKEN_MASK)


/*!
 * @}
 */ /* end of group OSC_Register_Masks */


/* OSC - Peripheral instance base addresses */
/** Peripheral OSC base address */
#define OSC_BASE                                 (0x40065000u)
/** Peripheral OSC base pointer */
#define OSC                                      ((OSC_Type *)OSC_BASE)
/** Array initializer of OSC peripheral base addresses */
#define OSC_BASE_ADDRS                           { OSC_BASE }
/** Array initializer of OSC peripheral base pointers */
#define OSC_BASE_PTRS                            { OSC }

/*!
 * @}
 */ /* end of group OSC_Peripheral_Access_Layer */


/* ----------------------------------------------------------------------------
   -- PDB Peripheral Access Layer
   ---------------------------------------------------------------------------- */

/*!
 * @addtogroup PDB_Peripheral_Access_Layer PDB Peripheral Access Layer
 * @{
 */

/** PDB - Register Layout Typedef */
typedef struct {
  __IO uint32_t SC;                                /**< Status and Control Register, offset: 0x0 */
  __IO uint32_t MOD;                               /**< Modulus Register, offset: 0x4 */
  __I  uint32_t CNT;                               /**< Counter Register, offset: 0x8 */
  __IO uint32_t IDLY;                              /**< Interrupt Delay Register, offset: 0xC */
  struct {                                         /* offset: 0x10, array step: 0x28 */
    __IO uint32_t C1;                                /**< Channel n Control Register 1, array offset: 0x10, array step: 0x28 */
    __IO uint32_t S;                                 /**< Channel n Status Register, array offset: 0x14, array step: 0x28 */
    __IO uint32_t DLY[2];                            /**< Channel n Delay 0 Register..Channel n Delay 1 Register, array offset: 0x18, array step: index*0x28, index2*0x4 */
         uint8_t RESERVED_0[24];
  } CH[2];
       uint8_t RESERVED_0[240];
  struct {                                         /* offset: 0x150, array step: 0x8 */
    __IO uint32_t INTC;                              /**< DAC Interval Trigger n Control Register, array offset: 0x150, array step: 0x8 */
    __IO uint32_t INT;                               /**< DAC Interval n Register, array offset: 0x154, array step: 0x8 */
  } DAC[1];
       uint8_t RESERVED_1[56];
  __IO uint32_t POEN;                              /**< Pulse-Out n Enable Register, offset: 0x190 */
  __IO uint32_t PODLY[2];                          /**< Pulse-Out n Delay Register, array offset: 0x194, array step: 0x4 */
} PDB_Type;

/* ----------------------------------------------------------------------------
   -- PDB Register Masks
   ---------------------------------------------------------------------------- */

/*!
 * @addtogroup PDB_Register_Masks PDB Register Masks
 * @{
 */

/*! @name SC - Status and Control Register */
#define PDB_SC_LDOK_MASK                         (0x1U)
#define PDB_SC_LDOK_SHIFT                        (0U)
#define PDB_SC_LDOK(x)                           (((uint32_t)(((uint32_t)(x)) << PDB_SC_LDOK_SHIFT)) & PDB_SC_LDOK_MASK)
#define PDB_SC_CONT_MASK                         (0x2U)
#define PDB_SC_CONT_SHIFT                        (1U)
#define PDB_SC_CONT(x)                           (((uint32_t)(((uint32_t)(x)) << PDB_SC_CONT_SHIFT)) & PDB_SC_CONT_MASK)
#define PDB_SC_MULT_MASK                         (0xCU)
#define PDB_SC_MULT_SHIFT                        (2U)
#define PDB_SC_MULT(x)                           (((uint32_t)(((uint32_t)(x)) << PDB_SC_MULT_SHIFT)) & PDB_SC_MULT_MASK)
#define PDB_SC_PDBIE_MASK                        (0x20U)
#define PDB_SC_PDBIE_SHIFT                       (5U)
#define PDB_SC_PDBIE(x)                          (((uint32_t)(((uint32_t)(x)) << PDB_SC_PDBIE_SHIFT)) & PDB_SC_PDBIE_MASK)
#define PDB_SC_PDBIF_MASK                        (0x40U)
#define PDB_SC_PDBIF_SHIFT                       (6U)
#define PDB_SC_PDBIF(x)                          (((uint32_t)(((uint32_t)(x)) << PDB_SC_PDBIF_SHIFT)) & PDB_SC_PDBIF_MASK)
#define PDB_SC_PDBEN_MASK                        (0x80U)
#define PDB_SC_PDBEN_SHIFT                       (7U)
#define PDB_SC_PDBEN(x)                          (((uint32_t)(((uint32_t)(x)) << PDB_SC_PDBEN_SHIFT)) & PDB_SC_PDBEN_MASK)
#define PDB_SC_TRGSEL_MASK                       (0xF00U)
#define PDB_SC_TRGSEL_SHIFT                      (8U)
#define PDB_SC_TRGSEL(x)                         (((uint32_t)(((uint32_t)(x)) << PDB_SC_TRGSEL_SHIFT)) & PDB_SC_TRGSEL_MASK)
#define PDB_SC_PRESCALER_MASK                    (0x7000U)
#define PDB_SC_PRESCALER_SHIFT                   (12U)
#define PDB_SC_PRESCALER(x)                      (((uint32_t)(((uint32_t)(x)) << PDB_SC_PRESCALER_SHIFT)) & PDB_SC_PRESCALER_MASK)
#define PDB_SC_DMAEN_MASK                        (0x8000U)
#define PDB_SC_DMAEN_SHIFT                       (15U)
#define PDB_SC_DMAEN(x)                          (((uint32_t)(((uint32_t)(x)) << PDB_SC_DMAEN_SHIFT)) & PDB_SC_DMAEN_MASK)
#define PDB_SC_SWTRIG_MASK                       (0x10000U)
#define PDB_SC_SWTRIG_SHIFT                      (16U)
#define PDB_SC_SWTRIG(x)                         (((uint32_t)(((uint32_t)(x)) << PDB_SC_SWTRIG_SHIFT)) & PDB_SC_SWTRIG_MASK)
#define PDB_SC_PDBEIE_MASK                       (0x20000U)
#define PDB_SC_PDBEIE_SHIFT                      (17U)
#define PDB_SC_PDBEIE(x)                         (((uint32_t)(((uint32_t)(x)) << PDB_SC_PDBEIE_SHIFT)) & PDB_SC_PDBEIE_MASK)
#define PDB_SC_LDMOD_MASK                        (0xC0000U)
#define PDB_SC_LDMOD_SHIFT                       (18U)
#define PDB_SC_LDMOD(x)                          (((uint32_t)(((uint32_t)(x)) << PDB_SC_LDMOD_SHIFT)) & PDB_SC_LDMOD_MASK)

/*! @name MOD - Modulus Register */
#define PDB_MOD_MOD_MASK                         (0xFFFFU)
#define PDB_MOD_MOD_SHIFT                        (0U)
#define PDB_MOD_MOD(x)                           (((uint32_t)(((uint32_t)(x)) << PDB_MOD_MOD_SHIFT)) & PDB_MOD_MOD_MASK)

/*! @name CNT - Counter Register */
#define PDB_CNT_CNT_MASK                         (0xFFFFU)
#define PDB_CNT_CNT_SHIFT                        (0U)
#define PDB_CNT_CNT(x)                           (((uint32_t)(((uint32_t)(x)) << PDB_CNT_CNT_SHIFT)) & PDB_CNT_CNT_MASK)

/*! @name IDLY - Interrupt Delay Register */
#define PDB_IDLY_IDLY_MASK                       (0xFFFFU)
#define PDB_IDLY_IDLY_SHIFT                      (0U)
#define PDB_IDLY_IDLY(x)                         (((uint32_t)(((uint32_t)(x)) << PDB_IDLY_IDLY_SHIFT)) & PDB_IDLY_IDLY_MASK)

/*! @name C1 - Channel n Control Register 1 */
#define PDB_C1_EN_MASK                           (0xFFU)
#define PDB_C1_EN_SHIFT                          (0U)
#define PDB_C1_EN(x)                             (((uint32_t)(((uint32_t)(x)) << PDB_C1_EN_SHIFT)) & PDB_C1_EN_MASK)
#define PDB_C1_TOS_MASK                          (0xFF00U)
#define PDB_C1_TOS_SHIFT                         (8U)
#define PDB_C1_TOS(x)                            (((uint32_t)(((uint32_t)(x)) << PDB_C1_TOS_SHIFT)) & PDB_C1_TOS_MASK)
#define PDB_C1_BB_MASK                           (0xFF0000U)
#define PDB_C1_BB_SHIFT                          (16U)
#define PDB_C1_BB(x)                             (((uint32_t)(((uint32_t)(x)) << PDB_C1_BB_SHIFT)) & PDB_C1_BB_MASK)

/* The count of PDB_C1 */
#define PDB_C1_COUNT                             (2U)

/*! @name S - Channel n Status Register */
#define PDB_S_ERR_MASK                           (0xFFU)
#define PDB_S_ERR_SHIFT                          (0U)
#define PDB_S_ERR(x)                             (((uint32_t)(((uint32_t)(x)) << PDB_S_ERR_SHIFT)) & PDB_S_ERR_MASK)
#define PDB_S_CF_MASK                            (0xFF0000U)
#define PDB_S_CF_SHIFT                           (16U)
#define PDB_S_CF(x)                              (((uint32_t)(((uint32_t)(x)) << PDB_S_CF_SHIFT)) & PDB_S_CF_MASK)

/* The count of PDB_S */
#define PDB_S_COUNT                              (2U)

/*! @name DLY - Channel n Delay 0 Register..Channel n Delay 1 Register */
#define PDB_DLY_DLY_MASK                         (0xFFFFU)
#define PDB_DLY_DLY_SHIFT                        (0U)
#define PDB_DLY_DLY(x)                           (((uint32_t)(((uint32_t)(x)) << PDB_DLY_DLY_SHIFT)) & PDB_DLY_DLY_MASK)

/* The count of PDB_DLY */
#define PDB_DLY_COUNT                            (2U)

/* The count of PDB_DLY */
#define PDB_DLY_COUNT2                           (2U)

/*! @name INTC - DAC Interval Trigger n Control Register */
#define PDB_INTC_TOE_MASK                        (0x1U)
#define PDB_INTC_TOE_SHIFT                       (0U)
#define PDB_INTC_TOE(x)                          (((uint32_t)(((uint32_t)(x)) << PDB_INTC_TOE_SHIFT)) & PDB_INTC_TOE_MASK)
#define PDB_INTC_EXT_MASK                        (0x2U)
#define PDB_INTC_EXT_SHIFT                       (1U)
#define PDB_INTC_EXT(x)                          (((uint32_t)(((uint32_t)(x)) << PDB_INTC_EXT_SHIFT)) & PDB_INTC_EXT_MASK)

/* The count of PDB_INTC */
#define PDB_INTC_COUNT                           (1U)

/*! @name INT - DAC Interval n Register */
#define PDB_INT_INT_MASK                         (0xFFFFU)
#define PDB_INT_INT_SHIFT                        (0U)
#define PDB_INT_INT(x)                           (((uint32_t)(((uint32_t)(x)) << PDB_INT_INT_SHIFT)) & PDB_INT_INT_MASK)

/* The count of PDB_INT */
#define PDB_INT_COUNT                            (1U)

/*! @name POEN - Pulse-Out n Enable Register */
#define PDB_POEN_POEN_MASK                       (0xFFU)
#define PDB_POEN_POEN_SHIFT                      (0U)
#define PDB_POEN_POEN(x)                         (((uint32_t)(((uint32_t)(x)) << PDB_POEN_POEN_SHIFT)) & PDB_POEN_POEN_MASK)

/*! @name PODLY - Pulse-Out n Delay Register */
#define PDB_PODLY_DLY2_MASK                      (0xFFFFU)
#define PDB_PODLY_DLY2_SHIFT                     (0U)
#define PDB_PODLY_DLY2(x)                        (((uint32_t)(((uint32_t)(x)) << PDB_PODLY_DLY2_SHIFT)) & PDB_PODLY_DLY2_MASK)
#define PDB_PODLY_DLY1_MASK                      (0xFFFF0000U)
#define PDB_PODLY_DLY1_SHIFT                     (16U)
#define PDB_PODLY_DLY1(x)                        (((uint32_t)(((uint32_t)(x)) << PDB_PODLY_DLY1_SHIFT)) & PDB_PODLY_DLY1_MASK)

/* The count of PDB_PODLY */
#define PDB_PODLY_COUNT                          (2U)


/*!
 * @}
 */ /* end of group PDB_Register_Masks */


/* PDB - Peripheral instance base addresses */
/** Peripheral PDB0 base address */
#define PDB0_BASE                                (0x40036000u)
/** Peripheral PDB0 base pointer */
#define PDB0                                     ((PDB_Type *)PDB0_BASE)
/** Array initializer of PDB peripheral base addresses */
#define PDB_BASE_ADDRS                           { PDB0_BASE }
/** Array initializer of PDB peripheral base pointers */
#define PDB_BASE_PTRS                            { PDB0 }
/** Interrupt vectors for the PDB peripheral type */
#define PDB_IRQS                                 { PDB0_IRQn }

/*!
 * @}
 */ /* end of group PDB_Peripheral_Access_Layer */


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
       uint8_t RESERVED_0[252];
  struct {                                         /* offset: 0x100, array step: 0x10 */
    __IO uint32_t LDVAL;                             /**< Timer Load Value Register, array offset: 0x100, array step: 0x10 */
    __I  uint32_t CVAL;                              /**< Current Timer Value Register, array offset: 0x104, array step: 0x10 */
    __IO uint32_t TCTRL;                             /**< Timer Control Register, array offset: 0x108, array step: 0x10 */
    __IO uint32_t TFLG;                              /**< Timer Flag Register, array offset: 0x10C, array step: 0x10 */
  } CHANNEL[4];
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

/*! @name LDVAL - Timer Load Value Register */
#define PIT_LDVAL_TSV_MASK                       (0xFFFFFFFFU)
#define PIT_LDVAL_TSV_SHIFT                      (0U)
#define PIT_LDVAL_TSV(x)                         (((uint32_t)(((uint32_t)(x)) << PIT_LDVAL_TSV_SHIFT)) & PIT_LDVAL_TSV_MASK)

/* The count of PIT_LDVAL */
#define PIT_LDVAL_COUNT                          (4U)

/*! @name CVAL - Current Timer Value Register */
#define PIT_CVAL_TVL_MASK                        (0xFFFFFFFFU)
#define PIT_CVAL_TVL_SHIFT                       (0U)
#define PIT_CVAL_TVL(x)                          (((uint32_t)(((uint32_t)(x)) << PIT_CVAL_TVL_SHIFT)) & PIT_CVAL_TVL_MASK)

/* The count of PIT_CVAL */
#define PIT_CVAL_COUNT                           (4U)

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
#define PIT_TCTRL_COUNT                          (4U)

/*! @name TFLG - Timer Flag Register */
#define PIT_TFLG_TIF_MASK                        (0x1U)
#define PIT_TFLG_TIF_SHIFT                       (0U)
#define PIT_TFLG_TIF(x)                          (((uint32_t)(((uint32_t)(x)) << PIT_TFLG_TIF_SHIFT)) & PIT_TFLG_TIF_MASK)

/* The count of PIT_TFLG */
#define PIT_TFLG_COUNT                           (4U)


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
#define PIT_IRQS                                 { { PIT0_IRQn, PIT1_IRQn, PIT2_IRQn, PIT3_IRQn } }

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
#define PMC_REGSC_BGEN_MASK                      (0x10U)
#define PMC_REGSC_BGEN_SHIFT                     (4U)
#define PMC_REGSC_BGEN(x)                        (((uint8_t)(((uint8_t)(x)) << PMC_REGSC_BGEN_SHIFT)) & PMC_REGSC_BGEN_MASK)


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
#define PMC_IRQS                                 { PMC_IRQn }

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
       uint8_t RESERVED_1[28];
  __IO uint32_t DFER;                              /**< Digital Filter Enable Register, offset: 0xC0 */
  __IO uint32_t DFCR;                              /**< Digital Filter Clock Register, offset: 0xC4 */
  __IO uint32_t DFWR;                              /**< Digital Filter Width Register, offset: 0xC8 */
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
#define PORT_PCR_ODE_MASK                        (0x20U)
#define PORT_PCR_ODE_SHIFT                       (5U)
#define PORT_PCR_ODE(x)                          (((uint32_t)(((uint32_t)(x)) << PORT_PCR_ODE_SHIFT)) & PORT_PCR_ODE_MASK)
#define PORT_PCR_DSE_MASK                        (0x40U)
#define PORT_PCR_DSE_SHIFT                       (6U)
#define PORT_PCR_DSE(x)                          (((uint32_t)(((uint32_t)(x)) << PORT_PCR_DSE_SHIFT)) & PORT_PCR_DSE_MASK)
#define PORT_PCR_MUX_MASK                        (0x700U)
#define PORT_PCR_MUX_SHIFT                       (8U)
#define PORT_PCR_MUX(x)                          (((uint32_t)(((uint32_t)(x)) << PORT_PCR_MUX_SHIFT)) & PORT_PCR_MUX_MASK)
#define PORT_PCR_LK_MASK                         (0x8000U)
#define PORT_PCR_LK_SHIFT                        (15U)
#define PORT_PCR_LK(x)                           (((uint32_t)(((uint32_t)(x)) << PORT_PCR_LK_SHIFT)) & PORT_PCR_LK_MASK)
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

/*! @name DFER - Digital Filter Enable Register */
#define PORT_DFER_DFE_MASK                       (0xFFFFFFFFU)
#define PORT_DFER_DFE_SHIFT                      (0U)
#define PORT_DFER_DFE(x)                         (((uint32_t)(((uint32_t)(x)) << PORT_DFER_DFE_SHIFT)) & PORT_DFER_DFE_MASK)

/*! @name DFCR - Digital Filter Clock Register */
#define PORT_DFCR_CS_MASK                        (0x1U)
#define PORT_DFCR_CS_SHIFT                       (0U)
#define PORT_DFCR_CS(x)                          (((uint32_t)(((uint32_t)(x)) << PORT_DFCR_CS_SHIFT)) & PORT_DFCR_CS_MASK)

/*! @name DFWR - Digital Filter Width Register */
#define PORT_DFWR_FILT_MASK                      (0x1FU)
#define PORT_DFWR_FILT_SHIFT                     (0U)
#define PORT_DFWR_FILT(x)                        (((uint32_t)(((uint32_t)(x)) << PORT_DFWR_FILT_SHIFT)) & PORT_DFWR_FILT_MASK)


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
/** Peripheral PORTD base address */
#define PORTD_BASE                               (0x4004C000u)
/** Peripheral PORTD base pointer */
#define PORTD                                    ((PORT_Type *)PORTD_BASE)
/** Peripheral PORTE base address */
#define PORTE_BASE                               (0x4004D000u)
/** Peripheral PORTE base pointer */
#define PORTE                                    ((PORT_Type *)PORTE_BASE)
/** Array initializer of PORT peripheral base addresses */
#define PORT_BASE_ADDRS                          { PORTA_BASE, PORTB_BASE, PORTC_BASE, PORTD_BASE, PORTE_BASE }
/** Array initializer of PORT peripheral base pointers */
#define PORT_BASE_PTRS                           { PORTA, PORTB, PORTC, PORTD, PORTE }
/** Interrupt vectors for the PORT peripheral type */
#define PORT_IRQS                                { PORTA_IRQn, PORTB_IRQn, PORTC_IRQn, PORTD_IRQn, PORTE_IRQn }

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
       uint8_t RESERVED_1[1];
  __I  uint8_t MR;                                 /**< Mode Register, offset: 0x7 */
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
#define RCM_SRS0_LOL_MASK                        (0x8U)
#define RCM_SRS0_LOL_SHIFT                       (3U)
#define RCM_SRS0_LOL(x)                          (((uint8_t)(((uint8_t)(x)) << RCM_SRS0_LOL_SHIFT)) & RCM_SRS0_LOL_MASK)
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
#define RCM_SRS1_JTAG_MASK                       (0x1U)
#define RCM_SRS1_JTAG_SHIFT                      (0U)
#define RCM_SRS1_JTAG(x)                         (((uint8_t)(((uint8_t)(x)) << RCM_SRS1_JTAG_SHIFT)) & RCM_SRS1_JTAG_MASK)
#define RCM_SRS1_LOCKUP_MASK                     (0x2U)
#define RCM_SRS1_LOCKUP_SHIFT                    (1U)
#define RCM_SRS1_LOCKUP(x)                       (((uint8_t)(((uint8_t)(x)) << RCM_SRS1_LOCKUP_SHIFT)) & RCM_SRS1_LOCKUP_MASK)
#define RCM_SRS1_SW_MASK                         (0x4U)
#define RCM_SRS1_SW_SHIFT                        (2U)
#define RCM_SRS1_SW(x)                           (((uint8_t)(((uint8_t)(x)) << RCM_SRS1_SW_SHIFT)) & RCM_SRS1_SW_MASK)
#define RCM_SRS1_MDM_AP_MASK                     (0x8U)
#define RCM_SRS1_MDM_AP_SHIFT                    (3U)
#define RCM_SRS1_MDM_AP(x)                       (((uint8_t)(((uint8_t)(x)) << RCM_SRS1_MDM_AP_SHIFT)) & RCM_SRS1_MDM_AP_MASK)
#define RCM_SRS1_EZPT_MASK                       (0x10U)
#define RCM_SRS1_EZPT_SHIFT                      (4U)
#define RCM_SRS1_EZPT(x)                         (((uint8_t)(((uint8_t)(x)) << RCM_SRS1_EZPT_SHIFT)) & RCM_SRS1_EZPT_MASK)
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

/*! @name MR - Mode Register */
#define RCM_MR_EZP_MS_MASK                       (0x2U)
#define RCM_MR_EZP_MS_SHIFT                      (1U)
#define RCM_MR_EZP_MS(x)                         (((uint8_t)(((uint8_t)(x)) << RCM_MR_EZP_MS_SHIFT)) & RCM_MR_EZP_MS_MASK)


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
   -- RFVBAT Peripheral Access Layer
   ---------------------------------------------------------------------------- */

/*!
 * @addtogroup RFVBAT_Peripheral_Access_Layer RFVBAT Peripheral Access Layer
 * @{
 */

/** RFVBAT - Register Layout Typedef */
typedef struct {
  __IO uint32_t REG[8];                            /**< VBAT register file register, array offset: 0x0, array step: 0x4 */
} RFVBAT_Type;

/* ----------------------------------------------------------------------------
   -- RFVBAT Register Masks
   ---------------------------------------------------------------------------- */

/*!
 * @addtogroup RFVBAT_Register_Masks RFVBAT Register Masks
 * @{
 */

/*! @name REG - VBAT register file register */
#define RFVBAT_REG_LL_MASK                       (0xFFU)
#define RFVBAT_REG_LL_SHIFT                      (0U)
#define RFVBAT_REG_LL(x)                         (((uint32_t)(((uint32_t)(x)) << RFVBAT_REG_LL_SHIFT)) & RFVBAT_REG_LL_MASK)
#define RFVBAT_REG_LH_MASK                       (0xFF00U)
#define RFVBAT_REG_LH_SHIFT                      (8U)
#define RFVBAT_REG_LH(x)                         (((uint32_t)(((uint32_t)(x)) << RFVBAT_REG_LH_SHIFT)) & RFVBAT_REG_LH_MASK)
#define RFVBAT_REG_HL_MASK                       (0xFF0000U)
#define RFVBAT_REG_HL_SHIFT                      (16U)
#define RFVBAT_REG_HL(x)                         (((uint32_t)(((uint32_t)(x)) << RFVBAT_REG_HL_SHIFT)) & RFVBAT_REG_HL_MASK)
#define RFVBAT_REG_HH_MASK                       (0xFF000000U)
#define RFVBAT_REG_HH_SHIFT                      (24U)
#define RFVBAT_REG_HH(x)                         (((uint32_t)(((uint32_t)(x)) << RFVBAT_REG_HH_SHIFT)) & RFVBAT_REG_HH_MASK)

/* The count of RFVBAT_REG */
#define RFVBAT_REG_COUNT                         (8U)


/*!
 * @}
 */ /* end of group RFVBAT_Register_Masks */


/* RFVBAT - Peripheral instance base addresses */
/** Peripheral RFVBAT base address */
#define RFVBAT_BASE                              (0x4003E000u)
/** Peripheral RFVBAT base pointer */
#define RFVBAT                                   ((RFVBAT_Type *)RFVBAT_BASE)
/** Array initializer of RFVBAT peripheral base addresses */
#define RFVBAT_BASE_ADDRS                        { RFVBAT_BASE }
/** Array initializer of RFVBAT peripheral base pointers */
#define RFVBAT_BASE_PTRS                         { RFVBAT }

/*!
 * @}
 */ /* end of group RFVBAT_Peripheral_Access_Layer */


/* ----------------------------------------------------------------------------
   -- RNG Peripheral Access Layer
   ---------------------------------------------------------------------------- */

/*!
 * @addtogroup RNG_Peripheral_Access_Layer RNG Peripheral Access Layer
 * @{
 */

/** RNG - Register Layout Typedef */
typedef struct {
  __IO uint32_t CR;                                /**< RNGA Control Register, offset: 0x0 */
  __I  uint32_t SR;                                /**< RNGA Status Register, offset: 0x4 */
  __O  uint32_t ER;                                /**< RNGA Entropy Register, offset: 0x8 */
  __I  uint32_t OR;                                /**< RNGA Output Register, offset: 0xC */
} RNG_Type;

/* ----------------------------------------------------------------------------
   -- RNG Register Masks
   ---------------------------------------------------------------------------- */

/*!
 * @addtogroup RNG_Register_Masks RNG Register Masks
 * @{
 */

/*! @name CR - RNGA Control Register */
#define RNG_CR_GO_MASK                           (0x1U)
#define RNG_CR_GO_SHIFT                          (0U)
#define RNG_CR_GO(x)                             (((uint32_t)(((uint32_t)(x)) << RNG_CR_GO_SHIFT)) & RNG_CR_GO_MASK)
#define RNG_CR_HA_MASK                           (0x2U)
#define RNG_CR_HA_SHIFT                          (1U)
#define RNG_CR_HA(x)                             (((uint32_t)(((uint32_t)(x)) << RNG_CR_HA_SHIFT)) & RNG_CR_HA_MASK)
#define RNG_CR_INTM_MASK                         (0x4U)
#define RNG_CR_INTM_SHIFT                        (2U)
#define RNG_CR_INTM(x)                           (((uint32_t)(((uint32_t)(x)) << RNG_CR_INTM_SHIFT)) & RNG_CR_INTM_MASK)
#define RNG_CR_CLRI_MASK                         (0x8U)
#define RNG_CR_CLRI_SHIFT                        (3U)
#define RNG_CR_CLRI(x)                           (((uint32_t)(((uint32_t)(x)) << RNG_CR_CLRI_SHIFT)) & RNG_CR_CLRI_MASK)
#define RNG_CR_SLP_MASK                          (0x10U)
#define RNG_CR_SLP_SHIFT                         (4U)
#define RNG_CR_SLP(x)                            (((uint32_t)(((uint32_t)(x)) << RNG_CR_SLP_SHIFT)) & RNG_CR_SLP_MASK)

/*! @name SR - RNGA Status Register */
#define RNG_SR_SECV_MASK                         (0x1U)
#define RNG_SR_SECV_SHIFT                        (0U)
#define RNG_SR_SECV(x)                           (((uint32_t)(((uint32_t)(x)) << RNG_SR_SECV_SHIFT)) & RNG_SR_SECV_MASK)
#define RNG_SR_LRS_MASK                          (0x2U)
#define RNG_SR_LRS_SHIFT                         (1U)
#define RNG_SR_LRS(x)                            (((uint32_t)(((uint32_t)(x)) << RNG_SR_LRS_SHIFT)) & RNG_SR_LRS_MASK)
#define RNG_SR_ORU_MASK                          (0x4U)
#define RNG_SR_ORU_SHIFT                         (2U)
#define RNG_SR_ORU(x)                            (((uint32_t)(((uint32_t)(x)) << RNG_SR_ORU_SHIFT)) & RNG_SR_ORU_MASK)
#define RNG_SR_ERRI_MASK                         (0x8U)
#define RNG_SR_ERRI_SHIFT                        (3U)
#define RNG_SR_ERRI(x)                           (((uint32_t)(((uint32_t)(x)) << RNG_SR_ERRI_SHIFT)) & RNG_SR_ERRI_MASK)
#define RNG_SR_SLP_MASK                          (0x10U)
#define RNG_SR_SLP_SHIFT                         (4U)
#define RNG_SR_SLP(x)                            (((uint32_t)(((uint32_t)(x)) << RNG_SR_SLP_SHIFT)) & RNG_SR_SLP_MASK)
#define RNG_SR_OREG_LVL_MASK                     (0xFF00U)
#define RNG_SR_OREG_LVL_SHIFT                    (8U)
#define RNG_SR_OREG_LVL(x)                       (((uint32_t)(((uint32_t)(x)) << RNG_SR_OREG_LVL_SHIFT)) & RNG_SR_OREG_LVL_MASK)
#define RNG_SR_OREG_SIZE_MASK                    (0xFF0000U)
#define RNG_SR_OREG_SIZE_SHIFT                   (16U)
#define RNG_SR_OREG_SIZE(x)                      (((uint32_t)(((uint32_t)(x)) << RNG_SR_OREG_SIZE_SHIFT)) & RNG_SR_OREG_SIZE_MASK)

/*! @name ER - RNGA Entropy Register */
#define RNG_ER_EXT_ENT_MASK                      (0xFFFFFFFFU)
#define RNG_ER_EXT_ENT_SHIFT                     (0U)
#define RNG_ER_EXT_ENT(x)                        (((uint32_t)(((uint32_t)(x)) << RNG_ER_EXT_ENT_SHIFT)) & RNG_ER_EXT_ENT_MASK)

/*! @name OR - RNGA Output Register */
#define RNG_OR_RANDOUT_MASK                      (0xFFFFFFFFU)
#define RNG_OR_RANDOUT_SHIFT                     (0U)
#define RNG_OR_RANDOUT(x)                        (((uint32_t)(((uint32_t)(x)) << RNG_OR_RANDOUT_SHIFT)) & RNG_OR_RANDOUT_MASK)


/*!
 * @}
 */ /* end of group RNG_Register_Masks */


/* RNG - Peripheral instance base addresses */
/** Peripheral RNG base address */
#define RNG_BASE                                 (0x40029000u)
/** Peripheral RNG base pointer */
#define RNG                                      ((RNG_Type *)RNG_BASE)
/** Array initializer of RNG peripheral base addresses */
#define RNG_BASE_ADDRS                           { RNG_BASE }
/** Array initializer of RNG peripheral base pointers */
#define RNG_BASE_PTRS                            { RNG }
/** Interrupt vectors for the RNG peripheral type */
#define RNG_IRQS                                 { RNG_IRQn }

/*!
 * @}
 */ /* end of group RNG_Peripheral_Access_Layer */


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
  __I  uint32_t TTSR;                              /**< RTC Tamper Time Seconds Register, offset: 0x20 */
  __IO uint32_t MER;                               /**< RTC Monotonic Enable Register, offset: 0x24 */
  __IO uint32_t MCLR;                              /**< RTC Monotonic Counter Low Register, offset: 0x28 */
  __IO uint32_t MCHR;                              /**< RTC Monotonic Counter High Register, offset: 0x2C */
       uint8_t RESERVED_0[2000];
  __IO uint32_t WAR;                               /**< RTC Write Access Register, offset: 0x800 */
  __IO uint32_t RAR;                               /**< RTC Read Access Register, offset: 0x804 */
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
#define RTC_SR_MOF_MASK                          (0x8U)
#define RTC_SR_MOF_SHIFT                         (3U)
#define RTC_SR_MOF(x)                            (((uint32_t)(((uint32_t)(x)) << RTC_SR_MOF_SHIFT)) & RTC_SR_MOF_MASK)
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
#define RTC_LR_TTSL_MASK                         (0x100U)
#define RTC_LR_TTSL_SHIFT                        (8U)
#define RTC_LR_TTSL(x)                           (((uint32_t)(((uint32_t)(x)) << RTC_LR_TTSL_SHIFT)) & RTC_LR_TTSL_MASK)
#define RTC_LR_MEL_MASK                          (0x200U)
#define RTC_LR_MEL_SHIFT                         (9U)
#define RTC_LR_MEL(x)                            (((uint32_t)(((uint32_t)(x)) << RTC_LR_MEL_SHIFT)) & RTC_LR_MEL_MASK)
#define RTC_LR_MCLL_MASK                         (0x400U)
#define RTC_LR_MCLL_SHIFT                        (10U)
#define RTC_LR_MCLL(x)                           (((uint32_t)(((uint32_t)(x)) << RTC_LR_MCLL_SHIFT)) & RTC_LR_MCLL_MASK)
#define RTC_LR_MCHL_MASK                         (0x800U)
#define RTC_LR_MCHL_SHIFT                        (11U)
#define RTC_LR_MCHL(x)                           (((uint32_t)(((uint32_t)(x)) << RTC_LR_MCHL_SHIFT)) & RTC_LR_MCHL_MASK)

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
#define RTC_IER_MOIE_MASK                        (0x8U)
#define RTC_IER_MOIE_SHIFT                       (3U)
#define RTC_IER_MOIE(x)                          (((uint32_t)(((uint32_t)(x)) << RTC_IER_MOIE_SHIFT)) & RTC_IER_MOIE_MASK)
#define RTC_IER_TSIE_MASK                        (0x10U)
#define RTC_IER_TSIE_SHIFT                       (4U)
#define RTC_IER_TSIE(x)                          (((uint32_t)(((uint32_t)(x)) << RTC_IER_TSIE_SHIFT)) & RTC_IER_TSIE_MASK)
#define RTC_IER_WPON_MASK                        (0x80U)
#define RTC_IER_WPON_SHIFT                       (7U)
#define RTC_IER_WPON(x)                          (((uint32_t)(((uint32_t)(x)) << RTC_IER_WPON_SHIFT)) & RTC_IER_WPON_MASK)

/*! @name TTSR - RTC Tamper Time Seconds Register */
#define RTC_TTSR_TTS_MASK                        (0xFFFFFFFFU)
#define RTC_TTSR_TTS_SHIFT                       (0U)
#define RTC_TTSR_TTS(x)                          (((uint32_t)(((uint32_t)(x)) << RTC_TTSR_TTS_SHIFT)) & RTC_TTSR_TTS_MASK)

/*! @name MER - RTC Monotonic Enable Register */
#define RTC_MER_MCE_MASK                         (0x10U)
#define RTC_MER_MCE_SHIFT                        (4U)
#define RTC_MER_MCE(x)                           (((uint32_t)(((uint32_t)(x)) << RTC_MER_MCE_SHIFT)) & RTC_MER_MCE_MASK)

/*! @name MCLR - RTC Monotonic Counter Low Register */
#define RTC_MCLR_MCL_MASK                        (0xFFFFFFFFU)
#define RTC_MCLR_MCL_SHIFT                       (0U)
#define RTC_MCLR_MCL(x)                          (((uint32_t)(((uint32_t)(x)) << RTC_MCLR_MCL_SHIFT)) & RTC_MCLR_MCL_MASK)

/*! @name MCHR - RTC Monotonic Counter High Register */
#define RTC_MCHR_MCH_MASK                        (0xFFFFFFFFU)
#define RTC_MCHR_MCH_SHIFT                       (0U)
#define RTC_MCHR_MCH(x)                          (((uint32_t)(((uint32_t)(x)) << RTC_MCHR_MCH_SHIFT)) & RTC_MCHR_MCH_MASK)

/*! @name WAR - RTC Write Access Register */
#define RTC_WAR_TSRW_MASK                        (0x1U)
#define RTC_WAR_TSRW_SHIFT                       (0U)
#define RTC_WAR_TSRW(x)                          (((uint32_t)(((uint32_t)(x)) << RTC_WAR_TSRW_SHIFT)) & RTC_WAR_TSRW_MASK)
#define RTC_WAR_TPRW_MASK                        (0x2U)
#define RTC_WAR_TPRW_SHIFT                       (1U)
#define RTC_WAR_TPRW(x)                          (((uint32_t)(((uint32_t)(x)) << RTC_WAR_TPRW_SHIFT)) & RTC_WAR_TPRW_MASK)
#define RTC_WAR_TARW_MASK                        (0x4U)
#define RTC_WAR_TARW_SHIFT                       (2U)
#define RTC_WAR_TARW(x)                          (((uint32_t)(((uint32_t)(x)) << RTC_WAR_TARW_SHIFT)) & RTC_WAR_TARW_MASK)
#define RTC_WAR_TCRW_MASK                        (0x8U)
#define RTC_WAR_TCRW_SHIFT                       (3U)
#define RTC_WAR_TCRW(x)                          (((uint32_t)(((uint32_t)(x)) << RTC_WAR_TCRW_SHIFT)) & RTC_WAR_TCRW_MASK)
#define RTC_WAR_CRW_MASK                         (0x10U)
#define RTC_WAR_CRW_SHIFT                        (4U)
#define RTC_WAR_CRW(x)                           (((uint32_t)(((uint32_t)(x)) << RTC_WAR_CRW_SHIFT)) & RTC_WAR_CRW_MASK)
#define RTC_WAR_SRW_MASK                         (0x20U)
#define RTC_WAR_SRW_SHIFT                        (5U)
#define RTC_WAR_SRW(x)                           (((uint32_t)(((uint32_t)(x)) << RTC_WAR_SRW_SHIFT)) & RTC_WAR_SRW_MASK)
#define RTC_WAR_LRW_MASK                         (0x40U)
#define RTC_WAR_LRW_SHIFT                        (6U)
#define RTC_WAR_LRW(x)                           (((uint32_t)(((uint32_t)(x)) << RTC_WAR_LRW_SHIFT)) & RTC_WAR_LRW_MASK)
#define RTC_WAR_IERW_MASK                        (0x80U)
#define RTC_WAR_IERW_SHIFT                       (7U)
#define RTC_WAR_IERW(x)                          (((uint32_t)(((uint32_t)(x)) << RTC_WAR_IERW_SHIFT)) & RTC_WAR_IERW_MASK)
#define RTC_WAR_TTSW_MASK                        (0x100U)
#define RTC_WAR_TTSW_SHIFT                       (8U)
#define RTC_WAR_TTSW(x)                          (((uint32_t)(((uint32_t)(x)) << RTC_WAR_TTSW_SHIFT)) & RTC_WAR_TTSW_MASK)
#define RTC_WAR_MERW_MASK                        (0x200U)
#define RTC_WAR_MERW_SHIFT                       (9U)
#define RTC_WAR_MERW(x)                          (((uint32_t)(((uint32_t)(x)) << RTC_WAR_MERW_SHIFT)) & RTC_WAR_MERW_MASK)
#define RTC_WAR_MCLW_MASK                        (0x400U)
#define RTC_WAR_MCLW_SHIFT                       (10U)
#define RTC_WAR_MCLW(x)                          (((uint32_t)(((uint32_t)(x)) << RTC_WAR_MCLW_SHIFT)) & RTC_WAR_MCLW_MASK)
#define RTC_WAR_MCHW_MASK                        (0x800U)
#define RTC_WAR_MCHW_SHIFT                       (11U)
#define RTC_WAR_MCHW(x)                          (((uint32_t)(((uint32_t)(x)) << RTC_WAR_MCHW_SHIFT)) & RTC_WAR_MCHW_MASK)

/*! @name RAR - RTC Read Access Register */
#define RTC_RAR_TSRR_MASK                        (0x1U)
#define RTC_RAR_TSRR_SHIFT                       (0U)
#define RTC_RAR_TSRR(x)                          (((uint32_t)(((uint32_t)(x)) << RTC_RAR_TSRR_SHIFT)) & RTC_RAR_TSRR_MASK)
#define RTC_RAR_TPRR_MASK                        (0x2U)
#define RTC_RAR_TPRR_SHIFT                       (1U)
#define RTC_RAR_TPRR(x)                          (((uint32_t)(((uint32_t)(x)) << RTC_RAR_TPRR_SHIFT)) & RTC_RAR_TPRR_MASK)
#define RTC_RAR_TARR_MASK                        (0x4U)
#define RTC_RAR_TARR_SHIFT                       (2U)
#define RTC_RAR_TARR(x)                          (((uint32_t)(((uint32_t)(x)) << RTC_RAR_TARR_SHIFT)) & RTC_RAR_TARR_MASK)
#define RTC_RAR_TCRR_MASK                        (0x8U)
#define RTC_RAR_TCRR_SHIFT                       (3U)
#define RTC_RAR_TCRR(x)                          (((uint32_t)(((uint32_t)(x)) << RTC_RAR_TCRR_SHIFT)) & RTC_RAR_TCRR_MASK)
#define RTC_RAR_CRR_MASK                         (0x10U)
#define RTC_RAR_CRR_SHIFT                        (4U)
#define RTC_RAR_CRR(x)                           (((uint32_t)(((uint32_t)(x)) << RTC_RAR_CRR_SHIFT)) & RTC_RAR_CRR_MASK)
#define RTC_RAR_SRR_MASK                         (0x20U)
#define RTC_RAR_SRR_SHIFT                        (5U)
#define RTC_RAR_SRR(x)                           (((uint32_t)(((uint32_t)(x)) << RTC_RAR_SRR_SHIFT)) & RTC_RAR_SRR_MASK)
#define RTC_RAR_LRR_MASK                         (0x40U)
#define RTC_RAR_LRR_SHIFT                        (6U)
#define RTC_RAR_LRR(x)                           (((uint32_t)(((uint32_t)(x)) << RTC_RAR_LRR_SHIFT)) & RTC_RAR_LRR_MASK)
#define RTC_RAR_IERR_MASK                        (0x80U)
#define RTC_RAR_IERR_SHIFT                       (7U)
#define RTC_RAR_IERR(x)                          (((uint32_t)(((uint32_t)(x)) << RTC_RAR_IERR_SHIFT)) & RTC_RAR_IERR_MASK)
#define RTC_RAR_TTSR_MASK                        (0x100U)
#define RTC_RAR_TTSR_SHIFT                       (8U)
#define RTC_RAR_TTSR(x)                          (((uint32_t)(((uint32_t)(x)) << RTC_RAR_TTSR_SHIFT)) & RTC_RAR_TTSR_MASK)
#define RTC_RAR_MERR_MASK                        (0x200U)
#define RTC_RAR_MERR_SHIFT                       (9U)
#define RTC_RAR_MERR(x)                          (((uint32_t)(((uint32_t)(x)) << RTC_RAR_MERR_SHIFT)) & RTC_RAR_MERR_MASK)
#define RTC_RAR_MCLR_MASK                        (0x400U)
#define RTC_RAR_MCLR_SHIFT                       (10U)
#define RTC_RAR_MCLR(x)                          (((uint32_t)(((uint32_t)(x)) << RTC_RAR_MCLR_SHIFT)) & RTC_RAR_MCLR_MASK)
#define RTC_RAR_MCHR_MASK                        (0x800U)
#define RTC_RAR_MCHR_SHIFT                       (11U)
#define RTC_RAR_MCHR(x)                          (((uint32_t)(((uint32_t)(x)) << RTC_RAR_MCHR_SHIFT)) & RTC_RAR_MCHR_MASK)


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
  __IO uint32_t SOPT1CFG;                          /**< SOPT1 Configuration Register, offset: 0x4 */
       uint8_t RESERVED_0[4092];
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
  __IO uint32_t CLKDIV2;                           /**< System Clock Divider Register 2, offset: 0x1048 */
  __IO uint32_t FCFG1;                             /**< Flash Configuration Register 1, offset: 0x104C */
  __I  uint32_t FCFG2;                             /**< Flash Configuration Register 2, offset: 0x1050 */
  __I  uint32_t UIDH;                              /**< Unique Identification Register High, offset: 0x1054 */
  __I  uint32_t UIDMH;                             /**< Unique Identification Register Mid-High, offset: 0x1058 */
  __I  uint32_t UIDML;                             /**< Unique Identification Register Mid Low, offset: 0x105C */
  __I  uint32_t UIDL;                              /**< Unique Identification Register Low, offset: 0x1060 */
} SIM_Type;

/* ----------------------------------------------------------------------------
   -- SIM Register Masks
   ---------------------------------------------------------------------------- */

/*!
 * @addtogroup SIM_Register_Masks SIM Register Masks
 * @{
 */

/*! @name SOPT1 - System Options Register 1 */
#define SIM_SOPT1_RAMSIZE_MASK                   (0xF000U)
#define SIM_SOPT1_RAMSIZE_SHIFT                  (12U)
#define SIM_SOPT1_RAMSIZE(x)                     (((uint32_t)(((uint32_t)(x)) << SIM_SOPT1_RAMSIZE_SHIFT)) & SIM_SOPT1_RAMSIZE_MASK)
#define SIM_SOPT1_OSC32KSEL_MASK                 (0xC0000U)
#define SIM_SOPT1_OSC32KSEL_SHIFT                (18U)
#define SIM_SOPT1_OSC32KSEL(x)                   (((uint32_t)(((uint32_t)(x)) << SIM_SOPT1_OSC32KSEL_SHIFT)) & SIM_SOPT1_OSC32KSEL_MASK)
#define SIM_SOPT1_USBVSTBY_MASK                  (0x20000000U)
#define SIM_SOPT1_USBVSTBY_SHIFT                 (29U)
#define SIM_SOPT1_USBVSTBY(x)                    (((uint32_t)(((uint32_t)(x)) << SIM_SOPT1_USBVSTBY_SHIFT)) & SIM_SOPT1_USBVSTBY_MASK)
#define SIM_SOPT1_USBSSTBY_MASK                  (0x40000000U)
#define SIM_SOPT1_USBSSTBY_SHIFT                 (30U)
#define SIM_SOPT1_USBSSTBY(x)                    (((uint32_t)(((uint32_t)(x)) << SIM_SOPT1_USBSSTBY_SHIFT)) & SIM_SOPT1_USBSSTBY_MASK)
#define SIM_SOPT1_USBREGEN_MASK                  (0x80000000U)
#define SIM_SOPT1_USBREGEN_SHIFT                 (31U)
#define SIM_SOPT1_USBREGEN(x)                    (((uint32_t)(((uint32_t)(x)) << SIM_SOPT1_USBREGEN_SHIFT)) & SIM_SOPT1_USBREGEN_MASK)

/*! @name SOPT1CFG - SOPT1 Configuration Register */
#define SIM_SOPT1CFG_URWE_MASK                   (0x1000000U)
#define SIM_SOPT1CFG_URWE_SHIFT                  (24U)
#define SIM_SOPT1CFG_URWE(x)                     (((uint32_t)(((uint32_t)(x)) << SIM_SOPT1CFG_URWE_SHIFT)) & SIM_SOPT1CFG_URWE_MASK)
#define SIM_SOPT1CFG_UVSWE_MASK                  (0x2000000U)
#define SIM_SOPT1CFG_UVSWE_SHIFT                 (25U)
#define SIM_SOPT1CFG_UVSWE(x)                    (((uint32_t)(((uint32_t)(x)) << SIM_SOPT1CFG_UVSWE_SHIFT)) & SIM_SOPT1CFG_UVSWE_MASK)
#define SIM_SOPT1CFG_USSWE_MASK                  (0x4000000U)
#define SIM_SOPT1CFG_USSWE_SHIFT                 (26U)
#define SIM_SOPT1CFG_USSWE(x)                    (((uint32_t)(((uint32_t)(x)) << SIM_SOPT1CFG_USSWE_SHIFT)) & SIM_SOPT1CFG_USSWE_MASK)

/*! @name SOPT2 - System Options Register 2 */
#define SIM_SOPT2_RTCCLKOUTSEL_MASK              (0x10U)
#define SIM_SOPT2_RTCCLKOUTSEL_SHIFT             (4U)
#define SIM_SOPT2_RTCCLKOUTSEL(x)                (((uint32_t)(((uint32_t)(x)) << SIM_SOPT2_RTCCLKOUTSEL_SHIFT)) & SIM_SOPT2_RTCCLKOUTSEL_MASK)
#define SIM_SOPT2_CLKOUTSEL_MASK                 (0xE0U)
#define SIM_SOPT2_CLKOUTSEL_SHIFT                (5U)
#define SIM_SOPT2_CLKOUTSEL(x)                   (((uint32_t)(((uint32_t)(x)) << SIM_SOPT2_CLKOUTSEL_SHIFT)) & SIM_SOPT2_CLKOUTSEL_MASK)
#define SIM_SOPT2_PTD7PAD_MASK                   (0x800U)
#define SIM_SOPT2_PTD7PAD_SHIFT                  (11U)
#define SIM_SOPT2_PTD7PAD(x)                     (((uint32_t)(((uint32_t)(x)) << SIM_SOPT2_PTD7PAD_SHIFT)) & SIM_SOPT2_PTD7PAD_MASK)
#define SIM_SOPT2_TRACECLKSEL_MASK               (0x1000U)
#define SIM_SOPT2_TRACECLKSEL_SHIFT              (12U)
#define SIM_SOPT2_TRACECLKSEL(x)                 (((uint32_t)(((uint32_t)(x)) << SIM_SOPT2_TRACECLKSEL_SHIFT)) & SIM_SOPT2_TRACECLKSEL_MASK)
#define SIM_SOPT2_PLLFLLSEL_MASK                 (0x10000U)
#define SIM_SOPT2_PLLFLLSEL_SHIFT                (16U)
#define SIM_SOPT2_PLLFLLSEL(x)                   (((uint32_t)(((uint32_t)(x)) << SIM_SOPT2_PLLFLLSEL_SHIFT)) & SIM_SOPT2_PLLFLLSEL_MASK)
#define SIM_SOPT2_USBSRC_MASK                    (0x40000U)
#define SIM_SOPT2_USBSRC_SHIFT                   (18U)
#define SIM_SOPT2_USBSRC(x)                      (((uint32_t)(((uint32_t)(x)) << SIM_SOPT2_USBSRC_SHIFT)) & SIM_SOPT2_USBSRC_MASK)

/*! @name SOPT4 - System Options Register 4 */
#define SIM_SOPT4_FTM0FLT0_MASK                  (0x1U)
#define SIM_SOPT4_FTM0FLT0_SHIFT                 (0U)
#define SIM_SOPT4_FTM0FLT0(x)                    (((uint32_t)(((uint32_t)(x)) << SIM_SOPT4_FTM0FLT0_SHIFT)) & SIM_SOPT4_FTM0FLT0_MASK)
#define SIM_SOPT4_FTM0FLT1_MASK                  (0x2U)
#define SIM_SOPT4_FTM0FLT1_SHIFT                 (1U)
#define SIM_SOPT4_FTM0FLT1(x)                    (((uint32_t)(((uint32_t)(x)) << SIM_SOPT4_FTM0FLT1_SHIFT)) & SIM_SOPT4_FTM0FLT1_MASK)
#define SIM_SOPT4_FTM1FLT0_MASK                  (0x10U)
#define SIM_SOPT4_FTM1FLT0_SHIFT                 (4U)
#define SIM_SOPT4_FTM1FLT0(x)                    (((uint32_t)(((uint32_t)(x)) << SIM_SOPT4_FTM1FLT0_SHIFT)) & SIM_SOPT4_FTM1FLT0_MASK)
#define SIM_SOPT4_FTM2FLT0_MASK                  (0x100U)
#define SIM_SOPT4_FTM2FLT0_SHIFT                 (8U)
#define SIM_SOPT4_FTM2FLT0(x)                    (((uint32_t)(((uint32_t)(x)) << SIM_SOPT4_FTM2FLT0_SHIFT)) & SIM_SOPT4_FTM2FLT0_MASK)
#define SIM_SOPT4_FTM1CH0SRC_MASK                (0xC0000U)
#define SIM_SOPT4_FTM1CH0SRC_SHIFT               (18U)
#define SIM_SOPT4_FTM1CH0SRC(x)                  (((uint32_t)(((uint32_t)(x)) << SIM_SOPT4_FTM1CH0SRC_SHIFT)) & SIM_SOPT4_FTM1CH0SRC_MASK)
#define SIM_SOPT4_FTM2CH0SRC_MASK                (0x300000U)
#define SIM_SOPT4_FTM2CH0SRC_SHIFT               (20U)
#define SIM_SOPT4_FTM2CH0SRC(x)                  (((uint32_t)(((uint32_t)(x)) << SIM_SOPT4_FTM2CH0SRC_SHIFT)) & SIM_SOPT4_FTM2CH0SRC_MASK)
#define SIM_SOPT4_FTM0CLKSEL_MASK                (0x1000000U)
#define SIM_SOPT4_FTM0CLKSEL_SHIFT               (24U)
#define SIM_SOPT4_FTM0CLKSEL(x)                  (((uint32_t)(((uint32_t)(x)) << SIM_SOPT4_FTM0CLKSEL_SHIFT)) & SIM_SOPT4_FTM0CLKSEL_MASK)
#define SIM_SOPT4_FTM1CLKSEL_MASK                (0x2000000U)
#define SIM_SOPT4_FTM1CLKSEL_SHIFT               (25U)
#define SIM_SOPT4_FTM1CLKSEL(x)                  (((uint32_t)(((uint32_t)(x)) << SIM_SOPT4_FTM1CLKSEL_SHIFT)) & SIM_SOPT4_FTM1CLKSEL_MASK)
#define SIM_SOPT4_FTM2CLKSEL_MASK                (0x4000000U)
#define SIM_SOPT4_FTM2CLKSEL_SHIFT               (26U)
#define SIM_SOPT4_FTM2CLKSEL(x)                  (((uint32_t)(((uint32_t)(x)) << SIM_SOPT4_FTM2CLKSEL_SHIFT)) & SIM_SOPT4_FTM2CLKSEL_MASK)
#define SIM_SOPT4_FTM0TRG0SRC_MASK               (0x10000000U)
#define SIM_SOPT4_FTM0TRG0SRC_SHIFT              (28U)
#define SIM_SOPT4_FTM0TRG0SRC(x)                 (((uint32_t)(((uint32_t)(x)) << SIM_SOPT4_FTM0TRG0SRC_SHIFT)) & SIM_SOPT4_FTM0TRG0SRC_MASK)
#define SIM_SOPT4_FTM0TRG1SRC_MASK               (0x20000000U)
#define SIM_SOPT4_FTM0TRG1SRC_SHIFT              (29U)
#define SIM_SOPT4_FTM0TRG1SRC(x)                 (((uint32_t)(((uint32_t)(x)) << SIM_SOPT4_FTM0TRG1SRC_SHIFT)) & SIM_SOPT4_FTM0TRG1SRC_MASK)

/*! @name SOPT5 - System Options Register 5 */
#define SIM_SOPT5_UART0TXSRC_MASK                (0x3U)
#define SIM_SOPT5_UART0TXSRC_SHIFT               (0U)
#define SIM_SOPT5_UART0TXSRC(x)                  (((uint32_t)(((uint32_t)(x)) << SIM_SOPT5_UART0TXSRC_SHIFT)) & SIM_SOPT5_UART0TXSRC_MASK)
#define SIM_SOPT5_UART0RXSRC_MASK                (0xCU)
#define SIM_SOPT5_UART0RXSRC_SHIFT               (2U)
#define SIM_SOPT5_UART0RXSRC(x)                  (((uint32_t)(((uint32_t)(x)) << SIM_SOPT5_UART0RXSRC_SHIFT)) & SIM_SOPT5_UART0RXSRC_MASK)
#define SIM_SOPT5_UART1TXSRC_MASK                (0x30U)
#define SIM_SOPT5_UART1TXSRC_SHIFT               (4U)
#define SIM_SOPT5_UART1TXSRC(x)                  (((uint32_t)(((uint32_t)(x)) << SIM_SOPT5_UART1TXSRC_SHIFT)) & SIM_SOPT5_UART1TXSRC_MASK)
#define SIM_SOPT5_UART1RXSRC_MASK                (0xC0U)
#define SIM_SOPT5_UART1RXSRC_SHIFT               (6U)
#define SIM_SOPT5_UART1RXSRC(x)                  (((uint32_t)(((uint32_t)(x)) << SIM_SOPT5_UART1RXSRC_SHIFT)) & SIM_SOPT5_UART1RXSRC_MASK)

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
#define SIM_SDID_FAMID_MASK                      (0x70U)
#define SIM_SDID_FAMID_SHIFT                     (4U)
#define SIM_SDID_FAMID(x)                        (((uint32_t)(((uint32_t)(x)) << SIM_SDID_FAMID_SHIFT)) & SIM_SDID_FAMID_MASK)
#define SIM_SDID_DIEID_MASK                      (0xF80U)
#define SIM_SDID_DIEID_SHIFT                     (7U)
#define SIM_SDID_DIEID(x)                        (((uint32_t)(((uint32_t)(x)) << SIM_SDID_DIEID_SHIFT)) & SIM_SDID_DIEID_MASK)
#define SIM_SDID_REVID_MASK                      (0xF000U)
#define SIM_SDID_REVID_SHIFT                     (12U)
#define SIM_SDID_REVID(x)                        (((uint32_t)(((uint32_t)(x)) << SIM_SDID_REVID_SHIFT)) & SIM_SDID_REVID_MASK)

/*! @name SCGC4 - System Clock Gating Control Register 4 */
#define SIM_SCGC4_EWM_MASK                       (0x2U)
#define SIM_SCGC4_EWM_SHIFT                      (1U)
#define SIM_SCGC4_EWM(x)                         (((uint32_t)(((uint32_t)(x)) << SIM_SCGC4_EWM_SHIFT)) & SIM_SCGC4_EWM_MASK)
#define SIM_SCGC4_CMT_MASK                       (0x4U)
#define SIM_SCGC4_CMT_SHIFT                      (2U)
#define SIM_SCGC4_CMT(x)                         (((uint32_t)(((uint32_t)(x)) << SIM_SCGC4_CMT_SHIFT)) & SIM_SCGC4_CMT_MASK)
#define SIM_SCGC4_I2C0_MASK                      (0x40U)
#define SIM_SCGC4_I2C0_SHIFT                     (6U)
#define SIM_SCGC4_I2C0(x)                        (((uint32_t)(((uint32_t)(x)) << SIM_SCGC4_I2C0_SHIFT)) & SIM_SCGC4_I2C0_MASK)
#define SIM_SCGC4_I2C1_MASK                      (0x80U)
#define SIM_SCGC4_I2C1_SHIFT                     (7U)
#define SIM_SCGC4_I2C1(x)                        (((uint32_t)(((uint32_t)(x)) << SIM_SCGC4_I2C1_SHIFT)) & SIM_SCGC4_I2C1_MASK)
#define SIM_SCGC4_UART0_MASK                     (0x400U)
#define SIM_SCGC4_UART0_SHIFT                    (10U)
#define SIM_SCGC4_UART0(x)                       (((uint32_t)(((uint32_t)(x)) << SIM_SCGC4_UART0_SHIFT)) & SIM_SCGC4_UART0_MASK)
#define SIM_SCGC4_UART1_MASK                     (0x800U)
#define SIM_SCGC4_UART1_SHIFT                    (11U)
#define SIM_SCGC4_UART1(x)                       (((uint32_t)(((uint32_t)(x)) << SIM_SCGC4_UART1_SHIFT)) & SIM_SCGC4_UART1_MASK)
#define SIM_SCGC4_UART2_MASK                     (0x1000U)
#define SIM_SCGC4_UART2_SHIFT                    (12U)
#define SIM_SCGC4_UART2(x)                       (((uint32_t)(((uint32_t)(x)) << SIM_SCGC4_UART2_SHIFT)) & SIM_SCGC4_UART2_MASK)
#define SIM_SCGC4_UART3_MASK                     (0x2000U)
#define SIM_SCGC4_UART3_SHIFT                    (13U)
#define SIM_SCGC4_UART3(x)                       (((uint32_t)(((uint32_t)(x)) << SIM_SCGC4_UART3_SHIFT)) & SIM_SCGC4_UART3_MASK)
#define SIM_SCGC4_USBOTG_MASK                    (0x40000U)
#define SIM_SCGC4_USBOTG_SHIFT                   (18U)
#define SIM_SCGC4_USBOTG(x)                      (((uint32_t)(((uint32_t)(x)) << SIM_SCGC4_USBOTG_SHIFT)) & SIM_SCGC4_USBOTG_MASK)
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
#define SIM_SCGC5_PORTA_MASK                     (0x200U)
#define SIM_SCGC5_PORTA_SHIFT                    (9U)
#define SIM_SCGC5_PORTA(x)                       (((uint32_t)(((uint32_t)(x)) << SIM_SCGC5_PORTA_SHIFT)) & SIM_SCGC5_PORTA_MASK)
#define SIM_SCGC5_PORTB_MASK                     (0x400U)
#define SIM_SCGC5_PORTB_SHIFT                    (10U)
#define SIM_SCGC5_PORTB(x)                       (((uint32_t)(((uint32_t)(x)) << SIM_SCGC5_PORTB_SHIFT)) & SIM_SCGC5_PORTB_MASK)
#define SIM_SCGC5_PORTC_MASK                     (0x800U)
#define SIM_SCGC5_PORTC_SHIFT                    (11U)
#define SIM_SCGC5_PORTC(x)                       (((uint32_t)(((uint32_t)(x)) << SIM_SCGC5_PORTC_SHIFT)) & SIM_SCGC5_PORTC_MASK)
#define SIM_SCGC5_PORTD_MASK                     (0x1000U)
#define SIM_SCGC5_PORTD_SHIFT                    (12U)
#define SIM_SCGC5_PORTD(x)                       (((uint32_t)(((uint32_t)(x)) << SIM_SCGC5_PORTD_SHIFT)) & SIM_SCGC5_PORTD_MASK)
#define SIM_SCGC5_PORTE_MASK                     (0x2000U)
#define SIM_SCGC5_PORTE_SHIFT                    (13U)
#define SIM_SCGC5_PORTE(x)                       (((uint32_t)(((uint32_t)(x)) << SIM_SCGC5_PORTE_SHIFT)) & SIM_SCGC5_PORTE_MASK)

/*! @name SCGC6 - System Clock Gating Control Register 6 */
#define SIM_SCGC6_FTFL_MASK                      (0x1U)
#define SIM_SCGC6_FTFL_SHIFT                     (0U)
#define SIM_SCGC6_FTFL(x)                        (((uint32_t)(((uint32_t)(x)) << SIM_SCGC6_FTFL_SHIFT)) & SIM_SCGC6_FTFL_MASK)
#define SIM_SCGC6_DMAMUX_MASK                    (0x2U)
#define SIM_SCGC6_DMAMUX_SHIFT                   (1U)
#define SIM_SCGC6_DMAMUX(x)                      (((uint32_t)(((uint32_t)(x)) << SIM_SCGC6_DMAMUX_SHIFT)) & SIM_SCGC6_DMAMUX_MASK)
#define SIM_SCGC6_RNGA_MASK                      (0x200U)
#define SIM_SCGC6_RNGA_SHIFT                     (9U)
#define SIM_SCGC6_RNGA(x)                        (((uint32_t)(((uint32_t)(x)) << SIM_SCGC6_RNGA_SHIFT)) & SIM_SCGC6_RNGA_MASK)
#define SIM_SCGC6_SPI0_MASK                      (0x1000U)
#define SIM_SCGC6_SPI0_SHIFT                     (12U)
#define SIM_SCGC6_SPI0(x)                        (((uint32_t)(((uint32_t)(x)) << SIM_SCGC6_SPI0_SHIFT)) & SIM_SCGC6_SPI0_MASK)
#define SIM_SCGC6_SPI1_MASK                      (0x2000U)
#define SIM_SCGC6_SPI1_SHIFT                     (13U)
#define SIM_SCGC6_SPI1(x)                        (((uint32_t)(((uint32_t)(x)) << SIM_SCGC6_SPI1_SHIFT)) & SIM_SCGC6_SPI1_MASK)
#define SIM_SCGC6_I2S_MASK                       (0x8000U)
#define SIM_SCGC6_I2S_SHIFT                      (15U)
#define SIM_SCGC6_I2S(x)                         (((uint32_t)(((uint32_t)(x)) << SIM_SCGC6_I2S_SHIFT)) & SIM_SCGC6_I2S_MASK)
#define SIM_SCGC6_CRC_MASK                       (0x40000U)
#define SIM_SCGC6_CRC_SHIFT                      (18U)
#define SIM_SCGC6_CRC(x)                         (((uint32_t)(((uint32_t)(x)) << SIM_SCGC6_CRC_SHIFT)) & SIM_SCGC6_CRC_MASK)
#define SIM_SCGC6_USBDCD_MASK                    (0x200000U)
#define SIM_SCGC6_USBDCD_SHIFT                   (21U)
#define SIM_SCGC6_USBDCD(x)                      (((uint32_t)(((uint32_t)(x)) << SIM_SCGC6_USBDCD_SHIFT)) & SIM_SCGC6_USBDCD_MASK)
#define SIM_SCGC6_PDB_MASK                       (0x400000U)
#define SIM_SCGC6_PDB_SHIFT                      (22U)
#define SIM_SCGC6_PDB(x)                         (((uint32_t)(((uint32_t)(x)) << SIM_SCGC6_PDB_SHIFT)) & SIM_SCGC6_PDB_MASK)
#define SIM_SCGC6_PIT_MASK                       (0x800000U)
#define SIM_SCGC6_PIT_SHIFT                      (23U)
#define SIM_SCGC6_PIT(x)                         (((uint32_t)(((uint32_t)(x)) << SIM_SCGC6_PIT_SHIFT)) & SIM_SCGC6_PIT_MASK)
#define SIM_SCGC6_FTM0_MASK                      (0x1000000U)
#define SIM_SCGC6_FTM0_SHIFT                     (24U)
#define SIM_SCGC6_FTM0(x)                        (((uint32_t)(((uint32_t)(x)) << SIM_SCGC6_FTM0_SHIFT)) & SIM_SCGC6_FTM0_MASK)
#define SIM_SCGC6_FTM1_MASK                      (0x2000000U)
#define SIM_SCGC6_FTM1_SHIFT                     (25U)
#define SIM_SCGC6_FTM1(x)                        (((uint32_t)(((uint32_t)(x)) << SIM_SCGC6_FTM1_SHIFT)) & SIM_SCGC6_FTM1_MASK)
#define SIM_SCGC6_FTM2_MASK                      (0x4000000U)
#define SIM_SCGC6_FTM2_SHIFT                     (26U)
#define SIM_SCGC6_FTM2(x)                        (((uint32_t)(((uint32_t)(x)) << SIM_SCGC6_FTM2_SHIFT)) & SIM_SCGC6_FTM2_MASK)
#define SIM_SCGC6_ADC0_MASK                      (0x8000000U)
#define SIM_SCGC6_ADC0_SHIFT                     (27U)
#define SIM_SCGC6_ADC0(x)                        (((uint32_t)(((uint32_t)(x)) << SIM_SCGC6_ADC0_SHIFT)) & SIM_SCGC6_ADC0_MASK)
#define SIM_SCGC6_RTC_MASK                       (0x20000000U)
#define SIM_SCGC6_RTC_SHIFT                      (29U)
#define SIM_SCGC6_RTC(x)                         (((uint32_t)(((uint32_t)(x)) << SIM_SCGC6_RTC_SHIFT)) & SIM_SCGC6_RTC_MASK)

/*! @name SCGC7 - System Clock Gating Control Register 7 */
#define SIM_SCGC7_DMA_MASK                       (0x2U)
#define SIM_SCGC7_DMA_SHIFT                      (1U)
#define SIM_SCGC7_DMA(x)                         (((uint32_t)(((uint32_t)(x)) << SIM_SCGC7_DMA_SHIFT)) & SIM_SCGC7_DMA_MASK)

/*! @name CLKDIV1 - System Clock Divider Register 1 */
#define SIM_CLKDIV1_OUTDIV4_MASK                 (0xF0000U)
#define SIM_CLKDIV1_OUTDIV4_SHIFT                (16U)
#define SIM_CLKDIV1_OUTDIV4(x)                   (((uint32_t)(((uint32_t)(x)) << SIM_CLKDIV1_OUTDIV4_SHIFT)) & SIM_CLKDIV1_OUTDIV4_MASK)
#define SIM_CLKDIV1_OUTDIV2_MASK                 (0xF000000U)
#define SIM_CLKDIV1_OUTDIV2_SHIFT                (24U)
#define SIM_CLKDIV1_OUTDIV2(x)                   (((uint32_t)(((uint32_t)(x)) << SIM_CLKDIV1_OUTDIV2_SHIFT)) & SIM_CLKDIV1_OUTDIV2_MASK)
#define SIM_CLKDIV1_OUTDIV1_MASK                 (0xF0000000U)
#define SIM_CLKDIV1_OUTDIV1_SHIFT                (28U)
#define SIM_CLKDIV1_OUTDIV1(x)                   (((uint32_t)(((uint32_t)(x)) << SIM_CLKDIV1_OUTDIV1_SHIFT)) & SIM_CLKDIV1_OUTDIV1_MASK)

/*! @name CLKDIV2 - System Clock Divider Register 2 */
#define SIM_CLKDIV2_USBFRAC_MASK                 (0x1U)
#define SIM_CLKDIV2_USBFRAC_SHIFT                (0U)
#define SIM_CLKDIV2_USBFRAC(x)                   (((uint32_t)(((uint32_t)(x)) << SIM_CLKDIV2_USBFRAC_SHIFT)) & SIM_CLKDIV2_USBFRAC_MASK)
#define SIM_CLKDIV2_USBDIV_MASK                  (0xEU)
#define SIM_CLKDIV2_USBDIV_SHIFT                 (1U)
#define SIM_CLKDIV2_USBDIV(x)                    (((uint32_t)(((uint32_t)(x)) << SIM_CLKDIV2_USBDIV_SHIFT)) & SIM_CLKDIV2_USBDIV_MASK)

/*! @name FCFG1 - Flash Configuration Register 1 */
#define SIM_FCFG1_FLASHDIS_MASK                  (0x1U)
#define SIM_FCFG1_FLASHDIS_SHIFT                 (0U)
#define SIM_FCFG1_FLASHDIS(x)                    (((uint32_t)(((uint32_t)(x)) << SIM_FCFG1_FLASHDIS_SHIFT)) & SIM_FCFG1_FLASHDIS_MASK)
#define SIM_FCFG1_FLASHDOZE_MASK                 (0x2U)
#define SIM_FCFG1_FLASHDOZE_SHIFT                (1U)
#define SIM_FCFG1_FLASHDOZE(x)                   (((uint32_t)(((uint32_t)(x)) << SIM_FCFG1_FLASHDOZE_SHIFT)) & SIM_FCFG1_FLASHDOZE_MASK)
#define SIM_FCFG1_DEPART_MASK                    (0xF00U)
#define SIM_FCFG1_DEPART_SHIFT                   (8U)
#define SIM_FCFG1_DEPART(x)                      (((uint32_t)(((uint32_t)(x)) << SIM_FCFG1_DEPART_SHIFT)) & SIM_FCFG1_DEPART_MASK)
#define SIM_FCFG1_EESIZE_MASK                    (0xF0000U)
#define SIM_FCFG1_EESIZE_SHIFT                   (16U)
#define SIM_FCFG1_EESIZE(x)                      (((uint32_t)(((uint32_t)(x)) << SIM_FCFG1_EESIZE_SHIFT)) & SIM_FCFG1_EESIZE_MASK)
#define SIM_FCFG1_PFSIZE_MASK                    (0xF000000U)
#define SIM_FCFG1_PFSIZE_SHIFT                   (24U)
#define SIM_FCFG1_PFSIZE(x)                      (((uint32_t)(((uint32_t)(x)) << SIM_FCFG1_PFSIZE_SHIFT)) & SIM_FCFG1_PFSIZE_MASK)
#define SIM_FCFG1_NVMSIZE_MASK                   (0xF0000000U)
#define SIM_FCFG1_NVMSIZE_SHIFT                  (28U)
#define SIM_FCFG1_NVMSIZE(x)                     (((uint32_t)(((uint32_t)(x)) << SIM_FCFG1_NVMSIZE_SHIFT)) & SIM_FCFG1_NVMSIZE_MASK)

/*! @name FCFG2 - Flash Configuration Register 2 */
#define SIM_FCFG2_MAXADDR1_MASK                  (0x7F0000U)
#define SIM_FCFG2_MAXADDR1_SHIFT                 (16U)
#define SIM_FCFG2_MAXADDR1(x)                    (((uint32_t)(((uint32_t)(x)) << SIM_FCFG2_MAXADDR1_SHIFT)) & SIM_FCFG2_MAXADDR1_MASK)
#define SIM_FCFG2_PFLSH_MASK                     (0x800000U)
#define SIM_FCFG2_PFLSH_SHIFT                    (23U)
#define SIM_FCFG2_PFLSH(x)                       (((uint32_t)(((uint32_t)(x)) << SIM_FCFG2_PFLSH_SHIFT)) & SIM_FCFG2_PFLSH_MASK)
#define SIM_FCFG2_MAXADDR0_MASK                  (0x7F000000U)
#define SIM_FCFG2_MAXADDR0_SHIFT                 (24U)
#define SIM_FCFG2_MAXADDR0(x)                    (((uint32_t)(((uint32_t)(x)) << SIM_FCFG2_MAXADDR0_SHIFT)) & SIM_FCFG2_MAXADDR0_MASK)
#define SIM_FCFG2_SWAPPFLSH_MASK                 (0x80000000U)
#define SIM_FCFG2_SWAPPFLSH_SHIFT                (31U)
#define SIM_FCFG2_SWAPPFLSH(x)                   (((uint32_t)(((uint32_t)(x)) << SIM_FCFG2_SWAPPFLSH_SHIFT)) & SIM_FCFG2_SWAPPFLSH_MASK)

/*! @name UIDH - Unique Identification Register High */
#define SIM_UIDH_UID_MASK                        (0xFFFFFFFFU)
#define SIM_UIDH_UID_SHIFT                       (0U)
#define SIM_UIDH_UID(x)                          (((uint32_t)(((uint32_t)(x)) << SIM_UIDH_UID_SHIFT)) & SIM_UIDH_UID_MASK)

/*! @name UIDMH - Unique Identification Register Mid-High */
#define SIM_UIDMH_UID_MASK                       (0xFFFFFFFFU)
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
  __IO uint8_t VLLSCTRL;                           /**< VLLS Control register, offset: 0x2 */
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
#define SMC_PMCTRL_LPWUI_MASK                    (0x80U)
#define SMC_PMCTRL_LPWUI_SHIFT                   (7U)
#define SMC_PMCTRL_LPWUI(x)                      (((uint8_t)(((uint8_t)(x)) << SMC_PMCTRL_LPWUI_SHIFT)) & SMC_PMCTRL_LPWUI_MASK)

/*! @name VLLSCTRL - VLLS Control register */
#define SMC_VLLSCTRL_VLLSM_MASK                  (0x7U)
#define SMC_VLLSCTRL_VLLSM_SHIFT                 (0U)
#define SMC_VLLSCTRL_VLLSM(x)                    (((uint8_t)(((uint8_t)(x)) << SMC_VLLSCTRL_VLLSM_SHIFT)) & SMC_VLLSCTRL_VLLSM_MASK)
#define SMC_VLLSCTRL_RAM2PO_MASK                 (0x10U)
#define SMC_VLLSCTRL_RAM2PO_SHIFT                (4U)
#define SMC_VLLSCTRL_RAM2PO(x)                   (((uint8_t)(((uint8_t)(x)) << SMC_VLLSCTRL_RAM2PO_SHIFT)) & SMC_VLLSCTRL_RAM2PO_MASK)
#define SMC_VLLSCTRL_PORPO_MASK                  (0x20U)
#define SMC_VLLSCTRL_PORPO_SHIFT                 (5U)
#define SMC_VLLSCTRL_PORPO(x)                    (((uint8_t)(((uint8_t)(x)) << SMC_VLLSCTRL_PORPO_SHIFT)) & SMC_VLLSCTRL_PORPO_MASK)

/*! @name PMSTAT - Power Mode Status register */
#define SMC_PMSTAT_PMSTAT_MASK                   (0x7FU)
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
  __IO uint32_t SR;                                /**< DSPI Status Register, offset: 0x2C */
  __IO uint32_t RSER;                              /**< DMA/Interrupt Request Select and Enable Register, offset: 0x30 */
  union {                                          /* offset: 0x34 */
    __IO uint32_t PUSHR;                             /**< PUSH TX FIFO Register In Master Mode, offset: 0x34 */
    __IO uint32_t PUSHR_SLAVE;                       /**< PUSH TX FIFO Register In Slave Mode, offset: 0x34 */
  };
  __I  uint32_t POPR;                              /**< POP RX FIFO Register, offset: 0x38 */
  __I  uint32_t TXFR0;                             /**< DSPI Transmit FIFO Registers, offset: 0x3C */
  __I  uint32_t TXFR1;                             /**< DSPI Transmit FIFO Registers, offset: 0x40 */
  __I  uint32_t TXFR2;                             /**< DSPI Transmit FIFO Registers, offset: 0x44 */
  __I  uint32_t TXFR3;                             /**< DSPI Transmit FIFO Registers, offset: 0x48 */
       uint8_t RESERVED_2[48];
  __I  uint32_t RXFR0;                             /**< DSPI Receive FIFO Registers, offset: 0x7C */
  __I  uint32_t RXFR1;                             /**< DSPI Receive FIFO Registers, offset: 0x80 */
  __I  uint32_t RXFR2;                             /**< DSPI Receive FIFO Registers, offset: 0x84 */
  __I  uint32_t RXFR3;                             /**< DSPI Receive FIFO Registers, offset: 0x88 */
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
#define SPI_MCR_PCSIS_MASK                       (0x1F0000U)
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
#define SPI_CTAR_SLAVE_FMSZ_MASK                 (0xF8000000U)
#define SPI_CTAR_SLAVE_FMSZ_SHIFT                (27U)
#define SPI_CTAR_SLAVE_FMSZ(x)                   (((uint32_t)(((uint32_t)(x)) << SPI_CTAR_SLAVE_FMSZ_SHIFT)) & SPI_CTAR_SLAVE_FMSZ_MASK)

/* The count of SPI_CTAR_SLAVE */
#define SPI_CTAR_SLAVE_COUNT                     (1U)

/*! @name SR - DSPI Status Register */
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
#define SPI_PUSHR_PCS_MASK                       (0x3F0000U)
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
#define SPI_PUSHR_SLAVE_TXDATA_MASK              (0xFFFFFFFFU)
#define SPI_PUSHR_SLAVE_TXDATA_SHIFT             (0U)
#define SPI_PUSHR_SLAVE_TXDATA(x)                (((uint32_t)(((uint32_t)(x)) << SPI_PUSHR_SLAVE_TXDATA_SHIFT)) & SPI_PUSHR_SLAVE_TXDATA_MASK)

/*! @name POPR - POP RX FIFO Register */
#define SPI_POPR_RXDATA_MASK                     (0xFFFFFFFFU)
#define SPI_POPR_RXDATA_SHIFT                    (0U)
#define SPI_POPR_RXDATA(x)                       (((uint32_t)(((uint32_t)(x)) << SPI_POPR_RXDATA_SHIFT)) & SPI_POPR_RXDATA_MASK)

/*! @name TXFR0 - DSPI Transmit FIFO Registers */
#define SPI_TXFR0_TXDATA_MASK                    (0xFFFFU)
#define SPI_TXFR0_TXDATA_SHIFT                   (0U)
#define SPI_TXFR0_TXDATA(x)                      (((uint32_t)(((uint32_t)(x)) << SPI_TXFR0_TXDATA_SHIFT)) & SPI_TXFR0_TXDATA_MASK)
#define SPI_TXFR0_TXCMD_TXDATA_MASK              (0xFFFF0000U)
#define SPI_TXFR0_TXCMD_TXDATA_SHIFT             (16U)
#define SPI_TXFR0_TXCMD_TXDATA(x)                (((uint32_t)(((uint32_t)(x)) << SPI_TXFR0_TXCMD_TXDATA_SHIFT)) & SPI_TXFR0_TXCMD_TXDATA_MASK)

/*! @name TXFR1 - DSPI Transmit FIFO Registers */
#define SPI_TXFR1_TXDATA_MASK                    (0xFFFFU)
#define SPI_TXFR1_TXDATA_SHIFT                   (0U)
#define SPI_TXFR1_TXDATA(x)                      (((uint32_t)(((uint32_t)(x)) << SPI_TXFR1_TXDATA_SHIFT)) & SPI_TXFR1_TXDATA_MASK)
#define SPI_TXFR1_TXCMD_TXDATA_MASK              (0xFFFF0000U)
#define SPI_TXFR1_TXCMD_TXDATA_SHIFT             (16U)
#define SPI_TXFR1_TXCMD_TXDATA(x)                (((uint32_t)(((uint32_t)(x)) << SPI_TXFR1_TXCMD_TXDATA_SHIFT)) & SPI_TXFR1_TXCMD_TXDATA_MASK)

/*! @name TXFR2 - DSPI Transmit FIFO Registers */
#define SPI_TXFR2_TXDATA_MASK                    (0xFFFFU)
#define SPI_TXFR2_TXDATA_SHIFT                   (0U)
#define SPI_TXFR2_TXDATA(x)                      (((uint32_t)(((uint32_t)(x)) << SPI_TXFR2_TXDATA_SHIFT)) & SPI_TXFR2_TXDATA_MASK)
#define SPI_TXFR2_TXCMD_TXDATA_MASK              (0xFFFF0000U)
#define SPI_TXFR2_TXCMD_TXDATA_SHIFT             (16U)
#define SPI_TXFR2_TXCMD_TXDATA(x)                (((uint32_t)(((uint32_t)(x)) << SPI_TXFR2_TXCMD_TXDATA_SHIFT)) & SPI_TXFR2_TXCMD_TXDATA_MASK)

/*! @name TXFR3 - DSPI Transmit FIFO Registers */
#define SPI_TXFR3_TXDATA_MASK                    (0xFFFFU)
#define SPI_TXFR3_TXDATA_SHIFT                   (0U)
#define SPI_TXFR3_TXDATA(x)                      (((uint32_t)(((uint32_t)(x)) << SPI_TXFR3_TXDATA_SHIFT)) & SPI_TXFR3_TXDATA_MASK)
#define SPI_TXFR3_TXCMD_TXDATA_MASK              (0xFFFF0000U)
#define SPI_TXFR3_TXCMD_TXDATA_SHIFT             (16U)
#define SPI_TXFR3_TXCMD_TXDATA(x)                (((uint32_t)(((uint32_t)(x)) << SPI_TXFR3_TXCMD_TXDATA_SHIFT)) & SPI_TXFR3_TXCMD_TXDATA_MASK)

/*! @name RXFR0 - DSPI Receive FIFO Registers */
#define SPI_RXFR0_RXDATA_MASK                    (0xFFFFFFFFU)
#define SPI_RXFR0_RXDATA_SHIFT                   (0U)
#define SPI_RXFR0_RXDATA(x)                      (((uint32_t)(((uint32_t)(x)) << SPI_RXFR0_RXDATA_SHIFT)) & SPI_RXFR0_RXDATA_MASK)

/*! @name RXFR1 - DSPI Receive FIFO Registers */
#define SPI_RXFR1_RXDATA_MASK                    (0xFFFFFFFFU)
#define SPI_RXFR1_RXDATA_SHIFT                   (0U)
#define SPI_RXFR1_RXDATA(x)                      (((uint32_t)(((uint32_t)(x)) << SPI_RXFR1_RXDATA_SHIFT)) & SPI_RXFR1_RXDATA_MASK)

/*! @name RXFR2 - DSPI Receive FIFO Registers */
#define SPI_RXFR2_RXDATA_MASK                    (0xFFFFFFFFU)
#define SPI_RXFR2_RXDATA_SHIFT                   (0U)
#define SPI_RXFR2_RXDATA(x)                      (((uint32_t)(((uint32_t)(x)) << SPI_RXFR2_RXDATA_SHIFT)) & SPI_RXFR2_RXDATA_MASK)

/*! @name RXFR3 - DSPI Receive FIFO Registers */
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
   -- UART Peripheral Access Layer
   ---------------------------------------------------------------------------- */

/*!
 * @addtogroup UART_Peripheral_Access_Layer UART Peripheral Access Layer
 * @{
 */

/** UART - Register Layout Typedef */
typedef struct {
  __IO uint8_t BDH;                                /**< UART Baud Rate Registers: High, offset: 0x0 */
  __IO uint8_t BDL;                                /**< UART Baud Rate Registers: Low, offset: 0x1 */
  __IO uint8_t C1;                                 /**< UART Control Register 1, offset: 0x2 */
  __IO uint8_t C2;                                 /**< UART Control Register 2, offset: 0x3 */
  __I  uint8_t S1;                                 /**< UART Status Register 1, offset: 0x4 */
  __IO uint8_t S2;                                 /**< UART Status Register 2, offset: 0x5 */
  __IO uint8_t C3;                                 /**< UART Control Register 3, offset: 0x6 */
  __IO uint8_t D;                                  /**< UART Data Register, offset: 0x7 */
  __IO uint8_t MA1;                                /**< UART Match Address Registers 1, offset: 0x8 */
  __IO uint8_t MA2;                                /**< UART Match Address Registers 2, offset: 0x9 */
  __IO uint8_t C4;                                 /**< UART Control Register 4, offset: 0xA */
  __IO uint8_t C5;                                 /**< UART Control Register 5, offset: 0xB */
  __I  uint8_t ED;                                 /**< UART Extended Data Register, offset: 0xC */
  __IO uint8_t MODEM;                              /**< UART Modem Register, offset: 0xD */
  __IO uint8_t IR;                                 /**< UART Infrared Register, offset: 0xE */
       uint8_t RESERVED_0[1];
  __IO uint8_t PFIFO;                              /**< UART FIFO Parameters, offset: 0x10 */
  __IO uint8_t CFIFO;                              /**< UART FIFO Control Register, offset: 0x11 */
  __IO uint8_t SFIFO;                              /**< UART FIFO Status Register, offset: 0x12 */
  __IO uint8_t TWFIFO;                             /**< UART FIFO Transmit Watermark, offset: 0x13 */
  __I  uint8_t TCFIFO;                             /**< UART FIFO Transmit Count, offset: 0x14 */
  __IO uint8_t RWFIFO;                             /**< UART FIFO Receive Watermark, offset: 0x15 */
  __I  uint8_t RCFIFO;                             /**< UART FIFO Receive Count, offset: 0x16 */
       uint8_t RESERVED_1[1];
  __IO uint8_t C7816;                              /**< UART 7816 Control Register, offset: 0x18 */
  __IO uint8_t IE7816;                             /**< UART 7816 Interrupt Enable Register, offset: 0x19 */
  __IO uint8_t IS7816;                             /**< UART 7816 Interrupt Status Register, offset: 0x1A */
  union {                                          /* offset: 0x1B */
    __IO uint8_t WP7816T0;                           /**< UART 7816 Wait Parameter Register, offset: 0x1B */
    __IO uint8_t WP7816T1;                           /**< UART 7816 Wait Parameter Register, offset: 0x1B */
  };
  __IO uint8_t WN7816;                             /**< UART 7816 Wait N Register, offset: 0x1C */
  __IO uint8_t WF7816;                             /**< UART 7816 Wait FD Register, offset: 0x1D */
  __IO uint8_t ET7816;                             /**< UART 7816 Error Threshold Register, offset: 0x1E */
  __IO uint8_t TL7816;                             /**< UART 7816 Transmit Length Register, offset: 0x1F */
} UART_Type;

/* ----------------------------------------------------------------------------
   -- UART Register Masks
   ---------------------------------------------------------------------------- */

/*!
 * @addtogroup UART_Register_Masks UART Register Masks
 * @{
 */

/*! @name BDH - UART Baud Rate Registers: High */
#define UART_BDH_SBR_MASK                        (0x1FU)
#define UART_BDH_SBR_SHIFT                       (0U)
#define UART_BDH_SBR(x)                          (((uint8_t)(((uint8_t)(x)) << UART_BDH_SBR_SHIFT)) & UART_BDH_SBR_MASK)
#define UART_BDH_RXEDGIE_MASK                    (0x40U)
#define UART_BDH_RXEDGIE_SHIFT                   (6U)
#define UART_BDH_RXEDGIE(x)                      (((uint8_t)(((uint8_t)(x)) << UART_BDH_RXEDGIE_SHIFT)) & UART_BDH_RXEDGIE_MASK)
#define UART_BDH_LBKDIE_MASK                     (0x80U)
#define UART_BDH_LBKDIE_SHIFT                    (7U)
#define UART_BDH_LBKDIE(x)                       (((uint8_t)(((uint8_t)(x)) << UART_BDH_LBKDIE_SHIFT)) & UART_BDH_LBKDIE_MASK)

/*! @name BDL - UART Baud Rate Registers: Low */
#define UART_BDL_SBR_MASK                        (0xFFU)
#define UART_BDL_SBR_SHIFT                       (0U)
#define UART_BDL_SBR(x)                          (((uint8_t)(((uint8_t)(x)) << UART_BDL_SBR_SHIFT)) & UART_BDL_SBR_MASK)

/*! @name C1 - UART Control Register 1 */
#define UART_C1_PT_MASK                          (0x1U)
#define UART_C1_PT_SHIFT                         (0U)
#define UART_C1_PT(x)                            (((uint8_t)(((uint8_t)(x)) << UART_C1_PT_SHIFT)) & UART_C1_PT_MASK)
#define UART_C1_PE_MASK                          (0x2U)
#define UART_C1_PE_SHIFT                         (1U)
#define UART_C1_PE(x)                            (((uint8_t)(((uint8_t)(x)) << UART_C1_PE_SHIFT)) & UART_C1_PE_MASK)
#define UART_C1_ILT_MASK                         (0x4U)
#define UART_C1_ILT_SHIFT                        (2U)
#define UART_C1_ILT(x)                           (((uint8_t)(((uint8_t)(x)) << UART_C1_ILT_SHIFT)) & UART_C1_ILT_MASK)
#define UART_C1_WAKE_MASK                        (0x8U)
#define UART_C1_WAKE_SHIFT                       (3U)
#define UART_C1_WAKE(x)                          (((uint8_t)(((uint8_t)(x)) << UART_C1_WAKE_SHIFT)) & UART_C1_WAKE_MASK)
#define UART_C1_M_MASK                           (0x10U)
#define UART_C1_M_SHIFT                          (4U)
#define UART_C1_M(x)                             (((uint8_t)(((uint8_t)(x)) << UART_C1_M_SHIFT)) & UART_C1_M_MASK)
#define UART_C1_RSRC_MASK                        (0x20U)
#define UART_C1_RSRC_SHIFT                       (5U)
#define UART_C1_RSRC(x)                          (((uint8_t)(((uint8_t)(x)) << UART_C1_RSRC_SHIFT)) & UART_C1_RSRC_MASK)
#define UART_C1_UARTSWAI_MASK                    (0x40U)
#define UART_C1_UARTSWAI_SHIFT                   (6U)
#define UART_C1_UARTSWAI(x)                      (((uint8_t)(((uint8_t)(x)) << UART_C1_UARTSWAI_SHIFT)) & UART_C1_UARTSWAI_MASK)
#define UART_C1_LOOPS_MASK                       (0x80U)
#define UART_C1_LOOPS_SHIFT                      (7U)
#define UART_C1_LOOPS(x)                         (((uint8_t)(((uint8_t)(x)) << UART_C1_LOOPS_SHIFT)) & UART_C1_LOOPS_MASK)

/*! @name C2 - UART Control Register 2 */
#define UART_C2_SBK_MASK                         (0x1U)
#define UART_C2_SBK_SHIFT                        (0U)
#define UART_C2_SBK(x)                           (((uint8_t)(((uint8_t)(x)) << UART_C2_SBK_SHIFT)) & UART_C2_SBK_MASK)
#define UART_C2_RWU_MASK                         (0x2U)
#define UART_C2_RWU_SHIFT                        (1U)
#define UART_C2_RWU(x)                           (((uint8_t)(((uint8_t)(x)) << UART_C2_RWU_SHIFT)) & UART_C2_RWU_MASK)
#define UART_C2_RE_MASK                          (0x4U)
#define UART_C2_RE_SHIFT                         (2U)
#define UART_C2_RE(x)                            (((uint8_t)(((uint8_t)(x)) << UART_C2_RE_SHIFT)) & UART_C2_RE_MASK)
#define UART_C2_TE_MASK                          (0x8U)
#define UART_C2_TE_SHIFT                         (3U)
#define UART_C2_TE(x)                            (((uint8_t)(((uint8_t)(x)) << UART_C2_TE_SHIFT)) & UART_C2_TE_MASK)
#define UART_C2_ILIE_MASK                        (0x10U)
#define UART_C2_ILIE_SHIFT                       (4U)
#define UART_C2_ILIE(x)                          (((uint8_t)(((uint8_t)(x)) << UART_C2_ILIE_SHIFT)) & UART_C2_ILIE_MASK)
#define UART_C2_RIE_MASK                         (0x20U)
#define UART_C2_RIE_SHIFT                        (5U)
#define UART_C2_RIE(x)                           (((uint8_t)(((uint8_t)(x)) << UART_C2_RIE_SHIFT)) & UART_C2_RIE_MASK)
#define UART_C2_TCIE_MASK                        (0x40U)
#define UART_C2_TCIE_SHIFT                       (6U)
#define UART_C2_TCIE(x)                          (((uint8_t)(((uint8_t)(x)) << UART_C2_TCIE_SHIFT)) & UART_C2_TCIE_MASK)
#define UART_C2_TIE_MASK                         (0x80U)
#define UART_C2_TIE_SHIFT                        (7U)
#define UART_C2_TIE(x)                           (((uint8_t)(((uint8_t)(x)) << UART_C2_TIE_SHIFT)) & UART_C2_TIE_MASK)

/*! @name S1 - UART Status Register 1 */
#define UART_S1_PF_MASK                          (0x1U)
#define UART_S1_PF_SHIFT                         (0U)
#define UART_S1_PF(x)                            (((uint8_t)(((uint8_t)(x)) << UART_S1_PF_SHIFT)) & UART_S1_PF_MASK)
#define UART_S1_FE_MASK                          (0x2U)
#define UART_S1_FE_SHIFT                         (1U)
#define UART_S1_FE(x)                            (((uint8_t)(((uint8_t)(x)) << UART_S1_FE_SHIFT)) & UART_S1_FE_MASK)
#define UART_S1_NF_MASK                          (0x4U)
#define UART_S1_NF_SHIFT                         (2U)
#define UART_S1_NF(x)                            (((uint8_t)(((uint8_t)(x)) << UART_S1_NF_SHIFT)) & UART_S1_NF_MASK)
#define UART_S1_OR_MASK                          (0x8U)
#define UART_S1_OR_SHIFT                         (3U)
#define UART_S1_OR(x)                            (((uint8_t)(((uint8_t)(x)) << UART_S1_OR_SHIFT)) & UART_S1_OR_MASK)
#define UART_S1_IDLE_MASK                        (0x10U)
#define UART_S1_IDLE_SHIFT                       (4U)
#define UART_S1_IDLE(x)                          (((uint8_t)(((uint8_t)(x)) << UART_S1_IDLE_SHIFT)) & UART_S1_IDLE_MASK)
#define UART_S1_RDRF_MASK                        (0x20U)
#define UART_S1_RDRF_SHIFT                       (5U)
#define UART_S1_RDRF(x)                          (((uint8_t)(((uint8_t)(x)) << UART_S1_RDRF_SHIFT)) & UART_S1_RDRF_MASK)
#define UART_S1_TC_MASK                          (0x40U)
#define UART_S1_TC_SHIFT                         (6U)
#define UART_S1_TC(x)                            (((uint8_t)(((uint8_t)(x)) << UART_S1_TC_SHIFT)) & UART_S1_TC_MASK)
#define UART_S1_TDRE_MASK                        (0x80U)
#define UART_S1_TDRE_SHIFT                       (7U)
#define UART_S1_TDRE(x)                          (((uint8_t)(((uint8_t)(x)) << UART_S1_TDRE_SHIFT)) & UART_S1_TDRE_MASK)

/*! @name S2 - UART Status Register 2 */
#define UART_S2_RAF_MASK                         (0x1U)
#define UART_S2_RAF_SHIFT                        (0U)
#define UART_S2_RAF(x)                           (((uint8_t)(((uint8_t)(x)) << UART_S2_RAF_SHIFT)) & UART_S2_RAF_MASK)
#define UART_S2_LBKDE_MASK                       (0x2U)
#define UART_S2_LBKDE_SHIFT                      (1U)
#define UART_S2_LBKDE(x)                         (((uint8_t)(((uint8_t)(x)) << UART_S2_LBKDE_SHIFT)) & UART_S2_LBKDE_MASK)
#define UART_S2_BRK13_MASK                       (0x4U)
#define UART_S2_BRK13_SHIFT                      (2U)
#define UART_S2_BRK13(x)                         (((uint8_t)(((uint8_t)(x)) << UART_S2_BRK13_SHIFT)) & UART_S2_BRK13_MASK)
#define UART_S2_RWUID_MASK                       (0x8U)
#define UART_S2_RWUID_SHIFT                      (3U)
#define UART_S2_RWUID(x)                         (((uint8_t)(((uint8_t)(x)) << UART_S2_RWUID_SHIFT)) & UART_S2_RWUID_MASK)
#define UART_S2_RXINV_MASK                       (0x10U)
#define UART_S2_RXINV_SHIFT                      (4U)
#define UART_S2_RXINV(x)                         (((uint8_t)(((uint8_t)(x)) << UART_S2_RXINV_SHIFT)) & UART_S2_RXINV_MASK)
#define UART_S2_MSBF_MASK                        (0x20U)
#define UART_S2_MSBF_SHIFT                       (5U)
#define UART_S2_MSBF(x)                          (((uint8_t)(((uint8_t)(x)) << UART_S2_MSBF_SHIFT)) & UART_S2_MSBF_MASK)
#define UART_S2_RXEDGIF_MASK                     (0x40U)
#define UART_S2_RXEDGIF_SHIFT                    (6U)
#define UART_S2_RXEDGIF(x)                       (((uint8_t)(((uint8_t)(x)) << UART_S2_RXEDGIF_SHIFT)) & UART_S2_RXEDGIF_MASK)
#define UART_S2_LBKDIF_MASK                      (0x80U)
#define UART_S2_LBKDIF_SHIFT                     (7U)
#define UART_S2_LBKDIF(x)                        (((uint8_t)(((uint8_t)(x)) << UART_S2_LBKDIF_SHIFT)) & UART_S2_LBKDIF_MASK)

/*! @name C3 - UART Control Register 3 */
#define UART_C3_PEIE_MASK                        (0x1U)
#define UART_C3_PEIE_SHIFT                       (0U)
#define UART_C3_PEIE(x)                          (((uint8_t)(((uint8_t)(x)) << UART_C3_PEIE_SHIFT)) & UART_C3_PEIE_MASK)
#define UART_C3_FEIE_MASK                        (0x2U)
#define UART_C3_FEIE_SHIFT                       (1U)
#define UART_C3_FEIE(x)                          (((uint8_t)(((uint8_t)(x)) << UART_C3_FEIE_SHIFT)) & UART_C3_FEIE_MASK)
#define UART_C3_NEIE_MASK                        (0x4U)
#define UART_C3_NEIE_SHIFT                       (2U)
#define UART_C3_NEIE(x)                          (((uint8_t)(((uint8_t)(x)) << UART_C3_NEIE_SHIFT)) & UART_C3_NEIE_MASK)
#define UART_C3_ORIE_MASK                        (0x8U)
#define UART_C3_ORIE_SHIFT                       (3U)
#define UART_C3_ORIE(x)                          (((uint8_t)(((uint8_t)(x)) << UART_C3_ORIE_SHIFT)) & UART_C3_ORIE_MASK)
#define UART_C3_TXINV_MASK                       (0x10U)
#define UART_C3_TXINV_SHIFT                      (4U)
#define UART_C3_TXINV(x)                         (((uint8_t)(((uint8_t)(x)) << UART_C3_TXINV_SHIFT)) & UART_C3_TXINV_MASK)
#define UART_C3_TXDIR_MASK                       (0x20U)
#define UART_C3_TXDIR_SHIFT                      (5U)
#define UART_C3_TXDIR(x)                         (((uint8_t)(((uint8_t)(x)) << UART_C3_TXDIR_SHIFT)) & UART_C3_TXDIR_MASK)
#define UART_C3_T8_MASK                          (0x40U)
#define UART_C3_T8_SHIFT                         (6U)
#define UART_C3_T8(x)                            (((uint8_t)(((uint8_t)(x)) << UART_C3_T8_SHIFT)) & UART_C3_T8_MASK)
#define UART_C3_R8_MASK                          (0x80U)
#define UART_C3_R8_SHIFT                         (7U)
#define UART_C3_R8(x)                            (((uint8_t)(((uint8_t)(x)) << UART_C3_R8_SHIFT)) & UART_C3_R8_MASK)

/*! @name D - UART Data Register */
#define UART_D_RT_MASK                           (0xFFU)
#define UART_D_RT_SHIFT                          (0U)
#define UART_D_RT(x)                             (((uint8_t)(((uint8_t)(x)) << UART_D_RT_SHIFT)) & UART_D_RT_MASK)

/*! @name MA1 - UART Match Address Registers 1 */
#define UART_MA1_MA_MASK                         (0xFFU)
#define UART_MA1_MA_SHIFT                        (0U)
#define UART_MA1_MA(x)                           (((uint8_t)(((uint8_t)(x)) << UART_MA1_MA_SHIFT)) & UART_MA1_MA_MASK)

/*! @name MA2 - UART Match Address Registers 2 */
#define UART_MA2_MA_MASK                         (0xFFU)
#define UART_MA2_MA_SHIFT                        (0U)
#define UART_MA2_MA(x)                           (((uint8_t)(((uint8_t)(x)) << UART_MA2_MA_SHIFT)) & UART_MA2_MA_MASK)

/*! @name C4 - UART Control Register 4 */
#define UART_C4_BRFA_MASK                        (0x1FU)
#define UART_C4_BRFA_SHIFT                       (0U)
#define UART_C4_BRFA(x)                          (((uint8_t)(((uint8_t)(x)) << UART_C4_BRFA_SHIFT)) & UART_C4_BRFA_MASK)
#define UART_C4_M10_MASK                         (0x20U)
#define UART_C4_M10_SHIFT                        (5U)
#define UART_C4_M10(x)                           (((uint8_t)(((uint8_t)(x)) << UART_C4_M10_SHIFT)) & UART_C4_M10_MASK)
#define UART_C4_MAEN2_MASK                       (0x40U)
#define UART_C4_MAEN2_SHIFT                      (6U)
#define UART_C4_MAEN2(x)                         (((uint8_t)(((uint8_t)(x)) << UART_C4_MAEN2_SHIFT)) & UART_C4_MAEN2_MASK)
#define UART_C4_MAEN1_MASK                       (0x80U)
#define UART_C4_MAEN1_SHIFT                      (7U)
#define UART_C4_MAEN1(x)                         (((uint8_t)(((uint8_t)(x)) << UART_C4_MAEN1_SHIFT)) & UART_C4_MAEN1_MASK)

/*! @name C5 - UART Control Register 5 */
#define UART_C5_RDMAS_MASK                       (0x20U)
#define UART_C5_RDMAS_SHIFT                      (5U)
#define UART_C5_RDMAS(x)                         (((uint8_t)(((uint8_t)(x)) << UART_C5_RDMAS_SHIFT)) & UART_C5_RDMAS_MASK)
#define UART_C5_TDMAS_MASK                       (0x80U)
#define UART_C5_TDMAS_SHIFT                      (7U)
#define UART_C5_TDMAS(x)                         (((uint8_t)(((uint8_t)(x)) << UART_C5_TDMAS_SHIFT)) & UART_C5_TDMAS_MASK)

/*! @name ED - UART Extended Data Register */
#define UART_ED_PARITYE_MASK                     (0x40U)
#define UART_ED_PARITYE_SHIFT                    (6U)
#define UART_ED_PARITYE(x)                       (((uint8_t)(((uint8_t)(x)) << UART_ED_PARITYE_SHIFT)) & UART_ED_PARITYE_MASK)
#define UART_ED_NOISY_MASK                       (0x80U)
#define UART_ED_NOISY_SHIFT                      (7U)
#define UART_ED_NOISY(x)                         (((uint8_t)(((uint8_t)(x)) << UART_ED_NOISY_SHIFT)) & UART_ED_NOISY_MASK)

/*! @name MODEM - UART Modem Register */
#define UART_MODEM_TXCTSE_MASK                   (0x1U)
#define UART_MODEM_TXCTSE_SHIFT                  (0U)
#define UART_MODEM_TXCTSE(x)                     (((uint8_t)(((uint8_t)(x)) << UART_MODEM_TXCTSE_SHIFT)) & UART_MODEM_TXCTSE_MASK)
#define UART_MODEM_TXRTSE_MASK                   (0x2U)
#define UART_MODEM_TXRTSE_SHIFT                  (1U)
#define UART_MODEM_TXRTSE(x)                     (((uint8_t)(((uint8_t)(x)) << UART_MODEM_TXRTSE_SHIFT)) & UART_MODEM_TXRTSE_MASK)
#define UART_MODEM_TXRTSPOL_MASK                 (0x4U)
#define UART_MODEM_TXRTSPOL_SHIFT                (2U)
#define UART_MODEM_TXRTSPOL(x)                   (((uint8_t)(((uint8_t)(x)) << UART_MODEM_TXRTSPOL_SHIFT)) & UART_MODEM_TXRTSPOL_MASK)
#define UART_MODEM_RXRTSE_MASK                   (0x8U)
#define UART_MODEM_RXRTSE_SHIFT                  (3U)
#define UART_MODEM_RXRTSE(x)                     (((uint8_t)(((uint8_t)(x)) << UART_MODEM_RXRTSE_SHIFT)) & UART_MODEM_RXRTSE_MASK)

/*! @name IR - UART Infrared Register */
#define UART_IR_TNP_MASK                         (0x3U)
#define UART_IR_TNP_SHIFT                        (0U)
#define UART_IR_TNP(x)                           (((uint8_t)(((uint8_t)(x)) << UART_IR_TNP_SHIFT)) & UART_IR_TNP_MASK)
#define UART_IR_IREN_MASK                        (0x4U)
#define UART_IR_IREN_SHIFT                       (2U)
#define UART_IR_IREN(x)                          (((uint8_t)(((uint8_t)(x)) << UART_IR_IREN_SHIFT)) & UART_IR_IREN_MASK)

/*! @name PFIFO - UART FIFO Parameters */
#define UART_PFIFO_RXFIFOSIZE_MASK               (0x7U)
#define UART_PFIFO_RXFIFOSIZE_SHIFT              (0U)
#define UART_PFIFO_RXFIFOSIZE(x)                 (((uint8_t)(((uint8_t)(x)) << UART_PFIFO_RXFIFOSIZE_SHIFT)) & UART_PFIFO_RXFIFOSIZE_MASK)
#define UART_PFIFO_RXFE_MASK                     (0x8U)
#define UART_PFIFO_RXFE_SHIFT                    (3U)
#define UART_PFIFO_RXFE(x)                       (((uint8_t)(((uint8_t)(x)) << UART_PFIFO_RXFE_SHIFT)) & UART_PFIFO_RXFE_MASK)
#define UART_PFIFO_TXFIFOSIZE_MASK               (0x70U)
#define UART_PFIFO_TXFIFOSIZE_SHIFT              (4U)
#define UART_PFIFO_TXFIFOSIZE(x)                 (((uint8_t)(((uint8_t)(x)) << UART_PFIFO_TXFIFOSIZE_SHIFT)) & UART_PFIFO_TXFIFOSIZE_MASK)
#define UART_PFIFO_TXFE_MASK                     (0x80U)
#define UART_PFIFO_TXFE_SHIFT                    (7U)
#define UART_PFIFO_TXFE(x)                       (((uint8_t)(((uint8_t)(x)) << UART_PFIFO_TXFE_SHIFT)) & UART_PFIFO_TXFE_MASK)

/*! @name CFIFO - UART FIFO Control Register */
#define UART_CFIFO_RXUFE_MASK                    (0x1U)
#define UART_CFIFO_RXUFE_SHIFT                   (0U)
#define UART_CFIFO_RXUFE(x)                      (((uint8_t)(((uint8_t)(x)) << UART_CFIFO_RXUFE_SHIFT)) & UART_CFIFO_RXUFE_MASK)
#define UART_CFIFO_TXOFE_MASK                    (0x2U)
#define UART_CFIFO_TXOFE_SHIFT                   (1U)
#define UART_CFIFO_TXOFE(x)                      (((uint8_t)(((uint8_t)(x)) << UART_CFIFO_TXOFE_SHIFT)) & UART_CFIFO_TXOFE_MASK)
#define UART_CFIFO_RXOFE_MASK                    (0x4U)
#define UART_CFIFO_RXOFE_SHIFT                   (2U)
#define UART_CFIFO_RXOFE(x)                      (((uint8_t)(((uint8_t)(x)) << UART_CFIFO_RXOFE_SHIFT)) & UART_CFIFO_RXOFE_MASK)
#define UART_CFIFO_RXFLUSH_MASK                  (0x40U)
#define UART_CFIFO_RXFLUSH_SHIFT                 (6U)
#define UART_CFIFO_RXFLUSH(x)                    (((uint8_t)(((uint8_t)(x)) << UART_CFIFO_RXFLUSH_SHIFT)) & UART_CFIFO_RXFLUSH_MASK)
#define UART_CFIFO_TXFLUSH_MASK                  (0x80U)
#define UART_CFIFO_TXFLUSH_SHIFT                 (7U)
#define UART_CFIFO_TXFLUSH(x)                    (((uint8_t)(((uint8_t)(x)) << UART_CFIFO_TXFLUSH_SHIFT)) & UART_CFIFO_TXFLUSH_MASK)

/*! @name SFIFO - UART FIFO Status Register */
#define UART_SFIFO_RXUF_MASK                     (0x1U)
#define UART_SFIFO_RXUF_SHIFT                    (0U)
#define UART_SFIFO_RXUF(x)                       (((uint8_t)(((uint8_t)(x)) << UART_SFIFO_RXUF_SHIFT)) & UART_SFIFO_RXUF_MASK)
#define UART_SFIFO_TXOF_MASK                     (0x2U)
#define UART_SFIFO_TXOF_SHIFT                    (1U)
#define UART_SFIFO_TXOF(x)                       (((uint8_t)(((uint8_t)(x)) << UART_SFIFO_TXOF_SHIFT)) & UART_SFIFO_TXOF_MASK)
#define UART_SFIFO_RXOF_MASK                     (0x4U)
#define UART_SFIFO_RXOF_SHIFT                    (2U)
#define UART_SFIFO_RXOF(x)                       (((uint8_t)(((uint8_t)(x)) << UART_SFIFO_RXOF_SHIFT)) & UART_SFIFO_RXOF_MASK)
#define UART_SFIFO_RXEMPT_MASK                   (0x40U)
#define UART_SFIFO_RXEMPT_SHIFT                  (6U)
#define UART_SFIFO_RXEMPT(x)                     (((uint8_t)(((uint8_t)(x)) << UART_SFIFO_RXEMPT_SHIFT)) & UART_SFIFO_RXEMPT_MASK)
#define UART_SFIFO_TXEMPT_MASK                   (0x80U)
#define UART_SFIFO_TXEMPT_SHIFT                  (7U)
#define UART_SFIFO_TXEMPT(x)                     (((uint8_t)(((uint8_t)(x)) << UART_SFIFO_TXEMPT_SHIFT)) & UART_SFIFO_TXEMPT_MASK)

/*! @name TWFIFO - UART FIFO Transmit Watermark */
#define UART_TWFIFO_TXWATER_MASK                 (0xFFU)
#define UART_TWFIFO_TXWATER_SHIFT                (0U)
#define UART_TWFIFO_TXWATER(x)                   (((uint8_t)(((uint8_t)(x)) << UART_TWFIFO_TXWATER_SHIFT)) & UART_TWFIFO_TXWATER_MASK)

/*! @name TCFIFO - UART FIFO Transmit Count */
#define UART_TCFIFO_TXCOUNT_MASK                 (0xFFU)
#define UART_TCFIFO_TXCOUNT_SHIFT                (0U)
#define UART_TCFIFO_TXCOUNT(x)                   (((uint8_t)(((uint8_t)(x)) << UART_TCFIFO_TXCOUNT_SHIFT)) & UART_TCFIFO_TXCOUNT_MASK)

/*! @name RWFIFO - UART FIFO Receive Watermark */
#define UART_RWFIFO_RXWATER_MASK                 (0xFFU)
#define UART_RWFIFO_RXWATER_SHIFT                (0U)
#define UART_RWFIFO_RXWATER(x)                   (((uint8_t)(((uint8_t)(x)) << UART_RWFIFO_RXWATER_SHIFT)) & UART_RWFIFO_RXWATER_MASK)

/*! @name RCFIFO - UART FIFO Receive Count */
#define UART_RCFIFO_RXCOUNT_MASK                 (0xFFU)
#define UART_RCFIFO_RXCOUNT_SHIFT                (0U)
#define UART_RCFIFO_RXCOUNT(x)                   (((uint8_t)(((uint8_t)(x)) << UART_RCFIFO_RXCOUNT_SHIFT)) & UART_RCFIFO_RXCOUNT_MASK)

/*! @name C7816 - UART 7816 Control Register */
#define UART_C7816_ISO_7816E_MASK                (0x1U)
#define UART_C7816_ISO_7816E_SHIFT               (0U)
#define UART_C7816_ISO_7816E(x)                  (((uint8_t)(((uint8_t)(x)) << UART_C7816_ISO_7816E_SHIFT)) & UART_C7816_ISO_7816E_MASK)
#define UART_C7816_TTYPE_MASK                    (0x2U)
#define UART_C7816_TTYPE_SHIFT                   (1U)
#define UART_C7816_TTYPE(x)                      (((uint8_t)(((uint8_t)(x)) << UART_C7816_TTYPE_SHIFT)) & UART_C7816_TTYPE_MASK)
#define UART_C7816_INIT_MASK                     (0x4U)
#define UART_C7816_INIT_SHIFT                    (2U)
#define UART_C7816_INIT(x)                       (((uint8_t)(((uint8_t)(x)) << UART_C7816_INIT_SHIFT)) & UART_C7816_INIT_MASK)
#define UART_C7816_ANACK_MASK                    (0x8U)
#define UART_C7816_ANACK_SHIFT                   (3U)
#define UART_C7816_ANACK(x)                      (((uint8_t)(((uint8_t)(x)) << UART_C7816_ANACK_SHIFT)) & UART_C7816_ANACK_MASK)
#define UART_C7816_ONACK_MASK                    (0x10U)
#define UART_C7816_ONACK_SHIFT                   (4U)
#define UART_C7816_ONACK(x)                      (((uint8_t)(((uint8_t)(x)) << UART_C7816_ONACK_SHIFT)) & UART_C7816_ONACK_MASK)

/*! @name IE7816 - UART 7816 Interrupt Enable Register */
#define UART_IE7816_RXTE_MASK                    (0x1U)
#define UART_IE7816_RXTE_SHIFT                   (0U)
#define UART_IE7816_RXTE(x)                      (((uint8_t)(((uint8_t)(x)) << UART_IE7816_RXTE_SHIFT)) & UART_IE7816_RXTE_MASK)
#define UART_IE7816_TXTE_MASK                    (0x2U)
#define UART_IE7816_TXTE_SHIFT                   (1U)
#define UART_IE7816_TXTE(x)                      (((uint8_t)(((uint8_t)(x)) << UART_IE7816_TXTE_SHIFT)) & UART_IE7816_TXTE_MASK)
#define UART_IE7816_GTVE_MASK                    (0x4U)
#define UART_IE7816_GTVE_SHIFT                   (2U)
#define UART_IE7816_GTVE(x)                      (((uint8_t)(((uint8_t)(x)) << UART_IE7816_GTVE_SHIFT)) & UART_IE7816_GTVE_MASK)
#define UART_IE7816_INITDE_MASK                  (0x10U)
#define UART_IE7816_INITDE_SHIFT                 (4U)
#define UART_IE7816_INITDE(x)                    (((uint8_t)(((uint8_t)(x)) << UART_IE7816_INITDE_SHIFT)) & UART_IE7816_INITDE_MASK)
#define UART_IE7816_BWTE_MASK                    (0x20U)
#define UART_IE7816_BWTE_SHIFT                   (5U)
#define UART_IE7816_BWTE(x)                      (((uint8_t)(((uint8_t)(x)) << UART_IE7816_BWTE_SHIFT)) & UART_IE7816_BWTE_MASK)
#define UART_IE7816_CWTE_MASK                    (0x40U)
#define UART_IE7816_CWTE_SHIFT                   (6U)
#define UART_IE7816_CWTE(x)                      (((uint8_t)(((uint8_t)(x)) << UART_IE7816_CWTE_SHIFT)) & UART_IE7816_CWTE_MASK)
#define UART_IE7816_WTE_MASK                     (0x80U)
#define UART_IE7816_WTE_SHIFT                    (7U)
#define UART_IE7816_WTE(x)                       (((uint8_t)(((uint8_t)(x)) << UART_IE7816_WTE_SHIFT)) & UART_IE7816_WTE_MASK)

/*! @name IS7816 - UART 7816 Interrupt Status Register */
#define UART_IS7816_RXT_MASK                     (0x1U)
#define UART_IS7816_RXT_SHIFT                    (0U)
#define UART_IS7816_RXT(x)                       (((uint8_t)(((uint8_t)(x)) << UART_IS7816_RXT_SHIFT)) & UART_IS7816_RXT_MASK)
#define UART_IS7816_TXT_MASK                     (0x2U)
#define UART_IS7816_TXT_SHIFT                    (1U)
#define UART_IS7816_TXT(x)                       (((uint8_t)(((uint8_t)(x)) << UART_IS7816_TXT_SHIFT)) & UART_IS7816_TXT_MASK)
#define UART_IS7816_GTV_MASK                     (0x4U)
#define UART_IS7816_GTV_SHIFT                    (2U)
#define UART_IS7816_GTV(x)                       (((uint8_t)(((uint8_t)(x)) << UART_IS7816_GTV_SHIFT)) & UART_IS7816_GTV_MASK)
#define UART_IS7816_INITD_MASK                   (0x10U)
#define UART_IS7816_INITD_SHIFT                  (4U)
#define UART_IS7816_INITD(x)                     (((uint8_t)(((uint8_t)(x)) << UART_IS7816_INITD_SHIFT)) & UART_IS7816_INITD_MASK)
#define UART_IS7816_BWT_MASK                     (0x20U)
#define UART_IS7816_BWT_SHIFT                    (5U)
#define UART_IS7816_BWT(x)                       (((uint8_t)(((uint8_t)(x)) << UART_IS7816_BWT_SHIFT)) & UART_IS7816_BWT_MASK)
#define UART_IS7816_CWT_MASK                     (0x40U)
#define UART_IS7816_CWT_SHIFT                    (6U)
#define UART_IS7816_CWT(x)                       (((uint8_t)(((uint8_t)(x)) << UART_IS7816_CWT_SHIFT)) & UART_IS7816_CWT_MASK)
#define UART_IS7816_WT_MASK                      (0x80U)
#define UART_IS7816_WT_SHIFT                     (7U)
#define UART_IS7816_WT(x)                        (((uint8_t)(((uint8_t)(x)) << UART_IS7816_WT_SHIFT)) & UART_IS7816_WT_MASK)

/*! @name WP7816T0 - UART 7816 Wait Parameter Register */
#define UART_WP7816T0_WI_MASK                    (0xFFU)
#define UART_WP7816T0_WI_SHIFT                   (0U)
#define UART_WP7816T0_WI(x)                      (((uint8_t)(((uint8_t)(x)) << UART_WP7816T0_WI_SHIFT)) & UART_WP7816T0_WI_MASK)

/*! @name WP7816T1 - UART 7816 Wait Parameter Register */
#define UART_WP7816T1_BWI_MASK                   (0xFU)
#define UART_WP7816T1_BWI_SHIFT                  (0U)
#define UART_WP7816T1_BWI(x)                     (((uint8_t)(((uint8_t)(x)) << UART_WP7816T1_BWI_SHIFT)) & UART_WP7816T1_BWI_MASK)
#define UART_WP7816T1_CWI_MASK                   (0xF0U)
#define UART_WP7816T1_CWI_SHIFT                  (4U)
#define UART_WP7816T1_CWI(x)                     (((uint8_t)(((uint8_t)(x)) << UART_WP7816T1_CWI_SHIFT)) & UART_WP7816T1_CWI_MASK)

/*! @name WN7816 - UART 7816 Wait N Register */
#define UART_WN7816_GTN_MASK                     (0xFFU)
#define UART_WN7816_GTN_SHIFT                    (0U)
#define UART_WN7816_GTN(x)                       (((uint8_t)(((uint8_t)(x)) << UART_WN7816_GTN_SHIFT)) & UART_WN7816_GTN_MASK)

/*! @name WF7816 - UART 7816 Wait FD Register */
#define UART_WF7816_GTFD_MASK                    (0xFFU)
#define UART_WF7816_GTFD_SHIFT                   (0U)
#define UART_WF7816_GTFD(x)                      (((uint8_t)(((uint8_t)(x)) << UART_WF7816_GTFD_SHIFT)) & UART_WF7816_GTFD_MASK)

/*! @name ET7816 - UART 7816 Error Threshold Register */
#define UART_ET7816_RXTHRESHOLD_MASK             (0xFU)
#define UART_ET7816_RXTHRESHOLD_SHIFT            (0U)
#define UART_ET7816_RXTHRESHOLD(x)               (((uint8_t)(((uint8_t)(x)) << UART_ET7816_RXTHRESHOLD_SHIFT)) & UART_ET7816_RXTHRESHOLD_MASK)
#define UART_ET7816_TXTHRESHOLD_MASK             (0xF0U)
#define UART_ET7816_TXTHRESHOLD_SHIFT            (4U)
#define UART_ET7816_TXTHRESHOLD(x)               (((uint8_t)(((uint8_t)(x)) << UART_ET7816_TXTHRESHOLD_SHIFT)) & UART_ET7816_TXTHRESHOLD_MASK)

/*! @name TL7816 - UART 7816 Transmit Length Register */
#define UART_TL7816_TLEN_MASK                    (0xFFU)
#define UART_TL7816_TLEN_SHIFT                   (0U)
#define UART_TL7816_TLEN(x)                      (((uint8_t)(((uint8_t)(x)) << UART_TL7816_TLEN_SHIFT)) & UART_TL7816_TLEN_MASK)


/*!
 * @}
 */ /* end of group UART_Register_Masks */


/* UART - Peripheral instance base addresses */
/** Peripheral UART0 base address */
#define UART0_BASE                               (0x4006A000u)
/** Peripheral UART0 base pointer */
#define UART0                                    ((UART_Type *)UART0_BASE)
/** Peripheral UART1 base address */
#define UART1_BASE                               (0x4006B000u)
/** Peripheral UART1 base pointer */
#define UART1                                    ((UART_Type *)UART1_BASE)
/** Peripheral UART2 base address */
#define UART2_BASE                               (0x4006C000u)
/** Peripheral UART2 base pointer */
#define UART2                                    ((UART_Type *)UART2_BASE)
/** Array initializer of UART peripheral base addresses */
#define UART_BASE_ADDRS                          { UART0_BASE, UART1_BASE, UART2_BASE }
/** Array initializer of UART peripheral base pointers */
#define UART_BASE_PTRS                           { UART0, UART1, UART2 }
/** Interrupt vectors for the UART peripheral type */
#define UART_RX_TX_IRQS                          { UART0_RX_TX_IRQn, UART1_RX_TX_IRQn, UART2_RX_TX_IRQn }
#define UART_ERR_IRQS                            { UART0_ERR_IRQn, UART1_ERR_IRQn, UART2_ERR_IRQn }

/*!
 * @}
 */ /* end of group UART_Peripheral_Access_Layer */


/* ----------------------------------------------------------------------------
   -- USB Peripheral Access Layer
   ---------------------------------------------------------------------------- */

/*!
 * @addtogroup USB_Peripheral_Access_Layer USB Peripheral Access Layer
 * @{
 */

/** USB - Register Layout Typedef */
typedef struct {
  __I  uint8_t PERID;                              /**< Peripheral ID register, offset: 0x0 */
       uint8_t RESERVED_0[3];
  __I  uint8_t IDCOMP;                             /**< Peripheral ID Complement register, offset: 0x4 */
       uint8_t RESERVED_1[3];
  __I  uint8_t REV;                                /**< Peripheral Revision register, offset: 0x8 */
       uint8_t RESERVED_2[3];
  __I  uint8_t ADDINFO;                            /**< Peripheral Additional Info register, offset: 0xC */
       uint8_t RESERVED_3[3];
  __IO uint8_t OTGISTAT;                           /**< OTG Interrupt Status register, offset: 0x10 */
       uint8_t RESERVED_4[3];
  __IO uint8_t OTGICR;                             /**< OTG Interrupt Control Register, offset: 0x14 */
       uint8_t RESERVED_5[3];
  __IO uint8_t OTGSTAT;                            /**< OTG Status register, offset: 0x18 */
       uint8_t RESERVED_6[3];
  __IO uint8_t OTGCTL;                             /**< OTG Control register, offset: 0x1C */
       uint8_t RESERVED_7[99];
  __IO uint8_t ISTAT;                              /**< Interrupt Status register, offset: 0x80 */
       uint8_t RESERVED_8[3];
  __IO uint8_t INTEN;                              /**< Interrupt Enable register, offset: 0x84 */
       uint8_t RESERVED_9[3];
  __IO uint8_t ERRSTAT;                            /**< Error Interrupt Status register, offset: 0x88 */
       uint8_t RESERVED_10[3];
  __IO uint8_t ERREN;                              /**< Error Interrupt Enable register, offset: 0x8C */
       uint8_t RESERVED_11[3];
  __I  uint8_t STAT;                               /**< Status register, offset: 0x90 */
       uint8_t RESERVED_12[3];
  __IO uint8_t CTL;                                /**< Control register, offset: 0x94 */
       uint8_t RESERVED_13[3];
  __IO uint8_t ADDR;                               /**< Address register, offset: 0x98 */
       uint8_t RESERVED_14[3];
  __IO uint8_t BDTPAGE1;                           /**< BDT Page Register 1, offset: 0x9C */
       uint8_t RESERVED_15[3];
  __IO uint8_t FRMNUML;                            /**< Frame Number Register Low, offset: 0xA0 */
       uint8_t RESERVED_16[3];
  __IO uint8_t FRMNUMH;                            /**< Frame Number Register High, offset: 0xA4 */
       uint8_t RESERVED_17[3];
  __IO uint8_t TOKEN;                              /**< Token register, offset: 0xA8 */
       uint8_t RESERVED_18[3];
  __IO uint8_t SOFTHLD;                            /**< SOF Threshold Register, offset: 0xAC */
       uint8_t RESERVED_19[3];
  __IO uint8_t BDTPAGE2;                           /**< BDT Page Register 2, offset: 0xB0 */
       uint8_t RESERVED_20[3];
  __IO uint8_t BDTPAGE3;                           /**< BDT Page Register 3, offset: 0xB4 */
       uint8_t RESERVED_21[11];
  struct {                                         /* offset: 0xC0, array step: 0x4 */
    __IO uint8_t ENDPT;                              /**< Endpoint Control register, array offset: 0xC0, array step: 0x4 */
         uint8_t RESERVED_0[3];
  } ENDPOINT[16];
  __IO uint8_t USBCTRL;                            /**< USB Control register, offset: 0x100 */
       uint8_t RESERVED_22[3];
  __I  uint8_t OBSERVE;                            /**< USB OTG Observe register, offset: 0x104 */
       uint8_t RESERVED_23[3];
  __IO uint8_t CONTROL;                            /**< USB OTG Control register, offset: 0x108 */
       uint8_t RESERVED_24[3];
  __IO uint8_t USBTRC0;                            /**< USB Transceiver Control Register 0, offset: 0x10C */
       uint8_t RESERVED_25[7];
  __IO uint8_t USBFRMADJUST;                       /**< Frame Adjust Register, offset: 0x114 */
} USB_Type;

/* ----------------------------------------------------------------------------
   -- USB Register Masks
   ---------------------------------------------------------------------------- */

/*!
 * @addtogroup USB_Register_Masks USB Register Masks
 * @{
 */

/*! @name PERID - Peripheral ID register */
#define USB_PERID_ID_MASK                        (0x3FU)
#define USB_PERID_ID_SHIFT                       (0U)
#define USB_PERID_ID(x)                          (((uint8_t)(((uint8_t)(x)) << USB_PERID_ID_SHIFT)) & USB_PERID_ID_MASK)

/*! @name IDCOMP - Peripheral ID Complement register */
#define USB_IDCOMP_NID_MASK                      (0x3FU)
#define USB_IDCOMP_NID_SHIFT                     (0U)
#define USB_IDCOMP_NID(x)                        (((uint8_t)(((uint8_t)(x)) << USB_IDCOMP_NID_SHIFT)) & USB_IDCOMP_NID_MASK)

/*! @name REV - Peripheral Revision register */
#define USB_REV_REV_MASK                         (0xFFU)
#define USB_REV_REV_SHIFT                        (0U)
#define USB_REV_REV(x)                           (((uint8_t)(((uint8_t)(x)) << USB_REV_REV_SHIFT)) & USB_REV_REV_MASK)

/*! @name ADDINFO - Peripheral Additional Info register */
#define USB_ADDINFO_IEHOST_MASK                  (0x1U)
#define USB_ADDINFO_IEHOST_SHIFT                 (0U)
#define USB_ADDINFO_IEHOST(x)                    (((uint8_t)(((uint8_t)(x)) << USB_ADDINFO_IEHOST_SHIFT)) & USB_ADDINFO_IEHOST_MASK)
#define USB_ADDINFO_IRQNUM_MASK                  (0xF8U)
#define USB_ADDINFO_IRQNUM_SHIFT                 (3U)
#define USB_ADDINFO_IRQNUM(x)                    (((uint8_t)(((uint8_t)(x)) << USB_ADDINFO_IRQNUM_SHIFT)) & USB_ADDINFO_IRQNUM_MASK)

/*! @name OTGISTAT - OTG Interrupt Status register */
#define USB_OTGISTAT_AVBUSCHG_MASK               (0x1U)
#define USB_OTGISTAT_AVBUSCHG_SHIFT              (0U)
#define USB_OTGISTAT_AVBUSCHG(x)                 (((uint8_t)(((uint8_t)(x)) << USB_OTGISTAT_AVBUSCHG_SHIFT)) & USB_OTGISTAT_AVBUSCHG_MASK)
#define USB_OTGISTAT_B_SESS_CHG_MASK             (0x4U)
#define USB_OTGISTAT_B_SESS_CHG_SHIFT            (2U)
#define USB_OTGISTAT_B_SESS_CHG(x)               (((uint8_t)(((uint8_t)(x)) << USB_OTGISTAT_B_SESS_CHG_SHIFT)) & USB_OTGISTAT_B_SESS_CHG_MASK)
#define USB_OTGISTAT_SESSVLDCHG_MASK             (0x8U)
#define USB_OTGISTAT_SESSVLDCHG_SHIFT            (3U)
#define USB_OTGISTAT_SESSVLDCHG(x)               (((uint8_t)(((uint8_t)(x)) << USB_OTGISTAT_SESSVLDCHG_SHIFT)) & USB_OTGISTAT_SESSVLDCHG_MASK)
#define USB_OTGISTAT_LINE_STATE_CHG_MASK         (0x20U)
#define USB_OTGISTAT_LINE_STATE_CHG_SHIFT        (5U)
#define USB_OTGISTAT_LINE_STATE_CHG(x)           (((uint8_t)(((uint8_t)(x)) << USB_OTGISTAT_LINE_STATE_CHG_SHIFT)) & USB_OTGISTAT_LINE_STATE_CHG_MASK)
#define USB_OTGISTAT_ONEMSEC_MASK                (0x40U)
#define USB_OTGISTAT_ONEMSEC_SHIFT               (6U)
#define USB_OTGISTAT_ONEMSEC(x)                  (((uint8_t)(((uint8_t)(x)) << USB_OTGISTAT_ONEMSEC_SHIFT)) & USB_OTGISTAT_ONEMSEC_MASK)
#define USB_OTGISTAT_IDCHG_MASK                  (0x80U)
#define USB_OTGISTAT_IDCHG_SHIFT                 (7U)
#define USB_OTGISTAT_IDCHG(x)                    (((uint8_t)(((uint8_t)(x)) << USB_OTGISTAT_IDCHG_SHIFT)) & USB_OTGISTAT_IDCHG_MASK)

/*! @name OTGICR - OTG Interrupt Control Register */
#define USB_OTGICR_AVBUSEN_MASK                  (0x1U)
#define USB_OTGICR_AVBUSEN_SHIFT                 (0U)
#define USB_OTGICR_AVBUSEN(x)                    (((uint8_t)(((uint8_t)(x)) << USB_OTGICR_AVBUSEN_SHIFT)) & USB_OTGICR_AVBUSEN_MASK)
#define USB_OTGICR_BSESSEN_MASK                  (0x4U)
#define USB_OTGICR_BSESSEN_SHIFT                 (2U)
#define USB_OTGICR_BSESSEN(x)                    (((uint8_t)(((uint8_t)(x)) << USB_OTGICR_BSESSEN_SHIFT)) & USB_OTGICR_BSESSEN_MASK)
#define USB_OTGICR_SESSVLDEN_MASK                (0x8U)
#define USB_OTGICR_SESSVLDEN_SHIFT               (3U)
#define USB_OTGICR_SESSVLDEN(x)                  (((uint8_t)(((uint8_t)(x)) << USB_OTGICR_SESSVLDEN_SHIFT)) & USB_OTGICR_SESSVLDEN_MASK)
#define USB_OTGICR_LINESTATEEN_MASK              (0x20U)
#define USB_OTGICR_LINESTATEEN_SHIFT             (5U)
#define USB_OTGICR_LINESTATEEN(x)                (((uint8_t)(((uint8_t)(x)) << USB_OTGICR_LINESTATEEN_SHIFT)) & USB_OTGICR_LINESTATEEN_MASK)
#define USB_OTGICR_ONEMSECEN_MASK                (0x40U)
#define USB_OTGICR_ONEMSECEN_SHIFT               (6U)
#define USB_OTGICR_ONEMSECEN(x)                  (((uint8_t)(((uint8_t)(x)) << USB_OTGICR_ONEMSECEN_SHIFT)) & USB_OTGICR_ONEMSECEN_MASK)
#define USB_OTGICR_IDEN_MASK                     (0x80U)
#define USB_OTGICR_IDEN_SHIFT                    (7U)
#define USB_OTGICR_IDEN(x)                       (((uint8_t)(((uint8_t)(x)) << USB_OTGICR_IDEN_SHIFT)) & USB_OTGICR_IDEN_MASK)

/*! @name OTGSTAT - OTG Status register */
#define USB_OTGSTAT_AVBUSVLD_MASK                (0x1U)
#define USB_OTGSTAT_AVBUSVLD_SHIFT               (0U)
#define USB_OTGSTAT_AVBUSVLD(x)                  (((uint8_t)(((uint8_t)(x)) << USB_OTGSTAT_AVBUSVLD_SHIFT)) & USB_OTGSTAT_AVBUSVLD_MASK)
#define USB_OTGSTAT_BSESSEND_MASK                (0x4U)
#define USB_OTGSTAT_BSESSEND_SHIFT               (2U)
#define USB_OTGSTAT_BSESSEND(x)                  (((uint8_t)(((uint8_t)(x)) << USB_OTGSTAT_BSESSEND_SHIFT)) & USB_OTGSTAT_BSESSEND_MASK)
#define USB_OTGSTAT_SESS_VLD_MASK                (0x8U)
#define USB_OTGSTAT_SESS_VLD_SHIFT               (3U)
#define USB_OTGSTAT_SESS_VLD(x)                  (((uint8_t)(((uint8_t)(x)) << USB_OTGSTAT_SESS_VLD_SHIFT)) & USB_OTGSTAT_SESS_VLD_MASK)
#define USB_OTGSTAT_LINESTATESTABLE_MASK         (0x20U)
#define USB_OTGSTAT_LINESTATESTABLE_SHIFT        (5U)
#define USB_OTGSTAT_LINESTATESTABLE(x)           (((uint8_t)(((uint8_t)(x)) << USB_OTGSTAT_LINESTATESTABLE_SHIFT)) & USB_OTGSTAT_LINESTATESTABLE_MASK)
#define USB_OTGSTAT_ONEMSECEN_MASK               (0x40U)
#define USB_OTGSTAT_ONEMSECEN_SHIFT              (6U)
#define USB_OTGSTAT_ONEMSECEN(x)                 (((uint8_t)(((uint8_t)(x)) << USB_OTGSTAT_ONEMSECEN_SHIFT)) & USB_OTGSTAT_ONEMSECEN_MASK)
#define USB_OTGSTAT_ID_MASK                      (0x80U)
#define USB_OTGSTAT_ID_SHIFT                     (7U)
#define USB_OTGSTAT_ID(x)                        (((uint8_t)(((uint8_t)(x)) << USB_OTGSTAT_ID_SHIFT)) & USB_OTGSTAT_ID_MASK)

/*! @name OTGCTL - OTG Control register */
#define USB_OTGCTL_OTGEN_MASK                    (0x4U)
#define USB_OTGCTL_OTGEN_SHIFT                   (2U)
#define USB_OTGCTL_OTGEN(x)                      (((uint8_t)(((uint8_t)(x)) << USB_OTGCTL_OTGEN_SHIFT)) & USB_OTGCTL_OTGEN_MASK)
#define USB_OTGCTL_DMLOW_MASK                    (0x10U)
#define USB_OTGCTL_DMLOW_SHIFT                   (4U)
#define USB_OTGCTL_DMLOW(x)                      (((uint8_t)(((uint8_t)(x)) << USB_OTGCTL_DMLOW_SHIFT)) & USB_OTGCTL_DMLOW_MASK)
#define USB_OTGCTL_DPLOW_MASK                    (0x20U)
#define USB_OTGCTL_DPLOW_SHIFT                   (5U)
#define USB_OTGCTL_DPLOW(x)                      (((uint8_t)(((uint8_t)(x)) << USB_OTGCTL_DPLOW_SHIFT)) & USB_OTGCTL_DPLOW_MASK)
#define USB_OTGCTL_DPHIGH_MASK                   (0x80U)
#define USB_OTGCTL_DPHIGH_SHIFT                  (7U)
#define USB_OTGCTL_DPHIGH(x)                     (((uint8_t)(((uint8_t)(x)) << USB_OTGCTL_DPHIGH_SHIFT)) & USB_OTGCTL_DPHIGH_MASK)

/*! @name ISTAT - Interrupt Status register */
#define USB_ISTAT_USBRST_MASK                    (0x1U)
#define USB_ISTAT_USBRST_SHIFT                   (0U)
#define USB_ISTAT_USBRST(x)                      (((uint8_t)(((uint8_t)(x)) << USB_ISTAT_USBRST_SHIFT)) & USB_ISTAT_USBRST_MASK)
#define USB_ISTAT_ERROR_MASK                     (0x2U)
#define USB_ISTAT_ERROR_SHIFT                    (1U)
#define USB_ISTAT_ERROR(x)                       (((uint8_t)(((uint8_t)(x)) << USB_ISTAT_ERROR_SHIFT)) & USB_ISTAT_ERROR_MASK)
#define USB_ISTAT_SOFTOK_MASK                    (0x4U)
#define USB_ISTAT_SOFTOK_SHIFT                   (2U)
#define USB_ISTAT_SOFTOK(x)                      (((uint8_t)(((uint8_t)(x)) << USB_ISTAT_SOFTOK_SHIFT)) & USB_ISTAT_SOFTOK_MASK)
#define USB_ISTAT_TOKDNE_MASK                    (0x8U)
#define USB_ISTAT_TOKDNE_SHIFT                   (3U)
#define USB_ISTAT_TOKDNE(x)                      (((uint8_t)(((uint8_t)(x)) << USB_ISTAT_TOKDNE_SHIFT)) & USB_ISTAT_TOKDNE_MASK)
#define USB_ISTAT_SLEEP_MASK                     (0x10U)
#define USB_ISTAT_SLEEP_SHIFT                    (4U)
#define USB_ISTAT_SLEEP(x)                       (((uint8_t)(((uint8_t)(x)) << USB_ISTAT_SLEEP_SHIFT)) & USB_ISTAT_SLEEP_MASK)
#define USB_ISTAT_RESUME_MASK                    (0x20U)
#define USB_ISTAT_RESUME_SHIFT                   (5U)
#define USB_ISTAT_RESUME(x)                      (((uint8_t)(((uint8_t)(x)) << USB_ISTAT_RESUME_SHIFT)) & USB_ISTAT_RESUME_MASK)
#define USB_ISTAT_ATTACH_MASK                    (0x40U)
#define USB_ISTAT_ATTACH_SHIFT                   (6U)
#define USB_ISTAT_ATTACH(x)                      (((uint8_t)(((uint8_t)(x)) << USB_ISTAT_ATTACH_SHIFT)) & USB_ISTAT_ATTACH_MASK)
#define USB_ISTAT_STALL_MASK                     (0x80U)
#define USB_ISTAT_STALL_SHIFT                    (7U)
#define USB_ISTAT_STALL(x)                       (((uint8_t)(((uint8_t)(x)) << USB_ISTAT_STALL_SHIFT)) & USB_ISTAT_STALL_MASK)

/*! @name INTEN - Interrupt Enable register */
#define USB_INTEN_USBRSTEN_MASK                  (0x1U)
#define USB_INTEN_USBRSTEN_SHIFT                 (0U)
#define USB_INTEN_USBRSTEN(x)                    (((uint8_t)(((uint8_t)(x)) << USB_INTEN_USBRSTEN_SHIFT)) & USB_INTEN_USBRSTEN_MASK)
#define USB_INTEN_ERROREN_MASK                   (0x2U)
#define USB_INTEN_ERROREN_SHIFT                  (1U)
#define USB_INTEN_ERROREN(x)                     (((uint8_t)(((uint8_t)(x)) << USB_INTEN_ERROREN_SHIFT)) & USB_INTEN_ERROREN_MASK)
#define USB_INTEN_SOFTOKEN_MASK                  (0x4U)
#define USB_INTEN_SOFTOKEN_SHIFT                 (2U)
#define USB_INTEN_SOFTOKEN(x)                    (((uint8_t)(((uint8_t)(x)) << USB_INTEN_SOFTOKEN_SHIFT)) & USB_INTEN_SOFTOKEN_MASK)
#define USB_INTEN_TOKDNEEN_MASK                  (0x8U)
#define USB_INTEN_TOKDNEEN_SHIFT                 (3U)
#define USB_INTEN_TOKDNEEN(x)                    (((uint8_t)(((uint8_t)(x)) << USB_INTEN_TOKDNEEN_SHIFT)) & USB_INTEN_TOKDNEEN_MASK)
#define USB_INTEN_SLEEPEN_MASK                   (0x10U)
#define USB_INTEN_SLEEPEN_SHIFT                  (4U)
#define USB_INTEN_SLEEPEN(x)                     (((uint8_t)(((uint8_t)(x)) << USB_INTEN_SLEEPEN_SHIFT)) & USB_INTEN_SLEEPEN_MASK)
#define USB_INTEN_RESUMEEN_MASK                  (0x20U)
#define USB_INTEN_RESUMEEN_SHIFT                 (5U)
#define USB_INTEN_RESUMEEN(x)                    (((uint8_t)(((uint8_t)(x)) << USB_INTEN_RESUMEEN_SHIFT)) & USB_INTEN_RESUMEEN_MASK)
#define USB_INTEN_ATTACHEN_MASK                  (0x40U)
#define USB_INTEN_ATTACHEN_SHIFT                 (6U)
#define USB_INTEN_ATTACHEN(x)                    (((uint8_t)(((uint8_t)(x)) << USB_INTEN_ATTACHEN_SHIFT)) & USB_INTEN_ATTACHEN_MASK)
#define USB_INTEN_STALLEN_MASK                   (0x80U)
#define USB_INTEN_STALLEN_SHIFT                  (7U)
#define USB_INTEN_STALLEN(x)                     (((uint8_t)(((uint8_t)(x)) << USB_INTEN_STALLEN_SHIFT)) & USB_INTEN_STALLEN_MASK)

/*! @name ERRSTAT - Error Interrupt Status register */
#define USB_ERRSTAT_PIDERR_MASK                  (0x1U)
#define USB_ERRSTAT_PIDERR_SHIFT                 (0U)
#define USB_ERRSTAT_PIDERR(x)                    (((uint8_t)(((uint8_t)(x)) << USB_ERRSTAT_PIDERR_SHIFT)) & USB_ERRSTAT_PIDERR_MASK)
#define USB_ERRSTAT_CRC5EOF_MASK                 (0x2U)
#define USB_ERRSTAT_CRC5EOF_SHIFT                (1U)
#define USB_ERRSTAT_CRC5EOF(x)                   (((uint8_t)(((uint8_t)(x)) << USB_ERRSTAT_CRC5EOF_SHIFT)) & USB_ERRSTAT_CRC5EOF_MASK)
#define USB_ERRSTAT_CRC16_MASK                   (0x4U)
#define USB_ERRSTAT_CRC16_SHIFT                  (2U)
#define USB_ERRSTAT_CRC16(x)                     (((uint8_t)(((uint8_t)(x)) << USB_ERRSTAT_CRC16_SHIFT)) & USB_ERRSTAT_CRC16_MASK)
#define USB_ERRSTAT_DFN8_MASK                    (0x8U)
#define USB_ERRSTAT_DFN8_SHIFT                   (3U)
#define USB_ERRSTAT_DFN8(x)                      (((uint8_t)(((uint8_t)(x)) << USB_ERRSTAT_DFN8_SHIFT)) & USB_ERRSTAT_DFN8_MASK)
#define USB_ERRSTAT_BTOERR_MASK                  (0x10U)
#define USB_ERRSTAT_BTOERR_SHIFT                 (4U)
#define USB_ERRSTAT_BTOERR(x)                    (((uint8_t)(((uint8_t)(x)) << USB_ERRSTAT_BTOERR_SHIFT)) & USB_ERRSTAT_BTOERR_MASK)
#define USB_ERRSTAT_DMAERR_MASK                  (0x20U)
#define USB_ERRSTAT_DMAERR_SHIFT                 (5U)
#define USB_ERRSTAT_DMAERR(x)                    (((uint8_t)(((uint8_t)(x)) << USB_ERRSTAT_DMAERR_SHIFT)) & USB_ERRSTAT_DMAERR_MASK)
#define USB_ERRSTAT_BTSERR_MASK                  (0x80U)
#define USB_ERRSTAT_BTSERR_SHIFT                 (7U)
#define USB_ERRSTAT_BTSERR(x)                    (((uint8_t)(((uint8_t)(x)) << USB_ERRSTAT_BTSERR_SHIFT)) & USB_ERRSTAT_BTSERR_MASK)

/*! @name ERREN - Error Interrupt Enable register */
#define USB_ERREN_PIDERREN_MASK                  (0x1U)
#define USB_ERREN_PIDERREN_SHIFT                 (0U)
#define USB_ERREN_PIDERREN(x)                    (((uint8_t)(((uint8_t)(x)) << USB_ERREN_PIDERREN_SHIFT)) & USB_ERREN_PIDERREN_MASK)
#define USB_ERREN_CRC5EOFEN_MASK                 (0x2U)
#define USB_ERREN_CRC5EOFEN_SHIFT                (1U)
#define USB_ERREN_CRC5EOFEN(x)                   (((uint8_t)(((uint8_t)(x)) << USB_ERREN_CRC5EOFEN_SHIFT)) & USB_ERREN_CRC5EOFEN_MASK)
#define USB_ERREN_CRC16EN_MASK                   (0x4U)
#define USB_ERREN_CRC16EN_SHIFT                  (2U)
#define USB_ERREN_CRC16EN(x)                     (((uint8_t)(((uint8_t)(x)) << USB_ERREN_CRC16EN_SHIFT)) & USB_ERREN_CRC16EN_MASK)
#define USB_ERREN_DFN8EN_MASK                    (0x8U)
#define USB_ERREN_DFN8EN_SHIFT                   (3U)
#define USB_ERREN_DFN8EN(x)                      (((uint8_t)(((uint8_t)(x)) << USB_ERREN_DFN8EN_SHIFT)) & USB_ERREN_DFN8EN_MASK)
#define USB_ERREN_BTOERREN_MASK                  (0x10U)
#define USB_ERREN_BTOERREN_SHIFT                 (4U)
#define USB_ERREN_BTOERREN(x)                    (((uint8_t)(((uint8_t)(x)) << USB_ERREN_BTOERREN_SHIFT)) & USB_ERREN_BTOERREN_MASK)
#define USB_ERREN_DMAERREN_MASK                  (0x20U)
#define USB_ERREN_DMAERREN_SHIFT                 (5U)
#define USB_ERREN_DMAERREN(x)                    (((uint8_t)(((uint8_t)(x)) << USB_ERREN_DMAERREN_SHIFT)) & USB_ERREN_DMAERREN_MASK)
#define USB_ERREN_BTSERREN_MASK                  (0x80U)
#define USB_ERREN_BTSERREN_SHIFT                 (7U)
#define USB_ERREN_BTSERREN(x)                    (((uint8_t)(((uint8_t)(x)) << USB_ERREN_BTSERREN_SHIFT)) & USB_ERREN_BTSERREN_MASK)

/*! @name STAT - Status register */
#define USB_STAT_ODD_MASK                        (0x4U)
#define USB_STAT_ODD_SHIFT                       (2U)
#define USB_STAT_ODD(x)                          (((uint8_t)(((uint8_t)(x)) << USB_STAT_ODD_SHIFT)) & USB_STAT_ODD_MASK)
#define USB_STAT_TX_MASK                         (0x8U)
#define USB_STAT_TX_SHIFT                        (3U)
#define USB_STAT_TX(x)                           (((uint8_t)(((uint8_t)(x)) << USB_STAT_TX_SHIFT)) & USB_STAT_TX_MASK)
#define USB_STAT_ENDP_MASK                       (0xF0U)
#define USB_STAT_ENDP_SHIFT                      (4U)
#define USB_STAT_ENDP(x)                         (((uint8_t)(((uint8_t)(x)) << USB_STAT_ENDP_SHIFT)) & USB_STAT_ENDP_MASK)

/*! @name CTL - Control register */
#define USB_CTL_USBENSOFEN_MASK                  (0x1U)
#define USB_CTL_USBENSOFEN_SHIFT                 (0U)
#define USB_CTL_USBENSOFEN(x)                    (((uint8_t)(((uint8_t)(x)) << USB_CTL_USBENSOFEN_SHIFT)) & USB_CTL_USBENSOFEN_MASK)
#define USB_CTL_ODDRST_MASK                      (0x2U)
#define USB_CTL_ODDRST_SHIFT                     (1U)
#define USB_CTL_ODDRST(x)                        (((uint8_t)(((uint8_t)(x)) << USB_CTL_ODDRST_SHIFT)) & USB_CTL_ODDRST_MASK)
#define USB_CTL_RESUME_MASK                      (0x4U)
#define USB_CTL_RESUME_SHIFT                     (2U)
#define USB_CTL_RESUME(x)                        (((uint8_t)(((uint8_t)(x)) << USB_CTL_RESUME_SHIFT)) & USB_CTL_RESUME_MASK)
#define USB_CTL_HOSTMODEEN_MASK                  (0x8U)
#define USB_CTL_HOSTMODEEN_SHIFT                 (3U)
#define USB_CTL_HOSTMODEEN(x)                    (((uint8_t)(((uint8_t)(x)) << USB_CTL_HOSTMODEEN_SHIFT)) & USB_CTL_HOSTMODEEN_MASK)
#define USB_CTL_RESET_MASK                       (0x10U)
#define USB_CTL_RESET_SHIFT                      (4U)
#define USB_CTL_RESET(x)                         (((uint8_t)(((uint8_t)(x)) << USB_CTL_RESET_SHIFT)) & USB_CTL_RESET_MASK)
#define USB_CTL_TXSUSPENDTOKENBUSY_MASK          (0x20U)
#define USB_CTL_TXSUSPENDTOKENBUSY_SHIFT         (5U)
#define USB_CTL_TXSUSPENDTOKENBUSY(x)            (((uint8_t)(((uint8_t)(x)) << USB_CTL_TXSUSPENDTOKENBUSY_SHIFT)) & USB_CTL_TXSUSPENDTOKENBUSY_MASK)
#define USB_CTL_SE0_MASK                         (0x40U)
#define USB_CTL_SE0_SHIFT                        (6U)
#define USB_CTL_SE0(x)                           (((uint8_t)(((uint8_t)(x)) << USB_CTL_SE0_SHIFT)) & USB_CTL_SE0_MASK)
#define USB_CTL_JSTATE_MASK                      (0x80U)
#define USB_CTL_JSTATE_SHIFT                     (7U)
#define USB_CTL_JSTATE(x)                        (((uint8_t)(((uint8_t)(x)) << USB_CTL_JSTATE_SHIFT)) & USB_CTL_JSTATE_MASK)

/*! @name ADDR - Address register */
#define USB_ADDR_ADDR_MASK                       (0x7FU)
#define USB_ADDR_ADDR_SHIFT                      (0U)
#define USB_ADDR_ADDR(x)                         (((uint8_t)(((uint8_t)(x)) << USB_ADDR_ADDR_SHIFT)) & USB_ADDR_ADDR_MASK)
#define USB_ADDR_LSEN_MASK                       (0x80U)
#define USB_ADDR_LSEN_SHIFT                      (7U)
#define USB_ADDR_LSEN(x)                         (((uint8_t)(((uint8_t)(x)) << USB_ADDR_LSEN_SHIFT)) & USB_ADDR_LSEN_MASK)

/*! @name BDTPAGE1 - BDT Page Register 1 */
#define USB_BDTPAGE1_BDTBA_MASK                  (0xFEU)
#define USB_BDTPAGE1_BDTBA_SHIFT                 (1U)
#define USB_BDTPAGE1_BDTBA(x)                    (((uint8_t)(((uint8_t)(x)) << USB_BDTPAGE1_BDTBA_SHIFT)) & USB_BDTPAGE1_BDTBA_MASK)

/*! @name FRMNUML - Frame Number Register Low */
#define USB_FRMNUML_FRM_MASK                     (0xFFU)
#define USB_FRMNUML_FRM_SHIFT                    (0U)
#define USB_FRMNUML_FRM(x)                       (((uint8_t)(((uint8_t)(x)) << USB_FRMNUML_FRM_SHIFT)) & USB_FRMNUML_FRM_MASK)

/*! @name FRMNUMH - Frame Number Register High */
#define USB_FRMNUMH_FRM_MASK                     (0x7U)
#define USB_FRMNUMH_FRM_SHIFT                    (0U)
#define USB_FRMNUMH_FRM(x)                       (((uint8_t)(((uint8_t)(x)) << USB_FRMNUMH_FRM_SHIFT)) & USB_FRMNUMH_FRM_MASK)

/*! @name TOKEN - Token register */
#define USB_TOKEN_TOKENENDPT_MASK                (0xFU)
#define USB_TOKEN_TOKENENDPT_SHIFT               (0U)
#define USB_TOKEN_TOKENENDPT(x)                  (((uint8_t)(((uint8_t)(x)) << USB_TOKEN_TOKENENDPT_SHIFT)) & USB_TOKEN_TOKENENDPT_MASK)
#define USB_TOKEN_TOKENPID_MASK                  (0xF0U)
#define USB_TOKEN_TOKENPID_SHIFT                 (4U)
#define USB_TOKEN_TOKENPID(x)                    (((uint8_t)(((uint8_t)(x)) << USB_TOKEN_TOKENPID_SHIFT)) & USB_TOKEN_TOKENPID_MASK)

/*! @name SOFTHLD - SOF Threshold Register */
#define USB_SOFTHLD_CNT_MASK                     (0xFFU)
#define USB_SOFTHLD_CNT_SHIFT                    (0U)
#define USB_SOFTHLD_CNT(x)                       (((uint8_t)(((uint8_t)(x)) << USB_SOFTHLD_CNT_SHIFT)) & USB_SOFTHLD_CNT_MASK)

/*! @name BDTPAGE2 - BDT Page Register 2 */
#define USB_BDTPAGE2_BDTBA_MASK                  (0xFFU)
#define USB_BDTPAGE2_BDTBA_SHIFT                 (0U)
#define USB_BDTPAGE2_BDTBA(x)                    (((uint8_t)(((uint8_t)(x)) << USB_BDTPAGE2_BDTBA_SHIFT)) & USB_BDTPAGE2_BDTBA_MASK)

/*! @name BDTPAGE3 - BDT Page Register 3 */
#define USB_BDTPAGE3_BDTBA_MASK                  (0xFFU)
#define USB_BDTPAGE3_BDTBA_SHIFT                 (0U)
#define USB_BDTPAGE3_BDTBA(x)                    (((uint8_t)(((uint8_t)(x)) << USB_BDTPAGE3_BDTBA_SHIFT)) & USB_BDTPAGE3_BDTBA_MASK)

/*! @name ENDPT - Endpoint Control register */
#define USB_ENDPT_EPHSHK_MASK                    (0x1U)
#define USB_ENDPT_EPHSHK_SHIFT                   (0U)
#define USB_ENDPT_EPHSHK(x)                      (((uint8_t)(((uint8_t)(x)) << USB_ENDPT_EPHSHK_SHIFT)) & USB_ENDPT_EPHSHK_MASK)
#define USB_ENDPT_EPSTALL_MASK                   (0x2U)
#define USB_ENDPT_EPSTALL_SHIFT                  (1U)
#define USB_ENDPT_EPSTALL(x)                     (((uint8_t)(((uint8_t)(x)) << USB_ENDPT_EPSTALL_SHIFT)) & USB_ENDPT_EPSTALL_MASK)
#define USB_ENDPT_EPTXEN_MASK                    (0x4U)
#define USB_ENDPT_EPTXEN_SHIFT                   (2U)
#define USB_ENDPT_EPTXEN(x)                      (((uint8_t)(((uint8_t)(x)) << USB_ENDPT_EPTXEN_SHIFT)) & USB_ENDPT_EPTXEN_MASK)
#define USB_ENDPT_EPRXEN_MASK                    (0x8U)
#define USB_ENDPT_EPRXEN_SHIFT                   (3U)
#define USB_ENDPT_EPRXEN(x)                      (((uint8_t)(((uint8_t)(x)) << USB_ENDPT_EPRXEN_SHIFT)) & USB_ENDPT_EPRXEN_MASK)
#define USB_ENDPT_EPCTLDIS_MASK                  (0x10U)
#define USB_ENDPT_EPCTLDIS_SHIFT                 (4U)
#define USB_ENDPT_EPCTLDIS(x)                    (((uint8_t)(((uint8_t)(x)) << USB_ENDPT_EPCTLDIS_SHIFT)) & USB_ENDPT_EPCTLDIS_MASK)
#define USB_ENDPT_RETRYDIS_MASK                  (0x40U)
#define USB_ENDPT_RETRYDIS_SHIFT                 (6U)
#define USB_ENDPT_RETRYDIS(x)                    (((uint8_t)(((uint8_t)(x)) << USB_ENDPT_RETRYDIS_SHIFT)) & USB_ENDPT_RETRYDIS_MASK)
#define USB_ENDPT_HOSTWOHUB_MASK                 (0x80U)
#define USB_ENDPT_HOSTWOHUB_SHIFT                (7U)
#define USB_ENDPT_HOSTWOHUB(x)                   (((uint8_t)(((uint8_t)(x)) << USB_ENDPT_HOSTWOHUB_SHIFT)) & USB_ENDPT_HOSTWOHUB_MASK)

/* The count of USB_ENDPT */
#define USB_ENDPT_COUNT                          (16U)

/*! @name USBCTRL - USB Control register */
#define USB_USBCTRL_PDE_MASK                     (0x40U)
#define USB_USBCTRL_PDE_SHIFT                    (6U)
#define USB_USBCTRL_PDE(x)                       (((uint8_t)(((uint8_t)(x)) << USB_USBCTRL_PDE_SHIFT)) & USB_USBCTRL_PDE_MASK)
#define USB_USBCTRL_SUSP_MASK                    (0x80U)
#define USB_USBCTRL_SUSP_SHIFT                   (7U)
#define USB_USBCTRL_SUSP(x)                      (((uint8_t)(((uint8_t)(x)) << USB_USBCTRL_SUSP_SHIFT)) & USB_USBCTRL_SUSP_MASK)

/*! @name OBSERVE - USB OTG Observe register */
#define USB_OBSERVE_DMPD_MASK                    (0x10U)
#define USB_OBSERVE_DMPD_SHIFT                   (4U)
#define USB_OBSERVE_DMPD(x)                      (((uint8_t)(((uint8_t)(x)) << USB_OBSERVE_DMPD_SHIFT)) & USB_OBSERVE_DMPD_MASK)
#define USB_OBSERVE_DPPD_MASK                    (0x40U)
#define USB_OBSERVE_DPPD_SHIFT                   (6U)
#define USB_OBSERVE_DPPD(x)                      (((uint8_t)(((uint8_t)(x)) << USB_OBSERVE_DPPD_SHIFT)) & USB_OBSERVE_DPPD_MASK)
#define USB_OBSERVE_DPPU_MASK                    (0x80U)
#define USB_OBSERVE_DPPU_SHIFT                   (7U)
#define USB_OBSERVE_DPPU(x)                      (((uint8_t)(((uint8_t)(x)) << USB_OBSERVE_DPPU_SHIFT)) & USB_OBSERVE_DPPU_MASK)

/*! @name CONTROL - USB OTG Control register */
#define USB_CONTROL_DPPULLUPNONOTG_MASK          (0x10U)
#define USB_CONTROL_DPPULLUPNONOTG_SHIFT         (4U)
#define USB_CONTROL_DPPULLUPNONOTG(x)            (((uint8_t)(((uint8_t)(x)) << USB_CONTROL_DPPULLUPNONOTG_SHIFT)) & USB_CONTROL_DPPULLUPNONOTG_MASK)

/*! @name USBTRC0 - USB Transceiver Control Register 0 */
#define USB_USBTRC0_USB_RESUME_INT_MASK          (0x1U)
#define USB_USBTRC0_USB_RESUME_INT_SHIFT         (0U)
#define USB_USBTRC0_USB_RESUME_INT(x)            (((uint8_t)(((uint8_t)(x)) << USB_USBTRC0_USB_RESUME_INT_SHIFT)) & USB_USBTRC0_USB_RESUME_INT_MASK)
#define USB_USBTRC0_SYNC_DET_MASK                (0x2U)
#define USB_USBTRC0_SYNC_DET_SHIFT               (1U)
#define USB_USBTRC0_SYNC_DET(x)                  (((uint8_t)(((uint8_t)(x)) << USB_USBTRC0_SYNC_DET_SHIFT)) & USB_USBTRC0_SYNC_DET_MASK)
#define USB_USBTRC0_USBRESMEN_MASK               (0x20U)
#define USB_USBTRC0_USBRESMEN_SHIFT              (5U)
#define USB_USBTRC0_USBRESMEN(x)                 (((uint8_t)(((uint8_t)(x)) << USB_USBTRC0_USBRESMEN_SHIFT)) & USB_USBTRC0_USBRESMEN_MASK)
#define USB_USBTRC0_USBRESET_MASK                (0x80U)
#define USB_USBTRC0_USBRESET_SHIFT               (7U)
#define USB_USBTRC0_USBRESET(x)                  (((uint8_t)(((uint8_t)(x)) << USB_USBTRC0_USBRESET_SHIFT)) & USB_USBTRC0_USBRESET_MASK)

/*! @name USBFRMADJUST - Frame Adjust Register */
#define USB_USBFRMADJUST_ADJ_MASK                (0xFFU)
#define USB_USBFRMADJUST_ADJ_SHIFT               (0U)
#define USB_USBFRMADJUST_ADJ(x)                  (((uint8_t)(((uint8_t)(x)) << USB_USBFRMADJUST_ADJ_SHIFT)) & USB_USBFRMADJUST_ADJ_MASK)


/*!
 * @}
 */ /* end of group USB_Register_Masks */


/* USB - Peripheral instance base addresses */
/** Peripheral USB0 base address */
#define USB0_BASE                                (0x40072000u)
/** Peripheral USB0 base pointer */
#define USB0                                     ((USB_Type *)USB0_BASE)
/** Array initializer of USB peripheral base addresses */
#define USB_BASE_ADDRS                           { USB0_BASE }
/** Array initializer of USB peripheral base pointers */
#define USB_BASE_PTRS                            { USB0 }
/** Interrupt vectors for the USB peripheral type */
#define USB_IRQS                                 { USB0_IRQn }

/*!
 * @}
 */ /* end of group USB_Peripheral_Access_Layer */


/* ----------------------------------------------------------------------------
   -- USBDCD Peripheral Access Layer
   ---------------------------------------------------------------------------- */

/*!
 * @addtogroup USBDCD_Peripheral_Access_Layer USBDCD Peripheral Access Layer
 * @{
 */

/** USBDCD - Register Layout Typedef */
typedef struct {
  __IO uint32_t CONTROL;                           /**< Control register, offset: 0x0 */
  __IO uint32_t CLOCK;                             /**< Clock register, offset: 0x4 */
  __I  uint32_t STATUS;                            /**< Status register, offset: 0x8 */
       uint8_t RESERVED_0[4];
  __IO uint32_t TIMER0;                            /**< TIMER0 register, offset: 0x10 */
  __IO uint32_t TIMER1;                            /**< TIMER1 register, offset: 0x14 */
  __IO uint32_t TIMER2;                            /**< TIMER2 register, offset: 0x18 */
} USBDCD_Type;

/* ----------------------------------------------------------------------------
   -- USBDCD Register Masks
   ---------------------------------------------------------------------------- */

/*!
 * @addtogroup USBDCD_Register_Masks USBDCD Register Masks
 * @{
 */

/*! @name CONTROL - Control register */
#define USBDCD_CONTROL_IACK_MASK                 (0x1U)
#define USBDCD_CONTROL_IACK_SHIFT                (0U)
#define USBDCD_CONTROL_IACK(x)                   (((uint32_t)(((uint32_t)(x)) << USBDCD_CONTROL_IACK_SHIFT)) & USBDCD_CONTROL_IACK_MASK)
#define USBDCD_CONTROL_IF_MASK                   (0x100U)
#define USBDCD_CONTROL_IF_SHIFT                  (8U)
#define USBDCD_CONTROL_IF(x)                     (((uint32_t)(((uint32_t)(x)) << USBDCD_CONTROL_IF_SHIFT)) & USBDCD_CONTROL_IF_MASK)
#define USBDCD_CONTROL_IE_MASK                   (0x10000U)
#define USBDCD_CONTROL_IE_SHIFT                  (16U)
#define USBDCD_CONTROL_IE(x)                     (((uint32_t)(((uint32_t)(x)) << USBDCD_CONTROL_IE_SHIFT)) & USBDCD_CONTROL_IE_MASK)
#define USBDCD_CONTROL_START_MASK                (0x1000000U)
#define USBDCD_CONTROL_START_SHIFT               (24U)
#define USBDCD_CONTROL_START(x)                  (((uint32_t)(((uint32_t)(x)) << USBDCD_CONTROL_START_SHIFT)) & USBDCD_CONTROL_START_MASK)
#define USBDCD_CONTROL_SR_MASK                   (0x2000000U)
#define USBDCD_CONTROL_SR_SHIFT                  (25U)
#define USBDCD_CONTROL_SR(x)                     (((uint32_t)(((uint32_t)(x)) << USBDCD_CONTROL_SR_SHIFT)) & USBDCD_CONTROL_SR_MASK)

/*! @name CLOCK - Clock register */
#define USBDCD_CLOCK_CLOCK_UNIT_MASK             (0x1U)
#define USBDCD_CLOCK_CLOCK_UNIT_SHIFT            (0U)
#define USBDCD_CLOCK_CLOCK_UNIT(x)               (((uint32_t)(((uint32_t)(x)) << USBDCD_CLOCK_CLOCK_UNIT_SHIFT)) & USBDCD_CLOCK_CLOCK_UNIT_MASK)
#define USBDCD_CLOCK_CLOCK_SPEED_MASK            (0xFFCU)
#define USBDCD_CLOCK_CLOCK_SPEED_SHIFT           (2U)
#define USBDCD_CLOCK_CLOCK_SPEED(x)              (((uint32_t)(((uint32_t)(x)) << USBDCD_CLOCK_CLOCK_SPEED_SHIFT)) & USBDCD_CLOCK_CLOCK_SPEED_MASK)

/*! @name STATUS - Status register */
#define USBDCD_STATUS_SEQ_RES_MASK               (0x30000U)
#define USBDCD_STATUS_SEQ_RES_SHIFT              (16U)
#define USBDCD_STATUS_SEQ_RES(x)                 (((uint32_t)(((uint32_t)(x)) << USBDCD_STATUS_SEQ_RES_SHIFT)) & USBDCD_STATUS_SEQ_RES_MASK)
#define USBDCD_STATUS_SEQ_STAT_MASK              (0xC0000U)
#define USBDCD_STATUS_SEQ_STAT_SHIFT             (18U)
#define USBDCD_STATUS_SEQ_STAT(x)                (((uint32_t)(((uint32_t)(x)) << USBDCD_STATUS_SEQ_STAT_SHIFT)) & USBDCD_STATUS_SEQ_STAT_MASK)
#define USBDCD_STATUS_ERR_MASK                   (0x100000U)
#define USBDCD_STATUS_ERR_SHIFT                  (20U)
#define USBDCD_STATUS_ERR(x)                     (((uint32_t)(((uint32_t)(x)) << USBDCD_STATUS_ERR_SHIFT)) & USBDCD_STATUS_ERR_MASK)
#define USBDCD_STATUS_TO_MASK                    (0x200000U)
#define USBDCD_STATUS_TO_SHIFT                   (21U)
#define USBDCD_STATUS_TO(x)                      (((uint32_t)(((uint32_t)(x)) << USBDCD_STATUS_TO_SHIFT)) & USBDCD_STATUS_TO_MASK)
#define USBDCD_STATUS_ACTIVE_MASK                (0x400000U)
#define USBDCD_STATUS_ACTIVE_SHIFT               (22U)
#define USBDCD_STATUS_ACTIVE(x)                  (((uint32_t)(((uint32_t)(x)) << USBDCD_STATUS_ACTIVE_SHIFT)) & USBDCD_STATUS_ACTIVE_MASK)

/*! @name TIMER0 - TIMER0 register */
#define USBDCD_TIMER0_TUNITCON_MASK              (0xFFFU)
#define USBDCD_TIMER0_TUNITCON_SHIFT             (0U)
#define USBDCD_TIMER0_TUNITCON(x)                (((uint32_t)(((uint32_t)(x)) << USBDCD_TIMER0_TUNITCON_SHIFT)) & USBDCD_TIMER0_TUNITCON_MASK)
#define USBDCD_TIMER0_TSEQ_INIT_MASK             (0x3FF0000U)
#define USBDCD_TIMER0_TSEQ_INIT_SHIFT            (16U)
#define USBDCD_TIMER0_TSEQ_INIT(x)               (((uint32_t)(((uint32_t)(x)) << USBDCD_TIMER0_TSEQ_INIT_SHIFT)) & USBDCD_TIMER0_TSEQ_INIT_MASK)

/*! @name TIMER1 - TIMER1 register */
#define USBDCD_TIMER1_TVDPSRC_ON_MASK            (0x3FFU)
#define USBDCD_TIMER1_TVDPSRC_ON_SHIFT           (0U)
#define USBDCD_TIMER1_TVDPSRC_ON(x)              (((uint32_t)(((uint32_t)(x)) << USBDCD_TIMER1_TVDPSRC_ON_SHIFT)) & USBDCD_TIMER1_TVDPSRC_ON_MASK)
#define USBDCD_TIMER1_TDCD_DBNC_MASK             (0x3FF0000U)
#define USBDCD_TIMER1_TDCD_DBNC_SHIFT            (16U)
#define USBDCD_TIMER1_TDCD_DBNC(x)               (((uint32_t)(((uint32_t)(x)) << USBDCD_TIMER1_TDCD_DBNC_SHIFT)) & USBDCD_TIMER1_TDCD_DBNC_MASK)

/*! @name TIMER2 - TIMER2 register */
#define USBDCD_TIMER2_CHECK_DM_MASK              (0xFU)
#define USBDCD_TIMER2_CHECK_DM_SHIFT             (0U)
#define USBDCD_TIMER2_CHECK_DM(x)                (((uint32_t)(((uint32_t)(x)) << USBDCD_TIMER2_CHECK_DM_SHIFT)) & USBDCD_TIMER2_CHECK_DM_MASK)
#define USBDCD_TIMER2_TVDPSRC_CON_MASK           (0x3FF0000U)
#define USBDCD_TIMER2_TVDPSRC_CON_SHIFT          (16U)
#define USBDCD_TIMER2_TVDPSRC_CON(x)             (((uint32_t)(((uint32_t)(x)) << USBDCD_TIMER2_TVDPSRC_CON_SHIFT)) & USBDCD_TIMER2_TVDPSRC_CON_MASK)


/*!
 * @}
 */ /* end of group USBDCD_Register_Masks */


/* USBDCD - Peripheral instance base addresses */
/** Peripheral USBDCD base address */
#define USBDCD_BASE                              (0x40035000u)
/** Peripheral USBDCD base pointer */
#define USBDCD                                   ((USBDCD_Type *)USBDCD_BASE)
/** Array initializer of USBDCD peripheral base addresses */
#define USBDCD_BASE_ADDRS                        { USBDCD_BASE }
/** Array initializer of USBDCD peripheral base pointers */
#define USBDCD_BASE_PTRS                         { USBDCD }
/** Interrupt vectors for the USBDCD peripheral type */
#define USBDCD_IRQS                              { USBDCD_IRQn }

/*!
 * @}
 */ /* end of group USBDCD_Peripheral_Access_Layer */


/* ----------------------------------------------------------------------------
   -- WDOG Peripheral Access Layer
   ---------------------------------------------------------------------------- */

/*!
 * @addtogroup WDOG_Peripheral_Access_Layer WDOG Peripheral Access Layer
 * @{
 */

/** WDOG - Register Layout Typedef */
typedef struct {
  __IO uint16_t STCTRLH;                           /**< Watchdog Status and Control Register High, offset: 0x0 */
  __IO uint16_t STCTRLL;                           /**< Watchdog Status and Control Register Low, offset: 0x2 */
  __IO uint16_t TOVALH;                            /**< Watchdog Time-out Value Register High, offset: 0x4 */
  __IO uint16_t TOVALL;                            /**< Watchdog Time-out Value Register Low, offset: 0x6 */
  __IO uint16_t WINH;                              /**< Watchdog Window Register High, offset: 0x8 */
  __IO uint16_t WINL;                              /**< Watchdog Window Register Low, offset: 0xA */
  __IO uint16_t REFRESH;                           /**< Watchdog Refresh register, offset: 0xC */
  __IO uint16_t UNLOCK;                            /**< Watchdog Unlock register, offset: 0xE */
  __IO uint16_t TMROUTH;                           /**< Watchdog Timer Output Register High, offset: 0x10 */
  __IO uint16_t TMROUTL;                           /**< Watchdog Timer Output Register Low, offset: 0x12 */
  __IO uint16_t RSTCNT;                            /**< Watchdog Reset Count register, offset: 0x14 */
  __IO uint16_t PRESC;                             /**< Watchdog Prescaler register, offset: 0x16 */
} WDOG_Type;

/* ----------------------------------------------------------------------------
   -- WDOG Register Masks
   ---------------------------------------------------------------------------- */

/*!
 * @addtogroup WDOG_Register_Masks WDOG Register Masks
 * @{
 */

/*! @name STCTRLH - Watchdog Status and Control Register High */
#define WDOG_STCTRLH_WDOGEN_MASK                 (0x1U)
#define WDOG_STCTRLH_WDOGEN_SHIFT                (0U)
#define WDOG_STCTRLH_WDOGEN(x)                   (((uint16_t)(((uint16_t)(x)) << WDOG_STCTRLH_WDOGEN_SHIFT)) & WDOG_STCTRLH_WDOGEN_MASK)
#define WDOG_STCTRLH_CLKSRC_MASK                 (0x2U)
#define WDOG_STCTRLH_CLKSRC_SHIFT                (1U)
#define WDOG_STCTRLH_CLKSRC(x)                   (((uint16_t)(((uint16_t)(x)) << WDOG_STCTRLH_CLKSRC_SHIFT)) & WDOG_STCTRLH_CLKSRC_MASK)
#define WDOG_STCTRLH_IRQRSTEN_MASK               (0x4U)
#define WDOG_STCTRLH_IRQRSTEN_SHIFT              (2U)
#define WDOG_STCTRLH_IRQRSTEN(x)                 (((uint16_t)(((uint16_t)(x)) << WDOG_STCTRLH_IRQRSTEN_SHIFT)) & WDOG_STCTRLH_IRQRSTEN_MASK)
#define WDOG_STCTRLH_WINEN_MASK                  (0x8U)
#define WDOG_STCTRLH_WINEN_SHIFT                 (3U)
#define WDOG_STCTRLH_WINEN(x)                    (((uint16_t)(((uint16_t)(x)) << WDOG_STCTRLH_WINEN_SHIFT)) & WDOG_STCTRLH_WINEN_MASK)
#define WDOG_STCTRLH_ALLOWUPDATE_MASK            (0x10U)
#define WDOG_STCTRLH_ALLOWUPDATE_SHIFT           (4U)
#define WDOG_STCTRLH_ALLOWUPDATE(x)              (((uint16_t)(((uint16_t)(x)) << WDOG_STCTRLH_ALLOWUPDATE_SHIFT)) & WDOG_STCTRLH_ALLOWUPDATE_MASK)
#define WDOG_STCTRLH_DBGEN_MASK                  (0x20U)
#define WDOG_STCTRLH_DBGEN_SHIFT                 (5U)
#define WDOG_STCTRLH_DBGEN(x)                    (((uint16_t)(((uint16_t)(x)) << WDOG_STCTRLH_DBGEN_SHIFT)) & WDOG_STCTRLH_DBGEN_MASK)
#define WDOG_STCTRLH_STOPEN_MASK                 (0x40U)
#define WDOG_STCTRLH_STOPEN_SHIFT                (6U)
#define WDOG_STCTRLH_STOPEN(x)                   (((uint16_t)(((uint16_t)(x)) << WDOG_STCTRLH_STOPEN_SHIFT)) & WDOG_STCTRLH_STOPEN_MASK)
#define WDOG_STCTRLH_WAITEN_MASK                 (0x80U)
#define WDOG_STCTRLH_WAITEN_SHIFT                (7U)
#define WDOG_STCTRLH_WAITEN(x)                   (((uint16_t)(((uint16_t)(x)) << WDOG_STCTRLH_WAITEN_SHIFT)) & WDOG_STCTRLH_WAITEN_MASK)
#define WDOG_STCTRLH_TESTWDOG_MASK               (0x400U)
#define WDOG_STCTRLH_TESTWDOG_SHIFT              (10U)
#define WDOG_STCTRLH_TESTWDOG(x)                 (((uint16_t)(((uint16_t)(x)) << WDOG_STCTRLH_TESTWDOG_SHIFT)) & WDOG_STCTRLH_TESTWDOG_MASK)
#define WDOG_STCTRLH_TESTSEL_MASK                (0x800U)
#define WDOG_STCTRLH_TESTSEL_SHIFT               (11U)
#define WDOG_STCTRLH_TESTSEL(x)                  (((uint16_t)(((uint16_t)(x)) << WDOG_STCTRLH_TESTSEL_SHIFT)) & WDOG_STCTRLH_TESTSEL_MASK)
#define WDOG_STCTRLH_BYTESEL_MASK                (0x3000U)
#define WDOG_STCTRLH_BYTESEL_SHIFT               (12U)
#define WDOG_STCTRLH_BYTESEL(x)                  (((uint16_t)(((uint16_t)(x)) << WDOG_STCTRLH_BYTESEL_SHIFT)) & WDOG_STCTRLH_BYTESEL_MASK)
#define WDOG_STCTRLH_DISTESTWDOG_MASK            (0x4000U)
#define WDOG_STCTRLH_DISTESTWDOG_SHIFT           (14U)
#define WDOG_STCTRLH_DISTESTWDOG(x)              (((uint16_t)(((uint16_t)(x)) << WDOG_STCTRLH_DISTESTWDOG_SHIFT)) & WDOG_STCTRLH_DISTESTWDOG_MASK)

/*! @name STCTRLL - Watchdog Status and Control Register Low */
#define WDOG_STCTRLL_INTFLG_MASK                 (0x8000U)
#define WDOG_STCTRLL_INTFLG_SHIFT                (15U)
#define WDOG_STCTRLL_INTFLG(x)                   (((uint16_t)(((uint16_t)(x)) << WDOG_STCTRLL_INTFLG_SHIFT)) & WDOG_STCTRLL_INTFLG_MASK)

/*! @name TOVALH - Watchdog Time-out Value Register High */
#define WDOG_TOVALH_TOVALHIGH_MASK               (0xFFFFU)
#define WDOG_TOVALH_TOVALHIGH_SHIFT              (0U)
#define WDOG_TOVALH_TOVALHIGH(x)                 (((uint16_t)(((uint16_t)(x)) << WDOG_TOVALH_TOVALHIGH_SHIFT)) & WDOG_TOVALH_TOVALHIGH_MASK)

/*! @name TOVALL - Watchdog Time-out Value Register Low */
#define WDOG_TOVALL_TOVALLOW_MASK                (0xFFFFU)
#define WDOG_TOVALL_TOVALLOW_SHIFT               (0U)
#define WDOG_TOVALL_TOVALLOW(x)                  (((uint16_t)(((uint16_t)(x)) << WDOG_TOVALL_TOVALLOW_SHIFT)) & WDOG_TOVALL_TOVALLOW_MASK)

/*! @name WINH - Watchdog Window Register High */
#define WDOG_WINH_WINHIGH_MASK                   (0xFFFFU)
#define WDOG_WINH_WINHIGH_SHIFT                  (0U)
#define WDOG_WINH_WINHIGH(x)                     (((uint16_t)(((uint16_t)(x)) << WDOG_WINH_WINHIGH_SHIFT)) & WDOG_WINH_WINHIGH_MASK)

/*! @name WINL - Watchdog Window Register Low */
#define WDOG_WINL_WINLOW_MASK                    (0xFFFFU)
#define WDOG_WINL_WINLOW_SHIFT                   (0U)
#define WDOG_WINL_WINLOW(x)                      (((uint16_t)(((uint16_t)(x)) << WDOG_WINL_WINLOW_SHIFT)) & WDOG_WINL_WINLOW_MASK)

/*! @name REFRESH - Watchdog Refresh register */
#define WDOG_REFRESH_WDOGREFRESH_MASK            (0xFFFFU)
#define WDOG_REFRESH_WDOGREFRESH_SHIFT           (0U)
#define WDOG_REFRESH_WDOGREFRESH(x)              (((uint16_t)(((uint16_t)(x)) << WDOG_REFRESH_WDOGREFRESH_SHIFT)) & WDOG_REFRESH_WDOGREFRESH_MASK)

/*! @name UNLOCK - Watchdog Unlock register */
#define WDOG_UNLOCK_WDOGUNLOCK_MASK              (0xFFFFU)
#define WDOG_UNLOCK_WDOGUNLOCK_SHIFT             (0U)
#define WDOG_UNLOCK_WDOGUNLOCK(x)                (((uint16_t)(((uint16_t)(x)) << WDOG_UNLOCK_WDOGUNLOCK_SHIFT)) & WDOG_UNLOCK_WDOGUNLOCK_MASK)

/*! @name TMROUTH - Watchdog Timer Output Register High */
#define WDOG_TMROUTH_TIMEROUTHIGH_MASK           (0xFFFFU)
#define WDOG_TMROUTH_TIMEROUTHIGH_SHIFT          (0U)
#define WDOG_TMROUTH_TIMEROUTHIGH(x)             (((uint16_t)(((uint16_t)(x)) << WDOG_TMROUTH_TIMEROUTHIGH_SHIFT)) & WDOG_TMROUTH_TIMEROUTHIGH_MASK)

/*! @name TMROUTL - Watchdog Timer Output Register Low */
#define WDOG_TMROUTL_TIMEROUTLOW_MASK            (0xFFFFU)
#define WDOG_TMROUTL_TIMEROUTLOW_SHIFT           (0U)
#define WDOG_TMROUTL_TIMEROUTLOW(x)              (((uint16_t)(((uint16_t)(x)) << WDOG_TMROUTL_TIMEROUTLOW_SHIFT)) & WDOG_TMROUTL_TIMEROUTLOW_MASK)

/*! @name RSTCNT - Watchdog Reset Count register */
#define WDOG_RSTCNT_RSTCNT_MASK                  (0xFFFFU)
#define WDOG_RSTCNT_RSTCNT_SHIFT                 (0U)
#define WDOG_RSTCNT_RSTCNT(x)                    (((uint16_t)(((uint16_t)(x)) << WDOG_RSTCNT_RSTCNT_SHIFT)) & WDOG_RSTCNT_RSTCNT_MASK)

/*! @name PRESC - Watchdog Prescaler register */
#define WDOG_PRESC_PRESCVAL_MASK                 (0x700U)
#define WDOG_PRESC_PRESCVAL_SHIFT                (8U)
#define WDOG_PRESC_PRESCVAL(x)                   (((uint16_t)(((uint16_t)(x)) << WDOG_PRESC_PRESCVAL_SHIFT)) & WDOG_PRESC_PRESCVAL_MASK)


/*!
 * @}
 */ /* end of group WDOG_Register_Masks */


/* WDOG - Peripheral instance base addresses */
/** Peripheral WDOG base address */
#define WDOG_BASE                                (0x40052000u)
/** Peripheral WDOG base pointer */
#define WDOG                                     ((WDOG_Type *)WDOG_BASE)
/** Array initializer of WDOG peripheral base addresses */
#define WDOG_BASE_ADDRS                          { WDOG_BASE }
/** Array initializer of WDOG peripheral base pointers */
#define WDOG_BASE_PTRS                           { WDOG }
/** Interrupt vectors for the WDOG peripheral type */
#define WDOG_IRQS                                { WDOG_EWM_IRQn }

/*!
 * @}
 */ /* end of group WDOG_Peripheral_Access_Layer */


/*
** End of section using anonymous unions
*/

#if defined(__ARMCC_VERSION)
  #pragma pop
#elif defined(__CWCC__)
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
   -- Macros for use with bit field definitions (xxx_SHIFT, xxx_MASK).
   ---------------------------------------------------------------------------- */

/*!
 * @addtogroup Bit_Field_Generic_Macros Macros for use with bit field definitions (xxx_SHIFT, xxx_MASK).
 * @{
 */

#if defined(__ARMCC_VERSION)
  #if (__ARMCC_VERSION >= 6010050)
    #pragma clang system_header
  #endif
#elif defined(__IAR_SYSTEMS_ICC__)
  #pragma system_include
#endif

/**
 * @brief Mask and left-shift a bit field value for use in a register bit range.
 * @param field Name of the register bit field.
 * @param value Value of the bit field.
 * @return Masked and shifted value.
 */
#define NXP_VAL2FLD(field, value)    (((value) << (field ## _SHIFT)) & (field ## _MASK))
/**
 * @brief Mask and right-shift a register value to extract a bit field value.
 * @param field Name of the register bit field.
 * @param value Value of the register.
 * @return Masked and shifted bit field value.
 */
#define NXP_FLD2VAL(field, value)    (((value) & (field ## _MASK)) >> (field ## _SHIFT))

/*!
 * @}
 */ /* end of group Bit_Field_Generic_Macros */


/* ----------------------------------------------------------------------------
   -- SDK Compatibility
   ---------------------------------------------------------------------------- */

/*!
 * @addtogroup SDK_Compatibility_Symbols SDK Compatibility
 * @{
 */

#define MCG_S_LOLS_MASK                   MCG_S_LOLS0_MASK
#define MCG_S_LOLS_SHIFT                  MCG_S_LOLS0_SHIFT
#define DAC0_IRQn                         This_symbol_has_been_deprecated
#define DAC_DATL_REG(base,index)          This_symbol_has_been_deprecated
#define DAC_DATH_REG(base,index)          This_symbol_has_been_deprecated
#define DAC_SR_REG(base)                  This_symbol_has_been_deprecated
#define DAC_C0_REG(base)                  This_symbol_has_been_deprecated
#define DAC_C1_REG(base)                  This_symbol_has_been_deprecated
#define DAC_C2_REG(base)                  This_symbol_has_been_deprecated
#define DAC_DATL_DATA0_MASK               This_symbol_has_been_deprecated
#define DAC_DATL_DATA0_SHIFT              This_symbol_has_been_deprecated
#define DAC_DATL_DATA0(x)                 This_symbol_has_been_deprecated
#define DAC_DATH_DATA1_MASK               This_symbol_has_been_deprecated
#define DAC_DATH_DATA1_SHIFT              This_symbol_has_been_deprecated
#define DAC_DATH_DATA1(x)                 This_symbol_has_been_deprecated
#define DAC_SR_DACBFRPBF_MASK             This_symbol_has_been_deprecated
#define DAC_SR_DACBFRPBF_SHIFT            This_symbol_has_been_deprecated
#define DAC_SR_DACBFRPTF_MASK             This_symbol_has_been_deprecated
#define DAC_SR_DACBFRPTF_SHIFT            This_symbol_has_been_deprecated
#define DAC_SR_DACBFWMF_MASK              This_symbol_has_been_deprecated
#define DAC_SR_DACBFWMF_SHIFT             This_symbol_has_been_deprecated
#define DAC_C0_DACBBIEN_MASK              This_symbol_has_been_deprecated
#define DAC_C0_DACBBIEN_SHIFT             This_symbol_has_been_deprecated
#define DAC_C0_DACBTIEN_MASK              This_symbol_has_been_deprecated
#define DAC_C0_DACBTIEN_SHIFT             This_symbol_has_been_deprecated
#define DAC_C0_DACBWIEN_MASK              This_symbol_has_been_deprecated
#define DAC_C0_DACBWIEN_SHIFT             This_symbol_has_been_deprecated
#define DAC_C0_LPEN_MASK                  This_symbol_has_been_deprecated
#define DAC_C0_LPEN_SHIFT                 This_symbol_has_been_deprecated
#define DAC_C0_DACSWTRG_MASK              This_symbol_has_been_deprecated
#define DAC_C0_DACSWTRG_SHIFT             This_symbol_has_been_deprecated
#define DAC_C0_DACTRGSEL_MASK             This_symbol_has_been_deprecated
#define DAC_C0_DACTRGSEL_SHIFT            This_symbol_has_been_deprecated
#define DAC_C0_DACRFS_MASK                This_symbol_has_been_deprecated
#define DAC_C0_DACRFS_SHIFT               This_symbol_has_been_deprecated
#define DAC_C0_DACEN_MASK                 This_symbol_has_been_deprecated
#define DAC_C0_DACEN_SHIFT                This_symbol_has_been_deprecated
#define DAC_C1_DACBFEN_MASK               This_symbol_has_been_deprecated
#define DAC_C1_DACBFEN_SHIFT              This_symbol_has_been_deprecated
#define DAC_C1_DACBFMD_MASK               This_symbol_has_been_deprecated
#define DAC_C1_DACBFMD_SHIFT              This_symbol_has_been_deprecated
#define DAC_C1_DACBFMD(x)                 This_symbol_has_been_deprecated
#define DAC_C1_DACBFWM_MASK               This_symbol_has_been_deprecated
#define DAC_C1_DACBFWM_SHIFT              This_symbol_has_been_deprecated
#define DAC_C1_DACBFWM(x)                 This_symbol_has_been_deprecated
#define DAC_C1_DMAEN_MASK                 This_symbol_has_been_deprecated
#define DAC_C1_DMAEN_SHIFT                This_symbol_has_been_deprecated
#define DAC_C2_DACBFUP_MASK               This_symbol_has_been_deprecated
#define DAC_C2_DACBFUP_SHIFT              This_symbol_has_been_deprecated
#define DAC_C2_DACBFUP(x)                 This_symbol_has_been_deprecated
#define DAC_C2_DACBFRP_MASK               This_symbol_has_been_deprecated
#define DAC_C2_DACBFRP_SHIFT              This_symbol_has_been_deprecated
#define DAC_C2_DACBFRP(x)                 This_symbol_has_been_deprecated
#define DAC0_BASE                         This_symbol_has_been_deprecated
#define DAC0                              This_symbol_has_been_deprecated
#define DAC_BASE_ADDRS                    This_symbol_has_been_deprecated
#define DAC_IRQS                          This_symbol_has_been_deprecated
#define DAC0_DAT0L                        This_symbol_has_been_deprecated
#define DAC0_DAT0H                        This_symbol_has_been_deprecated
#define DAC0_DAT1L                        This_symbol_has_been_deprecated
#define DAC0_DAT1H                        This_symbol_has_been_deprecated
#define DAC0_DAT2L                        This_symbol_has_been_deprecated
#define DAC0_DAT2H                        This_symbol_has_been_deprecated
#define DAC0_DAT3L                        This_symbol_has_been_deprecated
#define DAC0_DAT3H                        This_symbol_has_been_deprecated
#define DAC0_DAT4L                        This_symbol_has_been_deprecated
#define DAC0_DAT4H                        This_symbol_has_been_deprecated
#define DAC0_DAT5L                        This_symbol_has_been_deprecated
#define DAC0_DAT5H                        This_symbol_has_been_deprecated
#define DAC0_DAT6L                        This_symbol_has_been_deprecated
#define DAC0_DAT6H                        This_symbol_has_been_deprecated
#define DAC0_DAT7L                        This_symbol_has_been_deprecated
#define DAC0_DAT7H                        This_symbol_has_been_deprecated
#define DAC0_DAT8L                        This_symbol_has_been_deprecated
#define DAC0_DAT8H                        This_symbol_has_been_deprecated
#define DAC0_DAT9L                        This_symbol_has_been_deprecated
#define DAC0_DAT9H                        This_symbol_has_been_deprecated
#define DAC0_DAT10L                       This_symbol_has_been_deprecated
#define DAC0_DAT10H                       This_symbol_has_been_deprecated
#define DAC0_DAT11L                       This_symbol_has_been_deprecated
#define DAC0_DAT11H                       This_symbol_has_been_deprecated
#define DAC0_DAT12L                       This_symbol_has_been_deprecated
#define DAC0_DAT12H                       This_symbol_has_been_deprecated
#define DAC0_DAT13L                       This_symbol_has_been_deprecated
#define DAC0_DAT13H                       This_symbol_has_been_deprecated
#define DAC0_DAT14L                       This_symbol_has_been_deprecated
#define DAC0_DAT14H                       This_symbol_has_been_deprecated
#define DAC0_DAT15L                       This_symbol_has_been_deprecated
#define DAC0_DAT15H                       This_symbol_has_been_deprecated
#define DAC0_SR                           This_symbol_has_been_deprecated
#define DAC0_C0                           This_symbol_has_been_deprecated
#define DAC0_C1                           This_symbol_has_been_deprecated
#define DAC0_C2                           This_symbol_has_been_deprecated
#define DAC0_DATL(index)                  This_symbol_has_been_deprecated
#define DAC0_DATH(index)                  This_symbol_has_been_deprecated
#define DMA_EARS_REG(base)                This_symbol_has_been_deprecated
#define DMA_EARS_EDREQ_0_MASK             This_symbol_has_been_deprecated
#define DMA_EARS_EDREQ_0_SHIFT            This_symbol_has_been_deprecated
#define DMA_EARS_EDREQ_1_MASK             This_symbol_has_been_deprecated
#define DMA_EARS_EDREQ_1_SHIFT            This_symbol_has_been_deprecated
#define DMA_EARS_EDREQ_2_MASK             This_symbol_has_been_deprecated
#define DMA_EARS_EDREQ_2_SHIFT            This_symbol_has_been_deprecated
#define DMA_EARS_EDREQ_3_MASK             This_symbol_has_been_deprecated
#define DMA_EARS_EDREQ_3_SHIFT            This_symbol_has_been_deprecated
#define DMA_EARS_EDREQ_4_MASK             This_symbol_has_been_deprecated
#define DMA_EARS_EDREQ_4_SHIFT            This_symbol_has_been_deprecated
#define DMA_EARS_EDREQ_5_MASK             This_symbol_has_been_deprecated
#define DMA_EARS_EDREQ_5_SHIFT            This_symbol_has_been_deprecated
#define DMA_EARS_EDREQ_6_MASK             This_symbol_has_been_deprecated
#define DMA_EARS_EDREQ_6_SHIFT            This_symbol_has_been_deprecated
#define DMA_EARS_EDREQ_7_MASK             This_symbol_has_been_deprecated
#define DMA_EARS_EDREQ_7_SHIFT            This_symbol_has_been_deprecated
#define DMA_EARS_EDREQ_8_MASK             This_symbol_has_been_deprecated
#define DMA_EARS_EDREQ_8_SHIFT            This_symbol_has_been_deprecated
#define DMA_EARS_EDREQ_9_MASK             This_symbol_has_been_deprecated
#define DMA_EARS_EDREQ_9_SHIFT            This_symbol_has_been_deprecated
#define DMA_EARS_EDREQ_10_MASK            This_symbol_has_been_deprecated
#define DMA_EARS_EDREQ_10_SHIFT           This_symbol_has_been_deprecated
#define DMA_EARS_EDREQ_11_MASK            This_symbol_has_been_deprecated
#define DMA_EARS_EDREQ_11_SHIFT           This_symbol_has_been_deprecated
#define DMA_EARS_EDREQ_12_MASK            This_symbol_has_been_deprecated
#define DMA_EARS_EDREQ_12_SHIFT           This_symbol_has_been_deprecated
#define DMA_EARS_EDREQ_13_MASK            This_symbol_has_been_deprecated
#define DMA_EARS_EDREQ_13_SHIFT           This_symbol_has_been_deprecated
#define DMA_EARS_EDREQ_14_MASK            This_symbol_has_been_deprecated
#define DMA_EARS_EDREQ_14_SHIFT           This_symbol_has_been_deprecated
#define DMA_EARS_EDREQ_15_MASK            This_symbol_has_been_deprecated
#define DMA_EARS_EDREQ_15_SHIFT           This_symbol_has_been_deprecated
#define DMA_EARS                          This_symbol_has_been_deprecated
#define DSPI0                             SPI0
#define DSPI1                             SPI1
#define DMAMUX0                           DMAMUX
#define WP7816_T_TYPE0                    WP7816T0
#define WP7816_T_TYPE1                    WP7816T1
#define UART_WP7816_T_TYPE0_REG(base)     UART_WP7816T0_REG(base)
#define UART_WP7816_T_TYPE1_REG(base)     UART_WP7816T1_REG(base)
#define UART_WP7816_T_TYPE0_WI_MASK       UART_WP7816T0_WI_MASK
#define UART_WP7816_T_TYPE0_WI_SHIFT      UART_WP7816T0_WI_SHIFT
#define UART_WP7816_T_TYPE0_WI(X)         UART_WP7816T0_WI(X)
#define UART_WP7816_T_TYPE1_BWI_MASK      UART_WP7816T1_BWI_MASK
#define UART_WP7816_T_TYPE1_BWI_SHIFT     UART_WP7816T1_BWI_SHIFT
#define UART_WP7816_T_TYPE1_BWI(X)        UART_WP7816T1_BWI(X)
#define UART_WP7816_T_TYPE1_CWI_MASK      UART_WP7816T1_CWI_MASK
#define UART_WP7816_T_TYPE1_CWI_SHIFT     UART_WP7816T1_CWI_SHIFT
#define UART_WP7816_T_TYPE1_CWI(X)        UART_WP7816T1_CWI(X)
#define SIM_SCGC6_DAC0_MASK               This_symbol_has_been_deprecated
#define SIM_SCGC6_DAC0_SHIFT              This_symbol_has_been_deprecated
#define Watchdog_IRQn                     WDOG_EWM_IRQn
#define Watchdog_IRQHandler               WDOG_EWM_IRQHandler
#define LPTimer_IRQn                      LPTMR0_IRQn
#define LPTimer_IRQHandler                LPTMR0_IRQHandler
#define LLW_IRQn                          LLWU_IRQn
#define LLW_IRQHandler                    LLWU_IRQHandler

/*!
 * @}
 */ /* end of group SDK_Compatibility_Symbols */


#endif  /* _MKW24D5_H_ */

